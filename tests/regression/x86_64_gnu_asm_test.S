// Hand-written GNU AT&T assembly, called from x86_64_gnu_asm_test.c.

#define SYM(name) #name
#define IMM(x) $x

#define FUNC(name) \
	.global SYM(name); \
	.type SYM(name), @function; \
SYM(name):

	.text

// rdi + 1, computed with 64-bit inc and a 32-bit dec (which zero-extends).
FUNC(asm_incdec)
	movq %rdi, %rax
	incq %rax; incq %rax
	decl %eax
	ret

// Sized immediate stores over a 16-byte buffer of 0xff.
FUNC(asm_sized_stores)
	movb $0x11, (%rdi)
	movw $0x2233, 2(%rdi)
	movl $0x44556677, 4(%rdi)
	ret

// Sum of the .quad/.long/.word/.byte entries of `table`.
FUNC(asm_data_sum)
	movq table(%rip), %rax
	movl table+8(%rip), %ecx
	addq %rcx, %rax
	movzwl table+12(%rip), %ecx
	addq %rcx, %rax
	movzbl table+14(%rip), %ecx
	addq %rcx, %rax
	movzbl table+15(%rip), %ecx
	addq %rcx, %rax
	ret

// 10 * rdi, counted down with local labels.
FUNC(asm_local_labels)
	xorl %eax, %eax
1:	testq %rdi, %rdi
	jz 2f
	addq $10, %rax
	decq %rdi
	jmp 1b
2:	ret

// Element 3 of a long array, through an expression displacement.
FUNC(asm_disp_expr)
	movq 8 + 2 * 8(%rdi), %rax
	ret

// memcpy(rdi, rsi, rdx) with rep movsb.
FUNC(asm_rep_movsb)
	movq %rdx, %rcx
	rep movsb
	ret

// Atomic fetch-add: returns the old *rdi.
FUNC(asm_lock_xadd)
	movq %rsi, %rax
	lock xaddq %rax, (%rdi)
	ret

FUNC(asm_max)
	movq %rdi, %rax
	cmpq %rsi, %rax
	cmovlq %rsi, %rax
	ret

FUNC(asm_less)
	xorl %eax, %eax
	cmpq %rsi, %rdi
	setl %al
	ret

// Bits 8-15 of rdi, through %ah.
FUNC(asm_high_byte)
	movl %edi, %eax
	movb %ah, %al
	movzbl %al, %eax
	ret

// 0x2a + 16 from macro and expression immediates.
FUNC(asm_macro_imm)
	movl IMM(0x2a), %eax
	addl $(1 << 4), %eax
	ret

// Alignment padding inside a function is executed, so it must be NOPs.
FUNC(asm_padded)
	movl $1, %eax
	.p2align 4
	addl $2, %eax
	.balign 8
	ret

// Byte and word ALU forms on memory and on %ah, plus setcc to memory.
// Over {1, 2, 3, 4, 5, 6, 7, 8} this leaves
// {0x13, 2, 2, 3, 0x85, 0x26, 1, 8}.
FUNC(asm_byte_alu)
	movl $0x1234, %eax
	addb %ah, (%rdi)
	subw $0x101, 2(%rdi)
	orb $0x80, 4(%rdi)
	xorb %al, %ah
	movb %ah, 5(%rdi)
	cmpb $0x26, 5(%rdi)
	sete 6(%rdi)
	ret

// Byte arithmetic merges into the wider register and carries through adc.
FUNC(asm_byte_carry)
	movl $0x1ff, %eax
	addb $1, %al
	adcb $0, %ah
	ret

// strlen with repne scasb.
FUNC(asm_strlen)
	movq $-1, %rcx
	xorl %eax, %eax
	repne scasb
	movq $-2, %rax
	subq %rcx, %rax
	ret

// Fill rdx quadwords at rdi with rsi, using rep stosq.
FUNC(asm_fill_quads)
	movq %rsi, %rax
	movq %rdx, %rcx
	rep stosq
	ret

// memcmp for n > 0 with repe cmpsb.
FUNC(asm_memcmp)
	movq %rdx, %rcx
	xorl %eax, %eax
	repe cmpsb
	je 1f
	movzbl -1(%rdi), %eax
	movzbl -1(%rsi), %ecx
	subl %ecx, %eax
1:	ret

// memmove(rdi, rsi, rdx) for rdi > rsi: copy backwards with the direction
// flag set.
FUNC(asm_move_up)
	leaq -1(%rdi,%rdx), %rdi
	leaq -1(%rsi,%rdx), %rsi
	movq %rdx, %rcx
	std
	rep movsb
	cld
	ret

// Sum of rsi unsigned shorts at rdi, loaded with lodsw.
FUNC(asm_sum_words)
	movq %rsi, %rcx
	movq %rdi, %rsi
	xorl %edx, %edx
	xorl %eax, %eax
1:	lodsw
	addq %rax, %rdx
	decq %rcx
	jnz 1b
	movq %rdx, %rax
	ret

// An untaken 32-bit cmov still zero-extends its destination.
FUNC(asm_cmov32_untaken)
	movq %rdi, %rax
	cmpl %eax, %eax
	cmovnel %esi, %eax
	ret

// max(rdi, *rsi) with a memory-source cmov.
FUNC(asm_max_mem)
	movq %rdi, %rax
	cmpq (%rsi), %rax
	cmovlq (%rsi), %rax
	ret

	.data
	.balign 8
table:	.quad 1000000
	.long 2000
	.word 30
	.byte 4
	.zero 1
	.balign 8
