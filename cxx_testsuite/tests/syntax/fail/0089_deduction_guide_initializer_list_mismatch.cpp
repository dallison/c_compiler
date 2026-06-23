// RUN: -std=c++20
namespace std {
template <class T>
struct initializer_list {
  const T* __begin;
  unsigned long __size;
};
}

template <typename T>
struct ListOnly {
  ListOnly(std::initializer_list<T> values) {
  }
};

int main(void) {
  ListOnly mixed{1, 'a'};
  return 0;
}
