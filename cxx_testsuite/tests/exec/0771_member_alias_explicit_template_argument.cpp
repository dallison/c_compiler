class Copier {
  template <class Handler>
  using Value = typename Handler::Type;

  template <class Handler>
  static Value<Handler>* copy(const Value<Handler>* value) {
    return new Value<Handler>(*value);
  }

 public:
  template <class Handler>
  Value<Handler>* release(Value<Handler>* result) {
    return copy<Handler>(result);
  }
};

struct IntHandler {
  using Type = int;
};

int main() {
  Copier copier;
  int value = 3;
  int* copy = copier.release<IntHandler>(&value);
  int result = *copy;
  delete copy;
  return result != 3;
}
