//
//  bbc.h
//  BBC Micro Machine Operating System entry points and VDU graphics.
//
//  Link this with the BBC runtime (bbc_start.s, bbc_syscall.s, bbc_mos.s),
//  not the interpreter support ROM. MOS calls leave the language zero page
//  alone; on exit, bbc_return re-enters the current language (OSBYTE &8E).
//
//  MOS strings are terminated by CR (0x0d), not by a C NUL.
//
//  C++ puts the functions and the OSBYTE/OSWORD enumerations in namespace
//  bbc. <cbbc> brings those names into the global namespace.
//

#ifndef bbc_h
#define bbc_h

#ifdef __cplusplus
namespace bbc {
extern "C" {
#endif

/* Processor status after a call. Carry is bit 0. */
#define BBC_FLAG_C 0x01
#define BBC_FLAG_Z 0x02
#define BBC_FLAG_I 0x04
#define BBC_FLAG_D 0x08
#define BBC_FLAG_V 0x40
#define BBC_FLAG_N 0x80

#define BBC_CARRY(regs) (((regs).flags & BBC_FLAG_C) != 0)

typedef struct os_regs {
  unsigned char a;
  unsigned char x;
  unsigned char y;
  unsigned char flags;
} os_regs;

/* Fixed entry points. The vectored calls below jump through page 2. */
#define BBC_OSRDSC 0xffb9
#define BBC_OSEVEN 0xffbf
#define BBC_GSINIT 0xffc2
#define BBC_GSREAD 0xffc5
#define BBC_NVRDCH 0xffc8
#define BBC_NVWRCH 0xffcb
#define BBC_OSFIND 0xffce
#define BBC_OSGBPB 0xffd1
#define BBC_OSBPUT 0xffd4
#define BBC_OSBGET 0xffd7
#define BBC_OSARGS 0xffda
#define BBC_OSFILE 0xffdd
#define BBC_OSRDCH 0xffe0
#define BBC_OSASCI 0xffe3
#define BBC_OSNEWL 0xffe7
#define BBC_OSWRCR 0xffec
#define BBC_OSWRCH 0xffee
#define BBC_OSWORD 0xfff1
#define BBC_OSBYTE 0xfff4
#define BBC_OSCLI  0xfff7

/* Vectors. OSFSC has no &FFxx entry; call osfsc(). */
#define BBC_USERV 0x0200
#define BBC_BRKV  0x0202
#define BBC_IRQ1V 0x0204
#define BBC_IRQ2V 0x0206
#define BBC_CLIV  0x0208
#define BBC_BYTEV 0x020a
#define BBC_WORDV 0x020c
#define BBC_WRCHV 0x020e
#define BBC_RDCHV 0x0210
#define BBC_FILEV 0x0212
#define BBC_ARGSV 0x0214
#define BBC_BGETV 0x0216
#define BBC_BPUTV 0x0218
#define BBC_GBPBV 0x021a
#define BBC_FINDV 0x021c
#define BBC_FSCV  0x021e
#define BBC_EVNTV 0x0220
#define BBC_UPTV  0x0222
#define BBC_NETV  0x0224
#define BBC_VDUV  0x0226
#define BBC_KEYV  0x0228
#define BBC_INSV  0x022a
#define BBC_REMV  0x022c
#define BBC_CNPV  0x022e
#define BBC_IND1V 0x0230
#define BBC_IND2V 0x0232
#define BBC_IND3V 0x0234

#define BBC_OSFIND_CLOSE   0x00
#define BBC_OSFIND_OPENIN  0x40
#define BBC_OSFIND_OPENOUT 0x80
#define BBC_OSFIND_OPENUP  0xc0

#define BBC_OSFILE_SAVE   0x00
#define BBC_OSFILE_WRITE  0x01
#define BBC_OSFILE_LOADAD 0x02
#define BBC_OSFILE_EXECAD 0x03
#define BBC_OSFILE_ATTRS  0x04
#define BBC_OSFILE_INFO   0x05
#define BBC_OSFILE_DELETE 0x06
#define BBC_OSFILE_LOAD   0xff

#define BBC_OSGBPB_PUT    0x01
#define BBC_OSGBPB_PUTEND 0x02
#define BBC_OSGBPB_GET    0x03
#define BBC_OSGBPB_GETEND 0x04
#define BBC_OSGBPB_TITLE  0x05
#define BBC_OSGBPB_DIR    0x06
#define BBC_OSGBPB_LIB    0x07
#define BBC_OSGBPB_NAMES  0x08

/* 18-byte OSFILE control block. name is CR-terminated. */
typedef struct osfile_block {
  const char* name;
  unsigned long load_addr;
  unsigned long exec_addr;
  unsigned long start_addr;
  unsigned long end_addr;
} osfile_block;

/* 13-byte OSGBPB control block. */
typedef struct osgbpb_block {
  unsigned char handle;
  unsigned long address;
  unsigned long count;
  unsigned long seq_ptr;
} osgbpb_block;

/* A, X, and Y are the MOS arguments and come back in the result. */
os_regs osbyte(unsigned char a, unsigned char x, unsigned char y);
os_regs osfsc(unsigned char a, unsigned char x, unsigned char y);
os_regs osevent(unsigned char a, unsigned char x, unsigned char y);

/* XY is the address of the control block. Its layout depends on A. */
void osword(unsigned char a, void* block);

void oswrch(unsigned char c);
void osasci(unsigned char c);
void nvwrch(unsigned char c);
void osnewl(void);
void oswrcr(void);

/* Carry set means the read failed (usually Escape, A = 0x1b). */
os_regs osrdch(void);
os_regs nvrdch(void);

/* command is CR-terminated. */
void oscli(const char* command);

/* A is the OSFILE reason. The result's A is the object type. */
os_regs osfile(unsigned char a, void* block);

/* The 4-byte block is copied through zero page, which is what MOS requires. */
os_regs osargs(unsigned char a, unsigned char handle, void* block);

/* Carry set means end of file. The byte is in A. */
os_regs osbget(unsigned char handle);
void osbput(unsigned char byte, unsigned char handle);

os_regs osgbpb(unsigned char a, void* block);

/* a == 0 closes handle (0 closes every file). Otherwise a is the open
   mode and name is a CR-terminated path; handle is ignored. */
os_regs osfind(unsigned char a, unsigned char handle, const char* name);

/* Points (&F2) at string. a == 0 skips spaces and honours quotes.
   Y is the index GSREAD continues from. Carry set means the string ended. */
os_regs gsinit(unsigned char a, const char* string);
os_regs gsread(unsigned char y);

/* X and Y are the screen address. */
os_regs osrdsc(unsigned char x, unsigned char y);

/* BASIC graphics. Coordinates are signed BBC units: x is 0..1279 and y is
   0..1023, with the origin at the bottom left until origin() moves it.
   Colours from 128 upwards select the background. PLOT codes 0..7 are the
   solid actions (move/draw, relative/absolute); add 8, 16, or 24 for dotted
   and dashed lines. 64+ is a point, 80+ fills a triangle with the previous
   two points. */
#define BBC_PLOT_MOVE_REL 0
#define BBC_PLOT_DRAW_REL 1
#define BBC_PLOT_INVERT_REL 2
#define BBC_PLOT_BACK_REL 3
#define BBC_PLOT_MOVE 4
#define BBC_PLOT_DRAW 5
#define BBC_PLOT_INVERT 6
#define BBC_PLOT_BACK 7
#define BBC_PLOT_DOTTED 8
#define BBC_PLOT_SHORT_DASH 16
#define BBC_PLOT_LONG_DASH 24
#define BBC_PLOT_POINT 64
#define BBC_PLOT_TRIANGLE 80
#define BBC_BACKGROUND 128

#define BBC_GCOL_SET 0
#define BBC_GCOL_OR 1
#define BBC_GCOL_AND 2
#define BBC_GCOL_EOR 3
#define BBC_GCOL_INVERT 4
#define BBC_GCOL_LEAVE 5

/* Selects a screen mode, moves the software stack top to the new HIMEM, and
   clears the graphics viewport (CLG). Frames already on the stack stay where
   they are, so a program that switches to a larger screen must start its
   stack below that screen (bbc_mode4.ld does this for mode 4). */
void mode(unsigned char mode);
void colour(unsigned char colour);
void color(unsigned char colour);
void gcol(unsigned char action, unsigned char colour);
void palette(unsigned char logical, unsigned char physical);
void plot(unsigned char code, int x, int y);
void move(int x, int y);
void draw(int x, int y);
void dot(int x, int y);
void triangle(int x, int y);
/* Logical colour at this point, or -1 when it is outside the graphics window. */
int point(int x, int y);
void cls(void);
void clg(void);
void origin(int x, int y);
void graphics_window(int left, int bottom, int right, int top);
void text_window(unsigned char left, unsigned char bottom,
                 unsigned char right, unsigned char top);
void tab(unsigned char x, unsigned char y);
void home(void);
int pos(void);
int vpos(void);
void cursor(unsigned char on);
void text_cursor(void);
void graphics_text(void);
void defchar(unsigned char code, const unsigned char* rows);
void vdu(unsigned char byte);
void vdus(const unsigned char* bytes, unsigned int length);
void beep(void);
void paged(unsigned char on);
void colours_reset(void);
void windows_reset(void);

#ifdef __cplusplus
}  // extern "C"

/* Calls below 0x80 take one parameter in X. Calls from 0x80 take X and Y.
   These are the Acorn MOS calls, not sideways-ROM allocations. */
enum class osbyte_command : unsigned char {
  host = 0x00,
  user_flag = 0x01,
  input_stream = 0x02,
  output_streams = 0x03,
  cursor_editing = 0x04,
  printer_driver = 0x05,
  printer_ignore = 0x06,
  rs423_rx_baud = 0x07,
  rs423_tx_baud = 0x08,
  flash_mark = 0x09,
  flash_space = 0x0a,
  repeat_delay = 0x0b,
  repeat_period = 0x0c,
  disable_event = 0x0d,
  enable_event = 0x0e,
  flush_buffers = 0x0f,
  adc_channels = 0x10,
  force_adc = 0x11,
  reset_fkeys = 0x12,
  wait_vsync = 0x13,
  explode_font = 0x14,
  flush_buffer = 0x15,
  inc_poll_semaphore = 0x16,
  dec_poll_semaphore = 0x17,
  external_sound = 0x18,
  reset_font_group = 0x19,
  country_number = 0x46,
  alphabet = 0x47,
  bus_1mhz = 0x6b,
  shadow_usage = 0x6c,
  permanent_fs = 0x6d,
  rom_strobe = 0x6e,
  shadow_switch = 0x6f,
  vdu_shadow = 0x70,
  display_shadow = 0x71,
  shadow = 0x72,
  blank_palette = 0x73,
  reset_sound = 0x74,
  vdu_status = 0x75,
  keyboard_leds = 0x76,
  close_spool_exec = 0x77,
  write_keys = 0x78,
  keyboard_scan = 0x79,
  keyboard_scan_from_16 = 0x7a,
  printer_dormant = 0x7b,
  clear_escape = 0x7c,
  set_escape = 0x7d,
  ack_escape = 0x7e,
  eof = 0x7f,
  adval = 0x80,
  inkey = 0x81,
  high_order_address = 0x82,
  oshwm = 0x83,
  himem = 0x84,
  himem_for_mode = 0x85,
  cursor_position = 0x86,
  screen_char = 0x87,
  user_code = 0x88,
  cassette_motor = 0x89,
  insert_buffer = 0x8a,
  fs_opt = 0x8b,
  tape = 0x8c,
  rom_fs = 0x8d,
  enter_language = 0x8e,
  rom_service = 0x8f,
  tv = 0x90,
  remove_buffer = 0x91,
  read_fred = 0x92,
  write_fred = 0x93,
  read_jim = 0x94,
  write_jim = 0x95,
  read_sheila = 0x96,
  write_sheila = 0x97,
  buffer_status = 0x98,
  insert_input = 0x99,
  video_ula_control = 0x9a,
  video_ula_palette = 0x9b,
  acia = 0x9c,
  fast_bput = 0x9d,
  read_speech = 0x9e,
  write_speech = 0x9f,
  read_vdu_variable = 0xa0,
  read_cmos = 0xa1,
  write_cmos = 0xa2,
  application = 0xa3,
  processor_type = 0xa4,
  output_cursor = 0xa5,
  mos_variables = 0xa6,
  mos_variables_high = 0xa7,
  extended_vectors = 0xa8,
  extended_vectors_high = 0xa9,
  rom_table = 0xaa,
  rom_table_high = 0xab,
  keyboard_table = 0xac,
  keyboard_table_high = 0xad,
  vdu_variables = 0xae,
  vdu_variables_high = 0xaf,
  tape_timeout = 0xb0,
  input_device = 0xb1,
  keyboard_irq = 0xb2,
  primary_oshwm = 0xb3,
  oshwm_copy = 0xb4,
  rs423_mode = 0xb5,
  font_explosion = 0xb6,
  tape_rom_switch = 0xb7,
  video_ula_copy = 0xb8,
  palette_copy = 0xb9,
  rom_on_brk = 0xba,
  basic_rom = 0xbb,
  adc_channel = 0xbc,
  adc_max_channel = 0xbd,
  adc_type = 0xbe,
  rs423_busy = 0xbf,
  acia_control = 0xc0,
  flash_counter = 0xc1,
  flash_mark_copy = 0xc2,
  flash_space_copy = 0xc3,
  repeat_delay_copy = 0xc4,
  repeat_period_copy = 0xc5,
  exec_handle = 0xc6,
  spool_handle = 0xc7,
  break_escape_effect = 0xc8,
  keyboard_disable = 0xc9,
  keyboard_status = 0xca,
  rs423_buffer_min = 0xcb,
  rs423_ignore = 0xcc,
  rs423_destination = 0xcd,
  econet_call = 0xce,
  econet_input = 0xcf,
  econet_output = 0xd0,
  speech_suppress = 0xd1,
  sound_suppress = 0xd2,
  bell_channel = 0xd3,
  bell_volume = 0xd4,
  bell_frequency = 0xd5,
  bell_duration = 0xd6,
  startup_message = 0xd7,
  key_length = 0xd8,
  page_line_count = 0xd9,
  vdu_queue_length = 0xda,
  tab_char = 0xdb,
  escape_char = 0xdc,
  char_197_207 = 0xdd,
  char_208_223 = 0xde,
  char_224_239 = 0xdf,
  char_240_255 = 0xe0,
  fkey_status = 0xe1,
  shift_fkey_status = 0xe2,
  ctrl_fkey_status = 0xe3,
  ctrl_shift_fkey_status = 0xe4,
  escape_status = 0xe5,
  escape_effects = 0xe6,
  user_via_irq = 0xe7,
  acia_irq = 0xe8,
  system_via_irq = 0xe9,
  tube_present = 0xea,
  speech_present = 0xeb,
  output_device = 0xec,
  cursor_edit_state = 0xed,
  numeric_pad_base = 0xee,
  shadow_state = 0xef,
  country = 0xf0,
  user_flag_copy = 0xf1,
  serial_ula = 0xf2,
  time_offset = 0xf3,
  soft_key_consistency = 0xf4,
  printer_type_copy = 0xf5,
  printer_ignore_copy = 0xf6,
  break_intercept = 0xf7,
  break_intercept_low = 0xf8,
  break_intercept_high = 0xf9,
  vdu_ram = 0xfa,
  display_ram = 0xfb,
  language_rom = 0xfc,
  last_reset = 0xfd,
  available_ram = 0xfe,
  startup_options = 0xff,
};

enum class osword_command : unsigned char {
  read_line = 0x00,
  read_clock = 0x01,
  write_clock = 0x02,
  read_interval = 0x03,
  write_interval = 0x04,
  read_io_memory = 0x05,
  write_io_memory = 0x06,
  sound = 0x07,
  envelope = 0x08,
  read_pixel = 0x09,
  read_char_def = 0x0a,
  read_palette = 0x0b,
  write_palette = 0x0c,
  read_graphics_cursor = 0x0d,
  read_rtc = 0x0e,
  write_rtc = 0x0f,
  net = 0x14,
};

inline os_regs osbyte(osbyte_command cmd, unsigned char x, unsigned char y) {
  return osbyte(static_cast<unsigned char>(cmd), x, y);
}

/* MOS HIMEM (OSBYTE &84): .x = low, .y = high. On a Model B this is &5800
   in mode 4 and &7C00 in mode 7. */
inline unsigned mos_himem() {
  os_regs r = osbyte(osbyte_command::himem, 0, 0);
  return static_cast<unsigned>(r.x) | (static_cast<unsigned>(r.y) << 8);
}

inline void osword(osword_command cmd, void* block) {
  osword(static_cast<unsigned char>(cmd), block);
}

}  // namespace bbc
#endif

#endif
