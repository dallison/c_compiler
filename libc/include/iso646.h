//
//  iso646.h
//  c_compiler
//
//  C95 alternative spellings for the operator tokens.
//

#ifndef iso646_h
#define iso646_h

// In C++ these are keywords, so the header exists but defines nothing.
#ifndef __cplusplus

#define and &&
#define and_eq &=
#define bitand &
#define bitor |
#define compl ~
#define not !
#define not_eq !=
#define or ||
#define or_eq |=
#define xor ^
#define xor_eq ^=

#endif /* !__cplusplus */
#endif /* iso646_h */
