export module contracts;

export template <class T>
T module_checked(const T value)
    pre [[maybe_unused]] (value >= 0)
    post (result [[maybe_unused]]: result == value + 2) {
  contract_assert [[maybe_unused]] (value < 100);
  return value + 2;
}
