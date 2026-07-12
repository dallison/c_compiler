// RUN: -std=c++20

namespace very_long_namespace_name {
namespace nested {
constexpr int value = 37;
}
}

namespace short_name = very_long_namespace_name::nested;
namespace chained_name = short_name;
namespace short_name = chained_name;

static_assert(short_name::value == 37);
static_assert(chained_name::value == 37);

namespace alias_owner {
namespace local_name = ::very_long_namespace_name::nested;
int read() {
  return local_name::value;
}
}

namespace imported_alias_owner {
namespace imported_name = ::very_long_namespace_name::nested;
}
using namespace imported_alias_owner;

int read_block_alias() {
  namespace block_name = very_long_namespace_name::nested;
  return block_name::value;
}

int main() {
  return alias_owner::read() + imported_name::value + read_block_alias() == 111
             ? 0
             : 1;
}
