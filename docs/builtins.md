# Compiler builtins and architecture intrinsics

DaveCC recognizes a small set of compiler builtins in the frontend. Most of
them are available on every `-target`. Atomics, native bit instructions, and
SIMD headers are the exceptions; those are listed per architecture below.

There are three layers:

| Layer | What it is | How you spell it |
| --- | --- | --- |
| Frontend builtins | Parsed as special calls, not ordinary functions | `__builtin_*`, `__davecc_*`, `__atomic_*`, `__sync_*` |
| C++ type traits | Compile-time type queries | `__davecc_is_* (Type, …)` |
| Target headers / runtime | Inline wrappers or assembly helpers | `_mm_*`, `vaddq_*`, 6502 `__builtin_isalnum`, … |

GCC names that are **not** recognized include `__builtin_clz` /
`__builtin_ctz` / `__builtin_popcount` (use `__davecc_*`),
`__builtin_memcpy` as a compiler builtin, `__builtin_bswap*`,
`__builtin_ia32_*`, and `__builtin_neon_*`. SIMD is the header API, not
those GCC machine builtins.

The frontend table lives in `c_compiler/frontend/syntax/expr_parser.c`.

## Common compiler builtins

These parse on every target. Optional arguments are noted.

| Builtin | Signature | Effect |
| --- | --- | --- |
| `__builtin_FILE()` | `const char*` | Current source file |
| `__builtin_LINE()` | `unsigned` | Current source line |
| `__builtin_COLUMN()` | `unsigned` | Current source column |
| `__builtin_FUNCTION()` | `const char*` | Enclosing function name |
| `__builtin_PRETTY_FUNCTION()` | `const char*` | Decorated function name |
| `__builtin_expect(value, expected)` | both converted to `long`; result is `long` | Branch hint; both operands are evaluated |
| `__builtin_prefetch(addr [, rw [, locality]])` | `void` | Evaluates the arguments, then emits nothing. `rw` is 0 (read) or 1 (write); `locality` is 0–3 |
| `__builtin_trap()` | `void` | Calls `abort` |
| `__builtin_unreachable()` | `void` | Calls `abort` |
| `__builtin_is_constant_evaluated()` | `bool` / `int` | `true` only while folding a constant expression |
| `__builtin_start_lifetime(p)` | `void` | Lifetime start; a no-op in ordinary codegen |
| `__builtin_observable_checkpoint()` | `void` | Optimization barrier used by constexpr evaluation |
| `__builtin_va_start(ap, last)` | `void` | Standard varargs |
| `__builtin_va_arg(ap, type)` | `type` | Second argument is a type, not a value |
| `__builtin_va_copy(dst, src)` | `void` | |
| `__builtin_va_end(ap)` | `void` | |

`<stdarg.h>` macros expand to those `va_*` builtins.

`__builtin_prefetch` does not emit `PREFETCH` / `PRFM` / `pld`.
`__builtin_trap` and `__builtin_unreachable` are not `ud2` / `brk` /
`unimp`; they are calls to the guest `abort`.

## Bit operations

These take an **unsigned** integer. Count / width is any integer.
`clz` / `ctz` / `popcount` return `int`; rotates return the operand type.

| Builtin | Meaning |
| --- | --- |
| `__davecc_clz(value, width)` | Leading zeros in the low `width` bits (1–64) |
| `__davecc_ctz(value, width)` | Trailing zeros |
| `__davecc_popcount(value, width)` | Number of one-bits |
| `__davecc_rotl(value, count)` | Rotate left |
| `__davecc_rotr(value, count)` | Rotate right |

C23 `<stdbit.h>` is implemented with these (`stdc_leading_zeros`,
`stdc_trailing_zeros`, `stdc_count_ones`, `stdc_rotate_left`, …).

Native instructions are used only when the width matches the operand
width and the target has a matching opcode. Everything else, including
`popcount` on every target, is expanded in IR.

| Target | Native `clz` / `ctz` | Native rotate | `popcount` |
| --- | --- | --- | --- |
| x86_64 | 32- and 64-bit (`lzcnt` / `tzcnt` style, with a zero guard) | 32- and 64-bit | Software |
| AArch64 | 32- and 64-bit (`clz`; `ctz` via `rbit`+`clz`) | 32- and 64-bit | Software |
| ARM / armv7 | 32-bit only | 32-bit | Software |
| RISC-V 64/32, x86 (i386), 6502, wasm32, Xtensa, eBPF, p-code | Software | Software | Software |

## Atomics

`__atomic_*` and `__sync_*` are **not** parsed on targets that do not
advertise atomics. The call is rejected as an unknown function (or, on
6502 / 65C02, diagnosed as unavailable in that single-threaded profile).

Memory-order arguments are the GCC constants 0–5, also predefined as
macros on every target:

| Macro | Value |
| --- | --- |
| `__ATOMIC_RELAXED` | 0 |
| `__ATOMIC_CONSUME` | 1 |
| `__ATOMIC_ACQUIRE` | 2 |
| `__ATOMIC_RELEASE` | 3 |
| `__ATOMIC_ACQ_REL` | 4 |
| `__ATOMIC_SEQ_CST` | 5 |

A non-constant order is accepted and treated as sequential consistency.
Object sizes 1, 2, and 4 bytes are valid wherever atomics exist; 8-byte
objects need the 8-byte column below. The pointee must be a scalar or
pointer.

| Builtin | Arguments |
| --- | --- |
| `__atomic_load_n(ptr, order)` | Returns `*ptr` |
| `__atomic_store_n(ptr, val, order)` | `void` |
| `__atomic_fetch_add(ptr, val, order)` | Old value; integer or pointer `*ptr` |
| `__atomic_fetch_sub(ptr, val, order)` | Old value |
| `__atomic_add_fetch(ptr, val, order)` | New value |
| `__atomic_sub_fetch(ptr, val, order)` | New value |
| `__atomic_compare_exchange_n(ptr, expected, desired, weak, success, failure)` | `bool`; `expected` is a pointer |
| `__atomic_thread_fence(order)` | `void` |
| `__atomic_signal_fence(order)` | `void` |
| `__sync_fetch_and_add(ptr, val)` | Same as fetch-add, implicit seq_cst |
| `__sync_fetch_and_sub(ptr, val)` | |
| `__sync_add_and_fetch(ptr, val)` | |
| `__sync_sub_and_fetch(ptr, val)` | |
| `__sync_bool_compare_and_swap(ptr, expected, desired)` | `bool`; `expected` is a value |
| `__sync_val_compare_and_swap(ptr, expected, desired)` | Previous `*ptr` |
| `__sync_synchronize()` | Full fence |

| Target | Atomics | 8-byte | Complete C11 (`atomic_llong` and friends) |
| --- | --- | --- | --- |
| x86_64 | yes | yes | yes |
| RISC-V 64 | yes | yes | yes |
| RISC-V 32 | yes | no | no |
| AArch64 | yes | yes | no |
| ARM / armv7 | yes | no | no |
| x86 / i386 | yes | no | no |
| wasm32 | yes | yes | no |
| p-code | yes | yes | no |
| 6502 / 65C02 | no (`__STDC_NO_ATOMICS__`) | | |
| Xtensa / ESP32 | no | | |
| eBPF | no | | |

## C++ type-trait builtins

These take **types** in parentheses, not values, and fold to a constant.
They exist so `<type_traits>` does not depend on a host compiler. Available
on every target when compiling C++.

Unary (`__davecc_is_class(T)` and the like):

`__davecc_is_class`, `__davecc_is_enum`, `__davecc_is_union`,
`__davecc_is_destructible`, `__davecc_is_nothrow_destructible`,
`__davecc_is_trivially_destructible`, `__davecc_is_trivially_copyable`,
`__davecc_is_swappable`, `__davecc_is_member_pointer`,
`__davecc_is_member_object_pointer`, `__davecc_is_member_function_pointer`,
`__davecc_is_member_pointer_direct_object`

Binary or more (`__davecc_is_base_of(Base, Derived)`,
`__davecc_is_constructible(T, Args…)`):

`__davecc_is_base_of`, `__davecc_is_convertible`,
`__davecc_is_assignable`, `__davecc_is_nothrow_assignable`,
`__davecc_is_trivially_assignable`, `__davecc_is_constructible`,
`__davecc_is_nothrow_constructible`, `__davecc_is_trivially_constructible`,
`__davecc_is_invocable`, `__davecc_is_nothrow_invocable`,
`__davecc_is_swappable_with`

Clang / GCC `__is_class` / `__is_base_of` names are not aliases of these.

## x86 and x86_64

Predefined: `__SSE__`, `__SSE2__` (and `__x86_64__` or `__i386__`).
There is no `__SSSE3__` macro; `<tmmintrin.h>` is still accepted.

The Intel names are **headers**, implemented with GNU vector types
(`vector_size(16)`). Include `<emmintrin.h>` or `<immintrin.h>`
(`<xmmintrin.h>` only pulls in SSE2). They error if the target is not
x86 / x86_64.

### SSE2 (`<emmintrin.h>`)

Types: `__m128`, `__m128d`, `__m128i`, `__m128i_u`.

| Intrinsic | Meaning |
| --- | --- |
| `_mm_setzero_si128()` | All-zero `__m128i` |
| `_mm_set1_epi8(v)` / `_epi16` / `_epi32` | Broadcast |
| `_mm_set_epi32(e3, e2, e1, e0)` | `e0` is lane 0 |
| `_mm_set_epi64x(e1, e0)` | |
| `_mm_load_si128` / `_mm_loadu_si128` | Aligned / unaligned load |
| `_mm_store_si128` / `_mm_storeu_si128` | Aligned / unaligned store |
| `_mm_cmpeq_epi8` / `_mm_cmpgt_epi8` | Per-lane compare |
| `_mm_and_si128` / `_mm_andnot_si128` / `_mm_or_si128` / `_mm_xor_si128` | Bitwise; andnot is `(~a) & b` |
| `_mm_subs_epi8` | Saturating signed 8-bit subtract |
| `_mm_movemask_epi8` | Pack sign bits into an `int` |
| `_mm_cvtsi128_si64` | Low 64-bit lane as `long long` |

### SSSE3 (`<tmmintrin.h>`)

| Intrinsic | Meaning |
| --- | --- |
| `_mm_sign_epi8(value, signs)` | Zero / negate / keep per lane |
| `_mm_shuffle_epi8(value, indices)` | High bit of an index zeros that lane |

No AVX, AVX2, SSE4, or MMX headers. Scalar SSE (`_mm_add_ps` and the
rest of `<xmmintrin.h>`) is not implemented.

## AArch64 and ARM

`<arm_neon.h>` is available when `__aarch64__` or `__arm__` is set. The
compiler predefined `__ARM_NEON` / `__ARM_NEON__` only on **AArch64**.
ARM32 can still include the header.

Types are GNU vectors of 8 or 16 bytes: `int8x8_t`, `uint8x8_t`,
`uint64x1_t`, `float32x2_t`, `int8x16_t`, `uint8x16_t`, `int16x8_t`,
`uint16x8_t`, `int32x4_t`, `uint32x4_t`, `int64x2_t`, `uint64x2_t`,
`float32x4_t`, `float64x2_t`.

| Intrinsic | Meaning |
| --- | --- |
| `vld1_u8` / `vst1_u8` | 8-byte load / store |
| `vld1q_u8` / `vst1q_u8` | 16-byte load / store |
| `vdup_n_u8` / `vdup_n_s8` | 8-byte broadcast |
| `vdupq_n_u8` / `vdupq_n_s8` | 16-byte broadcast |
| `vceq_u8` / `vceq_s8` | 8-byte equal |
| `vcge_s8` / `vclt_s8` / `vcgt_s8` / `vcle_s8` | 8-byte signed compare |
| `vceqq_u8` / `vcgtq_s8` | 16-byte compare |
| `vreinterpret_u64_u8` / `vreinterpret_s8_u8` / `vreinterpret_u8_s8` | Same bits, new type |
| `vget_lane_u64(v, lane)` | Extract one `uint64x1_t` lane |
| `vaddq_s32` / `vaddq_f32` / `vaddq_f64` | 128-bit add |
| `vmulq_f32` | 128-bit float multiply |

This is a subset of ACLE. There is no `arm_acle.h`, SVE, or
`__builtin_arm_*`.

## 6502 and 65C02

No atomics (see [6502.md](6502.md)). No SIMD header.

Calls to the usual C library names are rewritten to assembly in
`6502 support/intrinsic.s`. The emitted symbols are the `__builtin_*`
names. Both `-target 6502` and `-target 65c02` share this list.

| Call | Runtime symbol | Role |
| --- | --- | --- |
| `isalnum`, `isalpha`, `isblank`, `iscntrl`, `isdigit`, `isgraph`, `islower`, `isprint`, `ispunct`, `isspace`, `isupper`, `isxdigit` | `__builtin_*` of the same name | Character class; result in A |
| `tolower`, `toupper` | `__builtin_tolower`, `__builtin_toupper` | Case convert |
| `memcpy`, `memset` | `__builtin_memcpy`, `__builtin_memset` | Block copy / fill |
| `memcmp` | `__builtin_memcmp` | Block compare |

Varargs lowering also uses `__builtin_va_arg`, `__builtin_va_arg2`,
`__builtin_va_arg4`, and `__builtin_va_arg8` from that file. Those are
not user-facing builtins; write `va_arg` as usual.

## Other architectures

| Target | Extra builtins or headers |
| --- | --- |
| RISC-V 64 / 32 | None. No RVV header; GNU vector types lower to scalar loops |
| wasm32 | None. No `wasm_simd128.h` |
| Xtensa / ESP32 | None |
| eBPF | None |
| p-code | None |

Those targets still have the common builtins, the `__davecc_*` bit
operations (in software), and — where the atomics table says so — the
atomic builtins.
