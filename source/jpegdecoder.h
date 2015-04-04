#include <3ds.h>
//-----------------------------------------------------------
// File: jpegdecoder.h
// Small JPEG Decoder Library v0.9
// Public domain, Rich Geldreich <>
//-----------------------------------------------------------
#ifndef JPEG_DECODER_H
#define JPEG_DECODER_H
//-----------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
//-----------------------------------------------------------
/*typedef unsigned char  u8;      
typedef   signed short s16;       
typedef unsigned short u16;      
typedef unsigned int   u32;       
typedef unsigned long  u32;       
typedef   signed int   s32;  */     
//------------------------------------------------------------------------------
#define jpgd_max(a,b) (((a)>(b)) ? (a) : (b))
#define jpgd_min(a,b) (((a)<(b)) ? (a) : (b))
//------------------------------------------------------------------------------
#define JPGD_TRUE (1)
#define JPGD_FALSE (0)
//------------------------------------------------------------------------------
// Inline x86 assembler code support.
#if defined(_MSC_VER) && !defined(_WIN64)
#define JPGD_SUPPORT_X86ASM
#endif
//------------------------------------------------------------------------------
/* JPEG specific error codes */
#define JPGD_BAD_DHT_COUNTS              -200
#define JPGD_BAD_DHT_INDEX               -201
#define JPGD_BAD_DHT_MARKER              -202
#define JPGD_BAD_DQT_MARKER              -203
#define JPGD_BAD_DQT_TABLE               -204
#define JPGD_BAD_PRECISION               -205
#define JPGD_BAD_HEIGHT                  -206
#define JPGD_BAD_WIDTH                   -207
#define JPGD_TOO_MANY_COMPONENTS         -208
#define JPGD_BAD_SOF_LENGTH              -209
#define JPGD_BAD_VARIABLE_MARKER         -210
#define JPGD_BAD_DRI_LENGTH              -211
#define JPGD_BAD_SOS_LENGTH              -212
#define JPGD_BAD_SOS_COMP_ID             -213
#define JPGD_W_EXTRA_BYTES_BEFORE_MARKER -214
#define JPGD_NO_ARITHMITIC_SUPPORT       -215
#define JPGD_UNEXPECTED_MARKER           -216
#define JPGD_NOT_JPEG                    -217
#define JPGD_UNSUPPORTED_MARKER          -218
#define JPGD_BAD_DQT_LENGTH              -219
#define JPGD_TOO_MANY_BLOCKS             -221
#define JPGD_UNDEFINED_QUANT_TABLE       -222
#define JPGD_UNDEFINED_HUFF_TABLE        -223
#define JPGD_NOT_SINGLE_SCAN             -224
#define JPGD_UNSUPPORTED_COLORSPACE      -225
#define JPGD_UNSUPPORTED_SAMP_FACTORS    -226
#define JPGD_DECODE_ERROR                -227
#define JPGD_BAD_RESTART_MARKER          -228
#define JPGD_ASSERTION_ERROR             -229
#define JPGD_BAD_SOS_SPECTRAL            -230
#define JPGD_BAD_SOS_SUCCESSIVE          -231
#define JPGD_STREAM_READ                 -232
#define JPGD_NOTENOUGHMEM                -233
//------------------------------------------------------------------------------
const int JPGD_FAILED = -1;
const int JPGD_DONE = 1;
const int JPGD_OKAY = 0;
//------------------------------------------------------------------------------
enum
{
	JPGD_GRAYSCALE = 0,
	JPGD_YH1V1,
	JPGD_YH2V1,
	JPGD_YH1V2,
	JPGD_YH2V2
};
//------------------------------------------------------------------------------
#define JPGD_INBUFSIZE 8192
//------------------------------------------------------------------------------
// May need to be adjusted if support for other colorspaces/sampling factors is added
#define JPGD_MAXBLOCKSPERMCU 10
//------------------------------------------------------------------------------
#define JPGD_MAXHUFFTABLES   8
#define JPGD_MAXQUANTTABLES  4
#define JPGD_MAXCOMPONENTS   4
#define JPGD_MAXCOMPSINSCAN  4
//------------------------------------------------------------------------------
// Max. allocated m_blocks
#define JPGD_MAX_ALLOCATED_BLOCKS 128
//------------------------------------------------------------------------------
// Increase this if you increase the max width!
#define JPGD_MAXBLOCKSPERROW 8192
//------------------------------------------------------------------------------
#define JPGD_MAX_HEIGHT 16384
#define JPGD_MAX_WIDTH  16384
//------------------------------------------------------------------------------
typedef s16 jpgd_quant_t;
typedef s16 jpgd_block_t;
//------------------------------------------------------------------------------
// idct.cpp
void idct(const jpgd_block_t *data, u8 *Pdst_ptr, int block_max_zag);
void idct_4x4(const jpgd_block_t* data, u8* Pdst_ptr);
//------------------------------------------------------------------------------
// fidctfst.cpp
void jpeg_idct_ifast (
  jpgd_block_t* inptr,
  short *quantptr,
  u8 * *outptr,
  int output_col);

void jpeg_idct_ifast_deinit(void);

bool jpeg_idct_ifast_avail(void);
//------------------------------------------------------------------------------
typedef enum
{
  M_SOF0  = 0xC0,
  M_SOF1  = 0xC1,
  M_SOF2  = 0xC2,
  M_SOF3  = 0xC3,

  M_SOF5  = 0xC5,
  M_SOF6  = 0xC6,
  M_SOF7  = 0xC7,

  M_JPG   = 0xC8,
  M_SOF9  = 0xC9,
  M_SOF10 = 0xCA,
  M_SOF11 = 0xCB,

  M_SOF13 = 0xCD,
  M_SOF14 = 0xCE,
  M_SOF15 = 0xCF,

  M_DHT   = 0xC4,

  M_DAC   = 0xCC,

  M_RST0  = 0xD0,
  M_RST1  = 0xD1,
  M_RST2  = 0xD2,
  M_RST3  = 0xD3,
  M_RST4  = 0xD4,
  M_RST5  = 0xD5,
  M_RST6  = 0xD6,
  M_RST7  = 0xD7,

  M_SOI   = 0xD8,
  M_EOI   = 0xD9,
  M_SOS   = 0xDA,
  M_DQT   = 0xDB,
  M_DNL   = 0xDC,
  M_DRI   = 0xDD,
  M_DHP   = 0xDE,
  M_EXP   = 0xDF,

  M_APP0  = 0xE0,
  M_APP15 = 0xEF,

  M_JPG0  = 0xF0,
  M_JPG13 = 0xFD,
  M_COM   = 0xFE,

  M_TEM   = 0x01,

  M_ERROR = 0x100
} JPEG_MARKER;
//------------------------------------------------------------------------------
#define RST0 0xD0
//------------------------------------------------------------------------------
typedef struct huff_tables_tag
{
  bool ac_table;
  u32  look_up[256];
  u32  look_up2[256];
  u8 code_size[256];
  u32  tree[512];
} huff_tables_t, *Phuff_tables_t;
//------------------------------------------------------------------------------
typedef struct coeff_buf_tag
{
  u8 *Pdata;

  int block_num_x, block_num_y;
  int block_len_x, block_len_y;

  int block_size;

} coeff_buf_t, *Pcoeff_buf_t;
//------------------------------------------------------------------------------
class jpeg_decoder;
typedef void (*Pdecode_block_func)(jpeg_decoder *, int, int, int);
//------------------------------------------------------------------------------
class progressive_block_decoder
{
public:
  static void decode_block_dc_first(
    jpeg_decoder *Pd,
    int component_id, int block_x, int block_y);
  static void decode_block_dc_refine(
    jpeg_decoder *Pd,
    int component_id, int block_x, int block_y);
  static void decode_block_ac_first(
    jpeg_decoder *Pd,
    int component_id, int block_x, int block_y);
  static void decode_block_ac_refine(
    jpeg_decoder *Pd,
    int component_id, int block_x, int block_y);
};
//------------------------------------------------------------------------------
// Input stream interface.
// Derive from this class to fetch input data from sources other than
// files. An important requirement is that you *must* set m_eof_flag to true
// when no more data is available to fetch!
// The decoder is rather "greedy": it will keep on calling this method until
// its internal input buffer is full, or until the EOF flag is set.
// It the input stream contains data after the JPEG stream's EOI (end of
// image) marker it will probably be pulled into the internal buffer.
// Call the get_total_bytes_read() method to determine the true
// size of the JPEG stream.
class jpeg_decoder_stream
{
public:

  jpeg_decoder_stream()
  {
  }

  virtual ~jpeg_decoder_stream()
  {
  }

  // The read() method is called when the internal input buffer is empty.
  // Pbuf - input buffer
  // max_bytes_to_read - maximum bytes that can be written to Pbuf
  // Peof_flag - set this to true if at end of stream (no more bytes remaining)
  // Return -1 on error, otherwise return the number of bytes actually
  // written to the buffer (which may be 0).
  // Notes: This method will be called in a loop until you set *Peof_flag to
  // true or the internal buffer is full.
  // called, unlike previous versions.
  virtual int read(u8 *Pbuf, int max_bytes_to_read, bool *Peof_flag) = 0;

  virtual void attach(void)
  {
  }

  virtual void detach(void)
  {
  }
};
//------------------------------------------------------------------------------
typedef jpeg_decoder_stream *Pjpeg_decoder_stream;
//------------------------------------------------------------------------------
// Here's an example FILE stream class.
class jpeg_decoder_file_stream : public jpeg_decoder_stream
{
  FILE *Pfile;
  bool m_eof_flag, error_flag;

public:

  jpeg_decoder_file_stream()
  {
    Pfile = NULL;
    m_eof_flag = false;
    error_flag = false;
  }

  void close(void)
  {
    if (Pfile)
    {
      fclose(Pfile);
      Pfile = NULL;
    }

    m_eof_flag = false;
    error_flag = false;
  }

  virtual ~jpeg_decoder_file_stream()
  {
    close();
  }

  bool open(const char *Pfilename)
  {
    close();

    m_eof_flag = false;
    error_flag = false;
#if defined(_MSC_VER)
	Pfile = NULL;
	fopen_s(&Pfile, Pfilename, "rb");
#else
	Pfile = fopen(Pfilename, "rb");
#endif
    if (!Pfile)
      return (true);

    return (false);
  }

  virtual int read(u8 *Pbuf, int max_bytes_to_read, bool *Peof_flag)
  {
    if (!Pfile)
      return (-1);

    if (m_eof_flag)
    {
      *Peof_flag = true;
      return (0);
    }

    if (error_flag)
      return (-1);

    int bytes_read = static_cast<int>(fread(Pbuf, 1, max_bytes_to_read, Pfile));

    if (bytes_read < max_bytes_to_read)
    {
      if (ferror(Pfile))
      {
        error_flag = true;
        return (-1);
      }

      m_eof_flag = true;
      *Peof_flag = true;
    }

    return (bytes_read);
  }

  bool get_error_status(void)
  {
    return (error_flag);
  }

  bool reset(void)
  {
    if (error_flag)
      return (true);

    fseek(Pfile, 0, SEEK_SET);

    m_eof_flag = false;

    return (false);
  }

  int get_size(void)
  {
    if (!Pfile)
      return (-1);

    int loc = ftell(Pfile);

    fseek(Pfile, 0, SEEK_END);

    int size = ftell(Pfile);

    fseek(Pfile, loc, SEEK_SET);

    return (size);
  }
};
//------------------------------------------------------------------------------
typedef jpeg_decoder_file_stream *jpeg_decoder_file_stream_ptr_t;
//------------------------------------------------------------------------------
// Disable no return value warning, for rol() method
#pragma warning(push)
#pragma warning( disable : 4035 4799 )
//------------------------------------------------------------------------------
class jpeg_decoder
{
  friend class progressive_block_decoder;

private:

  void free_all_blocks(void);
  void terminate(int status);
  void *alloc(int n);
  void word_clear(void *p, u16 c, u32 n);
  void prep_in_buffer(void);
  void read_dht_marker(void);
  void read_dqt_marker(void);
  void read_sof_marker(void);
  void skip_variable_marker(void);
  void read_dri_marker(void);
  void read_sos_marker(void);
  int next_marker(void);
  int process_markers(void);
  void locate_soi_marker(void);
  void locate_sof_marker(void);
  int locate_sos_marker(void);
  void init(Pjpeg_decoder_stream m_pStream);
  void create_look_ups(void);
  void fix_in_buffer(void);
  void transform_mcu(int mcu_row);
  void transform_mcu_expand(int mcu_row);

  Pcoeff_buf_t coeff_buf_open(
    int block_num_x, int block_num_y,
    int block_len_x, int block_len_y);

  jpgd_block_t *coeff_buf_getp(
    Pcoeff_buf_t cb,
    int block_x, int block_y);

  void load_next_row(void);
  void decode_next_row(void);
  void make_huff_table(
    int index,
    Phuff_tables_t hs);
  void check_quant_tables(void);
  void check_huff_tables(void);
  void calc_mcu_block_order(void);
  int init_scan(void);
  void init_quant_tables(void);
  void init_frame(void);
  void process_restart(void);
  void decode_scan(
    Pdecode_block_func decode_block_func);
  void init_progressive(void);
  void init_sequential(void);
  void decode_start(void);
  void decode_init(Pjpeg_decoder_stream m_pStream);
  void H2V2Convert(void);
  void H2V1Convert(void);
  void H1V2Convert(void);
  void H1V1Convert(void);
  void grey_convert(void);
  void expanded_convert(void);
  void find_eoi(void);
//------------------
  inline u32 get_char(void);
  inline u32 get_char(bool *Ppadding_flag);
  inline void stuff_char(u8 q);
  inline u8 get_octet(void);
  inline u32 get_bits_1(int num_bits);
  inline u32 get_bits_2(int numbits);
  inline int huff_decode(Phuff_tables_t Ph);
  inline int huff_decode(Phuff_tables_t Ph, int& extrabits);
#ifdef JPGD_SUPPORT_X86ASM
  inline u32 huff_extend(u32 i, int c);
#endif
  static inline u8 clamp(int i);
//------------------
	jmp_buf m_jmp_state;

  int   m_image_x_size;
  int   m_image_y_size;

  Pjpeg_decoder_stream m_pStream;

  int   m_progressive_flag;

  u8 m_huff_ac[JPGD_MAXHUFFTABLES];
  u8* m_huff_num[JPGD_MAXHUFFTABLES];  /* pointer to number of Huffman codes per bit size */
  u8* m_huff_val[JPGD_MAXHUFFTABLES];  /* pointer to Huffman codes per bit size */

  jpgd_quant_t* m_quant[JPGD_MAXQUANTTABLES];    /* pointer to quantization tables */

  int   m_scan_type;                      /* Grey, Yh1v1, Yh1v2, Yh2v1, Yh2v2,
                                           CMYK111, CMYK4114 */

  int   m_comps_in_frame;                 /* # of components in frame */
  int   m_comp_h_samp[JPGD_MAXCOMPONENTS];     /* component's horizontal sampling factor */
  int   m_comp_v_samp[JPGD_MAXCOMPONENTS];     /* component's vertical sampling factor */
  int   m_comp_quant[JPGD_MAXCOMPONENTS];      /* component's quantization table selector */
  int   m_comp_ident[JPGD_MAXCOMPONENTS];      /* component's ID */

  int   m_comp_h_blocks[JPGD_MAXCOMPONENTS];
  int   m_comp_v_blocks[JPGD_MAXCOMPONENTS];

  int   m_comps_in_scan;                  /* # of components in scan */
  int   m_comp_list[JPGD_MAXCOMPSINSCAN];      /* components in this scan */
  int   m_comp_dc_tab[JPGD_MAXCOMPONENTS];     /* component's DC Huffman coding table selector */
  int   m_comp_ac_tab[JPGD_MAXCOMPONENTS];     /* component's AC Huffman coding table selector */

  int   m_spectral_start;                 /* spectral selection start */
  int   m_spectral_end;                   /* spectral selection end   */
  int   m_successive_low;                 /* successive approximation low */
  int   m_successive_high;                /* successive approximation high */

  int   m_max_mcu_x_size;                 /* MCU's max. X size in pixels */
  int   m_max_mcu_y_size;                 /* MCU's max. Y size in pixels */

  int   m_blocks_per_mcu;
  int   m_max_blocks_per_row;
  int   m_mcus_per_row, m_mcus_per_col;

  int   m_mcu_org[JPGD_MAXBLOCKSPERMCU];

  int   m_total_lines_left;               /* total # lines left in image */
  int   m_mcu_lines_left;                 /* total # lines left in this MCU */

  int   m_real_dest_bytes_per_scan_line;
  int   m_dest_bytes_per_scan_line;        /* rounded up */
  int   m_dest_bytes_per_pixel;            /* currently, 4 (RGB) or 1 (Y) */

  void* m_blocks[JPGD_MAX_ALLOCATED_BLOCKS];         /* list of all dynamically allocated memory m_blocks */

  Phuff_tables_t m_pHuff_tabs[JPGD_MAXHUFFTABLES];

  Pcoeff_buf_t m_dc_coeffs[JPGD_MAXCOMPONENTS];
  Pcoeff_buf_t m_ac_coeffs[JPGD_MAXCOMPONENTS];

  int m_eob_run;

  int m_block_y_mcu[JPGD_MAXCOMPONENTS];

  u8* m_pIn_buf_ofs;
  int m_in_buf_left;
  int m_tem_flag;
  bool m_eof_flag;

  u8 m_padd_1[128];
  u8 m_in_buf[JPGD_INBUFSIZE + 128];
  u8 m_padd_2[128];

  int   m_bits_left;
  u32  m_bit_buf;
  
  int   m_restart_interval;
  int   m_restarts_left;
  int   m_next_restart_num;

  int   m_max_mcus_per_row;
  int   m_max_blocks_per_mcu;
  int   m_expanded_blocks_per_mcu;
  int   m_expanded_blocks_per_row;
  int   m_expanded_blocks_per_component;
  bool  m_freq_domain_chroma_upsample;

  int   m_max_mcus_per_col;

  u32  m_last_dc_val[JPGD_MAXCOMPONENTS];

  jpgd_block_t* m_pMCU_coefficients;
  int   m_mcu_block_max_zag[JPGD_MAXBLOCKSPERMCU];

  u8* m_pSample_buf;

  int   m_crr[256];
  int   m_cbb[256];
  int   m_padd;
  int  m_crg[256];
  int  m_cbg[256];

  u8* m_pScan_line_0;
  u8* m_pScan_line_1;
  
  int m_error_code;
  bool m_ready_flag;
  
  int m_total_bytes_read;

public:

  // Call get_error_code() after constructing to determine if the stream
  // was valid or not. You may call the get_width(), get_height(), etc.
  // methods after the constructor is called.
  // You may then either destruct the object, or begin decoding the image
  // by calling begin(), then decode().
  jpeg_decoder(Pjpeg_decoder_stream m_pStream);

  // If you wish to decompress the image, call this method after constructing
  // the object. If JPGD_OKAY is returned you may then call decode() to
  // fetch the scan lines.
  int begin(void);

  // Returns the next scan line.
  // Returns JPGD_DONE if all scan lines have been returned.
  // Returns JPGD_OKAY if a scan line has been returned.
  // Returns JPGD_FAILED if an error occured.
  int decode(const void** Pscan_line_ofs, u32* Pscan_line_len);

  ~jpeg_decoder();

  // if an error occurs, the error code will be non-zero.
  // See the "JPEG specific error codes" defines above.
  int get_error_code(void) const { return (m_error_code); }

  int get_width(void) const { return (m_image_x_size); }

	int width(void) const { return (m_image_x_size); }

  int get_height(void) const { return (m_image_y_size); }

  int get_num_components(void) const { return (m_comps_in_frame); }

  int get_bytes_per_pixel(void) const { return (m_dest_bytes_per_pixel); }

  int get_bytes_per_scan_line(void) const { return (m_image_x_size * get_bytes_per_pixel()); }

  int get_total_bytes_read(void) const { return (m_total_bytes_read); }
};
//------------------------------------------------------------------------------
#include "jpegdecoder.inl"
//------------------------------------------------------------------------------
#pragma warning(pop)
//------------------------------------------------------------------------------
typedef jpeg_decoder *jpeg_decoder_ptr_t;
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
