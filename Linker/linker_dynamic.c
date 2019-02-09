//
//  linker_dynamic.c
//  p_code_linker
//
//  Created by David Allison on 7/6/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "linker_dynamic.h"
#include <stdlib.h>
#include "linker_file.h"
#include "linker_symbols.h"
#include "linker_reloc.h"
#include "linker.h"
#include "elf_writer.h"


void DynamicSectionInit(DynamicSection* s, Linker* linker) {
  switch (linker->elf_machine_type) {
    case ELF_MACHINE_TYPE_PCODE:
      s->global_offset_table.num_reserved_entries = 1;
      s->global_offset_table.entry_size = 8;
      s->procedure_linkage_table.num_reserved_entries = 2;
      s->procedure_linkage_table.entry_size = 36;
      break;
      // TODO: RISC-V
  }
  s->global_offset_table.address = 0;
  s->procedure_linkage_table.address = 0;
  VectorInit(&s->global_offset_table.data_entries);
  VectorInit(&s->global_offset_table.function_entries);
  VectorInit(&s->procedure_linkage_table.trampolines);
  VectorInit(&s->relocations);
}

DynamicSection* NewDynamicSection(Linker* linker) {
  DynamicSection* s = malloc(sizeof(DynamicSection));
  DynamicSectionInit(s, linker);
  return s;
}

void DynamicSectionDestruct(DynamicSection* s) {
  // TODO: vector contents.
  VectorDestruct(&s->global_offset_table.data_entries);
  VectorDestruct(&s->global_offset_table.function_entries);
  VectorDestruct(&s->procedure_linkage_table.trampolines);
  VectorDestruct(&s->relocations);
}

void DynamicSectionDelete(DynamicSection* s) {
  DynamicSectionDestruct(s);
  free(s);
}

void DynamicSectionInventSymbols(struct Linker* linker, DynamicSection* s) {
  s->global_offset_table_symbol = LinkerInventSymbol(linker, "_GLOBAL_OFFSET_TABLE_", 8);
  s->dynamic_symbol = LinkerInventSymbol(linker, "_DYNAMIC_", 8);
}

DynamicLibrary* NewDynamicLibrary(LinkerFile* file) {
  DynamicLibrary* lib = malloc(sizeof(DynamicLibrary));
  lib->file = file;
  return lib;
}

void DynamicLibraryDestruct(DynamicLibrary* lib) {
  LinkerFileDelete(lib->file);
}

void DynamicLibraryDelete(DynamicLibrary* lib) {
  DynamicLibraryDestruct(lib);
  free(lib);
}

int GetDataGOTOffset(DynamicSection* s, LinkerSymbol* symbol) {
  if (symbol->got_offset == -1) {
    // Symbol is not in Global Offset Table, add it.
    VectorAppend(&s->global_offset_table.data_entries, symbol);
    symbol->got_offset = (int)s->global_offset_table.data_entries.length - 1 +
        s->global_offset_table.num_reserved_entries;
  }
  return symbol->got_offset;
}

// NOTE: the function's GOT offset is relative to the end of
// the data GOT entries.  We don't know how many data entries
// there are yet, so the actual offset will need be calculated later.
int GetFunctionGOTOffset(DynamicSection* s, LinkerSymbol* symbol) {
  if (symbol->got_offset == -1) {
    // Symbol is not in Global Offset Table, add it.
    VectorAppend(&s->global_offset_table.function_entries, symbol);
    symbol->got_offset = (int)s->global_offset_table.function_entries.length - 1;
  }
  return symbol->got_offset;
}

int GetPLTOffset(DynamicSection* s, LinkerSymbol* symbol) {  
  if (symbol->plt_offset == -1) {
    VectorAppend(&s->procedure_linkage_table.trampolines, symbol);
    symbol->plt_offset = (int)s->procedure_linkage_table.trampolines.length - 1 +
      s->procedure_linkage_table.num_reserved_entries;
  }
  return symbol->plt_offset;
}



static void AddGOTEntry(Linker* linker, LinkerSymbol* symbol,
                        ELFWriterSectionContents* contents,
                        bool is_function) {
  DynamicSection* dynamic = linker->dynamic_section;
  int32_t reloc_type = 0;
  int64_t offset = contents->data.buffered.length;
  
  switch (linker->elf_machine_type) {
    case ELF_MACHINE_TYPE_PCODE:
      if (is_function) {
        reloc_type = R_PCODE_GOT_FUNC;
      } else {
        reloc_type = R_PCODE_GOT_DATA;
      }
      BufferAppendLongLE(&contents->data.buffered, 0);
      break;
      // TODO: RISC-V
  }
  LinkerRelocation* reloc = NewLinkerRelocation(symbol->name.value,
                              offset, reloc_type);
  VectorAppend(&dynamic->relocations, reloc);
}

void BuildGlobalOffsetTable(struct Linker* linker, ELFWriterSectionContents* contents) {
  DynamicSection* dynamic = linker->dynamic_section;
  
  // Add space for reserved entries.
  BufferAddSpace(&contents->data.buffered,
                dynamic->global_offset_table.num_reserved_entries *
                  dynamic->global_offset_table.entry_size);
  
  // Add data entries.
  Vector* data_entries = &dynamic->global_offset_table.data_entries;
  for (size_t i = 0; i < data_entries->length; i++) {
    AddGOTEntry(linker, data_entries->value[i], contents, false);
  }
  
  // Add function entries.
  // First calculate the delta to add to the function got entries.  They
  // are initialially set relative to the end of the data entries.
  size_t first_func_offset = contents->data.buffered.length /
        dynamic->global_offset_table.entry_size;
  
  Vector* func_entries = &dynamic->global_offset_table.function_entries;
  for (size_t i = 0; i < func_entries->length; i++) {
    LinkerSymbol* symbol = func_entries->value[i];
    
    // Fix function GOT offset now that we know all the data entries.
    symbol->got_offset += first_func_offset;
    AddGOTEntry(linker, symbol, contents, true);
  }
}


void AddPLTEntry(Linker* linker, LinkerSymbol* symbol, ELFWriterSectionContents* contents) {
  switch (linker->elf_machine_type) {
    case ELF_MACHINE_TYPE_PCODE:
      // For P-Code a PLT entry is as follows:
      // cjmp got_entry_address
      // movc tmp, #got_index
      // jmp _dl_runtime_resolve
      //
      // Initially, the GOT entry for a function contains the
      // address of the movc instruction.  The first call to
      // this PLT trampoline will jump to the movc and then on
      // to _dl_runtime_resolve that will use the value in the
      // tmp register to determine the offset into the GOT
      // and the value on the stack (return address from the
      // call instruction) to determine the symbol.  It will
      // look up the symbol and overwrite the GOT entry with
      // the symbol value, then jump to it.
      // 
      break;
  }
  
}


static void ProcessPossibleDynamicRelocation(struct Linker* linker,
                                             struct LinkerFile* file,
                                             LinkerRelocation* reloc) {
  LinkerSymbol* symbol = LinkerFileFindSymbol(file,
                                              reloc->symbol_name.value);
  if (symbol != NULL) {
    switch (linker->elf_machine_type) {
      case ELF_MACHINE_TYPE_PCODE:
        switch (reloc->type) {
          case R_PCODE_GOT_ENTRY:
            reloc->got_offset =
            GetDataGOTOffset(linker->dynamic_section,
                             symbol);
            break;
            
          case R_PCODE_CALL_PLT:
            // Add GOT entry for function address.
            reloc->got_offset =
            GetFunctionGOTOffset(linker->dynamic_section,
                                 symbol);
            
            // Add PLT entry for call to GOT.
            reloc->plt_offset =
            GetPLTOffset(linker->dynamic_section, symbol);
            break;
        }
        break;
      case ELF_MACHINE_TYPE_RISC_V:
        switch (reloc->type) {
          case R_RISCV_GOT_HI20:
            reloc->got_offset =
            GetDataGOTOffset(linker->dynamic_section, symbol);
            break;
            
          case R_RISCV_CALL_PLT:
            reloc->plt_offset =
            GetPLTOffset(linker->dynamic_section, symbol);
            break;
        }
        break;
    }
  }
}

void GatherDynamicRelocations(struct Linker* linker) {
  for (size_t file_index = 0; file_index < linker->files.length; file_index++) {
    LinkerFile* file = linker->files.value[file_index];
    for (size_t reloc_index = 0; reloc_index < file->relocations.length; reloc_index++) {
      ProcessPossibleDynamicRelocation(linker, file, file->relocations.value[reloc_index]);
    }
  }
}

#if 0
Section* DynamicInventSection(const char* name) {
  return NULL;
}
#endif

