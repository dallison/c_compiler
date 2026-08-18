import trivial_union_lifetime;

static_assert(module_union_default_constructor_is_trivial);
static_assert(module_union_destructor_is_trivial);
static_assert(module_union_object.elements[1].number == 20);
static_assert(module_union_object.elements[3].number == 22);
static_assert(module_duplicate_union_object.second.number == 42);
static_assert(
    module_union_size<module_union_storage{42}>() ==
    sizeof(module_union_storage));

int main() {
  return 0;
}
