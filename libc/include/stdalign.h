//
//  stdalign.h
//  c_compiler
//
//  C11 alignment macros.
//

#ifndef stdalign_h
#define stdalign_h

// C23 makes alignas and alignof keywords, so the macros must not be defined
// there; the two __alignas_is_defined macros stay for source compatibility.
#ifndef __cplusplus
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
#define alignas _Alignas
#define alignof _Alignof
#endif
#endif

#define __alignas_is_defined 1
#define __alignof_is_defined 1

#endif /* stdalign_h */
