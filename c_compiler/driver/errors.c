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
#include "set.h"
#include "compiler.h"

// Error and warnings.

int NumErrors() { return compiler->num_errors; }

void SetMaxErrors(int n) { compiler->max_errors = n; }

void DisableWarning(const char* warning) {
  if (compiler->enable_all_warnings) {
    return;
  }
  SetInsert(&compiler->disabled_warnings, (void*)warning);
}

void EnableWarning(const char* warning) {
  SetRemove(&compiler->disabled_warnings, (void*)warning);
}

void MakeWarningError(const char* warning) {
  SetInsert(&compiler->error_warnings, (void*)warning);
}

void ExemptWarningFromError(const char* warning) {
  SetInsert(&compiler->no_error_warnings, (void*)warning);
}

// Which of davecc's diagnostics are also recognized (by the same -W spelling)
// by clang and/or gcc.  Used to gate #pragma clang/GCC diagnostic so that, for
// example, "#pragma GCC diagnostic ignored \"-Wmacro-redefined\"" is ignored
// (that spelling is clang-only) while "#pragma davecc ..." still honors it.
static const struct {
  const char* name;
  bool clang;
  bool gcc;
} kWarningVendors[] = {
    {"main", true, true},
    {"extern-initializer", true, false},
    {"attributes", true, true},
    {"duplicate-decl-specifier", true, true},
    {"implicit-function-declaration", true, true},
    {"unused-variable", true, true},
    {"macro-redefined", true, false},
    {"extra-tokens", true, false},
    {"return-type", true, true},
    {"unused-result", true, true},
    {"format", true, true},
    {"switch", true, true},
    {"uninitialized", true, true},
    {"conversion", true, true},
    {"incompatible-pointer-types", true, true},
    {"deprecated-declarations", true, true},
    {"preprocessor", false, false},
    {"pointer-types", false, false},
};

bool DiagnosticVendorKnowsWarning(DiagnosticVendor vendor, const char* name) {
  // davecc owns all of its diagnostics (and is lenient about unknown names,
  // matching the -W<name> command-line behavior).
  if (vendor == kDiagnosticVendorDavecc) {
    return true;
  }
  for (size_t i = 0; i < sizeof(kWarningVendors) / sizeof(kWarningVendors[0]);
       i++) {
    if (strcmp(kWarningVendors[i].name, name) == 0) {
      return vendor == kDiagnosticVendorClang ? kWarningVendors[i].clang
                                              : kWarningVendors[i].gcc;
    }
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
  return
      SetContains(&compiler->disabled_warnings, (void*)warning);
}

// Decides whether `warning` should be reported as an error.  A specific
// -Werror=<name> always wins; otherwise the global -Werror applies unless the
// warning was exempted with -Wno-error=<name>.
static bool IsWarningError(const char* warning) {
  if (SetContains(&compiler->error_warnings, (void*)warning)) {
    return true;
  }
  return compiler->convert_warnings_to_errors &&
         !SetContains(&compiler->no_error_warnings, (void*)warning);
}

void VReportError(const char* filename, int lineno, const char* error,
                  va_list arg) {
  char buf[4096];
  vsnprintf(buf, sizeof(buf), error, arg);
  if (lineno == 0) {
    fprintf(stderr, "error: %s: %s\n", filename, buf);
  } else {
    fprintf(stderr, "error: %s:%d: %s\n", filename, lineno, buf);
  }
  compiler->num_errors++;
  if (compiler->num_errors >= compiler->max_errors) {
    fprintf(stderr, "Too many errors; terminated\n");
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
  if (lineno == 0) {
    fprintf(stderr, "%s[%s]: %s: %s [%s]\n", begin_text, warn, filename, buf, end_text);
  } else {
    fprintf(stderr, "%s[%s]: %s:%d: %s [%s]\n", begin_text, warn, filename, lineno, buf, end_text);
  }
  if (is_error) {
    compiler->num_errors++;
  }
  if (compiler->num_errors >= compiler->max_errors) {
    fprintf(stderr, "Too many errors; terminated\n");
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
  char buf[4096];
  vsnprintf(buf, sizeof(buf), note, arg);
  if (filename == NULL) {
    fprintf(stderr, "    note: %s\n", buf);
  } else if (lineno == 0) {
    fprintf(stderr, "    note: %s: %s\n", filename, buf);
  } else {
    fprintf(stderr, "    note: %s:%d: %s\n", filename, lineno, buf);
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
