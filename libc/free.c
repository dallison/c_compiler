//
// The modern Free implementation lives with the TLSF metadata in malloc.c.
// Keep this compact address-ordered implementation for the 6502 target.
//

#if defined(__6502__)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "6502/_malloc.h"

#define Free free

void Free(void* pointer) {
  if (pointer == NULL) {
    return;
  }
  if (((size_t*)pointer)[-1] == 1) {
    pointer = (void*)(uintptr_t)((size_t*)pointer)[-2];
  }

  size_t payload_size = *((size_t*)pointer - 1);
  FreeBlockHeader* block =
      (FreeBlockHeader*)((char*)pointer - sizeof(size_t));
  block->length = payload_size + sizeof(size_t);

  FreeBlockHeader* previous = NULL;
  FreeBlockHeader* current = __free_list;
  while (current != NULL && current < block) {
    previous = current;
    current = current->next;
  }

  block->next = current;
  if (current != NULL &&
      (char*)block + block->length == (char*)current) {
    block->length += current->length;
    block->next = current->next;
  }

  if (previous != NULL &&
      (char*)previous + previous->length == (char*)block) {
    previous->length += block->length;
    previous->next = block->next;
  } else if (previous == NULL) {
    __free_list = block;
  } else {
    previous->next = block;
  }
}

#endif
