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
  `integral_constant<bool, ...>`, as already done in `<exception>` and
  `<memory>`.
- A `requires` clause in a pre-C++20 header becomes `enable_if` SFINAE — but
  see §3.6 for which spelling to use, because the obvious one does not work.
- A header from a later standard that is pulled in transitively gets its body
  wrapped in `#if __cplusplus >= <its standard>` so that it stays includable
  but empty, as already done in `<concepts>` and `<memory_resource>`.

Fixed so far: `<exception>`, `<memory>`, `<concepts>`, `<memory_resource>`.

Remaining blockers, in the order the harness reports them:

| Blocker | Nature |
| --- | --- |
| `<iterator>` concept definitions | C++20 API in a C++98 header; guard on C++20 |
| `<functional>` `requires` clauses | 8 outside the C++23 block; needs §3.6 |
| `<bit>` `requires` clauses | `<limits>` pulls `<bit>` in below C++20 |
| `<any>` `requires` clauses | C++17 header, so must not use `requires` |
| `<optional>` `requires` clauses | C++17 header, so must not use `requires` |
| `<tuple>` `if constexpr` | C++11 header; needs an explicit return type, not `auto&` |

`<tuple>` is the one that is not mechanical: `__get<I>()` returns `auto&` and
selects the member with `if constexpr`, so a pre-C++14 version needs the return
type spelled through `tuple_element` rather than deduced.

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

### 3.2 Frame slots ignored the requested alignment (fixed)

With the parse fixed, `alignas` on an automatic variable still had no effect.
Each backend's `AlignOffset` aligned the slot to `TypeRecordAlignment(type)`
only, ignoring the `aligned` attribute that the frontend had already recorded
in `Symbol::alignment`. Fixed with a shared `PoolEntryStackAlignment` in
`c_compiler/backend/codegen.c`, used by the x86-64, AArch64, ARM, and RISC-V
codegens.

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

After §3.2 and §3.3, `alignas` on a local is honored up to the alignment the
frame itself guarantees — 16 bytes on x86-64. Beyond that (`alignas(32)`,
`alignas(64)`) the object still lands wherever the frame happens to fall,
because no backend performs dynamic stack realignment. Nothing in the standard
headers needs it, so it is deferred, but it is a silent miscompile rather than
a diagnostic and should either be implemented or diagnosed.

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

### 3.6 SFINAE is ignored on a defaulted non-type template parameter (open)

This one gates all of Phase 6, so read it before converting any `requires`
clause.

Two problems, one fixed and one open. Both concern the standard pre-C++20
constraint idiom:

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

**Substitution failure (open.)** The parameter now parses, but it does not
participate in SFINAE: the two overloads below collapse into
`error: Duplicate definition of symbol which`, because the defaulted non-type
parameter is dropped from the signature and its condition is never evaluated.

```cpp
template <class T, typename enable_if<is_integral<T>::value, int>::type = 0>
int which(T) { return 1; }
template <class T, typename enable_if<!is_integral<T>::value, int>::type = 0>
int which(T) { return 2; }
```

Beware that this fails *silently* when there is only one overload: the
constraint is simply ignored, the header still compiles, and the harness still
reports a pass. Any conversion to this spelling must be checked by running two
overloads that only `enable_if` distinguishes, not by checking that the header
compiles.

Until it is fixed, use one of the spellings that were verified to work
correctly (each was checked by running the two-overload discrimination test
above, not merely compiling it):

- `enable_if` in the **return type** — works. Preferred for free functions.
- `enable_if` as a **defaulted function parameter** — works. The only option
  for constructors, which have no return type.
- **Tag dispatch** through an overloaded helper on `integral_constant` — works.
  Preferred where the condition selects between two implementations rather
  than removing an overload.

Note that dependent `enable_if` is fine everywhere else: as a nested typedef,
in a static member initializer, and with a non-dependent condition in a
template parameter list. It is specifically the combination of a dependent
condition with a defaulted non-type template parameter that is dropped.

### 3.7 `is_class`-style traits in a function parameter type (open)

```cpp
template <class U>
int f(U, typename enable_if<is_integral<U>::value, int>::type = 0);
```

compiles, but the same with a builtin-backed trait reports
`Class template instantiation is not supported yet` from `<type_traits>`.
Worth pinning down alongside §3.6 since both block the same conversions.

### 3.8 `__CHAR16_TYPE__` / `__CHAR32_TYPE__` / `__WCHAR_TYPE__` not predefined

`uchar.h` and a correct `wchar.h` need to typedef these consistently between C
and C++ without guessing. Add the predefined macros alongside the existing
`__INT*` set rather than hard-coding `unsigned short` in the headers.

### 3.9 No `-dM -E` support

`davecc -dM -E` does not dump predefined macros, which makes auditing the
preprocessor surface awkward. Worth adding while doing §3.8.

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
