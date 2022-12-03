	.file   "6502/6502_reg_alloc.c"
	.text
	.option pic
.PCbegin:
	.local  InitializeZeroPageRegister
	.type InitializeZeroPageRegister, @function

InitializeZeroPageRegister:

	// *** Basic block 0

	.global TargetRegisterInit
	// Leaf procedure, no stack frame generated
	j           TargetRegisterInit
.func_end_InitializeZeroPageRegister:
	.size InitializeZeroPageRegister, .func_end_InitializeZeroPageRegister-InitializeZeroPageRegister

	.local  AllocateNewSymbolRegister
	.type AllocateNewSymbolRegister, @function

AllocateNewSymbolRegister:

	// *** Basic block 0

	.global malloc
	.global TargetRegisterInit
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
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	mv          s4, a3
	li          a0, 32		// 0x20 ASCII ' '
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	addi        t0, s1, 968
	ld          t0, 8(t0)
	addi        a1, t0, 100
	mv          a0, s5
	call        TargetRegisterInit

	// *** Basic block 2

	sw          s2, 16(s5)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 20(s5)
	sd          s3, 24(s5)
	sd          s4, 8(s5)
	sb          t0, 4(s5)
	addi        a0, s1, 968
	mv          a1, s5
	call        VectorAppend

	// *** Basic block 3

	mv          a0, s5

	// *** Basic block 4

.AllocateNewSymbolRegister_label_61:
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
.func_end_AllocateNewSymbolRegister:
	.size AllocateNewSymbolRegister, .func_end_AllocateNewSymbolRegister-AllocateNewSymbolRegister

	.local  AllocateNewOffsetRegister
	.type AllocateNewOffsetRegister, @function

AllocateNewOffsetRegister:

	// *** Basic block 0

	.global malloc
	.global TargetRegisterInit
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
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	mv          s4, a3
	mv          s5, a4
	li          a0, 32		// 0x20 ASCII ' '
	call        malloc

	// *** Basic block 1

	mv          s6, a0
	addi        t0, s1, 968
	ld          t0, 8(t0)
	addi        a1, t0, 100
	mv          a0, s6
	call        TargetRegisterInit

	// *** Basic block 2

	sw          s2, 16(s6)
	sw          s3, 20(s6)
	addi        t0, s6, 24
	sh          s4, 0(t0)
	sd          s5, 8(s6)
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 4(s6)
	addi        a0, s1, 968
	mv          a1, s6
	call        VectorAppend

	// *** Basic block 3

	mv          a0, s6

	// *** Basic block 4

.AllocateNewOffsetRegister_label_67:
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
.func_end_AllocateNewOffsetRegister:
	.size AllocateNewOffsetRegister, .func_end_AllocateNewOffsetRegister-AllocateNewOffsetRegister

	.local  AllocateNewConstantRegister
	.type AllocateNewConstantRegister, @function

AllocateNewConstantRegister:

	// *** Basic block 0

	.global malloc
	.global TargetRegisterInit
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
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	mv          s4, a3
	li          a0, 32		// 0x20 ASCII ' '
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	addi        t0, s1, 968
	ld          t0, 8(t0)
	addi        a1, t0, 100
	mv          a0, s5
	call        TargetRegisterInit

	// *** Basic block 2

	sw          s2, 16(s5)
	li          t0, 4		// 0x4 ASCII \x4
	sw          t0, 20(s5)
	sd          s3, 24(s5)
	sd          s4, 8(s5)
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 4(s5)
	addi        a0, s1, 968
	mv          a1, s5
	call        VectorAppend

	// *** Basic block 3

	mv          a0, s5

	// *** Basic block 4

.AllocateNewConstantRegister_label_61:
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
.func_end_AllocateNewConstantRegister:
	.size AllocateNewConstantRegister, .func_end_AllocateNewConstantRegister-AllocateNewConstantRegister

	.global W65C02RegisterAllocatorInit
	.type W65C02RegisterAllocatorInit, @function

W65C02RegisterAllocatorInit:

	// *** Basic block 0

	.local InitializeZeroPageRegister
	.global BitSetInit
	.global VectorInit
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
	sd          a1, 0(s1)
	mv          s2, x0

	// *** Basic block 1

.W65C02RegisterAllocatorInit_label_42:
	addi        t0, s1, 8
	slli        t1, s2, 5
	add         a0, t0, t1
	mv          a2, x0
	mv          a1, s2
	call        InitializeZeroPageRegister

	// *** Basic block 2

.W65C02RegisterAllocatorInit_label_53:
	addi        s2, s2, 1
	li          s3, 4		// 0x4 ASCII \x4
	bge         s2, s3, .W65C02RegisterAllocatorInit_label_42

	// *** Basic block 3

.W65C02RegisterAllocatorInit_label_58:
	mv          s2, x0

	// *** Basic block 4

.W65C02RegisterAllocatorInit_label_62:
	addi        t0, s1, 136
	slli        t1, s2, 5
	add         a0, t0, t1
	li          s4, 1		// 0x1 ASCII \x1
	mv          a2, s4
	mv          a1, s2
	call        InitializeZeroPageRegister

	// *** Basic block 5

.W65C02RegisterAllocatorInit_label_73:
	addi        s2, s2, 1
	bge         s2, s3, .W65C02RegisterAllocatorInit_label_62

	// *** Basic block 6

.W65C02RegisterAllocatorInit_label_78:
	mv          s2, x0

	// *** Basic block 7

.W65C02RegisterAllocatorInit_label_82:
	addi        t0, s1, 264
	slli        t1, s2, 5
	add         a0, t0, t1
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, s2
	call        InitializeZeroPageRegister

	// *** Basic block 8

.W65C02RegisterAllocatorInit_label_93:
	addi        s2, s2, 1
	bge         s2, s3, .W65C02RegisterAllocatorInit_label_82

	// *** Basic block 9

.W65C02RegisterAllocatorInit_label_98:
	mv          s2, x0

	// *** Basic block 10

.W65C02RegisterAllocatorInit_label_102:
	addi        t0, s1, 392
	slli        t1, s2, 5
	add         a0, t0, t1
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	mv          a1, s2
	call        InitializeZeroPageRegister

	// *** Basic block 11

.W65C02RegisterAllocatorInit_label_113:
	addi        s2, s2, 1
	bge         s2, s3, .W65C02RegisterAllocatorInit_label_102

	// *** Basic block 12

.W65C02RegisterAllocatorInit_label_118:
	mv          s2, x0

	// *** Basic block 13

.W65C02RegisterAllocatorInit_label_122:
	addi        t0, s1, 520
	slli        t1, s2, 5
	add         a0, t0, t1
	mv          a2, s3
	mv          a1, s2
	call        InitializeZeroPageRegister

	// *** Basic block 14

.W65C02RegisterAllocatorInit_label_133:
	addi        s2, s2, 1
	bge         s2, s3, .W65C02RegisterAllocatorInit_label_122

	// *** Basic block 15

.W65C02RegisterAllocatorInit_label_138:
	mv          s2, x0

	// *** Basic block 16

.W65C02RegisterAllocatorInit_label_142:
	addi        t0, s1, 648
	slli        t1, s2, 5
	add         a0, t0, t1
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	mv          a1, s2
	call        InitializeZeroPageRegister

	// *** Basic block 17

.W65C02RegisterAllocatorInit_label_153:
	addi        s2, s2, 1
	bge         s2, s3, .W65C02RegisterAllocatorInit_label_142

	// *** Basic block 18

.W65C02RegisterAllocatorInit_label_158:
	addi        a0, s1, 776
	mv          a2, s4
	li          t0, 24		// 0x18 ASCII \x18
	mv          a1, t0
	call        InitializeZeroPageRegister

	// *** Basic block 19

	addi        a0, s1, 808
	mv          a2, s4
	li          t0, 26		// 0x1a ASCII \x1a
	mv          a1, t0
	call        InitializeZeroPageRegister

	// *** Basic block 20

	addi        a0, s1, 840
	mv          a2, s4
	li          t0, 28		// 0x1c ASCII \x1c
	mv          a1, t0
	call        InitializeZeroPageRegister

	// *** Basic block 21

	addi        a0, s1, 872
	call        BitSetInit

	// *** Basic block 22

	addi        a0, s1, 888
	call        BitSetInit

	// *** Basic block 23

	addi        a0, s1, 904
	call        BitSetInit

	// *** Basic block 24

	addi        a0, s1, 920
	call        BitSetInit

	// *** Basic block 25

	addi        a0, s1, 936
	call        BitSetInit

	// *** Basic block 26

	addi        a0, s1, 952
	call        BitSetInit

	// *** Basic block 27

	addi        a0, s1, 968
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorInit
.func_end_W65C02RegisterAllocatorInit:
	.size W65C02RegisterAllocatorInit, .func_end_W65C02RegisterAllocatorInit-W65C02RegisterAllocatorInit

	.global New6502RegisterAllocator
	.type New6502RegisterAllocator, @function

New6502RegisterAllocator:

	// *** Basic block 0

	.global malloc
	.global W65C02RegisterAllocatorInit
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
	li          a0, 992		// 0x3e0
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        W65C02RegisterAllocatorInit

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.New6502RegisterAllocator_label_21:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_New6502RegisterAllocator:
	.size New6502RegisterAllocator, .func_end_New6502RegisterAllocator-New6502RegisterAllocator

	.global W65C02RegisterAllocatorDestruct
	.type W65C02RegisterAllocatorDestruct, @function

W65C02RegisterAllocatorDestruct:

	// *** Basic block 0

	.global BitSetDestruct
	.global VectorDestructWithContents
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
	addi        a0, s1, 872
	call        BitSetDestruct

	// *** Basic block 1

	addi        a0, s1, 888
	call        BitSetDestruct

	// *** Basic block 2

	addi        a0, s1, 904
	call        BitSetDestruct

	// *** Basic block 3

	addi        a0, s1, 920
	call        BitSetDestruct

	// *** Basic block 4

	addi        a0, s1, 936
	call        BitSetDestruct

	// *** Basic block 5

	addi        a0, s1, 952
	call        BitSetDestruct

	// *** Basic block 6

	addi        a0, s1, 968
	mv          a1, x0
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorDestructWithContents
.func_end_W65C02RegisterAllocatorDestruct:
	.size W65C02RegisterAllocatorDestruct, .func_end_W65C02RegisterAllocatorDestruct-W65C02RegisterAllocatorDestruct

	.global W65C02RegisterAllocatorDelete
	.type W65C02RegisterAllocatorDelete, @function

W65C02RegisterAllocatorDelete:

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
	.global W65C02RegisterAllocatorDestruct
	.global free
	mv          s1, a0
	call        W65C02RegisterAllocatorDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_W65C02RegisterAllocatorDelete:
	.size W65C02RegisterAllocatorDelete, .func_end_W65C02RegisterAllocatorDelete-W65C02RegisterAllocatorDelete

	.local  FindFreeRegister
	.type FindFreeRegister, @function

FindFreeRegister:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	slli        t1, a1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 1

	j           .FindFreeRegister_label_32

	// *** Basic block 2

	j           .FindFreeRegister_label_38

	// *** Basic block 3

	j           .FindFreeRegister_label_42

	// *** Basic block 4

	j           .FindFreeRegister_label_46

	// *** Basic block 5

	j           .FindFreeRegister_label_50

	// *** Basic block 6

	j           .FindFreeRegister_label_54

	// *** Basic block 7

.FindFreeRegister_label_32:
	addi        t1, t0, 8
	li          t2, 4		// 0x4 ASCII \x4
	j           .FindFreeRegister_label_58

	// *** Basic block 8

.FindFreeRegister_label_38:
	addi        t1, t0, 136
	li          t2, 4		// 0x4 ASCII \x4
	j           .FindFreeRegister_label_58

	// *** Basic block 9

.FindFreeRegister_label_42:
	addi        t1, t0, 264
	li          t2, 4		// 0x4 ASCII \x4
	j           .FindFreeRegister_label_58

	// *** Basic block 10

.FindFreeRegister_label_46:
	addi        t1, t0, 392
	li          t2, 4		// 0x4 ASCII \x4
	j           .FindFreeRegister_label_58

	// *** Basic block 11

.FindFreeRegister_label_50:
	addi        t1, t0, 520
	li          t2, 4		// 0x4 ASCII \x4
	j           .FindFreeRegister_label_58

	// *** Basic block 12

.FindFreeRegister_label_54:
	addi        t1, t0, 648
	li          t2, 4		// 0x4 ASCII \x4
	j           .FindFreeRegister_label_58

	// *** Basic block 13

.FindFreeRegister_label_58:
	mv          t3, x0
	bge         x0, t2, .FindFreeRegister_label_87

	// *** Basic block 14

.FindFreeRegister_label_63:
	slli        t5, t3, 5
	add         t6, t1, t5
	lb          a1, 4(t6)
	not         t4, a1
	beqz        t4, .FindFreeRegister_label_75

	// *** Basic block 15

	ld          t6, 8(t6)
	sub         t6, t6, x0
	seqz        t4, t6

	// *** Basic block 16

.FindFreeRegister_label_75:
	beqz        t4, .FindFreeRegister_label_82

	// *** Basic block 17

	add         a0, t1, t5

	// *** Basic block 18

.FindFreeRegister_label_79:
	ret         

	// *** Basic block 19

.FindFreeRegister_label_82:

	// *** Basic block 20

.FindFreeRegister_label_83:
	addi        t3, t3, 1
	bge         t3, t2, .FindFreeRegister_label_63

	// *** Basic block 21

.FindFreeRegister_label_87:
	mv          a0, x0
	ret         
.func_end_FindFreeRegister:
	.size FindFreeRegister, .func_end_FindFreeRegister-FindFreeRegister

	.local  FreeRegister
	.type FreeRegister, @function

FreeRegister:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	lw          t1, 20(t0)
	beqz        t1, .FreeRegister_label_16

	// *** Basic block 1

.FreeRegister_label_13:
	ret         

	// *** Basic block 2

.FreeRegister_label_16:
	sd          x0, 8(t0)
	j           .FreeRegister_label_13
.func_end_FreeRegister:
	.size FreeRegister, .func_end_FreeRegister-FreeRegister

	.local  RegisterTypeFromTypeRecord
	.type RegisterTypeFromTypeRecord, @function

RegisterTypeFromTypeRecord:

	// *** Basic block 0

	.global TypeIsStructOrUnion
	.global TypeIsFloat
	.global TypeIsDouble
	.global TypeIsBool
	.global TypeIsChar
	.global TypeIsLong
	.global TypeIsLongLong
	.global TypeIsShort
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
	mv          t0, s1
	lw          s3, 16(t0)
	addi        t2, s3, -1
	seqz        t1, t2
	li          t2, 1		// 0x1 ASCII \x1
	beq         s3, t2, .RegisterTypeFromTypeRecord_label_36

	// *** Basic block 1

	addi        t0, s3, -2
	seqz        t1, t0

	// *** Basic block 2

.RegisterTypeFromTypeRecord_label_36:

	// *** Basic block 3

.RegisterTypeFromTypeRecord_label_38:
	mv          s2, t1
	bnez        t1, .RegisterTypeFromTypeRecord_label_47

	// *** Basic block 4

	j           .RegisterTypeFromTypeRecord_label_42

	// *** Basic block 5

.RegisterTypeFromTypeRecord_label_42:
	mv          a0, s1
	call        TypeIsStructOrUnion

	// *** Basic block 6

	mv          s2, a0

	// *** Basic block 7

.RegisterTypeFromTypeRecord_label_47:
	bnez        s2, .RegisterTypeFromTypeRecord_label_60

	// *** Basic block 8

	mv          s3, s1
	lw          t0, 16(s3)
	addi        t0, t0, -3
	seqz        s4, t0

	// *** Basic block 9

.RegisterTypeFromTypeRecord_label_57:
	mv          s2, s4
	j           .RegisterTypeFromTypeRecord_label_60

	// *** Basic block 10

.RegisterTypeFromTypeRecord_label_60:
	beqz        s2, .RegisterTypeFromTypeRecord_label_67

	// *** Basic block 11

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 12

.RegisterTypeFromTypeRecord_label_64:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 13

.RegisterTypeFromTypeRecord_label_67:
	mv          a0, s1
	call        TypeIsFloat

	// *** Basic block 14

	beqz        a0, .RegisterTypeFromTypeRecord_label_75

	// *** Basic block 15

	li          a0, 4		// 0x4 ASCII \x4
	j           .RegisterTypeFromTypeRecord_label_64

	// *** Basic block 16

.RegisterTypeFromTypeRecord_label_75:
	mv          a0, s1
	call        TypeIsDouble

	// *** Basic block 17

	beqz        a0, .RegisterTypeFromTypeRecord_label_83

	// *** Basic block 18

	li          a0, 5		// 0x5 ASCII \x5
	j           .RegisterTypeFromTypeRecord_label_64

	// *** Basic block 19

.RegisterTypeFromTypeRecord_label_83:
	mv          a0, s1
	call        TypeIsBool

	// *** Basic block 20

	beqz        a0, .RegisterTypeFromTypeRecord_label_91

	// *** Basic block 21

	mv          a0, x0
	j           .RegisterTypeFromTypeRecord_label_64

	// *** Basic block 22

.RegisterTypeFromTypeRecord_label_91:
	mv          a0, s1
	call        TypeIsChar

	// *** Basic block 23

	beqz        a0, .RegisterTypeFromTypeRecord_label_99

	// *** Basic block 24

	mv          a0, x0
	j           .RegisterTypeFromTypeRecord_label_64

	// *** Basic block 25

.RegisterTypeFromTypeRecord_label_99:
	mv          a0, s1
	call        TypeIsLong

	// *** Basic block 26

	mv          s4, a0
	bnez        a0, .RegisterTypeFromTypeRecord_label_110

	// *** Basic block 27

	mv          a0, s1
	call        TypeIsLongLong

	// *** Basic block 28

	mv          s4, a0

	// *** Basic block 29

.RegisterTypeFromTypeRecord_label_110:
	beqz        s4, .RegisterTypeFromTypeRecord_label_115

	// *** Basic block 30

	li          a0, 3		// 0x3 ASCII \x3
	j           .RegisterTypeFromTypeRecord_label_64

	// *** Basic block 31

.RegisterTypeFromTypeRecord_label_115:
	mv          a0, s1
	call        TypeIsShort

	// *** Basic block 32

	beqz        a0, .RegisterTypeFromTypeRecord_label_123

	// *** Basic block 33

	li          a0, 1		// 0x1 ASCII \x1
	j           .RegisterTypeFromTypeRecord_label_64

	// *** Basic block 34

.RegisterTypeFromTypeRecord_label_123:

	// *** Basic block 35

.RegisterTypeFromTypeRecord_label_124:

	// *** Basic block 36

.RegisterTypeFromTypeRecord_label_125:

	// *** Basic block 37

.RegisterTypeFromTypeRecord_label_126:

	// *** Basic block 38

.RegisterTypeFromTypeRecord_label_127:

	// *** Basic block 39

.RegisterTypeFromTypeRecord_label_128:

	// *** Basic block 40

.RegisterTypeFromTypeRecord_label_129:
	li          a0, 2		// 0x2 ASCII \x2
	j           .RegisterTypeFromTypeRecord_label_64
.func_end_RegisterTypeFromTypeRecord:
	.size RegisterTypeFromTypeRecord, .func_end_RegisterTypeFromTypeRecord-RegisterTypeFromTypeRecord

	.local  RegisterTypeFromInstruction
	.type RegisterTypeFromInstruction, @function

RegisterTypeFromInstruction:

	// *** Basic block 0

	.local RegisterTypeFromTypeRecord
	.global printf
	.global W65C02OpcodeName
	.global abort
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
	mv          s1, a0

	// *** Basic block 1

.RegisterTypeFromInstruction_label_20:
	lw          s2, 16(s1)
	li          t0, 2		// 0x2 ASCII \x2
	blt         s2, t0, .RegisterTypeFromInstruction_label_416

	// *** Basic block 2

	li          t0, 192		// 0xc0 ASCII \xc0
	blt         t0, s2, .RegisterTypeFromInstruction_label_416

	// *** Basic block 3

	addi        t0, s2, -2
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 4

	j           .RegisterTypeFromInstruction_label_398

	// *** Basic block 5

	j           .RegisterTypeFromInstruction_label_312

	// *** Basic block 6

	j           .RegisterTypeFromInstruction_label_348

	// *** Basic block 7

	j           .RegisterTypeFromInstruction_label_229

	// *** Basic block 8

	j           .RegisterTypeFromInstruction_label_290

	// *** Basic block 9

	j           .RegisterTypeFromInstruction_label_316

	// *** Basic block 10

	j           .RegisterTypeFromInstruction_label_352

	// *** Basic block 11

	j           .RegisterTypeFromInstruction_label_362

	// *** Basic block 12

	j           .RegisterTypeFromInstruction_label_380

	// *** Basic block 13

	j           .RegisterTypeFromInstruction_label_317

	// *** Basic block 14

	j           .RegisterTypeFromInstruction_label_363

	// *** Basic block 15

	j           .RegisterTypeFromInstruction_label_381

	// *** Basic block 16

	j           .RegisterTypeFromInstruction_label_318

	// *** Basic block 17

	j           .RegisterTypeFromInstruction_label_364

	// *** Basic block 18

	j           .RegisterTypeFromInstruction_label_382

	// *** Basic block 19

	j           .RegisterTypeFromInstruction_label_353

	// *** Basic block 20

	j           .RegisterTypeFromInstruction_label_339

	// *** Basic block 21

	j           .RegisterTypeFromInstruction_label_365

	// *** Basic block 22

	j           .RegisterTypeFromInstruction_label_383

	// *** Basic block 23

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 24

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 25

	j           .RegisterTypeFromInstruction_label_308

	// *** Basic block 26

	j           .RegisterTypeFromInstruction_label_309

	// *** Basic block 27

	j           .RegisterTypeFromInstruction_label_311

	// *** Basic block 28

	j           .RegisterTypeFromInstruction_label_356

	// *** Basic block 29

	j           .RegisterTypeFromInstruction_label_375

	// *** Basic block 30

	j           .RegisterTypeFromInstruction_label_391

	// *** Basic block 31

	j           .RegisterTypeFromInstruction_label_307

	// *** Basic block 32

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 33

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 34

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 35

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 36

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 37

	j           .RegisterTypeFromInstruction_label_294

	// *** Basic block 38

	j           .RegisterTypeFromInstruction_label_293

	// *** Basic block 39

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 40

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 41

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 42

	j           .RegisterTypeFromInstruction_label_306

	// *** Basic block 43

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 44

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 45

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 46

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 47

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 48

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 49

	j           .RegisterTypeFromInstruction_label_399

	// *** Basic block 50

	j           .RegisterTypeFromInstruction_label_400

	// *** Basic block 51

	j           .RegisterTypeFromInstruction_label_310

	// *** Basic block 52

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 53

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 54

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 55

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 56

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 57

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 58

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 59

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 60

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 61

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 62

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 63

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 64

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 65

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 66

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 67

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 68

	j           .RegisterTypeFromInstruction_label_291

	// *** Basic block 69

	j           .RegisterTypeFromInstruction_label_319

	// *** Basic block 70

	j           .RegisterTypeFromInstruction_label_300

	// *** Basic block 71

	j           .RegisterTypeFromInstruction_label_230

	// *** Basic block 72

	j           .RegisterTypeFromInstruction_label_320

	// *** Basic block 73

	j           .RegisterTypeFromInstruction_label_231

	// *** Basic block 74

	j           .RegisterTypeFromInstruction_label_302

	// *** Basic block 75

	j           .RegisterTypeFromInstruction_label_354

	// *** Basic block 76

	j           .RegisterTypeFromInstruction_label_366

	// *** Basic block 77

	j           .RegisterTypeFromInstruction_label_384

	// *** Basic block 78

	j           .RegisterTypeFromInstruction_label_292

	// *** Basic block 79

	j           .RegisterTypeFromInstruction_label_321

	// *** Basic block 80

	j           .RegisterTypeFromInstruction_label_301

	// *** Basic block 81

	j           .RegisterTypeFromInstruction_label_355

	// *** Basic block 82

	j           .RegisterTypeFromInstruction_label_367

	// *** Basic block 83

	j           .RegisterTypeFromInstruction_label_385

	// *** Basic block 84

	j           .RegisterTypeFromInstruction_label_232

	// *** Basic block 85

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 86

	j           .RegisterTypeFromInstruction_label_303

	// *** Basic block 87

	j           .RegisterTypeFromInstruction_label_368

	// *** Basic block 88

	j           .RegisterTypeFromInstruction_label_386

	// *** Basic block 89

	j           .RegisterTypeFromInstruction_label_325

	// *** Basic block 90

	j           .RegisterTypeFromInstruction_label_304

	// *** Basic block 91

	j           .RegisterTypeFromInstruction_label_326

	// *** Basic block 92

	j           .RegisterTypeFromInstruction_label_369

	// *** Basic block 93

	j           .RegisterTypeFromInstruction_label_387

	// *** Basic block 94

	j           .RegisterTypeFromInstruction_label_305

	// *** Basic block 95

	j           .RegisterTypeFromInstruction_label_327

	// *** Basic block 96

	j           .RegisterTypeFromInstruction_label_370

	// *** Basic block 97

	j           .RegisterTypeFromInstruction_label_388

	// *** Basic block 98

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 99

	j           .RegisterTypeFromInstruction_label_328

	// *** Basic block 100

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 101

	j           .RegisterTypeFromInstruction_label_371

	// *** Basic block 102

	j           .RegisterTypeFromInstruction_label_389

	// *** Basic block 103

	j           .RegisterTypeFromInstruction_label_329

	// *** Basic block 104

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 105

	j           .RegisterTypeFromInstruction_label_330

	// *** Basic block 106

	j           .RegisterTypeFromInstruction_label_331

	// *** Basic block 107

	j           .RegisterTypeFromInstruction_label_332

	// *** Basic block 108

	j           .RegisterTypeFromInstruction_label_333

	// *** Basic block 109

	j           .RegisterTypeFromInstruction_label_334

	// *** Basic block 110

	j           .RegisterTypeFromInstruction_label_335

	// *** Basic block 111

	j           .RegisterTypeFromInstruction_label_336

	// *** Basic block 112

	j           .RegisterTypeFromInstruction_label_337

	// *** Basic block 113

	j           .RegisterTypeFromInstruction_label_338

	// *** Basic block 114

	j           .RegisterTypeFromInstruction_label_372

	// *** Basic block 115

	j           .RegisterTypeFromInstruction_label_390

	// *** Basic block 116

	j           .RegisterTypeFromInstruction_label_233

	// *** Basic block 117

	j           .RegisterTypeFromInstruction_label_234

	// *** Basic block 118

	j           .RegisterTypeFromInstruction_label_235

	// *** Basic block 119

	j           .RegisterTypeFromInstruction_label_236

	// *** Basic block 120

	j           .RegisterTypeFromInstruction_label_237

	// *** Basic block 121

	j           .RegisterTypeFromInstruction_label_238

	// *** Basic block 122

	j           .RegisterTypeFromInstruction_label_239

	// *** Basic block 123

	j           .RegisterTypeFromInstruction_label_240

	// *** Basic block 124

	j           .RegisterTypeFromInstruction_label_241

	// *** Basic block 125

	j           .RegisterTypeFromInstruction_label_242

	// *** Basic block 126

	j           .RegisterTypeFromInstruction_label_243

	// *** Basic block 127

	j           .RegisterTypeFromInstruction_label_244

	// *** Basic block 128

	j           .RegisterTypeFromInstruction_label_245

	// *** Basic block 129

	j           .RegisterTypeFromInstruction_label_246

	// *** Basic block 130

	j           .RegisterTypeFromInstruction_label_247

	// *** Basic block 131

	j           .RegisterTypeFromInstruction_label_248

	// *** Basic block 132

	j           .RegisterTypeFromInstruction_label_249

	// *** Basic block 133

	j           .RegisterTypeFromInstruction_label_250

	// *** Basic block 134

	j           .RegisterTypeFromInstruction_label_251

	// *** Basic block 135

	j           .RegisterTypeFromInstruction_label_252

	// *** Basic block 136

	j           .RegisterTypeFromInstruction_label_253

	// *** Basic block 137

	j           .RegisterTypeFromInstruction_label_254

	// *** Basic block 138

	j           .RegisterTypeFromInstruction_label_255

	// *** Basic block 139

	j           .RegisterTypeFromInstruction_label_256

	// *** Basic block 140

	j           .RegisterTypeFromInstruction_label_257

	// *** Basic block 141

	j           .RegisterTypeFromInstruction_label_258

	// *** Basic block 142

	j           .RegisterTypeFromInstruction_label_259

	// *** Basic block 143

	j           .RegisterTypeFromInstruction_label_260

	// *** Basic block 144

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 145

	j           .RegisterTypeFromInstruction_label_262

	// *** Basic block 146

	j           .RegisterTypeFromInstruction_label_263

	// *** Basic block 147

	j           .RegisterTypeFromInstruction_label_264

	// *** Basic block 148

	j           .RegisterTypeFromInstruction_label_265

	// *** Basic block 149

	j           .RegisterTypeFromInstruction_label_266

	// *** Basic block 150

	j           .RegisterTypeFromInstruction_label_267

	// *** Basic block 151

	j           .RegisterTypeFromInstruction_label_268

	// *** Basic block 152

	j           .RegisterTypeFromInstruction_label_269

	// *** Basic block 153

	j           .RegisterTypeFromInstruction_label_270

	// *** Basic block 154

	j           .RegisterTypeFromInstruction_label_271

	// *** Basic block 155

	j           .RegisterTypeFromInstruction_label_272

	// *** Basic block 156

	j           .RegisterTypeFromInstruction_label_273

	// *** Basic block 157

	j           .RegisterTypeFromInstruction_label_274

	// *** Basic block 158

	j           .RegisterTypeFromInstruction_label_275

	// *** Basic block 159

	j           .RegisterTypeFromInstruction_label_276

	// *** Basic block 160

	j           .RegisterTypeFromInstruction_label_277

	// *** Basic block 161

	j           .RegisterTypeFromInstruction_label_278

	// *** Basic block 162

	j           .RegisterTypeFromInstruction_label_279

	// *** Basic block 163

	j           .RegisterTypeFromInstruction_label_280

	// *** Basic block 164

	j           .RegisterTypeFromInstruction_label_281

	// *** Basic block 165

	j           .RegisterTypeFromInstruction_label_282

	// *** Basic block 166

	j           .RegisterTypeFromInstruction_label_283

	// *** Basic block 167

	j           .RegisterTypeFromInstruction_label_284

	// *** Basic block 168

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 169

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 170

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 171

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 172

	j           .RegisterTypeFromInstruction_label_373

	// *** Basic block 173

	j           .RegisterTypeFromInstruction_label_393

	// *** Basic block 174

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 175

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 176

	j           .RegisterTypeFromInstruction_label_394

	// *** Basic block 177

	j           .RegisterTypeFromInstruction_label_374

	// *** Basic block 178

	j           .RegisterTypeFromInstruction_label_345

	// *** Basic block 179

	j           .RegisterTypeFromInstruction_label_346

	// *** Basic block 180

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 181

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 182

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 183

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 184

	j           .RegisterTypeFromInstruction_label_297

	// *** Basic block 185

	j           .RegisterTypeFromInstruction_label_298

	// *** Basic block 186

	j           .RegisterTypeFromInstruction_label_299

	// *** Basic block 187

	j           .RegisterTypeFromInstruction_label_295

	// *** Basic block 188

	j           .RegisterTypeFromInstruction_label_347

	// *** Basic block 189

	j           .RegisterTypeFromInstruction_label_357

	// *** Basic block 190

	j           .RegisterTypeFromInstruction_label_376

	// *** Basic block 191

	j           .RegisterTypeFromInstruction_label_392

	// *** Basic block 192

	j           .RegisterTypeFromInstruction_label_296

	// *** Basic block 193

	j           .RegisterTypeFromInstruction_label_416

	// *** Basic block 194

	j           .RegisterTypeFromInstruction_label_358

	// *** Basic block 195

.RegisterTypeFromInstruction_label_229:

	// *** Basic block 196

.RegisterTypeFromInstruction_label_230:

	// *** Basic block 197

.RegisterTypeFromInstruction_label_231:

	// *** Basic block 198

.RegisterTypeFromInstruction_label_232:

	// *** Basic block 199

.RegisterTypeFromInstruction_label_233:

	// *** Basic block 200

.RegisterTypeFromInstruction_label_234:

	// *** Basic block 201

.RegisterTypeFromInstruction_label_235:

	// *** Basic block 202

.RegisterTypeFromInstruction_label_236:

	// *** Basic block 203

.RegisterTypeFromInstruction_label_237:

	// *** Basic block 204

.RegisterTypeFromInstruction_label_238:

	// *** Basic block 205

.RegisterTypeFromInstruction_label_239:

	// *** Basic block 206

.RegisterTypeFromInstruction_label_240:

	// *** Basic block 207

.RegisterTypeFromInstruction_label_241:

	// *** Basic block 208

.RegisterTypeFromInstruction_label_242:

	// *** Basic block 209

.RegisterTypeFromInstruction_label_243:

	// *** Basic block 210

.RegisterTypeFromInstruction_label_244:

	// *** Basic block 211

.RegisterTypeFromInstruction_label_245:

	// *** Basic block 212

.RegisterTypeFromInstruction_label_246:

	// *** Basic block 213

.RegisterTypeFromInstruction_label_247:

	// *** Basic block 214

.RegisterTypeFromInstruction_label_248:

	// *** Basic block 215

.RegisterTypeFromInstruction_label_249:

	// *** Basic block 216

.RegisterTypeFromInstruction_label_250:

	// *** Basic block 217

.RegisterTypeFromInstruction_label_251:

	// *** Basic block 218

.RegisterTypeFromInstruction_label_252:

	// *** Basic block 219

.RegisterTypeFromInstruction_label_253:

	// *** Basic block 220

.RegisterTypeFromInstruction_label_254:

	// *** Basic block 221

.RegisterTypeFromInstruction_label_255:

	// *** Basic block 222

.RegisterTypeFromInstruction_label_256:

	// *** Basic block 223

.RegisterTypeFromInstruction_label_257:

	// *** Basic block 224

.RegisterTypeFromInstruction_label_258:

	// *** Basic block 225

.RegisterTypeFromInstruction_label_259:

	// *** Basic block 226

.RegisterTypeFromInstruction_label_260:

	// *** Basic block 227

.RegisterTypeFromInstruction_label_261:

	// *** Basic block 228

.RegisterTypeFromInstruction_label_262:

	// *** Basic block 229

.RegisterTypeFromInstruction_label_263:

	// *** Basic block 230

.RegisterTypeFromInstruction_label_264:

	// *** Basic block 231

.RegisterTypeFromInstruction_label_265:

	// *** Basic block 232

.RegisterTypeFromInstruction_label_266:

	// *** Basic block 233

.RegisterTypeFromInstruction_label_267:

	// *** Basic block 234

.RegisterTypeFromInstruction_label_268:

	// *** Basic block 235

.RegisterTypeFromInstruction_label_269:

	// *** Basic block 236

.RegisterTypeFromInstruction_label_270:

	// *** Basic block 237

.RegisterTypeFromInstruction_label_271:

	// *** Basic block 238

.RegisterTypeFromInstruction_label_272:

	// *** Basic block 239

.RegisterTypeFromInstruction_label_273:

	// *** Basic block 240

.RegisterTypeFromInstruction_label_274:

	// *** Basic block 241

.RegisterTypeFromInstruction_label_275:

	// *** Basic block 242

.RegisterTypeFromInstruction_label_276:

	// *** Basic block 243

.RegisterTypeFromInstruction_label_277:

	// *** Basic block 244

.RegisterTypeFromInstruction_label_278:

	// *** Basic block 245

.RegisterTypeFromInstruction_label_279:

	// *** Basic block 246

.RegisterTypeFromInstruction_label_280:

	// *** Basic block 247

.RegisterTypeFromInstruction_label_281:

	// *** Basic block 248

.RegisterTypeFromInstruction_label_282:

	// *** Basic block 249

.RegisterTypeFromInstruction_label_283:

	// *** Basic block 250

.RegisterTypeFromInstruction_label_284:
	mv          a0, x0

	// *** Basic block 251

.RegisterTypeFromInstruction_label_287:
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

	// *** Basic block 252

.RegisterTypeFromInstruction_label_290:

	// *** Basic block 253

.RegisterTypeFromInstruction_label_291:

	// *** Basic block 254

.RegisterTypeFromInstruction_label_292:

	// *** Basic block 255

.RegisterTypeFromInstruction_label_293:

	// *** Basic block 256

.RegisterTypeFromInstruction_label_294:

	// *** Basic block 257

.RegisterTypeFromInstruction_label_295:

	// *** Basic block 258

.RegisterTypeFromInstruction_label_296:

	// *** Basic block 259

.RegisterTypeFromInstruction_label_297:

	// *** Basic block 260

.RegisterTypeFromInstruction_label_298:

	// *** Basic block 261

.RegisterTypeFromInstruction_label_299:

	// *** Basic block 262

.RegisterTypeFromInstruction_label_300:

	// *** Basic block 263

.RegisterTypeFromInstruction_label_301:

	// *** Basic block 264

.RegisterTypeFromInstruction_label_302:

	// *** Basic block 265

.RegisterTypeFromInstruction_label_303:

	// *** Basic block 266

.RegisterTypeFromInstruction_label_304:

	// *** Basic block 267

.RegisterTypeFromInstruction_label_305:

	// *** Basic block 268

.RegisterTypeFromInstruction_label_306:

	// *** Basic block 269

.RegisterTypeFromInstruction_label_307:

	// *** Basic block 270

.RegisterTypeFromInstruction_label_308:

	// *** Basic block 271

.RegisterTypeFromInstruction_label_309:

	// *** Basic block 272

.RegisterTypeFromInstruction_label_310:

	// *** Basic block 273

.RegisterTypeFromInstruction_label_311:

	// *** Basic block 274

.RegisterTypeFromInstruction_label_312:
	li          a0, 1		// 0x1 ASCII \x1
	j           .RegisterTypeFromInstruction_label_287

	// *** Basic block 275

.RegisterTypeFromInstruction_label_316:

	// *** Basic block 276

.RegisterTypeFromInstruction_label_317:

	// *** Basic block 277

.RegisterTypeFromInstruction_label_318:

	// *** Basic block 278

.RegisterTypeFromInstruction_label_319:

	// *** Basic block 279

.RegisterTypeFromInstruction_label_320:

	// *** Basic block 280

.RegisterTypeFromInstruction_label_321:
	li          a0, 2		// 0x2 ASCII \x2
	j           .RegisterTypeFromInstruction_label_287

	// *** Basic block 281

.RegisterTypeFromInstruction_label_325:

	// *** Basic block 282

.RegisterTypeFromInstruction_label_326:

	// *** Basic block 283

.RegisterTypeFromInstruction_label_327:

	// *** Basic block 284

.RegisterTypeFromInstruction_label_328:

	// *** Basic block 285

.RegisterTypeFromInstruction_label_329:

	// *** Basic block 286

.RegisterTypeFromInstruction_label_330:

	// *** Basic block 287

.RegisterTypeFromInstruction_label_331:

	// *** Basic block 288

.RegisterTypeFromInstruction_label_332:

	// *** Basic block 289

.RegisterTypeFromInstruction_label_333:

	// *** Basic block 290

.RegisterTypeFromInstruction_label_334:

	// *** Basic block 291

.RegisterTypeFromInstruction_label_335:

	// *** Basic block 292

.RegisterTypeFromInstruction_label_336:

	// *** Basic block 293

.RegisterTypeFromInstruction_label_337:

	// *** Basic block 294

.RegisterTypeFromInstruction_label_338:

	// *** Basic block 295

.RegisterTypeFromInstruction_label_339:
	ld          s3, 40(s1)
	mv          s1, s3
	j           .RegisterTypeFromInstruction_label_20

	// *** Basic block 296

.RegisterTypeFromInstruction_label_345:

	// *** Basic block 297

.RegisterTypeFromInstruction_label_346:

	// *** Basic block 298

.RegisterTypeFromInstruction_label_347:

	// *** Basic block 299

.RegisterTypeFromInstruction_label_348:
	li          a0, 2		// 0x2 ASCII \x2
	j           .RegisterTypeFromInstruction_label_287

	// *** Basic block 300

.RegisterTypeFromInstruction_label_352:

	// *** Basic block 301

.RegisterTypeFromInstruction_label_353:

	// *** Basic block 302

.RegisterTypeFromInstruction_label_354:

	// *** Basic block 303

.RegisterTypeFromInstruction_label_355:

	// *** Basic block 304

.RegisterTypeFromInstruction_label_356:

	// *** Basic block 305

.RegisterTypeFromInstruction_label_357:

	// *** Basic block 306

.RegisterTypeFromInstruction_label_358:
	li          a0, 3		// 0x3 ASCII \x3
	j           .RegisterTypeFromInstruction_label_287

	// *** Basic block 307

.RegisterTypeFromInstruction_label_362:

	// *** Basic block 308

.RegisterTypeFromInstruction_label_363:

	// *** Basic block 309

.RegisterTypeFromInstruction_label_364:

	// *** Basic block 310

.RegisterTypeFromInstruction_label_365:

	// *** Basic block 311

.RegisterTypeFromInstruction_label_366:

	// *** Basic block 312

.RegisterTypeFromInstruction_label_367:

	// *** Basic block 313

.RegisterTypeFromInstruction_label_368:

	// *** Basic block 314

.RegisterTypeFromInstruction_label_369:

	// *** Basic block 315

.RegisterTypeFromInstruction_label_370:

	// *** Basic block 316

.RegisterTypeFromInstruction_label_371:

	// *** Basic block 317

.RegisterTypeFromInstruction_label_372:

	// *** Basic block 318

.RegisterTypeFromInstruction_label_373:

	// *** Basic block 319

.RegisterTypeFromInstruction_label_374:

	// *** Basic block 320

.RegisterTypeFromInstruction_label_375:

	// *** Basic block 321

.RegisterTypeFromInstruction_label_376:
	li          a0, 4		// 0x4 ASCII \x4
	j           .RegisterTypeFromInstruction_label_287

	// *** Basic block 322

.RegisterTypeFromInstruction_label_380:

	// *** Basic block 323

.RegisterTypeFromInstruction_label_381:

	// *** Basic block 324

.RegisterTypeFromInstruction_label_382:

	// *** Basic block 325

.RegisterTypeFromInstruction_label_383:

	// *** Basic block 326

.RegisterTypeFromInstruction_label_384:

	// *** Basic block 327

.RegisterTypeFromInstruction_label_385:

	// *** Basic block 328

.RegisterTypeFromInstruction_label_386:

	// *** Basic block 329

.RegisterTypeFromInstruction_label_387:

	// *** Basic block 330

.RegisterTypeFromInstruction_label_388:

	// *** Basic block 331

.RegisterTypeFromInstruction_label_389:

	// *** Basic block 332

.RegisterTypeFromInstruction_label_390:

	// *** Basic block 333

.RegisterTypeFromInstruction_label_391:

	// *** Basic block 334

.RegisterTypeFromInstruction_label_392:

	// *** Basic block 335

.RegisterTypeFromInstruction_label_393:

	// *** Basic block 336

.RegisterTypeFromInstruction_label_394:
	li          a0, 5		// 0x5 ASCII \x5
	j           .RegisterTypeFromInstruction_label_287

	// *** Basic block 337

.RegisterTypeFromInstruction_label_398:

	// *** Basic block 338

.RegisterTypeFromInstruction_label_399:

	// *** Basic block 339

.RegisterTypeFromInstruction_label_400:
	mv          s3, s1
	ld          s4, 112(s3)
	ld          s5, 40(s4)
	mv          a0, s5
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           RegisterTypeFromTypeRecord

	// *** Basic block 341

.RegisterTypeFromInstruction_label_416:
	lla         s4, .str.1
	mv          a0, s2
	call        W65C02OpcodeName

	// *** Basic block 342

	mv          a1, a0
	mv          a0, s4
	call        printf

	// *** Basic block 343

	lla         a0, .str.2
	lla         a1, .str.3
	lla         a3, .str.4
	li          t0, 375		// 0x177
	mv          a2, t0
	call        printf

	// *** Basic block 344

	call        abort

	// *** Basic block 345

	mv          a0, x0
	j           .RegisterTypeFromInstruction_label_287
.func_end_RegisterTypeFromInstruction:
	.size RegisterTypeFromInstruction, .func_end_RegisterTypeFromInstruction-RegisterTypeFromInstruction

	.local  FreeRegisters
	.type FreeRegisters, @function

FreeRegisters:

	// *** Basic block 0

	.global printf
	.global abort
	.local FreeRegister
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
	mv          s2, x0
	addi        t0, a1, 40

	// *** Basic block 1

.FreeRegisters_label_30:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          t1, 0(t0)
	beq         t1, x0, .FreeRegisters_label_87

	// *** Basic block 2

	ld          s3, 32(t1)
	sub         t2, s3, x0
	snez        t0, t2
	beq         s3, x0, .FreeRegisters_label_49

	// *** Basic block 3

	lb          t2, 4(s3)
	not         t0, t2

	// *** Basic block 4

.FreeRegisters_label_49:
	beqz        t0, .FreeRegisters_label_86

	// *** Basic block 5

	lw          s4, 88(t1)
	addi        t0, s4, -1
	sw          t0, 88(t1)
	blt         s4, x0, .FreeRegisters_label_60

	// *** Basic block 6

	j           .FreeRegisters_label_77

	// *** Basic block 7

.FreeRegisters_label_60:
	lla         a0, .str.5
	lla         a1, .str.6
	lla         a3, .str.7
	li          t0, 388		// 0x184
	mv          a2, t0
	call        printf

	// *** Basic block 8

	call        abort

	// *** Basic block 9

.FreeRegisters_label_77:
	bnez        s4, .FreeRegisters_label_85

	// *** Basic block 10

	mv          a1, s3
	mv          a0, s1
	call        FreeRegister

	// *** Basic block 11

.FreeRegisters_label_85:

	// *** Basic block 12

.FreeRegisters_label_86:

	// *** Basic block 13

.FreeRegisters_label_87:

	// *** Basic block 14

.FreeRegisters_label_88:
	addi        s2, s2, 1
	li          t0, 3		// 0x3 ASCII \x3
	bge         s2, t0, .FreeRegisters_label_30

	// *** Basic block 15

.FreeRegisters_label_93:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_FreeRegisters:
	.size FreeRegisters, .func_end_FreeRegisters-FreeRegisters

	.local  AllocateRegisterWithType
	.type AllocateRegisterWithType, @function

AllocateRegisterWithType:

	// *** Basic block 0

	.local FindFreeRegister
	.global printf
	.global abort
	.global BitSetInsert
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
	mv          s2, a1
	call        FindFreeRegister

	// *** Basic block 1

	mv          s3, a0
	beq         s3, x0, .AllocateRegisterWithType_label_35

	// *** Basic block 2

	j           .AllocateRegisterWithType_label_52

	// *** Basic block 3

.AllocateRegisterWithType_label_35:
	lla         a0, .str.8
	lla         a1, .str.9
	lla         a3, .str.10
	li          t0, 402		// 0x192
	mv          a2, t0
	call        printf

	// *** Basic block 4

	call        abort

	// *** Basic block 5

.AllocateRegisterWithType_label_52:
	slli        t0, s2, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 6

	j           .AllocateRegisterWithType_label_65

	// *** Basic block 7

	j           .AllocateRegisterWithType_label_72

	// *** Basic block 8

	j           .AllocateRegisterWithType_label_79

	// *** Basic block 9

	j           .AllocateRegisterWithType_label_86

	// *** Basic block 10

	j           .AllocateRegisterWithType_label_93

	// *** Basic block 11

	j           .AllocateRegisterWithType_label_100

	// *** Basic block 12

.AllocateRegisterWithType_label_65:
	addi        a0, s1, 872
	lw          a1, 0(s3)
	call        BitSetInsert

	// *** Basic block 13

	j           .AllocateRegisterWithType_label_107

	// *** Basic block 14

.AllocateRegisterWithType_label_72:
	addi        a0, s1, 888
	lw          a1, 0(s3)
	call        BitSetInsert

	// *** Basic block 15

	j           .AllocateRegisterWithType_label_107

	// *** Basic block 16

.AllocateRegisterWithType_label_79:
	addi        a0, s1, 904
	lw          a1, 0(s3)
	call        BitSetInsert

	// *** Basic block 17

	j           .AllocateRegisterWithType_label_107

	// *** Basic block 18

.AllocateRegisterWithType_label_86:
	addi        a0, s1, 888
	lw          a1, 0(s3)
	call        BitSetInsert

	// *** Basic block 19

	j           .AllocateRegisterWithType_label_107

	// *** Basic block 20

.AllocateRegisterWithType_label_93:
	addi        a0, s1, 936
	lw          a1, 0(s3)
	call        BitSetInsert

	// *** Basic block 21

	j           .AllocateRegisterWithType_label_107

	// *** Basic block 22

.AllocateRegisterWithType_label_100:
	addi        a0, s1, 952
	lw          a1, 0(s3)
	call        BitSetInsert

	// *** Basic block 23

	j           .AllocateRegisterWithType_label_107

	// *** Basic block 24

.AllocateRegisterWithType_label_107:
	mv          a0, s3

	// *** Basic block 25

.AllocateRegisterWithType_label_110:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AllocateRegisterWithType:
	.size AllocateRegisterWithType, .func_end_AllocateRegisterWithType-AllocateRegisterWithType

	.local  UsesFixedRegister
	.type UsesFixedRegister, @function

UsesFixedRegister:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 16(a0)
	li          t1, 23		// 0x17 ASCII \x17
	beq         t0, t1, .UsesFixedRegister_label_76

	// *** Basic block 1

	li          t1, 24		// 0x18 ASCII \x18
	beq         t0, t1, .UsesFixedRegister_label_75

	// *** Basic block 2

	li          t1, 25		// 0x19 ASCII \x19
	beq         t0, t1, .UsesFixedRegister_label_77

	// *** Basic block 3

	li          t1, 49		// 0x31 ASCII '1'
	beq         t0, t1, .UsesFixedRegister_label_74

	// *** Basic block 4

	li          t1, 185		// 0xb9 ASCII \xb9
	beq         t0, t1, .UsesFixedRegister_label_69

	// *** Basic block 5

	li          t1, 186		// 0xba ASCII \xba
	beq         t0, t1, .UsesFixedRegister_label_70

	// *** Basic block 6

	li          t1, 187		// 0xbb ASCII \xbb
	beq         t0, t1, .UsesFixedRegister_label_71

	// *** Basic block 7

	li          t1, 188		// 0xbc ASCII \xbc
	beq         t0, t1, .UsesFixedRegister_label_72

	// *** Basic block 8

	li          t1, 189		// 0xbd ASCII \xbd
	beq         t0, t1, .UsesFixedRegister_label_73

	// *** Basic block 9

.UsesFixedRegister_label_65:
	mv          a0, x0
	ret         

	// *** Basic block 10

.UsesFixedRegister_label_69:

	// *** Basic block 11

.UsesFixedRegister_label_70:

	// *** Basic block 12

.UsesFixedRegister_label_71:

	// *** Basic block 13

.UsesFixedRegister_label_72:

	// *** Basic block 14

.UsesFixedRegister_label_73:

	// *** Basic block 15

.UsesFixedRegister_label_74:

	// *** Basic block 16

.UsesFixedRegister_label_75:

	// *** Basic block 17

.UsesFixedRegister_label_76:

	// *** Basic block 18

.UsesFixedRegister_label_77:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 19

.UsesFixedRegister_label_80:
	ret         
.func_end_UsesFixedRegister:
	.size UsesFixedRegister, .func_end_UsesFixedRegister-UsesFixedRegister

	.local  AllocateForRmov
	.type AllocateForRmov, @function

AllocateForRmov:

	// *** Basic block 0

	.local UsesFixedRegister
	.local FreeRegisters
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
	mv          s2, a0
	addi        t0, s1, 40
	ld          t1, 40(s1)
	ld          s3, 32(t1)
	ld          s4, 8(t0)
	addi        t0, s4, 64
	ld          t0, 8(t0)
	addi        t1, t0, -1
	seqz        s5, t1
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .AllocateForRmov_label_40

	// *** Basic block 1

	mv          a0, s4
	call        UsesFixedRegister

	// *** Basic block 2

	not         s5, a0

	// *** Basic block 3

.AllocateForRmov_label_40:
	beqz        s5, .AllocateForRmov_label_62

	// *** Basic block 4

	mv          a1, s1
	mv          a0, s2
	call        FreeRegisters

	// *** Basic block 5

	sd          s3, 32(s4)
	lw          t0, 88(s4)
	addi        t0, t0, 1
	sw          t0, 88(s4)
	sd          s4, 8(s3)
	ld          t0, 32(s4)
	sd          t0, 32(s1)

	// *** Basic block 6

.AllocateForRmov_label_59:
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

	// *** Basic block 7

.AllocateForRmov_label_62:
	ld          t0, 40(s1)
	lw          t1, 88(t0)
	addi        t1, t1, 1
	sw          t1, 88(t0)
	mv          a1, s1
	mv          a0, s2
	call        FreeRegisters

	// *** Basic block 8

	addi        t0, s1, 64
	ld          t0, 8(t0)
	sw          t0, 88(s1)
	sd          s3, 32(s1)
	sd          s1, 8(s3)
	j           .AllocateForRmov_label_59
.func_end_AllocateForRmov:
	.size AllocateForRmov, .func_end_AllocateForRmov-AllocateForRmov

	.local  NeedsRegister
	.type NeedsRegister, @function

NeedsRegister:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 16(a0)
	li          t1, 53		// 0x35 ASCII '5'
	blt         t0, t1, .NeedsRegister_label_110

	// *** Basic block 1

	beq         t0, t1, .NeedsRegister_label_183

	// *** Basic block 2

	li          t1, 56		// 0x38 ASCII '8'
	beq         t0, t1, .NeedsRegister_label_185

	// *** Basic block 3

	li          t1, 57		// 0x39 ASCII '9'
	beq         t0, t1, .NeedsRegister_label_186

	// *** Basic block 4

	li          t1, 58		// 0x3a ASCII ':'
	beq         t0, t1, .NeedsRegister_label_184

	// *** Basic block 5

	li          t1, 59		// 0x3b ASCII ';'
	beq         t0, t1, .NeedsRegister_label_187

	// *** Basic block 6

	li          t1, 60		// 0x3c ASCII '<'
	beq         t0, t1, .NeedsRegister_label_188

	// *** Basic block 7

	li          t1, 63		// 0x3f ASCII '?'
	beq         t0, t1, .NeedsRegister_label_189

	// *** Basic block 8

	li          t1, 64		// 0x40 ASCII '@'
	beq         t0, t1, .NeedsRegister_label_190

	// *** Basic block 9

	li          t1, 65		// 0x41 ASCII 'A'
	beq         t0, t1, .NeedsRegister_label_191

	// *** Basic block 10

	li          t1, 166		// 0xa6 ASCII \xa6
	beq         t0, t1, .NeedsRegister_label_174

	// *** Basic block 11

	li          t1, 167		// 0xa7 ASCII \xa7
	beq         t0, t1, .NeedsRegister_label_175

	// *** Basic block 12

	li          t1, 168		// 0xa8 ASCII \xa8
	beq         t0, t1, .NeedsRegister_label_176

	// *** Basic block 13

	li          t1, 169		// 0xa9 ASCII \xa9
	beq         t0, t1, .NeedsRegister_label_178

	// *** Basic block 14

	li          t1, 180		// 0xb4 ASCII \xb4
	beq         t0, t1, .NeedsRegister_label_177

	// *** Basic block 15

	j           .NeedsRegister_label_206

	// *** Basic block 16

.NeedsRegister_label_110:
	beqz        t0, .NeedsRegister_label_193

	// *** Basic block 17

	li          t1, 1		// 0x1 ASCII \x1
	beq         t0, t1, .NeedsRegister_label_194

	// *** Basic block 18

	li          t1, 3		// 0x3 ASCII \x3
	beq         t0, t1, .NeedsRegister_label_197

	// *** Basic block 19

	li          t1, 21		// 0x15 ASCII \x15
	beq         t0, t1, .NeedsRegister_label_192

	// *** Basic block 20

	li          t1, 22		// 0x16 ASCII \x16
	beq         t0, t1, .NeedsRegister_label_179

	// *** Basic block 21

	li          t1, 30		// 0x1e ASCII \x1e
	beq         t0, t1, .NeedsRegister_label_198

	// *** Basic block 22

	li          t1, 31		// 0x1f ASCII \x1f
	beq         t0, t1, .NeedsRegister_label_199

	// *** Basic block 23

	li          t1, 32		// 0x20 ASCII ' '
	beq         t0, t1, .NeedsRegister_label_200

	// *** Basic block 24

	li          t1, 45		// 0x2d ASCII '-'
	beq         t0, t1, .NeedsRegister_label_195

	// *** Basic block 25

	li          t1, 46		// 0x2e ASCII '.'
	beq         t0, t1, .NeedsRegister_label_196

	// *** Basic block 26

	li          t1, 50		// 0x32 ASCII '2'
	beq         t0, t1, .NeedsRegister_label_180

	// *** Basic block 27

	li          t1, 51		// 0x33 ASCII '3'
	beq         t0, t1, .NeedsRegister_label_181

	// *** Basic block 28

	li          t1, 52		// 0x34 ASCII '4'
	beq         t0, t1, .NeedsRegister_label_182

	// *** Basic block 29

	j           .NeedsRegister_label_206

	// *** Basic block 30

.NeedsRegister_label_174:

	// *** Basic block 31

.NeedsRegister_label_175:

	// *** Basic block 32

.NeedsRegister_label_176:

	// *** Basic block 33

.NeedsRegister_label_177:

	// *** Basic block 34

.NeedsRegister_label_178:

	// *** Basic block 35

.NeedsRegister_label_179:

	// *** Basic block 36

.NeedsRegister_label_180:

	// *** Basic block 37

.NeedsRegister_label_181:

	// *** Basic block 38

.NeedsRegister_label_182:

	// *** Basic block 39

.NeedsRegister_label_183:

	// *** Basic block 40

.NeedsRegister_label_184:

	// *** Basic block 41

.NeedsRegister_label_185:

	// *** Basic block 42

.NeedsRegister_label_186:

	// *** Basic block 43

.NeedsRegister_label_187:

	// *** Basic block 44

.NeedsRegister_label_188:

	// *** Basic block 45

.NeedsRegister_label_189:

	// *** Basic block 46

.NeedsRegister_label_190:

	// *** Basic block 47

.NeedsRegister_label_191:

	// *** Basic block 48

.NeedsRegister_label_192:

	// *** Basic block 49

.NeedsRegister_label_193:

	// *** Basic block 50

.NeedsRegister_label_194:

	// *** Basic block 51

.NeedsRegister_label_195:

	// *** Basic block 52

.NeedsRegister_label_196:

	// *** Basic block 53

.NeedsRegister_label_197:

	// *** Basic block 54

.NeedsRegister_label_198:

	// *** Basic block 55

.NeedsRegister_label_199:

	// *** Basic block 56

.NeedsRegister_label_200:
	mv          a0, x0

	// *** Basic block 57

.NeedsRegister_label_203:
	ret         

	// *** Basic block 58

.NeedsRegister_label_206:
	li          a0, 1		// 0x1 ASCII \x1
	ret         
.func_end_NeedsRegister:
	.size NeedsRegister, .func_end_NeedsRegister-NeedsRegister

	.local  AllocateRegister
	.type AllocateRegister, @function

AllocateRegister:

	// *** Basic block 0

	.local AllocateForRmov
	.local FreeRegisters
	.local NeedsRegister
	.local AllocateNewConstantRegister
	.global TargetIntValue
	.local AllocateNewSymbolRegister
	.local RegisterTypeFromInstruction
	.local AllocateNewOffsetRegister
	.local AllocateRegisterWithType
	.local FreeRegister
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
	mv          s1, a1
	mv          s2, a0
	lw          s3, 16(s1)
	addi        t1, s3, -38
	seqz        t0, t1
	li          t1, 38		// 0x26 ASCII '&'
	beq         s3, t1, .AllocateRegister_label_92

	// *** Basic block 1

	addi        t1, s3, -18
	seqz        t0, t1

	// *** Basic block 2

.AllocateRegister_label_92:
	bnez        t0, .AllocateRegister_label_97

	// *** Basic block 3

	addi        t1, s3, -39
	seqz        t0, t1

	// *** Basic block 4

.AllocateRegister_label_97:
	bnez        t0, .AllocateRegister_label_102

	// *** Basic block 5

	addi        t1, s3, -19
	seqz        t0, t1

	// *** Basic block 6

.AllocateRegister_label_102:
	bnez        t0, .AllocateRegister_label_107

	// *** Basic block 7

	addi        t1, s3, -20
	seqz        t0, t1

	// *** Basic block 8

.AllocateRegister_label_107:
	beqz        t0, .AllocateRegister_label_118

	// *** Basic block 9

	mv          a1, s1
	mv          a0, s2
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
	j           AllocateForRmov

	// *** Basic block 10

.AllocateRegister_label_115:
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

	// *** Basic block 11

.AllocateRegister_label_118:
	mv          a1, s1
	mv          a0, s2
	call        FreeRegisters

	// *** Basic block 12

	slti        t1, s3, 134
	not         t0, t1
	li          t1, 134		// 0x86 ASCII \x86
	blt         s3, t1, .AllocateRegister_label_132

	// *** Basic block 13

	li          t1, 165		// 0xa5 ASCII \xa5
	slt         t1, t1, s3
	not         t0, t1

	// *** Basic block 14

.AllocateRegister_label_132:
	beqz        t0, .AllocateRegister_label_142

	// *** Basic block 15

	lw          t0, 104(s1)
	li          t1, 65536		// 0x10000
	and         t0, t0, t1
	bnez        t0, .AllocateRegister_label_141

	// *** Basic block 16

	j           .AllocateRegister_label_115

	// *** Basic block 17

.AllocateRegister_label_141:

	// *** Basic block 18

.AllocateRegister_label_142:
	mv          a0, s1
	call        NeedsRegister

	// *** Basic block 19

	not         t0, a0
	beqz        t0, .AllocateRegister_label_149

	// *** Basic block 20

	j           .AllocateRegister_label_115

	// *** Basic block 21

.AllocateRegister_label_149:
	li          s4, 48		// 0x30 ASCII '0'
	blt         s3, s4, .AllocateRegister_label_239

	// *** Basic block 22

	li          s5, 187		// 0xbb ASCII \xbb
	blt         s3, s5, .AllocateRegister_label_197

	// *** Basic block 23

	beq         s3, s5, .AllocateRegister_label_510

	// *** Basic block 24

	li          t0, 188		// 0xbc ASCII \xbc
	beq         s3, t0, .AllocateRegister_label_513

	// *** Basic block 25

	li          t0, 189		// 0xbd ASCII \xbd
	beq         s3, t0, .AllocateRegister_label_516

	// *** Basic block 26

	li          t0, 190		// 0xbe ASCII \xbe
	beq         s3, t0, .AllocateRegister_label_519

	// *** Basic block 27

	li          t0, 191		// 0xbf ASCII \xbf
	beq         s3, t0, .AllocateRegister_label_522

	// *** Basic block 28

	li          t0, 192		// 0xc0 ASCII \xc0
	beq         s3, t0, .AllocateRegister_label_525

	// *** Basic block 29

	li          t0, 193		// 0xc1 ASCII \xc1
	beq         s3, t0, .AllocateRegister_label_528

	// *** Basic block 30

	li          t0, 194		// 0xc2 ASCII \xc2
	beq         s3, t0, .AllocateRegister_label_531

	// *** Basic block 31

	j           .AllocateRegister_label_562

	// *** Basic block 32

.AllocateRegister_label_197:
	beq         s3, s4, .AllocateRegister_label_445

	// *** Basic block 33

	li          t0, 49		// 0x31 ASCII '1'
	beq         s3, t0, .AllocateRegister_label_483

	// *** Basic block 34

	li          t0, 171		// 0xab ASCII \xab
	beq         s3, t0, .AllocateRegister_label_534

	// *** Basic block 35

	li          t0, 174		// 0xae ASCII \xae
	beq         s3, t0, .AllocateRegister_label_543

	// *** Basic block 36

	li          t0, 176		// 0xb0 ASCII \xb0
	beq         s3, t0, .AllocateRegister_label_552

	// *** Basic block 37

	li          t0, 177		// 0xb1 ASCII \xb1
	beq         s3, t0, .AllocateRegister_label_553

	// *** Basic block 38

	li          t0, 185		// 0xb9 ASCII \xb9
	beq         s3, t0, .AllocateRegister_label_504

	// *** Basic block 39

	li          t0, 186		// 0xba ASCII \xba
	beq         s3, t0, .AllocateRegister_label_507

	// *** Basic block 40

	j           .AllocateRegister_label_562

	// *** Basic block 41

.AllocateRegister_label_239:
	li          s4, 24		// 0x18 ASCII \x18
	blt         s3, s4, .AllocateRegister_label_284

	// *** Basic block 42

	beq         s3, s4, .AllocateRegister_label_480

	// *** Basic block 43

	li          t0, 26		// 0x1a ASCII \x1a
	beq         s3, t0, .AllocateRegister_label_489

	// *** Basic block 44

	li          t0, 27		// 0x1b ASCII \x1b
	beq         s3, t0, .AllocateRegister_label_498

	// *** Basic block 45

	li          t0, 28		// 0x1c ASCII \x1c
	beq         s3, t0, .AllocateRegister_label_501

	// *** Basic block 46

	li          t0, 29		// 0x1d ASCII \x1d
	beq         s3, t0, .AllocateRegister_label_486

	// *** Basic block 47

	li          t0, 40		// 0x28 ASCII '('
	beq         s3, t0, .AllocateRegister_label_492

	// *** Basic block 48

	li          t0, 41		// 0x29 ASCII ')'
	beq         s3, t0, .AllocateRegister_label_495

	// *** Basic block 49

	li          t0, 47		// 0x2f ASCII '/'
	beq         s3, t0, .AllocateRegister_label_444

	// *** Basic block 50

	j           .AllocateRegister_label_562

	// *** Basic block 51

.AllocateRegister_label_284:
	li          s4, 2		// 0x2 ASCII \x2
	beq         s3, s4, .AllocateRegister_label_424

	// *** Basic block 52

	li          s6, 5		// 0x5 ASCII \x5
	beq         s3, s6, .AllocateRegister_label_326

	// *** Basic block 53

	li          t0, 6		// 0x6 ASCII \x6
	beq         s3, t0, .AllocateRegister_label_344

	// *** Basic block 54

	li          t0, 7		// 0x7 ASCII \x7
	beq         s3, t0, .AllocateRegister_label_360

	// *** Basic block 55

	li          t0, 8		// 0x8 ASCII \x8
	beq         s3, t0, .AllocateRegister_label_376

	// *** Basic block 56

	li          t0, 9		// 0x9 ASCII \x9
	beq         s3, t0, .AllocateRegister_label_392

	// *** Basic block 57

	li          t0, 10		// 0xa ASCII \xa
	beq         s3, t0, .AllocateRegister_label_408

	// *** Basic block 58

	li          t0, 23		// 0x17 ASCII \x17
	beq         s3, t0, .AllocateRegister_label_477

	// *** Basic block 59

	j           .AllocateRegister_label_562

	// *** Basic block 60

.AllocateRegister_label_326:
	mv          a0, s1
	call        TargetIntValue

	// *** Basic block 61

	mv          a3, s1
	mv          a2, a0
	mv          a1, x0
	mv          a0, s2
	call        AllocateNewConstantRegister

	// *** Basic block 62

	mv          s5, a0
	j           .AllocateRegister_label_574

	// *** Basic block 63

.AllocateRegister_label_344:
	mv          a0, s1
	call        TargetIntValue

	// *** Basic block 64

	mv          a3, s1
	mv          a2, a0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s2
	call        AllocateNewConstantRegister

	// *** Basic block 65

	mv          s5, a0
	j           .AllocateRegister_label_574

	// *** Basic block 66

.AllocateRegister_label_360:
	mv          a0, s1
	call        TargetIntValue

	// *** Basic block 67

	mv          a3, s1
	mv          a2, a0
	mv          a1, s4
	mv          a0, s2
	call        AllocateNewConstantRegister

	// *** Basic block 68

	mv          s5, a0
	j           .AllocateRegister_label_574

	// *** Basic block 69

.AllocateRegister_label_376:
	mv          a0, s1
	call        TargetIntValue

	// *** Basic block 70

	mv          a3, s1
	mv          a2, a0
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s2
	call        AllocateNewConstantRegister

	// *** Basic block 71

	mv          s5, a0
	j           .AllocateRegister_label_574

	// *** Basic block 72

.AllocateRegister_label_392:
	mv          a0, s1
	call        TargetIntValue

	// *** Basic block 73

	mv          a3, s1
	mv          a2, a0
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	mv          a0, s2
	call        AllocateNewConstantRegister

	// *** Basic block 74

	mv          s5, a0
	j           .AllocateRegister_label_574

	// *** Basic block 75

.AllocateRegister_label_408:
	mv          a0, s1
	call        TargetIntValue

	// *** Basic block 76

	mv          a3, s1
	mv          a2, a0
	mv          a1, s6
	mv          a0, s2
	call        AllocateNewConstantRegister

	// *** Basic block 77

	mv          s5, a0
	j           .AllocateRegister_label_574

	// *** Basic block 78

.AllocateRegister_label_424:
	mv          s4, s1
	mv          a0, s1
	call        RegisterTypeFromInstruction

	// *** Basic block 79

	ld          t0, 112(s4)
	ld          a2, 16(t0)
	mv          a3, s1
	mv          a1, a0
	mv          a0, s2
	call        AllocateNewSymbolRegister

	// *** Basic block 80

	mv          s5, a0
	j           .AllocateRegister_label_574

	// *** Basic block 81

.AllocateRegister_label_444:

	// *** Basic block 82

.AllocateRegister_label_445:
	mv          a0, s1
	call        RegisterTypeFromInstruction

	// *** Basic block 83

	li          t0, 47		// 0x2f ASCII '/'
	bne         s3, t0, .AllocateRegister_label_456

	// *** Basic block 84

	li          s6, 2		// 0x2 ASCII \x2
	j           .AllocateRegister_label_458

	// *** Basic block 85

.AllocateRegister_label_456:
	li          s6, 3		// 0x3 ASCII \x3

	// *** Basic block 86

.AllocateRegister_label_458:
	ld          a0, 40(s1)
	call        TargetIntValue

	// *** Basic block 87

	sext.w      a3, a0
	mv          a4, s1
	mv          a2, s6
	mv          a1, a0
	mv          a0, s2
	call        AllocateNewOffsetRegister

	// *** Basic block 88

	mv          s5, a0
	j           .AllocateRegister_label_574

	// *** Basic block 89

.AllocateRegister_label_477:
	addi        s5, s2, 840
	j           .AllocateRegister_label_574

	// *** Basic block 90

.AllocateRegister_label_480:
	addi        s5, s2, 808
	j           .AllocateRegister_label_574

	// *** Basic block 91

.AllocateRegister_label_483:
	addi        s5, s2, 776
	j           .AllocateRegister_label_574

	// *** Basic block 92

.AllocateRegister_label_486:
	addi        s5, s2, 136
	j           .AllocateRegister_label_574

	// *** Basic block 93

.AllocateRegister_label_489:
	addi        s5, s2, 392
	j           .AllocateRegister_label_574

	// *** Basic block 94

.AllocateRegister_label_492:
	addi        s5, s2, 136
	j           .AllocateRegister_label_574

	// *** Basic block 95

.AllocateRegister_label_495:
	addi        s5, s2, 264
	j           .AllocateRegister_label_574

	// *** Basic block 96

.AllocateRegister_label_498:
	addi        s5, s2, 520
	j           .AllocateRegister_label_574

	// *** Basic block 97

.AllocateRegister_label_501:
	addi        s5, s2, 648
	j           .AllocateRegister_label_574

	// *** Basic block 98

.AllocateRegister_label_504:
	addi        s5, s2, 136
	j           .AllocateRegister_label_574

	// *** Basic block 99

.AllocateRegister_label_507:
	addi        s5, s2, 264
	j           .AllocateRegister_label_574

	// *** Basic block 100

.AllocateRegister_label_510:
	addi        s5, s2, 392
	j           .AllocateRegister_label_574

	// *** Basic block 101

.AllocateRegister_label_513:
	addi        s5, s2, 520
	j           .AllocateRegister_label_574

	// *** Basic block 102

.AllocateRegister_label_516:
	addi        s5, s2, 648
	j           .AllocateRegister_label_574

	// *** Basic block 103

.AllocateRegister_label_519:
	addi        s5, s2, 136
	j           .AllocateRegister_label_574

	// *** Basic block 104

.AllocateRegister_label_522:
	addi        s5, s2, 264
	j           .AllocateRegister_label_574

	// *** Basic block 105

.AllocateRegister_label_525:
	addi        s5, s2, 392
	j           .AllocateRegister_label_574

	// *** Basic block 106

.AllocateRegister_label_528:
	addi        s5, s2, 520
	j           .AllocateRegister_label_574

	// *** Basic block 107

.AllocateRegister_label_531:
	addi        s5, s2, 648
	j           .AllocateRegister_label_574

	// *** Basic block 108

.AllocateRegister_label_534:
	li          t0, 5		// 0x5 ASCII \x5
	mv          a1, t0
	mv          a0, s2
	call        AllocateRegisterWithType

	// *** Basic block 109

	mv          s5, a0
	j           .AllocateRegister_label_574

	// *** Basic block 110

.AllocateRegister_label_543:
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	mv          a0, s2
	call        AllocateRegisterWithType

	// *** Basic block 111

	mv          s5, a0
	j           .AllocateRegister_label_574

	// *** Basic block 112

.AllocateRegister_label_552:

	// *** Basic block 113

.AllocateRegister_label_553:
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s2
	call        AllocateRegisterWithType

	// *** Basic block 114

	mv          s5, a0
	j           .AllocateRegister_label_574

	// *** Basic block 115

.AllocateRegister_label_562:
	mv          a0, s1
	call        RegisterTypeFromInstruction

	// *** Basic block 116

	mv          s3, a0
	mv          a1, s3
	mv          a0, s2
	call        AllocateRegisterWithType

	// *** Basic block 117

	mv          s5, a0

	// *** Basic block 118

.AllocateRegister_label_574:
	addi        t0, s1, 64
	ld          t0, 8(t0)
	sw          t0, 88(s1)
	sd          s5, 32(s1)
	sd          s1, 8(s5)
	lw          t0, 88(s1)
	bnez        t0, .AllocateRegister_label_593

	// *** Basic block 119

	mv          a1, s5
	mv          a0, s2
	call        FreeRegister

	// *** Basic block 120

.AllocateRegister_label_593:
	j           .AllocateRegister_label_115
.func_end_AllocateRegister:
	.size AllocateRegister, .func_end_AllocateRegister-AllocateRegister

	.global W65C02RegisterAsString
	.type W65C02RegisterAsString, @function

W65C02RegisterAsString:

	// *** Basic block 0

	.global snprintf
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
	mv          s1, a0
	mv          s2, a2
	mv          s3, a3
	mv          s4, a1
	lw          t0, 20(s1)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .W65C02RegisterAsString_label_50

	// *** Basic block 2

	j           .W65C02RegisterAsString_label_201

	// *** Basic block 3

	j           .W65C02RegisterAsString_label_216

	// *** Basic block 4

	j           .W65C02RegisterAsString_label_226

	// *** Basic block 5

	j           .W65C02RegisterAsString_label_236

	// *** Basic block 6

.W65C02RegisterAsString_label_50:
	lw          t0, 16(s1)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 7

	j           .W65C02RegisterAsString_label_63

	// *** Basic block 8

	j           .W65C02RegisterAsString_label_78

	// *** Basic block 9

	j           .W65C02RegisterAsString_label_144

	// *** Basic block 10

	j           .W65C02RegisterAsString_label_158

	// *** Basic block 11

	j           .W65C02RegisterAsString_label_172

	// *** Basic block 12

	j           .W65C02RegisterAsString_label_186

	// *** Basic block 13

.W65C02RegisterAsString_label_63:
	lla         a2, .str.11
	lw          a3, 0(s1)
	mv          a4, s4
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 14

	j           .W65C02RegisterAsString_label_199

	// *** Basic block 15

.W65C02RegisterAsString_label_78:
	lw          s5, 0(s1)
	li          t0, 26		// 0x1a ASCII \x1a
	bne         s5, t0, .W65C02RegisterAsString_label_96

	// *** Basic block 16

	lla         a2, .str.12
	mv          a3, s4
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 17

	j           .W65C02RegisterAsString_label_199

	// *** Basic block 18

.W65C02RegisterAsString_label_96:
	li          t0, 28		// 0x1c ASCII \x1c
	bne         s5, t0, .W65C02RegisterAsString_label_113

	// *** Basic block 19

	lla         a2, .str.13
	mv          a3, s4
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 20

	j           .W65C02RegisterAsString_label_199

	// *** Basic block 21

.W65C02RegisterAsString_label_113:
	li          t0, 24		// 0x18 ASCII \x18
	bne         s5, t0, .W65C02RegisterAsString_label_130

	// *** Basic block 22

	lla         a2, .str.14
	mv          a3, s4
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 23

	j           .W65C02RegisterAsString_label_199

	// *** Basic block 24

.W65C02RegisterAsString_label_130:
	lla         a2, .str.15
	mv          a4, s4
	mv          a3, s5
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 25

	j           .W65C02RegisterAsString_label_199

	// *** Basic block 26

.W65C02RegisterAsString_label_144:
	lla         a2, .str.16
	lw          a3, 0(s1)
	mv          a4, s4
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 27

	j           .W65C02RegisterAsString_label_199

	// *** Basic block 28

.W65C02RegisterAsString_label_158:
	lla         a2, .str.17
	lw          a3, 0(s1)
	mv          a4, s4
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 29

	j           .W65C02RegisterAsString_label_199

	// *** Basic block 30

.W65C02RegisterAsString_label_172:
	lla         a2, .str.18
	lw          a3, 0(s1)
	mv          a4, s4
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 31

	j           .W65C02RegisterAsString_label_199

	// *** Basic block 32

.W65C02RegisterAsString_label_186:
	lla         a2, .str.19
	lw          a3, 0(s1)
	mv          a4, s4
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 33

.W65C02RegisterAsString_label_199:
	j           .W65C02RegisterAsString_label_254

	// *** Basic block 34

.W65C02RegisterAsString_label_201:
	lla         a2, .str.20
	ld          a4, 24(s1)
	mv          a3, s4
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 35

	j           .W65C02RegisterAsString_label_254

	// *** Basic block 36

.W65C02RegisterAsString_label_216:
	lla         a2, .str.21
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 37

	j           .W65C02RegisterAsString_label_254

	// *** Basic block 38

.W65C02RegisterAsString_label_226:
	lla         a2, .str.22
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 39

	j           .W65C02RegisterAsString_label_254

	// *** Basic block 40

.W65C02RegisterAsString_label_236:
	lla         a2, .str.23
	ld          t0, 24(s1)
	slli        t1, s4, 3
	sra         t0, t0, t1
	sext.w      t0, t0
	andi        a3, t0, 255
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 41

	j           .W65C02RegisterAsString_label_254

	// *** Basic block 42

.W65C02RegisterAsString_label_254:
	mv          a0, s2

	// *** Basic block 43

.W65C02RegisterAsString_label_257:
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
.func_end_W65C02RegisterAsString:
	.size W65C02RegisterAsString, .func_end_W65C02RegisterAsString-W65C02RegisterAsString

	.global W65C02AllocateRegisters
	.type W65C02AllocateRegisters, @function

W65C02AllocateRegisters:

	// *** Basic block 0

	.global TargetFirstInstruction
	.local AllocateRegister
	.global TargetNext
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
	ld          a0, 0(s1)
	call        TargetFirstInstruction

	// *** Basic block 1

	mv          s2, a0
	beq         s2, x0, .W65C02AllocateRegisters_label_32

	// *** Basic block 2

.W65C02AllocateRegisters_label_18:
	mv          a1, s2
	mv          a0, s1
	call        AllocateRegister

	// *** Basic block 3

	mv          a0, s2
	call        TargetNext

	// *** Basic block 4

	mv          s2, a0
	bne         s2, x0, .W65C02AllocateRegisters_label_18

	// *** Basic block 5

.W65C02AllocateRegisters_label_32:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_W65C02AllocateRegisters:
	.size W65C02AllocateRegisters, .func_end_W65C02AllocateRegisters-W65C02AllocateRegisters

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "opcode: %s\n"
	.type .str.1, @object
	.size .str.1, 12

.str.2:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.2, @object
	.size .str.2, 30

.str.3:
	.asciz "(null)"
	.type .str.3, @object
	.size .str.3, 1

.str.4:
	.asciz "false"
	.type .str.4, @object
	.size .str.4, 6

.str.5:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.5, @object
	.size .str.5, 30

.str.6:
	.asciz "(null)"
	.type .str.6, @object
	.size .str.6, 1

.str.7:
	.asciz "op->uses >= 0"
	.type .str.7, @object
	.size .str.7, 14

.str.8:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.8, @object
	.size .str.8, 30

.str.9:
	.asciz "(null)"
	.type .str.9, @object
	.size .str.9, 1

.str.10:
	.asciz "reg != NULL"
	.type .str.10, @object
	.size .str.10, 12

.str.11:
	.asciz "b%d+%d"
	.type .str.11, @object
	.size .str.11, 7

.str.12:
	.asciz "sp+%d"
	.type .str.12, @object
	.size .str.12, 6

.str.13:
	.asciz "fp+%d"
	.type .str.13, @object
	.size .str.13, 6

.str.14:
	.asciz "ap+%d"
	.type .str.14, @object
	.size .str.14, 6

.str.15:
	.asciz "a%d+%d"
	.type .str.15, @object
	.size .str.15, 7

.str.16:
	.asciz "i%d+%d"
	.type .str.16, @object
	.size .str.16, 7

.str.17:
	.asciz "x%d+%d"
	.type .str.17, @object
	.size .str.17, 7

.str.18:
	.asciz "f%d+%d"
	.type .str.18, @object
	.size .str.18, 7

.str.19:
	.asciz "d%d+%d"
	.type .str.19, @object
	.size .str.19, 7

.str.20:
	.asciz "%%byte%d(%s)"
	.type .str.20, @object
	.size .str.20, 13

.str.21:
	.asciz "(fp),Y"
	.type .str.21, @object
	.size .str.21, 7

.str.22:
	.asciz "(ap),Y"
	.type .str.22, @object
	.size .str.22, 7

.str.23:
	.asciz "#%d"
	.type .str.23, @object
	.size .str.23, 4

