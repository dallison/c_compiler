#include <stdlib.h>
#include <stdint.h>
#include <eh_frame.h>

typedef struct {
  uintptr_t try_start;
  uintptr_t try_end;
  uintptr_t catch_label;
  uintptr_t catch_typeinfo;
} DaveExceptionTableEntry;

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
} CXXTypeInfo;

static intptr_t current_exception_object;
static const CXXTypeInfo* current_exception_typeinfo;

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

static int FindMatchingCatch(uintptr_t pc, const CXXTypeInfo* thrown_typeinfo,
                             uintptr_t* catch_label, long* catch_offset) {
  DaveExceptionTableEntry* entry =
      (DaveExceptionTableEntry*)__davecc_except_table_start;
  DaveExceptionTableEntry* end =
      (DaveExceptionTableEntry*)__davecc_except_table_end;
  DaveExceptionTableEntry* match = NULL;
  long match_offset = 0;

  while (entry < end) {
    const CXXTypeInfo* catch_typeinfo =
        (const CXXTypeInfo*)entry->catch_typeinfo;
    long offset = 0;
    if (pc >= entry->try_start && pc <= entry->try_end &&
        TypeInfoMatches(thrown_typeinfo, catch_typeinfo, &offset)) {
      if (match == NULL ||
          (entry->try_start > match->try_start &&
           entry->try_end <= match->try_end)) {
        match = entry;
        match_offset = offset;
      }
    }
    entry++;
  }
  if (match == NULL) {
    return 0;
  }
  *catch_label = match->catch_label;
  *catch_offset = match_offset;
  return 1;
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
  return (long long)current_exception_object;
}

void* __davecc_current_exception_ptr(void) {
  return (void*)current_exception_object;
}

void* __davecc_current_exception_addr(void) {
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

void __davecc_throw(intptr_t exception_object, const CXXTypeInfo* typeinfo) {
  // A null object and null typeinfo encode `throw;`: preserve the exception
  // currently being handled and resume unwinding from this frame.
  if (exception_object != 0 || typeinfo != NULL) {
    current_exception_object = exception_object;
    current_exception_typeinfo = typeinfo;
  }
  DaveEHFrameRegisters regs;
  DaveEHFrameWalkResult walk;
  uintptr_t catch_label;
  long catch_offset;

  __davecc_capture_regs(&regs);
  while (regs.pc != 0) {
    if (FindMatchingCatch(regs.pc, current_exception_typeinfo, &catch_label,
                          &catch_offset)) {
      // Adjust to the caught base subobject before the handler binds it.
      current_exception_object += catch_offset;
      __davecc_jump_to_landing_pad(catch_label, regs.rsp, regs.rbp);
    }
    if (!DaveEHFrameWalkFrame(&regs, &walk)) {
      break;
    }
    regs.pc = walk.caller_pc;
    regs.rsp = walk.caller_rsp;
    regs.rbp = walk.caller_rbp;
  }
  abort();
}
