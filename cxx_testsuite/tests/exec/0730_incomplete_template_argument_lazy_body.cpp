// RUN: -std=c++17
// EXPECT_EXIT: 0

class Value;

template <typename T>
struct HasMarker {
  template <typename U>
  static char Probe(const typename U::Marker*);

  template <typename U>
  static double Probe(...);

  static constexpr bool value =
      sizeof(Probe<T>(static_cast<const T*>(nullptr))) == sizeof(char);
};

template <typename T>
struct Container {
  bool UnusedProbe() const { return HasMarker<T>::value; }
};

// Forming Container<Value> must not instantiate UnusedProbe while Value is
// still incomplete and permanently cache a failed HasMarker<Value> probe.
Container<Value>& GetValues();

class Value {
 public:
  using Marker = void;
};

static_assert(HasMarker<Value>::value);

int main() { return HasMarker<Value>::value ? 0 : 1; }
