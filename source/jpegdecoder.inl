//------------------------------------------------------------------------------
// File: jpegdecoder.inl
// Small JPEG Decoder Library v0.96
// Public domain, Rich Geldreich <richgel99@gmail.com>
//------------------------------------------------------------------------------
// Retrieve one character from the input stream.
inline u32 jpeg_decoder::get_char(void)
{
  // Any bytes remaining in buffer?
  if (!m_in_buf_left)
  {
    // Try to get more bytes.
    prep_in_buffer();
    // Still nothing to get?
    if (!m_in_buf_left)
    {
      // Padd the end of the stream with 0xFF 0xD9 (EOI marker)
      int t = m_tem_flag;
      m_tem_flag ^= 1;
      if (t)
        return (0xD9);
      else
        return (0xFF);
    }
  }

  u32 c = *m_pIn_buf_ofs++;
  m_in_buf_left--;

  return (c);
}
//------------------------------------------------------------------------------
// Same as previus method, except can indicate if the character is
// a "m_padd" character or not.
inline u32 jpeg_decoder::get_char(bool *Ppadding_flag)
{
  if (!m_in_buf_left)
  {
    prep_in_buffer();
    if (!m_in_buf_left)
    {
      *Ppadding_flag = true;
      int t = m_tem_flag;
      m_tem_flag ^= 1;
      if (t)
        return (0xD9);
      else
        return (0xFF);
    }
  }

  *Ppadding_flag = false;

  u32 c = *m_pIn_buf_ofs++;
  m_in_buf_left--;

  return (c);
}
//------------------------------------------------------------------------------
// Inserts a previously retrieved character back into the input buffer.
inline void jpeg_decoder::stuff_char(u8 q)
{
  *(--m_pIn_buf_ofs) = q;
  m_in_buf_left++;
}
//------------------------------------------------------------------------------
// Retrieves one character from the input stream, but does
// not read past markers. Will continue to return 0xFF when a
// marker is encountered.
inline u8 jpeg_decoder::get_octet(void)
{
  bool padding_flag;
  int c = get_char(&padding_flag);

  if (c == 0xFF)
  {
    if (padding_flag)
      return (0xFF);

    c = get_char(&padding_flag);
    if (padding_flag)
    {
      stuff_char(0xFF);
      return (0xFF);
    }

    if (c == 0x00)
      return (0xFF);
    else
    {
      stuff_char(static_cast<u8>(c));
      stuff_char(0xFF);
      return (0xFF);
    }
  }

  return static_cast<u8>(c);
}
//------------------------------------------------------------------------------
// Retrieves a variable number of bits from the input stream.
// Does not recognize markers.
inline u32 jpeg_decoder::get_bits_1(int num_bits)
{
  if (!num_bits)
    return 0;

  u32 i = m_bit_buf >> (32 - num_bits);

  if ((m_bits_left -= num_bits) <= 0)
  {
    m_bit_buf <<= (num_bits += m_bits_left);
    
    u32 c1 = get_char();
    u32 c2 = get_char();
    m_bit_buf = (m_bit_buf & 0xFFFF0000) | (c1 << 8) | c2;
    
    m_bit_buf <<= -m_bits_left;

    m_bits_left += 16;

    assert(m_bits_left >= 0);
  }
  else
    m_bit_buf <<= num_bits;

  return i; 
}
//------------------------------------------------------------------------------
// Retrieves a variable number of bits from the input stream.
// Markers will not be read into the input bit buffer. Instead,
// an infinite number of all 1's will be returned when a marker
// is encountered.
// FIXME: Is it better to return all 0's instead, like the older implementation?
inline u32 jpeg_decoder::get_bits_2(int num_bits)
{
  if (!num_bits)
    return 0;

  u32 i = m_bit_buf >> (32 - num_bits);

  if ((m_bits_left -= num_bits) <= 0)
  {
    m_bit_buf <<= (num_bits += m_bits_left);
    
    if ((m_in_buf_left < 2) || (m_pIn_buf_ofs[0] == 0xFF) || (m_pIn_buf_ofs[1] == 0xFF))
    {   
      u32 c1 = get_octet();
      u32 c2 = get_octet();
      m_bit_buf |= (c1 << 8) | c2;
    }
    else
    {
      m_bit_buf |= ((u32)m_pIn_buf_ofs[0] << 8) | m_pIn_buf_ofs[1];
      m_in_buf_left -= 2; 
      m_pIn_buf_ofs += 2;
    }

    m_bit_buf <<= -m_bits_left;

    m_bits_left += 16;

    assert(m_bits_left >= 0);
  }
  else
    m_bit_buf <<= num_bits;

  return i;
}
//------------------------------------------------------------------------------
// Decodes a Huffman encoded symbol.
inline int jpeg_decoder::huff_decode(Phuff_tables_t Ph)
{
  int symbol;

  // Check first 8-bits: do we have a complete symbol?
  if ((symbol = Ph->look_up[m_bit_buf >> 24]) < 0)
  {
    // Decode more bits, use a tree traversal to find symbol.
    int ofs = 23;
    do
    {
      symbol = Ph->tree[-(int)(symbol + ((m_bit_buf >> ofs) & 1))];
      ofs--;
    } while (symbol < 0);

    get_bits_2(8 + (23 - ofs));
  }
  else
    get_bits_2(Ph->code_size[symbol]);

  return symbol;
}
//------------------------------------------------------------------------------
// Decodes a Huffman encoded symbol.
inline int jpeg_decoder::huff_decode(Phuff_tables_t Ph, int& extra_bits)
{
  int symbol;

  // Check first 8-bits: do we have a complete symbol?
  if ((symbol = Ph->look_up2[m_bit_buf >> 24]) < 0)
  {
    // Use a tree traversal to find symbol.
    int ofs = 23;
    do
    {
      symbol = Ph->tree[-(int)(symbol + ((m_bit_buf >> ofs) & 1))];
      ofs--;
    } while (symbol < 0);

    get_bits_2(8 + (23 - ofs));
    
    extra_bits = get_bits_2(symbol & 0xF);
  }
  else
  {
    assert(((symbol >> 8) & 31) == Ph->code_size[symbol & 255] + ((symbol & 0x8000) ? (symbol & 15) : 0));

    if (symbol & 0x8000)
    {
      get_bits_2((symbol >> 8) & 31);
      extra_bits = symbol >> 16;
    }
    else 
    {
      int code_size = (symbol >> 8) & 31;
      int num_extra_bits = symbol & 0xF;
      int bits = code_size + num_extra_bits;
      if (bits <= (m_bits_left + 16))
        extra_bits = get_bits_2(bits) & ((1 << num_extra_bits) - 1);
      else
      {
        get_bits_2(code_size);
        extra_bits = get_bits_2(num_extra_bits);
      }
    }
          
    symbol &= 0xFF;
  }

  return symbol;
}
//------------------------------------------------------------------------------
// Tables and macro used to fully decode the DPCM differences.
// (Note: In x86 asm this can be done without using tables.)
const int extend_test[16] =   /* entry n is 2**(n-1) */
  { 0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 };

const int extend_offset[16] = /* entry n is (-1 << n) + 1 */
  { 0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1,
    ((-1)<<5) + 1, ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1,
    ((-1)<<9) + 1, ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1,
    ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1 };

// used by huff_extend()
const int extend_mask[] =
{
  0,
  (1<<0), (1<<1), (1<<2), (1<<3),
  (1<<4), (1<<5), (1<<6), (1<<7),
  (1<<8), (1<<9), (1<<10), (1<<11),
  (1<<12), (1<<13), (1<<14), (1<<15),
  (1<<16),
};

#define HUFF_EXTEND_TBL(x,s) ((x) < extend_test[s] ? (x) + extend_offset[s] : (x))

#ifdef JPGD_SUPPORT_X86ASM
// Use the inline ASM version instead to prevent jump misprediction issues
  #define HUFF_EXTEND(x,s) huff_extend(x, s)
  #define HUFF_EXTEND_P(x,s) Pd->huff_extend(x, s)
#else
  #define HUFF_EXTEND(x,s) HUFF_EXTEND_TBL(x,s)
  #define HUFF_EXTEND_P(x,s) HUFF_EXTEND_TBL(x,s)
#endif
//------------------------------------------------------------------------------
#ifdef JPGD_SUPPORT_X86ASM
// This code converts the raw unsigned coefficient bits
// read from the data stream to the proper signed range.
// There are many ways of doing this, see the HUFF_EXTEND_TBL
// macro for an alternative way.
inline u32 jpeg_decoder::huff_extend(u32 i, int c)
{
  _asm
  {
    mov ecx, c
    mov eax, i
    cmp eax, [ecx*4+extend_mask]
    sbb edx, edx
    shl edx, cl
    adc eax, edx
  }
}
#endif
//------------------------------------------------------------------------------
// Clamps a value between 0-255.
inline u8 jpeg_decoder::clamp(int i)
{
	if (static_cast<u32>(i) > 255)
    i = (((~i) >> 31) & 0xFF);

  return static_cast<u8>(i);
}
//------------------------------------------------------------------------------
namespace DCT_Upsample 
{
  struct Matrix44
  {
    typedef Matrix44 Matrix_Type;
    typedef int Element_Type;
    enum { NUM_ROWS = 4, NUM_COLS = 4 };
    
    Element_Type v[NUM_ROWS][NUM_COLS];
    
    int rows() const { return NUM_ROWS; }
    int cols() const { return NUM_COLS; }
    
    const Element_Type & at(int r, int c) const { return v[r][c]; }
          Element_Type & at(int r, int c)       { return v[r][c]; }
    
    Matrix_Type& operator += (const Matrix_Type& a)
    {
      for (int r = 0; r < NUM_ROWS; r++)
      {
        at(r, 0) += a.at(r, 0);
        at(r, 1) += a.at(r, 1);
        at(r, 2) += a.at(r, 2);
        at(r, 3) += a.at(r, 3);
      }
      return *this;
    }
    
    Matrix_Type& operator -= (const Matrix_Type& a)
    {
      for (int r = 0; r < NUM_ROWS; r++)
      {
        at(r, 0) -= a.at(r, 0);
        at(r, 1) -= a.at(r, 1);
        at(r, 2) -= a.at(r, 2);
        at(r, 3) -= a.at(r, 3);
      }
      return *this;
    }
    
    friend Matrix_Type operator + (const Matrix_Type& a, const Matrix_Type& b)
    {
      Matrix_Type ret;
      for (int r = 0; r < NUM_ROWS; r++)
      {
        ret.at(r, 0) = a.at(r, 0) + b.at(r, 0);
        ret.at(r, 1) = a.at(r, 1) + b.at(r, 1);
        ret.at(r, 2) = a.at(r, 2) + b.at(r, 2);
        ret.at(r, 3) = a.at(r, 3) + b.at(r, 3);
      }
      return ret;
    }

    friend Matrix_Type operator - (const Matrix_Type& a, const Matrix_Type& b)
    {
      Matrix_Type ret;
      for (int r = 0; r < NUM_ROWS; r++)
      {
        ret.at(r, 0) = a.at(r, 0) - b.at(r, 0);
        ret.at(r, 1) = a.at(r, 1) - b.at(r, 1);
        ret.at(r, 2) = a.at(r, 2) - b.at(r, 2);
        ret.at(r, 3) = a.at(r, 3) - b.at(r, 3);
      }
      return ret;
    }

    static void add_and_store(jpgd_block_t* Pdst, const Matrix_Type& a, const Matrix_Type& b)
    {
      for (int r = 0; r < 4; r++)
      {
        Pdst[0*8 + r] = static_cast<jpgd_block_t>(a.at(r, 0) + b.at(r, 0));
        Pdst[1*8 + r] = static_cast<jpgd_block_t>(a.at(r, 1) + b.at(r, 1));
        Pdst[2*8 + r] = static_cast<jpgd_block_t>(a.at(r, 2) + b.at(r, 2));
        Pdst[3*8 + r] = static_cast<jpgd_block_t>(a.at(r, 3) + b.at(r, 3));
      }
    }

    static void sub_and_store(jpgd_block_t* Pdst, const Matrix_Type& a, const Matrix_Type& b)
    {
      for (int r = 0; r < 4; r++)
      {
        Pdst[0*8 + r] = static_cast<jpgd_block_t>(a.at(r, 0) - b.at(r, 0));
        Pdst[1*8 + r] = static_cast<jpgd_block_t>(a.at(r, 1) - b.at(r, 1));
        Pdst[2*8 + r] = static_cast<jpgd_block_t>(a.at(r, 2) - b.at(r, 2));
        Pdst[3*8 + r] = static_cast<jpgd_block_t>(a.at(r, 3) - b.at(r, 3));
      }
    }
  };
} // end namespace DCT_Upsample
//------------------------------------------------------------------------------
// Reference: "Fast Scheme for Image Size Change in the Compressed Domain"
// http://vision.ai.uiuc.edu/~dugad/research/dct/index.html
namespace DCT_Upsample 
{
	const int FRACT_BITS = 10;
	const int SCALE = 1 << FRACT_BITS;

	typedef int Temp_Type;
#define D(i) (((i) + (SCALE >> 1)) >> FRACT_BITS)
#define F(i) ((int)((i) * SCALE + .5f))

	//typedef float Temp_Type;
	//#define D(i) i
	//#define F(i) i

	// Any decent C++ compiler will optimize this at compile time to a 0, or an array access.
#define AT(c, r) ((((c)>=NUM_COLS)||((r)>=NUM_ROWS)) ? 0 : Psrc[(c)+(r)*8])

	// NUM_ROWS/NUM_COLS = # of non-zero rows/cols in input matrix
	template<int NUM_ROWS, int NUM_COLS>
	struct P_Q
	{
		static void calc(Matrix44& P, Matrix44& Q, const jpgd_block_t* Psrc)
		{
			// 4x8 = 4x8 times 8x8, matrix 0 is constant
			const Temp_Type X000 = AT(0, 0);
			const Temp_Type X001 = AT(0, 1);
			const Temp_Type X002 = AT(0, 2);
			const Temp_Type X003 = AT(0, 3);
			const Temp_Type X004 = AT(0, 4);
			const Temp_Type X005 = AT(0, 5);
			const Temp_Type X006 = AT(0, 6);
			const Temp_Type X007 = AT(0, 7);
			const Temp_Type X010 = D(F(0.415735f) * AT(1, 0) + F(0.791065f) * AT(3, 0) + F(-0.352443f) * AT(5, 0) + F(0.277785f) * AT(7, 0));
			const Temp_Type X011 = D(F(0.415735f) * AT(1, 1) + F(0.791065f) * AT(3, 1) + F(-0.352443f) * AT(5, 1) + F(0.277785f) * AT(7, 1));
			const Temp_Type X012 = D(F(0.415735f) * AT(1, 2) + F(0.791065f) * AT(3, 2) + F(-0.352443f) * AT(5, 2) + F(0.277785f) * AT(7, 2));
			const Temp_Type X013 = D(F(0.415735f) * AT(1, 3) + F(0.791065f) * AT(3, 3) + F(-0.352443f) * AT(5, 3) + F(0.277785f) * AT(7, 3));
			const Temp_Type X014 = D(F(0.415735f) * AT(1, 4) + F(0.791065f) * AT(3, 4) + F(-0.352443f) * AT(5, 4) + F(0.277785f) * AT(7, 4));
			const Temp_Type X015 = D(F(0.415735f) * AT(1, 5) + F(0.791065f) * AT(3, 5) + F(-0.352443f) * AT(5, 5) + F(0.277785f) * AT(7, 5));
			const Temp_Type X016 = D(F(0.415735f) * AT(1, 6) + F(0.791065f) * AT(3, 6) + F(-0.352443f) * AT(5, 6) + F(0.277785f) * AT(7, 6));
			const Temp_Type X017 = D(F(0.415735f) * AT(1, 7) + F(0.791065f) * AT(3, 7) + F(-0.352443f) * AT(5, 7) + F(0.277785f) * AT(7, 7));
			const Temp_Type X020 = AT(4, 0);
			const Temp_Type X021 = AT(4, 1);
			const Temp_Type X022 = AT(4, 2);
			const Temp_Type X023 = AT(4, 3);
			const Temp_Type X024 = AT(4, 4);
			const Temp_Type X025 = AT(4, 5);
			const Temp_Type X026 = AT(4, 6);
			const Temp_Type X027 = AT(4, 7);
			const Temp_Type X030 = D(F(0.022887f) * AT(1, 0) + F(-0.097545f) * AT(3, 0) + F(0.490393f) * AT(5, 0) + F(0.865723f) * AT(7, 0));
			const Temp_Type X031 = D(F(0.022887f) * AT(1, 1) + F(-0.097545f) * AT(3, 1) + F(0.490393f) * AT(5, 1) + F(0.865723f) * AT(7, 1));
			const Temp_Type X032 = D(F(0.022887f) * AT(1, 2) + F(-0.097545f) * AT(3, 2) + F(0.490393f) * AT(5, 2) + F(0.865723f) * AT(7, 2));
			const Temp_Type X033 = D(F(0.022887f) * AT(1, 3) + F(-0.097545f) * AT(3, 3) + F(0.490393f) * AT(5, 3) + F(0.865723f) * AT(7, 3));
			const Temp_Type X034 = D(F(0.022887f) * AT(1, 4) + F(-0.097545f) * AT(3, 4) + F(0.490393f) * AT(5, 4) + F(0.865723f) * AT(7, 4));
			const Temp_Type X035 = D(F(0.022887f) * AT(1, 5) + F(-0.097545f) * AT(3, 5) + F(0.490393f) * AT(5, 5) + F(0.865723f) * AT(7, 5));
			const Temp_Type X036 = D(F(0.022887f) * AT(1, 6) + F(-0.097545f) * AT(3, 6) + F(0.490393f) * AT(5, 6) + F(0.865723f) * AT(7, 6));
			const Temp_Type X037 = D(F(0.022887f) * AT(1, 7) + F(-0.097545f) * AT(3, 7) + F(0.490393f) * AT(5, 7) + F(0.865723f) * AT(7, 7));

			// 4x4 = 4x8 times 8x4, matrix 1 is constant
			P.at(0, 0) = X000;
			P.at(0, 1) = D(X001 * F(0.415735f) + X003 * F(0.791065f) + X005 * F(-0.352443f) + X007 * F(0.277785f));
			P.at(0, 2) = X004;
			P.at(0, 3) = D(X001 * F(0.022887f) + X003 * F(-0.097545f) + X005 * F(0.490393f) + X007 * F(0.865723f));
			P.at(1, 0) = X010;
			P.at(1, 1) = D(X011 * F(0.415735f) + X013 * F(0.791065f) + X015 * F(-0.352443f) + X017 * F(0.277785f));
			P.at(1, 2) = X014;
			P.at(1, 3) = D(X011 * F(0.022887f) + X013 * F(-0.097545f) + X015 * F(0.490393f) + X017 * F(0.865723f));
			P.at(2, 0) = X020;
			P.at(2, 1) = D(X021 * F(0.415735f) + X023 * F(0.791065f) + X025 * F(-0.352443f) + X027 * F(0.277785f));
			P.at(2, 2) = X024;
			P.at(2, 3) = D(X021 * F(0.022887f) + X023 * F(-0.097545f) + X025 * F(0.490393f) + X027 * F(0.865723f));
			P.at(3, 0) = X030;
			P.at(3, 1) = D(X031 * F(0.415735f) + X033 * F(0.791065f) + X035 * F(-0.352443f) + X037 * F(0.277785f));
			P.at(3, 2) = X034;
			P.at(3, 3) = D(X031 * F(0.022887f) + X033 * F(-0.097545f) + X035 * F(0.490393f) + X037 * F(0.865723f));
			// 40 muls 24 adds

			// 4x4 = 4x8 times 8x4, matrix 1 is constant
			Q.at(0, 0) = D(X001 * F(0.906127f) + X003 * F(-0.318190f) + X005 * F(0.212608f) + X007 * F(-0.180240f));
			Q.at(0, 1) = X002;
			Q.at(0, 2) = D(X001 * F(-0.074658f) + X003 * F(0.513280f) + X005 * F(0.768178f) + X007 * F(-0.375330f));
			Q.at(0, 3) = X006;
			Q.at(1, 0) = D(X011 * F(0.906127f) + X013 * F(-0.318190f) + X015 * F(0.212608f) + X017 * F(-0.180240f));
			Q.at(1, 1) = X012;
			Q.at(1, 2) = D(X011 * F(-0.074658f) + X013 * F(0.513280f) + X015 * F(0.768178f) + X017 * F(-0.375330f));
			Q.at(1, 3) = X016;
			Q.at(2, 0) = D(X021 * F(0.906127f) + X023 * F(-0.318190f) + X025 * F(0.212608f) + X027 * F(-0.180240f));
			Q.at(2, 1) = X022;
			Q.at(2, 2) = D(X021 * F(-0.074658f) + X023 * F(0.513280f) + X025 * F(0.768178f) + X027 * F(-0.375330f));
			Q.at(2, 3) = X026;
			Q.at(3, 0) = D(X031 * F(0.906127f) + X033 * F(-0.318190f) + X035 * F(0.212608f) + X037 * F(-0.180240f));
			Q.at(3, 1) = X032;
			Q.at(3, 2) = D(X031 * F(-0.074658f) + X033 * F(0.513280f) + X035 * F(0.768178f) + X037 * F(-0.375330f));
			Q.at(3, 3) = X036;
			// 40 muls 24 adds
		}
	};

	template<int NUM_ROWS, int NUM_COLS>
	struct R_S
	{
		static void calc(Matrix44& R, Matrix44& S, const jpgd_block_t* Psrc)
		{
			// 4x8 = 4x8 times 8x8, matrix 0 is constant
			const Temp_Type X100 = D(F(0.906127f) * AT(1, 0) + F(-0.318190f) * AT(3, 0) + F(0.212608f) * AT(5, 0) + F(-0.180240f) * AT(7, 0));
			const Temp_Type X101 = D(F(0.906127f) * AT(1, 1) + F(-0.318190f) * AT(3, 1) + F(0.212608f) * AT(5, 1) + F(-0.180240f) * AT(7, 1));
			const Temp_Type X102 = D(F(0.906127f) * AT(1, 2) + F(-0.318190f) * AT(3, 2) + F(0.212608f) * AT(5, 2) + F(-0.180240f) * AT(7, 2));
			const Temp_Type X103 = D(F(0.906127f) * AT(1, 3) + F(-0.318190f) * AT(3, 3) + F(0.212608f) * AT(5, 3) + F(-0.180240f) * AT(7, 3));
			const Temp_Type X104 = D(F(0.906127f) * AT(1, 4) + F(-0.318190f) * AT(3, 4) + F(0.212608f) * AT(5, 4) + F(-0.180240f) * AT(7, 4));
			const Temp_Type X105 = D(F(0.906127f) * AT(1, 5) + F(-0.318190f) * AT(3, 5) + F(0.212608f) * AT(5, 5) + F(-0.180240f) * AT(7, 5));
			const Temp_Type X106 = D(F(0.906127f) * AT(1, 6) + F(-0.318190f) * AT(3, 6) + F(0.212608f) * AT(5, 6) + F(-0.180240f) * AT(7, 6));
			const Temp_Type X107 = D(F(0.906127f) * AT(1, 7) + F(-0.318190f) * AT(3, 7) + F(0.212608f) * AT(5, 7) + F(-0.180240f) * AT(7, 7));
			const Temp_Type X110 = AT(2, 0);
			const Temp_Type X111 = AT(2, 1);
			const Temp_Type X112 = AT(2, 2);
			const Temp_Type X113 = AT(2, 3);
			const Temp_Type X114 = AT(2, 4);
			const Temp_Type X115 = AT(2, 5);
			const Temp_Type X116 = AT(2, 6);
			const Temp_Type X117 = AT(2, 7);
			const Temp_Type X120 = D(F(-0.074658f) * AT(1, 0) + F(0.513280f) * AT(3, 0) + F(0.768178f) * AT(5, 0) + F(-0.375330f) * AT(7, 0));
			const Temp_Type X121 = D(F(-0.074658f) * AT(1, 1) + F(0.513280f) * AT(3, 1) + F(0.768178f) * AT(5, 1) + F(-0.375330f) * AT(7, 1));
			const Temp_Type X122 = D(F(-0.074658f) * AT(1, 2) + F(0.513280f) * AT(3, 2) + F(0.768178f) * AT(5, 2) + F(-0.375330f) * AT(7, 2));
			const Temp_Type X123 = D(F(-0.074658f) * AT(1, 3) + F(0.513280f) * AT(3, 3) + F(0.768178f) * AT(5, 3) + F(-0.375330f) * AT(7, 3));
			const Temp_Type X124 = D(F(-0.074658f) * AT(1, 4) + F(0.513280f) * AT(3, 4) + F(0.768178f) * AT(5, 4) + F(-0.375330f) * AT(7, 4));
			const Temp_Type X125 = D(F(-0.074658f) * AT(1, 5) + F(0.513280f) * AT(3, 5) + F(0.768178f) * AT(5, 5) + F(-0.375330f) * AT(7, 5));
			const Temp_Type X126 = D(F(-0.074658f) * AT(1, 6) + F(0.513280f) * AT(3, 6) + F(0.768178f) * AT(5, 6) + F(-0.375330f) * AT(7, 6));
			const Temp_Type X127 = D(F(-0.074658f) * AT(1, 7) + F(0.513280f) * AT(3, 7) + F(0.768178f) * AT(5, 7) + F(-0.375330f) * AT(7, 7));
			const Temp_Type X130 = AT(6, 0);
			const Temp_Type X131 = AT(6, 1);
			const Temp_Type X132 = AT(6, 2);
			const Temp_Type X133 = AT(6, 3);
			const Temp_Type X134 = AT(6, 4);
			const Temp_Type X135 = AT(6, 5);
			const Temp_Type X136 = AT(6, 6);
			const Temp_Type X137 = AT(6, 7);
			// 80 muls 48 adds

			// 4x4 = 4x8 times 8x4, matrix 1 is constant
			R.at(0, 0) = X100;
			R.at(0, 1) = D(X101 * F(0.415735f) + X103 * F(0.791065f) + X105 * F(-0.352443f) + X107 * F(0.277785f));
			R.at(0, 2) = X104;
			R.at(0, 3) = D(X101 * F(0.022887f) + X103 * F(-0.097545f) + X105 * F(0.490393f) + X107 * F(0.865723f));
			R.at(1, 0) = X110;
			R.at(1, 1) = D(X111 * F(0.415735f) + X113 * F(0.791065f) + X115 * F(-0.352443f) + X117 * F(0.277785f));
			R.at(1, 2) = X114;
			R.at(1, 3) = D(X111 * F(0.022887f) + X113 * F(-0.097545f) + X115 * F(0.490393f) + X117 * F(0.865723f));
			R.at(2, 0) = X120;
			R.at(2, 1) = D(X121 * F(0.415735f) + X123 * F(0.791065f) + X125 * F(-0.352443f) + X127 * F(0.277785f));
			R.at(2, 2) = X124;
			R.at(2, 3) = D(X121 * F(0.022887f) + X123 * F(-0.097545f) + X125 * F(0.490393f) + X127 * F(0.865723f));
			R.at(3, 0) = X130;
			R.at(3, 1) = D(X131 * F(0.415735f) + X133 * F(0.791065f) + X135 * F(-0.352443f) + X137 * F(0.277785f));
			R.at(3, 2) = X134;
			R.at(3, 3) = D(X131 * F(0.022887f) + X133 * F(-0.097545f) + X135 * F(0.490393f) + X137 * F(0.865723f));
			// 40 muls 24 adds
			// 4x4 = 4x8 times 8x4, matrix 1 is constant
			S.at(0, 0) = D(X101 * F(0.906127f) + X103 * F(-0.318190f) + X105 * F(0.212608f) + X107 * F(-0.180240f));
			S.at(0, 1) = X102;
			S.at(0, 2) = D(X101 * F(-0.074658f) + X103 * F(0.513280f) + X105 * F(0.768178f) + X107 * F(-0.375330f));
			S.at(0, 3) = X106;
			S.at(1, 0) = D(X111 * F(0.906127f) + X113 * F(-0.318190f) + X115 * F(0.212608f) + X117 * F(-0.180240f));
			S.at(1, 1) = X112;
			S.at(1, 2) = D(X111 * F(-0.074658f) + X113 * F(0.513280f) + X115 * F(0.768178f) + X117 * F(-0.375330f));
			S.at(1, 3) = X116;
			S.at(2, 0) = D(X121 * F(0.906127f) + X123 * F(-0.318190f) + X125 * F(0.212608f) + X127 * F(-0.180240f));
			S.at(2, 1) = X122;
			S.at(2, 2) = D(X121 * F(-0.074658f) + X123 * F(0.513280f) + X125 * F(0.768178f) + X127 * F(-0.375330f));
			S.at(2, 3) = X126;
			S.at(3, 0) = D(X131 * F(0.906127f) + X133 * F(-0.318190f) + X135 * F(0.212608f) + X137 * F(-0.180240f));
			S.at(3, 1) = X132;
			S.at(3, 2) = D(X131 * F(-0.074658f) + X133 * F(0.513280f) + X135 * F(0.768178f) + X137 * F(-0.375330f));
			S.at(3, 3) = X136;
			// 40 muls 24 adds  
		}
	};
} // end namespace DCT_Upsample
//------------------------------------------------------------------------------

