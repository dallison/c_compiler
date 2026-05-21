//
//  main.c
//  syntax_test
//
//  Created by David Allison on 10/30/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include <stdio.h>
#include "compiler.h"

void test1() {
  Vector options;
  const char* code =
  "void func1(int x, int y) {\n"
  "  int a = x;\n"
  "}\n";
  CompileTranslationUnitFromString("test1", code, &options);
}

void test2() {
  Vector options;
  const char* code =
  "void func2(int x, int y) {\n"
  "  int a = x;\n"
  "  int b = y+1*a;\n"
  "}\n";
  CompileTranslationUnitFromString("test2", code, &options);
}

void test_struct1() {
  Vector options;
  const char* code =
  "struct Test {\n"
  "  int a;\n"
  "  char b;\n"
  "} foobar;\n";
  CompileTranslationUnitFromString("test_struct1", code, &options);
}

void test_struct2() {
  Vector options;
  const char* code = "struct Test {\n"
  "  int a;\n"
  "  char b;\n"
  "};\n"
  "\n"
  "struct Test foobar2;\n";
  CompileTranslationUnitFromString("test_struct2", code, &options);
}

void test_typedef() {
  Vector options;
  const char* code =
  "typedef int Foo;\n"
  "const Foo foo;\n";
  CompileTranslationUnitFromString("test_typedef", code, &options);
}

void test_if() {
  Vector options;
  const char* code =
  "void foobar(void) {\n"
  "  int a = 0;\n"
  "  if (a <= 10) {\n"
  "    int b = 5;\n"
  "  } else {\n"
  "    int c = 4;\n"
  "  }\n"
  "  return a*10;\n"
  "}\n";
  CompileTranslationUnitFromString("test_if", code, &options);
}

void test_enum() {
  Vector options;
  const char* code =
  "enum Color {\n"
  "  RED,\n"
  "  GREEN = 5,"
  "  BLUE,\n"
  "};"
  "enum Color color;\n"
  "void func() {\n"
  "  enum Color c = GREEN;\n"
  "}\n";
  CompileTranslationUnitFromString("test_enum", code, &options);
}

void test_goto() {
  Vector options;
  const char* code =
  "void foobar() {\n"
  "  goto label;\n"
  "  while (1) {\n"
  "    bar();\n"
  "  }\n"
  "label  :\n"
  " foo();\n"
  "}\n";
  CompileTranslationUnitFromString("test_goto", code, &options);
}

void test_semantic1() {
  Vector options;
  const char* code =
  "void foobar() {\n"
  "  int x = 0;\n"
  "  double y = 1;\n"
  "  short s = 3;\n"
  "  long l = 4;\n"
  "  x + y;\n"
  "  s * l;\n"
  "}\n";
  CompileTranslationUnitFromString("test_semantic1", code, &options);
}

void test_preprocessor1() {
  Vector options;
  const char* code =
  "#foobar\n"
  "# 124 \"dave\"\n"
  "#define dave1(a,a) ddd\n"
  "#define foo 1\n"
  "#define foo 2\n"
  "#define bar hello world\n"
  "#define dave(a,b) foobar\n"
  "#define dave(a) foobar\n"
  "#define fred(a,...) foobar\n"
  "#define foobar(a,b,c,d) hello\\\n"
  "world\\\n"
  "again\n"
  "\n";
  CompileTranslationUnitFromString("test_preprocessor1", code, &options);
}

void test_preprocessor2() {
  Vector options;
  const char* code =
  "#define foo 1\n"
  "#ifdef foo\n"
  "int dave() { int x; }\n"
  "#else\n"
  "foo bar baz\n"
  "#endif\n"
  "\n";
  CompileTranslationUnitFromString("test_preprocessor1", code, &options);
}

void test_preprocessor3() {
  Vector options;
  const char* code =
  "#define foo 1\n"
  "#if foo < 2\n"
  "int dave() { int x; }\n"
  "#elif foo >= 1\n"
  "int dave2() { int y; }\n"
  "#else\n"
  "foo bar baz\n"
  "#endif\n"
  "\n";
  CompileTranslationUnitFromString("test_preprocessor1", code, &options);
}

void test_preprocessor4() {
  Vector options;
  const char* code =
  "#define foo dave\n"
  "void foo() { int foo = __LINE__; }\n"
  "\n";
  CompileTranslationUnitFromString("test_preprocessor1", code, &options);
}

void test_preprocessor5() {
  Vector options;
  const char* code =
  "#define foo(a,b) void a(int b) {int x;}\n"
  "foo(dave, burt)\n"
  "\n";
  CompileTranslationUnitFromString("test_preprocessor1", code, &options);
}

int main(int argc, const char * argv[]) {
//  test1();
//  test2();
//  test_struct1();
//  test_struct2();
//  test_typedef();
//  test_if();
//  test_enum();
// test_goto();
//  test_semantic1();
//  test_preprocessor1();
//  test_preprocessor2();
//  test_preprocessor3();
//  test_preprocessor4();
  test_preprocessor5();
}
