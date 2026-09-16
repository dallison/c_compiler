#ifndef loader_arch_bpf_h
#define loader_arch_bpf_h

#include "loader_arch.h"

void BPFLoaderArchitectureInit(LoaderArchitecture* arch);
LoaderArchitecture* NewBPFLoaderArchitecture(void);

#endif
