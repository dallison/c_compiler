//
//  loader_arch_aarch64.h
//  c_compiler
//
//  Created by David Allison on 1/25/23.
//  Copyright © 2023 David Allison. All rights reserved.
//

#ifndef loader_arch_aarch64_h
#define loader_arch_aarch64_h

#include "loader_arch.h"

void AARCH64LoaderArchitectureInit(LoaderArchitecture* arch);
LoaderArchitecture* NewAARCH64LoaderArchitecture(void);

#endif /* loader_arch_aarch64_h */
