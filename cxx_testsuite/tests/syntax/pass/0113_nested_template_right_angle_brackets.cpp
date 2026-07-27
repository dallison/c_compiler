// RUN: -std=c++20
// C++11 CWG N1757: consecutive closing angle brackets in nested template-ids
// must be split (`vector<vector<int>>`) rather than lexed as a `>>` operator.
template <class T>
struct Box {
  T value;
};

template <class T, int N>
struct Arr {
  T data[N];
};

template <int N>
struct Val {
  int v;
};

// Two- and three-deep nesting closed by >> and >>>.
Box<Box<int>> two_deep;
Box<Box<Box<int>>> three_deep;
Box<Box<Box<Box<int>>>> four_deep;

// `>>=` must split into two template closers followed by `=`. In particular,
// a deleted declaration does not require whitespace after its return type.
auto deleted_factory() -> Box<Box<int>>=delete;

// Mixed type / non-type arguments.
Box<Arr<int, 2>> mixed;

// Default template argument that itself closes with >>.
template <class T = Box<Box<int>>>
struct Def {
  T t;
};
Def<> defaulted;

// Non-type argument whose value is a parenthesized shift: the `>>` here is a
// real right-shift operator and must not be treated as a template close.
Val<(6 >> 1)> paren_shift;

// A genuine relational >= in a non-type default is not a template closer.
template <int N = 1 >= 0>
struct GreaterEqualDefault {
  static constexpr int value = N;
};
static_assert(GreaterEqualDefault<>::value == 1);

template <class T>
T identity(T value) {
  return value;
}

struct Picker {
  template <class T>
  T pick(T value) {
    return value;
  }

  template <int N>
  int pick_value() {
    return N;
  }
};

template <class PickerType>
Box<Box<int>> dependent_pick(PickerType& picker, Box<Box<int>> value) {
  return picker.template pick<Box<Box<int>>>(value);
}

struct Stream {
  Stream& operator>>(int& value) {
    value = 4;
    return *this;
  }
};

int main(void) {
  Box<Box<int>> local;
  local.value.value = 3;
  // static_cast whose target type closes with >>.
  auto* p = static_cast<Box<Box<int>>*>(&local);
  // Explicit template argument list on a call, closed by >>.
  Box<int> made = identity<Box<int>>(local.value);
  Picker picker;
  Box<Box<int>> picked = picker.pick<Box<Box<int>>>(local);
  Box<Box<int>> dependent = dependent_pick(picker, local);
  int compared = picker.pick_value<1 >= 0>();
  int shifted = picker.pick_value<(8 >> 1)>();
  Stream stream;
  int streamed = 0;
  stream >> streamed;
  return p->value.value - made.value - (paren_shift.v == 0 ? 0 : 0) +
         (two_deep.value.value = 0) + (three_deep.value.value.value = 0) +
         (four_deep.value.value.value.value = 0) +
         (mixed.value.data[0] = 0) + (defaulted.t.value.value = 0) +
         picked.value.value + dependent.value.value +
         streamed + compared + shifted - 15;
}
