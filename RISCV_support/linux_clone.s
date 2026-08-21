.text

.global __davecc_linux_clone
.type __davecc_linux_clone, @function

__davecc_linux_clone:
	addi sp, sp, -24
	sd s0, 0(sp)
	sd s1, 8(sp)
	sd ra, 16(sp)
	addi t0, a0, -16
	sd a2, 0(t0)
	sd a3, 8(t0)
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
	ld s0, 0(sp)
	ld s1, 8(sp)
	ld ra, 16(sp)
	addi sp, sp, 24
	ret

.Lclone_child:
	ld a0, 0(sp)
	ld s1, 8(sp)
	jalr ra, s1, 0
	li a7, 93
	ecall
