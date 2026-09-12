//
//  lto_archive.c
//  c_compiler
//
//  DCCLTO03 IR bitcode objects: AR container plus compile/link orchestration.
//

#include "lto_archive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ar.h"
#include "buffer.h"
#include "compiler.h"
#include "errors.h"
#include "ir.h"
#include "serialize_common.h"
#include "source.h"
#include "symbol.h"
#include "symbol_table.h"
#include "type.h"
#include "type_core.h"

enum {
  kHeader_triple = 1,
  kHeader_standard = 2,
  kHeader_opt_level = 3,
  kHeader_pointer_size = 4,
  kHeader_int_size = 5,
  kHeader_tu_id = 6,
  kHeader_next_literal_id = 7,
};

enum {
  kFiles_file = 1,
};

enum {
  kFile_name = 1,
  kFile_system = 2,
  kFile_line = 3,
};

enum {
  kFn_symbol = 1,
  kFn_type = 2,
  kFn_nodes = 3,
};

enum {
  kInit_type = 1,
  kInit_offset = 2,
  kInit_addend = 3,
  kInit_byte = 4,
  kInit_half = 5,
  kInit_word = 6,
  kInit_long = 7,
  kInit_symbol = 8,
  kInit_literal_id = 9,
  kInit_memory = 10,
};

enum {
  kGInit_symbol = 1,
  kGInit_is_global = 2,
  kGInit_is_weak = 3,
  kGInit_size = 4,
  kGInit_alignment = 5,
  kGInit_is_tls = 6,
  kGInit_is_local = 7,
  kGInit_initializer = 8,
};

enum {
  kGUninit_symbol = 1,
  kGUninit_is_global = 2,
  kGUninit_is_weak = 3,
  kGUninit_size = 4,
  kGUninit_alignment = 5,
  kGUninit_is_tls = 6,
  kGUninit_is_local = 7,
};

enum {
  kLit_kind = 1,
  kLit_id = 2,
  kLit_disabled = 3,
  kLit_element_size = 4,
  kLit_string = 5,
  kLit_buffer = 6,
};

enum {
  kArr_init = 1,
  kArr_fini = 2,
};

typedef struct {
  SerialKind kind;
  const char* member;
} PoolMember;

static const PoolMember kPoolMembers[] = {
    {kSerialKindType, "TYPES"},
    {kSerialKindSymbol, "SYMBOLS"},
    {kSerialKindStruct, "STRUCTS"},
    {kSerialKindEnum, "ENUMS"},
    {kSerialKindStructMember, "MEMBERS"},
    {kSerialKindIRNode, "IR"},
};
#define kNumPoolMembers (sizeof(kPoolMembers) / sizeof(kPoolMembers[0]))

void LoadedLTOModuleInit(LoadedLTOModule* loaded) {
  memset(loaded, 0, sizeof(*loaded));
  LTOModule* created = LTOModuleCreate();
  loaded->module = *created;
  free(created);
}

void LoadedLTOModuleDestruct(LoadedLTOModule* loaded) {
  if (loaded == NULL) {
    return;
  }
  LTOModuleDestruct(&loaded->module);
  if (loaded->ctx != NULL) {
    DeserializeContextDestruct(loaded->ctx);
    free(loaded->ctx);
    loaded->ctx = NULL;
  }
}

bool LTOArchiveIsLTOObject(const char* path) {
  FILE* fp = fopen(path, "rb");
  if (fp == NULL) {
    return false;
  }
  char magic[DCC_LTO_MAGIC_LEN];
  bool ok = fread(magic, 1, DCC_LTO_MAGIC_LEN, fp) == DCC_LTO_MAGIC_LEN &&
            memcmp(magic, DCC_LTO_MAGIC, DCC_LTO_MAGIC_LEN) == 0;
  fclose(fp);
  return ok;
}

bool LTOArchiveBufferIsLTO(const void* data, size_t len) {
  return data != NULL && len >= DCC_LTO_MAGIC_LEN &&
         memcmp(data, DCC_LTO_MAGIC, DCC_LTO_MAGIC_LEN) == 0;
}

static bool ARMemberNameIsSpecial(const char* name) {
  if (name == NULL || name[0] == '\0') {
    return true;
  }
  if (name[0] == '/') {
    return true;
  }
  return strcmp(name, "__.SYMDEF") == 0 ||
         strcmp(name, "__.SYMDEF SORTED") == 0;
}

int LTOArchiveExtractLTOMembers(const char* path, Vector* blobs,
                                Vector* blob_lens, bool* has_native) {
  if (has_native != NULL) {
    *has_native = false;
  }
  if (path == NULL || blobs == NULL || blob_lens == NULL) {
    return -1;
  }
  FILE* fp = fopen(path, "rb");
  if (fp == NULL) {
    return -1;
  }
  ARArchive archive;
  ARArchiveInit(&archive, path);
  if (!ARArchiveOpen(&archive, fp)) {
    ARArchiveDestruct(&archive);
    fclose(fp);
    return -1;
  }
  int extracted = 0;
  for (size_t i = 0; i < archive.files.length; i++) {
    ARFile* file = (ARFile*)VectorGet(&archive.files, i);
    if (file == NULL || ARMemberNameIsSpecial(file->filename.value) ||
        file->size <= 0) {
      continue;
    }
    void* data = malloc((size_t)file->size);
    if (data == NULL) {
      continue;
    }
    if (fseek(fp, file->file_offset, SEEK_SET) != 0 ||
        fread(data, 1, (size_t)file->size, fp) != (size_t)file->size) {
      free(data);
      continue;
    }
    if (LTOArchiveBufferIsLTO(data, (size_t)file->size)) {
      VectorAppend(blobs, data);
      VectorAppend(blob_lens, (void*)(intptr_t)file->size);
      extracted++;
    } else {
      if (has_native != NULL) {
        *has_native = true;
      }
      free(data);
    }
  }
  ARArchiveDestruct(&archive);
  fclose(fp);
  return extracted;
}

static void WriteStringField(WireBuffer* buf, int field, const char* s) {
  if (s != NULL && s[0] != '\0') {
    WireWriteString(buf, field, s, strlen(s));
  }
}

static void InternModuleGraph(SerializeContext* ctx, LTOModule* module) {
  if (module == NULL) {
    return;
  }
  for (size_t i = 0; i < module->functions.length; i++) {
    LTOFunction* fn = (LTOFunction*)module->functions.value.p[i];
    if (fn == NULL) {
      continue;
    }
    SerializeIntern(ctx, kSerialKindSymbol, fn->symbol);
    SerializeIntern(ctx, kSerialKindType, fn->type);
    for (ListElement* e = fn->code.first; e != NULL; e = e->next) {
      SerializeIntern(ctx, kSerialKindIRNode, e);
    }
  }
  for (size_t i = 0; i < module->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        module->initialized_static_variables.value.p[i];
    if (var == NULL) {
      continue;
    }
    SerializeIntern(ctx, kSerialKindSymbol, var->symbol);
    for (size_t j = 0; j < var->initializers.length; j++) {
      Initializer* init = var->initializers.value.p[j];
      if (init != NULL && init->type == kInitTypeSymbol) {
        SerializeIntern(ctx, kSerialKindSymbol, init->value.symbol);
      }
    }
  }
  for (size_t i = 0; i < module->uninitialized_static_variables.length; i++) {
    UninitializedStaticVariable* var =
        module->uninitialized_static_variables.value.p[i];
    if (var != NULL) {
      SerializeIntern(ctx, kSerialKindSymbol, var->symbol);
    }
  }
  for (size_t i = 0; i < module->init_array.length; i++) {
    SerializeIntern(ctx, kSerialKindSymbol, module->init_array.value.p[i]);
  }
  for (size_t i = 0; i < module->fini_array.length; i++) {
    SerializeIntern(ctx, kSerialKindSymbol, module->fini_array.value.p[i]);
  }
  for (size_t i = 0; i < module->literals.length; i++) {
    Literal* lit = module->literals.value.p[i];
    if (lit == NULL) {
      continue;
    }
    if (lit->type == kLiteralString || lit->type == kLiteralWideString) {
      StringLiteral* sl = (StringLiteral*)lit;
      SerializeInternStringN(ctx, sl->value.value, sl->value.length);
    }
  }
}

static void WriteInitializer(SerializeContext* ctx, WireBuffer* buf,
                             Initializer* init) {
  WireBuffer rec;
  WireBufferInitOwned(&rec, 16);
  WireWriteInt32(&rec, kInit_type, (int32_t)init->type);
  WireWriteInt32(&rec, kInit_offset, init->offset);
  WireWriteInt64(&rec, kInit_addend, init->symbol_addend);
  switch (init->type) {
    case kInitTypeByte:
      WireWriteVarint(&rec, kInit_byte, init->value.byte);
      break;
    case kInitTypeHalf:
      WireWriteVarint(&rec, kInit_half, init->value.half);
      break;
    case kInitTypeWord:
      WireWriteVarint(&rec, kInit_word, init->value.word);
      break;
    case kInitTypeLong:
      WireWriteUint64(&rec, kInit_long, init->value._long);
      break;
    case kInitTypeSymbol:
      SWriteRef(ctx, &rec, kInit_symbol, kSerialKindSymbol, init->value.symbol);
      break;
    case kInitTypeString:
      WireWriteInt32(&rec, kInit_literal_id, init->value.literal_id);
      break;
    case kInitTypeMemory:
      WireWriteBytes(&rec, kInit_memory, init->value.memory.value,
                     init->value.memory.length);
      break;
  }
  WireWriteBytes(buf, kGInit_initializer, WireBufferData(&rec),
                 WireBufferSize(&rec));
  WireBufferDestruct(&rec);
}

static void WriteFunctions(SerializeContext* ctx, WireBuffer* buf,
                           LTOModule* module) {
  WireWriteRawVarint(buf, module == NULL ? 0 : module->functions.length);
  if (module == NULL) {
    return;
  }
  for (size_t i = 0; i < module->functions.length; i++) {
    LTOFunction* fn = (LTOFunction*)module->functions.value.p[i];
    WireBuffer rec;
    WireBufferInitOwned(&rec, 32);
    if (fn != NULL) {
      SWriteRef(ctx, &rec, kFn_symbol, kSerialKindSymbol, fn->symbol);
      SWriteRef(ctx, &rec, kFn_type, kSerialKindType, fn->type);
      Vector nodes = {0};
      for (ListElement* e = fn->code.first; e != NULL; e = e->next) {
        VectorAppend(&nodes, e);
      }
      SWriteRefVector(ctx, &rec, kFn_nodes, kSerialKindIRNode, &nodes);
      VectorDestruct(&nodes);
    }
    WireWriteRawVarint(buf, WireBufferSize(&rec));
    WireWriteRaw(buf, WireBufferData(&rec), WireBufferSize(&rec));
    WireBufferDestruct(&rec);
  }
}

static void WriteGlobals(SerializeContext* ctx, WireBuffer* buf,
                         LTOModule* module) {
  size_t ninit = module == NULL ? 0 : module->initialized_static_variables.length;
  WireWriteRawVarint(buf, ninit);
  for (size_t i = 0; i < ninit; i++) {
    InitializedStaticVariable* var =
        module->initialized_static_variables.value.p[i];
    WireBuffer rec;
    WireBufferInitOwned(&rec, 32);
    if (var != NULL) {
      SWriteRef(ctx, &rec, kGInit_symbol, kSerialKindSymbol, var->symbol);
      WireWriteBool(&rec, kGInit_is_global, var->is_global);
      WireWriteBool(&rec, kGInit_is_weak, var->is_weak);
      WireWriteUint64(&rec, kGInit_size, var->size);
      WireWriteInt32(&rec, kGInit_alignment, var->alignment);
      WireWriteBool(&rec, kGInit_is_tls, var->is_tls);
      WireWriteBool(&rec, kGInit_is_local, var->is_local);
      for (size_t j = 0; j < var->initializers.length; j++) {
        Initializer* init = var->initializers.value.p[j];
        if (init != NULL) {
          WriteInitializer(ctx, &rec, init);
        }
      }
    }
    WireWriteRawVarint(buf, WireBufferSize(&rec));
    WireWriteRaw(buf, WireBufferData(&rec), WireBufferSize(&rec));
    WireBufferDestruct(&rec);
  }

  size_t nuninit =
      module == NULL ? 0 : module->uninitialized_static_variables.length;
  WireWriteRawVarint(buf, nuninit);
  for (size_t i = 0; i < nuninit; i++) {
    UninitializedStaticVariable* var =
        module->uninitialized_static_variables.value.p[i];
    WireBuffer rec;
    WireBufferInitOwned(&rec, 16);
    if (var != NULL) {
      SWriteRef(ctx, &rec, kGUninit_symbol, kSerialKindSymbol, var->symbol);
      WireWriteBool(&rec, kGUninit_is_global, var->is_global);
      WireWriteBool(&rec, kGUninit_is_weak, var->is_weak);
      WireWriteUint64(&rec, kGUninit_size, var->size);
      WireWriteUint64(&rec, kGUninit_alignment, var->alignment);
      WireWriteBool(&rec, kGUninit_is_tls, var->is_tls);
      WireWriteBool(&rec, kGUninit_is_local, var->is_local);
    }
    WireWriteRawVarint(buf, WireBufferSize(&rec));
    WireWriteRaw(buf, WireBufferData(&rec), WireBufferSize(&rec));
    WireBufferDestruct(&rec);
  }
}

static void WriteLiterals(SerializeContext* ctx, WireBuffer* buf,
                          LTOModule* module) {
  size_t n = module == NULL ? 0 : module->literals.length;
  WireWriteRawVarint(buf, n);
  for (size_t i = 0; i < n; i++) {
    Literal* lit = module->literals.value.p[i];
    WireBuffer rec;
    WireBufferInitOwned(&rec, 16);
    if (lit != NULL) {
      WireWriteInt32(&rec, kLit_kind, (int32_t)lit->type);
      WireWriteInt32(&rec, kLit_id, lit->id);
      WireWriteBool(&rec, kLit_disabled, lit->disabled);
      if (lit->type == kLiteralString || lit->type == kLiteralWideString) {
        StringLiteral* sl = (StringLiteral*)lit;
        WireWriteInt32(&rec, kLit_element_size, sl->element_size);
        SerialHandle h =
            SerializeInternStringN(ctx, sl->value.value, sl->value.length);
        WireWriteVarint(&rec, kLit_string, h);
      } else if (lit->type == kLiteralBuffer) {
        BufferLiteral* bl = (BufferLiteral*)lit;
        WireWriteBytes(&rec, kLit_buffer, bl->value.value, bl->value.length);
      }
    }
    WireWriteRawVarint(buf, WireBufferSize(&rec));
    WireWriteRaw(buf, WireBufferData(&rec), WireBufferSize(&rec));
    WireBufferDestruct(&rec);
  }
}

static void WriteInitArray(SerializeContext* ctx, WireBuffer* buf,
                           LTOModule* module) {
  SWriteRefVector(ctx, buf, kArr_init, kSerialKindSymbol,
                  module == NULL ? NULL : &module->init_array);
  SWriteRefVector(ctx, buf, kArr_fini, kSerialKindSymbol,
                  module == NULL ? NULL : &module->fini_array);
}

static void WriteSourceFiles(WireBuffer* buf) {
  size_t n = SourceFileCount();
  for (size_t i = 0; i < n; i++) {
    File* file = SourceFileAt(i);
    if (file == NULL) {
      continue;
    }
    WireBuffer rec;
    WireBufferInitOwned(&rec, 16);
    WriteStringField(&rec, kFile_name, file->name.value);
    WireWriteBool(&rec, kFile_system, file->is_system_header);
    for (size_t j = 0; j < file->lines.length; j++) {
      WireWriteInt64(&rec, kFile_line, file->lines.value.w[j]);
    }
    WireWriteBytes(buf, kFiles_file, WireBufferData(&rec),
                   WireBufferSize(&rec));
    WireBufferDestruct(&rec);
  }
}

static bool ReadSourceFiles(WireBuffer* buf, Vector* new_indices) {
  while (!WireBufferEof(buf) && !WireBufferHasError(buf)) {
    int field;
    WireType wt;
    if (!WireReadTag(buf, &field, &wt)) {
      break;
    }
    if (field != kFiles_file) {
      WireSkip(buf, wt);
      continue;
    }
    const void* data = NULL;
    size_t len = 0;
    if (!WireReadBytes(buf, &data, &len)) {
      return false;
    }
    WireBuffer rec;
    WireBufferInitReader(&rec, data, len);
    char namebuf[4096];
    namebuf[0] = '\0';
    const char* name = namebuf;
    bool is_system = false;
    Vector lines = {0};
    while (!WireBufferEof(&rec) && !WireBufferHasError(&rec)) {
      int f;
      WireType inner_wt;
      if (!WireReadTag(&rec, &f, &inner_wt)) {
        break;
      }
      if (f == kFile_name) {
        const void* s = NULL;
        size_t slen = 0;
        WireReadBytes(&rec, &s, &slen);
        size_t copy = slen < sizeof(namebuf) - 1 ? slen : sizeof(namebuf) - 1;
        memcpy(namebuf, s, copy);
        namebuf[copy] = '\0';
      } else if (f == kFile_system) {
        WireReadBool(&rec, &is_system);
      } else if (f == kFile_line) {
        int64_t line = 0;
        WireReadInt64(&rec, &line);
        VectorAppend(&lines, (void*)(intptr_t)line);
      } else {
        WireSkip(&rec, inner_wt);
      }
    }
    int64_t* line_vals = (int64_t*)lines.value.w;
    uint32_t idx =
        SourceImportFile(name, is_system, line_vals, lines.length);
    VectorAppend(new_indices, (void*)(intptr_t)idx);
    VectorDestruct(&lines);
  }
  return !WireBufferHasError(buf);
}

static SourceLocation RemapLocation(SourceLocation loc, Vector* new_indices) {
  if (loc == SOURCE_LOCATION_COMMAND_LINE || loc == SOURCE_LOCATION_MISSING ||
      new_indices == NULL || new_indices->length == 0) {
    return loc;
  }
  uint32_t old_file = (uint32_t)((loc >> LOC_FILE_SHIFT) & LOC_FILE_MASK);
  if (old_file >= new_indices->length) {
    return loc;
  }
  uint32_t new_file = (uint32_t)(uintptr_t)new_indices->value.p[old_file];
  return SourceRemapLocationFile(loc, new_file);
}

static void RemapLoadedLocations(LoadedLTOModule* loaded, Vector* new_indices) {
  if (loaded == NULL || loaded->ctx == NULL || new_indices == NULL ||
      new_indices->length == 0) {
    return;
  }
  Vector* ir_pool = &loaded->ctx->objects[kSerialKindIRNode];
  for (size_t i = 0; i < ir_pool->length; i++) {
    IRNode* node = (IRNode*)ir_pool->value.p[i];
    if (node == NULL) {
      continue;
    }
    node->location = RemapLocation(node->location, new_indices);
    if (node->opcode == IR_OP(loc)) {
      ((IRLocation*)node)->location = node->location;
    }
  }
  Vector* sym_pool = &loaded->ctx->objects[kSerialKindSymbol];
  for (size_t i = 0; i < sym_pool->length; i++) {
    Symbol* symbol = (Symbol*)sym_pool->value.p[i];
    if (symbol != NULL) {
      symbol->location = RemapLocation(symbol->location, new_indices);
    }
  }
}

static bool CopyFileWithMagic(const char* ar_path, const char* dest_path) {
  FILE* in = fopen(ar_path, "rb");
  FILE* out = fopen(dest_path, "wb");
  if (in == NULL || out == NULL) {
    if (in != NULL) {
      fclose(in);
    }
    if (out != NULL) {
      fclose(out);
    }
    return false;
  }
  bool ok = fwrite(DCC_LTO_MAGIC, 1, DCC_LTO_MAGIC_LEN, out) == DCC_LTO_MAGIC_LEN;
  char buf[4096];
  size_t n;
  while (ok && (n = fread(buf, 1, sizeof(buf), in)) > 0) {
    ok = fwrite(buf, 1, n, out) == n;
  }
  fclose(in);
  fclose(out);
  return ok;
}

bool LTOArchiveWrite(const char* path, Compiler* compiler) {
  if (path == NULL || compiler == NULL) {
    return false;
  }
  SerializeRegisterAllKinds();
  LTOModule* module = compiler->lto_module;
  if (module != NULL) {
    LTOModuleCaptureCompilerState(module, compiler);
  }

  SerializeContext ctx;
  SerializeContextInit(&ctx);
  ctx.writing_lto_ir = true;
  InternModuleGraph(&ctx, module);
  if (!SerializeContextDrain(&ctx)) {
    SerializeContextDestruct(&ctx);
    return false;
  }

  WireBuffer header;
  WireBufferInitOwned(&header, 32);
  const char* triple = compiler->target_triple.architecture.value;
  WriteStringField(&header, kHeader_triple, triple);
  WireWriteInt32(&header, kHeader_standard, (int32_t)compiler->language_standard);
  WireWriteInt32(&header, kHeader_opt_level, compiler->opt_level);
  WireWriteInt32(&header, kHeader_pointer_size, compiler->pointer_size);
  WireWriteInt32(&header, kHeader_int_size, compiler->int_size);
  if (module != NULL && module->tu_id != NULL) {
    WriteStringField(&header, kHeader_tu_id, module->tu_id);
  }
  WireWriteInt32(&header, kHeader_next_literal_id,
                 module != NULL ? module->next_literal_id
                                : compiler->next_literal_id);

  WireBuffer strings_buf;
  WireBufferInitOwned(&strings_buf, 64);
  SerializeWriteStringPool(&ctx, &strings_buf);

  WireBuffer pool_bufs[kNumPoolMembers];
  for (size_t i = 0; i < kNumPoolMembers; i++) {
    WireBufferInitOwned(&pool_bufs[i], 64);
    SerializeWritePool(&ctx, kPoolMembers[i].kind, &pool_bufs[i]);
  }

  WireBuffer functions_buf;
  WireBufferInitOwned(&functions_buf, 64);
  WriteFunctions(&ctx, &functions_buf, module);

  WireBuffer globals_buf;
  WireBufferInitOwned(&globals_buf, 64);
  WriteGlobals(&ctx, &globals_buf, module);

  WireBuffer literals_buf;
  WireBufferInitOwned(&literals_buf, 64);
  WriteLiterals(&ctx, &literals_buf, module);

  WireBuffer init_buf;
  WireBufferInitOwned(&init_buf, 16);
  WriteInitArray(&ctx, &init_buf, module);

  WireBuffer files_buf;
  WireBufferInitOwned(&files_buf, 32);
  WriteSourceFiles(&files_buf);

  char ar_path[4096];
  snprintf(ar_path, sizeof(ar_path), "%s.ltoar", path);
  bool ok = !ctx.error;
  if (ok) {
    ARArchiveBuilder builder;
    ARArchiveBuilderInit(&builder, ar_path);
    ARFile* header_file = ARArchiveBuilderAddFile(
        &builder, "HEADER", WireBufferSize(&header), 0, 0, 0644, 0,
        (void*)WireBufferData(&header));
    ARArchiveBuilderAddFile(&builder, "STRINGS", WireBufferSize(&strings_buf),
                            0, 0, 0644, 0, (void*)WireBufferData(&strings_buf));
    for (size_t i = 0; i < kNumPoolMembers; i++) {
      ARArchiveBuilderAddFile(&builder, kPoolMembers[i].member,
                              WireBufferSize(&pool_bufs[i]), 0, 0, 0644, 0,
                              (void*)WireBufferData(&pool_bufs[i]));
    }
    ARArchiveBuilderAddFile(&builder, "FUNCTIONS",
                            WireBufferSize(&functions_buf), 0, 0, 0644, 0,
                            (void*)WireBufferData(&functions_buf));
    ARArchiveBuilderAddFile(&builder, "GLOBALS", WireBufferSize(&globals_buf),
                            0, 0, 0644, 0, (void*)WireBufferData(&globals_buf));
    ARArchiveBuilderAddFile(&builder, "LITERALS", WireBufferSize(&literals_buf),
                            0, 0, 0644, 0, (void*)WireBufferData(&literals_buf));
    ARArchiveBuilderAddFile(&builder, "INITARRAY", WireBufferSize(&init_buf), 0,
                            0, 0644, 0, (void*)WireBufferData(&init_buf));
    ARArchiveBuilderAddFile(&builder, "FILES", WireBufferSize(&files_buf), 0, 0,
                            0644, 0, (void*)WireBufferData(&files_buf));
    ARArchiveBuilderAddSymbol(&builder, header_file, "__dcc_lto_module__");
    ok = ARArchiveBuilderWrite(&builder);
    ARArchiveBuilderDestruct(&builder);
  }
  if (ok) {
    ok = CopyFileWithMagic(ar_path, path);
  }
  unlink(ar_path);

  WireBufferDestruct(&header);
  WireBufferDestruct(&strings_buf);
  for (size_t i = 0; i < kNumPoolMembers; i++) {
    WireBufferDestruct(&pool_bufs[i]);
  }
  WireBufferDestruct(&functions_buf);
  WireBufferDestruct(&globals_buf);
  WireBufferDestruct(&literals_buf);
  WireBufferDestruct(&init_buf);
  WireBufferDestruct(&files_buf);
  SerializeContextDestruct(&ctx);
  return ok;
}

static void* ReadMemberBytes(ARArchive* archive, FILE* fp, const char* name,
                             size_t* out_len) {
  for (size_t i = 0; i < archive->files.length; i++) {
    ARFile* file = (ARFile*)VectorGet(&archive->files, i);
    if (StringEqual(&file->filename, name)) {
      void* data = malloc((size_t)file->size);
      fseek(fp, file->file_offset, SEEK_SET);
      if (fread(data, 1, (size_t)file->size, fp) != (size_t)file->size) {
        free(data);
        return NULL;
      }
      *out_len = (size_t)file->size;
      return data;
    }
  }
  *out_len = 0;
  return NULL;
}

static Initializer* ReadInitializer(DeserializeContext* ctx, const void* data,
                                    size_t len) {
  Initializer* init = calloc(1, sizeof(Initializer));
  WireBuffer in;
  WireBufferInitReader(&in, data, len);
  while (!WireBufferEof(&in) && !WireBufferHasError(&in)) {
    int field;
    WireType wt;
    if (!WireReadTag(&in, &field, &wt)) {
      break;
    }
    switch (field) {
      case kInit_type: {
        int32_t t = 0;
        WireReadInt32(&in, &t);
        init->type = (InitializerType)t;
        break;
      }
      case kInit_offset:
        WireReadInt32(&in, &init->offset);
        break;
      case kInit_addend:
        WireReadInt64(&in, &init->symbol_addend);
        break;
      case kInit_byte: {
        uint64_t v = 0;
        WireReadVarint(&in, &v);
        init->value.byte = (uint8_t)v;
        break;
      }
      case kInit_half: {
        uint64_t v = 0;
        WireReadVarint(&in, &v);
        init->value.half = (uint16_t)v;
        break;
      }
      case kInit_word: {
        uint64_t v = 0;
        WireReadVarint(&in, &v);
        init->value.word = (uint32_t)v;
        break;
      }
      case kInit_long:
        WireReadUint64(&in, &init->value._long);
        break;
      case kInit_symbol:
        init->value.symbol = (Symbol*)SReadRef(ctx, &in, kSerialKindSymbol);
        break;
      case kInit_literal_id:
        WireReadInt32(&in, &init->value.literal_id);
        break;
      case kInit_memory: {
        const void* mem = NULL;
        size_t mem_len = 0;
        WireReadBytes(&in, &mem, &mem_len);
        BufferInit(&init->value.memory);
        BufferAppend(&init->value.memory, (char*)mem, mem_len);
        break;
      }
      default:
        WireSkip(&in, wt);
        break;
    }
  }
  return init;
}

static bool ReadFunctions(DeserializeContext* ctx, WireBuffer* in,
                          LTOModule* module) {
  uint64_t count = 0;
  if (!WireReadRawVarint(in, &count)) {
    return false;
  }
  for (uint64_t i = 0; i < count; i++) {
    uint64_t rec_len = 0;
    if (!WireReadRawVarint(in, &rec_len) ||
        rec_len > (uint64_t)(in->end - in->addr)) {
      return false;
    }
    WireBuffer rec;
    WireBufferInitReader(&rec, in->addr, (size_t)rec_len);
    in->addr += rec_len;
    LTOFunction* fn = calloc(1, sizeof(LTOFunction));
    ListInit(&fn->code);
    while (!WireBufferEof(&rec) && !WireBufferHasError(&rec)) {
      int field;
      WireType wt;
      if (!WireReadTag(&rec, &field, &wt)) {
        break;
      }
      if (field == kFn_symbol) {
        fn->symbol = (Symbol*)SReadRef(ctx, &rec, kSerialKindSymbol);
      } else if (field == kFn_type) {
        fn->type = (TypeRecord*)SReadRef(ctx, &rec, kSerialKindType);
        if (fn->type != NULL) {
          TypeRecordIncRef(fn->type);
        }
      } else if (field == kFn_nodes) {
        Vector nodes = {0};
        SReadRefVector(ctx, &rec, kSerialKindIRNode, &nodes);
        for (size_t j = 0; j < nodes.length; j++) {
          IRNode* node = (IRNode*)nodes.value.p[j];
          if (node != NULL) {
            ListAppend(&fn->code, &node->header);
          }
        }
        VectorDestruct(&nodes);
      } else {
        WireSkip(&rec, wt);
      }
    }
    fn->ir_node_count = 0;
    for (ListElement* e = fn->code.first; e != NULL; e = e->next) {
      fn->ir_node_count++;
    }
    VectorAppend(&module->functions, fn);
  }
  return true;
}

static bool ReadGlobals(DeserializeContext* ctx, WireBuffer* in,
                        LTOModule* module) {
  uint64_t ninit = 0;
  if (!WireReadRawVarint(in, &ninit)) {
    return false;
  }
  for (uint64_t i = 0; i < ninit; i++) {
    uint64_t rec_len = 0;
    if (!WireReadRawVarint(in, &rec_len) ||
        rec_len > (uint64_t)(in->end - in->addr)) {
      return false;
    }
    WireBuffer rec;
    WireBufferInitReader(&rec, in->addr, (size_t)rec_len);
    in->addr += rec_len;
    InitializedStaticVariable* var = calloc(1, sizeof(InitializedStaticVariable));
    VectorInit(&var->initializers);
    while (!WireBufferEof(&rec) && !WireBufferHasError(&rec)) {
      int field;
      WireType wt;
      if (!WireReadTag(&rec, &field, &wt)) {
        break;
      }
      switch (field) {
        case kGInit_symbol:
          var->symbol = (Symbol*)SReadRef(ctx, &rec, kSerialKindSymbol);
          break;
        case kGInit_is_global:
          WireReadBool(&rec, &var->is_global);
          break;
        case kGInit_is_weak:
          WireReadBool(&rec, &var->is_weak);
          break;
        case kGInit_size: {
          uint64_t sz = 0;
          WireReadUint64(&rec, &sz);
          var->size = (size_t)sz;
          break;
        }
        case kGInit_alignment:
          WireReadInt32(&rec, &var->alignment);
          break;
        case kGInit_is_tls:
          WireReadBool(&rec, &var->is_tls);
          break;
        case kGInit_is_local:
          WireReadBool(&rec, &var->is_local);
          break;
        case kGInit_initializer: {
          const void* data = NULL;
          size_t len = 0;
          WireReadBytes(&rec, &data, &len);
          VectorAppend(&var->initializers, ReadInitializer(ctx, data, len));
          break;
        }
        default:
          WireSkip(&rec, wt);
          break;
      }
    }
    VectorAppend(&module->initialized_static_variables, var);
  }

  uint64_t nuninit = 0;
  if (!WireReadRawVarint(in, &nuninit)) {
    return false;
  }
  for (uint64_t i = 0; i < nuninit; i++) {
    uint64_t rec_len = 0;
    if (!WireReadRawVarint(in, &rec_len) ||
        rec_len > (uint64_t)(in->end - in->addr)) {
      return false;
    }
    WireBuffer rec;
    WireBufferInitReader(&rec, in->addr, (size_t)rec_len);
    in->addr += rec_len;
    UninitializedStaticVariable* var =
        calloc(1, sizeof(UninitializedStaticVariable));
    while (!WireBufferEof(&rec) && !WireBufferHasError(&rec)) {
      int field;
      WireType wt;
      if (!WireReadTag(&rec, &field, &wt)) {
        break;
      }
      switch (field) {
        case kGUninit_symbol:
          var->symbol = (Symbol*)SReadRef(ctx, &rec, kSerialKindSymbol);
          break;
        case kGUninit_is_global:
          WireReadBool(&rec, &var->is_global);
          break;
        case kGUninit_is_weak:
          WireReadBool(&rec, &var->is_weak);
          break;
        case kGUninit_size: {
          uint64_t sz = 0;
          WireReadUint64(&rec, &sz);
          var->size = (size_t)sz;
          break;
        }
        case kGUninit_alignment: {
          uint64_t al = 0;
          WireReadUint64(&rec, &al);
          var->alignment = (size_t)al;
          break;
        }
        case kGUninit_is_tls:
          WireReadBool(&rec, &var->is_tls);
          break;
        case kGUninit_is_local:
          WireReadBool(&rec, &var->is_local);
          break;
        default:
          WireSkip(&rec, wt);
          break;
      }
    }
    VectorAppend(&module->uninitialized_static_variables, var);
  }
  return true;
}

static bool ReadLiterals(DeserializeContext* ctx, WireBuffer* in,
                         LTOModule* module) {
  uint64_t n = 0;
  if (!WireReadRawVarint(in, &n)) {
    return false;
  }
  for (uint64_t i = 0; i < n; i++) {
    uint64_t rec_len = 0;
    if (!WireReadRawVarint(in, &rec_len) ||
        rec_len > (uint64_t)(in->end - in->addr)) {
      return false;
    }
    WireBuffer rec;
    WireBufferInitReader(&rec, in->addr, (size_t)rec_len);
    in->addr += rec_len;
    int32_t kind = 0;
    int32_t id = 0;
    bool disabled = false;
    int32_t element_size = 1;
    const char* str = NULL;
    size_t str_len = 0;
    const void* mem = NULL;
    size_t mem_len = 0;
    while (!WireBufferEof(&rec) && !WireBufferHasError(&rec)) {
      int field;
      WireType wt;
      if (!WireReadTag(&rec, &field, &wt)) {
        break;
      }
      switch (field) {
        case kLit_kind:
          WireReadInt32(&rec, &kind);
          break;
        case kLit_id:
          WireReadInt32(&rec, &id);
          break;
        case kLit_disabled:
          WireReadBool(&rec, &disabled);
          break;
        case kLit_element_size:
          WireReadInt32(&rec, &element_size);
          break;
        case kLit_string: {
          uint64_t handle = 0;
          WireReadVarint(&rec, &handle);
          str = DeserializeResolveString(ctx, (SerialHandle)handle, &str_len);
          break;
        }
        case kLit_buffer:
          WireReadBytes(&rec, &mem, &mem_len);
          break;
        default:
          WireSkip(&rec, wt);
          break;
      }
    }
    if (kind == kLiteralBuffer) {
      BufferLiteral* lit = calloc(1, sizeof(BufferLiteral));
      lit->base.type = kLiteralBuffer;
      lit->base.id = id;
      lit->base.disabled = disabled;
      BufferInit(&lit->value);
      BufferAppend(&lit->value, (char*)mem, mem_len);
      VectorAppend(&module->literals, lit);
    } else {
      StringLiteral* lit = calloc(1, sizeof(StringLiteral));
      lit->base.type = (LiteralType)kind;
      lit->base.id = id;
      lit->base.disabled = disabled;
      lit->element_size = element_size;
      if (str != NULL) {
        StringInitFromSegment(&lit->value, str, str_len);
      } else {
        StringInit(&lit->value, "");
      }
      VectorAppend(&module->literals, lit);
    }
  }
  return true;
}

static bool ReadArchiveFromFile(FILE* fp, LoadedLTOModule* loaded) {
  SerializeRegisterAllKinds();
  LoadedLTOModuleInit(loaded);
  loaded->module.owns_globals = true;

  char magic[DCC_LTO_MAGIC_LEN];
  if (fread(magic, 1, DCC_LTO_MAGIC_LEN, fp) != DCC_LTO_MAGIC_LEN ||
      memcmp(magic, DCC_LTO_MAGIC, DCC_LTO_MAGIC_LEN) != 0) {
    LoadedLTOModuleDestruct(loaded);
    return false;
  }

  // AR member offsets are relative to the start of the archive, not the
  // DCCLTO03 prefix.  Copy the AR bytes into a standalone stream.
  FILE* ar_fp = tmpfile();
  if (ar_fp == NULL) {
    LoadedLTOModuleDestruct(loaded);
    return false;
  }
  char copy_buf[4096];
  size_t nread;
  while ((nread = fread(copy_buf, 1, sizeof(copy_buf), fp)) > 0) {
    if (fwrite(copy_buf, 1, nread, ar_fp) != nread) {
      fclose(ar_fp);
      LoadedLTOModuleDestruct(loaded);
      return false;
    }
  }
  rewind(ar_fp);

  ARArchive archive;
  ARArchiveInit(&archive, "");
  if (!ARArchiveOpen(&archive, ar_fp)) {
    fprintf(stderr, "LTO: failed to open AR archive\n");
    ARArchiveDestruct(&archive);
    fclose(ar_fp);
    LoadedLTOModuleDestruct(loaded);
    return false;
  }

  Vector owned;
  VectorInit(&owned);
  bool ok = true;

  loaded->ctx = calloc(1, sizeof(DeserializeContext));
  DeserializeContextInit(loaded->ctx);

  size_t strings_len = 0;
  void* strings_bytes = ReadMemberBytes(&archive, ar_fp, "STRINGS", &strings_len);
  if (strings_bytes == NULL) {
    fprintf(stderr, "LTO: missing STRINGS member\n");
    ok = false;
  } else {
    VectorAppend(&owned, strings_bytes);
    WireBuffer sb;
    WireBufferInitReader(&sb, strings_bytes, strings_len);
    ok = SerializeReadStringPool(loaded->ctx, &sb);
  }

  for (size_t i = 0; ok && i < kNumPoolMembers; i++) {
    size_t len = 0;
    void* bytes = ReadMemberBytes(&archive, ar_fp, kPoolMembers[i].member, &len);
    if (bytes == NULL) {
      fprintf(stderr, "LTO: missing pool member %s\n", kPoolMembers[i].member);
      ok = false;
      break;
    }
    VectorAppend(&owned, bytes);
    WireBuffer pb;
    WireBufferInitReader(&pb, bytes, len);
    ok = SerializeReadPool(loaded->ctx, kPoolMembers[i].kind, &pb);
    if (!ok) {
      fprintf(stderr, "LTO: SerializeReadPool failed for %s\n",
              kPoolMembers[i].member);
    }
  }

  size_t header_len = 0;
  void* header_bytes = ReadMemberBytes(&archive, ar_fp, "HEADER", &header_len);
  if (header_bytes != NULL) {
    VectorAppend(&owned, header_bytes);
    WireBuffer hb;
    WireBufferInitReader(&hb, header_bytes, header_len);
    while (!WireBufferEof(&hb) && !WireBufferHasError(&hb)) {
      int field;
      WireType wt;
      if (!WireReadTag(&hb, &field, &wt)) {
        break;
      }
      if (field == kHeader_tu_id) {
        const void* data = NULL;
        size_t len = 0;
        WireReadBytes(&hb, &data, &len);
        loaded->module.tu_id = malloc(len + 1);
        memcpy(loaded->module.tu_id, data, len);
        loaded->module.tu_id[len] = '\0';
      } else if (field == kHeader_next_literal_id) {
        int32_t id = 1;
        WireReadInt32(&hb, &id);
        loaded->module.next_literal_id = id;
      } else {
        WireSkip(&hb, wt);
      }
    }
  }

  if (ok) {
    ok = DeserializeContextResolve(loaded->ctx);
  }

  size_t fn_len = 0;
  void* fn_bytes = ReadMemberBytes(&archive, ar_fp, "FUNCTIONS", &fn_len);
  if (ok && fn_bytes != NULL) {
    VectorAppend(&owned, fn_bytes);
    WireBuffer fb;
    WireBufferInitReader(&fb, fn_bytes, fn_len);
    ok = ReadFunctions(loaded->ctx, &fb, &loaded->module);
    if (!ok) {
      fprintf(stderr, "LTO: ReadFunctions failed\n");
    }
  } else {
    fprintf(stderr, "LTO: missing FUNCTIONS member\n");
    ok = false;
  }

  size_t g_len = 0;
  void* g_bytes = ReadMemberBytes(&archive, ar_fp, "GLOBALS", &g_len);
  if (ok && g_bytes != NULL) {
    VectorAppend(&owned, g_bytes);
    WireBuffer gb;
    WireBufferInitReader(&gb, g_bytes, g_len);
    ok = ReadGlobals(loaded->ctx, &gb, &loaded->module);
    if (!ok) {
      fprintf(stderr, "LTO: ReadGlobals failed\n");
    }
  } else if (ok) {
    fprintf(stderr, "LTO: missing GLOBALS member\n");
    ok = false;
  }

  size_t lit_len = 0;
  void* lit_bytes = ReadMemberBytes(&archive, ar_fp, "LITERALS", &lit_len);
  if (ok && lit_bytes != NULL) {
    VectorAppend(&owned, lit_bytes);
    WireBuffer lb;
    WireBufferInitReader(&lb, lit_bytes, lit_len);
    ok = ReadLiterals(loaded->ctx, &lb, &loaded->module);
    if (!ok) {
      fprintf(stderr, "LTO: ReadLiterals failed\n");
    }
  } else if (ok) {
    fprintf(stderr, "LTO: missing LITERALS member\n");
    ok = false;
  }

  size_t arr_len = 0;
  void* arr_bytes = ReadMemberBytes(&archive, ar_fp, "INITARRAY", &arr_len);
  if (ok && arr_bytes != NULL) {
    VectorAppend(&owned, arr_bytes);
    WireBuffer ab;
    WireBufferInitReader(&ab, arr_bytes, arr_len);
    while (!WireBufferEof(&ab) && !WireBufferHasError(&ab)) {
      int field;
      WireType wt;
      if (!WireReadTag(&ab, &field, &wt)) {
        break;
      }
      if (field == kArr_init) {
        SReadRefVector(loaded->ctx, &ab, kSerialKindSymbol,
                       &loaded->module.init_array);
      } else if (field == kArr_fini) {
        SReadRefVector(loaded->ctx, &ab, kSerialKindSymbol,
                       &loaded->module.fini_array);
      } else {
        WireSkip(&ab, wt);
      }
    }
  }

  Vector file_map = {0};
  size_t files_len = 0;
  void* files_bytes = ReadMemberBytes(&archive, ar_fp, "FILES", &files_len);
  if (ok && files_bytes != NULL) {
    VectorAppend(&owned, files_bytes);
    WireBuffer fb;
    WireBufferInitReader(&fb, files_bytes, files_len);
    if (!ReadSourceFiles(&fb, &file_map)) {
      fprintf(stderr, "LTO: ReadSourceFiles failed\n");
      ok = false;
    } else {
      RemapLoadedLocations(loaded, &file_map);
    }
  }
  VectorDestruct(&file_map);

  for (size_t i = 0; i < owned.length; i++) {
    free(owned.value.p[i]);
  }
  VectorDestruct(&owned);
  ARArchiveDestruct(&archive);
  fclose(ar_fp);

  if (!ok) {
    LoadedLTOModuleDestruct(loaded);
  }
  return ok;
}

bool LTOArchiveRead(const char* path, LoadedLTOModule* loaded) {
  FILE* fp = fopen(path, "rb");
  if (fp == NULL) {
    return false;
  }
  bool ok = ReadArchiveFromFile(fp, loaded);
  fclose(fp);
  return ok;
}

bool LTOArchiveReadFromMemory(const void* data, size_t len,
                              LoadedLTOModule* loaded) {
  FILE* fp = tmpfile();
  if (fp == NULL) {
    return false;
  }
  bool ok = fwrite(data, 1, len, fp) == len;
  if (ok) {
    rewind(fp);
    ok = ReadArchiveFromFile(fp, loaded);
  }
  fclose(fp);
  return ok;
}

static bool ReadEntireFile(const char* path, void** out, size_t* out_len) {
  FILE* fp = fopen(path, "rb");
  if (fp == NULL) {
    return false;
  }
  fseek(fp, 0, SEEK_END);
  long sz = ftell(fp);
  rewind(fp);
  if (sz < 0) {
    fclose(fp);
    return false;
  }
  void* buf = malloc((size_t)sz);
  if (fread(buf, 1, (size_t)sz, fp) != (size_t)sz) {
    free(buf);
    fclose(fp);
    return false;
  }
  fclose(fp);
  *out = buf;
  *out_len = (size_t)sz;
  return true;
}

String* CompileLTOIRObject(const char* filename, Vector* options,
                           Vector* target_opts, const char* output_path) {
  if (!CompileTranslationUnitToLTOIR(filename, options, target_opts)) {
    return NULL;
  }
  bool ok = LTOArchiveWrite(output_path, compiler);
  CompilerDelete(compiler);
  compiler = NULL;
  ClearAllFiles();
  return ok ? NewString(output_path) : NULL;
}

String* CompileLTOIRModules(Vector* inputs, Vector* options,
                            Vector* target_opts, bool whole_program,
                            Vector* preserve_asm_names) {
  if (inputs == NULL || inputs->length == 0) {
    return NULL;
  }

  Vector blobs = {0};
  Vector blob_lens = {0};
  for (size_t i = 0; i < inputs->length; i++) {
    const char* path = (const char*)inputs->value.p[i];
    void* data = NULL;
    size_t len = 0;
    if (LTOArchiveIsLTOObject(path)) {
      if (!ReadEntireFile(path, &data, &len)) {
        fprintf(stderr, "Cannot read LTO object %s\n", path);
        goto fail_blobs;
      }
      VectorAppend(&blobs, data);
      VectorAppend(&blob_lens, (void*)(intptr_t)len);
      continue;
    }
    size_t path_len = strlen(path);
    bool looks_like_archive =
        path_len >= 2 && strcmp(path + path_len - 2, ".a") == 0;
    if (looks_like_archive) {
      int nmem = LTOArchiveExtractLTOMembers(path, &blobs, &blob_lens, NULL);
      if (nmem < 0) {
        fprintf(stderr, "Cannot read archive %s\n", path);
        goto fail_blobs;
      }
      continue;
    }
    Compiler* saved = compiler;
    if (!CompileTranslationUnitToLTOIR(path, options, target_opts)) {
      compiler = saved;
      goto fail_blobs;
    }
    char tmpl[] = "/tmp/dcc-lto-XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) {
      CompilerDelete(compiler);
      compiler = saved;
      goto fail_blobs;
    }
    close(fd);
    bool ok = LTOArchiveWrite(tmpl, compiler);
    CompilerDelete(compiler);
    compiler = saved;
    if (!ok || !ReadEntireFile(tmpl, &data, &len)) {
      unlink(tmpl);
      goto fail_blobs;
    }
    unlink(tmpl);
    VectorAppend(&blobs, data);
    VectorAppend(&blob_lens, (void*)(intptr_t)len);
  }

  ClearAllFiles();
  compiler = malloc(sizeof(Compiler));
  const char* first = (const char*)inputs->value.p[0];
  if (!CompilerInitForLTOLink(compiler, first, options, target_opts)) {
    free(compiler);
    compiler = NULL;
    goto fail_blobs;
  }
  compiler->lto = true;
  CompilerPrepareForIRLoad();

  LTOModule* merged = LTOModuleCreate();
  Vector loaded_mods = {0};
  bool ok = true;
  for (size_t i = 0; ok && i < blobs.length; i++) {
    LoadedLTOModule* loaded = calloc(1, sizeof(LoadedLTOModule));
    size_t len = (size_t)(intptr_t)blob_lens.value.p[i];
    if (!LTOArchiveReadFromMemory(blobs.value.p[i], len, loaded)) {
      fprintf(stderr, "Failed to read LTO module\n");
      free(loaded);
      ok = false;
      break;
    }
    if (!LTOModuleMerge(merged, &loaded->module)) {
      LoadedLTOModuleDestruct(loaded);
      free(loaded);
      ok = false;
      break;
    }
    VectorAppend(&loaded_mods, loaded);
  }

  String* object_file = NULL;
  if (ok) {
    LTOInlineModule(merged);
    LTODevirtualizeModule(merged, whole_program);
    LTOInlineModule(merged);
    if (!LTOCodegenModule(compiler, merged, whole_program,
                          preserve_asm_names)) {
      ok = false;
    } else {
      object_file = CompilerEmitTranslationUnit(compiler, options);
    }
  }

  for (size_t i = 0; i < loaded_mods.length; i++) {
    LoadedLTOModule* loaded = loaded_mods.value.p[i];
    LoadedLTOModuleDestruct(loaded);
    free(loaded);
  }
  VectorDestruct(&loaded_mods);
  LTOModuleDelete(merged);
  CompilerDelete(compiler);
  compiler = NULL;
  ClearAllFiles();

  for (size_t i = 0; i < blobs.length; i++) {
    free(blobs.value.p[i]);
  }
  VectorDestruct(&blobs);
  VectorDestruct(&blob_lens);
  return object_file;

fail_blobs:
  for (size_t i = 0; i < blobs.length; i++) {
    free(blobs.value.p[i]);
  }
  VectorDestruct(&blobs);
  VectorDestruct(&blob_lens);
  return NULL;
}
