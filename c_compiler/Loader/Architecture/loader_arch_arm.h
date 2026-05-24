//
//  loader_arch_arm.h
//  common_utils
//
//  Created by David Allison on 3/8/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef loader_arch_arm_h
#define loader_arch_arm_h

#include "loader_arch.h"

void ARMLoaderArchitectureInit(LoaderArchitecture* arch);
LoaderArchitecture* NewARMLoaderArchitecture(void);

#endif /* loader_arch_arm_h */
