// RUN: -std=c++26

#include <meta>

struct entity {
  int value;
};

struct sample {
 public:
  int visible;
};

void reflected_function(int);

static_assert(std::meta::identifier_of(^^entity::value).size() == 5);
static_assert(!std::meta::identifier_of(^^entity::value).empty());
static_assert(std::meta::identifier_of(^^entity::value)[0] == 'v');
static_assert(std::meta::identifier_of(^^sample::visible)[0] == 'v');
static_assert(std::meta::identifier_of(^^sample::visible)[6] == 'e');
static_assert(std::meta::identifier_of(^^sample::visible) == "visible");
static_assert(std::meta::parameters_of(^^reflected_function).size() == 1);

template <class T>
struct holder {
  T member;
};

static_assert(std::meta::template_arguments_of(^^holder<int>).size() == 1);

constexpr auto id = std::meta::identifier_of(^^sample::visible);
static_assert(id.size() == 7);

void check_ranges() {
  (void)std::meta::template_arguments_of(^^holder<int>);
  (void)std::meta::members_of(^^entity, std::meta::access_context::unchecked());
}

int main() { return 0; }
