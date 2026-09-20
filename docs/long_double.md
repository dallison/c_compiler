# Long double

DaveCC uses a distinct `long double` format wherever a 16-byte software IEEE
value is practical. A few targets stay aliased to `double` or `float`.

## Formats

| Target | Format | `sizeof` / align | Notes |
| --- | --- | --- | --- |
| AArch64 | IEEE binary128 | 16 / 16 | Software arithmetic |
| RISC-V 64 (LP64D) | IEEE binary128 | 16 / 16 | Software arithmetic |
| ARM (AAPCS) | IEEE binary128 | 16 / 16 | Software arithmetic |
| p-code | IEEE binary128 | 16 / 16 | Same helpers; used by constexpr / consteval |
| RISC-V 32 | IEEE binary128 | 16 / 16 | Software arithmetic |
| wasm32 | IEEE binary128 | 16 / 16 | Software arithmetic |
| x86_64 | Intel 80-bit in a 16-byte slot | 16 / 16 | Software arithmetic; bytes 10–15 are padding |
| i386 | IEEE binary64 | 8 / 8 | Shares the x86 stack/memcpy path; left aliased |
| eBPF | IEEE binary64 | 8 / 8 | No libc; not worth a 16-byte object |
| Xtensa / ESP32 | IEEE binary64 | 8 / 8 | Guest libc does not ship the helpers |
| 6502 / 65C02 | IEEE binary32 | 4 / 4 | Same representation as `float` |

The compiler predefines `__SIZEOF_LONG_DOUBLE__`, `__DAVECC_LDBL_FORMAT__`
(0=float32, 1=float64, 2=Intel 80-bit, 3=binary128), and the usual
`__LDBL_MANT_DIG__` / `__LDBL_MAX_EXP__` family. `<float.h>` reads those
macros.

## Code generation

A distinct `long double` is a 16-byte memory object. Named arguments and
returns use the existing aggregate / hidden-pointer convention. Variadic
arguments on x86_64 follow the SysV MEMORY class: the 16-byte object is
placed in the overflow area (not as a pointer in a GP register) so
`va_arg(ap, long double)` can memcpy from it. Arithmetic, compare, and
conversions call the guest helpers in `libc/long_double.c`:

`__davecc_ld_add`, `__davecc_ld_sub`, `__davecc_ld_mul`, `__davecc_ld_div`,
`__davecc_ld_neg`, `__davecc_ld_cmp`, `__davecc_ld_from_f32`,
`__davecc_ld_from_f64`, `__davecc_ld_from_i64`, `__davecc_ld_to_f32`,
`__davecc_ld_to_f64`, `__davecc_ld_to_i64`.

The helpers are implemented with the portable software IEEE in
`c_compiler/support/fp_extended.c`. The same code is used to convert
host `double` literals into the target encoding.

Constexpr and consteval compile to p-code and keep the original target's
`long_double_format`. The p-code VM implements the helpers as escapes, so
`1.0L + LDBL_EPSILON` is evaluated in the real format rather than as a host
`double`.

This does **not** yet match the native hardware ABIs (AAPCS64 Q registers,
RISC-V integer pairs, x86_64 `st(0)` / x87). The in-tree compiler and libc
agree with each other. Hardware x87 and `__float128` instructions can come
later.

## Library support

On distinct-`long double` targets the guest libc now keeps the extra precision
for:

- `strtold` / `wcstold` (software decimal accumulate)
- `scanf` `%Lf` (stores 16 bytes; parses through `strtold`)
- `printf` `%Lf` / `%Le` / `%Lg` (64-bit significand decimal conversion)
- `std::format` floating arguments and `num_put` / `num_get` `long double`
- exact `*l` helpers: `fabsl`, `copysignl`, `ceill`, `floorl`, `truncl`,
  `frexpl`, `ldexpl`, `scalbnl`, `scalblnl`, `modfl`, `ilogbl`, `nextafterl`,
  plus `fpclassify` / `signbit` on `long double`

Trigonometric and exponential `*l` wrappers still compute in `double`.
Complex `long double` arithmetic uses the same helpers per component.
