#include "macro_expansion.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"
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

typedef struct LocationExpansion {
  SourceLocation location;
  MacroExpansion* expansion;  // NULL marks an empty slot.
} LocationExpansion;

static Vector expansions;
// The expansion each token location came from, hashed by location.  Every
// token of every expansion is recorded, so this must not shift on insert.
static LocationExpansion* location_table;
static size_t location_capacity;
static size_t location_count;
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
  free(location_table);
  location_table = NULL;
  location_capacity = 0;
  location_count = 0;
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

static size_t LocationSlot(SourceLocation location, size_t capacity) {
  uint64_t h = location;
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdull;
  h ^= h >> 33;
  size_t mask = capacity - 1;
  size_t slot = (size_t)h & mask;
  while (location_table[slot].expansion != NULL &&
         location_table[slot].location != location) {
    slot = (slot + 1) & mask;
  }
  return slot;
}

static void GrowLocationTable(void) {
  LocationExpansion* old = location_table;
  size_t old_capacity = location_capacity;
  location_capacity = old_capacity ? old_capacity * 2 : 1024;
  location_table = calloc(location_capacity, sizeof(LocationExpansion));
  for (size_t i = 0; i < old_capacity; i++) {
    if (old[i].expansion != NULL) {
      location_table[LocationSlot(old[i].location, location_capacity)] =
          old[i];
    }
  }
  free(old);
}

void MacroExpansionRemember(SourceLocation location,
                            MacroExpansion* expansion) {
  if (expansion == NULL || location == SOURCE_LOCATION_MISSING ||
      location == SOURCE_LOCATION_COMMAND_LINE) {
    return;
  }
  EnsureInitialized();
  if ((location_count + 1) * 2 > location_capacity) {
    GrowLocationTable();
  }
  LocationExpansion* entry =
      &location_table[LocationSlot(location, location_capacity)];
  if (entry->expansion == NULL) {
    location_count++;
  }
  entry->location = location;
  entry->expansion = expansion;

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
    FileLineExpansion* line_entry = malloc(sizeof(FileLineExpansion));
    line_entry->fileno = fileno;
    line_entry->lineno = lineno;
    line_entry->expansion = expansion;
    VectorAppend(&file_line_expansions, line_entry);
  }
}

MacroExpansion* MacroExpansionForLocation(SourceLocation location) {
  if (!initialized || location == SOURCE_LOCATION_MISSING ||
      location == SOURCE_LOCATION_COMMAND_LINE) {
    return NULL;
  }
  if (location_capacity == 0) {
    return NULL;
  }
  return location_table[LocationSlot(location, location_capacity)].expansion;
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
    ReportNoteAtLocation(step->definition, "expanded from macro '%s'",
                         step->name);
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
