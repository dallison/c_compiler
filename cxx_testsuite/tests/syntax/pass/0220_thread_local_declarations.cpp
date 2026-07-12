// RUN: -std=c++11

thread_local int ns_tls = 1;
thread_local static int ns_tls_internal = 2;
extern thread_local int ns_tls_extern;
thread_local int ns_tls_extern = 3;

namespace tls_ns {
thread_local int value = 4;
}

struct ThreadLocalMember {
  static thread_local int declared;
  static thread_local int defined;
};
thread_local int ThreadLocalMember::declared;
thread_local int ThreadLocalMember::defined = 5;

template <typename T>
struct ThreadLocalTemplate {
  static thread_local T value;
};
template <>
thread_local int ThreadLocalTemplate<int>::value = 6;

int block_tls(void) {
  thread_local int block = 7;
  thread_local static int block_static = 8;
  extern thread_local int ns_tls;
  return block + block_static + ns_tls;
}

__thread int gnu_tls = 10;
int gnu_block(void) {
  __thread static int block_gnu = 11;
  return block_gnu;
}

int use_thread_local(void) {
  return ns_tls + ns_tls_internal + ns_tls_extern + tls_ns::value +
         ThreadLocalMember::defined + ThreadLocalTemplate<int>::value +
         block_tls() + gnu_tls + gnu_block();
}
