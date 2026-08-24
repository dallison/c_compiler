#include "eh_abi_sections.h"

#include <stdio.h>

const char* EHABIGxxPersonalitySymbol(void) { return "__gxx_personality_v0"; }

const char* EHABIARMUnwindCppPrSymbol(void) { return "__aeabi_unwind_cpp_pr1"; }

static void PrintEscapedAsmString(FILE* fp, const char* s) {
  for (; *s != '\0'; s++) {
    unsigned char ch = (unsigned char)*s;
    if (ch == '"' || ch == '\\') {
      fprintf(fp, "\\%c", ch);
    } else if (ch >= 32 && ch < 127) {
      fputc(ch, fp);
    } else {
      fprintf(fp, "\\%03o", ch);
    }
  }
}

static void PrintItaniumTypeStringSymbolName(FILE* fp, const char* type_name) {
  fprintf(fp, "_ZTS");
  for (const char* p = type_name; *p != '\0'; p++) {
    unsigned char ch = (unsigned char)*p;
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') || ch == '_') {
      fputc(ch, fp);
    } else {
      fputc('_', fp);
    }
  }
}

static void PrintItaniumTypeInfoSymbolName(FILE* fp, const char* type_name) {
  fprintf(fp, "_ZTI");
  for (const char* p = type_name; *p != '\0'; p++) {
    unsigned char ch = (unsigned char)*p;
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') || ch == '_') {
      fputc(ch, fp);
    } else {
      fputc('_', fp);
    }
  }
}

void EHABIPrintItaniumTypeInfoAliases(FILE* fp, const Vector* typeinfos,
                                      bool is_64bit) {
  if (typeinfos == NULL || typeinfos->length == 0) {
    return;
  }

  fprintf(fp, "\t.section \".rodata\", \"a\", @progbits\n");
  for (size_t i = 0; i < typeinfos->length; i++) {
    EHTypeInfo* info = typeinfos->value.p[i];
    const char* davecc_name = info->symbol_name.value;
    const char* type_name = info->type_name.value;

    fprintf(fp, "\t.p2align %d\n", is_64bit ? 3 : 2);
    fprintf(fp, "\t.local ");
    PrintItaniumTypeStringSymbolName(fp, type_name);
    fprintf(fp, "_name\n");
    PrintItaniumTypeStringSymbolName(fp, type_name);
    fprintf(fp, "_name:\n\t.asciz \"");
    PrintEscapedAsmString(fp, type_name);
    fprintf(fp, "\"\n");

    fprintf(fp, "\t.weak ");
    PrintItaniumTypeStringSymbolName(fp, type_name);
    fprintf(fp, "\n");
    PrintItaniumTypeStringSymbolName(fp, type_name);
    fprintf(fp, ":\n");
    if (is_64bit) {
      fprintf(fp, "\t.8byte ");
    } else {
      fprintf(fp, "\t.4byte ");
    }
    PrintItaniumTypeStringSymbolName(fp, type_name);
    fprintf(fp, "_name\n");

    fprintf(fp, "\t.weak ");
    PrintItaniumTypeInfoSymbolName(fp, type_name);
    fprintf(fp, "\n");
    PrintItaniumTypeInfoSymbolName(fp, type_name);
    fprintf(fp, ":\n");
    if (is_64bit) {
      fprintf(fp, "\t.8byte %s\n", davecc_name);
    } else {
      fprintf(fp, "\t.4byte %s\n", davecc_name);
    }
  }
  fprintf(fp, "\t.text\n\n");
}

void EHABIPrintGccExceptTable(FILE* fp, const char* func_tag,
                              bool has_exceptions, bool is_64bit) {
  if (!has_exceptions) {
    return;
  }

  fprintf(fp, "\t.section \".gcc_except_table\", \"a\", @progbits\n");
  fprintf(fp, "\t.p2align 2\n");
  fprintf(fp, ".Leh_abi_lsda_%s:\n", func_tag);
  fprintf(fp, "\t.byte 0xff\n");
  fprintf(fp, "\t.byte 0xff\n");
  fprintf(fp, "\t.byte 0\n");
  if (is_64bit) {
    fprintf(fp, "\t.8byte 0\n");
  } else {
    fprintf(fp, "\t.4byte 0\n");
  }
  fprintf(fp, "\t.text\n\n");
}

static void PrintUleb128(FILE* fp, int value) {
  unsigned int v = (unsigned int)value;
  do {
    unsigned char byte = v & 0x7f;
    v >>= 7;
    if (v != 0) {
      byte |= 0x80;
    }
    fprintf(fp, "\t.byte %u\n", byte);
  } while (v != 0);
}

static void PrintSleb128(FILE* fp, int value) {
  int more = 1;
  int negative = value < 0;
  unsigned int v = (unsigned int)value;
  while (more) {
    unsigned char byte = v & 0x7f;
    v >>= 7;
    if (negative) {
      more = !(((byte & 0x40) != 0) && ((v == 0) || ((v & 0x7f) == 0x7f)));
    } else {
      more = (v != 0);
    }
    if (more) {
      byte |= 0x80;
    }
    fprintf(fp, "\t.byte %u\n", byte);
  }
}

void EHABIPrintDwarfEHFrame(FILE* fp, const EHABIFrameParams* params) {
  if (params == NULL || params->func_name == NULL) {
    return;
  }

  const char* func_name = params->func_name;
  bool has_frame = params->has_stack_frame;
  const char* align_op = params->is_64bit ? ".8byte" : ".4byte";

  fprintf(fp, "\t.section \".eh_frame\", \"a\", @progbits\n");

  fprintf(fp, ".Leh_%s_cie:\n", func_name);
  fprintf(fp, "\t.4byte 20\n");
  fprintf(fp, ".Leh_%s_cie_start:\n", func_name);
  fprintf(fp, "\t.4byte 0\n");
  fprintf(fp, "\t.byte 1\n");
  fprintf(fp, "\t.asciz \"zR\"\n");
  fprintf(fp, "\t.byte 1\n");
  PrintSleb128(fp, params->data_align_sleb);
  PrintUleb128(fp, params->return_address_reg);
  fprintf(fp, "\t.byte 1\n");
  fprintf(fp, "\t.byte 0\n");
  fprintf(fp, "\t.byte 12, 7, %d\n", params->is_64bit ? 8 : 4);
  if (params->is_64bit) {
    fprintf(fp, "\t.byte 144, 1\n");
  } else {
    fprintf(fp, "\t.byte 141, 1\n");
  }
  fprintf(fp, ".Leh_%s_cie_end:\n", func_name);

  int fde_body = has_frame ? (params->is_64bit ? 38 : 30) : (params->is_64bit ? 21 : 17);

  fprintf(fp, ".Leh_%s_fde:\n", func_name);
  fprintf(fp, "\t.4byte %d\n", fde_body);
  fprintf(fp, ".Leh_%s_fde_start:\n", func_name);
  fprintf(fp, "\t.4byte .Leh_%s_fde_start-.Leh_%s_cie\n", func_name,
          func_name);
  fprintf(fp, "\t%s %s\n", align_op, func_name);
  fprintf(fp, "\t%s (.func_end_%s-%s)\n", align_op, func_name, func_name);
  fprintf(fp, "\t.byte 0\n");
  if (has_frame) {
    fprintf(fp, "\t.byte 4\n");
    fprintf(fp, "\t.4byte (.Leh_%s_after_push-%s)\n", func_name, func_name);
    if (params->is_64bit) {
      fprintf(fp, "\t.byte 14, 16\n");
      fprintf(fp, "\t.byte 134, 2\n");
      fprintf(fp, "\t.byte 4\n");
      fprintf(fp, "\t.4byte (.Leh_%s_after_leaq-.Leh_%s_after_push)\n",
              func_name, func_name);
      fprintf(fp, "\t.byte 12, 6, 8\n");
    } else {
      fprintf(fp, "\t.byte 14, 13\n");
      fprintf(fp, "\t.byte 134, 2\n");
      fprintf(fp, "\t.byte 4\n");
      fprintf(fp, "\t.4byte (.Leh_%s_after_push-%s)\n", func_name, func_name);
      fprintf(fp, "\t.byte 12, 5, %d\n", params->is_64bit ? 8 : 4);
    }
  }
  fprintf(fp, ".Leh_%s_fde_end:\n", func_name);
  fprintf(fp, "\t.text\n\n");
}

void EHABIPrintARMExidxExtab(FILE* fp, const char* func_name,
                             bool has_exceptions) {
  if (!has_exceptions || func_name == NULL) {
    return;
  }

  fprintf(fp, "\t.section \".ARM.extab\", \"a\", @progbits\n");
  fprintf(fp, "\t.p2align 2\n");
  fprintf(fp, ".Leh_arm_extab_%s:\n", func_name);
  fprintf(fp, "\t.4byte 0x80000000\n");
  fprintf(fp, "\t.4byte 0\n");
  fprintf(fp, "\t.4byte .Leh_arm_extab_%s\n", func_name);
  fprintf(fp, "\t.4byte 0\n");
  fprintf(fp, "\t.text\n\n");

  fprintf(fp, "\t.section \".ARM.exidx\", \"a\", @progbits\n");
  fprintf(fp, "\t.p2align 2\n");
  fprintf(fp, ".Leh_arm_exidx_%s:\n", func_name);
  fprintf(fp, "\t.4byte (%s - .Leh_arm_exidx_%s)\n", func_name, func_name);
  fprintf(fp, "\t.4byte (.Leh_arm_extab_%s - .Leh_arm_exidx_%s)\n", func_name,
          func_name);
  fprintf(fp, "\t.text\n\n");
}
