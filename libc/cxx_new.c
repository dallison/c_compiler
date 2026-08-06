//
//  cxx_new.c
//  c_compiler
//
//  Minimal C++ allocation runtime hooks.
//

#include <stddef.h>
#include <stdlib.h>

void* __davecc_operator_new(unsigned long size) asm("_Znwm");
void* __davecc_operator_new_array(unsigned long size) asm("_Znam");
void* __davecc_operator_new32(unsigned int size) asm("_Znwj");
void* __davecc_operator_new_array32(unsigned int size) asm("_Znaj");
void __davecc_operator_delete(void* ptr) asm("_ZdlPv");
void __davecc_operator_delete_array(void* ptr) asm("_ZdaPv");

static void* CXXAllocate(size_t size) {
  return malloc(size == 0 ? 1 : size);
}

static void* CXXAllocateLong(unsigned long size) {
  if (size > (unsigned long)(size_t)-1) {
    return NULL;
  }
  return CXXAllocate((size_t)size);
}

void* __davecc_operator_new(unsigned long size) {
  return CXXAllocateLong(size);
}

void* __davecc_operator_new_array(unsigned long size) {
  return CXXAllocateLong(size);
}

void* __davecc_operator_new32(unsigned int size) {
  return CXXAllocate(size);
}

void* __davecc_operator_new_array32(unsigned int size) {
  return CXXAllocate(size);
}

void __davecc_operator_delete(void* ptr) {
  if (ptr != NULL) {
    free(ptr);
  }
}

void __davecc_operator_delete_array(void* ptr) {
  if (ptr != NULL) {
    free(ptr);
  }
}

// std::new_handler is `void (*)()`; std::set_new_handler / std::get_new_handler
// manage the single process-wide handler.  The asm names are the Itanium
// manglings DaveCC emits for these std-namespace functions.
typedef void (*__davecc_new_handler)(void);

static __davecc_new_handler __davecc_current_new_handler = NULL;

__davecc_new_handler __davecc_set_new_handler(__davecc_new_handler handler)
    asm("_ZN3std15set_new_handlerEPFvE");
__davecc_new_handler __davecc_get_new_handler(void)
    asm("_ZN3std15get_new_handlerEv");

__davecc_new_handler __davecc_set_new_handler(__davecc_new_handler handler) {
  __davecc_new_handler previous = __davecc_current_new_handler;
  __davecc_current_new_handler = handler;
  return previous;
}

__davecc_new_handler __davecc_get_new_handler(void) {
  return __davecc_current_new_handler;
}
