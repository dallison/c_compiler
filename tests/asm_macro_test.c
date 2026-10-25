#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "assembler/assembler.h"
#include "compiler.h"
#include "elf.h"
#include "options.h"

static int g_failures = 0;

static void NoInstruction(Assembler* assembler, String* word) {
  AssemblerError(assembler, "unexpected instruction `%s'", word->value);
}

static bool AssembleText(const char* text, Assembler* assembler, char* path) {
  String input;
  String output;
  StringInit(&input, text);
  StringInit(&output, path);
  int relocs[kNumRelocTypes] = {0};
  bool ok = AssemblerInitFromString(assembler, ELF_MACHINE_TYPE_X86_64, 0,
                                    relocs, "macro.s", &input, &output);
  StringDestruct(&input);
  StringDestruct(&output);
  if (!ok) {
    return false;
  }
  AssemblerRun(assembler, NoInstruction);
  return true;
}

static const Buffer* TextBytes(Assembler* assembler) {
  for (size_t i = 0; i < assembler->object.sections.length; i++) {
    AssemblerSection* section = assembler->object.sections.value.p[i];
    if (section->name != NULL && strcmp(section->name->value, ".text") == 0 &&
        section->contents.data_location == kSectionContentsBuffered) {
      return &section->contents.data.buffered;
    }
  }
  return NULL;
}

static void ExpectBytes(const char* name, const char* text,
                        const unsigned char* want, size_t count) {
  char path[] = "/tmp/asm-macro-XXXXXX";
  int fd = mkstemp(path);
  if (fd < 0) {
    fprintf(stderr, "%s: mkstemp failed\n", name);
    g_failures++;
    return;
  }
  close(fd);
  Assembler assembler;
  if (!AssembleText(text, &assembler, path)) {
    fprintf(stderr, "%s: assembler init failed\n", name);
    g_failures++;
    unlink(path);
    return;
  }
  const Buffer* bytes = TextBytes(&assembler);
  // .text is aligned to 8 bytes when the object is written, which zero-pads
  // the section after the bytes the program emitted.
  bool padding_ok = bytes != NULL && bytes->length >= count;
  if (padding_ok) {
    for (size_t i = count; i < bytes->length; i++) {
      if (bytes->value[i] != 0) {
        padding_ok = false;
        break;
      }
    }
  }
  bool match = assembler.num_errors == 0 && padding_ok &&
               memcmp(bytes->value, want, count) == 0;
  if (!match) {
    fprintf(stderr, "%s: errors=%d length=%zu\n", name, assembler.num_errors,
            bytes != NULL ? bytes->length : 0);
    if (bytes != NULL) {
      fprintf(stderr, "  got:");
      for (size_t i = 0; i < bytes->length; i++) {
        fprintf(stderr, " %02x", (unsigned char)bytes->value[i]);
      }
      fprintf(stderr, "\n  want:");
      for (size_t i = 0; i < count; i++) {
        fprintf(stderr, " %02x", want[i]);
      }
      fprintf(stderr, "\n");
    }
    g_failures++;
  }
  AssemblerDestruct(&assembler);
  unlink(path);
}

static void ExpectError(const char* name, const char* text) {
  char path[] = "/tmp/asm-macro-XXXXXX";
  int fd = mkstemp(path);
  if (fd < 0) {
    fprintf(stderr, "%s: mkstemp failed\n", name);
    g_failures++;
    return;
  }
  close(fd);
  Assembler assembler;
  if (!AssembleText(text, &assembler, path)) {
    fprintf(stderr, "%s: assembler init failed\n", name);
    g_failures++;
    unlink(path);
    return;
  }
  if (assembler.num_errors == 0) {
    fprintf(stderr, "%s: expected an error\n", name);
    g_failures++;
  }
  AssemblerDestruct(&assembler);
  unlink(path);
}

static void TestGasSum(void) {
  static const unsigned char want[] = {0, 1, 2, 3, 4, 5};
  ExpectBytes("sum",
              ".macro sum from=0, to=5\n"
              ".byte \\from\n"
              ".if \\to-\\from\n"
              "sum \"(\\from+1)\", \\to\n"
              ".endif\n"
              ".endm\n"
              "sum 0, 5\n",
              want, sizeof(want));
}

static void TestKeywordDefaultsAndCase(void) {
  static const unsigned char want[] = {1, 3, 4, 4};
  ExpectBytes("keyword",
              ".macro foo bar=1, baz=2\n"
              ".byte \\bar, \\baz\n"
              ".endm\n"
              "foo baz=3\n"
              "FOO 4, 4\n",
              want, sizeof(want));
}

static void TestAltmacro(void) {
  static const unsigned char want[] = {'b', 'a', 'z', '=', '3', 'a', ',', 'b',
                                       3};
  ExpectBytes("altmacro",
              ".macro foo bar=1, baz=2\n"
              ".ascii \"\\bar\"\n"
              ".endm\n"
              ".altmacro\n"
              "foo baz=3\n"
              ".macro show a\n"
              ".ascii \"\\a\"\n"
              ".endm\n"
              "show <a,b>\n"
              ".macro num x\n"
              ".byte \\x\n"
              ".endm\n"
              "num %1+2\n"
              ".noaltmacro\n",
              want, sizeof(want));
}

static void TestSeparatorAndCounters(void) {
  static const unsigned char want[] = {'h', 'e', 'l', 'l', 'o', 1, 0, 2, 1};
  ExpectBytes("separator",
              ".macro cat a, b\n"
              ".ascii \"\\a\\()\\b\"\n"
              ".endm\n"
              "cat hel, lo\n"
              ".macro id\n"
              ".byte \\@, \\+\n"
              ".endm\n"
              "id\n"
              "id\n",
              want, sizeof(want));
}

static void TestReptIrpExit(void) {
  static const unsigned char want[] = {7, 7, 7, 1, 2, 3, 'A', 'B', 'C', 1};
  ExpectBytes("repeat",
              ".rept 3\n"
              ".byte 7\n"
              ".endr\n"
              ".irp n,1,2,3\n"
              ".byte \\n\n"
              ".endr\n"
              ".irpc c,ABC\n"
              ".ascii \"\\c\"\n"
              ".endr\n"
              ".macro once\n"
              ".byte 1\n"
              ".exitm\n"
              ".byte 9\n"
              ".endm\n"
              "once\n",
              want, sizeof(want));
}

static void TestConditionalsVarargAndCallTail(void) {
  static const unsigned char want[] = {2, 1, 2, 9, 1, 2, '1', '|', '2', ',', ' ',
                                       '3', 1, 2};
  ExpectBytes("cond",
              ".if 0\n"
              ".byte 1\n"
              ".else\n"
              ".byte 2\n"
              ".endif\n"
              ".macro pick a\n"
              ".ifb \\a\n"
              ".byte 1\n"
              ".else\n"
              ".byte 2\n"
              ".endif\n"
              ".endm\n"
              "pick\n"
              "pick 3\n"
              ".ifeq 1-1\n"
              ".byte 9\n"
              ".endif\n"
              ".macro emit\n"
              ".byte 1\n"
              ".endm\n"
              "emit; .byte 2\n"
              ".macro m a:req, b:vararg\n"
              ".ascii \"\\a|\\b\"\n"
              ".endm\n"
              "m 1, 2, 3\n"
              ".if 0\n"
              ".byte 8\n"
              ".elseif 1\n"
              ".byte 1\n"
              ".else\n"
              ".byte 4\n"
              ".endif\n"
              ".byte 2\n",
              want, sizeof(want));
}

static void TestLocalPurgem(void) {
  static const unsigned char want[] = {1, 1, 5};
  ExpectBytes("local",
              ".altmacro\n"
              ".macro lab\n"
              "LOCAL L\n"
              "L:\n"
              ".byte 1\n"
              ".endm\n"
              "lab\n"
              "lab\n"
              ".noaltmacro\n"
              ".macro old\n"
              ".byte 4\n"
              ".endm\n"
              ".purgem old\n"
              ".macro old\n"
              ".byte 5\n"
              ".endm\n"
              "old\n",
              want, sizeof(want));
  char path[] = "/tmp/asm-macro-XXXXXX";
  int fd = mkstemp(path);
  if (fd < 0) {
    g_failures++;
    return;
  }
  close(fd);
  Assembler assembler;
  if (!AssembleText(".altmacro\n.macro lab\nLOCAL L\nL:\n.byte 1\n.endm\n"
                    "lab\nlab\n",
                    &assembler, path)) {
    g_failures++;
    unlink(path);
    return;
  }
  if (AssemblerFindSymbol(&assembler, ".LL0001") == NULL ||
      AssemblerFindSymbol(&assembler, ".LL0002") == NULL) {
    fprintf(stderr, "local: missing LOCAL symbols\n");
    g_failures++;
  }
  AssemblerDestruct(&assembler);
  unlink(path);
}

static void TestErrors(void) {
  ExpectError("required",
              ".macro m a:req\n"
              ".byte \\a\n"
              ".endm\n"
              "m\n");
  ExpectError("keyword-then-positional",
              ".macro m a, b\n"
              ".byte \\a\n"
              ".endm\n"
              "m b=2, 1\n");
  ExpectError("redefine",
              ".macro m\n"
              ".endm\n"
              ".macro m\n"
              ".endm\n");
  ExpectError("endr", ".endr\n");
}

static void InitCompiler(void) {
  Vector* options = NewVector();
  CompilerOptionValue* target = calloc(1, sizeof(CompilerOptionValue));
  target->opt = kOptionTarget;
  StringInit(&target->value.svalue, "pcode");
  VectorAppend(options, target);
  CompilerOptionValue* standard = calloc(1, sizeof(CompilerOptionValue));
  standard->opt = kOptionStandard;
  StringInit(&standard->value.svalue, "c11");
  VectorAppend(options, standard);
  compiler = calloc(1, sizeof(Compiler));
  if (!CompilerInitFromString(compiler, "asm_macro_test", "", options)) {
    fprintf(stderr, "compiler init failed\n");
    exit(1);
  }
}

int main(void) {
  InitCompiler();
  TestGasSum();
  TestKeywordDefaultsAndCase();
  TestAltmacro();
  TestSeparatorAndCounters();
  TestReptIrpExit();
  TestConditionalsVarargAndCallTail();
  TestLocalPurgem();
  TestErrors();
  if (g_failures != 0) {
    fprintf(stderr, "%d macro tests failed\n", g_failures);
    return 1;
  }
  printf("ok\n");
  return 0;
}
