// RUN: -std=c++20
namespace reopen {
inline namespace {}
namespace {}
int value = 7;
}  // namespace reopen

int main(void) {
  return reopen::value - 7;
}
