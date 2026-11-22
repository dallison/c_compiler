//
//  target_ra_ir.c
//  c_compiler_library
//
//  See target_ra_ir.h for the rules this checks.
//

#include "target_ra_ir.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "target_basic_block.h"

enum { kMaxDefsPerInst = TARGET_RA_FIXED_KEYS + 2 };

enum {
  kDetailOrphaned,
  kDetailUndominated,
  kDetailDestNotLocation,
  kDetailWrittenOperand,
  kDetailOverwritten,
  kNumDetails,
};

typedef struct {
  TargetGenerator* gen;
  const TargetRAInfo* info;
  TargetRACheckCounts counts;
  FILE* fp;
  int max_details;
  int details[kNumDetails];

  // Instructions in block order; each block's instructions are contiguous.
  int num_insts;
  Vector insts;
  Vector inst_block;
  Vector inst_pos;
  TargetRAIndex positions;
  Vector block_first;  // Index in insts of each block's first instruction.

  // Locations: physical register keys first, then one per instruction.
  TargetRAIndex location_ids;
  int num_locations;
  // A location some value read reaches through the value's dest.
  BitSet candidate;

  // Definitions of candidate locations, in block order.
  int num_defs;
  Vector def_inst;
  Vector def_loc;
  Vector block_def_start;  // Per block, plus one past the last block.
  Vector loc_def_start;    // Per location, plus one past the last location.
  Vector loc_defs;         // Definition indices grouped by location.

  // Definitions reaching each block's entry and exit.
  size_t num_words;
  Vector in;
  Vector out;
} RAFunction;

static TargetInstruction* NextInBlock(TargetBasicBlock* block,
                                      TargetInstruction* inst) {
  return inst == block->end_code ? NULL : TargetNext(inst);
}

static TargetBasicBlock* BlockAt(TargetGenerator* gen, size_t i) {
  TargetBasicBlock* block = gen->basic_blocks.value.p[i];
  if (block == NULL || block->code == NULL || block->end_code == NULL) {
    return NULL;
  }
  return block;
}

static int LocationOf(RAFunction* fn, TargetInstruction* location) {
  int key = fn->info->location_key(location);
  if (key >= 0 && key < TARGET_RA_FIXED_KEYS) {
    return key;
  }
  int id = TargetRAIndexFind(&fn->location_ids, location);
  if (id < 0) {
    id = fn->num_locations++;
    TargetRAIndexInsert(&fn->location_ids, location, id);
  }
  return id;
}

static bool IsLocation(RAFunction* fn, TargetInstruction* inst) {
  return inst != NULL && fn->info->kind(inst) == kTargetRALocation;
}

static int DestKind(RAFunction* fn, TargetInstruction* dest) {
  int key = fn->info->location_key(dest);
  if (key >= 0 && key < TARGET_RA_FIXED_KEYS) {
    return kTargetRADestFixed;
  }
  if ((int)dest->opcode == (int)TARGET_OP(tmp)) {
    return kTargetRADestTmp;
  }
  return kTargetRADestVar;
}

// The locations |inst| writes, in the order it writes them.
static int DefinedLocations(RAFunction* fn, TargetInstruction* inst,
                            int* locs) {
  int n = fn->info->clobbers(inst, locs, kMaxDefsPerInst - 2);
  int written = fn->info->written_operand(inst);
  if (written >= 0 && IsLocation(fn, inst->operand[written])) {
    locs[n++] = LocationOf(fn, inst->operand[written]);
  }
  if (IsLocation(fn, inst->dest)) {
    locs[n++] = LocationOf(fn, inst->dest);
  }
  return n;
}

static bool DefinesLocation(RAFunction* fn, TargetInstruction* inst,
                            int loc) {
  int locs[kMaxDefsPerInst];
  int n = DefinedLocations(fn, inst, locs);
  for (int i = 0; i < n; i++) {
    if (locs[i] == loc) {
      return true;
    }
  }
  return false;
}

static void Detail(RAFunction* fn, int which, const char* what,
                   TargetInstruction* inst, int op, TargetInstruction* value,
                   TargetInstruction* other) {
  if (fn->fp == NULL || fn->details[which] >= fn->max_details) {
    return;
  }
  fn->details[which]++;
  const char* (*name)(int) = fn->gen->virtuals->opcode_name;
  fprintf(fn->fp, "ra-check-detail\t%s\t%s\t@%d:%s",
          fn->gen->function_name.value, what, inst->id, name(inst->opcode));
  if (value != NULL) {
    fprintf(fn->fp, " op%d=@%d:%s", op, value->id, name(value->opcode));
    if (value->dest != NULL) {
      fprintf(fn->fp, " dest=@%d:%s", value->dest->id,
              name(value->dest->opcode));
    }
  }
  if (other != NULL) {
    fprintf(fn->fp, " by=@%d:%s", other->id, name(other->opcode));
  }
  fprintf(fn->fp, "\n");
}

// Numbers the instructions and finds the locations that some value read
// reaches through its dest.
static void CollectInstructions(RAFunction* fn) {
  TargetGenerator* gen = fn->gen;
  size_t num_blocks = gen->basic_blocks.length;
  int count = 0;
  for (size_t b = 0; b < num_blocks; b++) {
    TargetBasicBlock* block = BlockAt(gen, b);
    for (TargetInstruction* inst = block == NULL ? NULL : block->code;
         inst != NULL; inst = NextInBlock(block, inst)) {
      count++;
    }
  }
  VectorInit(&fn->insts);
  VectorInit(&fn->inst_block);
  VectorInit(&fn->inst_pos);
  VectorInit(&fn->block_first);
  VectorResize(&fn->insts, (size_t)(count + 1));
  VectorResize(&fn->inst_block, (size_t)(count + 1));
  VectorResize(&fn->inst_pos, (size_t)(count + 1));
  VectorResize(&fn->block_first, num_blocks + 1);
  TargetRAIndexInit(&fn->positions, (size_t)count);
  TargetRAIndexInit(&fn->location_ids, 64);
  fn->num_locations = TARGET_RA_FIXED_KEYS;
  BitSetInit(&fn->candidate);

  int n = 0;
  for (size_t b = 0; b < num_blocks; b++) {
    fn->block_first.value.w[b] = n;
    TargetBasicBlock* block = BlockAt(gen, b);
    int pos = 0;
    for (TargetInstruction* inst = block == NULL ? NULL : block->code;
         inst != NULL; inst = NextInBlock(block, inst)) {
      fn->insts.value.p[n] = inst;
      fn->inst_block.value.w[n] = (int64_t)b;
      fn->inst_pos.value.w[n] = pos++;
      TargetRAIndexInsert(&fn->positions, inst, n);
      n++;
    }
  }
  fn->block_first.value.w[num_blocks] = n;
  fn->num_insts = n;

  for (int i = 0; i < n; i++) {
    TargetInstruction* inst = fn->insts.value.p[i];
    if (inst->dest != NULL && !IsLocation(fn, inst->dest)) {
      fn->counts.dest_not_location++;
      Detail(fn, kDetailDestNotLocation, "dest-not-location", inst, -1,
             inst->dest, NULL);
    }
    int written = fn->info->written_operand(inst);
    if (written >= 0) {
      fn->counts.written_operands++;
      Detail(fn, kDetailWrittenOperand, "written-operand", inst, written,
             inst->operand[written], NULL);
    }
    for (int op = 0; op < TARGET_MAX_OPERANDS; op++) {
      TargetInstruction* value = inst->operand[op];
      if (op == written || value == NULL ||
          fn->info->kind(value) != kTargetRAValue ||
          !IsLocation(fn, value->dest)) {
        continue;
      }
      BitSetInsert(&fn->candidate, (size_t)LocationOf(fn, value->dest));
    }
  }
}

static void AddDef(RAFunction* fn, int inst_index, int loc) {
  VectorAppend(&fn->def_inst, (void*)(intptr_t)inst_index);
  VectorAppend(&fn->def_loc, (void*)(intptr_t)loc);
  fn->num_defs++;
}

static void CollectDefinitions(RAFunction* fn) {
  size_t num_blocks = fn->gen->basic_blocks.length;
  VectorInit(&fn->def_inst);
  VectorInit(&fn->def_loc);
  VectorInit(&fn->block_def_start);
  VectorResize(&fn->block_def_start, num_blocks + 1);
  for (size_t b = 0; b < num_blocks; b++) {
    fn->block_def_start.value.w[b] = fn->num_defs;
    for (int i = (int)fn->block_first.value.w[b];
         i < (int)fn->block_first.value.w[b + 1]; i++) {
      int locs[kMaxDefsPerInst];
      int n = DefinedLocations(fn, fn->insts.value.p[i], locs);
      for (int k = 0; k < n; k++) {
        if (BitSetContains(&fn->candidate, (size_t)locs[k])) {
          AddDef(fn, i, locs[k]);
        }
      }
    }
  }
  fn->block_def_start.value.w[num_blocks] = fn->num_defs;

  int num_locations = fn->num_locations;
  VectorInit(&fn->loc_def_start);
  VectorInit(&fn->loc_defs);
  VectorResize(&fn->loc_def_start, (size_t)num_locations + 1);
  VectorResize(&fn->loc_defs, (size_t)(fn->num_defs + 1));
  for (int d = 0; d < fn->num_defs; d++) {
    fn->loc_def_start.value.w[fn->def_loc.value.w[d] + 1]++;
  }
  for (int l = 0; l < num_locations; l++) {
    fn->loc_def_start.value.w[l + 1] += fn->loc_def_start.value.w[l];
  }
  Vector fill;
  VectorInit(&fill);
  VectorCopy(&fill, &fn->loc_def_start);
  for (int d = 0; d < fn->num_defs; d++) {
    int64_t loc = fn->def_loc.value.w[d];
    fn->loc_defs.value.w[fill.value.w[loc]++] = d;
  }
  VectorDestruct(&fill);
}

static bool TestBit(const uint64_t* bits, int i) {
  return (bits[i / 64] >> (i % 64)) & 1;
}

static void SetBit(uint64_t* bits, int i) {
  bits[i / 64] |= UINT64_C(1) << (i % 64);
}

static void ClearBit(uint64_t* bits, int i) {
  bits[i / 64] &= ~(UINT64_C(1) << (i % 64));
}

// Standard reaching definitions over the candidate locations.
static void ComputeReachingDefinitions(RAFunction* fn) {
  TargetGenerator* gen = fn->gen;
  size_t num_blocks = gen->basic_blocks.length;
  size_t words = ((size_t)fn->num_defs + 63) / 64;
  fn->num_words = words;
  VectorInit(&fn->in);
  VectorInit(&fn->out);
  VectorResize(&fn->in, num_blocks * words + 1);
  VectorResize(&fn->out, num_blocks * words + 1);
  if (words == 0) {
    return;
  }
  Vector next;
  VectorInit(&next);
  VectorResize(&next, words);
  bool changed = true;
  while (changed) {
    changed = false;
    for (size_t b = 0; b < num_blocks; b++) {
      // An empty block has no definitions and passes its input through.
      TargetBasicBlock* block = gen->basic_blocks.value.p[b];
      if (block == NULL) {
        continue;
      }
      uint64_t* in = fn->in.value.u + b * words;
      for (size_t e = 0; e < block->in_edges.length; e++) {
        size_t pred = (size_t)block->in_edges.value.w[e];
        if (pred >= num_blocks) {
          continue;
        }
        const uint64_t* pred_out = fn->out.value.u + pred * words;
        for (size_t w = 0; w < words; w++) {
          in[w] |= pred_out[w];
        }
      }
      memcpy(next.value.u, in, sizeof(uint64_t) * words);
      for (int d = (int)fn->block_def_start.value.w[b];
           d < (int)fn->block_def_start.value.w[b + 1]; d++) {
        int loc = (int)fn->def_loc.value.w[d];
        for (int k = (int)fn->loc_def_start.value.w[loc];
             k < (int)fn->loc_def_start.value.w[loc + 1]; k++) {
          ClearBit(next.value.u, (int)fn->loc_defs.value.w[k]);
        }
        SetBit(next.value.u, d);
      }
      uint64_t* out = fn->out.value.u + b * words;
      if (memcmp(next.value.u, out, sizeof(uint64_t) * words) != 0) {
        memcpy(out, next.value.u, sizeof(uint64_t) * words);
        changed = true;
      }
    }
  }
  VectorDestruct(&next);
}

// Another write of |loc| that a read at insts[use] can see instead of the
// write |value| (insts[value_index]) made, or NULL.
static TargetInstruction* OverwritingDefinition(RAFunction* fn, int use,
                                                int value_index, int loc) {
  int block = (int)fn->inst_block.value.w[use];
  for (int i = use - 1; i >= (int)fn->block_first.value.w[block]; i--) {
    if (i == value_index) {
      return NULL;
    }
    if (DefinesLocation(fn, fn->insts.value.p[i], loc)) {
      return fn->insts.value.p[i];
    }
  }
  if (fn->num_words == 0) {
    return NULL;
  }
  const uint64_t* in = fn->in.value.u + (size_t)block * fn->num_words;
  for (int k = (int)fn->loc_def_start.value.w[loc];
       k < (int)fn->loc_def_start.value.w[loc + 1]; k++) {
    int d = (int)fn->loc_defs.value.w[k];
    if (TestBit(in, d) && (int)fn->def_inst.value.w[d] != value_index) {
      return fn->insts.value.p[fn->def_inst.value.w[d]];
    }
  }
  return NULL;
}

static void CheckReads(RAFunction* fn) {
  TargetGenerator* gen = fn->gen;
  for (int u = 0; u < fn->num_insts; u++) {
    TargetInstruction* inst = fn->insts.value.p[u];
    TargetBasicBlock* block = BlockAt(gen, (size_t)fn->inst_block.value.w[u]);
    int written = fn->info->written_operand(inst);
    for (int op = 0; op < TARGET_MAX_OPERANDS; op++) {
      TargetInstruction* value = inst->operand[op];
      if (op == written || value == NULL ||
          fn->info->kind(value) != kTargetRAValue) {
        continue;
      }
      fn->counts.reads++;
      int v = TargetRAIndexFind(&fn->positions, value);
      if (v < 0) {
        fn->counts.orphaned++;
        Detail(fn, kDetailOrphaned, "orphaned", inst, op, value, NULL);
        continue;
      }
      bool dominated =
          fn->inst_block.value.w[v] == fn->inst_block.value.w[u]
              ? fn->inst_pos.value.w[v] < fn->inst_pos.value.w[u]
              : BitSetContains(&block->dominators,
                               (size_t)fn->inst_block.value.w[v]);
      if (!dominated) {
        fn->counts.undominated++;
        Detail(fn, kDetailUndominated,
               gen->exception_edges.length != 0 ? "undominated-eh"
                                                : "undominated",
               inst, op, value, NULL);
      }
      if (!IsLocation(fn, value->dest)) {
        continue;
      }
      fn->counts.dest_reads++;
      int loc = LocationOf(fn, value->dest);
      TargetInstruction* by = OverwritingDefinition(fn, u, v, loc);
      if (by != NULL) {
        fn->counts.overwritten[DestKind(fn, value->dest)]++;
        Detail(fn, kDetailOverwritten, "overwritten", inst, op, value, by);
      }
    }
  }
}

static void AddCounts(TargetRACheckCounts* to,
                      const TargetRACheckCounts* from) {
  to->functions += from->functions;
  to->reads += from->reads;
  to->dest_reads += from->dest_reads;
  to->undominated += from->undominated;
  to->orphaned += from->orphaned;
  to->dest_not_location += from->dest_not_location;
  to->written_operands += from->written_operands;
  for (int k = 0; k < kTargetRADestKinds; k++) {
    to->overwritten[k] += from->overwritten[k];
  }
}

void TargetRACheck(TargetGenerator* gen, const TargetRAInfo* info,
                   TargetRACheckCounts* counts, FILE* fp, int max_details) {
  RAFunction fn;
  memset(&fn, 0, sizeof(fn));
  fn.gen = gen;
  fn.info = info;
  fn.fp = fp;
  fn.max_details = max_details;
  fn.counts.functions = 1;

  CollectInstructions(&fn);
  CollectDefinitions(&fn);
  ComputeReachingDefinitions(&fn);
  CheckReads(&fn);

  if (fp != NULL) {
    const TargetRACheckCounts* c = &fn.counts;
    fprintf(fp,
            "ra-check\t%s\treads=%d\tdest_reads=%d\tundominated=%d\t"
            "orphaned=%d\tdest_not_location=%d\twritten_operands=%d\t"
            "overwritten_var=%d\toverwritten_tmp=%d\toverwritten_fixed=%d\n",
            gen->function_name.value, c->reads, c->dest_reads, c->undominated,
            c->orphaned, c->dest_not_location, c->written_operands,
            c->overwritten[kTargetRADestVar], c->overwritten[kTargetRADestTmp],
            c->overwritten[kTargetRADestFixed]);
  }
  if (counts != NULL) {
    AddCounts(counts, &fn.counts);
  }

  VectorDestruct(&fn.insts);
  VectorDestruct(&fn.inst_block);
  VectorDestruct(&fn.inst_pos);
  VectorDestruct(&fn.block_first);
  TargetRAIndexDestruct(&fn.positions);
  TargetRAIndexDestruct(&fn.location_ids);
  BitSetDestruct(&fn.candidate);
  VectorDestruct(&fn.def_inst);
  VectorDestruct(&fn.def_loc);
  VectorDestruct(&fn.block_def_start);
  VectorDestruct(&fn.loc_def_start);
  VectorDestruct(&fn.loc_defs);
  VectorDestruct(&fn.in);
  VectorDestruct(&fn.out);
}
