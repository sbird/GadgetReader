#ifdef HAVE_HDF5

#include "gadgetwritefile.hpp"
#include <string>
#include <stdlib.h>
#include <hdf5.h>
#include <hdf5_hl.h>
#include <iostream>
#include <sstream>

namespace GadgetWriter{
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

  char GWriteHDFFile::get_block_type(std::string BlockName)
  {
      if (m_ints.count(BlockName) != 0)
          return 'i';
      else
          return 'f';

  }

  int GWriteHDFFile::WriteHeader(gadget_header header)
  {
            herr_t herr;
            for(int i=0; i<N_TYPE; i++){
                header.npart[i]=npart[i];
            }
            hid_t handle = H5Fopen(filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
            if (handle < 0)
                return -1*handle;
            // Lite interface for making simple attributes
            // H5LTset_attribute_int (file_id, dset_name, attr_name, data, size);
            // herr_t H5LTset_attribute_int( hid_t loc_id, const char *obj_name, const char *attr_name, int *buffer, size_t size)
            hid_t hdgrp = H5Gcreate(handle, "Header", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            herr = H5LTset_attribute_uint(handle, "Header", "NumPart_ThisFile", header.npart, N_TYPE);
            if (herr < 0)
                return -1*herr;
            herr = H5LTset_attribute_uint(handle, "Header", "NumPart_Total", header.npartTotal, N_TYPE);
            herr = H5LTset_attribute_uint(handle, "Header", "NumPart_Total_HighWord", header.NallHW, N_TYPE);
            herr = H5LTset_attribute_double(handle, "Header", "MassTable", header.mass, N_TYPE);
            if (herr < 0)
                return -1*herr;
            herr = H5LTset_attribute_double(handle, "Header", "Time", &header.time, 1);
            herr = H5LTset_attribute_double(handle, "Header", "Redshift", &header.redshift, 1);
            herr = H5LTset_attribute_double(handle, "Header", "BoxSize", &header.BoxSize, 1);
            herr = H5LTset_attribute_int(handle, "Header", "NumFilesPerSnapshot", &header.num_files, 1);
            herr = H5LTset_attribute_double(handle, "Header", "Omega0", &header.Omega0, 1);
            herr = H5LTset_attribute_double(handle, "Header", "OmegaLambda", &header.OmegaLambda, 1);
            herr = H5LTset_attribute_double(handle, "Header", "HubbleParam", &header.HubbleParam, 1);
            herr = H5LTset_attribute_int(handle, "Header", "Flag_Sfr", &header.flag_sfr, 1);
            herr = H5LTset_attribute_int(handle, "Header", "Flag_Cooling", &header.flag_cooling, 1);
            herr = H5LTset_attribute_int(handle, "Header", "Flag_StellarAge", &header.flag_stellarage, 1);
            herr = H5LTset_attribute_int(handle, "Header", "Flag_Metals", &header.flag_metals, 1);
            herr = H5LTset_attribute_int(handle, "Header", "Flag_Feedback", &header.flag_feedback, 1);
            herr = H5LTset_attribute_int(handle, "Header", "Flag_DoublePrecision", &header.flag_doubleprecision, 1);
            herr = H5LTset_attribute_int(handle, "Header", "Flag_IC_Info", &header.flag_ic_info, 1);
            if (herr < 0)
                return -1*herr;
            H5Gclose(hdgrp);
            H5Fclose(handle);
            return 0;
  }

  int64_t GWriteHDFFile::WriteBlock(std::string BlockName, int type, void *data, int partlen, uint32_t np_write, uint32_t begin)
  {
            herr_t herr;
            hid_t handle = H5Fopen(filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
            hid_t group = H5Gopen(handle, g_name[type], H5P_DEFAULT);
            if(group < 0)
                return group;
            hsize_t size[2];
            int rank=1;
            //Get type
            char b_type = get_block_type(BlockName);
            hid_t dtype;
            if(b_type == 'f') {
                size[1] = partlen/sizeof(float);
                dtype=H5T_NATIVE_FLOAT;
            }else if (b_type == 'i') {
                size[1] = partlen/sizeof(int64_t);
                //Hopefully this is 64 bits; the HDF5 manual is not clear.
                dtype = H5T_NATIVE_LLONG;
            }
            else{
                return -1000;
            }
            if (size[1] > 1) {
                    rank = 2;
            }
            /* I don't totally understand why the below works (it is not clear to me from the documentation).
             * I gleaned it from a posting to the HDF5 mailing list and a related stack overflow thread here:
             * http://stackoverflow.com/questions/24883461/hdf5-updating-a-cell-in-a-table-of-integers
             * http://lists.hdfgroup.org/pipermail/hdf-forum_lists.hdfgroup.org/2014-July/007966.html
             * The important thing seems to be that we have a dataspace for the whole array and create a hyperslab on that dataspace.
             * Then we need another dataspace with the size of the stuff we want to write.*/
            //Make space in memory for the whole array
            //Create a hyperslab that we will write to
            size[0] = npart[type];
            hid_t full_space_id = H5Screate_simple(rank, size, NULL);
            //If this is the first write, create the dataset
            if (begin==0) {
                H5Dcreate(group,BlockName.c_str(),dtype, full_space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            }
            hid_t dset = H5Dopen(group,BlockName.c_str(),H5P_DEFAULT);
            if (dset < 0)
                return dset;
            size[0] = np_write;
            hid_t space_id = H5Screate_simple(rank, size, NULL);
            hsize_t begins[2]={begin,0};
            //Select the hyperslab of elements we are about to write to
            H5Sselect_hyperslab(full_space_id, H5S_SELECT_SET, begins, NULL, size, NULL);
            /* Write to the dataset */
            herr = H5Dwrite(dset, dtype, space_id, full_space_id, H5P_DEFAULT, data);
            H5Dclose(dset);
            H5Sclose(space_id);
            H5Sclose(full_space_id);
            H5Gclose(group);
            H5Fclose(handle);
            if (herr < 0)
                return herr;
            return np_write;
  }

}

#endif


