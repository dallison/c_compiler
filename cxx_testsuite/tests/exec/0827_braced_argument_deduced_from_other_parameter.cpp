// A braced-init-list argument is a non-deduced context; its parameter's
// template arguments may come from another argument, and the candidate stays
// viable alongside overloads that fail to deduce.
#include <string>
#include <vector>

namespace internal {
template <class T>
class Flag {
 public:
  T value;
};
}  // namespace internal

template <class T>
using Flag = internal::Flag<T>;

template <class T>
void SetFlag(Flag<T>* flag, const T& v);
template <class T, class V>
void SetFlag(Flag<T>* flag, const V& v);

template <class T>
void SetFlag(Flag<T>* flag, const T& v) {
  flag->value = v;
}
template <class T, class V>
void SetFlag(Flag<T>* flag, const V& v) {
  T converted(v);
  flag->value = converted;
}

struct Registry {
  int count = 0;
  template <class T>
  int Set(Flag<T>* flag, const T& v) {
    flag->value = v;
    return ++count;
  }
  template <class T, class V>
  int Set(Flag<T>* flag, const V& v) {
    flag->value = T(v);
    return 100 + ++count;
  }
};

template <class T>
int Pick(T&&) {
  return 1;
}
int Pick(const std::vector<int>& v) { return 2 + (int)v.size(); }

template <class T = long>
int DefaultSize(const T&) {
  return (int)sizeof(T);
}

Flag<std::vector<std::string>> from_env;
Flag<bool> verbose;
Flag<std::string> name;

int main() {
  from_env.value.push_back("HOME");
  SetFlag(&from_env, {});
  if (!from_env.value.empty()) return 1;

  verbose.value = true;
  SetFlag(&verbose, {});
  if (verbose.value) return 2;

  SetFlag(&name, "abc");
  if (name.value != "abc") return 3;
  SetFlag(&name, {});
  if (!name.value.empty()) return 4;

  Registry registry;
  verbose.value = true;
  if (registry.Set(&verbose, {}) != 1 || verbose.value) return 5;
  if (registry.Set(&verbose, 1) != 102 || !verbose.value) return 6;

  if (Pick({1, 2, 3}) != 5) return 7;
  if (Pick(0) != 1) return 8;

  if (DefaultSize({}) != (int)sizeof(long)) return 9;
  return 0;
}
