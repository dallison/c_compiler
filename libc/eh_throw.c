#include <stdlib.h>
#include <stdint.h>
#include <eh_frame.h>

typedef struct {
  uintptr_t try_start;
  uintptr_t try_end;
  uintptr_t catch_label;
  uintptr_t catch_typeinfo;
} DaveExceptionTableEntry;

// Sentinel stored in `catch_typeinfo` to mark a *cleanup* range (a region whose
// landing pad destroys one automatic object and then resumes unwinding) rather
// than a handler.  A value of 0 means catch-all (`catch (...)` / the noexcept
// terminate guard); any real CXXTypeInfo lives in .rodata at an address far
// above these small sentinels, so 1 is safe to reserve.  Must match
// DAVECC_EH_CLEANUP_MARKER in the compiler backend (see codegen.h).
#define DAVECC_EH_CLEANUP 1

// Layout must match the objects emitted by the compiler (see
// X86_64PrintTypeInfoRecords in c_compiler/x86_64/x86_64_emitter.c).  `bases`
// is a flattened list of the exception type's public, non-virtual base
// subobjects with their byte offsets from the most-derived object, so a handler
// naming a base class matches and the exception pointer can be adjusted.
typedef struct CXXTypeInfoBase {
  const char* name;
  long offset;
} CXXTypeInfoBase;

typedef struct CXXTypeInfo {
  const char* name;
  long base_count;
  const CXXTypeInfoBase* bases;
  long object_size;
  long object_is_class;
} CXXTypeInfo;

#if defined(__arm__) || defined(__risc_v__)
#define DAVECC_EH_THREAD_LOCAL __thread
#else
#define DAVECC_EH_THREAD_LOCAL
#endif

static DAVECC_EH_THREAD_LOCAL intptr_t current_exception_object;
static DAVECC_EH_THREAD_LOCAL const CXXTypeInfo* current_exception_typeinfo;
typedef enum {
  kExceptionDirect,
  kExceptionI8,
  kExceptionF4,
  kExceptionF8,
} ExceptionValueKind;
static DAVECC_EH_THREAD_LOCAL ExceptionValueKind current_exception_kind;
static DAVECC_EH_THREAD_LOCAL long long current_exception_i8;
static DAVECC_EH_THREAD_LOCAL float current_exception_f4;
static DAVECC_EH_THREAD_LOCAL double current_exception_f8;

static intptr_t CopyExceptionObject(intptr_t exception_object,
                                    const CXXTypeInfo* typeinfo) {
  if (exception_object == 0 || typeinfo == 0 || !typeinfo->object_is_class ||
      typeinfo->object_size <= 0) {
    return exception_object;
  }
  unsigned char* copy = (unsigned char*)malloc((size_t)typeinfo->object_size);
  if (copy == 0) {
    abort();
  }
  const unsigned char* source = (const unsigned char*)exception_object;
  for (long i = 0; i < typeinfo->object_size; i++) {
    copy[i] = source[i];
  }
  return (intptr_t)copy;
}

extern char __davecc_except_table_start[];
extern char __davecc_except_table_end[];

void __davecc_capture_regs(DaveEHFrameRegisters* regs);
void __davecc_jump_to_landing_pad(uintptr_t target, uintptr_t rsp,
                                  uintptr_t rbp);

static int StringEqual(const char* a, const char* b) {
  if (a == b) {
    return 1;
  }
  if (a == 0 || b == 0) {
    return 0;
  }
  while (*a != 0 && *a == *b) {
    a++;
    b++;
  }
  return *a == *b;
}

// Decides whether a handler naming `caught` matches a thrown object whose static
// type is `caught`.  A null `caught` is `catch (...)`.  On a match through a base
// class, `*offset` receives the byte offset of that base subobject so the caller
// can adjust the exception object pointer; it is 0 for an exact match.
static int TypeInfoMatches(const CXXTypeInfo* thrown,
                           const CXXTypeInfo* caught, long* offset) {
  *offset = 0;
  if (caught == 0) {
    return 1;
  }
  if (thrown == 0) {
    return 0;
  }
  if (thrown == caught || StringEqual(thrown->name, caught->name)) {
    return 1;
  }
  for (long i = 0; i < thrown->base_count; i++) {
    const CXXTypeInfoBase* base = &thrown->bases[i];
    if (base->name == caught->name || StringEqual(base->name, caught->name)) {
      *offset = base->offset;
      return 1;
    }
  }
  return 0;
}

// True when [start,end] contains `pc` and *strictly* encloses the window
// [cs,ce] (the range of the cleanup most recently run at this pc).  The chain of
// ranges containing a pc is totally ordered by nesting, so "strictly encloses
// the last-run range" is how we advance outward one scope at a time.
static int RangeEncloses(uintptr_t start, uintptr_t end, uintptr_t pc,
                         uintptr_t cs, uintptr_t ce) {
  if (pc < start || pc > end) {
    return 0;
  }
  if (start > cs || end < ce) {
    return 0;
  }
  return start < cs || end > ce;
}

// Selects the innermost (tightest) actionable exception-table entry at `pc` that
// strictly encloses the window [cs,ce].  A cleanup range is always actionable; a
// handler range is actionable only when its type matches the thrown object.
// Returns the entry, sets `*is_catch` and (for a matching handler) `*offset` to
// the base-subobject adjustment.
static DaveExceptionTableEntry* FindInnermostAction(uintptr_t pc, uintptr_t cs,
                                                    uintptr_t ce,
                                                    const CXXTypeInfo* thrown,
                                                    int* is_catch,
                                                    long* offset) {
  DaveExceptionTableEntry* entry =
      (DaveExceptionTableEntry*)__davecc_except_table_start;
  DaveExceptionTableEntry* end =
      (DaveExceptionTableEntry*)__davecc_except_table_end;
  DaveExceptionTableEntry* best = NULL;
  int best_is_catch = 0;
  long best_offset = 0;

  while (entry < end) {
    if (RangeEncloses(entry->try_start, entry->try_end, pc, cs, ce)) {
      int entry_is_catch;
      long entry_offset = 0;
      if (entry->catch_typeinfo == DAVECC_EH_CLEANUP) {
        entry_is_catch = 0;
      } else if (TypeInfoMatches(
                     thrown, (const CXXTypeInfo*)entry->catch_typeinfo,
                     &entry_offset)) {
        entry_is_catch = 1;
      } else {
        entry++;
        continue;  // a handler whose type does not match: not actionable
      }
      // Tightest range wins: greatest start, then smallest end.
      if (best == NULL || entry->try_start > best->try_start ||
          (entry->try_start == best->try_start &&
           entry->try_end < best->try_end)) {
        best = entry;
        best_is_catch = entry_is_catch;
        best_offset = entry_offset;
      }
    }
    entry++;
  }
  if (best != NULL) {
    *is_catch = best_is_catch;
    *offset = best_offset;
  }
  return best;
}

// State handed to a cleanup landing pad so __davecc_resume can continue the
// containment chain in the same frame after the pad runs its destructor.
// Unwinding is sequential, so a single set of slots suffices.
static uintptr_t resume_pc;
static uintptr_t resume_rsp;
static uintptr_t resume_rbp;
static uintptr_t resume_cs;
static uintptr_t resume_ce;

// Drives unwinding from frame (pc,rsp,rbp) outward.  Within each frame it walks
// the scope-containment chain: at [cs,ce] (initially the degenerate window
// [pc,pc]) it runs the innermost enclosing cleanup and re-enters via
// __davecc_resume with [cs,ce] tightened to that cleanup's range, so enclosing
// objects are destroyed in reverse construction order; a matching handler that
// is inner to any remaining cleanup is entered instead (stopping unwinding).
// When a frame has no further action, control moves to the caller frame.
static void UnwindStep(uintptr_t pc, uintptr_t rsp, uintptr_t rbp, uintptr_t cs,
                       uintptr_t ce) {
  DaveEHFrameRegisters regs;
  DaveEHFrameWalkResult walk;
  for (;;) {
    int is_catch = 0;
    long offset = 0;
    DaveExceptionTableEntry* action = FindInnermostAction(
        pc, cs, ce, current_exception_typeinfo, &is_catch, &offset);
    if (action != NULL) {
      if (is_catch) {
        // Adjust to the caught base subobject before the handler binds it.
        current_exception_object += offset;
        __davecc_jump_to_landing_pad(action->catch_label, rsp, rbp);
      }
      // Cleanup: remember where to resume, then run the pad in this frame.
      resume_pc = pc;
      resume_rsp = rsp;
      resume_rbp = rbp;
      resume_cs = action->try_start;
      resume_ce = action->try_end;
      __davecc_jump_to_landing_pad(action->catch_label, rsp, rbp);
    }
    // No further action in this frame: unwind to the caller.
    regs.pc = pc;
    regs.rsp = rsp;
    regs.rbp = rbp;
    if (!DaveEHFrameWalkFrame(&regs, &walk)) {
      break;
    }
    pc = walk.caller_pc;
    rsp = walk.caller_rsp;
    rbp = walk.caller_rbp;
    cs = pc;
    ce = pc;
  }
  abort();
}

// Re-entry point from a cleanup landing pad once it has run its destructor.
void __davecc_resume(void) {
  UnwindStep(resume_pc, resume_rsp, resume_rbp, resume_cs, resume_ce);
}

char __davecc_current_exception_i1(void) {
  return (char)current_exception_object;
}

short __davecc_current_exception_i2(void) {
  return (short)current_exception_object;
}

int __davecc_current_exception_i4(void) {
  return (int)current_exception_object;
}

long long __davecc_current_exception_i8(void) {
  if (current_exception_kind == kExceptionI8) {
    return current_exception_i8;
  }
  return (long long)current_exception_object;
}

float __davecc_current_exception_f4(void) {
  return current_exception_f4;
}

double __davecc_current_exception_f8(void) {
  return current_exception_f8;
}

void* __davecc_current_exception_ptr(void) {
  return (void*)current_exception_object;
}

void* __davecc_current_exception_addr(void) {
  if (current_exception_kind == kExceptionI8) {
    return &current_exception_i8;
  }
  if (current_exception_kind == kExceptionF4) {
    return &current_exception_f4;
  }
  if (current_exception_kind == kExceptionF8) {
    return &current_exception_f8;
  }
  return &current_exception_object;
}

void* __davecc_current_exception_object(void) {
  return (void*)current_exception_object;
}

intptr_t __davecc_current_exception_int(void) {
  return current_exception_object;
}

// The default std::terminate handler (__davecc_terminate) lives in
// eh_terminate.c so it is available on every backend, not just those with the
// full exception-unwinding runtime below.

static void UnwindCurrentException(void) {
  DaveEHFrameRegisters regs;
  __davecc_capture_regs(&regs);
  // The captured frame is the throw helper's own (no ranges); UnwindStep walks
  // out to the throwing frame and beyond, running cleanups and seeking a
  // handler.
  UnwindStep(regs.pc, regs.rsp, regs.rbp, regs.pc, regs.pc);
}

void __davecc_throw(intptr_t exception_object, const CXXTypeInfo* typeinfo) {
  // A null object and null typeinfo encode `throw;`: preserve the exception
  // currently being handled and resume unwinding from this frame.
  if (exception_object != 0 || typeinfo != NULL) {
    // Class throw operands are initially materialized in the throwing frame.
    // Preserve the completed object before unwinding discards and reuses that
    // stack storage.
    current_exception_object = CopyExceptionObject(exception_object, typeinfo);
    current_exception_typeinfo = typeinfo;
    current_exception_kind = kExceptionDirect;
  }
  UnwindCurrentException();
}

void __davecc_throw_i8(long long exception_object,
                       const CXXTypeInfo* typeinfo) {
  current_exception_i8 = exception_object;
  current_exception_typeinfo = typeinfo;
  current_exception_kind = kExceptionI8;
  UnwindCurrentException();
}

void __davecc_throw_f4(float exception_object, const CXXTypeInfo* typeinfo) {
  current_exception_f4 = exception_object;
  current_exception_typeinfo = typeinfo;
  current_exception_kind = kExceptionF4;
  UnwindCurrentException();
}

void __davecc_throw_f8(double exception_object, const CXXTypeInfo* typeinfo) {
  current_exception_f8 = exception_object;
  current_exception_typeinfo = typeinfo;
  current_exception_kind = kExceptionF8;
  UnwindCurrentException();
}
