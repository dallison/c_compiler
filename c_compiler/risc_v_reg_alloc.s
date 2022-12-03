	.file   "risc_v_reg_alloc.c"
	.text
	.option pic
.PCbegin:
	.local  Trap
	.type Trap, @function

Trap:

	// *** Basic block 0

	ret         
.func_end_Trap:
	.size Trap, .func_end_Trap-Trap

	.local  TrapInstruction
	.type TrapInstruction, @function

TrapInstruction:

	// *** Basic block 0

	.local Trap
	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .TrapInstruction_label_18

	// *** Basic block 1

	j           Trap

	// *** Basic block 2

.TrapInstruction_label_18:
	ret         
.func_end_TrapInstruction:
	.size TrapInstruction, .func_end_TrapInstruction-TrapInstruction

	.local  TrapBlock
	.type TrapBlock, @function

TrapBlock:

	// *** Basic block 0

	.local Trap
	// Leaf procedure, no stack frame generated
	ld          t0, 0(a0)
	li          t1, 82		// 0x52 ASCII 'R'
	bne         t0, t1, .TrapBlock_label_17

	// *** Basic block 1

	j           Trap

	// *** Basic block 2

.TrapBlock_label_17:
	ret         
.func_end_TrapBlock:
	.size TrapBlock, .func_end_TrapBlock-TrapBlock

	.local  InitializeRegister
	.type InitializeRegister, @function

InitializeRegister:

	// *** Basic block 0

	.global TargetRegisterInit
	// Leaf procedure, no stack frame generated
	j           TargetRegisterInit
.func_end_InitializeRegister:
	.size InitializeRegister, .func_end_InitializeRegister-InitializeRegister

	.global RVRegisterAllocatorInit
	.type RVRegisterAllocatorInit, @function

RVRegisterAllocatorInit:

	// *** Basic block 0

	.local InitializeRegister
	.global BitSetInit
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

.RVRegisterAllocatorInit_label_32:
	addi        t0, s1, 8
	slli        t1, s2, 3
	slli        t2, s2, 4
	add         t1, t1, t2
	add         a0, t0, t1
	mv          a2, x0
	mv          a1, s2
	call        InitializeRegister

	// *** Basic block 2

.RVRegisterAllocatorInit_label_45:
	addi        s2, s2, 1
	li          s3, 32		// 0x20 ASCII ' '
	bge         s2, s3, .RVRegisterAllocatorInit_label_32

	// *** Basic block 3

.RVRegisterAllocatorInit_label_50:
	mv          s2, x0

	// *** Basic block 4

.RVRegisterAllocatorInit_label_54:
	addi        t0, s1, 776
	slli        t1, s2, 3
	slli        t2, s2, 4
	add         t1, t1, t2
	add         a0, t0, t1
	li          s4, 1		// 0x1 ASCII \x1
	mv          a2, s4
	mv          a1, s2
	call        InitializeRegister

	// *** Basic block 5

.RVRegisterAllocatorInit_label_67:
	addi        s2, s2, 1
	bge         s2, s3, .RVRegisterAllocatorInit_label_54

	// *** Basic block 6

.RVRegisterAllocatorInit_label_72:
	addi        t0, s1, 8
	sb          s4, 4(t0)
	addi        t0, s1, 8
	addi        t0, t0, 192
	sb          s4, 4(t0)
	addi        t0, s1, 8
	addi        t0, t0, 48
	sb          s4, 4(t0)
	addi        t0, s1, 8
	addi        t0, t0, 72
	sb          s4, 4(t0)
	addi        a0, s1, 1544
	call        BitSetInit

	// *** Basic block 7

	addi        a0, s1, 1560
	call        BitSetInit

	// *** Basic block 8

	sw          x0, 1576(s1)
	sw          x0, 1580(s1)
	addi        a0, s1, 1584
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           BitSetInit
.func_end_RVRegisterAllocatorInit:
	.size RVRegisterAllocatorInit, .func_end_RVRegisterAllocatorInit-RVRegisterAllocatorInit

	.global NewRVRegisterAllocator
	.type NewRVRegisterAllocator, @function

NewRVRegisterAllocator:

	// *** Basic block 0

	.global malloc
	.global RVRegisterAllocatorInit
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
	li          a0, 1600		// 0x640
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        RVRegisterAllocatorInit

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.NewRVRegisterAllocator_label_21:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewRVRegisterAllocator:
	.size NewRVRegisterAllocator, .func_end_NewRVRegisterAllocator-NewRVRegisterAllocator

	.global RVRegisterAllocatorDestruct
	.type RVRegisterAllocatorDestruct, @function

RVRegisterAllocatorDestruct:

	// *** Basic block 0

	.global BitSetDestruct
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
	addi        a0, s1, 1544
	call        BitSetDestruct

	// *** Basic block 1

	addi        a0, s1, 1560
	call        BitSetDestruct

	// *** Basic block 2

	addi        a0, s1, 1584
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           BitSetDestruct
.func_end_RVRegisterAllocatorDestruct:
	.size RVRegisterAllocatorDestruct, .func_end_RVRegisterAllocatorDestruct-RVRegisterAllocatorDestruct

	.global RVRegisterAllocatorDelete
	.type RVRegisterAllocatorDelete, @function

RVRegisterAllocatorDelete:

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
	.global RVRegisterAllocatorDestruct
	.global free
	mv          s1, a0
	call        RVRegisterAllocatorDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_RVRegisterAllocatorDelete:
	.size RVRegisterAllocatorDelete, .func_end_RVRegisterAllocatorDelete-RVRegisterAllocatorDelete

	.local  AssignRegister
	.type AssignRegister, @function

AssignRegister:

	// *** Basic block 0

	.global printf
	.global abort
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
	mv          s2, a0
	ld          t0, 32(s1)
	bne         t0, x0, .AssignRegister_label_28

	// *** Basic block 1

	j           .AssignRegister_label_46

	// *** Basic block 2

.AssignRegister_label_28:
	lla         a0, .str.12
	lla         a1, .str.13
	lla         a3, .str.14
	li          t0, 110		// 0x6e ASCII 'n'
	mv          a2, t0
	call        printf

	// *** Basic block 3

	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           abort

	// *** Basic block 4

.AssignRegister_label_46:
	sd          s2, 32(s1)
	sd          s1, 8(s2)
	addi        t0, s1, 64
	ld          t0, 8(t0)
	sw          t0, 88(s1)
	lw          t0, 104(s1)
	ori         t0, t0, 4
	sw          t0, 104(s1)
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AssignRegister:
	.size AssignRegister, .func_end_AssignRegister-AssignRegister

	.local  FindFreeRegister
	.type FindFreeRegister, @function

FindFreeRegister:

	// *** Basic block 0

	.local register_ranges
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	mv          t2, a2
	bnez        t0, .FindFreeRegister_label_31

	// *** Basic block 1

	not         t4, t2
	addi        t3, t1, 8
	j           .FindFreeRegister_label_33

	// *** Basic block 2

.FindFreeRegister_label_31:
	addi        t3, t1, 776

	// *** Basic block 3

.FindFreeRegister_label_33:
	mv          t4, x0

	// *** Basic block 4

.FindFreeRegister_label_38:
	slli        t5, t4, 5
	la          t6, register_ranges
	add         t5, t6, t5
	lw          t6, 0(t5)
	bne         t6, t0, .FindFreeRegister_label_102

	// *** Basic block 5

	beqz        t4, .FindFreeRegister_label_51

	// *** Basic block 6

	lb          t4, 28(t5)

	// *** Basic block 7

.FindFreeRegister_label_51:
	bnez        t4, .FindFreeRegister_label_103

	// *** Basic block 8

.FindFreeRegister_label_53:
	lw          t0, 4(t5)
	lw          t5, 8(t5)
	bge         t0, t5, .FindFreeRegister_label_101

	// *** Basic block 9

.FindFreeRegister_label_62:
	slli        a1, t0, 3
	slli        a2, t0, 4
	add         a1, a1, a2
	add         a2, t3, a1
	lb          a3, 4(a2)
	not         t6, a3
	beqz        t6, .FindFreeRegister_label_77

	// *** Basic block 10

	ld          a3, 8(a2)
	sub         a3, a3, x0
	seqz        t6, a3

	// *** Basic block 11

.FindFreeRegister_label_77:
	beqz        t6, .FindFreeRegister_label_95

	// *** Basic block 12

	lw          t6, 0(a2)
	li          a2, 1		// 0x1 ASCII \x1
	bne         t6, a2, .FindFreeRegister_label_89

	// *** Basic block 13

	ld          t6, 0(t1)
	sb          a2, 204(t6)

	// *** Basic block 14

.FindFreeRegister_label_89:
	add         a0, t3, a1

	// *** Basic block 15

.FindFreeRegister_label_92:
	ret         

	// *** Basic block 16

.FindFreeRegister_label_95:

	// *** Basic block 17

.FindFreeRegister_label_96:
	addi        t0, t0, 1
	bge         t0, t5, .FindFreeRegister_label_62

	// *** Basic block 18

.FindFreeRegister_label_101:

	// *** Basic block 19

.FindFreeRegister_label_102:

	// *** Basic block 20

.FindFreeRegister_label_103:
	addi        t4, t4, 1
	li          t0, 11		// 0xb ASCII \xb
	bge         t4, t0, .FindFreeRegister_label_38

	// *** Basic block 21

.FindFreeRegister_label_109:
	mv          a0, x0
	ret         
.func_end_FindFreeRegister:
	.size FindFreeRegister, .func_end_FindFreeRegister-FindFreeRegister

	.local  FreeRegister
	.type FreeRegister, @function

FreeRegister:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sd          x0, 8(a1)
	ret         
.func_end_FreeRegister:
	.size FreeRegister, .func_end_FreeRegister-FreeRegister

	.local  FreeRegisters
	.type FreeRegisters, @function

FreeRegisters:

	// *** Basic block 0

	.global RVIsFixedRegister
	.global printf
	.global abort
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
	li          t0, -1		// 0xffffffffffffffff
	ld          s3, 24(s1)
	beq         s3, x0, .FreeRegisters_label_42

	// *** Basic block 1

	addi        t1, s1, 40
	lw          t0, 20(s3)

	// *** Basic block 2

.FreeRegisters_label_42:
	mv          s3, x0

	// *** Basic block 3

.FreeRegisters_label_48:
	slli        t1, s3, 3
	add         t1, t1, t1
	ld          s4, 0(t1)
	beq         s4, x0, .FreeRegisters_label_135

	// *** Basic block 4

	lw          t1, 20(s4)
	bne         t1, t0, .FreeRegisters_label_63

	// *** Basic block 6

.FreeRegisters_label_63:
	lw          a0, 16(s4)
	call        RVIsFixedRegister

	// *** Basic block 7

	bnez        a0, .FreeRegisters_label_136

	// *** Basic block 8

.FreeRegisters_label_69:
	addi        t1, a0, -211
	seqz        t0, t1
	li          t1, 211		// 0xd3 ASCII \xd3
	beq         a0, t1, .FreeRegisters_label_79

	// *** Basic block 9

	addi        t1, a0, -210
	seqz        t0, t1

	// *** Basic block 10

.FreeRegisters_label_79:
	bnez        t0, .FreeRegisters_label_136

	// *** Basic block 11

.FreeRegisters_label_81:
	ld          s5, 32(s4)
	sub         t1, s5, x0
	snez        t0, t1
	beq         s5, x0, .FreeRegisters_label_92

	// *** Basic block 12

	lb          t1, 4(s5)
	not         t0, t1

	// *** Basic block 13

.FreeRegisters_label_92:
	beqz        t0, .FreeRegisters_label_134

	// *** Basic block 14

	lw          s6, 88(s4)
	addi        t0, s6, -1
	sw          t0, 88(s4)
	blt         s6, x0, .FreeRegisters_label_102

	// *** Basic block 15

	j           .FreeRegisters_label_119

	// *** Basic block 16

.FreeRegisters_label_102:
	lla         a0, .str.15
	lla         a1, .str.16
	lla         a3, .str.17
	li          t0, 173		// 0xad ASCII \xad
	mv          a2, t0
	call        printf

	// *** Basic block 17

	call        abort

	// *** Basic block 18

.FreeRegisters_label_119:
	bnez        s6, .FreeRegisters_label_133

	// *** Basic block 19

	ld          t0, 8(s5)
	bne         t0, s4, .FreeRegisters_label_132

	// *** Basic block 20

	mv          a1, s5
	mv          a0, s2
	call        FreeRegister

	// *** Basic block 21

.FreeRegisters_label_132:

	// *** Basic block 22

.FreeRegisters_label_133:

	// *** Basic block 23

.FreeRegisters_label_134:

	// *** Basic block 24

.FreeRegisters_label_135:

	// *** Basic block 25

.FreeRegisters_label_136:
	addi        s3, s3, 1
	li          t0, 3		// 0x3 ASCII \x3
	bge         s3, t0, .FreeRegisters_label_48

	// *** Basic block 26

.FreeRegisters_label_141:
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
.func_end_FreeRegisters:
	.size FreeRegisters, .func_end_FreeRegisters-FreeRegisters

	.local  IsSavedReg
	.type IsSavedReg, @function

IsSavedReg:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 0(a0)
	slti        t2, t0, 8
	not         t1, t2
	li          t2, 8		// 0x8 ASCII \x8
	blt         t0, t2, .IsSavedReg_label_25

	// *** Basic block 1

	li          t3, 9		// 0x9 ASCII \x9
	slt         t3, t3, t0
	not         t1, t3

	// *** Basic block 2

.IsSavedReg_label_25:
	bnez        t1, .IsSavedReg_label_36

	// *** Basic block 3

	slti        t3, t0, 18
	not         t1, t3
	li          t3, 18		// 0x12 ASCII \x12
	blt         t0, t3, .IsSavedReg_label_35

	// *** Basic block 4

	li          t3, 27		// 0x1b ASCII \x1b
	slt         t3, t3, t0
	not         t1, t3

	// *** Basic block 5

.IsSavedReg_label_35:

	// *** Basic block 6

.IsSavedReg_label_36:
	bnez        t1, .IsSavedReg_label_46

	// *** Basic block 7

	blt         t0, t2, .IsSavedReg_label_45

	// *** Basic block 8

	li          t2, 9		// 0x9 ASCII \x9
	slt         t2, t2, t0
	not         t1, t2

	// *** Basic block 9

.IsSavedReg_label_45:

	// *** Basic block 10

.IsSavedReg_label_46:
	bnez        t1, .IsSavedReg_label_57

	// *** Basic block 11

	slti        t2, t0, 18
	not         t1, t2
	li          t2, 18		// 0x12 ASCII \x12
	blt         t0, t2, .IsSavedReg_label_56

	// *** Basic block 12

	li          t2, 27		// 0x1b ASCII \x1b
	slt         t0, t2, t0
	not         t1, t0

	// *** Basic block 13

.IsSavedReg_label_56:

	// *** Basic block 14

.IsSavedReg_label_57:
	beqz        t1, .IsSavedReg_label_64

	// *** Basic block 15

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 16

.IsSavedReg_label_61:
	ret         

	// *** Basic block 17

.IsSavedReg_label_64:
	mv          a0, x0
	ret         
.func_end_IsSavedReg:
	.size IsSavedReg, .func_end_IsSavedReg-IsSavedReg

	.local  SpillCost
	.type SpillCost, @function

SpillCost:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 88(a0)
	mv          t1, x0
	addi        t2, a0, 64
	ld          t2, 8(t2)
	bge         x0, t2, .SpillCost_label_61

	// *** Basic block 1

	ld          t3, 64(a0)

	// *** Basic block 2

.SpillCost_label_28:
	slli        t4, t1, 3
	add         t3, t3, t4
	ld          t4, 0(t3)
	ld          t3, 32(t4)
	bne         t3, x0, .SpillCost_label_56

	// *** Basic block 3

	ld          t3, 96(t4)
	beq         t3, x0, .SpillCost_label_55

	// *** Basic block 4

.SpillCost_label_45:
	lw          t4, 212(t3)
	addi        t4, t4, 1
	mul         t0, t0, t4
	ld          t3, 136(t3)
	bne         t3, x0, .SpillCost_label_45

	// *** Basic block 5

.SpillCost_label_55:

	// *** Basic block 6

.SpillCost_label_56:

	// *** Basic block 7

.SpillCost_label_57:
	addi        t1, t1, 1
	bge         t1, t2, .SpillCost_label_28

	// *** Basic block 8

.SpillCost_label_61:
	mv          a0, t0

	// *** Basic block 9

.SpillCost_label_64:
	ret         
.func_end_SpillCost:
	.size SpillCost, .func_end_SpillCost-SpillCost

	.local  FindSpillVictim
	.type FindSpillVictim, @function

FindSpillVictim:

	// *** Basic block 0

	.local register_ranges
	.global printf
	.global abort
	.local SpillCost
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
	mv          t0, a1
	mv          s1, a0
	bnez        t0, .FindSpillVictim_label_41

	// *** Basic block 1

	addi        s2, s1, 8
	j           .FindSpillVictim_label_43

	// *** Basic block 2

.FindSpillVictim_label_41:
	addi        s2, s1, 776

	// *** Basic block 3

.FindSpillVictim_label_43:
	li          s3, 2147483647		// 0x7fffffff
	mv          s4, x0
	mv          s5, x0

	// *** Basic block 4

.FindSpillVictim_label_53:
	slli        t1, s5, 5
	la          t2, register_ranges
	add         s6, t2, t1
	lw          t1, 0(s6)
	bne         t1, t0, .FindSpillVictim_label_145

	// *** Basic block 5

	lw          s7, 4(s6)
	lw          s6, 8(s6)
	bge         s7, s6, .FindSpillVictim_label_144

	// *** Basic block 6

.FindSpillVictim_label_70:
	slli        t1, s7, 3
	slli        t2, s7, 4
	add         t1, t1, t2
	add         s2, s2, t1
	lb          t1, 4(s2)
	not         t0, t1
	beqz        t0, .FindSpillVictim_label_85

	// *** Basic block 7

	ld          t1, 8(s2)
	sub         t1, t1, x0
	snez        t0, t1

	// *** Basic block 8

.FindSpillVictim_label_85:
	beqz        t0, .FindSpillVictim_label_138

	// *** Basic block 9

	ld          s8, 8(s2)
	lw          s2, 16(s8)
	addi        t1, s2, -210
	seqz        t0, t1
	li          t1, 210		// 0xd2 ASCII \xd2
	beq         s2, t1, .FindSpillVictim_label_101

	// *** Basic block 10

	addi        t1, s2, -211
	seqz        t0, t1

	// *** Basic block 11

.FindSpillVictim_label_101:
	bnez        t0, .FindSpillVictim_label_139

	// *** Basic block 12

.FindSpillVictim_label_103:
	lw          t0, 104(s8)
	andi        t0, t0, 2
	bnez        t0, .FindSpillVictim_label_110

	// *** Basic block 13

	j           .FindSpillVictim_label_127

	// *** Basic block 14

.FindSpillVictim_label_110:
	lla         a0, .str.18
	lla         a1, .str.19
	lla         a3, .str.20
	li          t0, 250		// 0xfa ASCII \xfa
	mv          a2, t0
	call        printf

	// *** Basic block 15

	call        abort

	// *** Basic block 16

.FindSpillVictim_label_127:
	mv          a0, s8
	call        SpillCost

	// *** Basic block 17

	mv          s2, a0
	bge         s2, s3, .FindSpillVictim_label_137

	// *** Basic block 18

	mv          s4, s8

	// *** Basic block 19

.FindSpillVictim_label_137:

	// *** Basic block 20

.FindSpillVictim_label_138:

	// *** Basic block 21

.FindSpillVictim_label_139:
	addi        s7, s7, 1
	bge         s7, s6, .FindSpillVictim_label_70

	// *** Basic block 22

.FindSpillVictim_label_144:

	// *** Basic block 23

.FindSpillVictim_label_145:

	// *** Basic block 24

.FindSpillVictim_label_146:
	addi        s5, s5, 1
	li          t0, 11		// 0xb ASCII \xb
	bge         s5, t0, .FindSpillVictim_label_53

	// *** Basic block 25

.FindSpillVictim_label_152:
	beq         s4, x0, .FindSpillVictim_label_157

	// *** Basic block 26

	j           .FindSpillVictim_label_172

	// *** Basic block 27

.FindSpillVictim_label_157:
	lla         a0, .str.21
	lla         a1, .str.22
	lla         a3, .str.23
	li          t0, 260		// 0x104
	mv          a2, t0
	call        printf

	// *** Basic block 28

	call        abort

	// *** Basic block 29

.FindSpillVictim_label_172:
	mv          a0, s4

	// *** Basic block 30

.FindSpillVictim_label_175:
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
.func_end_FindSpillVictim:
	.size FindSpillVictim, .func_end_FindSpillVictim-FindSpillVictim

	.local  NotProcessed
	.type NotProcessed, @function

NotProcessed:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 104(a0)
	andi        t0, t0, 4
	seqz        a0, t0

	// *** Basic block 1

.NotProcessed_label_14:
	ret         
.func_end_NotProcessed:
	.size NotProcessed, .func_end_NotProcessed-NotProcessed

	.local  SpillInstruction
	.type SpillInstruction, @function

SpillInstruction:

	// *** Basic block 0

	.global TargetNewInstruction2
	.global TargetGetIntConstant
	.global RVIsVarRegister
	.global printf
	.global abort
	.global TargetBasicBlockEmitAfter
	.global TargetRetargetInstructionIf
	.local NotProcessed
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
	ld          s3, 32(s1)
	ld          a0, 0(s2)
	lw          a3, 1576(s2)
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	call        TargetGetIntConstant

	// *** Basic block 1

	mv          a2, a0
	mv          a1, x0
	li          t0, 210		// 0xd2 ASCII \xd2
	mv          a0, t0
	call        TargetNewInstruction2

	// *** Basic block 2

	mv          s4, a0
	lw          s5, 1576(s2)
	addi        t0, s5, 8
	sw          t0, 1576(s2)
	lw          t0, 1580(s2)
	bge         t0, s5, .SpillInstruction_label_69

	// *** Basic block 3

	sw          s5, 1580(s2)

	// *** Basic block 4

.SpillInstruction_label_69:
	mv          a0, s1
	call        RVIsVarRegister

	// *** Basic block 5

	beqz        a0, .SpillInstruction_label_110

	// *** Basic block 6

	addi        t0, s1, 64
	ld          t0, 8(t0)
	bge         x0, t0, .SpillInstruction_label_80

	// *** Basic block 7

	j           .SpillInstruction_label_95

	// *** Basic block 8

.SpillInstruction_label_80:
	lla         a0, .str.24
	lla         a1, .str.25
	lla         a3, .str.26
	li          t0, 292		// 0x124
	mv          a2, t0
	call        printf

	// *** Basic block 9

	call        abort

	// *** Basic block 10

.SpillInstruction_label_95:
	ld          t0, 64(s1)
	ld          s5, 0(t0)
	ld          a0, 0(s2)
	ld          a1, 96(s5)
	mv          a3, s5
	mv          a2, s4
	call        TargetBasicBlockEmitAfter

	// *** Basic block 11

	j           .SpillInstruction_label_121

	// *** Basic block 12

.SpillInstruction_label_110:
	ld          a0, 0(s2)
	ld          a1, 96(s1)
	mv          a3, s1
	mv          a2, s4
	call        TargetBasicBlockEmitAfter

	// *** Basic block 13

.SpillInstruction_label_121:
	la          t0, NotProcessed
	mv          a2, t0
	mv          a1, s4
	mv          a0, s1
	call        TargetRetargetInstructionIf

	// *** Basic block 14

	sd          s1, 40(s4)
	sd          s3, 32(s4)
	sd          x0, 8(s3)
	lw          t0, 104(s1)
	ori         t0, t0, 2
	sw          t0, 104(s1)
	mv          a0, s3

	// *** Basic block 15

.SpillInstruction_label_142:
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
.func_end_SpillInstruction:
	.size SpillInstruction, .func_end_SpillInstruction-SpillInstruction

	.local  AllocateRegisterWithType
	.type AllocateRegisterWithType, @function

AllocateRegisterWithType:

	// *** Basic block 0

	.local FindFreeRegister
	.local FindSpillVictim
	.local SpillInstruction
	.global printf
	.global abort
	.local IsSavedReg
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
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a3
	mv          a2, a4
	mv          a1, s2
	call        FindFreeRegister

	// *** Basic block 1

	mv          s3, a0
	bne         s3, x0, .AllocateRegisterWithType_label_55

	// *** Basic block 2

	mv          a1, s2
	mv          a0, s1
	call        FindSpillVictim

	// *** Basic block 3

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        SpillInstruction

	// *** Basic block 4

	mv          s3, a0

	// *** Basic block 5

.AllocateRegisterWithType_label_55:
	beq         s3, x0, .AllocateRegisterWithType_label_60

	// *** Basic block 6

	j           .AllocateRegisterWithType_label_75

	// *** Basic block 7

.AllocateRegisterWithType_label_60:
	lla         a0, .str.27
	lla         a1, .str.28
	lla         a3, .str.29
	li          t0, 323		// 0x143
	mv          a2, t0
	call        printf

	// *** Basic block 8

	call        abort

	// *** Basic block 9

.AllocateRegisterWithType_label_75:
	lb          t0, 4(s3)
	beqz        t0, .AllocateRegisterWithType_label_84

	// *** Basic block 10

	mv          a0, s3

	// *** Basic block 11

.AllocateRegisterWithType_label_81:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 12

.AllocateRegisterWithType_label_84:
	mv          a0, s3
	call        IsSavedReg

	// *** Basic block 13

	not         t0, a0
	beqz        t0, .AllocateRegisterWithType_label_93

	// *** Basic block 14

	mv          a0, s3
	j           .AllocateRegisterWithType_label_81

	// *** Basic block 15

.AllocateRegisterWithType_label_93:
	slli        t0, s2, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 16

	j           .AllocateRegisterWithType_label_102

	// *** Basic block 17

	j           .AllocateRegisterWithType_label_109

	// *** Basic block 18

.AllocateRegisterWithType_label_102:
	addi        a0, s1, 1544
	lw          a1, 0(s3)
	call        BitSetInsert

	// *** Basic block 19

	j           .AllocateRegisterWithType_label_116

	// *** Basic block 20

.AllocateRegisterWithType_label_109:
	addi        a0, s1, 1560
	lw          a1, 0(s3)
	call        BitSetInsert

	// *** Basic block 21

	j           .AllocateRegisterWithType_label_116

	// *** Basic block 22

.AllocateRegisterWithType_label_116:
	mv          a0, s3
	j           .AllocateRegisterWithType_label_81
.func_end_AllocateRegisterWithType:
	.size AllocateRegisterWithType, .func_end_AllocateRegisterWithType-AllocateRegisterWithType

	.local  RegisterTypeFromInstruction
	.type RegisterTypeFromInstruction, @function

RegisterTypeFromInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 16(a0)
	li          t1, 164		// 0xa4 ASCII \xa4
	blt         t0, t1, .RegisterTypeFromInstruction_label_107

	// *** Basic block 1

	beq         t0, t1, .RegisterTypeFromInstruction_label_197

	// *** Basic block 2

	li          t1, 165		// 0xa5 ASCII \xa5
	beq         t0, t1, .RegisterTypeFromInstruction_label_198

	// *** Basic block 3

	li          t1, 166		// 0xa6 ASCII \xa6
	beq         t0, t1, .RegisterTypeFromInstruction_label_186

	// *** Basic block 4

	li          t1, 172		// 0xac ASCII \xac
	beq         t0, t1, .RegisterTypeFromInstruction_label_184

	// *** Basic block 5

	li          t1, 173		// 0xad ASCII \xad
	beq         t0, t1, .RegisterTypeFromInstruction_label_183

	// *** Basic block 6

	li          t1, 198		// 0xc6 ASCII \xc6
	beq         t0, t1, .RegisterTypeFromInstruction_label_173

	// *** Basic block 7

	li          t1, 199		// 0xc7 ASCII \xc7
	beq         t0, t1, .RegisterTypeFromInstruction_label_174

	// *** Basic block 8

	li          t1, 200		// 0xc8 ASCII \xc8
	beq         t0, t1, .RegisterTypeFromInstruction_label_175

	// *** Basic block 9

	li          t1, 201		// 0xc9 ASCII \xc9
	beq         t0, t1, .RegisterTypeFromInstruction_label_176

	// *** Basic block 10

	li          t1, 202		// 0xca ASCII \xca
	beq         t0, t1, .RegisterTypeFromInstruction_label_177

	// *** Basic block 11

	li          t1, 203		// 0xcb ASCII \xcb
	beq         t0, t1, .RegisterTypeFromInstruction_label_178

	// *** Basic block 12

	li          t1, 204		// 0xcc ASCII \xcc
	beq         t0, t1, .RegisterTypeFromInstruction_label_179

	// *** Basic block 13

	li          t1, 205		// 0xcd ASCII \xcd
	beq         t0, t1, .RegisterTypeFromInstruction_label_180

	// *** Basic block 14

	j           .RegisterTypeFromInstruction_label_202

	// *** Basic block 15

.RegisterTypeFromInstruction_label_107:
	li          t1, 9		// 0x9 ASCII \x9
	beq         t0, t1, .RegisterTypeFromInstruction_label_169

	// *** Basic block 16

	li          t1, 10		// 0xa ASCII \xa
	beq         t0, t1, .RegisterTypeFromInstruction_label_170

	// *** Basic block 17

	li          t1, 12		// 0xc ASCII \xc
	beq         t0, t1, .RegisterTypeFromInstruction_label_171

	// *** Basic block 18

	li          t1, 13		// 0xd ASCII \xd
	beq         t0, t1, .RegisterTypeFromInstruction_label_172

	// *** Basic block 19

	li          t1, 34		// 0x22 ASCII '"'
	beq         t0, t1, .RegisterTypeFromInstruction_label_181

	// *** Basic block 20

	li          t1, 132		// 0x84 ASCII \x84
	beq         t0, t1, .RegisterTypeFromInstruction_label_182

	// *** Basic block 21

	li          t1, 133		// 0x85 ASCII \x85
	beq         t0, t1, .RegisterTypeFromInstruction_label_194

	// *** Basic block 22

	li          t1, 134		// 0x86 ASCII \x86
	beq         t0, t1, .RegisterTypeFromInstruction_label_195

	// *** Basic block 23

	li          t1, 135		// 0x87 ASCII \x87
	beq         t0, t1, .RegisterTypeFromInstruction_label_185

	// *** Basic block 24

	li          t1, 159		// 0x9f ASCII \x9f
	beq         t0, t1, .RegisterTypeFromInstruction_label_192

	// *** Basic block 25

	li          t1, 160		// 0xa0 ASCII \xa0
	beq         t0, t1, .RegisterTypeFromInstruction_label_193

	// *** Basic block 26

	li          t1, 163		// 0xa3 ASCII \xa3
	beq         t0, t1, .RegisterTypeFromInstruction_label_196

	// *** Basic block 27

	j           .RegisterTypeFromInstruction_label_202

	// *** Basic block 28

.RegisterTypeFromInstruction_label_169:

	// *** Basic block 29

.RegisterTypeFromInstruction_label_170:

	// *** Basic block 30

.RegisterTypeFromInstruction_label_171:

	// *** Basic block 31

.RegisterTypeFromInstruction_label_172:

	// *** Basic block 32

.RegisterTypeFromInstruction_label_173:

	// *** Basic block 33

.RegisterTypeFromInstruction_label_174:

	// *** Basic block 34

.RegisterTypeFromInstruction_label_175:

	// *** Basic block 35

.RegisterTypeFromInstruction_label_176:

	// *** Basic block 36

.RegisterTypeFromInstruction_label_177:

	// *** Basic block 37

.RegisterTypeFromInstruction_label_178:

	// *** Basic block 38

.RegisterTypeFromInstruction_label_179:

	// *** Basic block 39

.RegisterTypeFromInstruction_label_180:

	// *** Basic block 40

.RegisterTypeFromInstruction_label_181:

	// *** Basic block 41

.RegisterTypeFromInstruction_label_182:

	// *** Basic block 42

.RegisterTypeFromInstruction_label_183:

	// *** Basic block 43

.RegisterTypeFromInstruction_label_184:

	// *** Basic block 44

.RegisterTypeFromInstruction_label_185:

	// *** Basic block 45

.RegisterTypeFromInstruction_label_186:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 46

.RegisterTypeFromInstruction_label_189:
	ret         

	// *** Basic block 47

.RegisterTypeFromInstruction_label_192:

	// *** Basic block 48

.RegisterTypeFromInstruction_label_193:

	// *** Basic block 49

.RegisterTypeFromInstruction_label_194:

	// *** Basic block 50

.RegisterTypeFromInstruction_label_195:

	// *** Basic block 51

.RegisterTypeFromInstruction_label_196:

	// *** Basic block 52

.RegisterTypeFromInstruction_label_197:

	// *** Basic block 53

.RegisterTypeFromInstruction_label_198:
	mv          a0, x0
	ret         

	// *** Basic block 54

.RegisterTypeFromInstruction_label_202:
	slti        t2, t0, 107
	not         t1, t2
	li          t2, 107		// 0x6b ASCII 'k'
	blt         t0, t2, .RegisterTypeFromInstruction_label_211

	// *** Basic block 55

	li          t2, 168		// 0xa8 ASCII \xa8
	slt         t0, t2, t0
	not         t1, t0

	// *** Basic block 56

.RegisterTypeFromInstruction_label_211:
	beqz        t1, .RegisterTypeFromInstruction_label_216

	// *** Basic block 57

	li          a0, 1		// 0x1 ASCII \x1
	ret         

	// *** Basic block 58

.RegisterTypeFromInstruction_label_216:
	mv          a0, x0
	ret         
.func_end_RegisterTypeFromInstruction:
	.size RegisterTypeFromInstruction, .func_end_RegisterTypeFromInstruction-RegisterTypeFromInstruction

	.local  CanUseTemp
	.type CanUseTemp, @function

CanUseTemp:

	// *** Basic block 0

	.global BitSetContains
	addi sp, sp, -16
	// Saved return address (offset 8) and frame pointer (offset 0)
	sd ra, 8(sp)
	sd s0, 0(sp)
	addi s0, sp, 16
	// Local vars at offset -16(s0)
	// End of stack frame
	mv          t0, a0
	mv          t1, a1
	addi        a0, t0, 1584
	lw          a1, 20(t1)
	call        BitSetContains

	// *** Basic block 1

	not         a0, a0

	// *** Basic block 2

.CanUseTemp_label_20:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CanUseTemp:
	.size CanUseTemp, .func_end_CanUseTemp-CanUseTemp

	.local  AllocateVariableRegister
	.type AllocateVariableRegister, @function

AllocateVariableRegister:

	// *** Basic block 0

	.local RegisterTypeFromInstruction
	.local AllocateRegisterWithType
	.local CanUseTemp
	.local AssignRegister
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
	mv          s2, a0
	mv          a0, s1
	call        RegisterTypeFromInstruction

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 96(s1)
	mv          a1, s1
	mv          a0, s2
	call        CanUseTemp

	// *** Basic block 2

	mv          a4, a0
	mv          a3, s3
	mv          a2, s1
	mv          a1, s4
	mv          a0, s2
	call        AllocateRegisterWithType

	// *** Basic block 3

	mv          s4, a0
	mv          a1, s1
	mv          a0, s4
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssignRegister
.func_end_AllocateVariableRegister:
	.size AllocateVariableRegister, .func_end_AllocateVariableRegister-AllocateVariableRegister

	.local  AllocateForRmov
	.type AllocateForRmov, @function

AllocateForRmov:

	// *** Basic block 0

	.global printf
	.global abort
	.global RVIsVarRegister
	.local AllocateVariableRegister
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
	lw          s3, 16(s1)
	addi        t1, s3, -18
	seqz        t0, t1
	li          t1, 18		// 0x12 ASCII \x12
	beq         s3, t1, .AllocateForRmov_label_53

	// *** Basic block 1

	addi        t1, s3, -19
	seqz        t0, t1

	// *** Basic block 2

.AllocateForRmov_label_53:
	bnez        t0, .AllocateForRmov_label_58

	// *** Basic block 3

	addi        t1, s3, -20
	seqz        t0, t1

	// *** Basic block 4

.AllocateForRmov_label_58:
	beqz        t0, .AllocateForRmov_label_62

	// *** Basic block 5

	j           .AllocateForRmov_label_79

	// *** Basic block 6

.AllocateForRmov_label_62:
	lla         a0, .str.30
	lla         a1, .str.31
	lla         a3, .str.32
	li          t0, 401		// 0x191
	mv          a2, t0
	call        printf

	// *** Basic block 7

	call        abort

	// *** Basic block 8

.AllocateForRmov_label_79:
	addi        t0, s1, 64
	ld          t0, 8(t0)
	bnez        t0, .AllocateForRmov_label_86

	// *** Basic block 9

	j           .AllocateForRmov_label_101

	// *** Basic block 10

.AllocateForRmov_label_86:
	lla         a0, .str.33
	lla         a1, .str.34
	lla         a3, .str.35
	li          t0, 402		// 0x192
	mv          a2, t0
	call        printf

	// *** Basic block 11

	call        abort

	// *** Basic block 12

.AllocateForRmov_label_101:
	addi        t0, s1, 40
	ld          s3, 40(s1)
	ld          s4, 8(t0)
	mv          a0, s3
	call        RVIsVarRegister

	// *** Basic block 13

	mv          s5, a0
	beqz        a0, .AllocateForRmov_label_118

	// *** Basic block 14

	ld          t0, 32(s3)
	sub         t0, t0, x0
	seqz        s5, t0

	// *** Basic block 15

.AllocateForRmov_label_118:
	beqz        s5, .AllocateForRmov_label_125

	// *** Basic block 16

	mv          a1, s3
	mv          a0, s2
	call        AllocateVariableRegister

	// *** Basic block 17

.AllocateForRmov_label_125:
	ld          s5, 32(s3)
	beq         s5, x0, .AllocateForRmov_label_133

	// *** Basic block 18

	j           .AllocateForRmov_label_148

	// *** Basic block 19

.AllocateForRmov_label_133:
	lla         a0, .str.36
	lla         a1, .str.37
	lla         a3, .str.38
	li          t0, 411		// 0x19b
	mv          a2, t0
	call        printf

	// *** Basic block 20

	call        abort

	// *** Basic block 21

.AllocateForRmov_label_148:
	lw          t0, 16(s4)
	li          t1, 210		// 0xd2 ASCII \xd2
	bne         t0, t1, .AllocateForRmov_label_165

	// *** Basic block 22

	li          t0, 211		// 0xd3 ASCII \xd3
	sw          t0, 16(s1)
	sd          s4, 40(s1)
	addi        t0, s1, 40
	sd          x0, 8(t0)
	j           .AllocateForRmov_label_196

	// *** Basic block 23

.AllocateForRmov_label_165:
	mv          a0, s4
	call        RVIsVarRegister

	// *** Basic block 24

	mv          s3, a0
	beqz        a0, .AllocateForRmov_label_176

	// *** Basic block 25

	ld          t0, 32(s4)
	sub         t0, t0, x0
	seqz        s3, t0

	// *** Basic block 26

.AllocateForRmov_label_176:
	beqz        s3, .AllocateForRmov_label_183

	// *** Basic block 27

	mv          a1, s4
	mv          a0, s2
	call        AllocateVariableRegister

	// *** Basic block 28

.AllocateForRmov_label_183:
	ld          t0, 40(s1)
	lw          t1, 88(t0)
	addi        t1, t1, 1
	sw          t1, 88(t0)
	mv          a1, s1
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
	j           FreeRegisters

	// *** Basic block 29

.AllocateForRmov_label_196:
	sd          s5, 32(s1)
	lw          t0, 104(s1)
	ori         t0, t0, 4
	sw          t0, 104(s1)
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
.func_end_AllocateForRmov:
	.size AllocateForRmov, .func_end_AllocateForRmov-AllocateForRmov

	.local  ReloadSpills
	.type ReloadSpills, @function

ReloadSpills:

	// *** Basic block 0

	.global TargetNewInstruction1
	.global TargetBasicBlockEmitBefore
	.local RegisterTypeFromInstruction
	.local AllocateRegisterWithType
	.local CanUseTemp
	.local AssignRegister
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
	mv          s3, x0
	ld          s4, 0(s2)

	// *** Basic block 1

.ReloadSpills_label_30:
	addi        t0, s1, 40
	slli        s5, s3, 3
	add         t0, t0, s5
	ld          s6, 0(t0)
	sub         t1, s6, x0
	snez        t0, t1
	beq         s6, x0, .ReloadSpills_label_46

	// *** Basic block 2

	lw          t1, 16(s6)
	addi        t1, t1, -210
	seqz        t0, t1

	// *** Basic block 3

.ReloadSpills_label_46:
	beqz        t0, .ReloadSpills_label_102

	// *** Basic block 4

	mv          a1, s6
	li          t0, 211		// 0xd3 ASCII \xd3
	mv          a0, t0
	call        TargetNewInstruction1

	// *** Basic block 5

	mv          s6, a0
	ld          a1, 96(s1)
	mv          a3, s1
	mv          a2, s6
	mv          a0, s4
	call        TargetBasicBlockEmitBefore

	// *** Basic block 6

	addi        t0, s1, 40
	add         t0, t0, s5
	sd          s6, 0(t0)
	mv          a0, s1
	call        RegisterTypeFromInstruction

	// *** Basic block 7

	mv          s4, a0
	ld          s5, 96(s6)
	mv          a1, s6
	mv          a0, s2
	call        CanUseTemp

	// *** Basic block 8

	mv          a4, a0
	mv          a3, s4
	mv          a2, s6
	mv          a1, s5
	mv          a0, s2
	call        AllocateRegisterWithType

	// *** Basic block 9

	mv          s5, a0
	mv          a1, s6
	mv          a0, s5
	call        AssignRegister

	// *** Basic block 10

.ReloadSpills_label_102:

	// *** Basic block 11

.ReloadSpills_label_103:
	addi        s3, s3, 1
	li          t0, 3		// 0x3 ASCII \x3
	bge         s3, t0, .ReloadSpills_label_30

	// *** Basic block 12

.ReloadSpills_label_108:
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
.func_end_ReloadSpills:
	.size ReloadSpills, .func_end_ReloadSpills-ReloadSpills

	.local  AllocateRegister
	.type AllocateRegister, @function

AllocateRegister:

	// *** Basic block 0

	.global compiler
	.local TrapInstruction
	.local AllocateForRmov
	.global RVIsVarRegister
	.local ReloadSpills
	.local AllocateVariableRegister
	.local AllocateRegister
	.global printf
	.global abort
	.local FreeRegisters
	.global BitSetInsert
	.local AllocateRegisterWithType
	.local CanUseTemp
	.local RegisterTypeFromInstruction
	.local AssignRegister
	.local FreeRegister
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
	mv          s2, a1
	ld          s4, 0(s1)
	lw          t0, 44(s4)
	seqz        s3, t0
	bnez        t0, .AllocateRegister_label_110

	// *** Basic block 1

	la          t0, compiler
	ld          t0, 0(t0)
	lb          s3, 1229(t0)

	// *** Basic block 2

.AllocateRegister_label_110:
	lw          s5, 16(s2)
	mv          a0, s2
	call        TrapInstruction

	// *** Basic block 3

	ld          t0, 32(s2)
	beq         t0, x0, .AllocateRegister_label_126

	// *** Basic block 4

.AllocateRegister_label_123:
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

	// *** Basic block 5

.AllocateRegister_label_126:
	addi        t1, s5, -18
	seqz        t0, t1
	li          t1, 18		// 0x12 ASCII \x12
	beq         s5, t1, .AllocateRegister_label_137

	// *** Basic block 6

	addi        t1, s5, -19
	seqz        t0, t1

	// *** Basic block 7

.AllocateRegister_label_137:
	bnez        t0, .AllocateRegister_label_142

	// *** Basic block 8

	addi        t1, s5, -20
	seqz        t0, t1

	// *** Basic block 9

.AllocateRegister_label_142:
	beqz        t0, .AllocateRegister_label_150

	// *** Basic block 10

	mv          a1, s2
	mv          a0, s1
	call        AllocateForRmov

	// *** Basic block 11

	j           .AllocateRegister_label_123

	// *** Basic block 12

.AllocateRegister_label_150:
	mv          a0, s2
	call        RVIsVarRegister

	// *** Basic block 13

	beqz        a0, .AllocateRegister_label_156

	// *** Basic block 14

	j           .AllocateRegister_label_123

	// *** Basic block 15

.AllocateRegister_label_156:
	mv          a1, s2
	mv          a0, s1
	call        ReloadSpills

	// *** Basic block 16

	ld          s6, 24(s2)
	beq         s6, x0, .AllocateRegister_label_225

	// *** Basic block 17

	ld          s7, 32(s6)
	bne         s7, x0, .AllocateRegister_label_189

	// *** Basic block 18

	mv          a0, s6
	call        RVIsVarRegister

	// *** Basic block 19

	beqz        a0, .AllocateRegister_label_182

	// *** Basic block 20

	mv          a1, s6
	mv          a0, s1
	call        AllocateVariableRegister

	// *** Basic block 21

	j           .AllocateRegister_label_188

	// *** Basic block 22

.AllocateRegister_label_182:
	mv          a1, s6
	mv          a0, s1
	call        AllocateRegister

	// *** Basic block 23

.AllocateRegister_label_188:

	// *** Basic block 24

.AllocateRegister_label_189:
	beq         s7, x0, .AllocateRegister_label_194

	// *** Basic block 25

	j           .AllocateRegister_label_211

	// *** Basic block 26

.AllocateRegister_label_194:
	lla         a0, .str.39
	lla         a1, .str.40
	lla         a3, .str.41
	li          t0, 497		// 0x1f1
	mv          a2, t0
	call        printf

	// *** Basic block 27

	call        abort

	// *** Basic block 28

.AllocateRegister_label_211:
	sd          s7, 32(s2)
	mv          a1, s2
	mv          a0, s1
	call        FreeRegisters

	// *** Basic block 29

	lw          t0, 104(s2)
	ori         t0, t0, 4
	sw          t0, 104(s2)
	j           .AllocateRegister_label_123

	// *** Basic block 30

.AllocateRegister_label_225:
	mv          a1, s2
	mv          a0, s1
	call        FreeRegisters

	// *** Basic block 31

	li          s6, 127		// 0x7f ASCII \x7f
	blt         s5, s6, .AllocateRegister_label_385

	// *** Basic block 32

	li          t0, 194		// 0xc2 ASCII \xc2
	blt         s5, t0, .AllocateRegister_label_313

	// *** Basic block 33

	beq         s5, t0, .AllocateRegister_label_577

	// *** Basic block 34

	li          t0, 195		// 0xc3 ASCII \xc3
	beq         s5, t0, .AllocateRegister_label_578

	// *** Basic block 35

	li          t0, 196		// 0xc4 ASCII \xc4
	beq         s5, t0, .AllocateRegister_label_579

	// *** Basic block 36

	li          t0, 197		// 0xc5 ASCII \xc5
	beq         s5, t0, .AllocateRegister_label_580

	// *** Basic block 37

	li          t0, 198		// 0xc6 ASCII \xc6
	beq         s5, t0, .AllocateRegister_label_606

	// *** Basic block 38

	li          t0, 199		// 0xc7 ASCII \xc7
	beq         s5, t0, .AllocateRegister_label_607

	// *** Basic block 39

	li          t0, 200		// 0xc8 ASCII \xc8
	beq         s5, t0, .AllocateRegister_label_608

	// *** Basic block 40

	li          t0, 201		// 0xc9 ASCII \xc9
	beq         s5, t0, .AllocateRegister_label_609

	// *** Basic block 41

	li          t0, 202		// 0xca ASCII \xca
	beq         s5, t0, .AllocateRegister_label_610

	// *** Basic block 42

	li          t0, 203		// 0xcb ASCII \xcb
	beq         s5, t0, .AllocateRegister_label_611

	// *** Basic block 43

	li          t0, 204		// 0xcc ASCII \xcc
	beq         s5, t0, .AllocateRegister_label_612

	// *** Basic block 44

	li          t0, 205		// 0xcd ASCII \xcd
	beq         s5, t0, .AllocateRegister_label_613

	// *** Basic block 45

	li          t0, 207		// 0xcf ASCII \xcf
	beq         s5, t0, .AllocateRegister_label_558

	// *** Basic block 46

	li          t0, 208		// 0xd0 ASCII \xd0
	beq         s5, t0, .AllocateRegister_label_569

	// *** Basic block 47

	li          t0, 209		// 0xd1 ASCII \xd1
	beq         s5, t0, .AllocateRegister_label_556

	// *** Basic block 48

	j           .AllocateRegister_label_691

	// *** Basic block 49

.AllocateRegister_label_313:
	beq         s5, s6, .AllocateRegister_label_665

	// *** Basic block 50

	li          t0, 128		// 0x80 ASCII \x80
	beq         s5, t0, .AllocateRegister_label_666

	// *** Basic block 51

	li          t0, 155		// 0x9b ASCII \x9b
	beq         s5, t0, .AllocateRegister_label_667

	// *** Basic block 52

	li          t0, 156		// 0x9c ASCII \x9c
	beq         s5, t0, .AllocateRegister_label_668

	// *** Basic block 53

	li          t0, 157		// 0x9d ASCII \x9d
	beq         s5, t0, .AllocateRegister_label_669

	// *** Basic block 54

	li          t0, 181		// 0xb5 ASCII \xb5
	beq         s5, t0, .AllocateRegister_label_547

	// *** Basic block 55

	li          t0, 183		// 0xb7 ASCII \xb7
	beq         s5, t0, .AllocateRegister_label_654

	// *** Basic block 56

	li          t0, 184		// 0xb8 ASCII \xb8
	beq         s5, t0, .AllocateRegister_label_655

	// *** Basic block 57

	li          t0, 185		// 0xb9 ASCII \xb9
	beq         s5, t0, .AllocateRegister_label_659

	// *** Basic block 58

	li          t0, 186		// 0xba ASCII \xba
	beq         s5, t0, .AllocateRegister_label_660

	// *** Basic block 59

	li          t0, 190		// 0xbe ASCII \xbe
	beq         s5, t0, .AllocateRegister_label_573

	// *** Basic block 60

	li          t0, 191		// 0xbf ASCII \xbf
	beq         s5, t0, .AllocateRegister_label_574

	// *** Basic block 61

	li          t0, 192		// 0xc0 ASCII \xc0
	beq         s5, t0, .AllocateRegister_label_575

	// *** Basic block 62

	li          t0, 193		// 0xc1 ASCII \xc1
	beq         s5, t0, .AllocateRegister_label_576

	// *** Basic block 63

	j           .AllocateRegister_label_691

	// *** Basic block 64

.AllocateRegister_label_385:
	li          t0, 26		// 0x1a ASCII \x1a
	blt         s5, t0, .AllocateRegister_label_465

	// *** Basic block 65

	beq         s5, t0, .AllocateRegister_label_645

	// *** Basic block 66

	li          t0, 27		// 0x1b ASCII \x1b
	beq         s5, t0, .AllocateRegister_label_649

	// *** Basic block 67

	li          t0, 28		// 0x1c ASCII \x1c
	beq         s5, t0, .AllocateRegister_label_650

	// *** Basic block 68

	li          t0, 29		// 0x1d ASCII \x1d
	beq         s5, t0, .AllocateRegister_label_622

	// *** Basic block 69

	li          t0, 30		// 0x1e ASCII \x1e
	beq         s5, t0, .AllocateRegister_label_553

	// *** Basic block 70

	li          t0, 31		// 0x1f ASCII \x1f
	beq         s5, t0, .AllocateRegister_label_554

	// *** Basic block 71

	li          t0, 33		// 0x21 ASCII '!'
	beq         s5, t0, .AllocateRegister_label_589

	// *** Basic block 72

	li          t0, 34		// 0x22 ASCII '"'
	beq         s5, t0, .AllocateRegister_label_590

	// *** Basic block 73

	li          t0, 39		// 0x27 ASCII '''
	beq         s5, t0, .AllocateRegister_label_541

	// *** Basic block 74

	li          t0, 40		// 0x28 ASCII '('
	beq         s5, t0, .AllocateRegister_label_542

	// *** Basic block 75

	li          t0, 41		// 0x29 ASCII ')'
	beq         s5, t0, .AllocateRegister_label_543

	// *** Basic block 76

	li          t0, 42		// 0x2a ASCII '*'
	beq         s5, t0, .AllocateRegister_label_545

	// *** Basic block 77

	li          t0, 43		// 0x2b ASCII '+'
	beq         s5, t0, .AllocateRegister_label_544

	// *** Basic block 78

	li          t0, 44		// 0x2c ASCII ','
	beq         s5, t0, .AllocateRegister_label_546

	// *** Basic block 79

	li          t0, 126		// 0x7e ASCII '~'
	beq         s5, t0, .AllocateRegister_label_664

	// *** Basic block 80

	j           .AllocateRegister_label_691

	// *** Basic block 81

.AllocateRegister_label_465:
	beqz        s5, .AllocateRegister_label_550

	// *** Basic block 82

	li          t0, 1		// 0x1 ASCII \x1
	beq         s5, t0, .AllocateRegister_label_551

	// *** Basic block 83

	li          t0, 2		// 0x2 ASCII \x2
	beq         s5, t0, .AllocateRegister_label_540

	// *** Basic block 84

	li          t0, 3		// 0x3 ASCII \x3
	beq         s5, t0, .AllocateRegister_label_552

	// *** Basic block 85

	li          t0, 5		// 0x5 ASCII \x5
	beq         s5, t0, .AllocateRegister_label_534

	// *** Basic block 86

	li          t0, 6		// 0x6 ASCII \x6
	beq         s5, t0, .AllocateRegister_label_535

	// *** Basic block 87

	li          t0, 7		// 0x7 ASCII \x7
	beq         s5, t0, .AllocateRegister_label_536

	// *** Basic block 88

	li          t0, 8		// 0x8 ASCII \x8
	beq         s5, t0, .AllocateRegister_label_537

	// *** Basic block 89

	li          t0, 9		// 0x9 ASCII \x9
	beq         s5, t0, .AllocateRegister_label_538

	// *** Basic block 90

	li          t0, 10		// 0xa ASCII \xa
	beq         s5, t0, .AllocateRegister_label_539

	// *** Basic block 91

	li          t0, 21		// 0x15 ASCII \x15
	beq         s5, t0, .AllocateRegister_label_549

	// *** Basic block 92

	li          t0, 22		// 0x16 ASCII \x16
	beq         s5, t0, .AllocateRegister_label_548

	// *** Basic block 93

	li          t0, 23		// 0x17 ASCII \x17
	beq         s5, t0, .AllocateRegister_label_561

	// *** Basic block 94

	li          t0, 24		// 0x18 ASCII \x18
	beq         s5, t0, .AllocateRegister_label_565

	// *** Basic block 95

	j           .AllocateRegister_label_691

	// *** Basic block 96

.AllocateRegister_label_534:

	// *** Basic block 97

.AllocateRegister_label_535:

	// *** Basic block 98

.AllocateRegister_label_536:

	// *** Basic block 99

.AllocateRegister_label_537:

	// *** Basic block 100

.AllocateRegister_label_538:

	// *** Basic block 101

.AllocateRegister_label_539:

	// *** Basic block 102

.AllocateRegister_label_540:

	// *** Basic block 103

.AllocateRegister_label_541:

	// *** Basic block 104

.AllocateRegister_label_542:

	// *** Basic block 105

.AllocateRegister_label_543:

	// *** Basic block 106

.AllocateRegister_label_544:

	// *** Basic block 107

.AllocateRegister_label_545:

	// *** Basic block 108

.AllocateRegister_label_546:

	// *** Basic block 109

.AllocateRegister_label_547:

	// *** Basic block 110

.AllocateRegister_label_548:

	// *** Basic block 111

.AllocateRegister_label_549:

	// *** Basic block 112

.AllocateRegister_label_550:

	// *** Basic block 113

.AllocateRegister_label_551:

	// *** Basic block 114

.AllocateRegister_label_552:

	// *** Basic block 115

.AllocateRegister_label_553:

	// *** Basic block 116

.AllocateRegister_label_554:
	j           .AllocateRegister_label_123

	// *** Basic block 117

.AllocateRegister_label_556:
	j           .AllocateRegister_label_123

	// *** Basic block 118

.AllocateRegister_label_558:
	addi        s7, s1, 8
	j           .AllocateRegister_label_716

	// *** Basic block 119

.AllocateRegister_label_561:
	addi        t0, s1, 8
	addi        s7, t0, 192
	j           .AllocateRegister_label_716

	// *** Basic block 120

.AllocateRegister_label_565:
	addi        t0, s1, 8
	addi        s7, t0, 48
	j           .AllocateRegister_label_716

	// *** Basic block 121

.AllocateRegister_label_569:
	addi        t0, s1, 8
	addi        s7, t0, 120
	j           .AllocateRegister_label_716

	// *** Basic block 122

.AllocateRegister_label_573:

	// *** Basic block 123

.AllocateRegister_label_574:

	// *** Basic block 124

.AllocateRegister_label_575:

	// *** Basic block 125

.AllocateRegister_label_576:

	// *** Basic block 126

.AllocateRegister_label_577:

	// *** Basic block 127

.AllocateRegister_label_578:

	// *** Basic block 128

.AllocateRegister_label_579:

	// *** Basic block 129

.AllocateRegister_label_580:
	addi        t0, s1, 8
	addi        t1, s5, -190
	addi        t1, t1, 10
	slli        t2, t1, 3
	slli        t1, t1, 4
	add         t1, t2, t1
	add         s7, t0, t1
	j           .AllocateRegister_label_716

	// *** Basic block 130

.AllocateRegister_label_589:

	// *** Basic block 131

.AllocateRegister_label_590:
	lla         a0, .str.42
	lla         a1, .str.43
	lla         a3, .str.44
	li          t0, 567		// 0x237
	mv          a2, t0
	call        printf

	// *** Basic block 132

	call        abort

	// *** Basic block 133

	j           .AllocateRegister_label_716

	// *** Basic block 134

.AllocateRegister_label_606:

	// *** Basic block 135

.AllocateRegister_label_607:

	// *** Basic block 136

.AllocateRegister_label_608:

	// *** Basic block 137

.AllocateRegister_label_609:

	// *** Basic block 138

.AllocateRegister_label_610:

	// *** Basic block 139

.AllocateRegister_label_611:

	// *** Basic block 140

.AllocateRegister_label_612:

	// *** Basic block 141

.AllocateRegister_label_613:
	addi        t0, s1, 776
	addi        t1, s5, -198
	addi        t1, t1, 10
	slli        t2, t1, 3
	slli        t1, t1, 4
	add         t1, t2, t1
	add         s7, t0, t1
	j           .AllocateRegister_label_716

	// *** Basic block 142

.AllocateRegister_label_622:
	addi        s6, s1, 8
	beqz        s3, .AllocateRegister_label_629

	// *** Basic block 143

	li          s8, 28		// 0x1c ASCII \x1c
	j           .AllocateRegister_label_631

	// *** Basic block 144

.AllocateRegister_label_629:
	li          s8, 18		// 0x12 ASCII \x12

	// *** Basic block 145

.AllocateRegister_label_631:
	lw          t0, 200(s4)
	add         t0, s8, t0
	slli        t1, t0, 3
	slli        t0, t0, 4
	add         t0, t1, t0
	add         s7, s6, t0
	addi        a0, s1, 1544
	lw          a1, 0(s7)
	call        BitSetInsert

	// *** Basic block 146

	j           .AllocateRegister_label_716

	// *** Basic block 147

.AllocateRegister_label_645:
	addi        t0, s1, 8
	addi        s7, t0, 240
	j           .AllocateRegister_label_716

	// *** Basic block 148

.AllocateRegister_label_649:

	// *** Basic block 149

.AllocateRegister_label_650:
	addi        t0, s1, 776
	addi        s7, t0, 240
	j           .AllocateRegister_label_716

	// *** Basic block 150

.AllocateRegister_label_654:

	// *** Basic block 151

.AllocateRegister_label_655:
	addi        t0, s1, 8
	addi        s7, t0, 240
	j           .AllocateRegister_label_716

	// *** Basic block 152

.AllocateRegister_label_659:

	// *** Basic block 153

.AllocateRegister_label_660:
	addi        t0, s1, 776
	addi        s7, t0, 240
	j           .AllocateRegister_label_716

	// *** Basic block 154

.AllocateRegister_label_664:

	// *** Basic block 155

.AllocateRegister_label_665:

	// *** Basic block 156

.AllocateRegister_label_666:

	// *** Basic block 157

.AllocateRegister_label_667:

	// *** Basic block 158

.AllocateRegister_label_668:

	// *** Basic block 159

.AllocateRegister_label_669:
	ld          s3, 96(s2)
	mv          a1, s2
	mv          a0, s1
	call        CanUseTemp

	// *** Basic block 160

	mv          a4, a0
	mv          a3, x0
	mv          a2, s2
	mv          a1, s3
	mv          a0, s1
	call        AllocateRegisterWithType

	// *** Basic block 161

	mv          s7, a0
	j           .AllocateRegister_label_716

	// *** Basic block 162

.AllocateRegister_label_691:
	mv          a0, s2
	call        RegisterTypeFromInstruction

	// *** Basic block 163

	mv          s3, a0
	ld          s4, 96(s2)
	mv          a1, s2
	mv          a0, s1
	call        CanUseTemp

	// *** Basic block 164

	mv          a4, a0
	mv          a3, s3
	mv          a2, s2
	mv          a1, s4
	mv          a0, s1
	call        AllocateRegisterWithType

	// *** Basic block 165

	mv          s7, a0

	// *** Basic block 166

.AllocateRegister_label_716:
	mv          a1, s2
	mv          a0, s7
	call        AssignRegister

	// *** Basic block 167

	lw          t1, 88(s2)
	seqz        t0, t1
	bnez        t1, .AllocateRegister_label_730

	// *** Basic block 168

	lb          t1, 4(s7)
	not         t0, t1

	// *** Basic block 169

.AllocateRegister_label_730:
	beqz        t0, .AllocateRegister_label_737

	// *** Basic block 170

	mv          a1, s7
	mv          a0, s1
	call        FreeRegister

	// *** Basic block 171

.AllocateRegister_label_737:
	j           .AllocateRegister_label_123
.func_end_AllocateRegister:
	.size AllocateRegister, .func_end_AllocateRegister-AllocateRegister

	.local  InitializeBasicBlockRegisters
	.type InitializeBasicBlockRegisters, @function

InitializeBasicBlockRegisters:

	// *** Basic block 0

	.global RVIsVarRegister
	.global printf
	.global abort
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
	mv          t0, a0
	mv          t1, a1
	mv          t2, x0

	// *** Basic block 1

.InitializeBasicBlockRegisters_label_34:
	addi        t3, t0, 8
	slli        t4, t2, 3
	slli        t5, t2, 4
	add         t4, t4, t5
	add         s1, t3, t4
	lb          t3, 4(s1)
	bnez        t3, .InitializeBasicBlockRegisters_label_48

	// *** Basic block 2

.InitializeBasicBlockRegisters_label_45:
	sd          x0, 8(s1)

	// *** Basic block 3

.InitializeBasicBlockRegisters_label_48:
	addi        t2, t2, 1
	li          t3, 32		// 0x20 ASCII ' '
	bge         t2, t3, .InitializeBasicBlockRegisters_label_34

	// *** Basic block 4

.InitializeBasicBlockRegisters_label_53:
	mv          t2, x0

	// *** Basic block 5

.InitializeBasicBlockRegisters_label_57:
	addi        t4, t0, 776
	slli        t5, t2, 3
	slli        t6, t2, 4
	add         t5, t5, t6
	add         s1, t4, t5
	lb          t4, 4(s1)
	bnez        t4, .InitializeBasicBlockRegisters_label_70

	// *** Basic block 6

.InitializeBasicBlockRegisters_label_67:
	sd          x0, 8(s1)

	// *** Basic block 7

.InitializeBasicBlockRegisters_label_70:
	addi        t2, t2, 1
	bge         t2, t3, .InitializeBasicBlockRegisters_label_57

	// *** Basic block 8

.InitializeBasicBlockRegisters_label_75:
	mv          s1, x0
	addi        t0, t1, 144
	ld          s2, 8(t0)
	bge         x0, s2, .InitializeBasicBlockRegisters_label_154

	// *** Basic block 9

	ld          s3, 144(t1)

	// *** Basic block 10

.InitializeBasicBlockRegisters_label_84:
	slli        t0, s1, 3
	add         t0, s3, t0
	ld          s3, 0(t0)
	mv          a0, s3
	call        RVIsVarRegister

	// *** Basic block 11

	mv          s4, a0
	beqz        a0, .InitializeBasicBlockRegisters_label_99

	// *** Basic block 12

	ld          t0, 32(s3)
	sub         t0, t0, x0
	seqz        s4, t0

	// *** Basic block 13

.InitializeBasicBlockRegisters_label_99:
	bnez        s4, .InitializeBasicBlockRegisters_label_150

	// *** Basic block 14

.InitializeBasicBlockRegisters_label_101:
	lw          t1, 16(s3)
	addi        t2, t1, -210
	seqz        t0, t2
	li          t2, 210		// 0xd2 ASCII \xd2
	beq         t1, t2, .InitializeBasicBlockRegisters_label_114

	// *** Basic block 15

	lw          t1, 104(s3)
	andi        t1, t1, 2
	snez        t0, t1

	// *** Basic block 16

.InitializeBasicBlockRegisters_label_114:
	bnez        t0, .InitializeBasicBlockRegisters_label_150

	// *** Basic block 17

.InitializeBasicBlockRegisters_label_116:
	lw          t0, 88(s3)
	beqz        t0, .InitializeBasicBlockRegisters_label_150

	// *** Basic block 18

.InitializeBasicBlockRegisters_label_121:
	ld          t0, 32(s3)
	beq         t0, x0, .InitializeBasicBlockRegisters_label_128

	// *** Basic block 19

	j           .InitializeBasicBlockRegisters_label_145

	// *** Basic block 20

.InitializeBasicBlockRegisters_label_128:
	lla         a0, .str.45
	lla         a1, .str.46
	lla         a3, .str.47
	li          t0, 663		// 0x297
	mv          a2, t0
	call        printf

	// *** Basic block 21

	call        abort

	// *** Basic block 22

.InitializeBasicBlockRegisters_label_145:
	ld          t0, 32(s3)
	sd          s3, 8(t0)

	// *** Basic block 23

.InitializeBasicBlockRegisters_label_150:
	addi        s1, s1, 1
	bge         s1, s2, .InitializeBasicBlockRegisters_label_84

	// *** Basic block 24

.InitializeBasicBlockRegisters_label_154:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InitializeBasicBlockRegisters:
	.size InitializeBasicBlockRegisters, .func_end_InitializeBasicBlockRegisters-InitializeBasicBlockRegisters

	.local  ProcessBlock
	.type ProcessBlock, @function

ProcessBlock:

	// *** Basic block 0

	.local TrapBlock
	.local InitializeBasicBlockRegisters
	.local AllocateRegister
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
	call        TrapBlock

	// *** Basic block 1

	mv          s3, s2
	mv          a1, s1
	mv          a0, s3
	call        InitializeBasicBlockRegisters

	// *** Basic block 2

	ld          s4, 136(s1)
	beq         s4, x0, .ProcessBlock_label_44

	// *** Basic block 3

	lw          t0, 208(s4)
	sw          t0, 208(s1)
	lw          t0, 208(s1)
	slli        t0, t0, 3
	sw          t0, 1576(s3)

	// *** Basic block 4

.ProcessBlock_label_44:
	ld          s2, 56(s1)
	sub         t1, s2, x0
	snez        t0, t1
	beq         s2, x0, .ProcessBlock_label_58

	// *** Basic block 5

	ld          t1, 64(s1)
	ld          t2, 64(s1)
	sub         t2, s2, t2
	snez        t0, t2

	// *** Basic block 6

.ProcessBlock_label_58:
	beqz        t0, .ProcessBlock_label_79

	// *** Basic block 7

.ProcessBlock_label_60:
	mv          a1, s2
	mv          a0, s3
	call        AllocateRegister

	// *** Basic block 8

.ProcessBlock_label_66:
	mv          a0, s2
	call        TargetNext

	// *** Basic block 9

	mv          s2, a0
	sub         t1, s2, x0
	snez        t0, t1
	beq         s2, x0, .ProcessBlock_label_77

	// *** Basic block 10

	sub         t1, s2, t1
	snez        t0, t1

	// *** Basic block 11

.ProcessBlock_label_77:
	beqz        t0, .ProcessBlock_label_60

	// *** Basic block 12

.ProcessBlock_label_79:
	ld          t0, 64(s1)
	beq         t0, x0, .ProcessBlock_label_91

	// *** Basic block 13

	mv          a1, t0
	mv          a0, s3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AllocateRegister

	// *** Basic block 14

.ProcessBlock_label_91:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ProcessBlock:
	.size ProcessBlock, .func_end_ProcessBlock-ProcessBlock

	.local  ProcessBasicBlock
	.type ProcessBasicBlock, @function

ProcessBasicBlock:

	// *** Basic block 0

	.global TargetTraverseDominatorTree
	.local ProcessBlock
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 0(t0)
	mv          a3, t0
	mv          a2, x0
	la          a1, ProcessBlock
	j           TargetTraverseDominatorTree
.func_end_ProcessBasicBlock:
	.size ProcessBasicBlock, .func_end_ProcessBasicBlock-ProcessBasicBlock

	.local  BuildPreservedInstructionsSet
	.type BuildPreservedInstructionsSet, @function

BuildPreservedInstructionsSet:

	// *** Basic block 0

	.global BitSetUnionInPlace
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
	mv          s2, a1
	lb          t0, 216(s1)
	not         t0, t0
	beqz        t0, .BuildPreservedInstructionsSet_label_22

	// *** Basic block 1

.BuildPreservedInstructionsSet_label_19:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.BuildPreservedInstructionsSet_label_22:
	addi        a0, s2, 1584
	addi        a1, s1, 192
	call        BitSetUnionInPlace

	// *** Basic block 3

	j           .BuildPreservedInstructionsSet_label_19
.func_end_BuildPreservedInstructionsSet:
	.size BuildPreservedInstructionsSet, .func_end_BuildPreservedInstructionsSet-BuildPreservedInstructionsSet

	.global RVAllocateRegisters
	.type RVAllocateRegisters, @function

RVAllocateRegisters:

	// *** Basic block 0

	.global TargetTraverseDominatorTree
	.local BuildPreservedInstructionsSet
	.local ProcessBasicBlock
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
	ld          s2, 0(s1)
	mv          a3, s1
	mv          a2, x0
	la          a1, BuildPreservedInstructionsSet
	mv          a0, s2
	call        TargetTraverseDominatorTree

	// *** Basic block 1

	ld          a1, 448(s2)
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ProcessBasicBlock
.func_end_RVAllocateRegisters:
	.size RVAllocateRegisters, .func_end_RVAllocateRegisters-RVAllocateRegisters

	.global RVRegisterName
	.type RVRegisterName, @function

RVRegisterName:

	// *** Basic block 0

	.global RVRegisterNameFromNum
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, a2
	lw          a0, 0(t0)
	lw          a1, 16(t0)
	mv          a3, t2
	mv          a2, t1
	j           RVRegisterNameFromNum
.func_end_RVRegisterName:
	.size RVRegisterName, .func_end_RVRegisterName-RVRegisterName

	.global RVRegisterNameFromNum
	.type RVRegisterNameFromNum, @function

RVRegisterNameFromNum:

	// *** Basic block 0

	.local register_ranges
	.global snprintf
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
	mv          s1, a1
	mv          s2, a0
	mv          s3, a2
	mv          s4, a3
	mv          s5, x0

	// *** Basic block 1

.RVRegisterNameFromNum_label_37:
	slli        t1, s5, 5
	la          t2, register_ranges
	add         s6, t2, t1
	lw          t1, 0(s6)
	sub         t2, t1, s1
	seqz        t0, t2
	bne         t1, s1, .RVRegisterNameFromNum_label_52

	// *** Basic block 2

	lw          t1, 4(s6)
	addi        t1, t1, -1
	snez        t0, t1

	// *** Basic block 3

.RVRegisterNameFromNum_label_52:
	beqz        t0, .RVRegisterNameFromNum_label_89

	// *** Basic block 4

	lw          s7, 4(s6)
	slt         t1, s2, s7
	not         t0, t1
	blt         s2, s7, .RVRegisterNameFromNum_label_64

	// *** Basic block 5

	lw          t1, 8(s6)
	slt         t1, t1, s2
	not         t0, t1

	// *** Basic block 6

.RVRegisterNameFromNum_label_64:
	beqz        t0, .RVRegisterNameFromNum_label_88

	// *** Basic block 7

	lla         a2, .str.48
	ld          a3, 16(s6)
	sub         t0, s2, s7
	lw          t1, 24(s6)
	add         a4, t0, t1
	mv          a1, s4
	mv          a0, s3
	call        snprintf

	// *** Basic block 8

	mv          a0, s3

	// *** Basic block 9

.RVRegisterNameFromNum_label_85:
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

	// *** Basic block 10

.RVRegisterNameFromNum_label_88:

	// *** Basic block 11

.RVRegisterNameFromNum_label_89:

	// *** Basic block 12

.RVRegisterNameFromNum_label_90:
	addi        s5, s5, 1
	li          t0, 11		// 0xb ASCII \xb
	bge         s5, t0, .RVRegisterNameFromNum_label_37

	// *** Basic block 13

.RVRegisterNameFromNum_label_96:
	slli        t0, s1, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 14

	j           .RVRegisterNameFromNum_label_104

	// *** Basic block 15

	j           .RVRegisterNameFromNum_label_160

	// *** Basic block 16

.RVRegisterNameFromNum_label_104:
	li          t0, 2		// 0x2 ASCII \x2
	bne         s2, t0, .RVRegisterNameFromNum_label_119

	// *** Basic block 17

	lla         a2, .str.49
	mv          a1, s4
	mv          a0, s3
	call        snprintf

	// *** Basic block 18

	j           .RVRegisterNameFromNum_label_172

	// *** Basic block 19

.RVRegisterNameFromNum_label_119:
	li          t0, 8		// 0x8 ASCII \x8
	bne         s2, t0, .RVRegisterNameFromNum_label_134

	// *** Basic block 20

	lla         a2, .str.50
	mv          a1, s4
	mv          a0, s3
	call        snprintf

	// *** Basic block 21

	j           .RVRegisterNameFromNum_label_172

	// *** Basic block 22

.RVRegisterNameFromNum_label_134:
	li          t0, 1		// 0x1 ASCII \x1
	bne         s2, t0, .RVRegisterNameFromNum_label_148

	// *** Basic block 23

	lla         a2, .str.51
	mv          a1, s4
	mv          a0, s3
	call        snprintf

	// *** Basic block 24

	j           .RVRegisterNameFromNum_label_172

	// *** Basic block 25

.RVRegisterNameFromNum_label_148:
	lla         a2, .str.52
	mv          a3, s2
	mv          a1, s4
	mv          a0, s3
	call        snprintf

	// *** Basic block 26

	j           .RVRegisterNameFromNum_label_172

	// *** Basic block 27

.RVRegisterNameFromNum_label_160:
	lla         a2, .str.53
	mv          a3, s2
	mv          a1, s4
	mv          a0, s3
	call        snprintf

	// *** Basic block 28

	j           .RVRegisterNameFromNum_label_172

	// *** Basic block 29

.RVRegisterNameFromNum_label_172:
	mv          a0, s3
	j           .RVRegisterNameFromNum_label_85
.func_end_RVRegisterNameFromNum:
	.size RVRegisterNameFromNum, .func_end_RVRegisterNameFromNum-RVRegisterNameFromNum

.PCend:
	.data
register_ranges:
	.type   register_ranges,@object
	.local  register_ranges
	.size   register_ranges,352
	.p2align  3
	.word   0
	.word   5
	.word   7
	.space  4
	.long    .str.1
	.word   0
	.byte   1
	.space  3
	.word   0
	.word   28
	.word   31
	.space  4
	.long    .str.2
	.word   3
	.byte   1
	.space  3
	.word   0
	.word   10
	.word   17
	.space  4
	.long    .str.3
	.word   0
	.byte   1
	.space  3
	.word   0
	.word   1
	.word   1
	.space  4
	.long    .str.4
	.word   0
	.byte   1
	.space  3
	.word   0
	.word   8
	.word   9
	.space  4
	.long    .str.5
	.word   0
	.byte   0
	.space  3
	.word   0
	.word   18
	.word   27
	.space  4
	.long    .str.6
	.word   2
	.byte   0
	.space  3
	.word   1
	.word   0
	.word   7
	.space  4
	.long    .str.7
	.word   0
	.byte   1
	.space  3
	.word   1
	.word   28
	.word   31
	.space  4
	.long    .str.8
	.word   2
	.byte   1
	.space  3
	.word   1
	.word   10
	.word   17
	.space  4
	.long    .str.9
	.word   0
	.byte   1
	.space  3
	.word   1
	.word   8
	.word   9
	.space  4
	.long    .str.10
	.word   0
	.byte   0
	.space  3
	.word   1
	.word   18
	.word   27
	.space  4
	.long    .str.11
	.word   2
	.byte   0
	.space  3

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "t"
	.type .str.1, @object
	.size .str.1, 2

.str.2:
	.asciz "t"
	.type .str.2, @object
	.size .str.2, 2

.str.3:
	.asciz "a"
	.type .str.3, @object
	.size .str.3, 2

.str.4:
	.asciz "ra"
	.type .str.4, @object
	.size .str.4, 3

.str.5:
	.asciz "s"
	.type .str.5, @object
	.size .str.5, 2

.str.6:
	.asciz "s"
	.type .str.6, @object
	.size .str.6, 2

.str.7:
	.asciz "ft"
	.type .str.7, @object
	.size .str.7, 3

.str.8:
	.asciz "ft"
	.type .str.8, @object
	.size .str.8, 3

.str.9:
	.asciz "fa"
	.type .str.9, @object
	.size .str.9, 3

.str.10:
	.asciz "fs"
	.type .str.10, @object
	.size .str.10, 3

.str.11:
	.asciz "fs"
	.type .str.11, @object
	.size .str.11, 3

.str.12:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.12, @object
	.size .str.12, 30

.str.13:
	.asciz "(null)"
	.type .str.13, @object
	.size .str.13, 1

.str.14:
	.asciz "inst->reg == NULL"
	.type .str.14, @object
	.size .str.14, 18

.str.15:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.15, @object
	.size .str.15, 30

.str.16:
	.asciz "(null)"
	.type .str.16, @object
	.size .str.16, 1

.str.17:
	.asciz "op->uses >= 0"
	.type .str.17, @object
	.size .str.17, 14

.str.18:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.18, @object
	.size .str.18, 30

.str.19:
	.asciz "(null)"
	.type .str.19, @object
	.size .str.19, 1

.str.20:
	.asciz "(owner->flags & TARGET_INST_SPILLED) == 0"
	.type .str.20, @object
	.size .str.20, 42

.str.21:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.21, @object
	.size .str.21, 30

.str.22:
	.asciz "(null)"
	.type .str.22, @object
	.size .str.22, 1

.str.23:
	.asciz "victim != NULL"
	.type .str.23, @object
	.size .str.23, 15

.str.24:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.24, @object
	.size .str.24, 30

.str.25:
	.asciz "(null)"
	.type .str.25, @object
	.size .str.25, 1

.str.26:
	.asciz "inst->users.length > 0"
	.type .str.26, @object
	.size .str.26, 23

.str.27:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.27, @object
	.size .str.27, 30

.str.28:
	.asciz "(null)"
	.type .str.28, @object
	.size .str.28, 1

.str.29:
	.asciz "reg != NULL"
	.type .str.29, @object
	.size .str.29, 12

.str.30:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.30, @object
	.size .str.30, 30

.str.31:
	.asciz "(null)"
	.type .str.31, @object
	.size .str.31, 1

.str.32:
	.asciz "inst->opcode == RV_OP(rmov) || inst->opcode == RV_OP(rmovf) ||inst->opcode == RV_OP(rmovd)"
	.type .str.32, @object
	.size .str.32, 91

.str.33:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.33, @object
	.size .str.33, 30

.str.34:
	.asciz "(null)"
	.type .str.34, @object
	.size .str.34, 1

.str.35:
	.asciz "inst->users.length == 0"
	.type .str.35, @object
	.size .str.35, 24

.str.36:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.36, @object
	.size .str.36, 30

.str.37:
	.asciz "(null)"
	.type .str.37, @object
	.size .str.37, 1

.str.38:
	.asciz "reg != NULL"
	.type .str.38, @object
	.size .str.38, 12

.str.39:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.39, @object
	.size .str.39, 30

.str.40:
	.asciz "(null)"
	.type .str.40, @object
	.size .str.40, 1

.str.41:
	.asciz "inst->dest->reg != NULL"
	.type .str.41, @object
	.size .str.41, 24

.str.42:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.42, @object
	.size .str.42, 30

.str.43:
	.asciz "(null)"
	.type .str.43, @object
	.size .str.43, 1

.str.44:
	.asciz "false"
	.type .str.44, @object
	.size .str.44, 6

.str.45:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.45, @object
	.size .str.45, 30

.str.46:
	.asciz "(null)"
	.type .str.46, @object
	.size .str.46, 1

.str.47:
	.asciz "inst->reg != NULL"
	.type .str.47, @object
	.size .str.47, 18

.str.48:
	.asciz "%s%d"
	.type .str.48, @object
	.size .str.48, 5

.str.49:
	.asciz "sp"
	.type .str.49, @object
	.size .str.49, 3

.str.50:
	.asciz "s0"
	.type .str.50, @object
	.size .str.50, 3

.str.51:
	.asciz "ra"
	.type .str.51, @object
	.size .str.51, 3

.str.52:
	.asciz "x%d"
	.type .str.52, @object
	.size .str.52, 4

.str.53:
	.asciz "f%d"
	.type .str.53, @object
	.size .str.53, 4

