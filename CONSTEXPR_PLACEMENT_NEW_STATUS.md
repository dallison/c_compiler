# C++26 constexpr placement new — handoff

## Repository state

- Branch: `master`
- Implementation commit: `7565f7f` (`Implement C++26 constexpr placement new.`)
- Branch was 11 commits ahead of `origin/master` immediately after the commit.
- The implementation was fully committed before this handoff file was created.
- This handoff file is intentionally uncommitted.

## Completed work

- Added C++26 new-expression metadata and preserved it through template cloning.
- Added constexpr placement-new evaluation with allocated-type, provenance,
  storage-bound, array-bound, and lifetime validation.
- Added typed `void*` conversions under the C++26 constant-expression rules.
- Added object-lifetime tracking for `destroy_at`, explicit destruction, and
  placement reconstruction.
- Added AST overlays for PCode cases that need richer provenance information.
- Moved PCode failure kind and diagnostic reason into `ConstEvalContext`; there
  is no global PCode failure state and no diagnostic-string scraping.
- Added structured handling for unsupported PCode operations versus definitive
  invalid constant-expression failures.
- Added C++26 feature-test macros and constexpr placement declarations.
- Added `std::construct_at`, `std::destroy_at`, and constexpr allocator helpers.
- Added positive, negative, feature-macro, lifetime, array-placement, allocator,
  and runtime regressions.

## Validation completed

All of the following passed after the final changes:

```sh
bazel build //:davecc
bazel test //cxx_testsuite:lexical //cxx_testsuite:syntax --test_output=errors
bazel test //cxx_testsuite:exec_x86_64 --test_output=errors
```

Additional focused checks passed:

- `0441_cxx26_constexpr_placement_new.cpp` in AST and PCode evaluator modes.
- `0441_cxx26_constexpr_placement_new.cpp` for the 6502 target.
- Placement-new runtime test `0058_placement_new.cpp`.
- Existing PCode heap leak, invalid-free, and pointer-escape diagnostics.
- `git diff --check` and IDE lints for the modified compiler files.

## Current status

The planned implementation and validation tasks are complete. No known code
changes remain. A follow-up agent should start by checking `git status`; the
only expected worktree change is this handoff file.
