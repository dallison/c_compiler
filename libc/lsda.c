#include <lsda.h>

#include <stdint.h>
#include <string.h>

typedef struct CXXTypeInfoBase {
  const char* name;
  long offset;
} CXXTypeInfoBase;

typedef struct CXXTypeInfo {
  const char* name;
  long base_count;
  const CXXTypeInfoBase* bases;
  long object_size;
  long object_is_class;
} CXXTypeInfo;

static const uint8_t* ReadUleb128(const uint8_t* p, const uint8_t* end,
                                  unsigned long long* out) {
  unsigned long long value = 0;
  unsigned shift = 0;
  while (p < end) {
    unsigned char byte = *p++;
    value |= (unsigned long long)(byte & 0x7f) << shift;
    if ((byte & 0x80) == 0) {
      *out = value;
      return p;
    }
    shift += 7;
  }
  return NULL;
}

static const uint8_t* ReadSleb128(const uint8_t* p, const uint8_t* end,
                                  long long* out) {
  unsigned long long value = 0;
  unsigned shift = 0;
  while (p < end) {
    unsigned char byte = *p++;
    value |= (unsigned long long)(byte & 0x7f) << shift;
    shift += 7;
    if ((byte & 0x80) == 0) {
      if (shift < 64 && (byte & 0x40) != 0) {
        value |= (unsigned long long)(-1) << shift;
      }
      *out = (long long)value;
      return p;
    }
  }
  return NULL;
}

static int StringEqual(const char* a, const char* b) {
  if (a == b) {
    return 1;
  }
  if (a == 0 || b == 0) {
    return 0;
  }
  while (*a != 0 && *a == *b) {
    a++;
    b++;
  }
  return *a == *b;
}

static int TypeInfoMatches(const CXXTypeInfo* thrown, const CXXTypeInfo* caught,
                           long* offset) {
  *offset = 0;
  if (caught == 0) {
    return 1;
  }
  if (thrown == 0) {
    return 0;
  }
  if (thrown == caught || StringEqual(thrown->name, caught->name)) {
    return 1;
  }
  for (long i = 0; i < thrown->base_count; i++) {
    const CXXTypeInfoBase* base = &thrown->bases[i];
    if (base->name == caught->name || StringEqual(base->name, caught->name)) {
      *offset = base->offset;
      return 1;
    }
  }
  return 0;
}

static int RangeEncloses(uintptr_t start, uintptr_t end, uintptr_t pc,
                         uintptr_t cs, uintptr_t ce) {
  if (pc < start || pc > end) {
    return 0;
  }
  if (start > cs || end < ce) {
    return 0;
  }
  return start < cs || end > ce;
}

int DaveLSDAFindAction(const uint8_t* lsda, uintptr_t func_start, uintptr_t pc,
                       uintptr_t cs, uintptr_t ce, const CXXTypeInfo* thrown,
                       DaveLSDAAction* out) {
  const uint8_t* p = lsda;
  const uint8_t* end = lsda + 4096;
  unsigned long long tmp;
  long long type_filter;
  const uint8_t* call_sites;
  const uint8_t* call_sites_end;
  const uint8_t* action_table;
  const uint8_t* type_table = NULL;
  DaveLSDAAction best;
  int have_best = 0;
  uintptr_t best_start = 0;
  uintptr_t best_end = 0;

  if (lsda == 0 || out == 0) {
    return 0;
  }

  if (*p == 0xff) {
    p++;
  } else {
    p = ReadUleb128(p, end, &tmp);
    if (p == 0) {
      return 0;
    }
  }

  if (*p == 0xff) {
    p++;
  } else {
    p++;
    if (p + 4 > end) {
      return 0;
    }
    int32_t ttype_offset = *(const int32_t*)p;
    p += 4;
    type_table = p - 4 + ttype_offset;
  }

  p = ReadUleb128(p, end, &tmp);
  if (p == 0) {
    return 0;
  }
  call_sites = p;
  call_sites_end = p + tmp;
  action_table = call_sites_end;
  p = call_sites_end;

  while (call_sites < call_sites_end) {
    unsigned long long start_off;
    unsigned long long length;
    unsigned long long landing_off;
    unsigned long long action_index;
    const uint8_t* action;
    long long next_action;
    const CXXTypeInfo* caught = 0;
    long offset = 0;
    int is_catch = 0;
    int is_cleanup = 0;
    uintptr_t try_start;
    uintptr_t try_end;
    DaveLSDAAction candidate;

    call_sites = ReadUleb128(call_sites, call_sites_end, &start_off);
    if (call_sites == 0) {
      break;
    }
    call_sites = ReadUleb128(call_sites, call_sites_end, &length);
    call_sites = ReadUleb128(call_sites, call_sites_end, &landing_off);
    call_sites = ReadUleb128(call_sites, call_sites_end, &action_index);
    if (call_sites == 0) {
      break;
    }
    if (action_index == 0) {
      continue;
    }

    try_start = func_start + (uintptr_t)start_off;
    try_end = func_start + (uintptr_t)(start_off + length);
    if (!RangeEncloses(try_start, try_end, pc, cs, ce)) {
      continue;
    }

    action = action_table;
    for (unsigned long long idx = 1; idx < action_index; idx++) {
      action = ReadSleb128(action, end, &type_filter);
      if (action == 0) {
        break;
      }
      action = ReadSleb128(action, end, &next_action);
      if (action == 0) {
        break;
      }
      if (next_action != 0) {
        action += next_action;
      }
    }
    if (action == 0) {
      continue;
    }
    action = ReadSleb128(action, end, &type_filter);
    if (action == 0) {
      continue;
    }

    if (type_filter < 0) {
      is_cleanup = 1;
    } else if (type_filter == 0) {
      is_catch = 1;
    } else if (type_table != 0) {
      const CXXTypeInfo* const* entry =
          (const CXXTypeInfo* const*)(type_table - (intptr_t)type_filter * 8);
      caught = *entry;
      if (TypeInfoMatches(thrown, caught, &offset)) {
        is_catch = 1;
      } else {
        continue;
      }
    } else {
      continue;
    }

    if (have_best && !(try_start > best_start ||
                       (try_start == best_start && try_end < best_end))) {
      continue;
    }

    candidate.landing_pad = func_start + (uintptr_t)landing_off;
    candidate.try_start = try_start;
    candidate.try_end = try_end;
    candidate.is_catch = is_catch;
    candidate.is_cleanup = is_cleanup;
    candidate.type_offset = offset;
    best = candidate;
    best_start = try_start;
    best_end = try_end;
    have_best = 1;
  }

  if (!have_best) {
    return 0;
  }
  *out = best;
  return 1;
}
