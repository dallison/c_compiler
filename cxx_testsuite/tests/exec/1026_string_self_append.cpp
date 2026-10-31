// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <new>
#include <string>

template <class T>
struct poison_allocator {
  using value_type = T;

  poison_allocator() {}
  template <class U>
  poison_allocator(const poison_allocator<U>&) {}

  T* allocate(size_t count) {
    return static_cast<T*>(::operator new(count * sizeof(T)));
  }
  void deallocate(T* pointer, size_t count) {
    for (size_t index = 0; index < count; ++index) {
      pointer[index] = static_cast<T>('Z');
    }
    ::operator delete(pointer);
  }
  bool operator==(const poison_allocator&) const { return true; }
  bool operator!=(const poison_allocator&) const { return false; }
};

int main() {
  std::string hello = "hello";
  hello.assign(hello);
  if (hello != "hello") return 1;
  hello.assign(hello.data(), hello.size());
  if (hello != "hello") return 2;
  hello.assign(hello.data() + 1, 3);
  if (hello != "ell") return 3;

  std::string short_value = "hi";
  short_value.append(short_value);
  if (short_value != "hihi") return 4;

  using poisoned =
      std::basic_string<char, std::char_traits<char>, poison_allocator<char>>;
  poisoned grown(16, 'a');
  grown.append(grown);
  if (grown.size() != 32) return 5;
  for (poisoned::size_type index = 0; index < grown.size(); ++index) {
    if (grown[index] != 'a') return 6;
  }
  grown.append(grown.data(), 10);
  if (grown.size() != 42) return 7;
  for (poisoned::size_type index = 0; index < grown.size(); ++index) {
    if (grown[index] != 'a') return 8;
  }

  std::string plain(16, 'b');
  plain += plain;
  if (plain.size() != 32 || plain[0] != 'b' || plain[31] != 'b') return 9;
  plain.append(plain, 1, 2);
  if (plain.size() != 34 || plain[32] != 'b' || plain[33] != 'b') return 10;
  return 0;
}
