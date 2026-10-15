// Calls the functions in x86_64_gnu_asm_test.S; the exit status names the
// first failing check.

long asm_incdec(long x);
void asm_sized_stores(unsigned char* buf);
long asm_data_sum(void);
long asm_local_labels(long n);
long asm_disp_expr(const long* values);
void asm_rep_movsb(void* dst, const void* src, long n);
long asm_lock_xadd(long* p, long v);
long asm_max(long a, long b);
long asm_less(long a, long b);
long asm_high_byte(long x);
long asm_macro_imm(void);
long asm_padded(void);
void asm_byte_alu(unsigned char* p);
long asm_byte_carry(void);
long asm_strlen(const char* s);
void asm_fill_quads(long* p, long v, long n);
int asm_memcmp(const void* a, const void* b, long n);
void asm_move_up(void* dst, const void* src, long n);
long asm_sum_words(const unsigned short* p, long n);
long asm_cmov32_untaken(long a, long b);
long asm_max_mem(long a, const long* b);

int main(void) {
  if (asm_incdec((long)0x7fffffff00000005) != 6) return 1;

  unsigned char buf[16];
  for (int i = 0; i < 16; i++) buf[i] = 0xff;
  asm_sized_stores(buf);
  static const unsigned char expected[16] = {
      0x11, 0xff, 0x33, 0x22, 0x77, 0x66, 0x55, 0x44,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  for (int i = 0; i < 16; i++) {
    if (buf[i] != expected[i]) return 2;
  }

  if (asm_data_sum() != 1000000 + 2000 + 30 + 4) return 3;
  if (asm_local_labels(4) != 40) return 4;

  long values[4] = {1, 2, 3, 4};
  if (asm_disp_expr(values) != 4) return 5;

  char src[5] = "abcd";
  char dst[5] = {0};
  asm_rep_movsb(dst, src, 5);
  for (int i = 0; i < 5; i++) {
    if (dst[i] != src[i]) return 6;
  }

  long counter = 40;
  if (asm_lock_xadd(&counter, 2) != 40 || counter != 42) return 7;
  if (asm_max(-3, 9) != 9 || asm_max(9, -3) != 9) return 8;
  if (asm_less(-3, 9) != 1 || asm_less(9, -3) != 0) return 9;
  if (asm_high_byte(0x12345678) != 0x56) return 10;
  if (asm_macro_imm() != 58) return 11;
  if (asm_padded() != 3) return 12;

  unsigned char bytes[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  asm_byte_alu(bytes);
  static const unsigned char expected_bytes[8] = {0x13, 2, 2,    3,
                                                  0x85, 0x26, 1, 8};
  for (int i = 0; i < 8; i++) {
    if (bytes[i] != expected_bytes[i]) return 13;
  }
  if (asm_byte_carry() != 0x200) return 14;
  if (asm_strlen("hello") != 5 || asm_strlen("") != 0) return 15;

  long quads[4] = {0, 0, 0, 9};
  asm_fill_quads(quads, -2, 3);
  if (quads[0] != -2 || quads[1] != -2 || quads[2] != -2 || quads[3] != 9) {
    return 16;
  }
  if (asm_memcmp("abcd", "abcd", 4) != 0) return 17;
  if (asm_memcmp("abcd", "abxd", 4) >= 0) return 18;
  if (asm_memcmp("abzd", "abxd", 4) <= 0) return 19;

  char text[7] = "abcdef";
  asm_move_up(text + 2, text, 4);
  for (int i = 0; i < 6; i++) {
    if (text[i] != "ababcd"[i]) return 20;
  }
  static const unsigned short words[3] = {1000, 2000, 65535};
  if (asm_sum_words(words, 3) != 68535) return 21;
  if (asm_cmov32_untaken(0x123456789, 7) != 0x23456789) return 22;
  long bound = 9;
  if (asm_max_mem(-3, &bound) != 9 || asm_max_mem(12, &bound) != 12) return 23;
  return 0;
}
