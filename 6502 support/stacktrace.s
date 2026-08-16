#include "vars.s"

.text

.global __davecc_6502_hardware_stack_pointer
.type __davecc_6502_hardware_stack_pointer, @function
__davecc_6502_hardware_stack_pointer:
	stx __i0
	sty __i0+1
	tsx
	txa
	sta (__i0)
	rts
