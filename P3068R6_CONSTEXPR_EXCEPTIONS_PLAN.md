# C++26 constexpr exceptions (P3068R6) implementation plan

## Handoff state

- Baseline commit: `5ff840a` (`Implement C++26 contracts with cross-target correctness.`)
- The working tree was clean when this plan was written.
- Full C++ execution suites pass for x86-64, AArch64, ARM, and RISC-V.
- All ranges suites and the dynamic-interpreter suite pass.
- Preserve that cross-target baseline. Do not disable tests, reduce optimization,
  or route around compiler defects to make this feature pass.

### Implementation progress

- The AST constexpr evaluator now supports caught scalar and class exceptions,
  `catch (...)`, nested propagation, bare rethrow, catch-by-value/reference,
  exceptional local cleanup, exception-object destruction, and uncaught
  diagnostics.
- Native p-code constexpr dispatch now retains the emitted typeinfo and
  exception-range sections, walks frames and nested ranges in evaluator-local
  state, executes cleanup landing pads, binds scalar/class handlers, supports
  bare rethrow, and destroys catch-by-value and exception objects.
- The temporary C++26 try/throw AST-overlay requirement has been removed.
  Explicit `-fconstexpr-eval=pcode` and audit mode now exercise the native
  p-code exception path.
- P-code runtime exception metadata now distinguishes cleanup pads from
  catch-all handlers. Scalar p-code throws are widened to the runtime's i8
  entry point, and the p-code runtime smoke test covers scalar catch-by-reference
  plus cross-frame destructor unwinding.
- Focused AST, p-code, auto, and audit positive tests and negative diagnostic
  tests pass. The full syntax suite also passes; the AST reference/lvalue
  evaluation defect exposed by `0305_standard_mdspan_layouts.cpp` was fixed.
- Next major step: implement the standard-library and feature-test-macro work
  described in phases 7 and 8.

Paper: <https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3068r6.html>

## Goal

Implement C++26 constant evaluation of `throw`, `try`, and `catch`, including
exception-object lifetime, matching, rethrow, diagnostics, feature-test macros,
and the required standard-library exception support.

A throw may occur during constant evaluation if it is caught before the
evaluation finishes. An uncaught exception still makes the expression fail to
be a constant expression. Undefined behavior and other invalid constant
operations must not become catchable.

The final implementation must work in the AST and p-code constexpr evaluators.
An AST overlay may be useful while developing the first vertical slice, but it
is not the completed implementation: `-fconstexpr-eval=pcode` and audit mode
must eventually exercise real p-code exception handling.

## Existing architecture

Syntax, semantic analysis, serialization, and runtime exception lowering
already exist:

- `c_compiler/frontend/syntax/statement_parser.c`
  - `ParseTryStatement`, `ParseCatchHandler`, `ParseCatchDeclaration`
- `c_compiler/frontend/syntax/expr_parser.c`
  - `ParseCXXThrowExpression`
- `c_compiler/frontend/syntax/ast.h` and `ast.c`
  - `ThrowASTNode`, `CatchASTNode`, `TryASTNode`
- `c_compiler/frontend/semantic/expr_semantics.c`
  - `AnalyzeThrowExpression`, `EnsureThrowCopyConstructor`
- `c_compiler/frontend/semantic/statement_semantics.c`
  - `AnalyzeTryStatement`, `AnalyzeCatchStatement`
- `c_compiler/serialize/ast_serialize.c`
  - try/catch/throw shapes are already serialized
- `c_compiler/backend/expr_codegen.c`
  - runtime throw lowering
- `c_compiler/backend/statement_codegen.c`
  - try/catch lowering, catch binding, cleanup pads, exception ranges
- `c_compiler/backend/eh_metadata.c`
  - native exception metadata
- `libc/eh_throw.c` and `libc/eh_cxa.c`
  - p-code and Itanium runtime exception implementations

The missing behavior is primarily in constant evaluation.

### AST constexpr evaluator

`c_compiler/frontend/syntax/constexpr.c` currently:

- Represents statement completion as normal/return/break/continue/invalid, with
  no exceptional completion.
- Evaluates only the body of `AST_OP(try)` and deliberately ignores handlers.
- Rejects a reached throw through `ConstexprVoidExpressionThrows` or ordinary
  expression-evaluation failure.
- Has no pending-exception object or exceptional scope unwinding.

`c_compiler/frontend/syntax/expr_evaluator.c` has no constant-evaluation case
for `AST_OP(throw)`.

### P-code constexpr evaluator

`c_compiler/frontend/syntax/constexpr_pcode.c` currently resolves
`__davecc_throw` to a constexpr escape that returns
`kPCodeVMStatusInvalidConstantOperation`.

Runtime p-code exception ranges are emitted, but `p_code_vm.c` has no constexpr
unwind/handler-dispatch mechanism. Existing runtime handling in
`libc/eh_throw.c` is a useful semantic reference but should not be invoked as a
host/runtime side effect during constant evaluation.

### Library

- `libc/include/exception` lacks the C++26 constexpr surface.
- `libc/include/stdexcept` uses heap-backed message ownership unsuitable for
  constant evaluation.
- `c_compiler/frontend/lexical/preprocessor.c` does not define
  `__cpp_constexpr_exceptions`.
- `libc/include/version` does not define
  `__cpp_lib_constexpr_exceptions`.

Verify the final standardized macro values against the adopted wording before
adding them; do not copy a provisional value blindly.

## Implementation phases

### 1. Establish conformance tests first

Add focused tests that initially fail for the intended reason:

1. Caught scalar exception:

   ```cpp
   constexpr int divide(int n, int d) {
     if (d == 0) throw 42;
     return n / d;
   }

   constexpr int safe_divide(int n, int d) {
     try {
       return divide(n, d);
     } catch (int value) {
       return -value;
     }
   }

   static_assert(safe_divide(10, 2) == 5);
   static_assert(safe_divide(10, 0) == -42);
   ```

2. `catch (...)`.
3. Nested try blocks and propagation to an outer handler.
4. Bare `throw;` from a handler.
5. Uncaught exception rejected with a diagnostic at the throw site.
6. The same caught throw rejected before C++26.
7. A class exception copied into exception storage and caught by value and
   `const&`.
8. Base-class handler matching.
9. Destructors for locals run during exceptional unwinding.
10. Exception-object destruction after handler completion.
11. Invalid constant operations remain uncatchable.
12. `consteval`, `constexpr` initialization, `static_assert`, templates, and
    `if constexpr`.
13. A module exporting a constexpr function containing try/catch.

The existing pre-C++26 negative test
`cxx_testsuite/tests/syntax/fail/0310_constexpr_reached_throw.cpp` should remain
a pre-C++26 rejection test or gain a corresponding C++26 pass case.

Use the next available test numbers rather than assuming those listed in older
notes are still free.

### 2. Add exceptional completion to the AST evaluator

Primary files:

- `c_compiler/frontend/syntax/constexpr.h`
- `c_compiler/frontend/syntax/constexpr.c`

Introduce an evaluator-owned exception representation containing at least:

- The thrown static/dynamic type needed for catch matching.
- Scalar/pointer value or owned `ConstexprObject` storage.
- The source location of the originating throw.
- State needed to distinguish an active exception from an uncaught one.
- Ownership/lifetime state so the object cannot escape evaluation.

Extend statement completion with exceptional completion, either by adding
`kConstexprStmtThrow` plus a payload in the evaluation context, or by a clearly
equivalent pending-exception mechanism. Do not encode a throw as generic
`Invalid`; handlers must be able to consume it.

Implement:

- Evaluation of a throw operand, including required copy/move construction.
- Bare rethrow, valid only while handling an active exception.
- Propagation through expression statements, compound statements, loops,
  function calls, and nested constexpr calls.
- `AST_OP(try)` handler dispatch.
- Resumption after a handler completes normally.
- Propagation when no handler matches.
- C++26 language gating; preserve pre-C++26 behavior.

Remove or narrow `ConstexprVoidExpressionThrows`. It must no longer reject every
reached throw in C++26, but it may remain part of pre-C++26 validation.

### 3. Implement catch matching and bindings

Reuse the language rules already enforced by semantic analysis and mirror the
runtime implementation rather than inventing a second incompatible subset.

Support in the initial complete implementation:

- Exact scalar types.
- Pointer conversions allowed for exception handlers.
- Class types.
- Public, unambiguous base-class handlers.
- `catch (...)`.
- Catch by value and by reference with correct cv qualification.
- Handler declaration initialization from the exception object.

Relevant references:

- `GenerateCatchBinding` and catch helpers in
  `c_compiler/backend/statement_codegen.c`
- Type matching and exception table handling in `libc/eh_throw.c`
- RTTI/base conversion helpers used by runtime exception matching

Catch matching must not mutate or destroy the active exception until ownership
has transferred to the selected handler as required.

### 4. Add exceptional unwinding and lifetime tracking

This is required for correctness, not an optional enhancement.

Record a scope/binding/object watermark when entering each evaluated scope or
try frame. On propagation:

- Destroy fully constructed automatic objects in reverse construction order.
- Do not destroy objects whose construction did not complete.
- Pop bindings and temporary objects exactly once.
- Preserve the exception object while unwinding to a matching handler.
- Destroy the exception object after the handler completes unless it has been
  rethrown.
- Correctly handle a second exception thrown while evaluating a destructor.
- Prevent an exception object, reference into it, or exception handle from
  escaping the full constant-expression evaluation.

Review every early return from the evaluator for balanced binding/object-stack
cleanup. The normal, return, break, continue, throw, and invalid paths should
use one explicit cleanup model rather than duplicated ad hoc cleanup.

### 5. Improve diagnostics

An uncaught constexpr exception should report:

- That constant evaluation ended with an uncaught exception.
- The exception type.
- The original throw location.
- The constexpr call chain when available.
- A constant `what()` message when the library exception representation safely
  provides one.

Keep distinct diagnostics for:

- Uncaught exception.
- Unsupported constexpr operation.
- Undefined behavior/invalid memory operation.
- Step/depth limit.
- Exception-object lifetime escape.

Do not turn all failures into the generic p-code
`invalid constant operation` message.

### 6. Implement real p-code constexpr exceptions

Primary files:

- `c_compiler/frontend/syntax/constexpr_pcode.c`
- `c_compiler/p_code/p_code_vm.h`
- `c_compiler/p_code/p_code_vm.c`
- `c_compiler/backend/expr_codegen.c`
- `c_compiler/backend/statement_codegen.c`
- Possibly `c_compiler/p_code/p_code_codegen.c`

Implement evaluator-local p-code exception state:

- Pending exception object/handle.
- Active handler stack or lookup through emitted exception ranges.
- Frame-by-frame propagation.
- Catch selection and binding.
- Rethrow.
- Cleanup execution.
- Exception-object lifetime checks at evaluation completion.

Two viable designs should be evaluated before coding this phase:

1. Teach the VM to consult the already-emitted exception ranges and perform a
   constexpr-safe equivalent of the table walk in `libc/eh_throw.c`.
2. When `Generator.for_constant_evaluation` is true, lower try/catch to an
   explicit p-code control-flow protocol with evaluator-local exception slots.

Prefer the design that reuses established catch metadata without invoking the
runtime ABI or host unwinder. Avoid calling normal runtime allocation,
`__cxa_throw`, or process-global current-exception storage from the constexpr
VM.

During development, add `try`/`throw` to
`ConstexprPCodeRequiresASTOverlay` so auto mode does not return wrong results.
Before declaring P3068 complete, remove that dependency for supported cases and
verify:

- `-fconstexpr-eval=ast`
- `-fconstexpr-eval=pcode`
- `-fconstexpr-eval=audit`

Audit mode must compare actual independent evaluator results rather than an AST
result against an AST overlay.

### 7. Add standard-library constexpr exception types

Primary files:

- `libc/include/exception`
- `libc/include/stdexcept`
- `libc/exception.cc`
- `libc/stdexcept.cc`
- `libc/include/version`
- `BUILD.bazel` if source ownership changes

Implement the P3068-required constexpr constructors, copying, assignment,
destruction, and `what()` behavior. The current heap-owned `__what_holder`
cannot simply be used during constant evaluation.

Use a representation with:

- Correct runtime ownership and copy behavior.
- A constexpr-capable message path.
- No dangling pointer when a copied exception outlives a constructor argument.
- Stable `what()` contents through copies and catches.
- No leaked constexpr allocation.

Cover `std::exception`, the `logic_error` and `runtime_error` families, and the
other exception types explicitly required by the adopted paper. Treat
`exception_ptr`, nested exceptions, and allocation/typeid exceptions according
to the paper rather than assuming they are deferred; if a required facility
cannot be implemented in the first commit, document and test the exact
remaining conformance gap.

### 8. Feature-test macros and contracts interaction

Add the core and library macros only when their represented functionality is
actually available:

- `__cpp_constexpr_exceptions` in
  `c_compiler/frontend/lexical/preprocessor.c`
- `__cpp_lib_constexpr_exceptions` in `libc/include/version`

Review C++26 contracts:

- A throw from a contract predicate in observe mode uses
  `evaluation_exception` at runtime.
- Define and test the corresponding behavior during constant evaluation from
  the adopted contracts and constexpr-exception wording.
- Do not let exception handling bypass a failed constant-evaluated contract.

### 9. Serialization and modules

No AST serialization changes are expected because throw/catch/try shapes are
already supported. Verify rather than assume:

- Export a constexpr function containing nested try/catch from a module.
- Import it and use it in `static_assert`.
- Test a thrown class type declared in the module.
- Confirm no evaluator-only exception state is serialized.

## Test matrix

Run focused cases in all evaluator modes:

```sh
bazel-bin/davecc -std=c++26 -fconstexpr-eval=ast ...
bazel-bin/davecc -std=c++26 -fconstexpr-eval=pcode ...
bazel-bin/davecc -std=c++26 -fconstexpr-eval=audit ...
```

Then run at least:

```sh
bazel test //cxx_testsuite:syntax
bazel test //cxx_testsuite:modules
bazel test //cxx_testsuite:exec_x86_64
bazel test //cxx_testsuite:exec_aarch64
bazel test //cxx_testsuite:exec_arm
bazel test //cxx_testsuite:exec_riscv
bazel test //:dynamic_interpreters_test
```

Also run the repository's exception ABI gate target/script and all existing
runtime exception tests. Use the exact test target names from the current
`BUILD.bazel`; do not silently skip a missing target.

## Suggested commit sequence

Keep the implementation reviewable:

1. Add failing/pass conformance tests and AST scalar exception flow.
2. Add class exception objects, matching, unwinding, and diagnostics.
3. Add p-code VM parity and audit-mode tests.
4. Add standard-library exception changes and feature macros.
5. Add contracts/module integration and remaining conformance cases.

Do not commit an intermediate step that makes default mode pass only by hiding
a broken p-code path unless the commit explicitly remains part of an
unpublished local sequence.

## Acceptance criteria

P3068 is complete only when:

- Caught exceptions work in constant expressions under C++26.
- The same reached throw remains invalid under earlier language standards.
- Uncaught exceptions produce useful deterministic diagnostics.
- Catch matching, rethrow, scope unwinding, and destruction are correct.
- Exception objects and references cannot escape constant evaluation.
- Invalid constant operations remain uncatchable.
- AST, p-code, and audit modes agree.
- Required standard-library exception types are constexpr-capable.
- Final standardized feature-test macros are present.
- Module round-tripping works.
- Existing runtime exception ABI behavior is unchanged.
- Full cross-target execution, syntax, module, ranges, and dynamic-interpreter
  suites remain green without skips or test-specific compiler workarounds.

## High-risk review checklist

Before each commit, specifically inspect:

- Exception objects or catch references surviving their owning evaluation.
- Double destruction after rethrow or handler exit.
- Missing destruction on propagation across a constexpr function call.
- A cleanup that runs on normal exit but not exceptional exit, or vice versa.
- A pending-exception flag left set after a handler consumes it.
- Nested exceptions overwriting the active exception.
- P-code frame state retained after VM suspension/resume.
- Host pointers stored using the wrong source-target pointer width.
- Runtime-global exception state accidentally shared by constexpr evaluations.
- Retry/audit mode reusing stale evaluator exception state.
- Differences between 32-bit ARM and 64-bit targets.
