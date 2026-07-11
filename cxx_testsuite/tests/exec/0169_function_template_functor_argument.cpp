// RUN: -std=c++20
// EXPECT_EXIT: 0

struct Predicate {
  bool operator()(int value) const {
    return value == 7;
  }
};

template <class Pred>
bool apply_predicate(Pred pred, int value) {
  return pred.operator()(value);
}

int main() {
  return apply_predicate(Predicate(), 7) ? 0 : 1;
}
