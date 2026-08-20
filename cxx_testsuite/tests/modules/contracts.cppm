export module contracts;

export template <class T>
T module_checked(const T value)
    pre [[maybe_unused]] (value >= 0)
    post (result [[maybe_unused]]: result == value + 2) {
  contract_assert [[maybe_unused]] (value < 100);
  return value + 2;
}

export template <class T>
int module_virtual_checked(const T input) {
  struct interface {
    virtual int transform(const int value)
        pre (value >= 0)
        post (result: result == value + 1) {
      return value;
    }
  };

  struct implementation : interface {
    int transform(const int value) override
        pre (value < 100)
        post (result: result == value + 1) {
      return value + 1;
    }
  };

  implementation object;
  (void)object;
  return input + 1;
}
