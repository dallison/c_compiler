.text

.global longjmp
.type longjmp, @function
longjmp:
	mv t0, a0
	mv t1, a1
	bnez t1, .Llongjmp_value_ready
	li t1, 1
.Llongjmp_value_ready:
	ld ra, 0(t0)
	ld sp, 8(t0)
	ld s0, 16(t0)
	ld s1, 24(t0)
	ld s2, 32(t0)
	ld s3, 40(t0)
	ld s4, 48(t0)
	ld s5, 56(t0)
	ld s6, 64(t0)
	ld s7, 72(t0)
	ld s8, 80(t0)
	ld s9, 88(t0)
	ld s10, 96(t0)
	ld s11, 104(t0)
	fld fs0, 112(t0)
	fld fs1, 120(t0)
	fld fs2, 128(t0)
	fld fs3, 136(t0)
	fld fs4, 144(t0)
	fld fs5, 152(t0)
	fld fs6, 160(t0)
	fld fs7, 168(t0)
	fld fs8, 176(t0)
	fld fs9, 184(t0)
	fld fs10, 192(t0)
	fld fs11, 200(t0)
	mv a0, t1
	jr ra
