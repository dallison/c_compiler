// A local direct-initialized with a dependent specialization whose primary
// template is only declared (`template <class F> class Action;`) keeps its
// constructor arguments: at parse time no constructor is visible, so the
// arguments must survive until the function template is instantiated.
#include <stddef.h>
#include <stdio.h>

#include <tuple>
#include <utility>

template <typename F> struct Act;
template <typename R, typename... Args> struct Act<R(Args...)> {
  Act() = default;
  template <typename G> Act(G&& g) : fn(std::forward<G>(g)) {}
  Act(int a, int b) : sum(a + b) {}
  R (*fn)(Args...) = nullptr;
  int sum = 0;
};

template <typename F> struct Pair;
template <typename R> struct Pair<R(int)> {
  int first;
  int second;
};

bool Negative(int x) { return x < 0; }
typedef bool (*Predicate)(int);

template <typename R> int FromPointer(Predicate p) {
  Act<R(int)> action(p);
  return action.fn != nullptr && action.fn(-1);
}

template <typename R> int TwoArguments() {
  Act<R(int)> action(3, 4);
  return action.sum;
}

template <typename R> int Braced(Predicate p) {
  Act<R(int)> action{p};
  return action.fn != nullptr;
}

template <typename R> int Aggregate() {
  Pair<R(int)> pair{5, 6};
  return pair.first + pair.second;
}

// gmock's WithArgs: the inner action's signature selects some of the outer
// arguments.
template <typename Inner, size_t... I> struct WithArgs {
  Inner inner;
  template <typename R, typename... Args>
  using InnerSignature =
      R(typename std::tuple_element<I, std::tuple<Args...>>::type...);

  template <typename R, typename... Args> int Convert() const {
    Act<InnerSignature<R, Args...>> converted(inner);
    return converted.fn != nullptr && converted.fn(-2);
  }
};

int main() {
  int failures = 0;
  if (FromPointer<bool>(Negative) != 1) failures |= 1;
  if (TwoArguments<bool>() != 7) failures |= 2;
  if (Braced<bool>(Negative) != 1) failures |= 4;
  if (Aggregate<bool>() != 11) failures |= 8;
  WithArgs<Predicate, 1> with_args{Negative};
  if (with_args.Convert<bool, double, int>() != 1) failures |= 16;
  if (failures == 0) printf("ok\n");
  return failures;
}
