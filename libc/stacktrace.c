#include <stacktrace_runtime.h>

#if !defined(__6502__)
#include <eh_frame.h>
#endif

#define DAVECC_STACKTRACE_MAX_MODULES 32
#define DAVECC_STACKTRACE_WALK_LIMIT 1024

extern const unsigned char __davecc_stacktrace_start[];
extern const unsigned char __davecc_stacktrace_end[];

DaveStacktraceModule
    __davecc_stacktrace_modules[DAVECC_STACKTRACE_MAX_MODULES];
size_t __davecc_stacktrace_module_count;

static uintptr_t ReadNative(const unsigned char* p) {
  return *(const uintptr_t*)p;
}

static const char* FindDescriptionInTable(const unsigned char* start,
                                          const unsigned char* end,
                                          uintptr_t pc) {
  if (start == 0 || end <= start ||
      (size_t)(end - start) < sizeof(uintptr_t)) {
    return 0;
  }

  size_t count = (size_t)ReadNative(start);
  const unsigned char* records = start + sizeof(uintptr_t);
  if (count > (size_t)(end - records) / sizeof(DaveStacktraceSymbol)) {
    return 0;
  }

  size_t low = 0;
  size_t high = count;
  while (low < high) {
    size_t middle = low + (high - low) / 2;
    const DaveStacktraceSymbol* symbol =
        (const DaveStacktraceSymbol*)(records +
                                      middle * sizeof(DaveStacktraceSymbol));
    uintptr_t symbol_start =
        (uintptr_t)start + symbol->start_offset;
    if (pc < symbol_start) {
      high = middle;
      continue;
    }
    uintptr_t symbol_end = (uintptr_t)start + symbol->end_offset;
    if (pc >= symbol_end) {
      low = middle + 1;
      continue;
    }
    if (symbol->name_offset >= (uintptr_t)(end - start)) {
      return 0;
    }
    const char* name = (const char*)(start + symbol->name_offset);
    const char* limit = (const char*)end;
    for (const char* p = name; p < limit; p++) {
      if (*p == '\0') {
        return name;
      }
    }
    return 0;
  }
  return 0;
}

const char* __davecc_stacktrace_description(uintptr_t pc) {
  const char* result =
      FindDescriptionInTable(__davecc_stacktrace_start,
                             __davecc_stacktrace_end, pc);
  if (result != 0) {
    return result;
  }
  size_t count = __davecc_stacktrace_module_count;
  if (count > DAVECC_STACKTRACE_MAX_MODULES) {
    count = DAVECC_STACKTRACE_MAX_MODULES;
  }
  for (size_t i = 0; i < count; i++) {
    result = FindDescriptionInTable(
        __davecc_stacktrace_modules[i].start,
        __davecc_stacktrace_modules[i].end, pc);
    if (result != 0) {
      return result;
    }
  }
  return 0;
}

#if defined(__6502__)

extern unsigned char __davecc_6502_hardware_stack_pointer(void);

static size_t CaptureNativeFrames(uintptr_t* frames, size_t capacity,
                                  size_t skip, size_t max_depth) {
  /*
   * The 65C02 JSR stack contains two bytes per active call.  The helper's
   * saved stack pointer still addresses its just-popped return address, whose
   * bytes remain in page one.  Omit the helper, this runtime function, and
   * basic_stacktrace::current before applying the caller's skip.
   */
  const volatile unsigned char* hardware_stack =
      (const volatile unsigned char*)(uintptr_t)0x0100;
  unsigned int stack_pointer =
      (unsigned int)__davecc_6502_hardware_stack_pointer();
  size_t internal_skip = 3;
  size_t captured = 0;

  for (unsigned int index = stack_pointer + 1;
       index + 1 <= 0xff && captured < max_depth;
       index += 2) {
    uintptr_t pc =
        (uintptr_t)((unsigned int)hardware_stack[index] |
                    ((unsigned int)hardware_stack[index + 1] << 8));
    pc = (uintptr_t)(pc + 1);
    if (internal_skip != 0) {
      internal_skip--;
      continue;
    }
    if (skip != 0) {
      skip--;
      continue;
    }
    if (frames != 0 && captured < capacity) {
      frames[captured] = pc;
    }
    captured++;
  }
  return captured;
}

#else

extern void __davecc_capture_regs(DaveEHFrameRegisters* regs);

static size_t CaptureNativeFrames(uintptr_t* frames, size_t capacity,
                                  size_t skip, size_t max_depth) {
  DaveEHFrameRegisters regs;
  size_t internal_skip = 3;
  size_t captured = 0;

  __davecc_capture_regs(&regs);
  for (size_t walked = 0;
       walked < DAVECC_STACKTRACE_WALK_LIMIT && captured < max_depth;
       walked++) {
    DaveEHFrameWalkResult caller;
    if (!DaveEHFrameWalkFrame(&regs, &caller) || caller.caller_pc == 0) {
      break;
    }
    if (caller.caller_pc == regs.pc && caller.caller_rbp == regs.rbp &&
        caller.caller_rsp == regs.rsp) {
      break;
    }
    regs.pc = caller.caller_pc;
    regs.rsp = caller.caller_rsp;
    regs.rbp = caller.caller_rbp;

    if (internal_skip != 0) {
      internal_skip--;
      continue;
    }
    if (skip != 0) {
      skip--;
      continue;
    }
    if (frames != 0 && captured < capacity) {
      frames[captured] = regs.pc;
    }
    captured++;
  }
  return captured;
}

#endif

size_t __davecc_stacktrace_capture(uintptr_t* frames, size_t capacity,
                                   size_t skip, size_t max_depth) {
  if (max_depth == 0) {
    return 0;
  }
  return CaptureNativeFrames(frames, capacity, skip, max_depth);
}
