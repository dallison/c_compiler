// std::get on a tuple produced by a function call reads that tuple, not the
// call's arguments, including inside a pack expansion (googlemock's
// internal::Apply) and on a materialized prvalue.  Folding get<I>(tuple(...))
// must keep the dropped elements' side effects.
#include <cstddef>
#include <tuple>
#include <utility>

std::tuple<int, int> make_pair_of(int a, int b) { return std::tuple<int, int>(a, b); }
std::tuple<int, int>&& pass(std::tuple<int, int>& t) { return std::move(t); }

int sum2(int a, int b) { return a * 10 + b; }

template <std::size_t... I>
int apply_sum(std::tuple<int, int>&& args, std::index_sequence<I...>) {
  return sum2(std::get<I>(std::move(args))...);
}

template <std::size_t... I>
int apply_sum_via_call(std::tuple<int, int>& args, std::index_sequence<I...>) {
  return sum2(std::get<I>(pass(args))...);
}

void take(char*) {}

template <std::size_t... I>
void void_apply(std::tuple<char*>&& args, std::index_sequence<I...>) {
  return take(std::get<I>(std::move(args))...);
}

int counter = 0;
int bump() { return ++counter; }

int main() {
  int failures = 0;
  std::tuple<int, int> t(5, 6);
  int x = 1, y = 9;
  if (std::get<0>(pass(t)) != 5) failures |= 1;
  if (std::get<1>(std::move(t)) != 6) failures |= 2;
  if (std::get<0>(make_pair_of(y, x)) != 9) failures |= 4;
  if (std::get<1>(make_pair_of(y, x)) != 1) failures |= 8;
  if (std::get<0>(std::make_tuple(y, x)) != 9) failures |= 16;
  if (std::get<1>(std::forward_as_tuple(x, y)) != 9) failures |= 32;
  if (apply_sum(std::tuple<int, int>(3, 4), std::index_sequence<0, 1>()) != 34) failures |= 64;
  std::tuple<int, int> u(7, 8);
  if (apply_sum_via_call(u, std::index_sequence<0, 1>()) != 78) failures |= 128;
  char c = 0;
  void_apply(std::tuple<char*>(&c), std::index_sequence<0>());
  if (std::get<0>(std::tuple<int, int>(bump(), 2)) != 1 || counter != 1) failures |= 256;
  if (std::get<0>(std::tuple<int, int>(3, bump())) != 3 || counter != 2) failures |= 512;
  return failures;
}
