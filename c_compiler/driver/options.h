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
  kOptionNoStandardIncludes, // Do not use built-in system include paths.
  kOptionNoStandardLibraries,// Do not link the target system library.
  kOptionDefineMacro,        // Define a macro.
  kOptionUndefineMacro,      // Undefine a macro.
  kOptionPic,                // Position Independent Code.
  kOptionExceptions,         // -fexceptions: enable C++ exceptions.
  kOptionNoExceptions,       // -fno-exceptions: disable C++ exceptions.
  kOptionPrintfSpecialize,   // Analyze constant printf-family formats.
  kOptionNoPrintfSpecialize, // Disable printf-family specialization.
  kOptionWarning,            // Disable warning.
  kOptionWerror,             // All warnings are errors.
  kOptionWall,               // Enable all warnings.
  kOptionErrorLimit,         // Max error limit.
  kOptionStandard,           // Language standard.
  kOptionTlsModel,           // TLS model.
  kOptionChdir,              // Change dir before running.
  kOptionPrintFrontend,      // Debug front end.
  kOptionPrintBackend,       // Debug back end.
  kOptionPrintPreprocessor,  // Debug preprocessor.
  kOptionKeepAsmFile,        // Keep asm file after assembling.
  kOptionSaveIR,             // Save IR in file.
  kOptionSaveAST,            // Save AST in file.
  kOptionEmitModule,         // (hidden) Emit a C++20 module (.dcm) file.
  kOptionLoadModule,         // (hidden) Load+verify a C++20 module (.dcm) file.
  kOptionPrebuiltModulePath, // Directory to search for prebuilt .dcm modules.
  kOptionModuleFile,         // Logical module-name=artifact path mapping.
  kOptionModuleHeader,       // Compile input as a C++20 header unit.
  kOptionModuleName,         // Override logical module/header-unit name.
  kOptionModuleOutput,       // Emit module artifact alongside normal output.
  kOptionDepsFile,           // Write P1689 module dependency information.
  kOptionDepsFormat,         // Dependency output format (p1689r5).
  kOptionDepsScanOnly,       // Scan module dependencies without compiling.
  kOptionConstexprEval,      // Constant evaluator: auto, pcode, ast, or audit.
  kOptionContracts,          // Contract semantic: ignore/observe/enforce/quick-enforce.
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
