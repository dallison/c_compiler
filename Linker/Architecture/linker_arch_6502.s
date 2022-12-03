	.file   "Architecture/linker_arch_6502.c"
	.text
	.option pic
.PCbegin:
	.local  CodeStartAddress
	.type CodeStartAddress, @function

CodeStartAddress:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 576(a0)
	bnez        t0, .CodeStartAddress_label_16

	// *** Basic block 1

	li          t0, 1024		// 0x400
	j           .CodeStartAddress_label_17

	// *** Basic block 2

.CodeStartAddress_label_16:

	// *** Basic block 3

.CodeStartAddress_label_17:
	mv          a0, t0

	// *** Basic block 4

.CodeStartAddress_label_20:
	ret         
.func_end_CodeStartAddress:
	.size CodeStartAddress, .func_end_CodeStartAddress-CodeStartAddress

	.local  DataStartAddress
	.type DataStartAddress, @function

DataStartAddress:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	add         t0, a1, a2
	mv          a0, t0

	// *** Basic block 1

.DataStartAddress_label_11:
	ret         
.func_end_DataStartAddress:
	.size DataStartAddress, .func_end_DataStartAddress-DataStartAddress

	.local  HandlePICRelocation
	.type HandlePICRelocation, @function

HandlePICRelocation:

	// *** Basic block 0

	ret         
.func_end_HandlePICRelocation:
	.size HandlePICRelocation, .func_end_HandlePICRelocation-HandlePICRelocation

	.local  ApplyRelocation
	.type ApplyRelocation, @function

ApplyRelocation:

	// *** Basic block 0

	.global LinkerError
	addi sp, sp, -16
	// Saved return address (offset 8) and frame pointer (offset 0)
	sd ra, 8(sp)
	sd s0, 0(sp)
	addi s0, sp, 16
	// Local vars at offset -16(s0)
	// End of stack frame
	mv          t0, a4
	mv          t1, a1
	add         t2, a5, a6
	lw          t3, 56(a2)
	li          t4, 1		// 0x1 ASCII \x1
	beq         t3, t4, .ApplyRelocation_label_123

	// *** Basic block 1

	li          t4, 2		// 0x2 ASCII \x2
	beq         t3, t4, .ApplyRelocation_label_124

	// *** Basic block 2

	li          t4, 3		// 0x3 ASCII \x3
	beq         t3, t4, .ApplyRelocation_label_130

	// *** Basic block 3

	li          t4, 21		// 0x15 ASCII \x15
	beq         t3, t4, .ApplyRelocation_label_138

	// *** Basic block 4

	li          t4, 22		// 0x16 ASCII \x16
	beq         t3, t4, .ApplyRelocation_label_135

	// *** Basic block 5

	li          t4, 23		// 0x17 ASCII \x17
	beq         t3, t4, .ApplyRelocation_label_143

	// *** Basic block 6

	li          t4, 24		// 0x18 ASCII \x18
	beq         t3, t4, .ApplyRelocation_label_149

	// *** Basic block 7

	li          t4, 25		// 0x19 ASCII \x19
	beq         t3, t4, .ApplyRelocation_label_156

	// *** Basic block 8

	li          t4, 26		// 0x1a ASCII \x1a
	beq         t3, t4, .ApplyRelocation_label_163

	// *** Basic block 9

	li          t4, 27		// 0x1b ASCII \x1b
	beq         t3, t4, .ApplyRelocation_label_170

	// *** Basic block 10

	li          t4, 28		// 0x1c ASCII \x1c
	beq         t3, t4, .ApplyRelocation_label_177

	// *** Basic block 11

	li          t4, 29		// 0x1d ASCII \x1d
	beq         t3, t4, .ApplyRelocation_label_184

	// *** Basic block 12

	li          t4, 30		// 0x1e ASCII \x1e
	beq         t3, t4, .ApplyRelocation_label_191

	// *** Basic block 13

.ApplyRelocation_label_111:
	lla         a1, .str.1
	mv          a2, t3
	mv          a0, t1
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LinkerError

	// *** Basic block 15

.ApplyRelocation_label_123:

	// *** Basic block 16

.ApplyRelocation_label_124:
	addi        t3, t0, 1
	sh          t2, 0(t3)
	j           .ApplyRelocation_label_198

	// *** Basic block 17

.ApplyRelocation_label_130:
	sh          t2, 0(t0)
	j           .ApplyRelocation_label_198

	// *** Basic block 18

.ApplyRelocation_label_135:
	sd          t2, 0(t0)
	j           .ApplyRelocation_label_198

	// *** Basic block 19

.ApplyRelocation_label_138:
	li          t3, 4294967295		// 0xffffffff
	and         t3, t2, t3
	sw          t3, 0(t0)
	j           .ApplyRelocation_label_198

	// *** Basic block 20

.ApplyRelocation_label_143:
	andi        t3, t2, 255
	sb          t3, 0(t0)
	j           .ApplyRelocation_label_198

	// *** Basic block 21

.ApplyRelocation_label_149:
	srli        t3, t2, 8
	andi        t3, t3, 255
	sb          t3, 0(t0)
	j           .ApplyRelocation_label_198

	// *** Basic block 22

.ApplyRelocation_label_156:
	srli        t3, t2, 16
	andi        t3, t3, 255
	sb          t3, 0(t0)
	j           .ApplyRelocation_label_198

	// *** Basic block 23

.ApplyRelocation_label_163:
	srli        t3, t2, 24
	andi        t3, t3, 255
	sb          t3, 0(t0)
	j           .ApplyRelocation_label_198

	// *** Basic block 24

.ApplyRelocation_label_170:
	srli        t3, t2, 32
	andi        t3, t3, 255
	sb          t3, 0(t0)
	j           .ApplyRelocation_label_198

	// *** Basic block 25

.ApplyRelocation_label_177:
	srli        t3, t2, 40
	andi        t3, t3, 255
	sb          t3, 0(t0)
	j           .ApplyRelocation_label_198

	// *** Basic block 26

.ApplyRelocation_label_184:
	srli        t3, t2, 48
	andi        t3, t3, 255
	sb          t3, 0(t0)
	j           .ApplyRelocation_label_198

	// *** Basic block 27

.ApplyRelocation_label_191:
	srli        t3, t2, 56
	andi        t3, t3, 255
	sb          t3, 0(t0)
	j           .ApplyRelocation_label_198

	// *** Basic block 28

.ApplyRelocation_label_198:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ApplyRelocation:
	.size ApplyRelocation, .func_end_ApplyRelocation-ApplyRelocation

	.local  InitDynamicLinker
	.type InitDynamicLinker, @function

InitDynamicLinker:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sw          x0, 0(a0)
	sw          x0, 4(a0)
	sw          x0, 104(a0)
	addi        t0, a0, 104
	sw          x0, 4(t0)
	ret         
.func_end_InitDynamicLinker:
	.size InitDynamicLinker, .func_end_InitDynamicLinker-InitDynamicLinker

	.local  AddGOTEntry
	.type AddGOTEntry, @function

AddGOTEntry:

	// *** Basic block 0

	ret         
.func_end_AddGOTEntry:
	.size AddGOTEntry, .func_end_AddGOTEntry-AddGOTEntry

	.local  FixupGOTEntry
	.type FixupGOTEntry, @function

FixupGOTEntry:

	// *** Basic block 0

	ret         
.func_end_FixupGOTEntry:
	.size FixupGOTEntry, .func_end_FixupGOTEntry-FixupGOTEntry

	.local  AddPLTEntry
	.type AddPLTEntry, @function

AddPLTEntry:

	// *** Basic block 0

	ret         
.func_end_AddPLTEntry:
	.size AddPLTEntry, .func_end_AddPLTEntry-AddPLTEntry

	.local  SetupResolverPLTEntry
	.type SetupResolverPLTEntry, @function

SetupResolverPLTEntry:

	// *** Basic block 0

	ret         
.func_end_SetupResolverPLTEntry:
	.size SetupResolverPLTEntry, .func_end_SetupResolverPLTEntry-SetupResolverPLTEntry

	.local  FixupPLTEntry
	.type FixupPLTEntry, @function

FixupPLTEntry:

	// *** Basic block 0

	ret         
.func_end_FixupPLTEntry:
	.size FixupPLTEntry, .func_end_FixupPLTEntry-FixupPLTEntry

	.global New6502LinkerArchitecture
	.type New6502LinkerArchitecture, @function

New6502LinkerArchitecture:

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
	li          t0, 6502		// 0x1966
	sw          t0, 0(s1)
	la          t0, CodeStartAddress
	sd          t0, 8(s1)
	la          t0, DataStartAddress
	sd          t0, 16(s1)
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

.New6502LinkerArchitecture_label_67:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_New6502LinkerArchitecture:
	.size New6502LinkerArchitecture, .func_end_New6502LinkerArchitecture-New6502LinkerArchitecture

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "Unsupported 6502 relocation type %d"
	.type .str.1, @object
	.size .str.1, 36

