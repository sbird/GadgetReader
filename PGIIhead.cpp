/* A quick and easy program to extract the header of a GADGET file into ASCII*/

#include "gadgetreader.hpp"
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

using namespace GadgetReader;
using namespace std;

int main(int argc, char* argv[]){
     double tot_mass=0;
     int i;
     int verbose=0;
     gadget_header head;
     if(argc<2){
   		fprintf(stderr,"Usage: ./PGIIhead filename\n");
   		exit(1);
     }

     if(argc >2) verbose=1;
     
     string filename(argv[1]);
     GSnap snap(filename);
     if(snap.GetNumFiles() < 1){
             cout<<"Unable to load file. Probably does not exist"<<endl;
             return 0;
     }
     head=snap.GetHeader();
     for(i=0; i<6; i++)
            tot_mass+=head.mass[i]; 
     /*Print the header*/
     if(verbose) printf("N_part = ");
     printf("%d %d %d %d %d %d\n", head.npart[0],head.npart[1],head.npart[2],head.npart[3], head.npart[4], head.npart[5]);
     if(verbose) printf("Mass = ");
     printf("%f %f %f %f %f %f\n", head.mass[0],head.mass[1],head.mass[2],head.mass[3], head.mass[4], head.mass[5]);
     if(verbose) printf("Time = ");
     printf("%f\n",head.time);
     if(verbose) printf("Redshift = ");
     printf("%g\n",head.redshift);
     if(verbose) printf("Star formation is: ");
     printf("%d\n",head.flag_sfr);
     if(verbose) printf("Feedback is: ");
     printf("%d\n",head.flag_feedback);
     if(verbose) printf("Total N_part = ");
     printf("%ld %ld %ld %ld %ld %ld\n", snap.GetNpart(0),snap.GetNpart(1),snap.GetNpart(2),snap.GetNpart(3), snap.GetNpart(4), snap.GetNpart(5));
     if(verbose) printf("Cooling is: ");
     printf("%d\n",head.flag_cooling);
     if(verbose) printf("Numfiles = ");
     printf("%d\n",head.num_files);
     if(verbose) printf("Boxsize = ");
     printf("%e\n",head.BoxSize);
     if(verbose) printf("Omega_0 = ");
     printf("%g\n",head.Omega0);
     if(verbose) printf("Omega_L = ");
     printf("%g\n",head.OmegaLambda);
     if(verbose) printf("H0 = ");
     printf("%g\n",head.HubbleParam);
     if(verbose) printf("Stellar age is: ");
     printf("%d\n",head.flag_stellarage);
     if(verbose) printf("Metals: ");
     printf("%d\n",head.flag_metals);
     set<string> blocks=snap.GetBlocks();
     set<string>::iterator it;
     cout<<"Non-head blocks found:"<<endl;
     for(it=blocks.begin() ; it != blocks.end(); it++)
             cout<<(*it)<<" with "<<snap.GetBlockParts(*it)<<" particles, in "<<snap.GetBlockSize(*it)<<" bytes."<<endl;
     return 0;
}
