//
//  p_code_vm.c
//  c_compiler
//

#include "p_code_vm.h"
#include <stdlib.h>
#include <string.h>

#define DEST(inst) (((inst) >> 16) & 0xff)
#define SRC1(inst) (((inst) >> 8) & 0xff)
#define SRC2(inst) ((inst) & 0xff)

static PCodeVMStatus DefaultEscape(PCodeVM* vm, int32_t code, void* data) {
  (void)vm;
  (void)data;
  switch (code) {
    case 4:
      return kPCodeVMStatusHalted;
    case 0:
      return kPCodeVMStatusUndefinedInstruction;
    case 1:
      return kPCodeVMStatusDivisionByZero;
    default:
      return kPCodeVMStatusUndefinedEscape;
  }
}

void PCodeVMInit(PCodeVM* vm) {
  memset(vm, 0, sizeof(PCodeVM));
  vm->max_steps = -1;
  vm->status = kPCodeVMStatusRunning;
  vm->escape = DefaultEscape;
}

bool PCodeVMInitWithStack(PCodeVM* vm, size_t stack_size) {
  PCodeVMInit(vm);
  vm->stack = malloc(stack_size);
  if (vm->stack == NULL) {
    return false;
  }
  vm->stack_size = stack_size;
  vm->owns_stack = true;
  vm->iregs[PCODE_SP_REG] = (int64_t)(vm->stack + vm->stack_size);
  return true;
}

void PCodeVMDestruct(PCodeVM* vm) {
  if (vm->owns_stack) {
    free(vm->stack);
  }
  vm->stack = NULL;
  vm->stack_size = 0;
  vm->owns_stack = false;
}

void PCodeVMSetStack(PCodeVM* vm, void* stack, size_t stack_size) {
  if (vm->owns_stack) {
    free(vm->stack);
  }
  vm->stack = stack;
  vm->stack_size = stack_size;
  vm->owns_stack = false;
  vm->iregs[PCODE_SP_REG] = (int64_t)((char*)stack + stack_size);
}

void PCodeVMSetEntry(PCodeVM* vm, uint64_t entry_address) {
  vm->iregs[PCODE_PC_REG] = (int64_t)entry_address;
  vm->status = kPCodeVMStatusRunning;
}

void PCodeVMSetEscapeHandler(PCodeVM* vm, PCodeVMEscapeHandler handler,
                             void* data) {
  vm->escape = handler != NULL ? handler : DefaultEscape;
  vm->escape_data = data;
}

static PCodeVMStatus HandleEscape(PCodeVM* vm, int32_t code) {
  if (vm->escape == NULL) {
    return DefaultEscape(vm, code, NULL);
  }
  return vm->escape(vm, code, vm->escape_data);
}

static PCodeVMStatus DivisionByZero(PCodeVM* vm) {
  return HandleEscape(vm, 1);
}

static PCodeVMStatus UndefinedInstruction(PCodeVM* vm) {
  return HandleEscape(vm, 0);
}

PCodeVMStatus PCodeVMStep(PCodeVM* vm) {
  if (vm->status != kPCodeVMStatusRunning) {
    return vm->status;
  }
  if (vm->max_steps >= 0 && vm->steps >= vm->max_steps) {
    vm->status = kPCodeVMStatusStepLimit;
    return vm->status;
  }
  vm->steps++;

  int64_t* iregs = vm->iregs;
  float* fregs = vm->fregs;
  double* dregs = vm->dregs;
  int32_t* pc = (int32_t*)iregs[PCODE_PC_REG];
  uint32_t inst = *pc++;
  iregs[PCODE_PC_REG] += 4;

  bool is_32_bit = (inst & 0x80000000) == 0;
  if (is_32_bit) {
    switch ((inst >> 24) & 0x7f) {
      case PCODE_OP(add):
        iregs[DEST(inst)] = iregs[SRC1(inst)] + iregs[SRC2(inst)];
        break;
      case PCODE_OP(sub):
        iregs[DEST(inst)] = iregs[SRC1(inst)] - iregs[SRC2(inst)];
        break;
      case PCODE_OP(addf):
        fregs[DEST(inst)] = fregs[SRC1(inst)] + fregs[SRC2(inst)];
        break;
      case PCODE_OP(addd):
        dregs[DEST(inst)] = dregs[SRC1(inst)] + dregs[SRC2(inst)];
        break;
      case PCODE_OP(subf):
        fregs[DEST(inst)] = fregs[SRC1(inst)] - fregs[SRC2(inst)];
        break;
      case PCODE_OP(subd):
        dregs[DEST(inst)] = dregs[SRC1(inst)] - dregs[SRC2(inst)];
        break;
      case PCODE_OP(mul):
        iregs[DEST(inst)] = iregs[SRC1(inst)] * iregs[SRC2(inst)];
        break;
      case PCODE_OP(mulf):
        fregs[DEST(inst)] = fregs[SRC1(inst)] * fregs[SRC2(inst)];
        break;
      case PCODE_OP(muld):
        dregs[DEST(inst)] = dregs[SRC1(inst)] * dregs[SRC2(inst)];
        break;
      case PCODE_OP(div):
        if (iregs[SRC2(inst)] == 0) {
          vm->status = DivisionByZero(vm);
        } else {
          iregs[DEST(inst)] = iregs[SRC1(inst)] / iregs[SRC2(inst)];
        }
        break;
      case PCODE_OP(divu):
        if (iregs[SRC2(inst)] == 0) {
          vm->status = DivisionByZero(vm);
        } else {
          iregs[DEST(inst)] =
              (uint64_t)iregs[SRC1(inst)] / (uint64_t)iregs[SRC2(inst)];
        }
        break;
      case PCODE_OP(divf):
        if (fregs[SRC2(inst)] == 0) {
          vm->status = DivisionByZero(vm);
        } else {
          fregs[DEST(inst)] = fregs[SRC1(inst)] / fregs[SRC2(inst)];
        }
        break;
      case PCODE_OP(divd):
        if (dregs[SRC2(inst)] == 0) {
          vm->status = DivisionByZero(vm);
        } else {
          dregs[DEST(inst)] = dregs[SRC1(inst)] / dregs[SRC2(inst)];
        }
        break;
      case PCODE_OP(mod):
        if (iregs[SRC2(inst)] == 0) {
          vm->status = DivisionByZero(vm);
        } else {
          iregs[DEST(inst)] = iregs[SRC1(inst)] % iregs[SRC2(inst)];
        }
        break;
      case PCODE_OP(modu):
        if (iregs[SRC2(inst)] == 0) {
          vm->status = DivisionByZero(vm);
        } else {
          iregs[DEST(inst)] =
              (uint64_t)iregs[SRC1(inst)] % (uint64_t)iregs[SRC2(inst)];
        }
        break;
      case PCODE_OP(lsr):
        iregs[DEST(inst)] =
            (uint64_t)iregs[SRC1(inst)] >> iregs[SRC2(inst)];
        break;
      case PCODE_OP(asr):
        iregs[DEST(inst)] = iregs[SRC1(inst)] >> iregs[SRC2(inst)];
        break;
      case PCODE_OP(lsl):
        iregs[DEST(inst)] = iregs[SRC1(inst)] << iregs[SRC2(inst)];
        break;
      case PCODE_OP(or):
        iregs[DEST(inst)] = iregs[SRC1(inst)] | iregs[SRC2(inst)];
        break;
      case PCODE_OP(and):
        iregs[DEST(inst)] = iregs[SRC1(inst)] & iregs[SRC2(inst)];
        break;
      case PCODE_OP(xor):
        iregs[DEST(inst)] = iregs[SRC1(inst)] ^ iregs[SRC2(inst)];
        break;
      case PCODE_OP(not):
        iregs[DEST(inst)] = !iregs[SRC1(inst)];
        break;
      case PCODE_OP(inv):
        iregs[DEST(inst)] = ~iregs[SRC1(inst)];
        break;
      case PCODE_OP(neg):
        iregs[DEST(inst)] = -iregs[SRC1(inst)];
        break;
      case PCODE_OP(negf):
        fregs[DEST(inst)] = -fregs[SRC1(inst)];
        break;
      case PCODE_OP(negd):
        dregs[DEST(inst)] = -dregs[SRC1(inst)];
        break;
      case PCODE_OP(cmpeq):
        iregs[DEST(inst)] = iregs[SRC1(inst)] == iregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpne):
        iregs[DEST(inst)] = iregs[SRC1(inst)] != iregs[SRC2(inst)];
        break;
      case PCODE_OP(cmplt):
        iregs[DEST(inst)] = iregs[SRC1(inst)] < iregs[SRC2(inst)];
        break;
      case PCODE_OP(cmple):
        iregs[DEST(inst)] = iregs[SRC1(inst)] <= iregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpgt):
        iregs[DEST(inst)] = iregs[SRC1(inst)] > iregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpge):
        iregs[DEST(inst)] = iregs[SRC1(inst)] >= iregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpltu):
        iregs[DEST(inst)] =
            (uint64_t)iregs[SRC1(inst)] < (uint64_t)iregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpleu):
        iregs[DEST(inst)] =
            (uint64_t)iregs[SRC1(inst)] <= (uint64_t)iregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpgtu):
        iregs[DEST(inst)] =
            (uint64_t)iregs[SRC1(inst)] > (uint64_t)iregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpgeu):
        iregs[DEST(inst)] =
            (uint64_t)iregs[SRC1(inst)] >= (uint64_t)iregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpeqf):
        iregs[DEST(inst)] = fregs[SRC1(inst)] == fregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpnef):
        iregs[DEST(inst)] = fregs[SRC1(inst)] != fregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpltf):
        iregs[DEST(inst)] = fregs[SRC1(inst)] < fregs[SRC2(inst)];
        break;
      case PCODE_OP(cmplef):
        iregs[DEST(inst)] = fregs[SRC1(inst)] <= fregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpgtf):
        iregs[DEST(inst)] = fregs[SRC1(inst)] > fregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpgef):
        iregs[DEST(inst)] = fregs[SRC1(inst)] >= fregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpeqd):
        iregs[DEST(inst)] = dregs[SRC1(inst)] == dregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpned):
        iregs[DEST(inst)] = dregs[SRC1(inst)] != dregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpltd):
        iregs[DEST(inst)] = dregs[SRC1(inst)] < dregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpled):
        iregs[DEST(inst)] = dregs[SRC1(inst)] <= dregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpgtd):
        iregs[DEST(inst)] = dregs[SRC1(inst)] > dregs[SRC2(inst)];
        break;
      case PCODE_OP(cmpged):
        iregs[DEST(inst)] = dregs[SRC1(inst)] >= dregs[SRC2(inst)];
        break;
      case PCODE_OP(decsp):
        iregs[PCODE_SP_REG] -= inst & 0xffffff;
        break;
      case PCODE_OP(incsp):
        iregs[PCODE_SP_REG] += inst & 0xffffff;
        break;
      case PCODE_OP(push):
        iregs[PCODE_SP_REG] -= 4;
        *((int32_t*)iregs[PCODE_SP_REG]) = (int32_t)iregs[DEST(inst)];
        break;
      case PCODE_OP(pushf):
        iregs[PCODE_SP_REG] -= 4;
        *((float*)iregs[PCODE_SP_REG]) = fregs[DEST(inst)];
        break;
      case PCODE_OP(pushd):
        iregs[PCODE_SP_REG] -= 8;
        *((double*)iregs[PCODE_SP_REG]) = dregs[DEST(inst)];
        break;
      case PCODE_OP(pushx):
        iregs[PCODE_SP_REG] -= 8;
        *((uint64_t*)iregs[PCODE_SP_REG]) = iregs[DEST(inst)];
        break;
      case PCODE_OP(pop):
        iregs[DEST(inst)] = *((int32_t*)iregs[PCODE_SP_REG]);
        iregs[PCODE_SP_REG] += 4;
        break;
      case PCODE_OP(popf):
        fregs[DEST(inst)] = *((float*)iregs[PCODE_SP_REG]);
        iregs[PCODE_SP_REG] += 4;
        break;
      case PCODE_OP(popd):
        dregs[DEST(inst)] = *((double*)iregs[PCODE_SP_REG]);
        iregs[PCODE_SP_REG] += 8;
        break;
      case PCODE_OP(popx):
        iregs[DEST(inst)] = *((uint64_t*)iregs[PCODE_SP_REG]);
        iregs[PCODE_SP_REG] += 8;
        break;
      case PCODE_OP(mov):
        iregs[DEST(inst)] = iregs[SRC1(inst)];
        break;
      case PCODE_OP(movf):
        fregs[DEST(inst)] = fregs[SRC1(inst)];
        break;
      case PCODE_OP(movd):
        dregs[DEST(inst)] = dregs[SRC1(inst)];
        break;
      case PCODE_OP(ret):
        iregs[PCODE_PC_REG] = *((uint64_t*)iregs[PCODE_SP_REG]);
        iregs[PCODE_SP_REG] += 8;
        break;
      case PCODE_OP(cbra):
        if (iregs[DEST(inst)] != 0) {
          iregs[PCODE_PC_REG] = iregs[SRC1(inst)];
        }
        break;
      case PCODE_OP(i2f):
        fregs[DEST(inst)] = iregs[SRC1(inst)];
        break;
      case PCODE_OP(i2d):
        dregs[DEST(inst)] = iregs[SRC1(inst)];
        break;
      case PCODE_OP(ui2f):
        fregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)];
        break;
      case PCODE_OP(ui2d):
        dregs[DEST(inst)] = (uint64_t)iregs[SRC1(inst)];
        break;
      case PCODE_OP(f2d):
        dregs[DEST(inst)] = fregs[SRC1(inst)];
        break;
      case PCODE_OP(d2f):
        fregs[DEST(inst)] = dregs[SRC1(inst)];
        break;
      case PCODE_OP(f2i):
        iregs[DEST(inst)] = fregs[SRC1(inst)];
        break;
      case PCODE_OP(d2i):
        iregs[DEST(inst)] = dregs[SRC1(inst)];
        break;
      case PCODE_OP(f2ui):
        iregs[DEST(inst)] = (uint64_t)fregs[SRC1(inst)];
        break;
      case PCODE_OP(d2ui):
        iregs[DEST(inst)] = (uint64_t)dregs[SRC1(inst)];
        break;
      case PCODE_OP(rcall):
        iregs[PCODE_SP_REG] -= 8;
        *((uint64_t*)iregs[PCODE_SP_REG]) = iregs[PCODE_PC_REG] + 8;
        iregs[PCODE_PC_REG] = iregs[DEST(inst)];
        break;
      case PCODE_OP(esc): {
        PCodeVMStatus status = HandleEscape(vm, inst & 0xffffff);
        if (status != kPCodeVMStatusRunning) {
          vm->status = status;
        }
        break;
      }
      default:
        vm->status = UndefinedInstruction(vm);
        break;
    }
    return vm->status;
  }

  bool is_64_bit = (inst & 0x40000000) == 0;
  if (is_64_bit) {
    switch ((inst >> 24) & 0x3f) {
      case PCODE_OP(ldw):
        iregs[DEST(inst)] = *(int32_t*)(iregs[SRC1(inst)] + *pc);
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(ldh):
        iregs[DEST(inst)] = *(int16_t*)(iregs[SRC1(inst)] + *pc);
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(ldb):
        iregs[DEST(inst)] = *(int8_t*)(iregs[SRC1(inst)] + *pc);
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(lduw):
        iregs[DEST(inst)] = *(uint32_t*)(iregs[SRC1(inst)] + *pc);
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(ldub):
        iregs[DEST(inst)] = *(uint8_t*)(iregs[SRC1(inst)] + *pc);
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(lduh):
        iregs[DEST(inst)] = *(uint16_t*)(iregs[SRC1(inst)] + *pc);
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(ldx):
        iregs[DEST(inst)] = *(uint64_t*)(iregs[SRC1(inst)] + *pc);
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(ldf):
        fregs[DEST(inst)] = *(float*)(iregs[SRC1(inst)] + *pc);
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(ldd):
        dregs[DEST(inst)] = *(double*)(iregs[SRC1(inst)] + *pc);
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(stw):
        *(int32_t*)(iregs[SRC1(inst)] + *pc) = (int32_t)iregs[DEST(inst)];
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(sth):
        *(int16_t*)(iregs[SRC1(inst)] + *pc) = iregs[DEST(inst)];
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(stx):
        *(uint64_t*)(iregs[SRC1(inst)] + *pc) = iregs[DEST(inst)];
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(stf):
        *(float*)(iregs[SRC1(inst)] + *pc) = fregs[DEST(inst)];
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(std):
        *(double*)(iregs[SRC1(inst)] + *pc) = dregs[DEST(inst)];
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(stb):
        *(int8_t*)(iregs[SRC1(inst)] + *pc) = iregs[DEST(inst)];
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(movc):
        iregs[DEST(inst)] = *pc;
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(movfc):
        fregs[DEST(inst)] = *(float*)pc;
        iregs[PCODE_PC_REG] += 4;
        break;
      case PCODE_OP(bz):
        iregs[PCODE_PC_REG] += iregs[DEST(inst)] == 0 ? (int32_t)*pc + 4 : 4;
        break;
      case PCODE_OP(bnz):
        iregs[PCODE_PC_REG] += iregs[DEST(inst)] != 0 ? (int32_t)*pc + 4 : 4;
        break;
      case PCODE_OP(bra):
        iregs[PCODE_PC_REG] += (int32_t)*pc + 4;
        break;
      case PCODE_OP(addc):
        iregs[DEST(inst)] = iregs[SRC1(inst)] + *pc;
        iregs[PCODE_PC_REG] += 4;
        break;
      default:
        vm->status = UndefinedInstruction(vm);
        break;
    }
    return vm->status;
  }

  switch ((inst >> 24) & 0x3f) {
    case PCODE_OP(movdc):
      dregs[DEST(inst)] = *(double*)pc;
      iregs[PCODE_PC_REG] += 8;
      break;
    case PCODE_OP(movxc):
      iregs[DEST(inst)] = *(uint64_t*)pc;
      iregs[PCODE_PC_REG] += 8;
      break;
    case PCODE_OP(jmp):
      iregs[PCODE_PC_REG] = *(uint64_t*)pc + iregs[PCODE_PC_REG] + 8;
      break;
    case PCODE_OP(call):
      iregs[PCODE_SP_REG] -= 8;
      *((uint64_t*)iregs[PCODE_SP_REG]) = iregs[PCODE_PC_REG] + 8;
      iregs[PCODE_PC_REG] = *(uint64_t*)pc + iregs[PCODE_PC_REG] + 8;
      break;
    case PCODE_OP(cjmp): {
      uint64_t offset = *(uint64_t*)pc;
      uint64_t* addr = (uint64_t*)(iregs[PCODE_PC_REG] + 8 + offset);
      iregs[PCODE_PC_REG] = *addr;
      break;
    }
    case PCODE_OP(adr):
      iregs[DEST(inst)] = *(uint64_t*)pc + iregs[PCODE_PC_REG] + 8;
      iregs[PCODE_PC_REG] += 8;
      break;
    default:
      vm->status = UndefinedInstruction(vm);
      break;
  }
  return vm->status;
}

PCodeVMStatus PCodeVMRun(PCodeVM* vm) {
  while (vm->status == kPCodeVMStatusRunning) {
    PCodeVMStep(vm);
  }
  return vm->status;
}

const char* PCodeVMStatusName(PCodeVMStatus status) {
  switch (status) {
    case kPCodeVMStatusRunning:
      return "running";
    case kPCodeVMStatusHalted:
      return "halted";
    case kPCodeVMStatusStepLimit:
      return "step limit";
    case kPCodeVMStatusUndefinedInstruction:
      return "undefined instruction";
    case kPCodeVMStatusDivisionByZero:
      return "division by zero";
    case kPCodeVMStatusUndefinedEscape:
      return "undefined escape";
  }
  return "unknown";
}
