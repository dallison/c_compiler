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

// How many instructions still read this value.  A user the optimizer has
// already removed is not one of them.
static size_t LiveUsers(TargetInstruction* inst) {
  size_t live = 0;
  for (size_t i = 0; i < inst->users.length; i++) {
    TargetInstruction* user = inst->users.value.p[i];
    if ((user->flags & TARGET_INST_DEAD) == 0) {
      live++;
    }
  }
  return live;
}

// True when the only thing an instruction does is produce its value, so that
// nothing reading it makes the instruction itself unnecessary.  A call is
// excluded because the value is the least of what it does, and so is
// anything that grows memory.
//
// Every load is excluded too, for want of a way to tell one from a volatile
// one: by the time an access is a wasm opcode the type that said so is gone,
// and reading a volatile object is required to happen whether or not anybody
// wants the answer.  The loads worth removing are mostly gone already, the
// IR running its own dead code elimination first.
static bool IsPureValue(TargetInstruction* inst) {
  if (!Wasm32ProducesValue(inst) || Wasm32IsPseudo(inst)) {
    return false;
  }
  switch ((Wasm32Opcode)inst->opcode) {
    case W_OP(call):
    case W_OP(call_indirect):
    case W_OP(memory_grow):
    case W_OP(i32_load):
    case W_OP(i64_load):
    case W_OP(f32_load):
    case W_OP(f64_load):
    case W_OP(i32_load8_s):
    case W_OP(i32_load8_u):
    case W_OP(i32_load16_s):
    case W_OP(i32_load16_u):
    case W_OP(i64_load8_s):
    case W_OP(i64_load8_u):
    case W_OP(i64_load16_s):
    case W_OP(i64_load16_u):
    case W_OP(i64_load32_s):
    case W_OP(i64_load32_u):
      return false;
    default:
      return true;
  }
}

// Lowering emits a value wherever the IR mentions one, and the IR mentions
// plenty that nothing goes on to read: a widened condition the branch takes
// in its narrow form, an address computed for an access that folded the
// offset into its memarg.  Dropping one can leave its own operands unread in
// turn, so this repeats until nothing more falls out.
static void RemoveUnreadValues(Wasm32Generator* wasm) {
  bool changed = true;
  while (changed) {
    changed = false;
    for (TargetInstruction* inst = TargetFirstInstruction(&wasm->base);
         inst != NULL; inst = TargetNext(inst)) {
      // An instruction with a destination writes into a local someone else
      // owns, and that write is the point of it.
      if ((inst->flags & TARGET_INST_DEAD) != 0 || inst->dest != NULL ||
          !IsPureValue(inst) || LiveUsers(inst) != 0) {
        continue;
      }
      inst->flags |= TARGET_INST_DEAD;
      changed = true;
    }
  }
}

// An instruction whose whole job is to put its operand into a local: either
// a move, or the local.set that writing a promoted variable comes down to.
static bool IsCopy(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)W_OP(mov) ||
         inst->opcode == (TargetOpcode)W_OP(movf) ||
         inst->opcode == (TargetOpcode)W_OP(movd) ||
         inst->opcode == (TargetOpcode)W_OP(local_set);
}

// Hand everything that reads 'from' the value 'to' produces instead.
static void RedirectUsers(TargetInstruction* from, TargetInstruction* to) {
  for (size_t i = 0; i < from->users.length; i++) {
    TargetInstruction* user = from->users.value.p[i];
    for (int operand = 0; operand < TARGET_MAX_OPERANDS; operand++) {
      if (user->operand[operand] == from) {
        user->operand[operand] = to;
      }
    }
    TargetAddUser(to, user);
  }
}

// A value is computed into a local of its own and then copied into the local
// it actually belongs in, which costs a store and a load that say nothing:
//
//     i32.add / local.set $t / local.get $t / local.set $x
//
// When the copy is the only reader of the value and follows it with nothing
// in between, the value can be computed straight into its destination and
// the copy dropped, leaving a single local.set.
static void ForwardValuesIntoDestinations(Wasm32Generator* wasm) {
  for (TargetInstruction* copy = TargetFirstInstruction(&wasm->base);
       copy != NULL; copy = TargetNext(copy)) {
    if (!IsCopy(copy) || (copy->flags & TARGET_INST_DEAD) != 0) {
      continue;
    }
    // A copy with neither a destination nor a local of its own writes
    // nowhere, and dropping it would lose the value rather than move it.
    if (copy->dest == NULL && !Wasm32ProducesValue(copy)) {
      continue;
    }
    TargetInstruction* producer = copy->operand[0];
    // A pseudo names a local rather than producing a value, and a producer
    // that already has a destination is writing somewhere for a reason.
    if (producer == NULL || (producer->flags & TARGET_INST_DEAD) != 0 ||
        producer->dest != NULL || Wasm32IsPseudo(producer) ||
        !Wasm32ProducesValue(producer)) {
      continue;
    }
    if (LiveUsers(producer) != 1 || NextRealInstruction(producer) != copy) {
      continue;
    }
    // The local written, which is the copy's own when it has no destination.
    TargetInstruction* destination = copy->dest != NULL ? copy->dest : copy;
    // Widening or narrowing is the copy's whole purpose where the two differ.
    if (Wasm32InstructionType(producer) != Wasm32InstructionType(destination)) {
      continue;
    }

    if (copy->dest != NULL) {
      producer->dest = copy->dest;
    } else {
      // The copy owned the local its readers name, so they can read the
      // producer's instead.
      RedirectUsers(copy, producer);
    }
    copy->flags |= TARGET_INST_DEAD;
  }
}

void Wasm32Optimize(Wasm32Generator* wasm) {
  RemoveFallthroughBranches(wasm);
  ForwardSingleUseCopies(wasm);
  ForwardValuesIntoDestinations(wasm);
  // Last, because the two passes above are what leave values unread.
  RemoveUnreadValues(wasm);
}

void Wasm32OptimizeStackified(Wasm32Generator* wasm) {
  ForwardValuesIntoDestinations(wasm);
  RemoveUnreadValues(wasm);
}
