// `cond ? nullptr : "text"` decays the array arm; the result is a prvalue
// `const char*` (absl's Check_OPImpl returns `ok ? nullptr : message`).
#include <cstring>

template <typename T1, typename T2>
const char* CheckLE(const T1& a, const T2& b) {
  return a <= b ? nullptr : "check failed";
}

const char* Second(bool pick) { return pick ? "second" : nullptr; }

int main() {
  if (CheckLE(1, 2) != nullptr) {
    return 1;
  }
  const char* failed = CheckLE(3, 2);
  if (failed == nullptr || std::strcmp(failed, "check failed") != 0) {
    return 2;
  }
  if (Second(false) != nullptr || std::strcmp(Second(true), "second") != 0) {
    return 3;
  }
  char buffer[4] = {'a', 'b', 'c', '\0'};
  bool use = buffer[0] == 'a';
  char* p = use ? buffer : nullptr;
  return p == buffer && std::strlen(!use ? nullptr : buffer) == 3 ? 0 : 4;
}
