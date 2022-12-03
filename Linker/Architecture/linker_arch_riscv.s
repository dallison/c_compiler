	.file   "Architecture/linker_arch_riscv.c"
	.text
	.option pic
.PCbegin:
	.local  CodeStartAddress
	.type CodeStartAddress, @function

CodeStartAddress:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	lb          t1, 528(t0)
	beqz        t1, .CodeStartAddress_label_22

	// *** Basic block 1

	li          t1, 288		// 0x120
	j           .CodeStartAddress_label_31

	// *** Basic block 2

.CodeStartAddress_label_22:
	lb          t2, 529(t0)
	beqz        t2, .CodeStartAddress_label_28

	// *** Basic block 3

	li          t1, 1073742056		// 0x400000e8
	j           .CodeStartAddress_label_30

	// *** Basic block 4

.CodeStartAddress_label_28:
	li          t1, 1073742168		// 0x40000158

	// *** Basic block 5

.CodeStartAddress_label_30:

	// *** Basic block 6

.CodeStartAddress_label_31:
	addi        t2, t0, 248
	ld          t2, 8(t2)
	addi        t2, t2, 4
	addi        t2, t2, 1
	slli        t2, t2, 6
	add         t1, t1, t2
	mv          a0, t1

	// *** Basic block 7

.CodeStartAddress_label_42:
	ret         
.func_end_CodeStartAddress:
	.size CodeStartAddress, .func_end_CodeStartAddress-CodeStartAddress

	.local  DataStartAddress
	.type DataStartAddress, @function

DataStartAddress:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a2
	mv          t2, a1
	lb          t3, 528(t0)
	beqz        t3, .DataStartAddress_label_37

	// *** Basic block 1

	li          t3, 4096		// 0x1000
	add         t3, t2, t3
	addi        t3, t3, -1
	li          t4, -4096		// 0xfffffffffffff000
	and         t2, t3, t4
	addi        t2, t2, 288
	j           .DataStartAddress_label_46

	// *** Basic block 2

.DataStartAddress_label_37:
	lb          t3, 529(t0)
	beqz        t3, .DataStartAddress_label_43

	// *** Basic block 3

	li          t2, 1090519272		// 0x410000e8
	j           .DataStartAddress_label_45

	// *** Basic block 4

.DataStartAddress_label_43:
	li          t2, 1090519384		// 0x41000158

	// *** Basic block 5

.DataStartAddress_label_45:

	// *** Basic block 6

.DataStartAddress_label_46:
	addi        t3, t0, 248
	ld          t3, 8(t3)
	addi        t3, t3, 4
	addi        t3, t3, 1
	slli        t3, t3, 6
	add         t3, t3, t1
	add         t2, t2, t3
	mv          a0, t2

	// *** Basic block 7

.DataStartAddress_label_58:
	ret         
.func_end_DataStartAddress:
	.size DataStartAddress, .func_end_DataStartAddress-DataStartAddress

	.local  HandlePICRelocation
	.type HandlePICRelocation, @function

HandlePICRelocation:

	// *** Basic block 0

	.global NewRelativeRelocation
	.global VectorAppend
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 56(sp)
	sd s2, 48(sp)
	sd s3, 40(sp)
	sd s4, 32(sp)
	sd s5, 24(sp)
	sd s6, 16(sp)
	sd s7, 8(sp)
	// End of stack frame
	mv          s1, a2
	mv          s2, a1
	mv          s3, a3
	mv          s4, a0
	mv          s5, a4
	mv          s6, a5
	lw          s7, 56(s1)
	li          t0, 2		// 0x2 ASCII \x2
	beq         s7, t0, .HandlePICRelocation_label_123

	// *** Basic block 1

	li          t0, 19		// 0x13 ASCII \x13
	beq         s7, t0, .HandlePICRelocation_label_103

	// *** Basic block 2

	li          t0, 20		// 0x14 ASCII \x14
	beq         s7, t0, .HandlePICRelocation_label_66

	// *** Basic block 3

	li          t0, 21		// 0x15 ASCII \x15
	beq         s7, t0, .HandlePICRelocation_label_67

	// *** Basic block 4

	li          t0, 22		// 0x16 ASCII \x16
	beq         s7, t0, .HandlePICRelocation_label_80

	// *** Basic block 5

	j           .HandlePICRelocation_label_145

	// *** Basic block 6

.HandlePICRelocation_label_66:

	// *** Basic block 7

.HandlePICRelocation_label_67:
	beq         s2, x0, .HandlePICRelocation_label_78

	// *** Basic block 8

	mv          a1, s2
	mv          a0, s4
	jalr         x1, s3, 0

	// *** Basic block 9

	sw          a0, 88(s2)

	// *** Basic block 10

.HandlePICRelocation_label_78:
	j           .HandlePICRelocation_label_145

	// *** Basic block 11

.HandlePICRelocation_label_80:
	beq         s2, x0, .HandlePICRelocation_label_101

	// *** Basic block 12

	mv          a1, s2
	mv          a0, s4
	jalr         x1, s3, 0

	// *** Basic block 13

	sw          a0, 88(s2)
	mv          a1, s2
	mv          a0, s4
	jalr         x1, s3, 0

	// *** Basic block 14

	lw          t0, 88(s2)
	addi        t0, t0, -1
	sw          t0, 88(s2)

	// *** Basic block 15

.HandlePICRelocation_label_101:
	j           .HandlePICRelocation_label_145

	// *** Basic block 16

.HandlePICRelocation_label_103:
	beq         s2, x0, .HandlePICRelocation_label_121

	// *** Basic block 17

	mv          a1, s2
	mv          a0, s4
	jalr         x1, s5, 0

	// *** Basic block 18

	sw          a0, 88(s2)
	mv          a1, s2
	mv          a0, s4
	jalr         x1, s6, 0

	// *** Basic block 19

	sw          a0, 92(s2)

	// *** Basic block 20

.HandlePICRelocation_label_121:
	j           .HandlePICRelocation_label_145

	// *** Basic block 21

.HandlePICRelocation_label_123:
	ld          a0, 64(s1)
	ld          a1, 48(s1)
	ld          a3, 72(s1)
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	call        NewRelativeRelocation

	// *** Basic block 22

	mv          s7, a0
	addi        a0, s4, 184
	mv          a1, s7
	call        VectorAppend

	// *** Basic block 23

	j           .HandlePICRelocation_label_145

	// *** Basic block 24

.HandlePICRelocation_label_145:
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
.func_end_HandlePICRelocation:
	.size HandlePICRelocation, .func_end_HandlePICRelocation-HandlePICRelocation

	.local  SetBitField32
	.type SetBitField32, @function

SetBitField32:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	li          t0, 1		// 0x1 ASCII \x1
	sll         t0, t0, a2
	addi        t1, t0, -1
	and         t0, a3, t1
	sll         t1, t1, a1
	lw          t2, 0(a0)
	not         t1, t1
	and         t2, t2, t1
	sll         t1, a3, a1
	or          t2, t2, t1
	sw          t2, 0(a0)
	ret         
.func_end_SetBitField32:
	.size SetBitField32, .func_end_SetBitField32-SetBitField32

	.local  AddBitField32
	.type AddBitField32, @function

AddBitField32:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	li          t0, 1		// 0x1 ASCII \x1
	sll         t0, t0, a2
	addi        t1, t0, -1
	lw          t0, 0(a0)
	sra         t2, t0, a1
	and         t3, t2, t1
	add         t2, a3, t3
	and         t2, a3, t1
	sll         t1, t1, a1
	not         t1, t1
	and         t0, t0, t1
	sll         t1, a3, a1
	or          t0, t0, t1
	sw          t0, 0(a0)
	ret         
.func_end_AddBitField32:
	.size AddBitField32, .func_end_AddBitField32-AddBitField32

	.local  SubBitField32
	.type SubBitField32, @function

SubBitField32:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	li          t0, 1		// 0x1 ASCII \x1
	sll         t0, t0, a2
	addi        t1, t0, -1
	lw          t0, 0(a0)
	sra         t2, t0, a1
	and         t3, t2, t1
	sub         t2, t3, a3
	and         t2, a3, t1
	sll         t1, t1, a1
	not         t1, t1
	and         t0, t0, t1
	sll         t1, a3, a1
	or          t0, t0, t1
	sw          t0, 0(a0)
	ret         
.func_end_SubBitField32:
	.size SubBitField32, .func_end_SubBitField32-SubBitField32

	.local  SetJTypeImm
	.type SetJTypeImm, @function

SetJTypeImm:

	// *** Basic block 0

	.local SetBitField32
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	srai        t1, t0, 20
	andi        t1, t1, 1
	slli        t1, t1, 31
	srai        t2, t0, 1
	andi        t2, t2, 1023
	slli        t2, t2, 21
	or          t1, t1, t2
	srai        t2, t0, 11
	andi        t2, t2, 1
	slli        t2, t2, 20
	or          t1, t1, t2
	srai        t2, t0, 12
	andi        t2, t2, 255
	slli        t2, t2, 12
	or          t3, t1, t2
	mv          a3, t3
	li          a2, 20		// 0x14 ASCII \x14
	li          a1, 12		// 0xc ASCII \xc
	j           SetBitField32
.func_end_SetJTypeImm:
	.size SetJTypeImm, .func_end_SetJTypeImm-SetJTypeImm

	.local  FindRelocation
	.type FindRelocation, @function

FindRelocation:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, x0
	addi        t2, a0, 40
	ld          t2, 8(t2)
	bge         x0, t2, .FindRelocation_label_71

	// *** Basic block 1

	ld          t3, 40(a0)

	// *** Basic block 2

.FindRelocation_label_26:
	slli        t4, t1, 3
	add         t3, t3, t4
	ld          t4, 0(t3)
	mv          t3, x0
	addi        t5, t4, 136
	ld          t5, 8(t5)
	bge         x0, t5, .FindRelocation_label_66

	// *** Basic block 3

	ld          t4, 136(t4)

	// *** Basic block 4

.FindRelocation_label_40:
	slli        t6, t3, 3
	add         t4, t4, t6
	ld          t6, 0(t4)
	ld          t4, 48(t6)
	ld          t4, 56(t4)
	ld          a0, 64(t6)
	add         a1, t4, a0
	bne         a1, t0, .FindRelocation_label_61

	// *** Basic block 5

	mv          a0, t6

	// *** Basic block 6

.FindRelocation_label_58:
	ret         

	// *** Basic block 7

.FindRelocation_label_61:

	// *** Basic block 8

.FindRelocation_label_62:
	addi        t3, t3, 1
	bge         t3, t5, .FindRelocation_label_40

	// *** Basic block 9

.FindRelocation_label_66:

	// *** Basic block 10

.FindRelocation_label_67:
	addi        t1, t1, 1
	bge         t1, t2, .FindRelocation_label_26

	// *** Basic block 11

.FindRelocation_label_71:
	mv          a0, x0
	ret         
.func_end_FindRelocation:
	.size FindRelocation, .func_end_FindRelocation-FindRelocation

	.local  JTypeInstruction
	.type JTypeInstruction, @function

JTypeInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	srai        t0, a2, 20
	andi        t0, t0, 1
	slli        t0, t0, 31
	srai        t1, a2, 1
	andi        t1, t1, 1023
	slli        t1, t1, 21
	or          t0, t0, t1
	srai        t1, a2, 11
	andi        t1, t1, 1
	slli        t1, t1, 20
	or          t0, t0, t1
	srai        t1, a2, 12
	andi        t1, t1, 255
	slli        t1, t1, 12
	or          t0, t0, t1
	slli        t1, a1, 7
	or          t0, t0, t1
	or          t0, t0, a0
	mv          a0, t0

	// *** Basic block 1

.JTypeInstruction_label_40:
	ret         
.func_end_JTypeInstruction:
	.size JTypeInstruction, .func_end_JTypeInstruction-JTypeInstruction

	.local  ITypeInstruction
	.type ITypeInstruction, @function

ITypeInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	slli        t0, a4, 20
	slli        t1, a2, 15
	or          t0, t0, t1
	slli        t1, a3, 12
	or          t0, t0, t1
	slli        t1, a1, 7
	or          t0, t0, t1
	or          t0, t0, a0
	mv          a0, t0

	// *** Basic block 1

.ITypeInstruction_label_31:
	ret         
.func_end_ITypeInstruction:
	.size ITypeInstruction, .func_end_ITypeInstruction-ITypeInstruction

	.local  RTypeInstruction
	.type RTypeInstruction, @function

RTypeInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	slli        t0, a5, 25
	slli        t1, a3, 20
	or          t0, t0, t1
	slli        t1, a2, 15
	or          t0, t0, t1
	slli        t1, a4, 12
	or          t0, t0, t1
	slli        t1, a1, 7
	or          t0, t0, t1
	or          t0, t0, a0
	mv          a0, t0

	// *** Basic block 1

.RTypeInstruction_label_37:
	ret         
.func_end_RTypeInstruction:
	.size RTypeInstruction, .func_end_RTypeInstruction-RTypeInstruction

	.local  UTypeInstruction
	.type UTypeInstruction, @function

UTypeInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	slli        t0, a2, 12
	slli        t1, a1, 7
	or          t0, t0, t1
	or          t0, t0, a0
	mv          a0, t0

	// *** Basic block 1

.UTypeInstruction_label_19:
	ret         
.func_end_UTypeInstruction:
	.size UTypeInstruction, .func_end_UTypeInstruction-UTypeInstruction

	.local  High20
	.type High20, @function

High20:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sext.w      t0, a0
	li          t1, 2048		// 0x800
	add         t0, t0, t1
	srai        t0, t0, 12
	mv          a0, t0

	// *** Basic block 1

.High20_label_14:
	ret         
.func_end_High20:
	.size High20, .func_end_High20-High20

	.local  Low12
	.type Low12, @function

Low12:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sext.w      t0, a0
	slli        t1, a1, 12
	sub         t0, t0, t1
	mv          a0, t0

	// *** Basic block 1

.Low12_label_15:
	ret         
.func_end_Low12:
	.size Low12, .func_end_Low12-Low12

	.local  SplitValue
	.type SplitValue, @function

SplitValue:

	// *** Basic block 0

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
	.local High20
	.local Low12
	mv          s1, a1
	mv          s2, a0
	call        High20

	// *** Basic block 1

	sw          a0, 0(s1)
	lw          a1, 0(s1)
	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Low12
.func_end_SplitValue:
	.size SplitValue, .func_end_SplitValue-SplitValue

	.local  ApplyRelocation
	.type ApplyRelocation, @function

ApplyRelocation:

	// *** Basic block 0

	.local SetJTypeImm
	.local SplitValue
	.local SetBitField32
	.local JTypeInstruction
	.local FindRelocation
	.global LinkerError
	.global ObjectFileFindSymbol
	.local High20
	.local Low12
	.local AddBitField32
	.local SubBitField32
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -24(s0)
	// Spilled register region: 8 bytes at -32(s0) to -24(s0)
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
	mv          s1, a4
	mv          s2, a5
	mv          s3, a6
	mv          s4, a0
	mv          s5, a1
	mv          s6, a3
	ld          t0, 48(a2)
	ld          t0, 56(t0)
	ld          s7, 64(a2)
	sd          s7, -32(s0)	// Spilled @74
	add         s8, t0, s7
	lw          s9, 56(a2)
	blt         s9, x0, .ApplyRelocation_label_663

	// *** Basic block 1

	li          t0, 51		// 0x33 ASCII '3'
	blt         t0, s9, .ApplyRelocation_label_663

	// *** Basic block 2

	slli        t0, s9, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .ApplyRelocation_label_141

	// *** Basic block 4

	j           .ApplyRelocation_label_145

	// *** Basic block 5

	j           .ApplyRelocation_label_150

	// *** Basic block 6

	j           .ApplyRelocation_label_154

	// *** Basic block 7

	j           .ApplyRelocation_label_156

	// *** Basic block 8

	j           .ApplyRelocation_label_158

	// *** Basic block 9

	j           .ApplyRelocation_label_663

	// *** Basic block 10

	j           .ApplyRelocation_label_663

	// *** Basic block 11

	j           .ApplyRelocation_label_663

	// *** Basic block 12

	j           .ApplyRelocation_label_663

	// *** Basic block 13

	j           .ApplyRelocation_label_663

	// *** Basic block 14

	j           .ApplyRelocation_label_663

	// *** Basic block 15

	j           .ApplyRelocation_label_663

	// *** Basic block 16

	j           .ApplyRelocation_label_663

	// *** Basic block 17

	j           .ApplyRelocation_label_663

	// *** Basic block 18

	j           .ApplyRelocation_label_663

	// *** Basic block 19

	j           .ApplyRelocation_label_160

	// *** Basic block 20

	j           .ApplyRelocation_label_162

	// *** Basic block 21

	j           .ApplyRelocation_label_173

	// *** Basic block 22

	j           .ApplyRelocation_label_424

	// *** Basic block 23

	j           .ApplyRelocation_label_471

	// *** Basic block 24

	j           .ApplyRelocation_label_663

	// *** Basic block 25

	j           .ApplyRelocation_label_663

	// *** Basic block 26

	j           .ApplyRelocation_label_264

	// *** Basic block 27

	j           .ApplyRelocation_label_288

	// *** Basic block 28

	j           .ApplyRelocation_label_289

	// *** Basic block 29

	j           .ApplyRelocation_label_504

	// *** Basic block 30

	j           .ApplyRelocation_label_521

	// *** Basic block 31

	j           .ApplyRelocation_label_545

	// *** Basic block 32

	j           .ApplyRelocation_label_663

	// *** Basic block 33

	j           .ApplyRelocation_label_663

	// *** Basic block 34

	j           .ApplyRelocation_label_663

	// *** Basic block 35

	j           .ApplyRelocation_label_663

	// *** Basic block 36

	j           .ApplyRelocation_label_577

	// *** Basic block 37

	j           .ApplyRelocation_label_590

	// *** Basic block 38

	j           .ApplyRelocation_label_603

	// *** Basic block 39

	j           .ApplyRelocation_label_610

	// *** Basic block 40

	j           .ApplyRelocation_label_616

	// *** Basic block 41

	j           .ApplyRelocation_label_629

	// *** Basic block 42

	j           .ApplyRelocation_label_642

	// *** Basic block 43

	j           .ApplyRelocation_label_649

	// *** Basic block 44

	j           .ApplyRelocation_label_663

	// *** Basic block 45

	j           .ApplyRelocation_label_663

	// *** Basic block 46

	j           .ApplyRelocation_label_655

	// *** Basic block 47

	j           .ApplyRelocation_label_657

	// *** Basic block 48

	j           .ApplyRelocation_label_659

	// *** Basic block 49

	j           .ApplyRelocation_label_661

	// *** Basic block 50

	j           .ApplyRelocation_label_663

	// *** Basic block 51

	j           .ApplyRelocation_label_663

	// *** Basic block 52

	j           .ApplyRelocation_label_663

	// *** Basic block 53

	j           .ApplyRelocation_label_663

	// *** Basic block 54

	j           .ApplyRelocation_label_211

	// *** Basic block 55

.ApplyRelocation_label_141:

	// *** Basic block 56

.ApplyRelocation_label_142:
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

	// *** Basic block 57

.ApplyRelocation_label_145:
	add         t0, s2, s3
	sext.w      t0, t0
	sw          t0, 0(s1)
	j           .ApplyRelocation_label_142

	// *** Basic block 58

.ApplyRelocation_label_150:
	add         t0, s2, s3
	sd          t0, 0(s1)
	j           .ApplyRelocation_label_142

	// *** Basic block 59

.ApplyRelocation_label_154:
	j           .ApplyRelocation_label_665

	// *** Basic block 60

.ApplyRelocation_label_156:
	j           .ApplyRelocation_label_665

	// *** Basic block 61

.ApplyRelocation_label_158:
	j           .ApplyRelocation_label_665

	// *** Basic block 62

.ApplyRelocation_label_160:
	j           .ApplyRelocation_label_665

	// *** Basic block 63

.ApplyRelocation_label_162:
	sub         t0, s2, s8
	sub         t0, t0, s3
	sext.w      s10, t0
	mv          a1, s10
	mv          a0, s1
	call        SetJTypeImm

	// *** Basic block 64

	j           .ApplyRelocation_label_142

	// *** Basic block 65

.ApplyRelocation_label_173:
	sub         t0, s2, s8
	sub         t0, t0, s3
	sext.w      s10, t0
	addi        a1, s0, -24
	addi        a2, s0, -20
	mv          a0, s10
	call        SplitValue

	// *** Basic block 66

	lw          a3, -24(s0)
	li          s10, 20		// 0x14 ASCII \x14
	mv          a2, s10
	li          s11, 12		// 0xc ASCII \xc
	mv          a1, s11
	mv          a0, s1
	call        SetBitField32

	// *** Basic block 67

	addi        a0, s1, 4
	lw          a3, -20(s0)
	mv          a2, s11
	mv          a1, s10
	call        SetBitField32

	// *** Basic block 68

	j           .ApplyRelocation_label_142

	// *** Basic block 69

.ApplyRelocation_label_211:
	lw          s10, 0(s1)
	andi        t0, s10, 127
	li          t1, 23		// 0x17 ASCII \x17
	beq         t0, t1, .ApplyRelocation_label_221

	// *** Basic block 70

	j           .ApplyRelocation_label_142

	// *** Basic block 71

.ApplyRelocation_label_221:
	srai        s11, s10, 12
	srai        t0, s10, 7
	andi        s10, t0, 255
	bnez        s11, .ApplyRelocation_label_262

	// *** Basic block 72

	lw          s7, 4(s1)
	andi        t0, s7, 127
	li          t1, 103		// 0x67 ASCII 'g'
	beq         t0, t1, .ApplyRelocation_label_239

	// *** Basic block 73

	j           .ApplyRelocation_label_142

	// *** Basic block 74

.ApplyRelocation_label_239:
	srai        s11, s7, 20
	srai        t0, s7, 15
	sd          t0, -32(s0)	// Spilled @241
	andi        t0, t0, 31
	srai        t1, s7, 7
	andi        s7, t1, 31
	bne         t0, s10, .ApplyRelocation_label_261

	// *** Basic block 75

	sext.w      a2, s11
	mv          a1, s7
	li          t0, 111		// 0x6f ASCII 'o'
	mv          a0, t0
	call        JTypeInstruction

	// *** Basic block 76

	sw          a0, 0(s1)
	li          t0, 19		// 0x13 ASCII \x13
	sw          t0, 4(s1)

	// *** Basic block 77

.ApplyRelocation_label_261:

	// *** Basic block 78

.ApplyRelocation_label_262:
	j           .ApplyRelocation_label_142

	// *** Basic block 79

.ApplyRelocation_label_264:
	sub         t0, s2, s8
	sub         t0, t0, s3
	sext.w      s10, t0
	addi        a1, s0, -24
	addi        a2, s0, -20
	mv          a0, s10
	call        SplitValue

	// *** Basic block 80

	lw          a3, -24(s0)
	li          t0, 20		// 0x14 ASCII \x14
	mv          a2, t0
	li          t0, 12		// 0xc ASCII \xc
	mv          a1, t0
	mv          a0, s1
	call        SetBitField32

	// *** Basic block 81

	j           .ApplyRelocation_label_142

	// *** Basic block 82

.ApplyRelocation_label_288:

	// *** Basic block 83

.ApplyRelocation_label_289:
	mv          a1, s2
	mv          a0, s4
	call        FindRelocation

	// *** Basic block 84

	mv          s7, a0
	bne         s7, x0, .ApplyRelocation_label_307

	// *** Basic block 85

	lla         a1, .str.1
	mv          a0, s5
	call        LinkerError

	// *** Basic block 86

	j           .ApplyRelocation_label_142

	// *** Basic block 87

.ApplyRelocation_label_307:
	ld          s10, 16(s7)
	mv          a1, s10
	mv          a0, s5
	call        ObjectFileFindSymbol

	// *** Basic block 88

	mv          s6, a0
	bne         s6, x0, .ApplyRelocation_label_328

	// *** Basic block 89

	lla         a1, .str.2
	mv          a2, s10
	mv          a0, s5
	call        LinkerError

	// *** Basic block 90

	j           .ApplyRelocation_label_142

	// *** Basic block 91

.ApplyRelocation_label_328:
	ld          t0, 64(s7)
	ld          t1, -32(s0)	// Spilled @74
	sub         t0, t1, t0
	sext.w      s10, t0
	ld          s2, 80(s6)
	lw          t0, 56(s7)
	li          s11, 20		// 0x14 ASCII \x14
	bne         t0, s11, .ApplyRelocation_label_367

	// *** Basic block 92

	ld          t0, 536(s4)
	ld          t0, 256(t0)
	ld          s7, 96(t0)
	lw          t0, 88(s6)
	slli        t0, t0, 3
	add         t0, s7, t0
	add         s7, t0, s3
	sub         t0, s7, s8
	add         t0, t0, s10
	sext.w      s7, t0
	addi        a1, s0, -24
	addi        a2, s0, -20
	mv          a0, s7
	call        SplitValue

	// *** Basic block 93

	j           .ApplyRelocation_label_380

	// *** Basic block 94

.ApplyRelocation_label_367:
	sub         t0, s2, s8
	add         t0, t0, s10
	sub         t0, t0, s3
	sext.w      s7, t0
	addi        a1, s0, -24
	addi        a2, s0, -20
	mv          a0, s7
	call        SplitValue

	// *** Basic block 95

.ApplyRelocation_label_380:
	li          s7, 25		// 0x19 ASCII \x19
	bne         s9, s7, .ApplyRelocation_label_410

	// *** Basic block 96

	lw          s8, -20(s0)
	mv          a3, s8
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	li          s10, 7		// 0x7 ASCII \x7
	mv          a1, s10
	mv          a0, s1
	call        SetBitField32

	// *** Basic block 97

	srai        a3, s8, 5
	mv          a2, s10
	mv          a1, s7
	mv          a0, s1
	call        SetBitField32

	// *** Basic block 98

	j           .ApplyRelocation_label_422

	// *** Basic block 99

.ApplyRelocation_label_410:
	lw          a3, -20(s0)
	li          t0, 12		// 0xc ASCII \xc
	mv          a2, t0
	mv          a1, s11
	mv          a0, s1
	call        SetBitField32

	// *** Basic block 100

.ApplyRelocation_label_422:
	j           .ApplyRelocation_label_142

	// *** Basic block 101

.ApplyRelocation_label_424:
	ld          t0, 536(s4)
	ld          t0, 272(t0)
	ld          s10, 96(t0)
	lw          t0, 92(s6)
	slli        t0, t0, 4
	add         t0, s10, t0
	add         s10, t0, s3
	sub         t0, s10, s8
	sext.w      s10, t0
	addi        a1, s0, -24
	addi        a2, s0, -20
	mv          a0, s10
	call        SplitValue

	// *** Basic block 102

	lw          a3, -24(s0)
	li          s10, 20		// 0x14 ASCII \x14
	mv          a2, s10
	li          s11, 12		// 0xc ASCII \xc
	mv          a1, s11
	mv          a0, s1
	call        SetBitField32

	// *** Basic block 103

	addi        a0, s1, 4
	lw          a3, -20(s0)
	mv          a2, s11
	mv          a1, s10
	call        SetBitField32

	// *** Basic block 104

	j           .ApplyRelocation_label_142

	// *** Basic block 105

.ApplyRelocation_label_471:
	ld          t0, 536(s4)
	ld          t0, 264(t0)
	ld          s10, 96(t0)
	lw          t0, 88(s6)
	slli        t0, t0, 3
	add         t0, s10, t0
	add         s10, t0, s3
	sub         t0, s10, s8
	sext.w      s10, t0
	mv          a0, s10
	call        High20

	// *** Basic block 106

	sw          a0, -24(s0)
	lw          a3, -24(s0)
	li          t0, 20		// 0x14 ASCII \x14
	mv          a2, t0
	li          t0, 12		// 0xc ASCII \xc
	mv          a1, t0
	mv          a0, s1
	call        SetBitField32

	// *** Basic block 107

	j           .ApplyRelocation_label_142

	// *** Basic block 108

.ApplyRelocation_label_504:
	add         a0, s2, s3
	call        High20

	// *** Basic block 109

	sw          a0, -24(s0)
	lw          a3, -24(s0)
	li          t0, 20		// 0x14 ASCII \x14
	mv          a2, t0
	li          t0, 12		// 0xc ASCII \xc
	mv          a1, t0
	mv          a0, s1
	call        SetBitField32

	// *** Basic block 110

	j           .ApplyRelocation_label_142

	// *** Basic block 111

.ApplyRelocation_label_521:
	add         s10, s2, s3
	mv          a0, s10
	call        High20

	// *** Basic block 112

	sw          a0, -24(s0)
	lw          a1, -24(s0)
	mv          a0, s10
	call        Low12

	// *** Basic block 113

	sw          a0, -20(s0)
	lw          a3, -20(s0)
	li          t0, 12		// 0xc ASCII \xc
	mv          a2, t0
	li          t0, 20		// 0x14 ASCII \x14
	mv          a1, t0
	mv          a0, s1
	call        SetBitField32

	// *** Basic block 114

	j           .ApplyRelocation_label_142

	// *** Basic block 115

.ApplyRelocation_label_545:
	add         a0, s2, s3
	addi        a1, s0, -24
	addi        a2, s0, -20
	call        SplitValue

	// *** Basic block 116

	lw          s10, -20(s0)
	mv          a3, s10
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	li          s11, 7		// 0x7 ASCII \x7
	mv          a1, s11
	mv          a0, s1
	call        SetBitField32

	// *** Basic block 117

	srai        a3, s10, 5
	mv          a2, s11
	li          t0, 25		// 0x19 ASCII \x19
	mv          a1, t0
	mv          a0, s1
	call        SetBitField32

	// *** Basic block 118

	j           .ApplyRelocation_label_142

	// *** Basic block 119

.ApplyRelocation_label_577:
	add         t0, s2, s3
	sext.w      a3, t0
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        AddBitField32

	// *** Basic block 120

	j           .ApplyRelocation_label_142

	// *** Basic block 121

.ApplyRelocation_label_590:
	add         t0, s2, s3
	sext.w      a3, t0
	li          t0, 16		// 0x10 ASCII \x10
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        AddBitField32

	// *** Basic block 122

	j           .ApplyRelocation_label_142

	// *** Basic block 123

.ApplyRelocation_label_603:
	add         t0, s2, s3
	sext.w      t0, t0
	lw          t1, 0(s1)
	add         t0, t1, t0
	sw          t0, 0(s1)
	j           .ApplyRelocation_label_142

	// *** Basic block 124

.ApplyRelocation_label_610:
	add         t0, s2, s3
	ld          t1, 0(s1)
	add         t0, t1, t0
	sd          t0, 0(s1)
	j           .ApplyRelocation_label_142

	// *** Basic block 125

.ApplyRelocation_label_616:
	add         t0, s2, s3
	sext.w      a3, t0
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        SubBitField32

	// *** Basic block 126

	j           .ApplyRelocation_label_142

	// *** Basic block 127

.ApplyRelocation_label_629:
	add         t0, s2, s3
	sext.w      a3, t0
	li          t0, 16		// 0x10 ASCII \x10
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        SubBitField32

	// *** Basic block 128

	j           .ApplyRelocation_label_142

	// *** Basic block 129

.ApplyRelocation_label_642:
	add         t0, s2, s3
	sext.w      t0, t0
	lw          t1, 0(s1)
	sub         t0, t1, t0
	sw          t0, 0(s1)
	j           .ApplyRelocation_label_142

	// *** Basic block 130

.ApplyRelocation_label_649:
	add         t0, s2, s3
	ld          t1, 0(s1)
	sub         t0, t1, t0
	sd          t0, 0(s1)
	j           .ApplyRelocation_label_142

	// *** Basic block 131

.ApplyRelocation_label_655:
	j           .ApplyRelocation_label_665

	// *** Basic block 132

.ApplyRelocation_label_657:
	j           .ApplyRelocation_label_665

	// *** Basic block 133

.ApplyRelocation_label_659:
	j           .ApplyRelocation_label_665

	// *** Basic block 134

.ApplyRelocation_label_661:
	j           .ApplyRelocation_label_665

	// *** Basic block 135

.ApplyRelocation_label_663:
	j           .ApplyRelocation_label_665

	// *** Basic block 136

.ApplyRelocation_label_665:
	lla         a1, .str.3
	mv          a2, s9
	mv          a0, s5
	call        LinkerError

	// *** Basic block 137

	j           .ApplyRelocation_label_142
.func_end_ApplyRelocation:
	.size ApplyRelocation, .func_end_ApplyRelocation-ApplyRelocation

	.local  InitDynamicLinker
	.type InitDynamicLinker, @function

InitDynamicLinker:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, 0(a0)
	li          t1, 8		// 0x8 ASCII \x8
	sw          t1, 4(a0)
	sw          t0, 104(a0)
	addi        t0, a0, 104
	li          t1, 16		// 0x10 ASCII \x10
	sw          t1, 4(t0)
	ret         
.func_end_InitDynamicLinker:
	.size InitDynamicLinker, .func_end_InitDynamicLinker-InitDynamicLinker

	.local  AddGOTEntry
	.type AddGOTEntry, @function

AddGOTEntry:

	// *** Basic block 0

	.global BufferAppendLongLE
	.global NewSymbolRelocation
	.global VectorAppend
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// End of stack frame
	mv          s1, a2
	mv          s2, a1
	mv          s3, a3
	addi        t0, s1, 8
	ld          s4, 8(t0)
	slli        t0, a4, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .AddGOTEntry_label_43

	// *** Basic block 2

	j           .AddGOTEntry_label_39

	// *** Basic block 3

	j           .AddGOTEntry_label_46

	// *** Basic block 4

	j           .AddGOTEntry_label_49

	// *** Basic block 5

.AddGOTEntry_label_39:
	li          s5, 5		// 0x5 ASCII \x5
	j           .AddGOTEntry_label_52

	// *** Basic block 6

.AddGOTEntry_label_43:
	li          s5, 2		// 0x2 ASCII \x2
	j           .AddGOTEntry_label_52

	// *** Basic block 7

.AddGOTEntry_label_46:
	li          s5, 9		// 0x9 ASCII \x9
	j           .AddGOTEntry_label_52

	// *** Basic block 8

.AddGOTEntry_label_49:
	li          s5, 7		// 0x7 ASCII \x7
	j           .AddGOTEntry_label_52

	// *** Basic block 9

.AddGOTEntry_label_52:
	addi        a0, s1, 8
	mv          a1, x0
	call        BufferAppendLongLE

	// *** Basic block 10

	mv          a3, x0
	mv          a2, s5
	mv          a1, s4
	mv          a0, s2
	call        NewSymbolRelocation

	// *** Basic block 11

	mv          s4, a0
	mv          a1, s4
	mv          a0, s3
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorAppend
.func_end_AddGOTEntry:
	.size AddGOTEntry, .func_end_AddGOTEntry-AddGOTEntry

	.local  FixupGOTEntry
	.type FixupGOTEntry, @function

FixupGOTEntry:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 0(a1)
	lw          t1, 88(a0)
	slli        t1, t1, 3
	add         t2, t0, t1
	sd          a2, 0(t2)
	ret         
.func_end_FixupGOTEntry:
	.size FixupGOTEntry, .func_end_FixupGOTEntry-FixupGOTEntry

	.local  AddPLTEntry
	.type AddPLTEntry, @function

AddPLTEntry:

	// *** Basic block 0

	.local UTypeInstruction
	.global BufferAppendWordLE
	.local ITypeInstruction
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
	mv          s1, a2
	mv          a2, x0
	li          s2, 28		// 0x1c ASCII \x1c
	mv          a1, s2
	li          a0, 23		// 0x17 ASCII \x17
	call        UTypeInstruction

	// *** Basic block 1

	mv          s3, a0
	addi        a0, s1, 8
	mv          a1, s3
	call        BufferAppendWordLE

	// *** Basic block 2

	mv          a4, x0
	li          t0, 3		// 0x3 ASCII \x3
	mv          a3, t0
	mv          a2, s2
	mv          a1, s2
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 3

	mv          s3, a0
	addi        a0, s1, 8
	mv          a1, s3
	call        BufferAppendWordLE

	// *** Basic block 4

	mv          a4, x0
	mv          a3, x0
	mv          a2, s2
	li          t0, 6		// 0x6 ASCII \x6
	mv          a1, t0
	li          t0, 103		// 0x67 ASCII 'g'
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 5

	mv          s3, a0
	addi        a0, s1, 8
	mv          a1, s3
	call        BufferAppendWordLE

	// *** Basic block 6

	addi        a0, s1, 8
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           BufferAppendWordLE
.func_end_AddPLTEntry:
	.size AddPLTEntry, .func_end_AddPLTEntry-AddPLTEntry

	.local  SetupResolverPLTEntry
	.type SetupResolverPLTEntry, @function

SetupResolverPLTEntry:

	// *** Basic block 0

	.local SplitValue
	.local UTypeInstruction
	.local RTypeInstruction
	.local ITypeInstruction
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -24(s0)
	// Saved integer registers.
	sd s1, 48(sp)
	sd s2, 40(sp)
	sd s3, 32(sp)
	sd s4, 24(sp)
	sd s5, 16(sp)
	sd s6, 8(sp)
	sd s7, 0(sp)
	// End of stack frame
	mv          t0, a1
	mv          t1, a2
	ld          s1, 0(t0)
	sub         t2, t1, a3
	sext.w      t3, t2
	addi        a1, s0, -24
	addi        a2, s0, -20
	mv          a0, t3
	call        SplitValue

	// *** Basic block 1

	lw          a2, -24(s0)
	li          s2, 7		// 0x7 ASCII \x7
	mv          a1, s2
	li          t0, 23		// 0x17 ASCII \x17
	mv          a0, t0
	call        UTypeInstruction

	// *** Basic block 2

	sw          a0, 0(s1)
	li          t0, 32		// 0x20 ASCII ' '
	mv          a5, t0
	mv          a4, x0
	li          s3, 28		// 0x1c ASCII \x1c
	mv          a3, s3
	li          s4, 6		// 0x6 ASCII \x6
	mv          a2, s4
	mv          a1, s4
	li          t0, 51		// 0x33 ASCII '3'
	mv          a0, t0
	call        RTypeInstruction

	// *** Basic block 3

	sw          a0, 4(s1)
	lw          s5, -20(s0)
	mv          a4, s5
	li          s6, 3		// 0x3 ASCII \x3
	mv          a3, s6
	mv          a2, s2
	mv          a1, s3
	mv          a0, s6
	call        ITypeInstruction

	// *** Basic block 4

	sw          a0, 8(s1)
	li          t0, -44		// 0xffffffffffffffd4
	mv          a4, t0
	mv          a3, x0
	mv          a2, s4
	mv          a1, s4
	li          s7, 19		// 0x13 ASCII \x13
	mv          a0, s7
	call        ITypeInstruction

	// *** Basic block 5

	sw          a0, 12(s1)
	mv          a4, s5
	mv          a3, x0
	mv          a2, s2
	li          s2, 5		// 0x5 ASCII \x5
	mv          a1, s2
	mv          a0, s7
	call        ITypeInstruction

	// *** Basic block 6

	sw          a0, 16(s1)
	mv          a5, x0
	mv          a4, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, s4
	mv          a1, s4
	mv          a0, s7
	call        RTypeInstruction

	// *** Basic block 7

	sw          a0, 20(s1)
	li          t0, 8		// 0x8 ASCII \x8
	mv          a4, t0
	mv          a3, s6
	mv          a2, s2
	mv          a1, s2
	mv          a0, s6
	call        ITypeInstruction

	// *** Basic block 8

	sw          a0, 24(s1)
	mv          a4, x0
	mv          a3, x0
	mv          a2, s3
	mv          a1, x0
	li          t0, 103		// 0x67 ASCII 'g'
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 9

	sw          a0, 28(s1)
	// Restored registers.
	ld s1, 48(sp)
	ld s2, 40(sp)
	ld s3, 32(sp)
	ld s4, 24(sp)
	ld s5, 16(sp)
	ld s6, 8(sp)
	ld s7, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SetupResolverPLTEntry:
	.size SetupResolverPLTEntry, .func_end_SetupResolverPLTEntry-SetupResolverPLTEntry

	.local  FixupPLTEntry
	.type FixupPLTEntry, @function

FixupPLTEntry:

	// *** Basic block 0

	.local SplitValue
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -24(s0)
	// Saved integer registers.
	sd s1, 0(sp)
	// End of stack frame
	mv          t0, a2
	mv          t1, a0
	mv          t2, a1
	lw          t3, 92(t0)
	lw          t4, 4(t1)
	mul         t5, t3, t4
	lw          t3, 88(t0)
	lw          t4, 4(t2)
	mul         t3, t3, t4
	add         t4, a4, t3
	add         t3, a5, t5
	ld          t6, 0(a3)
	add         s1, t6, t5
	sub         t5, t4, t3
	addi        a1, s0, -24
	addi        a2, s0, -20
	mv          a0, t5
	call        SplitValue

	// *** Basic block 1

	lw          t0, -24(s0)
	slli        t0, t0, 12
	lwu         t1, 0(s1)
	or          t0, t1, t0
	sw          t0, 0(s1)
	lw          t0, -20(s0)
	slli        t0, t0, 20
	lwu         t1, 4(s1)
	or          t0, t1, t0
	sw          t0, 4(s1)
	// Restored registers.
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_FixupPLTEntry:
	.size FixupPLTEntry, .func_end_FixupPLTEntry-FixupPLTEntry

	.global NewRISCVLinkerArchitecture
	.type NewRISCVLinkerArchitecture, @function

NewRISCVLinkerArchitecture:

	// *** Basic block 0

	.global malloc
	.local CodeStartAddress
	.local DataStartAddress
	.local HandlePICRelocation
	.local ApplyRelocation
	.local InitDynamicLinker
	.local AddGOTEntry
	.local FixupGOTEntry
	.local AddPLTEntry
	.local SetupResolverPLTEntry
	.local FixupPLTEntry
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	li          a0, 88		// 0x58 ASCII 'X'
	call        malloc

	// *** Basic block 1

	mv          s1, a0
	la          t0, CodeStartAddress
	sd          t0, 8(s1)
	la          t0, DataStartAddress
	sd          t0, 16(s1)
	li          t0, 243		// 0xf3 ASCII \xf3
	sw          t0, 0(s1)
	la          t0, HandlePICRelocation
	sd          t0, 32(s1)
	la          t0, ApplyRelocation
	sd          t0, 40(s1)
	la          t0, InitDynamicLinker
	sd          t0, 24(s1)
	la          t0, AddGOTEntry
	sd          t0, 48(s1)
	la          t0, FixupGOTEntry
	sd          t0, 56(s1)
	la          t0, AddPLTEntry
	sd          t0, 64(s1)
	la          t0, SetupResolverPLTEntry
	sd          t0, 72(s1)
	la          t0, FixupPLTEntry
	sd          t0, 80(s1)
	mv          a0, s1

	// *** Basic block 2

.NewRISCVLinkerArchitecture_label_67:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewRISCVLinkerArchitecture:
	.size NewRISCVLinkerArchitecture, .func_end_NewRISCVLinkerArchitecture-NewRISCVLinkerArchitecture

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "Unable to locate R_RISCV_PCREL_HI20 relocation"
	.type .str.1, @object
	.size .str.1, 47

.str.2:
	.asciz "Undefined symbol %s"
	.type .str.2, @object
	.size .str.2, 20

.str.3:
	.asciz "Unsupported RISC-V relocation type %d"
	.type .str.3, @object
	.size .str.3, 38

