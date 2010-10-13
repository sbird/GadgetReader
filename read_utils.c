#include "read_utils.h"
#include <stdio.h>
#include <string.h>


/*Functions to swap the enddianness of shorts and ints.*/
/*inline void endian_swap(uint16_t& x)
{
    x = (x>>8) | 
        (x<<8);
} */         

/* Read data from a file, print an error message if things go wrong*/
/*size_t check_fread(void *ptr, size_t size, size_t nmemb, FILE * stream)
{
  size_t nread;
  if((nread = fread(ptr, size, nmemb, stream)) != nmemb){
      int err;
      if(feof(stream)){
              return 0;
      }
      WARN("fread error: %zd = fread(%d %zd %zd file)!\n",nread,fileno(stream),size,nmemb);
      err=ferror(stream);
      ERROR("ferror gives: %d : %s\n",err,strerror(err));
  }
  return nread;
}*/

/*Swap the endianness of a range of integers*/
void multi_endian_swap(uint32_t * start,int32_t range){
        uint32_t* cur=start;
        while(cur < start+range)
          endian_swap(cur++);
        return;
}

