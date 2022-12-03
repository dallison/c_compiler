	.file   "risc_v_emitter.c"
	.text
	.option pic
.PCbegin:
	.local  IsPrintable
	.type IsPrintable, @function

IsPrintable:

	// *** Basic block 0

	.global TargetIsConst
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	call        TargetIsConst

	// *** Basic block 1

	beqz        a0, .IsPrintable_label_49

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.IsPrintable_label_46:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.IsPrintable_label_49:
	lw          t0, 16(s1)
	li          t1, 195		// 0xc3 ASCII \xc3
	blt         t0, t1, .IsPrintable_label_131

	// *** Basic block 5

	beq         t0, t1, .IsPrintable_label_221

	// *** Basic block 6

	li          t1, 196		// 0xc4 ASCII \xc4
	beq         t0, t1, .IsPrintable_label_222

	// *** Basic block 7

	li          t1, 197		// 0xc5 ASCII \xc5
	beq         t0, t1, .IsPrintable_label_223

	// *** Basic block 8

	li          t1, 198		// 0xc6 ASCII \xc6
	beq         t0, t1, .IsPrintable_label_224

	// *** Basic block 9

	li          t1, 199		// 0xc7 ASCII \xc7
	beq         t0, t1, .IsPrintable_label_225

	// *** Basic block 10

	li          t1, 200		// 0xc8 ASCII \xc8
	beq         t0, t1, .IsPrintable_label_226

	// *** Basic block 11

	li          t1, 201		// 0xc9 ASCII \xc9
	beq         t0, t1, .IsPrintable_label_227

	// *** Basic block 12

	li          t1, 202		// 0xca ASCII \xca
	beq         t0, t1, .IsPrintable_label_228

	// *** Basic block 13

	li          t1, 203		// 0xcb ASCII \xcb
	beq         t0, t1, .IsPrintable_label_229

	// *** Basic block 14

	li          t1, 204		// 0xcc ASCII \xcc
	beq         t0, t1, .IsPrintable_label_230

	// *** Basic block 15

	li          t1, 205		// 0xcd ASCII \xcd
	beq         t0, t1, .IsPrintable_label_231

	// *** Basic block 16

	li          t1, 206		// 0xce ASCII \xce
	beq         t0, t1, .IsPrintable_label_237

	// *** Basic block 17

	li          t1, 207		// 0xcf ASCII \xcf
	beq         t0, t1, .IsPrintable_label_235

	// *** Basic block 18

	li          t1, 208		// 0xd0 ASCII \xd0
	beq         t0, t1, .IsPrintable_label_236

	// *** Basic block 19

	li          t1, 209		// 0xd1 ASCII \xd1
	beq         t0, t1, .IsPrintable_label_232

	// *** Basic block 20

	j           .IsPrintable_label_241

	// *** Basic block 21

.IsPrintable_label_131:
	li          t1, 3		// 0x3 ASCII \x3
	beq         t0, t1, .IsPrintable_label_211

	// *** Basic block 22

	li          t1, 4		// 0x4 ASCII \x4
	beq         t0, t1, .IsPrintable_label_208

	// *** Basic block 23

	li          t1, 23		// 0x17 ASCII \x17
	beq         t0, t1, .IsPrintable_label_209

	// *** Basic block 24

	li          t1, 24		// 0x18 ASCII \x18
	beq         t0, t1, .IsPrintable_label_210

	// *** Basic block 25

	li          t1, 26		// 0x1a ASCII \x1a
	beq         t0, t1, .IsPrintable_label_213

	// *** Basic block 26

	li          t1, 27		// 0x1b ASCII \x1b
	beq         t0, t1, .IsPrintable_label_214

	// *** Basic block 27

	li          t1, 28		// 0x1c ASCII \x1c
	beq         t0, t1, .IsPrintable_label_215

	// *** Basic block 28

	li          t1, 29		// 0x1d ASCII \x1d
	beq         t0, t1, .IsPrintable_label_212

	// *** Basic block 29

	li          t1, 33		// 0x21 ASCII '!'
	beq         t0, t1, .IsPrintable_label_233

	// *** Basic block 30

	li          t1, 34		// 0x22 ASCII '"'
	beq         t0, t1, .IsPrintable_label_234

	// *** Basic block 31

	li          t1, 190		// 0xbe ASCII \xbe
	beq         t0, t1, .IsPrintable_label_216

	// *** Basic block 32

	li          t1, 191		// 0xbf ASCII \xbf
	beq         t0, t1, .IsPrintable_label_217

	// *** Basic block 33

	li          t1, 192		// 0xc0 ASCII \xc0
	beq         t0, t1, .IsPrintable_label_218

	// *** Basic block 34

	li          t1, 193		// 0xc1 ASCII \xc1
	beq         t0, t1, .IsPrintable_label_219

	// *** Basic block 35

	li          t1, 194		// 0xc2 ASCII \xc2
	beq         t0, t1, .IsPrintable_label_220

	// *** Basic block 36

	j           .IsPrintable_label_241

	// *** Basic block 37

.IsPrintable_label_208:

	// *** Basic block 38

.IsPrintable_label_209:

	// *** Basic block 39

.IsPrintable_label_210:

	// *** Basic block 40

.IsPrintable_label_211:

	// *** Basic block 41

.IsPrintable_label_212:

	// *** Basic block 42

.IsPrintable_label_213:

	// *** Basic block 43

.IsPrintable_label_214:

	// *** Basic block 44

.IsPrintable_label_215:

	// *** Basic block 45

.IsPrintable_label_216:

	// *** Basic block 46

.IsPrintable_label_217:

	// *** Basic block 47

.IsPrintable_label_218:

	// *** Basic block 48

.IsPrintable_label_219:

	// *** Basic block 49

.IsPrintable_label_220:

	// *** Basic block 50

.IsPrintable_label_221:

	// *** Basic block 51

.IsPrintable_label_222:

	// *** Basic block 52

.IsPrintable_label_223:

	// *** Basic block 53

.IsPrintable_label_224:

	// *** Basic block 54

.IsPrintable_label_225:

	// *** Basic block 55

.IsPrintable_label_226:

	// *** Basic block 56

.IsPrintable_label_227:

	// *** Basic block 57

.IsPrintable_label_228:

	// *** Basic block 58

.IsPrintable_label_229:

	// *** Basic block 59

.IsPrintable_label_230:

	// *** Basic block 60

.IsPrintable_label_231:

	// *** Basic block 61

.IsPrintable_label_232:

	// *** Basic block 62

.IsPrintable_label_233:

	// *** Basic block 63

.IsPrintable_label_234:

	// *** Basic block 64

.IsPrintable_label_235:

	// *** Basic block 65

.IsPrintable_label_236:

	// *** Basic block 66

.IsPrintable_label_237:
	mv          a0, x0
	j           .IsPrintable_label_46

	// *** Basic block 67

.IsPrintable_label_241:

	// *** Basic block 68

.IsPrintable_label_242:
	li          a0, 1		// 0x1 ASCII \x1
	j           .IsPrintable_label_46
.func_end_IsPrintable:
	.size IsPrintable, .func_end_IsPrintable-IsPrintable

	.local  StackFrameSize
	.type StackFrameSize, @function

StackFrameSize:

	// *** Basic block 0

	.global BitSetCount
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0
	ld          s2, 0(s1)
	lw          t0, 128(s2)
	addi        s3, t0, 16
	lb          t0, 48(s2)
	beqz        t0, .StackFrameSize_label_40

	// *** Basic block 1

	lw          t0, 184(s2)
	li          t1, 8		// 0x8 ASCII \x8
	bge         t0, t1, .StackFrameSize_label_39

	// *** Basic block 2

	sub         t0, t1, t0
	slli        t0, t0, 3
	add         s3, s3, t0

	// *** Basic block 3

.StackFrameSize_label_39:

	// *** Basic block 4

.StackFrameSize_label_40:
	ld          t0, 8(s1)
	addi        a0, t0, 1544
	call        BitSetCount

	// *** Basic block 5

	slli        t0, a0, 3
	sext.w      t0, t0
	add         s3, s3, t0
	ld          t0, 8(s1)
	addi        a0, t0, 1560
	call        BitSetCount

	// *** Basic block 6

	slli        t0, a0, 3
	sext.w      t0, t0
	add         s3, s3, t0
	lw          t0, 20(s1)
	add         s3, s3, t0
	addi        t0, s3, 15
	andi        s3, t0, -16
	mv          a0, s3

	// *** Basic block 7

.StackFrameSize_label_64:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StackFrameSize:
	.size StackFrameSize, .func_end_StackFrameSize-StackFrameSize

	.local  EmptyStackFrame
	.type EmptyStackFrame, @function

EmptyStackFrame:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 0(a0)
	lw          t1, 128(t0)
	seqz        a0, t1
	bnez        t1, .EmptyStackFrame_label_20

	// *** Basic block 1

	lw          t1, 44(t0)
	seqz        a0, t1

	// *** Basic block 2

.EmptyStackFrame_label_20:
	beqz        a0, .EmptyStackFrame_label_25

	// *** Basic block 3

	lb          t0, 204(t0)
	not         a0, t0

	// *** Basic block 4

.EmptyStackFrame_label_25:

	// *** Basic block 5

.EmptyStackFrame_label_27:
	ret         
.func_end_EmptyStackFrame:
	.size EmptyStackFrame, .func_end_EmptyStackFrame-EmptyStackFrame

	.local  DecrementStackPointer
	.type DecrementStackPointer, @function

DecrementStackPointer:

	// *** Basic block 0

	.global fprintf
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a2
	li          t0, 2047		// 0x7ff
	bge         t0, s1, .DecrementStackPointer_label_45

	// *** Basic block 1

	lla         a1, .str.1
	srai        a2, s1, 12
	mv          a0, s2
	call        fprintf

	// *** Basic block 2

	lla         a1, .str.2
	li          t0, 4095		// 0xfff
	and         a2, s1, t0
	mv          a0, s2
	call        fprintf

	// *** Basic block 3

	lla         a1, .str.3
	mv          a0, s2
	call        fprintf

	// *** Basic block 4

	j           .DecrementStackPointer_label_55

	// *** Basic block 5

.DecrementStackPointer_label_45:
	lla         a1, .str.4
	mv          a2, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           fprintf

	// *** Basic block 6

.DecrementStackPointer_label_55:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DecrementStackPointer:
	.size DecrementStackPointer, .func_end_DecrementStackPointer-DecrementStackPointer

	.local  IncrementStackPointer
	.type IncrementStackPointer, @function

IncrementStackPointer:

	// *** Basic block 0

	.global fprintf
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a2
	li          t0, 2047		// 0x7ff
	bge         t0, s1, .IncrementStackPointer_label_45

	// *** Basic block 1

	lla         a1, .str.5
	srai        a2, s1, 12
	mv          a0, s2
	call        fprintf

	// *** Basic block 2

	lla         a1, .str.6
	li          t0, 4095		// 0xfff
	and         a2, s1, t0
	mv          a0, s2
	call        fprintf

	// *** Basic block 3

	lla         a1, .str.7
	mv          a0, s2
	call        fprintf

	// *** Basic block 4

	j           .IncrementStackPointer_label_55

	// *** Basic block 5

.IncrementStackPointer_label_45:
	lla         a1, .str.8
	mv          a2, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           fprintf

	// *** Basic block 6

.IncrementStackPointer_label_55:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_IncrementStackPointer:
	.size IncrementStackPointer, .func_end_IncrementStackPointer-IncrementStackPointer

	.local  LoadRegisterFromFrame
	.type LoadRegisterFromFrame, @function

LoadRegisterFromFrame:

	// *** Basic block 0

	.global fprintf
	.global RVRegisterNameFromNum
	addi sp, sp, -336
	// Saved return address (offset 328) and frame pointer (offset 320)
	sd ra, 328(sp)
	sd s0, 320(sp)
	addi s0, sp, 336
	// Local vars at offset -272(s0)
	// Saved integer registers.
	sd s1, 56(sp)
	sd s2, 48(sp)
	sd s3, 40(sp)
	sd s4, 32(sp)
	sd s5, 24(sp)
	sd s6, 16(sp)
	sd s7, 8(sp)
	// End of stack frame
	mv          t0, a3
	mv          s1, a2
	mv          s2, a5
	mv          s3, a1
	mv          s4, a4
	beqz        t0, .LoadRegisterFromFrame_label_36

	// *** Basic block 1

	lla         s5, .str.9
	j           .LoadRegisterFromFrame_label_39

	// *** Basic block 2

.LoadRegisterFromFrame_label_36:
	lla         s5, .str.10

	// *** Basic block 3

.LoadRegisterFromFrame_label_39:
	beqz        t0, .LoadRegisterFromFrame_label_45

	// *** Basic block 4

	li          s6, 1		// 0x1 ASCII \x1
	j           .LoadRegisterFromFrame_label_48

	// *** Basic block 5

.LoadRegisterFromFrame_label_45:

	// *** Basic block 6

.LoadRegisterFromFrame_label_48:
	li          t0, 2047		// 0x7ff
	bge         s1, t0, .LoadRegisterFromFrame_label_80

	// *** Basic block 7

	lla         s7, .str.11
	addi        a2, s0, -272
	li          t0, 256		// 0x100
	mv          a3, t0
	mv          a1, s6
	mv          a0, s3
	call        RVRegisterNameFromNum

	// *** Basic block 8

	mv          a4, s1
	mv          a3, a0
	mv          a2, s5
	mv          a1, s7
	mv          a0, s2
	call        fprintf

	// *** Basic block 9

	j           .LoadRegisterFromFrame_label_116

	// *** Basic block 10

.LoadRegisterFromFrame_label_80:
	lla         a1, .str.12
	mv          a2, s1
	mv          a0, s2
	call        fprintf

	// *** Basic block 11

	lla         a1, .str.13
	mv          a0, s2
	call        fprintf

	// *** Basic block 12

	lla         s7, .str.14
	addi        a2, s0, -272
	li          t0, 256		// 0x100
	mv          a3, t0
	mv          a1, s6
	mv          a0, s3
	call        RVRegisterNameFromNum

	// *** Basic block 13

	mv          a3, a0
	mv          a2, s5
	mv          a1, s7
	mv          a0, s2
	call        fprintf

	// *** Basic block 14

.LoadRegisterFromFrame_label_116:
	lla         a1, .str.15
	mv          a2, s4
	mv          a0, s2
	call        fprintf

	// *** Basic block 15

	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LoadRegisterFromFrame:
	.size LoadRegisterFromFrame, .func_end_LoadRegisterFromFrame-LoadRegisterFromFrame

	.local  GenerateOffsetFromFrame
	.type GenerateOffsetFromFrame, @function

GenerateOffsetFromFrame:

	// *** Basic block 0

	.global fprintf
	.global RVRegisterNameFromNum
	addi sp, sp, -320
	// Saved return address (offset 312) and frame pointer (offset 304)
	sd ra, 312(sp)
	sd s0, 304(sp)
	addi s0, sp, 320
	// Local vars at offset -272(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// End of stack frame
	mv          s1, a2
	mv          s2, a4
	mv          s3, a1
	mv          s4, a3
	li          t0, 2047		// 0x7ff
	bge         s1, t0, .GenerateOffsetFromFrame_label_52

	// *** Basic block 1

	lla         s5, .str.16
	addi        a2, s0, -272
	li          t0, 256		// 0x100
	mv          a3, t0
	mv          a1, x0
	mv          a0, s3
	call        RVRegisterNameFromNum

	// *** Basic block 2

	mv          a3, s1
	mv          a2, a0
	mv          a1, s5
	mv          a0, s2
	call        fprintf

	// *** Basic block 3

	j           .GenerateOffsetFromFrame_label_80

	// *** Basic block 4

.GenerateOffsetFromFrame_label_52:
	lla         a1, .str.17
	mv          a2, s1
	mv          a0, s2
	call        fprintf

	// *** Basic block 5

	lla         s5, .str.18
	addi        a2, s0, -272
	li          t0, 256		// 0x100
	mv          a3, t0
	mv          a1, x0
	mv          a0, s3
	call        RVRegisterNameFromNum

	// *** Basic block 6

	mv          a2, a0
	mv          a1, s5
	mv          a0, s2
	call        fprintf

	// *** Basic block 7

.GenerateOffsetFromFrame_label_80:
	lla         a1, .str.19
	mv          a2, s4
	mv          a0, s2
	call        fprintf

	// *** Basic block 8

	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GenerateOffsetFromFrame:
	.size GenerateOffsetFromFrame, .func_end_GenerateOffsetFromFrame-GenerateOffsetFromFrame

	.local  SaveRegisters
	.type SaveRegisters, @function

SaveRegisters:

	// *** Basic block 0

	.local StackFrameSize
	.global OptLevel1
	.global fprintf
	.local EmptyStackFrame
	.local DecrementStackPointer
	.global RVRegisterNameFromNum
	.global BitSetIteratorStart
	.global BitSetIteratorDone
	.global BitSetIteratorNext
	addi sp, sp, -208
	// Saved return address (offset 200) and frame pointer (offset 192)
	sd ra, 200(sp)
	sd s0, 192(sp)
	addi s0, sp, 208
	// Local vars at offset -64(s0)
	// Spilled register region: 56 bytes at -120(s0) to -64(s0)
	// Saved integer registers.
	sd s1, 80(sp)
	sd s2, 72(sp)
	sd s3, 64(sp)
	sd s4, 56(sp)
	sd s5, 48(sp)
	sd s6, 40(sp)
	sd s7, 32(sp)
	sd s8, 24(sp)
	sd s9, 16(sp)
	sd s10, 8(sp)
	sd s11, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	call        StackFrameSize

	// *** Basic block 1

	addi        s3, s0, -64
	addi        s4, s0, -56
	addi        s5, s0, -64
	addi        s6, s0, -64
	mv          s7, a0
	sd          s7, -96(s0)	// Spilled @66
	sd          s8, -104(s0)	// Spilled @69
	ld          s9, 0(s1)
	lw          t0, 44(s9)
	seqz        s8, t0
	bnez        t0, .SaveRegisters_label_78

	// *** Basic block 2

	call        OptLevel1

	// *** Basic block 3

	mv          s8, a0

	// *** Basic block 4

.SaveRegisters_label_78:
	beqz        s8, .SaveRegisters_label_83

	// *** Basic block 5

	lb          t0, 204(s9)
	not         s8, t0

	// *** Basic block 6

.SaveRegisters_label_83:
	lb          s10, 48(s9)
	beqz        s10, .SaveRegisters_label_98
	sd          s10, -80(s0)	// Spilled @87

	// *** Basic block 7

	lw          t0, 184(s9)
	li          t1, 8		// 0x8 ASCII \x8
	sub         t0, t1, t0
	slli        s11, t0, 3
	j           .SaveRegisters_label_100

	// *** Basic block 8

.SaveRegisters_label_98:

	// *** Basic block 9

.SaveRegisters_label_100:
	bge         s11, x0, .SaveRegisters_label_105
	sd          s11, -112(s0)	// Spilled @101

	// *** Basic block 10

	mv          s11, x0

	// *** Basic block 11

.SaveRegisters_label_105:
	beqz        s8, .SaveRegisters_label_113
	sd          s8, -96(s0)	// Spilled @84

	// *** Basic block 12

	lla         a1, .str.20
	mv          a0, s2
	call        fprintf

	// *** Basic block 13

.SaveRegisters_label_113:
	addi        t0, s7, -8
	sd          t0, -72(s0)	// Spilled @114
	sub         t0, t0, s11
	addi        s10, s7, -16
	sd          s10, -96(s0)	// Spilled @118
	sub         t0, s10, s11
	lw          t1, 128(s9)
	sd          t1, -96(s0)	// Spilled @123
	addi        t1, t1, 16
	addi        t2, s10, -8
	ld          t3, -96(s0)	// Spilled @123
	sub         t2, t2, t3
	sub         t2, t2, s11
	sd          t2, -104(s0)	// Spilled @128
	lw          t4, 20(s1)
	sub         t2, t2, t4
	beqz        s8, .SaveRegisters_label_136

	// *** Basic block 14

	addi        t0, t0, 8
	addi        t2, t2, 8

	// *** Basic block 15

.SaveRegisters_label_136:
	mv          a0, s1
	call        EmptyStackFrame

	// *** Basic block 16

	beqz        a0, .SaveRegisters_label_142

	// *** Basic block 17

	j           .SaveRegisters_label_294

	// *** Basic block 18

.SaveRegisters_label_142:
	li          s10, 2047		// 0x7ff
	bge         s10, s7, .SaveRegisters_label_179

	// *** Basic block 19

	mv          a2, s2
	li          t3, 16		// 0x10 ASCII \x10
	mv          a1, t3
	mv          a0, s1
	call        DecrementStackPointer

	// *** Basic block 20

	not         t3, s8
	beqz        t3, .SaveRegisters_label_164

	// *** Basic block 21

	lla         a1, .str.21
	mv          a0, s2
	call        fprintf

	// *** Basic block 22

.SaveRegisters_label_164:
	lla         a1, .str.22
	mv          a0, s2
	call        fprintf

	// *** Basic block 23

	mv          a2, s2
	ld          a1, -96(s0)	// Spilled @118
	mv          a0, s1
	call        DecrementStackPointer

	// *** Basic block 24

	j           .SaveRegisters_label_217

	// *** Basic block 25

.SaveRegisters_label_179:
	mv          a2, s2
	mv          a1, s7
	mv          a0, s1
	call        DecrementStackPointer

	// *** Basic block 26

	lla         a1, .str.23
	mv          a3, t0
	ld          a2, -88(s0)	// Spilled @117
	sd          t0, -88(s0)	// Spilled @117
	mv          a0, s2
	call        fprintf

	// *** Basic block 27

	not         t3, s8
	beqz        t3, .SaveRegisters_label_208

	// *** Basic block 28

	lla         a1, .str.24
	ld          a2, -88(s0)	// Spilled @117
	mv          a0, s2
	call        fprintf

	// *** Basic block 29

.SaveRegisters_label_208:
	lla         a1, .str.25
	mv          a2, t0
	mv          a0, s2
	call        fprintf

	// *** Basic block 30

.SaveRegisters_label_217:
	bge         s10, s7, .SaveRegisters_label_244

	// *** Basic block 31

	bge         x0, s11, .SaveRegisters_label_230

	// *** Basic block 32

	lla         a1, .str.26
	mv          a2, s11
	mv          a0, s2
	call        fprintf

	// *** Basic block 33

.SaveRegisters_label_230:
	lla         a1, .str.27
	mv          a0, s2
	call        fprintf

	// *** Basic block 34

	lla         a1, .str.28
	mv          a0, s2
	call        fprintf

	// *** Basic block 35

	j           .SaveRegisters_label_253

	// *** Basic block 36

.SaveRegisters_label_244:
	lla         a1, .str.29
	sub         a2, s7, s11
	mv          a0, s2
	call        fprintf

	// *** Basic block 37

.SaveRegisters_label_253:
	ld          t0, -80(s0)	// Spilled @87
	beqz        t0, .SaveRegisters_label_293

	// *** Basic block 38

	lw          a2, 184(s9)
	li          s10, 8		// 0x8 ASCII \x8
	sub         s7, s10, a2
	mv          s7, x0
	sd          s7, -96(s0)	// Spilled @260
	lla         a1, .str.30
	mv          a0, s2
	call        fprintf

	// *** Basic block 39

	ld          t0, -104(s0)	// Spilled @259
	sub         s8, s10, t0
	sd          s7, -104(s0)	// Spilled @259
	bge         s8, s10, .SaveRegisters_label_292

	// *** Basic block 40

.SaveRegisters_label_275:
	lla         a1, .str.31
	mv          a3, s7
	mv          a2, s8
	mv          a0, s2
	call        fprintf

	// *** Basic block 42

.SaveRegisters_label_287:
	addi        s8, s8, 1
	bge         s8, s10, .SaveRegisters_label_275

	// *** Basic block 43

.SaveRegisters_label_292:

	// *** Basic block 44

.SaveRegisters_label_293:

	// *** Basic block 45

.SaveRegisters_label_294:
	addi        t0, s9, 208
	ld          s10, 8(t0)
	bge         x0, s10, .SaveRegisters_label_307

	// *** Basic block 46

	ld          s9, 208(s9)
	lla         a1, .str.32
	mv          a0, s2
	call        fprintf

	// *** Basic block 47

.SaveRegisters_label_307:
	mv          s9, x0
	bge         x0, s10, .SaveRegisters_label_359

	// *** Basic block 48

.SaveRegisters_label_312:
	slli        t0, s9, 3
	add         t0, s9, t0
	ld          s7, 0(t0)
	lw          s8, 8(s7)
	lla         s11, .str.33
	lw          a0, 0(s7)
	li          t1, 8		// 0x8 ASCII \x8
	mv          a3, t1
	mv          a2, s3
	mv          a1, x0
	call        RVRegisterNameFromNum

	// *** Basic block 49

	lw          a0, 4(s7)
	mv          a3, t1
	mv          a2, s4
	mv          a1, x0
	call        RVRegisterNameFromNum

	// *** Basic block 50

	mv          a4, a0
	mv          a3, s8
	mv          a2, a0
	mv          a1, s11
	mv          a0, s2
	call        fprintf

	// *** Basic block 51

.SaveRegisters_label_355:
	addi        s9, s9, 1
	bge         s9, s10, .SaveRegisters_label_312

	// *** Basic block 52

.SaveRegisters_label_359:
	ld          t0, -96(s0)	// Spilled @84
	not         s3, t0
	beqz        s3, .SaveRegisters_label_370

	// *** Basic block 53

	lla         a1, .str.34
	ld          a2, -120(s0)	// Spilled @125
	sd          t1, -120(s0)	// Spilled @125
	mv          a0, s2
	call        fprintf

	// *** Basic block 54

.SaveRegisters_label_370:
	ld          s4, -120(s0)	// Spilled @125
	addi        s7, s4, 8
	sw          s7, 24(s1)
	lw          s8, 20(s1)
	bge         x0, s8, .SaveRegisters_label_394

	// *** Basic block 55

	lla         a1, .str.35
	add         t0, s7, s8
	addi        a3, t0, -8
	addi        a4, s7, -8
	mv          a2, s8
	mv          a0, s2
	call        fprintf

	// *** Basic block 56

.SaveRegisters_label_394:
	sw          t2, 16(s1)
	addi        a0, s0, -48
	ld          t0, 8(s1)
	addi        a1, t0, 1544
	call        BitSetIteratorStart

	// *** Basic block 57

	addi        a0, s0, -48
	call        BitSetIteratorDone

	// *** Basic block 58

	not         t0, a0
	beqz        t0, .SaveRegisters_label_416

	// *** Basic block 59

	lla         a1, .str.36
	mv          a0, s2
	call        fprintf

	// *** Basic block 60

.SaveRegisters_label_416:
	addi        a0, s0, -48
	call        BitSetIteratorDone

	// *** Basic block 61

	not         t0, a0
	beqz        t0, .SaveRegisters_label_468

	// *** Basic block 62

.SaveRegisters_label_422:
	addi        s4, s0, -48
	ld          t0, 8(s4)
	slli        t0, t0, 5
	ld          t1, 16(s4)
	add         s4, t0, t1

	// *** Basic block 63

.SaveRegisters_label_433:
	sext.w      s7, s4
	mv          s4, t2
	addi        t2, t2, -8
	lla         a1, .str.37
	li          t0, 8		// 0x8 ASCII \x8
	mv          a3, t0
	mv          a2, s5
	mv          a1, x0
	mv          a0, s7
	call        RVRegisterNameFromNum

	// *** Basic block 64

	mv          a3, s4
	mv          a2, a0
	mv          a0, s2
	call        fprintf

	// *** Basic block 65

	addi        a0, s0, -48
	call        BitSetIteratorNext

	// *** Basic block 66

	addi        a0, s0, -48
	call        BitSetIteratorDone

	// *** Basic block 67

	not         t0, a0
	bnez        t0, .SaveRegisters_label_422

	// *** Basic block 68

	j           .SaveRegisters_label_468

	// *** Basic block 69

.SaveRegisters_label_468:
	addi        a0, s0, -48
	ld          t0, 8(s1)
	addi        a1, t0, 1560
	call        BitSetIteratorStart

	// *** Basic block 70

	addi        a0, s0, -48
	call        BitSetIteratorDone

	// *** Basic block 71

	not         t0, a0
	beqz        t0, .SaveRegisters_label_487

	// *** Basic block 72

	lla         a1, .str.38
	mv          a0, s2
	call        fprintf

	// *** Basic block 73

.SaveRegisters_label_487:
	addi        a0, s0, -48
	call        BitSetIteratorDone

	// *** Basic block 74

	not         t0, a0
	beqz        t0, .SaveRegisters_label_539

	// *** Basic block 75

.SaveRegisters_label_493:
	addi        s5, s0, -48
	ld          t0, 8(s5)
	slli        t0, t0, 5
	ld          t1, 16(s5)
	add         s5, t0, t1

	// *** Basic block 76

.SaveRegisters_label_503:
	sext.w      s7, s5
	mv          s5, t2
	addi        t2, t2, -8
	lla         a1, .str.39
	li          t0, 8		// 0x8 ASCII \x8
	mv          a3, t0
	mv          a2, s6
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s7
	call        RVRegisterNameFromNum

	// *** Basic block 77

	mv          a3, s5
	mv          a2, a0
	mv          a0, s2
	call        fprintf

	// *** Basic block 78

	addi        a0, s0, -48
	call        BitSetIteratorNext

	// *** Basic block 79

	addi        a0, s0, -48
	call        BitSetIteratorDone

	// *** Basic block 80

	not         t0, a0
	bnez        t0, .SaveRegisters_label_493

	// *** Basic block 81

	j           .SaveRegisters_label_539

	// *** Basic block 82

.SaveRegisters_label_539:
	beqz        s3, .SaveRegisters_label_547

	// *** Basic block 83

	lla         a1, .str.40
	mv          a0, s2
	call        fprintf

	// *** Basic block 84

.SaveRegisters_label_547:
	// Restored registers.
	ld s1, 80(sp)
	ld s2, 72(sp)
	ld s3, 64(sp)
	ld s4, 56(sp)
	ld s5, 48(sp)
	ld s6, 40(sp)
	ld s7, 32(sp)
	ld s8, 24(sp)
	ld s9, 16(sp)
	ld s10, 8(sp)
	ld s11, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SaveRegisters:
	.size SaveRegisters, .func_end_SaveRegisters-SaveRegisters

	.local  RestoreRegisters
	.type RestoreRegisters, @function

RestoreRegisters:

	// *** Basic block 0

	.local StackFrameSize
	.global OptLevel1
	.global fprintf
	.global BitSetIteratorStart
	.global BitSetIteratorDone
	.global RVRegisterNameFromNum
	.global BitSetIteratorNext
	.local EmptyStackFrame
	addi sp, sp, -160
	// Saved return address (offset 152) and frame pointer (offset 144)
	sd ra, 152(sp)
	sd s0, 144(sp)
	addi s0, sp, 160
	// Local vars at offset -80(s0)
	// Saved integer registers.
	sd s1, 72(sp)
	sd s2, 64(sp)
	sd s3, 56(sp)
	sd s4, 48(sp)
	sd s5, 40(sp)
	sd s6, 32(sp)
	sd s7, 24(sp)
	sd s8, 16(sp)
	sd s9, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	call        StackFrameSize

	// *** Basic block 1

	addi        s3, s0, -32
	addi        s4, s0, -32
	mv          s5, a0
	ld          s7, 0(s1)
	lw          t0, 44(s7)
	seqz        s6, t0
	bnez        t0, .RestoreRegisters_label_54

	// *** Basic block 2

	call        OptLevel1

	// *** Basic block 3

	mv          s6, a0

	// *** Basic block 4

.RestoreRegisters_label_54:
	beqz        s6, .RestoreRegisters_label_59

	// *** Basic block 5

	lb          t0, 204(s7)
	not         s6, t0

	// *** Basic block 6

.RestoreRegisters_label_59:
	lb          s8, 48(s7)
	beqz        s8, .RestoreRegisters_label_74

	// *** Basic block 7

	lw          t0, 184(s7)
	li          t1, 8		// 0x8 ASCII \x8
	sub         t0, t1, t0
	slli        s9, t0, 3
	j           .RestoreRegisters_label_76

	// *** Basic block 8

.RestoreRegisters_label_74:

	// *** Basic block 9

.RestoreRegisters_label_76:
	bge         s9, x0, .RestoreRegisters_label_81

	// *** Basic block 10

	mv          s9, x0

	// *** Basic block 11

.RestoreRegisters_label_81:
	addi        t0, s5, -16
	sub         t1, t0, s9
	beqz        s6, .RestoreRegisters_label_89

	// *** Basic block 12

	j           .RestoreRegisters_label_96

	// *** Basic block 13

.RestoreRegisters_label_89:
	lla         a1, .str.41
	mv          a0, s2
	call        fprintf

	// *** Basic block 14

.RestoreRegisters_label_96:
	sd          x0, -80(s0)
	sd          x0, -72(s0)
	sd          x0, -64(s0)
	sd          x0, -80(s0)
	lw          s5, 16(s1)
	addi        a0, s0, -56
	ld          t0, 8(s1)
	addi        a1, t0, 1560
	call        BitSetIteratorStart

	// *** Basic block 15

	addi        a0, s0, -56
	call        BitSetIteratorDone

	// *** Basic block 16

	not         t0, a0
	beqz        t0, .RestoreRegisters_label_168

	// *** Basic block 17

.RestoreRegisters_label_120:
	addi        s7, s0, -56
	ld          t0, 8(s7)
	slli        t0, t0, 5
	ld          t1, 16(s7)
	add         s7, t0, t1

	// *** Basic block 18

.RestoreRegisters_label_131:
	sext.w      s8, s7
	lla         a1, .str.42
	li          t0, 8		// 0x8 ASCII \x8
	mv          a3, t0
	mv          a2, s3
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s8
	call        RVRegisterNameFromNum

	// *** Basic block 19

	mv          a3, s5
	mv          a2, a0
	mv          a0, s2
	call        fprintf

	// *** Basic block 20

	addi        s5, s5, -8
	addi        a0, s0, -56
	call        BitSetIteratorNext

	// *** Basic block 21

	addi        a0, s0, -56
	call        BitSetIteratorDone

	// *** Basic block 22

	not         t0, a0
	bnez        t0, .RestoreRegisters_label_120

	// *** Basic block 23

	j           .RestoreRegisters_label_168

	// *** Basic block 24

.RestoreRegisters_label_168:
	addi        a0, s0, -56
	ld          t0, 8(s1)
	addi        a1, t0, 1544
	call        BitSetIteratorStart

	// *** Basic block 25

	addi        a0, s0, -56
	call        BitSetIteratorDone

	// *** Basic block 26

	not         t0, a0
	beqz        t0, .RestoreRegisters_label_224

	// *** Basic block 27

.RestoreRegisters_label_181:
	addi        s3, s0, -56
	ld          t0, 8(s3)
	slli        t0, t0, 5
	ld          t1, 16(s3)
	add         s3, t0, t1

	// *** Basic block 28

.RestoreRegisters_label_191:
	sext.w      s7, s3
	lla         a1, .str.43
	li          t0, 8		// 0x8 ASCII \x8
	mv          a3, t0
	mv          a2, s4
	mv          a1, x0
	mv          a0, s7
	call        RVRegisterNameFromNum

	// *** Basic block 29

	mv          a3, s5
	mv          a2, a0
	mv          a0, s2
	call        fprintf

	// *** Basic block 30

	addi        s5, s5, -8
	addi        a0, s0, -56
	call        BitSetIteratorNext

	// *** Basic block 31

	addi        a0, s0, -56
	call        BitSetIteratorDone

	// *** Basic block 32

	not         t0, a0
	bnez        t0, .RestoreRegisters_label_181

	// *** Basic block 33

	j           .RestoreRegisters_label_224

	// *** Basic block 34

.RestoreRegisters_label_224:
	mv          a0, s1
	call        EmptyStackFrame

	// *** Basic block 35

	beqz        a0, .RestoreRegisters_label_230

	// *** Basic block 36

	j           .RestoreRegisters_label_262

	// *** Basic block 37

.RestoreRegisters_label_230:
	lla         a1, .str.44
	mv          a2, s9
	mv          a0, s2
	call        fprintf

	// *** Basic block 38

	not         t0, s6
	beqz        t0, .RestoreRegisters_label_254

	// *** Basic block 39

	lla         a1, .str.45
	mv          a0, s2
	call        fprintf

	// *** Basic block 40

	lla         a1, .str.46
	mv          a0, s2
	call        fprintf

	// *** Basic block 41

	j           .RestoreRegisters_label_261

	// *** Basic block 42

.RestoreRegisters_label_254:
	lla         a1, .str.47
	mv          a0, s2
	call        fprintf

	// *** Basic block 43

.RestoreRegisters_label_261:

	// *** Basic block 44

.RestoreRegisters_label_262:
	// Restored registers.
	ld s1, 72(sp)
	ld s2, 64(sp)
	ld s3, 56(sp)
	ld s4, 48(sp)
	ld s5, 40(sp)
	ld s6, 32(sp)
	ld s7, 24(sp)
	ld s8, 16(sp)
	ld s9, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_RestoreRegisters:
	.size RestoreRegisters, .func_end_RestoreRegisters-RestoreRegisters

	.local  PrintRmov
	.type PrintRmov, @function

PrintRmov:

	// *** Basic block 0

	.global printf
	.global abort
	.global fprintf
	.global RVRegisterName
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a2
	addi        s3, s1, 40
	ld          s4, 40(s1)
	beq         s4, x0, .PrintRmov_label_54

	// *** Basic block 1

	j           .PrintRmov_label_71

	// *** Basic block 2

.PrintRmov_label_54:
	lla         a0, .str.48
	lla         a1, .str.49
	lla         a3, .str.50
	li          t0, 522		// 0x20a
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.PrintRmov_label_71:
	ld          s3, 8(s3)
	beq         s3, x0, .PrintRmov_label_78

	// *** Basic block 5

	j           .PrintRmov_label_93

	// *** Basic block 6

.PrintRmov_label_78:
	lla         a0, .str.51
	lla         a1, .str.52
	lla         a3, .str.53
	li          t0, 523		// 0x20b
	mv          a2, t0
	call        printf

	// *** Basic block 7

	call        abort

	// *** Basic block 8

.PrintRmov_label_93:
	ld          t0, 96(s3)
	bne         t0, x0, .PrintRmov_label_102

	// *** Basic block 9

.PrintRmov_label_99:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 10

.PrintRmov_label_102:
	ld          a0, 32(s4)
	beq         a0, x0, .PrintRmov_label_109

	// *** Basic block 11

	j           .PrintRmov_label_124

	// *** Basic block 12

.PrintRmov_label_109:
	lla         a0, .str.54
	lla         a1, .str.55
	lla         a3, .str.56
	li          t0, 528		// 0x210
	mv          a2, t0
	call        printf

	// *** Basic block 13

	call        abort

	// *** Basic block 14

.PrintRmov_label_124:
	ld          a0, 32(s3)
	beq         a0, x0, .PrintRmov_label_131

	// *** Basic block 15

	j           .PrintRmov_label_146

	// *** Basic block 16

.PrintRmov_label_131:
	lla         a0, .str.57
	lla         a1, .str.58
	lla         a3, .str.59
	li          t0, 529		// 0x211
	mv          a2, t0
	call        printf

	// *** Basic block 17

	call        abort

	// *** Basic block 18

.PrintRmov_label_146:
	bne         a0, a0, .PrintRmov_label_151

	// *** Basic block 19

	j           .PrintRmov_label_99

	// *** Basic block 20

.PrintRmov_label_151:
	lla         s3, .str.60
	lw          s4, 16(s1)
	li          t0, 18		// 0x12 ASCII \x12
	blt         s4, t0, .PrintRmov_label_187

	// *** Basic block 21

	li          t0, 20		// 0x14 ASCII \x14
	blt         t0, s4, .PrintRmov_label_187

	// *** Basic block 22

	addi        t0, s4, -18
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 23

	j           .PrintRmov_label_175

	// *** Basic block 24

	j           .PrintRmov_label_179

	// *** Basic block 25

	j           .PrintRmov_label_183

	// *** Basic block 26

.PrintRmov_label_175:
	lla         s3, .str.61
	j           .PrintRmov_label_202

	// *** Basic block 27

.PrintRmov_label_179:
	lla         s3, .str.62
	j           .PrintRmov_label_202

	// *** Basic block 28

.PrintRmov_label_183:
	lla         s3, .str.63
	j           .PrintRmov_label_202

	// *** Basic block 29

.PrintRmov_label_187:
	lla         a0, .str.64
	lla         a1, .str.65
	lla         a3, .str.66
	li          t0, 548		// 0x224
	mv          a2, t0
	call        printf

	// *** Basic block 30

	call        abort

	// *** Basic block 31

.PrintRmov_label_202:
	lla         s1, .str.67
	addi        a1, s0, -32
	li          s4, 8		// 0x8 ASCII \x8
	mv          a2, s4
	call        RVRegisterName

	// *** Basic block 32

	addi        a1, s0, -24
	mv          a2, s4
	call        RVRegisterName

	// *** Basic block 33

	mv          a4, a0
	mv          a3, a0
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        fprintf

	// *** Basic block 34

	j           .PrintRmov_label_99
.func_end_PrintRmov:
	.size PrintRmov, .func_end_PrintRmov-PrintRmov

	.local  GetRegisterName
	.type GetRegisterName, @function

GetRegisterName:

	// *** Basic block 0

	.global RVRegisterName
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, a2
	ld          t3, 24(t0)
	beq         t3, x0, .GetRegisterName_label_35

	// *** Basic block 1

	ld          a0, 32(t3)
	mv          a2, t2
	mv          a1, t1
	j           RVRegisterName

	// *** Basic block 4

.GetRegisterName_label_35:
	ld          a0, 32(t0)
	mv          a2, t2
	mv          a1, t1
	j           RVRegisterName
.func_end_GetRegisterName:
	.size GetRegisterName, .func_end_GetRegisterName-GetRegisterName

	.local  PrintInstruction
	.type PrintInstruction, @function

PrintInstruction:

	// *** Basic block 0

	.global fprintf
	.local IsPrintable
	.local PrintRmov
	.global StorageIs
	.global printf
	.global abort
	.local GetRegisterName
	.local SaveRegisters
	.local RestoreRegisters
	.global CompilerFindStringLiteral
	.global SourceLocationNumbers
	.global TargetIntValue
	.global RVIsPossibleImmediate
	.global RVRegisterNameFromNum
	.global RVRegisterName
	.global RVOpcodeName
	.global TargetIsConst
	addi sp, sp, -192
	// Saved return address (offset 184) and frame pointer (offset 176)
	sd ra, 184(sp)
	sd s0, 176(sp)
	addi s0, sp, 192
	// Local vars at offset -48(s0)
	// Spilled register region: 48 bytes at -96(s0) to -48(s0)
	// Saved integer registers.
	sd s1, 88(sp)
	sd s2, 80(sp)
	sd s3, 72(sp)
	sd s4, 64(sp)
	sd s5, 56(sp)
	sd s6, 48(sp)
	sd s7, 40(sp)
	sd s8, 32(sp)
	sd s9, 24(sp)
	sd s10, 16(sp)
	sd s11, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	mv          s3, a3
	sd          s3, -56(s0)	// Spilled @258
	mv          s4, a2
	ld          s5, 96(s1)
	bne         s5, x0, .PrintInstruction_label_282

	// *** Basic block 1

	addi        t0, s1, 40
	lw          t1, 104(s1)
	li          t2, 4096		// 0x1000
	and         t1, t1, t2
	addi        t2, s0, -48

	// *** Basic block 2

.PrintInstruction_label_279:
	// Restored registers.
	ld s1, 88(sp)
	ld s2, 80(sp)
	ld s3, 72(sp)
	ld s4, 64(sp)
	ld s5, 56(sp)
	ld s6, 48(sp)
	ld s7, 40(sp)
	ld s8, 32(sp)
	ld s9, 24(sp)
	ld s10, 16(sp)
	ld s11, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.PrintInstruction_label_282:
	ld          t0, 32(s2)
	beq         s5, t0, .PrintInstruction_label_299

	// *** Basic block 4

	lla         a1, .str.68
	ld          a2, 0(s5)
	mv          a0, s3
	call        fprintf

	// *** Basic block 5

	sd          s5, 32(s2)

	// *** Basic block 6

.PrintInstruction_label_299:
	lw          s5, 16(s1)
	li          s6, 22		// 0x16 ASCII \x16
	sd          s6, -56(s0)	// Spilled @305
	bne         s5, s6, .PrintInstruction_label_337

	// *** Basic block 7

	lw          t0, 104(s1)
	li          t1, 65536		// 0x10000
	and         t0, t0, t1
	beqz        t0, .PrintInstruction_label_324

	// *** Basic block 8

	lla         a1, .str.69
	lw          a3, 20(s1)
	mv          a2, s4
	mv          a0, s3
	call        fprintf

	// *** Basic block 9

.PrintInstruction_label_324:
	lla         a1, .str.70
	lw          a3, 20(s1)
	mv          a2, s4
	mv          a0, s3
	call        fprintf

	// *** Basic block 10

	j           .PrintInstruction_label_279

	// *** Basic block 11

.PrintInstruction_label_337:
	li          s7, 32		// 0x20 ASCII ' '
	sd          s7, -64(s0)	// Spilled @341
	bne         s5, s7, .PrintInstruction_label_355

	// *** Basic block 12

	mv          s8, s1
	sd          s8, -88(s0)	// Spilled @343
	lla         a1, .str.71
	ld          a2, 112(s8)
	mv          a0, s3
	call        fprintf

	// *** Basic block 13

	j           .PrintInstruction_label_279

	// *** Basic block 14

.PrintInstruction_label_355:
	addi        t1, s5, -33
	seqz        t0, t1
	li          t1, 33		// 0x21 ASCII '!'
	beq         s5, t1, .PrintInstruction_label_365

	// *** Basic block 15

	addi        t1, s5, -34
	seqz        t0, t1

	// *** Basic block 16

.PrintInstruction_label_365:
	beqz        t0, .PrintInstruction_label_368

	// *** Basic block 17

	j           .PrintInstruction_label_279

	// *** Basic block 18

.PrintInstruction_label_368:
	mv          a0, s1
	call        IsPrintable

	// *** Basic block 19

	not         t0, a0
	beqz        t0, .PrintInstruction_label_375

	// *** Basic block 20

	j           .PrintInstruction_label_279

	// *** Basic block 21

.PrintInstruction_label_375:
	beqz        s5, .PrintInstruction_label_577

	// *** Basic block 22

	li          s9, 1		// 0x1 ASCII \x1
	sd          s9, -96(s0)	// Spilled @381
	beq         s5, s9, .PrintInstruction_label_584

	// *** Basic block 23

	li          s10, 2		// 0x2 ASCII \x2
	beq         s5, s10, .PrintInstruction_label_474

	// *** Basic block 24

	li          t0, 11		// 0xb ASCII \xb
	beq         s5, t0, .PrintInstruction_label_461

	// *** Basic block 25

	li          t0, 18		// 0x12 ASCII \x12
	beq         s5, t0, .PrintInstruction_label_450

	// *** Basic block 26

	li          t0, 19		// 0x13 ASCII \x13
	beq         s5, t0, .PrintInstruction_label_451

	// *** Basic block 27

	li          t0, 20		// 0x14 ASCII \x14
	beq         s5, t0, .PrintInstruction_label_452

	// *** Basic block 28

	li          t0, 30		// 0x1e ASCII \x1e
	beq         s5, t0, .PrintInstruction_label_591

	// *** Basic block 29

	li          t0, 31		// 0x1f ASCII \x1f
	beq         s5, t0, .PrintInstruction_label_635

	// *** Basic block 30

	li          t0, 183		// 0xb7 ASCII \xb7
	beq         s5, t0, .PrintInstruction_label_509

	// *** Basic block 31

	li          t0, 184		// 0xb8 ASCII \xb8
	beq         s5, t0, .PrintInstruction_label_552

	// *** Basic block 32

	li          t0, 185		// 0xb9 ASCII \xb9
	beq         s5, t0, .PrintInstruction_label_510

	// *** Basic block 33

	li          t0, 186		// 0xba ASCII \xba
	beq         s5, t0, .PrintInstruction_label_553

	// *** Basic block 34

	li          t0, 210		// 0xd2 ASCII \xd2
	beq         s5, t0, .PrintInstruction_label_666

	// *** Basic block 35

	li          t0, 211		// 0xd3 ASCII \xd3
	beq         s5, t0, .PrintInstruction_label_848

	// *** Basic block 36

.PrintInstruction_label_448:
	j           .PrintInstruction_label_1030

	// *** Basic block 37

.PrintInstruction_label_450:

	// *** Basic block 38

.PrintInstruction_label_451:

	// *** Basic block 39

.PrintInstruction_label_452:
	ld          a2, -56(s0)	// Spilled @258
	mv          a1, s1
	mv          a0, s2
	call        PrintRmov

	// *** Basic block 40

	j           .PrintInstruction_label_279

	// *** Basic block 41

.PrintInstruction_label_461:
	ld          t0, 40(s1)
	ld          t0, 32(t0)
	ld          t1, 32(s1)
	bne         t0, t1, .PrintInstruction_label_472

	// *** Basic block 42

	j           .PrintInstruction_label_279

	// *** Basic block 43

.PrintInstruction_label_472:
	j           .PrintInstruction_label_1030

	// *** Basic block 44

.PrintInstruction_label_474:
	mv          s3, s1
	ld          s6, 112(s3)
	lw          a0, 48(s6)
	mv          a1, s10
	call        StorageIs

	// *** Basic block 45

	beqz        a0, .PrintInstruction_label_497

	// *** Basic block 46

	lla         a1, .str.72
	ld          a2, 16(s6)
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 47

	j           .PrintInstruction_label_507

	// *** Basic block 48

.PrintInstruction_label_497:
	lla         a1, .str.73
	ld          a2, 16(s6)
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 49

.PrintInstruction_label_507:
	j           .PrintInstruction_label_279

	// *** Basic block 50

.PrintInstruction_label_509:

	// *** Basic block 51

.PrintInstruction_label_510:
	ld          s3, 40(s1)
	lw          t0, 16(s3)
	bne         t0, s10, .PrintInstruction_label_520

	// *** Basic block 52

	j           .PrintInstruction_label_535

	// *** Basic block 53

.PrintInstruction_label_520:
	lla         a0, .str.74
	lla         a1, .str.75
	lla         a3, .str.76
	li          t0, 626		// 0x272
	mv          a2, t0
	call        printf

	// *** Basic block 54

	call        abort

	// *** Basic block 55

.PrintInstruction_label_535:
	lla         a1, .str.77
	lla         a2, .str.78
	ld          t0, 112(s3)
	ld          a3, 16(t0)
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 56

	j           .PrintInstruction_label_279

	// *** Basic block 57

.PrintInstruction_label_552:

	// *** Basic block 58

.PrintInstruction_label_553:
	lla         s3, .str.79
	lla         s7, .str.80
	ld          a0, 40(s1)
	addi        a1, s0, -48
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	call        GetRegisterName

	// *** Basic block 59

	mv          a3, a0
	mv          a2, s7
	mv          a1, s3
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 60

	j           .PrintInstruction_label_279

	// *** Basic block 61

.PrintInstruction_label_577:
	ld          a1, -56(s0)	// Spilled @258
	mv          a0, s2
	call        SaveRegisters

	// *** Basic block 62

	j           .PrintInstruction_label_279

	// *** Basic block 63

.PrintInstruction_label_584:
	ld          a1, -56(s0)	// Spilled @258
	mv          a0, s2
	call        RestoreRegisters

	// *** Basic block 64

	j           .PrintInstruction_label_279

	// *** Basic block 65

.PrintInstruction_label_591:
	ld          s7, 40(s1)
	lw          a0, 112(s7)
	call        CompilerFindStringLiteral

	// *** Basic block 66

	mv          s7, a0
	sd          s7, -56(s0)	// Spilled @599
	beq         s7, x0, .PrintInstruction_label_605

	// *** Basic block 67

	j           .PrintInstruction_label_620

	// *** Basic block 68

.PrintInstruction_label_605:
	lla         a0, .str.81
	lla         a1, .str.82
	lla         a3, .str.83
	li          t0, 650		// 0x28a
	mv          a2, t0
	call        printf

	// *** Basic block 69

	call        abort

	// *** Basic block 70

.PrintInstruction_label_620:
	lla         a1, .str.84
	addi        t0, s7, 8
	ld          a2, 16(t0)
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 71

	ld          t0, -96(s0)	// Spilled @381
	sb          t0, 48(s7)
	j           .PrintInstruction_label_279

	// *** Basic block 72

.PrintInstruction_label_635:
	mv          s3, s1
	sd          s3, -56(s0)	// Spilled @636
	ld          a0, 112(s3)
	addi        a1, s0, -40
	addi        a2, s0, -36
	addi        a3, s0, -32
	call        SourceLocationNumbers

	// *** Basic block 73

	lla         a1, .str.85
	lw          t0, -40(s0)
	addi        a2, t0, 1
	lw          a3, -36(s0)
	lw          t0, -32(s0)
	addi        a4, t0, 1
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 74

	j           .PrintInstruction_label_279

	// *** Basic block 75

.PrintInstruction_label_666:
	ld          s3, 32(s1)
	addi        t0, s1, 40
	ld          a0, 8(t0)
	call        TargetIntValue

	// *** Basic block 76

	sext.w      t0, a0
	lw          t1, 24(s2)
	add         s7, t0, t1
	mv          a0, s7
	call        RVIsPossibleImmediate

	// *** Basic block 77

	not         t0, a0
	beqz        t0, .PrintInstruction_label_808

	// *** Basic block 78

	lla         s8, .str.86
	sd          s8, -56(s0)	// Spilled @688
	lla         s9, .str.87
	sd          s9, -64(s0)	// Spilled @690
	addi        s11, s0, -28
	li          s8, 8		// 0x8 ASCII \x8
	mv          a3, s8
	mv          a2, s11
	mv          a1, x0
	li          s9, 3		// 0x3 ASCII \x3
	mv          a0, s9
	call        RVRegisterNameFromNum
	sd          a0, -72(s0)	// Spilled @703

	// *** Basic block 79

	srai        a4, s7, 12
	mv          a3, a0
	ld          a2, -64(s0)	// Spilled @690
	ld          a1, -56(s0)	// Spilled @688
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 80

	lla         a0, .str.88
	sd          a0, -80(s0)	// Spilled @716
	lla         a0, .str.89
	mv          a3, s8
	mv          a2, s11
	mv          a1, x0
	mv          a0, s9
	call        RVRegisterNameFromNum
	sd          a0, -80(s0)	// Spilled @729

	// *** Basic block 81

	addi        a0, s0, -48
	mv          a3, s8
	mv          a2, a0
	mv          a1, x0
	mv          a0, s9
	call        RVRegisterNameFromNum
	sd          a0, -80(s0)	// Spilled @741

	// *** Basic block 82

	mv          a4, a0
	ld          a3, -80(s0)	// Spilled @729
	mv          a2, a0
	ld          a1, -80(s0)	// Spilled @716
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 83

	lla         a0, .str.90
	sd          a0, -88(s0)	// Spilled @754
	lw          t0, 16(s3)
	bnez        t0, .PrintInstruction_label_763

	// *** Basic block 84

	lla         a0, .str.91
	j           .PrintInstruction_label_766

	// *** Basic block 85

.PrintInstruction_label_763:
	lla         a0, .str.92

	// *** Basic block 86

.PrintInstruction_label_766:
	mv          a2, s8
	mv          a1, s11
	mv          a0, s3
	call        RVRegisterName

	// *** Basic block 87

	li          t0, 4095		// 0xfff
	and         s11, s7, t0
	mv          a3, s8
	mv          a2, a0
	mv          a1, x0
	mv          a0, s9
	call        RVRegisterNameFromNum

	// *** Basic block 88

	ld          t0, 40(s1)
	lw          a6, 20(t0)
	mv          a5, a0
	mv          a4, s11
	mv          a3, a0
	mv          a2, a0
	ld          a1, -88(s0)	// Spilled @754
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 89

	j           .PrintInstruction_label_846

	// *** Basic block 90

.PrintInstruction_label_808:
	lla         s8, .str.93
	lw          t0, 16(s3)
	bnez        t0, .PrintInstruction_label_819

	// *** Basic block 91

	lla         s9, .str.94
	j           .PrintInstruction_label_822

	// *** Basic block 92

.PrintInstruction_label_819:
	lla         s9, .str.95

	// *** Basic block 93

.PrintInstruction_label_822:
	addi        a1, s0, -28
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	mv          a0, s3
	call        RVRegisterName

	// *** Basic block 94

	ld          t0, 40(s1)
	lw          a5, 20(t0)
	mv          a4, s7
	mv          a3, a0
	mv          a2, s9
	mv          a1, s8
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 95

.PrintInstruction_label_846:
	j           .PrintInstruction_label_279

	// *** Basic block 96

.PrintInstruction_label_848:
	ld          s11, 32(s1)
	ld          s3, 40(s1)
	addi        t0, s3, 40
	ld          a0, 8(t0)
	call        TargetIntValue

	// *** Basic block 97

	sext.w      t0, a0
	sd          t0, -56(s0)	// Spilled @860
	lw          t1, 24(s2)
	add         t0, t0, t1
	mv          a0, t0
	call        RVIsPossibleImmediate

	// *** Basic block 98

	not         t1, a0
	beqz        t1, .PrintInstruction_label_990

	// *** Basic block 99

	lla         s7, .str.96
	sd          s7, -72(s0)	// Spilled @873
	lla         s7, .str.97
	sd          s7, -80(s0)	// Spilled @875
	addi        s7, s0, -28
	li          s8, 8		// 0x8 ASCII \x8
	mv          a3, s8
	mv          a2, s7
	mv          a1, x0
	li          s9, 3		// 0x3 ASCII \x3
	mv          a0, s9
	call        RVRegisterNameFromNum
	sd          a0, -80(s0)	// Spilled @887

	// *** Basic block 100

	srai        a4, t0, 12
	mv          a3, a0
	ld          a2, -80(s0)	// Spilled @875
	ld          a1, -72(s0)	// Spilled @873
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 101

	lla         a0, .str.98
	sd          a0, -88(s0)	// Spilled @900
	lla         a0, .str.99
	mv          a3, s8
	mv          a2, s7
	mv          a1, x0
	mv          a0, s9
	call        RVRegisterNameFromNum
	sd          a0, -88(s0)	// Spilled @913

	// *** Basic block 102

	addi        a0, s0, -48
	mv          a3, s8
	mv          a2, a0
	mv          a1, x0
	mv          a0, s9
	call        RVRegisterNameFromNum
	sd          a0, -88(s0)	// Spilled @925

	// *** Basic block 103

	mv          a4, a0
	ld          a3, -88(s0)	// Spilled @913
	mv          a2, a0
	ld          a1, -88(s0)	// Spilled @900
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 104

	lla         a0, .str.100
	sd          a0, -96(s0)	// Spilled @938
	lw          t1, 16(s11)
	bnez        t1, .PrintInstruction_label_947

	// *** Basic block 105

	lla         a0, .str.101
	j           .PrintInstruction_label_950

	// *** Basic block 106

.PrintInstruction_label_947:
	lla         a0, .str.102

	// *** Basic block 107

.PrintInstruction_label_950:
	mv          a2, s8
	mv          a1, s7
	mv          a0, s11
	call        RVRegisterName

	// *** Basic block 108

	li          t1, 4095		// 0xfff
	and         s7, t0, t1
	mv          a3, s8
	mv          a2, a0
	mv          a1, x0
	mv          a0, s9
	call        RVRegisterNameFromNum

	// *** Basic block 109

	ld          t1, 40(s3)
	lw          a6, 20(t1)
	mv          a5, a0
	mv          a4, s7
	mv          a3, a0
	mv          a2, a0
	ld          a1, -96(s0)	// Spilled @938
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 110

	j           .PrintInstruction_label_1028

	// *** Basic block 111

.PrintInstruction_label_990:
	lla         s7, .str.103
	lw          t1, 16(s11)
	bnez        t1, .PrintInstruction_label_1001

	// *** Basic block 112

	lla         s8, .str.104
	j           .PrintInstruction_label_1004

	// *** Basic block 113

.PrintInstruction_label_1001:
	lla         s8, .str.105

	// *** Basic block 114

.PrintInstruction_label_1004:
	addi        a1, s0, -28
	li          t1, 8		// 0x8 ASCII \x8
	mv          a2, t1
	mv          a0, s11
	call        RVRegisterName

	// *** Basic block 115

	ld          t1, 40(s3)
	lw          a5, 20(t1)
	mv          a4, t0
	mv          a3, a0
	mv          a2, s8
	mv          a1, s7
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 116

.PrintInstruction_label_1028:
	j           .PrintInstruction_label_279

	// *** Basic block 117

.PrintInstruction_label_1030:
	lla         s8, .str.106
	mv          a0, s5
	call        RVOpcodeName

	// *** Basic block 118

	mv          a2, a0
	mv          a1, s8
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 119

	li          s8, 52		// 0x34 ASCII '4'
	blt         s5, s8, .PrintInstruction_label_1117

	// *** Basic block 120

	beq         s5, s8, .PrintInstruction_label_1525

	// *** Basic block 121

	li          t0, 82		// 0x52 ASCII 'R'
	beq         s5, t0, .PrintInstruction_label_1190

	// *** Basic block 122

	li          t0, 83		// 0x53 ASCII 'S'
	beq         s5, t0, .PrintInstruction_label_1194

	// *** Basic block 123

	li          t0, 84		// 0x54 ASCII 'T'
	beq         s5, t0, .PrintInstruction_label_1527

	// *** Basic block 124

	li          t0, 107		// 0x6b ASCII 'k'
	beq         s5, t0, .PrintInstruction_label_1193

	// *** Basic block 125

	li          t0, 108		// 0x6c ASCII 'l'
	beq         s5, t0, .PrintInstruction_label_1529

	// *** Basic block 126

	li          t0, 137		// 0x89 ASCII \x89
	beq         s5, t0, .PrintInstruction_label_1195

	// *** Basic block 127

	li          t0, 138		// 0x8a ASCII \x8a
	beq         s5, t0, .PrintInstruction_label_1530

	// *** Basic block 128

	li          t0, 169		// 0xa9 ASCII \xa9
	beq         s5, t0, .PrintInstruction_label_1881

	// *** Basic block 129

	li          t0, 174		// 0xae ASCII \xae
	beq         s5, t0, .PrintInstruction_label_2326

	// *** Basic block 130

	li          t0, 179		// 0xb3 ASCII \xb3
	beq         s5, t0, .PrintInstruction_label_2038

	// *** Basic block 131

	li          t0, 180		// 0xb4 ASCII \xb4
	beq         s5, t0, .PrintInstruction_label_2039

	// *** Basic block 132

	li          t0, 181		// 0xb5 ASCII \xb5
	beq         s5, t0, .PrintInstruction_label_2129

	// *** Basic block 133

	li          t0, 182		// 0xb6 ASCII \xb6
	beq         s5, t0, .PrintInstruction_label_2206

	// *** Basic block 134

	j           .PrintInstruction_label_2397

	// *** Basic block 135

.PrintInstruction_label_1117:
	li          t0, 38		// 0x26 ASCII '&'
	beq         s5, t0, .PrintInstruction_label_2226

	// *** Basic block 136

	li          t0, 39		// 0x27 ASCII '''
	beq         s5, t0, .PrintInstruction_label_1889

	// *** Basic block 137

	li          t0, 40		// 0x28 ASCII '('
	beq         s5, t0, .PrintInstruction_label_1890

	// *** Basic block 138

	li          t0, 41		// 0x29 ASCII ')'
	beq         s5, t0, .PrintInstruction_label_1891

	// *** Basic block 139

	li          t0, 42		// 0x2a ASCII '*'
	beq         s5, t0, .PrintInstruction_label_1892

	// *** Basic block 140

	li          t0, 43		// 0x2b ASCII '+'
	beq         s5, t0, .PrintInstruction_label_1893

	// *** Basic block 141

	li          t0, 44		// 0x2c ASCII ','
	beq         s5, t0, .PrintInstruction_label_1894

	// *** Basic block 142

	li          t0, 45		// 0x2d ASCII '-'
	beq         s5, t0, .PrintInstruction_label_1189

	// *** Basic block 143

	li          t0, 46		// 0x2e ASCII '.'
	beq         s5, t0, .PrintInstruction_label_1188

	// *** Basic block 144

	li          t0, 47		// 0x2f ASCII '/'
	beq         s5, t0, .PrintInstruction_label_1187

	// *** Basic block 145

	li          t0, 48		// 0x30 ASCII '0'
	beq         s5, t0, .PrintInstruction_label_1191

	// *** Basic block 146

	li          t0, 49		// 0x31 ASCII '1'
	beq         s5, t0, .PrintInstruction_label_1192

	// *** Basic block 147

	li          t0, 50		// 0x32 ASCII '2'
	beq         s5, t0, .PrintInstruction_label_1528

	// *** Basic block 148

	li          t0, 51		// 0x33 ASCII '3'
	beq         s5, t0, .PrintInstruction_label_1526

	// *** Basic block 149

	j           .PrintInstruction_label_2397

	// *** Basic block 150

.PrintInstruction_label_1187:

	// *** Basic block 151

.PrintInstruction_label_1188:

	// *** Basic block 152

.PrintInstruction_label_1189:

	// *** Basic block 153

.PrintInstruction_label_1190:

	// *** Basic block 154

.PrintInstruction_label_1191:

	// *** Basic block 155

.PrintInstruction_label_1192:

	// *** Basic block 156

.PrintInstruction_label_1193:

	// *** Basic block 157

.PrintInstruction_label_1194:

	// *** Basic block 158

.PrintInstruction_label_1195:
	addi        s3, s1, 40
	ld          s5, 40(s1)
	beq         s5, x0, .PrintInstruction_label_1202

	// *** Basic block 159

	j           .PrintInstruction_label_1217

	// *** Basic block 160

.PrintInstruction_label_1202:
	lla         a0, .str.107
	lla         a1, .str.108
	lla         a3, .str.109
	li          t0, 744		// 0x2e8
	mv          a2, t0
	call        printf

	// *** Basic block 161

	call        abort

	// *** Basic block 162

.PrintInstruction_label_1217:
	ld          s3, 8(s3)
	beq         s3, x0, .PrintInstruction_label_1224

	// *** Basic block 163

	j           .PrintInstruction_label_1239

	// *** Basic block 164

.PrintInstruction_label_1224:
	lla         a0, .str.110
	lla         a1, .str.111
	lla         a3, .str.112
	li          t0, 745		// 0x2e9
	mv          a2, t0
	call        printf

	// *** Basic block 165

	call        abort

	// *** Basic block 166

.PrintInstruction_label_1239:
	ld          t0, 32(s1)
	beq         t0, x0, .PrintInstruction_label_1246

	// *** Basic block 167

	j           .PrintInstruction_label_1261

	// *** Basic block 168

.PrintInstruction_label_1246:
	lla         a0, .str.113
	lla         a1, .str.114
	lla         a3, .str.115
	li          t0, 746		// 0x2ea
	mv          a2, t0
	call        printf

	// *** Basic block 169

	call        abort

	// *** Basic block 170

.PrintInstruction_label_1261:
	ld          t0, 32(s5)
	beq         t0, x0, .PrintInstruction_label_1268

	// *** Basic block 171

	j           .PrintInstruction_label_1283

	// *** Basic block 172

.PrintInstruction_label_1268:
	lla         a0, .str.116
	lla         a1, .str.117
	lla         a3, .str.118
	li          t0, 747		// 0x2eb
	mv          a2, t0
	call        printf

	// *** Basic block 173

	call        abort

	// *** Basic block 174

.PrintInstruction_label_1283:
	mv          a0, s3
	call        TargetIsConst

	// *** Basic block 175

	beqz        a0, .PrintInstruction_label_1345

	// *** Basic block 176

	mv          a0, s3
	call        TargetIntValue

	// *** Basic block 177

	sext.w      s9, a0
	mv          a0, s9
	call        RVIsPossibleImmediate

	// *** Basic block 178

	beqz        a0, .PrintInstruction_label_1299

	// *** Basic block 179

	j           .PrintInstruction_label_1314

	// *** Basic block 180

.PrintInstruction_label_1299:
	lla         a0, .str.119
	lla         a1, .str.120
	lla         a3, .str.121
	li          t0, 750		// 0x2ee
	mv          a2, t0
	call        printf

	// *** Basic block 181

	call        abort

	// *** Basic block 182

.PrintInstruction_label_1314:
	lla         s11, .str.122
	addi        a1, s0, -28
	li          s7, 8		// 0x8 ASCII \x8
	mv          a2, s7
	mv          a0, s1
	call        GetRegisterName

	// *** Basic block 183

	addi        a1, s0, -48
	mv          a2, s7
	mv          a0, s5
	call        GetRegisterName

	// *** Basic block 184

	mv          a4, a0
	mv          a3, s9
	mv          a2, a0
	mv          a1, s11
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 185

	j           .PrintInstruction_label_1523

	// *** Basic block 186

.PrintInstruction_label_1345:
	lw          s7, 16(s3)
	bne         s7, s10, .PrintInstruction_label_1409

	// *** Basic block 187

	lw          t0, 104(s1)
	li          t1, 8192		// 0x2000
	and         t0, t0, t1
	beqz        t0, .PrintInstruction_label_1359

	// *** Basic block 188

	j           .PrintInstruction_label_1374

	// *** Basic block 189

.PrintInstruction_label_1359:
	lla         a0, .str.123
	lla         a1, .str.124
	lla         a3, .str.125
	li          t0, 758		// 0x2f6
	mv          a2, t0
	call        printf

	// *** Basic block 190

	call        abort

	// *** Basic block 191

.PrintInstruction_label_1374:
	lla         s9, .str.126
	addi        a1, s0, -28
	li          s11, 8		// 0x8 ASCII \x8
	mv          a2, s11
	mv          a0, s1
	call        GetRegisterName
	sd          a0, -56(s0)	// Spilled @1384

	// *** Basic block 192

	ld          t0, 112(s3)
	ld          a0, 16(t0)
	addi        a1, s0, -48
	mv          a2, s11
	mv          a0, s5
	call        GetRegisterName

	// *** Basic block 193

	mv          a4, a0
	mv          a3, a0
	ld          a2, -56(s0)	// Spilled @1384
	mv          a1, s9
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 194

	j           .PrintInstruction_label_1522

	// *** Basic block 195

.PrintInstruction_label_1409:
	bne         s7, s6, .PrintInstruction_label_1471

	// *** Basic block 196

	lw          t0, 104(s1)
	li          t1, 32768		// 0x8000
	and         t0, t0, t1
	beqz        t0, .PrintInstruction_label_1421

	// *** Basic block 197

	j           .PrintInstruction_label_1436

	// *** Basic block 198

.PrintInstruction_label_1421:
	lla         a0, .str.127
	lla         a1, .str.128
	lla         a3, .str.129
	li          t0, 765		// 0x2fd
	mv          a2, t0
	call        printf

	// *** Basic block 199

	call        abort

	// *** Basic block 200

.PrintInstruction_label_1436:
	lla         s9, .str.130
	addi        a1, s0, -28
	li          s11, 8		// 0x8 ASCII \x8
	mv          a2, s11
	mv          a0, s1
	call        GetRegisterName

	// *** Basic block 201

	lw          s3, 20(s3)
	addi        a1, s0, -48
	mv          a2, s11
	mv          a0, s5
	call        GetRegisterName

	// *** Basic block 202

	mv          a5, a0
	mv          a4, s3
	mv          a3, s4
	mv          a2, a0
	mv          a1, s9
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 203

	j           .PrintInstruction_label_1521

	// *** Basic block 204

.PrintInstruction_label_1471:
	li          t0, 207		// 0xcf ASCII \xcf
	bne         s7, t0, .PrintInstruction_label_1505

	// *** Basic block 205

	lla         s3, .str.131
	addi        a1, s0, -28
	li          s7, 8		// 0x8 ASCII \x8
	mv          a2, s7
	mv          a0, s1
	call        GetRegisterName

	// *** Basic block 206

	addi        a1, s0, -48
	mv          a2, s7
	mv          a0, s5
	call        GetRegisterName

	// *** Basic block 207

	mv          a3, a0
	mv          a2, a0
	mv          a1, s3
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 208

	j           .PrintInstruction_label_1520

	// *** Basic block 209

.PrintInstruction_label_1505:
	lla         a0, .str.132
	lla         a1, .str.133
	lla         a3, .str.134
	li          t0, 777		// 0x309
	mv          a2, t0
	call        printf

	// *** Basic block 210

	call        abort

	// *** Basic block 211

.PrintInstruction_label_1520:

	// *** Basic block 212

.PrintInstruction_label_1521:

	// *** Basic block 213

.PrintInstruction_label_1522:

	// *** Basic block 214

.PrintInstruction_label_1523:
	j           .PrintInstruction_label_2550

	// *** Basic block 215

.PrintInstruction_label_1525:

	// *** Basic block 216

.PrintInstruction_label_1526:

	// *** Basic block 217

.PrintInstruction_label_1527:

	// *** Basic block 218

.PrintInstruction_label_1528:

	// *** Basic block 219

.PrintInstruction_label_1529:

	// *** Basic block 220

.PrintInstruction_label_1530:
	addi        s3, s1, 40
	ld          s5, 40(s1)
	beq         s5, x0, .PrintInstruction_label_1537

	// *** Basic block 221

	j           .PrintInstruction_label_1552

	// *** Basic block 222

.PrintInstruction_label_1537:
	lla         a0, .str.135
	lla         a1, .str.136
	lla         a3, .str.137
	li          t0, 787		// 0x313
	mv          a2, t0
	call        printf

	// *** Basic block 223

	call        abort

	// *** Basic block 224

.PrintInstruction_label_1552:
	ld          s7, 8(s3)
	beq         s7, x0, .PrintInstruction_label_1559

	// *** Basic block 225

	j           .PrintInstruction_label_1574

	// *** Basic block 226

.PrintInstruction_label_1559:
	lla         a0, .str.138
	lla         a1, .str.139
	lla         a3, .str.140
	li          t0, 788		// 0x314
	mv          a2, t0
	call        printf

	// *** Basic block 227

	call        abort

	// *** Basic block 228

.PrintInstruction_label_1574:
	ld          s3, 16(s3)
	beq         s3, x0, .PrintInstruction_label_1581

	// *** Basic block 229

	j           .PrintInstruction_label_1596

	// *** Basic block 230

.PrintInstruction_label_1581:
	lla         a0, .str.141
	lla         a1, .str.142
	lla         a3, .str.143
	li          t0, 789		// 0x315
	mv          a2, t0
	call        printf

	// *** Basic block 231

	call        abort

	// *** Basic block 232

.PrintInstruction_label_1596:
	ld          t0, 32(s5)
	beq         t0, x0, .PrintInstruction_label_1603

	// *** Basic block 233

	j           .PrintInstruction_label_1618

	// *** Basic block 234

.PrintInstruction_label_1603:
	lla         a0, .str.144
	lla         a1, .str.145
	lla         a3, .str.146
	li          t0, 790		// 0x316
	mv          a2, t0
	call        printf

	// *** Basic block 235

	call        abort

	// *** Basic block 236

.PrintInstruction_label_1618:
	ld          t0, 32(s7)
	beq         t0, x0, .PrintInstruction_label_1625

	// *** Basic block 237

	j           .PrintInstruction_label_1640

	// *** Basic block 238

.PrintInstruction_label_1625:
	lla         a0, .str.147
	lla         a1, .str.148
	lla         a3, .str.149
	li          t0, 791		// 0x317
	mv          a2, t0
	call        printf

	// *** Basic block 239

	call        abort

	// *** Basic block 240

.PrintInstruction_label_1640:
	mv          a0, s3
	call        TargetIsConst

	// *** Basic block 241

	beqz        a0, .PrintInstruction_label_1702

	// *** Basic block 242

	mv          a0, s3
	call        TargetIntValue

	// *** Basic block 243

	sext.w      s9, a0
	mv          a0, s9
	call        RVIsPossibleImmediate

	// *** Basic block 244

	beqz        a0, .PrintInstruction_label_1656

	// *** Basic block 245

	j           .PrintInstruction_label_1671

	// *** Basic block 246

.PrintInstruction_label_1656:
	lla         a0, .str.150
	lla         a1, .str.151
	lla         a3, .str.152
	li          t0, 794		// 0x31a
	mv          a2, t0
	call        printf

	// *** Basic block 247

	call        abort

	// *** Basic block 248

.PrintInstruction_label_1671:
	lla         s11, .str.153
	addi        a1, s0, -28
	li          s6, 8		// 0x8 ASCII \x8
	mv          a2, s6
	mv          a0, s5
	call        GetRegisterName

	// *** Basic block 249

	addi        a1, s0, -48
	mv          a2, s6
	mv          a0, s7
	call        GetRegisterName

	// *** Basic block 250

	mv          a4, a0
	mv          a3, s9
	mv          a2, a0
	mv          a1, s11
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 251

	j           .PrintInstruction_label_1879

	// *** Basic block 252

.PrintInstruction_label_1702:
	lw          s6, 16(s3)
	bne         s6, s10, .PrintInstruction_label_1766

	// *** Basic block 253

	lw          t0, 104(s1)
	li          t1, 8192		// 0x2000
	and         t0, t0, t1
	beqz        t0, .PrintInstruction_label_1716

	// *** Basic block 254

	j           .PrintInstruction_label_1731

	// *** Basic block 255

.PrintInstruction_label_1716:
	lla         a0, .str.154
	lla         a1, .str.155
	lla         a3, .str.156
	li          t0, 802		// 0x322
	mv          a2, t0
	call        printf

	// *** Basic block 256

	call        abort

	// *** Basic block 257

.PrintInstruction_label_1731:
	lla         s9, .str.157
	addi        a1, s0, -28
	li          s11, 8		// 0x8 ASCII \x8
	mv          a2, s11
	mv          a0, s5
	call        GetRegisterName
	sd          a0, -56(s0)	// Spilled @1741

	// *** Basic block 258

	ld          t0, 112(s3)
	ld          a0, 16(t0)
	addi        a1, s0, -48
	mv          a2, s11
	mv          a0, s7
	call        GetRegisterName

	// *** Basic block 259

	mv          a4, a0
	mv          a3, a0
	ld          a2, -56(s0)	// Spilled @1741
	mv          a1, s9
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 260

	j           .PrintInstruction_label_1878

	// *** Basic block 261

.PrintInstruction_label_1766:
	ld          t0, -56(s0)	// Spilled @305
	bne         s6, t0, .PrintInstruction_label_1828

	// *** Basic block 262

	lw          t0, 104(s1)
	li          t1, 32768		// 0x8000
	and         t0, t0, t1
	beqz        t0, .PrintInstruction_label_1778

	// *** Basic block 263

	j           .PrintInstruction_label_1793

	// *** Basic block 264

.PrintInstruction_label_1778:
	lla         a0, .str.158
	lla         a1, .str.159
	lla         a3, .str.160
	li          t0, 810		// 0x32a
	mv          a2, t0
	call        printf

	// *** Basic block 265

	call        abort

	// *** Basic block 266

.PrintInstruction_label_1793:
	lla         s9, .str.161
	addi        a1, s0, -28
	li          s11, 8		// 0x8 ASCII \x8
	mv          a2, s11
	mv          a0, s5
	call        GetRegisterName

	// *** Basic block 267

	lw          s3, 20(s3)
	addi        a1, s0, -48
	mv          a2, s11
	mv          a0, s7
	call        GetRegisterName

	// *** Basic block 268

	mv          a5, a0
	mv          a4, s3
	mv          a3, s4
	mv          a2, a0
	mv          a1, s9
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 269

	j           .PrintInstruction_label_1877

	// *** Basic block 270

.PrintInstruction_label_1828:
	li          t0, 207		// 0xcf ASCII \xcf
	bne         s6, t0, .PrintInstruction_label_1861

	// *** Basic block 271

	lla         s3, .str.162
	addi        a1, s0, -28
	li          s6, 8		// 0x8 ASCII \x8
	mv          a2, s6
	mv          a0, s5
	call        GetRegisterName

	// *** Basic block 272

	addi        a1, s0, -48
	mv          a2, s6
	mv          a0, s7
	call        GetRegisterName

	// *** Basic block 273

	mv          a3, a0
	mv          a2, a0
	mv          a1, s3
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 274

	j           .PrintInstruction_label_1876

	// *** Basic block 275

.PrintInstruction_label_1861:
	lla         a0, .str.163
	lla         a1, .str.164
	lla         a3, .str.165
	li          t0, 825		// 0x339
	mv          a2, t0
	call        printf

	// *** Basic block 276

	call        abort

	// *** Basic block 277

.PrintInstruction_label_1876:

	// *** Basic block 278

.PrintInstruction_label_1877:

	// *** Basic block 279

.PrintInstruction_label_1878:

	// *** Basic block 280

.PrintInstruction_label_1879:
	j           .PrintInstruction_label_2550

	// *** Basic block 281

.PrintInstruction_label_1881:
	lla         a1, .str.166
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 282

	j           .PrintInstruction_label_2550

	// *** Basic block 283

.PrintInstruction_label_1889:

	// *** Basic block 284

.PrintInstruction_label_1890:

	// *** Basic block 285

.PrintInstruction_label_1891:

	// *** Basic block 286

.PrintInstruction_label_1892:

	// *** Basic block 287

.PrintInstruction_label_1893:

	// *** Basic block 288

.PrintInstruction_label_1894:
	addi        s5, s1, 40
	ld          s9, 40(s1)
	beq         s9, x0, .PrintInstruction_label_1901

	// *** Basic block 289

	j           .PrintInstruction_label_1916

	// *** Basic block 290

.PrintInstruction_label_1901:
	lla         a0, .str.167
	lla         a1, .str.168
	lla         a3, .str.169
	li          t0, 839		// 0x347
	mv          a2, t0
	call        printf

	// *** Basic block 291

	call        abort

	// *** Basic block 292

.PrintInstruction_label_1916:
	ld          s11, 8(s5)
	beq         s11, x0, .PrintInstruction_label_1923

	// *** Basic block 293

	j           .PrintInstruction_label_1938

	// *** Basic block 294

.PrintInstruction_label_1923:
	lla         a0, .str.170
	lla         a1, .str.171
	lla         a3, .str.172
	li          t0, 840		// 0x348
	mv          a2, t0
	call        printf

	// *** Basic block 295

	call        abort

	// *** Basic block 296

.PrintInstruction_label_1938:
	ld          s5, 16(s5)
	beq         s5, x0, .PrintInstruction_label_1945

	// *** Basic block 297

	j           .PrintInstruction_label_1960

	// *** Basic block 298

.PrintInstruction_label_1945:
	lla         a0, .str.173
	lla         a1, .str.174
	lla         a3, .str.175
	li          t0, 841		// 0x349
	mv          a2, t0
	call        printf

	// *** Basic block 299

	call        abort

	// *** Basic block 300

.PrintInstruction_label_1960:
	ld          t0, 32(s9)
	beq         t0, x0, .PrintInstruction_label_1967

	// *** Basic block 301

	j           .PrintInstruction_label_1982

	// *** Basic block 302

.PrintInstruction_label_1967:
	lla         a0, .str.176
	lla         a1, .str.177
	lla         a3, .str.178
	li          t0, 842		// 0x34a
	mv          a2, t0
	call        printf

	// *** Basic block 303

	call        abort

	// *** Basic block 304

.PrintInstruction_label_1982:
	ld          t0, 32(s11)
	beq         t0, x0, .PrintInstruction_label_1989

	// *** Basic block 305

	j           .PrintInstruction_label_2004

	// *** Basic block 306

.PrintInstruction_label_1989:
	lla         a0, .str.179
	lla         a1, .str.180
	lla         a3, .str.181
	li          t0, 843		// 0x34b
	mv          a2, t0
	call        printf

	// *** Basic block 307

	call        abort

	// *** Basic block 308

.PrintInstruction_label_2004:
	lla         s3, .str.182
	sd          s3, -64(s0)	// Spilled @2006
	addi        a1, s0, -28
	li          s3, 8		// 0x8 ASCII \x8
	mv          a2, s3
	mv          a0, s9
	call        GetRegisterName

	// *** Basic block 309

	addi        a1, s0, -48
	mv          a2, s3
	mv          a0, s11
	call        GetRegisterName

	// *** Basic block 310

	lw          a5, 20(s5)
	mv          a4, s4
	mv          a3, a0
	mv          a2, a0
	ld          a1, -64(s0)	// Spilled @2006
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 311

	j           .PrintInstruction_label_2550

	// *** Basic block 312

.PrintInstruction_label_2038:

	// *** Basic block 313

.PrintInstruction_label_2039:
	addi        s8, s1, 40
	ld          s9, 40(s1)
	beq         s9, x0, .PrintInstruction_label_2046

	// *** Basic block 314

	j           .PrintInstruction_label_2061

	// *** Basic block 315

.PrintInstruction_label_2046:
	lla         a0, .str.183
	lla         a1, .str.184
	lla         a3, .str.185
	li          t0, 855		// 0x357
	mv          a2, t0
	call        printf

	// *** Basic block 316

	call        abort

	// *** Basic block 317

.PrintInstruction_label_2061:
	ld          s8, 8(s8)
	beq         s8, x0, .PrintInstruction_label_2068

	// *** Basic block 318

	j           .PrintInstruction_label_2083

	// *** Basic block 319

.PrintInstruction_label_2068:
	lla         a0, .str.186
	lla         a1, .str.187
	lla         a3, .str.188
	li          t0, 856		// 0x358
	mv          a2, t0
	call        printf

	// *** Basic block 320

	call        abort

	// *** Basic block 321

.PrintInstruction_label_2083:
	ld          t0, 32(s9)
	beq         t0, x0, .PrintInstruction_label_2090

	// *** Basic block 322

	j           .PrintInstruction_label_2105

	// *** Basic block 323

.PrintInstruction_label_2090:
	lla         a0, .str.189
	lla         a1, .str.190
	lla         a3, .str.191
	li          t0, 857		// 0x359
	mv          a2, t0
	call        printf

	// *** Basic block 324

	call        abort

	// *** Basic block 325

.PrintInstruction_label_2105:
	lla         s11, .str.192
	addi        a1, s0, -28
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	mv          a0, s9
	call        GetRegisterName

	// *** Basic block 326

	lw          a4, 20(s8)
	mv          a3, s4
	mv          a2, a0
	mv          a1, s11
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 327

	j           .PrintInstruction_label_2550

	// *** Basic block 328

.PrintInstruction_label_2129:
	ld          s8, 40(s1)
	beq         s8, x0, .PrintInstruction_label_2136

	// *** Basic block 329

	j           .PrintInstruction_label_2151

	// *** Basic block 330

.PrintInstruction_label_2136:
	lla         a0, .str.193
	lla         a1, .str.194
	lla         a3, .str.195
	li          t0, 865		// 0x361
	mv          a2, t0
	call        printf

	// *** Basic block 331

	call        abort

	// *** Basic block 332

.PrintInstruction_label_2151:
	lw          s9, 16(s8)
	bne         s9, s6, .PrintInstruction_label_2171

	// *** Basic block 333

	lla         a1, .str.196
	lw          a3, 20(s8)
	mv          a2, s4
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 334

	j           .PrintInstruction_label_2204

	// *** Basic block 335

.PrintInstruction_label_2171:
	bne         s9, s10, .PrintInstruction_label_2188

	// *** Basic block 336

	lla         a1, .str.197
	ld          t0, 112(s8)
	ld          a2, 16(t0)
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 337

	j           .PrintInstruction_label_2203

	// *** Basic block 338

.PrintInstruction_label_2188:
	lla         a0, .str.198
	lla         a1, .str.199
	lla         a3, .str.200
	li          t0, 872		// 0x368
	mv          a2, t0
	call        printf

	// *** Basic block 339

	call        abort

	// *** Basic block 340

.PrintInstruction_label_2203:

	// *** Basic block 341

.PrintInstruction_label_2204:
	j           .PrintInstruction_label_2550

	// *** Basic block 342

.PrintInstruction_label_2206:
	lla         s8, .str.201
	ld          a0, 40(s1)
	addi        a1, s0, -28
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	call        GetRegisterName

	// *** Basic block 343

	mv          a2, a0
	mv          a1, s8
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 344

	j           .PrintInstruction_label_2550

	// *** Basic block 345

.PrintInstruction_label_2226:
	addi        s3, s1, 40
	ld          s5, 40(s1)
	beq         s5, x0, .PrintInstruction_label_2233

	// *** Basic block 346

	j           .PrintInstruction_label_2248

	// *** Basic block 347

.PrintInstruction_label_2233:
	lla         a0, .str.202
	lla         a1, .str.203
	lla         a3, .str.204
	li          t0, 884		// 0x374
	mv          a2, t0
	call        printf

	// *** Basic block 348

	call        abort

	// *** Basic block 349

.PrintInstruction_label_2248:
	ld          s9, 8(s3)
	beq         s9, x0, .PrintInstruction_label_2255

	// *** Basic block 350

	j           .PrintInstruction_label_2270

	// *** Basic block 351

.PrintInstruction_label_2255:
	lla         a0, .str.205
	lla         a1, .str.206
	lla         a3, .str.207
	li          t0, 885		// 0x375
	mv          a2, t0
	call        printf

	// *** Basic block 352

	call        abort

	// *** Basic block 353

.PrintInstruction_label_2270:
	ld          s3, 16(s3)
	sd          s3, -56(s0)	// Spilled @2272
	beq         s3, x0, .PrintInstruction_label_2277

	// *** Basic block 354

	j           .PrintInstruction_label_2292

	// *** Basic block 355

.PrintInstruction_label_2277:
	lla         a0, .str.208
	lla         a1, .str.209
	lla         a3, .str.210
	li          t0, 886		// 0x376
	mv          a2, t0
	call        printf

	// *** Basic block 356

	call        abort

	// *** Basic block 357

.PrintInstruction_label_2292:
	lla         s11, .str.211
	addi        a1, s0, -28
	li          s3, 8		// 0x8 ASCII \x8
	mv          a2, s3
	mv          a0, s5
	call        GetRegisterName

	// *** Basic block 358

	addi        a1, s0, -48
	mv          a2, s3
	mv          a0, s9
	call        GetRegisterName

	// *** Basic block 359

	ld          a0, -56(s0)	// Spilled @2272
	call        TargetIntValue

	// *** Basic block 360

	sext.w      a4, a0
	mv          a3, a0
	mv          a2, a0
	mv          a1, s11
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 361

	j           .PrintInstruction_label_2550

	// *** Basic block 362

.PrintInstruction_label_2326:
	ld          a0, 40(s1)
	call        TargetIntValue

	// *** Basic block 363

	mv          s8, a0
	lla         s9, .str.212
	addi        a1, s0, -28
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	mv          a0, s1
	call        GetRegisterName

	// *** Basic block 364

	mv          a4, s8
	mv          a3, s8
	mv          a2, a0
	mv          a1, s9
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 365

	sltz        t1, s8
	not         t0, t1
	blt         s8, x0, .PrintInstruction_label_2360

	// *** Basic block 366

	slti        t0, s8, 255

	// *** Basic block 367

.PrintInstruction_label_2360:
	beqz        t0, .PrintInstruction_label_2389

	// *** Basic block 368

	slti        t1, s8, 32
	not         t0, t1
	ld          t1, -64(s0)	// Spilled @341
	blt         s8, t1, .PrintInstruction_label_2368

	// *** Basic block 369

	slti        t0, s8, 127

	// *** Basic block 370

.PrintInstruction_label_2368:
	beqz        t0, .PrintInstruction_label_2379

	// *** Basic block 371

	lla         a1, .str.213
	sext.w      a2, s8
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 372

	j           .PrintInstruction_label_2388

	// *** Basic block 373

.PrintInstruction_label_2379:
	lla         a1, .str.214
	sext.w      a2, s8
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 374

.PrintInstruction_label_2388:

	// *** Basic block 375

.PrintInstruction_label_2389:
	lla         a1, .str.215
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 376

	j           .PrintInstruction_label_2550

	// *** Basic block 377

.PrintInstruction_label_2397:
	lla         s3, .str.216
	ld          t0, 32(s1)
	beq         t0, x0, .PrintInstruction_label_2425

	// *** Basic block 378

	lla         s5, .str.217
	addi        a1, s0, -28
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	mv          a0, s1
	call        GetRegisterName

	// *** Basic block 379

	mv          a2, a0
	mv          a1, s5
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 380

	lla         s3, .str.218

	// *** Basic block 381

.PrintInstruction_label_2425:
	mv          s5, x0

	// *** Basic block 382

.PrintInstruction_label_2429:
	slli        t0, s5, 3
	add         t0, t0, t0
	ld          s6, 0(t0)
	beq         s6, x0, .PrintInstruction_label_2537

	// *** Basic block 383

	mv          a0, s6
	call        TargetIsConst

	// *** Basic block 384

	beqz        a0, .PrintInstruction_label_2455

	// *** Basic block 385

	lla         s7, .str.219
	mv          a0, s6
	call        TargetIntValue

	// *** Basic block 386

	mv          a3, a0
	mv          a2, s3
	mv          a1, s7
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 387

	j           .PrintInstruction_label_2534

	// *** Basic block 388

.PrintInstruction_label_2455:
	lw          s7, 16(s6)
	bne         s7, s10, .PrintInstruction_label_2493

	// *** Basic block 389

	beqz        t1, .PrintInstruction_label_2477

	// *** Basic block 390

	lla         a1, .str.220
	ld          t0, 112(s6)
	ld          a3, 16(t0)
	mv          a2, s3
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 391

	j           .PrintInstruction_label_2491

	// *** Basic block 392

.PrintInstruction_label_2477:
	lla         a1, .str.221
	ld          t0, 112(s6)
	ld          a3, 16(t0)
	mv          a2, s3
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 393

.PrintInstruction_label_2491:
	j           .PrintInstruction_label_2533

	// *** Basic block 394

.PrintInstruction_label_2493:
	li          t0, 3		// 0x3 ASCII \x3
	bne         s7, t0, .PrintInstruction_label_2512

	// *** Basic block 395

	lla         a1, .str.222
	lw          a3, 112(s6)
	mv          a2, s3
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 396

	j           .PrintInstruction_label_2532

	// *** Basic block 397

.PrintInstruction_label_2512:
	lla         s7, .str.223
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	mv          a1, t2
	mv          a0, s6
	call        GetRegisterName

	// *** Basic block 398

	mv          a3, a0
	mv          a2, s3
	mv          a1, s7
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 399

.PrintInstruction_label_2532:

	// *** Basic block 400

.PrintInstruction_label_2533:

	// *** Basic block 401

.PrintInstruction_label_2534:
	lla         s3, .str.224

	// *** Basic block 402

.PrintInstruction_label_2537:

	// *** Basic block 403

.PrintInstruction_label_2538:
	addi        s5, s5, 1
	bge         s5, s10, .PrintInstruction_label_2429

	// *** Basic block 404

.PrintInstruction_label_2543:
	lla         a1, .str.225
	ld          a0, -56(s0)	// Spilled @258
	call        fprintf

	// *** Basic block 405

.PrintInstruction_label_2550:
	j           .PrintInstruction_label_279
.func_end_PrintInstruction:
	.size PrintInstruction, .func_end_PrintInstruction-PrintInstruction

	.global RVEmitterInit
	.type RVEmitterInit, @function

RVEmitterInit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sd          a1, 0(a0)
	addi        t0, a1, 464
	sd          t0, 8(a0)
	sw          x0, 16(a0)
	lw          t0, 1580(t0)
	sw          t0, 20(a0)
	sw          x0, 24(a0)
	sd          x0, 32(a0)
	ret         
.func_end_RVEmitterInit:
	.size RVEmitterInit, .func_end_RVEmitterInit-RVEmitterInit

	.global NewRVEmitter
	.type NewRVEmitter, @function

NewRVEmitter:

	// *** Basic block 0

	.global malloc
	.global RVEmitterInit
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	mv          s1, a0
	li          a0, 40		// 0x28 ASCII '('
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        RVEmitterInit

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.NewRVEmitter_label_21:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewRVEmitter:
	.size NewRVEmitter, .func_end_NewRVEmitter-NewRVEmitter

	.global RVEmitterDestruct
	.type RVEmitterDestruct, @function

RVEmitterDestruct:

	// *** Basic block 0

	ret         
.func_end_RVEmitterDestruct:
	.size RVEmitterDestruct, .func_end_RVEmitterDestruct-RVEmitterDestruct

	.global RVEmitterDelete
	.type RVEmitterDelete, @function

RVEmitterDelete:

	// *** Basic block 0

	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	.global RVEmitterDestruct
	.global free
	mv          s1, a0
	call        RVEmitterDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_RVEmitterDelete:
	.size RVEmitterDelete, .func_end_RVEmitterDelete-RVEmitterDelete

	.global RVPrintFunction
	.type RVPrintFunction, @function

RVPrintFunction:

	// *** Basic block 0

	.global fprintf
	.global TargetFirstInstruction
	.local PrintInstruction
	.global TargetNext
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	ld          t0, 0(s1)
	ld          s3, 16(t0)
	lb          t0, 40(t0)
	beqz        t0, .RVPrintFunction_label_39

	// *** Basic block 1

	lla         a1, .str.226
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 2

	j           .RVPrintFunction_label_48

	// *** Basic block 3

.RVPrintFunction_label_39:
	lla         a1, .str.227
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 4

.RVPrintFunction_label_48:
	lla         a1, .str.228
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 5

	lla         a1, .str.229
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 6

	ld          a0, 0(s1)
	call        TargetFirstInstruction

	// *** Basic block 7

	mv          s4, a0
	beq         s4, x0, .RVPrintFunction_label_91

	// *** Basic block 8

.RVPrintFunction_label_73:
	mv          a3, s2
	mv          a2, s3
	mv          a1, s4
	mv          a0, s1
	call        PrintInstruction

	// *** Basic block 9

	mv          a0, s4
	call        TargetNext

	// *** Basic block 10

	mv          s4, a0
	bne         s4, x0, .RVPrintFunction_label_73

	// *** Basic block 11

.RVPrintFunction_label_91:
	lla         a1, .str.230
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 12

	lla         a1, .str.231
	mv          a4, s3
	mv          a3, s3
	mv          a2, s3
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           fprintf
.func_end_RVPrintFunction:
	.size RVPrintFunction, .func_end_RVPrintFunction-RVPrintFunction

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "\tlui t0, %d\n"
	.type .str.1, @object
	.size .str.1, 13

.str.2:
	.asciz "\taddi t0, t0, %d\n"
	.type .str.2, @object
	.size .str.2, 18

.str.3:
	.asciz "\tsub sp, sp, t0\n"
	.type .str.3, @object
	.size .str.3, 17

.str.4:
	.asciz "\taddi sp, sp, -%d\n"
	.type .str.4, @object
	.size .str.4, 19

.str.5:
	.asciz "\tlui t0, %d\n"
	.type .str.5, @object
	.size .str.5, 13

.str.6:
	.asciz "\taddi t0, t0, %d\n"
	.type .str.6, @object
	.size .str.6, 18

.str.7:
	.asciz "\tadd sp, sp, t0\n"
	.type .str.7, @object
	.size .str.7, 17

.str.8:
	.asciz "\taddi sp, sp, %d\n"
	.type .str.8, @object
	.size .str.8, 18

.str.9:
	.asciz "fld"
	.type .str.9, @object
	.size .str.9, 4

.str.10:
	.asciz "ld"
	.type .str.10, @object
	.size .str.10, 3

.str.11:
	.asciz "\tf%s %s, -%d(s0)"
	.type .str.11, @object
	.size .str.11, 17

.str.12:
	.asciz "\tli t0, %d\n"
	.type .str.12, @object
	.size .str.12, 12

.str.13:
	.asciz "\taddi t0, t0, s0\n"
	.type .str.13, @object
	.size .str.13, 18

.str.14:
	.asciz "\t%s %s, 0(t0)"
	.type .str.14, @object
	.size .str.14, 14

.str.15:
	.asciz "\t\t// %s\n"
	.type .str.15, @object
	.size .str.15, 9

.str.16:
	.asciz "\taddi %s, s0, -%d"
	.type .str.16, @object
	.size .str.16, 18

.str.17:
	.asciz "\tli t0, %d\n"
	.type .str.17, @object
	.size .str.17, 12

.str.18:
	.asciz "\tadd %s, s0, t0"
	.type .str.18, @object
	.size .str.18, 16

.str.19:
	.asciz "\t\t// %s\n"
	.type .str.19, @object
	.size .str.19, 9

.str.20:
	.asciz "\t// Leaf procedure, no stack frame generated\n"
	.type .str.20, @object
	.size .str.20, 46

.str.21:
	.asciz "\tsd ra, 8(sp)\n"
	.type .str.21, @object
	.size .str.21, 15

.str.22:
	.asciz "\tsd s0, 0(sp)\n"
	.type .str.22, @object
	.size .str.22, 15

.str.23:
	.asciz "\t// Saved return address (offset %d) and frame pointer (offset %d)\n"
	.type .str.23, @object
	.size .str.23, 68

.str.24:
	.asciz "\tsd ra, %d(sp)\n"
	.type .str.24, @object
	.size .str.24, 16

.str.25:
	.asciz "\tsd s0, %d(sp)\n"
	.type .str.25, @object
	.size .str.25, 16

.str.26:
	.asciz "\taddi t0, t0, -%d\n"
	.type .str.26, @object
	.size .str.26, 19

.str.27:
	.asciz "\taddi t0, t0, 16\n"
	.type .str.27, @object
	.size .str.27, 18

.str.28:
	.asciz "\tadd s0, sp, t0\n"
	.type .str.28, @object
	.size .str.28, 17

.str.29:
	.asciz "\taddi s0, sp, %d\n"
	.type .str.29, @object
	.size .str.29, 18

.str.30:
	.asciz "\t// varargs function with %d declared args\n"
	.type .str.30, @object
	.size .str.30, 44

.str.31:
	.asciz "\tsd a%d, %d(s0)\n"
	.type .str.31, @object
	.size .str.31, 17

.str.32:
	.asciz "\t// Saved argument registers.\n"
	.type .str.32, @object
	.size .str.32, 31

.str.33:
	.asciz "\tsd %s, %d(%s)\n"
	.type .str.33, @object
	.size .str.33, 16

.str.34:
	.asciz "\t// Local vars at offset -%d(s0)\n"
	.type .str.34, @object
	.size .str.34, 34

.str.35:
	.asciz "\t// Spilled register region: %d bytes at -%d(s0) to -%d(s0)\n"
	.type .str.35, @object
	.size .str.35, 61

.str.36:
	.asciz "\t// Saved integer registers.\n"
	.type .str.36, @object
	.size .str.36, 30

.str.37:
	.asciz "\tsd %s, %d(sp)\n"
	.type .str.37, @object
	.size .str.37, 16

.str.38:
	.asciz "\t// Saved floating point registers.\n"
	.type .str.38, @object
	.size .str.38, 37

.str.39:
	.asciz "\tfsd %s, %d(sp)\n"
	.type .str.39, @object
	.size .str.39, 17

.str.40:
	.asciz "\t// End of stack frame\n"
	.type .str.40, @object
	.size .str.40, 24

.str.41:
	.asciz "\t// Restored registers.\n"
	.type .str.41, @object
	.size .str.41, 25

.str.42:
	.asciz "\tfld %s, %d(sp)\n"
	.type .str.42, @object
	.size .str.42, 17

.str.43:
	.asciz "\tld %s, %d(sp)\n"
	.type .str.43, @object
	.size .str.43, 16

.str.44:
	.asciz "\taddi sp, s0, %d\n"
	.type .str.44, @object
	.size .str.44, 18

.str.45:
	.asciz "\tld ra, -8(s0)\n"
	.type .str.45, @object
	.size .str.45, 16

.str.46:
	.asciz "\tld s0, -16(s0)\n"
	.type .str.46, @object
	.size .str.46, 17

.str.47:
	.asciz "\tld s0, -8(s0)\n"
	.type .str.47, @object
	.size .str.47, 16

.str.48:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.48, @object
	.size .str.48, 30

.str.49:
	.asciz "(null)"
	.type .str.49, @object
	.size .str.49, 1

.str.50:
	.asciz "inst->operand[0] != NULL"
	.type .str.50, @object
	.size .str.50, 25

.str.51:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.51, @object
	.size .str.51, 30

.str.52:
	.asciz "(null)"
	.type .str.52, @object
	.size .str.52, 1

.str.53:
	.asciz "inst->operand[1] != NULL"
	.type .str.53, @object
	.size .str.53, 25

.str.54:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.54, @object
	.size .str.54, 30

.str.55:
	.asciz "(null)"
	.type .str.55, @object
	.size .str.55, 1

.str.56:
	.asciz "inst->operand[0]->reg != NULL"
	.type .str.56, @object
	.size .str.56, 30

.str.57:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.57, @object
	.size .str.57, 30

.str.58:
	.asciz "(null)"
	.type .str.58, @object
	.size .str.58, 1

.str.59:
	.asciz "inst->operand[1]->reg != NULL"
	.type .str.59, @object
	.size .str.59, 30

.str.60:
	.asciz "(null)"
	.type .str.60, @object
	.size .str.60, 1

.str.61:
	.asciz "mv"
	.type .str.61, @object
	.size .str.61, 3

.str.62:
	.asciz "fmv.s"
	.type .str.62, @object
	.size .str.62, 6

.str.63:
	.asciz "fmv.d"
	.type .str.63, @object
	.size .str.63, 6

.str.64:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.64, @object
	.size .str.64, 30

.str.65:
	.asciz "(null)"
	.type .str.65, @object
	.size .str.65, 1

.str.66:
	.asciz "false"
	.type .str.66, @object
	.size .str.66, 6

.str.67:
	.asciz "\t%-12s%s, %s\n"
	.type .str.67, @object
	.size .str.67, 14

.str.68:
	.asciz "\n\t// *** Basic block %zd\n\n"
	.type .str.68, @object
	.size .str.68, 27

.str.69:
	.asciz "\t.local .%s_label_%d\n"
	.type .str.69, @object
	.size .str.69, 22

.str.70:
	.asciz ".%s_label_%d:\n"
	.type .str.70, @object
	.size .str.70, 15

.str.71:
	.asciz "%s:\n"
	.type .str.71, @object
	.size .str.71, 5

.str.72:
	.asciz "\t.local %s\n"
	.type .str.72, @object
	.size .str.72, 12

.str.73:
	.asciz "\t.global %s\n"
	.type .str.73, @object
	.size .str.73, 13

.str.74:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.74, @object
	.size .str.74, 30

.str.75:
	.asciz "(null)"
	.type .str.75, @object
	.size .str.75, 1

.str.76:
	.asciz "inst->operand[0]->opcode == RV_OP(symbol)"
	.type .str.76, @object
	.size .str.76, 42

.str.77:
	.asciz "\t%-12s%s\n"
	.type .str.77, @object
	.size .str.77, 10

.str.78:
	.asciz "call"
	.type .str.78, @object
	.size .str.78, 5

.str.79:
	.asciz "\t%-12s x1, %s, 0\n"
	.type .str.79, @object
	.size .str.79, 18

.str.80:
	.asciz "jalr"
	.type .str.80, @object
	.size .str.80, 5

.str.81:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.81, @object
	.size .str.81, 30

.str.82:
	.asciz "(null)"
	.type .str.82, @object
	.size .str.82, 1

.str.83:
	.asciz "lit != NULL"
	.type .str.83, @object
	.size .str.83, 12

.str.84:
	.asciz "\t%s\n"
	.type .str.84, @object
	.size .str.84, 5

.str.85:
	.asciz "\t.loc %d %d %d\n"
	.type .str.85, @object
	.size .str.85, 16

.str.86:
	.asciz "\t%-12s%s, %d\n"
	.type .str.86, @object
	.size .str.86, 14

.str.87:
	.asciz "lui"
	.type .str.87, @object
	.size .str.87, 4

.str.88:
	.asciz "\t%-12s%s, s0, %s\n"
	.type .str.88, @object
	.size .str.88, 18

.str.89:
	.asciz "sub"
	.type .str.89, @object
	.size .str.89, 4

.str.90:
	.asciz "\t%-12s%s, -%d(%s)\t// Spilled @%d\n"
	.type .str.90, @object
	.size .str.90, 34

.str.91:
	.asciz "sd"
	.type .str.91, @object
	.size .str.91, 3

.str.92:
	.asciz "fsd"
	.type .str.92, @object
	.size .str.92, 4

.str.93:
	.asciz "\t%-12s%s, -%d(s0)\t// Spilled @%d\n"
	.type .str.93, @object
	.size .str.93, 34

.str.94:
	.asciz "sd"
	.type .str.94, @object
	.size .str.94, 3

.str.95:
	.asciz "fsd"
	.type .str.95, @object
	.size .str.95, 4

.str.96:
	.asciz "\t%-12s%s, %d\n"
	.type .str.96, @object
	.size .str.96, 14

.str.97:
	.asciz "lui"
	.type .str.97, @object
	.size .str.97, 4

.str.98:
	.asciz "\t%-12s%s, s0, %s\n"
	.type .str.98, @object
	.size .str.98, 18

.str.99:
	.asciz "sub"
	.type .str.99, @object
	.size .str.99, 4

.str.100:
	.asciz "\t%-12s%s, -%d(%s)\t// Reloaded spilled @%d\n"
	.type .str.100, @object
	.size .str.100, 43

.str.101:
	.asciz "ld"
	.type .str.101, @object
	.size .str.101, 3

.str.102:
	.asciz "fld"
	.type .str.102, @object
	.size .str.102, 4

.str.103:
	.asciz "\t%-12s%s, -%d(s0)\t// Spilled @%d\n"
	.type .str.103, @object
	.size .str.103, 34

.str.104:
	.asciz "ld"
	.type .str.104, @object
	.size .str.104, 3

.str.105:
	.asciz "fld"
	.type .str.105, @object
	.size .str.105, 4

.str.106:
	.asciz "\t%-12s"
	.type .str.106, @object
	.size .str.106, 7

.str.107:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.107, @object
	.size .str.107, 30

.str.108:
	.asciz "(null)"
	.type .str.108, @object
	.size .str.108, 1

.str.109:
	.asciz "inst->operand[0] != NULL"
	.type .str.109, @object
	.size .str.109, 25

.str.110:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.110, @object
	.size .str.110, 30

.str.111:
	.asciz "(null)"
	.type .str.111, @object
	.size .str.111, 1

.str.112:
	.asciz "inst->operand[1] != NULL"
	.type .str.112, @object
	.size .str.112, 25

.str.113:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.113, @object
	.size .str.113, 30

.str.114:
	.asciz "(null)"
	.type .str.114, @object
	.size .str.114, 1

.str.115:
	.asciz "inst->reg != NULL"
	.type .str.115, @object
	.size .str.115, 18

.str.116:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.116, @object
	.size .str.116, 30

.str.117:
	.asciz "(null)"
	.type .str.117, @object
	.size .str.117, 1

.str.118:
	.asciz "inst->operand[0]->reg != NULL"
	.type .str.118, @object
	.size .str.118, 30

.str.119:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.119, @object
	.size .str.119, 30

.str.120:
	.asciz "(null)"
	.type .str.120, @object
	.size .str.120, 1

.str.121:
	.asciz "RVIsPossibleImmediate(offset)"
	.type .str.121, @object
	.size .str.121, 30

.str.122:
	.asciz "%s, %d(%s)\n"
	.type .str.122, @object
	.size .str.122, 12

.str.123:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.123, @object
	.size .str.123, 30

.str.124:
	.asciz "(null)"
	.type .str.124, @object
	.size .str.124, 1

.str.125:
	.asciz "(inst->flags & RV_LO_RELOC) != 0"
	.type .str.125, @object
	.size .str.125, 33

.str.126:
	.asciz "%s, %%lo(%s)(%s)\n"
	.type .str.126, @object
	.size .str.126, 18

.str.127:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.127, @object
	.size .str.127, 30

.str.128:
	.asciz "(null)"
	.type .str.128, @object
	.size .str.128, 1

.str.129:
	.asciz "(inst->flags & RV_PCREL_LO_RELOC) != 0"
	.type .str.129, @object
	.size .str.129, 39

.str.130:
	.asciz "%s, %%pcrel_lo(.%s_label_%d)(%s)\n"
	.type .str.130, @object
	.size .str.130, 34

.str.131:
	.asciz "%s, 0(%s)\n"
	.type .str.131, @object
	.size .str.131, 11

.str.132:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.132, @object
	.size .str.132, 30

.str.133:
	.asciz "(null)"
	.type .str.133, @object
	.size .str.133, 1

.str.134:
	.asciz "false"
	.type .str.134, @object
	.size .str.134, 6

.str.135:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.135, @object
	.size .str.135, 30

.str.136:
	.asciz "(null)"
	.type .str.136, @object
	.size .str.136, 1

.str.137:
	.asciz "inst->operand[0] != NULL"
	.type .str.137, @object
	.size .str.137, 25

.str.138:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.138, @object
	.size .str.138, 30

.str.139:
	.asciz "(null)"
	.type .str.139, @object
	.size .str.139, 1

.str.140:
	.asciz "inst->operand[1] != NULL"
	.type .str.140, @object
	.size .str.140, 25

.str.141:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.141, @object
	.size .str.141, 30

.str.142:
	.asciz "(null)"
	.type .str.142, @object
	.size .str.142, 1

.str.143:
	.asciz "inst->operand[2] != NULL"
	.type .str.143, @object
	.size .str.143, 25

.str.144:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.144, @object
	.size .str.144, 30

.str.145:
	.asciz "(null)"
	.type .str.145, @object
	.size .str.145, 1

.str.146:
	.asciz "inst->operand[0]->reg != NULL"
	.type .str.146, @object
	.size .str.146, 30

.str.147:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.147, @object
	.size .str.147, 30

.str.148:
	.asciz "(null)"
	.type .str.148, @object
	.size .str.148, 1

.str.149:
	.asciz "inst->operand[1]->reg != NULL"
	.type .str.149, @object
	.size .str.149, 30

.str.150:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.150, @object
	.size .str.150, 30

.str.151:
	.asciz "(null)"
	.type .str.151, @object
	.size .str.151, 1

.str.152:
	.asciz "RVIsPossibleImmediate(offset)"
	.type .str.152, @object
	.size .str.152, 30

.str.153:
	.asciz "%s, %d(%s)\n"
	.type .str.153, @object
	.size .str.153, 12

.str.154:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.154, @object
	.size .str.154, 30

.str.155:
	.asciz "(null)"
	.type .str.155, @object
	.size .str.155, 1

.str.156:
	.asciz "(inst->flags & RV_LO_RELOC) != 0"
	.type .str.156, @object
	.size .str.156, 33

.str.157:
	.asciz "%s, %%lo(%s)(%s)\n"
	.type .str.157, @object
	.size .str.157, 18

.str.158:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.158, @object
	.size .str.158, 30

.str.159:
	.asciz "(null)"
	.type .str.159, @object
	.size .str.159, 1

.str.160:
	.asciz "(inst->flags & RV_PCREL_LO_RELOC) != 0"
	.type .str.160, @object
	.size .str.160, 39

.str.161:
	.asciz "%s, %%pcrel_lo(.%s_label_%d)(%s)\n"
	.type .str.161, @object
	.size .str.161, 34

.str.162:
	.asciz "%s, 0(%s)\n"
	.type .str.162, @object
	.size .str.162, 11

.str.163:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.163, @object
	.size .str.163, 30

.str.164:
	.asciz "(null)"
	.type .str.164, @object
	.size .str.164, 1

.str.165:
	.asciz "false"
	.type .str.165, @object
	.size .str.165, 6

.str.166:
	.asciz "\n"
	.type .str.166, @object
	.size .str.166, 2

.str.167:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.167, @object
	.size .str.167, 30

.str.168:
	.asciz "(null)"
	.type .str.168, @object
	.size .str.168, 1

.str.169:
	.asciz "inst->operand[0] != NULL"
	.type .str.169, @object
	.size .str.169, 25

.str.170:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.170, @object
	.size .str.170, 30

.str.171:
	.asciz "(null)"
	.type .str.171, @object
	.size .str.171, 1

.str.172:
	.asciz "inst->operand[1] != NULL"
	.type .str.172, @object
	.size .str.172, 25

.str.173:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.173, @object
	.size .str.173, 30

.str.174:
	.asciz "(null)"
	.type .str.174, @object
	.size .str.174, 1

.str.175:
	.asciz "inst->operand[2] != NULL"
	.type .str.175, @object
	.size .str.175, 25

.str.176:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.176, @object
	.size .str.176, 30

.str.177:
	.asciz "(null)"
	.type .str.177, @object
	.size .str.177, 1

.str.178:
	.asciz "inst->operand[0]->reg != NULL"
	.type .str.178, @object
	.size .str.178, 30

.str.179:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.179, @object
	.size .str.179, 30

.str.180:
	.asciz "(null)"
	.type .str.180, @object
	.size .str.180, 1

.str.181:
	.asciz "inst->operand[1]->reg != NULL"
	.type .str.181, @object
	.size .str.181, 30

.str.182:
	.asciz "%s, %s, .%s_label_%d\n"
	.type .str.182, @object
	.size .str.182, 22

.str.183:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.183, @object
	.size .str.183, 30

.str.184:
	.asciz "(null)"
	.type .str.184, @object
	.size .str.184, 1

.str.185:
	.asciz "inst->operand[0] != NULL"
	.type .str.185, @object
	.size .str.185, 25

.str.186:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.186, @object
	.size .str.186, 30

.str.187:
	.asciz "(null)"
	.type .str.187, @object
	.size .str.187, 1

.str.188:
	.asciz "inst->operand[1] != NULL"
	.type .str.188, @object
	.size .str.188, 25

.str.189:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.189, @object
	.size .str.189, 30

.str.190:
	.asciz "(null)"
	.type .str.190, @object
	.size .str.190, 1

.str.191:
	.asciz "inst->operand[0]->reg != NULL"
	.type .str.191, @object
	.size .str.191, 30

.str.192:
	.asciz "%s, .%s_label_%d\n"
	.type .str.192, @object
	.size .str.192, 18

.str.193:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.193, @object
	.size .str.193, 30

.str.194:
	.asciz "(null)"
	.type .str.194, @object
	.size .str.194, 1

.str.195:
	.asciz "inst->operand[0] != NULL"
	.type .str.195, @object
	.size .str.195, 25

.str.196:
	.asciz ".%s_label_%d\n"
	.type .str.196, @object
	.size .str.196, 14

.str.197:
	.asciz "%s\n"
	.type .str.197, @object
	.size .str.197, 4

.str.198:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.198, @object
	.size .str.198, 30

.str.199:
	.asciz "(null)"
	.type .str.199, @object
	.size .str.199, 1

.str.200:
	.asciz "false"
	.type .str.200, @object
	.size .str.200, 6

.str.201:
	.asciz "%s\n"
	.type .str.201, @object
	.size .str.201, 4

.str.202:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.202, @object
	.size .str.202, 30

.str.203:
	.asciz "(null)"
	.type .str.203, @object
	.size .str.203, 1

.str.204:
	.asciz "inst->operand[0] != NULL"
	.type .str.204, @object
	.size .str.204, 25

.str.205:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.205, @object
	.size .str.205, 30

.str.206:
	.asciz "(null)"
	.type .str.206, @object
	.size .str.206, 1

.str.207:
	.asciz "inst->operand[1] != NULL"
	.type .str.207, @object
	.size .str.207, 25

.str.208:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.208, @object
	.size .str.208, 30

.str.209:
	.asciz "(null)"
	.type .str.209, @object
	.size .str.209, 1

.str.210:
	.asciz "inst->operand[2] != NULL"
	.type .str.210, @object
	.size .str.210, 25

.str.211:
	.asciz "%s, %s, %d\n"
	.type .str.211, @object
	.size .str.211, 12

.str.212:
	.asciz "%s, %" PRId64 "\t\t// 0x%" PRIx64 ""
	.type .str.212, @object
	.size .str.212, 20

.str.213:
	.asciz " ASCII \'%c\'"
	.type .str.213, @object
	.size .str.213, 12

.str.214:
	.asciz " ASCII \\x%x"
	.type .str.214, @object
	.size .str.214, 12

.str.215:
	.asciz "\n"
	.type .str.215, @object
	.size .str.215, 2

.str.216:
	.asciz "(null)"
	.type .str.216, @object
	.size .str.216, 1

.str.217:
	.asciz "%s"
	.type .str.217, @object
	.size .str.217, 3

.str.218:
	.asciz ", "
	.type .str.218, @object
	.size .str.218, 3

.str.219:
	.asciz "%s%" PRId64 ""
	.type .str.219, @object
	.size .str.219, 7

.str.220:
	.asciz "%s%%hi(%s)"
	.type .str.220, @object
	.size .str.220, 11

.str.221:
	.asciz "%s%s"
	.type .str.221, @object
	.size .str.221, 5

.str.222:
	.asciz "%s.str.%d"
	.type .str.222, @object
	.size .str.222, 10

.str.223:
	.asciz "%s%s"
	.type .str.223, @object
	.size .str.223, 5

.str.224:
	.asciz ", "
	.type .str.224, @object
	.size .str.224, 3

.str.225:
	.asciz "\n"
	.type .str.225, @object
	.size .str.225, 2

.str.226:
	.asciz "\t.global %s\n"
	.type .str.226, @object
	.size .str.226, 13

.str.227:
	.asciz "\t.local  %s\n"
	.type .str.227, @object
	.size .str.227, 13

.str.228:
	.asciz "\t.type %s, @function\n\n"
	.type .str.228, @object
	.size .str.228, 23

.str.229:
	.asciz "%s:\n"
	.type .str.229, @object
	.size .str.229, 5

.str.230:
	.asciz ".func_end_%s:\n"
	.type .str.230, @object
	.size .str.230, 15

.str.231:
	.asciz "\t.size %s, .func_end_%s-%s\n\n"
	.type .str.231, @object
	.size .str.231, 29

