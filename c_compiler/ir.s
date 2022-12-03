	.file   "ir.c"
	.text
	.option pic
.PCbegin:
	.global IRSetLocation
	.type IRSetLocation, @function

IRSetLocation:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local current_location
	la          t0, current_location
	sd          a0, 0(t0)
	ret         
.func_end_IRSetLocation:
	.size IRSetLocation, .func_end_IRSetLocation-IRSetLocation

	.global IROpcodeName
	.type IROpcodeName, @function

IROpcodeName:

	// *** Basic block 0

	.local opcodes
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, x0

	// *** Basic block 1

.IROpcodeName_label_16:
	slli        t2, t1, 4
	la          t3, opcodes
	add         t2, t3, t2
	lw          t3, 0(t2)
	bne         t3, t0, .IROpcodeName_label_31

	// *** Basic block 2

	ld          a0, 8(t2)

	// *** Basic block 3

.IROpcodeName_label_28:
	ret         

	// *** Basic block 4

.IROpcodeName_label_31:

	// *** Basic block 5

.IROpcodeName_label_32:
	addi        t1, t1, 1
	li          t0, 132		// 0x84 ASCII \x84
	bge         t1, t0, .IROpcodeName_label_16

	// *** Basic block 6

.IROpcodeName_label_37:
	lla         a0, .str.134
	ret         
.func_end_IROpcodeName:
	.size IROpcodeName, .func_end_IROpcodeName-IROpcodeName

	.global IRResetNodeId
	.type IRResetNodeId, @function

IRResetNodeId:

	// *** Basic block 0

	.local next_ir_id
	// Leaf procedure, no stack frame generated
	la          t0, next_ir_id
	li          t1, 1		// 0x1 ASCII \x1
	sw          t1, 0(t0)
	ret         
.func_end_IRResetNodeId:
	.size IRResetNodeId, .func_end_IRResetNodeId-IRResetNodeId

	.global IRInit
	.type IRInit, @function

IRInit:

	// *** Basic block 0

	.global ListElementInit
	.local next_ir_id
	.global VectorInit
	.local current_location
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
	call        ListElementInit

	// *** Basic block 1

	la          t0, next_ir_id
	lw          t1, 0(t0)
	addi        t0, t1, 1
	la          t1, next_ir_id
	sw          t0, 0(t1)
	sw          t1, 16(s1)
	sw          s2, 20(s1)
	addi        a0, s1, 24
	call        VectorInit

	// *** Basic block 2

	addi        a0, s1, 48
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorInit
.func_end_IRInit:
	.size IRInit, .func_end_IRInit-IRInit

	.global IRDestruct
	.type IRDestruct, @function

IRDestruct:

	// *** Basic block 0

	.global VectorDestruct
	.global TypeRecordDelete
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
	addi        a0, s1, 24
	call        VectorDestruct

	// *** Basic block 1

	addi        a0, s1, 48
	call        VectorDestruct

	// *** Basic block 2

	ld          a0, 80(s1)
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           TypeRecordDelete
.func_end_IRDestruct:
	.size IRDestruct, .func_end_IRDestruct-IRDestruct

	.global IRDelete
	.type IRDelete, @function

IRDelete:

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
	.global IRDestruct
	.global free
	mv          s1, a0
	call        IRDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_IRDelete:
	.size IRDelete, .func_end_IRDelete-IRDelete

	.global NewIR
	.type NewIR, @function

NewIR:

	// *** Basic block 0

	.global malloc
	.global IRInit
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
	li          a0, 136		// 0x88 ASCII \x88
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        IRInit

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.NewIR_label_21:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewIR:
	.size NewIR, .func_end_NewIR-NewIR

	.global IRNext
	.type IRNext, @function

IRNext:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          a0, 8(a0)

	// *** Basic block 1

.IRNext_label_11:
	ret         
.func_end_IRNext:
	.size IRNext, .func_end_IRNext-IRNext

	.global IRPrev
	.type IRPrev, @function

IRPrev:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          a0, 0(a0)

	// *** Basic block 1

.IRPrev_label_9:
	ret         
.func_end_IRPrev:
	.size IRPrev, .func_end_IRPrev-IRPrev

	.global IRSetType
	.type IRSetType, @function

IRSetType:

	// *** Basic block 0

	.global TypeRecordDelete
	.global TypeRecordIncRef
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
	mv          s1, a1
	mv          s2, a0
	sub         t1, s1, x0
	seqz        t0, t1
	beq         s1, x0, .IRSetType_label_21

	// *** Basic block 1

	ld          t1, 80(s2)
	sub         t1, s1, t1
	seqz        t0, t1

	// *** Basic block 2

.IRSetType_label_21:
	beqz        t0, .IRSetType_label_28

	// *** Basic block 3

	mv          a0, s2

	// *** Basic block 4

.IRSetType_label_25:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.IRSetType_label_28:
	ld          s3, 80(s2)
	beq         s3, x0, .IRSetType_label_37

	// *** Basic block 6

	mv          a0, s3
	call        TypeRecordDelete

	// *** Basic block 7

.IRSetType_label_37:
	mv          a0, s1
	call        TypeRecordIncRef

	// *** Basic block 8

	sd          s1, 80(s2)
	mv          a0, s2
	j           .IRSetType_label_25
.func_end_IRSetType:
	.size IRSetType, .func_end_IRSetType-IRSetType

	.global IRInList
	.type IRInList, @function

IRInList:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 8(t0)
	sub         t2, t1, x0
	snez        a0, t2
	bne         t1, x0, .IRInList_label_17

	// *** Basic block 1

	ld          t1, 0(t0)
	sub         t1, t1, x0
	snez        a0, t1

	// *** Basic block 2

.IRInList_label_17:

	// *** Basic block 3

.IRInList_label_19:
	ret         
.func_end_IRInList:
	.size IRInList, .func_end_IRInList-IRInList

	.global IRAddInput
	.type IRAddInput, @function

IRAddInput:

	// *** Basic block 0

	.global IRInList
	.global printf
	.global abort
	.global VectorAppend
	.global IRSetType
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
	mv          s1, a1
	mv          s2, a0
	mv          s3, a2
	lw          t0, 20(s1)
	li          t1, 17		// 0x11 ASCII \x11
	beq         t0, t1, .IRAddInput_label_57

	// *** Basic block 1

	mv          a0, s1
	call        IRInList

	// *** Basic block 2

	beqz        a0, .IRAddInput_label_40

	// *** Basic block 3

	j           .IRAddInput_label_56

	// *** Basic block 4

.IRAddInput_label_40:
	lla         a0, .str.135
	lla         a1, .str.136
	lla         a3, .str.137
	li          t0, 277		// 0x115
	mv          a2, t0
	call        printf

	// *** Basic block 5

	call        abort

	// *** Basic block 6

.IRAddInput_label_56:

	// *** Basic block 7

.IRAddInput_label_57:
	addi        a0, s2, 24
	mv          a1, s1
	call        VectorAppend

	// *** Basic block 8

	addi        a0, s1, 48
	mv          a1, s2
	call        VectorAppend

	// *** Basic block 9

	beqz        s3, .IRAddInput_label_76

	// *** Basic block 10

	ld          a1, 80(s1)
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType

	// *** Basic block 11

.IRAddInput_label_76:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_IRAddInput:
	.size IRAddInput, .func_end_IRAddInput-IRAddInput

	.global IRSetVarUse
	.type IRSetVarUse, @function

IRSetVarUse:

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
	mv          s1, a0
	mv          s2, a1
	lw          t0, 88(s1)
	bnez        t0, .IRSetVarUse_label_24

	// *** Basic block 1

	j           .IRSetVarUse_label_42

	// *** Basic block 2

.IRSetVarUse_label_24:
	lla         a0, .str.138
	lla         a1, .str.139
	lla         a3, .str.140
	li          t0, 287		// 0x11f
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

.IRSetVarUse_label_42:
	sd          s2, 120(s1)
	lw          t0, 88(s1)
	ori         t0, t0, 2
	sw          t0, 88(s1)
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_IRSetVarUse:
	.size IRSetVarUse, .func_end_IRSetVarUse-IRSetVarUse

	.global IRSetVarDef
	.type IRSetVarDef, @function

IRSetVarDef:

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
	mv          s1, a0
	mv          s2, a1
	lw          t0, 88(s1)
	bnez        t0, .IRSetVarDef_label_24

	// *** Basic block 1

	j           .IRSetVarDef_label_42

	// *** Basic block 2

.IRSetVarDef_label_24:
	lla         a0, .str.141
	lla         a1, .str.142
	lla         a3, .str.143
	li          t0, 293		// 0x125
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

.IRSetVarDef_label_42:
	sd          s2, 120(s1)
	lw          t0, 88(s1)
	ori         t0, t0, 1
	sw          t0, 88(s1)
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_IRSetVarDef:
	.size IRSetVarDef, .func_end_IRSetVarDef-IRSetVarDef

	.global IRReplaceInput
	.type IRReplaceInput, @function

IRReplaceInput:

	// *** Basic block 0

	.global VectorDeleteElement
	.global VectorAppend
	.global abort
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
	mv          s2, a2
	ld          t0, 24(s1)
	slli        s3, a1, 3
	add         t0, t0, s3
	ld          s4, 0(t0)
	mv          s5, x0
	addi        t0, s4, 48
	ld          s6, 8(t0)
	bge         x0, s6, .IRReplaceInput_label_67

	// *** Basic block 1

	ld          t0, 48(s4)

	// *** Basic block 2

.IRReplaceInput_label_36:
	slli        t1, s5, 3
	add         t0, t0, t1
	ld          s7, 0(t0)
	bne         s7, s1, .IRReplaceInput_label_62

	// *** Basic block 3

	addi        a0, s4, 48
	mv          a1, s5
	call        VectorDeleteElement

	// *** Basic block 4

	ld          t0, 24(s1)
	add         t0, t0, s3
	sd          s2, 0(t0)
	addi        a0, s2, 48
	mv          a1, s1
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
	j           VectorAppend

	// *** Basic block 5

.IRReplaceInput_label_59:
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

	// *** Basic block 6

.IRReplaceInput_label_62:

	// *** Basic block 7

.IRReplaceInput_label_63:
	addi        s5, s5, 1
	bge         s5, s6, .IRReplaceInput_label_36

	// *** Basic block 8

.IRReplaceInput_label_67:
	call        abort

	// *** Basic block 9

	j           .IRReplaceInput_label_59
.func_end_IRReplaceInput:
	.size IRReplaceInput, .func_end_IRReplaceInput-IRReplaceInput

	.global IRRemoveInput
	.type IRRemoveInput, @function

IRRemoveInput:

	// *** Basic block 0

	.global VectorDeleteElement
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
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	ld          t0, 24(s1)
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s3, 0(t0)
	mv          s4, x0
	addi        t0, s3, 48
	ld          s5, 8(t0)
	bge         x0, s5, .IRRemoveInput_label_59

	// *** Basic block 1

	ld          t0, 48(s3)

	// *** Basic block 2

.IRRemoveInput_label_32:
	slli        t1, s4, 3
	add         t0, t0, t1
	ld          s6, 0(t0)
	bne         s6, s1, .IRRemoveInput_label_54

	// *** Basic block 3

	addi        a0, s3, 48
	mv          a1, s4
	call        VectorDeleteElement

	// *** Basic block 4

	addi        a0, s1, 24
	mv          a1, s2
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
	j           VectorDeleteElement

	// *** Basic block 5

.IRRemoveInput_label_51:
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

.IRRemoveInput_label_54:

	// *** Basic block 7

.IRRemoveInput_label_55:
	addi        s4, s4, 1
	bge         s4, s5, .IRRemoveInput_label_32

	// *** Basic block 8

.IRRemoveInput_label_59:
	call        abort

	// *** Basic block 9

	j           .IRRemoveInput_label_51
.func_end_IRRemoveInput:
	.size IRRemoveInput, .func_end_IRRemoveInput-IRRemoveInput

	.global IRRemoveNode
	.type IRRemoveNode, @function

IRRemoveNode:

	// *** Basic block 0

	.global VectorDeleteElement
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
	mv          s2, x0
	addi        t0, s1, 24
	ld          s3, 8(t0)
	bge         x0, s3, .IRRemoveNode_label_59

	// *** Basic block 1

	ld          t0, 24(s1)

	// *** Basic block 2

.IRRemoveNode_label_21:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	mv          s5, x0
	addi        t0, s4, 48
	ld          s6, 8(t0)
	bge         x0, s6, .IRRemoveNode_label_54

	// *** Basic block 3

	ld          t0, 48(s4)

	// *** Basic block 4

.IRRemoveNode_label_35:
	slli        t1, s5, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	bne         t0, s1, .IRRemoveNode_label_49

	// *** Basic block 5

	addi        a0, s4, 48
	mv          a1, s5
	call        VectorDeleteElement

	// *** Basic block 6

	j           .IRRemoveNode_label_54

	// *** Basic block 7

.IRRemoveNode_label_49:

	// *** Basic block 8

.IRRemoveNode_label_50:
	addi        s5, s5, 1
	bge         s5, s6, .IRRemoveNode_label_35

	// *** Basic block 9

.IRRemoveNode_label_54:

	// *** Basic block 10

.IRRemoveNode_label_55:
	addi        s2, s2, 1
	bge         s2, s3, .IRRemoveNode_label_21

	// *** Basic block 11

.IRRemoveNode_label_59:
	mv          s3, x0
	addi        t0, s1, 48
	ld          s4, 8(t0)
	bge         x0, s4, .IRRemoveNode_label_104

	// *** Basic block 12

	ld          t0, 48(s1)

	// *** Basic block 13

.IRRemoveNode_label_68:
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s6, 0(t0)
	mv          s7, x0
	addi        t0, s6, 24
	ld          s8, 8(t0)
	bge         x0, s8, .IRRemoveNode_label_99

	// *** Basic block 14

	ld          t0, 24(s6)

	// *** Basic block 15

.IRRemoveNode_label_81:
	slli        t1, s7, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	bne         t0, s1, .IRRemoveNode_label_94

	// *** Basic block 16

	addi        a0, s6, 24
	mv          a1, s7
	call        VectorDeleteElement

	// *** Basic block 17

	j           .IRRemoveNode_label_99

	// *** Basic block 18

.IRRemoveNode_label_94:

	// *** Basic block 19

.IRRemoveNode_label_95:
	addi        s7, s7, 1
	bge         s7, s8, .IRRemoveNode_label_81

	// *** Basic block 20

.IRRemoveNode_label_99:

	// *** Basic block 21

.IRRemoveNode_label_100:
	addi        s3, s3, 1
	bge         s3, s4, .IRRemoveNode_label_68

	// *** Basic block 22

.IRRemoveNode_label_104:
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
.func_end_IRRemoveNode:
	.size IRRemoveNode, .func_end_IRRemoveNode-IRRemoveNode

	.global NewIR1
	.type NewIR1, @function

NewIR1:

	// *** Basic block 0

	.global NewIR
	.global IRAddInput
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
	call        NewIR

	// *** Basic block 1

	mv          s2, a0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	call        IRAddInput

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.NewIR1_label_27:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewIR1:
	.size NewIR1, .func_end_NewIR1-NewIR1

	.global NewIR2
	.type NewIR2, @function

NewIR2:

	// *** Basic block 0

	.global NewIR
	.global IRAddInput
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
	mv          s1, a1
	mv          s2, a2
	call        NewIR

	// *** Basic block 1

	mv          s3, a0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a1, s1
	mv          a0, s3
	call        IRAddInput

	// *** Basic block 2

	mv          a2, x0
	mv          a1, s2
	mv          a0, s3
	call        IRAddInput

	// *** Basic block 3

	mv          a0, s3

	// *** Basic block 4

.NewIR2_label_38:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewIR2:
	.size NewIR2, .func_end_NewIR2-NewIR2

	.global NewIR3
	.type NewIR3, @function

NewIR3:

	// *** Basic block 0

	.global NewIR
	.global IRAddInput
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
	mv          s3, a3
	call        NewIR

	// *** Basic block 1

	mv          s4, a0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a1, s1
	mv          a0, s4
	call        IRAddInput

	// *** Basic block 2

	mv          a2, x0
	mv          a1, s2
	mv          a0, s4
	call        IRAddInput

	// *** Basic block 3

	mv          a2, x0
	mv          a1, s3
	mv          a0, s4
	call        IRAddInput

	// *** Basic block 4

	mv          a0, s4

	// *** Basic block 5

.NewIR3_label_48:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewIR3:
	.size NewIR3, .func_end_NewIR3-NewIR3

	.global NewIR4
	.type NewIR4, @function

NewIR4:

	// *** Basic block 0

	.global NewIR
	.global IRAddInput
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
	mv          s2, a2
	mv          s3, a3
	mv          s4, a4
	call        NewIR

	// *** Basic block 1

	mv          s5, a0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a1, s1
	mv          a0, s5
	call        IRAddInput

	// *** Basic block 2

	mv          a2, x0
	mv          a1, s2
	mv          a0, s5
	call        IRAddInput

	// *** Basic block 3

	mv          a2, x0
	mv          a1, s3
	mv          a0, s5
	call        IRAddInput

	// *** Basic block 4

	mv          a2, x0
	mv          a1, s4
	mv          a0, s5
	call        IRAddInput

	// *** Basic block 5

	mv          a0, s5

	// *** Basic block 6

.NewIR4_label_58:
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
.func_end_NewIR4:
	.size NewIR4, .func_end_NewIR4-NewIR4

	.local  ConstantOpcode
	.type ConstantOpcode, @function

ConstantOpcode:

	// *** Basic block 0

	.local type_table
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
	mv          s1, a0
	bne         s1, x0, .ConstantOpcode_label_27

	// *** Basic block 1

	li          a0, 4		// 0x4 ASCII \x4

	// *** Basic block 2

.ConstantOpcode_label_24:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.ConstantOpcode_label_27:
	mv          s2, x0
	la          t0, type_table
	ld          t0, 0(t0)
	beq         t0, x0, .ConstantOpcode_label_59

	// *** Basic block 4

.ConstantOpcode_label_36:
	slli        t0, s2, 4
	la          t1, type_table
	add         s3, t1, t0
	ld          t0, 0(s3)
	mv          a0, s1
	jalr         x1, t0, 0

	// *** Basic block 5

	beqz        a0, .ConstantOpcode_label_49

	// *** Basic block 6

	lw          a0, 8(s3)
	j           .ConstantOpcode_label_24

	// *** Basic block 7

.ConstantOpcode_label_49:

	// *** Basic block 8

.ConstantOpcode_label_50:
	addi        s2, s2, 1
	slli        t0, s2, 4
	la          t1, type_table
	add         t0, t1, t0
	ld          t0, 0(t0)
	beq         t0, x0, .ConstantOpcode_label_36

	// *** Basic block 9

.ConstantOpcode_label_59:
	lla         a0, .str.144
	lla         a1, .str.145
	lla         a3, .str.146
	li          t0, 414		// 0x19e
	mv          a2, t0
	call        printf

	// *** Basic block 10

	call        abort

	// *** Basic block 11

	mv          a0, x0
	j           .ConstantOpcode_label_24
.func_end_ConstantOpcode:
	.size ConstantOpcode, .func_end_ConstantOpcode-ConstantOpcode

	.global NewIntIRConstant
	.type NewIntIRConstant, @function

NewIntIRConstant:

	// *** Basic block 0

	.global malloc
	.global IRInit
	.local ConstantOpcode
	.global IRSetType
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
	li          a0, 144		// 0x90 ASCII \x90
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	mv          a0, s1
	call        ConstantOpcode

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s3
	call        IRInit

	// *** Basic block 3

	sd          s2, 136(s3)
	mv          a1, s1
	mv          a0, s3
	call        IRSetType

	// *** Basic block 4

	mv          a0, s3

	// *** Basic block 5

.NewIntIRConstant_label_38:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewIntIRConstant:
	.size NewIntIRConstant, .func_end_NewIntIRConstant-NewIntIRConstant

	.global NewFloatingPointIRConstant
	.type NewFloatingPointIRConstant, @function

NewFloatingPointIRConstant:

	// *** Basic block 0

	.global malloc
	.global IRInit
	.local ConstantOpcode
	.global IRSetType
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	// Saved floating point registers.
	fsd fs0, 8(sp)
	// End of stack frame
	mv          s1, a0
	fmv.d       fs0, fa0
	li          a0, 144		// 0x90 ASCII \x90
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a0, s1
	call        ConstantOpcode

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s2
	call        IRInit

	// *** Basic block 3

	fsd         fs0, 136(s2)
	mv          a1, s1
	mv          a0, s2
	call        IRSetType

	// *** Basic block 4

	mv          a0, s2

	// *** Basic block 5

.NewFloatingPointIRConstant_label_39:
	// Restored registers.
	fld fs0, 24(sp)
	ld s1, 16(sp)
	ld s2, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewFloatingPointIRConstant:
	.size NewFloatingPointIRConstant, .func_end_NewFloatingPointIRConstant-NewFloatingPointIRConstant

	.global NewIRLocation
	.type NewIRLocation, @function

NewIRLocation:

	// *** Basic block 0

	.global malloc
	.global IRInit
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
	li          a0, 144		// 0x90 ASCII \x90
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	li          t0, 105		// 0x69 ASCII 'i'
	mv          a1, t0
	mv          a0, s2
	call        IRInit

	// *** Basic block 2

	sd          s1, 136(s2)
	mv          a0, s2

	// *** Basic block 3

.NewIRLocation_label_28:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewIRLocation:
	.size NewIRLocation, .func_end_NewIRLocation-NewIRLocation

	.global NewIRNamedLabel
	.type NewIRNamedLabel, @function

NewIRNamedLabel:

	// *** Basic block 0

	.global malloc
	.global IRInit
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
	li          a0, 144		// 0x90 ASCII \x90
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	mv          a0, s2
	call        IRInit

	// *** Basic block 2

	sd          s1, 136(s2)
	mv          a0, s2

	// *** Basic block 3

.NewIRNamedLabel_label_28:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewIRNamedLabel:
	.size NewIRNamedLabel, .func_end_NewIRNamedLabel-NewIRNamedLabel

	.global NewIRVariable
	.type NewIRVariable, @function

NewIRVariable:

	// *** Basic block 0

	.global malloc
	.global StorageIs
	.global IRInit
	.global IRSetType
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
	li          a0, 144		// 0x90 ASCII \x90
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	li          s3, 99		// 0x63 ASCII 'c'
	lb          s4, 56(s1)
	slli        t0, s4, 59
	srai        t0, t0, 63
	beqz        t0, .NewIRVariable_label_41

	// *** Basic block 2

	li          s3, 98		// 0x62 ASCII 'b'
	j           .NewIRVariable_label_77

	// *** Basic block 3

.NewIRVariable_label_41:
	slli        t0, s4, 58
	srai        t0, t0, 63
	beqz        t0, .NewIRVariable_label_47

	// *** Basic block 4

	li          s3, 100		// 0x64 ASCII 'd'
	j           .NewIRVariable_label_76

	// *** Basic block 5

.NewIRVariable_label_47:
	slli        t0, s4, 60
	srai        t0, t0, 63
	beqz        t0, .NewIRVariable_label_64

	// *** Basic block 6

	lw          a0, 48(s1)
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	call        StorageIs

	// *** Basic block 7

	not         t0, a0
	beqz        t0, .NewIRVariable_label_62

	// *** Basic block 8

	li          s3, 96		// 0x60 ASCII '`'

	// *** Basic block 9

.NewIRVariable_label_62:
	j           .NewIRVariable_label_75

	// *** Basic block 10

.NewIRVariable_label_64:
	lw          a0, 48(s1)
	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	call        StorageIs

	// *** Basic block 11

	beqz        a0, .NewIRVariable_label_74

	// *** Basic block 12

	li          s3, 97		// 0x61 ASCII 'a'

	// *** Basic block 13

.NewIRVariable_label_74:

	// *** Basic block 14

.NewIRVariable_label_75:

	// *** Basic block 15

.NewIRVariable_label_76:

	// *** Basic block 16

.NewIRVariable_label_77:
	mv          a1, s3
	mv          a0, s2
	call        IRInit

	// *** Basic block 17

	sd          s1, 136(s2)
	ld          a1, 40(s1)
	mv          a0, s2
	call        IRSetType

	// *** Basic block 18

	mv          a0, s2

	// *** Basic block 19

.NewIRVariable_label_93:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewIRVariable:
	.size NewIRVariable, .func_end_NewIRVariable-NewIRVariable

	.global NewIRPhi
	.type NewIRPhi, @function

NewIRPhi:

	// *** Basic block 0

	.global malloc
	.global IRInit
	.global IRSetType
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
	li          a0, 144		// 0x90 ASCII \x90
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	li          t0, 122		// 0x7a ASCII 'z'
	mv          a1, t0
	mv          a0, s2
	call        IRInit

	// *** Basic block 2

	sd          s1, 136(s2)
	ld          a1, 40(s1)
	mv          a0, s2
	call        IRSetType

	// *** Basic block 3

	mv          a0, s2

	// *** Basic block 4

.NewIRPhi_label_36:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewIRPhi:
	.size NewIRPhi, .func_end_NewIRPhi-NewIRPhi

	.global NewIRSSAVar
	.type NewIRSSAVar, @function

NewIRSSAVar:

	// *** Basic block 0

	.global malloc
	.global IRInit
	.global IRSetType
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
	li          a0, 144		// 0x90 ASCII \x90
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	li          t0, 101		// 0x65 ASCII 'e'
	mv          a1, t0
	mv          a0, s2
	call        IRInit

	// *** Basic block 2

	sd          s1, 136(s2)
	ld          a1, 40(s1)
	mv          a0, s2
	call        IRSetType

	// *** Basic block 3

	mv          a0, s2

	// *** Basic block 4

.NewIRSSAVar_label_36:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewIRSSAVar:
	.size NewIRSSAVar, .func_end_NewIRSSAVar-NewIRSSAVar

	.global IRPrint
	.type IRPrint, @function

IRPrint:

	// *** Basic block 0

	.global fprintf
	.global IROpcodeName
	.global DecodeSourceLocation
	.global IRIsVarDef
	.global IRIsVarRef
	.local kFlagNames
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -48(s0)
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
	lla         s3, .str.147
	lw          s4, 16(s2)
	lw          s5, 20(s2)
	mv          a0, s5
	call        IROpcodeName

	// *** Basic block 1

	mv          a3, a0
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 2

	lla         s3, .str.148
	mv          s4, x0
	addi        t0, s2, 24
	ld          s6, 8(t0)
	bge         x0, s6, .IRPrint_label_119

	// *** Basic block 3

	ld          s7, 24(s2)

	// *** Basic block 4

.IRPrint_label_97:
	slli        t0, s4, 3
	add         t0, s7, t0
	ld          s7, 0(t0)
	lla         a1, .str.149
	lw          a3, 16(s7)
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 5

	lla         s3, .str.150

	// *** Basic block 6

.IRPrint_label_115:
	addi        s4, s4, 1
	bge         s4, s6, .IRPrint_label_97

	// *** Basic block 7

.IRPrint_label_119:
	lla         a1, .str.151
	mv          a0, s1
	call        fprintf

	// *** Basic block 8

	lla         a1, .str.152
	mv          a0, s1
	call        fprintf

	// *** Basic block 9

	lla         s3, .str.153
	mv          s6, x0
	addi        t0, s2, 48
	ld          s7, 8(t0)
	bge         x0, s7, .IRPrint_label_164

	// *** Basic block 10

	ld          s8, 48(s2)

	// *** Basic block 11

.IRPrint_label_142:
	slli        t0, s6, 3
	add         t0, s8, t0
	ld          s8, 0(t0)
	lla         a1, .str.154
	lw          a3, 16(s8)
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 12

	lla         s3, .str.155

	// *** Basic block 13

.IRPrint_label_160:
	addi        s6, s6, 1
	bge         s6, s7, .IRPrint_label_142

	// *** Basic block 14

.IRPrint_label_164:
	lla         a1, .str.156
	mv          a0, s1
	call        fprintf

	// *** Basic block 15

	mv          s3, s2
	mv          s8, s2
	mv          s9, s2
	li          t0, 2		// 0x2 ASCII \x2
	beq         s5, t0, .IRPrint_label_255

	// *** Basic block 16

	li          t0, 3		// 0x3 ASCII \x3
	beq         s5, t0, .IRPrint_label_256

	// *** Basic block 17

	li          t0, 4		// 0x4 ASCII \x4
	beq         s5, t0, .IRPrint_label_254

	// *** Basic block 18

	li          t0, 5		// 0x5 ASCII \x5
	beq         s5, t0, .IRPrint_label_257

	// *** Basic block 19

	li          t0, 7		// 0x7 ASCII \x7
	beq         s5, t0, .IRPrint_label_269

	// *** Basic block 20

	li          t0, 8		// 0x8 ASCII \x8
	beq         s5, t0, .IRPrint_label_258

	// *** Basic block 21

	li          t0, 92		// 0x5c ASCII '\'
	beq         s5, t0, .IRPrint_label_335

	// *** Basic block 22

	li          t0, 96		// 0x60 ASCII '`'
	beq         s5, t0, .IRPrint_label_281

	// *** Basic block 23

	li          t0, 97		// 0x61 ASCII 'a'
	beq         s5, t0, .IRPrint_label_282

	// *** Basic block 24

	li          t0, 98		// 0x62 ASCII 'b'
	beq         s5, t0, .IRPrint_label_283

	// *** Basic block 25

	li          t0, 99		// 0x63 ASCII 'c'
	beq         s5, t0, .IRPrint_label_284

	// *** Basic block 26

	li          t0, 100		// 0x64 ASCII 'd'
	beq         s5, t0, .IRPrint_label_285

	// *** Basic block 27

	li          t0, 101		// 0x65 ASCII 'e'
	beq         s5, t0, .IRPrint_label_287

	// *** Basic block 28

	li          t0, 105		// 0x69 ASCII 'i'
	beq         s5, t0, .IRPrint_label_300

	// *** Basic block 29

	li          t0, 122		// 0x7a ASCII 'z'
	beq         s5, t0, .IRPrint_label_286

	// *** Basic block 30

.IRPrint_label_252:
	j           .IRPrint_label_349

	// *** Basic block 31

.IRPrint_label_254:

	// *** Basic block 32

.IRPrint_label_255:

	// *** Basic block 33

.IRPrint_label_256:

	// *** Basic block 34

.IRPrint_label_257:

	// *** Basic block 35

.IRPrint_label_258:
	lla         a1, .str.157
	ld          a2, 136(s3)
	mv          a0, s1
	call        fprintf

	// *** Basic block 36

	j           .IRPrint_label_349

	// *** Basic block 37

.IRPrint_label_269:
	lla         a1, .str.158
	fld         fa0, 136(s3)
	mv          a0, s1
	call        fprintf

	// *** Basic block 38

	j           .IRPrint_label_349

	// *** Basic block 39

.IRPrint_label_281:

	// *** Basic block 40

.IRPrint_label_282:

	// *** Basic block 41

.IRPrint_label_283:

	// *** Basic block 42

.IRPrint_label_284:

	// *** Basic block 43

.IRPrint_label_285:

	// *** Basic block 44

.IRPrint_label_286:

	// *** Basic block 45

.IRPrint_label_287:
	lla         a1, .str.159
	ld          t0, 136(s8)
	ld          a2, 16(t0)
	mv          a0, s1
	call        fprintf

	// *** Basic block 46

	j           .IRPrint_label_349

	// *** Basic block 47

.IRPrint_label_300:
	ld          a0, 136(s9)
	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	call        DecodeSourceLocation

	// *** Basic block 48

	lla         a1, .str.160
	ld          a2, -48(s0)
	lw          a3, -40(s0)
	lw          a4, -36(s0)
	lw          a5, -32(s0)
	mv          a0, s1
	call        fprintf

	// *** Basic block 49

	j           .IRPrint_label_349

	// *** Basic block 50

.IRPrint_label_335:
	lw          t0, 88(s2)
	andi        t0, t0, 4
	beqz        t0, .IRPrint_label_347

	// *** Basic block 51

	lla         a1, .str.161
	mv          a0, s1
	call        fprintf

	// *** Basic block 52

.IRPrint_label_347:
	j           .IRPrint_label_349

	// *** Basic block 53

.IRPrint_label_349:
	lla         a1, .str.162
	mv          a2, s7
	mv          a0, s1
	call        fprintf

	// *** Basic block 54

	mv          a0, s2
	call        IRIsVarDef

	// *** Basic block 55

	beqz        a0, .IRPrint_label_374

	// *** Basic block 56

	lla         a1, .str.163
	ld          t0, 120(s2)
	ld          a2, 16(t0)
	mv          a0, s1
	call        fprintf

	// *** Basic block 57

	j           .IRPrint_label_391

	// *** Basic block 58

.IRPrint_label_374:
	mv          a0, s2
	call        IRIsVarRef

	// *** Basic block 59

	beqz        a0, .IRPrint_label_390

	// *** Basic block 60

	lla         a1, .str.164
	ld          t0, 120(s2)
	ld          a2, 16(t0)
	mv          a0, s1
	call        fprintf

	// *** Basic block 61

.IRPrint_label_390:

	// *** Basic block 62

.IRPrint_label_391:
	lw          s5, 88(s2)
	beqz        s5, .IRPrint_label_442

	// *** Basic block 63

	lla         a1, .str.165
	mv          a0, s1
	call        fprintf

	// *** Basic block 64

	lla         s7, .str.166
	mv          s10, x0

	// *** Basic block 65

.IRPrint_label_408:
	li          t0, 1		// 0x1 ASCII \x1
	sll         t0, t0, s10
	and         t0, s5, t0
	beqz        t0, .IRPrint_label_429

	// *** Basic block 66

	lla         a1, .str.167
	slli        t0, s10, 3
	lla         t1, kFlagNames
	add         t0, t1, t0
	ld          a3, 0(t0)
	mv          a2, s7
	mv          a0, s1
	call        fprintf

	// *** Basic block 68

.IRPrint_label_429:

	// *** Basic block 69

.IRPrint_label_430:
	addi        s10, s10, 1
	li          t0, 32		// 0x20 ASCII ' '
	bge         s10, t0, .IRPrint_label_408

	// *** Basic block 70

.IRPrint_label_435:
	lla         a1, .str.169
	mv          a0, s1
	call        fprintf

	// *** Basic block 71

.IRPrint_label_442:
	lla         a1, .str.170
	mv          a0, s1
	call        fprintf

	// *** Basic block 72

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
.func_end_IRPrint:
	.size IRPrint, .func_end_IRPrint-IRPrint

	.global IRIsBranch
	.type IRIsBranch, @function

IRIsBranch:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	addi        t1, t0, -88
	seqz        a0, t1
	li          t1, 88		// 0x58 ASCII 'X'
	beq         t0, t1, .IRIsBranch_label_24

	// *** Basic block 1

	addi        t1, t0, -89
	seqz        a0, t1

	// *** Basic block 2

.IRIsBranch_label_24:
	bnez        a0, .IRIsBranch_label_29

	// *** Basic block 3

	addi        t1, t0, -90
	seqz        a0, t1

	// *** Basic block 4

.IRIsBranch_label_29:
	bnez        a0, .IRIsBranch_label_34

	// *** Basic block 5

	addi        t0, t0, -91
	seqz        a0, t0

	// *** Basic block 6

.IRIsBranch_label_34:

	// *** Basic block 7

.IRIsBranch_label_36:
	ret         
.func_end_IRIsBranch:
	.size IRIsBranch, .func_end_IRIsBranch-IRIsBranch

	.global IRIsUnconditionalBranch
	.type IRIsUnconditionalBranch, @function

IRIsUnconditionalBranch:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	addi        t0, t0, -90
	seqz        a0, t0

	// *** Basic block 1

.IRIsUnconditionalBranch_label_14:
	ret         
.func_end_IRIsUnconditionalBranch:
	.size IRIsUnconditionalBranch, .func_end_IRIsUnconditionalBranch-IRIsUnconditionalBranch

	.global IRIsConditionalBranch
	.type IRIsConditionalBranch, @function

IRIsConditionalBranch:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	addi        t1, t0, -88
	seqz        a0, t1
	li          t1, 88		// 0x58 ASCII 'X'
	beq         t0, t1, .IRIsConditionalBranch_label_20

	// *** Basic block 1

	addi        t0, t0, -89
	seqz        a0, t0

	// *** Basic block 2

.IRIsConditionalBranch_label_20:

	// *** Basic block 3

.IRIsConditionalBranch_label_22:
	ret         
.func_end_IRIsConditionalBranch:
	.size IRIsConditionalBranch, .func_end_IRIsConditionalBranch-IRIsConditionalBranch

	.global IRIsReturn
	.type IRIsReturn, @function

IRIsReturn:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	addi        t0, t0, -93
	seqz        a0, t0

	// *** Basic block 1

.IRIsReturn_label_14:
	ret         
.func_end_IRIsReturn:
	.size IRIsReturn, .func_end_IRIsReturn-IRIsReturn

	.global IRIsCall
	.type IRIsCall, @function

IRIsCall:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	addi        t0, t0, -92
	seqz        a0, t0

	// *** Basic block 1

.IRIsCall_label_14:
	ret         
.func_end_IRIsCall:
	.size IRIsCall, .func_end_IRIsCall-IRIsCall

	.global IRIsConst
	.type IRIsConst, @function

IRIsConst:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	slti        t1, t0, 2
	not         a0, t1
	li          t1, 2		// 0x2 ASCII \x2
	blt         t0, t1, .IRIsConst_label_19

	// *** Basic block 1

	li          t1, 8		// 0x8 ASCII \x8
	slt         t0, t1, t0
	not         a0, t0

	// *** Basic block 2

.IRIsConst_label_19:

	// *** Basic block 3

.IRIsConst_label_21:
	ret         
.func_end_IRIsConst:
	.size IRIsConst, .func_end_IRIsConst-IRIsConst

	.global IRIsZero
	.type IRIsZero, @function

IRIsZero:

	// *** Basic block 0

	.global IRIsConst
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
	call        IRIsConst

	// *** Basic block 1

	beqz        a0, .IRIsZero_label_19

	// *** Basic block 2

	ld          t0, 136(s1)
	seqz        a0, t0

	// *** Basic block 3

.IRIsZero_label_19:

	// *** Basic block 4

.IRIsZero_label_21:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_IRIsZero:
	.size IRIsZero, .func_end_IRIsZero-IRIsZero

	.global IRIsIntConst
	.type IRIsIntConst, @function

IRIsIntConst:

	// *** Basic block 0

	.global IRIsConst
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
	call        IRIsConst

	// *** Basic block 1

	beqz        a0, .IRIsIntConst_label_22

	// *** Basic block 2

	lw          t0, 20(s1)
	addi        t0, t0, -6
	snez        a0, t0

	// *** Basic block 3

.IRIsIntConst_label_22:
	beqz        a0, .IRIsIntConst_label_29

	// *** Basic block 4

	lw          t0, 20(s1)
	addi        t0, t0, -7
	snez        a0, t0

	// *** Basic block 5

.IRIsIntConst_label_29:

	// *** Basic block 6

.IRIsIntConst_label_31:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_IRIsIntConst:
	.size IRIsIntConst, .func_end_IRIsIntConst-IRIsIntConst

	.global IRIntConstValue
	.type IRIntConstValue, @function

IRIntConstValue:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 136(t0)

	// *** Basic block 1

.IRIntConstValue_label_13:
	ret         
.func_end_IRIntConstValue:
	.size IRIntConstValue, .func_end_IRIntConstValue-IRIntConstValue

	.global IRIsVariable
	.type IRIsVariable, @function

IRIsVariable:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	li          t1, 96		// 0x60 ASCII '`'
	beq         t0, t1, .IRIsVariable_label_63

	// *** Basic block 1

	li          t1, 97		// 0x61 ASCII 'a'
	beq         t0, t1, .IRIsVariable_label_64

	// *** Basic block 2

	li          t1, 98		// 0x62 ASCII 'b'
	beq         t0, t1, .IRIsVariable_label_65

	// *** Basic block 3

	li          t1, 99		// 0x63 ASCII 'c'
	beq         t0, t1, .IRIsVariable_label_66

	// *** Basic block 4

	li          t1, 100		// 0x64 ASCII 'd'
	beq         t0, t1, .IRIsVariable_label_67

	// *** Basic block 5

	li          t1, 101		// 0x65 ASCII 'e'
	beq         t0, t1, .IRIsVariable_label_68

	// *** Basic block 6

	li          t1, 102		// 0x66 ASCII 'f'
	beq         t0, t1, .IRIsVariable_label_70

	// *** Basic block 7

	li          t1, 122		// 0x7a ASCII 'z'
	beq         t0, t1, .IRIsVariable_label_69

	// *** Basic block 8

.IRIsVariable_label_59:
	mv          a0, x0
	ret         

	// *** Basic block 9

.IRIsVariable_label_63:

	// *** Basic block 10

.IRIsVariable_label_64:

	// *** Basic block 11

.IRIsVariable_label_65:

	// *** Basic block 12

.IRIsVariable_label_66:

	// *** Basic block 13

.IRIsVariable_label_67:

	// *** Basic block 14

.IRIsVariable_label_68:

	// *** Basic block 15

.IRIsVariable_label_69:

	// *** Basic block 16

.IRIsVariable_label_70:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 17

.IRIsVariable_label_73:
	ret         
.func_end_IRIsVariable:
	.size IRIsVariable, .func_end_IRIsVariable-IRIsVariable

	.global IRIsAutoVariable
	.type IRIsAutoVariable, @function

IRIsAutoVariable:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	li          t1, 96		// 0x60 ASCII '`'
	blt         t0, t1, .IRIsAutoVariable_label_40

	// *** Basic block 1

	li          t1, 100		// 0x64 ASCII 'd'
	blt         t1, t0, .IRIsAutoVariable_label_40

	// *** Basic block 2

	addi        t0, t0, -96
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .IRIsAutoVariable_label_33

	// *** Basic block 4

	j           .IRIsAutoVariable_label_40

	// *** Basic block 5

	j           .IRIsAutoVariable_label_40

	// *** Basic block 6

	j           .IRIsAutoVariable_label_40

	// *** Basic block 7

	j           .IRIsAutoVariable_label_34

	// *** Basic block 8

.IRIsAutoVariable_label_33:

	// *** Basic block 9

.IRIsAutoVariable_label_34:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 10

.IRIsAutoVariable_label_37:
	ret         

	// *** Basic block 11

.IRIsAutoVariable_label_40:
	mv          a0, x0
	ret         
.func_end_IRIsAutoVariable:
	.size IRIsAutoVariable, .func_end_IRIsAutoVariable-IRIsAutoVariable

	.global IRIsArgument
	.type IRIsArgument, @function

IRIsArgument:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	addi        t0, t0, -98
	seqz        a0, t0

	// *** Basic block 1

.IRIsArgument_label_14:
	ret         
.func_end_IRIsArgument:
	.size IRIsArgument, .func_end_IRIsArgument-IRIsArgument

	.global IRIsStaticVariable
	.type IRIsStaticVariable, @function

IRIsStaticVariable:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	li          t1, 97		// 0x61 ASCII 'a'
	blt         t0, t1, .IRIsStaticVariable_label_38

	// *** Basic block 1

	li          t1, 99		// 0x63 ASCII 'c'
	blt         t1, t0, .IRIsStaticVariable_label_38

	// *** Basic block 2

	addi        t0, t0, -97
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .IRIsStaticVariable_label_32

	// *** Basic block 4

	j           .IRIsStaticVariable_label_38

	// *** Basic block 5

	j           .IRIsStaticVariable_label_31

	// *** Basic block 6

.IRIsStaticVariable_label_31:

	// *** Basic block 7

.IRIsStaticVariable_label_32:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 8

.IRIsStaticVariable_label_35:
	ret         

	// *** Basic block 9

.IRIsStaticVariable_label_38:
	mv          a0, x0
	ret         
.func_end_IRIsStaticVariable:
	.size IRIsStaticVariable, .func_end_IRIsStaticVariable-IRIsStaticVariable

	.global IRIsThreadVariable
	.type IRIsThreadVariable, @function

IRIsThreadVariable:

	// *** Basic block 0

	.global StorageIs
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	lw          t1, 20(t0)
	li          t2, 97		// 0x61 ASCII 'a'
	blt         t1, t2, .IRIsThreadVariable_label_54

	// *** Basic block 1

	li          t2, 99		// 0x63 ASCII 'c'
	blt         t2, t1, .IRIsThreadVariable_label_54

	// *** Basic block 2

	addi        t1, t1, -97
	slli        t1, t1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 3

	j           .IRIsThreadVariable_label_35

	// *** Basic block 4

	j           .IRIsThreadVariable_label_54

	// *** Basic block 5

	j           .IRIsThreadVariable_label_34

	// *** Basic block 6

.IRIsThreadVariable_label_34:

	// *** Basic block 7

.IRIsThreadVariable_label_35:
	mv          t1, t0
	ld          t2, 136(t1)
	lw          a0, 48(t2)
	li          t2, 64		// 0x40 ASCII '@'
	mv          a1, t2
	j           StorageIs

	// *** Basic block 10

.IRIsThreadVariable_label_54:
	mv          a0, x0
	ret         
.func_end_IRIsThreadVariable:
	.size IRIsThreadVariable, .func_end_IRIsThreadVariable-IRIsThreadVariable

	.global IRIsVarDef
	.type IRIsVarDef, @function

IRIsVarDef:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 88(a0)
	andi        t0, t0, 1
	snez        a0, t0

	// *** Basic block 1

.IRIsVarDef_label_14:
	ret         
.func_end_IRIsVarDef:
	.size IRIsVarDef, .func_end_IRIsVarDef-IRIsVarDef

	.global IRIsVarRef
	.type IRIsVarRef, @function

IRIsVarRef:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 88(a0)
	andi        t0, t0, 2
	snez        a0, t0

	// *** Basic block 1

.IRIsVarRef_label_14:
	ret         
.func_end_IRIsVarRef:
	.size IRIsVarRef, .func_end_IRIsVarRef-IRIsVarRef

	.global IRIsExpression
	.type IRIsExpression, @function

IRIsExpression:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	li          t1, 1		// 0x1 ASCII \x1
	blt         t0, t1, .IRIsExpression_label_266

	// *** Basic block 1

	li          t1, 129		// 0x81 ASCII \x81
	blt         t1, t0, .IRIsExpression_label_266

	// *** Basic block 2

	addi        t0, t0, -1
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .IRIsExpression_label_171

	// *** Basic block 4

	j           .IRIsExpression_label_157

	// *** Basic block 5

	j           .IRIsExpression_label_158

	// *** Basic block 6

	j           .IRIsExpression_label_156

	// *** Basic block 7

	j           .IRIsExpression_label_159

	// *** Basic block 8

	j           .IRIsExpression_label_160

	// *** Basic block 9

	j           .IRIsExpression_label_161

	// *** Basic block 10

	j           .IRIsExpression_label_162

	// *** Basic block 11

	j           .IRIsExpression_label_163

	// *** Basic block 12

	j           .IRIsExpression_label_164

	// *** Basic block 13

	j           .IRIsExpression_label_165

	// *** Basic block 14

	j           .IRIsExpression_label_166

	// *** Basic block 15

	j           .IRIsExpression_label_167

	// *** Basic block 16

	j           .IRIsExpression_label_168

	// *** Basic block 17

	j           .IRIsExpression_label_169

	// *** Basic block 18

	j           .IRIsExpression_label_170

	// *** Basic block 19

	j           .IRIsExpression_label_266

	// *** Basic block 20

	j           .IRIsExpression_label_266

	// *** Basic block 21

	j           .IRIsExpression_label_172

	// *** Basic block 22

	j           .IRIsExpression_label_173

	// *** Basic block 23

	j           .IRIsExpression_label_174

	// *** Basic block 24

	j           .IRIsExpression_label_175

	// *** Basic block 25

	j           .IRIsExpression_label_176

	// *** Basic block 26

	j           .IRIsExpression_label_177

	// *** Basic block 27

	j           .IRIsExpression_label_178

	// *** Basic block 28

	j           .IRIsExpression_label_179

	// *** Basic block 29

	j           .IRIsExpression_label_180

	// *** Basic block 30

	j           .IRIsExpression_label_181

	// *** Basic block 31

	j           .IRIsExpression_label_182

	// *** Basic block 32

	j           .IRIsExpression_label_183

	// *** Basic block 33

	j           .IRIsExpression_label_184

	// *** Basic block 34

	j           .IRIsExpression_label_185

	// *** Basic block 35

	j           .IRIsExpression_label_186

	// *** Basic block 36

	j           .IRIsExpression_label_187

	// *** Basic block 37

	j           .IRIsExpression_label_188

	// *** Basic block 38

	j           .IRIsExpression_label_189

	// *** Basic block 39

	j           .IRIsExpression_label_190

	// *** Basic block 40

	j           .IRIsExpression_label_191

	// *** Basic block 41

	j           .IRIsExpression_label_192

	// *** Basic block 42

	j           .IRIsExpression_label_193

	// *** Basic block 43

	j           .IRIsExpression_label_194

	// *** Basic block 44

	j           .IRIsExpression_label_195

	// *** Basic block 45

	j           .IRIsExpression_label_196

	// *** Basic block 46

	j           .IRIsExpression_label_197

	// *** Basic block 47

	j           .IRIsExpression_label_198

	// *** Basic block 48

	j           .IRIsExpression_label_199

	// *** Basic block 49

	j           .IRIsExpression_label_200

	// *** Basic block 50

	j           .IRIsExpression_label_201

	// *** Basic block 51

	j           .IRIsExpression_label_202

	// *** Basic block 52

	j           .IRIsExpression_label_203

	// *** Basic block 53

	j           .IRIsExpression_label_204

	// *** Basic block 54

	j           .IRIsExpression_label_205

	// *** Basic block 55

	j           .IRIsExpression_label_206

	// *** Basic block 56

	j           .IRIsExpression_label_207

	// *** Basic block 57

	j           .IRIsExpression_label_208

	// *** Basic block 58

	j           .IRIsExpression_label_209

	// *** Basic block 59

	j           .IRIsExpression_label_210

	// *** Basic block 60

	j           .IRIsExpression_label_211

	// *** Basic block 61

	j           .IRIsExpression_label_212

	// *** Basic block 62

	j           .IRIsExpression_label_213

	// *** Basic block 63

	j           .IRIsExpression_label_214

	// *** Basic block 64

	j           .IRIsExpression_label_215

	// *** Basic block 65

	j           .IRIsExpression_label_216

	// *** Basic block 66

	j           .IRIsExpression_label_234

	// *** Basic block 67

	j           .IRIsExpression_label_235

	// *** Basic block 68

	j           .IRIsExpression_label_236

	// *** Basic block 69

	j           .IRIsExpression_label_237

	// *** Basic block 70

	j           .IRIsExpression_label_238

	// *** Basic block 71

	j           .IRIsExpression_label_239

	// *** Basic block 72

	j           .IRIsExpression_label_240

	// *** Basic block 73

	j           .IRIsExpression_label_241

	// *** Basic block 74

	j           .IRIsExpression_label_242

	// *** Basic block 75

	j           .IRIsExpression_label_243

	// *** Basic block 76

	j           .IRIsExpression_label_244

	// *** Basic block 77

	j           .IRIsExpression_label_245

	// *** Basic block 78

	j           .IRIsExpression_label_246

	// *** Basic block 79

	j           .IRIsExpression_label_247

	// *** Basic block 80

	j           .IRIsExpression_label_248

	// *** Basic block 81

	j           .IRIsExpression_label_249

	// *** Basic block 82

	j           .IRIsExpression_label_250

	// *** Basic block 83

	j           .IRIsExpression_label_251

	// *** Basic block 84

	j           .IRIsExpression_label_252

	// *** Basic block 85

	j           .IRIsExpression_label_253

	// *** Basic block 86

	j           .IRIsExpression_label_254

	// *** Basic block 87

	j           .IRIsExpression_label_255

	// *** Basic block 88

	j           .IRIsExpression_label_256

	// *** Basic block 89

	j           .IRIsExpression_label_257

	// *** Basic block 90

	j           .IRIsExpression_label_266

	// *** Basic block 91

	j           .IRIsExpression_label_266

	// *** Basic block 92

	j           .IRIsExpression_label_266

	// *** Basic block 93

	j           .IRIsExpression_label_266

	// *** Basic block 94

	j           .IRIsExpression_label_232

	// *** Basic block 95

	j           .IRIsExpression_label_266

	// *** Basic block 96

	j           .IRIsExpression_label_266

	// *** Basic block 97

	j           .IRIsExpression_label_266

	// *** Basic block 98

	j           .IRIsExpression_label_217

	// *** Basic block 99

	j           .IRIsExpression_label_218

	// *** Basic block 100

	j           .IRIsExpression_label_219

	// *** Basic block 101

	j           .IRIsExpression_label_220

	// *** Basic block 102

	j           .IRIsExpression_label_221

	// *** Basic block 103

	j           .IRIsExpression_label_222

	// *** Basic block 104

	j           .IRIsExpression_label_266

	// *** Basic block 105

	j           .IRIsExpression_label_259

	// *** Basic block 106

	j           .IRIsExpression_label_258

	// *** Basic block 107

	j           .IRIsExpression_label_266

	// *** Basic block 108

	j           .IRIsExpression_label_266

	// *** Basic block 109

	j           .IRIsExpression_label_266

	// *** Basic block 110

	j           .IRIsExpression_label_266

	// *** Basic block 111

	j           .IRIsExpression_label_266

	// *** Basic block 112

	j           .IRIsExpression_label_223

	// *** Basic block 113

	j           .IRIsExpression_label_224

	// *** Basic block 114

	j           .IRIsExpression_label_225

	// *** Basic block 115

	j           .IRIsExpression_label_226

	// *** Basic block 116

	j           .IRIsExpression_label_227

	// *** Basic block 117

	j           .IRIsExpression_label_228

	// *** Basic block 118

	j           .IRIsExpression_label_229

	// *** Basic block 119

	j           .IRIsExpression_label_230

	// *** Basic block 120

	j           .IRIsExpression_label_266

	// *** Basic block 121

	j           .IRIsExpression_label_266

	// *** Basic block 122

	j           .IRIsExpression_label_266

	// *** Basic block 123

	j           .IRIsExpression_label_260

	// *** Basic block 124

	j           .IRIsExpression_label_231

	// *** Basic block 125

	j           .IRIsExpression_label_266

	// *** Basic block 126

	j           .IRIsExpression_label_266

	// *** Basic block 127

	j           .IRIsExpression_label_266

	// *** Basic block 128

	j           .IRIsExpression_label_266

	// *** Basic block 129

	j           .IRIsExpression_label_266

	// *** Basic block 130

	j           .IRIsExpression_label_266

	// *** Basic block 131

	j           .IRIsExpression_label_233

	// *** Basic block 132

.IRIsExpression_label_156:

	// *** Basic block 133

.IRIsExpression_label_157:

	// *** Basic block 134

.IRIsExpression_label_158:

	// *** Basic block 135

.IRIsExpression_label_159:

	// *** Basic block 136

.IRIsExpression_label_160:

	// *** Basic block 137

.IRIsExpression_label_161:

	// *** Basic block 138

.IRIsExpression_label_162:

	// *** Basic block 139

.IRIsExpression_label_163:

	// *** Basic block 140

.IRIsExpression_label_164:

	// *** Basic block 141

.IRIsExpression_label_165:

	// *** Basic block 142

.IRIsExpression_label_166:

	// *** Basic block 143

.IRIsExpression_label_167:

	// *** Basic block 144

.IRIsExpression_label_168:

	// *** Basic block 145

.IRIsExpression_label_169:

	// *** Basic block 146

.IRIsExpression_label_170:

	// *** Basic block 147

.IRIsExpression_label_171:

	// *** Basic block 148

.IRIsExpression_label_172:

	// *** Basic block 149

.IRIsExpression_label_173:

	// *** Basic block 150

.IRIsExpression_label_174:

	// *** Basic block 151

.IRIsExpression_label_175:

	// *** Basic block 152

.IRIsExpression_label_176:

	// *** Basic block 153

.IRIsExpression_label_177:

	// *** Basic block 154

.IRIsExpression_label_178:

	// *** Basic block 155

.IRIsExpression_label_179:

	// *** Basic block 156

.IRIsExpression_label_180:

	// *** Basic block 157

.IRIsExpression_label_181:

	// *** Basic block 158

.IRIsExpression_label_182:

	// *** Basic block 159

.IRIsExpression_label_183:

	// *** Basic block 160

.IRIsExpression_label_184:

	// *** Basic block 161

.IRIsExpression_label_185:

	// *** Basic block 162

.IRIsExpression_label_186:

	// *** Basic block 163

.IRIsExpression_label_187:

	// *** Basic block 164

.IRIsExpression_label_188:

	// *** Basic block 165

.IRIsExpression_label_189:

	// *** Basic block 166

.IRIsExpression_label_190:

	// *** Basic block 167

.IRIsExpression_label_191:

	// *** Basic block 168

.IRIsExpression_label_192:

	// *** Basic block 169

.IRIsExpression_label_193:

	// *** Basic block 170

.IRIsExpression_label_194:

	// *** Basic block 171

.IRIsExpression_label_195:

	// *** Basic block 172

.IRIsExpression_label_196:

	// *** Basic block 173

.IRIsExpression_label_197:

	// *** Basic block 174

.IRIsExpression_label_198:

	// *** Basic block 175

.IRIsExpression_label_199:

	// *** Basic block 176

.IRIsExpression_label_200:

	// *** Basic block 177

.IRIsExpression_label_201:

	// *** Basic block 178

.IRIsExpression_label_202:

	// *** Basic block 179

.IRIsExpression_label_203:

	// *** Basic block 180

.IRIsExpression_label_204:

	// *** Basic block 181

.IRIsExpression_label_205:

	// *** Basic block 182

.IRIsExpression_label_206:

	// *** Basic block 183

.IRIsExpression_label_207:

	// *** Basic block 184

.IRIsExpression_label_208:

	// *** Basic block 185

.IRIsExpression_label_209:

	// *** Basic block 186

.IRIsExpression_label_210:

	// *** Basic block 187

.IRIsExpression_label_211:

	// *** Basic block 188

.IRIsExpression_label_212:

	// *** Basic block 189

.IRIsExpression_label_213:

	// *** Basic block 190

.IRIsExpression_label_214:

	// *** Basic block 191

.IRIsExpression_label_215:

	// *** Basic block 192

.IRIsExpression_label_216:

	// *** Basic block 193

.IRIsExpression_label_217:

	// *** Basic block 194

.IRIsExpression_label_218:

	// *** Basic block 195

.IRIsExpression_label_219:

	// *** Basic block 196

.IRIsExpression_label_220:

	// *** Basic block 197

.IRIsExpression_label_221:

	// *** Basic block 198

.IRIsExpression_label_222:

	// *** Basic block 199

.IRIsExpression_label_223:

	// *** Basic block 200

.IRIsExpression_label_224:

	// *** Basic block 201

.IRIsExpression_label_225:

	// *** Basic block 202

.IRIsExpression_label_226:

	// *** Basic block 203

.IRIsExpression_label_227:

	// *** Basic block 204

.IRIsExpression_label_228:

	// *** Basic block 205

.IRIsExpression_label_229:

	// *** Basic block 206

.IRIsExpression_label_230:

	// *** Basic block 207

.IRIsExpression_label_231:

	// *** Basic block 208

.IRIsExpression_label_232:

	// *** Basic block 209

.IRIsExpression_label_233:

	// *** Basic block 210

.IRIsExpression_label_234:

	// *** Basic block 211

.IRIsExpression_label_235:

	// *** Basic block 212

.IRIsExpression_label_236:

	// *** Basic block 213

.IRIsExpression_label_237:

	// *** Basic block 214

.IRIsExpression_label_238:

	// *** Basic block 215

.IRIsExpression_label_239:

	// *** Basic block 216

.IRIsExpression_label_240:

	// *** Basic block 217

.IRIsExpression_label_241:

	// *** Basic block 218

.IRIsExpression_label_242:

	// *** Basic block 219

.IRIsExpression_label_243:

	// *** Basic block 220

.IRIsExpression_label_244:

	// *** Basic block 221

.IRIsExpression_label_245:

	// *** Basic block 222

.IRIsExpression_label_246:

	// *** Basic block 223

.IRIsExpression_label_247:

	// *** Basic block 224

.IRIsExpression_label_248:

	// *** Basic block 225

.IRIsExpression_label_249:

	// *** Basic block 226

.IRIsExpression_label_250:

	// *** Basic block 227

.IRIsExpression_label_251:

	// *** Basic block 228

.IRIsExpression_label_252:

	// *** Basic block 229

.IRIsExpression_label_253:

	// *** Basic block 230

.IRIsExpression_label_254:

	// *** Basic block 231

.IRIsExpression_label_255:

	// *** Basic block 232

.IRIsExpression_label_256:

	// *** Basic block 233

.IRIsExpression_label_257:

	// *** Basic block 234

.IRIsExpression_label_258:

	// *** Basic block 235

.IRIsExpression_label_259:

	// *** Basic block 236

.IRIsExpression_label_260:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 237

.IRIsExpression_label_263:
	ret         

	// *** Basic block 238

.IRIsExpression_label_266:
	mv          a0, x0
	ret         
.func_end_IRIsExpression:
	.size IRIsExpression, .func_end_IRIsExpression-IRIsExpression

	.global IRIsCommutative
	.type IRIsCommutative, @function

IRIsCommutative:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	li          t1, 37		// 0x25 ASCII '%'
	beq         t0, t1, .IRIsCommutative_label_81

	// *** Basic block 1

	li          t1, 38		// 0x26 ASCII '&'
	beq         t0, t1, .IRIsCommutative_label_82

	// *** Basic block 2

	li          t1, 39		// 0x27 ASCII '''
	beq         t0, t1, .IRIsCommutative_label_83

	// *** Basic block 3

	li          t1, 40		// 0x28 ASCII '('
	beq         t0, t1, .IRIsCommutative_label_84

	// *** Basic block 4

	li          t1, 45		// 0x2d ASCII '-'
	beq         t0, t1, .IRIsCommutative_label_85

	// *** Basic block 5

	li          t1, 46		// 0x2e ASCII '.'
	beq         t0, t1, .IRIsCommutative_label_86

	// *** Basic block 6

	li          t1, 47		// 0x2f ASCII '/'
	beq         t0, t1, .IRIsCommutative_label_87

	// *** Basic block 7

	li          t1, 55		// 0x37 ASCII '7'
	beq         t0, t1, .IRIsCommutative_label_88

	// *** Basic block 8

	li          t1, 56		// 0x38 ASCII '8'
	beq         t0, t1, .IRIsCommutative_label_89

	// *** Basic block 9

	li          t1, 57		// 0x39 ASCII '9'
	beq         t0, t1, .IRIsCommutative_label_90

	// *** Basic block 10

	li          t1, 122		// 0x7a ASCII 'z'
	beq         t0, t1, .IRIsCommutative_label_91

	// *** Basic block 11

.IRIsCommutative_label_77:
	mv          a0, x0
	ret         

	// *** Basic block 12

.IRIsCommutative_label_81:

	// *** Basic block 13

.IRIsCommutative_label_82:

	// *** Basic block 14

.IRIsCommutative_label_83:

	// *** Basic block 15

.IRIsCommutative_label_84:

	// *** Basic block 16

.IRIsCommutative_label_85:

	// *** Basic block 17

.IRIsCommutative_label_86:

	// *** Basic block 18

.IRIsCommutative_label_87:

	// *** Basic block 19

.IRIsCommutative_label_88:

	// *** Basic block 20

.IRIsCommutative_label_89:

	// *** Basic block 21

.IRIsCommutative_label_90:

	// *** Basic block 22

.IRIsCommutative_label_91:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 23

.IRIsCommutative_label_94:
	ret         
.func_end_IRIsCommutative:
	.size IRIsCommutative, .func_end_IRIsCommutative-IRIsCommutative

	.global IRIsComparison
	.type IRIsComparison, @function

IRIsComparison:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	slti        t1, t0, 64
	not         a0, t1
	li          t1, 64		// 0x40 ASCII '@'
	blt         t0, t1, .IRIsComparison_label_19

	// *** Basic block 1

	li          t1, 87		// 0x57 ASCII 'W'
	slt         t0, t1, t0
	not         a0, t0

	// *** Basic block 2

.IRIsComparison_label_19:

	// *** Basic block 3

.IRIsComparison_label_21:
	ret         
.func_end_IRIsComparison:
	.size IRIsComparison, .func_end_IRIsComparison-IRIsComparison

	.global IRIsStore
	.type IRIsStore, @function

IRIsStore:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	li          t1, 30		// 0x1e ASCII \x1e
	beq         t0, t1, .IRIsStore_label_105

	// *** Basic block 1

	li          t1, 31		// 0x1f ASCII \x1f
	beq         t0, t1, .IRIsStore_label_106

	// *** Basic block 2

	li          t1, 32		// 0x20 ASCII ' '
	beq         t0, t1, .IRIsStore_label_107

	// *** Basic block 3

	li          t1, 33		// 0x21 ASCII '!'
	beq         t0, t1, .IRIsStore_label_108

	// *** Basic block 4

	li          t1, 34		// 0x22 ASCII '"'
	beq         t0, t1, .IRIsStore_label_109

	// *** Basic block 5

	li          t1, 35		// 0x23 ASCII '#'
	beq         t0, t1, .IRIsStore_label_110

	// *** Basic block 6

	li          t1, 36		// 0x24 ASCII '$'
	beq         t0, t1, .IRIsStore_label_111

	// *** Basic block 7

	li          t1, 103		// 0x67 ASCII 'g'
	beq         t0, t1, .IRIsStore_label_112

	// *** Basic block 8

	li          t1, 119		// 0x77 ASCII 'w'
	beq         t0, t1, .IRIsStore_label_118

	// *** Basic block 9

	li          t1, 120		// 0x78 ASCII 'x'
	beq         t0, t1, .IRIsStore_label_119

	// *** Basic block 10

	li          t1, 121		// 0x79 ASCII 'y'
	beq         t0, t1, .IRIsStore_label_113

	// *** Basic block 11

	li          t1, 128		// 0x80 ASCII \x80
	beq         t0, t1, .IRIsStore_label_114

	// *** Basic block 12

	li          t1, 129		// 0x81 ASCII \x81
	beq         t0, t1, .IRIsStore_label_116

	// *** Basic block 13

	li          t1, 130		// 0x82 ASCII \x82
	beq         t0, t1, .IRIsStore_label_115

	// *** Basic block 14

	li          t1, 131		// 0x83 ASCII \x83
	beq         t0, t1, .IRIsStore_label_117

	// *** Basic block 15

.IRIsStore_label_101:
	mv          a0, x0
	ret         

	// *** Basic block 16

.IRIsStore_label_105:

	// *** Basic block 17

.IRIsStore_label_106:

	// *** Basic block 18

.IRIsStore_label_107:

	// *** Basic block 19

.IRIsStore_label_108:

	// *** Basic block 20

.IRIsStore_label_109:

	// *** Basic block 21

.IRIsStore_label_110:

	// *** Basic block 22

.IRIsStore_label_111:

	// *** Basic block 23

.IRIsStore_label_112:

	// *** Basic block 24

.IRIsStore_label_113:

	// *** Basic block 25

.IRIsStore_label_114:

	// *** Basic block 26

.IRIsStore_label_115:

	// *** Basic block 27

.IRIsStore_label_116:

	// *** Basic block 28

.IRIsStore_label_117:

	// *** Basic block 29

.IRIsStore_label_118:

	// *** Basic block 30

.IRIsStore_label_119:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 31

.IRIsStore_label_122:
	ret         
.func_end_IRIsStore:
	.size IRIsStore, .func_end_IRIsStore-IRIsStore

	.global IRIsLoad
	.type IRIsLoad, @function

IRIsLoad:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	li          t1, 19		// 0x13 ASCII \x13
	beq         t0, t1, .IRIsLoad_label_92

	// *** Basic block 1

	li          t1, 20		// 0x14 ASCII \x14
	beq         t0, t1, .IRIsLoad_label_93

	// *** Basic block 2

	li          t1, 21		// 0x15 ASCII \x15
	beq         t0, t1, .IRIsLoad_label_94

	// *** Basic block 3

	li          t1, 22		// 0x16 ASCII \x16
	beq         t0, t1, .IRIsLoad_label_95

	// *** Basic block 4

	li          t1, 23		// 0x17 ASCII \x17
	beq         t0, t1, .IRIsLoad_label_96

	// *** Basic block 5

	li          t1, 24		// 0x18 ASCII \x18
	beq         t0, t1, .IRIsLoad_label_97

	// *** Basic block 6

	li          t1, 25		// 0x19 ASCII \x19
	beq         t0, t1, .IRIsLoad_label_98

	// *** Basic block 7

	li          t1, 26		// 0x1a ASCII \x1a
	beq         t0, t1, .IRIsLoad_label_99

	// *** Basic block 8

	li          t1, 27		// 0x1b ASCII \x1b
	beq         t0, t1, .IRIsLoad_label_100

	// *** Basic block 9

	li          t1, 28		// 0x1c ASCII \x1c
	beq         t0, t1, .IRIsLoad_label_101

	// *** Basic block 10

	li          t1, 29		// 0x1d ASCII \x1d
	beq         t0, t1, .IRIsLoad_label_102

	// *** Basic block 11

	li          t1, 103		// 0x67 ASCII 'g'
	beq         t0, t1, .IRIsLoad_label_103

	// *** Basic block 12

	li          t1, 121		// 0x79 ASCII 'y'
	beq         t0, t1, .IRIsLoad_label_104

	// *** Basic block 13

.IRIsLoad_label_88:
	mv          a0, x0
	ret         

	// *** Basic block 14

.IRIsLoad_label_92:

	// *** Basic block 15

.IRIsLoad_label_93:

	// *** Basic block 16

.IRIsLoad_label_94:

	// *** Basic block 17

.IRIsLoad_label_95:

	// *** Basic block 18

.IRIsLoad_label_96:

	// *** Basic block 19

.IRIsLoad_label_97:

	// *** Basic block 20

.IRIsLoad_label_98:

	// *** Basic block 21

.IRIsLoad_label_99:

	// *** Basic block 22

.IRIsLoad_label_100:

	// *** Basic block 23

.IRIsLoad_label_101:

	// *** Basic block 24

.IRIsLoad_label_102:

	// *** Basic block 25

.IRIsLoad_label_103:

	// *** Basic block 26

.IRIsLoad_label_104:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 27

.IRIsLoad_label_107:
	ret         
.func_end_IRIsLoad:
	.size IRIsLoad, .func_end_IRIsLoad-IRIsLoad

	.global IRIsResult
	.type IRIsResult, @function

IRIsResult:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	li          t1, 106		// 0x6a ASCII 'j'
	beq         t0, t1, .IRIsResult_label_45

	// *** Basic block 1

	li          t1, 107		// 0x6b ASCII 'k'
	beq         t0, t1, .IRIsResult_label_46

	// *** Basic block 2

	li          t1, 108		// 0x6c ASCII 'l'
	beq         t0, t1, .IRIsResult_label_47

	// *** Basic block 3

	li          t1, 109		// 0x6d ASCII 'm'
	beq         t0, t1, .IRIsResult_label_48

	// *** Basic block 4

	li          t1, 123		// 0x7b ASCII '{'
	beq         t0, t1, .IRIsResult_label_49

	// *** Basic block 5

.IRIsResult_label_41:
	mv          a0, x0
	ret         

	// *** Basic block 6

.IRIsResult_label_45:

	// *** Basic block 7

.IRIsResult_label_46:

	// *** Basic block 8

.IRIsResult_label_47:

	// *** Basic block 9

.IRIsResult_label_48:

	// *** Basic block 10

.IRIsResult_label_49:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 11

.IRIsResult_label_52:
	ret         
.func_end_IRIsResult:
	.size IRIsResult, .func_end_IRIsResult-IRIsResult

.PCend:
	.data
opcodes:
	.type   opcodes,@object
	.local  opcodes
	.size   opcodes,2128
	.p2align  3
	.word   0
	.space  4
	.long    .str.1
	.word   1
	.space  4
	.long    .str.2
	.word   4
	.space  4
	.long    .str.3
	.word   2
	.space  4
	.long    .str.4
	.word   3
	.space  4
	.long    .str.5
	.word   5
	.space  4
	.long    .str.6
	.word   6
	.space  4
	.long    .str.7
	.word   7
	.space  4
	.long    .str.8
	.word   8
	.space  4
	.long    .str.9
	.word   9
	.space  4
	.long    .str.10
	.word   10
	.space  4
	.long    .str.11
	.word   11
	.space  4
	.long    .str.12
	.word   12
	.space  4
	.long    .str.13
	.word   13
	.space  4
	.long    .str.14
	.word   14
	.space  4
	.long    .str.15
	.word   15
	.space  4
	.long    .str.16
	.word   16
	.space  4
	.long    .str.17
	.word   17
	.space  4
	.long    .str.18
	.word   18
	.space  4
	.long    .str.19
	.word   19
	.space  4
	.long    .str.20
	.word   20
	.space  4
	.long    .str.21
	.word   21
	.space  4
	.long    .str.22
	.word   22
	.space  4
	.long    .str.23
	.word   23
	.space  4
	.long    .str.24
	.word   24
	.space  4
	.long    .str.25
	.word   25
	.space  4
	.long    .str.26
	.word   26
	.space  4
	.long    .str.27
	.word   27
	.space  4
	.long    .str.28
	.word   28
	.space  4
	.long    .str.29
	.word   29
	.space  4
	.long    .str.30
	.word   30
	.space  4
	.long    .str.31
	.word   31
	.space  4
	.long    .str.32
	.word   32
	.space  4
	.long    .str.33
	.word   33
	.space  4
	.long    .str.34
	.word   34
	.space  4
	.long    .str.35
	.word   35
	.space  4
	.long    .str.36
	.word   36
	.space  4
	.long    .str.37
	.word   37
	.space  4
	.long    .str.38
	.word   38
	.space  4
	.long    .str.39
	.word   39
	.space  4
	.long    .str.40
	.word   40
	.space  4
	.long    .str.41
	.word   41
	.space  4
	.long    .str.42
	.word   42
	.space  4
	.long    .str.43
	.word   43
	.space  4
	.long    .str.44
	.word   44
	.space  4
	.long    .str.45
	.word   45
	.space  4
	.long    .str.46
	.word   46
	.space  4
	.long    .str.47
	.word   47
	.space  4
	.long    .str.48
	.word   48
	.space  4
	.long    .str.49
	.word   49
	.space  4
	.long    .str.50
	.word   50
	.space  4
	.long    .str.51
	.word   51
	.space  4
	.long    .str.52
	.word   52
	.space  4
	.long    .str.53
	.word   53
	.space  4
	.long    .str.54
	.word   54
	.space  4
	.long    .str.55
	.word   55
	.space  4
	.long    .str.56
	.word   56
	.space  4
	.long    .str.57
	.word   57
	.space  4
	.long    .str.58
	.word   58
	.space  4
	.long    .str.59
	.word   59
	.space  4
	.long    .str.60
	.word   60
	.space  4
	.long    .str.61
	.word   61
	.space  4
	.long    .str.62
	.word   62
	.space  4
	.long    .str.63
	.word   63
	.space  4
	.long    .str.64
	.word   64
	.space  4
	.long    .str.65
	.word   65
	.space  4
	.long    .str.66
	.word   66
	.space  4
	.long    .str.67
	.word   67
	.space  4
	.long    .str.68
	.word   68
	.space  4
	.long    .str.69
	.word   69
	.space  4
	.long    .str.70
	.word   70
	.space  4
	.long    .str.71
	.word   71
	.space  4
	.long    .str.72
	.word   72
	.space  4
	.long    .str.73
	.word   73
	.space  4
	.long    .str.74
	.word   74
	.space  4
	.long    .str.75
	.word   75
	.space  4
	.long    .str.76
	.word   76
	.space  4
	.long    .str.77
	.word   77
	.space  4
	.long    .str.78
	.word   78
	.space  4
	.long    .str.79
	.word   79
	.space  4
	.long    .str.80
	.word   80
	.space  4
	.long    .str.81
	.word   81
	.space  4
	.long    .str.82
	.word   82
	.space  4
	.long    .str.83
	.word   83
	.space  4
	.long    .str.84
	.word   84
	.space  4
	.long    .str.85
	.word   85
	.space  4
	.long    .str.86
	.word   86
	.space  4
	.long    .str.87
	.word   87
	.space  4
	.long    .str.88
	.word   88
	.space  4
	.long    .str.89
	.word   89
	.space  4
	.long    .str.90
	.word   90
	.space  4
	.long    .str.91
	.word   91
	.space  4
	.long    .str.92
	.word   92
	.space  4
	.long    .str.93
	.word   93
	.space  4
	.long    .str.94
	.word   94
	.space  4
	.long    .str.95
	.word   95
	.space  4
	.long    .str.96
	.word   96
	.space  4
	.long    .str.97
	.word   97
	.space  4
	.long    .str.98
	.word   98
	.space  4
	.long    .str.99
	.word   99
	.space  4
	.long    .str.100
	.word   100
	.space  4
	.long    .str.101
	.word   101
	.space  4
	.long    .str.102
	.word   102
	.space  4
	.long    .str.103
	.word   103
	.space  4
	.long    .str.104
	.word   104
	.space  4
	.long    .str.105
	.word   105
	.space  4
	.long    .str.106
	.word   18
	.space  4
	.long    .str.107
	.word   106
	.space  4
	.long    .str.108
	.word   107
	.space  4
	.long    .str.109
	.word   108
	.space  4
	.long    .str.110
	.word   109
	.space  4
	.long    .str.111
	.word   110
	.space  4
	.long    .str.112
	.word   111
	.space  4
	.long    .str.113
	.word   112
	.space  4
	.long    .str.114
	.word   113
	.space  4
	.long    .str.115
	.word   114
	.space  4
	.long    .str.116
	.word   115
	.space  4
	.long    .str.117
	.word   116
	.space  4
	.long    .str.118
	.word   117
	.space  4
	.long    .str.119
	.word   118
	.space  4
	.long    .str.120
	.word   119
	.space  4
	.long    .str.121
	.word   120
	.space  4
	.long    .str.122
	.word   121
	.space  4
	.long    .str.123
	.word   122
	.space  4
	.long    .str.124
	.word   123
	.space  4
	.long    .str.125
	.word   124
	.space  4
	.long    .str.126
	.word   125
	.space  4
	.long    .str.127
	.word   126
	.space  4
	.long    .str.128
	.word   127
	.space  4
	.long    .str.129
	.word   128
	.space  4
	.long    .str.130
	.word   129
	.space  4
	.long    .str.131
	.word   130
	.space  4
	.long    .str.132
	.word   131
	.space  4
	.long    .str.133

next_ir_id:
	.type   next_ir_id,@object
	.local  next_ir_id
	.size   next_ir_id,4
	.p2align  2
	.word   1

type_table:
	.type   type_table,@object
	.local  type_table
	.size   type_table,176
	.p2align  3
	.global TypeIsInt
	.long    TypeIsInt
	.word   4
	.space  4
	.global TypeIsBool
	.long    TypeIsBool
	.word   4
	.space  4
	.global TypeIsShort
	.long    TypeIsShort
	.word   3
	.space  4
	.global TypeIsChar
	.long    TypeIsChar
	.word   2
	.space  4
	.global TypeIsLong
	.long    TypeIsLong
	.word   5
	.space  4
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.word   5
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   6
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   7
	.space  4
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.word   7
	.space  4
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   8
	.space  4
	.word   0
	.space  4
	.word   0
	.space  4

kFlagNames:
	.type   kFlagNames,@object
	.local  kFlagNames
	.size   kFlagNames,56
	.p2align  3
	.long    .str.171
	.long    .str.172
	.long    .str.173
	.long    .str.174
	.long    .str.175
	.long    .str.176
	.long    .str.177

	.type   current_location,@object
	.local  current_location
	.comm   current_location,8,8

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "nop"
	.type .str.1, @object
	.size .str.1, 4

.str.2:
	.asciz "tmp"
	.type .str.2, @object
	.size .str.2, 4

.str.3:
	.asciz "consti"
	.type .str.3, @object
	.size .str.3, 7

.str.4:
	.asciz "constb"
	.type .str.4, @object
	.size .str.4, 7

.str.5:
	.asciz "consts"
	.type .str.5, @object
	.size .str.5, 7

.str.6:
	.asciz "constl"
	.type .str.6, @object
	.size .str.6, 7

.str.7:
	.asciz "constf"
	.type .str.7, @object
	.size .str.7, 7

.str.8:
	.asciz "constd"
	.type .str.8, @object
	.size .str.8, 7

.str.9:
	.asciz "consta"
	.type .str.9, @object
	.size .str.9, 7

.str.10:
	.asciz "movi"
	.type .str.10, @object
	.size .str.10, 5

.str.11:
	.asciz "movf"
	.type .str.11, @object
	.size .str.11, 5

.str.12:
	.asciz "movd"
	.type .str.12, @object
	.size .str.12, 5

.str.13:
	.asciz "mova"
	.type .str.13, @object
	.size .str.13, 5

.str.14:
	.asciz "rmovi"
	.type .str.14, @object
	.size .str.14, 6

.str.15:
	.asciz "rmovf"
	.type .str.15, @object
	.size .str.15, 6

.str.16:
	.asciz "rmovd"
	.type .str.16, @object
	.size .str.16, 6

.str.17:
	.asciz "rmova"
	.type .str.17, @object
	.size .str.17, 6

.str.18:
	.asciz "label"
	.type .str.18, @object
	.size .str.18, 6

.str.19:
	.asciz "namedlabel"
	.type .str.19, @object
	.size .str.19, 11

.str.20:
	.asciz "loadi"
	.type .str.20, @object
	.size .str.20, 6

.str.21:
	.asciz "loadb"
	.type .str.21, @object
	.size .str.21, 6

.str.22:
	.asciz "loadl"
	.type .str.22, @object
	.size .str.22, 6

.str.23:
	.asciz "loads"
	.type .str.23, @object
	.size .str.23, 6

.str.24:
	.asciz "loadui"
	.type .str.24, @object
	.size .str.24, 7

.str.25:
	.asciz "loadub"
	.type .str.25, @object
	.size .str.25, 7

.str.26:
	.asciz "loadus"
	.type .str.26, @object
	.size .str.26, 7

.str.27:
	.asciz "loadf"
	.type .str.27, @object
	.size .str.27, 6

.str.28:
	.asciz "loadd"
	.type .str.28, @object
	.size .str.28, 6

.str.29:
	.asciz "loada"
	.type .str.29, @object
	.size .str.29, 6

.str.30:
	.asciz "structarg"
	.type .str.30, @object
	.size .str.30, 10

.str.31:
	.asciz "storei"
	.type .str.31, @object
	.size .str.31, 7

.str.32:
	.asciz "storeb"
	.type .str.32, @object
	.size .str.32, 7

.str.33:
	.asciz "stores"
	.type .str.33, @object
	.size .str.33, 7

.str.34:
	.asciz "storel"
	.type .str.34, @object
	.size .str.34, 7

.str.35:
	.asciz "storef"
	.type .str.35, @object
	.size .str.35, 7

.str.36:
	.asciz "stored"
	.type .str.36, @object
	.size .str.36, 7

.str.37:
	.asciz "storea"
	.type .str.37, @object
	.size .str.37, 7

.str.38:
	.asciz "addi"
	.type .str.38, @object
	.size .str.38, 5

.str.39:
	.asciz "addf"
	.type .str.39, @object
	.size .str.39, 5

.str.40:
	.asciz "addd"
	.type .str.40, @object
	.size .str.40, 5

.str.41:
	.asciz "adda"
	.type .str.41, @object
	.size .str.41, 5

.str.42:
	.asciz "subi"
	.type .str.42, @object
	.size .str.42, 5

.str.43:
	.asciz "subf"
	.type .str.43, @object
	.size .str.43, 5

.str.44:
	.asciz "subd"
	.type .str.44, @object
	.size .str.44, 5

.str.45:
	.asciz "suba"
	.type .str.45, @object
	.size .str.45, 5

.str.46:
	.asciz "muli"
	.type .str.46, @object
	.size .str.46, 5

.str.47:
	.asciz "mulf"
	.type .str.47, @object
	.size .str.47, 5

.str.48:
	.asciz "muld"
	.type .str.48, @object
	.size .str.48, 5

.str.49:
	.asciz "divi"
	.type .str.49, @object
	.size .str.49, 5

.str.50:
	.asciz "divf"
	.type .str.50, @object
	.size .str.50, 5

.str.51:
	.asciz "divd"
	.type .str.51, @object
	.size .str.51, 5

.str.52:
	.asciz "modi"
	.type .str.52, @object
	.size .str.52, 5

.str.53:
	.asciz "lsri"
	.type .str.53, @object
	.size .str.53, 5

.str.54:
	.asciz "asri"
	.type .str.54, @object
	.size .str.54, 5

.str.55:
	.asciz "lsli"
	.type .str.55, @object
	.size .str.55, 5

.str.56:
	.asciz "ori"
	.type .str.56, @object
	.size .str.56, 4

.str.57:
	.asciz "andi"
	.type .str.57, @object
	.size .str.57, 5

.str.58:
	.asciz "xori"
	.type .str.58, @object
	.size .str.58, 5

.str.59:
	.asciz "noti"
	.type .str.59, @object
	.size .str.59, 5

.str.60:
	.asciz "nota"
	.type .str.60, @object
	.size .str.60, 5

.str.61:
	.asciz "onescomp"
	.type .str.61, @object
	.size .str.61, 9

.str.62:
	.asciz "negi"
	.type .str.62, @object
	.size .str.62, 5

.str.63:
	.asciz "negf"
	.type .str.63, @object
	.size .str.63, 5

.str.64:
	.asciz "negd"
	.type .str.64, @object
	.size .str.64, 5

.str.65:
	.asciz "cmpeqi"
	.type .str.65, @object
	.size .str.65, 7

.str.66:
	.asciz "cmpnei"
	.type .str.66, @object
	.size .str.66, 7

.str.67:
	.asciz "cmplti"
	.type .str.67, @object
	.size .str.67, 7

.str.68:
	.asciz "cmplei"
	.type .str.68, @object
	.size .str.68, 7

.str.69:
	.asciz "cmpgti"
	.type .str.69, @object
	.size .str.69, 7

.str.70:
	.asciz "cmpgei"
	.type .str.70, @object
	.size .str.70, 7

.str.71:
	.asciz "cmpeqf"
	.type .str.71, @object
	.size .str.71, 7

.str.72:
	.asciz "cmpnef"
	.type .str.72, @object
	.size .str.72, 7

.str.73:
	.asciz "cmpltf"
	.type .str.73, @object
	.size .str.73, 7

.str.74:
	.asciz "cmplef"
	.type .str.74, @object
	.size .str.74, 7

.str.75:
	.asciz "cmpgtf"
	.type .str.75, @object
	.size .str.75, 7

.str.76:
	.asciz "cmpgef"
	.type .str.76, @object
	.size .str.76, 7

.str.77:
	.asciz "cmpeqd"
	.type .str.77, @object
	.size .str.77, 7

.str.78:
	.asciz "cmpned"
	.type .str.78, @object
	.size .str.78, 7

.str.79:
	.asciz "cmpltd"
	.type .str.79, @object
	.size .str.79, 7

.str.80:
	.asciz "cmpled"
	.type .str.80, @object
	.size .str.80, 7

.str.81:
	.asciz "cmpgtd"
	.type .str.81, @object
	.size .str.81, 7

.str.82:
	.asciz "cmpged"
	.type .str.82, @object
	.size .str.82, 7

.str.83:
	.asciz "cmpeqa"
	.type .str.83, @object
	.size .str.83, 7

.str.84:
	.asciz "cmpnea"
	.type .str.84, @object
	.size .str.84, 7

.str.85:
	.asciz "cmplta"
	.type .str.85, @object
	.size .str.85, 7

.str.86:
	.asciz "cmplea"
	.type .str.86, @object
	.size .str.86, 7

.str.87:
	.asciz "cmpgta"
	.type .str.87, @object
	.size .str.87, 7

.str.88:
	.asciz "cmpgea"
	.type .str.88, @object
	.size .str.88, 7

.str.89:
	.asciz "btrue"
	.type .str.89, @object
	.size .str.89, 6

.str.90:
	.asciz "bfalse"
	.type .str.90, @object
	.size .str.90, 7

.str.91:
	.asciz "bra"
	.type .str.91, @object
	.size .str.91, 4

.str.92:
	.asciz "cbra"
	.type .str.92, @object
	.size .str.92, 5

.str.93:
	.asciz "calla"
	.type .str.93, @object
	.size .str.93, 6

.str.94:
	.asciz "ret"
	.type .str.94, @object
	.size .str.94, 4

.str.95:
	.asciz "enter"
	.type .str.95, @object
	.size .str.95, 6

.str.96:
	.asciz "leave"
	.type .str.96, @object
	.size .str.96, 6

.str.97:
	.asciz "localvar"
	.type .str.97, @object
	.size .str.97, 9

.str.98:
	.asciz "externvar"
	.type .str.98, @object
	.size .str.98, 10

.str.99:
	.asciz "argument"
	.type .str.99, @object
	.size .str.99, 9

.str.100:
	.asciz "staticvar"
	.type .str.100, @object
	.size .str.100, 10

.str.101:
	.asciz "tempvar"
	.type .str.101, @object
	.size .str.101, 8

.str.102:
	.asciz "ssavar"
	.type .str.102, @object
	.size .str.102, 7

.str.103:
	.asciz "structreturn"
	.type .str.103, @object
	.size .str.103, 13

.str.104:
	.asciz "addressof"
	.type .str.104, @object
	.size .str.104, 10

.str.105:
	.asciz "literalref"
	.type .str.105, @object
	.size .str.105, 11

.str.106:
	.asciz "loc"
	.type .str.106, @object
	.size .str.106, 4

.str.107:
	.asciz "label"
	.type .str.107, @object
	.size .str.107, 6

.str.108:
	.asciz "resulti"
	.type .str.108, @object
	.size .str.108, 8

.str.109:
	.asciz "resultf"
	.type .str.109, @object
	.size .str.109, 8

.str.110:
	.asciz "resultd"
	.type .str.110, @object
	.size .str.110, 8

.str.111:
	.asciz "resulta"
	.type .str.111, @object
	.size .str.111, 8

.str.112:
	.asciz "i2f"
	.type .str.112, @object
	.size .str.112, 4

.str.113:
	.asciz "i2d"
	.type .str.113, @object
	.size .str.113, 4

.str.114:
	.asciz "f2d"
	.type .str.114, @object
	.size .str.114, 4

.str.115:
	.asciz "d2f"
	.type .str.115, @object
	.size .str.115, 4

.str.116:
	.asciz "f2i"
	.type .str.116, @object
	.size .str.116, 4

.str.117:
	.asciz "d2i"
	.type .str.117, @object
	.size .str.117, 4

.str.118:
	.asciz "maski"
	.type .str.118, @object
	.size .str.118, 6

.str.119:
	.asciz "signextendi"
	.type .str.119, @object
	.size .str.119, 12

.str.120:
	.asciz "aligni"
	.type .str.120, @object
	.size .str.120, 7

.str.121:
	.asciz "memzero"
	.type .str.121, @object
	.size .str.121, 8

.str.122:
	.asciz "memcpy"
	.type .str.122, @object
	.size .str.122, 7

.str.123:
	.asciz "cast"
	.type .str.123, @object
	.size .str.123, 5

.str.124:
	.asciz "phi"
	.type .str.124, @object
	.size .str.124, 4

.str.125:
	.asciz "asm"
	.type .str.125, @object
	.size .str.125, 4

.str.126:
	.asciz "nvroval"
	.type .str.126, @object
	.size .str.126, 8

.str.127:
	.asciz "decsp"
	.type .str.127, @object
	.size .str.127, 6

.str.128:
	.asciz "savesp"
	.type .str.128, @object
	.size .str.128, 7

.str.129:
	.asciz "restoresp"
	.type .str.129, @object
	.size .str.129, 10

.str.130:
	.asciz "builtin_va_start"
	.type .str.130, @object
	.size .str.130, 17

.str.131:
	.asciz "builtin_va_arg"
	.type .str.131, @object
	.size .str.131, 15

.str.132:
	.asciz "builtin_va_end"
	.type .str.132, @object
	.size .str.132, 15

.str.133:
	.asciz "builtin_va_copy"
	.type .str.133, @object
	.size .str.133, 16

.str.134:
	.asciz "unknown"
	.type .str.134, @object
	.size .str.134, 8

.str.135:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.135, @object
	.size .str.135, 30

.str.136:
	.asciz "ir.c"
	.type .str.136, @object
	.size .str.136, 5

.str.137:
	.asciz "IRInList(to)"
	.type .str.137, @object
	.size .str.137, 13

.str.138:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.138, @object
	.size .str.138, 30

.str.139:
	.asciz "ir.c"
	.type .str.139, @object
	.size .str.139, 5

.str.140:
	.asciz "inst->flags == 0"
	.type .str.140, @object
	.size .str.140, 17

.str.141:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.141, @object
	.size .str.141, 30

.str.142:
	.asciz "ir.c"
	.type .str.142, @object
	.size .str.142, 5

.str.143:
	.asciz "inst->flags == 0"
	.type .str.143, @object
	.size .str.143, 17

.str.144:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.144, @object
	.size .str.144, 30

.str.145:
	.asciz "ir.c"
	.type .str.145, @object
	.size .str.145, 5

.str.146:
	.asciz "false"
	.type .str.146, @object
	.size .str.146, 6

.str.147:
	.asciz "$%d %s("
	.type .str.147, @object
	.size .str.147, 8

.str.148:
	.asciz "(null)"
	.type .str.148, @object
	.size .str.148, 1

.str.149:
	.asciz "%s$%d"
	.type .str.149, @object
	.size .str.149, 6

.str.150:
	.asciz ", "
	.type .str.150, @object
	.size .str.150, 3

.str.151:
	.asciz ")"
	.type .str.151, @object
	.size .str.151, 2

.str.152:
	.asciz " ["
	.type .str.152, @object
	.size .str.152, 3

.str.153:
	.asciz "(null)"
	.type .str.153, @object
	.size .str.153, 1

.str.154:
	.asciz "%s$%d"
	.type .str.154, @object
	.size .str.154, 6

.str.155:
	.asciz ", "
	.type .str.155, @object
	.size .str.155, 3

.str.156:
	.asciz "]"
	.type .str.156, @object
	.size .str.156, 2

.str.157:
	.asciz " %" PRId64 ""
	.type .str.157, @object
	.size .str.157, 6

.str.158:
	.asciz " %g"
	.type .str.158, @object
	.size .str.158, 4

.str.159:
	.asciz " %s"
	.type .str.159, @object
	.size .str.159, 4

.str.160:
	.asciz " %s, %d, %d, %d"
	.type .str.160, @object
	.size .str.160, 16

.str.161:
	.asciz " [tail]"
	.type .str.161, @object
	.size .str.161, 8

.str.162:
	.asciz " *%zd"
	.type .str.162, @object
	.size .str.162, 6

.str.163:
	.asciz " DEF %s"
	.type .str.163, @object
	.size .str.163, 8

.str.164:
	.asciz " REF %s"
	.type .str.164, @object
	.size .str.164, 8

.str.165:
	.asciz " {"
	.type .str.165, @object
	.size .str.165, 3

.str.166:
	.asciz "(null)"
	.type .str.166, @object
	.size .str.166, 1

.str.167:
	.asciz "%s%s"
	.type .str.167, @object
	.size .str.167, 5

.str.168:
	.asciz ","
	.type .str.168, @object
	.size .str.168, 2

.str.169:
	.asciz "}"
	.type .str.169, @object
	.size .str.169, 2

.str.170:
	.asciz "\n"
	.type .str.170, @object
	.size .str.170, 2

.str.171:
	.asciz "vardef"
	.type .str.171, @object
	.size .str.171, 7

.str.172:
	.asciz "varuse"
	.type .str.172, @object
	.size .str.172, 7

.str.173:
	.asciz "tailcall"
	.type .str.173, @object
	.size .str.173, 9

.str.174:
	.asciz "returnjump"
	.type .str.174, @object
	.size .str.174, 11

.str.175:
	.asciz "rvocall"
	.type .str.175, @object
	.size .str.175, 8

.str.176:
	.asciz "nrvomarker"
	.type .str.176, @object
	.size .str.176, 11

.str.177:
	.asciz "jumptablebranch"
	.type .str.177, @object
	.size .str.177, 16

