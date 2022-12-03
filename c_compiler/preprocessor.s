	.file   "preprocessor.c"
	.text
	.option pic
.PCbegin:
	.global NewMacro
	.type NewMacro, @function

NewMacro:

	// *** Basic block 0

	.global malloc
	.global BinaryTreeNodeInit
	.global StringInit
	.global VectorInit
	.global VectorCopy
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
	mv          s1, a0
	mv          s2, a3
	mv          s3, a4
	mv          s4, a1
	mv          s5, a2
	mv          s6, a5
	li          a0, 160		// 0xa0 ASCII \xa0
	call        malloc

	// *** Basic block 1

	mv          s7, a0
	mv          a0, s7
	call        BinaryTreeNodeInit

	// *** Basic block 2

	addi        a0, s7, 40
	mv          a1, s1
	call        StringInit

	// *** Basic block 3

	addi        a0, s7, 80
	call        VectorInit

	// *** Basic block 4

	addi        a0, s7, 80
	mv          a1, s2
	call        VectorCopy

	// *** Basic block 5

	addi        a0, s7, 104
	ld          a1, 16(s3)
	call        StringInit

	// *** Basic block 6

	sb          s4, 144(s7)
	sb          x0, 145(s7)
	sb          s5, 146(s7)
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 147(s7)
	sd          s6, 152(s7)
	mv          a0, s7

	// *** Basic block 7

.NewMacro_label_78:
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
.func_end_NewMacro:
	.size NewMacro, .func_end_NewMacro-NewMacro

	.global MacroDestruct
	.type MacroDestruct, @function

MacroDestruct:

	// *** Basic block 0

	.global StringDestruct
	.global VectorDestruct
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
	addi        a0, s1, 40
	call        StringDestruct

	// *** Basic block 1

	addi        a0, s1, 104
	call        StringDestruct

	// *** Basic block 2

	addi        a0, s1, 80
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorDestruct
.func_end_MacroDestruct:
	.size MacroDestruct, .func_end_MacroDestruct-MacroDestruct

	.global MacroDisable
	.type MacroDisable, @function

MacroDisable:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sb          x0, 147(a0)
	ret         
.func_end_MacroDisable:
	.size MacroDisable, .func_end_MacroDisable-MacroDisable

	.global MacroEnable
	.type MacroEnable, @function

MacroEnable:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 147(a0)
	ret         
.func_end_MacroEnable:
	.size MacroEnable, .func_end_MacroEnable-MacroEnable

	.local  MacroInsertCompare
	.type MacroInsertCompare, @function

MacroInsertCompare:

	// *** Basic block 0

	.global StringCompareString
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, t0
	mv          t3, t1
	addi        a0, t2, 40
	addi        a1, t3, 40
	j           StringCompareString
.func_end_MacroInsertCompare:
	.size MacroInsertCompare, .func_end_MacroInsertCompare-MacroInsertCompare

	.local  MacroSearchCompare
	.type MacroSearchCompare, @function

MacroSearchCompare:

	// *** Basic block 0

	.global StringCompare
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	addi        a0, t1, 40
	j           StringCompare
.func_end_MacroSearchCompare:
	.size MacroSearchCompare, .func_end_MacroSearchCompare-MacroSearchCompare

	.local  MacroDestructor
	.type MacroDestructor, @function

MacroDestructor:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global MacroDestruct
	j           MacroDestruct
.func_end_MacroDestructor:
	.size MacroDestructor, .func_end_MacroDestructor-MacroDestructor

	.local  DeleteMacroTree
	.type DeleteMacroTree, @function

DeleteMacroTree:

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
	.global BinaryTreeDestruct
	.global free
	mv          s1, a0
	call        BinaryTreeDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_DeleteMacroTree:
	.size DeleteMacroTree, .func_end_DeleteMacroTree-DeleteMacroTree

	.local  HashMacro
	.type HashMacro, @function

HashMacro:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	slli        t1, a2, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 1

	j           .HashMacro_label_30

	// *** Basic block 2

	j           .HashMacro_label_24

	// *** Basic block 3

.HashMacro_label_24:
	addi        t2, t0, 40
	ld          t1, 16(t2)
	j           .HashMacro_label_33

	// *** Basic block 4

.HashMacro_label_30:
	mv          t1, t0
	j           .HashMacro_label_33

	// *** Basic block 5

.HashMacro_label_33:
	li          t2, 5381		// 0x1505
	lb          t3, 0(t1)
	beqz        t3, .HashMacro_label_50

	// *** Basic block 6

.HashMacro_label_39:
	slli        t3, t2, 5
	add         t3, t3, t2
	mv          t4, t1
	addi        t1, t1, 1
	lb          t5, 0(t4)
	add         t2, t3, t5
	lb          t3, 0(t1)
	bnez        t3, .HashMacro_label_39

	// *** Basic block 7

.HashMacro_label_50:
	mv          a0, t2

	// *** Basic block 8

.HashMacro_label_53:
	ret         
.func_end_HashMacro:
	.size HashMacro, .func_end_HashMacro-HashMacro

	.local  InsertMacroInHashTable
	.type InsertMacroInHashTable, @function

InsertMacroInHashTable:

	// *** Basic block 0

	.global NewBinaryTree
	.local MacroInsertCompare
	.local MacroSearchCompare
	.local MacroDestructor
	.global BinaryTreeInsert
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
	mv          s2, a1
	mv          s3, a0
	bne         a0, x0, .InsertMacroInHashTable_label_35

	// *** Basic block 1

	la          t0, MacroDestructor
	mv          a2, t0
	la          t0, MacroSearchCompare
	mv          a1, t0
	la          t0, MacroInsertCompare
	mv          a0, t0
	call        NewBinaryTree

	// *** Basic block 2

	mv          s3, a0
	sd          s3, 0(s1)

	// *** Basic block 3

.InsertMacroInHashTable_label_35:
	mv          a1, s2
	mv          a0, s3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           BinaryTreeInsert
.func_end_InsertMacroInHashTable:
	.size InsertMacroInHashTable, .func_end_InsertMacroInHashTable-InsertMacroInHashTable

	.local  FindMacroInHashTable
	.type FindMacroInHashTable, @function

FindMacroInHashTable:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global BinaryTreeSearch
	mv          t0, a0
	mv          t1, t0
	mv          a0, t1
	j           BinaryTreeSearch
.func_end_FindMacroInHashTable:
	.size FindMacroInHashTable, .func_end_FindMacroInHashTable-FindMacroInHashTable

	.local  PredefineMacros
	.type PredefineMacros, @function

PredefineMacros:

	// *** Basic block 0

	.global PreprocessorDefineMacro
	.global time
	.global ctime
	.global StringInitFromSegment
	.global StringAppendSegment
	.global StringDestruct
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -112(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0
	lla         a1, .str.1
	lla         a2, .str.2
	call        PreprocessorDefineMacro

	// *** Basic block 1

	lla         a1, .str.3
	lla         a2, .str.4
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 2

	lla         a1, .str.5
	lla         a2, .str.6
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 3

	lla         a1, .str.7
	lla         a2, .str.8
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 4

	lla         a1, .str.9
	lla         a2, .str.10
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 5

	mv          a0, x0
	call        time

	// *** Basic block 6

	sd          a0, -112(s0)
	addi        a0, s0, -112
	call        ctime

	// *** Basic block 7

	mv          s2, a0
	addi        a0, s0, -104
	addi        a1, s2, 4
	li          s3, 4		// 0x4 ASCII \x4
	mv          a2, s3
	call        StringInitFromSegment

	// *** Basic block 8

	addi        a0, s0, -104
	addi        a1, s2, 8
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	call        StringAppendSegment

	// *** Basic block 9

	addi        a0, s0, -104
	addi        a1, s2, 18
	mv          a2, s3
	call        StringAppendSegment

	// *** Basic block 10

	lla         a1, .str.11
	addi        t0, s0, -104
	ld          a2, 16(t0)
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 11

	addi        a0, s0, -104
	call        StringDestruct

	// *** Basic block 12

	addi        a0, s0, -64
	addi        a1, s2, 10
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	call        StringInitFromSegment

	// *** Basic block 13

	lla         a1, .str.12
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 14

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 15

	lla         a1, .str.13
	lla         a2, .str.14
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 16

	lla         a1, .str.15
	lla         a2, .str.16
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 17

	lla         a1, .str.17
	lla         a2, .str.18
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 18

	lla         a1, .str.19
	lla         a2, .str.20
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 19

	lla         a1, .str.21
	lla         a2, .str.22
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 20

	lla         a1, .str.23
	lla         a2, .str.24
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 21

	lla         a1, .str.25
	lla         a2, .str.26
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 22

	lla         a1, .str.27
	lla         a2, .str.28
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 23

	lla         a1, .str.29
	lla         a2, .str.30
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 24

	lla         a1, .str.31
	lla         a2, .str.32
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 25

	lla         a1, .str.33
	lla         a2, .str.34
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 26

	lla         a1, .str.35
	lla         a2, .str.36
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 27

	lla         a1, .str.37
	lla         a2, .str.38
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 28

	lla         a1, .str.39
	lla         a2, .str.40
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 29

	lla         a1, .str.41
	lla         a2, .str.42
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 30

	lla         a1, .str.43
	lla         a2, .str.44
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 31

	lla         a1, .str.45
	lla         a2, .str.46
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 32

	lla         a1, .str.47
	lla         a2, .str.48
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 33

	lla         a1, .str.49
	lla         a2, .str.50
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 34

	lla         a1, .str.51
	lla         a2, .str.52
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 35

	lla         a1, .str.53
	lla         a2, .str.54
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 36

	lla         a1, .str.55
	lla         a2, .str.56
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 37

	lla         a1, .str.57
	lla         a2, .str.58
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 38

	lla         a1, .str.59
	lla         a2, .str.60
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 39

	lla         a1, .str.61
	lla         a2, .str.62
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 40

	lla         a1, .str.63
	lla         a2, .str.64
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 41

	lla         a1, .str.65
	lla         a2, .str.66
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 42

	lla         a1, .str.67
	lla         a2, .str.68
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 43

	lla         a1, .str.69
	lla         a2, .str.70
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 44

	lla         a1, .str.71
	lla         a2, .str.72
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 45

	lla         a1, .str.73
	lla         a2, .str.74
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 46

	lla         a1, .str.75
	lla         a2, .str.76
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 47

	lla         a1, .str.77
	lla         a2, .str.78
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 48

	lla         a1, .str.79
	lla         a2, .str.80
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 49

	lla         a1, .str.81
	lla         a2, .str.82
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 50

	lla         a1, .str.83
	lla         a2, .str.84
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 51

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PredefineMacros:
	.size PredefineMacros, .func_end_PredefineMacros-PredefineMacros

	.global PreprocessorDefineArchitectureMacros
	.type PreprocessorDefineArchitectureMacros, @function

PreprocessorDefineArchitectureMacros:

	// *** Basic block 0

	.global StringEqual
	.global compiler
	.global PreprocessorDefineMacro
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
	la          t0, compiler
	ld          t0, 0(t0)
	ld          s2, 1120(t0)
	lla         a1, .str.85
	mv          a0, s2
	call        StringEqual

	// *** Basic block 1

	beqz        a0, .PreprocessorDefineArchitectureMacros_label_51

	// *** Basic block 2

	lla         a1, .str.86
	lla         a2, .str.87
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 3

	j           .PreprocessorDefineArchitectureMacros_label_147

	// *** Basic block 4

.PreprocessorDefineArchitectureMacros_label_51:
	lla         a1, .str.88
	mv          a0, s2
	call        StringEqual

	// *** Basic block 5

	beqz        a0, .PreprocessorDefineArchitectureMacros_label_69

	// *** Basic block 6

	lla         a1, .str.89
	lla         a2, .str.90
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 7

	j           .PreprocessorDefineArchitectureMacros_label_146

	// *** Basic block 8

.PreprocessorDefineArchitectureMacros_label_69:
	lla         a1, .str.91
	mv          a0, s2
	call        StringEqual

	// *** Basic block 9

	mv          s3, a0
	bnez        a0, .PreprocessorDefineArchitectureMacros_label_86

	// *** Basic block 10

	lla         a1, .str.92
	mv          a0, s2
	call        StringEqual

	// *** Basic block 11

	mv          s3, a0

	// *** Basic block 12

.PreprocessorDefineArchitectureMacros_label_86:
	beqz        s3, .PreprocessorDefineArchitectureMacros_label_107

	// *** Basic block 13

	lla         a1, .str.93
	lla         a2, .str.94
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 14

	lla         a1, .str.95
	lla         a2, .str.96
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 15

	j           .PreprocessorDefineArchitectureMacros_label_145

	// *** Basic block 16

.PreprocessorDefineArchitectureMacros_label_107:
	lla         a1, .str.97
	mv          a0, s2
	call        StringEqual

	// *** Basic block 17

	mv          s4, a0
	bnez        a0, .PreprocessorDefineArchitectureMacros_label_124

	// *** Basic block 18

	lla         a1, .str.98
	mv          a0, s2
	call        StringEqual

	// *** Basic block 19

	mv          s4, a0

	// *** Basic block 20

.PreprocessorDefineArchitectureMacros_label_124:
	beqz        s4, .PreprocessorDefineArchitectureMacros_label_144

	// *** Basic block 21

	lla         a1, .str.99
	lla         a2, .str.100
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 22

	lla         a1, .str.101
	lla         a2, .str.102
	mv          a0, s1
	call        PreprocessorDefineMacro

	// *** Basic block 23

.PreprocessorDefineArchitectureMacros_label_144:

	// *** Basic block 24

.PreprocessorDefineArchitectureMacros_label_145:

	// *** Basic block 25

.PreprocessorDefineArchitectureMacros_label_146:

	// *** Basic block 26

.PreprocessorDefineArchitectureMacros_label_147:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PreprocessorDefineArchitectureMacros:
	.size PreprocessorDefineArchitectureMacros, .func_end_PreprocessorDefineArchitectureMacros-PreprocessorDefineArchitectureMacros

	.global PreprocessorInit
	.type PreprocessorInit, @function

PreprocessorInit:

	// *** Basic block 0

	.global HashTableInit
	.local HashMacro
	.local InsertMacroInHashTable
	.local FindMacroInHashTable
	.global VectorInit
	.global PreprocessorAddSystemIncludePath
	.global getcwd
	.global PreprocessorAddUserIncludePath
	.local PredefineMacros
	addi sp, sp, -16
	sd ra, 8(sp)
	sd s0, 0(sp)
	lui t0, 1
	addi t0, t0, 16
	sub sp, sp, t0
	addi t0, t0, 16
	add s0, sp, t0
	// Local vars at offset -4112(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	mv          s1, a0
	lla         a1, .str.103
	la          a5, FindMacroInHashTable
	la          a4, InsertMacroInHashTable
	la          a3, HashMacro
	li          a2, 1009		// 0x3f1
	call        HashTableInit

	// *** Basic block 1

	addi        a0, s1, 88
	call        VectorInit

	// *** Basic block 2

	sd          x0, 160(s1)
	addi        a0, s1, 112
	call        VectorInit

	// *** Basic block 3

	addi        a0, s1, 136
	call        VectorInit

	// *** Basic block 4

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 168(s1)
	lla         a1, .str.104
	mv          a0, s1
	call        PreprocessorAddSystemIncludePath

	// *** Basic block 5

	li          t0, -4096		// 0xfffffffffffff000
	add         t0, s0, t0
	addi        s2, t0, -16
	li          t0, 4096		// 0x1000
	mv          a1, t0
	mv          a0, s2
	call        getcwd

	// *** Basic block 6

	mv          a1, s2
	mv          a0, s1
	call        PreprocessorAddUserIncludePath

	// *** Basic block 7

	mv          a0, s1
	call        PredefineMacros

	// *** Basic block 8

	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PreprocessorInit:
	.size PreprocessorInit, .func_end_PreprocessorInit-PreprocessorInit

	.global PreprocessorPrintStats
	.type PreprocessorPrintStats, @function

PreprocessorPrintStats:

	// *** Basic block 0

	.global HashTablePrintStats
	// Leaf procedure, no stack frame generated
	j           HashTablePrintStats
.func_end_PreprocessorPrintStats:
	.size PreprocessorPrintStats, .func_end_PreprocessorPrintStats-PreprocessorPrintStats

	.global PreprocessorFinishInit
	.type PreprocessorFinishInit, @function

PreprocessorFinishInit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sd          a1, 160(a0)
	ret         
.func_end_PreprocessorFinishInit:
	.size PreprocessorFinishInit, .func_end_PreprocessorFinishInit-PreprocessorFinishInit

	.global PreprocessorDestruct
	.type PreprocessorDestruct, @function

PreprocessorDestruct:

	// *** Basic block 0

	.global VectorDestruct
	.global StringDelete
	.global PreprocessorReset
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
	addi        a0, s1, 88
	call        VectorDestruct

	// *** Basic block 1

	mv          s2, x0
	addi        t0, s1, 112
	ld          s3, 8(t0)
	bge         x0, s3, .PreprocessorDestruct_label_38

	// *** Basic block 2

	ld          s4, 112(s1)

	// *** Basic block 3

.PreprocessorDestruct_label_27:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          a0, 0(t0)
	call        StringDelete

	// *** Basic block 4

.PreprocessorDestruct_label_34:
	addi        s2, s2, 1
	bge         s2, s3, .PreprocessorDestruct_label_27

	// *** Basic block 5

.PreprocessorDestruct_label_38:
	addi        a0, s1, 112
	call        VectorDestruct

	// *** Basic block 6

	mv          s3, x0
	addi        t0, s1, 136
	ld          s4, 8(t0)
	bge         x0, s4, .PreprocessorDestruct_label_60

	// *** Basic block 7

	ld          s5, 136(s1)

	// *** Basic block 8

.PreprocessorDestruct_label_50:
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          a0, 0(t0)
	call        StringDelete

	// *** Basic block 9

.PreprocessorDestruct_label_56:
	addi        s3, s3, 1
	bge         s3, s4, .PreprocessorDestruct_label_50

	// *** Basic block 10

.PreprocessorDestruct_label_60:
	addi        a0, s1, 136
	call        VectorDestruct

	// *** Basic block 11

	mv          a0, s1
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           PreprocessorReset
.func_end_PreprocessorDestruct:
	.size PreprocessorDestruct, .func_end_PreprocessorDestruct-PreprocessorDestruct

	.global PreprocessorReset
	.type PreprocessorReset, @function

PreprocessorReset:

	// *** Basic block 0

	.global HashTableTraverse
	.local DeleteMacroTree
	.global HashTableClear
	.local PredefineMacros
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
	mv          a2, x0
	la          a1, DeleteMacroTree
	call        HashTableTraverse

	// *** Basic block 1

	mv          a0, s1
	call        HashTableClear

	// *** Basic block 2

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           PredefineMacros
.func_end_PreprocessorReset:
	.size PreprocessorReset, .func_end_PreprocessorReset-PreprocessorReset

	.global PreprocessorAddUserIncludePath
	.type PreprocessorAddUserIncludePath, @function

PreprocessorAddUserIncludePath:

	// *** Basic block 0

	.global StringEqual
	.global VectorAppend
	.global NewString
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
	mv          s3, x0
	addi        t0, s1, 112
	ld          s4, 8(t0)
	bge         x0, s4, .PreprocessorAddUserIncludePath_label_43

	// *** Basic block 1

	ld          s5, 112(s1)

	// *** Basic block 2

.PreprocessorAddUserIncludePath_label_25:
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          a0, 0(t0)
	mv          a1, s2
	call        StringEqual

	// *** Basic block 3

	beqz        a0, .PreprocessorAddUserIncludePath_label_38

	// *** Basic block 4

.PreprocessorAddUserIncludePath_label_35:
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

	// *** Basic block 5

.PreprocessorAddUserIncludePath_label_38:

	// *** Basic block 6

.PreprocessorAddUserIncludePath_label_39:
	addi        s3, s3, 1
	bge         s3, s4, .PreprocessorAddUserIncludePath_label_25

	// *** Basic block 7

.PreprocessorAddUserIncludePath_label_43:
	addi        s4, s1, 112
	mv          a0, s2
	call        NewString

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s4
	call        VectorAppend

	// *** Basic block 9

	j           .PreprocessorAddUserIncludePath_label_35
.func_end_PreprocessorAddUserIncludePath:
	.size PreprocessorAddUserIncludePath, .func_end_PreprocessorAddUserIncludePath-PreprocessorAddUserIncludePath

	.global PreprocessorAddSystemIncludePath
	.type PreprocessorAddSystemIncludePath, @function

PreprocessorAddSystemIncludePath:

	// *** Basic block 0

	.global StringEqual
	.global VectorAppend
	.global NewString
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
	mv          s3, x0
	addi        t0, s1, 136
	ld          s4, 8(t0)
	bge         x0, s4, .PreprocessorAddSystemIncludePath_label_43

	// *** Basic block 1

	ld          s5, 136(s1)

	// *** Basic block 2

.PreprocessorAddSystemIncludePath_label_25:
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          a0, 0(t0)
	mv          a1, s2
	call        StringEqual

	// *** Basic block 3

	beqz        a0, .PreprocessorAddSystemIncludePath_label_38

	// *** Basic block 4

.PreprocessorAddSystemIncludePath_label_35:
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

	// *** Basic block 5

.PreprocessorAddSystemIncludePath_label_38:

	// *** Basic block 6

.PreprocessorAddSystemIncludePath_label_39:
	addi        s3, s3, 1
	bge         s3, s4, .PreprocessorAddSystemIncludePath_label_25

	// *** Basic block 7

.PreprocessorAddSystemIncludePath_label_43:
	addi        s4, s1, 136
	mv          a0, s2
	call        NewString

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s4
	call        VectorAppend

	// *** Basic block 9

	j           .PreprocessorAddSystemIncludePath_label_35
.func_end_PreprocessorAddSystemIncludePath:
	.size PreprocessorAddSystemIncludePath, .func_end_PreprocessorAddSystemIncludePath-PreprocessorAddSystemIncludePath

	.global PreprocessorInsertSystemIncludePath
	.type PreprocessorInsertSystemIncludePath, @function

PreprocessorInsertSystemIncludePath:

	// *** Basic block 0

	.global StringEqual
	.global VectorInsertBefore
	.global NewString
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
	mv          s3, x0
	addi        t0, s1, 136
	ld          s4, 8(t0)
	bge         x0, s4, .PreprocessorInsertSystemIncludePath_label_44

	// *** Basic block 1

	ld          s5, 136(s1)

	// *** Basic block 2

.PreprocessorInsertSystemIncludePath_label_25:
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          a0, 0(t0)
	mv          a1, s2
	call        StringEqual

	// *** Basic block 3

	beqz        a0, .PreprocessorInsertSystemIncludePath_label_39

	// *** Basic block 4

.PreprocessorInsertSystemIncludePath_label_36:
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

	// *** Basic block 5

.PreprocessorInsertSystemIncludePath_label_39:

	// *** Basic block 6

.PreprocessorInsertSystemIncludePath_label_40:
	addi        s3, s3, 1
	bge         s3, s4, .PreprocessorInsertSystemIncludePath_label_25

	// *** Basic block 7

.PreprocessorInsertSystemIncludePath_label_44:
	addi        s4, s1, 136
	mv          a0, s2
	call        NewString

	// *** Basic block 8

	mv          a2, a0
	mv          a1, x0
	mv          a0, s4
	call        VectorInsertBefore

	// *** Basic block 9

	j           .PreprocessorInsertSystemIncludePath_label_36
.func_end_PreprocessorInsertSystemIncludePath:
	.size PreprocessorInsertSystemIncludePath, .func_end_PreprocessorInsertSystemIncludePath-PreprocessorInsertSystemIncludePath

	.global PreprocessorDefineMacro
	.type PreprocessorDefineMacro, @function

PreprocessorDefineMacro:

	// *** Basic block 0

	.global StringInitImmutable
	.local Tokenize
	.global HashTableSearch
	.global StringSetString
	.global NewMacro
	.global NewVector
	.global HashTableInsert
	.global StringDestruct
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -96(s0)
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
	addi        a0, s0, -96
	mv          a1, a2
	call        StringInitImmutable

	// *** Basic block 1

	addi        s3, s0, -96
	addi        s4, s0, -56
	ld          s6, 160(s1)
	bne         s6, x0, .PreprocessorDefineMacro_label_47

	// *** Basic block 2

	mv          s5, x0
	j           .PreprocessorDefineMacro_label_50

	// *** Basic block 3

.PreprocessorDefineMacro_label_47:
	lb          s5, 178(s6)

	// *** Basic block 4

.PreprocessorDefineMacro_label_50:
	slli        t0, s5, 56
	srai        a5, t0, 56
	mv          a6, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a4, t0
	mv          a3, x0
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	call        Tokenize

	// *** Basic block 5

	mv          a1, s2
	mv          a0, s1
	call        HashTableSearch

	// *** Basic block 6

	mv          s3, a0
	beq         s3, x0, .PreprocessorDefineMacro_label_92

	// *** Basic block 7

	addi        a0, s3, 104
	addi        a1, s0, -56
	call        StringSetString

	// *** Basic block 8

	sb          x0, 145(s3)

	// *** Basic block 9

.PreprocessorDefineMacro_label_89:
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

	// *** Basic block 10

.PreprocessorDefineMacro_label_92:
	call        NewVector

	// *** Basic block 11

	addi        a4, s0, -56
	li          t0, -1		// 0xffffffffffffffff
	mv          a5, t0
	mv          a3, a0
	mv          a2, x0
	mv          a1, x0
	mv          a0, s2
	call        NewMacro

	// *** Basic block 12

	mv          s3, a0
	mv          a1, s3
	mv          a0, s1
	call        HashTableInsert

	// *** Basic block 13

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 14

	j           .PreprocessorDefineMacro_label_89
.func_end_PreprocessorDefineMacro:
	.size PreprocessorDefineMacro, .func_end_PreprocessorDefineMacro-PreprocessorDefineMacro

	.global PreprocessorUndefineMacro
	.type PreprocessorUndefineMacro, @function

PreprocessorUndefineMacro:

	// *** Basic block 0

	.global HashTableSearch
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
	ld          a1, 16(t0)
	call        HashTableSearch

	// *** Basic block 1

	mv          s1, a0
	beq         s1, x0, .PreprocessorUndefineMacro_label_28

	// *** Basic block 2

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 145(s1)

	// *** Basic block 3

.PreprocessorUndefineMacro_label_28:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PreprocessorUndefineMacro:
	.size PreprocessorUndefineMacro, .func_end_PreprocessorUndefineMacro-PreprocessorUndefineMacro

	.global PreprocessorError
	.type PreprocessorError, @function

PreprocessorError:

	// *** Basic block 0

	.global VReportError
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	mv          t2, s0
	ld          t3, 160(t1)
	ld          t3, 0(t3)
	ld          a0, 16(t3)
	lw          a1, 80(t3)
	mv          a3, t2
	mv          a2, t0
	j           VReportError
.func_end_PreprocessorError:
	.size PreprocessorError, .func_end_PreprocessorError-PreprocessorError

	.global VPreprocessorError
	.type VPreprocessorError, @function

VPreprocessorError:

	// *** Basic block 0

	.global VReportError
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, a2
	ld          t3, 160(t0)
	ld          t3, 0(t3)
	ld          a0, 16(t3)
	lw          a1, 80(t3)
	mv          a3, t2
	mv          a2, t1
	j           VReportError
.func_end_VPreprocessorError:
	.size VPreprocessorError, .func_end_VPreprocessorError-VPreprocessorError

	.global PreprocessorWarning
	.type PreprocessorWarning, @function

PreprocessorWarning:

	// *** Basic block 0

	.global VReportWarning
	// Leaf procedure, no stack frame generated
	mv          t0, a2
	mv          t1, a0
	mv          t2, a1
	mv          t3, s0
	ld          t4, 160(t1)
	ld          t4, 0(t4)
	ld          a0, 16(t4)
	lw          a1, 80(t4)
	mv          a4, t3
	mv          a3, t0
	mv          a2, t2
	j           VReportWarning
.func_end_PreprocessorWarning:
	.size PreprocessorWarning, .func_end_PreprocessorWarning-PreprocessorWarning

	.global VPreprocessorWarning
	.type VPreprocessorWarning, @function

VPreprocessorWarning:

	// *** Basic block 0

	.global VReportWarning
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, a2
	mv          t3, a3
	ld          t4, 160(t0)
	ld          t4, 0(t4)
	ld          a0, 16(t4)
	lw          a1, 80(t4)
	mv          a4, t3
	mv          a3, t2
	mv          a2, t1
	j           VReportWarning
.func_end_VPreprocessorWarning:
	.size VPreprocessorWarning, .func_end_VPreprocessorWarning-VPreprocessorWarning

	.local  EncodeLength
	.type EncodeLength, @function

EncodeLength:

	// *** Basic block 0

	.global StringAppendChar
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

	// *** Basic block 1

.EncodeLength_label_15:
	andi        t0, s1, -128
	bnez        t0, .EncodeLength_label_31

	// *** Basic block 2

	andi        t0, s1, 127
	slli        t0, t0, 56
	srai        a1, t0, 56
	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           StringAppendChar

	// *** Basic block 4

.EncodeLength_label_31:
	andi        t0, s1, 127
	slli        t0, t0, 56
	srai        t0, t0, 56
	ori         a1, t0, -128
	mv          a0, s2
	call        StringAppendChar

	// *** Basic block 5

	srli        s1, s1, 7

	// *** Basic block 6

.EncodeLength_label_41:
	j           .EncodeLength_label_15
.func_end_EncodeLength:
	.size EncodeLength, .func_end_EncodeLength-EncodeLength

	.local  DecodeLength
	.type DecodeLength, @function

DecodeLength:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a2
	mv          t2, x0
	mv          t3, x0
	ld          t4, 16(a0)

	// *** Basic block 1

.DecodeLength_label_27:
	mv          t5, t0
	addi        t0, t0, 1
	add         t4, t4, t5
	lb          t6, 0(t4)
	andi        t4, t6, 127
	sll         t4, t4, t2
	or          t3, t3, t4
	andi        t4, t6, -128
	bnez        t4, .DecodeLength_label_46

	// *** Basic block 2

	sd          t3, 0(t1)
	mv          a0, t0

	// *** Basic block 3

.DecodeLength_label_43:
	ret         

	// *** Basic block 4

.DecodeLength_label_46:

	// *** Basic block 5

.DecodeLength_label_48:
	j           .DecodeLength_label_27
.func_end_DecodeLength:
	.size DecodeLength, .func_end_DecodeLength-DecodeLength

	.local  FindNextTokenIndex
	.type FindNextTokenIndex, @function

FindNextTokenIndex:

	// *** Basic block 0

	.local DecodeLength
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	sd          x0, -32(s0)
	ld          s1, 24(a0)
	ld          s2, 8(a0)
	ld          t0, 24(s2)
	blt         s1, t0, .FindNextTokenIndex_label_33

	// *** Basic block 1

	mv          a0, s1

	// *** Basic block 2

.FindNextTokenIndex_label_30:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.FindNextTokenIndex_label_33:
	ld          t0, 16(s2)
	add         t0, t0, s1
	lb          t1, 0(t0)
	addi        t0, t1, -1
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 4

	j           .FindNextTokenIndex_label_67

	// *** Basic block 5

	j           .FindNextTokenIndex_label_88

	// *** Basic block 6

	j           .FindNextTokenIndex_label_68

	// *** Basic block 7

	j           .FindNextTokenIndex_label_69

	// *** Basic block 8

	j           .FindNextTokenIndex_label_70

	// *** Basic block 9

	j           .FindNextTokenIndex_label_71

	// *** Basic block 10

	j           .FindNextTokenIndex_label_72

	// *** Basic block 11

	j           .FindNextTokenIndex_label_73

	// *** Basic block 12

	j           .FindNextTokenIndex_label_74

	// *** Basic block 13

	j           .FindNextTokenIndex_label_92

	// *** Basic block 14

	j           .FindNextTokenIndex_label_93

	// *** Basic block 15

	j           .FindNextTokenIndex_label_94

	// *** Basic block 16

	j           .FindNextTokenIndex_label_95

	// *** Basic block 17

	j           .FindNextTokenIndex_label_96

	// *** Basic block 18

	j           .FindNextTokenIndex_label_97

	// *** Basic block 19

	j           .FindNextTokenIndex_label_76

	// *** Basic block 20

	j           .FindNextTokenIndex_label_98

	// *** Basic block 21

	j           .FindNextTokenIndex_label_75

	// *** Basic block 22

	j           .FindNextTokenIndex_label_102

	// *** Basic block 23

.FindNextTokenIndex_label_67:

	// *** Basic block 24

.FindNextTokenIndex_label_68:

	// *** Basic block 25

.FindNextTokenIndex_label_69:

	// *** Basic block 26

.FindNextTokenIndex_label_70:

	// *** Basic block 27

.FindNextTokenIndex_label_71:

	// *** Basic block 28

.FindNextTokenIndex_label_72:

	// *** Basic block 29

.FindNextTokenIndex_label_73:

	// *** Basic block 30

.FindNextTokenIndex_label_74:

	// *** Basic block 31

.FindNextTokenIndex_label_75:

	// *** Basic block 32

.FindNextTokenIndex_label_76:
	addi        a1, s1, 1
	addi        a2, s0, -32
	mv          a0, s2
	call        DecodeLength

	// *** Basic block 33

	mv          s1, a0
	j           .FindNextTokenIndex_label_104

	// *** Basic block 34

.FindNextTokenIndex_label_88:
	li          t0, 2		// 0x2 ASCII \x2
	sd          t0, -32(s0)
	j           .FindNextTokenIndex_label_104

	// *** Basic block 35

.FindNextTokenIndex_label_92:

	// *** Basic block 36

.FindNextTokenIndex_label_93:

	// *** Basic block 37

.FindNextTokenIndex_label_94:

	// *** Basic block 38

.FindNextTokenIndex_label_95:

	// *** Basic block 39

.FindNextTokenIndex_label_96:

	// *** Basic block 40

.FindNextTokenIndex_label_97:

	// *** Basic block 41

.FindNextTokenIndex_label_98:
	li          t0, 1		// 0x1 ASCII \x1
	sd          t0, -32(s0)
	j           .FindNextTokenIndex_label_104

	// *** Basic block 42

.FindNextTokenIndex_label_102:
	j           .FindNextTokenIndex_label_104

	// *** Basic block 43

.FindNextTokenIndex_label_104:
	ld          t0, -32(s0)
	add         a0, s1, t0
	j           .FindNextTokenIndex_label_30
.func_end_FindNextTokenIndex:
	.size FindNextTokenIndex, .func_end_FindNextTokenIndex-FindNextTokenIndex

	.local  TokenIteratorInit
	.type TokenIteratorInit, @function

TokenIteratorInit:

	// *** Basic block 0

	.local FindNextTokenIndex
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	sd          a1, 0(t0)
	sd          a2, 8(t0)
	sd          x0, 16(t0)
	sd          x0, 24(t0)
	j           FindNextTokenIndex
.func_end_TokenIteratorInit:
	.size TokenIteratorInit, .func_end_TokenIteratorInit-TokenIteratorInit

	.local  CurrentToken
	.type CurrentToken, @function

CurrentToken:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 24(a0)
	ld          t1, 8(a0)
	ld          t2, 24(t1)
	blt         t0, t2, .CurrentToken_label_25

	// *** Basic block 1

	li          a0, 19		// 0x13 ASCII \x13

	// *** Basic block 2

.CurrentToken_label_22:
	ret         

	// *** Basic block 3

.CurrentToken_label_25:
	ld          t1, 16(t1)
	add         t0, t1, t0
	lb          a0, 0(t0)
	ret         
.func_end_CurrentToken:
	.size CurrentToken, .func_end_CurrentToken-CurrentToken

	.local  PrevToken
	.type PrevToken, @function

PrevToken:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 16(a0)
	ld          t1, 8(a0)
	ld          t2, 24(t1)
	blt         t0, t2, .PrevToken_label_25

	// *** Basic block 1

	li          a0, 19		// 0x13 ASCII \x13

	// *** Basic block 2

.PrevToken_label_22:
	ret         

	// *** Basic block 3

.PrevToken_label_25:
	ld          t1, 16(t1)
	add         t0, t1, t0
	lb          a0, 0(t0)
	ret         
.func_end_PrevToken:
	.size PrevToken, .func_end_PrevToken-PrevToken

	.local  NextToken
	.type NextToken, @function

NextToken:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 32(a0)
	ld          t1, 8(a0)
	ld          t2, 24(t1)
	blt         t0, t2, .NextToken_label_26

	// *** Basic block 1

	li          a0, 19		// 0x13 ASCII \x13

	// *** Basic block 2

.NextToken_label_23:
	ret         

	// *** Basic block 3

.NextToken_label_26:
	ld          t1, 16(t1)
	add         t0, t1, t0
	lb          a0, 0(t0)
	ret         
.func_end_NextToken:
	.size NextToken, .func_end_NextToken-NextToken

	.local  AppendCurrentToken
	.type AppendCurrentToken, @function

AppendCurrentToken:

	// *** Basic block 0

	.global StringAppendSegment
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	ld          t2, 32(t0)
	ld          t3, 24(t0)
	sub         t4, t2, t3
	ld          t2, 8(t0)
	ld          t2, 16(t2)
	add         a1, t2, t3
	mv          a2, t4
	mv          a0, t1
	j           StringAppendSegment
.func_end_AppendCurrentToken:
	.size AppendCurrentToken, .func_end_AppendCurrentToken-AppendCurrentToken

	.local  AppendTokenSpelling
	.type AppendTokenSpelling, @function

AppendTokenSpelling:

	// *** Basic block 0

	.local DecodeLength
	.global StringAppendSegment
	.global StringAppendChar
	.global StringAppend
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
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	ld          s4, 16(s1)
	add         t0, s4, s2
	lb          t0, 0(t0)
	addi        t0, t0, -1
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .AppendTokenSpelling_label_59

	// *** Basic block 2

	j           .AppendTokenSpelling_label_90

	// *** Basic block 3

	j           .AppendTokenSpelling_label_60

	// *** Basic block 4

	j           .AppendTokenSpelling_label_61

	// *** Basic block 5

	j           .AppendTokenSpelling_label_62

	// *** Basic block 6

	j           .AppendTokenSpelling_label_64

	// *** Basic block 7

	j           .AppendTokenSpelling_label_65

	// *** Basic block 8

	j           .AppendTokenSpelling_label_66

	// *** Basic block 9

	j           .AppendTokenSpelling_label_68

	// *** Basic block 10

	j           .AppendTokenSpelling_label_99

	// *** Basic block 11

	j           .AppendTokenSpelling_label_107

	// *** Basic block 12

	j           .AppendTokenSpelling_label_115

	// *** Basic block 13

	j           .AppendTokenSpelling_label_117

	// *** Basic block 14

	j           .AppendTokenSpelling_label_125

	// *** Basic block 15

	j           .AppendTokenSpelling_label_133

	// *** Basic block 16

	j           .AppendTokenSpelling_label_63

	// *** Basic block 17

	j           .AppendTokenSpelling_label_141

	// *** Basic block 18

	j           .AppendTokenSpelling_label_67

	// *** Basic block 19

	j           .AppendTokenSpelling_label_149

	// *** Basic block 20

.AppendTokenSpelling_label_59:

	// *** Basic block 21

.AppendTokenSpelling_label_60:

	// *** Basic block 22

.AppendTokenSpelling_label_61:

	// *** Basic block 23

.AppendTokenSpelling_label_62:

	// *** Basic block 24

.AppendTokenSpelling_label_63:

	// *** Basic block 25

.AppendTokenSpelling_label_64:

	// *** Basic block 26

.AppendTokenSpelling_label_65:

	// *** Basic block 27

.AppendTokenSpelling_label_66:

	// *** Basic block 28

.AppendTokenSpelling_label_67:

	// *** Basic block 29

.AppendTokenSpelling_label_68:
	sd          x0, -32(s0)
	addi        a1, s2, 1
	addi        a2, s0, -32
	mv          a0, s1
	call        DecodeLength

	// *** Basic block 30

	mv          s2, a0
	ld          t0, 16(s1)
	add         a1, t0, s2
	ld          a2, -32(s0)
	mv          a0, s3
	call        StringAppendSegment

	// *** Basic block 31

	j           .AppendTokenSpelling_label_151

	// *** Basic block 32

.AppendTokenSpelling_label_90:
	addi        t0, s2, 1
	add         t0, s4, t0
	lb          a1, 0(t0)
	mv          a0, s3
	call        StringAppendChar

	// *** Basic block 33

	j           .AppendTokenSpelling_label_151

	// *** Basic block 34

.AppendTokenSpelling_label_99:
	li          t0, 35		// 0x23 ASCII '#'
	mv          a1, t0
	mv          a0, s3
	call        StringAppendChar

	// *** Basic block 35

	j           .AppendTokenSpelling_label_151

	// *** Basic block 36

.AppendTokenSpelling_label_107:
	lla         a1, .str.105
	mv          a0, s3
	call        StringAppend

	// *** Basic block 37

	j           .AppendTokenSpelling_label_151

	// *** Basic block 38

.AppendTokenSpelling_label_115:
	j           .AppendTokenSpelling_label_151

	// *** Basic block 39

.AppendTokenSpelling_label_117:
	li          t0, 40		// 0x28 ASCII '('
	mv          a1, t0
	mv          a0, s3
	call        StringAppendChar

	// *** Basic block 40

	j           .AppendTokenSpelling_label_151

	// *** Basic block 41

.AppendTokenSpelling_label_125:
	li          t0, 41		// 0x29 ASCII ')'
	mv          a1, t0
	mv          a0, s3
	call        StringAppendChar

	// *** Basic block 42

	j           .AppendTokenSpelling_label_151

	// *** Basic block 43

.AppendTokenSpelling_label_133:
	li          t0, 44		// 0x2c ASCII ','
	mv          a1, t0
	mv          a0, s3
	call        StringAppendChar

	// *** Basic block 44

	j           .AppendTokenSpelling_label_151

	// *** Basic block 45

.AppendTokenSpelling_label_141:
	li          t0, 32		// 0x20 ASCII ' '
	mv          a1, t0
	mv          a0, s3
	call        StringAppendChar

	// *** Basic block 46

	j           .AppendTokenSpelling_label_151

	// *** Basic block 47

.AppendTokenSpelling_label_149:
	j           .AppendTokenSpelling_label_151

	// *** Basic block 48

.AppendTokenSpelling_label_151:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AppendTokenSpelling:
	.size AppendTokenSpelling, .func_end_AppendTokenSpelling-AppendTokenSpelling

	.local  AppendCurrentTokenSpelling
	.type AppendCurrentTokenSpelling, @function

AppendCurrentTokenSpelling:

	// *** Basic block 0

	.local AppendTokenSpelling
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	ld          a0, 8(t0)
	ld          a1, 24(t0)
	mv          a2, t1
	j           AppendTokenSpelling
.func_end_AppendCurrentTokenSpelling:
	.size AppendCurrentTokenSpelling, .func_end_AppendCurrentTokenSpelling-AppendCurrentTokenSpelling

	.local  GetCurrentTokenSpelling
	.type GetCurrentTokenSpelling, @function

GetCurrentTokenSpelling:

	// *** Basic block 0

	.global StringInit
	.local AppendCurrentTokenSpelling
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
	mv          a1, x0
	mv          a0, s1
	call        StringInit

	// *** Basic block 1

	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AppendCurrentTokenSpelling
.func_end_GetCurrentTokenSpelling:
	.size GetCurrentTokenSpelling, .func_end_GetCurrentTokenSpelling-GetCurrentTokenSpelling

	.local  IsSpaceToken
	.type IsSpaceToken, @function

IsSpaceToken:

	// *** Basic block 0

	.local CurrentToken
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	call        CurrentToken

	// *** Basic block 1

	mv          s1, a0
	addi        t0, s1, -17
	seqz        a0, t0
	li          t0, 17		// 0x11 ASCII \x11
	beq         s1, t0, .IsSpaceToken_label_26

	// *** Basic block 2

	addi        t0, s1, -16
	seqz        a0, t0

	// *** Basic block 3

.IsSpaceToken_label_26:

	// *** Basic block 4

.IsSpaceToken_label_28:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_IsSpaceToken:
	.size IsSpaceToken, .func_end_IsSpaceToken-IsSpaceToken

	.local  MoveToNextToken
	.type MoveToNextToken, @function

MoveToNextToken:

	// *** Basic block 0

	.local IsSpaceToken
	.local FindNextTokenIndex
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
	call        IsSpaceToken

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .MoveToNextToken_label_20

	// *** Basic block 2

	ld          t0, 24(s1)
	sd          t0, 16(s1)

	// *** Basic block 3

.MoveToNextToken_label_20:
	ld          t0, 32(s1)
	sd          t0, 24(s1)
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           FindNextTokenIndex
.func_end_MoveToNextToken:
	.size MoveToNextToken, .func_end_MoveToNextToken-MoveToNextToken

	.local  SkipIntegerSuffix
	.type SkipIntegerSuffix, @function

SkipIntegerSuffix:

	// *** Basic block 0

	.global toupper
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
	mv          t0, a0
	mv          s1, a1
	ld          s2, 16(t0)
	add         t1, s2, s1
	lb          a0, 0(t1)
	call        toupper

	// *** Basic block 1

	mv          s3, a0
	mv          s4, x0
	li          s5, 85		// 0x55 ASCII 'U'
	bne         s3, s5, .SkipIntegerSuffix_label_37

	// *** Basic block 2

	addi        s1, s1, 1
	li          s4, 1		// 0x1 ASCII \x1

	// *** Basic block 3

.SkipIntegerSuffix_label_37:
	add         t0, s2, s1
	lb          a0, 0(t0)
	call        toupper

	// *** Basic block 4

	mv          s3, a0
	li          s6, 76		// 0x4c ASCII 'L'
	bne         s3, s6, .SkipIntegerSuffix_label_51

	// *** Basic block 5

	addi        s1, s1, 1

	// *** Basic block 6

.SkipIntegerSuffix_label_51:
	add         t0, s2, s1
	lb          a0, 0(t0)
	call        toupper

	// *** Basic block 7

	mv          s3, a0
	bne         s3, s6, .SkipIntegerSuffix_label_64

	// *** Basic block 8

	addi        s1, s1, 1

	// *** Basic block 9

.SkipIntegerSuffix_label_64:
	not         t0, s4
	beqz        t0, .SkipIntegerSuffix_label_80

	// *** Basic block 10

	add         t0, s2, s1
	lb          a0, 0(t0)
	call        toupper

	// *** Basic block 11

	mv          s3, a0
	bne         s3, s5, .SkipIntegerSuffix_label_79

	// *** Basic block 12

	addi        s1, s1, 1

	// *** Basic block 13

.SkipIntegerSuffix_label_79:

	// *** Basic block 14

.SkipIntegerSuffix_label_80:
	mv          a0, s1

	// *** Basic block 15

.SkipIntegerSuffix_label_83:
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
.func_end_SkipIntegerSuffix:
	.size SkipIntegerSuffix, .func_end_SkipIntegerSuffix-SkipIntegerSuffix

	.local  SkipFloatingSuffix
	.type SkipFloatingSuffix, @function

SkipFloatingSuffix:

	// *** Basic block 0

	.global toupper
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
	mv          t0, a0
	mv          s1, a1
	ld          t1, 16(t0)
	add         t1, t1, s1
	lb          s2, 0(t1)
	mv          a0, s2
	call        toupper

	// *** Basic block 1

	mv          s3, a0
	li          t0, 70		// 0x46 ASCII 'F'
	bne         s3, t0, .SkipFloatingSuffix_label_35

	// *** Basic block 2

	addi        s1, s1, 1
	j           .SkipFloatingSuffix_label_49

	// *** Basic block 3

.SkipFloatingSuffix_label_35:
	mv          a0, s2
	call        toupper

	// *** Basic block 4

	mv          s3, a0
	li          t0, 76		// 0x4c ASCII 'L'
	bne         s3, t0, .SkipFloatingSuffix_label_48

	// *** Basic block 5

	addi        s1, s1, 1

	// *** Basic block 6

.SkipFloatingSuffix_label_48:

	// *** Basic block 7

.SkipFloatingSuffix_label_49:
	mv          a0, s1

	// *** Basic block 8

.SkipFloatingSuffix_label_52:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SkipFloatingSuffix:
	.size SkipFloatingSuffix, .func_end_SkipFloatingSuffix-SkipFloatingSuffix

	.local  SkipNumber
	.type SkipNumber, @function

SkipNumber:

	// *** Basic block 0

	.global isdigit
	.global toupper
	.local SkipFloatingSuffix
	.local SkipIntegerSuffix
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
	mv          s3, x0
	mv          s4, x0
	mv          s5, x0
	ld          s6, 24(s2)
	bge         s1, s6, .SkipNumber_label_102

	// *** Basic block 1

	ld          t0, 16(s2)

	// *** Basic block 2

.SkipNumber_label_37:
	add         t0, t0, s1
	lb          s7, 0(t0)
	li          t0, 46		// 0x2e ASCII '.'
	bne         s7, t0, .SkipNumber_label_50

	// *** Basic block 3

	bnez        s4, .SkipNumber_label_102

	// *** Basic block 4

.SkipNumber_label_47:
	li          s4, 1		// 0x1 ASCII \x1
	j           .SkipNumber_label_98

	// *** Basic block 5

.SkipNumber_label_50:
	addi        t1, s7, -101
	seqz        t0, t1
	li          t1, 101		// 0x65 ASCII 'e'
	beq         s7, t1, .SkipNumber_label_60

	// *** Basic block 6

	addi        t1, s7, -69
	seqz        t0, t1

	// *** Basic block 7

.SkipNumber_label_60:
	beqz        t0, .SkipNumber_label_66

	// *** Basic block 8

	bnez        s3, .SkipNumber_label_102

	// *** Basic block 9

.SkipNumber_label_63:
	li          s3, 1		// 0x1 ASCII \x1
	j           .SkipNumber_label_97

	// *** Basic block 10

.SkipNumber_label_66:
	addi        t1, s7, -43
	seqz        t0, t1
	li          t1, 43		// 0x2b ASCII '+'
	beq         s7, t1, .SkipNumber_label_76

	// *** Basic block 11

	addi        t1, s7, -45
	seqz        t0, t1

	// *** Basic block 12

.SkipNumber_label_76:
	beqz        t0, .SkipNumber_label_87

	// *** Basic block 13

	not         t0, s3
	bnez        t0, .SkipNumber_label_82

	// *** Basic block 14

	mv          t0, s5

	// *** Basic block 15

.SkipNumber_label_82:
	bnez        t0, .SkipNumber_label_102

	// *** Basic block 16

.SkipNumber_label_84:
	li          s5, 1		// 0x1 ASCII \x1
	j           .SkipNumber_label_96

	// *** Basic block 17

.SkipNumber_label_87:
	mv          a0, s7
	call        isdigit

	// *** Basic block 18

	slli        t0, a0, 56
	srai        t0, t0, 56
	not         t0, t0
	bnez        t0, .SkipNumber_label_102

	// *** Basic block 19

.SkipNumber_label_95:

	// *** Basic block 20

.SkipNumber_label_96:

	// *** Basic block 21

.SkipNumber_label_97:

	// *** Basic block 22

.SkipNumber_label_98:
	addi        s1, s1, 1
	blt         s1, s6, .SkipNumber_label_37

	// *** Basic block 23

.SkipNumber_label_102:
	mv          s6, s4
	bnez        s4, .SkipNumber_label_108

	// *** Basic block 24

	mv          s6, s3

	// *** Basic block 25

.SkipNumber_label_108:
	bnez        s6, .SkipNumber_label_119

	// *** Basic block 26

	ld          t0, 16(s2)
	add         t0, t0, s1
	lb          a0, 0(t0)
	call        toupper

	// *** Basic block 27

	addi        t0, a0, -70
	seqz        s6, t0

	// *** Basic block 28

.SkipNumber_label_119:
	beqz        s6, .SkipNumber_label_128

	// *** Basic block 29

	mv          a1, s1
	mv          a0, s2
	call        SkipFloatingSuffix

	// *** Basic block 30

	j           .SkipNumber_label_134

	// *** Basic block 31

.SkipNumber_label_128:
	mv          a1, s1
	mv          a0, s2
	call        SkipIntegerSuffix

	// *** Basic block 32

.SkipNumber_label_134:
	mv          a0, s1

	// *** Basic block 33

.SkipNumber_label_137:
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
.func_end_SkipNumber:
	.size SkipNumber, .func_end_SkipNumber-SkipNumber

	.local  AppendNumber
	.type AppendNumber, @function

AppendNumber:

	// *** Basic block 0

	.local SkipNumber
	.local EncodeLength
	.global StringAppendSegment
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
	mv          s2, a0
	mv          s3, a1
	mv          s4, s1
	mv          a1, s1
	call        SkipNumber

	// *** Basic block 1

	mv          s1, a0
	sub         s5, s1, s4
	mv          a1, s5
	mv          a0, s3
	call        EncodeLength

	// *** Basic block 2

	ld          t0, 16(s2)
	add         a1, t0, s4
	mv          a2, s5
	mv          a0, s3
	call        StringAppendSegment

	// *** Basic block 3

	mv          a0, s1

	// *** Basic block 4

.AppendNumber_label_43:
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
.func_end_AppendNumber:
	.size AppendNumber, .func_end_AppendNumber-AppendNumber

	.local  AppendStringLiteral
	.type AppendStringLiteral, @function

AppendStringLiteral:

	// *** Basic block 0

	.local EncodeLength
	.global StringAppendSegment
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
	mv          s2, a0
	mv          s3, a1
	mv          s4, s1
	ld          s5, 24(s2)
	bge         s1, s5, .AppendStringLiteral_label_48

	// *** Basic block 1

	ld          t0, 16(s2)

	// *** Basic block 2

.AppendStringLiteral_label_28:
	add         t0, t0, s1
	lb          s6, 0(t0)
	li          t0, 34		// 0x22 ASCII '"'
	beq         s6, t0, .AppendStringLiteral_label_48

	// *** Basic block 3

.AppendStringLiteral_label_37:
	li          t0, 92		// 0x5c ASCII '\'
	bne         s6, t0, .AppendStringLiteral_label_44

	// *** Basic block 4

	addi        s1, s1, 1

	// *** Basic block 5

.AppendStringLiteral_label_44:
	addi        s1, s1, 1
	blt         s1, s5, .AppendStringLiteral_label_28

	// *** Basic block 6

.AppendStringLiteral_label_48:
	sub         s5, s1, s4
	mv          a1, s5
	mv          a0, s3
	call        EncodeLength

	// *** Basic block 7

	ld          t0, 16(s2)
	add         a1, t0, s4
	mv          a2, s5
	mv          a0, s3
	call        StringAppendSegment

	// *** Basic block 8

	addi        a0, s1, 1

	// *** Basic block 9

.AppendStringLiteral_label_67:
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
.func_end_AppendStringLiteral:
	.size AppendStringLiteral, .func_end_AppendStringLiteral-AppendStringLiteral

	.local  AppendCharLiteral
	.type AppendCharLiteral, @function

AppendCharLiteral:

	// *** Basic block 0

	.local EncodeLength
	.global StringAppendSegment
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
	mv          s2, a0
	mv          s3, a1
	mv          s4, s1
	ld          s5, 24(s2)
	bge         s1, s5, .AppendCharLiteral_label_48

	// *** Basic block 1

	ld          t0, 16(s2)

	// *** Basic block 2

.AppendCharLiteral_label_28:
	add         t0, t0, s1
	lb          s6, 0(t0)
	li          t0, 39		// 0x27 ASCII '''
	beq         s6, t0, .AppendCharLiteral_label_48

	// *** Basic block 3

.AppendCharLiteral_label_37:
	li          t0, 92		// 0x5c ASCII '\'
	bne         s6, t0, .AppendCharLiteral_label_44

	// *** Basic block 4

	addi        s1, s1, 1

	// *** Basic block 5

.AppendCharLiteral_label_44:
	addi        s1, s1, 1
	blt         s1, s5, .AppendCharLiteral_label_28

	// *** Basic block 6

.AppendCharLiteral_label_48:
	sub         s5, s1, s4
	mv          a1, s5
	mv          a0, s3
	call        EncodeLength

	// *** Basic block 7

	ld          t0, 16(s2)
	add         a1, t0, s4
	mv          a2, s5
	mv          a0, s3
	call        StringAppendSegment

	// *** Basic block 8

	addi        a0, s1, 1

	// *** Basic block 9

.AppendCharLiteral_label_67:
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
.func_end_AppendCharLiteral:
	.size AppendCharLiteral, .func_end_AppendCharLiteral-AppendCharLiteral

	.local  CanStartToken
	.type CanStartToken, @function

CanStartToken:

	// *** Basic block 0

	.global isspace
	.global isalnum
	.global isdigit
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
	mv          s1, a1
	ld          s2, 16(t0)
	add         t1, s2, s1
	lb          s3, 0(t1)
	mv          a0, s3
	call        isspace

	// *** Basic block 1

	mv          s4, a0
	bnez        a0, .CanStartToken_label_44

	// *** Basic block 2

	mv          a0, s3
	call        isalnum

	// *** Basic block 3

	mv          s4, a0

	// *** Basic block 4

.CanStartToken_label_44:
	bnez        s4, .CanStartToken_label_49

	// *** Basic block 5

	addi        t0, s3, -95
	seqz        s4, t0

	// *** Basic block 6

.CanStartToken_label_49:
	bnez        s4, .CanStartToken_label_54

	// *** Basic block 7

	addi        t0, s3, -34
	seqz        s4, t0

	// *** Basic block 8

.CanStartToken_label_54:
	bnez        s4, .CanStartToken_label_59

	// *** Basic block 9

	addi        t0, s3, -39
	seqz        s4, t0

	// *** Basic block 10

.CanStartToken_label_59:
	bnez        s4, .CanStartToken_label_64

	// *** Basic block 11

	addi        t0, s3, -35
	seqz        s4, t0

	// *** Basic block 12

.CanStartToken_label_64:
	bnez        s4, .CanStartToken_label_69

	// *** Basic block 13

	addi        t0, s3, -40
	seqz        s4, t0

	// *** Basic block 14

.CanStartToken_label_69:
	bnez        s4, .CanStartToken_label_74

	// *** Basic block 15

	addi        t0, s3, -41
	seqz        s4, t0

	// *** Basic block 16

.CanStartToken_label_74:
	beqz        s4, .CanStartToken_label_81

	// *** Basic block 17

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 18

.CanStartToken_label_78:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 19

.CanStartToken_label_81:
	li          t0, 46		// 0x2e ASCII '.'
	bne         s3, t0, .CanStartToken_label_97

	// *** Basic block 20

	addi        t0, s1, 1
	add         t0, s2, t0
	lb          a0, 0(t0)
	call        isdigit

	// *** Basic block 21

	beqz        a0, .CanStartToken_label_96

	// *** Basic block 22

	li          a0, 1		// 0x1 ASCII \x1
	j           .CanStartToken_label_78

	// *** Basic block 23

.CanStartToken_label_96:

	// *** Basic block 24

.CanStartToken_label_97:
	mv          a0, x0
	j           .CanStartToken_label_78
.func_end_CanStartToken:
	.size CanStartToken, .func_end_CanStartToken-CanStartToken

	.local  AppendOtherToken
	.type AppendOtherToken, @function

AppendOtherToken:

	// *** Basic block 0

	.local CanStartToken
	.global StringAppendChar
	.local EncodeLength
	.global StringAppendSegment
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
	mv          s2, a0
	mv          s3, a1
	mv          s4, s1
	ld          s5, 24(s2)
	bge         s1, s5, .AppendOtherToken_label_39

	// *** Basic block 1

.AppendOtherToken_label_28:
	mv          a1, s1
	mv          a0, s2
	call        CanStartToken

	// *** Basic block 2

	bnez        a0, .AppendOtherToken_label_39

	// *** Basic block 3

.AppendOtherToken_label_35:
	addi        s1, s1, 1
	blt         s1, s5, .AppendOtherToken_label_28

	// *** Basic block 4

.AppendOtherToken_label_39:
	sub         s5, s1, s4
	li          t0, 1		// 0x1 ASCII \x1
	bne         s5, t0, .AppendOtherToken_label_66

	// *** Basic block 5

	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s3
	call        StringAppendChar

	// *** Basic block 6

	ld          t0, 16(s2)
	add         t0, t0, s4
	lb          a1, 0(t0)
	mv          a0, s3
	call        StringAppendChar

	// *** Basic block 7

	mv          a0, s1

	// *** Basic block 8

.AppendOtherToken_label_63:
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

	// *** Basic block 9

.AppendOtherToken_label_66:
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s3
	call        StringAppendChar

	// *** Basic block 10

	mv          a1, s5
	mv          a0, s3
	call        EncodeLength

	// *** Basic block 11

	ld          t0, 16(s2)
	add         a1, t0, s4
	mv          a2, s5
	mv          a0, s3
	call        StringAppendSegment

	// *** Basic block 12

	mv          a0, s1
	j           .AppendOtherToken_label_63
.func_end_AppendOtherToken:
	.size AppendOtherToken, .func_end_AppendOtherToken-AppendOtherToken

	.local  SkipSpacesAndCommentsInLine
	.type SkipSpacesAndCommentsInLine, @function

SkipSpacesAndCommentsInLine:

	// *** Basic block 0

	.global isspace
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
	mv          t0, a0
	ld          s2, 24(t0)
	bge         s1, s2, .SkipSpacesAndCommentsInLine_label_99

	// *** Basic block 1

	ld          s3, 16(t0)
	addi        s4, s2, -1

	// *** Basic block 2

.SkipSpacesAndCommentsInLine_label_26:
	add         t0, s3, s1
	lb          s5, 0(t0)
	li          t0, 47		// 0x2f ASCII '/'
	bne         s5, t0, .SkipSpacesAndCommentsInLine_label_87

	// *** Basic block 3

	slt         t0, s1, s2
	bge         s1, s2, .SkipSpacesAndCommentsInLine_label_43

	// *** Basic block 4

	addi        t1, s1, 1
	add         t1, s3, t1
	lb          t1, 0(t1)
	addi        t1, t1, -47
	seqz        t0, t1

	// *** Basic block 5

.SkipSpacesAndCommentsInLine_label_43:
	bnez        t0, .SkipSpacesAndCommentsInLine_label_99

	// *** Basic block 6

.SkipSpacesAndCommentsInLine_label_45:
	slt         t0, s1, s4
	bge         s1, s4, .SkipSpacesAndCommentsInLine_label_55

	// *** Basic block 7

	addi        t1, s1, 1
	add         t1, s3, t1
	lb          t1, 0(t1)
	addi        t1, t1, -42
	seqz        t0, t1

	// *** Basic block 8

.SkipSpacesAndCommentsInLine_label_55:
	beqz        t0, .SkipSpacesAndCommentsInLine_label_85

	// *** Basic block 9

	addi        s1, s1, 2

	// *** Basic block 10

.SkipSpacesAndCommentsInLine_label_58:

	// *** Basic block 11

.SkipSpacesAndCommentsInLine_label_59:
	addi        s1, s1, 1
	add         t0, s3, s1
	lb          s5, 0(t0)

	// *** Basic block 12

.SkipSpacesAndCommentsInLine_label_63:
	slt         t0, s1, s2
	bge         s1, s2, .SkipSpacesAndCommentsInLine_label_69

	// *** Basic block 13

	addi        t1, s5, -42
	snez        t0, t1

	// *** Basic block 14

.SkipSpacesAndCommentsInLine_label_69:
	bnez        t0, .SkipSpacesAndCommentsInLine_label_59

	// *** Basic block 15

.SkipSpacesAndCommentsInLine_label_71:
	addi        s1, s1, 1
	add         t0, s3, s1
	lb          s5, 0(t0)

	// *** Basic block 16

.SkipSpacesAndCommentsInLine_label_75:
	slt         t0, s1, s2
	bge         s1, s2, .SkipSpacesAndCommentsInLine_label_81

	// *** Basic block 17

	addi        t1, s5, -47
	snez        t0, t1

	// *** Basic block 18

.SkipSpacesAndCommentsInLine_label_81:
	bnez        t0, .SkipSpacesAndCommentsInLine_label_58

	// *** Basic block 19

.SkipSpacesAndCommentsInLine_label_83:
	j           .SkipSpacesAndCommentsInLine_label_26

	// *** Basic block 20

.SkipSpacesAndCommentsInLine_label_85:

	// *** Basic block 21

.SkipSpacesAndCommentsInLine_label_86:

	// *** Basic block 22

.SkipSpacesAndCommentsInLine_label_87:
	mv          a0, s5
	call        isspace

	// *** Basic block 23

	slli        t0, a0, 56
	srai        t0, t0, 56
	not         t0, t0
	bnez        t0, .SkipSpacesAndCommentsInLine_label_99

	// *** Basic block 24

.SkipSpacesAndCommentsInLine_label_95:
	addi        s1, s1, 1
	blt         s1, s2, .SkipSpacesAndCommentsInLine_label_26

	// *** Basic block 25

.SkipSpacesAndCommentsInLine_label_99:
	mv          a0, s1

	// *** Basic block 26

.SkipSpacesAndCommentsInLine_label_102:
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
.func_end_SkipSpacesAndCommentsInLine:
	.size SkipSpacesAndCommentsInLine, .func_end_SkipSpacesAndCommentsInLine-SkipSpacesAndCommentsInLine

	.local  GetNextCommentChar
	.type GetNextCommentChar, @function

GetNextCommentChar:

	// *** Basic block 0

	.global LexEof
	.global SourceReadLine
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
	mv          t0, a0
	mv          s1, a2
	mv          s2, a1
	ld          s4, 160(t0)
	mv          a0, s4
	call        LexEof

	// *** Basic block 1

	ld          s5, 0(s4)
	ld          t0, 0(s1)
	ld          t1, 24(s2)
	sub         t0, t0, t1
	seqz        s6, t0
	not         s3, a0
	beqz        s3, .GetNextCommentChar_label_40

	// *** Basic block 2

	ld          t0, 0(s1)
	ld          t1, 24(s2)
	sub         t0, t0, t1
	seqz        s3, t0

	// *** Basic block 3

.GetNextCommentChar_label_40:
	beqz        s3, .GetNextCommentChar_label_56

	// *** Basic block 4

.GetNextCommentChar_label_42:
	mv          a1, s2
	mv          a0, s5
	call        SourceReadLine

	// *** Basic block 5

	mv          a0, s4
	call        LexEof

	// *** Basic block 6

	not         s6, a0
	beqz        s6, .GetNextCommentChar_label_54

	// *** Basic block 7

.GetNextCommentChar_label_54:
	bnez        s6, .GetNextCommentChar_label_42

	// *** Basic block 8

.GetNextCommentChar_label_56:
	mv          a0, s4
	call        LexEof

	// *** Basic block 9

	beqz        a0, .GetNextCommentChar_label_66

	// *** Basic block 10

	mv          a0, x0

	// *** Basic block 11

.GetNextCommentChar_label_63:
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

	// *** Basic block 12

.GetNextCommentChar_label_66:
	ld          t0, 0(s1)
	addi        t1, t0, 1
	sd          t1, 0(s1)
	ld          t1, 16(s2)
	add         t0, t1, t0
	lb          a0, 0(t0)
	j           .GetNextCommentChar_label_63
.func_end_GetNextCommentChar:
	.size GetNextCommentChar, .func_end_GetNextCommentChar-GetNextCommentChar

	.local  HandleSpacesAndComments
	.type HandleSpacesAndComments, @function

HandleSpacesAndComments:

	// *** Basic block 0

	.local GetNextCommentChar
	.global LexEof
	.global StringAppendChar
	.local EncodeLength
	.global StringAppendSegment
	.global isspace
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Saved argument registers.
	sd a3, -24(s0)
	// Local vars at offset -32(s0)
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
	mv          s1, a1
	mv          s2, a0
	mv          s3, a2
	ld          t0, -24(s0)
	ld          t1, 24(s1)
	bge         t0, t1, .HandleSpacesAndComments_label_197

	// *** Basic block 1

	ld          s4, 160(s2)

	// *** Basic block 2

.HandleSpacesAndComments_label_39:
	ld          s5, 16(s1)
	ld          s6, -24(s0)
	add         t0, s5, s6
	lb          s7, 0(t0)
	li          t0, 47		// 0x2f ASCII '/'
	bne         s7, t0, .HandleSpacesAndComments_label_152

	// *** Basic block 3

	ld          a0, 24(s1)
	slt         t0, s6, a0
	bge         s6, a0, .HandleSpacesAndComments_label_61

	// *** Basic block 4

	addi        t1, s6, 1
	add         t1, s5, t1
	lb          t1, 0(t1)
	addi        t1, t1, -47
	seqz        t0, t1

	// *** Basic block 5

.HandleSpacesAndComments_label_61:
	beqz        t0, .HandleSpacesAndComments_label_67

	// *** Basic block 6

.HandleSpacesAndComments_label_64:
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

	// *** Basic block 7

.HandleSpacesAndComments_label_67:
	addi        t1, a0, -1
	slt         t0, s6, t1
	bge         s6, t1, .HandleSpacesAndComments_label_79

	// *** Basic block 8

	addi        t1, s6, 1
	add         t1, s5, t1
	lb          t1, 0(t1)
	addi        t1, t1, -42
	seqz        t0, t1

	// *** Basic block 9

.HandleSpacesAndComments_label_79:
	beqz        t0, .HandleSpacesAndComments_label_150

	// *** Basic block 10

	addi        t0, s6, 2
	sd          t0, -24(s0)

	// *** Basic block 11

.HandleSpacesAndComments_label_84:

	// *** Basic block 12

.HandleSpacesAndComments_label_85:
	addi        a2, s0, -24
	mv          a1, s1
	mv          a0, s2
	call        GetNextCommentChar

	// *** Basic block 13

	mv          s7, a0

	// *** Basic block 14

.HandleSpacesAndComments_label_94:
	mv          a0, s4
	call        LexEof

	// *** Basic block 15

	not         s8, a0
	beqz        s8, .HandleSpacesAndComments_label_103

	// *** Basic block 16

	addi        t0, s7, -42
	snez        s8, t0

	// *** Basic block 17

.HandleSpacesAndComments_label_103:
	bnez        s8, .HandleSpacesAndComments_label_85

	// *** Basic block 18

.HandleSpacesAndComments_label_105:
	addi        a2, s0, -24
	mv          a1, s1
	mv          a0, s2
	call        GetNextCommentChar

	// *** Basic block 19

	mv          s7, a0

	// *** Basic block 20

.HandleSpacesAndComments_label_114:
	mv          a0, s4
	call        LexEof

	// *** Basic block 21

	not         s2, a0
	beqz        s2, .HandleSpacesAndComments_label_123

	// *** Basic block 22

	addi        t0, s7, -47
	snez        s2, t0

	// *** Basic block 23

.HandleSpacesAndComments_label_123:
	bnez        s2, .HandleSpacesAndComments_label_84

	// *** Basic block 24

.HandleSpacesAndComments_label_125:
	li          t0, 16		// 0x10 ASCII \x10
	mv          a1, t0
	mv          a0, s3
	call        StringAppendChar

	// *** Basic block 25

	ld          t0, -24(s0)
	sub         s2, t0, s6
	mv          a1, s2
	mv          a0, s3
	call        EncodeLength

	// *** Basic block 26

	ld          t0, 16(s1)
	add         a1, t0, s6
	mv          a2, s2
	mv          a0, s3
	call        StringAppendSegment

	// *** Basic block 27

	j           .HandleSpacesAndComments_label_39

	// *** Basic block 28

.HandleSpacesAndComments_label_150:

	// *** Basic block 29

.HandleSpacesAndComments_label_151:

	// *** Basic block 30

.HandleSpacesAndComments_label_152:
	mv          a0, s7
	call        isspace

	// *** Basic block 31

	beqz        a0, .HandleSpacesAndComments_label_195

	// *** Basic block 32

	seqz        s4, s6
	ld          s8, 24(s1)
	bge         s6, s8, .HandleSpacesAndComments_label_180

	// *** Basic block 33

.HandleSpacesAndComments_label_163:
	ld          s6, -24(s0)
	add         t0, s5, s6
	lb          s7, 0(t0)
	mv          a0, s7
	call        isspace

	// *** Basic block 34

	slli        t0, a0, 56
	srai        t0, t0, 56
	not         t0, t0
	bnez        t0, .HandleSpacesAndComments_label_180

	// *** Basic block 35

.HandleSpacesAndComments_label_174:
	addi        t0, s6, 1
	sd          t0, -24(s0)
	ld          t0, -24(s0)
	blt         t0, s8, .HandleSpacesAndComments_label_163

	// *** Basic block 36

.HandleSpacesAndComments_label_180:
	ld          t0, -24(s0)
	sub         t0, t0, s8
	seqz        t0, t0
	or          s4, s4, t0
	not         t0, s4
	beqz        t0, .HandleSpacesAndComments_label_193

	// *** Basic block 37

	li          t0, 17		// 0x11 ASCII \x11
	mv          a1, t0
	mv          a0, s3
	call        StringAppendChar

	// *** Basic block 38

.HandleSpacesAndComments_label_193:
	j           .HandleSpacesAndComments_label_39

	// *** Basic block 39

.HandleSpacesAndComments_label_195:
	j           .HandleSpacesAndComments_label_197

	// *** Basic block 40

.HandleSpacesAndComments_label_197:
	ld          a0, -24(s0)
	j           .HandleSpacesAndComments_label_64
.func_end_HandleSpacesAndComments:
	.size HandleSpacesAndComments, .func_end_HandleSpacesAndComments-HandleSpacesAndComments

	.local  AppendDefined
	.type AppendDefined, @function

AppendDefined:

	// *** Basic block 0

	.local SkipSpacesAndCommentsInLine
	.global isalnum
	.local EncodeLength
	.global StringAppendSegment
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
	// End of stack frame
	mv          s1, a2
	mv          s2, a0
	mv          s3, a1
	mv          a1, s1
	call        SkipSpacesAndCommentsInLine

	// *** Basic block 1

	mv          s1, a0
	mv          s4, s1
	mv          s5, x0
	ld          s6, 16(s2)
	add         t0, s6, s1
	lb          s7, 0(t0)
	li          t0, 40		// 0x28 ASCII '('
	bne         s7, t0, .AppendDefined_label_80

	// *** Basic block 2

	addi        s1, s1, 1
	mv          a1, s1
	mv          a0, s2
	call        SkipSpacesAndCommentsInLine

	// *** Basic block 3

	mv          s1, a0
	mv          s4, s1
	add         t0, s6, s1
	lb          s9, 0(t0)
	mv          a0, s9
	call        isalnum

	// *** Basic block 4

	mv          s8, a0
	bnez        a0, .AppendDefined_label_60

	// *** Basic block 5

	addi        t0, s9, -95
	seqz        s8, t0

	// *** Basic block 6

.AppendDefined_label_60:
	beqz        s8, .AppendDefined_label_76

	// *** Basic block 7

.AppendDefined_label_62:
	addi        s1, s1, 1
	add         t0, s6, s1
	lb          s9, 0(t0)
	mv          a0, s9
	call        isalnum

	// *** Basic block 8

	mv          s8, a0
	bnez        a0, .AppendDefined_label_74

	// *** Basic block 9

	addi        t0, s9, -95
	seqz        s8, t0

	// *** Basic block 10

.AppendDefined_label_74:
	bnez        s8, .AppendDefined_label_62

	// *** Basic block 11

.AppendDefined_label_76:
	sub         s5, s1, s4
	addi        s1, s1, 1
	j           .AppendDefined_label_107

	// *** Basic block 12

.AppendDefined_label_80:
	mv          a0, s7
	call        isalnum

	// *** Basic block 13

	mv          s8, a0
	bnez        a0, .AppendDefined_label_89

	// *** Basic block 14

	addi        t0, s7, -95
	seqz        s8, t0

	// *** Basic block 15

.AppendDefined_label_89:
	beqz        s8, .AppendDefined_label_105

	// *** Basic block 16

.AppendDefined_label_91:
	addi        s1, s1, 1
	add         t0, s6, s1
	lb          s6, 0(t0)
	mv          a0, s6
	call        isalnum

	// *** Basic block 17

	mv          s7, a0
	bnez        a0, .AppendDefined_label_103

	// *** Basic block 18

	addi        t0, s6, -95
	seqz        s7, t0

	// *** Basic block 19

.AppendDefined_label_103:
	bnez        s7, .AppendDefined_label_91

	// *** Basic block 20

.AppendDefined_label_105:
	sub         s5, s1, s4

	// *** Basic block 21

.AppendDefined_label_107:
	mv          a1, s5
	mv          a0, s3
	call        EncodeLength

	// *** Basic block 22

	ld          t0, 16(s2)
	add         a1, t0, s4
	mv          a2, s5
	mv          a0, s3
	call        StringAppendSegment

	// *** Basic block 23

	mv          a0, s1

	// *** Basic block 24

.AppendDefined_label_124:
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
.func_end_AppendDefined:
	.size AppendDefined, .func_end_AppendDefined-AppendDefined

	.local  Tokenize
	.type Tokenize, @function

Tokenize:

	// *** Basic block 0

	.global StringInit
	.local HandleSpacesAndComments
	.global isalpha
	.global StringAppendChar
	.local AppendStringLiteral
	.local AppendCharLiteral
	.global isalnum
	.global StringEqual
	.local AppendDefined
	.local EncodeLength
	.global StringAppendString
	.global StringDestruct
	.global isdigit
	.local AppendNumber
	.local AppendOtherToken
	addi sp, sp, -192
	// Saved return address (offset 184) and frame pointer (offset 176)
	sd ra, 184(sp)
	sd s0, 176(sp)
	addi s0, sp, 192
	// Local vars at offset -96(s0)
	// Spilled register region: 8 bytes at -104(s0) to -96(s0)
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
	mv          s1, a2
	sd          s1, -104(s0)	// Spilled @54
	mv          s2, a3
	mv          s3, a1
	mv          s4, a0
	mv          s5, a5
	mv          s6, a6
	beqz        a4, .Tokenize_label_79

	// *** Basic block 1

	mv          a1, x0
	mv          a0, s1
	call        StringInit

	// *** Basic block 2

.Tokenize_label_79:
	mv          s7, s2
	ld          s8, 24(s3)
	bge         s7, s8, .Tokenize_label_555

	// *** Basic block 3

	ld          s2, 16(s3)

	// *** Basic block 4

.Tokenize_label_88:
	mv          a3, s7
	mv          a2, s1
	mv          a1, s3
	mv          a0, s4
	call        HandleSpacesAndComments

	// *** Basic block 5

	mv          s7, a0
	bge         s7, s8, .Tokenize_label_555

	// *** Basic block 6

.Tokenize_label_102:
	add         t0, s2, s7
	lb          s4, 0(t0)
	mv          a0, s4
	call        isalpha

	// *** Basic block 7

	mv          s9, a0
	bnez        a0, .Tokenize_label_116

	// *** Basic block 8

	addi        t0, s4, -95
	seqz        s9, t0

	// *** Basic block 9

.Tokenize_label_116:
	bnez        s9, .Tokenize_label_132

	// *** Basic block 10

	beqz        s5, .Tokenize_label_131

	// *** Basic block 11

	addi        t0, s4, -46
	seqz        s9, t0
	li          t0, 46		// 0x2e ASCII '.'
	beq         s4, t0, .Tokenize_label_130

	// *** Basic block 12

	addi        t0, s4, -64
	seqz        s9, t0

	// *** Basic block 13

.Tokenize_label_130:

	// *** Basic block 14

.Tokenize_label_131:

	// *** Basic block 15

.Tokenize_label_132:
	beqz        s9, .Tokenize_label_325

	// *** Basic block 16

	addi        t1, s4, -76
	seqz        t0, t1
	li          s9, 76		// 0x4c ASCII 'L'
	beq         s4, s9, .Tokenize_label_144

	// *** Basic block 17

	addi        t1, s4, -108
	seqz        t0, t1

	// *** Basic block 18

.Tokenize_label_144:
	beqz        t0, .Tokenize_label_152

	// *** Basic block 19

	addi        t1, s7, 1
	add         t1, s2, t1
	lb          t1, 0(t1)
	addi        t1, t1, -34
	seqz        t0, t1

	// *** Basic block 20

.Tokenize_label_152:
	beqz        t0, .Tokenize_label_169

	// *** Basic block 21

	li          t1, 7		// 0x7 ASCII \x7
	mv          a1, t1
	mv          a0, s1
	call        StringAppendChar

	// *** Basic block 22

	addi        a2, s7, 2
	mv          a1, s1
	mv          a0, s3
	call        AppendStringLiteral

	// *** Basic block 23

	mv          s7, a0
	j           .Tokenize_label_323

	// *** Basic block 24

.Tokenize_label_169:
	mv          t1, t0
	beq         s4, s9, .Tokenize_label_177

	// *** Basic block 25

	addi        t0, s4, -108
	seqz        t1, t0

	// *** Basic block 26

.Tokenize_label_177:
	beqz        t1, .Tokenize_label_185

	// *** Basic block 27

	addi        t0, s7, 1
	add         t0, s2, t0
	lb          t0, 0(t0)
	addi        t0, t0, -39
	seqz        t1, t0

	// *** Basic block 28

.Tokenize_label_185:
	beqz        t1, .Tokenize_label_202

	// *** Basic block 29

	li          t0, 9		// 0x9 ASCII \x9
	mv          a1, t0
	mv          a0, s1
	call        StringAppendChar

	// *** Basic block 30

	addi        a2, s7, 2
	mv          a1, s1
	mv          a0, s3
	call        AppendCharLiteral

	// *** Basic block 31

	mv          s7, a0
	j           .Tokenize_label_322

	// *** Basic block 32

.Tokenize_label_202:
	mv          s9, s7
	sd          x0, -96(s0)
	sd          x0, -88(s0)
	sd          x0, -80(s0)
	sd          x0, -72(s0)
	sd          x0, -64(s0)
	sb          x0, -96(s0)
	mv          a0, s4
	call        isalnum

	// *** Basic block 33

	mv          s10, a0
	bnez        a0, .Tokenize_label_225

	// *** Basic block 34

	addi        t0, s4, -95
	seqz        s10, t0

	// *** Basic block 35

.Tokenize_label_225:
	bnez        s10, .Tokenize_label_239

	// *** Basic block 36

	beqz        s5, .Tokenize_label_238

	// *** Basic block 37

	addi        t0, s4, -46
	seqz        s10, t0
	li          t0, 46		// 0x2e ASCII '.'
	beq         s4, t0, .Tokenize_label_237

	// *** Basic block 38

	addi        t0, s4, -64
	seqz        s10, t0

	// *** Basic block 39

.Tokenize_label_237:

	// *** Basic block 40

.Tokenize_label_238:

	// *** Basic block 41

.Tokenize_label_239:
	beqz        s10, .Tokenize_label_276

	// *** Basic block 42

.Tokenize_label_241:
	addi        a0, s0, -96
	add         t0, s2, s7
	lb          a1, 0(t0)
	call        StringAppendChar

	// *** Basic block 43

	addi        s7, s7, 1
	add         t0, s2, s7
	lb          s11, 0(t0)
	mv          a0, s11
	call        isalnum

	// *** Basic block 44

	mv          s10, a0
	bnez        a0, .Tokenize_label_260

	// *** Basic block 45

	addi        t0, s11, -95
	seqz        s10, t0

	// *** Basic block 46

.Tokenize_label_260:
	bnez        s10, .Tokenize_label_274

	// *** Basic block 47

	beqz        s5, .Tokenize_label_273

	// *** Basic block 48

	addi        t0, s11, -46
	seqz        s10, t0
	li          t0, 46		// 0x2e ASCII '.'
	beq         s11, t0, .Tokenize_label_272

	// *** Basic block 49

	addi        t0, s11, -64
	seqz        s10, t0

	// *** Basic block 50

.Tokenize_label_272:

	// *** Basic block 51

.Tokenize_label_273:

	// *** Basic block 52

.Tokenize_label_274:
	bnez        s10, .Tokenize_label_241

	// *** Basic block 53

.Tokenize_label_276:
	addi        a0, s0, -96
	lla         a1, .str.106
	call        StringEqual

	// *** Basic block 54

	beqz        a0, .Tokenize_label_299

	// *** Basic block 55

	li          t0, 5		// 0x5 ASCII \x5
	mv          a1, t0
	mv          a0, s1
	call        StringAppendChar

	// *** Basic block 56

	mv          a2, s7
	mv          a1, s1
	mv          a0, s3
	call        AppendDefined

	// *** Basic block 57

	mv          s7, a0
	j           .Tokenize_label_318

	// *** Basic block 58

.Tokenize_label_299:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        StringAppendChar

	// *** Basic block 59

	sub         s10, s7, s9
	mv          a1, s10
	mv          a0, s1
	call        EncodeLength

	// *** Basic block 60

	addi        a1, s0, -96
	mv          a0, s1
	call        StringAppendString

	// *** Basic block 61

.Tokenize_label_318:
	addi        a0, s0, -96
	call        StringDestruct

	// *** Basic block 62

.Tokenize_label_322:

	// *** Basic block 63

.Tokenize_label_323:
	j           .Tokenize_label_552

	// *** Basic block 64

.Tokenize_label_325:
	addi        t0, s4, -60
	seqz        s10, t0
	li          t0, 60		// 0x3c ASCII '<'
	bne         s4, t0, .Tokenize_label_333

	// *** Basic block 65

	mv          s10, s6

	// *** Basic block 66

.Tokenize_label_333:
	beqz        s10, .Tokenize_label_396

	// *** Basic block 67

	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	mv          a0, s1
	call        StringAppendChar

	// *** Basic block 68

	addi        s7, s7, 1
	mv          s11, s7
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -24(s0)
	sb          x0, -56(s0)
	slt         t0, s7, s8
	bge         s7, s8, .Tokenize_label_363

	// *** Basic block 69

	add         t1, s2, s7
	lb          t1, 0(t1)
	addi        t1, t1, -62
	snez        t0, t1

	// *** Basic block 70

.Tokenize_label_363:
	beqz        t0, .Tokenize_label_382

	// *** Basic block 71

.Tokenize_label_365:
	addi        a0, s0, -56
	add         t0, s2, s7
	lb          a1, 0(t0)
	call        StringAppendChar

	// *** Basic block 72

	addi        s7, s7, 1
	slt         t0, s7, s8
	bge         s7, s8, .Tokenize_label_380

	// *** Basic block 73

	add         t1, s2, s7
	lb          t1, 0(t1)
	addi        t1, t1, -62
	snez        t0, t1

	// *** Basic block 74

.Tokenize_label_380:
	bnez        t0, .Tokenize_label_365

	// *** Basic block 75

.Tokenize_label_382:
	sub         s1, s7, s11
	mv          a1, s1
	ld          a0, -104(s0)	// Spilled @54
	call        EncodeLength

	// *** Basic block 76

	addi        a1, s0, -56
	ld          a0, -104(s0)	// Spilled @54
	call        StringAppendString

	// *** Basic block 77

	j           .Tokenize_label_552

	// *** Basic block 78

.Tokenize_label_396:
	li          t0, 34		// 0x22 ASCII '"'
	bne         s4, t0, .Tokenize_label_416

	// *** Basic block 79

	li          t0, 6		// 0x6 ASCII \x6
	mv          a1, t0
	ld          a0, -104(s0)	// Spilled @54
	call        StringAppendChar

	// *** Basic block 80

	addi        a2, s7, 1
	ld          a1, -104(s0)	// Spilled @54
	mv          a0, s3
	call        AppendStringLiteral

	// *** Basic block 81

	mv          s7, a0
	j           .Tokenize_label_552

	// *** Basic block 82

.Tokenize_label_416:
	li          t0, 39		// 0x27 ASCII '''
	bne         s4, t0, .Tokenize_label_436

	// *** Basic block 83

	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	ld          a0, -104(s0)	// Spilled @54
	call        StringAppendChar

	// *** Basic block 84

	addi        a2, s7, 1
	ld          a1, -104(s0)	// Spilled @54
	mv          a0, s3
	call        AppendCharLiteral

	// *** Basic block 85

	mv          s7, a0
	j           .Tokenize_label_552

	// *** Basic block 86

.Tokenize_label_436:
	mv          a0, s4
	call        isdigit

	// *** Basic block 87

	mv          s1, a0
	bnez        a0, .Tokenize_label_455

	// *** Basic block 88

	addi        t0, s4, -46
	seqz        s1, t0
	li          t0, 46		// 0x2e ASCII '.'
	bne         s4, t0, .Tokenize_label_454

	// *** Basic block 89

	addi        t0, s7, 1
	add         t0, s2, t0
	lb          a0, 0(t0)
	call        isdigit

	// *** Basic block 91

.Tokenize_label_454:

	// *** Basic block 92

.Tokenize_label_455:
	beqz        s1, .Tokenize_label_472

	// *** Basic block 93

	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	ld          a0, -104(s0)	// Spilled @54
	call        StringAppendChar

	// *** Basic block 94

	mv          a2, s7
	ld          a1, -104(s0)	// Spilled @54
	mv          a0, s3
	call        AppendNumber

	// *** Basic block 95

	mv          s7, a0
	j           .Tokenize_label_552

	// *** Basic block 96

.Tokenize_label_472:
	li          s1, 35		// 0x23 ASCII '#'
	bne         s4, s1, .Tokenize_label_502

	// *** Basic block 97

	addi        s7, s7, 1
	add         t0, s2, s7
	lb          t0, 0(t0)
	bne         t0, s1, .Tokenize_label_492

	// *** Basic block 98

	li          t0, 11		// 0xb ASCII \xb
	mv          a1, t0
	ld          a0, -104(s0)	// Spilled @54
	call        StringAppendChar

	// *** Basic block 99

	j           .Tokenize_label_499

	// *** Basic block 100

.Tokenize_label_492:
	li          t0, 10		// 0xa ASCII \xa
	mv          a1, t0
	ld          a0, -104(s0)	// Spilled @54
	call        StringAppendChar

	// *** Basic block 101

.Tokenize_label_499:
	addi        s7, s7, 1
	j           .Tokenize_label_552

	// *** Basic block 102

.Tokenize_label_502:
	li          t0, 40		// 0x28 ASCII '('
	bne         s4, t0, .Tokenize_label_515

	// *** Basic block 103

	li          t0, 13		// 0xd ASCII \xd
	mv          a1, t0
	ld          a0, -104(s0)	// Spilled @54
	call        StringAppendChar

	// *** Basic block 104

	addi        s7, s7, 1
	j           .Tokenize_label_552

	// *** Basic block 105

.Tokenize_label_515:
	li          t0, 41		// 0x29 ASCII ')'
	bne         s4, t0, .Tokenize_label_529

	// *** Basic block 106

	li          t0, 14		// 0xe ASCII \xe
	mv          a1, t0
	ld          a0, -104(s0)	// Spilled @54
	call        StringAppendChar

	// *** Basic block 107

	addi        s7, s7, 1
	j           .Tokenize_label_552

	// *** Basic block 108

.Tokenize_label_529:
	li          t0, 44		// 0x2c ASCII ','
	bne         s4, t0, .Tokenize_label_543

	// *** Basic block 109

	li          t0, 15		// 0xf ASCII \xf
	mv          a1, t0
	ld          a0, -104(s0)	// Spilled @54
	call        StringAppendChar

	// *** Basic block 110

	addi        s7, s7, 1
	j           .Tokenize_label_552

	// *** Basic block 111

.Tokenize_label_543:
	mv          a2, s7
	ld          a1, -104(s0)	// Spilled @54
	mv          a0, s3
	call        AppendOtherToken

	// *** Basic block 112

	mv          s7, a0

	// *** Basic block 113

.Tokenize_label_552:
	bge         s7, s8, .Tokenize_label_88

	// *** Basic block 114

.Tokenize_label_555:
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
.func_end_Tokenize:
	.size Tokenize, .func_end_Tokenize-Tokenize

	.local  PrintLengthDelimitedToken
	.type PrintLengthDelimitedToken, @function

PrintLengthDelimitedToken:

	// *** Basic block 0

	.local DecodeLength
	.global putchar
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
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	addi        a2, s0, -32
	call        DecodeLength

	// *** Basic block 1

	ld          s3, 16(s2)
	mv          s1, a0
	ld          t0, -32(s0)
	addi        t0, t0, -1
	sd          t0, -32(s0)
	bge         x0, t0, .PrintLengthDelimitedToken_label_50

	// *** Basic block 2

.PrintLengthDelimitedToken_label_36:
	mv          t0, s1
	addi        s1, s1, 1
	add         t1, s3, t0
	lb          a0, 0(t1)
	call        putchar

	// *** Basic block 3

	ld          t0, -32(s0)
	addi        t0, t0, -1
	sd          t0, -32(s0)
	blt         x0, t0, .PrintLengthDelimitedToken_label_36

	// *** Basic block 4

.PrintLengthDelimitedToken_label_50:
	li          t0, 10		// 0xa ASCII \xa
	mv          a0, t0
	call        putchar

	// *** Basic block 5

	mv          a0, s1

	// *** Basic block 6

.PrintLengthDelimitedToken_label_57:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PrintLengthDelimitedToken:
	.size PrintLengthDelimitedToken, .func_end_PrintLengthDelimitedToken-PrintLengthDelimitedToken

	.local  PrintTokenizedLine
	.type PrintTokenizedLine, @function

PrintTokenizedLine:

	// *** Basic block 0

	.global printf
	.local PrintLengthDelimitedToken
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
	ld          s3, 24(s1)
	bge         x0, s3, .PrintTokenizedLine_label_257

	// *** Basic block 1

	ld          s4, 16(s1)

	// *** Basic block 2

.PrintTokenizedLine_label_41:
	mv          t0, s2
	addi        s2, s2, 1
	add         t1, s4, t0
	lb          t1, 0(t1)
	addi        t1, t1, -1
	slli        t1, t1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 3

	j           .PrintTokenizedLine_label_75

	// *** Basic block 4

	j           .PrintTokenizedLine_label_184

	// *** Basic block 5

	j           .PrintTokenizedLine_label_88

	// *** Basic block 6

	j           .PrintTokenizedLine_label_100

	// *** Basic block 7

	j           .PrintTokenizedLine_label_112

	// *** Basic block 8

	j           .PrintTokenizedLine_label_124

	// *** Basic block 9

	j           .PrintTokenizedLine_label_136

	// *** Basic block 10

	j           .PrintTokenizedLine_label_148

	// *** Basic block 11

	j           .PrintTokenizedLine_label_172

	// *** Basic block 12

	j           .PrintTokenizedLine_label_194

	// *** Basic block 13

	j           .PrintTokenizedLine_label_200

	// *** Basic block 14

	j           .PrintTokenizedLine_label_206

	// *** Basic block 15

	j           .PrintTokenizedLine_label_212

	// *** Basic block 16

	j           .PrintTokenizedLine_label_218

	// *** Basic block 17

	j           .PrintTokenizedLine_label_224

	// *** Basic block 18

	j           .PrintTokenizedLine_label_236

	// *** Basic block 19

	j           .PrintTokenizedLine_label_230

	// *** Basic block 20

	j           .PrintTokenizedLine_label_160

	// *** Basic block 21

	j           .PrintTokenizedLine_label_248

	// *** Basic block 22

.PrintTokenizedLine_label_75:
	lla         a0, .str.107
	call        printf

	// *** Basic block 23

	mv          a1, s2
	mv          a0, s1
	call        PrintLengthDelimitedToken

	// *** Basic block 24

	mv          s2, a0
	j           .PrintTokenizedLine_label_254

	// *** Basic block 25

.PrintTokenizedLine_label_88:
	lla         a0, .str.108
	call        printf

	// *** Basic block 26

	mv          a1, s2
	mv          a0, s1
	call        PrintLengthDelimitedToken

	// *** Basic block 27

	mv          s2, a0
	j           .PrintTokenizedLine_label_254

	// *** Basic block 28

.PrintTokenizedLine_label_100:
	lla         a0, .str.109
	call        printf

	// *** Basic block 29

	mv          a1, s2
	mv          a0, s1
	call        PrintLengthDelimitedToken

	// *** Basic block 30

	mv          s2, a0
	j           .PrintTokenizedLine_label_254

	// *** Basic block 31

.PrintTokenizedLine_label_112:
	lla         a0, .str.110
	call        printf

	// *** Basic block 32

	mv          a1, s2
	mv          a0, s1
	call        PrintLengthDelimitedToken

	// *** Basic block 33

	mv          s2, a0
	j           .PrintTokenizedLine_label_254

	// *** Basic block 34

.PrintTokenizedLine_label_124:
	lla         a0, .str.111
	call        printf

	// *** Basic block 35

	mv          a1, s2
	mv          a0, s1
	call        PrintLengthDelimitedToken

	// *** Basic block 36

	mv          s2, a0
	j           .PrintTokenizedLine_label_254

	// *** Basic block 37

.PrintTokenizedLine_label_136:
	lla         a0, .str.112
	call        printf

	// *** Basic block 38

	mv          a1, s2
	mv          a0, s1
	call        PrintLengthDelimitedToken

	// *** Basic block 39

	mv          s2, a0
	j           .PrintTokenizedLine_label_254

	// *** Basic block 40

.PrintTokenizedLine_label_148:
	lla         a0, .str.113
	call        printf

	// *** Basic block 41

	mv          a1, s2
	mv          a0, s1
	call        PrintLengthDelimitedToken

	// *** Basic block 42

	mv          s2, a0
	j           .PrintTokenizedLine_label_254

	// *** Basic block 43

.PrintTokenizedLine_label_160:
	lla         a0, .str.114
	call        printf

	// *** Basic block 44

	mv          a1, s2
	mv          a0, s1
	call        PrintLengthDelimitedToken

	// *** Basic block 45

	mv          s2, a0
	j           .PrintTokenizedLine_label_254

	// *** Basic block 46

.PrintTokenizedLine_label_172:
	lla         a0, .str.115
	call        printf

	// *** Basic block 47

	mv          a1, s2
	mv          a0, s1
	call        PrintLengthDelimitedToken

	// *** Basic block 48

	mv          s2, a0
	j           .PrintTokenizedLine_label_254

	// *** Basic block 49

.PrintTokenizedLine_label_184:
	lla         a0, .str.116
	add         t0, s4, s2
	lb          a1, 0(t0)
	call        printf

	// *** Basic block 50

	addi        s2, s2, 1
	j           .PrintTokenizedLine_label_254

	// *** Basic block 51

.PrintTokenizedLine_label_194:
	lla         a0, .str.117
	call        printf

	// *** Basic block 52

	j           .PrintTokenizedLine_label_254

	// *** Basic block 53

.PrintTokenizedLine_label_200:
	lla         a0, .str.118
	call        printf

	// *** Basic block 54

	j           .PrintTokenizedLine_label_254

	// *** Basic block 55

.PrintTokenizedLine_label_206:
	lla         a0, .str.119
	call        printf

	// *** Basic block 56

	j           .PrintTokenizedLine_label_254

	// *** Basic block 57

.PrintTokenizedLine_label_212:
	lla         a0, .str.120
	call        printf

	// *** Basic block 58

	j           .PrintTokenizedLine_label_254

	// *** Basic block 59

.PrintTokenizedLine_label_218:
	lla         a0, .str.121
	call        printf

	// *** Basic block 60

	j           .PrintTokenizedLine_label_254

	// *** Basic block 61

.PrintTokenizedLine_label_224:
	lla         a0, .str.122
	call        printf

	// *** Basic block 62

	j           .PrintTokenizedLine_label_254

	// *** Basic block 63

.PrintTokenizedLine_label_230:
	lla         a0, .str.123
	call        printf

	// *** Basic block 64

	j           .PrintTokenizedLine_label_254

	// *** Basic block 65

.PrintTokenizedLine_label_236:
	lla         a0, .str.124
	call        printf

	// *** Basic block 66

	mv          a1, s2
	mv          a0, s1
	call        PrintLengthDelimitedToken

	// *** Basic block 67

	mv          s2, a0
	j           .PrintTokenizedLine_label_254

	// *** Basic block 68

.PrintTokenizedLine_label_248:
	lla         a0, .str.125
	call        printf

	// *** Basic block 69

	j           .PrintTokenizedLine_label_254

	// *** Basic block 70

.PrintTokenizedLine_label_254:
	blt         s2, s3, .PrintTokenizedLine_label_41

	// *** Basic block 71

.PrintTokenizedLine_label_257:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PrintTokenizedLine:
	.size PrintTokenizedLine, .func_end_PrintTokenizedLine-PrintTokenizedLine

	.local  ReplaceCurrentToken
	.type ReplaceCurrentToken, @function

ReplaceCurrentToken:

	// *** Basic block 0

	.global StringReplaceString
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	ld          a0, 8(t0)
	ld          t2, 24(t0)
	ld          t3, 32(t0)
	sub         a2, t3, t2
	mv          a3, t1
	mv          a1, t2
	j           StringReplaceString
.func_end_ReplaceCurrentToken:
	.size ReplaceCurrentToken, .func_end_ReplaceCurrentToken-ReplaceCurrentToken

	.local  EraseCurrentToken
	.type EraseCurrentToken, @function

EraseCurrentToken:

	// *** Basic block 0

	.global StringErase
	.local FindNextTokenIndex
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
	ld          a0, 8(s1)
	ld          a1, 24(s1)
	ld          t0, 32(s1)
	sub         a2, t0, a1
	call        StringErase

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           FindNextTokenIndex
.func_end_EraseCurrentToken:
	.size EraseCurrentToken, .func_end_EraseCurrentToken-EraseCurrentToken

	.local  SkipSpaceTokens
	.type SkipSpaceTokens, @function

SkipSpaceTokens:

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
	.local IsSpaceToken
	.local MoveToNextToken
	mv          s1, a0
	call        IsSpaceToken

	// *** Basic block 1

	beqz        a0, .SkipSpaceTokens_label_19

	// *** Basic block 2

.SkipSpaceTokens_label_11:
	mv          a0, s1
	call        MoveToNextToken

	// *** Basic block 3

	mv          a0, s1
	call        IsSpaceToken

	// *** Basic block 4

	bnez        a0, .SkipSpaceTokens_label_11

	// *** Basic block 5

.SkipSpaceTokens_label_19:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SkipSpaceTokens:
	.size SkipSpaceTokens, .func_end_SkipSpaceTokens-SkipSpaceTokens

	.local  TokenizeAndReplaceCurrentToken
	.type TokenizeAndReplaceCurrentToken, @function

TokenizeAndReplaceCurrentToken:

	// *** Basic block 0

	.local Tokenize
	.local ReplaceCurrentToken
	.global StringDestruct
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          t0, a2
	ld          a0, 0(s1)
	addi        a2, s0, -64
	mv          a6, x0
	mv          a5, t0
	li          a4, 1		// 0x1 ASCII \x1
	mv          a3, x0
	call        Tokenize

	// *** Basic block 1

	addi        a1, s0, -64
	mv          a0, s1
	call        ReplaceCurrentToken

	// *** Basic block 2

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 3

	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TokenizeAndReplaceCurrentToken:
	.size TokenizeAndReplaceCurrentToken, .func_end_TokenizeAndReplaceCurrentToken-TokenizeAndReplaceCurrentToken

	.local  DetokenizeToken
	.type DetokenizeToken, @function

DetokenizeToken:

	// *** Basic block 0

	.local AppendTokenSpelling
	.global StringPrintf
	.global StringAppendChar
	.global StringDestruct
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	ld          s4, 16(s1)
	add         t0, s4, s2
	lb          t0, 0(t0)
	addi        t0, t0, -1
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .DetokenizeToken_label_80

	// *** Basic block 2

	j           .DetokenizeToken_label_220

	// *** Basic block 3

	j           .DetokenizeToken_label_81

	// *** Basic block 4

	j           .DetokenizeToken_label_82

	// *** Basic block 5

	j           .DetokenizeToken_label_92

	// *** Basic block 6

	j           .DetokenizeToken_label_125

	// *** Basic block 7

	j           .DetokenizeToken_label_144

	// *** Basic block 8

	j           .DetokenizeToken_label_163

	// *** Basic block 9

	j           .DetokenizeToken_label_201

	// *** Basic block 10

	j           .DetokenizeToken_label_229

	// *** Basic block 11

	j           .DetokenizeToken_label_237

	// *** Basic block 12

	j           .DetokenizeToken_label_245

	// *** Basic block 13

	j           .DetokenizeToken_label_247

	// *** Basic block 14

	j           .DetokenizeToken_label_255

	// *** Basic block 15

	j           .DetokenizeToken_label_263

	// *** Basic block 16

	j           .DetokenizeToken_label_83

	// *** Basic block 17

	j           .DetokenizeToken_label_271

	// *** Basic block 18

	j           .DetokenizeToken_label_182

	// *** Basic block 19

	j           .DetokenizeToken_label_279

	// *** Basic block 20

.DetokenizeToken_label_80:

	// *** Basic block 21

.DetokenizeToken_label_81:

	// *** Basic block 22

.DetokenizeToken_label_82:

	// *** Basic block 23

.DetokenizeToken_label_83:
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        AppendTokenSpelling

	// *** Basic block 24

	j           .DetokenizeToken_label_281

	// *** Basic block 25

.DetokenizeToken_label_92:
	addi        a2, s0, -64
	mv          a1, s2
	mv          a0, s1
	call        AppendTokenSpelling

	// *** Basic block 26

	addi        t0, s0, -64
	ld          t0, 24(t0)
	bnez        t0, .DetokenizeToken_label_112

	// *** Basic block 27

	lla         a1, .str.126
	mv          a0, s3
	call        StringPrintf

	// *** Basic block 28

	j           .DetokenizeToken_label_123

	// *** Basic block 29

.DetokenizeToken_label_112:
	lla         a1, .str.127
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s3
	call        StringPrintf

	// *** Basic block 30

.DetokenizeToken_label_123:
	j           .DetokenizeToken_label_281

	// *** Basic block 31

.DetokenizeToken_label_125:
	addi        a2, s0, -64
	mv          a1, s2
	mv          a0, s1
	call        AppendTokenSpelling

	// *** Basic block 32

	lla         a1, .str.128
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s3
	call        StringPrintf

	// *** Basic block 33

	j           .DetokenizeToken_label_281

	// *** Basic block 34

.DetokenizeToken_label_144:
	addi        a2, s0, -64
	mv          a1, s2
	mv          a0, s1
	call        AppendTokenSpelling

	// *** Basic block 35

	lla         a1, .str.129
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s3
	call        StringPrintf

	// *** Basic block 36

	j           .DetokenizeToken_label_281

	// *** Basic block 37

.DetokenizeToken_label_163:
	addi        a2, s0, -64
	mv          a1, s2
	mv          a0, s1
	call        AppendTokenSpelling

	// *** Basic block 38

	lla         a1, .str.130
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s3
	call        StringPrintf

	// *** Basic block 39

	j           .DetokenizeToken_label_281

	// *** Basic block 40

.DetokenizeToken_label_182:
	addi        a2, s0, -64
	mv          a1, s2
	mv          a0, s1
	call        AppendTokenSpelling

	// *** Basic block 41

	lla         a1, .str.131
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s3
	call        StringPrintf

	// *** Basic block 42

	j           .DetokenizeToken_label_281

	// *** Basic block 43

.DetokenizeToken_label_201:
	addi        a2, s0, -64
	mv          a1, s2
	mv          a0, s1
	call        AppendTokenSpelling

	// *** Basic block 44

	lla         a1, .str.132
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s3
	call        StringPrintf

	// *** Basic block 45

	j           .DetokenizeToken_label_281

	// *** Basic block 46

.DetokenizeToken_label_220:
	addi        t0, s2, 1
	add         t0, s4, t0
	lb          a1, 0(t0)
	mv          a0, s3
	call        StringAppendChar

	// *** Basic block 47

	j           .DetokenizeToken_label_281

	// *** Basic block 48

.DetokenizeToken_label_229:
	lla         a1, .str.133
	mv          a0, s3
	call        StringPrintf

	// *** Basic block 49

	j           .DetokenizeToken_label_281

	// *** Basic block 50

.DetokenizeToken_label_237:
	lla         a1, .str.134
	mv          a0, s3
	call        StringPrintf

	// *** Basic block 51

	j           .DetokenizeToken_label_281

	// *** Basic block 52

.DetokenizeToken_label_245:
	j           .DetokenizeToken_label_281

	// *** Basic block 53

.DetokenizeToken_label_247:
	lla         a1, .str.135
	mv          a0, s3
	call        StringPrintf

	// *** Basic block 54

	j           .DetokenizeToken_label_281

	// *** Basic block 55

.DetokenizeToken_label_255:
	lla         a1, .str.136
	mv          a0, s3
	call        StringPrintf

	// *** Basic block 56

	j           .DetokenizeToken_label_281

	// *** Basic block 57

.DetokenizeToken_label_263:
	lla         a1, .str.137
	mv          a0, s3
	call        StringPrintf

	// *** Basic block 58

	j           .DetokenizeToken_label_281

	// *** Basic block 59

.DetokenizeToken_label_271:
	lla         a1, .str.138
	mv          a0, s3
	call        StringPrintf

	// *** Basic block 60

	j           .DetokenizeToken_label_281

	// *** Basic block 61

.DetokenizeToken_label_279:
	j           .DetokenizeToken_label_281

	// *** Basic block 62

.DetokenizeToken_label_281:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 63

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DetokenizeToken:
	.size DetokenizeToken, .func_end_DetokenizeToken-DetokenizeToken

	.local  Detokenize
	.type Detokenize, @function

Detokenize:

	// *** Basic block 0

	.local TokenIteratorInit
	.local CurrentToken
	.local DetokenizeToken
	.local MoveToNextToken
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	mv          t0, a0
	mv          t1, a1
	mv          s1, a2
	addi        a0, s0, -64
	mv          a2, t1
	mv          a1, t0
	call        TokenIteratorInit

	// *** Basic block 1

	addi        a0, s0, -64
	call        CurrentToken

	// *** Basic block 2

	li          s2, 19		// 0x13 ASCII \x13
	beq         a0, s2, .Detokenize_label_58

	// *** Basic block 3

.Detokenize_label_35:
	addi        t0, s0, -64
	ld          a0, 8(t0)
	addi        t0, s0, -64
	ld          a1, 24(t0)
	mv          a2, s1
	call        DetokenizeToken

	// *** Basic block 4

	addi        a0, s0, -64
	call        MoveToNextToken

	// *** Basic block 5

	addi        a0, s0, -64
	call        CurrentToken

	// *** Basic block 6

	bne         a0, s2, .Detokenize_label_35

	// *** Basic block 7

.Detokenize_label_58:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Detokenize:
	.size Detokenize, .func_end_Detokenize-Detokenize

	.local  SkipToEndOfComment
	.type SkipToEndOfComment, @function

SkipToEndOfComment:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	ld          t2, 24(t1)
	bge         t0, t2, .SkipToEndOfComment_label_45

	// *** Basic block 1

	ld          t3, 16(t1)

	// *** Basic block 2

.SkipToEndOfComment_label_22:
	add         t4, t3, t0
	lb          t4, 0(t4)
	addi        t5, t4, -42
	seqz        t1, t5
	li          t5, 42		// 0x2a ASCII '*'
	bne         t4, t5, .SkipToEndOfComment_label_37

	// *** Basic block 3

	addi        t4, t0, 1
	add         t3, t3, t4
	lb          t3, 0(t3)
	addi        t3, t3, -47
	seqz        t1, t3

	// *** Basic block 4

.SkipToEndOfComment_label_37:
	beqz        t1, .SkipToEndOfComment_label_41

	// *** Basic block 5

	addi        t0, t0, 2
	j           .SkipToEndOfComment_label_45

	// *** Basic block 6

.SkipToEndOfComment_label_41:
	addi        t0, t0, 1
	blt         t0, t2, .SkipToEndOfComment_label_22

	// *** Basic block 7

.SkipToEndOfComment_label_45:
	mv          a0, t0

	// *** Basic block 8

.SkipToEndOfComment_label_48:
	ret         
.func_end_SkipToEndOfComment:
	.size SkipToEndOfComment, .func_end_SkipToEndOfComment-SkipToEndOfComment

	.local  SkipSpacesAndComments
	.type SkipSpacesAndComments, @function

SkipSpacesAndComments:

	// *** Basic block 0

	.global StringAppendChar
	.global isblank
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
	mv          t0, a2
	mv          s2, a3
	ld          s1, 24(t0)
	bge         s1, s1, .SkipSpacesAndComments_label_142

	// *** Basic block 1

	ld          s3, 16(t0)
	addi        s4, s1, -1

	// *** Basic block 2

.SkipSpacesAndComments_label_32:
	add         t0, s3, s1
	lb          s5, 0(t0)
	li          t0, 47		// 0x2f ASCII '/'
	bne         s5, t0, .SkipSpacesAndComments_label_119

	// *** Basic block 3

	slt         t0, s1, s1
	bge         s1, s1, .SkipSpacesAndComments_label_49

	// *** Basic block 4

	addi        t1, s1, 1
	add         t1, s3, t1
	lb          t1, 0(t1)
	addi        t1, t1, -47
	seqz        t0, t1

	// *** Basic block 5

.SkipSpacesAndComments_label_49:
	beqz        t0, .SkipSpacesAndComments_label_67

	// *** Basic block 6

	beq         s2, x0, .SkipSpacesAndComments_label_61

	// *** Basic block 7

	li          t0, 32		// 0x20 ASCII ' '
	mv          a1, t0
	mv          a0, s2
	call        StringAppendChar

	// *** Basic block 8

.SkipSpacesAndComments_label_61:
	mv          a0, s1

	// *** Basic block 9

.SkipSpacesAndComments_label_64:
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

	// *** Basic block 10

.SkipSpacesAndComments_label_67:
	slt         t0, s1, s4
	bge         s1, s4, .SkipSpacesAndComments_label_77

	// *** Basic block 11

	addi        t1, s1, 1
	add         t1, s3, t1
	lb          t1, 0(t1)
	addi        t1, t1, -42
	seqz        t0, t1

	// *** Basic block 12

.SkipSpacesAndComments_label_77:
	beqz        t0, .SkipSpacesAndComments_label_117

	// *** Basic block 13

	addi        s1, s1, 2

	// *** Basic block 14

.SkipSpacesAndComments_label_80:

	// *** Basic block 15

.SkipSpacesAndComments_label_81:
	mv          t0, s1
	addi        s1, s1, 1
	add         t1, s3, t0
	lb          s5, 0(t1)

	// *** Basic block 16

.SkipSpacesAndComments_label_87:
	slt         t0, s1, s1
	bge         s1, s1, .SkipSpacesAndComments_label_93

	// *** Basic block 17

	addi        t1, s5, -42
	snez        t0, t1

	// *** Basic block 18

.SkipSpacesAndComments_label_93:
	bnez        t0, .SkipSpacesAndComments_label_81

	// *** Basic block 19

.SkipSpacesAndComments_label_95:
	bge         s1, s1, .SkipSpacesAndComments_label_113

	// *** Basic block 20

.SkipSpacesAndComments_label_99:
	mv          t0, s1
	addi        s1, s1, 1
	add         t1, s3, t0
	lb          s5, 0(t1)

	// *** Basic block 21

.SkipSpacesAndComments_label_105:
	slt         t0, s1, s1
	bge         s1, s1, .SkipSpacesAndComments_label_111

	// *** Basic block 22

	addi        t1, s5, -47
	snez        t0, t1

	// *** Basic block 23

.SkipSpacesAndComments_label_111:
	bnez        t0, .SkipSpacesAndComments_label_80

	// *** Basic block 24

.SkipSpacesAndComments_label_113:
	mv          a0, s1
	j           .SkipSpacesAndComments_label_64

	// *** Basic block 25

.SkipSpacesAndComments_label_117:

	// *** Basic block 26

.SkipSpacesAndComments_label_118:

	// *** Basic block 27

.SkipSpacesAndComments_label_119:
	mv          a0, s5
	call        isblank

	// *** Basic block 28

	slli        t0, a0, 56
	srai        t0, t0, 56
	not         t0, t0
	bnez        t0, .SkipSpacesAndComments_label_142

	// *** Basic block 29

.SkipSpacesAndComments_label_127:
	addi        s1, s1, 1
	beq         s2, x0, .SkipSpacesAndComments_label_139

	// *** Basic block 30

	li          t0, 32		// 0x20 ASCII ' '
	mv          a1, t0
	mv          a0, s2
	call        StringAppendChar

	// *** Basic block 31

	mv          s2, x0

	// *** Basic block 32

.SkipSpacesAndComments_label_139:
	blt         s1, s1, .SkipSpacesAndComments_label_32

	// *** Basic block 33

.SkipSpacesAndComments_label_142:
	mv          a0, s1
	j           .SkipSpacesAndComments_label_64
.func_end_SkipSpacesAndComments:
	.size SkipSpacesAndComments, .func_end_SkipSpacesAndComments-SkipSpacesAndComments

	.local  ReadIdentifier
	.type ReadIdentifier, @function

ReadIdentifier:

	// *** Basic block 0

	.global StringInit
	.global isalpha
	.global isalnum
	.global StringAppendChar
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
	mv          s3, a0
	mv          a1, x0
	mv          a0, s1
	call        StringInit

	// *** Basic block 1

	ld          s5, 24(s3)
	slt         s4, s2, s5
	bge         s2, s5, .ReadIdentifier_label_49

	// *** Basic block 2

	ld          s6, 16(s3)
	ld          t0, 16(s3)
	add         t0, t0, s2
	lb          s7, 0(t0)
	mv          a0, s7
	call        isalpha

	// *** Basic block 3

	bnez        a0, .ReadIdentifier_label_48

	// *** Basic block 4

	addi        t0, s7, -95
	seqz        s4, t0

	// *** Basic block 5

.ReadIdentifier_label_48:

	// *** Basic block 6

.ReadIdentifier_label_49:
	beqz        s4, .ReadIdentifier_label_80

	// *** Basic block 7

	bge         s2, s5, .ReadIdentifier_label_79

	// *** Basic block 8

.ReadIdentifier_label_52:
	add         t0, s6, s2
	lb          s6, 0(t0)
	mv          a0, s6
	call        isalnum

	// *** Basic block 9

	slli        t0, a0, 56
	srai        t0, t0, 56
	not         s4, t0
	beqz        s4, .ReadIdentifier_label_65

	// *** Basic block 10

	addi        t0, s6, -95
	snez        s4, t0

	// *** Basic block 11

.ReadIdentifier_label_65:
	bnez        s4, .ReadIdentifier_label_79

	// *** Basic block 12

.ReadIdentifier_label_67:
	mv          t0, s2
	addi        s2, s2, 1
	add         t1, s6, t0
	lb          a1, 0(t1)
	mv          a0, s1
	call        StringAppendChar

	// *** Basic block 13

	blt         s2, s5, .ReadIdentifier_label_52

	// *** Basic block 14

.ReadIdentifier_label_79:

	// *** Basic block 15

.ReadIdentifier_label_80:
	mv          a0, s2

	// *** Basic block 16

.ReadIdentifier_label_83:
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
.func_end_ReadIdentifier:
	.size ReadIdentifier, .func_end_ReadIdentifier-ReadIdentifier

	.local  ReadString
	.type ReadString, @function

ReadString:

	// *** Basic block 0

	.global StringAppendChar
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
	mv          s3, a2
	ld          s4, 24(s2)
	slt         t0, s1, s4
	bge         s1, s4, .ReadString_label_33

	// *** Basic block 1

	ld          t1, 16(s2)
	ld          t2, 16(s2)
	add         t2, t2, s1
	lb          t2, 0(t2)
	addi        t2, t2, -34
	seqz        t0, t2

	// *** Basic block 2

.ReadString_label_33:
	beqz        t0, .ReadString_label_89

	// *** Basic block 3

	addi        s1, s1, 1
	bge         s1, s4, .ReadString_label_88

	// *** Basic block 4

.ReadString_label_38:
	add         t0, t1, s1
	lb          s5, 0(t0)
	li          t0, 92		// 0x5c ASCII '\'
	bne         s5, t0, .ReadString_label_69

	// *** Basic block 5

	mv          t0, s1
	addi        s1, s1, 1
	add         t1, t1, t0
	lb          a1, 0(t1)
	mv          a0, s3
	call        StringAppendChar

	// *** Basic block 6

	beq         s1, s4, .ReadString_label_88

	// *** Basic block 7

.ReadString_label_58:
	mv          t0, s1
	addi        s1, s1, 1
	add         t1, t1, t0
	lb          a1, 0(t1)
	mv          a0, s3
	call        StringAppendChar

	// *** Basic block 8

	j           .ReadString_label_38

	// *** Basic block 9

.ReadString_label_69:
	li          t0, 34		// 0x22 ASCII '"'
	bne         s5, t0, .ReadString_label_76

	// *** Basic block 10

	addi        s1, s1, 1
	j           .ReadString_label_88

	// *** Basic block 11

.ReadString_label_76:
	mv          t0, s1
	addi        s1, s1, 1
	add         t1, t1, t0
	lb          a1, 0(t1)
	mv          a0, s3
	call        StringAppendChar

	// *** Basic block 12

	blt         s1, s4, .ReadString_label_38

	// *** Basic block 13

.ReadString_label_88:

	// *** Basic block 14

.ReadString_label_89:
	mv          a0, s1

	// *** Basic block 15

.ReadString_label_92:
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
.func_end_ReadString:
	.size ReadString, .func_end_ReadString-ReadString

	.local  MacroEqual
	.type MacroEqual, @function

MacroEqual:

	// *** Basic block 0

	.global StringEqualString
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
	mv          s1, a0
	mv          t0, a1
	mv          s2, a3
	mv          s3, a4
	mv          s4, a2
	lb          t1, 145(s1)
	beqz        t1, .MacroEqual_label_37

	// *** Basic block 1

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 2

.MacroEqual_label_34:
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

	// *** Basic block 3

.MacroEqual_label_37:
	lb          t2, 144(s1)
	sub         t3, t2, t0
	snez        t1, t3
	bne         t2, t0, .MacroEqual_label_48

	// *** Basic block 4

	lb          t0, 146(s1)
	sub         t0, t0, s2
	snez        t1, t0

	// *** Basic block 5

.MacroEqual_label_48:
	beqz        t1, .MacroEqual_label_53

	// *** Basic block 6

	mv          a0, x0
	j           .MacroEqual_label_34

	// *** Basic block 7

.MacroEqual_label_53:
	addi        a0, s1, 104
	mv          a1, s3
	call        StringEqualString

	// *** Basic block 8

	not         t0, a0
	beqz        t0, .MacroEqual_label_64

	// *** Basic block 9

	mv          a0, x0
	j           .MacroEqual_label_34

	// *** Basic block 10

.MacroEqual_label_64:
	addi        t0, s1, 80
	ld          t0, 8(t0)
	ld          s3, 8(s4)
	beq         t0, s3, .MacroEqual_label_78

	// *** Basic block 11

	ld          t0, 80(s1)
	ld          t1, 0(s4)
	mv          a0, x0
	j           .MacroEqual_label_34

	// *** Basic block 12

.MacroEqual_label_78:
	mv          s5, x0
	bge         x0, s3, .MacroEqual_label_107

	// *** Basic block 13

.MacroEqual_label_83:
	slli        t0, s5, 3
	add         t1, t0, t0
	ld          s6, 0(t1)
	add         t0, t1, t0
	ld          s7, 0(t0)
	mv          a1, s7
	mv          a0, s6
	call        StringEqualString

	// *** Basic block 14

	not         t0, a0
	beqz        t0, .MacroEqual_label_102

	// *** Basic block 15

	mv          a0, x0
	j           .MacroEqual_label_34

	// *** Basic block 16

.MacroEqual_label_102:

	// *** Basic block 17

.MacroEqual_label_103:
	addi        s5, s5, 1
	bge         s5, s3, .MacroEqual_label_83

	// *** Basic block 18

.MacroEqual_label_107:
	li          a0, 1		// 0x1 ASCII \x1
	j           .MacroEqual_label_34
.func_end_MacroEqual:
	.size MacroEqual, .func_end_MacroEqual-MacroEqual

	.local  FindFileInPath
	.type FindFileInPath, @function

FindFileInPath:

	// *** Basic block 0

	.global StringPrintf
	.global fopen
	.global StringSetString
	.global StringDestruct
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a2
	mv          t0, a0
	mv          s2, a1
	ld          s3, 0(s1)
	ld          s4, 8(t0)
	bge         s3, s4, .FindFileInPath_label_96

	// *** Basic block 1

	ld          s5, 0(t0)
	ld          s6, 16(s2)

	// *** Basic block 2

.FindFileInPath_label_32:
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	addi        a0, s0, -64
	lla         a1, .str.139
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          t0, 0(t0)
	ld          a2, 16(t0)
	mv          a3, s6
	call        StringPrintf

	// *** Basic block 3

	addi        t0, s0, -64
	ld          a0, 16(t0)
	lla         a1, .str.140
	call        fopen

	// *** Basic block 4

	mv          s5, a0
	beq         s5, x0, .FindFileInPath_label_88

	// *** Basic block 5

	addi        a1, s0, -64
	mv          a0, s2
	call        StringSetString

	// *** Basic block 6

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 7

	sd          s3, 0(s1)
	mv          a0, s5

	// *** Basic block 8

.FindFileInPath_label_85:
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

	// *** Basic block 9

.FindFileInPath_label_88:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 10

.FindFileInPath_label_92:
	addi        s3, s3, 1
	bge         s3, s4, .FindFileInPath_label_32

	// *** Basic block 11

.FindFileInPath_label_96:
	mv          a0, x0
	j           .FindFileInPath_label_85
.func_end_FindFileInPath:
	.size FindFileInPath, .func_end_FindFileInPath-FindFileInPath

	.local  ReadMacroFormalArguments
	.type ReadMacroFormalArguments, @function

ReadMacroFormalArguments:

	// *** Basic block 0

	.local SkipSpacesAndComments
	.local ReadIdentifier
	.global StringEqualString
	.global PreprocessorError
	.global VectorAppend
	.global NewString
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -64(s0)
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
	mv          s1, a2
	mv          s2, a0
	mv          s3, a1
	mv          s4, a5
	mv          s5, a4
	mv          s6, a3
	addi        s1, s1, 1
	mv          a3, x0
	mv          a2, s3
	mv          a1, s1
	call        SkipSpacesAndComments

	// *** Basic block 1

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 0(s4)
	ld          s7, 8(s5)
	ld          s8, 0(s5)
	mv          s1, a0
	ld          s9, 16(s3)
	add         t0, s9, s1
	lb          t0, 0(t0)
	li          s10, 41		// 0x29 ASCII ')'
	beq         t0, s10, .ReadMacroFormalArguments_label_182

	// *** Basic block 2

.ReadMacroFormalArguments_label_66:
	mv          a3, x0
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        SkipSpacesAndComments

	// *** Basic block 3

	mv          s1, a0
	add         t1, s9, s1
	lb          t1, 0(t1)
	addi        t2, t1, -46
	seqz        t0, t2
	li          t2, 46		// 0x2e ASCII '.'
	bne         t1, t2, .ReadMacroFormalArguments_label_91

	// *** Basic block 4

	addi        t1, s1, 1
	add         t1, s9, t1
	lb          t1, 0(t1)
	addi        t1, t1, -46
	seqz        t0, t1

	// *** Basic block 5

.ReadMacroFormalArguments_label_91:
	beqz        t0, .ReadMacroFormalArguments_label_98

	// *** Basic block 6

	addi        t1, s1, 2
	add         t1, s9, t1
	lb          t1, 0(t1)
	addi        t1, t1, -46
	seqz        t0, t1

	// *** Basic block 7

.ReadMacroFormalArguments_label_98:
	beqz        t0, .ReadMacroFormalArguments_label_102

	// *** Basic block 8

	addi        s1, s1, 3
	j           .ReadMacroFormalArguments_label_182

	// *** Basic block 9

.ReadMacroFormalArguments_label_102:
	addi        a2, s0, -64
	mv          a1, s1
	mv          a0, s3
	call        ReadIdentifier

	// *** Basic block 10

	mv          s1, a0
	mv          s4, x0
	bge         x0, s7, .ReadMacroFormalArguments_label_146

	// *** Basic block 11

.ReadMacroFormalArguments_label_116:
	slli        t0, s4, 3
	add         t0, s8, t0
	ld          a0, 0(t0)
	addi        a1, s0, -64
	call        StringEqualString

	// *** Basic block 12

	beqz        a0, .ReadMacroFormalArguments_label_141

	// *** Basic block 13

	lla         a1, .str.141
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s2
	call        PreprocessorError

	// *** Basic block 14

	mv          a0, s1

	// *** Basic block 15

.ReadMacroFormalArguments_label_138:
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

	// *** Basic block 16

.ReadMacroFormalArguments_label_141:

	// *** Basic block 17

.ReadMacroFormalArguments_label_142:
	addi        s4, s4, 1
	bge         s4, s7, .ReadMacroFormalArguments_label_116

	// *** Basic block 18

.ReadMacroFormalArguments_label_146:
	addi        t0, s0, -64
	ld          a0, 16(t0)
	call        NewString

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s5
	call        VectorAppend

	// *** Basic block 20

	mv          a3, x0
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        SkipSpacesAndComments

	// *** Basic block 21

	mv          s1, a0
	add         t0, s9, s1
	lb          t0, 0(t0)
	li          t1, 44		// 0x2c ASCII ','
	bne         t0, t1, .ReadMacroFormalArguments_label_182

	// *** Basic block 22

.ReadMacroFormalArguments_label_174:
	addi        s1, s1, 1
	add         t0, s9, s1
	lb          t0, 0(t0)
	bne         t0, s10, .ReadMacroFormalArguments_label_66

	// *** Basic block 23

.ReadMacroFormalArguments_label_182:
	add         t0, s9, s1
	lb          t0, 0(t0)
	beq         t0, s10, .ReadMacroFormalArguments_label_201

	// *** Basic block 24

	lla         a1, .str.142
	ld          a2, 16(s6)
	mv          a0, s2
	call        PreprocessorError

	// *** Basic block 25

	mv          a0, s1
	j           .ReadMacroFormalArguments_label_138

	// *** Basic block 26

.ReadMacroFormalArguments_label_201:
	addi        s1, s1, 1
	mv          a0, s1
	j           .ReadMacroFormalArguments_label_138
.func_end_ReadMacroFormalArguments:
	.size ReadMacroFormalArguments, .func_end_ReadMacroFormalArguments-ReadMacroFormalArguments

	.local  CheckHashHash
	.type CheckHashHash, @function

CheckHashHash:

	// *** Basic block 0

	.local TokenIteratorInit
	.local CurrentToken
	.global PreprocessorError
	.local MoveToNextToken
	.local PrevToken
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          t0, a1
	addi        a0, s0, -64
	mv          a2, t0
	mv          a1, s1
	call        TokenIteratorInit

	// *** Basic block 1

	addi        a0, s0, -64
	call        CurrentToken

	// *** Basic block 2

	li          s2, 11		// 0xb ASCII \xb
	bne         a0, s2, .CheckHashHash_label_41

	// *** Basic block 3

	lla         a1, .str.143
	mv          a0, s1
	call        PreprocessorError

	// *** Basic block 4

.CheckHashHash_label_41:
	addi        a0, s0, -64
	call        CurrentToken

	// *** Basic block 5

	li          s3, 19		// 0x13 ASCII \x13
	beq         a0, s3, .CheckHashHash_label_61

	// *** Basic block 6

.CheckHashHash_label_50:
	addi        a0, s0, -64
	call        MoveToNextToken

	// *** Basic block 7

	addi        a0, s0, -64
	call        CurrentToken

	// *** Basic block 8

	bne         a0, s3, .CheckHashHash_label_50

	// *** Basic block 9

.CheckHashHash_label_61:
	addi        a0, s0, -64
	call        PrevToken

	// *** Basic block 10

	bne         a0, s2, .CheckHashHash_label_75

	// *** Basic block 11

	lla         a1, .str.144
	mv          a0, s1
	call        PreprocessorError

	// *** Basic block 12

.CheckHashHash_label_75:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CheckHashHash:
	.size CheckHashHash, .func_end_CheckHashHash-CheckHashHash

	.local  Define
	.type Define, @function

Define:

	// *** Basic block 0

	.local ReadIdentifier
	.local ReadMacroFormalArguments
	.local SkipSpacesAndComments
	.global StringInit
	.local Tokenize
	.local CheckHashHash
	.global HashTableSearch
	.global NewMacro
	.global NewSourceLocation
	.global HashTableInsert
	.global printf
	.global abort
	.local MacroEqual
	.global DecodeSourceLocation
	.global PreprocessorWarning
	.global StringDestruct
	addi sp, sp, -288
	// Saved return address (offset 280) and frame pointer (offset 272)
	sd ra, 280(sp)
	sd s0, 272(sp)
	addi s0, sp, 288
	// Local vars at offset -192(s0)
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
	mv          s1, a0
	mv          s2, a2
	mv          s3, a1
	lb          t0, 168(s1)
	not         t0, t0
	beqz        t0, .Define_label_51

	// *** Basic block 1

.Define_label_48:
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

	// *** Basic block 2

.Define_label_51:
	mv          s4, s2
	addi        a2, s0, -192
	mv          a1, s2
	mv          a0, s3
	call        ReadIdentifier

	// *** Basic block 3

	mv          s2, a0
	mv          s5, s2
	mv          s6, x0
	sb          x0, -152(s0)
	sd          x0, -144(s0)
	sd          x0, -136(s0)
	sd          x0, -128(s0)
	sd          x0, -144(s0)
	ld          t0, 16(s3)
	add         t0, t0, s2
	lb          t0, 0(t0)
	li          t1, 40		// 0x28 ASCII '('
	bne         t0, t1, .Define_label_104

	// *** Basic block 4

	li          s6, 1		// 0x1 ASCII \x1
	addi        a3, s0, -192
	addi        a4, s0, -144
	addi        a5, s0, -152
	mv          a2, s2
	mv          a1, s3
	mv          a0, s1
	call        ReadMacroFormalArguments

	// *** Basic block 5

	mv          s2, a0

	// *** Basic block 6

.Define_label_104:
	mv          a3, x0
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        SkipSpacesAndComments

	// *** Basic block 7

	mv          s2, a0
	addi        a0, s0, -120
	ld          t0, 16(s3)
	add         a1, t0, s2
	call        StringInit

	// *** Basic block 8

	addi        a1, s0, -120
	addi        a2, s0, -80
	ld          t0, 160(s1)
	lb          a5, 178(t0)
	mv          a6, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a4, t0
	mv          a3, x0
	mv          a0, s1
	call        Tokenize

	// *** Basic block 9

	addi        a1, s0, -80
	mv          a0, s1
	call        CheckHashHash

	// *** Basic block 10

	addi        t0, s0, -192
	ld          s7, 16(t0)
	mv          a1, s7
	mv          a0, s1
	call        HashTableSearch

	// *** Basic block 11

	mv          s8, a0
	bne         s8, x0, .Define_label_218

	// *** Basic block 12

	lb          s9, -152(s0)
	addi        s10, s0, -144
	addi        s11, s0, -80
	ld          t0, 160(s1)
	ld          a0, 0(t0)
	lw          a1, 80(a0)
	mv          a3, s5
	mv          a2, s4
	call        NewSourceLocation

	// *** Basic block 13

	mv          a5, a0
	mv          a4, s11
	mv          a3, s10
	mv          a2, s9
	mv          a1, s6
	mv          a0, s7
	call        NewMacro

	// *** Basic block 14

	mv          s8, a0
	mv          a1, s8
	mv          a0, s1
	call        HashTableInsert

	// *** Basic block 15

	mv          s9, a0
	beqz        s9, .Define_label_201

	// *** Basic block 16

	j           .Define_label_216

	// *** Basic block 17

.Define_label_201:
	lla         a0, .str.145
	lla         a1, .str.146
	lla         a3, .str.147
	li          t0, 1553		// 0x611
	mv          a2, t0
	call        printf

	// *** Basic block 18

	call        abort

	// *** Basic block 19

.Define_label_216:
	j           .Define_label_265

	// *** Basic block 20

.Define_label_218:
	addi        a2, s0, -144
	lb          a3, -152(s0)
	addi        a4, s0, -80
	mv          a1, s6
	mv          a0, s8
	call        MacroEqual

	// *** Basic block 21

	not         t0, a0
	beqz        t0, .Define_label_262

	// *** Basic block 22

	ld          a0, 152(s8)
	addi        a1, s0, -40
	addi        a2, s0, -32
	addi        a3, s0, -28
	addi        a4, s0, -24
	call        DecodeSourceLocation

	// *** Basic block 23

	lla         a1, .str.148
	lla         a2, .str.149
	ld          a4, -40(s0)
	lw          a5, -32(s0)
	mv          a3, s7
	mv          a0, s1
	call        PreprocessorWarning

	// *** Basic block 24

.Define_label_262:
	sb          x0, 145(s8)

	// *** Basic block 25

.Define_label_265:
	addi        a0, s0, -192
	call        StringDestruct

	// *** Basic block 26

	addi        a0, s0, -80
	call        StringDestruct

	// *** Basic block 27

	j           .Define_label_48
.func_end_Define:
	.size Define, .func_end_Define-Define

	.local  Undef
	.type Undef, @function

Undef:

	// *** Basic block 0

	.local ReadIdentifier
	.global HashTableSearch
	.local SkipSpacesAndComments
	.global PreprocessorWarning
	.global StringDestruct
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a2
	mv          s3, a1
	lb          t0, 168(s1)
	not         t0, t0
	beqz        t0, .Undef_label_32

	// *** Basic block 1

.Undef_label_29:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.Undef_label_32:
	addi        a2, s0, -64
	mv          a1, s2
	mv          a0, s3
	call        ReadIdentifier

	// *** Basic block 3

	mv          s2, a0
	addi        t0, s0, -64
	ld          a1, 16(t0)
	mv          a0, s1
	call        HashTableSearch

	// *** Basic block 4

	mv          s4, a0
	beq         s4, x0, .Undef_label_58

	// *** Basic block 5

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 145(s4)

	// *** Basic block 6

.Undef_label_58:
	mv          a3, x0
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        SkipSpacesAndComments

	// *** Basic block 7

	mv          s2, a0
	ld          t0, 24(s3)
	bge         s2, t0, .Undef_label_83

	// *** Basic block 8

	lla         a1, .str.150
	lla         a2, .str.151
	mv          a0, s1
	call        PreprocessorWarning

	// *** Basic block 9

.Undef_label_83:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 10

	j           .Undef_label_29
.func_end_Undef:
	.size Undef, .func_end_Undef-Undef

	.global PreprocessorParseIncludeFilename
	.type PreprocessorParseIncludeFilename, @function

PreprocessorParseIncludeFilename:

	// *** Basic block 0

	.global PreprocessorError
	.global StringInitFromSegment
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
	mv          s1, a2
	mv          s2, a1
	mv          s3, a4
	mv          s4, a0
	mv          s5, a3
	ld          s6, 0(s1)
	ld          s7, 16(s2)
	add         t0, s7, s6
	lb          s8, 0(t0)
	li          t0, 60		// 0x3c ASCII '<'
	bne         s8, t0, .PreprocessorParseIncludeFilename_label_84

	// *** Basic block 1

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 0(s3)
	addi        s6, s6, 1
	mv          s9, s6
	ld          s10, 24(s2)
	bge         s9, s10, .PreprocessorParseIncludeFilename_label_64

	// *** Basic block 2

.PreprocessorParseIncludeFilename_label_52:
	add         t0, s7, s9
	lb          t0, 0(t0)
	li          t1, 62		// 0x3e ASCII '>'
	beq         t0, t1, .PreprocessorParseIncludeFilename_label_64

	// *** Basic block 3

.PreprocessorParseIncludeFilename_label_60:
	addi        s9, s9, 1
	blt         s9, s10, .PreprocessorParseIncludeFilename_label_52

	// *** Basic block 4

.PreprocessorParseIncludeFilename_label_64:
	add         t0, s7, s9
	lb          t0, 0(t0)
	li          t1, 62		// 0x3e ASCII '>'
	beq         t0, t1, .PreprocessorParseIncludeFilename_label_82

	// *** Basic block 5

	lla         a1, .str.152
	mv          a0, s4
	call        PreprocessorError

	// *** Basic block 6

	mv          a0, x0

	// *** Basic block 7

.PreprocessorParseIncludeFilename_label_79:
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

	// *** Basic block 8

.PreprocessorParseIncludeFilename_label_82:
	j           .PreprocessorParseIncludeFilename_label_136

	// *** Basic block 9

.PreprocessorParseIncludeFilename_label_84:
	li          s10, 34		// 0x22 ASCII '"'
	bne         s8, s10, .PreprocessorParseIncludeFilename_label_125

	// *** Basic block 10

	addi        s6, s6, 1
	mv          s9, s6
	ld          s8, 24(s2)
	bge         s9, s8, .PreprocessorParseIncludeFilename_label_107

	// *** Basic block 11

.PreprocessorParseIncludeFilename_label_96:
	add         t0, s7, s9
	lb          t0, 0(t0)
	beq         t0, s10, .PreprocessorParseIncludeFilename_label_107

	// *** Basic block 12

.PreprocessorParseIncludeFilename_label_103:
	addi        s9, s9, 1
	blt         s9, s8, .PreprocessorParseIncludeFilename_label_96

	// *** Basic block 13

.PreprocessorParseIncludeFilename_label_107:
	add         t0, s7, s9
	lb          t0, 0(t0)
	beq         t0, s10, .PreprocessorParseIncludeFilename_label_123

	// *** Basic block 14

	lla         a1, .str.153
	mv          a0, s4
	call        PreprocessorError

	// *** Basic block 15

	mv          a0, x0
	j           .PreprocessorParseIncludeFilename_label_79

	// *** Basic block 16

.PreprocessorParseIncludeFilename_label_123:
	j           .PreprocessorParseIncludeFilename_label_135

	// *** Basic block 17

.PreprocessorParseIncludeFilename_label_125:
	lla         a1, .str.154
	mv          a0, s4
	call        PreprocessorError

	// *** Basic block 18

	mv          a0, x0
	j           .PreprocessorParseIncludeFilename_label_79

	// *** Basic block 19

.PreprocessorParseIncludeFilename_label_135:

	// *** Basic block 20

.PreprocessorParseIncludeFilename_label_136:
	bne         s9, s6, .PreprocessorParseIncludeFilename_label_149

	// *** Basic block 21

	lla         a1, .str.155
	mv          a0, s4
	call        PreprocessorError

	// *** Basic block 22

	mv          a0, x0
	j           .PreprocessorParseIncludeFilename_label_79

	// *** Basic block 23

.PreprocessorParseIncludeFilename_label_149:
	sub         s7, s9, s6
	ld          t0, 16(s2)
	add         a1, t0, s6
	mv          a2, s7
	mv          a0, s5
	call        StringInitFromSegment

	// *** Basic block 24

	addi        t0, s9, 1
	sd          t0, 0(s1)
	li          a0, 1		// 0x1 ASCII \x1
	j           .PreprocessorParseIncludeFilename_label_79
.func_end_PreprocessorParseIncludeFilename:
	.size PreprocessorParseIncludeFilename, .func_end_PreprocessorParseIncludeFilename-PreprocessorParseIncludeFilename

	.local  PrintSearchDetails
	.type PrintSearchDetails, @function

PrintSearchDetails:

	// *** Basic block 0

	.global ReportNote
	.global StringPrintf
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -96(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a2
	mv          s2, a0
	mv          s3, a1
	lla         a2, .str.156
	mv          a1, x0
	mv          a0, x0
	call        ReportNote

	// *** Basic block 1

	not         t0, s1
	beqz        t0, .PrintSearchDetails_label_91

	// *** Basic block 2

	mv          s1, x0
	addi        t0, s2, 112
	ld          s4, 8(t0)
	bge         x0, s4, .PrintSearchDetails_label_90

	// *** Basic block 3

	ld          s5, 112(s2)

	// *** Basic block 4

.PrintSearchDetails_label_45:
	sd          x0, -96(s0)
	sd          x0, -88(s0)
	sd          x0, -80(s0)
	sd          x0, -72(s0)
	sd          x0, -64(s0)
	sb          x0, -96(s0)
	addi        a0, s0, -96
	lla         a1, .str.157
	slli        t0, s1, 3
	add         t0, s5, t0
	ld          t0, 0(t0)
	ld          a2, 16(t0)
	mv          a3, s3
	call        StringPrintf

	// *** Basic block 5

	lla         a2, .str.158
	addi        t0, s0, -96
	ld          a3, 16(t0)
	mv          a1, x0
	mv          a0, x0
	call        ReportNote

	// *** Basic block 6

.PrintSearchDetails_label_86:
	addi        s1, s1, 1
	bge         s1, s4, .PrintSearchDetails_label_45

	// *** Basic block 7

.PrintSearchDetails_label_90:

	// *** Basic block 8

.PrintSearchDetails_label_91:
	mv          s4, x0
	addi        t0, s2, 136
	ld          s5, 8(t0)
	bge         x0, s5, .PrintSearchDetails_label_142

	// *** Basic block 9

	ld          s6, 136(s2)

	// *** Basic block 10

.PrintSearchDetails_label_100:
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -24(s0)
	sb          x0, -56(s0)
	addi        a0, s0, -56
	lla         a1, .str.159
	slli        t0, s4, 3
	add         t0, s6, t0
	ld          t0, 0(t0)
	ld          a2, 16(t0)
	mv          a3, s3
	call        StringPrintf

	// *** Basic block 11

	lla         a2, .str.160
	addi        t0, s0, -56
	ld          a3, 16(t0)
	mv          a1, x0
	mv          a0, x0
	call        ReportNote

	// *** Basic block 12

.PrintSearchDetails_label_138:
	addi        s4, s4, 1
	bge         s4, s5, .PrintSearchDetails_label_100

	// *** Basic block 13

.PrintSearchDetails_label_142:
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
.func_end_PrintSearchDetails:
	.size PrintSearchDetails, .func_end_PrintSearchDetails-PrintSearchDetails

	.local  DoInclude
	.type DoInclude, @function

DoInclude:

	// *** Basic block 0

	.local Tokenize
	.local ReplaceMacrosInTokenizedLine
	.local TokenIteratorInit
	.local CurrentToken
	.local GetCurrentTokenSpelling
	.global PreprocessorError
	.global StringDestruct
	.local FindFileInPath
	.local PrintSearchDetails
	.global exit
	.global NewSourceFromFile
	.global compiler
	.global printf
	addi sp, sp, -208
	// Saved return address (offset 200) and frame pointer (offset 192)
	sd ra, 200(sp)
	sd s0, 192(sp)
	addi s0, sp, 208
	// Local vars at offset -144(s0)
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
	mv          s3, a2
	mv          s4, a3
	lb          t0, 168(s1)
	not         t0, t0
	beqz        t0, .DoInclude_label_52

	// *** Basic block 1

.DoInclude_label_49:
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

	// *** Basic block 2

.DoInclude_label_52:
	addi        a2, s0, -144
	li          s5, 1		// 0x1 ASCII \x1
	mv          a6, s5
	mv          a5, x0
	mv          a4, s5
	mv          a3, s3
	mv          a1, s2
	mv          a0, s1
	call        Tokenize

	// *** Basic block 3

	addi        a1, s0, -144
	mv          a2, s5
	mv          a0, s1
	call        ReplaceMacrosInTokenizedLine

	// *** Basic block 4

	addi        a0, s0, -104
	addi        a2, s0, -144
	mv          a1, s1
	call        TokenIteratorInit

	// *** Basic block 5

	mv          s6, x0
	addi        a0, s0, -104
	call        CurrentToken

	// *** Basic block 6

	li          t0, 6		// 0x6 ASCII \x6
	bne         a0, t0, .DoInclude_label_108

	// *** Basic block 7

	addi        a0, s0, -104
	addi        a1, s0, -64
	call        GetCurrentTokenSpelling

	// *** Basic block 8

	j           .DoInclude_label_136

	// *** Basic block 9

.DoInclude_label_108:
	addi        a0, s0, -104
	call        CurrentToken

	// *** Basic block 10

	li          t0, 18		// 0x12 ASCII \x12
	bne         a0, t0, .DoInclude_label_124

	// *** Basic block 11

	addi        a0, s0, -104
	addi        a1, s0, -64
	call        GetCurrentTokenSpelling

	// *** Basic block 12

	li          s6, 1		// 0x1 ASCII \x1
	j           .DoInclude_label_135

	// *** Basic block 13

.DoInclude_label_124:
	lla         a1, .str.161
	mv          a0, s1
	call        PreprocessorError

	// *** Basic block 14

	addi        a0, s0, -144
	call        StringDestruct

	// *** Basic block 15

	j           .DoInclude_label_49

	// *** Basic block 16

.DoInclude_label_135:

	// *** Basic block 17

.DoInclude_label_136:
	mv          s2, x0
	sd          s4, -24(s0)
	not         t0, s6
	beqz        t0, .DoInclude_label_151

	// *** Basic block 18

	addi        a0, s1, 112
	addi        a1, s0, -64
	addi        a2, s0, -24
	call        FindFileInPath

	// *** Basic block 19

	mv          s2, a0

	// *** Basic block 20

.DoInclude_label_151:
	bne         s2, x0, .DoInclude_label_163

	// *** Basic block 21

	addi        a0, s1, 136
	addi        a1, s0, -64
	addi        a2, s0, -24
	call        FindFileInPath

	// *** Basic block 22

	mv          s2, a0

	// *** Basic block 23

.DoInclude_label_163:
	bne         s2, x0, .DoInclude_label_193

	// *** Basic block 24

	beqz        s4, .DoInclude_label_170

	// *** Basic block 25

	j           .DoInclude_label_49

	// *** Basic block 26

.DoInclude_label_170:
	lla         a1, .str.162
	addi        t0, s0, -64
	ld          s3, 16(t0)
	mv          a2, s3
	mv          a0, s1
	call        PreprocessorError

	// *** Basic block 27

	mv          a2, s6
	mv          a1, s3
	mv          a0, s1
	call        PrintSearchDetails

	// *** Basic block 28

	mv          a0, s5
	call        exit

	// *** Basic block 29

.DoInclude_label_193:
	addi        t0, s0, -64
	ld          s3, 16(t0)
	mv          a1, s2
	mv          a0, s3
	call        NewSourceFromFile

	// *** Basic block 30

	mv          s5, a0
	ld          t0, 160(s1)
	ld          t0, 0(t0)
	sd          t0, 88(s5)
	la          t0, compiler
	ld          s7, 0(t0)
	lb          t0, 1233(s7)
	beqz        t0, .DoInclude_label_220

	// *** Basic block 31

	lla         a0, .str.163
	mv          a1, s3
	call        printf

	// *** Basic block 32

.DoInclude_label_220:
	ld          t0, 160(s1)
	ld          t0, 0(t0)
	ld          t1, 1088(s7)
	sd          t1, 120(t0)
	la          t0, compiler
	ld          t0, 0(t0)
	ld          t1, -24(s0)
	sd          t1, 1088(t0)
	ld          t0, 160(s1)
	sd          s5, 0(t0)
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 33

	addi        a0, s0, -144
	call        StringDestruct

	// *** Basic block 34

	j           .DoInclude_label_49
.func_end_DoInclude:
	.size DoInclude, .func_end_DoInclude-DoInclude

	.local  Include
	.type Include, @function

Include:

	// *** Basic block 0

	.local DoInclude
	// Leaf procedure, no stack frame generated
	mv          a3, x0
	j           DoInclude
.func_end_Include:
	.size Include, .func_end_Include-Include

	.local  IncludeNext
	.type IncludeNext, @function

IncludeNext:

	// *** Basic block 0

	.local DoInclude
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 160(t0)
	ld          t1, 0(t1)
	ld          t1, 120(t1)
	addi        a3, t1, 1
	j           DoInclude
.func_end_IncludeNext:
	.size IncludeNext, .func_end_IncludeNext-IncludeNext

	.local  UpdateState
	.type UpdateState, @function

UpdateState:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, x0
	addi        t2, t0, 88
	ld          t2, 8(t2)
	bge         x0, t2, .UpdateState_label_46

	// *** Basic block 1

	ld          t3, 88(t0)
	li          t4, -1		// 0xffffffffffffffff

	// *** Basic block 2

.UpdateState_label_23:
	slli        t6, t1, 3
	add         t3, t3, t6
	ld          t3, 0(t3)
	sub         t6, t3, x0
	seqz        t5, t6
	beq         t3, x0, .UpdateState_label_34

	// *** Basic block 3

	sub         t3, t3, t4
	seqz        t5, t3

	// *** Basic block 4

.UpdateState_label_34:
	beqz        t5, .UpdateState_label_41

	// *** Basic block 5

	sb          x0, 168(t0)

	// *** Basic block 6

.UpdateState_label_38:
	ret         

	// *** Basic block 7

.UpdateState_label_41:

	// *** Basic block 8

.UpdateState_label_42:
	addi        t1, t1, 1
	bge         t1, t2, .UpdateState_label_23

	// *** Basic block 9

.UpdateState_label_46:
	li          t2, 1		// 0x1 ASCII \x1
	sb          t2, 168(t0)
	j           .UpdateState_label_38
.func_end_UpdateState:
	.size UpdateState, .func_end_UpdateState-UpdateState

	.local  EvaluateExpression
	.type EvaluateExpression, @function

EvaluateExpression:

	// *** Basic block 0

	.global LexInitFromString
	.global LexNextToken
	.global SyntaxInit
	.global abort_on_error
	.global setjmp
	.global error_abort_state
	.global SyntaxParseExpression
	.global AnalyzeExpression
	.global EvaluateIntegerExpression
	.local true_value
	.global SyntaxDestruct
	.global LexDestruct
	addi sp, sp, -416
	// Saved return address (offset 408) and frame pointer (offset 400)
	sd ra, 408(sp)
	sd s0, 400(sp)
	addi s0, sp, 416
	// Local vars at offset -368(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          t0, a1
	li          s2, 1		// 0x1 ASCII \x1
	sb          s2, 168(s1)
	ld          s3, 160(s1)
	addi        a0, s0, -368
	ld          t1, 0(s3)
	ld          a1, 16(t1)
	mv          a3, s1
	mv          a2, t0
	call        LexInitFromString

	// *** Basic block 1

	addi        t0, s0, -368
	sb          s2, 176(t0)
	addi        t0, s0, -368
	lb          t1, 178(s3)
	sb          t1, 178(t0)
	addi        a0, s0, -368
	call        LexNextToken

	// *** Basic block 2

	addi        a0, s0, -184
	addi        a1, s0, -368
	call        SyntaxInit

	// *** Basic block 3

	la          t0, abort_on_error
	lb          s4, 0(t0)
	la          t0, abort_on_error
	sb          s2, 0(t0)
	mv          s2, x0
	la          a0, error_abort_state
	call        setjmp

	// *** Basic block 4

	bnez        a0, .EvaluateExpression_label_113

	// *** Basic block 5

	addi        a0, s0, -184
	mv          a1, x0
	call        SyntaxParseExpression

	// *** Basic block 6

	mv          s5, a0
	beq         s5, x0, .EvaluateExpression_label_112

	// *** Basic block 7

	mv          a0, s5
	call        AnalyzeExpression

	// *** Basic block 8

	mv          s5, a0
	addi        a1, s0, -24
	mv          a0, s5
	call        EvaluateIntegerExpression

	// *** Basic block 9

	beqz        a0, .EvaluateExpression_label_111

	// *** Basic block 10

	ld          t0, -24(s0)
	bnez        t0, .EvaluateExpression_label_108

	// *** Basic block 11

	j           .EvaluateExpression_label_110

	// *** Basic block 12

.EvaluateExpression_label_108:
	lla         s2, true_value

	// *** Basic block 13

.EvaluateExpression_label_110:

	// *** Basic block 14

.EvaluateExpression_label_111:

	// *** Basic block 15

.EvaluateExpression_label_112:

	// *** Basic block 16

.EvaluateExpression_label_113:
	la          t0, abort_on_error
	sb          s4, 0(t0)
	addi        a0, s0, -184
	call        SyntaxDestruct

	// *** Basic block 17

	sd          x0, -368(s0)
	addi        a0, s0, -368
	call        LexDestruct

	// *** Basic block 18

	sd          s3, 160(s1)
	mv          a0, s2

	// *** Basic block 19

.EvaluateExpression_label_127:
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
.func_end_EvaluateExpression:
	.size EvaluateExpression, .func_end_EvaluateExpression-EvaluateExpression

	.local  If
	.type If, @function

If:

	// *** Basic block 0

	.global VectorPush
	.global StringInit
	.global StringAppend
	.local EvaluateExpression
	.global StringDestruct
	.local UpdateState
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	lb          t0, 168(s1)
	not         t0, t0
	beqz        t0, .If_label_36

	// *** Basic block 1

	addi        a0, s1, 88
	mv          a1, x0
	call        VectorPush

	// *** Basic block 2

.If_label_33:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.If_label_36:
	addi        a0, s0, -64
	ld          t0, 16(s2)
	add         a1, t0, s3
	call        StringInit

	// *** Basic block 4

	addi        a0, s0, -64
	lla         a1, .str.164
	call        StringAppend

	// *** Basic block 5

	addi        a1, s0, -64
	mv          a0, s1
	call        EvaluateExpression

	// *** Basic block 6

	mv          s4, a0
	addi        a0, s1, 88
	mv          a1, s4
	call        VectorPush

	// *** Basic block 7

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 8

	mv          a0, s1
	call        UpdateState

	// *** Basic block 9

	j           .If_label_33
.func_end_If:
	.size If, .func_end_If-If

	.local  Endif
	.type Endif, @function

Endif:

	// *** Basic block 0

	.global PreprocessorError
	.global VectorPop
	.local SkipSpacesAndComments
	.global PreprocessorWarning
	.local UpdateState
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
	mv          s2, a2
	mv          s3, a1
	addi        t0, s1, 88
	ld          t0, 8(t0)
	bnez        t0, .Endif_label_40

	// *** Basic block 1

	lla         a1, .str.165
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           PreprocessorError

	// *** Basic block 2

.Endif_label_37:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.Endif_label_40:
	addi        a0, s1, 88
	call        VectorPop

	// *** Basic block 4

	mv          a3, x0
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        SkipSpacesAndComments

	// *** Basic block 5

	mv          s2, a0
	ld          t0, 24(s3)
	bge         s2, t0, .Endif_label_68

	// *** Basic block 6

	lla         a1, .str.166
	lla         a2, .str.167
	mv          a0, s1
	call        PreprocessorWarning

	// *** Basic block 7

.Endif_label_68:
	mv          a0, s1
	call        UpdateState

	// *** Basic block 8

	j           .Endif_label_37
.func_end_Endif:
	.size Endif, .func_end_Endif-Endif

	.local  Ifndef
	.type Ifndef, @function

Ifndef:

	// *** Basic block 0

	.local ReadIdentifier
	.global PreprocessorError
	.global HashTableSearch
	.global VectorPush
	.local macro_not_defined
	.global StringDestruct
	.local SkipSpacesAndComments
	.global PreprocessorWarning
	.local UpdateState
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a2
	mv          s2, a1
	mv          s3, a0
	addi        a2, s0, -64
	mv          a1, s1
	mv          a0, s2
	call        ReadIdentifier

	// *** Basic block 1

	mv          s1, a0
	addi        t0, s0, -64
	ld          t0, 24(t0)
	bnez        t0, .Ifndef_label_50

	// *** Basic block 2

	lla         a1, .str.168
	mv          a0, s3
	call        PreprocessorError

	// *** Basic block 3

.Ifndef_label_50:
	addi        t0, s0, -64
	ld          a1, 16(t0)
	mv          a0, s3
	call        HashTableSearch

	// *** Basic block 4

	mv          s4, a0
	addi        s5, s3, 88
	bne         s4, x0, .Ifndef_label_67

	// *** Basic block 5

	lla         s6, macro_not_defined
	j           .Ifndef_label_69

	// *** Basic block 6

.Ifndef_label_67:
	mv          s6, x0

	// *** Basic block 7

.Ifndef_label_69:
	mv          a1, s6
	mv          a0, s5
	call        VectorPush

	// *** Basic block 8

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 9

	mv          a3, x0
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        SkipSpacesAndComments

	// *** Basic block 10

	mv          s1, a0
	ld          t0, 24(s2)
	bge         s1, t0, .Ifndef_label_102

	// *** Basic block 11

	lla         a1, .str.169
	lla         a2, .str.170
	mv          a0, s3
	call        PreprocessorWarning

	// *** Basic block 12

.Ifndef_label_102:
	mv          a0, s3
	call        UpdateState

	// *** Basic block 13

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
.func_end_Ifndef:
	.size Ifndef, .func_end_Ifndef-Ifndef

	.local  Ifdef
	.type Ifdef, @function

Ifdef:

	// *** Basic block 0

	.local ReadIdentifier
	.global PreprocessorError
	.global HashTableSearch
	.global VectorPush
	.global StringDestruct
	.local SkipSpacesAndComments
	.global PreprocessorWarning
	.local UpdateState
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a2
	mv          s2, a1
	mv          s3, a0
	addi        a2, s0, -64
	mv          a1, s1
	mv          a0, s2
	call        ReadIdentifier

	// *** Basic block 1

	mv          s1, a0
	addi        t0, s0, -64
	ld          t0, 24(t0)
	bnez        t0, .Ifdef_label_49

	// *** Basic block 2

	lla         a1, .str.171
	mv          a0, s3
	call        PreprocessorError

	// *** Basic block 3

.Ifdef_label_49:
	addi        t0, s0, -64
	ld          a1, 16(t0)
	mv          a0, s3
	call        HashTableSearch

	// *** Basic block 4

	mv          s4, a0
	addi        a0, s3, 88
	mv          a1, s4
	call        VectorPush

	// *** Basic block 5

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 6

	mv          a3, x0
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        SkipSpacesAndComments

	// *** Basic block 7

	mv          s1, a0
	ld          t0, 24(s2)
	bge         s1, t0, .Ifdef_label_91

	// *** Basic block 8

	lla         a1, .str.172
	lla         a2, .str.173
	mv          a0, s3
	call        PreprocessorWarning

	// *** Basic block 9

.Ifdef_label_91:
	mv          a0, s3
	call        UpdateState

	// *** Basic block 10

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Ifdef:
	.size Ifdef, .func_end_Ifdef-Ifdef

	.local  Elif
	.type Elif, @function

Elif:

	// *** Basic block 0

	.global PreprocessorError
	.local UpdateState
	.global StringInit
	.global StringAppend
	.local EvaluateExpression
	.global StringDestruct
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -64(s0)
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
	mv          s3, a2
	addi        t0, s1, 88
	ld          s4, 8(t0)
	bnez        s4, .Elif_label_42

	// *** Basic block 1

	lla         a1, .str.174
	mv          a0, s1
	call        PreprocessorError

	// *** Basic block 2

.Elif_label_39:
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

	// *** Basic block 3

.Elif_label_42:
	ld          t0, 88(s1)
	addi        t1, s4, -1
	slli        s4, t1, 3
	add         t0, t0, s4
	ld          s5, 0(t0)
	bne         s5, x0, .Elif_label_55

	// *** Basic block 4

	j           .Elif_label_70

	// *** Basic block 5

.Elif_label_55:
	li          s7, -1		// 0xffffffffffffffff
	bne         s5, s7, .Elif_label_61

	// *** Basic block 6

	j           .Elif_label_39

	// *** Basic block 7

.Elif_label_61:
	ld          t0, 88(s1)
	add         t0, t0, s4
	sd          s7, 0(t0)
	mv          a0, s1
	call        UpdateState

	// *** Basic block 8

	j           .Elif_label_39

	// *** Basic block 9

.Elif_label_70:
	addi        a0, s0, -64
	ld          t0, 16(s2)
	add         a1, t0, s3
	call        StringInit

	// *** Basic block 10

	addi        a0, s0, -64
	lla         a1, .str.175
	call        StringAppend

	// *** Basic block 11

	addi        a1, s0, -64
	mv          a0, s1
	call        EvaluateExpression

	// *** Basic block 12

	mv          s6, a0
	ld          t0, 88(s1)
	add         t0, t0, s4
	sd          s6, 0(t0)
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 13

	mv          a0, s1
	call        UpdateState

	// *** Basic block 14

	j           .Elif_label_39
.func_end_Elif:
	.size Elif, .func_end_Elif-Elif

	.local  Else
	.type Else, @function

Else:

	// *** Basic block 0

	.global PreprocessorError
	.local inverted_value
	.local SkipSpacesAndComments
	.global PreprocessorWarning
	.local UpdateState
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
	mv          s3, a1
	addi        t0, s1, 88
	ld          t0, 8(t0)
	bnez        t0, .Else_label_41

	// *** Basic block 1

	lla         a1, .str.176
	mv          a0, s1
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           PreprocessorError

	// *** Basic block 2

.Else_label_38:
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

	// *** Basic block 3

.Else_label_41:
	ld          t1, 88(s1)
	addi        t0, t0, -1
	slli        t0, t0, 3
	add         t1, t1, t0
	ld          t2, 0(t1)
	ld          t1, 88(s1)
	add         s4, t1, t0
	bne         t2, x0, .Else_label_59

	// *** Basic block 4

	lla         s5, inverted_value
	j           .Else_label_61

	// *** Basic block 5

.Else_label_59:
	mv          s5, x0

	// *** Basic block 6

.Else_label_61:
	sd          s5, 0(s4)
	mv          a3, x0
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        SkipSpacesAndComments

	// *** Basic block 7

	mv          s2, a0
	ld          t0, 24(s3)
	bge         s2, t0, .Else_label_87

	// *** Basic block 8

	lla         a1, .str.177
	lla         a2, .str.178
	mv          a0, s1
	call        PreprocessorWarning

	// *** Basic block 9

.Else_label_87:
	mv          a0, s1
	call        UpdateState

	// *** Basic block 10

	j           .Else_label_38
.func_end_Else:
	.size Else, .func_end_Else-Else

	.local  Line
	.type Line, @function

Line:

	// *** Basic block 0

	.local Tokenize
	.local ReplaceMacrosInTokenizedLine
	.local TokenIteratorInit
	.local CurrentToken
	.local GetCurrentTokenSpelling
	.global isdigit
	.global PreprocessorError
	.local MoveToNextToken
	.global StringDestruct
	.global StringSetString
	addi sp, sp, -224
	// Saved return address (offset 216) and frame pointer (offset 208)
	sd ra, 216(sp)
	sd s0, 208(sp)
	addi s0, sp, 224
	// Local vars at offset -176(s0)
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
	lb          t0, 168(s1)
	not         t0, t0
	beqz        t0, .Line_label_48

	// *** Basic block 1

.Line_label_45:
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

	// *** Basic block 2

.Line_label_48:
	addi        a2, s0, -176
	mv          a6, x0
	mv          a5, x0
	li          s4, 1		// 0x1 ASCII \x1
	mv          a4, s4
	mv          a3, s3
	mv          a1, s2
	mv          a0, s1
	call        Tokenize

	// *** Basic block 3

	addi        a1, s0, -176
	mv          a2, s4
	mv          a0, s1
	call        ReplaceMacrosInTokenizedLine

	// *** Basic block 4

	addi        a0, s0, -136
	addi        a2, s0, -176
	mv          a1, s1
	call        TokenIteratorInit

	// *** Basic block 5

	li          s4, -1		// 0xffffffffffffffff
	mv          s5, x0
	addi        a0, s0, -136
	call        CurrentToken

	// *** Basic block 6

	li          t0, 4		// 0x4 ASCII \x4
	bne         a0, t0, .Line_label_153

	// *** Basic block 7

	addi        a0, s0, -136
	addi        a1, s0, -96
	call        GetCurrentTokenSpelling

	// *** Basic block 8

	addi        t0, s0, -96
	ld          s2, 16(t0)
	mv          s4, x0
	mv          s3, x0
	addi        t0, s0, -96
	ld          s6, 24(t0)
	bge         x0, s6, .Line_label_146

	// *** Basic block 9

.Line_label_116:
	add         t0, s2, s3
	lb          s2, 0(t0)
	mv          a0, s2
	call        isdigit

	// *** Basic block 10

	slli        t0, a0, 56
	srai        t0, t0, 56
	not         t0, t0
	beqz        t0, .Line_label_134

	// *** Basic block 11

	lla         a1, .str.179
	mv          a0, s1
	call        PreprocessorError

	// *** Basic block 12

	li          s5, 1		// 0x1 ASCII \x1
	j           .Line_label_146

	// *** Basic block 13

.Line_label_134:
	slli        t0, s4, 1
	slli        t1, s4, 3
	add         t0, t0, t1
	add         t0, t0, s2
	addi        s4, t0, -48

	// *** Basic block 14

.Line_label_142:
	addi        s3, s3, 1
	bge         s3, s6, .Line_label_116

	// *** Basic block 15

.Line_label_146:
	addi        a0, s0, -136
	call        MoveToNextToken

	// *** Basic block 16

	addi        a0, s0, -96
	call        StringDestruct

	// *** Basic block 17

.Line_label_153:
	not         t0, s5
	beqz        t0, .Line_label_158

	// *** Basic block 18

	sltz        t0, s4

	// *** Basic block 19

.Line_label_158:
	beqz        t0, .Line_label_167

	// *** Basic block 20

	lla         a1, .str.180
	mv          a0, s1
	call        PreprocessorError

	// *** Basic block 21

	li          s5, 1		// 0x1 ASCII \x1

	// *** Basic block 22

.Line_label_167:
	mv          s2, x0
	not         s6, s5
	beqz        s6, .Line_label_179

	// *** Basic block 23

	addi        a0, s0, -136
	call        CurrentToken

	// *** Basic block 24

	addi        t0, a0, -6
	seqz        s6, t0

	// *** Basic block 25

.Line_label_179:
	beqz        s6, .Line_label_189

	// *** Basic block 26

	addi        a0, s0, -136
	addi        a1, s0, -56
	call        GetCurrentTokenSpelling

	// *** Basic block 27

	li          s2, 1		// 0x1 ASCII \x1
	j           .Line_label_197

	// *** Basic block 28

.Line_label_189:
	lla         a1, .str.181
	mv          a0, s1
	call        PreprocessorError

	// *** Basic block 29

	li          s5, 1		// 0x1 ASCII \x1

	// *** Basic block 30

.Line_label_197:
	not         t0, s5
	beqz        t0, .Line_label_229

	// *** Basic block 31

	ld          t0, 160(s1)
	ld          t0, 0(t0)
	sw          s4, 80(t0)
	beqz        s2, .Line_label_217

	// *** Basic block 32

	ld          t0, 160(s1)
	ld          a0, 0(t0)
	addi        a1, s0, -56
	call        StringSetString

	// *** Basic block 33

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 34

	j           .Line_label_228

	// *** Basic block 35

.Line_label_217:
	ld          t0, 160(s1)
	ld          a0, 0(t0)
	ld          t0, 160(s1)
	ld          t0, 0(t0)
	addi        a1, t0, 40
	call        StringSetString

	// *** Basic block 36

.Line_label_228:

	// *** Basic block 37

.Line_label_229:
	addi        a0, s0, -176
	call        StringDestruct

	// *** Basic block 38

	j           .Line_label_45
.func_end_Line:
	.size Line, .func_end_Line-Line

	.local  Error
	.type Error, @function

Error:

	// *** Basic block 0

	.global StringAppend
	.global PreprocessorError
	.global StringDestruct
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	lb          t0, 168(s1)
	not         t0, t0
	beqz        t0, .Error_label_26

	// *** Basic block 1

.Error_label_23:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.Error_label_26:
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	addi        a0, s0, -64
	ld          t0, 16(s2)
	add         a1, t0, s3
	call        StringAppend

	// *** Basic block 3

	addi        t0, s0, -64
	ld          a1, 16(t0)
	mv          a0, s1
	call        PreprocessorError

	// *** Basic block 4

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 5

	j           .Error_label_23
.func_end_Error:
	.size Error, .func_end_Error-Error

	.local  Warning
	.type Warning, @function

Warning:

	// *** Basic block 0

	.global StringAppend
	.global PreprocessorWarning
	.global StringDestruct
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	lb          t0, 168(s1)
	not         t0, t0
	beqz        t0, .Warning_label_27

	// *** Basic block 1

.Warning_label_24:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.Warning_label_27:
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	addi        a0, s0, -64
	ld          t0, 16(s2)
	add         a1, t0, s3
	call        StringAppend

	// *** Basic block 3

	lla         a1, .str.182
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s1
	call        PreprocessorWarning

	// *** Basic block 4

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 5

	j           .Warning_label_24
.func_end_Warning:
	.size Warning, .func_end_Warning-Warning

	.local  MatchIdentifierToken
	.type MatchIdentifierToken, @function

MatchIdentifierToken:

	// *** Basic block 0

	.local SkipSpaceTokens
	.local CurrentToken
	.local GetCurrentTokenSpelling
	.global StringEqual
	.global StringDestruct
	.local MoveToNextToken
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	call        SkipSpaceTokens

	// *** Basic block 1

	mv          a0, s1
	call        CurrentToken

	// *** Basic block 2

	li          t0, 1		// 0x1 ASCII \x1
	bne         a0, t0, .MatchIdentifierToken_label_54

	// *** Basic block 3

	addi        a1, s0, -64
	mv          a0, s1
	call        GetCurrentTokenSpelling

	// *** Basic block 4

	addi        a0, s0, -64
	mv          a1, s2
	call        StringEqual

	// *** Basic block 5

	mv          s3, a0
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 6

	beqz        s3, .MatchIdentifierToken_label_48

	// *** Basic block 7

	mv          a0, s1
	call        MoveToNextToken

	// *** Basic block 8

.MatchIdentifierToken_label_48:
	mv          a0, s3

	// *** Basic block 9

.MatchIdentifierToken_label_51:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 10

.MatchIdentifierToken_label_54:
	mv          a0, x0
	j           .MatchIdentifierToken_label_51
.func_end_MatchIdentifierToken:
	.size MatchIdentifierToken, .func_end_MatchIdentifierToken-MatchIdentifierToken

	.local  GetIdentifierToken
	.type GetIdentifierToken, @function

GetIdentifierToken:

	// *** Basic block 0

	.local SkipSpaceTokens
	.local CurrentToken
	.local GetCurrentTokenSpelling
	.local MoveToNextToken
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
	call        SkipSpaceTokens

	// *** Basic block 1

	mv          a0, s1
	call        CurrentToken

	// *** Basic block 2

	li          t0, 1		// 0x1 ASCII \x1
	bne         a0, t0, .GetIdentifierToken_label_38

	// *** Basic block 3

	mv          a1, s2
	mv          a0, s1
	call        GetCurrentTokenSpelling

	// *** Basic block 4

	mv          a0, s1
	call        MoveToNextToken

	// *** Basic block 5

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 6

.GetIdentifierToken_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.GetIdentifierToken_label_38:
	mv          a0, x0
	j           .GetIdentifierToken_label_35
.func_end_GetIdentifierToken:
	.size GetIdentifierToken, .func_end_GetIdentifierToken-GetIdentifierToken

	.local  Pragma
	.type Pragma, @function

Pragma:

	// *** Basic block 0

	.local Tokenize
	.local TokenIteratorInit
	.local CurrentToken
	.local MatchIdentifierToken
	.local GetIdentifierToken
	.global EnableWarning
	.global DisableWarning
	.global StringDestruct
	addi sp, sp, -176
	// Saved return address (offset 168) and frame pointer (offset 160)
	sd ra, 168(sp)
	sd s0, 160(sp)
	addi s0, sp, 176
	// Local vars at offset -144(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	lb          t0, 168(s1)
	not         t0, t0
	beqz        t0, .Pragma_label_34

	// *** Basic block 1

.Pragma_label_31:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.Pragma_label_34:
	addi        a2, s0, -144
	mv          a6, x0
	mv          a5, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a4, t0
	mv          a3, s3
	mv          a1, s2
	mv          a0, s1
	call        Tokenize

	// *** Basic block 3

	addi        a0, s0, -104
	addi        a2, s0, -144
	mv          a1, s1
	call        TokenIteratorInit

	// *** Basic block 4

	addi        a0, s0, -104
	call        CurrentToken

	// *** Basic block 5

	li          s4, 19		// 0x13 ASCII \x13
	beq         a0, s4, .Pragma_label_125

	// *** Basic block 6

.Pragma_label_73:
	addi        a0, s0, -104
	lla         a1, .str.183
	call        MatchIdentifierToken

	// *** Basic block 7

	beqz        a0, .Pragma_label_123

	// *** Basic block 8

	addi        a0, s0, -104
	addi        a1, s0, -64
	call        GetIdentifierToken

	// *** Basic block 9

	beqz        a0, .Pragma_label_110

	// *** Basic block 10

	addi        a0, s0, -104
	lla         a1, .str.184
	call        MatchIdentifierToken

	// *** Basic block 11

	mv          s1, a0
	beqz        s1, .Pragma_label_103

	// *** Basic block 12

	addi        t0, s0, -64
	ld          a0, 16(t0)
	call        EnableWarning

	// *** Basic block 13

	j           .Pragma_label_109

	// *** Basic block 14

.Pragma_label_103:
	addi        t0, s0, -64
	ld          a0, 16(t0)
	call        DisableWarning

	// *** Basic block 15

.Pragma_label_109:

	// *** Basic block 16

.Pragma_label_110:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 17

.Pragma_label_114:
	addi        a0, s0, -104
	call        CurrentToken

	// *** Basic block 18

	bne         a0, s4, .Pragma_label_73

	// *** Basic block 19

	j           .Pragma_label_125

	// *** Basic block 20

.Pragma_label_123:
	j           .Pragma_label_125

	// *** Basic block 21

.Pragma_label_125:
	addi        a0, s0, -144
	call        StringDestruct

	// *** Basic block 22

	j           .Pragma_label_31
.func_end_Pragma:
	.size Pragma, .func_end_Pragma-Pragma

	.global PreprocessorParseDirective
	.type PreprocessorParseDirective, @function

PreprocessorParseDirective:

	// *** Basic block 0

	.local SkipToEndOfComment
	.local SkipSpacesAndComments
	.global isalpha
	.global isspace
	.global StringAppendChar
	.local preprocessor_commands
	.global StringEqual
	.global PreprocessorError
	.global StringDestruct
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -64(s0)
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
	ld          t0, 160(s1)
	lb          t0, 177(t0)
	beqz        t0, .PreprocessorParseDirective_label_45

	// *** Basic block 1

	mv          a1, x0
	mv          a0, s2
	call        SkipToEndOfComment

	// *** Basic block 2

	mv          s3, a0

	// *** Basic block 3

.PreprocessorParseDirective_label_45:
	mv          a3, x0
	mv          a2, s2
	mv          a1, s3
	mv          a0, s1
	call        SkipSpacesAndComments

	// *** Basic block 4

	ld          s4, 16(s2)
	mv          s3, a0
	ld          s5, 24(s2)
	slt         t1, s3, s5
	not         t0, t1
	bge         s3, s5, .PreprocessorParseDirective_label_73

	// *** Basic block 5

	ld          t1, 16(s2)
	add         t1, t1, s3
	lb          t1, 0(t1)
	addi        t1, t1, -35
	snez        t0, t1

	// *** Basic block 6

.PreprocessorParseDirective_label_73:
	beqz        t0, .PreprocessorParseDirective_label_80

	// *** Basic block 7

	mv          a0, x0

	// *** Basic block 8

.PreprocessorParseDirective_label_77:
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

	// *** Basic block 9

.PreprocessorParseDirective_label_80:
	addi        s3, s3, 1
	mv          a3, x0
	mv          a2, s2
	mv          a1, s3
	mv          a0, s1
	call        SkipSpacesAndComments

	// *** Basic block 10

	mv          s3, a0
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	slt         s6, s3, s5
	bge         s3, s5, .PreprocessorParseDirective_label_114

	// *** Basic block 11

	ld          t0, 16(s2)
	add         t0, t0, s3
	lb          a0, 0(t0)
	call        isalpha

	// *** Basic block 12

	mv          s6, a0

	// *** Basic block 13

.PreprocessorParseDirective_label_114:
	beqz        s6, .PreprocessorParseDirective_label_151

	// *** Basic block 14

	mv          s7, s6
	bge         s3, s5, .PreprocessorParseDirective_label_128

	// *** Basic block 15

	ld          t0, 16(s2)
	add         t0, t0, s3
	lb          a0, 0(t0)
	call        isspace

	// *** Basic block 16

	slli        t0, a0, 56
	srai        t0, t0, 56
	not         s7, t0

	// *** Basic block 17

.PreprocessorParseDirective_label_128:
	beqz        s7, .PreprocessorParseDirective_label_150

	// *** Basic block 18

.PreprocessorParseDirective_label_130:
	addi        a0, s0, -64
	add         t0, s4, s3
	lb          a1, 0(t0)
	call        StringAppendChar

	// *** Basic block 19

	addi        s3, s3, 1
	slt         s7, s3, s5
	bge         s3, s5, .PreprocessorParseDirective_label_148

	// *** Basic block 20

	add         t0, s4, s3
	lb          a0, 0(t0)
	call        isspace

	// *** Basic block 21

	slli        t0, a0, 56
	srai        t0, t0, 56
	not         s7, t0

	// *** Basic block 22

.PreprocessorParseDirective_label_148:
	bnez        s7, .PreprocessorParseDirective_label_130

	// *** Basic block 23

.PreprocessorParseDirective_label_150:

	// *** Basic block 24

.PreprocessorParseDirective_label_151:
	mv          a3, x0
	mv          a2, s2
	mv          a1, s3
	mv          a0, s1
	call        SkipSpacesAndComments

	// *** Basic block 25

	mv          s3, a0
	mv          s4, x0
	mv          s5, x0
	la          t0, preprocessor_commands
	ld          t0, 0(t0)
	beq         t0, x0, .PreprocessorParseDirective_label_196

	// *** Basic block 26

.PreprocessorParseDirective_label_172:
	addi        a0, s0, -64
	slli        t0, s5, 4
	la          t1, preprocessor_commands
	add         s7, t1, t0
	ld          a1, 0(s7)
	call        StringEqual

	// *** Basic block 27

	beqz        a0, .PreprocessorParseDirective_label_186

	// *** Basic block 28

	ld          s4, 8(s7)
	j           .PreprocessorParseDirective_label_196

	// *** Basic block 29

.PreprocessorParseDirective_label_186:

	// *** Basic block 30

.PreprocessorParseDirective_label_187:
	addi        s5, s5, 1
	slli        t0, s5, 4
	la          t1, preprocessor_commands
	add         t0, t1, t0
	ld          t0, 0(t0)
	beq         t0, x0, .PreprocessorParseDirective_label_172

	// *** Basic block 31

.PreprocessorParseDirective_label_196:
	bne         s4, x0, .PreprocessorParseDirective_label_220

	// *** Basic block 32

	lb          t0, 168(s1)
	beqz        t0, .PreprocessorParseDirective_label_213

	// *** Basic block 33

	lla         a1, .str.199
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s1
	call        PreprocessorError

	// *** Basic block 34

.PreprocessorParseDirective_label_213:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 35

	li          a0, 1		// 0x1 ASCII \x1
	j           .PreprocessorParseDirective_label_77

	// *** Basic block 36

.PreprocessorParseDirective_label_220:
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	jalr         x1, s4, 0

	// *** Basic block 37

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 38

	li          a0, 1		// 0x1 ASCII \x1
	j           .PreprocessorParseDirective_label_77
.func_end_PreprocessorParseDirective:
	.size PreprocessorParseDirective, .func_end_PreprocessorParseDirective-PreprocessorParseDirective

	.global PreprocessorHasInclude
	.type PreprocessorHasInclude, @function

PreprocessorHasInclude:

	// *** Basic block 0

	.local FindFileInPath
	.global fclose
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
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, x0
	sd          x0, -32(s0)
	not         t0, a2
	beqz        t0, .PreprocessorHasInclude_label_34

	// *** Basic block 1

	addi        a0, s1, 112
	addi        a2, s0, -32
	mv          a1, s2
	call        FindFileInPath

	// *** Basic block 2

	mv          s3, a0

	// *** Basic block 3

.PreprocessorHasInclude_label_34:
	bne         s3, x0, .PreprocessorHasInclude_label_46

	// *** Basic block 4

	addi        a0, s1, 136
	addi        a2, s0, -32
	mv          a1, s2
	call        FindFileInPath

	// *** Basic block 5

	mv          s3, a0

	// *** Basic block 6

.PreprocessorHasInclude_label_46:
	bne         s3, x0, .PreprocessorHasInclude_label_55

	// *** Basic block 7

	mv          a0, x0

	// *** Basic block 8

.PreprocessorHasInclude_label_52:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 9

.PreprocessorHasInclude_label_55:
	mv          a0, s3
	call        fclose

	// *** Basic block 10

	li          a0, 1		// 0x1 ASCII \x1
	j           .PreprocessorHasInclude_label_52
.func_end_PreprocessorHasInclude:
	.size PreprocessorHasInclude, .func_end_PreprocessorHasInclude-PreprocessorHasInclude

	.global PreprocessorHasIncludeNext
	.type PreprocessorHasIncludeNext, @function

PreprocessorHasIncludeNext:

	// *** Basic block 0

	.global compiler
	.local FindFileInPath
	.global fclose
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
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, x0
	la          t0, compiler
	ld          t0, 0(t0)
	ld          t0, 1088(t0)
	addi        t0, t0, 1
	sd          t0, -32(s0)
	not         t0, a2
	beqz        t0, .PreprocessorHasIncludeNext_label_41

	// *** Basic block 1

	addi        a0, s1, 112
	addi        a2, s0, -32
	mv          a1, s2
	call        FindFileInPath

	// *** Basic block 2

	mv          s3, a0

	// *** Basic block 3

.PreprocessorHasIncludeNext_label_41:
	bne         s3, x0, .PreprocessorHasIncludeNext_label_53

	// *** Basic block 4

	addi        a0, s1, 136
	addi        a2, s0, -32
	mv          a1, s2
	call        FindFileInPath

	// *** Basic block 5

	mv          s3, a0

	// *** Basic block 6

.PreprocessorHasIncludeNext_label_53:
	bne         s3, x0, .PreprocessorHasIncludeNext_label_62

	// *** Basic block 7

	mv          a0, x0

	// *** Basic block 8

.PreprocessorHasIncludeNext_label_59:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 9

.PreprocessorHasIncludeNext_label_62:
	mv          a0, s3
	call        fclose

	// *** Basic block 10

	li          a0, 1		// 0x1 ASCII \x1
	j           .PreprocessorHasIncludeNext_label_59
.func_end_PreprocessorHasIncludeNext:
	.size PreprocessorHasIncludeNext, .func_end_PreprocessorHasIncludeNext-PreprocessorHasIncludeNext

	.global PreprocessorFindMacro
	.type PreprocessorFindMacro, @function

PreprocessorFindMacro:

	// *** Basic block 0

	.global HashTableSearch
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	ld          a1, 16(t0)
	j           HashTableSearch
.func_end_PreprocessorFindMacro:
	.size PreprocessorFindMacro, .func_end_PreprocessorFindMacro-PreprocessorFindMacro

	.global PreprocessorLineIsCompiledIn
	.type PreprocessorLineIsCompiledIn, @function

PreprocessorLineIsCompiledIn:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lb          a0, 168(a0)

	// *** Basic block 1

.PreprocessorLineIsCompiledIn_label_10:
	ret         
.func_end_PreprocessorLineIsCompiledIn:
	.size PreprocessorLineIsCompiledIn, .func_end_PreprocessorLineIsCompiledIn-PreprocessorLineIsCompiledIn

	.local  FindMacroArg
	.type FindMacroArg, @function

FindMacroArg:

	// *** Basic block 0

	.global MapFind
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -32(s0)
	// End of stack frame
	mv          t0, a1
	sd          t0, -32(s0)
	ld          a1, -32(s0)
	call        MapFind

	// *** Basic block 1


	// *** Basic block 2

.FindMacroArg_label_21:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_FindMacroArg:
	.size FindMacroArg, .func_end_FindMacroArg-FindMacroArg

	.local  CollectActualArguments
	.type CollectActualArguments, @function

CollectActualArguments:

	// *** Basic block 0

	.local CurrentToken
	.global NewString
	.global SourceReadLine
	.local Tokenize
	.local FindNextTokenIndex
	.global StringDestruct
	.local MoveToNextToken
	.local SkipSpaceTokens
	.local AppendCurrentToken
	.global StringAppendChar
	.global StringAppend
	.global StringDelete
	.global MapInsert
	.global PreprocessorError
	sd          a1, -8(s0)	// Spilled @38
	sd          a0, -0(s0)	// Spilled @44
	addi sp, sp, -256
	// Saved return address (offset 248) and frame pointer (offset 240)
	sd ra, 248(sp)
	sd s0, 240(sp)
	addi s0, sp, 256
	// Local vars at offset -96(s0)
	// Spilled register region: 72 bytes at -168(s0) to -96(s0)
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
	sd          s1, -120(s0)	// Spilled @37
	mv          s2, a6
	mv          s3, a0
	mv          s4, a2
	mv          s5, a3
	sd          s5, -144(s0)	// Spilled @49
	mv          s6, a5
	mv          s7, a4
	mv          s8, x0
	mv          s9, x0
	mv          s10, x0
	sd          s10, -136(s0)	// Spilled @63
	mv          s11, x0
	mv          a0, x0
	sd          a1, -128(s0)	// Spilled @69
	li          a1, 1		// 0x1 ASCII \x1
	ld          t0, 160(s3)
	ld          s1, 0(t0)
	lb          a1, 178(t0)
	addi        t0, s5, 80
	ld          s10, 8(t0)
	sd          s10, -152(s0)	// Spilled @79
	lb          s5, 146(s5)
	sd          s5, -160(s0)	// Spilled @81
	ld          t0, -144(s0)	// Spilled @49
	ld          s10, 80(t0)
	sd          s10, -168(s0)	// Spilled @82
	ld          a0, -120(s0)	// Spilled @37
	ld          t1, -104(s0)	// Spilled @44
	call        CurrentToken

	// *** Basic block 1

	addi        t0, a0, -14
	snez        a1, t0

	// *** Basic block 2

.CollectActualArguments_label_89:
	ld          t0, -128(s0)	// Spilled @69
	beqz        t0, .CollectActualArguments_label_277

	// *** Basic block 3

.CollectActualArguments_label_91:
	mv          a0, x0
	ld          t0, -104(s0)	// Spilled @44
	call        NewString

	// *** Basic block 4

	mv          s5, a0
	li          s10, 1		// 0x1 ASCII \x1
	not         t0, a0
	beqz        t0, .CollectActualArguments_label_213

	// *** Basic block 5

.CollectActualArguments_label_101:
	ld          a0, -120(s0)	// Spilled @37
	ld          t0, -104(s0)	// Spilled @44
	call        CurrentToken

	// *** Basic block 6

	mv          s10, a0
	sd          s10, -168(s0)	// Spilled @105
	li          s10, 19		// 0x13 ASCII \x13
	ld          t0, -168(s0)	// Spilled @105
	bne         t0, s10, .CollectActualArguments_label_166

	// *** Basic block 7

	beqz        s2, .CollectActualArguments_label_115

	// *** Basic block 8

	li          a0, 1		// 0x1 ASCII \x1
	j           .CollectActualArguments_label_213

	// *** Basic block 9

.CollectActualArguments_label_115:
	sd          x0, -96(s0)
	sd          x0, -88(s0)
	sd          x0, -80(s0)
	sd          x0, -72(s0)
	sd          x0, -64(s0)
	sb          x0, -96(s0)
	addi        a1, s0, -96
	mv          a0, s1
	ld          t0, -104(s0)	// Spilled @44
	ld          t1, -112(s0)	// Spilled @38
	call        SourceReadLine

	// *** Basic block 10

	addi        a1, s0, -96
	mv          a6, x0
	mv          a5, a1
	mv          a4, x0
	mv          a3, x0
	mv          a2, s4
	mv          a0, s3
	ld          t0, -104(s0)	// Spilled @44
	ld          t1, -112(s0)	// Spilled @38
	call        Tokenize

	// *** Basic block 11

	ld          a0, -120(s0)	// Spilled @37
	ld          t0, -104(s0)	// Spilled @44
	call        FindNextTokenIndex

	// *** Basic block 12

	ld          t0, -120(s0)	// Spilled @37
	sd          a0, 32(t0)
	addi        a0, s0, -96
	ld          t1, -104(s0)	// Spilled @44
	call        StringDestruct

	// *** Basic block 13

	ld          a0, -120(s0)	// Spilled @37
	ld          t0, -104(s0)	// Spilled @44
	call        CurrentToken

	// *** Basic block 14

	bne         a0, s10, .CollectActualArguments_label_164

	// *** Basic block 15

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 16

.CollectActualArguments_label_164:
	j           .CollectActualArguments_label_101

	// *** Basic block 17

.CollectActualArguments_label_166:
	li          t0, 13		// 0xd ASCII \xd
	ld          t1, -168(s0)	// Spilled @105
	bne         t1, t0, .CollectActualArguments_label_174

	// *** Basic block 18

	ld          t0, -160(s0)	// Spilled @97
	addi        s10, t0, 1
	sd          s10, -160(s0)	// Spilled @97
	j           .CollectActualArguments_label_185

	// *** Basic block 19

.CollectActualArguments_label_174:
	li          t0, 14		// 0xe ASCII \xe
	ld          t1, -168(s0)	// Spilled @105
	bne         t1, t0, .CollectActualArguments_label_184

	// *** Basic block 20

	ld          t0, -160(s0)	// Spilled @97
	addi        s10, t0, -1
	ld          t1, -160(s0)	// Spilled @97
	beqz        t1, .CollectActualArguments_label_213

	// *** Basic block 21

.CollectActualArguments_label_183:

	// *** Basic block 22

.CollectActualArguments_label_184:

	// *** Basic block 23

.CollectActualArguments_label_185:
	li          t0, 15		// 0xf ASCII \xf
	ld          t1, -168(s0)	// Spilled @105
	bne         t1, t0, .CollectActualArguments_label_202

	// *** Basic block 24

	ld          a0, -120(s0)	// Spilled @37
	ld          t0, -104(s0)	// Spilled @44
	call        MoveToNextToken

	// *** Basic block 25

	ld          a0, -120(s0)	// Spilled @37
	ld          t0, -104(s0)	// Spilled @44
	call        SkipSpaceTokens

	// *** Basic block 26

	li          t0, 1		// 0x1 ASCII \x1
	ld          t1, -160(s0)	// Spilled @97
	beq         t1, t0, .CollectActualArguments_label_213

	// *** Basic block 27

.CollectActualArguments_label_201:

	// *** Basic block 28

.CollectActualArguments_label_202:
	mv          a1, s5
	ld          a0, -120(s0)	// Spilled @37
	ld          t0, -104(s0)	// Spilled @44
	ld          t1, -112(s0)	// Spilled @38
	call        AppendCurrentToken

	// *** Basic block 29

	ld          a0, -120(s0)	// Spilled @37
	ld          t0, -104(s0)	// Spilled @44
	call        MoveToNextToken

	// *** Basic block 30

	not         t0, a0
	bnez        t0, .CollectActualArguments_label_101

	// *** Basic block 31

.CollectActualArguments_label_213:
	ld          t0, -152(s0)	// Spilled @79
	blt         s9, t0, .CollectActualArguments_label_241

	// *** Basic block 32

	ld          t0, -160(s0)	// Spilled @81
	beqz        t0, .CollectActualArguments_label_234

	// *** Basic block 33

	beqz        s11, .CollectActualArguments_label_225

	// *** Basic block 34

	li          t0, 15		// 0xf ASCII \xf
	mv          a1, t0
	mv          a0, s6
	ld          t0, -104(s0)	// Spilled @44
	ld          t1, -112(s0)	// Spilled @38
	call        StringAppendChar

	// *** Basic block 35

.CollectActualArguments_label_225:
	ld          a1, 16(s5)
	mv          a0, s6
	ld          t0, -104(s0)	// Spilled @44
	ld          t1, -112(s0)	// Spilled @38
	call        StringAppend

	// *** Basic block 36

	j           .CollectActualArguments_label_236

	// *** Basic block 37

.CollectActualArguments_label_234:
	li          s10, 1		// 0x1 ASCII \x1

	// *** Basic block 38

.CollectActualArguments_label_236:
	mv          a0, s5
	ld          t0, -104(s0)	// Spilled @44
	call        StringDelete

	// *** Basic block 39

	j           .CollectActualArguments_label_265

	// *** Basic block 40

.CollectActualArguments_label_241:
	slli        t0, s8, 3
	ld          t1, -168(s0)	// Spilled @82
	add         t0, t1, t0
	ld          t0, 0(t0)
	sd          t0, -56(s0)
	addi        t0, s0, -56
	sd          s5, 8(t0)
	addi        sp, sp, -16
	ld          t0, -56(s0)
	sd          t0, 0(sp)
	ld          t0, -48(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s7
	ld          t0, -104(s0)	// Spilled @44
	call        MapInsert

	// *** Basic block 41

	addi        sp, sp, 16
	addi        s8, s8, 1

	// *** Basic block 42

.CollectActualArguments_label_265:
	addi        s9, s9, 1
	not         s1, a0
	beqz        s1, .CollectActualArguments_label_275

	// *** Basic block 43

	ld          a0, -120(s0)	// Spilled @37
	ld          t0, -104(s0)	// Spilled @44
	call        CurrentToken

	// *** Basic block 44

	addi        t0, a0, -14
	snez        s1, t0

	// *** Basic block 45

.CollectActualArguments_label_275:
	bnez        s1, .CollectActualArguments_label_91

	// *** Basic block 46

.CollectActualArguments_label_277:
	ld          t0, -144(s0)	// Spilled @49
	addi        t1, t0, 80
	ld          s1, 8(t1)
	bge         s9, s1, .CollectActualArguments_label_309

	// *** Basic block 47

	ld          t0, -144(s0)	// Spilled @49
	ld          t1, 80(t0)
	slli        t2, s8, 3
	add         t1, t1, t2
	ld          t1, 0(t1)
	sd          t1, -40(s0)
	addi        s10, s0, -40
	lla         a0, .str.200
	ld          t1, -104(s0)	// Spilled @44
	call        NewString

	// *** Basic block 48

	sd          a0, 8(s10)
	addi        sp, sp, -16
	ld          t0, -40(s0)
	sd          t0, 0(sp)
	ld          t0, -32(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s7
	ld          t0, -104(s0)	// Spilled @44
	call        MapInsert

	// *** Basic block 49

	addi        s9, s9, 1
	addi        s8, s8, 1

	// *** Basic block 50

.CollectActualArguments_label_309:
	ld          a0, -120(s0)	// Spilled @37
	ld          t0, -104(s0)	// Spilled @44
	call        CurrentToken

	// *** Basic block 51

	li          t0, 14		// 0xe ASCII \xe
	beq         a0, t0, .CollectActualArguments_label_324

	// *** Basic block 52

	lla         a1, .str.201
	mv          a0, s3
	ld          t0, -104(s0)	// Spilled @44
	ld          t1, -112(s0)	// Spilled @38
	call        PreprocessorError

	// *** Basic block 53

	j           .CollectActualArguments_label_328

	// *** Basic block 54

.CollectActualArguments_label_324:
	ld          a0, -120(s0)	// Spilled @37
	ld          t0, -104(s0)	// Spilled @44
	call        MoveToNextToken

	// *** Basic block 55

.CollectActualArguments_label_328:
	ld          t0, -136(s0)	// Spilled @63
	beqz        t0, .CollectActualArguments_label_344

	// *** Basic block 56

	lla         a1, .str.202
	ld          t0, -144(s0)	// Spilled @49
	addi        t1, t0, 40
	ld          a2, 16(t1)
	mv          a4, s9
	mv          a3, s1
	mv          a0, s3
	ld          t1, -104(s0)	// Spilled @44
	ld          t2, -112(s0)	// Spilled @38
	call        PreprocessorError

	// *** Basic block 57

.CollectActualArguments_label_344:
	bge         s9, s1, .CollectActualArguments_label_361

	// *** Basic block 58

	lla         a1, .str.203
	ld          t0, -144(s0)	// Spilled @49
	addi        t1, t0, 40
	ld          a2, 16(t1)
	mv          a4, s9
	mv          a3, s1
	mv          a0, s3
	ld          t1, -104(s0)	// Spilled @44
	ld          t2, -112(s0)	// Spilled @38
	call        PreprocessorError

	// *** Basic block 59

.CollectActualArguments_label_361:
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
.func_end_CollectActualArguments:
	.size CollectActualArguments, .func_end_CollectActualArguments-CollectActualArguments

	.local  Paste
	.type Paste, @function

Paste:

	// *** Basic block 0

	.local PrevToken
	.local NextToken
	.local MoveToNextToken
	.local EraseCurrentToken
	.local SkipSpaceTokens
	.local DetokenizeToken
	.global StringAppendChar
	.local EncodeLength
	.global StringAppendString
	.global StringDestruct
	.global StringReplaceString
	.local FindNextTokenIndex
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -96(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a0
	call        PrevToken

	// *** Basic block 1

	mv          s2, a0
	mv          a0, s1
	call        NextToken

	// *** Basic block 2

	mv          s3, a0
	li          s4, 12		// 0xc ASCII \xc
	bne         s2, s4, .Paste_label_58

	// *** Basic block 3

	bne         s3, s4, .Paste_label_50

	// *** Basic block 4

	mv          a0, s1
	call        MoveToNextToken

	// *** Basic block 5

	mv          a0, s1
	call        EraseCurrentToken

	// *** Basic block 6

	j           .Paste_label_54

	// *** Basic block 7

.Paste_label_50:
	mv          a0, s1
	call        EraseCurrentToken

	// *** Basic block 8

.Paste_label_54:

	// *** Basic block 9

.Paste_label_55:
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

	// *** Basic block 10

.Paste_label_58:
	bne         s3, s4, .Paste_label_70

	// *** Basic block 11

	mv          a0, s1
	call        MoveToNextToken

	// *** Basic block 12

	mv          a0, s1
	call        EraseCurrentToken

	// *** Basic block 13

	j           .Paste_label_55

	// *** Basic block 14

.Paste_label_70:
	ld          s2, 16(s1)
	mv          a0, s1
	call        MoveToNextToken

	// *** Basic block 15

	mv          a0, s1
	call        SkipSpaceTokens

	// *** Basic block 16

	ld          s4, 24(s1)
	ld          s5, 32(s1)
	sd          x0, -96(s0)
	sd          x0, -88(s0)
	sd          x0, -80(s0)
	sd          x0, -72(s0)
	sd          x0, -64(s0)
	sb          x0, -96(s0)
	ld          s6, 8(s1)
	addi        a2, s0, -96
	mv          a1, s2
	mv          a0, s6
	call        DetokenizeToken

	// *** Basic block 17

	addi        a2, s0, -96
	mv          a1, s4
	mv          a0, s6
	call        DetokenizeToken

	// *** Basic block 18

	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -24(s0)
	sb          x0, -56(s0)
	addi        a0, s0, -56
	li          s4, 3		// 0x3 ASCII \x3
	mv          a1, s4
	call        StringAppendChar

	// *** Basic block 19

	addi        a0, s0, -56
	addi        t0, s0, -96
	ld          a1, 24(t0)
	call        EncodeLength

	// *** Basic block 20

	addi        a0, s0, -56
	addi        a1, s0, -96
	call        StringAppendString

	// *** Basic block 21

	addi        a0, s0, -96
	call        StringDestruct

	// *** Basic block 22

	sub         a2, s5, s2
	addi        a3, s0, -56
	mv          a1, s2
	mv          a0, s6
	call        StringReplaceString

	// *** Basic block 23

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 24

	sd          s2, 24(s1)
	mv          a0, s1
	call        FindNextTokenIndex

	// *** Basic block 25

	sd          a0, 32(s1)
	j           .Paste_label_55
.func_end_Paste:
	.size Paste, .func_end_Paste-Paste

	.local  PasteTokens
	.type PasteTokens, @function

PasteTokens:

	// *** Basic block 0

	.local TokenIteratorInit
	.local CurrentToken
	.local Paste
	.local MoveToNextToken
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          t0, a0
	mv          t1, a1
	addi        a0, s0, -64
	mv          a2, t1
	mv          a1, t0
	call        TokenIteratorInit

	// *** Basic block 1

	addi        a0, s0, -64
	call        CurrentToken

	// *** Basic block 2

	li          s1, 19		// 0x13 ASCII \x13
	beq         a0, s1, .PasteTokens_label_55

	// *** Basic block 3

.PasteTokens_label_32:
	addi        a0, s0, -64
	call        CurrentToken

	// *** Basic block 4

	li          t0, 11		// 0xb ASCII \xb
	bne         a0, t0, .PasteTokens_label_44

	// *** Basic block 5

	addi        a0, s0, -64
	call        Paste

	// *** Basic block 6

.PasteTokens_label_44:
	addi        a0, s0, -64
	call        MoveToNextToken

	// *** Basic block 7

	addi        a0, s0, -64
	call        CurrentToken

	// *** Basic block 8

	bne         a0, s1, .PasteTokens_label_32

	// *** Basic block 9

.PasteTokens_label_55:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PasteTokens:
	.size PasteTokens, .func_end_PasteTokens-PasteTokens

	.local  RemovePlacemarkers
	.type RemovePlacemarkers, @function

RemovePlacemarkers:

	// *** Basic block 0

	.local TokenIteratorInit
	.local CurrentToken
	.local EraseCurrentToken
	.local MoveToNextToken
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          t0, a0
	mv          t1, a1
	addi        a0, s0, -64
	mv          a2, t1
	mv          a1, t0
	call        TokenIteratorInit

	// *** Basic block 1

	addi        a0, s0, -64
	call        CurrentToken

	// *** Basic block 2

	li          s1, 19		// 0x13 ASCII \x13
	beq         a0, s1, .RemovePlacemarkers_label_56

	// *** Basic block 3

.RemovePlacemarkers_label_32:
	addi        a0, s0, -64
	call        CurrentToken

	// *** Basic block 4

	li          t0, 12		// 0xc ASCII \xc
	bne         a0, t0, .RemovePlacemarkers_label_45

	// *** Basic block 5

	addi        a0, s0, -64
	call        EraseCurrentToken

	// *** Basic block 6

	j           .RemovePlacemarkers_label_32

	// *** Basic block 7

.RemovePlacemarkers_label_45:
	addi        a0, s0, -64
	call        MoveToNextToken

	// *** Basic block 8

	addi        a0, s0, -64
	call        CurrentToken

	// *** Basic block 9

	bne         a0, s1, .RemovePlacemarkers_label_32

	// *** Basic block 10

.RemovePlacemarkers_label_56:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_RemovePlacemarkers:
	.size RemovePlacemarkers, .func_end_RemovePlacemarkers-RemovePlacemarkers

	.local  ReplaceFunctionLikeMacroText
	.type ReplaceFunctionLikeMacroText, @function

ReplaceFunctionLikeMacroText:

	// *** Basic block 0

	.global StringReplaceString
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, a2
	ld          a0, 8(t0)
	ld          t3, 24(t0)
	sub         a2, t3, t1
	mv          a3, t2
	j           StringReplaceString
.func_end_ReplaceFunctionLikeMacroText:
	.size ReplaceFunctionLikeMacroText, .func_end_ReplaceFunctionLikeMacroText-ReplaceFunctionLikeMacroText

	.local  ProcessFunctionLikeReplacementText
	.type ProcessFunctionLikeReplacementText, @function

ProcessFunctionLikeReplacementText:

	// *** Basic block 0

	.global StringInit
	.local TokenIteratorInit
	.local CurrentToken
	.local GetCurrentTokenSpelling
	.global StringEqual
	.local ReplaceCurrentToken
	.global PreprocessorError
	.local FindMacroArg
	.global StringAppendChar
	.global StringDestruct
	.local PrevToken
	.local NextToken
	.local ReplaceMacrosInTokenizedLine
	.local EraseCurrentToken
	.local SkipSpaceTokens
	.local Detokenize
	.local EncodeLength
	.global StringEscape
	.local MoveToNextToken
	.local PasteTokens
	.local RemovePlacemarkers
	.local ReplaceFunctionLikeMacroText
	addi sp, sp, -416
	// Saved return address (offset 408) and frame pointer (offset 400)
	sd ra, 408(sp)
	sd s0, 400(sp)
	addi s0, sp, 416
	// Local vars at offset -336(s0)
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
	mv          s1, a1
	mv          s2, a0
	mv          s3, a5
	mv          s4, a4
	mv          s5, a2
	mv          s6, a3
	addi        a0, s0, -336
	addi        t0, s1, 104
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 1

	lb          s7, 146(s1)
	addi        a0, s0, -296
	addi        a2, s0, -336
	mv          a1, s2
	call        TokenIteratorInit

	// *** Basic block 2

	addi        a0, s0, -296
	call        CurrentToken

	// *** Basic block 3

	li          s8, 19		// 0x13 ASCII \x13
	beq         a0, s8, .ProcessFunctionLikeReplacementText_label_330

	// *** Basic block 4

.ProcessFunctionLikeReplacementText_label_86:
	addi        a0, s0, -296
	call        CurrentToken

	// *** Basic block 5

	li          s1, 1		// 0x1 ASCII \x1
	beq         a0, s1, .ProcessFunctionLikeReplacementText_label_102

	// *** Basic block 6

	li          t0, 10		// 0xa ASCII \xa
	beq         a0, t0, .ProcessFunctionLikeReplacementText_label_228

	// *** Basic block 7

.ProcessFunctionLikeReplacementText_label_100:
	j           .ProcessFunctionLikeReplacementText_label_319

	// *** Basic block 8

.ProcessFunctionLikeReplacementText_label_102:
	addi        a0, s0, -296
	addi        a1, s0, -256
	call        GetCurrentTokenSpelling

	// *** Basic block 9

	addi        a0, s0, -256
	lla         a1, .str.204
	call        StringEqual

	// *** Basic block 10

	beqz        a0, .ProcessFunctionLikeReplacementText_label_132

	// *** Basic block 11

	beqz        s7, .ProcessFunctionLikeReplacementText_label_123

	// *** Basic block 12

	addi        a0, s0, -296
	mv          a1, s3
	call        ReplaceCurrentToken

	// *** Basic block 13

	j           .ProcessFunctionLikeReplacementText_label_130

	// *** Basic block 14

.ProcessFunctionLikeReplacementText_label_123:
	lla         a1, .str.205
	mv          a0, s2
	call        PreprocessorError

	// *** Basic block 15

.ProcessFunctionLikeReplacementText_label_130:
	j           .ProcessFunctionLikeReplacementText_label_223

	// *** Basic block 16

.ProcessFunctionLikeReplacementText_label_132:
	addi        a1, s0, -256
	mv          a0, s4
	call        FindMacroArg

	// *** Basic block 17

	mv          s7, a0
	beq         s7, x0, .ProcessFunctionLikeReplacementText_label_222

	// *** Basic block 18

	ld          t0, 24(s7)
	bnez        t0, .ProcessFunctionLikeReplacementText_label_173

	// *** Basic block 19

	sd          x0, -216(s0)
	sd          x0, -208(s0)
	sd          x0, -200(s0)
	sd          x0, -192(s0)
	sd          x0, -184(s0)
	sb          x0, -216(s0)
	addi        a0, s0, -216
	li          t0, 12		// 0xc ASCII \xc
	mv          a1, t0
	call        StringAppendChar

	// *** Basic block 20

	addi        a0, s0, -296
	addi        a1, s0, -216
	call        ReplaceCurrentToken

	// *** Basic block 21

	addi        a0, s0, -216
	call        StringDestruct

	// *** Basic block 22

	j           .ProcessFunctionLikeReplacementText_label_221

	// *** Basic block 23

.ProcessFunctionLikeReplacementText_label_173:
	addi        a0, s0, -296
	call        PrevToken

	// *** Basic block 24

	addi        t0, a0, -11
	seqz        s10, t0
	li          t0, 11		// 0xb ASCII \xb
	beq         a0, t0, .ProcessFunctionLikeReplacementText_label_188

	// *** Basic block 25

	addi        a0, s0, -296
	call        NextToken

	// *** Basic block 26

	addi        t0, a0, -11
	seqz        s10, t0

	// *** Basic block 27

.ProcessFunctionLikeReplacementText_label_188:
	beqz        s10, .ProcessFunctionLikeReplacementText_label_196

	// *** Basic block 28

	addi        a0, s0, -296
	mv          a1, s7
	call        ReplaceCurrentToken

	// *** Basic block 29

	j           .ProcessFunctionLikeReplacementText_label_220

	// *** Basic block 30

.ProcessFunctionLikeReplacementText_label_196:
	addi        a0, s0, -176
	ld          a1, 16(s7)
	call        StringInit

	// *** Basic block 31

	addi        a1, s0, -176
	mv          a2, s1
	mv          a0, s2
	call        ReplaceMacrosInTokenizedLine

	// *** Basic block 32

	addi        a0, s0, -296
	addi        a1, s0, -176
	call        ReplaceCurrentToken

	// *** Basic block 33

	addi        a0, s0, -176
	call        StringDestruct

	// *** Basic block 34

.ProcessFunctionLikeReplacementText_label_220:

	// *** Basic block 35

.ProcessFunctionLikeReplacementText_label_221:

	// *** Basic block 36

.ProcessFunctionLikeReplacementText_label_222:

	// *** Basic block 37

.ProcessFunctionLikeReplacementText_label_223:
	addi        a0, s0, -256
	call        StringDestruct

	// *** Basic block 38

	j           .ProcessFunctionLikeReplacementText_label_319

	// *** Basic block 39

.ProcessFunctionLikeReplacementText_label_228:
	addi        a0, s0, -296
	call        EraseCurrentToken

	// *** Basic block 40

	addi        a0, s0, -296
	call        SkipSpaceTokens

	// *** Basic block 41

	addi        a0, s0, -296
	addi        a1, s0, -136
	call        GetCurrentTokenSpelling

	// *** Basic block 42

	addi        a1, s0, -136
	mv          a0, s4
	call        FindMacroArg

	// *** Basic block 43

	mv          s9, a0
	bne         s9, x0, .ProcessFunctionLikeReplacementText_label_258

	// *** Basic block 44

	lla         a1, .str.206
	mv          a0, s2
	call        PreprocessorError

	// *** Basic block 45

	j           .ProcessFunctionLikeReplacementText_label_317

	// *** Basic block 46

.ProcessFunctionLikeReplacementText_label_258:
	sd          x0, -96(s0)
	sd          x0, -88(s0)
	sd          x0, -80(s0)
	sd          x0, -72(s0)
	sd          x0, -64(s0)
	sb          x0, -96(s0)
	addi        a2, s0, -96
	mv          a1, s9
	mv          a0, s2
	call        Detokenize

	// *** Basic block 47

	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -24(s0)
	sb          x0, -56(s0)
	addi        a0, s0, -56
	li          t0, 6		// 0x6 ASCII \x6
	mv          a1, t0
	call        StringAppendChar

	// *** Basic block 48

	addi        a0, s0, -56
	addi        t0, s0, -96
	ld          a1, 24(t0)
	call        EncodeLength

	// *** Basic block 49

	addi        a0, s0, -96
	addi        a1, s0, -56
	call        StringEscape

	// *** Basic block 50

	addi        a0, s0, -296
	addi        a1, s0, -56
	call        ReplaceCurrentToken

	// *** Basic block 51

	addi        a0, s0, -96
	call        StringDestruct

	// *** Basic block 52

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 53

.ProcessFunctionLikeReplacementText_label_317:
	j           .ProcessFunctionLikeReplacementText_label_319

	// *** Basic block 54

.ProcessFunctionLikeReplacementText_label_319:
	addi        a0, s0, -296
	call        MoveToNextToken

	// *** Basic block 55

	addi        a0, s0, -296
	call        CurrentToken

	// *** Basic block 56

	bne         a0, s8, .ProcessFunctionLikeReplacementText_label_86

	// *** Basic block 57

.ProcessFunctionLikeReplacementText_label_330:
	addi        a1, s0, -336
	mv          a0, s2
	call        PasteTokens

	// *** Basic block 58

	addi        a1, s0, -336
	mv          a0, s2
	call        RemovePlacemarkers

	// *** Basic block 59

	addi        a1, s0, -336
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a0, s2
	call        ReplaceMacrosInTokenizedLine

	// *** Basic block 60

	addi        a2, s0, -336
	mv          a1, s6
	mv          a0, s5
	call        ReplaceFunctionLikeMacroText

	// *** Basic block 61

	addi        a0, s0, -336
	call        StringDestruct

	// *** Basic block 62

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
.func_end_ProcessFunctionLikeReplacementText:
	.size ProcessFunctionLikeReplacementText, .func_end_ProcessFunctionLikeReplacementText-ProcessFunctionLikeReplacementText

	.local  DeleteActualArg
	.type DeleteActualArg, @function

DeleteActualArg:

	// *** Basic block 0

	.global StringDelete
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 8(t0)
	j           StringDelete
.func_end_DeleteActualArg:
	.size DeleteActualArg, .func_end_DeleteActualArg-DeleteActualArg

	.local  ReplaceFunctionLikeMacro
	.type ReplaceFunctionLikeMacro, @function

ReplaceFunctionLikeMacro:

	// *** Basic block 0

	.local MoveToNextToken
	.local SkipSpaceTokens
	.local CurrentToken
	.global MapInitForStringKeys
	.local CollectActualArguments
	.local ProcessFunctionLikeReplacementText
	.global MapTraverse
	.local DeleteActualArg
	.global MapDestruct
	.global StringDestruct
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -96(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a2
	mv          s2, a0
	mv          s3, a3
	mv          s4, a1
	mv          s5, a4
	ld          s6, 24(s1)
	mv          a0, s1
	call        MoveToNextToken

	// *** Basic block 1

	mv          a0, s1
	call        SkipSpaceTokens

	// *** Basic block 2

	mv          a0, s1
	call        CurrentToken

	// *** Basic block 3

	li          t0, 13		// 0xd ASCII \xd
	beq         a0, t0, .ReplaceFunctionLikeMacro_label_52

	// *** Basic block 4

.ReplaceFunctionLikeMacro_label_49:
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

	// *** Basic block 5

.ReplaceFunctionLikeMacro_label_52:
	mv          a0, s1
	call        MoveToNextToken

	// *** Basic block 6

	addi        a0, s0, -96
	call        MapInitForStringKeys

	// *** Basic block 7

	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	addi        a4, s0, -96
	addi        a5, s0, -64
	mv          a6, s5
	mv          a3, s4
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        CollectActualArguments

	// *** Basic block 8

	addi        a4, s0, -96
	addi        a5, s0, -64
	mv          a3, s6
	mv          a2, s1
	mv          a1, s4
	mv          a0, s2
	call        ProcessFunctionLikeReplacementText

	// *** Basic block 9

	addi        a0, s0, -96
	mv          a2, x0
	la          t0, DeleteActualArg
	mv          a1, t0
	call        MapTraverse

	// *** Basic block 10

	addi        a0, s0, -96
	call        MapDestruct

	// *** Basic block 11

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 12

	j           .ReplaceFunctionLikeMacro_label_49
.func_end_ReplaceFunctionLikeMacro:
	.size ReplaceFunctionLikeMacro, .func_end_ReplaceFunctionLikeMacro-ReplaceFunctionLikeMacro

	.local  ProcessPossibleMacro
	.type ProcessPossibleMacro, @function

ProcessPossibleMacro:

	// *** Basic block 0

	.global StringEqual
	.global StringPrintf
	.local TokenizeAndReplaceCurrentToken
	.global compiler
	.global HashTableSearch
	.global MacroDisable
	.local ReplaceFunctionLikeMacro
	.global StringSet
	.local PasteTokens
	.local RemovePlacemarkers
	.local ReplaceMacrosInTokenizedLine
	.local ReplaceCurrentToken
	.global MacroEnable
	.global StringDestruct
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -64(s0)
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, a4
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	lla         a1, .str.207
	mv          a0, s1
	call        StringEqual

	// *** Basic block 1

	beqz        a0, .ProcessPossibleMacro_label_72

	// *** Basic block 2

	j           .ProcessPossibleMacro_label_251

	// *** Basic block 3

.ProcessPossibleMacro_label_72:
	lla         a1, .str.208
	mv          a0, s1
	call        StringEqual

	// *** Basic block 4

	beqz        a0, .ProcessPossibleMacro_label_99

	// *** Basic block 5

	addi        a0, s0, -64
	lla         a1, .str.209
	ld          s6, 160(s2)
	ld          a2, 0(s6)
	call        StringPrintf

	// *** Basic block 6

	addi        a1, s0, -64
	lb          a2, 178(s6)
	mv          a0, s3
	call        TokenizeAndReplaceCurrentToken

	// *** Basic block 7

	j           .ProcessPossibleMacro_label_250

	// *** Basic block 8

.ProcessPossibleMacro_label_99:
	lla         a1, .str.210
	mv          a0, s1
	call        StringEqual

	// *** Basic block 9

	beqz        a0, .ProcessPossibleMacro_label_128

	// *** Basic block 10

	addi        a0, s0, -64
	lla         a1, .str.211
	ld          s6, 160(s2)
	ld          t0, 0(s6)
	lw          a2, 80(t0)
	call        StringPrintf

	// *** Basic block 11

	addi        a1, s0, -64
	lb          a2, 178(s6)
	mv          a0, s3
	call        TokenizeAndReplaceCurrentToken

	// *** Basic block 12

	j           .ProcessPossibleMacro_label_249

	// *** Basic block 13

.ProcessPossibleMacro_label_128:
	lla         a1, .str.212
	mv          a0, s1
	call        StringEqual

	// *** Basic block 14

	mv          s6, a0
	bnez        a0, .ProcessPossibleMacro_label_145

	// *** Basic block 15

	lla         a1, .str.213
	mv          a0, s1
	call        StringEqual

	// *** Basic block 16

	mv          s6, a0

	// *** Basic block 17

.ProcessPossibleMacro_label_145:
	beqz        s6, .ProcessPossibleMacro_label_177

	// *** Basic block 18

	la          t0, compiler
	ld          t0, 0(t0)
	ld          s7, 1096(t0)
	beq         s7, x0, .ProcessPossibleMacro_label_175

	// *** Basic block 19

	addi        a0, s0, -64
	lla         a1, .str.214
	ld          t0, 32(s7)
	ld          a2, 16(t0)
	call        StringPrintf

	// *** Basic block 20

	addi        a1, s0, -64
	ld          t0, 160(s2)
	lb          a2, 178(t0)
	mv          a0, s3
	call        TokenizeAndReplaceCurrentToken

	// *** Basic block 21

.ProcessPossibleMacro_label_175:
	j           .ProcessPossibleMacro_label_248

	// *** Basic block 22

.ProcessPossibleMacro_label_177:
	ld          a1, 16(s1)
	mv          a0, s2
	call        HashTableSearch

	// *** Basic block 23

	mv          s7, a0
	sub         t1, s7, x0
	snez        t0, t1
	beq         s7, x0, .ProcessPossibleMacro_label_192

	// *** Basic block 24

	lb          t0, 147(s7)

	// *** Basic block 25

.ProcessPossibleMacro_label_192:
	beqz        t0, .ProcessPossibleMacro_label_247

	// *** Basic block 26

	mv          a0, s7
	call        MacroDisable

	// *** Basic block 27

	lb          t0, 144(s7)
	beqz        t0, .ProcessPossibleMacro_label_212

	// *** Basic block 28

	mv          a4, s5
	mv          a3, s4
	mv          a2, s3
	mv          a1, s7
	mv          a0, s2
	call        ReplaceFunctionLikeMacro

	// *** Basic block 29

	j           .ProcessPossibleMacro_label_243

	// *** Basic block 30

.ProcessPossibleMacro_label_212:
	addi        a0, s0, -64
	addi        t0, s7, 104
	ld          a1, 16(t0)
	call        StringSet

	// *** Basic block 31

	addi        a1, s0, -64
	mv          a0, s2
	call        PasteTokens

	// *** Basic block 32

	addi        a1, s0, -64
	mv          a0, s2
	call        RemovePlacemarkers

	// *** Basic block 33

	addi        a1, s0, -64
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a0, s2
	call        ReplaceMacrosInTokenizedLine

	// *** Basic block 34

	addi        a1, s0, -64
	mv          a0, s3
	call        ReplaceCurrentToken

	// *** Basic block 35

.ProcessPossibleMacro_label_243:
	mv          a0, s7
	call        MacroEnable

	// *** Basic block 36

.ProcessPossibleMacro_label_247:

	// *** Basic block 37

.ProcessPossibleMacro_label_248:

	// *** Basic block 38

.ProcessPossibleMacro_label_249:

	// *** Basic block 39

.ProcessPossibleMacro_label_250:

	// *** Basic block 40

.ProcessPossibleMacro_label_251:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 41

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
.func_end_ProcessPossibleMacro:
	.size ProcessPossibleMacro, .func_end_ProcessPossibleMacro-ProcessPossibleMacro

	.local  ReplaceMacrosInTokenizedLine
	.type ReplaceMacrosInTokenizedLine, @function

ReplaceMacrosInTokenizedLine:

	// *** Basic block 0

	.local TokenIteratorInit
	.local CurrentToken
	.local GetCurrentTokenSpelling
	.local ProcessPossibleMacro
	.global StringDestruct
	.local MoveToNextToken
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -96(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	addi        a0, s0, -96
	mv          a2, s2
	mv          a1, s1
	call        TokenIteratorInit

	// *** Basic block 1

	addi        a0, s0, -96
	call        CurrentToken

	// *** Basic block 2

	li          s4, 19		// 0x13 ASCII \x13
	beq         a0, s4, .ReplaceMacrosInTokenizedLine_label_78

	// *** Basic block 3

.ReplaceMacrosInTokenizedLine_label_36:
	addi        a0, s0, -96
	call        CurrentToken

	// *** Basic block 4

	li          t0, 1		// 0x1 ASCII \x1
	bne         a0, t0, .ReplaceMacrosInTokenizedLine_label_67

	// *** Basic block 5

	addi        a0, s0, -96
	addi        a1, s0, -56
	call        GetCurrentTokenSpelling

	// *** Basic block 6

	addi        a1, s0, -56
	addi        a3, s0, -96
	mv          a4, s3
	mv          a2, s2
	mv          a0, s1
	call        ProcessPossibleMacro

	// *** Basic block 7

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 8

.ReplaceMacrosInTokenizedLine_label_67:
	addi        a0, s0, -96
	call        MoveToNextToken

	// *** Basic block 9

	addi        a0, s0, -96
	call        CurrentToken

	// *** Basic block 10

	bne         a0, s4, .ReplaceMacrosInTokenizedLine_label_36

	// *** Basic block 11

.ReplaceMacrosInTokenizedLine_label_78:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ReplaceMacrosInTokenizedLine:
	.size ReplaceMacrosInTokenizedLine, .func_end_ReplaceMacrosInTokenizedLine-ReplaceMacrosInTokenizedLine

	.global PreprocessorReplaceMacros
	.type PreprocessorReplaceMacros, @function

PreprocessorReplaceMacros:

	// *** Basic block 0

	.local SkipToEndOfComment
	.local Tokenize
	.local ReplaceMacrosInTokenizedLine
	.global StringClear
	.local Detokenize
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	ld          t0, 24(s1)
	bnez        t0, .PreprocessorReplaceMacros_label_28

	// *** Basic block 1

.PreprocessorReplaceMacros_label_25:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.PreprocessorReplaceMacros_label_28:
	mv          s3, x0
	ld          s4, 160(s2)
	lb          t0, 177(s4)
	beqz        t0, .PreprocessorReplaceMacros_label_42

	// *** Basic block 3

	mv          a1, x0
	mv          a0, s1
	call        SkipToEndOfComment

	// *** Basic block 4

	mv          s3, a0

	// *** Basic block 5

.PreprocessorReplaceMacros_label_42:
	addi        a2, s0, -64
	lb          a5, 178(s4)
	mv          a6, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a4, t0
	mv          a3, s3
	mv          a1, s1
	mv          a0, s2
	call        Tokenize

	// *** Basic block 6

	addi        a1, s0, -64
	mv          a2, x0
	mv          a0, s2
	call        ReplaceMacrosInTokenizedLine

	// *** Basic block 7

	beqz        s3, .PreprocessorReplaceMacros_label_79

	// *** Basic block 8

	sd          s3, 24(s1)
	j           .PreprocessorReplaceMacros_label_83

	// *** Basic block 9

.PreprocessorReplaceMacros_label_79:
	mv          a0, s1
	call        StringClear

	// *** Basic block 10

.PreprocessorReplaceMacros_label_83:
	addi        a1, s0, -64
	mv          a2, s1
	mv          a0, s2
	call        Detokenize

	// *** Basic block 11

	j           .PreprocessorReplaceMacros_label_25
.func_end_PreprocessorReplaceMacros:
	.size PreprocessorReplaceMacros, .func_end_PreprocessorReplaceMacros-PreprocessorReplaceMacros

.PCend:
	.data
preprocessor_commands:
	.type   preprocessor_commands,@object
	.local  preprocessor_commands
	.size   preprocessor_commands,240
	.p2align  3
	.long    .str.185
	.global Define
	.long    Define
	.long    .str.186
	.global Include
	.long    Include
	.long    .str.187
	.global If
	.long    If
	.long    .str.188
	.global Ifdef
	.long    Ifdef
	.long    .str.189
	.global Ifndef
	.long    Ifndef
	.long    .str.190
	.global Endif
	.long    Endif
	.long    .str.191
	.global Elif
	.long    Elif
	.long    .str.192
	.global Else
	.long    Else
	.long    .str.193
	.global Line
	.long    Line
	.long    .str.194
	.global Error
	.long    Error
	.long    .str.195
	.global Pragma
	.long    Pragma
	.long    .str.196
	.global Undef
	.long    Undef
	.long    .str.197
	.global Warning
	.long    Warning
	.long    .str.198
	.global IncludeNext
	.long    IncludeNext
	.word   0
	.space  4
	.word   0
	.space  4

	.type   true_value,@object
	.local  true_value
	.comm   true_value,4,4

	.type   macro_not_defined,@object
	.local  macro_not_defined
	.comm   macro_not_defined,4,4

	.type   inverted_value,@object
	.local  inverted_value
	.comm   inverted_value,4,4

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "__STDC__"
	.type .str.1, @object
	.size .str.1, 9

.str.2:
	.asciz "1"
	.type .str.2, @object
	.size .str.2, 2

.str.3:
	.asciz "__STDC_HOSTED__"
	.type .str.3, @object
	.size .str.3, 16

.str.4:
	.asciz "1"
	.type .str.4, @object
	.size .str.4, 2

.str.5:
	.asciz "__STDC_VERSION__"
	.type .str.5, @object
	.size .str.5, 17

.str.6:
	.asciz "199901L"
	.type .str.6, @object
	.size .str.6, 8

.str.7:
	.asciz "__STDC_MB_MIGHT_NEQ_WC__"
	.type .str.7, @object
	.size .str.7, 25

.str.8:
	.asciz "1"
	.type .str.8, @object
	.size .str.8, 2

.str.9:
	.asciz "__DAVECC__"
	.type .str.9, @object
	.size .str.9, 11

.str.10:
	.asciz "1"
	.type .str.10, @object
	.size .str.10, 2

.str.11:
	.asciz "__DATE__"
	.type .str.11, @object
	.size .str.11, 9

.str.12:
	.asciz "__TIME__"
	.type .str.12, @object
	.size .str.12, 9

.str.13:
	.asciz "__GNUC__"
	.type .str.13, @object
	.size .str.13, 9

.str.14:
	.asciz "4"
	.type .str.14, @object
	.size .str.14, 2

.str.15:
	.asciz "__llvm__"
	.type .str.15, @object
	.size .str.15, 9

.str.16:
	.asciz "1"
	.type .str.16, @object
	.size .str.16, 2

.str.17:
	.asciz "__linux__"
	.type .str.17, @object
	.size .str.17, 10

.str.18:
	.asciz "1"
	.type .str.18, @object
	.size .str.18, 2

.str.19:
	.asciz "__SIZE_TYPE__"
	.type .str.19, @object
	.size .str.19, 14

.str.20:
	.asciz "unsigned long"
	.type .str.20, @object
	.size .str.20, 14

.str.21:
	.asciz "__PTRDIFF_TYPE__"
	.type .str.21, @object
	.size .str.21, 17

.str.22:
	.asciz "unsigned long"
	.type .str.22, @object
	.size .str.22, 14

.str.23:
	.asciz "__WCHAR_TYPE__"
	.type .str.23, @object
	.size .str.23, 15

.str.24:
	.asciz "int"
	.type .str.24, @object
	.size .str.24, 4

.str.25:
	.asciz "__WINT_TYPE__"
	.type .str.25, @object
	.size .str.25, 14

.str.26:
	.asciz "int"
	.type .str.26, @object
	.size .str.26, 4

.str.27:
	.asciz "__INTMAX_TYPE__"
	.type .str.27, @object
	.size .str.27, 16

.str.28:
	.asciz "int"
	.type .str.28, @object
	.size .str.28, 4

.str.29:
	.asciz "__UINTMAX_TYPE__"
	.type .str.29, @object
	.size .str.29, 17

.str.30:
	.asciz "int"
	.type .str.30, @object
	.size .str.30, 4

.str.31:
	.asciz "__SIG_ATOMIC_TYPE__"
	.type .str.31, @object
	.size .str.31, 20

.str.32:
	.asciz "int"
	.type .str.32, @object
	.size .str.32, 4

.str.33:
	.asciz "__INT8_TYPE__"
	.type .str.33, @object
	.size .str.33, 14

.str.34:
	.asciz "char"
	.type .str.34, @object
	.size .str.34, 5

.str.35:
	.asciz "__INT16_TYPE__"
	.type .str.35, @object
	.size .str.35, 15

.str.36:
	.asciz "short"
	.type .str.36, @object
	.size .str.36, 6

.str.37:
	.asciz "__INT32_TYPE__"
	.type .str.37, @object
	.size .str.37, 15

.str.38:
	.asciz "int"
	.type .str.38, @object
	.size .str.38, 4

.str.39:
	.asciz "__INT64_TYPE__"
	.type .str.39, @object
	.size .str.39, 15

.str.40:
	.asciz "long long"
	.type .str.40, @object
	.size .str.40, 10

.str.41:
	.asciz "__UINT8_TYPE__"
	.type .str.41, @object
	.size .str.41, 15

.str.42:
	.asciz "unsigned char"
	.type .str.42, @object
	.size .str.42, 14

.str.43:
	.asciz "__UINT16_TYPE__"
	.type .str.43, @object
	.size .str.43, 16

.str.44:
	.asciz "unsigned short"
	.type .str.44, @object
	.size .str.44, 15

.str.45:
	.asciz "__UINT32_TYPE__"
	.type .str.45, @object
	.size .str.45, 16

.str.46:
	.asciz "unsigned int"
	.type .str.46, @object
	.size .str.46, 13

.str.47:
	.asciz "__UINT64_TYPE__"
	.type .str.47, @object
	.size .str.47, 16

.str.48:
	.asciz "unsigned long long"
	.type .str.48, @object
	.size .str.48, 19

.str.49:
	.asciz "__INT_LEAST8_TYPE__"
	.type .str.49, @object
	.size .str.49, 20

.str.50:
	.asciz "int"
	.type .str.50, @object
	.size .str.50, 4

.str.51:
	.asciz "__INT_LEAST16_TYPE__"
	.type .str.51, @object
	.size .str.51, 21

.str.52:
	.asciz "int"
	.type .str.52, @object
	.size .str.52, 4

.str.53:
	.asciz "__INT_LEAST32_TYPE__"
	.type .str.53, @object
	.size .str.53, 21

.str.54:
	.asciz "int"
	.type .str.54, @object
	.size .str.54, 4

.str.55:
	.asciz "__INT_LEAST64_TYPE__"
	.type .str.55, @object
	.size .str.55, 21

.str.56:
	.asciz "int"
	.type .str.56, @object
	.size .str.56, 4

.str.57:
	.asciz "__UINT_LEAST8_TYPE__"
	.type .str.57, @object
	.size .str.57, 21

.str.58:
	.asciz "int"
	.type .str.58, @object
	.size .str.58, 4

.str.59:
	.asciz "__UINT_LEAST16_TYPE__"
	.type .str.59, @object
	.size .str.59, 22

.str.60:
	.asciz "int"
	.type .str.60, @object
	.size .str.60, 4

.str.61:
	.asciz "__UINT_LEAST32_TYPE__"
	.type .str.61, @object
	.size .str.61, 22

.str.62:
	.asciz "int"
	.type .str.62, @object
	.size .str.62, 4

.str.63:
	.asciz "__UINT_LEAST64_TYPE__"
	.type .str.63, @object
	.size .str.63, 22

.str.64:
	.asciz "int"
	.type .str.64, @object
	.size .str.64, 4

.str.65:
	.asciz "__INT_FAST8_TYPE__"
	.type .str.65, @object
	.size .str.65, 19

.str.66:
	.asciz "int"
	.type .str.66, @object
	.size .str.66, 4

.str.67:
	.asciz "__INT_FAST16_TYPE__"
	.type .str.67, @object
	.size .str.67, 20

.str.68:
	.asciz "int"
	.type .str.68, @object
	.size .str.68, 4

.str.69:
	.asciz "__INT_FAST32_TYPE__"
	.type .str.69, @object
	.size .str.69, 20

.str.70:
	.asciz "int"
	.type .str.70, @object
	.size .str.70, 4

.str.71:
	.asciz "__INT_FAST64_TYPE__"
	.type .str.71, @object
	.size .str.71, 20

.str.72:
	.asciz "int"
	.type .str.72, @object
	.size .str.72, 4

.str.73:
	.asciz "__UINT_FAST8_TYPE__"
	.type .str.73, @object
	.size .str.73, 20

.str.74:
	.asciz "int"
	.type .str.74, @object
	.size .str.74, 4

.str.75:
	.asciz "__UINT_FAST16_TYPE__"
	.type .str.75, @object
	.size .str.75, 21

.str.76:
	.asciz "int"
	.type .str.76, @object
	.size .str.76, 4

.str.77:
	.asciz "__UINT_FAST32_TYPE__"
	.type .str.77, @object
	.size .str.77, 21

.str.78:
	.asciz "int"
	.type .str.78, @object
	.size .str.78, 4

.str.79:
	.asciz "__UINT_FAST64_TYPE__"
	.type .str.79, @object
	.size .str.79, 21

.str.80:
	.asciz "int"
	.type .str.80, @object
	.size .str.80, 4

.str.81:
	.asciz "__INTPTR_TYPE__"
	.type .str.81, @object
	.size .str.81, 16

.str.82:
	.asciz "int*"
	.type .str.82, @object
	.size .str.82, 5

.str.83:
	.asciz "__UINTPTR_TYPE__"
	.type .str.83, @object
	.size .str.83, 17

.str.84:
	.asciz "unsigned int*"
	.type .str.84, @object
	.size .str.84, 14

.str.85:
	.asciz "x86_64"
	.type .str.85, @object
	.size .str.85, 7

.str.86:
	.asciz "__x86_64__"
	.type .str.86, @object
	.size .str.86, 11

.str.87:
	.asciz "1"
	.type .str.87, @object
	.size .str.87, 2

.str.88:
	.asciz "arm"
	.type .str.88, @object
	.size .str.88, 4

.str.89:
	.asciz "__arm__"
	.type .str.89, @object
	.size .str.89, 8

.str.90:
	.asciz "1"
	.type .str.90, @object
	.size .str.90, 2

.str.91:
	.asciz "p-code"
	.type .str.91, @object
	.size .str.91, 7

.str.92:
	.asciz "pcode"
	.type .str.92, @object
	.size .str.92, 6

.str.93:
	.asciz "__p_code__"
	.type .str.93, @object
	.size .str.93, 11

.str.94:
	.asciz "1"
	.type .str.94, @object
	.size .str.94, 2

.str.95:
	.asciz "__x86_64__"
	.type .str.95, @object
	.size .str.95, 11

.str.96:
	.asciz "1"
	.type .str.96, @object
	.size .str.96, 2

.str.97:
	.asciz "risc-v"
	.type .str.97, @object
	.size .str.97, 7

.str.98:
	.asciz "riscv"
	.type .str.98, @object
	.size .str.98, 6

.str.99:
	.asciz "__risc_v__"
	.type .str.99, @object
	.size .str.99, 11

.str.100:
	.asciz "1"
	.type .str.100, @object
	.size .str.100, 2

.str.101:
	.asciz "__x86_64__"
	.type .str.101, @object
	.size .str.101, 11

.str.102:
	.asciz "1"
	.type .str.102, @object
	.size .str.102, 2

.str.103:
	.asciz "macros"
	.type .str.103, @object
	.size .str.103, 7

.str.104:
	.asciz "/Users/dallison/Google Drive/c_compiler/libc/include"
	.type .str.104, @object
	.size .str.104, 53

.str.105:
	.asciz "##"
	.type .str.105, @object
	.size .str.105, 3

.str.106:
	.asciz "defined"
	.type .str.106, @object
	.size .str.106, 8

.str.107:
	.asciz "identifier: "
	.type .str.107, @object
	.size .str.107, 13

.str.108:
	.asciz "other_string: "
	.type .str.108, @object
	.size .str.108, 15

.str.109:
	.asciz "number: "
	.type .str.109, @object
	.size .str.109, 9

.str.110:
	.asciz "defined: "
	.type .str.110, @object
	.size .str.110, 10

.str.111:
	.asciz "literal: "
	.type .str.111, @object
	.size .str.111, 10

.str.112:
	.asciz "wide_literal: "
	.type .str.112, @object
	.size .str.112, 15

.str.113:
	.asciz "char_literal: "
	.type .str.113, @object
	.size .str.113, 15

.str.114:
	.asciz "system_header: "
	.type .str.114, @object
	.size .str.114, 16

.str.115:
	.asciz "wide_char_literal: "
	.type .str.115, @object
	.size .str.115, 20

.str.116:
	.asciz "other_char: %c\n"
	.type .str.116, @object
	.size .str.116, 16

.str.117:
	.asciz "hash\n"
	.type .str.117, @object
	.size .str.117, 6

.str.118:
	.asciz "hashhash\n"
	.type .str.118, @object
	.size .str.118, 10

.str.119:
	.asciz "placemarker\n"
	.type .str.119, @object
	.size .str.119, 13

.str.120:
	.asciz "openparen\n"
	.type .str.120, @object
	.size .str.120, 11

.str.121:
	.asciz "closeparen\n"
	.type .str.121, @object
	.size .str.121, 12

.str.122:
	.asciz "comma\n"
	.type .str.122, @object
	.size .str.122, 7

.str.123:
	.asciz "space\n"
	.type .str.123, @object
	.size .str.123, 7

.str.124:
	.asciz "comment\n"
	.type .str.124, @object
	.size .str.124, 9

.str.125:
	.asciz "end\n"
	.type .str.125, @object
	.size .str.125, 5

.str.126:
	.asciz "defined"
	.type .str.126, @object
	.size .str.126, 8

.str.127:
	.asciz "defined(%s)"
	.type .str.127, @object
	.size .str.127, 12

.str.128:
	.asciz "\"%s\""
	.type .str.128, @object
	.size .str.128, 5

.str.129:
	.asciz "L\"%s\""
	.type .str.129, @object
	.size .str.129, 6

.str.130:
	.asciz "\'%s\'"
	.type .str.130, @object
	.size .str.130, 5

.str.131:
	.asciz "<%s>"
	.type .str.131, @object
	.size .str.131, 5

.str.132:
	.asciz "L\'%s\'"
	.type .str.132, @object
	.size .str.132, 6

.str.133:
	.asciz "#"
	.type .str.133, @object
	.size .str.133, 2

.str.134:
	.asciz "##"
	.type .str.134, @object
	.size .str.134, 3

.str.135:
	.asciz "("
	.type .str.135, @object
	.size .str.135, 2

.str.136:
	.asciz ")"
	.type .str.136, @object
	.size .str.136, 2

.str.137:
	.asciz ","
	.type .str.137, @object
	.size .str.137, 2

.str.138:
	.asciz " "
	.type .str.138, @object
	.size .str.138, 2

.str.139:
	.asciz "%s/%s"
	.type .str.139, @object
	.size .str.139, 6

.str.140:
	.asciz "r"
	.type .str.140, @object
	.size .str.140, 2

.str.141:
	.asciz "Duplicate macro argument %s"
	.type .str.141, @object
	.size .str.141, 28

.str.142:
	.asciz "Missing \')\' for function-like macro %s"
	.type .str.142, @object
	.size .str.142, 39

.str.143:
	.asciz "## is not allowed at start of replacement text"
	.type .str.143, @object
	.size .str.143, 47

.str.144:
	.asciz "## is not allowed at end of replacement text"
	.type .str.144, @object
	.size .str.144, 45

.str.145:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.145, @object
	.size .str.145, 30

.str.146:
	.asciz "preprocessor.c"
	.type .str.146, @object
	.size .str.146, 15

.str.147:
	.asciz "ok"
	.type .str.147, @object
	.size .str.147, 3

.str.148:
	.asciz "macro-redef"
	.type .str.148, @object
	.size .str.148, 12

.str.149:
	.asciz "Macro %s redefined; previously defined at %s:%d"
	.type .str.149, @object
	.size .str.149, 48

.str.150:
	.asciz "extra-tokens"
	.type .str.150, @object
	.size .str.150, 13

.str.151:
	.asciz "Extra tokens after #undef"
	.type .str.151, @object
	.size .str.151, 26

.str.152:
	.asciz "Missing > for #include filename"
	.type .str.152, @object
	.size .str.152, 32

.str.153:
	.asciz "Missing closing \" for #include filename"
	.type .str.153, @object
	.size .str.153, 40

.str.154:
	.asciz "Bad #include filename; need <...> or \"...\""
	.type .str.154, @object
	.size .str.154, 43

.str.155:
	.asciz "Empty #include filename"
	.type .str.155, @object
	.size .str.155, 24

.str.156:
	.asciz "Looked in the following locations:"
	.type .str.156, @object
	.size .str.156, 35

.str.157:
	.asciz "%s/%s"
	.type .str.157, @object
	.size .str.157, 6

.str.158:
	.asciz "  %s"
	.type .str.158, @object
	.size .str.158, 5

.str.159:
	.asciz "%s/%s"
	.type .str.159, @object
	.size .str.159, 6

.str.160:
	.asciz "  %s"
	.type .str.160, @object
	.size .str.160, 5

.str.161:
	.asciz "#include needs \"filename\" or <filename>"
	.type .str.161, @object
	.size .str.161, 40

.str.162:
	.asciz "Fatal error: cannot open include file \"%s\""
	.type .str.162, @object
	.size .str.162, 43

.str.163:
	.asciz "Including file %s\n"
	.type .str.163, @object
	.size .str.163, 19

.str.164:
	.asciz "\n\n"
	.type .str.164, @object
	.size .str.164, 3

.str.165:
	.asciz "Extraneous #endif"
	.type .str.165, @object
	.size .str.165, 18

.str.166:
	.asciz "extra-tokens"
	.type .str.166, @object
	.size .str.166, 13

.str.167:
	.asciz "Extra tokens after #endif"
	.type .str.167, @object
	.size .str.167, 26

.str.168:
	.asciz "Expected macro name after #ifndef"
	.type .str.168, @object
	.size .str.168, 34

.str.169:
	.asciz "extra-tokens"
	.type .str.169, @object
	.size .str.169, 13

.str.170:
	.asciz "Extra tokens after #ifndef"
	.type .str.170, @object
	.size .str.170, 27

.str.171:
	.asciz "Expected macro name after #ifdef"
	.type .str.171, @object
	.size .str.171, 33

.str.172:
	.asciz "extra-tokens"
	.type .str.172, @object
	.size .str.172, 13

.str.173:
	.asciz "Extra tokens after #ifdef"
	.type .str.173, @object
	.size .str.173, 26

.str.174:
	.asciz "#elif outside #if..#endif"
	.type .str.174, @object
	.size .str.174, 26

.str.175:
	.asciz "\n\n"
	.type .str.175, @object
	.size .str.175, 3

.str.176:
	.asciz "#else outside #if..#endif"
	.type .str.176, @object
	.size .str.176, 26

.str.177:
	.asciz "extra-tokens"
	.type .str.177, @object
	.size .str.177, 13

.str.178:
	.asciz "Extra tokens after #else"
	.type .str.178, @object
	.size .str.178, 25

.str.179:
	.asciz "#line neeed a simple digit sequence"
	.type .str.179, @object
	.size .str.179, 36

.str.180:
	.asciz "#line directive requires a positive integer line number"
	.type .str.180, @object
	.size .str.180, 56

.str.181:
	.asciz "Bad filename for #line directive"
	.type .str.181, @object
	.size .str.181, 33

.str.182:
	.asciz "preprocessor"
	.type .str.182, @object
	.size .str.182, 13

.str.183:
	.asciz "warning"
	.type .str.183, @object
	.size .str.183, 8

.str.184:
	.asciz "on"
	.type .str.184, @object
	.size .str.184, 3

.str.185:
	.asciz "define"
	.type .str.185, @object
	.size .str.185, 7

.str.186:
	.asciz "include"
	.type .str.186, @object
	.size .str.186, 8

.str.187:
	.asciz "if"
	.type .str.187, @object
	.size .str.187, 3

.str.188:
	.asciz "ifdef"
	.type .str.188, @object
	.size .str.188, 6

.str.189:
	.asciz "ifndef"
	.type .str.189, @object
	.size .str.189, 7

.str.190:
	.asciz "endif"
	.type .str.190, @object
	.size .str.190, 6

.str.191:
	.asciz "elif"
	.type .str.191, @object
	.size .str.191, 5

.str.192:
	.asciz "else"
	.type .str.192, @object
	.size .str.192, 5

.str.193:
	.asciz "line"
	.type .str.193, @object
	.size .str.193, 5

.str.194:
	.asciz "error"
	.type .str.194, @object
	.size .str.194, 6

.str.195:
	.asciz "pragma"
	.type .str.195, @object
	.size .str.195, 7

.str.196:
	.asciz "undef"
	.type .str.196, @object
	.size .str.196, 6

.str.197:
	.asciz "warning"
	.type .str.197, @object
	.size .str.197, 8

.str.198:
	.asciz "include_next"
	.type .str.198, @object
	.size .str.198, 13

.str.199:
	.asciz "Invalid preprocessor directive %s"
	.type .str.199, @object
	.size .str.199, 34

.str.200:
	.asciz "(null)"
	.type .str.200, @object
	.size .str.200, 1

.str.201:
	.asciz "Missing \')\' for function like macro arguments"
	.type .str.201, @object
	.size .str.201, 46

.str.202:
	.asciz "Too many actual arguments for macro %s; expected %zd, got %d"
	.type .str.202, @object
	.size .str.202, 61

.str.203:
	.asciz "Insufficient actual arguments for macro %s; expected %zd, got %d"
	.type .str.203, @object
	.size .str.203, 65

.str.204:
	.asciz "__VA_ARGS__"
	.type .str.204, @object
	.size .str.204, 12

.str.205:
	.asciz "Use of __VA_ARGS__ outside of varargs macro"
	.type .str.205, @object
	.size .str.205, 44

.str.206:
	.asciz "# is not followed by a macro argument name"
	.type .str.206, @object
	.size .str.206, 43

.str.207:
	.asciz "_Pragma"
	.type .str.207, @object
	.size .str.207, 8

.str.208:
	.asciz "__FILE__"
	.type .str.208, @object
	.size .str.208, 9

.str.209:
	.asciz "\"%s\""
	.type .str.209, @object
	.size .str.209, 5

.str.210:
	.asciz "__LINE__"
	.type .str.210, @object
	.size .str.210, 9

.str.211:
	.asciz "%d"
	.type .str.211, @object
	.size .str.211, 3

.str.212:
	.asciz "__func__"
	.type .str.212, @object
	.size .str.212, 9

.str.213:
	.asciz "__FUNCTION__"
	.type .str.213, @object
	.size .str.213, 13

.str.214:
	.asciz "\"%s\""
	.type .str.214, @object
	.size .str.214, 5

