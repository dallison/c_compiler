#include "linker_stacktrace.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "elf.h"
#include "linker.h"
#include "linker_file.h"
#include "linker_symbols.h"

typedef struct {
  String name;
  size_t output_offset;
} LinkerStacktraceSourceFile;

typedef struct {
  ELFReaderSection* text;
  uint64_t address;
  uint32_t line;
  LinkerStacktraceSourceFile* source;
} LinkerStacktraceLine;

typedef struct {
  Vector symbols;
  Vector source_files;
  Vector lines;
  ELFWriterSection* section;
  ELFWriterSectionContents* contents;
  size_t pointer_size;
  size_t line_records_offset;
  size_t names_offset;
} LinkerStacktraceInfo;

typedef struct {
  Vector* symbols;
  size_t names_size;
} CollectData;

typedef struct {
  const unsigned char* current;
  const unsigned char* end;
  bool failed;
} DwarfLineReader;

static uint8_t ReadU8(DwarfLineReader* reader) {
  if (reader->current == reader->end) {
    reader->failed = true;
    return 0;
  }
  return *reader->current++;
}

static uint16_t ReadU16(DwarfLineReader* reader) {
  uint16_t result = ReadU8(reader);
  result |= (uint16_t)ReadU8(reader) << 8;
  return result;
}

static uint32_t ReadU32(DwarfLineReader* reader) {
  uint32_t result = ReadU16(reader);
  result |= (uint32_t)ReadU16(reader) << 16;
  return result;
}

static uint64_t ReadU64(DwarfLineReader* reader) {
  uint64_t result = ReadU32(reader);
  result |= (uint64_t)ReadU32(reader) << 32;
  return result;
}

static uint64_t ReadULEB128(DwarfLineReader* reader) {
  uint64_t result = 0;
  unsigned shift = 0;
  while (!reader->failed) {
    uint8_t byte = ReadU8(reader);
    if (shift >= 64 && (byte & 0x7f) != 0) {
      reader->failed = true;
      return 0;
    }
    if (shift < 64) {
      result |= (uint64_t)(byte & 0x7f) << shift;
    }
    if ((byte & 0x80) == 0) {
      return result;
    }
    shift += 7;
  }
  return 0;
}

static int64_t ReadSLEB128(DwarfLineReader* reader) {
  uint64_t result = 0;
  unsigned shift = 0;
  uint8_t byte = 0;
  do {
    byte = ReadU8(reader);
    if (reader->failed || shift >= 64) {
      reader->failed = true;
      return 0;
    }
    result |= (uint64_t)(byte & 0x7f) << shift;
    shift += 7;
  } while ((byte & 0x80) != 0);
  if (shift < 64 && (byte & 0x40) != 0) {
    result |= UINT64_MAX << shift;
  }
  return (int64_t)result;
}

static String* ReadDwarfString(DwarfLineReader* reader) {
  const unsigned char* start = reader->current;
  while (reader->current != reader->end && *reader->current != 0) {
    reader->current++;
  }
  if (reader->current == reader->end) {
    reader->failed = true;
    return NULL;
  }
  String* result = malloc(sizeof(String));
  StringInitFromSegment(result, (const char*)start,
                        (size_t)(reader->current - start));
  reader->current++;
  return result;
}

static void DeleteStringElement(void* element) {
  StringDelete((String*)element);
}

static LinkerStacktraceSourceFile* InternSourceFile(
    LinkerStacktraceInfo* info, const String* name) {
  const char* value = name->value;
  static const char runfiles_marker[] = "/runfiles/_main/";
  static const char execroot_marker[] = "/execroot/_main/";
  const char* workspace_path = strstr(value, runfiles_marker);
  if (workspace_path != NULL) {
    value = workspace_path + sizeof(runfiles_marker) - 1;
  } else {
    workspace_path = strstr(value, execroot_marker);
    if (workspace_path != NULL) {
      value = workspace_path + sizeof(execroot_marker) - 1;
    }
  }

  for (size_t i = 0; i < info->source_files.length; i++) {
    LinkerStacktraceSourceFile* source = info->source_files.value.p[i];
    if (StringEqual(&source->name, value)) {
      return source;
    }
  }
  LinkerStacktraceSourceFile* source =
      calloc(1, sizeof(LinkerStacktraceSourceFile));
  StringInit(&source->name, value);
  VectorAppend(&info->source_files, source);
  return source;
}

static void DeleteSourceFile(void* element) {
  LinkerStacktraceSourceFile* source = element;
  StringDestruct(&source->name);
  free(source);
}

static void AppendSourceLine(LinkerStacktraceInfo* info,
                             ELFReaderSection* text, uint64_t address,
                             int64_t line, uint64_t file,
                             const Vector* files) {
  if (line <= 0 || line > UINT32_MAX || file == 0 ||
      file > files->length) {
    return;
  }
  LinkerStacktraceLine* entry = calloc(1, sizeof(LinkerStacktraceLine));
  entry->text = text;
  entry->address = address;
  entry->line = (uint32_t)line;
  entry->source = files->value.p[file - 1];
  VectorAppend(&info->lines, entry);
}

static size_t TargetPointerSize(const Linker* linker) {
  if (linker->elf_machine_type == ELF_MACHINE_TYPEW65C02) {
    return 2;
  }
  return linker->ops->is_64_bit ? 8 : 4;
}

static void ParseObjectDebugLines(LinkerStacktraceInfo* info,
                                  ObjectFile* file) {
  ELFReaderSection* debug_line =
      ELFReaderFileFindSection(file->elf_file, ".debug_line");
  ELFReaderSection* text =
      ELFReaderFileFindSection(file->elf_file, ".text");
  if (debug_line == NULL || text == NULL || debug_line->contents == NULL ||
      debug_line->header->size < 10) {
    return;
  }

  DwarfLineReader reader = {
      .current = debug_line->contents,
      .end = (const unsigned char*)debug_line->contents +
             debug_line->header->size,
  };
  uint32_t unit_length = ReadU32(&reader);
  if (reader.failed || unit_length > (uint64_t)(reader.end - reader.current)) {
    return;
  }
  reader.end = reader.current + unit_length;
  if (ReadU16(&reader) != 4) {
    return;
  }
  uint32_t header_length = ReadU32(&reader);
  if (reader.failed || header_length > (uint64_t)(reader.end - reader.current)) {
    return;
  }
  const unsigned char* header_end = reader.current + header_length;
  uint8_t minimum_instruction_length = ReadU8(&reader);
  (void)ReadU8(&reader);  // maximum_operations_per_instruction
  uint8_t default_is_stmt = ReadU8(&reader);
  int8_t line_base = (int8_t)ReadU8(&reader);
  uint8_t line_range = ReadU8(&reader);
  uint8_t opcode_base = ReadU8(&reader);
  if (reader.failed || minimum_instruction_length == 0 || line_range == 0 ||
      opcode_base == 0 || header_end < reader.current ||
      (size_t)(header_end - reader.current) < opcode_base - 1) {
    return;
  }

  uint8_t standard_lengths[256] = {0};
  for (unsigned opcode = 1; opcode < opcode_base; opcode++) {
    standard_lengths[opcode] = ReadU8(&reader);
  }

  Vector directories;
  Vector files;
  VectorInit(&directories);
  VectorInit(&files);
  while (!reader.failed && reader.current < header_end) {
    String* directory = ReadDwarfString(&reader);
    if (directory == NULL) {
      break;
    }
    if (directory->length == 0) {
      StringDelete(directory);
      break;
    }
    VectorAppend(&directories, directory);
  }
  while (!reader.failed && reader.current < header_end) {
    String* filename = ReadDwarfString(&reader);
    if (filename == NULL) {
      break;
    }
    if (filename->length == 0) {
      StringDelete(filename);
      break;
    }
    uint64_t directory_index = ReadULEB128(&reader);
    (void)ReadULEB128(&reader);
    (void)ReadULEB128(&reader);
    if (reader.failed || directory_index > directories.length) {
      StringDelete(filename);
      break;
    }

    String full_name;
    StringInit(&full_name, NULL);
    if (directory_index != 0 && filename->value[0] != '/') {
      String* directory = directories.value.p[directory_index - 1];
      StringAppendString(&full_name, directory);
      if (full_name.length != 0 &&
          full_name.value[full_name.length - 1] != '/') {
        StringAppendChar(&full_name, '/');
      }
    }
    StringAppendString(&full_name, filename);
    VectorAppend(&files, InternSourceFile(info, &full_name));
    StringDestruct(&full_name);
    StringDelete(filename);
  }
  if (reader.failed || reader.current > header_end) {
    goto cleanup;
  }
  reader.current = header_end;

  uint64_t address = 0;
  int64_t line = 1;
  uint64_t source_file = 1;
  bool is_stmt = default_is_stmt != 0;
  while (!reader.failed && reader.current < reader.end) {
    uint8_t opcode = ReadU8(&reader);
    if (opcode == 0) {
      uint64_t instruction_length = ReadULEB128(&reader);
      if (reader.failed || instruction_length == 0 ||
          instruction_length > (uint64_t)(reader.end - reader.current)) {
        reader.failed = true;
        break;
      }
      const unsigned char* instruction_end =
          reader.current + instruction_length;
      uint8_t extended_opcode = ReadU8(&reader);
      if (extended_opcode == 1) {  // DW_LNE_end_sequence
        address = 0;
        line = 1;
        source_file = 1;
        is_stmt = default_is_stmt != 0;
      } else if (extended_opcode == 2) {  // DW_LNE_set_address
        size_t address_size = (size_t)(instruction_end - reader.current);
        if (address_size == 8) {
          address = ReadU64(&reader);
        } else if (address_size == 4) {
          address = ReadU32(&reader);
        } else if (address_size == 2) {
          address = ReadU16(&reader);
        }
      }
      reader.current = instruction_end;
      continue;
    }

    if (opcode < opcode_base) {
      switch (opcode) {
        case 1:  // DW_LNS_copy
          AppendSourceLine(info, text, address, line, source_file, &files);
          break;
        case 2:  // DW_LNS_advance_pc
          address += ReadULEB128(&reader) * minimum_instruction_length;
          break;
        case 3:  // DW_LNS_advance_line
          line += ReadSLEB128(&reader);
          break;
        case 4:  // DW_LNS_set_file
          source_file = ReadULEB128(&reader);
          break;
        case 5:  // DW_LNS_set_column
          (void)ReadULEB128(&reader);
          break;
        case 6:  // DW_LNS_negate_stmt
          is_stmt = !is_stmt;
          break;
        case 8:  // DW_LNS_const_add_pc
          address +=
              ((255 - opcode_base) / line_range) *
              minimum_instruction_length;
          break;
        case 9:  // DW_LNS_fixed_advance_pc
          address += ReadU16(&reader);
          break;
        case 12:  // DW_LNS_set_isa
          (void)ReadULEB128(&reader);
          break;
        default:
          for (unsigned operand = 0; operand < standard_lengths[opcode];
               operand++) {
            (void)ReadULEB128(&reader);
          }
          break;
      }
      continue;
    }

    uint8_t adjusted_opcode = opcode - opcode_base;
    address += (adjusted_opcode / line_range) *
               minimum_instruction_length;
    line += line_base + adjusted_opcode % line_range;
    AppendSourceLine(info, text, address, line, source_file, &files);
  }
  (void)is_stmt;

cleanup:
  VectorDestructWithContents(&directories, DeleteStringElement,
                             /*free_element=*/false);
  VectorDestruct(&files);
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
  symbol->header->shndx = linker->building_dso ? 1 : SHN_ABS;
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
  VectorInit(&info->source_files);
  VectorInit(&info->lines);
  info->pointer_size = TargetPointerSize(linker);

  CollectData data = {
      .symbols = &info->symbols,
      .names_size = 0,
  };
  HashTableTraverse(&linker->global_symbol_table, CollectSymbolBucket, &data);
  for (size_t i = 0; i < linker->files.length; i++) {
    ObjectFile* file = linker->files.value.p[i];
    HashTableTraverse(&file->local_symbol_table, CollectSymbolBucket, &data);
    ParseObjectDebugLines(info, file);
  }
  for (size_t i = 0; i < info->source_files.length; i++) {
    LinkerStacktraceSourceFile* source = info->source_files.value.p[i];
    size_t name_size = source->name.length + 1;
    if (source->name.length == SIZE_MAX ||
        name_size > SIZE_MAX - data.names_size) {
      data.names_size = SIZE_MAX;
      break;
    }
    data.names_size += name_size;
  }

  size_t header_size = 4 * info->pointer_size;
  size_t record_size = 3 * info->pointer_size;
  if (data.names_size > SIZE_MAX - header_size ||
      info->symbols.length >
          (SIZE_MAX - header_size - data.names_size) / record_size) {
    VectorDestruct(&info->symbols);
    VectorDestructWithContents(&info->source_files, DeleteSourceFile,
                               /*free_element=*/false);
    VectorDestructWithContents(&info->lines, NULL, /*free_element=*/true);
    free(info);
    LinkerError(NULL, "Stacktrace symbol metadata is too large");
    return;
  }
  info->line_records_offset =
      header_size + info->symbols.length * record_size;
  if (info->lines.length >
      (SIZE_MAX - info->line_records_offset - data.names_size) /
          record_size) {
    VectorDestruct(&info->symbols);
    VectorDestructWithContents(&info->source_files, DeleteSourceFile,
                               /*free_element=*/false);
    VectorDestructWithContents(&info->lines, NULL, /*free_element=*/true);
    free(info);
    LinkerError(NULL, "Stacktrace source metadata is too large");
    return;
  }
  info->names_offset =
      info->line_records_offset + info->lines.length * record_size;
  size_t total_size = info->names_offset + data.names_size;
  if (info->pointer_size == 2 && total_size > UINT16_MAX) {
    VectorDestruct(&info->symbols);
    VectorDestructWithContents(&info->source_files, DeleteSourceFile,
                               /*free_element=*/false);
    VectorDestructWithContents(&info->lines, NULL, /*free_element=*/true);
    free(info);
    LinkerError(NULL, "Stacktrace metadata exceeds the 16-bit target limit");
    return;
  }

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
  DefineBoundSymbol(linker, "__davecc_stacktrace_start",
                    info->pointer_size, 0);
  DefineBoundSymbol(linker, "__davecc_stacktrace_end",
                    info->pointer_size, 0);
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

static uint64_t SourceLineAddress(const LinkerStacktraceLine* line) {
  if (line->address > UINT64_MAX - line->text->address) {
    return UINT64_MAX;
  }
  return line->text->address + line->address;
}

static int CompareLinesByAddress(const void* left, const void* right) {
  const LinkerStacktraceLine* a =
      *(const LinkerStacktraceLine* const*)left;
  const LinkerStacktraceLine* b =
      *(const LinkerStacktraceLine* const*)right;
  uint64_t a_address = SourceLineAddress(a);
  uint64_t b_address = SourceLineAddress(b);
  if (a_address < b_address) {
    return -1;
  }
  if (a_address > b_address) {
    return 1;
  }
  return 0;
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
  VectorSortPointers(&info->lines, CompareLinesByAddress);
  unsigned char* output =
      (unsigned char*)info->contents->data.buffered.value;
  const uint64_t table_address = info->section->address;
  const size_t record_size = 3 * info->pointer_size;
  const size_t header_size = 4 * info->pointer_size;
  size_t name_cursor = info->names_offset;
  size_t symbol_count = 0;
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
        output + header_size + symbol_count * record_size;
    StoreTargetValue(record, info->pointer_size,
                     symbol->address - table_address);
    StoreTargetValue(record + info->pointer_size, info->pointer_size,
                     symbol_end - table_address);
    StoreTargetValue(record + 2 * info->pointer_size, info->pointer_size,
                     name_cursor);

    memcpy(output + name_cursor, symbol->name.value,
           symbol->name.length + 1);
    name_cursor += symbol->name.length + 1;
    symbol_count++;
  }

  for (size_t i = 0; i < info->source_files.length; i++) {
    LinkerStacktraceSourceFile* source = info->source_files.value.p[i];
    source->output_offset = name_cursor;
    memcpy(output + name_cursor, source->name.value,
           source->name.length + 1);
    name_cursor += source->name.length + 1;
  }

  size_t line_count = 0;
  previous_address = UINT64_MAX;
  for (size_t i = 0; i < info->lines.length; i++) {
    LinkerStacktraceLine* line = info->lines.value.p[i];
    uint64_t address = SourceLineAddress(line);
    if (address == UINT64_MAX || address == previous_address) {
      continue;
    }
    previous_address = address;
    unsigned char* record =
        output + info->line_records_offset + line_count * record_size;
    StoreTargetValue(record, info->pointer_size, address - table_address);
    StoreTargetValue(record + info->pointer_size, info->pointer_size,
                     line->source->output_offset);
    StoreTargetValue(record + 2 * info->pointer_size, info->pointer_size,
                     line->line);
    line_count++;
  }

  StoreTargetValue(output, info->pointer_size, symbol_count);
  StoreTargetValue(output + info->pointer_size, info->pointer_size,
                   line_count);
  StoreTargetValue(output + 2 * info->pointer_size, info->pointer_size,
                   info->line_records_offset);
  StoreTargetValue(output + 3 * info->pointer_size, info->pointer_size,
                   info->names_offset);
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
  VectorDestructWithContents(&info->source_files, DeleteSourceFile,
                             /*free_element=*/false);
  VectorDestructWithContents(&info->lines, NULL, /*free_element=*/true);
  ELFWriterSectionContentsDestruct(info->contents);
  free(info->contents);
  free(info);
  linker->stacktrace_info = NULL;
}
