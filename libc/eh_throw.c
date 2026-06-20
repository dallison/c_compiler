#include <stdlib.h>
#include <stdint.h>
#include <eh_frame.h>

typedef struct {
  uintptr_t try_start;
  uintptr_t try_end;
  uintptr_t catch_label;
  uintptr_t catch_typeinfo;
} DaveExceptionTableEntry;

typedef char DaveTypeInfo;

static intptr_t current_exception_object;
static const DaveTypeInfo* current_exception_typeinfo;

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

static int TypeInfoMatches(const DaveTypeInfo* thrown,
                           const DaveTypeInfo* caught) {
  if (caught == 0) {
    return 1;
  }
  if (thrown == 0) {
    return 0;
  }
  return thrown == caught || StringEqual((const char*)thrown, (const char*)caught);
}

static int FindMatchingCatch(uintptr_t pc, const DaveTypeInfo* thrown_typeinfo,
                             uintptr_t* catch_label) {
  DaveExceptionTableEntry* entry =
      (DaveExceptionTableEntry*)__davecc_except_table_start;
  DaveExceptionTableEntry* end =
      (DaveExceptionTableEntry*)__davecc_except_table_end;
  DaveExceptionTableEntry* match = NULL;

  while (entry < end) {
    const DaveTypeInfo* catch_typeinfo =
        (const DaveTypeInfo*)entry->catch_typeinfo;
    if (pc >= entry->try_start && pc < entry->try_end &&
        TypeInfoMatches(thrown_typeinfo, catch_typeinfo)) {
      if (match == NULL ||
          (entry->try_start > match->try_start &&
           entry->try_end <= match->try_end)) {
        match = entry;
      }
    }
    entry++;
  }
  if (match == NULL) {
    return 0;
  }
  *catch_label = match->catch_label;
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

void __davecc_throw(intptr_t exception_object, const DaveTypeInfo* typeinfo) {
  current_exception_object = exception_object;
  current_exception_typeinfo = typeinfo;
  DaveEHFrameRegisters regs;
  DaveEHFrameWalkResult walk;
  uintptr_t catch_label;

  __davecc_capture_regs(&regs);
  while (regs.pc != 0) {
    if (FindMatchingCatch(regs.pc, current_exception_typeinfo, &catch_label)) {
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
