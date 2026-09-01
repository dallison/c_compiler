#include "aarch64_object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_emitter.h"
#include "compiler.h"
#include "options.h"
#include "aarch64_encode.h"

static void ReplaceSourceExtension(String* filename, const char* extension) {
  const char* suffixes[] = {".cpp", ".cxx", ".cc", ".c"};
  for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++) {
    const char* suffix = suffixes[i];
    size_t suffix_len = strlen(suffix);
    if (filename->length >= suffix_len &&
        strcmp(filename->value + filename->length - suffix_len, suffix) == 0) {
      filename->value[filename->length - suffix_len] = '\0';
      filename->length -= suffix_len;
      StringAppend(filename, extension);
      return;
    }
  }
  StringAppend(filename, extension);
}

static void RecordedInputDelete(void* value) {
  AssemblerRecordedInput* input = value;
  if (input->kind == kAssemblerRecordedText) {
    StringDelete(input->value.text);
  }
  free(input);
}

static bool AARCH64ObjectModuleAppendText(AARCH64ObjectModule* module,
                                         String* text) {
  AssemblerRecordedInput* input = calloc(1, sizeof(*input));
  if (input == NULL) {
    module->failed = true;
    StringDelete(text);
    return false;
  }
  input->kind = kAssemblerRecordedText;
  input->name = "<generated>";
  input->value.text = text;
  VectorAppend(&module->chunks, input);
  return true;
}

static void AARCH64ObjectModuleFlushPending(AARCH64ObjectModule* module) {
  if (module->pending_stream != NULL) {
    if (fclose(module->pending_stream) != 0) {
      module->failed = true;
    }
    module->pending_stream = NULL;
  }
  if (module->pending_size > 0) {
    String* chunk = NewStringWithLength(module->pending_ptr, module->pending_size);
    if (chunk == NULL) {
      module->failed = true;
    } else {
      AARCH64ObjectModuleAppendText(module, chunk);
    }
    free(module->pending_ptr);
    module->pending_ptr = NULL;
    module->pending_size = 0;
  }
}

static bool AARCH64ObjectModuleOpenPending(AARCH64ObjectModule* module) {
  if (module->pending_stream != NULL) {
    return true;
  }
  module->pending_stream =
      open_memstream(&module->pending_ptr, &module->pending_size);
  if (module->pending_stream == NULL) {
    module->failed = true;
  }
  return module->pending_stream != NULL;
}

static bool AARCH64ObjectModuleAppendChunk(AARCH64ObjectModule* module,
                                           const char* text, size_t length) {
  if (module == NULL || text == NULL || length == 0) {
    return true;
  }
  AARCH64ObjectModuleFlushPending(module);
  String* chunk = NewStringWithLength(text, length);
  if (chunk == NULL) {
    module->failed = true;
    return false;
  }
  return AARCH64ObjectModuleAppendText(module, chunk);
}

static void AARCH64ObjectModuleEmitFunction(Assembler* assembler,
                                            void* context) {
  AARCH64DirectEncodeFunction(context, assembler);
}

bool AARCH64ObjectModuleAppendFunction(AARCH64ObjectModule* module,
                                      AARCH64Generator* generator) {
  AARCH64ObjectModuleFlushPending(module);
  AssemblerRecordedInput* input = calloc(1, sizeof(*input));
  if (input == NULL) {
    module->failed = true;
    return false;
  }
  input->kind = kAssemblerRecordedEmitter;
  input->name = "<aarch64-function>";
  input->value.emitter.emit = AARCH64ObjectModuleEmitFunction;
  input->value.emitter.context = generator;
  VectorAppend(&module->chunks, input);
  return true;
}

void AARCH64ObjectModuleEndFunction(AARCH64ObjectModule* module) {
  AARCH64ObjectModuleFlushPending(module);
  if (!AARCH64ObjectModuleOpenPending(module)) {
    return;
  }
}

bool AARCH64ObjectModuleInit(AARCH64ObjectModule* module, String* src_file,
                             String* object_file, bool pic) {
  memset(module, 0, sizeof(*module));
  StringInit(&module->object_file, object_file->value);
  StringInit(&module->src_file, src_file->value);
  module->pic = pic;
  VectorInit(&module->chunks);

  String empty;
  StringInit(&empty, "");
  if (!AARCH64AssemblerInitFromGeneratedString(&module->assembler,
                                               src_file->value, &empty,
                                               object_file)) {
    StringDestruct(&empty);
    AARCH64ObjectModuleDestruct(module);
    return false;
  }
  module->owns_assembler = true;
  StringDestruct(&empty);
  module->assembler.base.object.pic = pic;

  char* preamble_ptr = NULL;
  size_t preamble_size = 0;
  FILE* preamble_stream = open_memstream(&preamble_ptr, &preamble_size);
  if (preamble_stream == NULL) {
    AARCH64ObjectModuleDestruct(module);
    return false;
  }
  EmitAssemblyPreamble(src_file, preamble_stream);
  fprintf(preamble_stream, ".PCbegin:\n");
  fclose(preamble_stream);
  if (!AARCH64ObjectModuleAppendChunk(module, preamble_ptr, preamble_size)) {
    free(preamble_ptr);
    AARCH64ObjectModuleDestruct(module);
    return false;
  }
  free(preamble_ptr);

  if (!AARCH64ObjectModuleOpenPending(module)) {
    AARCH64ObjectModuleDestruct(module);
    return false;
  }
  return true;
}

FILE* AARCH64ObjectModuleAssemblyStream(AARCH64ObjectModule* module) {
  if (module == NULL) {
    return NULL;
  }
  if (!AARCH64ObjectModuleOpenPending(module)) {
    return NULL;
  }
  return module->pending_stream;
}

static String* AARCH64ObjectModuleFormatFragment(const char* text, size_t length) {
  if (text == NULL || length == 0) {
    return NewString("");
  }
  if (text[0] == '\t' || text[0] == '.' || text[0] == '#') {
    String* chunk = NewStringWithLength(text, length);
    if (chunk == NULL) {
      return NULL;
    }
    if (text[length - 1] != '\n') {
      StringAppend(chunk, "\n");
    }
    return chunk;
  }
  String* chunk = NewString("\t");
  StringAppendSegment(chunk, text, length);
  StringAppend(chunk, "\n");
  return chunk;
}

bool AARCH64AssembleFragment(AARCH64ObjectModule* module, const char* name,
                             const char* text, size_t length) {
  (void)name;
  if (module == NULL || text == NULL || length == 0) {
    return true;
  }

  String* chunk = AARCH64ObjectModuleFormatFragment(text, length);
  if (chunk == NULL) {
    module->failed = true;
    return false;
  }
  if (module->pending_stream != NULL) {
    if (fwrite(chunk->value, 1, chunk->length, module->pending_stream) !=
        chunk->length) {
      module->failed = true;
    }
    StringDelete(chunk);
  } else {
    AARCH64ObjectModuleAppendText(module, chunk);
  }
  module->fragment_id++;
  return true;
}

bool AARCH64ObjectModuleFinalize(AARCH64ObjectModule* module) {
  AARCH64ObjectModuleFlushPending(module);
  if (module->failed || module->chunks.length == 0) {
    return false;
  }

  AssemblerRunRecordedOperations(&module->assembler.base, &module->chunks,
                                 AssembleAARCH64Instruction);
  return module->assembler.base.num_errors == 0;
}

void AARCH64ObjectModuleDestruct(AARCH64ObjectModule* module) {
  if (module->pending_stream != NULL) {
    fclose(module->pending_stream);
    module->pending_stream = NULL;
  }
  free(module->pending_ptr);
  module->pending_ptr = NULL;
  module->pending_size = 0;
  VectorDestructWithContents(&module->chunks, RecordedInputDelete, false);
  if (module->owns_assembler) {
    AARCH64AssemblerDestruct(&module->assembler);
    module->owns_assembler = false;
  }
  StringDestruct(&module->object_file);
  StringDestruct(&module->src_file);
}

String* AARCH64EmitObjectFile(Compiler* compiler, Vector* options) {
  String* output_filename = OptionStringValue(kOptionOutputFile, options);
  String* object_filename;
  if (output_filename != NULL) {
    object_filename = NewString(output_filename->value);
  } else {
    object_filename = NewString(compiler->infile.value);
    ReplaceSourceExtension(object_filename, ".o");
  }

  AARCH64ObjectModule module;
  if (!AARCH64ObjectModuleInit(&module, &compiler->infile, object_filename,
                               compiler->pic)) {
    StringDelete(object_filename);
    return NULL;
  }

  bool ok = true;
  for (size_t i = 0; i < compiler->functions.length && ok; i++) {
    AARCH64Generator* generator = compiler->functions.value.p[i];
    if (compiler->print_back_end) {
      AARCH64Emitter debug_emitter;
      AARCH64EmitterInit(&debug_emitter, generator);
      AARCH64EmitFunction(&debug_emitter, stdout, NULL);
      AARCH64EmitterDestruct(&debug_emitter);
    }
    FILE* function_stream = AARCH64ObjectModuleAssemblyStream(&module);
    if (function_stream == NULL) {
      ok = false;
      break;
    }
    AARCH64Emitter emitter;
    AARCH64EmitterInit(&emitter, generator);
    AARCH64EmitFunction(&emitter, function_stream, &module);
    AARCH64EmitterDestruct(&emitter);
  }
  FILE* asm_file = AARCH64ObjectModuleAssemblyStream(&module);
  ok = ok && asm_file != NULL &&
       EmitTranslationUnitRemainder(compiler, asm_file);
  if (!ok) {
    AARCH64ObjectModuleDestruct(&module);
    StringDelete(object_filename);
    return NULL;
  }

  if (!AARCH64ObjectModuleFinalize(&module)) {
    AARCH64ObjectModuleDestruct(&module);
    StringDelete(object_filename);
    return NULL;
  }
  AARCH64ObjectModuleDestruct(&module);
  return object_filename;
}
