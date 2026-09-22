//
//  xtensa_interpreter.c
//  c_compiler
//

#include "xtensa_interpreter.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../libc/include/davecc_guest_syscalls.h"
#include "xtensa_isa.h"

static unsigned PhysicalRegister(const XtensaInterpreter* interpreter,
                                 unsigned visible) {
  return (interpreter->window_base + visible) & 63;
}

static uint32_t ReadRegister(const XtensaInterpreter* interpreter,
                             unsigned visible) {
  return interpreter->aregs[PhysicalRegister(interpreter, visible)];
}

static void WriteRegister(XtensaInterpreter* interpreter, unsigned visible,
                          uint32_t value) {
  interpreter->aregs[PhysicalRegister(interpreter, visible)] = value;
}

static void* GuestPointer(XtensaInterpreter* interpreter, uint32_t address,
                          size_t size) {
  uint64_t end = (uint64_t)address + size;
  if (address >= XTENSA_HOSTED_STACK_BASE &&
      end <= XTENSA_HOSTED_STACK_BASE + XTENSA_HOSTED_STACK_SIZE) {
    return interpreter->stack + (address - XTENSA_HOSTED_STACK_BASE);
  }
  if (end > (uint64_t)UINT32_MAX + 1) {
    return NULL;
  }
  uint64_t runtime;
  if (!LoaderLinkedAddressToRuntime(interpreter->loader, NULL, address,
                                    &runtime)) {
    return NULL;
  }
  if (size > 1) {
    uint64_t last_runtime;
    if (!LoaderLinkedAddressToRuntime(interpreter->loader, NULL,
                                      (uint32_t)(end - 1), &last_runtime) ||
        last_runtime != runtime + size - 1) {
      return NULL;
    }
  }
  return (void*)(uintptr_t)runtime;
}

static int CopyGuestString(XtensaInterpreter* interpreter, uint32_t address,
                           char* destination, size_t capacity) {
  for (size_t i = 0; i < capacity; i++) {
    if ((uint64_t)address + i > UINT32_MAX) {
      return EFAULT;
    }
    const char* character = GuestPointer(interpreter, address + (uint32_t)i, 1);
    if (character == NULL) {
      return EFAULT;
    }
    destination[i] = *character;
    if (*character == '\0') {
      return 0;
    }
  }
  return ENAMETOOLONG;
}

static bool ReadMemory(XtensaInterpreter* interpreter, uint32_t address,
                       void* value, size_t size) {
  const void* source = GuestPointer(interpreter, address, size);
  if (source == NULL) {
    fprintf(stderr, "ESP32 invalid read at 0x%08" PRIx32 "\n", address);
    interpreter->running = false;
    interpreter->exit_code = 1;
    return false;
  }
  memcpy(value, source, size);
  return true;
}

static bool WriteMemory(XtensaInterpreter* interpreter, uint32_t address,
                        const void* value, size_t size) {
  void* destination = GuestPointer(interpreter, address, size);
  if (destination == NULL) {
    fprintf(stderr, "ESP32 invalid write at 0x%08" PRIx32 "\n", address);
    interpreter->running = false;
    interpreter->exit_code = 1;
    return false;
  }
  memcpy(destination, value, size);
  return true;
}

static bool Fetch(XtensaInterpreter* interpreter, uint8_t bytes[3]) {
  return ReadMemory(interpreter, interpreter->pc, bytes, 3);
}

static bool PushCall(XtensaInterpreter* interpreter, uint32_t return_pc) {
  if (interpreter->call_depth == XTENSA_MAX_CALL_DEPTH) {
    fprintf(stderr, "ESP32 hosted call depth exceeded\n");
    interpreter->running = false;
    interpreter->exit_code = 1;
    return false;
  }
  XtensaCallFrame* frame = &interpreter->calls[interpreter->call_depth++];
  memcpy(frame->registers, interpreter->aregs, sizeof(frame->registers));
  frame->return_pc = return_pc;
  frame->window_base = interpreter->window_base;
  return true;
}

static bool PopCall(XtensaInterpreter* interpreter) {
  if (interpreter->call_depth == 0) {
    interpreter->running = false;
    return false;
  }
  uint32_t result0 = ReadRegister(interpreter, 2);
  uint32_t result1 = ReadRegister(interpreter, 3);
  XtensaCallFrame* frame = &interpreter->calls[--interpreter->call_depth];
  memcpy(interpreter->aregs, frame->registers, sizeof(frame->registers));
  interpreter->window_base = frame->window_base;
  WriteRegister(interpreter, 10, result0);
  WriteRegister(interpreter, 11, result1);
  interpreter->pc = frame->return_pc;
  interpreter->window_start =
      interpreter->call_depth >= 8
          ? UINT64_MAX
          : ((1ull << (interpreter->call_depth + 1)) - 1);
  return true;
}

#define XTENSA_SETJMP_BREAK 0x11
#define XTENSA_LONGJMP_BREAK 0x12
#define XTENSA_SETJMP_REGS 256

static bool HandleSetJmp(XtensaInterpreter* interpreter) {
  uint32_t buf = ReadRegister(interpreter, 2);
  if (interpreter->call_depth == 0) {
    fprintf(stderr, "ESP32 setjmp with empty call stack\n");
    interpreter->running = false;
    interpreter->exit_code = 1;
    return false;
  }
  XtensaCallFrame* frame = &interpreter->calls[interpreter->call_depth - 1];
  uint32_t depth = (uint32_t)interpreter->call_depth;
  uint32_t window_base = frame->window_base;
  return WriteMemory(interpreter, buf, &depth, sizeof(depth)) &&
         WriteMemory(interpreter, buf + 4, &frame->return_pc,
                     sizeof(frame->return_pc)) &&
         WriteMemory(interpreter, buf + 8, &window_base, sizeof(window_base)) &&
         WriteMemory(interpreter, buf + 12, frame->registers,
                     XTENSA_SETJMP_REGS);
}

static bool HandleLongJmp(XtensaInterpreter* interpreter, uint32_t* next_pc) {
  uint32_t buf = ReadRegister(interpreter, 2);
  uint32_t value = ReadRegister(interpreter, 3);
  if (value == 0) {
    value = 1;
  }
  uint32_t depth = 0;
  uint32_t return_pc = 0;
  uint32_t window_base = 0;
  if (!ReadMemory(interpreter, buf, &depth, sizeof(depth)) ||
      !ReadMemory(interpreter, buf + 4, &return_pc, sizeof(return_pc)) ||
      !ReadMemory(interpreter, buf + 8, &window_base, sizeof(window_base))) {
    return false;
  }
  if (depth == 0 || depth > XTENSA_MAX_CALL_DEPTH) {
    fprintf(stderr, "ESP32 longjmp with invalid setjmp buffer\n");
    interpreter->running = false;
    interpreter->exit_code = 1;
    return false;
  }
  XtensaCallFrame* frame = &interpreter->calls[depth - 1];
  if (!ReadMemory(interpreter, buf + 12, frame->registers,
                  XTENSA_SETJMP_REGS)) {
    return false;
  }
  frame->return_pc = return_pc;
  frame->window_base = (uint8_t)window_base;
  interpreter->call_depth = depth;
  WriteRegister(interpreter, 2, value);
  if (!PopCall(interpreter)) {
    return false;
  }
  *next_pc = interpreter->pc;
  return true;
}

static void TraceInstruction(const XtensaInterpreter* interpreter,
                             const XtensaInstruction* instruction) {
  if (!interpreter->trace) {
    return;
  }
  fprintf(stderr,
          "%08" PRIx32 ": %-8s a%u, a%u, a%u, %" PRId32 " [rs=%08" PRIx32
          " rt=%08" PRIx32 "]\n",
          interpreter->pc, XtensaInstructionName(instruction->kind),
          instruction->rd, instruction->rs, instruction->rt,
          instruction->immediate, ReadRegister(interpreter, instruction->rs),
          ReadRegister(interpreter, instruction->rt));
}

static void HandleHostTrap(XtensaInterpreter* interpreter) {
  uint32_t service = ReadRegister(interpreter, 8);
  uint32_t a2 = ReadRegister(interpreter, 2);
  uint32_t a3 = ReadRegister(interpreter, 3);
  uint32_t a4 = ReadRegister(interpreter, 4);
  uint32_t a5 = ReadRegister(interpreter, 5);
  switch (service) {
    case DAVE_SYS_EXIT:
    case DAVE_SYS_EXIT_CLEAN:
      interpreter->exit_code = (int)a2;
      interpreter->running = false;
      return;
    case DAVE_SYS_ABORT:
      interpreter->exit_code = 134;
      interpreter->running = false;
      return;
    case DAVE_SYS_WRITE: {
      const void* data = GuestPointer(interpreter, a3, a4);
      WriteRegister(interpreter, 2,
                    data == NULL ? (uint32_t)-EFAULT
                                 : (uint32_t)write((int)a2, data, a4));
      return;
    }
    case DAVE_SYS_READ: {
      void* data = GuestPointer(interpreter, a3, a4);
      WriteRegister(
          interpreter, 2,
          data == NULL ? (uint32_t)-EFAULT : (uint32_t)read((int)a2, data, a4));
      return;
    }
    case DAVE_SYS_OPEN: {
      char path[PATH_MAX];
      int error = CopyGuestString(interpreter, a2, path, sizeof(path));
      WriteRegister(interpreter, 2,
                    error == 0 ? (uint32_t)open(path, (int)a3, (mode_t)a4)
                               : (uint32_t)-error);
      return;
    }
    case DAVE_SYS_CLOSE:
      WriteRegister(interpreter, 2, (uint32_t)close((int)a2));
      return;
    case DAVE_SYS_POLL: {
      nfds_t n = (nfds_t)a3;
      void* fds = GuestPointer(interpreter, a2, n * sizeof(struct pollfd));
      WriteRegister(interpreter, 2,
                    fds == NULL ? (uint32_t)-EFAULT
                                : (uint32_t)poll((struct pollfd*)fds, n,
                                                 (int)a4));
      return;
    }
    case DAVE_SYS_LSEEK:
      WriteRegister(interpreter, 2,
                    (uint32_t)lseek((int)a2, (off_t)(int32_t)a3, (int)a4));
      return;
    case DAVE_SYS_TIME:
      WriteRegister(interpreter, 2, (uint32_t)time(NULL));
      return;
    case DAVE_SYS_CLOCK:
    case DAVE_SYS_MONOTONIC_TIME:
    case DAVE_SYS_REALTIME_TIME: {
      struct timespec now;
      clockid_t clock_id =
          service == DAVE_SYS_REALTIME_TIME ? CLOCK_REALTIME : CLOCK_MONOTONIC;
      int status = clock_gettime(clock_id, &now);
      uint64_t nanos =
          (uint64_t)now.tv_sec * 1000000000ull + (uint64_t)now.tv_nsec;
      WriteRegister(interpreter, 2, status == 0 ? (uint32_t)nanos : UINT32_MAX);
      WriteRegister(interpreter, 3,
                    status == 0 ? (uint32_t)(nanos >> 32) : UINT32_MAX);
      return;
    }
    case DAVE_SYS_RANDOM_BYTES: {
      void* data = GuestPointer(interpreter, a2, a3);
      int fd = open("/dev/urandom", O_RDONLY);
      ssize_t result = data == NULL || fd < 0 ? -1 : read(fd, data, (size_t)a3);
      if (fd >= 0) {
        close(fd);
      }
      WriteRegister(interpreter, 2, (uint32_t)result);
      return;
    }
    default:
      fprintf(stderr,
              "unsupported ESP32 host service %" PRIu32 " (%" PRIu32
              ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 ")\n",
              service, a2, a3, a4, a5);
      interpreter->exit_code = 1;
      interpreter->running = false;
      return;
  }
}

bool XtensaInterpreterInit(XtensaInterpreter* interpreter, Loader* loader,
                           uint32_t entry, bool trace) {
  memset(interpreter, 0, sizeof(*interpreter));
  interpreter->loader = loader;
  interpreter->pc = entry;
  interpreter->trace = trace;
  interpreter->running = true;
  interpreter->stack = calloc(1, XTENSA_HOSTED_STACK_SIZE);
  if (interpreter->stack == NULL) {
    return false;
  }
  interpreter->window_start = 1;
  WriteRegister(interpreter, 1,
                XTENSA_HOSTED_STACK_BASE + XTENSA_HOSTED_STACK_SIZE);
  return true;
}

void XtensaInterpreterDestruct(XtensaInterpreter* interpreter) {
  free(interpreter->stack);
  interpreter->stack = NULL;
}

bool XtensaInterpreterStep(XtensaInterpreter* interpreter) {
  uint8_t bytes[3];
  XtensaInstruction instruction;
  if (!interpreter->running || !Fetch(interpreter, bytes) ||
      !XtensaDecode(bytes, sizeof(bytes), &instruction)) {
    if (interpreter->running) {
      fprintf(stderr,
              "unsupported Xtensa instruction at 0x%08" PRIx32
              ": %02x %02x %02x\n",
              interpreter->pc, bytes[0], bytes[1], bytes[2]);
      interpreter->exit_code = 1;
      interpreter->running = false;
    }
    return false;
  }
  TraceInstruction(interpreter, &instruction);
  uint32_t next_pc = interpreter->pc + instruction.size;
  uint32_t lhs = ReadRegister(interpreter, instruction.rs);
  uint32_t rhs = ReadRegister(interpreter, instruction.rt);
  uint32_t address;
  switch (instruction.kind) {
    case kXtensaAdd:
      WriteRegister(interpreter, instruction.rd, lhs + rhs);
      break;
    case kXtensaSub:
      WriteRegister(interpreter, instruction.rd, lhs - rhs);
      break;
    case kXtensaAnd:
      WriteRegister(interpreter, instruction.rd, lhs & rhs);
      break;
    case kXtensaOr:
      WriteRegister(interpreter, instruction.rd, lhs | rhs);
      break;
    case kXtensaXor:
      WriteRegister(interpreter, instruction.rd, lhs ^ rhs);
      break;
    case kXtensaNeg:
      WriteRegister(interpreter, instruction.rd, (uint32_t)-(int32_t)lhs);
      break;
    case kXtensaAddi:
      WriteRegister(interpreter, instruction.rd, lhs + instruction.immediate);
      break;
    case kXtensaMovi:
      WriteRegister(interpreter, instruction.rd,
                    (uint32_t)instruction.immediate);
      break;
    case kXtensaL8ui: {
      uint8_t value;
      address = lhs + instruction.immediate;
      if (!ReadMemory(interpreter, address, &value, sizeof(value)))
        return false;
      WriteRegister(interpreter, instruction.rd, value);
      break;
    }
    case kXtensaL16ui:
    case kXtensaL16si: {
      uint16_t value;
      address = lhs + instruction.immediate;
      if (!ReadMemory(interpreter, address, &value, sizeof(value)))
        return false;
      WriteRegister(interpreter, instruction.rd,
                    instruction.kind == kXtensaL16si
                        ? (uint32_t)(int32_t)(int16_t)value
                        : value);
      break;
    }
    case kXtensaL32i: {
      uint32_t value;
      address = lhs + instruction.immediate;
      if (!ReadMemory(interpreter, address, &value, sizeof(value)))
        return false;
      WriteRegister(interpreter, instruction.rd, value);
      break;
    }
    case kXtensaS8i: {
      uint8_t value = (uint8_t)ReadRegister(interpreter, instruction.rd);
      address = lhs + instruction.immediate;
      if (!WriteMemory(interpreter, address, &value, sizeof(value)))
        return false;
      break;
    }
    case kXtensaS16i: {
      uint16_t value = (uint16_t)ReadRegister(interpreter, instruction.rd);
      address = lhs + instruction.immediate;
      if (!WriteMemory(interpreter, address, &value, sizeof(value)))
        return false;
      break;
    }
    case kXtensaS32i: {
      uint32_t value = ReadRegister(interpreter, instruction.rd);
      address = lhs + instruction.immediate;
      if (!WriteMemory(interpreter, address, &value, sizeof(value)))
        return false;
      break;
    }
    case kXtensaBeq:
      if (lhs == rhs) next_pc = interpreter->pc + 4 + instruction.immediate;
      break;
    case kXtensaBne:
      if (lhs != rhs) next_pc = interpreter->pc + 4 + instruction.immediate;
      break;
    case kXtensaBlt:
      if ((int32_t)lhs < (int32_t)rhs)
        next_pc = interpreter->pc + 4 + instruction.immediate;
      break;
    case kXtensaBge:
      if ((int32_t)lhs >= (int32_t)rhs)
        next_pc = interpreter->pc + 4 + instruction.immediate;
      break;
    case kXtensaBltu:
      if (lhs < rhs) next_pc = interpreter->pc + 4 + instruction.immediate;
      break;
    case kXtensaBgeu:
      if (lhs >= rhs) next_pc = interpreter->pc + 4 + instruction.immediate;
      break;
    case kXtensaBeqz:
      if (lhs == 0) next_pc = interpreter->pc + 4 + instruction.immediate;
      break;
    case kXtensaBnez:
      if (lhs != 0) next_pc = interpreter->pc + 4 + instruction.immediate;
      break;
    case kXtensaBltz:
      if ((int32_t)lhs < 0)
        next_pc = interpreter->pc + 4 + instruction.immediate;
      break;
    case kXtensaBgez:
      if ((int32_t)lhs >= 0)
        next_pc = interpreter->pc + 4 + instruction.immediate;
      break;
    case kXtensaL32r: {
      uint32_t value;
      address = ((interpreter->pc + 3) & ~3u) + instruction.immediate;
      if (!ReadMemory(interpreter, address, &value, sizeof(value)))
        return false;
      WriteRegister(interpreter, instruction.rd, value);
      break;
    }
    case kXtensaJ:
      next_pc = interpreter->pc + 4 + instruction.immediate;
      break;
    case kXtensaJx:
      next_pc = lhs;
      break;
    case kXtensaCall8:
      if (!PushCall(interpreter, next_pc)) return false;
      WriteRegister(interpreter, 8, 0x80000000u | (next_pc & 0x3fffffffu));
      interpreter->call_increment = 8;
      next_pc = (interpreter->pc & ~3u) + 4 + instruction.immediate;
      break;
    case kXtensaCallx8:
      if (!PushCall(interpreter, next_pc)) return false;
      WriteRegister(interpreter, 8, 0x80000000u | (next_pc & 0x3fffffffu));
      interpreter->call_increment = 8;
      next_pc = lhs;
      break;
    case kXtensaEntry: {
      uint32_t old_sp = lhs;
      unsigned physical = (interpreter->window_base +
                           interpreter->call_increment + instruction.rs) &
                          63;
      interpreter->aregs[physical] = old_sp - instruction.immediate;
      interpreter->window_base =
          (interpreter->window_base + interpreter->call_increment) & 63;
      interpreter->window_start |= 1ull << (interpreter->window_base / 4);
      interpreter->call_increment = 0;
      break;
    }
    case kXtensaRetw:
      if (!PopCall(interpreter)) return false;
      next_pc = interpreter->pc;
      break;
    case kXtensaBreak:
      if (instruction.immediate == XTENSA_SETJMP_BREAK) {
        if (!HandleSetJmp(interpreter)) return false;
      } else if (instruction.immediate == XTENSA_LONGJMP_BREAK) {
        if (!HandleLongJmp(interpreter, &next_pc)) return false;
      } else {
        HandleHostTrap(interpreter);
      }
      break;
    case kXtensaSlli:
      WriteRegister(interpreter, instruction.rd,
                    lhs << (instruction.immediate & 31));
      break;
    case kXtensaSrai:
      WriteRegister(interpreter, instruction.rd,
                    (uint32_t)((int32_t)lhs >> (instruction.immediate & 31)));
      break;
    case kXtensaSrli:
      WriteRegister(interpreter, instruction.rd,
                    lhs >> (instruction.immediate & 31));
      break;
    case kXtensaSsl:
    case kXtensaSsr:
      interpreter->sar = lhs & 31;
      break;
    case kXtensaSll:
      WriteRegister(interpreter, instruction.rd,
                    lhs << (interpreter->sar & 31));
      break;
    case kXtensaSrl:
      WriteRegister(interpreter, instruction.rd,
                    lhs >> (interpreter->sar & 31));
      break;
    case kXtensaSra:
      WriteRegister(interpreter, instruction.rd,
                    (uint32_t)((int32_t)lhs >> (interpreter->sar & 31)));
      break;
    case kXtensaMull:
      WriteRegister(interpreter, instruction.rd, lhs * rhs);
      break;
    case kXtensaQuou:
      WriteRegister(interpreter, instruction.rd,
                    rhs == 0 ? UINT32_MAX : lhs / rhs);
      break;
    case kXtensaQuos:
      WriteRegister(interpreter, instruction.rd,
                    rhs == 0 ? UINT32_MAX
                             : ((lhs == 0x80000000u && rhs == UINT32_MAX)
                                    ? 0x80000000u
                                    : (uint32_t)((int32_t)lhs / (int32_t)rhs)));
      break;
    case kXtensaRemu:
      WriteRegister(interpreter, instruction.rd, rhs == 0 ? lhs : lhs % rhs);
      break;
    case kXtensaRems:
      WriteRegister(interpreter, instruction.rd,
                    rhs == 0 ? lhs
                             : ((lhs == 0x80000000u && rhs == UINT32_MAX)
                                    ? 0
                                    : (uint32_t)((int32_t)lhs % (int32_t)rhs)));
      break;
    case kXtensaInvalid:
      return false;
  }
  interpreter->pc = next_pc;
  interpreter->steps++;
  return interpreter->running;
}

int XtensaInterpreterRun(XtensaInterpreter* interpreter) {
  while (interpreter->running) {
    XtensaInterpreterStep(interpreter);
  }
  return interpreter->exit_code;
}
