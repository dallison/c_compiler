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

static bool AssembleNamed(const char* name, const char* text, Assembler* assembler,
                          char* path) {
  String input;
  String output;
  StringInit(&input, text);
  StringInit(&output, path);
  int relocs[kNumRelocTypes] = {0};
  bool ok = AssemblerInitFromString(assembler, ELF_MACHINE_TYPE_X86_64, 0,
                                    relocs, name, &input, &output);
  StringDestruct(&input);
  StringDestruct(&output);
  if (!ok) {
    return false;
  }
  AssemblerRun(assembler, NoInstruction);
  return true;
}

static const Buffer* SectionBytes(Assembler* assembler, const char* name) {
  for (size_t i = 0; i < assembler->object.sections.length; i++) {
    AssemblerSection* section = assembler->object.sections.value.p[i];
    if (section->name != NULL && strcmp(section->name->value, name) == 0 &&
        section->contents.data_location == kSectionContentsBuffered) {
      return &section->contents.data.buffered;
    }
  }
  return NULL;
}

static bool BytesMatch(const Buffer* bytes, const unsigned char* want,
                       size_t count) {
  if (bytes == NULL || bytes->length < count) {
    return false;
  }
  if (memcmp(bytes->value, want, count) != 0) {
    return false;
  }
  for (size_t i = count; i < bytes->length; i++) {
    if (bytes->value[i] != 0) {
      return false;
    }
  }
  return true;
}

static void ShowBytes(const char* name, Assembler* assembler, const char* section) {
  const Buffer* bytes = SectionBytes(assembler, section);
  fprintf(stderr, "%s: errors=%d %s length=%zu\n", name, assembler->num_errors,
          section, bytes != NULL ? bytes->length : 0);
  if (bytes == NULL) {
    return;
  }
  fprintf(stderr, "  got:");
  for (size_t i = 0; i < bytes->length; i++) {
    fprintf(stderr, " %02x", (unsigned char)bytes->value[i]);
  }
  fprintf(stderr, "\n");
}

static void ExpectSection(const char* name, const char* text, const char* section,
                          const unsigned char* want, size_t count) {
  char path[] = "/tmp/asm-control-XXXXXX";
  int fd = mkstemp(path);
  if (fd < 0) {
    fprintf(stderr, "%s: mkstemp failed\n", name);
    g_failures++;
    return;
  }
  close(fd);
  Assembler assembler;
  if (!AssembleNamed("control.s", text, &assembler, path)) {
    fprintf(stderr, "%s: assembler init failed\n", name);
    g_failures++;
    unlink(path);
    return;
  }
  const Buffer* bytes = SectionBytes(&assembler, section);
  if (assembler.num_errors != 0 || !BytesMatch(bytes, want, count)) {
    ShowBytes(name, &assembler, section);
    g_failures++;
  }
  AssemblerDestruct(&assembler);
  unlink(path);
}

static void ExpectBytes(const char* name, const char* text,
                        const unsigned char* want, size_t count) {
  ExpectSection(name, text, ".text", want, count);
}

static void ExpectError(const char* name, const char* text) {
  char path[] = "/tmp/asm-control-XXXXXX";
  int fd = mkstemp(path);
  if (fd < 0) {
    g_failures++;
    return;
  }
  close(fd);
  Assembler assembler;
  if (!AssembleNamed("control.s", text, &assembler, path)) {
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

static void TestAssignAndFill(void) {
  static const unsigned char want[] = {5, 3, 2, 1, 2, 1, 2, 1, 9, 0, 4};
  ExpectBytes("assign-fill",
              ".equ n, 1\n"
              ".eqv m, n+1\n"
              ".equ n, 4\n"
              ".byte m\n"
              ".set a, 1\n"
              ".set a, 3\n"
              ".byte a\n"
              ".fill 3, 2, 0x0102\n"
              ".byte 9\n"
              ".org 10\n"
              ".byte 4\n",
              want, sizeof(want));
}

static void TestIfnotdefAndSubsection(void) {
  static const unsigned char want[] = {1, 2, 3, 3};
  ExpectBytes("ifnotdef-sub",
              ".ifnotdef missing\n"
              ".byte 1\n"
              ".else\n"
              ".byte 9\n"
              ".endif\n"
              ".set missing, 1\n"
              ".ifnotdef missing\n"
              ".byte 9\n"
              ".endif\n"
              ".subsection 1\n"
              "later:\n"
              ".byte 3\n"
              ".subsection 0\n"
              ".byte 2\n"
              ".byte later\n",
              want, sizeof(want));
}

static void TestSectionStack(void) {
  static const unsigned char text[] = {8};
  static const unsigned char data[] = {9, 7};
  ExpectSection("section-text",
                ".section .data\n"
                ".byte 9\n"
                ".previous\n"
                ".byte 8\n"
                ".previous\n"
                ".byte 7\n",
                ".text", text, sizeof(text));
  ExpectSection("section-data",
                ".section .data\n"
                ".byte 9\n"
                ".previous\n"
                ".byte 8\n"
                ".previous\n"
                ".byte 7\n",
                ".data", data, sizeof(data));
  static const unsigned char pushed_text[] = {1, 3, 4};
  static const unsigned char pushed_data[] = {2};
  ExpectSection("push-text",
                ".byte 1\n"
                ".pushsection .data\n"
                ".byte 2\n"
                ".previous\n"
                ".byte 3\n"
                ".popsection\n"
                ".byte 4\n",
                ".text", pushed_text, sizeof(pushed_text));
  ExpectSection("push-data",
                ".byte 1\n"
                ".pushsection .data\n"
                ".byte 2\n"
                ".previous\n"
                ".byte 3\n"
                ".popsection\n"
                ".byte 4\n",
                ".data", pushed_data, sizeof(pushed_data));
}

static bool WriteFile(const char* path, const void* data, size_t size) {
  FILE* fp = fopen(path, "wb");
  if (fp == NULL) {
    return false;
  }
  bool ok = fwrite(data, 1, size, fp) == size;
  fclose(fp);
  return ok;
}

static void TestInclude(void) {
  char dir[] = "/tmp/asm-ctl-XXXXXX";
  if (mkdtemp(dir) == NULL) {
    fprintf(stderr, "include: mkdtemp failed\n");
    g_failures++;
    return;
  }
  char piece[256];
  char binary[256];
  char main_name[256];
  char obj[] = "/tmp/asm-control-XXXXXX";
  snprintf(piece, sizeof(piece), "%s/piece.s", dir);
  snprintf(binary, sizeof(binary), "%s/blob.bin", dir);
  snprintf(main_name, sizeof(main_name), "%s/main.s", dir);
  int fd = mkstemp(obj);
  if (fd < 0 || !WriteFile(piece, ".byte 9\n", 8) ||
      !WriteFile(binary, "\x11\x22\x33\x44", 4)) {
    fprintf(stderr, "include: setup failed\n");
    g_failures++;
    return;
  }
  close(fd);
  char source[512];
  snprintf(source, sizeof(source),
           ".include \"piece.s\"\n.incbin \"blob.bin\", 1, 2\n");
  Assembler assembler;
  if (!AssembleNamed(main_name, source, &assembler, obj)) {
    fprintf(stderr, "include: assembler init failed\n");
    g_failures++;
  } else {
    static const unsigned char want[] = {9, 0x22, 0x33};
    const Buffer* bytes = SectionBytes(&assembler, ".text");
    if (assembler.num_errors != 0 || !BytesMatch(bytes, want, sizeof(want))) {
      ShowBytes("include", &assembler, ".text");
      g_failures++;
    }
    AssemblerDestruct(&assembler);
  }
  unlink(piece);
  unlink(binary);
  unlink(obj);
  rmdir(dir);
}

static void TestDiagnostics(void) {
  ExpectError("equiv", ".equiv q, 1\n.equiv q, 2\n");
  ExpectError("err", ".err\n");
  ExpectError("error", ".error \"nope\"\n");
  ExpectError("fail", ".fail 0\n");
  ExpectError("abort", ".abort\n.byte 1\n");
  ExpectError("org-back", ".org 4\n.org 1\n");
  ExpectError("missing-include", ".include \"no-such-file.s\"\n");
  ExpectError("pop", ".popsection\n");
  static const unsigned char want[] = {1};
  ExpectBytes("skipped-abort",
              ".if 0\n.abort\n.endif\n.byte 1\n", want, sizeof(want));
  ExpectBytes("fail-warning", ".fail 500\n.byte 1\n", want, sizeof(want));
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
  if (!CompilerInitFromString(compiler, "asm_control_test", "", options)) {
    fprintf(stderr, "compiler init failed\n");
    exit(1);
  }
}

int main(void) {
  InitCompiler();
  TestAssignAndFill();
  TestIfnotdefAndSubsection();
  TestSectionStack();
  TestInclude();
  TestDiagnostics();
  if (g_failures != 0) {
    fprintf(stderr, "%d control tests failed\n", g_failures);
    return 1;
  }
  return 0;
}
