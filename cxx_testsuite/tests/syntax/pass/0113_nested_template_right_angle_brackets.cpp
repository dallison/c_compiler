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

template <class T>
T identity(T value) {
  return value;
}

int main(void) {
  Box<Box<int>> local;
  local.value.value = 3;
  // static_cast whose target type closes with >>.
  auto* p = static_cast<Box<Box<int>>*>(&local);
  // Explicit template argument list on a call, closed by >>.
  Box<int> made = identity<Box<int>>(local.value);
  return p->value.value - made.value - (paren_shift.v == 0 ? 0 : 0) +
         (two_deep.value.value = 0) + (three_deep.value.value.value = 0) +
         (mixed.value.data[0] = 0) + (defaulted.t.value.value = 0) - 3;
}
