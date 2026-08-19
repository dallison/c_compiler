// RUN: -std=c++17

#include <memory>
#include <version>

#ifdef __cpp_lib_constexpr_dynamic_alloc
#error "__cpp_lib_constexpr_dynamic_alloc must not be defined before C++20"
#endif

struct tracked {
  ~tracked();
};

void destroy(tracked* value) {
  std::destroy_at(value);
}
