#include <typeinfo>
#include <stdlib.h>

extern "C" void __davecc_raise_bad_cast(void) {
#ifdef __cpp_exceptions
  throw std::bad_cast();
#else
  ::abort();
#endif
}
