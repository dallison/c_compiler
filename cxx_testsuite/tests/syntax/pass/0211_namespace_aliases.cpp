// RUN: -std=c++20

namespace library {
namespace implementation {
struct value_type {
  int value;
};
}
}

namespace impl = library::implementation;
namespace implementation_alias = impl;
namespace impl = implementation_alias;

impl::value_type global_value{1};

namespace client {
namespace local_impl = ::library::implementation;
local_impl::value_type namespaced_value{2};
}

int use_block_alias() {
  namespace local_impl = library::implementation;
  local_impl::value_type local{3};
  return local.value;
}

namespace exports_alias {
namespace public_impl = ::library::implementation;
}
using namespace exports_alias;
public_impl::value_type imported_value{4};
