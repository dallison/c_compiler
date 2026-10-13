// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Cache {
  union {
    mutable const int* pointer;
    int unused;
  };

  Cache() : pointer(nullptr) {}

  void Set(const int* value) const { pointer = value; }
};

int main() {
  const int value = 23;
  const Cache cache;
  cache.Set(&value);
  return cache.pointer != nullptr && *cache.pointer == 23 ? 0 : 1;
}
