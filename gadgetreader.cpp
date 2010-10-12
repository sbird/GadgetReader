#include "gadgetreader.hpp"
#include "read_utils.h"
#include <string.h>

namespace gadgetreader{

  //Constructor; this does almost all the hard work of building a "map" of the block positions
  GadgetIISnap::GadgetIISnap(f_name snap_filename, bool debugflag=false)
  {
        file_map first_map;
        f_name first_file=snap_filename;
        FILE *fd;
        int64_t npart[N_TYPE]={0};
        int files_expected=1;
        swap_endian = false;
        debug=debugflag;

        //Try to open the file 
        if(!(fd=fopen(first_file.c_str(),"r")) || check_filetype(fd)){
                //Append ".0" to the filename if necessary.
                first_file+=".0";
                //and then try again
                if(!(fd=fopen(first_file.c_str(),"r")) || check_filetype(fd))
                        ERROR("Could not open %s (.0)\nDoes not exist, or is corrupt.\n",snap_filename.c_str());
        }
        //Read the first file
        first_map= construct_file_map(fd,first_file);
        //Set the global variables. 
        base_filename=first_file;
        //Take the ".0" from the end if needed.
        if(base_filename.compare(base_filename.size()-2,2,".0")){
                std::string::iterator it=base_filename.end();
                base_filename.erase(it-2,it);
        }
        files_expected=first_map.header.num_files;
        if(files_expected < 1){
                WARN("Implausible number of files supposedly in simulation set: %d\n",files_expected);
                ERROR("Probably corrupt header in first file. Dying.\n");
        }
        //Put the information from the first file in
        file_maps.push_back(first_map);
        //Close the first file
        fclose(fd);
        //Get the information for the other files.
        for(int i=1;i<files_expected;i++){
                f_name c_name=base_filename;
                char tmp[6];
                file_map tmp_map;
                snprintf(tmp,6,".%d",i);
                c_name+=(std::string(tmp));
                if(!(fd=fopen(c_name.c_str(),"r"))){
                        WARN("Could not open file %d of %d\n",i,files_expected);
                        continue;
                }
                tmp_map=construct_file_map(fd,c_name);
                fclose(fd);
                if(!check_headers(tmp_map.header,file_maps[0].header)){
                        WARN("Headers of inconsistent between file 0 and file %d, ignoring file %d\n",i,i);
                        continue;
                }
                file_maps.push_back(tmp_map);
                for(int j=0; j<N_TYPE; j++)
                   npart[j]+=file_maps[i].header.npart[j];
        }
        //Check we have found as many particles as we were expecting.
        for(int i=0; i<N_TYPE; i++)
            if(npart[i] != GetNpart(i))
               WARN("Expected %ld particles of Type %d, but found %ld!\n",npart[i],i,GetNpart(i));
        return;
  }

  //This takes an open file and constructs a map of where the blocks are within it, and then returns said map.
  //It ought to support endian swapped files as well as Gadget-I files.
  file_map GadgetIISnap::construct_file_map(FILE *fd,const f_name strfile)
  {
          file_map c_map;
          //Default ordering of blocks for Gadget-I files
          char *default_blocks[12]={"HEAD","POS ","VEL ","ID  ","MASS","U   ","RHO ","NE  ","NH  ","NHE ","HSML","SFR "};
          char** b_ptr=default_blocks;
          uint32_t record_size;
          c_map.name=strfile;
          const char * file=strfile.c_str();
          //Read now until we run out of file
          while(!feof(fd)){
                  block_info c_info;
                  char c_name[5];
                  //Read another block header
                  if(format_2){
                     c_info.length=read_G2_block_head(c_name,fd,file);
                     if(c_info.length ==0)
                             break;//We have run out of file
                     if(!(check_fread(&record_size,sizeof(int32_t),1,fd))){
                             WARN("Ran out of data in %s before block %s started\n",file,c_name);
                             break;
                     }
                     if(swap_endian) endian_swap(&record_size);
                     if(c_info.length != record_size){
                             WARN("Corrupt record in %s for block %s, skipping rest of file\n",file,c_name);
                             break;
                     }
                     if(debug) printf("Found block %s of size %u!",c_name,c_info.length);
                  }
                  else{
                    //For Gadget-I files, we have to settle for less certainty, 
                    //and read the name from a pre-guessed table
                    //and the length from the record length.
                    strncpy(c_name, *(b_ptr++),5);
                    if(!check_fread(&c_info.length,sizeof(int32_t),1,fd))
                            break;//out of file
                    if(swap_endian) endian_swap(&c_info.length);
                  }
                  //Do special things for the HEAD block
                  if(strncmp(c_name,default_blocks[0],4) ==0){
                          //Read the actual header. 
                          if(c_info.length != sizeof(gadget_header)){
                                  WARN("Mis-sized HEAD block in %s\n",file);
                                  c_map.header.num_files=-1;
                                  return c_map;
                          }
                          if(!(check_fread(&(c_map.header),sizeof(gadget_header),1,fd)) ||
                          !(check_fread(&record_size,sizeof(int32_t),1,fd))){
                                  WARN("Could not read HEAD in %s!\n",file);
                                  c_map.header.num_files=-1;
                                  return c_map;
                          }
                          if(swap_endian) endian_swap(&record_size);
                          if(record_size != sizeof(gadget_header)){
                                 WARN("Bad record size for HEAD in %s!\n",file);
                                 c_map.header.num_files=-1;
                                 return c_map;
                          }
                          //Next block
                          continue;
                  }
                  //Store the start of the data block
                  c_info.start_pos=ftell(fd);
                  //Skip reading the actual data
                  fseek(fd,c_info.length,SEEK_CUR);
                  if(!(check_fread(&record_size,sizeof(int32_t),1,fd)) || 
                      ( swap_endian || record_size != c_info.length) ||
                      ( !swap_endian || endian_swap(&record_size) != c_info.length)){
                          WARN("Corrupt or non-existent record for block %s in %s!\n",c_name,file);
                          WARN("Cannot read the rest of this file\n");
                          return c_map;
                  }
                  //If POS or VEL block, 3 floats per particle.
                  if(strncmp(c_name,default_blocks[1],4)==0 || strncmp(c_name,default_blocks[2],4)==0)
                      c_info.partlen=12;
                  //Append the current info to the map.
                  c_map.blocks[std::string(c_name)] = c_info;
          }
          return c_map;
  }
 
 /* Read the header of a Gadget file. This is the "block header" not the file header, and gives the 
  * name and length of the block.
  * Return integer length of block. Arguments are:
  * name - place to store block name
  * fd - file descriptor
  * file - filename of open file */
 uint32_t GadgetIISnap::read_G2_block_head(char* name, FILE *fd, const char * file)
 {
        uint32_t head[4];
        //Read the "block header" record, only present on Gadget-II files. 
        //Has format: 
        //size (8), name (HEAD), length (256), size (8)
        if(check_fread(&head,sizeof(uint32_t),4,fd)==0)
           //If we have run out of file, we can just return.
           return 0;
        if(swap_endian)  multi_endian_swap(&head[0],4);
        if(head[0] != 8 || head[3] !=8){
                WARN("Corrupt header record in file %s.\n",file);
                WARN("Record length is: %u and ended as %u\n",head[0],head[3]);
                WARN("Header bytes are: %s %u\n",(char *) head[1],head[2]);
                ERROR("Maybe trying to read old-format file as new-format?\n");
        }
        strncpy(name,(char *)head[1],4);
        //Null-terminate the string
        *(name+5)='\0';
        return head[2];
  }

  /*Sets swap_endian and format_2. Returns 0 for success, 1 for an empty file, and 2 
   * if the filetype is weird (ie, if you tried to open a text file by mistake)*/
  int GadgetIISnap::check_filetype(FILE* fd)
  {
          uint32_t record_size;
          //Start at the beginning.
          rewind(fd);
          //Read the first integer
          if(!check_fread(&record_size,sizeof(int32_t),1,fd)){
                 return 1;
          }
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
                       WARN("WARNING: Reading Gadget-I file using pre-computed\nblock order, which may not correspond to the actual order of your file!\n");
                       format_2=false;
                       break;
                  default:
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
  bool GadgetIISnap::check_headers(gadget_header head1, gadget_header head2)
  {
    /*Check single quantities*/
    /*Even the floats ought to be really identical*/
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
                head1.npartTotal[i] != head2.npartTotal[i] ||
                head1.NallHW[i] != head2.NallHW[i])
                    return false;
    return true;
  }

  /* Gets the total number of particles in a simulation. */
  int64_t GadgetIISnap::GetNpart(int type)
  {
          int64_t npart;
          if(type >= N_TYPE || type < 0) 
                  return 0;
          //Get the part that doesn't fit in 32 bits
          npart=file_maps[0].header.NallHW[type];
          //Bitshift it and add on the other part
          return (npart << 32) + file_maps[0].header.npartTotal[type];
  }

  /*Check the given block exists*/
  bool GadgetIISnap::IsBlock(char * BlockName)
  {
        std::string bname(BlockName);
        //Find total number of particles needed
        for(int i=0; i<file_maps.size(); i++)
                if(file_maps[i].blocks.count(bname))
                        return true;
        return false;
  }
  
  /*Get total size of a block in the snapshot, in bytes*/
  int64_t GadgetIISnap::GetBlockSize(char * BlockName)
  {
        int64_t size=0;
        std::string bname(BlockName);
        //Find total number of particles needed
        for(int i=0; i<file_maps.size(); i++)
                if(file_maps[i].blocks.count(bname))
                        size+=file_maps[i].blocks[bname].length;
        return size;
  }

  /*Get a set of the block names in the snapshot*/
  std::set<std::string> GadgetIISnap::GetBlocks()
  {
          std::map<std::string,block_info>::iterator it;
          std::set<std::string> names;
          for(int i=0; i<file_maps.size();i++)
                for(it=file_maps[i].blocks.begin() ; it != file_maps[i].blocks.end(); it++)
                          names.insert((*it).first);
          return names;
  }

  /*Allocates memory for a block of particles, then reads the particles into 
   * the block. Returns NULL if cannot complete.
   * Returns a block of particles. Remember to free the pointer it gives back once done.
   * Takes: block name, 
   *        particles to read, 
   *        pointer to allocated memory for block
   *        particles to skip initially
   *        Types to skip, as a bitfield. 
   *        Pass 1 to skip baryons, 3 to skip baryons and dm, 2 to skip dm, etc.
   *        Only skip types for which the block is actually present:
   *        Unfortunately there is no way of the library knowing which particle has which type, so 
   *        there is no way of telling that in advance.  */
  int64_t GadgetIISnap::GetBlock(char* BlockName, char *block, int64_t npart_toread, int64_t start_part, int skip_type)
  {
        int64_t npart_read;
        std::string BlockNameStr(BlockName);
        //Check the block really exists
        if(!IsBlock(BlockName)){
                WARN("Block %s is not in this snapshot\n",BlockName);
                return 0;
        }
        //Read a chunk of particles from a file
        for(int i=0;i<file_maps.size(); i++){
                uint32_t read_data,start_pos=0,npart_file;
                FILE *fd;
                block_info cur_block;
                //Get current block
                if(file_maps[i].blocks.count(BlockNameStr))
                        cur_block=file_maps[i].blocks[BlockNameStr];
                else{
                       WARN("Block %s not in file %d\n",BlockName,i);
                       continue;
                }
                npart_file = cur_block.length/cur_block.partlen;
                //Don't want to read the skip_types
                for(int j=0; j<N_TYPE;j++)
                        if(skip_type & (1 << j))
                                npart_file-=file_maps[i].header.npart[j];
                //There is also a maximum amount of particles we want to read. If we have reached it, truncate.
                if(npart_file > npart_toread-npart_read)
                       npart_file=npart_toread-npart_read;
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
                        if(npart_file < start_part){
                                start_part-=npart_file;
                                continue;
                        }
                        //Otherwise add something to start_pos and take it off npart_file
                        start_pos+=start_part*cur_block.partlen;
                        npart_file-=start_part;
                }
                                
                //Open file: If this fails skip to the next file.
                if(!(fd=fopen(file_maps[i].name.c_str(),"r"))){
                        WARN("Could not open file %d of %lu, continuing\n",i,file_maps.size());
                        continue;
                }
                //Seek to first particle
                if(fseek(fd,start_pos,SEEK_SET) == -1)
                        WARN("Failed to seek\n");
                //Read the data!
                read_data=fread(block+npart_read*cur_block.partlen,cur_block.partlen,npart_file,fd);
                //Don't die if we read the wrong amount of data; maybe we can find it in the next file.
                if(read_data !=npart_file)
                        WARN("Only read %u particles of %u from file %d\n",read_data,npart_file,i);
                fclose(fd);
                npart_read+=read_data;
        }
        if(npart_toread > npart_read){
                WARN("Read %ld particles out of %ld\n",npart_read,npart_toread);
        }
        return npart_read;
  }
}
