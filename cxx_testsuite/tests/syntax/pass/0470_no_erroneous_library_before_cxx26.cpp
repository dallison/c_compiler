// RUN: -std=c++23

#include <version>

#ifdef __cpp_lib_observable_checkpoint
#error "observable checkpoint must not be advertised before C++26"
#endif

void cxx23_uninitialized_storage_is_accepted() {
  int scalar;
  int values[2];
  (void)&scalar;
  (void)&values;
}
