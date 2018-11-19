//
//  vector.h
//  c_compiler
//
//  Created by David Allison on 10/26/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef vector_h
#define vector_h

#include <stdbool.h>
#include <stddef.h>

// Vector of pointers.  This is a dynamic array of pointers.  The array will
// be expanded as space is exhausted.  The Vector does not own the pointers
// and is not responsible for freeing up memory occupied by them.
//
// Since a pointer is the largest scalar type we can use the space
// occupied by the pointer to store any other scalar type, with
// appropriate use of casts.  For example, we can easily store 32 bit
// integers directly in the vector memory without mallocing 4 bytes
// and pointing to it.

typedef struct {
  void** value;     // Memory holding the pointers.
  size_t length;    // Number of pointers in memory.
  size_t capacity;  // Number of pointers we have space for.
} Vector;

// Initializes an empty vector.
void VectorInit(Vector* vec);

// Allocates a new vector using malloc and initializes it.
Vector* NewVector(void);

// Function pointer type to destruct contents of vector.
typedef void (*VectorElementDestructor)(void*);

// Destroys a vector by freeing up the pointer storage, not the memory
// used by the things being pointed to.
void VectorDestruct(Vector* vec);
void VectorDelete(Vector* vec);
void VectorDestructWithContents(Vector* vec,
                                VectorElementDestructor destructor);
void VectorDeleteWithContents(Vector* vec, VectorElementDestructor destructor);

void VectorClear(Vector* vec);

// Appends a pointer to the vector, reallocating the space
// as necessary.
void VectorAppend(Vector* vec, void* value);

// Sets an element in the vector to the value given.
void VectorSet(Vector* vec, size_t index, void* value);

// Gets a value from the vector at the given index.
void* VectorGet(Vector* vec, size_t index);

// Gets the last pointer in the vector.
void* VectorLast(Vector* vec);

void VectorCopy(Vector* dest, Vector* src);

void VectorPush(Vector* v, void* value);
void VectorPop(Vector* v);

bool VectorEqual(Vector* a, Vector* b);

void VectorInsertBefore(Vector* vec, size_t index, void* value);
void VectorInsertAfter(Vector* vec, size_t index, void* value);
void VectorDeleteElement(Vector* vec, size_t index);

#endif /* vector_h */
