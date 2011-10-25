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
     char *indir=NULL,*gasfile=NULL,*dmfile=NULL;
     FILE* output;
     while((c = getopt(argc, argv, "i:g:d:h")) !=-1)
     {
       switch(c)
       {
        case 'i':
           indir=optarg;
           break;
        case 'g':
           gasfile=optarg;
           break;
        case 'd':
           dmfile=optarg;
           break;
        case 'h':
        case '?':
        default:
           fprintf(stderr,"Usage: ./PosDump -i input-file -g file-for-gas-positions -d file-for-dm-positions\n");
   	   exit(1);
       }
     }
     if(!indir || !gasfile || !dmfile){
           fprintf(stderr,"Usage: ./PosDump -i input-file -g file-for-gas-positions -d file-for-dm-positions\n");
   	   exit(1);
     }

     string filename(indir);
     GSnap snap(filename);
     if(snap.GetNumFiles() < 1){
             cout<<"Unable to load file. Probably does not exist"<<endl;
             return 0;
     }
     printf("Total N_part = ");
     printf("%ld %ld %ld %ld %ld %ld\n", snap.GetNpart(0),snap.GetNpart(1),snap.GetNpart(2),snap.GetNpart(3), snap.GetNpart(4), snap.GetNpart(5));
     float * gaspos=(float *) malloc(snap.GetNpart(0)*3*sizeof(float));
     if(!gaspos){
         fprintf(stderr,"Error allocating memory\n");
         exit(1);
     } 
     snap.GetBlock("POS ",gaspos,snap.GetNpart(0),0, (1<<N_TYPE)-1-(1<<0));
     if(!(output=fopen(gasfile,"w"))){
             fprintf(stderr, "Error opening %s: %s\n",gasfile, strerror(errno));
     }
     fprintf(stderr,"Writing gas positions...\n");
     fwrite(gaspos,sizeof(float),snap.GetNpart(0)*3,output);
     fclose(output);
     free(gaspos);
     float * dmpos=(float *) malloc(snap.GetNpart(1)*3*sizeof(float));
     if(!dmpos){
         fprintf(stderr,"Error allocating memory\n");
         exit(1);
     } 
     snap.GetBlock("POS ",dmpos,snap.GetNpart(1),0, (1<<N_TYPE)-1-(1<<1));
     if(!(output=fopen(dmfile,"w"))){
             fprintf(stderr, "Error opening %s: %s\n",dmfile, strerror(errno));
     }
     fprintf(stderr,"Writing dm positions...\n");
     fwrite(dmpos,sizeof(float),snap.GetNpart(1)*3,output);   
     fclose(output);
     free(dmpos);
     return 0;
}
