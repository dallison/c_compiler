#include <vector>

int main() {
  int values[] = {2, 4, 6};
  int* first = values;
  int* last = values + 3;
  std::vector<int> copy{first, last};
  return copy.size() != 3 || copy[1] != 4;
}
