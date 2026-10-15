// A class template specialization instantiated while an argument is
// incomplete (a `Map<int, Value>` member before `Value` is defined, as in
// protobuf's struct.pb.h) clones its member bodies on demand from the
// primary template, with the class arguments ahead of the member template's
// own.
// `sizeof...(Args)` must still count the member's pack: unfolded, the
// `if constexpr` kept both branches and the specialization was never emitted.
#include <utility>

struct Value;

template <typename Key, typename T>
class Map {
 public:
  T* value = nullptr;
  int inserted = 0;

  template <typename K, typename... Args>
  int try_emplace(K&& k, Args&&... args) {
    if constexpr (sizeof...(Args) == 0) {
      return Insert(std::forward<K>(k));
    } else {
      *value = T(std::forward<Args>(args)...);
      return Insert(std::forward<K>(k)) + 100;
    }
  }

 private:
  template <typename K>
  int Insert(K&& k) {
    ++inserted;
    return static_cast<int>(k);
  }
};

struct Struct {
  Map<int, Value> fields_;
};

struct Value {
  int v = 0;
  Value() = default;
  explicit Value(int x) : v(x) {}
};

int main() {
  Struct s;
  Value storage;
  Map<int, Value>& m = s.fields_;
  m.value = &storage;
  if (m.try_emplace(3) != 3) {
    return 1;
  }
  if (m.try_emplace(4, 7) != 104 || storage.v != 7) {
    return 2;
  }
  return m.inserted == 2 ? 0 : 3;
}
