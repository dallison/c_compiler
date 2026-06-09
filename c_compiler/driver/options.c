//
//  options.c
//  c_compiler_library
//
//  Created by David Allison on 2/16/23.
//  Copyright © 2023 David Allison. All rights reserved.
//

#include "options.h"
#include "vector.h"
#include "dstring.h"
#include <stdlib.h>
#include <stdio.h>

CompilerOptionString* NewOptionString(const char* name) {
  CompilerOptionString* s = malloc(sizeof(CompilerOptionString));
  StringInit(&s->name, name);
  StringInit(&s->value, NULL);
  return s;
}

CompilerOptionString* NewOptionStringWithValue(const char* name, size_t namelen, const char* value) {
  CompilerOptionString* s = malloc(sizeof(CompilerOptionString));
  StringInit(&s->name, NULL);
  StringAppendSegment(&s->name, name, namelen);
  StringInit(&s->value, value);
  return s;
}

void CompilerOptionStringDelete(CompilerOptionString* c) {
  StringDestruct(&c->name);
  StringDestruct(&c->value);
  free(c);
}

// Parse the option in strings[*index] into options.  Returns true if option
// is valid.
static bool ParseOption(CompilerOptionDefinition* compiler_options,
                       Vector* strings, int *index, Vector* options) {
  int i = *index;
  CompilerOptionString* s = strings->value.p[i];
  bool found = false;
  for (size_t opt = 0; compiler_options[opt].name != NULL; opt++) {
    if (compiler_options[opt].is_prefix &&
        s->name.value[1] == compiler_options[opt].name[1]) {
      // Prefix option takes name from index 2 onwards.
      CompilerOptionValue* o = calloc(sizeof(CompilerOptionValue), 1);
      o->opt = compiler_options[opt].opt;
      // Prefixed options are always a string.
      StringInit(&o->value.svalue, &s->name.value[2]);
      VectorAppend(options, o);
      found = true;
      break;
    } else if (StringEqual(&s->name, compiler_options[opt].name)) {
      CompilerOptionValue* o = calloc(sizeof(CompilerOptionValue), 1);
      o->opt = compiler_options[opt].opt;
      switch (compiler_options[opt].type) {
        case kCompilerOptionString:
          if (s->value.length == 0) {
            // Value is in next arg.
            i++;
            if (i < strings->length) {
              s = strings->value.p[i];
              StringInit(&o->value.svalue, s->name.value);
            } else {
              fprintf(stderr, "Missing value for %s\n", s->name.value);
              exit(1);
            }
          } else {
            StringInit(&o->value.svalue, s->value.value);
          }
          break;
        case kCompilerOptionBool:
          o->value.bvalue = true;
          break;
        case kCompilerOptionInt: {
          bool value_ok = false;
          char* end;
          if (s->value.length == 0) {
            // Value is in next arg.
            i++;
            if (i < strings->length) {
              CompilerOptionString *vs = strings->value.p[i];
              o->value.ivalue = (int)strtol(vs->name.value, &end, 0);
              value_ok = end != vs->name.value;
            } else {
              fprintf(stderr, "Missing value for %s\n", s->name.value);
              exit(1);
            }
          } else {
            o->value.ivalue = (int)strtol(s->value.value, &end, 0);
            value_ok = end != s->value.value;
          }
          if (!value_ok) {
            fprintf(stderr, "Invalid integer value for %s\n", s->name.value);
            exit(1);
         }
         break;
        }
      }
      VectorAppend(options, o);
      found = true;
      break;
    }
  }
  *index = i;
  return found;
}

Vector* ParseOptionSet(CompilerOptionDefinition* compiler_options,
                       Vector* strings, Vector* options) {
  Vector* unused_strings = NULL;
  for (int i = 0; i < strings->length; i++) {
    int start = i;
    CompilerOptionString* s = strings->value.p[i];
    bool option_ok = false;
    if (s->name.value[0] == '-') {
      option_ok = ParseOption(compiler_options, strings, &i, options);
    } else {
      CompilerOptionValue* o = calloc(sizeof(CompilerOptionValue), 1);
      o->opt = kOptionInputFile;
      StringInit(&o->value.svalue, s->name.value);
      VectorAppend(options, o);
      option_ok = true;
    }
    if (option_ok) {
      // ParseOption may have consumed a following value arg (advancing i); free
      // every option string it consumed, not just the first.
      for (int k = start; k <= i; k++) {
        CompilerOptionStringDelete((CompilerOptionString*)strings->value.p[k]);
      }
    } else {
      if (unused_strings == NULL) {
        unused_strings = NewVector();
      }
      VectorAppend(unused_strings, s);      // Takes ownership.
      strings->value.p[i] = NULL;
    }
  }
  // All strings in parts have been moved to other vectors.
  VectorDestruct(strings);
  return unused_strings;
}

String* OptionStringValue(CompilerOption option, Vector* options) {
  if (options == NULL) {
    return NULL;
  }
  for (size_t i = options->length; i > 0; i--) {
    CompilerOptionValue* opt = options->value.p[i - 1];
    if (opt->opt == option) {
      return &opt->value.svalue;
    }
  }
  return NULL;
}

int OptionIntValue(CompilerOption option, Vector* options, int def) {
  if (options == NULL) {
    return def;
  }
  for (size_t i = options->length; i > 0; i--) {
    CompilerOptionValue* opt = options->value.p[i - 1];
    if (opt->opt == option) {
      return opt->value.ivalue;
    }
  }
  return def;
}

bool OptionBoolValue(CompilerOption option, Vector* options, bool def) {
  if (options == NULL) {
    return def;
  }
  for (size_t i = options->length; i > 0; i--) {
    CompilerOptionValue* opt = options->value.p[i - 1];
    if (opt->opt == option) {
      return opt->value.bvalue;
    }
  }
  return def;
}

void PrintAllOptions(CompilerOptionDefinition* options) {
  if (options == NULL) {
    return;
  }
  for (size_t i = 0; options[i].name != NULL; i++) {
    printf("  %-20s %s\n", options[i].name, options[i].help);
  }
}


