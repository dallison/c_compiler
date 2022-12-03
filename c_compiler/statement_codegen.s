	.file   "statement_codegen.c"
	.text
	.option pic
.PCbegin:
	.local  GenerateDeclarationList
	.type GenerateDeclarationList, @function

GenerateDeclarationList:

	// *** Basic block 0

	.global GenerateStatement
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
	ld          t0, 56(a1)
	ld          s2, 8(t0)
	mv          s3, x0
	bge         x0, s2, .GenerateDeclarationList_label_38

	// *** Basic block 1

	ld          s4, 0(t0)

	// *** Basic block 2

.GenerateDeclarationList_label_25:
	slli        t0, s3, 3
	add         t0, s4, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        GenerateStatement

	// *** Basic block 3

.GenerateDeclarationList_label_34:
	addi        s3, s3, 1
	bge         s3, s2, .GenerateDeclarationList_label_25

	// *** Basic block 4

.GenerateDeclarationList_label_38:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GenerateDeclarationList:
	.size GenerateDeclarationList, .func_end_GenerateDeclarationList-GenerateDeclarationList

	.local  FindEnclosingLoop
	.type FindEnclosingLoop, @function

FindEnclosingLoop:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	beq         t0, x0, .FindEnclosingLoop_label_58

	// *** Basic block 1

.FindEnclosingLoop_label_16:
	lw          t1, 0(t0)
	li          t2, 32		// 0x20 ASCII ' '
	beq         t1, t2, .FindEnclosingLoop_label_51

	// *** Basic block 2

	li          t2, 35		// 0x23 ASCII '#'
	beq         t1, t2, .FindEnclosingLoop_label_41

	// *** Basic block 3

	li          t2, 72		// 0x48 ASCII 'H'
	beq         t1, t2, .FindEnclosingLoop_label_50

	// *** Basic block 4

.FindEnclosingLoop_label_33:

	// *** Basic block 5

.FindEnclosingLoop_label_34:
	ld          t0, 24(t0)
	bne         t0, x0, .FindEnclosingLoop_label_16

	// *** Basic block 6

	j           .FindEnclosingLoop_label_58

	// *** Basic block 7

.FindEnclosingLoop_label_41:
	mv          t1, t0
	ld          a0, 80(t1)

	// *** Basic block 8

.FindEnclosingLoop_label_47:
	ret         

	// *** Basic block 9

.FindEnclosingLoop_label_50:

	// *** Basic block 10

.FindEnclosingLoop_label_51:
	mv          t2, t0
	ld          a0, 64(t2)
	ret         

	// *** Basic block 11

.FindEnclosingLoop_label_58:
	mv          a0, x0
	ret         
.func_end_FindEnclosingLoop:
	.size FindEnclosingLoop, .func_end_FindEnclosingLoop-FindEnclosingLoop

	.local  FindEnclosingLoopOrSwitch
	.type FindEnclosingLoopOrSwitch, @function

FindEnclosingLoopOrSwitch:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	beq         t0, x0, .FindEnclosingLoopOrSwitch_label_71

	// *** Basic block 1

.FindEnclosingLoopOrSwitch_label_17:
	lw          t1, 0(t0)
	li          t2, 32		// 0x20 ASCII ' '
	beq         t1, t2, .FindEnclosingLoopOrSwitch_label_57

	// *** Basic block 2

	li          t2, 35		// 0x23 ASCII '#'
	beq         t1, t2, .FindEnclosingLoopOrSwitch_label_47

	// *** Basic block 3

	li          t2, 70		// 0x46 ASCII 'F'
	beq         t1, t2, .FindEnclosingLoopOrSwitch_label_64

	// *** Basic block 4

	li          t2, 72		// 0x48 ASCII 'H'
	beq         t1, t2, .FindEnclosingLoopOrSwitch_label_56

	// *** Basic block 5

.FindEnclosingLoopOrSwitch_label_39:

	// *** Basic block 6

.FindEnclosingLoopOrSwitch_label_40:
	ld          t0, 24(t0)
	bne         t0, x0, .FindEnclosingLoopOrSwitch_label_17

	// *** Basic block 7

	j           .FindEnclosingLoopOrSwitch_label_71

	// *** Basic block 8

.FindEnclosingLoopOrSwitch_label_47:
	mv          t2, t0
	ld          a0, 80(t2)

	// *** Basic block 9

.FindEnclosingLoopOrSwitch_label_53:
	ret         

	// *** Basic block 10

.FindEnclosingLoopOrSwitch_label_56:

	// *** Basic block 11

.FindEnclosingLoopOrSwitch_label_57:
	mv          t3, t0
	ld          a0, 64(t3)
	ret         

	// *** Basic block 12

.FindEnclosingLoopOrSwitch_label_64:
	mv          t1, t0
	ld          a0, 64(t1)
	ret         

	// *** Basic block 13

.FindEnclosingLoopOrSwitch_label_71:
	mv          a0, x0
	ret         
.func_end_FindEnclosingLoopOrSwitch:
	.size FindEnclosingLoopOrSwitch, .func_end_FindEnclosingLoopOrSwitch-FindEnclosingLoopOrSwitch

	.local  ContainsVLA
	.type ContainsVLA, @function

ContainsVLA:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, x0
	ld          t1, 56(a0)
	ld          t2, 8(t1)
	bge         x0, t2, .ContainsVLA_label_72

	// *** Basic block 1

	ld          t1, 0(t1)

	// *** Basic block 2

.ContainsVLA_label_27:
	slli        t3, t0, 3
	add         t1, t1, t3
	ld          t3, 0(t1)
	ld          t1, 16(t3)
	lw          t5, 16(t1)
	addi        t6, t5, -2
	seqz        t4, t6
	li          t6, 2		// 0x2 ASCII \x2
	beq         t5, t6, .ContainsVLA_label_48

	// *** Basic block 3

	addi        t5, t5, -1
	seqz        t4, t5

	// *** Basic block 4

.ContainsVLA_label_48:
	beqz        t4, .ContainsVLA_label_55

	// *** Basic block 5

	addi        t1, t1, 32
	lb          t1, 16(t1)
	slli        t1, t1, 61
	srai        t4, t1, 63

	// *** Basic block 6

.ContainsVLA_label_55:

	// *** Basic block 7

.ContainsVLA_label_57:
	beqz        t4, .ContainsVLA_label_67

	// *** Basic block 8

	j           .ContainsVLA_label_60

	// *** Basic block 9

.ContainsVLA_label_60:
	ld          a0, 72(t3)

	// *** Basic block 10

.ContainsVLA_label_64:
	ret         

	// *** Basic block 11

.ContainsVLA_label_67:

	// *** Basic block 12

.ContainsVLA_label_68:
	addi        t0, t0, 1
	bge         t0, t2, .ContainsVLA_label_27

	// *** Basic block 13

.ContainsVLA_label_72:
	mv          a0, x0
	ret         
.func_end_ContainsVLA:
	.size ContainsVLA, .func_end_ContainsVLA-ContainsVLA

	.local  FindTopVLAForJump
	.type FindTopVLAForJump, @function

FindTopVLAForJump:

	// *** Basic block 0

	.local ContainsVLA
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
	mv          s3, x0
	beq         s2, s1, .FindTopVLAForJump_label_103

	// *** Basic block 1

.FindTopVLAForJump_label_26:
	ld          t1, 24(s2)
	sub         t2, t1, x0
	snez        t0, t2
	beq         t1, x0, .FindTopVLAForJump_label_37

	// *** Basic block 2

	lw          t2, 0(t1)
	addi        t2, t2, -41
	seqz        t0, t2

	// *** Basic block 3

.FindTopVLAForJump_label_37:
	beqz        t0, .FindTopVLAForJump_label_98

	// *** Basic block 4

	mv          s4, x0
	mv          s5, x0
	li          t0, 1		// 0x1 ASCII \x1
	ld          t2, 56(t1)
	ld          t3, 0(t2)
	ld          s6, 8(t2)
	ld          t1, 56(t1)
	ld          t1, 8(t1)
	slt         t0, x0, t1

	// *** Basic block 5

.FindTopVLAForJump_label_57:
	beqz        t0, .FindTopVLAForJump_label_97

	// *** Basic block 6

.FindTopVLAForJump_label_59:
	slli        t0, s5, 3
	add         t0, t3, t0
	ld          s7, 0(t0)
	beq         s7, s2, .FindTopVLAForJump_label_97

	// *** Basic block 7

.FindTopVLAForJump_label_68:
	lw          t0, 0(s7)
	li          t2, 83		// 0x53 ASCII 'S'
	bne         t0, t2, .FindTopVLAForJump_label_88

	// *** Basic block 8

	mv          s8, s7
	mv          a0, s8
	call        ContainsVLA

	// *** Basic block 9

	mv          s7, a0
	beq         s7, x0, .FindTopVLAForJump_label_87

	// *** Basic block 10

	mv          s3, s7
	li          s4, 1		// 0x1 ASCII \x1

	// *** Basic block 11

.FindTopVLAForJump_label_87:

	// *** Basic block 12

.FindTopVLAForJump_label_88:

	// *** Basic block 13

.FindTopVLAForJump_label_89:
	addi        s5, s5, 1
	not         t0, s4
	beqz        t0, .FindTopVLAForJump_label_95

	// *** Basic block 14

	slt         t0, s5, s6

	// *** Basic block 15

.FindTopVLAForJump_label_95:
	beqz        t0, .FindTopVLAForJump_label_59

	// *** Basic block 16

.FindTopVLAForJump_label_97:

	// *** Basic block 17

.FindTopVLAForJump_label_98:
	mv          s2, t1
	bne         s2, s1, .FindTopVLAForJump_label_26

	// *** Basic block 18

.FindTopVLAForJump_label_103:
	mv          a0, s3

	// *** Basic block 19

.FindTopVLAForJump_label_106:
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
.func_end_FindTopVLAForJump:
	.size FindTopVLAForJump, .func_end_FindTopVLAForJump-FindTopVLAForJump

	.global GenerateVLASize
	.type GenerateVLASize, @function

GenerateVLASize:

	// *** Basic block 0

	.global GenerateExpression
	.global GenerateVLASize
	.global GeneratorGetIntConstant
	.global GeneratorEmit
	.global NewIR2
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
	ld          a1, 32(s2)
	call        GenerateExpression

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 24(s2)
	lw          s6, 16(s4)
	addi        t0, s6, -2
	seqz        s5, t0
	li          t0, 2		// 0x2 ASCII \x2
	beq         s6, t0, .GenerateVLASize_label_48

	// *** Basic block 2

	addi        t0, s6, -1
	seqz        s5, t0

	// *** Basic block 3

.GenerateVLASize_label_48:
	beqz        s5, .GenerateVLASize_label_55

	// *** Basic block 4

	addi        t0, s4, 32
	lb          t0, 16(t0)
	slli        t0, t0, 61
	srai        s5, t0, 63

	// *** Basic block 5

.GenerateVLASize_label_55:

	// *** Basic block 6

.GenerateVLASize_label_57:
	beqz        s5, .GenerateVLASize_label_69

	// *** Basic block 7

	j           .GenerateVLASize_label_60

	// *** Basic block 8

.GenerateVLASize_label_60:
	mv          a1, s4
	mv          a0, s1
	call        GenerateVLASize

	// *** Basic block 9

	mv          s5, a0
	j           .GenerateVLASize_label_80

	// *** Basic block 10

.GenerateVLASize_label_69:
	lw          a2, 20(s4)
	mv          a1, x0
	mv          a0, s1
	call        GeneratorGetIntConstant

	// *** Basic block 11

	mv          s5, a0

	// *** Basic block 12

.GenerateVLASize_label_80:
	mv          a2, s5
	mv          a1, s3
	li          t0, 45		// 0x2d ASCII '-'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 14

	mv          s4, a0
	addi        t0, s2, 32
	sd          s4, 8(t0)
	mv          a0, s4

	// *** Basic block 15

.GenerateVLASize_label_101:
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
.func_end_GenerateVLASize:
	.size GenerateVLASize, .func_end_GenerateVLASize-GenerateVLASize

	.local  GenerateVLADefinition
	.type GenerateVLADefinition, @function

GenerateVLADefinition:

	// *** Basic block 0

	.global GenerateVLASize
	.global GeneratorEmit
	.global NewIR
	.global NewIR1
	.global NewIR2
	.global GeneratorGetIntConstant
	.global compiler
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
	call        GenerateVLASize

	// *** Basic block 1

	mv          s3, a0
	li          s4, 1		// 0x1 ASCII \x1
	mv          a0, s4
	call        NewIR

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 3

	mv          s5, a0
	mv          a1, s5
	li          s6, 126		// 0x7e ASCII '~'
	mv          a0, s6
	call        NewIR1

	// *** Basic block 4

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 5

	sd          s5, 72(s2)
	mv          a0, s4
	call        NewIR

	// *** Basic block 6

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 7

	mv          s4, a0
	la          t0, compiler
	ld          t0, 0(t0)
	ld          t0, 1112(t0)
	lw          a2, 44(t0)
	mv          a1, x0
	mv          a0, s1
	call        GeneratorGetIntConstant

	// *** Basic block 8

	mv          a2, a0
	mv          a1, s3
	li          t0, 118		// 0x76 ASCII 'v'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 10

	mv          s7, a0
	mv          a1, s7
	li          t0, 125		// 0x7d ASCII '}'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 11

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 12

	mv          a1, s4
	mv          a0, s6
	call        NewIR1

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 14

	mv          a0, s4

	// *** Basic block 15

.GenerateVLADefinition_label_120:
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
.func_end_GenerateVLADefinition:
	.size GenerateVLADefinition, .func_end_GenerateVLADefinition-GenerateVLADefinition

	.local  GenerateVariableDeclaration
	.type GenerateVariableDeclaration, @function

GenerateVariableDeclaration:

	// *** Basic block 0

	.local GenerateVLADefinition
	.global GenerateExpression
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
	ld          t0, 56(s1)
	ld          s3, 40(t0)
	lw          s4, 16(s3)
	addi        t1, s4, -2
	seqz        t0, t1
	li          t1, 2		// 0x2 ASCII \x2
	beq         s4, t1, .GenerateVariableDeclaration_label_39

	// *** Basic block 1

	addi        t1, s4, -1
	seqz        t0, t1

	// *** Basic block 2

.GenerateVariableDeclaration_label_39:
	beqz        t0, .GenerateVariableDeclaration_label_46

	// *** Basic block 3

	addi        t1, s3, 32
	lb          t1, 16(t1)
	slli        t1, t1, 61
	srai        t0, t1, 63

	// *** Basic block 4

.GenerateVariableDeclaration_label_46:

	// *** Basic block 5

.GenerateVariableDeclaration_label_48:
	beqz        t0, .GenerateVariableDeclaration_label_66

	// *** Basic block 6

	j           .GenerateVariableDeclaration_label_51

	// *** Basic block 7

.GenerateVariableDeclaration_label_51:
	mv          a2, s1
	mv          a1, s3
	mv          a0, s2
	call        GenerateVLADefinition

	// *** Basic block 8

	mv          s3, a0
	ld          t0, 56(s1)
	sd          s3, 112(t0)

	// *** Basic block 9

.GenerateVariableDeclaration_label_66:
	ld          t0, 64(s1)
	beq         t0, x0, .GenerateVariableDeclaration_label_78

	// *** Basic block 10

	mv          a1, t0
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GenerateExpression

	// *** Basic block 11

.GenerateVariableDeclaration_label_78:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GenerateVariableDeclaration:
	.size GenerateVariableDeclaration, .func_end_GenerateVariableDeclaration-GenerateVariableDeclaration

	.local  GenerateExpressionStatement
	.type GenerateExpressionStatement, @function

GenerateExpressionStatement:

	// *** Basic block 0

	.global GenerateExpression
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	ld          a1, 56(t0)
	j           GenerateExpression
.func_end_GenerateExpressionStatement:
	.size GenerateExpressionStatement, .func_end_GenerateExpressionStatement-GenerateExpressionStatement

	.local  GenerateCompoundStatement
	.type GenerateCompoundStatement, @function

GenerateCompoundStatement:

	// *** Basic block 0

	.global GenerateStatement
	.local ContainsVLA
	.global GeneratorEmit
	.global NewIR1
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
	ld          s2, 56(a1)
	ld          s3, 8(s2)
	mv          s4, x0
	bge         x0, s3, .GenerateCompoundStatement_label_46

	// *** Basic block 1

	ld          s5, 0(s2)
	ld          s2, 0(s2)

	// *** Basic block 2

.GenerateCompoundStatement_label_31:
	slli        t0, s4, 3
	add         t0, s5, t0
	ld          s5, 0(t0)
	mv          a1, s5
	mv          a0, s1
	call        GenerateStatement

	// *** Basic block 3

.GenerateCompoundStatement_label_42:
	addi        s4, s4, 1
	bge         s4, s3, .GenerateCompoundStatement_label_31

	// *** Basic block 4

.GenerateCompoundStatement_label_46:
	mv          s2, x0
	bge         x0, s3, .GenerateCompoundStatement_label_90

	// *** Basic block 5

.GenerateCompoundStatement_label_51:
	slli        t0, s2, 3
	add         t0, s2, t0
	ld          s5, 0(t0)
	lw          t0, 0(s5)
	li          t1, 83		// 0x53 ASCII 'S'
	bne         t0, t1, .GenerateCompoundStatement_label_85

	// *** Basic block 6

	mv          s6, s5
	mv          a0, s6
	call        ContainsVLA

	// *** Basic block 7

	mv          s5, a0
	beq         s5, x0, .GenerateCompoundStatement_label_84

	// *** Basic block 8

	mv          a1, s5
	li          t0, 127		// 0x7f ASCII \x7f
	mv          a0, t0
	call        NewIR1

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 10

	j           .GenerateCompoundStatement_label_90

	// *** Basic block 11

.GenerateCompoundStatement_label_84:

	// *** Basic block 12

.GenerateCompoundStatement_label_85:

	// *** Basic block 13

.GenerateCompoundStatement_label_86:
	addi        s2, s2, 1
	bge         s2, s3, .GenerateCompoundStatement_label_51

	// *** Basic block 14

.GenerateCompoundStatement_label_90:
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
.func_end_GenerateCompoundStatement:
	.size GenerateCompoundStatement, .func_end_GenerateCompoundStatement-GenerateCompoundStatement

	.local  CheckSingleBranch
	.type CheckSingleBranch, @function

CheckSingleBranch:

	// *** Basic block 0

	.global OptLevel0
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
	bne         s1, x0, .CheckSingleBranch_label_23

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.CheckSingleBranch_label_20:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.CheckSingleBranch_label_23:
	call        OptLevel0

	// *** Basic block 4

	beqz        a0, .CheckSingleBranch_label_29

	// *** Basic block 5

	mv          a0, x0
	j           .CheckSingleBranch_label_20

	// *** Basic block 6

.CheckSingleBranch_label_29:
	lw          t0, 0(s1)
	li          t1, 41		// 0x29 ASCII ')'
	bne         t0, t1, .CheckSingleBranch_label_50

	// *** Basic block 7

	mv          s3, s1
	ld          t0, 56(s3)
	ld          t1, 8(t0)
	li          t2, 1		// 0x1 ASCII \x1
	blt         t1, t2, .CheckSingleBranch_label_49

	// *** Basic block 8

	ld          t0, 0(t0)
	ld          s1, 0(t0)

	// *** Basic block 9

.CheckSingleBranch_label_49:

	// *** Basic block 10

.CheckSingleBranch_label_50:
	lw          t0, 0(s1)
	bne         t0, s2, .CheckSingleBranch_label_58

	// *** Basic block 11

	mv          a0, s1
	j           .CheckSingleBranch_label_20

	// *** Basic block 12

.CheckSingleBranch_label_58:
	mv          a0, x0
	j           .CheckSingleBranch_label_20
.func_end_CheckSingleBranch:
	.size CheckSingleBranch, .func_end_CheckSingleBranch-CheckSingleBranch

	.local  GenerateIfStatement
	.type GenerateIfStatement, @function

GenerateIfStatement:

	// *** Basic block 0

	.global OptLevel1
	.global ASTNodeIsIntConstant
	.global GenerateStatement
	.global GenerateExpression
	.global NewIR
	.local CheckSingleBranch
	.global GeneratorEmit
	.global NewIR2
	.global NewIR1
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
	call        OptLevel1

	// *** Basic block 1

	mv          s3, a0
	beqz        a0, .GenerateIfStatement_label_40

	// *** Basic block 2

	ld          a0, 56(s1)
	call        ASTNodeIsIntConstant

	// *** Basic block 3

	mv          s3, a0

	// *** Basic block 4

.GenerateIfStatement_label_40:
	beqz        s3, .GenerateIfStatement_label_72

	// *** Basic block 5

	ld          s4, 56(s1)
	ld          t0, 56(s4)
	beqz        t0, .GenerateIfStatement_label_56

	// *** Basic block 6

	ld          a1, 64(s1)
	mv          a0, s2
	call        GenerateStatement

	// *** Basic block 7

	j           .GenerateIfStatement_label_68

	// *** Basic block 8

.GenerateIfStatement_label_56:
	ld          s4, 72(s1)
	beq         s4, x0, .GenerateIfStatement_label_67

	// *** Basic block 9

	mv          a1, s4
	mv          a0, s2
	call        GenerateStatement

	// *** Basic block 10

.GenerateIfStatement_label_67:

	// *** Basic block 11

.GenerateIfStatement_label_68:

	// *** Basic block 12

.GenerateIfStatement_label_69:
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

	// *** Basic block 13

.GenerateIfStatement_label_72:
	ld          a1, 56(s1)
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 14

	mv          s4, a0
	li          s5, 17		// 0x11 ASCII \x11
	mv          a0, s5
	call        NewIR

	// *** Basic block 15

	mv          s6, a0
	mv          s7, x0
	ld          s8, 72(s1)
	beq         s8, x0, .GenerateIfStatement_label_99

	// *** Basic block 16

	mv          a0, s5
	call        NewIR

	// *** Basic block 17

	mv          s7, a0

	// *** Basic block 18

.GenerateIfStatement_label_99:
	ld          s9, 64(s1)
	li          t0, 23		// 0x17 ASCII \x17
	mv          a1, t0
	mv          a0, s9
	call        CheckSingleBranch
	sd          a0, -24(s0)	// Spilled @107

	// *** Basic block 19

	mv          s10, a0
	li          t0, 31		// 0x1f ASCII \x1f
	mv          a1, t0
	mv          a0, s9
	call        CheckSingleBranch

	// *** Basic block 20

	mv          s11, a0
	li          t0, 36		// 0x24 ASCII '$'
	mv          a1, t0
	mv          a0, s9
	call        CheckSingleBranch

	// *** Basic block 21

	beq         s10, x0, .GenerateIfStatement_label_145

	// *** Basic block 22

	ld          a2, 56(s2)
	mv          a1, s4
	li          t0, 88		// 0x58 ASCII 'X'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 23

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 24

	j           .GenerateIfStatement_label_236

	// *** Basic block 25

.GenerateIfStatement_label_145:
	beq         s11, x0, .GenerateIfStatement_label_164

	// *** Basic block 26

	ld          a2, 64(s2)
	mv          a1, s4
	li          t0, 88		// 0x58 ASCII 'X'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 27

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 28

	j           .GenerateIfStatement_label_235

	// *** Basic block 29

.GenerateIfStatement_label_164:
	beq         a0, x0, .GenerateIfStatement_label_200

	// *** Basic block 30

	mv          s10, a0
	ld          s11, 64(s10)
	ld          t0, 104(s11)
	bne         t0, x0, .GenerateIfStatement_label_184

	// *** Basic block 31

	mv          a0, s5
	call        NewIR

	// *** Basic block 32

	sd          a0, 104(s11)

	// *** Basic block 33

.GenerateIfStatement_label_184:
	ld          a2, 104(s11)
	mv          a1, s4
	li          t0, 88		// 0x58 ASCII 'X'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 34

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 35

	j           .GenerateIfStatement_label_234

	// *** Basic block 36

.GenerateIfStatement_label_200:
	mv          a2, s6
	mv          a1, s4
	li          t0, 89		// 0x59 ASCII 'Y'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 37

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 38

	mv          a1, s9
	mv          a0, s2
	call        GenerateStatement

	// *** Basic block 39

	beq         s7, x0, .GenerateIfStatement_label_233

	// *** Basic block 40

	mv          a1, s7
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 41

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 42

.GenerateIfStatement_label_233:

	// *** Basic block 43

.GenerateIfStatement_label_234:

	// *** Basic block 44

.GenerateIfStatement_label_235:

	// *** Basic block 45

.GenerateIfStatement_label_236:
	beq         s8, x0, .GenerateIfStatement_label_254

	// *** Basic block 46

	mv          a1, s6
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 47

	mv          a1, s8
	mv          a0, s2
	call        GenerateStatement

	// *** Basic block 48

	mv          a1, s7
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 49

	j           .GenerateIfStatement_label_260

	// *** Basic block 50

.GenerateIfStatement_label_254:
	mv          a1, s6
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 51

.GenerateIfStatement_label_260:
	j           .GenerateIfStatement_label_69
.func_end_GenerateIfStatement:
	.size GenerateIfStatement, .func_end_GenerateIfStatement-GenerateIfStatement

	.local  GenerateWhileStatement
	.type GenerateWhileStatement, @function

GenerateWhileStatement:

	// *** Basic block 0

	.global OptLevel1
	.global ASTNodeIsIntConstant
	.global NewIR
	.global GenerateExpression
	.global GeneratorEmit
	.global NewIR2
	.global GenerateStatement
	.global NewIR1
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
	mv          s1, a1
	mv          s2, a0
	mv          s3, x0
	call        OptLevel1

	// *** Basic block 1

	mv          s4, a0
	beqz        a0, .GenerateWhileStatement_label_36

	// *** Basic block 2

	ld          a0, 56(s1)
	call        ASTNodeIsIntConstant

	// *** Basic block 3

	mv          s4, a0

	// *** Basic block 4

.GenerateWhileStatement_label_36:
	beqz        s4, .GenerateWhileStatement_label_48

	// *** Basic block 5

	ld          s3, 56(s1)
	ld          t0, 56(s3)
	bnez        t0, .GenerateWhileStatement_label_47

	// *** Basic block 6

.GenerateWhileStatement_label_44:
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

	// *** Basic block 7

.GenerateWhileStatement_label_47:

	// *** Basic block 8

.GenerateWhileStatement_label_48:
	ld          s5, 56(s2)
	ld          s6, 64(s2)
	li          s7, 17		// 0x11 ASCII \x11
	mv          a0, s7
	call        NewIR

	// *** Basic block 9

	sd          a0, 56(s2)
	mv          a0, s7
	call        NewIR

	// *** Basic block 10

	sd          a0, 64(s2)
	bne         s3, x0, .GenerateWhileStatement_label_93

	// *** Basic block 11

	ld          a1, 56(s1)
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 12

	mv          s7, a0
	ld          a2, 56(s2)
	mv          a1, s7
	li          t0, 89		// 0x59 ASCII 'Y'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 14

.GenerateWhileStatement_label_93:
	ld          s8, 64(s2)
	mv          a1, s8
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 15

	ld          a1, 64(s1)
	mv          a0, s2
	call        GenerateStatement

	// *** Basic block 16

	bne         s3, x0, .GenerateWhileStatement_label_129

	// *** Basic block 17

	ld          a1, 56(s1)
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 18

	mv          s9, a0
	mv          a2, s8
	mv          a1, s9
	li          t0, 88		// 0x58 ASCII 'X'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 20

	j           .GenerateWhileStatement_label_141

	// *** Basic block 21

.GenerateWhileStatement_label_129:
	mv          a1, s8
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 22

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 23

.GenerateWhileStatement_label_141:
	ld          a1, 56(s2)
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 24

	sd          s5, 56(s2)
	sd          s6, 64(s2)
	j           .GenerateWhileStatement_label_44
.func_end_GenerateWhileStatement:
	.size GenerateWhileStatement, .func_end_GenerateWhileStatement-GenerateWhileStatement

	.local  GenerateDoStatement
	.type GenerateDoStatement, @function

GenerateDoStatement:

	// *** Basic block 0

	.global NewIR
	.global GeneratorEmit
	.global GenerateStatement
	.global OptLevel1
	.global ASTNodeIsIntConstant
	.global NewIR1
	.global GenerateExpression
	.global NewIR2
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
	li          s3, 17		// 0x11 ASCII \x11
	mv          a0, s3
	call        NewIR

	// *** Basic block 1

	sd          a0, 56(s1)
	mv          a0, s3
	call        NewIR

	// *** Basic block 2

	sd          a0, 64(s1)
	mv          a0, s3
	call        NewIR

	// *** Basic block 3

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 4

	mv          s3, a0
	ld          a1, 64(s2)
	mv          a0, s1
	call        GenerateStatement

	// *** Basic block 5

	ld          a1, 64(s1)
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 6

	call        OptLevel1

	// *** Basic block 7

	mv          s4, a0
	beqz        a0, .GenerateDoStatement_label_73

	// *** Basic block 8

	ld          a0, 56(s2)
	call        ASTNodeIsIntConstant

	// *** Basic block 9

	mv          s4, a0

	// *** Basic block 10

.GenerateDoStatement_label_73:
	beqz        s4, .GenerateDoStatement_label_97

	// *** Basic block 11

	ld          s5, 56(s2)
	ld          t0, 56(s5)
	beqz        t0, .GenerateDoStatement_label_94

	// *** Basic block 12

	mv          a1, s3
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 14

	j           .GenerateDoStatement_label_95

	// *** Basic block 15

.GenerateDoStatement_label_94:

	// *** Basic block 16

.GenerateDoStatement_label_95:
	j           .GenerateDoStatement_label_120

	// *** Basic block 17

.GenerateDoStatement_label_97:
	ld          a1, 56(s2)
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 18

	mv          s5, a0
	mv          a2, s3
	mv          a1, s5
	li          t0, 88		// 0x58 ASCII 'X'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 20

.GenerateDoStatement_label_120:
	ld          a1, 56(s1)
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
	j           GeneratorEmit
.func_end_GenerateDoStatement:
	.size GenerateDoStatement, .func_end_GenerateDoStatement-GenerateDoStatement

	.local  GenerateDenseSwitch
	.type GenerateDenseSwitch, @function

GenerateDenseSwitch:

	// *** Basic block 0

	.global NewIR
	.global GenerateExpression
	.global GeneratorGetIntConstant
	.global GeneratorEmit
	.global NewIR2
	.global NewIR1
	.global GenerateStatement
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
	mv          s1, a0
	mv          s2, a1
	li          a0, 17		// 0x11 ASCII \x11
	call        NewIR

	// *** Basic block 1

	sd          a0, 56(s1)
	ld          s3, 56(s2)
	mv          a1, s3
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 2

	mv          s4, a0
	ld          s6, 96(s2)
	beq         s6, x0, .GenerateDenseSwitch_label_62

	// *** Basic block 3

	ld          s5, 80(s6)
	j           .GenerateDenseSwitch_label_64

	// *** Basic block 4

.GenerateDenseSwitch_label_62:
	ld          s5, 56(s1)

	// *** Basic block 5

.GenerateDenseSwitch_label_64:
	ld          s3, 16(s3)
	ld          s6, 112(s2)
	mv          a2, s6
	mv          a1, s3
	mv          a0, s1
	call        GeneratorGetIntConstant

	// *** Basic block 6

	mv          s7, a0
	ld          a2, 120(s2)
	mv          a1, s3
	mv          a0, s1
	call        GeneratorGetIntConstant

	// *** Basic block 7

	mv          s3, a0
	sd          s3, -24(s0)	// Spilled @88
	lb          t0, 128(s2)
	not         t0, t0
	beqz        t0, .GenerateDenseSwitch_label_150

	// *** Basic block 8

	mv          a2, s7
	mv          a1, s4
	li          t0, 66		// 0x42 ASCII 'B'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 10

	mv          s8, a0
	mv          a2, s5
	mv          a1, s8
	li          s9, 88		// 0x58 ASCII 'X'
	mv          a0, s9
	call        NewIR2

	// *** Basic block 11

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 12

	mv          a2, s3
	mv          a1, s4
	li          t0, 68		// 0x44 ASCII 'D'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 14

	mv          s10, a0
	mv          a2, s5
	mv          a1, s10
	mv          a0, s9
	call        NewIR2

	// *** Basic block 15

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 16

.GenerateDenseSwitch_label_150:
	mv          a2, s7
	mv          a1, s4
	li          t0, 41		// 0x29 ASCII ')'
	mv          a0, t0
	call        NewIR2
	sd          a0, -24(s0)	// Spilled @158

	// *** Basic block 17

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 18

	mv          s9, a0
	mv          a1, s9
	li          t0, 91		// 0x5b ASCII '['
	mv          a0, t0
	call        NewIR1

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 20

	mv          s11, x0
	addi        t0, s2, 72
	ld          a0, 8(t0)
	bge         x0, a0, .GenerateDenseSwitch_label_245

	// *** Basic block 21

	ld          t0, 72(s2)

	// *** Basic block 22

.GenerateDenseSwitch_label_186:
	slli        t1, s11, 3
	add         t0, t0, t1
	ld          s9, 0(t0)
	ld          s3, 72(s9)
	beq         s6, s3, .GenerateDenseSwitch_label_221

	// *** Basic block 23

.GenerateDenseSwitch_label_197:
	mv          a1, s5
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 24

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 25

	mv          s5, a0
	lw          t0, 88(s5)
	ori         t0, t0, 64
	sw          t0, 88(s5)
	addi        s6, s6, 1

	// *** Basic block 26

.GenerateDenseSwitch_label_216:
	bne         s6, s3, .GenerateDenseSwitch_label_197

	// *** Basic block 27

.GenerateDenseSwitch_label_220:

	// *** Basic block 28

.GenerateDenseSwitch_label_221:
	ld          a1, 80(s9)
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 29

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 30

	mv          s3, a0
	lw          t0, 88(s3)
	ori         t0, t0, 64
	sw          t0, 88(s3)
	addi        s6, s6, 1

	// *** Basic block 31

.GenerateDenseSwitch_label_241:
	addi        s11, s11, 1
	bge         s11, a0, .GenerateDenseSwitch_label_186

	// *** Basic block 32

.GenerateDenseSwitch_label_245:
	ld          a1, 64(s2)
	mv          a0, s1
	call        GenerateStatement

	// *** Basic block 33

	ld          a1, 56(s1)
	mv          a0, s1
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
	j           GeneratorEmit
.func_end_GenerateDenseSwitch:
	.size GenerateDenseSwitch, .func_end_GenerateDenseSwitch-GenerateDenseSwitch

	.local  GenerateBinaryCaseSearch
	.type GenerateBinaryCaseSearch, @function

GenerateBinaryCaseSearch:

	// *** Basic block 0

	.global NewIR
	.global GeneratorEmit
	.global NewIR2
	.global GeneratorGetIntConstant
	.local GenerateBinaryCaseSearch
	.global NewIR1
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
	mv          s1, a5
	mv          s2, a4
	mv          s3, a1
	mv          s4, a0
	mv          s5, a2
	mv          s6, a3
	sub         s7, s1, s2
	li          t0, 15		// 0xf ASCII \xf
	bge         t0, s7, .GenerateBinaryCaseSearch_label_145

	// *** Basic block 1

	ld          s8, 72(s3)
	ld          t0, 56(s3)
	ld          s9, 16(t0)
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewIR

	// *** Basic block 2

	mv          s10, a0
	srli        t0, s7, 1
	add         s7, s2, t0
	ld          t0, 72(s3)
	slli        t1, s7, 3
	add         t0, t0, t1
	ld          s11, 0(t0)
	ld          t0, 56(s3)
	ld          a1, 16(t0)
	ld          a2, 72(s11)
	mv          a0, s4
	call        GeneratorGetIntConstant

	// *** Basic block 3

	mv          a2, a0
	mv          a1, s5
	li          t0, 66		// 0x42 ASCII 'B'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 4

	mv          a1, a0
	mv          a0, s4
	call        GeneratorEmit

	// *** Basic block 5

	mv          s11, a0
	mv          a2, s10
	mv          a1, s11
	li          t0, 88		// 0x58 ASCII 'X'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 6

	mv          a1, a0
	mv          a0, s4
	call        GeneratorEmit

	// *** Basic block 7

	mv          a5, s1
	mv          a4, s7
	mv          a3, s6
	mv          a2, s5
	mv          a1, s3
	mv          a0, s4
	call        GenerateBinaryCaseSearch

	// *** Basic block 8

	mv          a1, s10
	mv          a0, s4
	call        GeneratorEmit

	// *** Basic block 9

	mv          a5, s7
	mv          a4, s2
	mv          a3, s6
	mv          a2, s5
	mv          a1, s3
	mv          a0, s4
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
	j           GenerateBinaryCaseSearch

	// *** Basic block 10

.GenerateBinaryCaseSearch_label_142:
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

	// *** Basic block 11

.GenerateBinaryCaseSearch_label_145:
	mv          s7, s2
	bge         s7, s1, .GenerateBinaryCaseSearch_label_196

	// *** Basic block 12

.GenerateBinaryCaseSearch_label_150:
	slli        t0, s7, 3
	add         t0, s8, t0
	ld          s8, 0(t0)
	ld          a2, 72(s8)
	mv          a1, s9
	mv          a0, s4
	call        GeneratorGetIntConstant

	// *** Basic block 13

	mv          a2, a0
	mv          a1, s5
	li          t0, 64		// 0x40 ASCII '@'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 14

	mv          a1, a0
	mv          a0, s4
	call        GeneratorEmit

	// *** Basic block 15

	mv          s9, a0
	ld          a2, 80(s8)
	mv          a1, s9
	li          t0, 88		// 0x58 ASCII 'X'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 16

	mv          a1, a0
	mv          a0, s4
	call        GeneratorEmit

	// *** Basic block 17

.GenerateBinaryCaseSearch_label_192:
	addi        s7, s7, 1
	bge         s7, s1, .GenerateBinaryCaseSearch_label_150

	// *** Basic block 18

.GenerateBinaryCaseSearch_label_196:
	lb          t0, 128(s3)
	not         t0, t0
	beqz        t0, .GenerateBinaryCaseSearch_label_212

	// *** Basic block 19

	mv          a1, s6
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 20

	mv          a1, a0
	mv          a0, s4
	call        GeneratorEmit

	// *** Basic block 21

.GenerateBinaryCaseSearch_label_212:
	j           .GenerateBinaryCaseSearch_label_142
.func_end_GenerateBinaryCaseSearch:
	.size GenerateBinaryCaseSearch, .func_end_GenerateBinaryCaseSearch-GenerateBinaryCaseSearch

	.local  GenerateSparseSwitch
	.type GenerateSparseSwitch, @function

GenerateSparseSwitch:

	// *** Basic block 0

	.global NewIR
	.global GenerateExpression
	.local GenerateBinaryCaseSearch
	.global GenerateStatement
	.global GeneratorEmit
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
	li          a0, 17		// 0x11 ASCII \x11
	call        NewIR

	// *** Basic block 1

	sd          a0, 56(s1)
	ld          a1, 56(s2)
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 2

	mv          s3, a0
	ld          s5, 96(s2)
	beq         s5, x0, .GenerateSparseSwitch_label_48

	// *** Basic block 3

	ld          s4, 80(s5)
	j           .GenerateSparseSwitch_label_50

	// *** Basic block 4

.GenerateSparseSwitch_label_48:
	ld          s4, 56(s1)

	// *** Basic block 5

.GenerateSparseSwitch_label_50:
	addi        t0, s2, 72
	ld          a5, 8(t0)
	mv          a4, x0
	mv          a3, s4
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        GenerateBinaryCaseSearch

	// *** Basic block 6

	ld          a1, 64(s2)
	mv          a0, s1
	call        GenerateStatement

	// *** Basic block 7

	ld          a1, 56(s1)
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
	j           GeneratorEmit
.func_end_GenerateSparseSwitch:
	.size GenerateSparseSwitch, .func_end_GenerateSparseSwitch-GenerateSparseSwitch

	.local  GenerateConstantSwitch
	.type GenerateConstantSwitch, @function

GenerateConstantSwitch:

	// *** Basic block 0

	.global NewIR
	.global GeneratorEmit
	.global NewIR1
	.global GenerateStatement
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
	ld          t0, 56(s1)
	ld          s3, 56(t0)
	mv          s4, x0
	mv          s5, x0
	addi        t0, s1, 72
	ld          s6, 8(t0)
	bge         x0, s6, .GenerateConstantSwitch_label_58

	// *** Basic block 1

	ld          t0, 72(s1)

	// *** Basic block 2

.GenerateConstantSwitch_label_40:
	slli        t1, s5, 3
	add         t0, t0, t1
	ld          s7, 0(t0)
	ld          t0, 72(s7)
	bne         t0, s3, .GenerateConstantSwitch_label_53

	// *** Basic block 3

	mv          s4, s7
	j           .GenerateConstantSwitch_label_58

	// *** Basic block 4

.GenerateConstantSwitch_label_53:

	// *** Basic block 5

.GenerateConstantSwitch_label_54:
	addi        s5, s5, 1
	bge         s5, s6, .GenerateConstantSwitch_label_40

	// *** Basic block 6

.GenerateConstantSwitch_label_58:
	beq         s4, x0, .GenerateConstantSwitch_label_82

	// *** Basic block 7

	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewIR

	// *** Basic block 8

	mv          s3, a0
	sd          s3, 80(s4)
	mv          a1, s3
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 10

	j           .GenerateConstantSwitch_label_113

	// *** Basic block 11

.GenerateConstantSwitch_label_82:
	ld          t0, 96(s1)
	bne         t0, x0, .GenerateConstantSwitch_label_91

	// *** Basic block 12

.GenerateConstantSwitch_label_88:
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

	// *** Basic block 13

.GenerateConstantSwitch_label_91:
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewIR

	// *** Basic block 14

	mv          s6, a0
	ld          t0, 96(s1)
	sd          s6, 80(t0)
	mv          a1, s6
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 15

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 16

.GenerateConstantSwitch_label_113:
	ld          s7, 56(s2)
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewIR

	// *** Basic block 17

	sd          a0, 56(s2)
	ld          a1, 64(s1)
	mv          a0, s2
	call        GenerateStatement

	// *** Basic block 18

	ld          a1, 56(s2)
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 19

	sd          s7, 56(s2)
	j           .GenerateConstantSwitch_label_88
.func_end_GenerateConstantSwitch:
	.size GenerateConstantSwitch, .func_end_GenerateConstantSwitch-GenerateConstantSwitch

	.local  GenerateSwitchStatement
	.type GenerateSwitchStatement, @function

GenerateSwitchStatement:

	// *** Basic block 0

	.global OptLevel1
	.global ASTNodeIsIntConstant
	.local GenerateConstantSwitch
	.global NewIR
	.local GenerateDenseSwitch
	.local GenerateSparseSwitch
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
	call        OptLevel1

	// *** Basic block 1

	mv          s3, a0
	beqz        a0, .GenerateSwitchStatement_label_35

	// *** Basic block 2

	ld          a0, 56(s1)
	call        ASTNodeIsIntConstant

	// *** Basic block 3

	mv          s3, a0

	// *** Basic block 4

.GenerateSwitchStatement_label_35:
	beqz        s3, .GenerateSwitchStatement_label_46

	// *** Basic block 5

	mv          a1, s1
	mv          a0, s2
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
	j           GenerateConstantSwitch

	// *** Basic block 6

.GenerateSwitchStatement_label_43:
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

	// *** Basic block 7

.GenerateSwitchStatement_label_46:
	mv          s4, x0
	addi        t0, s1, 72
	ld          s5, 8(t0)
	bge         x0, s5, .GenerateSwitchStatement_label_73

	// *** Basic block 8

	ld          s6, 72(s1)

	// *** Basic block 9

.GenerateSwitchStatement_label_55:
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewIR

	// *** Basic block 10

	mv          s7, a0
	slli        t0, s4, 3
	add         t0, s6, t0
	ld          s6, 0(t0)
	sd          s7, 80(s6)

	// *** Basic block 11

.GenerateSwitchStatement_label_69:
	addi        s4, s4, 1
	bge         s4, s5, .GenerateSwitchStatement_label_55

	// *** Basic block 12

.GenerateSwitchStatement_label_73:
	ld          s5, 96(s1)
	beq         s5, x0, .GenerateSwitchStatement_label_88

	// *** Basic block 13

	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewIR

	// *** Basic block 14

	mv          s6, a0
	sd          s6, 80(s5)

	// *** Basic block 15

.GenerateSwitchStatement_label_88:
	flw         ft0, 104(s1)
	fcvt.d.s    ft0, ft0
	li          t0, 4602678819172646912		// 0x3fe0000000000000
	fmv.d.x     ft1, t0
	flt.d       t0, ft1, ft0
	beqz        t0, .GenerateSwitchStatement_label_103

	// *** Basic block 16

	mv          a1, s1
	mv          a0, s2
	call        GenerateDenseSwitch

	// *** Basic block 17

	j           .GenerateSwitchStatement_label_109

	// *** Basic block 18

.GenerateSwitchStatement_label_103:
	mv          a1, s1
	mv          a0, s2
	call        GenerateSparseSwitch

	// *** Basic block 19

.GenerateSwitchStatement_label_109:
	j           .GenerateSwitchStatement_label_43
.func_end_GenerateSwitchStatement:
	.size GenerateSwitchStatement, .func_end_GenerateSwitchStatement-GenerateSwitchStatement

	.local  GenerateForStatement
	.type GenerateForStatement, @function

GenerateForStatement:

	// *** Basic block 0

	.global NewIR
	.global GenerateStatement
	.global GenerateExpression
	.global OptLevel1
	.global ASTNodeIsIntConstant
	.global GeneratorEmit
	.global NewIR2
	.global NewIR1
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
	mv          s1, a0
	mv          s2, a1
	ld          s3, 56(s1)
	ld          s4, 64(s1)
	li          s5, 17		// 0x11 ASCII \x11
	mv          a0, s5
	call        NewIR

	// *** Basic block 1

	sd          a0, 56(s1)
	mv          a0, s5
	call        NewIR

	// *** Basic block 2

	sd          a0, 64(s1)
	mv          a0, s5
	call        NewIR

	// *** Basic block 3

	mv          s5, a0
	ld          s6, 56(s2)
	beq         s6, x0, .GenerateForStatement_label_76

	// *** Basic block 4

	lw          t0, 0(s6)
	li          t1, 83		// 0x53 ASCII 'S'
	bne         t0, t1, .GenerateForStatement_label_69

	// *** Basic block 5

	mv          a1, s6
	mv          a0, s1
	call        GenerateStatement

	// *** Basic block 6

	j           .GenerateForStatement_label_75

	// *** Basic block 7

.GenerateForStatement_label_69:
	mv          a1, s6
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 8

.GenerateForStatement_label_75:

	// *** Basic block 9

.GenerateForStatement_label_76:
	mv          s6, x0
	ld          s7, 64(s2)
	sub         t0, s7, x0
	snez        t1, t0
	beq         s7, x0, .GenerateForStatement_label_109

	// *** Basic block 10

	call        OptLevel1

	// *** Basic block 11

	mv          s8, a0
	beqz        a0, .GenerateForStatement_label_92

	// *** Basic block 12

	mv          a0, s7
	call        ASTNodeIsIntConstant

	// *** Basic block 13

	mv          s8, a0

	// *** Basic block 14

.GenerateForStatement_label_92:
	beqz        s8, .GenerateForStatement_label_108

	// *** Basic block 15

	ld          t0, 56(s7)
	bnez        t0, .GenerateForStatement_label_106

	// *** Basic block 16

	sd          s3, 56(s1)
	sd          s4, 64(s1)

	// *** Basic block 17

.GenerateForStatement_label_103:
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

	// *** Basic block 18

.GenerateForStatement_label_106:
	li          s6, 1		// 0x1 ASCII \x1

	// *** Basic block 19

.GenerateForStatement_label_108:

	// *** Basic block 20

.GenerateForStatement_label_109:
	beq         s7, x0, .GenerateForStatement_label_113

	// *** Basic block 21

	not         t1, s6

	// *** Basic block 22

.GenerateForStatement_label_113:
	beqz        t1, .GenerateForStatement_label_137

	// *** Basic block 23

	mv          a1, s7
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 24

	mv          s8, a0
	ld          a2, 56(s1)
	mv          a1, s8
	li          t0, 89		// 0x59 ASCII 'Y'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 25

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 26

.GenerateForStatement_label_137:
	mv          a1, s5
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 27

	ld          a1, 80(s2)
	mv          a0, s1
	call        GenerateStatement

	// *** Basic block 28

	ld          a1, 64(s1)
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 29

	ld          s9, 72(s2)
	beq         s9, x0, .GenerateForStatement_label_164

	// *** Basic block 30

	mv          a1, s9
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 31

.GenerateForStatement_label_164:
	mv          t0, t1
	beq         s7, x0, .GenerateForStatement_label_169

	// *** Basic block 32

	not         t0, s6

	// *** Basic block 33

.GenerateForStatement_label_169:
	beqz        t0, .GenerateForStatement_label_192

	// *** Basic block 34

	mv          a1, s7
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 35

	mv          s2, a0
	mv          a2, s5
	mv          a1, s2
	li          t0, 89		// 0x59 ASCII 'Y'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 36

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 37

	j           .GenerateForStatement_label_204

	// *** Basic block 38

.GenerateForStatement_label_192:
	mv          a1, s5
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 39

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 40

.GenerateForStatement_label_204:
	ld          a1, 56(s1)
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 41

	sd          s3, 56(s1)
	sd          s4, 64(s1)
	j           .GenerateForStatement_label_103
.func_end_GenerateForStatement:
	.size GenerateForStatement, .func_end_GenerateForStatement-GenerateForStatement

	.local  GenerateReturnStatement
	.type GenerateReturnStatement, @function

GenerateReturnStatement:

	// *** Basic block 0

	.global GenerateStatement
	.global GenerateExpression
	.global TypeIsStructOrUnion
	.global GeneratorEmit
	.global NewIR1
	.global CheckForVarUse
	.global NewIR3
	.global GeneratorGetIntConstant
	.global CheckForVarDef
	.global TypeIsIntegral
	.global TypeIsFloat
	.global TypeIsDouble
	.global GeneratorGetReturnLabel
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
	ld          s4, 56(s1)
	beq         s4, x0, .GenerateReturnStatement_label_178

	// *** Basic block 1

	lw          t0, 0(s4)
	li          t1, 74		// 0x4a ASCII 'J'
	bne         t0, t1, .GenerateReturnStatement_label_58

	// *** Basic block 2

	mv          a1, s4
	mv          a0, s2
	call        GenerateStatement

	// *** Basic block 3

	j           .GenerateReturnStatement_label_177

	// *** Basic block 4

.GenerateReturnStatement_label_58:
	mv          a1, s4
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 5

	mv          s5, a0
	ld          s6, 16(s4)
	mv          a0, s6
	call        TypeIsStructOrUnion

	// *** Basic block 6

	beqz        a0, .GenerateReturnStatement_label_139

	// *** Basic block 7

	lw          t0, 8(s4)
	andi        t1, t0, 32
	beqz        t1, .GenerateReturnStatement_label_78

	// *** Basic block 8

	j           .GenerateReturnStatement_label_137

	// *** Basic block 9

.GenerateReturnStatement_label_78:
	andi        t0, t0, 64
	beqz        t0, .GenerateReturnStatement_label_84

	// *** Basic block 10

	mv          s3, s5
	j           .GenerateReturnStatement_label_136

	// *** Basic block 11

.GenerateReturnStatement_label_84:
	mv          a1, s5
	li          t0, 103		// 0x67 ASCII 'g'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 12

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 13

	mv          s5, a0
	mv          a1, s4
	mv          a0, s5
	call        CheckForVarUse

	// *** Basic block 14

	ld          s4, 72(s2)
	lw          a2, 20(s6)
	mv          a1, x0
	mv          a0, s2
	call        GeneratorGetIntConstant

	// *** Basic block 15

	mv          a3, a0
	mv          a2, s5
	mv          a1, s4
	li          t0, 120		// 0x78 ASCII 'x'
	mv          a0, t0
	call        NewIR3

	// *** Basic block 16

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 17

	mv          s4, a0
	mv          a1, s1
	mv          a0, s4
	call        CheckForVarDef

	// *** Basic block 18

.GenerateReturnStatement_label_136:

	// *** Basic block 19

.GenerateReturnStatement_label_137:
	j           .GenerateReturnStatement_label_176

	// *** Basic block 20

.GenerateReturnStatement_label_139:
	mv          a0, s6
	call        TypeIsIntegral

	// *** Basic block 21

	beqz        a0, .GenerateReturnStatement_label_147

	// *** Basic block 22

	li          s7, 106		// 0x6a ASCII 'j'
	j           .GenerateReturnStatement_label_165

	// *** Basic block 23

.GenerateReturnStatement_label_147:
	mv          a0, s6
	call        TypeIsFloat

	// *** Basic block 24

	beqz        a0, .GenerateReturnStatement_label_154

	// *** Basic block 25

	li          s7, 107		// 0x6b ASCII 'k'
	j           .GenerateReturnStatement_label_164

	// *** Basic block 26

.GenerateReturnStatement_label_154:
	mv          a0, s6
	call        TypeIsDouble

	// *** Basic block 27

	beqz        a0, .GenerateReturnStatement_label_161

	// *** Basic block 28

	li          s7, 108		// 0x6c ASCII 'l'
	j           .GenerateReturnStatement_label_163

	// *** Basic block 29

.GenerateReturnStatement_label_161:
	li          s7, 109		// 0x6d ASCII 'm'

	// *** Basic block 30

.GenerateReturnStatement_label_163:

	// *** Basic block 31

.GenerateReturnStatement_label_164:

	// *** Basic block 32

.GenerateReturnStatement_label_165:
	mv          a1, s5
	mv          a0, s7
	call        NewIR1

	// *** Basic block 33

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 34

.GenerateReturnStatement_label_176:

	// *** Basic block 35

.GenerateReturnStatement_label_177:

	// *** Basic block 36

.GenerateReturnStatement_label_178:
	mv          a0, s2
	call        GeneratorGetReturnLabel

	// *** Basic block 37

	mv          s6, a0
	mv          a1, s6
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 38

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 39

	mv          s7, a0
	lw          t0, 88(s7)
	ori         t0, t0, 8
	sw          t0, 88(s7)
	beq         s3, x0, .GenerateReturnStatement_label_208

	// *** Basic block 40

	lw          t0, 88(s7)
	ori         t0, t0, 32
	sw          t0, 88(s7)

	// *** Basic block 41

.GenerateReturnStatement_label_208:
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
.func_end_GenerateReturnStatement:
	.size GenerateReturnStatement, .func_end_GenerateReturnStatement-GenerateReturnStatement

	.local  GenerateCaseLabel
	.type GenerateCaseLabel, @function

GenerateCaseLabel:

	// *** Basic block 0

	.global GeneratorEmit
	.global GenerateStatement
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
	ld          s3, 80(s1)
	bne         s3, x0, .GenerateCaseLabel_label_22

	// *** Basic block 1

.GenerateCaseLabel_label_19:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.GenerateCaseLabel_label_22:
	mv          a1, s3
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 3

	ld          a1, 64(s1)
	mv          a0, s2
	call        GenerateStatement

	// *** Basic block 4

	j           .GenerateCaseLabel_label_19
.func_end_GenerateCaseLabel:
	.size GenerateCaseLabel, .func_end_GenerateCaseLabel-GenerateCaseLabel

	.local  GenerateGotoStatement
	.type GenerateGotoStatement, @function

GenerateGotoStatement:

	// *** Basic block 0

	.global NewIR
	.global printf
	.global abort
	.local FindTopVLAForJump
	.global GeneratorEmit
	.global NewIR1
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
	ld          s3, 64(s1)
	ld          t0, 104(s3)
	bne         t0, x0, .GenerateGotoStatement_label_40

	// *** Basic block 1

	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewIR

	// *** Basic block 2

	sd          a0, 104(s3)

	// *** Basic block 3

.GenerateGotoStatement_label_40:
	ld          t0, 72(s1)
	beq         t0, x0, .GenerateGotoStatement_label_47

	// *** Basic block 4

	j           .GenerateGotoStatement_label_64

	// *** Basic block 5

.GenerateGotoStatement_label_47:
	lla         a0, .str.1
	lla         a1, .str.2
	lla         a3, .str.3
	li          t0, 764		// 0x2fc
	mv          a2, t0
	call        printf

	// *** Basic block 6

	call        abort

	// *** Basic block 7

.GenerateGotoStatement_label_64:
	ld          a1, 72(s1)
	mv          a0, s1
	call        FindTopVLAForJump

	// *** Basic block 8

	mv          s4, a0
	beq         s4, x0, .GenerateGotoStatement_label_87

	// *** Basic block 9

	mv          a1, s4
	li          t0, 127		// 0x7f ASCII \x7f
	mv          a0, t0
	call        NewIR1

	// *** Basic block 10

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 11

.GenerateGotoStatement_label_87:
	ld          a1, 104(s3)
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 12

	mv          a1, a0
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GeneratorEmit
.func_end_GenerateGotoStatement:
	.size GenerateGotoStatement, .func_end_GenerateGotoStatement-GenerateGotoStatement

	.local  GenerateLabel
	.type GenerateLabel, @function

GenerateLabel:

	// *** Basic block 0

	.global NewIRNamedLabel
	.global NewIR
	.global GeneratorEmit
	.global GenerateStatement
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
	ld          t0, 104(s1)
	bne         t0, x0, .GenerateLabel_label_44

	// *** Basic block 1

	lb          t0, 112(s1)
	beqz        t0, .GenerateLabel_label_36

	// *** Basic block 2

	addi        t0, s1, 64
	ld          a0, 16(t0)
	call        NewIRNamedLabel

	// *** Basic block 3

	sd          a0, 104(s1)
	j           .GenerateLabel_label_43

	// *** Basic block 4

.GenerateLabel_label_36:
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewIR

	// *** Basic block 5

	sd          a0, 104(s1)

	// *** Basic block 6

.GenerateLabel_label_43:

	// *** Basic block 7

.GenerateLabel_label_44:
	ld          a1, 104(s1)
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 8

	ld          a1, 56(s1)
	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GenerateStatement
.func_end_GenerateLabel:
	.size GenerateLabel, .func_end_GenerateLabel-GenerateLabel

	.local  GenerateBreak
	.type GenerateBreak, @function

GenerateBreak:

	// *** Basic block 0

	.local FindEnclosingLoopOrSwitch
	.global printf
	.global abort
	.local FindTopVLAForJump
	.global GeneratorEmit
	.global NewIR1
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
	call        FindEnclosingLoopOrSwitch

	// *** Basic block 1

	mv          s3, a0
	beq         s3, x0, .GenerateBreak_label_32

	// *** Basic block 2

	j           .GenerateBreak_label_49

	// *** Basic block 3

.GenerateBreak_label_32:
	lla         a0, .str.4
	lla         a1, .str.5
	lla         a3, .str.6
	li          t0, 792		// 0x318
	mv          a2, t0
	call        printf

	// *** Basic block 4

	call        abort

	// *** Basic block 5

.GenerateBreak_label_49:
	mv          a1, s3
	mv          a0, s1
	call        FindTopVLAForJump

	// *** Basic block 6

	mv          s4, a0
	beq         s4, x0, .GenerateBreak_label_71

	// *** Basic block 7

	mv          a1, s4
	li          t0, 127		// 0x7f ASCII \x7f
	mv          a0, t0
	call        NewIR1

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 9

.GenerateBreak_label_71:
	ld          a1, 56(s2)
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 10

	mv          a1, a0
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GeneratorEmit
.func_end_GenerateBreak:
	.size GenerateBreak, .func_end_GenerateBreak-GenerateBreak

	.local  GenerateContinue
	.type GenerateContinue, @function

GenerateContinue:

	// *** Basic block 0

	.local FindEnclosingLoop
	.global printf
	.global abort
	.local FindTopVLAForJump
	.global GeneratorEmit
	.global NewIR1
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
	call        FindEnclosingLoop

	// *** Basic block 1

	mv          s3, a0
	beq         s3, x0, .GenerateContinue_label_32

	// *** Basic block 2

	j           .GenerateContinue_label_49

	// *** Basic block 3

.GenerateContinue_label_32:
	lla         a0, .str.7
	lla         a1, .str.8
	lla         a3, .str.9
	li          t0, 802		// 0x322
	mv          a2, t0
	call        printf

	// *** Basic block 4

	call        abort

	// *** Basic block 5

.GenerateContinue_label_49:
	mv          a1, s3
	mv          a0, s1
	call        FindTopVLAForJump

	// *** Basic block 6

	mv          s4, a0
	beq         s4, x0, .GenerateContinue_label_71

	// *** Basic block 7

	mv          a1, s4
	li          t0, 127		// 0x7f ASCII \x7f
	mv          a0, t0
	call        NewIR1

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 9

.GenerateContinue_label_71:
	ld          a1, 64(s2)
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 10

	mv          a1, a0
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GeneratorEmit
.func_end_GenerateContinue:
	.size GenerateContinue, .func_end_GenerateContinue-GenerateContinue

	.local  GenerateAsm
	.type GenerateAsm, @function

GenerateAsm:

	// *** Basic block 0

	.global CompilerAddStringLiteral
	.global GeneratorEmit
	.global NewIR1
	.global GeneratorGetIntConstant
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
	ld          a0, 56(a1)
	call        CompilerAddStringLiteral

	// *** Basic block 1

	mv          s2, a0
	mv          a2, s2
	mv          a1, x0
	mv          a0, s1
	call        GeneratorGetIntConstant

	// *** Basic block 2

	mv          a1, a0
	li          t0, 123		// 0x7b ASCII '{'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 3

	mv          a1, a0
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GeneratorEmit
.func_end_GenerateAsm:
	.size GenerateAsm, .func_end_GenerateAsm-GenerateAsm

	.global GenerateStatement
	.type GenerateStatement, @function

GenerateStatement:

	// *** Basic block 0

	.global IRSetLocation
	.global compiler
	.global GeneratorEmit
	.global NewIRLocation
	.local GenerateDeclarationList
	.local GenerateVariableDeclaration
	.local GenerateExpressionStatement
	.local GenerateCompoundStatement
	.local GenerateIfStatement
	.local GenerateWhileStatement
	.local GenerateDoStatement
	.local GenerateSwitchStatement
	.local GenerateForStatement
	.local GenerateReturnStatement
	.local GenerateCaseLabel
	.local GenerateGotoStatement
	.local GenerateLabel
	.local GenerateBreak
	.local GenerateContinue
	.local GenerateAsm
	.global printf
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
	mv          s1, a1
	mv          s2, a0
	bne         s1, x0, .GenerateStatement_label_62

	// *** Basic block 1

.GenerateStatement_label_59:
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

.GenerateStatement_label_62:
	ld          s3, 40(s1)
	mv          a0, s3
	call        IRSetLocation

	// *** Basic block 3

	la          t0, compiler
	ld          t0, 0(t0)
	lb          t0, 1228(t0)
	beqz        t0, .GenerateStatement_label_129

	// *** Basic block 4

	li          s4, 1		// 0x1 ASCII \x1
	lw          s5, 0(s1)
	li          t0, 41		// 0x29 ASCII ')'
	beq         s5, t0, .GenerateStatement_label_112

	// *** Basic block 5

	li          t0, 81		// 0x51 ASCII 'Q'
	beq         s5, t0, .GenerateStatement_label_115

	// *** Basic block 6

	li          t0, 82		// 0x52 ASCII 'R'
	beq         s5, t0, .GenerateStatement_label_101

	// *** Basic block 7

	li          t0, 83		// 0x53 ASCII 'S'
	beq         s5, t0, .GenerateStatement_label_98

	// *** Basic block 8

.GenerateStatement_label_96:
	j           .GenerateStatement_label_118

	// *** Basic block 9

.GenerateStatement_label_98:
	mv          s4, x0
	j           .GenerateStatement_label_118

	// *** Basic block 10

.GenerateStatement_label_101:
	mv          s5, s1
	ld          t0, 64(s5)
	bne         t0, x0, .GenerateStatement_label_110

	// *** Basic block 11

	mv          s4, x0

	// *** Basic block 12

.GenerateStatement_label_110:
	j           .GenerateStatement_label_118

	// *** Basic block 13

.GenerateStatement_label_112:
	mv          s4, x0
	j           .GenerateStatement_label_118

	// *** Basic block 14

.GenerateStatement_label_115:
	mv          s4, x0
	j           .GenerateStatement_label_118

	// *** Basic block 15

.GenerateStatement_label_118:
	beqz        s4, .GenerateStatement_label_128

	// *** Basic block 16

	mv          a0, s3
	call        NewIRLocation

	// *** Basic block 17

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 18

.GenerateStatement_label_128:

	// *** Basic block 19

.GenerateStatement_label_129:
	lw          s3, 0(s1)
	li          s6, 62		// 0x3e ASCII '>'
	blt         s3, s6, .GenerateStatement_label_172

	// *** Basic block 20

	beq         s3, s6, .GenerateStatement_label_276

	// *** Basic block 21

	li          t0, 70		// 0x46 ASCII 'F'
	beq         s3, t0, .GenerateStatement_label_262

	// *** Basic block 22

	li          t0, 72		// 0x48 ASCII 'H'
	beq         s3, t0, .GenerateStatement_label_248

	// *** Basic block 23

	li          t0, 74		// 0x4a ASCII 'J'
	beq         s3, t0, .GenerateStatement_label_318

	// *** Basic block 24

	li          t0, 81		// 0x51 ASCII 'Q'
	beq         s3, t0, .GenerateStatement_label_297

	// *** Basic block 25

	li          t0, 82		// 0x52 ASCII 'R'
	beq         s3, t0, .GenerateStatement_label_220

	// *** Basic block 26

	li          t0, 83		// 0x53 ASCII 'S'
	beq         s3, t0, .GenerateStatement_label_213

	// *** Basic block 27

	li          t0, 85		// 0x55 ASCII 'U'
	beq         s3, t0, .GenerateStatement_label_227

	// *** Basic block 28

	j           .GenerateStatement_label_325

	// *** Basic block 29

.GenerateStatement_label_172:
	li          t0, 23		// 0x17 ASCII \x17
	beq         s3, t0, .GenerateStatement_label_304

	// *** Basic block 30

	li          t0, 26		// 0x1a ASCII \x1a
	beq         s3, t0, .GenerateStatement_label_283

	// *** Basic block 31

	li          t0, 31		// 0x1f ASCII \x1f
	beq         s3, t0, .GenerateStatement_label_311

	// *** Basic block 32

	li          t0, 32		// 0x20 ASCII ' '
	beq         s3, t0, .GenerateStatement_label_255

	// *** Basic block 33

	li          t0, 35		// 0x23 ASCII '#'
	beq         s3, t0, .GenerateStatement_label_269

	// *** Basic block 34

	li          t0, 36		// 0x24 ASCII '$'
	beq         s3, t0, .GenerateStatement_label_290

	// *** Basic block 35

	li          t0, 39		// 0x27 ASCII '''
	beq         s3, t0, .GenerateStatement_label_241

	// *** Basic block 36

	li          t0, 41		// 0x29 ASCII ')'
	beq         s3, t0, .GenerateStatement_label_234

	// *** Basic block 37

	j           .GenerateStatement_label_325

	// *** Basic block 38

.GenerateStatement_label_213:
	mv          a1, s1
	mv          a0, s2
	call        GenerateDeclarationList

	// *** Basic block 39

	j           .GenerateStatement_label_342

	// *** Basic block 40

.GenerateStatement_label_220:
	mv          a1, s1
	mv          a0, s2
	call        GenerateVariableDeclaration

	// *** Basic block 41

	j           .GenerateStatement_label_342

	// *** Basic block 42

.GenerateStatement_label_227:
	mv          a1, s1
	mv          a0, s2
	call        GenerateExpressionStatement

	// *** Basic block 43

	j           .GenerateStatement_label_342

	// *** Basic block 44

.GenerateStatement_label_234:
	mv          a1, s1
	mv          a0, s2
	call        GenerateCompoundStatement

	// *** Basic block 45

	j           .GenerateStatement_label_342

	// *** Basic block 46

.GenerateStatement_label_241:
	mv          a1, s1
	mv          a0, s2
	call        GenerateIfStatement

	// *** Basic block 47

	j           .GenerateStatement_label_342

	// *** Basic block 48

.GenerateStatement_label_248:
	mv          a1, s1
	mv          a0, s2
	call        GenerateWhileStatement

	// *** Basic block 49

	j           .GenerateStatement_label_342

	// *** Basic block 50

.GenerateStatement_label_255:
	mv          a1, s1
	mv          a0, s2
	call        GenerateDoStatement

	// *** Basic block 51

	j           .GenerateStatement_label_342

	// *** Basic block 52

.GenerateStatement_label_262:
	mv          a1, s1
	mv          a0, s2
	call        GenerateSwitchStatement

	// *** Basic block 53

	j           .GenerateStatement_label_342

	// *** Basic block 54

.GenerateStatement_label_269:
	mv          a1, s1
	mv          a0, s2
	call        GenerateForStatement

	// *** Basic block 55

	j           .GenerateStatement_label_342

	// *** Basic block 56

.GenerateStatement_label_276:
	mv          a1, s1
	mv          a0, s2
	call        GenerateReturnStatement

	// *** Basic block 57

	j           .GenerateStatement_label_342

	// *** Basic block 58

.GenerateStatement_label_283:
	mv          a1, s1
	mv          a0, s2
	call        GenerateCaseLabel

	// *** Basic block 59

	j           .GenerateStatement_label_342

	// *** Basic block 60

.GenerateStatement_label_290:
	mv          a1, s1
	mv          a0, s2
	call        GenerateGotoStatement

	// *** Basic block 61

	j           .GenerateStatement_label_342

	// *** Basic block 62

.GenerateStatement_label_297:
	mv          a1, s1
	mv          a0, s2
	call        GenerateLabel

	// *** Basic block 63

	j           .GenerateStatement_label_342

	// *** Basic block 64

.GenerateStatement_label_304:
	mv          a1, s1
	mv          a0, s2
	call        GenerateBreak

	// *** Basic block 65

	j           .GenerateStatement_label_342

	// *** Basic block 66

.GenerateStatement_label_311:
	mv          a1, s1
	mv          a0, s2
	call        GenerateContinue

	// *** Basic block 67

	j           .GenerateStatement_label_342

	// *** Basic block 68

.GenerateStatement_label_318:
	mv          a1, s1
	mv          a0, s2
	call        GenerateAsm

	// *** Basic block 69

	j           .GenerateStatement_label_342

	// *** Basic block 70

.GenerateStatement_label_325:
	lla         a0, .str.10
	lla         a1, .str.11
	lla         a3, .str.12
	li          t0, 908		// 0x38c
	mv          a2, t0
	call        printf

	// *** Basic block 71

	call        abort

	// *** Basic block 72

.GenerateStatement_label_342:
	j           .GenerateStatement_label_59
.func_end_GenerateStatement:
	.size GenerateStatement, .func_end_GenerateStatement-GenerateStatement

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.1, @object
	.size .str.1, 30

.str.2:
	.asciz "(null)"
	.type .str.2, @object
	.size .str.2, 1

.str.3:
	.asciz "node->lca != NULL"
	.type .str.3, @object
	.size .str.3, 18

.str.4:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.4, @object
	.size .str.4, 30

.str.5:
	.asciz "(null)"
	.type .str.5, @object
	.size .str.5, 1

.str.6:
	.asciz "loop_or_switch != NULL"
	.type .str.6, @object
	.size .str.6, 23

.str.7:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.7, @object
	.size .str.7, 30

.str.8:
	.asciz "(null)"
	.type .str.8, @object
	.size .str.8, 1

.str.9:
	.asciz "loop != NULL"
	.type .str.9, @object
	.size .str.9, 13

.str.10:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.10, @object
	.size .str.10, 30

.str.11:
	.asciz "(null)"
	.type .str.11, @object
	.size .str.11, 1

.str.12:
	.asciz "false"
	.type .str.12, @object
	.size .str.12, 6

