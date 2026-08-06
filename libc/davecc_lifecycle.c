//
//  davecc_lifecycle.c
//  libc
//
//  Idempotent guest program lifecycle: preinit/init arrays forward, fini array
//  reverse, with supported TLS main fini and __cxa_finalize ordered before
//  fini teardown.
//

#include <davecc_lifecycle.h>
#include <stddef.h>
#include <stdlib.h>

typedef void (*DaveCCInitFiniFn)(void);

extern DaveCCInitFiniFn __preinit_array_start[];
extern DaveCCInitFiniFn __preinit_array_end[];
extern DaveCCInitFiniFn __init_array_start[];
extern DaveCCInitFiniFn __init_array_end[];
extern DaveCCInitFiniFn __fini_array_start[];
extern DaveCCInitFiniFn __fini_array_end[];

#if !defined(__6502__)
void __davecc_tls_thread_fini(void) __attribute__((weak));
#endif
void __cxa_finalize(void* dso);

static unsigned char __davecc_preinit_done;
static unsigned char __davecc_init_done;
static unsigned char __davecc_fini_done;
DaveCCInitFiniFn __davecc_stdio_fini_hook;

static DaveCCInitFiniFn* ArrayStart(DaveCCInitFiniFn* start) {
  return start;
}

static DaveCCInitFiniFn* ArrayEnd(DaveCCInitFiniFn* end) {
  return end;
}

static void WalkInitArrayForward(DaveCCInitFiniFn* start,
                                 DaveCCInitFiniFn* end) {
  DaveCCInitFiniFn* lo = ArrayStart(start);
  DaveCCInitFiniFn* hi = ArrayEnd(end);
  if (lo == NULL || hi == NULL || lo >= hi) {
    return;
  }
  for (DaveCCInitFiniFn* entry = lo; entry < hi; entry++) {
    DaveCCInitFiniFn fn = *entry;
    if (fn != NULL) {
      fn();
    }
  }
}

static void WalkFiniArrayReverse(DaveCCInitFiniFn* start,
                                 DaveCCInitFiniFn* end) {
  DaveCCInitFiniFn* lo = ArrayStart(start);
  DaveCCInitFiniFn* hi = ArrayEnd(end);
  if (lo == NULL || hi == NULL || lo >= hi) {
    return;
  }
  for (DaveCCInitFiniFn* entry = hi; entry > lo;) {
    entry--;
    DaveCCInitFiniFn fn = *entry;
    if (fn != NULL) {
      fn();
    }
  }
}

void __davecc_run_preinit(void) {
  if (__davecc_preinit_done != 0) {
    return;
  }
  __davecc_preinit_done = 1;
  WalkInitArrayForward(__preinit_array_start, __preinit_array_end);
}

void __davecc_run_init(void) {
  if (__davecc_init_done != 0) {
    return;
  }
  __davecc_init_done = 1;
  WalkInitArrayForward(__init_array_start, __init_array_end);
}

void __davecc_program_init(void) {
  __davecc_run_preinit();
  __davecc_run_init();
}

void __davecc_run_fini(void) {
  if (__davecc_fini_done != 0) {
    return;
  }
  __davecc_fini_done = 1;
#if !defined(__6502__)
  if (__davecc_tls_thread_fini != NULL) {
    __davecc_tls_thread_fini();
  }
#endif
  __cxa_finalize(NULL);
  WalkFiniArrayReverse(__fini_array_start, __fini_array_end);
  if (__davecc_stdio_fini_hook != NULL) {
    __davecc_stdio_fini_hook();
  }
}
