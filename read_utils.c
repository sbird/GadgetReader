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

