# C++29 roadmap

DaveCC accepts `-std=c++29`, `-std=gnu++29`, and the draft aliases
`-std=c++2d` and `-std=gnu++2d`. These modes define `__cplusplus` as
`202700L` and retain all C++26 features.

C++29 is still under development. The list below distinguishes proposals from
features already adopted into the working draft so experimental work does not
accidentally claim final-standard conformance.

## Injection-first work

1. [P3294R2: Code Injection with Token Sequences](https://wg21.link/p3294r2)
   proposes token-sequence literals written `^{ ... }`, interpolation, queued
   injection, and namespace injection. This is the most capable option, but its
   design is still evolving.
2. [P4033R1: Synthesizing enum at compile time with
   `define_enum`](https://wg21.link/p4033r1) is a smaller generative-reflection
   step that fits DaveCC's existing `define_aggregate` machinery.
3. [P3385R8: Attributes reflection](https://wg21.link/p3385r8) would allow
   attributes to be inspected and attached to synthesized declarations.

The recommended first P3294 implementation slice is:

1. represent token sequences as compile-time values;
2. parse `^{ ... }` literals while retaining source tokens;
3. implement token, identifier, and reflection interpolation;
4. implement local `queue_injection`;
5. add namespace injection after declaration ordering, lookup, diagnostics, and
   module serialization are stable.

Because P3294 has not been adopted and is marked as needing revision, DaveCC
should treat this syntax as experimental until WG21 settles the design.

## Adopted C++29 language features

- [P3097R3: Contracts for virtual functions](https://wg21.link/p3097r3)
- [P2287R6: Designated initializers for base
  classes](https://wg21.link/p2287r6)
- [P3670R4: Pack indexing for templates](https://wg21.link/p3670r4)
- [P3540R3: `#embed` offset parameter](https://wg21.link/p3540r3)
- [P3822R2: Conditional `noexcept` in compound
  requirements](https://wg21.link/p3822r2)
- [P3668R4: Defaulted postfix increment and decrement
  operators](https://wg21.link/p3668r4)

## Adopted C++29 library features

- [P3091R6: `lookup` for associative
  containers](https://wg21.link/p3091r6)
- [P3125R6: `constexpr` pointer tagging](https://wg21.link/p3125r6)
- [P3248R4: Require `intptr_t` and `uintptr_t`](https://wg21.link/p3248r4)

Good small follow-up projects are the `#embed` offset parameter and defaulted
postfix operators. Good projects that reuse DaveCC's strongest existing
subsystems are virtual-function contracts, template pack indexing, and
`define_enum`.
