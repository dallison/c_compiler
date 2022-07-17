//
//  linker_arch_6502.h
//  p_code_linker
//
//  Created by David Allison on 6/25/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef linker_arch_6502_h
#define linker_arch_6502_h

#include "linker_arch.h"
#include "6502_machine.h"

#define W65C02_CODE_START 0x400

LinkerArchitecture* New6502LinkerArchitecture(void);

#endif /* linker_arch_6502_h */
