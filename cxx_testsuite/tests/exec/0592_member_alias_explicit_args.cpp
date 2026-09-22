// RUN: -std=c++17
// EXPECT_EXIT: 0

template <typename A, typename It>
class Adapter {
 public:
  explicit Adapter(It it) : it_(it) {}
  int get() const { return *it_; }

 private:
  It it_;
};

template <typename T>
struct Holder {
  template <typename TheA, typename Iterator>
  using AdapterT = Adapter<TheA, Iterator>;

  T value;
  explicit Holder(T v) : value(v) {}

  int wrap() {
    AdapterT<T, T*> adapter(&value);
    return adapter.get();
  }
};

int main() {
  Holder<int> h(6);
  return h.wrap() - 6;
}
