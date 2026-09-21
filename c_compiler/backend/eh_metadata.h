#ifndef eh_metadata_h
#define eh_metadata_h

#include <stdbool.h>
#include <stdio.h>
#include "asm_module.h"

// Itanium/GCC LSDA and EH-frame constants shared by target emitters.

#define DAVECC_EH_PERSONALITY "__gxx_personality_v0"

// Itanium action filter zero denotes cleanup-only.
#define DAVECC_EH_LSDA_CLEANUP_FILTER 0

typedef struct {
  long long try_start_id;
  long long try_end_id;
  long long landing_pad_id;
  const char* catch_typeinfo;  // NULL = catch(...); ignored when is_cleanup
  bool is_cleanup;
} DaveEHLSDARange;

typedef struct {
  int dwarf_reg;
  int cfa_offset;
} DaveEHFrameSavedReg;

typedef struct {
  const DaveEHLSDARange* ranges;
  size_t range_count;
  const char* func_name;
  bool has_frame;
  bool is_64bit;
  int cie_ra_reg;
  int cie_cfa_reg;
  int cie_fp_reg;
  int entry_cfa_offset;
  int frame_cfa_offset;
  int fp_cfa_offset;
  int saved_fp_offset;
  int saved_ra_offset;
  const DaveEHFrameSavedReg* saved_regs;
  size_t saved_reg_count;
} DaveEHFrameEmitInfo;

void DaveEHPrintUleb128(FILE* fp, unsigned long long value);
void DaveEHPrintSleb128(FILE* fp, long long value);
// Local label at the function's .text start. FDE PC must relocate against
// this, not the exported name: GNU ld rejects FDEs whose PC reloc binds to
// another object's winning weak definition.
void DaveEHPrintFuncTextLabel(FILE* fp, const char* func_name);
void DaveEHEmitFuncTextLabel(AsmModule* module, const char* func_name);

void DaveEHPrintGCCExceptTable(FILE* fp, const DaveEHFrameEmitInfo* info);
void DaveEHPrintARMExtabLSDA(FILE* fp, const DaveEHFrameEmitInfo* info);
void DaveEHPrintEHFrameCIE(FILE* fp, const DaveEHFrameEmitInfo* info,
                           const char* cie_label_suffix);
void DaveEHPrintEHFrameFDE(FILE* fp, const DaveEHFrameEmitInfo* info,
                           const char* cie_label_suffix);
void DaveEHEmitGCCExceptTable(AsmModule* module,
                              const DaveEHFrameEmitInfo* info);
void DaveEHEmitEHFrameCIE(AsmModule* module,
                          const DaveEHFrameEmitInfo* info,
                          const char* cie_label_suffix);
void DaveEHEmitEHFrameFDE(AsmModule* module,
                          const DaveEHFrameEmitInfo* info,
                          const char* cie_label_suffix);

#endif /* eh_metadata_h */
