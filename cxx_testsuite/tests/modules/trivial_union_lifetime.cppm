module;

#include <memory>
#include <type_traits>

export module trivial_union_lifetime;

export struct module_union_value {
  int number;
};

export union module_union_storage {
  int fallback;
  module_union_value elements[4];
};

export union module_duplicate_union_storage {
  module_union_value first;
  module_union_value second;
};

export constexpr module_union_storage make_module_union_storage() {
  module_union_storage result{.fallback = 17};
  std::start_lifetime(result.elements);
  std::construct_at(&result.elements[1], module_union_value{20});
  std::construct_at(&result.elements[3], module_union_value{22});
  return result;
}

export constexpr module_union_storage module_union_object =
    make_module_union_storage();
static_assert(module_union_object.elements[1].number == 20);
static_assert(module_union_object.elements[3].number == 22);

export constexpr module_duplicate_union_storage
make_module_duplicate_union_storage() {
  module_duplicate_union_storage result;
  std::start_lifetime(result.second);
  std::construct_at(&result.second, module_union_value{42});
  return result;
}

export constexpr module_duplicate_union_storage module_duplicate_union_object =
    make_module_duplicate_union_storage();
static_assert(module_duplicate_union_object.second.number == 42);

export constexpr bool module_union_default_constructor_is_trivial =
    std::is_trivially_default_constructible_v<module_union_storage>;
export constexpr bool module_union_destructor_is_trivial =
    std::is_trivially_destructible_v<module_union_storage>;

export template <module_union_storage Value>
struct module_union_token {
  static constexpr int sum =
      Value.elements[1].number + Value.elements[3].number;
};

export template <module_union_storage Value>
constexpr int module_union_size() {
  return sizeof(Value);
}

static_assert(std::is_trivially_default_constructible_v<module_union_storage>);
static_assert(std::is_trivially_destructible_v<module_union_storage>);
