// RUN: -std=c++20
// EXPECT: cannot reopen anonymous namespace as inline
namespace {
int first;
}

inline namespace {
int second;
}

int main(void) {
  return first + second;
}
