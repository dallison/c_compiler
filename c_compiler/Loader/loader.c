//
//  loader.c
//
//  Created by David Allison on 1/18/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "loader.h"
#include "loader_arch.h"
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <inttypes.h>

// MAP_ANON seems to have an issue on Raspbian.
#ifndef MAP_ANON
#define MAP_ANON 0x20
#endif

const bool kPrintSymbolTableOnStart = true;
static int num_errors;

int LoaderNumErrors() {
  return num_errors;
}

Region* NewRegion(void* addr, int64_t offset, int64_t length,
                  ELFProgramHeader* segment,
                  LoadedDynamicLibrary* owner) {
  Region* region = malloc(sizeof(Region));
  region->address = addr;
  region->offset = offset;
  region->length = length;
  region->segment = segment;
  region->owner = owner;
  VectorInit(&region->sections);
  return region;
}

void RegionDestruct(Region* region) {
  VectorDestruct(&region->sections);
}

void VLoaderError(const char* error, va_list ap) {
  char buf[4096];
  vsnprintf(buf, sizeof(buf), error, ap);
  fprintf(stderr, "Loader error: %s\n", buf);
  num_errors++;
}

void LoaderError(const char* error, ...) {
  va_list ap;
  va_start(ap, error);
  VLoaderError(error, ap);
  va_end(ap);
}



void VLoaderWarning(const char* warn, const char* error, va_list ap) {
  vfprintf(stderr, error, ap);
}

void LoaderWarning(const char* warn, const char* error, ...) {
  va_list ap;
  va_start(ap, error);
  VLoaderWarning(warn, error, ap);
  va_end(ap);
  
}

static bool LoaderRuntimeAddressToLinked(Loader* loader, uint64_t runtime,
                                          uint64_t* linked);

static bool FindStaticSymbolByAddress(Loader* loader,
                             uint64_t address,
                             const char** name,
                             uint64_t* start,
                             uint64_t* length) {
  StaticSymbolTable* symbols = &loader->static_symbol_table;
  uint64_t linked_address = address;
  if (loader->arch->ignore_vaddr &&
      !LoaderRuntimeAddressToLinked(loader, address, &linked_address)) {
    return false;
  }
  // Binary search for symbols_by_addr.
  int begin = 0;
  int end = (int)symbols->symbols_by_addr.length - 1;
  int mid = end / 2;
  int num_symbols = (int)symbols->symbols_by_addr.length;
  while (begin <= end) {
    ELFSymbol* sym1 = symbols->symbols_by_addr.value.p[mid];
    ELFSymbol* sym2 = NULL;
    if (mid < num_symbols - 1) {
      sym2 = symbols->symbols_by_addr.value.p[mid+1];
    }
    if (linked_address >= sym1->value && (sym2 == NULL || linked_address < sym2->value)) {
      // Found between sym1 and sym2.
      if (loader->arch->ignore_vaddr) {
        if (!LoaderLinkedAddressToRuntime(loader, loader->dynamic_lib,
                                          sym1->value, start)) {
          return false;
        }
      } else {
        *start = sym1->value;
      }
      *length = sym1->size;
      *name = symbols->strtab + sym1->name;
      return true;
    }
    if (linked_address < sym1->value) {
      end = mid - 1;
    } else {
      begin = mid + 1;
    }
    mid = (end + begin) / 2;
  }
  return false;
}

static ELFSymbol* FindStaticSymbolByName(Loader* loader, const char* name) {
  MapKeyType key = {.p = (void*)name};
  return MapFind(&loader->static_symbol_table.symbols_by_name, key);
}

bool LoaderFindSymbol(Loader* loader,
                             uint64_t address,
                      SymbolScope* symbol) {
   uint64_t symbol_address;
   uint64_t symbol_length;
   const char* symbol_name;
  bool found;
  if (loader->is_static) {
    found = FindStaticSymbolByAddress(loader, address, &symbol_name,
                             &symbol_address,
                             &symbol_length);
  } else {
    found = DynamicLoaderLookupSymbolByAddress(
                                             &loader->loaded_libraries,
                                             address,
                                             &symbol_name,
                                             &symbol_address,
                                             &symbol_length);
  }
  if (!found) {
    return false;
  }
  symbol->start = symbol_address;
  symbol->end = symbol_address + symbol_length;
  symbol->name = symbol_name;
  return true;
}

SymbolScope* LoaderFindSymbolAndCacheResult(Loader* loader,
                      uint64_t address) {
  if (address >= loader->current_symbol.start && address < loader->current_symbol.end) {
    return &loader->current_symbol;
  }
  
  bool found = LoaderFindSymbol(loader, address, &loader->current_symbol);
  if (!found) {
    return NULL;
  }

  return &loader->current_symbol;
}

uint64_t LoaderLookupSymbol(Loader* loader, const char* name) {
  const ELFSymbol* symbol;
  if (loader->is_static) {
    symbol = FindStaticSymbolByName(loader, name);
    if (symbol == NULL) {
      return 0;
    }
    uint64_t runtime = 0;
    if (!LoaderLinkedAddressToRuntime(loader, loader->dynamic_lib,
                                      symbol->value, &runtime)) {
      return 0;
    }
    return runtime;
  }
  LoadedDynamicLibrary* lib;
  bool found = DynamicLoaderFindSymbol(&loader->loaded_libraries,
                                            name,
                                       &symbol, &lib);
  if (!found) {
    return 0;
  }
  return lib->load_address + symbol->value;
}

void LoaderSetCurrentSymbol(Loader* loader,
                            uint64_t address,
                            uint64_t length,
                            const char* name) {
}

SymbolScope* LoaderGetCurrentSymbol(Loader* loader) {
  return &loader->current_symbol;
}


// Align the given value to a power of 2 alignment.
static uint64_t AlignUp(uint64_t v, uint64_t alignment) {
  return (v + (alignment - 1)) & ~(alignment - 1);
}

static uint64_t AlignDown(uint64_t v, uint64_t alignment) {
  return v & ~(alignment - 1);
}

// Find all the sections that are part of the program segment and
// add them to the region.
static void GetRegionSections(Loader* loader, Region* region,
                              ELFProgramHeader* segment, size_t segment_number) {
  uint64_t segment_start = segment->offset;
  uint64_t segment_end = segment->offset + segment->filesz;
  for (size_t i = 0; i < loader->elf_file->sections.length; i++) {
    ELFReaderSection* section = loader->elf_file->sections.value.p[i];
    if (section->header->type == SHT(null)) {
      continue;
    }
    uint64_t section_start = section->header->offset;
    uint64_t section_end = section->header->offset + section->header->size;
    if (section_start >= segment_start && section_end <= segment_end) {
      VectorAppend(&region->sections, section);
    }
  }
}


static int CompareSymbolAddress(const void* a, const void* b) {
  const ELFSymbol* s1 = *(const ELFSymbol**)a;
  const ELFSymbol* s2 = *(const ELFSymbol**)b;
  return (int)(s1->value - s2->value);
}

static int64_t SegmentFileOffsetDelta(const ELFProgramHeader* segment) {
  int64_t page_size = sysconf(_SC_PAGESIZE);
  return segment->offset - AlignDown(segment->offset, page_size);
}

bool LoaderLinkedAddressToRuntime(Loader* loader, LoadedDynamicLibrary* lib,
                                  uint64_t linked,
                                  uint64_t* runtime) {
  if (!loader->arch->ignore_vaddr) {
    *runtime = linked;
    return true;
  }
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    if (lib != NULL && region->owner != lib) {
      continue;
    }
    if (region->segment == NULL) {
      continue;
    }
    ELFProgramHeader* segment = region->segment;
    if (linked < segment->vaddr || linked >= segment->vaddr + segment->memsz) {
      continue;
    }
    *runtime = (uint64_t)region->address + SegmentFileOffsetDelta(segment) +
               (linked - segment->vaddr);
    return true;
  }
  return false;
}

static bool LoaderRuntimeAddressToLinked(Loader* loader, uint64_t runtime,
                                          uint64_t* linked) {
  if (!loader->arch->ignore_vaddr) {
    *linked = runtime;
    return true;
  }
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    if (region->segment == NULL) {
      continue;
    }
    ELFProgramHeader* segment = region->segment;
    uint64_t base = (uint64_t)region->address + SegmentFileOffsetDelta(segment);
    if (runtime < base || runtime >= base + segment->memsz) {
      continue;
    }
    *linked = segment->vaddr + (runtime - base);
    return true;
  }
  return false;
}

static void LoaderFixupStaticAddresses(Loader* loader) {
  if (!loader->arch->ignore_vaddr) {
    return;
  }
  uint64_t runtime_entry = 0;
  if (!LoaderLinkedAddressToRuntime(loader, loader->dynamic_lib,
                                    loader->main_address,
                                    &runtime_entry)) {
    LoaderError("Cannot translate entry point 0x%" PRIx64 "\n",
                loader->main_address);
    return;
  }
  loader->main_address = runtime_entry;
}

static void MapSymbolTable(Loader* loader, int fd,
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
  int64_t length = AlignUp(end - start_offset, page_size);
  void* addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, start_offset);
  if (addr == MAP_FAILED) {
    return;
  }
  loader->static_symbol_table.symtab = (const ELFSymbol*)((char*)addr + delta + (symtab->offset - start));
  loader->static_symbol_table.strtab = (const char*)addr + delta + (strtab->offset - start);
  VectorAppend(&loader->regions, NewRegion(addr, start_offset, length, NULL, NULL));
  loader->static_symbol_table.load_address = (uint64_t)addr;
}

static void FindAndLoadSymbolTable(Loader* loader, int fd) {
  // Find the regular symbol table and string table so that we
  // can lookup symbols by address.  This is useful for printing
  // the location of instructions.
  // The only way to do this is to search the sections for the
  // SHT(symtab).
  for (int i = 0; i < loader->elf_file->header->shnum; i++) {
    const ELFReaderSection* section = loader->elf_file->sections.value.p[i];
    if (section->header->type == SHT(symtab)) {
      // Found symbol table section.  Its link field is the section
      // index of the string table.
      
      loader->static_symbol_table.num_symtab_symbols = section->header->size / section->header->entsize;
      const ELFReaderSection* strtab = loader->elf_file->sections.value.p[section->header->link];
      MapSymbolTable(loader, fd, section->header, strtab->header);
      
      // Add all symbols to internal caches.
      StaticSymbolTable* symbols = &loader->static_symbol_table;
      for (int i = 0; i < symbols->num_symtab_symbols; i++) {
        const ELFSymbol* symbol =  &symbols->symtab[i];
        const char* symname = symbols->strtab + symbol->name;
        MapKeyValue kv = {.key.p = (void*)symname, .value.p = (void*)symbol};
        MapInsert(&symbols->symbols_by_name, kv);
        VectorAppend(&symbols->symbols_by_addr, (void*)symbol);
      }
      VectorSortPointers(&symbols->symbols_by_addr, CompareSymbolAddress);
      break;
    }
  }
}

#define PRINT_DEBUG 0
#define DPRINTF(...) if (PRINT_DEBUG) printf(__VA_ARGS__)

static bool LoaderMapIgnoreVaddrSegment(int elf_fd, ELFProgramHeader* segment,
                                        int64_t offset_diff, uint64_t map_length,
                                        void** out_ptr) {
  void* segment_ptr = mmap(NULL, map_length, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANON, -1, 0);
  if (segment_ptr == MAP_FAILED) {
    printf("Failed to map in ELF segment: %s\n", strerror(errno));
    return false;
  }
  if (segment->filesz > 0) {
    ssize_t bytes =
        pread(elf_fd, (char*)segment_ptr + offset_diff, segment->filesz,
              (off_t)segment->offset);
    if (bytes != (ssize_t)segment->filesz) {
      printf("Failed to read ELF segment: %s\n", strerror(errno));
      munmap(segment_ptr, map_length);
      return false;
    }
  }
  if (segment->memsz > segment->filesz) {
    memset((char*)segment_ptr + offset_diff + segment->filesz, 0,
           (size_t)(segment->memsz - segment->filesz));
  }
  *out_ptr = segment_ptr;
  return true;
}

static bool LoadStaticSegments(Loader* loader, String* filename) {
  // Get page size and mask (almost guaranteed to be 4K).
  int page_size = (int)sysconf(_SC_PAGESIZE);
  
  // Process all program segments and look for PT_LOAD types.  These are
  // loadable segments that have an address assigned to them.  We map them in
  // from the file directly using the 'mmap' function provided by the operating
  // system.  This allocates the virtual memory pages at the specified address
  // and points them to the file contents so that when they are read the page
  // is filled with the data in the file.
  for (size_t i = 0; i < loader->elf_file->segments.length; i++) {
    ELFProgramHeader* segment = loader->elf_file->segments.value.p[i];
    if (segment->type == PT(load)) {
      if (segment->memsz == 0) {
        // No point in trying to map a zero length segment.
        break;
      }
      
      uint64_t mmap_addr = loader->arch->ignore_vaddr ? 0 : segment->vaddr;       // Address to place segment at.
      uint64_t vaddr = segment->vaddr;
      DPRINTF("mmap_addr: %" PRIx64 "\n", mmap_addr);
      DPRINTF("vaddr: %" PRIx64 "\n", vaddr);

      uint64_t offset = segment->offset;    // Offset into file.
      DPRINTF("offset: %" PRIx64 "\n", offset);

      // Align address to the segment alignment.
      uint64_t alignment_mask = segment->align - 1;
      mmap_addr &= ~alignment_mask;
      
      // Align address and offset to lower page boundary.  The address and
      // file offset must be page-aligned for the mmap function to operate
      // correctly (it will error out if this is not the case).
      mmap_addr = AlignDown(mmap_addr, page_size);
      DPRINTF("aligned mmap_addr: %" PRIx64 "\n", mmap_addr);
      vaddr = AlignDown(vaddr, page_size);
      DPRINTF("aligned vaddr: %" PRIx64 "\n", vaddr);
      offset = AlignDown(offset, page_size);
      int64_t offset_diff =  segment->offset - offset;

      // Difference between vaddr and aligned address.
      uint64_t mem_delta = segment->vaddr - vaddr;
      DPRINTF("mem_delta: %" PRIx64 "\n", mem_delta);

      // Length of segment in memory.  We can map memory up the next
      // page boundary beyond the end of the file.  If the memsz is
      // beyond that we need to mmap a new ANON segment for it.
      uint64_t length = offset_diff + segment->filesz;
      DPRINTF("length: %" PRIx64 "\n", length);
      
      length = AlignUp(length, page_size);
      DPRINTF("aligned length: %" PRIx64 "\n", length);

      // Protection for mmap. MAP_PRIVATE writable mappings do not require the
      // underlying file descriptor to be writable.
      int prot = PROT_READ;
      if ((segment->flags & PF(w)) != 0 ||
          (loader->flags & LOADER_WRITEABLE_TEXT) != 0) {
        // Segment is writeable.
        prot |= PROT_WRITE;
      }
      if ((segment->flags & PF(x)) != 0) {
        // Segment is executable.
        prot |= PROT_EXEC;
      }
      
             // Open the ELF file again to get a file descriptor that we can use
      // for mmap.
      int fd = open(filename->value, O_RDONLY);
      if (fd < 0) {
        printf("Failed to open ELF file segment\n");
        return false;
      }
      
      if (length == 0) {
        close(fd);
        continue;
      }

      void* segment_ptr = NULL;
      if (loader->arch->ignore_vaddr) {
        uint64_t map_length = AlignUp(offset_diff + segment->memsz, page_size);
        if (!LoaderMapIgnoreVaddrSegment(fd, segment, offset_diff, map_length,
                                         &segment_ptr)) {
          close(fd);
          return false;
        }
        length = map_length;
        close(fd);
      } else {
        int flags = MAP_PRIVATE;
        if (mmap_addr != 0) {
          flags |= MAP_FIXED;
        }
        segment_ptr = mmap((void*)mmap_addr, length, prot, flags, fd, offset);
        close(fd);
        if (segment_ptr == MAP_FAILED) {
          printf("Failed to map in ELF segment: %s\n", strerror(errno));
          return false;
        }
      }
      
      DPRINTF("aligned segment loaded at %p\n", segment_ptr);
      DPRINTF("loaded length: %" PRIx64 "\n", length);
      DPRINTF("offset diff: %" PRIx64 "\n", offset_diff);
      // Zero out any difference between memsz and filesz.  This will really only
      // be the .bss section.  We have mapped the contents of the file but some of
      // it will need to be zeroed out.  We also need to allocate a contiguous
      // anonymous region of zeros above the segment.
      // We need the .bss to be all zeroes before thae program starts.
      if ((prot & PROT_WRITE) != 0 && !loader->arch->ignore_vaddr) {
        void* zeroed_region = (loader->arch->ignore_vaddr ?
                               (segment_ptr + offset_diff): (char*)segment->vaddr) +
            segment->filesz;   // Start of zero memory.
        DPRINTF("zeroed region at %p (segment file size: %" PRIx64 ", mem size: %" PRIx64 "\n", zeroed_region,
               segment->filesz, segment->memsz);
        // Calculate end of mapped memory.  The 'length' contains the total length
        // of the mapped memory.
        uint64_t end_of_mapped_memory = (loader->arch->ignore_vaddr ?
                                         (uint64_t)segment_ptr : mmap_addr) +
                                          length;
        DPRINTF("end of mapped memory at %" PRIx64 "\n", end_of_mapped_memory);
        
        int64_t zeroed_region_size = end_of_mapped_memory - (uint64_t)zeroed_region;
        DPRINTF("zeroed region size: %" PRIx64 "\n", zeroed_region_size);
        if (zeroed_region_size > 0) {
          DPRINTF("zeroing %p for %" PRIx64 " bytes\n", zeroed_region, zeroed_region_size);
          memset(zeroed_region, 0, zeroed_region_size);
        }
        // Any additional memory beyond the file.
        int64_t additional_memory = AlignUp(segment->memsz - length, page_size);
        DPRINTF("additional mempory: %" PRId64 "\n", additional_memory);
        if (additional_memory > 0) {
          void* zero = (char*)end_of_mapped_memory;
          zero = mmap(zero, additional_memory, PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANON, 0, 0);
          if (zero == MAP_FAILED) {
            LoaderError("Failed to map in dynamic segment: %s\n", strerror(errno));
            return false;
          }
          VectorAppend(&loader->regions, NewRegion(zero, 0, additional_memory, NULL, NULL));
        }
      }
      
      // Add a new region to the regions vector so that we can remove it
      // when destructed.
      Region* region = NewRegion(segment_ptr, offset, length, segment, NULL);
      VectorAppend(&loader->regions, region);
      GetRegionSections(loader, region, segment, i);
    }
  }
  // Open file to map symbol table.
  int fd = open(filename->value, O_RDONLY);
  if (fd < 0) {
    printf("Failed to open ELF file segment\n");
    return false;
  }
  FindAndLoadSymbolTable(loader, fd);
  return true;
}

// Map in the memory for the symbol table and string table.  The address

// Find the .dynamic section.  Return NULL if not found.
static DynamicSection* FindDynamicSection(Loader* loader) {
  for (size_t i = 0; i < loader->elf_file->sections.length; i++) {
    ELFReaderSection* section = loader->elf_file->sections.value.p[i];
    if (section->header->type == SHT(dynamic)) {
      return section->contents;
    }
  }
  return NULL;
}


static bool LoadDynamic(Loader* loader, bool lazy) {
  // Create a new LoaddedDynamicLibrary from the currently loaded
  // file. This will recursively load all the libraries it needs.
  // TODO: figure out where to get the address from.
  static uint64_t load_addr = 0x600000000LL;
  uint64_t next_available_address = 0;
  loader->dynamic_lib = NewLoadedDynamicLibrary(loader->filename.value,
                                                loader);
  DynamicLibraryRegistryInsert(&loader->loaded_libraries, loader->dynamic_lib);
  bool ok = LoadedDynamicLibraryLoad(loader->dynamic_lib, &loader->loaded_libraries,
                                     &loader->library_search_path,
                                     load_addr,
                                     &next_available_address);
  
   if (!ok) {
     LoaderError("Unable to find %s", loader->dynamic_lib->libname.value);
     LoadedDynamicLibraryDelete(loader->dynamic_lib);
     return false;
  }
  
  // Next load address is at end of loaded libary.
  load_addr = AlignUp(next_available_address, (int)sysconf(_SC_PAGESIZE));
  
  // Relocate all the loaded libraries.
  for (size_t i = 0; i < loader->loaded_libraries.search.length; i++) {
    LoadedDynamicLibrary* lib = loader->loaded_libraries.search.value.p[i];
    LoadedDynamicLibraryRelocate(loader, lib,
                                 &loader->loaded_libraries,
                                 lazy);
  }
  if (loader->arch->ignore_vaddr) {
    LoaderFixupStaticAddresses(loader);
  }
  return true;
}

static void InitLibrarySearchPath(Loader* loader) {
  VectorAppend(&loader->library_search_path, NewString("/usr/lib"));
  VectorAppend(&loader->library_search_path, NewString("/lib"));
  
  char* ld_library_path = getenv("LD_LIBRARY_PATH");
  if (ld_library_path != NULL) {
    char* start = ld_library_path;
    while (*start != '\0') {
      char* p = start;
      while (*p != ':' && *p != '\0') {
        p++;
      }
      String* dir = NewString("");
      StringAppendSegment(dir, start, p - start);
      VectorAppend(&loader->library_search_path, dir);
      start = p;
      if (*start == ':') {
        start++;
      }
    }
  }
}

bool LoaderInitFromFile(Loader* loader, String* filename,  int32_t flags,
                        LoaderArchitecture* arch, void* arch_data,
                        const char* initial_path) {
  loader->arch = arch;
  loader->arch_data = arch_data;
  loader->flags = flags;
  VectorInit(&loader->static_symbol_table.symbols_by_addr);
  MapInitForCharPointerKeys(&loader->static_symbol_table.symbols_by_name);
  
  // Load the ELF file.
  loader->elf_file = NewELFReaderFile(filename);
  bool ok = ELFReaderFileRead(loader->elf_file, 0, 0);
  if (!ok) {
    fprintf(stderr, "Failed to read ELF file %s\n", filename->value);
    return false;
  }

  if (loader->elf_file->header->machine != loader->arch->machine_type) {
    fprintf(stderr, "Wrong architecture: got %d, need %d\n",
            loader->elf_file->header->machine,
            loader->arch->machine_type);
    return false;
  }
  
  // Set origin to the directory name of the executable file, or empty
  // string if there isn't one.
  StringInit(&loader->origin, NULL);
  const char* slash = strrchr(filename->value, '/');
  if (slash != NULL) {
    StringAppendSegment(&loader->origin,
                        filename->value,
                        slash - filename->value);
  } else {
    StringAppend(&loader->origin, ".");
  }

  // Expand any symbolic links in the origin path.
  char resolved[4096];
  char* p = realpath(loader->origin.value, resolved);
  if (p != NULL) {
    StringInit(&loader->resolved_origin, resolved);
  } else {
    StringInit(&loader->resolved_origin, loader->origin.value);

  }
  DynamicLibraryRegistryInit(&loader->loaded_libraries);
  VectorInit(&loader->library_search_path);
  if (initial_path != NULL) {
    VectorAppend(&loader->library_search_path, NewString(initial_path));
  }
  InitLibrarySearchPath(loader);
  StringInit(&loader->filename, filename->value);
  
  // Initialize regions vector.
  VectorInit(&loader->regions);

  // Add the whole ELF file to the regions vector.
  VectorAppend(&loader->regions, NewRegion(loader->elf_file->header, 0,
                                           loader->elf_file->file_length, NULL,
                                           NULL));

  // Get the entry point address.
  loader->main_address = loader->elf_file->header->entry;
  
  loader->dynamic = FindDynamicSection(loader);
  if (loader->dynamic == NULL) {
    // No dynamic segment means this is a fully static executable.
    if (loader->elf_file->header->type != ET(exec)) {
      fprintf(stderr, "Not a static executable\n");
      return false;
    }
    ok = LoadStaticSegments(loader, filename);
    if (ok) {
      LoaderFixupStaticAddresses(loader);
    }
    loader->is_static = true;
  } else {
    // This is a dynamic executable.
    ok = LoadDynamic(loader, (flags & LOADER_LAZY_RESOLVE) != 0);
    loader->is_static = false;
}
    
  memset(&loader->current_symbol, 0, sizeof(SymbolScope));
  
  return ok && LoaderNumErrors() == 0;
}

void LoaderDestruct(Loader* loader) {
  // Unmap all regions.
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    munmap(region->address, region->length);
  }
  DynamicLibraryRegistryInit(&loader->loaded_libraries);
  
  VectorDestructWithContents(&loader->regions,
                             (VectorElementDestructor)RegionDestruct, /*free_element=*/true);
  VectorDestruct(&loader->static_symbol_table.symbols_by_addr);
  MapDestruct(&loader->static_symbol_table.symbols_by_name);
}
