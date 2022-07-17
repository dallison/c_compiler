//
//  linker_config.c
//  c_compiler
//
//  Created by David Allison on 4/13/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include "linker_config.h"
#include "linker.h"
#include <stdarg.h>
#include <stdlib.h>
#include "vector.h"

static void ConfigError(LinkerConfig* config, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  config->errors++;
}

static void ConfigRegionDestruct(ConfigRegion* r) {
  StringDestruct(&r->name);
  VectorDestructWithContents(&r->sections, (VectorElementDestructor)StringDestruct);
}

static ConfigRegion* NewConfigRegion(LinkerConfig* config, ConfigObject* region) {
  ConfigRegion* r = malloc(sizeof(ConfigRegion));
  StringInit(&r->name, NULL);
  r->start_addr = 0;
  r->size = 0;
  r->falign = false;
  VectorInit(&r->sections);
  
  ConfigNode* name = ConfigObjectFind(region, "name");
  if (name == NULL) {
    ConfigError(config, "Missing region name");
  } else {
    StringInit(&r->name, name->value.string_value.value);
  }
  ConfigNode* start_addr = ConfigObjectFind(region, "start_addr");
  if (start_addr != NULL) {
    r->start_addr = start_addr->value.int_value;
  }
  ConfigNode* size = ConfigObjectFind(region, "size");
  if (size != NULL) {
    r->size = size->value.int_value;
  }
  ConfigNode* falign = ConfigObjectFind(region, "falign");
  if (falign != NULL) {
    r->falign = (bool)falign->value.int_value;
  }
  // Parse the section names. These are inserted as pointers to String
  // objects in a vector.
  ConfigNode* sections = ConfigObjectFind(region, "section");
  if (sections != NULL) {
    ConfigNodeVectorize(sections);
    for (size_t i = 0; i < sections->value.vector_value.length; i++) {
      ConfigNode* section_name = sections->value.vector_value.value.p[i];
      VectorAppend(&r->sections, NewString(section_name->value.string_value.value));
    }
  }
  return r;
}

static void ConfigSegmentDestruct(ConfigSegment* s) {
  VectorDestructWithContents(&s->regions, (VectorElementDestructor)ConfigRegionDestruct);
}

static ConfigSegment* NewConfigSegment(LinkerConfig* config, ConfigObject* segment) {
  ConfigSegment* s = malloc(sizeof(ConfigSegment));
  VectorInit(&s->regions);
  ConfigNode* type = ConfigObjectFind(segment, "type");
  s->type = kConfigSegmentTypeUnknown;
  if (type == NULL) {
    LinkerError(NULL, "Missing type for segment");
  } else {
    s->type = (ConfigSegmentType)type->value.int_value;
  }
  ConfigNode* alignment = ConfigObjectFind(segment, "alignment");
  if (alignment != NULL) {
    s->alignment = alignment->value.int_value;
  }
  ConfigNode* regions = ConfigObjectFind(segment, "region");
  if (regions != NULL) {
    ConfigNodeVectorize(regions);
    for (size_t i = 0; i < regions->value.vector_value.length; i++) {
      ConfigNode* region = regions->value.vector_value.value.p[i];
      VectorAppend(&s->regions, NewConfigRegion(config, region->value.object_value));
    }

  }
  return s;
}


static void CreateSegmentsFromConfig(MapKeyValue* kv, void* data) {
  LinkerConfig* config = data;
  if (!StringEqual(kv->key.p, "segment")) {
    return;
  }
  ConfigNode* value_node = kv->value.p;
  // kv->value is a pointer to a ConfigNode whose value is a ConfigObject.
  ConfigNodeVectorize(value_node);
  for (size_t i = 0; i < value_node->value.vector_value.length; i++) {
    ConfigNode* seg_node = value_node->value.vector_value.value.p[i];
    ConfigObject* seg = seg_node->value.object_value;
    ConfigSegment* segment = NewConfigSegment(config, seg);
    VectorAppend(&config->segments, segment);
  }
}

// Passed a ConfigObject that contains the segment configs.
void LinkerConfigInit(LinkerConfig* config, ConfigObject* layout) {
  config->layout = layout;
  config->errors = 0;
  VectorInit(&config->segments);
  
  MapTraverse(&layout->nodes, CreateSegmentsFromConfig, config);
}

void LinkerConfigDestruct(LinkerConfig* config) {
  VectorDestructWithContents(&config->segments, (VectorElementDestructor)ConfigSegmentDestruct);
}

struct Segment* LinkerConfigSegmentForSection(LinkerConfig* config, String* section) {
  return NULL;
}
