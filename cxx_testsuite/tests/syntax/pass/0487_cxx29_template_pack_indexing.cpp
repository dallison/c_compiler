// RUN: -std=c++29

#if __cpp_pack_indexing != 202606L
#error "__cpp_pack_indexing must advertise template-name support"
#endif

template <typename T>
struct first_box {
  static constexpr int id = 1;
  T value;
};

template <typename T>
struct second_box {
  static constexpr int id = 2;
  T value;
};

template <template <typename> typename... Templates>
struct select_template {
  template <unsigned Index, typename T>
  using apply = Templates...[Index]<T>;
};

template <template <typename> typename Template>
struct template_holder {
  using type = Template<int>;
};

template <template <typename> typename... Templates>
struct select_template_argument {
  using second = template_holder<Templates...[1]>;
};

template <typename T>
constexpr int first_value = 10;

template <typename T>
constexpr int second_value = 20;

template <unsigned Index, typename T,
          template <typename> auto... Values>
constexpr int selected_value = Values...[Index]<T>;

template <typename T>
concept small_type = sizeof(T) <= sizeof(int);

template <typename T>
concept large_type = sizeof(T) > sizeof(int);

template <unsigned Index, typename T,
          template <typename> concept... Constraints>
concept selected_constraint = Constraints...[Index]<T>;

using selected_first = select_template<first_box, second_box>::apply<0, int>;
using selected_second = select_template<first_box, second_box>::apply<1, long>;
using held_second =
    select_template_argument<first_box, second_box>::second;

static_assert(selected_first::id == 1);
static_assert(selected_second::id == 2);
static_assert(held_second::type::id == 2);
static_assert(selected_value<0, int, first_value, second_value> == 10);
static_assert(selected_value<1, long, first_value, second_value> == 20);
static_assert(selected_constraint<0, char, small_type, large_type>);
static_assert(selected_constraint<1, long long, small_type, large_type>);

int main(void) {
  selected_first value{42};
  return sizeof(value) != sizeof(int);
}
