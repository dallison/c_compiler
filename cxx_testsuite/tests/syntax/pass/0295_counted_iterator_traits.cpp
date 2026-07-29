// RUN: -std=c++20

#include <iterator>
#include <type_traits>

struct FwdIter {
  using difference_type = ptrdiff_t;
  using value_type = int;
  using pointer = int*;
  using reference = int&;
  using iterator_category = std::forward_iterator_tag;

  int* p;

  int& operator*() const { return *p; }
  FwdIter& operator++() {
    ++p;
    return *this;
  }
  FwdIter operator++(int) {
    FwdIter t = *this;
    ++p;
    return t;
  }
  bool operator==(const FwdIter&) const = default;
};

struct InputIterator {
  using difference_type = long;
  using value_type = int;
  using pointer = int*;
  using reference = int&;
  using iterator_category = std::input_iterator_tag;

  int* pointer_value;

  int& operator*() const { return *pointer_value; }
  InputIterator& operator++() {
    ++pointer_value;
    return *this;
  }
  void operator++(int) { ++*this; }
  bool operator==(const InputIterator&) const = default;
};

using PointerCounted = std::counted_iterator<int*>;
using ConstPointerCounted = std::counted_iterator<const int*>;
using ForwardCounted = std::counted_iterator<FwdIter>;
using InputCounted = std::counted_iterator<InputIterator>;

static_assert(std::input_or_output_iterator<PointerCounted>);
static_assert(std::input_iterator<PointerCounted>);
static_assert(std::forward_iterator<PointerCounted>);
static_assert(std::bidirectional_iterator<PointerCounted>);
static_assert(std::random_access_iterator<PointerCounted>);

static_assert(
    std::is_same_v<std::iter_value_t<PointerCounted>, int>);
static_assert(
    std::is_same_v<std::iter_reference_t<PointerCounted>, int&>);
static_assert(
    std::is_same_v<std::iter_difference_t<PointerCounted>, ptrdiff_t>);
static_assert(std::is_same_v<
              typename std::iterator_traits<PointerCounted>::iterator_concept,
              std::contiguous_iterator_tag>);
static_assert(std::is_same_v<
              typename std::iterator_traits<ForwardCounted>::iterator_category,
              std::forward_iterator_tag>);

static_assert(
    std::is_constructible_v<ConstPointerCounted, const PointerCounted&>);
static_assert(std::is_assignable_v<ConstPointerCounted&, const PointerCounted&>);
static_assert(std::is_same_v<decltype(
                  std::declval<ForwardCounted&>()++), ForwardCounted>);
static_assert(
    std::is_same_v<decltype(std::declval<InputCounted&>()++), void>);

int main() {}
