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

bool AARCH64ObjectModuleInit(AARCH64ObjectModule* module, String* src_file,
                             String* object_file, bool pic) {
  memset(module, 0, sizeof(*module));
  StringInit(&module->object_file, object_file->value);
  StringInit(&module->src_file, src_file->value);
  module->pic = pic;
  AsmModuleInit(&module->program, AARCH64ProgramTargetOps());

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

  EmitAssemblyPreambleToModule(compiler, &module->program);
  AsmModuleLabel(&module->program, ".PCbegin");
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
  AsmModuleDestruct(&module->program);
  if (module->owns_assembler) {
    AARCH64AssemblerDestruct(&module->assembler);
    module->owns_assembler = false;
  }
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

static bool AARCH64BuildProgram(Compiler* compiler,
                                AARCH64ObjectModule* module) {
  bool ok = true;
  for (size_t i = 0; i < compiler->functions.length && ok; i++) {
    AARCH64Generator* generator = compiler->functions.value.p[i];
    if (compiler->print_back_end) {
      AARCH64Emitter debug_emitter;
      AARCH64EmitterInit(&debug_emitter, generator);
      AARCH64EmitFunction(&debug_emitter, stdout);
      AARCH64EmitterDestruct(&debug_emitter);
    }
    AARCH64Emitter emitter;
    AARCH64EmitterInit(&emitter, generator);
    AARCH64EmitFunctionToModule(&emitter, &module->program);
    AARCH64EmitterDestruct(&emitter);
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
  AARCH64ObjectModule module;
  if (!AARCH64ObjectModuleInit(&module, &compiler->infile, assembler_output,
                               compiler->pic)) {
    StringDestruct(&null_output);
    StringDelete(result);
    return NULL;
  }

  bool ok = AARCH64BuildProgram(compiler, &module);
  if (!ok) {
    AARCH64ObjectModuleDestruct(&module);
    StringDestruct(&null_output);
    StringDelete(result);
    return NULL;
  }

  if (assembly_only) {
    ok = AARCH64WriteProgramAssembly(&module, result);
  } else {
    if (compiler->keep_asm_file) {
      String asm_filename;
      StringInit(&asm_filename, compiler->infile.value);
      ReplaceSourceExtension(&asm_filename, ".s");
      ok = AARCH64WriteProgramAssembly(&module, &asm_filename);
      StringDestruct(&asm_filename);
    }
    ok = ok && AARCH64ObjectModuleFinalize(&module);
  }
  if (!ok) {
    AARCH64ObjectModuleDestruct(&module);
    StringDestruct(&null_output);
    StringDelete(result);
    return NULL;
  }
  AARCH64ObjectModuleDestruct(&module);
  StringDestruct(&null_output);
  return result;
}

String* AARCH64EmitObjectFile(Compiler* compiler, Vector* options) {
  return AARCH64EmitProgramFile(compiler, options, false);
}
