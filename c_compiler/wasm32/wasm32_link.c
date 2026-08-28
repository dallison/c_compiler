//
//  wasm32_link.c
//  c_compiler
//
//  Merging relocatable wasm objects into a runnable module.
//
//  The shape of the job is different from an ELF link.  There are no
//  addresses to assign to code, because wasm code is reached by index rather
//  than by address, so most of the work is renumbering: every object numbers
//  its own functions, types and table slots from zero, and the link has to
//  agree on one numbering and rewrite every reference to match.  Data is the
//  one thing that does get addresses, because linear memory is flat.
//

#include "wasm32_link.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ar.h"
#include "map.h"
#include "vector.h"
#include "wasm32_machine.h"
#include "wasm32_object.h"

// Data is grouped so that read-only bytes, writable bytes and zeroed space
// each end up contiguous; .bss last is what lets its bytes stay out of the
// module.  The initializer arrays get groups of their own because libc walks
// each as one array, which it can only do if nothing else lies between the
// pieces the objects contributed.
enum {
  kSegmentRodata,
  kSegmentPreinitArray,
  kSegmentInitArray,
  kSegmentFiniArray,
  kSegmentData,
  kSegmentBss,
  kSegmentGroups
};

typedef struct {
  Vector objects;  // Wasm32ObjectFile* for everything read.
  Vector search_path;
  Map globals;  // char* name -> Wasm32Symbol* definition or first reference.

  // What each archive member would define, so that a member is only read
  // into the link once something actually needs it.
  Map archive_index;  // char* name -> Wasm32ObjectFile* not yet live.

  String output;
  const char* entry;
  bool failed;

  // Calls the host supplies.  They occupy the bottom of the function index
  // space, so every defined function is numbered after them.
  Vector imports;    // Wasm32Symbol* in import index order.
  Map import_index;  // char* name -> Wasm32Symbol* already imported.

  // Layout results.
  uint32_t group_start[kSegmentGroups];
  uint32_t group_end[kSegmentGroups];
  uint32_t data_end;
  uint32_t stack_top;
  uint32_t num_table_slots;
  Vector table_functions;  // Wasm32Symbol* in slot order, from slot 1.
  Vector types;            // Buffer* merged type table.
  Vector functions;        // Wasm32Function* in final index order.
} Wasm32Linker;

static void Fail(Wasm32Linker* linker, const char* format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "wasm32-ld: ");
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
  va_end(args);
  linker->failed = true;
}

static uint32_t AlignUp(uint32_t value, uint32_t alignment) {
  if (alignment < 1) {
    alignment = 1;
  }
  return (value + alignment - 1) & ~(alignment - 1);
}

static bool IsLocal(Wasm32Symbol* symbol) {
  return (symbol->flags & WASM_SYM_BINDING_LOCAL) != 0;
}

// Names whose addresses are the linker's to decide rather than any object's:
// the edges of the heap, and the bounds of each initializer array, which
// libc walks but no object can see the whole of.
typedef enum {
  kAddressDataEnd,
  kAddressHeapBase,
  kAddressHeapEnd,
  kAddressGroupStart,
  kAddressGroupEnd
} LinkerAddress;

typedef struct {
  const char* name;
  LinkerAddress kind;
  int group;  // Only for the group bounds.
} LinkerDefinedSymbol;

static const LinkerDefinedSymbol linker_defined[] = {
    {WASM32_DATA_END, kAddressDataEnd, 0},
    {WASM32_HEAP_BASE, kAddressHeapBase, 0},
    {WASM32_HEAP_END, kAddressHeapEnd, 0},
    {"__preinit_array_start", kAddressGroupStart, kSegmentPreinitArray},
    {"__preinit_array_end", kAddressGroupEnd, kSegmentPreinitArray},
    {"__init_array_start", kAddressGroupStart, kSegmentInitArray},
    {"__init_array_end", kAddressGroupEnd, kSegmentInitArray},
    {"__fini_array_start", kAddressGroupStart, kSegmentFiniArray},
    {"__fini_array_end", kAddressGroupEnd, kSegmentFiniArray},
};

static const LinkerDefinedSymbol* FindLinkerDefined(const char* name) {
  for (size_t i = 0; i < sizeof(linker_defined) / sizeof(linker_defined[0]);
       i++) {
    if (strcmp(name, linker_defined[i].name) == 0) {
      return &linker_defined[i];
    }
  }
  return NULL;
}

static bool IsDefined(Wasm32Symbol* symbol) {
  return (symbol->flags & WASM_SYM_UNDEFINED) == 0;
}

static bool IsWeak(Wasm32Symbol* symbol) {
  return (symbol->flags & WASM_SYM_BINDING_WEAK) != 0;
}

// Reading inputs.

static uint8_t* ReadWholeFile(const char* filename, size_t* length) {
  FILE* fp = fopen(filename, "rb");
  if (fp == NULL) {
    return NULL;
  }
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (size < 0) {
    fclose(fp);
    return NULL;
  }
  uint8_t* bytes = malloc((size_t)size + 1);
  size_t got = fread(bytes, 1, (size_t)size, fp);
  fclose(fp);
  if (got != (size_t)size) {
    free(bytes);
    return NULL;
  }
  *length = got;
  return bytes;
}

// The object copies everything it needs out of the bytes, so they only have
// to live as long as the parse.
static Wasm32ObjectFile* ReadObject(Wasm32Linker* linker, const char* filename,
                                    const uint8_t* bytes, size_t length) {
  Wasm32ObjectFile* object = malloc(sizeof(Wasm32ObjectFile));
  if (!Wasm32ReadObjectFile(object, filename, bytes, length)) {
    Wasm32ObjectFileDestruct(object);
    free(object);
    linker->failed = true;
    return NULL;
  }
  VectorAppend(&linker->objects, object);
  return object;
}

static bool IsWasmObject(const uint8_t* bytes, size_t length) {
  return length >= 8 && memcmp(bytes, "\0asm", 4) == 0;
}

// Note what an archive member would define without reading it into the link.
// The first member to define a name is the one that will be pulled in, which
// is the rule every other linker uses.
static void IndexArchiveMember(Wasm32Linker* linker,
                               Wasm32ObjectFile* member) {
  for (size_t i = 0; i < member->symbols.length; i++) {
    Wasm32Symbol* symbol = member->symbols.value.p[i];
    if (IsLocal(symbol) || !IsDefined(symbol) ||
        symbol->kind == WASM_SYMBOL_GLOBAL) {
      continue;
    }
    if (MapFindPointerKey(&linker->archive_index, symbol->name) != NULL) {
      continue;
    }
    MapKeyValue kv = {{.p = symbol->name}, {.p = member}};
    MapInsert(&linker->archive_index, kv);
  }
}

static bool AddArchive(Wasm32Linker* linker, const char* filename, FILE* fp) {
  ARArchive archive;
  ARArchiveInit(&archive, filename);
  if (!ARArchiveOpen(&archive, fp)) {
    Fail(linker, "%s is not an archive this linker understands", filename);
    ARArchiveDestruct(&archive);
    return false;
  }

  for (size_t i = 0; i < archive.files.length; i++) {
    ARFile* file = archive.files.value.p[i];
    if (file->deleted || file->size <= 0) {
      continue;
    }
    uint8_t* bytes = malloc((size_t)file->size);
    if (fseek(fp, file->file_offset, SEEK_SET) != 0 ||
        fread(bytes, 1, (size_t)file->size, fp) != (size_t)file->size) {
      Fail(linker, "cannot read %s(%s)", filename, file->filename.value);
      free(bytes);
      break;
    }
    // An archive holds its symbol index and long filenames as members too,
    // and those are not objects.
    if (!IsWasmObject(bytes, (size_t)file->size)) {
      free(bytes);
      continue;
    }
    String name;
    StringInit(&name, "");
    StringPrintf(&name, "%s(%s)", filename, file->filename.value);
    Wasm32ObjectFile* member =
        ReadObject(linker, name.value, bytes, (size_t)file->size);
    StringDestruct(&name);
    free(bytes);
    if (member == NULL) {
      break;
    }
    member->is_live = false;
    IndexArchiveMember(linker, member);
  }

  ARArchiveDestruct(&archive);
  return !linker->failed;
}

// An input is an object or an archive; which one is settled by what is in
// it rather than by what it is called.
static bool AddInputFile(Wasm32Linker* linker, const char* filename) {
  FILE* fp = fopen(filename, "rb");
  if (fp == NULL) {
    Fail(linker, "cannot read %s", filename);
    return false;
  }
  char magic[8];
  bool is_archive = fread(magic, 1, sizeof(magic), fp) == sizeof(magic) &&
                    memcmp(magic, AR_MAGIC, sizeof(magic)) == 0;
  if (is_archive) {
    rewind(fp);
    bool ok = AddArchive(linker, filename, fp);
    fclose(fp);
    return ok;
  }
  fclose(fp);

  size_t length = 0;
  uint8_t* bytes = ReadWholeFile(filename, &length);
  if (bytes == NULL) {
    Fail(linker, "cannot read %s", filename);
    return false;
  }
  Wasm32ObjectFile* object = ReadObject(linker, filename, bytes, length);
  free(bytes);
  return object != NULL;
}

// -lfoo means libfoo.a somewhere on the search path.
static bool AddLibrary(Wasm32Linker* linker, const char* name) {
  for (size_t i = 0; i < linker->search_path.length; i++) {
    String path;
    StringInit(&path, "");
    StringPrintf(&path, "%s/lib%s.a", (const char*)linker->search_path.value.p[i],
                 name);
    FILE* probe = fopen(path.value, "rb");
    if (probe != NULL) {
      fclose(probe);
      bool ok = AddInputFile(linker, path.value);
      StringDestruct(&path);
      return ok;
    }
    StringDestruct(&path);
  }
  Fail(linker, "cannot find -l%s", name);
  return false;
}

// Symbol resolution.

// Merge one object's symbols into the global table.  A definition displaces
// a reference, and a strong definition displaces a weak one; two strong
// definitions of the same name are an error.
static void MergeSymbols(Wasm32Linker* linker, Wasm32ObjectFile* object) {
  for (size_t i = 0; i < object->symbols.length; i++) {
    Wasm32Symbol* symbol = object->symbols.value.p[i];
    if (IsLocal(symbol) || symbol->kind == WASM_SYMBOL_GLOBAL) {
      continue;
    }
    Wasm32Symbol* existing = MapFindPointerKey(&linker->globals, symbol->name);
    if (existing == NULL) {
      MapKeyValue kv = {{.p = symbol->name}, {.p = symbol}};
      MapInsert(&linker->globals, kv);
      continue;
    }
    if (!IsDefined(symbol)) {
      continue;
    }
    if (!IsDefined(existing) || (IsWeak(existing) && !IsWeak(symbol))) {
      MapKeyValue kv = {{.p = symbol->name}, {.p = symbol}};
      MapInsert(&linker->globals, kv);
      continue;
    }
    if (IsDefined(existing) && !IsWeak(existing) && !IsWeak(symbol)) {
      Wasm32ObjectFile* owner = existing->owner;
      Fail(linker, "'%s' is defined in both %s and %s", symbol->name,
           owner->filename.value, object->filename.value);
    }
  }
}

// Pull in archive members until nothing else is needed.  A member that is
// brought in can reference something only another member defines, so this
// has to keep going round until a pass changes nothing.  A weak reference
// never pulls anything in, which is what makes it weak.
static void ResolveFromArchives(Wasm32Linker* linker) {
  // Nothing calls the entry point, so it would never be asked for; it is a
  // root of the link rather than something reached from one.  A program
  // built without a runtime has no entry at all, which is not an error.
  Wasm32Symbol* entry = MapFindPointerKey(&linker->globals, linker->entry);
  if (entry == NULL || !IsDefined(entry)) {
    Wasm32ObjectFile* member =
        MapFindPointerKey(&linker->archive_index, linker->entry);
    if (member != NULL && !member->is_live) {
      member->is_live = true;
      MergeSymbols(linker, member);
    }
  }

  bool changed = true;
  while (changed && !linker->failed) {
    changed = false;
    for (size_t i = 0; i < linker->objects.length; i++) {
      Wasm32ObjectFile* object = linker->objects.value.p[i];
      if (!object->is_live) {
        continue;
      }
      for (size_t j = 0; j < object->symbols.length; j++) {
        Wasm32Symbol* symbol = object->symbols.value.p[j];
        if (IsLocal(symbol) || IsDefined(symbol) || IsWeak(symbol) ||
            symbol->kind == WASM_SYMBOL_GLOBAL) {
          continue;
        }
        Wasm32Symbol* known =
            MapFindPointerKey(&linker->globals, symbol->name);
        if (known != NULL && IsDefined(known)) {
          continue;
        }
        Wasm32ObjectFile* member =
            MapFindPointerKey(&linker->archive_index, symbol->name);
        if (member == NULL || member->is_live) {
          continue;
        }
        member->is_live = true;
        MergeSymbols(linker, member);
        changed = true;
      }
    }
  }
}

// A call the host answers needs no definition, only an import, and one
// import serves every object that calls it.  The first symbol to ask for it
// becomes the one the others are bound to, which is also the one whose
// declared signature the import is written with.
static Wasm32Symbol* ImportForHostCall(Wasm32Linker* linker,
                                       Wasm32Symbol* symbol) {
  Wasm32Symbol* existing = MapFindPointerKey(&linker->import_index,
                                             symbol->name);
  if (existing != NULL) {
    return existing;
  }
  symbol->final_index = (int32_t)linker->imports.length;
  VectorAppend(&linker->imports, symbol);
  MapKeyValue kv = {{.p = symbol->name}, {.p = symbol}};
  MapInsert(&linker->import_index, kv);
  return symbol;
}

// Point every reference at the definition the link settled on.  A local
// symbol is its own definition and never appears in the global table.
static void BindSymbols(Wasm32Linker* linker) {
  for (size_t i = 0; i < linker->objects.length; i++) {
    Wasm32ObjectFile* object = linker->objects.value.p[i];
    if (!object->is_live) {
      continue;
    }
    for (size_t j = 0; j < object->symbols.length; j++) {
      Wasm32Symbol* symbol = object->symbols.value.p[j];
      if (IsLocal(symbol)) {
        symbol->definition = symbol;
        continue;
      }
      // The stack pointer is the linker's to define, so an object naming it
      // is not looking for a definition anywhere else.
      if (symbol->kind == WASM_SYMBOL_GLOBAL &&
          strcmp(symbol->name, WASM32_STACK_POINTER) == 0) {
        symbol->definition = symbol;
        symbol->final_index = 0;
        continue;
      }
      // Nor are the addresses the linker settles itself, which fall out of
      // the layout and so are filled in once that has been done.
      if (symbol->kind == WASM_SYMBOL_DATA &&
          FindLinkerDefined(symbol->name) != NULL) {
        symbol->flags &= ~WASM_SYM_UNDEFINED;
        symbol->definition = symbol;
        continue;
      }
      Wasm32Symbol* definition =
          MapFindPointerKey(&linker->globals, symbol->name);
      symbol->definition = definition;
      if (definition == NULL || !IsDefined(definition)) {
        if (symbol->kind == WASM_SYMBOL_FUNCTION &&
            Wasm32WasiFieldName(symbol->name) != NULL) {
          symbol->definition = ImportForHostCall(linker, symbol);
          continue;
        }
        // A weak reference that nothing defines is a null pointer, which is
        // exactly what leaving it at zero produces.
        if (!IsWeak(symbol)) {
          Fail(linker, "undefined symbol '%s', referenced by %s", symbol->name,
               object->filename.value);
        }
        symbol->definition = NULL;
      }
    }
  }
}

// Layout.

static int SegmentClass(Wasm32Segment* segment) {
  const char* name = segment->name == NULL ? "" : segment->name;
  if (strncmp(name, ".bss", 4) == 0) {
    return kSegmentBss;
  }
  if (strncmp(name, ".rodata", 7) == 0) {
    return kSegmentRodata;
  }
  if (strncmp(name, ".preinit_array", 14) == 0) {
    return kSegmentPreinitArray;
  }
  if (strncmp(name, ".init_array", 11) == 0) {
    return kSegmentInitArray;
  }
  if (strncmp(name, ".fini_array", 11) == 0) {
    return kSegmentFiniArray;
  }
  return kSegmentData;
}

static bool IsBss(Wasm32Segment* segment) {
  return SegmentClass(segment) == kSegmentBss;
}

// How much linear memory the module starts with: the data and the shadow
// stack, and then a heap on top of them.  Only meaningful once the layout
// has settled the stack's position.
static uint32_t MemoryPages(Wasm32Linker* linker) {
  uint32_t pages =
      (linker->stack_top + 0xFFFF) / 0x10000 + WASM32_DEFAULT_HEAP_PAGES;
  return pages < WASM32_DEFAULT_MEMORY_PAGES ? WASM32_DEFAULT_MEMORY_PAGES
                                             : pages;
}

static void LayOutData(Wasm32Linker* linker) {
  uint32_t address = WASM32_DATA_START;
  for (int group = 0; group < kSegmentGroups; group++) {
    // A group is known by the bounds of what it holds, not by where the group
    // before it stopped: libc walks the initializer arrays as arrays, so any
    // padding left ahead of the first segment would be read as an entry.
    linker->group_start[group] = address;
    linker->group_end[group] = address;
    bool started = false;
    for (size_t i = 0; i < linker->objects.length; i++) {
      Wasm32ObjectFile* object = linker->objects.value.p[i];
      if (!object->is_live) {
        continue;
      }
      for (size_t j = 0; j < object->segments.length; j++) {
        Wasm32Segment* segment = object->segments.value.p[j];
        if (SegmentClass(segment) != group) {
          continue;
        }
        uint32_t size = segment->size;
        if (size < segment->bytes.length) {
          size = (uint32_t)segment->bytes.length;
        }
        address = AlignUp(address, 1u << segment->alignment);
        if (!started) {
          linker->group_start[group] = address;
          started = true;
        }
        segment->address = (int32_t)address;
        address += size;
        linker->group_end[group] = address;
      }
    }
  }
  linker->data_end = address;

  for (size_t i = 0; i < linker->objects.length; i++) {
    Wasm32ObjectFile* object = linker->objects.value.p[i];
    if (!object->is_live) {
      continue;
    }
    for (size_t j = 0; j < object->symbols.length; j++) {
      Wasm32Symbol* symbol = object->symbols.value.p[j];
      if (symbol->kind != WASM_SYMBOL_DATA || !IsDefined(symbol) ||
          FindLinkerDefined(symbol->name) != NULL) {
        continue;
      }
      if (symbol->segment >= object->segments.length) {
        Fail(linker, "'%s' names a data segment %s does not have",
             symbol->name, object->filename.value);
        continue;
      }
      Wasm32Segment* segment = object->segments.value.p[symbol->segment];
      symbol->address = segment->address + (int32_t)symbol->offset;
    }
  }

  // The shadow stack sits above the data and grows down from its top.
  linker->stack_top = AlignUp(address, 16) + WASM32_STACK_SIZE;

  // Nothing in linear memory says where anything is, so every address libc
  // needs but no object could know has to be handed to it by name.
  for (size_t i = 0; i < linker->objects.length; i++) {
    Wasm32ObjectFile* object = linker->objects.value.p[i];
    if (!object->is_live) {
      continue;
    }
    for (size_t j = 0; j < object->symbols.length; j++) {
      Wasm32Symbol* symbol = object->symbols.value.p[j];
      if (symbol->kind != WASM_SYMBOL_DATA) {
        continue;
      }
      const LinkerDefinedSymbol* defined = FindLinkerDefined(symbol->name);
      if (defined == NULL) {
        continue;
      }
      switch (defined->kind) {
        case kAddressDataEnd:
          symbol->address = (int32_t)linker->data_end;
          break;
        case kAddressHeapBase:
          symbol->address = (int32_t)linker->stack_top;
          break;
        case kAddressHeapEnd:
          symbol->address = (int32_t)(MemoryPages(linker) * 0x10000);
          break;
        case kAddressGroupStart:
          symbol->address = (int32_t)linker->group_start[defined->group];
          break;
        case kAddressGroupEnd:
          symbol->address = (int32_t)linker->group_end[defined->group];
          break;
      }
    }
  }
}

// Every function that survives gets one index, in the order the objects were
// given, which keeps the output deterministic.  Imports were numbered first
// and are not in this space's gift, so it starts above them.
static void AssignFunctionIndices(Wasm32Linker* linker) {
  for (size_t i = 0; i < linker->objects.length; i++) {
    Wasm32ObjectFile* object = linker->objects.value.p[i];
    if (!object->is_live) {
      continue;
    }
    for (size_t j = 0; j < object->functions.length; j++) {
      Wasm32Function* function = object->functions.value.p[j];
      if (function->symbol != NULL) {
        function->symbol->final_index =
            (int32_t)(linker->imports.length + linker->functions.length);
      }
      VectorAppend(&linker->functions, function);
    }
  }
}

// Only a function whose address is taken needs a table slot, so the table
// stays as small as the program actually needs.  Slot 0 is the null one.
static void AssignTableSlot(Wasm32Linker* linker, Wasm32Symbol* symbol) {
  if (symbol == NULL || symbol->table_slot >= 0) {
    return;
  }
  symbol->table_slot = (int32_t)linker->table_functions.length + 1;
  VectorAppend(&linker->table_functions, symbol);
}

static void AssignTableSlots(Wasm32Linker* linker) {
  for (size_t i = 0; i < linker->objects.length; i++) {
    Wasm32ObjectFile* object = linker->objects.value.p[i];
    if (!object->is_live) {
      continue;
    }
    for (size_t j = 0; j < object->functions.length; j++) {
      Wasm32Function* function = object->functions.value.p[j];
      for (size_t k = 0; k < function->relocs.length; k++) {
        Wasm32Reloc* reloc = function->relocs.value.p[k];
        if (reloc->type == R_WASM_TABLE_INDEX_SLEB) {
          Wasm32Symbol* symbol = object->symbols.value.p[reloc->index];
          AssignTableSlot(linker, symbol->definition);
        }
      }
    }
    for (size_t j = 0; j < object->segments.length; j++) {
      Wasm32Segment* segment = object->segments.value.p[j];
      for (size_t k = 0; k < segment->relocs.length; k++) {
        Wasm32Reloc* reloc = segment->relocs.value.p[k];
        if (reloc->type == R_WASM_TABLE_INDEX_I32) {
          Wasm32Symbol* symbol = object->symbols.value.p[reloc->index];
          AssignTableSlot(linker, symbol->definition);
        }
      }
    }
  }
  linker->num_table_slots = (uint32_t)linker->table_functions.length + 1;
}

// One type table for the module, with each object's indices remapped onto
// it.  The map is stored on the object so relocation can use it.
static void MergeTypes(Wasm32Linker* linker, Vector* maps) {
  for (size_t i = 0; i < linker->objects.length; i++) {
    Wasm32ObjectFile* object = linker->objects.value.p[i];
    Vector* map = malloc(sizeof(Vector));
    VectorInit(map);
    VectorAppend(maps, map);
    if (!object->is_live) {
      continue;
    }
    for (size_t j = 0; j < object->types.length; j++) {
      Buffer* encoding = object->types.value.p[j];
      int index = -1;
      for (size_t k = 0; k < linker->types.length; k++) {
        if (BufferCompare(linker->types.value.p[k], encoding) == 0) {
          index = (int)k;
          break;
        }
      }
      if (index < 0) {
        Buffer* copy = NewBuffer();
        BufferAppend(copy, encoding->value, encoding->length);
        VectorAppend(&linker->types, copy);
        index = (int)(linker->types.length - 1);
      }
      VectorAppend(map, (void*)(intptr_t)index);
    }
  }
}

// Relocation.

static void ApplyReloc(Wasm32Linker* linker, Wasm32ObjectFile* object,
                       Buffer* bytes, Wasm32Reloc* reloc, Vector* type_map) {
  if (reloc->index >= object->symbols.length &&
      reloc->type != R_WASM_TYPE_INDEX_LEB) {
    Fail(linker, "%s has a relocation against a symbol it does not have",
         object->filename.value);
    return;
  }

  if (reloc->type == R_WASM_TYPE_INDEX_LEB) {
    if (reloc->index >= type_map->length) {
      Fail(linker, "%s has a relocation against a type it does not have",
           object->filename.value);
      return;
    }
    WasmPatchPaddedU32(bytes, reloc->offset,
                       (uint32_t)(intptr_t)type_map->value.p[reloc->index]);
    return;
  }

  Wasm32Symbol* symbol = object->symbols.value.p[reloc->index];
  Wasm32Symbol* definition = symbol->definition;

  switch (reloc->type) {
    case R_WASM_FUNCTION_INDEX_LEB:
      if (definition == NULL || definition->final_index < 0) {
        // Only reachable for a weak reference nothing defined; calling it
        // is a trap rather than a jump into whatever is at index zero.
        WasmPatchPaddedU32(bytes, reloc->offset, 0);
        break;
      }
      WasmPatchPaddedU32(bytes, reloc->offset,
                         (uint32_t)definition->final_index);
      break;
    case R_WASM_TABLE_INDEX_SLEB:
      WasmPatchPaddedU32(
          bytes, reloc->offset,
          definition == NULL ? 0 : (uint32_t)definition->table_slot);
      break;
    case R_WASM_TABLE_INDEX_I32:
      WasmPatchU32(bytes, reloc->offset,
                   definition == NULL ? 0 : (uint32_t)definition->table_slot);
      break;
    case R_WASM_MEMORY_ADDR_LEB:
    case R_WASM_MEMORY_ADDR_SLEB:
      WasmPatchPaddedU32(
          bytes, reloc->offset,
          definition == NULL ? 0
                             : (uint32_t)(definition->address + reloc->addend));
      break;
    case R_WASM_MEMORY_ADDR_I32:
      WasmPatchU32(bytes, reloc->offset,
                   definition == NULL
                       ? 0
                       : (uint32_t)(definition->address + reloc->addend));
      break;
    case R_WASM_GLOBAL_INDEX_LEB:
      // The stack pointer is the only global, and it is global 0.
      WasmPatchPaddedU32(bytes, reloc->offset, 0);
      break;
    default:
      Fail(linker, "%s uses relocation type %d, which is not supported",
           object->filename.value, reloc->type);
      break;
  }
}

static void ApplyRelocations(Wasm32Linker* linker, Vector* type_maps) {
  for (size_t i = 0; i < linker->objects.length; i++) {
    Wasm32ObjectFile* object = linker->objects.value.p[i];
    if (!object->is_live) {
      continue;
    }
    Vector* type_map = type_maps->value.p[i];
    for (size_t j = 0; j < object->functions.length; j++) {
      Wasm32Function* function = object->functions.value.p[j];
      for (size_t k = 0; k < function->relocs.length; k++) {
        ApplyReloc(linker, object, &function->body,
                   function->relocs.value.p[k], type_map);
      }
    }
    for (size_t j = 0; j < object->segments.length; j++) {
      Wasm32Segment* segment = object->segments.value.p[j];
      for (size_t k = 0; k < segment->relocs.length; k++) {
        ApplyReloc(linker, object, &segment->bytes, segment->relocs.value.p[k],
                   type_map);
      }
    }
  }
}

// Emission.

static void WriteOpcodeByte(Buffer* buf, int byte) {
  BufferAppendByte(buf, (char)byte);
}

// Where a symbol's declared signature ended up in the merged type table.
// The signature is recorded as an index into the table of whichever object
// the symbol was read from, so getting at it means going back to that
// object's map.
static uint32_t MergedTypeIndex(Wasm32Linker* linker, Vector* type_maps,
                                Wasm32Symbol* symbol) {
  if (symbol->type_index == WASM32_NO_TYPE) {
    Fail(linker, "nothing says what signature '%s' has", symbol->name);
    return 0;
  }
  for (size_t i = 0; i < linker->objects.length; i++) {
    if (linker->objects.value.p[i] != symbol->owner) {
      continue;
    }
    Vector* map = type_maps->value.p[i];
    if (symbol->type_index >= map->length) {
      break;
    }
    return (uint32_t)(intptr_t)map->value.p[symbol->type_index];
  }
  Fail(linker, "'%s' has a signature its object does not", symbol->name);
  return 0;
}

static bool WriteModule(Wasm32Linker* linker, Vector* type_maps) {
  Buffer module;
  BufferInit(&module);
  BufferAppendByte(&module, 0x00);
  BufferAppendByte(&module, 0x61);
  BufferAppendByte(&module, 0x73);
  BufferAppendByte(&module, 0x6D);
  BufferAppendWordLE(&module, 1);

  Buffer section;
  BufferInit(&section);

  WasmWriteULEB128(&section, linker->types.length);
  for (size_t i = 0; i < linker->types.length; i++) {
    Buffer* encoding = linker->types.value.p[i];
    BufferAppend(&section, encoding->value, encoding->length);
  }
  WasmWriteSection(&module, WASM_SECTION_TYPE, &section);

  // Import section: the calls the host answers.  These take the low function
  // indices, which is why the function section starts counting above them.
  if (linker->imports.length > 0) {
    BufferClear(&section);
    WasmWriteULEB128(&section, linker->imports.length);
    for (size_t i = 0; i < linker->imports.length; i++) {
      Wasm32Symbol* symbol = linker->imports.value.p[i];
      WasmWriteName(&section, WASM32_WASI_MODULE);
      WasmWriteName(&section, Wasm32WasiFieldName(symbol->name));
      BufferAppendByte(&section, WASM_EXTERN_FUNC);
      WasmWriteULEB128(&section, MergedTypeIndex(linker, type_maps, symbol));
    }
    WasmWriteSection(&module, WASM_SECTION_IMPORT, &section);
  }

  // Function section, in the same order the code section will use.
  BufferClear(&section);
  WasmWriteULEB128(&section, linker->functions.length);
  for (size_t i = 0; i < linker->objects.length; i++) {
    Wasm32ObjectFile* object = linker->objects.value.p[i];
    if (!object->is_live) {
      continue;
    }
    Vector* type_map = type_maps->value.p[i];
    for (size_t j = 0; j < object->functions.length; j++) {
      Wasm32Function* function = object->functions.value.p[j];
      uint32_t type_index = function->type_index < type_map->length
                                ? (uint32_t)(intptr_t)
                                      type_map->value.p[function->type_index]
                                : 0;
      WasmWriteULEB128(&section, type_index);
    }
  }
  WasmWriteSection(&module, WASM_SECTION_FUNCTION, &section);

  // Table section: one funcref table with a slot for every function whose
  // address is taken, plus the null slot.
  BufferClear(&section);
  WasmWriteULEB128(&section, 1);
  BufferAppendByte(&section, (char)kWasmTypeFuncRef);
  BufferAppendByte(&section, 0x01);  // Both a minimum and a maximum.
  WasmWriteULEB128(&section, linker->num_table_slots);
  WasmWriteULEB128(&section, linker->num_table_slots);
  WasmWriteSection(&module, WASM_SECTION_TABLE, &section);

  // Memory section: enough pages for the data, the shadow stack and a heap.
  BufferClear(&section);
  WasmWriteULEB128(&section, 1);
  BufferAppendByte(&section, 0x00);  // No maximum.
  WasmWriteULEB128(&section, MemoryPages(linker));
  WasmWriteSection(&module, WASM_SECTION_MEMORY, &section);

  // Global section: the shadow stack pointer.
  BufferClear(&section);
  WasmWriteULEB128(&section, 1);
  BufferAppendByte(&section, (char)kWasmTypeI32);
  BufferAppendByte(&section, 0x01);  // Mutable.
  WriteOpcodeByte(&section, 0x41);   // i32.const
  WasmWriteSLEB128(&section, linker->stack_top);
  WriteOpcodeByte(&section, 0x0B);  // end
  WasmWriteSection(&module, WASM_SECTION_GLOBAL, &section);

  // Export section: the memory, so a host can see the heap, plus every
  // function with external linkage.
  BufferClear(&section);
  size_t num_exports = 1;
  for (size_t i = 0; i < linker->functions.length; i++) {
    Wasm32Function* function = linker->functions.value.p[i];
    if (function->symbol != NULL && !IsLocal(function->symbol)) {
      num_exports++;
    }
  }
  WasmWriteULEB128(&section, num_exports);
  WasmWriteName(&section, "memory");
  BufferAppendByte(&section, WASM_EXTERN_MEMORY);
  WasmWriteULEB128(&section, 0);
  for (size_t i = 0; i < linker->functions.length; i++) {
    Wasm32Function* function = linker->functions.value.p[i];
    if (function->symbol == NULL || IsLocal(function->symbol)) {
      continue;
    }
    WasmWriteName(&section, function->symbol->name);
    BufferAppendByte(&section, WASM_EXTERN_FUNC);
    WasmWriteULEB128(&section, (uint64_t)function->symbol->final_index);
  }
  WasmWriteSection(&module, WASM_SECTION_EXPORT, &section);

  // Element section: fill the table from slot 1 on.
  if (linker->table_functions.length > 0) {
    BufferClear(&section);
    WasmWriteULEB128(&section, 1);
    WasmWriteULEB128(&section, 0);   // Table 0, active.
    WriteOpcodeByte(&section, 0x41);  // i32.const
    WasmWriteSLEB128(&section, 1);
    WriteOpcodeByte(&section, 0x0B);  // end
    WasmWriteULEB128(&section, linker->table_functions.length);
    for (size_t i = 0; i < linker->table_functions.length; i++) {
      Wasm32Symbol* symbol = linker->table_functions.value.p[i];
      WasmWriteULEB128(&section, (uint64_t)symbol->final_index);
    }
    WasmWriteSection(&module, WASM_SECTION_ELEMENT, &section);
  }

  BufferClear(&section);
  WasmWriteULEB128(&section, linker->functions.length);
  for (size_t i = 0; i < linker->functions.length; i++) {
    Wasm32Function* function = linker->functions.value.p[i];
    WasmWriteULEB128(&section, function->body.length);
    BufferAppend(&section, function->body.value, function->body.length);
  }
  WasmWriteSection(&module, WASM_SECTION_CODE, &section);

  // Data section: one active segment per object segment that has bytes worth
  // writing.  Linear memory starts out zeroed, so .bss needs none.
  BufferClear(&section);
  Buffer segments;
  BufferInit(&segments);
  size_t num_segments = 0;
  for (size_t i = 0; i < linker->objects.length; i++) {
    Wasm32ObjectFile* object = linker->objects.value.p[i];
    if (!object->is_live) {
      continue;
    }
    for (size_t j = 0; j < object->segments.length; j++) {
      Wasm32Segment* segment = object->segments.value.p[j];
      if (IsBss(segment) || segment->bytes.length == 0) {
        continue;
      }
      WasmWriteULEB128(&segments, 0);      // Active, memory 0.
      WriteOpcodeByte(&segments, 0x41);    // i32.const
      WasmWriteSLEB128(&segments, segment->address);
      WriteOpcodeByte(&segments, 0x0B);    // end
      WasmWriteULEB128(&segments, segment->bytes.length);
      BufferAppend(&segments, segment->bytes.value, segment->bytes.length);
      num_segments++;
    }
  }
  if (num_segments > 0) {
    WasmWriteULEB128(&section, num_segments);
    BufferAppend(&section, segments.value, segments.length);
    WasmWriteSection(&module, WASM_SECTION_DATA, &section);
  }
  BufferDestruct(&segments);
  BufferDestruct(&section);

  bool ok = false;
  FILE* fp = fopen(linker->output.value, "wb");
  if (fp == NULL) {
    Fail(linker, "cannot open %s for writing", linker->output.value);
  } else {
    size_t written = fwrite(module.value, 1, module.length, fp);
    ok = fclose(fp) == 0 && written == module.length;
    if (!ok) {
      Fail(linker, "cannot write %s", linker->output.value);
    }
  }
  BufferDestruct(&module);
  return ok;
}

// Driver.

static void LinkerInit(Wasm32Linker* linker) {
  VectorInit(&linker->objects);
  VectorInit(&linker->search_path);
  MapInitForCharPointerKeys(&linker->globals);
  MapInitForCharPointerKeys(&linker->archive_index);
  VectorInit(&linker->imports);
  MapInitForCharPointerKeys(&linker->import_index);
  StringInit(&linker->output, "a.wasm");
  // What a WASI host calls to run a command.  A module built without a
  // runtime has none, and is then something the host reaches into rather
  // than something it runs.
  linker->entry = "_start";
  linker->failed = false;
  linker->data_end = 0;
  linker->stack_top = 0;
  linker->num_table_slots = 1;
  VectorInit(&linker->table_functions);
  VectorInit(&linker->types);
  VectorInit(&linker->functions);
}

static void LinkerDestruct(Wasm32Linker* linker) {
  for (size_t i = 0; i < linker->objects.length; i++) {
    Wasm32ObjectFile* object = linker->objects.value.p[i];
    Wasm32ObjectFileDestruct(object);
    free(object);
  }
  VectorDestruct(&linker->objects);
  VectorDestruct(&linker->search_path);
  MapDestruct(&linker->globals);
  MapDestruct(&linker->archive_index);
  VectorDestruct(&linker->imports);
  MapDestruct(&linker->import_index);
  StringDestruct(&linker->output);
  VectorDestruct(&linker->table_functions);
  for (size_t i = 0; i < linker->types.length; i++) {
    BufferDelete(linker->types.value.p[i]);
  }
  VectorDestruct(&linker->types);
  VectorDestruct(&linker->functions);
}

String* Wasm32Link(int argc, char** argv) {
  Wasm32Linker linker;
  LinkerInit(&linker);

  Vector inputs;
  VectorInit(&inputs);
  for (int i = 1; i < argc; i++) {
    const char* arg = argv[i];
    if (strcmp(arg, "-o") == 0 && i + 1 < argc) {
      StringDestruct(&linker.output);
      StringInit(&linker.output, argv[++i]);
    } else if (strcmp(arg, "-e") == 0 && i + 1 < argc) {
      linker.entry = argv[++i];
    } else if (strncmp(arg, "-L", 2) == 0 && arg[2] != '\0') {
      VectorAppend(&linker.search_path, (void*)(arg + 2));
    } else if (strcmp(arg, "-L") == 0 && i + 1 < argc) {
      VectorAppend(&linker.search_path, (void*)argv[++i]);
    } else if (strncmp(arg, "-l", 2) == 0 && arg[2] != '\0') {
      VectorAppend(&inputs, (void*)arg);
    } else if (arg[0] == '-') {
      // The driver passes flags that only mean something to an ELF link.
      continue;
    } else {
      VectorAppend(&inputs, (void*)arg);
    }
  }

  for (size_t i = 0; i < inputs.length && !linker.failed; i++) {
    const char* input = inputs.value.p[i];
    if (strncmp(input, "-l", 2) == 0) {
      AddLibrary(&linker, input + 2);
    } else {
      AddInputFile(&linker, input);
    }
  }
  VectorDestruct(&inputs);

  // Objects named on the command line are always in; archive members wait
  // to be asked for.
  for (size_t i = 0; i < linker.objects.length; i++) {
    Wasm32ObjectFile* object = linker.objects.value.p[i];
    if (object->is_live) {
      MergeSymbols(&linker, object);
    }
  }
  ResolveFromArchives(&linker);
  BindSymbols(&linker);

  Vector type_maps;
  VectorInit(&type_maps);
  if (!linker.failed) {
    LayOutData(&linker);
    AssignFunctionIndices(&linker);
    AssignTableSlots(&linker);
    MergeTypes(&linker, &type_maps);
    ApplyRelocations(&linker, &type_maps);
  }

  String* result = NULL;
  if (!linker.failed && WriteModule(&linker, &type_maps)) {
    result = NewString(linker.output.value);
  }

  for (size_t i = 0; i < type_maps.length; i++) {
    VectorDelete(type_maps.value.p[i]);
  }
  VectorDestruct(&type_maps);
  LinkerDestruct(&linker);
  return result;
}

bool Wasm32IsTargetName(const char* name) {
  return name != NULL &&
         (strcmp(name, "wasm32") == 0 || strcmp(name, "wasm") == 0);
}
