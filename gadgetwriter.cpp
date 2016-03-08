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
#include "gadgetwritefile.hpp"
#include <stdlib.h>
#include <iostream>
#include <sstream>

/** \file
 * GadgetReader library method file*/
namespace GadgetWriter{

    //Empty declaration to make linker happy
   GBaseWriteFile::~GBaseWriteFile()
   {}

  uint32_t GBaseWriteFile::GetNPart(int type)
  {
          if((unsigned int) type > npart.size())
                  return 0;
          return npart[type];
  }


  GWriteSnap::GWriteSnap(std::string snap_filename, std::valarray<int64_t> npart_in,int num_files_in,int idsize,  bool debug, bool format_2,std::vector<block_info> *BlockNamesIn) :
      GWriteBaseSnap(2, npart_in, num_files_in, debug)
  {
          std::valarray<uint32_t> npart_file(N_TYPE);
          std::vector<block_info>::iterator it, jt;
          //Auto-detect an HDF5 snapshot: if the desired filename ends in .hdf5, we want hdf5 output
          bool hdf5 = false;
#ifdef HAVE_HDF5
          std::size_t hdf5_ext = snap_filename.find(".hdf5");
          hdf5 = (hdf5_ext != std::string::npos);
          if (hdf5){
//             std::cerr << "Using hdf5 snapshot for "<<snap_filename<<std::endl;
            snap_filename = snap_filename.substr(0,hdf5_ext);
          }
#endif
          //Set up some default BlockNames: every valid simulation must have these.
          //HDF5 has different default names
          if (hdf5)
          {
                format=3;
                BlockNames.push_back(block_info("Coordinates",std::valarray<bool>(true,N_TYPE),3*sizeof(float)));
                BlockNames.push_back(block_info("Velocities",std::valarray<bool>(true,N_TYPE),3*sizeof(float)));
                BlockNames.push_back(block_info("ParticleIDs",std::valarray<bool>(true,N_TYPE),idsize));
                //By default MASS is given in the header (hence 0's)
                //but the user may wish to override it.
                BlockNames.push_back(block_info("Masses",std::valarray<bool>(false,N_TYPE),sizeof(float)));
                block_info u("InternalEnergy",std::valarray<bool>(false,N_TYPE),sizeof(float));
                u.types[BARYON_TYPE]=true;
                BlockNames.push_back(u);
          }
          else
          {
                format = 1 + format_2;
                BlockNames.push_back(block_info("POS ",std::valarray<bool>(true,N_TYPE),3*sizeof(float)));
                BlockNames.push_back(block_info("VEL ",std::valarray<bool>(true,N_TYPE),3*sizeof(float)));
                BlockNames.push_back(block_info("ID  ",std::valarray<bool>(true,N_TYPE),idsize));
                //By default MASS is given in the header (hence 0's)
                //but the user may wish to override it.
                BlockNames.push_back(block_info("MASS",std::valarray<bool>(false,N_TYPE),sizeof(float)));
                block_info u("U   ",std::valarray<bool>(false,N_TYPE),sizeof(float));
                u.types[BARYON_TYPE]=true;
                BlockNames.push_back(u);
          }
          //Add the user-specified blocks, which override the defaults.
          //In case of conflict, later overrides earlier.
          if(BlockNamesIn != NULL){
                for(it=(*BlockNamesIn).begin(); it<(*BlockNamesIn).end(); ++it){
                        if( (*it).types.size() != N_TYPE )
                                continue; //We don't want invalid blocks
                        if( !hdf5 && ((*it).name == std::string("HEAD") ) )
                                continue; //We don't want head blocks
                        if( hdf5 && ((*it).name == std::string("Header") ) )
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
          if((size_t) 3*(npart.max()/num_files) > (1L<<31)/sizeof(float)){
                  ERROR("Not enough room for %ld particles in %d files\n",npart.max(),num_files);
//                   num_files = 3*sizeof(float)*npart.max()/(1L<<31);
//                   WARN("Increasing to %d files\n",num_files);
          }
          for(unsigned int i=0; i<npart.size();++i)
                npart_file[i]=npart[i]/num_files;
          for(int i=0; i<num_files; i++){
                //Next two lines are a somewhat over-engineered way of converting
                //an integer to a string
                std::string filename=snap_filename;
                if(num_files > 1){
                    std::ostringstream s;
                    s<<i;
                    filename+="."+s.str();
                }
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
