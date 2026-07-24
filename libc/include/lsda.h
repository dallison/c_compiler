#ifndef DAVECC_LSDA_H
#define DAVECC_LSDA_H

#include <stdint.h>

typedef struct {
  uintptr_t landing_pad;
  uintptr_t try_start;
  uintptr_t try_end;
  int is_catch;
  int is_cleanup;
  long type_offset;
  long selector;
} DaveLSDAAction;

struct type_info;

typedef struct {
  uintptr_t pc;
  uintptr_t scope_start;
  uintptr_t scope_end;
  const struct type_info* thrown;
  int search_phase;
  int handler_frame;
} DaveLSDAQuery;

int DaveLSDAFindAction(const uint8_t* lsda, uintptr_t func_start,
                       const DaveLSDAQuery* query, DaveLSDAAction* out);

#endif /* DAVECC_LSDA_H */
