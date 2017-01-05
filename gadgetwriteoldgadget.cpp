/* Class specialising to the gadget binary format*/
#include "gadgetwritefile.hpp"
#include <cassert>

namespace GadgetWriter{
  GWriteFile::GWriteFile(std::string filename, std::valarray<uint32_t> npart_in, std::vector<block_info>* BlockNames, bool format_2,bool debug) : GBaseWriteFile(filename, npart_in), format_2(format_2), debug(debug)
  {
          header_size=sizeof(int32_t);
          if(format_2)
                  header_size+=3*sizeof(int32_t)+4*sizeof(char);
          footer_size=sizeof(int32_t);
          construct_blocks(BlockNames);
          fd=NULL;
          return;
  }
  
  int GWriteFile::WriteHeader(gadget_header& head)
  {
        for(int i=0; i<N_TYPE; i++){
                head.npart[i]=npart[i];
        }
        if(!fd && !(fd = fopen(filename.c_str(), "w"))){
               WARN("Can't open '%s' for writing!\n", filename.c_str());
               return 1;
        }
        assert(sizeof(head) == 256);
        fseek(fd,0,SEEK_SET); //Header always at start of file
        if(write_block_header(fd, "HEAD", sizeof(head)) || 
                fwrite(&head, sizeof(head), 1, fd)!=1 ||
                write_block_footer(fd, "HEAD", sizeof(head)) ){
                WARN("Could not write header for %s\n",filename.c_str());
                return 1;
        }
        return 0;
  }

  int64_t GWriteFile::WriteBlock(std::string BlockName, int type, void *data, int partlen, uint32_t np_write, uint32_t begin)
  {
          if((int64_t) np_write+begin > npart[type]){
                  WARN("Not enough room for %ld particles of type %d\n",(int64_t)np_write+begin,type);
                  WARN("Block %s, file %s. Truncated to %d particles\n",BlockName.c_str(), filename.c_str(),npart[type]);
                  np_write=npart[type]-begin;
          }
          if(!fd && !(fd = fopen(filename.c_str(),"w"))){
                 WARN("Can't open '%s' for writing!\n", filename.c_str());
                 return 0;
          }
          std::map<std::string, std::map<int,int64_t> >::iterator ip=blocks.find(BlockName);
          if(ip == blocks.end()) {
                  WARN("Block %s not found\n",BlockName.c_str());
                  return 0;
          }
          /*Note that because internally all map keys are ordered by
           * comparison, the first element of the map is also the one with the lowest type,
           * and the final element has the largest type*/
          int MinType =  (ip->second).begin()->first;
          int MaxType =  (--(ip->second).end())->first;
          std::map<int,int64_t>::iterator it=((*ip).second).find(type);
          if(it == ((*ip).second).end()) {
                WARN("Type %d not found in block %s\n",type,BlockName.c_str());
                return 0;
          }
          //If we are writing from the beginning, write the block header
          if(begin == 0 && type == MinType){
                if(fseek(fd, (*it).second,SEEK_SET)) {
                    WARN("Could not seek to %lu in file %s\n",(*it).second, filename.c_str());
                    return 0;
                }
                if(write_block_header(fd, BlockName, partlen*calc_block_size(BlockName))){
                        WARN("Could not write block header %s in file %s\n",BlockName.c_str(), filename.c_str());
                        return 0;
                }
          }
          else {
                int64_t init_fpos = (*it).second+begin*partlen;
                if(type==MinType)
                        init_fpos+=header_size;
                if(fseek(fd,init_fpos, SEEK_SET) < 0) {
                    WARN("Could not seek to correct location\n");
                    return 0;
                }
          }
          int64_t ret=fwrite(data, partlen, np_write, fd);
          if(ret != np_write){
                  WARN("Wrote only %ld particles of %d\n",ret,np_write);
                  return ret;
          }
          //If this is the last write to this segment, write the footer and close the file
          if(type == MaxType && (np_write+begin == npart[type])){
                  if(write_block_footer(fd, BlockName, partlen*calc_block_size(BlockName))){
                        WARN("Could not write block footer %s in file %s\n",BlockName.c_str(), filename.c_str());
                        return np_write+1;
                  }
          }
          return np_write;
  }

  //This takes an list of blocks and constructs a map of where they should go in a file.
  //Check for duplicate blocks in the caller.
  void GWriteFile::construct_blocks(std::vector<block_info> * BlockNames)
  {
          //Skip the header
          int64_t cur_pos=sizeof(gadget_header)+header_size+footer_size;
          std::vector<block_info>::iterator it;
          for(it=(*BlockNames).begin(); it<(*BlockNames).end(); ++it){
                  block_info block=(*it);
                  /*Only add the block if it is present for some particle types*/
                  if(block.types.sum()) {
                          std::map<int,int64_t>p;
                          bool first=true;
                          for(int type=0; type< N_TYPE; type++){
                                  /*Continue if empty block*/
                                  if(!block.types[type] || !npart[type]) 
                                          continue;
                                  p[type]=cur_pos;
                                  cur_pos+=block.partlen*npart[type];
                                  /*Want to add space for a header block after the first type only*/
                                  if(first){
                                          cur_pos+=header_size;
                                          first=false;
                                  }
                          }
                          blocks[block.name]=p;
                          /*Only add a footer if we have also added a header*/
                          if(!first)
                                cur_pos+=footer_size;
                  }
          }
          return;
  }
 
  uint32_t GWriteFile::calc_block_size(std::string name)
  {
          std::map<std::string,std::map<int, int64_t> >::iterator it=blocks.find(name);
          if(it == blocks.end())
                  return 0;
          uint32_t total=0;
          std::map<int, int64_t>& p = it->second;
          std::map<int, int64_t>::iterator jt;
          for(jt=p.begin(); jt != p.end(); ++jt){
                  //jt->first is the integer corresponding to the type
                  total +=npart[jt->first];
          }
          return total;
  }

  int GWriteFile::write_block_header(FILE * fd, std::string name, uint32_t blocksize)
  {
      if(format_2){
        /*This is the block header record, which we want for format two files*/
        int32_t blkheadsize = sizeof(int32_t) + 4 * sizeof(char);
        uint32_t nextblock = blocksize + 2 * sizeof(uint32_t);
        /*Write format 2 header header*/
        if ( fwrite(&blkheadsize,sizeof(int32_t),1,fd) != 1 ||
             fwrite(name.c_str(), sizeof(char), 4, fd) != 4 ||
             fwrite(&nextblock, sizeof(uint32_t), 1, fd) != 1 || 
             fwrite(&blkheadsize,sizeof(int32_t),1,fd) !=1)
                return 1;
      }
      /*This is the record size, which we want for all files*/
      if(fwrite(&blocksize, sizeof(int32_t), 1, fd) !=1)
              return 1;
      return 0;
  }

  int GWriteFile::write_block_footer(FILE * fd, std::string name, uint32_t blocksize)
  {
        /*This is the record size, which we want for all files*/
        if(fwrite(&blocksize, sizeof(uint32_t), 1, fd) !=1)
                return 1;
        return 0;
  }
}
