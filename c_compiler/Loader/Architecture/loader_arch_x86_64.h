//
//  loader_arch_x86_64.h
//  c_compiler
//

#ifndef loader_arch_x86_64_h
#define loader_arch_x86_64_h

#include "loader_arch.h"

void X86_64LoaderArchitectureInit(LoaderArchitecture* arch);
LoaderArchitecture* NewX86_64LoaderArchitecture(void);

#endif /* loader_arch_x86_64_h */
