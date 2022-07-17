//
//  _malloc.h
//  c_compiler
//
//  Created by David Allison on 10/15/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#ifndef _malloc_h
#define _malloc_h

#include <stddef.h>

// A free block starts with the FreeBlockHeader
// An allocated block starts with a length (size_t bytes long)
// that contains the allocated length.  The actual length is the length
// plus sizeof(size_t).

typedef struct FreeBlockHeader {
  size_t length;        // Length including header.
  struct FreeBlockHeader* next;
} FreeBlockHeader;

extern FreeBlockHeader* __free_list;

#endif /* _malloc_h */
