#ifndef __davecc_const_generic_h
#define __davecc_const_generic_h

#define __DAVECC_CONST_GENERIC(pointer, const_type, call) \
  _Generic(0 ? (pointer) : (void*)1,                      \
      const void*: (const_type)(call),                    \
      void*: (call))

#endif
