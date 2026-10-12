// RUN: -std=c++17
// EXPECT_EXIT: 0

struct First {
  struct Internal {
    static constexpr int value = 11;
  };
};

struct Second {
  struct Internal {
    static constexpr int value = 31;
  };
};

const int* first_value = &First::Internal::value;
const int* second_value = &Second::Internal::value;

int main() {
  return *first_value + *second_value == 42 ? 0 : 1;
}
