# Cursor Restart Continuation Plan

This note captures the current context so work can resume after restarting Cursor without rebuilding the thread history.

## Current Goal

Continue making the DaveCC C++20 template groundwork more complete while keeping the C++ syntax and x86_64 execution suites green. The current focus is dependent nested class-template types, including mixed type/non-type owners such as `SizedNestedOwner<T, N>::Inner` and `nested_ns::SizedOwner<T, N>::Inner`, flowing through function template deduction, template-id arguments, pointers, references, arrays, const-qualified forms, member layouts, and return types.

## Current Working Tree

Current continuation-slice files:

- `c_compiler/frontend/syntax/type.c`
- `c_compiler/frontend/syntax/syntax.c`
- `c_compiler/backend/expr_codegen.c`
- `cxx_testsuite/tests/exec/0020_class_template_instantiation.cpp`
- `cxx_testsuite/tests/syntax/pass/0049_class_template_instantiation.cpp`
- `CONTINUATION_PLAN.md`

There are also many other pre-existing modified/untracked files in the working tree. Do not assume they belong to the current slice without checking `git diff`.

Do not revert these changes. They are the current in-progress implementation and tests. The user explicitly said we do not need to commit every slice, so keep continuing unless asked to commit.

## Last Committed Point

The last commit made in this session was:

```text
8533d22 Expand C++ dependent signature coverage.
```

Everything below is currently uncommitted.

## Implemented Since Last Commit

### Dependent Nested Signature Deduction

Added and verified deduction for helpers like:

```cpp
template <typename T>
typename NestedOwner<T>::Inner make_dependent_nested(T value);

template <typename T>
T read_dependent_nested(typename NestedOwner<T>::Inner value);
```

Important implementation points:

- `InstantiateSimpleClassTemplate` now preserves class-template instantiation metadata with `template_origin` and `template_arguments`.
- `TypeRecordToTemplateKeyString` distinguishes template placeholder types using `$T<index>` so `Holder<T>` and `Holder<int>` do not collide in the class-template instantiation cache.
- Function-template deduction can match class-template argument lists before falling back to structural member deduction.
- Concrete function-template type arguments are normalized so direct placeholder type arguments do not remain accidentally dependent.

### Nested Struct Substitution

Dependent nested structs now get a substituted synthetic struct layout when needed. This fixes cases where `NestedOwner<int>::Inner::value` must be concretely `int`, not the original `T` placeholder.

Important implementation points:

- `StructContainsTemplateParameter` only treats non-static data members as structural dependency.
- `SubstituteNestedStructTemplateParameters` clones data-member layout with substituted member types.
- Member functions are intentionally not copied into these synthetic nested layouts to avoid ownership/teardown problems.

### Struct Expression Initialization

`AnalyzeInitializer` now accepts compatible cloned struct layouts for expression initialization, rather than requiring identical `Struct*` identity. This is needed when function template return types and declared variable types are equivalent substituted nested structs but not pointer-identical.

### Coverage Added

All of these are covered in both:

- `cxx_testsuite/tests/syntax/pass/0049_class_template_instantiation.cpp`
- `cxx_testsuite/tests/exec/0020_class_template_instantiation.cpp`

Covered forms include:

- Explicit and deduced calls involving `typename NestedOwner<T>::Inner`.
- Wrapped dependent parameters: `Holder<typename NestedOwner<T>::Inner>`.
- Namespaced wrapped dependent parameters: `Holder<typename nested_ns::Owner<T>::Inner>`.
- Wrapped dependent return types.
- Namespaced wrapped dependent return types.
- Pointer/reference deduction for `typename NestedOwner<T>::Inner*` and `typename NestedOwner<T>::Inner&`.
- Pointer/reference deduction for `typename nested_ns::Owner<T>::Inner*` and `typename nested_ns::Owner<T>::Inner&`.
- Wrapped pointer/reference deduction for `Holder<typename NestedOwner<T>::Inner>*` and `Holder<typename NestedOwner<T>::Inner>&`.
- Wrapped namespaced pointer/reference deduction for `Holder<typename nested_ns::Owner<T>::Inner>*` and `Holder<typename nested_ns::Owner<T>::Inner>&`.
- Direct array-parameter deduction for `typename NestedOwner<T>::Inner value[1]`.
- Wrapped array-parameter deduction for `Holder<typename NestedOwner<T>::Inner> value[1]`.
- Namespaced array-parameter deduction for `typename nested_ns::Owner<T>::Inner value[1]`.
- Wrapped namespaced array-parameter deduction for `Holder<typename nested_ns::Owner<T>::Inner> value[1]`.
- Const-qualified pointer/reference deduction for `const typename NestedOwner<T>::Inner*`, `const typename NestedOwner<T>::Inner&`, and the namespaced equivalents.
- Wrapped const-qualified pointer/reference deduction for `const Holder<typename NestedOwner<T>::Inner>*`, `const Holder<typename NestedOwner<T>::Inner>&`, and the namespaced equivalents.

### Qualified Static Member Functions On Dependent Nested Types

Fixed and verified qualified static member calls through dependent mixed type/non-type nested types:

```cpp
template <typename T, int N>
int read_qualified_sized_static_method(T value, int extra) {
  return SizedMethodOwner<T, N>::Inner::static_total(value, extra);
}
```

The namespaced equivalent (`nested_ns::SizedMethodOwner<T, N>::Inner::static_total`) is covered in both syntax and exec fixtures. Follow-on coverage also passes for helpers that take the dependent nested type as a parameter and call the static member through the qualified dependent owner:

```cpp
template <typename T, int N>
int read_qualified_sized_static_method_nested(
    typename SizedMethodOwner<T, N>::Inner value) {
  return SizedMethodOwner<T, N>::Inner::static_total(value.value,
                                                    value.data[2]);
}
```

Wrapped dependent nested values are also covered:

```cpp
template <typename T, int N>
int read_qualified_wrapped_sized_static_method_nested(
    Holder<typename SizedMethodOwner<T, N>::Inner> value) {
  return SizedMethodOwner<T, N>::Inner::static_total(value.value.value,
                                                    value.value.data[2]);
}
```

The same direct and wrapped qualified static forms pass for the `nested_ns::SizedMethodOwner<T, N>::Inner` equivalent.

Pointer/reference variants of the qualified static path are also covered:

```cpp
template <typename T, int N>
int read_qualified_sized_static_method_nested_pointer(
    typename SizedMethodOwner<T, N>::Inner* value) {
  return SizedMethodOwner<T, N>::Inner::static_total(value->value,
                                                    value->data[2]);
}

template <typename T, int N>
int read_qualified_sized_static_method_nested_reference(
    typename SizedMethodOwner<T, N>::Inner& value) {
  return SizedMethodOwner<T, N>::Inner::static_total(value.value,
                                                    value.data[2]);
}
```

The same pointer/reference qualified static forms pass for the namespaced owner.

Additional qualified static coverage added after that also passes in both the
focused syntax and x86_64 exec tests:

- Array parameters: `typename SizedMethodOwner<T, N>::Inner value[1]`.
- Const pointer/reference parameters: `const typename SizedMethodOwner<T, N>::Inner*`
  and `const typename SizedMethodOwner<T, N>::Inner&`.
- Wrapped pointer/reference parameters:
  `Holder<typename SizedMethodOwner<T, N>::Inner>*` and
  `Holder<typename SizedMethodOwner<T, N>::Inner>&`.
- Const wrapped pointer/reference parameters:
  `const Holder<typename SizedMethodOwner<T, N>::Inner>*` and
  `const Holder<typename SizedMethodOwner<T, N>::Inner>&`.
- Wrapped array and const wrapped array parameters.
- Local dependent typedef aliases, e.g.:

```cpp
template <typename T, int N>
int read_qualified_alias_sized_static_method_nested(
    typename SizedMethodOwner<T, N>::Inner value) {
  typedef typename SizedMethodOwner<T, N>::Inner Inner;
  return Inner::static_total(value.value, value.data[2]);
}
```

All of the above have namespaced equivalents through
`nested_ns::SizedMethodOwner<T, N>::Inner`.

Current compiler gap found by continuing coverage: assigning a qualified
dependent static member function to a function pointer segfaults the compiler
during the syntax suite. The new probes are present in both fixtures:

```cpp
template <typename T, int N>
int read_qualified_sized_static_method_function_pointer(
    typename SizedMethodOwner<T, N>::Inner value) {
  int (*fn)(T, int) = SizedMethodOwner<T, N>::Inner::static_total;
  return fn(value.value, value.data[2]);
}
```

and the namespaced equivalent:

```cpp
int (*fn)(T, int) = nested_ns::SizedMethodOwner<T, N>::Inner::static_total;
```

Verification result at this stopping point:

```sh
bazel test //cxx_testsuite:syntax --test_filter=0049_class_template_instantiation
```

fails with a `Segmentation fault: 11` while compiling
`pass/0049_class_template_instantiation.cpp`. The previously completed focused
checks were green before adding the function-pointer probe:

```sh
bazel test //cxx_testsuite:syntax --test_filter=0049_class_template_instantiation
bazel test //cxx_testsuite:exec_x86_64 --test_filter=0020_class_template_instantiation
```

Important implementation points:

- Qualified symbol lookup can now recursively resolve class-member prefixes for the expression-owner path while preserving conservative public prefix lookup behavior.
- Dependent non-type template arguments are preserved as dependent (`$N<index>` in instantiation keys) until concrete values are available.
- Cloned template bodies no longer replace dependent non-type identifiers with placeholder `0`; they only emit integer constants for concrete non-type arguments.
- Dependent member-function bodies are not queued for codegen until concretely instantiated.
- Static member-function identifiers are remapped during template body cloning when their dependent owner type becomes concrete.
- `TypeRecordCopy` now deep-copies function prototype symbols to avoid dangling prototype references in copied function types.
- Multiple template parameter deduction through `PairNestedOwner<T, U>::Inner`.
- Duplicate nested type names across class templates, such as `NestedOwner<T>::Inner` and `PairNestedOwner<T, U>::Inner`.
- Mixed type and non-type template parameter deduction through `SizedNestedOwner<T, N>::Inner`.
- Wrapped mixed type and non-type template parameter deduction through `Holder<typename SizedNestedOwner<T, N>::Inner>`.
- Pointer/reference, const pointer/reference, array-parameter, explicit return-type, and class-member layout coverage for `SizedNestedOwner<T, N>::Inner` and `Holder<typename SizedNestedOwner<T, N>::Inner>`.
- Namespaced mixed type/non-type coverage through `nested_ns::SizedOwner<T, N>::Inner`, including direct/wrapped value parameters, pointer/reference forms, const pointer/reference forms, array-parameter forms, and explicit return types.

### Direct Array Forms

Added and verified:

```cpp
template <typename T>
T read_dependent_nested_array(typename NestedOwner<T>::Inner value[1]) {
  return value[0].value;
}
```

The syntax fixture calls this with a one-element `NestedOwner<int>::Inner` array. The exec fixture also checks that the helper returns the assigned element value before including it in the aggregate return sum.

Implementation detail:

- Function-template deduction now accepts the usual array-to-pointer adjustment when the formal parameter is a pointer and the actual argument is an array. This is needed because function prototype parsing already decays array parameters to pointer types.

### Wrapped Array Forms

Added and verified:

```cpp
template <typename T>
T read_wrapped_dependent_nested_array(
    Holder<typename NestedOwner<T>::Inner> value[1]) {
  return value[0].value.value;
}
```

This was coverage-only after the direct array-to-pointer deduction fix.

### Namespaced Array Forms

Added and verified:

```cpp
template <typename T>
T read_namespaced_dependent_nested_array(
    typename nested_ns::Owner<T>::Inner value[1]) {
  return value[0].value;
}
```

This was coverage-only after the direct array-to-pointer deduction fix.

### Wrapped Namespaced Array Forms

Added and verified:

```cpp
template <typename T>
T read_wrapped_namespaced_dependent_array(
    Holder<typename nested_ns::Owner<T>::Inner> value[1]) {
  return value[0].value.value;
}
```

This was coverage-only after the direct array-to-pointer deduction fix.

### Const-Qualified Dependent Nested Parameters

Added and verified:

```cpp
template <typename T>
T read_const_dependent_nested_pointer(
    const typename NestedOwner<T>::Inner* value);

template <typename T>
T read_const_dependent_nested_reference(
    const typename NestedOwner<T>::Inner& value);
```

The same pointer/reference coverage was added for `typename nested_ns::Owner<T>::Inner`. This was coverage-only; parsing `const typename ...` and deduction from non-const actual lvalues/pointers already worked.

### Wrapped Const-Qualified Dependent Nested Parameters

Added and verified:

```cpp
template <typename T>
T read_const_wrapped_dependent_nested_pointer(
    const Holder<typename NestedOwner<T>::Inner>* value);

template <typename T>
T read_const_wrapped_dependent_nested_reference(
    const Holder<typename NestedOwner<T>::Inner>& value);
```

The same pointer/reference coverage was added for `Holder<typename nested_ns::Owner<T>::Inner>`. This was coverage-only after the previous deduction/substitution work.

### Multiple Template Parameters In Nested Owners

Added and verified:

```cpp
template <typename T, typename U>
struct PairNestedOwner {
  struct Inner {
    T first;
    U second;
  };
};

template <typename T, typename U>
int read_pair_dependent_nested(
    typename PairNestedOwner<T, U>::Inner value);
```

The helper is called with `PairNestedOwner<int, char>::Inner`, so deduction must infer both `T` and `U` through the nested type's substituted members.

### Duplicate Nested Type Names Across Class Templates

Implemented and verified C++ class-local tag scoping while parsing class members. This prevents nested tags such as `Inner` from leaking into the surrounding namespace/global tag scope and colliding with nested tags from another class template. The typedef-like nested member representation still exposes the nested type through qualified lookup.

Implementation detail:

- `ParseStructBody` now pushes a temporary local tag scope while parsing C++ class members and pops it after `ParseStructMembers`.
- The class tag itself remains in the surrounding scope; only member nested tags are scoped to the class body.

### Mixed Type And Non-Type Parameters In Nested Owners

Added and verified:

```cpp
template <typename T, int N>
struct SizedNestedOwner {
  struct Inner {
    T value;
    int data[N];
  };
};

template <typename T, int N>
int read_sized_dependent_nested(
    typename SizedNestedOwner<T, N>::Inner value);
```

The helper is called with `SizedNestedOwner<int, 3>::Inner`, so deduction has to infer both a type argument and a non-type array-bound argument through the substituted nested struct layout. This was coverage-only.

### Wrapped Mixed Type And Non-Type Nested Owner

Added and verified:

```cpp
template <typename T, int N>
int read_wrapped_sized_dependent_nested(
    Holder<typename SizedNestedOwner<T, N>::Inner> value);
```

The helper is called with `Holder<SizedNestedOwner<int, 3>::Inner>`, so deduction has to recover both `T` and `N` through the wrapper template argument.

Implementation detail:

- `TypeRecordToTemplateKeyString` now includes concrete `Struct*` identity for struct/union types. This prevents class-template instantiation cache collisions between same-spelled nested types like `NestedOwner<int>::Inner` and `SizedNestedOwner<int, 3>::Inner` when they appear as template arguments to another class template such as `Holder<...>`.

Many later slices were coverage-only after the core deduction/substitution fixes landed.

### Mixed Type/Non-Type Coverage Streak

After the wrapped mixed owner cache-key fix, these coverage-only slices were added and verified without requiring new compiler modifications:

- Direct and wrapped pointer/reference forms for `SizedNestedOwner<T, N>::Inner`.
- Direct and wrapped array-parameter forms for `SizedNestedOwner<T, N>::Inner`.
- Direct and wrapped const pointer/reference forms for `SizedNestedOwner<T, N>::Inner`.
- Direct and wrapped explicit return-type helpers for `SizedNestedOwner<T, N>::Inner`.
- Class-template member layout coverage with `UsesSizedNested<T, N>` and `UsesWrappedSizedNested<T, N>`.
- Namespaced mixed owner coverage with `nested_ns::SizedOwner<T, N>::Inner`, including value parameters, pointer/reference forms, array-parameter forms, const pointer/reference forms, and explicit return types.
- Owner-array coverage with `SizedMemberOwner<T, N>` and `nested_ns::SizedMemberOwner<T, N>`.
- Member-function coverage with `SizedMethodOwner<T, N>::Inner::total()` and `nested_ns::SizedMethodOwner<T, N>::Inner::total()`, including direct, wrapped, pointer, reference, direct array, and wrapped array forms.

### Member Functions On Substituted Nested Types

The compiler gap exposed by `SizedMethodOwner<T, N>::Inner::total()` has been fixed:

- `SubstituteNestedStructTemplateParameters` now carries member functions onto synthetic substituted nested layouts using the existing template member-function instantiation path.
- `CloneTemplateFunctionBodyNode` remaps identifier symbols before substituting stale pre-clone identifier types, avoiding recursive re-instantiation of the same method-bearing nested struct.
- `RewriteClonedConstructorMemberCall` now avoids rewriting ordinary member calls such as `value.total()` when the substituted receiver already has a member by that name.
- Method-bearing synthetic nested structs get unique private tag names for mangling, preventing duplicate `_ZN5Inner...` symbols across unrelated substituted `Inner` layouts.

### Qualified Static Member Function Pointers

The function-pointer initializer and parameter gaps have been fixed:

- Template body local declarations now keep the substituted local symbol type alive instead of deleting the newly retained type after `NewSymbol`.
- `SubstituteTemplateParameters` retains the original `next` chain while substituting through it, avoiding aliasing destruction when copying pointer/function chains.
- Function prototype parameter types are substituted when a function type is cloned through the generic type substitution path.
- Function-pointer/type diagnostic helpers now tolerate malformed intermediate types instead of crashing while reporting.
- Function-template argument deduction now treats a function expression as matching a formal function pointer and recursively matches function return/prototype types.
- Template body cloning now substitutes explicit template arguments attached to identifier nodes, so nested calls such as `call<T, N>(...)` become concrete when the outer template is instantiated.
- Conditional expressions now normalize function-designator/function-pointer arms, and both-function-designator arms, to a common function-pointer type.
- Conditional expressions now also normalize function-designator/null-pointer arms to a common function-pointer type.
- The conditional function-pointer rule is factored through `TryAnalyzeConditionalFunctionPointer`, which classifies each arm once instead of using a long special-case ladder.
- Codegen for `&function` now returns the function symbol address directly instead of emitting an extra `addressof` around it.
- Class-template instantiation now preserves static data members through both
  simple instantiation and nested dependent substitution paths, and template
  argument deduction ignores static data members when deducing from object
  argument shapes.
- Stale generic template diagnostics were refined for wrong-kind class
  template arguments, inconsistent function-template deduction, and
  declaration-only function-template instantiation.

Additional green coverage was added for primary and namespaced
`SizedMethodOwner<T, N>::Inner::static_total`:

- Direct function pointer initializer.
- Local typedef function pointer initializer.
- Explicit address-of function pointer initializer.
- One-element local function-pointer array initializer.
- Function-pointer parameter, both implicit function-to-pointer and explicit `&` forms.
- Function-pointer assignment after declaration, both implicit function-to-pointer and explicit `&` forms.
- Conditional function-pointer selection across implicit and explicit `&` arms.
- Conditional function-pointer selection across two function designators.
- Passing a conditional-selected function pointer as a function-template argument.
- Returning a conditional-selected function pointer from a helper.
- Directly calling a conditional-selected function pointer expression.
- Conditional selection between a function designator and `0`, in both arm orders.
- Conditional selection between `&function` and `0`, in both arm orders.
- Conditional selection between a function designator and `nullptr`.
- Assignment, parameter passing, direct call, `auto`, `decltype`, comparison, return, array, and `Holder<int (*)(T, int)>` forms using function/null conditionals.
- Primary and namespaced class-template instantiation with static data
  members.

## Tests Last Run

The focused syntax and exec checks passed after the conditional expression and
static data-member fixes:

```sh
bazel test //cxx_testsuite:syntax --test_filter=0049_class_template_instantiation
bazel test //cxx_testsuite:exec_x86_64 --test_filter=0020_class_template_instantiation --nocache_test_results
```

Broader verification also passed:

```sh
bazel test //cxx_testsuite:syntax //cxx_testsuite:exec_x86_64 --nocache_test_results
bazel test //...
```

## Immediate Next Step

No current focused compiler gap is known after the latest slice. The previous
gaps, where one conditional arm was the qualified dependent static member
function and the other arm was the null pointer constant `0`, and where class
templates containing static data members were rejected or lost static metadata
during instantiation, are fixed:

```cpp
template <typename T, int N>
int read_qualified_sized_static_method_conditional_null_function_pointer(
    typename SizedMethodOwner<T, N>::Inner value, int pick) {
  int (*fn)(T, int) = pick ? SizedMethodOwner<T, N>::Inner::static_total : 0;
  return fn(value.value, value.data[2]);
}
```

The namespaced equivalent is also green. Next task, if approved: continue with
broader non-template feature work; the template-focused C++ syntax/exec suites
and full repository tests are green.

## Suggested Upcoming Slices

### 1. Additional Mixed Type/Non-Type Stress Coverage

The owner-array coverage passed:

```cpp
template <typename T, int N>
int read_sized_member_owner_array(SizedMemberOwner<T, N> owners[1]);
```

The member-function gap is fixed and this probe is now restored:

```cpp
template <typename T, int N>
struct SizedMethodOwner {
  struct Inner {
    T value;
    int data[N];
    int total(void) { return value + data[2]; }
  };
};
```

Const member-function coverage now passes. Static member-function calls through objects, pointers, references, arrays, wrappers, and qualified dependent nested names all pass, including the namespaced equivalents.

## Likely Risks

- Class-template instantiation metadata is now retained on instantiated `TypeRecord`s. Watch for dependency checks that accidentally interpret concrete template arguments as still dependent.
- `TypeRecordToTemplateKeyString` now gives placeholders distinct `$T<index>` cache keys. Watch for any tests that compare template instantiation tag names or assume old string shapes.
- Synthetic nested struct substitution now clones data members and instantiates member functions, but uses private synthetic tag names for method-bearing substituted layouts. Watch for any tests that depend on exact mangled owner names for these synthetic nested structs.
- Struct expression initialization now permits equivalent cloned layouts, not just identical `Struct*`. This is useful for template substitution but should be watched for overly-permissive aggregate conversions.
- Most exec fixture additions require balancing the final constant. If an exec test fails with only a run exit value and no compile diagnostic, first adjust the balance by the observed exit value.

## Useful Verification Commands

Focused checks:

```sh
bazel test //cxx_testsuite:syntax --test_filter=0049_class_template_instantiation
bazel test //cxx_testsuite:exec_x86_64 --test_filter=0020_class_template_instantiation
```

Broader C++ checks:

```sh
bazel test //cxx_testsuite:syntax
bazel test //cxx_testsuite:exec_x86_64
```

Full repo check when needed:

```sh
bazel test //...
```

