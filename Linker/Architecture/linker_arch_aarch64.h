//
//  linker_arch_aarch64.h
//  c_compiler
//
//  Created by David Allison on 1/25/23.
//  Copyright © 2023 David Allison. All rights reserved.
//

#ifndef linker_arch_aarch64_h
#define linker_arch_aarch64_h

#include <stdlib.h>
#include <string.h>

#include "linker_arch.h"
#include "aarch64_machine.h"

LinkerArchitecture* NewAARCH64LinkerArchitecture(void);

#endif /* linker_arch_aarch64_h */
