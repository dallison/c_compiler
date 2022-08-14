//
//  arm_reg_alloc.c
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "arm_reg_alloc.h"

void ARMRegisterAllocatorInit(ARMRegisterAllocator* alloc,
                             struct ARMGenerator* ARM) {
  
}

ARMRegisterAllocator* NewARMRegisterAllocator(struct ARMGenerator* pcode) {
  return NULL;
}

const char* ARMRegisterNameFromNum(int num, ARMRegisterType type, char* buf,
                                   size_t len) {
  return NULL;
}

void ARMRegisterAllocatorDestruct(ARMRegisterAllocator* alloc) {
  
}

void ARMRegisterAllocatorDelete(ARMRegisterAllocator* alloc) {
  
}

void ARMAllocateRegisters(ARMRegisterAllocator* emitter) {
  
}

const char* ARMRegisterName(ARMRegister* reg, char* buf, size_t len) {
  return NULL;
}

