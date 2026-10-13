#include <iterator>

using Function = int (*)(int);

int identity(int value) { return value; }

int main() {
  static constexpr Function functions[] = {{}, &identity, &identity};
  return std::end(functions) - std::begin(functions) != 3;
}
