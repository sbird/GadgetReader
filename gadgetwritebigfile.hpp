#ifndef GADGETWRITE_BIGFILE_H
#define GADGETWRITE_BIGFILE_H

#include <valarray>
#include <string>
#include <stdint.h>
#include "gadgetheader.h"
#include "bigfile.h"

namespace GadgetWriter {
  /** Main class for reading Gadget BigFile snapshots. */
  class GWriteBigSnap {
          public:
                  /** Base constructor. If you want an HDF5 snapshot, pass a filename ending in .hdf5 */
                  GWriteBigSnap(std::string snap_filename, std::valarray<int64_t> npart_in,int NumFiles, bool debug=true);
                  int64_t WriteBlock(std::string& BlockName, int type, void *data, const char * dtype, int items_per_particle, uint64_t np_write, uint64_t begin);
                  //Note the value of npart used is that from npart_in, not the header.
                  int WriteHeaders(gadget_header head);
                  //Returns the format. 1== Gadget-1, 2==Gadget-2, 3==HDF5, 4==BigFile.
                  int GetFormat(){
                      return 4;
                  }
                  ~GWriteBigSnap();
          private:
                  BigFile bf;
                  std::valarray<int64_t> npart;
                  /** Flag to control whether WARN prints anything */
                  //Number of files to use on-disc
                  const int NumFiles;
  };

}

#endif
