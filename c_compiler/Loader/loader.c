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

const bool kPrintSymbolTableOnStart = true;

Region* NewRegion(void* addr, int64_t length) {
  Region* region = malloc(sizeof(Region));
  region->address = addr;
  region->length = length;
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

SymbolScope* LoaderFindSymbol(Loader* loader,
                      uint64_t address) {
  if (address >= loader->current_symbol.start && address < loader->current_symbol.end) {
    return &loader->current_symbol;
  }
  
  // Symbol is not cached in current_symbol, replace it.
  uint64_t symbol_address;
  uint64_t symbol_length;
  const char* symbol_name;
  bool found = DynamicLoaderLookupSymbolByAddress(
                                            &loader->loaded_libraries,
                                            address,
                                            &symbol_name,
                                            &symbol_address,
                                            &symbol_length);
  if (found) {
    loader->current_symbol.start = symbol_address;
    loader->current_symbol.end = symbol_address + symbol_length;
    loader->current_symbol.name = symbol_name;
    return &loader->current_symbol;
  }
  return NULL;
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
static uint64_t Align(uint64_t v, uint64_t alignment) {
  return (v + (alignment - 1)) & ~(alignment - 1);
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

static bool LoadStaticSegments(Loader* loader, String* filename) {
  // Get page size and mask (almost guaranteed to be 4K).
  int page_size = (int)sysconf(_SC_PAGESIZE);
  int page_mask = page_size - 1;
  
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
      uint64_t addr = loader->arch->ignore_vaddr ? 0 : segment->vaddr;       // Address to place segment at.
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
      int file_prot = O_RDONLY;
      if ((segment->flags & PF(w)) != 0) {
        // Segment is writeable.
        prot |= PROT_WRITE;
        file_prot = O_RDWR;
      }
      if ((segment->flags & PF(x)) != 0) {
        // Segment is executable.
        prot |= PROT_EXEC;
      }
      
      // Length of segment in memory.  We can map memory up the next
      // page boundary beyond the end of the file.  If the memsz is
      // beyond that we need to mmap a new ANON segment for it.
      int64_t length = segment->filesz + (segment->vaddr & page_mask);
      length = (length + page_mask) & ~page_mask;
      
      // Open the ELF file again to get a file descriptor that we can use
      // for mmap.
      int fd = open(filename->value, file_prot);
      if (fd < 0) {
        printf("Failed to open ELF file segment\n");
        return false;
      }
      
      if (length == 0) {
        continue;
      }
      
      int flags = MAP_PRIVATE;
      if (addr != 0) {
        flags |= MAP_FIXED;
      }
      // Map in the segment at the address specified in the ELF file.  This is done using
      // the MAP_PRIVATE flag so that the pages are all copy-on-write, meaning that they
      // will be copied to a new physical address if they are written to, otherwise they
      // are shared with other physical pages that map the same file in.  The MAP_FIXED
      // flag says that we are providing the address to map the pages at.  Normally mmap
      // chooses the address, but in this case we need them at the same virtual address
      // that he Loader chose and specified in the segment header.
      void* segment_ptr = mmap((void*)addr, length, prot, flags, fd, offset);
      if (segment_ptr == MAP_FAILED) {
        printf("Failed to map in ELF segment: %s\n", strerror(errno));
        return false;
      }
      
      // Don't need the file descriptor now.
      close(fd);
      
      // Zero out any difference between memsz and filesz.  This will really only
      // be the .bss section.  We have mapped the contents of the file but some of
      // it will need to be zeroed out.  We also need to allocate a contiguous
      // anonymous region of zeros above the segment.
      // We need the .bss to be all zeroes before thae program starts.
      if ((prot & PROT_WRITE) != 0) {
        void* zeroed_region = (loader->arch->ignore_vaddr ?
                               segment_ptr : (char*)segment->vaddr) +
            segment->filesz;   // Start of zero memory.
        // Calculate end of mapped memory.  The 'length' contains the total length
        // of the mapped memory.
        uint64_t end_of_mapped_memory = (loader->arch->ignore_vaddr ?
                                         (uint64_t)segment_ptr : addr) +
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
            return false;
          }
          VectorAppend(&loader->regions, NewRegion(zero, additional_memory));
        }
      }
      
      // Add a new region to the regions vector so that we can remove it
      // when destructed.
      Region* region = NewRegion(segment_ptr, length);
      VectorAppend(&loader->regions, region);
      GetRegionSections(loader, region, segment, i);
    }
  }
  return true;
}


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
  uint64_t load_addr = 0x600000000LL;
  loader->dynamic_lib = NewLoadedDynamicLibrary(loader->filename.value,
                                                loader);
  DynamicLibraryRegistryInsert(&loader->loaded_libraries, loader->dynamic_lib);
  bool ok = LoadedDynamicLibraryLoad(loader->dynamic_lib, &loader->loaded_libraries,
                                     &loader->library_search_path,
                                     &load_addr);
  
   if (!ok) {
     LoaderError("Unable to find %s", loader->dynamic_lib->libname.value);
     LoadedDynamicLibraryDelete(loader->dynamic_lib);
     return false;
  }
  
  // Relocate all the loaded libraries.
  for (size_t i = 0; i < loader->loaded_libraries.search.length; i++) {
    LoadedDynamicLibrary* lib = loader->loaded_libraries.search.value.p[i];
    LoadedDynamicLibraryRelocate(loader, lib,
                                 &loader->loaded_libraries,
                                 lazy);
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
      char* p = ld_library_path;
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
                        LoaderArchitecture* arch, void* arch_data) {
  loader->arch = arch;
  loader->arch_data = arch_data;
  loader->flags = flags;
  
  // Load the ELF file.
  loader->elf_file = NewELFReaderFile(filename);
  bool ok = ELFReaderFileRead(loader->elf_file, 0, 0);
  if (!ok) {
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
  InitLibrarySearchPath(loader);
  StringInit(&loader->filename, filename->value);
  
  // Initialize regions vector.
  VectorInit(&loader->regions);

  // Add the whole ELF file to the regions vector.
  VectorAppend(&loader->regions, NewRegion(loader->elf_file->header,
                                           loader->elf_file->file_length));

  // Get the entry point address.
  loader->main_address = loader->elf_file->header->entry;
  
  loader->dynamic = FindDynamicSection(loader);
  if (loader->dynamic == NULL) {
    // No dynamic segment means this is a fully static executable.
    if (loader->elf_file->header->type != ET(exec)) {
      return false;
    }
    ok = LoadStaticSegments(loader, filename);
  } else {
    // This is a dynamic executable.
    ok = LoadDynamic(loader, (flags & LOADER_LAZY_RESOLVE) != 0);
  }
    
  memset(&loader->current_symbol, 0, sizeof(SymbolScope));
  return ok;
}

void LoaderDestruct(Loader* loader) {
  // Unmap all regions.
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    munmap(region->address, region->length);
  }
  DynamicLibraryRegistryInit(&loader->loaded_libraries);
  
  VectorDestructWithContents(&loader->regions,
                             (VectorElementDestructor)RegionDestruct);
}
