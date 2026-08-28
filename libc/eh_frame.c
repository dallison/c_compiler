#include <eh_frame.h>
#include <stdlib.h>
#if defined(__arm__)
#include <unwind.h>
#endif

extern char __eh_frame_start[];
extern char __eh_frame_end[];

#define DAVECC_EH_MAX_MODULES 32
#define DAVECC_EH_MAX_FOREIGN 32
#define DAVECC_EH_CFI_STACK 8

typedef struct {
  uintptr_t eh_frame_start;
  uintptr_t eh_frame_end;
  uintptr_t gcc_except_table_start;
  uintptr_t gcc_except_table_end;
  uintptr_t arm_exidx_start;
  uintptr_t arm_exidx_end;
} DaveEHModuleRange;

DaveEHModuleRange __davecc_eh_modules[DAVECC_EH_MAX_MODULES];
size_t __davecc_eh_module_count;

typedef struct {
  const uint8_t* start;
  const uint8_t* end;
} DaveEHForeignRange;

static DaveEHForeignRange g_foreign_frames[DAVECC_EH_MAX_FOREIGN];
static size_t g_foreign_frame_count;

typedef struct {
  const uint8_t* start;
  const uint8_t* end;
  uint8_t pointer_encoding;
  uint8_t lsda_encoding;
  uint8_t personality_encoding;
  void* personality;
  int code_alignment;
  int data_alignment;
  int ra_reg;
  const uint8_t* initial_instructions;
  const uint8_t* initial_instructions_end;
  DaveEHFrameCFI initial_cfi;
  int have_initial_cfi;
} DaveEHCIE;

/* EH pointer encodings (DW_EH_PE_*). */
#define DW_EH_PE_omit 0xff
#define DW_EH_PE_absptr 0x00
#define DW_EH_PE_uleb128 0x01
#define DW_EH_PE_udata2 0x02
#define DW_EH_PE_udata4 0x03
#define DW_EH_PE_udata8 0x04
#define DW_EH_PE_sleb128 0x09
#define DW_EH_PE_sdata2 0x0a
#define DW_EH_PE_sdata4 0x0b
#define DW_EH_PE_sdata8 0x0c
#define DW_EH_PE_pcrel 0x10
#define DW_EH_PE_textrel 0x20
#define DW_EH_PE_funcrel 0x40
#define DW_EH_PE_aligned 0x50
#define DW_EH_PE_indirect 0x80

#if defined(__x86_64__)
#define DAVE_EH_DREG_PC 16
#define DAVE_EH_DREG_SP 7
#define DAVE_EH_DREG_FP 6
#elif defined(__aarch64__)
#define DAVE_EH_DREG_PC 30
#define DAVE_EH_DREG_SP 31
#define DAVE_EH_DREG_FP 29
#elif defined(__riscv) || defined(__risc_v__)
#define DAVE_EH_DREG_PC 1
#define DAVE_EH_DREG_SP 2
#define DAVE_EH_DREG_FP 8
#elif defined(__arm__)
#define DAVE_EH_DREG_PC 15
#define DAVE_EH_DREG_SP 13
#define DAVE_EH_DREG_FP 11
#else
#define DAVE_EH_DREG_PC 0
#define DAVE_EH_DREG_SP 0
#define DAVE_EH_DREG_FP 0
#endif

static void InitCFIState(DaveEHFrameCFI* cfi, const DaveEHCIE* cie);
static int RunCFIProgram(const uint8_t* start, const uint8_t* end,
                         uintptr_t target_pc, uintptr_t pc_begin,
                         const DaveEHCIE* cie, DaveEHFrameCFI* cfi,
                         DaveEHFrameCFI* stack, int* stack_depth,
                         const DaveEHFrameCFI* initial_cfi);
static const uint8_t* FindCIE(const uint8_t* fde_start, const uint8_t* end);
static const uint8_t* ResyncAfterCIE(const uint8_t* cie_start,
                                     const uint8_t* padded_end,
                                     const uint8_t* range_end);

static const uint8_t* ReadUleb128(const uint8_t* p, const uint8_t* end,
                                  unsigned long long* out) {
  unsigned long long value = 0;
  unsigned int shift = 0;
  if (p >= end) {
    return 0;
  }
  do {
    unsigned char byte = *p++;
    value |= ((unsigned long long)(byte & 0x7f)) << shift;
    if ((byte & 0x80) == 0) {
      *out = value;
      return p;
    }
    shift += 7;
  } while (p < end && shift < 64);
  return 0;
}

static const uint8_t* ReadSleb128(const uint8_t* p, const uint8_t* end,
                                  long long* out) {
  long long value = 0;
  unsigned int shift = 0;
  if (p >= end) {
    return 0;
  }
  do {
    unsigned char byte = *p++;
    value |= ((long long)(byte & 0x7f)) << shift;
    shift += 7;
    if ((byte & 0x80) == 0) {
      if (shift < 64 && (byte & 0x40) != 0) {
        value |= -(1LL << shift);
      }
      *out = value;
      return p;
    }
  } while (p < end && shift < 64);
  return 0;
}

static size_t EncodedPointerSize(uint8_t encoding) {
  switch (encoding & 0x0f) {
    case DW_EH_PE_uleb128:
    case DW_EH_PE_sleb128:
      return 0;
    case DW_EH_PE_udata2:
    case DW_EH_PE_sdata2:
      return 2;
    case DW_EH_PE_udata4:
    case DW_EH_PE_sdata4:
      return 4;
    case DW_EH_PE_udata8:
    case DW_EH_PE_sdata8:
      return 8;
    case DW_EH_PE_absptr:
    default:
      return sizeof(uintptr_t);
  }
}

static int ReadEncodedPointer(const uint8_t* p, const uint8_t* end,
                              uint8_t encoding, uintptr_t rel_base,
                              uintptr_t* out) {
  const uint8_t* cursor = p;
  unsigned long long uleb = 0;
  long long sleb = 0;
  uintptr_t value = 0;
  size_t size;

  if (encoding == DW_EH_PE_omit) {
    *out = 0;
    return 1;
  }
  if (cursor >= end) {
    return 0;
  }

  switch (encoding & 0x0f) {
    case DW_EH_PE_uleb128:
      cursor = ReadUleb128(cursor, end, &uleb);
      if (cursor == 0) {
        return 0;
      }
      value = (uintptr_t)uleb;
      break;
    case DW_EH_PE_sleb128:
      cursor = ReadSleb128(cursor, end, &sleb);
      if (cursor == 0) {
        return 0;
      }
      value = (uintptr_t)sleb;
      break;
    case DW_EH_PE_udata2:
      if (cursor + 2 > end) {
        return 0;
      }
      value = *(const uint16_t*)cursor;
      cursor += 2;
      break;
    case DW_EH_PE_udata4:
      if (cursor + 4 > end) {
        return 0;
      }
      value = *(const uint32_t*)cursor;
      cursor += 4;
      break;
    case DW_EH_PE_udata8:
      if (cursor + 8 > end) {
        return 0;
      }
      value = (uintptr_t)(*(const uint64_t*)cursor);
      cursor += 8;
      break;
    case DW_EH_PE_sdata2:
      if (cursor + 2 > end) {
        return 0;
      }
      value = (uintptr_t)(*(const int16_t*)cursor);
      cursor += 2;
      break;
    case DW_EH_PE_sdata4:
      if (cursor + 4 > end) {
        return 0;
      }
      value = (uintptr_t)(*(const int32_t*)cursor);
      cursor += 4;
      break;
    case DW_EH_PE_sdata8:
      if (cursor + 8 > end) {
        return 0;
      }
      value = (uintptr_t)(*(const int64_t*)cursor);
      cursor += 8;
      break;
    case DW_EH_PE_absptr:
    default:
      size = sizeof(uintptr_t);
      if (cursor + size > end) {
        return 0;
      }
      if (size == 8) {
        value = (uintptr_t)(*(const uint64_t*)cursor);
      } else {
        value = *(const uint32_t*)cursor;
      }
      cursor += size;
      break;
  }

  if (value == 0) {
    *out = 0;
    return 1;
  }

  if (sizeof(uintptr_t) > 4 && (encoding & 0x70) == 0 &&
      ((encoding & 0x0f) == DW_EH_PE_udata4 ||
       (encoding & 0x0f) == DW_EH_PE_sdata4)) {
    value = ((uintptr_t)p & ~(uintptr_t)0xffffffffu) | (uint32_t)value;
  }

  if (encoding & DW_EH_PE_pcrel) {
    value = (uintptr_t)((const uint8_t*)p + (intptr_t)value);
  } else if (encoding & DW_EH_PE_textrel) {
    value = rel_base + value;
  } else if (encoding & DW_EH_PE_funcrel) {
    value = rel_base + value;
  }

  if (encoding & DW_EH_PE_indirect) {
    if (value == 0) {
      *out = 0;
      return 1;
    }
    value = *(uintptr_t*)value;
  }

  *out = value;
  (void)cursor;
  return 1;
}

static const uint8_t* SkipEncodedPointer(const uint8_t* p, const uint8_t* end,
                                         uint8_t encoding) {
  uintptr_t ignored;
  if (encoding == DW_EH_PE_omit) {
    return p;
  }
  if ((encoding & 0x0f) == DW_EH_PE_uleb128) {
    unsigned long long tmp;
    return ReadUleb128(p, end, &tmp);
  }
  if ((encoding & 0x0f) == DW_EH_PE_sleb128) {
    long long tmp;
    return ReadSleb128(p, end, &tmp);
  }
  {
    size_t size = EncodedPointerSize(encoding);
    if (size == 0 || p + size > end) {
      return 0;
    }
    return p + size;
  }
}

static int ParseCIE(const uint8_t* entry, const uint8_t* entry_end,
                    DaveEHCIE* out) {
  const uint8_t* body = entry + 4;
  const uint8_t* cursor;
  unsigned long long tmp_uleb;
  long long tmp_sleb;
  int has_z = 0;
  int has_p = 0;
  int has_l = 0;
  int has_r = 0;
  const char* aug;

  if (body + 4 > entry_end || *(const uint32_t*)body != 0) {
    return 0;
  }
  cursor = body + 4;
  if (cursor >= entry_end) {
    return 0;
  }
  if (*cursor++ != 1) {
    return 0;
  }
  aug = (const char*)cursor;
  while (cursor < entry_end && *cursor != 0) {
    if (*cursor == 'z') {
      has_z = 1;
    } else if (*cursor == 'P') {
      has_p = 1;
    } else if (*cursor == 'L') {
      has_l = 1;
    } else if (*cursor == 'R') {
      has_r = 1;
    }
    cursor++;
  }
  if (cursor >= entry_end) {
    return 0;
  }
  cursor++;

  cursor = ReadUleb128(cursor, entry_end, &tmp_uleb);
  if (cursor == 0) {
    return 0;
  }
  out->code_alignment = (int)tmp_uleb;

  cursor = ReadSleb128(cursor, entry_end, &tmp_sleb);
  if (cursor == 0) {
    return 0;
  }
  out->data_alignment = (int)tmp_sleb;

  cursor = ReadUleb128(cursor, entry_end, &tmp_uleb);
  if (cursor == 0) {
    return 0;
  }
  out->ra_reg = (int)tmp_uleb;

  out->pointer_encoding = DW_EH_PE_absptr;
  out->lsda_encoding = DW_EH_PE_omit;
  out->personality_encoding = DW_EH_PE_omit;
  out->personality = 0;

  if (has_z) {
    const uint8_t* aug_data;
    const uint8_t* aug_end;
    unsigned long long aug_len;
    cursor = ReadUleb128(cursor, entry_end, &aug_len);
    if (cursor == 0 || cursor + aug_len > entry_end) {
      return 0;
    }
    aug_data = cursor;
    aug_end = cursor + aug_len;
    cursor += aug_len;
    if (has_p) {
      if (aug_data >= aug_end) {
        return 0;
      }
      out->personality_encoding = *aug_data++;
      if (!ReadEncodedPointer(aug_data, aug_end, out->personality_encoding,
                              (uintptr_t)aug_data, (uintptr_t*)&out->personality)) {
        return 0;
      }
      aug_data = SkipEncodedPointer(aug_data, aug_end, out->personality_encoding);
      if (aug_data == 0) {
        return 0;
      }
    }
    if (has_l) {
      if (aug_data >= aug_end) {
        return 0;
      }
      out->lsda_encoding = *aug_data++;
    }
    if (has_r) {
      if (aug_data >= aug_end) {
        return 0;
      }
      out->pointer_encoding = *aug_data++;
    }
  }

  out->start = entry;
  out->end = entry_end;
  out->initial_instructions = cursor;
  out->initial_instructions_end = entry_end;
  out->have_initial_cfi = 0;
  if (cursor < entry_end) {
    DaveEHFrameCFI stack[DAVECC_EH_CFI_STACK];
    int stack_depth = 0;
    InitCFIState(&out->initial_cfi, out);
    if (RunCFIProgram(out->initial_instructions, out->initial_instructions_end,
                      ~(uintptr_t)0, 0, out, &out->initial_cfi, stack,
                      &stack_depth, 0)) {
      out->have_initial_cfi = 1;
    }
  }
  return 1;
}

static const uint8_t* FindCIE(const uint8_t* fde_start, const uint8_t* end) {
  uint32_t cie_offset = *(const uint32_t*)(fde_start + 4);
  const uint8_t* cie_field = fde_start + 4;
  const uint8_t* cie_ptr = 0;

  if (cie_offset > 0 && cie_offset <= 0x100000 &&
      (uintptr_t)cie_field >= cie_offset) {
    cie_ptr = cie_field - cie_offset;
  }
  if (cie_ptr != 0 && cie_ptr + 8 <= fde_start &&
      *(const uint32_t*)(cie_ptr + 4) == 0) {
    uint32_t len = *(const uint32_t*)cie_ptr;
    if (len >= 4 && cie_ptr + 4 + len <= fde_start) {
      return cie_ptr;
    }
  }
  {
    const uint8_t* scan;
    for (scan = fde_start - 1; scan + 8 <= end && scan >= fde_start - 4096;
         scan--) {
      if (*(const uint32_t*)(scan + 4) == 0) {
        uint32_t len = *(const uint32_t*)scan;
        if (len >= 4 && scan + 4 + len == fde_start) {
          return scan;
        }
      }
    }
    for (scan = fde_start - 1; scan + 8 <= end && scan >= fde_start - 4096;
         scan--) {
      if (*(const uint32_t*)(scan + 4) == 0) {
        uint32_t len = *(const uint32_t*)scan;
        if (len >= 4 && scan < fde_start && fde_start >= scan + 12 &&
            fde_start <= scan + 4 + len) {
          return scan;
        }
      }
    }
  }
  return 0;
}

static int ValidFDEEntry(const uint8_t* entry, const uint8_t* range_end) {
  uint32_t length;
  if (entry + 12 > range_end) {
    return 0;
  }
  length = *(const uint32_t*)entry;
  if (length < 12 || entry + 4 + length > range_end) {
    return 0;
  }
  return FindCIE(entry, range_end) != 0;
}

static const uint8_t* ResyncAfterCIE(const uint8_t* cie_start,
                                     const uint8_t* padded_end,
                                     const uint8_t* range_end) {
  const uint8_t* scan;
  const uint8_t* aligned;
  uint32_t cie_length;

  cie_length = *(const uint32_t*)cie_start;
  aligned = cie_start + 4 + cie_length;
  if (aligned < padded_end && ValidFDEEntry(aligned, range_end)) {
    return aligned;
  }
  for (scan = padded_end; scan + 8 > cie_start + 12; scan--) {
    if (ValidFDEEntry(scan, range_end)) {
      return scan;
    }
  }
  return padded_end;
}

static int FDEUsesDaveCCPCBegin(const uint8_t* entry, const uint8_t* entry_end,
                                const uint8_t* cie_start) {
  uint32_t cie_offset;
  const uint8_t* cie_field;
  const uint8_t* cie_ptr;

  if (entry + 8 > entry_end || cie_start == 0) {
    return 0;
  }
  cie_offset = *(const uint32_t*)(entry + 4);
  if (cie_offset == 0) {
    return 0;
  }
  cie_field = entry + 4;
  cie_ptr = cie_field - cie_offset;
  if (cie_ptr >= cie_start && cie_ptr + 8 <= entry_end &&
      *(const uint32_t*)(cie_ptr + 4) == 0 && cie_ptr == cie_start) {
    return 0;
  }
  return 1;
}

static int CIEHasAugLetter(const DaveEHCIE* cie, char letter) {
  const char* aug;
  if (cie == 0 || cie->start == 0) {
    return 0;
  }
  aug = (const char*)(cie->start + 9);
  while (*aug != 0 && (const uint8_t*)aug < cie->end) {
    if (*aug == letter) {
      return 1;
    }
    aug++;
  }
  return 0;
}

static int ParseFDE(const uint8_t* entry, const uint8_t* entry_end,
                    const uint8_t* cie_hint, DaveEHFDE* out) {
  const uint8_t* fde_body = entry + 4;
  const uint8_t* cie_start;
  DaveEHCIE cie;
  const uint8_t* cursor;
  unsigned long long aug_len;
  uintptr_t pc_begin;
  uintptr_t pc_range;
  uint32_t cie_length;

  if (fde_body + 4 > entry_end || *(const uint32_t*)fde_body == 0) {
    return 0;
  }
  cie_start = cie_hint != 0 ? cie_hint : FindCIE(entry, entry_end);
  if (cie_start == 0) {
    return 0;
  }
  cie_length = *(const uint32_t*)cie_start;
  if (!ParseCIE(cie_start, cie_start + 4 + cie_length, &cie)) {
    return 0;
  }

  if (FDEUsesDaveCCPCBegin(entry, entry_end, cie_start)) {
    cursor = entry + 4;
  } else {
    cursor = fde_body + 4;
  }
  if (!ReadEncodedPointer(cursor, entry_end, cie.pointer_encoding,
                          (uintptr_t)cursor, &pc_begin)) {
    return 0;
  }
  cursor = SkipEncodedPointer(cursor, entry_end, cie.pointer_encoding);
  if (cursor == 0) {
    return 0;
  }
  {
    uint8_t range_encoding = cie.pointer_encoding & 0x0f;
    if (!ReadEncodedPointer(cursor, entry_end, range_encoding, (uintptr_t)cursor,
                            &pc_range)) {
      return 0;
    }
    if (range_encoding == DW_EH_PE_udata4 ||
        range_encoding == DW_EH_PE_sdata4) {
      pc_range = (uint32_t)pc_range;
    }
    cursor = SkipEncodedPointer(cursor, entry_end, range_encoding);
    if (cursor == 0) {
      return 0;
    }
  }

  out->lsda = 0;
  out->has_lsda = 0;
  if (CIEHasAugLetter(&cie, 'z')) {
    cursor = ReadUleb128(cursor, entry_end, &aug_len);
    if (cursor == 0 || cursor + aug_len > entry_end) {
      return 0;
    }
    if (aug_len > 0 && cie.lsda_encoding != DW_EH_PE_omit) {
      uintptr_t lsda = 0;
      if (!ReadEncodedPointer(cursor, cursor + aug_len, cie.lsda_encoding,
                              (uintptr_t)cursor, &lsda)) {
        return 0;
      }
      out->lsda = (const uint8_t*)lsda;
      out->has_lsda = lsda != 0;
    }
    cursor += aug_len;
  }

  out->pc_begin = pc_begin;
  out->pc_end = pc_begin + pc_range;
  if (out->pc_end <= out->pc_begin) {
    return 0;
  }
  out->fde_start = entry;
  out->fde_end = entry_end;
  out->instructions = cursor;
  out->instructions_end = entry_end;
  out->cie_start = cie_start;
  out->personality = cie.personality;
  out->has_frame = cursor < entry_end;
  return 1;
}

int DaveEHFrameGetRange(DaveEHFrameRange* range) {
  if (range == 0) {
    return 0;
  }
  range->start = (const uint8_t*)__eh_frame_start;
  range->end = (const uint8_t*)__eh_frame_end;
  return range->start < range->end;
}

static int ScanForeignRange(const uint8_t* start, const uint8_t* end) {
  uintptr_t cursor = (uintptr_t)start;
  DaveEHFDE fde;
  int found = 0;
  while (DaveEHFrameNextFDE(&cursor, (uintptr_t)end, &fde)) {
    found = 1;
  }
  return found;
}

static const uint8_t* ForeignRangeEnd(const uint8_t* start) {
  uintptr_t cursor = (uintptr_t)start;
  const uint8_t* entry;
  while (1) {
    entry = (const uint8_t*)cursor;
    if (entry + 4 > start + 0x10000000) {
      break;
    }
    if (*(const uint32_t*)entry == 0) {
      return entry + 4;
    }
    {
      uint32_t length = *(const uint32_t*)entry;
      if (length < 4) {
        break;
      }
      cursor += 4 + length;
    }
  }
  return start;
}

void __register_frame(const void* begin) {
  const uint8_t* start = (const uint8_t*)begin;
  const uint8_t* end;
  if (begin == 0 || g_foreign_frame_count >= DAVECC_EH_MAX_FOREIGN) {
    return;
  }
  end = ForeignRangeEnd(start);
  if (end <= start || !ScanForeignRange(start, end)) {
    return;
  }
  g_foreign_frames[g_foreign_frame_count].start = start;
  g_foreign_frames[g_foreign_frame_count].end = end;
  g_foreign_frame_count++;
}

void __deregister_frame(const void* begin) {
  const uint8_t* start = (const uint8_t*)begin;
  size_t i;
  if (begin == 0) {
    return;
  }
  for (i = 0; i < g_foreign_frame_count; i++) {
    if (g_foreign_frames[i].start == start) {
      g_foreign_frames[i] = g_foreign_frames[g_foreign_frame_count - 1];
      g_foreign_frame_count--;
      return;
    }
  }
}

int DaveEHFrameCountFDEs(void) {
  DaveEHFrameRange range;
  uintptr_t cursor;
  DaveEHFDE fde;
  int count = 0;

  if (!DaveEHFrameGetRange(&range)) {
    return 0;
  }
  cursor = (uintptr_t)range.start;
  while (DaveEHFrameNextFDE(&cursor, (uintptr_t)range.end, &fde)) {
    count++;
  }
  return count;
}

static int PlausibleEHFrameLength(uint32_t length, uintptr_t entry_addr,
                                  uintptr_t end, int is_cie) {
  uintptr_t padded_end;
  if (length < 4 || length > 0x100000) {
    return 0;
  }
  padded_end = entry_addr + 4 + length;
  if (padded_end > end || padded_end < entry_addr + 8) {
    return 0;
  }
  if (is_cie) {
    return length >= 4;
  }
  return length >= 12;
}

static uintptr_t ScanForNextEHFrameEntry(uintptr_t start, uintptr_t end) {
  uintptr_t scan;
  for (scan = start; scan + 8 <= end; scan++) {
    uint32_t length = *(const uint32_t*)scan;
    if (*(const uint32_t*)(scan + 4) == 0) {
      if (PlausibleEHFrameLength(length, scan, end, 1)) {
        return scan;
      }
      continue;
    }
    if (PlausibleEHFrameLength(length, scan, end, 0)) {
      return scan;
    }
  }
  return end;
}

int DaveEHFrameNextFDE(uintptr_t* cursor, uintptr_t end, DaveEHFDE* out) {
  uintptr_t entry_addr;
  const uint8_t* entry;
  const uint8_t* cie_hint;
  uint32_t entry_length;

  if (cursor == 0 || out == 0) {
    return 0;
  }
  entry_addr = *cursor;
  while (entry_addr + 8 <= end) {
    while (entry_addr + 8 <= end && *(const uint8_t*)entry_addr == 0) {
      entry_addr++;
    }
    if (entry_addr + 8 > end) {
      break;
    }
    entry = (const uint8_t*)entry_addr;
    cie_hint = 0;
    if (*(const uint32_t*)(entry + 4) == 0) {
      uint32_t cie_length = *(const uint32_t*)entry;
      const uint8_t* cie_start = entry;
      if (!PlausibleEHFrameLength(cie_length, entry_addr, end, 1)) {
        entry_addr = ScanForNextEHFrameEntry(entry_addr + 1, end);
        continue;
      }
      cie_hint = cie_start;
      entry = ResyncAfterCIE(cie_start, entry + 4 + cie_length,
                             (const uint8_t*)end);
      entry_addr = (uintptr_t)entry;
      if (entry_addr + 8 > end) {
        break;
      }
      entry = (const uint8_t*)entry_addr;
    }

    entry_length = *(const uint32_t*)entry;
    if (!PlausibleEHFrameLength(entry_length, entry_addr, end, 0) ||
        *(const uint32_t*)(entry + 4) == 0) {
      entry_addr = ScanForNextEHFrameEntry(entry_addr + 1, end);
      continue;
    }

    if (!ParseFDE(entry, entry + 4 + entry_length, cie_hint, out)) {
      entry_addr = ScanForNextEHFrameEntry(entry_addr + 1, end);
      continue;
    }
    *cursor = entry_addr + 4 + entry_length;
    return 1;
  }
  *cursor = end;
  return 0;
}

#if defined(__x86_64__) || defined(__aarch64__) || defined(__riscv) || \
    defined(__risc_v__)
#define DAVE_EH_GUEST_TEXT_BASE 0x400000000ULL
#define DAVE_EH_GUEST_ALT_TEXT_BASE 0x410000000ULL
#define DAVE_EH_GUEST_ALT_TEXT_BIAS \
  (DAVE_EH_GUEST_ALT_TEXT_BASE - DAVE_EH_GUEST_TEXT_BASE)

static int FDEContainsPC(const DaveEHFDE* fde, uintptr_t pc) {
  uintptr_t span;
  if (fde->pc_begin == 0 || fde->pc_end <= fde->pc_begin) {
    return 0;
  }
  span = fde->pc_end - fde->pc_begin;
  if (span == 0 || span > 0x100000) {
    return 0;
  }
  if (pc >= fde->pc_begin && pc < fde->pc_end) {
    return 1;
  }
  if (pc >= DAVE_EH_GUEST_ALT_TEXT_BASE && pc < 0x500000000ULL) {
    uintptr_t rel = pc - DAVE_EH_GUEST_ALT_TEXT_BIAS;
    return rel >= fde->pc_begin && rel < fde->pc_end;
  }
  return 0;
}

static void AdjustFDEPCForLookup(DaveEHFDE* fde, uintptr_t pc) {
  if (pc >= fde->pc_begin && pc < fde->pc_end) {
    return;
  }
  if (pc >= DAVE_EH_GUEST_ALT_TEXT_BASE && pc < 0x500000000ULL) {
    fde->pc_begin += DAVE_EH_GUEST_ALT_TEXT_BIAS;
    fde->pc_end += DAVE_EH_GUEST_ALT_TEXT_BIAS;
  }
}

// The address an entry would be recorded under, for a pc that may name the
// aliased text mapping.  Entries hold unbiased addresses, so searching for such a
// pc has to look for the unbiased spelling as well as the literal one.
static uintptr_t UnbiasedLookupPC(uintptr_t pc) {
  if (pc >= DAVE_EH_GUEST_ALT_TEXT_BASE && pc < 0x500000000ULL) {
    return pc - DAVE_EH_GUEST_ALT_TEXT_BIAS;
  }
  return pc;
}
#else
static int FDEContainsPC(const DaveEHFDE* fde, uintptr_t pc) {
  uintptr_t span;
  if (fde->pc_begin == 0 || fde->pc_end <= fde->pc_begin) {
    return 0;
  }
  span = fde->pc_end - fde->pc_begin;
  if (span == 0 || span > 0x100000) {
    return 0;
  }
  return pc >= fde->pc_begin && pc < fde->pc_end;
}

static void AdjustFDEPCForLookup(DaveEHFDE* fde, uintptr_t pc) {
  (void)fde;
  (void)pc;
}

static uintptr_t UnbiasedLookupPC(uintptr_t pc) {
  return pc;
}
#endif

// Of two FDEs that both cover the pc, which one should be used?  A table can
// describe the same address more than once, so prefer the entry that carries the
// most information: one with a personality routine over one without, then one
// with a language-specific data area, then the tightest range.
static int BetterFDE(const DaveEHFDE* best, const DaveEHFDE* candidate) {
  if (best->personality == 0 && candidate->personality != 0) {
    return 1;
  }
  if (best->personality != 0 && candidate->personality == 0) {
    return 0;
  }
  if (!best->has_lsda && candidate->has_lsda) {
    return 1;
  }
  if (best->has_lsda && !candidate->has_lsda) {
    return 0;
  }
  return (candidate->pc_end - candidate->pc_begin) <
         (best->pc_end - best->pc_begin);
}

static int FindFDEInRange(uintptr_t pc, const uint8_t* start, const uint8_t* end,
                          DaveEHFDE* out) {
  uintptr_t cursor = (uintptr_t)start;
  DaveEHFDE fde;
  DaveEHFDE best;
  int have_best = 0;

  if (start == 0 || end <= start) {
    return 0;
  }
  while (DaveEHFrameNextFDE(&cursor, (uintptr_t)end, &fde)) {
    if (!FDEContainsPC(&fde, pc)) {
      continue;
    }
    if (!have_best || BetterFDE(&best, &fde)) {
      best = fde;
      have_best = 1;
    }
  }
  if (have_best && out != 0) {
    AdjustFDEPCForLookup(&best, pc);
    *out = best;
  }
  return have_best;
}

// An index over the program's own .eh_frame, so that a frame lookup costs a
// binary search rather than a walk of the whole table.  Without it, unwinding one
// frame parses every entry -- FindFDEInRange cannot stop at the first match
// because a later entry may describe the address better -- which made a throw
// cost time proportional to the size of the table times the number of frames.
//
// Only this one range is indexed.  It is delimited by linker symbols and so never
// changes, which is what lets the index be built once and never invalidated;
// module and foreign ranges come and go through __register_frame and hold few
// entries each, so they keep walking.
typedef struct {
  uintptr_t pc_begin;
  uintptr_t pc_end;
  const uint8_t* fde_start;
} DaveEHIndexEntry;

typedef struct {
  DaveEHIndexEntry* entries;
  size_t count;
  // The widest range any entry covers, which bounds how far below the pc a
  // covering entry can start and so how far the search has to walk back.
  uintptr_t max_span;
} DaveEHIndex;

// Published as a single pointer store once fully built, so a reader either sees
// no index or a complete one.  A thread that loses the race to build it frees its
// own copy.
static DaveEHIndex* volatile g_primary_index;
static volatile int g_primary_index_unavailable;

// Insert the entry keeping the array ascending by pc_begin.  A linker emits
// .eh_frame in address order, so this almost always appends.
static void InsertIndexEntry(DaveEHIndexEntry* entries, size_t count,
                             const DaveEHIndexEntry* entry) {
  size_t i = count;
  while (i > 0 && entries[i - 1].pc_begin > entry->pc_begin) {
    entries[i] = entries[i - 1];
    i--;
  }
  entries[i] = *entry;
}

static DaveEHIndex* BuildPrimaryIndex(const uint8_t* start, const uint8_t* end) {
  DaveEHIndex* index;
  DaveEHIndexEntry* entries;
  uintptr_t cursor;
  DaveEHFDE fde;
  // Grown as the table is walked rather than sized by a counting pass, because
  // parsing the table is the expensive part and this is on the path of the first
  // throw.
  size_t capacity = 256;
  size_t count = 0;

  index = (DaveEHIndex*)malloc(sizeof(*index));
  if (index == 0) {
    return 0;
  }
  entries = (DaveEHIndexEntry*)malloc(capacity * sizeof(*entries));
  if (entries == 0) {
    free(index);
    return 0;
  }

  index->max_span = 0;
  cursor = (uintptr_t)start;
  while (DaveEHFrameNextFDE(&cursor, (uintptr_t)end, &fde)) {
    DaveEHIndexEntry entry;
    uintptr_t span;
    // Leave out what a lookup could never match anyway (see FDEContainsPC).
    // Keeping an implausibly wide entry would also raise max_span, and with it
    // the number of entries every later search has to walk back over.
    if (fde.pc_begin == 0 || fde.pc_end <= fde.pc_begin ||
        fde.pc_end - fde.pc_begin > 0x100000) {
      continue;
    }
    if (count == capacity) {
      DaveEHIndexEntry* grown;
      size_t larger = capacity * 2;
      grown = (DaveEHIndexEntry*)realloc(entries, larger * sizeof(*entries));
      if (grown == 0) {
        free(entries);
        free(index);
        return 0;
      }
      entries = grown;
      capacity = larger;
    }
    entry.pc_begin = fde.pc_begin;
    entry.pc_end = fde.pc_end;
    entry.fde_start = fde.fde_start;
    InsertIndexEntry(entries, count, &entry);
    count++;
    span = fde.pc_end - fde.pc_begin;
    if (span > index->max_span) {
      index->max_span = span;
    }
  }
  if (count == 0) {
    free(entries);
    free(index);
    return 0;
  }
  index->entries = entries;
  index->count = count;
  return index;
}

static const DaveEHIndex* PrimaryIndex(const uint8_t* start,
                                       const uint8_t* end) {
  DaveEHIndex* index = g_primary_index;
  DaveEHIndex* published;
  if (index != 0) {
    return index;
  }
  if (g_primary_index_unavailable) {
    return 0;
  }
  index = BuildPrimaryIndex(start, end);
  if (index == 0) {
    // Nothing to index, or no memory to index it with.  Remember that, so every
    // later lookup does not pay for another walk of the table before falling
    // back to walking it.
    g_primary_index_unavailable = 1;
    return 0;
  }
  published = g_primary_index;
  if (published != 0) {
    free(index->entries);
    free(index);
    return published;
  }
  // Kept for the life of the process: it describes a range that cannot change,
  // and an unwind is the wrong place to be rebuilding it.
  g_primary_index = index;
  return index;
}

// Re-read the entry the index points at, to recover the fields the index does not
// store.  This runs for the handful of entries that can cover one address, not
// for the whole table.
static int ParseIndexedFDE(const DaveEHIndexEntry* entry, const uint8_t* end,
                           DaveEHFDE* out) {
  uintptr_t cursor = (uintptr_t)entry->fde_start;
  return DaveEHFrameNextFDE(&cursor, (uintptr_t)end, out) &&
         out->pc_begin == entry->pc_begin && out->pc_end == entry->pc_end;
}

// Index of the first entry that starts after key, i.e. one past the last entry
// that could possibly cover it.
static size_t UpperBoundByPCBegin(const DaveEHIndex* index, uintptr_t key) {
  size_t low = 0;
  size_t high = index->count;
  while (low < high) {
    size_t mid = low + (high - low) / 2;
    if (index->entries[mid].pc_begin <= key) {
      low = mid + 1;
    } else {
      high = mid;
    }
  }
  return low;
}

// Consider every entry that starts at or below key and is still close enough to
// reach the pc, keeping the best one.  'pc' is the address being looked up and
// 'key' the value being searched for, which differ when the pc has to be
// un-biased first.
static void SearchIndexForKey(const DaveEHIndex* index, const uint8_t* end,
                              uintptr_t pc, uintptr_t key, DaveEHFDE* best,
                              int* have_best) {
  size_t i = UpperBoundByPCBegin(index, key);
  while (i > 0) {
    const DaveEHIndexEntry* entry = &index->entries[--i];
    DaveEHFDE fde;
    if (key - entry->pc_begin >= index->max_span) {
      // Entries below this one start even earlier, so none can reach the pc.
      break;
    }
    if (pc < entry->pc_begin || pc >= entry->pc_end) {
      if (key < entry->pc_begin || key >= entry->pc_end) {
        continue;
      }
    }
    if (!ParseIndexedFDE(entry, end, &fde) || !FDEContainsPC(&fde, pc)) {
      continue;
    }
    if (!*have_best || BetterFDE(best, &fde)) {
      *best = fde;
      *have_best = 1;
    }
  }
}

static int FindFDEInPrimaryRange(uintptr_t pc, const uint8_t* start,
                                 const uint8_t* end, DaveEHFDE* out) {
  const DaveEHIndex* index;
  DaveEHFDE best;
  int have_best = 0;
  uintptr_t key;

  if (start == 0 || end <= start) {
    return 0;
  }
  index = PrimaryIndex(start, end);
  if (index == 0) {
    return FindFDEInRange(pc, start, end, out);
  }
  SearchIndexForKey(index, end, pc, pc, &best, &have_best);
  // An address in the aliased text mapping matches an entry through the bias, so
  // it has to be searched for under that spelling too (see FDEContainsPC).
  key = UnbiasedLookupPC(pc);
  if (key != pc) {
    SearchIndexForKey(index, end, pc, key, &best, &have_best);
  }
  if (have_best && out != 0) {
    AdjustFDEPCForLookup(&best, pc);
    *out = best;
  }
  return have_best;
}

int DaveEHFrameFindFDE(uintptr_t pc, DaveEHFDE* out) {
#if defined(__arm__)
  DaveARMUnwindInfo info;
  if (!DaveARMLookupUnwindInfo(pc, &info)) {
    return 0;
  }
  if (out != 0) {
    out->pc_begin = info.pc_begin;
    out->pc_end = info.pc_end;
    out->fde_start = 0;
    out->instructions = 0;
    out->instructions_end = 0;
    out->fde_end = 0;
    out->cie_start = 0;
    out->lsda = info.lsda;
    out->personality = info.personality;
    out->has_frame = 1;
    out->has_lsda = info.lsda != 0;
  }
  return 1;
#else
  DaveEHFrameRange range;
  size_t i;
  if (DaveEHFrameGetRange(&range) &&
      FindFDEInPrimaryRange(pc, range.start, range.end, out)) {
    return 1;
  }
  if (__davecc_eh_module_count > DAVECC_EH_MAX_MODULES) {
    return 0;
  }
  for (i = 0; i < __davecc_eh_module_count; i++) {
    DaveEHModuleRange* module = &__davecc_eh_modules[i];
    if (FindFDEInRange(pc, (const uint8_t*)module->eh_frame_start,
                       (const uint8_t*)module->eh_frame_end, out)) {
      return 1;
    }
  }
  for (i = 0; i < g_foreign_frame_count; i++) {
    if (FindFDEInRange(pc, g_foreign_frames[i].start, g_foreign_frames[i].end,
                       out)) {
      return 1;
    }
  }
  return 0;
#endif
}

void DaveEHFrameInitRegisters(DaveEHFrameRegisters* regs) {
  size_t i;
  if (regs == 0) {
    return;
  }
  regs->pc = 0;
  regs->rsp = 0;
  regs->rbp = 0;
  for (i = 0; i < DAVE_EH_MAX_DREG; i++) {
    regs->gr[i] = 0;
  }
}

void DaveEHFrameSyncCanonical(DaveEHFrameRegisters* regs) {
  if (regs == 0) {
    return;
  }
  regs->gr[DAVE_EH_DREG_PC] = regs->pc;
  regs->gr[DAVE_EH_DREG_SP] = regs->rsp;
  regs->gr[DAVE_EH_DREG_FP] = regs->rbp;
  regs->pc = regs->gr[DAVE_EH_DREG_PC];
  regs->rsp = regs->gr[DAVE_EH_DREG_SP];
  regs->rbp = regs->gr[DAVE_EH_DREG_FP];
}

uintptr_t DaveEHFrameGetReg(const DaveEHFrameRegisters* regs, int dwarf_reg) {
  if (regs == 0 || dwarf_reg < 0 || dwarf_reg >= DAVE_EH_MAX_DREG) {
    return 0;
  }
  if (dwarf_reg == DAVE_EH_DREG_PC) {
    return regs->pc;
  }
  if (dwarf_reg == DAVE_EH_DREG_SP) {
    return regs->rsp;
  }
  if (dwarf_reg == DAVE_EH_DREG_FP) {
    return regs->rbp;
  }
  return regs->gr[dwarf_reg];
}

void DaveEHFrameSetReg(DaveEHFrameRegisters* regs, int dwarf_reg,
                       uintptr_t value) {
  if (regs == 0 || dwarf_reg < 0 || dwarf_reg >= DAVE_EH_MAX_DREG) {
    return;
  }
  regs->gr[dwarf_reg] = value;
  if (dwarf_reg == DAVE_EH_DREG_PC) {
    regs->pc = value;
  } else if (dwarf_reg == DAVE_EH_DREG_SP) {
    regs->rsp = value;
  } else if (dwarf_reg == DAVE_EH_DREG_FP) {
    regs->rbp = value;
  }
}

static void RestoreCFIReg(DaveEHFrameCFI* cfi, int reg,
                          const DaveEHFrameCFI* initial) {
  if (reg < 0 || reg >= DAVE_EH_MAX_DREG) {
    return;
  }
  if (initial != 0) {
    cfi->regs[reg] = initial->regs[reg];
  } else {
    cfi->regs[reg].rule = DAVE_CFI_REG_UNDEFINED;
    cfi->regs[reg].offset = 0;
    cfi->regs[reg].reg = 0;
  }
}

static void InitCFIState(DaveEHFrameCFI* cfi, const DaveEHCIE* cie) {
  size_t i;
  (void)cie;
  cfi->cfa_reg = 0;
  cfi->cfa_offset = 0;
  cfi->ra_reg = cie->ra_reg;
  for (i = 0; i < DAVE_EH_MAX_DREG; i++) {
    cfi->regs[i].rule = DAVE_CFI_REG_UNDEFINED;
    cfi->regs[i].offset = 0;
    cfi->regs[i].reg = 0;
  }
}

static void CopyCFIState(DaveEHFrameCFI* dst, const DaveEHFrameCFI* src) {
  size_t i;
  dst->cfa_reg = src->cfa_reg;
  dst->cfa_offset = src->cfa_offset;
  dst->ra_reg = src->ra_reg;
  for (i = 0; i < DAVE_EH_MAX_DREG; i++) {
    dst->regs[i] = src->regs[i];
  }
}

static int ApplyCFIOp(DaveEHFrameCFI* cfi, const DaveEHCIE* cie,
                      const DaveEHFrameCFI* initial_cfi, uint8_t op,
                      const uint8_t** cursor, const uint8_t* end) {
  const uint8_t* p = *cursor + 1;
  unsigned long long uleb = 0;
  long long sleb = 0;
  int reg;

  if (op >= 0x80 && op <= 0xbf) {
    reg = (int)(op - 0x80);
    p = ReadUleb128(p, end, &uleb);
    if (p == 0 || reg >= DAVE_EH_MAX_DREG) {
      return 0;
    }
    cfi->regs[reg].rule = DAVE_CFI_REG_OFFSET;
    cfi->regs[reg].offset = (intptr_t)((long long)uleb * cie->data_alignment);
    *cursor = p;
    return 1;
  }
  if (op >= 0xc0) {
    reg = (int)(op - 0xc0);
    RestoreCFIReg(cfi, reg, initial_cfi);
    *cursor = *cursor + 1;
    return 1;
  }

  switch (op) {
    case 0:
      *cursor = p;
      return 1;
    case 1:
      if (cie->pointer_encoding == DW_EH_PE_uleb128) {
        p = ReadUleb128(p, end, &uleb);
      } else {
        size_t size = EncodedPointerSize(cie->pointer_encoding);
        if (p + size > end) {
          return 0;
        }
        if (size == 8) {
          uleb = *(const uint64_t*)p;
        } else if (size == 4) {
          uleb = *(const uint32_t*)p;
        } else {
          uleb = *p;
        }
        p += size;
      }
      if (p == 0) {
        return 0;
      }
      *cursor = p;
      return 1;
    case 0x05: {
      p = ReadUleb128(p, end, &uleb);
      if (p == 0) {
        return 0;
      }
      reg = (int)uleb;
      p = ReadUleb128(p, end, &uleb);
      if (p == 0 || reg >= DAVE_EH_MAX_DREG) {
        return 0;
      }
      cfi->regs[reg].rule = DAVE_CFI_REG_OFFSET;
      cfi->regs[reg].offset =
          (intptr_t)((long long)uleb * cie->data_alignment);
      *cursor = p;
      return 1;
    }
    case 0x06:
      p = ReadUleb128(p, end, &uleb);
      if (p == 0) {
        return 0;
      }
      RestoreCFIReg(cfi, (int)uleb, initial_cfi);
      *cursor = p;
      return 1;
    case 0x07:
      p = ReadUleb128(p, end, &uleb);
      if (p == 0) {
        return 0;
      }
      reg = (int)uleb;
      if (reg >= DAVE_EH_MAX_DREG) {
        return 0;
      }
      cfi->regs[reg].rule = DAVE_CFI_REG_UNDEFINED;
      cfi->regs[reg].offset = 0;
      cfi->regs[reg].reg = 0;
      *cursor = p;
      return 1;
    case 0x08:
      p = ReadUleb128(p, end, &uleb);
      if (p == 0) {
        return 0;
      }
      reg = (int)uleb;
      if (reg >= DAVE_EH_MAX_DREG) {
        return 0;
      }
      cfi->regs[reg].rule = DAVE_CFI_REG_SAME;
      *cursor = p;
      return 1;
    case 0x09:
      p = ReadUleb128(p, end, &uleb);
      if (p == 0) {
        return 0;
      }
      reg = (int)uleb;
      p = ReadUleb128(p, end, &uleb);
      if (p == 0 || reg >= DAVE_EH_MAX_DREG) {
        return 0;
      }
      cfi->regs[reg].rule = DAVE_CFI_REG_REGISTER;
      cfi->regs[reg].reg = (int)uleb;
      *cursor = p;
      return 1;
    case 0x0a:
    case 0x0b:
      *cursor = p;
      return 1;
    case 0x0c:
      p = ReadUleb128(p, end, &uleb);
      if (p == 0) {
        return 0;
      }
      cfi->cfa_reg = (int)uleb;
      p = ReadUleb128(p, end, &uleb);
      if (p == 0) {
        return 0;
      }
      cfi->cfa_offset = (intptr_t)uleb;
      *cursor = p;
      return 1;
    case 0x0d:
      p = ReadUleb128(p, end, &uleb);
      if (p == 0) {
        return 0;
      }
      cfi->cfa_reg = (int)uleb;
      *cursor = p;
      return 1;
    case 0x0e:
      p = ReadUleb128(p, end, &uleb);
      if (p == 0) {
        return 0;
      }
      cfi->cfa_offset = (intptr_t)uleb;
      *cursor = p;
      return 1;
    case 0x0f:
    case 0x10:
    case 0x16:
      return 0;
    case 0x11:
      p = ReadUleb128(p, end, &uleb);
      if (p == 0) {
        return 0;
      }
      reg = (int)uleb;
      p = ReadSleb128(p, end, &sleb);
      if (p == 0 || reg >= DAVE_EH_MAX_DREG) {
        return 0;
      }
      cfi->regs[reg].rule = DAVE_CFI_REG_OFFSET;
      cfi->regs[reg].offset = (intptr_t)sleb;
      *cursor = p;
      return 1;
    case 0x12:
      p = ReadUleb128(p, end, &uleb);
      if (p == 0) {
        return 0;
      }
      cfi->cfa_reg = (int)uleb;
      p = ReadSleb128(p, end, &sleb);
      if (p == 0) {
        return 0;
      }
      cfi->cfa_offset = (intptr_t)sleb;
      *cursor = p;
      return 1;
    case 0x13:
      p = ReadSleb128(p, end, &sleb);
      if (p == 0) {
        return 0;
      }
      cfi->cfa_offset = (intptr_t)sleb;
      *cursor = p;
      return 1;
    case 0x14:
      p = ReadUleb128(p, end, &uleb);
      if (p == 0) {
        return 0;
      }
      reg = (int)uleb;
      p = ReadUleb128(p, end, &uleb);
      if (p == 0 || reg >= DAVE_EH_MAX_DREG) {
        return 0;
      }
      cfi->regs[reg].rule = DAVE_CFI_REG_VAL_OFFSET;
      cfi->regs[reg].offset =
          (intptr_t)((long long)uleb * cie->data_alignment);
      *cursor = p;
      return 1;
    case 0x15:
      p = ReadUleb128(p, end, &uleb);
      if (p == 0) {
        return 0;
      }
      reg = (int)uleb;
      p = ReadSleb128(p, end, &sleb);
      if (p == 0 || reg >= DAVE_EH_MAX_DREG) {
        return 0;
      }
      cfi->regs[reg].rule = DAVE_CFI_REG_VAL_OFFSET;
      cfi->regs[reg].offset = (intptr_t)sleb;
      *cursor = p;
      return 1;
    case 0x2e:
      p = ReadUleb128(p, end, &uleb);
      if (p == 0) {
        return 0;
      }
      *cursor = p;
      return 1;
    default:
      return 0;
  }
}

static int RunCFIProgram(const uint8_t* start, const uint8_t* end,
                         uintptr_t target_pc, uintptr_t pc_begin,
                         const DaveEHCIE* cie, DaveEHFrameCFI* cfi,
                         DaveEHFrameCFI* stack, int* stack_depth,
                         const DaveEHFrameCFI* initial_cfi) {
  const uint8_t* cursor = start;
  uintptr_t loc = pc_begin;
  int in_cie = (start >= cie->initial_instructions &&
                start < cie->initial_instructions_end);
  int seen_advance = in_cie;

  while (cursor < end) {
    uint8_t op = *cursor++;
    if (op >= 0x40 && op <= 0x7f) {
      seen_advance = 1;
      loc += (uintptr_t)((op - 0x40) * cie->code_alignment);
      if (loc > target_pc) {
        break;
      }
      continue;
    }
    if (op == 1) {
      uintptr_t new_loc;
      seen_advance = 1;
      if (!ReadEncodedPointer(cursor, end, cie->pointer_encoding,
                              (uintptr_t)cursor, &new_loc)) {
        return 0;
      }
      cursor = SkipEncodedPointer(cursor, end, cie->pointer_encoding);
      if (cursor == 0) {
        return 0;
      }
      loc = new_loc;
      if (loc > target_pc) {
        break;
      }
      continue;
    }
    if (op == 0x0a) {
      if (*stack_depth >= DAVECC_EH_CFI_STACK) {
        return 0;
      }
      CopyCFIState(&stack[(*stack_depth)++], cfi);
      continue;
    }
    if (op == 0x0b) {
      if (*stack_depth <= 0) {
        return 0;
      }
      CopyCFIState(cfi, &stack[--(*stack_depth)]);
      continue;
    }
    if (op == 0x02 || op == 0x03 || op == 0x04) {
      uintptr_t delta = 0;
      seen_advance = 1;
      if (op == 0x02) {
        if (cursor >= end) {
          return 0;
        }
        delta = *cursor++;
      } else if (op == 0x03) {
        if (cursor + 2 > end) {
          return 0;
        }
        delta = *(const uint16_t*)cursor;
        cursor += 2;
      } else {
        if (cursor + 4 > end) {
          return 0;
        }
        delta = *(const uint32_t*)cursor;
        cursor += 4;
      }
      loc += delta * (uintptr_t)cie->code_alignment;
      if (loc > target_pc) {
        break;
      }
      continue;
    }
    cursor--;
    if (!seen_advance) {
      DaveEHFrameCFI scratch;
      CopyCFIState(&scratch, cfi);
      if (!ApplyCFIOp(&scratch, cie, initial_cfi, op, &cursor, end)) {
        return 0;
      }
      continue;
    }
    if (!ApplyCFIOp(cfi, cie, initial_cfi, op, &cursor, end)) {
      return 0;
    }
  }
  return 1;
}

int DaveEHFrameCFIAtPC(const DaveEHFDE* fde, uintptr_t pc,
                       DaveEHFrameCFI* out) {
  DaveEHCIE cie;
  DaveEHFrameCFI stack[DAVECC_EH_CFI_STACK];
  int stack_depth = 0;

  if (fde == 0 || out == 0 || fde->cie_start == 0) {
    return 0;
  }
  if (!ParseCIE(fde->cie_start,
                fde->cie_start + 4 + *(const uint32_t*)fde->cie_start, &cie)) {
    return 0;
  }
  if (!cie.have_initial_cfi) {
    InitCFIState(out, &cie);
  } else {
    *out = cie.initial_cfi;
  }
  if (fde->has_frame &&
      !RunCFIProgram(fde->instructions, fde->instructions_end, pc, fde->pc_begin,
                     &cie, out, stack, &stack_depth, &cie.initial_cfi)) {
    return 0;
  }
  return 1;
}

#if defined(__x86_64__) || defined(__aarch64__) || defined(__riscv) || \
    defined(__risc_v__)

static uintptr_t ComputeCFA(const DaveEHFrameCFI* cfi,
                            const DaveEHFrameRegisters* regs) {
  uintptr_t base = DaveEHFrameGetReg(regs, cfi->cfa_reg);
  return (uintptr_t)((intptr_t)base + cfi->cfa_offset);
}

static uintptr_t ResolveCFIReg(const DaveEHFrameCFI* cfi,
                               const DaveEHFrameRegisters* regs, int reg,
                               uintptr_t cfa) {
  DaveCFIRegState rule;
  uintptr_t slot;
  if (reg < 0 || reg >= DAVE_EH_MAX_DREG) {
    return 0;
  }
  rule = cfi->regs[reg];
  switch (rule.rule) {
    case DAVE_CFI_REG_UNDEFINED:
      return 0;
    case DAVE_CFI_REG_SAME:
      return DaveEHFrameGetReg(regs, reg);
    case DAVE_CFI_REG_OFFSET:
      slot = (uintptr_t)((intptr_t)cfa + rule.offset);
      if (slot < 0x1000) {
        return 0;
      }
      return *(uintptr_t*)slot;
    case DAVE_CFI_REG_VAL_OFFSET:
      return (uintptr_t)((intptr_t)cfa + rule.offset);
    case DAVE_CFI_REG_REGISTER:
      return DaveEHFrameGetReg(regs, rule.reg);
    default:
      return 0;
  }
}

static int FramePointerFallback(const DaveEHFrameRegisters* regs,
                                DaveEHFrameWalkResult* out) {
  uintptr_t caller_pc = 0;
  uintptr_t caller_rsp = 0;
  uintptr_t caller_rbp = 0;
#if defined(__x86_64__)
  const uintptr_t* frame;
  if (regs->rbp == 0 || (regs->rbp & (sizeof(uintptr_t) - 1)) != 0) {
    return 0;
  }
  frame = (const uintptr_t*)regs->rbp;
  caller_rbp = frame[0];
  caller_pc = frame[1];
  caller_rsp = regs->rbp + 2 * sizeof(uintptr_t);
  if (caller_rbp != 0 && caller_rbp <= regs->rbp) {
    return 0;
  }
#elif defined(__aarch64__)
  {
    const uintptr_t* frame = (const uintptr_t*)regs->rbp;
    if (regs->rbp == 0) {
      return 0;
    }
    caller_rbp = frame[0];
    caller_pc = frame[1];
    caller_rsp = regs->rbp + 2 * sizeof(uintptr_t);
  }
#elif defined(__riscv) || defined(__risc_v__)
  {
    const uintptr_t* frame = (const uintptr_t*)regs->rbp;
    if (regs->rbp == 0) {
      return 0;
    }
    caller_rbp = frame[-2];
    caller_pc = frame[-1];
    caller_rsp = regs->rbp;
  }
#else
  (void)regs;
  (void)out;
  return 0;
#endif
  out->caller = *regs;
  out->caller.pc = caller_pc;
  out->caller.rsp = caller_rsp;
  out->caller.rbp = caller_rbp;
  DaveEHFrameSyncCanonical(&out->caller);
  return out->caller.pc != 0;
}

int DaveEHFrameWalkFrame(const DaveEHFrameRegisters* regs,
                         DaveEHFrameWalkResult* out) {
  DaveEHFDE fde;
  DaveEHFDE previous_fde;
  DaveEHFrameCFI cfi;
  uintptr_t cfi_pc;
  uintptr_t cfa;
  int reg;

  if (regs == 0 || out == 0) {
    return 0;
  }
  if (!DaveEHFrameFindFDE(regs->pc, &fde)) {
    if (regs->pc == 0 || !DaveEHFrameFindFDE(regs->pc - 1, &fde)) {
      return FramePointerFallback(regs, out);
    }
  } else if (fde.pc_begin == regs->pc && regs->pc > 0 &&
             DaveEHFrameFindFDE(regs->pc - 1, &previous_fde) &&
             previous_fde.pc_end == regs->pc) {
    fde = previous_fde;
  }
  cfi_pc = regs->pc;
  if (cfi_pc >= fde.pc_end && cfi_pc > fde.pc_begin) {
    cfi_pc--;
  }
  if (!DaveEHFrameCFIAtPC(&fde, cfi_pc, &cfi)) {
    return FramePointerFallback(regs, out);
  }
  cfa = ComputeCFA(&cfi, regs);
  if (cfa < 0x1000) {
    return FramePointerFallback(regs, out);
  }

  out->caller = *regs;
  for (reg = 0; reg < DAVE_EH_MAX_DREG; reg++) {
    if (cfi.regs[reg].rule == DAVE_CFI_REG_SAME) {
      DaveEHFrameSetReg(&out->caller, reg, DaveEHFrameGetReg(regs, reg));
    } else if (cfi.regs[reg].rule == DAVE_CFI_REG_OFFSET ||
               cfi.regs[reg].rule == DAVE_CFI_REG_VAL_OFFSET ||
               cfi.regs[reg].rule == DAVE_CFI_REG_REGISTER) {
      DaveEHFrameSetReg(&out->caller, reg, ResolveCFIReg(&cfi, regs, reg, cfa));
    }
  }
  DaveEHFrameSetReg(&out->caller, cfi.ra_reg,
                    ResolveCFIReg(&cfi, regs, cfi.ra_reg, cfa));
  DaveEHFrameSetReg(&out->caller, DAVE_EH_DREG_SP, cfa);
  out->caller.pc = DaveEHFrameGetReg(&out->caller, cfi.ra_reg);
  out->caller.rsp = cfa;
  out->caller.rbp = DaveEHFrameGetReg(&out->caller, DAVE_EH_DREG_FP);
  DaveEHFrameSyncCanonical(&out->caller);
  if (out->caller.pc == 0) {
    return FramePointerFallback(regs, out);
  }
  return 1;
}

#endif /* LP64 CFI targets */
