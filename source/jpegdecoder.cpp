//------------------------------------------------------------------------------
// File: jpegdecoder.cpp
// Small JPEG Decoder Library v0.96
// Public domain, Rich Geldreich <richgel99@gmail.com>
// Last updated Nov. 26, 2010
//------------------------------------------------------------------------------
#include "jpegdecoder.h"
//------------------------------------------------------------------------------
#pragma warning (disable : 4611) //warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
//------------------------------------------------------------------------------
// Disable this define to enable "center of bucket" AC coefficient dequantization. 
// Otherwise, AC coefficients are dequantized to slightly lower magnitudes within the quantization interval.
#define JPGD_APPROX_LAPLACIAN_AC_DEQUANT 1

// Set to 1 to enable freq. domain chroma upsampling on images using H2V2 subsampling (0=faster nearest neighbor sampling).
#define JPGD_SUPPORT_FREQ_DOMAIN_UPSAMPLING 1
//------------------------------------------------------------------------------
// DCT coefficients are stored in this sequence.
static int g_ZAG[64] =
{
  0,  1,  8, 16,  9,  2,  3, 10,
 17, 24, 32, 25, 18, 11,  4,  5,
 12, 19, 26, 33, 40, 48, 41, 34,
 27, 20, 13,  6,  7, 14, 21, 28,
 35, 42, 49, 56, 57, 50, 43, 36,
 29, 22, 15, 23, 30, 37, 44, 51,
 58, 59, 52, 45, 38, 31, 39, 46,
 53, 60, 61, 54, 47, 55, 62, 63,
};
//------------------------------------------------------------------------------
// Unconditionally frees all allocated m_blocks.
void jpeg_decoder::free_all_blocks(void)
{
  if (m_pStream)
  {
    m_pStream->detach();
    m_pStream = NULL;
  }

  for (int i = 0; i < JPGD_MAX_ALLOCATED_BLOCKS; i++)
  {
    free(m_blocks[i]);
    m_blocks[i] = NULL;
  }
}
//------------------------------------------------------------------------------
// This method handles all errors.
// It could easily be changed to use C++ exceptions.
void jpeg_decoder::terminate(int status)
{
  m_error_code = status;

  free_all_blocks();

  longjmp(m_jmp_state, status);
}
//------------------------------------------------------------------------------
// Allocate a block of memory-- store block's address in list for
// later deallocation by free_all_blocks().
void *jpeg_decoder::alloc(int n)
{
  // Find a free slot. The number of allocated slots will
  // always be very low, so a linear search is good enough.
  int i;
  for (i = 0; i < JPGD_MAX_ALLOCATED_BLOCKS; i++)
    if (m_blocks[i] == NULL)
      break;

  if (i == JPGD_MAX_ALLOCATED_BLOCKS)
    terminate(JPGD_TOO_MANY_BLOCKS);

  void *q = malloc(n + 8);

  if (q == NULL)
    terminate(JPGD_NOTENOUGHMEM);

  memset(q, 0, n + 8);

  m_blocks[i] = q;

  // Round to qword boundry, to avoid misaligned accesses with MMX code (old comment - mmx code removed 11/26/10)
  return ((void *)(((u32)q + 7) & ~7));
}
//------------------------------------------------------------------------------
// Clear buffer to word values.
void jpeg_decoder::word_clear(void *p, u16 c, u32 n)
{
  u16 *ps = (u16 *)p;
  while (n)
  {
		// TODO: fix unaligned writes
    *ps++ = c;
    n--;
  }
}
//------------------------------------------------------------------------------
// Refill the input buffer.
// This method will sit in a loop until (A) the buffer is full or (B)
// the stream's read() method reports and end of file condition.
void jpeg_decoder::prep_in_buffer(void)
{
  m_in_buf_left = 0;
  m_pIn_buf_ofs = m_in_buf;

  if (m_eof_flag)
    return;

  do
  {
    int bytes_read = m_pStream->read(m_in_buf + m_in_buf_left,
                                   JPGD_INBUFSIZE - m_in_buf_left,
                                   &m_eof_flag);

    if (bytes_read == -1)
      terminate(JPGD_STREAM_READ);

    m_in_buf_left += bytes_read;

  } while ((m_in_buf_left < JPGD_INBUFSIZE) && (!m_eof_flag));

  m_total_bytes_read += m_in_buf_left;

  word_clear(m_pIn_buf_ofs + m_in_buf_left, 0xD9FF, 64);
}
//------------------------------------------------------------------------------
// Read a Huffman code table.
void jpeg_decoder::read_dht_marker(void)
{
  int i, index, count;
  u32 left;
  u8 m_huff_num[17];
  u8 m_huff_val[256];

  left = get_bits_1(16);

  if (left < 2)
    terminate(JPGD_BAD_DHT_MARKER);

  left -= 2;

  while (left)
  {
    index = get_bits_1(8);

    m_huff_num[0] = 0;

    count = 0;

    for (i = 1; i <= 16; i++)
    {
      m_huff_num[i] = static_cast<u8>(get_bits_1(8));
      count += m_huff_num[i];
    }

    if (count > 255)
      terminate(JPGD_BAD_DHT_COUNTS);

    for (i = 0; i < count; i++)
      m_huff_val[i] = static_cast<u8>(get_bits_1(8));

    i = 1 + 16 + count;

    if (left < (u32)i)
      terminate(JPGD_BAD_DHT_MARKER);

    left -= i;

    if ((index & 0x10) > 0x10)
      terminate(JPGD_BAD_DHT_INDEX);
    
    index = (index & 0x0F) + ((index & 0x10) >> 4) * (JPGD_MAXHUFFTABLES >> 1);

    if (index >= JPGD_MAXHUFFTABLES)
      terminate(JPGD_BAD_DHT_INDEX);

    if (!this->m_huff_num[index])
      this->m_huff_num[index] = (u8 *)alloc(17);

    if (!this->m_huff_val[index])
      this->m_huff_val[index] = (u8 *)alloc(256);

    this->m_huff_ac[index] = (index & 0x10) != 0;
    memcpy(this->m_huff_num[index], m_huff_num, 17);
    memcpy(this->m_huff_val[index], m_huff_val, 256);
  }
}
//------------------------------------------------------------------------------
// Read a quantization table.
void jpeg_decoder::read_dqt_marker(void)
{
  int n, i, prec;
  u32 left;
  u32 temp;

  left = get_bits_1(16);

  if (left < 2)
    terminate(JPGD_BAD_DQT_MARKER);

  left -= 2;

  while (left)
  {
    n = get_bits_1(8);
    prec = n >> 4;
    n &= 0x0F;

    if (n >= JPGD_MAXQUANTTABLES)
      terminate(JPGD_BAD_DQT_TABLE);

    if (!m_quant[n])
      m_quant[n] = (jpgd_quant_t *)alloc(64 * sizeof(jpgd_quant_t));

    // read quantization entries, in zag order
    for (i = 0; i < 64; i++)
    {
      temp = get_bits_1(8);

      if (prec)
        temp = (temp << 8) + get_bits_1(8);

			m_quant[n][i] = static_cast<jpgd_quant_t>(temp);
    }

    i = 64 + 1;

    if (prec)
      i += 64;

    if (left < (u32)i)
      terminate(JPGD_BAD_DQT_LENGTH);

    left -= i;
  }
}
//------------------------------------------------------------------------------
// Read the start of frame (SOF) marker.
void jpeg_decoder::read_sof_marker(void)
{
  int i;
  u32 left;

  left = get_bits_1(16);

  if (get_bits_1(8) != 8)   /* precision: sorry, only 8-bit precision is supported right now */
    terminate(JPGD_BAD_PRECISION);

  m_image_y_size = get_bits_1(16);

  if ((m_image_y_size < 1) || (m_image_y_size > JPGD_MAX_HEIGHT))
    terminate(JPGD_BAD_HEIGHT);

  m_image_x_size = get_bits_1(16);

  if ((m_image_x_size < 1) || (m_image_x_size > JPGD_MAX_WIDTH))
    terminate(JPGD_BAD_WIDTH);

  m_comps_in_frame = get_bits_1(8);

  if (m_comps_in_frame > JPGD_MAXCOMPONENTS)
    terminate(JPGD_TOO_MANY_COMPONENTS);

  if (left != (u32)(m_comps_in_frame * 3 + 8))
    terminate(JPGD_BAD_SOF_LENGTH);

  for (i = 0; i < m_comps_in_frame; i++)
  {
    m_comp_ident[i]  = get_bits_1(8);
    m_comp_h_samp[i] = get_bits_1(4);
    m_comp_v_samp[i] = get_bits_1(4);
    m_comp_quant[i]  = get_bits_1(8);
  }
}
//------------------------------------------------------------------------------
// Used to skip unrecognized markers.
void jpeg_decoder::skip_variable_marker(void)
{
  u32 left;

  left = get_bits_1(16);

  if (left < 2)
    terminate(JPGD_BAD_VARIABLE_MARKER);

  left -= 2;

  while (left)
  {
    get_bits_1(8);
    left--;
  }
}
//------------------------------------------------------------------------------
// Read a define restart interval (DRI) marker.
void jpeg_decoder::read_dri_marker(void)
{
  if (get_bits_1(16) != 4)
    terminate(JPGD_BAD_DRI_LENGTH);

  m_restart_interval = get_bits_1(16);
}
//------------------------------------------------------------------------------
// Read a start of scan (SOS) marker.
void jpeg_decoder::read_sos_marker(void)
{
  u32 left;
  int i, ci, n, c, cc;

  left = get_bits_1(16);

  n = get_bits_1(8);

  m_comps_in_scan = n;

  left -= 3;

  if ( (left != (u32)(n * 2 + 3)) || (n < 1) || (n > JPGD_MAXCOMPSINSCAN) )
    terminate(JPGD_BAD_SOS_LENGTH);

  for (i = 0; i < n; i++)
  {
    cc = get_bits_1(8);
    c = get_bits_1(8);
    left -= 2;

    for (ci = 0; ci < m_comps_in_frame; ci++)
      if (cc == m_comp_ident[ci])
        break;

    if (ci >= m_comps_in_frame)
      terminate(JPGD_BAD_SOS_COMP_ID);

    m_comp_list[i]    = ci;
    m_comp_dc_tab[ci] = (c >> 4) & 15;
    m_comp_ac_tab[ci] = (c & 15) + (JPGD_MAXHUFFTABLES >> 1);
  }

  m_spectral_start  = get_bits_1(8);
  m_spectral_end    = get_bits_1(8);
  m_successive_high = get_bits_1(4);
  m_successive_low  = get_bits_1(4);

  if (!m_progressive_flag)
  {
    m_spectral_start = 0;
    m_spectral_end = 63;
  }

  left -= 3;

  while (left)                  /* read past whatever is left */
  {
    get_bits_1(8);
    left--;
  }
}
//------------------------------------------------------------------------------
// Finds the next marker.
int jpeg_decoder::next_marker(void)
{
  u32 c, bytes;

  bytes = 0;

  do
  {
    do
    {
      bytes++;

      c = get_bits_1(8);

    } while (c != 0xFF);

    do
    {
      c = get_bits_1(8);

    } while (c == 0xFF);

  } while (c == 0);

  // If bytes > 0 here, there where extra bytes before the marker (not good).

  return c;
}
//------------------------------------------------------------------------------
// Process markers. Returns when an SOFx, SOI, EOI, or SOS marker is
// encountered.
int jpeg_decoder::process_markers(void)
{
  int c;

  for ( ; ; )
  {
    c = next_marker();

    switch (c)
    {
      case M_SOF0:
      case M_SOF1:
      case M_SOF2:
      case M_SOF3:
      case M_SOF5:
      case M_SOF6:
      case M_SOF7:
//      case M_JPG:
      case M_SOF9:
      case M_SOF10:
      case M_SOF11:
      case M_SOF13:
      case M_SOF14:
      case M_SOF15:
      case M_SOI:
      case M_EOI:
      case M_SOS:
      {
        return c;
      }
      case M_DHT:
      {
        read_dht_marker();
        break;
      }
      // Sorry, no arithmitic support at this time. Dumb patents!
      case M_DAC:
      {
        terminate(JPGD_NO_ARITHMITIC_SUPPORT);
        break;
      }
      case M_DQT:
      {
        read_dqt_marker();
        break;
      }
      case M_DRI:
      {
        read_dri_marker();
        break;
      }
      //case M_APP0:  /* no need to read the JFIF marker */

      case M_JPG:
      case M_RST0:    /* no parameters */
      case M_RST1:
      case M_RST2:
      case M_RST3:
      case M_RST4:
      case M_RST5:
      case M_RST6:
      case M_RST7:
      case M_TEM:
      {
        terminate(JPGD_UNEXPECTED_MARKER);
        break;
      }
      default:    /* must be DNL, DHP, EXP, APPn, JPGn, COM, or RESn or APP0 */
      {
        skip_variable_marker();
        break;
      }

    }
  }
}
//------------------------------------------------------------------------------
// Finds the start of image (SOI) marker.
// This code is rather defensive: it only checks the first 512 bytes to avoid
// false positives.
void jpeg_decoder::locate_soi_marker(void)
{
  u32 lastchar, thischar;
  u32 bytesleft;

  lastchar = get_bits_1(8);

  thischar = get_bits_1(8);

  /* ok if it's a normal JPEG file without a special header */

  if ((lastchar == 0xFF) && (thischar == M_SOI))
    return;

  bytesleft = 4096; //512;

  for ( ; ; )
  {
    if (--bytesleft == 0)
      terminate(JPGD_NOT_JPEG);

    lastchar = thischar;

    thischar = get_bits_1(8);

    if (lastchar == 0xFF) 
    {
      if (thischar == M_SOI)
        break;
      else if (thischar == M_EOI) //get_bits_1 will keep returning M_EOI if we read past the end
        terminate(JPGD_NOT_JPEG);
    }
  }

  /* Check the next character after marker: if it's not 0xFF, it can't
     be the start of the next marker, so the file is bad */

  thischar = (m_bit_buf >> 24) & 0xFF;

  if (thischar != 0xFF)
    terminate(JPGD_NOT_JPEG);
}
//------------------------------------------------------------------------------
// Find a start of frame (SOF) marker.
void jpeg_decoder::locate_sof_marker(void)
{
  int c;

  locate_soi_marker();

  c = process_markers();

  switch (c)
  {
    case M_SOF2:
      m_progressive_flag = JPGD_TRUE;
    case M_SOF0:  /* baseline DCT */
    case M_SOF1:  /* extended sequential DCT */
    {
      read_sof_marker();
      break;
    }
    case M_SOF9:  /* Arithmitic coding */
    {
      terminate(JPGD_NO_ARITHMITIC_SUPPORT);
      break;
    }

    default:
    {
      terminate(JPGD_UNSUPPORTED_MARKER);
      break;
    }
  }
}
//------------------------------------------------------------------------------
// Find a start of scan (SOS) marker.
int jpeg_decoder::locate_sos_marker(void)
{
  int c;

  c = process_markers();

  if (c == M_EOI)
    return JPGD_FALSE;
  else if (c != M_SOS)
    terminate(JPGD_UNEXPECTED_MARKER);

  read_sos_marker();

  return JPGD_TRUE;
}
//------------------------------------------------------------------------------
// Reset everything to default/uninitialized state.
void jpeg_decoder::init(Pjpeg_decoder_stream m_pStream)
{
  m_error_code = 0;

  m_ready_flag = false;

  m_image_x_size = m_image_y_size = 0;

  this->m_pStream = m_pStream;

  m_progressive_flag = JPGD_FALSE;

  memset(m_huff_ac, 0, sizeof(m_huff_ac));
  memset(m_huff_num, 0, sizeof(m_huff_num));
  memset(m_huff_val, 0, sizeof(m_huff_val));
  memset(m_quant, 0, sizeof(m_quant));

  m_scan_type = 0;

  m_comps_in_frame = 0;

  memset(m_comp_h_samp, 0, sizeof(m_comp_h_samp));
  memset(m_comp_v_samp, 0, sizeof(m_comp_v_samp));
  memset(m_comp_quant, 0, sizeof(m_comp_quant));
  memset(m_comp_ident, 0, sizeof(m_comp_ident));
  memset(m_comp_h_blocks, 0, sizeof(m_comp_h_blocks));
  memset(m_comp_v_blocks, 0, sizeof(m_comp_v_blocks));

  m_comps_in_scan = 0;
  memset(m_comp_list, 0, sizeof(m_comp_list));
  memset(m_comp_dc_tab, 0, sizeof(m_comp_dc_tab));
  memset(m_comp_ac_tab, 0, sizeof(m_comp_ac_tab));

  m_spectral_start = 0;
  m_spectral_end = 0;
  m_successive_low = 0;
  m_successive_high = 0;

  m_max_mcu_x_size = 0;
  m_max_mcu_y_size = 0;

  m_blocks_per_mcu = 0;
  m_max_blocks_per_row = 0;
  m_mcus_per_row = 0;
  m_mcus_per_col = 0;

  m_expanded_blocks_per_component = 0;
  m_expanded_blocks_per_mcu = 0;
  m_expanded_blocks_per_row = 0;
  m_freq_domain_chroma_upsample = false;

  memset(m_mcu_org, 0, sizeof(m_mcu_org));

  m_total_lines_left = 0;
  m_mcu_lines_left = 0;

  m_real_dest_bytes_per_scan_line = 0;
  m_dest_bytes_per_scan_line = 0;
  m_dest_bytes_per_pixel = 0;

  memset(m_blocks, 0, sizeof(m_blocks));

  memset(m_pHuff_tabs, 0, sizeof(m_pHuff_tabs));

  memset(m_dc_coeffs, 0, sizeof(m_dc_coeffs));
  memset(m_ac_coeffs, 0, sizeof(m_ac_coeffs));
  memset(m_block_y_mcu, 0, sizeof(m_block_y_mcu));

  m_eob_run = 0;

  memset(m_block_y_mcu, 0, sizeof(m_block_y_mcu));

  m_pIn_buf_ofs = m_in_buf;
  m_in_buf_left = 0;
  m_eof_flag = false;
  m_tem_flag = 0;

  memset(m_padd_1, 0, sizeof(m_padd_1));
  memset(m_in_buf, 0, sizeof(m_in_buf));
  memset(m_padd_2, 0, sizeof(m_padd_2));

  m_restart_interval = 0;
  m_restarts_left    = 0;
  m_next_restart_num = 0;

  m_max_mcus_per_row = 0;
  m_max_blocks_per_mcu = 0;
  m_max_mcus_per_col = 0;

  memset(m_last_dc_val, 0, sizeof(m_last_dc_val));
  m_pMCU_coefficients = NULL;
  m_pSample_buf = NULL;

  m_total_bytes_read = 0;

  // Tell the stream we're going to use it.
  m_pStream->attach();
  
  // Ready the input buffer.
  prep_in_buffer();

  // Prime the bit buffer.
  m_bits_left = 16;
  m_bit_buf = 0;

  get_bits_1(16);
  get_bits_1(16);

  for (int i = 0; i < JPGD_MAXBLOCKSPERMCU; i++)
    m_mcu_block_max_zag[i] = 64;
}
//------------------------------------------------------------------------------
#define SCALEBITS 16
#define ONE_HALF  ((int) 1 << (SCALEBITS-1))
#define FIX(x)    ((int) ((x) * (1L<<SCALEBITS) + 0.5f))
//------------------------------------------------------------------------------
// Create a few tables that allow us to quickly convert YCbCr to RGB.
void jpeg_decoder::create_look_ups(void)
{
  for (int i = 0; i <= 255; i++)
  {
    int k = i - 128;
    m_crr[i] = ( FIX(1.40200f)  * k + ONE_HALF) >> SCALEBITS;
    m_cbb[i] = ( FIX(1.77200f)  * k + ONE_HALF) >> SCALEBITS;
    m_crg[i] = (-FIX(0.71414f)) * k;
    m_cbg[i] = (-FIX(0.34414f)) * k + ONE_HALF;
  }
}
//------------------------------------------------------------------------------
// This method throws back into the stream any bytes that where read
// into the bit buffer during initial marker scanning.
void jpeg_decoder::fix_in_buffer(void)
{
  /* In case any 0xFF's where pulled into the buffer during marker scanning */

  assert((m_bits_left & 7) == 0);

  if (m_bits_left == 16)
    stuff_char( (u8)(m_bit_buf & 0xFF));

  if (m_bits_left >= 8)  
    stuff_char( (u8)((m_bit_buf >> 8) & 0xFF));

  stuff_char((u8)((m_bit_buf >> 16) & 0xFF));
  stuff_char((u8)((m_bit_buf >> 24) & 0xFF));

  m_bits_left = 16;
  get_bits_2(16);
  get_bits_2(16);
}
//------------------------------------------------------------------------------
void jpeg_decoder::transform_mcu(int mcu_row)
{
  jpgd_block_t* Psrc_ptr = m_pMCU_coefficients;
  u8* Pdst_ptr = m_pSample_buf + mcu_row * m_blocks_per_mcu * 64;

  for (int mcu_block = 0; mcu_block < m_blocks_per_mcu; mcu_block++)
  {
    idct(Psrc_ptr, Pdst_ptr, m_mcu_block_max_zag[mcu_block]);
    Psrc_ptr += 64;
    Pdst_ptr += 64;
  }
}
//------------------------------------------------------------------------------
static u8 max_rc[64] = {
  17, 18, 34, 50, 50, 51, 52, 52, 52, 68, 84, 84, 84, 84, 85, 86, 86, 86, 86, 86,
  102, 118, 118, 118, 118, 118, 118, 119, 120, 120, 120, 120, 120, 120, 120, 136,
  136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
  136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136
};
//------------------------------------------------------------------------------
void jpeg_decoder::transform_mcu_expand(int mcu_row)
{
  jpgd_block_t* Psrc_ptr = m_pMCU_coefficients;
  u8* Pdst_ptr = m_pSample_buf + mcu_row * m_expanded_blocks_per_mcu * 64;

  // Y IDCT
	int mcu_block;
  for (mcu_block = 0; mcu_block < m_expanded_blocks_per_component; mcu_block++)
  {
    idct(Psrc_ptr, Pdst_ptr, m_mcu_block_max_zag[mcu_block]);
    Psrc_ptr += 64;
    Pdst_ptr += 64;
  }
  
  // Chroma IDCT, with upsampling
	jpgd_block_t temp_block[64];

  for (int i = 0; i < 2; i++)
  {
    DCT_Upsample::Matrix44 P, Q, R, S;
    
    assert(m_mcu_block_max_zag[mcu_block] >= 1);
    assert(m_mcu_block_max_zag[mcu_block] <= 64);
    
    switch (max_rc[m_mcu_block_max_zag[mcu_block++] - 1])
    {
    case 1*16+1:
      DCT_Upsample::P_Q<1, 1>::calc(P, Q, Psrc_ptr);
      DCT_Upsample::R_S<1, 1>::calc(R, S, Psrc_ptr);
      break;
    case 1*16+2:
      DCT_Upsample::P_Q<1, 2>::calc(P, Q, Psrc_ptr);
      DCT_Upsample::R_S<1, 2>::calc(R, S, Psrc_ptr);
      break;
    case 2*16+2:
      DCT_Upsample::P_Q<2, 2>::calc(P, Q, Psrc_ptr);
      DCT_Upsample::R_S<2, 2>::calc(R, S, Psrc_ptr);
      break;
    case 3*16+2:
      DCT_Upsample::P_Q<3, 2>::calc(P, Q, Psrc_ptr);
      DCT_Upsample::R_S<3, 2>::calc(R, S, Psrc_ptr);
      break;
    case 3*16+3:
      DCT_Upsample::P_Q<3, 3>::calc(P, Q, Psrc_ptr);
      DCT_Upsample::R_S<3, 3>::calc(R, S, Psrc_ptr);
      break;
    case 3*16+4:
      DCT_Upsample::P_Q<3, 4>::calc(P, Q, Psrc_ptr);
      DCT_Upsample::R_S<3, 4>::calc(R, S, Psrc_ptr);
      break;
    case 4*16+4:
      DCT_Upsample::P_Q<4, 4>::calc(P, Q, Psrc_ptr);
      DCT_Upsample::R_S<4, 4>::calc(R, S, Psrc_ptr);
      break;
    case 5*16+4:
      DCT_Upsample::P_Q<5, 4>::calc(P, Q, Psrc_ptr);
      DCT_Upsample::R_S<5, 4>::calc(R, S, Psrc_ptr);
      break;
    case 5*16+5:
      DCT_Upsample::P_Q<5, 5>::calc(P, Q, Psrc_ptr);
      DCT_Upsample::R_S<5, 5>::calc(R, S, Psrc_ptr);
      break;
    case 5*16+6:
      DCT_Upsample::P_Q<5, 6>::calc(P, Q, Psrc_ptr);
      DCT_Upsample::R_S<5, 6>::calc(R, S, Psrc_ptr);
      break;
    case 6*16+6:
      DCT_Upsample::P_Q<6, 6>::calc(P, Q, Psrc_ptr);
      DCT_Upsample::R_S<6, 6>::calc(R, S, Psrc_ptr);
      break;
    case 7*16+6:
      DCT_Upsample::P_Q<7, 6>::calc(P, Q, Psrc_ptr);
      DCT_Upsample::R_S<7, 6>::calc(R, S, Psrc_ptr);
      break;
    case 7*16+7:
      DCT_Upsample::P_Q<7, 7>::calc(P, Q, Psrc_ptr);
      DCT_Upsample::R_S<7, 7>::calc(R, S, Psrc_ptr);
      break;
    case 7*16+8:
      DCT_Upsample::P_Q<7, 8>::calc(P, Q, Psrc_ptr);
      DCT_Upsample::R_S<7, 8>::calc(R, S, Psrc_ptr);
      break;
    case 8*16+8:
      DCT_Upsample::P_Q<8, 8>::calc(P, Q, Psrc_ptr);
      DCT_Upsample::R_S<8, 8>::calc(R, S, Psrc_ptr);
      break;
    default:
      assert(false);      
    }
  
    DCT_Upsample::Matrix44 a(P + Q); P -= Q;
    DCT_Upsample::Matrix44& b = P; 
    DCT_Upsample::Matrix44 c(R + S); R -= S;
    DCT_Upsample::Matrix44& d = R; 
    
    DCT_Upsample::Matrix44::add_and_store(temp_block, a, c);
    idct_4x4(temp_block, Pdst_ptr);
    Pdst_ptr += 64;                   

    DCT_Upsample::Matrix44::sub_and_store(temp_block, a, c);
    idct_4x4(temp_block, Pdst_ptr);
    Pdst_ptr += 64;                   

    DCT_Upsample::Matrix44::add_and_store(temp_block, b, d);
    idct_4x4(temp_block, Pdst_ptr);
    Pdst_ptr += 64;                   

    DCT_Upsample::Matrix44::sub_and_store(temp_block, b, d);
    idct_4x4(temp_block, Pdst_ptr);
    Pdst_ptr += 64;                   

    Psrc_ptr += 64;
  }
}
//------------------------------------------------------------------------------
// Loads and dequantizes the next row of (already decoded) coefficients.
// Progressive images only.
void jpeg_decoder::load_next_row(void)
{
  int i;
  jpgd_block_t *p;
  jpgd_quant_t *q;
  int mcu_row, mcu_block, row_block = 0;
  int component_num, component_id;
  int block_x_mcu[JPGD_MAXCOMPONENTS];

  memset(block_x_mcu, 0, JPGD_MAXCOMPONENTS * sizeof(int));

  for (mcu_row = 0; mcu_row < m_mcus_per_row; mcu_row++)
  {
    int block_x_mcu_ofs = 0, block_y_mcu_ofs = 0;

    for (mcu_block = 0; mcu_block < m_blocks_per_mcu; mcu_block++)
    {
      component_id = m_mcu_org[mcu_block];
      q = m_quant[m_comp_quant[component_id]];

      p = m_pMCU_coefficients + 64 * mcu_block;
      
      jpgd_block_t* pAC = coeff_buf_getp(m_ac_coeffs[component_id],
                                  block_x_mcu[component_id] + block_x_mcu_ofs,
                                  m_block_y_mcu[component_id] + block_y_mcu_ofs);

      jpgd_block_t* pDC = coeff_buf_getp(m_dc_coeffs[component_id],
                                  block_x_mcu[component_id] + block_x_mcu_ofs,
                                  m_block_y_mcu[component_id] + block_y_mcu_ofs);
      p[0] = pDC[0];
      memcpy(&p[1], &pAC[1], 63 * sizeof(jpgd_block_t));

      for (i = 63; i > 0; i--)
        if (p[g_ZAG[i]])
          break;

      m_mcu_block_max_zag[mcu_block] = i + 1;

      for ( ; i >= 0; i--)
				if (p[g_ZAG[i]])
					p[g_ZAG[i]] = static_cast<jpgd_block_t>(p[g_ZAG[i]] * q[i]);

      row_block++;

      if (m_comps_in_scan == 1)
        block_x_mcu[component_id]++;
      else
      {
        if (++block_x_mcu_ofs == m_comp_h_samp[component_id])
        {
          block_x_mcu_ofs = 0;

          if (++block_y_mcu_ofs == m_comp_v_samp[component_id])
          {
            block_y_mcu_ofs = 0;

            block_x_mcu[component_id] += m_comp_h_samp[component_id];
          }
        }
      }
    }

    if (m_freq_domain_chroma_upsample)
      transform_mcu_expand(mcu_row);
    else
      transform_mcu(mcu_row);
  }

  if (m_comps_in_scan == 1)
    m_block_y_mcu[m_comp_list[0]]++;
  else
  {
    for (component_num = 0; component_num < m_comps_in_scan; component_num++)
    {
      component_id = m_comp_list[component_num];

      m_block_y_mcu[component_id] += m_comp_v_samp[component_id];
    }
  }
}
//------------------------------------------------------------------------------
// Restart interval processing.
void jpeg_decoder::process_restart(void)
{
  int i;
  int c = 0;

  // Align to a byte boundry
  // FIXME: Is this really necessary? get_bits_2() never reads in markers!
  //get_bits_2(m_bits_left & 7);

  // Let's scan a little bit to find the marker, but not _too_ far.
  // 1536 is a "fudge factor" that determines how much to scan.
  for (i = 1536; i > 0; i--)
    if (get_char() == 0xFF)
      break;

  if (i == 0)
    terminate(JPGD_BAD_RESTART_MARKER);

  for ( ; i > 0; i--)
    if ((c = get_char()) != 0xFF)
      break;

  if (i == 0)
    terminate(JPGD_BAD_RESTART_MARKER);

  // Is it the expected marker? If not, something bad happened.
  if (c != (m_next_restart_num + M_RST0))
    terminate(JPGD_BAD_RESTART_MARKER);

  // Reset each component's DC prediction values.
  memset(&m_last_dc_val, 0, m_comps_in_frame * sizeof(u32));

  m_eob_run = 0;

  m_restarts_left = m_restart_interval;

  m_next_restart_num = (m_next_restart_num + 1) & 7;

  // Get the bit buffer going again...

  m_bits_left = 16;
  get_bits_2(16);
  get_bits_2(16);
}
//------------------------------------------------------------------------------
namespace 
{
	inline int dequantize_ac(int c, int q)
	{
		c *= q;

#if JPGD_APPROX_LAPLACIAN_AC_DEQUANT
		{
			if (c < 0)
			{
				c += (q * q + 64) >> 7;
				if (c > 0) c = 0;
			}
			else
			{
				c -= (q * q + 64) >> 7;
				if (c < 0) c = 0;
			}
		}
#endif

		return c;
	}
}
//------------------------------------------------------------------------------
// Decodes and dequantizes the next row of coefficients.
void jpeg_decoder::decode_next_row(void)
{
  int row_block = 0;

  for (int mcu_row = 0; mcu_row < m_mcus_per_row; mcu_row++)
  {
    if ((m_restart_interval) && (m_restarts_left == 0))
      process_restart();

    jpgd_block_t* p = m_pMCU_coefficients;
    for (int mcu_block = 0; mcu_block < m_blocks_per_mcu; mcu_block++, p += 64)
    {
      int component_id = m_mcu_org[mcu_block];
      jpgd_quant_t* q = m_quant[m_comp_quant[component_id]];  

      int r, s;
      s = huff_decode(m_pHuff_tabs[m_comp_dc_tab[component_id]], r);
      s = HUFF_EXTEND(r, s);

      m_last_dc_val[component_id] = (s += m_last_dc_val[component_id]);

      p[0] = static_cast<jpgd_block_t>(s * q[0]);

      int prev_num_set = m_mcu_block_max_zag[mcu_block];

      Phuff_tables_t Ph = m_pHuff_tabs[m_comp_ac_tab[component_id]];

      int k;
      for (k = 1; k < 64; k++)
      {
        int extra_bits;
        s = huff_decode(Ph, extra_bits);

        r = s >> 4;
        s &= 15;

        if (s)
        {
          if (r)
          {
            if ((k + r) > 63)
              terminate(JPGD_DECODE_ERROR);

            if (k < prev_num_set)
            {
              int n = jpgd_min(r, prev_num_set - k);
              int kt = k;
              while (n--)
                p[g_ZAG[kt++]] = 0;
            }

            k += r;
          }

          s = HUFF_EXTEND(extra_bits, s);

          assert(k < 64);

          p[g_ZAG[k]] = static_cast<jpgd_block_t>(dequantize_ac(s, q[k])); //s * q[k];
        }
        else
        {
          if (r == 15)
          {
            if ((k + 16) > 64)
              terminate(JPGD_DECODE_ERROR);

            if (k < prev_num_set)
            {
              int n = jpgd_min(16, prev_num_set - k);    // Dec. 19, 2001 - bugfix! was 15
              int kt = k;
              while (n--)
              {
                assert(kt <= 63);
                p[g_ZAG[kt++]] = 0;
              }
            }

            k += 16 - 1; // - 1 because the loop counter is k!
            assert(p[g_ZAG[k]] == 0);
          }
          else
            break;
        }
      }

      if (k < prev_num_set)
      {
        int kt = k;
        while (kt < prev_num_set)
          p[g_ZAG[kt++]] = 0;
      }

      m_mcu_block_max_zag[mcu_block] = k;

      row_block++;
    }

    if (m_freq_domain_chroma_upsample)
      transform_mcu_expand(mcu_row);
    else
      transform_mcu(mcu_row);

    m_restarts_left--;
  }
}
//------------------------------------------------------------------------------
// YCbCr H1V1 (1x1:1:1, 3 m_blocks per MCU) to 24-bit RGB
void jpeg_decoder::H1V1Convert(void)
{
  int row = m_max_mcu_y_size - m_mcu_lines_left;
  u8 *d = m_pScan_line_0;
  u8 *s = m_pSample_buf + row * 8;

  for (int i = m_max_mcus_per_row; i > 0; i--)
  {
    for (int j = 0; j < 8; j++)
    {
      int y = s[j];
      int cb = s[64+j];
      int cr = s[128+j];

      d[0] = clamp(y + m_crr[cr]);
      d[1] = clamp(y + ((m_crg[cr] + m_cbg[cb]) >> 16));
      d[2] = clamp(y + m_cbb[cb]);
      d[3] = 255;

      d += 4;
    }

    s += 64*3;
  }
}
//------------------------------------------------------------------------------
// YCbCr H2V1 (2x1:1:1, 4 m_blocks per MCU) to 24-bit RGB
void jpeg_decoder::H2V1Convert(void)
{
  int row = m_max_mcu_y_size - m_mcu_lines_left;
  u8 *d0 = m_pScan_line_0;
  u8 *y = m_pSample_buf + row * 8;
  u8 *c = m_pSample_buf + 2*64 + row * 8;

  for (int i = m_max_mcus_per_row; i > 0; i--)
  {
    for (int l = 0; l < 2; l++)
    {
      for (int j = 0; j < 4; j++)
      {
        int cb = c[0];
        int cr = c[64];

        int rc = m_crr[cr];
        int gc = ((m_crg[cr] + m_cbg[cb]) >> 16);
        int bc = m_cbb[cb];

        int yy = y[j<<1];
        d0[0] = clamp(yy+rc);
        d0[1] = clamp(yy+gc);
        d0[2] = clamp(yy+bc);
        d0[3] = 255;

        yy = y[(j<<1)+1];
        d0[4] = clamp(yy+rc);
        d0[5] = clamp(yy+gc);
        d0[6] = clamp(yy+bc);
        d0[7] = 255;

        d0 += 8;

        c++;
      }
      y += 64;
    }

    y += 64*4 - 64*2;
    c += 64*4 - 8;
  }
}
//------------------------------------------------------------------------------
// YCbCr H2V1 (1x2:1:1, 4 m_blocks per MCU) to 24-bit RGB
void jpeg_decoder::H1V2Convert(void)
{
  int row = m_max_mcu_y_size - m_mcu_lines_left;
  u8 *d0 = m_pScan_line_0;
  u8 *d1 = m_pScan_line_1;
  u8 *y;
  u8 *c;

  if (row < 8)
    y = m_pSample_buf + row * 8;
  else
    y = m_pSample_buf + 64*1 + (row & 7) * 8;

  c = m_pSample_buf + 64*2 + (row >> 1) * 8;

  for (int i = m_max_mcus_per_row; i > 0; i--)
  {
    for (int j = 0; j < 8; j++)
    {
      int cb = c[0+j];
      int cr = c[64+j];

      int rc = m_crr[cr];
      int gc = ((m_crg[cr] + m_cbg[cb]) >> 16);
      int bc = m_cbb[cb];

      int yy = y[j];
      d0[0] = clamp(yy+rc);
      d0[1] = clamp(yy+gc);
      d0[2] = clamp(yy+bc);
      d0[3] = 255;

      yy = y[8+j];
      d1[0] = clamp(yy+rc);
      d1[1] = clamp(yy+gc);
      d1[2] = clamp(yy+bc);
      d1[3] = 255;

      d0 += 4;
      d1 += 4;
    }

    y += 64*4;
    c += 64*4;
  }
}
//------------------------------------------------------------------------------
// YCbCr H2V2 (2x2:1:1, 6 m_blocks per MCU) to 24-bit RGB
void jpeg_decoder::H2V2Convert(void)
{
	int row = m_max_mcu_y_size - m_mcu_lines_left;
	u8 *d0 = m_pScan_line_0;
	u8 *d1 = m_pScan_line_1;
	u8 *y;
	u8 *c;

	if (row < 8)
		y = m_pSample_buf + row * 8;
	else
		y = m_pSample_buf + 64*2 + (row & 7) * 8;

	c = m_pSample_buf + 64*4 + (row >> 1) * 8;

	for (int i = m_max_mcus_per_row; i > 0; i--)
	{
		for (int l = 0; l < 2; l++)
		{
			for (int j = 0; j < 8; j += 2)
			{
				int cb = c[0];
				int cr = c[64];

				int rc = m_crr[cr];
				int gc = ((m_crg[cr] + m_cbg[cb]) >> 16);
				int bc = m_cbb[cb];

				int yy = y[j];
				d0[0] = clamp(yy+rc);
				d0[1] = clamp(yy+gc);
				d0[2] = clamp(yy+bc);
				d0[3] = 255;

				yy = y[j+1];
				d0[4] = clamp(yy+rc);
				d0[5] = clamp(yy+gc);
				d0[6] = clamp(yy+bc);
				d0[7] = 255;

				yy = y[j+8];
				d1[0] = clamp(yy+rc);
				d1[1] = clamp(yy+gc);
				d1[2] = clamp(yy+bc);
				d1[3] = 255;

				yy = y[j+8+1];
				d1[4] = clamp(yy+rc);
				d1[5] = clamp(yy+gc);
				d1[6] = clamp(yy+bc);
				d1[7] = 255;

				d0 += 8;
				d1 += 8;

				c++;
			}
			y += 64;
		}

		y += 64*6 - 64*2;
		c += 64*6 - 8;
	}
}
//------------------------------------------------------------------------------
// Y (1 block per MCU) to 8-bit greyscale
void jpeg_decoder::grey_convert(void)
{
  int row = m_max_mcu_y_size - m_mcu_lines_left;
  u8 *d = m_pScan_line_0;
  u8 *s = m_pSample_buf + row * 8;

  for (int i = m_max_mcus_per_row; i > 0; i--)
  {
    *(u32 *)d = *(u32 *)s;
    *(u32 *)(&d[4]) = *(u32 *)(&s[4]);

    s += 64;
    d += 8;
  }
}
//------------------------------------------------------------------------------
void jpeg_decoder::expanded_convert(void)
{
  int row = m_max_mcu_y_size - m_mcu_lines_left;
    
  u8* Py = m_pSample_buf + (row / 8) * 64 * m_comp_h_samp[0] + (row & 7) * 8;
  
  u8* d = m_pScan_line_0;

  for (int i = m_max_mcus_per_row; i > 0; i--)
  {
    for (int k = 0; k < m_max_mcu_x_size; k += 8)
    {
      const int Y_ofs = k * 8;
      const int Cb_ofs = Y_ofs + 64 * m_expanded_blocks_per_component;
      const int Cr_ofs = Y_ofs + 64 * m_expanded_blocks_per_component * 2;
      for (int j = 0; j < 8; j++)
      {
        int y = Py[Y_ofs + j];
        int cb = Py[Cb_ofs + j];
        int cr = Py[Cr_ofs + j];

        d[0] = clamp(y + m_crr[cr]);
        d[1] = clamp(y + ((m_crg[cr] + m_cbg[cb]) >> 16));
        d[2] = clamp(y + m_cbb[cb]);
        d[3] = 255;

        d += 4;
      }
    }

    Py += 64 * m_expanded_blocks_per_mcu;
  }
}
//------------------------------------------------------------------------------
// Find end of image (EOI) marker, so we can return to the user the
// exact size of the input stream.
void jpeg_decoder::find_eoi(void)
{
  if (!m_progressive_flag)
  {
    // Attempt to read the EOI marker.
    //get_bits_2(m_bits_left & 7);

    // Prime the bit buffer
    m_bits_left = 16;
    get_bits_1(16);
    get_bits_1(16);

    // The next marker _should_ be EOI
    process_markers();
  }

  m_total_bytes_read -= m_in_buf_left;
}
//------------------------------------------------------------------------------
// Returns the next scan line.
// Returns JPGD_DONE if all scan lines have been returned.
// Returns JPGD_OKAY if a scan line has been returned.
// Returns JPGD_FAILED if an error occured.
int jpeg_decoder::decode(
  const void** Pscan_line_ofs, u32* Pscan_line_len)
{
  if ((m_error_code) || (!m_ready_flag))
    return (JPGD_FAILED);

  if (m_total_lines_left == 0)
    return (JPGD_DONE);

  if (m_mcu_lines_left == 0)
  {
    if (setjmp(m_jmp_state))
      return (JPGD_DECODE_ERROR);

    if (m_progressive_flag)
      load_next_row();
    else
      decode_next_row();

    // Find the EOI marker if that was the last row.
    if (m_total_lines_left <= m_max_mcu_y_size)
      find_eoi();

    m_mcu_lines_left = m_max_mcu_y_size;
  }

  if (m_freq_domain_chroma_upsample)
  {
    expanded_convert();
    *Pscan_line_ofs = m_pScan_line_0;
  }
  else
  {
    switch (m_scan_type)
    {
      case JPGD_YH2V2:
      {
        if ((m_mcu_lines_left & 1) == 0)
        {
          H2V2Convert();
          *Pscan_line_ofs = m_pScan_line_0;
        }
        else
          *Pscan_line_ofs = m_pScan_line_1;

        break;
      }
      case JPGD_YH2V1:
      {
        H2V1Convert();
        *Pscan_line_ofs = m_pScan_line_0;
        break;
      }
      case JPGD_YH1V2:
      {
        if ((m_mcu_lines_left & 1) == 0)
        {
          H1V2Convert();
          *Pscan_line_ofs = m_pScan_line_0;
        }
        else
          *Pscan_line_ofs = m_pScan_line_1;

        break;
      }
      case JPGD_YH1V1:
      {
        H1V1Convert();
        *Pscan_line_ofs = m_pScan_line_0;
        break;
      }
      case JPGD_GRAYSCALE:
      {
        grey_convert();
        *Pscan_line_ofs = m_pScan_line_0;

        break;
      }
    }
  }

  *Pscan_line_len = m_real_dest_bytes_per_scan_line;

  m_mcu_lines_left--;
  m_total_lines_left--;

  return (JPGD_OKAY);
}
//------------------------------------------------------------------------------
// Creates the tables needed for efficient Huffman decoding.
void jpeg_decoder::make_huff_table(
  int index,
  Phuff_tables_t hs)
{
  int p, i, l, si;
  u8 huffsize[257];
  u32 huffcode[257];
  u32 code;
  u32 subtree;
  int code_size;
  int lastp;
  int nextfreeentry;
  int currententry;
  
  hs->ac_table = m_huff_ac[index] != 0;

  p = 0;

  for (l = 1; l <= 16; l++)
  {
    for (i = 1; i <= m_huff_num[index][l]; i++)
      huffsize[p++] = static_cast<u8>(l);
  }

  huffsize[p] = 0;

  lastp = p;

  code = 0;
  si = huffsize[0];
  p = 0;

  while (huffsize[p])
  {
    while (huffsize[p] == si)
    {
      huffcode[p++] = code;
      code++;
    }

    code <<= 1;
    si++;
  }

  memset(hs->look_up, 0, sizeof(hs->look_up));
  memset(hs->look_up2, 0, sizeof(hs->look_up2));
  memset(hs->tree, 0, sizeof(hs->tree));
  memset(hs->code_size, 0, sizeof(hs->code_size));

  nextfreeentry = -1;

  p = 0;

  while (p < lastp)
  {
    i = m_huff_val[index][p];
    code = huffcode[p];
    code_size = huffsize[p];

    hs->code_size[i] = static_cast<u8>(code_size);

    if (code_size <= 8)
    {
      code <<= (8 - code_size);

      for (l = 1 << (8 - code_size); l > 0; l--)
      {
        assert(i < 256);

        hs->look_up[code] = i;

        bool has_extrabits = false;
				int extra_bits = 0;
        int num_extra_bits = i & 15;

        int bits_to_fetch = code_size;
        if (num_extra_bits)
        {
          int total_codesize = code_size + num_extra_bits;
          if (total_codesize <= 8)
          {
            has_extrabits = true;
            extra_bits = ((1 << num_extra_bits) - 1) & (code >> (8 - total_codesize));
            assert(extra_bits <= 0x7FFF);
            bits_to_fetch += num_extra_bits;
          }
        }

        if (!has_extrabits)
          hs->look_up2[code] = i | (bits_to_fetch << 8);
        else
          hs->look_up2[code] = i | 0x8000 | (extra_bits << 16) | (bits_to_fetch << 8);
        
        code++;
      }
    }
    else
    {
      subtree = (code >> (code_size - 8)) & 0xFF;

      currententry = hs->look_up[subtree];

      if (currententry == 0)
      {
        hs->look_up[subtree] = currententry = nextfreeentry;
        hs->look_up2[subtree] = currententry = nextfreeentry;

        nextfreeentry -= 2;
      }

      code <<= (16 - (code_size - 8));

      for (l = code_size; l > 9; l--)
      {
        if ((code & 0x8000) == 0)
          currententry--;

        if (hs->tree[-currententry - 1] == 0)
        {
          hs->tree[-currententry - 1] = nextfreeentry;

          currententry = nextfreeentry;

          nextfreeentry -= 2;
        }
        else
          currententry = hs->tree[-currententry - 1];

        code <<= 1;
      }

      if ((code & 0x8000) == 0)
        currententry--;

      hs->tree[-currententry - 1] = i;
    }

    p++;
  }
}
//------------------------------------------------------------------------------
// Verifies the quantization tables needed for this scan are available.
void jpeg_decoder::check_quant_tables(void)
{
  int i;

  for (i = 0; i < m_comps_in_scan; i++)
    if (m_quant[m_comp_quant[m_comp_list[i]]] == NULL)
      terminate(JPGD_UNDEFINED_QUANT_TABLE);
}
//------------------------------------------------------------------------------
// Verifies that all the Huffman tables needed for this scan are available.
void jpeg_decoder::check_huff_tables(void)
{
  int i;

  for (i = 0; i < m_comps_in_scan; i++)
  {
    if ((m_spectral_start == 0) && (m_huff_num[m_comp_dc_tab[m_comp_list[i]]] == NULL))
      terminate(JPGD_UNDEFINED_HUFF_TABLE);

    if ((m_spectral_end > 0) && (m_huff_num[m_comp_ac_tab[m_comp_list[i]]] == NULL))
      terminate(JPGD_UNDEFINED_HUFF_TABLE);
  }

  for (i = 0; i < JPGD_MAXHUFFTABLES; i++)
    if (m_huff_num[i])
    {
      if (!m_pHuff_tabs[i])
        m_pHuff_tabs[i] = (Phuff_tables_t)alloc(sizeof(huff_tables_t));

      make_huff_table(i, m_pHuff_tabs[i]);
    }
}
//------------------------------------------------------------------------------
// Determines the component order inside each MCU.
// Also calcs how many MCU's are on each row, etc.
void jpeg_decoder::calc_mcu_block_order(void)
{
  int component_num, component_id;
  int max_h_samp = 0, max_v_samp = 0;

  for (component_id = 0; component_id < m_comps_in_frame; component_id++)
  {
    if (m_comp_h_samp[component_id] > max_h_samp)
      max_h_samp = m_comp_h_samp[component_id];

    if (m_comp_v_samp[component_id] > max_v_samp)
      max_v_samp = m_comp_v_samp[component_id];
  }

  for (component_id = 0; component_id < m_comps_in_frame; component_id++)
  {
    m_comp_h_blocks[component_id] = ((((m_image_x_size * m_comp_h_samp[component_id]) + (max_h_samp - 1)) / max_h_samp) + 7) / 8;
    m_comp_v_blocks[component_id] = ((((m_image_y_size * m_comp_v_samp[component_id]) + (max_v_samp - 1)) / max_v_samp) + 7) / 8;
  }

  if (m_comps_in_scan == 1)
  {
    m_mcus_per_row = m_comp_h_blocks[m_comp_list[0]];
    m_mcus_per_col = m_comp_v_blocks[m_comp_list[0]];
  }
  else
  {
    m_mcus_per_row = (((m_image_x_size + 7) / 8) + (max_h_samp - 1)) / max_h_samp;
    m_mcus_per_col = (((m_image_y_size + 7) / 8) + (max_v_samp - 1)) / max_v_samp;
  }

  if (m_comps_in_scan == 1)
  {
    m_mcu_org[0] = m_comp_list[0];

    m_blocks_per_mcu = 1;
  }
  else
  {
    m_blocks_per_mcu = 0;

    for (component_num = 0; component_num < m_comps_in_scan; component_num++)
    {
      int num_blocks;

      component_id = m_comp_list[component_num];

      num_blocks = m_comp_h_samp[component_id] * m_comp_v_samp[component_id];

      while (num_blocks--)
        m_mcu_org[m_blocks_per_mcu++] = component_id;
    }
  }
}
//------------------------------------------------------------------------------
// Starts a new scan.
int jpeg_decoder::init_scan(void)
{
  if (!locate_sos_marker())
    return JPGD_FALSE;

  calc_mcu_block_order();

  check_huff_tables();

  check_quant_tables();

  memset(m_last_dc_val, 0, m_comps_in_frame * sizeof(u32));

  m_eob_run = 0;

  if (m_restart_interval)
  {
    m_restarts_left = m_restart_interval;
    m_next_restart_num = 0;
  }

  fix_in_buffer();

  return JPGD_TRUE;
}
//------------------------------------------------------------------------------
void jpeg_decoder::init_quant_tables(void)
{
}
//------------------------------------------------------------------------------
// Starts a frame. Determines if the number of components or sampling factors
// are supported.
void jpeg_decoder::init_frame(void)
{
  int i;
  u8 *q;

  if (m_comps_in_frame == 1)
  {
    if ((m_comp_h_samp[0] != 1) || (m_comp_v_samp[0] != 1))
      terminate(JPGD_UNSUPPORTED_SAMP_FACTORS);
    
    m_scan_type          = JPGD_GRAYSCALE;

    m_max_blocks_per_mcu = 1;

    m_max_mcu_x_size     = 8;
    m_max_mcu_y_size     = 8;
  }
  else if (m_comps_in_frame == 3)
  {
    if ( ((m_comp_h_samp[1] != 1) || (m_comp_v_samp[1] != 1)) ||
         ((m_comp_h_samp[2] != 1) || (m_comp_v_samp[2] != 1)) )
      terminate(JPGD_UNSUPPORTED_SAMP_FACTORS);

    if ((m_comp_h_samp[0] == 1) && (m_comp_v_samp[0] == 1))
    {
      m_scan_type          = JPGD_YH1V1;

      m_max_blocks_per_mcu = 3;
      
      m_max_mcu_x_size     = 8;
      m_max_mcu_y_size     = 8;
    }
    else if ((m_comp_h_samp[0] == 2) && (m_comp_v_samp[0] == 1))
    {
      m_scan_type          = JPGD_YH2V1;

      m_max_blocks_per_mcu = 4;
      
      m_max_mcu_x_size     = 16;
      m_max_mcu_y_size     = 8;
    }
    else if ((m_comp_h_samp[0] == 1) && (m_comp_v_samp[0] == 2))
    {
      m_scan_type          = JPGD_YH1V2;

      m_max_blocks_per_mcu = 4;
      
      m_max_mcu_x_size     = 8;
      m_max_mcu_y_size     = 16;
    }
    else if ((m_comp_h_samp[0] == 2) && (m_comp_v_samp[0] == 2))
    {
      m_scan_type          = JPGD_YH2V2;

      m_max_blocks_per_mcu = 6;
      
      m_max_mcu_x_size     = 16;
      m_max_mcu_y_size     = 16;
    }
    else
      terminate(JPGD_UNSUPPORTED_SAMP_FACTORS);
  }
  else
    terminate(JPGD_UNSUPPORTED_COLORSPACE);

  m_max_mcus_per_row = (m_image_x_size + (m_max_mcu_x_size - 1)) / m_max_mcu_x_size;
  m_max_mcus_per_col = (m_image_y_size + (m_max_mcu_y_size - 1)) / m_max_mcu_y_size;

  /* these values are for the *destination* pixels: after conversion */

  if (m_scan_type == JPGD_GRAYSCALE)
    m_dest_bytes_per_pixel = 1;
  else
    m_dest_bytes_per_pixel = 4;

  m_dest_bytes_per_scan_line = ((m_image_x_size + 15) & 0xFFF0) * m_dest_bytes_per_pixel;

  m_real_dest_bytes_per_scan_line = (m_image_x_size * m_dest_bytes_per_pixel);

  // Initialize two scan line buffers.
  // FIXME: Only the V2 sampling factors need two buffers.
  m_pScan_line_0         = (u8 *)alloc(m_dest_bytes_per_scan_line + 8);
  memset(m_pScan_line_0, 0, m_dest_bytes_per_scan_line);

  m_pScan_line_1         = (u8 *)alloc(m_dest_bytes_per_scan_line + 8);
  memset(m_pScan_line_1, 0, m_dest_bytes_per_scan_line);

  m_max_blocks_per_row = m_max_mcus_per_row * m_max_blocks_per_mcu;

  // Should never happen
  if (m_max_blocks_per_row > JPGD_MAXBLOCKSPERROW)
    terminate(JPGD_ASSERTION_ERROR);

  // Allocate the coefficient buffer, enough for one MCU
  q = (u8 *)alloc(m_max_blocks_per_mcu * 64 * sizeof(jpgd_block_t) + 8);

  // Align to 8-byte boundry, for MMX code (old comment - mmx code removed 11/26/10)
  q = (u8 *)(((u32)q + 7) & ~7);

  m_pMCU_coefficients = (jpgd_block_t*)q;

  for (i = 0; i < m_max_blocks_per_mcu; i++)
    m_mcu_block_max_zag[i] = 64;

  m_expanded_blocks_per_component = m_comp_h_samp[0] * m_comp_v_samp[0];
  m_expanded_blocks_per_mcu = m_expanded_blocks_per_component * m_comps_in_frame;
  m_expanded_blocks_per_row = m_max_mcus_per_row * m_expanded_blocks_per_mcu;
	// Freq. domain chroma upsampling only supported for H2V2 subsampling factor.
  m_freq_domain_chroma_upsample = (JPGD_SUPPORT_FREQ_DOMAIN_UPSAMPLING != 0) && (m_expanded_blocks_per_mcu == 4*3);
  	    
  if (m_freq_domain_chroma_upsample)
    m_pSample_buf = (u8 *)(((u32)alloc(m_expanded_blocks_per_row * 64 + 8) + 7) & ~7);
  else
    m_pSample_buf = (u8 *)(((u32)alloc(m_max_blocks_per_row * 64 + 8) + 7) & ~7);
  
  m_total_lines_left = m_image_y_size;

  m_mcu_lines_left = 0;

  create_look_ups();

  init_quant_tables();
}
//------------------------------------------------------------------------------
// The coeff_buf series of methods originally stored the coefficients
// into a "virtual" file which was located in EMS, XMS, or a disk file. A cache
// was used to make this process more efficient. Now, we can store the entire
// thing in RAM.
Pcoeff_buf_t jpeg_decoder::coeff_buf_open(
  int block_num_x, int block_num_y,
  int block_len_x, int block_len_y)
{
  Pcoeff_buf_t cb = (Pcoeff_buf_t)alloc(sizeof(coeff_buf_t));

  cb->block_num_x = block_num_x;
  cb->block_num_y = block_num_y;

  cb->block_len_x = block_len_x;
  cb->block_len_y = block_len_y;

  cb->block_size = (block_len_x * block_len_y) * sizeof(jpgd_block_t);

  cb->Pdata = (u8 *)alloc(cb->block_size * block_num_x * block_num_y);

  return cb;
}
//------------------------------------------------------------------------------
jpgd_block_t *jpeg_decoder::coeff_buf_getp(
  Pcoeff_buf_t cb,
  int block_x, int block_y)
{
  if (block_x >= cb->block_num_x)
    terminate(JPGD_ASSERTION_ERROR);

  if (block_y >= cb->block_num_y)
    terminate(JPGD_ASSERTION_ERROR);

  return (jpgd_block_t *)(cb->Pdata + block_x * cb->block_size + block_y * (cb->block_size * cb->block_num_x));
}
//------------------------------------------------------------------------------
// The following methods decode the various types of m_blocks encountered
// in progressively encoded images.
void progressive_block_decoder::decode_block_dc_first(
  jpeg_decoder *Pd,
  int component_id, int block_x, int block_y)
{
  int s, r;
  jpgd_block_t *p = Pd->coeff_buf_getp(Pd->m_dc_coeffs[component_id], block_x, block_y);

  if ((s = Pd->huff_decode(Pd->m_pHuff_tabs[Pd->m_comp_dc_tab[component_id]])) != 0)
  {
    r = Pd->get_bits_2(s);
    s = HUFF_EXTEND_P(r, s);
  }

  Pd->m_last_dc_val[component_id] = (s += Pd->m_last_dc_val[component_id]);

  p[0] = static_cast<jpgd_block_t>(s << Pd->m_successive_low);
}
//------------------------------------------------------------------------------
void progressive_block_decoder::decode_block_dc_refine(
  jpeg_decoder *Pd,
  int component_id, int block_x, int block_y)
{
  if (Pd->get_bits_2(1))
  {
    jpgd_block_t *p = Pd->coeff_buf_getp(Pd->m_dc_coeffs[component_id], block_x, block_y);

    p[0] |= (1 << Pd->m_successive_low);
  }
}
//------------------------------------------------------------------------------
void progressive_block_decoder::decode_block_ac_first(
  jpeg_decoder *Pd,
  int component_id, int block_x, int block_y)
{
  int k, s, r;

  if (Pd->m_eob_run)
  {
    Pd->m_eob_run--;
    return;
  }

  jpgd_block_t *p = Pd->coeff_buf_getp(Pd->m_ac_coeffs[component_id], block_x, block_y);

  for (k = Pd->m_spectral_start; k <= Pd->m_spectral_end; k++)
  {
    s = Pd->huff_decode(Pd->m_pHuff_tabs[Pd->m_comp_ac_tab[component_id]]);

    r = s >> 4;
    s &= 15;

    if (s)
    {
      if ((k += r) > 63)
        Pd->terminate(JPGD_DECODE_ERROR);

      r = Pd->get_bits_2(s);
      s = HUFF_EXTEND_P(r, s);

      p[g_ZAG[k]] = static_cast<jpgd_block_t>(s << Pd->m_successive_low);
    }
    else
    {
      if (r == 15)
      {
        if ((k += 15) > 63)
          Pd->terminate(JPGD_DECODE_ERROR);
      }
      else
      {
        Pd->m_eob_run = 1 << r;

        if (r)
          Pd->m_eob_run += Pd->get_bits_2(r);

        Pd->m_eob_run--;

        break;
      }
    }
  }
}
//------------------------------------------------------------------------------
void progressive_block_decoder::decode_block_ac_refine(
  jpeg_decoder *Pd,
  int component_id, int block_x, int block_y)
{
  int s, k, r;
  int p1 = 1 << Pd->m_successive_low;
  int m1 = (-1) << Pd->m_successive_low;
  jpgd_block_t *p = Pd->coeff_buf_getp(Pd->m_ac_coeffs[component_id], block_x, block_y);

  k = Pd->m_spectral_start;

  if (Pd->m_eob_run == 0)
  {
    for ( ; k <= Pd->m_spectral_end; k++)
    {
      s = Pd->huff_decode(Pd->m_pHuff_tabs[Pd->m_comp_ac_tab[component_id]]);

      r = s >> 4;
      s &= 15;

      if (s)
      {
        if (s != 1)
          Pd->terminate(JPGD_DECODE_ERROR);

        if (Pd->get_bits_2(1))
          s = p1;
        else
          s = m1;
      }
      else
      {
        if (r != 15)
        {
          Pd->m_eob_run = 1 << r;

          if (r)
            Pd->m_eob_run += Pd->get_bits_2(r);

          break;
        }
      }

      do
      {
        jpgd_block_t *this_coef = p + g_ZAG[k];

        if (*this_coef != 0)
        {
          if (Pd->get_bits_2(1))
          {
            if ((*this_coef & p1) == 0)
            {
              if (*this_coef >= 0)
                *this_coef = static_cast<jpgd_block_t>(*this_coef + p1);
              else
                *this_coef = static_cast<jpgd_block_t>(*this_coef + m1);
            }
          }
        }
        else
        {
          if (--r < 0)
            break;
        }

        k++;

      } while (k <= Pd->m_spectral_end);

      if ((s) && (k < 64))
      {
        p[g_ZAG[k]] = static_cast<jpgd_block_t>(s);
      }
    }
  }

  if (Pd->m_eob_run > 0)
  {
    for ( ; k <= Pd->m_spectral_end; k++)
    {
      jpgd_block_t *this_coef = p + g_ZAG[k];

      if (*this_coef != 0)
      {
        if (Pd->get_bits_2(1))
        {
          if ((*this_coef & p1) == 0)
          {
            if (*this_coef >= 0)
              *this_coef = static_cast<jpgd_block_t>(*this_coef + p1);
            else
              *this_coef = static_cast<jpgd_block_t>(*this_coef + m1);
          }
        }
      }
    }

    Pd->m_eob_run--;
  }
}
//------------------------------------------------------------------------------
// Decode a scan in a progressively encoded image.
void jpeg_decoder::decode_scan(
  Pdecode_block_func decode_block_func)
{
  int mcu_row, mcu_col, mcu_block;
  int block_x_mcu[JPGD_MAXCOMPONENTS], m_block_y_mcu[JPGD_MAXCOMPONENTS];

  memset(m_block_y_mcu, 0, sizeof(m_block_y_mcu));

  for (mcu_col = 0; mcu_col < m_mcus_per_col; mcu_col++)
  {
    int component_num, component_id;

    memset(block_x_mcu, 0, sizeof(block_x_mcu));

    for (mcu_row = 0; mcu_row < m_mcus_per_row; mcu_row++)
    {
      int block_x_mcu_ofs = 0, block_y_mcu_ofs = 0;

      if ((m_restart_interval) && (m_restarts_left == 0))
        process_restart();

      for (mcu_block = 0; mcu_block < m_blocks_per_mcu; mcu_block++)
      {
        component_id = m_mcu_org[mcu_block];

        decode_block_func(this, component_id,
                          block_x_mcu[component_id] + block_x_mcu_ofs,
                          m_block_y_mcu[component_id] + block_y_mcu_ofs);

        if (m_comps_in_scan == 1)
          block_x_mcu[component_id]++;
        else
        {
          if (++block_x_mcu_ofs == m_comp_h_samp[component_id])
          {
            block_x_mcu_ofs = 0;

            if (++block_y_mcu_ofs == m_comp_v_samp[component_id])
            {
              block_y_mcu_ofs = 0;

              block_x_mcu[component_id] += m_comp_h_samp[component_id];
            }
          }
        }
      }

      m_restarts_left--;
    }

    if (m_comps_in_scan == 1)
      m_block_y_mcu[m_comp_list[0]]++;
    else
    {
      for (component_num = 0; component_num < m_comps_in_scan; component_num++)
      {
        component_id = m_comp_list[component_num];

        m_block_y_mcu[component_id] += m_comp_v_samp[component_id];
      }
    }
  }
}
//------------------------------------------------------------------------------
// Decode a progressively encoded image.
void jpeg_decoder::init_progressive(void)
{
  int i;

  if (m_comps_in_frame == 4)
    terminate(JPGD_UNSUPPORTED_COLORSPACE);

  // Allocate the coefficient buffers.
  for (i = 0; i < m_comps_in_frame; i++)
  {
    m_dc_coeffs[i] = coeff_buf_open(m_max_mcus_per_row * m_comp_h_samp[i],
                                  m_max_mcus_per_col * m_comp_v_samp[i], 1, 1);
    m_ac_coeffs[i] = coeff_buf_open(m_max_mcus_per_row * m_comp_h_samp[i],
                                  m_max_mcus_per_col * m_comp_v_samp[i], 8, 8);
  }

  for ( ; ; )
  {
    int dc_only_scan, refinement_scan;
    Pdecode_block_func decode_block_func;

    if (!init_scan())
      break;

    dc_only_scan    = (m_spectral_start == 0);
    refinement_scan = (m_successive_high != 0);

    if ((m_spectral_start > m_spectral_end) || (m_spectral_end > 63))
      terminate(JPGD_BAD_SOS_SPECTRAL);

    if (dc_only_scan)
    {
      if (m_spectral_end)
        terminate(JPGD_BAD_SOS_SPECTRAL);
    }
    else if (m_comps_in_scan != 1)  /* AC scans can only contain one component */
      terminate(JPGD_BAD_SOS_SPECTRAL);

    if ((refinement_scan) && (m_successive_low != m_successive_high - 1))
      terminate(JPGD_BAD_SOS_SUCCESSIVE);

    if (dc_only_scan)
    {
      if (refinement_scan)
        decode_block_func = progressive_block_decoder::decode_block_dc_refine;
      else
        decode_block_func = progressive_block_decoder::decode_block_dc_first;
    }
    else
    {
      if (refinement_scan)
        decode_block_func = progressive_block_decoder::decode_block_ac_refine;
      else
        decode_block_func = progressive_block_decoder::decode_block_ac_first;
    }

    decode_scan(decode_block_func);

    m_bits_left = 16;
    get_bits_1(16);
    get_bits_1(16);
  }

  m_comps_in_scan = m_comps_in_frame;

  for (i = 0; i < m_comps_in_frame; i++)
    m_comp_list[i] = i;

  calc_mcu_block_order();
}
//------------------------------------------------------------------------------
void jpeg_decoder::init_sequential(void)
{
  if (!init_scan())
    terminate(JPGD_UNEXPECTED_MARKER);
}
//------------------------------------------------------------------------------
void jpeg_decoder::decode_start(void)
{
  init_frame();

  if (m_progressive_flag)
    init_progressive();
  else
    init_sequential();
}
//------------------------------------------------------------------------------
// Find the start of the JPEG file and reads enough data to determine
// its size, number of components, etc.
void jpeg_decoder::decode_init(Pjpeg_decoder_stream m_pStream)
{
  init(m_pStream);

  locate_sof_marker();
}
//------------------------------------------------------------------------------
// Call get_error_code() after constructing to determine if the stream
// was valid or not. You may call the get_width(), get_height(), etc.
// methods after the constructor is called.
// You may then either destruct the object, or begin decoding the image
// by calling begin(), then decode().
jpeg_decoder::jpeg_decoder(Pjpeg_decoder_stream m_pStream)
{
  if (setjmp(m_jmp_state))
    return;

  decode_init(m_pStream);
}
//------------------------------------------------------------------------------
// If you wish to decompress the image, call this method after constructing
// the object. If JPGD_OKAY is returned you may then call decode() to
// fetch the scan lines.
int jpeg_decoder::begin(void)
{
  if (m_ready_flag)
    return (JPGD_OKAY);

  if (m_error_code)
    return (JPGD_FAILED);

  if (setjmp(m_jmp_state))
    return (JPGD_FAILED);

  decode_start();

  m_ready_flag = true;

  return (JPGD_OKAY);
}
//------------------------------------------------------------------------------
// Completely destroys the decoder object. May be called at any time.
jpeg_decoder::~jpeg_decoder()
{
  free_all_blocks();
}
//------------------------------------------------------------------------------

