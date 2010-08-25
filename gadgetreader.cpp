#include "gadgetreader.hpp"
#include "read_utils.h"
#include <string.h>

namespace gadgetreader{

  GadgetIISnap::GadgetIISnap(f_name snap_filename, bool debugflag=false){
        file_map first_map;
        f_name first_file=snap_filename;
        FILE *fd;
        swap_endian = false;
        debug=debugflag;
        //Try to open the file 
        if(!(fd=fopen(first_file.c_str(),"r")) || check_filetype(fd)){
                //Append ".0" to the filename if necessary.
                first_file+=".0";
                //and then try again
                if(!(fd=fopen(first_file.c_str(),"r")) || check_filetype(fd))
                        ERROR("Could not open %s or %s.0\nDoes not exist, or is corrupt.\n",snap_filename.c_str(),snap_filename.c_str());
        }
        //Read the first file
        first_map= construct_file_map(fd,first_file);
        //Set the global variables. 
        base_filename=first_file;
        //Take the ".0" from the end if needed.
        if(base_filename.compare(base_filename.size()-2,2,".0"))
                base_filename.erase(base_filename.end()-2,base_filename.end());
        num_files=first_map.header.num_files;
        //Make the file_maps
        file_maps = new file_map[num_files];
        //Put the information from the first file in
        file_maps[0]=first_map;
        //Close the first file
        fclose(fd);
        //Get the information for the other files.
        for(int i=1;i<num_files;i++){
                f_name c_name=base_filename;
                char tmp[6];
                snprintf(tmp,6,".%d",i);
                c_name+=(std::string(tmp));
                if(!(fd=fopen(c_name.c_str(),"r")))
                        ERROR("Could not open file %d of %d\n",i,num_files);
                file_maps[i]=construct_file_map(fd,c_name);
                fclose(fd);
                if(!check_headers(file_maps[i].header,file_maps[0].header)){
                  fprintf(stderr, "Headers inconsistent!");
                  exit(99);
                }
        }
        return;
  }

  GadgetIISnap::~GadgetIISnap(){
     delete[] file_maps;
  }

  //Copy constructor
  GadgetIISnap::GadgetIISnap(const GadgetIISnap& other){
          //Allocate memory
          file_maps=new file_map[other.num_files];
          //Just call the assignment operator for the rest
          (*this)=other;
  }

  GadgetIISnap& GadgetIISnap::operator=(const GadgetIISnap& rhs){
          base_filename=rhs.base_filename;
          swap_endian=rhs.swap_endian;
          num_files=rhs.num_files;
          std::copy(rhs.file_maps,rhs.file_maps+rhs.num_files,file_maps);
          return *this;
  }
  
                
  file_map GadgetIISnap::construct_file_map(FILE *fd,const f_name strfile){
          file_map c_map;
          //Default ordering of blocks for Gadget-I files
          char * default_blocks[12]={"HEAD","POS ","VEL ","ID  ","MASS","U   ","RHO ","NE  ","NH  ","NHE ","HSML","SFR "};
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
                     if(debug) printf("Found block %s of size %d!",c_name,c_info.length);
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
                          if(c_info.length != 256) ERROR("Mis-sized HEAD block in %s\n",file);
                          if(!(check_fread(&(c_map.header),sizeof(gadget_header),1,fd)) ||
                          !(check_fread(&record_size,sizeof(int32_t),1,fd)))
                                  ERROR("Could not read HEAD in %s!\n",file);
                          if(swap_endian) endian_swap(&record_size);
                          if(record_size != 256) ERROR("Bad record size for HEAD in %s!\n",file);
                          //Next block
                          continue;
                  }
                  //Store the start of the data block
                  c_info.start_pos=ftell(fd);
                  //Skip reading the actual data
                  fseek(fd,c_info.length,SEEK_CUR);
                  if(!(check_fread(&record_size,sizeof(int32_t),1,fd)) || 
                      ( swap_endian || record_size != c_info.length) ||
                      ( !swap_endian || endian_swap(&record_size) != c_info.length))
                          ERROR("Corrupt or non-existent record for block %s in %s!\n",c_name,file);
                  //If POS or VEL block, 3 floats per particle.
                  if(strncmp(c_name,default_blocks[1],4)==0 || strncmp(c_name,default_blocks[2],4)==0)
                      c_info.per_part_length=12;
                  //Append the current info to the map.
                  c_map.blocks[std::string(c_name)] = c_info;
          }
          return c_map;
  }
 
 /* Return integer length of block. Arguments are:
  * name - place to store block name
  * fd - file descriptor
  * file - filename of open file */
 uint32_t GadgetIISnap::read_G2_block_head(char* name, FILE *fd, const char * file){
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
                WARN("Record length is: %d and ended as %d\n",head[0],head[3]);
                WARN("Header bytes are: %s %d\n",(char *) head[1],head[2]);
                ERROR("Maybe trying to read old-format file as new-format?\n");
        }
        strncpy(name,(char *)head[1],4);
        //Null-terminate the string
        *(name+5)='\0';
        return head[2];
  }

  /*Sets swap_endian and format_2. Returns 0 for success, 1 for an empty file, and 2 
   * if the filetype is weird (ie, if you tried to open a text file by mistake)*/
  int GadgetIISnap::check_filetype(FILE* fd){
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
  
  bool GadgetIISnap::check_headers(gadget_header head1, gadget_header head2){
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
}
