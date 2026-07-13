// RUN: -std=c++20
#include <concepts>
#include <type_traits>

struct DefaultOnly {
  int value;
  DefaultOnly() : value(0) {}
};

struct NoDefault {
  explicit NoDefault(int value) : value(value) {}
  int value;
};

struct DeletedCopy {
  DeletedCopy() {}
  DeletedCopy(const DeletedCopy&) = delete;
  DeletedCopy& operator=(const DeletedCopy&) = delete;
};

struct NonSwappable {
  NonSwappable() {}
  NonSwappable(const NonSwappable&) = delete;
  NonSwappable& operator=(const NonSwappable&) = delete;
};

struct AdlSwap {
  int value;
  AdlSwap() : value(0) {}
  AdlSwap(const AdlSwap&) = default;
  AdlSwap& operator=(const AdlSwap&) = default;
};

void swap(AdlSwap& left, AdlSwap& right) {
  int tmp = left.value;
  left.value = right.value;
  right.value = tmp;
}

struct RegularInt {
  int value;
  RegularInt() : value(0) {}
  RegularInt(int v) : value(v) {}
  bool operator==(const RegularInt& other) const {
    return value == other.value;
  }
  bool operator!=(const RegularInt& other) const {
    return !(*this == other);
  }
};

static_assert(std::destructible<int>);
static_assert(std::destructible<DefaultOnly>);
static_assert(!std::destructible<void>);

static_assert(std::constructible_from<int>);
static_assert(std::constructible_from<NoDefault, int>);
static_assert(!std::constructible_from<NoDefault>);

static_assert(std::default_initializable<int>);
static_assert(std::default_initializable<DefaultOnly>);
static_assert(!std::default_initializable<NoDefault>);

static_assert(std::move_constructible<int>);
static_assert(std::move_constructible<DefaultOnly>);
static_assert(!std::move_constructible<DeletedCopy>);

static_assert(std::copy_constructible<int>);
static_assert(!std::copy_constructible<DeletedCopy>);

static_assert(std::assignable_from<int&, int>);
static_assert(!std::assignable_from<const int&, int>);
static_assert(!std::assignable_from<DeletedCopy&, DeletedCopy>);

static_assert(std::swappable<int>);
static_assert(std::swappable<DefaultOnly>);
static_assert(std::swappable<AdlSwap>);
static_assert(!std::swappable<NonSwappable>);

static_assert(std::swappable_with<int&, int&>);
static_assert(std::swappable_with<AdlSwap&, AdlSwap&>);
static_assert(!std::swappable_with<NonSwappable&, NonSwappable&>);

static_assert(std::movable<int>);
static_assert(std::movable<DefaultOnly>);
static_assert(!std::movable<int&>);

static_assert(std::copyable<int>);
static_assert(!std::copyable<DeletedCopy>);

static_assert(std::semiregular<int>);
static_assert(std::semiregular<DefaultOnly>);
static_assert(std::regular<int>);
static_assert(std::regular<RegularInt>);

static_assert(!std::semiregular<NoDefault>);
static_assert(!std::regular<NoDefault>);

// Repeated concept evaluation must be idempotent across alternating results.
static_assert(std::regular<int>);
static_assert(!std::regular<NoDefault>);
static_assert(std::regular<int>);
static_assert(!std::semiregular<NoDefault>);
static_assert(std::copyable<DefaultOnly>);
static_assert(!std::copyable<DeletedCopy>);
static_assert(std::copyable<DefaultOnly>);

int main() {
  return 0;
}
