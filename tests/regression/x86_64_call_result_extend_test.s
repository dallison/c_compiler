// Stands in for code from another compiler: the SysV ABI leaves the upper
// half of a 32-bit return value undefined, so these leave it dirty.
	.text
	.globl	dirty_int
dirty_int:
	movq	$12345678fffffffb, %rax
	ret
	.globl	dirty_uint
dirty_uint:
	movq	$fedcba9800000007, %rax
	ret
