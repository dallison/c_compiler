# C++23 `mdspan` implementation plan

Implement one step at a time. Run the step's focused and regression tests, update
this checklist, and stop for review before beginning the next step. Compiler
defects exposed by the library implementation must be fixed at their source
rather than hidden behind library workarounds.

## Current handoff

Steps 1 through 3 are complete. The next agent should begin with Step 4 (the
`mdspan` view) and stop after that step for review.

Step 3 validation completed successfully with:

- `bazel test //cxx_testsuite:syntax --test_output=errors`
- `bazel test //cxx_testsuite:exec_x86_64 --test_output=errors`

## Step 1: Multidimensional subscript language support

- [x] Parse C++23 `object[i, j, ...]` as an argument list while preserving a
      parenthesized comma expression as one argument.
- [x] Resolve member `operator[]` overloads with zero or multiple arguments,
      including static operators and dependent variadic templates.
- [x] Reject multi-argument built-in array/pointer subscripting and enforce
      pre-C++23 `operator[]` arity rules.
- [x] Define `__cpp_multidimensional_subscript` as `202211L` in C++23 mode.
- [x] Add syntax, constexpr, template, and execution regressions.
- [x] Run the focused tests and broad syntax/execution regression suites.

## Step 2: Extents and access policy

- [x] Add `<mdspan>` and define `__cpp_lib_mdspan` as `202207L`.
- [x] Implement shared `dynamic_extent` handling.
- [x] Implement `extents` and `dextents`, including static/dynamic observers,
      converting construction, equality, and array/span construction.
- [x] Implement `default_accessor`.
- [x] Add focused compile-time and execution tests.

## Step 3: Layout mappings

- [x] Implement `layout_right::mapping`.
- [x] Implement `layout_left::mapping`.
- [x] Implement `layout_stride::mapping`.
- [x] Cover zero rank, zero extents, mixed static/dynamic extents, strides,
      conversions, and mapping properties.

## Step 4: `mdspan` view

- [ ] Implement the standard aliases, constructors, conversions, and deduction
      guides.
- [ ] Implement multidimensional element access and all core observers.
- [ ] Cover pointer, array, span, mapping, accessor, CTAD, conversion, and
      custom-policy use cases.

## Step 5: Integration and completion

- [ ] Add permanent C++23 execution coverage on x86-64, AArch64, ARM, and
      RISC-V.
- [ ] Export `<mdspan>` through `libc/modules/std.hpp` and run all module suites.
- [ ] Fix any importer, template, constexpr, serialization, or backend defects
      uncovered by integration.
- [ ] Run the complete syntax and hosted execution matrices.
- [ ] Mark multidimensional subscripts and `<mdspan>` complete in
      `CXX20_CXX23_REMAINING.md` and synchronize its recommended-order list.
- [ ] Perform a final full-diff review before commit.
