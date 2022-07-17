//
//  preprocessor.c
//  c_compiler
//
//  Created by David Allison on 11/13/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "preprocessor.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#if defined(__APPLE__)
#include <dirent.h>
#include <sys/stat.h>
#endif

#include "compiler.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "lex.h"

// Forward declarations.
static void Tokenize(Preprocessor* p, String* input, String* output, size_t start,
                     bool init_output, bool assembler_mode,
                     bool header_names_ok);


static void ReplaceMacrosInTokenizedLine(Preprocessor* p, String* line,
                                         bool whole_input);


Macro* NewMacro(const char* name, bool is_function_like, bool varargs,
                Vector* args, String* replacement_text,
                SourceLocation location) {
  Macro* macro = malloc(sizeof(Macro));
  BinaryTreeNodeInit(&macro->header);
  StringInit(&macro->name, name);
  VectorInit(&macro->args);
  VectorCopy(&macro->args, args);
  StringInit(&macro->replacement_text, replacement_text->value);
  macro->is_function_like = is_function_like;
  macro->undefined = false;
  macro->varargs = varargs;
  macro->enabled = true;
  macro->location = location;
  return macro;
}


void MacroDestruct(Macro* macro) {
  StringDestruct(&macro->name);
  StringDestruct(&macro->replacement_text);
  VectorDestruct(&macro->args);
}

void MacroDisable(Macro* macro) {
  macro->enabled = false;
}

void MacroEnable(Macro* macro) {
  macro->enabled = true;
}

#define MACROS_TABLE_SIZE 1009

static int MacroInsertCompare(BinaryTreeNode* node1, BinaryTreeNode* node2) {
  Macro* macro1 = (Macro*)node1;
  Macro* macro2 = (Macro*)node2;
  return StringCompareString(&macro1->name, &macro2->name);
}

static int MacroSearchCompare(BinaryTreeNode* node, void* name) {
  Macro* macro = (Macro*)node;
  return StringCompare(&macro->name, name);
}

static void MacroDestructor(BinaryTreeNode* node, void* data) {
  MacroDestruct((Macro*)node);
}


static void DeleteMacroTree(void* tree, void* data) {
  BinaryTreeDestruct(tree, data);
  free(tree);
}

static size_t HashMacro(void* value, HashTable* table, HashMode mode) {
  const char* name;
  switch (mode) {
    case kHashInsert:
      // For insertion we have a pointer to macro.
      name = ((Macro*)value)->name.value;
      break;
    case kHashSearch:
      // For search we have pointer to the name.
      // to find.
      name = (const char*)value;
      break;
  }
  uint32_t hash = 5381;
  while (*name != '\0') {
    hash = (hash << 5) + hash + *name++;
  }
  return hash;
}

static bool InsertMacroInHashTable(void* entry, void* value, void** parent) {
  BinaryTree* tree = entry;
  if (entry == NULL) {
    tree = NewBinaryTree(MacroInsertCompare,
                         MacroSearchCompare,
                         MacroDestructor);
    *parent = tree;
  }
  return BinaryTreeInsert(tree, value);
}

static void* FindMacroInHashTable(void* entry, void* value) {
  BinaryTree* tree = entry;
  return BinaryTreeSearch(tree, value);
}

static void PredefineMacros(Preprocessor* p) {
  // Define the macros defined by the standard.
  PreprocessorDefineMacro(p, "__STDC__", "1");
  PreprocessorDefineMacro(p, "__STDC_HOSTED__", "1");
  PreprocessorDefineMacro(p, "__STDC_VERSION__", "199901L");
  PreprocessorDefineMacro(p, "__STDC_MB_MIGHT_NEQ_WC__", "1");

  PreprocessorDefineMacro(p, "__DAVECC__", "1");

  // Date and time are defined as coming from the 'asctime' function.
  // We are using 'ctime' because it's a wrapper for asctime.  We extract
  // the strings produced by it into the __DATE__ and __TIME__ macros.
  time_t now = time(NULL);
  char* now_string = ctime(&now);
  // now_string is of the form: Thu Nov 24 18:22:48 1986\n\0
  //                            ^   ^   ^  ^        ^
  //                            0   4   8  10       18

  // __DATE__ is defined to be of the form: Mmm dd yyyy
  String date_value;
  StringInitFromSegment(&date_value, &now_string[4], 4);  // Including space.
  StringAppendSegment(&date_value, &now_string[8], 3);    // Including space.
  StringAppendSegment(&date_value, &now_string[18], 4);
  PreprocessorDefineMacro(p, "__DATE__", date_value.value);
  StringDestruct(&date_value);

  // __TIME__ is of the form: hh:mm:ss
  // It is the time when the translation unit was translated, not the
  // time when the macro is replaced.  In other words, this does not
  // change during the compilation.
  String time_value;
  StringInitFromSegment(&time_value, &now_string[10], 8);
  PreprocessorDefineMacro(p, "__TIME__", time_value.value);
  StringDestruct(&time_value);

  // Define additional practical macros provided by some compilers and
  // need for include files in the OS.
  PreprocessorDefineMacro(p, "__GNUC__", "4");

  // Pretend to be llvm to get compatibility with code in standard header files.
  PreprocessorDefineMacro(p, "__llvm__", "1");

  // Pretend to be on linux.
  PreprocessorDefineMacro(p, "__linux__", "1");

  // These are defined by GCC and clang and are used in header files.  We need
  // to define them too.
  if (!StringEqual(compiler->target_name, "6502") &&
      !StringEqual(compiler->target_name, "65c02")) {
    PreprocessorDefineMacro(p, "__SIZE_TYPE__", "unsigned long");
    PreprocessorDefineMacro(p, "__PTRDIFF_TYPE__", "unsigned long");
    PreprocessorDefineMacro(p, "__WCHAR_TYPE__", "int");
    PreprocessorDefineMacro(p, "__WINT_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INTMAX_TYPE__", "int");
    PreprocessorDefineMacro(p, "__UINTMAX_TYPE__", "int");
    PreprocessorDefineMacro(p, "__SIG_ATOMIC_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INT8_TYPE__", "char");
    PreprocessorDefineMacro(p, "__INT16_TYPE__", "short");
    PreprocessorDefineMacro(p, "__INT32_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INT64_TYPE__", "long long");
    PreprocessorDefineMacro(p, "__UINT8_TYPE__", "unsigned char");
    PreprocessorDefineMacro(p, "__UINT16_TYPE__", "unsigned short");
    PreprocessorDefineMacro(p, "__UINT32_TYPE__", "unsigned int");
    PreprocessorDefineMacro(p, "__UINT64_TYPE__", "unsigned long long");
    PreprocessorDefineMacro(p, "__INT_LEAST8_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INT_LEAST16_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INT_LEAST32_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INT_LEAST64_TYPE__", "int");
    PreprocessorDefineMacro(p, "__UINT_LEAST8_TYPE__", "int");
    PreprocessorDefineMacro(p, "__UINT_LEAST16_TYPE__", "int");
    PreprocessorDefineMacro(p, "__UINT_LEAST32_TYPE__", "int");
    PreprocessorDefineMacro(p, "__UINT_LEAST64_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INT_FAST8_TYPE__", "char");
    PreprocessorDefineMacro(p, "__INT_FAST16_TYPE__", "short");
    PreprocessorDefineMacro(p, "__INT_FAST32_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INT_FAST64_TYPE__", "long");
    PreprocessorDefineMacro(p, "__UINT_FAST8_TYPE__", "char");
    PreprocessorDefineMacro(p, "__UINT_FAST16_TYPE__", "short");
    PreprocessorDefineMacro(p, "__UINT_FAST32_TYPE__", "int");
    PreprocessorDefineMacro(p, "__UINT_FAST64_TYPE__", "long");
    PreprocessorDefineMacro(p, "__INTPTR_TYPE__", "int*");
    PreprocessorDefineMacro(p, "__UINTPTR_TYPE__", "unsigned int*");
  } else {
    PreprocessorDefineMacro(p, "__SIZE_TYPE__", "unsigned int");
    PreprocessorDefineMacro(p, "__PTRDIFF_TYPE__", "unsigned int");
    PreprocessorDefineMacro(p, "__WCHAR_TYPE__", "int");
    PreprocessorDefineMacro(p, "__WINT_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INTMAX_TYPE__", "int");
    PreprocessorDefineMacro(p, "__UINTMAX_TYPE__", "int");
    PreprocessorDefineMacro(p, "__SIG_ATOMIC_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INT8_TYPE__", "char");
    PreprocessorDefineMacro(p, "__INT16_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INT32_TYPE__", "long");
    PreprocessorDefineMacro(p, "__INT64_TYPE__", "long long");
    PreprocessorDefineMacro(p, "__UINT8_TYPE__", "unsigned char");
    PreprocessorDefineMacro(p, "__UINT16_TYPE__", "unsigned int");
    PreprocessorDefineMacro(p, "__UINT32_TYPE__", "unsigned long");
    PreprocessorDefineMacro(p, "__UINT64_TYPE__", "unsigned long long");
    PreprocessorDefineMacro(p, "__INT_LEAST8_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INT_LEAST16_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INT_LEAST32_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INT_LEAST64_TYPE__", "int");
    PreprocessorDefineMacro(p, "__UINT_LEAST8_TYPE__", "int");
    PreprocessorDefineMacro(p, "__UINT_LEAST16_TYPE__", "int");
    PreprocessorDefineMacro(p, "__UINT_LEAST32_TYPE__", "int");
    PreprocessorDefineMacro(p, "__UINT_LEAST64_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INT_FAST8_TYPE__", "char");
    PreprocessorDefineMacro(p, "__INT_FAST16_TYPE__", "int");
    PreprocessorDefineMacro(p, "__INT_FAST32_TYPE__", "long");
    PreprocessorDefineMacro(p, "__INT_FAST64_TYPE__", "long long");
    PreprocessorDefineMacro(p, "__UINT_FAST8_TYPE__", "char");
    PreprocessorDefineMacro(p, "__UINT_FAST16_TYPE__", "int");
    PreprocessorDefineMacro(p, "__UINT_FAST32_TYPE__", "long");
    PreprocessorDefineMacro(p, "__UINT_FAST64_TYPE__", "long long");
    PreprocessorDefineMacro(p, "__INTPTR_TYPE__", "int*");
    PreprocessorDefineMacro(p, "__UINTPTR_TYPE__", "unsigned int*");
  }
}

void PreprocessorDefineArchitectureMacros(Preprocessor* p) {
  // Architecture macros.
  if (StringEqual(compiler->target_name, "x86_64")) {
    PreprocessorDefineMacro(p, "__x86_64__", "1");
  } else if (StringEqual(compiler->target_name, "arm")) {
    PreprocessorDefineMacro(p, "__arm__", "1");
  } else if (StringEqual(compiler->target_name, "p-code") ||
             StringEqual(compiler->target_name, "pcode")) {
    PreprocessorDefineMacro(p, "__p_code__", "1");
    PreprocessorDefineMacro(p, "__x86_64__", "1");
  } else if (StringEqual(compiler->target_name, "risc-v") ||
             StringEqual(compiler->target_name, "riscv")) {
    PreprocessorDefineMacro(p, "__risc_v__", "1");
    PreprocessorDefineMacro(p, "__x86_64__", "1");
  } else if (StringEqual(compiler->target_name, "6502")) {
    PreprocessorDefineMacro(p, "__6502__", "1");
  } else if (StringEqual(compiler->target_name, "65c02")) {
    PreprocessorDefineMacro(p, "__W65C02__", "1");
    PreprocessorDefineMacro(p, "__6502__", "1"); }
}

void PreprocessorInit(Preprocessor* p) {
  HashTableInit(&p->macros, "macros", MACROS_TABLE_SIZE, HashMacro,
                InsertMacroInHashTable, FindMacroInHashTable);
  VectorInit(&p->if_stack);
  p->lex = NULL;
  VectorInit(&p->user_include_paths);
  VectorInit(&p->system_include_paths);
  p->is_compiled_in = true;

#ifndef DAVECC_SYSROOT_HDRS
#error "Please define DAVECC_SYSROOT_HDRS to tell the compiler where the headers are"
#else
#define xstr(s) str(s)
#define str(s) #s
  PreprocessorAddSystemIncludePath(p, xstr(DAVECC_SYSROOT_HDRS));
#undef str
#undef xstr
#endif
  // Add current dir to the include paths.
  char current_dir[4096];
  getcwd(current_dir, sizeof(current_dir));

  PreprocessorAddUserIncludePath(p, current_dir);

  PredefineMacros(p);
}

void PreprocessorPrintStats(Preprocessor* p, FILE* fp) {
  HashTablePrintStats(&p->macros, fp);
}

void PreprocessorFinishInit(Preprocessor* p, struct Lex* lex) { p->lex = lex; }

void PreprocessorDestruct(Preprocessor* p) {
  VectorDestruct(&p->if_stack);

  // Delete all user paths.
  for (size_t i = 0; i < p->user_include_paths.length; i++) {
    StringDelete((String*)p->user_include_paths.value.p[i]);
  }
  VectorDestruct(&p->user_include_paths);

  // Delete all system paths.
  for (size_t i = 0; i < p->system_include_paths.length; i++) {
    StringDelete((String*)p->system_include_paths.value.p[i]);
  }
  VectorDestruct(&p->system_include_paths);

  PreprocessorReset(p);
}

void PreprocessorReset(Preprocessor* p) {
  HashTableTraverse(&p->macros, DeleteMacroTree, NULL);
  HashTableClear(&p->macros);
  PredefineMacros(p);
}

void PreprocessorAddUserIncludePath(Preprocessor* p, const char* path) {
  // Ensure only one entry with this path.
  for (size_t i = 0; i < p->user_include_paths.length; i++) {
    if (StringEqual(p->user_include_paths.value.p[i], path)) {
      return;
    }
  }
  VectorAppend(&p->user_include_paths, NewString(path));
}

void PreprocessorAddSystemIncludePath(Preprocessor* p, const char* path) {
  // Ensure only one entry with this path.
  for (size_t i = 0; i < p->system_include_paths.length; i++) {
    if (StringEqual(p->system_include_paths.value.p[i], path)) {
      return;
    }
  }
  VectorAppend(&p->system_include_paths, NewString(path));
}

void PreprocessorInsertSystemIncludePath(Preprocessor* p, int index,
                                         const char* path) {

   // Ensure only one entry with this path.
    for (size_t i = 0; i < p->system_include_paths.length; i++) {
      if (StringEqual(p->system_include_paths.value.p[i], path)) {
        return;
      }
    }
    VectorInsertBefore(&p->system_include_paths, 0, NewString(path));
  }

static void CopyMacro(BinaryTreeNode* node, int depth, void* data) {
  HashTable* to_table = data;
  Macro* macro = (Macro*)node;
  Vector* args = NewVector();
  for (size_t i = 0; i < macro->args.length; i++) {
    String* a = macro->args.value.p[i];
    VectorAppend(args, NewString(a->value));
  }
  HashTableInsert(to_table, NewMacro(macro->name.value,
                  macro->is_function_like, macro->varargs, args,
                  NewString(macro->replacement_text.value),
                  macro->location));

}

static void CopyMacroTree(void* m, void* data) {
  BinaryTreeTraverse(m, CopyMacro, data);
 }

void PreprocessorCopyOptions(Preprocessor* to, Preprocessor* from) {
  for (size_t i = 0; i < from->system_include_paths.length; i++) {
    String* path = from->system_include_paths.value.p[i];
    VectorAppend(&to->system_include_paths, NewString(path->value));
  }
  for (size_t i = 0; i < from->user_include_paths.length; i++) {
    String* path = from->user_include_paths.value.p[i];
    VectorAppend(&to->user_include_paths, NewString(path->value));
  }
  // Copy the macros.
  HashTableTraverse(&from->macros, CopyMacroTree, &to->macros);
}

void PreprocessorDefineMacro(Preprocessor* p, const char* macro_name,
                             const char* value) {
  String raw_value;
  StringInitImmutable(&raw_value, value);
  String tokens;
  Tokenize(p, &raw_value, &tokens, 0, true,
           p->lex == NULL ? false : p->lex->assembler_mode, false);

  Macro* macro = HashTableSearch(&p->macros, (char*)macro_name);
  if (macro != NULL) {
    // Macro exists, replace the value with that given.
    StringSetString(&macro->replacement_text, &tokens);
    macro->undefined = false;
    return;
  }
  macro = NewMacro(macro_name, false, false, NewVector(), &tokens,
                   SOURCE_LOCATION_COMMAND_LINE);
  HashTableInsert(&p->macros, macro);
  StringDestruct(&tokens);
}

void PreprocessorUndefineMacro(Preprocessor* p, String* macro_name) {
  Macro* macro = HashTableSearch(&p->macros, macro_name->value);
   if (macro != NULL) {
     macro->undefined = true;
   }
}

void PreprocessorError(Preprocessor* preprocessor, const char* error, ...) {
  va_list ap;
  va_start(ap, error);
  VReportError(preprocessor->lex->source->filename.value,
               preprocessor->lex->source->lineno, error, ap);
  va_end(ap);
}

void VPreprocessorError(Preprocessor* preprocessor, const char* error,
                        va_list ap) {
  VReportError(preprocessor->lex->source->filename.value,
               preprocessor->lex->source->lineno, error, ap);
}

void PreprocessorWarning(Preprocessor* preprocessor, const char* warn,
                         const char* error, ...) {
  va_list ap;
  va_start(ap, error);
  VReportWarning(preprocessor->lex->source->filename.value,
                 preprocessor->lex->source->lineno, warn, error, ap);
  va_end(ap);
}

void VPreprocessorWarning(Preprocessor* preprocessor, const char* warn,
                          const char* error, va_list ap) {
  VReportWarning(preprocessor->lex->source->filename.value,
                 preprocessor->lex->source->lineno, warn, error, ap);
}


//
// Tokenizer and detokenizer.
//


// Preprocessing tokens.  Those marked with (*) have encoded
// length and value.
#define PPTOK(x) kPreprocessingToken_##x

typedef enum {
  PPTOK(identifier) = 1,      // Any identifier (*)
  PPTOK(other_char),          // Single character.
  PPTOK(other_string),        // Multiple characters (*)
  PPTOK(number),              // Number (int or fp) (*)
  PPTOK(defined),             // "defined(macro)" (*)
  PPTOK(literal),             // String literal. (*)
  PPTOK(wide_literal),        // Wide string literal (*)
  PPTOK(char_literal),        // Char literal (*)
  PPTOK(wide_char_literal),   // Wide char literal (*)
  PPTOK(hash),                // Single #.
  PPTOK(hashhash),            // ## (token concatenation).
  PPTOK(placemarker),         // Special
  PPTOK(openparen),           // (
  PPTOK(closeparen),          // )
  PPTOK(comma),               // ,
  PPTOK(comment),             // comment (*)
  PPTOK(space),               // single space char
  PPTOK(system_header),       // system header name
  PPTOK(end),                 // Not present in token stream.
} PreprocessingToken;

// This provides the ability to iterate over the preprocessing tokens
// in a token sequence.
//
typedef struct {
  Preprocessor* p;    // Preprocessor.
  String* input;      // String this iterates through.
  size_t prev;        // Index of start of previous token.
  size_t curr;        // Index of start of current token.
  size_t next;        // Index of start of next token.
} TokenIterator;

// Encode the integer length into the string in LEB128.
// Since we are putting the output in a string, a value of zero is
// problematic since it will interfere with the string copy operations.
static void EncodeLength(String* output, size_t v) {
  v += 1;         // Don't encode 0 as zero (interferes with string end).
  for (;;) {
    if ((v & ~0x7fLL) == 0) {
      StringAppendChar(output, (char)(v & 0x7f));
      return;
    }
    StringAppendChar(output, (char)(v & 0x7f) | 0x80);
    v >>= 7;
  }
}

// Decode length of a token and return position after last byte.
static size_t DecodeLength(String* input, size_t pos, size_t* length) {
  int bits = 0;
  size_t result = 0;
  for (;;) {
    char b = input->value[pos++];
    result |= (b & 0x7f) << bits;
    if ((b & 0x80) == 0) {
      *length = result - 1;
      return pos;
    }
    bits += 7;
  }
}

// Given a TokenIterator, use the current token to find the next
// token and return its index.  Skips space tokens.
static size_t FindNextTokenIndex(TokenIterator* t) {
  size_t length = 0;
  size_t curr = t->curr;
  if (curr >= t->input->length) {
    return curr;
  }
  PreprocessingToken tok = (PreprocessingToken)t->input->value[curr];
  switch (tok) {
    case PPTOK(identifier):
    case PPTOK(other_string):
    case PPTOK(number):
    case PPTOK(defined):
    case PPTOK(literal):
    case PPTOK(wide_literal):
    case PPTOK(char_literal):
    case PPTOK(wide_char_literal):
    case PPTOK(system_header): {
    case PPTOK(comment):
      curr = DecodeLength(t->input, curr+1, &length);
      break;
    }
    case PPTOK(other_char):
      length = 2;
      break;
    case PPTOK(hash):
    case PPTOK(hashhash):
    case PPTOK(placemarker):
    case PPTOK(openparen):
    case PPTOK(closeparen):
    case PPTOK(comma):
    case PPTOK(space):
      length = 1;
      break;
    case PPTOK(end):
      break;
  }
  return curr + length;
}

static void TokenIteratorInit(TokenIterator* t, Preprocessor* p, String* input) {
  t->p = p;
  t->input = input;
  t->prev = 0;
  t->curr = 0;
  t->next = FindNextTokenIndex(t);
}

static PreprocessingToken CurrentToken(TokenIterator* t) {
  if (t->curr >= t->input->length) {
    return PPTOK(end);
  }
  return (PreprocessingToken)t->input->value[t->curr];
}

static PreprocessingToken PrevToken(TokenIterator* t) {
  if (t->prev >= t->input->length) {
    return PPTOK(end);
  }
  return (PreprocessingToken)t->input->value[t->prev];
}

static PreprocessingToken NextToken(TokenIterator* t) {
  if (t->next >= t->input->length) {
    return PPTOK(end);
  }
  return (PreprocessingToken)t->input->value[t->next];
}

static void AppendCurrentToken(TokenIterator* t, String* output) {
  size_t tok_len = t->next - t->curr;
  StringAppendSegment(output, &t->input->value[t->curr], tok_len);
}

static void AppendTokenSpelling(String* tokens,
                                size_t index,
                                String* output) {
  switch ((PreprocessingToken)tokens->value[index]) {
    case PPTOK(identifier):
    case PPTOK(other_string):
    case PPTOK(number):
    case PPTOK(defined):
    case PPTOK(comment):
    case PPTOK(literal):
    case PPTOK(wide_literal):
    case PPTOK(char_literal):
    case PPTOK(system_header):
    case PPTOK(wide_char_literal): {
      size_t length = 0;
      index = DecodeLength(tokens, index+1, &length);
      StringAppendSegment(output, &tokens->value[index], length);
      break;
    }
      
    case PPTOK(other_char):
      StringAppendChar(output, tokens->value[index+1]);
      break;
      
    case PPTOK(hash):
      StringAppendChar(output, '#');
      break;
    case PPTOK(hashhash):
      StringAppend(output, "##");
      break;
    case PPTOK(placemarker):
      break;
    case PPTOK(openparen):
      StringAppendChar(output, '(');
      break;
    case PPTOK(closeparen):
      StringAppendChar(output, ')');
      break;
    case PPTOK(comma):
      StringAppendChar(output, ',');
      break;
    case PPTOK(space):
      StringAppendChar(output, ' ');
      break;
    case PPTOK(end):
      break;
  }
}

// Append the spelling of the current token to the output string.
static void AppendCurrentTokenSpelling(TokenIterator* t, String* output) {
  AppendTokenSpelling(t->input, t->curr, output);
}

// Read the spelling of the current token into the string.
// Initializes the string.
static void GetCurrentTokenSpelling(TokenIterator* t, String* spelling) {
  StringInit(spelling, NULL);
  AppendCurrentTokenSpelling(t, spelling);
}

static bool IsSpaceToken(TokenIterator* t) {
  char tok = CurrentToken(t);
  return tok == PPTOK(space) || tok == PPTOK(comment);
}

static void MoveToNextToken(TokenIterator* t) {
  // Set prev to curr as long as curr isn't a space.
  if (!IsSpaceToken(t)) {
    t->prev = t->curr;
  }
  t->curr = t->next;
  t->next = FindNextTokenIndex(t);
}


static size_t SkipIntegerSuffix(String* line, size_t pos) {
  char ch = toupper(line->value[pos]);
  bool foundu = false;
  if (ch == 'U') {
    pos++;
    foundu = true;
  }
  ch = toupper(line->value[pos]);
  if (ch == 'L') {
    pos++;
  }
  ch = toupper(line->value[pos]);
  if (ch == 'L') {
    pos++;
  }
  if (!foundu) {
    ch = toupper(line->value[pos]);
    // Allow U to appear after L or LL.
    if (ch == 'U') {
      pos++;
    }
  }
  return pos;
}

// Collect a floating point suffix
// Allows F or L.
static size_t SkipFloatingSuffix(String* line, size_t pos) {
  char ch = toupper(line->value[pos]);
  if (ch == 'F') {
    pos++;
  } else {
    ch = toupper(line->value[pos]);
    if (ch == 'L') {
      pos++;
    }
  }
  return pos;
}

// Skip a number preprocessing token.
static size_t SkipNumber(String* line, size_t pos) {
  bool seenexp = false;      // Have we seen an exponent?
  bool seendot = false;  // Have we seen a dot?
  bool seensign = false;     // Have we seen a sign char?
  
  while (pos < line->length) {
    char ch = line->value[pos];
    if (ch == '.') {
      if (seendot) {
        // Two dots terminate number.
        break;
      }
      seendot = true;
    } else if (ch == 'e' || ch == 'E') {
      if (seenexp) {
        // Already seen exponent, terminate.
        break;
      }
      seenexp = true;
    } else if (ch == '+' || ch == '-') {
      if (!seenexp || seensign) {
        // Signs can only be after exponent.
        break;
      }
      seensign = true;
    } else if (!isdigit(ch)) {
      // Not a digit, terminate.
      break;
    }
    pos++;
  }
  
  // Now we can determine the type.  If we've seen a dot
  // or exponent then we are a floating point number.
  // A suffix of ‘F’ or ‘f’ is also floating point.
  bool isfp = seendot || seenexp || toupper(line->value[pos]) == 'F';
  
  // Skip the appropriate type of suffix.
  if (isfp) {
    SkipFloatingSuffix(line, pos);
  } else {
    SkipIntegerSuffix(line, pos);
  }
  return pos;
}

static size_t AppendNumber(String* input, String* output, size_t pos) {
  size_t start = pos;
  pos = SkipNumber(input, pos);
  size_t length = pos - start;
  EncodeLength(output, length);
  StringAppendSegment(output, &input->value[start], length);
  return pos;
}

static size_t AppendStringLiteral(String* input, String* output, size_t pos) {
  // Count length, not including quotes.
  size_t start = pos;
  while (pos < input->length) {
    char ch = input->value[pos];
    if (ch == '"') {
      break;
    }
    if (ch == '\\') {
      pos++;
    }
    pos++;
  }
  size_t length = pos - start;
  EncodeLength(output, length);
  StringAppendSegment(output, &input->value[start], length);
  return pos + 1;
}

static size_t AppendCharLiteral(String* input, String* output, size_t pos) {
  // Count length, not including quotes.
  size_t start = pos;
  while (pos < input->length) {
    char ch = input->value[pos];
    if (ch == '\'') {
      break;
    }
    if (ch == '\\') {
      pos++;
    }
    pos++;
  }
  size_t length = pos - start;
  EncodeLength(output, length);
  StringAppendSegment(output, &input->value[start], length);
  return pos + 1;
}

static bool CanStartToken(String* input, size_t pos) {
  char ch = input->value[pos];
  if (isspace(ch) || isalnum(ch) || ch == '_' ||
      ch == '"' || ch == '\'' || ch == '#' || ch == '(' || ch == ')') {
    return true;
  }
  if (ch == '.') {
    if (isdigit(input->value[pos+1])) {
      return true;
    }
  }
  return false;
}

static size_t AppendOtherToken(String* input, String* output, size_t pos) {
  size_t start = pos;
  while (pos < input->length) {
    if (input->value[pos] == ',') {
      break;
    }
    if (CanStartToken(input, pos)) {
      break;
    }
    pos++;
  }
  size_t length = pos - start;
  if (length == 1) {
    StringAppendChar(output, PPTOK(other_char));
    StringAppendChar(output, input->value[start]);
    return pos;
  }
  StringAppendChar(output, PPTOK(other_string));
  EncodeLength(output, length);
  StringAppendSegment(output, &input->value[start], length);
  return pos;
}

static size_t SkipSpacesAndCommentsInLine(String* line, size_t pos) {
  while (pos < line->length) {
    char ch = line->value[pos];
    
    // Check for a comment.
    if (ch == '/') {
      // '//' comment?
      if (pos < line->length &&
          line->value[pos + 1] == '/') {
        break;
      } else if (pos < line->length - 1 &&
                 line->value[pos + 1] == '*') {
        // Multi-line comment.  Read until we find the */ at the end
        pos += 2;  // Skip /*.
        do {
          do {
            ch = line->value[++pos];
          } while (pos < line->length && ch != '*');
          ch = line->value[++pos];
        } while (pos < line->length && ch != '/');
        
        // Continue to get another token.
        continue;
      }
    }
    
    if (!isspace(ch)) {
      break;
    }
    pos++;
  }
  return pos;
}

// Get next char in a multi-line comment, reading new lines and appending
// them to the current line as necessary.
static char GetNextCommentChar(Preprocessor* p, String* line, size_t* pos) {
  while (!LexEof(p->lex) && *pos >= line->length - 1) {
    SourceReadLine(p->lex->source, line);
  }
  if (LexEof(p->lex)) {
    return '\0';
  }
  (*pos)++;
  return line->value[*pos];
}

static size_t HandleSpacesAndComments(Preprocessor* p, String* line, String* output,
                                      size_t pos) {
  while (pos < line->length) {
    char ch = line->value[pos];
    
    // Check for a comment.
    if (ch == '/') {
      // '//' comment?
      if (pos < line->length &&
          line->value[pos + 1] == '/') {
        // Single line comment.  Remove rest of line.
        return line->length;
      } else if (pos < line->length - 1 &&
                 line->value[pos + 1] == '*') {
        size_t start = pos;
        // Multi-line comment.  Read until we find the */ at the end or
        // end of line.
        pos += 2;  // Skip /*.
        do {
          do {
            ch = GetNextCommentChar(p, line, &pos);
          } while (!LexEof(p->lex) && ch != '*');
           ch = GetNextCommentChar(p, line, &pos);
        } while (!LexEof(p->lex) && ch != '/');
        
        // Append comment token with comment as spelling.
        StringAppendChar(output, PPTOK(comment));
        size_t length = pos - start;
        EncodeLength(output, length);
        StringAppendSegment(output, &line->value[start], length);

        // Continue to get another token.
        continue;
      }
    }
    
    // Sequence of spaces.  Appended as single space as long as it isn't
    // at the start or end of the line.
    if (isspace(ch)) {
      bool start_or_end = pos == 0;
      while (pos < line->length) {
        ch = line->value[pos];
        if (!isspace(ch)) {
          break;
        }
        pos++;
      }
      start_or_end |= pos == line->length;
      if (!start_or_end) {
         // Append space token
         StringAppendChar(output, PPTOK(space));
      }
      continue;
    }
    break;
   }
  return pos;
}


// Handle:
// defined name
// defined (name) - spaces optional
static size_t AppendDefined(String* input, String* output, size_t pos) {
  pos = SkipSpacesAndCommentsInLine(input, pos);
  size_t start = pos;
  size_t length = 0;
  if (input->value[pos] == '(') {
    pos++;
    pos = SkipSpacesAndCommentsInLine(input, pos);
    start = pos;
    while (isalnum(input->value[pos]) || input->value[pos] == '_') {
      pos++;
    }
    length = pos - start;
    pos++;
  } else {
    while (isalnum(input->value[pos]) || input->value[pos] == '_') {
      pos++;
    }
    length = pos - start;
  }
  EncodeLength(output, length);
  StringAppendSegment(output, &input->value[start], length);
  
  return pos;
}

static void Tokenize(Preprocessor* p, String* input, String* output, size_t start,
                     bool init_output, bool assembler_mode, bool header_names_ok) {
  if (init_output) {
    StringInit(output, NULL);
  }
  for (size_t i = start; i < input->length;) {
    i = HandleSpacesAndComments(p, input, output, i);
    if (i >= input->length) {
      break;
    }
    char ch = input->value[i];
    if (isalpha(ch) || ch == '_' || (assembler_mode && (ch == '.' || ch == '@'))) {
      if ((ch == 'L' || ch == 'l') && input->value[i+1] == '"') {
        StringAppendChar(output, PPTOK(wide_literal));
        i = AppendStringLiteral(input, output, i + 2);
      } else if ((ch == 'L' || ch == 'l') && input->value[i+1] == '\'') {
        StringAppendChar(output, PPTOK(wide_char_literal));
        i = AppendCharLiteral(input, output, i + 2);
      } else {
        size_t start = i;
        String spelling = {0};
        while (isalnum(input->value[i]) || input->value[i] == '_' ||
               (assembler_mode && (input->value[i] == '.' || input->value[i] == '@'))) {
          StringAppendChar(&spelling, input->value[i]);
          i++;
        }
        if (StringEqual(&spelling, "defined")) {
          StringAppendChar(output, PPTOK(defined));
          i = AppendDefined(input, output, i);
        } else {
          StringAppendChar(output, PPTOK(identifier));
          size_t length = i - start;
          EncodeLength(output, length);
          StringAppendString(output, &spelling);
        }
        StringDestruct(&spelling);
      }
      continue;
    }
    if (ch == '<' && header_names_ok) {
      StringAppendChar(output, PPTOK(system_header));
      i++;
      size_t start = i;
      String spelling = {0};
      while (i < input->length && input->value[i] != '>') {
        StringAppendChar(&spelling, input->value[i]);
        i++;
      }
      size_t length = i - start;
      EncodeLength(output, length);
      StringAppendString(output, &spelling);
      continue;
    }
    if (ch == '"') {
      StringAppendChar(output, PPTOK(literal));
      i = AppendStringLiteral(input, output, i+1);
      continue;
    }
    if (ch == '\'') {
      StringAppendChar(output, PPTOK(char_literal));
      i = AppendCharLiteral(input, output, i+1);
      continue;
    }
    if (isdigit(ch) || (ch == '.' && isdigit(input->value[i+1]))) {
      StringAppendChar(output, PPTOK(number));
      i = AppendNumber(input, output, i);
      continue;
    }
    if (ch == '#') {
      if (input->value[i+1] == '#') {
        StringAppendChar(output, PPTOK(hashhash));
        i++;
      } else {
        StringAppendChar(output, PPTOK(hash));
      }
      i++;
      continue;
    }
    
    if (ch == '(') {
      StringAppendChar(output, PPTOK(openparen));
      i++;
      continue;
    }
    if (ch == ')') {
      StringAppendChar(output, PPTOK(closeparen));
      i++;
      continue;
    }
    if (ch == ',') {
      StringAppendChar(output, PPTOK(comma));
      i++;
      continue;
    }
    
    // Other token.  This is a sequence of arbitrary characters terminated
    // by something that can start a different token.
    i = AppendOtherToken(input, output, i);
  }
}

static size_t PrintLengthDelimitedToken(String* line, size_t pos) {
  size_t length;
  pos = DecodeLength(line, pos, &length);
  while (length-- > 0) {
    putchar(line->value[pos++]);
  }
  putchar('\n');
  return pos;
}

static void PrintTokenizedLine(String* line) {
  size_t i = 0;
  while (i < line->length) {
    switch ((PreprocessingToken)line->value[i++]) {
      case PPTOK(identifier):
        printf("identifier: ");
        i = PrintLengthDelimitedToken(line, i);
        break;
      case PPTOK(other_string):
        printf("other_string: ");
        i = PrintLengthDelimitedToken(line, i);
        break;
      case PPTOK(number):
        printf("number: ");
        i = PrintLengthDelimitedToken(line, i);
        break;
      case PPTOK(defined):
        printf("defined: ");
        i = PrintLengthDelimitedToken(line, i);
        break;
     case PPTOK(literal):
        printf("literal: ");
        i = PrintLengthDelimitedToken(line, i);
        break;
      case PPTOK(wide_literal):
        printf("wide_literal: ");
        i = PrintLengthDelimitedToken(line, i);
        break;
      case PPTOK(char_literal):
        printf("char_literal: ");
        i = PrintLengthDelimitedToken(line, i);
        break;
      case PPTOK(system_header):
        printf("system_header: ");
        i = PrintLengthDelimitedToken(line, i);
        break;
      case PPTOK(wide_char_literal):
        printf("wide_char_literal: ");
        i = PrintLengthDelimitedToken(line, i);
        break;
      case PPTOK(other_char):
        printf("other_char: %c\n", line->value[i]);
        i++;
        break;
      case PPTOK(hash):
        printf("hash\n");
        break;
      case PPTOK(hashhash):
        printf("hashhash\n");
        break;
      case PPTOK(placemarker):
        printf("placemarker\n");
        break;
      case PPTOK(openparen):
        printf("openparen\n");
        break;
      case PPTOK(closeparen):
        printf("closeparen\n");
        break;
      case PPTOK(comma):
         printf("comma\n");
         break;
      case PPTOK(space):
          printf("space\n");
          break;
      case PPTOK(comment):
         printf("comment\n");
         i = PrintLengthDelimitedToken(line, i);
         break;
      case PPTOK(end):
        printf("end\n");
        break;
    }
  }
}


// Replace the current token with the set of tokens specified.  Modifies
// the current string being iterated over.
static void ReplaceCurrentToken(TokenIterator* t, String* tokens) {
  StringReplaceString(t->input, t->curr, t->next - t->curr, tokens);
  t->next = t->curr + tokens->length;
}

static void EraseCurrentToken(TokenIterator* t) {
  StringErase(t->input, t->curr, t->next - t->curr);
  t->next = FindNextTokenIndex(t);
}

static void SkipSpaceTokens(TokenIterator* t) {
  while (IsSpaceToken(t)) {
    MoveToNextToken(t);
  }
}

static void TokenizeAndReplaceCurrentToken(TokenIterator* t,
                                           String* text, bool assembler_mode) {
  String tokens;
  Tokenize(t->p, text, &tokens, 0, true, assembler_mode, false);
  ReplaceCurrentToken(t, &tokens);
  StringDestruct(&tokens);
}

static void DetokenizeToken(String* tokens,
                            size_t index,
                            String* text) {
  String spelling = {0};
  switch ((PreprocessingToken)tokens->value[index]) {
    case PPTOK(identifier):
    case PPTOK(other_string):
    case PPTOK(number):
    case PPTOK(comment):
      AppendTokenSpelling(tokens, index, text);
      break;
    case PPTOK(defined):
      AppendTokenSpelling(tokens, index, &spelling);
      if (spelling.length == 0) {
        StringPrintf(text, "defined");
      } else {
        StringPrintf(text, "defined(%s)", spelling.value);
      }
      break;
    case PPTOK(literal):
      AppendTokenSpelling(tokens, index, &spelling);
      StringPrintf(text, "\"%s\"", spelling.value);
      break;
    case PPTOK(wide_literal):
      AppendTokenSpelling(tokens, index, &spelling);
      StringPrintf(text, "L\"%s\"", spelling.value);
      break;
    case PPTOK(char_literal):
      AppendTokenSpelling(tokens, index, &spelling);
      StringPrintf(text, "'%s'", spelling.value);
      break;
    case PPTOK(system_header):
      AppendTokenSpelling(tokens, index, &spelling);
      StringPrintf(text, "<%s>", spelling.value);
      break;
    case PPTOK(wide_char_literal):
      AppendTokenSpelling(tokens, index, &spelling);
      StringPrintf(text, "L'%s'", spelling.value);
      break;
    case PPTOK(other_char):
      StringAppendChar(text, tokens->value[index+1]);
      break;
    case PPTOK(hash):
      StringPrintf(text, "#");
      break;
    case PPTOK(hashhash):
      StringPrintf(text, "##");
      break;
    case PPTOK(placemarker):
      break;
    case PPTOK(openparen):
      StringPrintf(text, "(");
      break;
    case PPTOK(closeparen):
      StringPrintf(text, ")");
      break;
    case PPTOK(comma):
      StringPrintf(text, ",");
      break;
    case PPTOK(space):
      StringPrintf(text, " ");
      break;
    case PPTOK(end):
      break;
  }
  StringDestruct(&spelling);
}

static void Detokenize(Preprocessor* p, String* tokens, String* text) {
  TokenIterator ti;
  TokenIteratorInit(&ti, p, tokens);
  while (CurrentToken(&ti) != PPTOK(end)) {
    DetokenizeToken(ti.input, ti.curr, text);
    MoveToNextToken(&ti);
  }
}

// Returns index after comment close (or eol).
static size_t SkipToEndOfComment(String* line, size_t pos) {
  while (pos < line->length) {
    if (line->value[pos] == '*' && line->value[pos+1] == '/') {
      pos += 2;
      break;
    }
    pos++;
  }
  return pos;
}

// This skips spaces and tabs, but not newlines.  Comments are also skipped.
// If the 'newline' arg is not NULL a single space will be appended to it for
// all skipped spaces.
static size_t SkipSpacesAndComments(Preprocessor* p, size_t pos, String* line,
                                    String* newline) {
  while (pos < line->length) {
    char ch = line->value[pos];

    // Check for a comment.
    if (ch == '/') {
      // '//' comment?
      if (pos < line->length && line->value[pos + 1] == '/') {
        // Single line comment, ends at end of line.
        pos = line->length;
        if (newline != NULL) {
          StringAppendChar(newline, ' ');
        }
        return pos;
      } else if (pos < line->length - 1 && line->value[pos + 1] == '*') {
        // Multi-line comment.  Read until we find the */ at the end.
        pos += 2;  // Skip /*.
        do {
          do {
            ch = line->value[pos++];
          } while (pos < line->length && ch != '*');
          if (pos >= line->length) {
            break;
          }
          ch = line->value[pos++];
        } while (pos < line->length && ch != '/');
        return pos;
      }
    }
    if (!isblank(line->value[pos])) {
      break;
    }
    pos++;
    if (newline != NULL) {
      StringAppendChar(newline, ' ');
      newline = NULL;  // Only one space appended.
    }
  }
  return pos;
}


// Reads an identifier, appending the result to the 'result' argument
// and returning the updated position in the input line.  Initializes
// result.
static size_t ReadIdentifier(String* line, size_t pos, String* result) {
  StringInit(result, NULL);
  if (pos < line->length &&
      (isalpha(line->value[pos]) || line->value[pos] == '_')) {
    while (pos < line->length) {
      if (!isalnum(line->value[pos]) && line->value[pos] != '_') {
        break;
      }
      StringAppendChar(result, line->value[pos++]);
    }
  }
  return pos;
}

// Reads a string, returning the new position in the line.  The result
// is appended to the result argument.
static size_t ReadString(String* line, size_t pos, String* result) {
  if (pos < line->length && line->value[pos] == '"') {
    pos++;
    while (pos < line->length) {
      // Escape sequences in the string don't terminate it.
      if (line->value[pos] == '\\') {
        StringAppendChar(result, line->value[pos++]);
        if (pos == line->length) {
          break;
        }
        StringAppendChar(result, line->value[pos++]);
        continue;
      }

      if (line->value[pos] == '"') {
        pos++;
        break;
      }
      StringAppendChar(result, line->value[pos++]);
    }
  }
  return pos;
}

// Checks if the given macro has the same composition as the arguments.
static bool MacroEqual(Macro* macro, bool is_function_like, Vector* args,
                       bool varargs, String* replacement_text) {
  if (macro->undefined) {
    return true;
  }
  if (macro->is_function_like != is_function_like ||
      macro->varargs != varargs) {
    return false;
  }

  // The replacement text has already been tokenized.
  if (!StringEqualString(&macro->replacement_text, replacement_text)) {
    return false;
  }
  if (macro->args.length != args->length) {
    return false;
  }
  for (size_t i = 0; i < args->length; i++) {
    String* old = (String*)macro->args.value.p[i];
    String* new = (String*)args->value.p[i];
    if (!StringEqualString(old, new)) {
      return false;
    }
  }
  return true;
}


// Given a path (vector of strings) and a filename, search the path
// for the file and open it if found.  If it is found, sets the
// filename string to the pathname.  Returns NULL if the file couldn't
// be found or couldn't be opened due to permissions problems.
static FILE* FindFileInPath(Vector* path, String* filename, size_t* start) {
  for (size_t i = *start; i < path->length; i++) {
    String pathname = {0};
    StringPrintf(&pathname, "%s/%s", ((String*)path->value.p[i])->value,
                 filename->value);
    FILE* fp = fopen(pathname.value, "r");
    if (fp != NULL) {
      StringSetString(filename, &pathname);
      StringDestruct(&pathname);
      *start = i;
      return fp;
    }
    StringDestruct(&pathname);
  }
  return NULL;
}

// Type for function to handle preprocessor commands.
typedef void (*PreprocessorCommand)(Preprocessor* p, String* line, size_t pos);

static size_t ReadMacroFormalArguments(Preprocessor* p,
                                       String* line,
                                       size_t pos,
                                       String* macro_name,
                                       Vector* args,
                                       bool* varargs) {
  pos++;  // Skip open paren.
  pos = SkipSpacesAndComments(p, pos, line, NULL);
  
  while (line->value[pos] != ')') {
    String arg;
    
    pos = SkipSpacesAndComments(p, pos, line, NULL);
    
    // Check for ... and if so we terminated the arguments and mark
    // this macro has having a variable number of args.
    if (line->value[pos] == '.' && line->value[pos + 1] == '.' &&
        line->value[pos + 2] == '.') {
      *varargs = true;
      pos += 3;
      break;
    }
    
    // Read the argument name.
    pos = ReadIdentifier(line, pos, &arg);
    
    // Make sure it's not already defined in this macro.
    for (size_t i = 0; i < args->length; i++) {
      if (StringEqualString((String*)args->value.p[i], &arg)) {
        PreprocessorError(p, "Duplicate macro argument %s", arg.value);
        return pos;
      }
    }
    
    // Add argument to the set of known arguments.
    VectorAppend(args, NewString(arg.value));
    
    // Check for more arguments.
    pos = SkipSpacesAndComments(p, pos, line, NULL);
    if (line->value[pos] != ',') {
      break;
    }
    pos++;  // Skip comma.
  }
  if (line->value[pos] != ')') {
    PreprocessorError(p, "Missing ')' for function-like macro %s",
                      macro_name->value);
    return pos;
  }
  pos++;  // Skip close paren.
  return pos;
}

static void CheckHashHash(Preprocessor* p, String* rep) {
  TokenIterator ti;
  TokenIteratorInit(&ti, p, rep);
  if (CurrentToken(&ti) == PPTOK(hashhash)) {
    PreprocessorError(p, "## is not allowed at start of replacement text");
  }

  // Move to last token
  while (CurrentToken(&ti) != PPTOK(end)) {
    MoveToNextToken(&ti);
  }
  
  // Ending in ## not allowed.
  if (PrevToken(&ti) == PPTOK(hashhash)) {
    PreprocessorError(p, "## is not allowed at end of replacement text");
  }
}

static void Define(Preprocessor* p, String* line, size_t pos) {
  if (!p->is_compiled_in) {
    // Ignore this if it is #ifed out.
    return;
  }
  
  // Collect the macro name (the identifier after #define) into a string
  String macro_name;

  size_t name_start = pos;
  pos = ReadIdentifier(line, pos, &macro_name);
  size_t name_end = pos;
  String replacement_text;  // Value of macro.
  bool function_like_macro = false;
  bool varargs = false;

  // Arguments for function-like macro."
  Vector args = {0};

  // No space allowed before open paren for function-like macro.
  if (line->value[pos] == '(') {
    // Function like macro, collect the arguments.
    function_like_macro = true;
    pos = ReadMacroFormalArguments(p, line, pos, &macro_name, &args, &varargs);
  }

  // Skip any spaces or comments before the replacement text.
  pos = SkipSpacesAndComments(p, pos, line, NULL);

  // Read the replacement text.  This just reads up the the end of line, which
  // has already been processed by appending all lines ending in \.  Then we
  // need to tokenize the string.
  String raw_replacement_text;
  StringInit(&raw_replacement_text, &line->value[pos]);
  Tokenize(p, &raw_replacement_text, &replacement_text, 0, true, p->lex->assembler_mode, false);
  
  // Check that ## rules are not violated.
  CheckHashHash(p, &replacement_text);

  // Check if the macro has already been defined and if so, make sure this
  // definition is the same as the old old.
  Macro* macro = HashTableSearch(&p->macros, macro_name.value);
  if (macro == NULL) {
    macro = NewMacro(macro_name.value, function_like_macro, varargs, &args,
                     &replacement_text,
                     NewSourceLocation(p->lex->source, p->lex->source->lineno,
                                       name_start, name_end));
    bool ok = HashTableInsert(&p->macros, macro);
    assert(ok);
  } else {
    if (!MacroEqual(macro, function_like_macro, &args, varargs,
                    &replacement_text)) {
      const char* filename;
      int lineno;
      int start, end;
      DecodeSourceLocation(macro->location, &filename, &lineno, &start, &end);
      PreprocessorWarning(p, "macro-redef",
                          "Macro %s redefined; previously"
                          " defined at %s:%d",
                          macro_name.value, filename, lineno);
    }
    macro->undefined = false;
    // If macro was undefined it will have its original value.  Give it the
    // new one.
    StringSet(&macro->replacement_text, replacement_text.value);
  }

  // We're done with these strings, clean up.
  StringDestruct(&macro_name);
  StringDestruct(&replacement_text);
}

// Undefine a macro if it has been defined.  No effect if it is not known.
static void Undef(Preprocessor* p, String* line, size_t pos) {
  if (!p->is_compiled_in) {
    return;
  }
  String macro_name;
  pos = ReadIdentifier(line, pos, &macro_name);
  Macro* macro = HashTableSearch(&p->macros, macro_name.value);
  if (macro != NULL) {
    macro->undefined = true;
  }

  pos = SkipSpacesAndComments(p, pos, line, NULL);
  if (pos < line->length) {
    PreprocessorWarning(p, "extra-tokens", "Extra tokens after #undef");
  }
  StringDestruct(&macro_name);
}

// #include processing.

bool PreprocessorParseIncludeFilename(Preprocessor* p, String* line,
                                      size_t* in_out_pos, String* filename,
                                      bool* system_include) {
  size_t end;
  size_t pos = *in_out_pos;
  if (line->value[pos] == '<') {
    // #include <...>.
    *system_include = true;
    pos++;
    end = pos;
    while (end < line->length) {
      if (line->value[end] == '>') {
        break;
      }
      end++;
    }
    if (line->value[end] != '>') {
      PreprocessorError(p, "Missing > for #include filename");
      return false;
    }
  } else if (line->value[pos] == '"') {
    // #include "...".
    pos++;
    end = pos;
    while (end < line->length) {
      if (line->value[end] == '"') {
        break;
      }
      end++;
    }
    if (line->value[end] != '"') {
      PreprocessorError(p, "Missing closing \" for #include filename");
      return false;
    }
  } else {
    PreprocessorError(p, "Bad #include filename; need <...> or \"...\"");
    return false;
  }

  if (end == pos) {
    PreprocessorError(p, "Empty #include filename");
    return false;
  }

  // Now, 'pos' is the start of the filename and 'end' is the character
  // after the end of it.
  size_t filename_length = end - pos;  // Not including \0.

  // Filename of file to find
  StringInitFromSegment(filename, &line->value[pos], filename_length);
  *in_out_pos = end + 1;
  return true;
}

static void PrintSearchDetails(Preprocessor* p, const char* file, bool system_include) {
  ReportNote(NULL, 0, "Looked in the following locations:");
  if (!system_include) {
    for (size_t i = 0; i < p->user_include_paths.length; i++) {
      String pathname = {0};
      StringPrintf(&pathname, "%s/%s",
                   ((String*)p->user_include_paths.value.p[i])->value,
                   file);
      ReportNote(NULL, 0, "  %s", pathname.value);
    }
  }
  for (size_t i = 0; i < p->system_include_paths.length; i++) {
    String pathname = {0};
    StringPrintf(&pathname, "%s/%s",
                 ((String*)p->system_include_paths.value.p[i])->value,
                 file);
    ReportNote(NULL, 0, "  %s", pathname.value);
  }

}

static void DoInclude(Preprocessor* p, String* line, size_t pos,
                      size_t start_index) {
  if (!p->is_compiled_in) {
    return;
  }
  
  String tokenized_line;
  // Tokenize, allowing header names.
  Tokenize(p, line, &tokenized_line, pos,  true, false, true);
  ReplaceMacrosInTokenizedLine(p, &tokenized_line, true);

  TokenIterator ti;
  TokenIteratorInit(&ti, p, &tokenized_line);

  String filename;
  bool system_include = false;
  if (CurrentToken(&ti) == PPTOK(literal)) {
    GetCurrentTokenSpelling(&ti, &filename);
  } else if (CurrentToken(&ti) == PPTOK(system_header)) {
    GetCurrentTokenSpelling(&ti, &filename);
    system_include = true;
  } else {
    PreprocessorError(p, "#include needs \"filename\" or <filename>");
    StringDestruct(&tokenized_line);
    return;
  }
  
  FILE* fp = NULL;
  size_t path_index = start_index;
  if (!system_include) {
    // Not a system include (#include "...") so search user include
    // paths.
    fp = FindFileInPath(&p->user_include_paths, &filename, &path_index);
  }
  if (fp == NULL) {
    // Not found in user include paths or this was a system include
    // (#include <...>).  Search system include paths.
    fp = FindFileInPath(&p->system_include_paths, &filename, &path_index);
  }
  if (fp == NULL) {
    if (start_index != 0) {
      // For #include_next we don't abort if there is no next include file.
      return;
    }
    // There is no point in continuing to compile if we can't find
    // an include file.  This will generated many, many errors for undefined
    // types and functions.
    PreprocessorError(p, "Fatal error: cannot open include file \"%s\"",
                      filename.value);
    PrintSearchDetails(p, filename.value, system_include);
    exit(1);
  }

  // We have found the include file, process it as if it was inline
  // in the current source.

  // Allocate a new source provider from the include file and push
  // it as the current source in the Lex.
  Source* include_source = NewSourceFromFile(filename.value, fp);
  include_source->prev = p->lex->source;
  if (compiler->print_preprocessor) {
    printf("Including file %s\n", filename.value);
  }
  
  // Save current path index and set new current.
  p->lex->source->path_index = compiler->current_include_path_index;
  compiler->current_include_path_index = path_index;

  p->lex->source = include_source;
  
  // Done with this string since the Source will create a new one.
  StringDestruct(&filename);
  
  StringDestruct(&tokenized_line);

  // We have pushed a new Source provider as the current one
  // for Lex so returning from here will cause
  // the Lex to read the first line in this new source.  The
  // include_source object will be deleted by the Lex when it
  // reaches the end of file.  It will then pop the source to the
  // previous one and continue reading lines.
}

static void Include(Preprocessor* p, String* line, size_t pos) {
  DoInclude(p, line, pos, 0);
}

static void IncludeNext(Preprocessor* p, String* line, size_t pos) {
  DoInclude(p, line, pos, p->lex->source->path_index + 1);
}

// Update the is_compiled_in state based on the if_stack.
// An earlier compiled-out block dominates all lower level
// blocks.
static void UpdateState(Preprocessor* p) {
  for (size_t i = 0; i < p->if_stack.length; i++) {
    if (p->if_stack.value.p[i] == NULL || p->if_stack.value.p[i] == (void*)(-1LL)) {
      p->is_compiled_in = false;
      return;
    }
  }
  p->is_compiled_in = true;
}

static void* EvaluateExpression(Preprocessor* p, String* expr_string) {
  static int true_value;
  
  // Allow the lexical analyzer to see the controlling expression.
  p->is_compiled_in = true;

  int lineno = p->lex->source->lineno;
  Lex lex;
  Lex* prev_lex = p->lex;
  LexInitFromString(&lex, p->lex->source->filename.value, expr_string, p);
  lex.source->lineno = lineno - 1;      // Will be incremented on first read.
  lex.preprocessor_mode = true;
  lex.assembler_mode = p->lex->assembler_mode;
  LexNextToken(&lex);
  
  Syntax syntax;
  SyntaxInit(&syntax, &lex);
  bool prev_abort_on_error = abort_on_error;
  abort_on_error = true;
  void* controlling_value = NULL;
  if (setjmp(error_abort_state) == 0) {
    ASTNode* expr = SyntaxParseExpression(&syntax, 0);
    if (expr != NULL) {
      expr = AnalyzeExpression(expr);
      int64_t value;
      if (EvaluateIntegerExpression(expr, &value)) {
        controlling_value = value == 0 ? NULL : &true_value;
      }
    }
  }
  abort_on_error = prev_abort_on_error;
  SyntaxDestruct(&syntax);
  lex.source = NULL;  // Prevent Lex from freeing source.
  LexDestruct(&lex);
  p->lex = prev_lex;

  return controlling_value;
}

// #if processing.
static void If(Preprocessor* p, String* line, size_t pos) {
  // #if processing needs to evaluate the expression only if the current
  // state of conditional processing is true.
  if (!p->is_compiled_in) {
    VectorPush(&p->if_stack, NULL);
    return;
  }

  String controlling_expr;
  StringInit(&controlling_expr, &line->value[pos]);
  StringAppend(&controlling_expr, "\n\n");

  void* controlling_value = EvaluateExpression(p, &controlling_expr);
  VectorPush(&p->if_stack, controlling_value);
  StringDestruct(&controlling_expr);

  // Update is is_compiled_in state.
  UpdateState(p);
}

static void Endif(Preprocessor* p, String* line, size_t pos) {
  if (p->if_stack.length == 0) {
    PreprocessorError(p, "Extraneous #endif");
    return;
  }
  VectorPop(&p->if_stack);
  pos = SkipSpacesAndComments(p, pos, line, NULL);
  if (pos < line->length) {
    PreprocessorWarning(p, "extra-tokens", "Extra tokens after #endif");
  }
  UpdateState(p);
}

static void Ifndef(Preprocessor* p, String* line, size_t pos) {
  static int macro_not_defined;

  String macro_name;
  pos = ReadIdentifier(line, pos, &macro_name);
  if (macro_name.length == 0) {
    PreprocessorError(p, "Expected macro name after #ifndef");
    // An empty macro name will always be missing.
  }
 
  Macro* macro = HashTableSearch(&p->macros, macro_name.value);
  VectorPush(&p->if_stack, macro == NULL ? &macro_not_defined : NULL);
  StringDestruct(&macro_name);

  pos = SkipSpacesAndComments(p, pos, line, NULL);
  if (pos < line->length) {
    PreprocessorWarning(p, "extra-tokens", "Extra tokens after #ifndef");
  }
  UpdateState(p);
}

static void Ifdef(Preprocessor* p, String* line, size_t pos) {
  String macro_name;
  pos = ReadIdentifier(line, pos, &macro_name);
  if (macro_name.length == 0) {
    PreprocessorError(p, "Expected macro name after #ifdef");
    // An empty macro name will always be missing from the table
    // of macros so it won't be found.
  }
  Macro* macro = HashTableSearch(&p->macros, macro_name.value);
  // #undef marks the macro as being undefined
  if (macro != NULL && macro->undefined) {
    macro = NULL;
  }
  VectorPush(&p->if_stack, macro);
  StringDestruct(&macro_name);

  pos = SkipSpacesAndComments(p, pos, line, NULL);
  if (pos < line->length) {
    PreprocessorWarning(p, "extra-tokens", "Extra tokens after #ifdef");
  }
  UpdateState(p);
}

static void Elif(Preprocessor* p, String* line, size_t pos) {
  if (p->if_stack.length == 0) {
    PreprocessorError(p, "#elif outside #if..#endif");
    return;
  }

  // #elif needs to evaluate its expression only if the current #if
  // block hasn't ever been compiled in.  This is determined by looking
  // at the top of the if_stack.
  // There are 3 cases:
  // 1. NULL: the #if block has never been compiled in
  // 2. -1: the #if block has been compiled in and all subsequent #elif and
  //        #else are compiled out
  // 3. other value: the most recent #if or #elif was compiled in and we
  //                 now compile out all subsequent blocks.
  void* top_value = p->if_stack.value.p[p->if_stack.length - 1];
  if (top_value == NULL) {
    // Need to evaulate expression.
  } else if (top_value == (void*)(-1LL)) {
    // The #if..#elif... sequence was compiled in at some point.  Leave this
    // as is on the stack.
    return;
  } else {
    // Replace the top of the if_stack with -1 to tell all subsequent #elif
    // and #else that this #if block has been compiled in.
    p->if_stack.value.p[p->if_stack.length - 1] = (void*)(-1LL);
    UpdateState(p);
    return;
  }

  String controlling_expr;
  StringInit(&controlling_expr, &line->value[pos]);
  StringAppend(&controlling_expr, "\n\n");

  void* controlling_value = EvaluateExpression(p, &controlling_expr);

  // Replace the top of the if_stack with the new controlling value.
  p->if_stack.value.p[p->if_stack.length - 1] = controlling_value;

  StringDestruct(&controlling_expr);

  UpdateState(p);
}

static void Else(Preprocessor* p, String* line, size_t pos) {
  static int inverted_value;

  if (p->if_stack.length == 0) {
    PreprocessorError(p, "#else outside #if..#endif");
    return;
  }
  // #else sets the top of the if_stack to true only if it current
  // has the value NULL (meaning that no #if or #elif block was compiled in)
  void* top_value = p->if_stack.value.p[p->if_stack.length - 1];
  p->if_stack.value.p[p->if_stack.length - 1] =
      top_value == NULL ? &inverted_value : NULL;

  pos = SkipSpacesAndComments(p, pos, line, NULL);
  if (pos < line->length) {
    PreprocessorWarning(p, "extra-tokens", "Extra tokens after #else");
  }
  UpdateState(p);
}

static void Line(Preprocessor* p, String* line, size_t pos) {
  if (!p->is_compiled_in) {
    return;
  }
  String tokenized_line;
  Tokenize(p, line, &tokenized_line, pos, true, false, false);
  ReplaceMacrosInTokenizedLine(p, &tokenized_line, true);
  
  TokenIterator ti;
  TokenIteratorInit(&ti, p, &tokenized_line);
  
  int lineno = -1;
  bool error = false;
  
  // Need a line number as a simple digit sequence.
  if (CurrentToken(&ti) == PPTOK(number)) {
    String spelling;
    GetCurrentTokenSpelling(&ti, &spelling);
    lineno = 0;
    for (size_t i = 0; i < spelling.length; i++) {
      if (!isdigit(spelling.value[i])) {
        PreprocessorError(p, "#line neeed a simple digit sequence");
        error = true;
        break;
      }
      lineno = lineno * 10 + spelling.value[i] - '0';
    }
    MoveToNextToken(&ti);
    StringDestruct(&spelling);
  }
  if (!error && lineno < 0) {
    PreprocessorError(p, "#line directive requires a positive integer line number");
    error = true;
  }
  String filename;
  bool filename_set = false;
  if (!error && CurrentToken(&ti) == PPTOK(literal)) {
    GetCurrentTokenSpelling(&ti, &filename);
    filename_set = true;
  }
  
  if (!error) {
    // Set the line number in the source.
    p->lex->source->lineno = lineno - 1;     // Next line will have this number.

    if (filename_set) {
       StringSetString(&p->lex->source->filename, &filename);
       StringDestruct(&filename);
     } else {
       StringSetString(&p->lex->source->filename, &p->lex->source->original);
     }
  }
  StringDestruct(&tokenized_line);
}

static void Error(Preprocessor* p, String* line, size_t pos) {
  if (!p->is_compiled_in) {
    return;
  }
  String error = {0};
  StringAppend(&error, &line->value[pos]);  // Rest of line
  PreprocessorError(p, error.value);
  StringDestruct(&error);
}

static void Warning(Preprocessor* p, String* line, size_t pos) {
  if (!p->is_compiled_in) {
    return;
  }
  String warning = {0};
  StringAppend(&warning, &line->value[pos]);  // Rest of line
  PreprocessorWarning(p, "preprocessor", warning.value);
  StringDestruct(&warning);
}

static bool MatchIdentifierToken(TokenIterator* ti, const char* spelling) {
  SkipSpaceTokens(ti);
  if (CurrentToken(ti) == PPTOK(identifier)) {
    String v;
    GetCurrentTokenSpelling(ti, &v);
    bool match = StringEqual(&v, spelling);
    StringDestruct(&v);
    if (match) {
      MoveToNextToken(ti);
    }
    return match;
  }
  return false;
}

static bool GetIdentifierToken(TokenIterator* ti, String* spelling) {
  SkipSpaceTokens(ti);
  if (CurrentToken(ti) == PPTOK(identifier)) {
    GetCurrentTokenSpelling(ti, spelling);
    MoveToNextToken(ti);
    return true;
  }
  return false;
}

static void Pragma(Preprocessor* p, String* line, size_t pos) {
  if (!p->is_compiled_in) {
    return;
  }
  String tokenized_tail;
  Tokenize(p, line, &tokenized_tail, pos, true, false, false);
  
  TokenIterator ti;
  TokenIteratorInit(&ti,p,  &tokenized_tail);
  while (CurrentToken(&ti) != PPTOK(end)) {
    if (MatchIdentifierToken(&ti, "warning")) {
      String warning;
      if (GetIdentifierToken(&ti, &warning)) {
        bool on = MatchIdentifierToken(&ti, "on");
        if (on) {
          EnableWarning(warning.value);
        } else {
          DisableWarning(warning.value);
        }
      }
      StringDestruct(&warning);
    } else {
      // Unknown pragma, ignore rest of line.
      break;
    }
  }
  StringDestruct(&tokenized_tail);
}

// Mapping of command (directive) name and function to handle it.
static struct {
  const char* command_name;
  PreprocessorCommand command;
} preprocessor_commands[] = {
    {"define", Define},   {"include", Include},
    {"if", If},           {"ifdef", Ifdef},
    {"ifndef", Ifndef},   {"endif", Endif},
    {"elif", Elif},       {"else", Else},
    {"line", Line},       {"error", Error},
    {"pragma", Pragma},   {"undef", Undef},
    {"warning", Warning}, {"include_next", IncludeNext},
    {NULL, NULL},
};

bool PreprocessorParseDirective(Preprocessor* p, String* line) {
  size_t pos = 0;
  if (p->lex->in_comment) {
    pos = SkipToEndOfComment(line, pos);
  }
  pos = SkipSpacesAndComments(p, pos, line, NULL);
  if (pos >= line->length || line->value[pos] != '#') {
    return false;
  }
  pos++;
  pos = SkipSpacesAndComments(p, pos, line, NULL);

  String command_name = {0};

  // Read directive (command) name.  This must start with a letter.
  if (pos < line->length && isalpha(line->value[pos])) {
    while (pos < line->length && !isspace(line->value[pos])) {
      StringAppendChar(&command_name, line->value[pos]);
      pos++;
    }
  }

  // pos is the index of the character after the last in the command name.
  pos = SkipSpacesAndComments(p, pos, line, NULL);

  PreprocessorCommand command = NULL;
  for (size_t i = 0; preprocessor_commands[i].command_name != NULL; i++) {
    if (StringEqual(&command_name, preprocessor_commands[i].command_name)) {
      command = preprocessor_commands[i].command;
      break;
    }
  }
  if (command == NULL) {
    // Unknown preprocessor command, we have an unknown preprocessor directive.
    // Only give this error if the line is compiled in.
    if (p->is_compiled_in) {
      PreprocessorError(p, "Invalid preprocessor directive %s",
                          command_name.value);
    }
    StringDestruct(&command_name);
    return true;
  }

  // Run the command parser.
  command(p, line, pos);
  
  StringDestruct(&command_name);
  return true;
}

bool PreprocessorHasInclude(Preprocessor* p, String* filename,
                            bool system_include) {
  FILE* fp = NULL;
  size_t path_index = 0;
  if (!system_include) {
    // Not a system include (#include "...") so search user include
    // paths.
    fp = FindFileInPath(&p->user_include_paths, filename, &path_index);
  }
  if (fp == NULL) {
    // Not found in user include paths or this was a system include
    // (#include <...>).  Search system include paths.
    fp = FindFileInPath(&p->system_include_paths, filename, &path_index);
  }
  if (fp == NULL) {
    return false;
  }
  fclose(fp);
  return true;
}

bool PreprocessorHasIncludeNext(Preprocessor* p, String* filename,
                                bool system_include) {
  FILE* fp = NULL;
  size_t path_index = compiler->current_include_path_index + 1;
  if (!system_include) {
    // Not a system include (#include "...") so search user include
    // paths.
    fp = FindFileInPath(&p->user_include_paths, filename, &path_index);
  }
  if (fp == NULL) {
    // Not found in user include paths or this was a system include
    // (#include <...>).  Search system include paths.
    fp = FindFileInPath(&p->system_include_paths, filename, &path_index);
  }
  if (fp == NULL) {
    return false;
  }
  fclose(fp);
  return true;
}

//
// Macro replacement.
//

Macro* PreprocessorFindMacro(Preprocessor* p, String* macro_name) {
  return HashTableSearch(&p->macros, macro_name->value);
}

bool PreprocessorLineIsCompiledIn(Preprocessor* p) { return p->is_compiled_in; }

static String* FindMacroArg(Map* args, String* name) {
  MapKeyType key;
  key.p = name;
  return MapFind(args, key);
}

// Collect the actual arguments from the source code.
static void CollectActualArguments(Preprocessor* p,
                                     TokenIterator* ti,
                                     String* tokens,
                                     Macro* macro,
                                     Map* args,
                                     String* va_args,
                                     bool whole_input) {
  int formal_index = 0;
  int actual_index = 0;
  bool too_many_args = false;
  bool separate_va_arg = false;
  bool end = false;
    
  while (!end && CurrentToken(ti) != PPTOK(closeparen)) {
    String* actual = NewString(NULL);
    int num_nested_brackets = 1;
    while (!end) {
      PreprocessingToken tok = CurrentToken(ti);
      if (tok == PPTOK(end)) {
        if (whole_input) {
          // No more input available.
          end = true;
          break;
        }
        // In a function-like macro invocation the language allows newline
        // characters to be treated as spaces.  This means we need to read
        // another line when we encounter the end of line.
        String newline = {0};
        SourceReadLine(p->lex->source, &newline);
        // Append newly read chars to the current tokens.
        Tokenize(p, &newline, tokens, 0, false, p->lex->assembler_mode, false);
        ti->next = FindNextTokenIndex(ti);
        StringDestruct(&newline);
        if (CurrentToken(ti) == PPTOK(end)) {
          end = true;
        }
        continue;
      }
      if (tok == PPTOK(openparen)) {
        num_nested_brackets++;
      } else if (tok == PPTOK(closeparen)) {
        num_nested_brackets--;
        if (num_nested_brackets == 0) {
          // Close paren at end of actual.
          break;
        }
      }
      if (tok == PPTOK(comma)) {
        MoveToNextToken(ti);
        SkipSpaceTokens(ti);
        if (num_nested_brackets == 1) {
          // Comma outside of nested brackets, end of actual.
          break;
        }
      }
      
      AppendCurrentToken(ti, actual);
      MoveToNextToken(ti);
    }
    
    if (actual_index >= macro->args.length) {
      if (macro->varargs) {
        // Append actual arg to the va_args string, separated by comma from
        // the previous one.
        if (separate_va_arg) {
          StringAppendChar(va_args, PPTOK(comma));
        }
        StringAppend(va_args, actual->value);
        separate_va_arg = true;
      } else {
        // Note the fact that we have too many arguments for an non-varargs
        // macro.
        too_many_args = true;
      }
      StringDelete(actual);
    } else {
      MapKeyValue arg;
      arg.key.p = macro->args.value.p[formal_index];
      arg.value.p = actual;
      MapInsert(args, arg);
      formal_index++;
    }
    actual_index++;
  }
  // If the last actual value is empty we will have terminated the loop
  // early (on the close paren).
  if (actual_index < macro->args.length) {
    MapKeyValue arg;
    arg.key.p = macro->args.value.p[formal_index];
    arg.value.p = NewString("");
    MapInsert(args, arg);
    actual_index++;
    formal_index++;
  }
  if (CurrentToken(ti) != PPTOK(closeparen)) {
    PreprocessorError(p, "Missing ')' for function like macro arguments");
  } else {
    MoveToNextToken(ti);
  }
  
  if (too_many_args) {
    PreprocessorError(
                      p, "Too many actual arguments for macro %s; expected %zd, got %d",
                      macro->name.value, macro->args.length, actual_index);
  }
  if (actual_index < macro->args.length) {
    PreprocessorError(
                      p, "Insufficient actual arguments for macro %s; expected %zd, got %d",
                      macro->name.value, macro->args.length, actual_index);
  }
}

// The current token is ##.  Join the previous and next
// tokens together.
static void Paste(TokenIterator* ti) {
  PreprocessingToken prev = PrevToken(ti);
  PreprocessingToken next = NextToken(ti);
  // Placemarker tokens require special handling.
  if (prev == PPTOK(placemarker)) {
    if (next == PPTOK(placemarker)) {
      MoveToNextToken(ti);
      EraseCurrentToken(ti);
    } else {
      // Erase placemarker token.
      EraseCurrentToken(ti);
    }
    return;
  }
  if (next == PPTOK(placemarker)) {
    MoveToNextToken(ti);
    EraseCurrentToken(ti);
    return;
  }
  
  // Not a placemarker in previous or next.
  size_t prev_index = ti->prev;
  MoveToNextToken(ti);
  SkipSpaceTokens(ti);
  size_t next_index = ti->curr;
  size_t end_index = ti->next;
  String pasted = {0};
  DetokenizeToken(ti->input, prev_index, &pasted);
  DetokenizeToken(ti->input, next_index, &pasted);
  
  // The result is a single token containing the pasted tokens.
  String tokenized_paste = {0};
  StringAppendChar(&tokenized_paste, PPTOK(other_string));
  EncodeLength(&tokenized_paste, pasted.length);
  StringAppendString(&tokenized_paste, &pasted);
  StringDestruct(&pasted);
  StringReplaceString(ti->input,
                      prev_index,
                      end_index - prev_index,
                      &tokenized_paste);
  StringDestruct(&tokenized_paste);
  
  // The current token is now the original previous.
  ti->curr = prev_index;
  ti->next = FindNextTokenIndex(ti);
}



static void PasteTokens(Preprocessor* p, String* tokens) {
  TokenIterator ti;
  TokenIteratorInit(&ti, p, tokens);
  while (CurrentToken(&ti) != PPTOK(end)) {
    if (CurrentToken(&ti) == PPTOK(hashhash)) {
      Paste(&ti);
    }
    MoveToNextToken(&ti);
  }
}

static void RemovePlacemarkers(Preprocessor* p, String* tokens) {
  TokenIterator ti;
  TokenIteratorInit(&ti, p, tokens);
  while (CurrentToken(&ti) != PPTOK(end)) {
    if (CurrentToken(&ti) == PPTOK(placemarker)) {
      EraseCurrentToken(&ti);
      continue;
    }
    MoveToNextToken(&ti);
  }
}

static void ReplaceFunctionLikeMacroText(TokenIterator* ti,
                                     size_t macro_name_token_index,
                                     String* tokens) {
  StringReplaceString(ti->input,
                      macro_name_token_index,
                      ti->curr - macro_name_token_index,
                      tokens);
  ti->next = macro_name_token_index + tokens->length;
}

// Process any macros and arguments in the replacement text. This also
// handles the # operator.
static void ProcessFunctionLikeReplacementText(Preprocessor* p,
                                   Macro* macro,
                                   TokenIterator* ti,
                                   size_t macro_name_token_index,
                                   Map* args,
                                   String* va_args) {
  // Copy replacement text tokens.
  String rep;
  StringInit(&rep, macro->replacement_text.value);
  
  // Iterator for passing over replacement text (tokens).
  TokenIterator rep_ti;
  TokenIteratorInit(&rep_ti, p, &rep);
  
  // Replace all arguments in replacement text by their actual value.
  while (CurrentToken(&rep_ti) != PPTOK(end)) {
    switch (CurrentToken(&rep_ti)) {
      case PPTOK(identifier): {
        String possible_arg;
        GetCurrentTokenSpelling(&rep_ti, &possible_arg);
        if (StringEqual(&possible_arg, "__VA_ARGS__")) {
          if (macro->varargs) {
            ReplaceCurrentToken(&rep_ti, va_args);
          } else {
            PreprocessorError(p, "Use of __VA_ARGS__ outside of varargs macro");
          }
        } else {
          String* actual = FindMacroArg(args, &possible_arg);
          if (actual != NULL) {
            // 6.10.3.1 Argument substitution
            // 1 After the arguments for the invocation of a function-like macro
            // have been identified, argument substitution takes place. A parameter
            // in the replacement list, unless preceded by a # or ## preprocessing
            // token or followed by a ## preprocessing token (see below), is
            // replaced by the corresponding argument after all macros contained
            // therein have been expanded. Before being substituted, each
            // argument’s preprocessing tokens are completely macro replaced
            // as if they formed the rest of the preprocessing file;
            // no other preprocessing tokens are available.
            if (actual->length == 0) {
              String p = {0};
              StringAppendChar(&p, PPTOK(placemarker));
              ReplaceCurrentToken(&rep_ti, &p);
              StringDestruct(&p);
            } else {
              if (PrevToken(&rep_ti) == PPTOK(hashhash) ||
                  NextToken(&rep_ti) == PPTOK(hashhash)) {
                // Preceded or followed by ##, the macros in the argument
                // are not expanded.
                ReplaceCurrentToken(&rep_ti, actual);
              } else {
                // Expand any macros in the actual value.
                String expanded_actual;
                StringInit(&expanded_actual, actual->value);
                ReplaceMacrosInTokenizedLine(p, &expanded_actual, true);
                ReplaceCurrentToken(&rep_ti, &expanded_actual);
                StringDestruct(&expanded_actual);
              }
            }
          }
        }
        StringDestruct(&possible_arg);
        break;
      }
      case PPTOK(hash): {
        EraseCurrentToken(&rep_ti);      // Remove # token.
        SkipSpaceTokens(&rep_ti);
        String possible_arg;
        GetCurrentTokenSpelling(&rep_ti, &possible_arg);

        String* actual = FindMacroArg(args, &possible_arg);
        if (actual == NULL) {
          // Not an argument, error.
          PreprocessorError(p, "# is not followed by a macro argument name");
        } else {
          // Detokenize the actual value.
          String detokenized = {0};
          Detokenize(p, actual, &detokenized);
          
          String literal = {0};
          // Append a PPTOK(literal) to the temp value.
          StringAppendChar(&literal, PPTOK(literal));
          EncodeLength(&literal, detokenized.length);
          StringEscape(&detokenized, &literal);
          ReplaceCurrentToken(&rep_ti, &literal);
          StringDestruct(&detokenized);
          StringDestruct(&literal);
        }
        break;
      }
        
      default:
        break;
   }
    MoveToNextToken(&rep_ti);
  }
  
  // Paste tokens either side of ## in replacement text.
  PasteTokens(p, &rep);
  
  // Remove placemarker tokens.
  RemovePlacemarkers(p, &rep);
  
  // Rescan replacement text for more macros to replace.
  ReplaceMacrosInTokenizedLine(p, &rep, true);
  
  // Replace macro name and args with replacement text.
  ReplaceFunctionLikeMacroText(ti, macro_name_token_index, &rep);
  StringDestruct(&rep);
}

static void DeleteActualArg(MapKeyValue* kv, void* data) {
  StringDelete(kv->value.p);
}

// We have a function-like macro invokation.  It will be followed by
// a set of actual arguments.
static void ReplaceFunctionLikeMacro(Preprocessor* p, Macro* macro,
                                     TokenIterator* ti,
                                     String* tokens,
                                      bool whole_input) {
  // Save current token (the macro name).  We need to replace the
  // function-like macro name and its args with its replacement text.
  size_t current_token_index = ti->curr;
  MoveToNextToken(ti);
  SkipSpaceTokens(ti);      // Space between macro and ( is OK.
  
  if (CurrentToken(ti) != PPTOK(openparen)) {
    // Missing open paren means that we don't replace the macro.
    return;
  }
  MoveToNextToken(ti);
  
  // Collect the actual arguments.
  // Mapping of formal to actual for each argument.
  Map args;
  MapInitForStringKeys(&args);
  String va_args = {0};
  CollectActualArguments(p, ti, tokens, macro, &args, &va_args, whole_input);

  // Now we process the macro replacement list, replacing all formal arguments
  // by their actuals.  This handles # operator.
  ProcessFunctionLikeReplacementText(p, macro, ti, current_token_index,
                                     &args, &va_args);
  
  // Now delete the args and va_args.  Note that we don't delete key
  // in the args map since this is owned by the macro itself.  The
  // actual is deleted.
  MapTraverse(&args, DeleteActualArg, NULL);
  MapDestruct(&args);
  StringDestruct(&va_args);
}

// We have a possible macro name.  See if it's a macro or other special
// name and if so, replace it by the replacement text.
static void ProcessPossibleMacro(Preprocessor* p,
                                   String* possible_macro_name,
                                   String* tokenized_line,
                                   TokenIterator* ti,
                                   bool whole_input) {
  String replacement = {0};
  if (StringEqual(possible_macro_name, "_Pragma")) {
    // No pragmas in this compiler.
  } else if (StringEqual(possible_macro_name, "__FILE__")) {
    StringPrintf(&replacement, "\"%s\"", &p->lex->source->filename);
    TokenizeAndReplaceCurrentToken(ti, &replacement, p->lex->assembler_mode);
  } else if (StringEqual(possible_macro_name, "__LINE__")) {
    StringPrintf(&replacement, "%d", p->lex->source->lineno);
    TokenizeAndReplaceCurrentToken(ti, &replacement, p->lex->assembler_mode);
  } else if (StringEqual(possible_macro_name, "__func__") ||
             StringEqual(possible_macro_name, "__FUNCTION__")) {
    if (compiler->current_function != NULL) {
      StringPrintf(
                   &replacement, "\"%s\"",
                   compiler->current_function->info.function.symbol->name.value);
      TokenizeAndReplaceCurrentToken(ti, &replacement, p->lex->assembler_mode);
    }
  } else {
    // Not a predefined macro, let's try a user-defined one.
    Macro* macro = HashTableSearch(&p->macros, possible_macro_name->value);
    if (macro != NULL && macro->enabled) {
      MacroDisable(macro);
      if (macro->is_function_like) {
        // Function-like macro, more complex processing needed.
        ReplaceFunctionLikeMacro(p, macro, ti, tokenized_line, whole_input);
      } else {
        // Object-like macro.  Handle ## by pasting adjacent tokens.
        StringSet(&replacement, macro->replacement_text.value);
        PasteTokens(p, &replacement);
        // Remove placemarker tokens.
        RemovePlacemarkers(p, &replacement);
        
        // Rescan the replacement text for more macros.  This macro is
        // disabled so it won't be replaced.
        ReplaceMacrosInTokenizedLine(p, &replacement, true);
        
        // Replace the current token with the new replacement text.
        ReplaceCurrentToken(ti, &replacement);
      }
      MacroEnable(macro);
    }
  }
  StringDestruct(&replacement);
}

static void ReplaceMacrosInTokenizedLine(Preprocessor* p,
                                         String* tokenized_line,
                                         bool whole_input) {
  TokenIterator ti;
  TokenIteratorInit(&ti, p, tokenized_line);
  while (CurrentToken(&ti) != PPTOK(end)) {
    PreprocessingToken tok = CurrentToken(&ti);
    if (tok == PPTOK(identifier) || tok == PPTOK(other_string)) {
      String possible_macro_name;
      GetCurrentTokenSpelling(&ti, &possible_macro_name);
      
      // See if it's a macro or special name.
      ProcessPossibleMacro(p, &possible_macro_name, tokenized_line,
                           &ti, whole_input);
      
      StringDestruct(&possible_macro_name);
    }
    MoveToNextToken(&ti);
  }
}

// Replace all macros in the given source line.  This can also read
// additional lines for macros that span multiple lines.  The string
// pointed to by 'line' is overwritten by the processed source text.
void PreprocessorReplaceMacros(Preprocessor* p, String* line) {
  if (line->length == 0) {
    return;
  }
  String tokenized_line;
  size_t start = 0;
  if (p->lex->in_comment) {
    start = SkipToEndOfComment(line, 0);
  }
  Tokenize(p, line, &tokenized_line, start, true, p->lex->assembler_mode, false);
  int limit = 20;
  while (--limit > 0) {
    String copy;
    StringInit(&copy, tokenized_line.value);
    ReplaceMacrosInTokenizedLine(p, &copy, false);
    if (StringEqualString(&tokenized_line, &copy)) {
      break;
    }
    StringDestruct(&tokenized_line);
    StringInit(&tokenized_line, copy.value);
    StringDestruct(&copy);
  }
  if (limit == 0) {
    PreprocessorError(p, "Infinite macro expansion detected");
  }
  
  // Detokenize new line into output.
  if (start != 0) {
    // If we started out in a comment, put the comment back at the
    // start of the line.
    line->length = start;
  } else {
    StringClear(line);
  }
  Detokenize(p, &tokenized_line, line);
}


