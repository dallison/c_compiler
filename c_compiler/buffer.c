//
//  buffer.c
//  c_compiler
//
//  Created by David Allison on 11/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "buffer.h"
#include <stdlib.h>
#include <string.h>

void BufferInit(Buffer* buf) {
  buf->value = NULL;
  buf->length = 0;
  buf->capacity = 0;
}

Buffer* NewBuffer() {
  Buffer* buf = malloc(sizeof(Buffer));
  BufferInit(buf);
  return buf;
}

void BufferDestruct(Buffer* buf) {
  free(buf->value);
  buf->length = buf->capacity = 0;
}

void BufferDelete(Buffer* buf) {
  BufferDestruct(buf);
  free(buf);
}

void BufferClear(Buffer* buf) { buf->length = 0; }

void BufferAppend(Buffer* buf, char* value, size_t length) {
  size_t new_length = buf->length + length;

  // Make room for new contents by doubling the necessary memory.
  if (new_length > buf->capacity) {
    buf->capacity = new_length * 2;
    if (buf->value == NULL) {
      buf->value = malloc(buf->capacity);
    } else {
      buf->value = realloc(buf->value, buf->capacity);
    }
  }

  // Append value to end of memory.
  memcpy(&buf->value[buf->length], value, length);
  buf->length += length;
}

void BufferAppendByte(Buffer* buf, char byte) {
  size_t new_length = buf->length + 1;

  // Make room for new contents by doubling the necessary memory.
  if (new_length > buf->capacity) {
    buf->capacity = new_length * 2;
    if (buf->value == NULL) {
      buf->value = malloc(buf->capacity);
    } else {
      buf->value = realloc(buf->value, buf->capacity);
    }
  }

  // Append value to end of memory.
  buf->value[buf->length] = byte;
  buf->length++;
}

void BufferAppendHalfLE(Buffer* buf, uint16_t v) {
  BufferAppend(buf, (char*)&v, 2);
}

void BufferAppendWordLE(Buffer* buf, uint32_t v) {
  BufferAppend(buf, (char*)&v, 4);
}

void BufferAppendLongLE(Buffer* buf, uint64_t v) {
  BufferAppend(buf, (char*)&v, 8);
}

void BufferAddSpace(Buffer* buf, size_t length) {
  size_t new_length = buf->length + length;
  // Make room for new contents by doubling the necessary memory,
  if (new_length > buf->capacity) {
    buf->capacity = new_length * 2;
    if (buf->value == NULL) {
      buf->value = calloc(buf->capacity, 1);
    } else {
      buf->value = realloc(buf->value, buf->capacity);
      memset(&buf->value[buf->length], 0, new_length - buf->length);
    }
  }
  buf->length = new_length;
}

void BufferAlignLength(Buffer* buf, int alignment) {
  size_t new_length = (buf->length + (alignment - 1)) & ~(alignment - 1);
  BufferAddSpace(buf, new_length - buf->length);
}
