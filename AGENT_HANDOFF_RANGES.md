# C++23 ranges handoff — 2026-07-31

## Repository state

The completed `<mdspan>` integration is committed through `45cd71e`.  The
existing C++20 ranges runtime corruption was fixed and committed as `5c09cab`
(`Fix functional class construction lowering.`); after that commit,
`exec_ranges_x86_64`, `exec_ranges_aarch64`, `exec_ranges_arm`, and
`exec_ranges_riscv` all passed.

The `views::zip`, `views::zip_transform`, `views::enumerate`,
`views::adjacent`, `views::adjacent_transform`, `views::chunk`, `views::slide`,
`views::chunk_by`, `views::stride`, `views::join_with`, `views::repeat`, and
`views::cartesian_product` roadmap slices are implemented and runtime-clean on
all supported ranges-test targets. The C++23 `ranges::to`, fold, contains,
subrange, prefix, and suffix algorithm slice is also complete.

## Completed implementation

- `libc/include/tuple`
  - Reworked `tuple` into variadic recursive storage.
  - Added `tuple_size`, `tuple_element`, cv propagation, lvalue/rvalue `get`,
    reference-element construction, and internal recursive access.
- `libc/include/ranges`
  - Added variadic recursive `zip_view` storage/cursors.
  - Added `zip_transform_view` and recursive tuple invocation.
  - Added `enumerate_view`.
  - Added `views::zip`, `views::zip_transform`, and `views::enumerate`.
  - Added forward-window `adjacent_view` and `adjacent_transform_view`,
    including non-common sentinels, pipe closures, and the `pairwise` aliases.
  - Added runtime chunk and slide windows, predicate-based chunking, and stride
    iteration with direct and piped adaptors.
  - Added scalar/range-delimited joining, bounded and unbounded repetition, and
    variadic odometer-order cartesian products.
- `libc/include/algorithm`
  - Added `fold_left`, `fold_left_first`, `fold_right`, `fold_right_last`, and
    both left-fold-with-iterator result forms.
  - Added `contains`, `contains_subrange`, `starts_with`, and `ends_with`,
    including predicates and projections.
- `libc/include/ranges`
  - Added explicit and deduced `ranges::to` conversion, explicit-target pipe
    closures, nested conversion, and non-common input handling.
- `cxx_testsuite/tests/syntax/pass/0307_standard_ranges_zip.cpp`
  - Passes with:

    ```bash
    bazel-bin/davecc -target x86_64 -std=c++23 -isystem libc/include \
      -c cxx_testsuite/tests/syntax/pass/0307_standard_ranges_zip.cpp \
      -o /tmp/ranges_zip_syntax.o
    ```
- `cxx_testsuite/tests/exec_ranges/0008_zip_enumerate_views.cpp`
  - Covers shortest-range termination, reference mutation, four-way zip,
    zip-transform, direct enumerate, and piped enumerate.
- `cxx_testsuite/tests/syntax/pass/0308_standard_ranges_adjacent.cpp`
  - Covers view/range concepts, repeated tuple-reference types, transforms,
    pipes, aliases, and a non-common counted range.
- `cxx_testsuite/tests/exec_ranges/0009_adjacent_views.cpp`
  - Covers pair and triple windows, short ranges, reference mutation,
    transformations, pipes, aliases, and non-common sentinels.
- `cxx_testsuite/tests/syntax/pass/0309_standard_ranges_windows.cpp`
  - Covers concepts, direct and piped adaptors, inner subranges, predicates,
    references, and non-common counted ranges.
- `cxx_testsuite/tests/exec_ranges/0010_window_views.cpp`
  - Covers partial chunks, short slides, reference mutation, predicate groups,
    stride sizing, and all four adaptors over non-common ranges.
- `cxx_testsuite/tests/syntax/pass/0310_standard_ranges_composition.cpp`
  - Covers concepts, reference types, bounded and unbounded repeats, variadic
    products, delimiter forms, pipes, and counted ranges.
- `cxx_testsuite/tests/exec_ranges/0011_composition_views.cpp`
  - Covers repeat iteration, cartesian order and empty products, scalar and
    range delimiters, empty inner ranges, and joined-reference mutation.
- `cxx_testsuite/tests/syntax/pass/0311_standard_ranges_algorithms.cpp`
  - Covers fold result types, counted inputs, projected searches, direct and
    piped conversion, allocator construction, deduction, and nested containers.
- `cxx_testsuite/tests/exec_ranges/0012_cxx23_range_algorithms.cpp`
  - Covers fold direction and empty inputs, iterator results, contains/prefix/
    suffix edge cases, projections, filtered conversion, and nested conversion.

## Resolved compiler issues

- Range-for declarations now parse and lower structured bindings without a
  special-case library workaround.
- Recursive tuple access handles four-way zip references.
- Concept argument normalization gathers flat trailing arguments into a
  declared parameter pack, so `regular_invocable<F&, A, B>` evaluates with both
  argument types.
- Template-instantiation keys include array-node cv-qualifiers. This prevents
  const and non-const array-reference specializations from colliding during
  `declval`/`views::all` substitution.
- The standard `zip_transform_view` and adaptor callable constraints are
  restored.

## Validation

The following all pass:

```bash
bazel test //cxx_testsuite:syntax \
  //cxx_testsuite:exec_ranges_x86_64 \
  //cxx_testsuite:exec_ranges_aarch64 \
  //cxx_testsuite:exec_ranges_arm \
  //cxx_testsuite:exec_ranges_riscv --test_output=errors

bazel test //cxx_testsuite:exec_x86_64 --test_output=errors
```

## Historical C++20 ranges handoff

The remainder of this file is the older C++20 ranges investigation.  Its
repository-state claims are historical, but the compiler-debugging notes and
probe descriptions can still be useful.

Everything below is uncommitted work in the working tree (29 files, ~4800 added
lines). Nothing in this effort has been committed; `git log` HEAD is
`2e5f67b Optimize 6502 code generation after register allocation`.

## How to build and test

```bash
bazel build //:davecc          # NOTE: the target is //:davecc, not //c_compiler:davecc
bazel build //:libcx86_64      # target libc used by the runner script

notepad/runone.sh notepad/probe_foo.cc   # compile+link+run one file, result in /tmp/res.txt,
                                         # compiler log /tmp/q.log, program output /tmp/qr.log
bash notepad/mkviews.sh notepad/views_list.txt   # compile-only sweep, one view expression per file
```

A pristine HEAD build lives in the git worktree `/tmp/cc_head`
(`bazel-bin/davecc` there) — use it to decide whether a failure is a regression
from this diff or a pre-existing gap. Remove it with
`git worktree remove /tmp/cc_head` when done.

Debug env vars:

* `DAVECC_TRACE_RANGEFOR=1` — prints the type the range-for resolver sees and
  whether it found member `begin`/`end` (`statement_parser.c`,
  `SyntaxResolveRangeForIterator`).
* `DAVECC_TRACE_AGG=1` — **temporary scaffolding I added** in
  `expr_semantics.c`, `AnalyzeCXXFunctionalClassConstruction`, just before the
  `is_aggregate` branch. Delete before committing.
* `-Xfe-print` dumps the analyzed AST; this is what finally identified the
  compound-literal bug below and is by far the most useful tool here.

## Current sweep result (compile-only, `notepad/mkviews.sh`)

```
OK    std::views::all(values)
OK    std::views::empty<int>
OK    std::views::single(7)
OK    std::views::iota(1, 5)
OK    std::views::iota(3) | std::views::take(4)
FAIL  values | std::views::transform([](int v) { return v * 2; })     No matching overload for begin/end
OK    values | std::views::filter([](int v) { return v % 2 == 0; })
OK    values | std::views::take(3)
OK    values | std::views::take_while([](int v) { return v < 4; })
OK    values | std::views::drop(4)
FAIL  values | std::views::drop_while(...)                            *** compiler SEGFAULT (139) ***
FAIL  values | std::views::reverse                                    No matching overload for begin/end
FAIL  values | std::views::take(2) | std::views::common               constraints not satisfied for common_view;
                                                                      no member named '__make' in the dependent scope
OK    std::views::counted(values + 1, 3)
OK    values | filter(...) | transform(...) | take(2)
```

Important caveat: the sweep only *compiles*. Several "OK" lines are broken at
run time — see the transform crash below — so do not treat this list as a
measure of correctness.

## Fixes made this session

`c_compiler/frontend/syntax/statement_parser.c`, in
`TryParseCXXRangeForStatement`: the choice between the three `[stmt.ranged]`
forms is now deferred (via the existing `AST_OP(range_begin)` /
`AST_OP(range_end)` nodes and `SyntaxResolveRangeForIterator`) for *any* range
whose type is not a known array and not known to have member `begin`/`end` at
parse time — not just for types that look template-dependent. The old code
committed to the ADL form at parse time, which is wrong for
`auto v = <expr returning auto>; for (auto&& x : v)`, because `v`'s type is only
deduced during analysis. `RangeTypeIsDependent` became unused and was deleted.
The member check also materializes the class-template specialization first
(`TypeMaterializeClassTemplateSpecialization`), since members only exist on the
specialization, not on the primary.

This was necessary but not sufficient; the remaining failures are the two
compiler bugs below.

## Bug 1 (root cause of most of it): `T<Args>{...}` inside a template keeps the *primary* class type

Reproducer `notepad/probe_aggbrace2.cc` — only the braced form fails:

```cpp
template <class T> struct box { T v; };
template <class T> static box<T> ret_brace(T x) { return box<T>{x}; }   // ERROR
template <class T> static box<T> ret_paren(T x) { return box<T>(x); }   // ok
template <class T> static box<T> ret_named(T x) { box<T> t{x}; return t; }  // ok
template <class T> static box<T> ret_bare (T x) { return {x}; }        // ok
```

```
error: No matching overload for box<int> (box<int>::box<int>(void))
    note: candidate (implicit copy constructor) not viable: no known conversion
          from 'box' to 'const box<int>&' for argument 1
error: Incorrect number of arguments supplied to function call; need 1, got 2
```

`-Xfe-print` shows exactly what is wrong in the instantiated body:

```
ret_brace: auto function (x: int) {
  return
  +-- , -> box<int>
  |   +-- ( -> void
  |   |   +-- identifier box<int> -> void(box<int>*)          <- return-by-move copy ctor
  |   |   +-- & -> box<int>*
  |   |   |   +-- identifier __invented__11 -> box<int>
  |   |   +-- compound_literal -> box                         <- WRONG: primary template struct
  |   |   |   +-- identifier __invented__3 -> box             <- temp symbol not cloned/substituted
  |   |   |   +-- braced-init -> box
  |   |   |   |   +-- designated-init -> T                    <- still the template parameter
```

Path: `expr_parser.c:3412 ParseCXXBracedTemporaryExpression` sees `box<T>{`,
finds no constructor on the primary, and calls
`ParseCompoundLiteral(syntax, TypeRecordCopy(type))`, creating a temporary whose
type is the *primary* struct `box` (whose member `v` has type `T`). When the body
is cloned for `T = int`, that temporary is not substituted:
`type_template_clone.c:574 CloneTemplateDependentTemporarySymbol` is only reached
through the identifier path (~line 3043) and `SubstituteTemplateParameters`
appears to leave a bare primary-struct type unchanged, so the compound literal
keeps type `box` while the enclosing return-by-move
(`statement_semantics.c:1525 MaterializeCXXReturnByMove`) builds
`box<int>::box<int>(box)` — no viable constructor.

This is **pre-existing at HEAD** (verified with `/tmp/cc_head`), not a regression
from this diff, but it matters enormously here because `libc/include/ranges` uses
`return some_view<...>{...}` throughout (`take_while_view{...}`, `drop_view{...}`,
`filter_view(...)`, …). I believe fixing this — making the compound-literal
temporary's type substitute/materialize to the specialization during cloning —
will also fix Bug 2 and several of the sweep failures at once.

Suggested attack: either (a) substitute the type of compound-literal temporaries
during body cloning (extend `CloneTemplateDependentTemporarySymbol` /
`SubstituteTemplateParameters` so a primary struct with dependent
`template_arguments` materializes to the specialization), or (b) make
`AnalyzeCompoundLiteral` materialize a primary-struct type via
`TypeMaterializeClassTemplateSpecialization` before initializing.

## Bug 2: `auto` from a pipe expression deduces garbage (`int`)

```cpp
auto view = values | std::views::transform(fun);
for (auto&& v : view) ...      // error: No matching overload for begin
```

`DAVECC_TRACE_RANGEFOR=1` prints `range-for begin: type=int struct=0
member_begin_end=0`, i.e. `view`'s deduced type is `int`, not
`transform_view<...>`. Evidence that the view machinery itself is fine
(`notepad/probe_tv5.cc`):

* `std::views::all(values)` → `ref_view`, iterates correctly (sum printed).
* `transform_view<ref_view<int[6]>, F>::__make(rv, fun)` constructs and iterates
  correctly (`explicit=42`).
* Only the `auto`-deduced-from-`__make_transform_adaptor` form fails.

So the failure is in deducing the `auto` return type through
`__make_transform_adaptor` (`libc/include/ranges:2350`), whose body is
`using Base = all_t<R>; using Result = transform_view<Base, decay_t<F>,
iterator_t<Base>, sentinel_t<Base>>; return Result::__make(...)`. Simpler
versions of that shape do work (`notepad/probe_autoret.cc`,
`notepad/probe_staticmake.cc` both pass), so the trigger is the alias-template
chain (`all_t` / `iterator_t` via `decltype`) or the same primary-vs-
specialization confusion as Bug 1. When the type is spelled explicitly, the same
code compiles and runs (`notepad/probe_tv3.cc` compiles once `TV& r = view;` is
introduced).

## Bug 3: transform crashes at run time — call to a never-emitted instantiation

`notepad/probe_tv4.cc` prints `closure size=2` then dies with
`Unsupported instruction at 0x400000000` on
`auto v2 = std::views::transform(values, fun);`. That address is the image base,
i.e. a call through an unrelocated/never-emitted function. Disassembling
(`bazel-bin/x86_64dasm`) shows the emitted instantiations mangled with `RPi`
(`int*&`) for the range parameter:

```
call _ZNK3std6ranges8__detail14__transform_fnclEIRPiR16__invented__3793ERPiR...
call _ZN3std6ranges8__detail24__make_transform_adaptorEIRPiRK16__invented__3793E...
```

so the *definition* that got emitted is keyed on a decayed `int*&` while the call
site wants the `int(&)[6]` instantiation (or vice versa). Deduction itself is
correct — `notepad/probe_arrdeduce2.cc` reports `R = int(&)[6]` — so the mismatch
is in mangling/instantiation keying, not in `[temp.deduct.call]`.

## Bug 4 (separate, small): `sizeof(R)` where `R = int(&)[6]` returns 8

`notepad/probe_arrdeduce.cc`: `fwd=8 by_ref=24 by_array=6 nested=6` — `sizeof` of
a reference-to-array type yields the pointer size instead of the referent's size.
Independent of ranges, but it misled me for a while, so worth fixing.

## Also still open

* `values | std::views::drop_while(...)` **segfaults the compiler** — not
  investigated at all yet. Reproduce with
  `bazel-bin/davecc -target x86_64 -std=c++20 -isystem libc/include /tmp/vp/v11.cc -c -o /tmp/v11.o`
  after running the sweep (or regenerate that file with `notepad/mkviews.sh`).
* `views::common`: "constraints not satisfied for class template common_view" plus
  "no member named `__make` in the dependent scope" (`libc/include/ranges:2543`).
* `views::reverse`: same `begin`/`end` failure as transform; expect it to follow
  from Bugs 1/2.

## Todo list carried over

1. (done) x86_64 register-allocator live-in ownership clash / aggregate-return corruption.
2. Add an exec regression test for that aggregate-return clobber.
3. Audit/complete C++20 views and adaptors in `libc/include/ranges`.
4. Verify constrained algorithms are complete and projection-correct.
5. Write and run an idiomatic vector/map ranges demo test.
6. Run the full cxx exec + C regression suites on all targets — **this has not
   been done since any of the frontend changes in this diff**, so there may be
   regressions hiding in it (`cxx_testsuite/BUILD.bazel` gained
   `exec_ranges_{x86_64,aarch64,arm,riscv}` targets).
7. Remove the throwaway `notepad/probe_*.cc` probes and the `DAVECC_TRACE_AGG`
   scaffolding in `expr_semantics.c`.

## Probe files worth keeping while working on this

| file | what it shows |
| --- | --- |
| `notepad/probe_aggbrace2.cc` | Bug 1, minimal, 4 return forms side by side |
| `notepad/probe_tv1.cc` | Bug 2: range-for over a piped view, template and non-template |
| `notepad/probe_tv5.cc` | proves the view types themselves work when named explicitly |
| `notepad/probe_tv4.cc` | Bug 3 runtime crash, step by step through the pipe |
| `notepad/probe_arrdeduce.cc`, `probe_arrdeduce2.cc` | Bug 4 vs correct deduction |
| `notepad/probe_autoret.cc`, `probe_staticmake.cc` | auto-return shapes that *do* work |
| `notepad/probe_nested_concept2.cc` | `viewable_range<int(&)[3]>` is satisfied — concepts are fine |
