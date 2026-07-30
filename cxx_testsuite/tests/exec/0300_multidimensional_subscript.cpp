// RUN: -std=c++23
// EXPECT_EXIT: 0

struct matrix {
  int* data;
  int columns;

  constexpr int& operator[](int row, int column) const {
    return data[row * columns + column];
  }
};

struct tensor {
  int* data;

  template <class... Indices>
  int& operator[](Indices... indices) const {
    int offset = 0;
    ((offset = offset * 2 + indices), ...);
    return data[offset];
  }
};

template <class View, class... Indices>
decltype(auto) element(View& view, Indices... indices) {
  return view[indices...];
}

int main() {
  int matrix_data[] = {1, 2, 3, 4, 5, 6};
  matrix view{matrix_data, 3};
  view[1, 2] = 9;
  if (matrix_data[5] != 9 || element(view, 0, 1) != 2) {
    return 1;
  }

  int tensor_data[] = {0, 1, 2, 3, 4, 5, 6, 7};
  tensor cube{tensor_data};
  element(cube, 1, 0, 1) = 11;
  if (tensor_data[5] != 11 || cube[0, 1, 1] != 3) {
    return 2;
  }

  int indices[] = {0, 2};
  return matrix_data[indices[(0, 1)]] == 3 ? 0 : 3;
}
