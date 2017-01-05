/* Copyright (c) 2010, Simeon Bird <spb41@cam.ac.uk>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */
#include "gadgetreader.hpp"
#include "read_utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

//Swap the endianness of the header correctly.
//doubles need endian swapping differently to ints.
void header_endian_swap(gadget_header * header)
{
    //Pointer to start of header
    void * ptr = (void *) &(header->npart[0]);
    //npart.
    ptr = multi_endian_swap((uint32_t *)ptr, 6);
    //mass array, redshift and time.
    ptr = multi_endian_swap64((uint64_t *)ptr, 8);
    //Various flags
    ptr = multi_endian_swap((uint32_t *)ptr, 10);
    //cosmology
    ptr = multi_endian_swap64((uint64_t *)ptr, 4);
    //Remaining material
    ptr = multi_endian_swap((uint32_t *)ptr, (sizeof(gadget_header)-11*4-12*8)/4);
}


/** \file 
 * GadgetReader library method file*/
namespace GadgetReader{

/*Error output macros*/
#define ERROR(...) do{ fprintf(stderr,__VA_ARGS__);abort();}while(0)
#define WARN(...) do{ \
        if(debug){ \
                fprintf(stderr,"[GadgetReader]: "); \
                fprintf(stderr, __VA_ARGS__); \
        }}while(0)
  //Constructor; this does almost all the hard work of building a "map" of the block positions
  GSnap::GSnap(std::string snap_filename, bool debug, std::vector<std::string> *BlockNames): debug(debug)
  {
        f_name first_file=snap_filename;
        FILE *fd;
        int64_t npart[N_TYPE];
        int files_expected=1;
        bad_head64=false;

        //Try to open the file 
        if(!(fd=fopen(first_file.c_str(),"r"))){
                //Append ".0" to the filename if necessary.
                first_file+=".0";
                //and then try again
                if(!(fd=fopen(first_file.c_str(),"r"))){
                        WARN("Could not open %s (.0)\n",snap_filename.c_str());
                        WARN("Does not exist, or is corrupt.\n");
                        return;
                }
        }
        fclose(fd);
        //Read the first file
        GSnapFile first_map(first_file, debug,BlockNames);
        //Set the global variables. 
        base_filename=first_file;
        //Take the ".0" from the end if needed.
        if(base_filename.compare(base_filename.size()-2,2,".0")==0){
                std::string::iterator it=base_filename.end();
                base_filename.erase(it-2,it);
        }
        //Put the information from the first file in
        file_maps.push_back(first_map);
        //Add the particles for this file to the totals
        for(int j=0; j<N_TYPE; j++)
           npart[j]=file_maps[0].header.npart[j];
        files_expected=first_map.header.num_files;
        if(files_expected < 1 || files_expected > 999){
                WARN("Implausible number of files supposedly in simulation set: %d\n",files_expected);
                return;
        }
        if(!BlockNames && !first_map.format_2 )
               WARN("WARNING: Reading Gadget-I file using pre-computed\nblock order, which may not correspond to the actual order of your file!\n");
        //Get the information for the other files.
        for(int i=1;i<files_expected;i++){
                f_name c_name=base_filename;
                char tmp[6];
                snprintf(tmp,6,".%d",i);
                c_name+=(std::string(tmp));
                GSnapFile tmp_map(c_name,debug, BlockNames);
                if(tmp_map.GetNumBlocks() ==0 || !check_headers(tmp_map.header,file_maps[0].header)){
                        WARN("Headers inconsistent between file 0 and file %d, ignoring file %d\n",i,i);
                        continue;
                }
                file_maps.push_back(tmp_map);
                for(int j=0; j<N_TYPE; j++)
                   npart[j]+=file_maps[i].header.npart[j];
        }
        //Check we have found as many particles as we were expecting.
        for(int i=0; i<N_TYPE; i++){
            /*If we have few enough particles to fit into 32 bits and our header is 
             * strange, set bad_head64, indicating that our long word is bogus.*/
            if(npart[i] != GetNpart(i) && npart[i] == file_maps[0].header.npartTotal[i]){
                   WARN("Detected bogus value for 64-bit particle number in header!\n"); 
                   bad_head64=true;
            }
            if(npart[i] != GetNpart(i))
                    WARN("Expected %ld particles of Type %d, but found %ld!\n",GetNpart(i),i,npart[i]);
        }
        return;
  }

  //This takes an open file and constructs a map of where the blocks are within it, and then returns said map.
  //It ought to support endian swapped files as well as Gadget-I files.
   GSnapFile::GSnapFile(const f_name strfile, bool debug, std::vector<std::string>* BlockNames) : name(strfile),debug(debug)
  {
          //Default ordering of blocks for Gadget-I files
          //The odd double declaration of a string array is to work around lack of initialiser lists in C++ 98.
          const char * default_blocks_char[12] = {"HEAD","POS ","VEL ","ID  ","MASS","U   ","RHO ","NE  ","NH  ","NHE ","HSML","SFR "};
          std::vector<std::string> default_blocks(default_blocks_char, default_blocks_char+12);
          unsigned int cur_block=0;
          FILE * fd; 
          uint32_t record_size;
          /*Total number of particles in file*/
          total_file_part=0;
          const char * file=strfile.c_str();
          if(!(fd=fopen(strfile.c_str(),"r")) || check_filetype(fd)){
                  WARN("Could not open %s (.0)\n",strfile.c_str());
                  WARN("Does not exist, or is corrupt.\n");
                  if(fd)
                      fclose(fd);
                  return;
          }
          header.num_files = -1;
          //Read now until we run out of file
          while(!feof(fd)){
                  block_info c_info;
                  char c_name[5]={"    "};
                  //Read another block header
                  if(format_2){
                     c_info.length=read_block_head(c_name,fd,file);
                     if(c_info.length ==0)
                             break;//We have run out of file
                  }
                  else{
                    /*For Gadget-I files, we have to settle for less certainty, 
                     * and read the name from a pre-guessed table, 
                     * or a user-supplied input table,
                     * or just make something up if all else fails.*/
                    if(BlockNames == NULL && cur_block< default_blocks.size())
                                    strncpy(c_name, default_blocks[cur_block++].c_str(),5);
                    else if(BlockNames != NULL && cur_block< (*BlockNames).size()) {
                            strncpy(c_name, (*BlockNames)[cur_block++].c_str(),5);
                            c_name[4]='\0';
                    }
                    else {
                            c_name[0]=(char) 65-default_blocks.size()+cur_block;
                    }
                    /* Read the length from the record length*/
                    if(fread(&record_size,sizeof(uint32_t),1,fd)!=1)
                            break;//out of file
                    if(swap_endian) endian_swap(&record_size);
                    /*Because c_info.length needs to be 64 bits*/
                    c_info.length=record_size;
                  }
                  //Do special things for the HEAD block
                  if(strncmp(c_name,"HEAD",4) ==0){
                          //Read the actual header. 
                          if(c_info.length != sizeof(gadget_header)){
                                  WARN("Mis-sized HEAD block in %s\n",file);
                          }
                          if((fread(&header,c_info.length,1,fd)!=1) ||
                          (fread(&record_size,sizeof(uint32_t),1,fd)!=1)){
                                  WARN("Could not read HEAD, skipping file: %s!\n",file);
                                  break;
                          }
                          if(swap_endian){
                              endian_swap(&record_size);
                              header_endian_swap(&header);
                          }
                          if(record_size != sizeof(gadget_header)){
                                 WARN("Bad record size for HEAD in %s!\n",file);
                                 break;
                          }
                          /*Set the total_file_part local variable*/
                          for(int i=0; i<N_TYPE; i++)
                                  total_file_part+=header.npart[i];
                          //If using format 1 and the mass is in the header block, skip the mass block.
                          if(!format_2){
                            bool all_masses_in_header = true;
                            //Check that all masses are in the header
                            for(int i=0; i<N_TYPE; i++){
                                if(header.mass[i] == 0 && header.npart[i] > 0){
                                    all_masses_in_header = false;
                                }
                            }
                            //If so, remove the mass block from the default list
                            if(all_masses_in_header) {
                                std::vector<std::string>::iterator it = std::find(default_blocks.begin(), default_blocks.end(),"MASS");
                                if(it != default_blocks.end())
                                    default_blocks.erase(it);
                            }
                          }
                          //Next block
                          continue;
                  }
                  //If POS or VEL block, 3 floats per particle.
                  if(strncmp(c_name,"POS ",4)==0 || strncmp(c_name,"VEL ",4)==0) {
                      //Sometimes blocks are in double precision.
                      if (c_info.length == 3*total_file_part*sizeof(double) )
                          c_info.partlen = 3*sizeof(double);
                      else
                        c_info.partlen=3*sizeof(float);
                  }
                  /*A heuristic to detect LongIDs. Won't always work.*/
                  else if(strncmp(c_name,"ID  ",4)==0){
                          if(c_info.length == total_file_part*sizeof(int32_t))
                                c_info.partlen=sizeof(int32_t);
                          else
                                c_info.partlen=sizeof(int64_t);
                  }
                  //Otherwise one float per particle.
                  else {
                      if (c_info.length == total_file_part*sizeof(double) )
                          c_info.partlen = sizeof(double);
                      else
                        c_info.partlen = sizeof(float);
                  }
                  //Store the start of the data block
                  c_info.start_pos=ftell(fd);
                  /*Check for the case where the record size overflows an int.
                   * If this is true, we can't get record size from the length and we just have to guess
                   * At least the record sizes at either end should be consistently wrong. */
                  /* Better hope this only happens for blocks where all particles are present.*/
                  uint64_t extra_len=total_file_part*c_info.partlen;
                  if(extra_len >= ((uint64_t)1)<<32){
                          WARN("Block %s was longer than could fit in %lu bytes.\n",c_name,sizeof(uint32_t));
                          WARN("Guessed size of %lu from header\n",extra_len);
                          /*Seek the rest of the block*/
                          fseek(fd,extra_len,SEEK_CUR);
                  }
                  else
                          fseek(fd,c_info.length,SEEK_CUR);
                  if((fread(&record_size,sizeof(uint32_t),1,fd)!=1) ||
                      ( !swap_endian && record_size != c_info.length) ||
                      ( swap_endian && endian_swap(&record_size) != c_info.length)){
                          WARN("Corrupt record in %s footer for block %s (%lu vs %u), skipping rest of file\n",file, c_name, c_info.length, record_size);
                          break;
                  }
                  /*Store new block length*/
                  if(extra_len >= ((uint64_t)1)<<32)
                        c_info.length=extra_len;
                  // Set up the particle types in the block. This also is a heuristic,
                  // which assumes that blocks are either fully present or not for a given particle type
                  if(!SetBlockTypes(c_info)){
                        WARN("SetBlockTypes failed for block %s in file %s, length %lu\n",c_name,file,c_info.length);
                        continue;
                  }
                  //Append the current info to the map; if there are duplicates move ahead one.
                  while(blocks.count(c_name) > 0){
                          c_name[3]++;
                  }
                  blocks[std::string(c_name)] = c_info;
          }
          fclose(fd);
          return;
  }
 
 /* Read the header of a Gadget file. This is the "block header" not the file header, and gives the 
  * name and length of the block.
  * Return integer length of block. Arguments are:
  * name - place to store block name
  * fd - file descriptor
  * file - filename of open file */
 uint32_t GSnapFile::read_block_head(char* name, FILE *fd, const char * file)
 {
        uint32_t head[5];
        //Read the "block header" record, only present on Gadget-II files. 
        //Has format: 
        //size (8), name (HEAD), length (256), size (8)
        if(fread(&head,sizeof(uint32_t),5,fd)!=5)
           //If we have run out of file, we can just return.
           return 0;
        if(swap_endian)  multi_endian_swap(&head[0],5);
        if(head[0] != 8 || head[3] !=8){
                WARN("Corrupt header record in file %s.\n",file);
                WARN("Record length is: %u and ended as %u\n",head[0],head[3]);
                WARN("Header bytes are: %s %u\n",(char *) &head[1],head[2]);
                WARN("Maybe trying to read old-format file as new-format?\n");
                return 0;
        }
        if(head[4] != head[2]-2*sizeof(uint32_t)){
                WARN("Corrupt record in %s header for block %s (%lu vs %u), skipping rest of file\n",file,(char *) &head[1], head[2]-2*sizeof(uint32_t), head[4]);
                return 0;
        }
        strncpy(name,(char *)&head[1],4);
        //Null-terminate the string
        *(name+5)='\0';
        //Don't include the two "record_size" indicators in the total length count
        return head[2]-2*sizeof(uint32_t);
  }

  /*Sets swap_endian and format_2. Returns 0 for success, 1 for an empty file, and 2 
   * if the filetype is weird (ie, if you tried to open a text file by mistake)*/
  int GSnapFile::check_filetype(FILE* fd)
  {
          uint32_t record_size;
          //Start at the beginning.
          rewind(fd);
          //Read the first integer
          if(fread(&record_size,sizeof(int32_t),1,fd)!=1){
                 WARN("Empty file: Could not read first integer\n");
                 return 1;
          }
          swap_endian = false;
          switch(record_size){
                  //For a valid Gadget-II file, the first integer will be 8.
                  //If the endianness is swapped
                  //endian_swap(8)
                  case 134217728:
                      WARN("WARNING: Endianness swapped\n");
                      swap_endian=true;
                  case 8:
                      format_2=true;
                      break; 
                  //If it is a format 1 file, the first record ought 
                  //to be the header, which ought to be 256 bytes long.
                  //OR the endian swapped version of that.
                  //endian_swap(256):
                  case 65536:
                  //A Gadget-I format file with the header first, 
                  //endian swapped.
                       WARN("WARNING: Endianness swapped\n");
                       swap_endian=true;
                  case 256:
                  //A Gadget-I format file with the header first.
                       format_2=false;
                       break;
                  default:
                       WARN("WARNING: File corrupt! First integer is: %d\n",record_size);
                       rewind(fd);
                       return 2;
          }
          //Put the file back to the beginning again, for simplicity.
          rewind(fd);
          return 0;
  }
  
  /*Check the consistency of file headers. This is just a short sanity check 
   * to make sure the user hasn't put two entirely different simulations with the same 
   * snapshot name in the same directory or something*/
  bool GSnap::check_headers(gadget_header head1, gadget_header head2)
  {
    /*Check single quantities*/
    /*Even the floats ought to be really identical if we have read them from disc*/
    if(head1.time != head2.time ||
       head1.redshift!= head2.redshift ||
       head1.flag_sfr != head2.flag_sfr ||
       head1.flag_feedback != head2.flag_feedback ||
       head1.num_files != head2.num_files ||
       head1.BoxSize != head2.BoxSize ||
       head1.Omega0 != head2.Omega0 ||
       head1.OmegaLambda != head2.OmegaLambda ||
       head1.HubbleParam != head2.HubbleParam  || 
       head1.flag_stellarage != head2.flag_stellarage ||
       head1.flag_metals != head2.flag_metals)
            return false;
    /*Check array quantities*/
    for(int i=0; i<6; i++)
            if(head1.mass[i] != head2.mass[i] ||
                head1.npartTotal[i] != head2.npartTotal[i])// ||
//                head1.NallHW[i] != head2.NallHW[i])
                    return false;
/*                At least one version of N-GenICs writes a header file which 
                ignores everything past flag_metals (!), leaving it uninitialised. 
                Therefore, we can't check them. */
    return true;
  }

  /* Gets the total number of particles in a simulation. */
  int64_t GSnap::GetNpart(int type, bool found)
  {
          int64_t npart=0;
          if(type >= N_TYPE || type < 0 || file_maps.size() == 0) 
                  return 0;
          if(found){
                  for(unsigned int i=0; i<file_maps.size(); i++)
                          npart+=file_maps[i].header.npart[type];
                  return npart;
          }
          //Get the part that doesn't fit in 32 bits, if it isn't bogus
          if(!bad_head64)
                  npart=file_maps[0].header.NallHW[type];
          //Bitshift it and add on the other part
          return (npart << 32) + file_maps[0].header.npartTotal[type];
  }

  /*Check the given block exists*/
  bool GSnap::IsBlock(std::string BlockName)
  {
        //Find total number of particles needed
        for(unsigned int i=0; i<file_maps.size(); i++)
                if(file_maps[i].blocks.count(BlockName))
                        return true;
        return false;
  }
  
  /*Get number of particles a block has data for*/
  int64_t GSnap::GetBlockParts(std::string BlockName)
  {
        int64_t size=0;
        //Find total number of particles needed
        for(unsigned int i=0; i<file_maps.size(); i++)
                if(file_maps[i].blocks.count(BlockName))
                        size+=file_maps[i].blocks[BlockName].length/file_maps[i].blocks[BlockName].partlen;
        return size;
  }

  /*Get total size of a block in the snapshot, in bytes*/
  int64_t GSnap::GetBlockSize(std::string BlockName, int type)
  {
        int64_t size=0;
        //Find total number of particles needed
        for(unsigned int i=0; i<file_maps.size(); i++)
                if(file_maps[i].blocks.count(BlockName)){
                        if(type >= 0 && type < N_TYPE)
                                size+=file_maps[i].header.npart[type]*file_maps[i].blocks[BlockName].partlen;
                        else
                                size+=file_maps[i].blocks[BlockName].length;
                }
        return size;
  }

  /*Get a set of the block names in the snapshot*/
  std::set<std::string> GSnap::GetBlocks()
  {
          std::map<std::string,block_info>::iterator it;
          std::set<std::string> names;
          for(unsigned int i=0; i<file_maps.size();i++)
                for(it=file_maps[i].blocks.begin() ; it != file_maps[i].blocks.end(); it++)
                          names.insert((*it).first);
          return names;
  }

  /* Reads particles from a file block into the memory pointed to by block
   * Returns the number of particles read.
   * Takes: block name, 
   *        particles to read, 
   *        pointer to allocated memory for block
   *        particles to skip initially
   *        Types to skip, as a bitfield. 
   *        Pass 1 to skip baryons, 3 to skip baryons and dm, 2 to skip dm, etc.
   *        Only skip types for which the block is actually present:
   *        Unfortunately there is no way of the library knowing which particle has which type, so 
   *        there is no way of telling that in advance.  */
  int64_t GSnap::GetBlock(std::string BlockName, void *block, int64_t npart_toread, int64_t start_part, int skip_type)
  {
        int64_t npart_read=0;
        //Check the block really exists
        if(!IsBlock(BlockName)){
                WARN("Block %s is not in this snapshot\n",BlockName.c_str());
                return 0;
        }
        //Read a chunk of particles from a file
        for(unsigned int i=0;i<file_maps.size(); i++){
                uint32_t read_data,npart_file;
                int64_t start_pos=0;
                FILE *fd;
                block_info cur_block;
                //Get current block
                if(file_maps[i].blocks.count(BlockName))
                        cur_block=file_maps[i].blocks[BlockName];
                else{
                       WARN("Block %s not in file %d\n",BlockName.c_str(),i);
                       continue;
                }
                npart_file = cur_block.length/cur_block.partlen;
                //Don't want to read the skip_types
                for(int j=0; j<N_TYPE;j++)
                        if(skip_type & (1 << j))
                                npart_file-=file_maps[i].header.npart[j];
                //So now we have the amount of data to read, and we want to find the starting position 
                start_pos=cur_block.start_pos;
                //Don't want to read the skip_types before the start of our first type.
                for(int j=0; j<N_TYPE;j++){
                        if(skip_type & (1 << j))
                                start_pos+=file_maps[i].header.npart[j]*cur_block.partlen;
                        else 
                                break;
                }
                //Also skip however many start_parts are left.
                if(start_part > 0){
                        //If the skip is larger than this file, go onto the next file.
                        if(npart_file <= start_part){
                                start_part-=npart_file;
                                continue;
                        }
                        //Otherwise add something to start_pos and take it off npart_file
                        start_pos+=start_part*cur_block.partlen;
                        npart_file-=start_part;
                        start_part =0;
                }
                //There is a maximum amount of particles we want to read. 
                //If we have reached it, truncate.
                if(npart_file > npart_toread-npart_read)
                       npart_file=npart_toread-npart_read;
                                
                //Open file: If this fails skip to the next file.
                if(!(fd=fopen(file_maps[i].name.c_str(),"r"))){
                        WARN("Could not open file %d of %lu, continuing\n",i,file_maps.size());
                        continue;
                }
                //Check whether need to swap endianness
                bool swap_endian = file_maps[i].GetFormat() & 2;
                //Seek to first particle
                if(fseek(fd,start_pos,SEEK_SET) == -1)
                        WARN("Failed to seek\n");
                //Read the data!
                read_data=fread(((char *)block)+npart_read*cur_block.partlen,cur_block.partlen,npart_file,fd);
                if(swap_endian){
                    //Swap the endianness of the data.
                    //If we have 1 64-bit entry (say, particle ID) swap it 64-bit wise.
                    //FIXME: This will fail for, eg, 3D 64-bit entries.
                    //Otherwise, swap it 32-bit wise.
                    if (cur_block.partlen == 8)
                        multi_endian_swap64((uint64_t *)block+npart_read*cur_block.partlen,npart_file);
                    else
                        multi_endian_swap((uint32_t *)block+npart_read*cur_block.partlen,npart_file*cur_block.partlen/4);
                }
                //Don't die if we read the wrong amount of data; maybe we can find it in the next file.
                if(read_data !=npart_file)
                        WARN("Only read %u particles of %u from file %d\n",read_data,npart_file,i);
                fclose(fd);
                npart_read+=read_data;
                if(npart_read == npart_toread) //We have enough
                        break;
        }
        if(npart_toread > npart_read){
                WARN("Read %ld particles out of %ld\n",npart_read,npart_toread);
        }
        return npart_read;
  }

  /* Return a header*/
  gadget_header GSnap::GetHeader(int i)
  {
        if((int)file_maps.size()<= i){
                gadget_header head;
                head.num_files=-1;
                head.npart[0]=0;
                head.npart[1]=0;
                return head;
        }
        return file_maps[i].header;
  }

  /*Set the length per particle*/
  void GSnap::SetPartLen(std::string BlockName, short partlen)
  {
          for(unsigned int i=0; i<file_maps.size();i++){
                  if(file_maps[i].blocks.count(BlockName))
                          file_maps[i].blocks[BlockName].partlen=partlen;
          }
          return;
  }
  
  #define MIN_READ_SPLIT 6000
  /*Memory-safe wrapper functions for the bindings. It is not anticipated that people writing codes in C 
   * will want to use these, as they need to allocate a significant quantity of temporary memory.
   * We do not attempt to work out whether the block requested is a float or an int.*/
  std::vector<float> GSnap::GetBlock(std::string BlockName, int64_t npart_toread, int64_t start_part, int skip_type)
  {
          std::vector<float> data;
          float* block=NULL;
          int64_t read_chunk, total_read=0, read;
          short partlen;
          if(!IsBlock(BlockName))
                  return data;
          partlen=file_maps[0].blocks[BlockName].partlen;
          /*If it is a small amount of data, read it all at once*/
          if(npart_toread < MIN_READ_SPLIT)
                  read_chunk=npart_toread;
          else
          /*Read a third of the data at a time.*/
                  read_chunk=MIN_READ_SPLIT+1;
          /*Allocate memory*/
          if(!(block=(float *)malloc(read_chunk*partlen))){
                  WARN("Could not allocate temporary memory for particles!\n");
                  return data;
          }
          /*Read a segment*/
          while(total_read < npart_toread){
                  /*For final segment to avoid off-by-one error*/
                  if(read_chunk > npart_toread-total_read)
                          read_chunk=npart_toread-total_read;
                  read= GetBlock(BlockName, block, read_chunk, start_part+total_read, skip_type);
                  /*Assume we read all of them; otherwise if one day we fail to read, 
                   * the loop will not stop*/
                  total_read+=read_chunk;
                  /*Append what we have to the vector*/
                  for(uint64_t i=0; i< read*partlen/sizeof(float); i++)
                          data.push_back(block[i]);
          }
          free(block);
          return data;
  }

  /*Support getting IDs: is exactly the same as the above*/
  std::vector<long long> GSnap::GetBlockInt(std::string BlockName, int64_t npart_toread, int64_t start_part, int skip_type)
  {
          std::vector<long long> data;
          int64_t* block=NULL;
          int64_t read_chunk, total_read=0, read;
          short partlen;
          if(!IsBlock(BlockName))
                  return data;
          partlen=file_maps[0].blocks[BlockName].partlen;
          /*If it is a small amount of data, read it all at once*/
          if(npart_toread < MIN_READ_SPLIT)
                  read_chunk=npart_toread;
          else
          /*Read a third of the data at a time.*/
                  read_chunk=MIN_READ_SPLIT+1;
          /*Allocate memory*/
          if(!(block=(int64_t *)malloc(read_chunk*partlen))){
                  WARN("Could not allocate temporary memory for particles!\n");
                  return data;
          }
          /*Read a segment*/
          while(total_read < npart_toread){
                  /*For final segment to avoid off-by-one error*/
                  if(read_chunk > npart_toread-total_read)
                          read_chunk=npart_toread-total_read;
                  read= GetBlock(BlockName, block, read_chunk, start_part+total_read, skip_type);
                  /*Assume we read all of them; otherwise if one day we fail to read, 
                   * the loop will not stop*/
                  total_read+=read_chunk;
                  /*Append what we have to the vector*/
                  if(partlen == sizeof(int)){
                        int* sblock=(int *) block;
                        for(uint64_t i=0; i< read*partlen/sizeof(int); i++)
                          data.push_back(sblock[i]);
                  }
                  /*Make it a 64-bit pointer*/
                  else{
                        for(uint64_t i=0; i< read*partlen/sizeof(int64_t); i++)
                          data.push_back(block[i]);
                  }
          }
          free(block);
          return data;
  }

  bool GSnapFile::SetBlockTypes(block_info block)
  {
        /* Set up the particle types in the block, with a heuristic,
        which assumes that blocks are either fully present or not for a given particle type */
        for (int i=0; i<N_TYPE; i++)
                block.p_types[i] = false;
        if ((int64_t)block.length/block.partlen == total_file_part){
                for (int i=0; i<N_TYPE; i++)
                        block.p_types[i] = true;
                return true;
        }
        //Blocks which contain a single particle type
        for (int n=0; n<N_TYPE; n++){
            if (block.length == header.npart[n]*block.partlen){
                block.p_types[n] = true;
                return true;
            }
        }
        //Blocks which contain two particle types
        for (int n=0; n<N_TYPE; n++){
                for (int m=0; m<N_TYPE; m++){
                    if (block.length == (header.npart[m]+header.npart[n])*block.partlen){
                        block.p_types[n] = true;
                        block.p_types[m] = true;
                        return true;
                    }
                }
        }
        //Blocks which contain three particle types
        for (int n=0; n<N_TYPE; n++){
                for (int m=0; m<N_TYPE; m++){
                        for (int l=0; l<N_TYPE; l++){
                            if (block.length == (header.npart[m]+header.npart[n])*block.partlen){
                                block.p_types[n] = true;
                                block.p_types[m] = true;
                                block.p_types[l] = true;
                                return true;
                            }
                        }
                }
        }
        for (int i=0; i<N_TYPE; i++)
                block.p_types[i] = true;
        //Blocks which contain four particle types
        for (int n=0; n<N_TYPE; n++){
                for (int m=0; m<N_TYPE; m++){
                    if ((int64_t)block.length/block.partlen == total_file_part-header.npart[m]-header.npart[n]){
                        block.p_types[n] = false;
                        block.p_types[m] = false;
                        return true;
                    }
                }
        }
        //Blocks which contain five particle type
        for (int n=0; n<N_TYPE; n++){
            if ((int64_t)block.length/block.partlen == total_file_part - header.npart[n]){
                block.p_types[n] = false;
                return true;
            }
        }
        return false;
  }
}
