	.file   "ssa.c"
	.text
	.option pic
.PCbegin:
	.local  FindVariableStack
	.type FindVariableStack, @function

FindVariableStack:

	// *** Basic block 0

	.global MapFindPointerKey
	.global NewVector
	.global MapInsert
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
	call        MapFindPointerKey

	// *** Basic block 1

	mv          s3, a0
	beq         s3, x0, .FindVariableStack_label_29

	// *** Basic block 2

	mv          a0, s3

	// *** Basic block 3

.FindVariableStack_label_26:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.FindVariableStack_label_29:
	call        NewVector

	// *** Basic block 5

	mv          s4, a0
	sd          s2, -32(s0)
	addi        t0, s0, -32
	sd          s4, 8(t0)
	addi        sp, sp, -16
	ld          t0, -32(s0)
	sd          t0, 0(sp)
	ld          t0, -24(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 6

	mv          a0, s4
	j           .FindVariableStack_label_26
.func_end_FindVariableStack:
	.size FindVariableStack, .func_end_FindVariableStack-FindVariableStack

	.local  RenameTopVariable
	.type RenameTopVariable, @function

RenameTopVariable:

	// *** Basic block 0

	.global MapFindPointerKey
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
	// End of stack frame
	mv          s1, a2
	call        MapFindPointerKey

	// *** Basic block 1

	mv          s2, a0
	beq         s2, x0, .RenameTopVariable_label_33

	// *** Basic block 2

	j           .RenameTopVariable_label_50

	// *** Basic block 3

.RenameTopVariable_label_33:
	lla         a0, .str.1
	lla         a1, .str.2
	lla         a3, .str.3
	li          t0, 39		// 0x27 ASCII '''
	mv          a2, t0
	call        printf

	// *** Basic block 4

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           abort

	// *** Basic block 5

.RenameTopVariable_label_50:
	ld          t0, 0(s2)
	ld          t1, 8(s2)
	addi        t1, t1, -1
	slli        t1, t1, 3
	add         s3, t0, t1
	sd          s1, 0(s3)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_RenameTopVariable:
	.size RenameTopVariable, .func_end_RenameTopVariable-RenameTopVariable

	.local  GetTopVariable
	.type GetTopVariable, @function

GetTopVariable:

	// *** Basic block 0

	.global MapFindPointerKey
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	call        MapFindPointerKey

	// *** Basic block 1

	mv          s1, a0
	bne         s1, x0, .GetTopVariable_label_29

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.GetTopVariable_label_26:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.GetTopVariable_label_29:
	ld          t0, 8(s1)
	bnez        t0, .GetTopVariable_label_37

	// *** Basic block 5

	mv          a0, x0
	j           .GetTopVariable_label_26

	// *** Basic block 6

.GetTopVariable_label_37:
	ld          t1, 0(s1)
	addi        t0, t0, -1
	slli        t0, t0, 3
	add         t0, t1, t0
	ld          a0, 0(t0)
	j           .GetTopVariable_label_26
.func_end_GetTopVariable:
	.size GetTopVariable, .func_end_GetTopVariable-GetTopVariable

	.local  FindVariableReference
	.type FindVariableReference, @function

FindVariableReference:

	// *** Basic block 0

	.global IRIsVariable
	.global IRIsLoad
	.global IRIsStore
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
	mv          s3, a0
	beq         s2, x0, .FindVariableReference_label_80

	// *** Basic block 1

.FindVariableReference_label_25:
	mv          a0, s2
	call        IRIsVariable

	// *** Basic block 2

	beqz        a0, .FindVariableReference_label_43

	// *** Basic block 3

	mv          s4, s2
	ld          t0, 136(s4)
	bne         t0, s1, .FindVariableReference_label_42

	// *** Basic block 4

	mv          a0, s3

	// *** Basic block 5

.FindVariableReference_label_39:
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

	// *** Basic block 6

.FindVariableReference_label_42:

	// *** Basic block 7

.FindVariableReference_label_43:
	lw          s5, 20(s2)
	li          t0, 102		// 0x66 ASCII 'f'
	bne         s5, t0, .FindVariableReference_label_54

	// *** Basic block 8

	mv          a0, s2
	j           .FindVariableReference_label_39

	// *** Basic block 9

.FindVariableReference_label_54:
	mv          s3, s2
	mv          a0, s2
	call        IRIsLoad

	// *** Basic block 10

	mv          s6, a0
	bnez        a0, .FindVariableReference_label_67

	// *** Basic block 11

	mv          a0, s2
	call        IRIsStore

	// *** Basic block 12

	mv          s6, a0

	// *** Basic block 13

.FindVariableReference_label_67:
	bnez        s6, .FindVariableReference_label_72

	// *** Basic block 14

	addi        t0, s5, -40
	seqz        s6, t0

	// *** Basic block 15

.FindVariableReference_label_72:
	beqz        s6, .FindVariableReference_label_78

	// *** Basic block 16

	ld          t0, 24(s2)
	ld          s2, 0(t0)
	j           .FindVariableReference_label_25

	// *** Basic block 17

.FindVariableReference_label_78:
	j           .FindVariableReference_label_80

	// *** Basic block 18

.FindVariableReference_label_80:
	mv          a0, x0
	j           .FindVariableReference_label_39
.func_end_FindVariableReference:
	.size FindVariableReference, .func_end_FindVariableReference-FindVariableReference

	.local  RenameVariables
	.type RenameVariables, @function

RenameVariables:

	// *** Basic block 0

	.global printf
	.local FindVariableStack
	.global GeneratorGetVariable
	.global VectorPush
	.global IRPrev
	.global IRIsVarDef
	.local RenameTopVariable
	.global IRIsStore
	.global abort
	.local FindVariableReference
	.global NewIRSSAVar
	.global BasicBlockInsertVar
	.global IRReplaceInput
	.global IRIsVarRef
	.global IRIsLoad
	.local GetTopVariable
	.global IRNext
	.global VectorGet
	.local RenameVariables
	.global VectorPop
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -16(s0)
	// Spilled register region: 16 bytes at -32(s0) to -16(s0)
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
	mv          s2, a2
	mv          s3, a0
	sd          s3, -32(s0)	// Spilled @59
	ld          t0, 0(s1)
	li          t1, 18		// 0x12 ASCII \x12
	bne         t0, t1, .RenameVariables_label_75

	// *** Basic block 1

	ld          s4, 64(s1)
	lla         a0, .str.4
	call        printf

	// *** Basic block 2

.RenameVariables_label_75:
	mv          s4, x0
	addi        t0, s1, 160
	ld          s5, 8(t0)
	bge         x0, s5, .RenameVariables_label_126

	// *** Basic block 3

	ld          s6, 160(s1)

	// *** Basic block 4

.RenameVariables_label_84:
	slli        t0, s4, 4
	add         t0, s6, t0
	ld          s6, 0(t0)
	mv          a1, s6
	mv          a0, s2
	call        FindVariableStack

	// *** Basic block 5

	mv          s7, a0
	sd          s7, -24(s0)	// Spilled @94
	ld          s8, 8(s7)
	bnez        s8, .RenameVariables_label_108

	// *** Basic block 6

	mv          a1, s6
	mv          a0, s3
	call        GeneratorGetVariable

	// *** Basic block 7

	mv          s6, a0
	sd          s6, -24(s0)	// Spilled @105
	j           .RenameVariables_label_116

	// *** Basic block 8

.RenameVariables_label_108:
	ld          t0, 0(s7)
	addi        t1, s8, -1
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          s6, 0(t0)

	// *** Basic block 9

.RenameVariables_label_116:
	mv          a1, s6
	mv          a0, s7
	call        VectorPush

	// *** Basic block 10

.RenameVariables_label_122:
	addi        s4, s4, 1
	bge         s4, s5, .RenameVariables_label_84

	// *** Basic block 11

.RenameVariables_label_126:
	ld          s8, 56(s1)
	sub         t0, s8, x0
	snez        s9, t0
	beq         s8, x0, .RenameVariables_label_141

	// *** Basic block 12

	mv          a0, s8
	call        IRPrev

	// *** Basic block 13

	ld          t0, 64(s1)
	sub         t0, a0, t0
	snez        s9, t0

	// *** Basic block 14

.RenameVariables_label_141:
	beqz        s9, .RenameVariables_label_345

	// *** Basic block 15

.RenameVariables_label_143:
	mv          a0, s8
	call        IRIsVarDef

	// *** Basic block 16

	beqz        a0, .RenameVariables_label_254

	// *** Basic block 17

	lw          t0, 20(s8)
	li          t1, 122		// 0x7a ASCII 'z'
	bne         t0, t1, .RenameVariables_label_164

	// *** Basic block 18

	ld          a1, 120(s8)
	mv          a2, s8
	mv          a0, s2
	call        RenameTopVariable

	// *** Basic block 19

	j           .RenameVariables_label_252

	// *** Basic block 20

.RenameVariables_label_164:
	mv          a0, s8
	call        IRIsStore

	// *** Basic block 21

	beqz        a0, .RenameVariables_label_171

	// *** Basic block 22

	j           .RenameVariables_label_187

	// *** Basic block 23

.RenameVariables_label_171:
	lla         a0, .str.5
	lla         a1, .str.6
	lla         a3, .str.7
	li          t0, 121		// 0x79 ASCII 'y'
	mv          a2, t0
	call        printf

	// *** Basic block 24

	call        abort

	// *** Basic block 25

.RenameVariables_label_187:
	ld          s9, 120(s8)
	mv          a1, s9
	mv          a0, s8
	call        FindVariableReference

	// *** Basic block 26

	mv          s10, a0
	beq         s10, x0, .RenameVariables_label_201

	// *** Basic block 27

	j           .RenameVariables_label_216

	// *** Basic block 28

.RenameVariables_label_201:
	lla         a0, .str.8
	lla         a1, .str.9
	lla         a3, .str.10
	li          t0, 123		// 0x7b ASCII '{'
	mv          a2, t0
	call        printf

	// *** Basic block 29

	call        abort

	// *** Basic block 30

.RenameVariables_label_216:
	lw          t0, 20(s10)
	li          t1, 102		// 0x66 ASCII 'f'
	beq         t0, t1, .RenameVariables_label_251

	// *** Basic block 31

	mv          a0, s9
	call        NewIRSSAVar

	// *** Basic block 32

	mv          s11, a0
	sd          s11, -32(s0)	// Spilled @227
	ld          a1, 72(s10)
	mv          a2, s11
	mv          a0, s3
	call        BasicBlockInsertVar

	// *** Basic block 33

	mv          a2, s11
	mv          a1, s9
	mv          a0, s2
	call        RenameTopVariable

	// *** Basic block 34

	mv          a2, s11
	mv          a1, x0
	mv          a0, s10
	call        IRReplaceInput

	// *** Basic block 35

.RenameVariables_label_251:

	// *** Basic block 36

.RenameVariables_label_252:
	j           .RenameVariables_label_329

	// *** Basic block 37

.RenameVariables_label_254:
	mv          a0, s8
	call        IRIsVarRef

	// *** Basic block 38

	beqz        a0, .RenameVariables_label_328

	// *** Basic block 39

	mv          a0, s8
	call        IRIsLoad

	// *** Basic block 40

	beqz        a0, .RenameVariables_label_265

	// *** Basic block 41

	j           .RenameVariables_label_280

	// *** Basic block 42

.RenameVariables_label_265:
	lla         a0, .str.11
	lla         a1, .str.12
	lla         a3, .str.13
	li          t0, 144		// 0x90 ASCII \x90
	mv          a2, t0
	call        printf

	// *** Basic block 43

	call        abort

	// *** Basic block 44

.RenameVariables_label_280:
	ld          s9, 120(s8)
	mv          a1, s9
	mv          a0, s8
	call        FindVariableReference

	// *** Basic block 45

	mv          s6, a0
	beq         s6, x0, .RenameVariables_label_294

	// *** Basic block 46

	j           .RenameVariables_label_309

	// *** Basic block 47

.RenameVariables_label_294:
	lla         a0, .str.14
	lla         a1, .str.15
	lla         a3, .str.16
	li          t0, 146		// 0x92 ASCII \x92
	mv          a2, t0
	call        printf

	// *** Basic block 48

	call        abort

	// *** Basic block 49

.RenameVariables_label_309:
	mv          a1, s9
	mv          a0, s2
	call        GetTopVariable

	// *** Basic block 50

	mv          s9, a0
	beq         s9, x0, .RenameVariables_label_327

	// *** Basic block 51

	mv          a2, s9
	mv          a1, x0
	mv          a0, s6
	call        IRReplaceInput

	// *** Basic block 52

.RenameVariables_label_327:

	// *** Basic block 53

.RenameVariables_label_328:

	// *** Basic block 54

.RenameVariables_label_329:
	mv          a0, s8
	call        IRNext
	sd          a0, -24(s0)	// Spilled @332

	// *** Basic block 55

	mv          s8, a0
	sub         t0, s8, x0
	snez        a0, t0
	beq         s8, x0, .RenameVariables_label_343

	// *** Basic block 56

	mv          a0, s8
	call        IRPrev

	// *** Basic block 57

	sub         t0, a0, s4
	snez        a0, t0

	// *** Basic block 58

.RenameVariables_label_343:
	bnez        a0, .RenameVariables_label_143

	// *** Basic block 59

.RenameVariables_label_345:
	mv          s7, x0
	addi        t0, s1, 96
	ld          s11, 8(t0)
	sd          s11, -24(s0)	// Spilled @350
	bge         x0, s11, .RenameVariables_label_375

	// *** Basic block 60

	ld          s11, 96(s1)

	// *** Basic block 61

.RenameVariables_label_354:
	addi        a0, s3, 168
	slli        t0, s7, 3
	add         t0, s11, t0
	ld          a1, 0(t0)
	call        VectorGet

	// *** Basic block 62

	mv          s11, a0
	sd          s11, -32(s0)	// Spilled @362
	mv          a2, s2
	mv          a1, s11
	mv          a0, s3
	call        RenameVariables

	// *** Basic block 63

.RenameVariables_label_371:
	addi        s7, s7, 1
	ld          t0, -24(s0)	// Spilled @350
	bge         s7, t0, .RenameVariables_label_354

	// *** Basic block 64

.RenameVariables_label_375:
	mv          s3, x0
	bge         x0, s5, .RenameVariables_label_415

	// *** Basic block 65

.RenameVariables_label_380:
	ld          t0, 160(s1)
	slli        s5, s3, 4
	add         t0, t0, s5
	ld          s11, 0(t0)
	mv          a1, s11
	mv          a0, s2
	call        FindVariableStack
	sd          a0, -32(s0)	// Spilled @391

	// *** Basic block 66

	mv          s11, a0
	addi        a0, s1, 160
	ld          t0, 160(s1)
	add         t0, t0, s5
	ld          t1, 0(s11)
	ld          t2, 8(s11)
	addi        t2, t2, -1
	slli        t2, t2, 3
	add         t1, t1, t2
	ld          t1, 0(t1)
	sd          t1, 8(t0)
	mv          a0, s11
	call        VectorPop

	// *** Basic block 67

.RenameVariables_label_409:
	addi        s3, s3, 1
	ld          t0, 8(a0)
	bge         s3, t0, .RenameVariables_label_380

	// *** Basic block 68

.RenameVariables_label_415:
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
.func_end_RenameVariables:
	.size RenameVariables, .func_end_RenameVariables-RenameVariables

	.local  InsertPhiNodes
	.type InsertPhiNodes, @function

InsertPhiNodes:

	// *** Basic block 0

	.global BitSetIteratorStart
	.global BitSetIteratorDone
	.global VectorGet
	.global BasicBlockInsertPhi
	.global MapInsert
	.global BitSetIteratorNext
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
	sd s8, 0(sp)
	// End of stack frame
	mv          s1, a0

	// *** Basic block 1

.InsertPhiNodes_label_21:
	mv          s2, x0
	mv          s3, x0
	addi        t0, s1, 168
	ld          t0, 8(t0)
	bge         x0, t0, .InsertPhiNodes_label_143

	// *** Basic block 2

.InsertPhiNodes_label_31:
	ld          t0, 168(s1)
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	mv          s5, x0
	addi        t0, s4, 160
	ld          t0, 8(t0)
	bge         x0, t0, .InsertPhiNodes_label_135

	// *** Basic block 3

.InsertPhiNodes_label_46:
	ld          t0, 160(s4)
	slli        t1, s5, 4
	add         t0, t0, t1
	ld          s6, 0(t0)
	addi        a0, s0, -64
	addi        a1, s4, 120
	call        BitSetIteratorStart

	// *** Basic block 4

	addi        a0, s0, -64
	call        BitSetIteratorDone

	// *** Basic block 5

	not         t0, a0
	beqz        t0, .InsertPhiNodes_label_127

	// *** Basic block 6

.InsertPhiNodes_label_67:
	addi        a0, s1, 168
	addi        s7, s0, -64
	ld          t0, 8(s7)
	slli        t0, t0, 5
	ld          t1, 16(s7)
	add         s7, t0, t1

	// *** Basic block 7

.InsertPhiNodes_label_79:
	mv          a1, s7
	call        VectorGet

	// *** Basic block 8

	mv          s7, a0
	mv          a2, s6
	mv          a1, s7
	mv          a0, s1
	call        BasicBlockInsertPhi

	// *** Basic block 9

	mv          s8, a0
	beqz        s8, .InsertPhiNodes_label_117

	// *** Basic block 10

	j           .InsertPhiNodes_label_98

	// *** Basic block 11

.InsertPhiNodes_label_98:
	sd          s6, -40(s0)
	addi        t0, s0, -40
	sd          x0, 8(t0)
	addi        a0, s7, 160
	addi        sp, sp, -16
	ld          t0, -40(s0)
	sd          t0, 0(sp)
	ld          t0, -32(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	call        MapInsert

	// *** Basic block 13

.InsertPhiNodes_label_117:
	or          s2, s2, s8
	addi        a0, s0, -64
	call        BitSetIteratorNext

	// *** Basic block 14

	addi        a0, s0, -64
	call        BitSetIteratorDone

	// *** Basic block 15

	not         t0, a0
	bnez        t0, .InsertPhiNodes_label_67

	// *** Basic block 16

.InsertPhiNodes_label_127:

	// *** Basic block 17

.InsertPhiNodes_label_128:
	addi        s5, s5, 1
	addi        t0, s4, 160
	ld          t0, 8(t0)
	bge         s5, t0, .InsertPhiNodes_label_46

	// *** Basic block 18

.InsertPhiNodes_label_135:

	// *** Basic block 19

.InsertPhiNodes_label_136:
	addi        s3, s3, 1
	addi        t0, s1, 168
	ld          t0, 8(t0)
	bge         s3, t0, .InsertPhiNodes_label_31

	// *** Basic block 20

.InsertPhiNodes_label_143:

	// *** Basic block 21

.InsertPhiNodes_label_144:
	bnez        s2, .InsertPhiNodes_label_21

	// *** Basic block 22

.InsertPhiNodes_label_146:
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
.func_end_InsertPhiNodes:
	.size InsertPhiNodes, .func_end_InsertPhiNodes-InsertPhiNodes

	.local  FindSSAVar
	.type FindSSAVar, @function

FindSSAVar:

	// *** Basic block 0

	.global MapFindPointerKey
	.global BitSetContains
	.global BitSetInsert
	.global VectorGet
	.local FindSSAVar
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
	mv          s1, a1
	mv          s2, a2
	mv          s3, a3
	mv          s4, a0
	addi        a0, s1, 160
	mv          a1, s2
	call        MapFindPointerKey

	// *** Basic block 1

	mv          s5, a0
	beq         s5, x0, .FindSSAVar_label_41

	// *** Basic block 2

	mv          a0, s5

	// *** Basic block 3

.FindSSAVar_label_38:
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

	// *** Basic block 4

.FindSSAVar_label_41:
	mv          s6, x0
	addi        t0, s1, 8
	ld          s7, 8(t0)
	bge         x0, s7, .FindSSAVar_label_96

	// *** Basic block 5

	ld          s8, 8(s1)

	// *** Basic block 6

.FindSSAVar_label_50:
	slli        t0, s6, 3
	add         t0, s8, t0
	ld          s1, 0(t0)
	mv          a1, s1
	mv          a0, s3
	call        BitSetContains

	// *** Basic block 7

	bnez        a0, .FindSSAVar_label_92

	// *** Basic block 8

.FindSSAVar_label_62:
	mv          a1, s1
	mv          a0, s3
	call        BitSetInsert

	// *** Basic block 9

	addi        a0, s4, 168
	mv          a1, s1
	call        VectorGet

	// *** Basic block 10

	mv          s1, a0
	mv          a3, s3
	mv          a2, s2
	mv          a1, s1
	mv          a0, s4
	call        FindSSAVar

	// *** Basic block 11

	mv          s5, a0
	beq         s5, x0, .FindSSAVar_label_91

	// *** Basic block 12

	mv          a0, s5
	j           .FindSSAVar_label_38

	// *** Basic block 13

.FindSSAVar_label_91:

	// *** Basic block 14

.FindSSAVar_label_92:
	addi        s6, s6, 1
	bge         s6, s7, .FindSSAVar_label_50

	// *** Basic block 15

.FindSSAVar_label_96:
	mv          a0, x0
	j           .FindSSAVar_label_38
.func_end_FindSSAVar:
	.size FindSSAVar, .func_end_FindSSAVar-FindSSAVar

	.local  AddPhiInputs
	.type AddPhiInputs, @function

AddPhiInputs:

	// *** Basic block 0

	.global IRNext
	.global VectorGet
	.local FindSSAVar
	.global IRAddInput
	.global BitSetDestruct
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -32(s0)
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
	ld          s3, 56(s1)
	sd          x0, -32(s0)
	sd          x0, -24(s0)
	sd          x0, -32(s0)
	sub         t1, s3, x0
	snez        t0, t1
	beq         s3, x0, .AddPhiInputs_label_47

	// *** Basic block 1

	addi        t1, s1, 8
	ld          t1, 8(t1)
	ld          t2, 8(s1)
	lw          t3, 20(s3)
	addi        t3, t3, -101
	seqz        t0, t3

	// *** Basic block 2

.AddPhiInputs_label_47:
	beqz        t0, .AddPhiInputs_label_64

	// *** Basic block 3

.AddPhiInputs_label_49:
	mv          a0, s3
	call        IRNext

	// *** Basic block 4

	mv          s3, a0
	sub         t1, s3, x0
	snez        t0, t1
	beq         s3, x0, .AddPhiInputs_label_62

	// *** Basic block 5

	lw          t1, 20(s3)
	addi        t1, t1, -101
	seqz        t0, t1

	// *** Basic block 6

.AddPhiInputs_label_62:
	bnez        t0, .AddPhiInputs_label_49

	// *** Basic block 7

.AddPhiInputs_label_64:
	sub         t1, s3, x0
	snez        t0, t1
	beq         s3, x0, .AddPhiInputs_label_74

	// *** Basic block 8

	lw          t1, 20(s3)
	addi        t1, t1, -122
	seqz        t0, t1

	// *** Basic block 9

.AddPhiInputs_label_74:
	beqz        t0, .AddPhiInputs_label_139

	// *** Basic block 10

.AddPhiInputs_label_76:
	mv          s4, s3
	ld          s5, 136(s4)
	mv          s6, x0
	bge         x0, t1, .AddPhiInputs_label_124

	// *** Basic block 11

.AddPhiInputs_label_85:
	addi        a0, s2, 168
	slli        t0, s6, 3
	add         t0, t2, t0
	ld          a1, 0(t0)
	call        VectorGet

	// *** Basic block 12

	mv          s7, a0
	addi        a3, s0, -32
	mv          a2, s5
	mv          a1, s7
	mv          a0, s2
	call        FindSSAVar

	// *** Basic block 13

	mv          s5, a0
	beq         s5, x0, .AddPhiInputs_label_119

	// *** Basic block 14

	mv          a2, x0
	mv          a1, s5
	mv          a0, s4
	call        IRAddInput

	// *** Basic block 15

.AddPhiInputs_label_119:

	// *** Basic block 16

.AddPhiInputs_label_120:
	addi        s6, s6, 1
	bge         s6, t1, .AddPhiInputs_label_85

	// *** Basic block 17

.AddPhiInputs_label_124:
	mv          a0, s3
	call        IRNext

	// *** Basic block 18

	mv          s3, a0
	sub         t1, s3, x0
	snez        t0, t1
	beq         s3, x0, .AddPhiInputs_label_137

	// *** Basic block 19

	lw          t1, 20(s3)
	addi        t1, t1, -122
	seqz        t0, t1

	// *** Basic block 20

.AddPhiInputs_label_137:
	bnez        t0, .AddPhiInputs_label_76

	// *** Basic block 21

.AddPhiInputs_label_139:
	addi        a0, s0, -32
	call        BitSetDestruct

	// *** Basic block 22

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
.func_end_AddPhiInputs:
	.size AddPhiInputs, .func_end_AddPhiInputs-AddPhiInputs

	.local  SymbolCompare
	.type SymbolCompare, @function

SymbolCompare:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	ld          t2, 0(t0)
	ld          t3, 0(t1)
	sub         t2, t2, t3
	div         t3, t2, x0
	sext.w      a0, t3

	// *** Basic block 1

.SymbolCompare_label_22:
	ret         
.func_end_SymbolCompare:
	.size SymbolCompare, .func_end_SymbolCompare-SymbolCompare

	.global GeneratorRenameVariables
	.type GeneratorRenameVariables, @function

GeneratorRenameVariables:

	// *** Basic block 0

	.global MapInit
	.local SymbolCompare
	.local RenameVariables
	.global VectorDelete
	.global MapDestruct
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -48(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	addi        a0, s0, -48
	la          a1, SymbolCompare
	call        MapInit

	// *** Basic block 1

	ld          a1, 192(s1)
	addi        a2, s0, -48
	mv          a0, s1
	call        RenameVariables

	// *** Basic block 2

	ld          s2, -48(s0)
	mv          s3, x0
	addi        t0, s0, -48
	ld          s4, 8(t0)
	bge         x0, s4, .GeneratorRenameVariables_label_54

	// *** Basic block 3

.GeneratorRenameVariables_label_42:
	slli        t0, s3, 4
	add         t0, s2, t0
	ld          a0, 8(t0)
	call        VectorDelete

	// *** Basic block 4

.GeneratorRenameVariables_label_50:
	addi        s3, s3, 1
	bge         s3, s4, .GeneratorRenameVariables_label_42

	// *** Basic block 5

.GeneratorRenameVariables_label_54:
	addi        a0, s0, -48
	call        MapDestruct

	// *** Basic block 6

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GeneratorRenameVariables:
	.size GeneratorRenameVariables, .func_end_GeneratorRenameVariables-GeneratorRenameVariables

	.global GeneratorConvertToSSA
	.type GeneratorConvertToSSA, @function

GeneratorConvertToSSA:

	// *** Basic block 0

	.local InsertPhiNodes
	.global GeneratorRenameVariables
	.local AddPhiInputs
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
	call        InsertPhiNodes

	// *** Basic block 1

	mv          a0, s1
	call        GeneratorRenameVariables

	// *** Basic block 2

	mv          s2, x0
	addi        t0, s1, 168
	ld          s3, 8(t0)
	bge         x0, s3, .GeneratorConvertToSSA_label_42

	// *** Basic block 3

	ld          s4, 168(s1)

	// *** Basic block 4

.GeneratorConvertToSSA_label_28:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        AddPhiInputs

	// *** Basic block 5

.GeneratorConvertToSSA_label_38:
	addi        s2, s2, 1
	bge         s2, s3, .GeneratorConvertToSSA_label_28

	// *** Basic block 6

.GeneratorConvertToSSA_label_42:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GeneratorConvertToSSA:
	.size GeneratorConvertToSSA, .func_end_GeneratorConvertToSSA-GeneratorConvertToSSA

	.local  RemovePhiNodes
	.type RemovePhiNodes, @function

RemovePhiNodes:

	// *** Basic block 0

	.global IRNext
	.global GeneratorGetVariable
	.global printf
	.global abort
	.global IRReplaceInput
	.global BasicBlockRemoveInstruction
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -16(s0)
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
	ld          s3, 56(s1)
	sub         t1, s3, x0
	snez        t0, t1
	beq         s3, x0, .RemovePhiNodes_label_43

	// *** Basic block 1

	lw          t1, 20(s3)
	addi        t1, t1, -101
	seqz        t0, t1

	// *** Basic block 2

.RemovePhiNodes_label_43:
	beqz        t0, .RemovePhiNodes_label_60

	// *** Basic block 3

.RemovePhiNodes_label_45:
	mv          a0, s3
	call        IRNext

	// *** Basic block 4

	mv          s3, a0
	sub         t1, s3, x0
	snez        t0, t1
	beq         s3, x0, .RemovePhiNodes_label_58

	// *** Basic block 5

	lw          t1, 20(s3)
	addi        t1, t1, -101
	seqz        t0, t1

	// *** Basic block 6

.RemovePhiNodes_label_58:
	bnez        t0, .RemovePhiNodes_label_45

	// *** Basic block 7

.RemovePhiNodes_label_60:
	sub         t1, s3, x0
	snez        t0, t1
	beq         s3, x0, .RemovePhiNodes_label_70

	// *** Basic block 8

	lw          t1, 20(s3)
	addi        t1, t1, -122
	seqz        t0, t1

	// *** Basic block 9

.RemovePhiNodes_label_70:
	beqz        t0, .RemovePhiNodes_label_179

	// *** Basic block 10

.RemovePhiNodes_label_72:
	mv          a0, s3
	call        IRNext

	// *** Basic block 11

	mv          s4, a0
	ld          a1, 120(s3)
	mv          a0, s2
	call        GeneratorGetVariable

	// *** Basic block 12

	mv          s5, a0
	beq         s5, x0, .RemovePhiNodes_label_90

	// *** Basic block 13

	j           .RemovePhiNodes_label_107

	// *** Basic block 14

.RemovePhiNodes_label_90:
	lla         a0, .str.17
	lla         a1, .str.18
	lla         a3, .str.19
	li          t0, 324		// 0x144
	mv          a2, t0
	call        printf

	// *** Basic block 15

	call        abort

	// *** Basic block 16

.RemovePhiNodes_label_107:
	mv          s6, x0
	addi        t0, s3, 48
	ld          s7, 8(t0)
	bge         x0, s7, .RemovePhiNodes_label_160

	// *** Basic block 17

	ld          t0, 48(s3)

	// *** Basic block 18

.RemovePhiNodes_label_116:
	slli        t1, s6, 3
	add         t0, t0, t1
	ld          s8, 0(t0)
	mv          s9, x0
	mv          s10, x0
	addi        t0, s8, 24
	ld          s11, 8(t0)
	bge         x0, s11, .RemovePhiNodes_label_153

	// *** Basic block 19

	ld          t0, 24(s8)

	// *** Basic block 20

.RemovePhiNodes_label_132:
	slli        t1, s10, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	bne         t0, s3, .RemovePhiNodes_label_148

	// *** Basic block 21

	mv          a2, s5
	mv          a1, s10
	mv          a0, s8
	call        IRReplaceInput

	// *** Basic block 22

	li          s9, 1		// 0x1 ASCII \x1
	j           .RemovePhiNodes_label_153

	// *** Basic block 23

.RemovePhiNodes_label_148:

	// *** Basic block 24

.RemovePhiNodes_label_149:
	addi        s10, s10, 1
	bge         s10, s11, .RemovePhiNodes_label_132

	// *** Basic block 25

.RemovePhiNodes_label_153:
	not         t0, s9
	beqz        t0, .RemovePhiNodes_label_157

	// *** Basic block 26

	addi        s6, s6, 1

	// *** Basic block 27

.RemovePhiNodes_label_157:
	blt         s6, s7, .RemovePhiNodes_label_116

	// *** Basic block 28

.RemovePhiNodes_label_160:
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        BasicBlockRemoveInstruction

	// *** Basic block 29

	mv          s3, s4
	sub         t1, s3, x0
	snez        t0, t1
	beq         s3, x0, .RemovePhiNodes_label_177

	// *** Basic block 30

	lw          t1, 20(s3)
	addi        t1, t1, -122
	seqz        t0, t1

	// *** Basic block 31

.RemovePhiNodes_label_177:
	bnez        t0, .RemovePhiNodes_label_72

	// *** Basic block 32

.RemovePhiNodes_label_179:
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
.func_end_RemovePhiNodes:
	.size RemovePhiNodes, .func_end_RemovePhiNodes-RemovePhiNodes

	.local  RemoveSSAVariables
	.type RemoveSSAVariables, @function

RemoveSSAVariables:

	// *** Basic block 0

	.global IRPrev
	.global IRNext
	.global GeneratorGetVariable
	.global IRReplaceInput
	.global BasicBlockRemoveInstruction
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -16(s0)
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
	ld          s3, 56(s1)
	sub         t0, s3, x0
	snez        s4, t0
	beq         s3, x0, .RemoveSSAVariables_label_42

	// *** Basic block 1

	ld          s5, 64(s1)
	mv          a0, s3
	call        IRPrev

	// *** Basic block 2

	ld          t0, 64(s1)
	sub         t0, a0, t0
	snez        s4, t0

	// *** Basic block 3

.RemoveSSAVariables_label_42:
	beqz        s4, .RemoveSSAVariables_label_141

	// *** Basic block 4

.RemoveSSAVariables_label_44:
	mv          a0, s3
	call        IRNext

	// *** Basic block 5

	mv          s4, a0
	lw          t0, 20(s3)
	li          t1, 101		// 0x65 ASCII 'e'
	bne         t0, t1, .RemoveSSAVariables_label_128

	// *** Basic block 6

	mv          s5, s3
	ld          a1, 136(s5)
	mv          a0, s2
	call        GeneratorGetVariable

	// *** Basic block 7

	mv          s6, a0
	mv          s7, x0
	addi        t0, s3, 48
	ld          s8, 8(t0)
	bge         x0, s8, .RemoveSSAVariables_label_120

	// *** Basic block 8

	ld          t0, 48(s3)

	// *** Basic block 9

.RemoveSSAVariables_label_75:
	slli        t1, s7, 3
	add         t0, t0, t1
	ld          s5, 0(t0)
	mv          s9, x0
	mv          s10, x0
	addi        t0, s5, 24
	ld          s11, 8(t0)
	bge         x0, s11, .RemoveSSAVariables_label_113

	// *** Basic block 10

	ld          t0, 24(s5)

	// *** Basic block 11

.RemoveSSAVariables_label_91:
	slli        t1, s10, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	bne         t0, s3, .RemoveSSAVariables_label_108

	// *** Basic block 12

	mv          a2, s6
	mv          a1, s10
	mv          a0, s5
	call        IRReplaceInput

	// *** Basic block 13

	li          s9, 1		// 0x1 ASCII \x1
	j           .RemoveSSAVariables_label_113

	// *** Basic block 14

.RemoveSSAVariables_label_108:

	// *** Basic block 15

.RemoveSSAVariables_label_109:
	addi        s10, s10, 1
	bge         s10, s11, .RemoveSSAVariables_label_91

	// *** Basic block 16

.RemoveSSAVariables_label_113:
	not         t0, s9
	beqz        t0, .RemoveSSAVariables_label_117

	// *** Basic block 17

	addi        s7, s7, 1

	// *** Basic block 18

.RemoveSSAVariables_label_117:
	blt         s7, s8, .RemoveSSAVariables_label_75

	// *** Basic block 19

.RemoveSSAVariables_label_120:
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        BasicBlockRemoveInstruction

	// *** Basic block 20

.RemoveSSAVariables_label_128:
	mv          s3, s4
	sub         t0, s3, x0
	snez        s8, t0
	beq         s3, x0, .RemoveSSAVariables_label_139

	// *** Basic block 21

	mv          a0, s3
	call        IRPrev

	// *** Basic block 22

	sub         t0, a0, s5
	snez        s8, t0

	// *** Basic block 23

.RemoveSSAVariables_label_139:
	bnez        s8, .RemoveSSAVariables_label_44

	// *** Basic block 24

.RemoveSSAVariables_label_141:
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
.func_end_RemoveSSAVariables:
	.size RemoveSSAVariables, .func_end_RemoveSSAVariables-RemoveSSAVariables

	.global GeneratorRemoveSSA
	.type GeneratorRemoveSSA, @function

GeneratorRemoveSSA:

	// *** Basic block 0

	.local RemovePhiNodes
	.local RemoveSSAVariables
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
	mv          s2, x0
	addi        t0, s1, 168
	ld          s3, 8(t0)
	bge         x0, s3, .GeneratorRemoveSSA_label_36

	// *** Basic block 1

	ld          s4, 168(s1)
	ld          s5, 168(s1)

	// *** Basic block 2

.GeneratorRemoveSSA_label_22:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        RemovePhiNodes

	// *** Basic block 3

.GeneratorRemoveSSA_label_32:
	addi        s2, s2, 1
	bge         s2, s3, .GeneratorRemoveSSA_label_22

	// *** Basic block 4

.GeneratorRemoveSSA_label_36:
	mv          s4, x0
	bge         x0, s3, .GeneratorRemoveSSA_label_53

	// *** Basic block 5

.GeneratorRemoveSSA_label_41:
	slli        t0, s4, 3
	add         t0, s5, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        RemoveSSAVariables

	// *** Basic block 6

.GeneratorRemoveSSA_label_49:
	addi        s4, s4, 1
	bge         s4, s3, .GeneratorRemoveSSA_label_41

	// *** Basic block 7

.GeneratorRemoveSSA_label_53:
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
.func_end_GeneratorRemoveSSA:
	.size GeneratorRemoveSSA, .func_end_GeneratorRemoveSSA-GeneratorRemoveSSA

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.1, @object
	.size .str.1, 30

.str.2:
	.asciz "ssa.c"
	.type .str.2, @object
	.size .str.2, 6

.str.3:
	.asciz "versions != NULL"
	.type .str.3, @object
	.size .str.3, 17

.str.4:
	.asciz "(null)"
	.type .str.4, @object
	.size .str.4, 1

.str.5:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.5, @object
	.size .str.5, 30

.str.6:
	.asciz "ssa.c"
	.type .str.6, @object
	.size .str.6, 6

.str.7:
	.asciz "IRIsStore(inst)"
	.type .str.7, @object
	.size .str.7, 16

.str.8:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.8, @object
	.size .str.8, 30

.str.9:
	.asciz "ssa.c"
	.type .str.9, @object
	.size .str.9, 6

.str.10:
	.asciz "ref != NULL"
	.type .str.10, @object
	.size .str.10, 12

.str.11:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.11, @object
	.size .str.11, 30

.str.12:
	.asciz "ssa.c"
	.type .str.12, @object
	.size .str.12, 6

.str.13:
	.asciz "IRIsLoad(inst)"
	.type .str.13, @object
	.size .str.13, 15

.str.14:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.14, @object
	.size .str.14, 30

.str.15:
	.asciz "ssa.c"
	.type .str.15, @object
	.size .str.15, 6

.str.16:
	.asciz "ref != NULL"
	.type .str.16, @object
	.size .str.16, 12

.str.17:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.17, @object
	.size .str.17, 30

.str.18:
	.asciz "ssa.c"
	.type .str.18, @object
	.size .str.18, 6

.str.19:
	.asciz "input != NULL"
	.type .str.19, @object
	.size .str.19, 14

