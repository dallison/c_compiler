// RUN: -std=c++23

#include <type_traits>
#include <version>

#ifdef __cpp_lib_start_lifetime
#error "__cpp_lib_start_lifetime must not be defined before C++26"
#endif

struct nontrivial {
  nontrivial();
  nontrivial(const nontrivial&);
  nontrivial& operator=(const nontrivial&);
  ~nontrivial();
};

union storage {
  nontrivial object;
  int integer;
};

static_assert(!std::is_default_constructible<storage>::value);
static_assert(!std::is_destructible<storage>::value);
static_assert(!std::is_copy_constructible<storage>::value);
static_assert(!std::is_copy_assignable<storage>::value);
