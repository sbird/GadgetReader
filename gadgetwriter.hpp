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
/** \file 
 * Gadget writer library header file*/

/** \namespace GadgetWriter
 *  Classes for writing Gadget files*/

/** 
 * \section intro_sec Writer 
 * GadgetWriter (libwgad.so) is a small library 
 * for easily writing Gadget-1 and 2 formatted files. 
 *
 * \section req_sec Requirements
 * A C++ compiler with map, vector, set and stdint.h
 */
#ifndef __GADGETWRITER_H
#define __GADGETWRITER_H

/*Symbol table visibility stuff*/
#if __GNUC__ >= 4
    #define DLL_PUBLIC __attribute__ ((visibility("default")))
    #define DLL_LOCAL  __attribute__ ((visibility("hidden")))
#else
    #define DLL_PUBLIC
    #define DLL_LOCAL
#endif

#include <map>
#include <vector>
#include <string>
#include <valarray>
#include <stdint.h>
#include <stdio.h>
#include <set>

/* Include the file header structure*/
#include "gadgetheader.h"

namespace GadgetWriter{

  /** Public structure for passing around 
   * the metadata for each block: 
   * types is a bitfield containing the types that can have this block
   * partlen is bits per particle for this block */
  /* Put this in a map with start_pos*/
  class DLL_PUBLIC block_info {
          public:
                block_info(std::string name, std::valarray<bool> types, short partlen): name(name),types(types), partlen(partlen)
          {};
                std::string name;
                std::valarray<bool>  types;
                short       partlen;
  };
  
  // The following are private structures that we don't want wrapped
#ifndef SWIG 

    class DLL_LOCAL GBaseWriteFile {
         public:
                GBaseWriteFile(std::string filename, std::valarray<uint32_t> npart_in): filename(filename), npart(N_TYPE)
                {
                    for(int i=0; i< N_TYPE; i++)
                        npart[i] = npart_in[i];
                };
                // Begin should specify which particle to begin at
                virtual uint32_t WriteBlock(std::string BlockName, int type, void *data, int partlen, uint32_t np_write, uint32_t begin) =0;
                /** Note npart is silently ignored.*/
                virtual int WriteHeader(gadget_header head) =0;
                uint32_t GetNPart(int type);
                virtual ~GBaseWriteFile()=0;
         protected:
                //The file's actual name
                std::string filename;
                std::valarray<uint32_t> npart; //Number of particles in this file.
  };

  /** This private structure stores information about each file. 
   * May change without warning, don't use it.
   */
  class DLL_LOCAL GWriteFile: public GBaseWriteFile {
         public:
                GWriteFile(std::string filename, std::valarray<uint32_t> npart_in, std::vector<block_info>* BlockNames, bool format_2, bool debug);
                // Begin should specify which particle to begin at
                uint32_t WriteBlock(std::string BlockName, int type, void *data, int partlen, uint32_t np_write, uint32_t begin);
                /** Note npart is silently ignored.*/
                int WriteHeader(gadget_header head);
                ~GWriteFile()
                {
                        if(fd)
                           fclose(fd);
                }
         private:
                bool format_2;
                bool debug;
                int header_size,footer_size;
                FILE * fd;
                //Go from Key = <BlockName> Value = <Type, start>
                std::map<std::string,std::map<int, int64_t> > blocks;
                /** Private function to populate the above */
                void construct_blocks(std::vector<block_info> * BlockNames);
                /** Private function to calculate the size of a block*/
                uint32_t calc_block_size(std::string name);
                /**Function to write the block header
                 * @return 1 if error, 0 if fine.*/
                int write_block_header(FILE * fd, std::string name, uint32_t blocksize);
                /**Function to write the block header; all else as write_block_header() */
                int write_block_footer(FILE * fd, std::string name, uint32_t blocksize);
  };

#ifdef HAVE_HDF5

#include <hdf5.h>

  class DLL_LOCAL GWriteHDFFile: public GBaseWriteFile{
         public:
                GWriteHDFFile(std::string filename, std::valarray<uint32_t> npart_in, std::vector<block_info>* BlockNames, bool format_2, bool debug);
                //begin should specify which particle to begin at
                uint32_t WriteBlock(std::string BlockName, int type, void *data, int partlen, uint32_t np_write, uint32_t begin);
                /** Note npart is silently ignored.*/
                int WriteHeader(gadget_header head);
                ~GWriteHDFFile(){};
         private:
                bool debug;
                //For storing group names: PartType0, etc.
                char g_name[N_TYPE][20];
                /** Private function to find block datatype (int64 or float)*/
                std::set<std::string> m_ints;
                char get_block_type(std::string BlockName);
  };
#endif //HAVE_HDF5

#endif //SWIG

  /** Main class for reading Gadget snapshots. */
  class DLL_PUBLIC GWriteSnap{
          public:
                  /** Base constructor. If you want an HDF5 snapshot, pass a filename ending in .hdf5 */
                  GWriteSnap(std::string snap_filename, std::valarray<int64_t> npart_in,int num_files=1, int idsize=sizeof(int64_t),bool debug=true, bool format_2 = true, std::vector<block_info> *BlockNames=NULL);
                  int64_t WriteBlocks(std::string BlockName, int type, void *data, int64_t np_write, int64_t begin);
                  //npart, num_files and friends silently ignored
                  int WriteHeaders(gadget_header head);
                  /** Get the number of files */
                  int GetNumFiles(){
                          return files.size();
                  }
                  ~GWriteSnap()
                  {
                      std::vector<GBaseWriteFile *>::iterator it;
                      for(it=files.begin(); it<files.end(); ++it)
                            delete *it;
                  }

          private:
                  /** Vector to store the maps of each simulation snapshot */
                  std::vector<GBaseWriteFile *> files;
                  std::vector<block_info> BlockNames;
                  std::valarray<int64_t> npart;
                  int num_files;
                  /** Flag to control whether WARN prints anything */
                  bool debug;
  };

}

#endif //__GADGETREADER_H
