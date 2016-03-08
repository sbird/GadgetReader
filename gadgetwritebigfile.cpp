/*Write a BigFile snapshot for Gadget.*/

#ifdef HAVE_BIGFILE
#include "bigfile-mpi.h"
#include <mpi.h>
#include <valarray>
#include <string>
#include <stdint.h>
#include "gadgetwritebigfile.hpp"

namespace GadgetWriter {

  GWriteBigSnap::GWriteBigSnap(std::string snap_filename, std::valarray<int64_t> npart_in,bool debug) : 
      npart(npart_in), NumFiles(1)
  {
          //Create file
          if(0 != big_file_mpi_create(&bf, snap_filename.c_str(), MPI_COMM_WORLD)) {
              throw  std::ios_base::failure(std::string("Unable to create file: ")+snap_filename+ ":" + big_file_get_error_message());
          }
          return;
  }

  GWriteBigSnap::~GWriteBigSnap()
  {
      big_file_mpi_close(&bf, MPI_COMM_WORLD);
  }

  int GWriteBigSnap::WriteHeaders(gadget_header header)
  {
      BigBlock bheader;
      int ret = big_file_mpi_create_block(&bf, &bheader, "header", NULL, 0, 0, 0, MPI_COMM_WORLD);
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
      ret |= big_block_set_attr(&bheader, "OmegaM", &header.Omega0, "f8", 1);
      ret |= big_block_set_attr(&bheader, "OmegaB", &header.OmegaB, "f8", 1);
      ret |= big_block_set_attr(&bheader, "OmegaL", &header.OmegaLambda, "f8", 1);
      ret |= big_block_set_attr(&bheader, "HubbleParam", &header.HubbleParam, "f8", 1);
      ret |= big_block_set_attr(&bheader, "UnitLength_in_cm", &header.UnitLength_in_cm, "f8", 1);
      ret |= big_block_set_attr(&bheader, "UnitMass_in_g", &header.UnitMass_in_g, "f8", 1);
      ret |= big_block_set_attr(&bheader, "UnitVelocity_in_cm_per_s", &header.UnitVelocity_in_cm_per_s, "f8", 1);
      if (ret != 0)
          return ret;

      ret = big_block_mpi_close(&bheader, MPI_COMM_WORLD);
      if(ret != 0)
          return ret;
      return 0;
  }

  int64_t GWriteBigSnap::WriteBlock(std::string& BlockName, int type, void *data, const char * dtype, int items_per_particle, uint64_t np_write, uint64_t begin)
  {
      BigBlock block;
      BigArray array;
      BigBlockPtr ptr;
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
      int opened = big_file_mpi_open_block(&bf, &block, FullString.c_str(), MPI_COMM_WORLD);
     
      /* If we couldn't open it, try to create it. Note that the last argument, size of the array, is not dims[0], 
       * as the array could be split over different processors.*/
      if(opened < 0 && big_file_mpi_create_block(&bf, &block, FullString.c_str(), dtype, items_per_particle, NumFiles, npart[type], MPI_COMM_WORLD) != 0) {
              throw std::ios_base::failure("Unable to create block: "+FullString+ ":" + big_file_get_error_message());
      }
      if(0 != big_block_seek(&block, &ptr, begin)) {
          throw std::ios_base::failure("Failed seeking in " + FullString + " to " + std::to_string(begin) + ":" + big_file_get_error_message());
      }
      if(0 != big_block_write(&block, &ptr, &array)) {
          throw std::ios_base::failure("Failed writing " + FullString + ":" + big_file_get_error_message());
      }
      big_block_mpi_close(&block, MPI_COMM_WORLD);
      return np_write;
  }
}

#endif
