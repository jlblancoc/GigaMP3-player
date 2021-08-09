#ifndef MATH_H
  #define MATH_H

  u16 byte2int (u08 b1, u08 b0);
  u32 byte2long(u08 b3, u08 b2, u08 b1, u08 b0);
  u32 int2long (u16 i1, u16 i0);

  #define high(x) (((x) >> 8) & 0xff)
  #define low(x)  ((x) & 0xff)

#endif
