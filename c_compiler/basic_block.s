	.file   "basic_block.c"
	.text
	.option pic
.PCbegin:
	.local  CompareVariable
	.type CompareVariable, @function

CompareVariable:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 0(a0)
	ld          t1, 0(a1)
	sub         t0, t0, t1
	li          t1, 136		// 0x88 ASCII \x88
	div         t2, t0, t1
	sext.w      a0, t2

	// *** Basic block 1

.CompareVariable_label_21:
	ret         
.func_end_CompareVariable:
	.size CompareVariable, .func_end_CompareVariable-CompareVariable

	.global NewBasicBlock
	.type NewBasicBlock, @function

NewBasicBlock:

	// *** Basic block 0

	.global malloc
	.global VectorInit
	.global BitSetInit
	.global MapInit
	.local CompareVariable
	.global SetInitForPointers
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
	li          a0, 248		// 0xf8 ASCII \xf8
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	sd          s1, 0(s2)
	sd          x0, 56(s2)
	sd          x0, 64(s2)
	addi        a0, s2, 8
	call        VectorInit

	// *** Basic block 2

	addi        a0, s2, 32
	call        VectorInit

	// *** Basic block 3

	addi        a0, s2, 72
	call        BitSetInit

	// *** Basic block 4

	sd          x0, 88(s2)
	addi        a0, s2, 96
	call        VectorInit

	// *** Basic block 5

	addi        a0, s2, 120
	call        BitSetInit

	// *** Basic block 6

	addi        a0, s2, 160
	la          t0, CompareVariable
	mv          a1, t0
	call        MapInit

	// *** Basic block 7

	addi        a0, s2, 192
	call        SetInitForPointers

	// *** Basic block 8

	sd          x0, 136(s2)
	addi        a0, s2, 144
	call        BitSetInit

	// *** Basic block 9

	sd          x0, 224(s2)
	sw          x0, 232(s2)
	sb          x0, 236(s2)
	sb          x0, 237(s2)
	sb          x0, 238(s2)
	sw          x0, 240(s2)
	mv          a0, s2

	// *** Basic block 10

.NewBasicBlock_label_89:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewBasicBlock:
	.size NewBasicBlock, .func_end_NewBasicBlock-NewBasicBlock

	.global BasicBlockDelete
	.type BasicBlockDelete, @function

BasicBlockDelete:

	// *** Basic block 0

	.global VectorDestruct
	.global BitSetDestruct
	.global MapDestruct
	.global SetDestruct
	.global free
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
	addi        a0, s1, 8
	call        VectorDestruct

	// *** Basic block 1

	addi        a0, s1, 32
	call        VectorDestruct

	// *** Basic block 2

	addi        a0, s1, 72
	call        BitSetDestruct

	// *** Basic block 3

	addi        a0, s1, 120
	call        BitSetDestruct

	// *** Basic block 4

	addi        a0, s1, 144
	call        BitSetDestruct

	// *** Basic block 5

	addi        a0, s1, 96
	call        VectorDestruct

	// *** Basic block 6

	addi        a0, s1, 160
	call        MapDestruct

	// *** Basic block 7

	addi        a0, s1, 192
	call        SetDestruct

	// *** Basic block 8

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_BasicBlockDelete:
	.size BasicBlockDelete, .func_end_BasicBlockDelete-BasicBlockDelete

	.global BasicBlockAddInEdge
	.type BasicBlockAddInEdge, @function

BasicBlockAddInEdge:

	// *** Basic block 0

	.global VectorAppend
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	addi        a0, t0, 8
	ld          a1, 0(t1)
	j           VectorAppend
.func_end_BasicBlockAddInEdge:
	.size BasicBlockAddInEdge, .func_end_BasicBlockAddInEdge-BasicBlockAddInEdge

	.global BasicBlockAddEdge
	.type BasicBlockAddEdge, @function

BasicBlockAddEdge:

	// *** Basic block 0

	.global VectorAppend
	.global BasicBlockAddInEdge
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
	addi        a0, s1, 32
	ld          a1, 0(s2)
	call        VectorAppend

	// *** Basic block 1

	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           BasicBlockAddInEdge
.func_end_BasicBlockAddEdge:
	.size BasicBlockAddEdge, .func_end_BasicBlockAddEdge-BasicBlockAddEdge

	.global BasicBlockAddBackEdge
	.type BasicBlockAddBackEdge, @function

BasicBlockAddBackEdge:

	// *** Basic block 0

	.global BitSetInsert
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	addi        a0, t0, 144
	ld          a1, 0(t1)
	j           BitSetInsert
.func_end_BasicBlockAddBackEdge:
	.size BasicBlockAddBackEdge, .func_end_BasicBlockAddBackEdge-BasicBlockAddBackEdge

	.global BasicBlockCalculateDominators
	.type BasicBlockCalculateDominators, @function

BasicBlockCalculateDominators:

	// *** Basic block 0

	.global printf
	.global BitSetInit
	.global BitSetCopy
	.global BasicBlockIsUnreachable
	.global BitSetClear
	.global BitSetIntersection
	.global BitSetInsert
	.global BitSetEqual
	.global BitSetDestruct
	.global BitSetCount
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
	mv          s1, a1
	mv          s2, a2
	mv          s3, a0
	ld          t0, 0(s1)
	li          t1, 22		// 0x16 ASCII \x16
	bne         t0, t1, .BasicBlockCalculateDominators_label_41

	// *** Basic block 1

	ld          s4, 0(s2)
	lla         a0, .str.1
	call        printf

	// *** Basic block 2

.BasicBlockCalculateDominators_label_41:
	addi        a0, s0, -48
	call        BitSetInit

	// *** Basic block 3

	addi        a0, s0, -48
	addi        a1, s1, 72
	call        BitSetCopy

	// *** Basic block 4

	addi        a0, s0, -32
	call        BitSetInit

	// *** Basic block 5

	mv          s4, x0
	mv          s5, x0
	addi        t0, s1, 8
	ld          s6, 8(t0)
	bge         x0, s6, .BasicBlockCalculateDominators_label_106

	// *** Basic block 6

	ld          s7, 8(s1)

	// *** Basic block 7

.BasicBlockCalculateDominators_label_66:
	slli        t0, s5, 3
	add         t0, s7, t0
	ld          s7, 0(t0)
	slli        t0, s7, 3
	add         t0, s4, t0
	ld          s7, 0(t0)
	mv          a1, s7
	mv          a0, s3
	call        BasicBlockIsUnreachable

	// *** Basic block 8

	bnez        a0, .BasicBlockCalculateDominators_label_102

	// *** Basic block 9

.BasicBlockCalculateDominators_label_82:
	addi        a0, s0, -32
	call        BitSetClear

	// *** Basic block 10

	addi        a0, s0, -48
	addi        a1, s7, 72
	addi        a2, s0, -32
	call        BitSetIntersection

	// *** Basic block 11

	addi        a0, s0, -48
	call        BitSetClear

	// *** Basic block 12

	addi        a0, s0, -48
	addi        a1, s0, -32
	call        BitSetCopy

	// *** Basic block 13

	addi        s4, s4, 1

	// *** Basic block 14

.BasicBlockCalculateDominators_label_102:
	addi        s5, s5, 1
	bge         s5, s6, .BasicBlockCalculateDominators_label_66

	// *** Basic block 15

.BasicBlockCalculateDominators_label_106:
	bnez        s4, .BasicBlockCalculateDominators_label_112

	// *** Basic block 16

	addi        a0, s0, -48
	call        BitSetClear

	// *** Basic block 17

.BasicBlockCalculateDominators_label_112:
	addi        a0, s0, -48
	ld          a1, 0(s1)
	call        BitSetInsert

	// *** Basic block 18

	addi        a0, s0, -48
	addi        a1, s1, 72
	call        BitSetEqual

	// *** Basic block 19

	not         s6, a0
	beqz        s6, .BasicBlockCalculateDominators_label_134

	// *** Basic block 20

	addi        a0, s1, 72
	call        BitSetClear

	// *** Basic block 21

	addi        a0, s1, 72
	addi        a1, s0, -48
	call        BitSetCopy

	// *** Basic block 22

.BasicBlockCalculateDominators_label_134:
	addi        a0, s0, -48
	call        BitSetDestruct

	// *** Basic block 23

	addi        a0, s0, -32
	call        BitSetDestruct

	// *** Basic block 24

	addi        a0, s1, 72
	call        BitSetCount

	// *** Basic block 25

	sd          a0, 88(s1)
	mv          a0, s6

	// *** Basic block 26

.BasicBlockCalculateDominators_label_148:
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
.func_end_BasicBlockCalculateDominators:
	.size BasicBlockCalculateDominators, .func_end_BasicBlockCalculateDominators-BasicBlockCalculateDominators

	.global BasicBlockCalculateImmediateDominator
	.type BasicBlockCalculateImmediateDominator, @function

BasicBlockCalculateImmediateDominator:

	// *** Basic block 0

	.global BitSetIteratorStart
	.global BitSetIteratorDone
	.global VectorGet
	.global BitSetIteratorNext
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
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, x0
	addi        a0, s0, -48
	addi        a1, s1, 72
	call        BitSetIteratorStart

	// *** Basic block 1

	addi        a0, s0, -48
	call        BitSetIteratorDone

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .BasicBlockCalculateImmediateDominator_label_78

	// *** Basic block 3

.BasicBlockCalculateImmediateDominator_label_35:
	addi        s4, s0, -48
	ld          t0, 8(s4)
	slli        t0, t0, 5
	ld          t1, 16(s4)
	add         s4, t0, t1

	// *** Basic block 4

.BasicBlockCalculateImmediateDominator_label_46:
	mv          s5, s4
	ld          t0, 0(s1)
	beq         s5, t0, .BasicBlockCalculateImmediateDominator_label_69

	// *** Basic block 5

	j           .BasicBlockCalculateImmediateDominator_label_54

	// *** Basic block 6

.BasicBlockCalculateImmediateDominator_label_54:
	mv          a1, s5
	mv          a0, s2
	call        VectorGet

	// *** Basic block 7

	mv          s4, a0
	ld          s3, 88(s4)
	bge         s3, s3, .BasicBlockCalculateImmediateDominator_label_68

	// *** Basic block 8

	sd          s4, 136(s1)

	// *** Basic block 9

.BasicBlockCalculateImmediateDominator_label_68:

	// *** Basic block 10

.BasicBlockCalculateImmediateDominator_label_69:
	addi        a0, s0, -48
	call        BitSetIteratorNext

	// *** Basic block 11

	addi        a0, s0, -48
	call        BitSetIteratorDone

	// *** Basic block 12

	not         t0, a0
	bnez        t0, .BasicBlockCalculateImmediateDominator_label_35

	// *** Basic block 13

.BasicBlockCalculateImmediateDominator_label_78:
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
.func_end_BasicBlockCalculateImmediateDominator:
	.size BasicBlockCalculateImmediateDominator, .func_end_BasicBlockCalculateImmediateDominator-BasicBlockCalculateImmediateDominator

	.global BasicBlockCalculateDominanceFrontier
	.type BasicBlockCalculateDominanceFrontier, @function

BasicBlockCalculateDominanceFrontier:

	// *** Basic block 0

	.global VectorGet
	.global BasicBlockIsUnreachable
	.global BasicBlockAddToDF
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
	mv          s1, a2
	mv          s2, a0
	addi        t1, t0, 8
	ld          s3, 8(t1)
	li          t1, 2		// 0x2 ASCII \x2
	blt         s3, t1, .BasicBlockCalculateDominanceFrontier_label_91

	// *** Basic block 1

	ld          s4, 8(t0)
	ld          s5, 136(t0)
	ld          s6, 0(t0)
	ld          s7, 136(t0)
	mv          s8, x0
	bge         x0, s3, .BasicBlockCalculateDominanceFrontier_label_90

	// *** Basic block 2

.BasicBlockCalculateDominanceFrontier_label_39:
	slli        t0, s8, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	mv          a1, s4
	mv          a0, s1
	call        VectorGet

	// *** Basic block 3

	mv          s4, a0
	mv          a1, s4
	mv          a0, s2
	call        BasicBlockIsUnreachable

	// *** Basic block 4

	bnez        a0, .BasicBlockCalculateDominanceFrontier_label_86

	// *** Basic block 5

.BasicBlockCalculateDominanceFrontier_label_58:
	mv          s1, s4
	sub         t1, s1, x0
	snez        t0, t1
	beq         s1, x0, .BasicBlockCalculateDominanceFrontier_label_67

	// *** Basic block 6

	sub         t1, s1, s5
	snez        t0, t1

	// *** Basic block 7

.BasicBlockCalculateDominanceFrontier_label_67:
	beqz        t0, .BasicBlockCalculateDominanceFrontier_label_85

	// *** Basic block 8

.BasicBlockCalculateDominanceFrontier_label_69:
	mv          a1, s6
	mv          a0, s1
	call        BasicBlockAddToDF

	// *** Basic block 9

	ld          s1, 136(s1)
	sub         t1, s1, x0
	snez        t0, t1
	beq         s1, x0, .BasicBlockCalculateDominanceFrontier_label_83

	// *** Basic block 10

	sub         t1, s1, s7
	snez        t0, t1

	// *** Basic block 11

.BasicBlockCalculateDominanceFrontier_label_83:
	bnez        t0, .BasicBlockCalculateDominanceFrontier_label_69

	// *** Basic block 12

.BasicBlockCalculateDominanceFrontier_label_85:

	// *** Basic block 13

.BasicBlockCalculateDominanceFrontier_label_86:
	addi        s8, s8, 1
	bge         s8, s3, .BasicBlockCalculateDominanceFrontier_label_39

	// *** Basic block 14

.BasicBlockCalculateDominanceFrontier_label_90:

	// *** Basic block 15

.BasicBlockCalculateDominanceFrontier_label_91:
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
.func_end_BasicBlockCalculateDominanceFrontier:
	.size BasicBlockCalculateDominanceFrontier, .func_end_BasicBlockCalculateDominanceFrontier-BasicBlockCalculateDominanceFrontier

	.global BasicBlockInitDominators
	.type BasicBlockInitDominators, @function

BasicBlockInitDominators:

	// *** Basic block 0

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
	mv          s2, a2
	beqz        a1, .BasicBlockInitDominators_label_25

	// *** Basic block 1

	addi        a0, s1, 72
	ld          a1, 0(s1)
	call        BitSetInsert

	// *** Basic block 2

	j           .BasicBlockInitDominators_label_50

	// *** Basic block 3

.BasicBlockInitDominators_label_25:
	addi        t0, s1, 8
	ld          t0, 8(t0)
	bnez        t0, .BasicBlockInitDominators_label_34

	// *** Basic block 4

.BasicBlockInitDominators_label_31:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.BasicBlockInitDominators_label_34:
	mv          s3, x0
	bge         x0, s2, .BasicBlockInitDominators_label_49

	// *** Basic block 6

.BasicBlockInitDominators_label_39:
	addi        a0, s1, 72
	mv          a1, s3
	call        BitSetInsert

	// *** Basic block 7

.BasicBlockInitDominators_label_45:
	addi        s3, s3, 1
	bge         s3, s2, .BasicBlockInitDominators_label_39

	// *** Basic block 8

.BasicBlockInitDominators_label_49:

	// *** Basic block 9

.BasicBlockInitDominators_label_50:
	j           .BasicBlockInitDominators_label_31
.func_end_BasicBlockInitDominators:
	.size BasicBlockInitDominators, .func_end_BasicBlockInitDominators-BasicBlockInitDominators

	.global BasicBlockAddToDF
	.type BasicBlockAddToDF, @function

BasicBlockAddToDF:

	// *** Basic block 0

	.global BitSetInsert
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 120
	j           BitSetInsert
.func_end_BasicBlockAddToDF:
	.size BasicBlockAddToDF, .func_end_BasicBlockAddToDF-BasicBlockAddToDF

	.global BasicBlockPrint
	.type BasicBlockPrint, @function

BasicBlockPrint:

	// *** Basic block 0

	.global fprintf
	.global BasicBlockIsUnreachable
	.global BitSetPrint
	.global IRPrint
	.global IRNext
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
	mv          s2, a1
	mv          s3, a3
	mv          s4, a0
	lla         s5, .str.2
	ld          s6, 0(s2)
	bne         s2, a2, .BasicBlockPrint_label_76

	// *** Basic block 1

	lla         a3, .str.3
	j           .BasicBlockPrint_label_88

	// *** Basic block 2

.BasicBlockPrint_label_76:
	bne         s2, s3, .BasicBlockPrint_label_84

	// *** Basic block 3

	lla         a3, .str.4
	j           .BasicBlockPrint_label_87

	// *** Basic block 4

.BasicBlockPrint_label_84:
	lla         a3, .str.5

	// *** Basic block 5

.BasicBlockPrint_label_87:

	// *** Basic block 6

.BasicBlockPrint_label_88:
	mv          a2, s6
	mv          a1, s5
	mv          a0, s1
	call        fprintf

	// *** Basic block 7

	mv          a1, s2
	mv          a0, s4
	call        BasicBlockIsUnreachable

	// *** Basic block 8

	beqz        a0, .BasicBlockPrint_label_109

	// *** Basic block 9

	lla         a1, .str.6
	mv          a0, s1
	call        fprintf

	// *** Basic block 10

.BasicBlockPrint_label_109:
	lb          t0, 236(s2)
	beqz        t0, .BasicBlockPrint_label_119

	// *** Basic block 11

	lla         a1, .str.7
	mv          a0, s1
	call        fprintf

	// *** Basic block 12

.BasicBlockPrint_label_119:
	lla         a1, .str.8
	mv          a0, s1
	call        fprintf

	// *** Basic block 13

	mv          s4, x0
	addi        t0, s2, 8
	ld          s5, 8(t0)
	bge         x0, s5, .BasicBlockPrint_label_149

	// *** Basic block 14

	ld          s6, 8(s2)

	// *** Basic block 15

.BasicBlockPrint_label_134:
	lla         a1, .str.9
	slli        t0, s4, 3
	add         t0, s6, t0
	ld          a2, 0(t0)
	mv          a0, s1
	call        fprintf

	// *** Basic block 16

.BasicBlockPrint_label_145:
	addi        s4, s4, 1
	bge         s4, s5, .BasicBlockPrint_label_134

	// *** Basic block 17

.BasicBlockPrint_label_149:
	lla         a1, .str.10
	mv          a0, s1
	call        fprintf

	// *** Basic block 18

	mv          s5, x0
	addi        t0, s2, 32
	ld          s6, 8(t0)
	bge         x0, s6, .BasicBlockPrint_label_179

	// *** Basic block 19

	ld          s7, 32(s2)

	// *** Basic block 20

.BasicBlockPrint_label_164:
	lla         a1, .str.11
	slli        t0, s5, 3
	add         t0, s7, t0
	ld          a2, 0(t0)
	mv          a0, s1
	call        fprintf

	// *** Basic block 21

.BasicBlockPrint_label_175:
	addi        s5, s5, 1
	bge         s5, s6, .BasicBlockPrint_label_164

	// *** Basic block 22

.BasicBlockPrint_label_179:
	lla         a1, .str.12
	mv          a0, s1
	call        fprintf

	// *** Basic block 23

	addi        a0, s2, 144
	mv          a1, s1
	call        BitSetPrint

	// *** Basic block 24

	lla         a1, .str.13
	mv          a0, s1
	call        fprintf

	// *** Basic block 25

	addi        a0, s2, 72
	mv          a1, s1
	call        BitSetPrint

	// *** Basic block 26

	lla         a1, .str.14
	mv          a0, s1
	call        fprintf

	// *** Basic block 27

	mv          s6, x0
	addi        t0, s2, 96
	ld          s7, 8(t0)
	bge         x0, s7, .BasicBlockPrint_label_231

	// *** Basic block 28

	ld          s8, 96(s2)

	// *** Basic block 29

.BasicBlockPrint_label_216:
	lla         a1, .str.15
	slli        t0, s6, 3
	add         t0, s8, t0
	ld          a2, 0(t0)
	mv          a0, s1
	call        fprintf

	// *** Basic block 30

.BasicBlockPrint_label_227:
	addi        s6, s6, 1
	bge         s6, s7, .BasicBlockPrint_label_216

	// *** Basic block 31

.BasicBlockPrint_label_231:
	lla         a1, .str.16
	mv          a0, s1
	call        fprintf

	// *** Basic block 32

	addi        a0, s2, 120
	mv          a1, s1
	call        BitSetPrint

	// *** Basic block 33

	lla         a1, .str.17
	mv          a0, s1
	call        fprintf

	// *** Basic block 34

	ld          s7, 136(s2)
	bne         s7, x0, .BasicBlockPrint_label_261

	// *** Basic block 35

	lla         a1, .str.18
	mv          a0, s1
	call        fprintf

	// *** Basic block 36

	j           .BasicBlockPrint_label_270

	// *** Basic block 37

.BasicBlockPrint_label_261:
	lla         a1, .str.19
	ld          a2, 0(s7)
	mv          a0, s1
	call        fprintf

	// *** Basic block 38

.BasicBlockPrint_label_270:
	lla         a1, .str.20
	lw          a2, 240(s2)
	mv          a0, s1
	call        fprintf

	// *** Basic block 39

	lla         a1, .str.21
	lw          a2, 232(s2)
	mv          a0, s1
	call        fprintf

	// *** Basic block 40

	lla         a1, .str.22
	mv          a0, s1
	call        fprintf

	// *** Basic block 41

	mv          s7, x0
	addi        t0, s2, 160
	ld          s8, 8(t0)
	bge         x0, s8, .BasicBlockPrint_label_321

	// *** Basic block 42

	ld          s9, 160(s2)

	// *** Basic block 43

.BasicBlockPrint_label_303:
	slli        t0, s7, 4
	add         t0, s9, t0
	ld          s9, 0(t0)
	lla         a1, .str.23
	ld          a2, 16(s9)
	mv          a0, s1
	call        fprintf

	// *** Basic block 44

.BasicBlockPrint_label_317:
	addi        s7, s7, 1
	bge         s7, s8, .BasicBlockPrint_label_303

	// *** Basic block 45

.BasicBlockPrint_label_321:
	lla         a1, .str.24
	mv          a0, s1
	call        fprintf

	// *** Basic block 46

	lla         a1, .str.25
	mv          a0, s1
	call        fprintf

	// *** Basic block 47

	mv          s8, x0
	addi        t0, s2, 192
	ld          s9, 8(t0)
	bge         x0, s9, .BasicBlockPrint_label_360

	// *** Basic block 48

	ld          s10, 192(s2)

	// *** Basic block 49

.BasicBlockPrint_label_342:
	slli        t0, s8, 3
	add         t0, s10, t0
	ld          s10, 0(t0)
	lla         a1, .str.26
	ld          a2, 16(s10)
	mv          a0, s1
	call        fprintf

	// *** Basic block 50

.BasicBlockPrint_label_356:
	addi        s8, s8, 1
	bge         s8, s9, .BasicBlockPrint_label_342

	// *** Basic block 51

.BasicBlockPrint_label_360:
	lla         a1, .str.27
	mv          a0, s1
	call        fprintf

	// *** Basic block 52

	ld          s9, 56(s2)
	ld          s10, 64(s2)
	beq         s9, s10, .BasicBlockPrint_label_388

	// *** Basic block 53

.BasicBlockPrint_label_375:
	mv          a1, s1
	mv          a0, s9
	call        IRPrint

	// *** Basic block 54

	mv          a0, s9
	call        IRNext

	// *** Basic block 55

	mv          s9, a0
	bne         s9, s10, .BasicBlockPrint_label_375

	// *** Basic block 56

.BasicBlockPrint_label_388:
	beq         s10, x0, .BasicBlockPrint_label_397

	// *** Basic block 57

	mv          a1, s1
	mv          a0, s10
	call        IRPrint

	// *** Basic block 58

.BasicBlockPrint_label_397:
	lla         a1, .str.28
	mv          a0, s1
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
	j           fprintf
.func_end_BasicBlockPrint:
	.size BasicBlockPrint, .func_end_BasicBlockPrint-BasicBlockPrint

	.global BasicBlockEndsInBranchReturnOrCall
	.type BasicBlockEndsInBranchReturnOrCall, @function

BasicBlockEndsInBranchReturnOrCall:

	// *** Basic block 0

	.global IRIsBranch
	.global IRIsReturn
	.global IRIsCall
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	ld          s1, 64(a0)
	sub         t0, s1, x0
	snez        a0, t0
	beq         s1, x0, .BasicBlockEndsInBranchReturnOrCall_label_36

	// *** Basic block 1

	mv          a0, s1
	call        IRIsBranch

	// *** Basic block 2

	bnez        a0, .BasicBlockEndsInBranchReturnOrCall_label_29

	// *** Basic block 3

	mv          a0, s1
	call        IRIsReturn

	// *** Basic block 4


	// *** Basic block 5

.BasicBlockEndsInBranchReturnOrCall_label_29:
	bnez        a0, .BasicBlockEndsInBranchReturnOrCall_label_35

	// *** Basic block 6

	mv          a0, s1
	call        IRIsCall

	// *** Basic block 8

.BasicBlockEndsInBranchReturnOrCall_label_35:

	// *** Basic block 9

.BasicBlockEndsInBranchReturnOrCall_label_36:

	// *** Basic block 10

.BasicBlockEndsInBranchReturnOrCall_label_38:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BasicBlockEndsInBranchReturnOrCall:
	.size BasicBlockEndsInBranchReturnOrCall, .func_end_BasicBlockEndsInBranchReturnOrCall-BasicBlockEndsInBranchReturnOrCall

	.global BasicBlockInsertVar
	.type BasicBlockInsertVar, @function

BasicBlockInsertVar:

	// *** Basic block 0

	.global BasicBlockEmitBefore
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	ld          a3, 56(t0)
	j           BasicBlockEmitBefore
.func_end_BasicBlockInsertVar:
	.size BasicBlockInsertVar, .func_end_BasicBlockInsertVar-BasicBlockInsertVar

	.global BasicBlockInsertPhi
	.type BasicBlockInsertPhi, @function

BasicBlockInsertPhi:

	// *** Basic block 0

	.global IRNext
	.global NewIRPhi
	.global IRSetVarDef
	.global GeneratorEmitBefore
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
	mv          s2, a2
	mv          s3, a0
	ld          s4, 56(s1)
	bne         s4, x0, .BasicBlockInsertPhi_label_33

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.BasicBlockInsertPhi_label_30:
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

	// *** Basic block 3

.BasicBlockInsertPhi_label_33:
	lw          t0, 20(s4)
	li          s5, 122		// 0x7a ASCII 'z'
	bne         t0, s5, .BasicBlockInsertPhi_label_64

	// *** Basic block 4

.BasicBlockInsertPhi_label_42:
	mv          s6, s4
	ld          t0, 136(s6)
	bne         t0, s2, .BasicBlockInsertPhi_label_53

	// *** Basic block 5

	mv          a0, x0
	j           .BasicBlockInsertPhi_label_30

	// *** Basic block 6

.BasicBlockInsertPhi_label_53:
	mv          a0, s4
	call        IRNext

	// *** Basic block 7

	mv          s4, a0
	lw          t0, 20(s4)
	beq         t0, s5, .BasicBlockInsertPhi_label_42

	// *** Basic block 8

.BasicBlockInsertPhi_label_64:
	mv          a0, s2
	call        NewIRPhi

	// *** Basic block 9

	mv          s5, a0
	mv          a1, s2
	mv          a0, s5
	call        IRSetVarDef

	// *** Basic block 10

	mv          a2, s4
	mv          a1, s5
	mv          a0, s3
	call        GeneratorEmitBefore

	// *** Basic block 11

	sd          a0, 56(s1)
	sd          s1, 72(s5)
	li          a0, 1		// 0x1 ASCII \x1
	j           .BasicBlockInsertPhi_label_30
.func_end_BasicBlockInsertPhi:
	.size BasicBlockInsertPhi, .func_end_BasicBlockInsertPhi-BasicBlockInsertPhi

	.global BasicBlockRemoveInstruction
	.type BasicBlockRemoveInstruction, @function

BasicBlockRemoveInstruction:

	// *** Basic block 0

	.global IRNext
	.global IRPrev
	.global GeneratorRemoveInstruction
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
	mv          s2, a1
	mv          s3, a0
	ld          t0, 56(s2)
	bne         s1, t0, .BasicBlockRemoveInstruction_label_39

	// *** Basic block 1

	ld          s4, 64(s2)
	bne         s1, s4, .BasicBlockRemoveInstruction_label_32

	// *** Basic block 2

	sd          s4, 56(s2)
	sd          x0, 0(t0)
	j           .BasicBlockRemoveInstruction_label_38

	// *** Basic block 3

.BasicBlockRemoveInstruction_label_32:
	mv          a0, s1
	call        IRNext

	// *** Basic block 4

	sd          a0, 56(s2)

	// *** Basic block 5

.BasicBlockRemoveInstruction_label_38:

	// *** Basic block 6

.BasicBlockRemoveInstruction_label_39:
	ld          t0, 64(s2)
	bne         s1, t0, .BasicBlockRemoveInstruction_label_50

	// *** Basic block 7

	mv          a0, s1
	call        IRPrev

	// *** Basic block 8

	sd          a0, 64(s2)

	// *** Basic block 9

.BasicBlockRemoveInstruction_label_50:
	mv          a1, s1
	mv          a0, s3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GeneratorRemoveInstruction
.func_end_BasicBlockRemoveInstruction:
	.size BasicBlockRemoveInstruction, .func_end_BasicBlockRemoveInstruction-BasicBlockRemoveInstruction

	.global BasicBlockMoveInstructionAfter
	.type BasicBlockMoveInstructionAfter, @function

BasicBlockMoveInstructionAfter:

	// *** Basic block 0

	.global IRNext
	.global IRPrev
	.global GeneratorMoveInstructionAfter
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
	ld          s4, 72(s1)
	ld          t0, 56(s4)
	bne         s1, t0, .BasicBlockMoveInstructionAfter_label_43

	// *** Basic block 1

	ld          s5, 64(s4)
	bne         s1, s5, .BasicBlockMoveInstructionAfter_label_36

	// *** Basic block 2

	sd          s5, 56(s4)
	sd          x0, 0(t0)
	j           .BasicBlockMoveInstructionAfter_label_42

	// *** Basic block 3

.BasicBlockMoveInstructionAfter_label_36:
	mv          a0, s1
	call        IRNext

	// *** Basic block 4

	sd          a0, 56(s4)

	// *** Basic block 5

.BasicBlockMoveInstructionAfter_label_42:

	// *** Basic block 6

.BasicBlockMoveInstructionAfter_label_43:
	ld          t0, 64(s4)
	bne         s1, t0, .BasicBlockMoveInstructionAfter_label_54

	// *** Basic block 7

	mv          a0, s1
	call        IRPrev

	// *** Basic block 8

	sd          a0, 64(s4)

	// *** Basic block 9

.BasicBlockMoveInstructionAfter_label_54:
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        GeneratorMoveInstructionAfter

	// *** Basic block 10

	ld          t0, 72(s3)
	sd          t0, 72(s1)
	ld          t0, 64(t0)
	bne         t0, s3, .BasicBlockMoveInstructionAfter_label_75

	// *** Basic block 11

	ld          t0, 72(s3)
	sd          s1, 64(t0)

	// *** Basic block 12

.BasicBlockMoveInstructionAfter_label_75:
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
.func_end_BasicBlockMoveInstructionAfter:
	.size BasicBlockMoveInstructionAfter, .func_end_BasicBlockMoveInstructionAfter-BasicBlockMoveInstructionAfter

	.global BasicBlockMoveInstructionBefore
	.type BasicBlockMoveInstructionBefore, @function

BasicBlockMoveInstructionBefore:

	// *** Basic block 0

	.global IRNext
	.global IRPrev
	.global GeneratorMoveInstructionBefore
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
	ld          s4, 72(s1)
	ld          t0, 56(s4)
	bne         s1, t0, .BasicBlockMoveInstructionBefore_label_43

	// *** Basic block 1

	ld          s5, 64(s4)
	bne         s1, s5, .BasicBlockMoveInstructionBefore_label_36

	// *** Basic block 2

	sd          s5, 56(s4)
	sd          x0, 0(t0)
	j           .BasicBlockMoveInstructionBefore_label_42

	// *** Basic block 3

.BasicBlockMoveInstructionBefore_label_36:
	mv          a0, s1
	call        IRNext

	// *** Basic block 4

	sd          a0, 56(s4)

	// *** Basic block 5

.BasicBlockMoveInstructionBefore_label_42:

	// *** Basic block 6

.BasicBlockMoveInstructionBefore_label_43:
	ld          t0, 64(s4)
	bne         s1, t0, .BasicBlockMoveInstructionBefore_label_54

	// *** Basic block 7

	mv          a0, s1
	call        IRPrev

	// *** Basic block 8

	sd          a0, 64(s4)

	// *** Basic block 9

.BasicBlockMoveInstructionBefore_label_54:
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        GeneratorMoveInstructionBefore

	// *** Basic block 10

	ld          t0, 72(s3)
	sd          t0, 72(s1)
	ld          t0, 56(t0)
	bne         t0, s3, .BasicBlockMoveInstructionBefore_label_75

	// *** Basic block 11

	ld          t0, 72(s3)
	sd          s1, 56(t0)

	// *** Basic block 12

.BasicBlockMoveInstructionBefore_label_75:
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
.func_end_BasicBlockMoveInstructionBefore:
	.size BasicBlockMoveInstructionBefore, .func_end_BasicBlockMoveInstructionBefore-BasicBlockMoveInstructionBefore

	.global BasicBlockReplaceInstruction
	.type BasicBlockReplaceInstruction, @function

BasicBlockReplaceInstruction:

	// *** Basic block 0

	.global IRNext
	.global IRPrev
	.global GeneratorReplaceInstruction
	.global BasicBlockRemoveInstruction
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
	mv          s2, a1
	mv          s3, a0
	mv          s4, a3
	ld          t0, 56(s2)
	bne         s1, t0, .BasicBlockReplaceInstruction_label_31

	// *** Basic block 1

	mv          a0, s1
	call        IRNext

	// *** Basic block 2

	sd          a0, 56(s2)

	// *** Basic block 3

.BasicBlockReplaceInstruction_label_31:
	ld          t0, 64(s2)
	bne         s1, t0, .BasicBlockReplaceInstruction_label_42

	// *** Basic block 4

	mv          a0, s1
	call        IRPrev

	// *** Basic block 5

	sd          a0, 64(s2)

	// *** Basic block 6

.BasicBlockReplaceInstruction_label_42:
	mv          a2, s4
	mv          a1, s1
	mv          a0, s3
	call        GeneratorReplaceInstruction

	// *** Basic block 7

	mv          a2, s1
	mv          a1, s2
	mv          a0, s3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           BasicBlockRemoveInstruction
.func_end_BasicBlockReplaceInstruction:
	.size BasicBlockReplaceInstruction, .func_end_BasicBlockReplaceInstruction-BasicBlockReplaceInstruction

	.global BasicBlockEmitBefore
	.type BasicBlockEmitBefore, @function

BasicBlockEmitBefore:

	// *** Basic block 0

	.global GeneratorEmitBefore
	// Leaf procedure, no stack frame generated
	mv          t0, a3
	mv          t1, a1
	mv          t2, a2
	mv          t3, a0
	ld          t4, 56(t1)
	bne         t0, t4, .BasicBlockEmitBefore_label_25

	// *** Basic block 1

	sd          t2, 56(t1)

	// *** Basic block 2

.BasicBlockEmitBefore_label_25:
	mv          a2, t0
	mv          a1, t2
	mv          a0, t3
	j           GeneratorEmitBefore
.func_end_BasicBlockEmitBefore:
	.size BasicBlockEmitBefore, .func_end_BasicBlockEmitBefore-BasicBlockEmitBefore

	.global BasicBlockIsUnreachable
	.type BasicBlockIsUnreachable, @function

BasicBlockIsUnreachable:

	// *** Basic block 0

	.global VectorGet
	.global BasicBlockIsUnreachable
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
	ld          t0, 192(s2)
	bne         s1, t0, .BasicBlockIsUnreachable_label_30

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.BasicBlockIsUnreachable_label_27:
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

	// *** Basic block 3

.BasicBlockIsUnreachable_label_30:
	lb          t0, 237(s1)
	beqz        t0, .BasicBlockIsUnreachable_label_38

	// *** Basic block 4

	lb          a0, 238(s1)
	j           .BasicBlockIsUnreachable_label_27

	// *** Basic block 5

.BasicBlockIsUnreachable_label_38:
	li          s3, 1		// 0x1 ASCII \x1
	sb          s3, 237(s1)
	mv          s4, x0
	addi        t0, s1, 8
	ld          s5, 8(t0)
	bge         x0, s5, .BasicBlockIsUnreachable_label_78

	// *** Basic block 6

	ld          s6, 8(s1)

	// *** Basic block 7

.BasicBlockIsUnreachable_label_50:
	slli        t0, s4, 3
	add         t0, s6, t0
	ld          s6, 0(t0)
	addi        a0, s2, 168
	mv          a1, s6
	call        VectorGet

	// *** Basic block 8

	mv          s6, a0
	mv          a1, s6
	mv          a0, s2
	call        BasicBlockIsUnreachable

	// *** Basic block 9

	not         t0, a0
	beqz        t0, .BasicBlockIsUnreachable_label_73

	// *** Basic block 10

	mv          a0, x0
	j           .BasicBlockIsUnreachable_label_27

	// *** Basic block 11

.BasicBlockIsUnreachable_label_73:

	// *** Basic block 12

.BasicBlockIsUnreachable_label_74:
	addi        s4, s4, 1
	bge         s4, s5, .BasicBlockIsUnreachable_label_50

	// *** Basic block 13

.BasicBlockIsUnreachable_label_78:
	sb          s3, 238(s1)
	li          a0, 1		// 0x1 ASCII \x1
	j           .BasicBlockIsUnreachable_label_27
.func_end_BasicBlockIsUnreachable:
	.size BasicBlockIsUnreachable, .func_end_BasicBlockIsUnreachable-BasicBlockIsUnreachable

	.global BasicBlockRemoveInput
	.type BasicBlockRemoveInput, @function

BasicBlockRemoveInput:

	// *** Basic block 0

	.global VectorDeleteElement
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
	mv          t0, a1
	mv          s2, x0
	addi        t1, s1, 8
	ld          s3, 8(t1)
	bge         x0, s3, .BasicBlockRemoveInput_label_42

	// *** Basic block 1

	ld          t1, 8(s1)

	// *** Basic block 2

.BasicBlockRemoveInput_label_22:
	slli        t2, s2, 3
	add         t1, t1, t2
	ld          t2, 0(t1)
	bne         t2, t0, .BasicBlockRemoveInput_label_37

	// *** Basic block 3

	addi        a0, s1, 8
	mv          a1, s2
	call        VectorDeleteElement

	// *** Basic block 4

	j           .BasicBlockRemoveInput_label_42

	// *** Basic block 5

.BasicBlockRemoveInput_label_37:

	// *** Basic block 6

.BasicBlockRemoveInput_label_38:
	addi        s2, s2, 1
	bge         s2, s3, .BasicBlockRemoveInput_label_22

	// *** Basic block 7

.BasicBlockRemoveInput_label_42:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BasicBlockRemoveInput:
	.size BasicBlockRemoveInput, .func_end_BasicBlockRemoveInput-BasicBlockRemoveInput

	.global BasicBlockRemoveEdge
	.type BasicBlockRemoveEdge, @function

BasicBlockRemoveEdge:

	// *** Basic block 0

	.global VectorDeleteElement
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
	addi        t0, s1, 8
	ld          s4, 8(t0)
	bge         x0, s4, .BasicBlockRemoveEdge_label_45

	// *** Basic block 1

	ld          t0, 8(s1)
	ld          t1, 0(s2)

	// *** Basic block 2

.BasicBlockRemoveEdge_label_25:
	slli        t2, s3, 3
	add         t0, t0, t2
	ld          s5, 0(t0)
	bne         s5, t1, .BasicBlockRemoveEdge_label_40

	// *** Basic block 3

	addi        a0, s1, 8
	mv          a1, s3
	call        VectorDeleteElement

	// *** Basic block 4

	j           .BasicBlockRemoveEdge_label_45

	// *** Basic block 5

.BasicBlockRemoveEdge_label_40:

	// *** Basic block 6

.BasicBlockRemoveEdge_label_41:
	addi        s3, s3, 1
	bge         s3, s4, .BasicBlockRemoveEdge_label_25

	// *** Basic block 7

.BasicBlockRemoveEdge_label_45:
	mv          s4, x0
	addi        t0, s2, 32
	ld          s5, 8(t0)
	bge         x0, s5, .BasicBlockRemoveEdge_label_74

	// *** Basic block 8

	ld          t0, 32(s2)
	ld          t1, 0(s1)

	// *** Basic block 9

.BasicBlockRemoveEdge_label_55:
	slli        t2, s4, 3
	add         t0, t0, t2
	ld          s6, 0(t0)
	bne         s6, t1, .BasicBlockRemoveEdge_label_69

	// *** Basic block 10

	addi        a0, s2, 32
	mv          a1, s4
	call        VectorDeleteElement

	// *** Basic block 11

	j           .BasicBlockRemoveEdge_label_74

	// *** Basic block 12

.BasicBlockRemoveEdge_label_69:

	// *** Basic block 13

.BasicBlockRemoveEdge_label_70:
	addi        s4, s4, 1
	bge         s4, s5, .BasicBlockRemoveEdge_label_55

	// *** Basic block 14

.BasicBlockRemoveEdge_label_74:
	sb          x0, 237(s1)
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
.func_end_BasicBlockRemoveEdge:
	.size BasicBlockRemoveEdge, .func_end_BasicBlockRemoveEdge-BasicBlockRemoveEdge

	.global BasicBlockClear
	.type BasicBlockClear, @function

BasicBlockClear:

	// *** Basic block 0

	.global IRNext
	.global GeneratorRemoveInstruction
	.global VectorGet
	.global BasicBlockRemoveInput
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
	mv          s1, a1
	mv          s2, a0
	ld          s3, 56(s1)
	bne         s3, x0, .BasicBlockClear_label_33

	// *** Basic block 1

	ld          t0, 64(s1)

	// *** Basic block 2

.BasicBlockClear_label_30:
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

	// *** Basic block 3

.BasicBlockClear_label_33:
	mv          s4, x0
	mv          s5, x0
	sub         t1, s3, x0
	snez        t0, t1
	beq         s3, x0, .BasicBlockClear_label_47

	// *** Basic block 4

	ld          t1, 64(s1)
	sub         t1, s3, t1
	snez        t0, t1

	// *** Basic block 5

.BasicBlockClear_label_47:
	beqz        t0, .BasicBlockClear_label_88

	// *** Basic block 6

.BasicBlockClear_label_49:
	mv          a0, s3
	call        IRNext

	// *** Basic block 7

	mv          s6, a0
	lw          t0, 20(s3)
	li          t1, 18		// 0x12 ASCII \x12
	beq         t0, t1, .BasicBlockClear_label_68

	// *** Basic block 8

	mv          a1, s3
	mv          a0, s2
	call        GeneratorRemoveInstruction

	// *** Basic block 9

	j           .BasicBlockClear_label_78

	// *** Basic block 10

.BasicBlockClear_label_68:
	bne         s4, x0, .BasicBlockClear_label_75

	// *** Basic block 11

	mv          s4, s3
	mv          s5, s3
	j           .BasicBlockClear_label_77

	// *** Basic block 12

.BasicBlockClear_label_75:
	mv          s5, s3

	// *** Basic block 13

.BasicBlockClear_label_77:

	// *** Basic block 14

.BasicBlockClear_label_78:
	mv          s3, s6
	sub         t1, s3, x0
	snez        t0, t1
	beq         s3, x0, .BasicBlockClear_label_86

	// *** Basic block 15

	sub         t1, s3, t0
	snez        t0, t1

	// *** Basic block 16

.BasicBlockClear_label_86:
	bnez        t0, .BasicBlockClear_label_49

	// *** Basic block 17

.BasicBlockClear_label_88:
	ld          s7, 64(s1)
	sub         t1, s7, x0
	snez        t0, t1
	beq         s7, x0, .BasicBlockClear_label_99

	// *** Basic block 18

	lw          t1, 20(s7)
	addi        t1, t1, -18
	snez        t0, t1

	// *** Basic block 19

.BasicBlockClear_label_99:
	beqz        t0, .BasicBlockClear_label_107

	// *** Basic block 20

	mv          a1, s7
	mv          a0, s2
	call        GeneratorRemoveInstruction

	// *** Basic block 21

	j           .BasicBlockClear_label_117

	// *** Basic block 22

.BasicBlockClear_label_107:
	bne         s4, x0, .BasicBlockClear_label_114

	// *** Basic block 23

	mv          s4, s3
	mv          s5, s3
	j           .BasicBlockClear_label_116

	// *** Basic block 24

.BasicBlockClear_label_114:
	mv          s5, s3

	// *** Basic block 25

.BasicBlockClear_label_116:

	// *** Basic block 26

.BasicBlockClear_label_117:
	sd          s4, 56(s1)
	sd          s5, 64(s1)
	mv          s7, x0
	addi        t0, s1, 32
	ld          s8, 8(t0)
	bge         x0, s8, .BasicBlockClear_label_153

	// *** Basic block 27

	ld          s9, 32(s1)
	ld          s10, 0(s1)

	// *** Basic block 28

.BasicBlockClear_label_131:
	slli        t0, s7, 3
	add         t0, s9, t0
	ld          s9, 0(t0)
	addi        a0, s2, 168
	mv          a1, s9
	call        VectorGet

	// *** Basic block 29

	mv          s9, a0
	mv          a1, s10
	mv          a0, s9
	call        BasicBlockRemoveInput

	// *** Basic block 30

.BasicBlockClear_label_149:
	addi        s7, s7, 1
	bge         s7, s8, .BasicBlockClear_label_131

	// *** Basic block 31

.BasicBlockClear_label_153:
	j           .BasicBlockClear_label_30
.func_end_BasicBlockClear:
	.size BasicBlockClear, .func_end_BasicBlockClear-BasicBlockClear

	.local  TraverseDomTree
	.type TraverseDomTree, @function

TraverseDomTree:

	// *** Basic block 0

	.local TraverseDomTree
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
	mv          s2, a3
	mv          s3, a2
	mv          s4, a4
	mv          s5, a0
	bne         s1, x0, .TraverseDomTree_label_34

	// *** Basic block 1

	ld          t0, 168(s5)

	// *** Basic block 2

.TraverseDomTree_label_31:
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

	// *** Basic block 3

.TraverseDomTree_label_34:
	bnez        s2, .TraverseDomTree_label_42

	// *** Basic block 4

	mv          a1, s4
	mv          a0, s1
	jalr         x1, s3, 0

	// *** Basic block 5

.TraverseDomTree_label_42:
	mv          s6, x0
	addi        t0, s1, 96
	ld          s7, 8(t0)
	bge         x0, s7, .TraverseDomTree_label_80

	// *** Basic block 6

	ld          t0, 96(s1)

	// *** Basic block 7

.TraverseDomTree_label_51:
	slli        t1, s6, 3
	add         t0, t0, t1
	ld          s8, 0(t0)
	slli        t0, s8, 3
	add         t0, t0, t0
	ld          s8, 0(t0)
	beq         s8, s1, .TraverseDomTree_label_76

	// *** Basic block 8

.TraverseDomTree_label_64:
	mv          a4, s4
	mv          a3, s2
	mv          a2, s3
	mv          a1, s8
	mv          a0, s5
	call        TraverseDomTree

	// *** Basic block 9

.TraverseDomTree_label_76:
	addi        s6, s6, 1
	bge         s6, s7, .TraverseDomTree_label_51

	// *** Basic block 10

.TraverseDomTree_label_80:
	li          t0, 1		// 0x1 ASCII \x1
	bne         s2, t0, .TraverseDomTree_label_91

	// *** Basic block 11

	mv          a1, s4
	mv          a0, s1
	jalr         x1, s3, 0

	// *** Basic block 12

.TraverseDomTree_label_91:
	j           .TraverseDomTree_label_31
.func_end_TraverseDomTree:
	.size TraverseDomTree, .func_end_TraverseDomTree-TraverseDomTree

	.global BasicBlockTraverseDominatorTree
	.type BasicBlockTraverseDominatorTree, @function

BasicBlockTraverseDominatorTree:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local TraverseDomTree
	j           TraverseDomTree
.func_end_BasicBlockTraverseDominatorTree:
	.size BasicBlockTraverseDominatorTree, .func_end_BasicBlockTraverseDominatorTree-BasicBlockTraverseDominatorTree

	.global BasicBlockBegin
	.type BasicBlockBegin, @function

BasicBlockBegin:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          a0, 56(a0)

	// *** Basic block 1

.BasicBlockBegin_label_10:
	ret         
.func_end_BasicBlockBegin:
	.size BasicBlockBegin, .func_end_BasicBlockBegin-BasicBlockBegin

	.global BasicBlockEnd
	.type BasicBlockEnd, @function

BasicBlockEnd:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 64(a0)
	bne         t0, x0, .BasicBlockEnd_label_19

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.BasicBlockEnd_label_16:
	ret         

	// *** Basic block 3

.BasicBlockEnd_label_19:
	ld          a0, 8(t0)
	ret         
.func_end_BasicBlockEnd:
	.size BasicBlockEnd, .func_end_BasicBlockEnd-BasicBlockEnd

	.global BasicBlockRBegin
	.type BasicBlockRBegin, @function

BasicBlockRBegin:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          a0, 64(a0)

	// *** Basic block 1

.BasicBlockRBegin_label_10:
	ret         
.func_end_BasicBlockRBegin:
	.size BasicBlockRBegin, .func_end_BasicBlockRBegin-BasicBlockRBegin

	.global BasicBlockREnd
	.type BasicBlockREnd, @function

BasicBlockREnd:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 56(a0)
	bne         t0, x0, .BasicBlockREnd_label_18

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.BasicBlockREnd_label_15:
	ret         

	// *** Basic block 3

.BasicBlockREnd_label_18:
	ld          a0, 0(t0)
	ret         
.func_end_BasicBlockREnd:
	.size BasicBlockREnd, .func_end_BasicBlockREnd-BasicBlockREnd

	.global BasicBlockIsEmpty
	.type BasicBlockIsEmpty, @function

BasicBlockIsEmpty:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 56(t0)
	sub         t2, t1, x0
	seqz        a0, t2
	beq         t1, x0, .BasicBlockIsEmpty_label_19

	// *** Basic block 1

	ld          t1, 64(t0)
	sub         t1, t1, x0
	seqz        a0, t1

	// *** Basic block 2

.BasicBlockIsEmpty_label_19:

	// *** Basic block 3

.BasicBlockIsEmpty_label_21:
	ret         
.func_end_BasicBlockIsEmpty:
	.size BasicBlockIsEmpty, .func_end_BasicBlockIsEmpty-BasicBlockIsEmpty

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "(null)"
	.type .str.1, @object
	.size .str.1, 1

.str.2:
	.asciz "*** Basic block #%zd%s\n"
	.type .str.2, @object
	.size .str.2, 24

.str.3:
	.asciz " (ENTRY)"
	.type .str.3, @object
	.size .str.3, 9

.str.4:
	.asciz " (EXIT)"
	.type .str.4, @object
	.size .str.4, 8

.str.5:
	.asciz "(null)"
	.type .str.5, @object
	.size .str.5, 1

.str.6:
	.asciz "** Unreachable **\n"
	.type .str.6, @object
	.size .str.6, 19

.str.7:
	.asciz "  [return]\n"
	.type .str.7, @object
	.size .str.7, 12

.str.8:
	.asciz "  In:"
	.type .str.8, @object
	.size .str.8, 6

.str.9:
	.asciz " %zd"
	.type .str.9, @object
	.size .str.9, 5

.str.10:
	.asciz "\n  Out:"
	.type .str.10, @object
	.size .str.10, 8

.str.11:
	.asciz " %zd"
	.type .str.11, @object
	.size .str.11, 5

.str.12:
	.asciz "\n  Back: "
	.type .str.12, @object
	.size .str.12, 10

.str.13:
	.asciz "\n  Dominators: "
	.type .str.13, @object
	.size .str.13, 16

.str.14:
	.asciz "\n  Dominatees:"
	.type .str.14, @object
	.size .str.14, 15

.str.15:
	.asciz " %zd"
	.type .str.15, @object
	.size .str.15, 5

.str.16:
	.asciz "\n  DF: "
	.type .str.16, @object
	.size .str.16, 8

.str.17:
	.asciz "\n  Immediate Dominator: "
	.type .str.17, @object
	.size .str.17, 25

.str.18:
	.asciz "NIL\n"
	.type .str.18, @object
	.size .str.18, 5

.str.19:
	.asciz "%zd\n"
	.type .str.19, @object
	.size .str.19, 5

.str.20:
	.asciz "  Loop nesting: %d\n"
	.type .str.20, @object
	.size .str.20, 20

.str.21:
	.asciz "  Number of calls: %d\n"
	.type .str.21, @object
	.size .str.21, 23

.str.22:
	.asciz "  Defined variables:"
	.type .str.22, @object
	.size .str.22, 21

.str.23:
	.asciz " %s"
	.type .str.23, @object
	.size .str.23, 4

.str.24:
	.asciz "\n"
	.type .str.24, @object
	.size .str.24, 2

.str.25:
	.asciz "  Referenced variables:"
	.type .str.25, @object
	.size .str.25, 24

.str.26:
	.asciz " %s"
	.type .str.26, @object
	.size .str.26, 4

.str.27:
	.asciz "\n"
	.type .str.27, @object
	.size .str.27, 2

.str.28:
	.asciz "\n"
	.type .str.28, @object
	.size .str.28, 2

