//
//  cxx_tls.c
//  libc
//
//  Idempotent per-thread lifetime hooks.  The compiler emits the weakly
//  referenced implementation functions when dynamic thread_local objects are
//  present.
//

#include <stddef.h>
#include <stdlib.h>
#include <syscall.h>

void __davecc_tls_thread_init_impl(void);
void __davecc_tls_thread_fini_impl(void);

#if defined(__DAVECC_HAS_GUEST_THREADS__)
typedef void (*DaveCCTlsBlockDtorFn)(void*);

typedef struct DaveCCTlsBlockDtorEntry {
  DaveCCTlsBlockDtorFn fn;
  void* obj;
  struct DaveCCTlsBlockDtorEntry* next;
} DaveCCTlsBlockDtorEntry;

static __thread int __davecc_tls_lifetime_state;
static __thread DaveCCTlsBlockDtorEntry* __davecc_tls_block_dtors;

void __davecc_tls_register_block_dtor(void* fn, void* obj) {
  if (fn == NULL) {
    return;
  }
  DaveCCTlsBlockDtorEntry* entry = malloc(sizeof(*entry));
  if (entry == NULL) {
    abort();
  }
  entry->fn = (DaveCCTlsBlockDtorFn)fn;
  entry->obj = obj;
  entry->next = __davecc_tls_block_dtors;
  __davecc_tls_block_dtors = entry;
}

static void RunBlockDestructors(void) {
  while (__davecc_tls_block_dtors != NULL) {
    DaveCCTlsBlockDtorEntry* entry = __davecc_tls_block_dtors;
    __davecc_tls_block_dtors = entry->next;
    DaveCCTlsBlockDtorFn fn = entry->fn;
    void* obj = entry->obj;
    free(entry);
    fn(obj);
  }
}
#else
static int __davecc_tls_lifetime_state;

void __davecc_tls_register_block_dtor(void* fn, void* obj) {
  (void)fn;
  (void)obj;
}
#endif

void __davecc_tls_thread_init(void) {
  if (__davecc_tls_lifetime_state != 0) {
    return;
  }
  __davecc_tls_lifetime_state = 1;
  __davecc_tls_thread_init_impl();
}

void __davecc_tls_thread_fini(void) {
  if (__davecc_tls_lifetime_state != 1) {
    return;
  }
  __davecc_tls_lifetime_state = 2;
#if defined(__DAVECC_HAS_GUEST_THREADS__)
  RunBlockDestructors();
#endif
  __davecc_tls_thread_fini_impl();
}
