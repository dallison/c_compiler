// RUN: -std=c++20
// EXPECT: cannot reopen namespace 'V' as inline
namespace N {
namespace V {}
}

namespace N {
inline namespace V {}
}

int main(void) {
  return 0;
}
