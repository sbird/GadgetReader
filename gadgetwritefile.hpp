/* This file contains structures specialising the base file writing routine to different types of file output.
 * All format specific information is in here*/
#ifndef GADGETWRITEFILE_H
#define GADGETWRITEFILE_H
#include "gadgetwriter.hpp"

#include <stdio.h>
/*Error output macros*/
#define ERROR(...) do{ fprintf(stderr,__VA_ARGS__);abort();}while(0)
#define WARN(...) do{ \
        if(debug){ \
                fprintf(stderr,"[GadgetWriter]: "); \
                fprintf(stderr, __VA_ARGS__); \
        }}while(0)

namespace GadgetWriter{
/** Specialise to the old style binary gadget output*/
  class DLL_LOCAL GWriteFile: public GBaseWriteFile {
         public:
                GWriteFile(std::string filename, std::valarray<uint32_t> npart_in, std::vector<block_info>* BlockNames, bool format_2, bool debug);
                // Begin should specify which particle to begin at
                int64_t WriteBlock(std::string BlockName, int type, void *data, int partlen, uint32_t np_write, uint32_t begin);
                /** Note npart is silently ignored.*/
                int WriteHeader(gadget_header& head);
                ~GWriteFile()
                {
                        if(fd)
                           fclose(fd);
                }
         private:
                bool format_2;
                bool debug;
                int header_size,footer_size;
                FILE * fd;
                //Go from Key = <BlockName> Value = <Type, start>
                std::map<std::string,std::map<int, int64_t> > blocks;
                /** Private function to populate the above */
                void construct_blocks(std::vector<block_info> * BlockNames);
                /** Private function to calculate the size of a block*/
                uint32_t calc_block_size(std::string name);
                /**Function to write the block header
                 * @return 1 if error, 0 if fine.*/
                int write_block_header(FILE * fd, std::string name, uint32_t blocksize);
                /**Function to write the block header; all else as write_block_header() */
                int write_block_footer(FILE * fd, std::string name, uint32_t blocksize);
  };

#ifdef HAVE_HDF5
    /*Specialise to HDF5*/
  class DLL_LOCAL GWriteHDFFile: public GBaseWriteFile{
         public:
                GWriteHDFFile(std::string filename, std::valarray<uint32_t> npart_in, std::vector<block_info>* BlockNames, bool format_2, bool debug);
                //begin should specify which particle to begin at
                int64_t WriteBlock(std::string BlockName, int type, void *data, int partlen, uint32_t np_write, uint32_t begin);
                /** Note npart is silently ignored.*/
                int WriteHeader(gadget_header& head);
                ~GWriteHDFFile(){};
         private:
                bool debug;
                //For storing group names: PartType0, etc.
                char g_name[N_TYPE][20];
                /** Private function to find block datatype (int64 or float)*/
                std::set<std::string> m_ints;
                char get_block_type(std::string BlockName);
  };

#endif

} //GadgetWriter
#endif
