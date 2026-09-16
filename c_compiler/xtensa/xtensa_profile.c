//
//  xtensa_profile.c
//  c_compiler
//

#include "xtensa_profile.h"

// Mirrors the public ESP-IDF core configuration for the original ESP32:
// Xtensa LX6.0.3, little-endian, 64 physical ARs, windowed registers,
// 16-bit density instructions, loops, integer multiply/divide, S32C1I,
// boolean registers, MAC16, and the single-precision floating-point option.
const XtensaProfile kXtensaProfileESP32LX6 = {
    .name = "esp32-lx6",
    .num_aregs = 64,
    .max_instruction_size = 3,
    .stack_alignment = 16,
    .big_endian = false,
    .windowed = true,
    .density = true,
    .loops = true,
    .mul32 = true,
    .mul32_high = true,
    .div32 = true,
    .s32c1i = true,
    .booleans = true,
    .mac16 = true,
    .single_float = true,
};
