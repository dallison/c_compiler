// RUN: -std=c++23

template <class T>
struct holder {
  struct value {
    value() {}

    template <class It>
    value(It first, It last) : count(last - first) {}

    int count = 0;
  };

  template <class Stream>
  friend void restore(Stream&, holder& output) {
    T values[2] = {};
    output.state = value(values, values + 2);
  }

  value state;
};

template <class Engine>
int direct_initialize() {
  Engine engine(17);
  return engine.value;
}

struct engine {
  explicit engine(int input) : value(input) {}
  int value;
};

void instantiate() {
  int stream = 0;
  holder<double> value;
  restore(stream, value);
  (void)direct_initialize<engine>();
}
