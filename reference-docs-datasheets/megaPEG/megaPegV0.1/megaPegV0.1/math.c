#define MATH_C
#include "equates.h"

u16 byte2int(u08 b1, u08 b0){
  return ((u16)b1<<8 | (u16)b0);
}

u32 byte2long(u08 b3, u08 b2, u08 b1, u08 b0){
  return (((u32)b3<<24) | ((u32)b2<<16) | ((u32)b1<<8) | (u32)b0);
}

u32 int2long(u16 i1, u16 i0){
  return (((u32)i1<<16) | (u32)i0);
}
