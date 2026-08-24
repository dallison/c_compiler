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

static void SyncLegacyFromAdjusted(void) {
  void* adjusted = __davecc_eh_current_adjusted_ptr();
  if (adjusted != NULL) {
    current_exception_object = (intptr_t)adjusted;
  }
}

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
        SyncLegacyFromAdjusted();
        if (__davecc_eh_current_adjusted_ptr() == NULL) {
          current_exception_object += offset;
        }
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

char __davecc_current_exception_i1(void) {
  if (current_exception_kind == kExceptionI8) {
    return (char)current_exception_i8;
  }
  void* adjusted = __davecc_eh_current_adjusted_ptr();
  if (adjusted != NULL) {
    return *(char*)adjusted;
  }
  return (char)current_exception_object;
}

short __davecc_current_exception_i2(void) {
  if (current_exception_kind == kExceptionI8) {
    return (short)current_exception_i8;
  }
  void* adjusted = __davecc_eh_current_adjusted_ptr();
  if (adjusted != NULL) {
    return *(short*)adjusted;
  }
  return (short)current_exception_object;
}

int __davecc_current_exception_i4(void) {
  if (current_exception_kind == kExceptionI8) {
    return (int)current_exception_i8;
  }
  void* adjusted = __davecc_eh_current_adjusted_ptr();
  if (adjusted != NULL) {
    return *(int*)adjusted;
  }
  return (int)current_exception_object;
}

long long __davecc_current_exception_i8(void) {
  if (current_exception_kind == kExceptionI8) {
    return current_exception_i8;
  }
  void* adjusted = __davecc_eh_current_adjusted_ptr();
  if (adjusted != NULL) {
    return *(long long*)adjusted;
  }
  return (long long)current_exception_object;
}

float __davecc_current_exception_f4(void) {
  void* adjusted = __davecc_eh_current_adjusted_ptr();
  if (adjusted != NULL) {
    return *(float*)adjusted;
  }
  return current_exception_f4;
}

double __davecc_current_exception_f8(void) {
  void* adjusted = __davecc_eh_current_adjusted_ptr();
  if (adjusted != NULL) {
    return *(double*)adjusted;
  }
  return current_exception_f8;
}

void* __davecc_current_exception_ptr(void) {
  void* adjusted = __davecc_eh_current_adjusted_ptr();
  if (adjusted != NULL) {
    return adjusted;
  }
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
  return __davecc_current_exception_ptr();
}

intptr_t __davecc_current_exception_int(void) {
  return current_exception_object;
}

static void UnwindCurrentException(void) {
  DaveEHFrameRegisters regs;
  __davecc_capture_regs(&regs);
  UnwindStep(regs.pc, regs.rsp, regs.rbp, regs.pc, regs.pc);
}

static void MarkUncaught(void) {
  struct __cxa_eh_globals* globals = __davecc_eh_get_globals();
  globals->uncaughtExceptions++;
}

void __davecc_throw(intptr_t exception_object, const CXXTypeInfo* typeinfo) {
  if (exception_object != 0 || typeinfo != NULL) {
    current_exception_object = CopyExceptionObject(exception_object, typeinfo);
    current_exception_typeinfo = typeinfo;
    current_exception_kind = kExceptionDirect;
    __davecc_eh_sync_legacy_current_exception((void*)current_exception_object,
                                              typeinfo);
    MarkUncaught();
  } else {
    struct __cxa_eh_globals* globals = __davecc_eh_get_globals();
    if (globals->caughtExceptions != NULL) {
      struct __cxa_exception* header = globals->caughtExceptions;
      void* object = __davecc_eh_object_from_header(header);
      header->handlerCount--;
      if (header->handlerCount == 0) {
        globals->caughtExceptions = header->nextException;
        header->nextException = NULL;
      }
      __davecc_eh_install_active_exception(header, object);
    } else {
      MarkUncaught();
    }
  }
  UnwindCurrentException();
}

void __davecc_throw_i8(long long exception_object,
                       const CXXTypeInfo* typeinfo) {
  current_exception_i8 = exception_object;
  current_exception_object = (intptr_t)exception_object;
  current_exception_typeinfo = typeinfo;
  current_exception_kind = kExceptionI8;
  __davecc_eh_sync_legacy_current_exception(NULL, typeinfo);
  MarkUncaught();
  UnwindCurrentException();
}

void __davecc_throw_f4(float exception_object, const CXXTypeInfo* typeinfo) {
  current_exception_f4 = exception_object;
  current_exception_typeinfo = typeinfo;
  current_exception_kind = kExceptionF4;
  __davecc_eh_sync_legacy_current_exception(NULL, typeinfo);
  MarkUncaught();
  UnwindCurrentException();
}

void __davecc_throw_f8(double exception_object, const CXXTypeInfo* typeinfo) {
  current_exception_f8 = exception_object;
  current_exception_typeinfo = typeinfo;
  current_exception_kind = kExceptionF8;
  __davecc_eh_sync_legacy_current_exception(NULL, typeinfo);
  MarkUncaught();
  UnwindCurrentException();
}

#endif /* __p_code__ */
