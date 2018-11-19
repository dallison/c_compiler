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


// Reports an error with varargs (printf style) arguments.
void ReportError(const char* filename, int lineno, const char* error, ...);

// Reports an error with vprintf style arguments.
void VReportError(const char* filename, int lineno, const char* error,
                  va_list arg);

// Reports a warning with varargs (printf style) arguments.
void ReportWarning(const char* filename, int lineno, const char* warn,
                   const char* warning, ...);

// Reports a warning with vprintf style arguments.
void VReportWarning(const char* filename, int lineno, const char* warn,
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

void FatalError(const char* format, ...);

int NumErrors(void);

#endif /* errors_h */
