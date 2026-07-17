//
//  errors.c
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "set.h"
#include "compiler.h"

// Error and warnings.

int NumErrors() { return compiler->num_errors; }

void SetMaxErrors(int n) { compiler->max_errors = n; }

#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_ERROR "\033[1;31m"
#define ANSI_WARNING "\033[1;35m"
#define ANSI_NOTE "\033[1;36m"
#define ANSI_OPTION "\033[36m"

static bool DiagnosticsUseColor(void) {
  static int use_color = -1;
  if (use_color == -1) {
    use_color = isatty(STDERR_FILENO) && getenv("NO_COLOR") == NULL;
  }
  return use_color != 0;
}

static const char* Color(const char* code) {
  return DiagnosticsUseColor() ? code : "";
}

static void PrintDiagnosticLocation(const char* filename, int lineno) {
  fprintf(stderr, "%s%s", Color(ANSI_BOLD), filename);
  if (lineno != 0) {
    fprintf(stderr, ":%d", lineno);
  }
  fprintf(stderr, "%s: ", Color(ANSI_RESET));
}

static void PrintDiagnosticKind(const char* kind, const char* kind_color) {
  fprintf(stderr, "%s%s%s", Color(kind_color), kind, Color(ANSI_RESET));
}

enum {
  kWarningGroupWall = 1 << 0,
  kWarningGroupExtra = 1 << 1,
  kWarningGroupPedantic = 1 << 2,
  kWarningGroupEverything = 1 << 3,
};

typedef struct WarningInfo {
  const char* name;
  const char* parent;
  bool clang;
  bool gcc;
  bool default_enabled;
  unsigned groups;
} WarningInfo;

// Central warning registry.  The compiler still stores warning state by name,
// but the registry lets command-line handling expand groups and validate names.
static const WarningInfo kWarnings[] = {
    {"main", NULL, true, true, true, kWarningGroupWall},
    {"extern-initializer", NULL, true, false, true, kWarningGroupWall},
    {"attributes", NULL, true, true, false, kWarningGroupWall},
    {"duplicate-decl-specifier", NULL, true, true, true, kWarningGroupWall},
    {"implicit-function-declaration", NULL, true, true, true, kWarningGroupWall},
    {"unused-variable", NULL, true, true, false, kWarningGroupWall},
    {"macro-redefined", NULL, true, false, true, 0},
    {"extra-tokens", NULL, true, false, true, 0},
    {"return-type", NULL, true, true, true, kWarningGroupWall},
    {"unused-result", NULL, true, true, true, kWarningGroupWall},
    {"format", NULL, true, true, false, kWarningGroupWall},
    {"format-zero-length", "format", true, true, true, kWarningGroupWall},
    {"format-invalid-specifier", "format", true, false, true, kWarningGroupWall},
    {"format-nonliteral", "format", true, true, false, kWarningGroupWall},
    {"format-security", "format", true, true, false, kWarningGroupWall},
    {"switch", NULL, true, true, false, kWarningGroupWall},
    {"switch-enum", "switch", true, true, true, kWarningGroupWall},
    {"switch-default", "switch", true, true, false, kWarningGroupExtra},
    {"uninitialized", NULL, true, true, false, kWarningGroupWall},
    {"conversion", NULL, true, true, false, 0},
    {"sign-compare", NULL, true, true, false, kWarningGroupExtra},
    {"pointer-sign", NULL, true, true, false, kWarningGroupWall},
    {"discarded-qualifiers", NULL, true, true, false, kWarningGroupWall},
    {"int-conversion", NULL, true, true, false, kWarningGroupWall},
    {"incompatible-pointer-types", NULL, true, true, true, kWarningGroupWall},
    {"deprecated-declarations", NULL, true, true, true, kWarningGroupWall},
    {"implicit-int", NULL, true, true, false, kWarningGroupWall | kWarningGroupPedantic},
    {"strict-prototypes", NULL, true, true, false, kWarningGroupExtra},
    {"old-style-definition", NULL, true, true, false, kWarningGroupExtra},
    {"declaration-after-statement", NULL, true, true, false, kWarningGroupPedantic},
    {"reorder-ctor-init", NULL, true, true, true, kWarningGroupWall},
    {"unused-parameter", NULL, true, true, false, kWarningGroupExtra},
    // gcc has no equivalent; clang enables -Wunused-private-field.  Grouped with
    // the other unused-entity diagnostics under -Wall.
    {"unused-private-field", NULL, true, false, false, kWarningGroupWall},
    // A local variable that is assigned to but whose value is never read.
    {"unused-but-set-variable", NULL, true, true, false, kWarningGroupWall},
    // An unused typedef/alias declared in a function body.
    {"unused-local-typedef", NULL, true, true, false, kWarningGroupWall},
    // An unused file-scope const/constexpr object with internal linkage.
    {"unused-const-variable", NULL, true, true, false, kWarningGroupWall},
    {"unused-function", NULL, true, true, false, kWarningGroupWall},
    {"shadow", NULL, true, true, false, 0},
    {"unused-label", NULL, true, true, false, kWarningGroupWall},
    {"unused-value", NULL, true, true, false, kWarningGroupWall},
    {"undef", NULL, true, true, false, kWarningGroupWall},
    {"unknown-pragmas", NULL, true, true, false, kWarningGroupWall},
    {"comment", NULL, true, true, false, kWarningGroupWall},
    {"multichar", NULL, true, true, false, kWarningGroupWall},
    {"pragma-messages", NULL, true, false, true, 0},
    {"unknown-warning-option", NULL, true, false, true, 0},
    {"preprocessor", NULL, false, false, true, 0},
    {"pointer-types", NULL, false, false, false, 0},
    {"missing-template-keyword", NULL, true, true, true, kWarningGroupWall},
    // Off by default and not part of -Wall/-Wextra, matching gcc and clang;
    // enabled explicitly via -Wsuggest-override or -Weverything.
    {"suggest-override", NULL, true, true, false, 0},
};

static bool IsWarningDisabled(const char* warning);

bool WarningIsEnabled(const char* warning) {
  return !IsWarningDisabled(warning);
}

static const WarningInfo* FindWarning(const char* name) {
  for (size_t i = 0; i < sizeof(kWarnings) / sizeof(kWarnings[0]); i++) {
    if (strcmp(kWarnings[i].name, name) == 0) {
      return &kWarnings[i];
    }
  }
  return NULL;
}

static unsigned GroupMask(const char* group) {
  if (strcmp(group, "all") == 0) {
    return kWarningGroupWall;
  }
  if (strcmp(group, "extra") == 0) {
    return kWarningGroupExtra;
  }
  if (strcmp(group, "pedantic") == 0) {
    return kWarningGroupPedantic;
  }
  if (strcmp(group, "everything") == 0) {
    return kWarningGroupEverything;
  }
  return 0;
}

bool WarningExists(const char* warning) {
  return FindWarning(warning) != NULL || WarningGroupExists(warning);
}

bool WarningGroupExists(const char* group) {
  return GroupMask(group) != 0;
}

void DisableWarning(const char* warning) {
  SetInsert(&compiler->disabled_warnings, (void*)warning);
}

void EnableWarning(const char* warning) {
  SetRemove(&compiler->disabled_warnings, (void*)warning);
}

void EnableWarningGroup(const char* group) {
  unsigned mask = GroupMask(group);
  for (size_t i = 0; i < sizeof(kWarnings) / sizeof(kWarnings[0]); i++) {
    if (mask == kWarningGroupEverything || (kWarnings[i].groups & mask) != 0) {
      EnableWarning(kWarnings[i].name);
    }
  }
}

void DisableWarningGroup(const char* group) {
  unsigned mask = GroupMask(group);
  for (size_t i = 0; i < sizeof(kWarnings) / sizeof(kWarnings[0]); i++) {
    if (mask == kWarningGroupEverything || (kWarnings[i].groups & mask) != 0) {
      DisableWarning(kWarnings[i].name);
    }
  }
}

void DisableDefaultWarnings(void) {
  for (size_t i = 0; i < sizeof(kWarnings) / sizeof(kWarnings[0]); i++) {
    if (!kWarnings[i].default_enabled) {
      DisableWarning(kWarnings[i].name);
    }
  }
}

void MakeWarningError(const char* warning) {
  SetInsert(&compiler->error_warnings, (void*)warning);
}

void ExemptWarningFromError(const char* warning) {
  SetInsert(&compiler->no_error_warnings, (void*)warning);
}

bool DiagnosticVendorKnowsWarning(DiagnosticVendor vendor, const char* name) {
  // davecc owns all of its diagnostics (and is lenient about unknown names,
  // matching the -W<name> command-line behavior).
  if (vendor == kDiagnosticVendorDavecc) {
    return true;
  }
  const WarningInfo* warning = FindWarning(name);
  if (warning != NULL) {
    return vendor == kDiagnosticVendorClang ? warning->clang : warning->gcc;
  }
  return false;
}

// Comparator for the warning-name sets used by diagnostic-state snapshots.
static int CompareWarningName(const void* a, const void* b) {
  return strcmp(*(const char* const*)a, *(const char* const*)b);
}

// A saved copy of the full warning state for #pragma diagnostic push/pop and
// for per-declaration scoping.  The sets hold the same (stable) name pointers
// as the live sets; the vectors themselves are independent copies.
typedef struct {
  Set disabled;
  Set errors;
  Set no_errors;
  bool werror;
} DiagnosticState;

void* DiagnosticSnapshotState(void) {
  DiagnosticState* s = malloc(sizeof(DiagnosticState));
  SetInit(&s->disabled, CompareWarningName);
  SetInit(&s->errors, CompareWarningName);
  SetInit(&s->no_errors, CompareWarningName);
  SetCopy(&s->disabled, &compiler->disabled_warnings);
  SetCopy(&s->errors, &compiler->error_warnings);
  SetCopy(&s->no_errors, &compiler->no_error_warnings);
  s->werror = compiler->convert_warnings_to_errors;
  return s;
}

void DiagnosticFreeState(void* handle) {
  DiagnosticState* s = handle;
  SetDestruct(&s->disabled);
  SetDestruct(&s->errors);
  SetDestruct(&s->no_errors);
  free(s);
}

// Exchanges the live warning state with the saved one in `handle`.  Calling it
// twice with the same handle leaves both unchanged, which is how a caller
// installs a snapshot for a region and then puts the original state back.
void DiagnosticSwapState(void* handle) {
  DiagnosticState* s = handle;
  Set tmp;
  tmp = compiler->disabled_warnings;
  compiler->disabled_warnings = s->disabled;
  s->disabled = tmp;
  tmp = compiler->error_warnings;
  compiler->error_warnings = s->errors;
  s->errors = tmp;
  tmp = compiler->no_error_warnings;
  compiler->no_error_warnings = s->no_errors;
  s->no_errors = tmp;
  bool tw = compiler->convert_warnings_to_errors;
  compiler->convert_warnings_to_errors = s->werror;
  s->werror = tw;
}

void DiagnosticSuppressBegin(void) {
  compiler->diagnostic_suppression_depth++;
}

void DiagnosticSuppressEnd(void) {
  if (compiler->diagnostic_suppression_depth > 0) {
    compiler->diagnostic_suppression_depth--;
  }
}

bool DiagnosticsSuppressed(void) {
  return compiler->diagnostic_suppression_depth > 0;
}

bool DiagnosticErrorTrapBegin(void) {
  bool saved = compiler->diagnostic_error_trapped;
  compiler->diagnostic_error_trap_depth++;
  compiler->diagnostic_error_trapped = false;
  return saved;
}

void DiagnosticErrorTrapEnd(bool saved) {
  if (compiler->diagnostic_error_trap_depth > 0) {
    compiler->diagnostic_error_trap_depth--;
  }
  // The trapped bit is meaningful only while a trap is active. Never carry a
  // speculative failure into unrelated later analysis after the outermost trap
  // closes; nested traps still restore their parent's state.
  compiler->diagnostic_error_trapped =
      compiler->diagnostic_error_trap_depth > 0 ? saved : false;
}

bool DiagnosticErrorTrapped(void) {
  return compiler->diagnostic_error_trapped;
}

void DiagnosticPush(void) {
  VectorAppend(&compiler->diagnostic_stack, DiagnosticSnapshotState());
}

void DiagnosticPop(void) {
  // A pop without a matching push is ignored (gcc/clang warn; we stay lenient).
  if (compiler->diagnostic_stack.length == 0) {
    return;
  }
  DiagnosticState* s = compiler->diagnostic_stack.value
                           .p[compiler->diagnostic_stack.length - 1];
  compiler->diagnostic_stack.length--;
  // Drop the live vectors (the name pointers they hold are stable and possibly
  // shared with the snapshot, so we only free the vector arrays) and adopt the
  // saved ones.
  SetDestruct(&compiler->disabled_warnings);
  SetDestruct(&compiler->error_warnings);
  SetDestruct(&compiler->no_error_warnings);
  compiler->disabled_warnings = s->disabled;
  compiler->error_warnings = s->errors;
  compiler->no_error_warnings = s->no_errors;
  compiler->convert_warnings_to_errors = s->werror;
  free(s);
}

void DiagnosticIgnore(const char* warning) {
  // Unlike the CLI DisableWarning, a pragma overrides -Wall, so insert directly
  // rather than going through the enable_all_warnings guard.
  if (!SetContains(&compiler->disabled_warnings, (void*)warning)) {
    SetInsert(&compiler->disabled_warnings, strdup(warning));
  }
}

void DiagnosticWarn(const char* warning) {
  // Force plain-warning level: enabled, and not an error even under -Werror.
  SetRemove(&compiler->disabled_warnings, (void*)warning);
  SetRemove(&compiler->error_warnings, (void*)warning);
  if (!SetContains(&compiler->no_error_warnings, (void*)warning)) {
    SetInsert(&compiler->no_error_warnings, strdup(warning));
  }
}

void DiagnosticError(const char* warning) {
  // Enable the warning and promote it to an error.
  SetRemove(&compiler->disabled_warnings, (void*)warning);
  SetRemove(&compiler->no_error_warnings, (void*)warning);
  if (!SetContains(&compiler->error_warnings, (void*)warning)) {
    SetInsert(&compiler->error_warnings, strdup(warning));
  }
}

static bool IsWarningDisabled(const char* warning) {
  if (SetContains(&compiler->disabled_warnings, (void*)warning)) {
    return true;
  }
  const WarningInfo* info = FindWarning(warning);
  if (info != NULL && info->parent != NULL) {
    return SetContains(&compiler->disabled_warnings, (void*)info->parent);
  }
  return false;
}

// Decides whether `warning` should be reported as an error.  A specific
// -Werror=<name> always wins; otherwise the global -Werror applies unless the
// warning was exempted with -Wno-error=<name>.
static bool IsWarningError(const char* warning) {
  if (SetContains(&compiler->error_warnings, (void*)warning)) {
    return true;
  }
  const WarningInfo* info = FindWarning(warning);
  if (info != NULL && info->parent != NULL &&
      SetContains(&compiler->error_warnings, (void*)info->parent)) {
    return true;
  }
  if (info != NULL && info->parent != NULL &&
      SetContains(&compiler->no_error_warnings, (void*)info->parent)) {
    return false;
  }
  return compiler->convert_warnings_to_errors &&
         !SetContains(&compiler->no_error_warnings, (void*)warning);
}

void VReportError(const char* filename, int lineno, const char* error,
                  va_list arg) {
  if (compiler->diagnostic_error_trap_depth > 0) {
    // A speculative parse is in progress; record that an error occurred but do
    // not print or count it.  The caller decides whether to roll back.
    compiler->diagnostic_error_trapped = true;
    return;
  }
  if (DiagnosticsSuppressed()) {
    return;
  }
  char buf[4096];
  vsnprintf(buf, sizeof(buf), error, arg);
  PrintDiagnosticKind("error", ANSI_ERROR);
  fprintf(stderr, ": ");
  PrintDiagnosticLocation(filename, lineno);
  fprintf(stderr, "%s\n", buf);
  compiler->num_errors++;
  if (compiler->num_errors >= compiler->max_errors) {
    fprintf(stderr, "%sToo many errors; terminated%s\n", Color(ANSI_ERROR),
            Color(ANSI_RESET));
    exit(1);
  }
}

void ReportError(const char* filename, int lineno, const char* error, ...) {
  va_list arg;
  va_start(arg, error);
  VReportError(filename, lineno, error, arg);
  va_end(arg);
}

void VReportWarning(const char* filename, int lineno, const char* warn,
                    const char* warning, va_list arg) {
  if (DiagnosticsSuppressed()) {
    return;
  }
  if (IsWarningDisabled(warn)) {
    return;
  }
  char warning_option[64];
  const char* begin_text = "warning";
  snprintf(warning_option, sizeof(warning_option), "-W%s", warn);
  const char* end_text = warning_option;
  bool is_error = IsWarningError(warn);
  if (is_error) {
    begin_text = "error";
    // Show the flag that promoted it: -Werror=<name> for a specific promotion,
    // plain -Werror for the global one.
    if (SetContains(&compiler->error_warnings, (void*)warn)) {
      snprintf(warning_option, sizeof(warning_option), "-Werror=%s", warn);
      end_text = warning_option;
    } else {
      end_text = "-Werror";
    }
  }
  char buf[4096];
  vsnprintf(buf, sizeof(buf), warning, arg);
  PrintDiagnosticKind(begin_text, is_error ? ANSI_ERROR : ANSI_WARNING);
  if (is_error) {
    fprintf(stderr, ": [%s%s%s]: ", Color(ANSI_OPTION), warn,
            Color(ANSI_RESET));
  } else {
    fprintf(stderr, "[%s%s%s]: ", Color(ANSI_OPTION), warn,
            Color(ANSI_RESET));
  }
  PrintDiagnosticLocation(filename, lineno);
  fprintf(stderr, "%s [%s%s%s]\n", buf, Color(ANSI_OPTION), end_text,
          Color(ANSI_RESET));
  if (is_error) {
    compiler->num_errors++;
  }
  if (compiler->num_errors >= compiler->max_errors) {
    fprintf(stderr, "%sToo many errors; terminated%s\n", Color(ANSI_ERROR),
            Color(ANSI_RESET));
    exit(1);
  }
}

void ReportWarning(const char* filename, int lineno, const char* warn,
                   const char* warning, ...) {
  va_list arg;
  va_start(arg, warning);
  VReportWarning(filename, lineno, warn, warning, arg);
  va_end(arg);
}

void VReportNote(const char* filename, int lineno, const char* note,
                 va_list arg) {
  if (DiagnosticsSuppressed()) {
    return;
  }
  char buf[4096];
  vsnprintf(buf, sizeof(buf), note, arg);
  if (filename == NULL) {
    fprintf(stderr, "    ");
    PrintDiagnosticKind("note", ANSI_NOTE);
    fprintf(stderr, ": %s\n", buf);
  } else {
    fprintf(stderr, "    ");
    PrintDiagnosticKind("note", ANSI_NOTE);
    fprintf(stderr, ": ");
    PrintDiagnosticLocation(filename, lineno);
    fprintf(stderr, "%s\n", buf);
  }
}

void ReportNote(const char* filename, int lineno, const char* note, ...) {
  va_list arg;
  va_start(arg, note);
  VReportNote(filename, lineno, note, arg);
  va_end(arg);
}


void FatalError(const char* format, ...) {
  va_list arg;
  va_start(arg, format);
  vfprintf(stderr, format, arg);
  va_end(arg);
  abort();
}
