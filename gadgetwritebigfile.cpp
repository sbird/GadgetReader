#ifdef HAVE_BIGFILE
#include "bigfile.h"
#include "gadgetwritefile.hpp"

  GWriteHDFFile::GWriteHDFFile(std::string filename, std::valarray<uint32_t> npart_in, std::vector<block_info>* BlockNames, bool format_2, bool debug) : GBaseWriteFile(filename, npart_in), debug(debug)
  {
          //Create file
          hid_t hdf_file = H5Fcreate(filename.c_str(),H5F_ACC_TRUNC,H5P_DEFAULT,H5P_DEFAULT);
          if(hdf_file < 0){
              throw  std::ios_base::failure(std::string("Unable to create file: ")+filename);
          }
          //Create groups in the file
          for(int i = 0; i < N_TYPE; i++){
                if(npart_in[i] == 0)
                    continue;
                snprintf(g_name[i], 20,"PartType%d",i);
                hid_t group = H5Gcreate(hdf_file,g_name[i],H5P_DEFAULT, H5P_DEFAULT,H5P_DEFAULT);
                if (group < 0)
                    throw  std::ios_base::failure(std::string("Unable to create group: ")+std::string(g_name[i]));
          }
          //Create metadata about datablocks: we only actually use partlen.
          std::vector<block_info>::iterator it;
          for(it=(*BlockNames).begin(); it<(*BlockNames).end(); ++it){
              //Detect an integer type
              if ((*it).partlen == sizeof(int64_t))
                  m_ints.insert((*it).name);
          }
          return;

  }
#endif
