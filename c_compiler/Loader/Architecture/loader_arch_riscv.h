//
//  loader_arch_riscv.h
//  common_utils
//
//  Created by David Allison on 3/8/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef loader_arch_riscv_h
#define loader_arch_riscv_h

#include "loader_arch.h"

void RISCVLoaderArchitectureInit(LoaderArchitecture* arch);
LoaderArchitecture* NewRISCVLoaderArchitecture(void);

#endif /* loader_arch_riscv_h */
