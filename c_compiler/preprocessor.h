//
//  preprocessor.h
//  c_compiler
//
//  Created by David Allison on 11/13/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef preprocessor_h
#define preprocessor_h

#include "dstring.h"
#include "hashtable.h"
#include "source.h"
#include "vector.h"
#include "binary_tree.h"

// This is a macro defined using #define (or predefined).  Macros are held
// in a hash table of binary trees so the lookup is quick.
typedef struct Macro {
  BinaryTreeNode header;    // Embedded binary tree node.
  String name;              // Macro name.
  Vector args;              // Arg names (for function-like macro).
  String replacement_text;  // Value of macro (tokenized).
  bool is_function_like;    // Is the macro a function-like macro?
  bool undefined;           // If the user says #undef, this is true.
  bool varargs;             // Variable args.
  bool enabled;             // Macro is enabled.
  SourceLocation location;  // Where the macro was defined.
} Macro;

Macro* NewMacro(const char* name, bool is_function_like, bool varargs,
                Vector* args, String* replacement_text,
                SourceLocation location);
void MacroDestruct(Macro* macro);
void MacroDisable(Macro* macro);
void MacroEnable(Macro* macro);

typedef struct {
  HashTable macros;
  Vector if_stack;
  Vector user_include_paths;
  Vector system_include_paths;
  struct Lex* lex;
  bool is_compiled_in;  // Is the current state compiled in?
} Preprocessor;

void PreprocessorInit(Preprocessor* p);
void PreprocessorDestruct(Preprocessor* p);
void PreprocessorReset(Preprocessor* p);

void PreprocessorAddUserIncludePath(Preprocessor* p, const char* path);
void PreprocessorAddSystemIncludePath(Preprocessor* p, const char* path);
void PreprocessorDefineMacro(Preprocessor* p, const char* macro_name,
                             const char* value);

void PreprocessorReplaceMacros(Preprocessor* p, String* line);

bool PreprocessorParseDirective(Preprocessor* p, String* line);
Macro* PreprocessorFindMacro(Preprocessor* p, String* macro_name);

bool PreprocessorLineIsCompiledIn(Preprocessor* p);

void PreprocessorPrintStats(Preprocessor* p);

void PreprocessorDefineArchitectureMacros(Preprocessor* p);

bool PreprocessorParseIncludeFilename(Preprocessor* p, String* line,
                                      size_t* pos, String* filename,
                                      bool* system_include);

bool PreprocessorHasInclude(Preprocessor* p, String* filename,
                            bool system_include);
bool PreprocessorHasIncludeNext(Preprocessor* p, String* filename,
                                bool system_include);

#endif /* preprocessor_h */
