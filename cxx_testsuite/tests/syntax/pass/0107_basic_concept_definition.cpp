// RUN: -std=c++20
template <typename T>
concept Always = true;

template <typename T>
concept NonZero = sizeof(T) != 0;

template <typename T>
concept HasSizeof = requires { sizeof(T); };

template <typename T>
concept Large = sizeof(T) > 4;

template <typename T>
concept HasMissingMember = requires { T::missing; };

template <typename T>
concept HasCompoundSizeof = requires { { sizeof(T) }; };

template <typename T>
concept HasCompoundMissingMember = requires { { T::missing }; };

template <typename T>
concept Addable = requires(T a, T b) { { a + b }; };

template <typename T>
concept HasLocalMissingMember = requires(T value) { { value.missing }; };

template <typename T>
concept NestedAddable = requires(T a, T b) {
  requires Addable<T>;
  requires sizeof(T) != 0;
  { a + b };
};

template <typename T>
concept NestedLarge = requires {
  requires Large<T>;
};

int concept_noexcept_call(int value) noexcept;
int concept_may_throw_call(int value);

template <typename T>
concept NoexceptAddable = requires(T a, T b) { { a + b } noexcept; };

template <typename T>
concept HasNoexceptCall = requires(T value) {
  { concept_noexcept_call(value) } noexcept;
};

template <typename T>
concept HasThrowingCall = requires(T value) {
  { concept_may_throw_call(value) } noexcept;
};

template <typename T, int N>
concept SizeIs = sizeof(T) == N;

int concept_returns_int(int value);
char concept_returns_char(char value);

template <typename T>
concept ReturnsIntSized = requires(T value) {
  { concept_returns_int(value) } -> SizeIs<sizeof(int)>;
};

template <typename T>
concept ReturnsCharIntSized = requires(T value) {
  { concept_returns_char(value) } -> SizeIs<sizeof(int)>;
};

struct ConceptHasType {
  typedef int type;
};

template <typename T>
concept HasNestedType = requires { typename T::type; };

template <Always T>
concept WrappedAlways = Always<T>;

template <typename T>
requires Always<T>
int constrained_identity(T value) {
  return value;
}

template <typename T>
requires Large<T>
int constrained_or_fallback(T value) {
  return 1;
}

int constrained_or_fallback(char value) {
  return 2;
}

template <typename T>
char* constrained_template_pick(T value) {
  return (char*)0;
}

template <typename T>
requires Large<T>
long long* constrained_template_pick(T value) {
  return (long long*)0;
}

template <typename T>
requires Large<T>
char* constrained_subsumption_pick(T value) {
  return (char*)0;
}

template <typename T>
requires Large<T> && Addable<T>
long long* constrained_subsumption_pick(T value) {
  return (long long*)0;
}

template <typename T>
requires (Large<T>)
char* parenthesized_subsumption_pick(T value) {
  return (char*)0;
}

template <typename T>
requires (Large<T> && Addable<T>)
long long* parenthesized_subsumption_pick(T value) {
  return (long long*)0;
}

template <Large T>
long long* constrained_parameter_pick(T value) {
  return (long long*)0;
}

char* constrained_parameter_pick(char value) {
  return (char*)0;
}

long long* abbreviated_large_pick(Large auto value) {
  return (long long*)0;
}

char* abbreviated_large_pick(char value) {
  return (char*)0;
}

int abbreviated_identity(auto value) {
  return sizeof(value);
}

int abbreviated_pair(auto left, auto right) {
  return sizeof(left) + sizeof(right);
}

long long* abbreviated_trailing_requires(auto value)
    requires Large<decltype(value)> {
  return (long long*)0;
}

char* abbreviated_trailing_requires(char value) {
  return (char*)0;
}

long long* abbreviated_mixed_requires(Large auto value)
    requires Addable<decltype(value)> {
  return (long long*)0;
}

static_assert(Always<int>);
static_assert(NonZero<int>);
static_assert(HasSizeof<int>);
static_assert(Large<long long>);
static_assert(!Large<char>);
static_assert(!HasMissingMember<int>);
static_assert(HasCompoundSizeof<int>);
static_assert(!HasCompoundMissingMember<int>);
static_assert(Addable<int>);
static_assert(!HasLocalMissingMember<int>);
static_assert(NestedAddable<int>);
static_assert(NestedLarge<long long>);
static_assert(!NestedLarge<char>);
static_assert(NoexceptAddable<int>);
static_assert(HasNoexceptCall<int>);
static_assert(!HasThrowingCall<int>);
static_assert(ReturnsIntSized<int>);
static_assert(!ReturnsCharIntSized<char>);
static_assert(HasNestedType<ConceptHasType>);
static_assert(!HasNestedType<int>);
static_assert(WrappedAlways<int>);

int main(void) {
  long long* large_pick = constrained_template_pick((long long)0);
  char* small_pick = constrained_template_pick((char)0);
  long long* more_constrained_pick =
      constrained_subsumption_pick((long long)0);
  long long* parenthesized_more_constrained_pick =
      parenthesized_subsumption_pick((long long)0);
  long long* constrained_param_large =
      constrained_parameter_pick((long long)0);
  char* constrained_param_small = constrained_parameter_pick((char)0);
  long long* abbreviated_large = abbreviated_large_pick((long long)0);
  char* abbreviated_small = abbreviated_large_pick((char)0);
  int abbreviated_char = abbreviated_identity((char)0);
  int abbreviated_ll = abbreviated_identity((long long)0);
  int abbreviated_two = abbreviated_pair((char)0, (long long)0);
  long long* abbreviated_trailing_large =
      abbreviated_trailing_requires((long long)0);
  char* abbreviated_trailing_small = abbreviated_trailing_requires((char)0);
  long long* abbreviated_mixed_large =
      abbreviated_mixed_requires((long long)0);
  auto lambda_large = [](Large auto value) { return sizeof(value); };
  int lambda_large_size = lambda_large((long long)0);
  auto lambda_trailing_large =
      [](auto value) requires Large<decltype(value)> { return sizeof(value); };
  int lambda_trailing_size = lambda_trailing_large((long long)0);
  auto lambda_mixed_large = [](Large auto value)
      requires Addable<decltype(value)> { return sizeof(value); };
  int lambda_mixed_size = lambda_mixed_large((long long)0);
  return constrained_identity(0) + constrained_or_fallback('x') +
         (large_pick != (long long*)0) + (small_pick != (char*)0) +
         (more_constrained_pick != (long long*)0) +
         (parenthesized_more_constrained_pick != (long long*)0) +
         (constrained_param_large != (long long*)0) +
         (constrained_param_small != (char*)0) +
         (abbreviated_large != (long long*)0) +
         (abbreviated_small != (char*)0) + abbreviated_char +
         abbreviated_ll + abbreviated_two +
         (abbreviated_trailing_large != (long long*)0) +
         (abbreviated_trailing_small != (char*)0) +
         (abbreviated_mixed_large != (long long*)0) + lambda_large_size +
         lambda_trailing_size + lambda_mixed_size;
}
