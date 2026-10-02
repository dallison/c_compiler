#ifndef macro_expansion_h
#define macro_expansion_h

#include "source.h"

// One step in a macro expansion chain.  `parent` is the enclosing expansion
// (NULL at the outermost invocation).  Nodes are interned for the current
// compilation and must not be freed by callers.
typedef struct MacroExpansion {
  const char* name;
  SourceLocation definition;
  SourceLocation invocation;
  struct MacroExpansion* parent;
  // The file and line last recorded for this expansion's tokens.
  int remembered_fileno;
  int remembered_lineno;
} MacroExpansion;

void MacroExpansionClear(void);

MacroExpansion* NewMacroExpansion(const char* name, SourceLocation definition,
                                  SourceLocation invocation,
                                  MacroExpansion* parent);

// Remember that `location` (a token after expansion) came from `expansion`.
void MacroExpansionRemember(SourceLocation location, MacroExpansion* expansion);
MacroExpansion* MacroExpansionForLocation(SourceLocation location);

// Spans in the current expanded source line, used while lexing that line.
void MacroExpansionBeginLine(void);
void MacroExpansionAddLineSpan(size_t start, size_t end,
                               MacroExpansion* expansion);
MacroExpansion* MacroExpansionAtLineOffset(size_t offset);

// After a diagnostic at `location`, emit Clang-style
// "expanded from macro 'NAME'" notes walking the expansion chain.
void ReportMacroExpansionNotes(SourceLocation location);

#endif
