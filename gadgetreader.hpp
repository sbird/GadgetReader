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
/*Gadget reader library header file*/

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
#define BULGE_TYPE 3
#define STARS_TYPE 4
#define BNDRY_TYPE 5
#define N_TYPE 6

namespace GadgetReader{

  //The Gadget file header.
  /*This differs from the standard formulation by using explicit integer sizes. 
   * Note however that at least one implementation of N-GenICs does NOT have 
   * the NallHW field, setting it to something arbitrary.*/
  typedef struct {
    uint32_t npart[N_TYPE];
    double   mass[N_TYPE];
    double   time;
    double   redshift;
    int32_t  flag_sfr;
    int32_t  flag_feedback;
    int32_t  npartTotal[N_TYPE];
    int32_t  flag_cooling;
    int32_t  num_files;
    double   BoxSize;
    double   Omega0;
    double   OmegaLambda;
    double   HubbleParam; 
    int32_t  flag_stellarage;
    int32_t  flag_metals;
    int32_t  NallHW[N_TYPE];
    int32_t  flag_entr_ics;
    char     fill[256- N_TYPE*sizeof(uint32_t)- (6+N_TYPE)*sizeof(double)- (7+2*N_TYPE)*sizeof(int32_t)];  /* fills to 256 Bytes */
  } gadget_header;
 
  //Things that we don't want wrapped
#ifndef SWIG 
  //Filename type
  typedef std::string f_name;
  
  //This is to hold information about a given block
  typedef struct{
    //Starting position in the file
    int64_t start_pos;
    uint32_t length; //in bytes, excluding the two integer "record sizes" at either end
    short partlen; //length for a single particle. Likely to be 4 or 12.
  } block_info;
  
  //This structure stores information about each file. 
  //Stores block maps, headers, 
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
                  /** GSnap: Constructor: does most of the hard work of looking over the file.
                   * Will seek through the file, reading the header and building a map of where the 
                   * data blocks are.
                   * Partlen is hardcoded to be 12 for POS and VEL and 4 otherwise. 
                   * A better detection method would be nice. 
                   * The first argument is the (base) filename, the second controls whether WARN prints anything
                   * and the third is a list of block headers for format 1 files. 
                   * If not set, will use the default hard-coded values. */
                  GSnap(std::string snap_filename, bool debugf=true, std::vector<std::string> *BlockNames=NULL);
                  /* Reads particles from a file block into the memory pointed to by block
                   * Returns the number of particles read.
                   * Takes: block name, 
                   *        particles to read, 
                   *        pointer to allocated memory for block
                   *        particles to skip initially
                   *        Types to skip, as a bitfield. 
                   *        Pass 1 to skip baryons, 2 to skip dm, 3 to skip baryons and dm, etc.
                   *        Only skip types for which the block is actually present:
                   *        Unfortunately there is no way of the library knowing
                   *        which particle has which type,
                   *        so there is no way of telling that in advance.  */
                  /*This function is also insanity with respect to bindings, for 
                   * reasons which probably have to do with the total lack of memory or type safety.
                   * Therefore don't bind it and provide wrapper functions. */
                #ifndef SWIG
                  int64_t GetBlock(std::string BlockName, void *block, int64_t npart_toread, int64_t start_part, int skip_type);
                #endif
                  /* Memory-safe wrapper functions for the bindings. It is not anticipated that people writing codes in C 
                   * will want to use these, as they need to allocate a significant quantity of temporary memory.
                   * We do not attempt to work out whether the block requested is a float or an int.*/
                  std::vector<float> GetBlock(std::string BlockName, int64_t npart_toread, int64_t start_part, int skip_type);
                  /* Support getting IDs: is exactly the same as the above*/
                  std::vector<int> GetBlockInt(std::string BlockName, int64_t npart_toread, int64_t start_part, int skip_type);
                  /* Ideally here we would have a wrapper for returning 3-float blocks such as POS and VEL, 
                   * BUT SWIG can't handle nested classes, so we can't do that.*/

                  //Tests whether a particular block exists
                  bool IsBlock(std::string BlockName);
                  //Gets a file header
                  //Note this means GetHeader().Npart[0] != GetNpart(0)
                  //This has to be the case to avoid overflow issues.
                  gadget_header GetHeader(int i=0);
                  //Get the filename we put in
                  std::string GetFileName(){
                          return base_filename;
                  }
                  //Get the number of files we found in the snapshot
                  int GetNumFiles(){
                          return file_maps.size();
                  }
                  //Get the file format: first bit is format 2, second is swap_endian
                  int GetFormat(){
                          return swap_endian*2+(!format_2);
                  }
                  /*Convenience function to get the total number of particles easily for a type
                   * Note when calculating total header, to add npart, not to use npart total, 
                   * in case we have more than 2**32 particles.
                   * First argument is particle type, second, if true will return the 
                   * particles actually found instead of those the snapshot is reporting to exist*/
                  int64_t GetNpart(int type, bool found=false);
                  //Get total size of a block in the snapshot, in bytes.
                  int64_t GetBlockSize(std::string BlockName);
                  //Get number of particles a block has data for
                  int64_t GetBlockParts(std::string BlockName);
                  //Get a list of all blocks present in the snapshot.
                  std::set<std::string> GetBlocks();
                  /*Set the per-particle length for a given block to partlen.
                   * This could be useful if the automatic detection failed.*/
                  void SetPartLen(std::string BlockName, short partlen);
                  /*These functions have no purpose except to help the bindings*/
/*                  int64_t GetBlock(std::string BlockName, float *block, int64_t npart_toread, int64_t start_part, int skip_type){
                          return GetBlock(BlockName, block, npart_toread/sizeof(float),start_part,skip_type);
                  }*/
          private:
                  //Function to check the headers of different files are 
                  //consistent with each other.
                  DLL_LOCAL bool check_headers(gadget_header head1, gadget_header head2);
                  //Construct a map of where the blocks start and finish for a single file
                  DLL_LOCAL file_map construct_file_map(FILE *file,f_name filename, std::vector<std::string> *BlockNames);
                  /*Sets swap_endian and format_2. 
                   * Returns 0 for success, 1 for an empty file, and 2 
                   * if the filetype is weird (eg, if you tried to open a text file by mistake)*/
                  DLL_LOCAL int check_filetype(FILE* fd);
                  /*Read the block header*/
                  DLL_LOCAL uint32_t read_G2_block_head(char* name, FILE *fd, const char * file);
  
                  //Base filename for the snapshot
                  f_name base_filename;
                  bool swap_endian; //Do we want to swap the enddianness of the files?
                  bool format_2; //Are we using format 2 files?
                  bool bad_head64; //This flag indicates that the header is setting 
                                  //the 64 bit part of nparttotal incorrectly
                  std::vector<file_map> file_maps; //Pointer to the file data
                  /*Does WARN print anything?*/
                  bool debug;
  };

}

#endif //__GADGETREADER_H
