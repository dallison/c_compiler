//
//  options.h
//  c_compiler_library
//
//  Created by David Allison on 2/16/23.
//  Copyright © 2023 David Allison. All rights reserved.
//

#ifndef options_h
#define options_h

#include <stdbool.h>
#include "dstring.h"
#include "vector.h"

typedef enum {
  kOptionInputFile,          // File being compiled.
  kOptionDebug,              // Generate debug output.
  kOptionOptimize,           // Optimize code.
  kOptionTarget,             // Target architecture.
  kOptionCompileOnly,        // Compile only, don't link.
  kOptionAssemblyOutput,     // Output asembly only, dont assemble.
  kOptionOutputFile,         // Output file name.
  kOptionIncludePath,        // Add to user include search path.
  kOptionSystemIncludePath,  // Add to system include search path.
  kOptionDefineMacro,        // Define a macro.
  kOptionUndefineMacro,      // Undefine a macro.
  kOptionPic,                // Position Independent Code.
  kOptionWarning,            // Disable warning.
  kOptionWerror,             // All warnings are errors.
  kOptionWall,               // Enable all warnings.
  kOptionErrorLimit,         // Max error limit.
  kOptionTlsModel,           // TLS model.
  kOptionChdir,              // Change dir before running.
  kOptionPrintFrontend,      // Debug front end.
  kOptionPrintBackend,       // Debug back end.
  kOptionPrintPreprocessor,  // Debug preprocessor.
  kOptionKeepAsmFile,        // Keep asm file after assembling.
  kOptionSaveIR,             // Save IR in file.
  kOptionSaveAST,            // Save AST in file.
} CompilerOption;

// This holds the strings from the command line, split into two
// at an equals sign if present.
typedef struct {
  String name;
  String value;
} CompilerOptionString;

typedef struct {
  CompilerOption opt;   // Option identifier.
  union {
    int ivalue;         // Integer value.
    String svalue;      // String value.
    bool bvalue;        // Boolean value.
  } value;
} CompilerOptionValue;

typedef enum {
  kCompilerOptionString,
  kCompilerOptionInt,
  kCompilerOptionBool,
} CompilerOptionType;

typedef struct {
  const char* name;  // -name ("" means positional arg)
  CompilerOptionType type;
  CompilerOption opt;
  bool is_prefix;
  const char* help;
} CompilerOptionDefinition;

CompilerOptionString* NewOptionString(const char* name);
CompilerOptionString* NewOptionStringWithValue(const char* name, size_t namelen, const char* value);
void CompilerOptionStringDelete(CompilerOptionString* c);

Vector* ParseOptions(int argc, char** argv, Vector* options);
Vector* ParseOptionSet(CompilerOptionDefinition* target_opts, Vector* strings, Vector* options);

String* OptionStringValue(CompilerOption option, Vector* options);
int OptionIntValue(CompilerOption option, Vector* options, int def);
bool OptionBoolValue(CompilerOption option, Vector* options, bool def);

void PrintAllOptions(CompilerOptionDefinition* options);

#endif /* options_h */
