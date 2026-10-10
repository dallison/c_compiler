#include <stdlib.h>
#include <stdint.h>
#include <eh_frame.h>

#include "eh_cxa_internal.h"

#if defined(__p_code__)

typedef struct {
  uintptr_t try_start;
  uintptr_t try_end;
  uintptr_t catch_label;
  uintptr_t catch_typeinfo;
} DaveExceptionTableEntry;

#define DAVECC_EH_CLEANUP 1

typedef DaveCXXTypeInfo CXXTypeInfo;

#if defined(__arm__) || defined(__risc_v__)
#define DAVECC_EH_THREAD_LOCAL __thread
#else
#define DAVECC_EH_THREAD_LOCAL
#endif

static DAVECC_EH_THREAD_LOCAL const CXXTypeInfo* current_exception_typeinfo;

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
    const DaveCXXTypeInfoBase* base = &thrown->bases[i];
    if (base->name == caught->name || StringEqual(base->name, caught->name)) {
      *offset = base->offset;
      return 1;
    }
  }
  return 0;
}

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
        continue;
      }
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

const CXXTypeInfo* DaveCurrentExceptionTypeInfo(void) {
  return current_exception_typeinfo;
}

// State handed to a cleanup landing pad so __davecc_resume can continue the
// containment chain in the same frame after the pad runs its destructor.
// Unwinding is sequential, so a single set of slots suffices.
static uintptr_t resume_pc;
static uintptr_t resume_rsp;
static uintptr_t resume_rbp;
static uintptr_t resume_cs;
static uintptr_t resume_ce;

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
        __davecc_eh_enter_catch_from_unwinder(offset);
        __davecc_jump_to_landing_pad(action->catch_label, rsp, rbp);
      }
      resume_pc = pc;
      resume_rsp = rsp;
      resume_rbp = rbp;
      resume_cs = action->try_start;
      resume_ce = action->try_end;
      __davecc_jump_to_landing_pad(action->catch_label, rsp, rbp);
    }
    regs.pc = pc;
    regs.rsp = rsp;
    regs.rbp = rbp;
    if (!DaveEHFrameWalkFrame(&regs, &walk)) {
      break;
    }
    pc = walk.caller.pc;
    rsp = walk.caller.rsp;
    rbp = walk.caller.rbp;
    cs = pc;
    ce = pc;
  }
  abort();
}

void __davecc_resume(void) {
  UnwindStep(resume_pc, resume_rsp, resume_rbp, resume_cs, resume_ce);
}

// Every thrown object lives in an exception header; entering a handler leaves
// the current adjusted pointer at the caught, base-adjusted object.
char __davecc_current_exception_i1(void) {
  return *(char*)__davecc_eh_current_adjusted_ptr();
}

short __davecc_current_exception_i2(void) {
  return *(short*)__davecc_eh_current_adjusted_ptr();
}

int __davecc_current_exception_i4(void) {
  return *(int*)__davecc_eh_current_adjusted_ptr();
}

long long __davecc_current_exception_i8(void) {
  return *(long long*)__davecc_eh_current_adjusted_ptr();
}

float __davecc_current_exception_f4(void) {
  return *(float*)__davecc_eh_current_adjusted_ptr();
}

double __davecc_current_exception_f8(void) {
  return *(double*)__davecc_eh_current_adjusted_ptr();
}

void* __davecc_current_exception_ptr(void) {
  return *(void**)__davecc_eh_current_adjusted_ptr();
}

void* __davecc_current_exception_addr(void) {
  return __davecc_eh_current_adjusted_ptr();
}

void* __davecc_current_exception_object(void) {
  return __davecc_eh_current_adjusted_ptr();
}

static void UnwindCurrentException(void) {
  DaveEHFrameRegisters regs;
  __davecc_capture_regs(&regs);
  UnwindStep(regs.pc, regs.rsp, regs.rbp, regs.pc, regs.pc);
}

// The compiler allocates the object with __cxa_allocate_exception and builds
// it in place; its header owns the object until the last handler's
// __cxa_end_catch destroys and frees it.
void __davecc_throw_object(void* object, const CXXTypeInfo* typeinfo,
                           void (*destructor)(void*)) {
  struct __cxa_exception* header = __davecc_eh_header_from_object(object);
  header->exceptionType = (struct type_info*)typeinfo;
  header->exceptionDestructor = destructor;
  header->adjustedPtr = object;
  header->handlerCount = 0;
  header->handlerSwitchValue = 0;
  header->nextException = NULL;
  current_exception_typeinfo = typeinfo;
  __davecc_eh_install_active_exception(header, object);
  __davecc_eh_sync_legacy_current_exception(object, typeinfo);
  UnwindCurrentException();
}

// A handler of a rethrown exception stays active until its __cxa_end_catch,
// which the compiler runs as the exception leaves the handler; a negative
// count marks it rethrown.
void __davecc_eh_raise_header(struct __cxa_exception* header) {
  if (header->handlerCount > 0) {
    header->handlerCount = -header->handlerCount;
  }
  void* object = __davecc_eh_object_from_header(header);
  const CXXTypeInfo* typeinfo = (const CXXTypeInfo*)header->exceptionType;
  current_exception_typeinfo = typeinfo;
  __davecc_eh_install_active_exception(header, object);
  __davecc_eh_sync_legacy_current_exception(object, typeinfo);
  UnwindCurrentException();
}

void __davecc_rethrow(void) {
  struct __cxa_exception* header = __davecc_eh_get_globals()->caughtExceptions;
  if (header == NULL) {
    abort();
  }
  __davecc_eh_raise_header(header);
}

#endif /* __p_code__ */
