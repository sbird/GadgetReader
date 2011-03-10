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
#include "gadgetwriter.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <sstream>

/** \file 
 * GadgetReader library method file*/
namespace GadgetWriter{

/*Error output macros*/
#define ERROR(...) do{ fprintf(stderr,__VA_ARGS__);abort();}while(0)
#define WARN(...) do{ \
        if(debug){ \
                fprintf(stderr,"[GadgetWriter]: "); \
                fprintf(stderr, __VA_ARGS__); \
        }}while(0)
  
  GWriteFile::GWriteFile(std::string filename, std::valarray<uint32_t> npart_in, std::vector<block_info>* BlockNames, bool format_2,bool debug) : filename(filename), format_2(format_2), debug(debug), npart(N_TYPE)
  {
          for(int i=0; i< N_TYPE; i++)
                  npart[i] = npart_in[i];
          header_size=sizeof(int32_t);
          if(format_2)
                  header_size+=3*sizeof(int32_t)+4*sizeof(char);
          footer_size=sizeof(int32_t);
          construct_blocks(BlockNames);
          fd=NULL;
          return;
  }
  
  int GWriteFile::WriteHeader(gadget_header head)
  {
        for(int i=0; i<N_TYPE; i++){
                head.npart[i]=npart[i];
        }
        if(!fd && !(fd = fopen(filename.c_str(), "w"))){
               WARN("Can't open '%s' for writing!\n", filename.c_str());
               return 1;
        }
        fseek(fd,0,SEEK_SET); //Header always at start of file
        if(write_block_header(fd, "HEAD", sizeof(head)) || 
                fwrite(&head, sizeof(head), 1, fd)!=1 ||
                write_block_footer(fd, "HEAD", sizeof(head)) ){
                WARN("Could not write header for %s\n",filename.c_str());
                return 1;
        }
        return 0;
  }

  uint32_t GWriteFile::WriteBlock(std::string BlockName, int type, void *data, int partlen, uint32_t np_write, uint32_t begin)
  {
          uint32_t ret;
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
          if(ip == blocks.end())
                  WARN("Block %s not found\n",BlockName.c_str());
          /*Note that because internally all map keys are ordered by
           * comparison, the first element of the map is also the one with the lowest type,
           * and the final element has the largest type*/
          int MinType =  (ip->second).begin()->first;
          int MaxType =  (--(ip->second).end())->first;
          std::map<int,int64_t>::iterator it=((*ip).second).find(type);
          if(it == ((*ip).second).end())
                WARN("Type %d not found in block %s\n",type,BlockName.c_str());
          //If we are writing from the beginning, write the block header
          if(begin == 0 && type == MinType){
                fseek(fd, (*it).second,SEEK_SET);
                if(write_block_header(fd, BlockName, partlen*calc_block_size(BlockName))){
                        WARN("Could not write block header %s in file %s\n",BlockName.c_str(), filename.c_str());
                        return 0;
                }
          }
          else {
                int64_t init_fpos = (*it).second+begin*partlen;
                if(type==MinType)
                        init_fpos+=header_size;
                fseek(fd,init_fpos, SEEK_SET);
          }
          ret=fwrite(data, partlen, np_write, fd);
          if(ret != np_write){
                  WARN("Wrote only %d particles of %d\n",ret,np_write);
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

  uint32_t GWriteFile::GetNPart(int type)
  {
          if(type <0 || type > N_TYPE)
                  return 0;
          return npart[type];
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


  GWriteSnap::GWriteSnap(std::string snap_filename, std::valarray<int64_t> npart_in,int num_files,int idsize,  bool debug, bool format_2,std::vector<block_info> *BlockNamesIn) : npart(N_TYPE),num_files(num_files),debug(debug)
  {
          std::valarray<uint32_t> npart_file(N_TYPE);
          std::vector<block_info>::iterator it, jt;
          //Set up some default BlockNames: every valid simulation must have these.
          BlockNames.push_back(block_info("POS ",std::valarray<bool>(true,N_TYPE),3*sizeof(float)));
          BlockNames.push_back(block_info("VEL ",std::valarray<bool>(true,N_TYPE),3*sizeof(float)));
          BlockNames.push_back(block_info("ID  ",std::valarray<bool>(true,N_TYPE),idsize));
          //By default MASS is given in the header (hence 0's) 
          //but the user may wish to override it.
          BlockNames.push_back(block_info("MASS",std::valarray<bool>(false,N_TYPE),sizeof(float)));
          block_info u("U   ",std::valarray<bool>(false,N_TYPE),sizeof(float));
          u.types[BARYON_TYPE]=true;
          BlockNames.push_back(u);
          //Add the user-specified blocks, which override the defaults.
          //In case of conflict, later overrides earlier. 
          if(BlockNamesIn != NULL){
                for(it=(*BlockNamesIn).begin(); it<(*BlockNamesIn).end(); ++it){
                        if( (*it).types.size() != N_TYPE )
                                continue; //We don't want invalid blocks
                        if( (*it).name == std::string("HEAD") )
                                continue; //We don't want head blocks
                        for(jt=BlockNames.begin();jt<=BlockNames.end();++jt){
                                if(jt == BlockNames.end())
                                        BlockNames.push_back(*it);
                                else if((*jt).name == (*it).name){
                                        (*jt)=(*it);
                                        break;
                                }
                        }
                }
          }
          //Set up npart
          if(npart.size() < N_TYPE)
                  npart[std::slice(0,npart.size(),1)] = npart_in;
          else
                  npart = npart_in[std::slice(0,N_TYPE,1)];
          if(3*(npart.max()/num_files) > (1L<<31)/sizeof(float)){
                  WARN("Not enough room for %ld particles in %d files\n",npart.max(),num_files);
                  num_files = 3*sizeof(float)*npart.max()/(1L<<31);
                  WARN("Increasing to %d files\n",num_files);
          }
          for(unsigned int i=0; i<npart.size();++i)
                npart_file[i]=npart[i]/num_files;
          for(int i=0; i<num_files; i++){
                //Next two lines are a somewhat over-engineered way of converting
                //an integer to a string
                std::ostringstream s;
                s<<i;
                std::string filename=snap_filename+"."+s.str();
                if(i == num_files -1) //Extra particles in the last file
                        for(unsigned int i=0; i<npart.size();++i)
                                npart_file[i]+=npart[i] % num_files;
                files.push_back(GWriteFile(filename,npart_file,&BlockNames,format_2,debug));
          }
          return;

  }
  int64_t GWriteSnap::WriteBlocks(std::string BlockName, int type, void *data, int64_t np_write, int64_t begin)
  {
        std::vector<GWriteFile>::iterator it;
        std::vector<block_info>::iterator jt;
        short partlen=0;
        uint32_t ret;
        int64_t np_written=0;
        if(type < 0 || type >= N_TYPE)
                return 0;
        if(np_write == 0)
                return 0;
        for(jt=BlockNames.begin();jt<BlockNames.end();++jt){
                if((*jt).name == BlockName){
                        partlen=(*jt).partlen;
                        break;
                }
        }
        if(jt == BlockNames.end()){
                WARN("No block %s specified in constructor\n",BlockName.c_str());
                return 0;
        }
        for(it=files.begin(); it<files.end(); ++it){
                int64_t np_file=np_write;
                if(begin > (*it).GetNPart(type)){
                        begin-=(*it).GetNPart(type);
                        continue;
                }
                if(np_file > (*it).GetNPart(type)-begin){
                        np_file = (*it).GetNPart(type)-begin;
                }
                ret=(*it).WriteBlock(BlockName, type, data, partlen, np_file, begin);
                if(ret != np_file)
                        return np_written+ret;
                begin=0;
                np_write-=np_file;
                np_written+=np_file;
                data=((char *)data)+np_file*partlen;
                if(np_write <= 0)
                        break;
        }
        return np_written;
  }

  //npart, num_files and friends silently ignored
  int GWriteSnap::WriteHeaders(gadget_header head)
  {
        head.num_files=num_files;
        for(int i=0; i< N_TYPE; ++i){
            head.NallHW[i] = ( npart[i] >> 32);
            head.npartTotal[i] = npart[i] - ((uint64_t)head.NallHW[i] << 32);
        }
        std::vector<GWriteFile>::iterator it;
        for(it=files.begin(); it<files.end(); ++it)
                if((*it).WriteHeader(head))
                        return 1;
        return 0;
  }
}
