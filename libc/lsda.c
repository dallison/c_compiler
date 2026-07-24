#include <lsda.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define DW_EH_PE_omit 0xff
#define DW_EH_PE_uleb128 0x01
#define DW_EH_PE_pcrel 0x10
#define DW_EH_PE_sdata4 0x0b
#define DW_EH_PE_pcrel_sdata4 0x1b

typedef struct {
  const void* vptr;
  const char* __type_name;
} DaveClassTypeInfo;

typedef struct {
  const void* vptr;
  const char* __type_name;
  const DaveClassTypeInfo* __base_type;
} DaveSiClassTypeInfo;

typedef struct {
  const DaveClassTypeInfo* __base_type;
  long __offset_flags;
} DaveBaseClassTypeInfo;

typedef struct {
  const void* vptr;
  const char* __type_name;
  unsigned int __flags;
  unsigned int __base_count;
} DaveVmiClassTypeInfo;

typedef struct {
  const uint8_t* ttype_table;
  uint8_t ttype_encoding;
  uint8_t call_site_encoding;
  const uint8_t* call_sites;
  const uint8_t* call_sites_end;
  const uint8_t* action_table;
} LSDAHeader;

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
    if (shift >= 64) {
      return NULL;
    }
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
    if (shift >= 64) {
      return NULL;
    }
  }
  return NULL;
}

static const uint8_t* ReadEncodedPointer(const uint8_t* p, const uint8_t* end,
                                         uint8_t encoding, uintptr_t pc,
                                         uintptr_t* out) {
  if (encoding == DW_EH_PE_omit) {
    *out = 0;
    return p;
  }
  if (encoding == DW_EH_PE_uleb128) {
    unsigned long long value = 0;
    p = ReadUleb128(p, end, &value);
    if (p == NULL) {
      return NULL;
    }
    *out = (uintptr_t)value;
    return p;
  }
  if ((encoding & 0x0f) == (DW_EH_PE_sdata4 & 0x0f)) {
    if (p + 4 > end) {
      return NULL;
    }
    int32_t value = *(const int32_t*)p;
    uintptr_t field_address = (uintptr_t)p;
    p += 4;
    if (encoding & DW_EH_PE_pcrel) {
      *out = field_address + (intptr_t)value;
    } else {
      *out = (uintptr_t)(intptr_t)value;
    }
    return p;
  }
  return NULL;
}

static int ParseLSDAHeader(const uint8_t* lsda, const uint8_t* end,
                           uintptr_t pc_for_enc, LSDAHeader* header) {
  const uint8_t* p = lsda;
  unsigned long long tmp;
  uint8_t lp_encoding;

  memset(header, 0, sizeof(*header));

  if (p >= end) {
    return 0;
  }
  lp_encoding = *p++;
  if (lp_encoding != DW_EH_PE_omit) {
    p = ReadEncodedPointer(p, end, lp_encoding, pc_for_enc, &tmp);
    if (p == NULL) {
      return 0;
    }
  }

  if (p >= end) {
    return 0;
  }
  header->ttype_encoding = *p++;
  if (header->ttype_encoding != DW_EH_PE_omit) {
    unsigned long long ttype_offset = 0;
    p = ReadUleb128(p, end, &ttype_offset);
    if (p == NULL) {
      return 0;
    }
    header->ttype_table = p + ttype_offset;
    if (header->ttype_table > end) {
      return 0;
    }
  }

  if (p >= end) {
    return 0;
  }
  header->call_site_encoding = *p++;
  if (header->call_site_encoding == DW_EH_PE_omit) {
    header->call_sites = p;
    header->call_sites_end = p;
    header->action_table = p;
    return 1;
  }

  p = ReadUleb128(p, end, &tmp);
  if (p == NULL) {
    return 0;
  }
  header->call_sites = p;
  header->call_sites_end = p + tmp;
  if (header->call_sites_end > end) {
    return 0;
  }
  header->action_table = header->call_sites_end;
  return 1;
}

static int StringEqual(const char* a, const char* b) {
  if (a == b) {
    return 1;
  }
  if (a == 0 || b == 0) {
    return 0;
  }
  return strcmp(a, b) == 0;
}

static int ClassTypeInfoEqual(const DaveClassTypeInfo* left,
                              const DaveClassTypeInfo* right) {
  if (left == right) {
    return 1;
  }
  if (left == 0 || right == 0) {
    return 0;
  }
  return StringEqual(left->__type_name, right->__type_name);
}

#if !defined(__p_code__)
extern const void* __davecc_itanium_vptr_si_class;
extern const void* __davecc_itanium_vptr_vmi_class;

static int IsTypeInfoKind(const void* actual, const void* kind,
                          const void* kind_address) {
  return actual == kind || actual == kind_address;
}

static void FindCatchBase(const DaveClassTypeInfo* current,
                          const DaveClassTypeInfo* caught, long current_offset,
                          unsigned depth, long* found_offset, int* count) {
  if (current == 0 || caught == 0 || depth >= 64) {
    return;
  }
  if (ClassTypeInfoEqual(current, caught)) {
    if (*count == 0) {
      *found_offset = current_offset;
    }
    (*count)++;
    return;
  }
  if (IsTypeInfoKind(current->vptr, __davecc_itanium_vptr_si_class,
                     (const void*)&__davecc_itanium_vptr_si_class)) {
    const DaveSiClassTypeInfo* si = (const DaveSiClassTypeInfo*)current;
    FindCatchBase(si->__base_type, caught, current_offset, depth + 1,
                  found_offset, count);
    return;
  }
  if (IsTypeInfoKind(current->vptr, __davecc_itanium_vptr_vmi_class,
                     (const void*)&__davecc_itanium_vptr_vmi_class)) {
    const DaveVmiClassTypeInfo* vmi = (const DaveVmiClassTypeInfo*)current;
    const DaveBaseClassTypeInfo* bases =
        (const DaveBaseClassTypeInfo*)((const char*)vmi +
                                      2 * sizeof(void*) +
                                      2 * sizeof(unsigned int));
    for (unsigned int i = 0; i < vmi->__base_count; i++) {
      long flags = bases[i].__offset_flags;
      if ((flags & 0x1) != 0 || (flags & 0x2) == 0) {
        continue;
      }
      FindCatchBase(bases[i].__base_type, caught,
                    current_offset + (flags >> 8), depth + 1, found_offset,
                    count);
    }
  }
}
#endif

static int TypeInfoMatches(const DaveClassTypeInfo* thrown,
                           const DaveClassTypeInfo* caught, long* offset) {
#if defined(__p_code__)
  *offset = 0;
  return caught == 0 || ClassTypeInfoEqual(thrown, caught);
#else
  int count = 0;
  *offset = 0;
  if (caught == 0) {
    return 1;
  }
  if (thrown == 0) {
    return 0;
  }
  FindCatchBase(thrown, caught, 0, 0, offset, &count);
  return count == 1;
#endif
}

static const DaveClassTypeInfo* ReadTypeTableEntry(const LSDAHeader* header,
                                                   long long type_filter,
                                                   uintptr_t lsda_base) {
  const uint8_t* entry;
  uintptr_t value = 0;
  if (header->ttype_table == 0 || type_filter <= 0) {
    return 0;
  }
  entry = header->ttype_table - (size_t)type_filter * 4;
  if (header->ttype_encoding == DW_EH_PE_pcrel_sdata4) {
    int32_t rel = *(const int32_t*)entry;
    if (rel == 0) {
      return 0;
    }
    value = (uintptr_t)entry + (intptr_t)rel;
  } else {
    return 0;
  }
  if (value == 0) {
    return 0;
  }
  return (const DaveClassTypeInfo*)value;
}

static int RangeEncloses(uintptr_t start, uintptr_t end, uintptr_t pc,
                         uintptr_t cs, uintptr_t ce) {
  if (pc < start || pc > end) {
    return 0;
  }
  if (cs == ce) {
    return 1;
  }
  if (start > cs || end < ce) {
    return 0;
  }
  return start < cs || end > ce;
}

static int ActionMatches(const LSDAHeader* header, long long type_filter,
                         const DaveClassTypeInfo* thrown, long* offset,
                         int* is_catch, int* is_cleanup) {
  *offset = 0;
  *is_catch = 0;
  *is_cleanup = 0;
  if (type_filter == 0) {
    *is_cleanup = 1;
    return 1;
  }
  if (type_filter < 0) {
    return 0;
  }
  const DaveClassTypeInfo* caught =
      ReadTypeTableEntry(header, type_filter, (uintptr_t)header->ttype_table);
  if (TypeInfoMatches(thrown, caught, offset)) {
    *is_catch = 1;
    return 1;
  }
  return 0;
}

int DaveLSDAFindAction(const uint8_t* lsda, uintptr_t func_start,
                       const DaveLSDAQuery* query, DaveLSDAAction* out) {
  const DaveClassTypeInfo* thrown_info;
  LSDAHeader header;
  const uint8_t* end = lsda + 4096;
  DaveLSDAAction best;
  int have_best = 0;
  uintptr_t best_start = 0;
  uintptr_t best_end = 0;

  if (lsda == 0 || query == 0 || out == 0) {
    return 0;
  }
  thrown_info = (const DaveClassTypeInfo*)query->thrown;

  if (!ParseLSDAHeader(lsda, end, func_start, &header)) {
    return 0;
  }

  const uint8_t* call_sites = header.call_sites;
  while (call_sites < header.call_sites_end) {
    unsigned long long start_off = 0;
    unsigned long long length = 0;
    unsigned long long landing_off = 0;
    unsigned long long action_index = 0;
    uintptr_t try_start;
    uintptr_t try_end;
    const uint8_t* action;
    long long type_filter;
    long long next_action;

    if (header.call_site_encoding != DW_EH_PE_uleb128) {
      break;
    }
    call_sites = ReadUleb128(call_sites, header.call_sites_end, &start_off);
    if (call_sites == 0) {
      break;
    }
    call_sites = ReadUleb128(call_sites, header.call_sites_end, &length);
    if (call_sites == 0) {
      break;
    }
    call_sites = ReadUleb128(call_sites, header.call_sites_end, &landing_off);
    if (call_sites == 0) {
      break;
    }
    call_sites = ReadUleb128(call_sites, header.call_sites_end, &action_index);
    if (call_sites == 0) {
      break;
    }
    if (action_index == 0) {
      continue;
    }

    try_start = func_start + (uintptr_t)start_off;
    try_end = func_start + (uintptr_t)(start_off + length);
    if (!RangeEncloses(try_start, try_end, query->pc, query->scope_start,
                       query->scope_end)) {
      continue;
    }

    action = header.action_table + action_index - 1;

    for (;;) {
      int is_catch = 0;
      int is_cleanup = 0;
      long offset = 0;
      DaveLSDAAction candidate;

      const uint8_t* next_field =
          ReadSleb128(action, end, &type_filter);
      if (next_field == 0) {
        break;
      }
      const uint8_t* after_record =
          ReadSleb128(next_field, end, &next_action);
      if (after_record == 0) {
        break;
      }

      if (!ActionMatches(&header, type_filter, thrown_info, &offset, &is_catch,
                         &is_cleanup)) {
        if (next_action == 0) {
          break;
        }
        action = next_field + next_action;
        continue;
      }

      if (query->search_phase) {
        if (!is_catch) {
          if (next_action == 0) {
            break;
          }
          action = next_field + next_action;
          continue;
        }
      } else if (is_catch && !query->handler_frame) {
        if (next_action == 0) {
          break;
        }
        action = next_field + next_action;
        continue;
      }

      if (have_best && !(try_start > best_start ||
                         (try_start == best_start && try_end < best_end))) {
        if (next_action == 0) {
          break;
        }
        action = next_field + next_action;
        continue;
      }

      candidate.landing_pad = func_start + (uintptr_t)landing_off;
      candidate.try_start = try_start;
      candidate.try_end = try_end;
      candidate.is_catch = is_catch;
      candidate.is_cleanup = is_cleanup;
      candidate.type_offset = offset;
      candidate.selector = (long)type_filter;
      best = candidate;
      best_start = try_start;
      best_end = try_end;
      have_best = 1;

      if (next_action == 0) {
        break;
      }
      action = next_field + next_action;
    }
  }

  if (!have_best) {
    return 0;
  }
  *out = best;
  return 1;
}
