//
//  wasm32_object.c
//  c_compiler
//

#include "wasm32_object.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Encoding primitives.

void WasmWriteULEB128(Buffer* buf, uint64_t value) {
  do {
    uint8_t byte = value & 0x7F;
    value >>= 7;
    if (value != 0) {
      byte |= 0x80;
    }
    BufferAppendByte(buf, (char)byte);
  } while (value != 0);
}

void WasmWriteSLEB128(Buffer* buf, int64_t value) {
  bool more = true;
  while (more) {
    uint8_t byte = value & 0x7F;
    value >>= 7;  // Arithmetic shift keeps the sign.
    bool sign_bit_set = (byte & 0x40) != 0;
    if ((value == 0 && !sign_bit_set) || (value == -1 && sign_bit_set)) {
      more = false;
    } else {
      byte |= 0x80;
    }
    BufferAppendByte(buf, (char)byte);
  }
}

// Five bytes always, so that patching cannot move anything.  Read as a signed
// LEB128 this is still the value it names, because the top byte only ever
// holds bits 28 to 30 and so never sets the sign bit.
void WasmWritePaddedU32(Buffer* buf, uint32_t value) {
  for (int i = 0; i < 5; i++) {
    uint8_t byte = value & 0x7F;
    value >>= 7;
    if (i < 4) {
      byte |= 0x80;
    }
    BufferAppendByte(buf, (char)byte);
  }
}

void WasmPatchPaddedU32(Buffer* buf, size_t offset, uint32_t value) {
  assert(offset + 5 <= buf->length);
  for (int i = 0; i < 5; i++) {
    uint8_t byte = value & 0x7F;
    value >>= 7;
    if (i < 4) {
      byte |= 0x80;
    }
    buf->value[offset + i] = (char)byte;
  }
}

void WasmPatchU32(Buffer* buf, size_t offset, uint32_t value) {
  assert(offset + 4 <= buf->length);
  for (int i = 0; i < 4; i++) {
    buf->value[offset + i] = (char)(value & 0xFF);
    value >>= 8;
  }
}

void WasmWriteName(Buffer* buf, const char* name) {
  size_t length = strlen(name);
  WasmWriteULEB128(buf, length);
  BufferAppend(buf, (char*)name, length);
}

void WasmWriteSection(Buffer* out, int id, Buffer* body) {
  if (body->length == 0) {
    return;
  }
  BufferAppendByte(out, (char)id);
  WasmWriteULEB128(out, body->length);
  BufferAppend(out, body->value, body->length);
}

void WasmWriteCustomSection(Buffer* out, const char* name, Buffer* body) {
  Buffer section;
  BufferInit(&section);
  WasmWriteName(&section, name);
  BufferAppend(&section, body->value, body->length);
  BufferAppendByte(out, (char)WASM_SECTION_CUSTOM);
  WasmWriteULEB128(out, section.length);
  BufferAppend(out, section.value, section.length);
  BufferDestruct(&section);
}

// Decoding primitives.

typedef struct {
  const uint8_t* bytes;
  size_t length;
  size_t pos;
  bool error;
  const char* filename;
} WasmCursor;

static void CursorFail(WasmCursor* cursor, const char* what) {
  if (!cursor->error) {
    fprintf(stderr, "wasm32: %s: malformed object, %s\n", cursor->filename,
            what);
    cursor->error = true;
  }
}

static uint8_t ReadByte(WasmCursor* cursor) {
  if (cursor->pos >= cursor->length) {
    CursorFail(cursor, "ran off the end");
    return 0;
  }
  return cursor->bytes[cursor->pos++];
}

static uint64_t ReadULEB128(WasmCursor* cursor) {
  uint64_t result = 0;
  int shift = 0;
  while (true) {
    uint8_t byte = ReadByte(cursor);
    if (cursor->error) {
      return 0;
    }
    if (shift < 64) {
      result |= (uint64_t)(byte & 0x7F) << shift;
    }
    shift += 7;
    if ((byte & 0x80) == 0) {
      break;
    }
    if (shift > 70) {
      CursorFail(cursor, "LEB128 too long");
      return 0;
    }
  }
  return result;
}

static int64_t ReadSLEB128(WasmCursor* cursor) {
  int64_t result = 0;
  int shift = 0;
  uint8_t byte;
  do {
    byte = ReadByte(cursor);
    if (cursor->error) {
      return 0;
    }
    if (shift < 64) {
      result |= (int64_t)(byte & 0x7F) << shift;
    }
    shift += 7;
    if (shift > 70) {
      CursorFail(cursor, "LEB128 too long");
      return 0;
    }
  } while ((byte & 0x80) != 0);
  if (shift < 64 && (byte & 0x40) != 0) {
    result |= -((int64_t)1 << shift);
  }
  return result;
}

// Names in a wasm module are not terminated, so hand back a copy that is.
static char* ReadName(WasmCursor* cursor) {
  uint64_t length = ReadULEB128(cursor);
  if (cursor->error || cursor->pos + length > cursor->length) {
    CursorFail(cursor, "name runs past the end");
    return NULL;
  }
  char* name = malloc(length + 1);
  memcpy(name, cursor->bytes + cursor->pos, length);
  name[length] = '\0';
  cursor->pos += length;
  return name;
}

static void Skip(WasmCursor* cursor, size_t count) {
  if (cursor->pos + count > cursor->length) {
    CursorFail(cursor, "ran off the end");
    cursor->pos = cursor->length;
    return;
  }
  cursor->pos += count;
}

// Object construction.

void Wasm32ObjectFileInit(Wasm32ObjectFile* object, const char* filename) {
  StringInit(&object->filename, filename);
  VectorInit(&object->types);
  VectorInit(&object->symbols);
  VectorInit(&object->segments);
  VectorInit(&object->functions);
  VectorInit(&object->imported_functions);
  object->is_live = true;
}

static void SegmentDelete(Wasm32Segment* segment) {
  free(segment->name);
  BufferDestruct(&segment->bytes);
  VectorDestructWithContents(&segment->relocs, NULL, /*free_element=*/true);
  free(segment);
}

static void FunctionDelete(Wasm32Function* function) {
  free(function->name);
  BufferDestruct(&function->body);
  VectorDestructWithContents(&function->relocs, NULL, /*free_element=*/true);
  free(function);
}

static void SymbolDelete(Wasm32Symbol* symbol) {
  free(symbol->name);
  free(symbol);
}

void Wasm32ObjectFileDestruct(Wasm32ObjectFile* object) {
  StringDestruct(&object->filename);
  for (size_t i = 0; i < object->types.length; i++) {
    BufferDelete(object->types.value.p[i]);
  }
  VectorDestruct(&object->types);
  for (size_t i = 0; i < object->symbols.length; i++) {
    SymbolDelete(object->symbols.value.p[i]);
  }
  VectorDestruct(&object->symbols);
  for (size_t i = 0; i < object->segments.length; i++) {
    SegmentDelete(object->segments.value.p[i]);
  }
  VectorDestruct(&object->segments);
  for (size_t i = 0; i < object->functions.length; i++) {
    FunctionDelete(object->functions.value.p[i]);
  }
  VectorDestruct(&object->functions);
  // The imported list only borrows symbols the table already owns.
  VectorDestruct(&object->imported_functions);
}

Wasm32Symbol* Wasm32ObjectFindSymbol(Wasm32ObjectFile* object, uint8_t kind,
                                     const char* name) {
  for (size_t i = 0; i < object->symbols.length; i++) {
    Wasm32Symbol* symbol = object->symbols.value.p[i];
    if (symbol->kind == kind && strcmp(symbol->name, name) == 0) {
      return symbol;
    }
  }
  return NULL;
}

Wasm32Symbol* Wasm32ObjectSymbol(Wasm32ObjectFile* object, uint8_t kind,
                                 const char* name) {
  Wasm32Symbol* symbol = Wasm32ObjectFindSymbol(object, kind, name);
  if (symbol != NULL) {
    return symbol;
  }
  symbol = malloc(sizeof(Wasm32Symbol));
  symbol->kind = kind;
  symbol->flags = WASM_SYM_UNDEFINED;
  symbol->name = strdup(name);
  symbol->index = 0;
  symbol->type_index = WASM32_NO_TYPE;
  symbol->segment = 0;
  symbol->offset = 0;
  symbol->size = 0;
  symbol->address = 0;
  symbol->final_index = -1;
  symbol->table_slot = -1;
  symbol->owner = object;
  symbol->definition = NULL;
  VectorAppend(&object->symbols, symbol);
  return symbol;
}

int Wasm32ObjectSymbolIndex(Wasm32ObjectFile* object, Wasm32Symbol* symbol) {
  for (size_t i = 0; i < object->symbols.length; i++) {
    if (object->symbols.value.p[i] == symbol) {
      return (int)i;
    }
  }
  return -1;
}

int Wasm32ObjectInternType(Wasm32ObjectFile* object, Buffer* encoding) {
  for (size_t i = 0; i < object->types.length; i++) {
    if (BufferCompare(object->types.value.p[i], encoding) == 0) {
      return (int)i;
    }
  }
  Buffer* copy = NewBuffer();
  BufferAppend(copy, encoding->value, encoding->length);
  VectorAppend(&object->types, copy);
  return (int)(object->types.length - 1);
}

Wasm32Reloc* Wasm32NewReloc(uint8_t type, uint32_t offset, uint32_t index,
                            int32_t addend) {
  Wasm32Reloc* reloc = malloc(sizeof(Wasm32Reloc));
  reloc->type = type;
  reloc->offset = offset;
  reloc->index = index;
  reloc->addend = addend;
  return reloc;
}

const char* Wasm32WasiFieldName(const char* name) {
  size_t prefix = sizeof(WASM32_WASI_PREFIX) - 1;
  if (name == NULL || strncmp(name, WASM32_WASI_PREFIX, prefix) != 0 ||
      name[prefix] == '\0') {
    return NULL;
  }
  return name + prefix;
}

static bool RelocHasAddend(uint8_t type) {
  return type == R_WASM_MEMORY_ADDR_LEB || type == R_WASM_MEMORY_ADDR_SLEB ||
         type == R_WASM_MEMORY_ADDR_I32;
}

// Writing.

static void WriteImports(Wasm32ObjectFile* object, Buffer* section) {
  WasmWriteULEB128(section, object->imported_functions.length + 3);

  // The three things every object expects the linker to supply.
  WasmWriteName(section, WASM32_IMPORT_MODULE);
  WasmWriteName(section, WASM32_MEMORY_IMPORT);
  BufferAppendByte(section, WASM_EXTERN_MEMORY);
  BufferAppendByte(section, 0x00);  // No maximum.
  WasmWriteULEB128(section, 0);

  WasmWriteName(section, WASM32_IMPORT_MODULE);
  WasmWriteName(section, WASM32_TABLE_IMPORT);
  BufferAppendByte(section, WASM_EXTERN_TABLE);
  BufferAppendByte(section, 0x70);  // funcref.
  BufferAppendByte(section, 0x00);
  WasmWriteULEB128(section, 0);

  WasmWriteName(section, WASM32_IMPORT_MODULE);
  WasmWriteName(section, WASM32_STACK_POINTER);
  BufferAppendByte(section, WASM_EXTERN_GLOBAL);
  BufferAppendByte(section, 0x7F);  // i32.
  BufferAppendByte(section, 0x01);  // Mutable.

  for (size_t i = 0; i < object->imported_functions.length; i++) {
    Wasm32Symbol* symbol = object->imported_functions.value.p[i];
    const char* field = Wasm32WasiFieldName(symbol->name);
    WasmWriteName(section,
                  field == NULL ? WASM32_IMPORT_MODULE : WASM32_WASI_MODULE);
    WasmWriteName(section, field == NULL ? symbol->name : field);
    BufferAppendByte(section, WASM_EXTERN_FUNC);
    WasmWriteULEB128(section, symbol->type_index);
  }
}

static void WriteSymbolTable(Wasm32ObjectFile* object, Buffer* out) {
  Buffer body;
  BufferInit(&body);
  WasmWriteULEB128(&body, object->symbols.length);
  for (size_t i = 0; i < object->symbols.length; i++) {
    Wasm32Symbol* symbol = object->symbols.value.p[i];
    BufferAppendByte(&body, (char)symbol->kind);
    WasmWriteULEB128(&body, symbol->flags);
    if (symbol->kind == WASM_SYMBOL_DATA) {
      WasmWriteName(&body, symbol->name);
      if ((symbol->flags & WASM_SYM_UNDEFINED) == 0) {
        WasmWriteULEB128(&body, symbol->segment);
        WasmWriteULEB128(&body, symbol->offset);
        WasmWriteULEB128(&body, symbol->size);
      }
    } else {
      WasmWriteULEB128(&body, symbol->index);
      // An undefined symbol takes its name from the import unless it says
      // otherwise, and ours always match, so the name is only written for
      // the defined ones.
      if ((symbol->flags & WASM_SYM_UNDEFINED) == 0 ||
          (symbol->flags & WASM_SYM_EXPLICIT_NAME) != 0) {
        WasmWriteName(&body, symbol->name);
      }
    }
  }
  BufferAppendByte(out, (char)WASM_SYMBOL_TABLE);
  WasmWriteULEB128(out, body.length);
  BufferAppend(out, body.value, body.length);
  BufferDestruct(&body);
}

static void WriteSegmentInfo(Wasm32ObjectFile* object, Buffer* out) {
  if (object->segments.length == 0) {
    return;
  }
  Buffer body;
  BufferInit(&body);
  WasmWriteULEB128(&body, object->segments.length);
  for (size_t i = 0; i < object->segments.length; i++) {
    Wasm32Segment* segment = object->segments.value.p[i];
    WasmWriteName(&body, segment->name);
    WasmWriteULEB128(&body, segment->alignment);
    WasmWriteULEB128(&body, segment->flags);
  }
  BufferAppendByte(out, (char)WASM_SEGMENT_INFO);
  WasmWriteULEB128(out, body.length);
  BufferAppend(out, body.value, body.length);
  BufferDestruct(&body);
}

// One reloc.* section.  'relocs' hold offsets already translated into the
// coordinates of the target section's payload.
static void WriteRelocSection(Buffer* module, const char* name,
                              uint32_t section_index, Vector* relocs) {
  if (relocs->length == 0) {
    return;
  }
  Buffer body;
  BufferInit(&body);
  WasmWriteULEB128(&body, section_index);
  WasmWriteULEB128(&body, relocs->length);
  for (size_t i = 0; i < relocs->length; i++) {
    Wasm32Reloc* reloc = relocs->value.p[i];
    BufferAppendByte(&body, (char)reloc->type);
    WasmWriteULEB128(&body, reloc->offset);
    WasmWriteULEB128(&body, reloc->index);
    if (RelocHasAddend(reloc->type)) {
      WasmWriteSLEB128(&body, reloc->addend);
    }
  }
  WasmWriteCustomSection(module, name, &body);
  BufferDestruct(&body);
}

bool Wasm32WriteObjectFile(Wasm32ObjectFile* object, String* filename) {
  Buffer module;
  BufferInit(&module);
  BufferAppendByte(&module, 0x00);
  BufferAppendByte(&module, 0x61);
  BufferAppendByte(&module, 0x73);
  BufferAppendByte(&module, 0x6D);
  BufferAppendWordLE(&module, 1);

  Buffer section;
  BufferInit(&section);

  // Sections are numbered by position for the reloc sections to point at, so
  // count every one actually emitted.
  uint32_t section_index = 0;

  WasmWriteULEB128(&section, object->types.length);
  for (size_t i = 0; i < object->types.length; i++) {
    Buffer* encoding = object->types.value.p[i];
    BufferAppend(&section, encoding->value, encoding->length);
  }
  WasmWriteSection(&module, WASM_SECTION_TYPE, &section);
  section_index++;

  BufferClear(&section);
  WriteImports(object, &section);
  WasmWriteSection(&module, WASM_SECTION_IMPORT, &section);
  section_index++;

  BufferClear(&section);
  WasmWriteULEB128(&section, object->functions.length);
  for (size_t i = 0; i < object->functions.length; i++) {
    Wasm32Function* function = object->functions.value.p[i];
    WasmWriteULEB128(&section, function->type_index);
  }
  WasmWriteSection(&module, WASM_SECTION_FUNCTION, &section);
  section_index++;

  // Code section, collecting each function's relocations into payload
  // coordinates as the bodies are laid down.
  Vector code_relocs;
  VectorInit(&code_relocs);
  uint32_t code_section_index = 0;
  if (object->functions.length > 0) {
    BufferClear(&section);
    WasmWriteULEB128(&section, object->functions.length);
    for (size_t i = 0; i < object->functions.length; i++) {
      Wasm32Function* function = object->functions.value.p[i];
      WasmWriteULEB128(&section, function->body.length);
      size_t body_start = section.length;
      BufferAppend(&section, function->body.value, function->body.length);
      for (size_t j = 0; j < function->relocs.length; j++) {
        Wasm32Reloc* reloc = function->relocs.value.p[j];
        VectorAppend(&code_relocs,
                     Wasm32NewReloc(reloc->type,
                                    (uint32_t)(body_start + reloc->offset),
                                    reloc->index, reloc->addend));
      }
    }
    code_section_index = section_index;
    WasmWriteSection(&module, WASM_SECTION_CODE, &section);
    section_index++;
  }

  // Data section.  A segment's offset in an object is a placeholder; where
  // it actually lands is the linker's decision, recorded in segment info.
  Vector data_relocs;
  VectorInit(&data_relocs);
  uint32_t data_section_index = 0;
  if (object->segments.length > 0) {
    BufferClear(&section);
    WasmWriteULEB128(&section, object->segments.length);
    for (size_t i = 0; i < object->segments.length; i++) {
      Wasm32Segment* segment = object->segments.value.p[i];
      WasmWriteULEB128(&section, 0);    // Active, memory 0.
      BufferAppendByte(&section, 0x41);  // i32.const
      BufferAppendByte(&section, 0x00);
      BufferAppendByte(&section, 0x0B);  // end
      WasmWriteULEB128(&section, segment->bytes.length);
      size_t bytes_start = section.length;
      BufferAppend(&section, segment->bytes.value, segment->bytes.length);
      for (size_t j = 0; j < segment->relocs.length; j++) {
        Wasm32Reloc* reloc = segment->relocs.value.p[j];
        VectorAppend(&data_relocs,
                     Wasm32NewReloc(reloc->type,
                                    (uint32_t)(bytes_start + reloc->offset),
                                    reloc->index, reloc->addend));
      }
    }
    data_section_index = section_index;
    WasmWriteSection(&module, WASM_SECTION_DATA, &section);
    section_index++;
  }

  BufferClear(&section);
  WasmWriteULEB128(&section, WASM_LINKING_VERSION);
  WriteSymbolTable(object, &section);
  WriteSegmentInfo(object, &section);
  WasmWriteCustomSection(&module, "linking", &section);

  WriteRelocSection(&module, "reloc.CODE", code_section_index, &code_relocs);
  WriteRelocSection(&module, "reloc.DATA", data_section_index, &data_relocs);

  VectorDestructWithContents(&code_relocs, NULL, /*free_element=*/true);
  VectorDestructWithContents(&data_relocs, NULL, /*free_element=*/true);
  BufferDestruct(&section);

  bool ok = false;
  FILE* fp = fopen(filename->value, "wb");
  if (fp == NULL) {
    fprintf(stderr, "wasm32: cannot open %s for writing\n", filename->value);
  } else {
    size_t written = fwrite(module.value, 1, module.length, fp);
    ok = fclose(fp) == 0 && written == module.length;
    if (!ok) {
      fprintf(stderr, "wasm32: cannot write %s\n", filename->value);
    }
  }
  BufferDestruct(&module);
  return ok;
}

// Reading.
//
// The parse runs in dependency order rather than file order: the symbol
// table names functions by index, and the relocation sections name symbols
// by index, so the index spaces have to exist before either is read.

typedef struct {
  uint8_t id;
  char* name;    // Custom sections only.
  size_t start;  // Payload start, after the id, size and custom name.
  size_t length;
} WasmSectionRange;

// A range within a section payload, used to hand a relocation to whichever
// function body or data segment contains it.
typedef struct {
  size_t start;
  size_t length;
  void* owner;
} WasmPayloadRange;

static void ReadLimits(WasmCursor* cursor) {
  uint8_t flags = ReadByte(cursor);
  ReadULEB128(cursor);
  if ((flags & 0x01) != 0) {
    ReadULEB128(cursor);
  }
}

// Skip a constant expression, which for our purposes is always short and
// always ends with 'end'.
static void SkipConstantExpression(WasmCursor* cursor) {
  while (!cursor->error) {
    uint8_t opcode = ReadByte(cursor);
    if (opcode == 0x0B) {
      return;
    }
    switch (opcode) {
      case 0x41:  // i32.const
      case 0x42:  // i64.const
        ReadSLEB128(cursor);
        break;
      case 0x23:  // global.get
        ReadULEB128(cursor);
        break;
      case 0x43:  // f32.const
        Skip(cursor, 4);
        break;
      case 0x44:  // f64.const
        Skip(cursor, 8);
        break;
      default:
        CursorFail(cursor, "unexpected opcode in a constant expression");
        return;
    }
  }
}

static bool CollectSections(WasmCursor* cursor, Vector* sections) {
  while (cursor->pos < cursor->length && !cursor->error) {
    uint8_t id = ReadByte(cursor);
    uint64_t size = ReadULEB128(cursor);
    if (cursor->error || cursor->pos + size > cursor->length) {
      CursorFail(cursor, "section runs past the end");
      return false;
    }
    size_t payload_end = cursor->pos + size;
    WasmSectionRange* range = malloc(sizeof(WasmSectionRange));
    range->id = id;
    range->name = NULL;
    if (id == WASM_SECTION_CUSTOM) {
      range->name = ReadName(cursor);
      if (cursor->error) {
        free(range->name);
        free(range);
        return false;
      }
    }
    range->start = cursor->pos;
    range->length = payload_end - cursor->pos;
    VectorAppend(sections, range);
    cursor->pos = payload_end;
  }
  return !cursor->error;
}

static WasmSectionRange* FindSection(Vector* sections, uint8_t id,
                                     const char* name) {
  for (size_t i = 0; i < sections->length; i++) {
    WasmSectionRange* range = sections->value.p[i];
    if (range->id != id) {
      continue;
    }
    if (name == NULL || (range->name != NULL && strcmp(range->name, name) == 0)) {
      return range;
    }
  }
  return NULL;
}

static WasmCursor SectionCursor(WasmCursor* file, WasmSectionRange* range) {
  WasmCursor cursor = *file;
  cursor.bytes = file->bytes + range->start;
  cursor.length = range->length;
  cursor.pos = 0;
  return cursor;
}

static void ReadTypeSection(WasmCursor* file, WasmSectionRange* range,
                            Wasm32ObjectFile* object) {
  WasmCursor cursor = SectionCursor(file, range);
  uint64_t count = ReadULEB128(&cursor);
  for (uint64_t i = 0; i < count && !cursor.error; i++) {
    size_t start = cursor.pos;
    uint8_t form = ReadByte(&cursor);
    if (form != WASM_FUNCTYPE) {
      CursorFail(&cursor, "a type that is not a function type");
      break;
    }
    uint64_t params = ReadULEB128(&cursor);
    Skip(&cursor, params);
    uint64_t results = ReadULEB128(&cursor);
    Skip(&cursor, results);
    if (cursor.error) {
      break;
    }
    Buffer* encoding = NewBuffer();
    BufferAppend(encoding, (char*)(cursor.bytes + start), cursor.pos - start);
    VectorAppend(&object->types, encoding);
  }
  file->error |= cursor.error;
}

// Names and declared types of the imports, in index order within each kind.
// An undefined symbol carries no name of its own, so the symbol table takes
// the name of the import it points at.
static void ReadImportSection(WasmCursor* file, WasmSectionRange* range,
                              Vector* function_names, Vector* function_types,
                              Vector* global_names) {
  WasmCursor cursor = SectionCursor(file, range);
  uint64_t count = ReadULEB128(&cursor);
  for (uint64_t i = 0; i < count && !cursor.error; i++) {
    char* module = ReadName(&cursor);
    char* field = ReadName(&cursor);
    // A host call was written out under the name the host knows, so the
    // symbol name it came from has to be put back together to match what
    // the rest of the link calls it.
    if (module != NULL && field != NULL &&
        strcmp(module, WASM32_WASI_MODULE) == 0) {
      char* name = malloc(sizeof(WASM32_WASI_PREFIX) + strlen(field));
      strcpy(name, WASM32_WASI_PREFIX);
      strcat(name, field);
      free(field);
      field = name;
    }
    free(module);
    uint8_t kind = ReadByte(&cursor);
    switch (kind) {
      case WASM_EXTERN_FUNC:
        VectorAppend(function_names, field);
        VectorAppend(function_types, (void*)(intptr_t)ReadULEB128(&cursor));
        field = NULL;
        break;
      case WASM_EXTERN_TABLE:
        ReadByte(&cursor);
        ReadLimits(&cursor);
        break;
      case WASM_EXTERN_MEMORY:
        ReadLimits(&cursor);
        break;
      case WASM_EXTERN_GLOBAL:
        ReadByte(&cursor);
        ReadByte(&cursor);
        VectorAppend(global_names, field);
        field = NULL;
        break;
      default:
        CursorFail(&cursor, "an import of an unknown kind");
        break;
    }
    free(field);
  }
  file->error |= cursor.error;
}

// Defined functions come from the function and code sections together: one
// gives the signature and the other the body.  Names arrive later, from the
// symbol table.
static void ReadFunctions(WasmCursor* file, Vector* sections,
                          Wasm32ObjectFile* object, Vector* body_ranges) {
  WasmSectionRange* function_range =
      FindSection(sections, WASM_SECTION_FUNCTION, NULL);
  WasmSectionRange* code_range = FindSection(sections, WASM_SECTION_CODE, NULL);
  if (function_range == NULL || code_range == NULL) {
    return;
  }

  Vector type_indices;
  VectorInit(&type_indices);
  WasmCursor cursor = SectionCursor(file, function_range);
  uint64_t count = ReadULEB128(&cursor);
  for (uint64_t i = 0; i < count && !cursor.error; i++) {
    VectorAppend(&type_indices, (void*)(intptr_t)ReadULEB128(&cursor));
  }
  file->error |= cursor.error;

  cursor = SectionCursor(file, code_range);
  uint64_t bodies = ReadULEB128(&cursor);
  if (bodies != type_indices.length) {
    CursorFail(&cursor, "the code and function sections disagree");
  }
  for (uint64_t i = 0; i < bodies && !cursor.error; i++) {
    uint64_t size = ReadULEB128(&cursor);
    if (cursor.pos + size > cursor.length) {
      CursorFail(&cursor, "a function body runs past the end");
      break;
    }
    Wasm32Function* function = malloc(sizeof(Wasm32Function));
    function->name = NULL;
    function->type_index = (uint32_t)(intptr_t)type_indices.value.p[i];
    function->symbol = NULL;
    BufferInit(&function->body);
    BufferAppend(&function->body, (char*)(cursor.bytes + cursor.pos), size);
    VectorInit(&function->relocs);
    VectorAppend(&object->functions, function);

    WasmPayloadRange* body = malloc(sizeof(WasmPayloadRange));
    body->start = cursor.pos;
    body->length = size;
    body->owner = function;
    VectorAppend(body_ranges, body);

    cursor.pos += size;
  }
  file->error |= cursor.error;
  VectorDestruct(&type_indices);
}

static void ReadDataSection(WasmCursor* file, WasmSectionRange* range,
                            Wasm32ObjectFile* object, Vector* byte_ranges) {
  WasmCursor cursor = SectionCursor(file, range);
  uint64_t count = ReadULEB128(&cursor);
  for (uint64_t i = 0; i < count && !cursor.error; i++) {
    uint64_t flags = ReadULEB128(&cursor);
    if (flags == 0 || flags == 2) {
      if (flags == 2) {
        ReadULEB128(&cursor);  // Memory index.
      }
      SkipConstantExpression(&cursor);
    }
    uint64_t size = ReadULEB128(&cursor);
    if (cursor.error || cursor.pos + size > cursor.length) {
      CursorFail(&cursor, "a data segment runs past the end");
      break;
    }
    Wasm32Segment* segment = malloc(sizeof(Wasm32Segment));
    segment->name = NULL;
    segment->alignment = 0;
    segment->flags = 0;
    segment->size = (uint32_t)size;
    segment->address = 0;
    BufferInit(&segment->bytes);
    BufferAppend(&segment->bytes, (char*)(cursor.bytes + cursor.pos), size);
    VectorInit(&segment->relocs);
    VectorAppend(&object->segments, segment);

    WasmPayloadRange* bytes = malloc(sizeof(WasmPayloadRange));
    bytes->start = cursor.pos;
    bytes->length = size;
    bytes->owner = segment;
    VectorAppend(byte_ranges, bytes);

    cursor.pos += size;
  }
  file->error |= cursor.error;
}

static void ReadSymbolTable(WasmCursor* cursor, Wasm32ObjectFile* object,
                            Vector* import_names, Vector* global_names) {
  uint64_t count = ReadULEB128(cursor);
  for (uint64_t i = 0; i < count && !cursor->error; i++) {
    Wasm32Symbol* symbol = malloc(sizeof(Wasm32Symbol));
    symbol->kind = ReadByte(cursor);
    symbol->flags = (uint32_t)ReadULEB128(cursor);
    symbol->name = NULL;
    symbol->index = 0;
    symbol->type_index = 0;
    symbol->segment = 0;
    symbol->offset = 0;
    symbol->size = 0;
    symbol->address = 0;
    symbol->final_index = -1;
    symbol->table_slot = -1;
    symbol->owner = object;
    symbol->definition = NULL;

    bool undefined = (symbol->flags & WASM_SYM_UNDEFINED) != 0;
    if (symbol->kind == WASM_SYMBOL_DATA) {
      symbol->name = ReadName(cursor);
      if (!undefined) {
        symbol->segment = (uint32_t)ReadULEB128(cursor);
        symbol->offset = (uint32_t)ReadULEB128(cursor);
        symbol->size = (uint32_t)ReadULEB128(cursor);
      }
    } else {
      symbol->index = (uint32_t)ReadULEB128(cursor);
      if (!undefined || (symbol->flags & WASM_SYM_EXPLICIT_NAME) != 0) {
        symbol->name = ReadName(cursor);
      } else {
        Vector* names =
            symbol->kind == WASM_SYMBOL_GLOBAL ? global_names : import_names;
        if (symbol->index < names->length) {
          symbol->name = strdup(names->value.p[symbol->index]);
        }
      }
    }
    if (symbol->name == NULL) {
      symbol->name = strdup("");
    }
    VectorAppend(&object->symbols, symbol);

    // Tie a defined function symbol to the body it names.
    if (symbol->kind == WASM_SYMBOL_FUNCTION && !undefined) {
      size_t defined = symbol->index - import_names->length;
      if (symbol->index >= import_names->length &&
          defined < object->functions.length) {
        Wasm32Function* function = object->functions.value.p[defined];
        function->symbol = symbol;
        free(function->name);
        function->name = strdup(symbol->name);
      }
    }
  }
}

static void ReadSegmentInfo(WasmCursor* cursor, Wasm32ObjectFile* object) {
  uint64_t count = ReadULEB128(cursor);
  for (uint64_t i = 0; i < count && !cursor->error; i++) {
    char* name = ReadName(cursor);
    uint32_t alignment = (uint32_t)ReadULEB128(cursor);
    uint32_t flags = (uint32_t)ReadULEB128(cursor);
    if (i < object->segments.length) {
      Wasm32Segment* segment = object->segments.value.p[i];
      free(segment->name);
      segment->name = name;
      segment->alignment = alignment;
      segment->flags = flags;
    } else {
      free(name);
    }
  }
}

static void ReadLinkingSection(WasmCursor* file, WasmSectionRange* range,
                               Wasm32ObjectFile* object, Vector* import_names,
                               Vector* global_names) {
  WasmCursor cursor = SectionCursor(file, range);
  uint64_t version = ReadULEB128(&cursor);
  if (version != WASM_LINKING_VERSION) {
    CursorFail(&cursor, "an unsupported linking section version");
  }
  while (cursor.pos < cursor.length && !cursor.error) {
    uint8_t id = ReadByte(&cursor);
    uint64_t size = ReadULEB128(&cursor);
    if (cursor.pos + size > cursor.length) {
      CursorFail(&cursor, "a linking subsection runs past the end");
      break;
    }
    size_t end = cursor.pos + size;
    switch (id) {
      case WASM_SYMBOL_TABLE:
        ReadSymbolTable(&cursor, object, import_names, global_names);
        break;
      case WASM_SEGMENT_INFO:
        ReadSegmentInfo(&cursor, object);
        break;
      default:
        break;  // Init functions and comdats are not something we emit.
    }
    cursor.pos = end;
  }
  file->error |= cursor.error;
}

static void* OwnerOfOffset(Vector* ranges, size_t offset, size_t* relative) {
  for (size_t i = 0; i < ranges->length; i++) {
    WasmPayloadRange* range = ranges->value.p[i];
    if (offset >= range->start && offset < range->start + range->length) {
      *relative = offset - range->start;
      return range->owner;
    }
  }
  return NULL;
}

static void ReadRelocSection(WasmCursor* file, WasmSectionRange* range,
                             Vector* ranges, bool is_code) {
  WasmCursor cursor = SectionCursor(file, range);
  ReadULEB128(&cursor);  // Which section; there is only one of each for us.
  uint64_t count = ReadULEB128(&cursor);
  for (uint64_t i = 0; i < count && !cursor.error; i++) {
    uint8_t type = ReadByte(&cursor);
    uint32_t offset = (uint32_t)ReadULEB128(&cursor);
    uint32_t index = (uint32_t)ReadULEB128(&cursor);
    int32_t addend = RelocHasAddend(type) ? (int32_t)ReadSLEB128(&cursor) : 0;
    if (cursor.error) {
      break;
    }
    size_t relative = 0;
    void* owner = OwnerOfOffset(ranges, offset, &relative);
    if (owner == NULL) {
      CursorFail(&cursor, "a relocation that points outside every body");
      break;
    }
    Wasm32Reloc* reloc =
        Wasm32NewReloc(type, (uint32_t)relative, index, addend);
    if (is_code) {
      VectorAppend(&((Wasm32Function*)owner)->relocs, reloc);
    } else {
      VectorAppend(&((Wasm32Segment*)owner)->relocs, reloc);
    }
  }
  file->error |= cursor.error;
}

bool Wasm32ReadObjectFile(Wasm32ObjectFile* object, const char* filename,
                          const uint8_t* bytes, size_t length) {
  Wasm32ObjectFileInit(object, filename);

  WasmCursor file = {bytes, length, 0, false, filename};
  if (length < 8 || memcmp(bytes, "\0asm", 4) != 0) {
    fprintf(stderr, "wasm32: %s is not a wasm object\n", filename);
    return false;
  }
  file.pos = 8;

  Vector sections;
  VectorInit(&sections);
  Vector import_names;
  VectorInit(&import_names);
  Vector import_types;
  VectorInit(&import_types);
  Vector global_names;
  VectorInit(&global_names);
  Vector body_ranges;
  VectorInit(&body_ranges);
  Vector byte_ranges;
  VectorInit(&byte_ranges);

  bool ok = CollectSections(&file, &sections);

  WasmSectionRange* range;
  if (ok && (range = FindSection(&sections, WASM_SECTION_TYPE, NULL)) != NULL) {
    ReadTypeSection(&file, range, object);
  }
  if (ok &&
      (range = FindSection(&sections, WASM_SECTION_IMPORT, NULL)) != NULL) {
    ReadImportSection(&file, range, &import_names, &import_types,
                      &global_names);
  }
  if (ok) {
    ReadFunctions(&file, &sections, object, &body_ranges);
  }
  if (ok && (range = FindSection(&sections, WASM_SECTION_DATA, NULL)) != NULL) {
    ReadDataSection(&file, range, object, &byte_ranges);
  }
  if (ok &&
      (range = FindSection(&sections, WASM_SECTION_CUSTOM, "linking")) != NULL) {
    ReadLinkingSection(&file, range, object, &import_names, &global_names);
  } else if (ok) {
    fprintf(stderr,
            "wasm32: %s has no linking section, so it is a finished module "
            "rather than an object the linker can use\n",
            filename);
    file.error = true;
  }
  if (ok && (range = FindSection(&sections, WASM_SECTION_CUSTOM,
                                 "reloc.CODE")) != NULL) {
    ReadRelocSection(&file, range, &body_ranges, /*is_code=*/true);
  }
  if (ok && (range = FindSection(&sections, WASM_SECTION_CUSTOM,
                                 "reloc.DATA")) != NULL) {
    ReadRelocSection(&file, range, &byte_ranges, /*is_code=*/false);
  }

  // Rebuild the imported function list from the symbols that name them, so
  // that the rest of the linker can work in symbols alone.
  for (size_t i = 0; ok && i < import_names.length; i++) {
    for (size_t j = 0; j < object->symbols.length; j++) {
      Wasm32Symbol* symbol = object->symbols.value.p[j];
      if (symbol->kind == WASM_SYMBOL_FUNCTION &&
          (symbol->flags & WASM_SYM_UNDEFINED) != 0 && symbol->index == i) {
        symbol->type_index = (uint32_t)(intptr_t)import_types.value.p[i];
        VectorAppend(&object->imported_functions, symbol);
        break;
      }
    }
  }

  ok = ok && !file.error;

  for (size_t i = 0; i < sections.length; i++) {
    WasmSectionRange* section = sections.value.p[i];
    free(section->name);
    free(section);
  }
  VectorDestruct(&sections);
  for (size_t i = 0; i < import_names.length; i++) {
    free(import_names.value.p[i]);
  }
  VectorDestruct(&import_names);
  VectorDestruct(&import_types);
  for (size_t i = 0; i < global_names.length; i++) {
    free(global_names.value.p[i]);
  }
  VectorDestruct(&global_names);
  VectorDestructWithContents(&body_ranges, NULL, /*free_element=*/true);
  VectorDestructWithContents(&byte_ranges, NULL, /*free_element=*/true);
  return ok;
}
