	.file   "expr_codegen.c"
	.text
	.option pic
.PCbegin:
	.local  FindIROpcode
	.type FindIROpcode, @function

FindIROpcode:

	// *** Basic block 0

	.local expr_operators
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
	mv          s2, a0
	mv          s3, x0
	la          t0, expr_operators
	lw          t0, 0(t0)
	beqz        t0, .FindIROpcode_label_66

	// *** Basic block 1

	ld          s4, 16(s2)

	// *** Basic block 2

.FindIROpcode_label_31:
	slli        t0, s3, 3
	slli        t1, s3, 4
	add         t0, t0, t1
	la          t1, expr_operators
	add         s2, t1, t0
	lw          t0, 0(s2)
	bne         t0, s1, .FindIROpcode_label_55

	// *** Basic block 3

	ld          t0, 8(s2)
	mv          a0, s4
	jalr         x1, t0, 0

	// *** Basic block 4

	beqz        a0, .FindIROpcode_label_54

	// *** Basic block 5

	lw          a0, 16(s2)

	// *** Basic block 6

.FindIROpcode_label_51:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.FindIROpcode_label_54:

	// *** Basic block 8

.FindIROpcode_label_55:

	// *** Basic block 9

.FindIROpcode_label_56:
	addi        s3, s3, 1
	slli        t0, s3, 3
	slli        t1, s3, 4
	add         t0, t0, t1
	la          t1, expr_operators
	add         t0, t1, t0
	lw          t0, 0(t0)
	beqz        t0, .FindIROpcode_label_31

	// *** Basic block 10

.FindIROpcode_label_66:
	lla         a0, .str.1
	lla         a1, .str.2
	lla         a3, .str.3
	li          t0, 96		// 0x60 ASCII '`'
	mv          a2, t0
	call        printf

	// *** Basic block 11

	call        abort

	// *** Basic block 12

	mv          a0, x0
	j           .FindIROpcode_label_51
.func_end_FindIROpcode:
	.size FindIROpcode, .func_end_FindIROpcode-FindIROpcode

	.local  GetLoadOpcode
	.type GetLoadOpcode, @function

GetLoadOpcode:

	// *** Basic block 0

	.local load_store_ops
	.global TypeIsUnsigned
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
	mv          s2, x0
	la          t0, load_store_ops
	ld          t0, 0(t0)
	beq         t0, x0, .GetLoadOpcode_label_70

	// *** Basic block 1

	ld          s3, 16(s1)

	// *** Basic block 2

.GetLoadOpcode_label_31:
	slli        t0, s2, 3
	slli        t1, s2, 4
	add         t0, t0, t1
	la          t1, load_store_ops
	add         s1, t1, t0
	ld          t0, 0(s1)
	mv          a0, s3
	jalr         x1, t0, 0

	// *** Basic block 3

	beqz        a0, .GetLoadOpcode_label_58

	// *** Basic block 4

	mv          a0, s3
	call        TypeIsUnsigned

	// *** Basic block 5

	beqz        a0, .GetLoadOpcode_label_53

	// *** Basic block 6

	lw          a0, 12(s1)

	// *** Basic block 7

.GetLoadOpcode_label_50:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 8

.GetLoadOpcode_label_53:
	lw          a0, 8(s1)
	j           .GetLoadOpcode_label_50

	// *** Basic block 9

.GetLoadOpcode_label_58:

	// *** Basic block 10

.GetLoadOpcode_label_59:
	addi        s2, s2, 1
	slli        t0, s2, 3
	slli        t1, s2, 4
	add         t0, t0, t1
	la          t1, load_store_ops
	add         t0, t1, t0
	ld          t0, 0(t0)
	beq         t0, x0, .GetLoadOpcode_label_31

	// *** Basic block 11

.GetLoadOpcode_label_70:
	lla         a0, .str.4
	lla         a1, .str.5
	lla         a3, .str.6
	li          t0, 131		// 0x83 ASCII \x83
	mv          a2, t0
	call        printf

	// *** Basic block 12

	call        abort

	// *** Basic block 13

	mv          a0, x0
	j           .GetLoadOpcode_label_50
.func_end_GetLoadOpcode:
	.size GetLoadOpcode, .func_end_GetLoadOpcode-GetLoadOpcode

	.local  GetStoreOpcode
	.type GetStoreOpcode, @function

GetStoreOpcode:

	// *** Basic block 0

	.local load_store_ops
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
	mv          s2, x0
	la          t0, load_store_ops
	ld          t0, 0(t0)
	beq         t0, x0, .GetStoreOpcode_label_59

	// *** Basic block 1

	ld          s3, 16(s1)

	// *** Basic block 2

.GetStoreOpcode_label_28:
	slli        t0, s2, 3
	slli        t1, s2, 4
	add         t0, t0, t1
	la          t1, load_store_ops
	add         s1, t1, t0
	ld          t0, 0(s1)
	mv          a0, s3
	jalr         x1, t0, 0

	// *** Basic block 3

	beqz        a0, .GetStoreOpcode_label_47

	// *** Basic block 4

	lw          a0, 16(s1)

	// *** Basic block 5

.GetStoreOpcode_label_44:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.GetStoreOpcode_label_47:

	// *** Basic block 7

.GetStoreOpcode_label_48:
	addi        s2, s2, 1
	slli        t0, s2, 3
	slli        t1, s2, 4
	add         t0, t0, t1
	la          t1, load_store_ops
	add         t0, t1, t0
	ld          t0, 0(t0)
	beq         t0, x0, .GetStoreOpcode_label_28

	// *** Basic block 8

.GetStoreOpcode_label_59:
	lla         a0, .str.7
	lla         a1, .str.8
	lla         a3, .str.9
	li          t0, 141		// 0x8d ASCII \x8d
	mv          a2, t0
	call        printf

	// *** Basic block 9

	call        abort

	// *** Basic block 10

	mv          a0, x0
	j           .GetStoreOpcode_label_44
.func_end_GetStoreOpcode:
	.size GetStoreOpcode, .func_end_GetStoreOpcode-GetStoreOpcode

	.local  GenerateLiteral
	.type GenerateLiteral, @function

GenerateLiteral:

	// *** Basic block 0

	.global CompilerAddStringLiteral
	.global IRSetType
	.global GeneratorEmit
	.global NewIR1
	.global GeneratorGetIntConstant
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
	ld          a0, 56(s1)
	call        CompilerAddStringLiteral

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 16(s1)
	mv          a2, s3
	mv          a1, s4
	mv          a0, s2
	call        GeneratorGetIntConstant

	// *** Basic block 2

	mv          a1, a0
	li          t0, 104		// 0x68 ASCII 'h'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 3

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 4

	mv          a1, s4
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType
.func_end_GenerateLiteral:
	.size GenerateLiteral, .func_end_GenerateLiteral-GenerateLiteral

	.local  MoveToTmpOpcode
	.type MoveToTmpOpcode, @function

MoveToTmpOpcode:

	// *** Basic block 0

	.local rmov_opcodes
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
	mv          s2, x0
	la          t0, rmov_opcodes
	ld          t0, 0(t0)
	beq         t0, x0, .MoveToTmpOpcode_label_52

	// *** Basic block 1

.MoveToTmpOpcode_label_26:
	slli        t0, s2, 4
	la          t1, rmov_opcodes
	add         s3, t1, t0
	ld          t0, 0(s3)
	mv          a0, s1
	jalr         x1, t0, 0

	// *** Basic block 2

	beqz        a0, .MoveToTmpOpcode_label_42

	// *** Basic block 3

	lw          a0, 8(s3)

	// *** Basic block 4

.MoveToTmpOpcode_label_39:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.MoveToTmpOpcode_label_42:

	// *** Basic block 6

.MoveToTmpOpcode_label_43:
	addi        s2, s2, 1
	slli        t0, s2, 4
	la          t1, rmov_opcodes
	add         t0, t1, t0
	ld          t0, 0(t0)
	beq         t0, x0, .MoveToTmpOpcode_label_26

	// *** Basic block 7

.MoveToTmpOpcode_label_52:
	lla         a0, .str.10
	lla         a1, .str.11
	lla         a3, .str.12
	li          t0, 172		// 0xac ASCII \xac
	mv          a2, t0
	call        printf

	// *** Basic block 8

	call        abort

	// *** Basic block 9

	mv          a0, x0
	j           .MoveToTmpOpcode_label_39
.func_end_MoveToTmpOpcode:
	.size MoveToTmpOpcode, .func_end_MoveToTmpOpcode-MoveToTmpOpcode

	.local  RemoveUnnecesaryShortening
	.type RemoveUnnecesaryShortening, @function

RemoveUnnecesaryShortening:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a2
	lw          t3, 20(t0)
	addi        t4, t3, -117
	snez        t2, t4
	li          t4, 117		// 0x75 ASCII 'u'
	beq         t3, t4, .RemoveUnnecesaryShortening_label_31

	// *** Basic block 1

	addi        t3, t3, -116
	snez        t2, t3

	// *** Basic block 2

.RemoveUnnecesaryShortening_label_31:
	beqz        t2, .RemoveUnnecesaryShortening_label_38

	// *** Basic block 3

	mv          a0, t0

	// *** Basic block 4

.RemoveUnnecesaryShortening_label_35:
	ret         

	// *** Basic block 5

.RemoveUnnecesaryShortening_label_38:
	ld          t2, 24(t0)
	ld          t3, 0(t2)
	ld          t2, 80(t3)
	lw          t4, 20(t2)
	mv          t2, x0
	li          t5, 30		// 0x1e ASCII \x1e
	blt         t1, t5, .RemoveUnnecesaryShortening_label_79

	// *** Basic block 6

	li          t5, 32		// 0x20 ASCII ' '
	blt         t5, t1, .RemoveUnnecesaryShortening_label_79

	// *** Basic block 7

	addi        t5, t1, -30
	slli        t5, t5, 2
	auipc       t6, 0
	add         t5, t6, t5
	jalr        x0, t5, 12

	// *** Basic block 8

	j           .RemoveUnnecesaryShortening_label_75

	// *** Basic block 9

	j           .RemoveUnnecesaryShortening_label_67

	// *** Basic block 10

	j           .RemoveUnnecesaryShortening_label_71

	// *** Basic block 11

.RemoveUnnecesaryShortening_label_67:
	li          t1, 1		// 0x1 ASCII \x1
	slt         t2, t1, t4
	j           .RemoveUnnecesaryShortening_label_83

	// *** Basic block 12

.RemoveUnnecesaryShortening_label_71:
	li          t1, 2		// 0x2 ASCII \x2
	slt         t2, t1, t4
	j           .RemoveUnnecesaryShortening_label_83

	// *** Basic block 13

.RemoveUnnecesaryShortening_label_75:
	li          t1, 4		// 0x4 ASCII \x4
	slt         t2, t1, t4
	j           .RemoveUnnecesaryShortening_label_83

	// *** Basic block 14

.RemoveUnnecesaryShortening_label_79:
	mv          a0, t0
	ret         

	// *** Basic block 15

.RemoveUnnecesaryShortening_label_83:
	beqz        t2, .RemoveUnnecesaryShortening_label_89

	// *** Basic block 16

	mv          a0, t3
	j           .RemoveUnnecesaryShortening_label_91

	// *** Basic block 17

.RemoveUnnecesaryShortening_label_89:
	mv          a0, t0

	// *** Basic block 18

.RemoveUnnecesaryShortening_label_91:
	ret         
.func_end_RemoveUnnecesaryShortening:
	.size RemoveUnnecesaryShortening, .func_end_RemoveUnnecesaryShortening-RemoveUnnecesaryShortening

	.local  StashCallResult
	.type StashCallResult, @function

StashCallResult:

	// *** Basic block 0

	.global GeneratorEmit
	.global NewIR
	.local MoveToTmpOpcode
	.global NewIR2
	.global IRSetType
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
	li          a0, 1		// 0x1 ASCII \x1
	call        NewIR

	// *** Basic block 1

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 2

	mv          s3, a0
	ld          s4, 80(s2)
	mv          a0, s4
	call        MoveToTmpOpcode

	// *** Basic block 3

	mv          s5, a0
	mv          a2, s2
	mv          a1, s3
	mv          a0, s5
	call        NewIR2

	// *** Basic block 4

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 5

	mv          a1, s4
	mv          a0, s3
	call        IRSetType

	// *** Basic block 6

	mv          a0, s3

	// *** Basic block 7

.StashCallResult_label_53:
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
.func_end_StashCallResult:
	.size StashCallResult, .func_end_StashCallResult-StashCallResult

	.local  FindCall
	.type FindCall, @function

FindCall:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	lw          t1, 0(a0)
	li          t2, 46		// 0x2e ASCII '.'
	bne         t1, t2, .FindCall_label_22

	// *** Basic block 1

	li          t1, 1		// 0x1 ASCII \x1
	sb          t1, 0(t0)

	// *** Basic block 2

.FindCall_label_22:
	ret         
.func_end_FindCall:
	.size FindCall, .func_end_FindCall-FindCall

	.local  ContainsCall
	.type ContainsCall, @function

ContainsCall:

	// *** Basic block 0

	.global ASTNodeVisit
	.local FindCall
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -32(s0)
	// End of stack frame
	sd          x0, -32(s0)
	sb          x0, -32(s0)
	addi        a3, s0, -32
	mv          a2, x0
	la          a1, FindCall
	call        ASTNodeVisit

	// *** Basic block 1

	lb          a0, -32(s0)

	// *** Basic block 2

.ContainsCall_label_27:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ContainsCall:
	.size ContainsCall, .func_end_ContainsCall-ContainsCall

	.local  GenerateBinaryExpression
	.type GenerateBinaryExpression, @function

GenerateBinaryExpression:

	// *** Basic block 0

	.local ContainsCall
	.global GenerateExpression
	.local StashCallResult
	.local FindIROpcode
	.global IRSetType
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
	mv          s1, a1
	mv          s2, a0
	ld          s4, 56(s1)
	mv          a0, s4
	call        ContainsCall

	// *** Basic block 1

	mv          s3, a0
	beqz        a0, .GenerateBinaryExpression_label_33

	// *** Basic block 2

	ld          a0, 64(s1)
	call        ContainsCall

	// *** Basic block 4

.GenerateBinaryExpression_label_33:
	mv          a1, s4
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 5

	mv          s4, a0
	beqz        s3, .GenerateBinaryExpression_label_49

	// *** Basic block 6

	mv          a1, s4
	mv          a0, s2
	call        StashCallResult

	// *** Basic block 7

	mv          s4, a0

	// *** Basic block 8

.GenerateBinaryExpression_label_49:
	ld          s5, 64(s1)
	mv          a1, s5
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 9

	mv          s6, a0
	beqz        s3, .GenerateBinaryExpression_label_66

	// *** Basic block 10

	mv          a1, s6
	mv          a0, s2
	call        StashCallResult

	// *** Basic block 11

	mv          s6, a0

	// *** Basic block 12

.GenerateBinaryExpression_label_66:
	lw          a1, 0(s1)
	mv          a0, s5
	call        FindIROpcode

	// *** Basic block 13

	mv          s3, a0
	mv          a2, s6
	mv          a1, s4
	mv          a0, s3
	call        NewIR2

	// *** Basic block 14

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 15

	ld          a1, 16(s1)
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
	j           IRSetType
.func_end_GenerateBinaryExpression:
	.size GenerateBinaryExpression, .func_end_GenerateBinaryExpression-GenerateBinaryExpression

	.local  GenerateUnaryExpression
	.type GenerateUnaryExpression, @function

GenerateUnaryExpression:

	// *** Basic block 0

	.global GenerateExpression
	.local FindIROpcode
	.global IRSetType
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
	mv          s1, a0
	mv          s2, a1
	ld          a1, 56(s2)
	call        GenerateExpression

	// *** Basic block 1

	mv          s3, a0
	lw          t0, 0(s2)
	li          t1, 10		// 0xa ASCII \xa
	bne         t0, t1, .GenerateUnaryExpression_label_37

	// *** Basic block 2

	mv          a0, s3

	// *** Basic block 3

.GenerateUnaryExpression_label_34:
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

.GenerateUnaryExpression_label_37:
	lw          a1, 0(s2)
	mv          a0, s2
	call        FindIROpcode

	// *** Basic block 5

	mv          s4, a0
	mv          a1, s3
	mv          a0, s4
	call        NewIR1

	// *** Basic block 6

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 7

	ld          a1, 16(s2)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType
.func_end_GenerateUnaryExpression:
	.size GenerateUnaryExpression, .func_end_GenerateUnaryExpression-GenerateUnaryExpression

	.local  IncDecOp
	.type IncDecOp, @function

IncDecOp:

	// *** Basic block 0

	.global TypeIsIntegral
	.global TypeIsFloat
	.global TypeIsDouble
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
	mv          t0, a0
	mv          s1, a1
	ld          s2, 16(t0)
	mv          a0, s2
	call        TypeIsIntegral

	// *** Basic block 1

	beqz        a0, .IncDecOp_label_40

	// *** Basic block 2

	beqz        s1, .IncDecOp_label_33

	// *** Basic block 3

	li          a0, 37		// 0x25 ASCII '%'
	j           .IncDecOp_label_35

	// *** Basic block 4

.IncDecOp_label_33:
	li          a0, 41		// 0x29 ASCII ')'

	// *** Basic block 5

.IncDecOp_label_35:

	// *** Basic block 6

.IncDecOp_label_37:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.IncDecOp_label_40:
	mv          a0, s2
	call        TypeIsFloat

	// *** Basic block 8

	beqz        a0, .IncDecOp_label_55

	// *** Basic block 9

	beqz        s1, .IncDecOp_label_50

	// *** Basic block 10

	li          a0, 38		// 0x26 ASCII '&'
	j           .IncDecOp_label_52

	// *** Basic block 11

.IncDecOp_label_50:
	li          a0, 42		// 0x2a ASCII '*'

	// *** Basic block 12

.IncDecOp_label_52:
	j           .IncDecOp_label_37

	// *** Basic block 13

.IncDecOp_label_55:
	mv          a0, s2
	call        TypeIsDouble

	// *** Basic block 14

	beqz        a0, .IncDecOp_label_70

	// *** Basic block 15

	beqz        s1, .IncDecOp_label_65

	// *** Basic block 16

	li          a0, 39		// 0x27 ASCII '''
	j           .IncDecOp_label_67

	// *** Basic block 17

.IncDecOp_label_65:
	li          a0, 43		// 0x2b ASCII '+'

	// *** Basic block 18

.IncDecOp_label_67:
	j           .IncDecOp_label_37

	// *** Basic block 19

.IncDecOp_label_70:
	beqz        s1, .IncDecOp_label_76

	// *** Basic block 20

	li          a0, 40		// 0x28 ASCII '('
	j           .IncDecOp_label_78

	// *** Basic block 21

.IncDecOp_label_76:
	li          a0, 44		// 0x2c ASCII ','

	// *** Basic block 22

.IncDecOp_label_78:
	j           .IncDecOp_label_37
.func_end_IncDecOp:
	.size IncDecOp, .func_end_IncDecOp-IncDecOp

	.local  LoadBitfield
	.type LoadBitfield, @function

LoadBitfield:

	// *** Basic block 0

	.global CheckForVarUse
	.global TypeIsUnsigned
	.global IRSetType
	.global GeneratorEmit
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
	mv          s1, a2
	mv          s2, a1
	mv          s3, a0
	ld          t0, 64(s1)
	ld          s4, 56(t0)
	lw          s5, 16(s4)
	ld          t0, 0(s4)
	ld          s6, 40(t0)
	lw          t0, 20(s6)
	slli        t0, t0, 3
	bne         s5, t0, .LoadBitfield_label_56

	// *** Basic block 1

	mv          a0, s2

	// *** Basic block 2

.LoadBitfield_label_53:
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

.LoadBitfield_label_56:
	mv          a1, s1
	mv          a0, s2
	call        CheckForVarUse

	// *** Basic block 4

	mv          a0, s6
	call        TypeIsUnsigned

	// *** Basic block 5

	beqz        a0, .LoadBitfield_label_131

	// *** Basic block 6

	lw          s7, 12(s4)
	bnez        s7, .LoadBitfield_label_73

	// *** Basic block 7

	mv          s1, s2
	j           .LoadBitfield_label_100

	// *** Basic block 8

.LoadBitfield_label_73:
	mv          a2, s7
	mv          a1, s6
	mv          a0, s3
	call        GeneratorGetIntConstant

	// *** Basic block 9

	mv          a2, a0
	mv          a1, s2
	li          t0, 52		// 0x34 ASCII '4'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 10

	mv          a1, a0
	mv          a0, s3
	call        GeneratorEmit

	// *** Basic block 11

	mv          a1, s6
	call        IRSetType

	// *** Basic block 12

	mv          s1, a0

	// *** Basic block 13

.LoadBitfield_label_100:
	li          t0, 1		// 0x1 ASCII \x1
	sll         t0, t0, s5
	addi        s7, t0, -1
	mv          a2, s7
	mv          a1, s6
	mv          a0, s3
	call        GeneratorGetIntConstant

	// *** Basic block 14

	mv          a2, a0
	mv          a1, s1
	li          t0, 116		// 0x74 ASCII 't'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 15

	mv          a1, a0
	mv          a0, s3
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
	j           GeneratorEmit

	// *** Basic block 17

.LoadBitfield_label_131:
	la          t0, compiler
	ld          t0, 0(t0)
	lw          t0, 1080(t0)
	slli        s1, t0, 3
	lw          t0, 12(s4)
	add         t0, s5, t0
	sub         a2, s1, t0
	mv          a1, s6
	mv          a0, s3
	call        GeneratorGetIntConstant

	// *** Basic block 18

	mv          a2, a0
	mv          a1, s2
	li          t0, 54		// 0x36 ASCII '6'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s3
	call        GeneratorEmit

	// *** Basic block 20

	mv          a1, s6
	call        IRSetType

	// *** Basic block 21

	mv          s4, a0
	sub         a2, s1, s5
	mv          a1, s6
	mv          a0, s3
	call        GeneratorGetIntConstant

	// *** Basic block 22

	mv          a2, a0
	mv          a1, s4
	li          t0, 53		// 0x35 ASCII '5'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 23

	mv          a1, a0
	mv          a0, s3
	call        GeneratorEmit

	// *** Basic block 24

	mv          a1, s6
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
	j           IRSetType
.func_end_LoadBitfield:
	.size LoadBitfield, .func_end_LoadBitfield-LoadBitfield

	.local  CalculateNewBitfieldValue
	.type CalculateNewBitfieldValue, @function

CalculateNewBitfieldValue:

	// *** Basic block 0

	.global GeneratorEmit
	.global NewIR2
	.global GeneratorGetIntConstant
	.global IRIsConst
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
	mv          s1, a3
	mv          s2, a2
	mv          s3, a0
	mv          s4, a1
	ld          t0, 64(s1)
	ld          s5, 56(t0)
	lw          s6, 16(s5)
	ld          t0, 0(s5)
	ld          t0, 40(t0)
	lw          t0, 20(t0)
	slli        t0, t0, 3
	bne         s6, t0, .CalculateNewBitfieldValue_label_54

	// *** Basic block 1

	mv          a0, s2

	// *** Basic block 2

.CalculateNewBitfieldValue_label_51:
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

	// *** Basic block 3

.CalculateNewBitfieldValue_label_54:
	li          t0, 1		// 0x1 ASCII \x1
	sll         t0, t0, s6
	addi        s6, t0, -1
	lw          s5, 12(s5)
	sll         t0, s6, s5
	not         s7, t0
	ld          s8, 16(s1)
	mv          a2, s7
	mv          a1, s8
	mv          a0, s3
	call        GeneratorGetIntConstant

	// *** Basic block 4

	mv          a2, a0
	mv          a1, s4
	li          s7, 56		// 0x38 ASCII '8'
	mv          a0, s7
	call        NewIR2

	// *** Basic block 5

	mv          a1, a0
	mv          a0, s3
	call        GeneratorEmit

	// *** Basic block 6

	mv          s9, a0
	mv          a0, s2
	call        IRIsConst

	// *** Basic block 7

	beqz        a0, .CalculateNewBitfieldValue_label_107

	// *** Basic block 8

	ld          s1, 136(s2)
	and         t0, s1, s6
	sll         a2, t0, s5
	mv          a1, s8
	mv          a0, s3
	call        GeneratorGetIntConstant

	// *** Basic block 9

	mv          s1, a0
	j           .CalculateNewBitfieldValue_label_158

	// *** Basic block 10

.CalculateNewBitfieldValue_label_107:
	mv          a2, s6
	mv          a1, s8
	mv          a0, s3
	call        GeneratorGetIntConstant

	// *** Basic block 11

	mv          a2, a0
	mv          a1, s2
	mv          a0, s7
	call        NewIR2

	// *** Basic block 12

	mv          a1, a0
	mv          a0, s3
	call        GeneratorEmit

	// *** Basic block 13

	mv          s4, a0
	bnez        s5, .CalculateNewBitfieldValue_label_135

	// *** Basic block 14

	mv          s1, s4
	j           .CalculateNewBitfieldValue_label_157

	// *** Basic block 15

.CalculateNewBitfieldValue_label_135:
	mv          a2, s5
	mv          a1, s8
	mv          a0, s3
	call        GeneratorGetIntConstant

	// *** Basic block 16

	mv          a2, a0
	mv          a1, s4
	li          t0, 54		// 0x36 ASCII '6'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 17

	mv          a1, a0
	mv          a0, s3
	call        GeneratorEmit

	// *** Basic block 18

	mv          s1, a0

	// *** Basic block 19

.CalculateNewBitfieldValue_label_157:

	// *** Basic block 20

.CalculateNewBitfieldValue_label_158:
	mv          a2, s1
	mv          a1, s9
	li          t0, 55		// 0x37 ASCII '7'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 21

	mv          a1, a0
	mv          a0, s3
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
	j           GeneratorEmit
.func_end_CalculateNewBitfieldValue:
	.size CalculateNewBitfieldValue, .func_end_CalculateNewBitfieldValue-CalculateNewBitfieldValue

	.local  GenerateVariableReference
	.type GenerateVariableReference, @function

GenerateVariableReference:

	// *** Basic block 0

	.global GeneratorGetVariable
	.global TypeIsStructOrUnion
	.local GetLoadOpcode
	.global GeneratorEmit
	.global NewIR1
	.global IRSetVarUse
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
	ld          a1, 56(s2)
	call        GeneratorGetVariable

	// *** Basic block 1

	mv          s3, a0
	lw          t0, 8(s2)
	andi        t0, t0, 1
	beqz        t0, .GenerateVariableReference_label_39

	// *** Basic block 2

	mv          s4, s3
	j           .GenerateVariableReference_label_71

	// *** Basic block 3

.GenerateVariableReference_label_39:
	ld          a0, 16(s2)
	call        TypeIsStructOrUnion

	// *** Basic block 4

	beqz        a0, .GenerateVariableReference_label_47

	// *** Basic block 5

	mv          s4, s3
	j           .GenerateVariableReference_label_64

	// *** Basic block 6

.GenerateVariableReference_label_47:
	mv          a0, s2
	call        GetLoadOpcode

	// *** Basic block 7

	mv          s5, a0
	mv          a1, s3
	mv          a0, s5
	call        NewIR1

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 9

	mv          s4, a0

	// *** Basic block 10

.GenerateVariableReference_label_64:
	ld          a1, 56(s2)
	mv          a0, s4
	call        IRSetVarUse

	// *** Basic block 11

.GenerateVariableReference_label_71:
	lw          t0, 8(s2)
	andi        t0, t0, 64
	beqz        t0, .GenerateVariableReference_label_81

	// *** Basic block 12

	lw          t0, 88(s3)
	ori         t0, t0, 32
	sw          t0, 88(s3)

	// *** Basic block 13

.GenerateVariableReference_label_81:
	mv          a0, s4

	// *** Basic block 14

.GenerateVariableReference_label_84:
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
.func_end_GenerateVariableReference:
	.size GenerateVariableReference, .func_end_GenerateVariableReference-GenerateVariableReference

	.local  GenerateIncDec
	.type GenerateIncDec, @function

GenerateIncDec:

	// *** Basic block 0

	.global GenerateExpression
	.global GeneratorGetIntConstant
	.global TypeIsFloatingPoint
	.global TypeIsFloat
	.global GeneratorGetFloatingPointConstant
	.local GetLoadOpcode
	.global GeneratorEmit
	.global NewIR1
	.global IsBitfieldReference
	.local LoadBitfield
	.global CheckForVarUse
	.global OptLevel0
	.global ASTNodeUsesValue
	.global NewIR
	.global NewIR2
	.global IRSetType
	.local IncDecOp
	.local CalculateNewBitfieldValue
	.local GetStoreOpcode
	.local RemoveUnnecesaryShortening
	.global CheckForVarDef
	sd          a0, -0(s0)	// Spilled @342
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
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	mv          s4, a3
	ld          a2, 56(s2)
	mv          a1, a2
	call        GenerateExpression

	// *** Basic block 1

	mv          s5, a0
	ld          s6, 16(s2)
	ld          s7, 16(a2)
	lw          t0, 16(s7)
	addi        t0, t0, -1
	seqz        s7, t0

	// *** Basic block 2

.GenerateIncDec_label_74:
	beqz        s7, .GenerateIncDec_label_126

	// *** Basic block 3

	j           .GenerateIncDec_label_77

	// *** Basic block 4

.GenerateIncDec_label_77:
	ld          s8, 24(s7)
	lw          s10, 16(s8)
	addi        t0, s10, -2
	seqz        s9, t0
	li          t0, 2		// 0x2 ASCII \x2
	beq         s10, t0, .GenerateIncDec_label_92

	// *** Basic block 5

	addi        t0, s10, -1
	seqz        s9, t0

	// *** Basic block 6

.GenerateIncDec_label_92:
	beqz        s9, .GenerateIncDec_label_99

	// *** Basic block 7

	addi        t0, s8, 32
	lb          t0, 16(t0)
	slli        t0, t0, 61
	srai        s9, t0, 63

	// *** Basic block 8

.GenerateIncDec_label_99:

	// *** Basic block 9

.GenerateIncDec_label_101:
	beqz        s9, .GenerateIncDec_label_112

	// *** Basic block 10

	j           .GenerateIncDec_label_104

	// *** Basic block 11

.GenerateIncDec_label_104:
	addi        t0, s8, 32
	ld          s9, 8(t0)
	li          s10, 16		// 0x10 ASCII \x10
	j           .GenerateIncDec_label_124

	// *** Basic block 12

.GenerateIncDec_label_112:
	lw          a2, 20(s8)
	mv          a1, s6
	sd          s6, -32(s0)	// Spilled @64
	mv          a0, s1
	call        GeneratorGetIntConstant

	// *** Basic block 13

	mv          s9, a0
	li          s10, 16		// 0x10 ASCII \x10
	mv          s6, s7

	// *** Basic block 14

.GenerateIncDec_label_124:
	j           .GenerateIncDec_label_165

	// *** Basic block 15

.GenerateIncDec_label_126:
	li          s10, 13		// 0xd ASCII \xd
	mv          a0, s7
	call        TypeIsFloatingPoint

	// *** Basic block 16

	beqz        a0, .GenerateIncDec_label_154

	// *** Basic block 17

	mv          a0, s7
	call        TypeIsFloat

	// *** Basic block 18

	beqz        a0, .GenerateIncDec_label_138

	// *** Basic block 19

	li          s10, 14		// 0xe ASCII \xe
	j           .GenerateIncDec_label_140

	// *** Basic block 20

.GenerateIncDec_label_138:
	li          s10, 15		// 0xf ASCII \xf

	// *** Basic block 21

.GenerateIncDec_label_140:
	li          t0, 4607182418800017408		// 0x3ff0000000000000
	fmv.d.x     ft0, t0
	fmv.d       fa0, ft0
	mv          a1, s6
	mv          a0, s1
	call        GeneratorGetFloatingPointConstant

	// *** Basic block 22

	mv          s9, a0
	j           .GenerateIncDec_label_164

	// *** Basic block 23

.GenerateIncDec_label_154:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a1, s6
	mv          a0, s1
	call        GeneratorGetIntConstant

	// *** Basic block 24

	mv          s9, a0

	// *** Basic block 25

.GenerateIncDec_label_164:

	// *** Basic block 26

.GenerateIncDec_label_165:
	mv          a0, s2
	call        GetLoadOpcode

	// *** Basic block 27

	mv          s7, a0
	mv          a1, s5
	mv          a0, s7
	call        NewIR1

	// *** Basic block 28

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 29

	mv          s8, a0
	mv          s11, s8
	mv          a0, a2
	call        IsBitfieldReference

	// *** Basic block 30

	beqz        a0, .GenerateIncDec_label_196

	// *** Basic block 31

	mv          a1, s8
	mv          a0, s1
	call        LoadBitfield

	// *** Basic block 32

	mv          s11, a0

	// *** Basic block 33

.GenerateIncDec_label_196:
	mv          a1, a2
	mv          a0, s8
	call        CheckForVarUse

	// *** Basic block 34

	call        OptLevel0

	// *** Basic block 35

	mv          s7, a0
	bnez        a0, .GenerateIncDec_label_213

	// *** Basic block 36

	ld          a0, 24(s2)
	mv          a1, s2
	call        ASTNodeUsesValue

	// *** Basic block 38

.GenerateIncDec_label_213:
	mv          a0, x0
	mv          s6, s3
	beqz        s3, .GenerateIncDec_label_221

	// *** Basic block 39

	mv          s6, s7

	// *** Basic block 40

.GenerateIncDec_label_221:
	beqz        s6, .GenerateIncDec_label_252

	// *** Basic block 41

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	call        NewIR

	// *** Basic block 42

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 43

	mv          a2, s8
	mv          a1, a0
	mv          a0, s10
	call        NewIR2

	// *** Basic block 44

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 45

	ld          a1, 80(s8)
	call        IRSetType

	// *** Basic block 46

.GenerateIncDec_label_252:
	ld          a3, 56(s2)
	mv          a1, s4
	mv          a0, a3
	call        IncDecOp

	// *** Basic block 47

	mv          s3, a0
	mv          a2, s9
	mv          a1, s11
	mv          a0, s3
	call        NewIR2

	// *** Basic block 48

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 49

	mv          s7, a0
	ld          a1, 16(s2)
	mv          a0, s7
	call        IRSetType

	// *** Basic block 50

	mv          a0, a3
	call        IsBitfieldReference

	// *** Basic block 51

	beqz        a0, .GenerateIncDec_label_295

	// *** Basic block 52

	mv          a2, s7
	mv          a1, s8
	mv          a0, s1
	call        CalculateNewBitfieldValue

	// *** Basic block 53

	mv          s7, a0

	// *** Basic block 54

.GenerateIncDec_label_295:
	mv          a0, s2
	call        GetStoreOpcode

	// *** Basic block 55

	mv          s3, a0
	mv          a2, s3
	mv          a1, s7
	mv          a0, s1
	call        RemoveUnnecesaryShortening

	// *** Basic block 56

	mv          a2, a0
	mv          a1, s5
	mv          a0, s3
	call        NewIR2

	// *** Basic block 57

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 58

	mv          s4, a0
	ld          a1, 80(s7)
	mv          a0, s4
	call        IRSetType

	// *** Basic block 59

	mv          a1, a3
	mv          a0, s4
	call        CheckForVarDef

	// *** Basic block 60

	bne         a0, x0, .GenerateIncDec_label_341

	// *** Basic block 61

	mv          a0, s4

	// *** Basic block 62

.GenerateIncDec_label_338:
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

	// *** Basic block 63

.GenerateIncDec_label_341:
	j           .GenerateIncDec_label_338
.func_end_GenerateIncDec:
	.size GenerateIncDec, .func_end_GenerateIncDec-GenerateIncDec

	.local  GeneratePointerScale
	.type GeneratePointerScale, @function

GeneratePointerScale:

	// *** Basic block 0

	.global GenerateExpression
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
	ld          a1, 72(s2)
	call        GenerateExpression

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 56(s2)
	lw          s6, 16(s4)
	addi        t0, s6, -2
	seqz        s5, t0
	li          t0, 2		// 0x2 ASCII \x2
	beq         s6, t0, .GeneratePointerScale_label_51

	// *** Basic block 2

	addi        t0, s6, -1
	seqz        s5, t0

	// *** Basic block 3

.GeneratePointerScale_label_51:
	beqz        s5, .GeneratePointerScale_label_58

	// *** Basic block 4

	addi        t0, s4, 32
	lb          t0, 16(t0)
	slli        t0, t0, 61
	srai        s5, t0, 63

	// *** Basic block 5

.GeneratePointerScale_label_58:

	// *** Basic block 6

.GeneratePointerScale_label_60:
	beqz        s5, .GeneratePointerScale_label_69

	// *** Basic block 7

	j           .GeneratePointerScale_label_63

	// *** Basic block 8

.GeneratePointerScale_label_63:
	addi        t0, s4, 32
	ld          s5, 8(t0)
	j           .GeneratePointerScale_label_82

	// *** Basic block 9

.GeneratePointerScale_label_69:
	lw          s6, 20(s4)
	mv          a2, s6
	mv          a1, x0
	mv          a0, s1
	call        GeneratorGetIntConstant

	// *** Basic block 10

	mv          s5, a0

	// *** Basic block 11

.GeneratePointerScale_label_82:
	li          s4, 45		// 0x2d ASCII '-'
	lw          t0, 64(s2)
	li          t1, 66		// 0x42 ASCII 'B'
	bne         t0, t1, .GeneratePointerScale_label_93

	// *** Basic block 12

	li          s4, 48		// 0x30 ASCII '0'

	// *** Basic block 13

.GeneratePointerScale_label_93:
	mv          a2, s5
	mv          a1, s3
	mv          a0, s4
	call        NewIR2

	// *** Basic block 14

	mv          a1, a0
	mv          a0, s1
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
	j           GeneratorEmit
.func_end_GeneratePointerScale:
	.size GeneratePointerScale, .func_end_GeneratePointerScale-GeneratePointerScale

	.local  InitArrayWithString
	.type InitArrayWithString, @function

InitArrayWithString:

	// *** Basic block 0

	.global TypeIsInt
	.global IRSetType
	.global GeneratorEmit
	.global NewIR3
	.global GeneratorGetIntConstant
	.global CheckForVarDef
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
	mv          s3, a3
	mv          s4, a1
	ld          a0, 16(a4)
	lw          s5, 20(a0)
	mv          s6, a4
	call        TypeIsInt

	// *** Basic block 1

	beqz        a0, .InitArrayWithString_label_47

	// *** Basic block 2

	ld          t0, 56(s6)
	ld          s7, 24(t0)
	j           .InitArrayWithString_label_53

	// *** Basic block 3

.InitArrayWithString_label_47:
	ld          t0, 56(s6)
	ld          t0, 24(t0)
	addi        s7, t0, 1

	// *** Basic block 4

.InitArrayWithString_label_53:
	bge         s5, s7, .InitArrayWithString_label_57

	// *** Basic block 5

	mv          s7, s5

	// *** Basic block 6

.InitArrayWithString_label_57:
	mv          a2, s7
	mv          a1, x0
	mv          a0, s1
	call        GeneratorGetIntConstant

	// *** Basic block 7

	mv          a3, a0
	mv          a2, s3
	mv          a1, s2
	li          t0, 120		// 0x78 ASCII 'x'
	mv          a0, t0
	call        NewIR3

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 9

	ld          a1, 16(s4)
	call        IRSetType

	// *** Basic block 10

	mv          s5, a0
	mv          a1, s4
	mv          a0, s5
	call        CheckForVarDef

	// *** Basic block 11

	mv          a0, s5

	// *** Basic block 12

.InitArrayWithString_label_95:
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
.func_end_InitArrayWithString:
	.size InitArrayWithString, .func_end_InitArrayWithString-InitArrayWithString

	.local  GenerateBracedInitializer
	.type GenerateBracedInitializer, @function

GenerateBracedInitializer:

	// *** Basic block 0

	.global printf
	.global abort
	.global GeneratorEmit
	.global NewIR2
	.global GeneratorGetIntConstant
	.global GenerateExpression
	.global TypeIsStructOrUnion
	.global IRSetType
	.global NewIR1
	.global NewPointerTo
	.global CheckForVarUse
	.global NewIR3
	.local InitArrayWithString
	.global IsBitfieldReference
	.local GetLoadOpcode
	.local LoadBitfield
	.local CalculateNewBitfieldValue
	.local GetStoreOpcode
	.local RemoveUnnecesaryShortening
	.global CheckForVarDef
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
	mv          s1, a3
	sd          s1, -24(s0)	// Spilled @43
	mv          s2, a0
	sd          s2, -24(s0)	// Spilled @46
	mv          s3, a1
	mv          s4, x0
	ld          t0, 56(a2)
	ld          s5, 8(t0)
	bge         x0, s5, .GenerateBracedInitializer_label_356

	// *** Basic block 1

	ld          t0, 0(t0)
	ld          s6, 16(s3)

	// *** Basic block 2

.GenerateBracedInitializer_label_64:
	mv          s7, s1
	sd          s7, -32(s0)	// Spilled @65
	slli        t1, s4, 3
	add         t0, t0, t1
	ld          s8, 0(t0)
	lw          t0, 0(s8)
	sd          s8, -24(s0)	// Spilled @71
	li          t1, 89		// 0x59 ASCII 'Y'
	bne         t0, t1, .GenerateBracedInitializer_label_79

	// *** Basic block 3

	j           .GenerateBracedInitializer_label_94

	// *** Basic block 4

.GenerateBracedInitializer_label_79:
	lla         a0, .str.13
	lla         a1, .str.14
	lla         a3, .str.15
	li          t0, 529		// 0x211
	mv          a2, t0
	call        printf

	// *** Basic block 5

	call        abort

	// *** Basic block 6

.GenerateBracedInitializer_label_94:
	mv          s9, s8
	ld          s10, 56(s9)
	beq         s10, x0, .GenerateBracedInitializer_label_177

	// *** Basic block 7

	ld          s11, 0(s10)
	ld          s10, 8(s10)
	bge         x0, s10, .GenerateBracedInitializer_label_176

	// *** Basic block 8

	mv          s2, x0
	mv          s7, x0
	bge         x0, s10, .GenerateBracedInitializer_label_148

	// *** Basic block 9

.GenerateBracedInitializer_label_113:
	slli        t0, s7, 3
	add         t0, s11, t0
	ld          s11, 0(t0)
	lw          t0, 0(s11)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 10

	j           .GenerateBracedInitializer_label_126

	// *** Basic block 11

	j           .GenerateBracedInitializer_label_136

	// *** Basic block 12

.GenerateBracedInitializer_label_126:
	lw          t0, 16(s11)
	ld          t1, 8(s11)
	lw          t1, 20(t1)
	mul         t0, t0, t1
	add         s2, s2, t0
	j           .GenerateBracedInitializer_label_143

	// *** Basic block 13

.GenerateBracedInitializer_label_136:
	ld          t0, 16(s11)
	lw          t0, 8(t0)
	add         s2, s2, t0
	j           .GenerateBracedInitializer_label_143

	// *** Basic block 14

.GenerateBracedInitializer_label_143:

	// *** Basic block 15

.GenerateBracedInitializer_label_144:
	addi        s7, s7, 1
	bge         s7, s10, .GenerateBracedInitializer_label_113

	// *** Basic block 16

.GenerateBracedInitializer_label_148:
	bnez        s2, .GenerateBracedInitializer_label_153

	// *** Basic block 17

	mv          s7, s1
	j           .GenerateBracedInitializer_label_175

	// *** Basic block 18

.GenerateBracedInitializer_label_153:
	mv          a2, s2
	mv          a1, x0
	ld          a0, -24(s0)	// Spilled @46
	call        GeneratorGetIntConstant

	// *** Basic block 19

	mv          a2, a0
	mv          a1, s1
	li          t0, 40		// 0x28 ASCII '('
	mv          a0, t0
	call        NewIR2

	// *** Basic block 20

	mv          a1, a0
	ld          a0, -24(s0)	// Spilled @46
	call        GeneratorEmit

	// *** Basic block 21

	mv          s7, a0

	// *** Basic block 22

.GenerateBracedInitializer_label_175:

	// *** Basic block 23

.GenerateBracedInitializer_label_176:

	// *** Basic block 24

.GenerateBracedInitializer_label_177:
	ld          s10, 64(s9)
	mv          a1, s10
	ld          a0, -24(s0)	// Spilled @46
	call        GenerateExpression

	// *** Basic block 25

	mv          s11, a0
	ld          s8, 16(s9)
	mv          a0, s8
	call        TypeIsStructOrUnion

	// *** Basic block 26

	beqz        a0, .GenerateBracedInitializer_label_248

	// *** Basic block 27

	mv          a1, s11
	li          t0, 103		// 0x67 ASCII 'g'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 28

	mv          a1, a0
	ld          a0, -24(s0)	// Spilled @46
	call        GeneratorEmit

	// *** Basic block 29

	mv          a1, s8
	mv          a0, x0
	call        NewPointerTo

	// *** Basic block 30

	mv          a1, a0
	call        IRSetType

	// *** Basic block 31

	mv          s11, a0
	mv          a1, s10
	mv          a0, s11
	call        CheckForVarUse

	// *** Basic block 32

	ld          t0, -24(s0)	// Spilled @71
	ld          t1, 16(t0)
	lw          a2, 20(t1)
	mv          a1, x0
	ld          a0, -24(s0)	// Spilled @46
	call        GeneratorGetIntConstant

	// *** Basic block 33

	mv          a3, a0
	mv          a2, s11
	ld          a1, -32(s0)	// Spilled @65
	li          t0, 120		// 0x78 ASCII 'x'
	mv          a0, t0
	call        NewIR3

	// *** Basic block 34

	mv          a1, a0
	ld          a0, -24(s0)	// Spilled @46
	call        GeneratorEmit

	// *** Basic block 35

	mv          s9, a0
	j           .GenerateBracedInitializer_label_346

	// *** Basic block 36

.GenerateBracedInitializer_label_248:
	lw          t0, 16(s8)
	addi        t0, t0, -2
	seqz        s8, t0

	// *** Basic block 37

.GenerateBracedInitializer_label_256:
	beqz        s8, .GenerateBracedInitializer_label_274

	// *** Basic block 38

	j           .GenerateBracedInitializer_label_259

	// *** Basic block 39

.GenerateBracedInitializer_label_259:
	mv          a4, s10
	mv          a3, s11
	ld          a2, -32(s0)	// Spilled @65
	mv          a1, s3
	ld          a0, -24(s0)	// Spilled @46
	call        InitArrayWithString

	// *** Basic block 40

	mv          s9, a0
	j           .GenerateBracedInitializer_label_345

	// *** Basic block 41

.GenerateBracedInitializer_label_274:
	ld          a0, -24(s0)	// Spilled @71
	call        IsBitfieldReference

	// *** Basic block 42

	beqz        a0, .GenerateBracedInitializer_label_314

	// *** Basic block 43

	mv          a0, s3
	call        GetLoadOpcode

	// *** Basic block 44

	mv          s8, a0
	mv          a1, s1
	mv          a0, s8
	call        NewIR1

	// *** Basic block 45

	mv          a1, a0
	ld          a0, -24(s0)	// Spilled @46
	call        GeneratorEmit

	// *** Basic block 46

	mv          s10, a0
	ld          a2, -24(s0)	// Spilled @71
	mv          a1, s10
	ld          a0, -24(s0)	// Spilled @46
	call        LoadBitfield

	// *** Basic block 47

	mv          s10, a0
	ld          a3, -24(s0)	// Spilled @71
	mv          a2, s11
	mv          a1, s10
	ld          a0, -24(s0)	// Spilled @46
	call        CalculateNewBitfieldValue

	// *** Basic block 48

	mv          s11, a0

	// *** Basic block 49

.GenerateBracedInitializer_label_314:
	ld          a0, -24(s0)	// Spilled @71
	call        GetStoreOpcode

	// *** Basic block 50

	mv          s1, a0
	mv          a2, s1
	mv          a1, s11
	ld          a0, -24(s0)	// Spilled @46
	call        RemoveUnnecesaryShortening

	// *** Basic block 51

	mv          a2, a0
	ld          a1, -32(s0)	// Spilled @65
	mv          a0, s1
	call        NewIR2

	// *** Basic block 52

	mv          a1, a0
	ld          a0, -24(s0)	// Spilled @46
	call        GeneratorEmit

	// *** Basic block 53

	mv          a1, s6
	call        IRSetType

	// *** Basic block 54

	mv          s9, a0

	// *** Basic block 55

.GenerateBracedInitializer_label_345:

	// *** Basic block 56

.GenerateBracedInitializer_label_346:
	mv          a1, s3
	mv          a0, s9
	call        CheckForVarDef

	// *** Basic block 57

.GenerateBracedInitializer_label_352:
	addi        s4, s4, 1
	bge         s4, s5, .GenerateBracedInitializer_label_64

	// *** Basic block 58

.GenerateBracedInitializer_label_356:
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
.func_end_GenerateBracedInitializer:
	.size GenerateBracedInitializer, .func_end_GenerateBracedInitializer-GenerateBracedInitializer

	.local  CanElideMemzero
	.type CanElideMemzero, @function

CanElideMemzero:

	// *** Basic block 0

	.global TypeIsStructOrUnion
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	ld          s1, 56(a0)
	ld          t0, 8(s1)
	li          t1, 1		// 0x1 ASCII \x1
	beq         t0, t1, .CanElideMemzero_label_28

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.CanElideMemzero_label_25:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.CanElideMemzero_label_28:
	ld          t0, 0(s1)
	ld          s1, 0(t0)
	ld          t0, 64(s1)
	ld          s1, 16(t0)
	mv          a0, s1
	call        TypeIsStructOrUnion

	// *** Basic block 4

	bnez        a0, .CanElideMemzero_label_52

	// *** Basic block 5

	lw          t0, 16(s1)
	addi        t0, t0, -2
	seqz        t1, t0

	// *** Basic block 6

.CanElideMemzero_label_49:
	j           .CanElideMemzero_label_52

	// *** Basic block 7

.CanElideMemzero_label_52:
	j           .CanElideMemzero_label_25
.func_end_CanElideMemzero:
	.size CanElideMemzero, .func_end_CanElideMemzero-CanElideMemzero

	.local  GenerateInitialization
	.type GenerateInitialization, @function

GenerateInitialization:

	// *** Basic block 0

	.global printf
	.global abort
	.global GenerateExpression
	.global TypeIsStructOrUnion
	.local CanElideMemzero
	.global GeneratorEmit
	.global NewIR1
	.global CheckForVarDef
	.local GenerateBracedInitializer
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
	ld          s3, 64(s1)
	lw          t0, 0(s3)
	li          t1, 88		// 0x58 ASCII 'X'
	bne         t0, t1, .GenerateInitialization_label_37

	// *** Basic block 1

	j           .GenerateInitialization_label_54

	// *** Basic block 2

.GenerateInitialization_label_37:
	lla         a0, .str.16
	lla         a1, .str.17
	lla         a3, .str.18
	li          t0, 619		// 0x26b
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.GenerateInitialization_label_54:
	ld          a1, 56(s1)
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 5

	mv          s4, a0
	ld          s6, 16(s1)
	mv          a0, s6
	call        TypeIsStructOrUnion

	// *** Basic block 6

	mv          s5, a0
	bnez        a0, .GenerateInitialization_label_82

	// *** Basic block 7

	lw          t0, 16(s6)
	addi        t0, t0, -2
	seqz        s6, t0

	// *** Basic block 8

.GenerateInitialization_label_79:
	mv          s5, s6
	j           .GenerateInitialization_label_82

	// *** Basic block 9

.GenerateInitialization_label_82:
	beqz        s5, .GenerateInitialization_label_108

	// *** Basic block 10

	mv          a0, s3
	call        CanElideMemzero

	// *** Basic block 11

	not         t0, a0
	beqz        t0, .GenerateInitialization_label_107

	// *** Basic block 12

	mv          a1, s4
	li          t0, 119		// 0x77 ASCII 'w'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 14

	mv          s6, a0
	mv          a1, s1
	mv          a0, s6
	call        CheckForVarDef

	// *** Basic block 15

.GenerateInitialization_label_107:

	// *** Basic block 16

.GenerateInitialization_label_108:
	mv          a3, s4
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        GenerateBracedInitializer

	// *** Basic block 17

	mv          a0, s4

	// *** Basic block 18

.GenerateInitialization_label_120:
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
.func_end_GenerateInitialization:
	.size GenerateInitialization, .func_end_GenerateInitialization-GenerateInitialization

	.local  GenerateAssignment
	.type GenerateAssignment, @function

GenerateAssignment:

	// *** Basic block 0

	.global GenerateExpression
	.global TypeIsStructOrUnion
	.global GeneratorEmit
	.global NewIR1
	.global IRSetType
	.global NewPointerTo
	.global CheckForVarUse
	.global NewIR3
	.global GeneratorGetIntConstant
	.global IsBitfieldReference
	.local GetLoadOpcode
	.local CalculateNewBitfieldValue
	.local GetStoreOpcode
	.global NewIR2
	.local RemoveUnnecesaryShortening
	.global CheckForVarDef
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
	ld          a3, 56(s2)
	mv          a1, a3
	call        GenerateExpression

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 16(a3)
	mv          a0, s4
	call        TypeIsStructOrUnion

	// *** Basic block 2

	beqz        a0, .GenerateAssignment_label_154

	// *** Basic block 3

	ld          s5, 64(s2)
	lw          t0, 0(s5)
	li          t1, 46		// 0x2e ASCII '.'
	bne         t0, t1, .GenerateAssignment_label_99

	// *** Basic block 4

	ld          s6, 80(s1)
	mv          a1, s3
	li          t0, 103		// 0x67 ASCII 'g'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 5

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 6

	mv          s7, a0
	mv          a1, s4
	mv          a0, x0
	call        NewPointerTo

	// *** Basic block 7

	mv          a1, a0
	mv          a0, s7
	call        IRSetType

	// *** Basic block 8

	sd          s7, 80(s1)
	mv          a1, s5
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 9

	mv          s4, a0
	sd          s6, 80(s1)
	mv          a0, s4

	// *** Basic block 10

.GenerateAssignment_label_96:
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

	// *** Basic block 11

.GenerateAssignment_label_99:
	mv          a1, s5
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 12

	mv          s4, a0
	mv          a1, s4
	li          t0, 103		// 0x67 ASCII 'g'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 14

	mv          s4, a0
	mv          a1, s5
	mv          a0, s4
	call        CheckForVarUse

	// *** Basic block 15

	ld          t0, 16(s2)
	lw          a2, 20(t0)
	mv          a1, x0
	mv          a0, s1
	call        GeneratorGetIntConstant

	// *** Basic block 16

	mv          a3, a0
	mv          a2, s4
	mv          a1, s3
	li          t0, 120		// 0x78 ASCII 'x'
	mv          a0, t0
	call        NewIR3

	// *** Basic block 17

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 18

	mv          s5, a0

	// *** Basic block 19

.GenerateAssignment_label_152:
	j           .GenerateAssignment_label_223

	// *** Basic block 20

.GenerateAssignment_label_154:
	ld          a1, 64(s2)
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 21

	mv          s4, a0
	mv          a0, a3
	call        IsBitfieldReference

	// *** Basic block 22

	beqz        a0, .GenerateAssignment_label_197

	// *** Basic block 23

	mv          a0, a3
	call        GetLoadOpcode

	// *** Basic block 24

	mv          s6, a0
	mv          a1, s3
	mv          a0, s6
	call        NewIR1

	// *** Basic block 25

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 26

	mv          s8, a0
	mv          a1, a3
	mv          a0, s8
	call        CheckForVarUse

	// *** Basic block 27

	mv          a2, s4
	mv          a1, s8
	mv          a0, s1
	call        CalculateNewBitfieldValue

	// *** Basic block 28

	mv          s4, a0

	// *** Basic block 29

.GenerateAssignment_label_197:
	mv          a0, s2
	call        GetStoreOpcode

	// *** Basic block 30

	mv          s9, a0
	mv          a2, s9
	mv          a1, s4
	mv          a0, s1
	call        RemoveUnnecesaryShortening

	// *** Basic block 31

	mv          a2, a0
	mv          a1, s3
	mv          a0, s9
	call        NewIR2

	// *** Basic block 32

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 33

	mv          s5, a0

	// *** Basic block 34

.GenerateAssignment_label_223:
	mv          a1, a3
	mv          a0, s5
	call        CheckForVarDef

	// *** Basic block 35

	mv          a0, s5
	j           .GenerateAssignment_label_96
.func_end_GenerateAssignment:
	.size GenerateAssignment, .func_end_GenerateAssignment-GenerateAssignment

	.local  GenerateCompoundAssignment
	.type GenerateCompoundAssignment, @function

GenerateCompoundAssignment:

	// *** Basic block 0

	.global GenerateExpression
	.local compound_assignment_ops
	.global printf
	.global abort
	.global TypeIsUnsigned
	.local GetLoadOpcode
	.global IRSetType
	.global GeneratorEmit
	.global NewIR1
	.global CheckForVarUse
	.global IsBitfieldReference
	.local LoadBitfield
	.local FindIROpcode
	.global NewIR2
	.local CalculateNewBitfieldValue
	.local GetStoreOpcode
	.local RemoveUnnecesaryShortening
	.global CheckForVarDef
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
	ld          a1, 64(s2)
	call        GenerateExpression

	// *** Basic block 1

	lw          s3, 0(s2)
	mv          s4, a0
	ld          a2, 56(s2)
	mv          a1, a2
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 2

	mv          s5, a0
	mv          s6, x0
	mv          s7, x0
	la          t0, compound_assignment_ops
	lw          t0, 0(t0)
	beqz        t0, .GenerateCompoundAssignment_label_88

	// *** Basic block 3

.GenerateCompoundAssignment_label_67:
	slli        t0, s7, 3
	la          t1, compound_assignment_ops
	add         t0, t1, t0
	lw          t1, 0(t0)
	bne         t1, s3, .GenerateCompoundAssignment_label_79

	// *** Basic block 4

	lw          s6, 4(t0)
	j           .GenerateCompoundAssignment_label_88

	// *** Basic block 5

.GenerateCompoundAssignment_label_79:

	// *** Basic block 6

.GenerateCompoundAssignment_label_80:
	addi        s7, s7, 1
	slli        t0, s7, 3
	la          t1, compound_assignment_ops
	add         t0, t1, t0
	lw          t0, 0(t0)
	beqz        t0, .GenerateCompoundAssignment_label_67

	// *** Basic block 7

.GenerateCompoundAssignment_label_88:
	beqz        s6, .GenerateCompoundAssignment_label_92

	// *** Basic block 8

	j           .GenerateCompoundAssignment_label_109

	// *** Basic block 9

.GenerateCompoundAssignment_label_92:
	lla         a0, .str.19
	lla         a1, .str.20
	lla         a3, .str.21
	li          t0, 721		// 0x2d1
	mv          a2, t0
	call        printf

	// *** Basic block 10

	call        abort

	// *** Basic block 11

.GenerateCompoundAssignment_label_109:
	li          t0, 14		// 0xe ASCII \xe
	bne         s6, t0, .GenerateCompoundAssignment_label_122

	// *** Basic block 12

	ld          a0, 16(s2)
	call        TypeIsUnsigned

	// *** Basic block 13

	beqz        a0, .GenerateCompoundAssignment_label_121

	// *** Basic block 14

	li          s6, 13		// 0xd ASCII \xd

	// *** Basic block 15

.GenerateCompoundAssignment_label_121:

	// *** Basic block 16

.GenerateCompoundAssignment_label_122:
	mv          a0, s2
	call        GetLoadOpcode

	// *** Basic block 17

	mv          s3, a0
	mv          a1, s5
	mv          a0, s3
	call        NewIR1

	// *** Basic block 18

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 19

	ld          s8, 16(s2)
	mv          a1, s8
	call        IRSetType

	// *** Basic block 20

	mv          s9, a0
	mv          a1, a2
	mv          a0, s9
	call        CheckForVarUse

	// *** Basic block 21

	mv          a0, a2
	call        IsBitfieldReference

	// *** Basic block 22

	beqz        a0, .GenerateCompoundAssignment_label_163

	// *** Basic block 23

	mv          a1, s9
	mv          a0, s1
	call        LoadBitfield

	// *** Basic block 24

	mv          s9, a0

	// *** Basic block 25

.GenerateCompoundAssignment_label_163:
	mv          a1, s6
	mv          a0, s2
	call        FindIROpcode

	// *** Basic block 26

	mv          s3, a0
	mv          a2, s4
	mv          a1, s9
	mv          a0, s3
	call        NewIR2

	// *** Basic block 27

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 28

	mv          a1, s8
	call        IRSetType

	// *** Basic block 29

	mv          s4, a0
	mv          a0, a2
	call        IsBitfieldReference

	// *** Basic block 30

	beqz        a0, .GenerateCompoundAssignment_label_203

	// *** Basic block 31

	mv          a3, a2
	mv          a2, s4
	mv          a1, s9
	mv          a0, s1
	call        CalculateNewBitfieldValue

	// *** Basic block 32

	mv          s4, a0

	// *** Basic block 33

.GenerateCompoundAssignment_label_203:
	mv          a0, s2
	call        GetStoreOpcode

	// *** Basic block 34

	mv          s3, a0
	mv          a2, s3
	mv          a1, s4
	mv          a0, s1
	call        RemoveUnnecesaryShortening

	// *** Basic block 35

	mv          a2, a0
	mv          a1, s5
	mv          a0, s3
	call        NewIR2

	// *** Basic block 36

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 37

	mv          s6, a0
	mv          a1, a2
	mv          a0, s6
	call        CheckForVarDef

	// *** Basic block 38

	mv          a1, s8
	mv          a0, s6
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
	j           IRSetType
.func_end_GenerateCompoundAssignment:
	.size GenerateCompoundAssignment, .func_end_GenerateCompoundAssignment-GenerateCompoundAssignment

	.local  GenerateIndexExpression
	.type GenerateIndexExpression, @function

GenerateIndexExpression:

	// *** Basic block 0

	.global GenerateExpression
	.global GeneratorEmit
	.global NewIR2
	.global IRIsConst
	.global GeneratorGetIntConstant
	.global IRSetType
	.local GetLoadOpcode
	.global NewIR1
	.global CheckForVarUse
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
	ld          s3, 56(s2)
	mv          a1, s3
	call        GenerateExpression

	// *** Basic block 1

	mv          s4, a0
	ld          s5, 64(s2)
	mv          a1, s5
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 2

	mv          s6, a0
	ld          t0, 16(s3)
	ld          s7, 24(t0)
	lw          s9, 16(s7)
	addi        t0, s9, -2
	seqz        s8, t0
	li          t0, 2		// 0x2 ASCII \x2
	beq         s9, t0, .GenerateIndexExpression_label_69

	// *** Basic block 3

	addi        t0, s9, -1
	seqz        s8, t0

	// *** Basic block 4

.GenerateIndexExpression_label_69:
	beqz        s8, .GenerateIndexExpression_label_76

	// *** Basic block 5

	addi        t0, s7, 32
	lb          t0, 16(t0)
	slli        t0, t0, 61
	srai        s8, t0, 63

	// *** Basic block 6

.GenerateIndexExpression_label_76:

	// *** Basic block 7

.GenerateIndexExpression_label_78:
	beqz        s8, .GenerateIndexExpression_label_103

	// *** Basic block 8

	j           .GenerateIndexExpression_label_81

	// *** Basic block 9

.GenerateIndexExpression_label_81:
	addi        t0, s7, 32
	ld          s8, 8(t0)
	mv          a2, s8
	mv          a1, s6
	li          t0, 45		// 0x2d ASCII '-'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 10

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 11

	mv          s8, a0
	j           .GenerateIndexExpression_label_148

	// *** Basic block 12

.GenerateIndexExpression_label_103:
	lw          s9, 20(s7)
	mv          a0, s6
	call        IRIsConst

	// *** Basic block 13

	beqz        a0, .GenerateIndexExpression_label_125

	// *** Basic block 14

	mv          s7, s6
	ld          a1, 16(s5)
	ld          t0, 136(s7)
	mul         a2, t0, s9
	mv          a0, s1
	call        GeneratorGetIntConstant

	// *** Basic block 15

	mv          s8, a0
	j           .GenerateIndexExpression_label_147

	// *** Basic block 16

.GenerateIndexExpression_label_125:
	mv          a2, s9
	mv          a1, x0
	mv          a0, s1
	call        GeneratorGetIntConstant

	// *** Basic block 17

	mv          a2, a0
	mv          a1, s6
	li          t0, 45		// 0x2d ASCII '-'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 18

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 19

	mv          s8, a0

	// *** Basic block 20

.GenerateIndexExpression_label_147:

	// *** Basic block 21

.GenerateIndexExpression_label_148:
	ld          a1, 80(s6)
	mv          a0, s8
	call        IRSetType

	// *** Basic block 22

	mv          a2, s8
	mv          a1, s4
	li          t0, 40		// 0x28 ASCII '('
	mv          a0, t0
	call        NewIR2

	// *** Basic block 23

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 24

	mv          s4, a0
	lw          t0, 8(s2)
	andi        t0, t0, 1
	beqz        t0, .GenerateIndexExpression_label_179

	// *** Basic block 25

	mv          a0, s4

	// *** Basic block 26

.GenerateIndexExpression_label_176:
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

	// *** Basic block 27

.GenerateIndexExpression_label_179:
	mv          a0, s2
	call        GetLoadOpcode

	// *** Basic block 28

	mv          s5, a0
	mv          a1, s4
	mv          a0, s5
	call        NewIR1

	// *** Basic block 29

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 30

	mv          s9, a0
	mv          a1, s3
	mv          a0, s9
	call        CheckForVarUse

	// *** Basic block 31

	ld          a1, 16(s2)
	mv          a0, s9
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
	j           IRSetType
.func_end_GenerateIndexExpression:
	.size GenerateIndexExpression, .func_end_GenerateIndexExpression-GenerateIndexExpression

	.local  GenerateFunctionCall
	.type GenerateFunctionCall, @function

GenerateFunctionCall:

	// *** Basic block 0

	.global GenerateExpression
	.global NewIR1
	.global TypeIsStructOrUnion
	.global IRAddInput
	.global SyntaxNewTemporary
	.global GeneratorGetVariable
	.global GeneratorEmit
	.global IRSetType
	.global NewPointerTo
	.global CheckForVarUse
	sd          a0, -0(s0)	// Spilled @294
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
	sd          s2, -24(s0)	// Spilled @32
	ld          a1, 56(s2)
	call        GenerateExpression

	// *** Basic block 1

	mv          s3, a0
	mv          a1, s3
	li          t0, 92		// 0x5c ASCII '\'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 2

	mv          s4, a0
	sd          s4, -24(s0)	// Spilled @50
	ld          s5, 16(s2)
	mv          a0, s5
	call        TypeIsStructOrUnion

	// *** Basic block 3

	mv          s6, a0
	beqz        s6, .GenerateFunctionCall_label_140

	// *** Basic block 4

	lw          t0, 8(s2)
	andi        t0, t0, 32
	beqz        t0, .GenerateFunctionCall_label_79

	// *** Basic block 5

	ld          a1, 72(s1)
	mv          a2, x0
	mv          a0, s4
	call        IRAddInput

	// *** Basic block 6

	lw          t0, 88(s4)
	ori         t0, t0, 16
	sw          t0, 88(s4)
	j           .GenerateFunctionCall_label_139

	// *** Basic block 7

.GenerateFunctionCall_label_79:
	ld          s3, 80(s1)
	bne         s3, x0, .GenerateFunctionCall_label_130

	// *** Basic block 8

	ld          a0, 0(s1)
	mv          a1, s5
	call        SyntaxNewTemporary

	// *** Basic block 9

	mv          s7, a0
	mv          a1, s7
	mv          a0, s1
	call        GeneratorGetVariable

	// *** Basic block 10

	mv          s8, a0
	mv          a1, s8
	li          t0, 103		// 0x67 ASCII 'g'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 11

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 12

	mv          s9, a0
	mv          a1, s5
	mv          a0, x0
	call        NewPointerTo

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s9
	call        IRSetType

	// *** Basic block 14

	mv          a2, x0
	mv          a1, s9
	mv          a0, s4
	call        IRAddInput

	// *** Basic block 15

	j           .GenerateFunctionCall_label_138

	// *** Basic block 16

.GenerateFunctionCall_label_130:
	mv          a2, x0
	mv          a1, s3
	mv          a0, s4
	call        IRAddInput

	// *** Basic block 17

.GenerateFunctionCall_label_138:

	// *** Basic block 18

.GenerateFunctionCall_label_139:

	// *** Basic block 19

.GenerateFunctionCall_label_140:
	mv          s3, x0
	ld          s10, 64(s2)
	ld          s11, 8(s10)
	bge         x0, s11, .GenerateFunctionCall_label_272

	// *** Basic block 20

	ld          s10, 0(s10)

	// *** Basic block 21

.GenerateFunctionCall_label_150:
	slli        t0, s3, 3
	add         t0, s10, t0
	ld          s10, 0(t0)
	ld          s4, 16(s10)
	mv          a0, s4
	call        TypeIsStructOrUnion

	// *** Basic block 22

	beqz        a0, .GenerateFunctionCall_label_210

	// *** Basic block 23

	mv          a1, s10
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 24

	ld          a0, 80(a0)
	call        TypeIsStructOrUnion

	// *** Basic block 25

	beqz        a0, .GenerateFunctionCall_label_200

	// *** Basic block 26

	mv          a1, a0
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a0, t0
	call        NewIR1

	// *** Basic block 27

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 28

	mv          s2, a0
	mv          a1, s10
	mv          a0, s2
	call        CheckForVarUse

	// *** Basic block 29

	mv          a2, x0
	mv          a1, s2
	ld          a0, -24(s0)	// Spilled @50
	call        IRAddInput

	// *** Basic block 30

	j           .GenerateFunctionCall_label_208

	// *** Basic block 31

.GenerateFunctionCall_label_200:
	mv          a2, x0
	mv          a1, a0
	ld          a0, -24(s0)	// Spilled @50
	call        IRAddInput

	// *** Basic block 32

.GenerateFunctionCall_label_208:
	j           .GenerateFunctionCall_label_267

	// *** Basic block 33

.GenerateFunctionCall_label_210:
	lw          t0, 16(s4)
	addi        t0, t0, -2
	seqz        s4, t0

	// *** Basic block 34

.GenerateFunctionCall_label_218:
	beqz        s4, .GenerateFunctionCall_label_253

	// *** Basic block 35

	j           .GenerateFunctionCall_label_221

	// *** Basic block 36

.GenerateFunctionCall_label_221:
	mv          a1, s10
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 37

	mv          a1, a0
	li          t0, 103		// 0x67 ASCII 'g'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 38

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 39

	mv          s4, a0
	mv          a1, s10
	mv          a0, s4
	call        CheckForVarUse

	// *** Basic block 40

	mv          a2, x0
	mv          a1, s4
	ld          a0, -24(s0)	// Spilled @50
	call        IRAddInput

	// *** Basic block 41

	j           .GenerateFunctionCall_label_266

	// *** Basic block 42

.GenerateFunctionCall_label_253:
	mv          a1, s10
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 43

	mv          a2, x0
	mv          a1, a0
	ld          a0, -24(s0)	// Spilled @50
	call        IRAddInput

	// *** Basic block 44

.GenerateFunctionCall_label_266:

	// *** Basic block 45

.GenerateFunctionCall_label_267:

	// *** Basic block 46

.GenerateFunctionCall_label_268:
	addi        s3, s3, 1
	bge         s3, s11, .GenerateFunctionCall_label_150

	// *** Basic block 47

.GenerateFunctionCall_label_272:
	ld          a1, -24(s0)	// Spilled @50
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 48

	mv          a1, s5
	call        IRSetType

	// *** Basic block 49

	mv          s4, a0
	beqz        s6, .GenerateFunctionCall_label_293

	// *** Basic block 50

	ld          t0, -24(s0)	// Spilled @50
	ld          t1, 24(t0)
	ld          a0, 8(t1)

	// *** Basic block 51

.GenerateFunctionCall_label_290:
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

	// *** Basic block 52

.GenerateFunctionCall_label_293:
	ld          a0, -24(s0)	// Spilled @50
	j           .GenerateFunctionCall_label_290
.func_end_GenerateFunctionCall:
	.size GenerateFunctionCall, .func_end_GenerateFunctionCall-GenerateFunctionCall

	.local  GenerateContentsOf
	.type GenerateContentsOf, @function

GenerateContentsOf:

	// *** Basic block 0

	.global GenerateExpression
	.global IRSetType
	.global TypeIsStructOrUnion
	.local GetLoadOpcode
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
	mv          s2, a1
	ld          a1, 56(s2)
	call        GenerateExpression

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 16(s2)
	mv          a1, s4
	mv          a0, s3
	call        IRSetType

	// *** Basic block 2

	lw          t0, 8(s2)
	andi        t0, t0, 1
	beqz        t0, .GenerateContentsOf_label_47

	// *** Basic block 3

	mv          a0, s3

	// *** Basic block 4

.GenerateContentsOf_label_44:
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

.GenerateContentsOf_label_47:
	mv          a0, s4
	call        TypeIsStructOrUnion

	// *** Basic block 6

	beqz        a0, .GenerateContentsOf_label_55

	// *** Basic block 7

	mv          a0, s3
	j           .GenerateContentsOf_label_44

	// *** Basic block 8

.GenerateContentsOf_label_55:
	lw          t0, 16(s4)
	addi        t0, t0, -2
	seqz        s4, t0

	// *** Basic block 9

.GenerateContentsOf_label_63:
	beqz        s4, .GenerateContentsOf_label_70

	// *** Basic block 10

	j           .GenerateContentsOf_label_66

	// *** Basic block 11

.GenerateContentsOf_label_66:
	mv          a0, s3
	j           .GenerateContentsOf_label_44

	// *** Basic block 12

.GenerateContentsOf_label_70:
	mv          s5, s4
	lw          t0, 16(s5)
	addi        t0, t0, -3
	seqz        s6, t0

	// *** Basic block 13

.GenerateContentsOf_label_79:
	beqz        s6, .GenerateContentsOf_label_86

	// *** Basic block 14

	j           .GenerateContentsOf_label_82

	// *** Basic block 15

.GenerateContentsOf_label_82:
	mv          a0, s3
	j           .GenerateContentsOf_label_44

	// *** Basic block 16

.GenerateContentsOf_label_86:
	mv          a0, s2
	call        GetLoadOpcode

	// *** Basic block 17

	mv          s5, a0
	mv          a1, s3
	mv          a0, s5
	call        NewIR1

	// *** Basic block 18

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 19

	mv          a1, s4
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
	j           IRSetType
.func_end_GenerateContentsOf:
	.size GenerateContentsOf, .func_end_GenerateContentsOf-GenerateContentsOf

	.local  GenerateInlineCall
	.type GenerateInlineCall, @function

GenerateInlineCall:

	// *** Basic block 0

	.global GenerateStatement
	.global GenerateExpression
	.global GeneratorGetIntConstant
	.global NewTypeRecord
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
	ld          a1, 56(s2)
	call        GenerateStatement

	// *** Basic block 1

	ld          s3, 64(s2)
	beq         s3, x0, .GenerateInlineCall_label_40

	// *** Basic block 2

	mv          a1, s3
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GenerateExpression

	// *** Basic block 5

.GenerateInlineCall_label_40:
	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 6

	mv          a2, x0
	mv          a1, a0
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GeneratorGetIntConstant
.func_end_GenerateInlineCall:
	.size GenerateInlineCall, .func_end_GenerateInlineCall-GenerateInlineCall

	.local  GenerateAddressOf
	.type GenerateAddressOf, @function

GenerateAddressOf:

	// *** Basic block 0

	.global GenerateExpression
	.global GeneratorEmit
	.global NewIR1
	.global CheckForVarDef
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
	mv          s2, a1
	ld          a1, 56(s2)
	call        GenerateExpression

	// *** Basic block 1

	mv          s3, a0
	mv          a1, s3
	li          t0, 103		// 0x67 ASCII 'g'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 3

	mv          s4, a0
	mv          a1, s2
	mv          a0, s4
	call        CheckForVarDef

	// *** Basic block 4

	ld          a1, 16(s2)
	mv          a0, s4
	call        IRSetType

	// *** Basic block 5

	mv          a0, s4

	// *** Basic block 6

.GenerateAddressOf_label_52:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GenerateAddressOf:
	.size GenerateAddressOf, .func_end_GenerateAddressOf-GenerateAddressOf

	.local  GenerateMemberReference
	.type GenerateMemberReference, @function

GenerateMemberReference:

	// *** Basic block 0

	.global GenerateExpression
	.global GeneratorEmit
	.global NewIR2
	.global GeneratorGetIntConstant
	.global IRSetType
	.global StructMemberIsBitField
	.local GetLoadOpcode
	.global NewIR1
	.local LoadBitfield
	.global TypeIsStructOrUnion
	.global CheckForVarUse
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
	ld          a1, 56(s2)
	call        GenerateExpression

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 64(s2)
	ld          s4, 56(s4)
	lw          a2, 8(s4)
	mv          a1, x0
	mv          a0, s1
	call        GeneratorGetIntConstant

	// *** Basic block 2

	mv          a2, a0
	mv          a1, s3
	li          t0, 40		// 0x28 ASCII '('
	mv          a0, t0
	call        NewIR2

	// *** Basic block 3

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 4

	mv          s3, a0
	ld          s5, 16(s2)
	mv          a1, s5
	mv          a0, s3
	call        IRSetType

	// *** Basic block 5

	lw          t0, 8(s2)
	andi        t0, t0, 1
	beqz        t0, .GenerateMemberReference_label_81

	// *** Basic block 6

	mv          a0, s3

	// *** Basic block 7

.GenerateMemberReference_label_78:
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

	// *** Basic block 8

.GenerateMemberReference_label_81:
	mv          a0, s4
	call        StructMemberIsBitField

	// *** Basic block 9

	beqz        a0, .GenerateMemberReference_label_119

	// *** Basic block 10

	mv          a0, s2
	call        GetLoadOpcode

	// *** Basic block 11

	mv          s6, a0
	mv          a1, s3
	mv          a0, s6
	call        NewIR1

	// *** Basic block 12

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 13

	mv          a1, s5
	call        IRSetType

	// *** Basic block 14

	mv          s7, a0
	mv          a2, s2
	mv          a1, s7
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
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LoadBitfield

	// *** Basic block 16

.GenerateMemberReference_label_119:
	ld          t0, 0(s4)
	ld          s4, 40(t0)
	lw          t0, 16(s4)
	addi        t0, t0, -2
	seqz        s4, t0

	// *** Basic block 17

.GenerateMemberReference_label_131:
	mv          s8, s4
	bnez        s4, .GenerateMemberReference_label_140

	// *** Basic block 18

	j           .GenerateMemberReference_label_135

	// *** Basic block 19

.GenerateMemberReference_label_135:
	mv          a0, s4
	call        TypeIsStructOrUnion

	// *** Basic block 20

	mv          s8, a0

	// *** Basic block 21

.GenerateMemberReference_label_140:
	beqz        s8, .GenerateMemberReference_label_145

	// *** Basic block 22

	mv          a0, s3
	j           .GenerateMemberReference_label_78

	// *** Basic block 23

.GenerateMemberReference_label_145:
	mv          a0, s2
	call        GetLoadOpcode

	// *** Basic block 24

	mv          s4, a0
	mv          a1, s3
	mv          a0, s4
	call        NewIR1

	// *** Basic block 25

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 26

	mv          s9, a0
	mv          a1, s5
	mv          a0, s9
	call        IRSetType

	// *** Basic block 27

	mv          a1, s2
	mv          a0, s9
	call        CheckForVarUse

	// *** Basic block 28

	mv          a0, s9
	j           .GenerateMemberReference_label_78
.func_end_GenerateMemberReference:
	.size GenerateMemberReference, .func_end_GenerateMemberReference-GenerateMemberReference

	.local  GenerateLogicalOperation
	.type GenerateLogicalOperation, @function

GenerateLogicalOperation:

	// *** Basic block 0

	.global OptLevel1
	.global ASTNodeIsIntConstant
	.global GenerateExpression
	.global OptLevel0
	.global ASTNodeUsesValue
	.global NewIR
	.global GeneratorEmit
	.global NewIR2
	.global IRSetType
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
	call        OptLevel1

	// *** Basic block 1

	mv          s3, a0
	beqz        a0, .GenerateLogicalOperation_label_39

	// *** Basic block 2

	ld          a0, 56(s1)
	call        ASTNodeIsIntConstant

	// *** Basic block 3

	mv          s3, a0

	// *** Basic block 4

.GenerateLogicalOperation_label_39:
	beqz        s3, .GenerateLogicalOperation_label_101

	// *** Basic block 5

	ld          s4, 56(s1)
	lw          t0, 0(s1)
	li          t1, 44		// 0x2c ASCII ','
	bne         t0, t1, .GenerateLogicalOperation_label_76

	// *** Basic block 6

	ld          t0, 56(s4)
	bnez        t0, .GenerateLogicalOperation_label_65

	// *** Basic block 7

	mv          a1, s4
	mv          a0, s2
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
	j           GenerateExpression

	// *** Basic block 9

.GenerateLogicalOperation_label_62:
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

	// *** Basic block 10

.GenerateLogicalOperation_label_65:
	ld          a1, 64(s1)
	mv          a0, s2
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
	j           GenerateExpression

	// *** Basic block 12

.GenerateLogicalOperation_label_76:
	ld          t0, 56(s4)
	beqz        t0, .GenerateLogicalOperation_label_90

	// *** Basic block 13

	mv          a1, s4
	mv          a0, s2
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
	j           GenerateExpression

	// *** Basic block 15

.GenerateLogicalOperation_label_90:
	ld          a1, 64(s1)
	mv          a0, s2
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
	j           GenerateExpression

	// *** Basic block 17

.GenerateLogicalOperation_label_101:
	call        OptLevel0

	// *** Basic block 18

	mv          s4, a0
	bnez        a0, .GenerateLogicalOperation_label_113

	// *** Basic block 19

	ld          a0, 24(s1)
	mv          a1, s1
	call        ASTNodeUsesValue

	// *** Basic block 21

.GenerateLogicalOperation_label_113:
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewIR

	// *** Basic block 22

	mv          s5, a0
	mv          s6, x0
	beqz        s4, .GenerateLogicalOperation_label_134

	// *** Basic block 23

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	call        NewIR

	// *** Basic block 24

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 25

	mv          s6, a0

	// *** Basic block 26

.GenerateLogicalOperation_label_134:
	ld          a1, 56(s1)
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 27

	mv          s7, a0
	beqz        s4, .GenerateLogicalOperation_label_158

	// *** Basic block 28

	mv          a2, s7
	mv          a1, s6
	li          t0, 13		// 0xd ASCII \xd
	mv          a0, t0
	call        NewIR2

	// *** Basic block 29

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 30

.GenerateLogicalOperation_label_158:
	lw          t0, 0(s1)
	li          t1, 44		// 0x2c ASCII ','
	bne         t0, t1, .GenerateLogicalOperation_label_167

	// *** Basic block 31

	li          s8, 89		// 0x59 ASCII 'Y'
	j           .GenerateLogicalOperation_label_169

	// *** Basic block 32

.GenerateLogicalOperation_label_167:
	li          s8, 88		// 0x58 ASCII 'X'

	// *** Basic block 33

.GenerateLogicalOperation_label_169:
	mv          a2, s5
	mv          a1, s7
	mv          a0, s8
	call        NewIR2

	// *** Basic block 34

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 35

	ld          a1, 64(s1)
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 36

	mv          s8, a0
	beqz        s4, .GenerateLogicalOperation_label_204

	// *** Basic block 37

	mv          a2, s8
	mv          a1, s6
	li          t0, 13		// 0xd ASCII \xd
	mv          a0, t0
	call        NewIR2

	// *** Basic block 38

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 39

.GenerateLogicalOperation_label_204:
	mv          a1, s5
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 40

	beq         s6, x0, .GenerateLogicalOperation_label_219

	// *** Basic block 41

	ld          a1, 16(s1)
	mv          a0, s6
	call        IRSetType

	// *** Basic block 42

.GenerateLogicalOperation_label_219:
	beqz        s4, .GenerateLogicalOperation_label_225

	// *** Basic block 43

	mv          a0, s6
	j           .GenerateLogicalOperation_label_227

	// *** Basic block 44

.GenerateLogicalOperation_label_225:
	mv          a0, s8

	// *** Basic block 45

.GenerateLogicalOperation_label_227:
	j           .GenerateLogicalOperation_label_62
.func_end_GenerateLogicalOperation:
	.size GenerateLogicalOperation, .func_end_GenerateLogicalOperation-GenerateLogicalOperation

	.local  GenerateConditionalExpression
	.type GenerateConditionalExpression, @function

GenerateConditionalExpression:

	// *** Basic block 0

	.global OptLevel1
	.global ASTNodeIsIntConstant
	.global GenerateExpression
	.global OptLevel0
	.global ASTNodeUsesValue
	.global NewIR
	.global GeneratorEmit
	.global IRSetType
	.global IRIsComparison
	.global NewIR2
	.global GeneratorGetIntConstant
	.local MoveToTmpOpcode
	.global NewIR1
	.global NewTypeRecord
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
	ld          s3, 64(s1)
	call        OptLevel1

	// *** Basic block 1

	mv          s4, a0
	beqz        a0, .GenerateConditionalExpression_label_47

	// *** Basic block 2

	ld          a0, 56(s1)
	call        ASTNodeIsIntConstant

	// *** Basic block 3

	mv          s4, a0

	// *** Basic block 4

.GenerateConditionalExpression_label_47:
	beqz        s4, .GenerateConditionalExpression_label_79

	// *** Basic block 5

	ld          s5, 56(s1)
	ld          t0, 56(s5)
	beqz        t0, .GenerateConditionalExpression_label_68

	// *** Basic block 6

	ld          a1, 56(s3)
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
	j           GenerateExpression

	// *** Basic block 8

.GenerateConditionalExpression_label_65:
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

	// *** Basic block 9

.GenerateConditionalExpression_label_68:
	ld          a1, 64(s3)
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
	j           GenerateExpression

	// *** Basic block 11

.GenerateConditionalExpression_label_79:
	call        OptLevel0

	// *** Basic block 12

	mv          s5, a0
	bnez        a0, .GenerateConditionalExpression_label_91

	// *** Basic block 13

	ld          a0, 24(s1)
	mv          a1, s1
	call        ASTNodeUsesValue

	// *** Basic block 15

.GenerateConditionalExpression_label_91:
	li          s6, 17		// 0x11 ASCII \x11
	mv          a0, s6
	call        NewIR

	// *** Basic block 16

	mv          s7, a0
	mv          a0, s6
	call        NewIR

	// *** Basic block 17

	mv          s6, a0
	mv          s8, x0
	beqz        s5, .GenerateConditionalExpression_label_124

	// *** Basic block 18

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	call        NewIR

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 20

	mv          s8, a0
	ld          a1, 16(s1)
	mv          a0, s8
	call        IRSetType

	// *** Basic block 21

.GenerateConditionalExpression_label_124:
	ld          s9, 56(s1)
	mv          a1, s9
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 22

	mv          s10, a0
	mv          a0, s10
	call        IRIsComparison

	// *** Basic block 23

	beqz        a0, .GenerateConditionalExpression_label_141

	// *** Basic block 24

	mv          s11, s10
	j           .GenerateConditionalExpression_label_165

	// *** Basic block 25

.GenerateConditionalExpression_label_141:
	ld          a1, 16(s9)
	mv          a2, x0
	mv          a0, s2
	call        GeneratorGetIntConstant

	// *** Basic block 26

	mv          a2, a0
	mv          a1, s10
	li          t0, 65		// 0x41 ASCII 'A'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 27

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 28

	mv          s11, a0

	// *** Basic block 29

.GenerateConditionalExpression_label_165:
	mv          a2, s7
	mv          a1, s11
	li          t0, 89		// 0x59 ASCII 'Y'
	mv          a0, t0
	call        NewIR2
	sd          a0, -24(s0)	// Spilled @173

	// *** Basic block 30

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 31

	ld          s9, 56(s3)
	mv          a1, s9
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 32

	beqz        s5, .GenerateConditionalExpression_label_205

	// *** Basic block 33

	ld          a0, 16(s9)
	call        MoveToTmpOpcode

	// *** Basic block 34

	mv          a2, a0
	mv          a1, s8
	call        NewIR2

	// *** Basic block 35

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 36

.GenerateConditionalExpression_label_205:
	mv          a1, s6
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 37

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 38

	mv          a1, s7
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 39

	ld          s3, 64(s3)
	mv          a1, s3
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 40

	mv          s9, a0
	beqz        s5, .GenerateConditionalExpression_label_248

	// *** Basic block 41

	ld          a0, 16(s3)
	call        MoveToTmpOpcode

	// *** Basic block 42

	mv          a2, s9
	mv          a1, s8
	call        NewIR2

	// *** Basic block 43

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 44

.GenerateConditionalExpression_label_248:
	mv          a1, s6
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 45

	beqz        s5, .GenerateConditionalExpression_label_259

	// *** Basic block 46

	mv          a0, s8
	j           .GenerateConditionalExpression_label_274

	// *** Basic block 47

.GenerateConditionalExpression_label_259:
	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 48

	mv          a2, x0
	mv          a1, a0
	mv          a0, s2
	call        GeneratorGetIntConstant

	// *** Basic block 49


	// *** Basic block 50

.GenerateConditionalExpression_label_274:
	j           .GenerateConditionalExpression_label_65
.func_end_GenerateConditionalExpression:
	.size GenerateConditionalExpression, .func_end_GenerateConditionalExpression-GenerateConditionalExpression

	.local  GenerateBuiltinVaStart
	.type GenerateBuiltinVaStart, @function

GenerateBuiltinVaStart:

	// *** Basic block 0

	.global GenerateExpression
	.global GeneratorEmit
	.global NewIR2
	.global CheckForVarDef
	.global IRSetType
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
	ld          t0, 64(s2)
	ld          s3, 0(t0)
	ld          a1, 0(s3)
	call        GenerateExpression

	// *** Basic block 1

	mv          s4, a0
	ld          a1, 8(s3)
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 2

	mv          s3, a0
	mv          a2, s3
	mv          a1, s4
	li          t0, 128		// 0x80 ASCII \x80
	mv          a0, t0
	call        NewIR2

	// *** Basic block 3

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 4

	mv          s5, a0
	mv          a1, s2
	mv          a0, s5
	call        CheckForVarDef

	// *** Basic block 5

	ld          a1, 16(s2)
	mv          a0, s5
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType
.func_end_GenerateBuiltinVaStart:
	.size GenerateBuiltinVaStart, .func_end_GenerateBuiltinVaStart-GenerateBuiltinVaStart

	.local  GenerateBuiltinVaArg
	.type GenerateBuiltinVaArg, @function

GenerateBuiltinVaArg:

	// *** Basic block 0

	.global GenerateExpression
	.global GeneratorEmit
	.global NewIntIRConstant
	.global NewIR2
	.global CheckForVarDef
	.global IRSetType
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
	ld          t0, 64(s2)
	ld          t0, 0(t0)
	ld          a1, 0(t0)
	call        GenerateExpression

	// *** Basic block 1

	mv          s3, a0
	ld          a0, 16(s2)
	lw          a1, 20(a0)
	call        NewIntIRConstant

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 3

	mv          s4, a0
	mv          a2, s4
	mv          a1, s3
	li          t0, 129		// 0x81 ASCII \x81
	mv          a0, t0
	call        NewIR2

	// *** Basic block 4

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 5

	mv          s5, a0
	mv          a1, s2
	mv          a0, s5
	call        CheckForVarDef

	// *** Basic block 6

	ld          a1, 16(s2)
	mv          a0, s5
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType
.func_end_GenerateBuiltinVaArg:
	.size GenerateBuiltinVaArg, .func_end_GenerateBuiltinVaArg-GenerateBuiltinVaArg

	.local  GenerateBuiltinVaEnd
	.type GenerateBuiltinVaEnd, @function

GenerateBuiltinVaEnd:

	// *** Basic block 0

	.global GenerateExpression
	.global GeneratorEmit
	.global NewIR1
	.global CheckForVarDef
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
	mv          s2, a1
	ld          t0, 64(s2)
	ld          t0, 0(t0)
	ld          a1, 0(t0)
	call        GenerateExpression

	// *** Basic block 1

	mv          s3, a0
	mv          a1, s3
	li          t0, 130		// 0x82 ASCII \x82
	mv          a0, t0
	call        NewIR1

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 3

	mv          s4, a0
	mv          a1, s2
	mv          a0, s4
	call        CheckForVarDef

	// *** Basic block 4

	ld          a1, 16(s2)
	mv          a0, s4
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType
.func_end_GenerateBuiltinVaEnd:
	.size GenerateBuiltinVaEnd, .func_end_GenerateBuiltinVaEnd-GenerateBuiltinVaEnd

	.local  GenerateBuiltinVaCopy
	.type GenerateBuiltinVaCopy, @function

GenerateBuiltinVaCopy:

	// *** Basic block 0

	.global GenerateExpression
	.global GeneratorEmit
	.global NewIR2
	.global CheckForVarDef
	.global IRSetType
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
	ld          t0, 64(s2)
	ld          t0, 0(t0)
	ld          s3, 0(t0)
	mv          a1, s3
	call        GenerateExpression

	// *** Basic block 1

	mv          s4, a0
	mv          a1, s3
	mv          a0, s1
	call        GenerateExpression

	// *** Basic block 2

	mv          s3, a0
	mv          a2, s3
	mv          a1, s4
	li          t0, 131		// 0x83 ASCII \x83
	mv          a0, t0
	call        NewIR2

	// *** Basic block 3

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 4

	mv          s5, a0
	mv          a1, s2
	mv          a0, s5
	call        CheckForVarDef

	// *** Basic block 5

	ld          a1, 16(s2)
	mv          a0, s5
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType
.func_end_GenerateBuiltinVaCopy:
	.size GenerateBuiltinVaCopy, .func_end_GenerateBuiltinVaCopy-GenerateBuiltinVaCopy

	.local  GenerateMask
	.type GenerateMask, @function

GenerateMask:

	// *** Basic block 0

	.global IRIsConst
	.global TypeIsIntegral
	.global printf
	.global abort
	.global GeneratorGetIntConstant
	.global IRSetType
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
	mv          s1, a2
	mv          s2, a1
	mv          s3, a0
	mv          s4, a3
	mv          a0, s1
	call        IRIsConst

	// *** Basic block 1

	beqz        a0, .GenerateMask_label_76

	// *** Basic block 2

	ld          s5, 16(s2)
	mv          a0, s5
	call        TypeIsIntegral

	// *** Basic block 3

	beqz        a0, .GenerateMask_label_43

	// *** Basic block 4

	j           .GenerateMask_label_58

	// *** Basic block 5

.GenerateMask_label_43:
	lla         a0, .str.22
	lla         a1, .str.23
	lla         a3, .str.24
	li          t0, 1118		// 0x45e
	mv          a2, t0
	call        printf

	// *** Basic block 6

	call        abort

	// *** Basic block 7

.GenerateMask_label_58:
	mv          s6, s1
	ld          t0, 136(s6)
	and         a2, t0, s4
	mv          a1, s5
	mv          a0, s3
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
	j           GeneratorGetIntConstant

	// *** Basic block 10

.GenerateMask_label_76:
	ld          s5, 16(s2)
	mv          a2, s4
	mv          a1, s5
	mv          a0, s3
	call        GeneratorGetIntConstant

	// *** Basic block 11

	mv          a2, a0
	mv          a1, s1
	li          t0, 116		// 0x74 ASCII 't'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 12

	mv          a1, a0
	mv          a0, s3
	call        GeneratorEmit

	// *** Basic block 13

	mv          a1, s5
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
	j           IRSetType
.func_end_GenerateMask:
	.size GenerateMask, .func_end_GenerateMask-GenerateMask

	.local  GenerateSignExtend
	.type GenerateSignExtend, @function

GenerateSignExtend:

	// *** Basic block 0

	.global compiler
	.global IRIsConst
	.global GeneratorGetIntConstant
	.global IRSetType
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
	mv          s1, a1
	mv          s2, a0
	mv          s3, a2
	la          t0, compiler
	ld          t0, 0(t0)
	lw          t0, 1080(t0)
	slli        t0, t0, 3
	sub         s4, t0, a3
	bnez        s4, .GenerateSignExtend_label_42

	// *** Basic block 1

	mv          a0, s1

	// *** Basic block 2

.GenerateSignExtend_label_39:
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

.GenerateSignExtend_label_42:
	mv          a0, s1
	call        IRIsConst

	// *** Basic block 4

	beqz        a0, .GenerateSignExtend_label_66

	// *** Basic block 5

	mv          s5, s1
	ld          s6, 136(s5)
	sll         s6, s6, s4
	sra         s6, s6, s4
	ld          a1, 16(s3)
	mv          a2, s6
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
	j           GeneratorGetIntConstant

	// *** Basic block 7

.GenerateSignExtend_label_66:
	ld          s6, 16(s3)
	mv          a2, s4
	mv          a1, s6
	mv          a0, s2
	call        GeneratorGetIntConstant

	// *** Basic block 8

	mv          a2, a0
	mv          a1, s1
	li          t0, 117		// 0x75 ASCII 'u'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 10

	mv          a1, s6
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
	j           IRSetType
.func_end_GenerateSignExtend:
	.size GenerateSignExtend, .func_end_GenerateSignExtend-GenerateSignExtend

	.local  GenerateToInt
	.type GenerateToInt, @function

GenerateToInt:

	// *** Basic block 0

	.global GeneratorEmit
	.global NewIR1
	.global IRSetType
	.global NewIR2
	.global GeneratorGetIntConstant
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
	mv          s3, a4
	mv          a1, a3
	mv          a0, a2
	call        NewIR1

	// *** Basic block 1

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 2

	mv          s4, a0
	ld          s5, 16(s2)
	mv          a2, s3
	mv          a1, s5
	mv          a0, s1
	call        GeneratorGetIntConstant

	// *** Basic block 3

	mv          a2, a0
	mv          a1, s4
	li          t0, 116		// 0x74 ASCII 't'
	mv          a0, t0
	call        NewIR2

	// *** Basic block 4

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 5

	mv          a1, s5
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType
.func_end_GenerateToInt:
	.size GenerateToInt, .func_end_GenerateToInt-GenerateToInt

	.local  ShortenInt
	.type ShortenInt, @function

ShortenInt:

	// *** Basic block 0

	.global TypeIsUnsigned
	.local GenerateMask
	.local GenerateSignExtend
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
	mv          s2, a3
	mv          s3, a0
	mv          s4, a2
	ld          a0, 16(s1)
	call        TypeIsUnsigned

	// *** Basic block 1

	beqz        a0, .ShortenInt_label_45

	// *** Basic block 2

	li          t0, 1		// 0x1 ASCII \x1
	sll         t0, t0, s2
	addi        s5, t0, -1
	mv          a3, s5
	mv          a2, s4
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
	j           GenerateMask

	// *** Basic block 5

.ShortenInt_label_45:
	mv          a3, s2
	mv          a2, s1
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
	j           GenerateSignExtend
.func_end_ShortenInt:
	.size ShortenInt, .func_end_ShortenInt-ShortenInt

	.local  GenerateConversion
	.type GenerateConversion, @function

GenerateConversion:

	// *** Basic block 0

	.local ShortenInt
	.global IRSetType
	.global GeneratorEmit
	.global NewIR1
	.local GenerateToInt
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
	lw          t0, 0(s1)
	li          t1, 91		// 0x5b ASCII '['
	blt         t0, t1, .GenerateConversion_label_518

	// *** Basic block 1

	li          t1, 162		// 0xa2 ASCII \xa2
	blt         t1, t0, .GenerateConversion_label_518

	// *** Basic block 2

	addi        t0, t0, -91
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .GenerateConversion_label_121

	// *** Basic block 4

	j           .GenerateConversion_label_141

	// *** Basic block 5

	j           .GenerateConversion_label_163

	// *** Basic block 6

	j           .GenerateConversion_label_164

	// *** Basic block 7

	j           .GenerateConversion_label_168

	// *** Basic block 8

	j           .GenerateConversion_label_194

	// *** Basic block 9

	j           .GenerateConversion_label_195

	// *** Basic block 10

	j           .GenerateConversion_label_142

	// *** Basic block 11

	j           .GenerateConversion_label_225

	// *** Basic block 12

	j           .GenerateConversion_label_226

	// *** Basic block 13

	j           .GenerateConversion_label_227

	// *** Basic block 14

	j           .GenerateConversion_label_228

	// *** Basic block 15

	j           .GenerateConversion_label_169

	// *** Basic block 16

	j           .GenerateConversion_label_196

	// *** Basic block 17

	j           .GenerateConversion_label_197

	// *** Basic block 18

	j           .GenerateConversion_label_229

	// *** Basic block 19

	j           .GenerateConversion_label_232

	// *** Basic block 20

	j           .GenerateConversion_label_143

	// *** Basic block 21

	j           .GenerateConversion_label_233

	// *** Basic block 22

	j           .GenerateConversion_label_234

	// *** Basic block 23

	j           .GenerateConversion_label_170

	// *** Basic block 24

	j           .GenerateConversion_label_198

	// *** Basic block 25

	j           .GenerateConversion_label_199

	// *** Basic block 26

	j           .GenerateConversion_label_144

	// *** Basic block 27

	j           .GenerateConversion_label_238

	// *** Basic block 28

	j           .GenerateConversion_label_145

	// *** Basic block 29

	j           .GenerateConversion_label_122

	// *** Basic block 30

	j           .GenerateConversion_label_230

	// *** Basic block 31

	j           .GenerateConversion_label_171

	// *** Basic block 32

	j           .GenerateConversion_label_200

	// *** Basic block 33

	j           .GenerateConversion_label_201

	// *** Basic block 34

	j           .GenerateConversion_label_146

	// *** Basic block 35

	j           .GenerateConversion_label_239

	// *** Basic block 36

	j           .GenerateConversion_label_147

	// *** Basic block 37

	j           .GenerateConversion_label_123

	// *** Basic block 38

	j           .GenerateConversion_label_231

	// *** Basic block 39

	j           .GenerateConversion_label_172

	// *** Basic block 40

	j           .GenerateConversion_label_202

	// *** Basic block 41

	j           .GenerateConversion_label_203

	// *** Basic block 42

	j           .GenerateConversion_label_148

	// *** Basic block 43

	j           .GenerateConversion_label_254

	// *** Basic block 44

	j           .GenerateConversion_label_338

	// *** Basic block 45

	j           .GenerateConversion_label_273

	// *** Basic block 46

	j           .GenerateConversion_label_291

	// *** Basic block 47

	j           .GenerateConversion_label_292

	// *** Basic block 48

	j           .GenerateConversion_label_314

	// *** Basic block 49

	j           .GenerateConversion_label_315

	// *** Basic block 50

	j           .GenerateConversion_label_337

	// *** Basic block 51

	j           .GenerateConversion_label_356

	// *** Basic block 52

	j           .GenerateConversion_label_375

	// *** Basic block 53

	j           .GenerateConversion_label_396

	// *** Basic block 54

	j           .GenerateConversion_label_415

	// *** Basic block 55

	j           .GenerateConversion_label_416

	// *** Basic block 56

	j           .GenerateConversion_label_440

	// *** Basic block 57

	j           .GenerateConversion_label_463

	// *** Basic block 58

	j           .GenerateConversion_label_376

	// *** Basic block 59

	j           .GenerateConversion_label_357

	// *** Basic block 60

	j           .GenerateConversion_label_377

	// *** Basic block 61

	j           .GenerateConversion_label_397

	// *** Basic block 62

	j           .GenerateConversion_label_417

	// *** Basic block 63

	j           .GenerateConversion_label_418

	// *** Basic block 64

	j           .GenerateConversion_label_441

	// *** Basic block 65

	j           .GenerateConversion_label_464

	// *** Basic block 66

	j           .GenerateConversion_label_378

	// *** Basic block 67

	j           .GenerateConversion_label_469

	// *** Basic block 68

	j           .GenerateConversion_label_465

	// *** Basic block 69

	j           .GenerateConversion_label_466

	// *** Basic block 70

	j           .GenerateConversion_label_467

	// *** Basic block 71

	j           .GenerateConversion_label_468

	// *** Basic block 72

	j           .GenerateConversion_label_473

	// *** Basic block 73

	j           .GenerateConversion_label_495

	// *** Basic block 74

	j           .GenerateConversion_label_496

	// *** Basic block 75

.GenerateConversion_label_121:

	// *** Basic block 76

.GenerateConversion_label_122:

	// *** Basic block 77

.GenerateConversion_label_123:
	li          t0, 16		// 0x10 ASCII \x10
	mv          a3, t0
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ShortenInt

	// *** Basic block 79

.GenerateConversion_label_138:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 80

.GenerateConversion_label_141:

	// *** Basic block 81

.GenerateConversion_label_142:

	// *** Basic block 82

.GenerateConversion_label_143:

	// *** Basic block 83

.GenerateConversion_label_144:

	// *** Basic block 84

.GenerateConversion_label_145:

	// *** Basic block 85

.GenerateConversion_label_146:

	// *** Basic block 86

.GenerateConversion_label_147:

	// *** Basic block 87

.GenerateConversion_label_148:
	li          t0, 8		// 0x8 ASCII \x8
	mv          a3, t0
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ShortenInt

	// *** Basic block 89

.GenerateConversion_label_163:

	// *** Basic block 90

.GenerateConversion_label_164:
	mv          a0, s3
	j           .GenerateConversion_label_138

	// *** Basic block 91

.GenerateConversion_label_168:

	// *** Basic block 92

.GenerateConversion_label_169:

	// *** Basic block 93

.GenerateConversion_label_170:

	// *** Basic block 94

.GenerateConversion_label_171:

	// *** Basic block 95

.GenerateConversion_label_172:
	mv          a1, s3
	li          t0, 110		// 0x6e ASCII 'n'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 96

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 97

	ld          a1, 16(s1)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType

	// *** Basic block 99

.GenerateConversion_label_194:

	// *** Basic block 100

.GenerateConversion_label_195:

	// *** Basic block 101

.GenerateConversion_label_196:

	// *** Basic block 102

.GenerateConversion_label_197:

	// *** Basic block 103

.GenerateConversion_label_198:

	// *** Basic block 104

.GenerateConversion_label_199:

	// *** Basic block 105

.GenerateConversion_label_200:

	// *** Basic block 106

.GenerateConversion_label_201:

	// *** Basic block 107

.GenerateConversion_label_202:

	// *** Basic block 108

.GenerateConversion_label_203:
	mv          a1, s3
	li          t0, 111		// 0x6f ASCII 'o'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 109

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 110

	ld          a1, 16(s1)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType

	// *** Basic block 112

.GenerateConversion_label_225:

	// *** Basic block 113

.GenerateConversion_label_226:

	// *** Basic block 114

.GenerateConversion_label_227:

	// *** Basic block 115

.GenerateConversion_label_228:

	// *** Basic block 116

.GenerateConversion_label_229:

	// *** Basic block 117

.GenerateConversion_label_230:

	// *** Basic block 118

.GenerateConversion_label_231:

	// *** Basic block 119

.GenerateConversion_label_232:

	// *** Basic block 120

.GenerateConversion_label_233:

	// *** Basic block 121

.GenerateConversion_label_234:
	mv          a0, s3
	j           .GenerateConversion_label_138

	// *** Basic block 122

.GenerateConversion_label_238:

	// *** Basic block 123

.GenerateConversion_label_239:
	li          t0, 32		// 0x20 ASCII ' '
	mv          a3, t0
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ShortenInt

	// *** Basic block 125

.GenerateConversion_label_254:
	li          t0, 65535		// 0xffff
	mv          a4, t0
	mv          a3, s3
	li          t0, 114		// 0x72 ASCII 'r'
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GenerateToInt

	// *** Basic block 127

.GenerateConversion_label_273:
	li          t0, 4294967295		// 0xffffffff
	mv          a4, t0
	mv          a3, s3
	li          t0, 114		// 0x72 ASCII 'r'
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GenerateToInt

	// *** Basic block 129

.GenerateConversion_label_291:

	// *** Basic block 130

.GenerateConversion_label_292:
	mv          a1, s3
	li          t0, 114		// 0x72 ASCII 'r'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 131

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 132

	ld          a1, 16(s1)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType

	// *** Basic block 134

.GenerateConversion_label_314:

	// *** Basic block 135

.GenerateConversion_label_315:
	mv          a1, s3
	li          t0, 112		// 0x70 ASCII 'p'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 136

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 137

	ld          a1, 16(s1)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType

	// *** Basic block 139

.GenerateConversion_label_337:

	// *** Basic block 140

.GenerateConversion_label_338:
	li          t0, 255		// 0xff
	mv          a4, t0
	mv          a3, s3
	li          t0, 114		// 0x72 ASCII 'r'
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GenerateToInt

	// *** Basic block 142

.GenerateConversion_label_356:

	// *** Basic block 143

.GenerateConversion_label_357:
	li          t0, 4294967295		// 0xffffffff
	mv          a4, t0
	mv          a3, s3
	li          t0, 115		// 0x73 ASCII 's'
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GenerateToInt

	// *** Basic block 145

.GenerateConversion_label_375:

	// *** Basic block 146

.GenerateConversion_label_376:

	// *** Basic block 147

.GenerateConversion_label_377:

	// *** Basic block 148

.GenerateConversion_label_378:
	li          t0, 255		// 0xff
	mv          a4, t0
	mv          a3, s3
	li          t0, 115		// 0x73 ASCII 's'
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GenerateToInt

	// *** Basic block 150

.GenerateConversion_label_396:

	// *** Basic block 151

.GenerateConversion_label_397:
	li          t0, 1048575		// 0xfffff
	mv          a4, t0
	mv          a3, s3
	li          t0, 115		// 0x73 ASCII 's'
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GenerateToInt

	// *** Basic block 153

.GenerateConversion_label_415:

	// *** Basic block 154

.GenerateConversion_label_416:

	// *** Basic block 155

.GenerateConversion_label_417:

	// *** Basic block 156

.GenerateConversion_label_418:
	mv          a1, s3
	li          t0, 115		// 0x73 ASCII 's'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 157

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 158

	ld          a1, 16(s1)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType

	// *** Basic block 160

.GenerateConversion_label_440:

	// *** Basic block 161

.GenerateConversion_label_441:
	mv          a1, s3
	li          t0, 113		// 0x71 ASCII 'q'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 162

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 163

	ld          a1, 16(s1)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType

	// *** Basic block 165

.GenerateConversion_label_463:

	// *** Basic block 166

.GenerateConversion_label_464:

	// *** Basic block 167

.GenerateConversion_label_465:

	// *** Basic block 168

.GenerateConversion_label_466:

	// *** Basic block 169

.GenerateConversion_label_467:

	// *** Basic block 170

.GenerateConversion_label_468:

	// *** Basic block 171

.GenerateConversion_label_469:
	mv          a0, s3
	j           .GenerateConversion_label_138

	// *** Basic block 172

.GenerateConversion_label_473:
	mv          a1, s3
	li          t0, 110		// 0x6e ASCII 'n'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 173

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 174

	ld          a1, 16(s1)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType

	// *** Basic block 176

.GenerateConversion_label_495:

	// *** Basic block 177

.GenerateConversion_label_496:
	mv          a1, s3
	li          t0, 111		// 0x6f ASCII 'o'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 178

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 179

	ld          a1, 16(s1)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType

	// *** Basic block 181

.GenerateConversion_label_518:
	mv          a0, s3
	j           .GenerateConversion_label_138
.func_end_GenerateConversion:
	.size GenerateConversion, .func_end_GenerateConversion-GenerateConversion

	.local  GenerateAsm
	.type GenerateAsm, @function

GenerateAsm:

	// *** Basic block 0

	.global CompilerAddStringLiteral
	.global IRSetType
	.global GeneratorEmit
	.global NewIR1
	.global GeneratorGetIntConstant
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
	ld          a0, 56(s1)
	call        CompilerAddStringLiteral

	// *** Basic block 1

	mv          s3, a0
	mv          a2, s3
	mv          a1, x0
	mv          a0, s2
	call        GeneratorGetIntConstant

	// *** Basic block 2

	mv          a1, a0
	li          t0, 123		// 0x7b ASCII '{'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 3

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 4

	ld          a1, 16(s1)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRSetType
.func_end_GenerateAsm:
	.size GenerateAsm, .func_end_GenerateAsm-GenerateAsm

	.global GenerateExpression
	.type GenerateExpression, @function

GenerateExpression:

	// *** Basic block 0

	.global IRSetLocation
	.global GeneratorGetIntConstant
	.global GeneratorEmitConstant
	.global NewFloatingPointIRConstant
	.local GenerateLiteral
	.local GenerateVariableReference
	.local GenerateBinaryExpression
	.global GenerateExpression
	.local GenerateConversion
	.global GeneratorEmit
	.global NewIR1
	.global IRSetType
	.local GeneratePointerScale
	.local GenerateIncDec
	.local GenerateInitialization
	.global NewTypeRecord
	.local GenerateAssignment
	.local GenerateCompoundAssignment
	.local GenerateUnaryExpression
	.local GenerateIndexExpression
	.local GenerateFunctionCall
	.local GenerateInlineCall
	.local GenerateContentsOf
	.local GenerateAddressOf
	.local GenerateMemberReference
	.global printf
	.global abort
	.local GenerateLogicalOperation
	.local GenerateConditionalExpression
	.local GenerateBuiltinVaStart
	.local GenerateBuiltinVaArg
	.local GenerateBuiltinVaEnd
	.local GenerateBuiltinVaCopy
	.local GenerateAsm
	.global fprintf
	.global stderr
	.global ASTOpcodeName
	sd          a0, -0(s0)	// Spilled @856
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -16(s0)
	// Spilled register region: 24 bytes at -40(s0) to -16(s0)
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
	sd          s1, -24(s0)	// Spilled @70
	mv          s2, a0
	mv          s3, s1
	mv          s4, s1
	sd          s4, -32(s0)	// Spilled @78
	mv          s5, s1
	mv          s6, s1
	mv          s7, s1
	mv          s8, s1
	mv          s9, s1
	ld          a0, 40(s1)
	call        IRSetLocation

	// *** Basic block 1

	mv          s10, x0
	lw          s11, 0(s1)
	li          a0, 1		// 0x1 ASCII \x1
	blt         s11, a0, .GenerateExpression_label_801

	// *** Basic block 2

	li          t0, 162		// 0xa2 ASCII \xa2
	blt         t0, s11, .GenerateExpression_label_801

	// *** Basic block 3

	addi        t0, s11, -1
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 4

	j           .GenerateExpression_label_274

	// *** Basic block 5

	j           .GenerateExpression_label_314

	// *** Basic block 6

	j           .GenerateExpression_label_305

	// *** Basic block 7

	j           .GenerateExpression_label_306

	// *** Basic block 8

	j           .GenerateExpression_label_275

	// *** Basic block 9

	j           .GenerateExpression_label_289

	// *** Basic block 10

	j           .GenerateExpression_label_500

	// *** Basic block 11

	j           .GenerateExpression_label_514

	// *** Basic block 12

	j           .GenerateExpression_label_584

	// *** Basic block 13

	j           .GenerateExpression_label_583

	// *** Basic block 14

	j           .GenerateExpression_label_618

	// *** Basic block 15

	j           .GenerateExpression_label_626

	// *** Basic block 16

	j           .GenerateExpression_label_329

	// *** Basic block 17

	j           .GenerateExpression_label_328

	// *** Basic block 18

	j           .GenerateExpression_label_572

	// *** Basic block 19

	j           .GenerateExpression_label_571

	// *** Basic block 20

	j           .GenerateExpression_label_330

	// *** Basic block 21

	j           .GenerateExpression_label_573

	// *** Basic block 22

	j           .GenerateExpression_label_635

	// *** Basic block 23

	j           .GenerateExpression_label_557

	// *** Basic block 24

	j           .GenerateExpression_label_585

	// *** Basic block 25

	j           .GenerateExpression_label_331

	// *** Basic block 26

	j           .GenerateExpression_label_801

	// *** Basic block 27

	j           .GenerateExpression_label_332

	// *** Basic block 28

	j           .GenerateExpression_label_575

	// *** Basic block 29

	j           .GenerateExpression_label_801

	// *** Basic block 30

	j           .GenerateExpression_label_276

	// *** Basic block 31

	j           .GenerateExpression_label_801

	// *** Basic block 32

	j           .GenerateExpression_label_727

	// *** Basic block 33

	j           .GenerateExpression_label_801

	// *** Basic block 34

	j           .GenerateExpression_label_801

	// *** Basic block 35

	j           .GenerateExpression_label_801

	// *** Basic block 36

	j           .GenerateExpression_label_634

	// *** Basic block 37

	j           .GenerateExpression_label_337

	// *** Basic block 38

	j           .GenerateExpression_label_801

	// *** Basic block 39

	j           .GenerateExpression_label_801

	// *** Basic block 40

	j           .GenerateExpression_label_335

	// *** Basic block 41

	j           .GenerateExpression_label_336

	// *** Basic block 42

	j           .GenerateExpression_label_801

	// *** Basic block 43

	j           .GenerateExpression_label_801

	// *** Basic block 44

	j           .GenerateExpression_label_801

	// *** Basic block 45

	j           .GenerateExpression_label_333

	// *** Basic block 46

	j           .GenerateExpression_label_334

	// *** Basic block 47

	j           .GenerateExpression_label_710

	// *** Basic block 48

	j           .GenerateExpression_label_711

	// *** Basic block 49

	j           .GenerateExpression_label_602

	// *** Basic block 50

	j           .GenerateExpression_label_610

	// *** Basic block 51

	j           .GenerateExpression_label_327

	// *** Basic block 52

	j           .GenerateExpression_label_570

	// *** Basic block 53

	j           .GenerateExpression_label_594

	// *** Basic block 54

	j           .GenerateExpression_label_323

	// *** Basic block 55

	j           .GenerateExpression_label_566

	// *** Basic block 56

	j           .GenerateExpression_label_488

	// *** Basic block 57

	j           .GenerateExpression_label_338

	// *** Basic block 58

	j           .GenerateExpression_label_574

	// *** Basic block 59

	j           .GenerateExpression_label_326

	// *** Basic block 60

	j           .GenerateExpression_label_569

	// *** Basic block 61

	j           .GenerateExpression_label_322

	// *** Basic block 62

	j           .GenerateExpression_label_565

	// *** Basic block 63

	j           .GenerateExpression_label_474

	// *** Basic block 64

	j           .GenerateExpression_label_719

	// *** Basic block 65

	j           .GenerateExpression_label_801

	// *** Basic block 66

	j           .GenerateExpression_label_801

	// *** Basic block 67

	j           .GenerateExpression_label_801

	// *** Basic block 68

	j           .GenerateExpression_label_643

	// *** Basic block 69

	j           .GenerateExpression_label_325

	// *** Basic block 70

	j           .GenerateExpression_label_568

	// *** Basic block 71

	j           .GenerateExpression_label_324

	// *** Basic block 72

	j           .GenerateExpression_label_567

	// *** Basic block 73

	j           .GenerateExpression_label_801

	// *** Basic block 74

	j           .GenerateExpression_label_586

	// *** Basic block 75

	j           .GenerateExpression_label_801

	// *** Basic block 76

	j           .GenerateExpression_label_801

	// *** Basic block 77

	j           .GenerateExpression_label_793

	// *** Basic block 78

	j           .GenerateExpression_label_801

	// *** Basic block 79

	j           .GenerateExpression_label_761

	// *** Basic block 80

	j           .GenerateExpression_label_769

	// *** Basic block 81

	j           .GenerateExpression_label_777

	// *** Basic block 82

	j           .GenerateExpression_label_785

	// *** Basic block 83

	j           .GenerateExpression_label_435

	// *** Basic block 84

	j           .GenerateExpression_label_801

	// *** Basic block 85

	j           .GenerateExpression_label_801

	// *** Basic block 86

	j           .GenerateExpression_label_801

	// *** Basic block 87

	j           .GenerateExpression_label_801

	// *** Basic block 88

	j           .GenerateExpression_label_801

	// *** Basic block 89

	j           .GenerateExpression_label_527

	// *** Basic block 90

	j           .GenerateExpression_label_743

	// *** Basic block 91

	j           .GenerateExpression_label_759

	// *** Basic block 92

	j           .GenerateExpression_label_742

	// *** Basic block 93

	j           .GenerateExpression_label_466

	// *** Basic block 94

	j           .GenerateExpression_label_346

	// *** Basic block 95

	j           .GenerateExpression_label_347

	// *** Basic block 96

	j           .GenerateExpression_label_348

	// *** Basic block 97

	j           .GenerateExpression_label_349

	// *** Basic block 98

	j           .GenerateExpression_label_350

	// *** Basic block 99

	j           .GenerateExpression_label_351

	// *** Basic block 100

	j           .GenerateExpression_label_352

	// *** Basic block 101

	j           .GenerateExpression_label_353

	// *** Basic block 102

	j           .GenerateExpression_label_354

	// *** Basic block 103

	j           .GenerateExpression_label_355

	// *** Basic block 104

	j           .GenerateExpression_label_356

	// *** Basic block 105

	j           .GenerateExpression_label_357

	// *** Basic block 106

	j           .GenerateExpression_label_358

	// *** Basic block 107

	j           .GenerateExpression_label_359

	// *** Basic block 108

	j           .GenerateExpression_label_360

	// *** Basic block 109

	j           .GenerateExpression_label_361

	// *** Basic block 110

	j           .GenerateExpression_label_362

	// *** Basic block 111

	j           .GenerateExpression_label_363

	// *** Basic block 112

	j           .GenerateExpression_label_364

	// *** Basic block 113

	j           .GenerateExpression_label_365

	// *** Basic block 114

	j           .GenerateExpression_label_366

	// *** Basic block 115

	j           .GenerateExpression_label_367

	// *** Basic block 116

	j           .GenerateExpression_label_368

	// *** Basic block 117

	j           .GenerateExpression_label_369

	// *** Basic block 118

	j           .GenerateExpression_label_370

	// *** Basic block 119

	j           .GenerateExpression_label_371

	// *** Basic block 120

	j           .GenerateExpression_label_372

	// *** Basic block 121

	j           .GenerateExpression_label_373

	// *** Basic block 122

	j           .GenerateExpression_label_374

	// *** Basic block 123

	j           .GenerateExpression_label_375

	// *** Basic block 124

	j           .GenerateExpression_label_376

	// *** Basic block 125

	j           .GenerateExpression_label_377

	// *** Basic block 126

	j           .GenerateExpression_label_378

	// *** Basic block 127

	j           .GenerateExpression_label_379

	// *** Basic block 128

	j           .GenerateExpression_label_380

	// *** Basic block 129

	j           .GenerateExpression_label_381

	// *** Basic block 130

	j           .GenerateExpression_label_382

	// *** Basic block 131

	j           .GenerateExpression_label_383

	// *** Basic block 132

	j           .GenerateExpression_label_384

	// *** Basic block 133

	j           .GenerateExpression_label_385

	// *** Basic block 134

	j           .GenerateExpression_label_386

	// *** Basic block 135

	j           .GenerateExpression_label_387

	// *** Basic block 136

	j           .GenerateExpression_label_388

	// *** Basic block 137

	j           .GenerateExpression_label_389

	// *** Basic block 138

	j           .GenerateExpression_label_390

	// *** Basic block 139

	j           .GenerateExpression_label_391

	// *** Basic block 140

	j           .GenerateExpression_label_392

	// *** Basic block 141

	j           .GenerateExpression_label_393

	// *** Basic block 142

	j           .GenerateExpression_label_394

	// *** Basic block 143

	j           .GenerateExpression_label_395

	// *** Basic block 144

	j           .GenerateExpression_label_396

	// *** Basic block 145

	j           .GenerateExpression_label_397

	// *** Basic block 146

	j           .GenerateExpression_label_398

	// *** Basic block 147

	j           .GenerateExpression_label_399

	// *** Basic block 148

	j           .GenerateExpression_label_400

	// *** Basic block 149

	j           .GenerateExpression_label_401

	// *** Basic block 150

	j           .GenerateExpression_label_402

	// *** Basic block 151

	j           .GenerateExpression_label_403

	// *** Basic block 152

	j           .GenerateExpression_label_404

	// *** Basic block 153

	j           .GenerateExpression_label_405

	// *** Basic block 154

	j           .GenerateExpression_label_406

	// *** Basic block 155

	j           .GenerateExpression_label_407

	// *** Basic block 156

	j           .GenerateExpression_label_408

	// *** Basic block 157

	j           .GenerateExpression_label_409

	// *** Basic block 158

	j           .GenerateExpression_label_410

	// *** Basic block 159

	j           .GenerateExpression_label_411

	// *** Basic block 160

	j           .GenerateExpression_label_412

	// *** Basic block 161

	j           .GenerateExpression_label_413

	// *** Basic block 162

	j           .GenerateExpression_label_414

	// *** Basic block 163

	j           .GenerateExpression_label_415

	// *** Basic block 164

	j           .GenerateExpression_label_416

	// *** Basic block 165

	j           .GenerateExpression_label_417

	// *** Basic block 166

.GenerateExpression_label_274:

	// *** Basic block 167

.GenerateExpression_label_275:

	// *** Basic block 168

.GenerateExpression_label_276:
	ld          a1, 16(s1)
	ld          a2, 56(s3)
	mv          a0, s2
	call        GeneratorGetIntConstant

	// *** Basic block 169

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 170

.GenerateExpression_label_289:
	ld          a0, 16(s1)
	fld         fa0, 56(s3)
	call        NewFloatingPointIRConstant

	// *** Basic block 171

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmitConstant

	// *** Basic block 172

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 173

.GenerateExpression_label_305:

	// *** Basic block 174

.GenerateExpression_label_306:
	mv          a1, s3
	mv          a0, s2
	call        GenerateLiteral

	// *** Basic block 175

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 176

.GenerateExpression_label_314:
	mv          a1, s4
	mv          a0, s2
	call        GenerateVariableReference

	// *** Basic block 177

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 178

.GenerateExpression_label_322:

	// *** Basic block 179

.GenerateExpression_label_323:

	// *** Basic block 180

.GenerateExpression_label_324:

	// *** Basic block 181

.GenerateExpression_label_325:

	// *** Basic block 182

.GenerateExpression_label_326:

	// *** Basic block 183

.GenerateExpression_label_327:

	// *** Basic block 184

.GenerateExpression_label_328:

	// *** Basic block 185

.GenerateExpression_label_329:

	// *** Basic block 186

.GenerateExpression_label_330:

	// *** Basic block 187

.GenerateExpression_label_331:

	// *** Basic block 188

.GenerateExpression_label_332:

	// *** Basic block 189

.GenerateExpression_label_333:

	// *** Basic block 190

.GenerateExpression_label_334:

	// *** Basic block 191

.GenerateExpression_label_335:

	// *** Basic block 192

.GenerateExpression_label_336:

	// *** Basic block 193

.GenerateExpression_label_337:

	// *** Basic block 194

.GenerateExpression_label_338:
	mv          a1, s5
	mv          a0, s2
	call        GenerateBinaryExpression

	// *** Basic block 195

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 196

.GenerateExpression_label_346:

	// *** Basic block 197

.GenerateExpression_label_347:

	// *** Basic block 198

.GenerateExpression_label_348:

	// *** Basic block 199

.GenerateExpression_label_349:

	// *** Basic block 200

.GenerateExpression_label_350:

	// *** Basic block 201

.GenerateExpression_label_351:

	// *** Basic block 202

.GenerateExpression_label_352:

	// *** Basic block 203

.GenerateExpression_label_353:

	// *** Basic block 204

.GenerateExpression_label_354:

	// *** Basic block 205

.GenerateExpression_label_355:

	// *** Basic block 206

.GenerateExpression_label_356:

	// *** Basic block 207

.GenerateExpression_label_357:

	// *** Basic block 208

.GenerateExpression_label_358:

	// *** Basic block 209

.GenerateExpression_label_359:

	// *** Basic block 210

.GenerateExpression_label_360:

	// *** Basic block 211

.GenerateExpression_label_361:

	// *** Basic block 212

.GenerateExpression_label_362:

	// *** Basic block 213

.GenerateExpression_label_363:

	// *** Basic block 214

.GenerateExpression_label_364:

	// *** Basic block 215

.GenerateExpression_label_365:

	// *** Basic block 216

.GenerateExpression_label_366:

	// *** Basic block 217

.GenerateExpression_label_367:

	// *** Basic block 218

.GenerateExpression_label_368:

	// *** Basic block 219

.GenerateExpression_label_369:

	// *** Basic block 220

.GenerateExpression_label_370:

	// *** Basic block 221

.GenerateExpression_label_371:

	// *** Basic block 222

.GenerateExpression_label_372:

	// *** Basic block 223

.GenerateExpression_label_373:

	// *** Basic block 224

.GenerateExpression_label_374:

	// *** Basic block 225

.GenerateExpression_label_375:

	// *** Basic block 226

.GenerateExpression_label_376:

	// *** Basic block 227

.GenerateExpression_label_377:

	// *** Basic block 228

.GenerateExpression_label_378:

	// *** Basic block 229

.GenerateExpression_label_379:

	// *** Basic block 230

.GenerateExpression_label_380:

	// *** Basic block 231

.GenerateExpression_label_381:

	// *** Basic block 232

.GenerateExpression_label_382:

	// *** Basic block 233

.GenerateExpression_label_383:

	// *** Basic block 234

.GenerateExpression_label_384:

	// *** Basic block 235

.GenerateExpression_label_385:

	// *** Basic block 236

.GenerateExpression_label_386:

	// *** Basic block 237

.GenerateExpression_label_387:

	// *** Basic block 238

.GenerateExpression_label_388:

	// *** Basic block 239

.GenerateExpression_label_389:

	// *** Basic block 240

.GenerateExpression_label_390:

	// *** Basic block 241

.GenerateExpression_label_391:

	// *** Basic block 242

.GenerateExpression_label_392:

	// *** Basic block 243

.GenerateExpression_label_393:

	// *** Basic block 244

.GenerateExpression_label_394:

	// *** Basic block 245

.GenerateExpression_label_395:

	// *** Basic block 246

.GenerateExpression_label_396:

	// *** Basic block 247

.GenerateExpression_label_397:

	// *** Basic block 248

.GenerateExpression_label_398:

	// *** Basic block 249

.GenerateExpression_label_399:

	// *** Basic block 250

.GenerateExpression_label_400:

	// *** Basic block 251

.GenerateExpression_label_401:

	// *** Basic block 252

.GenerateExpression_label_402:

	// *** Basic block 253

.GenerateExpression_label_403:

	// *** Basic block 254

.GenerateExpression_label_404:

	// *** Basic block 255

.GenerateExpression_label_405:

	// *** Basic block 256

.GenerateExpression_label_406:

	// *** Basic block 257

.GenerateExpression_label_407:

	// *** Basic block 258

.GenerateExpression_label_408:

	// *** Basic block 259

.GenerateExpression_label_409:

	// *** Basic block 260

.GenerateExpression_label_410:

	// *** Basic block 261

.GenerateExpression_label_411:

	// *** Basic block 262

.GenerateExpression_label_412:

	// *** Basic block 263

.GenerateExpression_label_413:

	// *** Basic block 264

.GenerateExpression_label_414:

	// *** Basic block 265

.GenerateExpression_label_415:

	// *** Basic block 266

.GenerateExpression_label_416:

	// *** Basic block 267

.GenerateExpression_label_417:
	ld          a1, 56(s6)
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 268

	mv          s4, a0
	mv          a2, s4
	mv          a1, s6
	mv          a0, s2
	call        GenerateConversion

	// *** Basic block 269

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 270

.GenerateExpression_label_435:
	ld          a1, 64(s7)
	lw          t0, 8(s7)
	sw          t0, 8(a1)
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 271

	mv          s10, a0
	mv          a1, s10
	li          t0, 121		// 0x79 ASCII 'y'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 272

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmit

	// *** Basic block 273

	mv          s10, a0
	ld          a1, 16(s1)
	mv          a0, s10
	call        IRSetType

	// *** Basic block 274

	j           .GenerateExpression_label_833

	// *** Basic block 275

.GenerateExpression_label_466:
	mv          a1, s1
	mv          a0, s2
	call        GeneratePointerScale

	// *** Basic block 276

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 277

.GenerateExpression_label_474:
	mv          a3, a0
	mv          a2, x0
	mv          a1, s6
	mv          a0, s2
	call        GenerateIncDec

	// *** Basic block 278

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 279

.GenerateExpression_label_488:
	mv          a3, x0
	mv          a2, x0
	mv          a1, s6
	mv          a0, s2
	call        GenerateIncDec

	// *** Basic block 280

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 281

.GenerateExpression_label_500:
	mv          a3, a0
	mv          a2, a0
	mv          a1, s6
	mv          a0, s2
	call        GenerateIncDec

	// *** Basic block 282

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 283

.GenerateExpression_label_514:
	mv          a3, x0
	mv          a2, a0
	mv          a1, s6
	mv          a0, s2
	call        GenerateIncDec

	// *** Basic block 284

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 285

.GenerateExpression_label_527:
	lw          t0, 8(s1)
	andi        t0, t0, 2
	bnez        t0, .GenerateExpression_label_540

	// *** Basic block 286

	mv          a1, s5
	mv          a0, s2
	call        GenerateInitialization

	// *** Basic block 287

	mv          s10, a0
	j           .GenerateExpression_label_555

	// *** Basic block 288

.GenerateExpression_label_540:
	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 289

	mv          a2, x0
	mv          a1, a0
	mv          a0, s2
	call        GeneratorGetIntConstant

	// *** Basic block 290

	mv          s10, a0

	// *** Basic block 291

.GenerateExpression_label_555:
	j           .GenerateExpression_label_833

	// *** Basic block 292

.GenerateExpression_label_557:
	mv          a1, s5
	mv          a0, s2
	call        GenerateAssignment

	// *** Basic block 293

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 294

.GenerateExpression_label_565:

	// *** Basic block 295

.GenerateExpression_label_566:

	// *** Basic block 296

.GenerateExpression_label_567:

	// *** Basic block 297

.GenerateExpression_label_568:

	// *** Basic block 298

.GenerateExpression_label_569:

	// *** Basic block 299

.GenerateExpression_label_570:

	// *** Basic block 300

.GenerateExpression_label_571:

	// *** Basic block 301

.GenerateExpression_label_572:

	// *** Basic block 302

.GenerateExpression_label_573:

	// *** Basic block 303

.GenerateExpression_label_574:

	// *** Basic block 304

.GenerateExpression_label_575:
	mv          a1, s5
	mv          a0, s2
	call        GenerateCompoundAssignment

	// *** Basic block 305

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 306

.GenerateExpression_label_583:

	// *** Basic block 307

.GenerateExpression_label_584:

	// *** Basic block 308

.GenerateExpression_label_585:

	// *** Basic block 309

.GenerateExpression_label_586:
	mv          a1, s6
	mv          a0, s2
	call        GenerateUnaryExpression

	// *** Basic block 310

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 311

.GenerateExpression_label_594:
	mv          a1, s5
	mv          a0, s2
	call        GenerateIndexExpression

	// *** Basic block 312

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 313

.GenerateExpression_label_602:
	mv          a1, s8
	mv          a0, s2
	call        GenerateFunctionCall

	// *** Basic block 314

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 315

.GenerateExpression_label_610:
	mv          a1, s1
	mv          a0, s2
	call        GenerateInlineCall

	// *** Basic block 316

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 317

.GenerateExpression_label_618:
	mv          a1, s6
	mv          a0, s2
	call        GenerateContentsOf

	// *** Basic block 318

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 319

.GenerateExpression_label_626:
	mv          a1, s6
	mv          a0, s2
	call        GenerateAddressOf

	// *** Basic block 320

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 321

.GenerateExpression_label_634:

	// *** Basic block 322

.GenerateExpression_label_635:
	mv          a1, s5
	mv          a0, s2
	call        GenerateMemberReference

	// *** Basic block 323

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 324

.GenerateExpression_label_643:
	ld          t0, 64(s9)
	sd          t0, -24(s0)	// Spilled @645
	ld          t0, 16(t0)
	lw          t0, 16(t0)
	addi        t1, t0, -2
	seqz        s4, t1
	li          t1, 2		// 0x2 ASCII \x2
	beq         t0, t1, .GenerateExpression_label_660

	// *** Basic block 325

	addi        t0, t0, -1
	seqz        s4, t0

	// *** Basic block 326

.GenerateExpression_label_660:
	beqz        s4, .GenerateExpression_label_667

	// *** Basic block 327

	ld          t1, -40(s0)	// Spilled @648
	addi        t2, t1, 32
	sd          t0, -40(s0)	// Spilled @648
	lb          t2, 16(t2)
	slli        t2, t2, 61
	srai        s4, t2, 63

	// *** Basic block 328

.GenerateExpression_label_667:

	// *** Basic block 329

.GenerateExpression_label_669:
	beqz        s4, .GenerateExpression_label_697

	// *** Basic block 330

	j           .GenerateExpression_label_672

	// *** Basic block 331

.GenerateExpression_label_672:
	addi        t0, t0, 32
	ld          s10, 8(t0)
	beq         s10, x0, .GenerateExpression_label_680

	// *** Basic block 332

	j           .GenerateExpression_label_695

	// *** Basic block 333

.GenerateExpression_label_680:
	lla         a0, .str.25
	lla         a1, .str.26
	lla         a3, .str.27
	li          t0, 1537		// 0x601
	mv          a2, t0
	call        printf

	// *** Basic block 334

	call        abort

	// *** Basic block 335

.GenerateExpression_label_695:
	j           .GenerateExpression_label_708

	// *** Basic block 336

.GenerateExpression_label_697:
	ld          a1, 16(s1)
	ld          a2, 56(s9)
	mv          a0, s2
	call        GeneratorGetIntConstant

	// *** Basic block 337

	mv          s10, a0

	// *** Basic block 338

.GenerateExpression_label_708:
	j           .GenerateExpression_label_833

	// *** Basic block 339

.GenerateExpression_label_710:

	// *** Basic block 340

.GenerateExpression_label_711:
	mv          a1, s5
	mv          a0, s2
	call        GenerateLogicalOperation

	// *** Basic block 341

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 342

.GenerateExpression_label_719:
	mv          a1, s5
	mv          a0, s2
	call        GenerateConditionalExpression

	// *** Basic block 343

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 344

.GenerateExpression_label_727:
	ld          a1, 56(s5)
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 345

	ld          a1, 64(s5)
	mv          a0, s2
	call        GenerateExpression

	// *** Basic block 346

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 347

.GenerateExpression_label_742:

	// *** Basic block 348

.GenerateExpression_label_743:
	lla         a0, .str.28
	lla         a1, .str.29
	lla         a3, .str.30
	li          t0, 1560		// 0x618
	mv          a2, t0
	call        printf

	// *** Basic block 349

	call        abort

	// *** Basic block 350

	j           .GenerateExpression_label_833

	// *** Basic block 351

.GenerateExpression_label_759:
	j           .GenerateExpression_label_833

	// *** Basic block 352

.GenerateExpression_label_761:
	mv          a1, s8
	mv          a0, s2
	call        GenerateBuiltinVaStart

	// *** Basic block 353

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 354

.GenerateExpression_label_769:
	mv          a1, s8
	mv          a0, s2
	call        GenerateBuiltinVaArg

	// *** Basic block 355

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 356

.GenerateExpression_label_777:
	mv          a1, s8
	mv          a0, s2
	call        GenerateBuiltinVaEnd

	// *** Basic block 357

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 358

.GenerateExpression_label_785:
	mv          a1, s8
	mv          a0, s2
	call        GenerateBuiltinVaCopy

	// *** Basic block 359

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 360

.GenerateExpression_label_793:
	mv          a1, s1
	mv          a0, s2
	call        GenerateAsm

	// *** Basic block 361

	mv          s10, a0
	j           .GenerateExpression_label_833

	// *** Basic block 362

.GenerateExpression_label_801:
	la          t0, stderr
	ld          s1, 0(t0)
	sd          s1, -32(s0)	// Spilled @803
	lla         s1, .str.31
	mv          a0, s11
	call        ASTOpcodeName

	// *** Basic block 363

	mv          a3, s11
	mv          a2, a0
	mv          a1, s1
	ld          a0, -32(s0)	// Spilled @803
	call        fprintf

	// *** Basic block 364

	lla         a0, .str.32
	lla         a1, .str.33
	lla         a3, .str.34
	li          t0, 1589		// 0x635
	mv          a2, t0
	call        printf

	// *** Basic block 365

	call        abort

	// *** Basic block 366

	j           .GenerateExpression_label_833

	// *** Basic block 367

.GenerateExpression_label_833:
	ld          t0, 80(s10)
	beq         t0, x0, .GenerateExpression_label_840

	// *** Basic block 368

	j           .GenerateExpression_label_855

	// *** Basic block 369

.GenerateExpression_label_840:
	lla         a0, .str.35
	lla         a1, .str.36
	lla         a3, .str.37
	li          t0, 1593		// 0x639
	mv          a2, t0
	call        printf

	// *** Basic block 370

	call        abort

	// *** Basic block 371

.GenerateExpression_label_855:
	mv          a0, s10

	// *** Basic block 372

.GenerateExpression_label_858:
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
.func_end_GenerateExpression:
	.size GenerateExpression, .func_end_GenerateExpression-GenerateExpression

.PCend:
	.data
expr_operators:
	.type   expr_operators,@object
	.local  expr_operators
	.size   expr_operators,1248
	.p2align  3
	.word   58
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   37
	.space  4
	.word   58
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   38
	.space  4
	.word   58
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   39
	.space  4
	.word   58
	.space  4
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   40
	.space  4
	.word   51
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   41
	.space  4
	.word   51
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   42
	.space  4
	.word   51
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   43
	.space  4
	.word   51
	.space  4
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   44
	.space  4
	.word   68
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   45
	.space  4
	.word   68
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   46
	.space  4
	.word   68
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   47
	.space  4
	.word   66
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   48
	.space  4
	.word   66
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   49
	.space  4
	.word   66
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   50
	.space  4
	.word   56
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   51
	.space  4
	.word   48
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   54
	.space  4
	.word   13
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   52
	.space  4
	.word   14
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   53
	.space  4
	.word   17
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   56
	.space  4
	.word   22
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   55
	.space  4
	.word   24
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   57
	.space  4
	.word   34
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   64
	.space  4
	.word   54
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   65
	.space  4
	.word   42
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   66
	.space  4
	.word   43
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   67
	.space  4
	.word   37
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   68
	.space  4
	.word   38
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   69
	.space  4
	.word   34
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   70
	.space  4
	.word   54
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   71
	.space  4
	.word   42
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   72
	.space  4
	.word   43
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   73
	.space  4
	.word   37
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   74
	.space  4
	.word   38
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   75
	.space  4
	.word   34
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   76
	.space  4
	.word   54
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   77
	.space  4
	.word   42
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   78
	.space  4
	.word   43
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   79
	.space  4
	.word   37
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   80
	.space  4
	.word   38
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   81
	.space  4
	.word   34
	.space  4
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   82
	.space  4
	.word   54
	.space  4
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   83
	.space  4
	.word   42
	.space  4
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   84
	.space  4
	.word   43
	.space  4
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   85
	.space  4
	.word   37
	.space  4
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   86
	.space  4
	.word   38
	.space  4
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   87
	.space  4
	.word   21
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   58
	.space  4
	.word   21
	.space  4
	.global TypeIsPointer
	.long    TypeIsPointer
	.word   59
	.space  4
	.word   71
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   60
	.space  4
	.word   9
	.space  4
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   61
	.space  4
	.word   9
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   62
	.space  4
	.word   9
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   63
	.space  4
	.word   0
	.space  4
	.word   0
	.space  4
	.word   0
	.space  4

load_store_ops:
	.type   load_store_ops,@object
	.local  load_store_ops
	.size   load_store_ops,288
	.p2align  3
	.global TypeIsInt
	.long    TypeIsInt
	.word   19
	.word   23
	.word   30
	.space  4
	.global TypeIsChar
	.long    TypeIsChar
	.word   20
	.word   24
	.word   31
	.space  4
	.global TypeIsBool
	.long    TypeIsBool
	.word   20
	.word   24
	.word   31
	.space  4
	.global TypeIsShort
	.long    TypeIsShort
	.word   22
	.word   25
	.word   32
	.space  4
	.global TypeIsLong
	.long    TypeIsLong
	.word   21
	.word   21
	.word   33
	.space  4
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.word   21
	.word   21
	.word   33
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   26
	.word   26
	.word   34
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   27
	.word   27
	.word   35
	.space  4
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   28
	.word   28
	.word   36
	.space  4
	.global TypeIsFunction
	.long    TypeIsFunction
	.word   28
	.word   28
	.word   36
	.space  4
	.global TypeIsStructOrUnion
	.long    TypeIsStructOrUnion
	.word   28
	.word   28
	.word   36
	.space  4
	.word   0
	.space  4
	.word   0
	.word   0
	.word   0
	.space  4

rmov_opcodes:
	.type   rmov_opcodes,@object
	.local  rmov_opcodes
	.size   rmov_opcodes,128
	.p2align  3
	.global TypeIsIntegral
	.long    TypeIsIntegral
	.word   13
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   14
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   15
	.space  4
	.global TypeIsStructOrUnion
	.long    TypeIsStructOrUnion
	.word   16
	.space  4
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   16
	.space  4
	.global TypeIsFunction
	.long    TypeIsFunction
	.word   16
	.space  4
	.global TypeIsVoid
	.long    TypeIsVoid
	.word   16
	.space  4
	.word   0
	.space  4
	.word   0
	.space  4

compound_assignment_ops:
	.type   compound_assignment_ops,@object
	.local  compound_assignment_ops
	.size   compound_assignment_ops,96
	.p2align  3
	.word   59
	.word   58
	.word   52
	.word   51
	.word   69
	.word   68
	.word   67
	.word   66
	.word   57
	.word   56
	.word   49
	.word   48
	.word   16
	.word   14
	.word   15
	.word   13
	.word   18
	.word   17
	.word   55
	.word   22
	.word   25
	.word   24
	.word   0
	.word   0

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.1, @object
	.size .str.1, 30

.str.2:
	.asciz "expr_codegen.c"
	.type .str.2, @object
	.size .str.2, 15

.str.3:
	.asciz "false"
	.type .str.3, @object
	.size .str.3, 6

.str.4:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.4, @object
	.size .str.4, 30

.str.5:
	.asciz "expr_codegen.c"
	.type .str.5, @object
	.size .str.5, 15

.str.6:
	.asciz "false"
	.type .str.6, @object
	.size .str.6, 6

.str.7:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.7, @object
	.size .str.7, 30

.str.8:
	.asciz "expr_codegen.c"
	.type .str.8, @object
	.size .str.8, 15

.str.9:
	.asciz "false"
	.type .str.9, @object
	.size .str.9, 6

.str.10:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.10, @object
	.size .str.10, 30

.str.11:
	.asciz "expr_codegen.c"
	.type .str.11, @object
	.size .str.11, 15

.str.12:
	.asciz "false"
	.type .str.12, @object
	.size .str.12, 6

.str.13:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.13, @object
	.size .str.13, 30

.str.14:
	.asciz "expr_codegen.c"
	.type .str.14, @object
	.size .str.14, 15

.str.15:
	.asciz "subinit->op == AST_OP(designated_init)"
	.type .str.15, @object
	.size .str.15, 39

.str.16:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.16, @object
	.size .str.16, 30

.str.17:
	.asciz "expr_codegen.c"
	.type .str.17, @object
	.size .str.17, 15

.str.18:
	.asciz "node->right->op == AST_OP(braced_init)"
	.type .str.18, @object
	.size .str.18, 39

.str.19:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.19, @object
	.size .str.19, 30

.str.20:
	.asciz "expr_codegen.c"
	.type .str.20, @object
	.size .str.20, 15

.str.21:
	.asciz "alu_op != AST_OP(bad)"
	.type .str.21, @object
	.size .str.21, 22

.str.22:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.22, @object
	.size .str.22, 30

.str.23:
	.asciz "expr_codegen.c"
	.type .str.23, @object
	.size .str.23, 15

.str.24:
	.asciz "TypeIsIntegral(node->type)"
	.type .str.24, @object
	.size .str.24, 27

.str.25:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.25, @object
	.size .str.25, 30

.str.26:
	.asciz "expr_codegen.c"
	.type .str.26, @object
	.size .str.26, 15

.str.27:
	.asciz "result != NULL"
	.type .str.27, @object
	.size .str.27, 15

.str.28:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.28, @object
	.size .str.28, 30

.str.29:
	.asciz "expr_codegen.c"
	.type .str.29, @object
	.size .str.29, 15

.str.30:
	.asciz "false"
	.type .str.30, @object
	.size .str.30, 6

.str.31:
	.asciz "Invalid expression AST op: %s (%d)\n"
	.type .str.31, @object
	.size .str.31, 36

.str.32:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.32, @object
	.size .str.32, 30

.str.33:
	.asciz "expr_codegen.c"
	.type .str.33, @object
	.size .str.33, 15

.str.34:
	.asciz "false"
	.type .str.34, @object
	.size .str.34, 6

.str.35:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.35, @object
	.size .str.35, 30

.str.36:
	.asciz "expr_codegen.c"
	.type .str.36, @object
	.size .str.36, 15

.str.37:
	.asciz "result->type != NULL"
	.type .str.37, @object
	.size .str.37, 21

