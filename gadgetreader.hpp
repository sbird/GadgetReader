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
 * Gadget reader library header file*/

/** \namespace GadgetReader
 *  Class for reading Gadget files*/

/** \mainpage 
 * \section intro_sec Introduction 
 * GadgetReader (librgad.so) is a small library with one single class 
 * for easily reading Gadget-1 and 2 formatted files. 
 * \section feat_sec Features
 * Somewhat higher level than the readgadget.c that comes with Gadget-II, GadgetReader has 
 * been written because I noticed that I was maintaining several long, fragile and 
 * immensely tedious files called things like "read_snapshot.c" whose sole purpose was to deal 
 * with multiple files, check header consistency, work out how much memory to allocate, etc, etc.
 *
 * GadgetReader attempts to do all these things automatically with an easy programmatic interface. 
 *
 * It aims to support Gadget-I and endian swapping simply and as robustly as possible. 
 *
 * It attempts to detect a few trivial programmer errors, or file corruptions. 
 *
 * The original readgadget.c has to seek through the *whole* file to find every single block. 
 * On an NFS mounted filesystem, this can be prohibitively slow. GadgetReader builds a map of the 
 * block locations once, in the class constructor, and further reads involve only a single seek. 
 *
 * Finally, it includes an example program, a test suite, and bindings for both python and perl. 
 *
 * \section usage_sec Usage
 * To use GadgetReader in your C++ program, simply do:
 *
 * using namespace GadgetReader;
 *
 * GSnap snap("some_snapshot");
 *
 * snap.GetBlocks("POS ",data_array, snap.GetNpart(BARYON_TYPE), 0, 0);
 *
 * This will get you the positions of all baryons in the snapshot.
 *
 * A longer example is contained within PGIIhead.cpp; this program is also useful for printing Gadget file headers. 
 *
 * \section details_sec Details
 * 
 * There are a few gotchas to use of this library. 
 *
 * The largest one concerns the final argument of GetBlocks , called skip_types. 
 * In order to support getting one type of particle at a time, we need to know which particles
 * correspond to which particle types. This would be easy if all blocks contained data for all particles,
 * however this is not the case; an internal energy block, for example, will only contain data for gas particles
 * (which may be any subset of types 0 and 2-5). Therefore we provide the skip_types argument;
 * this is a bitfield which instructs GadgetReader to skip particle types which are present in the block, 
 * but which are not desired.
 *
 * If the header shows that there are no particles of a given type in this snapshot file, skip_types for this
 * type has no effect.
 *
 * Do not attempt to read multiple non-contiguous types at once, this is not yet supported.
 *
 * Some examples:
 * For a POS block containing Baryons, DM and stars, to get only the DM, use:
 *
 * skip_types = 2**BARYON_TYPE+ 2**STARS_TYPE, or 2**N_TYPE-1 -2**DM_TYPE
 *
 * For a U block containing Baryons and stars, to get only the stars, use:
 *
 * skip_types = 2**BARYON_TYPE or 2**N_TYPE -1 -2**STARS_TYPE - 2**DM_TYPE 
 *
 * Note the DM_TYPE is not skipped, as it does not appear in the file.
 *
 * Gadget-I format files do not contain block names. GadgetReader attempts to detect the default block order, but
 * this will not work if you are using a modified GADGET. Therefore, in this case, you should pass a vector of 
 * std:strings containing the ordered names of the blocks to the constructor. You can then load the particle blocks
 * as normal using GetBlocks()
 *
 * There is no metadata containing the number of bytes used per particle in a given block. 
 * GadgetReader attempts to guess this, however, should the guess be incorrect, you can override it with 
 * the SetPartLen() method.
 *
 * \section req_sec Requirements
 * A C++ compiler with map, vector, set and stdint.h
 *
 * getopt for the example program
 *
 * Boost::Test library >= 1.34 for the test suite
 *
 * Swig > 1.30 for the bindings
 */
#ifndef __GADGETREADER_H
#define __GADGETREADER_H

/*Symbol table visibility stuff*/
#if __GNUC__ >= 4
    #define DLL_PUBLIC __attribute__ ((visibility("default")))
    #define DLL_LOCAL  __attribute__ ((visibility("hidden")))
#else
    #define DLL_PUBLIC
    #define DLL_LOCAL
#endif

#include <map>
#include <set>
#include <vector>
#include <string>
#include <stdint.h>

/*Define the particle types*/
#define BARYON_TYPE 0 
#define DM_TYPE 1
#define DISK_TYPE 2 
#define NEUTRINO_TYPE 2
#define BULGE_TYPE 3
#define STARS_TYPE 4
#define BNDRY_TYPE 5
#define N_TYPE 6

/* Include the file header structure*/
#include "gadgetheader.h"

namespace GadgetReader{

  // The following are private structures that we don't want wrapped
#ifndef SWIG 
  //Filename type
  typedef std::string f_name;
  
  /** This private structure is to hold information about a given block. May change without warning, don't use it*/
 typedef struct{
    //Starting position in the file
    int64_t start_pos;
    uint64_t length; //in bytes, excluding the two integer "record sizes" at either end
    short partlen; //length for a single particle. Likely to be 4 or 12.
  } block_info;
  
  /** This private structure stores information about each file. 
   * May change without warning, don't use it.
   * Stores block maps, caches headers
   */
 typedef struct{
    //The file's actual name
    f_name name;
    //The header is stored inline, as it is small
    gadget_header header;
    //The block map
    std::map<std::string,block_info> blocks;
  } file_map;
    
#endif
  
  /** Main class for reading Gadget snapshots. */
  class DLL_PUBLIC GSnap{
          public:
                  /** Constructor: does most of the hard work of looking over the file.
                   * Will seek through the file, reading the header and building a map of where the 
                   * data blocks are.
                   * Partlen is hardcoded to be 12 for POS and VEL and 4 otherwise. 
                   * A better detection method would be nice. 
                   * @param snap_filename the (base) filename. The trailing .0 is optional
                   * @param debugf Whether runtime warnings are printed.
                   * @param BlockNames A list of block names, for format 1 files where we can't autodetect. If NULL, 
                   * a default value is returned. */
                  GSnap(std::string snap_filename, bool debugf=true, std::vector<std::string> *BlockNames=NULL);
                  /** Reads particles from a file block into the memory pointed to by block.
                   *This function is insanity with respect to bindings, for
                   * reasons which probably have to do with the total lack of memory or type safety.
                   * Therefore we don't bind it and instead provide wrapper functions. 
                   * @return The number of particles read.
                   * @param BlockName Name of block to read
                   * @param block Pointer to a block of memory to read the particles into
                   * @param npart_toread Number of particles to read
                   * @param start_part  Starting particle 
                   * @param skip_type   Types to skip, as a bitfield. 
                   * Pass 1 to skip baryons, 2 to skip dm, 3 to skip baryons and dm, etc.
                   * Only skip types for which the block is actually present:
                   * Unfortunately there is no way of the library knowing
                   * which particle has which type,
                   * so there is no way of telling that in advance.
                   * FIXME: Do not try to read two non-contiguous types from the file in one call.*/
                #ifndef SWIG
                  int64_t GetBlock(std::string BlockName, void *block, int64_t npart_toread, int64_t start_part, int skip_type);
                #endif
                  /** GetBlock overload returning a vector.
                   * @see GetBlock
                   * Memory-safe wrapper functions for the bindings. It is not anticipated that people writing codes in C
                   * will want to use these, as they need to allocate a significant quantity of temporary memory.
                   * We do not attempt to work out whether the block requested is a float or an int.*/
                  std::vector<float> GetBlock(std::string BlockName, int64_t npart_toread, int64_t start_part, int skip_type);
                  /** GetBlock overload returning an int.
                   * @see GetBlock
                   * This is here to support getting IDs, it is exactly the same as the earlier GetBlock overload*/
                  std::vector<int> GetBlockInt(std::string BlockName, int64_t npart_toread, int64_t start_part, int skip_type);
                  /* Ideally here we would have a wrapper for returning 3-float blocks such as POS and VEL, 
                   * BUT SWIG can't handle nested classes, so we can't do that.*/

                  /** Tests whether a particular block exists.  */
                  bool IsBlock(std::string BlockName);
                  /** Gets a file header.
                   * Note this means GetHeader().Npart[0] != GetNpart(0)
                   * This has to be the case to avoid overflow issues. 
                   * @param i File to read header from*/
                  gadget_header GetHeader(int i=0);
                  /** Get the filename of the snapshot, the original argument of the constructor*/
                  std::string GetFileName(){
                          return base_filename;
                  }
                  /** Get the number of files we actually found in the snapshot.
                   *  Note this is not necessarily the number the snapshot thinks is there */
                  int GetNumFiles(){
                          return file_maps.size();
                  }
                  /** Get the file format. 
                   * First bit is format 2, second is swap_endian.
                   * Allows us to test if we have Gadget 1 or endian swapped files. */
                  int GetFormat(){
                          return swap_endian*2+(!format_2);
                  }
                  /** Convenience function to get the total number of particles easily for a type
                   * Note when calculating total header, to add npart, not to use npart total, 
                   * in case we have more than 2**32 particles.
                   * @param type Particle type to get number of
                   * @param found if true will return the 
                   * particles actually found instead of those the snapshot is reporting to exist*/
                  int64_t GetNpart(int type, bool found=false);
                  /** Get total size of a block in the snapshot, in bytes.
                   * Useful for allocating memory.*/
                  int64_t GetBlockSize(std::string BlockName);
                 /** Get number of particles a block has data for, Same as GetBlockSize but divided by partlen */
                  int64_t GetBlockParts(std::string BlockName);
                 /** Get a list of all blocks present in the snapshot, as a set. */
                  std::set<std::string> GetBlocks();
                 /** Set the per-particle length for a given block to partlen.
                   * This could be useful if the automatic detection failed.*/
                  void SetPartLen(std::string BlockName, short partlen);
          private:
                  /** Private function to check whether two headers are
                   * consistent with being from the same snapshot. */
                  DLL_LOCAL bool check_headers(gadget_header head1, gadget_header head2);
                  /** Private function that does the hard work of looking over a file
                   * and constructing a map of where the blocks start and finish. */
                  DLL_LOCAL file_map construct_file_map(FILE *file,f_name filename, std::vector<std::string> *BlockNames);
                  /** Private function to detect endian swapping or format 1 files.
                   * Sets swap_endian and format_2.
                   * Returns 0 for success, 1 for an empty file, and 2 
                   * if the filetype is weird (eg, if you tried to open a text file by mistake)*/
                  DLL_LOCAL int check_filetype(FILE* fd);
                  /** Private function to read the block (not the file) header. */
                  DLL_LOCAL uint32_t read_G2_block_head(char* name, FILE *fd, const char * file);
  
                  /** Base filename for the snapshot*/
                  f_name base_filename;

                  /** Stores whether the file is endian swapped*/
                  bool swap_endian;

                  /** Stores whether we are using format 1 or 2 files*/
                  bool format_2;

                  /** This flag is a silly hack to indicate whether the header is setting
                   * the long word part of nparttotal incorrectly, as some versions of Genics do*/
                  bool bad_head64;

                  /** Vector to store the maps of each simulation snapshot*/
                  std::vector<file_map> file_maps;

                  /** Flag to control whether WARN prints anything */
                  bool debug;
  };

}

#endif //__GADGETREADER_H
