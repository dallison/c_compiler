#include "eh_metadata.h"

#include <stdlib.h>
#include <string.h>

// DWARF EH pointer encodings used in LSDA headers.
#define DW_EH_PE_omit 0xff
#define DW_EH_PE_uleb128 0x01
#define DW_EH_PE_pcrel_sdata4 0x1b

typedef struct {
  long long type_filter;
} LSDAActionEntry;

static size_t Sleb128Size(long long value) {
  size_t size = 0;
  int more = 1;
  while (more) {
    unsigned char byte = (unsigned char)(value & 0x7f);
    value >>= 7;
    if ((value == 0 && (byte & 0x40) == 0) ||
        (value == -1 && (byte & 0x40) != 0)) {
      more = 0;
    }
    size++;
  }
  return size;
}

static size_t Uleb128Size(unsigned long long value) {
  size_t size = 1;
  while (value >= 0x80) {
    value >>= 7;
    size++;
  }
  return size;
}

typedef struct {
  long long try_start_id;
  long long try_end_id;
  long long landing_pad_id;
  size_t first_action;
  size_t action_count;
} LSDACallSiteGroup;

void DaveEHPrintUleb128(FILE* fp, unsigned long long value) {
  do {
    unsigned char byte = (unsigned char)(value & 0x7f);
    value >>= 7;
    if (value != 0) {
      byte |= 0x80;
    }
    fprintf(fp, "\t.byte 0x%02x\n", byte);
  } while (value != 0);
}

void DaveEHPrintSleb128(FILE* fp, long long value) {
  int more = 1;
  while (more) {
    unsigned char byte = (unsigned char)(value & 0x7f);
    value >>= 7;
    if ((value == 0 && (byte & 0x40) == 0) ||
        (value == -1 && (byte & 0x40) != 0)) {
      more = 0;
    } else {
      byte |= 0x80;
    }
    fprintf(fp, "\t.byte 0x%02x\n", byte);
  }
}

static void PrintInsnLabel(FILE* fp, const char* func_name, long long label_id) {
  fprintf(fp, ".%s_label_%lld", func_name, label_id);
}

static void PrintOffsetFromFunc(FILE* fp, const char* func_name,
                                long long from_id, long long to_id) {
  if (from_id == to_id) {
    fprintf(fp, "0");
    return;
  }
  fprintf(fp, "(");
  PrintInsnLabel(fp, func_name, to_id);
  fprintf(fp, "-");
  if (from_id == 0) {
    fprintf(fp, "%s", func_name);
  } else {
    PrintInsnLabel(fp, func_name, from_id);
  }
  fprintf(fp, ")");
}

static size_t FindTypeIndex(const char* const* type_symbols, size_t type_count,
                            const char* sym) {
  for (size_t i = 0; i < type_count; i++) {
    if ((type_symbols[i] == NULL && sym == NULL) ||
        (type_symbols[i] != NULL && sym != NULL &&
         strcmp(type_symbols[i], sym) == 0)) {
      return i + 1;
    }
  }
  return 0;
}

static long long RangeTypeFilter(const DaveEHLSDARange* range,
                                 const char* const* type_symbols,
                                 size_t type_count) {
  if (range->is_cleanup) {
    return DAVECC_EH_LSDA_CLEANUP_FILTER;
  }
  return (long long)FindTypeIndex(type_symbols, type_count, range->catch_typeinfo);
}

static int CallSiteGroupLess(const LSDACallSiteGroup* a,
                             const LSDACallSiteGroup* b) {
  if (a->try_start_id != b->try_start_id) {
    return a->try_start_id < b->try_start_id ? -1 : 1;
  }
  if (a->try_end_id != b->try_end_id) {
    return a->try_end_id < b->try_end_id ? -1 : 1;
  }
  if (a->landing_pad_id != b->landing_pad_id) {
    return a->landing_pad_id < b->landing_pad_id ? -1 : 1;
  }
  return 0;
}

static void DaveEHPrintExceptTable(FILE* fp, const DaveEHFrameEmitInfo* info,
                                   const char* section_name) {
  if (info == NULL || info->range_count == 0) {
    return;
  }

  const char* func = info->func_name;
  const char* type_symbols[64];
  size_t type_count = 0;
  bool has_catch_all = false;

  for (size_t i = 0; i < info->range_count; i++) {
    const DaveEHLSDARange* range = &info->ranges[i];
    if (range->is_cleanup) {
      continue;
    }
    if (range->catch_typeinfo == NULL) {
      has_catch_all = true;
      continue;
    }
    if (FindTypeIndex(type_symbols, type_count, range->catch_typeinfo) != 0) {
      continue;
    }
    if (type_count >= sizeof(type_symbols) / sizeof(type_symbols[0])) {
      continue;
    }
    type_symbols[type_count++] = range->catch_typeinfo;
  }
  // A catch-all is represented by a positive filter whose type-table entry is
  // null. Keep it after all typed entries so typed selector numbering remains
  // stable and deterministic.
  if (has_catch_all &&
      type_count < sizeof(type_symbols) / sizeof(type_symbols[0])) {
    type_symbols[type_count++] = NULL;
  }

  LSDACallSiteGroup groups[64];
  size_t group_count = 0;
  LSDAActionEntry actions[128];
  size_t action_count = 0;

  for (size_t i = 0; i < info->range_count; i++) {
    const DaveEHLSDARange* range = &info->ranges[i];
    size_t g = 0;
    for (; g < group_count; g++) {
      if (groups[g].try_start_id == range->try_start_id &&
          groups[g].try_end_id == range->try_end_id &&
          groups[g].landing_pad_id == range->landing_pad_id) {
        break;
      }
    }
    if (g == group_count) {
      if (group_count >= sizeof(groups) / sizeof(groups[0]) ||
          action_count >= sizeof(actions) / sizeof(actions[0])) {
        continue;
      }
      groups[group_count].try_start_id = range->try_start_id;
      groups[group_count].try_end_id = range->try_end_id;
      groups[group_count].landing_pad_id = range->landing_pad_id;
      groups[group_count].first_action = action_count;
      groups[group_count].action_count = 0;
      group_count++;
    }

    if (action_count >= sizeof(actions) / sizeof(actions[0])) {
      continue;
    }
    actions[action_count].type_filter = RangeTypeFilter(range, type_symbols,
                                                      type_count);
    groups[g].action_count++;
    action_count++;
  }

  for (size_t i = 0; i < group_count; i++) {
    for (size_t j = i + 1; j < group_count; j++) {
      if (CallSiteGroupLess(&groups[j], &groups[i]) < 0) {
        LSDACallSiteGroup tmp = groups[i];
        groups[i] = groups[j];
        groups[j] = tmp;
      }
    }
  }
  // Sorting call sites must also reorder their action slices. The action field
  // is a byte offset into the action table; retaining pre-sort indices would
  // associate each landing pad with another range's handler.
  LSDAActionEntry sorted_actions[128];
  size_t sorted_action_count = 0;
  for (size_t i = 0; i < group_count; i++) {
    size_t old_first = groups[i].first_action;
    groups[i].first_action = sorted_action_count;
    for (size_t j = 0; j < groups[i].action_count; j++) {
      sorted_actions[sorted_action_count++] = actions[old_first + j];
    }
  }
  memcpy(actions, sorted_actions,
         sorted_action_count * sizeof(LSDAActionEntry));
  action_count = sorted_action_count;
  size_t action_byte_offset = 0;
  for (size_t i = 0; i < group_count; i++) {
    groups[i].first_action = action_byte_offset;
    for (size_t j = 0; j < groups[i].action_count; j++) {
      size_t index = 0;
      for (size_t k = 0; k < i; k++) {
        index += groups[k].action_count;
      }
      index += j;
      action_byte_offset += Sleb128Size(actions[index].type_filter) + 1;
    }
  }

  fprintf(fp, "\t.section \"%s\", \"a\", @progbits\n", section_name);
  fprintf(fp, "\t.align 2\n");
  fprintf(fp, ".Leh_%s_lsda:\n", func);

  // @LPStart omitted: landing pad offsets are relative to function start.
  fprintf(fp, "\t.byte 0x%02x\n", DW_EH_PE_omit);

  if (type_count > 0) {
    fprintf(fp, "\t.byte 0x%02x\n", DW_EH_PE_pcrel_sdata4);
    fprintf(fp, "\t.uleb128 .Leh_%s_ttype_end-.Leh_%s_ttype_base\n", func,
            func);
    fprintf(fp, ".Leh_%s_ttype_base:\n", func);
  } else {
    fprintf(fp, "\t.byte 0x%02x\n", DW_EH_PE_omit);
  }

  fprintf(fp, "\t.byte 0x%02x\n", DW_EH_PE_uleb128);
  fprintf(fp, "\t.uleb128 .Leh_%s_cs_end-.Leh_%s_cs_start\n", func, func);
  fprintf(fp, ".Leh_%s_cs_start:\n", func);

  for (size_t i = 0; i < group_count; i++) {
    const LSDACallSiteGroup* group = &groups[i];
    fprintf(fp, "\t.uleb128 ");
    PrintOffsetFromFunc(fp, func, 0, group->try_start_id);
    fprintf(fp, "\n\t.uleb128 ");
    PrintOffsetFromFunc(fp, func, group->try_start_id, group->try_end_id);
    fprintf(fp, "\n\t.uleb128 ");
    PrintOffsetFromFunc(fp, func, 0, group->landing_pad_id);
    fprintf(fp, "\n\t.uleb128 %zu\n", group->first_action + 1);
  }
  fprintf(fp, ".Leh_%s_cs_end:\n", func);

  size_t emitted_action = 0;
  for (size_t i = 0; i < group_count; i++) {
    const LSDACallSiteGroup* group = &groups[i];
    for (size_t j = 0; j < group->action_count; j++) {
      fprintf(fp, "\t.sleb128 %lld\n",
              actions[emitted_action++].type_filter);
      // The displacement is relative to its own field. A one-byte value of 1
      // therefore points at the immediately following action record.
      fprintf(fp, "\t.sleb128 %d\n",
              j + 1 < group->action_count ? 1 : 0);
    }
  }

  if (type_count > 0) {
    fprintf(fp, ".Leh_%s_ttype:\n", func);
    for (size_t t = type_count; t-- > 0;) {
      fprintf(fp, ".Leh_%s_ttype_entry_%zu:\n", func, t);
      if (type_symbols[t] == NULL) {
        fprintf(fp, "\t.4byte 0\n");
      } else {
        fprintf(fp, "\t.4byte %s-.Leh_%s_ttype_entry_%zu\n", type_symbols[t],
                func, t);
      }
    }
    fprintf(fp, ".Leh_%s_ttype_end:\n", func);
  }

  fprintf(fp, "\t.align 2\n");
  fprintf(fp, "\t.text\n\n");
}

void DaveEHPrintGCCExceptTable(FILE* fp, const DaveEHFrameEmitInfo* info) {
  DaveEHPrintExceptTable(fp, info, ".gcc_except_table");
}

void DaveEHPrintARMExtabLSDA(FILE* fp, const DaveEHFrameEmitInfo* info) {
  DaveEHPrintExceptTable(fp, info, ".ARM.extab");
}

static size_t EHFrameCIELength(const DaveEHFrameEmitInfo* info) {
  bool with_eh = info->range_count > 0;
  size_t initial_cfi_size =
      1 + Uleb128Size(info->cie_cfa_reg) +
      Uleb128Size(info->entry_cfa_offset);
  if (info->saved_ra_offset != 0 &&
      info->saved_ra_offset == -info->entry_cfa_offset) {
    initial_cfi_size +=
        1 + Uleb128Size((unsigned long long)(-info->saved_ra_offset / 8));
  }
  return 4 + 1 + (with_eh ? 5 : 3) + Uleb128Size(1) + Sleb128Size(-8) +
         Uleb128Size(info->cie_ra_reg) + Uleb128Size(with_eh ? 7 : 1) +
         (with_eh ? 7 : 1) + initial_cfi_size;
}

void DaveEHPrintEHFrameCIE(FILE* fp, const DaveEHFrameEmitInfo* info,
                           const char* cie_label_suffix) {
  const char* func = info->func_name;
  bool with_eh = info->range_count > 0;
  size_t cie_length = EHFrameCIELength(info);

  fprintf(fp, "\t.section \".eh_frame\", \"a\", @progbits\n");
  fprintf(fp, "\t.align 3\n");
  if (with_eh) {
    fprintf(fp, "\t.weak %s\n", DAVECC_EH_PERSONALITY);
  }
  fprintf(fp, ".Leh_%s_cie%s:\n", func, cie_label_suffix);
  fprintf(fp, "\t.4byte %zu\n", cie_length);
  fprintf(fp, ".Leh_%s_cie_start%s:\n", func, cie_label_suffix);
  fprintf(fp, "\t.4byte 0\n");
  fprintf(fp, "\t.byte 1\n");
  fprintf(fp, with_eh ? "\t.asciz \"zPLR\"\n" : "\t.asciz \"zR\"\n");
  DaveEHPrintUleb128(fp, 1);
  DaveEHPrintSleb128(fp, -8);
  DaveEHPrintUleb128(fp, info->cie_ra_reg);
  if (with_eh) {
    DaveEHPrintUleb128(fp, 7);
    fprintf(fp, "\t.byte 0x1b\n");
    fprintf(fp, ".Leh_%s_cie_pers_ref%s:\n", func, cie_label_suffix);
    fprintf(fp, "\t.4byte %s-.Leh_%s_cie_pers_ref%s\n",
            DAVECC_EH_PERSONALITY, func, cie_label_suffix);
    fprintf(fp, "\t.byte 0x1b\n");
    fprintf(fp, "\t.byte 0x1b\n");
  } else {
    DaveEHPrintUleb128(fp, 1);
    fprintf(fp, "\t.byte 0x1b\n");
  }
  fprintf(fp, "\t.byte 12\n");
  DaveEHPrintUleb128(fp, info->cie_cfa_reg);
  DaveEHPrintUleb128(fp, info->entry_cfa_offset);
  if (info->saved_ra_offset != 0 &&
      info->saved_ra_offset == -info->entry_cfa_offset) {
    fprintf(fp, "\t.byte 0x%x\n", 0x80 | info->cie_ra_reg);
    DaveEHPrintUleb128(fp, -info->saved_ra_offset / 8);
  }
  fprintf(fp, ".Leh_%s_cie_end%s:\n", func, cie_label_suffix);
}

void DaveEHPrintEHFrameFDE(FILE* fp, const DaveEHFrameEmitInfo* info,
                           const char* cie_label_suffix) {
  const char* func = info->func_name;
  (void)cie_label_suffix;
  bool with_eh = info->range_count > 0;
  bool has_frame = info->has_frame;
  size_t fde_length = 12 + Uleb128Size(with_eh ? 4 : 0) +
                      (with_eh ? 4 : 0);
  if (has_frame) {
    fde_length += 5;
    fde_length += 1 + Uleb128Size(info->frame_cfa_offset);
    fde_length +=
        1 + Uleb128Size((unsigned long long)(-info->saved_fp_offset / 8));
    if (info->saved_ra_offset != 0 &&
        info->saved_ra_offset != -info->entry_cfa_offset) {
      fde_length +=
          1 + Uleb128Size((unsigned long long)(-info->saved_ra_offset / 8));
    }
    fde_length += 5;
    fde_length += 1 + Uleb128Size(info->cie_fp_reg) +
                  Uleb128Size(info->fp_cfa_offset);
    for (size_t i = 0; i < info->saved_reg_count; i++) {
      const DaveEHFrameSavedReg* saved = &info->saved_regs[i];
      if (saved->dwarf_reg >= 0 && saved->dwarf_reg < 64 &&
          saved->cfa_offset < 0 && saved->cfa_offset % 8 == 0) {
        fde_length +=
            1 + Uleb128Size((unsigned long long)(-saved->cfa_offset / 8));
      }
    }
  }

  fprintf(fp, ".Leh_%s_fde:\n", func);
  fprintf(fp, "\t.4byte %zu\n", fde_length);
  fprintf(fp, ".Leh_%s_fde_start:\n", func);
  fprintf(fp, "\t.4byte %zu\n", EHFrameCIELength(info) + 8);
  fprintf(fp, ".Leh_%s_fde_pc:\n", func);
  fprintf(fp, "\t.4byte %s-.Leh_%s_fde_pc\n", func, func);
  fprintf(fp, "\t.4byte (.func_end_%s-%s)\n", func, func);
  if (with_eh) {
    DaveEHPrintUleb128(fp, 4);
    fprintf(fp, ".Leh_%s_fde_lsda_ref:\n", func);
    fprintf(fp, "\t.4byte .Leh_%s_lsda-.Leh_%s_fde_lsda_ref\n", func, func);
  } else {
    DaveEHPrintUleb128(fp, 0);
  }
  if (has_frame) {
    fprintf(fp, "\t.byte 4\n");
    fprintf(fp, "\t.4byte (.Leh_%s_after_push-%s)\n", func, func);
    fprintf(fp, "\t.byte 14\n");
    DaveEHPrintUleb128(fp, info->frame_cfa_offset);
    fprintf(fp, "\t.byte 0x%x\n", 0x80 | info->cie_fp_reg);
    DaveEHPrintUleb128(fp, -info->saved_fp_offset / 8);
    if (info->saved_ra_offset != 0 &&
        info->saved_ra_offset != -info->entry_cfa_offset) {
      fprintf(fp, "\t.byte 0x%x\n", 0x80 | info->cie_ra_reg);
      DaveEHPrintUleb128(fp, -info->saved_ra_offset / 8);
    }
    fprintf(fp, "\t.byte 4\n");
    fprintf(fp, "\t.4byte (.Leh_%s_after_leaq-.Leh_%s_after_push)\n", func,
            func);
    fprintf(fp, "\t.byte 12\n");
    DaveEHPrintUleb128(fp, info->cie_fp_reg);
    DaveEHPrintUleb128(fp, info->fp_cfa_offset);
    for (size_t i = 0; i < info->saved_reg_count; i++) {
      const DaveEHFrameSavedReg* saved = &info->saved_regs[i];
      if (saved->dwarf_reg >= 0 && saved->dwarf_reg < 64 &&
          saved->cfa_offset < 0 && saved->cfa_offset % 8 == 0) {
        fprintf(fp, "\t.byte 0x%x\n", 0x80 | saved->dwarf_reg);
        DaveEHPrintUleb128(fp, -saved->cfa_offset / 8);
      }
    }
  }
  fprintf(fp, ".Leh_%s_fde_end:\n", func);
  fprintf(fp, "\t.align 3\n");
  fprintf(fp, "\t.text\n\n");
}
