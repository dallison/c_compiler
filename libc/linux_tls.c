#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <syscall.h>

enum {
  DAVE_AT_PHDR = 3,
  DAVE_AT_PHENT = 4,
  DAVE_AT_PHNUM = 5,
  DAVE_PT_TLS = 7,
};

typedef struct {
  uint32_t type;
  uint32_t flags;
  uint64_t offset;
  uint64_t virtual_address;
  uint64_t physical_address;
  uint64_t file_size;
  uint64_t memory_size;
  uint64_t alignment;
} DaveELF64ProgramHeader;

typedef struct {
  uint32_t type;
  uint32_t offset;
  uint32_t virtual_address;
  uint32_t physical_address;
  uint32_t file_size;
  uint32_t memory_size;
  uint32_t flags;
  uint32_t alignment;
} DaveELF32ProgramHeader;

static uintptr_t tls_image_address;
static uintptr_t tls_template_address;
static size_t tls_file_size;
static size_t tls_memory_size;

#if defined(__arm__) || defined(__x86_64__)
#define DAVE_TLS_TCB_SIZE 8
#else
#define DAVE_TLS_TCB_SIZE 16
#endif

void __davecc_linux_tls_copy(void* thread_pointer) {
#if defined(__x86_64__)
  *(uintptr_t*)thread_pointer = (uintptr_t)thread_pointer;
#endif
  unsigned char* destination =
      (unsigned char*)thread_pointer + DAVE_TLS_TCB_SIZE;
  memset(destination, 0, tls_memory_size);
  uintptr_t source =
      tls_template_address != 0 ? tls_template_address : tls_image_address;
  if (tls_file_size != 0 && source != 0) {
    memcpy(destination, (const void*)source, tls_file_size);
  }
}

void __davecc_linux_tls_init(void* thread_pointer, void* initial_stack) {
  uintptr_t* cursor = (uintptr_t*)initial_stack;
  uintptr_t argument_count = *cursor++;
  cursor += argument_count + 1;
  while (*cursor++ != 0) {}

  uintptr_t program_headers = 0;
  uintptr_t program_header_size = 0;
  uintptr_t program_header_count = 0;
  while (cursor[0] != 0) {
    uintptr_t type = cursor[0];
    uintptr_t value = cursor[1];
    if (type == DAVE_AT_PHDR) program_headers = value;
    if (type == DAVE_AT_PHENT) program_header_size = value;
    if (type == DAVE_AT_PHNUM) program_header_count = value;
    cursor += 2;
  }

  for (uintptr_t index = 0; index < program_header_count; ++index) {
    const unsigned char* header =
        (const unsigned char*)program_headers + index * program_header_size;
#if defined(__arm__)
    const DaveELF32ProgramHeader* program =
        (const DaveELF32ProgramHeader*)header;
#else
    const DaveELF64ProgramHeader* program =
        (const DaveELF64ProgramHeader*)header;
#endif
    if (program->type == DAVE_PT_TLS) {
      tls_image_address = program->virtual_address;
      tls_file_size = program->file_size;
      tls_memory_size = program->memory_size;
      break;
    }
  }
  __davecc_linux_tls_copy(thread_pointer);
  if (tls_memory_size != 0) {
#if defined(__arm__)
    const int mmap_syscall = SYS_mmap2;
#else
    const int mmap_syscall = SYS_mmap;
#endif
    void* template_memory =
        (void*)syscall(mmap_syscall, 0, tls_memory_size, 3, 0x22, -1, 0);
    if (template_memory != (void*)-1) {
      memset(template_memory, 0, tls_memory_size);
      if (tls_file_size != 0 && tls_image_address != 0) {
        memcpy(template_memory, (const void*)tls_image_address, tls_file_size);
      }
      tls_template_address = (uintptr_t)template_memory;
    }
  }
}
