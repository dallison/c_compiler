//
//  linker_config.c
//  c_compiler
//
//  Created by David Allison on 4/13/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include "linker_config.h"

#include <stdlib.h>
#include <string.h>

#include "elf.h"

static void ConfigRegionDestruct(ConfigRegion* r) {
  StringDestruct(&r->name);
  VectorDestructWithContents(&r->sections,
                             (VectorElementDestructor)StringDestruct,
                             /*free_element=*/true);
}

static void ConfigScriptSymbolDestruct(ConfigScriptSymbol* sym) {
  StringDestruct(&sym->name);
  VectorDestructWithContents(&sym->patterns,
                             (VectorElementDestructor)StringDestruct,
                             /*free_element=*/true);
}

static void ConfigSegmentDestruct(ConfigSegment* s) {
  VectorDestructWithContents(&s->regions,
                             (VectorElementDestructor)ConfigRegionDestruct,
                             /*free_element=*/true);
}

void LinkerConfigInitEmpty(LinkerConfig* config) {
  config->errors = 0;
  VectorInit(&config->segments);
  StringInit(&config->entry_symbol, NULL);
  VectorInit(&config->discard_patterns);
  VectorInit(&config->script_symbols);
}

void LinkerConfigDestruct(LinkerConfig* config) {
  VectorDestructWithContents(&config->segments,
                             (VectorElementDestructor)ConfigSegmentDestruct,
                             /*free_element=*/true);
  StringDestruct(&config->entry_symbol);
  VectorDestructWithContents(&config->discard_patterns,
                             (VectorElementDestructor)StringDestruct,
                             /*free_element=*/true);
  VectorDestructWithContents(&config->script_symbols,
                             (VectorElementDestructor)ConfigScriptSymbolDestruct,
                             /*free_element=*/true);
}

ConfigSegment* LinkerConfigFindSegment(LinkerConfig* config,
                                       ConfigSegmentType type) {
  for (size_t i = 0; i < config->segments.length; i++) {
    ConfigSegment* segment = config->segments.value.p[i];
    if (segment->type == type) {
      return segment;
    }
  }
  return NULL;
}

ConfigSegment* LinkerConfigAddSegment(LinkerConfig* config,
                                      ConfigSegmentType type,
                                      uint64_t alignment) {
  ConfigSegment* existing = LinkerConfigFindSegment(config, type);
  if (existing != NULL) {
    if (alignment > existing->alignment) {
      existing->alignment = alignment;
    }
    return existing;
  }
  ConfigSegment* s = malloc(sizeof(ConfigSegment));
  s->type = type;
  s->alignment = alignment;
  VectorInit(&s->regions);
  VectorAppend(&config->segments, s);
  return s;
}

ConfigRegion* LinkerConfigAddRegion(ConfigSegment* segment, const char* name,
                                    uint64_t start_addr, uint64_t size,
                                    bool falign) {
  ConfigRegion* r = malloc(sizeof(ConfigRegion));
  StringInit(&r->name, name);
  r->start_addr = start_addr;
  r->size = size;
  r->falign = falign;
  r->trailing_align = 0;
  VectorInit(&r->sections);
  VectorAppend(&segment->regions, r);
  return r;
}

void ConfigRegionAddSection(ConfigRegion* region, const char* section) {
  VectorAppend(&region->sections, NewString(section));
}

void LinkerConfigEnsureSpecialSegments(LinkerConfig* config) {
  LinkerConfigAddSegment(config, kConfigSegmentTypeDynamic, 1);
  ConfigSegment* dynamic =
      LinkerConfigFindSegment(config, kConfigSegmentTypeDynamic);
  if (dynamic != NULL && dynamic->regions.length == 0) {
    LinkerConfigAddRegion(dynamic, "dynamic", 0, 0, false);
  }
  LinkerConfigAddSegment(config, kConfigSegmentTypeInterp, 1);
  ConfigSegment* interp =
      LinkerConfigFindSegment(config, kConfigSegmentTypeInterp);
  if (interp != NULL && interp->regions.length == 0) {
    LinkerConfigAddRegion(interp, "interp", 0, 0, false);
  }
}

void LinkerConfigApplyTargetDefaults(LinkerConfig* config,
                                     int elf_machine_type) {
  bool is_6502 = elf_machine_type == ELF_MACHINE_TYPEW65C02;
  uint64_t alignment = is_6502 ? 1 : 0x1000;
  for (size_t i = 0; i < config->segments.length; i++) {
    ConfigSegment* segment = config->segments.value.p[i];
    if (segment->type == kConfigSegmentTypeText ||
        segment->type == kConfigSegmentTypeData) {
      segment->alignment = alignment;
      for (size_t j = 0; j < segment->regions.length; j++) {
        ConfigRegion* region = segment->regions.value.p[j];
        region->falign = !is_6502 && region->start_addr != 0;
      }
    }
  }
}

static bool PatternCharClassMatches(const char* pattern, const char* name,
                                    const char** rest) {
  const char* end = strchr(pattern, ']');
  if (end == NULL) {
    *rest = pattern + 1;
    return name[0] == '[';
  }
  bool negate = pattern[1] == '^' || pattern[1] == '!';
  const char* p = pattern + (negate ? 2 : 1);
  bool found = false;
  while (p < end) {
    if (p + 2 < end && p[1] == '-') {
      if (name[0] >= p[0] && name[0] <= p[2]) {
        found = true;
      }
      p += 3;
    } else {
      if (name[0] == p[0]) {
        found = true;
      }
      p++;
    }
  }
  if (negate) {
    found = !found;
  }
  *rest = end + 1;
  return name[0] != '\0' && found;
}

static bool PatternMatch(const char* pattern, const char* name) {
  while (pattern[0] != '\0') {
    if (pattern[0] == '*') {
      if (pattern[1] == '\0') {
        return true;
      }
      for (const char* s = name;; s++) {
        if (PatternMatch(pattern + 1, s)) {
          return true;
        }
        if (s[0] == '\0') {
          return false;
        }
      }
    }
    if (pattern[0] == '?') {
      if (name[0] == '\0') {
        return false;
      }
      pattern++;
      name++;
      continue;
    }
    if (pattern[0] == '[') {
      const char* rest = NULL;
      if (!PatternCharClassMatches(pattern, name, &rest)) {
        return false;
      }
      pattern = rest;
      name++;
      continue;
    }
    if (name[0] != pattern[0]) {
      return false;
    }
    pattern++;
    name++;
  }
  return name[0] == '\0';
}

bool LinkerConfigPatternMatch(const char* pattern, const char* name) {
  if (pattern == NULL || name == NULL) {
    return false;
  }
  if (strcmp(pattern, "COMMON") == 0) {
    return strcmp(name, "COMMON") == 0 || strcmp(name, ".bss") == 0 ||
           strcmp(name, ".common") == 0;
  }
  return PatternMatch(pattern, name);
}

bool LinkerConfigSectionMatches(const Vector* patterns, const char* name) {
  return LinkerConfigPatternIndex(patterns, name) >= 0;
}

int LinkerConfigPatternIndex(const Vector* patterns, const char* name) {
  if (patterns == NULL || name == NULL) {
    return -1;
  }
  for (size_t i = 0; i < patterns->length; i++) {
    String* pattern = patterns->value.p[i];
    if (LinkerConfigPatternMatch(pattern->value, name)) {
      return (int)i;
    }
  }
  return -1;
}

bool LinkerConfigShouldDiscard(const LinkerConfig* config, const char* name) {
  if (config == NULL) {
    return false;
  }
  return LinkerConfigSectionMatches(&config->discard_patterns, name);
}

struct Segment* LinkerConfigSegmentForSection(LinkerConfig* config,
                                              String* section) {
  (void)config;
  (void)section;
  return NULL;
}
