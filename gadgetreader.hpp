/*Gadget reader library header file*/

#ifndef __GADGETREADER_H
#define __GADGETREADER_H

#include <map>
#include <set>
#include <vector>
#include <string>
#include <stdint.h>
#include <stdlib.h>

/*Define the particle types*/
#define BARYON_TYPE 0 
#define DM_TYPE 1
#define DISK_TYPE 2 
#define BULGE_TYPE 3
#define STARS_TYPE 4
#define BNDRY_TYPE 5
#define N_TYPE 6

namespace gadgetreader{

  //The Gadget-II file header.
  //This differs from the standard formulation by using explicit 
  //integer sizes and including the NallHW group, which allows for more 
  //than 2^32 particles.
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
  
  //Filename type
  typedef std::string f_name;
  
  //This is to hold information about a given block
  typedef struct{
    //Starting position in the file
    int64_t start_pos;
    uint32_t length; //in bytes
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
    
  
  //Class for reading Gadget II snapshots. 
  //Eventually generalise this to Gadget I snapshots as well. 
  //A key point of this is to hide the number of files behind a generic interface, and to do as little I/O as possible.
  //Try to do all the fseeks and mapping in the constructor.
  class GadgetIISnap{
          public:
                  //Constuctor: does most of the hard work of looking over the file.
                  GadgetIISnap(f_name snap_filename, bool debugflag);
                  
                  /*Allocates memory for a block of particles, then reads the particles into 
                   * the block. Returns NULL if cannot complete.
                   * Returns a block of particles. Remember to free the pointer it gives back once done.
                   * Takes: block name, 
                   *        particles to read, 
                   *        void pointer to allocated memory for block
                   *        particles to skip initially
                   *        Types to skip, as a bitfield. 
                   *        Pass 1 to skip baryons, 3 to skip baryons and dm, 2 to skip dm, etc.
                   *        Only skip types for which the block is actually present:
                   *        Unfortunately there is no way of the library knowing which particle 
                   *        types are present*/
                  int64_t GetBlock(char* BlockName, char *block, int64_t npart_toread, int64_t start_part, int skip_type);
                  //Tests whether a particular block exists
                  bool IsBlock(char * BlockName);
                  //Gets a file header from the first file.
                  //Note this means GetHeader().Npart[0] != GetNpart(0)
                  //This has to be the case to avoid overflow issues.
                  //TODO: special-case mass; 
                  //always set the mass in the header to save memory. 
                  //No-one uses variable mass particles anyway.
                  gadget_header GetHeader();
                  //Convenience function to get the total number of particles easily for a type
                  //Note when calculating total header, to add npart, not to use npart total, 
                  //in case we have more than 2**32 particles.
                  int64_t GetNpart(int type);
                  //Get total size of a block in the snapshot, in bytes.
                  int64_t GetBlockSize(char * BlockName);
                  //Get a list of all blocks present in the snapshot.
                  std::set<std::string> GetBlocks();
          private:
                  //Construct a map of where the blocks start and finish for a single file
                  file_map construct_file_map(FILE *file,f_name filename); 
                  //Function to check the headers of different files are 
                  //consistent with each other.
                  bool check_headers(gadget_header head1, gadget_header head2);
                  //Turn a type and a particle offset into a file and starting particle.
                  int64_t find_start_position(int Type,int64_t start_part, f_name *file);
                  /*Sets swap_endian and format_2. 
                   * Returns 0 for success, 1 for an empty file, and 2 
                   * if the filetype is weird (eg, if you tried to open a text file by mistake)*/
                  int check_filetype(FILE* fd);
                  /*Read the block header*/
                  uint32_t read_G2_block_head(char* name, FILE *fd, const char * file);
  
                  //Base filename for the snapshot
                  f_name base_filename;
                  bool swap_endian; //Do we want to swap the enddianness of the files?
                  bool format_2; //Are we using format 2 files?
                  bool debug; //Output debug information?
                  std::vector<file_map> file_maps; //Pointer to the file data
  };

}

#endif //__GADGETREADER_H
