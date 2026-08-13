// RUN: -std=c++26
// EXPECT: template-generated assertion

struct message {
  char text[29];

  constexpr const char* data() const { return text; }
  constexpr unsigned size() const { return 28; }
};

constexpr message generated{"template-generated assertion"};

template <bool condition>
void check() {
  static_assert(condition, generated);
}

int main() {
  check<false>();
  return 0;
}
