	.file   "binary_tree.c"
	.text
	.option pic
.PCbegin:
	.global BinaryTreeNodeInit
	.type BinaryTreeNodeInit, @function

BinaryTreeNodeInit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sw          x0, 0(a0)
	sd          x0, 8(a0)
	sd          x0, 16(a0)
	sd          x0, 24(a0)
	sw          x0, 32(a0)
	ret         
.func_end_BinaryTreeNodeInit:
	.size BinaryTreeNodeInit, .func_end_BinaryTreeNodeInit-BinaryTreeNodeInit

	.local  RotateLeft
	.type RotateLeft, @function

RotateLeft:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	ld          t2, 16(t0)
	ld          t3, 8(t2)
	sd          t3, 16(t0)
	beq         t3, x0, .RotateLeft_label_27

	// *** Basic block 1

	ld          t3, 8(t2)
	sd          t0, 24(t3)

	// *** Basic block 2

.RotateLeft_label_27:
	ld          t3, 24(t0)
	sd          t3, 24(t2)
	bne         t3, x0, .RotateLeft_label_37

	// *** Basic block 3

	sd          t2, 0(t1)
	j           .RotateLeft_label_54

	// *** Basic block 4

.RotateLeft_label_37:
	ld          t3, 8(t3)
	bne         t0, t3, .RotateLeft_label_48

	// *** Basic block 5

	ld          t3, 24(t0)
	sd          t2, 8(t3)
	j           .RotateLeft_label_53

	// *** Basic block 6

.RotateLeft_label_48:
	ld          t3, 24(t0)
	sd          t2, 16(t3)

	// *** Basic block 7

.RotateLeft_label_53:

	// *** Basic block 8

.RotateLeft_label_54:
	sd          t0, 8(t2)
	sd          t2, 24(t0)
	ret         
.func_end_RotateLeft:
	.size RotateLeft, .func_end_RotateLeft-RotateLeft

	.local  RotateRight
	.type RotateRight, @function

RotateRight:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	ld          t2, 8(t0)
	ld          t3, 16(t2)
	sd          t3, 8(t0)
	beq         t3, x0, .RotateRight_label_27

	// *** Basic block 1

	ld          t3, 16(t2)
	sd          t0, 24(t3)

	// *** Basic block 2

.RotateRight_label_27:
	ld          t3, 24(t0)
	sd          t3, 24(t2)
	bne         t3, x0, .RotateRight_label_37

	// *** Basic block 3

	sd          t2, 0(t1)
	j           .RotateRight_label_54

	// *** Basic block 4

.RotateRight_label_37:
	ld          t3, 8(t3)
	bne         t0, t3, .RotateRight_label_48

	// *** Basic block 5

	ld          t3, 24(t0)
	sd          t2, 8(t3)
	j           .RotateRight_label_53

	// *** Basic block 6

.RotateRight_label_48:
	ld          t3, 24(t0)
	sd          t2, 16(t3)

	// *** Basic block 7

.RotateRight_label_53:

	// *** Basic block 8

.RotateRight_label_54:
	sd          t0, 16(t2)
	sd          t2, 24(t0)
	ret         
.func_end_RotateRight:
	.size RotateRight, .func_end_RotateRight-RotateRight

	.local  InsertNode
	.type InsertNode, @function

InsertNode:

	// *** Basic block 0

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
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	mv          s4, a3

	// *** Basic block 1

.InsertNode_label_20:

	// *** Basic block 2

.InsertNode_label_21:
	ld          s5, 0(s1)
	bne         s5, x0, .InsertNode_label_34

	// *** Basic block 3

	sd          s2, 0(s1)
	sd          s3, 24(s2)
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 4

.InsertNode_label_31:
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

	// *** Basic block 5

.InsertNode_label_34:
	mv          a1, s2
	mv          a0, s5
	jalr         x1, s4, 0

	// *** Basic block 6

	mv          s6, a0
	bnez        s6, .InsertNode_label_47

	// *** Basic block 7

	mv          a0, x0
	j           .InsertNode_label_31

	// *** Basic block 8

.InsertNode_label_47:
	bge         x0, s6, .InsertNode_label_62

	// *** Basic block 9

	addi        s6, s5, 8
	mv          s7, s2
	mv          s8, s4
	mv          s1, s6
	mv          s2, s7
	mv          s3, s5
	mv          s4, s8
	j           .InsertNode_label_21

	// *** Basic block 10

.InsertNode_label_62:
	addi        s6, s5, 16
	mv          s9, s2
	mv          s10, s5
	mv          s5, s4
	mv          s1, s6
	mv          s2, s9
	mv          s3, s10
	mv          s4, s5
	j           .InsertNode_label_20
.func_end_InsertNode:
	.size InsertNode, .func_end_InsertNode-InsertNode

	.local  BinaryTreeNodeInsert
	.type BinaryTreeNodeInsert, @function

BinaryTreeNodeInsert:

	// *** Basic block 0

	.local InsertNode
	.local RotateLeft
	.local RotateRight
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
	mv          t0, a2
	mv          a3, t0
	mv          a2, x0
	call        InsertNode

	// *** Basic block 1

	mv          s3, a0
	not         t0, s3
	beqz        t0, .BinaryTreeNodeInsert_label_39

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.BinaryTreeNodeInsert_label_36:
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

	// *** Basic block 4

.BinaryTreeNodeInsert_label_39:
	mv          s3, s2
	ld          s4, 0(s1)
	sub         t1, s3, s4
	snez        t0, t1
	beq         s3, s4, .BinaryTreeNodeInsert_label_51

	// *** Basic block 5

	ld          t1, 24(s3)
	lw          t1, 0(t1)
	seqz        t0, t1

	// *** Basic block 6

.BinaryTreeNodeInsert_label_51:
	beqz        t0, .BinaryTreeNodeInsert_label_173

	// *** Basic block 7

.BinaryTreeNodeInsert_label_53:
	ld          s3, 24(s3)
	ld          s2, 24(s3)
	ld          s5, 8(s2)
	bne         s3, s5, .BinaryTreeNodeInsert_label_113

	// *** Basic block 8

	ld          s6, 16(s2)
	sub         t1, s6, x0
	snez        t0, t1
	beq         s6, x0, .BinaryTreeNodeInsert_label_72

	// *** Basic block 9

	lw          t1, 0(s6)
	seqz        t0, t1

	// *** Basic block 10

.BinaryTreeNodeInsert_label_72:
	beqz        t0, .BinaryTreeNodeInsert_label_86

	// *** Basic block 11

	ld          t0, 24(s3)
	li          t1, 1		// 0x1 ASCII \x1
	sw          t1, 0(t0)
	sw          t1, 0(s6)
	ld          t0, 24(s3)
	ld          s3, 24(t0)
	sw          x0, 0(s3)
	j           .BinaryTreeNodeInsert_label_111

	// *** Basic block 12

.BinaryTreeNodeInsert_label_86:
	ld          t0, 16(s3)
	bne         s3, t0, .BinaryTreeNodeInsert_label_97

	// *** Basic block 13

	mv          a1, s3
	mv          a0, s1
	call        RotateLeft

	// *** Basic block 14

.BinaryTreeNodeInsert_label_97:
	ld          t0, 24(s3)
	li          t1, 1		// 0x1 ASCII \x1
	sw          t1, 0(t0)
	ld          t0, 24(s3)
	ld          a1, 24(t0)
	sw          x0, 0(a1)
	mv          a0, s1
	call        RotateRight

	// *** Basic block 15

.BinaryTreeNodeInsert_label_111:
	j           .BinaryTreeNodeInsert_label_162

	// *** Basic block 16

.BinaryTreeNodeInsert_label_113:
	sub         t1, s5, x0
	snez        t0, t1
	beq         s5, x0, .BinaryTreeNodeInsert_label_121

	// *** Basic block 17

	lw          t1, 0(s5)
	seqz        t0, t1

	// *** Basic block 18

.BinaryTreeNodeInsert_label_121:
	beqz        t0, .BinaryTreeNodeInsert_label_135

	// *** Basic block 19

	ld          t0, 24(s3)
	li          t1, 1		// 0x1 ASCII \x1
	sw          t1, 0(t0)
	sw          t1, 0(s5)
	ld          t0, 24(s3)
	ld          s3, 24(t0)
	sw          x0, 0(s3)
	j           .BinaryTreeNodeInsert_label_161

	// *** Basic block 20

.BinaryTreeNodeInsert_label_135:
	ld          t0, 8(s3)
	bne         s3, t0, .BinaryTreeNodeInsert_label_147

	// *** Basic block 21

	mv          a1, s3
	mv          a0, s1
	call        RotateRight

	// *** Basic block 22

.BinaryTreeNodeInsert_label_147:
	ld          t0, 24(s3)
	li          t1, 1		// 0x1 ASCII \x1
	sw          t1, 0(t0)
	ld          t0, 24(s3)
	ld          a1, 24(t0)
	sw          x0, 0(a1)
	mv          a0, s1
	call        RotateLeft

	// *** Basic block 23

.BinaryTreeNodeInsert_label_161:

	// *** Basic block 24

.BinaryTreeNodeInsert_label_162:
	sub         t1, s3, s4
	snez        t0, t1
	beq         s3, s4, .BinaryTreeNodeInsert_label_171

	// *** Basic block 25

	ld          t1, 24(s3)
	lw          t1, 0(t1)
	seqz        t0, t1

	// *** Basic block 26

.BinaryTreeNodeInsert_label_171:
	bnez        t0, .BinaryTreeNodeInsert_label_53

	// *** Basic block 27

.BinaryTreeNodeInsert_label_173:
	ld          t0, 0(s1)
	li          t1, 1		// 0x1 ASCII \x1
	sw          t1, 0(t0)
	li          a0, 1		// 0x1 ASCII \x1
	j           .BinaryTreeNodeInsert_label_36
.func_end_BinaryTreeNodeInsert:
	.size BinaryTreeNodeInsert, .func_end_BinaryTreeNodeInsert-BinaryTreeNodeInsert

	.local  BinaryTreeNodeSearch
	.type BinaryTreeNodeSearch, @function

BinaryTreeNodeSearch:

	// *** Basic block 0

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
	beq         s1, x0, .BinaryTreeNodeSearch_label_41

	// *** Basic block 1

.BinaryTreeNodeSearch_label_18:
	mv          a1, s3
	mv          a0, s1
	jalr         x1, s2, 0

	// *** Basic block 2

	mv          s4, a0
	beqz        s4, .BinaryTreeNodeSearch_label_41

	// *** Basic block 3

.BinaryTreeNodeSearch_label_28:
	bge         x0, s4, .BinaryTreeNodeSearch_label_34

	// *** Basic block 4

	ld          s1, 8(s1)
	j           .BinaryTreeNodeSearch_label_37

	// *** Basic block 5

.BinaryTreeNodeSearch_label_34:
	ld          s1, 16(s1)

	// *** Basic block 6

.BinaryTreeNodeSearch_label_37:
	bne         s1, x0, .BinaryTreeNodeSearch_label_18

	// *** Basic block 7

.BinaryTreeNodeSearch_label_41:
	mv          a0, s1

	// *** Basic block 8

.BinaryTreeNodeSearch_label_44:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BinaryTreeNodeSearch:
	.size BinaryTreeNodeSearch, .func_end_BinaryTreeNodeSearch-BinaryTreeNodeSearch

	.local  BinaryTreeNodeDelete
	.type BinaryTreeNodeDelete, @function

BinaryTreeNodeDelete:

	// *** Basic block 0

	.local BinaryTreeNodeDelete
	.global free
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
	mv          s3, a2
	bne         s1, x0, .BinaryTreeNodeDelete_label_23

	// *** Basic block 1

.BinaryTreeNodeDelete_label_20:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.BinaryTreeNodeDelete_label_23:
	ld          a0, 8(s1)
	mv          a2, s3
	mv          a1, s2
	call        BinaryTreeNodeDelete

	// *** Basic block 3

	ld          a0, 16(s1)
	mv          a2, s3
	mv          a1, s2
	call        BinaryTreeNodeDelete

	// *** Basic block 4

	beq         s3, x0, .BinaryTreeNodeDelete_label_48

	// *** Basic block 5

	mv          a1, s2
	mv          a0, s1
	jalr         x1, s3, 0

	// *** Basic block 6

.BinaryTreeNodeDelete_label_48:
	mv          a0, s1
	call        free

	// *** Basic block 7

	j           .BinaryTreeNodeDelete_label_20
.func_end_BinaryTreeNodeDelete:
	.size BinaryTreeNodeDelete, .func_end_BinaryTreeNodeDelete-BinaryTreeNodeDelete

	.local  BinaryTreeNodeTraverse
	.type BinaryTreeNodeTraverse, @function

BinaryTreeNodeTraverse:

	// *** Basic block 0

	.local BinaryTreeNodeTraverse
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
	bne         s1, x0, .BinaryTreeNodeTraverse_label_26

	// *** Basic block 1

.BinaryTreeNodeTraverse_label_23:
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

	// *** Basic block 2

.BinaryTreeNodeTraverse_label_26:
	ld          a0, 16(s1)
	addi        s5, s3, 1
	mv          a3, s4
	mv          a2, s5
	mv          a1, s2
	call        BinaryTreeNodeTraverse

	// *** Basic block 3

	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	jalr         x1, s2, 0

	// *** Basic block 4

	ld          a0, 8(s1)
	mv          a3, s4
	mv          a2, s5
	mv          a1, s2
	call        BinaryTreeNodeTraverse

	// *** Basic block 5

	j           .BinaryTreeNodeTraverse_label_23
.func_end_BinaryTreeNodeTraverse:
	.size BinaryTreeNodeTraverse, .func_end_BinaryTreeNodeTraverse-BinaryTreeNodeTraverse

	.global BinaryTreeInit
	.type BinaryTreeInit, @function

BinaryTreeInit:

	// *** Basic block 0

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
	mv          s1, a1
	mv          s2, a2
	mv          s3, a0
	mv          s4, a3
	beq         s1, x0, .BinaryTreeInit_label_34

	// *** Basic block 1

	j           .BinaryTreeInit_label_49

	// *** Basic block 2

.BinaryTreeInit_label_34:
	lla         a0, .str.1
	lla         a1, .str.2
	lla         a3, .str.3
	li          t0, 226		// 0xe2 ASCII \xe2
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.BinaryTreeInit_label_49:
	beq         s2, x0, .BinaryTreeInit_label_54

	// *** Basic block 5

	j           .BinaryTreeInit_label_70

	// *** Basic block 6

.BinaryTreeInit_label_54:
	lla         a0, .str.4
	lla         a1, .str.5
	lla         a3, .str.6
	li          t0, 227		// 0xe3 ASCII \xe3
	mv          a2, t0
	call        printf

	// *** Basic block 7

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           abort

	// *** Basic block 8

.BinaryTreeInit_label_70:
	sd          x0, 0(s3)
	sd          s1, 8(s3)
	sd          s2, 16(s3)
	sd          s4, 24(s3)
	sw          x0, 32(s3)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BinaryTreeInit:
	.size BinaryTreeInit, .func_end_BinaryTreeInit-BinaryTreeInit

	.global NewBinaryTree
	.type NewBinaryTree, @function

NewBinaryTree:

	// *** Basic block 0

	.global malloc
	.global BinaryTreeInit
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
	li          a0, 40		// 0x28 ASCII '('
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	mv          a3, s3
	mv          a2, s2
	mv          a1, s1
	mv          a0, s4
	call        BinaryTreeInit

	// *** Basic block 2

	mv          a0, s4

	// *** Basic block 3

.NewBinaryTree_label_31:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewBinaryTree:
	.size NewBinaryTree, .func_end_NewBinaryTree-NewBinaryTree

	.global BinaryTreeInsert
	.type BinaryTreeInsert, @function

BinaryTreeInsert:

	// *** Basic block 0

	.global printf
	.global abort
	.local BinaryTreeNodeInsert
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
	beq         s1, x0, .BinaryTreeInsert_label_27

	// *** Basic block 1

	j           .BinaryTreeInsert_label_44

	// *** Basic block 2

.BinaryTreeInsert_label_27:
	lla         a0, .str.7
	lla         a1, .str.8
	lla         a3, .str.9
	li          t0, 244		// 0xf4 ASCII \xf4
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.BinaryTreeInsert_label_44:
	beq         s2, x0, .BinaryTreeInsert_label_49

	// *** Basic block 5

	j           .BinaryTreeInsert_label_64

	// *** Basic block 6

.BinaryTreeInsert_label_49:
	lla         a0, .str.10
	lla         a1, .str.11
	lla         a3, .str.12
	li          t0, 245		// 0xf5 ASCII \xf5
	mv          a2, t0
	call        printf

	// *** Basic block 7

	call        abort

	// *** Basic block 8

.BinaryTreeInsert_label_64:
	lw          t0, 32(s1)
	addi        t0, t0, 1
	sw          t0, 32(s1)
	sw          t0, 32(s2)
	ld          a2, 8(s1)
	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           BinaryTreeNodeInsert
.func_end_BinaryTreeInsert:
	.size BinaryTreeInsert, .func_end_BinaryTreeInsert-BinaryTreeInsert

	.global BinaryTreeSearch
	.type BinaryTreeSearch, @function

BinaryTreeSearch:

	// *** Basic block 0

	.local BinaryTreeNodeSearch
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	bne         t0, x0, .BinaryTreeSearch_label_20

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.BinaryTreeSearch_label_17:
	ret         

	// *** Basic block 3

.BinaryTreeSearch_label_20:
	ld          a0, 0(t0)
	ld          a2, 16(t0)
	mv          a1, t1
	j           BinaryTreeNodeSearch
.func_end_BinaryTreeSearch:
	.size BinaryTreeSearch, .func_end_BinaryTreeSearch-BinaryTreeSearch

	.global BinaryTreeDestruct
	.type BinaryTreeDestruct, @function

BinaryTreeDestruct:

	// *** Basic block 0

	.local BinaryTreeNodeDelete
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	bne         t0, x0, .BinaryTreeDestruct_label_18

	// *** Basic block 1

.BinaryTreeDestruct_label_15:
	ret         

	// *** Basic block 2

.BinaryTreeDestruct_label_18:
	ld          a0, 0(t0)
	ld          a2, 24(t0)
	mv          a1, t1
	j           BinaryTreeNodeDelete
.func_end_BinaryTreeDestruct:
	.size BinaryTreeDestruct, .func_end_BinaryTreeDestruct-BinaryTreeDestruct

	.global BinaryTreeDelete
	.type BinaryTreeDelete, @function

BinaryTreeDelete:

	// *** Basic block 0

	.global BinaryTreeDestruct
	.global free
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
	bne         s1, x0, .BinaryTreeDelete_label_18

	// *** Basic block 1

.BinaryTreeDelete_label_15:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.BinaryTreeDelete_label_18:
	mv          a1, s2
	mv          a0, s1
	call        BinaryTreeDestruct

	// *** Basic block 3

	mv          a0, s1
	call        free

	// *** Basic block 4

	j           .BinaryTreeDelete_label_15
.func_end_BinaryTreeDelete:
	.size BinaryTreeDelete, .func_end_BinaryTreeDelete-BinaryTreeDelete

	.global BinaryTreeTraverse
	.type BinaryTreeTraverse, @function

BinaryTreeTraverse:

	// *** Basic block 0

	.local BinaryTreeNodeTraverse
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a2
	ld          a0, 0(t0)
	mv          a3, t1
	mv          a2, x0
	j           BinaryTreeNodeTraverse
.func_end_BinaryTreeTraverse:
	.size BinaryTreeTraverse, .func_end_BinaryTreeTraverse-BinaryTreeTraverse

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.1, @object
	.size .str.1, 30

.str.2:
	.asciz "binary_tree.c"
	.type .str.2, @object
	.size .str.2, 14

.str.3:
	.asciz "insert != NULL"
	.type .str.3, @object
	.size .str.3, 15

.str.4:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.4, @object
	.size .str.4, 30

.str.5:
	.asciz "binary_tree.c"
	.type .str.5, @object
	.size .str.5, 14

.str.6:
	.asciz "search != NULL"
	.type .str.6, @object
	.size .str.6, 15

.str.7:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.7, @object
	.size .str.7, 30

.str.8:
	.asciz "binary_tree.c"
	.type .str.8, @object
	.size .str.8, 14

.str.9:
	.asciz "tree != NULL"
	.type .str.9, @object
	.size .str.9, 13

.str.10:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.10, @object
	.size .str.10, 30

.str.11:
	.asciz "binary_tree.c"
	.type .str.11, @object
	.size .str.11, 14

.str.12:
	.asciz "node != NULL"
	.type .str.12, @object
	.size .str.12, 13

