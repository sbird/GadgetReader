/*Write a BigFile snapshot for Gadget.*/
#ifdef HAVE_BGFL
#ifdef BIGFILE_MPI
#include "bigfile-mpi.h"
#include <mpi.h>
#else
#include "bigfile.h"
#endif
#include <iostream>
#include <valarray>
#include <string>
#include <stdint.h>
#include "gadgetwritebigfile.hpp"

#ifdef BIGFILE_MPI
#define BIG_FILE_CREATE(bf, filename) big_file_mpi_create(bf, filename, MPI_COMM_WORLD)
#define BIG_FILE_CLOSE(bf) big_file_mpi_close(bf, MPI_COMM_WORLD)
#define BIG_FILE_CREATE_BLOCK(bf, block, basename, dtype, nmemb, Nfile, size) \
    big_file_mpi_create_block(bf, block, basename, dtype, nmemb, Nfile, size, MPI_COMM_WORLD)
#define BIG_FILE_OPEN_BLOCK(bf, block, basename) big_file_mpi_open_block(bf, block, basename, MPI_COMM_WORLD)
#define BIG_BLOCK_CLOSE(bheader) big_block_mpi_close(bheader, MPI_COMM_WORLD)
#else
int big_file_create_block_wrapper(BigFile * bf, BigBlock * block, const char * blockname, const char * dtype, int nmemb, int Nfile, size_t size) {
    size_t fsize[Nfile];
    int i;
    for(i = 0; i < Nfile; i ++) {
        fsize[i] = size * (i + 1) / Nfile 
                 - size * (i) / Nfile;
    }
    return big_file_create_block(bf, block, blockname, dtype, nmemb, Nfile, fsize);
}

#define BIG_FILE_CREATE(bf, filename) big_file_create(bf, filename)
#define BIG_FILE_CLOSE(bf) big_file_close(bf)
#define BIG_FILE_CREATE_BLOCK(bf, block, basename, dtype, nmemb, Nfile, size) \
    big_file_create_block_wrapper(bf, block, basename, dtype, nmemb, Nfile, size)
#define BIG_FILE_OPEN_BLOCK(bf, block, basename) big_file_open_block(bf, block, basename)
#define BIG_BLOCK_CLOSE(bheader) big_block_close(bheader)

#endif

namespace GadgetWriter {

  GWriteBigSnap::GWriteBigSnap(const std::string snap_filename, std::valarray<int64_t> npart_in,int num_files, bool debug) :
      GWriteBaseSnap(4, npart_in, num_files, debug)
  {
          //Create file
          bf = {0};
          if(0 != BIG_FILE_CREATE(&bf, snap_filename.c_str())) {
              throw  std::ios_base::failure(std::string("Unable to create file: ")+snap_filename+ ":" + big_file_get_error_message());
          }

          return;
  }

  GWriteBigSnap::~GWriteBigSnap()
  {
      BIG_FILE_CLOSE(&bf);
  }

  int GWriteBigSnap::WriteHeaders(gadget_header header)
  {
      BigBlock bheader = {0};
      int ret = BIG_FILE_CREATE_BLOCK(&bf, &bheader, "header", NULL, 0, 0, 0);
      if (ret != 0)
          return ret;
      int64_t npart_arr[N_TYPE];
      for(int i=0; i< N_TYPE; ++i){
          npart_arr[i] = npart[i];
      }
      ret |= big_block_set_attr(&bheader, "TotNumPart", npart_arr, "i8", N_TYPE);
      ret |= big_block_set_attr(&bheader, "MassTable", header.mass, "f8", N_TYPE);
      ret |= big_block_set_attr(&bheader, "Time", &header.time, "f8", 1);
      ret |= big_block_set_attr(&bheader, "Redshift", &header.redshift, "f8", 1);
      ret |= big_block_set_attr(&bheader, "BoxSize", &header.BoxSize, "f8", 1);
      ret |= big_block_set_attr(&bheader, "Omega0", &header.Omega0, "f8", 1);
      ret |= big_block_set_attr(&bheader, "OmegaB", &header.OmegaB, "f8", 1);
      ret |= big_block_set_attr(&bheader, "OmegaL", &header.OmegaLambda, "f8", 1);
      ret |= big_block_set_attr(&bheader, "HubbleParam", &header.HubbleParam, "f8", 1);
      ret |= big_block_set_attr(&bheader, "UnitLength_in_cm", &header.UnitLength_in_cm, "f8", 1);
      ret |= big_block_set_attr(&bheader, "UnitMass_in_g", &header.UnitMass_in_g, "f8", 1);
      ret |= big_block_set_attr(&bheader, "UnitVelocity_in_cm_per_s", &header.UnitVelocity_in_cm_per_s, "f8", 1);
      if (ret != 0)
          return ret;

      ret = BIG_BLOCK_CLOSE(&bheader);
      if(ret != 0)
          return ret;
      return 0;
  }

  int64_t GWriteBigSnap::WriteBlocks(const std::string& BlockName, int type, void *data, uint64_t np_write, uint64_t begin, const char * dtype, int items_per_particle)
  {
      BigBlock block = {0};
      BigArray array = {0};
      BigBlockPtr ptr = {0};
      size_t dims[2];
      ptrdiff_t strides[2];
      /*This should be the total size of the array*/
      dims[0] = np_write;
      dims[1] = items_per_particle;
      strides[1] = dtype_itemsize(dtype);
      strides[0] = items_per_particle * strides[1];
      /*Initialise the BigArray with this data.*/
      big_array_init(&array, data, dtype, 2, dims, strides);
    
      std::string FullString(std::to_string(type)+"/"+BlockName);
      /*Try to open the block*/
      int opened = BIG_FILE_OPEN_BLOCK(&bf, &block, FullString.c_str());
     
      /* If we couldn't open it, try to create it. Note that the last argument, size of the array, is not dims[0], 
       * as the array could be split over different processors.*/
      if(opened < 0 && BIG_FILE_CREATE_BLOCK(&bf, &block, FullString.c_str(), dtype, items_per_particle, num_files, npart[type]) != 0) {
              throw std::ios_base::failure("Unable to create block: "+FullString+ ":" + big_file_get_error_message());
      }
      if(0 != big_block_seek(&block, &ptr, begin)) {
          throw std::ios_base::failure("Failed seeking in " + FullString + " to " + std::to_string(begin) + ":" + big_file_get_error_message());
      }
      if(0 != big_block_write(&block, &ptr, &array)) {
          throw std::ios_base::failure("Failed writing " + FullString + ":" + big_file_get_error_message());
      }
      BIG_BLOCK_CLOSE(&block);
      return np_write;
  }
}

#endif
