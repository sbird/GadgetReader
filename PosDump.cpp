/* Copyright (c) 2011, Simeon Bird <spb41@cam.ac.uk>
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
 * A quick and easy example program to extract POS arrays*/

#include "gadgetreader.hpp"
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

using namespace GadgetReader;
using namespace std;

int main(int argc, char* argv[]){
     double tot_mass=0;
     int i;
     char c;
     string filename,outfile;
     char block[5]="\0\0\0\0";
     while((c = getopt(argc, argv, "i:o:b:h")) !=-1)
     {
       switch(c)
       {
        case 'i':
           filename=optarg;
           break;
        case 'o':
           outfile=optarg;
           break;
        case 'b':
           strncpy(block,optarg,4);
           block[4]='\0';
           break;
        case 'h':
        case '?':
        default:
           fprintf(stderr,"Usage: ./PosDump -i input-file -o outputfile (will have type appended) -b block-to-extract\n");
   	   exit(1);
       }
     }
     if(filename.length()==0 || outfile.length()==0 || !block){
           fprintf(stderr,"Usage: ./PosDump -i input-file -o outputfile (will have type appended) -b block-to-extract\n");
   	   exit(1);
     }

     GSnap snap(filename);
     if(snap.GetNumFiles() < 1){
             cerr<<"Unable to load file. Probably does not exist"<<endl;
             return 1;
     }
     printf("Total N_part = ");
     printf("%ld %ld %ld %ld %ld %ld\n", snap.GetNpart(0),snap.GetNpart(1),snap.GetNpart(2),snap.GetNpart(3), snap.GetNpart(4), snap.GetNpart(5));
     if(!snap.IsBlock(block)){
             cerr<<"Block "<<block<<" not found in file "<<filename<<endl;
             return 1;
     }

     for(int type=0; type<N_TYPE; type++){ 
           if(snap.GetNpart(type) < 1)
               continue;
           FILE* output;
           float * data=(float *) malloc(snap.GetBlockSize(block,type));
           if(!data){
               fprintf(stderr,"Error allocating memory for type %d\n",type);
               exit(1);
           } 
           stringstream typefile;
           typefile<<outfile<<"_type_"<<type<<".txt";
           if(!(output=fopen(typefile.str().c_str(),"w"))){
                   cerr<<"Error opening "<<typefile<<": "<<strerror(errno)<<endl;
           }
           snap.GetBlock(block,data,snap.GetNpart(0),0, (1<<N_TYPE)-1-(1<<type));
           cerr<<"Writing data for type "<<type<<endl;
           fwrite(data,sizeof(float),snap.GetBlockSize(block,type)/sizeof(float),output);
           fclose(output);
           free(data);
     }
     return 0;
}
