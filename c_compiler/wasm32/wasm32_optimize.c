//
//  wasm32_optimize.c
//  c_compiler
//

#include "wasm32_optimize.h"

// The next instruction that will actually be encoded, skipping pseudo
// operations and anything already marked dead.
static TargetInstruction* NextRealInstruction(TargetInstruction* inst) {
  TargetInstruction* next = TargetNext(inst);
  while (next != NULL) {
    if ((next->flags & TARGET_INST_DEAD) == 0 && !Wasm32IsPseudo(next)) {
      return next;
    }
    next = TargetNext(next);
  }
  return NULL;
}

// A branch whose target label is the very next thing in the instruction
// stream is a fallthrough and encodes to nothing.
static void RemoveFallthroughBranches(Wasm32Generator* wasm) {
  TargetInstruction* inst = TargetFirstInstruction(&wasm->base);
  while (inst != NULL) {
    if (inst->opcode == (TargetOpcode)W_OP(br)) {
      TargetInstruction* target = inst->operand[1];
      if (target != NULL && NextRealInstruction(inst) == target) {
        inst->flags |= TARGET_INST_DEAD;
      }
    }
    inst = TargetNext(inst);
  }
}

// Anything that can begin or end a straight run of instructions.  A value
// must not be forwarded across one: the reader could be arrived at by a path
// that never ran the instruction producing it.
static bool BreaksStraightLine(TargetInstruction* inst) {
  switch ((Wasm32Opcode)inst->opcode) {
    case W_OP(label):
    case W_OP(named_label):
    case W_OP(block):
    case W_OP(loop):
    case W_OP(if):
    case W_OP(else):
    case W_OP(end):
    case W_OP(br):
    case W_OP(br_if):
    case W_OP(br_table):
    case W_OP(return):
    case W_OP(ret):
    case W_OP(unreachable):
      return true;
    default:
      return false;
  }
}

// True when the source of a copy still holds the copied value where the
// reader picks it up.
static bool CopyReachesReader(TargetInstruction* copy,
                              TargetInstruction* reader) {
  TargetInstruction* source = copy->operand[0];
  for (TargetInstruction* inst = TargetNext(copy); inst != NULL;
       inst = TargetNext(inst)) {
    if (inst == reader) {
      return true;
    }
    if ((inst->flags & TARGET_INST_DEAD) != 0) {
      continue;
    }
    if (inst->dest == source || BreaksStraightLine(inst)) {
      return false;
    }
  }
  return false;
}

// Reading a variable that lives in a wasm local copies it into a local of its
// own, so that the read happens where the program says it does rather than
// wherever the value is eventually used.  When only one instruction reads the
// copy and nothing writes the variable in between, that distinction makes no
// difference and the reader can name the variable's own local.
static void ForwardSingleUseCopies(Wasm32Generator* wasm) {
  // A copy that something else writes into is sharing its local on purpose,
  // and the local has to stay.
  Vector shared;
  VectorInit(&shared);
  for (TargetInstruction* inst = TargetFirstInstruction(&wasm->base);
       inst != NULL; inst = TargetNext(inst)) {
    if (inst->dest != NULL) {
      VectorAppend(&shared, inst->dest);
    }
  }

  for (TargetInstruction* copy = TargetFirstInstruction(&wasm->base);
       copy != NULL; copy = TargetNext(copy)) {
    if (copy->opcode != (TargetOpcode)W_OP(mov) || copy->dest != NULL ||
        (copy->flags & TARGET_INST_DEAD) != 0 ||
        !Wasm32NamesLocal(copy->operand[0]) || copy->users.length != 1) {
      continue;
    }
    bool is_shared = false;
    for (size_t i = 0; i < shared.length && !is_shared; i++) {
      is_shared = shared.value.p[i] == copy;
    }
    if (is_shared) {
      continue;
    }
    TargetInstruction* reader = copy->users.value.p[0];
    if (!CopyReachesReader(copy, reader)) {
      continue;
    }
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      if (reader->operand[i] == copy) {
        reader->operand[i] = copy->operand[0];
      }
    }
    TargetAddUser(copy->operand[0], reader);
    copy->flags |= TARGET_INST_DEAD;
  }
  VectorDestruct(&shared);
}

void Wasm32Optimize(Wasm32Generator* wasm) {
  RemoveFallthroughBranches(wasm);
  ForwardSingleUseCopies(wasm);
}
