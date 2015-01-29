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
#ifndef __READ_UTILS_H
#define __READ_UTILS_H

#include <stdint.h>
/** \file 
 * Contains routines to swap the enddianness of data. */

#ifdef __cplusplus
extern "C"{
#endif //__cplusplus
/*Functions to swap the enddianness of shorts and ints.*/
//inline void endian_swap(uint16_t& x);

/** Function to swap the enddianness of ints.
 * @param x int to swap*/
inline uint32_t endian_swap(uint32_t* x)
{
    *x = (*x>>24) | 
        ((*x<<8) & 0x00FF0000) |
        ((*x>>8) & 0x0000FF00) |
        (*x<<24);
    return *x;
}

/** Function to swap the enddianness of a 64-bit thing.
 * @param x thing to swap*/
inline uint64_t endian_swap_64(uint64_t* x)
{
     *x = (((*x & 0xff00000000000000ull) >> 56)
		     | ((*x & 0x00ff000000000000ull) >> 40)
		     | ((*x & 0x0000ff0000000000ull) >> 24)
		     | ((*x & 0x000000ff00000000ull) >> 8)
		     | ((*x & 0x00000000ff000000ull) << 8)
		     | ((*x & 0x0000000000ff0000ull) << 24)
		     | ((*x & 0x000000000000ff00ull) << 40)
		     | ((*x & 0x00000000000000ffull) << 56));
    return *x;
}


/** Swap the endianness of a range of integers
 * @param start Pointer to memory to start at.
 * @param range Range to swap, in bytes.*/
uint32_t * multi_endian_swap(uint32_t * start,uint32_t range){
        uint32_t* cur=start;
        while(cur < start+range)
          endian_swap(cur++);
        return cur;
}

uint64_t * multi_endian_swap64(uint64_t * start,uint64_t range){
        uint64_t* cur=start;
        while(cur < start+range)
          endian_swap_64(cur++);
        return cur;
}

#ifdef __cplusplus
        }
#endif //__cplusplus
#endif //__READ_UTILS_H

