//
//  vector.c
//  c_compiler
//
//  Created by David Allison on 10/26/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "vector.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// A lot of vectors only have one member so let's keep the
// initial capacity small so that we don't waste memory.
#define INIT_CAPACITY 1

void VectorInit(Vector* vec) {
  vec->value = NULL;
  vec->length = 0;
  vec->capacity = 0;
}

Vector* NewVector() {
  Vector* vec = malloc(sizeof(Vector));
  VectorInit(vec);
  return vec;
}

void VectorDestruct(Vector* vec) {
  free(vec->value);
  vec->length = vec->capacity = 0;
  vec->value = NULL;
}

void VectorDelete(Vector* vec) {
  VectorDestruct(vec);
  free(vec);
}

void VectorDestructWithContents(Vector* vec,
                                VectorElementDestructor destructor) {
  for (size_t i = 0; i < vec->length; i++) {
    if (destructor != NULL) {
      (*destructor)(vec->value[i]);
    }
    free(vec->value[i]);
  }
  VectorDestruct(vec);
}

void VectorDeleteWithContents(Vector* vec, VectorElementDestructor destructor) {
  VectorDestructWithContents(vec, destructor);
  free(vec);
}

void VectorClear(Vector* vec) { vec->length = 0; }

static void MakeSpace(Vector* vec) {
  // If the vector is initially empty, allocate it with default capacity.
  if (vec->value == NULL) {
    vec->capacity = INIT_CAPACITY;
    vec->value = malloc(sizeof(void*) * vec->capacity);
    memset(vec->value, 0, sizeof(void*) * vec->capacity);
  }

  // Make room for new contents.
  if (vec->length + 1 > vec->capacity) {
    size_t old_capacity = vec->capacity;
    vec->capacity *= 2;
    vec->value = realloc(vec->value, sizeof(void*) * vec->capacity);
    memset(vec->value + old_capacity, 0,
           (vec->capacity - old_capacity) * sizeof(void*));
  }
}

void VectorAppend(Vector* vec, void* value) {
  MakeSpace(vec);

  // Append value to end of memory.
  vec->value[vec->length] = value;
  vec->length++;
}

void VectorSet(Vector* vec, size_t index, void* value) {
  vec->value[index] = value;
}

void* VectorGet(Vector* vec, size_t index) { return vec->value[index]; }

void* VectorLast(Vector* vec) { return vec->value[vec->length - 1]; }

void VectorCopy(Vector* dest, Vector* src) {
  VectorInit(dest);
  for (size_t i = 0; i < src->length; i++) {
    VectorAppend(dest, src->value[i]);
  }
}

void VectorPush(Vector* v, void* value) { VectorAppend(v, value); }

void VectorPop(Vector* v) {
  if (v->length > 0) {
    v->length--;
  }
}

bool VectorEqual(Vector* a, Vector* b) {
  if (a->length != b->length) {
    return false;
  }
  for (size_t i = 0; i < a->length; i++) {
    if (a->value[i] != b->value[i]) {
      return false;
    }
  }
  return true;
}

void VectorInsertBefore(Vector* vec, size_t index, void* value) {
  assert(index < vec->length);
  MakeSpace(vec);
  size_t elements_to_move = vec->length - index;
  memmove(vec->value + index + 1, vec->value + index,
          sizeof(void*) * elements_to_move);
  vec->value[index] = value;
  vec->length++;
}

void VectorInsertAfter(Vector* vec, size_t index, void* value) {
  assert(index < vec->length);
  if (index == vec->length - 1) {
    VectorAppend(vec, value);
    return;
  }

  MakeSpace(vec);
  size_t elements_to_move = vec->length - index - 1;

  memmove(vec->value + index + 2, vec->value + index + 1,
          sizeof(void*) * elements_to_move);
  vec->value[index + 1] = value;
  vec->length++;
}

void VectorDeleteElement(Vector* vec, size_t index) {
  assert(index < vec->length);
  size_t elements_to_move = vec->length - index - 1;
  memmove(vec->value + index, vec->value + index + 1,
          sizeof(void*) * elements_to_move);
  vec->length--;
}
