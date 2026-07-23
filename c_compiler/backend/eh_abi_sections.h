#ifndef eh_abi_sections_h
#define eh_abi_sections_h

#include <stdio.h>

#include "codegen.h"
#include "vector.h"

const char* EHABIGxxPersonalitySymbol(void);
const char* EHABIARMUnwindCppPrSymbol(void);

typedef struct {
  const char* func_name;
  bool has_stack_frame;
  bool has_exceptions;
  int return_address_reg;
  int data_align_sleb;
  bool is_64bit;
} EHABIFrameParams;

void EHABIPrintItaniumTypeInfoAliases(FILE* fp, const Vector* typeinfos,
                                      bool is_64bit);
void EHABIPrintGccExceptTable(FILE* fp, const char* func_tag,
                              bool has_exceptions, bool is_64bit);
void EHABIPrintDwarfEHFrame(FILE* fp, const EHABIFrameParams* params);
void EHABIPrintARMExidxExtab(FILE* fp, const char* func_name,
                             bool has_exceptions);

#endif /* eh_abi_sections_h */
