#include "eh_metadata.h"

#include <string.h>

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

void DaveEHPrintGCCExceptTable(FILE* fp, const DaveEHFrameEmitInfo* info) {
  if (info == NULL || info->range_count == 0) {
    return;
  }

  const char* func = info->func_name;
  size_t type_count = 0;
  const char* type_symbols[64];

  for (size_t i = 0; i < info->range_count; i++) {
    const DaveEHLSDARange* range = &info->ranges[i];
    if (range->is_cleanup || range->catch_typeinfo == NULL) {
      continue;
    }
    const char* sym = range->catch_typeinfo;
    size_t j = 0;
    for (; j < type_count; j++) {
      if (strcmp(type_symbols[j], sym) == 0) {
        break;
      }
    }
    if (j == type_count && type_count < sizeof(type_symbols) / sizeof(type_symbols[0])) {
      type_symbols[type_count++] = sym;
    }
  }

  fprintf(fp, "\t.section \".gcc_except_table\", \"a\", @progbits\n");
  fprintf(fp, ".Leh_%s_lsda:\n", func);

  // @LPStart omitted: landing pad offsets are relative to function start.
  fprintf(fp, "\t.byte 0xff\n");

  if (type_count > 0) {
    fprintf(fp, "\t.byte 0x1b\n");
    fprintf(fp, "\t.4byte .Leh_%s_ttype-.Leh_%s_lsda\n", func, func);
  } else {
    fprintf(fp, "\t.byte 0xff\n");
  }

  fprintf(fp, "\t.uleb128 .Leh_%s_cs_end-.Leh_%s_cs_start\n", func, func);
  fprintf(fp, ".Leh_%s_cs_start:\n", func);

  for (size_t i = 0; i < info->range_count; i++) {
    const DaveEHLSDARange* range = &info->ranges[i];
    fprintf(fp, "\t.uleb128 ");
    PrintOffsetFromFunc(fp, func, 0, range->try_start_id);
    fprintf(fp, "\n\t.uleb128 ");
    PrintOffsetFromFunc(fp, func, range->try_start_id, range->try_end_id);
    fprintf(fp, "\n\t.uleb128 ");
    PrintOffsetFromFunc(fp, func, 0, range->landing_pad_id);
    fprintf(fp, "\n\t.uleb128 %zu\n", i + 1);
  }
  fprintf(fp, ".Leh_%s_cs_end:\n", func);

  for (size_t i = 0; i < info->range_count; i++) {
    const DaveEHLSDARange* range = &info->ranges[i];
    long long type_filter;
    if (range->is_cleanup) {
      type_filter = DAVECC_EH_LSDA_CLEANUP_FILTER;
    } else if (range->catch_typeinfo == NULL) {
      type_filter = 0;
    } else {
      const char* sym = range->catch_typeinfo;
      size_t index = 1;
      for (size_t t = 0; t < type_count; t++) {
        if (strcmp(type_symbols[t], sym) == 0) {
          index = t + 1;
          break;
        }
      }
      type_filter = (long long)index;
    }
    fprintf(fp, "\t.sleb128 %lld\n", type_filter);
    fprintf(fp, "\t.sleb128 0\n");
  }

  if (type_count > 0) {
    for (size_t t = 0; t < type_count; t++) {
      fprintf(fp, "\t.8byte %s\n", type_symbols[t]);
    }
    fprintf(fp, ".Leh_%s_ttype:\n", func);
  }

  fprintf(fp, "\t.text\n\n");
}

void DaveEHPrintEHFrameCIE(FILE* fp, const DaveEHFrameEmitInfo* info,
                           const char* cie_label_suffix) {
  const char* func = info->func_name;
  bool with_eh = info->range_count > 0;

  fprintf(fp, "\t.section \".eh_frame\", \"a\", @progbits\n");
  if (with_eh) {
    fprintf(fp, "\t.weak %s\n", DAVECC_EH_PERSONALITY);
  }
  fprintf(fp, ".Leh_%s_cie%s:\n", func, cie_label_suffix);
  fprintf(fp, "\t.4byte %d\n", with_eh ? 26 : 18);
  fprintf(fp, ".Leh_%s_cie_start%s:\n", func, cie_label_suffix);
  fprintf(fp, "\t.4byte 0\n");
  fprintf(fp, "\t.byte 1\n");
  fprintf(fp, with_eh ? "\t.asciz \"zPLR\"\n" : "\t.asciz \"zR\"\n");
  fprintf(fp, "\t.byte 1\n");
  fprintf(fp, "\t.byte 120\n");
  fprintf(fp, "\t.byte %d\n", info->cie_ra_reg);
  if (with_eh) {
    fprintf(fp, "\t.byte 7\n");
    fprintf(fp, "\t.byte 0x1b\n");
    fprintf(fp, "\t.4byte %s-.Leh_%s_cie_start%s\n", DAVECC_EH_PERSONALITY, func,
            cie_label_suffix);
    fprintf(fp, "\t.byte 0x1b\n");
    fprintf(fp, "\t.byte 0x10\n");
  } else {
    fprintf(fp, "\t.byte 1\n");
    fprintf(fp, "\t.byte 0\n");
  }
  fprintf(fp, "\t.byte 12, %d, 8\n", info->cie_cfa_reg);
  fprintf(fp, "\t.byte 144, 1\n");
  fprintf(fp, ".Leh_%s_cie_end%s:\n", func, cie_label_suffix);
}

void DaveEHPrintEHFrameFDE(FILE* fp, const DaveEHFrameEmitInfo* info,
                           const char* cie_label_suffix) {
  const char* func = info->func_name;
  bool with_eh = info->range_count > 0;
  bool has_frame = info->has_frame;

  fprintf(fp, ".Leh_%s_fde:\n", func);
  fprintf(fp, "\t.4byte %d\n",
          with_eh ? (has_frame ? 42 : 25) : (has_frame ? 38 : 21));
  fprintf(fp, ".Leh_%s_fde_start:\n", func);
  fprintf(fp, "\t.4byte .Leh_%s_fde_start-.Leh_%s_cie%s\n", func, func,
          cie_label_suffix);
  fprintf(fp, "\t.8byte %s\n", func);
  fprintf(fp, "\t.8byte (.func_end_%s-%s)\n", func, func);
  if (with_eh) {
    fprintf(fp, "\t.byte 4\n");
    fprintf(fp, "\t.4byte .Leh_%s_lsda-.Leh_%s_fde_start\n", func, func);
  } else {
    fprintf(fp, "\t.byte 0\n");
  }
  if (has_frame) {
    fprintf(fp, "\t.byte 4\n");
    fprintf(fp, "\t.4byte (.Leh_%s_after_push-%s)\n", func, func);
    fprintf(fp, "\t.byte 14, 16\n");
    fprintf(fp, "\t.byte 134, 2\n");
    fprintf(fp, "\t.byte 4\n");
    fprintf(fp, "\t.4byte (.Leh_%s_after_leaq-.Leh_%s_after_push)\n", func,
            func);
    fprintf(fp, "\t.byte 12, %d, 8\n", info->cie_fp_reg);
  }
  fprintf(fp, ".Leh_%s_fde_end:\n", func);
  fprintf(fp, "\t.text\n\n");
}
