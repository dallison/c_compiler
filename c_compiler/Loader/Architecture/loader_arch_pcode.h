//
//  loader_arch_pcode.h
//  common_utils
//
//  Created by David Allison on 3/8/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef loader_arch_pcode_h
#define loader_arch_pcode_h

#include "loader_arch.h"

void PCodeLoaderArchitectureInit(LoaderArchitecture* arch);
LoaderArchitecture* NewPCodeLoaderArchitecture(void);

#endif /* loader_arch_pcode_h */
