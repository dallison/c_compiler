#include "vars.s"

.section ".text.__davecc_6502_hardware_stack_pointer", "ax", @progbits

.global __davecc_6502_hardware_stack_pointer
.type __davecc_6502_hardware_stack_pointer, @function
__davecc_6502_hardware_stack_pointer:
	stx __i0
	sty __i0+1
	tsx
	txa
#ifdef __65c02__
	STA (__i0)
#else
	PHP
	STY __nmos_tmp
	LDY #0
	STA (__i0),Y
	LDY __nmos_tmp
	PLP
#endif
	rts
