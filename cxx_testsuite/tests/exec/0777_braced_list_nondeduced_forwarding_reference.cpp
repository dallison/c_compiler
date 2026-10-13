struct Pair {
  Pair(int, int) {}
};

int insert(Pair&&) { return 1; }

template <class T>
int insert(T&&) {
  return 2;
}

int main() { return insert({1, 2}) != 1; }
