/*Gadget reader library header file*/

#ifndef __GADGETREADER_H
#define __GADGETREADER_H

#include <map>
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

namespace gadgetreader{

  //The Gadget-II file header.
  //This differs from the standard formulation by using explicit 
  //integer sizes and including the NallHW group, which allows for more 
  //than 2^32 particles.
  typedef struct {
    uint32_t npart[6];
    double   mass[6];
    double   time;
    double   redshift;
    int32_t      flag_sfr;
    int32_t      flag_feedback;
    int32_t      npartTotal[6];
    int32_t      flag_cooling;
    int32_t      num_files;
    double   BoxSize;
    double   Omega0;
    double   OmegaLambda;
    double   HubbleParam; 
    int32_t      flag_stellarage;
    int32_t      flag_metals;
    int32_t      NallHW[6];
    int32_t      flag_entr_ics;
    char     fill[256- 6*4- 6*8- 2*8- 2*4- 6*4- 2*4 - 4*8 - 2*4 -6*4-4];  /* fills to 256 Bytes */
  } gadget_header;
  
  //Filename type
  typedef std::string f_name;
  
  //This is to hold information about a given block
  typedef struct{
    //Starting position in the file
    int64_t start_pos;
    uint32_t length; //in bytes
    short per_part_length; //length for a single particle. Likely to be 4 or 12.
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
                  //Constuctor
                  GadgetIISnap(f_name snap_filename, bool debugflag);
                  //Copy constructor
                  GadgetIISnap(const GadgetIISnap& other);
                  GadgetIISnap& operator=(const GadgetIISnap& rhs);
                  //Destructor
                  ~GadgetIISnap();
                  //Copy constructor
                  GadgetIISnap(GadgetIISnap &);

                  //Get a pointer to a block of data. Allocate memory in the function.
                  //Note we want this pointer to be good even when the object has been destroyed
                  //Remember to delete it when done. Maybe use a smart pointer?
                  float * GetBlock(char* BlockName, int Type, int64_t start_part, int64_t end_part);
                  //Tests whether a particular block exists
                  bool IsBlock(char * BlockName);
                  //Gets a header for the first file
                  //TODO: special-case mass; 
                  //always set the mass in the header to save memory. 
                  //No-one uses variable mass particles anyway.
                  gadget_header GetHeader();
                  //Convenience function to get the total number of particles easily for a type
                  //Note when calculating total header, to add npart, not to use npart total, 
                  //in case we have more than 2**32 particles.
                  int64_t GetNpart(int Type);
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
                  int num_files; //Number of files in the snapshot
                  //This is private because the whole point of the interface is that 
                  //we never want to know it.
                  bool swap_endian; //Do we want to swap the enddianness of the files?
                  bool format_2; //Are we using format 2 files?
                  bool debug; //Output debug information?
                  file_map *file_maps; //Pointer to the file data
  };

}

#endif //__GADGETREADER_H
