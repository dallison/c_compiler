.text

.global setjmp
.type setjmp, @function
setjmp:
	sw ra, 0(a0)
	sw sp, 4(a0)
	sw s0, 8(a0)
	sw s1, 12(a0)
	sw s2, 16(a0)
	sw s3, 20(a0)
	sw s4, 24(a0)
	sw s5, 28(a0)
	sw s6, 32(a0)
	sw s7, 36(a0)
	sw s8, 40(a0)
	sw s9, 44(a0)
	sw s10, 48(a0)
	sw s11, 52(a0)
	fsd fs0, 112(a0)
	fsd fs1, 120(a0)
	fsd fs2, 128(a0)
	fsd fs3, 136(a0)
	fsd fs4, 144(a0)
	fsd fs5, 152(a0)
	fsd fs6, 160(a0)
	fsd fs7, 168(a0)
	fsd fs8, 176(a0)
	fsd fs9, 184(a0)
	fsd fs10, 192(a0)
	fsd fs11, 200(a0)
	li a0, 0
	ret
