//
//  linker_config.h
//  c_compiler
//
//  Created by David Allison on 4/13/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#ifndef linker_config_h
#define linker_config_h

#include "config.h"
#include "vector.h"

struct Segment;

typedef struct {
  String name;
  uint64_t start_addr;
  uint64_t size;
  uint64_t alignment;
  Vector sections;
} ConfigRegion;

typedef enum  {
  kConfigSegmentTypeUnknown,
  kConfigSegmentTypeText,
  kConfigSegmentTypeData,
  kConfigSegmentTypeDynamic,
  kConfigSegmentTypeInterp,
} ConfigSegmentType;

typedef struct {
  ConfigSegmentType type;
  Vector regions;
} ConfigSegment;

typedef struct {
  ConfigObject *layout;
  int errors;
  Vector segments;      // Vector of ConfigSegment*.
} LinkerConfig;

void LinkerConfigInit(LinkerConfig* config, ConfigObject* layout);
void LinkerConfigDestruct(LinkerConfig* config);

struct Segment* LinkerConfigSegmentForSection(LinkerConfig* config, String* section);
#endif /* linker_config_h */
