// Full C++ runtimes define this in exception.cc and route it through
// std::terminate(). WebAssembly does not build the C++ exception runtime, but
// compiler-generated cleanup guards still need a non-returning fallback.
#if defined(__wasm32__)
#include <stdlib.h>

void __davecc_terminate(void) {
  abort();
}
#endif
