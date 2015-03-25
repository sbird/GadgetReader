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
#include "gadgetwriter.hpp"
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hdf5.h"

#include "type_map.h"

using namespace GadgetReader;
using namespace GadgetWriter;
using namespace std;

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
     /*Convert the header*/
     head=snap.GetHeader();
     std::valarray<int64_t> npart(N_TYPE);
     for (int i=0; i<N_TYPE; i++)
         npart[i] = snap.GetNpart(i);
     GWriteSnap hdf5snap(outfile, npart);
     hdf5snap.WriteHeaders(head);
     /*Now write some data types*/
     /*Initialise the map*/
     init_map();
     set<string> blocks=snap.GetBlocks();
     set<string>::iterator it;
     for(int i = 0; i < N_TYPE; i++){
         if(snap.GetNpart(i) == 0)
             continue;
         for(it=blocks.begin() ; it != blocks.end(); it++){
             /*Default to float, except when we have IDs*/
             void * blockdata;
             if(*it == "ID  ")
                 blockdata = malloc(sizeof(int64_t)*snap.GetBlockSize(*it,i));
             else
                blockdata =malloc(sizeof(float)*snap.GetBlockSize(*it,i));
             snap.GetBlock(*it,blockdata,snap.GetNpart(i),0,0);
             hdf5snap.WriteBlocks(type_map[*it], i, blockdata, snap.GetNpart(i), 0);
             free(blockdata);
         }
     }
     return 0;
}


