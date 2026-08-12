// RUN: -std=c++23 -O2
// EXPECT_EXIT: 0

struct value {
  bool (*predicate)(const void*);
  const void* data;
};

static bool reject(const void*) {
  return false;
}

static bool invalid(const value* argument) {
  return argument == nullptr || argument->predicate == nullptr ||
         !argument->predicate(argument->data);
}

int main() {
  value missing{nullptr, nullptr};
  value rejected{reject, nullptr};
  if (!invalid(nullptr)) return 1;
  if (!invalid(&missing)) return 2;
  return invalid(&rejected) ? 0 : 3;
}
