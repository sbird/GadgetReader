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
#include <iostream>
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

#ifdef HAVE_HDF5

#include <hdf5_hl.h>

  GWriteHDFFile::GWriteHDFFile(std::string filename, std::valarray<uint32_t> npart_in, std::vector<block_info>* BlockNames, bool format_2, bool debug) : GBaseWriteFile(filename, npart_in), debug(debug)
  {
          for(int i=0; i< N_TYPE; i++)
                  npart[i] = npart_in[i];
          //Create file
          hid_t hdf_file = H5Fcreate(filename.c_str(),H5F_ACC_EXCL,H5P_DEFAULT,H5P_DEFAULT);
          if(hdf_file < 0){
              throw  std::ios_base::failure(std::string("Unable to create file: ")+filename);
          }
          //Create groups in the file
          for(int i = 0; i < N_TYPE; i++){
                if(npart_in[i] == 0)
                    continue;
                snprintf(g_name[i], 20,"PartType%d",i);
                hid_t group = H5Gcreate(hdf_file,g_name[i],H5P_DEFAULT, H5P_DEFAULT,H5P_DEFAULT);
                if (group < 0)
                    throw  std::ios_base::failure(std::string("Unable to create group: ")+std::string(g_name[i]));
          }
          //Create metadata about datablocks: we only actually use partlen.
          std::vector<block_info>::iterator it;
          for(it=(*BlockNames).begin(); it<(*BlockNames).end(); ++it){
              //Detect an integer type
              if ((*it).partlen == sizeof(int64_t))
                  m_ints.insert((*it).name);
          }
                  block_info block=(*it);
          return;

  }

  char GWriteHDFFile::get_block_type(std::string BlockName)
  {
      if (m_ints.count(BlockName) != 0)
          return 'i';
      else
          return 'f';

  }

  int GWriteHDFFile::WriteHeader(gadget_header header)
  {
            herr_t herr;
            for(int i=0; i<N_TYPE; i++){
                header.npart[i]=npart[i];
            }
            hid_t handle = H5Fopen(filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
            if (handle < 0)
                return -1*handle;
            // Lite interface for making simple attributes
            // H5LTset_attribute_int (file_id, dset_name, attr_name, data, size);
            // herr_t H5LTset_attribute_int( hid_t loc_id, const char *obj_name, const char *attr_name, int *buffer, size_t size)
            hid_t hdgrp = H5Gcreate(handle, "Header", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            herr = H5LTset_attribute_uint(handle, "Header", "NumPart_ThisFile", header.npart, N_TYPE);
            if (herr < 0)
                return -1*herr;
            herr = H5LTset_attribute_uint(handle, "Header", "NumPart_Total", header.npartTotal, N_TYPE);
            herr = H5LTset_attribute_uint(handle, "Header", "NumPart_Total_HighWord", header.NallHW, N_TYPE);
            herr = H5LTset_attribute_double(handle, "Header", "MassTable", header.mass, N_TYPE);
            if (herr < 0)
                return -1*herr;
            herr = H5LTset_attribute_double(handle, "Header", "Time", &header.time, 1);
            herr = H5LTset_attribute_double(handle, "Header", "Redshift", &header.redshift, 1);
            herr = H5LTset_attribute_double(handle, "Header", "BoxSize", &header.BoxSize, 1);
            herr = H5LTset_attribute_int(handle, "Header", "NumFilesPerSnapshot", &header.num_files, 1);
            herr = H5LTset_attribute_double(handle, "Header", "Omega0", &header.Omega0, 1);
            herr = H5LTset_attribute_double(handle, "Header", "OmegaLambda", &header.OmegaLambda, 1);
            herr = H5LTset_attribute_double(handle, "Header", "HubbleParam", &header.HubbleParam, 1);
            herr = H5LTset_attribute_int(handle, "Header", "Flag_Sfr", &header.flag_sfr, 1);
            herr = H5LTset_attribute_int(handle, "Header", "Flag_Cooling", &header.flag_cooling, 1);
            herr = H5LTset_attribute_int(handle, "Header", "Flag_StellarAge", &header.flag_stellarage, 1);
            herr = H5LTset_attribute_int(handle, "Header", "Flag_Metals", &header.flag_metals, 1);
            herr = H5LTset_attribute_int(handle, "Header", "Flag_Feedback", &header.flag_feedback, 1);
            herr = H5LTset_attribute_int(handle, "Header", "Flag_DoublePrecision", &header.flag_doubleprecision, 1);
            herr = H5LTset_attribute_int(handle, "Header", "Flag_IC_Info", &header.flag_ic_info, 1);
            if (herr < 0)
                return -1*herr;
            H5Gclose(hdgrp);
            H5Fclose(handle);
            return 0;
  }

  uint32_t GWriteHDFFile::WriteBlock(std::string BlockName, int type, void *data, int partlen, uint32_t np_write, uint32_t begin)
  {
            herr_t herr;
            hid_t handle = H5Fopen(filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
            hid_t group = H5Gopen(handle, g_name[type], H5P_DEFAULT);
            if(group < 0)
                return group;
            hsize_t size[2];
            int rank=1;
            //Get type
            char b_type = get_block_type(BlockName);
            hid_t dtype;
            if(b_type == 'f') {
                size[1] = partlen/sizeof(float);
                dtype=H5T_NATIVE_FLOAT;
            }else if (b_type == 'i') {
                size[1] = partlen/sizeof(int64_t);
                //Hopefully this is 64 bits; the HDF5 manual is not clear.
                dtype = H5T_NATIVE_LLONG;
            }
            else{
                return -1000;
            }
            if (size[1] > 1) {
                    rank = 2;
            }
            //If this is the first write, create the dataset
            if (begin==0) {
                size[0] = npart[type];
                hid_t space_id = H5Screate_simple(rank, size, NULL);
                H5Dcreate(group,BlockName.c_str(),dtype, space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                H5Sclose(space_id);
            }
            hid_t dset = H5Dopen(group,BlockName.c_str(),H5P_DEFAULT);
            if (dset < 0)
                return dset;
            size[0] = np_write;
            hsize_t begins[2]={begin,0};
            //Create a hyperslab that we will write to
            hid_t space_id = H5Screate_simple(rank, size, NULL);
            //Select the hyperslab of elements we are about to write to
            H5Sselect_hyperslab(space_id, H5S_SELECT_SET, begins, NULL, size, NULL);
            /* Write to the dataset */
            herr = H5Dwrite(dset, dtype, H5S_ALL, space_id, H5P_DEFAULT, data);
            H5Dclose(dset);
            H5Gclose(group);
            H5Fclose(handle);
            if (herr < 0)
                return herr;
            return np_write;
  }

#endif

    //Empty declaration to make linker happy
   GBaseWriteFile::~GBaseWriteFile()
   {}

  GWriteFile::GWriteFile(std::string filename, std::valarray<uint32_t> npart_in, std::vector<block_info>* BlockNames, bool format_2,bool debug) : GBaseWriteFile(filename, npart_in), format_2(format_2), debug(debug)
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

  uint32_t GBaseWriteFile::GetNPart(int type)
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


  GWriteSnap::GWriteSnap(std::string snap_filename, std::valarray<int64_t> npart_in,int num_files_in,int idsize,  bool debug, bool format_2,std::vector<block_info> *BlockNamesIn) : npart(N_TYPE),debug(debug)
  {
          std::valarray<uint32_t> npart_file(N_TYPE);
          std::vector<block_info>::iterator it, jt;
          //Auto-detect an HDF5 snapshot: if the desired filename ends in .hdf5, we want hdf5 output
#ifdef HAVE_HDF5
          std::size_t hdf5_ext = snap_filename.find(".hdf5");
          bool hdf5 = (hdf5_ext != std::string::npos);
          if (hdf5)
            snap_filename = snap_filename.substr(0,hdf5_ext);
#endif
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
          num_files = num_files_in;
          //Set up npart
          if(npart_in.size() < N_TYPE)
                  npart[std::slice(0,npart_in.size(),1)] = npart_in;
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
#ifdef HAVE_HDF5
                if (hdf5){
                    filename+=".hdf5";
                    files.push_back(new GWriteHDFFile(filename,npart_file,&BlockNames,format_2,debug));
                }
                else
#endif
                    files.push_back(new GWriteFile(filename,npart_file,&BlockNames,format_2,debug));
          }
          return;

  }
  int64_t GWriteSnap::WriteBlocks(std::string BlockName, int type, void *data, int64_t np_write, int64_t begin)
  {
        std::vector<GBaseWriteFile *>::iterator it;
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
                if(begin > (**it).GetNPart(type)){
                        begin-=(**it).GetNPart(type);
                        continue;
                }
                if(np_file > (**it).GetNPart(type)-begin){
                        np_file = (**it).GetNPart(type)-begin;
                }
                ret=(**it).WriteBlock(BlockName, type, data, partlen, np_file, begin);
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
        std::vector<GBaseWriteFile *>::iterator it;
        for(it=files.begin(); it<files.end(); ++it)
                if((**it).WriteHeader(head))
                        return 1;
        return 0;
  }
}
