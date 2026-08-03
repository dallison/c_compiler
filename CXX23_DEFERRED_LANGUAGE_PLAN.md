# Deferred C++23 Language Plan

## To-do
- [x] Stabilize and revalidate the range-for parser lookahead fix.
- [x] Implement P2718 range-initializer temporary lifetime semantics.
- [x] Implement source/literal encoding and P2290 delimited escapes.
- [x] Vendor pinned Unicode data and implement P2071 named escapes.
- [ ] Implement P2564 and complete the deferred `constexpr` rules.
- [ ] Implement P2582 inherited-constructor CTAD.
- [ ] Validate feature macros, run conformance suites, and close the roadmap.

## 1. Stabilize the range-for parser baseline
- Retain the diagnostic-free top-level-colon probe in
  `c_compiler/frontend/syntax/statement_parser.c`, including nesting and
  conditional-operator handling.
- Keep the focused classic-loop regression in
  `cxx_testsuite/tests/syntax/pass/0313_cxx23_low_coupling_language.cpp`.
- Re-run the existing unordered-container, syntax, and x86-64 execution
  coverage after later range changes.

## 2. Implement P2718 range-for temporary lifetime extension
- Mark the synthesized range-for outer compound/range initializer so semantic
  cleanup can distinguish it from ordinary reference initialization.
- Collect non-parameter temporaries created by the C++23
  for-range-initializer, transfer destructor ownership from full-expression
  cleanup to the loop scope, and preserve reverse construction order.
- Integrate extended objects with scope-exit destruction for normal completion,
  early exits, exceptions, and coroutine suspension without double-destruction.
- Add C++20/C++23 execution tests and advertise
  `__cpp_range_based_for == 202211L` only after the behavior is complete.

## 3. Complete encoding and delimited escapes
- Preserve embedded NULs in narrow string-literal spellings.
- Implement P2290 `\o{...}`, `\x{...}`, and `\u{...}` with C++23 gating,
  range checks, diagnostics, and correct UTF-8/16/32/wide encoding.
- Complete `u`/`U` literal type and code-unit handling.
- Support explicit UTF-8 source input, validate UTF-8, remove an initial BOM,
  and implement P2314/P2316-consistent literal encoding.
- Add lexical, syntax, and execution tests for valid and malformed encodings.

## 4. Implement P2071 named universal character escapes
- Vendor a pinned Unicode Character Database release and license.
- Add a deterministic generator for a compact immutable name lookup.
- Implement exact case- and whitespace-sensitive `\N{...}` lookup for assigned,
  aliased, control, and algorithmically named characters.
- Reuse universal-character validation and encoding in literals and identifiers.
- Define `__cpp_named_character_escapes == 202207L` after coverage passes.

## 5. Implement P2564 and expanded `constexpr`
- Model the standard-defined immediate-escalating expressions and eligible
  functions, including templates, lambdas, defaulted special members, and
  `if consteval` boundaries.
- Persist escalation state through instantiation and serialization, then raise
  `__cpp_consteval` to `202211L`.
- Implement P2242 path-sensitive labels, `goto`, and non-literal locals; audit
  P2448; explicitly test the already-working P2647 static `constexpr` locals.
- Keep AST and pcode evaluators aligned before raising `__cpp_constexpr`.

## 6. Implement P2582 inherited-constructor CTAD
- Fix dependent `using Base<T>::Base` resolution and constructor importing.
- Synthesize transformed inherited-constructor guides, rewrite their result to
  the derived specialization, substitute constraints, and apply the required
  preference for non-inherited guides.
- Cover dependent bases, user-defined guides, ambiguity, constraints, modules,
  and pre-C++23 behavior before advertising `202207L`.

## 7. Integration and roadmap closure
- Check every completed feature macro in C++23 and its lower value or absence in
  C++20 without advertising partial support.
- Run focused tests per phase, then full lexical, syntax, x86-64 execution,
  coroutine, module, and relevant multi-target suites.
- Update `CXX20_CXX23_REMAINING.md` only after all deferred items have passing
  conformance coverage.
