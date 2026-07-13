//
//  davecc_lifecycle.h
//  libc
//
//  Guest program lifecycle hooks shared across DaveCC targets.  Runtimes call
//  these before main and after normal main return or exit(); they walk linker-
//  provided init/fini array bounds and coordinate TLS and atexit teardown.
//

#ifndef davecc_lifecycle_h
#define davecc_lifecycle_h

#ifdef __DAVECC__

#ifdef __cplusplus
extern "C" {
#endif

void __davecc_run_preinit(void);
void __davecc_run_init(void);
void __davecc_program_init(void);
void __davecc_run_fini(void);

#ifdef __cplusplus
}
#endif

#endif /* __DAVECC__ */

#endif /* davecc_lifecycle_h */
