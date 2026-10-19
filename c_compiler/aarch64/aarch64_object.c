#include "aarch64_object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_emitter.h"
#include "compiler.h"
#include "options.h"
#include "aarch64_program.h"

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

void AARCH64ObjectModuleStart(AARCH64ObjectModule* module, bool keep_text) {
  memset(module, 0, sizeof(*module));
  AsmModuleInit(&module->program, AARCH64ProgramTargetOps());
  module->program.keep_text = keep_text;
}

bool AARCH64ObjectModuleOpen(AARCH64ObjectModule* module, String* src_file,
                             String* object_file, bool pic) {
  StringInit(&module->object_file, object_file->value);
  StringInit(&module->src_file, src_file->value);
  module->pic = pic;

  String empty;
  StringInit(&empty, "");
  if (!AARCH64AssemblerInitFromGeneratedString(&module->assembler,
                                               src_file->value, &empty,
                                               object_file)) {
    StringDestruct(&empty);
    return false;
  }
  module->owns_assembler = true;
  StringDestruct(&empty);
  module->assembler.base.object.pic = pic;

  // The preamble lists every source file, so it can only be written once the
  // translation unit is complete, but it has to come first.
  size_t emitted = module->program.operations.length;
  EmitAssemblyPreambleToModule(compiler, &module->program);
  AsmModuleLabel(&module->program, ".PCbegin");
  AsmModuleMoveOperationsToFront(&module->program, emitted);
  return true;
}

bool AARCH64ObjectModuleFinalize(AARCH64ObjectModule* module) {
  if (module->failed || module->program.failed ||
      module->program.operations.length == 0) {
    return false;
  }

  AssemblerRunModule(&module->assembler.base, &module->program);
  return module->assembler.base.num_errors == 0;
}

bool AARCH64ObjectModuleWriteAssembly(AARCH64ObjectModule* module, FILE* out) {
  if (module->failed || module->program.failed) {
    return false;
  }
  return AsmModuleWriteText(&module->program, out);
}

void AARCH64ObjectModuleDestruct(AARCH64ObjectModule* module) {
  // The assembler's symbols borrow the program's names.
  if (module->owns_assembler) {
    AARCH64AssemblerDestruct(&module->assembler);
    module->owns_assembler = false;
  }
  AsmModuleDestruct(&module->program);
  StringDestruct(&module->object_file);
  StringDestruct(&module->src_file);
}

static String* AARCH64ProgramOutputFilename(Compiler* compiler,
                                            Vector* options,
                                            const char* extension) {
  String* output_filename = OptionStringValue(kOptionOutputFile, options);
  if (output_filename != NULL) {
    return NewString(output_filename->value);
  }
  String* filename = NewString(compiler->infile.value);
  ReplaceSourceExtension(filename, extension);
  return filename;
}

// Emits the function and deletes its generator: the program owns copies of
// everything it needs.
static void AARCH64EmitGeneratedFunction(Compiler* compiler,
                                         AARCH64Generator* generator,
                                         AsmModule* program) {
  if (compiler->print_back_end) {
    AARCH64Emitter debug_emitter;
    AARCH64EmitterInit(&debug_emitter, generator);
    AARCH64EmitFunction(&debug_emitter, stdout);
    AARCH64EmitterDestruct(&debug_emitter);
  }
  AARCH64Emitter emitter;
  AARCH64EmitterInit(&emitter, generator);
  AARCH64EmitFunctionToModule(&emitter, program);
  AARCH64EmitterDestruct(&emitter);
  AARCH64GeneratorDelete(generator);
}

static void DeleteFunctionStream(void* stream) {
  AARCH64ObjectModuleDestruct(stream);
  free(stream);
}

bool AARCH64StreamFunction(Compiler* compiler, void* code) {
  AARCH64ObjectModule* module = compiler->function_stream;
  if (module == NULL) {
    module = malloc(sizeof(*module));
    if (module == NULL) {
      return false;
    }
    AARCH64ObjectModuleStart(
        module, compiler->assembly_output || compiler->keep_asm_file);
    compiler->function_stream = module;
    compiler->function_stream_delete = DeleteFunctionStream;
  }
  if (module->program.failed) {
    AARCH64GeneratorDelete(code);
  } else {
    AARCH64EmitGeneratedFunction(compiler, code, &module->program);
  }
  return true;
}

static bool AARCH64BuildProgram(Compiler* compiler,
                                AARCH64ObjectModule* module) {
  bool ok = !module->program.failed;
  for (size_t i = 0; i < compiler->functions.length && ok; i++) {
    AARCH64EmitGeneratedFunction(compiler, compiler->functions.value.p[i],
                                 &module->program);
    compiler->functions.value.p[i] = NULL;
    ok = !module->program.failed;
  }
  AARCH64EmitCXXAdjustorThunksToModule(&module->program);
  ok = ok && EmitTranslationUnitRemainderToModule(compiler,
                                                   &module->program);
  for (size_t i = 0; i < module->program.operations.length; i++) {
    AsmModuleOp* op = module->program.operations.value.p[i];
    if (op->kind == kAsmModuleOpText &&
        strcmp(op->u.text.name.value, "inline_asm") != 0 &&
        strcmp(op->u.text.name.value, "extended_asm") != 0) {
      fprintf(stderr,
              "AArch64 compiler output unexpectedly used generated text: %s\n",
              op->u.text.name.value);
      module->program.failed = true;
      ok = false;
    }
  }
  return ok;
}

static bool AARCH64WriteProgramAssembly(AARCH64ObjectModule* module,
                                        String* filename) {
  FILE* out = StringEqual(filename, "-") ? stdout : fopen(filename->value, "w");
  if (out == NULL) {
    return false;
  }
  bool ok = AARCH64ObjectModuleWriteAssembly(module, out);
  if (out != stdout && fclose(out) != 0) {
    ok = false;
  }
  return ok;
}

String* AARCH64EmitProgramFile(Compiler* compiler, Vector* options,
                               bool assembly_only) {
  String* result = AARCH64ProgramOutputFilename(
      compiler, options, assembly_only ? ".s" : ".o");
  String null_output;
  StringInit(&null_output, "/dev/null");
  String* assembler_output = assembly_only ? &null_output : result;
  AARCH64ObjectModule local_module;
  AARCH64ObjectModule* module = compiler->function_stream;
  if (module != NULL) {
    compiler->function_stream = NULL;
    compiler->function_stream_delete = NULL;
  } else {
    module = &local_module;
    AARCH64ObjectModuleStart(module, assembly_only || compiler->keep_asm_file);
  }

  bool ok = AARCH64ObjectModuleOpen(module, &compiler->infile, assembler_output,
                                    compiler->pic) &&
            AARCH64BuildProgram(compiler, module);
  if (ok && assembly_only) {
    ok = AARCH64WriteProgramAssembly(module, result);
  } else if (ok) {
    if (compiler->keep_asm_file) {
      String asm_filename;
      StringInit(&asm_filename, compiler->infile.value);
      ReplaceSourceExtension(&asm_filename, ".s");
      ok = AARCH64WriteProgramAssembly(module, &asm_filename);
      StringDestruct(&asm_filename);
    }
    ok = ok && AARCH64ObjectModuleFinalize(module);
  }
  AARCH64ObjectModuleDestruct(module);
  if (module != &local_module) {
    free(module);
  }
  StringDestruct(&null_output);
  if (!ok) {
    StringDelete(result);
    return NULL;
  }
  return result;
}

String* AARCH64EmitObjectFile(Compiler* compiler, Vector* options) {
  return AARCH64EmitProgramFile(compiler, options, false);
}
