// File: idct.cpp
// 2D IDCT with several optimizations to exploit runs of contiguous zero coefficients (Rich Geldreich)
// Derived from an older version of the IJG's JPEG software.

/*
 * jidctint.c
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains a slow-but-accurate integer implementation of the
 * inverse DCT (Discrete Cosine Transform).  In the IJG code, this routine
 * must also perform dequantization of the input coefficients.
 *
 * A 2-D IDCT can be done by 1-D IDCT on each column followed by 1-D IDCT
 * on each row (or vice versa, but it's more convenient to emit a row at
 * a time).  Direct algorithms are also available, but they are much more
 * complex and seem not to be any faster when reduced to code.
 *
 * This implementation is based on an algorithm described in
 *   C. Loeffler, A. Ligtenberg and G. Moschytz, "Practical Fast 1-D DCT
 *   Algorithms with 11 Multiplications", Proc. Int'l. Conf. on Acoustics,
 *   Speech, and Signal Processing 1989 (ICASSP '89), pp. 988-991.
 * The primary algorithm described there uses 11 multiplies and 29 adds.
 * We use their alternate method with 12 multiplies and 32 adds.
 * The advantage of this method is that no data path contains more than one
 * multiplication; this allows a very simple and accurate implementation in
 * scaled fixed-point arithmetic, with a minimal number of shifts.
 */

/*----------------------------------------------------------------------------*/
#include "jpegdecoder.h"
/*----------------------------------------------------------------------------*/
#define CONST_BITS  13
#define PASS1_BITS  2
#define SCALEDONE ((s32) 1)
#define CONST_SCALE (SCALEDONE << CONST_BITS)
#define FIX(x)  ((s32) ((x) * CONST_SCALE + 0.5f))
/*----------------------------------------------------------------------------*/
#define FIX_0_298631336  ((s32)  2446)        /* FIX(0.298631336) */
#define FIX_0_390180644  ((s32)  3196)        /* FIX(0.390180644) */
#define FIX_0_541196100  ((s32)  4433)        /* FIX(0.541196100) */
#define FIX_0_765366865  ((s32)  6270)        /* FIX(0.765366865) */
#define FIX_0_899976223  ((s32)  7373)        /* FIX(0.899976223) */
#define FIX_1_175875602  ((s32)  9633)        /* FIX(1.175875602) */
#define FIX_1_501321110  ((s32)  12299)       /* FIX(1.501321110) */
#define FIX_1_847759065  ((s32)  15137)       /* FIX(1.847759065) */
#define FIX_1_961570560  ((s32)  16069)       /* FIX(1.961570560) */
#define FIX_2_053119869  ((s32)  16819)       /* FIX(2.053119869) */
#define FIX_2_562915447  ((s32)  20995)       /* FIX(2.562915447) */
#define FIX_3_072711026  ((s32)  25172)       /* FIX(3.072711026) */
/*----------------------------------------------------------------------------*/
#define DESCALE(x,n)  (((x) + (SCALEDONE << ((n)-1))) >> (n))
#define DESCALE_ZEROSHIFT(x,n)  (((x) + (128 << (n)) + (SCALEDONE << ((n)-1))) >> (n))
/*----------------------------------------------------------------------------*/
#define MULTIPLY(var,cnst)  ((var) * (cnst))
/*----------------------------------------------------------------------------*/
//#define CLAMP(i) if (i & 0xFF00) i = (((~i) >> 31) & 0xFF)
#define CLAMP(i) if (static_cast<u32>(i) > 255) i = (((~i) >> 31) & 0xFF)
/*----------------------------------------------------------------------------*/
// Compiler creates a fast path 1D IDCT for X non-zero columns
template <int NONZERO_COLS>
struct Row
{
  static void idct(int* tempptr, const jpgd_block_t* dataptr)
  {
    // will be optimized at compile time to either an array access, or 0
    #define ACCESS_COL(x) (((x) < NONZERO_COLS) ? (int)dataptr[x] : 0)
      
    const int z2 = ACCESS_COL(2);
    const int z3 = ACCESS_COL(6);

    const int z1 = MULTIPLY(z2 + z3, FIX_0_541196100);
    const int tmp2 = z1 + MULTIPLY(z3, - FIX_1_847759065);
    const int tmp3 = z1 + MULTIPLY(z2, FIX_0_765366865);

    const int tmp0 = (ACCESS_COL(0) + ACCESS_COL(4)) << CONST_BITS;
    const int tmp1 = (ACCESS_COL(0) - ACCESS_COL(4)) << CONST_BITS;

    const int tmp10 = tmp0 + tmp3;
    const int tmp13 = tmp0 - tmp3;
    const int tmp11 = tmp1 + tmp2;
    const int tmp12 = tmp1 - tmp2;

    const int atmp0 = ACCESS_COL(7);
    const int atmp1 = ACCESS_COL(5);
    const int atmp2 = ACCESS_COL(3);
    const int atmp3 = ACCESS_COL(1);

    const int bz1 = atmp0 + atmp3;
    const int bz2 = atmp1 + atmp2;
    const int bz3 = atmp0 + atmp2;
    const int bz4 = atmp1 + atmp3;
    const int bz5 = MULTIPLY(bz3 + bz4, FIX_1_175875602);
       
    const int az1 = MULTIPLY(bz1, - FIX_0_899976223);
    const int az2 = MULTIPLY(bz2, - FIX_2_562915447);
    const int az3 = MULTIPLY(bz3, - FIX_1_961570560) + bz5;
    const int az4 = MULTIPLY(bz4, - FIX_0_390180644) + bz5;
    
    const int btmp0 = MULTIPLY(atmp0, FIX_0_298631336) + az1 + az3;
    const int btmp1 = MULTIPLY(atmp1, FIX_2_053119869) + az2 + az4;
    const int btmp2 = MULTIPLY(atmp2, FIX_3_072711026) + az2 + az3;
    const int btmp3 = MULTIPLY(atmp3, FIX_1_501321110) + az1 + az4;

    tempptr[0] = DESCALE(tmp10 + btmp3, CONST_BITS-PASS1_BITS);
    tempptr[7] = DESCALE(tmp10 - btmp3, CONST_BITS-PASS1_BITS);
    tempptr[1] = DESCALE(tmp11 + btmp2, CONST_BITS-PASS1_BITS);
    tempptr[6] = DESCALE(tmp11 - btmp2, CONST_BITS-PASS1_BITS);
    tempptr[2] = DESCALE(tmp12 + btmp1, CONST_BITS-PASS1_BITS);
    tempptr[5] = DESCALE(tmp12 - btmp1, CONST_BITS-PASS1_BITS);
    tempptr[3] = DESCALE(tmp13 + btmp0, CONST_BITS-PASS1_BITS);
    tempptr[4] = DESCALE(tmp13 - btmp0, CONST_BITS-PASS1_BITS);
  }
};

template <> 
struct Row<0>
{
  static void idct(int* tempptr, const jpgd_block_t* dataptr)
  {
	   tempptr;
	   dataptr;
#if 0
    const int dcval = 0;

    tempptr[0] = dcval;
    tempptr[1] = dcval;
    tempptr[2] = dcval;
    tempptr[3] = dcval;
    tempptr[4] = dcval;
    tempptr[5] = dcval;
    tempptr[6] = dcval;
    tempptr[7] = dcval;
#endif
  }
};

template <> 
struct Row<1>
{
  static void idct(int* tempptr, const jpgd_block_t* dataptr)
  {
    const int dcval = (dataptr[0] << PASS1_BITS);

    tempptr[0] = dcval;
    tempptr[1] = dcval;
    tempptr[2] = dcval;
    tempptr[3] = dcval;
    tempptr[4] = dcval;
    tempptr[5] = dcval;
    tempptr[6] = dcval;
    tempptr[7] = dcval;
  }
};
/*----------------------------------------------------------------------------*/
// Compiler creates a fast path 1D IDCT for X non-zero rows
template <int NONZERO_ROWS>
struct Col
{
  static void idct(u8* Pdst_ptr, const int* tempptr)
  {
    // will be optimized at compile time to either an array access, or 0
    #define ACCESS_ROW(x) (((x) < NONZERO_ROWS) ? tempptr[x * 8] : 0)
    
    const int z2 = ACCESS_ROW(2);
    const int z3 = ACCESS_ROW(6);

    const int z1 = MULTIPLY(z2 + z3, FIX_0_541196100);
    const int tmp2 = z1 + MULTIPLY(z3, - FIX_1_847759065);
    const int tmp3 = z1 + MULTIPLY(z2, FIX_0_765366865);

    const int tmp0 = (ACCESS_ROW(0) + ACCESS_ROW(4)) << CONST_BITS;
    const int tmp1 = (ACCESS_ROW(0) - ACCESS_ROW(4)) << CONST_BITS;

    const int tmp10 = tmp0 + tmp3;
    const int tmp13 = tmp0 - tmp3;
    const int tmp11 = tmp1 + tmp2;
    const int tmp12 = tmp1 - tmp2;

    const int atmp0 = ACCESS_ROW(7);
    const int atmp1 = ACCESS_ROW(5);
    const int atmp2 = ACCESS_ROW(3);
    const int atmp3 = ACCESS_ROW(1);

    const int bz1 = atmp0 + atmp3;
    const int bz2 = atmp1 + atmp2;
    const int bz3 = atmp0 + atmp2;
    const int bz4 = atmp1 + atmp3;
    const int bz5 = MULTIPLY(bz3 + bz4, FIX_1_175875602);
       
    const int az1 = MULTIPLY(bz1, - FIX_0_899976223);
    const int az2 = MULTIPLY(bz2, - FIX_2_562915447);
    const int az3 = MULTIPLY(bz3, - FIX_1_961570560) + bz5;
    const int az4 = MULTIPLY(bz4, - FIX_0_390180644) + bz5;

    const int btmp0 = MULTIPLY(atmp0, FIX_0_298631336) + az1 + az3;
    const int btmp1 = MULTIPLY(atmp1, FIX_2_053119869) + az2 + az4;
    const int btmp2 = MULTIPLY(atmp2, FIX_3_072711026) + az2 + az3;
    const int btmp3 = MULTIPLY(atmp3, FIX_1_501321110) + az1 + az4;

    int i;

//#define CLAMP2(i) { int tmp = -((i & 0xFF00) != 0); i = (tmp & ((~i) >> 31)) | (~tmp & i); }
#define CLAMP2 CLAMP

    i = DESCALE_ZEROSHIFT(tmp10 + btmp3, CONST_BITS+PASS1_BITS+3);
    CLAMP2(i);
    Pdst_ptr[8*0] = (u8)i;

    i = DESCALE_ZEROSHIFT(tmp10 - btmp3, CONST_BITS+PASS1_BITS+3);
    CLAMP2(i);
    Pdst_ptr[8*7] = (u8)i;

    i = DESCALE_ZEROSHIFT(tmp11 + btmp2, CONST_BITS+PASS1_BITS+3);
    CLAMP2(i);
    Pdst_ptr[8*1] = (u8)i;

    i = DESCALE_ZEROSHIFT(tmp11 - btmp2, CONST_BITS+PASS1_BITS+3);
    CLAMP2(i);
    Pdst_ptr[8*6] = (u8)i;

    i = DESCALE_ZEROSHIFT(tmp12 + btmp1, CONST_BITS+PASS1_BITS+3);
    CLAMP2(i);
    Pdst_ptr[8*2] = (u8)i;

    i = DESCALE_ZEROSHIFT(tmp12 - btmp1, CONST_BITS+PASS1_BITS+3);
    CLAMP2(i);
    Pdst_ptr[8*5] = (u8)i;

    i = DESCALE_ZEROSHIFT(tmp13 + btmp0, CONST_BITS+PASS1_BITS+3);
    CLAMP2(i);
    Pdst_ptr[8*3] = (u8)i;

    i = DESCALE_ZEROSHIFT(tmp13 - btmp0, CONST_BITS+PASS1_BITS+3);
    CLAMP2(i);
    Pdst_ptr[8*4] = (u8)i;
  }
};

template <>
struct Col<1>
{
  static void idct(u8* Pdst_ptr, const int* tempptr) 
  {
    int dcval = DESCALE_ZEROSHIFT(tempptr[0], PASS1_BITS+3);
    
    CLAMP(dcval);
		const u8 dcvalByte = static_cast<u8>(dcval);

		Pdst_ptr[0*8] = dcvalByte;
		Pdst_ptr[1*8] = dcvalByte;
		Pdst_ptr[2*8] = dcvalByte;
		Pdst_ptr[3*8] = dcvalByte;
		Pdst_ptr[4*8] = dcvalByte;
		Pdst_ptr[5*8] = dcvalByte;
		Pdst_ptr[6*8] = dcvalByte;
		Pdst_ptr[7*8] = dcvalByte;
  }
};

#define R1_Z 1
#define R2_Z 2
#define R3_Z 3
#define R4_Z 4
#define R5_Z 5
#define R6_Z 6
#define R7_Z 7
#define R8_Z 8
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7
#define R8 8

static const u8 row_table[] = 
{
  R1_Z,  0,    0,    0,    0,    0,    0,    0,
  R2_Z,  0,    0,    0,    0,    0,    0,    0,
  R2,   R1_Z,  0,    0,    0,    0,    0,    0,
  R2,   R1,   R1_Z,  0,    0,    0,    0,    0,
  R2,   R2,   R1_Z,  0,    0,    0,    0,    0,
  R3,   R2,   R1_Z,  0,    0,    0,    0,    0,
  R4,   R2,   R1_Z,  0,    0,    0,    0,    0,
  R4,   R3,   R1_Z,  0,    0,    0,    0,    0,
  R4,   R3,   R2_Z,  0,    0,    0,    0,    0,
  R4,   R3,   R2,   R1_Z,  0,    0,    0,    0,
  R4,   R3,   R2,   R1,   R1_Z,  0,    0,    0,
  R4,   R3,   R2,   R2,   R1_Z,  0,    0,    0,
  R4,   R3,   R3,   R2,   R1_Z,  0,    0,    0,
  R4,   R4,   R3,   R2,   R1_Z,  0,    0,    0,
  R5,   R4,   R3,   R2,   R1_Z,  0,    0,    0,
  R6,   R4,   R3,   R2,   R1_Z,  0,    0,    0,
  R6,   R5,   R3,   R2,   R1_Z,  0,    0,    0,
  R6,   R5,   R4,   R2,   R1_Z,  0,    0,    0,
  R6,   R5,   R4,   R3,   R1_Z,  0,    0,    0,
  R6,   R5,   R4,   R3,   R2_Z,  0,    0,    0,
  R6,   R5,   R4,   R3,   R2,   R1_Z,  0,    0,
  R6,   R5,   R4,   R3,   R2,   R1,   R1_Z,  0,
  R6,   R5,   R4,   R3,   R2,   R2,   R1_Z,  0,
  R6,   R5,   R4,   R3,   R3,   R2,   R1_Z,  0,
  R6,   R5,   R4,   R4,   R3,   R2,   R1_Z,  0,
  R6,   R5,   R5,   R4,   R3,   R2,   R1_Z,  0,
  R6,   R6,   R5,   R4,   R3,   R2,   R1_Z,  0,
  R7,   R6,   R5,   R4,   R3,   R2,   R1_Z,  0,
  R8,   R6,   R5,   R4,   R3,   R2,   R1_Z,  0,
  R8,   R7,   R5,   R4,   R3,   R2,   R1_Z,  0,
  R8,   R7,   R6,   R4,   R3,   R2,   R1_Z,  0,
  R8,   R7,   R6,   R5,   R3,   R2,   R1_Z,  0,
  R8,   R7,   R6,   R5,   R4,   R2,   R1_Z,  0,
  R8,   R7,   R6,   R5,   R4,   R3,   R1_Z,  0,
  R8,   R7,   R6,   R5,   R4,   R3,   R2_Z,  0,
  R8,   R7,   R6,   R5,   R4,   R3,   R2,   R1_Z,
  R8,   R7,   R6,   R5,   R4,   R3,   R2,   R2_Z,
  R8,   R7,   R6,   R5,   R4,   R3,   R3,   R2_Z,
  R8,   R7,   R6,   R5,   R4,   R4,   R3,   R2_Z,
  R8,   R7,   R6,   R5,   R5,   R4,   R3,   R2_Z,
  R8,   R7,   R6,   R6,   R5,   R4,   R3,   R2_Z,
  R8,   R7,   R7,   R6,   R5,   R4,   R3,   R2_Z,
  R8,   R8,   R7,   R6,   R5,   R4,   R3,   R2_Z,
  R8,   R8,   R8,   R6,   R5,   R4,   R3,   R2_Z,
  R8,   R8,   R8,   R7,   R5,   R4,   R3,   R2_Z,
  R8,   R8,   R8,   R7,   R6,   R4,   R3,   R2_Z,
  R8,   R8,   R8,   R7,   R6,   R5,   R3,   R2_Z,
  R8,   R8,   R8,   R7,   R6,   R5,   R4,   R2_Z,
  R8,   R8,   R8,   R7,   R6,   R5,   R4,   R3_Z,
  R8,   R8,   R8,   R7,   R6,   R5,   R4,   R4_Z,
  R8,   R8,   R8,   R7,   R6,   R5,   R5,   R4_Z,
  R8,   R8,   R8,   R7,   R6,   R6,   R5,   R4_Z,
  R8,   R8,   R8,   R7,   R7,   R6,   R5,   R4_Z,
  R8,   R8,   R8,   R8,   R7,   R6,   R5,   R4_Z,
  R8,   R8,   R8,   R8,   R8,   R6,   R5,   R4_Z,
  R8,   R8,   R8,   R8,   R8,   R7,   R5,   R4_Z,
  R8,   R8,   R8,   R8,   R8,   R7,   R6,   R4_Z,
  R8,   R8,   R8,   R8,   R8,   R7,   R6,   R5_Z,
  R8,   R8,   R8,   R8,   R8,   R7,   R6,   R6_Z,
  R8,   R8,   R8,   R8,   R8,   R7,   R7,   R6_Z,
  R8,   R8,   R8,   R8,   R8,   R8,   R7,   R6_Z,
  R8,   R8,   R8,   R8,   R8,   R8,   R8,   R6_Z,
  R8,   R8,   R8,   R8,   R8,   R8,   R8,   R7_Z,
  R8,   R8,   R8,   R8,   R8,   R8,   R8,   R8_Z,
};

#define C1 1
#define C2 2
#define C3 3
#define C4 4
#define C5 5
#define C6 6
#define C7 7
#define C8 8

static const u8 col_table[] = 
{
  C1, C1, C2, C3, C3, C3, C3, C3,
  C3, C4, C5, C5, C5, C5, C5, C5,
  C5, C5, C5, C5, C6, C7, C7, C7,
  C7, C7, C7, C7, C7, C7, C7, C7,
  C7, C7, C7, C8, C8, C8, C8, C8,
  C8, C8, C8, C8, C8, C8, C8, C8,
  C8, C8, C8, C8, C8, C8, C8, C8,
  C8, C8, C8, C8, C8, C8, C8, C8,
};

void idct(const jpgd_block_t* data, u8* Pdst_ptr, int block_max_zag)
{
  assert(block_max_zag >= 1);
  assert(block_max_zag <= 64);

  if (block_max_zag == 1)
  {
    int k = ((data[0] + 4) >> 3) + 128;
    CLAMP(k);
    k = k | (k<<8);
    k = k | (k<<16);

    for (int i = 8; i > 0; i--)
    {
      *(int*)&Pdst_ptr[0] = k;
      *(int*)&Pdst_ptr[4] = k;
      Pdst_ptr += 8;
    }
    return;
  }

  int temp[64];
  
  const jpgd_block_t* dataptr = data;
  int* tempptr = temp;

  const u8* Prow_tab = &row_table[(block_max_zag - 1) * 8];
  int i;
  for (i = 8; i > 0; i--, Prow_tab++)
  {
    switch (*Prow_tab)
    {
      case 0: Row<0>::idct(tempptr, dataptr); break;
      case 1: Row<1>::idct(tempptr, dataptr); break;
      case 2: Row<2>::idct(tempptr, dataptr); break;
      case 3: Row<3>::idct(tempptr, dataptr); break;
      case 4: Row<4>::idct(tempptr, dataptr); break;
      case 5: Row<5>::idct(tempptr, dataptr); break;
      case 6: Row<6>::idct(tempptr, dataptr); break;
      case 7: Row<7>::idct(tempptr, dataptr); break;
      case 8: Row<8>::idct(tempptr, dataptr); break;
      default:
        assert(false);
    }

    dataptr += 8;
    tempptr += 8;
  }

  tempptr = temp;

  const int nonzero_rows = col_table[block_max_zag - 1];
  for (i = 8; i > 0; i--)
  {
    switch (nonzero_rows)
    {
      case 1: Col<1>::idct(Pdst_ptr, tempptr); break;
      case 2: Col<2>::idct(Pdst_ptr, tempptr); break;
      case 3: Col<3>::idct(Pdst_ptr, tempptr); break;
      case 4: Col<4>::idct(Pdst_ptr, tempptr); break;
      case 5: Col<5>::idct(Pdst_ptr, tempptr); break;
      case 6: Col<6>::idct(Pdst_ptr, tempptr); break;
      case 7: Col<7>::idct(Pdst_ptr, tempptr); break;
      case 8: Col<8>::idct(Pdst_ptr, tempptr); break;
      default:  
        assert(false);
    }

    tempptr++;
    Pdst_ptr++;
  }
}

void idct_4x4(const jpgd_block_t* data, u8* Pdst_ptr)
{
  int temp[64];
  
  const jpgd_block_t* dataptr = data;
  int* tempptr = temp;

  int i;
  for (i = 4; i > 0; i--)
  {
    Row<4>::idct(tempptr, dataptr); 
      
    dataptr += 8;
    tempptr += 8;
  }

  tempptr = temp;

  for (i = 8; i > 0; i--)
  {
    Col<4>::idct(Pdst_ptr, tempptr); 

    tempptr++;
    Pdst_ptr++;
  }
}
/*----------------------------------------------------------------------------*/
