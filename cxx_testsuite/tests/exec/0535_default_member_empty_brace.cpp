// RUN: -std=c++17
// EXPECT_EXIT: 0

template <class T>
struct EmptyBrace {
  T alloc_ = {};
};

template <class T>
struct AlignasMutable {
  alignas(T) mutable unsigned char slot_space_[sizeof(T)] = {};
};

struct Plain {
  int a = {};
  int b[2] = {};
};

int main() {
  EmptyBrace<int> brace;
  AlignasMutable<int> aligned;
  Plain plain;
  return brace.alloc_ + aligned.slot_space_[0] + plain.a + plain.b[0] +
         plain.b[1];
}
