// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <meta>

using std::meta::info;

int reflected_global = 11;

int reflected_function(int value) {
  return value + 3;
}

struct reflected_object {
  int field;
};

template <info type>
struct spliced_holder {
  typename[:type:] value;
};

constexpr info global_info = ^^reflected_global;
constexpr info function_info = ^^reflected_function;
constexpr info field_info = ^^reflected_object::field;

int main() {
  spliced_holder<^^long> holder{17};
  reflected_object object{19};
  int* global_address = &[:global_info:];
  auto function_address = &[:function_info:];
  int direct_result = [:function_info:](4);
  return (holder.value != 17) + (*global_address != 11) +
         (direct_result != 7) + (function_address(5) != 8) +
         (object.[:field_info:] != 19);
}
