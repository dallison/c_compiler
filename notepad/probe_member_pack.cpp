template <class From, class To>
inline constexpr bool accepts = true;

template <class T, int... Values>
struct holder {
  static constexpr unsigned count() {
    return sizeof...(Values);
  }

  template <class... Args>
    requires(sizeof...(Args) == count() &&
             (accepts<Args, T> && ...))
  holder(Args... args) {}
};

int main() {
  holder<int, 1, 2> value(3, 4);
  return 0;
}
