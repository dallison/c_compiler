//
//  target_ra_ir.h
//  c_compiler_library
//
//  How a register allocator reads the Target IR, and a check that a function
//  follows those rules.
//
//  1. An operand edge to X reads the value X computed.  X dominates the read,
//     except across the conservative edges the CFG adds for exception
//     cleanups; there the value is undefined on a path that never reaches
//     the read at run time.
//  2. X->dest = D also copies X's value into D when X executes.  D is a
//     location: storage that more than one instruction may write, such as a
//     variable register, a merge tmp or a fixed register.  A read of D reads
//     whatever was copied into it last.
//  3. An instruction writes only its own result, its dest and the registers
//     it clobbers.
//
//  Under rule 2 dest is a copy, not an alias.  The existing allocators give X
//  the register of D, which reads the same value only while nothing else
//  writes D before X's last read.  TargetRACheck counts the reads where the
//  two readings differ, along with IR that breaks the rules.
//

#ifndef target_ra_ir_h
#define target_ra_ir_h

#include <stdint.h>
#include <stdio.h>
#include "hashmap.h"
#include "target_generator.h"

typedef enum {
  kTargetRANone,      // Not read from a register: constant, label, marker.
  kTargetRAValue,     // Computes one value.
  kTargetRALocation,  // Storage written through other instructions' dest.
} TargetRAKind;

typedef enum {
  kTargetRANotCopy,
  // A copy the allocator may delete by giving both sides one register.
  kTargetRACopy,
  // A copy that also changes the value, such as a 32-bit move that clears
  // the upper half.  Both sides must stay in different registers.
  kTargetRAConvertingCopy,
} TargetRACopyKind;

// Locations that name a physical register have a key below this.  Every
// other location is its own storage.
#define TARGET_RA_FIXED_KEYS 128

typedef struct {
  TargetRAKind (*kind)(TargetInstruction* inst);
  // Physical register key for a location, or -1 when the location is not
  // tied to a physical register.
  int (*location_key)(TargetInstruction* inst);
  // Operand that |inst| writes instead of reading, or -1.
  int (*written_operand)(TargetInstruction* inst);
  // Physical register keys |inst| overwrites as a side effect, such as a
  // call's caller-saved registers.  Returns how many it stored in |keys|.
  int (*clobbers)(TargetInstruction* inst, int* keys, int max_keys);
  // Whether reading |operand| reads its own operands, as a chain of
  // argument holders does at the call.
  bool (*reads_through)(TargetInstruction* operand);
  // Whether |inst| copies operand 0 into its result, and how.
  TargetRACopyKind (*copy_kind)(TargetInstruction* inst);
} TargetRAInfo;

enum {
  kTargetRADestVar,    // ivarreg / fvarreg.
  kTargetRADestTmp,    // Merge tmp.
  kTargetRADestFixed,  // Physical register.
  kTargetRADestKinds,
};

typedef struct {
  int functions;
  int reads;              // Operand reads of values.
  int dest_reads;         // Reads of a value that also has a dest.
  int undominated;        // Reads not dominated by the value they read.
  int orphaned;           // Reads of a value that is in no block.
  int dest_not_location;  // A dest that names a value.
  int written_operands;   // Instructions that write through an operand.
  int overwritten[kTargetRADestKinds];  // Rule 2 reads, by dest kind.
} TargetRACheckCounts;

// Checks |gen|'s current function.  Adds to |counts| and, when |fp| is not
// NULL, prints one summary line plus up to |max_details| examples of each
// problem found.
void TargetRACheck(TargetGenerator* gen, const TargetRAInfo* info,
                   TargetRACheckCounts* counts, FILE* fp, int max_details);

// Map from an instruction to a non-negative integer.  Instruction ids repeat
// across functions, so this is keyed by the pointer.
typedef struct {
  HashMap map;
} TargetRAIndex;

static inline void TargetRAIndexInit(TargetRAIndex* index, size_t capacity) {
  HashMapInit(&index->map, capacity);
}

static inline void TargetRAIndexDestruct(TargetRAIndex* index) {
  HashMapDestruct(&index->map);
}

// The value stored for |inst|, or -1.
static inline int TargetRAIndexFind(const TargetRAIndex* index,
                                    const TargetInstruction* inst) {
  return (int)HashMapFind(&index->map, (uint64_t)(uintptr_t)inst, -1);
}

static inline void TargetRAIndexInsert(TargetRAIndex* index,
                                       const TargetInstruction* inst,
                                       int value) {
  HashMapInsert(&index->map, (uint64_t)(uintptr_t)inst, value);
}

#endif /* target_ra_ir_h */
