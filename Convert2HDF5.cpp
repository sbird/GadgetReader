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
 * A quick and easy example program to extract the header of a GADGET file into ASCII*/

#include "gadgetreader.hpp"
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hdf5.h"
#include "hdf5_hl.h"

#include "type_map.h"

using namespace GadgetReader;
using namespace std;

void write_header_attributes_in_hdf5(hid_t handle, gadget_header& header)
{
  /*Write a header. Note handle should be a file handle.*/
  H5Gcreate(handle,"Header",H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  H5LTset_attribute_int(handle,"Header","NumPart_ThisFile",(int *)header.npart,N_TYPE);
  H5LTset_attribute_uint(handle,"Header","NumPart_Total",(unsigned int *)header.npartTotal,N_TYPE);
  H5LTset_attribute_uint(handle,"Header","NumPart_TotalHighWord",(unsigned int *)header.NallHW,N_TYPE);
  H5LTset_attribute_double(handle,"Header","MassTable",header.mass,N_TYPE);
  H5LTset_attribute_double(handle,"Header","Time",&header.time,1);
  H5LTset_attribute_double(handle,"Header","Redshift",&header.redshift,1);
  H5LTset_attribute_double(handle,"Header","BoxSize",&header.BoxSize,1);
  H5LTset_attribute_int(handle,"Header","NumFilesPerSnapshot",&header.num_files,1);
  H5LTset_attribute_double(handle,"Header", "Omega0", &header.Omega0,1);
  H5LTset_attribute_double(handle, "Header", "OmegaLambda", &header.OmegaLambda, 1);
  H5LTset_attribute_double(handle, "Header", "HubbleParam", &header.HubbleParam, 1);
  H5LTset_attribute_int(handle, "Header", "Flag_Sfr", &header.flag_sfr,1);
  H5LTset_attribute_int(handle, "Header", "Flag_Cooling", &header.flag_cooling,1);
  H5LTset_attribute_int(handle, "Header", "Flag_StellarAge", &header.flag_stellarage,1);
  H5LTset_attribute_int(handle, "Header", "Flag_Metals",  &header.flag_metals,1);
  H5LTset_attribute_int(handle, "Header", "Flag_Feedback", &header.flag_feedback,1);
  H5LTset_attribute_int(handle, "Header", "Flag_DoublePrecision",&header.flag_doubleprecision,1);
  H5LTset_attribute_int(handle, "Header", "Flag_IC_Info", &header.flag_ic_info,1);
  return;
}

      
int main(int argc, char* argv[]){
     int verbose=0;
     gadget_header head;
     if(argc<2){
   		fprintf(stderr,"Usage: ./Convert2HDF5 filename. Will output filename.hdf5\n");
   		exit(1);
     }

     if(argc >2) verbose=1;
     
     string filename(argv[1]);
     string outfile = filename +".hdf5";
     GSnap snap(filename);
     if(snap.GetNumFiles() < 1){
             cout<<"Unable to load file. Probably does not exist"<<endl;
             return 0;
     }
     /*Make HDF5 file*/
     hid_t hdf_file = H5Fcreate(outfile.c_str(),H5F_ACC_EXCL,H5P_DEFAULT,H5P_DEFAULT);
     if(hdf_file < 0){
        cerr<<"Could not create hdf file: "<<outfile<<endl;
        exit(1);
     }
     /*Convert the header*/
     head=snap.GetHeader();
     write_header_attributes_in_hdf5(hdf_file, head);
     
     /*Now write some data types*/
     /*Initialise the map*/
     init_map();
     set<string> blocks=snap.GetBlocks();
     set<string>::iterator it;
     for(int i = 0; i < N_TYPE; i++){
         if(snap.GetNpart(i) == 0)
             continue;
         char g_name[20];
         snprintf(g_name, 20,"PartType%d",i);
         hid_t group = H5Gcreate(hdf_file,g_name,H5P_DEFAULT, H5P_DEFAULT,H5P_DEFAULT);
         for(it=blocks.begin() ; it != blocks.end(); it++){
             int rank = 1;
             hsize_t size[2] = {snap.GetBlockSize(*it,i)/sizeof(float)};
             if(*it == "POS " || *it == "VEL "){
                 rank = 2;
                 size[0]/=3;
                 size[1]=3;
             }
               /*Default to float, except when we have IDs*/
             if(*it == "ID  "){
                 size[0] = snap.GetBlockSize(*it,i)/sizeof(int64_t);
                 int64_t * blockdata = (int64_t *)malloc(snap.GetBlockSize(*it,i));
                 snap.GetBlock(*it,(void *)blockdata,snap.GetNpart(i),0,0);
                 if(H5LTmake_dataset_long(group, type_map[*it].c_str(),rank,size,blockdata) < 0){
                    cerr<<"Failed to make dataset: ID"<<endl;
                    exit(1);
                 }
             }
             else{
                float * blockdata =(float *) malloc(sizeof(float)*snap.GetBlockSize(*it,i));
                snap.GetBlock(*it,(void *)blockdata,snap.GetNpart(i),0,0);
                if(H5LTmake_dataset_float(group,type_map[*it].c_str(),rank,size,blockdata) < 0){
                    cerr<<"Failed to make dataset: "<<*it<<endl;
                    exit(1);
                }
             }
         }
         H5Gclose(group);
     }
     H5Fclose(hdf_file);
     return 0;
}


