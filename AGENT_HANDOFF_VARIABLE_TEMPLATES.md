# Agent handoff: variable templates, protobuf / Abseil patterns

**Updated:** 2026-10-10  
**Repo:** `/Users/FZDSZZ/c_compiler`  
**Branch:** `master` (merge from `feature/subspace` completed locally; **not pushed** unless user asks)

## Goal

Compile protobuf-style **static member variable templates** with `constexpr auto` and initializer chains like:

```cpp
template <typename T>
static inline constexpr auto kUnionMember =
    std::get<T Extension::*>(std::tuple{&Extension::float_value});
```

Usage: `Extension::kUnionMember<float>`, `float Extension::* pm = …`, `this->*kUnionMember<T>`, etc.

## Done (merged on `master`)

- [x] Generic lambda → function pointer in call arguments (`atomic::store`-style).
- [x] `is_function` for cv-qualified member function types (`AnyInvocable`).
- [x] `absl::Overload` inherited constructor pack expansion (`T(ts)...`).
- [x] Qualified / static **variable template-id** parsing (`Class::vt<T>` → dot + structmember + template args).
- [x] **`CXXMaterializeVariableTemplateExpression`** exported; unwrap init / expr_init / cast; structmember + dot/arrow.
- [x] **`CXXInstantiateVariableTemplateValue`:** drop wrong `auto` deduced concrete; accept non-dependent init path when concrete is null.
- [x] **`CXXPrepareStdGetVariableTemplateInitializer`** / **`CXXTryFoldStdGetFromTupleCall`** for braced tuple + `std::get` (partial).
- [x] Variable-template clone: set `parser.template_substitution_source/target` from `var_template->static_data_member_class`.
- [x] Materialize before `NormalConversion` on variable definitions (`semantics.c`; merged with master’s const ref-init binding).
- [x] Commit **`5bc30f04`** on `feature/subspace`; merge **`bf3598f5`** into `master`.

**Key files (latest materialization commit):**

- `c_compiler/frontend/semantic/expr_semantics.c` / `.h`
- `c_compiler/frontend/semantic/semantics.c`
- `c_compiler/frontend/syntax/expr_parser.c`
- `c_compiler/frontend/syntax/type_template_instantiate.c`

**Transcript (full context):**  
`/Users/FZDSZZ/.cursor/projects/Users-FZDSZZ-c-compiler/agent-transcripts/ebb5446d-ff2f-4d1e-b947-2277c0b92651/ebb5446d-ff2f-4d1e-b947-2277c0b92651.jsonl`

**Subspace worktree (optional repros):** `/Users/FZDSZZ/c_compiler-subspace` on `feature/subspace` (shares git with main repo).

## Todo (pick up here)

### P0 — finish the protobuf pattern

- [x] **CTAD `std::tuple{…}` inside cloned variable-template initializers.** `std::get` was folding to the element, then overwriting that element with the still-dependent return type of the primary `get(integer_sequence)` overload (`T`). The element type is kept when the call type is still dependent. A successful replacement of `Class::vt<T>` is what `AnalyzeExpression` returns (the old node was deleted and then written back). Namespace-scope `auto` variable templates are not type-checked at the definition.
- [x] **`tmp/kunion_assign.cc`**, **`tmp/kunion_only.cc`**, **`tmp/kunion_call.cc`**, **`tmp/vt_get0.cc`**, **`tmp/vt_get_ns.cc`** compile (`-c`). Link needs `//:libc_x86_64`, which currently aborts in `GetLoadOpcodeForType` while compiling `libc/eh_frame.c` (pre-existing on this tree, including without these edits).
- [ ] Re-run **`extension_set.cc`** (or `./tmp/compile_protobuf.sh <file.cc>` if present in subspace worktree) once libc links.

### P1 — verify / harden

- [ ] After fix, run C++ exec tests per **`.cursor/skills/run-cxx-exec-tests/SKILL.md`**. Blocked: `//:libc_x86_64` aborts compiling `libc/eh_frame.c` (`GetLoadOpcodeForType`, null type). Not caused by the variable-template edits.
- [ ] Confirm no regression on cases that already pass:
  - `constexpr int k = 1` variable template
  - `auto k = (T)0`
  - `auto k = (float E::*)&E::f`
  - `get<T E::*>(tuple<T E::*>{…})` (explicit typed tuple)

### P2 — housekeeping (optional)

- [ ] Push `master` to `origin` if user wants (**23 commits** ahead of `origin/master` at handoff time).
- [ ] Do **not** commit unrelated local WIP on main repo unless asked (e.g. unstaged `c_compiler/backend/expr_codegen.c` `start_lifetime` void/nop tweak).

## Symptoms when broken

- RHS stays type **`const`** (declared `auto`) instead of member pointer.
- **`CXXInstantiateVariableTemplateValue`** returns null.
- Linker undefined symbol like `_ZN9Extension12kUnionMemberE`.
- Errors such as: cannot convert from `'const '` to `'Extension::* float'`.

**Misleading signal:** In-class `use()` can look fine with `-c` only because dead code is stripped; calling `use()` from `main` still shows the bug.

## Repro status (last known)

| Repro | Status |
|--------|--------|
| Simple `constexpr int` variable template | OK |
| `auto k = (T)0` | OK |
| `auto k = (float E::*)&E::f` | OK |
| `get<T E::*>(tuple<T E::*>{…})` | OK |
| **`std::tuple{…}` + `std::get` in variable-template init** | Compiles (`-c`), in-class and namespace |

## Build / compile one file

```bash
cd /Users/FZDSZZ/c_compiler   # or c_compiler-subspace
bazel build //:davecc
# Single-file (adjust path):
bazel run //:davecc -- -target x86_64 -static -std=c++20 -isystem libc/include -c tmp/kunion_assign.cc -o /tmp/kunion.o
```

## Root cause (partial, for next agent)

Variable-template initializer **cloning** + **CTAD braced `std::tuple`** does not lower/fold the same way as at namespace scope or with explicit `std::tuple<T…>{…}`. Investigation should compare:

- `type_template_instantiate.c` clone path and substitution context
- `CXXPrepareStdGetVariableTemplateInitializer` / `LowerCXXBracedInitToTarget`
- `CXXTryFoldStdGetFromTupleCall` after `AnalyzeExpression` on the cloned init

Prefer **compiler fix** over changing protobuf sources.
