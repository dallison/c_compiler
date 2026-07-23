//
//  loop_info.h
//  c_compiler
//
//  Persistent natural-loop information for IR optimization.
//

#ifndef loop_info_h
#define loop_info_h

#include <stdbool.h>
#include <stddef.h>

#include "bitset.h"
#include "vector.h"

struct BasicBlock;
struct Generator;

typedef struct LoopInfo {
  size_t loop_id;
  struct BasicBlock* header;
  struct BasicBlock* preheader;
  BitSet blocks;   // Natural-loop blocks, including header and latches.
  BitSet latches;  // Blocks with a back edge to the header.
  BitSet exits;    // Loop blocks with at least one successor outside the loop.
  struct LoopInfo* parent;
  Vector children;  // LoopInfo*.
  int depth;
} LoopInfo;

void LoopInfoBuild(struct Generator* gen);
void LoopInfoClear(struct Generator* gen);
bool LoopInfoContainsBlock(const LoopInfo* loop,
                           const struct BasicBlock* block);

// Create dedicated preheaders before SSA conversion.  Returns true when the
// CFG changed.  LoopInfoBuild must be called again after a true result.
bool LoopInfoCreatePreheaders(struct Generator* gen);

#endif /* loop_info_h */
