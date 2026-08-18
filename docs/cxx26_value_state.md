# C++26 erroneous values and observable checkpoints

DaveCC models object lifetime and byte value state independently. A live byte
is `valid`, `erroneous`, or `indeterminate`.

For C++26 automatic objects that have no initializer, DaveCC chooses zero for
the implementation-defined bytes associated with an erroneous value. The
compiler emits zero stores so this policy is deterministic at every
optimization level and on every target, while retaining erroneous semantic
state until the bytes are overwritten. A provable value-producing read is
diagnosed; DaveCC does not add a runtime trap.

`[[indeterminate]]` opts an automatic block variable or function parameter out
of that policy. Its uninitialized bytes are not written and an evaluated read
has undefined behavior. Dynamic storage also begins indeterminate. Trivial
bytewise copies preserve state rather than turning zero bits into valid values.
The uninitialized-friendly `unsigned char` and `std::byte` operations preserve
the source state as required by `[basic.indet]`.

`std::observable_checkpoint()` lowers to DaveCC's
`__builtin_observable_checkpoint()`. The frontend emits a dedicated semantic IR
barrier. It is retained by optimization and constrains undefined-behavior-based
code motion, but is neither a memory clobber nor a hardware fence and emits no
machine instruction.
