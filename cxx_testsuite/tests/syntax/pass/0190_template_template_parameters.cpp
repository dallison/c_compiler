// RUN: -std=c++20

template <class T>
struct unary {};

template <class T, class U = int>
struct with_default {};

template <class T>
using unary_alias = unary<T>;

template <template <class> class Named>
struct named_parameter {
  Named<int> value;
};

template <template <class> class>
struct unnamed_parameter {};

template <template <class, class = int> class C>
struct multiple_parameters {
  C<int> value;
};

template <template <class> class C = unary>
struct default_argument {
  C<int> value;
};

template <template <class> class... Cs>
struct parameter_pack {};

template <template <class...> class C>
struct variadic_signature {
  C<int, long> value;
};

template <template <template <class> class> class Outer>
struct nested_parameter {};

template <template <class> class Inner>
struct outer {};

named_parameter<unary> named;
unnamed_parameter<unary_alias> unnamed;
multiple_parameters<with_default> multiple;
default_argument<> by_default;
parameter_pack<unary, unary_alias> packed;
variadic_signature<with_default> variadic_match;
nested_parameter<outer> nested;
