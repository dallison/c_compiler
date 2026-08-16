// RUN: -std=c++26

template <typename T>
struct primary {
  T value;
};

template [: ^^primary :] struct secondary {
  T value;
};

void use(secondary<int> s) {
  (void)s.value;
}
