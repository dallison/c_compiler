# Remaining C++20 and C++23 plan

Updated from the C++23 readiness canvas on 2026-07-28.

The original C++20 correctness gates are now green on x86-64: the main
execution suite passes 261/261 and the ranges suite passes 4/4. C++23 mode,
template-template parameters, static call/subscript operators, `if consteval`,
explicit object parameters, `std::generator`, `<format>`, `<print>`, initial
`import std`, `<span>`, multidimensional subscripts, and `<mdspan>` have been
implemented.

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
- [x] Support fold expressions involving multiple parameter packs.
  - Simultaneously expand every pack referenced by a fold pattern in
    `c_compiler/frontend/syntax/type_template_clone.c`.
  - Diagnose mismatched pack lengths and cover mixed fold patterns,
    associativity, empty seeded folds, and mismatched packs.
- [x] Audit remaining `constexpr` and `consteval` restrictions.
  - Constant-evaluate aggregate arguments/returns and same-type static aggregate
    initializers.
  - Support non-throwing `try` paths, deeper recursive AST evaluation, immediate
    lambdas, same-array pointer ordering, and active-union-member tracking.
  - Reject reached throws, out-of-bounds array pointers, inactive union reads,
    and `constexpr`/`consteval` coroutines with focused diagnostics.
  - Advertise the implemented C++20 `__cpp_constexpr` feature level.
- [x] Audit and complete coroutine suspension/lifetime cases.
  - Persist class and reference parameters even when only initial/final suspend
    requires a frame, and retain parameter copies through exceptional completion
    until frame destruction.
  - Destroy frame-backed non-trivial locals on `break` and `continue`, including
    early frame destruction and exception paths.
  - Support class `for` initializers across suspension by preserving their full
    loop scope; cover scalar/range-for state and member-coroutine `this`.

## Remaining C++20 library work

- [x] Implement `std::counted_iterator`.
- [x] Implement `std::common_iterator`.
- [x] Harden `common_view`, `views::counted`, and related iterator/ranges
      interoperability after those iterator types exist.
- [x] Complete the atomics surface.
  - `atomic_ref`.
  - `wait`, `notify_one`, and `notify_all`.
  - Backend and freestanding-profile audit, with hosted support on x86-64,
    AArch64, ARM, and RISC-V; 65(C)02 intentionally diagnoses atomics because
    its embedded profile has no threading support.
- [x] Add the standard threading surface where supported.
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

- [x] Implement multidimensional subscript expressions and overloads
      (`object[i, j]`), distinct from the completed static `operator[]` work.
- [ ] Implement `auto(x)` and `auto{x}` decay-copy syntax.
- [ ] Implement the `#warning` preprocessing directive.
- [ ] Audit the remaining low-coupling C++23 language proposals and add explicit
      feature tests or unsupported diagnostics rather than silently accepting
      partial behavior.

## Remaining C++23 library work

- [x] Implement `<mdspan>`.
  - `extents`, `dextents`, and `dynamic_extent` integration.
  - `layout_left`, `layout_right`, and `layout_stride`.
  - `default_accessor` and `mdspan`.
  - Construction from arrays and spans.
  - Compile-time mapping tests and execution tests on all backends.
- [x] Implement the C++23 ranges expansion.
  - [x] `views::zip`, `zip_transform`, and `enumerate`.
    - Includes variadic tuple support, standard callable constraints, range-for
      structured bindings, and recursive tuple access.
    - Syntax, the full x86-64 C++ execution suite, and all four dedicated ranges
      execution targets pass.
  - [x] `views::adjacent` and `adjacent_transform`.
    - Includes `pairwise` aliases, non-common sentinel support, reference
      windows, callable transforms, and direct and piped adaptor forms.
    - Syntax, the full x86-64 C++ execution suite, and all four dedicated ranges
      execution targets pass.
  - [x] `views::chunk`, `slide`, `chunk_by`, and `stride`.
    - Includes partial trailing chunks, overlapping runtime windows, predicate
      grouping, stepped iteration, and non-common sentinel support.
    - Syntax, the full x86-64 C++ execution suite, and all four dedicated ranges
      execution targets pass.
  - [x] `views::join_with`, `repeat`, and `cartesian_product`.
    - Includes scalar and range delimiters, bounded and unbounded repetition,
      variadic odometer-order products, empty products, reference tuples, and
      non-common counted-range integration.
    - Syntax, the full x86-64 C++ execution suite, and all four dedicated ranges
      execution targets pass.
  - [x] `ranges::to`, range fold algorithms, and the new
    contains/prefix/suffix algorithms.
    - Includes explicit and deduced container conversion, pipe closures,
      nested conversion, non-common input views, all six C++23 fold forms,
      projections, and empty-range behavior.
    - Syntax, the full x86-64 C++ execution suite, and all four dedicated ranges
      execution targets pass.
- [ ] Expand `import std`.
  - Export each supported standard header, including ranges, generator, span,
    format, and print.
  - Fix imported class-template members, deduction guides, partial
    specializations, and overload sets before exposing affected headers.
  - Add useful container, string, span, format, and ranges execution coverage;
    the current smoke test covers `std::array`, stop tokens, and `std::mdspan`.
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
- [x] 2. Finish the C++20 language tail: multi-pack folds and the
      coroutine/constexpr audit.
- [x] 3. Add `counted_iterator` and `common_iterator`.
- [x] 4. Implement `<mdspan>` now that `<span>` exists.
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
