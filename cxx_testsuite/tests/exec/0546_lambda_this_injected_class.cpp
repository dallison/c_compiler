// RUN: -std=c++17
// EXPECT_EXIT: 0

template <class Sig>
class Impl {};

template <class R>
struct Core {
  int invoker_;
  bool HasValue() const { return invoker_ != 0; }
};

template <class R>
class Impl<R()> : public Core<R> {
 public:
  int ExtractInvoker() {
    return [this]() {
      const_cast<Impl*>(this)->invoker_ = 1;
      return this->HasValue() ? 1 : 0;
    }();
  }
};

int main() {
  Impl<void()> i;
  i.invoker_ = 0;
  return i.ExtractInvoker() == 1 ? 0 : 1;
}
