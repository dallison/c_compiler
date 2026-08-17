// RUN: -std=c++26
// EXPECT: comparison between two arrays is not allowed in C++26

int left_array[2];
int right_array[2];

bool compare_arrays() {
  return left_array == right_array;
}

bool order_arrays() {
  return left_array < right_array;
}
