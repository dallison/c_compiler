#include "disassembler_internal.h"

#include "wasm32_machine.h"
#include "wasm32_object.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const unsigned char* bytes;
  size_t length;
  size_t pos;
  bool error;
} WasmCursor;

static void WasmFail(WasmCursor* c) { c->error = true; }

static bool WasmNeed(WasmCursor* c, size_t n) {
  return !c->error && c->pos + n <= c->length;
}

static uint8_t WasmByte(WasmCursor* c) {
  if (!WasmNeed(c, 1)) {
    WasmFail(c);
    return 0;
  }
  return c->bytes[c->pos++];
}

static uint64_t WasmULEB(WasmCursor* c) {
  uint64_t value = 0;
  int shift = 0;
  for (;;) {
    uint8_t byte = WasmByte(c);
    if (c->error) {
      return 0;
    }
    value |= (uint64_t)(byte & 0x7f) << shift;
    if ((byte & 0x80) == 0) {
      return value;
    }
    shift += 7;
    if (shift > 63) {
      WasmFail(c);
      return 0;
    }
  }
}

static int64_t WasmSLEB(WasmCursor* c) {
  uint64_t value = 0;
  int shift = 0;
  uint8_t byte = 0;
  do {
    byte = WasmByte(c);
    if (c->error) {
      return 0;
    }
    value |= (uint64_t)(byte & 0x7f) << shift;
    shift += 7;
    if (shift > 63) {
      WasmFail(c);
      return 0;
    }
  } while ((byte & 0x80) != 0);
  if (shift < 64 && (byte & 0x40) != 0) {
    value |= ~0ULL << shift;
  }
  return (int64_t)value;
}

static const char* kByteName[256];
static const char* kFcName[16];
static const char* kFdName[256];
static bool kNamesReady;

static void SetDotName(const char** slot, const char* src) {
  static char store[512][32];
  static size_t next;
  if (*slot != NULL || next >= sizeof(store) / sizeof(store[0])) {
    return;
  }
  char* out = store[next++];
  size_t i = 0;
  bool dotted = false;
  for (; src[i] != '\0' && i + 1 < sizeof(store[0]); i++) {
    if (!dotted && src[i] == '_') {
      out[i] = '.';
      dotted = true;
    } else {
      out[i] = src[i];
    }
  }
  out[i] = '\0';
  *slot = out;
}

static void EnsureNames(void) {
  if (kNamesReady) {
    return;
  }
#define WASM_ADD_NAME(name, encoding)                                          \
  do {                                                                         \
    int enc = (encoding);                                                      \
    if (enc < 0) {                                                             \
      break;                                                                   \
    }                                                                          \
    if (enc & WASM_PREFIX_FC) {                                                \
      unsigned sub = (unsigned)(enc & 0xffff);                                 \
      if (sub < sizeof(kFcName) / sizeof(kFcName[0])) {                        \
        SetDotName(&kFcName[sub], #name);                                      \
      }                                                                        \
    } else if (enc & WASM_PREFIX_FD) {                                         \
      unsigned sub = (unsigned)(enc & 0xffff);                                 \
      if (sub < sizeof(kFdName) / sizeof(kFdName[0])) {                        \
        SetDotName(&kFdName[sub], #name);                                      \
      }                                                                        \
    } else if (enc < 256) {                                                    \
      SetDotName(&kByteName[enc], #name);                                      \
    }                                                                          \
  } while (0);
  WASM32_OPCODES(WASM_ADD_NAME)
#undef WASM_ADD_NAME
  kNamesReady = true;
}

static const char* BlockTypeName(int64_t type) {
  switch (type) {
    case kWasmTypeI32:
      return "i32";
    case kWasmTypeI64:
      return "i64";
    case kWasmTypeF32:
      return "f32";
    case kWasmTypeF64:
      return "f64";
    case kWasmTypeV128:
      return "v128";
    case kWasmTypeFuncRef:
      return "funcref";
    case kWasmTypeVoid:
      return "";
    default:
      return NULL;
  }
}

static bool ReadBlockType(WasmCursor* c, char* out, size_t out_size) {
  if (!WasmNeed(c, 1)) {
    return false;
  }
  uint8_t first = c->bytes[c->pos];
  const char* named = BlockTypeName((int64_t)(int8_t)first);
  if (named != NULL || first == kWasmTypeVoid || first == kWasmTypeI32 ||
      first == kWasmTypeI64 || first == kWasmTypeF32 || first == kWasmTypeF64 ||
      first == kWasmTypeV128 || first == kWasmTypeFuncRef) {
    c->pos++;
    if (named == NULL || named[0] == '\0') {
      out[0] = '\0';
    } else {
      snprintf(out, out_size, " %s", named);
    }
    return true;
  }
  int64_t index = WasmSLEB(c);
  snprintf(out, out_size, " type%" PRId64, index);
  return !c->error;
}

static bool ReadMemarg(WasmCursor* c, char* out, size_t out_size) {
  uint64_t align = WasmULEB(c);
  uint64_t offset = WasmULEB(c);
  if (c->error) {
    return false;
  }
  snprintf(out, out_size, " offset=%" PRIu64 " align=%" PRIu64, offset, align);
  return true;
}

bool DAsmDisassembleWasm(const void* bytes, size_t length, uint64_t address,
                         DAsmInstruction* inst) {
  EnsureNames();
  if (length == 0) {
    return false;
  }
  WasmCursor c = {.bytes = bytes, .length = length, .pos = 0};
  uint8_t op = WasmByte(&c);
  char extra[96] = "";
  const char* name = NULL;

  if (op == 0xfc) {
    uint64_t sub = WasmULEB(&c);
    if (sub < sizeof(kFcName) / sizeof(kFcName[0])) {
      name = kFcName[sub];
    }
    if (sub == 10) {
      uint64_t dst = WasmULEB(&c);
      uint64_t src = WasmULEB(&c);
      snprintf(extra, sizeof(extra), " %" PRIu64 " %" PRIu64, dst, src);
    } else if (sub == 11) {
      uint64_t mem = WasmULEB(&c);
      snprintf(extra, sizeof(extra), " %" PRIu64, mem);
    }
  } else if (op == 0xfd) {
    uint64_t sub = WasmULEB(&c);
    if (sub < sizeof(kFdName) / sizeof(kFdName[0])) {
      name = kFdName[sub];
    }
    if (sub == 0x00 || sub == 0x0b) {
      if (!ReadMemarg(&c, extra, sizeof(extra))) {
        return false;
      }
    }
  } else {
    name = kByteName[op];
    switch (op) {
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x1f:
        if (!ReadBlockType(&c, extra, sizeof(extra))) {
          return false;
        }
        if (op == 0x1f) {
          uint64_t catches = WasmULEB(&c);
          for (uint64_t i = 0; i < catches && !c.error; i++) {
            uint8_t kind = WasmByte(&c);
            if (kind == 0x00 || kind == 0x02 || kind == 0x03) {
              WasmULEB(&c);
            } else {
              WasmULEB(&c);
              WasmULEB(&c);
            }
          }
        }
        break;
      case 0x0c:
      case 0x0d:
      case 0x08:
      case 0x10:
      case 0x20:
      case 0x21:
      case 0x22:
      case 0x23:
      case 0x24: {
        uint64_t index = WasmULEB(&c);
        snprintf(extra, sizeof(extra), " %" PRIu64, index);
        break;
      }
      case 0x0e: {
        uint64_t count = WasmULEB(&c);
        size_t used = 0;
        used += (size_t)snprintf(extra, sizeof(extra), " %" PRIu64, count);
        for (uint64_t i = 0; i < count && !c.error; i++) {
          uint64_t label = WasmULEB(&c);
          if (used + 16 < sizeof(extra)) {
            used += (size_t)snprintf(extra + used, sizeof(extra) - used,
                                     " %" PRIu64, label);
          }
        }
        uint64_t def = WasmULEB(&c);
        if (used + 16 < sizeof(extra)) {
          snprintf(extra + used, sizeof(extra) - used, " %" PRIu64, def);
        }
        break;
      }
      case 0x11: {
        uint64_t type = WasmULEB(&c);
        uint64_t table = WasmULEB(&c);
        snprintf(extra, sizeof(extra), " %" PRIu64 " %" PRIu64, type, table);
        break;
      }
      case 0x28:
      case 0x29:
      case 0x2a:
      case 0x2b:
      case 0x2c:
      case 0x2d:
      case 0x2e:
      case 0x2f:
      case 0x30:
      case 0x31:
      case 0x32:
      case 0x33:
      case 0x34:
      case 0x35:
      case 0x36:
      case 0x37:
      case 0x38:
      case 0x39:
      case 0x3a:
      case 0x3b:
      case 0x3c:
      case 0x3d:
      case 0x3e:
        if (!ReadMemarg(&c, extra, sizeof(extra))) {
          return false;
        }
        break;
      case 0x3f:
      case 0x40:
        WasmByte(&c);
        break;
      case 0x41: {
        int64_t value = WasmSLEB(&c);
        snprintf(extra, sizeof(extra), " %" PRId64, value);
        break;
      }
      case 0x42: {
        int64_t value = WasmSLEB(&c);
        snprintf(extra, sizeof(extra), " %" PRId64, value);
        break;
      }
      case 0x43: {
        if (!WasmNeed(&c, 4)) {
          return false;
        }
        uint32_t bits = DAsmRead32LE(c.bytes + c.pos);
        c.pos += 4;
        float value;
        memcpy(&value, &bits, sizeof(value));
        snprintf(extra, sizeof(extra), " %g", value);
        break;
      }
      case 0x44: {
        if (!WasmNeed(&c, 8)) {
          return false;
        }
        uint64_t bits = DAsmRead64LE(c.bytes + c.pos);
        c.pos += 8;
        double value;
        memcpy(&value, &bits, sizeof(value));
        snprintf(extra, sizeof(extra), " %g", value);
        break;
      }
      default:
        break;
    }
  }

  if (c.error || c.pos == 0) {
    return false;
  }
  DAsmInitInstruction(inst, bytes, length, address, c.pos);
  if (name == NULL) {
    DAsmUnknownInstruction(inst, ".byte 0x%02" PRIx64, op);
    return true;
  }
  DAsmFormat(inst, "%s%s", name, extra);
  return true;
}

typedef struct {
  const unsigned char* bytes;
  size_t length;
  size_t start;
  uint8_t id;
  char* name;
} WasmSection;

static bool ReadFileBytes(const char* filename, unsigned char** out,
                          size_t* length) {
  FILE* fp = fopen(filename, "rb");
  if (fp == NULL) {
    return false;
  }
  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    return false;
  }
  long size = ftell(fp);
  if (size < 0) {
    fclose(fp);
    return false;
  }
  rewind(fp);
  unsigned char* bytes = malloc((size_t)size);
  if (bytes == NULL ||
      fread(bytes, 1, (size_t)size, fp) != (size_t)size) {
    free(bytes);
    fclose(fp);
    return false;
  }
  fclose(fp);
  *out = bytes;
  *length = (size_t)size;
  return true;
}

static char* DupRange(const unsigned char* p, size_t n) {
  char* s = malloc(n + 1);
  if (s == NULL) {
    return NULL;
  }
  memcpy(s, p, n);
  s[n] = '\0';
  return s;
}

static char* ReadName(WasmCursor* c) {
  uint64_t length = WasmULEB(c);
  if (c->error || !WasmNeed(c, (size_t)length)) {
    WasmFail(c);
    return NULL;
  }
  char* name = DupRange(c->bytes + c->pos, (size_t)length);
  c->pos += (size_t)length;
  return name;
}

static void SetName(char** names, size_t count, uint64_t index,
                    const char* name) {
  if (name == NULL || index >= count || names[index] != NULL) {
    return;
  }
  names[index] = strdup(name);
}

static void CollectExportNames(WasmCursor* c, char** names, size_t count) {
  uint64_t n = WasmULEB(c);
  for (uint64_t i = 0; i < n && !c->error; i++) {
    char* name = ReadName(c);
    uint8_t kind = WasmByte(c);
    uint64_t index = WasmULEB(c);
    if (kind == WASM_EXTERN_FUNC) {
      SetName(names, count, index, name);
    }
    free(name);
  }
}

static void CollectNameSection(WasmCursor* c, char** names, size_t count) {
  while (c->pos < c->length && !c->error) {
    uint8_t id = WasmByte(c);
    uint64_t size = WasmULEB(c);
    if (c->error || c->pos + size > c->length) {
      break;
    }
    size_t end = c->pos + (size_t)size;
    if (id == 1) {
      WasmCursor sub = *c;
      sub.length = end;
      uint64_t n = WasmULEB(&sub);
      for (uint64_t i = 0; i < n && !sub.error; i++) {
        uint64_t index = WasmULEB(&sub);
        char* name = ReadName(&sub);
        SetName(names, count, index, name);
        free(name);
      }
    }
    c->pos = end;
  }
}

static void CollectLinkingNames(WasmCursor* c, char** names, size_t count,
                                size_t import_funcs) {
  uint64_t version = WasmULEB(c);
  if (version != WASM_LINKING_VERSION) {
    return;
  }
  while (c->pos < c->length && !c->error) {
    uint8_t id = WasmByte(c);
    uint64_t size = WasmULEB(c);
    if (c->error || c->pos + size > c->length) {
      break;
    }
    size_t end = c->pos + (size_t)size;
    if (id == WASM_SYMBOL_TABLE) {
      WasmCursor sub = *c;
      sub.length = end;
      uint64_t n = WasmULEB(&sub);
      for (uint64_t i = 0; i < n && !sub.error; i++) {
        uint8_t kind = WasmByte(&sub);
        uint32_t flags = (uint32_t)WasmULEB(&sub);
        bool undefined = (flags & WASM_SYM_UNDEFINED) != 0;
        if (kind == WASM_SYMBOL_DATA) {
          free(ReadName(&sub));
          if (!undefined) {
            WasmULEB(&sub);
            WasmULEB(&sub);
            WasmULEB(&sub);
          }
          continue;
        }
        uint64_t index = WasmULEB(&sub);
        char* name = NULL;
        if (!undefined || (flags & WASM_SYM_EXPLICIT_NAME) != 0) {
          name = ReadName(&sub);
        }
        if (kind == WASM_SYMBOL_FUNCTION && !undefined) {
          SetName(names, count, index, name);
          (void)import_funcs;
        }
        free(name);
      }
    }
    c->pos = end;
  }
}

static size_t CountImportsAndFuncs(const WasmSection* sections, size_t nsec,
                                   size_t* import_funcs) {
  size_t imports = 0;
  size_t defined = 0;
  for (size_t i = 0; i < nsec; i++) {
    WasmCursor c = {.bytes = sections[i].bytes + sections[i].start,
                    .length = sections[i].length,
                    .pos = 0};
    if (sections[i].id == WASM_SECTION_IMPORT) {
      uint64_t count = WasmULEB(&c);
      for (uint64_t n = 0; n < count && !c.error; n++) {
        free(ReadName(&c));
        free(ReadName(&c));
        uint8_t kind = WasmByte(&c);
        if (kind == WASM_EXTERN_FUNC) {
          imports++;
          WasmULEB(&c);
        } else if (kind == WASM_EXTERN_TABLE) {
          WasmByte(&c);
          uint64_t flags = WasmULEB(&c);
          WasmULEB(&c);
          if (flags & 1) {
            WasmULEB(&c);
          }
        } else if (kind == WASM_EXTERN_MEMORY) {
          uint64_t flags = WasmULEB(&c);
          WasmULEB(&c);
          if (flags & 1) {
            WasmULEB(&c);
          }
        } else if (kind == WASM_EXTERN_GLOBAL) {
          WasmByte(&c);
          WasmByte(&c);
        } else if (kind == WASM_EXTERN_TAG) {
          WasmByte(&c);
          WasmULEB(&c);
        }
      }
    } else if (sections[i].id == WASM_SECTION_FUNCTION) {
      defined = (size_t)WasmULEB(&c);
    }
  }
  *import_funcs = imports;
  return imports + defined;
}

static void SkipLocals(WasmCursor* c) {
  uint64_t groups = WasmULEB(c);
  for (uint64_t i = 0; i < groups && !c->error; i++) {
    WasmULEB(c);
    WasmByte(c);
  }
}

bool DAsmIsWasmModule(const void* bytes, size_t length) {
  return length >= 4 && memcmp(bytes, "\0asm", 4) == 0;
}

bool DAsmDisassembleWasmFile(const char* filename, const DAsmOptions* options,
                             FILE* fp) {
  unsigned char* bytes = NULL;
  size_t length = 0;
  if (!ReadFileBytes(filename, &bytes, &length)) {
    fprintf(stderr, "unable to open or read wasm file '%s'\n", filename);
    return false;
  }
  if (!DAsmIsWasmModule(bytes, length) || length < 8) {
    fprintf(stderr, "'%s' is not a WebAssembly module\n", filename);
    free(bytes);
    return false;
  }

  WasmSection sections[64];
  size_t nsec = 0;
  WasmCursor file = {.bytes = bytes, .length = length, .pos = 8};
  while (file.pos < file.length && !file.error && nsec < 64) {
    uint8_t id = WasmByte(&file);
    uint64_t size = WasmULEB(&file);
    if (file.error || file.pos + size > file.length) {
      fprintf(stderr, "truncated wasm section in '%s'\n", filename);
      free(bytes);
      return false;
    }
    sections[nsec].bytes = bytes;
    sections[nsec].start = file.pos;
    sections[nsec].length = (size_t)size;
    sections[nsec].id = id;
    sections[nsec].name = NULL;
    if (id == WASM_SECTION_CUSTOM) {
      WasmCursor names = {.bytes = bytes,
                          .length = file.pos + (size_t)size,
                          .pos = file.pos};
      sections[nsec].name = ReadName(&names);
      sections[nsec].start = names.pos;
      sections[nsec].length = file.pos + (size_t)size - names.pos;
    }
    file.pos += (size_t)size;
    nsec++;
  }

  size_t import_funcs = 0;
  size_t nfuncs = CountImportsAndFuncs(sections, nsec, &import_funcs);
  char** names = calloc(nfuncs == 0 ? 1 : nfuncs, sizeof(*names));
  if (names == NULL) {
    free(bytes);
    return false;
  }
  for (size_t i = 0; i < nsec; i++) {
    WasmCursor c = {.bytes = sections[i].bytes + sections[i].start,
                    .length = sections[i].length,
                    .pos = 0};
    if (sections[i].id == WASM_SECTION_EXPORT) {
      CollectExportNames(&c, names, nfuncs);
    } else if (sections[i].id == WASM_SECTION_CUSTOM &&
               sections[i].name != NULL) {
      if (strcmp(sections[i].name, "name") == 0) {
        CollectNameSection(&c, names, nfuncs);
      } else if (strcmp(sections[i].name, "linking") == 0) {
        CollectLinkingNames(&c, names, nfuncs, import_funcs);
      }
    }
  }

  bool print_names = options != NULL && options->print_section_names;
  for (size_t i = 0; i < nsec; i++) {
    if (sections[i].id != WASM_SECTION_CODE) {
      continue;
    }
    if (print_names) {
      fprintf(fp, "code:\n");
    }
    WasmCursor c = {.bytes = sections[i].bytes + sections[i].start,
                    .length = sections[i].length,
                    .pos = 0};
    uint64_t bodies = WasmULEB(&c);
    for (uint64_t fi = 0; fi < bodies && !c.error; fi++) {
      uint64_t size = WasmULEB(&c);
      if (c.pos + size > c.length) {
        break;
      }
      size_t func_index = import_funcs + (size_t)fi;
      const char* label =
          func_index < nfuncs && names[func_index] != NULL &&
                  names[func_index][0] != '\0'
              ? names[func_index]
              : NULL;
      if (label != NULL) {
        fprintf(fp, "%s:\n", label);
      } else {
        fprintf(fp, "func%" PRIu64 ":\n", (uint64_t)func_index);
      }
      WasmCursor body = {.bytes = c.bytes + c.pos, .length = (size_t)size, .pos = 0};
      SkipLocals(&body);
      uint64_t pc = (uint64_t)(sections[i].start + c.pos + body.pos);
      while (body.pos < body.length) {
        if (options != NULL && options->has_start_address &&
            pc < options->start_address) {
          body.pos++;
          pc++;
          continue;
        }
        if (options != NULL && options->has_start_address &&
            options->has_length &&
            pc >= options->start_address + options->length) {
          break;
        }
        DAsmInstruction inst;
        if (!DAsmDisassembleWasm(body.bytes + body.pos, body.length - body.pos,
                                 pc, &inst) ||
            inst.size == 0) {
          DAsmInitInstruction(&inst, body.bytes + body.pos,
                              body.length - body.pos, pc, 1);
          DAsmUnknownInstruction(&inst, ".byte 0x%02" PRIx64, inst.bytes[0]);
        }
        DAsmPrintInstruction(fp, &inst);
        body.pos += inst.size;
        pc += inst.size;
      }
      c.pos += (size_t)size;
    }
  }

  for (size_t i = 0; i < nsec; i++) {
    free(sections[i].name);
  }
  for (size_t i = 0; i < nfuncs; i++) {
    free(names[i]);
  }
  free(names);
  free(bytes);
  return true;
}
