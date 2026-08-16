#ifndef DAVECC_STACKTRACE_RUNTIME_H
#define DAVECC_STACKTRACE_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uintptr_t start_offset;
  uintptr_t end_offset;
  uintptr_t name_offset;
} DaveStacktraceSymbol;

typedef struct {
  const unsigned char* start;
  const unsigned char* end;
} DaveStacktraceModule;

/*
 * Capture at most max_depth caller program counters after omitting skip
 * user-visible frames.  Passing frames == NULL obtains the required count.
 */
size_t __davecc_stacktrace_capture(uintptr_t* frames, size_t capacity,
                                   size_t skip, size_t max_depth);

/* Return static symbol text for pc, or NULL when no metadata is available. */
const char* __davecc_stacktrace_description(uintptr_t pc);

/* Return static source-file text for pc, or NULL when unavailable. */
const char* __davecc_stacktrace_source_file(uintptr_t pc);

/* Return the source line for pc, or zero when unavailable. */
uint_least32_t __davecc_stacktrace_source_line(uintptr_t pc);

#ifdef __cplusplus
}
#endif

#endif /* DAVECC_STACKTRACE_RUNTIME_H */
