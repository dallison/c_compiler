import meta_synthesis;

#include <meta>

static_assert(imported_value == imported_value);
static_assert(std::meta::is_enumerator_spec(imported_negative));
static_assert(static_cast<int>(imported_enum::negative) == -3);
static_assert(static_cast<int>(imported_enum::zero) == -2);

constexpr auto imported_enumerators =
    std::meta::enumerators_of(^^imported_enum);
static_assert(imported_enumerators.size() == 2);
static_assert(std::meta::is_enumerator(imported_enumerators[0]));
static_assert(static_cast<int>(std::meta::extract<imported_enum>(
                  std::meta::constant_of(imported_enumerators[1]))) == -2);

int main() { return 0; }
