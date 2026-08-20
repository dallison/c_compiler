# C++29 roadmap

DaveCC accepts `-std=c++29`, `-std=gnu++29`, and the draft aliases
`-std=c++2d` and `-std=gnu++2d`. These modes define `__cplusplus` as
`202700L` and retain all C++26 features.

C++29 is still under development. The list below distinguishes proposals from
features already adopted into the working draft so experimental work does not
accidentally claim final-standard conformance.

## Injection-first work

1. [P3294R2: Code Injection with Token Sequences](https://wg21.link/p3294r2)
   is implemented experimentally in C++29 mode. DaveCC supports `^{ ... }`,
   `\(value)`, `\id(...)`, `\tokens(...)`, `queue_injection`,
   `namespace_inject`, structural token-sequence equality, token replay, and
   module serialization. The proposal's design is still evolving.
2. [P4033R1: Synthesizing enum at compile time with
   `define_enum`](https://wg21.link/p4033r1) is implemented experimentally for
   opaque scoped enums.
3. [P3385R8: Attributes reflection](https://wg21.link/p3385r8) is implemented
   experimentally for DaveCC's supported standard and GNU attributes.
4. [P2287R6: Designated initializers for base
   classes](https://wg21.link/p2287r6) is implemented in C++29 mode.
5. [P3668R4: Defaulted postfix increment and decrement
   operators](https://wg21.link/p3668r4) is implemented in C++29 mode.
6. [P3540R3: `#embed` offset parameter](https://wg21.link/p3540r3) is
   implemented in C++29 mode.
7. [P3822R2: Conditional `noexcept` in compound
   requirements](https://wg21.link/p3822r2) is implemented in C++29 mode.
8. [P3670R4: Pack indexing for template
   names](https://wg21.link/p3670r4) is implemented in C++29 mode.

`<meta>` exposes the experimental APIs in C++29 mode and `<version>` defines
the vendor probes `__davecc_p3294_token_injection` and
`__davecc_p4033_define_enum`, and
`__davecc_p3385_attribute_reflection`. No standard feature-test macro is
defined because WG21 has not assigned one.

Token pieces retain their preprocessed boundaries and source locations.
`\tokens` concatenates sequences without token pasting, while `\(value)` is
replayed as a typed compiler pseudo-token. Namespace declarations follow the
ordinary symbol, ODR, code-generation, and module paths.

`queue_injection` drains at the end of its active mandatory constant
evaluation in source order. Namespace, class-data-member, and block-scope
declaration targets are supported; local declarations are installed before
parsing resumes after the `consteval` block. Failed replay rolls back installed
symbols, class layout changes, and injected AST statements. Class member
function, template, `friend`, and type-alias injection remain unsupported while
DaveCC's class parser is generalized to replay every member-declaration form.

Because P3294 has not been adopted and is marked as needing revision, DaveCC
should treat this syntax as experimental until WG21 settles the design.

`enumerator_spec` accepts validated identifier names, explicit reflected
integral constants or implicit values, and reflected annotations.
`define_enum` completes an opaque scoped enum inside a `consteval` block,
preserving source order, fixed-underlying-type representability, explicit
values, and ordinary implicit incrementing. Duplicate names are rejected
except for repeated `_` placeholders. Synthesized enumerators use the normal
qualified lookup, reflection-query, switch/code-generation, debug-info, and
module-serialization paths. A failed enclosing block rolls the enum back,
including through nested injection frames.

P3385 support includes `^^[[attribute]]`, structural attribute identity,
`is_attribute`, `attributes_of`, both `has_attribute` overloads, identifier and
display-string queries, and module persistence. Reflected attributes can be
attached through `data_member_options::attributes` and
`enumerator_options::attributes`; synthesized `[[no_unique_address]]` members
participate in empty-member layout. The legacy
`data_member_options::no_unique_address` field remains as a deprecated
compatibility spelling.

Namespaced user-defined attributes such as `[[acme::audit("write")]]` are
preserved as identity-only metadata and can be reflected, queried, serialized,
and attached to synthesized members and enumerators. They do not require a C++
namespace declaration. Unqualified unknown attributes and unknown attributes
in the reserved `std` or `gnu` namespaces remain diagnosed in attribute
reflect-expressions.

DaveCC diagnoses `[[assume]]`, multiple attributes in one attribute
reflect-expression, and unsupported argument forms. Annotation reflections are
intentionally distinct from attribute reflections. Attribute appertainment and
effects are limited to the attributes already modeled by DaveCC;
accepted-but-unmodeled attributes retain identity but have no new semantic
effect.

P2287 support permits inherited non-static data members to be named directly
in designated initializer lists and permits a positional prefix when every
positional clause initializes a direct base class. The implementation preserves
recursive base-member ordering, rejects ambiguous or non-aggregate lookup
paths, diagnoses a base initialized by both forms, supports direct braced
designators, constant evaluation, code generation, and module serialization,
and defines `__cpp_designated_initializers` as `202606L` in C++29 mode.

P3668 support permits member, explicit-object member, and non-member postfix
`operator++` and `operator--` definitions to use `= default`. Their generated
bodies copy the operand, invoke the corresponding prefix operator, and return
the saved value. Invalid signatures are diagnosed, unavailable copy, destructor,
or prefix operations define the postfix operator as deleted, and class-template
members are synthesized after substitution. As specified by P3668R4, no feature
test macro is provided.

P3540 support adds the standard `offset(constant-expression)` parameter to
`#embed` and `__has_embed`. The offset is applied to the original resource
before `limit`, an offset at or beyond the resource size makes it empty, and
duplicate or negative offsets are diagnosed. `__cpp_pp_embed` is `202606L` in
C++29 mode and remains `202502L` in C++26 mode.

P3822 support accepts `noexcept(constant-expression)` in compound requirements.
The condition is substituted and contextually converted to a constant `bool`;
an invalid condition makes the requirement unsatisfied, while `false` permits
a potentially throwing operand. `__cpp_concepts` is `202606L` in C++29 mode.

P3670 support extends pack indexing to type, variable, and concept
template-template parameter packs. Indexed template names can be applied
directly or passed as template arguments, use the existing dependent-index and
bounds diagnostics, and persist through module serialization.
`__cpp_pack_indexing` is `202606L` in C++29 mode and remains `202311L` in
C++26 mode.

## Adopted C++29 language features

- [P3097R3: Contracts for virtual functions](https://wg21.link/p3097r3)

## Adopted C++29 library features

- [P3091R6: `lookup` for associative
  containers](https://wg21.link/p3091r6)
- [P3125R6: `constexpr` pointer tagging](https://wg21.link/p3125r6)
- [P3248R4: Require `intptr_t` and `uintptr_t`](https://wg21.link/p3248r4)

Virtual-function contracts are the strongest remaining language follow-up.
