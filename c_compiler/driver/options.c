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
#include <string.h>

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

// Does a prefix option written bare take its value from the following argument?
// GCC and Clang accept both spellings for these ("-Idir" and "-I dir").  Read as
// an ordinary prefix option a bare one carries an empty value and the argument
// after it is taken for an input file, so the path or macro is dropped without a
// diagnostic.  The other prefix options are excluded because a bare spelling is
// itself meaningful for them, "-O" selecting a default optimization level, and so
// must not consume whatever follows it.
static bool PrefixOptionTakesNextArgument(CompilerOption opt) {
  return opt == kOptionIncludePath || opt == kOptionDefineMacro ||
         opt == kOptionUndefineMacro;
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
      CompilerOptionValue* o = calloc(sizeof(CompilerOptionValue), 1);
      o->opt = compiler_options[opt].opt;
      if (s->name.length == 2 && s->value.length == 0 &&
          PrefixOptionTakesNextArgument(o->opt)) {
        i++;
        if (i >= strings->length) {
          fprintf(stderr, "Missing value for %s\n", s->name.value);
          exit(1);
        }
        // A separated value keeps any '=' it contains: ParseOptions only splits
        // arguments that begin with '-', so "-D" "foo=bar" arrives whole.
        CompilerOptionString* vs = strings->value.p[i];
        StringInit(&o->value.svalue, vs->name.value);
        VectorAppend(options, o);
        found = true;
        break;
      }
      // Prefix option takes name from index 2 onwards.
      // Prefixed options are always a string.
      StringInit(&o->value.svalue, &s->name.value[2]);
      // ParseOptions splits "-Xfoo=bar" into name "-Xfoo" and value "bar"; for a
      // prefix option re-join the "=bar" so the value is whole (e.g. "-Dfoo=bar"
      // or "-Werror=format" rather than just "foo"/"error").
      if (s->value.length > 0) {
        StringAppend(&o->value.svalue, "=");
        StringAppend(&o->value.svalue, s->value.value);
      }
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
    // A lone "-" is not an option: it is the input-file name meaning standard
    // input, so treat it as a positional argument.
    if (s->name.value[0] == '-' && s->name.length != 1) {
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

// Help layout.  Option syntax sits in the left column and its text is wrapped
// in the right one; a syntax too wide for the left column takes a line of its
// own so the right column stays straight.
#define kHelpWidth 80
#define kHelpMargin 2
#define kHelpGap 2
#define kHelpMaxSyntax 26
#define kHelpMinTextWidth 28

static const char* option_group_names[kNumOptionGroups] = {
    "Overall options",
    "Language and standards",
    "Preprocessor and include paths",
    "Diagnostics",
    "Code generation and optimization",
    "Linking",
    "C++20 modules",
    "Source listings",
    "Compiler developer options",
};

static bool OptionIsHidden(const CompilerOptionDefinition* option) {
  return option->help == NULL || option->help[0] == '\0' ||
         strncmp(option->help, "(hidden)", 8) == 0;
}

// Render an option the way it is written on the command line, e.g. "-c",
// "-o <file>" or "-I<dir>".
static void OptionSyntax(const CompilerOptionDefinition* option, char* buffer,
                         size_t size) {
  if (option->type == kCompilerOptionBool) {
    snprintf(buffer, size, "%s", option->name);
    return;
  }
  const char* value = option->value_name;
  if (value == NULL) {
    value = option->type == kCompilerOptionInt ? "n" : "value";
  }
  snprintf(buffer, size, option->is_prefix ? "%s<%s>" : "%s <%s>", option->name,
           value);
}

// Print `text` wrapped into the column starting at `column`.  `used` is how
// many columns of the current line are already written; later lines are
// indented to `column`.
static void PrintWrappedText(const char* text, int column, int used) {
  int width = kHelpWidth - column;
  if (width < kHelpMinTextWidth) {
    width = kHelpMinTextWidth;
  }
  if (text == NULL || text[0] == '\0') {
    printf("\n");
    return;
  }
  const char* cursor = text;
  while (*cursor != '\0') {
    while (*cursor == ' ') {
      cursor++;
    }
    if (*cursor == '\0') {
      break;
    }
    size_t take = strlen(cursor);
    if (take > (size_t)width) {
      size_t split = (size_t)width;
      while (split > 0 && cursor[split] != ' ') {
        split--;
      }
      // A word longer than the column cannot be broken on a space, so let it
      // run past the right margin rather than splitting it.
      take = split > 0 ? split : (size_t)width;
    }
    printf("%*s%.*s\n", column - used, "", (int)take, cursor);
    cursor += take;
    used = 0;
  }
}

static void PrintOptionRow(const CompilerOptionDefinition* option, int column) {
  char syntax[128];
  OptionSyntax(option, syntax, sizeof(syntax));
  int used = printf("%*s%s", kHelpMargin, "", syntax);
  if (used >= column) {
    printf("\n");
    used = 0;
  }
  PrintWrappedText(option->help, column, used);
}

// Line up the right column just past the widest syntax that fits in the left
// one, so narrow tables are not padded out to a fixed width.
static int HelpTextColumn(CompilerOptionDefinition** tables,
                          size_t num_tables) {
  size_t widest = 0;
  for (size_t t = 0; t < num_tables; t++) {
    CompilerOptionDefinition* options = tables[t];
    if (options == NULL) {
      continue;
    }
    for (size_t i = 0; options[i].name != NULL; i++) {
      if (OptionIsHidden(&options[i])) {
        continue;
      }
      char syntax[128];
      OptionSyntax(&options[i], syntax, sizeof(syntax));
      size_t length = strlen(syntax);
      if (length > widest && length <= kHelpMaxSyntax) {
        widest = length;
      }
    }
  }
  return (int)(kHelpMargin + widest + kHelpGap);
}

void PrintHelpParagraph(const char* text, int indent) {
  PrintWrappedText(text, indent, 0);
}

void PrintOptionTables(CompilerOptionDefinition** tables, size_t num_tables) {
  if (tables == NULL) {
    return;
  }
  int column = HelpTextColumn(tables, num_tables);
  for (size_t group = 0; group < kNumOptionGroups; group++) {
    bool heading_printed = false;
    for (size_t t = 0; t < num_tables; t++) {
      CompilerOptionDefinition* options = tables[t];
      if (options == NULL) {
        continue;
      }
      for (size_t i = 0; options[i].name != NULL; i++) {
        if (OptionIsHidden(&options[i]) ||
            (size_t)options[i].group != group) {
          continue;
        }
        if (!heading_printed) {
          const char* heading = option_group_names[group];
          printf("\n%s:\n", heading != NULL ? heading : "Other options");
          heading_printed = true;
        }
        PrintOptionRow(&options[i], column);
      }
    }
  }
}

void PrintOptionList(CompilerOptionDefinition* options) {
  if (options == NULL) {
    return;
  }
  int column = HelpTextColumn(&options, 1);
  for (size_t i = 0; options[i].name != NULL; i++) {
    if (OptionIsHidden(&options[i])) {
      continue;
    }
    PrintOptionRow(&options[i], column);
  }
}


