# Remaining C++20 and C++23 plan

Updated from the C++23 readiness canvas on 2026-07-28.

The original C++20 correctness gates are now green on x86-64: the main
execution suite passes 259/259 and the ranges suite passes 4/4. C++23 mode,
template-template parameters, static call/subscript operators, `if consteval`,
explicit object parameters, `std::generator`, `<format>`, `<print>`, initial
`import std`, and `<span>` have been implemented.

## Immediate validation gates

- [x] Diagnose the AArch64 failure in
      `cxx_testsuite/tests/exec/0224_cxx_local_static_thread_safe.cpp`.
- [x] Characterize the RISC-V full-suite timeout and separate slow tests from
      hangs or correctness failures.
- [x] Confirm the default CI path runs the main, ranges, format, coroutine, and
      module suites on every primary backend.
- [x] Re-run the broader ranges/view compile sweep and convert any remaining
      useful probes into permanent tests.

## Remaining C++20 language work

- [x] Implement complete `char8_t` and `u8` literal semantics.
  - Give UTF-8 character and string literals the correct semantic types.
  - Add semantic, overload-resolution, template-deduction, and execution tests.
  - Audit the advertised `__cpp_char8_t` feature macro against actual support.
- [ ] Support fold expressions involving multiple parameter packs.
  - Remove the unsupported path in
    `c_compiler/frontend/syntax/type_template_clone.c`.
  - Add positive and diagnostic coverage for mixed and mismatched packs.
- [ ] Audit remaining `constexpr` and `consteval` restrictions.
  - Separate intentional standard diagnostics from evaluator limitations.
  - Cover aggregate arguments, recursion, exceptions, and coroutine interaction.
- [ ] Audit and complete coroutine suspension/lifetime cases.
  - Class parameters and non-trivial locals live across suspension.
  - `for` initializer lifetimes across suspension.
  - `constexpr`/`consteval` coroutine diagnostics.

## Remaining C++20 library work

- [ ] Implement `std::counted_iterator`.
- [ ] Implement `std::common_iterator`.
- [ ] Harden `common_view`, `views::counted`, and related iterator/ranges
      interoperability after those iterator types exist.
- [ ] Complete the atomics surface.
  - `atomic_ref`.
  - `wait`, `notify_one`, and `notify_all`.
  - Backend and freestanding-profile audit, especially 65C02 and RISC-V.
- [ ] Add the standard threading surface where supported.
  - `<thread>` and `std::thread`.
  - `jthread` and stop tokens.
  - Semaphores, latches, barriers, and condition variables.
  - Document intentionally unsupported embedded-target profiles.
- [ ] Add shared ownership and polymorphic allocation.
  - `shared_ptr`, `weak_ptr`, and `enable_shared_from_this`.
  - `memory_resource` and the core PMR aliases.
- [ ] Implement `<system_error>` and `<filesystem>`.
- [ ] Complete `<chrono>`.
  - Calendar and civil-time types.
  - Time-zone support where the target profile permits it.
  - Chrono formatting integration.

## Remaining C++23 language work

- [ ] Implement multidimensional subscript expressions and overloads
      (`object[i, j]`), distinct from the completed static `operator[]` work.
- [ ] Implement `auto(x)` and `auto{x}` decay-copy syntax.
- [ ] Implement the `#warning` preprocessing directive.
- [ ] Audit the remaining low-coupling C++23 language proposals and add explicit
      feature tests or unsupported diagnostics rather than silently accepting
      partial behavior.

## Remaining C++23 library work

- [ ] Implement `<mdspan>`.
  - `extents`, `dextents`, and `dynamic_extent` integration.
  - `layout_left`, `layout_right`, and `layout_stride`.
  - `default_accessor` and `mdspan`.
  - Construction from arrays and spans.
  - Compile-time mapping tests and execution tests on all backends.
- [ ] Implement the C++23 ranges expansion.
  - `views::zip`, `zip_transform`, and `enumerate`.
  - `views::adjacent` and `adjacent_transform`.
  - `views::chunk`, `slide`, `chunk_by`, and `stride`.
  - `views::join_with`, `repeat`, and `cartesian_product`.
  - `ranges::to`, range fold algorithms, and the new contains/prefix/suffix
    algorithms.
- [ ] Expand `import std`.
  - Export each supported standard header, including ranges, generator, span,
    format, and print.
  - Fix imported class-template members, deduction guides, partial
    specializations, and overload sets before exposing affected headers.
  - Add useful container, string, span, format, and ranges execution coverage;
    the current smoke test primarily proves `std::array`.
- [ ] Complete `<format>` and `<print>` beyond the initial narrow implementation.
  - Locale-aware formatting.
  - Unicode printing and `vprint_unicode`.
  - Standard formatters for chrono, filesystem paths, and other library types.
  - Wide-character and UTF-8 contexts if those are in the supported profile.
- [ ] Implement `<expected>`.
- [ ] Implement `<flat_map>` and `<flat_set>`.
- [ ] Implement `move_only_function`, `bind_back`, and remaining C++23
      functional utilities.
- [ ] Decide target support for `<stacktrace>` and `<stdfloat>`, then implement
      or document them as profile exclusions.

## Recommended order

- [x] 1. Close the AArch64 and RISC-V validation gaps.
- [ ] 2. Finish the C++20 language tail: multi-pack folds and the
      coroutine/constexpr audit.
- [ ] 3. Add `counted_iterator` and `common_iterator`.
- [ ] 4. Implement `<mdspan>` now that `<span>` exists.
- [ ] 5. Build the C++23 ranges expansion on the completed iterator foundation.
- [ ] 6. Expand `import std` incrementally as module serialization becomes safe
      for each header.
- [ ] 7. Complete format/print, threading, atomics, memory ownership, chrono,
      filesystem, and the independent C++23 library headers.

## Completion criteria

- [x] The main execution, ranges, format, coroutine, and module suites pass on
      x86-64, AArch64, ARM, and RISC-V.
- [ ] Every advertised language and library feature macro has corresponding
      positive and negative tests.
- [ ] Unsupported embedded-target features have explicit documented profiles
      and useful diagnostics.
- [ ] `import std` exposes the supported standard-library profile without
      importer crashes, incorrect template instantiation, or runtime corruption.
