// RUN: -std=c++17
// EXPECT_EXIT: 0

#define LITTLE 1

enum class endian {
  little,
  big,
#if LITTLE
  native = little
#elif defined(BIG)
  native = big
#else
#error "no endian"
#endif
};

int main() { return (int)endian::native == (int)endian::little ? 0 : 1; }
