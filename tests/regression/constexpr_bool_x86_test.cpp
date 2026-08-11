#include <limits>

int main() {
  if (!std::numeric_limits<int>::is_specialized) return 10;
  if (!std::numeric_limits<int>::is_signed) return 11;
  return 0;
}
