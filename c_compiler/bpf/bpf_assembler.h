#ifndef bpf_assembler_h
#define bpf_assembler_h

#include "assembler.h"
#include "map.h"

typedef struct {
  Assembler base;
  Map instructions;
  int32_t bss;
} BPFAssembler;

bool BPFAssemblerInit(BPFAssembler* assembler, String* infile, String* outfile);
BPFAssembler* NewBPFAssembler(String* infile, String* outfile);
void BPFAssemblerDestruct(BPFAssembler* assembler);
void BPFAssemblerDelete(BPFAssembler* assembler);
void AssembleBPFInstruction(Assembler* assembler, String* word);

#endif /* bpf_assembler_h */
