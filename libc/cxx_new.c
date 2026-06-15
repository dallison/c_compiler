//
//  cxx_new.c
//  c_compiler
//
//  Minimal C++ allocation runtime hooks.
//

#include <stddef.h>
#include <stdlib.h>

void* __davecc_operator_new(size_t size) asm("_Znwy");
void* __davecc_operator_new_array(size_t size) asm("_Znay");
void* __davecc_operator_new32(unsigned int size) asm("_Znwj");
void* __davecc_operator_new_array32(unsigned int size) asm("_Znaj");
void __davecc_operator_delete(void* ptr) asm("_ZdlPv");
void __davecc_operator_delete_array(void* ptr) asm("_ZdaPv");

static void* CXXAllocate(size_t size) {
  return malloc(size == 0 ? 1 : size);
}

void* __davecc_operator_new(size_t size) {
  return CXXAllocate(size);
}

void* __davecc_operator_new_array(size_t size) {
  return CXXAllocate(size);
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
