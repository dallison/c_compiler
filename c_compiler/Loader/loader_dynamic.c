//
//  dynamic.c
//  common_utils
//
//  Created by David Allison on 2/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include "loader_dynamic.h"
#include "loader_arch.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "loader.h"

bool print_libraries_only;

void DynamicLibraryRegistryInit(DynamicLibraryRegistry* reg) {
  MapInitForStringKeys(&reg->loaded_libraries);
  VectorInit(&reg->search);
}

void DynamicLibraryRegistryDestruct(DynamicLibraryRegistry* reg) {
  VectorDestructWithContents(&reg->search,
                             (VectorElementDestructor)LoadedDynamicLibraryDestruct);
  MapDestruct(&reg->loaded_libraries);
}


// Align the given value to a power of 2 alignment.
static uint64_t Align(uint64_t v, uint64_t alignment) {
  return (v + (alignment - 1)) & ~(alignment - 1);
}

static MappedSegment* NewMappedSegment(void* addr, size_t length) {
  MappedSegment* seg = malloc(sizeof(MappedSegment));
  seg->address = addr;
  seg->length = length;
  return seg;
}

static void MappedSegmentDestruct(MappedSegment* s) {
  munmap(s->address, s->length);
}

// Given the name of a dynamic libary, find it in the dynamic linker's
// library map.
LoadedDynamicLibrary* DynamicLoaderFindLibrary(DynamicLibraryRegistry* registry,
                                               String* filename) {
  return MapFindPointerKey(&registry->loaded_libraries, filename);
}

bool DynamicLoaderFindSymbol(DynamicLibraryRegistry* registry,
                                             const char* name,
                             const ELFSymbol** symbol,
                             LoadedDynamicLibrary** library) {
  for (size_t i = 0; i < registry->search.length; i++) {
    LoadedDynamicLibrary* lib = registry->search.value.p[i];
    const ELFSymbol* sym = LoadedDynamicLibraryFindSymbol(lib, name);
    if (sym != NULL) {
      *symbol = sym;
      *library = lib;
      return true;
    }
  }
  return false;
}

bool DynamicLoaderLookupSymbolByAddress(DynamicLibraryRegistry* registry,
                                        uint64_t address,
                                        const char** name,
                                        uint64_t* start,
                                        uint64_t* length) {
  for (size_t i = 0; i < registry->search.length; i++) {
    LoadedDynamicLibrary* lib = registry->search.value.p[i];
    bool found = LoadedDynamicLibraryLookupSymbolByAddress(lib,
                                                           address,
                                                           name,
                                                           start,
                                                           length);
    if (found) {
      return true;
    }
  }
  return false;
}

void DynamicLibraryRegistryInsert(DynamicLibraryRegistry* reg,
                                  LoadedDynamicLibrary* lib) {
  MapKeyValue kv;
  kv.key.p = &lib->filename;
  kv.value.p = lib;
  MapInsert(&reg->loaded_libraries, kv);
  
  // TODO: use the symbolic flag to insert at the beginning
  VectorAppend(&reg->search, lib);

}

const void* DynamicLoaderFindDynamicSectionAddressEntry(
                                                  const LoadedDynamicLibrary* lib,
                                                  ELFDynamicTag tag) {
  const DynamicSection* dynamic = lib->dynamic;
  for (size_t i = 0; dynamic->entries[i].tag != DT(null); i++) {
    if (dynamic->entries[i].tag == tag) {
      return (const void*)(lib->load_address + dynamic->entries[i].un.ptr);
    }
  }
  return NULL;
}

const int64_t DynamicLoaderFindDynamicSectionOffsetEntry(
                                                        const LoadedDynamicLibrary* lib,
                                                        ELFDynamicTag tag) {
  const DynamicSection* dynamic = lib->dynamic;
  for (size_t i = 0; dynamic->entries[i].tag != DT(null); i++) {
    if (dynamic->entries[i].tag == tag) {
      return dynamic->entries[i].un.val;
    }
  }
  return -1;
}

// Calculate GNU hash code for a symbol name.  This isn't documented
// anywhere except articles on the internet and the source code.
// See the article https://flapenguin.me/2017/05/10/elf-lookup-dt-gnu-hash/
// for a good explanation.
// Another good paper is https://www.akkadia.org/drepper/dsohowto.pdf
uint32_t DynamicLoaderGNUHash(const char* name) {
  uint32_t hash = 5381;
  while (*name != '\0') {
    hash = (hash << 5) + hash + *name++;
  }
  return hash;
}

// Set the bits corresponding to bits 5:0 and bits 31:26 of the hash value.
uint64_t DynamicLoaderBloomBits64(uint32_t hash) {
  return (1LL << (hash % 64)) |
    (1LL << ((hash >> 26) % 64));
}


// Load the DT_NEEDED libraries into the dynamic linker.  These
// are not added to the DT_NEEDED list of the output.
static void LoadNeededLibraries(DynamicLibraryRegistry* registry,
                                LoadedDynamicLibrary* lib,
                                Vector* search_path,
                                uint64_t* load_addr) {
  const DynamicSection* section = lib->dynamic;
  const char* strtab = DynamicLoaderFindDynamicSectionAddressEntry(lib, DT(strtab));
  if (strtab == NULL) {
    return;
  }
  String libname;
  StringInit(&libname, NULL);
  
  // All new libraries.  Contains pointers to LoadedDynamicLibrary but does
  // now own them.
  Vector new_libraries;
  VectorInit(&new_libraries);
  
  // Go through all the NEEDED libraries and see if they are already loaded.
  // If they aren't create the LoadedDynamicLibrary and add it to the
  // registry.
  for (size_t i = 0; section->entries[i].tag != DT(null); i++) {
    if (section->entries[i].tag == DT(needed)) {
      StringSet(&libname, strtab + section->entries[i].un.val);
      LoadedDynamicLibrary* dep = DynamicLoaderFindLibrary(registry, &libname);
      if (dep == NULL) {
        LoadedDynamicLibrary* new_lib = NewLoadedDynamicLibrary(libname.value,
                                                                lib->loader);
        DynamicLibraryRegistryInsert(registry, new_lib);
        VectorAppend(&new_libraries, new_lib);
      }
    }
  }
  
  // Now load all the newly created libraries - the ones that are not
  // already loaded.
  for (size_t i = 0; i < new_libraries.length; i++) {
    LoadedDynamicLibrary* lib = new_libraries.value.p[i];
    LoadedDynamicLibraryLoad(lib, registry, search_path, load_addr);
  }
  
  StringDestruct(&libname);
  VectorDestruct(&new_libraries);
}

// Find a dynamic library file by searching for it in the given
// paths.
// If the file is absolute (rooted), it looks for it as a file.
// This looks in the runtime search paths first (also called the RPATH)
// Then it looks in the static paths (LD_LIBRARY_PATH)
static bool FindDynamicLibraryFile(Vector* static_search_path,
                                   Vector* runtime_search_path,
                                   const char* filename,
                                   String* pathname) {
  struct stat st;
  // Check as full path.
  if (stat(filename, &st) == 0) {
    StringSet(pathname, filename);
    return true;
  }
  
  // The search uses the environment variable LD_LIBRARY_PATH first
  // then the runtime_search_paths, which is set from DT_RUNPATH
  // in the dynamic section.
  
  // Search for the file in the static search path.
  for (size_t i = 0; i < static_search_path->length; i++) {
    String* path = static_search_path->value.p[i];
    StringClear(pathname);
    StringPrintf(pathname, "%s/%s", path->value, filename);
    if (stat(pathname->value, &st) == 0) {
      // Found file at the path.
      return true;
    }
  }
  
  // Search runtime path if is is present.
  if (runtime_search_path != NULL) {
    for (size_t i = 0; i < runtime_search_path->length; i++) {
      String* path = runtime_search_path->value.p[i];
      StringClear(pathname);
      StringPrintf(pathname, "%s/%s", path->value, filename);
      if (stat(pathname->value, &st) == 0) {
        // Found file at the path.
        return true;
      }
    }
  }
  
  return false;
}

// Add the load address to all pointer values in the dynamic section.
static void RelocateDynamicSection(LoadedDynamicLibrary* lib) {
  DynamicSection* section = (DynamicSection*)lib->dynamic;
  if (section == NULL) {
    return;
  }
  for (size_t i = 0; section->entries[i].tag != DT(null); i++) {
    printf("%llx\n", section->entries[i].tag);
    switch (section->entries[i].tag) {
      case DT(symtab):
      case DT(strtab):
      case DT(rela):
      case DT(jmprel):
      case DT(pltgot):
        section->entries[i].un.val += lib->load_address;
        break;
      default:
        break;
    }
  }
}

static void PerformDynamicRelocations(Loader* loader, LoadedDynamicLibrary* lib,
                                      DynamicLibraryRegistry* loaded_libraries,
                                      bool lazy) {
  // All the information about how to perform dynamic relocations is
  // held in the .dynamic section, which is mapped in the DYNAMIC
  // segment.
  const DynamicSection* section = lib->dynamic;
  if (section == NULL) {
    return;
  }
  // Symbol and string tables;
  const ELFSymbol* symtab = NULL;
  const char* strtab = NULL;
  
  // Data relocations in GOT.
  ELFRelocation* data_relocations = NULL;
  
  // PLT relocations in GOT.
  ELFRelocation* plt_relocations = NULL;
  
  // Address of PLT entries in GOT.
  void* pltgot = NULL;
  int64_t reloc_type = DT(rela);
  int64_t rela_size = 0;
  int64_t rela_entsize = 0;
  int64_t rela_count = 0;
  int64_t plt_rel_size = 0;
  for (size_t i = 0; section->entries[i].tag != DT(null); i++) {
    switch (section->entries[i].tag) {
      case DT(symtab):
        symtab = (const ELFSymbol*)(section->entries[i].un.ptr);
        break;
      case DT(strtab):
        strtab = (const char*)(section->entries[i].un.ptr);
        break;
      case DT(rela):
        data_relocations = (ELFRelocation*)(section->entries[i].un.ptr);
        break;
      case DT(jmprel):
        plt_relocations = (ELFRelocation*)(section->entries[i].un.ptr);
        break;
      case DT(pltgot):
        pltgot = (void*)section->entries[i].un.ptr;
        break;
      case DT(pltrel):
        reloc_type = section->entries[i].un.val;
        break;
      case DT(relasz):
        rela_size = section->entries[i].un.val;
        break;
      case DT(relacount):
        rela_count= section->entries[i].un.val;
        break;
      case DT(relaent):
        rela_entsize = section->entries[i].un.val;
        break;
      case DT(pltrelsz):
        plt_rel_size = section->entries[i].un.val;
        break;
    }
  }
  if (symtab == NULL || strtab == NULL) {
    return;
  }
  
  // The first n entries in the PLTGOT contain architecture specific resolver
  // data that is set by a function provided by the user of the loader.
  // NOTE: the naming here is strange.  The section is called .got.plt
  // but the dynamic entry is called DT_PLTGOT.  Someone should make up
  // their mind.
  loader->arch->init_got_plt(lib, loader->arch_data);
  
  // Data relocations refer to symbols so they need to be set to the
  // symbol address.
  //
  // All relocations are relative to the load address of the whole
  // library.
  if (data_relocations != NULL) {
    int64_t num_relocations = 0;
    if (rela_count > 0) {
      num_relocations = rela_count;
    } else if (rela_size > 0 && rela_entsize > 0) {
      num_relocations = rela_size / rela_entsize;
    }
    
    for (int64_t i = 0; i < num_relocations; i++) {
      ELFRelocation* reloc = &data_relocations[i];
      int32_t sym_index = ELF_R_SYM(reloc->info);
      const char* sym_name = strtab + symtab[sym_index].name;
      const ELFSymbol* symbol = NULL;
      LoadedDynamicLibrary* found_lib = NULL;
      DynamicLoaderFindSymbol(loaded_libraries,
                                      sym_name, &symbol,
                                      &found_lib);
      char* target_address = (char*)lib->load_address + reloc->offset;
      loader->arch->apply_got_data_relocation(found_lib, reloc,
                                              symbol, sym_name,
                                              target_address);
      
    }
  }
  
  // In this phase, PLT relocations just need to have the load address
  // added to them since they already contain the offset of the
  // PLT entry, relative to the start of the library.
  if (plt_relocations != NULL && plt_rel_size != 0) {
    // In this loader, the default relocation type is RELA, so if the DT(pltrel)
    // (the relocation type) is not DT(rela) we subtract the addend size from
    // the size of ELFRelocation.
    int64_t num_relocations = plt_rel_size /
        (reloc_type != DT(rela) ?
            sizeof(ELFRelocation) - sizeof(ELF_Sxword) :
            sizeof(ELFRelocation));
    
    for (int64_t i = 0; i < num_relocations; i++) {
      ELFRelocation* reloc = &plt_relocations[i];
      char* target_address = (char*)lib->load_address + reloc->offset;
      int32_t sym_index = ELF_R_SYM(reloc->info);
      const char* sym_name = strtab + symtab[sym_index].name;
      const ELFSymbol* symbol = NULL;
      LoadedDynamicLibrary* found_lib = lib;
      if (!lazy) {
        DynamicLoaderFindSymbol(loaded_libraries,
                              sym_name, &symbol,
                              &found_lib);
      }
      loader->arch->apply_got_plt_relocation(found_lib, reloc,
                                             symbol, sym_name,
                                             target_address, lazy);
    }
  }
}


// Map a dynamic library into memory, returning the fd used to
// access the file (or -1 if things go badly).
// Set *addr to the address of the library and
// *length to the length of the memory mapped.
static int MapDynamicLibraryHeader(String* filename,
                             const void** addr, size_t* length) {
  // Open the file.
  int fd = open(filename->value, O_RDONLY);
  if (fd < 0) {
    return fd;
  }
  
  // Read the header from the file.  This will allow us to find
  // how much memory we need to map in to find the program headers
  // and section headers.
  ELFHeader header;
  ssize_t n = read(fd, (char*)&header, sizeof(header));
  if (n != sizeof(header)) {
    close(fd);
    return -1;
  }
  int page_size = (int)sysconf(_SC_PAGESIZE);
  off_t end_program_headers = header.phoff + header.phnum * header.phentsize;
  off_t end_section_headers = header.shoff + header.shnum * header.shentsize;
  off_t max_offset = end_program_headers > end_section_headers ?
          end_program_headers :
          end_section_headers;
  
  off_t map_size = Align(max_offset,
                        page_size);
  *length = map_size;
  
  // Map enough of the file into memory to allow us to read the
  // program headers and section heders.  Let the OS choose the address to map
  // the file at.
  *addr = mmap(NULL, map_size, PROT_READ, MAP_PRIVATE, fd,
               0);
  
  // Now we have the file mapped into memory at address 'addr'.  We can
  // access this memory directly.  It is mapped read-only so we can't
  // write to it.
  
  if (*addr == MAP_FAILED) {
    return -1;
  }
  return fd;
}

// Map the whole dynamic library.  This is used in the linker when it's
// loading a library.  It doesn't need to map the segments at a certain
// address and only needs the dynamic symbol table.
static int MapWholeDynamicLibrary(String* filename,
                                   const void** addr, size_t* length) {
  struct stat st;
  int e = stat(filename->value, &st);
  if (e != 0) {
    return -1;
  }
  *length = st.st_size;
  
  // Open the file.
  int fd = open(filename->value, O_RDONLY);
  if (fd < 0) {
    return fd;
  }
  
  
  // Map enough of the file into memory to allow us to read the
  // program headers and section heders.  Let the OS choose the address to map
  // the file at.
  *addr = mmap(NULL, *length, PROT_READ, MAP_PRIVATE, fd,
               0);
  
  // Now we have the file mapped into memory at address 'addr'.  We can
  // access this memory directly.  It is mapped read-only so we can't
  // write to it.
  
  if (*addr == MAP_FAILED) {
    return -1;
  }
  return fd;
}

// Map in the memory for the symbol table and string table.  The address
// will be chosen by the OS.
static void MapSymbolTable(LoadedDynamicLibrary* lib,
                           const ELFSectionHeader* symtab,
                           const ELFSectionHeader* strtab) {
  int64_t start;
  int64_t end;
  if (symtab->offset < strtab->offset) {
    start = symtab->offset;
    end = strtab->offset + strtab->size;
  } else {
    start = strtab->offset;
    end = symtab->offset + symtab->size;
  }
  int64_t page_size = sysconf(_SC_PAGESIZE);
  int64_t page_mask = page_size - 1;
  int64_t delta = start & page_mask;    // From mapped address to start.
  int64_t start_offset = start & ~page_mask;
  int64_t length = Align(end - start_offset, page_size);
  void* addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, lib->fd, start_offset);
  if (addr == MAP_FAILED) {
    return;
  }
  lib->symtab = (const ELFSymbol*)((char*)addr + delta + (symtab->offset - start));
  lib->strtab = (const char*)addr + delta + (strtab->offset - start);
  VectorAppend(&lib->mapped_segments, NewMappedSegment(addr, length));
}

static void FindAndLoadSymbolTable(LoadedDynamicLibrary* lib) {
  // Find the regular symbol table and string table so that we
  // can lookup symbols by address.  This is useful for printing
  // the location of instructions.
  // The only way to do this is to search the sections for the
  // SHT(symtab).
  for (int i = 0; i < lib->header->shnum; i++) {
    const ELFSectionHeader* section = &lib->section_headers[i];
    if (section->type == SHT(symtab)) {
      // Found symbol table section.  Its link field is the section
      // index of the string table.
      
      lib->num_symtab_symbols = section->size / section->entsize;
      const ELFSectionHeader* strtab = &lib->section_headers[section->link];
      MapSymbolTable(lib, section, strtab);
      break;
    }
  }
}

static bool LoadSegments(LoadedDynamicLibrary* lib, uint64_t *load_address) {
  if (load_address != NULL) {
    // Most of the file data is located in the PT_LOAD segments.  This
    // includes the symbol tables. The PT_DYNAMIC segment also needs
    // to be loaded.
    if (lib->header->type == ET(exec)) {
      // Executable file.  The load address is as specified in
      // the file.
      lib->load_address = LoadedDynamicLibraryLoadSegments(lib, NULL);
    } else if (lib->header->type == ET(dyn)){
      lib->load_address = LoadedDynamicLibraryLoadSegments(lib, load_address);
    } else {
      // Unknown ELF file type.
      LoadedDynamicLibraryDelete(lib);
      return false;
    }
  } else {
    // Load address is the address of the whole library.
    lib->load_address = (uint64_t)lib->addr;
  }
  return true;
}

static bool FindDynamicSection(LoadedDynamicLibrary* lib,
                               uint64_t* load_address) {
  for (int i = 0; i < lib->header->phnum; i++) {
    if (lib->program_headers[i].type == PT(dynamic)) {
      if (load_address == NULL) {
        // If we are not loading into a particular address the dynamic segment
        // is loaded with the whole file.
        lib->dynamic = (DynamicSection*)(lib->load_address + lib->program_headers[i].offset);
      } else {
        lib->dynamic = (DynamicSection*)(lib->load_address + lib->program_headers[i].vaddr);
      }
      break;
    }
  }
  if (lib->dynamic == NULL) {
    return false;
  }
  
  // Relocate the pointer entries in the dynamic section.
  if (load_address != NULL) {
    RelocateDynamicSection(lib);
  }
  
  // Find the symbol table and string table.
  lib->dynsym = DynamicLoaderFindDynamicSectionAddressEntry(lib,
                                                            DT(symtab));
  lib->dynstr = DynamicLoaderFindDynamicSectionAddressEntry(lib,
                                                            DT(strtab));
  
  // These must exist.
  if (lib->dynsym == NULL || lib->dynstr == NULL) {
    LoaderError("Failed to load dynamic library %s: no symbol table",
                lib->filename.value);
    return false;
  }
  return true;
}

// Find the GNU Hash table in the dynamic section.  It is most likely
// that it exists but it might not.
static void FindHashTable(LoadedDynamicLibrary* lib) {
  lib->gnu_hash = DynamicLoaderFindDynamicSectionAddressEntry(lib,
                                                              DT(gnu_hash));
  if (lib->gnu_hash == NULL) {
    // There is no GNU_HASH (and we don't support the old DT_HASH format)
    // so we will need to do a linear search for symbols.  But first we
    // need to figure out how many symbols there are.  Unfortunately, the
    // ELF designers seem to have forgotten to add that to the DYNAMIC
    // section so the only way to know is to look at the section header
    // for the SHT(dynsym) and divide its size by the entry size.
    for (int i = 0; i < lib->header->shnum; i++) {
      const ELFSectionHeader* section = &lib->section_headers[i];
      if (section->type == SHT(dynsym)) {
        lib->num_dynamic_symbols = section->size / section->entsize;
        break;
      }
    }
  }
}

// Expand all special path segments in the rpath string, replacing its
// value.
//
// There are special values in the path:
// * $ORIGIN or ${ORIGIN}: path where executable was found, with symbolic
//   links expanded.
// * $EXEC_ORIGIN or ${EXEC_ORIGIN}: path where executable was found, with no
//   symbolic link expansion.
// * $LIB or ${LIB}: "lib" or "lib64"
// * $PLATFORM or ${PLATFORM}: machine architecture name (e.g. x86_64).
void ExpandRuntimePath(LoadedDynamicLibrary* lib, String* rpath) {
  String result;
  StringInit(&result, NULL);
  size_t i = 0;
  while (i < rpath->length) {
    String part;
    StringInit(&part, NULL);
    while (i < rpath->length && rpath->value[i] != '/') {
      StringAppendChar(&part, rpath->value[i]);
      i++;
    }
    if (part.length > 0 && part.value[0] == '$') {
      if (StringEqual(&part, "$ORIGIN") || StringEqual(&part, "${ORIGIN}")) {
        StringSetString(&part, &lib->loader->resolved_origin);
      } else if (StringEqual(&part, "$EXEC_ORIGIN") ||
                 StringEqual(&part, "${EXEC_ORIGIN}")) {
        StringSetString(&part, &lib->loader->origin);
      } else if (StringEqual(&part, "$LIB") || StringEqual(&part, "${LIB}")) {
          StringSet(&part, "lib");
      } else if (StringEqual(&part, "$PLATFORM}") ||
                 StringEqual(&part, "${PLATFORM}")) {
        StringSet(&part, lib->loader->arch->platform);
      }
    }
    StringAppendString(&result, &part);
    StringDestruct(&part);
    if (rpath->value[i] == '/') {
      StringAppendChar(&result, '/');
      i++;
    }
  }
  StringSetString(rpath, &result);
  StringDestruct(&result);
}

// Look for DT_RUNPATH in the dynamic section and build
// the runtime path vector from its contents.  Older libraries
// used DT_RPATH but this is not supported here.
static void BuildRuntimePaths(LoadedDynamicLibrary* lib) {
  int64_t runpath_offset = DynamicLoaderFindDynamicSectionOffsetEntry(lib,
                                                                  DT(runpath));
  if (runpath_offset == -1) {
    return;
  }
  
  // Runpath is a string in the dynstr.
  const char* runpath = lib->dynstr + runpath_offset;
  
  // The runpath is a colon-separated list of directories to search.
  const char* p = runpath;
  while (*p != '\0') {
    String* path = NewString("");
    while (*p != '\0' && *p != ':') {
      StringAppendChar(path, *p++);
    }
    ExpandRuntimePath(lib, path);
    if (path->length > 0) {
      VectorAppend(&lib->runtime_search_path, path);
    } else {
      StringDelete(path);
    }
    if (*p == '\0') {
      break;
    }
    p++;
  }
}

bool LoadedDynamicLibraryLoad(LoadedDynamicLibrary* lib,
                              DynamicLibraryRegistry* registry,
                              Vector* search_path,
                              uint64_t *load_address) {
  // Find the pathname for the file.
  if (!FindDynamicLibraryFile(search_path, &lib->runtime_search_path,
                              lib->libname.value,
                              &lib->filename)) {
    return false;
  }
  const void* addr;
  size_t length;
  
  if (load_address == NULL) {
     lib->fd = MapWholeDynamicLibrary(&lib->filename, &addr, &length);
  } else {
    // Try to map the file header into memory.  This includes just
    // enough to include the program headers and section headers.
    lib->fd = MapDynamicLibraryHeader(&lib->filename, &addr, &length);
  }
  if (lib->fd < 0) {
    return false;
  }
  
  lib->addr = addr;
  lib->length = length;
  
  lib->header = (const ELFHeader*)lib->addr;
  lib->program_headers = (const ELFProgramHeader*)(lib->addr + lib->header->phoff);
  lib->section_headers = (const ELFSectionHeader*)(lib->addr + lib->header->shoff);

  if (!LoadSegments(lib, load_address)) {
    return false;
  }
  
  if (print_libraries_only) {
    uint64_t base_address = lib->load_address;
    if (base_address != 0) {
      printf("\t%s => %s (0x%llx)\n", lib->libname.value, lib->filename.value, base_address);
    }
  }
  
  // Find the PT(dynamic) segment.
  if (!FindDynamicSection(lib, load_address)) {
    return false;
  }
  
  // Find the symbol hash table.  It is most likely that it is
  // present.
  FindHashTable(lib);
  
  // Build runtime paths from DT_RPATH or DT_RUNPATH.  Only do this for
  // executable, not loaded libraries.
  if (lib->loader != NULL && lib->header->type == ET(exec)) {
    BuildRuntimePaths(lib);
  }
  
  if (lib->load_symbol_table) {
    FindAndLoadSymbolTable(lib);
  }
  
  // Load all libraries needed by this one.
  LoadNeededLibraries(registry, lib, search_path, load_address);
  return true;
}

LoadedDynamicLibrary* NewLoadedDynamicLibrary(const char* libname,
                                              Loader* loader) {
  // Create the dynamic library struct.
  LoadedDynamicLibrary* lib = malloc(sizeof(LoadedDynamicLibrary));
  lib->loader = loader;
  lib->fd = 0;
  StringInit(&lib->libname, libname);
  StringInit(&lib->filename, NULL);

  lib->addr = 0;
  lib->length = 0;
  lib->num_dynamic_symbols = 0;
  lib->header = NULL;
  lib->dynamic = NULL;
  lib->dynstr = NULL;
  lib->dynsym = NULL;
  lib->gnu_hash = NULL;
  lib->program_headers = NULL;
  lib->section_headers = NULL;
  VectorInit(&lib->mapped_segments);
  VectorInit(&lib->runtime_search_path);
  lib->load_symbol_table = loader == NULL ? false :
            (loader->flags & LOADER_MAP_SYMTAB) != 0;
  lib->symtab = NULL;
  lib->strtab = NULL;
  lib->num_symtab_symbols = 0;
  return lib;
}

void LoadedDynamicLibraryDestruct(LoadedDynamicLibrary* lib) {
  if (lib->length > 0) {
    munmap((void*)lib->addr, lib->length);
  }
  close(lib->fd);
  VectorDestructWithContents(&lib->mapped_segments,
                             (VectorElementDestructor)MappedSegmentDestruct);
  VectorDestructWithContents(&lib->runtime_search_path,
                             (VectorElementDestructor)StringDestruct);
}

void LoadedDynamicLibraryDelete(LoadedDynamicLibrary* lib) {
  LoadedDynamicLibraryDestruct(lib);
  free(lib);
}

// Find a symbol by a linear search.  This is only used when there is
// no GNU_HASH section, which is very unlikely.
static const ELFSymbol* FindSymbolSlowly(LoadedDynamicLibrary* lib,
                                   const char* name) {
  for (int64_t i = 0; i < lib->num_dynamic_symbols; i++) {
    const ELFSymbol* symbol = &lib->dynsym[i];
    const char* symbol_name = &lib->dynstr[symbol->name];
    if (symbol->shndx == 0) {
      // Symbol is undefined.
      continue;
    }
    if (strcmp(symbol_name, name) == 0) {
      return symbol;
    }
  }
  return NULL;
}

// Find a symbol in a library by the fastest means possible.
const ELFSymbol* LoadedDynamicLibraryFindSymbol(LoadedDynamicLibrary* lib,
                                                    const char* name) {
  if (lib->gnu_hash == NULL) {
    // No option but to do a linear search.
    return FindSymbolSlowly(lib, name);
  }
  
  // Search the hash table.
  // First get hash value.
  uint32_t hash = DynamicLoaderGNUHash(name);
  
  // Look in Bloom filter.  If either bit is not set the symbol doesn't
  // exist.  The bloom filter is an array of 64 bit words located immediately
  // after the hash table header.
  uint64_t* bloom_filter = (uint64_t*)(lib->gnu_hash + 1);
  int index = (hash / 64) % lib->gnu_hash->bloom_size;
  uint64_t bits = DynamicLoaderBloomBits64(hash);
  if ((bloom_filter[index] & bits) != bits) {
    // Bloom filter says this hash code is definitely not in the table.
    return NULL;
  }
  
  // Symbol may or may not be in the table.  We find out by looking in the
  // chains array indexed by the bucket.
  int bucket = hash % lib->gnu_hash->num_buckets;
  uint32_t* buckets = (uint32_t*)(bloom_filter + lib->gnu_hash->bloom_size);
  int symbol_index = buckets[bucket];
  if (symbol_index < lib->gnu_hash->symoffset) {
    // An undefined symbol.
    return NULL;
  }
  
  // The chains are immediately after the buckets.
  uint32_t* chains = buckets + lib->gnu_hash->num_buckets;
  
  // Go through the chain entries starting at the symbol index minus
  // the symoffset from the header.  Compare the hash codes and if they
  // match perform a string compare with the symbol name.
  for (;;) {
    const ELFSymbol* symbol = &lib->dynsym[symbol_index];
    const char* symbol_name = &lib->dynstr[symbol->name];
    
    // Look at the chain entry and compare the top 31 bits of that entry
    // with the top 31 bits of the hash.  If they match do a string
    // comparison with the symbol name.
    uint32_t chain_hash = chains[symbol_index - lib->gnu_hash->symoffset];
    if ((chain_hash | 1) == (hash | 1) && strcmp(name, symbol_name) == 0) {
      return symbol;
    }
    
    // End of chain has the bottom bit set.
    if ((chain_hash & 1) != 0) {
      break;
    }
    symbol_index++;
  }
  return NULL;
}

// Lookup a symbol in the library by address.  
bool LoadedDynamicLibraryLookupSymbolByAddress(LoadedDynamicLibrary* lib,
                                               uint64_t address,
                                               const char** name,
                                               uint64_t* start,
                                               uint64_t* length) {
  if (lib->symtab == NULL) {
    return false;
  }
  // First make sure the address is within a loaded segment.
  bool possibly_in_this_library = false;
  for (size_t i = 0; i < lib->mapped_segments.length; i++) {
    MappedSegment* segment = lib->mapped_segments.value.p[i];
    uint64_t segment_address = (uint64_t)segment->address;
    if (address >= segment_address &&
        address < (segment_address + segment->length)) {
      possibly_in_this_library = true;
      break;
    }
  }
  if (!possibly_in_this_library) {
    return false;
  }
  
  // The symbol might be in this library, the only way to know is to
  // perform a linear search of the symbol table.  The symbol table
  // is not guaranteed to be ordered in any way and it would use too
  // much memory to load it into a hash table.  This is only used
  // for debugging prints anyway so speed isn't really that
  // important.  A debugger would load them into a hash table.
  for (int i = 0; i < lib->num_symtab_symbols; i++) {
    const ELFSymbol* symbol =  &lib->symtab[i];
    uint64_t symbol_start_address = lib->load_address + symbol->value;
    uint64_t symbol_end_address = symbol_start_address + symbol->size;
    if (address >= symbol_start_address && address < symbol_end_address) {
      *start = symbol_start_address;
      *length = symbol->size;
      *name = lib->strtab + symbol->name;
      return true;
    }
  }
  return false;
}

// Load the segments of the dynamic library starting at the given
// address.  The PT_LOAD segments are loaded in order specified
// in the file.  Returns the address of the first loaded segment and
// updates *load_address to be the end of the loaded segments.
// If the load_address is NULL then the address for each segment
// is the address specified in the program header.
uint64_t LoadedDynamicLibraryLoadSegments(LoadedDynamicLibrary* lib,
            uint64_t* load_address) {
  // Get page size and mask (almost guaranteed to be 4K).
  int page_size = (int)sysconf(_SC_PAGESIZE);
  int page_mask = page_size - 1;
  
  uint64_t loaded_at = 0;
  for (int i = 0; i < lib->header->phnum; i++) {
    const ELFProgramHeader* segment = &lib->program_headers[i];
    // Load the PT(load) and PT(dynamic) segments into memory
    // at the address specified by their alignment.
    if (segment->type == PT(load) || segment->type == PT(dynamic)) {
      uint64_t addr;
      if (load_address == NULL) {
        addr = segment->vaddr;       // Address to place segment at.
      } else {
        addr = *load_address;
        
        // The address to map at is the current load_address aligned up to
        // the boundary specified in the program header.
        uint64_t alignment = segment->align;
        addr = Align(addr, alignment);
      }
      if (load_address != NULL && loaded_at == 0) {
        loaded_at = addr;
      }
      uint64_t offset = segment->offset;    // Offset into file.
      
      // Align address and offset to lower page boundary.  The address and
      // file offset must be page-aligned for the mmap function to operate
      // correctly (it will error out if this is not the case).
      addr &= ~page_mask;
      offset &= ~page_mask;

      // Protection for mmap and open.  We have to open the file in order the mmap it.
      // If the mapping is going to allow writes to the pages we need to open the file
      // in read-write mode, but we won't be writing to it.
      int prot = PROT_READ;
      if ((segment->flags & PF(w)) != 0) {
        // Segment is writeable.
        prot |= PROT_WRITE;
      }
      if ((segment->flags & PF(x)) != 0) {
        // Segment is executable.
        prot |= PROT_EXEC;
      }
      
      // Length of segment in memory.  We can map memory up the next
      // page boundary beyond the end of the file.  If the memsz is
      // beyond that we need to mmap a new ANON segment for it.
      int64_t length = segment->filesz + (segment->vaddr & page_mask);
      length = Align(length, page_size);
      
      if (length == 0) {
        goto next_segment;
      }

      // Map in the segment at an address chosen by the OS.  This is done using
      // the MAP_PRIVATE flag so that the pages are all copy-on-write, meaning that they
      // will be copied to a new physical address if they are written to, otherwise they
      // are shared with other physical pages that map the same file in.
      void* segment_ptr = mmap((void*)addr, length, prot,
                               MAP_PRIVATE|MAP_FIXED, lib->fd, offset);
      if (segment_ptr == MAP_FAILED) {
        LoaderError("Failed to map in dynamic segment: %s\n", strerror(errno));
        goto next_segment;
      }

      // Zero out any difference between memsz and filesz.  This will really only
      // be the .bss section.  We have mapped the contents of the file but some of
      // it will need to be zeroed out.  We also need to allocate a contiguous
      // anonymous region of zeros above the segment.
      // We need the .bss to be all zeroes before thae program starts.
      if ((prot & PROT_WRITE) != 0 && segment->memsz > segment->filesz) {
        void* zeroed_region = (char*)(loaded_at + segment->vaddr) +
            segment->filesz;   // Start of zero memory.
        // Calculate end of mapped memory.  The 'length' contains the total length
        // of the mapped memory.
        uint64_t end_of_mapped_memory = (uint64_t)addr +
               length;
        int64_t zeroed_region_size = end_of_mapped_memory - (uint64_t)zeroed_region;
        memset(zeroed_region, 0, zeroed_region_size);
        
        // Any additional memory beyond the file.
        int64_t additional_memory = Align(segment->memsz - length, page_size);
        if (additional_memory > 0) {
          void* zero = (char*)end_of_mapped_memory;
          zero = mmap(zero, additional_memory, PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANON, 0, 0);
          if (zero == MAP_FAILED) {
            LoaderError("Failed to map in dynamic segment: %s\n", strerror(errno));
            goto next_segment;
          }
          VectorAppend(&lib->mapped_segments, NewMappedSegment(zero, additional_memory));
        }
      }
      
      // Add a new region to the regions vector so that we can remove it
      // when destructed.
      VectorAppend(&lib->mapped_segments, NewMappedSegment(segment_ptr, length));
      
    next_segment:
      if (load_address != NULL) {
        // Move to next load address.
        *load_address += segment->memsz;
        
        // And align to the segment alignment.
        *load_address = Align(*load_address, segment->align);
       }
    }
  }
  return loaded_at;
}

void LoadedDynamicLibraryRelocate(Loader* loader,
                                  LoadedDynamicLibrary* lib,
                                  DynamicLibraryRegistry* registry,
                                  bool lazy) {
  PerformDynamicRelocations(loader, lib, registry, lazy);
}


