	.file   "6502/6502_emitter.c"
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

	beqz        a0, .IsPrintable_label_32

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.IsPrintable_label_29:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.IsPrintable_label_32:
	lw          t0, 16(s1)
	li          t1, 3		// 0x3 ASCII \x3
	beq         t0, t1, .IsPrintable_label_110

	// *** Basic block 5

	li          t1, 4		// 0x4 ASCII \x4
	beq         t0, t1, .IsPrintable_label_105

	// *** Basic block 6

	li          t1, 21		// 0x15 ASCII \x15
	beq         t0, t1, .IsPrintable_label_115

	// *** Basic block 7

	li          t1, 23		// 0x17 ASCII \x17
	beq         t0, t1, .IsPrintable_label_106

	// *** Basic block 8

	li          t1, 24		// 0x18 ASCII \x18
	beq         t0, t1, .IsPrintable_label_107

	// *** Basic block 9

	li          t1, 25		// 0x19 ASCII \x19
	beq         t0, t1, .IsPrintable_label_109

	// *** Basic block 10

	li          t1, 26		// 0x1a ASCII \x1a
	beq         t0, t1, .IsPrintable_label_112

	// *** Basic block 11

	li          t1, 27		// 0x1b ASCII \x1b
	beq         t0, t1, .IsPrintable_label_113

	// *** Basic block 12

	li          t1, 28		// 0x1c ASCII \x1c
	beq         t0, t1, .IsPrintable_label_114

	// *** Basic block 13

	li          t1, 29		// 0x1d ASCII \x1d
	beq         t0, t1, .IsPrintable_label_111

	// *** Basic block 14

	li          t1, 47		// 0x2f ASCII '/'
	beq         t0, t1, .IsPrintable_label_116

	// *** Basic block 15

	li          t1, 48		// 0x30 ASCII '0'
	beq         t0, t1, .IsPrintable_label_117

	// *** Basic block 16

	li          t1, 49		// 0x31 ASCII '1'
	beq         t0, t1, .IsPrintable_label_108

	// *** Basic block 17

.IsPrintable_label_100:

	// *** Basic block 18

.IsPrintable_label_101:
	li          a0, 1		// 0x1 ASCII \x1
	j           .IsPrintable_label_29

	// *** Basic block 19

.IsPrintable_label_105:

	// *** Basic block 20

.IsPrintable_label_106:

	// *** Basic block 21

.IsPrintable_label_107:

	// *** Basic block 22

.IsPrintable_label_108:

	// *** Basic block 23

.IsPrintable_label_109:

	// *** Basic block 24

.IsPrintable_label_110:

	// *** Basic block 25

.IsPrintable_label_111:

	// *** Basic block 26

.IsPrintable_label_112:

	// *** Basic block 27

.IsPrintable_label_113:

	// *** Basic block 28

.IsPrintable_label_114:

	// *** Basic block 29

.IsPrintable_label_115:

	// *** Basic block 30

.IsPrintable_label_116:

	// *** Basic block 31

.IsPrintable_label_117:
	mv          a0, x0
	j           .IsPrintable_label_29
.func_end_IsPrintable:
	.size IsPrintable, .func_end_IsPrintable-IsPrintable

	.local  GenerateRegisterMask
	.type GenerateRegisterMask, @function

GenerateRegisterMask:

	// *** Basic block 0

	.global BitSetExpand
	.global VectorClear
	.global VectorDestruct
	.global fprintf
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -48(s0)
	// Saved integer registers.
	sd s1, 56(sp)
	sd s2, 48(sp)
	sd s3, 40(sp)
	sd s4, 32(sp)
	sd s5, 24(sp)
	sd s6, 16(sp)
	sd s7, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, x0
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -48(s0)
	ld          t0, 8(s1)
	addi        a0, t0, 872
	addi        a1, s0, -48
	call        BitSetExpand

	// *** Basic block 1

	addi        t0, s0, -48
	ld          t0, 8(t0)
	bge         x0, t0, .GenerateRegisterMask_label_53

	// *** Basic block 2

	mv          s3, x0

	// *** Basic block 3

.GenerateRegisterMask_label_53:
	addi        a0, s0, -48
	call        VectorClear

	// *** Basic block 4

	ld          t0, 8(s1)
	addi        a0, t0, 888
	addi        a1, s0, -48
	call        BitSetExpand

	// *** Basic block 5

	ld          s4, -48(s0)
	li          s5, 1		// 0x1 ASCII \x1
	addi        t0, s0, -48
	ld          s6, 8(t0)
	li          s7, 1		// 0x1 ASCII \x1
	bge         s7, s6, .GenerateRegisterMask_label_91

	// *** Basic block 6

.GenerateRegisterMask_label_74:
	slli        t0, s5, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	addi        t0, s4, 1
	sll         t0, s7, t0
	slli        t0, t0, 48
	srai        t0, t0, 48
	or          s3, s3, t0

	// *** Basic block 7

.GenerateRegisterMask_label_87:
	addi        s5, s5, 1
	bge         s5, s6, .GenerateRegisterMask_label_74

	// *** Basic block 8

.GenerateRegisterMask_label_91:
	addi        a0, s0, -48
	call        VectorClear

	// *** Basic block 9

	ld          t0, 8(s1)
	addi        a0, t0, 904
	addi        a1, s0, -48
	call        BitSetExpand

	// *** Basic block 10

	ld          s4, -48(s0)
	li          s5, 1		// 0x1 ASCII \x1
	addi        t0, s0, -48
	ld          s6, 8(t0)
	bge         s7, s6, .GenerateRegisterMask_label_127

	// *** Basic block 11

.GenerateRegisterMask_label_112:
	slli        t0, s5, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	addi        t0, s4, 4
	sll         t0, s7, t0
	slli        t0, t0, 48
	srai        t0, t0, 48
	or          s3, s3, t0

	// *** Basic block 12

.GenerateRegisterMask_label_123:
	addi        s5, s5, 1
	bge         s5, s6, .GenerateRegisterMask_label_112

	// *** Basic block 13

.GenerateRegisterMask_label_127:
	addi        a0, s0, -48
	call        VectorClear

	// *** Basic block 14

	ld          t0, 8(s1)
	addi        a0, t0, 920
	addi        a1, s0, -48
	call        BitSetExpand

	// *** Basic block 15

	ld          s4, -48(s0)
	li          s5, 1		// 0x1 ASCII \x1
	addi        t0, s0, -48
	ld          s6, 8(t0)
	bge         s7, s6, .GenerateRegisterMask_label_163

	// *** Basic block 16

.GenerateRegisterMask_label_148:
	slli        t0, s5, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	addi        t0, s4, 7
	sll         t0, s7, t0
	slli        t0, t0, 48
	srai        t0, t0, 48
	or          s3, s3, t0

	// *** Basic block 17

.GenerateRegisterMask_label_159:
	addi        s5, s5, 1
	bge         s5, s6, .GenerateRegisterMask_label_148

	// *** Basic block 18

.GenerateRegisterMask_label_163:
	addi        a0, s0, -48
	call        VectorClear

	// *** Basic block 19

	ld          t0, 8(s1)
	addi        a0, t0, 936
	addi        a1, s0, -48
	call        BitSetExpand

	// *** Basic block 20

	ld          s4, -48(s0)
	li          s5, 1		// 0x1 ASCII \x1
	addi        t0, s0, -48
	ld          s6, 8(t0)
	bge         s7, s6, .GenerateRegisterMask_label_199

	// *** Basic block 21

.GenerateRegisterMask_label_184:
	slli        t0, s5, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	addi        t0, s4, 10
	sll         t0, s7, t0
	slli        t0, t0, 48
	srai        t0, t0, 48
	or          s3, s3, t0

	// *** Basic block 22

.GenerateRegisterMask_label_195:
	addi        s5, s5, 1
	bge         s5, s6, .GenerateRegisterMask_label_184

	// *** Basic block 23

.GenerateRegisterMask_label_199:
	addi        a0, s0, -48
	call        VectorClear

	// *** Basic block 24

	ld          t0, 8(s1)
	addi        a0, t0, 952
	addi        a1, s0, -48
	call        BitSetExpand

	// *** Basic block 25

	ld          s4, -48(s0)
	li          s5, 1		// 0x1 ASCII \x1
	addi        t0, s0, -48
	ld          s6, 8(t0)
	bge         s7, s6, .GenerateRegisterMask_label_235

	// *** Basic block 26

.GenerateRegisterMask_label_220:
	slli        t0, s5, 3
	add         t0, s4, t0
	ld          s1, 0(t0)
	addi        t0, s1, 13
	sll         t0, s7, t0
	slli        t0, t0, 48
	srai        t0, t0, 48
	or          s3, s3, t0

	// *** Basic block 27

.GenerateRegisterMask_label_231:
	addi        s5, s5, 1
	bge         s5, s6, .GenerateRegisterMask_label_220

	// *** Basic block 28

.GenerateRegisterMask_label_235:
	addi        a0, s0, -48
	call        VectorDestruct

	// *** Basic block 29

	lla         a1, .str.1
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 30

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
.func_end_GenerateRegisterMask:
	.size GenerateRegisterMask, .func_end_GenerateRegisterMask-GenerateRegisterMask

	.local  RegisterSize
	.type RegisterSize, @function

RegisterSize:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 16(a0)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .RegisterSize_label_32

	// *** Basic block 2

	j           .RegisterSize_label_26

	// *** Basic block 3

	j           .RegisterSize_label_44

	// *** Basic block 4

	j           .RegisterSize_label_48

	// *** Basic block 5

	j           .RegisterSize_label_40

	// *** Basic block 6

	j           .RegisterSize_label_36

	// *** Basic block 7

.RegisterSize_label_26:
	li          a0, 2		// 0x2 ASCII \x2

	// *** Basic block 8

.RegisterSize_label_29:
	ret         

	// *** Basic block 9

.RegisterSize_label_32:
	li          a0, 1		// 0x1 ASCII \x1
	ret         

	// *** Basic block 10

.RegisterSize_label_36:
	li          a0, 8		// 0x8 ASCII \x8
	ret         

	// *** Basic block 11

.RegisterSize_label_40:
	li          a0, 4		// 0x4 ASCII \x4
	ret         

	// *** Basic block 12

.RegisterSize_label_44:
	li          a0, 4		// 0x4 ASCII \x4
	ret         

	// *** Basic block 13

.RegisterSize_label_48:
	li          a0, 8		// 0x8 ASCII \x8
	ret         
.func_end_RegisterSize:
	.size RegisterSize, .func_end_RegisterSize-RegisterSize

	.local  InitIndex
	.type InitIndex, @function

InitIndex:

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
	beqz        a0, .InitIndex_label_25

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.InitIndex_label_22:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.InitIndex_label_25:
	lw          t0, 20(s1)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 4

	j           .InitIndex_label_40

	// *** Basic block 5

	j           .InitIndex_label_38

	// *** Basic block 6

	j           .InitIndex_label_45

	// *** Basic block 7

	j           .InitIndex_label_46

	// *** Basic block 8

	j           .InitIndex_label_39

	// *** Basic block 9

.InitIndex_label_38:

	// *** Basic block 10

.InitIndex_label_39:

	// *** Basic block 11

.InitIndex_label_40:

	// *** Basic block 12

.InitIndex_label_41:
	mv          a0, x0
	j           .InitIndex_label_22

	// *** Basic block 13

.InitIndex_label_45:

	// *** Basic block 14

.InitIndex_label_46:
	lla         a1, .str.2
	lh          a2, 24(s1)
	mv          a0, s2
	call        fprintf

	// *** Basic block 15

	li          a0, 1		// 0x1 ASCII \x1
	j           .InitIndex_label_22
.func_end_InitIndex:
	.size InitIndex, .func_end_InitIndex-InitIndex

	.local  IncrementIndex
	.type IncrementIndex, @function

IncrementIndex:

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
	// End of stack frame
	mv          t0, a1
	mv          t1, a2
	mv          t2, a3
	mv          s1, a4
	beqz        a0, .IncrementIndex_label_30

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.IncrementIndex_label_27:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.IncrementIndex_label_30:
	lw          t3, 20(t0)
	slli        t3, t3, 2
	auipc       t4, 0
	add         t3, t4, t3
	jalr        x0, t3, 12

	// *** Basic block 4

	j           .IncrementIndex_label_45

	// *** Basic block 5

	j           .IncrementIndex_label_44

	// *** Basic block 6

	j           .IncrementIndex_label_48

	// *** Basic block 7

	j           .IncrementIndex_label_49

	// *** Basic block 8

	j           .IncrementIndex_label_46

	// *** Basic block 9

.IncrementIndex_label_44:

	// *** Basic block 10

.IncrementIndex_label_45:

	// *** Basic block 11

.IncrementIndex_label_46:
	j           .IncrementIndex_label_65

	// *** Basic block 12

.IncrementIndex_label_48:

	// *** Basic block 13

.IncrementIndex_label_49:
	addi        t0, t2, -1
	bge         t1, t0, .IncrementIndex_label_63

	// *** Basic block 14

	lla         a1, .str.3
	mv          a0, s1
	call        fprintf

	// *** Basic block 15

	li          a0, 1		// 0x1 ASCII \x1
	j           .IncrementIndex_label_27

	// *** Basic block 16

.IncrementIndex_label_63:
	j           .IncrementIndex_label_65

	// *** Basic block 17

.IncrementIndex_label_65:
	mv          a0, x0
	j           .IncrementIndex_label_27
.func_end_IncrementIndex:
	.size IncrementIndex, .func_end_IncrementIndex-IncrementIndex

	.local  LoadByte
	.type LoadByte, @function

LoadByte:

	// *** Basic block 0

	.global fprintf
	.global W65C02RegisterAsString
	.local buf
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
	mv          s1, a2
	lla         s2, .str.4
	lla         a2, buf
	li          a3, 4096		// 0x1000
	call        W65C02RegisterAsString

	// *** Basic block 1

	mv          a2, a0
	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           fprintf
.func_end_LoadByte:
	.size LoadByte, .func_end_LoadByte-LoadByte

	.local  OperateByte
	.type OperateByte, @function

OperateByte:

	// *** Basic block 0

	.global fprintf
	.global W65C02RegisterAsString
	.local buf
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
	mv          s1, a3
	mv          s2, a2
	lla         s3, .str.5
	lla         a2, buf
	li          a3, 4096		// 0x1000
	call        W65C02RegisterAsString

	// *** Basic block 1

	mv          a3, a0
	mv          a2, s2
	mv          a1, s3
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           fprintf
.func_end_OperateByte:
	.size OperateByte, .func_end_OperateByte-OperateByte

	.local  StoreByte
	.type StoreByte, @function

StoreByte:

	// *** Basic block 0

	.global fprintf
	.global W65C02RegisterAsString
	.local buf
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
	mv          s1, a2
	lla         s2, .str.6
	lla         a2, buf
	li          a3, 4096		// 0x1000
	call        W65C02RegisterAsString

	// *** Basic block 1

	mv          a2, a0
	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           fprintf
.func_end_StoreByte:
	.size StoreByte, .func_end_StoreByte-StoreByte

	.local  StoreZero
	.type StoreZero, @function

StoreZero:

	// *** Basic block 0

	.global fprintf
	.global W65C02RegisterAsString
	.local buf
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
	mv          s2, a2
	mv          s3, a1
	lw          s4, 20(s1)
	addi        t1, s4, -2
	seqz        t0, t1
	li          t1, 2		// 0x2 ASCII \x2
	beq         s4, t1, .StoreZero_label_32

	// *** Basic block 1

	addi        t1, s4, -3
	seqz        t0, t1

	// *** Basic block 2

.StoreZero_label_32:
	beqz        t0, .StoreZero_label_55

	// *** Basic block 3

	lla         s4, .str.7
	lla         a2, buf
	li          t0, 4096		// 0x1000
	mv          a3, t0
	mv          a1, s3
	mv          a0, s1
	call        W65C02RegisterAsString

	// *** Basic block 4

	mv          a2, a0
	mv          a1, s4
	mv          a0, s2
	call        fprintf

	// *** Basic block 5

	j           .StoreZero_label_76

	// *** Basic block 6

.StoreZero_label_55:
	lla         s4, .str.8
	lla         a2, buf
	li          t0, 4096		// 0x1000
	mv          a3, t0
	mv          a1, s3
	mv          a0, s1
	call        W65C02RegisterAsString

	// *** Basic block 7

	mv          a2, a0
	mv          a1, s4
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

	// *** Basic block 8

.StoreZero_label_76:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StoreZero:
	.size StoreZero, .func_end_StoreZero-StoreZero

	.local  CopyConstant
	.type CopyConstant, @function

CopyConstant:

	// *** Basic block 0

	.local RegisterSize
	.local InitIndex
	.global fprintf
	.local StoreZero
	.local StoreByte
	.local IncrementIndex
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
	sd s8, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a2
	mv          s3, a1
	call        RegisterSize

	// *** Basic block 1

	lw          s4, 20(s1)
	addi        t0, s4, -3
	seqz        t1, t0
	addi        t0, s4, -2
	seqz        t1, t0
	mv          s5, a0
	mv          a2, s2
	mv          a1, s1
	mv          a0, x0
	call        InitIndex

	// *** Basic block 2

	li          s6, -1		// 0xffffffffffffffff
	mv          s7, x0
	bge         x0, s5, .CopyConstant_label_126

	// *** Basic block 3

.CopyConstant_label_55:
	slli        t0, s7, 3
	sra         t0, s3, t0
	andi        t0, t0, 255
	sext.w      s8, t0
	bnez        s8, .CopyConstant_label_87

	// *** Basic block 4

	li          t0, 3		// 0x3 ASCII \x3
	beq         s4, t0, .CopyConstant_label_66

	// *** Basic block 5

.CopyConstant_label_66:
	beqz        t1, .CopyConstant_label_78

	// *** Basic block 6

	beq         s6, s8, .CopyConstant_label_77

	// *** Basic block 7

	lla         a1, .str.9
	mv          a0, s2
	call        fprintf

	// *** Basic block 8

.CopyConstant_label_77:

	// *** Basic block 9

.CopyConstant_label_78:
	mv          a2, s2
	mv          a1, s7
	mv          a0, s1
	call        StoreZero

	// *** Basic block 10

	j           .CopyConstant_label_107

	// *** Basic block 11

.CopyConstant_label_87:
	beq         s6, s8, .CopyConstant_label_99

	// *** Basic block 12

	lla         a1, .str.10
	mv          a2, s8
	mv          a0, s2
	call        fprintf

	// *** Basic block 13

.CopyConstant_label_99:
	mv          a2, s2
	mv          a1, s7
	mv          a0, s1
	call        StoreByte

	// *** Basic block 14

.CopyConstant_label_107:
	mv          a4, s2
	mv          a3, s5
	mv          a2, s7
	mv          a1, s1
	mv          a0, x0
	call        IncrementIndex

	// *** Basic block 15

	mv          s6, s8

	// *** Basic block 16

.CopyConstant_label_122:
	addi        s7, s7, 1
	bge         s7, s5, .CopyConstant_label_55

	// *** Basic block 17

.CopyConstant_label_126:
	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	ld s8, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CopyConstant:
	.size CopyConstant, .func_end_CopyConstant-CopyConstant

	.local  CopyRegister
	.type CopyRegister, @function

CopyRegister:

	// *** Basic block 0

	.local InitIndex
	.local LoadByte
	.local StoreByte
	.local IncrementIndex
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
	mv          s1, a1
	mv          s2, a3
	mv          s3, a2
	mv          s4, a0
	mv          a2, s2
	mv          a0, x0
	call        InitIndex

	// *** Basic block 1

	mv          a2, s2
	mv          a1, s3
	call        InitIndex

	// *** Basic block 2

	mv          s5, x0
	bge         x0, s4, .CopyRegister_label_81

	// *** Basic block 3

.CopyRegister_label_39:
	mv          a2, s2
	mv          a1, s5
	mv          a0, s3
	call        LoadByte

	// *** Basic block 4

	mv          a2, s2
	mv          a1, s5
	mv          a0, s1
	call        StoreByte

	// *** Basic block 5

	mv          a4, s2
	mv          a3, s4
	mv          a2, s5
	mv          a1, s1
	mv          a0, x0
	call        IncrementIndex

	// *** Basic block 6

	mv          a4, s2
	mv          a3, s4
	mv          a2, s5
	mv          a1, s3
	call        IncrementIndex

	// *** Basic block 7

.CopyRegister_label_77:
	addi        s5, s5, 1
	bge         s5, s4, .CopyRegister_label_39

	// *** Basic block 8

.CopyRegister_label_81:
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
.func_end_CopyRegister:
	.size CopyRegister, .func_end_CopyRegister-CopyRegister

	.local  Copy
	.type Copy, @function

Copy:

	// *** Basic block 0

	.global TargetIsConst
	.global TargetIntValue
	.local CopyConstant
	.local CopyRegister
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
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a2
	mv          s2, a3
	mv          s3, a0
	ld          s4, 32(a1)
	mv          a0, s1
	call        TargetIsConst

	// *** Basic block 1

	beqz        a0, .Copy_label_40

	// *** Basic block 2

	mv          a0, s1
	call        TargetIntValue

	// *** Basic block 3

	mv          s5, a0
	mv          a2, s2
	mv          a1, s5
	mv          a0, s4
	call        CopyConstant

	// *** Basic block 4

	j           .Copy_label_54

	// *** Basic block 5

.Copy_label_40:
	ld          s6, 32(s1)
	mv          a3, s2
	mv          a2, s6
	mv          a1, s4
	mv          a0, s3
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld s6, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           CopyRegister

	// *** Basic block 6

.Copy_label_54:
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld s6, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Copy:
	.size Copy, .func_end_Copy-Copy

	.local  PrintRmov
	.type PrintRmov, @function

PrintRmov:

	// *** Basic block 0

	.global printf
	.global abort
	.local CopyRegister
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
	mv          s1, a1
	mv          s2, a2
	addi        s3, s1, 40
	ld          s4, 40(s1)
	beq         s4, x0, .PrintRmov_label_47

	// *** Basic block 1

	j           .PrintRmov_label_64

	// *** Basic block 2

.PrintRmov_label_47:
	lla         a0, .str.11
	lla         a1, .str.12
	lla         a3, .str.13
	li          t0, 240		// 0xf0 ASCII \xf0
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.PrintRmov_label_64:
	ld          s3, 8(s3)
	beq         s3, x0, .PrintRmov_label_71

	// *** Basic block 5

	j           .PrintRmov_label_86

	// *** Basic block 6

.PrintRmov_label_71:
	lla         a0, .str.14
	lla         a1, .str.15
	lla         a3, .str.16
	li          t0, 241		// 0xf1 ASCII \xf1
	mv          a2, t0
	call        printf

	// *** Basic block 7

	call        abort

	// *** Basic block 8

.PrintRmov_label_86:
	ld          a1, 32(s4)
	beq         a1, x0, .PrintRmov_label_93

	// *** Basic block 9

	j           .PrintRmov_label_108

	// *** Basic block 10

.PrintRmov_label_93:
	lla         a0, .str.17
	lla         a1, .str.18
	lla         a3, .str.19
	li          t0, 242		// 0xf2 ASCII \xf2
	mv          a2, t0
	call        printf

	// *** Basic block 11

	call        abort

	// *** Basic block 12

.PrintRmov_label_108:
	ld          a2, 32(s3)
	beq         a2, x0, .PrintRmov_label_115

	// *** Basic block 13

	j           .PrintRmov_label_130

	// *** Basic block 14

.PrintRmov_label_115:
	lla         a0, .str.20
	lla         a1, .str.21
	lla         a3, .str.22
	li          t0, 243		// 0xf3 ASCII \xf3
	mv          a2, t0
	call        printf

	// *** Basic block 15

	call        abort

	// *** Basic block 16

.PrintRmov_label_130:
	bne         a1, a2, .PrintRmov_label_137

	// *** Basic block 17

.PrintRmov_label_134:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 18

.PrintRmov_label_137:
	lw          s3, 16(s1)
	li          t0, 18		// 0x12 ASCII \x12
	beq         s3, t0, .PrintRmov_label_191

	// *** Basic block 19

	li          t0, 19		// 0x13 ASCII \x13
	beq         s3, t0, .PrintRmov_label_215

	// *** Basic block 20

	li          t0, 20		// 0x14 ASCII \x14
	beq         s3, t0, .PrintRmov_label_227

	// *** Basic block 21

	li          t0, 38		// 0x26 ASCII '&'
	beq         s3, t0, .PrintRmov_label_181

	// *** Basic block 22

	li          t0, 39		// 0x27 ASCII '''
	beq         s3, t0, .PrintRmov_label_203

	// *** Basic block 23

.PrintRmov_label_165:
	lla         a0, .str.23
	lla         a1, .str.24
	lla         a3, .str.25
	li          t0, 267		// 0x10b
	mv          a2, t0
	call        printf

	// *** Basic block 24

	call        abort

	// *** Basic block 25

	j           .PrintRmov_label_239

	// *** Basic block 26

.PrintRmov_label_181:
	mv          a3, s2
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        CopyRegister

	// *** Basic block 27

	j           .PrintRmov_label_239

	// *** Basic block 28

.PrintRmov_label_191:
	mv          a3, s2
	li          t0, 4		// 0x4 ASCII \x4
	mv          a0, t0
	call        CopyRegister

	// *** Basic block 29

	j           .PrintRmov_label_239

	// *** Basic block 30

.PrintRmov_label_203:
	mv          a3, s2
	li          t0, 8		// 0x8 ASCII \x8
	mv          a0, t0
	call        CopyRegister

	// *** Basic block 31

	j           .PrintRmov_label_239

	// *** Basic block 32

.PrintRmov_label_215:
	mv          a3, s2
	li          t0, 4		// 0x4 ASCII \x4
	mv          a0, t0
	call        CopyRegister

	// *** Basic block 33

	j           .PrintRmov_label_239

	// *** Basic block 34

.PrintRmov_label_227:
	mv          a3, s2
	li          t0, 8		// 0x8 ASCII \x8
	mv          a0, t0
	call        CopyRegister

	// *** Basic block 35

	j           .PrintRmov_label_239

	// *** Basic block 36

.PrintRmov_label_239:
	j           .PrintRmov_label_134
.func_end_PrintRmov:
	.size PrintRmov, .func_end_PrintRmov-PrintRmov

	.local  FindComparison
	.type FindComparison, @function

FindComparison:

	// *** Basic block 0

	.local comparisons
	.global abort
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          t0, a0
	mv          t1, x0

	// *** Basic block 1

.FindComparison_label_15:
	slli        s1, t1, 4
	la          t2, comparisons
	add         t2, t2, s1
	lw          t2, 0(t2)
	bne         t2, t0, .FindComparison_label_30

	// *** Basic block 2

	la          t0, comparisons
	add         a0, t0, s1

	// *** Basic block 3

.FindComparison_label_27:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.FindComparison_label_30:

	// *** Basic block 5

.FindComparison_label_31:
	addi        t1, t1, 1
	li          t0, 52		// 0x34 ASCII '4'
	bge         t1, t0, .FindComparison_label_15

	// *** Basic block 6

.FindComparison_label_36:
	call        abort

	// *** Basic block 7

	mv          a0, x0
	j           .FindComparison_label_27
.func_end_FindComparison:
	.size FindComparison, .func_end_FindComparison-FindComparison

	.local  Compare
	.type Compare, @function

Compare:

	// *** Basic block 0

	.global snprintf
	.local FindComparison
	.global fprintf
	.global abort
	.local LoadByte
	.local OperateByte
	.global W65C02RegisterAsString
	.local buf
	addi sp, sp, -224
	// Saved return address (offset 216) and frame pointer (offset 208)
	sd ra, 216(sp)
	sd s0, 208(sp)
	addi s0, sp, 224
	// Local vars at offset -112(s0)
	// Spilled register region: 24 bytes at -136(s0) to -112(s0)
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
	mv          s1, a1
	mv          s2, a0
	sd          s2, -136(s0)	// Spilled @53
	mv          s3, a2
	addi        s4, s0, -112
	lla         a2, .str.26
	lw          s5, 20(s2)
	mv          a4, s5
	mv          a3, s1
	li          s6, 32		// 0x20 ASCII ' '
	mv          a1, s6
	mv          a0, s4
	call        snprintf

	// *** Basic block 1

	addi        s7, s0, -80
	lla         a2, .str.27
	mv          a4, s5
	mv          a3, s1
	mv          a1, s6
	mv          a0, s7
	call        snprintf

	// *** Basic block 2

	lw          a0, 16(s2)
	call        FindComparison

	// *** Basic block 3

	mv          s5, a0
	lw          s8, 12(s5)
	slli        t0, s8, 20
	lb          s9, 8(s5)
	slli        t1, s9, 24
	or          t0, t0, t1
	lw          t1, 104(s2)
	sd          t1, -120(s0)	// Spilled @109
	or          t0, t1, t0
	sw          t0, 104(s2)
	addi        t0, s2, 40
	ld          s10, 40(s2)
	ld          s11, 8(t0)
	ld          t1, 32(s10)
	ld          s10, 32(s11)
	li          t0, 65536		// 0x10000
	ld          s11, -120(s0)	// Spilled @109
	and         t0, s11, t0
	sd          t0, -128(s0)	// Spilled @124
	snez        t0, t0
	beqz        t0, .Compare_label_135

	// *** Basic block 4

	lla         a1, .str.28
	mv          a0, s3
	call        fprintf

	// *** Basic block 5

.Compare_label_135:
	slli        t2, s8, 2
	auipc       t3, 0
	add         t2, t3, t2
	jalr        x0, t2, 12

	// *** Basic block 6

	j           .Compare_label_148

	// *** Basic block 7

	j           .Compare_label_151

	// *** Basic block 8

	j           .Compare_label_206

	// *** Basic block 9

	j           .Compare_label_263

	// *** Basic block 10

	j           .Compare_label_436

	// *** Basic block 11

	j           .Compare_label_261

	// *** Basic block 12

	j           .Compare_label_438

	// *** Basic block 13

.Compare_label_148:
	call        abort

	// *** Basic block 14

	j           .Compare_label_440

	// *** Basic block 15

.Compare_label_151:
	mv          s1, x0
	lw          s8, 4(s5)
	bge         x0, s8, .Compare_label_188

	// *** Basic block 16

.Compare_label_158:
	mv          a2, s3
	mv          a1, s1
	mv          a0, t1
	call        LoadByte

	// *** Basic block 17

	lla         a2, .str.29
	mv          a3, s3
	mv          a1, s1
	mv          a0, s10
	call        OperateByte

	// *** Basic block 18

	lla         a1, .str.30
	mv          a2, s4
	mv          a0, s3
	call        fprintf

	// *** Basic block 19

.Compare_label_184:
	addi        s1, s1, 1
	bge         s1, s8, .Compare_label_158

	// *** Basic block 20

.Compare_label_188:
	beqz        t0, .Compare_label_196

	// *** Basic block 21

	lla         a1, .str.31
	mv          a0, s3
	call        fprintf

	// *** Basic block 22

.Compare_label_196:
	lla         a1, .str.32
	mv          a2, s4
	mv          a0, s3
	call        fprintf

	// *** Basic block 23

	j           .Compare_label_440

	// *** Basic block 24

.Compare_label_206:
	mv          s8, x0
	lw          s11, 4(s5)
	bge         x0, s11, .Compare_label_243

	// *** Basic block 25

.Compare_label_213:
	mv          a2, s3
	mv          a1, s8
	mv          a0, t1
	call        LoadByte

	// *** Basic block 26

	lla         a2, .str.33
	mv          a3, s3
	mv          a1, s8
	mv          a0, s10
	call        OperateByte

	// *** Basic block 27

	lla         a1, .str.34
	mv          a2, s4
	mv          a0, s3
	call        fprintf

	// *** Basic block 28

.Compare_label_239:
	addi        s8, s8, 1
	bge         s8, s11, .Compare_label_213

	// *** Basic block 29

.Compare_label_243:
	beqz        t0, .Compare_label_251

	// *** Basic block 30

	lla         a1, .str.35
	mv          a0, s3
	call        fprintf

	// *** Basic block 31

.Compare_label_251:
	lla         a1, .str.36
	mv          a2, s4
	mv          a0, s3
	call        fprintf

	// *** Basic block 32

	j           .Compare_label_440

	// *** Basic block 33

.Compare_label_261:
	j           .Compare_label_440

	// *** Basic block 34

.Compare_label_263:
	beqz        s9, .Compare_label_359

	// *** Basic block 35

	lla         s9, .str.37
	addi        s11, s0, -48
	lla         a2, .str.38
	lw          a3, 20(s2)
	mv          a1, s6
	mv          a0, s11
	call        snprintf

	// *** Basic block 36

	mv          s6, x0
	lw          s2, 4(s5)
	bge         x0, s2, .Compare_label_311

	// *** Basic block 37

.Compare_label_288:
	mv          a2, s3
	mv          a1, s6
	mv          a0, t1
	call        LoadByte

	// *** Basic block 38

	mv          a3, s3
	mv          a2, s9
	mv          a1, s6
	mv          a0, s10
	call        OperateByte

	// *** Basic block 40

.Compare_label_307:
	addi        s6, s6, 1
	bge         s6, s2, .Compare_label_288

	// *** Basic block 41

.Compare_label_311:
	lla         a1, .str.40
	mv          a2, s11
	mv          a0, s3
	call        fprintf

	// *** Basic block 42

	lla         a1, .str.41
	mv          a0, s3
	call        fprintf

	// *** Basic block 43

	lla         a1, .str.42
	mv          a2, s11
	mv          a0, s3
	call        fprintf

	// *** Basic block 44

	beqz        t0, .Compare_label_357

	// *** Basic block 45

	lla         a1, .str.43
	mv          a2, s4
	mv          a0, s3
	call        fprintf

	// *** Basic block 46

	lla         a1, .str.44
	mv          a0, s3
	call        fprintf

	// *** Basic block 47

	lla         a1, .str.45
	mv          a2, s4
	mv          a0, s3
	call        fprintf

	// *** Basic block 48

.Compare_label_357:
	j           .Compare_label_434

	// *** Basic block 49

.Compare_label_359:
	lw          s2, 4(s5)
	blt         s2, x0, .Compare_label_409

	// *** Basic block 50

.Compare_label_366:
	mv          a2, s3
	mv          a1, s2
	mv          a0, t1
	call        LoadByte

	// *** Basic block 51

	lla         a2, .str.46
	mv          a3, s3
	mv          a1, s2
	mv          a0, s10
	call        OperateByte

	// *** Basic block 52

	lla         a1, .str.47
	mv          a2, s7
	mv          a0, s3
	call        fprintf

	// *** Basic block 53

	bge         x0, s2, .Compare_label_402

	// *** Basic block 54

	lla         a1, .str.48
	mv          a2, s7
	mv          a0, s3
	call        fprintf

	// *** Basic block 55

.Compare_label_402:

	// *** Basic block 56

.Compare_label_403:
	addi        s2, s2, -1
	blt         s2, x0, .Compare_label_366

	// *** Basic block 57

.Compare_label_409:
	lla         a1, .str.49
	mv          a2, s7
	mv          a0, s3
	call        fprintf

	// *** Basic block 58

	beqz        t0, .Compare_label_425

	// *** Basic block 59

	lla         a1, .str.50
	mv          a0, s3
	call        fprintf

	// *** Basic block 60

.Compare_label_425:
	lla         a1, .str.51
	mv          a2, s4
	mv          a0, s3
	call        fprintf

	// *** Basic block 61

.Compare_label_434:
	j           .Compare_label_440

	// *** Basic block 62

.Compare_label_436:
	j           .Compare_label_440

	// *** Basic block 63

.Compare_label_438:
	j           .Compare_label_440

	// *** Basic block 64

.Compare_label_440:
	beqz        t0, .Compare_label_462

	// *** Basic block 65

	lla         s2, .str.52
	ld          t0, -136(s0)	// Spilled @53
	ld          a0, 32(t0)
	lla         a2, buf
	li          t1, 4096		// 0x1000
	mv          a3, t1
	mv          a1, x0
	call        W65C02RegisterAsString

	// *** Basic block 66

	mv          a2, a0
	mv          a1, s2
	mv          a0, s3
	call        fprintf

	// *** Basic block 67

.Compare_label_462:
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
.func_end_Compare:
	.size Compare, .func_end_Compare-Compare

	.local  EmitBranch
	.type EmitBranch, @function

EmitBranch:

	// *** Basic block 0

	.global snprintf
	.global abort
	.global fprintf
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -48(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	sd s6, 0(sp)
	// End of stack frame
	mv          t0, a0
	mv          t1, a2
	mv          s1, a3
	mv          s2, a1
	ld          t2, 40(t0)
	lw          t3, 16(t0)
	addi        t3, t3, -166
	seqz        s3, t3
	lw          s4, 104(t2)
	srai        t2, s4, 24
	slli        t2, t2, 56
	srai        s5, t2, 56
	addi        s6, s0, -48
	lla         a2, .str.53
	lw          a4, 20(t0)
	mv          a3, t1
	li          a1, 32		// 0x20 ASCII ' '
	mv          a0, s6
	call        snprintf

	// *** Basic block 1

	srai        t0, s4, 20
	andi        s4, t0, 15
	beqz        s5, .EmitBranch_label_293

	// *** Basic block 2

	slli        t0, s4, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .EmitBranch_label_123

	// *** Basic block 4

	j           .EmitBranch_label_149

	// *** Basic block 5

	j           .EmitBranch_label_209

	// *** Basic block 6

	j           .EmitBranch_label_126

	// *** Basic block 7

	j           .EmitBranch_label_231

	// *** Basic block 8

	j           .EmitBranch_label_171

	// *** Basic block 9

	j           .EmitBranch_label_269

	// *** Basic block 10

.EmitBranch_label_123:
	call        abort

	// *** Basic block 11

	j           .EmitBranch_label_291

	// *** Basic block 12

.EmitBranch_label_126:
	beqz        s3, .EmitBranch_label_133

	// *** Basic block 13

	lla         s5, .str.54
	j           .EmitBranch_label_136

	// *** Basic block 14

.EmitBranch_label_133:
	lla         s5, .str.55

	// *** Basic block 15

.EmitBranch_label_136:
	lla         a1, .str.56
	mv          a3, s2
	mv          a2, s5
	mv          a0, s1
	call        fprintf

	// *** Basic block 16

	j           .EmitBranch_label_291

	// *** Basic block 17

.EmitBranch_label_149:
	beqz        s3, .EmitBranch_label_156

	// *** Basic block 18

	lla         s5, .str.57
	j           .EmitBranch_label_159

	// *** Basic block 19

.EmitBranch_label_156:
	lla         s5, .str.58

	// *** Basic block 20

.EmitBranch_label_159:
	lla         a1, .str.59
	mv          a3, s2
	mv          a2, s5
	mv          a0, s1
	call        fprintf

	// *** Basic block 21

	j           .EmitBranch_label_291

	// *** Basic block 22

.EmitBranch_label_171:
	beqz        s3, .EmitBranch_label_178

	// *** Basic block 23

	lla         s5, .str.60
	j           .EmitBranch_label_181

	// *** Basic block 24

.EmitBranch_label_178:
	lla         s5, .str.61

	// *** Basic block 25

.EmitBranch_label_181:
	lla         a1, .str.62
	mv          a2, s6
	mv          a0, s1
	call        fprintf

	// *** Basic block 26

	lla         a1, .str.63
	mv          a3, s2
	mv          a2, s5
	mv          a0, s1
	call        fprintf

	// *** Basic block 27

	lla         a1, .str.64
	mv          a2, s6
	mv          a0, s1
	call        fprintf

	// *** Basic block 28

	j           .EmitBranch_label_291

	// *** Basic block 29

.EmitBranch_label_209:
	beqz        s3, .EmitBranch_label_216

	// *** Basic block 30

	lla         s5, .str.65
	j           .EmitBranch_label_219

	// *** Basic block 31

.EmitBranch_label_216:
	lla         s5, .str.66

	// *** Basic block 32

.EmitBranch_label_219:
	lla         a1, .str.67
	mv          a3, s2
	mv          a2, s5
	mv          a0, s1
	call        fprintf

	// *** Basic block 33

	j           .EmitBranch_label_291

	// *** Basic block 34

.EmitBranch_label_231:
	beqz        s3, .EmitBranch_label_238

	// *** Basic block 35

	lla         s5, .str.68
	j           .EmitBranch_label_241

	// *** Basic block 36

.EmitBranch_label_238:
	lla         s5, .str.69

	// *** Basic block 37

.EmitBranch_label_241:
	lla         a1, .str.70
	mv          a2, s6
	mv          a0, s1
	call        fprintf

	// *** Basic block 38

	lla         a1, .str.71
	mv          a3, s2
	mv          a2, s5
	mv          a0, s1
	call        fprintf

	// *** Basic block 39

	lla         a1, .str.72
	mv          a2, s6
	mv          a0, s1
	call        fprintf

	// *** Basic block 40

	j           .EmitBranch_label_291

	// *** Basic block 41

.EmitBranch_label_269:
	beqz        s3, .EmitBranch_label_276

	// *** Basic block 42

	lla         s5, .str.73
	j           .EmitBranch_label_279

	// *** Basic block 43

.EmitBranch_label_276:
	lla         s5, .str.74

	// *** Basic block 44

.EmitBranch_label_279:
	lla         a1, .str.75
	mv          a3, s2
	mv          a2, s5
	mv          a0, s1
	call        fprintf

	// *** Basic block 45

	j           .EmitBranch_label_291

	// *** Basic block 46

.EmitBranch_label_291:
	j           .EmitBranch_label_473

	// *** Basic block 47

.EmitBranch_label_293:
	slli        t0, s4, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 48

	j           .EmitBranch_label_305

	// *** Basic block 49

	j           .EmitBranch_label_330

	// *** Basic block 50

	j           .EmitBranch_label_390

	// *** Basic block 51

	j           .EmitBranch_label_308

	// *** Basic block 52

	j           .EmitBranch_label_412

	// *** Basic block 53

	j           .EmitBranch_label_352

	// *** Basic block 54

	j           .EmitBranch_label_450

	// *** Basic block 55

.EmitBranch_label_305:
	call        abort

	// *** Basic block 56

	j           .EmitBranch_label_472

	// *** Basic block 57

.EmitBranch_label_308:
	beqz        s3, .EmitBranch_label_315

	// *** Basic block 58

	lla         s5, .str.76
	j           .EmitBranch_label_318

	// *** Basic block 59

.EmitBranch_label_315:
	lla         s5, .str.77

	// *** Basic block 60

.EmitBranch_label_318:
	lla         a1, .str.78
	mv          a3, s2
	mv          a2, s5
	mv          a0, s1
	call        fprintf

	// *** Basic block 61

	j           .EmitBranch_label_472

	// *** Basic block 62

.EmitBranch_label_330:
	beqz        s3, .EmitBranch_label_337

	// *** Basic block 63

	lla         s5, .str.79
	j           .EmitBranch_label_340

	// *** Basic block 64

.EmitBranch_label_337:
	lla         s5, .str.80

	// *** Basic block 65

.EmitBranch_label_340:
	lla         a1, .str.81
	mv          a3, s2
	mv          a2, s5
	mv          a0, s1
	call        fprintf

	// *** Basic block 66

	j           .EmitBranch_label_472

	// *** Basic block 67

.EmitBranch_label_352:
	beqz        s3, .EmitBranch_label_359

	// *** Basic block 68

	lla         s5, .str.82
	j           .EmitBranch_label_362

	// *** Basic block 69

.EmitBranch_label_359:
	lla         s5, .str.83

	// *** Basic block 70

.EmitBranch_label_362:
	lla         a1, .str.84
	mv          a2, s6
	mv          a0, s1
	call        fprintf

	// *** Basic block 71

	lla         a1, .str.85
	mv          a3, s2
	mv          a2, s5
	mv          a0, s1
	call        fprintf

	// *** Basic block 72

	lla         a1, .str.86
	mv          a2, s6
	mv          a0, s1
	call        fprintf

	// *** Basic block 73

	j           .EmitBranch_label_472

	// *** Basic block 74

.EmitBranch_label_390:
	beqz        s3, .EmitBranch_label_397

	// *** Basic block 75

	lla         s5, .str.87
	j           .EmitBranch_label_400

	// *** Basic block 76

.EmitBranch_label_397:
	lla         s5, .str.88

	// *** Basic block 77

.EmitBranch_label_400:
	lla         a1, .str.89
	mv          a3, s2
	mv          a2, s5
	mv          a0, s1
	call        fprintf

	// *** Basic block 78

	j           .EmitBranch_label_472

	// *** Basic block 79

.EmitBranch_label_412:
	beqz        s3, .EmitBranch_label_419

	// *** Basic block 80

	lla         s5, .str.90
	j           .EmitBranch_label_422

	// *** Basic block 81

.EmitBranch_label_419:
	lla         s5, .str.91

	// *** Basic block 82

.EmitBranch_label_422:
	lla         a1, .str.92
	mv          a2, s6
	mv          a0, s1
	call        fprintf

	// *** Basic block 83

	lla         a1, .str.93
	mv          a3, s2
	mv          a2, s5
	mv          a0, s1
	call        fprintf

	// *** Basic block 84

	lla         a1, .str.94
	mv          a2, s6
	mv          a0, s1
	call        fprintf

	// *** Basic block 85

	j           .EmitBranch_label_472

	// *** Basic block 86

.EmitBranch_label_450:
	beqz        s3, .EmitBranch_label_457

	// *** Basic block 87

	lla         s5, .str.95
	j           .EmitBranch_label_460

	// *** Basic block 88

.EmitBranch_label_457:
	lla         s5, .str.96

	// *** Basic block 89

.EmitBranch_label_460:
	lla         a1, .str.97
	mv          a3, s2
	mv          a2, s5
	mv          a0, s1
	call        fprintf

	// *** Basic block 90

	j           .EmitBranch_label_472

	// *** Basic block 91

.EmitBranch_label_472:

	// *** Basic block 92

.EmitBranch_label_473:
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld s6, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_EmitBranch:
	.size EmitBranch, .func_end_EmitBranch-EmitBranch

	.local  EnterSubroutine
	.type EnterSubroutine, @function

EnterSubroutine:

	// *** Basic block 0

	.global fprintf
	.local GenerateRegisterMask
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
	mv          s1, a2
	mv          s2, a0
	lla         a1, .str.98
	mv          a0, s1
	call        fprintf

	// *** Basic block 1

	mv          a1, s1
	mv          a0, s2
	call        GenerateRegisterMask

	// *** Basic block 2

	lla         a1, .str.99
	ld          t0, 0(s2)
	lw          a2, 128(t0)
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           fprintf
.func_end_EnterSubroutine:
	.size EnterSubroutine, .func_end_EnterSubroutine-EnterSubroutine

	.local  ReturnFromSubroutine
	.type ReturnFromSubroutine, @function

ReturnFromSubroutine:

	// *** Basic block 0

	.global fprintf
	.local GenerateRegisterMask
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
	mv          s1, a2
	mv          s2, a0
	lla         a1, .str.100
	mv          a0, s1
	call        fprintf

	// *** Basic block 1

	mv          a1, s1
	mv          a0, s2
	call        GenerateRegisterMask

	// *** Basic block 2

	lla         a1, .str.101
	ld          t0, 0(s2)
	lw          a2, 128(t0)
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           fprintf
.func_end_ReturnFromSubroutine:
	.size ReturnFromSubroutine, .func_end_ReturnFromSubroutine-ReturnFromSubroutine

	.local  OperateThroughAccumulator
	.type OperateThroughAccumulator, @function

OperateThroughAccumulator:

	// *** Basic block 0

	.local LoadByte
	.local OperateByte
	.local StoreByte
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
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a5
	mv          s3, a4
	ld          s4, 32(a2)
	ld          s5, 32(a3)
	mv          s6, x0
	bge         x0, s1, .OperateThroughAccumulator_label_63

	// *** Basic block 1

.OperateThroughAccumulator_label_34:
	mv          a2, s2
	mv          a1, s6
	mv          a0, s4
	call        LoadByte

	// *** Basic block 2

	mv          a3, s2
	mv          a2, s3
	mv          a1, s6
	mv          a0, s5
	call        OperateByte

	// *** Basic block 3

	mv          a2, s2
	mv          a1, s6
	mv          a0, s5
	call        StoreByte

	// *** Basic block 4

.OperateThroughAccumulator_label_59:
	addi        s6, s6, 1
	bge         s6, s1, .OperateThroughAccumulator_label_34

	// *** Basic block 5

.OperateThroughAccumulator_label_63:
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld s6, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_OperateThroughAccumulator:
	.size OperateThroughAccumulator, .func_end_OperateThroughAccumulator-OperateThroughAccumulator

	.local  Add
	.type Add, @function

Add:

	// *** Basic block 0

	.global fprintf
	.local OperateThroughAccumulator
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
	mv          s1, a4
	mv          s2, a0
	mv          s3, a1
	mv          s4, a2
	mv          s5, a3
	lla         a1, .str.102
	mv          a0, s1
	call        fprintf

	// *** Basic block 1

	lla         a4, .str.103
	mv          a5, s1
	mv          a3, s5
	mv          a2, s4
	mv          a1, s3
	mv          a0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           OperateThroughAccumulator
.func_end_Add:
	.size Add, .func_end_Add-Add

	.local  Sub
	.type Sub, @function

Sub:

	// *** Basic block 0

	.global fprintf
	.local OperateThroughAccumulator
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
	mv          s1, a4
	mv          s2, a0
	mv          s3, a1
	mv          s4, a2
	mv          s5, a3
	lla         a1, .str.104
	mv          a0, s1
	call        fprintf

	// *** Basic block 1

	lla         a4, .str.105
	mv          a5, s1
	mv          a3, s5
	mv          a2, s4
	mv          a1, s3
	mv          a0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           OperateThroughAccumulator
.func_end_Sub:
	.size Sub, .func_end_Sub-Sub

	.local  And
	.type And, @function

And:

	// *** Basic block 0

	.local OperateThroughAccumulator
	// Leaf procedure, no stack frame generated
	mv          t0, a4
	lla         a4, .str.106
	mv          a5, t0
	j           OperateThroughAccumulator
.func_end_And:
	.size And, .func_end_And-And

	.local  Ora
	.type Ora, @function

Ora:

	// *** Basic block 0

	.local OperateThroughAccumulator
	// Leaf procedure, no stack frame generated
	mv          t0, a4
	lla         a4, .str.107
	mv          a5, t0
	j           OperateThroughAccumulator
.func_end_Ora:
	.size Ora, .func_end_Ora-Ora

	.local  Eor
	.type Eor, @function

Eor:

	// *** Basic block 0

	.local OperateThroughAccumulator
	// Leaf procedure, no stack frame generated
	mv          t0, a4
	lla         a4, .str.108
	mv          a5, t0
	j           OperateThroughAccumulator
.func_end_Eor:
	.size Eor, .func_end_Eor-Eor

	.local  Inc
	.type Inc, @function

Inc:

	// *** Basic block 0

	.global snprintf
	.local OperateByte
	.global fprintf
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -48(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	sd s6, 0(sp)
	// End of stack frame
	mv          t0, a4
	mv          t1, a2
	mv          s1, a0
	mv          s2, a3
	mv          s3, a5
	addi        s4, s0, -48
	lla         a2, .str.109
	lw          a4, 20(t1)
	mv          a3, t0
	li          a1, 32		// 0x20 ASCII ' '
	mv          a0, s4
	call        snprintf

	// *** Basic block 1

	ld          s5, 32(s2)
	mv          s6, x0
	bge         x0, s1, .Inc_label_76

	// *** Basic block 2

.Inc_label_53:
	lla         a2, .str.110
	mv          a3, s3
	mv          a1, s6
	mv          a0, s5
	call        OperateByte

	// *** Basic block 3

	lla         a1, .str.111
	mv          a2, s4
	mv          a0, s3
	call        fprintf

	// *** Basic block 4

.Inc_label_72:
	addi        s6, s6, 1
	bge         s6, s1, .Inc_label_53

	// *** Basic block 5

.Inc_label_76:
	lla         a1, .str.112
	mv          a2, s4
	mv          a0, s3
	call        fprintf

	// *** Basic block 6

	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld s6, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Inc:
	.size Inc, .func_end_Inc-Inc

	.local  Push
	.type Push, @function

Push:

	// *** Basic block 0

	.local InitIndex
	.local LoadByte
	.global fprintf
	.local IncrementIndex
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
	mv          s1, a1
	mv          s2, a2
	mv          s3, a0
	mv          a0, x0
	call        InitIndex

	// *** Basic block 1

	mv          s4, x0
	bge         x0, s3, .Push_label_62

	// *** Basic block 2

.Push_label_31:
	mv          a2, s2
	mv          a1, s4
	mv          a0, s1
	call        LoadByte

	// *** Basic block 3

	lla         a1, .str.113
	mv          a0, s2
	call        fprintf

	// *** Basic block 4

	mv          a4, s2
	mv          a3, s3
	mv          a2, s4
	mv          a1, s1
	mv          a0, x0
	call        IncrementIndex

	// *** Basic block 5

.Push_label_58:
	addi        s4, s4, 1
	bge         s4, s3, .Push_label_31

	// *** Basic block 6

.Push_label_62:
	lla         a1, .str.114
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
.func_end_Push:
	.size Push, .func_end_Push-Push

	.local  Pull
	.type Pull, @function

Pull:

	// *** Basic block 0

	.global fprintf
	.local InitIndex
	.local StoreByte
	.local IncrementIndex
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
	mv          s1, a2
	mv          s2, a0
	mv          s3, a1
	lla         a1, .str.115
	mv          a2, s2
	mv          a0, s1
	call        fprintf

	// *** Basic block 1

	mv          a2, s1
	mv          a1, s3
	mv          a0, x0
	call        InitIndex

	// *** Basic block 2

	mv          s4, x0
	bge         x0, s2, .Pull_label_70

	// *** Basic block 3

.Pull_label_39:
	lla         a1, .str.116
	mv          a0, s1
	call        fprintf

	// *** Basic block 4

	mv          a2, s1
	mv          a1, s4
	mv          a0, s3
	call        StoreByte

	// *** Basic block 5

	mv          a4, s1
	mv          a3, s2
	mv          a2, s4
	mv          a1, s3
	mv          a0, x0
	call        IncrementIndex

	// *** Basic block 6

.Pull_label_66:
	addi        s4, s4, 1
	bge         s4, s2, .Pull_label_39

	// *** Basic block 7

.Pull_label_70:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Pull:
	.size Pull, .func_end_Pull-Pull

	.local  PrintInstruction
	.type PrintInstruction, @function

PrintInstruction:

	// *** Basic block 0

	.global fprintf
	.global W65C02PrintInstruction
	.local IsPrintable
	.global TargetIsConst
	.global printf
	.global abort
	.local Copy
	.local RegisterSize
	.local PrintRmov
	.global StorageIs
	.global TargetIntValue
	.global W65C02RegisterAsString
	.local buf2
	.local EnterSubroutine
	.local ReturnFromSubroutine
	.global CompilerFindStringLiteral
	.global SourceLocationNumbers
	.local Push
	.local Pull
	.global snprintf
	.local EmitBranch
	.local Compare
	.local Add
	.local Inc
	.local Sub
	.local Ora
	.local And
	.local Eor
	.global stdout
	addi sp, sp, -176
	// Saved return address (offset 168) and frame pointer (offset 160)
	sd ra, 168(sp)
	sd s0, 160(sp)
	addi s0, sp, 176
	// Local vars at offset -64(s0)
	// Spilled register region: 16 bytes at -80(s0) to -64(s0)
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
	mv          s1, a3
	mv          s2, a1
	mv          s3, a2
	mv          s4, a0
	sd          s4, -72(s0)	// Spilled @136
	lla         a1, .str.117
	mv          a0, s1
	call        fprintf

	// *** Basic block 1

	mv          a1, s1
	mv          a0, s2
	call        W65C02PrintInstruction

	// *** Basic block 2

	lw          s5, 16(s2)
	li          t0, 22		// 0x16 ASCII \x16
	bne         s5, t0, .PrintInstruction_label_172

	// *** Basic block 3

	lla         a1, .str.118
	lw          a3, 20(s2)
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 4

.PrintInstruction_label_169:
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

	// *** Basic block 5

.PrintInstruction_label_172:
	li          s6, 32		// 0x20 ASCII ' '
	bne         s5, s6, .PrintInstruction_label_190

	// *** Basic block 6

	mv          s7, s2
	lla         a1, .str.119
	ld          a2, 112(s7)
	mv          a0, s1
	call        fprintf

	// *** Basic block 7

	j           .PrintInstruction_label_169

	// *** Basic block 8

.PrintInstruction_label_190:
	mv          a0, s2
	call        IsPrintable

	// *** Basic block 9

	not         t0, a0
	beqz        t0, .PrintInstruction_label_197

	// *** Basic block 10

	j           .PrintInstruction_label_169

	// *** Basic block 11

.PrintInstruction_label_197:
	li          s8, 2		// 0x2 ASCII \x2
	blt         s5, s8, .PrintInstruction_label_1445

	// *** Basic block 12

	li          t0, 194		// 0xc2 ASCII \xc2
	blt         t0, s5, .PrintInstruction_label_1445

	// *** Basic block 13

	addi        t0, s5, -2
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 14

	j           .PrintInstruction_label_461

	// *** Basic block 15

	j           .PrintInstruction_label_1445

	// *** Basic block 16

	j           .PrintInstruction_label_1445

	// *** Basic block 17

	j           .PrintInstruction_label_1445

	// *** Basic block 18

	j           .PrintInstruction_label_1445

	// *** Basic block 19

	j           .PrintInstruction_label_1445

	// *** Basic block 20

	j           .PrintInstruction_label_1445

	// *** Basic block 21

	j           .PrintInstruction_label_1445

	// *** Basic block 22

	j           .PrintInstruction_label_1445

	// *** Basic block 23

	j           .PrintInstruction_label_1445

	// *** Basic block 24

	j           .PrintInstruction_label_1445

	// *** Basic block 25

	j           .PrintInstruction_label_1445

	// *** Basic block 26

	j           .PrintInstruction_label_406

	// *** Basic block 27

	j           .PrintInstruction_label_408

	// *** Basic block 28

	j           .PrintInstruction_label_409

	// *** Basic block 29

	j           .PrintInstruction_label_407

	// *** Basic block 30

	j           .PrintInstruction_label_449

	// *** Basic block 31

	j           .PrintInstruction_label_451

	// *** Basic block 32

	j           .PrintInstruction_label_452

	// *** Basic block 33

	j           .PrintInstruction_label_1445

	// *** Basic block 34

	j           .PrintInstruction_label_1445

	// *** Basic block 35

	j           .PrintInstruction_label_1445

	// *** Basic block 36

	j           .PrintInstruction_label_1445

	// *** Basic block 37

	j           .PrintInstruction_label_1445

	// *** Basic block 38

	j           .PrintInstruction_label_1445

	// *** Basic block 39

	j           .PrintInstruction_label_1445

	// *** Basic block 40

	j           .PrintInstruction_label_1445

	// *** Basic block 41

	j           .PrintInstruction_label_1445

	// *** Basic block 42

	j           .PrintInstruction_label_753

	// *** Basic block 43

	j           .PrintInstruction_label_797

	// *** Basic block 44

	j           .PrintInstruction_label_1445

	// *** Basic block 45

	j           .PrintInstruction_label_1445

	// *** Basic block 46

	j           .PrintInstruction_label_1445

	// *** Basic block 47

	j           .PrintInstruction_label_405

	// *** Basic block 48

	j           .PrintInstruction_label_1445

	// *** Basic block 49

	j           .PrintInstruction_label_1445

	// *** Basic block 50

	j           .PrintInstruction_label_448

	// *** Basic block 51

	j           .PrintInstruction_label_450

	// *** Basic block 52

	j           .PrintInstruction_label_1445

	// *** Basic block 53

	j           .PrintInstruction_label_1445

	// *** Basic block 54

	j           .PrintInstruction_label_1445

	// *** Basic block 55

	j           .PrintInstruction_label_1445

	// *** Basic block 56

	j           .PrintInstruction_label_1445

	// *** Basic block 57

	j           .PrintInstruction_label_735

	// *** Basic block 58

	j           .PrintInstruction_label_744

	// *** Basic block 59

	j           .PrintInstruction_label_1445

	// *** Basic block 60

	j           .PrintInstruction_label_1445

	// *** Basic block 61

	j           .PrintInstruction_label_1445

	// *** Basic block 62

	j           .PrintInstruction_label_496

	// *** Basic block 63

	j           .PrintInstruction_label_577

	// *** Basic block 64

	j           .PrintInstruction_label_831

	// *** Basic block 65

	j           .PrintInstruction_label_832

	// *** Basic block 66

	j           .PrintInstruction_label_1445

	// *** Basic block 67

	j           .PrintInstruction_label_830

	// *** Basic block 68

	j           .PrintInstruction_label_834

	// *** Basic block 69

	j           .PrintInstruction_label_835

	// *** Basic block 70

	j           .PrintInstruction_label_833

	// *** Basic block 71

	j           .PrintInstruction_label_854

	// *** Basic block 72

	j           .PrintInstruction_label_855

	// *** Basic block 73

	j           .PrintInstruction_label_1445

	// *** Basic block 74

	j           .PrintInstruction_label_853

	// *** Basic block 75

	j           .PrintInstruction_label_857

	// *** Basic block 76

	j           .PrintInstruction_label_858

	// *** Basic block 77

	j           .PrintInstruction_label_856

	// *** Basic block 78

	j           .PrintInstruction_label_1445

	// *** Basic block 79

	j           .PrintInstruction_label_876

	// *** Basic block 80

	j           .PrintInstruction_label_877

	// *** Basic block 81

	j           .PrintInstruction_label_878

	// *** Basic block 82

	j           .PrintInstruction_label_879

	// *** Basic block 83

	j           .PrintInstruction_label_880

	// *** Basic block 84

	j           .PrintInstruction_label_881

	// *** Basic block 85

	j           .PrintInstruction_label_883

	// *** Basic block 86

	j           .PrintInstruction_label_882

	// *** Basic block 87

	j           .PrintInstruction_label_884

	// *** Basic block 88

	j           .PrintInstruction_label_1445

	// *** Basic block 89

	j           .PrintInstruction_label_941

	// *** Basic block 90

	j           .PrintInstruction_label_942

	// *** Basic block 91

	j           .PrintInstruction_label_943

	// *** Basic block 92

	j           .PrintInstruction_label_945

	// *** Basic block 93

	j           .PrintInstruction_label_946

	// *** Basic block 94

	j           .PrintInstruction_label_944

	// *** Basic block 95

	j           .PrintInstruction_label_1207

	// *** Basic block 96

	j           .PrintInstruction_label_1269

	// *** Basic block 97

	j           .PrintInstruction_label_1338

	// *** Basic block 98

	j           .PrintInstruction_label_1339

	// *** Basic block 99

	j           .PrintInstruction_label_1289

	// *** Basic block 100

	j           .PrintInstruction_label_1224

	// *** Basic block 101

	j           .PrintInstruction_label_1341

	// *** Basic block 102

	j           .PrintInstruction_label_1361

	// *** Basic block 103

	j           .PrintInstruction_label_1362

	// *** Basic block 104

	j           .PrintInstruction_label_1364

	// *** Basic block 105

	j           .PrintInstruction_label_1365

	// *** Basic block 106

	j           .PrintInstruction_label_1366

	// *** Basic block 107

	j           .PrintInstruction_label_1367

	// *** Basic block 108

	j           .PrintInstruction_label_1368

	// *** Basic block 109

	j           .PrintInstruction_label_1369

	// *** Basic block 110

	j           .PrintInstruction_label_1370

	// *** Basic block 111

	j           .PrintInstruction_label_1371

	// *** Basic block 112

	j           .PrintInstruction_label_1372

	// *** Basic block 113

	j           .PrintInstruction_label_1373

	// *** Basic block 114

	j           .PrintInstruction_label_1374

	// *** Basic block 115

	j           .PrintInstruction_label_1375

	// *** Basic block 116

	j           .PrintInstruction_label_1376

	// *** Basic block 117

	j           .PrintInstruction_label_1377

	// *** Basic block 118

	j           .PrintInstruction_label_1378

	// *** Basic block 119

	j           .PrintInstruction_label_1398

	// *** Basic block 120

	j           .PrintInstruction_label_1418

	// *** Basic block 121

	j           .PrintInstruction_label_1438

	// *** Basic block 122

	j           .PrintInstruction_label_1439

	// *** Basic block 123

	j           .PrintInstruction_label_1441

	// *** Basic block 124

	j           .PrintInstruction_label_1442

	// *** Basic block 125

	j           .PrintInstruction_label_1443

	// *** Basic block 126

	j           .PrintInstruction_label_1147

	// *** Basic block 127

	j           .PrintInstruction_label_1148

	// *** Basic block 128

	j           .PrintInstruction_label_1149

	// *** Basic block 129

	j           .PrintInstruction_label_1150

	// *** Basic block 130

	j           .PrintInstruction_label_1151

	// *** Basic block 131

	j           .PrintInstruction_label_1152

	// *** Basic block 132

	j           .PrintInstruction_label_1153

	// *** Basic block 133

	j           .PrintInstruction_label_1154

	// *** Basic block 134

	j           .PrintInstruction_label_1155

	// *** Basic block 135

	j           .PrintInstruction_label_1156

	// *** Basic block 136

	j           .PrintInstruction_label_1157

	// *** Basic block 137

	j           .PrintInstruction_label_1158

	// *** Basic block 138

	j           .PrintInstruction_label_1159

	// *** Basic block 139

	j           .PrintInstruction_label_1160

	// *** Basic block 140

	j           .PrintInstruction_label_1161

	// *** Basic block 141

	j           .PrintInstruction_label_1162

	// *** Basic block 142

	j           .PrintInstruction_label_1163

	// *** Basic block 143

	j           .PrintInstruction_label_1164

	// *** Basic block 144

	j           .PrintInstruction_label_1165

	// *** Basic block 145

	j           .PrintInstruction_label_1166

	// *** Basic block 146

	j           .PrintInstruction_label_1167

	// *** Basic block 147

	j           .PrintInstruction_label_1168

	// *** Basic block 148

	j           .PrintInstruction_label_1169

	// *** Basic block 149

	j           .PrintInstruction_label_1170

	// *** Basic block 150

	j           .PrintInstruction_label_1171

	// *** Basic block 151

	j           .PrintInstruction_label_1172

	// *** Basic block 152

	j           .PrintInstruction_label_1173

	// *** Basic block 153

	j           .PrintInstruction_label_1174

	// *** Basic block 154

	j           .PrintInstruction_label_1175

	// *** Basic block 155

	j           .PrintInstruction_label_1176

	// *** Basic block 156

	j           .PrintInstruction_label_1177

	// *** Basic block 157

	j           .PrintInstruction_label_1178

	// *** Basic block 158

	j           .PrintInstruction_label_1179

	// *** Basic block 159

	j           .PrintInstruction_label_1180

	// *** Basic block 160

	j           .PrintInstruction_label_1181

	// *** Basic block 161

	j           .PrintInstruction_label_1182

	// *** Basic block 162

	j           .PrintInstruction_label_1183

	// *** Basic block 163

	j           .PrintInstruction_label_1184

	// *** Basic block 164

	j           .PrintInstruction_label_1185

	// *** Basic block 165

	j           .PrintInstruction_label_1186

	// *** Basic block 166

	j           .PrintInstruction_label_1187

	// *** Basic block 167

	j           .PrintInstruction_label_1188

	// *** Basic block 168

	j           .PrintInstruction_label_1189

	// *** Basic block 169

	j           .PrintInstruction_label_1190

	// *** Basic block 170

	j           .PrintInstruction_label_1191

	// *** Basic block 171

	j           .PrintInstruction_label_1192

	// *** Basic block 172

	j           .PrintInstruction_label_1193

	// *** Basic block 173

	j           .PrintInstruction_label_1194

	// *** Basic block 174

	j           .PrintInstruction_label_1195

	// *** Basic block 175

	j           .PrintInstruction_label_1196

	// *** Basic block 176

	j           .PrintInstruction_label_1197

	// *** Basic block 177

	j           .PrintInstruction_label_1198

	// *** Basic block 178

	j           .PrintInstruction_label_1005

	// *** Basic block 179

	j           .PrintInstruction_label_1006

	// *** Basic block 180

	j           .PrintInstruction_label_1077

	// *** Basic block 181

	j           .PrintInstruction_label_1445

	// *** Basic block 182

	j           .PrintInstruction_label_1445

	// *** Basic block 183

	j           .PrintInstruction_label_1445

	// *** Basic block 184

	j           .PrintInstruction_label_1445

	// *** Basic block 185

	j           .PrintInstruction_label_1445

	// *** Basic block 186

	j           .PrintInstruction_label_1445

	// *** Basic block 187

	j           .PrintInstruction_label_1445

	// *** Basic block 188

	j           .PrintInstruction_label_1445

	// *** Basic block 189

	j           .PrintInstruction_label_1445

	// *** Basic block 190

	j           .PrintInstruction_label_1445

	// *** Basic block 191

	j           .PrintInstruction_label_1445

	// *** Basic block 192

	j           .PrintInstruction_label_1112

	// *** Basic block 193

	j           .PrintInstruction_label_1445

	// *** Basic block 194

	j           .PrintInstruction_label_1445

	// *** Basic block 195

	j           .PrintInstruction_label_1445

	// *** Basic block 196

	j           .PrintInstruction_label_1445

	// *** Basic block 197

	j           .PrintInstruction_label_658

	// *** Basic block 198

	j           .PrintInstruction_label_659

	// *** Basic block 199

	j           .PrintInstruction_label_1445

	// *** Basic block 200

	j           .PrintInstruction_label_660

	// *** Basic block 201

	j           .PrintInstruction_label_661

	// *** Basic block 202

	j           .PrintInstruction_label_703

	// *** Basic block 203

	j           .PrintInstruction_label_704

	// *** Basic block 204

	j           .PrintInstruction_label_1445

	// *** Basic block 205

	j           .PrintInstruction_label_705

	// *** Basic block 206

	j           .PrintInstruction_label_706

	// *** Basic block 207

.PrintInstruction_label_405:

	// *** Basic block 208

.PrintInstruction_label_406:

	// *** Basic block 209

.PrintInstruction_label_407:

	// *** Basic block 210

.PrintInstruction_label_408:

	// *** Basic block 211

.PrintInstruction_label_409:
	ld          s5, 40(s2)
	mv          a0, s5
	call        TargetIsConst

	// *** Basic block 212

	beqz        a0, .PrintInstruction_label_418

	// *** Basic block 213

	j           .PrintInstruction_label_433

	// *** Basic block 214

.PrintInstruction_label_418:
	lla         a0, .str.120
	lla         a1, .str.121
	lla         a3, .str.122
	li          t0, 661		// 0x295
	mv          a2, t0
	call        printf

	// *** Basic block 215

	call        abort

	// *** Basic block 216

.PrintInstruction_label_433:
	ld          a0, 32(s2)
	call        RegisterSize

	// *** Basic block 217

	mv          a3, s1
	mv          a2, s5
	mv          a1, s2
	call        Copy

	// *** Basic block 218

	j           .PrintInstruction_label_1454

	// *** Basic block 219

.PrintInstruction_label_448:

	// *** Basic block 220

.PrintInstruction_label_449:

	// *** Basic block 221

.PrintInstruction_label_450:

	// *** Basic block 222

.PrintInstruction_label_451:

	// *** Basic block 223

.PrintInstruction_label_452:
	mv          a2, s1
	mv          a1, s2
	mv          a0, s4
	call        PrintRmov

	// *** Basic block 224

	j           .PrintInstruction_label_169

	// *** Basic block 225

.PrintInstruction_label_461:
	mv          s5, s2
	sd          s5, -72(s0)	// Spilled @462
	ld          s9, 112(s5)
	lw          a0, 48(s9)
	mv          a1, s8
	call        StorageIs

	// *** Basic block 226

	beqz        a0, .PrintInstruction_label_484

	// *** Basic block 227

	lla         a1, .str.123
	ld          a2, 16(s9)
	mv          a0, s1
	call        fprintf

	// *** Basic block 228

	j           .PrintInstruction_label_494

	// *** Basic block 229

.PrintInstruction_label_484:
	lla         a1, .str.124
	ld          a2, 16(s9)
	mv          a0, s1
	call        fprintf

	// *** Basic block 230

.PrintInstruction_label_494:
	j           .PrintInstruction_label_169

	// *** Basic block 231

.PrintInstruction_label_496:
	ld          s11, 40(s2)
	mv          a0, s11
	call        TargetIsConst

	// *** Basic block 232

	beqz        a0, .PrintInstruction_label_505

	// *** Basic block 233

	j           .PrintInstruction_label_520

	// *** Basic block 234

.PrintInstruction_label_505:
	lla         a0, .str.125
	lla         a1, .str.126
	lla         a3, .str.127
	li          t0, 684		// 0x2ac
	mv          a2, t0
	call        printf

	// *** Basic block 235

	call        abort

	// *** Basic block 236

.PrintInstruction_label_520:
	mv          a0, s11
	call        TargetIntValue
	sd          a0, -72(s0)	// Spilled @523

	// *** Basic block 237

	sext.w      s11, a0
	andi        a0, s11, 255
	srai        a0, s11, 8
	lla         a1, .str.128
	mv          a0, s1
	call        fprintf

	// *** Basic block 238

	lla         a1, .str.129
	mv          a0, s1
	call        fprintf

	// *** Basic block 239

	lla         a1, .str.130
	ld          a2, -80(s0)	// Spilled @527
	sd          a0, -80(s0)	// Spilled @527
	mv          a0, s1
	call        fprintf

	// *** Basic block 240

	lla         a1, .str.131
	mv          a0, s1
	call        fprintf

	// *** Basic block 241

	lla         a1, .str.132
	mv          a0, s1
	call        fprintf

	// *** Basic block 242

	lla         a1, .str.133
	mv          a2, a0
	mv          a0, s1
	call        fprintf

	// *** Basic block 243

	lla         a1, .str.134
	mv          a0, s1
	call        fprintf

	// *** Basic block 244

	j           .PrintInstruction_label_1454

	// *** Basic block 245

.PrintInstruction_label_577:
	ld          s11, 40(s2)
	mv          a0, s11
	call        TargetIsConst

	// *** Basic block 246

	beqz        a0, .PrintInstruction_label_586

	// *** Basic block 247

	j           .PrintInstruction_label_601

	// *** Basic block 248

.PrintInstruction_label_586:
	lla         a0, .str.135
	lla         a1, .str.136
	lla         a3, .str.137
	li          t0, 699		// 0x2bb
	mv          a2, t0
	call        printf

	// *** Basic block 249

	call        abort

	// *** Basic block 250

.PrintInstruction_label_601:
	mv          a0, s11
	call        TargetIntValue
	sd          a0, -72(s0)	// Spilled @604

	// *** Basic block 251

	sext.w      s11, a0
	andi        a0, s11, 255
	srai        a0, s11, 8
	lla         a1, .str.138
	mv          a0, s1
	call        fprintf

	// *** Basic block 252

	lla         a1, .str.139
	mv          a0, s1
	call        fprintf

	// *** Basic block 253

	lla         a1, .str.140
	ld          a2, -80(s0)	// Spilled @608
	sd          a0, -80(s0)	// Spilled @608
	mv          a0, s1
	call        fprintf

	// *** Basic block 254

	lla         a1, .str.141
	mv          a0, s1
	call        fprintf

	// *** Basic block 255

	lla         a1, .str.142
	mv          a0, s1
	call        fprintf

	// *** Basic block 256

	lla         a1, .str.143
	mv          a2, a0
	mv          a0, s1
	call        fprintf

	// *** Basic block 257

	lla         a1, .str.144
	mv          a0, s1
	call        fprintf

	// *** Basic block 258

	j           .PrintInstruction_label_1454

	// *** Basic block 259

.PrintInstruction_label_658:

	// *** Basic block 260

.PrintInstruction_label_659:

	// *** Basic block 261

.PrintInstruction_label_660:

	// *** Basic block 262

.PrintInstruction_label_661:
	ld          s5, 40(s2)
	lw          t0, 16(s5)
	bne         t0, s8, .PrintInstruction_label_671

	// *** Basic block 263

	j           .PrintInstruction_label_686

	// *** Basic block 264

.PrintInstruction_label_671:
	lla         a0, .str.145
	lla         a1, .str.146
	lla         a3, .str.147
	li          t0, 717		// 0x2cd
	mv          a2, t0
	call        printf

	// *** Basic block 265

	call        abort

	// *** Basic block 266

.PrintInstruction_label_686:
	lla         a1, .str.148
	lla         a2, .str.149
	ld          t0, 112(s5)
	ld          a3, 16(t0)
	mv          a0, s1
	call        fprintf

	// *** Basic block 267

	j           .PrintInstruction_label_169

	// *** Basic block 268

.PrintInstruction_label_703:

	// *** Basic block 269

.PrintInstruction_label_704:

	// *** Basic block 270

.PrintInstruction_label_705:

	// *** Basic block 271

.PrintInstruction_label_706:
	lla         s5, .str.150
	lla         s11, .str.151
	ld          t0, 40(s2)
	ld          a0, 32(t0)
	lla         a2, buf2
	li          t0, 4096		// 0x1000
	mv          a3, t0
	mv          a1, s8
	call        W65C02RegisterAsString

	// *** Basic block 272

	mv          a3, a0
	mv          a2, s11
	mv          a1, s5
	mv          a0, s1
	call        fprintf

	// *** Basic block 273

	j           .PrintInstruction_label_169

	// *** Basic block 274

.PrintInstruction_label_735:
	mv          a2, s1
	mv          a1, s2
	mv          a0, s4
	call        EnterSubroutine

	// *** Basic block 275

	j           .PrintInstruction_label_169

	// *** Basic block 276

.PrintInstruction_label_744:
	mv          a2, s1
	mv          a1, s2
	mv          a0, s4
	call        ReturnFromSubroutine

	// *** Basic block 277

	j           .PrintInstruction_label_169

	// *** Basic block 278

.PrintInstruction_label_753:
	ld          s9, 40(s2)
	lw          a0, 112(s9)
	call        CompilerFindStringLiteral

	// *** Basic block 279

	mv          s9, a0
	beq         s9, x0, .PrintInstruction_label_767

	// *** Basic block 280

	j           .PrintInstruction_label_782

	// *** Basic block 281

.PrintInstruction_label_767:
	lla         a0, .str.152
	lla         a1, .str.153
	lla         a3, .str.154
	li          t0, 743		// 0x2e7
	mv          a2, t0
	call        printf

	// *** Basic block 282

	call        abort

	// *** Basic block 283

.PrintInstruction_label_782:
	lla         a1, .str.155
	addi        t0, s9, 8
	ld          a2, 16(t0)
	mv          a0, s1
	call        fprintf

	// *** Basic block 284

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 48(s9)
	j           .PrintInstruction_label_169

	// *** Basic block 285

.PrintInstruction_label_797:
	mv          s10, s2
	ld          a0, 112(s10)
	addi        a1, s0, -64
	addi        a2, s0, -60
	addi        a3, s0, -56
	call        SourceLocationNumbers

	// *** Basic block 286

	lla         a1, .str.156
	lw          t0, -64(s0)
	addi        a2, t0, 1
	lw          a3, -60(s0)
	lw          t0, -56(s0)
	addi        a4, t0, 1
	mv          a0, s1
	call        fprintf

	// *** Basic block 287

	j           .PrintInstruction_label_169

	// *** Basic block 288

.PrintInstruction_label_830:

	// *** Basic block 289

.PrintInstruction_label_831:

	// *** Basic block 290

.PrintInstruction_label_832:

	// *** Basic block 291

.PrintInstruction_label_833:

	// *** Basic block 292

.PrintInstruction_label_834:

	// *** Basic block 293

.PrintInstruction_label_835:
	ld          t0, 40(s2)
	ld          s5, 32(t0)
	mv          a0, s5
	call        RegisterSize

	// *** Basic block 294

	mv          s8, a0
	mv          a2, s1
	mv          a1, s5
	mv          a0, s8
	call        Push

	// *** Basic block 295

	j           .PrintInstruction_label_1454

	// *** Basic block 296

.PrintInstruction_label_853:

	// *** Basic block 297

.PrintInstruction_label_854:

	// *** Basic block 298

.PrintInstruction_label_855:

	// *** Basic block 299

.PrintInstruction_label_856:

	// *** Basic block 300

.PrintInstruction_label_857:

	// *** Basic block 301

.PrintInstruction_label_858:
	ld          t0, 40(s2)
	ld          s5, 32(t0)
	mv          a0, s5
	call        RegisterSize

	// *** Basic block 302

	mv          s11, a0
	mv          a2, s1
	mv          a1, s5
	mv          a0, s11
	call        Pull

	// *** Basic block 303

	j           .PrintInstruction_label_1454

	// *** Basic block 304

.PrintInstruction_label_876:

	// *** Basic block 305

.PrintInstruction_label_877:

	// *** Basic block 306

.PrintInstruction_label_878:

	// *** Basic block 307

.PrintInstruction_label_879:

	// *** Basic block 308

.PrintInstruction_label_880:

	// *** Basic block 309

.PrintInstruction_label_881:

	// *** Basic block 310

.PrintInstruction_label_882:

	// *** Basic block 311

.PrintInstruction_label_883:

	// *** Basic block 312

.PrintInstruction_label_884:
	ld          s5, 40(s2)
	beq         s5, x0, .PrintInstruction_label_891

	// *** Basic block 313

	j           .PrintInstruction_label_906

	// *** Basic block 314

.PrintInstruction_label_891:
	lla         a0, .str.157
	lla         a1, .str.158
	lla         a3, .str.159
	li          t0, 790		// 0x316
	mv          a2, t0
	call        printf

	// *** Basic block 315

	call        abort

	// *** Basic block 316

.PrintInstruction_label_906:
	ld          a0, 32(s2)
	beq         a0, x0, .PrintInstruction_label_913

	// *** Basic block 317

	j           .PrintInstruction_label_928

	// *** Basic block 318

.PrintInstruction_label_913:
	lla         a0, .str.160
	lla         a1, .str.161
	lla         a3, .str.162
	li          t0, 791		// 0x317
	mv          a2, t0
	call        printf

	// *** Basic block 319

	call        abort

	// *** Basic block 320

.PrintInstruction_label_928:
	call        RegisterSize

	// *** Basic block 321

	mv          a3, s1
	mv          a2, s5
	mv          a1, s2
	call        Copy

	// *** Basic block 322

	j           .PrintInstruction_label_1454

	// *** Basic block 323

.PrintInstruction_label_941:

	// *** Basic block 324

.PrintInstruction_label_942:

	// *** Basic block 325

.PrintInstruction_label_943:

	// *** Basic block 326

.PrintInstruction_label_944:

	// *** Basic block 327

.PrintInstruction_label_945:

	// *** Basic block 328

.PrintInstruction_label_946:
	addi        s5, s2, 40
	ld          s4, 40(s2)
	beq         s4, x0, .PrintInstruction_label_953

	// *** Basic block 329

	j           .PrintInstruction_label_968

	// *** Basic block 330

.PrintInstruction_label_953:
	lla         a0, .str.163
	lla         a1, .str.164
	lla         a3, .str.165
	li          t0, 801		// 0x321
	mv          a2, t0
	call        printf

	// *** Basic block 331

	call        abort

	// *** Basic block 332

.PrintInstruction_label_968:
	ld          s5, 8(s5)
	beq         s5, x0, .PrintInstruction_label_975

	// *** Basic block 333

	j           .PrintInstruction_label_990

	// *** Basic block 334

.PrintInstruction_label_975:
	lla         a0, .str.166
	lla         a1, .str.167
	lla         a3, .str.168
	li          t0, 802		// 0x322
	mv          a2, t0
	call        printf

	// *** Basic block 335

	call        abort

	// *** Basic block 336

.PrintInstruction_label_990:
	ld          a0, 32(s2)
	call        RegisterSize

	// *** Basic block 337

	mv          a3, s1
	mv          a2, s4
	mv          a1, s5
	call        Copy

	// *** Basic block 338

	j           .PrintInstruction_label_1454

	// *** Basic block 339

.PrintInstruction_label_1005:

	// *** Basic block 340

.PrintInstruction_label_1006:
	addi        s4, s2, 40
	ld          t0, 40(s2)
	beq         t0, x0, .PrintInstruction_label_1013

	// *** Basic block 341

	j           .PrintInstruction_label_1028

	// *** Basic block 342

.PrintInstruction_label_1013:
	lla         a0, .str.169
	lla         a1, .str.170
	lla         a3, .str.171
	li          t0, 808		// 0x328
	mv          a2, t0
	call        printf

	// *** Basic block 343

	call        abort

	// *** Basic block 344

.PrintInstruction_label_1028:
	ld          s4, 8(s4)
	beq         s4, x0, .PrintInstruction_label_1035

	// *** Basic block 345

	j           .PrintInstruction_label_1050

	// *** Basic block 346

.PrintInstruction_label_1035:
	lla         a0, .str.172
	lla         a1, .str.173
	lla         a3, .str.174
	li          t0, 809		// 0x329
	mv          a2, t0
	call        printf

	// *** Basic block 347

	call        abort

	// *** Basic block 348

.PrintInstruction_label_1050:
	addi        s5, s0, -52
	lla         a2, .str.175
	lw          a4, 20(s4)
	mv          a3, s3
	mv          a1, s6
	mv          a0, s5
	call        snprintf

	// *** Basic block 349

	mv          a3, s1
	mv          a2, s3
	mv          a1, s5
	mv          a0, s2
	call        EmitBranch

	// *** Basic block 350

	j           .PrintInstruction_label_1454

	// *** Basic block 351

.PrintInstruction_label_1077:
	ld          s5, 40(s2)
	beq         s5, x0, .PrintInstruction_label_1084

	// *** Basic block 352

	j           .PrintInstruction_label_1099

	// *** Basic block 353

.PrintInstruction_label_1084:
	lla         a0, .str.176
	lla         a1, .str.177
	lla         a3, .str.178
	li          s11, 817		// 0x331
	mv          a2, s11
	call        printf

	// *** Basic block 354

	call        abort

	// *** Basic block 355

.PrintInstruction_label_1099:
	lla         a1, .str.179
	lw          a3, 20(s5)
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 356

	j           .PrintInstruction_label_1454

	// *** Basic block 357

.PrintInstruction_label_1112:
	ld          s5, 40(s2)
	beq         s5, x0, .PrintInstruction_label_1119

	// *** Basic block 358

	j           .PrintInstruction_label_1134

	// *** Basic block 359

.PrintInstruction_label_1119:
	lla         a0, .str.180
	lla         a1, .str.181
	lla         a3, .str.182
	li          s11, 821		// 0x335
	mv          a2, s11
	call        printf

	// *** Basic block 360

	call        abort

	// *** Basic block 361

.PrintInstruction_label_1134:
	lla         a1, .str.183
	lw          a3, 20(s5)
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 362

	j           .PrintInstruction_label_1454

	// *** Basic block 363

.PrintInstruction_label_1147:

	// *** Basic block 364

.PrintInstruction_label_1148:

	// *** Basic block 365

.PrintInstruction_label_1149:

	// *** Basic block 366

.PrintInstruction_label_1150:

	// *** Basic block 367

.PrintInstruction_label_1151:

	// *** Basic block 368

.PrintInstruction_label_1152:

	// *** Basic block 369

.PrintInstruction_label_1153:

	// *** Basic block 370

.PrintInstruction_label_1154:

	// *** Basic block 371

.PrintInstruction_label_1155:

	// *** Basic block 372

.PrintInstruction_label_1156:

	// *** Basic block 373

.PrintInstruction_label_1157:

	// *** Basic block 374

.PrintInstruction_label_1158:

	// *** Basic block 375

.PrintInstruction_label_1159:

	// *** Basic block 376

.PrintInstruction_label_1160:

	// *** Basic block 377

.PrintInstruction_label_1161:

	// *** Basic block 378

.PrintInstruction_label_1162:

	// *** Basic block 379

.PrintInstruction_label_1163:

	// *** Basic block 380

.PrintInstruction_label_1164:

	// *** Basic block 381

.PrintInstruction_label_1165:

	// *** Basic block 382

.PrintInstruction_label_1166:

	// *** Basic block 383

.PrintInstruction_label_1167:

	// *** Basic block 384

.PrintInstruction_label_1168:

	// *** Basic block 385

.PrintInstruction_label_1169:

	// *** Basic block 386

.PrintInstruction_label_1170:

	// *** Basic block 387

.PrintInstruction_label_1171:

	// *** Basic block 388

.PrintInstruction_label_1172:

	// *** Basic block 389

.PrintInstruction_label_1173:

	// *** Basic block 390

.PrintInstruction_label_1174:

	// *** Basic block 391

.PrintInstruction_label_1175:

	// *** Basic block 392

.PrintInstruction_label_1176:

	// *** Basic block 393

.PrintInstruction_label_1177:

	// *** Basic block 394

.PrintInstruction_label_1178:

	// *** Basic block 395

.PrintInstruction_label_1179:

	// *** Basic block 396

.PrintInstruction_label_1180:

	// *** Basic block 397

.PrintInstruction_label_1181:

	// *** Basic block 398

.PrintInstruction_label_1182:

	// *** Basic block 399

.PrintInstruction_label_1183:

	// *** Basic block 400

.PrintInstruction_label_1184:

	// *** Basic block 401

.PrintInstruction_label_1185:

	// *** Basic block 402

.PrintInstruction_label_1186:

	// *** Basic block 403

.PrintInstruction_label_1187:

	// *** Basic block 404

.PrintInstruction_label_1188:

	// *** Basic block 405

.PrintInstruction_label_1189:

	// *** Basic block 406

.PrintInstruction_label_1190:

	// *** Basic block 407

.PrintInstruction_label_1191:

	// *** Basic block 408

.PrintInstruction_label_1192:

	// *** Basic block 409

.PrintInstruction_label_1193:

	// *** Basic block 410

.PrintInstruction_label_1194:

	// *** Basic block 411

.PrintInstruction_label_1195:

	// *** Basic block 412

.PrintInstruction_label_1196:

	// *** Basic block 413

.PrintInstruction_label_1197:

	// *** Basic block 414

.PrintInstruction_label_1198:
	mv          a2, s1
	mv          a1, s3
	mv          a0, s2
	call        Compare

	// *** Basic block 415

	j           .PrintInstruction_label_1454

	// *** Basic block 416

.PrintInstruction_label_1207:
	ld          a1, 32(s2)
	addi        t0, s2, 40
	ld          a2, 40(s2)
	ld          a3, 8(t0)
	mv          a4, s1
	mv          a0, s8
	call        Add

	// *** Basic block 417

	j           .PrintInstruction_label_1454

	// *** Basic block 418

.PrintInstruction_label_1224:
	addi        t0, s2, 40
	ld          s5, 8(t0)
	mv          a0, s5
	call        TargetIntValue

	// *** Basic block 419

	li          t0, 1		// 0x1 ASCII \x1
	bne         a0, t0, .PrintInstruction_label_1253

	// *** Basic block 420

	ld          a1, 32(s2)
	ld          a3, 40(s2)
	mv          a5, s1
	mv          a4, s3
	mv          a2, s2
	mv          a0, s8
	call        Inc

	// *** Basic block 421

	j           .PrintInstruction_label_1267

	// *** Basic block 422

.PrintInstruction_label_1253:
	ld          a1, 32(s2)
	ld          a2, 40(s2)
	mv          a4, s1
	mv          a3, s5
	mv          a0, s8
	call        Add

	// *** Basic block 423

.PrintInstruction_label_1267:
	j           .PrintInstruction_label_1454

	// *** Basic block 424

.PrintInstruction_label_1269:
	ld          s11, 32(s2)
	mv          a0, s11
	call        RegisterSize

	// *** Basic block 425

	addi        t0, s2, 40
	ld          a2, 40(s2)
	ld          a3, 8(t0)
	mv          a4, s1
	mv          a1, s11
	call        Add

	// *** Basic block 426

	j           .PrintInstruction_label_1454

	// *** Basic block 427

.PrintInstruction_label_1289:
	addi        t0, s2, 40
	ld          s11, 8(t0)
	mv          a0, s11
	call        TargetIntValue

	// *** Basic block 428

	li          t0, 1		// 0x1 ASCII \x1
	bne         a0, t0, .PrintInstruction_label_1319

	// *** Basic block 429

	ld          s5, 32(s2)
	mv          a0, s5
	call        RegisterSize

	// *** Basic block 430

	ld          a3, 40(s2)
	mv          a5, s1
	mv          a4, s3
	mv          a2, s2
	mv          a1, s5
	call        Inc

	// *** Basic block 431

	j           .PrintInstruction_label_1336

	// *** Basic block 432

.PrintInstruction_label_1319:
	ld          s5, 32(s2)
	mv          a0, s5
	call        RegisterSize

	// *** Basic block 433

	ld          a2, 40(s2)
	mv          a4, s1
	mv          a3, s11
	mv          a1, s5
	call        Add

	// *** Basic block 434

.PrintInstruction_label_1336:
	j           .PrintInstruction_label_1454

	// *** Basic block 435

.PrintInstruction_label_1338:

	// *** Basic block 436

.PrintInstruction_label_1339:
	j           .PrintInstruction_label_1454

	// *** Basic block 437

.PrintInstruction_label_1341:
	ld          s5, 32(s2)
	mv          a0, s5
	call        RegisterSize

	// *** Basic block 438

	addi        t0, s2, 40
	ld          a2, 40(s2)
	ld          a3, 8(t0)
	mv          a4, s1
	mv          a1, s5
	call        Sub

	// *** Basic block 439

	j           .PrintInstruction_label_1454

	// *** Basic block 440

.PrintInstruction_label_1361:

	// *** Basic block 441

.PrintInstruction_label_1362:
	j           .PrintInstruction_label_1454

	// *** Basic block 442

.PrintInstruction_label_1364:

	// *** Basic block 443

.PrintInstruction_label_1365:

	// *** Basic block 444

.PrintInstruction_label_1366:

	// *** Basic block 445

.PrintInstruction_label_1367:

	// *** Basic block 446

.PrintInstruction_label_1368:

	// *** Basic block 447

.PrintInstruction_label_1369:

	// *** Basic block 448

.PrintInstruction_label_1370:

	// *** Basic block 449

.PrintInstruction_label_1371:

	// *** Basic block 450

.PrintInstruction_label_1372:

	// *** Basic block 451

.PrintInstruction_label_1373:

	// *** Basic block 452

.PrintInstruction_label_1374:

	// *** Basic block 453

.PrintInstruction_label_1375:

	// *** Basic block 454

.PrintInstruction_label_1376:

	// *** Basic block 455

.PrintInstruction_label_1377:

	// *** Basic block 456

.PrintInstruction_label_1378:
	ld          s4, 32(s2)
	mv          a0, s4
	call        RegisterSize

	// *** Basic block 457

	addi        t0, s2, 40
	ld          a2, 40(s2)
	ld          a3, 8(t0)
	mv          a4, s1
	mv          a1, s4
	call        Ora

	// *** Basic block 458

	j           .PrintInstruction_label_1454

	// *** Basic block 459

.PrintInstruction_label_1398:
	ld          s5, 32(s2)
	mv          a0, s5
	call        RegisterSize

	// *** Basic block 460

	addi        t0, s2, 40
	ld          a2, 40(s2)
	ld          a3, 8(t0)
	mv          a4, s1
	mv          a1, s5
	call        And

	// *** Basic block 461

	j           .PrintInstruction_label_1454

	// *** Basic block 462

.PrintInstruction_label_1418:
	ld          s5, 32(s2)
	mv          a0, s5
	call        RegisterSize

	// *** Basic block 463

	addi        t0, s2, 40
	ld          a2, 40(s2)
	ld          a3, 8(t0)
	mv          a4, s1
	mv          a1, s5
	call        Eor

	// *** Basic block 464

	j           .PrintInstruction_label_1454

	// *** Basic block 465

.PrintInstruction_label_1438:

	// *** Basic block 466

.PrintInstruction_label_1439:
	j           .PrintInstruction_label_1454

	// *** Basic block 467

.PrintInstruction_label_1441:

	// *** Basic block 468

.PrintInstruction_label_1442:

	// *** Basic block 469

.PrintInstruction_label_1443:
	j           .PrintInstruction_label_1454

	// *** Basic block 470

.PrintInstruction_label_1445:
	la          t0, stdout
	ld          a1, 0(t0)
	mv          a0, s2
	call        W65C02PrintInstruction

	// *** Basic block 471

	call        abort

	// *** Basic block 472

	j           .PrintInstruction_label_1454

	// *** Basic block 473

.PrintInstruction_label_1454:
	j           .PrintInstruction_label_169
.func_end_PrintInstruction:
	.size PrintInstruction, .func_end_PrintInstruction-PrintInstruction

	.global W65C02EmitterInit
	.type W65C02EmitterInit, @function

W65C02EmitterInit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sd          a1, 0(a0)
	addi        t0, a1, 192
	sd          t0, 8(a0)
	ret         
.func_end_W65C02EmitterInit:
	.size W65C02EmitterInit, .func_end_W65C02EmitterInit-W65C02EmitterInit

	.global New6502Emitter
	.type New6502Emitter, @function

New6502Emitter:

	// *** Basic block 0

	.global malloc
	.global W65C02EmitterInit
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
	li          a0, 16		// 0x10 ASCII \x10
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        W65C02EmitterInit

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.New6502Emitter_label_21:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_New6502Emitter:
	.size New6502Emitter, .func_end_New6502Emitter-New6502Emitter

	.global W65C02EmitterDestruct
	.type W65C02EmitterDestruct, @function

W65C02EmitterDestruct:

	// *** Basic block 0

	ret         
.func_end_W65C02EmitterDestruct:
	.size W65C02EmitterDestruct, .func_end_W65C02EmitterDestruct-W65C02EmitterDestruct

	.global W65C02EmitterDelete
	.type W65C02EmitterDelete, @function

W65C02EmitterDelete:

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
	.global W65C02EmitterDestruct
	.global free
	mv          s1, a0
	call        W65C02EmitterDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_W65C02EmitterDelete:
	.size W65C02EmitterDelete, .func_end_W65C02EmitterDelete-W65C02EmitterDelete

	.global W65C02PrintFunction
	.type W65C02PrintFunction, @function

W65C02PrintFunction:

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
	beqz        t0, .W65C02PrintFunction_label_39

	// *** Basic block 1

	lla         a1, .str.184
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 2

	j           .W65C02PrintFunction_label_48

	// *** Basic block 3

.W65C02PrintFunction_label_39:
	lla         a1, .str.185
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 4

.W65C02PrintFunction_label_48:
	lla         a1, .str.186
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 5

	lla         a1, .str.187
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 6

	ld          a0, 0(s1)
	call        TargetFirstInstruction

	// *** Basic block 7

	mv          s4, a0
	beq         s4, x0, .W65C02PrintFunction_label_91

	// *** Basic block 8

.W65C02PrintFunction_label_73:
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
	bne         s4, x0, .W65C02PrintFunction_label_73

	// *** Basic block 11

.W65C02PrintFunction_label_91:
	lla         a1, .str.188
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 12

	lla         a1, .str.189
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
.func_end_W65C02PrintFunction:
	.size W65C02PrintFunction, .func_end_W65C02PrintFunction-W65C02PrintFunction

.PCend:
	.data
comparisons:
	.type   comparisons,@object
	.local  comparisons
	.size   comparisons,832
	.p2align  3
	.word   114
	.word   1
	.byte   1
	.byte   0
	.space  2
	.word   1
	.word   115
	.word   1
	.byte   1
	.byte   0
	.space  2
	.word   2
	.word   116
	.word   1
	.byte   1
	.byte   0
	.space  2
	.word   3
	.word   117
	.word   1
	.byte   1
	.byte   0
	.space  2
	.word   4
	.word   118
	.word   1
	.byte   1
	.byte   0
	.space  2
	.word   5
	.word   119
	.word   1
	.byte   1
	.byte   0
	.space  2
	.word   6
	.word   120
	.word   1
	.byte   0
	.byte   0
	.space  2
	.word   3
	.word   121
	.word   1
	.byte   0
	.byte   0
	.space  2
	.word   4
	.word   122
	.word   1
	.byte   0
	.byte   0
	.space  2
	.word   5
	.word   123
	.word   1
	.byte   0
	.byte   0
	.space  2
	.word   6
	.word   124
	.word   2
	.byte   1
	.byte   0
	.space  2
	.word   1
	.word   125
	.word   2
	.byte   1
	.byte   0
	.space  2
	.word   2
	.word   126
	.word   2
	.byte   1
	.byte   0
	.space  2
	.word   3
	.word   127
	.word   2
	.byte   1
	.byte   0
	.space  2
	.word   4
	.word   128
	.word   2
	.byte   1
	.byte   0
	.space  2
	.word   5
	.word   129
	.word   2
	.byte   1
	.byte   0
	.space  2
	.word   6
	.word   130
	.word   2
	.byte   0
	.byte   0
	.space  2
	.word   3
	.word   131
	.word   2
	.byte   0
	.byte   0
	.space  2
	.word   4
	.word   132
	.word   2
	.byte   0
	.byte   0
	.space  2
	.word   5
	.word   133
	.word   2
	.byte   0
	.byte   0
	.space  2
	.word   6
	.word   134
	.word   4
	.byte   1
	.byte   0
	.space  2
	.word   1
	.word   135
	.word   4
	.byte   1
	.byte   0
	.space  2
	.word   2
	.word   136
	.word   4
	.byte   1
	.byte   0
	.space  2
	.word   3
	.word   137
	.word   4
	.byte   1
	.byte   0
	.space  2
	.word   4
	.word   138
	.word   4
	.byte   1
	.byte   0
	.space  2
	.word   5
	.word   139
	.word   4
	.byte   1
	.byte   0
	.space  2
	.word   6
	.word   140
	.word   4
	.byte   0
	.byte   0
	.space  2
	.word   3
	.word   141
	.word   4
	.byte   0
	.byte   0
	.space  2
	.word   4
	.word   142
	.word   4
	.byte   0
	.byte   0
	.space  2
	.word   5
	.word   143
	.word   4
	.byte   0
	.byte   0
	.space  2
	.word   6
	.word   144
	.word   8
	.byte   1
	.byte   0
	.space  2
	.word   1
	.word   145
	.word   8
	.byte   1
	.byte   0
	.space  2
	.word   2
	.word   146
	.word   8
	.byte   1
	.byte   0
	.space  2
	.word   3
	.word   147
	.word   8
	.byte   1
	.byte   0
	.space  2
	.word   4
	.word   148
	.word   8
	.byte   1
	.byte   0
	.space  2
	.word   5
	.word   149
	.word   8
	.byte   1
	.byte   0
	.space  2
	.word   6
	.word   150
	.word   8
	.byte   0
	.byte   0
	.space  2
	.word   3
	.word   151
	.word   8
	.byte   0
	.byte   0
	.space  2
	.word   4
	.word   152
	.word   8
	.byte   0
	.byte   0
	.space  2
	.word   5
	.word   153
	.word   8
	.byte   0
	.byte   0
	.space  2
	.word   6
	.word   154
	.word   4
	.byte   1
	.byte   1
	.space  2
	.word   1
	.word   155
	.word   4
	.byte   1
	.byte   1
	.space  2
	.word   2
	.word   156
	.word   4
	.byte   1
	.byte   1
	.space  2
	.word   3
	.word   157
	.word   4
	.byte   1
	.byte   1
	.space  2
	.word   4
	.word   158
	.word   4
	.byte   1
	.byte   1
	.space  2
	.word   5
	.word   159
	.word   4
	.byte   1
	.byte   1
	.space  2
	.word   6
	.word   160
	.word   8
	.byte   1
	.byte   1
	.space  2
	.word   1
	.word   161
	.word   8
	.byte   1
	.byte   1
	.space  2
	.word   2
	.word   162
	.word   8
	.byte   1
	.byte   1
	.space  2
	.word   3
	.word   163
	.word   8
	.byte   1
	.byte   1
	.space  2
	.word   4
	.word   164
	.word   8
	.byte   1
	.byte   1
	.space  2
	.word   5
	.word   165
	.word   8
	.byte   1
	.byte   1
	.space  2
	.word   6

	.type   buf,@object
	.local  buf
	.comm   buf,4096,1

	.type   buf,@object
	.local  buf
	.comm   buf,4096,1

	.type   buf,@object
	.local  buf
	.comm   buf,4096,1

	.type   buf,@object
	.local  buf
	.comm   buf,4096,1

	.type   buf,@object
	.local  buf
	.comm   buf,4096,1

	.type   buf2,@object
	.local  buf2
	.comm   buf2,4096,1

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "\t.hword %d\n"
	.type .str.1, @object
	.size .str.1, 12

.str.2:
	.asciz "\tLDY #%d\n"
	.type .str.2, @object
	.size .str.2, 10

.str.3:
	.asciz "\tINY\n"
	.type .str.3, @object
	.size .str.3, 6

.str.4:
	.asciz "\tLDA %s\n"
	.type .str.4, @object
	.size .str.4, 9

.str.5:
	.asciz "\t%s %s\n"
	.type .str.5, @object
	.size .str.5, 8

.str.6:
	.asciz "\tSTA %s\n"
	.type .str.6, @object
	.size .str.6, 9

.str.7:
	.asciz "\tSTA %s\n"
	.type .str.7, @object
	.size .str.7, 9

.str.8:
	.asciz "\tSTZ %s\n"
	.type .str.8, @object
	.size .str.8, 9

.str.9:
	.asciz "\tLDA #0\n"
	.type .str.9, @object
	.size .str.9, 9

.str.10:
	.asciz "\tLDA #%d\n"
	.type .str.10, @object
	.size .str.10, 10

.str.11:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.11, @object
	.size .str.11, 30

.str.12:
	.asciz "(null)"
	.type .str.12, @object
	.size .str.12, 1

.str.13:
	.asciz "inst->operand[0] != NULL"
	.type .str.13, @object
	.size .str.13, 25

.str.14:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.14, @object
	.size .str.14, 30

.str.15:
	.asciz "(null)"
	.type .str.15, @object
	.size .str.15, 1

.str.16:
	.asciz "inst->operand[1] != NULL"
	.type .str.16, @object
	.size .str.16, 25

.str.17:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.17, @object
	.size .str.17, 30

.str.18:
	.asciz "(null)"
	.type .str.18, @object
	.size .str.18, 1

.str.19:
	.asciz "inst->operand[0]->reg != NULL"
	.type .str.19, @object
	.size .str.19, 30

.str.20:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.20, @object
	.size .str.20, 30

.str.21:
	.asciz "(null)"
	.type .str.21, @object
	.size .str.21, 1

.str.22:
	.asciz "inst->operand[1]->reg != NULL"
	.type .str.22, @object
	.size .str.22, 30

.str.23:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.23, @object
	.size .str.23, 30

.str.24:
	.asciz "(null)"
	.type .str.24, @object
	.size .str.24, 1

.str.25:
	.asciz "false"
	.type .str.25, @object
	.size .str.25, 6

.str.26:
	.asciz ".%s_cmp_false_%d"
	.type .str.26, @object
	.size .str.26, 17

.str.27:
	.asciz ".%s_cmp_true_%d"
	.type .str.27, @object
	.size .str.27, 16

.str.28:
	.asciz "\tLDX #0\n"
	.type .str.28, @object
	.size .str.28, 9

.str.29:
	.asciz "CMP"
	.type .str.29, @object
	.size .str.29, 4

.str.30:
	.asciz "BNE %s\n"
	.type .str.30, @object
	.size .str.30, 8

.str.31:
	.asciz "\tINX\n"
	.type .str.31, @object
	.size .str.31, 6

.str.32:
	.asciz "%s:\n"
	.type .str.32, @object
	.size .str.32, 5

.str.33:
	.asciz "CMP"
	.type .str.33, @object
	.size .str.33, 4

.str.34:
	.asciz "BEQ %s\n"
	.type .str.34, @object
	.size .str.34, 8

.str.35:
	.asciz "\tINX\n"
	.type .str.35, @object
	.size .str.35, 6

.str.36:
	.asciz "%s:\n"
	.type .str.36, @object
	.size .str.36, 5

.str.37:
	.asciz "CMP"
	.type .str.37, @object
	.size .str.37, 4

.str.38:
	.asciz ".cmp_vc_%d"
	.type .str.38, @object
	.size .str.38, 11

.str.39:
	.asciz "SBC"
	.type .str.39, @object
	.size .str.39, 4

.str.40:
	.asciz "\tBVC %s\n"
	.type .str.40, @object
	.size .str.40, 9

.str.41:
	.asciz "\tEOR #0x80\n"
	.type .str.41, @object
	.size .str.41, 12

.str.42:
	.asciz "%s:\n"
	.type .str.42, @object
	.size .str.42, 5

.str.43:
	.asciz "\tBPL %s\n"
	.type .str.43, @object
	.size .str.43, 9

.str.44:
	.asciz "\tINX\n"
	.type .str.44, @object
	.size .str.44, 6

.str.45:
	.asciz "%s:\n"
	.type .str.45, @object
	.size .str.45, 5

.str.46:
	.asciz "CMP"
	.type .str.46, @object
	.size .str.46, 4

.str.47:
	.asciz "BCC %s\n"
	.type .str.47, @object
	.size .str.47, 8

.str.48:
	.asciz "BNE %s\n"
	.type .str.48, @object
	.size .str.48, 8

.str.49:
	.asciz "%s:\n"
	.type .str.49, @object
	.size .str.49, 5

.str.50:
	.asciz "\tINX\n"
	.type .str.50, @object
	.size .str.50, 6

.str.51:
	.asciz "%s:\n"
	.type .str.51, @object
	.size .str.51, 5

.str.52:
	.asciz "\tSTX %s\n"
	.type .str.52, @object
	.size .str.52, 9

.str.53:
	.asciz ".%s_bra_false_%d"
	.type .str.53, @object
	.size .str.53, 17

.str.54:
	.asciz "BMI"
	.type .str.54, @object
	.size .str.54, 4

.str.55:
	.asciz "BPL"
	.type .str.55, @object
	.size .str.55, 4

.str.56:
	.asciz "\t%s %s\n"
	.type .str.56, @object
	.size .str.56, 8

.str.57:
	.asciz "BEQ"
	.type .str.57, @object
	.size .str.57, 4

.str.58:
	.asciz "BNE"
	.type .str.58, @object
	.size .str.58, 4

.str.59:
	.asciz "\t%s %s\n"
	.type .str.59, @object
	.size .str.59, 8

.str.60:
	.asciz "BPL"
	.type .str.60, @object
	.size .str.60, 4

.str.61:
	.asciz "BMI"
	.type .str.61, @object
	.size .str.61, 4

.str.62:
	.asciz "\tBNE %s\n"
	.type .str.62, @object
	.size .str.62, 9

.str.63:
	.asciz "\t%s %s\n"
	.type .str.63, @object
	.size .str.63, 8

.str.64:
	.asciz "%s:\n"
	.type .str.64, @object
	.size .str.64, 5

.str.65:
	.asciz "BNE"
	.type .str.65, @object
	.size .str.65, 4

.str.66:
	.asciz "BEQ"
	.type .str.66, @object
	.size .str.66, 4

.str.67:
	.asciz "\t%s %s\n"
	.type .str.67, @object
	.size .str.67, 8

.str.68:
	.asciz "BMI"
	.type .str.68, @object
	.size .str.68, 4

.str.69:
	.asciz "BPL"
	.type .str.69, @object
	.size .str.69, 4

.str.70:
	.asciz "\tBEQ %s\n"
	.type .str.70, @object
	.size .str.70, 9

.str.71:
	.asciz "\t%s %s\n"
	.type .str.71, @object
	.size .str.71, 8

.str.72:
	.asciz "%s:\n"
	.type .str.72, @object
	.size .str.72, 5

.str.73:
	.asciz "BPL"
	.type .str.73, @object
	.size .str.73, 4

.str.74:
	.asciz "BMI"
	.type .str.74, @object
	.size .str.74, 4

.str.75:
	.asciz "\t%s %s\n"
	.type .str.75, @object
	.size .str.75, 8

.str.76:
	.asciz "BCC"
	.type .str.76, @object
	.size .str.76, 4

.str.77:
	.asciz "BCS"
	.type .str.77, @object
	.size .str.77, 4

.str.78:
	.asciz "\t%s %s\n"
	.type .str.78, @object
	.size .str.78, 8

.str.79:
	.asciz "BEQ"
	.type .str.79, @object
	.size .str.79, 4

.str.80:
	.asciz "BNE"
	.type .str.80, @object
	.size .str.80, 4

.str.81:
	.asciz "\t%s %s\n"
	.type .str.81, @object
	.size .str.81, 8

.str.82:
	.asciz "BCS"
	.type .str.82, @object
	.size .str.82, 4

.str.83:
	.asciz "BCC"
	.type .str.83, @object
	.size .str.83, 4

.str.84:
	.asciz "\tBNE %s\n"
	.type .str.84, @object
	.size .str.84, 9

.str.85:
	.asciz "\t%s %s\n"
	.type .str.85, @object
	.size .str.85, 8

.str.86:
	.asciz "%s:\n"
	.type .str.86, @object
	.size .str.86, 5

.str.87:
	.asciz "BNE"
	.type .str.87, @object
	.size .str.87, 4

.str.88:
	.asciz "BEQ"
	.type .str.88, @object
	.size .str.88, 4

.str.89:
	.asciz "\t%s %s\n"
	.type .str.89, @object
	.size .str.89, 8

.str.90:
	.asciz "BCC"
	.type .str.90, @object
	.size .str.90, 4

.str.91:
	.asciz "BCS"
	.type .str.91, @object
	.size .str.91, 4

.str.92:
	.asciz "\tBEQ %s\n"
	.type .str.92, @object
	.size .str.92, 9

.str.93:
	.asciz "\t%s %s\n"
	.type .str.93, @object
	.size .str.93, 8

.str.94:
	.asciz "%s:\n"
	.type .str.94, @object
	.size .str.94, 5

.str.95:
	.asciz "BCS"
	.type .str.95, @object
	.size .str.95, 4

.str.96:
	.asciz "BCC"
	.type .str.96, @object
	.size .str.96, 4

.str.97:
	.asciz "\t%s %s\n"
	.type .str.97, @object
	.size .str.97, 8

.str.98:
	.asciz "\tJSR __enter\n"
	.type .str.98, @object
	.size .str.98, 14

.str.99:
	.asciz "\t.hword %d\n"
	.type .str.99, @object
	.size .str.99, 12

.str.100:
	.asciz "\tJSR __rts\n"
	.type .str.100, @object
	.size .str.100, 12

.str.101:
	.asciz "\t.hword %d\n"
	.type .str.101, @object
	.size .str.101, 12

.str.102:
	.asciz "\tCLC\n"
	.type .str.102, @object
	.size .str.102, 6

.str.103:
	.asciz "ADC"
	.type .str.103, @object
	.size .str.103, 4

.str.104:
	.asciz "\tSEC\n"
	.type .str.104, @object
	.size .str.104, 6

.str.105:
	.asciz "SBC"
	.type .str.105, @object
	.size .str.105, 4

.str.106:
	.asciz "AND"
	.type .str.106, @object
	.size .str.106, 4

.str.107:
	.asciz "ORA"
	.type .str.107, @object
	.size .str.107, 4

.str.108:
	.asciz "EOR"
	.type .str.108, @object
	.size .str.108, 4

.str.109:
	.asciz ".%s_inc_%d"
	.type .str.109, @object
	.size .str.109, 11

.str.110:
	.asciz "INC"
	.type .str.110, @object
	.size .str.110, 4

.str.111:
	.asciz "\tBCC %s\n"
	.type .str.111, @object
	.size .str.111, 9

.str.112:
	.asciz "%s:\n"
	.type .str.112, @object
	.size .str.112, 5

.str.113:
	.asciz "\tPHA\n"
	.type .str.113, @object
	.size .str.113, 6

.str.114:
	.asciz "\tJSR __push_reg_%d\n"
	.type .str.114, @object
	.size .str.114, 20

.str.115:
	.asciz "\tJSR __pull_reg_%d\n"
	.type .str.115, @object
	.size .str.115, 20

.str.116:
	.asciz "\tPLA\n"
	.type .str.116, @object
	.size .str.116, 6

.str.117:
	.asciz "// "
	.type .str.117, @object
	.size .str.117, 4

.str.118:
	.asciz ".%s_label_%d:\n"
	.type .str.118, @object
	.size .str.118, 15

.str.119:
	.asciz "%s:\n"
	.type .str.119, @object
	.size .str.119, 5

.str.120:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.120, @object
	.size .str.120, 30

.str.121:
	.asciz "(null)"
	.type .str.121, @object
	.size .str.121, 1

.str.122:
	.asciz "TargetIsConst(inst->operand[0])"
	.type .str.122, @object
	.size .str.122, 32

.str.123:
	.asciz "\t.local %s\n"
	.type .str.123, @object
	.size .str.123, 12

.str.124:
	.asciz "\t.global %s\n"
	.type .str.124, @object
	.size .str.124, 13

.str.125:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.125, @object
	.size .str.125, 30

.str.126:
	.asciz "(null)"
	.type .str.126, @object
	.size .str.126, 1

.str.127:
	.asciz "TargetIsConst(inst->operand[0])"
	.type .str.127, @object
	.size .str.127, 32

.str.128:
	.asciz "\tSEC\n"
	.type .str.128, @object
	.size .str.128, 6

.str.129:
	.asciz "\tLDA sp\n"
	.type .str.129, @object
	.size .str.129, 9

.str.130:
	.asciz "\tSBC #%d\n"
	.type .str.130, @object
	.size .str.130, 10

.str.131:
	.asciz "\tSTA sp\n"
	.type .str.131, @object
	.size .str.131, 9

.str.132:
	.asciz "\tLDA sp+1\n"
	.type .str.132, @object
	.size .str.132, 11

.str.133:
	.asciz "\tSBC #%d\n"
	.type .str.133, @object
	.size .str.133, 10

.str.134:
	.asciz "\tSTA sp+1\n"
	.type .str.134, @object
	.size .str.134, 11

.str.135:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.135, @object
	.size .str.135, 30

.str.136:
	.asciz "(null)"
	.type .str.136, @object
	.size .str.136, 1

.str.137:
	.asciz "TargetIsConst(inst->operand[0])"
	.type .str.137, @object
	.size .str.137, 32

.str.138:
	.asciz "\tCLC\n"
	.type .str.138, @object
	.size .str.138, 6

.str.139:
	.asciz "\tLDA sp\n"
	.type .str.139, @object
	.size .str.139, 9

.str.140:
	.asciz "\tADC #%d\n"
	.type .str.140, @object
	.size .str.140, 10

.str.141:
	.asciz "\tSTA sp\n"
	.type .str.141, @object
	.size .str.141, 9

.str.142:
	.asciz "\tLDA sp+1\n"
	.type .str.142, @object
	.size .str.142, 11

.str.143:
	.asciz "\tADC #%d\n"
	.type .str.143, @object
	.size .str.143, 10

.str.144:
	.asciz "\tSTA sp+1\n"
	.type .str.144, @object
	.size .str.144, 11

.str.145:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.145, @object
	.size .str.145, 30

.str.146:
	.asciz "(null)"
	.type .str.146, @object
	.size .str.146, 1

.str.147:
	.asciz "inst->operand[0]->opcode == W65C02_OP(symbol)"
	.type .str.147, @object
	.size .str.147, 45

.str.148:
	.asciz "\t%-8s %s\n"
	.type .str.148, @object
	.size .str.148, 10

.str.149:
	.asciz "JSR"
	.type .str.149, @object
	.size .str.149, 4

.str.150:
	.asciz "\t%-8s %s\n"
	.type .str.150, @object
	.size .str.150, 10

.str.151:
	.asciz "JSR"
	.type .str.151, @object
	.size .str.151, 4

.str.152:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.152, @object
	.size .str.152, 30

.str.153:
	.asciz "(null)"
	.type .str.153, @object
	.size .str.153, 1

.str.154:
	.asciz "lit != NULL"
	.type .str.154, @object
	.size .str.154, 12

.str.155:
	.asciz "\t%s\n"
	.type .str.155, @object
	.size .str.155, 5

.str.156:
	.asciz "\t.loc %d %d %d\n"
	.type .str.156, @object
	.size .str.156, 16

.str.157:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.157, @object
	.size .str.157, 30

.str.158:
	.asciz "(null)"
	.type .str.158, @object
	.size .str.158, 1

.str.159:
	.asciz "inst->operand[0] != NULL"
	.type .str.159, @object
	.size .str.159, 25

.str.160:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.160, @object
	.size .str.160, 30

.str.161:
	.asciz "(null)"
	.type .str.161, @object
	.size .str.161, 1

.str.162:
	.asciz "inst->reg != NULL"
	.type .str.162, @object
	.size .str.162, 18

.str.163:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.163, @object
	.size .str.163, 30

.str.164:
	.asciz "(null)"
	.type .str.164, @object
	.size .str.164, 1

.str.165:
	.asciz "inst->operand[0] != NULL"
	.type .str.165, @object
	.size .str.165, 25

.str.166:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.166, @object
	.size .str.166, 30

.str.167:
	.asciz "(null)"
	.type .str.167, @object
	.size .str.167, 1

.str.168:
	.asciz "inst->operand[1] != NULL"
	.type .str.168, @object
	.size .str.168, 25

.str.169:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.169, @object
	.size .str.169, 30

.str.170:
	.asciz "(null)"
	.type .str.170, @object
	.size .str.170, 1

.str.171:
	.asciz "inst->operand[0] != NULL"
	.type .str.171, @object
	.size .str.171, 25

.str.172:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.172, @object
	.size .str.172, 30

.str.173:
	.asciz "(null)"
	.type .str.173, @object
	.size .str.173, 1

.str.174:
	.asciz "inst->operand[1] != NULL"
	.type .str.174, @object
	.size .str.174, 25

.str.175:
	.asciz ".%s_label_%d"
	.type .str.175, @object
	.size .str.175, 13

.str.176:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.176, @object
	.size .str.176, 30

.str.177:
	.asciz "(null)"
	.type .str.177, @object
	.size .str.177, 1

.str.178:
	.asciz "inst->operand[0] != NULL"
	.type .str.178, @object
	.size .str.178, 25

.str.179:
	.asciz "\tBRA .%s_label_%d\n"
	.type .str.179, @object
	.size .str.179, 19

.str.180:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.180, @object
	.size .str.180, 30

.str.181:
	.asciz "(null)"
	.type .str.181, @object
	.size .str.181, 1

.str.182:
	.asciz "inst->operand[0] != NULL"
	.type .str.182, @object
	.size .str.182, 25

.str.183:
	.asciz "\tJMP .%s_label_%d\n"
	.type .str.183, @object
	.size .str.183, 19

.str.184:
	.asciz "\t.global %s\n"
	.type .str.184, @object
	.size .str.184, 13

.str.185:
	.asciz "\t.local  %s\n"
	.type .str.185, @object
	.size .str.185, 13

.str.186:
	.asciz "\t.type %s, @function\n\n"
	.type .str.186, @object
	.size .str.186, 23

.str.187:
	.asciz "%s:\n"
	.type .str.187, @object
	.size .str.187, 5

.str.188:
	.asciz ".func_end_%s:\n"
	.type .str.188, @object
	.size .str.188, 15

.str.189:
	.asciz "\t.size %s, .func_end_%s-%s\n\n"
	.type .str.189, @object
	.size .str.189, 29

