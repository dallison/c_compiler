typedef int int4 __attribute__((vector_size(16)));
typedef unsigned short ushort8 __attribute__((vector_size(16)));
typedef signed char byte16 __attribute__((vector_size(16)));
typedef long long long2 __attribute__((vector_size(16)));
typedef float float4 __attribute__((vector_size(16)));
typedef double double2 __attribute__((vector_size(16)));
typedef int int2 __attribute__((vector_size(8)));

int main(void) {
  int4 left = {7, 11, 13, 17};
  int4 right = {1, 2, 3, 4};
  int4 sum = left + right;
  int4 diff = left - right;
  int4 prod = left * right;
  int4 bits = left | right;
  int4 flipped = left ^ right;
  int4 mask = left > right;
  if (sum[0] != 8 || sum[1] != 13 || sum[2] != 16 || sum[3] != 21) {
    return 1;
  }
  if (diff[0] != 6 || diff[3] != 13) {
    return 2;
  }
  if (prod[0] != 7 || prod[1] != 22 || prod[2] != 39 || prod[3] != 68) {
    return 3;
  }
  if (bits[0] != 7 || bits[1] != 11 || bits[2] != 15 || bits[3] != 21) {
    return 4;
  }
  if (flipped[0] != 6 || flipped[1] != 9 || flipped[2] != 14 || flipped[3] != 21) {
    return 16;
  }
  if (mask[0] != -1 || mask[1] != -1 || mask[2] != -1 || mask[3] != -1) {
    return 5;
  }

  ushort8 wide_left = {1, 2, 3, 4, 5, 6, 7, 8};
  ushort8 wide_right = {2, 2, 2, 2, 2, 2, 2, 2};
  ushort8 wide_prod = wide_left * wide_right;
  if (wide_prod[0] != 2 || wide_prod[7] != 16) {
    return 6;
  }

  byte16 bytes_left = {1, 2, 3, 4, 5, 6, 7, 8,
                       9, 10, 11, 12, 13, 14, 15, 16};
  byte16 bytes_right = {1, 1, 1, 1, 1, 1, 1, 1,
                        1, 1, 1, 1, 1, 1, 1, 1};
  byte16 bytes_sum = bytes_left + bytes_right;
  if (bytes_sum[0] != 2 || bytes_sum[15] != 17) {
    return 7;
  }

  long2 long_left = {8, 27};
  long2 long_right = {2, 3};
  long2 long_sum = long_left + long_right;
  long2 long_prod = long_left * long_right;
  if (long_sum[0] != 10 || long_sum[1] != 30) {
    return 8;
  }
  if (long_prod[0] != 16 || long_prod[1] != 81) {
    return 9;
  }

  float4 fleft = {1.f, 2.f, 3.f, 4.f};
  float4 fright = {2.f, 0.5f, 4.f, 0.25f};
  float4 fsum = fleft + fright;
  float4 fprod = fleft * fright;
  float4 fquot = fleft / fright;
  if (fsum[0] != 3.f || fsum[3] != 4.25f) {
    return 10;
  }
  if (fprod[0] != 2.f || fprod[1] != 1.f) {
    return 11;
  }
  if (fquot[0] != 0.5f || fquot[3] != 16.f) {
    return 12;
  }

  double2 dleft = {8.0, 27.0};
  double2 dright = {2.0, 3.0};
  double2 dquot = dleft / dright;
  if (dquot[0] != 4.0 || dquot[1] != 9.0) {
    return 13;
  }

  int2 short_left = {9, 11};
  int2 short_right = {2, 4};
  int2 short_sum = short_left + short_right;
  if (short_sum[0] != 11 || short_sum[1] != 15) {
    return 14;
  }

  int values[8];
  int i;
  for (i = 0; i < 8; i++) {
    values[i] = i + 1;
  }
  for (i = 0; i < 8; i++) {
    values[i] = values[i] + 10;
  }
  if (values[0] != 11 || values[7] != 18) {
    return 15;
  }
  return 0;
}
