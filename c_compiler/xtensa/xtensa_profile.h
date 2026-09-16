//
//  xtensa_profile.h
//  c_compiler
//
//  Xtensa processor configuration used by the classic ESP32.
//

#ifndef xtensa_profile_h
#define xtensa_profile_h

#include <stdbool.h>

typedef struct {
  const char* name;
  int num_aregs;
  int max_instruction_size;
  int stack_alignment;
  bool big_endian;
  bool windowed;
  bool density;
  bool loops;
  bool mul32;
  bool mul32_high;
  bool div32;
  bool s32c1i;
  bool booleans;
  bool mac16;
  bool single_float;
} XtensaProfile;

extern const XtensaProfile kXtensaProfileESP32LX6;

#endif /* xtensa_profile_h */
