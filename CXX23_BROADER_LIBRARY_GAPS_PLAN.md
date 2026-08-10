# C++23 broader standard-library gaps implementation plan

Implement this plan in order. Compiler defects exposed by conforming library
code must be fixed at their source; do not add library or test workarounds.
Preserve existing behavior and run focused regressions before moving to the
next step.

Supported execution targets for `<filesystem>` and `<chrono>`:

- x86-64
- AArch64
- ARM
- RISC-V
- 65C02/6502
- P-code

P-code and 65C02 are resource-constrained but are supported targets, not
compile-only or `ENOTSUP` profiles. Time-zone data should remain host-side and
be queried lazily so these targets do not need to materialize the complete IANA
database in guest memory.

## Current handoff

The working tree already contains an in-progress `<filesystem>` implementation:

- `c_compiler/Loader/filesystem_host.{h,c}` defines host filesystem services.
- x86-64, AArch64, ARM, and RISC-V have filesystem syscall dispatch in progress.
- `libc/include/filesystem` and `libc/filesystem.cc` implement most public APIs.
- `cxx_testsuite/tests/exec/0327_standard_filesystem.cpp` is the focused test.
- `BUILD.bazel`, `libc/include/version`, and `libc/modules/std.hpp` have partial
  integration.

Do not replace `make_shared` in the directory iterators with direct allocation.
The current directory-iterator failure exposes a compiler bug:

- A `make_shared<T>(class_ref, scalar_ref, class_ref)` specialization returning
  `shared_ptr<T>` receives its hidden structure-return pointer in `%rdi`.
- The first source argument should arrive in `%rsi`, but generated x86-64 code
  reads an uninitialized local at `-32(%rbp)`.
- Later arguments arriving in `%rdx` and `%rcx` are saved correctly.
- Direct `shared_ptr<T>(new T(...))` hides the defect and is therefore not an
  acceptable fix.

Add an independent regression for this forwarding-pack/class-reference case,
then repair template-body symbol remapping, reference materialization, or
backend argument classification according to where the first incorrect IR
appears. The expanded identifier must reference the instantiated function's
formal argument symbol and remain `is_argument` with the correct `arg_number`.

The existing filesystem implementation currently has explicit
`__6502__`/`__p_code__` unsupported branches. Remove them as the corresponding
runtime services become available.

## Step 1: Stabilize the cross-target syscall ABI

- [ ] Define one stable set of guest operation numbers in
      `libc/include/syscall.h` for filesystem, clocks, and time-zone queries.
- [ ] Keep all wire records fixed-width and explicitly laid out. Do not expose
      host `stat`, `dirent`, `timespec`, or native pointer-sized fields.
- [ ] Return 64-bit values through output pointers where necessary. This is
      required for 65C02, whose ordinary syscall return type cannot represent
      file sizes or microsecond timestamps.
- [ ] Pass resize sizes and modification timestamps by pointer on every target,
      preserving one ABI shape across 16-, 32-, and 64-bit guests.
- [ ] Validate null pointers, unterminated paths, output capacities, integer
      ranges, and guest-memory boundaries before calling host services.
- [ ] Keep host-to-guest errno translation centralized and identical across all
      architectures.
- [ ] Ensure directory handles are bounded integer tokens rather than host
      pointers and are closed exactly once on every iterator/error path.

### Architecture dispatch

- [ ] x86-64: complete syscall constants and dispatch in
      `x86_64_interpreter/x86_64_syscalls.{h,c}`.
- [ ] AArch64: complete syscall constants and dispatch in
      `aarch64_interpreter/aarch64_syscalls.{h,c}`.
- [ ] ARM: complete syscall constants, guest-pointer resolution, and dispatch
      in `arm_interpreter/arm_interpreter.{h,c}`.
- [ ] RISC-V: complete ecall constants, guest-pointer resolution, and dispatch
      in `risc_v_interpreter/risc_v_interpreter.{h,c}`.
- [ ] 65C02: add BRK operation numbers and handlers in
      `6502_interpreter/6502_interpreter.{h,c}` and the runtime bridge in
      `6502 support/6502runtime.s`.
- [ ] P-code: add an escape/syscall bridge in
      `p_code_interpreter/p_code_interpreter.{h,c}` and marshal guest arguments
      in `libc/syscall.c`.
- [ ] Add `:filesystem_host` and the chrono host service dependency to
      `6502_lib` and `p_code_lib` in `BUILD.bazel`.

The existing 65C02 libc syscall constants and interpreter BRK constants do not
currently agree for `lseek`, `write`, `read`, and `abort`. Normalize this ABI
and add regressions before allocating new operation numbers.

P-code currently implements only a few direct escapes; generic `syscall()`,
`open()`, and `lseek()` are mostly stubs. Implement real marshalling rather than
special-casing `<filesystem>` in the C++ library. At minimum, P-code must support
the ordinary file operations needed to read TZif files as well as all new
filesystem and clock operations.

**Gate:** Build every interpreter and libc archive, then run focused C syscall
tests that exercise each operation on all six targets.

## Step 2: Fix the forwarding-pack compiler regression

- [ ] Add a focused C++ execution test independent of `<filesystem>`. It must:
  - return a non-trivial class such as `shared_ptr<T>` from a variadic template;
  - forward a `const` reference to a non-trivial class as the first pack element;
  - include later scalar and class references;
  - verify the constructed object observes the original values.
- [ ] Inspect the cloned AST and saved IR before changing the backend.
- [ ] If the expanded identifier points to the template pattern symbol or a
      temporary local, repair pack expansion and formal-symbol mapping in
      `c_compiler/frontend/syntax/type_template_clone.c`.
- [ ] If semantic analysis materializes an addressable class lvalue, repair the
      reference-binding logic in
      `c_compiler/frontend/semantic/expr_semantics.c`.
- [ ] If the IR is correct but lowering loses the first parameter after a hidden
      structure return, repair argument location/prologue handling in every
      affected backend, not only x86-64.
- [ ] Verify generated x86-64 code saves/uses `%rsi` for the first explicit
      parameter when `%rdi` is the hidden result pointer.
- [ ] Run existing template, `shared_ptr`, `move_only_function`, flat-container,
      syntax, and module regressions.

**Gate:** Restore `make_shared` in filesystem iterators and confirm the focused
compiler regression and existing smart-pointer tests pass.

## Step 3: Complete the filesystem host runtime

- [ ] Finish and audit `c_compiler/Loader/filesystem_host.{h,c}`.
- [ ] Cover status/lstatus, directory open/read/close, create/remove/rename,
      current path, symlinks, hard links, permissions, resize, timestamps,
      space, copy, and canonicalization.
- [ ] Skip `.` and `..` in host directory traversal.
- [ ] Detect names that do not fit the guest-provided buffer and return
      `ENAMETOOLONG`; never silently truncate.
- [ ] Clamp or reject host values that cannot be represented by wire fields.
- [ ] Make directory handle allocation and teardown thread-safe and retry-safe.
- [ ] Preserve symlink-following semantics separately for `status` and
      `symlink_status`.
- [ ] Ensure every host failure maps to a stable guest `errc` value.
- [ ] Add host-unit tests for success, missing paths, invalid handles, long
      names, non-empty directories, symlink loops, cross-device errors where
      reproducible, and cleanup after early return.

## Step 4: Complete `std::filesystem`

- [ ] Finish the complete C++17 filesystem API surface required by C++23 in
      `libc/include/filesystem` and `libc/filesystem.cc`.
- [ ] Audit `path` construction, decomposition, iteration, comparison,
      concatenation, lexical normalization, relative/proximate operations, and
      hashing.
- [ ] Preserve POSIX root-directory semantics consistently on every target.
- [ ] Complete `directory_entry` caching and refresh behavior.
- [ ] Complete `directory_iterator` and `recursive_directory_iterator`,
      including shared-state lifetime, postfix increment, recursion depth,
      `pop`, `disable_recursion_pending`, permission-denied behavior, and end
      iterator comparisons.
- [ ] Ensure iterator copies share traversal state exactly as required and all
      directory handles close on normal completion, errors, assignment, and
      destruction.
- [ ] Complete `copy`/`copy_file` option validation and recursive behavior.
- [ ] Ensure `remove_all` never follows directory symlinks and returns the exact
      number of removed entries.
- [ ] Complete throwing and `error_code` overload symmetry. Throwing overloads
      must populate `filesystem_error::path1()` and `path2()` correctly.
- [ ] Verify file times, permission masks, hard-link counts, space values,
      equivalence, weak canonicalization, and temporary-directory selection.
- [ ] Keep memory use bounded on 65C02: avoid fixed kilobyte-scale automatic
      buffers in recursive traversal and allocate path/name storage only as
      needed.
- [ ] Define `__cpp_lib_filesystem` as `201703L`, export through `import std`,
      and include all sources/headers in every libc build.

Expand `0327_standard_filesystem.cpp` and add smaller target-focused tests so
failures are attributable. Tests must cover lexical-only behavior separately
from host operations and must use unique temporary paths with reliable cleanup.

**Gate:** Run focused filesystem tests on all six targets, then the full hosted
execution and module suites on x86-64, AArch64, ARM, and RISC-V.

## Step 5: Complete chrono clocks, durations, and civil calendar

- [ ] Add a host chrono service, preferably
      `c_compiler/Loader/chrono_host.{h,c}`, using fixed-width wire types.
- [ ] Expose realtime and monotonic timestamps at microsecond or nanosecond
      precision through output pointers.
- [ ] Implement clock dispatch for all six interpreters.
- [ ] Replace the deterministic 65C02/P-code steady-clock fallback with the host
      monotonic clock. Thread support may remain unavailable; clock support must
      be controlled by a separate capability macro.
- [ ] Make `system_clock::now()` use the realtime high-resolution service rather
      than whole-second `time(nullptr)`.
- [ ] Complete duration arithmetic, comparisons, conversions, `floor`, `ceil`,
      `round`, `abs`, `common_type`, and overflow-sensitive ratio handling.
- [ ] Complete `system_clock`, `steady_clock`, `file_clock`, `utc_clock`,
      `tai_clock`, and `gps_clock` conversions required by the supported C++23
      API.
- [ ] Implement `hh_mm_ss`.
- [ ] Implement calendar types and constants: `day`, `month`, `year`,
      `weekday`, indexed/last forms, `year_month`, `month_day`, `month_day_last`,
      `month_weekday`, `month_weekday_last`, `year_month_day`,
      `year_month_day_last`, `year_month_weekday`, and
      `year_month_weekday_last`.
- [ ] Implement civil-date validation, arithmetic, leap-year handling, and
      `sys_days`/`local_days` conversions across negative and positive epochs.
- [ ] Add constexpr boundary tests for leap years, month rollover, weekdays,
      invalid dates, rounding ties, and clock conversions.

## Step 6: Implement the TZif-backed time-zone database

Do not parse and retain the entire database in 65C02 guest memory. Parse/cache
TZif data in the host runtime and expose stable query syscalls.

- [ ] Implement TZif v1/v2/v3 parsing with strict bounds checks.
- [ ] Add host queries for database version, current zone, zone count/name,
      sys-time transition lookup, local-time ambiguity/nonexistence resolution,
      leap-second data, and reload generation.
- [ ] Keep returned guest records fixed-width; copy zone names and
      abbreviations into caller-provided buffers with explicit capacities.
- [ ] Make host caches immutable or synchronized and make `reload_tzdb`
      idempotent.
- [ ] Implement `sys_info`, `local_info`, `choose`, `time_zone`,
      `time_zone_link`, `leap_second`, `tzdb`, `tzdb_list`, `get_tzdb`,
      `get_tzdb_list`, `reload_tzdb`, `remote_version`, `locate_zone`,
      `current_zone`, and `zoned_time`.
- [ ] Correctly handle ambiguous and nonexistent local times and throw the
      standard exceptions with useful diagnostics.
- [ ] Provide deterministic UTC behavior when the host has no zone database,
      while reporting unavailable named zones through standard errors.
- [ ] Add tests using checked-in minimal TZif fixtures so results do not depend
      exclusively on the developer machine's `/usr/share/zoneinfo`.
- [ ] Test transition boundaries, non-hour offsets, DST gaps/folds, aliases,
      leap seconds, current-zone fallback, reload, and concurrent queries.
- [ ] Run the same logical tests on all six targets, with compact targeted cases
      for 65C02.

## Step 7: Complete locale, format, and print

The repository currently lacks a complete `<locale>` header. Implement the
locale functionality required by standard formatting rather than embedding
locale behavior directly in `<format>`.

- [ ] Add the required locale core, facets, `locale::id`, `locale::facet`,
      `use_facet`, `has_facet`, `numpunct`, and character classification support.
- [ ] Complete compile-time format-string validation and runtime `vformat`
      behavior.
- [ ] Complete alignment, fill, sign, alternate form, zero padding, width,
      precision, dynamic arguments, integer, floating-point, pointer, character,
      string, and escaped/debug formatting.
- [ ] Implement localized numeric formatting through locale facets.
- [ ] Implement C++23 range formatting, including strings, maps, sets,
      sequences, tuples, pairs, nested ranges, and format-kind overrides.
- [ ] Implement chrono formatting for durations, calendar values, clocks,
      `zoned_time`, offsets, abbreviations, and locale-sensitive names.
- [ ] Implement Unicode-aware width and precision without splitting UTF-8 code
      points or grapheme-relevant combining sequences required by the standard.
- [ ] Complete `format`, `format_to`, `format_to_n`, `formatted_size`,
      `vformat`, and locale overloads.
- [ ] Complete `<print>` Unicode and non-Unicode APIs, `FILE*` overloads,
      newline behavior, write-error reporting, and synchronization requirements.
- [ ] Add the correct C++23 feature-test macros in `libc/include/version`.
- [ ] Export `<locale>`, `<format>`, and `<print>` through `libc/modules/std.hpp`.

## Step 8: Integration and validation

- [ ] Add every new header and source to `BUILD.bazel` and all target libc
      archives.
- [ ] Add focused syntax tests for every compiler defect fixed during the work.
- [ ] Add execution tests for filesystem, chrono calendar, clocks, time zones,
      locale, format ranges/chrono/Unicode, and print.
- [ ] Add module coverage for every completed header.
- [ ] Add dedicated 65C02 and P-code filesystem/chrono smoke targets if the
      general C++ execution harness cannot run those profiles.
- [ ] Run:
  - `bazel test //cxx_testsuite:syntax --test_output=errors`
  - `bazel test //cxx_testsuite:modules_{x86_64,aarch64,arm,riscv} --test_output=errors`
  - `bazel test //cxx_testsuite:exec_{x86_64,aarch64,arm,riscv} --test_output=errors`
  - `bazel test //cxx_testsuite:exec_format_{x86_64,aarch64,arm,riscv} --test_output=errors`
  - all libc compile/runtime tests for x86-64, AArch64, ARM, RISC-V, P-code,
    and 65C02;
  - the new dedicated P-code and 65C02 filesystem/chrono tests.
- [ ] Re-run focused smart-pointer, template-pack, defaulted-special-member,
      conditional-expression, inline-linkage, range-for, and preprocessor
      regressions introduced by the in-progress filesystem work.
- [ ] Perform a critical review of the full staged and unstaged diff, checking
      lifetime, cleanup, handle symmetry, error paths, retry safety, cache
      synchronization, and cross-target wire-layout assumptions.
- [ ] Do not commit unless explicitly requested.

## Completion criteria

This work is complete only when:

1. `<filesystem>` and `<chrono>` execute real host-backed behavior on all six
   targets, including 65C02 and P-code.
2. No target uses an `ENOTSUP`, fake clock, or direct-allocation workaround for
   required functionality.
3. Time-zone lookup is TZif-backed and memory-bounded on constrained guests.
4. `<format>` and `<print>` cover locale, ranges/tuples, chrono, and Unicode.
5. Feature macros and `import std` expose the completed APIs.
6. Focused tests and the full applicable validation matrix pass.
