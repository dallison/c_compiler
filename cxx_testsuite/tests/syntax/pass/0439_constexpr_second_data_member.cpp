// RUN: -std=c++26

#include <stddef.h>

struct constexpr_pair {
  size_t first;
  size_t second;

  constexpr constexpr_pair(size_t left, size_t right)
      : first(left), second(right) {}
};

constexpr constexpr_pair pair_value(4, 32);

static_assert(pair_value.first == 4);
static_assert(pair_value.second == 32);
static_assert(constexpr_pair(4, 32).first == 4);
static_assert(constexpr_pair(4, 32).second == 32);

struct constexpr_pair_owner {
  constexpr_pair pair;

  constexpr explicit constexpr_pair_owner(constexpr_pair value) : pair(value) {}
};

constexpr bool nested_pair_copy_works() {
  constexpr_pair_owner owner(constexpr_pair(4, 32));
  return owner.pair.first == 4 && owner.pair.second == 32;
}

static_assert(nested_pair_copy_works());

constexpr constexpr_pair copy_pair(const constexpr_pair& value) {
  return value;
}

static_assert(copy_pair(pair_value).first == 4);
static_assert(copy_pair(pair_value).second == 32);

struct constexpr_pair_holder {
  constexpr_pair pair;

  constexpr explicit constexpr_pair_holder(constexpr_pair value) : pair(value) {}
  constexpr bool valid() const { return pair.first == 4; }
  constexpr constexpr_pair get() const { return pair; }
};

constexpr bool member_pair_return_works() {
  constexpr_pair_holder holder(constexpr_pair(4, 32));
  return holder.valid() && holder.get().first == 4 &&
         holder.get().second == 32;
}

static_assert(member_pair_return_works());

constexpr bool pair_by_value_works(constexpr_pair value) {
  return value.first == 4 && value.second == 32;
}

static_assert(pair_by_value_works(constexpr_pair(4, 32)));
