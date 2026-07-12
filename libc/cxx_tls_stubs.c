//
//  cxx_tls_stubs.c
//  libc
//
//  Archive fallbacks used when a C++ translation unit does not emit dynamic
//  thread_local lifetime functions.
//

void __davecc_tls_thread_init_impl(void) __attribute__((weak));
void __davecc_tls_thread_fini_impl(void) __attribute__((weak));

void __davecc_tls_thread_init_impl(void) {}

void __davecc_tls_thread_fini_impl(void) {}
