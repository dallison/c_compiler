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
} DaveLSDAAction;

typedef struct CXXTypeInfo CXXTypeInfo;

int DaveLSDAFindAction(const uint8_t* lsda, uintptr_t func_start, uintptr_t pc,
                       uintptr_t cs, uintptr_t ce,
                       const CXXTypeInfo* thrown, DaveLSDAAction* out);

#endif /* DAVECC_LSDA_H */
