//
//  target_reg_alloc.c
//  c_compiler_library
//
//  See target_reg_alloc.h.  The coloring follows "Iterated Register
//  Coalescing", George and Appel, TOPLAS 1996.
//

#include "target_reg_alloc.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashset.h"
#include "target_basic_block.h"
#include "vector.h"

enum {
  kNoNode = -1,
  // Clobbers, implicit uses or forbidden keys.
  kMaxKeys = TARGET_RA_FIXED_KEYS,
  kMaxReadDepth = 32,  // Operands read through, such as an argument chain.
  kMaxRounds = 32,
  kChunkBits = 8,
  kChunk = 1 << kChunkBits,
  kFirstNeighbors = 16,  // Room a node's adjacency list starts with.
  // Beyond this many nodes, the edges go in a set rather than a bit matrix.
  kMaxMatrixNodes = 4096,
};

#define kUnspillableCost 1e30

// The physical registers fill whole words of a live set.
typedef char FixedKeysFillWords[TARGET_RA_FIXED_KEYS % 64 == 0 ? 1 : -1];


static void AppendInt(Vector* vec, int x) {
  VectorAppend(vec, (void*)(intptr_t)x);
}

static int IntAt(const Vector* vec, size_t i) { return (int)vec->value.w[i]; }

static int PopInt(Vector* vec) { return (int)vec->value.w[--vec->length]; }

// Elements of one size kept kChunk to a block, so that a pointer to one
// stays valid as more are added.
typedef struct {
  Vector chunks;
  int length;
} ChunkArray;

static void* ChunkAt(const ChunkArray* array, int i, size_t size) {
  char* chunk = array->chunks.value.p[i >> kChunkBits];
  return chunk + size * (size_t)(i & (kChunk - 1));
}

// Adds a zeroed element.  An array whose length was reset reuses its chunks.
static void* ChunkAdd(ChunkArray* array, size_t size) {
  int i = array->length++;
  if ((size_t)(i >> kChunkBits) == array->chunks.length) {
    VectorAppend(&array->chunks, malloc(size * kChunk));
  }
  void* p = ChunkAt(array, i, size);
  memset(p, 0, size);
  return p;
}

static void ChunkFree(ChunkArray* array) {
  VectorDestructWithContents(&array->chunks, NULL, /*free_element=*/true);
  array->length = 0;
}

// Interference edges are keyed once, by low << 32 | high.
static uint64_t EdgeKey(int a, int b) {
  return a < b ? ((uint64_t)a << 32) | (uint32_t)b
               : ((uint64_t)b << 32) | (uint32_t)a;
}

typedef enum {
  kNodePrecolored,
  kNodeInitial,
  kNodeSimplify,
  kNodeFreeze,
  kNodeSpill,
  kNodeSpilled,
  kNodeCoalesced,
  kNodeColored,
  kNodeSelected,
} NodeState;

typedef enum {
  kMoveWorklist,
  kMoveActive,
  kMoveCoalesced,
  kMoveConstrained,
  kMoveFrozen,
} MoveState;

typedef struct {
  int a;
  int b;
  int weight;
  MoveState state;
} Move;

// A value or location to color, or a physical register.
typedef struct {
  TargetInstruction* inst;  // NULL for a physical register.
  int reg_class;            // -1 for a register that is not allocatable.
  NodeState state;
  int alias;
  int color;
  int degree;
  int mark;
  double cost;
  // The instructions where its class has more values live than registers
  // and spilling it would free its register.
  int relief;
  Vector adj;    // Neighbors.
  Vector moves;  // Indices of the moves this node takes part in.
  // The physical registers among its neighbors, by key.
  uint64_t fixed_adj[TARGET_RA_FIXED_KEYS / 64];
} Node;

typedef struct {
  TargetInstruction* inst;
  int block;
  // Ranges in RA::uses and RA::defs.
  int first_use;
  int end_use;
  int first_def;
  int end_def;
  // Nodes that hold one value just after the instruction executes: the copy
  // source, the instruction's own result and its dest.
  int group[3];
  // For a copy whose two sides must differ, its source; otherwise kNoNode.
  int converted;
  bool early_clobber;  // Its register must differ from its operands'.
} InstInfo;

// What the allocator keeps about an instruction it has met.  The instruction
// holds its index in RA::slots, which counts only if the slot names it back,
// so an index left from another function or copied with the instruction is
// harmless.
typedef struct {
  TargetInstruction* inst;
  int reads;     // Operand reads, as of the last CountAllReads.
  int node;      // Its node in the current round, or kNoNode.
  int position;  // Its index in RA::insts, or -1.
  TargetInstruction* fixed_copy;  // The copy CopyFixedValues reads it through.
  bool unspillable;  // A value the spill code gave a short range.
  bool spilled_location;
  bool transparent;  // A copy added for a value in a fixed register.
  bool split_copy;   // A copy joining the two sides of a split.
} Slot;

typedef struct {
  TargetGenerator* gen;
  const TargetRegAllocDesc* desc;
  const TargetRAInfo* info;
  int key_class[TARGET_RA_FIXED_KEYS];
  int key_index[TARGET_RA_FIXED_KEYS];  // Position in its class's regs.
  int k[TARGET_RA_MAX_CLASSES];

  ChunkArray slots;  // Slot.
  ChunkArray insts;  // InstInfo, in block order.

  // Node.  The first TARGET_RA_FIXED_KEYS are the physical registers.
  ChunkArray nodes;
  Vector uses;  // Nodes each instruction reads.
  Vector defs;  // Nodes each instruction writes.

  // Live nodes at the start and end of each block.
  size_t words;
  uint64_t* live_in;
  uint64_t* live_out;
  bool* pads;  // Exception landing pads, by block id.

  // Interference edges between nodes that are not physical registers: a
  // triangular bit matrix by MatrixBit, or for a large function a set by
  // EdgeKey.  Node::fixed_adj holds the edges to physical registers.
  uint64_t* matrix;
  HashSet edges;
  ChunkArray moves;  // Move.
  Vector simplify_list;
  Vector freeze_list;
  Vector spill_list;
  Vector move_worklist;
  Vector select_stack;
  int mark_gen;
  bool changed_ir;  // By fixed copies, splitting or spilling.

  TargetRegAllocStats stats;
} RA;

static InstInfo* InstAt(RA* ra, int i) {
  return ChunkAt(&ra->insts, i, sizeof(InstInfo));
}

static int NumInsts(RA* ra) { return ra->insts.length; }

static Slot* SlotAt(RA* ra, int s) {
  return ChunkAt(&ra->slots, s, sizeof(Slot));
}

// The slot of |inst|, or NULL if it has none.
static Slot* FindSlot(RA* ra, const TargetInstruction* inst) {
  int s = inst->ra_slot;
  return s >= 0 && s < ra->slots.length && SlotAt(ra, s)->inst == inst
             ? SlotAt(ra, s)
             : NULL;
}

// The slot of |inst|, made if need be.
static Slot* GetSlot(RA* ra, TargetInstruction* inst) {
  Slot* slot = FindSlot(ra, inst);
  if (slot != NULL) {
    return slot;
  }
  inst->ra_slot = ra->slots.length;
  slot = ChunkAdd(&ra->slots, sizeof(Slot));
  *slot = (Slot){.inst = inst, .node = kNoNode, .position = -1};
  return slot;
}

static int Position(RA* ra, const TargetInstruction* inst) {
  const Slot* slot = FindSlot(ra, inst);
  return slot == NULL ? -1 : slot->position;
}

static bool IsUnspillable(RA* ra, const TargetInstruction* inst) {
  const Slot* slot = FindSlot(ra, inst);
  return slot != NULL && slot->unspillable;
}

static bool CanSpill(RA* ra, TargetInstruction* inst) {
  return !IsUnspillable(ra, inst) &&
         (ra->desc->can_spill == NULL ||
          ra->desc->can_spill(ra->desc->ctx, inst));
}

static TargetInstruction* InstOf(RA* ra, int i) { return InstAt(ra, i)->inst; }

static TargetBasicBlock* InstBlock(RA* ra, int i) {
  return ra->gen->basic_blocks.value.p[InstAt(ra, i)->block];
}

static Node* NodeAt(RA* ra, int n) {
  return ChunkAt(&ra->nodes, n, sizeof(Node));
}

static int NumNodes(RA* ra) { return ra->nodes.length; }

static Move* MoveAt(RA* ra, int m) {
  return ChunkAt(&ra->moves, m, sizeof(Move));
}

static bool IsPrecolored(int n) { return n < TARGET_RA_FIXED_KEYS; }

// The bit for the edge between |a| and |b|, which differ and are not physical
// registers.
static size_t MatrixBit(int a, int b) {
  size_t lo = (size_t)((a < b ? a : b) - TARGET_RA_FIXED_KEYS);
  size_t hi = (size_t)((a < b ? b : a) - TARGET_RA_FIXED_KEYS);
  return hi * (hi - 1) / 2 + lo;
}

// Where the edge between |a| and |b|, which differ and are not both physical
// registers, is kept: a word and a bit in it.
static uint64_t* EdgeWord(RA* ra, int a, int b, uint64_t* mask) {
  if (IsPrecolored(a) || IsPrecolored(b)) {
    int key = IsPrecolored(a) ? a : b;
    *mask = UINT64_C(1) << (key % 64);
    return &NodeAt(ra, IsPrecolored(a) ? b : a)->fixed_adj[key / 64];
  }
  size_t bit = MatrixBit(a, b);
  *mask = UINT64_C(1) << (bit % 64);
  return &ra->matrix[bit / 64];
}

static bool Interferes(RA* ra, int a, int b) {
  if (a == b || (IsPrecolored(a) && IsPrecolored(b))) {
    return false;
  }
  if (ra->matrix == NULL && !IsPrecolored(a) && !IsPrecolored(b)) {
    return HashSetContains(&ra->edges, EdgeKey(a, b));
  }
  uint64_t mask;
  return (*EdgeWord(ra, a, b, &mask) & mask) != 0;
}

// Records the edge between |a| and |b|, which differ and are not both
// physical registers, and returns whether it is new.
static bool InsertEdge(RA* ra, int a, int b) {
  if (ra->matrix == NULL && !IsPrecolored(a) && !IsPrecolored(b)) {
    return HashSetInsert(&ra->edges, EdgeKey(a, b));
  }
  uint64_t mask;
  uint64_t* word = EdgeWord(ra, a, b, &mask);
  if ((*word & mask) != 0) {
    return false;
  }
  *word |= mask;
  return true;
}

static TargetBasicBlock* BlockAt(TargetGenerator* gen, size_t b) {
  TargetBasicBlock* block = gen->basic_blocks.value.p[b];
  if (block == NULL || block->code == NULL || block->end_code == NULL) {
    return NULL;
  }
  return block;
}

static TargetInstruction* NextInBlock(TargetBasicBlock* block,
                                      TargetInstruction* inst) {
  return inst == block->end_code ? NULL : TargetNext(inst);
}

static int FixedKey(RA* ra, TargetInstruction* inst) {
  int key = ra->info->location_key(inst);
  return key < TARGET_RA_FIXED_KEYS ? key : -1;
}

static int TrackedKey(RA* ra, int key) {
  return key >= 0 && key < TARGET_RA_FIXED_KEYS && ra->key_class[key] >= 0
             ? key
             : kNoNode;
}

static bool IsLocation(RA* ra, TargetInstruction* inst) {
  return inst != NULL && ra->info->kind(inst) == kTargetRALocation;
}

static bool IsSpilledLocation(RA* ra, TargetInstruction* inst) {
  const Slot* slot = FindSlot(ra, inst);
  return slot != NULL && slot->spilled_location;
}

// A location still held in a register.
static bool IsRegisterLocation(RA* ra, TargetInstruction* inst) {
  return IsLocation(ra, inst) &&
         (FixedKey(ra, inst) >= 0 || !IsSpilledLocation(ra, inst));
}

static bool WritesRegister(RA* ra, TargetInstruction* inst) {
  return ra->info->kind(inst) == kTargetRAValue &&
         ra->desc->defines_register(inst);
}

static int NewNode(RA* ra, TargetInstruction* inst, int reg_class) {
  int n = NumNodes(ra);
  Node* node = ChunkAdd(&ra->nodes, sizeof(Node));
  node->inst = inst;
  node->reg_class = reg_class;
  node->alias = n;
  if (IsPrecolored(n)) {
    node->state = kNodePrecolored;
    node->color = n;
    node->degree = 1 << 28;
  } else {
    node->state = kNodeInitial;
    node->color = -1;
  }
  return n;
}

static void FreeNodes(RA* ra) {
  for (int n = 0; n < NumNodes(ra); n++) {
    VectorDestruct(&NodeAt(ra, n)->adj);
    VectorDestruct(&NodeAt(ra, n)->moves);
  }
  ChunkFree(&ra->nodes);
}

static int LocationNode(RA* ra, TargetInstruction* location) {
  int key = FixedKey(ra, location);
  if (key >= 0) {
    return TrackedKey(ra, key);
  }
  if (IsSpilledLocation(ra, location)) {
    return kNoNode;
  }
  int n = GetSlot(ra, location)->node;
  if (n < 0) {
    n = NewNode(ra, location, ra->desc->reg_class(location));
    GetSlot(ra, location)->node = n;
  }
  return n;
}

static int ValueNode(RA* ra, TargetInstruction* value) {
  int key = ra->desc->fixed_value_key(value);
  if (key >= 0) {
    return TrackedKey(ra, key);
  }
  const Slot* slot = FindSlot(ra, value);
  return slot == NULL ? kNoNode : slot->node;
}

static int OperandNode(RA* ra, TargetInstruction* op) {
  if (op == NULL) {
    return kNoNode;
  }
  switch (ra->info->kind(op)) {
    case kTargetRAValue:
      return ValueNode(ra, op);
    case kTargetRALocation:
      return LocationNode(ra, op);
    default:
      return kNoNode;
  }
}

static bool ReadsThrough(RA* ra, TargetInstruction* op, int depth) {
  return depth < kMaxReadDepth && ra->info->kind(op) == kTargetRANone &&
         ra->info->reads_through(op);
}

static void CollectInstructions(RA* ra) {
  TargetGenerator* gen = ra->gen;
  ra->insts.length = 0;
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    TargetBasicBlock* block = BlockAt(gen, b);
    for (TargetInstruction* inst = block == NULL ? NULL : block->code;
         inst != NULL; inst = NextInBlock(block, inst)) {
      GetSlot(ra, inst)->position = NumInsts(ra);
      InstInfo* info = ChunkAdd(&ra->insts, sizeof(InstInfo));
      info->inst = inst;
      info->block = (int)b;
      info->group[0] = info->group[1] = info->group[2] = kNoNode;
      info->converted = kNoNode;
    }
  }
}

// Ends the use of RA::insts, whose storage is kept for the next round.
static void FreeInstructions(RA* ra) {
  for (int i = 0; i < NumInsts(ra); i++) {
    GetSlot(ra, InstOf(ra, i))->position = -1;
  }
  ra->insts.length = 0;
}

static void CountReads(RA* ra, TargetInstruction* op, int depth) {
  if (op == NULL) {
    return;
  }
  if (ra->info->kind(op) == kTargetRAValue) {
    GetSlot(ra, op)->reads++;
    return;
  }
  if (ReadsThrough(ra, op, depth)) {
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      CountReads(ra, op->operand[i], depth + 1);
    }
  }
}

static void CountAllReads(RA* ra) {
  for (int s = 0; s < ra->slots.length; s++) {
    SlotAt(ra, s)->reads = 0;
  }
  for (int i = 0; i < NumInsts(ra); i++) {
    for (int op = 0; op < TARGET_MAX_OPERANDS; op++) {
      CountReads(ra, InstOf(ra, i)->operand[op], 0);
    }
  }
}

static bool IsRead(RA* ra, TargetInstruction* value) {
  const Slot* slot = FindSlot(ra, value);
  return slot != NULL && slot->reads > 0;
}

// The class of the registers that |location| can be given, or -1 if none.
static int LocationClass(RA* ra, TargetInstruction* location) {
  int key = FixedKey(ra, location);
  return key >= 0 ? ra->key_class[key] : ra->desc->reg_class(location);
}

// Whether a value is read in its own right, or belongs in a different class
// from its dest, and so gets its own node, rather than its result going
// straight into its dest.
static bool HasOwnNode(RA* ra, TargetInstruction* inst) {
  if (!WritesRegister(ra, inst) || ra->desc->fixed_value_key(inst) >= 0) {
    return false;
  }
  if (IsRead(ra, inst) || !IsRegisterLocation(ra, inst->dest)) {
    return true;
  }
  int dest_class = LocationClass(ra, inst->dest);
  return dest_class >= 0 && dest_class != ra->desc->reg_class(inst);
}

static bool IsTransparentCopy(RA* ra, TargetInstruction* inst) {
  const Slot* slot = FindSlot(ra, inst);
  return slot != NULL && slot->transparent;
}

static bool IsSplitCopy(RA* ra, TargetInstruction* inst) {
  const Slot* slot = FindSlot(ra, inst);
  return slot != NULL && slot->split_copy;
}

static TargetRACopyKind CopyKind(RA* ra, TargetInstruction* inst) {
  if (IsTransparentCopy(ra, inst) || IsSplitCopy(ra, inst)) {
    return kTargetRACopy;
  }
  return ra->info->copy_kind(inst);
}

// ---------------------------------------------------------------------------
// The IR the allocator accepts.

static bool ReadsOrphan(RA* ra, TargetInstruction* op, int depth) {
  if (op == NULL) {
    return false;
  }
  if (ra->info->kind(op) == kTargetRAValue) {
    return WritesRegister(ra, op) && Position(ra, op) < 0;
  }
  if (ReadsThrough(ra, op, depth)) {
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      if (ReadsOrphan(ra, op->operand[i], depth + 1)) {
        return true;
      }
    }
  }
  return false;
}

static const char* Unsupported(RA* ra) {
  for (int i = 0; i < NumInsts(ra); i++) {
    TargetInstruction* inst = InstOf(ra, i);
    if (ra->info->written_operand(inst) >= 0) {
      return "written-operand";
    }
    if (inst->dest != NULL) {
      if (!IsLocation(ra, inst->dest)) {
        return "dest-not-location";
      }
      if (!WritesRegister(ra, inst)) {
        return "dest-without-result";
      }
    }
    for (int op = 0; op < TARGET_MAX_OPERANDS; op++) {
      if (ReadsOrphan(ra, inst->operand[op], 0)) {
        return "orphaned-read";
      }
    }
  }
  return NULL;
}

// ---------------------------------------------------------------------------
// Values in fixed registers.
//
// A call's result arrives in a fixed register that the next call clobbers, so
// it cannot be colored like other values.  Every read of it goes through a
// copy placed right after it, which coalescing removes when it can.
//
// A value that clobbers its operands early writes its dest through a copy
// too, since the dest may be one of the operands.

static bool EarlyClobber(RA* ra, TargetInstruction* inst) {
  return ra->desc->early_clobber != NULL && ra->desc->early_clobber(inst);
}

static void CopyFixedValues(RA* ra) {
  bool any = false;
  for (int i = 0; i < NumInsts(ra); i++) {
    TargetInstruction* inst = InstOf(ra, i);
    if (WritesRegister(ra, inst) && inst->dest != NULL &&
        EarlyClobber(ra, inst)) {
      TargetInstruction* copy =
          ra->desc->create_copy(ra->desc->ctx, inst, inst->dest);
      inst->dest = NULL;
      TargetBasicBlockEmitAfter(ra->gen, InstBlock(ra, i), copy, inst);
      GetSlot(ra, copy)->transparent = true;
      ra->changed_ir = true;
      continue;
    }
    if (!WritesRegister(ra, inst) || ra->desc->fixed_value_key(inst) < 0 ||
        (!IsRead(ra, inst) && inst->dest == NULL)) {
      continue;
    }
    TargetInstruction* dest = inst->dest;
    TargetInstruction* copy = ra->desc->create_copy(ra->desc->ctx, inst, dest);
    inst->dest = NULL;
    GetSlot(ra, inst)->fixed_copy = copy;
    any = true;
    ra->changed_ir = true;
    TargetBasicBlockEmitAfter(ra->gen, InstBlock(ra, i), copy, inst);
    GetSlot(ra, copy)->transparent = true;
  }
  for (int u = 0; any && u < NumInsts(ra); u++) {
    TargetInstruction* user = InstOf(ra, u);
    for (int op = 0; op < TARGET_MAX_OPERANDS; op++) {
      const Slot* slot =
          user->operand[op] == NULL ? NULL : FindSlot(ra, user->operand[op]);
      if (slot != NULL && slot->fixed_copy != NULL) {
        TargetReplaceOperand(user, op, slot->fixed_copy);
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Nodes, uses and definitions.

static void AddReads(RA* ra, TargetInstruction* op, int depth) {
  if (op == NULL) {
    return;
  }
  TargetRAKind kind = ra->info->kind(op);
  if (kind == kTargetRAValue || kind == kTargetRALocation) {
    int n = OperandNode(ra, op);
    if (n >= 0) {
      AppendInt(&ra->uses, n);
    }
    return;
  }
  if (ReadsThrough(ra, op, depth)) {
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      AddReads(ra, op->operand[i], depth + 1);
    }
  }
}

static int LoopWeight(TargetBasicBlock* block) {
  int depth = block->loop_nesting;
  if (depth > 6) {
    depth = 6;
  }
  return 1 << (3 * depth);
}

static Move* AddMove(RA* ra, int a, int b, int weight) {
  if (a < 0 || b < 0 || a == b || (IsPrecolored(a) && IsPrecolored(b)) ||
      NodeAt(ra, a)->reg_class != NodeAt(ra, b)->reg_class) {
    return NULL;
  }
  Move* m = ChunkAdd(&ra->moves, sizeof(Move));
  m->a = a;
  m->b = b;
  m->weight = weight;
  m->state = kMoveWorklist;
  return m;
}

static void BuildNodes(RA* ra) {
  for (int k = 0; k < TARGET_RA_FIXED_KEYS; k++) {
    NewNode(ra, NULL, ra->key_class[k]);
  }
  CountAllReads(ra);
  for (int s = 0; s < ra->slots.length; s++) {
    SlotAt(ra, s)->node = kNoNode;
  }
  for (int i = 0; i < NumInsts(ra); i++) {
    TargetInstruction* inst = InstOf(ra, i);
    if (HasOwnNode(ra, inst)) {
      int n = NewNode(ra, inst, ra->desc->reg_class(inst));
      GetSlot(ra, inst)->node = n;
    }
  }

  int keys[kMaxKeys];
  for (int i = 0; i < NumInsts(ra); i++) {
    InstInfo* info = InstAt(ra, i);
    TargetInstruction* inst = info->inst;
    int weight = LoopWeight(InstBlock(ra, i));

    info->first_use = (int)ra->uses.length;
    for (int op = 0; op < TARGET_MAX_OPERANDS; op++) {
      AddReads(ra, inst->operand[op], 0);
    }
    int num_keys = ra->desc->implicit_uses(ra->desc->ctx, inst, keys, kMaxKeys);
    for (int k = 0; k < num_keys; k++) {
      int n = TrackedKey(ra, keys[k]);
      if (n >= 0) {
        AppendInt(&ra->uses, n);
      }
    }
    info->end_use = (int)ra->uses.length;

    info->first_def = (int)ra->defs.length;
    num_keys = ra->info->clobbers(inst, keys, kMaxKeys);
    for (int k = 0; k < num_keys; k++) {
      int n = TrackedKey(ra, keys[k]);
      if (n >= 0) {
        AppendInt(&ra->defs, n);
      }
    }
    if (WritesRegister(ra, inst)) {
      int own = ValueNode(ra, inst);
      int dest = IsRegisterLocation(ra, inst->dest)
                     ? LocationNode(ra, inst->dest)
                     : kNoNode;
      if (own >= 0) {
        AppendInt(&ra->defs, own);
      }
      if (dest >= 0) {
        AppendInt(&ra->defs, dest);
      }
      info->group[1] = own;
      info->group[2] = dest;
      TargetRACopyKind copy = CopyKind(ra, inst);
      if (copy != kTargetRANotCopy) {
        int src = OperandNode(ra, inst->operand[0]);
        if (copy == kTargetRACopy) {
          info->group[0] = src;
          Move* m = AddMove(ra, src, own >= 0 ? own : dest, weight);
          // Coalescing the two sides of a split would undo it.  Once one side
          // is a reload, or a value stored as soon as it is made, the copy
          // joins a short range instead and may go.
          if (m != NULL && own < 0 && IsSplitCopy(ra, inst) &&
              !IsUnspillable(ra, inst->operand[0])) {
            m->state = kMoveFrozen;
          }
        } else {
          info->converted = src;
        }
      }
      AddMove(ra, own, dest, weight);
      info->early_clobber = EarlyClobber(ra, inst);
    }
    info->end_def = (int)ra->defs.length;
  }
}

// ---------------------------------------------------------------------------
// Liveness.

static void SetBit(uint64_t* bits, int i) {
  bits[i / 64] |= UINT64_C(1) << (i % 64);
}

static void ClearBit(uint64_t* bits, int i) {
  bits[i / 64] &= ~(UINT64_C(1) << (i % 64));
}

// The unwinder enters a landing pad from a call in a protected region and
// skips what follows the call on the normal path.  So whatever the pad reads
// is live on both sides of each call in a block that can reach it, even where
// the call's result or a copy after it writes the same location.

static void MarkLandingPads(RA* ra) {
  TargetGenerator* gen = ra->gen;
  size_t num_blocks = gen->basic_blocks.length;
  ra->pads = calloc(num_blocks + 1, sizeof(bool));
  for (size_t e = 0; e < gen->exception_edges.length; e++) {
    const TargetExceptionEdge* edge = gen->exception_edges.value.p[e];
    TargetBasicBlock* pad =
        edge->catch_label == NULL ? NULL : edge->catch_label->block;
    if (pad != NULL && pad->block_id < num_blocks) {
      ra->pads[pad->block_id] = true;
    }
  }
}

// Sets |bits| to the nodes live into the landing pads |block| reaches, and
// returns whether there are any.
static bool PadLive(RA* ra, TargetBasicBlock* block, uint64_t* bits) {
  size_t words = ra->words;
  size_t num_blocks = ra->gen->basic_blocks.length;
  memset(bits, 0, sizeof(uint64_t) * words);
  bool any = false;
  for (size_t s = 0; s < block->out_edges.length; s++) {
    size_t succ = (size_t)block->out_edges.value.w[s];
    if (succ < num_blocks && ra->pads[succ]) {
      const uint64_t* in = ra->live_in + succ * words;
      for (size_t w = 0; w < words; w++) {
        bits[w] |= in[w];
      }
      any = true;
    }
  }
  return any;
}

static void ComputeLiveness(RA* ra) {
  TargetGenerator* gen = ra->gen;
  size_t num_blocks = gen->basic_blocks.length;
  size_t words = ((size_t)NumNodes(ra) + 63) / 64;
  ra->words = words;
  uint64_t* gen_bits = calloc(num_blocks * words + 1, sizeof(uint64_t));
  uint64_t* kill_bits = calloc(num_blocks * words + 1, sizeof(uint64_t));
  // Nodes each block writes before its first call.
  uint64_t* throw_kill_bits = calloc(num_blocks * words + 1, sizeof(uint64_t));
  bool* calls = calloc(num_blocks + 1, sizeof(bool));
  uint64_t* pad_live = calloc(words + 1, sizeof(uint64_t));
  uint64_t* live_in = calloc(num_blocks * words + 1, sizeof(uint64_t));
  ra->live_in = live_in;
  ra->live_out = calloc(num_blocks * words + 1, sizeof(uint64_t));
  MarkLandingPads(ra);

  for (int i = NumInsts(ra) - 1; i >= 0; i--) {
    const InstInfo* info = InstAt(ra, i);
    uint64_t* g = gen_bits + (size_t)info->block * words;
    uint64_t* k = kill_bits + (size_t)info->block * words;
    uint64_t* tk = throw_kill_bits + (size_t)info->block * words;
    if (gen->virtuals->is_call(info->inst)) {
      calls[info->block] = true;
      memset(tk, 0, sizeof(uint64_t) * words);
    }
    for (int d = info->first_def; d < info->end_def; d++) {
      int n = IntAt(&ra->defs, (size_t)d);
      SetBit(k, n);
      SetBit(tk, n);
      ClearBit(g, n);
    }
    for (int u = info->first_use; u < info->end_use; u++) {
      SetBit(g, IntAt(&ra->uses, (size_t)u));
    }
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (size_t bb = num_blocks; bb-- > 0;) {
      TargetBasicBlock* block = gen->basic_blocks.value.p[bb];
      if (block == NULL) {
        continue;
      }
      uint64_t* out = ra->live_out + bb * words;
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
      uint64_t* in = live_in + bb * words;
      const uint64_t* g = gen_bits + bb * words;
      const uint64_t* k = kill_bits + bb * words;
      const uint64_t* tk = throw_kill_bits + bb * words;
      bool throws = calls[bb] && PadLive(ra, block, pad_live);
      for (size_t w = 0; w < words; w++) {
        uint64_t next = g[w] | (out[w] & ~k[w]);
        if (throws) {
          next |= pad_live[w] & ~tk[w];
        }
        if (next != in[w]) {
          in[w] = next;
          changed = true;
        }
      }
    }
  }
  free(gen_bits);
  free(kill_bits);
  free(throw_kill_bits);
  free(calls);
  free(pad_live);
}

// ---------------------------------------------------------------------------
// Interference.

static int K(RA* ra, int n) { return ra->k[NodeAt(ra, n)->reg_class]; }

static void AddEdge(RA* ra, int a, int b) {
  Node* na = NodeAt(ra, a);
  Node* nb = NodeAt(ra, b);
  if (a == b || na->reg_class < 0 || na->reg_class != nb->reg_class) {
    return;
  }
  bool pa = IsPrecolored(a);
  bool pb = IsPrecolored(b);
  if (pa && pb) {
    return;
  }
  if (!InsertEdge(ra, a, b)) {
    return;
  }
  if (!pa) {
    if (na->adj.capacity == 0) {
      VectorReserve(&na->adj, kFirstNeighbors);
    }
    AppendInt(&na->adj, b);
    na->degree++;
  }
  if (!pb) {
    if (nb->adj.capacity == 0) {
      VectorReserve(&nb->adj, kFirstNeighbors);
    }
    AppendInt(&nb->adj, a);
    nb->degree++;
  }
}

static bool InGroup(const InstInfo* info, int n) {
  return n >= 0 && (info->group[0] == n || info->group[1] == n ||
                    info->group[2] == n);
}

// Adds the edges between the nodes in |live| and the run of physical
// registers that |info| defines from |info|'s def |d|, and returns the end of
// the run.  An edge to a register changes only the other node, so walking
// |live| once for the whole run gives each node the edges, in the order, that
// walking it once for each register would.  A run longer than the buffers
// ends early, and the caller starts another.
static int AddRegisterDefEdges(RA* ra, const InstInfo* info,
                               const uint64_t* live, int d) {
  // A run is at most the clobbers plus the result.
  enum { kMaxRun = kMaxKeys + 2 };
  int keys[TARGET_RA_MAX_CLASSES][kMaxRun];
  bool grouped[TARGET_RA_MAX_CLASSES][kMaxRun];
  int num_keys[TARGET_RA_MAX_CLASSES] = {0};
  int end = d;
  for (; end < info->end_def && IsPrecolored(IntAt(&ra->defs, (size_t)end));
       end++) {
    int key = IntAt(&ra->defs, (size_t)end);
    int c = NodeAt(ra, key)->reg_class;
    if (c < 0) {
      continue;
    }
    if (num_keys[c] == kMaxRun) {
      break;
    }
    keys[c][num_keys[c]] = key;
    grouped[c][num_keys[c]] = InGroup(info, key);
    num_keys[c]++;
  }
  for (size_t w = TARGET_RA_FIXED_KEYS / 64; w < ra->words; w++) {
    uint64_t bits = live[w];
    while (bits != 0) {
      int l = (int)(w * 64) + __builtin_ctzll(bits);
      bits &= bits - 1;
      int c = NodeAt(ra, l)->reg_class;
      if (c < 0) {
        continue;
      }
      bool l_grouped = InGroup(info, l);
      for (int k = 0; k < num_keys[c]; k++) {
        if (!(grouped[c][k] && l_grouped)) {
          AddEdge(ra, keys[c][k], l);
        }
      }
    }
  }
  return end;
}

static bool Names(RA* ra, const InstInfo* info, int n) {
  for (int u = info->first_use; u < info->end_use; u++) {
    if (IntAt(&ra->uses, (size_t)u) == n) {
      return true;
    }
  }
  for (int d = info->first_def; d < info->end_def; d++) {
    if (IntAt(&ra->defs, (size_t)d) == n) {
      return true;
    }
  }
  return false;
}

// Counts |info| toward the relief of the values in |live|, the set after
// |info|, that are neither named by |info| nor in |held|, in the classes it
// leaves with more values than registers.
static void CountRelief(RA* ra, const InstInfo* info, const uint64_t* live,
                        const uint64_t* held) {
  int pressure[TARGET_RA_MAX_CLASSES] = {0};
  for (size_t w = 0; w < ra->words; w++) {
    for (uint64_t bits = live[w]; bits != 0; bits &= bits - 1) {
      int c = NodeAt(ra, (int)(w * 64) + __builtin_ctzll(bits))->reg_class;
      if (c >= 0) {
        pressure[c]++;
      }
    }
  }
  for (int d = info->first_def; d < info->end_def; d++) {
    int def = IntAt(&ra->defs, (size_t)d);
    int c = NodeAt(ra, def)->reg_class;
    if (c >= 0 && (live[def / 64] & (UINT64_C(1) << (def % 64))) == 0) {
      pressure[c]++;
    }
  }
  bool short_of_registers = false;
  for (int c = 0; c < ra->desc->num_classes; c++) {
    short_of_registers |= pressure[c] > ra->desc->classes[c].num_regs;
  }
  if (!short_of_registers) {
    return;
  }
  for (size_t w = TARGET_RA_FIXED_KEYS / 64; w < ra->words; w++) {
    for (uint64_t bits = live[w] & ~held[w]; bits != 0; bits &= bits - 1) {
      int l = (int)(w * 64) + __builtin_ctzll(bits);
      Node* node = NodeAt(ra, l);
      if (node->reg_class >= 0 &&
          pressure[node->reg_class] >
              ra->desc->classes[node->reg_class].num_regs &&
          !Names(ra, info, l)) {
        node->relief++;
      }
    }
  }
}

// Records in |held| which values |info| names that held_since_previous says
// are kept in a register back to the previous instruction naming them.
static void UpdateHeld(RA* ra, const InstInfo* info, uint64_t* held) {
  const TargetRegAllocDesc* desc = ra->desc;
  for (int d = info->first_def; d < info->end_def; d++) {
    int n = IntAt(&ra->defs, (size_t)d);
    if (!IsPrecolored(n)) {
      ClearBit(held, n);
    }
  }
  for (int u = info->first_use; u < info->end_use; u++) {
    int n = IntAt(&ra->uses, (size_t)u);
    if (!IsPrecolored(n)) {
      if (desc->held_since_previous(desc->ctx, info->inst,
                                    NodeAt(ra, n)->inst)) {
        SetBit(held, n);
      } else {
        ClearBit(held, n);
      }
    }
  }
}

static void BuildGraph(RA* ra) {
  TargetGenerator* gen = ra->gen;
  size_t num_blocks = gen->basic_blocks.length;
  size_t n = (size_t)NumNodes(ra) - TARGET_RA_FIXED_KEYS;
  if (n <= kMaxMatrixNodes) {
    ra->matrix = calloc(n * (n + 1) / 2 / 64 + 1, sizeof(uint64_t));
  } else {
    HashSetInit(&ra->edges, n * 4);
  }

  size_t words = ra->words;
  uint64_t* live = malloc(sizeof(uint64_t) * (words + 1));
  uint64_t* pad_live = malloc(sizeof(uint64_t) * (words + 1));
  // The values that the instructions after this one, in its block, keep in a
  // register back to here even when spilled.
  uint64_t* held = calloc(words + 1, sizeof(uint64_t));
  int i = NumInsts(ra) - 1;
  for (size_t bb = num_blocks; bb-- > 0;) {
    if (i < 0 || InstAt(ra, i)->block != (int)bb) {
      continue;
    }
    memcpy(live, ra->live_out + bb * words, sizeof(uint64_t) * words);
    memset(held, 0, sizeof(uint64_t) * words);
    bool reaches_pad = PadLive(ra, InstBlock(ra, i), pad_live);
    for (; i >= 0 && InstAt(ra, i)->block == (int)bb; i--) {
      const InstInfo* info = InstAt(ra, i);
      bool throws = reaches_pad && gen->virtuals->is_call(info->inst);
      for (size_t w = 0; throws && w < words; w++) {
        live[w] |= pad_live[w];
      }
      CountRelief(ra, info, live, held);
      if (ra->desc->held_since_previous != NULL) {
        UpdateHeld(ra, info, held);
      }
      for (int d = info->first_def; d < info->end_def;) {
        int def = IntAt(&ra->defs, (size_t)d);
        if (IsPrecolored(def)) {
          d = AddRegisterDefEdges(ra, info, live, d);
          continue;
        }
        bool grouped = InGroup(info, def);
        for (size_t w = 0; w < words; w++) {
          uint64_t bits = live[w];
          while (bits != 0) {
            int l = (int)(w * 64) + __builtin_ctzll(bits);
            bits &= bits - 1;
            if (!(grouped && InGroup(info, l))) {
              AddEdge(ra, def, l);
            }
          }
        }
        d++;
      }
      for (int d = info->first_def; info->converted >= 0 && d < info->end_def;
           d++) {
        AddEdge(ra, IntAt(&ra->defs, (size_t)d), info->converted);
      }
      for (int d = info->first_def; info->early_clobber && d < info->end_def;
           d++) {
        for (int u = info->first_use; u < info->end_use; u++) {
          AddEdge(ra, IntAt(&ra->defs, (size_t)d), IntAt(&ra->uses, (size_t)u));
        }
      }
      for (int d = info->first_def; d < info->end_def; d++) {
        ClearBit(live, IntAt(&ra->defs, (size_t)d));
      }
      for (size_t w = 0; throws && w < words; w++) {
        live[w] |= pad_live[w];
      }
      for (int u = info->first_use; u < info->end_use; u++) {
        SetBit(live, IntAt(&ra->uses, (size_t)u));
      }
    }
  }
  free(live);
  free(pad_live);
  free(held);

  int keys[kMaxKeys];
  for (int v = TARGET_RA_FIXED_KEYS; v < NumNodes(ra); v++) {
    int num_keys =
        ra->desc->forbidden_keys(NodeAt(ra, v)->inst, keys, kMaxKeys);
    for (int k = 0; k < num_keys; k++) {
      int key = TrackedKey(ra, keys[k]);
      if (key >= 0) {
        AddEdge(ra, v, key);
      }
    }
  }

  for (int idx = 0; idx < NumInsts(ra); idx++) {
    const InstInfo* info = InstAt(ra, idx);
    double weight = LoopWeight(InstBlock(ra, idx));
    for (int u = info->first_use; u < info->end_use; u++) {
      NodeAt(ra, IntAt(&ra->uses, (size_t)u))->cost += weight;
    }
    for (int d = info->first_def; d < info->end_def; d++) {
      NodeAt(ra, IntAt(&ra->defs, (size_t)d))->cost += weight;
    }
  }
  for (int v = TARGET_RA_FIXED_KEYS; v < NumNodes(ra); v++) {
    Node* node = NodeAt(ra, v);
    if (!CanSpill(ra, node->inst)) {
      node->cost = kUnspillableCost;
    }
  }

  for (int m = 0; m < ra->moves.length; m++) {
    AppendInt(&NodeAt(ra, MoveAt(ra, m)->a)->moves, m);
    AppendInt(&NodeAt(ra, MoveAt(ra, m)->b)->moves, m);
  }
}

// ---------------------------------------------------------------------------
// Iterated register coalescing.

static int Alias(RA* ra, int n) {
  while (NodeAt(ra, n)->state == kNodeCoalesced) {
    n = NodeAt(ra, n)->alias;
  }
  return n;
}

static bool IsAdjacent(RA* ra, int t) {
  NodeState state = NodeAt(ra, t)->state;
  return state != kNodeSelected && state != kNodeCoalesced;
}

static bool MoveIsLive(const Move* m) {
  return m->state == kMoveWorklist || m->state == kMoveActive;
}

static bool MoveRelated(RA* ra, int n) {
  const Vector* moves = &NodeAt(ra, n)->moves;
  for (size_t i = 0; i < moves->length; i++) {
    if (MoveIsLive(MoveAt(ra, IntAt(moves, i)))) {
      return true;
    }
  }
  return false;
}

static void SetState(RA* ra, int n, NodeState state) {
  NodeAt(ra, n)->state = state;
  switch (state) {
    case kNodeSimplify:
      AppendInt(&ra->simplify_list, n);
      break;
    case kNodeFreeze:
      AppendInt(&ra->freeze_list, n);
      break;
    case kNodeSpill:
      AppendInt(&ra->spill_list, n);
      break;
    default:
      break;
  }
}

// Lists hold stale entries for nodes that have moved on; skip them.
static int PopState(RA* ra, Vector* list, NodeState state) {
  while (list->length > 0) {
    int n = PopInt(list);
    if (NodeAt(ra, n)->state == state) {
      return n;
    }
  }
  return kNoNode;
}

static void MakeWorklist(RA* ra) {
  for (int n = TARGET_RA_FIXED_KEYS; n < NumNodes(ra); n++) {
    if (NodeAt(ra, n)->degree >= K(ra, n)) {
      SetState(ra, n, kNodeSpill);
    } else if (MoveRelated(ra, n)) {
      SetState(ra, n, kNodeFreeze);
    } else {
      SetState(ra, n, kNodeSimplify);
    }
  }
  // The worklist pops from the end, so push the most frequently executed
  // moves last to coalesce them first.
  for (int w = 1; w <= 1 << 18; w <<= 3) {
    for (int m = ra->moves.length; m-- > 0;) {
      int weight = MoveAt(ra, m)->weight;
      if (weight >= w && weight < (w << 3)) {
        AppendInt(&ra->move_worklist, m);
      }
    }
  }
}

static void EnableMoves(RA* ra, int n) {
  const Vector* moves = &NodeAt(ra, n)->moves;
  for (size_t i = 0; i < moves->length; i++) {
    Move* m = MoveAt(ra, IntAt(moves, i));
    if (m->state == kMoveActive) {
      m->state = kMoveWorklist;
      AppendInt(&ra->move_worklist, IntAt(moves, i));
    }
  }
}

static void DecrementDegree(RA* ra, int m) {
  if (IsPrecolored(m)) {
    return;
  }
  Node* node = NodeAt(ra, m);
  int d = node->degree--;
  if (d != K(ra, m) || node->state != kNodeSpill) {
    return;
  }
  EnableMoves(ra, m);
  for (size_t i = 0; i < node->adj.length; i++) {
    int t = IntAt(&node->adj, i);
    if (IsAdjacent(ra, t)) {
      EnableMoves(ra, t);
    }
  }
  SetState(ra, m, MoveRelated(ra, m) ? kNodeFreeze : kNodeSimplify);
}

static void Simplify(RA* ra, int n) {
  Node* node = NodeAt(ra, n);
  node->state = kNodeSelected;
  AppendInt(&ra->select_stack, n);
  for (size_t i = 0; i < node->adj.length; i++) {
    int t = IntAt(&node->adj, i);
    if (!IsPrecolored(t) && IsAdjacent(ra, t)) {
      DecrementDegree(ra, t);
    }
  }
}

static void AddWorkList(RA* ra, int u) {
  if (!IsPrecolored(u) && NodeAt(ra, u)->state == kNodeFreeze &&
      !MoveRelated(ra, u) && NodeAt(ra, u)->degree < K(ra, u)) {
    SetState(ra, u, kNodeSimplify);
  }
}

static bool AdjacentOK(RA* ra, int t, int r) {
  return NodeAt(ra, t)->degree < K(ra, t) || IsPrecolored(t) ||
         Interferes(ra, t, r);
}

// George's test for merging |v| into the precolored |u|.
static bool AllAdjacentOK(RA* ra, int v, int u) {
  const Vector* adj = &NodeAt(ra, v)->adj;
  for (size_t i = 0; i < adj->length; i++) {
    int t = IntAt(adj, i);
    if (!IsPrecolored(t) && IsAdjacent(ra, t) && !AdjacentOK(ra, t, u)) {
      return false;
    }
  }
  return true;
}

// Briggs's test: the merged node has fewer than K significant neighbors.
// Precolored neighbors are always significant, so they are counted from the
// masks.
static bool Conservative(RA* ra, int u, int v) {
  int gen = ++ra->mark_gen;
  int k = K(ra, u);
  int significant = 0;
  int nodes[2] = {u, v};
  const Node* nu = NodeAt(ra, u);
  const Node* nv = NodeAt(ra, v);
  for (int w = 0; w < TARGET_RA_FIXED_KEYS / 64; w++) {
    significant += __builtin_popcountll(nu->fixed_adj[w] | nv->fixed_adj[w]);
  }
  if (significant >= k) {
    return false;
  }
  for (int j = 0; j < 2; j++) {
    const Vector* adj = &NodeAt(ra, nodes[j])->adj;
    for (size_t i = 0; i < adj->length; i++) {
      int t = IntAt(adj, i);
      if (IsPrecolored(t)) {
        continue;
      }
      Node* neighbor = NodeAt(ra, t);
      if (!IsAdjacent(ra, t) || neighbor->mark == gen) {
        continue;
      }
      neighbor->mark = gen;
      if (neighbor->degree >= K(ra, t) && ++significant >= k) {
        return false;
      }
    }
  }
  return true;
}

static void Combine(RA* ra, int u, int v) {
  Node* nu = NodeAt(ra, u);
  Node* nv = NodeAt(ra, v);
  nv->state = kNodeCoalesced;
  nv->alias = u;
  VectorAppendVector(&nu->moves, &nv->moves);
  EnableMoves(ra, v);
  // AddEdge grows the lists of v's neighbors and of u, never v's own.
  for (size_t i = 0; i < nv->adj.length; i++) {
    int t = IntAt(&nv->adj, i);
    if (IsAdjacent(ra, t)) {
      AddEdge(ra, t, u);
      DecrementDegree(ra, t);
    }
  }
  if (!IsPrecolored(u)) {
    nu->cost += nv->cost;
    nu->relief += nv->relief;
    if (nu->degree >= K(ra, u) && nu->state == kNodeFreeze) {
      SetState(ra, u, kNodeSpill);
    }
  }
}

static void Coalesce(RA* ra, int index) {
  Move* m = MoveAt(ra, index);
  int x = Alias(ra, m->a);
  int y = Alias(ra, m->b);
  int u = x, v = y;
  if (IsPrecolored(y)) {
    u = y;
    v = x;
  }
  if (u == v) {
    m->state = kMoveCoalesced;
    AddWorkList(ra, u);
  } else if (IsPrecolored(v) || Interferes(ra, u, v)) {
    m->state = kMoveConstrained;
    AddWorkList(ra, u);
    AddWorkList(ra, v);
  } else if (IsPrecolored(u) ? AllAdjacentOK(ra, v, u)
                             : Conservative(ra, u, v)) {
    m->state = kMoveCoalesced;
    ra->stats.coalesced++;
    Combine(ra, u, v);
    AddWorkList(ra, u);
  } else {
    m->state = kMoveActive;
  }
}

static void FreezeMoves(RA* ra, int u) {
  const Vector* moves = &NodeAt(ra, u)->moves;
  for (size_t i = 0; i < moves->length; i++) {
    Move* m = MoveAt(ra, IntAt(moves, i));
    if (!MoveIsLive(m)) {
      continue;
    }
    int x = Alias(ra, m->a);
    int y = Alias(ra, m->b);
    int v = y == Alias(ra, u) ? x : y;
    m->state = kMoveFrozen;
    if (!IsPrecolored(v) && NodeAt(ra, v)->state == kNodeFreeze &&
        !MoveRelated(ra, v) && NodeAt(ra, v)->degree < K(ra, v)) {
      SetState(ra, v, kNodeSimplify);
    }
  }
}

static int PopMove(RA* ra) {
  while (ra->move_worklist.length > 0) {
    int m = PopInt(&ra->move_worklist);
    if (MoveAt(ra, m)->state == kMoveWorklist) {
      return m;
    }
  }
  return -1;
}

// Prefers the nodes whose spill would free a register where their class has
// too few, by how many such places: spilling one that is in a register
// wherever its class runs short only leads to spilling what it leaves.
static int SelectSpill(RA* ra) {
  Vector* list = &ra->spill_list;
  int best = kNoNode;
  bool best_relieves = false;
  double best_cost = 0;
  size_t kept = 0;
  for (size_t i = 0; i < list->length; i++) {
    int n = IntAt(list, i);
    Node* node = NodeAt(ra, n);
    if (node->state != kNodeSpill) {
      continue;
    }
    list->value.w[kept++] = n;
    double cost = node->cost / (double)(node->degree + 1);
    bool relieves = node->relief > 0 && node->cost < kUnspillableCost;
    if (relieves) {
      cost /= node->relief;
    }
    if (best == kNoNode || (relieves && !best_relieves) ||
        (relieves == best_relieves && cost < best_cost)) {
      best = n;
      best_relieves = relieves;
      best_cost = cost;
    }
  }
  list->length = kept;
  return best;
}

static int PickColor(RA* ra, int n, const bool* ok) {
  Node* node = NodeAt(ra, n);
  int best = -1;
  int best_weight = 0;
  for (size_t i = 0; i < node->moves.length; i++) {
    const Move* m = MoveAt(ra, IntAt(&node->moves, i));
    int other = Alias(ra, m->a) == n ? Alias(ra, m->b) : Alias(ra, m->a);
    Node* partner = NodeAt(ra, other);
    int c = partner->color;
    if (c >= 0 && (partner->state == kNodeColored || IsPrecolored(other)) &&
        ok[ra->key_index[c]] && m->weight > best_weight) {
      best = c;
      best_weight = m->weight;
    }
  }
  if (best >= 0) {
    return best;
  }
  const TargetRAClass* rc = &ra->desc->classes[node->reg_class];
  for (int r = 0; r < rc->num_regs; r++) {
    if (ok[r]) {
      return rc->regs[r];
    }
  }
  return -1;
}

static void AssignColors(RA* ra, Vector* spilled) {
  bool ok[TARGET_RA_MAX_CLASS_REGS];
  while (ra->select_stack.length > 0) {
    int n = PopInt(&ra->select_stack);
    Node* node = NodeAt(ra, n);
    const TargetRAClass* rc = &ra->desc->classes[node->reg_class];
    for (int r = 0; r < rc->num_regs; r++) {
      ok[r] = true;
    }
    for (int w = 0; w < TARGET_RA_FIXED_KEYS / 64; w++) {
      for (uint64_t bits = node->fixed_adj[w]; bits != 0; bits &= bits - 1) {
        int index = ra->key_index[w * 64 + __builtin_ctzll(bits)];
        if (index >= 0) {
          ok[index] = false;
        }
      }
    }
    for (size_t i = 0; i < node->adj.length; i++) {
      int t = IntAt(&node->adj, i);
      if (IsPrecolored(t)) {
        continue;
      }
      int a = Alias(ra, t);
      Node* neighbor = NodeAt(ra, a);
      if (neighbor->state == kNodeColored || IsPrecolored(a)) {
        int index = ra->key_index[neighbor->color];
        if (index >= 0) {
          ok[index] = false;
        }
      }
    }
    int c = PickColor(ra, n, ok);
    if (c < 0) {
      node->state = kNodeSpilled;
      AppendInt(spilled, n);
    } else {
      node->state = kNodeColored;
      node->color = c;
    }
  }
  for (int n = TARGET_RA_FIXED_KEYS; n < NumNodes(ra); n++) {
    Node* node = NodeAt(ra, n);
    if (node->state == kNodeCoalesced) {
      Node* rep = NodeAt(ra, Alias(ra, n));
      if (rep->state == kNodeSpilled) {
        AppendInt(spilled, n);
      } else {
        node->color = rep->color;
      }
    }
  }
}

static void Color(RA* ra, Vector* spilled) {
  MakeWorklist(ra);
  for (;;) {
    int n;
    if ((n = PopState(ra, &ra->simplify_list, kNodeSimplify)) >= 0) {
      Simplify(ra, n);
    } else if ((n = PopMove(ra)) >= 0) {
      Coalesce(ra, n);
    } else if ((n = PopState(ra, &ra->freeze_list, kNodeFreeze)) >= 0) {
      SetState(ra, n, kNodeSimplify);
      FreezeMoves(ra, n);
    } else if ((n = SelectSpill(ra)) >= 0) {
      SetState(ra, n, kNodeSimplify);
      FreezeMoves(ra, n);
    } else {
      break;
    }
  }
  AssignColors(ra, spilled);
}

// ---------------------------------------------------------------------------
// Spilling.

typedef struct {
  TargetInstruction* from;
  TargetInstruction* to;
} Reload;

static TargetInstruction* SpillValue(RA* ra, TargetInstruction* value,
                                     int slot) {
  TargetInstruction* spill =
      ra->desc->create_spill(ra->desc->ctx, value, slot);
  TargetBasicBlockEmitAfter(ra->gen, InstBlock(ra, Position(ra, value)), spill,
                            value);
  return spill;
}

// Replaces the reads of spilled values and locations in |holder|, an operand
// tree of |user|, with reloads placed before |user|.
static void ReplaceReads(RA* ra, const TargetRAIndex* handle_index,
                         const Vector* handles, int user,
                         TargetInstruction* holder, Reload* reloads,
                         int* num_reloads, int depth) {
  for (int op = 0; op < TARGET_MAX_OPERANDS; op++) {
    TargetInstruction* value = holder->operand[op];
    if (value == NULL) {
      continue;
    }
    TargetRAKind kind = ra->info->kind(value);
    if (kind == kTargetRANone) {
      if (ReadsThrough(ra, value, depth)) {
        ReplaceReads(ra, handle_index, handles, user, value, reloads,
                     num_reloads, depth + 1);
      }
      continue;
    }
    int h = TargetRAIndexFind(handle_index, value);
    if (h < 0) {
      continue;
    }
    TargetInstruction* reload = NULL;
    for (int r = 0; r < *num_reloads; r++) {
      if (reloads[r].from == value) {
        reload = reloads[r].to;
      }
    }
    if (reload == NULL) {
      reload = ra->desc->create_reload(ra->desc->ctx,
                                       VectorGet(handles, (size_t)h));
      TargetBasicBlockEmitBefore(ra->gen, InstBlock(ra, user), reload,
                                 InstOf(ra, user));
      GetSlot(ra, reload)->unspillable = true;
      if (*num_reloads < 4 * TARGET_MAX_OPERANDS) {
        reloads[(*num_reloads)++] = (Reload){value, reload};
      }
    }
    TargetReplaceOperand(holder, op, reload);
  }
}

static void RewriteSpills(RA* ra, const Vector* spilled) {
  // The spill that names the slot of each spilled value or location.
  TargetRAIndex handle_index;
  TargetRAIndexInit(&handle_index, spilled->length);
  Vector handles;
  VectorInit(&handles);
  for (size_t s = 0; s < spilled->length; s++) {
    Node* node = NodeAt(ra, IntAt(spilled, s));
    TargetInstruction* inst = node->inst;
    int slot = ra->desc->new_slot(ra->desc->ctx, inst);
    TargetInstruction* handle = NULL;
    if (ra->info->kind(inst) == kTargetRAValue) {
      handle = SpillValue(ra, inst, slot);
      GetSlot(ra, inst)->unspillable = true;
    } else {
      GetSlot(ra, inst)->spilled_location = true;
      for (int i = 0; i < NumInsts(ra); i++) {
        TargetInstruction* def = InstOf(ra, i);
        if (def->dest != inst) {
          continue;
        }
        bool had_own_node = ValueNode(ra, def) >= TARGET_RA_FIXED_KEYS;
        TargetInstruction* spill = SpillValue(ra, def, slot);
        if (handle == NULL) {
          handle = spill;
        }
        if (!had_own_node) {
          GetSlot(ra, def)->unspillable = true;
        }
      }
      if (handle == NULL) {
        handle = ra->desc->create_spill(ra->desc->ctx, inst, slot);
        TargetTrackOrphanInstruction(ra->gen, handle);
      }
    }
    TargetRAIndexInsert(&handle_index, inst, (int)handles.length);
    VectorAppend(&handles, handle);
  }

  ra->changed_ir = true;
  Reload reloads[4 * TARGET_MAX_OPERANDS];
  for (int u = 0; u < NumInsts(ra); u++) {
    int num_reloads = 0;
    ReplaceReads(ra, &handle_index, &handles, u, InstOf(ra, u), reloads,
                 &num_reloads, 0);
  }
  ra->stats.spilled += (int)spilled->length;
  TargetRAIndexDestruct(&handle_index);
  VectorDestruct(&handles);
}

// Has the target spill each value it can.  Returns whether that changed
// anything for the next round: a value spilled or found unspillable.
static bool RewriteSpillsByTarget(RA* ra, const Vector* spilled) {
  bool changed = false;
  for (size_t s = 0; s < spilled->length; s++) {
    TargetInstruction* inst = NodeAt(ra, IntAt(spilled, s))->inst;
    if (!CanSpill(ra, inst)) {
      continue;
    }
    changed = true;
    GetSlot(ra, inst)->unspillable = true;
    if (ra->desc->rewrite_spill(ra->desc->ctx, inst)) {
      ra->changed_ir = true;
      ra->stats.spilled++;
    }
  }
  return changed;
}

// ---------------------------------------------------------------------------
// Assignment.

static int NodeColor(RA* ra, int n) { return NodeAt(ra, Alias(ra, n))->color; }

static int LocationColor(RA* ra, TargetInstruction* location) {
  int key = FixedKey(ra, location);
  if (key >= 0) {
    return key;
  }
  const Slot* slot = FindSlot(ra, location);
  int n = slot == NULL ? kNoNode : slot->node;
  if (n < 0) {
    fprintf(stderr, "register allocation of %s: no node for @%d\n",
            ra->gen->function_name.value, location->id);
    abort();
  }
  return NodeColor(ra, n);
}

static void AssignFixed(RA* ra, TargetInstruction* inst, int depth) {
  if (inst == NULL || depth > kMaxReadDepth) {
    return;
  }
  int key = FixedKey(ra, inst);
  if (key >= 0) {
    ra->desc->assign(ra->desc->ctx, inst, key);
  }
  if (ReadsThrough(ra, inst, depth)) {
    for (int op = 0; op < TARGET_MAX_OPERANDS; op++) {
      AssignFixed(ra, inst->operand[op], depth + 1);
    }
  }
}

static void AssignRegisters(RA* ra) {
  void* ctx = ra->desc->ctx;
  for (int n = TARGET_RA_FIXED_KEYS; n < NumNodes(ra); n++) {
    ra->desc->assign(ctx, NodeAt(ra, n)->inst, NodeColor(ra, n));
  }
  for (int i = 0; i < NumInsts(ra); i++) {
    TargetInstruction* inst = InstOf(ra, i);
    AssignFixed(ra, inst, 0);
    AssignFixed(ra, inst->dest, 0);
    for (int op = 0; op < TARGET_MAX_OPERANDS; op++) {
      AssignFixed(ra, inst->operand[op], 0);
    }
    if (!WritesRegister(ra, inst)) {
      continue;
    }
    int own = ra->desc->fixed_value_key(inst);
    if (own >= 0) {
      ra->desc->assign(ctx, inst, own);
    } else {
      int node = ValueNode(ra, inst);
      if (node >= 0) {
        own = NodeColor(ra, node);
      } else {
        ra->desc->assign(ctx, inst, LocationColor(ra, inst->dest));
      }
    }
    if (own < 0 || !IsRegisterLocation(ra, inst->dest)) {
      continue;
    }
    int target = LocationColor(ra, inst->dest);
    if (target == own) {
      continue;
    }
    TargetInstruction* copy = ra->desc->create_copy(ctx, inst, inst->dest);
    TargetBasicBlockEmitAfter(ra->gen, InstBlock(ra, i), copy, inst);
    ra->desc->assign(ctx, copy, target);
    ra->stats.copies++;
  }
}

// ---------------------------------------------------------------------------

static void FreeRound(RA* ra) {
  FreeNodes(ra);
  VectorDestruct(&ra->uses);
  VectorDestruct(&ra->defs);
  free(ra->live_in);
  ra->live_in = NULL;
  free(ra->live_out);
  ra->live_out = NULL;
  free(ra->pads);
  ra->pads = NULL;
  free(ra->matrix);
  ra->matrix = NULL;
  HashSetDestruct(&ra->edges);
  ChunkFree(&ra->moves);
  VectorDestruct(&ra->simplify_list);
  VectorDestruct(&ra->freeze_list);
  VectorDestruct(&ra->spill_list);
  VectorDestruct(&ra->move_worklist);
  VectorDestruct(&ra->select_stack);
  FreeInstructions(ra);
}

// |analyzed| is set when the instructions, nodes and liveness are already
// built.
static void BuildRound(RA* ra, bool analyzed) {
  if (!analyzed) {
    CollectInstructions(ra);
    BuildNodes(ra);
    ComputeLiveness(ra);
  }
  BuildGraph(ra);
}

// ---------------------------------------------------------------------------
// Splitting at the fast path.
//
// The blocks the entry reaches before any call form the fast path, and the
// rest of the function the slow path.  When the fast path can return, a
// target can leave its frame to the slow path, provided the fast path keeps
// out of callee-saved registers and stack slots.  But a value live across a
// call takes a callee-saved register for its whole range, and spilling it
// stores it where it is made.  So each value or location live on an edge
// between the two paths gets a location of its own on each side, with copies
// on the edges between them, and its fast side never crosses a call.  The
// copies bias coloring toward one register for both sides, but are never
// coalesced, which would undo the split.

enum {
  // Blocks the fast path shares with the slow path, such as the epilogue.
  kMaxJoinedSize = 24,
  kMaxJoinedTotal = 48,
};

typedef struct {
  bool fast_home;            // Whether it is made on the fast path.
  TargetInstruction* home;   // Its name on the side it is made on.
  TargetInstruction* other;  // Its name on the other side; NULL if unsplit.
} SplitNames;

typedef enum {
  kPlaceAtStart,  // Every edge into the target crosses the same way.
  kPlaceAtEnd,    // Every edge out of the source crosses the same way.
  kPlaceOnEdge,   // A new block on the edge.
  kPlaceNowhere,
} Placement;

typedef struct {
  TargetBasicBlock* from;
  TargetBasicBlock* to;
  Placement placement;
} CrossingEdge;

static TargetBasicBlock* EdgeBlock(RA* ra, const Vector* edges, size_t i) {
  size_t id = (size_t)edges->value.w[i];
  return id < ra->gen->basic_blocks.length ? BlockAt(ra->gen, id) : NULL;
}

// How many of |block|'s predecessors are on the fast path.  |forward| is set
// when every edge into and out of |block| runs forward in block order.
static size_t FastPreds(RA* ra, TargetBasicBlock* block, const bool* fast,
                        bool* forward) {
  size_t n = 0;
  *forward = true;
  for (size_t i = 0; i < block->in_edges.length; i++) {
    TargetBasicBlock* pred = EdgeBlock(ra, &block->in_edges, i);
    n += pred != NULL && fast[pred->block_id];
    *forward = *forward && pred != NULL && pred->block_id < block->block_id;
  }
  for (size_t i = 0; i < block->out_edges.length; i++) {
    size_t succ = (size_t)block->out_edges.value.w[i];
    *forward = *forward && succ > block->block_id;
  }
  return n;
}

// Marks the fast path in |fast|, by block id: the entry and every call-free
// block whose predecessors all lie on it, then short call-free blocks it
// shares with the slow path.  Returns whether splitting can help, which needs
// a fast block that returns and a slow block that calls.
static bool FindFastPath(RA* ra, bool* fast) {
  TargetGenerator* gen = ra->gen;
  size_t num_blocks = gen->basic_blocks.length;
  bool* calls = calloc(num_blocks, sizeof(bool));
  bool* returns = calloc(num_blocks, sizeof(bool));
  int* sizes = calloc(num_blocks, sizeof(int));
  for (size_t b = 0; b < num_blocks; b++) {
    TargetBasicBlock* block = BlockAt(gen, b);
    for (TargetInstruction* inst = block == NULL ? NULL : block->code;
         inst != NULL; inst = NextInBlock(block, inst)) {
      calls[b] = calls[b] || gen->virtuals->is_call(inst);
      returns[b] = returns[b] || gen->virtuals->is_return(inst);
      sizes[b] += !TargetIsDebugRangeLabel(inst);
    }
  }
  bool useful = false;
  size_t entry = gen->entry_block->block_id;
  if (entry < num_blocks && BlockAt(gen, entry) != NULL && !calls[entry]) {
    fast[entry] = true;
    int joined = 0;
    for (;;) {
      bool grew = false;
      for (size_t b = 0; b < num_blocks; b++) {
        TargetBasicBlock* block = BlockAt(gen, b);
        bool forward;
        if (block != NULL && !fast[b] && !calls[b] &&
            block->in_edges.length > 0 &&
            FastPreds(ra, block, fast, &forward) == block->in_edges.length) {
          fast[b] = true;
          grew = true;
        }
      }
      for (size_t b = 0; b < num_blocks && !grew; b++) {
        TargetBasicBlock* block = BlockAt(gen, b);
        bool forward;
        if (block != NULL && !fast[b] && !calls[b] &&
            sizes[b] <= kMaxJoinedSize &&
            joined + sizes[b] <= kMaxJoinedTotal &&
            FastPreds(ra, block, fast, &forward) > 0 && forward) {
          fast[b] = true;
          joined += sizes[b];
          grew = true;
        }
      }
      if (!grew) {
        break;
      }
    }
    bool fast_returns = false;
    bool slow_calls = false;
    for (size_t b = 0; b < num_blocks; b++) {
      if (BlockAt(gen, b) != NULL) {
        fast_returns = fast_returns || (fast[b] && returns[b]);
        slow_calls = slow_calls || (!fast[b] && calls[b]);
      }
    }
    useful = fast_returns && slow_calls;
  }
  free(calls);
  free(returns);
  free(sizes);
  return useful;
}

static bool AllEdgesOnSide(RA* ra, const Vector* edges, const bool* fast,
                           bool side) {
  for (size_t i = 0; i < edges->length; i++) {
    TargetBasicBlock* block = EdgeBlock(ra, edges, i);
    if (block == NULL || fast[block->block_id] != side) {
      return false;
    }
  }
  return edges->length > 0;
}

// The block control falls into from the end of |block|, or NULL.
static TargetBasicBlock* FallthroughBlock(RA* ra, TargetBasicBlock* block) {
  TargetVirtuals* v = ra->gen->virtuals;
  TargetInstruction* end = block->end_code;
  if (v->is_return(end) || (v->is_branch(end) && !v->is_conditional_branch(end))) {
    return NULL;
  }
  TargetInstruction* next = TargetNext(end);
  return next == NULL ? NULL : next->block;
}

// The operand of the branch ending |from| that names |to|'s label, or -1.
static int BranchOperand(RA* ra, TargetBasicBlock* from, TargetBasicBlock* to) {
  TargetVirtuals* v = ra->gen->virtuals;
  TargetInstruction* end = from->end_code;
  if (!v->is_branch(end) || (end->flags & TARGET_INST_TABLE_JUMP) != 0) {
    return -1;
  }
  TargetInstruction* label = v->get_branch_target(end);
  if (label == NULL || label->block != to) {
    return -1;
  }
  for (int op = 0; op < TARGET_MAX_OPERANDS; op++) {
    if (end->operand[op] == label) {
      return op;
    }
  }
  return -1;
}

static bool EndsWithoutFallthrough(RA* ra, TargetInstruction* inst) {
  TargetVirtuals* v = ra->gen->virtuals;
  if (inst->block == NULL || inst->block->end_code != inst ||
      (inst->flags & TARGET_INST_TABLE_JUMP) != 0) {
    return false;
  }
  return v->is_return(inst) ||
         (v->is_branch(inst) && !v->is_conditional_branch(inst));
}

// Where a block for a branch out of |from| can go: after the nearest block
// that control cannot fall out of, so that the new block runs only when
// branched to and stays within the branch's reach.
static TargetInstruction* PlaceNear(RA* ra, TargetBasicBlock* from) {
  for (TargetInstruction* inst = from->end_code; inst != NULL;
       inst = TargetNext(inst)) {
    if (EndsWithoutFallthrough(ra, inst)) {
      return inst;
    }
  }
  for (TargetInstruction* inst = TargetPrev(from->code); inst != NULL;
       inst = TargetPrev(inst)) {
    if (EndsWithoutFallthrough(ra, inst)) {
      return inst;
    }
  }
  return NULL;
}

static Placement PlaceCopies(RA* ra, const bool* fast, TargetBasicBlock* from,
                             TargetBasicBlock* to) {
  if (AllEdgesOnSide(ra, &to->in_edges, fast, fast[from->block_id])) {
    return kPlaceAtStart;
  }
  if (AllEdgesOnSide(ra, &from->out_edges, fast, fast[to->block_id])) {
    return kPlaceAtEnd;
  }
  bool falls = FallthroughBlock(ra, from) == to;
  bool branches = BranchOperand(ra, from, to) >= 0;
  if (falls != branches && (falls || PlaceNear(ra, from) != NULL)) {
    return kPlaceOnEdge;
  }
  return kPlaceNowhere;
}

static void ReplaceEdge(Vector* edges, size_t from, size_t to) {
  for (size_t i = 0; i < edges->length; i++) {
    if ((size_t)edges->value.w[i] == from) {
      edges->value.w[i] = (intptr_t)to;
      return;
    }
  }
}

// Puts a new block on the edge from |from| to |to|: right after |from| when
// control falls through, otherwise near it, ending in a jump.
static TargetBasicBlock* SplitEdge(RA* ra, TargetBasicBlock* from,
                                   TargetBasicBlock* to) {
  TargetGenerator* gen = ra->gen;
  const TargetRegAllocDesc* desc = ra->desc;
  TargetBasicBlock* edge = NewTargetBasicBlock(gen->basic_blocks.length);
  VectorAppend(&gen->basic_blocks, edge);
  TargetInstruction* label = desc->create_label(desc->ctx);
  label->block = edge;
  edge->code = edge->end_code = label;
  int op = BranchOperand(ra, from, to);
  if (op < 0) {
    TargetEmitAfter(gen, label, from->end_code);
  } else {
    TargetInstruction* branch = from->end_code;
    TargetInstruction* jump = desc->create_jump(desc->ctx, branch->operand[op]);
    TargetEmitAfter(gen, label, PlaceNear(ra, from));
    TargetEmitAfter(gen, jump, label);
    jump->block = edge;
    edge->end_code = jump;
    TargetReplaceOperand(branch, op, label);
  }
  ReplaceEdge(&from->out_edges, to->block_id, edge->block_id);
  ReplaceEdge(&to->in_edges, from->block_id, edge->block_id);
  VectorAppend(&edge->in_edges, (void*)(uintptr_t)from->block_id);
  VectorAppend(&edge->out_edges, (void*)(uintptr_t)to->block_id);
  BitSetCopy(&edge->dominators, &from->dominators);
  BitSetInsert(&edge->dominators, edge->block_id);
  edge->idom = from;
  edge->loop_nesting = from->loop_nesting < to->loop_nesting
                           ? from->loop_nesting
                           : to->loop_nesting;
  return edge;
}

static void EmitAtStart(RA* ra, TargetBasicBlock* block,
                        TargetInstruction* inst) {
  TargetInstruction* pos = block->code;
  while (pos != NULL && ra->gen->virtuals->is_label(pos)) {
    pos = NextInBlock(block, pos);
  }
  if (pos == NULL) {
    TargetBasicBlockEmitAfter(ra->gen, block, inst, block->end_code);
  } else {
    TargetBasicBlockEmitBefore(ra->gen, block, inst, pos);
  }
}

static void EmitAtEnd(RA* ra, TargetBasicBlock* block,
                      TargetInstruction* inst) {
  TargetInstruction* end = block->end_code;
  if (ra->gen->virtuals->is_branch(end)) {
    TargetBasicBlockEmitBefore(ra->gen, block, inst, end);
  } else {
    TargetBasicBlockEmitAfter(ra->gen, block, inst, end);
  }
}

// The name |inst| has on one side of a split, or NULL if it is not split.
static TargetInstruction* SplitName(RA* ra, const SplitNames* names,
                                    int num_nodes, TargetInstruction* inst,
                                    bool fast) {
  TargetRAKind kind = ra->info->kind(inst);
  int n = kind == kTargetRAValue      ? ValueNode(ra, inst)
          : kind == kTargetRALocation ? LocationNode(ra, inst)
                                      : kNoNode;
  if (n < TARGET_RA_FIXED_KEYS || n >= num_nodes || names[n].other == NULL) {
    return NULL;
  }
  return fast == names[n].fast_home ? names[n].home : names[n].other;
}

static void RenameReads(RA* ra, const SplitNames* names, int num_nodes,
                        TargetInstruction* holder, bool fast, int depth) {
  for (int op = 0; op < TARGET_MAX_OPERANDS; op++) {
    TargetInstruction* read = holder->operand[op];
    if (read == NULL) {
      continue;
    }
    if (ra->info->kind(read) == kTargetRANone) {
      if (ReadsThrough(ra, read, depth)) {
        RenameReads(ra, names, num_nodes, read, fast, depth + 1);
      }
      continue;
    }
    TargetInstruction* name = SplitName(ra, names, num_nodes, read, fast);
    if (name != NULL) {
      TargetReplaceOperand(holder, op, name);
    }
  }
}

// Copies every split node in |live| from its name on one side to its name on
// the other, at the start or the end of |block|.
static void EmitSplitCopies(RA* ra, const SplitNames* names, int num_nodes,
                            const uint64_t* live, bool from_fast,
                            TargetBasicBlock* block, bool at_start) {
  const TargetRegAllocDesc* desc = ra->desc;
  for (int n = TARGET_RA_FIXED_KEYS; n < num_nodes; n++) {
    if ((live[n / 64] & (UINT64_C(1) << (n % 64))) == 0 ||
        names[n].other == NULL) {
      continue;
    }
    bool home_first = from_fast == names[n].fast_home;
    TargetInstruction* from = home_first ? names[n].home : names[n].other;
    TargetInstruction* to = home_first ? names[n].other : names[n].home;
    TargetInstruction* copy = desc->create_copy(desc->ctx, from, to);
    GetSlot(ra, copy)->split_copy = true;
    if (at_start) {
      EmitAtStart(ra, block, copy);
    } else {
      EmitAtEnd(ra, block, copy);
    }
  }
}

// Picks the nodes to split from the edges between the two paths, and where
// their copies go.  Returns the number of crossing edges, or 0 when there is
// nothing to split or a copy has nowhere to go.
static size_t PlanSplit(RA* ra, const bool* fast, uint64_t* split,
                        CrossingEdge* crossing) {
  TargetGenerator* gen = ra->gen;
  size_t words = ra->words;
  size_t num_crossing = 0;
  bool any = false;
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    TargetBasicBlock* from = BlockAt(gen, b);
    for (size_t i = 0; from != NULL && i < from->out_edges.length; i++) {
      TargetBasicBlock* to = EdgeBlock(ra, &from->out_edges, i);
      if (to == NULL || fast[b] == fast[to->block_id]) {
        continue;
      }
      const uint64_t* live = ra->live_in + to->block_id * words;
      bool live_across = false;
      for (int n = TARGET_RA_FIXED_KEYS; n < NumNodes(ra); n++) {
        if ((live[n / 64] & (UINT64_C(1) << (n % 64))) != 0) {
          split[n / 64] |= UINT64_C(1) << (n % 64);
          live_across = true;
        }
      }
      Placement placement = PlaceCopies(ra, fast, from, to);
      if (live_across && placement == kPlaceNowhere) {
        return 0;
      }
      any = any || live_across;
      crossing[num_crossing++] = (CrossingEdge){from, to, placement};
    }
  }
  return any ? num_crossing : 0;
}

// Returns whether it left the analysis BuildRound would make, having split
// nothing.
static bool SplitAtFastPath(RA* ra) {
  TargetGenerator* gen = ra->gen;
  const TargetRegAllocDesc* desc = ra->desc;
  if (desc->create_location == NULL || desc->create_label == NULL ||
      desc->create_jump == NULL || gen->entry_block == NULL ||
      gen->exception_edges.length != 0) {
    return false;
  }
  size_t num_blocks = gen->basic_blocks.length;
  bool* fast = calloc(num_blocks, sizeof(bool));
  if (!FindFastPath(ra, fast)) {
    free(fast);
    return false;
  }
  CollectInstructions(ra);
  BuildNodes(ra);
  ComputeLiveness(ra);

  size_t num_edges = 0;
  for (size_t b = 0; b < num_blocks; b++) {
    TargetBasicBlock* block = BlockAt(gen, b);
    num_edges += block == NULL ? 0 : block->out_edges.length;
  }
  uint64_t* split = calloc(ra->words + 1, sizeof(uint64_t));
  CrossingEdge* crossing = calloc(num_edges + 1, sizeof(CrossingEdge));
  size_t num_crossing = PlanSplit(ra, fast, split, crossing);
  int num_nodes = NumNodes(ra);
  SplitNames* names = calloc((size_t)num_nodes, sizeof(SplitNames));
  // Name both sides.  A value's reads all move to locations, so that the
  // side it is made on can be written by copies too.
  for (int n = TARGET_RA_FIXED_KEYS; num_crossing > 0 && n < num_nodes; n++) {
    if ((split[n / 64] & (UINT64_C(1) << (n % 64))) == 0) {
      continue;
    }
    TargetInstruction* inst = NodeAt(ra, n)->inst;
    SplitNames* name = &names[n];
    if (ra->info->kind(inst) == kTargetRAValue) {
      name->fast_home = fast[InstAt(ra, Position(ra, inst))->block];
      name->home = desc->create_location(desc->ctx, inst);
    } else {
      name->fast_home = true;
      name->home = inst;
    }
    name->other = name->home == NULL
                      ? NULL
                      : desc->create_location(desc->ctx, inst);
    ra->stats.split += name->other != NULL;
  }

  for (int i = 0; num_crossing > 0 && i < NumInsts(ra); i++) {
    TargetInstruction* inst = InstOf(ra, i);
    bool side = fast[InstAt(ra, i)->block];
    RenameReads(ra, names, num_nodes, inst, side, 0);
    if (inst->dest != NULL) {
      TargetInstruction* dest =
          SplitName(ra, names, num_nodes, inst->dest, side);
      if (dest != NULL) {
        inst->dest = dest;
      }
    }
  }
  for (int n = TARGET_RA_FIXED_KEYS; num_crossing > 0 && n < num_nodes; n++) {
    TargetInstruction* value = NodeAt(ra, n)->inst;
    if (names[n].other == NULL || names[n].home == value) {
      continue;
    }
    TargetInstruction* copy = desc->create_copy(desc->ctx, value, names[n].home);
    TargetBasicBlockEmitAfter(gen, InstBlock(ra, Position(ra, value)), copy,
                              value);
    GetSlot(ra, copy)->transparent = true;
  }

  bool* start_done = calloc(num_blocks, sizeof(bool));
  bool* end_done = calloc(num_blocks, sizeof(bool));
  uint64_t* live = calloc(ra->words + 1, sizeof(uint64_t));
  for (size_t e = 0; e < num_crossing; e++) {
    TargetBasicBlock* from = crossing[e].from;
    TargetBasicBlock* to = crossing[e].to;
    bool from_fast = fast[from->block_id];
    const uint64_t* live_in = ra->live_in + to->block_id * ra->words;
    switch (crossing[e].placement) {
      case kPlaceAtStart:
        if (!start_done[to->block_id]) {
          start_done[to->block_id] = true;
          EmitSplitCopies(ra, names, num_nodes, live_in, from_fast, to,
                          /*at_start=*/true);
        }
        break;
      case kPlaceAtEnd:
        if (!end_done[from->block_id]) {
          end_done[from->block_id] = true;
          // Successors that take the copies at their start need none here.
          memset(live, 0, sizeof(uint64_t) * ra->words);
          for (size_t c = 0; c < num_crossing; c++) {
            if (crossing[c].from != from ||
                crossing[c].placement != kPlaceAtEnd) {
              continue;
            }
            size_t succ = crossing[c].to->block_id;
            for (size_t w = 0; w < ra->words; w++) {
              live[w] |= ra->live_in[succ * ra->words + w];
            }
          }
          EmitSplitCopies(ra, names, num_nodes, live, from_fast, from,
                          /*at_start=*/false);
        }
        break;
      case kPlaceOnEdge: {
        TargetBasicBlock* edge = SplitEdge(ra, from, to);
        EmitSplitCopies(ra, names, num_nodes, live_in, from_fast, edge,
                        /*at_start=*/false);
        break;
      }
      case kPlaceNowhere:
        break;
    }
  }
  free(live);
  free(start_done);
  free(end_done);
  free(names);
  free(crossing);
  free(split);
  free(fast);
  if (num_crossing > 0) {
    ra->changed_ir = true;
    FreeRound(ra);
  }
  return num_crossing == 0;
}

static bool GiveUp(RA* ra, const char* reason) {
  if (getenv("DAVECC_NEW_RA_STATS") != NULL) {
    fprintf(stderr, "new-ra-fallback\t%s\t%s\n", ra->gen->function_name.value,
            reason);
  }
  ChunkFree(&ra->insts);
  ChunkFree(&ra->slots);
  free(ra);
  return false;
}

bool TargetAllocateRegisters(TargetGenerator* gen,
                             const TargetRegAllocDesc* desc,
                             TargetRegAllocStats* stats) {
  RA* ra = calloc(1, sizeof(RA));
  ra->gen = gen;
  ra->desc = desc;
  ra->info = desc->info;
  for (int k = 0; k < TARGET_RA_FIXED_KEYS; k++) {
    ra->key_class[k] = -1;
    ra->key_index[k] = -1;
  }
  for (int c = 0; c < desc->num_classes; c++) {
    ra->k[c] = desc->classes[c].num_regs;
    for (int r = 0; r < desc->classes[c].num_regs; r++) {
      int key = desc->classes[c].regs[r];
      assert(ra->key_class[key] < 0);
      ra->key_class[key] = c;
      ra->key_index[key] = r;
    }
  }

  CollectInstructions(ra);
  const char* unsupported = Unsupported(ra);
  if (unsupported != NULL) {
    return GiveUp(ra, unsupported);
  }
  CountAllReads(ra);
  CopyFixedValues(ra);
  FreeInstructions(ra);
  bool analyzed = SplitAtFastPath(ra);

  for (;;) {
    ra->stats.rounds++;
    BuildRound(ra, analyzed);
    analyzed = false;
    if (ra->stats.rounds == 1) {
      ra->stats.nodes = NumNodes(ra) - TARGET_RA_FIXED_KEYS;
      ra->stats.moves = ra->moves.length;
    }
    Vector spilled;
    VectorInit(&spilled);
    Color(ra, &spilled);
    if (spilled.length == 0) {
      AssignRegisters(ra);
      VectorDestruct(&spilled);
      FreeRound(ra);
      break;
    }
    const char* stuck = NULL;
    if (desc->create_spill == NULL && desc->rewrite_spill == NULL) {
      stuck = "spill";
    } else if (ra->stats.rounds >= kMaxRounds) {
      stuck = "rounds";
    } else if (desc->rewrite_spill != NULL) {
      if (!RewriteSpillsByTarget(ra, &spilled)) {
        stuck = "spill";
      }
    } else {
      RewriteSpills(ra, &spilled);
    }
    VectorDestruct(&spilled);
    FreeRound(ra);
    if (stuck != NULL) {
      if (ra->changed_ir) {
        fprintf(stderr, "register allocation of %s %s\n",
                gen->function_name.value,
                strcmp(stuck, "rounds") == 0 ? "did not converge"
                                             : "needs to spill");
        abort();
      }
      return GiveUp(ra, stuck);
    }
  }

  if (getenv("DAVECC_NEW_RA_STATS") != NULL) {
    const TargetRegAllocStats* s = &ra->stats;
    fprintf(stderr,
            "new-ra\t%s\trounds=%d\tnodes=%d\tmoves=%d\tcoalesced=%d\t"
            "spilled=%d\tcopies=%d\tsplit=%d\n",
            gen->function_name.value, s->rounds, s->nodes, s->moves,
            s->coalesced, s->spilled, s->copies, s->split);
  }
  if (stats != NULL) {
    *stats = ra->stats;
  }
  ChunkFree(&ra->insts);
  ChunkFree(&ra->slots);
  free(ra);
  return true;
}
