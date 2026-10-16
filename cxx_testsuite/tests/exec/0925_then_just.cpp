// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <exception>
#include <execution>

int add2(int value) { return value + 2; }

struct Rcvr {
  using receiver_concept = std::execution::receiver_t;
  int* out;
  void set_value(int value) { *out = value; }
  void set_error(std::exception_ptr) {}
  void set_stopped() {}
  std::execution::env<> get_env() const { return {}; }
};

int main() {
  int out = 0;
  Rcvr rcvr{&out};
  auto op = std::execution::connect(
      std::execution::then(std::execution::just(40), add2), rcvr);
  std::execution::start(op);
  if (out != 42) return 1;
  return 0;
}
