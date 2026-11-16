//
//  target_debug_ranges.c
//  c_compiler_library
//

#include "target_debug_ranges.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "target_basic_block.h"

// As deep as the register allocator reads through operands.
enum { kMaxReadDepth = 32 };

// Debug labels are numbered across the translation unit, so that each name is
// defined once in the object file.
static int next_label;


// The liveness of the variables' locations, and of the values that copy them
// into the same registers.  Both are called locations here.
typedef struct {
  const TargetRAInfo* info;
  bool (*emits_code)(TargetInstruction* inst);
  TargetRAIndex index;  // The number of each location.
  size_t words;         // In a set of locations.
} Liveness;

static void SetBit(uint64_t* bits, int n) {
  bits[n >> 6] |= (uint64_t)1 << (n & 63);
}

static void ClearBit(uint64_t* bits, int n) {
  bits[n >> 6] &= ~((uint64_t)1 << (n & 63));
}

static bool TestBit(const uint64_t* bits, int n) {
  return ((bits[n >> 6] >> (n & 63)) & 1) != 0;
}

static int LocationNumber(Liveness* l, TargetInstruction* inst) {
  return inst == NULL ? -1 : TargetRAIndexFind(&l->index, inst);
}

// Adds the locations |op| reads to |live|, and returns whether it reads any.
static bool AddReads(Liveness* l, TargetInstruction* op, uint64_t* live,
                     int depth) {
  if (op == NULL) {
    return false;
  }
  int n = LocationNumber(l, op);
  if (n >= 0) {
    SetBit(live, n);
    return true;
  }
  bool any = false;
  if (l->info->kind(op) == kTargetRANone && depth < kMaxReadDepth &&
      l->info->reads_through(op)) {
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      any |= AddReads(l, op->operand[i], live, depth + 1);
    }
  }
  return any;
}

// Stores the numbers of the locations |inst| writes, or -1s, in |writes|:
// its dest, and itself when it is a copy.
static void Writes(Liveness* l, TargetInstruction* inst, int writes[2]) {
  writes[0] = LocationNumber(l, inst->dest);
  writes[1] =
      l->info->kind(inst) == kTargetRAValue ? LocationNumber(l, inst) : -1;
}

// Changes |live| from the locations live after |inst| to those live before.
// Returns false when |inst| neither writes nor reads any, leaving |live| as
// it was.
static bool StepBack(Liveness* l, TargetInstruction* inst, uint64_t* live) {
  int writes[2];
  Writes(l, inst, writes);
  bool any = false;
  for (int i = 0; i < 2; i++) {
    if (writes[i] >= 0) {
      ClearBit(live, writes[i]);
      any = true;
    }
  }
  for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
    any |= AddReads(l, inst->operand[i], live, 0);
  }
  return any;
}

// Sets the |l->words| words at |live_out| + |b| * |l->words| to the locations
// live at the end of block |b|.
static void ComputeLiveOut(Liveness* l, TargetGenerator* gen,
                           uint64_t* live_out) {
  size_t num_blocks = gen->basic_blocks.length;
  size_t words = l->words;
  uint64_t* gen_bits = calloc(num_blocks * words + 1, sizeof(uint64_t));
  uint64_t* kill_bits = calloc(num_blocks * words + 1, sizeof(uint64_t));
  uint64_t* live_in = calloc(num_blocks * words + 1, sizeof(uint64_t));
  for (size_t b = 0; b < num_blocks; b++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[b];
    if (block == NULL || block->code == NULL || block->end_code == NULL) {
      continue;
    }
    uint64_t* g = gen_bits + b * words;
    uint64_t* k = kill_bits + b * words;
    for (TargetInstruction* inst = block->end_code; inst != NULL;
         inst = TargetPrev(inst)) {
      int writes[2];
      Writes(l, inst, writes);
      for (int i = 0; i < 2; i++) {
        if (writes[i] >= 0) {
          SetBit(k, writes[i]);
          ClearBit(g, writes[i]);
        }
      }
      for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
        AddReads(l, inst->operand[i], g, 0);
      }
      if (inst == block->code) {
        break;
      }
    }
  }
  bool changed = true;
  while (changed) {
    changed = false;
    for (size_t b = num_blocks; b-- > 0;) {
      TargetBasicBlock* block = gen->basic_blocks.value.p[b];
      if (block == NULL) {
        continue;
      }
      uint64_t* out = live_out + b * words;
      for (size_t s = 0; s < block->out_edges.length; s++) {
        size_t succ = (size_t)block->out_edges.value.w[s];
        if (succ >= num_blocks) {
          continue;
        }
        const uint64_t* in = live_in + succ * words;
        for (size_t w = 0; w < words; w++) {
          out[w] |= in[w];
        }
      }
      uint64_t* in = live_in + b * words;
      const uint64_t* g = gen_bits + b * words;
      const uint64_t* k = kill_bits + b * words;
      for (size_t w = 0; w < words; w++) {
        uint64_t next = g[w] | (out[w] & ~k[w]);
        if (next != in[w]) {
          in[w] = next;
          changed = true;
        }
      }
    }
  }
  free(gen_bits);
  free(kill_bits);
  free(live_in);
}

static int Label(TargetGenerator* gen, TargetInstruction* inst) {
  if (gen->debug_labels == NULL) {
    gen->debug_labels = malloc(sizeof(HashMap));
    HashMapInit(gen->debug_labels, 64);
  }
  int label = TargetDebugLabel(gen, inst);
  if (label < 0) {
    label = next_label++;
    HashMapInsert(gen->debug_labels, (uint64_t)(uintptr_t)inst, label);
  }
  return label;
}

// A stretch of code over which a group of locations holds its variable: from
// debug label |begin| to debug label |end|, or to the end of the function
// when |end| is negative.
typedef struct {
  int group;
  int begin;
  int end;
} Range;

static void AppendRange(Vector* ranges, int group, int begin, int end) {
  Range* range = malloc(sizeof(Range));
  range->group = group;
  range->begin = begin;
  range->end = end;
  VectorAppend(ranges, range);
}

// False when |inst| changes no register that holds a variable, as when it
// emits no machine code.  Stretches start and end only at other instructions,
// so that they are not empty.
static bool MayEmitCode(const Liveness* l, TargetInstruction* inst) {
  // The prologue only stores registers and sets up the frame.
  if (inst->opcode == TARGET_OP(save) || !l->emits_code(inst)) {
    return false;
  }
  if (l->info->copy_kind(inst) != kTargetRANotCopy) {
    TargetRegister* out = inst->reg != NULL    ? inst->reg
                          : inst->dest != NULL ? inst->dest->reg
                                               : NULL;
    if (out != NULL && out == inst->operand[0]->reg) {
      return false;
    }
  }
  return true;
}

// Finds the code over which each group of locations holds its variable, which
// is wherever one of the group's locations is live.  Location |i| is in group
// |groups[i]|, below |num_groups|.  Appends a new Range for each stretch to
// |ranges|, in code order.
static void FindRanges(TargetGenerator* gen, Liveness* l, const int64_t* groups,
                       int num_groups, Vector* ranges) {
  size_t num_blocks = gen->basic_blocks.length;
  uint64_t* live_out = calloc(num_blocks * l->words + 1, sizeof(uint64_t));
  ComputeLiveOut(l, gen, live_out);

  // Walk the code backwards, so that each instruction's live locations
  // follow from those of the instruction after it.  A stretch is found at its
  // end, and appended when its start is.
  size_t group_words = ((size_t)num_groups + 63) / 64;
  uint64_t* live = calloc(l->words + 1, sizeof(uint64_t));
  uint64_t* held = calloc(group_words + 1, sizeof(uint64_t));  // At |next|.
  uint64_t* holds = calloc(group_words + 1, sizeof(uint64_t));
  TargetInstruction** ends =
      calloc((size_t)num_groups + 1, sizeof(TargetInstruction*));
  size_t first_range = ranges->length;
  TargetBasicBlock* block = NULL;
  TargetInstruction* next = NULL;
  bool ok = true;
  for (TargetInstruction* inst = TargetLastInstruction(gen); inst != NULL;
       inst = TargetPrev(inst)) {
    // The emitters skip instructions in no block.
    if (inst->block == NULL) {
      continue;
    }
    bool entered = false;
    if (inst->block != block) {
      block = inst->block;
      // Each block's code must be one run, entered here at its end.
      if (inst != block->end_code || block->block_id >= num_blocks) {
        ok = false;
        break;
      }
      memcpy(live, live_out + block->block_id * l->words,
             l->words * sizeof(uint64_t));
      entered = true;
    }
    if (!StepBack(l, inst, live) && !entered) {
      if (MayEmitCode(l, inst)) {
        next = inst;
      }
      continue;
    }
    memset(holds, 0, group_words * sizeof(uint64_t));
    for (size_t w = 0; w < l->words; w++) {
      for (uint64_t bits = live[w]; bits != 0; bits &= bits - 1) {
        SetBit(holds, groups[w * 64 + (size_t)__builtin_ctzll(bits)]);
      }
    }
    for (size_t w = 0; w < group_words; w++) {
      for (uint64_t bits = holds[w] ^ held[w]; bits != 0; bits &= bits - 1) {
        int g = (int)(w * 64) + __builtin_ctzll(bits);
        if (TestBit(holds, g)) {
          ends[g] = next;
        } else if (next != ends[g]) {
          AppendRange(ranges, g, Label(gen, next),
                      ends[g] == NULL ? -1 : Label(gen, ends[g]));
        }
      }
    }
    uint64_t* swap = held;
    held = holds;
    holds = swap;
    if (MayEmitCode(l, inst)) {
      next = inst;
    }
  }
  for (int g = 0; ok && g < num_groups; g++) {
    if (TestBit(held, g) && next != ends[g]) {
      AppendRange(ranges, g, Label(gen, next),
                  ends[g] == NULL ? -1 : Label(gen, ends[g]));
    }
  }
  while (!ok && ranges->length > first_range) {
    free(VectorLast(ranges));
    VectorPop(ranges);
  }
  for (size_t i = first_range, j = ranges->length; i + 1 < j; i++, j--) {
    void* swap = ranges->value.p[i];
    ranges->value.p[i] = ranges->value.p[j - 1];
    ranges->value.p[j - 1] = swap;
  }
  free(live_out);
  free(live);
  free(held);
  free(holds);
  free(ends);
}

// Numbers |location| and puts it in |group|, so that |groups| holds the group
// of each location by number.
static void AddLocation(Liveness* l, Vector* groups,
                        TargetInstruction* location, int group) {
  TargetRAIndexInsert(&l->index, location, (int)groups->length);
  VectorAppend(groups, (void*)(intptr_t)group);
}

void TargetSetDebugLocationLists(
    TargetGenerator* gen, const TargetRAInfo* info,
    const TargetDebugVariable* vars, int num_vars,
    int (*dwarf_register)(TargetGenerator* gen, TargetRegister* reg),
    bool (*emits_code)(TargetInstruction* inst)) {
  // A group is the locations of one variable that are in one register.
  size_t max = 2 * (size_t)num_vars + 1;
  int* group_vars = malloc(max * sizeof(int));
  int* group_regs = malloc(max * sizeof(int));
  bool* follows_copies = calloc(max, sizeof(bool));
  int num_groups = 0;
  Liveness l;
  l.info = info;
  l.emits_code = emits_code;
  TargetRAIndexInit(&l.index, max);
  Vector groups;
  VectorInit(&groups);
  for (int v = 0; v < num_vars; v++) {
    int first_group = num_groups;
    bool elsewhere = false;
    for (int i = 0; i < 2; i++) {
      TargetInstruction* location = vars[v].locations[i];
      if (location == NULL) {
        continue;
      }
      int reg = dwarf_register(gen, location->reg);
      if (reg < 0) {
        elsewhere = true;
        continue;
      }
      int group = first_group;
      if (num_groups == first_group || group_regs[first_group] != reg) {
        group = num_groups++;
        group_vars[group] = v;
        group_regs[group] = reg;
      }
      AddLocation(&l, &groups, location, group);
    }
    // While a copy of the variable in its register is live, a new value for
    // the variable would overwrite the copy, unless the new value goes to
    // another register.
    if (num_groups == first_group + 1 && !elsewhere) {
      follows_copies[first_group] = true;
    }
  }
  // Values that copy a location into its own register join its group.  A
  // copy of such a copy joins only when the code has the copy it reads first.
  for (TargetInstruction* inst = TargetFirstInstruction(gen); inst != NULL;
       inst = TargetNext(inst)) {
    if (inst->block == NULL || inst->reg == NULL ||
        info->kind(inst) != kTargetRAValue ||
        info->copy_kind(inst) != kTargetRACopy) {
      continue;
    }
    int n = LocationNumber(&l, inst->operand[0]);
    int group = n < 0 ? -1 : (int)groups.value.w[n];
    if (group >= 0 && follows_copies[group] &&
        dwarf_register(gen, inst->reg) == group_regs[group] &&
        LocationNumber(&l, inst) < 0) {
      AddLocation(&l, &groups, inst, group);
    }
  }
  l.words = (groups.length + 63) / 64;
  Vector ranges;
  VectorInit(&ranges);
  FindRanges(gen, &l, groups.value.w, num_groups, &ranges);
  TargetRAIndexDestruct(&l.index);
  VectorDestruct(&groups);
  free(follows_copies);

  Vector* lists = calloc((size_t)num_vars + 1, sizeof(Vector));
  String func_end = {0};
  StringAppend(&func_end, ".func_end_");
  StringAppend(&func_end, gen->function_name.value);
  for (size_t r = 0; r < ranges.length; r++) {
    const Range* range = ranges.value.p[r];
    char begin[32];
    char end[32];
    TargetDebugLabelName(range->begin, begin, sizeof(begin));
    TargetDebugLabelName(range->end, end, sizeof(end));
    int v = group_vars[range->group];
    VectorAppend(&lists[v],
                 NewVariableLocationRange(
                     begin, range->end < 0 ? func_end.value : end,
                     group_regs[range->group], vars[v].holds_address));
  }
  for (int v = 0; v < num_vars; v++) {
    if (lists[v].length != 0) {
      VariableDIESetLocationList(SymbolDIE(vars[v].symbol), &lists[v]);
    }
    VectorDestruct(&lists[v]);
  }
  StringDestruct(&func_end);
  free(lists);
  VectorDestructWithContents(&ranges, NULL, /*free_element=*/true);
  free(group_vars);
  free(group_regs);
}

void TargetDebugLabelName(int label, char* buf, size_t size) {
  snprintf(buf, size, ".Ldl%d", label);
}
