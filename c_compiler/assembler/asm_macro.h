//
// GNU as macro and conditional-assembly support shared by every target
// assembler.  Syntax follows the GNU assembler: .macro/.endm/.exitm/.purgem,
// argument substitution (\name, \(), \@, \+), .altmacro, .rept/.irp/.irpc,
// and the .if family used inside macros.
//

#ifndef asm_macro_h
#define asm_macro_h

#include "assembler.h"

void AssemblerMacroInit(Assembler* assembler);
// Drops macro definitions and expansion state so the next assembly pass
// re-reads them from the source.  Invocation counters restart too.
void AssemblerMacroReset(Assembler* assembler);
void AssemblerMacroShutdown(Assembler* assembler);

bool AssemblerCondActive(const Assembler* assembler);
bool AssemblerIsConditionalDirective(const char* spelling);

// Reads the next assembly line, popping a finished macro expansion first.
void AssemblerReadAssemblyLine(Assembler* assembler);
// Skips the rest of a statement while a conditional is false.
void AssemblerSkipInactiveStatement(Assembler* assembler);

// Returns true when `spelling` names a macro and the call was consumed.
bool AssemblerMacroTryInvoke(Assembler* assembler, const char* spelling);

void HandleDirective_macro(Assembler* assembler);
void HandleDirective_endm(Assembler* assembler);
void HandleDirective_exitm(Assembler* assembler);
void HandleDirective_purgem(Assembler* assembler);
void HandleDirective_rept(Assembler* assembler);
void HandleDirective_irp(Assembler* assembler);
void HandleDirective_irpc(Assembler* assembler);
void HandleDirective_endr(Assembler* assembler);
void HandleDirective_altmacro(Assembler* assembler);
void HandleDirective_noaltmacro(Assembler* assembler);
void HandleDirective_if(Assembler* assembler);
void HandleDirective_ifdef(Assembler* assembler);
void HandleDirective_ifndef(Assembler* assembler);
void HandleDirective_ifnotdef(Assembler* assembler);
void HandleDirective_include(Assembler* assembler);
void HandleDirective_incbin(Assembler* assembler);
void HandleDirective_ifb(Assembler* assembler);
void HandleDirective_ifnb(Assembler* assembler);
void HandleDirective_ifc(Assembler* assembler);
void HandleDirective_ifnc(Assembler* assembler);
void HandleDirective_ifeq(Assembler* assembler);
void HandleDirective_ifne(Assembler* assembler);
void HandleDirective_ifgt(Assembler* assembler);
void HandleDirective_ifge(Assembler* assembler);
void HandleDirective_iflt(Assembler* assembler);
void HandleDirective_ifle(Assembler* assembler);
void HandleDirective_ifeqs(Assembler* assembler);
void HandleDirective_ifnes(Assembler* assembler);
void HandleDirective_else(Assembler* assembler);
void HandleDirective_elseif(Assembler* assembler);
void HandleDirective_endif(Assembler* assembler);

#endif /* asm_macro_h */
