.text

.global longjmp
.type longjmp, @function
longjmp:
	mv t0, a0
	mv t1, a1
	bnez t1, .Llongjmp_value_ready
	li t1, 1
.Llongjmp_value_ready:
	lw ra, 0(t0)
	lw sp, 4(t0)
	lw s0, 8(t0)
	lw s1, 12(t0)
	lw s2, 16(t0)
	lw s3, 20(t0)
	lw s4, 24(t0)
	lw s5, 28(t0)
	lw s6, 32(t0)
	lw s7, 36(t0)
	lw s8, 40(t0)
	lw s9, 44(t0)
	lw s10, 48(t0)
	lw s11, 52(t0)
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
