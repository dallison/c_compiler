//
//  x86_64_syscalls.c
//  x86_64_interpreter
//

#include "x86_64_syscalls.h"
#include "x86_64_interpreter.h"
#include "loader_dynamic.h"
#include "loader.h"
#include "elf.h"
#include "x86_64_machine.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool GuestAddressOk(Loader* loader, uint64_t addr, size_t size) {
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    uint64_t start = (uint64_t)(uintptr_t)region->address;
    uint64_t end = start + (uint64_t)region->length;
    if (addr >= start && addr + size <= end) {
      return true;
    }
  }
  return false;
}

static bool SectionAddressAndSize(LoadedDynamicLibrary* lib, const char* name,
                                  uint64_t* addr, uint64_t* size) {
  if (lib->header == NULL || lib->section_headers == NULL || lib->addr == 0) {
    return false;
  }
  const ELFSectionHeader* shstr_sh =
      &lib->section_headers[lib->header->shstrndx];
  const char* shstrtab = (const char*)lib->addr + shstr_sh->offset;
  for (int i = 0; i < lib->header->shnum; i++) {
    const ELFSectionHeader* sh = &lib->section_headers[i];
    if (strcmp(shstrtab + sh->name, name) != 0) {
      continue;
    }
    *addr = sh->addr;
    *size = sh->size;
    return true;
  }
  return false;
}

static bool ReadRelaPltEntry(LoadedDynamicLibrary* lib, int64_t index,
                             ELFRelocation* out) {
  if (lib->header == NULL || lib->section_headers == NULL || lib->fd < 0) {
    return false;
  }
  const ELFSectionHeader* shstr_sh =
      &lib->section_headers[lib->header->shstrndx];
  const char* shstrtab = (const char*)lib->addr + shstr_sh->offset;
  for (int i = 0; i < lib->header->shnum; i++) {
    const ELFSectionHeader* sh = &lib->section_headers[i];
    if (strcmp(shstrtab + sh->name, ".rela.plt") != 0) {
      continue;
    }
    off_t file_offset =
        (off_t)(sh->offset + (uint64_t)index * sizeof(ELFRelocation));
    return pread(lib->fd, out, sizeof(*out), file_offset) ==
           (ssize_t)sizeof(*out);
  }
  return false;
}

static bool GotPointsIntoPlt(Loader* loader, LoadedDynamicLibrary* lib,
                             uint64_t got_value) {
  uint64_t plt_linked = 0;
  uint64_t plt_size = 0;
  if (!SectionAddressAndSize(lib, ".plt", &plt_linked, &plt_size)) {
    return false;
  }
  uint64_t plt_runtime = 0;
  if (!LoaderLinkedAddressToRuntime(loader, lib, plt_linked, &plt_runtime)) {
    return false;
  }
  return got_value >= plt_runtime && got_value < plt_runtime + plt_size;
}

static bool ResolveByPltIndex(X86_64Interpreter* interpreter,
                              LoadedDynamicLibrary* lib, int64_t index,
                              bool* rip_updated) {
  ELFRelocation reloc;
  if (!ReadRelaPltEntry(lib, index, &reloc)) {
    return false;
  }
  if (ELF_R_TYPE(reloc.info) != R_X86_64_JUMP_SLOT) {
    return false;
  }
  int32_t sym_index = ELF_R_SYM(reloc.info);
  const char* sym_name = lib->dynstr + lib->dynsym[sym_index].name;
  const ELFSymbol* symbol;
  LoadedDynamicLibrary* found_lib;
  bool ok = DynamicLoaderFindSymbol(&lib->loader->loaded_libraries, sym_name,
                                    &symbol, &found_lib);
  if (!ok) {
    fprintf(stderr, "Undefined symbol %s\n", sym_name);
    exit(1);
  }
  uint64_t symbol_address = 0;
  if (!LoaderLinkedAddressToRuntime(lib->loader, found_lib, symbol->value,
                                    &symbol_address)) {
    fprintf(stderr, "Cannot translate symbol %s\n", sym_name);
    exit(1);
  }
  uint64_t got_offset = 0;
  if (!LoaderLinkedAddressToRuntime(lib->loader, lib, reloc.offset,
                                    &got_offset)) {
    fprintf(stderr, "Cannot translate GOT slot for %s\n", sym_name);
    exit(1);
  }
  *(uint64_t*)(uintptr_t)got_offset = symbol_address;
  interpreter->rip = symbol_address;
  *rip_updated = true;
  return true;
}

static void ResolveAndFixupSymbol(X86_64Interpreter* interpreter,
                                  bool* rip_updated) {
  LoadedDynamicLibrary* lib =
      (LoadedDynamicLibrary*)interpreter->iregs[X86_REG_RDI];
  int64_t index = (int64_t)interpreter->iregs[X86_REG_RSI];
  if (lib == NULL || index < 0) {
    if (GuestAddressOk(interpreter->loader, interpreter->rsp, 16)) {
      lib = *(LoadedDynamicLibrary**)(uintptr_t)interpreter->rsp;
      index = *(int64_t*)(uintptr_t)(interpreter->rsp + 8);
    }
  }
  if (lib != NULL && index >= 0 &&
      ResolveByPltIndex(interpreter, lib, index, rip_updated)) {
    return;
  }

  lib = (LoadedDynamicLibrary*)interpreter->iregs[X86_REG_RDI];
  if (lib == NULL && GuestAddressOk(interpreter->loader, interpreter->rsp, 8)) {
    lib = *(LoadedDynamicLibrary**)(uintptr_t)interpreter->rsp;
  }
  if (lib == NULL) {
    fprintf(stderr, "Undefined symbol\n");
    exit(1);
  }

  int64_t plt_rel_size = 0;
  const DynamicSection* section = lib->dynamic;
  if (section != NULL) {
    for (size_t i = 0; section->entries[i].tag != DT(null); i++) {
      if (section->entries[i].tag == DT(pltrelsz)) {
        plt_rel_size = section->entries[i].un.val;
      }
    }
  }
  int64_t num_relocations = plt_rel_size / (int64_t)sizeof(ELFRelocation);

  const ELFRelocation* reloc = NULL;
  ELFRelocation file_reloc;
  for (int64_t i = 0; i < num_relocations; i++) {
    if (!ReadRelaPltEntry(lib, i, &file_reloc)) {
      continue;
    }
    if (ELF_R_TYPE(file_reloc.info) != R_X86_64_JUMP_SLOT) {
      continue;
    }
    uint64_t got_slot = 0;
    if (!LoaderLinkedAddressToRuntime(lib->loader, lib, file_reloc.offset,
                                      &got_slot)) {
      continue;
    }
    uint64_t got_value = *(uint64_t*)(uintptr_t)got_slot;
    if (GotPointsIntoPlt(lib->loader, lib, got_value)) {
      reloc = &file_reloc;
      break;
    }
  }
  if (reloc == NULL) {
    fprintf(stderr, "Undefined symbol\n");
    exit(1);
  }

  int32_t sym_index = ELF_R_SYM(reloc->info);
  const char* sym_name = lib->dynstr + lib->dynsym[sym_index].name;
  const ELFSymbol* symbol;
  LoadedDynamicLibrary* found_lib;
  bool ok = DynamicLoaderFindSymbol(&lib->loader->loaded_libraries, sym_name,
                                    &symbol, &found_lib);
  if (!ok) {
    fprintf(stderr, "Undefined symbol %s\n", sym_name);
    exit(1);
  }
  uint64_t symbol_address = 0;
  if (!LoaderLinkedAddressToRuntime(lib->loader, found_lib, symbol->value,
                                    &symbol_address)) {
    fprintf(stderr, "Cannot translate symbol %s\n", sym_name);
    exit(1);
  }
  uint64_t got_offset = 0;
  if (!LoaderLinkedAddressToRuntime(lib->loader, lib, reloc->offset,
                                    &got_offset)) {
    fprintf(stderr, "Cannot translate GOT slot for %s\n", sym_name);
    exit(1);
  }
  *(uint64_t*)(uintptr_t)got_offset = symbol_address;
  interpreter->rip = symbol_address;
  *rip_updated = true;
}

int64_t X86_64HandleSyscall(X86_64Interpreter* interpreter, int64_t number,
                            int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                            int64_t a4, int64_t a5) {
  (void)a4;
  (void)a5;
  switch (number) {
    case X86_64_SYSCALL_HALT:
    case X86_64_SYSCALL_EXIT:
      exit((int)a0);
      break;
    case X86_64_SYSCALL_OPEN:
      return open((const char*)(uintptr_t)a0, (int)a1, (mode_t)a2);
    case X86_64_SYSCALL_CLOSE:
      return close((int)a0);
    case X86_64_SYSCALL_READ:
      return read((int)a0, (void*)(uintptr_t)a1, (size_t)a2);
    case X86_64_SYSCALL_WRITE:
      return write((int)a0, (const void*)(uintptr_t)a1, (size_t)a2);
    case X86_64_SYSCALL_LSEEK:
      return lseek((int)a0, (off_t)a1, (int)a2);
    case X86_64_SYSCALL_MALLOC:
      return (int64_t)(uintptr_t)malloc((size_t)a0);
    case X86_64_SYSCALL_REALLOC:
      return (int64_t)(uintptr_t)realloc((void*)(uintptr_t)a0, (size_t)a1);
    case X86_64_SYSCALL_FREE:
      free((void*)(uintptr_t)a0);
      return 0;
    case X86_64_SYSCALL_ABORT:
      abort();
      break;
    case X86_64_SYSCALL_RESOLVE: {
      bool rip_updated = false;
      ResolveAndFixupSymbol(interpreter, &rip_updated);
      interpreter->rip_updated = rip_updated;
      return 0;
    }
    default:
      fprintf(stderr, "Unknown x86_64 syscall %lld\n", (long long)number);
      exit(1);
  }
  return -1;
}
