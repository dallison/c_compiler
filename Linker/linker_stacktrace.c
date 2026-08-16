#include "linker_stacktrace.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "elf.h"
#include "linker.h"
#include "linker_file.h"
#include "linker_symbols.h"

typedef struct {
  Vector symbols;
  ELFWriterSection* section;
  ELFWriterSectionContents* contents;
  size_t pointer_size;
  size_t names_offset;
} LinkerStacktraceInfo;

typedef struct {
  Vector* symbols;
  size_t names_size;
} CollectData;

static size_t TargetPointerSize(const Linker* linker) {
  if (linker->elf_machine_type == ELF_MACHINE_TYPEW65C02) {
    return 2;
  }
  return linker->ops->is_64_bit ? 8 : 4;
}

static void CollectSymbolBucket(void* entry, void* data_ptr) {
  Vector* bucket = entry;
  CollectData* data = data_ptr;
  for (size_t i = 0; i < bucket->length; i++) {
    LinkerSymbol* symbol = bucket->value.p[i];
    if (!symbol->defined || symbol->header == NULL ||
        ELF_ST_TYPE(symbol->header->info) != STT(func) ||
        symbol->size == 0 || symbol->name.length == 0) {
      continue;
    }
    VectorAppend(data->symbols, symbol);
    if (symbol->name.length == SIZE_MAX) {
      data->names_size = SIZE_MAX;
    } else {
      size_t name_size = symbol->name.length + 1;
      if (name_size > SIZE_MAX - data->names_size) {
        data->names_size = SIZE_MAX;
      } else {
        data->names_size += name_size;
      }
    }
  }
}

static ELFWriterSection* NewStacktraceSection(
    ELFWriterSectionContents* contents, size_t pointer_size) {
  ELFWriterSection* section = calloc(1, sizeof(ELFWriterSection));
  StringInit(&section->name, ".davecc_stacktrace");
  section->header.type = SHT(progbits);
  section->header.flags = SHF(alloc);
  section->header.addralign = (int64_t)pointer_size;
  section->contents = contents;
  section->relocations = NewVector();
  return section;
}

static void DefineBoundSymbol(Linker* linker, const char* name,
                              size_t pointer_size, uint64_t address) {
  LinkerSymbol* symbol =
      LinkerFindSymbol(&linker->global_symbol_table, name);
  if (symbol == NULL) {
    symbol = LinkerInventSymbol(linker, name, (int)pointer_size);
  }
  symbol->defined = true;
  symbol->size = pointer_size;
  symbol->address = address;
}

void LinkerStacktracePrepare(Linker* linker) {
  LinkerSymbol* capture =
      LinkerFindSymbol(&linker->global_symbol_table,
                       "__davecc_stacktrace_capture");
  if (capture == NULL || !capture->defined || linker->stacktrace_info != NULL) {
    return;
  }

  LinkerStacktraceInfo* info = calloc(1, sizeof(LinkerStacktraceInfo));
  VectorInit(&info->symbols);
  info->pointer_size = TargetPointerSize(linker);

  CollectData data = {
      .symbols = &info->symbols,
      .names_size = 0,
  };
  HashTableTraverse(&linker->global_symbol_table, CollectSymbolBucket, &data);
  for (size_t i = 0; i < linker->files.length; i++) {
    ObjectFile* file = linker->files.value.p[i];
    HashTableTraverse(&file->local_symbol_table, CollectSymbolBucket, &data);
  }

  size_t record_size = 3 * info->pointer_size;
  if (data.names_size > SIZE_MAX - info->pointer_size ||
      info->symbols.length >
      (SIZE_MAX - info->pointer_size - data.names_size) / record_size) {
    VectorDestruct(&info->symbols);
    free(info);
    LinkerError(NULL, "Stacktrace symbol metadata is too large");
    return;
  }
  info->names_offset =
      info->pointer_size + info->symbols.length * record_size;
  size_t total_size = info->names_offset + data.names_size;

  info->contents =
      NewELFWriterSectionContents(kSectionContentsBuffered);
  char* zeroes = calloc(total_size == 0 ? 1 : total_size, 1);
  BufferAppend(&info->contents->data.buffered, zeroes, total_size);
  free(zeroes);
  info->contents->size = total_size;
  info->section =
      NewStacktraceSection(info->contents, info->pointer_size);

  SectionGroup* group =
      NewSectionGroup(&info->section->name, SHT(progbits), SHF(alloc),
                      (int64_t)info->pointer_size);
  VectorAppend(&group->components, NewGroupedSection(info->section));
  VectorAppend(&linker->section_groups, group);
  linker->stacktrace_info = info;
}

static int CompareSymbolsByAddress(const void* left, const void* right) {
  const LinkerSymbol* a = *(const LinkerSymbol* const*)left;
  const LinkerSymbol* b = *(const LinkerSymbol* const*)right;
  if (a->address < b->address) {
    return -1;
  }
  if (a->address > b->address) {
    return 1;
  }
  if (a->size > b->size) {
    return -1;
  }
  if (a->size < b->size) {
    return 1;
  }
  return strcmp(a->name.value, b->name.value);
}

static void StoreTargetValue(unsigned char* destination, size_t size,
                             uint64_t value) {
  if (size == 2) {
    uint16_t narrowed = (uint16_t)value;
    memcpy(destination, &narrowed, sizeof(narrowed));
  } else if (size == 4) {
    uint32_t narrowed = (uint32_t)value;
    memcpy(destination, &narrowed, sizeof(narrowed));
  } else {
    memcpy(destination, &value, sizeof(value));
  }
}

void LinkerStacktraceFinalize(Linker* linker) {
  LinkerStacktraceInfo* info = linker->stacktrace_info;
  if (info == NULL) {
    return;
  }

  VectorSortPointers(&info->symbols, CompareSymbolsByAddress);
  unsigned char* output =
      (unsigned char*)info->contents->data.buffered.value;
  const uint64_t table_address = info->section->address;
  const size_t record_size = 3 * info->pointer_size;
  size_t name_cursor = info->names_offset;
  size_t count = 0;
  uint64_t previous_address = UINT64_MAX;

  for (size_t i = 0; i < info->symbols.length; i++) {
    LinkerSymbol* symbol = info->symbols.value.p[i];
    if (symbol->address == previous_address) {
      continue;
    }
    if (symbol->address + symbol->size < symbol->address) {
      continue;
    }
    previous_address = symbol->address;
    uint64_t symbol_end = symbol->address + symbol->size;
    for (size_t next_index = i + 1;
         next_index < info->symbols.length; next_index++) {
      LinkerSymbol* next = info->symbols.value.p[next_index];
      if (next->address != symbol->address) {
        if (next->address < symbol_end) {
          symbol_end = next->address;
        }
        break;
      }
    }

    unsigned char* record =
        output + info->pointer_size + count * record_size;
    StoreTargetValue(record, info->pointer_size,
                     symbol->address - table_address);
    StoreTargetValue(record + info->pointer_size, info->pointer_size,
                     symbol_end - table_address);
    StoreTargetValue(record + 2 * info->pointer_size, info->pointer_size,
                     name_cursor);

    memcpy(output + name_cursor, symbol->name.value,
           symbol->name.length + 1);
    name_cursor += symbol->name.length + 1;
    count++;
  }

  StoreTargetValue(output, info->pointer_size, count);
  DefineBoundSymbol(linker, "__davecc_stacktrace_start",
                    info->pointer_size, table_address);
  DefineBoundSymbol(linker, "__davecc_stacktrace_end",
                    info->pointer_size,
                    table_address + info->contents->size);
}

void LinkerStacktraceDestruct(Linker* linker) {
  LinkerStacktraceInfo* info = linker->stacktrace_info;
  if (info == NULL) {
    return;
  }
  VectorDestruct(&info->symbols);
  ELFWriterSectionContentsDestruct(info->contents);
  free(info->contents);
  free(info);
  linker->stacktrace_info = NULL;
}
