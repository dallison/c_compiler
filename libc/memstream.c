#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// open_memstream is a FILE over a temporary file.  fflush and fclose copy the
// bytes into a malloc buffer and publish it through the caller's pointers.
// The libc is single-threaded, so one reentry flag covers the fseek that
// fflush would otherwise call back into.

struct MemStream {
  FILE* stream;
  char** bufp;
  size_t* sizep;
  char* owned;
  struct MemStream* next;
};

static struct MemStream* memstreams;
static int memstream_syncing;

static struct MemStream* FindMemStream(FILE* stream) {
  struct MemStream* entry = memstreams;
  while (entry != NULL) {
    if (entry->stream == stream) {
      return entry;
    }
    entry = entry->next;
  }
  return NULL;
}

int __davecc_memstream_sync(FILE* stream) {
  struct MemStream* entry;
  long position;
  long length;
  char* buffer;
  if (memstream_syncing) {
    return 0;
  }
  entry = FindMemStream(stream);
  if (entry == NULL) {
    return 0;
  }
  memstream_syncing = 1;
  position = ftell(stream);
  if (fseek(stream, 0, SEEK_END) != 0) {
    memstream_syncing = 0;
    return -1;
  }
  length = ftell(stream);
  if (length < 0) {
    memstream_syncing = 0;
    return -1;
  }
  buffer = malloc((size_t)length + 1);
  if (buffer == NULL) {
    memstream_syncing = 0;
    return -1;
  }
  if (length > 0) {
    if (fseek(stream, 0, SEEK_SET) != 0 ||
        fread(buffer, 1, (size_t)length, stream) != (size_t)length) {
      free(buffer);
      memstream_syncing = 0;
      return -1;
    }
  }
  buffer[length] = '\0';
  free(entry->owned);
  entry->owned = buffer;
  *entry->bufp = buffer;
  *entry->sizep = (size_t)length;
  if (position >= 0) {
    fseek(stream, position, SEEK_SET);
  }
  memstream_syncing = 0;
  return 0;
}

void __davecc_memstream_detach(FILE* stream) {
  struct MemStream** link = &memstreams;
  while (*link != NULL) {
    if ((*link)->stream == stream) {
      struct MemStream* entry = *link;
      *link = entry->next;
      free(entry);
      return;
    }
    link = &(*link)->next;
  }
}

FILE* open_memstream(char** bufp, size_t* sizep) {
  FILE* stream;
  struct MemStream* entry;
  if (bufp == NULL || sizep == NULL) {
    errno = EINVAL;
    return NULL;
  }
  stream = tmpfile();
  if (stream == NULL) {
    return NULL;
  }
  entry = malloc(sizeof(*entry));
  if (entry == NULL) {
    fclose(stream);
    return NULL;
  }
  entry->stream = stream;
  entry->bufp = bufp;
  entry->sizep = sizep;
  entry->owned = NULL;
  entry->next = memstreams;
  memstreams = entry;
  return stream;
}
