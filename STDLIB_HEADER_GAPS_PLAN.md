# Standard-library header gaps: survey and implementation plan

Branch: `feature/stdlib-header-gaps` (worktree `/Users/FZDSZZ/c_compiler-header-gaps`)

Scope: close the remaining gaps in the C and C++ standard header sets shipped
from `libc/include`, including headers mandated by *old* standards (C89/C95/C99,
C++98/03/11/14) that were never written, not just recent ones.

**Ground rule for this work: when conforming header or test code exposes a
compiler defect, fix the defect in the compiler.** Do not reshape the library
code to dodge it, do not add `#ifdef __davecc__` escape hatches, and do not mark
the test `expected_fail`. Every such fix gets its own minimal regression test
under `cxx_testsuite/tests/exec/` (or `c_testsuite/`) that is independent of the
header that exposed it.

## 1. Survey

Inventory taken against `libc/include` at `98324eb`.

### 1.1 C headers

Present (23): `assert.h` `ctype.h` `errno.h` `float.h` `inttypes.h` `limits.h`
`math.h` `setjmp.h` `stdarg.h` `stdatomic.h` `stdbit.h` `stdbool.h`
`stdckdint.h` `stddef.h` `stdint.h` `stdio.h` `stdlib.h` `stdnoreturn.h`
`string.h` `threads.h` `time.h` `wchar.h` (stub) — plus POSIX extras
(`fcntl.h` `unistd.h` `strings.h` `sys/*`).

Missing (9):

| Header | Std | Notes |
| --- | --- | --- |
| `locale.h` | C89 | `setlocale`, `localeconv`. C++ `<locale>` already exists and works. |
| `signal.h` | C89 | `signal`, `raise`, `sig_atomic_t`. No signal support anywhere in libc, loader, or interpreters. |
| `iso646.h` | C95 | Pure macros; trivial. |
| `wctype.h` | C95 | `iswalpha` family, `wctype_t`, `towlower`. |
| `complex.h` | C99 | Blocked: compiler rejects `_Complex`. |
| `fenv.h` | C99 | Rounding modes and exception flags; per-backend. |
| `tgmath.h` | C99 | `_Generic` works, so the real-typed half is unblocked today. |
| `stdalign.h` | C11 | Macros only, but blocked by an `_Alignas` parser bug (§3.1). |
| `uchar.h` | C11 | `mbrtoc16`/`c16rtomb` etc.; `char16_t`/`char32_t` work in C++ already. |

`wchar.h` is present but is a 31-line stub that only typedefs `wchar_t`,
`size_t`, `NULL`, and misspells `WCHAR_MAX` as `WHAR_MAX` (include guard is
`whar_h`). It has none of the C95/C99 wide-string, wide-I/O, or `mbstate_t`
surface, so it counts as a gap rather than a completed header.

### 1.2 C++ headers

Present (~110), including a strong C++20/23/26 surface: `<format>` `<ranges>`
`<coroutine>` `<mdspan>` `<flat_map>` `<generator>` `<expected>` `<stacktrace>`
`<print>` `<hive>` `<contracts>` `<inplace_vector>` `<meta>` `<filesystem>`
`<syncstream>`.

Missing, grouped by the standard that introduced them:

| Std | Missing headers |
| --- | --- |
| C++98 | `<bitset>` `<complex>` `<iomanip>` `<numeric>` `<valarray>` `<ciso646>` `<clocale>` `<csignal>` `<cwctype>` |
| C++11 | `<forward_list>` `<future>` `<regex>` `<scoped_allocator>` `<typeindex>` `<cfenv>` `<cuchar>` + deprecated `<ccomplex>` `<cstdalign>` `<cstdbool>` `<ctgmath>` `<codecvt>` |
| C++14 | `<shared_mutex>` |
| C++17 | `<charconv>` `<execution>` |
| C++23 | `<spanstream>` |
| C++26 | `<debugging>` `<hazard_pointer>` `<linalg>` `<rcu>` `<simd>` `<text_encoding>` |

The C++98 gaps are the notable finding: the library skipped forward past a
chunk of its own foundation. These are not merely absent wrappers — the
underlying facilities are absent too. `accumulate`, `iota` (outside
`<ranges>`), `gcd`, `lcm`, `midpoint`, `to_chars`, and `from_chars` have no
definition anywhere in `libc/include`.

### 1.3 Compiler capability probes

Probed against a fresh `bazel build //:davecc //:libc_x86_64 //:x86_64`:

| Capability | Result |
| --- | --- |
| `_Generic` (C11) | works — `<tgmath.h>` is viable |
| `_Alignof`, file-scope `_Alignas`, `__attribute__((aligned))` | work |
| block-scope `_Alignas` | **broken** (§3.1) |
| C++ `alignas` / `alignof` | work |
| `char16_t` / `char32_t` / `wchar_t` + `u""` `U""` `L""` | work |
| non-type template params, proxy references, template UDL `operator""` | work |
| `_Complex` | **unsupported**, diagnosed outright (§3.2) |
| `__CHAR16_TYPE__` and friends | not predefined (§3.3) |
| `<locale>`, `<ios>` width/precision/fill/flags | work |

### 1.4 The C++ library only works at C++20 and later

Found by the Phase 0 harness (§2, `libc/tests/standard_headers.sh`) once it
existed. Independently of the missing headers, **most of the existing C++
headers do not compile at `-std=c++11`, `-std=c++14`, or `-std=c++17`**. They
were only ever exercised at C++20 and above, and they use C++17/20 syntax
unconditionally:

- `if constexpr` in headers that predate C++17 (`<memory>`, `<variant>`,
  `<tuple>`, `<chrono>`, `<functional>`, `<iterator>`, `<any>`, ...).
- `requires` clauses in headers that predate C++20 (`<optional>`, `<any>`,
  `<functional>`, `<bit>`, `<iterator>`, `<map>`, `<set>`, `<thread>`, ...).
- Unconditional `#include <concepts>` from `<iterator>` and `<random>`.

Roughly 130 `requires` clauses and 70 `if constexpr` uses sit in headers
mandated by an earlier standard. Because these headers are included
transitively by nearly everything, a single blocker fails dozens of headers at
once, so the count comes down in large steps as each is fixed.

This is a larger body of work than the missing headers themselves and is
tracked as Phase 6.

## 2. Implementation phases

Ordered so that each phase unblocks the next and the cheap, high-value headers
land first. Each header is done when it (a) compiles standalone in every
`-std=` mode that mandates it, (b) has an exec test, (c) is registered in the
three integration points of §4.

### Phase 0 — harness (done)

`libc/tests/standard_headers.sh <davecc> <target>` compiles a one-line
translation unit per mandated header at every `-std=` level that mandates it.
It carries explicit per-standard header lists, so it doubles as the gap ledger:
headers still to be written sit in the `c_todo`/`cxx_todo` sets and are reported
as `TODO` without failing the run; anything outside those sets that fails to
compile is a `FAIL` and fails the run. Move a header out of its todo set the
moment it lands.

Still to do: extend it (or `libc/tests/compile_all.sh`) across AArch64, ARM,
RISC-V, 65C02, and P-code. The constrained targets are supported targets, so a
header that only builds on x86-64 is not finished.

### Phase 1 — macro-only headers (done)

`iso646.h`, `stdalign.h`, `<ciso646>`, `<cstdalign>`, `<cstdbool>`, covered by
`cxx_testsuite/tests/exec/0461_stdalign_iso646_headers.cpp`.

The real content of this phase was the §3.1 parser fix and the two alignment
defects behind it (§3.2, §3.3).

`<ciso646>`, `<cstdalign>`, and `<cstdbool>` are removed in C++20, so they
`#error` there and the harness stops requiring them from C++20 on. The same
treatment applies to `<ctgmath>` and `<ccomplex>` when they land.

### Phase 2 — C++ headers with no new runtime dependency

Highest value per unit of risk; all are pure headers over facilities that
already exist.

1. `<numeric>` — `accumulate`, `inner_product`, `partial_sum`,
   `adjacent_difference`, `iota`, `gcd`, `lcm`, `midpoint`, `reduce`,
   `transform_reduce`, `inclusive_scan`, `exclusive_scan`, `saturate_cast`.
2. `<bitset>` — including the `reference` proxy, stream insertion/extraction,
   and `to_string`/`to_ulong`/`to_ullong`. Proxy references already work.
3. `<typeindex>` — thin, needs `<typeinfo>` (present) and `std::hash`.
4. `<forward_list>` — singly linked list; mirrors the existing `<list>`.
5. `<iomanip>` — `setw`, `setprecision`, `setfill`, `setbase`, `quoted`,
   `get_money`/`put_money`, `get_time`/`put_time`. All `<ios>` state accessors
   it needs already work.
6. `<complex>` — `std::complex` is a class template and does **not** depend on
   the compiler's `_Complex`, so it is unblocked. Do it here, well before the C
   `complex.h`.
7. `<charconv>` — `to_chars`/`from_chars` for integers first, then
   floating-point. Nothing exists today; `libc/include/__itoa.h` and the
   `string_to_string_*` translation units are prior art to reuse or supersede.
8. `<scoped_allocator>` — mechanical over the existing allocator machinery.
9. `<valarray>` — the expression-template slice/mask/indirect surface; the
   largest item in this phase.
10. `<spanstream>` — `<span>` and `<streambuf>` both exist.

### Phase 3 — C runtime headers

1. `locale.h` + `<clocale>` — `setlocale`, `localeconv`, `struct lconv`, the
   `LC_*` categories, with a conforming `"C"` locale. Reconcile with the
   existing `libc/locale.cc` and `libc/include/__locale_core` rather than
   duplicating state.
2. `wchar.h` rewrite + `wctype.h` + `<cwctype>` — fix the stub: correct include
   guard, `WCHAR_MAX`/`WCHAR_MIN`, `mbstate_t`, `wint_t`, `WEOF`, the
   `wcs*`/`wmem*` families, wide `printf`/`scanf`, and the `isw*`/`tow*`
   classification functions. `<cwchar>` currently wraps the stub and will need
   to grow with it.
3. `uchar.h` + `<cuchar>` — `mbrtoc8`/`c8rtomb`/`mbrtoc16`/`c16rtomb`/
   `mbrtoc32`/`c32rtomb` over the same `mbstate_t`. Needs §3.3.
4. `signal.h` + `<csignal>` — decide the model explicitly. Recommendation:
   implement `signal`/`raise`/`sig_atomic_t` entirely guest-side (a handler
   table that `raise` dispatches directly, `SIG_DFL` for `SIGABRT` terminating
   the process) and route the existing `abort` through `raise(SIGABRT)`.
   Asynchronous host-delivered signals are a separate, larger loader and
   interpreter change and are explicitly out of scope for the header work; the
   header must still declare the full C89 set.
5. `fenv.h` + `<cfenv>` — `fenv_t`, `fexcept_t`, `FE_*` flags and rounding
   modes, per backend. On targets with no hardware FP control this becomes a
   software-emulated rounding mode honored by the interpreter, so scope this
   against the constrained targets before starting.

### Phase 4 — threading headers

Gate on `__DAVECC_HAS_GUEST_THREADS__` the way `<thread>`, `<latch>`, and
`<barrier>` already do; the headers must still parse where threads are absent.

1. `<shared_mutex>` — `shared_mutex`, `shared_timed_mutex`, `shared_lock`.
2. `<future>` — `promise`, `future`, `shared_future`, `packaged_task`, `async`,
   `future_error`, `launch`. Builds on `<mutex>` and `<condition_variable>`,
   both present.
3. `<execution>` — the C++17 policy tag types plus a correct serial fallback
   for the parallel overloads.

### Phase 5 — long tail

1. `<regex>` — the largest single item in the whole plan; ECMAScript grammar
   plus the POSIX variants, `regex_iterator`, `regex_token_iterator`.
2. `<codecvt>` — deprecated in C++17, removed in C++26. Needed for C++11–17
   conformance; gate accordingly.
3. `<complex.h>` + `<ccomplex>` + `<tgmath.h>` + `<ctgmath>` — blocked on §3.2.
   `tgmath.h` can ship its real-typed half earlier if the complex half is
   staged behind the compiler work.
4. C++26: `<debugging>`, `<text_encoding>`, `<hazard_pointer>`, `<rcu>`,
   `<linalg>`, `<simd>`. Treat as a separate effort after the C++98–23 set is
   whole; `<simd>` in particular wants backend vector support.

### Phase 6 — make the existing headers work below C++20

Per §1.4. Run `libc/tests/standard_headers.sh` and fix whatever blocker it
reports first: the headers are so interdependent that one blocker masks the
rest, and clearing it typically drops the failure count by dozens at a time.

The mechanical patterns are:

- `if constexpr` in a pre-C++17 header becomes tag dispatch on
  `integral_constant<bool, ...>` — but only where the untaken branch would have
  been well-formed anyway; see the warning at the end of §3.6.
- A `requires` clause in a pre-C++20 header becomes `enable_if` SFINAE. All the
  usual spellings now work, including the defaulted non-type parameter (§3.6).
- A header from a later standard that is pulled in transitively gets its body
  wrapped in `#if __cplusplus >= <its standard>` so that it stays includable
  but empty, as already done in `<concepts>` and `<memory_resource>`.

Fixed so far: `<exception>`, `<memory>`, `<concepts>`, `<memory_resource>`,
`<iterator>`, `<optional>`, `<functional>`. C++17 now reaches `<algorithm>`,
`<array>`, `<deque>`, `<iostream>`, `<iterator>`, `<list>`, `<map>`, `<memory>`,
`<optional>`, `<set>`, `<string>`, `<tuple>` and `<vector>`.

Remaining blockers, in the order the harness reports them:

| Blocker | Nature |
| --- | --- |
| `<bit>` `requires` clauses | `<limits>` pulls `<bit>` in below C++20 |
| `<any>` `requires` clauses | C++17 header, so must not use `requires` |
| `<tuple>` `if constexpr` | blocks C++11/14 only; needs an explicit return type |

`<tuple>` is the one that is not mechanical: `__get<I>()` returns `auto&` and
selects the member with `if constexpr`, so a C++11 version needs the return
type spelled through `tuple_element` rather than deduced. It is also the last
blocker for C++11 and C++14, so it stays last.

## 3. Compiler defects

Expect more to surface as conforming header code gets written — each one is
fixed at its source, per the ground rule above.

### 3.1 Block-scope `_Alignas` is not parsed (fixed)

```c
int main(void) {
    _Alignas(16) char buf[16];   // error: Expression syntax error; primary expression expected
    return buf[0];
}
```

File-scope `_Alignas`, `_Alignof`, and C++ `alignas` all worked, so the
alignment specifier was missing only from the block-scope declaration path.
`SyntaxLookingAtDeclaration` did not list `TOK(alignas)`, so a statement
beginning with an alignment specifier was parsed as an expression. No
expression can begin with `alignas`, so the token now unconditionally selects
the declaration path. Regression:
`cxx_testsuite/tests/exec/0460_block_scope_alignas.cpp`.

### 3.2 Frame slots ignored the requested alignment (partly fixed, still open)

With the parse fixed, `alignas` on an automatic variable still had no effect.
Each backend's `AlignOffset` aligned the slot to `TypeRecordAlignment(type)`
only, ignoring the `aligned` attribute that the frontend had already recorded
in `Symbol::alignment`. A shared `PoolEntryStackAlignment` in
`c_compiler/backend/codegen.c` now supplies the requested alignment to the
x86-64, AArch64, ARM, and RISC-V codegens.

**That is necessary but not sufficient, and the committed change is incomplete.**
`AlignOffset` rounds `var_offset`, which is a *relative* distance measured from
the start of the local area and growing downward from zero, not the address the
object ends up at. The emitted address is the frame pointer minus a base that
depends on the total frame size and on the saved-argument and callee-saved
register areas, so rounding the relative offset only aligns the object when that
base happens to be a multiple of the requested alignment. It is therefore
accidental, and two committed tests fail because of it:

- `cxx_testsuite/tests/exec/0460_block_scope_alignas.cpp` passes at `-O0` on
  x86-64 but returns 4 at `-O2`, where saving `rbx`/`r12`/`r13` into the frame
  shifts the base. A reduced probe shows `alignas(16)` and a 16-byte-aligned
  class landing at `address % 16 == 8` at `-O2` while every request of 8 or
  less is satisfied.
- `cxx_testsuite/tests/exec/0461_stdalign_iso646_headers.cpp` returns 1 on ARM,
  where even `alignas(8)` is missed.

Measuring the frame directly (reading `%rbp` with inline asm and printing its
distance to each slot) shows the slot rounding is *not* the problem. At both
`-O0` and `-O2` the offsets are identical and correctly aligned relative to the
frame pointer — `%rbp - &a16 == 0x30`, `%rbp - &w == 0x20`. What differs is the
frame pointer itself: `%rbp % 16` is 0 at `-O0` and **8** at `-O2`. So
`AlignOffset` is right relative to `%rbp`, and `%rbp` is what is unreliable.

The reason is an ABI bug in the prologue. With `E` as `%rsp` at the first
instruction of the function, `pushq %rbp; leaq 8(%rsp), %rbp;
subq $(stack_frame_size - 8), %rsp` (`SaveRegisters` in
`c_compiler/x86_64/x86_64_emitter.c`, `int remaining = stack_frame_size - 8`)
gives `%rbp == E` and `%rsp == E - stack_frame_size`. `StackFrameSize` rounds to
a multiple of 16, and the ABI makes `E ≡ 8 (mod 16)` because the `call` pushed a
return address, so the body runs with **`%rsp ≡ 8 (mod 16)` where the ABI
requires `≡ 0` at every call**. (The emitted `subq $58` / `subq $78` are hex —
see `PrintAsmImmediate` — i.e. 88 and 120, which cross-checks against the
`// Local vars at offset -96(rbp)` / `-128` comments printed in decimal.)

Every function therefore hands its callees a stack that is 8 off, and since
`%rbp == E`, each callee's `%rbp` residue is decided by whether its caller
happened to violate the guarantee an even or odd number of times up the chain.
Over-aligned locals then work only when two errors cancel, which is exactly the
observed behaviour: at `-O0` `main` leaves `%rsp ≡ 8` so `check` gets
`%rbp ≡ 0` and the 16-aligned offsets come out right, while at `-O2` a different
frame size flips the parity and every 16-byte request lands 8 bytes off.

So the fix is two parts, and the first must come first:

1. Make the prologue keep `%rsp` 16-byte aligned in the function body, so the
   guarantee actually holds and `%rbp` is reliably `≡ 8 (mod 16)`. `remaining`
   has to become a multiple of 16 rather than an odd multiple of 8, and the
   `%rsp`-relative saved-register and spill offsets computed from
   `stack_frame_size` have to move with it. This changes generated code for
   every function on x86-64 and needs the same audit on ARM (where `alignas(8)`
   already fails), AArch64, and RISC-V.
2. Then bias the slot rounding by that known residue, so a request of `A` bytes
   picks `var_offset ≡ 8 (mod A)` rather than `var_offset ≡ 0 (mod A)`. Only
   requests above 8 are affected, which is why `alignas(8)` and below have
   always appeared to work on x86-64.

This is a correctness bug well beyond `alignas`: any callee that relies on the
16-byte guarantee (aligned SSE spills, `movaps` on a stack temporary) is exposed
to it today.

### 3.3 The assembler dropped `.comm` alignment (fixed)

An over-aligned block-scope or file-scope static reached the assembler with a
correct `.comm name,size,align` directive, and `HandleDirective_comm` parsed
the alignment into `AssemblerSymbol::alignment` — but nothing ever read it
back. Both ELF symbol writers passed `sym->value` as `st_value`, which is 0 for
a common symbol. ELF stores a common symbol's required alignment in `st_value`,
and the linker reads it there (`Linker/linker_symbols.c`), falling back to a
size-derived guess when it is 0. So a one-byte `alignas(16)` static was placed
at alignment 1. Fixed by writing the alignment as `st_value` for `SHN_COM`
symbols.

### 3.4 Over-aligned automatic storage beyond 16 bytes (open)

Even once §3.2 computes slot addresses correctly, `alignas` on a local can only
be honored up to the alignment the frame itself guarantees — 16 bytes on
x86-64, 8 on ARM. Beyond that (`alignas(32)`, `alignas(64)`) the object still
lands wherever the frame happens to fall, because no backend performs dynamic
stack realignment. Nothing in the standard headers needs it, so it is deferred,
but it is a silent miscompile rather than a diagnostic and should either be
implemented or diagnosed.

Note for test authors: the exec harness enters at `main` via `-Wl,-e -Wl,main`,
so `main`'s own frame does not get the entry alignment the ABI would otherwise
guarantee, and only 8-byte alignment is observable there. Alignment assertions
belong in a callee.

### 3.5 `_Complex` is unsupported

`error: '_Complex' types are not supported`. The keyword is lexed and then
rejected; there is no complex type in the type system, no arithmetic lowering,
and no backend ABI story for complex return values. This is the single largest
compiler prerequisite in the plan and is why `complex.h`/`tgmath.h` are staged
last. Sequence it as: type-system representation → constant folding and
arithmetic lowering → per-target ABI (return-in-registers vs. sret) → library.

### 3.6 SFINAE is ignored on a defaulted non-type template parameter (fixed)

This gated all of Phase 6. It is fixed; the idiom below now works in every
mode from C++11 up. Regression:
`cxx_testsuite/tests/exec/0463_enable_if_nontype_parameter_sfinae.cpp`.

Both problems concerned the standard pre-C++20 constraint idiom:

```cpp
template <class T, typename enable_if<is_integral<T>::value, int>::type = 0>
int f(T);
```

**Parsing (fixed.)** `ParseTemplateParameter` treated any leading `typename` as
the type-parameter keyword, so the parameter above was read as declaring a type
parameter and the rest of its type produced `Missing >`. `typename` has two
roles in a template parameter list — the type-parameter keyword
(`typename T`, `typename ...Ts`, `typename T = int`) and the disambiguator in a
non-type parameter's type (`typename Dep<U>::type N`) — and only the first is
followed directly by the end of the parameter. `TypenameOpensTypeParameter`
now looks past the keyword and the optional name to tell them apart.
Regression: `cxx_testsuite/tests/exec/0462_dependent_nontype_template_parameter.cpp`.

**Substitution failure (fixed.)** Once it parsed, the parameter still did not
participate in SFINAE: the two overloads below collapsed into
`error: Duplicate definition of symbol which`, because the defaulted non-type
parameter was dropped from the signature and its condition was never evaluated.

```cpp
template <class T, typename enable_if<is_integral<T>::value, int>::type = 0>
int which(T) { return 1; }
template <class T, typename enable_if<!is_integral<T>::value, int>::type = 0>
int which(T) { return 2; }
```

It failed *silently* when there was only one overload: the constraint was
simply ignored, the header still compiled, and the harness still reported a
pass. That is worth remembering when adding coverage here — check the idiom by
running two overloads that only `enable_if` distinguishes, not by checking that
a header compiles.

There were four distinct layers, all of which had to be fixed together.

*Layer 1 — the parameter's type is erased after parsing.* `ParseTemplateParameter`
parsed `typename enable_if<C<T>, int>::type` into the correct representation
(origin `enable_if`, its two template arguments, member name `type`) and then
released its own reference to that record with `TypeRecordDelete`. The
declarator's symbol had already taken the only other reference, so the count
fell back to zero and `TypeRecordDelete` destructed the record in place,
freeing and nulling `dependent_member_name` and `template_arguments` while the
symbol still pointed at it. TypeRecords are never returned to the arena, so the
primitive bits survived and the type merely looked "collapsed" to an `int`
placeholder rather than crashing. A primitive parameter type is unaffected
(its bits live in the record itself), which is why this went unnoticed. The
same hazard is already documented a few lines above for a type parameter's
`default_type`.

*Layer 2 — the type is never substituted, and the overloads are merged.*
`CompleteTemplateArguments` skipped a non-type parameter whose type contains a
template parameter, so nothing evaluated the condition; and on the declaration
side, two function templates whose signatures match were treated as
redeclarations of each other even when their template parameter lists differ,
which produced the duplicate-definition error.
`SubstituteDependentNonTypeParameterTypes` now substitutes each such type
against the completed argument vector and fails the candidate when the
substitution does, and `TryAppendSameSignatureConstrainedTemplateOverload`
admits same-signature overloads whose parameter lists differ.

The declaration-side comparison has one trap. At the point it runs, the new
declaration's parameters have not reached its function type yet —
`MoveCurrentTemplateParametersToFunction` hands them over later — so reading
the type yields an empty list and *every* redeclaration of a function template
looks like a new overload. That is what made `libc/chrono.cc`'s `duration_cast`
split in two and fail with `Ambiguous overload` plus `Function template
definition is required for instantiation`. The parameters are still in
`syntax->current_template_parameters` at that moment, so
`PendingTemplateParameterList` reads them from there.

*Layer 3 — parameters were not in scope for their own list.* This was the real
cause of the silent misbehaviour. `BuildDependentTemplateScopeValueName` treats
a qualified name as dependent only when `current_template_parameter_count > 0`,
and `ParseTemplateDeclaration` did not publish that count until the whole
parameter list had been read. So while parsing `typename enable_if<C<T>,
int>::type`, the compiler believed no template parameters were in scope,
resolved `C<T>::value` against C's *primary* template, and folded the condition
to a constant right there — every instantiation then saw the same answer and no
candidate was ever discarded. Each parameter is now counted as soon as it is
parsed, since a parameter is in scope for the ones that follow it, and
`SyntaxParseTemplateArgumentList` asks that count as well as the
declaration flag before deciding an argument is value-dependent.

*Layer 4 — an enclosing class's pack was not substituted into a member
template's parameter type.* With the condition correctly deferred, a member
template constrained on the enclosing class's parameters started failing with
`template argument pack expansion requires a parameter pack` — which broke
every `std::variant` test, since that is the shape of its converting
constructor (`enable_if_t<__converting_candidate<T, Types...>::value, int>`).
`CopyFunctionTemplateParameters` substituted the enclosing arguments into a
parameter's *default* but only rebased its *type*, so `Types` kept an index
that addresses one of the member's own parameters once the member is renumbered
to a standalone 0-based template. The type now gets the same substitute-then-
rebase treatment the default already had.

Dependent `enable_if` was always fine elsewhere: as a nested typedef, in a
static member initializer, and with a non-dependent condition in a template
parameter list. These other spellings also work and remain reasonable choices:

- `enable_if` in the **return type**. Preferred for free functions.
- `enable_if` as a **defaulted function parameter**.
- **Tag dispatch** through an overloaded helper on `integral_constant`, but see
  the warning below.

**Converting a working `if constexpr` to tag dispatch is not behaviour-preserving.**
`<exception>`'s `throw_with_nested` and `rethrow_if_nested`, and `<memory>`'s
`destroy`, were converted this way and the `<exception>` conversion regressed
`cxx_testsuite/tests/syntax/pass/0408_constexpr_exception_propagation.cpp`
(`No matching overload for nested_error`, then a constexpr evaluator mismatch).
An `if constexpr` branch that is not taken is never instantiated; a tag-dispatch
overload set does not give that guarantee. Both conversions have been reverted.
Making these headers work below C++17 is a compiler job — `if constexpr`
support in older modes — not a header-rewriting job.

### 3.7 `enable_if` cannot hold certain types (open)

Two types are rejected as the second argument of `enable_if`. This mattered a
great deal while §3.6 forced every constraint into the return type; with the
defaulted non-type parameter available it is now avoidable, but the underlying
defects are still there.

**The invoke-result builtin.**

```cpp
typename enable_if<C, __davecc_invoke_result_t(F, Args...)>::type f(F&&, Args&&...);
```

fails with `No matching overload`, though the same builtin works as a plain
return type. Naming it through a typedef first is enough to work around, and
that is what `__invoke_detail::__invoke_result_direct` in `<functional>` is for.

**The injected class name of a partial specialization.**

```cpp
typename enable_if<C, function&>::type operator=(F&&);   // inside function<R(Args...)>
```

fails with `Template argument must name a type` at a bogus location one line
past the end of the translation unit. Referring to the class through its own
`__self` typedef works. The bogus location is worth fixing on its own: it cost
a bisect to find out which declaration was at fault.

**A deduction guide cannot be constrained below C++20 at all.** A guide has no
return type, `enable_if` in its deduced type is rejected, SFINAE is not applied
to the deduced type either (a guide whose deduced type is ill-formed still
competes, giving `Ambiguous class template argument deduction`). Also
`decltype(&F::operator())` is not a substitution failure for a non-class `F`,
so a guide cannot even filter itself: it deduced
`__function_guide<int*>` for `F = int(*)(int)`. `<functional>`'s functor guide
is therefore compiled only from C++20, which costs deduction from a functor
below that but keeps deduction from a function pointer unambiguous.

### 3.8 `is_constructible` ignores a constructor's constraint (open, pre-existing)

`is_constructible<function<int(int)>, F>` is false even for an `F` that
`function` accepts, whether the constructor is constrained by a `requires`
clause or by `enable_if`. The rejecting direction works, so a constraint that
should exclude a type does; it is the accepting direction that is wrong.

This predates the Phase 6 work — the committed `requires`-based `<functional>`
fails the same assertions — but it means `is_constructible` cannot be used to
check that a conversion preserved a constraint. Check the rejecting direction,
which does discriminate.

### 3.9 `__CHAR16_TYPE__` / `__CHAR32_TYPE__` / `__WCHAR_TYPE__` not predefined

`uchar.h` and a correct `wchar.h` need to typedef these consistently between C
and C++ without guessing. Add the predefined macros alongside the existing
`__INT*` set rather than hard-coding `unsigned short` in the headers.

### 3.10 No `-dM -E` support

`davecc -dM -E` does not dump predefined macros, which makes auditing the
preprocessor surface awkward. Worth adding while doing §3.9.

## 4. Integration checklist per header

Every new header must be added to all of:

1. `BUILD.bazel`, the `libc_headers` filegroup (extensionless C++ headers are
   listed explicitly; `*.h` files are covered by the glob).
2. `libc/modules/std.hpp`, with the right `__cplusplus` gate.
3. `libc/include/version`, for the feature-test macros it defines.

Plus an exec test in `cxx_testsuite/tests/exec/` (return-code based — see the
`run-cxx-exec-tests` skill; there is no C runtime init, so tests must not rely
on stdout) and, for C headers, a `c_testsuite` or `libc/tests/runtime` test.

## 5. Validation

Per header: fast single-file compile-and-run loop from the `run-cxx-exec-tests`
skill.

Per phase: `bazel test //cxx_testsuite:exec_x86_64` plus the AArch64, ARM, and
RISC-V variants, and `libc/tests/compile_all.sh` for every target including
65C02 and P-code.
