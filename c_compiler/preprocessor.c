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
#include <sys/param.h>
#include <time.h>
#include <unistd.h>

#include "compiler.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "lex.h"

Macro* NewMacro(const char* name, bool is_function_like, bool varargs,
                Vector* args, const char* replacement_text,
                SourceLocation location) {
  Macro* macro = malloc(sizeof(Macro));
  StringInit(&macro->name, name);
  VectorCopy(&macro->args, args);
  StringInit(&macro->replacement_text, replacement_text);
  macro->is_function_like = is_function_like;
  macro->undefined = false;
  macro->varargs = varargs;
  macro->location = location;
  macro->left = NULL;
  macro->right = NULL;
  return macro;
}

void MacroDestruct(Macro* macro) {
  StringDestruct(&macro->name);
  StringDestruct(&macro->replacement_text);
  VectorDestruct(&macro->args);
}

#define MACROS_TABLE_SIZE 1009

static bool InsertMacro(Macro* table, Macro* macro, Macro** parent) {
  if (table == NULL) {
    *parent = macro;
    return true;
  } else {
    int comp = StringCompareString(&table->name, &macro->name);
    if (comp == 0) {
      // Duplicate macro.
      return false;
    }
    if (comp < 0) {
      return InsertMacro(table->left, macro, &table->left);
    }
    return InsertMacro(table->right, macro, &table->right);
  }
}

static Macro* FindMacro(Macro* table, const char* name) {
  if (table == NULL) {
    return NULL;
  }
  int comp = StringCompare(&table->name, name);
  if (comp == 0) {
    return table;
  }
  if (comp < 0) {
    return FindMacro(table->left, name);
  }
  return FindMacro(table->right, name);
}

static void DeleteMacro(void* macro, void* data) {
  Macro* mac = (Macro*)macro;
  if (mac == NULL) {
    return;
  }
  DeleteMacro(mac->left, data);
  DeleteMacro(mac->right, data);
  MacroDestruct(mac);
  free(mac);
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
  size_t hash = 0;
  for (size_t i = 0; name[i] != '\0'; i++) {
    hash = (hash << 1) ^ name[i];
  }
  return hash;
}

static bool InsertMacroInHashTable(void* entry, void* value, void** parent) {
  return InsertMacro(entry, value, (Macro**)parent);
}

static void* FindMacroInHashTable(void* entry, void* value) {
  return FindMacro(entry, value);
}

static void PredefineMacros(Preprocessor* p) {
  // Define the macros defined by the standard.
  PreprocessorDefineMacro(p, "__STDC__", "1");
  PreprocessorDefineMacro(p, "__STDC_HOSTED__", "1");
  PreprocessorDefineMacro(p, "__STDC_VERSION__", "199901L");
  PreprocessorDefineMacro(p, "__STDC_MB_MIGHT_NEQ_WC__", "1");

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

  // These are defined by GCC and clang and are used in header files.  We need
  // to define them too.
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
  PreprocessorDefineMacro(p, "__INT_FAST8_TYPE__", "int");
  PreprocessorDefineMacro(p, "__INT_FAST16_TYPE__", "int");
  PreprocessorDefineMacro(p, "__INT_FAST32_TYPE__", "int");
  PreprocessorDefineMacro(p, "__INT_FAST64_TYPE__", "int");
  PreprocessorDefineMacro(p, "__UINT_FAST8_TYPE__", "int");
  PreprocessorDefineMacro(p, "__UINT_FAST16_TYPE__", "int");
  PreprocessorDefineMacro(p, "__UINT_FAST32_TYPE__", "int");
  PreprocessorDefineMacro(p, "__UINT_FAST64_TYPE__", "int");
  PreprocessorDefineMacro(p, "__INTPTR_TYPE__", "int*");
  PreprocessorDefineMacro(p, "__UINTPTR_TYPE__", "unsigned int*");
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
  }
}

void PreprocessorInit(Preprocessor* p) {
  HashTableInit(&p->macros, "macros", MACROS_TABLE_SIZE, HashMacro,
                InsertMacroInHashTable, FindMacroInHashTable);
  VectorInit(&p->if_stack);
  p->lex = NULL;
  VectorInit(&p->user_include_paths);
  VectorInit(&p->system_include_paths);
  p->is_compiled_in = true;

  PreprocessorAddSystemIncludePath(p, "/usr/include");
#ifdef __APPLE__
  // TODO: this is all wrong.  There has to be a better way.
  PreprocessorAddSystemIncludePath(
      p,
      "/Applications/Xcode.app/Contents/Developer/Toolchains/"
      "XcodeDefault.xctoolchain/usr/lib/clang/9.1.0/include");
  PreprocessorAddSystemIncludePath(
      p,
      "/Applications/Xcode.app/Contents/Developer/Toolchains/"
      "XcodeDefault.xctoolchain/usr/include");
  PreprocessorAddSystemIncludePath(
      p,
      "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/"
      "Developer/SDKs/MacOSX10.13.sdk/usr/include");
  PreprocessorAddSystemIncludePath(
      p,
      "/Applications/Xcode.app/Contents/Developer/Toolchains/"
      "XcodeDefault.xctoolchain/usr/lib/clang/9.0.0/include");
#endif

  // Add current dir to the include paths.
  char current_dir[MAXPATHLEN];
  getcwd(current_dir, sizeof(current_dir));

  PreprocessorAddUserIncludePath(p, current_dir);

  PredefineMacros(p);
}

void PreprocessorPrintStats(Preprocessor* p) {
  HashTablePrintStats(&p->macros);
}

void PreprocessorFinishInit(Preprocessor* p, struct Lex* lex) { p->lex = lex; }

void PreprocessorDestruct(Preprocessor* p) {
  VectorDestruct(&p->if_stack);

  // Delete all user paths.
  for (size_t i = 0; i < p->user_include_paths.length; i++) {
    StringDelete((String*)p->user_include_paths.value[i]);
  }
  VectorDestruct(&p->user_include_paths);

  // Delete all system paths.
  for (size_t i = 0; i < p->system_include_paths.length; i++) {
    StringDelete((String*)p->system_include_paths.value[i]);
  }
  VectorDestruct(&p->system_include_paths);

  PreprocessorReset(p);
}

void PreprocessorReset(Preprocessor* p) {
  HashTableTraverse(&p->macros, DeleteMacro, NULL);
  HashTableClear(&p->macros);
  PredefineMacros(p);
}

void PreprocessorAddUserIncludePath(Preprocessor* p, const char* path) {
  VectorAppend(&p->user_include_paths, NewString(path));
}

void PreprocessorAddSystemIncludePath(Preprocessor* p, const char* path) {
  VectorAppend(&p->system_include_paths, NewString(path));
}

void PreprocessorDefineMacro(Preprocessor* p, const char* macro_name,
                             const char* value) {
  Macro* macro = HashTableSearch(&p->macros, (char*)macro_name);
  if (macro != NULL) {
    // Macro exists, replace the value with that given.
    StringSet(&macro->replacement_text, value);
    macro->undefined = false;
    return;
  }
  macro = NewMacro(macro_name, false, false, NewVector(), value,
                   SOURCE_LOCATION_COMMAND_LINE);
  HashTableInsert(&p->macros, macro);
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

static void CompactString(String* in, String* out) {
  bool skipping_spaces = true;
  bool in_string = false;
  for (size_t i = 0; i < in->length; i++) {
    char ch = in->value[i];
    if (ch == '"') {
      in_string = !in_string;
    }
    // Check for a comment and skip it.
    if (!in_string && ch == '/') {
      // '//' comment?
      if (i < in->length && in->value[i + 1] == '/') {
        // Single line comment, ends at end of line.
        break;
      } else if (i < in->length - 1 && in->value[i + 1] == '*') {
        // Multi-line comment.  Read until we find the */ at the end.
        i += 2;  // Skip /*.
        do {
          do {
            ch = in->value[i++];
          } while (i < in->length && ch != '*');
          if (i >= in->length) {
            break;
          }
          ch = in->value[i++];
        } while (i < in->length && ch != '/');
        ch = in->value[i];
      }
    }
    if (!in_string && skipping_spaces && isblank(ch)) {
      continue;
    }
    StringAppendChar(out, ch);
    skipping_spaces = isblank(ch);
  }
  // Strip trailing spaces by reducing the length of the output
  // string until we find the first non-space.  Note that because the
  // size_t type is unsigned we cannot check for <0 in the loop.  So we
  // use an index one greater than the element we want to see.
  for (size_t i = out->length; i > 0; i--) {
    char ch = out->value[i - 1];
    if (!isblank(ch)) {
      break;
    }
    out->length--;
  }
}

// Reads an identifier, appending the result to the 'result' argument
// and returning the updated position in the input line.
static size_t ReadIdentifier(String* line, size_t pos, String* result) {
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
  if (macro->is_function_like != is_function_like ||
      macro->varargs != varargs) {
    return false;
  }

  // The replacement text has already been compacted.
  if (!StringEqualString(&macro->replacement_text, replacement_text)) {
    return false;
  }
  if (macro->args.length != args->length) {
    return false;
  }
  for (size_t i = 0; i < args->length; i++) {
    String* old = (String*)macro->args.value[i];
    String* new = (String*)args->value[i];
    if (!StringEqualString(old, new)) {
      return false;
    }
  }
  return true;
}

static bool ParseSourceControlLine(Preprocessor* p, String* line, size_t pos) {
  // Skip spaces.
  pos = SkipSpacesAndComments(p, pos, line, NULL);
  if (pos < line->length && isdigit(line->value[pos])) {
    int lineno = 0;

    // Read the new source's line number.
    while (pos < line->length && isdigit(line->value[pos])) {
      lineno = lineno * 10 + line->value[pos++] - '0';
    }

    // Skip spaces up to filename.
    pos = SkipSpacesAndComments(p, pos, line, NULL);

    // String for filename.
    String filename;
    StringInit(&filename, NULL);

    // Read filename if it is present.
    ReadString(line, pos, &filename);

    // Set the line number in the source.
    p->lex->source->lineno = lineno;

    // If there is a filename, set it in the source, otherwise
    // it's a reset to the previous source filename.
    if (filename.length != 0) {
      StringSetString(&p->lex->source->filename, &filename);
    } else {
      StringSetString(&p->lex->source->filename, &p->lex->source->original);
    }
    return true;
  }
  return false;
}

// Given a path (vector of strings) and a filename, search the path
// for the file and open it if found.  If it is found, sets the
// filename string to the pathname.  Returns NULL if the file couldn't
// be found or couldn't be opened due to permissions problems.
static FILE* FindFileInPath(Vector* path, String* filename, size_t* start) {
  for (size_t i = *start; i < path->length; i++) {
    String pathname;
    StringInit(&pathname, NULL);
    StringPrintf(&pathname, "%s/%s", ((String*)path->value[i])->value,
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

static void Define(Preprocessor* p, String* line, size_t pos) {
  if (!p->is_compiled_in) {
    // Ignore this if it is #ifed out.
    return;
  }

  // Collect the macro name (the identifier after #define) into a string
  String macro_name;
  StringInit(&macro_name, NULL);

  size_t name_start = pos;
  pos = ReadIdentifier(line, pos, &macro_name);
  size_t name_end = pos;

  String replacement_text;  // Value of macro.
  bool function_like_macro = false;
  bool varargs = false;

  // Arguments for function-like macro.
  Vector args;
  VectorInit(&args);

  // No space allowed before open paren for function-like macro.
  if (line->value[pos] == '(') {
    // Function like macro, collect the arguments.
    function_like_macro = true;
    pos++;  // Skip open paren.
    pos = SkipSpacesAndComments(p, pos, line, NULL);

    while (line->value[pos] != ')') {
      String arg;
      StringInit(&arg, NULL);

      pos = SkipSpacesAndComments(p, pos, line, NULL);

      // Check for ... and if so we terminated the arguments and mark
      // this macro has having a variable number of args.
      if (line->value[pos] == '.' && line->value[pos + 1] == '.' &&
          line->value[pos + 2] == '.') {
        varargs = true;
        pos += 3;
        break;
      }

      // Read the argument name.
      pos = ReadIdentifier(line, pos, &arg);

      // Make sure it's not already defined in this macro.
      for (size_t i = 0; i < args.length; i++) {
        if (StringEqualString((String*)args.value[i], &arg)) {
          PreprocessorError(p, "Duplicate macro argument %s", arg.value);
          return;
        }
      }

      // Add argument to the set of known arguments.
      VectorAppend(&args, NewString(arg.value));

      // Check for more arguments.
      pos = SkipSpacesAndComments(p, pos, line, NULL);
      if (line->value[pos] != ',') {
        break;
      }
      pos++;  // Skip comma.
    }
    if (line->value[pos] != ')') {
      PreprocessorError(p, "Missing ')' for function-like macro %s",
                        macro_name.value);
      return;
    }
    pos++;  // Skip close paren.
  }

  // Skip any spaces or comments before the replacement text.
  pos = SkipSpacesAndComments(p, pos, line, NULL);

  // Read the replacement text.  This just reads up the the end of line, which
  // has already been processed by appending all lines ending in \.  Then we
  // need to compact the string by removing leading and trailing spaces and
  // replacing all multi-spaces by a single one.
  String raw_replacement_text;
  StringInit(&raw_replacement_text, &line->value[pos]);
  StringInit(&replacement_text, NULL);
  CompactString(&raw_replacement_text, &replacement_text);

  // Check if the macro has already been defined and if so, make sure this
  // definition is the same as the old old.
  Macro* macro = HashTableSearch(&p->macros, macro_name.value);
  if (macro == NULL) {
    macro = NewMacro(macro_name.value, function_like_macro, varargs, &args,
                     replacement_text.value,
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
  StringInit(&macro_name, NULL);
  pos = ReadIdentifier(line, pos, &macro_name);
  Macro* macro = HashTableSearch(&p->macros, macro_name.value);
  if (macro != NULL) {
    macro->undefined = true;
  }

  pos = SkipSpacesAndComments(p, pos, line, NULL);
  if (pos < line->length) {
    PreprocessorWarning(p, "extra-tokens", "Extra tokens after #undef");
  }
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

static void DoInclude(Preprocessor* p, String* line, size_t pos,
                      size_t start_index) {
  if (!p->is_compiled_in) {
    return;
  }
  pos = SkipSpacesAndComments(p, pos, line, NULL);

  bool system_include = false;  // #include <...> used.

  // Filename of file to find
  String filename;

  if (!PreprocessorParseIncludeFilename(p, line, &pos, &filename,
                                        &system_include)) {
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
    // There is no point in continuing to compile if we can't find
    // an include file.  This will generated many, many errors for undefined
    // types and functions.
    PreprocessorError(p, "Fatal error: cannot open include file %s",
                      filename.value);
    exit(1);
  }

  // We have found the include file, process it as if it was inline
  // in the current source.

  // Allocate a new source provider from the include file and push
  // it as the current source in the Lex.
  Source* include_source = NewSourceFromFile(filename.value, fp);
  include_source->prev = p->lex->source;
  printf("Including file %s\n", filename.value);

  // Save current path index and set new current.
  p->lex->source->path_index = compiler->current_include_path_index;
  compiler->current_include_path_index = path_index;

  p->lex->source = include_source;
  // Done with this string since the Source will create a new one.
  StringDestruct(&filename);

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
// An earilier compiled-out block dominates all lower level
// blocks.
static void UpdateState(Preprocessor* p) {
  for (size_t i = 0; i < p->if_stack.length; i++) {
    if (p->if_stack.value[i] == NULL || p->if_stack.value[i] == (void*)(-1LL)) {
      p->is_compiled_in = false;
      return;
    }
  }
  p->is_compiled_in = true;
}

// #if processing.
static void If(Preprocessor* p, String* line, size_t pos) {
  static int true_value;

  // #if processing needs to evaluate the expression only if the current
  // state of conditional processing is true.
  if (!p->is_compiled_in) {
    VectorPush(&p->if_stack, NULL);
    return;
  }

  // Allow the lexical analyzer to see the controlling expression.
  p->is_compiled_in = true;

  String controlling_expr;
  StringInit(&controlling_expr, &line->value[pos]);
  StringAppend(&controlling_expr, "\n\n");

  Lex lex;
  Lex* prev_lex = p->lex;
  LexInitFromString(&lex, p->lex->source->filename.value, &controlling_expr, p);
  lex.preprocessor_mode = true;
  lex.assembler_mode = p->lex->assembler_mode;
  LexNextToken(&lex);

  Syntax syntax;
  SyntaxInit(&syntax, &lex);
  void* controlling_value = NULL;
  ASTNode* expr = SyntaxParseExpression(&syntax, 0);
  if (expr != NULL) {
    AnalyzeExpression(expr);
    int64_t value;
    if (EvaluateIntegerExpression(expr, &value)) {
      controlling_value = value == 0 ? NULL : &true_value;
    }
  }

  VectorPush(&p->if_stack, controlling_value);

  StringDestruct(&controlling_expr);
  SyntaxDestruct(&syntax);
  lex.source = NULL;  // Prevent Lex from freeing source.
  LexDestruct(&lex);
  p->lex = prev_lex;

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
  StringInit(&macro_name, NULL);
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
  StringInit(&macro_name, NULL);
  pos = ReadIdentifier(line, pos, &macro_name);
  if (macro_name.length == 0) {
    PreprocessorError(p, "Expected macro name after #ifdef");
    // An empty macro name will always be missing from the table
    // of macros so it won't be found.
  }
  Macro* macro = HashTableSearch(&p->macros, macro_name.value);
  VectorPush(&p->if_stack, macro);
  StringDestruct(&macro_name);

  pos = SkipSpacesAndComments(p, pos, line, NULL);
  if (pos < line->length) {
    PreprocessorWarning(p, "extra-tokens", "Extra tokens after #ifdef");
  }
  UpdateState(p);
}

static void Elif(Preprocessor* p, String* line, size_t pos) {
  static int true_value;

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
  void* top_value = p->if_stack.value[p->if_stack.length - 1];
  if (top_value == NULL) {
    // Need to evaulate expression.
  } else if (top_value == (void*)(-1LL)) {
    // The #if..#elif... sequence was compiled in at some point.  Leave this
    // as is on the stack.
    return;
  } else {
    // Replace the top of the if_stack with -1 to tell all subsequent #elif
    // and #else that this #if block has been compiled in.
    p->if_stack.value[p->if_stack.length - 1] = (void*)(-1LL);
    UpdateState(p);
    return;
  }

  // Allow the lexical analyzer to see the controlling expression.
  p->is_compiled_in = true;

  String controlling_expr;
  StringInit(&controlling_expr, &line->value[pos]);
  StringAppend(&controlling_expr, "\n\n");

  Lex lex;
  Lex* prev_lex = p->lex;
  LexInitFromString(&lex, p->lex->source->filename.value, &controlling_expr, p);
  lex.preprocessor_mode = true;
  lex.assembler_mode = p->lex->assembler_mode;
  LexNextToken(&lex);

  Syntax syntax;
  SyntaxInit(&syntax, &lex);
  void* controlling_value = NULL;
  ASTNode* expr = SyntaxParseExpression(&syntax, 0);
  if (expr != NULL) {
    AnalyzeExpression(expr);
    int64_t value;
    if (EvaluateIntegerExpression(expr, &value)) {
      controlling_value = value == 0 ? NULL : &true_value;
    }
  }

  // Replace the top of the if_stack with the new controlling value.
  p->if_stack.value[p->if_stack.length - 1] = controlling_value;

  StringDestruct(&controlling_expr);
  SyntaxDestruct(&syntax);
  lex.source = NULL;  // Prevent Lex from freeing source.
  LexDestruct(&lex);
  p->lex = prev_lex;

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
  void* top_value = p->if_stack.value[p->if_stack.length - 1];
  p->if_stack.value[p->if_stack.length - 1] =
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
  if (!ParseSourceControlLine(p, line, pos)) {
    PreprocessorError(p, "Invalid #line directive");
  }
}

static void Error(Preprocessor* p, String* line, size_t pos) {
  if (!p->is_compiled_in) {
    return;
  }
  String error;
  StringInit(&error, NULL);
  if (line->value[pos] == '"') {
    ReadString(line, pos, &error);
  } else {
    StringAppend(&error, &line->value[pos]);  // Rest of line
  }
  PreprocessorError(p, error.value);
  StringDestruct(&error);
}

static void Warning(Preprocessor* p, String* line, size_t pos) {
  if (!p->is_compiled_in) {
    return;
  }
  String error;
  StringInit(&error, NULL);
  if (line->value[pos] == '"') {
    ReadString(line, pos, &error);
  } else {
    StringAppend(&error, &line->value[pos]);  // Rest of line
  }
  PreprocessorWarning(p, "preprocessor", error.value);
  StringDestruct(&error);
}

static void Pragma(Preprocessor* p, String* line, size_t pos) {
  if (!p->is_compiled_in) {
    return;
  }
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
  size_t pos = SkipSpacesAndComments(p, 0, line, NULL);
  if (line->value[pos] != '#') {
    return false;
  }
  pos++;
  pos = SkipSpacesAndComments(p, pos, line, NULL);

  String command_name;
  StringInit(&command_name, NULL);

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
    // Unknown preprocessor command, check for control line and if not
    // we have an unknown preprocessor directive.  Only give this error
    // if the line is compiled in.
    if (!ParseSourceControlLine(p, line, pos)) {
      if (p->is_compiled_in) {
        PreprocessorError(p, "Invalid preprocessor directive %s",
                          command_name.value);
      }
    }
    return true;
  }

  // Run the command parser.
  command(p, line, pos);
  return true;
}

Macro* PreprocessorFindMacro(Preprocessor* p, String* macro_name) {
  return HashTableSearch(&p->macros, macro_name->value);
}

bool PreprocessorLineIsCompiledIn(Preprocessor* p) { return p->is_compiled_in; }

// Skip forward in the input line to the next possible macro name.  Any
// characters skipped are copied to the 'newline'.
static size_t SkipToMacroName(Preprocessor* p, String* line, size_t pos,
                              String* newline, bool* hash, bool* hashhash) {
  bool in_string = false;
  bool in_comment = p->lex->in_comment;
  char prev_char = '\0';
  while (pos < line->length) {
    if (line->value[pos] == '"') {
      if (prev_char != '\\') {
        in_string = !in_string;
      }
    }

    if (!in_string) {
      if (!in_comment && line->value[pos] == '/' &&
          line->value[pos + 1] == '/') {
        // //comment, skip to end of line
        while (pos < line->length) {
          StringAppendChar(newline, line->value[pos++]);
        }
        return pos;
      }
      if (!in_comment && line->value[pos] == '/' &&
          line->value[pos + 1] == '*') {
        pos += 2;
        in_comment = true;
        StringAppend(newline, "/*");
      }
      if (in_comment && line->value[pos] == '*' &&
          line->value[pos + 1] == '/') {
        pos += 2;
        in_comment = false;
        StringAppend(newline, "*/");
      }

      if (in_comment) {
        StringAppendChar(newline, line->value[pos]);
        pos++;
        continue;
      }
      if ((isalpha(line->value[pos]) || line->value[pos] == '_') &&
          !isdigit(prev_char)) {
        break;
      }
      if (line->value[pos] == '#') {
        if (line->value[pos + 1] == '#') {
          *hashhash = true;
          pos += 2;
        } else {
          *hash = true;
          pos++;
        }
        break;
      }
    }

    // Merge adjacent spaces into one.
    if (in_string || !(isblank(prev_char) && isblank(line->value[pos]))) {
      prev_char = line->value[pos];
      StringAppendChar(newline, prev_char);
    }
    pos++;
  }
  return pos;
}

typedef struct {
  String* formal;
  String* actual;
} Arg;

static String* FindMacroArg(Vector* args, String* name) {
  for (size_t i = 0; i < args->length; i++) {
    Arg* arg = args->value[i];
    if (StringEqualString(arg->formal, name)) {
      return arg->actual;
    }
  }
  return NULL;
}

// We have a function-like macro invokation.  It will be followed by
// a set of actual arguments.
static size_t ReplaceFunctionLikeMacro(Preprocessor* p, Macro* macro,
                                       String* line, size_t pos,
                                       String* newline) {
  pos = SkipSpacesAndComments(p, pos, line, NULL);
  if (line->value[pos] != '(') {
    PreprocessorError(p, "Missing '(' for function-like macro");
    return pos;
  }
  pos++;  // Skip (.

  // Collect the actual arguments.
  int formal_index = 0;
  int actual_index = 0;
  bool too_many_args = false;
  const char* va_arg_separator = "";

  // Mapping of formal to actual for each argument.   Each memory of the 'args'
  // vector is a pointer to an Arg object.
  Vector args;
  VectorInit(&args);
  String va_args;
  StringInit(&va_args, NULL);

  while (!SourceEof(p->lex->source) && line->value[pos] != ')') {
    String* actual = NewString(NULL);
    int num_nested_brackets = 1;
    bool in_string = false;
    while (!SourceEof(p->lex->source)) {
      pos = SkipSpacesAndComments(p, pos, line, actual);
      if (pos >= line->length) {
        // In a function-like macro invokation the language allows newline
        // characters to be treated as spaces.  This means we need to read
        // another line when we encounter the end of line.
        StringClear(line);
        SourceReadLine(p->lex->source, line);
        pos = 0;
        continue;
      }
      char ch = line->value[pos];
      if (ch == '"') {
        in_string = !in_string;
      } else if (ch == '\\') {
        StringAppendChar(actual, ch);
        pos++;
        StringAppendChar(actual, line->value[pos++]);
        continue;
      }

      if (!in_string) {
        if (ch == '(') {
          num_nested_brackets++;
        } else if (ch == ')') {
          num_nested_brackets--;
          if (num_nested_brackets == 0) {
            // Close paren at end of actual.
            break;
          }
        }
        if (num_nested_brackets == 1 && ch == ',') {
          // Comma outside of nested brackets, end of actual.
          break;
        }
      }
      StringAppendChar(actual, ch);
      pos++;
    }

    // Replace all macros in the actual argument.
    PreprocessorReplaceMacros(p, actual);

    if (actual_index >= macro->args.length) {
      if (macro->varargs) {
        // Append actual arg to the va_args string, separated by comma from
        // the previous one.
        StringAppend(&va_args, va_arg_separator);
        StringAppend(&va_args, actual->value);
        StringDelete(actual);
        va_arg_separator = ",";
      } else {
        // Note the fact that we have too many arguments for an non-varargs
        // macro.
        too_many_args = true;
      }
    } else {
      Arg* arg = malloc(sizeof(Arg));
      arg->formal = macro->args.value[formal_index];
      arg->actual = actual;
      VectorAppend(&args, arg);
      formal_index++;
    }
    actual_index++;
    if (line->value[pos] == ',') {
      pos++;
    }
  }
  if (line->value[pos] != ')') {
    PreprocessorError(p, "Missing ')' for function like macro arguments");
  } else {
    pos++;
  }

  if (too_many_args) {
    PreprocessorError(
        p, "Too many actual arguments for macro %s; expected %zd, got %d)",
        macro->name.value, macro->args.length, actual_index);
  }
  if (actual_index < formal_index) {
    PreprocessorError(
        p, "Insufficient actual arguments for macro %s; expected %zd, got %d)",
        macro->name.value, macro->args.length, actual_index);
  }

  // Now we process the macro replacement list, replacing all formal arguments
  // by their actuals.  This handles # and ## operators.
  String temp;
  StringInit(&temp, NULL);

  size_t i = 0;

  // Alias to avoid excess typing.
  String* rep = &macro->replacement_text;

  while (i < rep->length) {
    bool hash = false;
    bool hashhash = false;
    i = SkipToMacroName(p, rep, i, &temp, &hash, &hashhash);
    if (i >= rep->length) {
      break;
    }
    String possible_arg;
    StringInit(&possible_arg, NULL);
    i = ReadIdentifier(rep, i, &possible_arg);
    if (StringEqual(&possible_arg, "__VA_ARGS__")) {
      if (macro->varargs) {
        StringAppendString(&temp, &va_args);
      } else {
        PreprocessorError(p, "Use of __VA_ARGS__ outside of varargs macro");
      }
    } else {
      if (hash) {
        // # must be followed by an argument name to be effective.
        String* actual = FindMacroArg(&args, &possible_arg);
        if (actual == NULL) {
          // Not an argument, retain original text.
          StringAppendChar(&temp, '#');
          StringAppendString(&temp, &possible_arg);
        } else {
          // Form a string literal out of the argument value, but first compact
          // it by trimming spaces at both ends and replacing multiple spaces by
          // single spaces.  Special characters in argumenta are then escaped.
          String literal;
          StringInit(&literal, NULL);
          CompactString(actual, &literal);
          StringAppendChar(&temp, '"');
          StringEscape(&literal, &temp);
          StringAppendChar(&temp, '"');
          StringDestruct(&literal);
        }
        continue;
      } else if (hashhash) {
        // Back up to the first non-space char.
        StringTrimEnd(&temp);
        i = SkipSpacesAndComments(p, i, rep, NULL);
      }
      String* actual = FindMacroArg(&args, &possible_arg);
      if (actual != NULL) {
        StringAppendString(&temp, actual);
      } else {
        StringAppendString(&temp, &possible_arg);
      }
    }
  }
  StringAppendString(newline, &temp);
  StringDestruct(&temp);

  // Now delete the args and va_args.  Note that we don't delete the formal
  // field in the args vector since this is owned by the macro itself.  The
  // actual is deleted.
  for (size_t i = 0; i < args.length; i++) {
    Arg* arg = args.value[i];
    StringDelete(arg->actual);
  }
  VectorDestruct(&args);
  StringDestruct(&va_args);
  return pos;
}

void PreprocessorReplaceMacros(Preprocessor* p, String* line) {
  int total_replacements = 0;
  const int kMaxMacroReplacements = 1000;

  int num_replacements = 0;
  do {
    size_t pos = 0;
    num_replacements = 0;

    String newline;
    StringInit(&newline, NULL);

    while (pos < line->length) {
      bool hash = false;
      bool hashhash = false;
      pos = SkipToMacroName(p, line, pos, &newline, &hash, &hashhash);
      if (pos >= line->length) {
        break;
      }
      if (hash) {
        // Keep # token.
        StringAppendChar(&newline, '#');
        continue;
      } else if (hashhash) {
        // ## tokens are deleted.
        // TODO: does this concatenate?
        continue;
      }

      String possible_macro_name;
      StringInit(&possible_macro_name, NULL);
      pos = ReadIdentifier(line, pos, &possible_macro_name);
      if (StringEqual(&possible_macro_name, "defined")) {
        // We don't replace any macros in the whole "defined" unary operator
        // because this needs to be seen by the expression parser.  So we
        // just copy the 'defined' and the following (possibly parenthesized)
        // macro name to the newline.
        StringAppendString(&newline, &possible_macro_name);
        pos = SkipSpacesAndComments(p, pos, line, &newline);
        if (line->value[pos] == '(') {
          StringAppendChar(&newline, '(');
          StringClear(&possible_macro_name);
          pos++;
          pos = ReadIdentifier(line, pos, &possible_macro_name);
          StringAppendString(&newline, &possible_macro_name);
          if (line->value[pos] == ')') {
            StringAppendChar(&newline, ')');
            pos++;
          }
        } else {
          pos = ReadIdentifier(line, pos, &possible_macro_name);
          StringAppendString(&newline, &possible_macro_name);
        }
      } else if (StringEqual(&possible_macro_name, "_Pragma")) {
        // TODO:
      } else if (StringEqual(&possible_macro_name, "__FILE__")) {
        StringPrintf(&newline, "\"%s\"", &p->lex->source->filename);
      } else if (StringEqual(&possible_macro_name, "__LINE__")) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", p->lex->source->lineno);
        StringAppend(&newline, buf);
      } else if (StringEqual(&possible_macro_name, "__func__") ||
                 StringEqual(&possible_macro_name, "__FUNCTION__")) {
        if (compiler->current_function != NULL) {
          StringPrintf(
              &newline, "\"%s\"",
              compiler->current_function->info.function.symbol->name.value);
        }
      } else {
        // Not a predefined macro, let's try a user-defined one.
        Macro* macro = HashTableSearch(&p->macros, possible_macro_name.value);
        if (macro != NULL) {
          // printf("replacing macro %s with %s\n",possible_macro_name.value,
          // macro->replacement_text.value);
          if (macro->is_function_like) {
            // Function-like macro, more complex processing needed.
            pos = ReplaceFunctionLikeMacro(p, macro, line, pos, &newline);
          } else {
            // Regular macro.
            StringAppendString(&newline, &macro->replacement_text);
          }
          num_replacements++;
        } else {
          StringAppendString(&newline, &possible_macro_name);
        }
      }
      StringDestruct(&possible_macro_name);
    }

    StringSetString(line, &newline);
    StringDestruct(&newline);

    total_replacements += num_replacements;

    // Check for infinite recursion.
    if (total_replacements > kMaxMacroReplacements) {
      PreprocessorError(p, "Macro recursion detected");
      return;
    }
  } while (num_replacements > 0);
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
