#include "macro_expansion.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"
#include "map.h"
#include "vector.h"

typedef struct LineExpansionSpan {
  size_t start;
  size_t end;
  MacroExpansion* expansion;
} LineExpansionSpan;

typedef struct FileLineExpansion {
  int fileno;
  int lineno;
  MacroExpansion* expansion;
} FileLineExpansion;

static Vector expansions;
static Map location_map;
static Vector line_spans;
static bool line_spans_ordered = true;
static Vector file_line_expansions;
static SourceLocation last_noted_location;
static bool initialized = false;

static void EnsureInitialized(void) {
  if (initialized) {
    return;
  }
  VectorInit(&expansions);
  MapInitForInt64Keys(&location_map);
  VectorInit(&line_spans);
  VectorInit(&file_line_expansions);
  last_noted_location = SOURCE_LOCATION_MISSING;
  initialized = true;
}

static void FreeExpansion(void* value) {
  MacroExpansion* expansion = value;
  free((void*)expansion->name);
}

static void FreeSpan(void* value) {
  (void)value;
}

void MacroExpansionClear(void) {
  if (!initialized) {
    return;
  }
  VectorClearWithContents(&expansions, FreeExpansion, true);
  MapClear(&location_map);
  VectorClearWithContents(&line_spans, FreeSpan, true);
  line_spans_ordered = true;
  VectorClearWithContents(&file_line_expansions, FreeSpan, true);
  last_noted_location = SOURCE_LOCATION_MISSING;
}

MacroExpansion* NewMacroExpansion(const char* name, SourceLocation definition,
                                  SourceLocation invocation,
                                  MacroExpansion* parent) {
  EnsureInitialized();
  MacroExpansion* expansion = malloc(sizeof(MacroExpansion));
  expansion->name = strdup(name != NULL ? name : "");
  expansion->definition = definition;
  expansion->invocation = invocation;
  expansion->parent = parent;
  expansion->remembered_fileno = -1;
  expansion->remembered_lineno = -1;
  VectorAppend(&expansions, expansion);
  return expansion;
}

void MacroExpansionRemember(SourceLocation location,
                            MacroExpansion* expansion) {
  if (expansion == NULL || location == SOURCE_LOCATION_MISSING ||
      location == SOURCE_LOCATION_COMMAND_LINE) {
    return;
  }
  EnsureInitialized();
  MapKeyValue kv;
  kv.key.w = (int64_t)location;
  kv.value.p = expansion;
  MapInsert(&location_map, kv);

  int fileno = 0;
  int lineno = 0;
  int colno = 0;
  SourceLocationNumbers(location, &fileno, &lineno, &colno);
  if (lineno > 0) {
    // An expansion's tokens arrive together on one line.  A repeat that this
    // misses only duplicates an entry; the notes skip repeated expansions.
    if (expansion->remembered_fileno == fileno &&
        expansion->remembered_lineno == lineno) {
      return;
    }
    expansion->remembered_fileno = fileno;
    expansion->remembered_lineno = lineno;
    FileLineExpansion* entry = malloc(sizeof(FileLineExpansion));
    entry->fileno = fileno;
    entry->lineno = lineno;
    entry->expansion = expansion;
    VectorAppend(&file_line_expansions, entry);
  }
}

MacroExpansion* MacroExpansionForLocation(SourceLocation location) {
  if (!initialized || location == SOURCE_LOCATION_MISSING ||
      location == SOURCE_LOCATION_COMMAND_LINE) {
    return NULL;
  }
  return (MacroExpansion*)MapFindInt64Key(&location_map, (int64_t)location);
}

void MacroExpansionBeginLine(void) {
  EnsureInitialized();
  VectorClearWithContents(&line_spans, FreeSpan, true);
  line_spans_ordered = true;
}

void MacroExpansionAddLineSpan(size_t start, size_t end,
                               MacroExpansion* expansion) {
  if (expansion == NULL || end <= start) {
    return;
  }
  EnsureInitialized();
  if (line_spans.length > 0) {
    LineExpansionSpan* last = line_spans.value.p[line_spans.length - 1];
    if (start < last->end) {
      line_spans_ordered = false;
    }
  }
  LineExpansionSpan* span = malloc(sizeof(LineExpansionSpan));
  span->start = start;
  span->end = end;
  span->expansion = expansion;
  VectorAppend(&line_spans, span);
}

MacroExpansion* MacroExpansionAtLineOffset(size_t offset) {
  if (!initialized) {
    return NULL;
  }
  // Detokenizing appends one span per token, so the spans are disjoint and
  // ascending; at most one contains `offset`.
  if (line_spans_ordered) {
    size_t low = 0;
    size_t high = line_spans.length;
    while (low < high) {
      size_t mid = low + (high - low) / 2;
      LineExpansionSpan* span = line_spans.value.p[mid];
      if (offset < span->start) {
        high = mid;
      } else if (offset >= span->end) {
        low = mid + 1;
      } else {
        return span->expansion;
      }
    }
    return NULL;
  }
  for (size_t i = 0; i < line_spans.length; i++) {
    LineExpansionSpan* span = line_spans.value.p[i];
    if (offset >= span->start && offset < span->end) {
      return span->expansion;
    }
  }
  return NULL;
}

static bool LocationIsPrintable(SourceLocation location) {
  return location != SOURCE_LOCATION_MISSING &&
         location != SOURCE_LOCATION_COMMAND_LINE;
}

static void PrintCaretSnippet(SourceLocation location) {
  const char* filename = NULL;
  int lineno = 0;
  int start = 0;
  int end = 0;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);
  if (filename == NULL || lineno <= 0) {
    return;
  }
  int fileno = -1;
  int decoded_line = 0;
  int colno = 0;
  SourceLocationNumbers(location, &fileno, &decoded_line, &colno);
  File* file = fileno >= 0 ? SourceFileAt((size_t)fileno) : NULL;
  const char* text = file != NULL ? SourceFileLineText(file, lineno) : NULL;
  if (text == NULL) {
    return;
  }
  fprintf(stderr, "%s\n", text);
  if (start < 0) {
    start = 0;
  }
  int caret_end = end;
  if (caret_end <= start) {
    caret_end = start + 1;
  }
  for (int i = 0; i < start; i++) {
    fputc(text[i] == '\t' ? '\t' : ' ', stderr);
  }
  fputc('^', stderr);
  for (int i = start + 1; i < caret_end && text[i] != '\0'; i++) {
    fputc('~', stderr);
  }
  fputc('\n', stderr);
}

static void EmitExpansionChain(MacroExpansion* expansion,
                               MacroExpansion** seen, size_t* seen_count) {
  MacroExpansion* chain[32];
  size_t depth = 0;
  for (MacroExpansion* step = expansion; step != NULL && depth < 32;
       step = step->parent) {
    chain[depth++] = step;
  }

  for (size_t i = depth; i > 0; i--) {
    MacroExpansion* step = chain[i - 1];
    if (step->name == NULL || step->name[0] == '\0' ||
        !LocationIsPrintable(step->definition)) {
      continue;
    }
    bool already = false;
    for (size_t j = 0; j < *seen_count; j++) {
      if (seen[j] == step) {
        already = true;
        break;
      }
    }
    if (already) {
      continue;
    }
    if (*seen_count < 32) {
      seen[(*seen_count)++] = step;
    }
    const char* filename = NULL;
    int lineno = 0;
    int start = 0;
    int end = 0;
    DecodeSourceLocation(step->definition, &filename, &lineno, &start, &end);
    ReportNote(filename, lineno, "expanded from macro '%s'", step->name);
    PrintCaretSnippet(step->definition);
  }
}

void ReportMacroExpansionNotes(SourceLocation location) {
  if (location == last_noted_location &&
      last_noted_location != SOURCE_LOCATION_MISSING) {
    return;
  }
  last_noted_location = location;

  MacroExpansion* seen[32];
  size_t seen_count = 0;
  MacroExpansion* expansion = MacroExpansionForLocation(location);
  if (expansion != NULL) {
    EmitExpansionChain(expansion, seen, &seen_count);
    return;
  }

  int fileno = 0;
  int lineno = 0;
  int colno = 0;
  SourceLocationNumbers(location, &fileno, &lineno, &colno);
  if (lineno <= 0) {
    return;
  }
  for (size_t i = 0; i < file_line_expansions.length; i++) {
    FileLineExpansion* entry = file_line_expansions.value.p[i];
    if (entry->fileno == fileno && entry->lineno == lineno) {
      EmitExpansionChain(entry->expansion, seen, &seen_count);
    }
  }
}
