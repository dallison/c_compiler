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

typedef struct {
  uintptr_t address_offset;
  uintptr_t file_offset;
  uintptr_t line;
} DaveStacktraceLine;

typedef struct {
  const unsigned char* start;
  const unsigned char* end;
  const DaveStacktraceSymbol* symbols;
  size_t symbol_count;
  const DaveStacktraceLine* lines;
  size_t line_count;
  size_t names_offset;
} DaveStacktraceTable;

static int ReadStacktraceTable(const unsigned char* start,
                               const unsigned char* end,
                               DaveStacktraceTable* table) {
  const size_t header_size = 4 * sizeof(uintptr_t);
  if (start == 0 || end <= start ||
      (size_t)(end - start) < header_size) {
    return 0;
  }

  size_t symbol_count = (size_t)ReadNative(start);
  size_t line_count =
      (size_t)ReadNative(start + sizeof(uintptr_t));
  size_t line_offset =
      (size_t)ReadNative(start + 2 * sizeof(uintptr_t));
  size_t names_offset =
      (size_t)ReadNative(start + 3 * sizeof(uintptr_t));
  size_t table_size = (size_t)(end - start);
  if (line_offset < header_size || line_offset > names_offset ||
      names_offset > table_size ||
      symbol_count >
          (line_offset - header_size) / sizeof(DaveStacktraceSymbol) ||
      line_count >
          (names_offset - line_offset) / sizeof(DaveStacktraceLine)) {
    return 0;
  }

  table->start = start;
  table->end = end;
  table->symbols =
      (const DaveStacktraceSymbol*)(start + header_size);
  table->symbol_count = symbol_count;
  table->lines = (const DaveStacktraceLine*)(start + line_offset);
  table->line_count = line_count;
  table->names_offset = names_offset;
  return 1;
}

static const DaveStacktraceSymbol* FindSymbol(
    const DaveStacktraceTable* table, uintptr_t pc) {
  size_t low = 0;
  size_t high = table->symbol_count;
  while (low < high) {
    size_t middle = low + (high - low) / 2;
    const DaveStacktraceSymbol* symbol = &table->symbols[middle];
    uintptr_t symbol_start =
        (uintptr_t)table->start + symbol->start_offset;
    if (pc < symbol_start) {
      high = middle;
      continue;
    }
    uintptr_t symbol_end =
        (uintptr_t)table->start + symbol->end_offset;
    if (pc >= symbol_end) {
      low = middle + 1;
      continue;
    }
    return symbol;
  }
  return 0;
}

static const char* ReadTableString(const DaveStacktraceTable* table,
                                   uintptr_t offset) {
  if (offset < table->names_offset ||
      offset >= (uintptr_t)(table->end - table->start)) {
    return 0;
  }
  const char* text = (const char*)(table->start + offset);
  const char* limit = (const char*)table->end;
  for (const char* p = text; p < limit; p++) {
    if (*p == '\0') {
      return text;
    }
  }
  return 0;
}

static const char* FindDescriptionInTable(const unsigned char* start,
                                          const unsigned char* end,
                                          uintptr_t pc) {
  DaveStacktraceTable table;
  if (!ReadStacktraceTable(start, end, &table)) {
    return 0;
  }
  const DaveStacktraceSymbol* symbol = FindSymbol(&table, pc);
  if (symbol == 0) {
    return 0;
  }
  return ReadTableString(&table, symbol->name_offset);
}

static const DaveStacktraceLine* FindSourceInTable(
    const unsigned char* start, const unsigned char* end, uintptr_t pc,
    DaveStacktraceTable* table) {
  if (!ReadStacktraceTable(start, end, table)) {
    return 0;
  }
  const DaveStacktraceSymbol* symbol = FindSymbol(table, pc);
  if (symbol == 0 || table->line_count == 0) {
    return 0;
  }

  size_t low = 0;
  size_t high = table->line_count;
  while (low < high) {
    size_t middle = low + (high - low) / 2;
    uintptr_t line_address =
        (uintptr_t)table->start + table->lines[middle].address_offset;
    if (pc < line_address) {
      high = middle;
    } else {
      low = middle + 1;
    }
  }
  if (low == 0) {
    return 0;
  }
  const DaveStacktraceLine* line = &table->lines[low - 1];
  uintptr_t line_address =
      (uintptr_t)table->start + line->address_offset;
  uintptr_t symbol_start =
      (uintptr_t)table->start + symbol->start_offset;
  return line_address < symbol_start ? 0 : line;
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

const char* __davecc_stacktrace_source_file(uintptr_t pc) {
  DaveStacktraceTable table;
  const DaveStacktraceLine* line =
      FindSourceInTable(__davecc_stacktrace_start,
                        __davecc_stacktrace_end, pc, &table);
  if (line != 0) {
    return ReadTableString(&table, line->file_offset);
  }
  size_t count = __davecc_stacktrace_module_count;
  if (count > DAVECC_STACKTRACE_MAX_MODULES) {
    count = DAVECC_STACKTRACE_MAX_MODULES;
  }
  for (size_t i = 0; i < count; i++) {
    line = FindSourceInTable(__davecc_stacktrace_modules[i].start,
                             __davecc_stacktrace_modules[i].end, pc,
                             &table);
    if (line != 0) {
      return ReadTableString(&table, line->file_offset);
    }
  }
  return 0;
}

uint_least32_t __davecc_stacktrace_source_line(uintptr_t pc) {
  DaveStacktraceTable table;
  const DaveStacktraceLine* line =
      FindSourceInTable(__davecc_stacktrace_start,
                        __davecc_stacktrace_end, pc, &table);
  if (line != 0) {
    return (uint_least32_t)line->line;
  }
  size_t count = __davecc_stacktrace_module_count;
  if (count > DAVECC_STACKTRACE_MAX_MODULES) {
    count = DAVECC_STACKTRACE_MAX_MODULES;
  }
  for (size_t i = 0; i < count; i++) {
    line = FindSourceInTable(__davecc_stacktrace_modules[i].start,
                             __davecc_stacktrace_modules[i].end, pc,
                             &table);
    if (line != 0) {
      return (uint_least32_t)line->line;
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
