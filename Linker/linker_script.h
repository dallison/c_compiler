//
//  linker_script.h
//  c_compiler
//
//  GNU ld / LLVM lld linker-script support.  davecc accepts the common
//  MEMORY / SECTIONS / PHDRS / ENTRY / INCLUDE language used by
//  -T/--script on those linkers, plus the assignments, globs, and
//  region helpers typically found in those scripts.
//

#ifndef linker_script_h
#define linker_script_h

#include "linker_config.h"

// Parse a GNU ld / LLVM lld linker script from a file or memory.
// `filename` is used only for diagnostics.  Returns false on a fatal
// parse error; `config` is initialized in either case and must be
// destroyed by the caller.  Target-specific defaults (page alignment,
// file-offset matching) are applied using `elf_machine_type`.
bool LinkerScriptParseFile(const char* filename, int elf_machine_type,
                           LinkerConfig* config);
bool LinkerScriptParseString(const char* filename, const char* text,
                             int elf_machine_type, LinkerConfig* config);

// Load the built-in script for this ELF machine, ELF class, and layout type
// ("program", "rom", or "introm").  The class distinguishes targets such as
// RV32 and RV64 that share one ELF machine ID.
bool LinkerScriptLoadBuiltin(int elf_machine_type, bool is_64_bit,
                             const char* layout_type, LinkerConfig* config);

#endif /* linker_script_h */
