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
                virtual int64_t WriteBlock(std::string BlockName, int type, void *data, int partlen, uint32_t np_write, uint32_t begin) =0;
                /** Note npart is silently ignored.*/
                virtual int WriteHeader(gadget_header& head) =0;
                uint32_t GetNPart(int type);
                virtual ~GBaseWriteFile()=0;
         protected:
                //The file's actual name
                std::string filename;
                std::valarray<uint32_t> npart; //Number of particles in this file.
  };
  #endif //SWIG

  /*Base class for snapshot sets, inherited by GWriteSnap,
   * which implements formats where we have to do the file striping ourselves,
   * (Gadget and HDF5) and GWriteBigSnap, which implements BigFile*/
  class DLL_PUBLIC GWriteBaseSnap{
          public:
                  /** Base constructor. If you want an HDF5 snapshot, pass a filename ending in .hdf5 */
                  GWriteBaseSnap(int format, std::valarray<int64_t> npart_in,int num_files=1, bool debug=true) :
                      npart(npart_in), num_files(num_files), format(format), debug(debug)
                  {}
                  virtual int WriteHeaders(gadget_header head) = 0;
                  /** Get the number of files */
                  int GetNumFiles() {
                     return num_files;
                  }
                  //Returns the format. 1== Gadget-1, 2==Gadget-2, 3==HDF5, 4 == BigFile.
                  int GetFormat(){
                      return format;
                  }
                  ~GWriteBaseSnap() {};
          protected:
                  /** Vector to store the maps of each simulation snapshot */
                  const std::valarray<int64_t> npart;
                  const int num_files;
                  int format;
                  /** Flag to control whether WARN prints anything */
                  bool debug;
  };

  /** Main class for reading Gadget snapshots. */
  class DLL_PUBLIC GWriteSnap : public GWriteBaseSnap {
          public:
                  /** Base constructor. If you want an HDF5 snapshot, pass a filename ending in .hdf5 */
                  GWriteSnap(std::string snap_filename, std::valarray<int64_t> npart_in,int num_files=1, int idsize=sizeof(int64_t),bool debug=true, bool format_2 = true, std::vector<block_info> *BlockNames=NULL);
                  int64_t WriteBlocks(std::string BlockName, int type, void *data, int64_t np_write, int64_t begin);
                  //npart, num_files and friends silently ignored
                  int WriteHeaders(gadget_header head);
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
  };

}

#endif //__GADGETREADER_H
