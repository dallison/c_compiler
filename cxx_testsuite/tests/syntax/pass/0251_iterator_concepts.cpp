// RUN: -std=c++20
// C++20 iterator concepts, associated types, and the ranges::iter_move/iter_swap
// CPOs.  Also pins several frontend fixes uncovered while building these:
//   * pointer difference is the signed ptrdiff_t (not unsigned);
//   * pointer arithmetic drops top-level cv on the prvalue result;
//   * a concrete declaration-only template call (std::declval<T>()) used as an
//     argument is not treated as a dependent functor call;
//   * is_constructible<T, const T> selects the copy constructor.
#include <iterator>
#include <type_traits>

using namespace std;

// Pointer difference is signed ptrdiff_t.
static_assert(is_same_v<decltype(declval<int*>() - declval<int*>()), ptrdiff_t>,
              "pointer difference is signed");
static_assert(is_same_v<decltype(declval<int*>() - declval<int*>()), long>,
              "ptrdiff_t is long");

// Pointer arithmetic yields a cv-unqualified prvalue.
static_assert(is_same_v<decltype(declval<int* const&>() + declval<long>()), int*>,
              "p + n drops top-level const");
static_assert(is_same_v<decltype(declval<long>() + declval<int* const&>()), int*>,
              "n + p drops top-level const");
static_assert(is_same_v<decltype(declval<int* const&>() - declval<long>()), int*>,
              "p - n drops top-level const");

// is_constructible from a const prvalue selects the copy constructor.
struct Copyable {
  int* p;
};
static_assert(is_constructible_v<Copyable, const Copyable>,
              "constructible from const prvalue");
static_assert(copy_constructible<Copyable>, "copy_constructible");

// Raw pointers satisfy the whole iterator hierarchy.
static_assert(input_or_output_iterator<int*>, "ptr io iterator");
static_assert(input_iterator<int*>, "ptr input");
static_assert(forward_iterator<int*>, "ptr forward");
static_assert(bidirectional_iterator<int*>, "ptr bidir");
static_assert(random_access_iterator<int*>, "ptr random");
static_assert(contiguous_iterator<int*>, "ptr contiguous");
static_assert(sentinel_for<int*, int*>, "ptr sentinel");
static_assert(sized_sentinel_for<int*, int*>, "ptr sized sentinel");

// Associated types.
static_assert(is_same_v<iter_value_t<int*>, int>, "iter_value_t");
static_assert(is_same_v<iter_reference_t<int*>, int&>, "iter_reference_t");
static_assert(is_same_v<iter_difference_t<int*>, ptrdiff_t>, "iter_difference_t");
static_assert(is_same_v<iter_rvalue_reference_t<int*>, int&&>,
              "iter_rvalue_reference_t");

// A forward-only class iterator with member typedefs.
struct FwdIter {
  using difference_type = ptrdiff_t;
  using value_type = int;
  using pointer = int*;
  using reference = int&;
  using iterator_category = forward_iterator_tag;
  int* p;
  int& operator*() const { return *p; }
  FwdIter& operator++() { ++p; return *this; }
  FwdIter operator++(int) { FwdIter t = *this; ++p; return t; }
  bool operator==(const FwdIter&) const = default;
};

static_assert(input_iterator<FwdIter>, "fwd input");
static_assert(forward_iterator<FwdIter>, "fwd forward");
static_assert(!bidirectional_iterator<FwdIter>, "fwd not bidir");
static_assert(!random_access_iterator<FwdIter>, "fwd not random");

// Indirect callable concepts and projected.
struct Times2 { int operator()(int x) const { return x * 2; } };
struct IsPos { bool operator()(int x) const { return x > 0; } };
static_assert(indirectly_unary_invocable<Times2, int*>, "unary invocable");
static_assert(indirect_unary_predicate<IsPos, int*>, "unary predicate");
static_assert(is_same_v<iter_value_t<projected<int*, Times2> >, int>,
              "projected value_type");

int main() {
  int a[3] = {1, 2, 3};
  int* first = a;
  int* last = a + 3;
  // ranges::iter_move yields an xvalue.
  static_assert(
      is_same_v<decltype(ranges::iter_move(first)), int&&>, "iter_move xvalue");
  // ranges::iter_swap swaps through iterators.
  ranges::iter_swap(first, last - 1);
  return (last - first) - 3 + first[0] - 3 + a[2] - 1;
}
