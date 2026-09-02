#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assembler/asm_module.h"
#include "elf.h"

static char* Render(const AsmModule* module) {
  FILE* file = tmpfile();
  assert(file != NULL);
  assert(AsmModuleWriteText(module, file));
  assert(fseek(file, 0, SEEK_END) == 0);
  long size = ftell(file);
  assert(size >= 0);
  assert(fseek(file, 0, SEEK_SET) == 0);
  char* text = malloc((size_t)size + 1);
  assert(text != NULL);
  assert(fread(text, 1, (size_t)size, file) == (size_t)size);
  text[size] = '\0';
  fclose(file);
  return text;
}

static void TestRetainedOperationsAndText(void) {
  AsmModule module;
  AsmModuleInit(&module, NULL);
  AsmModuleSection(&module, ".data", SHT(progbits),
                   SHF(alloc) | SHF(write), 8);
  AsmModuleSymbol(&module, "value", SYM_TYPE(object), SYM_BIND(global), 0, 8,
                  false, true, false);
  AsmModuleLabel(&module, "value");

  AsmExpr reference;
  AsmExprInitSymbol(&reference, "external+16", 4);
  assert(strcmp(reference.symbol.value, "external") == 0);
  assert(reference.addend == 20);
  AsmModuleInteger(&module, 8, &reference);
  AsmExprDestruct(&reference);

  AsmExpr difference;
  AsmExprInitDifference(&difference, "value_end", "value", 0);
  AsmModuleInteger(&module, 4, &difference);
  AsmExprDestruct(&difference);
  AsmModuleComment(&module, "retained, zero-sized comment");
  AsmModuleLabel(&module, "value_end");

  assert(!module.failed);
  assert(module.operations.length == 7);
  assert(((AsmModuleOp*)module.operations.value.p[5])->kind ==
         kAsmModuleOpComment);

  char* text = Render(&module);
  assert(strstr(text, "\t.data\n") != NULL);
  assert(strstr(text, "\t.p2align 3\n") != NULL);
  assert(strstr(text, "value:\n") != NULL);
  assert(strstr(text, "\t.8byte external+20\n") != NULL);
  assert(strstr(text, "\t.word (value_end-value)\n") != NULL);
  assert(strstr(text, "\t// retained, zero-sized comment\n") != NULL);
  free(text);
  AsmModuleDestruct(&module);
}

static void TestInvalidOperationsFailModule(void) {
  AsmModule module;
  AsmModuleInit(&module, NULL);
  AsmModuleSection(&module, ".text", SHT(progbits), SHF(alloc), 3);
  assert(module.failed);
  assert(module.operations.length == 0);
  FILE* file = tmpfile();
  assert(file != NULL);
  assert(!AsmModuleWriteText(&module, file));
  fclose(file);
  AsmModuleDestruct(&module);
}

int main(void) {
  TestRetainedOperationsAndText();
  TestInvalidOperationsFailModule();
  return 0;
}
