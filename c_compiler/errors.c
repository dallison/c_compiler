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

static bool IsWarningDisabled(const char* warning) {
  return
      SetContains(&compiler->disabled_warnings, (void*)warning);
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
  if (compiler->convert_warnings_to_errors) {
    begin_text = "error";
    end_text = "-Werror";
  }
  char buf[4096];
  vsnprintf(buf, sizeof(buf), warning, arg);
  if (lineno == 0) {
    fprintf(stderr, "%s[%s]: %s: %s [%s]\n", begin_text, warn, filename, buf, end_text);
  } else {
    fprintf(stderr, "%s[%s]: %s:%d: %s [%s]\n", begin_text, warn, filename, lineno, buf, end_text);
  }
  compiler->num_errors++;
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
