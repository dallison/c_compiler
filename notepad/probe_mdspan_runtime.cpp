#include <mdspan>

int main() {
  std::extents<int, 2, std::dynamic_extent, 4, std::dynamic_extent> shape(3, 5);
  return shape.extent(1) == 3 && shape.extent(3) == 5 ? 0 : 1;
}
