#ifndef __READ_UTILS_H
#define __READ_UTILS_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"{
#endif //__cplusplus
/*Functions to swap the enddianness of shorts and ints.*/
//inline void endian_swap(uint16_t& x);

inline uint32_t endian_swap(uint32_t* x)
{
    *x = (*x>>24) | 
        ((*x<<8) & 0x00FF0000) |
        ((*x>>8) & 0x0000FF00) |
        (*x<<24);
    return *x;
}

/*Swap the endianness of a range of integers*/
/*range is in bytes*/
void multi_endian_swap(uint32_t * start,int32_t range);

/*Error output macro*/
#define ERROR(...) do{ fprintf(stderr,__VA_ARGS__);exit(1);}while(0)
#define WARN(...) fprintf(stderr, __VA_ARGS__)
#ifdef __cplusplus
        }
#endif //__cplusplus
#endif //__READ_UTILS_H

