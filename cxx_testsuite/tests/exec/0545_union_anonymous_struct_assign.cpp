// RUN: -std=c++17
// EXPECT_EXIT: 0

union State {
  struct {
    void* target;
    unsigned long size;
  } remote;
  alignas(16) unsigned char storage[16];
};

int main() {
  State a = {};
  State b = {};
  a.remote.target = &a;
  a.remote.size = 4;
  b = a;
  return b.remote.size == 4 && b.remote.target == &a ? 0 : 1;
}
