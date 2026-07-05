# Agent Handoff: `std::variant` C++20 Compliance

Work dir: `/Users/FZDSZZ/c_compiler`. Tree is dirty. This handoff covers the
`std::variant` effort specifically (the older `AGENT_HANDOFF.md` covers
`std::optional`).

## User intent (unchanged, important)

- Make `std::variant` fully C++20 compliant.
- **No workarounds.** If the library header hits a real compiler gap, fix the
  compiler, not the header.
- Work **one bug at a time**; keep the suite green as you go.
- Do not edit the plan files; use the todo list.

## Current suite state

Build the compiler with `bazel build //:davecc` then run:

```
bash cxx_testsuite/run_exec_tests.sh --davecc bazel-bin/davecc --target x86_64 \
  --libc bazel-bin/libc/libcx86_64.a --interpreter bazel-bin/x86_64 \
  --suite-root cxx_testsuite --compile-arg -Wl,-e --compile-arg -Wl,main \
  --interp-arg -i
```

`pass=140 fail=2`. The two failures are the variant tests:
- `0140_standard_variant_core.cpp` (compile) — see "Active bug" below.
- `0142_standard_variant_visit.cpp` (compile) — `std::visit`, not yet investigated.

The other variant tests already pass:
- `0139_standard_variant_foundations.cpp` PASS
- `0141_standard_variant_comparison.cpp` PASS

The tree currently has **no debug instrumentation** and builds clean.

## Compiler fixes completed this session (all in the working tree)

1. **Variadic template pack "doubling"** — `c_compiler/frontend/syntax/type.c`,
   `DeduceFunctionTemplateTemplateArguments` (the `is_pack_expansion` branch).
   When a pack `Types...` appears in more than one function parameter (e.g.
   `operator==(const variant<Types...>&, const variant<Types...>&)`), deduction
   used to *concatenate* the elements from each argument (giving
   `variant<int,long,int,long>`). It now records the already-deduced prefix and,
   for subsequent parameters, verifies the newly deduced elements match the
   prefix and truncates the duplicates (failing deduction on mismatch). This is
   what got `0141` passing.

2. **`get_if<I>` const/non-const pointer overload ambiguity** —
   `c_compiler/frontend/semantic/expr_semantics.c`, `OverloadConversionRank`.
   Added a sub-tier `qualification_penalty` (`+1`) for by-value pointer/array
   conversions that differ *only* in pointee/element `const` qualification, so
   `T*`→`const T*` ranks just below an exact match without disturbing other rank
   tiers or the implicit-object (`this`) binding. (An earlier attempt that
   changed `OverloadBaseConversionRank` by a whole tier regressed 0134/0139/0141
   and was reverted.)

3. **Dependent qualified-id as a template argument misparsed as a type** —
   `c_compiler/frontend/syntax/syntax.c`. Added
   `SyntaxTemplateArgumentLooksLikeType`, used by
   `SyntaxParseTemplateArgumentList` in place of the bare `SyntaxLookingAtType`.
   Rationale: for a template argument, a *dependent qualified-id not introduced
   by `typename`* is a value per `[temp.res]`, so
   `ptr<__index_of<remove_cvref_t<T>, Types...>::value>` must be parsed as a
   non-type argument, not a type-id. `SyntaxLookingAtType` conservatively said
   "type" for any dependent `a::b`. The new helper parses the full qualified-id;
   if it is qualified and does not resolve to a tag/typedef, it is treated as a
   non-type. This moved `0140` past the "Unknown type name / Missing >" parse
   errors. **Verified no suite regressions.**

## Active bug (0140): `operator=(T&&)` deref at `libc/include/variant:708-709`

Error: `libc/include/variant:709: Cannot take contents of this expression`.

### What the test needs

`0140` does `std::variant<int, const char*> value; ... value = "world";` (line
87) and `std::variant<int,const char*> text("hello");` (line 77). Both go
through the converting constructor/assignment which the header selects with:

```cpp
this->template __ptr<
    __variant_detail::__index_of<remove_cvref_t<T>, Types...>::value>()
```

`__index_of<...>` is an *exact-match* index lookup with no conversions. For a
string literal `T` deduces to `const char(&)[6]`, `remove_cvref_t<T>` is
`const char[6]`, which is **not** an alternative of `variant<int, const char*>`,
so `__index_of` recurses into the incomplete `__index_of<const char[6]>` and is
ill-formed. Conforming `std::variant` instead selects the alternative via the
"FUN" overload-set trick, which honors the `const char[6]`→`const char*`
conversion.

### Decision already made with the user

We asked whether to (a) implement the compiler features needed for FUN-based
selection, (b) switch the header to `decay_t`, or (c) **fix the secondary
compiler bug first, then decide on selection**. The user chose **(c)**. So:

- **Do first:** fix the "pointer dropped" compiler bug described below.
- **Then revisit:** how variant selects the converting alternative. Note the
  full FUN trick does **not** compile today — see `f1.cpp` repro: the compiler
  lacks variadic using-declarations over an inherited `operator()` set and
  `decltype` of a call expression. So conforming selection likely needs more
  compiler work, or a simpler recursive `is_convertible`-based selector in the
  header (to be decided after the pointer-drop fix, since a clean SFINAE failure
  on the ill-formed `__index_of` is a prerequisite for any recursive selector).

### Secondary compiler bug to fix first: `typename X::type*` return type loses its pointer

Diagnosis (traced with temporary instrumentation, now removed):

- The failing deref operand is a **call** node (`op == AST_OP(call)`) whose type
  is `int` — the pointer was dropped.
- The callee (the `__storage_.template __ptr<0>()` member access) has the
  **correct** function type `int *(struct __storage<0,[int,const char*]>*)`, and
  the resolved member symbol's return type is `int *`. So overload resolution is
  fine; only the enclosing call node's result type is a **stale `int`**.
- The call node's result type is set at **clone/instantiation time** by
  `SetClonedCallReturnType` (`type.c:4646`), invoked from `type.c:5708-5711`
  (`if (node->op == AST_OP(call)) { ... SetClonedCallReturnType(call, call->left->type); }`).
  `SetClonedCallReturnType` itself is correct (`func->next` is the return type),
  but at the moment it runs the cloned callee's substituted return type is
  already `int` (pointer dropped). Later `MemberTemplateOverloadCandidate`
  (`expr_semantics.c:4272`) re-resolves `__ptr` to a function returning `int *`
  and fixes the callee node's type, **but the call node's result type is never
  recomputed**, so `AnalyzeContentsOperator` (`expr_semantics.c` ~line 5521)
  sees `int` and errors.

Where the pointer is actually dropped is the generic-body substitution of
`__ptr`'s return type `typename __type_at<Target - I, T, Rest...>::type*`
(`libc/include/variant:135` and `:220`). Substitution happens in
`SubstituteTemplateParameters` (`type.c:2417`). Note that dependent-member
handling there only triggers when `type->declarator == kDeclPrimitive`
(lines 2449 and 2549); a **pointer wrapping** a dependent-member type goes
through the generic "copy then recurse into `->next`" path (lines ~2603-2646).
The exact line that yields `int` instead of `int*` for the ill-formed/unfoldable
`Target` was not yet pinned down — this is the next thing to find.

Two candidate fixes (pick after confirming the exact drop point):
- Make the pointer-to-dependent-member substitution preserve the pointer
  declarator (fix in `SubstituteTemplateParameters`), and/or
- After the call's callee is re-resolved by `MemberTemplateOverloadCandidate` /
  `InstantiateClonedFunctionTemplateCall`, recompute the call node's result type
  from the (now correct) callee function type instead of trusting the stale
  clone-time type.

Also worth deciding: when `__index_of<...>::value` is genuinely ill-formed, the
compiler should produce a clean diagnostic (e.g. "no member named 'value'")
rather than silently degrading `int*`→`int` and reporting the misleading
"Cannot take contents".

## Reproducers (`/tmp/vtests/tests/exec/`, run with `bash /tmp/vtests/run.sh <file>`)

- **`p10.cpp` — reliable minimal repro of the pointer-drop bug.** Fails with
  `Cannot take contents of this expression`. It mirrors the header: recursive
  `union storage` with `if constexpr` and a `ptr<Target>()` returning
  `typename type_at<Target - I, ...>::type*`, an `index_of` that inherits
  `::value` from `std::integral_constant`, an `operator=(T&&)` with the
  `if (index_==...) *ptr<...>()=... else emplace<...>(...)` shape, competing
  copy/move assignment, and two assignments `v = 17; v = "world";` on
  `Var<int, const char*>`. The `v = "world"` (array `const char[6]`)
  instantiation is what triggers it.
- `p12.cpp` — an earlier reduction that **no longer reproduces** (it dropped the
  `if/else`+`emplace`+`construct` structure). Do not rely on it; use `p10.cpp`
  or the real `0140` compile.
- `f1.cpp` — shows the compiler cannot compile the standard FUN selector
  (variadic `using base::operator()...` + `decltype(x(y))::value`).
- `p2.cpp`–`p9.cpp`, `w*.cpp`, `g*.cpp` — intermediate reductions; mostly
  passing, kept for reference.

Instrument quickly by re-adding a guarded print in
`AnalyzeContentsOperator` (dump `node->sub->op`, `node->sub->type`, and for a
call node its `->left` callee type and the resolved member's
`symbol->type->next`), and/or in `MemberTemplateOverloadCandidate` (dump the
explicit arg and the instantiated return type). Compile the real 0140 with:

```
D=bazel-bin/davecc; L=bazel-bin/libc/libcx86_64.a; INC=libc/include
$D -target x86_64 -static -std=c++20 -isystem $INC -Wl,-e -Wl,main \
  cxx_testsuite/tests/exec/0140_standard_variant_core.cpp $L -o /tmp/x.bin
```

## Todo state at handoff

- DONE: variadic pack doubling (0141), get_if qualification tie-break,
  dependent-qualified template-argument parse.
- IN PROGRESS: pointer-drop compiler bug (this file's "Active bug").
- PENDING: decide variant converting-ctor/assignment alternative selection.
- PENDING: `0142` `std::visit` resolution.
- PENDING: re-run full cxx exec suite; ensure green; clean up.

## Key file/line references

- `libc/include/variant:135,220` — `__ptr<Target>()` return `typename __type_at<...>::type*`.
- `libc/include/variant:708-709` — failing `*__storage_.template __ptr<__index_of<...>::value>()`.
- `c_compiler/frontend/syntax/type.c:2417` — `SubstituteTemplateParameters`
  (dependent-member at 2449/2549; pointer/`next` recursion ~2603-2646).
- `c_compiler/frontend/syntax/type.c:4646` — `SetClonedCallReturnType`.
- `c_compiler/frontend/syntax/type.c:5708-5711` — where the call's clone-time type is set.
- `c_compiler/frontend/semantic/expr_semantics.c:4272` — `MemberTemplateOverloadCandidate`.
- `c_compiler/frontend/semantic/expr_semantics.c` ~5521 — `AnalyzeContentsOperator` (error site).
- `c_compiler/frontend/syntax/syntax.c` — `SyntaxTemplateArgumentLooksLikeType` and `SyntaxParseTemplateArgumentList`.
