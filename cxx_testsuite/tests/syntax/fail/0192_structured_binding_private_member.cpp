// RUN: -std=c++20
// EXPECT: Cannot decompose non-public member in structured binding
class Secret {
  int hidden;

 public:
  int visible;
};

int value(void) {
  Secret secret;
  auto [hidden, visible] = secret;
  return hidden + visible;
}
