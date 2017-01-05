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
#include <vector>
#include <string>
#include <fstream>
#include <locale>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

using namespace GadgetReader;
using namespace std;

/* Get a list of blocks from a file, with each file on a different line*/
std::vector<std::string> get_blocks(std::string file);

int main(int argc, char* argv[]){
     int i;
     char c;
     string filename,outfile, blockfile;
     char block[5]="\0\0\0\0";
     std::vector<std::string> *BlockNames=NULL;
     std::vector<std::string> Blocks;
     while((c = getopt(argc, argv, "i:o:b:if:h")) !=-1)
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
           for(i=0; i<4;i++){
                   block[i]=toupper(block[i]);
                   if(block[i] == '\0')
                           block[i]=' ';
           }
           block[4]='\0';
           break;
        case 'f':
           blockfile=optarg;
           Blocks=get_blocks(blockfile);
           BlockNames=&Blocks;
           break;
        case 'h':
        case '?':
        default:
           fprintf(stderr,"Usage: ./PosDump -i input-file -o outputfile (will have type appended) -b block-to-extract -f list of blocks (optional)\n");
   	   exit(1);
       }
     }
     if(filename.length()==0 || outfile.length()==0 || block[0]=='\0'){
           fprintf(stderr,"Usage: ./PosDump -i input-file -o outputfile (will have type appended) -b block-to-extract\n");
   	   exit(1);
     }
     GSnap snap(filename,true,BlockNames);
     if(snap.GetNumFiles() < 1){
             cerr<<"Unable to load file. Probably does not exist"<<endl;
             return 1;
     }
     printf("Dumping block %s. Total N_part = ", block);
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
                   cerr<<"Error opening "<<typefile.str()<<": "<<strerror(errno)<<endl;
           }
           else {
                snap.GetBlock(block,data,snap.GetNpart(type),0, (1<<N_TYPE)-1-(1<<type));
                cerr<<"Writing data for type "<<type<<endl;
                fwrite(data,sizeof(float),snap.GetBlockSize(block,type)/sizeof(float),output);
                fclose(output);
           }
           free(data);
     }
     return 0;
}

/*Get a list of blocks from a file, where each line names a new block. # is a comment*/
std::vector<std::string> get_blocks(std::string file)
{
        std::vector<std::string> Blocks;
        ifstream in;
        in.open(file.c_str());
        while(in.good()){
                string line,out(4,' ');
                getline(in,line);
                if(line.size() < 1 || line[0] == '#')
                        continue;
                for(unsigned int i=0; i<min((size_t) 4,line.size());i++)
                        out[i]=toupper(line[i]);
                Blocks.push_back(out);
        }
        return Blocks;
}
