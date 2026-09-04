// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class T>
struct non_deduced {
  using type = T;
};

template <class... Types>
struct descriptor {};

template <class... Args>
int argument_count(
    const typename non_deduced<descriptor<Args...>>::type&,
    Args&&...) {
  return sizeof...(Args);
}

template <class... Args>
int argument_count(
    int,
    const typename non_deduced<descriptor<Args...>>::type& description,
    Args&&... args) {
  return argument_count(description, static_cast<Args&&>(args)...);
}

int main() {
  descriptor<int> description;
  return argument_count(0, description, 42) == 1 ? 0 : 1;
}
