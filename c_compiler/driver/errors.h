//
//  errors.h
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef errors_h
#define errors_h

#include <stdarg.h>
#include <stdbool.h>


// Reports an error with varargs (printf style) arguments.
void ReportError(const char* filename, int lineno, const char* error, ...);

// Reports an error with vprintf style arguments.
// Returns true when the diagnostic was emitted.
bool VReportError(const char* filename, int lineno, const char* error,
                  va_list arg);

// Reports a warning with varargs (printf style) arguments.
void ReportWarning(const char* filename, int lineno, const char* warn,
                   const char* warning, ...);

// Reports a warning with vprintf style arguments.
// Returns true when the diagnostic was emitted.
bool VReportWarning(const char* filename, int lineno, const char* warn,
                    const char* warning, va_list arg);

void ReportNote(const char* filename, int lineno, const char* error, ...);
void VReportNote(const char* filename, int lineno, const char* error,
                 va_list ap);

// Sets the max errors.
void SetMaxErrors(int n);

// Disables the given warning.
void DisableWarning(const char* warning);

// Enables the given warning.
void EnableWarning(const char* warning);

bool WarningIsEnabled(const char* warning);

// Returns whether a warning or warning group is known to this compiler.
bool WarningExists(const char* warning);
bool WarningGroupExists(const char* group);

// Enables/disables all warnings in a named group such as "all" or "extra".
void EnableWarningGroup(const char* group);
void DisableWarningGroup(const char* group);

// Disables warnings that are known but not enabled by default.  Called after
// the warning sets have been initialized and before command-line -W options are
// applied.
void DisableDefaultWarnings(void);

// Promotes the given warning to an error (-Werror=<name>).
void MakeWarningError(const char* warning);

// Exempts the given warning from -Werror (-Wno-error=<name>).
void ExemptWarningFromError(const char* warning);

// Vendor namespace for #pragma <vendor> diagnostic ... directives.
typedef enum {
  kDiagnosticVendorDavecc,  // #pragma davecc diagnostic: all davecc diagnostics.
  kDiagnosticVendorClang,   // #pragma clang diagnostic: clang-supported only.
  kDiagnosticVendorGcc,     // #pragma GCC diagnostic: gcc-supported only.
} DiagnosticVendor;

// Saves/restores the whole diagnostic state for #pragma diagnostic push/pop.
void DiagnosticPush(void);
void DiagnosticPop(void);

// Captures the current diagnostic state, swaps the live state with a saved
// snapshot, and frees a snapshot.  Used to scope diagnostics to a single
// declaration: parsing consumes a lookahead token that may process a trailing
// "#pragma diagnostic pop" before the declaration is analyzed and code-genned,
// so the snapshot taken before parsing is reinstalled around those phases.
void* DiagnosticSnapshotState(void);
void DiagnosticSwapState(void* snapshot);
void DiagnosticFreeState(void* snapshot);
void DiagnosticSuppressBegin(void);
void DiagnosticSuppressEnd(void);
bool DiagnosticsSuppressed(void);

// Tentative-parse error trap.  While a trap is active, reported errors are
// swallowed (neither printed nor counted toward num_errors) and recorded via a
// sticky flag.  Use to speculatively parse a construct and roll it back on
// failure without emitting diagnostics.  Traps nest; each Begin clears the flag
// for the new region, and End restores the enclosing region's flag.  The
// returned value from Begin must be passed to the matching End.
bool DiagnosticErrorTrapBegin(void);
void DiagnosticErrorTrapEnd(bool saved);
bool DiagnosticErrorTrapped(void);

// #pragma diagnostic {ignored,warning,error} "-W<name>" actions.
void DiagnosticIgnore(const char* warning);
void DiagnosticWarn(const char* warning);
void DiagnosticError(const char* warning);

// Returns whether `vendor` recognizes the warning `name`.  davecc recognizes
// every davecc diagnostic; clang and gcc only recognize the warnings those
// compilers actually provide, so vendor-specific pragmas naming a diagnostic
// the vendor lacks are ignored.
bool DiagnosticVendorKnowsWarning(DiagnosticVendor vendor, const char* name);

void FatalError(const char* format, ...);

int NumErrors(void);

#endif /* errors_h */
