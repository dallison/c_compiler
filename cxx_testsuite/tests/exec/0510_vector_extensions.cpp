// RUN: -std=c++20
// EXPECT_EXIT: 0

typedef int int4 __attribute__((vector_size(16)));
typedef unsigned char byte16 __attribute__((ext_vector_type(16)));
using using_int4 = int __attribute__((vector_size(16)));

struct VectorAliases {
  using int4 = int __attribute__((vector_size(16)));
};

#if !__has_attribute(vector_size)
#error vector_size must be advertised
#endif
#if !__has_attribute(ext_vector_type)
#error ext_vector_type must be advertised
#endif

static_assert(sizeof(int4) == 16);
static_assert(alignof(int4) == 16);
static_assert(sizeof(byte16) == 16);
static_assert(sizeof(using_int4) == 16);
static_assert(sizeof(VectorAliases::int4) == 16);

static int4 add(int4 left, int4 right) {
  return left + right;
}

static int4 add_ten(int4 a0, int4 a1, int4 a2, int4 a3, int4 a4,
                    int4 a5, int4 a6, int4 a7, int4 a8, int4 a9) {
  return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9;
}

int main() {
  int4 left = {7, 11, 13, 17};
  int4 right = {1, 2, 3, 4};
  int4 sum = add(left, right);
  int4 mask = left > right;
  if (sum[0] != 8 || sum[1] != 13 || sum[2] != 16 || sum[3] != 21) {
    return 1;
  }
  if (mask[0] != -1 || mask[1] != -1 ||
      mask[2] != -1 || mask[3] != -1) {
    return 2;
  }
  int4 bits = (left ^ right) << right;
  if (bits[0] != 12 || bits[1] != 36 ||
      bits[2] != 112 || bits[3] != 336) {
    return 3;
  }
  int4 ten = add_ten(right, right, right, right, right,
                     right, right, right, right, right);
  if (ten[0] != 10 || ten[1] != 20 || ten[2] != 30 || ten[3] != 40) {
    return 4;
  }
  VectorAliases::int4 nested_left = {1, 2, 3, 4};
  VectorAliases::int4 nested_right = {4, 3, 2, 1};
  VectorAliases::int4 nested_sum = nested_left + nested_right;
  if (nested_sum[0] != 5 || nested_sum[3] != 5) {
    return 5;
  }
  return 0;
}
