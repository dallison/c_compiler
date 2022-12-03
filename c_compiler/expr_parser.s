	.file   "expr_parser.c"
	.text
	.option pic
.PCbegin:
	.local  GetIntrinsicIndex
	.type GetIntrinsicIndex, @function

GetIntrinsicIndex:

	// *** Basic block 0

	.local intrinsics
	.global strcmp
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
	la          t0, intrinsics
	ld          t0, 0(t0)
	beq         t0, x0, .GetIntrinsicIndex_label_48

	// *** Basic block 1

.GetIntrinsicIndex_label_20:
	slli        t0, s2, 4
	la          t1, intrinsics
	add         t0, t1, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        strcmp

	// *** Basic block 2

	bnez        a0, .GetIntrinsicIndex_label_38

	// *** Basic block 3

	mv          a0, s2

	// *** Basic block 4

.GetIntrinsicIndex_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.GetIntrinsicIndex_label_38:

	// *** Basic block 6

.GetIntrinsicIndex_label_39:
	addi        s2, s2, 1
	slli        t0, s2, 4
	la          t1, intrinsics
	add         t0, t1, t0
	ld          t0, 0(t0)
	beq         t0, x0, .GetIntrinsicIndex_label_20

	// *** Basic block 7

.GetIntrinsicIndex_label_48:
	li          a0, -1		// 0xffffffffffffffff
	j           .GetIntrinsicIndex_label_35
.func_end_GetIntrinsicIndex:
	.size GetIntrinsicIndex, .func_end_GetIntrinsicIndex-GetIntrinsicIndex

	.local  ParseIdentifier
	.type ParseIdentifier, @function

ParseIdentifier:

	// *** Basic block 0

	.global StringInit
	.global LexNextToken
	.global NewMacroNameASTNode
	.global StringDestruct
	.global SyntaxFindSymbol
	.global NewTypeRecord
	.global NewSymbol
	.global LexLookingAt
	.local GetIntrinsicIndex
	.global SyntaxWarning
	.global NewFunctionTypeRecord
	.global TypeRecordChain
	.global SyntaxError
	.global SyntaxFakeName
	.global NewIdentifierASTNode
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
	ld          s2, 0(s1)
	addi        a0, s0, -64
	addi        t0, s2, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 1

	mv          a0, s2
	call        LexNextToken

	// *** Basic block 2

	lb          t0, 176(s2)
	beqz        t0, .ParseIdentifier_label_74

	// *** Basic block 3

	addi        a0, s0, -64
	ld          a1, 56(s2)
	call        NewMacroNameASTNode

	// *** Basic block 4

	mv          s3, a0
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 5

	mv          a0, s3

	// *** Basic block 6

.ParseIdentifier_label_71:
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

.ParseIdentifier_label_74:
	addi        a1, s0, -64
	mv          a0, s1
	call        SyntaxFindSymbol

	// *** Basic block 8

	mv          s4, a0
	bne         s4, x0, .ParseIdentifier_label_216

	// *** Basic block 9

	lb          t0, 178(s2)
	beqz        t0, .ParseIdentifier_label_113

	// *** Basic block 10

	mv          a1, x0
	li          t0, 16392		// 0x4008
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 11

	mv          s5, a0
	addi        t0, s0, -64
	ld          a0, 16(t0)
	mv          a2, x0
	mv          a1, s5
	call        NewSymbol

	// *** Basic block 12

	mv          s4, a0
	lb          t0, 56(s4)
	andi        t0, t0, -5
	ori         t0, t0, 4
	sb          t0, 56(s4)
	j           .ParseIdentifier_label_215

	// *** Basic block 13

.ParseIdentifier_label_113:
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	mv          a0, s2
	call        LexLookingAt

	// *** Basic block 14

	beqz        a0, .ParseIdentifier_label_178

	// *** Basic block 15

	addi        t0, s0, -64
	ld          s6, 16(t0)
	mv          a0, s6
	call        GetIntrinsicIndex

	// *** Basic block 16

	li          t0, -1		// 0xffffffffffffffff
	bne         a0, t0, .ParseIdentifier_label_143

	// *** Basic block 17

	lla         a1, .str.5
	lla         a2, .str.6
	mv          a3, s6
	mv          a0, s1
	call        SyntaxWarning

	// *** Basic block 18

.ParseIdentifier_label_143:
	mv          a1, x0
	li          t0, 32770		// 0x8002
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 19

	mv          s7, a0
	call        NewFunctionTypeRecord

	// *** Basic block 20

	mv          s8, a0
	addi        t0, s8, 32
	li          t1, 1		// 0x1 ASCII \x1
	sb          t1, 48(t0)
	mv          a1, s7
	mv          a0, s8
	call        TypeRecordChain

	// *** Basic block 21

	mv          a2, x0
	mv          a1, s8
	mv          a0, s6
	call        NewSymbol

	// *** Basic block 22

	mv          s4, a0
	lb          t0, 56(s4)
	andi        t0, t0, -5
	ori         t0, t0, 4
	sb          t0, 56(s4)
	j           .ParseIdentifier_label_214

	// *** Basic block 23

.ParseIdentifier_label_178:
	lla         a1, .str.7
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 24

	mv          a1, x0
	li          t0, 32770		// 0x8002
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 25

	mv          s6, a0
	mv          a0, s1
	call        SyntaxFakeName

	// *** Basic block 26

	mv          a2, x0
	mv          a1, s6
	call        NewSymbol

	// *** Basic block 27

	mv          s4, a0
	addi        t0, s4, 56
	lb          t1, 1(t0)
	andi        t1, t1, -2
	ori         t1, t1, 1
	sb          t1, 1(t0)

	// *** Basic block 28

.ParseIdentifier_label_214:

	// *** Basic block 29

.ParseIdentifier_label_215:

	// *** Basic block 30

.ParseIdentifier_label_216:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 31

	ld          a1, 56(s2)
	mv          a0, s4
	call        NewIdentifierASTNode

	// *** Basic block 32

	j           .ParseIdentifier_label_71
.func_end_ParseIdentifier:
	.size ParseIdentifier, .func_end_ParseIdentifier-ParseIdentifier

	.local  ParseIntegerConstant
	.type ParseIntegerConstant, @function

ParseIntegerConstant:

	// *** Basic block 0

	.global LexNextToken
	.global StringContainsChar
	.global StringContainsString
	.global NewTypeRecord
	.global NewIntConstantASTNode
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
	ld          s1, 0(t0)
	ld          s2, 120(s1)
	mv          a0, s1
	call        LexNextToken

	// *** Basic block 1

	li          s3, 2		// 0x2 ASCII \x2
	addi        a0, s1, 136
	li          t0, 85		// 0x55 ASCII 'U'
	mv          a1, t0
	call        StringContainsChar

	// *** Basic block 2

	beqz        a0, .ParseIntegerConstant_label_42

	// *** Basic block 3

	li          s3, 16386		// 0x4002

	// *** Basic block 4

.ParseIntegerConstant_label_42:
	addi        a0, s1, 136
	lla         a1, .str.8
	call        StringContainsString

	// *** Basic block 5

	beqz        a0, .ParseIntegerConstant_label_53

	// *** Basic block 6

	andi        s3, s3, -3
	ori         s3, s3, 16
	j           .ParseIntegerConstant_label_64

	// *** Basic block 7

.ParseIntegerConstant_label_53:
	addi        a0, s1, 136
	li          t0, 76		// 0x4c ASCII 'L'
	mv          a1, t0
	call        StringContainsChar

	// *** Basic block 8

	beqz        a0, .ParseIntegerConstant_label_63

	// *** Basic block 9

	andi        s3, s3, -3
	ori         s3, s3, 8

	// *** Basic block 10

.ParseIntegerConstant_label_63:

	// *** Basic block 11

.ParseIntegerConstant_label_64:
	mv          a1, x0
	mv          a0, s3
	call        NewTypeRecord

	// *** Basic block 12

	mv          s3, a0
	ld          a2, 56(s1)
	mv          a1, s3
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewIntConstantASTNode
.func_end_ParseIntegerConstant:
	.size ParseIntegerConstant, .func_end_ParseIntegerConstant-ParseIntegerConstant

	.local  ParseFloatingPointConstant
	.type ParseFloatingPointConstant, @function

ParseFloatingPointConstant:

	// *** Basic block 0

	.global LexNextToken
	.global StringContainsChar
	.global NewTypeRecord
	.global NewRealConstantASTNode
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
	// Saved floating point registers.
	fsd fs0, 0(sp)
	// End of stack frame
	mv          t0, a0
	ld          s1, 0(t0)
	fld         fs0, 128(s1)
	mv          a0, s1
	call        LexNextToken

	// *** Basic block 1

	li          s2, 64		// 0x40 ASCII '@'
	addi        a0, s1, 136
	li          t0, 70		// 0x46 ASCII 'F'
	mv          a1, t0
	call        StringContainsChar

	// *** Basic block 2

	beqz        a0, .ParseFloatingPointConstant_label_40

	// *** Basic block 3

	mv          s2, x0
	li          s2, 32		// 0x20 ASCII ' '
	j           .ParseFloatingPointConstant_label_51

	// *** Basic block 4

.ParseFloatingPointConstant_label_40:
	addi        a0, s1, 136
	li          t0, 76		// 0x4c ASCII 'L'
	mv          a1, t0
	call        StringContainsChar

	// *** Basic block 5

	beqz        a0, .ParseFloatingPointConstant_label_50

	// *** Basic block 6

	mv          s2, x0
	li          s2, 128		// 0x80 ASCII \x80

	// *** Basic block 7

.ParseFloatingPointConstant_label_50:

	// *** Basic block 8

.ParseFloatingPointConstant_label_51:
	mv          a1, x0
	mv          a0, s2
	call        NewTypeRecord

	// *** Basic block 9

	mv          s3, a0
	ld          a1, 56(s1)
	mv          a0, s3
	fmv.d       fa0, fs0
	// Restored registers.
	fld fs0, 24(sp)
	ld s1, 16(sp)
	ld s2, 8(sp)
	ld s3, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewRealConstantASTNode
.func_end_ParseFloatingPointConstant:
	.size ParseFloatingPointConstant, .func_end_ParseFloatingPointConstant-ParseFloatingPointConstant

	.local  ParseStringLiteral
	.type ParseStringLiteral, @function

ParseStringLiteral:

	// *** Basic block 0

	.global NewString
	.global LexNextToken
	.global LexLookingAt
	.global StringAppend
	.global NewBasicArrayTypeRecord
	.global NewTypeRecord
	.global TypeRecordChain
	.global NewStringConstantASTNode
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
	ld          s1, 0(t0)
	addi        t1, s1, 80
	ld          s2, 16(t1)
	mv          a0, s2
	call        NewString

	// *** Basic block 1

	mv          s3, a0
	mv          a0, s1
	call        LexNextToken

	// *** Basic block 2

	li          s4, 4		// 0x4 ASCII \x4
	mv          a1, s4
	mv          a0, s1
	call        LexLookingAt

	// *** Basic block 3

	beqz        a0, .ParseStringLiteral_label_59

	// *** Basic block 4

.ParseStringLiteral_label_43:
	mv          a1, s2
	mv          a0, s3
	call        StringAppend

	// *** Basic block 5

	mv          a0, s1
	call        LexNextToken

	// *** Basic block 6

	mv          a1, s4
	mv          a0, s1
	call        LexLookingAt

	// *** Basic block 7

	bnez        a0, .ParseStringLiteral_label_43

	// *** Basic block 8

.ParseStringLiteral_label_59:
	ld          t0, 24(s3)
	addi        a1, t0, 1
	mv          a2, x0
	mv          a0, x0
	call        NewBasicArrayTypeRecord

	// *** Basic block 9

	mv          s2, a0
	mv          a1, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 10

	mv          s4, a0
	mv          a1, s4
	mv          a0, s2
	call        TypeRecordChain

	// *** Basic block 11

	ld          a2, 56(s1)
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
	j           NewStringConstantASTNode
.func_end_ParseStringLiteral:
	.size ParseStringLiteral, .func_end_ParseStringLiteral-ParseStringLiteral

	.local  ParseWideStringLiteral
	.type ParseWideStringLiteral, @function

ParseWideStringLiteral:

	// *** Basic block 0

	.global NewStringWithLength
	.global LexNextToken
	.global LexLookingAt
	.global StringAppend
	.global NewBasicArrayTypeRecord
	.global NewTypeRecord
	.global TypeRecordChain
	.global NewWideStringConstantASTNode
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
	ld          s1, 0(t0)
	addi        t1, s1, 80
	ld          s2, 16(t1)
	ld          t1, 24(t1)
	addi        a1, t1, 4
	mv          a0, s2
	call        NewStringWithLength

	// *** Basic block 1

	mv          s3, a0
	mv          a0, s1
	call        LexNextToken

	// *** Basic block 2

	li          s4, 5		// 0x5 ASCII \x5
	mv          a1, s4
	mv          a0, s1
	call        LexLookingAt

	// *** Basic block 3

	beqz        a0, .ParseWideStringLiteral_label_65

	// *** Basic block 4

.ParseWideStringLiteral_label_49:
	mv          a1, s2
	mv          a0, s3
	call        StringAppend

	// *** Basic block 5

	mv          a0, s1
	call        LexNextToken

	// *** Basic block 6

	mv          a1, s4
	mv          a0, s1
	call        LexLookingAt

	// *** Basic block 7

	bnez        a0, .ParseWideStringLiteral_label_49

	// *** Basic block 8

.ParseWideStringLiteral_label_65:
	ld          t0, 24(s3)
	addi        a1, t0, 4
	mv          a2, x0
	mv          a0, x0
	call        NewBasicArrayTypeRecord

	// *** Basic block 9

	mv          s2, a0
	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 10

	mv          s4, a0
	mv          a1, s4
	mv          a0, s2
	call        TypeRecordChain

	// *** Basic block 11

	ld          a2, 56(s1)
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
	j           NewWideStringConstantASTNode
.func_end_ParseWideStringLiteral:
	.size ParseWideStringLiteral, .func_end_ParseWideStringLiteral-ParseWideStringLiteral

	.local  ParseCharacterConstant
	.type ParseCharacterConstant, @function

ParseCharacterConstant:

	// *** Basic block 0

	.global LexNextToken
	.global NewTypeRecord
	.global NewCharConstantASTNode
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
	ld          t1, 0(t0)
	ld          s1, 120(t1)
	mv          a0, t1
	call        LexNextToken

	// *** Basic block 1

	mv          a1, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 2

	mv          s2, a0
	ld          a2, 56(t1)
	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewCharConstantASTNode
.func_end_ParseCharacterConstant:
	.size ParseCharacterConstant, .func_end_ParseCharacterConstant-ParseCharacterConstant

	.local  ParseWideCharacterConstant
	.type ParseWideCharacterConstant, @function

ParseWideCharacterConstant:

	// *** Basic block 0

	.global LexNextToken
	.global NewTypeRecord
	.global NewCharConstantASTNode
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
	ld          t1, 0(t0)
	ld          s1, 120(t1)
	mv          a0, t1
	call        LexNextToken

	// *** Basic block 1

	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 2

	mv          s2, a0
	ld          a2, 56(t1)
	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewCharConstantASTNode
.func_end_ParseWideCharacterConstant:
	.size ParseWideCharacterConstant, .func_end_ParseWideCharacterConstant-ParseWideCharacterConstant

	.local  ParsePrimaryExpression
	.type ParsePrimaryExpression, @function

ParsePrimaryExpression:

	// *** Basic block 0

	.global LexMatch
	.global SyntaxParseExpression
	.global SyntaxNeedBracket
	.global LexLookingAt
	.local ParseIdentifier
	.local ParseIntegerConstant
	.local ParseFloatingPointConstant
	.local ParseStringLiteral
	.local ParseWideStringLiteral
	.local ParseCharacterConstant
	.local ParseWideCharacterConstant
	.global SyntaxError
	.global SyntaxRecover
	.global NewTypeRecord
	.global NewIntConstantASTNode
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
	ld          s3, 0(s1)
	lb          s4, 68(s1)
	bnez        s4, .ParsePrimaryExpression_label_52

	// *** Basic block 1

	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 2

	mv          s4, a0

	// *** Basic block 3

.ParsePrimaryExpression_label_52:
	beqz        s4, .ParsePrimaryExpression_label_77

	// *** Basic block 4

	sb          x0, 68(s1)
	ori         a1, s2, 8
	mv          a0, s1
	call        SyntaxParseExpression

	// *** Basic block 5

	mv          s5, a0
	mv          a2, s2
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 6

	mv          a0, s5

	// *** Basic block 7

.ParsePrimaryExpression_label_74:
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

	// *** Basic block 8

.ParsePrimaryExpression_label_77:
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 9

	beqz        a0, .ParsePrimaryExpression_label_94

	// *** Basic block 10

	mv          a1, s2
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
	j           ParseIdentifier

	// *** Basic block 12

.ParsePrimaryExpression_label_94:
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 13

	beqz        a0, .ParsePrimaryExpression_label_111

	// *** Basic block 14

	mv          a1, s2
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
	j           ParseIntegerConstant

	// *** Basic block 16

.ParsePrimaryExpression_label_111:
	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 17

	beqz        a0, .ParsePrimaryExpression_label_128

	// *** Basic block 18

	mv          a1, s2
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
	j           ParseFloatingPointConstant

	// *** Basic block 20

.ParsePrimaryExpression_label_128:
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 21

	beqz        a0, .ParsePrimaryExpression_label_145

	// *** Basic block 22

	mv          a1, s2
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
	j           ParseStringLiteral

	// *** Basic block 24

.ParsePrimaryExpression_label_145:
	li          t0, 5		// 0x5 ASCII \x5
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 25

	beqz        a0, .ParsePrimaryExpression_label_162

	// *** Basic block 26

	mv          a1, s2
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
	j           ParseWideStringLiteral

	// *** Basic block 28

.ParsePrimaryExpression_label_162:
	li          t0, 6		// 0x6 ASCII \x6
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 29

	beqz        a0, .ParsePrimaryExpression_label_179

	// *** Basic block 30

	mv          a1, s2
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
	j           ParseCharacterConstant

	// *** Basic block 32

.ParsePrimaryExpression_label_179:
	li          t0, 7		// 0x7 ASCII \x7
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 33

	beqz        a0, .ParsePrimaryExpression_label_196

	// *** Basic block 34

	mv          a1, s2
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
	j           ParseWideCharacterConstant

	// *** Basic block 36

.ParsePrimaryExpression_label_196:
	lla         a1, .str.9
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 37

	mv          a1, s2
	mv          a0, s1
	call        SyntaxRecover

	// *** Basic block 38

	mv          a1, x0
	li          t0, 32770		// 0x8002
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 39

	mv          s6, a0
	ld          a2, 56(s3)
	mv          a1, s6
	mv          a0, x0
	call        NewIntConstantASTNode

	// *** Basic block 40

	j           .ParsePrimaryExpression_label_74
.func_end_ParsePrimaryExpression:
	.size ParsePrimaryExpression, .func_end_ParsePrimaryExpression-ParsePrimaryExpression

	.local  VarargsIntrinsic
	.type VarargsIntrinsic, @function

VarargsIntrinsic:

	// *** Basic block 0

	.local GetIntrinsicIndex
	.global NewVector
	.global LexLookingAt
	.local intrinsics
	.global TypeParserInit
	.global TypeParserParseType
	.global TypeParserParseDeclarator
	.global NewIntConstantASTNode
	.global SymbolDelete
	.global SyntaxParseSingleExpression
	.global VectorAppend
	.global LexMatch
	.global SyntaxNeedBracket
	.global SyntaxError
	.global NewVectorASTNode
	sd          a0, -0(s0)	// Spilled @226
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
	mv          s1, a1
	mv          s2, a0
	mv          s3, a2
	lw          t0, 0(s1)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .VarargsIntrinsic_label_225

	// *** Basic block 1

	mv          s4, s1
	ld          t0, 56(s4)
	ld          s5, 16(t0)
	mv          a0, s5
	call        GetIntrinsicIndex

	// *** Basic block 2

	mv          s5, a0
	li          t0, -1		// 0xffffffffffffffff
	bne         s5, t0, .VarargsIntrinsic_label_78

	// *** Basic block 3

	slli        t0, s5, 4
	la          t1, intrinsics
	add         t0, t1, t0
	lw          t0, 8(t0)
	addi        t1, t0, -77
	seqz        t2, t1
	mv          a0, x0

	// *** Basic block 4

.VarargsIntrinsic_label_75:
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

	// *** Basic block 5

.VarargsIntrinsic_label_78:
	call        NewVector

	// *** Basic block 6

	mv          s4, a0
	ld          s6, 0(s2)
	li          s7, 45		// 0x2d ASCII '-'
	mv          a1, s7
	mv          a0, s6
	call        LexLookingAt

	// *** Basic block 7

	ld          t0, 8(s4)
	addi        t0, t0, -1
	seqz        t2, t0
	ld          s8, 56(s6)
	not         t0, a0
	beqz        t0, .VarargsIntrinsic_label_179

	// *** Basic block 8

.VarargsIntrinsic_label_97:
	li          t0, 77		// 0x4d ASCII 'M'
	bne         t0, t0, .VarargsIntrinsic_label_101

	// *** Basic block 9

.VarargsIntrinsic_label_101:
	beqz        t2, .VarargsIntrinsic_label_149

	// *** Basic block 10

	addi        a0, s0, -96
	li          s9, 1		// 0x1 ASCII \x1
	mv          a4, s9
	mv          a3, x0
	mv          a2, s2
	mv          a1, s6
	call        TypeParserInit

	// *** Basic block 11

	addi        a0, s0, -96
	mv          a1, s9
	call        TypeParserParseType

	// *** Basic block 12

	mv          s9, a0
	addi        a0, s0, -96
	mv          a1, s9
	call        TypeParserParseDeclarator

	// *** Basic block 13

	mv          s10, a0
	ld          s9, 40(s10)
	mv          a2, s8
	mv          a1, s9
	mv          a0, x0
	call        NewIntConstantASTNode

	// *** Basic block 14

	mv          s8, a0
	mv          a0, s10
	call        SymbolDelete

	// *** Basic block 15

	j           .VarargsIntrinsic_label_156

	// *** Basic block 16

.VarargsIntrinsic_label_149:
	mv          a1, s3
	mv          a0, s2
	call        SyntaxParseSingleExpression

	// *** Basic block 17

	mv          s8, a0

	// *** Basic block 18

.VarargsIntrinsic_label_156:
	mv          a1, s8
	mv          a0, s4
	call        VectorAppend

	// *** Basic block 19

	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	mv          a0, s6
	call        LexMatch

	// *** Basic block 20

	not         t0, a0
	bnez        t0, .VarargsIntrinsic_label_179

	// *** Basic block 21

.VarargsIntrinsic_label_170:
	mv          a1, s7
	mv          a0, s6
	call        LexLookingAt

	// *** Basic block 22

	not         t0, a0
	bnez        t0, .VarargsIntrinsic_label_97

	// *** Basic block 23

.VarargsIntrinsic_label_179:
	mv          a2, s3
	mv          a1, s7
	mv          a0, s2
	call        SyntaxNeedBracket

	// *** Basic block 24

	ld          s7, 8(s4)
	slli        t0, s5, 4
	la          t1, intrinsics
	add         s11, t1, t0
	lw          a0, 12(s11)
	beq         s7, a0, .VarargsIntrinsic_label_208

	// *** Basic block 25

	lla         a1, .str.10
	mv          a3, s7
	mv          a2, a0
	mv          a0, s2
	call        SyntaxError

	// *** Basic block 26

.VarargsIntrinsic_label_208:
	lw          a0, 8(s11)
	ld          a2, 56(s6)
	mv          a4, s4
	mv          a3, s1
	mv          a1, x0
	call        NewVectorASTNode

	// *** Basic block 27

	j           .VarargsIntrinsic_label_75

	// *** Basic block 28

.VarargsIntrinsic_label_225:
	mv          a0, x0
	j           .VarargsIntrinsic_label_75
.func_end_VarargsIntrinsic:
	.size VarargsIntrinsic, .func_end_VarargsIntrinsic-VarargsIntrinsic

	.local  ParseArraySubscript
	.type ParseArraySubscript, @function

ParseArraySubscript:

	// *** Basic block 0

	.global SyntaxParseExpression
	.global SyntaxNeedBracket
	.global NewBinaryASTNode
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
	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseExpression

	// *** Basic block 1

	mv          s4, a0
	mv          a2, s2
	li          t0, 48		// 0x30 ASCII '0'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 2

	ld          t0, 0(s1)
	ld          a2, 56(t0)
	mv          a4, s4
	mv          a3, s3
	mv          a1, x0
	li          t0, 50		// 0x32 ASCII '2'
	mv          a0, t0
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewBinaryASTNode
.func_end_ParseArraySubscript:
	.size ParseArraySubscript, .func_end_ParseArraySubscript-ParseArraySubscript

	.local  ParseFunctionCall
	.type ParseFunctionCall, @function

ParseFunctionCall:

	// *** Basic block 0

	.local VarargsIntrinsic
	.global NewVector
	.global LexLookingAt
	.global SyntaxParseSingleExpression
	.global VectorAppend
	.global LexMatch
	.global SyntaxNeedBracket
	.global NewVectorASTNode
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
	mv          a1, s2
	mv          a0, s1
	call        VarargsIntrinsic

	// *** Basic block 1

	mv          s4, a0
	beq         s4, x0, .ParseFunctionCall_label_42

	// *** Basic block 2

	mv          a0, s4

	// *** Basic block 3

.ParseFunctionCall_label_39:
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

.ParseFunctionCall_label_42:
	call        NewVector

	// *** Basic block 5

	mv          s5, a0
	ld          s6, 0(s1)
	li          s7, 45		// 0x2d ASCII '-'
	mv          a1, s7
	mv          a0, s6
	call        LexLookingAt

	// *** Basic block 6

	not         t0, a0
	beqz        t0, .ParseFunctionCall_label_85

	// *** Basic block 7

.ParseFunctionCall_label_55:
	mv          a1, s3
	mv          a0, s1
	call        SyntaxParseSingleExpression

	// *** Basic block 8

	mv          s8, a0
	mv          a1, s8
	mv          a0, s5
	call        VectorAppend

	// *** Basic block 9

	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	mv          a0, s6
	call        LexMatch

	// *** Basic block 10

	not         t0, a0
	bnez        t0, .ParseFunctionCall_label_85

	// *** Basic block 11

.ParseFunctionCall_label_76:
	mv          a1, s7
	mv          a0, s6
	call        LexLookingAt

	// *** Basic block 12

	not         t0, a0
	bnez        t0, .ParseFunctionCall_label_55

	// *** Basic block 13

.ParseFunctionCall_label_85:
	mv          a2, s3
	mv          a1, s7
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 14

	ld          a2, 56(s6)
	mv          a4, s5
	mv          a3, s2
	mv          a1, x0
	li          t0, 46		// 0x2e ASCII '.'
	mv          a0, t0
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
	j           NewVectorASTNode
.func_end_ParseFunctionCall:
	.size ParseFunctionCall, .func_end_ParseFunctionCall-ParseFunctionCall

	.local  ParseStructMember
	.type ParseStructMember, @function

ParseStructMember:

	// *** Basic block 0

	.global LexLookingAt
	.global NewString
	.global LexNextToken
	.global SyntaxError
	.global SyntaxFakeName
	.global NewStringConstantASTNode
	.global NewBinaryASTNode
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
	ld          s4, 0(s1)
	li          a1, 3		// 0x3 ASCII \x3
	mv          a0, s4
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .ParseStructMember_label_43

	// *** Basic block 2

	addi        t0, s4, 80
	ld          a0, 16(t0)
	call        NewString

	// *** Basic block 3

	mv          s5, a0
	mv          a0, s4
	call        LexNextToken

	// *** Basic block 4

	j           .ParseStructMember_label_57

	// *** Basic block 5

.ParseStructMember_label_43:
	lla         a1, .str.11
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 6

	mv          a0, s1
	call        SyntaxFakeName

	// *** Basic block 7

	call        NewString

	// *** Basic block 8

	mv          s5, a0

	// *** Basic block 9

.ParseStructMember_label_57:
	ld          s4, 56(s4)
	mv          a2, s4
	mv          a1, x0
	mv          a0, s5
	call        NewStringConstantASTNode

	// *** Basic block 10

	mv          s6, a0
	mv          a4, s6
	mv          a3, s3
	mv          a2, s4
	mv          a1, x0
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
	j           NewBinaryASTNode
.func_end_ParseStructMember:
	.size ParseStructMember, .func_end_ParseStructMember-ParseStructMember

	.local  ParsePostfixExpression
	.type ParsePostfixExpression, @function

ParsePostfixExpression:

	// *** Basic block 0

	.local ParsePrimaryExpression
	.global LexEof
	.global LexMatch
	.local ParseArraySubscript
	.local ParseFunctionCall
	.global NewUnaryASTNode
	.local ParseStructMember
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
	call        ParsePrimaryExpression

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 0(s1)
	lb          t0, 178(s4)
	beqz        t0, .ParsePostfixExpression_label_48

	// *** Basic block 2

	ld          t0, 56(s4)
	ld          t1, 56(s4)
	mv          a0, s3

	// *** Basic block 3

.ParsePostfixExpression_label_45:
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

	// *** Basic block 4

.ParsePostfixExpression_label_48:
	mv          a0, s4
	call        LexEof

	// *** Basic block 5

	not         t0, a0
	beqz        t0, .ParsePostfixExpression_label_183

	// *** Basic block 6

.ParsePostfixExpression_label_54:
	li          t0, 32		// 0x20 ASCII ' '
	mv          a1, t0
	mv          a0, s4
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .ParsePostfixExpression_label_72

	// *** Basic block 8

	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        ParseArraySubscript

	// *** Basic block 9

	mv          s3, a0
	j           .ParsePostfixExpression_label_177

	// *** Basic block 10

.ParsePostfixExpression_label_72:
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	mv          a0, s4
	call        LexMatch

	// *** Basic block 11

	beqz        a0, .ParsePostfixExpression_label_89

	// *** Basic block 12

	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        ParseFunctionCall

	// *** Basic block 13

	mv          s3, a0
	j           .ParsePostfixExpression_label_176

	// *** Basic block 14

.ParsePostfixExpression_label_89:
	li          t0, 42		// 0x2a ASCII '*'
	mv          a1, t0
	mv          a0, s4
	call        LexMatch

	// *** Basic block 15

	beqz        a0, .ParsePostfixExpression_label_110

	// *** Basic block 16

	mv          a3, s3
	mv          a2, t0
	mv          a1, x0
	li          t0, 7		// 0x7 ASCII \x7
	mv          a0, t0
	call        NewUnaryASTNode

	// *** Basic block 17

	mv          s3, a0
	j           .ParsePostfixExpression_label_175

	// *** Basic block 18

.ParsePostfixExpression_label_110:
	li          t0, 35		// 0x23 ASCII '#'
	mv          a1, t0
	mv          a0, s4
	call        LexMatch

	// *** Basic block 19

	beqz        a0, .ParsePostfixExpression_label_130

	// *** Basic block 20

	mv          a3, s3
	mv          a2, t1
	mv          a1, x0
	li          t0, 8		// 0x8 ASCII \x8
	mv          a0, t0
	call        NewUnaryASTNode

	// *** Basic block 21

	mv          s3, a0
	j           .ParsePostfixExpression_label_174

	// *** Basic block 22

.ParsePostfixExpression_label_130:
	li          s5, 19		// 0x13 ASCII \x13
	mv          a1, s5
	mv          a0, s4
	call        LexMatch

	// *** Basic block 23

	beqz        a0, .ParsePostfixExpression_label_150

	// *** Basic block 24

	mv          a3, s2
	mv          a2, s1
	li          t0, 33		// 0x21 ASCII '!'
	mv          a1, t0
	mv          a0, s3
	call        ParseStructMember

	// *** Basic block 25

	mv          s3, a0
	j           .ParsePostfixExpression_label_173

	// *** Basic block 26

.ParsePostfixExpression_label_150:
	li          t0, 11		// 0xb ASCII \xb
	mv          a1, t0
	mv          a0, s4
	call        LexMatch

	// *** Basic block 27

	beqz        a0, .ParsePostfixExpression_label_170

	// *** Basic block 28

	mv          a3, s2
	mv          a2, s1
	mv          a1, s5
	mv          a0, s3
	call        ParseStructMember

	// *** Basic block 29

	mv          s3, a0
	j           .ParsePostfixExpression_label_172

	// *** Basic block 30

.ParsePostfixExpression_label_170:
	j           .ParsePostfixExpression_label_183

	// *** Basic block 31

.ParsePostfixExpression_label_172:

	// *** Basic block 32

.ParsePostfixExpression_label_173:

	// *** Basic block 33

.ParsePostfixExpression_label_174:

	// *** Basic block 34

.ParsePostfixExpression_label_175:

	// *** Basic block 35

.ParsePostfixExpression_label_176:

	// *** Basic block 36

.ParsePostfixExpression_label_177:
	mv          a0, s4
	call        LexEof

	// *** Basic block 37

	not         t0, a0
	bnez        t0, .ParsePostfixExpression_label_54

	// *** Basic block 38

.ParsePostfixExpression_label_183:
	mv          a0, s3
	j           .ParsePostfixExpression_label_45
.func_end_ParsePostfixExpression:
	.size ParsePostfixExpression, .func_end_ParsePostfixExpression-ParsePostfixExpression

	.local  ParsePossiblePreprocessorFunction
	.type ParsePossiblePreprocessorFunction, @function

ParsePossiblePreprocessorFunction:

	// *** Basic block 0

	.global StringEqual
	.global LexNextToken
	.global LexMatch
	.global LexLookingAt
	.global StringInit
	.global SyntaxNeedBracket
	.global PreprocessorFindMacro
	.global NewTypeRecord
	.global NewIntConstantASTNode
	.global PreprocessorParseIncludeFilename
	.global PreprocessorHasIncludeNext
	.global PreprocessorHasInclude
	addi sp, sp, -176
	// Saved return address (offset 168) and frame pointer (offset 160)
	sd ra, 168(sp)
	sd s0, 160(sp)
	addi s0, sp, 176
	// Local vars at offset -112(s0)
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
	mv          s2, a1
	ld          s3, 0(s1)
	addi        a0, s3, 80
	lla         a1, .str.12
	call        StringEqual

	// *** Basic block 1

	beqz        a0, .ParsePossiblePreprocessorFunction_label_149

	// *** Basic block 2

	mv          a0, s3
	call        LexNextToken

	// *** Basic block 3

	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 4

	beqz        a0, .ParsePossiblePreprocessorFunction_label_92

	// *** Basic block 5

	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 6

	beqz        a0, .ParsePossiblePreprocessorFunction_label_75

	// *** Basic block 7

	addi        a0, s0, -112
	ld          a1, 16(a0)
	call        StringInit

	// *** Basic block 8

	mv          a0, s3
	call        LexNextToken

	// *** Basic block 9

	j           .ParsePossiblePreprocessorFunction_label_81

	// *** Basic block 10

.ParsePossiblePreprocessorFunction_label_75:
	addi        a0, s0, -112
	mv          a1, x0
	call        StringInit

	// *** Basic block 11

.ParsePossiblePreprocessorFunction_label_81:
	mv          a2, s2
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 12

	j           .ParsePossiblePreprocessorFunction_label_110

	// *** Basic block 13

.ParsePossiblePreprocessorFunction_label_92:
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 14

	beqz        a0, .ParsePossiblePreprocessorFunction_label_109

	// *** Basic block 15

	addi        a0, s0, -112
	ld          a1, 16(a0)
	call        StringInit

	// *** Basic block 16

	mv          a0, s3
	call        LexNextToken

	// *** Basic block 17

.ParsePossiblePreprocessorFunction_label_109:

	// *** Basic block 18

.ParsePossiblePreprocessorFunction_label_110:
	ld          a0, 64(s3)
	addi        a1, s0, -112
	call        PreprocessorFindMacro

	// *** Basic block 19

	mv          s4, a0
	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 20

	mv          s5, a0
	bne         s4, x0, .ParsePossiblePreprocessorFunction_label_133

	// *** Basic block 21

	mv          s6, x0
	j           .ParsePossiblePreprocessorFunction_label_135

	// *** Basic block 22

.ParsePossiblePreprocessorFunction_label_133:
	li          s6, 1		// 0x1 ASCII \x1

	// *** Basic block 23

.ParsePossiblePreprocessorFunction_label_135:
	ld          a2, 56(s3)
	mv          a1, s5
	mv          a0, s6
	call        NewIntConstantASTNode

	// *** Basic block 24


	// *** Basic block 25

.ParsePossiblePreprocessorFunction_label_146:
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

	// *** Basic block 26

.ParsePossiblePreprocessorFunction_label_149:
	ld          s3, 0(s1)
	addi        a0, s3, 80
	lla         a1, .str.13
	call        StringEqual

	// *** Basic block 27

	beqz        a0, .ParsePossiblePreprocessorFunction_label_206

	// *** Basic block 28

	mv          a0, s3
	call        LexNextToken

	// *** Basic block 29

	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 30

	beqz        a0, .ParsePossiblePreprocessorFunction_label_204

	// *** Basic block 31

	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 32

	beqz        a0, .ParsePossiblePreprocessorFunction_label_178

	// *** Basic block 33

	mv          a0, s3
	call        LexNextToken

	// *** Basic block 34

.ParsePossiblePreprocessorFunction_label_178:
	mv          a2, s2
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 35

	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 36

	ld          a2, 56(s3)
	mv          a1, a0
	mv          a0, x0
	call        NewIntConstantASTNode

	// *** Basic block 37

	j           .ParsePossiblePreprocessorFunction_label_146

	// *** Basic block 38

.ParsePossiblePreprocessorFunction_label_204:
	j           .ParsePossiblePreprocessorFunction_label_321

	// *** Basic block 39

.ParsePossiblePreprocessorFunction_label_206:
	ld          t0, 0(s1)
	addi        a0, t0, 80
	lla         a1, .str.14
	call        StringEqual

	// *** Basic block 40

	mv          s3, a0
	bnez        a0, .ParsePossiblePreprocessorFunction_label_225

	// *** Basic block 41

	ld          t0, 0(s1)
	addi        a0, t0, 80
	lla         a1, .str.15
	call        StringEqual

	// *** Basic block 42

	mv          s3, a0

	// *** Basic block 43

.ParsePossiblePreprocessorFunction_label_225:
	beqz        s3, .ParsePossiblePreprocessorFunction_label_320

	// *** Basic block 44

	ld          s6, 0(s1)
	addi        a0, s6, 80
	lla         a1, .str.16
	call        StringEqual

	// *** Basic block 45

	mv          s7, a0
	mv          a0, s6
	call        LexNextToken

	// *** Basic block 46

	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	mv          a0, s6
	call        LexLookingAt

	// *** Basic block 47

	beqz        a0, .ParsePossiblePreprocessorFunction_label_319

	// *** Basic block 48

	sb          x0, -72(s0)
	mv          s8, x0
	ld          a0, 64(s6)
	ld          t0, 0(s1)
	addi        a1, t0, 8
	ld          s6, 0(s1)
	addi        a2, s6, 48
	addi        a3, s0, -64
	addi        a4, s0, -72
	call        PreprocessorParseIncludeFilename

	// *** Basic block 49

	beqz        a0, .ParsePossiblePreprocessorFunction_label_290

	// *** Basic block 50

	beqz        s7, .ParsePossiblePreprocessorFunction_label_279

	// *** Basic block 51

	ld          a0, 64(s6)
	addi        a1, s0, -64
	lb          a2, -72(s0)
	call        PreprocessorHasIncludeNext

	// *** Basic block 52

	mv          s8, a0
	j           .ParsePossiblePreprocessorFunction_label_289

	// *** Basic block 53

.ParsePossiblePreprocessorFunction_label_279:
	ld          a0, 64(s6)
	addi        a1, s0, -64
	lb          a2, -72(s0)
	call        PreprocessorHasInclude

	// *** Basic block 54

	mv          s8, a0

	// *** Basic block 55

.ParsePossiblePreprocessorFunction_label_289:

	// *** Basic block 56

.ParsePossiblePreprocessorFunction_label_290:
	mv          a0, s6
	call        LexNextToken

	// *** Basic block 57

	mv          a2, s2
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 58

	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 59

	ld          a2, 56(s6)
	mv          a1, a0
	mv          a0, s8
	call        NewIntConstantASTNode

	// *** Basic block 60

	j           .ParsePossiblePreprocessorFunction_label_146

	// *** Basic block 61

.ParsePossiblePreprocessorFunction_label_319:

	// *** Basic block 62

.ParsePossiblePreprocessorFunction_label_320:

	// *** Basic block 63

.ParsePossiblePreprocessorFunction_label_321:

	// *** Basic block 64

.ParsePossiblePreprocessorFunction_label_322:
	mv          a0, x0
	j           .ParsePossiblePreprocessorFunction_label_146
.func_end_ParsePossiblePreprocessorFunction:
	.size ParsePossiblePreprocessorFunction, .func_end_ParsePossiblePreprocessorFunction-ParsePossiblePreprocessorFunction

	.local  CloneVLASize
	.type CloneVLASize, @function

CloneVLASize:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated

	// *** Basic block 1

.CloneVLASize_label_7:
	ret         
.func_end_CloneVLASize:
	.size CloneVLASize, .func_end_CloneVLASize-CloneVLASize

	.local  GetSizeofVLA
	.type GetSizeofVLA, @function

GetSizeofVLA:

	// *** Basic block 0

	.global ASTNodeClone
	.local CloneVLASize
	.global NewIntConstantASTNode
	.global NewTypeRecord
	.global NewBinaryASTNode
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
	ld          a0, 32(s1)
	mv          a3, x0
	mv          a2, x0
	la          t0, CloneVLASize
	mv          a1, t0
	call        ASTNodeClone

	// *** Basic block 1

	mv          s3, a0
	sd          s2, 40(s3)
	ld          s4, 24(s1)

	// *** Basic block 2

.GetSizeofVLA_label_46:
	mv          s1, s4
	lw          s6, 16(s1)
	addi        t0, s6, -2
	seqz        s5, t0
	li          t0, 2		// 0x2 ASCII \x2
	beq         s6, t0, .GetSizeofVLA_label_61

	// *** Basic block 3

	addi        t0, s6, -1
	seqz        s5, t0

	// *** Basic block 4

.GetSizeofVLA_label_61:
	beqz        s5, .GetSizeofVLA_label_68

	// *** Basic block 5

	addi        t0, s1, 32
	lb          t0, 16(t0)
	slli        t0, t0, 61
	srai        s5, t0, 63

	// *** Basic block 6

.GetSizeofVLA_label_68:

	// *** Basic block 7

.GetSizeofVLA_label_70:
	beqz        s5, .GetSizeofVLA_label_88

	// *** Basic block 8

	j           .GetSizeofVLA_label_73

	// *** Basic block 9

.GetSizeofVLA_label_73:
	ld          a0, 32(s4)
	mv          a3, x0
	mv          a2, x0
	la          t0, CloneVLASize
	mv          a1, t0
	call        ASTNodeClone

	// *** Basic block 10

	mv          s5, a0
	j           .GetSizeofVLA_label_105

	// *** Basic block 11

.GetSizeofVLA_label_88:
	lw          s6, 20(s4)
	mv          a1, x0
	li          t0, 16392		// 0x4008
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 12

	mv          a2, s2
	mv          a1, a0
	mv          a0, s6
	call        NewIntConstantASTNode

	// *** Basic block 13

	mv          s5, a0

	// *** Basic block 14

.GetSizeofVLA_label_105:
	mv          a4, s5
	mv          a3, s3
	mv          a2, s2
	mv          a1, s4
	li          t0, 68		// 0x44 ASCII 'D'
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 15

	mv          s3, a0
	ld          s4, 24(s4)

	// *** Basic block 16

.GetSizeofVLA_label_122:
	sub         t1, s4, x0
	snez        t0, t1
	beq         s4, x0, .GetSizeofVLA_label_137

	// *** Basic block 17

	mv          s6, s4
	lw          t1, 16(s6)
	addi        t1, t1, -2
	seqz        s4, t1

	// *** Basic block 18

.GetSizeofVLA_label_134:
	mv          t0, s4
	j           .GetSizeofVLA_label_137

	// *** Basic block 19

.GetSizeofVLA_label_137:
	bnez        t0, .GetSizeofVLA_label_46

	// *** Basic block 20

.GetSizeofVLA_label_139:
	mv          a0, s3

	// *** Basic block 21

.GetSizeofVLA_label_142:
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
.func_end_GetSizeofVLA:
	.size GetSizeofVLA, .func_end_GetSizeofVLA-GetSizeofVLA

	.local  ParseSizeof
	.type ParseSizeof, @function

ParseSizeof:

	// *** Basic block 0

	.global LexMatch
	.global SyntaxLookingAtType
	.global SyntaxError
	.global TypeParserInit
	.global TypeParserParseType
	.global TypeParserParseDeclarator
	.local GetSizeofVLA
	.global SymbolDelete
	.global printf
	.global abort
	.global NewSizeofASTNodeWithKnownSize
	.local ParseUnaryExpression
	.global NewSizeofASTNodeWithExpression
	.global SyntaxNeedBracket
	addi sp, sp, -176
	// Saved return address (offset 168) and frame pointer (offset 160)
	sd ra, 168(sp)
	sd s0, 160(sp)
	addi s0, sp, 176
	// Local vars at offset -96(s0)
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
	ld          s3, 0(s1)
	li          a1, 29		// 0x1d ASCII \x1d
	mv          a0, s3
	call        LexMatch

	// *** Basic block 1

	mv          s4, a0
	mv          s5, x0
	mv          s6, x0
	mv          a0, s1
	call        SyntaxLookingAtType

	// *** Basic block 2

	beqz        a0, .ParseSizeof_label_67

	// *** Basic block 3

	not         t0, s4
	beqz        t0, .ParseSizeof_label_65

	// *** Basic block 4

	lla         a1, .str.17
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 5

.ParseSizeof_label_65:
	li          s6, 1		// 0x1 ASCII \x1

	// *** Basic block 6

.ParseSizeof_label_67:
	beqz        s6, .ParseSizeof_label_180

	// *** Basic block 7

	addi        a0, s0, -96
	lw          a4, 152(s1)
	mv          a3, x0
	mv          a2, s1
	mv          a1, s3
	call        TypeParserInit

	// *** Basic block 8

	addi        a0, s0, -96
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	call        TypeParserParseType

	// *** Basic block 9

	mv          s6, a0
	li          s7, -1		// 0xffffffffffffffff
	addi        a0, s0, -96
	mv          a1, s6
	call        TypeParserParseDeclarator

	// *** Basic block 10

	mv          s8, a0
	beq         s8, x0, .ParseSizeof_label_150

	// *** Basic block 11

	ld          s6, 40(s8)
	lw          s10, 16(s6)
	addi        t0, s10, -2
	seqz        s9, t0
	li          t0, 2		// 0x2 ASCII \x2
	beq         s10, t0, .ParseSizeof_label_120

	// *** Basic block 12

	addi        t0, s10, -1
	seqz        s9, t0

	// *** Basic block 13

.ParseSizeof_label_120:
	beqz        s9, .ParseSizeof_label_127

	// *** Basic block 14

	addi        t0, s6, 32
	lb          t0, 16(t0)
	slli        t0, t0, 61
	srai        s9, t0, 63

	// *** Basic block 15

.ParseSizeof_label_127:

	// *** Basic block 16

.ParseSizeof_label_129:
	beqz        s9, .ParseSizeof_label_144

	// *** Basic block 17

	j           .ParseSizeof_label_132

	// *** Basic block 18

.ParseSizeof_label_132:
	ld          a1, 56(s3)
	mv          a0, s6
	call        GetSizeofVLA

	// *** Basic block 19

	mv          s5, a0
	mv          a0, s8
	call        SymbolDelete

	// *** Basic block 20

	j           .ParseSizeof_label_196

	// *** Basic block 21

.ParseSizeof_label_144:
	lw          s7, 20(s6)
	mv          a0, s8
	call        SymbolDelete

	// *** Basic block 22

.ParseSizeof_label_150:
	li          t0, -1		// 0xffffffffffffffff
	beq         s7, t0, .ParseSizeof_label_156

	// *** Basic block 23

	j           .ParseSizeof_label_171

	// *** Basic block 24

.ParseSizeof_label_156:
	lla         a0, .str.18
	lla         a1, .str.19
	lla         a3, .str.20
	li          t0, 549		// 0x225
	mv          a2, t0
	call        printf

	// *** Basic block 25

	call        abort

	// *** Basic block 26

.ParseSizeof_label_171:
	ld          a1, 56(s3)
	mv          a0, s7
	call        NewSizeofASTNodeWithKnownSize

	// *** Basic block 27

	mv          s5, a0
	j           .ParseSizeof_label_195

	// *** Basic block 28

.ParseSizeof_label_180:
	mv          a1, s2
	mv          a0, s1
	call        ParseUnaryExpression

	// *** Basic block 29

	mv          s7, a0
	ld          a1, 56(s3)
	mv          a0, s7
	call        NewSizeofASTNodeWithExpression

	// *** Basic block 30

	mv          s5, a0

	// *** Basic block 31

.ParseSizeof_label_195:

	// *** Basic block 32

.ParseSizeof_label_196:
	beqz        s4, .ParseSizeof_label_206

	// *** Basic block 33

	mv          a2, s2
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 34

.ParseSizeof_label_206:
	mv          a0, s5

	// *** Basic block 35

.ParseSizeof_label_209:
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
.func_end_ParseSizeof:
	.size ParseSizeof, .func_end_ParseSizeof-ParseSizeof

	.local  ParseUnaryExpression
	.type ParseUnaryExpression, @function

ParseUnaryExpression:

	// *** Basic block 0

	.global LexLookingAt
	.local ParsePossiblePreprocessorFunction
	.global LexMatch
	.local ParseCastExpression
	.global NewUnaryASTNode
	.local ParseUnaryExpression
	.local ParseSizeof
	.local ParsePostfixExpression
	sd          a0, -0(s0)	// Spilled @62
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
	ld          s3, 0(s1)
	lb          t0, 176(s3)
	beqz        t0, .ParseUnaryExpression_label_69

	// *** Basic block 1

	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 2

	beqz        a0, .ParseUnaryExpression_label_68

	// *** Basic block 3

	mv          a1, s2
	mv          a0, s1
	call        ParsePossiblePreprocessorFunction

	// *** Basic block 4

	mv          s4, a0
	beq         s4, x0, .ParseUnaryExpression_label_67

	// *** Basic block 5

	mv          a0, s4

	// *** Basic block 6

.ParseUnaryExpression_label_64:
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

	// *** Basic block 7

.ParseUnaryExpression_label_67:

	// *** Basic block 8

.ParseUnaryExpression_label_68:

	// *** Basic block 9

.ParseUnaryExpression_label_69:
	li          t0, 40		// 0x28 ASCII '('
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 10

	beqz        a0, .ParseUnaryExpression_label_101

	// *** Basic block 11

	mv          a1, s2
	mv          a0, s1
	call        ParseCastExpression

	// *** Basic block 12

	mv          s5, a0
	ld          a2, 56(s3)
	mv          a3, s5
	mv          a1, x0
	li          t0, 10		// 0xa ASCII \xa
	mv          a0, t0
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
	j           NewUnaryASTNode

	// *** Basic block 14

.ParseUnaryExpression_label_101:
	li          t0, 33		// 0x21 ASCII '!'
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 15

	beqz        a0, .ParseUnaryExpression_label_131

	// *** Basic block 16

	mv          a1, s2
	mv          a0, s1
	call        ParseCastExpression

	// *** Basic block 17

	mv          s6, a0
	ld          a2, 56(s3)
	mv          a3, s6
	mv          a1, x0
	li          t0, 9		// 0x9 ASCII \x9
	mv          a0, t0
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
	j           NewUnaryASTNode

	// *** Basic block 19

.ParseUnaryExpression_label_131:
	li          t0, 9		// 0x9 ASCII \x9
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 20

	beqz        a0, .ParseUnaryExpression_label_177

	// *** Basic block 21

	mv          a1, s2
	mv          a0, s1
	call        ParseCastExpression

	// *** Basic block 22

	mv          s7, a0
	lw          t0, 0(s7)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .ParseUnaryExpression_label_161

	// *** Basic block 23

	mv          s8, s7
	ld          t0, 56(s8)
	lb          t1, 56(t0)
	andi        t1, t1, -65
	ori         t1, t1, 64
	sb          t1, 56(t0)

	// *** Basic block 24

.ParseUnaryExpression_label_161:
	ld          a2, 56(s3)
	mv          a3, s7
	mv          a1, x0
	li          t0, 12		// 0xc ASCII \xc
	mv          a0, t0
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
	j           NewUnaryASTNode

	// *** Basic block 26

.ParseUnaryExpression_label_177:
	li          t0, 52		// 0x34 ASCII '4'
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 27

	beqz        a0, .ParseUnaryExpression_label_207

	// *** Basic block 28

	mv          a1, s2
	mv          a0, s1
	call        ParseCastExpression

	// *** Basic block 29

	mv          s9, a0
	ld          a2, 56(s3)
	mv          a3, s9
	mv          a1, x0
	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
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
	j           NewUnaryASTNode

	// *** Basic block 31

.ParseUnaryExpression_label_207:
	li          t0, 54		// 0x36 ASCII '6'
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 32

	beqz        a0, .ParseUnaryExpression_label_237

	// *** Basic block 33

	mv          a1, s2
	mv          a0, s1
	call        ParseCastExpression

	// *** Basic block 34

	mv          s10, a0
	ld          a2, 56(s3)
	mv          a3, s10
	mv          a1, x0
	li          t0, 71		// 0x47 ASCII 'G'
	mv          a0, t0
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
	j           NewUnaryASTNode

	// *** Basic block 36

.ParseUnaryExpression_label_237:
	li          t0, 13		// 0xd ASCII \xd
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 37

	beqz        a0, .ParseUnaryExpression_label_267

	// *** Basic block 38

	mv          a1, s2
	mv          a0, s1
	call        ParseCastExpression

	// *** Basic block 39

	mv          s11, a0
	ld          a2, 56(s3)
	mv          a3, s11
	mv          a1, x0
	li          t0, 21		// 0x15 ASCII \x15
	mv          a0, t0
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
	j           NewUnaryASTNode

	// *** Basic block 41

.ParseUnaryExpression_label_267:
	li          t0, 42		// 0x2a ASCII '*'
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 42

	beqz        a0, .ParseUnaryExpression_label_297

	// *** Basic block 43

	mv          a1, s2
	mv          a0, s1
	call        ParseUnaryExpression

	// *** Basic block 44

	sd          a0, -24(s0)	// Spilled @280
	ld          a2, 56(s3)
	mv          a3, a0
	mv          a1, x0
	li          t0, 60		// 0x3c ASCII '<'
	mv          a0, t0
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
	j           NewUnaryASTNode

	// *** Basic block 46

.ParseUnaryExpression_label_297:
	li          t0, 35		// 0x23 ASCII '#'
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 47

	beqz        a0, .ParseUnaryExpression_label_327

	// *** Basic block 48

	mv          a1, s2
	mv          a0, s1
	call        ParseUnaryExpression

	// *** Basic block 49

	ld          a2, 56(s3)
	mv          a3, a0
	mv          a1, x0
	li          t0, 53		// 0x35 ASCII '5'
	mv          a0, t0
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
	j           NewUnaryASTNode

	// *** Basic block 51

.ParseUnaryExpression_label_327:
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 52

	beqz        a0, .ParseUnaryExpression_label_344

	// *** Basic block 53

	mv          a1, s2
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
	j           ParseSizeof

	// *** Basic block 55

.ParseUnaryExpression_label_344:
	mv          a1, s2
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
	j           ParsePostfixExpression
.func_end_ParseUnaryExpression:
	.size ParseUnaryExpression, .func_end_ParseUnaryExpression-ParseUnaryExpression

	.local  ParseCastExpression
	.type ParseCastExpression, @function

ParseCastExpression:

	// *** Basic block 0

	.global LexMatch
	.global SyntaxLookingAtType
	.global TypeParserInit
	.global TypeParserParseType
	.global SyntaxError
	.global NewTypeRecord
	.global TypeParserParseDeclarator
	.global SyntaxNeedBracket
	.local ParseCastExpression
	.global NewCastASTNode
	.global SymbolDelete
	.local ParsePostfixExpression
	.local ParseUnaryExpression
	addi sp, sp, -160
	// Saved return address (offset 152) and frame pointer (offset 144)
	sd ra, 152(sp)
	sd s0, 144(sp)
	addi s0, sp, 160
	// Local vars at offset -96(s0)
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
	ld          s4, 0(s1)
	lb          t0, 176(s4)
	not         s3, t0
	beqz        s3, .ParseCastExpression_label_44

	// *** Basic block 1

	lb          t0, 178(s4)
	not         s3, t0

	// *** Basic block 2

.ParseCastExpression_label_44:
	beqz        s3, .ParseCastExpression_label_53

	// *** Basic block 3

	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	mv          a0, s4
	call        LexMatch

	// *** Basic block 4

	mv          s3, a0

	// *** Basic block 5

.ParseCastExpression_label_53:
	beqz        s3, .ParseCastExpression_label_161

	// *** Basic block 6

	mv          a0, s1
	call        SyntaxLookingAtType

	// *** Basic block 7

	beqz        a0, .ParseCastExpression_label_149

	// *** Basic block 8

	addi        a0, s0, -96
	lw          a4, 152(s1)
	mv          a3, x0
	mv          a2, s1
	mv          a1, s4
	call        TypeParserInit

	// *** Basic block 9

	addi        a0, s0, -96
	mv          a1, x0
	call        TypeParserParseType

	// *** Basic block 10

	mv          s5, a0
	mv          s6, x0
	bne         s5, x0, .ParseCastExpression_label_102

	// *** Basic block 11

	lla         a1, .str.21
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 12

	mv          a1, x0
	li          t0, 32770		// 0x8002
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 13

	mv          s5, a0
	j           .ParseCastExpression_label_111

	// *** Basic block 14

.ParseCastExpression_label_102:
	addi        a0, s0, -96
	mv          a1, s5
	call        TypeParserParseDeclarator

	// *** Basic block 15

	mv          s6, a0
	ld          s5, 40(s6)

	// *** Basic block 16

.ParseCastExpression_label_111:
	mv          a2, s2
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 17

	mv          a1, s2
	mv          a0, s1
	call        ParseCastExpression

	// *** Basic block 18

	mv          s7, a0
	ld          a1, 56(s4)
	mv          a2, s7
	mv          a0, s5
	call        NewCastASTNode

	// *** Basic block 19

	mv          s4, a0
	beq         s6, x0, .ParseCastExpression_label_143

	// *** Basic block 20

	mv          a0, s6
	call        SymbolDelete

	// *** Basic block 21

.ParseCastExpression_label_143:
	mv          a0, s4

	// *** Basic block 22

.ParseCastExpression_label_146:
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

	// *** Basic block 23

.ParseCastExpression_label_149:
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 68(s1)
	mv          a1, s2
	mv          a0, s1
	call        ParsePostfixExpression

	// *** Basic block 24

	j           .ParseCastExpression_label_146

	// *** Basic block 25

.ParseCastExpression_label_161:
	mv          a1, s2
	mv          a0, s1
	call        ParseUnaryExpression

	// *** Basic block 26

	j           .ParseCastExpression_label_146
.func_end_ParseCastExpression:
	.size ParseCastExpression, .func_end_ParseCastExpression-ParseCastExpression

	.local  ParseMultiplicativeExpression
	.type ParseMultiplicativeExpression, @function

ParseMultiplicativeExpression:

	// *** Basic block 0

	.local ParseCastExpression
	.global LexMatch
	.global NewBinaryASTNode
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
	mv          s2, a1
	call        ParseCastExpression

	// *** Basic block 1

	ld          s3, 0(s1)
	ld          s4, 56(s3)
	ld          s5, 56(s3)
	ld          s6, 56(s3)
	mv          s7, a0

	// *** Basic block 2

.ParseMultiplicativeExpression_label_33:
	li          t0, 52		// 0x34 ASCII '4'
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .ParseMultiplicativeExpression_label_65

	// *** Basic block 4

	mv          a1, s2
	mv          a0, s1
	call        ParseCastExpression

	// *** Basic block 5

	mv          s8, a0
	mv          a4, s8
	mv          a3, s7
	mv          a2, s4
	mv          a1, x0
	li          t0, 68		// 0x44 ASCII 'D'
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 6

	mv          s7, a0
	j           .ParseMultiplicativeExpression_label_127

	// *** Basic block 7

.ParseMultiplicativeExpression_label_65:
	li          t0, 50		// 0x32 ASCII '2'
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 8

	beqz        a0, .ParseMultiplicativeExpression_label_94

	// *** Basic block 9

	mv          a1, s2
	mv          a0, s1
	call        ParseCastExpression

	// *** Basic block 10

	mv          s4, a0
	mv          a4, s4
	mv          a3, s7
	mv          a2, s5
	mv          a1, x0
	li          t0, 66		// 0x42 ASCII 'B'
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 11

	mv          s7, a0
	j           .ParseMultiplicativeExpression_label_126

	// *** Basic block 12

.ParseMultiplicativeExpression_label_94:
	li          t0, 38		// 0x26 ASCII '&'
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 13

	beqz        a0, .ParseMultiplicativeExpression_label_123

	// *** Basic block 14

	mv          a1, s2
	mv          a0, s1
	call        ParseCastExpression

	// *** Basic block 15

	mv          s3, a0
	mv          a4, s3
	mv          a3, s7
	mv          a2, s6
	mv          a1, x0
	li          t0, 56		// 0x38 ASCII '8'
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 16

	mv          s7, a0
	j           .ParseMultiplicativeExpression_label_125

	// *** Basic block 17

.ParseMultiplicativeExpression_label_123:
	j           .ParseMultiplicativeExpression_label_130

	// *** Basic block 18

.ParseMultiplicativeExpression_label_125:

	// *** Basic block 19

.ParseMultiplicativeExpression_label_126:

	// *** Basic block 20

.ParseMultiplicativeExpression_label_127:

	// *** Basic block 21

.ParseMultiplicativeExpression_label_128:
	j           .ParseMultiplicativeExpression_label_33

	// *** Basic block 22

.ParseMultiplicativeExpression_label_130:
	mv          a0, s7

	// *** Basic block 23

.ParseMultiplicativeExpression_label_133:
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
.func_end_ParseMultiplicativeExpression:
	.size ParseMultiplicativeExpression, .func_end_ParseMultiplicativeExpression-ParseMultiplicativeExpression

	.local  ParseAdditiveExpression
	.type ParseAdditiveExpression, @function

ParseAdditiveExpression:

	// *** Basic block 0

	.local ParseMultiplicativeExpression
	.global LexMatch
	.global NewBinaryASTNode
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
	call        ParseMultiplicativeExpression

	// *** Basic block 1

	ld          s3, 0(s1)
	ld          s4, 56(s3)
	ld          s5, 56(s3)
	mv          s6, a0

	// *** Basic block 2

.ParseAdditiveExpression_label_30:
	li          t0, 40		// 0x28 ASCII '('
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .ParseAdditiveExpression_label_62

	// *** Basic block 4

	mv          a1, s2
	mv          a0, s1
	call        ParseMultiplicativeExpression

	// *** Basic block 5

	mv          s7, a0
	mv          a4, s7
	mv          a3, s6
	mv          a2, s4
	mv          a1, x0
	li          t0, 58		// 0x3a ASCII ':'
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 6

	mv          s6, a0
	j           .ParseAdditiveExpression_label_94

	// *** Basic block 7

.ParseAdditiveExpression_label_62:
	li          t0, 33		// 0x21 ASCII '!'
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 8

	beqz        a0, .ParseAdditiveExpression_label_91

	// *** Basic block 9

	mv          a1, s2
	mv          a0, s1
	call        ParseMultiplicativeExpression

	// *** Basic block 10

	mv          s3, a0
	mv          a4, s3
	mv          a3, s6
	mv          a2, s5
	mv          a1, x0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 11

	mv          s6, a0
	j           .ParseAdditiveExpression_label_93

	// *** Basic block 12

.ParseAdditiveExpression_label_91:
	j           .ParseAdditiveExpression_label_97

	// *** Basic block 13

.ParseAdditiveExpression_label_93:

	// *** Basic block 14

.ParseAdditiveExpression_label_94:

	// *** Basic block 15

.ParseAdditiveExpression_label_95:
	j           .ParseAdditiveExpression_label_30

	// *** Basic block 16

.ParseAdditiveExpression_label_97:
	mv          a0, s6

	// *** Basic block 17

.ParseAdditiveExpression_label_100:
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
.func_end_ParseAdditiveExpression:
	.size ParseAdditiveExpression, .func_end_ParseAdditiveExpression-ParseAdditiveExpression

	.local  ParseShiftExpression
	.type ParseShiftExpression, @function

ParseShiftExpression:

	// *** Basic block 0

	.local ParseAdditiveExpression
	.global LexMatch
	.global NewBinaryASTNode
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
	call        ParseAdditiveExpression

	// *** Basic block 1

	ld          s3, 0(s1)
	ld          s4, 56(s3)
	ld          s5, 56(s3)
	mv          s6, a0

	// *** Basic block 2

.ParseShiftExpression_label_30:
	li          t0, 30		// 0x1e ASCII \x1e
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .ParseShiftExpression_label_62

	// *** Basic block 4

	mv          a1, s2
	mv          a0, s1
	call        ParseAdditiveExpression

	// *** Basic block 5

	mv          s7, a0
	mv          a4, s7
	mv          a3, s6
	mv          a2, s4
	mv          a1, x0
	li          t0, 48		// 0x30 ASCII '0'
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 6

	mv          s6, a0
	j           .ParseShiftExpression_label_94

	// *** Basic block 7

.ParseShiftExpression_label_62:
	li          t0, 46		// 0x2e ASCII '.'
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 8

	beqz        a0, .ParseShiftExpression_label_91

	// *** Basic block 9

	mv          a1, s2
	mv          a0, s1
	call        ParseAdditiveExpression

	// *** Basic block 10

	mv          s3, a0
	mv          a4, s3
	mv          a3, s6
	mv          a2, s5
	mv          a1, x0
	li          t0, 63		// 0x3f ASCII '?'
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 11

	mv          s6, a0
	j           .ParseShiftExpression_label_93

	// *** Basic block 12

.ParseShiftExpression_label_91:
	j           .ParseShiftExpression_label_97

	// *** Basic block 13

.ParseShiftExpression_label_93:

	// *** Basic block 14

.ParseShiftExpression_label_94:

	// *** Basic block 15

.ParseShiftExpression_label_95:
	j           .ParseShiftExpression_label_30

	// *** Basic block 16

.ParseShiftExpression_label_97:
	mv          a0, s6

	// *** Basic block 17

.ParseShiftExpression_label_100:
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
.func_end_ParseShiftExpression:
	.size ParseShiftExpression, .func_end_ParseShiftExpression-ParseShiftExpression

	.local  ParseRelationalExpression
	.type ParseRelationalExpression, @function

ParseRelationalExpression:

	// *** Basic block 0

	.local ParseShiftExpression
	.global LexMatch
	.global NewBinaryASTNode
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
	call        ParseShiftExpression

	// *** Basic block 1

	ld          s3, 0(s1)
	ld          s4, 56(s3)
	ld          s5, 56(s3)
	ld          s6, 56(s3)
	ld          s7, 56(s3)
	mv          s8, a0

	// *** Basic block 2

.ParseRelationalExpression_label_38:
	li          t0, 25		// 0x19 ASCII \x19
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .ParseRelationalExpression_label_70

	// *** Basic block 4

	mv          a1, s2
	mv          a0, s1
	call        ParseShiftExpression

	// *** Basic block 5

	mv          s9, a0
	mv          a4, s9
	mv          a3, s8
	mv          a2, s4
	mv          a1, x0
	li          t0, 42		// 0x2a ASCII '*'
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 6

	mv          s8, a0
	j           .ParseRelationalExpression_label_162

	// *** Basic block 7

.ParseRelationalExpression_label_70:
	li          t0, 26		// 0x1a ASCII \x1a
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 8

	beqz        a0, .ParseRelationalExpression_label_99

	// *** Basic block 9

	mv          a1, s2
	mv          a0, s1
	call        ParseShiftExpression

	// *** Basic block 10

	mv          s4, a0
	mv          a4, s4
	mv          a3, s8
	mv          a2, s5
	mv          a1, x0
	li          t0, 43		// 0x2b ASCII '+'
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 11

	mv          s8, a0
	j           .ParseRelationalExpression_label_161

	// *** Basic block 12

.ParseRelationalExpression_label_99:
	li          t0, 22		// 0x16 ASCII \x16
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 13

	beqz        a0, .ParseRelationalExpression_label_128

	// *** Basic block 14

	mv          a1, s2
	mv          a0, s1
	call        ParseShiftExpression

	// *** Basic block 15

	mv          s5, a0
	mv          a4, s5
	mv          a3, s8
	mv          a2, s6
	mv          a1, x0
	li          t0, 37		// 0x25 ASCII '%'
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 16

	mv          s8, a0
	j           .ParseRelationalExpression_label_160

	// *** Basic block 17

.ParseRelationalExpression_label_128:
	li          t0, 23		// 0x17 ASCII \x17
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 18

	beqz        a0, .ParseRelationalExpression_label_157

	// *** Basic block 19

	mv          a1, s2
	mv          a0, s1
	call        ParseShiftExpression

	// *** Basic block 20

	mv          s3, a0
	mv          a4, s3
	mv          a3, s8
	mv          a2, s7
	mv          a1, x0
	li          t0, 38		// 0x26 ASCII '&'
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 21

	mv          s8, a0
	j           .ParseRelationalExpression_label_159

	// *** Basic block 22

.ParseRelationalExpression_label_157:
	j           .ParseRelationalExpression_label_165

	// *** Basic block 23

.ParseRelationalExpression_label_159:

	// *** Basic block 24

.ParseRelationalExpression_label_160:

	// *** Basic block 25

.ParseRelationalExpression_label_161:

	// *** Basic block 26

.ParseRelationalExpression_label_162:

	// *** Basic block 27

.ParseRelationalExpression_label_163:
	j           .ParseRelationalExpression_label_38

	// *** Basic block 28

.ParseRelationalExpression_label_165:
	mv          a0, s8

	// *** Basic block 29

.ParseRelationalExpression_label_168:
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
.func_end_ParseRelationalExpression:
	.size ParseRelationalExpression, .func_end_ParseRelationalExpression-ParseRelationalExpression

	.local  ParseEqualityExpression
	.type ParseEqualityExpression, @function

ParseEqualityExpression:

	// *** Basic block 0

	.local ParseRelationalExpression
	.global LexMatch
	.global NewBinaryASTNode
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
	call        ParseRelationalExpression

	// *** Basic block 1

	ld          s3, 0(s1)
	ld          s4, 56(s3)
	ld          s5, 56(s3)
	mv          s6, a0

	// *** Basic block 2

.ParseEqualityExpression_label_30:
	li          t0, 21		// 0x15 ASCII \x15
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .ParseEqualityExpression_label_62

	// *** Basic block 4

	mv          a1, s2
	mv          a0, s1
	call        ParseRelationalExpression

	// *** Basic block 5

	mv          s7, a0
	mv          a4, s7
	mv          a3, s6
	mv          a2, s4
	mv          a1, x0
	li          t0, 34		// 0x22 ASCII '"'
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 6

	mv          s6, a0
	j           .ParseEqualityExpression_label_94

	// *** Basic block 7

.ParseEqualityExpression_label_62:
	li          t0, 36		// 0x24 ASCII '$'
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 8

	beqz        a0, .ParseEqualityExpression_label_91

	// *** Basic block 9

	mv          a1, s2
	mv          a0, s1
	call        ParseRelationalExpression

	// *** Basic block 10

	mv          s3, a0
	mv          a4, s3
	mv          a3, s6
	mv          a2, s5
	mv          a1, x0
	li          t0, 54		// 0x36 ASCII '6'
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 11

	mv          s6, a0
	j           .ParseEqualityExpression_label_93

	// *** Basic block 12

.ParseEqualityExpression_label_91:
	j           .ParseEqualityExpression_label_97

	// *** Basic block 13

.ParseEqualityExpression_label_93:

	// *** Basic block 14

.ParseEqualityExpression_label_94:

	// *** Basic block 15

.ParseEqualityExpression_label_95:
	j           .ParseEqualityExpression_label_30

	// *** Basic block 16

.ParseEqualityExpression_label_97:
	mv          a0, s6

	// *** Basic block 17

.ParseEqualityExpression_label_100:
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
.func_end_ParseEqualityExpression:
	.size ParseEqualityExpression, .func_end_ParseEqualityExpression-ParseEqualityExpression

	.local  ParseAndExpression
	.type ParseAndExpression, @function

ParseAndExpression:

	// *** Basic block 0

	.local ParseEqualityExpression
	.global LexMatch
	.global NewBinaryASTNode
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
	call        ParseEqualityExpression

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 0(s1)
	li          s5, 9		// 0x9 ASCII \x9
	mv          a1, s5
	mv          a0, s4
	call        LexMatch

	// *** Basic block 2

	ld          s6, 56(s4)
	beqz        a0, .ParseAndExpression_label_64

	// *** Basic block 3

.ParseAndExpression_label_33:
	mv          a1, s2
	mv          a0, s1
	call        ParseEqualityExpression

	// *** Basic block 4

	mv          s7, a0
	mv          a4, s7
	mv          a3, s3
	mv          a2, s6
	mv          a1, x0
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 5

	mv          s3, a0
	mv          a1, s5
	mv          a0, s4
	call        LexMatch

	// *** Basic block 6

	bnez        a0, .ParseAndExpression_label_33

	// *** Basic block 7

.ParseAndExpression_label_64:
	mv          a0, s3

	// *** Basic block 8

.ParseAndExpression_label_67:
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
.func_end_ParseAndExpression:
	.size ParseAndExpression, .func_end_ParseAndExpression-ParseAndExpression

	.local  ParseExclusiveOrExpression
	.type ParseExclusiveOrExpression, @function

ParseExclusiveOrExpression:

	// *** Basic block 0

	.local ParseAndExpression
	.global LexMatch
	.global NewBinaryASTNode
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
	call        ParseAndExpression

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 0(s1)
	li          s5, 15		// 0xf ASCII \xf
	mv          a1, s5
	mv          a0, s4
	call        LexMatch

	// *** Basic block 2

	ld          s6, 56(s4)
	beqz        a0, .ParseExclusiveOrExpression_label_64

	// *** Basic block 3

.ParseExclusiveOrExpression_label_33:
	mv          a1, s2
	mv          a0, s1
	call        ParseAndExpression

	// *** Basic block 4

	mv          s7, a0
	mv          a4, s7
	mv          a3, s3
	mv          a2, s6
	mv          a1, x0
	li          t0, 24		// 0x18 ASCII \x18
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 5

	mv          s3, a0
	mv          a1, s5
	mv          a0, s4
	call        LexMatch

	// *** Basic block 6

	bnez        a0, .ParseExclusiveOrExpression_label_33

	// *** Basic block 7

.ParseExclusiveOrExpression_label_64:
	mv          a0, s3

	// *** Basic block 8

.ParseExclusiveOrExpression_label_67:
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
.func_end_ParseExclusiveOrExpression:
	.size ParseExclusiveOrExpression, .func_end_ParseExclusiveOrExpression-ParseExclusiveOrExpression

	.local  ParseInclusiveOrExpression
	.type ParseInclusiveOrExpression, @function

ParseInclusiveOrExpression:

	// *** Basic block 0

	.local ParseExclusiveOrExpression
	.global LexMatch
	.global NewBinaryASTNode
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
	call        ParseExclusiveOrExpression

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 0(s1)
	li          s5, 14		// 0xe ASCII \xe
	mv          a1, s5
	mv          a0, s4
	call        LexMatch

	// *** Basic block 2

	ld          s6, 56(s4)
	beqz        a0, .ParseInclusiveOrExpression_label_64

	// *** Basic block 3

.ParseInclusiveOrExpression_label_33:
	mv          a1, s2
	mv          a0, s1
	call        ParseExclusiveOrExpression

	// *** Basic block 4

	mv          s7, a0
	mv          a4, s7
	mv          a3, s3
	mv          a2, s6
	mv          a1, x0
	li          t0, 22		// 0x16 ASCII \x16
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 5

	mv          s3, a0
	mv          a1, s5
	mv          a0, s4
	call        LexMatch

	// *** Basic block 6

	bnez        a0, .ParseInclusiveOrExpression_label_33

	// *** Basic block 7

.ParseInclusiveOrExpression_label_64:
	mv          a0, s3

	// *** Basic block 8

.ParseInclusiveOrExpression_label_67:
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
.func_end_ParseInclusiveOrExpression:
	.size ParseInclusiveOrExpression, .func_end_ParseInclusiveOrExpression-ParseInclusiveOrExpression

	.local  ParseLogicalAndExpression
	.type ParseLogicalAndExpression, @function

ParseLogicalAndExpression:

	// *** Basic block 0

	.local ParseInclusiveOrExpression
	.global LexMatch
	.global NewBinaryASTNode
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
	call        ParseInclusiveOrExpression

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 0(s1)
	li          s5, 27		// 0x1b ASCII \x1b
	mv          a1, s5
	mv          a0, s4
	call        LexMatch

	// *** Basic block 2

	ld          s6, 56(s4)
	beqz        a0, .ParseLogicalAndExpression_label_64

	// *** Basic block 3

.ParseLogicalAndExpression_label_33:
	mv          a1, s2
	mv          a0, s1
	call        ParseInclusiveOrExpression

	// *** Basic block 4

	mv          s7, a0
	mv          a4, s7
	mv          a3, s3
	mv          a2, s6
	mv          a1, x0
	li          t0, 44		// 0x2c ASCII ','
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 5

	mv          s3, a0
	mv          a1, s5
	mv          a0, s4
	call        LexMatch

	// *** Basic block 6

	bnez        a0, .ParseLogicalAndExpression_label_33

	// *** Basic block 7

.ParseLogicalAndExpression_label_64:
	mv          a0, s3

	// *** Basic block 8

.ParseLogicalAndExpression_label_67:
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
.func_end_ParseLogicalAndExpression:
	.size ParseLogicalAndExpression, .func_end_ParseLogicalAndExpression-ParseLogicalAndExpression

	.local  ParseLogicalOrExpression
	.type ParseLogicalOrExpression, @function

ParseLogicalOrExpression:

	// *** Basic block 0

	.local ParseLogicalAndExpression
	.global LexMatch
	.global NewBinaryASTNode
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
	call        ParseLogicalAndExpression

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 0(s1)
	li          s5, 28		// 0x1c ASCII \x1c
	mv          a1, s5
	mv          a0, s4
	call        LexMatch

	// *** Basic block 2

	ld          s6, 56(s4)
	beqz        a0, .ParseLogicalOrExpression_label_64

	// *** Basic block 3

.ParseLogicalOrExpression_label_33:
	mv          a1, s2
	mv          a0, s1
	call        ParseLogicalAndExpression

	// *** Basic block 4

	mv          s7, a0
	mv          a4, s7
	mv          a3, s3
	mv          a2, s6
	mv          a1, x0
	li          t0, 45		// 0x2d ASCII '-'
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 5

	mv          s3, a0
	mv          a1, s5
	mv          a0, s4
	call        LexMatch

	// *** Basic block 6

	bnez        a0, .ParseLogicalOrExpression_label_33

	// *** Basic block 7

.ParseLogicalOrExpression_label_64:
	mv          a0, s3

	// *** Basic block 8

.ParseLogicalOrExpression_label_67:
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
.func_end_ParseLogicalOrExpression:
	.size ParseLogicalOrExpression, .func_end_ParseLogicalOrExpression-ParseLogicalOrExpression

	.local  ParseConditionalExpression
	.type ParseConditionalExpression, @function

ParseConditionalExpression:

	// *** Basic block 0

	.local ParseLogicalOrExpression
	.global LexMatch
	.global SyntaxParseExpression
	.local ParseConditionalExpression
	.global NewBinaryASTNode
	.global SyntaxError
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
	call        ParseLogicalOrExpression

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 0(s1)
	li          t0, 43		// 0x2b ASCII '+'
	mv          a1, t0
	mv          a0, s4
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .ParseConditionalExpression_label_99

	// *** Basic block 3

	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseExpression

	// *** Basic block 4

	mv          s5, a0
	mv          s6, x0
	li          t0, 17		// 0x11 ASCII \x11
	mv          a1, t0
	mv          a0, s4
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .ParseConditionalExpression_label_77

	// *** Basic block 6

	mv          a1, s2
	mv          a0, s1
	call        ParseConditionalExpression

	// *** Basic block 7

	mv          s6, a0
	ld          a2, 56(s4)
	mv          a4, s6
	mv          a3, s5
	mv          a1, x0
	li          t0, 28		// 0x1c ASCII \x1c
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 8

	mv          s6, a0
	j           .ParseConditionalExpression_label_84

	// *** Basic block 9

.ParseConditionalExpression_label_77:
	lla         a1, .str.22
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 10

.ParseConditionalExpression_label_84:
	ld          a2, 56(s4)
	mv          a4, s6
	mv          a3, s3
	mv          a1, x0
	li          t0, 61		// 0x3d ASCII '='
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 11

	mv          s3, a0

	// *** Basic block 12

.ParseConditionalExpression_label_99:
	mv          a0, s3

	// *** Basic block 13

.ParseConditionalExpression_label_102:
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
.func_end_ParseConditionalExpression:
	.size ParseConditionalExpression, .func_end_ParseConditionalExpression-ParseConditionalExpression

	.local  AssignASTOpcode
	.type AssignASTOpcode, @function

AssignASTOpcode:

	// *** Basic block 0

	.global printf
	.global abort
	addi sp, sp, -16
	// Saved return address (offset 8) and frame pointer (offset 0)
	sd ra, 8(sp)
	sd s0, 0(sp)
	addi s0, sp, 16
	// Local vars at offset -16(s0)
	// End of stack frame
	mv          t0, a1
	li          t1, 10		// 0xa ASCII \xa
	beq         t0, t1, .AssignASTOpcode_label_144

	// *** Basic block 1

	li          t1, 12		// 0xc ASCII \xc
	beq         t0, t1, .AssignASTOpcode_label_110

	// *** Basic block 2

	li          t1, 16		// 0x10 ASCII \x10
	beq         t0, t1, .AssignASTOpcode_label_152

	// *** Basic block 3

	li          t1, 31		// 0x1f ASCII \x1f
	beq         t0, t1, .AssignASTOpcode_label_136

	// *** Basic block 4

	li          t1, 34		// 0x22 ASCII '"'
	beq         t0, t1, .AssignASTOpcode_label_120

	// *** Basic block 5

	li          t1, 37		// 0x25 ASCII '%'
	beq         t0, t1, .AssignASTOpcode_label_148

	// *** Basic block 6

	li          t1, 39		// 0x27 ASCII '''
	beq         t0, t1, .AssignASTOpcode_label_132

	// *** Basic block 7

	li          t1, 41		// 0x29 ASCII ')'
	beq         t0, t1, .AssignASTOpcode_label_116

	// *** Basic block 8

	li          t1, 47		// 0x2f ASCII '/'
	beq         t0, t1, .AssignASTOpcode_label_140

	// *** Basic block 9

	li          t1, 51		// 0x33 ASCII '3'
	beq         t0, t1, .AssignASTOpcode_label_128

	// *** Basic block 10

	li          t1, 53		// 0x35 ASCII '5'
	beq         t0, t1, .AssignASTOpcode_label_124

	// *** Basic block 11

.AssignASTOpcode_label_88:
	lla         a0, .str.23
	lla         a1, .str.24
	lla         a3, .str.25
	li          t0, 912		// 0x390
	mv          a2, t0
	call        printf

	// *** Basic block 12

	call        abort

	// *** Basic block 13

	mv          a0, x0
	j           .AssignASTOpcode_label_113

	// *** Basic block 14

.AssignASTOpcode_label_110:
	li          a0, 20		// 0x14 ASCII \x14

	// *** Basic block 15

.AssignASTOpcode_label_113:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 16

.AssignASTOpcode_label_116:
	li          a0, 59		// 0x3b ASCII ';'
	j           .AssignASTOpcode_label_113

	// *** Basic block 17

.AssignASTOpcode_label_120:
	li          a0, 52		// 0x34 ASCII '4'
	j           .AssignASTOpcode_label_113

	// *** Basic block 18

.AssignASTOpcode_label_124:
	li          a0, 69		// 0x45 ASCII 'E'
	j           .AssignASTOpcode_label_113

	// *** Basic block 19

.AssignASTOpcode_label_128:
	li          a0, 67		// 0x43 ASCII 'C'
	j           .AssignASTOpcode_label_113

	// *** Basic block 20

.AssignASTOpcode_label_132:
	li          a0, 57		// 0x39 ASCII '9'
	j           .AssignASTOpcode_label_113

	// *** Basic block 21

.AssignASTOpcode_label_136:
	li          a0, 49		// 0x31 ASCII '1'
	j           .AssignASTOpcode_label_113

	// *** Basic block 22

.AssignASTOpcode_label_140:
	li          a0, 64		// 0x40 ASCII '@'
	j           .AssignASTOpcode_label_113

	// *** Basic block 23

.AssignASTOpcode_label_144:
	li          a0, 18		// 0x12 ASCII \x12
	j           .AssignASTOpcode_label_113

	// *** Basic block 24

.AssignASTOpcode_label_148:
	li          a0, 55		// 0x37 ASCII '7'
	j           .AssignASTOpcode_label_113

	// *** Basic block 25

.AssignASTOpcode_label_152:
	li          a0, 25		// 0x19 ASCII \x19
	j           .AssignASTOpcode_label_113
.func_end_AssignASTOpcode:
	.size AssignASTOpcode, .func_end_AssignASTOpcode-AssignASTOpcode

	.local  ParseAssignmentExpression
	.type ParseAssignmentExpression, @function

ParseAssignmentExpression:

	// *** Basic block 0

	.local ParseConditionalExpression
	.global LexNextToken
	.global NewBinaryASTNode
	.local AssignASTOpcode
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
	call        ParseConditionalExpression

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 0(s1)
	lb          t0, 176(s4)
	bnez        t0, .ParseAssignmentExpression_label_81

	// *** Basic block 2

	lw          s5, 72(s4)
	ld          t1, 56(s4)
	lb          t0, 178(s4)

	// *** Basic block 3

.ParseAssignmentExpression_label_81:
	beqz        t0, .ParseAssignmentExpression_label_88

	// *** Basic block 4

	mv          a0, s3

	// *** Basic block 5

.ParseAssignmentExpression_label_85:
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

.ParseAssignmentExpression_label_88:
	mv          s6, x0

	// *** Basic block 7

.ParseAssignmentExpression_label_92:
	li          t0, 10		// 0xa ASCII \xa
	beq         s5, t0, .ParseAssignmentExpression_label_126

	// *** Basic block 8

	li          t0, 12		// 0xc ASCII \xc
	beq         s5, t0, .ParseAssignmentExpression_label_118

	// *** Basic block 9

	li          t0, 16		// 0x10 ASCII \x10
	beq         s5, t0, .ParseAssignmentExpression_label_128

	// *** Basic block 10

	li          t0, 31		// 0x1f ASCII \x1f
	beq         s5, t0, .ParseAssignmentExpression_label_124

	// *** Basic block 11

	li          t0, 34		// 0x22 ASCII '"'
	beq         s5, t0, .ParseAssignmentExpression_label_120

	// *** Basic block 12

	li          t0, 37		// 0x25 ASCII '%'
	beq         s5, t0, .ParseAssignmentExpression_label_127

	// *** Basic block 13

	li          t0, 39		// 0x27 ASCII '''
	beq         s5, t0, .ParseAssignmentExpression_label_123

	// *** Basic block 14

	li          t0, 41		// 0x29 ASCII ')'
	beq         s5, t0, .ParseAssignmentExpression_label_119

	// *** Basic block 15

	li          t0, 47		// 0x2f ASCII '/'
	beq         s5, t0, .ParseAssignmentExpression_label_125

	// *** Basic block 16

	li          t0, 51		// 0x33 ASCII '3'
	beq         s5, t0, .ParseAssignmentExpression_label_122

	// *** Basic block 17

	li          t0, 53		// 0x35 ASCII '5'
	beq         s5, t0, .ParseAssignmentExpression_label_121

	// *** Basic block 18

.ParseAssignmentExpression_label_115:
	li          s6, 1		// 0x1 ASCII \x1
	j           .ParseAssignmentExpression_label_161

	// *** Basic block 19

.ParseAssignmentExpression_label_118:

	// *** Basic block 20

.ParseAssignmentExpression_label_119:

	// *** Basic block 21

.ParseAssignmentExpression_label_120:

	// *** Basic block 22

.ParseAssignmentExpression_label_121:

	// *** Basic block 23

.ParseAssignmentExpression_label_122:

	// *** Basic block 24

.ParseAssignmentExpression_label_123:

	// *** Basic block 25

.ParseAssignmentExpression_label_124:

	// *** Basic block 26

.ParseAssignmentExpression_label_125:

	// *** Basic block 27

.ParseAssignmentExpression_label_126:

	// *** Basic block 28

.ParseAssignmentExpression_label_127:

	// *** Basic block 29

.ParseAssignmentExpression_label_128:
	mv          a0, s4
	call        LexNextToken

	// *** Basic block 30

	mv          a1, s2
	mv          a0, s1
	call        ParseConditionalExpression

	// *** Basic block 31

	mv          s4, a0
	mv          a1, s5
	mv          a0, s1
	call        AssignASTOpcode

	// *** Basic block 32

	mv          a4, s4
	mv          a3, s3
	mv          a2, t1
	mv          a1, x0
	call        NewBinaryASTNode

	// *** Basic block 33

	mv          s3, a0
	j           .ParseAssignmentExpression_label_161

	// *** Basic block 34

.ParseAssignmentExpression_label_161:
	not         t0, s6
	bnez        t0, .ParseAssignmentExpression_label_92

	// *** Basic block 35

.ParseAssignmentExpression_label_164:
	mv          a0, s3
	j           .ParseAssignmentExpression_label_85
.func_end_ParseAssignmentExpression:
	.size ParseAssignmentExpression, .func_end_ParseAssignmentExpression-ParseAssignmentExpression

	.global SyntaxParseExpression
	.type SyntaxParseExpression, @function

SyntaxParseExpression:

	// *** Basic block 0

	.local ParseAssignmentExpression
	.global LexMatch
	.global NewBinaryASTNode
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
	mv          t0, a1
	ori         s2, t0, 64
	mv          a1, s2
	call        ParseAssignmentExpression

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 0(s1)
	li          s5, 18		// 0x12 ASCII \x12
	mv          a1, s5
	mv          a0, s4
	call        LexMatch

	// *** Basic block 2

	ld          s6, 56(s4)
	beqz        a0, .SyntaxParseExpression_label_66

	// *** Basic block 3

.SyntaxParseExpression_label_35:
	mv          a1, s2
	mv          a0, s1
	call        ParseAssignmentExpression

	// *** Basic block 4

	mv          s2, a0
	mv          a4, s2
	mv          a3, s3
	mv          a2, s6
	mv          a1, x0
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 5

	mv          s3, a0
	mv          a1, s5
	mv          a0, s4
	call        LexMatch

	// *** Basic block 6

	bnez        a0, .SyntaxParseExpression_label_35

	// *** Basic block 7

.SyntaxParseExpression_label_66:
	mv          a0, s3

	// *** Basic block 8

.SyntaxParseExpression_label_69:
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
.func_end_SyntaxParseExpression:
	.size SyntaxParseExpression, .func_end_SyntaxParseExpression-SyntaxParseExpression

	.global SyntaxParseSingleExpression
	.type SyntaxParseSingleExpression, @function

SyntaxParseSingleExpression:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local ParseAssignmentExpression
	j           ParseAssignmentExpression
.func_end_SyntaxParseSingleExpression:
	.size SyntaxParseSingleExpression, .func_end_SyntaxParseSingleExpression-SyntaxParseSingleExpression

.PCend:
	.data
intrinsics:
	.type   intrinsics,@object
	.local  intrinsics
	.size   intrinsics,80
	.p2align  3
	.long    .str.1
	.word   76
	.word   2
	.long    .str.2
	.word   77
	.word   2
	.long    .str.3
	.word   78
	.word   1
	.long    .str.4
	.word   79
	.word   2
	.word   0
	.space  4
	.word   0
	.word   0

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "__builtin_va_start"
	.type .str.1, @object
	.size .str.1, 19

.str.2:
	.asciz "__builtin_va_arg"
	.type .str.2, @object
	.size .str.2, 17

.str.3:
	.asciz "__builtin_va_end"
	.type .str.3, @object
	.size .str.3, 17

.str.4:
	.asciz "__builtin_va_copy"
	.type .str.4, @object
	.size .str.4, 18

.str.5:
	.asciz "unknown-func"
	.type .str.5, @object
	.size .str.5, 13

.str.6:
	.asciz "Calling undeclared function %s"
	.type .str.6, @object
	.size .str.6, 31

.str.7:
	.asciz "No such symbol \"%s\""
	.type .str.7, @object
	.size .str.7, 20

.str.8:
	.asciz "LL"
	.type .str.8, @object
	.size .str.8, 3

.str.9:
	.asciz "Expression syntax error; primary expression expected"
	.type .str.9, @object
	.size .str.9, 53

.str.10:
	.asciz "Wrong number of args for varargs builtin; expected %d, got %zd"
	.type .str.10, @object
	.size .str.10, 63

.str.11:
	.asciz "Expected struct or union member name"
	.type .str.11, @object
	.size .str.11, 37

.str.12:
	.asciz "defined"
	.type .str.12, @object
	.size .str.12, 8

.str.13:
	.asciz "__has_feature"
	.type .str.13, @object
	.size .str.13, 14

.str.14:
	.asciz "__has_include"
	.type .str.14, @object
	.size .str.14, 14

.str.15:
	.asciz "__has_include_next"
	.type .str.15, @object
	.size .str.15, 19

.str.16:
	.asciz "__has_include_next"
	.type .str.16, @object
	.size .str.16, 19

.str.17:
	.asciz "Parentheses expected around type name  in sizeof operator"
	.type .str.17, @object
	.size .str.17, 58

.str.18:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.18, @object
	.size .str.18, 30

.str.19:
	.asciz "expr_parser.c"
	.type .str.19, @object
	.size .str.19, 14

.str.20:
	.asciz "size != -1"
	.type .str.20, @object
	.size .str.20, 11

.str.21:
	.asciz "Invalid cast"
	.type .str.21, @object
	.size .str.21, 13

.str.22:
	.asciz "Missing : in conditional expression"
	.type .str.22, @object
	.size .str.22, 36

.str.23:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.23, @object
	.size .str.23, 30

.str.24:
	.asciz "expr_parser.c"
	.type .str.24, @object
	.size .str.24, 14

.str.25:
	.asciz "false"
	.type .str.25, @object
	.size .str.25, 6

