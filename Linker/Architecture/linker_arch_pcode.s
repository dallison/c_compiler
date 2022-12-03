	.file   "Architecture/linker_arch_pcode.c"
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
	li          t0, 4		// 0x4 ASCII \x4
	beq         s7, t0, .HandlePICRelocation_label_110

	// *** Basic block 1

	li          t0, 12		// 0xc ASCII \xc
	beq         s7, t0, .HandlePICRelocation_label_94

	// *** Basic block 2

	li          t0, 13		// 0xd ASCII \xd
	beq         s7, t0, .HandlePICRelocation_label_65

	// *** Basic block 3

	li          t0, 17		// 0x11 ASCII \x11
	beq         s7, t0, .HandlePICRelocation_label_66

	// *** Basic block 4

	li          t0, 18		// 0x12 ASCII \x12
	beq         s7, t0, .HandlePICRelocation_label_75

	// *** Basic block 5

	j           .HandlePICRelocation_label_132

	// *** Basic block 6

.HandlePICRelocation_label_65:

	// *** Basic block 7

.HandlePICRelocation_label_66:
	mv          a1, s2
	mv          a0, s4
	jalr         x1, s3, 0

	// *** Basic block 8

	sw          a0, 88(s2)
	j           .HandlePICRelocation_label_132

	// *** Basic block 9

.HandlePICRelocation_label_75:
	mv          a1, s2
	mv          a0, s4
	jalr         x1, s3, 0

	// *** Basic block 10

	sw          a0, 88(s2)
	mv          a1, s2
	mv          a0, s4
	jalr         x1, s3, 0

	// *** Basic block 11

	lw          t0, 88(s2)
	addi        t0, t0, -1
	sw          t0, 88(s2)
	j           .HandlePICRelocation_label_132

	// *** Basic block 12

.HandlePICRelocation_label_94:
	mv          a1, s2
	mv          a0, s4
	jalr         x1, s5, 0

	// *** Basic block 13

	sw          a0, 88(s2)
	mv          a1, s2
	mv          a0, s4
	jalr         x1, s6, 0

	// *** Basic block 14

	sw          a0, 92(s2)
	j           .HandlePICRelocation_label_132

	// *** Basic block 15

.HandlePICRelocation_label_110:
	ld          a0, 64(s1)
	ld          a1, 48(s1)
	ld          a3, 72(s1)
	li          t0, 22		// 0x16 ASCII \x16
	mv          a2, t0
	call        NewRelativeRelocation

	// *** Basic block 16

	mv          s7, a0
	addi        a0, s4, 184
	mv          a1, s7
	call        VectorAppend

	// *** Basic block 17

	j           .HandlePICRelocation_label_132

	// *** Basic block 18

.HandlePICRelocation_label_132:
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

	.local  ApplyRelocation
	.type ApplyRelocation, @function

ApplyRelocation:

	// *** Basic block 0

	.global LinkerError
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -16(s0)
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
	sd s10, 0(sp)
	// End of stack frame
	mv          s1, a4
	mv          s2, a5
	mv          s3, a6
	mv          s4, a1
	mv          s5, a0
	mv          s6, a3
	ld          t0, 48(a2)
	ld          t0, 56(t0)
	ld          t1, 64(a2)
	add         t0, t0, t1
	addi        s7, t0, 12
	lw          s8, 56(a2)
	li          s9, 1		// 0x1 ASCII \x1
	blt         s8, s9, .ApplyRelocation_label_194

	// *** Basic block 1

	li          t0, 19		// 0x13 ASCII \x13
	blt         t0, s8, .ApplyRelocation_label_194

	// *** Basic block 2

	addi        t0, s8, -1
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .ApplyRelocation_label_128

	// *** Basic block 4

	j           .ApplyRelocation_label_91

	// *** Basic block 5

	j           .ApplyRelocation_label_127

	// *** Basic block 6

	j           .ApplyRelocation_label_134

	// *** Basic block 7

	j           .ApplyRelocation_label_138

	// *** Basic block 8

	j           .ApplyRelocation_label_194

	// *** Basic block 9

	j           .ApplyRelocation_label_194

	// *** Basic block 10

	j           .ApplyRelocation_label_194

	// *** Basic block 11

	j           .ApplyRelocation_label_194

	// *** Basic block 12

	j           .ApplyRelocation_label_194

	// *** Basic block 13

	j           .ApplyRelocation_label_194

	// *** Basic block 14

	j           .ApplyRelocation_label_144

	// *** Basic block 15

	j           .ApplyRelocation_label_166

	// *** Basic block 16

	j           .ApplyRelocation_label_184

	// *** Basic block 17

	j           .ApplyRelocation_label_186

	// *** Basic block 18

	j           .ApplyRelocation_label_188

	// *** Basic block 19

	j           .ApplyRelocation_label_164

	// *** Basic block 20

	j           .ApplyRelocation_label_165

	// *** Basic block 21

	j           .ApplyRelocation_label_90

	// *** Basic block 22

.ApplyRelocation_label_90:

	// *** Basic block 23

.ApplyRelocation_label_91:
	lb          t0, 3(s1)
	andi        s10, t0, 63
	bne         s10, s9, .ApplyRelocation_label_104

	// *** Basic block 24

	add         t0, s2, s3
	sd          t0, 4(s1)
	j           .ApplyRelocation_label_125

	// *** Basic block 25

.ApplyRelocation_label_104:
	li          t0, 5		// 0x5 ASCII \x5
	bne         s10, t0, .ApplyRelocation_label_115

	// *** Basic block 26

	add         t0, s2, s3
	sub         t0, t0, s7
	sd          t0, 4(s1)
	j           .ApplyRelocation_label_124

	// *** Basic block 27

.ApplyRelocation_label_115:
	lla         a1, .str.1
	mv          a2, s10
	mv          a0, s4
	call        LinkerError

	// *** Basic block 28

.ApplyRelocation_label_124:

	// *** Basic block 29

.ApplyRelocation_label_125:
	j           .ApplyRelocation_label_204

	// *** Basic block 30

.ApplyRelocation_label_127:

	// *** Basic block 31

.ApplyRelocation_label_128:
	add         t0, s2, s3
	sub         t0, t0, s7
	sd          t0, 4(s1)
	j           .ApplyRelocation_label_204

	// *** Basic block 32

.ApplyRelocation_label_134:
	add         t0, s2, s3
	sd          t0, 0(s1)
	j           .ApplyRelocation_label_204

	// *** Basic block 33

.ApplyRelocation_label_138:
	add         t0, s2, s3
	li          t1, 4294967295		// 0xffffffff
	and         t0, t0, t1
	sw          t0, 0(s1)
	j           .ApplyRelocation_label_204

	// *** Basic block 34

.ApplyRelocation_label_144:
	ld          t0, 536(s5)
	ld          t0, 272(t0)
	ld          s10, 96(t0)
	lw          t0, 92(s6)
	slli        t1, t0, 2
	slli        t0, t0, 5
	add         t0, t1, t0
	add         t0, s10, t0
	add         s10, t0, s3
	sub         t0, s10, s7
	sd          t0, 4(s1)
	j           .ApplyRelocation_label_204

	// *** Basic block 35

.ApplyRelocation_label_164:

	// *** Basic block 36

.ApplyRelocation_label_165:

	// *** Basic block 37

.ApplyRelocation_label_166:
	ld          t0, 536(s5)
	ld          t0, 256(t0)
	ld          s9, 96(t0)
	lw          t0, 88(s6)
	slli        t0, t0, 3
	add         t0, s9, t0
	add         s9, t0, s3
	sub         t0, s9, s7
	sd          t0, 4(s1)
	j           .ApplyRelocation_label_204

	// *** Basic block 38

.ApplyRelocation_label_184:
	j           .ApplyRelocation_label_204

	// *** Basic block 39

.ApplyRelocation_label_186:
	j           .ApplyRelocation_label_204

	// *** Basic block 40

.ApplyRelocation_label_188:
	add         t0, s2, s3
	sub         t0, t0, s7
	sd          t0, 4(s1)
	j           .ApplyRelocation_label_204

	// *** Basic block 41

.ApplyRelocation_label_194:
	lla         a1, .str.2
	mv          a2, s8
	mv          a0, s4
	call        LinkerError

	// *** Basic block 42

	j           .ApplyRelocation_label_204

	// *** Basic block 43

.ApplyRelocation_label_204:
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
	ld s10, 0(sp)
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
	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, 0(a0)
	li          t0, 8		// 0x8 ASCII \x8
	sw          t0, 4(a0)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 104(a0)
	addi        t0, a0, 104
	li          t1, 36		// 0x24 ASCII '$'
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

	j           .AddGOTEntry_label_44

	// *** Basic block 2

	j           .AddGOTEntry_label_40

	// *** Basic block 3

	j           .AddGOTEntry_label_47

	// *** Basic block 4

	j           .AddGOTEntry_label_50

	// *** Basic block 5

.AddGOTEntry_label_40:
	li          s5, 15		// 0xf ASCII \xf
	j           .AddGOTEntry_label_53

	// *** Basic block 6

.AddGOTEntry_label_44:
	li          s5, 14		// 0xe ASCII \xe
	j           .AddGOTEntry_label_53

	// *** Basic block 7

.AddGOTEntry_label_47:
	li          s5, 20		// 0x14 ASCII \x14
	j           .AddGOTEntry_label_53

	// *** Basic block 8

.AddGOTEntry_label_50:
	li          s5, 21		// 0x15 ASCII \x15
	j           .AddGOTEntry_label_53

	// *** Basic block 9

.AddGOTEntry_label_53:
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
	lw          t0, 92(a0)
	mul         t0, t0, a3
	add         t0, a2, t0
	addi        t1, t0, 12
	ld          t0, 0(a1)
	lw          t2, 88(a0)
	slli        t2, t2, 3
	add         t3, t0, t2
	sd          t1, 0(t3)
	ret         
.func_end_FixupGOTEntry:
	.size FixupGOTEntry, .func_end_FixupGOTEntry-FixupGOTEntry

	.local  AddPLTEntry
	.type AddPLTEntry, @function

AddPLTEntry:

	// *** Basic block 0

	.global BufferAppendWordLE
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
	mv          s1, a2
	mv          s2, a1
	ld          s3, 536(t0)
	addi        s4, s3, 104
	addi        a0, s1, 8
	li          t1, -1006632960		// 0xffffffffc4000000
	mv          a1, t1
	call        BufferAppendWordLE

	// *** Basic block 1

	addi        a0, s1, 8
	mv          a1, x0
	call        BufferAppendWordLE

	// *** Basic block 2

	addi        a0, s1, 8
	mv          a1, x0
	call        BufferAppendWordLE

	// *** Basic block 3

	addi        a0, s1, 8
	li          t0, -1055260672		// 0xffffffffc11a0000
	mv          a1, t0
	call        BufferAppendWordLE

	// *** Basic block 4

	addi        a0, s1, 8
	lw          t0, 88(s2)
	lw          t1, 0(s3)
	sub         a1, t0, t1
	call        BufferAppendWordLE

	// *** Basic block 5

	addi        a0, s1, 8
	mv          a1, x0
	call        BufferAppendWordLE

	// *** Basic block 6

	addi        a0, s1, 8
	li          t0, -1040187392		// 0xffffffffc2000000
	mv          a1, t0
	call        BufferAppendWordLE

	// *** Basic block 7

	lw          t0, 92(s2)
	lw          t1, 4(s4)
	mul         t0, t0, t1
	add         t0, t0, t1
	neg         s3, t0
	addi        a0, s1, 8
	li          s4, 4294967295		// 0xffffffff
	and         t0, s3, s4
	and         a1, t0, s4
	call        BufferAppendWordLE

	// *** Basic block 8

	addi        a0, s1, 8
	srai        t0, s3, 32
	and         a1, t0, s4
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
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

	// Leaf procedure, no stack frame generated
	ld          t0, 0(a1)
	sw          x0, 0(t0)
	addi        t1, a3, 12
	sub         t2, a2, t1
	li          t3, 4294967295		// 0xffffffff
	and         t4, t2, t3
	sw          t4, 4(t0)
	srai        t4, t2, 32
	sw          t4, 8(t0)
	sw          x0, 12(t0)
	addi        t4, a2, 8
	addi        t1, t1, 12
	sub         t2, t4, t1
	and         t1, t2, t3
	sw          t1, 16(t0)
	srai        t1, t2, 32
	sw          t1, 20(t0)
	ret         
.func_end_SetupResolverPLTEntry:
	.size SetupResolverPLTEntry, .func_end_SetupResolverPLTEntry-SetupResolverPLTEntry

	.local  FixupPLTEntry
	.type FixupPLTEntry, @function

FixupPLTEntry:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 92(a2)
	lw          t1, 4(a0)
	mul         t2, t0, t1
	lw          t0, 88(a2)
	lw          t1, 4(a1)
	mul         t0, t0, t1
	add         t1, a4, t0
	add         t0, a5, t2
	ld          t3, 0(a3)
	add         t2, t3, t2
	addi        t3, t2, 4
	addi        t0, t0, 12
	sub         t2, t1, t0
	li          t0, 4294967295		// 0xffffffff
	and         t0, t2, t0
	sw          t0, 0(t3)
	srai        t0, t2, 32
	sw          t0, 4(t3)
	ret         
.func_end_FixupPLTEntry:
	.size FixupPLTEntry, .func_end_FixupPLTEntry-FixupPLTEntry

	.global NewPCodeLinkerArchitecture
	.type NewPCodeLinkerArchitecture, @function

NewPCodeLinkerArchitecture:

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
	li          t0, 6500		// 0x1964
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

.NewPCodeLinkerArchitecture_label_67:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewPCodeLinkerArchitecture:
	.size NewPCodeLinkerArchitecture, .func_end_NewPCodeLinkerArchitecture-NewPCodeLinkerArchitecture

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "Unsupported P-CODE ABS relocation opcode %d"
	.type .str.1, @object
	.size .str.1, 44

.str.2:
	.asciz "Unsupported P-CODE relocation type %d"
	.type .str.2, @object
	.size .str.2, 38

