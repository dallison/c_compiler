	.file   "init_semantics.c"
	.text
	.option pic
.PCbegin:
	.local  NewINode
	.type NewINode, @function

NewINode:

	// *** Basic block 0

	.global malloc
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
	mv          s2, a1
	mv          s3, a2
	li          a0, 80		// 0x50 ASCII 'P'
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	sw          s1, 0(s4)
	sd          s2, 8(s4)
	sd          x0, 16(s4)
	sd          s3, 24(s4)
	sd          x0, 32(s4)
	sd          x0, 40(s4)
	sd          x0, 72(s4)
	addi        a0, s4, 48
	call        VectorInit

	// *** Basic block 2

	mv          a0, s4

	// *** Basic block 3

.NewINode_label_47:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewINode:
	.size NewINode, .func_end_NewINode-NewINode

	.local  AppendStructMembers
	.type AppendStructMembers, @function

AppendStructMembers:

	// *** Basic block 0

	.global BuildINode
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
	addi        t0, s1, 48
	ld          t0, 8(t0)
	bge         x0, t0, .AppendStructMembers_label_28

	// *** Basic block 1

.AppendStructMembers_label_25:
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

.AppendStructMembers_label_28:
	ld          t0, 8(s1)
	mv          s2, x0
	ld          t0, 32(t0)
	addi        t1, t0, 16
	ld          s3, 8(t1)
	mv          s4, x0
	bge         x0, s3, .AppendStructMembers_label_107

	// *** Basic block 3

	ld          t1, 16(t0)
	lb          s5, 80(t0)

	// *** Basic block 4

.AppendStructMembers_label_47:
	slli        t0, s4, 3
	add         t0, t1, t0
	ld          t1, 0(t0)
	ld          t1, 0(t1)
	ld          t2, 40(t1)
	lw          t1, 16(t2)
	addi        t1, t1, -2
	seqz        t2, t1

	// *** Basic block 5

.AppendStructMembers_label_64:
	mv          t0, t2
	beqz        t2, .AppendStructMembers_label_74

	// *** Basic block 6

	j           .AppendStructMembers_label_68

	// *** Basic block 7

.AppendStructMembers_label_68:
	addi        t1, t2, 32
	lb          t1, 16(t1)
	slli        t1, t1, 63
	srai        t0, t1, 63

	// *** Basic block 8

.AppendStructMembers_label_74:
	bnez        t0, .AppendStructMembers_label_103

	// *** Basic block 9

.AppendStructMembers_label_76:
	mv          a1, s1
	mv          a0, t2
	call        BuildINode

	// *** Basic block 10

	mv          s6, a0
	sd          s4, 72(s6)
	addi        a0, s1, 48
	mv          a1, s6
	call        VectorAppend

	// *** Basic block 11

	beqz        s5, .AppendStructMembers_label_94

	// *** Basic block 12

	j           .AppendStructMembers_label_102

	// *** Basic block 13

.AppendStructMembers_label_94:
	beq         s2, x0, .AppendStructMembers_label_100

	// *** Basic block 14

	sd          s6, 32(s2)

	// *** Basic block 15

.AppendStructMembers_label_100:
	mv          s2, s6

	// *** Basic block 16

.AppendStructMembers_label_102:

	// *** Basic block 17

.AppendStructMembers_label_103:
	addi        s4, s4, 1
	bge         s4, s3, .AppendStructMembers_label_47

	// *** Basic block 18

.AppendStructMembers_label_107:
	ld          t0, 48(s1)
	ld          t0, 0(t0)
	sd          t0, 40(s1)
	j           .AppendStructMembers_label_25
.func_end_AppendStructMembers:
	.size AppendStructMembers, .func_end_AppendStructMembers-AppendStructMembers

	.local  AppendArrayINodeChildren
	.type AppendArrayINodeChildren, @function

AppendArrayINodeChildren:

	// *** Basic block 0

	.global VectorLast
	.global BuildINode
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
	mv          s1, a0
	mv          s2, a1
	addi        t0, s1, 48
	ld          s3, 8(t0)
	addi        a0, s1, 48
	call        VectorLast

	// *** Basic block 1

	mv          s4, a0
	ld          t1, 40(s1)
	sub         t2, t1, s4
	snez        t0, t2
	bne         t1, s4, .AppendArrayINodeChildren_label_43

	// *** Basic block 2

	ld          t1, 8(s1)
	lw          t1, 32(t1)
	sub         t1, s3, t1
	seqz        t0, t1

	// *** Basic block 3

.AppendArrayINodeChildren_label_43:
	beqz        t0, .AppendArrayINodeChildren_label_48

	// *** Basic block 4

.AppendArrayINodeChildren_label_45:
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

	// *** Basic block 5

.AppendArrayINodeChildren_label_48:
	bnez        s3, .AppendArrayINodeChildren_label_54

	// *** Basic block 6

	li          s5, 1		// 0x1 ASCII \x1
	j           .AppendArrayINodeChildren_label_57

	// *** Basic block 7

.AppendArrayINodeChildren_label_54:
	slli        s5, s3, 1

	// *** Basic block 8

.AppendArrayINodeChildren_label_57:
	slt         t0, x0, s2
	bge         x0, s2, .AppendArrayINodeChildren_label_63

	// *** Basic block 9

	slt         t0, s5, s2

	// *** Basic block 10

.AppendArrayINodeChildren_label_63:
	beqz        t0, .AppendArrayINodeChildren_label_66

	// *** Basic block 11

	mv          s5, s2

	// *** Basic block 12

.AppendArrayINodeChildren_label_66:
	mv          s6, s3
	bge         s6, s5, .AppendArrayINodeChildren_label_100

	// *** Basic block 13

.AppendArrayINodeChildren_label_71:
	ld          t0, 8(s1)
	ld          a0, 24(t0)
	mv          a1, s1
	call        BuildINode

	// *** Basic block 14

	mv          s7, a0
	sd          s6, 72(s7)
	addi        a0, s1, 48
	mv          a1, s7
	call        VectorAppend

	// *** Basic block 15

	beq         s4, x0, .AppendArrayINodeChildren_label_94

	// *** Basic block 16

	sd          s7, 32(s4)

	// *** Basic block 17

.AppendArrayINodeChildren_label_94:
	mv          s4, s7

	// *** Basic block 18

.AppendArrayINodeChildren_label_96:
	addi        s6, s6, 1
	bge         s6, s5, .AppendArrayINodeChildren_label_71

	// *** Basic block 19

.AppendArrayINodeChildren_label_100:
	bnez        s3, .AppendArrayINodeChildren_label_107

	// *** Basic block 20

	ld          t0, 48(s1)
	ld          t0, 0(t0)
	sd          t0, 40(s1)

	// *** Basic block 21

.AppendArrayINodeChildren_label_107:
	j           .AppendArrayINodeChildren_label_45
.func_end_AppendArrayINodeChildren:
	.size AppendArrayINodeChildren, .func_end_AppendArrayINodeChildren-AppendArrayINodeChildren

	.global BuildINode
	.type BuildINode, @function

BuildINode:

	// *** Basic block 0

	.global TypeIsStructOrUnion
	.local NewINode
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
	call        TypeIsStructOrUnion

	// *** Basic block 1

	beqz        a0, .BuildINode_label_23

	// *** Basic block 2

	li          s3, 2		// 0x2 ASCII \x2
	j           .BuildINode_label_38

	// *** Basic block 3

.BuildINode_label_23:
	mv          s4, s1
	lw          t0, 16(s4)
	addi        t0, t0, -2
	seqz        s5, t0

	// *** Basic block 4

.BuildINode_label_32:
	beqz        s5, .BuildINode_label_37

	// *** Basic block 5

	j           .BuildINode_label_35

	// *** Basic block 6

.BuildINode_label_35:
	li          s3, 1		// 0x1 ASCII \x1

	// *** Basic block 7

.BuildINode_label_37:

	// *** Basic block 8

.BuildINode_label_38:
	mv          a2, s2
	mv          a1, s1
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
	j           NewINode
.func_end_BuildINode:
	.size BuildINode, .func_end_BuildINode-BuildINode

	.global LazyInitINode
	.type LazyInitINode, @function

LazyInitINode:

	// *** Basic block 0

	.local AppendArrayINodeChildren
	.local AppendStructMembers
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
	lw          t0, 0(s1)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .LazyInitINode_label_33

	// *** Basic block 2

	j           .LazyInitINode_label_20

	// *** Basic block 3

	j           .LazyInitINode_label_28

	// *** Basic block 4

.LazyInitINode_label_20:
	mv          a1, x0
	mv          a0, s1
	call        AppendArrayINodeChildren

	// *** Basic block 5

	j           .LazyInitINode_label_35

	// *** Basic block 6

.LazyInitINode_label_28:
	mv          a0, s1
	call        AppendStructMembers

	// *** Basic block 7

	j           .LazyInitINode_label_35

	// *** Basic block 8

.LazyInitINode_label_33:
	j           .LazyInitINode_label_35

	// *** Basic block 9

.LazyInitINode_label_35:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LazyInitINode:
	.size LazyInitINode, .func_end_LazyInitINode-LazyInitINode

	.local  DeleteINodeChild
	.type DeleteINodeChild, @function

DeleteINodeChild:

	// *** Basic block 0

	.global VectorDestructWithContents
	.local DeleteINodeChild
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 48
	la          t1, DeleteINodeChild
	ld          a1, 0(t1)
	j           VectorDestructWithContents
.func_end_DeleteINodeChild:
	.size DeleteINodeChild, .func_end_DeleteINodeChild-DeleteINodeChild

	.local  DeleteINode
	.type DeleteINode, @function

DeleteINode:

	// *** Basic block 0

	.global VectorDestructWithContents
	.local DeleteINodeChild
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
	addi        a0, s1, 48
	la          t0, DeleteINodeChild
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_DeleteINode:
	.size DeleteINode, .func_end_DeleteINode-DeleteINode

	.local  DoIndent
	.type DoIndent, @function

DoIndent:

	// *** Basic block 0

	.global putchar
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
	mv          s2, x0
	bge         x0, s1, .DoIndent_label_23

	// *** Basic block 1

.DoIndent_label_14:
	li          t0, 32		// 0x20 ASCII ' '
	mv          a0, t0
	call        putchar

	// *** Basic block 2

.DoIndent_label_19:
	addi        s2, s2, 1
	bge         s2, s1, .DoIndent_label_14

	// *** Basic block 3

.DoIndent_label_23:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DoIndent:
	.size DoIndent, .func_end_DoIndent-DoIndent

	.local  PrintINode
	.type PrintINode, @function

PrintINode:

	// *** Basic block 0

	.local DoIndent
	.global printf
	.local PrintINode
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
	mv          a0, s1
	call        DoIndent

	// *** Basic block 1

	addi        s3, s1, 2
	lla         a0, .str.1
	ld          a2, 24(s2)
	ld          a3, 40(s2)
	ld          a4, 16(s2)
	mv          a1, s2
	call        printf

	// *** Basic block 2

	lw          s4, 0(s2)
	slli        t0, s4, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .PrintINode_label_58

	// *** Basic block 4

	j           .PrintINode_label_65

	// *** Basic block 5

	j           .PrintINode_label_64

	// *** Basic block 6

.PrintINode_label_58:
	lla         a0, .str.2
	call        printf

	// *** Basic block 7

	j           .PrintINode_label_114

	// *** Basic block 8

.PrintINode_label_64:

	// *** Basic block 9

.PrintINode_label_65:
	lla         s5, .str.3
	li          t0, 1		// 0x1 ASCII \x1
	bne         s4, t0, .PrintINode_label_77

	// *** Basic block 10

	lla         a1, .str.4
	j           .PrintINode_label_80

	// *** Basic block 11

.PrintINode_label_77:
	lla         a1, .str.5

	// *** Basic block 12

.PrintINode_label_80:
	mv          a0, s5
	call        printf

	// *** Basic block 13

	mv          s4, x0
	addi        t0, s2, 48
	ld          s5, 8(t0)
	bge         x0, s5, .PrintINode_label_105

	// *** Basic block 14

	ld          s6, 48(s2)

	// *** Basic block 15

.PrintINode_label_93:
	slli        t0, s4, 3
	add         t0, s6, t0
	ld          a0, 0(t0)
	mv          a1, s3
	call        PrintINode

	// *** Basic block 16

.PrintINode_label_101:
	addi        s4, s4, 1
	bge         s4, s5, .PrintINode_label_93

	// *** Basic block 17

.PrintINode_label_105:
	mv          a0, s1
	call        DoIndent

	// *** Basic block 18

	lla         a0, .str.6
	call        printf

	// *** Basic block 19

	j           .PrintINode_label_114

	// *** Basic block 20

.PrintINode_label_114:
	mv          a0, s1
	call        DoIndent

	// *** Basic block 21

	lla         a0, .str.7
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
	j           printf
.func_end_PrintINode:
	.size PrintINode, .func_end_PrintINode-PrintINode

	.local  AdvanceCurrent
	.type AdvanceCurrent, @function

AdvanceCurrent:

	// *** Basic block 0

	.local AppendArrayINodeChildren
	.local AdvanceCurrent
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
	bne         s1, x0, .AdvanceCurrent_label_22

	// *** Basic block 1

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 2

.AdvanceCurrent_label_19:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.AdvanceCurrent_label_22:
	lw          s2, 0(s1)
	li          t0, 1		// 0x1 ASCII \x1
	bne         s2, t0, .AdvanceCurrent_label_35

	// *** Basic block 4

	mv          a1, x0
	mv          a0, s1
	call        AppendArrayINodeChildren

	// *** Basic block 5

.AdvanceCurrent_label_35:
	seqz        t0, s2
	beqz        s2, .AdvanceCurrent_label_45

	// *** Basic block 6

	ld          t1, 40(s1)
	ld          t1, 32(t1)
	sub         t1, t1, x0
	seqz        t0, t1

	// *** Basic block 7

.AdvanceCurrent_label_45:
	beqz        t0, .AdvanceCurrent_label_54

	// *** Basic block 8

	ld          a0, 24(s1)
	call        AdvanceCurrent

	// *** Basic block 9

	li          a0, 1		// 0x1 ASCII \x1
	j           .AdvanceCurrent_label_19

	// *** Basic block 10

.AdvanceCurrent_label_54:
	ld          t0, 40(s1)
	ld          t0, 32(t0)
	sd          t0, 40(s1)
	li          a0, 1		// 0x1 ASCII \x1
	j           .AdvanceCurrent_label_19
.func_end_AdvanceCurrent:
	.size AdvanceCurrent, .func_end_AdvanceCurrent-AdvanceCurrent

	.local  InitArrayAndAdvance
	.type InitArrayAndAdvance, @function

InitArrayAndAdvance:

	// *** Basic block 0

	.global TypeIsChar
	.global SemanticError
	.global TypeRecordCalculateSize
	.global ASTNodeMove
	.local AdvanceCurrent
	.global TypeIsInt
	.local AppendArrayINodeChildren
	.local InitCurrentAndAdvance
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
	mv          s2, a0
	mv          s3, a2
	lw          s4, 0(s1)
	li          t0, 3		// 0x3 ASCII \x3
	bne         s4, t0, .InitArrayAndAdvance_label_136

	// *** Basic block 1

	ld          s5, 8(s2)
	ld          s6, 24(s5)
	mv          a0, s6
	call        TypeIsChar

	// *** Basic block 2

	beqz        a0, .InitArrayAndAdvance_label_123

	// *** Basic block 3

	mv          s7, s1
	addi        t0, s5, 32
	lb          t0, 16(t0)
	slli        t0, t0, 63
	srai        t0, t0, 63
	not         t0, t0
	beqz        t0, .InitArrayAndAdvance_label_80

	// *** Basic block 4

	ld          t0, 56(s7)
	ld          t0, 24(t0)
	addi        s8, t0, 1
	lw          t0, 32(s5)
	addi        t0, t0, 1
	bge         t0, s8, .InitArrayAndAdvance_label_78

	// *** Basic block 5

	lla         a1, .str.8
	mv          a0, s1
	call        SemanticError

	// *** Basic block 6

.InitArrayAndAdvance_label_78:
	j           .InitArrayAndAdvance_label_99

	// *** Basic block 7

.InitArrayAndAdvance_label_80:
	ld          t0, 8(s2)
	addi        t0, t0, 32
	lb          t1, 16(t0)
	andi        t1, t1, -2
	sb          t1, 16(t0)
	ld          a0, 8(s2)
	ld          t0, 56(s7)
	ld          t0, 24(t0)
	addi        t0, t0, 1
	sw          t0, 32(a0)
	call        TypeRecordCalculateSize

	// *** Basic block 8

.InitArrayAndAdvance_label_99:
	mv          a0, s1
	call        ASTNodeMove

	// *** Basic block 9

	sd          a0, 16(s2)
	ld          t0, 16(s1)
	ld          t1, 8(s2)
	lw          t1, 20(t1)
	sw          t1, 20(t0)
	ld          a0, 24(s2)
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
	j           AdvanceCurrent

	// *** Basic block 12

.InitArrayAndAdvance_label_123:
	mv          a0, s6
	call        TypeIsInt

	// *** Basic block 13

	beqz        a0, .InitArrayAndAdvance_label_134

	// *** Basic block 14

	lla         a1, .str.9
	mv          a0, s1
	call        SemanticError

	// *** Basic block 15

.InitArrayAndAdvance_label_134:

	// *** Basic block 16

.InitArrayAndAdvance_label_135:

	// *** Basic block 17

.InitArrayAndAdvance_label_136:
	li          t0, 4		// 0x4 ASCII \x4
	bne         s4, t0, .InitArrayAndAdvance_label_234

	// *** Basic block 18

	ld          s4, 8(s2)
	ld          s5, 24(s4)
	mv          a0, s5
	call        TypeIsInt

	// *** Basic block 19

	beqz        a0, .InitArrayAndAdvance_label_221

	// *** Basic block 20

	mv          s6, s1
	addi        t0, s4, 32
	lb          t0, 16(t0)
	slli        t0, t0, 63
	srai        t0, t0, 63
	not         t0, t0
	beqz        t0, .InitArrayAndAdvance_label_178

	// *** Basic block 21

	ld          t0, 56(s6)
	ld          t0, 24(t0)
	srai        s8, t0, 2
	lw          t0, 32(s4)
	addi        t0, t0, 1
	bge         t0, s8, .InitArrayAndAdvance_label_176

	// *** Basic block 22

	lla         a1, .str.10
	mv          a0, s1
	call        SemanticError

	// *** Basic block 23

.InitArrayAndAdvance_label_176:
	j           .InitArrayAndAdvance_label_199

	// *** Basic block 24

.InitArrayAndAdvance_label_178:
	ld          t0, 8(s2)
	addi        t0, t0, 32
	lb          t1, 16(t0)
	andi        t1, t1, -2
	sb          t1, 16(t0)
	ld          a0, 8(s2)
	ld          t0, 56(s6)
	ld          t0, 24(t0)
	srai        t0, t0, 2
	sext.w      t0, t0
	addi        t0, t0, 1
	sw          t0, 32(a0)
	call        TypeRecordCalculateSize

	// *** Basic block 25

.InitArrayAndAdvance_label_199:
	mv          a0, s1
	call        ASTNodeMove

	// *** Basic block 26

	sd          a0, 16(s2)
	ld          t0, 16(s1)
	ld          t1, 8(s2)
	lw          t1, 20(t1)
	sw          t1, 20(t0)
	ld          a0, 24(s2)
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
	j           AdvanceCurrent

	// *** Basic block 28

.InitArrayAndAdvance_label_221:
	mv          a0, s5
	call        TypeIsChar

	// *** Basic block 29

	beqz        a0, .InitArrayAndAdvance_label_232

	// *** Basic block 30

	lla         a1, .str.11
	mv          a0, s1
	call        SemanticError

	// *** Basic block 31

.InitArrayAndAdvance_label_232:

	// *** Basic block 32

.InitArrayAndAdvance_label_233:

	// *** Basic block 33

.InitArrayAndAdvance_label_234:
	mv          a1, x0
	mv          a0, s2
	call        AppendArrayINodeChildren

	// *** Basic block 34

	ld          a0, 40(s2)
	mv          a2, s3
	mv          a1, s1
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
	j           InitCurrentAndAdvance
.func_end_InitArrayAndAdvance:
	.size InitArrayAndAdvance, .func_end_InitArrayAndAdvance-InitArrayAndAdvance

	.local  InitCurrentAndAdvance
	.type InitCurrentAndAdvance, @function

InitCurrentAndAdvance:

	// *** Basic block 0

	.global AnalyzeExpression
	.global IsConstantExpression
	.global SemanticError
	.global ASTNodeMove
	.global printf
	.global abort
	.local AdvanceCurrent
	.local InitArrayAndAdvance
	.global TypeEqual
	.local AppendStructMembers
	.local InitCurrentAndAdvance
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
	mv          s3, a2
	ld          t0, 16(s1)
	beq         t0, x0, .InitCurrentAndAdvance_label_46

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.InitCurrentAndAdvance_label_43:
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

.InitCurrentAndAdvance_label_46:
	mv          a0, s2
	call        AnalyzeExpression

	// *** Basic block 4

	mv          s2, a0
	lw          t0, 0(s1)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 5

	j           .InitCurrentAndAdvance_label_59

	// *** Basic block 6

	j           .InitCurrentAndAdvance_label_113

	// *** Basic block 7

	j           .InitCurrentAndAdvance_label_125

	// *** Basic block 8

.InitCurrentAndAdvance_label_59:
	mv          s4, s3
	beqz        s3, .InitCurrentAndAdvance_label_67

	// *** Basic block 9

	mv          a0, s2
	call        IsConstantExpression

	// *** Basic block 10

	not         s4, a0

	// *** Basic block 11

.InitCurrentAndAdvance_label_67:
	beqz        s4, .InitCurrentAndAdvance_label_75

	// *** Basic block 12

	lla         a1, .str.12
	mv          a0, s2
	call        SemanticError

	// *** Basic block 13

.InitCurrentAndAdvance_label_75:
	mv          a0, s2
	call        ASTNodeMove

	// *** Basic block 14

	sd          a0, 16(s1)
	ld          t0, 16(s1)
	lw          t0, 0(t0)
	li          t1, 88		// 0x58 ASCII 'X'
	beq         t0, t1, .InitCurrentAndAdvance_label_89

	// *** Basic block 15

	j           .InitCurrentAndAdvance_label_105

	// *** Basic block 16

.InitCurrentAndAdvance_label_89:
	lla         a0, .str.13
	lla         a1, .str.14
	lla         a3, .str.15
	li          t0, 292		// 0x124
	mv          a2, t0
	call        printf

	// *** Basic block 17

	call        abort

	// *** Basic block 18

.InitCurrentAndAdvance_label_105:
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AdvanceCurrent

	// *** Basic block 20

.InitCurrentAndAdvance_label_113:
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           InitArrayAndAdvance

	// *** Basic block 22

.InitCurrentAndAdvance_label_125:
	ld          a0, 8(s1)
	ld          a1, 16(s2)
	call        TypeEqual

	// *** Basic block 23

	beqz        a0, .InitCurrentAndAdvance_label_158

	// *** Basic block 24

	beqz        s3, .InitCurrentAndAdvance_label_144

	// *** Basic block 25

	lla         a1, .str.16
	mv          a0, s2
	call        SemanticError

	// *** Basic block 26

	li          a0, 1		// 0x1 ASCII \x1
	j           .InitCurrentAndAdvance_label_43

	// *** Basic block 27

.InitCurrentAndAdvance_label_144:
	mv          a0, s2
	call        ASTNodeMove

	// *** Basic block 28

	sd          a0, 16(s1)
	ld          a0, 24(s1)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AdvanceCurrent

	// *** Basic block 30

.InitCurrentAndAdvance_label_158:
	mv          a0, s1
	call        AppendStructMembers

	// *** Basic block 31

	ld          a0, 40(s1)
	mv          a2, s3
	mv          a1, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           InitCurrentAndAdvance
.func_end_InitCurrentAndAdvance:
	.size InitCurrentAndAdvance, .func_end_InitCurrentAndAdvance-InitCurrentAndAdvance

	.local  GetChildAtIndex
	.type GetChildAtIndex, @function

GetChildAtIndex:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	addi        t1, a0, 48
	ld          t1, 8(t1)
	blt         t0, t1, .GetChildAtIndex_label_23

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.GetChildAtIndex_label_20:
	ret         

	// *** Basic block 3

.GetChildAtIndex_label_23:
	ld          t1, 48(a0)
	slli        t2, t0, 3
	add         t1, t1, t2
	ld          a0, 0(t1)
	ret         
.func_end_GetChildAtIndex:
	.size GetChildAtIndex, .func_end_GetChildAtIndex-GetChildAtIndex

	.local  FindDesignator
	.type FindDesignator, @function

FindDesignator:

	// *** Basic block 0

	.global SemanticError
	.local AppendArrayINodeChildren
	.local GetChildAtIndex
	.local AppendStructMembers
	.global FindStructMember
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
	lw          t0, 0(s1)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .FindDesignator_label_39

	// *** Basic block 2

	j           .FindDesignator_label_90

	// *** Basic block 3

.FindDesignator_label_39:
	lw          t0, 0(s2)
	li          t1, 1		// 0x1 ASCII \x1
	beq         t0, t1, .FindDesignator_label_57

	// *** Basic block 4

	lla         a1, .str.17
	mv          a0, s3
	call        SemanticError

	// *** Basic block 5

	mv          a0, x0

	// *** Basic block 6

.FindDesignator_label_54:
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

	// *** Basic block 7

.FindDesignator_label_57:
	lw          s4, 16(s1)
	mv          a1, s4
	mv          a0, s2
	call        AppendArrayINodeChildren

	// *** Basic block 8

	mv          a1, s4
	mv          a0, s2
	call        GetChildAtIndex

	// *** Basic block 9

	mv          s5, a0
	bne         s5, x0, .FindDesignator_label_86

	// *** Basic block 10

	lla         a1, .str.18
	mv          a2, s4
	mv          a0, s3
	call        SemanticError

	// *** Basic block 11

	mv          a0, x0
	j           .FindDesignator_label_54

	// *** Basic block 12

.FindDesignator_label_86:
	mv          a0, s5
	j           .FindDesignator_label_54

	// *** Basic block 13

.FindDesignator_label_90:
	lw          t0, 0(s2)
	li          t1, 2		// 0x2 ASCII \x2
	beq         t0, t1, .FindDesignator_label_106

	// *** Basic block 14

	lla         a1, .str.19
	mv          a0, s3
	call        SemanticError

	// *** Basic block 15

	mv          a0, x0
	j           .FindDesignator_label_54

	// *** Basic block 16

.FindDesignator_label_106:
	mv          a0, s2
	call        AppendStructMembers

	// *** Basic block 17

	ld          t0, 8(s2)
	ld          a0, 32(t0)
	ld          s4, 16(s1)
	mv          a1, s4
	call        FindStructMember

	// *** Basic block 18

	mv          s6, a0
	bne         s6, x0, .FindDesignator_label_137

	// *** Basic block 19

	lla         a1, .str.20
	ld          a2, 16(s4)
	mv          a0, s3
	call        SemanticError

	// *** Basic block 20

	mv          a0, x0
	j           .FindDesignator_label_54

	// *** Basic block 21

.FindDesignator_label_137:
	ld          t1, 0(s6)
	ld          s4, 40(t1)
	lw          t1, 16(s4)
	addi        t1, t1, -2
	seqz        s4, t1

	// *** Basic block 22

.FindDesignator_label_148:
	mv          t0, s4
	beqz        s4, .FindDesignator_label_158

	// *** Basic block 23

	j           .FindDesignator_label_152

	// *** Basic block 24

.FindDesignator_label_152:
	addi        t1, s4, 32
	lb          t1, 16(t1)
	slli        t1, t1, 63
	srai        t0, t1, 63

	// *** Basic block 25

.FindDesignator_label_158:
	beqz        t0, .FindDesignator_label_169

	// *** Basic block 26

	lla         a1, .str.21
	mv          a0, s3
	call        SemanticError

	// *** Basic block 27

	mv          a0, x0
	j           .FindDesignator_label_54

	// *** Basic block 28

.FindDesignator_label_169:
	ld          a1, 24(s6)
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
	j           GetChildAtIndex
.func_end_FindDesignator:
	.size FindDesignator, .func_end_FindDesignator-FindDesignator

	.local  GetArraySizeFromInitializer
	.type GetArraySizeFromInitializer, @function

GetArraySizeFromInitializer:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, x0
	mv          t1, x0
	ld          t2, 56(a0)
	ld          t3, 8(t2)
	bge         x0, t3, .GetArraySizeFromInitializer_label_64

	// *** Basic block 1

	ld          t2, 0(t2)

	// *** Basic block 2

.GetArraySizeFromInitializer_label_25:
	slli        t4, t1, 3
	add         t2, t2, t4
	ld          t4, 0(t2)
	lw          t2, 0(t4)
	li          t5, 89		// 0x59 ASCII 'Y'
	bne         t2, t5, .GetArraySizeFromInitializer_label_57

	// *** Basic block 3

	mv          t2, t4
	ld          t4, 56(t2)
	ld          t4, 0(t4)
	ld          t5, 0(t4)
	lw          t4, 0(t5)
	bnez        t4, .GetArraySizeFromInitializer_label_55

	// *** Basic block 4

	lw          t2, 16(t5)
	addi        t4, t2, 1
	bge         t0, t4, .GetArraySizeFromInitializer_label_54

	// *** Basic block 5

	mv          t0, t4

	// *** Basic block 6

.GetArraySizeFromInitializer_label_54:

	// *** Basic block 7

.GetArraySizeFromInitializer_label_55:
	j           .GetArraySizeFromInitializer_label_59

	// *** Basic block 8

.GetArraySizeFromInitializer_label_57:
	addi        t0, t0, 1

	// *** Basic block 9

.GetArraySizeFromInitializer_label_59:

	// *** Basic block 10

.GetArraySizeFromInitializer_label_60:
	addi        t1, t1, 1
	bge         t1, t3, .GetArraySizeFromInitializer_label_25

	// *** Basic block 11

.GetArraySizeFromInitializer_label_64:
	mv          a0, t0

	// *** Basic block 12

.GetArraySizeFromInitializer_label_67:
	ret         
.func_end_GetArraySizeFromInitializer:
	.size GetArraySizeFromInitializer, .func_end_GetArraySizeFromInitializer-GetArraySizeFromInitializer

	.local  InitializeINode
	.type InitializeINode, @function

InitializeINode:

	// *** Basic block 0

	.local InitCurrentAndAdvance
	.local GetArraySizeFromInitializer
	.global TypeRecordCalculateSize
	.global LazyInitINode
	.local InitializeINode
	.global SemanticError
	.local AdvanceCurrent
	.local FindDesignator
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -16(s0)
	// Spilled register region: 8 bytes at -24(s0) to -16(s0)
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
	mv          s3, a2

	// *** Basic block 1

.InitializeINode_label_36:
	lw          t0, 0(s1)
	li          t1, 87		// 0x57 ASCII 'W'
	blt         t0, t1, .InitializeINode_label_228

	// *** Basic block 2

	li          t1, 89		// 0x59 ASCII 'Y'
	blt         t1, t0, .InitializeINode_label_228

	// *** Basic block 3

	addi        t0, t0, -87
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 4

	j           .InitializeINode_label_56

	// *** Basic block 5

	j           .InitializeINode_label_73

	// *** Basic block 6

	j           .InitializeINode_label_172

	// *** Basic block 7

.InitializeINode_label_56:
	mv          s4, s1
	sd          s4, -24(s0)	// Spilled @57
	ld          a1, 56(s4)
	mv          a2, s3
	mv          a0, s2
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
	j           InitCurrentAndAdvance

	// *** Basic block 9

.InitializeINode_label_70:
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

	// *** Basic block 10

.InitializeINode_label_73:
	mv          s5, s1
	lw          t1, 0(s2)
	addi        t2, t1, -1
	seqz        t0, t2
	li          t2, 1		// 0x1 ASCII \x1
	bne         t1, t2, .InitializeINode_label_90

	// *** Basic block 11

	ld          t1, 8(s2)
	addi        t1, t1, 32
	lb          t1, 16(t1)
	slli        t1, t1, 63
	srai        t0, t1, 63

	// *** Basic block 12

.InitializeINode_label_90:
	beqz        t0, .InitializeINode_label_108

	// *** Basic block 13

	ld          s6, 8(s2)
	mv          a0, s5
	call        GetArraySizeFromInitializer

	// *** Basic block 14

	sw          a0, 32(s6)
	ld          a0, 8(s2)
	addi        t0, a0, 32
	lb          t1, 16(t0)
	andi        t1, t1, -2
	sb          t1, 16(t0)
	call        TypeRecordCalculateSize

	// *** Basic block 15

.InitializeINode_label_108:
	mv          a0, s2
	call        LazyInitINode

	// *** Basic block 16

	ld          s6, 24(s2)
	sd          x0, 24(s2)
	mv          s7, x0
	ld          t0, 56(s5)
	ld          s8, 8(t0)
	bge         x0, s8, .InitializeINode_label_162

	// *** Basic block 17

	ld          s9, 40(s2)
	ld          s10, 0(t0)

	// *** Basic block 18

.InitializeINode_label_130:
	bne         s9, x0, .InitializeINode_label_135

	// *** Basic block 19

	j           .InitializeINode_label_136

	// *** Basic block 20

.InitializeINode_label_135:

	// *** Basic block 21

.InitializeINode_label_136:
	slli        t0, s7, 3
	add         t0, s10, t0
	ld          a0, 0(t0)
	mv          a2, s3
	mv          a1, a0
	mv          a0, s9
	call        InitializeINode

	// *** Basic block 22

	not         t0, a0
	beqz        t0, .InitializeINode_label_157

	// *** Basic block 23

	lla         a1, .str.22
	call        SemanticError

	// *** Basic block 24

	j           .InitializeINode_label_162

	// *** Basic block 25

.InitializeINode_label_157:

	// *** Basic block 26

.InitializeINode_label_158:
	addi        s7, s7, 1
	bge         s7, s8, .InitializeINode_label_130

	// *** Basic block 27

.InitializeINode_label_162:
	sd          s6, 24(s2)
	mv          a0, s6
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
	j           AdvanceCurrent

	// *** Basic block 29

.InitializeINode_label_172:
	mv          s6, s1
	ld          s8, 24(s2)
	mv          a0, s2
	call        LazyInitINode

	// *** Basic block 30

	mv          s9, x0
	ld          t0, 56(s6)
	ld          s10, 8(t0)
	bge         x0, s10, .InitializeINode_label_212

	// *** Basic block 31

	ld          s11, 0(t0)

	// *** Basic block 32

.InitializeINode_label_190:
	slli        t0, s9, 3
	add         t0, s11, t0
	ld          a2, 0(t0)
	mv          a1, s1
	mv          a0, s8
	call        FindDesignator

	// *** Basic block 33

	mv          s8, a0
	bne         s8, x0, .InitializeINode_label_207

	// *** Basic block 34

	mv          a0, x0
	j           .InitializeINode_label_70

	// *** Basic block 35

.InitializeINode_label_207:

	// *** Basic block 36

.InitializeINode_label_208:
	addi        s9, s9, 1
	bge         s9, s10, .InitializeINode_label_190

	// *** Basic block 37

.InitializeINode_label_212:
	ld          t0, 24(s8)
	sd          s8, 40(t0)
	mv          s10, s8
	ld          s11, 64(s6)
	mv          s4, s3
	mv          s2, s10
	mv          s1, s11
	mv          s3, s4
	j           .InitializeINode_label_36

	// *** Basic block 38

.InitializeINode_label_228:
	mv          a0, x0
	j           .InitializeINode_label_70
.func_end_InitializeINode:
	.size InitializeINode, .func_end_InitializeINode-InitializeINode

	.local  BuildSingleDesignator
	.type BuildSingleDesignator, @function

BuildSingleDesignator:

	// *** Basic block 0

	.global VectorAppend
	.global NewArrayDesignator
	.global NewStructMemberDesignator
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
	mv          s2, a0
	mv          s3, a2
	slli        t0, a1, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .BuildSingleDesignator_label_65

	// *** Basic block 2

	j           .BuildSingleDesignator_label_32

	// *** Basic block 3

	j           .BuildSingleDesignator_label_46

	// *** Basic block 4

.BuildSingleDesignator_label_32:
	ld          a0, 8(s2)
	ld          a1, 72(s2)
	call        NewArrayDesignator

	// *** Basic block 5

	mv          a1, a0
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 6

	j           .BuildSingleDesignator_label_67

	// *** Basic block 7

.BuildSingleDesignator_label_46:
	ld          t0, 32(s3)
	ld          t0, 16(t0)
	ld          t1, 72(s2)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          a0, 0(t0)
	call        NewStructMemberDesignator

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 9

	j           .BuildSingleDesignator_label_67

	// *** Basic block 10

.BuildSingleDesignator_label_65:
	j           .BuildSingleDesignator_label_67

	// *** Basic block 11

.BuildSingleDesignator_label_67:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BuildSingleDesignator:
	.size BuildSingleDesignator, .func_end_BuildSingleDesignator-BuildSingleDesignator

	.local  BuildDesignator
	.type BuildDesignator, @function

BuildDesignator:

	// *** Basic block 0

	.local BuildDesignator
	.local BuildSingleDesignator
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
	ld          s3, 24(s1)
	bne         s3, x0, .BuildDesignator_label_22

	// *** Basic block 1

.BuildDesignator_label_19:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.BuildDesignator_label_22:
	mv          a1, s2
	mv          a0, s3
	call        BuildDesignator

	// *** Basic block 3

	lw          a1, 0(s3)
	ld          a2, 8(s3)
	mv          a3, s2
	mv          a0, s1
	call        BuildSingleDesignator

	// *** Basic block 4

	j           .BuildDesignator_label_19
.func_end_BuildDesignator:
	.size BuildDesignator, .func_end_BuildDesignator-BuildDesignator

	.local  BuildDesignatedInitializer
	.type BuildDesignatedInitializer, @function

BuildDesignatedInitializer:

	// *** Basic block 0

	.global NewVector
	.local BuildSingleDesignator
	.local BuildDesignator
	.global AnalyzeExpression
	.global NewDesignatedInitializerASTNode
	.global NormalConversion
	.global ASTNodeSetType
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
	call        NewVector

	// *** Basic block 1

	mv          s2, a0
	ld          t0, 24(s1)
	bne         t0, x0, .BuildDesignatedInitializer_label_40

	// *** Basic block 2

	lw          a1, 0(s1)
	ld          a2, 8(s1)
	mv          a3, s2
	mv          a0, s1
	call        BuildSingleDesignator

	// *** Basic block 3

	j           .BuildDesignatedInitializer_label_46

	// *** Basic block 4

.BuildDesignatedInitializer_label_40:
	mv          a1, s2
	mv          a0, s1
	call        BuildDesignator

	// *** Basic block 5

.BuildDesignatedInitializer_label_46:
	ld          a0, 16(s1)
	call        AnalyzeExpression

	// *** Basic block 6

	sd          a0, 16(s1)
	ld          s3, 16(s1)
	ld          a2, 40(s3)
	mv          a1, s3
	mv          a0, s2
	call        NewDesignatedInitializerASTNode

	// *** Basic block 7

	mv          s4, a0
	ld          s5, 8(s1)
	mv          a1, s5
	mv          a0, s3
	call        NormalConversion

	// *** Basic block 8

	mv          a1, s5
	mv          a0, s4
	call        ASTNodeSetType

	// *** Basic block 9

	mv          a0, s4

	// *** Basic block 10

.BuildDesignatedInitializer_label_78:
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
.func_end_BuildDesignatedInitializer:
	.size BuildDesignatedInitializer, .func_end_BuildDesignatedInitializer-BuildDesignatedInitializer

	.local  FlattenINode
	.type FlattenINode, @function

FlattenINode:

	// *** Basic block 0

	.global VectorAppend
	.local BuildDesignatedInitializer
	.local FlattenINode
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
	ld          t0, 16(s1)
	beq         t0, x0, .FlattenINode_label_34

	// *** Basic block 1

	ld          s3, 56(s2)
	mv          a0, s1
	call        BuildDesignatedInitializer

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s3
	call        VectorAppend

	// *** Basic block 3

.FlattenINode_label_34:
	mv          s3, x0
	addi        t0, s1, 48
	ld          s4, 8(t0)
	bge         x0, s4, .FlattenINode_label_56

	// *** Basic block 4

	ld          s5, 48(s1)

	// *** Basic block 5

.FlattenINode_label_43:
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          a0, 0(t0)
	mv          a1, s2
	call        FlattenINode

	// *** Basic block 6

.FlattenINode_label_52:
	addi        s3, s3, 1
	bge         s3, s4, .FlattenINode_label_43

	// *** Basic block 7

.FlattenINode_label_56:
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
.func_end_FlattenINode:
	.size FlattenINode, .func_end_FlattenINode-FlattenINode

	.global AnalyzeInitializer
	.type AnalyzeInitializer, @function

AnalyzeInitializer:

	// *** Basic block 0

	.global BuildINode
	.local InitializeINode
	.global NewBracedInitializerASTNode
	.global NewVector
	.local FlattenINode
	.local DeleteINode
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
	mv          a1, x0
	call        BuildINode

	// *** Basic block 1

	mv          s3, a0
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        InitializeINode

	// *** Basic block 2

	call        NewVector

	// *** Basic block 3

	ld          a1, 40(s1)
	call        NewBracedInitializerASTNode

	// *** Basic block 4

	mv          s4, a0
	mv          a1, s4
	mv          a0, s3
	call        FlattenINode

	// *** Basic block 5

	mv          a0, s3
	call        DeleteINode

	// *** Basic block 6

	mv          a0, s4

	// *** Basic block 7

.AnalyzeInitializer_label_53:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AnalyzeInitializer:
	.size AnalyzeInitializer, .func_end_AnalyzeInitializer-AnalyzeInitializer

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%p: parent: %p current: %p expr: %p "
	.type .str.1, @object
	.size .str.1, 37

.str.2:
	.asciz "scalar\n"
	.type .str.2, @object
	.size .str.2, 8

.str.3:
	.asciz "%s {\n"
	.type .str.3, @object
	.size .str.3, 6

.str.4:
	.asciz "array"
	.type .str.4, @object
	.size .str.4, 6

.str.5:
	.asciz "struct"
	.type .str.5, @object
	.size .str.5, 7

.str.6:
	.asciz "}\n"
	.type .str.6, @object
	.size .str.6, 3

.str.7:
	.asciz "}\n"
	.type .str.7, @object
	.size .str.7, 3

.str.8:
	.asciz "Too many initializers for character array"
	.type .str.8, @object
	.size .str.8, 42

.str.9:
	.asciz "Initializing a wide-char array with a non-wide string literal"
	.type .str.9, @object
	.size .str.9, 62

.str.10:
	.asciz "Too many initializers for wide character array"
	.type .str.10, @object
	.size .str.10, 47

.str.11:
	.asciz "Initializing a char array with a wide string literal"
	.type .str.11, @object
	.size .str.11, 53

.str.12:
	.asciz "Expression is not a compile-time constant"
	.type .str.12, @object
	.size .str.12, 42

.str.13:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.13, @object
	.size .str.13, 30

.str.14:
	.asciz "(null)"
	.type .str.14, @object
	.size .str.14, 1

.str.15:
	.asciz "inode->expr->op != AST_OP(braced_init)"
	.type .str.15, @object
	.size .str.15, 39

.str.16:
	.asciz "Expression is not a compile-time constant"
	.type .str.16, @object
	.size .str.16, 42

.str.17:
	.asciz "Use of array designator on a non-array"
	.type .str.17, @object
	.size .str.17, 39

.str.18:
	.asciz "Array designator [%d] is outside bounds of the array"
	.type .str.18, @object
	.size .str.18, 53

.str.19:
	.asciz "Use of struct designator on a non-struct"
	.type .str.19, @object
	.size .str.19, 41

.str.20:
	.asciz "Unknown struct member %s used in designator"
	.type .str.20, @object
	.size .str.20, 44

.str.21:
	.asciz "Use of flexible array member in designator"
	.type .str.21, @object
	.size .str.21, 43

.str.22:
	.asciz "Too many initializers"
	.type .str.22, @object
	.size .str.22, 22

