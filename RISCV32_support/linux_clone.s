.text

.global __davecc_linux_clone
.type __davecc_linux_clone, @function

__davecc_linux_clone:
	addi sp, sp, -16
	sw s0, 0(sp)
	sw s1, 4(sp)
	sw ra, 8(sp)
	addi t0, a0, -16
	sw a2, 0(t0)
	sw a3, 4(t0)
	mv s0, a2
	mv s1, a3
	mv a0, a1
	mv a1, t0
	mv a2, a5
	mv a3, a4
	mv a4, a5
	li a7, 220
	ecall
	beq a0, zero, .Lclone_child
	lw s0, 0(sp)
	lw s1, 4(sp)
	lw ra, 8(sp)
	addi sp, sp, 16
	ret

.Lclone_child:
	lw a0, 0(sp)
	lw s1, 4(sp)
	jalr ra, s1, 0
	li a7, 93
	ecall
