	.file   "libc/posix_fs.c"
	.file 1 "libc/include/errno.h"
	.file 2 "libc/include/syscall.h"
	.file 3 "libc/include/davecc_guest_syscalls.h"
	.file 4 "libc/include/fcntl.h"
	.file 5 "libc/include/unistd.h"
	.file 6 "libc/include/limits.h"
	.file 7 "libc/include/stdlib.h"
	.file 8 "libc/include/string.h"
	.file 9 "libc/include/sys/stat.h"
	.file 10 "libc/include/stdint.h"
	.file 11 "libc/include/time.h"
	.file 12 "libc/posix_fs.h"
	.file 13 "libc/posix_fs.c"
	.text
	.set __b0 0x0
	.set __b1 0x1
	.set __b2 0x2
	.set __b3 0x3
	.set __b4 0x4
	.set __b5 0x5
	.set __b6 0x6
	.set __b7 0x7
	.set __i0 0x8
	.set __i1 0xa
	.set __i2 0xc
	.set __i3 0xe
	.set __i4 0x10
	.set __i5 0x12
	.set __i6 0x14
	.set __i7 0x16
	.set __i8 0x18
	.set __i9 0x1a
	.set __i10 0x1c
	.set __i11 0x1e
	.set __i12 0x20
	.set __i13 0x22
	.set __i14 0x24
	.set __i15 0x26
	.set __l0 0x28
	.set __l1 0x2c
	.set __l2 0x30
	.set __l3 0x34
	.set __l4 0x38
	.set __l5 0x3c
	.set __l6 0x40
	.set __l7 0x44
	.set __x0 0x48
	.set __x1 0x50
	.set __x2 0x58
	.set __x3 0x60
	.set __f0 0x68
	.set __f1 0x6c
	.set __f2 0x70
	.set __f3 0x74
	.set __sp 0x78
	.set __fp 0x7a
	.set __result 0x7c
	.set __t0 0x7e
	.set __t1 0x7f
	.set __t2 0x80
	.set __t3 0x81
	.set __mem_src 0x82
	.set __mem_dest 0x84
	.set __mem_size 0x86

.PCbegin:
	.section ".text.FsCall", "ax", @progbits
	.local  FsCall
	.type FsCall, @function

FsCall:
	lda          #7
	jsr          __enter_res
	.byte        0x23,0x00,0x00		// Save mask i:3 b:1 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i0			// operation
	ldx          #6
	jsr          __arg_value2_i4			// third
	ldx          #4
	jsr          __arg_value2_i5			// second
	ldx          #2
	jsr          __arg_value2_i6			// first
	.loc 14 28 20
	lda         __i0+1
	and          #128
	ora         __i0
	sta         __b2
	cmp          #1
	beq         .FsCall_label_94
	lda         __b2
	cmp          #6
	beq         .FsCall_label_117
	lda         __b2
	cmp          #8
	beq         .FsCall_label_134
	lda         __b2
	cmp          #9
	bne         .FsCall_label_244
	jmp         .FsCall_label_152
.FsCall_label_244:
	lda         __b2
	cmp          #13
	bne         .FsCall_label_238
	jmp         .FsCall_label_168
.FsCall_label_238:
	lda         __b2
	cmp          #18
	bne         .FsCall_label_240
	jmp         .FsCall_label_186
.FsCall_label_240:
	lda         __b2
	cmp          #19
	bne         .FsCall_label_242
	jmp         .FsCall_label_204
.FsCall_label_242:
	jmp         .FsCall_label_221
.FsCall_label_94:
	.loc 14 30 1
	jsr         __pushi4
	jsr         __pushi5
	jsr         __pushi6
	ldx          #31
	jsr         __pushxy0
	ldx         #__l0
	ldy          #0
	jsr         syscall
	jsr         __incsp8
	ldx          #8
	jsr          __load_result
	lda         #__l0
	jsr         __result4
.FsCall_label_114:
	ldy          #18
	jmp          __leave
.FsCall_label_117:
	.loc 14 32 1
	jsr         __pushi6
	ldx          #36
	jsr         __pushxy0
	ldx         #__l0
	ldy          #0
	jsr         syscall
	jsr         __incsp4
	ldx          #8
	jsr          __load_result
	lda         #__l0
	jsr         __result4
	bra         .FsCall_label_114
.FsCall_label_134:
	.loc 14 34 1
	jsr         __pushi5
	jsr         __pushi6
	ldx          #38
	jsr         __pushxy0
	ldx         #__l0
	ldy          #0
	jsr         syscall
	jsr         __incsp6
	ldx          #8
	jsr          __load_result
	lda         #__l0
	jsr         __result4
	bra         .FsCall_label_114
.FsCall_label_152:
	.loc 14 36 1
	jsr         __pushi6
	ldx          #39
	jsr         __pushxy0
	ldx         #__l0
	ldy          #0
	jsr         syscall
	jsr         __incsp4
	ldx          #8
	jsr          __load_result
	lda         #__l0
	jsr         __result4
	bra         .FsCall_label_114
.FsCall_label_168:
	.loc 14 38 1
	jsr         __pushi4
	jsr         __pushi5
	jsr         __pushi6
	ldx          #43
	jsr         __pushxy0
	ldx         #__l0
	ldy          #0
	jsr         syscall
	jsr         __incsp8
	ldx          #8
	jsr          __load_result
	lda         #__l0
	jsr         __result4
	jmp         .FsCall_label_114
.FsCall_label_186:
	.loc 14 40 1
	jsr         __pushi4
	jsr         __pushi5
	jsr         __pushi6
	ldx          #48
	jsr         __pushxy0
	ldx         #__l0
	ldy          #0
	jsr         syscall
	jsr         __incsp8
	ldx          #8
	jsr          __load_result
	lda         #__l0
	jsr         __result4
	jmp         .FsCall_label_114
.FsCall_label_204:
	.loc 14 42 1
	jsr         __pushi5
	jsr         __pushi6
	ldx          #61
	jsr         __pushxy0
	ldx         #__l0
	ldy          #0
	jsr         syscall
	jsr         __incsp6
	ldx          #8
	jsr          __load_result
	lda         #__l0
	jsr         __result4
	jmp         .FsCall_label_114
.FsCall_label_221:
	.loc 14 44 1
	ldx          #3
.FsCall_label_228:
	lda         .lit.33, X
	sta         __l0, X
	dex         
	bpl         .FsCall_label_228
	ldx          #8
	jsr          __load_result
	lda         #__l0
	jsr         __result4
	jmp         .FsCall_label_114
.func_end_FsCall:
	.size FsCall, .func_end_FsCall-FsCall

	.section ".text.Failed", "ax", @progbits
	.local  Failed
	.type Failed, @function

Failed:
	lda          #5
	jsr          __enter_leaf_res_nomask
	ldx          #0
	jsr          __arg_value4_l0			// result
	.loc 14 50 18
	lda         __l0
	cmp          #0
	lda         __l0+1
	sbc          #0
	lda         __l0+2
	sbc          #0
	lda         __l0+3
	sbc          #0
	bvc         .Failed_label_13
	eor          #128
.Failed_label_13:
	bmi         .Failed_label_41
	.loc 14 50 18
	stz         __i0
	stz         __i0+1
	lda         #__i0
	jsr         __result2
.Failed_label_38:
	ldy          #12
	jmp          __leave_leaf_nomask
.Failed_label_41:
	.loc 14 51 1
	sec         
	ldy          #4
	ldx          #0
.Failed_label_49:
	lda          #0
	sbc         __l0, X
	sta         __l1, X
	inx         
	dey         
	bne         .Failed_label_49
	lda         __l1
	sta         __i0
	lda         __l1+3
	and          #128
	ora         __l1+1
	sta         __i0+1
	lda          #214
	sta         __i1
	lda          #3
	sta         __i1+1
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
	.loc 14 52 1
	lda          #1
	sta         __i0
	dec          A
	stz         __i0+1
	lda         #__i0
	jsr         __result2
	bra         .Failed_label_38
.func_end_Failed:
	.size Failed, .func_end_Failed-Failed

	.section ".text.CopyStatus", "ax", @progbits
	.local  CopyStatus
	.type CopyStatus, @function

CopyStatus:
	ldx          #7
	jsr          __enter
	.byte        0x00,0x40,0x00		// Save mask i:0 b:0 l:0 x:2 f:0 
	ldx          #0
	jsr          __arg_value2_i0			// output
	ldx          #2
	jsr          __arg_value2_i1			// input
	.loc 14 56 1
	lda         __i0
	sta         __i2
	lda         __i0+1
	sta         __i2+1
	lda          #128
	sta         __i3
	stz         __i3+1
	sta         __mem_size
	lda         __i3+1
	sta         __mem_size+1
	stz         __mem_src
	lda         __i2
	sta         __mem_dest
	lda         __i2+1
	sta         __mem_dest+1
	jsr         __builtin_memset
	.loc 14 57 1
	lda         __i1
	sta         __i3
	lda         __i1+1
	sta         __i3+1
	ldx          #__i3
	ldy          #__x0
	jsr          __load_indirect8
	ldx          #__x0
	ldy          #__i2
	jsr          __store_indirect8
	.loc 14 58 1
	ldy          #15
	ldx          #7
.CopyStatus_label_98:
	lda         (__i3), Y
	sta         __x0, X
	dey         
	dex         
	bpl         .CopyStatus_label_98
	ldy          #15
	ldx          #7
.CopyStatus_label_111:
	lda         __x0, X
	sta         (__i2), Y
	dey         
	dex         
	bpl         .CopyStatus_label_111
	.loc 14 59 1
	ldy          #15
	ldx          #7
.CopyStatus_label_124:
	lda         (__i3), Y
	sta         __x0, X
	dey         
	dex         
	bpl         .CopyStatus_label_124
	ldy          #127
	ldx          #7
.CopyStatus_label_137:
	lda         __x0, X
	sta         (__i2), Y
	dey         
	dex         
	bpl         .CopyStatus_label_137
	.loc 14 60 1
	ldy          #59
	ldx          #3
.CopyStatus_label_152:
	lda         (__i3), Y
	sta         __l0, X
	dey         
	dex         
	bpl         .CopyStatus_label_152
	ldy          #19
	ldx          #3
.CopyStatus_label_165:
	lda         __l0, X
	sta         (__i2), Y
	dey         
	dex         
	bpl         .CopyStatus_label_165
	.loc 14 61 1
	ldy          #31
	ldx          #7
.CopyStatus_label_179:
	lda         (__i3), Y
	sta         __x0, X
	dey         
	dex         
	bpl         .CopyStatus_label_179
	lda         __x0
	sta         __l0
	lda         __x0+1
	sta         __l0+1
	lda         __x0+2
	sta         __l0+2
	lda         __x0+3
	sta         __l0+3
	ldy          #23
	ldx          #3
.CopyStatus_label_204:
	lda         __l0, X
	sta         (__i2), Y
	dey         
	dex         
	bpl         .CopyStatus_label_204
	.loc 14 62 1
	ldy          #23
	ldx          #7
.CopyStatus_label_217:
	lda         (__i3), Y
	sta         __x0, X
	dey         
	dex         
	bpl         .CopyStatus_label_217
	ldy          #55
	ldx          #7
.CopyStatus_label_230:
	lda         __x0, X
	sta         (__i2), Y
	dey         
	dex         
	bpl         .CopyStatus_label_230
	.loc 14 63 1
	ldy          #63
	ldx          #7
.CopyStatus_label_243:
	lda         .lit.58, X
	sta         (__i2), Y
	dey         
	dex         
	bpl         .CopyStatus_label_243
	.loc 14 64 1
	ldy          #23
	ldx          #7
.CopyStatus_label_256:
	lda         (__i3), Y
	sta         __x0, X
	dey         
	dex         
	bpl         .CopyStatus_label_256
	clc         
	ldy          #8
	ldx          #0
.CopyStatus_label_270:
	lda         __x0, X
	adc         .lit.61, X
	sta         __x1, X
	inx         
	dey         
	bne         .CopyStatus_label_270
	lda         __x1+1
	sta         __x0
	lda         __x1+2
	sta         __x0+1
	lda         __x1+3
	sta         __x0+2
	lda         __x1+4
	sta         __x0+3
	lda         __x1+5
	sta         __x0+4
	lda         __x1+6
	sta         __x0+5
	lda         __x1+7
	sta         __x0+6
	lda          #0
	stz         __x0+7
	lsr         __x0+7
	ror         __x0+6
	ror         __x0+5
	ror         __x0+4
	ror         __x0+3
	ror         __x0+2
	ror         __x0+1
	ror         __x0
	ldy          #71
	ldx          #7
.CopyStatus_label_314:
	lda         __x0, X
	sta         (__i2), Y
	dey         
	dex         
	bpl         .CopyStatus_label_314
	.loc 14 65 1
	ldy          #39
	ldx          #7
.CopyStatus_label_328:
	lda         (__i3), Y
	sta         __x0, X
	dey         
	dex         
	bpl         .CopyStatus_label_328
	ldx          #7
.CopyStatus_label_340:
	lda         .lit.65, X
	sta         __x1, X
	dex         
	bpl         .CopyStatus_label_340
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __sdiv8
	ldy          #79
	ldx          #7
.CopyStatus_label_361:
	lda         __x2, X
	sta         (__i2), Y
	dey         
	dex         
	bpl         .CopyStatus_label_361
	.loc 14 66 1
	ldy          #39
	ldx          #7
.CopyStatus_label_374:
	lda         (__i3), Y
	sta         __x0, X
	dey         
	dex         
	bpl         .CopyStatus_label_374
	ldx          #7
.CopyStatus_label_385:
	lda         .lit.65, X
	sta         __x1, X
	dex         
	bpl         .CopyStatus_label_385
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __smod8
	ldy          #87
	ldx          #7
.CopyStatus_label_406:
	lda         __x2, X
	sta         (__i2), Y
	dey         
	dex         
	bpl         .CopyStatus_label_406
	.loc 14 67 1
	ldy          #47
	ldx          #7
.CopyStatus_label_420:
	lda         (__i3), Y
	sta         __x0, X
	dey         
	dex         
	bpl         .CopyStatus_label_420
	ldx          #7
.CopyStatus_label_432:
	lda         .lit.65, X
	sta         __x1, X
	dex         
	bpl         .CopyStatus_label_432
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __sdiv8
	ldy          #95
	ldx          #7
.CopyStatus_label_452:
	lda         __x2, X
	sta         (__i2), Y
	dey         
	dex         
	bpl         .CopyStatus_label_452
	.loc 14 68 1
	ldy          #47
	ldx          #7
.CopyStatus_label_465:
	lda         (__i3), Y
	sta         __x0, X
	dey         
	dex         
	bpl         .CopyStatus_label_465
	ldx          #7
.CopyStatus_label_476:
	lda         .lit.65, X
	sta         __x1, X
	dex         
	bpl         .CopyStatus_label_476
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __smod8
	ldy          #103
	ldx          #7
.CopyStatus_label_496:
	lda         __x2, X
	sta         (__i2), Y
	dey         
	dex         
	bpl         .CopyStatus_label_496
	.loc 14 70 1
	ldy          #55
	ldx          #7
.CopyStatus_label_509:
	lda         (__i3), Y
	sta         __x0, X
	dey         
	dex         
	bpl         .CopyStatus_label_509
	ldx          #7
.CopyStatus_label_521:
	lda         .lit.65, X
	sta         __x1, X
	dex         
	bpl         .CopyStatus_label_521
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __sdiv8
	ldy          #111
	ldx          #7
.CopyStatus_label_541:
	lda         __x2, X
	sta         (__i2), Y
	dey         
	dex         
	bpl         .CopyStatus_label_541
	.loc 14 71 1
	ldy          #55
	ldx          #7
.CopyStatus_label_554:
	lda         (__i3), Y
	sta         __x0, X
	dey         
	dex         
	bpl         .CopyStatus_label_554
	ldx          #7
.CopyStatus_label_565:
	lda         .lit.65, X
	sta         __x1, X
	dex         
	bpl         .CopyStatus_label_565
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __smod8
	ldy          #119
	ldx          #7
.CopyStatus_label_585:
	lda         __x2, X
	sta         (__i2), Y
	dey         
	dex         
	bpl         .CopyStatus_label_585
	ldy          #10
	jmp          __leave_void
.func_end_CopyStatus:
	.size CopyStatus, .func_end_CopyStatus-CopyStatus

	.section ".text.Status", "ax", @progbits
	.local  Status
	.type Status, @function

Status:
	lda          #72
	jsr          __enter_res
	.byte        0x05,0x04,0x00		// Save mask i:5 b:0 l:2 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i4			// path
	ldx          #2
	jsr          __arg_value2_i5			// output
	ldx          #4
	jsr          __arg_value2_i6			// follow
	.loc 14 76 49
	ldx          #1
	lda         __i4
	bne         .Status_label_33
	lda         __i4+1
	beq         .Status_label_34
.Status_label_33:
	dex         
.Status_label_34:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i7			// __invented__23
	lda         __b0
	sta         (__i7)
	lda         __i4
	bne         .Status_label_54
	lda         __i4+1
	beq         .Status_label_83
.Status_label_54:
	ldx          #1
	lda         __i5
	bne         .Status_label_64
	lda         __i5+1
	beq         .Status_label_65
.Status_label_64:
	dex         
.Status_label_65:
	txa         
	sta         (__i7)
.Status_label_83:
	lda         (__i7)
	beq         .Status_label_121
	.loc 14 77 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #7
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 14 78 1
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #73
	lda          #__i0
	jsr         __load_result_value2
.Status_label_118:
	ldy          #81
	jmp          __leave
.Status_label_121:
	.loc 14 81 73
	ldx          #68
	lda          #__i8
	jsr          __var_addr_push_i8
	jsr         __pushi6
	jsr         __pushi4
	ldx          #1
	jsr         __pushxy0
	ldx         #__l3
	ldy          #0
	jsr         FsCall
	ldx          #3
.Status_label_141:
	lda         __l3, X
	sta         __l2, X
	dex         
	bpl         .Status_label_141
	.loc 14 82 21
	jsr         __pushl3
	ldx         #__i0
	ldy          #0
	jsr         Failed
	lda         __i0
	ora         __i0+1
	beq         .Status_label_169
	.loc 14 82 21
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #73
	lda          #__i0
	jsr         __load_result_value2
	bra         .Status_label_118
.Status_label_169:
	.loc 14 83 1
	jsr         __pushi8
	jsr         __pushi5
	jsr         CopyStatus
	jsr         __incsp4
	.loc 14 84 1
	stz         __i0
	stz         __i0+1
	ldx          #73
	lda          #__i0
	jsr         __load_result_value2
	bra         .Status_label_118
.func_end_Status:
	.size Status, .func_end_Status-Status

	.section ".text.stat", "ax", @progbits
	.global stat
	.type stat, @function

stat:
	lda          #7
	jsr          __enter_res_nomask
	ldx          #2
	jsr          __arg_value2_i0			// output
	ldx          #0
	jsr          __arg_value2_i1			// path
	.loc 14 88 1
	ldx          #1
	jsr         __pushxy0
	jsr         __pushi0
	jsr         __pushi1
	ldx         #__i2
	ldy          #0
	jsr         Status
	ldx          #8
	lda          #__i2
	jsr         __load_result_value2
	ldy          #14
	jmp          __leave_nomask
.func_end_stat:
	.size stat, .func_end_stat-stat

	.section ".text.lstat", "ax", @progbits
	.global lstat
	.type lstat, @function

lstat:
	lda          #7
	jsr          __enter_res_nomask
	ldx          #2
	jsr          __arg_value2_i0			// output
	ldx          #0
	jsr          __arg_value2_i1			// path
	.loc 14 92 1
	ldx          #0
	jsr         __pushxy0
	jsr         __pushi0
	jsr         __pushi1
	ldx         #__i2
	ldy          #0
	jsr         Status
	ldx          #8
	lda          #__i2
	jsr         __load_result_value2
	ldy          #14
	jmp          __leave_nomask
.func_end_lstat:
	.size lstat, .func_end_lstat-lstat

	.section ".text.fstat", "ax", @progbits
	.global fstat
	.type fstat, @function

fstat:
	lda          #71
	jsr          __enter_res
	.byte        0x03,0x04,0x00		// Save mask i:3 b:0 l:2 x:0 f:0 
	ldx          #2
	jsr          __arg_value2_i4			// output
	ldx          #0
	jsr          __arg_value2_i5			// fd
	.loc 14 96 27
	lda         __i4
	bne         .fstat_label_63
	lda         __i4+1
	bne         .fstat_label_63
	.loc 14 97 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #7
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 14 98 1
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #72
	lda          #__i0
	jsr         __load_result_value2
.fstat_label_60:
	ldy          #78
	jmp          __leave
.fstat_label_63:
	.loc 14 102 62
	ldx          #67
	jsr          __var_addr_i6			// wire
	ldx          #0
	jsr         __pushxy0
	jsr         __pushi6
	jsr         __pushi5
	ldx          #19
	jsr         __pushxy0
	ldx         #__l3
	ldy          #0
	jsr         FsCall
	ldx          #3
.fstat_label_85:
	lda         __l3, X
	sta         __l2, X
	dex         
	bpl         .fstat_label_85
	.loc 14 103 21
	jsr         __pushl3
	ldx         #__i0
	ldy          #0
	jsr         Failed
	lda         __i0
	ora         __i0+1
	beq         .fstat_label_113
	.loc 14 103 21
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #72
	lda          #__i0
	jsr         __load_result_value2
	bra         .fstat_label_60
.fstat_label_113:
	.loc 14 104 1
	jsr         __pushi6
	jsr         __pushi4
	jsr         CopyStatus
	jsr         __incsp4
	.loc 14 105 1
	stz         __i0
	stz         __i0+1
	ldx          #72
	lda          #__i0
	jsr         __load_result_value2
	bra         .fstat_label_60
.func_end_fstat:
	.size fstat, .func_end_fstat-fstat

	.section ".text.getcwd", "ax", @progbits
	.global getcwd
	.type getcwd, @function

getcwd:
	lda          #10
	jsr          __enter_res
	.byte        0x03,0x04,0x00		// Save mask i:3 b:0 l:2 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i4			// buffer
	ldx          #2
	jsr          __arg_value2_i5			// size
	.loc 14 109 40
	ldx          #1
	lda         __i4
	bne         .getcwd_label_28
	lda         __i4+1
	beq         .getcwd_label_29
.getcwd_label_28:
	dex         
.getcwd_label_29:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i6			// __invented__34
	lda         __b0
	sta         (__i6)
	lda         __i4
	bne         .getcwd_label_49
	lda         __i4+1
	beq         .getcwd_label_76
.getcwd_label_49:
	ldx          #1
	lda         __i5
	ora         __i5+1
	beq         .getcwd_label_60
	dex         
.getcwd_label_60:
	txa         
	sta         (__i6)
.getcwd_label_76:
	lda         (__i6)
	beq         .getcwd_label_112
	.loc 14 110 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #7
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 14 111 1
	stz         __i0
	stz         __i0+1
	ldx          #11
	lda          #__i0
	jsr         __load_result_value2
.getcwd_label_109:
	ldy          #17
	jmp          __leave
.getcwd_label_112:
	.loc 14 113 74
	ldx          #0
	jsr         __pushxy0
	jsr         __pushi5
	jsr         __pushi4
	ldx          #8
	jsr         __pushxy0
	ldx         #__l3
	ldy          #0
	jsr         FsCall
	ldx          #3
.getcwd_label_130:
	lda         __l3, X
	sta         __l2, X
	dex         
	bpl         .getcwd_label_130
	.loc 14 114 1
	jsr         __pushl3
	ldx         #__i0
	ldy          #0
	jsr         Failed
	lda         __i0
	ora         __i0+1
	beq         .getcwd_label_160
	ldx          #6
	jsr          __var_addr_i0			// __invented__35
	ldy          #1
	lda          #0
.getcwd_label_155:
	sta         (__i0), Y
	dey         
	bpl         .getcwd_label_155
	bra         .getcwd_label_170
.getcwd_label_160:
	ldx          #6
	jsr          __var_addr_i0			// __invented__35
	lda         __i4
	sta         (__i0)
	lda         __i4+1
	ldy          #1
	sta         (__i0), Y
.getcwd_label_170:
	ldx          #6
	jsr          __var_addr_i0			// __invented__35
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	ldx          #11
	lda          #__i1
	jsr         __load_result_value2
	bra         .getcwd_label_109
.func_end_getcwd:
	.size getcwd, .func_end_getcwd-getcwd

	.section ".text.chdir", "ax", @progbits
	.global chdir
	.type chdir, @function

chdir:
	lda          #9
	jsr          __enter_res_nomask
	ldx          #0
	jsr          __arg_value2_i0			// path
	.loc 14 118 62
	ldx          #0
	jsr         __pushxy0
	ldx          #0
	jsr         __pushxy0
	jsr         __pushi0
	ldx          #9
	jsr         __pushxy0
	ldx         #__l1
	ldy          #0
	jsr         FsCall
	.loc 14 119 1
	jsr         __pushl1
	ldx         #__i0
	ldy          #0
	jsr         Failed
	lda         __i0
	ora         __i0+1
	beq         .chdir_label_59
	ldx          #5
	jsr          __var_addr_i0			// __invented__37
	lda          #255
	ldy          #0
	sta         (__i0)
	iny         
	sta         (__i0), Y
	bra         .chdir_label_71
.chdir_label_59:
	ldx          #5
	jsr          __var_addr_i0			// __invented__37
	ldy          #1
	lda          #0
.chdir_label_66:
	sta         (__i0), Y
	dey         
	bpl         .chdir_label_66
.chdir_label_71:
	ldx          #5
	jsr          __var_addr_i0			// __invented__37
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	ldx          #10
	lda          #__i1
	jsr         __load_result_value2
	ldy          #14
	jmp          __leave_nomask
.func_end_chdir:
	.size chdir, .func_end_chdir-chdir

	.section ".text.access", "ax", @progbits
	.global access
	.type access, @function

access:
	lda          #138
	jsr          __enter_res
	.byte        0x03,0x06,0x00		// Save mask i:3 b:0 l:3 x:0 f:0 
	ldx          #2
	jsr          __arg_value2_i4			// mode
	ldx          #0
	jsr          __arg_value2_i5			// path
	.loc 14 123 33
	lda         __i4
	and          #248
	sta         __i0
	lda         __i4+1
	sta         __i0+1
	lda         __i0
	ora         __i0+1
	beq         .access_label_78
	.loc 14 124 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #7
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 14 125 1
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #139
	lda          #__i0
	jsr         __load_result_value2
.access_label_75:
	ldy          #145
	jmp          __leave
.access_label_78:
	.loc 14 131 30
	ldx          #131
	lda          #__i6
	jsr          __var_addr_push_i6
	jsr         __pushi5
	ldx         #__i0
	ldy          #0
	jsr         stat
	lda         __i0
	ora         __i0+1
	beq         .access_label_104
	.loc 14 131 30
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #139
	lda          #__i0
	jsr         __load_result_value2
	bra         .access_label_75
.access_label_104:
	.loc 14 132 16
	lda         __i4
	ora         __i4+1
	bne         .access_label_116
	.loc 14 132 16
	stz         __i0
	stz         __i0+1
	ldx          #139
	lda          #__i0
	jsr         __load_result_value2
	bra         .access_label_75
.access_label_116:
	.loc 14 134 66
	ldy          #19
	ldx          #3
.access_label_127:
	lda         (__i6), Y
	sta         __l0, X
	dey         
	dex         
	bpl         .access_label_127
	lda         __l0+3
	lsr          A
	sta         __l1+3
	lda         __l0+2
	ror          A
	sta         __l1+2
	lda         __l0+1
	ror          A
	sta         __l1+1
	lda         __l0
	ror          A
	sta         __l1
	ldx          #5
.access_label_152:
	lsr         __l1+3
	ror         __l1+2
	ror         __l1+1
	ror         __l1
	dex         
	bne         .access_label_152
	lda         __l0+3
	lsr          A
	sta         __l3+3
	lda         __l0+2
	ror          A
	sta         __l3+2
	lda         __l0+1
	ror          A
	sta         __l3+1
	lda         __l0
	ror          A
	sta         __l3
	ldx          #2
.access_label_175:
	lsr         __l3+3
	ror         __l3+2
	ror         __l3+1
	ror         __l3
	dex         
	bne         .access_label_175
	lda         __l1
	ora         __l3
	sta         __l4
	lda         __l1+1
	ora         __l3+1
	sta         __l4+1
	lda         __l1+2
	ora         __l3+2
	sta         __l4+2
	lda         __l1+3
	ora         __l3+3
	sta         __l4+3
	lda         __l4
	ora         __l0
	sta         __l1
	lda         __l4+1
	ora         __l0+1
	sta         __l1+1
	lda         __l4+2
	ora         __l0+2
	sta         __l1+2
	lda         __l4+3
	ora         __l0+3
	sta         __l1+3
	lda         __l1
	and          #7
	sta         __l0
	stz         __l0+1
	stz         __l0+2
	lda          #0
	stz         __l0+3
	ldx          #3
.access_label_229:
	lda         __l0, X
	sta         __l2, X
	dex         
	bpl         .access_label_229
	.loc 14 135 35
	lda         __i4
	and          #4
	sta         __i0
	stz         __i0+1
	ldx          #1
	ora         __i0+1
	bne         .access_label_246
	dex         
.access_label_246:
	stx         __b0
	ldx          #132
	jsr          __var_addr_i1			// __invented__40
	lda         __b0
	sta         (__i1)
	lda         __i0
	ora         __i0+1
	beq         .access_label_311
	lda         __l2
	and          #4
	sta         __l0
	stz         __l0+1
	stz         __l0+2
	stz         __l0+3
	ldx          #1
	ora         __l0+1
	ora         __l0+2
	ora         __l0+3
	bne         .access_label_282
	dex         
.access_label_282:
	txa         
	cmp          #0
	beq         .access_label_295
	lda          #255
.access_label_295:
	inc          A
	sta         (__i1)
.access_label_311:
	lda         (__i1)
	beq         .access_label_533
	jmp         .access_label_509
.access_label_533:
	.loc 14 136 35
	lda         __i4
	and          #2
	sta         __i0
	stz         __i0+1
	ldx          #1
	ora         __i0+1
	bne         .access_label_336
	dex         
.access_label_336:
	stx         __b0
	ldx          #133
	jsr          __var_addr_i2			// __invented__41
	lda         __b0
	sta         (__i2)
	lda         __i0
	ora         __i0+1
	beq         .access_label_400
	lda         __l2
	and          #2
	sta         __l0
	stz         __l0+1
	stz         __l0+2
	stz         __l0+3
	ldx          #1
	ora         __l0+1
	ora         __l0+2
	ora         __l0+3
	bne         .access_label_372
	dex         
.access_label_372:
	txa         
	cmp          #0
	beq         .access_label_385
	lda          #255
.access_label_385:
	inc          A
	sta         (__i2)
.access_label_400:
	lda         (__i2)
	bne         .access_label_509
	.loc 14 137 35
	lda         __i4
	and          #1
	sta         __i0
	stz         __i0+1
	ldx          #1
	ora         __i0+1
	bne         .access_label_425
	dex         
.access_label_425:
	stx         __b0
	ldx          #134
	jsr          __var_addr_i3			// __invented__42
	lda         __b0
	sta         (__i3)
	lda         __i0
	ora         __i0+1
	beq         .access_label_489
	lda         __l2
	and          #1
	sta         __l0
	stz         __l0+1
	stz         __l0+2
	stz         __l0+3
	ldx          #1
	ora         __l0+1
	ora         __l0+2
	ora         __l0+3
	bne         .access_label_461
	dex         
.access_label_461:
	txa         
	cmp          #0
	beq         .access_label_474
	lda          #255
.access_label_474:
	inc          A
	sta         (__i3)
.access_label_489:
	lda         (__i3)
	bne         .access_label_509
	.loc 14 138 1
	stz         __i0
	stz         __i0+1
	ldx          #139
	lda          #__i0
	jsr         __load_result_value2
	jmp         .access_label_75
.access_label_509:
	.loc 14 140 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 14 141 1
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #139
	lda          #__i0
	jsr         __load_result_value2
	jmp         .access_label_75
.func_end_access:
	.size access, .func_end_access-access

	.section ".text.realpath", "ax", @progbits
	.global realpath
	.type realpath, @function

realpath:
	lda          #7
	jsr          __enter_res
	.byte        0x04,0x04,0x00		// Save mask i:4 b:0 l:2 x:0 f:0 
	ldx          #2
	jsr          __arg_value2_i5			// resolved
	ldx          #0
	jsr          __arg_value2_i6			// path
	.loc 14 146 18
	stz         __i4
	stz         __i4+1
	.loc 14 147 29
	lda         __i5
	bne         .realpath_label_80
	lda         __i5+1
	bne         .realpath_label_80
	.loc 14 148 1
	ldx          #0
	ldy          #16
	jsr         __pushxy
	ldx         #__i7
	ldy          #0
	jsr         malloc
	lda         __i7
	sta         __i5
	lda         __i7+1
	sta         __i5+1
	.loc 14 149 29
	lda         __i5
	bne         .realpath_label_74
	lda         __i5+1
	bne         .realpath_label_74
	.loc 14 149 29
	stz         __i0
	stz         __i0+1
	ldx          #8
	lda          #__i0
	jsr         __load_result_value2
.realpath_label_71:
	ldy          #14
	jmp          __leave
.realpath_label_74:
	.loc 14 150 1
	lda          #1
	sta         __i4
	dec          A
	stz         __i4+1
.realpath_label_80:
	.loc 14 153 63
	ldx          #0
	ldy          #16
	jsr         __pushxy
	jsr         __pushi5
	jsr         __pushi6
	ldx          #18
	jsr         __pushxy0
	ldx         #__l3
	ldy          #0
	jsr         FsCall
	ldx          #3
.realpath_label_99:
	lda         __l3, X
	sta         __l2, X
	dex         
	bpl         .realpath_label_99
	.loc 14 154 21
	jsr         __pushl3
	ldx         #__i0
	ldy          #0
	jsr         Failed
	lda         __i0
	ora         __i0+1
	beq         .realpath_label_136
	.loc 14 155 16
	lda         __i4
	ora         __i4+1
	beq         .realpath_label_128
	.loc 14 155 16
	jsr         __pushi5
	jsr         free
	jsr         __incsp2
.realpath_label_128:
	.loc 14 156 1
	stz         __i0
	stz         __i0+1
	ldx          #8
	lda          #__i0
	jsr         __load_result_value2
	bra         .realpath_label_71
.realpath_label_136:
	.loc 14 158 1
	ldx          #8
	lda          #__i5
	jsr         __load_result_value2
	bra         .realpath_label_71
.func_end_realpath:
	.size realpath, .func_end_realpath-realpath

	.section ".text.remove", "ax", @progbits
	.global remove
	.type remove, @function

remove:
	lda          #9
	jsr          __enter_res_nomask
	ldx          #0
	jsr          __arg_value2_i0			// path
	.loc 14 162 54
	ldx          #0
	jsr         __pushxy0
	ldx          #0
	jsr         __pushxy0
	jsr         __pushi0
	ldx          #6
	jsr         __pushxy0
	ldx         #__l1
	ldy          #0
	jsr         FsCall
	.loc 14 163 1
	jsr         __pushl1
	ldx         #__i0
	ldy          #0
	jsr         Failed
	lda         __i0
	ora         __i0+1
	beq         .remove_label_59
	ldx          #5
	jsr          __var_addr_i0			// __invented__50
	lda          #255
	ldy          #0
	sta         (__i0)
	iny         
	sta         (__i0), Y
	bra         .remove_label_71
.remove_label_59:
	ldx          #5
	jsr          __var_addr_i0			// __invented__50
	ldy          #1
	lda          #0
.remove_label_66:
	sta         (__i0), Y
	dey         
	bpl         .remove_label_66
.remove_label_71:
	ldx          #5
	jsr          __var_addr_i0			// __invented__50
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	ldx          #10
	lda          #__i1
	jsr         __load_result_value2
	ldy          #14
	jmp          __leave_nomask
.func_end_remove:
	.size remove, .func_end_remove-remove

	.section ".text.chmod", "ax", @progbits
	.global chmod
	.type chmod, @function

chmod:
	lda          #9
	jsr          __enter_res
	.byte        0x00,0x02,0x00		// Save mask i:0 b:0 l:1 x:0 f:0 
	ldx          #2
	jsr          __arg_value4_l1			// mode
	ldx          #0
	jsr          __arg_value2_i0			// path
	.loc 14 168 61
	lda         __l1
	sta         __i1
	lda         __l1+1
	sta         __i1+1
	ldx          #0
	jsr         __pushxy0
	jsr         __pushi1
	jsr         __pushi0
	ldx          #13
	jsr         __pushxy0
	ldx         #__l2
	ldy          #0
	jsr         FsCall
	.loc 14 169 1
	jsr         __pushl2
	ldx         #__i0
	ldy          #0
	jsr         Failed
	lda         __i0
	ora         __i0+1
	beq         .chmod_label_70
	ldx          #5
	jsr          __var_addr_i0			// __invented__53
	lda          #255
	ldy          #0
	sta         (__i0)
	iny         
	sta         (__i0), Y
	bra         .chmod_label_82
.chmod_label_70:
	ldx          #5
	jsr          __var_addr_i0			// __invented__53
	ldy          #1
	lda          #0
.chmod_label_77:
	sta         (__i0), Y
	dey         
	bpl         .chmod_label_77
.chmod_label_82:
	ldx          #5
	jsr          __var_addr_i0			// __invented__53
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	ldx          #10
	lda          #__i1
	jsr         __load_result_value2
	ldy          #18
	jmp          __leave
.func_end_chmod:
	.size chmod, .func_end_chmod-chmod

	.section ".text.mkstemp", "ax", @progbits
	.global mkstemp
	.type mkstemp, @function

mkstemp:
	lda          #12
	jsr          __enter_res
	.byte        0x05,0x06,0x00		// Save mask i:5 b:0 l:3 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i6			// template_name
	.loc 14 173 34
	lda         __i6
	bne         .mkstemp_label_88
	lda         __i6+1
	bne         .mkstemp_label_88
	.loc 14 174 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #7
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 14 175 1
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #13
	lda          #__i0
	jsr         __load_result_value2
.mkstemp_label_85:
	ldy          #17
	jmp          __leave
.mkstemp_label_88:
	.loc 14 177 38
	jsr         __pushi6
	ldx         #__i7
	ldy          #0
	jsr         strlen
	lda         __i7
	sta         __i5
	lda         __i7+1
	sta         __i5+1
	.loc 14 178 22
	lda          #__i7
	ldx          #5
	jsr         __set_var_value2
	.loc 14 179 55
.mkstemp_label_110:
	ldx          #5
	jsr          __var_value2_i0			// start
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .mkstemp_label_117
	dex         
.mkstemp_label_117:
	stx         __b0
	ldx          #6
	jsr          __var_addr_i7			// __invented__64
	lda         __b0
	sta         (__i7)
	lda         __i0
	ora         __i0+1
	beq         .mkstemp_label_195
	ldx          #5
	jsr          __var_value2_i0			// start
	sec         
	lda         __i0
	sbc          #1
	sta         __i1
	lda         __i0+1
	sbc          #0
	sta         __i1+1
	clc         
	lda         __i6
	adc         __i1
	sta         __i0
	lda         __i6+1
	adc         __i1+1
	sta         __i0+1
	lda         (__i0)
	sta         __i0
	stz         __i0+1
	ldx          #1
	cmp          #88
	bne         .mkstemp_label_176
	lda         __i0+1
	beq         .mkstemp_label_177
.mkstemp_label_176:
	dex         
.mkstemp_label_177:
	txa         
	sta         (__i7)
.mkstemp_label_195:
	lda         (__i7)
	beq         .mkstemp_label_216
	.loc 14 179 55
	ldx          #5
	jsr          __var_addr_i0			// start
	lda          #__i0
	jsr         __dec21
	bra         .mkstemp_label_110
.mkstemp_label_216:
	.loc 14 180 25
	ldx          #5
	jsr          __var_value2_i0			// start
	sec         
	lda         __i5
	sbc         __i0
	sta         __i1
	lda         __i5+1
	sbc         __i0+1
	sta         __i1+1
	cmp          #0
	bcc         .mkstemp_label_231
	bne         .mkstemp_label_263
	lda         __i1
	cmp          #6
	bcs         .mkstemp_label_263
.mkstemp_label_231:
	.loc 14 181 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #7
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 14 182 1
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #13
	lda          #__i0
	jsr         __load_result_value2
	jmp         .mkstemp_label_85
.mkstemp_label_263:
	.loc 14 186 74
	stz         __i0
	stz         __i0+1
	jsr         __pushi0
	ldx         #__l0
	ldy          #0
	jsr         time
	lda         __i6
	sta         __l1
	lda         __i6+1
	sta         __l1+1
	stz         __l1+2
	stz         __l1+3
	lda         __l0
	eor         __l1
	sta         __l4
	lda         __l0+1
	eor         __l1+1
	sta         __l4+1
	lda         __l0+2
	eor         __l1+2
	sta         __l4+2
	lda         __l0+3
	eor         __l1+3
	sta         __l4+3
	ldx          #3
.mkstemp_label_304:
	lda         __l4, X
	sta         __l2, X
	dex         
	bpl         .mkstemp_label_304
	.loc 14 188 65
	.loc 14 189 50
	.loc 14 189 21
.mkstemp_label_313:
	.loc 14 190 70
	ldx          #3
	lda          #0
.mkstemp_label_320:
	sta         __l0, X
	dex         
	bpl         .mkstemp_label_320
	ldx          #3
.mkstemp_label_328:
	lda         .lit.197, X
	sta         __l1, X
	dex         
	bpl         .mkstemp_label_328
	lda          #__l4
	ldx          #__l0
	ldy          #__l1
	jsr         __umul4
	clc         
	ldy          #4
	ldx          #0
.mkstemp_label_349:
	lda         __l2, X
	adc         __l4, X
	sta         __l0, X
	inx         
	dey         
	bne         .mkstemp_label_349
	ldx          #3
.mkstemp_label_359:
	lda         __l0, X
	sta         __l3, X
	dex         
	bpl         .mkstemp_label_359
	.loc 14 191 54
	.loc 14 191 26
	ldx          #5
	jsr          __var_value2_i0			// start
	lda          #__i0
	ldx          #8
	jsr         __set_var_value2
.mkstemp_label_374:
	ldx          #8
	jsr          __var_value2_i0			// index
	lda         __i0
	cmp         __i5
	bne         .mkstemp_label_378
	lda         __i0+1
	cmp         __i5+1
	beq         .mkstemp_label_495
.mkstemp_label_378:
	.loc 14 192 1
	ldx          #3
.mkstemp_label_392:
	lda         .lit.198, X
	sta         __l0, X
	dex         
	bpl         .mkstemp_label_392
	lda          #__l1
	ldx          #__l3
	ldy          #__l0
	jsr         __umul4
	clc         
	ldy          #4
	ldx          #0
.mkstemp_label_411:
	lda         __l1, X
	adc         .lit.199, X
	sta         __l0, X
	inx         
	dey         
	bne         .mkstemp_label_411
	ldx          #3
.mkstemp_label_421:
	lda         __l0, X
	sta         __l3, X
	dex         
	bpl         .mkstemp_label_421
	.loc 14 193 1
	ldx          #3
.mkstemp_label_432:
	lda         .lit.213, X
	sta         __l1, X
	dex         
	bpl         .mkstemp_label_432
	lda          #__l4
	ldx          #__l0
	ldy          #__l1
	jsr         __umod4
	lda         #%lo(.local.alphabet.3810)
	sta         __i0
	lda         #%hi(.local.alphabet.3810)
	sta         __i0+1
	clc         
	lda         __i0
	adc         __l4
	sta         __i1
	lda         __i0+1
	adc         __l4+1
	sta         __i1+1
	lda         (__i1)
	sta         __b0
	ldx          #8
	jsr          __var_value2_i0			// index
	clc         
	lda         __i6
	adc         __i0
	sta         __i1
	lda         __i6+1
	adc         __i0+1
	sta         __i1+1
	lda         __b0
	sta         (__i1)
	ldx          #8
	jsr          __var_addr_i0			// index
	lda          #__i0
	jsr         __inc21
	jmp         .mkstemp_label_374
.mkstemp_label_495:
	.loc 14 195 67
	ldx          #128
	ldy          #1
	jsr         __pushxy
	ldx 