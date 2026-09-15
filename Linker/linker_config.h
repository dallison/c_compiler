//
//  linker_config.h
//  c_compiler
//
//  Created by David Allison on 4/13/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#ifndef linker_config_h
#define linker_config_h

#include <stdbool.h>
#include <stdint.h>

#include "dstring.h"
#include "vector.h"

struct Segment;

typedef struct {
  String name;
  uint64_t start_addr;
  uint64_t size;
  Vector sections;      // Section-name glob patterns (* ? []).
  bool falign;
  uint64_t trailing_align;  // From `. = ALIGN(n)` at the end of a region.
} ConfigRegion;

// A symbol assignment from the linker script (`_etext = .`, PROVIDE(...)).
typedef struct {
  String name;
  bool provide;
  bool image_end;
  bool has_absolute;
  uint64_t absolute;
  uint64_t align;
  Vector patterns;      // Section globs whose end address defines `.`.
} ConfigScriptSymbol;

typedef enum {
  kConfigSegmentTypeUnknown,
  kConfigSegmentTypeText,
  kConfigSegmentTypeData,
  kConfigSegmentTypeDynamic,
  kConfigSegmentTypeInterp,
} ConfigSegmentType;

typedef struct {
  ConfigSegmentType type;
  uint64_t alignment;
  Vector regions;
} ConfigSegment;

typedef struct {
  int errors;
  Vector segments;          // Vector of ConfigSegment*.
  String entry_symbol;      // From ENTRY() in a linker script, or empty.
  Vector discard_patterns;  // Section globs from /DISCARD/.
  Vector script_symbols;    // ConfigScriptSymbol*.
} LinkerConfig;

void LinkerConfigInitEmpty(LinkerConfig* config);
void LinkerConfigDestruct(LinkerConfig* config);

ConfigSegment* LinkerConfigFindSegment(LinkerConfig* config,
                                       ConfigSegmentType type);
ConfigSegment* LinkerConfigAddSegment(LinkerConfig* config,
                                      ConfigSegmentType type,
                                      uint64_t alignment);
ConfigRegion* LinkerConfigAddRegion(ConfigSegment* segment, const char* name,
                                    uint64_t start_addr, uint64_t size,
                                    bool falign);
void ConfigRegionAddSection(ConfigRegion* region, const char* section);

void LinkerConfigApplyTargetDefaults(LinkerConfig* config, int elf_machine_type);
void LinkerConfigEnsureSpecialSegments(LinkerConfig* config);

// GNU ld / lld style glob: `*` any run, `?` one character, `[abc]` / `[a-z]`.
bool LinkerConfigPatternMatch(const char* pattern, const char* name);
bool LinkerConfigSectionMatches(const Vector* patterns, const char* name);
bool LinkerConfigShouldDiscard(const LinkerConfig* config, const char* name);
int LinkerConfigPatternIndex(const Vector* patterns, const char* name);

struct Segment* LinkerConfigSegmentForSection(LinkerConfig* config,
                                              String* section);
#endif /* linker_config_h */
