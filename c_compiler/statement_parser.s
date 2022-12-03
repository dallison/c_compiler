	.file   "statement_parser.c"
	.text
	.option pic
.PCbegin:
	.local  ParseCompoundStatement
	.type ParseCompoundStatement, @function

ParseCompoundStatement:

	// *** Basic block 0

	.global SyntaxOpenScope
	.global NewVector
	.global compiler
	.global VectorAppend
	.global SyntaxNewPCLabel
	.global LexEof
	.global LexLookingAt
	.global LexMatch
	.global SyntaxLookingAtDeclaration
	.global SyntaxParseLocalDeclaration
	.global SyntaxParseStatement
	.global SyntaxNeedBracket
	.global SyntaxCloseScope
	.global NewCompoundStatementASTNode
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
	mv          s2, a2
	mv          s3, a1
	call        SyntaxOpenScope

	// *** Basic block 1

	ori         s4, s3, 16
	call        NewVector

	// *** Basic block 2

	mv          s5, a0
	ld          s6, 0(s1)
	la          t0, compiler
	ld          t0, 0(t0)
	lb          s7, 1228(t0)
	beqz        s7, .ParseCompoundStatement_label_53

	// *** Basic block 3

	mv          a0, s2
	call        SyntaxNewPCLabel

	// *** Basic block 4

	mv          a1, a0
	mv          a0, s5
	call        VectorAppend

	// *** Basic block 5

.ParseCompoundStatement_label_53:
	mv          a0, s6
	call        LexEof

	// *** Basic block 6

	not         s8, a0
	beqz        s8, .ParseCompoundStatement_label_67

	// *** Basic block 7

	li          t0, 44		// 0x2c ASCII ','
	mv          a1, t0
	mv          a0, s6
	call        LexLookingAt

	// *** Basic block 8

	not         s8, a0

	// *** Basic block 9

.ParseCompoundStatement_label_67:
	beqz        s8, .ParseCompoundStatement_label_121

	// *** Basic block 10

.ParseCompoundStatement_label_69:
	li          t0, 49		// 0x31 ASCII '1'
	mv          a1, t0
	mv          a0, s6
	call        LexMatch

	// *** Basic block 11

	not         t0, a0
	beqz        t0, .ParseCompoundStatement_label_105

	// *** Basic block 12

	mv          a0, s1
	call        SyntaxLookingAtDeclaration

	// *** Basic block 13

	beqz        a0, .ParseCompoundStatement_label_88

	// *** Basic block 14

	mv          a0, s1
	call        SyntaxParseLocalDeclaration

	// *** Basic block 15

	mv          s8, a0
	j           .ParseCompoundStatement_label_95

	// *** Basic block 16

.ParseCompoundStatement_label_88:
	mv          a1, s4
	mv          a0, s1
	call        SyntaxParseStatement

	// *** Basic block 17

	mv          s8, a0

	// *** Basic block 18

.ParseCompoundStatement_label_95:
	beq         s8, x0, .ParseCompoundStatement_label_104

	// *** Basic block 19

	mv          a1, s8
	mv          a0, s5
	call        VectorAppend

	// *** Basic block 20

.ParseCompoundStatement_label_104:

	// *** Basic block 21

.ParseCompoundStatement_label_105:
	mv          a0, s6
	call        LexEof

	// *** Basic block 22

	not         s4, a0
	beqz        s4, .ParseCompoundStatement_label_119

	// *** Basic block 23

	li          t0, 44		// 0x2c ASCII ','
	mv          a1, t0
	mv          a0, s6
	call        LexLookingAt

	// *** Basic block 24

	not         s4, a0

	// *** Basic block 25

.ParseCompoundStatement_label_119:
	bnez        s4, .ParseCompoundStatement_label_69

	// *** Basic block 26

.ParseCompoundStatement_label_121:
	beqz        s7, .ParseCompoundStatement_label_131

	// *** Basic block 27

	mv          a0, s2
	call        SyntaxNewPCLabel

	// *** Basic block 28

	mv          a1, a0
	mv          a0, s5
	call        VectorAppend

	// *** Basic block 29

.ParseCompoundStatement_label_131:
	mv          a2, s3
	li          t0, 44		// 0x2c ASCII ','
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 30

	mv          a0, s1
	call        SyntaxCloseScope

	// *** Basic block 31

	mv          a1, s2
	mv          a0, s5
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
	j           NewCompoundStatementASTNode
.func_end_ParseCompoundStatement:
	.size ParseCompoundStatement, .func_end_ParseCompoundStatement-ParseCompoundStatement

	.local  ParseIfStatement
	.type ParseIfStatement, @function

ParseIfStatement:

	// *** Basic block 0

	.global SyntaxNeedBracket
	.global SyntaxParseExpression
	.global SyntaxParseStatement
	.global LexMatch
	.global NewIfStatementASTNode
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
	mv          s3, a2
	mv          a2, s2
	li          a1, 29		// 0x1d ASCII \x1d
	call        SyntaxNeedBracket

	// *** Basic block 1

	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseExpression

	// *** Basic block 2

	mv          s4, a0
	mv          a2, s2
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 3

	ld          s5, 0(s1)
	ld          s3, 56(s5)
	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseStatement

	// *** Basic block 4

	mv          s6, a0
	mv          s7, x0
	li          t0, 66		// 0x42 ASCII 'B'
	mv          a1, t0
	mv          a0, s5
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .ParseIfStatement_label_70

	// *** Basic block 6

	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseStatement

	// *** Basic block 7

	mv          s7, a0

	// *** Basic block 8

.ParseIfStatement_label_70:
	mv          a3, s3
	mv          a2, s7
	mv          a1, s6
	mv          a0, s4
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
	j           NewIfStatementASTNode
.func_end_ParseIfStatement:
	.size ParseIfStatement, .func_end_ParseIfStatement-ParseIfStatement

	.local  ParseWhileStatement
	.type ParseWhileStatement, @function

ParseWhileStatement:

	// *** Basic block 0

	.global SyntaxNeedBracket
	.global SyntaxParseExpression
	.global SyntaxParseStatement
	.global NewCombinedStatementASTNode
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
	mv          a2, s2
	li          a1, 29		// 0x1d ASCII \x1d
	call        SyntaxNeedBracket

	// *** Basic block 1

	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseExpression

	// *** Basic block 2

	mv          s4, a0
	mv          a2, s2
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 3

	ld          t0, 0(s1)
	ld          s3, 56(t0)
	lw          t0, 72(s1)
	addi        t0, t0, 1
	sw          t0, 72(s1)
	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseStatement

	// *** Basic block 4

	mv          s5, a0
	lw          t0, 72(s1)
	addi        t0, t0, -1
	sw          t0, 72(s1)
	mv          a3, s3
	mv          a2, s5
	mv          a1, s4
	li          t0, 72		// 0x48 ASCII 'H'
	mv          a0, t0
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewCombinedStatementASTNode
.func_end_ParseWhileStatement:
	.size ParseWhileStatement, .func_end_ParseWhileStatement-ParseWhileStatement

	.local  ParseDoStatement
	.type ParseDoStatement, @function

ParseDoStatement:

	// *** Basic block 0

	.global SyntaxParseStatement
	.global LexMatch
	.global SyntaxError
	.global SyntaxNeedBracket
	.global SyntaxParseExpression
	.global NewCombinedStatementASTNode
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
	ld          s4, 0(s1)
	lw          t0, 72(s1)
	addi        t0, t0, 1
	sw          t0, 72(s1)
	call        SyntaxParseStatement

	// *** Basic block 1

	mv          s5, a0
	lw          t0, 72(s1)
	addi        t0, t0, -1
	sw          t0, 72(s1)
	ld          t0, 0(s1)
	ld          s3, 56(t0)
	li          t0, 94		// 0x5e ASCII '^'
	mv          a1, t0
	mv          a0, s4
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .ParseDoStatement_label_61

	// *** Basic block 3

	lla         a1, .str.1
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 4

.ParseDoStatement_label_61:
	mv          a2, s2
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 5

	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseExpression

	// *** Basic block 6

	mv          s4, a0
	mv          a2, s2
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 7

	mv          a3, s3
	mv          a2, s5
	mv          a1, s4
	li          t0, 32		// 0x20 ASCII ' '
	mv          a0, t0
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewCombinedStatementASTNode
.func_end_ParseDoStatement:
	.size ParseDoStatement, .func_end_ParseDoStatement-ParseDoStatement

	.local  ParseSwitchStatement
	.type ParseSwitchStatement, @function

ParseSwitchStatement:

	// *** Basic block 0

	.global SyntaxNeedBracket
	.global SyntaxParseExpression
	.global SyntaxParseStatement
	.global NewSwitchStatementASTNode
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
	mv          a2, s2
	li          a1, 29		// 0x1d ASCII \x1d
	call        SyntaxNeedBracket

	// *** Basic block 1

	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseExpression

	// *** Basic block 2

	mv          s4, a0
	mv          a2, s2
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 3

	ld          t0, 0(s1)
	ld          s3, 56(t0)
	lw          t0, 76(s1)
	addi        t0, t0, 1
	sw          t0, 76(s1)
	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseStatement

	// *** Basic block 4

	mv          s5, a0
	lw          t0, 76(s1)
	addi        t0, t0, -1
	sw          t0, 76(s1)
	mv          a2, s3
	mv          a1, s5
	mv          a0, s4
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewSwitchStatementASTNode
.func_end_ParseSwitchStatement:
	.size ParseSwitchStatement, .func_end_ParseSwitchStatement-ParseSwitchStatement

	.local  ParseAsmStatement
	.type ParseAsmStatement, @function

ParseAsmStatement:

	// *** Basic block 0

	.global SyntaxNeedBracket
	.global LexMatch
	.global NewString
	.global LexLookingAt
	.global StringAppend
	.global LexNextToken
	.global NewAsmASTNode
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
	li          a2, 32		// 0x20 ASCII ' '
	li          a1, 29		// 0x1d ASCII \x1d
	call        SyntaxNeedBracket

	// *** Basic block 1

	ld          s3, 0(s1)
	li          t0, 92		// 0x5c ASCII '\'
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 2

	addi        t0, s3, 80
	ld          s4, 16(t0)
	mv          s5, a0
	lla         a0, .str.2
	call        NewString

	// *** Basic block 3

	mv          s6, a0
	li          s7, 4		// 0x4 ASCII \x4
	mv          a1, s7
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 4

	beqz        a0, .ParseAsmStatement_label_75

	// *** Basic block 5

.ParseAsmStatement_label_59:
	mv          a1, s4
	mv          a0, s6
	call        StringAppend

	// *** Basic block 6

	mv          a0, s3
	call        LexNextToken

	// *** Basic block 7

	mv          a1, s7
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 8

	bnez        a0, .ParseAsmStatement_label_59

	// *** Basic block 9

.ParseAsmStatement_label_75:
	li          t0, 192		// 0xc0 ASCII \xc0
	mv          a2, t0
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 10

	mv          a2, s2
	mv          a1, s5
	mv          a0, s6
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
	j           NewAsmASTNode
.func_end_ParseAsmStatement:
	.size ParseAsmStatement, .func_end_ParseAsmStatement-ParseAsmStatement

	.local  ParseForStatement
	.type ParseForStatement, @function

ParseForStatement:

	// *** Basic block 0

	.global SyntaxNeedBracket
	.global SyntaxOpenScope
	.global LexLookingAt
	.global SyntaxLookingAtType
	.global SyntaxParseLocalDeclaration
	.global SyntaxParseExpression
	.global SyntaxNeedSemicolon
	.global SyntaxParseStatement
	.global SyntaxCloseScope
	.global NewForStatementASTNode
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
	mv          s3, a2
	mv          a2, s2
	li          a1, 29		// 0x1d ASCII \x1d
	call        SyntaxNeedBracket

	// *** Basic block 1

	ld          s4, 0(s1)
	mv          s5, x0
	mv          s6, x0
	mv          s7, x0
	mv          a0, s1
	call        SyntaxOpenScope

	// *** Basic block 2

	li          s8, 49		// 0x31 ASCII '1'
	mv          a1, s8
	mv          a0, s4
	call        LexLookingAt

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .ParseForStatement_label_80

	// *** Basic block 4

	mv          a0, s1
	call        SyntaxLookingAtType

	// *** Basic block 5

	beqz        a0, .ParseForStatement_label_66

	// *** Basic block 6

	mv          a0, s1
	call        SyntaxParseLocalDeclaration

	// *** Basic block 7

	mv          s5, a0
	j           .ParseForStatement_label_78

	// *** Basic block 8

.ParseForStatement_label_66:
	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseExpression

	// *** Basic block 9

	mv          s5, a0
	ori         a1, s2, 4
	mv          a0, s1
	call        SyntaxNeedSemicolon

	// *** Basic block 10

.ParseForStatement_label_78:
	j           .ParseForStatement_label_86

	// *** Basic block 11

.ParseForStatement_label_80:
	ori         a1, s2, 4
	mv          a0, s1
	call        SyntaxNeedSemicolon

	// *** Basic block 12

.ParseForStatement_label_86:
	mv          a1, s8
	mv          a0, s4
	call        LexLookingAt

	// *** Basic block 13

	not         t0, a0
	beqz        t0, .ParseForStatement_label_101

	// *** Basic block 14

	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseExpression

	// *** Basic block 15

	mv          s6, a0

	// *** Basic block 16

.ParseForStatement_label_101:
	ori         a1, s2, 8
	mv          a0, s1
	call        SyntaxNeedSemicolon

	// *** Basic block 17

	li          s8, 45		// 0x2d ASCII '-'
	mv          a1, s8
	mv          a0, s4
	call        LexLookingAt

	// *** Basic block 18

	not         t0, a0
	beqz        t0, .ParseForStatement_label_121

	// *** Basic block 19

	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseExpression

	// *** Basic block 20

	mv          s7, a0

	// *** Basic block 21

.ParseForStatement_label_121:
	mv          a2, s2
	mv          a1, s8
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 22

	ld          s3, 56(s4)
	lw          t0, 72(s1)
	addi        t0, t0, 1
	sw          t0, 72(s1)
	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseStatement

	// *** Basic block 23

	mv          s4, a0
	lw          t0, 72(s1)
	addi        t0, t0, -1
	sw          t0, 72(s1)
	mv          a0, s1
	call        SyntaxCloseScope

	// *** Basic block 24

	mv          a4, s3
	mv          a3, s4
	mv          a2, s7
	mv          a1, s6
	mv          a0, s5
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
	j           NewForStatementASTNode
.func_end_ParseForStatement:
	.size ParseForStatement, .func_end_ParseForStatement-ParseForStatement

	.local  ParseBreakStatement
	.type ParseBreakStatement, @function

ParseBreakStatement:

	// *** Basic block 0

	.global SyntaxError
	.global NewASTNode
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
	mv          s2, a2
	lw          t1, 72(s1)
	seqz        t0, t1
	bnez        t1, .ParseBreakStatement_label_24

	// *** Basic block 1

	lw          t1, 76(s1)
	seqz        t0, t1

	// *** Basic block 2

.ParseBreakStatement_label_24:
	beqz        t0, .ParseBreakStatement_label_33

	// *** Basic block 3

	lla         a1, .str.3
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 4

.ParseBreakStatement_label_33:
	mv          a2, s2
	mv          a1, x0
	li          t0, 23		// 0x17 ASCII \x17
	mv          a0, t0
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewASTNode
.func_end_ParseBreakStatement:
	.size ParseBreakStatement, .func_end_ParseBreakStatement-ParseBreakStatement

	.local  ParseContinueStatement
	.type ParseContinueStatement, @function

ParseContinueStatement:

	// *** Basic block 0

	.global SyntaxError
	.global NewASTNode
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
	mv          s2, a2
	lw          t0, 72(s1)
	bnez        t0, .ParseContinueStatement_label_26

	// *** Basic block 1

	lla         a1, .str.4
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 2

.ParseContinueStatement_label_26:
	mv          a2, s2
	mv          a1, x0
	li          t0, 31		// 0x1f ASCII \x1f
	mv          a0, t0
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewASTNode
.func_end_ParseContinueStatement:
	.size ParseContinueStatement, .func_end_ParseContinueStatement-ParseContinueStatement

	.local  ParseCaseStatement
	.type ParseCaseStatement, @function

ParseCaseStatement:

	// *** Basic block 0

	.global SyntaxError
	.global SyntaxParseExpression
	.global LexMatch
	.global LexLookingAt
	.global SyntaxParseStatement
	.global NewCaseLabelASTNode
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
	mv          s3, a2
	lw          t0, 76(s1)
	bnez        t0, .ParseCaseStatement_label_35

	// *** Basic block 1

	lla         a1, .str.5
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 2

.ParseCaseStatement_label_35:
	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseExpression

	// *** Basic block 3

	mv          s4, a0
	ld          s5, 0(s1)
	li          t0, 17		// 0x11 ASCII \x11
	mv          a1, t0
	mv          a0, s5
	call        LexMatch

	// *** Basic block 4

	not         t0, a0
	beqz        t0, .ParseCaseStatement_label_58

	// *** Basic block 5

	lla         a1, .str.6
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 6

.ParseCaseStatement_label_58:
	mv          s6, x0
	li          t0, 58		// 0x3a ASCII ':'
	mv          a1, t0
	mv          a0, s5
	call        LexLookingAt

	// *** Basic block 7

	not         s7, a0
	beqz        s7, .ParseCaseStatement_label_77

	// *** Basic block 8

	li          t0, 63		// 0x3f ASCII '?'
	mv          a1, t0
	mv          a0, s5
	call        LexLookingAt

	// *** Basic block 9

	not         s7, a0

	// *** Basic block 10

.ParseCaseStatement_label_77:
	beqz        s7, .ParseCaseStatement_label_85

	// *** Basic block 11

	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseStatement

	// *** Basic block 12

	mv          s6, a0

	// *** Basic block 13

.ParseCaseStatement_label_85:
	mv          a2, s3
	mv          a1, s6
	mv          a0, s4
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
	j           NewCaseLabelASTNode
.func_end_ParseCaseStatement:
	.size ParseCaseStatement, .func_end_ParseCaseStatement-ParseCaseStatement

	.local  ParseDefaultStatement
	.type ParseDefaultStatement, @function

ParseDefaultStatement:

	// *** Basic block 0

	.global SyntaxError
	.global LexMatch
	.global SyntaxParseStatement
	.global NewCaseLabelASTNode
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
	lw          t0, 76(s1)
	bnez        t0, .ParseDefaultStatement_label_31

	// *** Basic block 1

	lla         a1, .str.7
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 2

.ParseDefaultStatement_label_31:
	ld          a0, 0(s1)
	li          t0, 17		// 0x11 ASCII \x11
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .ParseDefaultStatement_label_46

	// *** Basic block 4

	lla         a1, .str.8
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 5

.ParseDefaultStatement_label_46:
	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseStatement

	// *** Basic block 6

	mv          s4, a0
	mv          a2, s3
	mv          a1, s4
	mv          a0, x0
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewCaseLabelASTNode
.func_end_ParseDefaultStatement:
	.size ParseDefaultStatement, .func_end_ParseDefaultStatement-ParseDefaultStatement

	.local  ParseReturnStatement
	.type ParseReturnStatement, @function

ParseReturnStatement:

	// *** Basic block 0

	.global LexLookingAt
	.global LexMatch
	.local ParseAsmStatement
	.global SyntaxParseExpression
	.global NewCombinedStatementASTNode
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
	mv          s4, x0
	ld          s5, 0(s1)
	li          a1, 49		// 0x31 ASCII '1'
	mv          a0, s5
	call        LexLookingAt

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .ParseReturnStatement_label_55

	// *** Basic block 2

	li          t0, 95		// 0x5f ASCII '_'
	mv          a1, t0
	mv          a0, s5
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .ParseReturnStatement_label_47

	// *** Basic block 4

	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        ParseAsmStatement

	// *** Basic block 5

	mv          s4, a0
	j           .ParseReturnStatement_label_54

	// *** Basic block 6

.ParseReturnStatement_label_47:
	mv          a1, s2
	mv          a0, s1
	call        SyntaxParseExpression

	// *** Basic block 7

	mv          s4, a0

	// *** Basic block 8

.ParseReturnStatement_label_54:

	// *** Basic block 9

.ParseReturnStatement_label_55:
	mv          a3, s3
	mv          a2, x0
	mv          a1, s4
	li          t0, 62		// 0x3e ASCII '>'
	mv          a0, t0
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewCombinedStatementASTNode
.func_end_ParseReturnStatement:
	.size ParseReturnStatement, .func_end_ParseReturnStatement-ParseReturnStatement

	.local  ParseGotoStatement
	.type ParseGotoStatement, @function

ParseGotoStatement:

	// *** Basic block 0

	.global LexLookingAt
	.global SyntaxError
	.global NewString
	.global LexNextToken
	.global NewGotoStatementASTNode
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
	ld          s3, 0(s1)
	li          a1, 3		// 0x3 ASCII \x3
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .ParseGotoStatement_label_39

	// *** Basic block 2

	lla         a1, .str.9
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 3

	mv          a0, x0

	// *** Basic block 4

.ParseGotoStatement_label_36:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.ParseGotoStatement_label_39:
	addi        t0, s3, 80
	ld          a0, 16(t0)
	call        NewString

	// *** Basic block 6

	mv          s4, a0
	mv          a0, s3
	call        LexNextToken

	// *** Basic block 7

	mv          a1, s2
	mv          a0, s4
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewGotoStatementASTNode
.func_end_ParseGotoStatement:
	.size ParseGotoStatement, .func_end_ParseGotoStatement-ParseGotoStatement

	.global SyntaxParseStatement
	.type SyntaxParseStatement, @function

SyntaxParseStatement:

	// *** Basic block 0

	.global statement_parsers
	.global LexNextToken
	.global LexLookingAt
	.global LexSkipSpacesAndComments
	.global StringInit
	.global SyntaxParseStatement
	.global NewLabelASTNode
	.global StringDestruct
	.global SyntaxParseExpression
	.global NewExpressionStatementASTNode
	.global SyntaxNeedSemicolon
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
	mv          s1, a1
	mv          s2, a0
	ori         s1, s1, 1
	ld          s3, 0(s2)
	mv          s4, x0
	li          s5, 1		// 0x1 ASCII \x1
	ld          s6, 56(s3)
	mv          s7, x0
	mv          s8, x0
	lw          t0, 72(s3)

	// *** Basic block 1

.SyntaxParseStatement_label_52:
	slli        t1, s8, 3
	slli        t2, s8, 4
	add         t1, t1, t2
	la          t2, statement_parsers
	add         s9, t2, t1
	lw          t1, 0(s9)
	bne         t0, t1, .SyntaxParseStatement_label_85

	// *** Basic block 2

	mv          a0, s3
	call        LexNextToken

	// *** Basic block 3

	ld          s10, 8(s9)
	beq         s10, x0, .SyntaxParseStatement_label_80

	// *** Basic block 4

	mv          a2, s6
	mv          a1, s1
	mv          a0, s2
	jalr         x1, s10, 0

	// *** Basic block 5

	mv          s4, a0

	// *** Basic block 6

.SyntaxParseStatement_label_80:
	lb          s5, 16(s9)
	li          s7, 1		// 0x1 ASCII \x1
	j           .SyntaxParseStatement_label_92

	// *** Basic block 7

.SyntaxParseStatement_label_85:

	// *** Basic block 8

.SyntaxParseStatement_label_86:
	addi        s8, s8, 1
	li          t0, 14		// 0xe ASCII \xe
	bge         s8, t0, .SyntaxParseStatement_label_52

	// *** Basic block 9

.SyntaxParseStatement_label_92:
	not         t0, s7
	beqz        t0, .SyntaxParseStatement_label_188

	// *** Basic block 10

	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 11

	beqz        a0, .SyntaxParseStatement_label_173

	// *** Basic block 12

	mv          a0, s3
	call        LexSkipSpacesAndComments

	// *** Basic block 13

	addi        t0, s3, 8
	ld          t0, 16(t0)
	ld          t1, 48(s3)
	add         t0, t0, t1
	lb          t0, 0(t0)
	li          t1, 58		// 0x3a ASCII ':'
	bne         t0, t1, .SyntaxParseStatement_label_157

	// *** Basic block 14

	addi        a0, s0, -64
	addi        t0, s3, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 15

	mv          a0, s3
	call        LexNextToken

	// *** Basic block 16

	mv          a0, s3
	call        LexNextToken

	// *** Basic block 17

	mv          a1, s1
	mv          a0, s2
	call        SyntaxParseStatement

	// *** Basic block 18

	mv          s3, a0
	addi        t0, s0, -64
	ld          a0, 16(t0)
	mv          a3, s6
	mv          a2, x0
	mv          a1, s3
	call        NewLabelASTNode

	// *** Basic block 19

	mv          s4, a0
	mv          s5, x0
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 20

	j           .SyntaxParseStatement_label_171

	// *** Basic block 21

.SyntaxParseStatement_label_157:
	mv          a1, s1
	mv          a0, s2
	call        SyntaxParseExpression

	// *** Basic block 22

	mv          s7, a0
	mv          a1, s6
	mv          a0, s7
	call        NewExpressionStatementASTNode

	// *** Basic block 23

	mv          s4, a0

	// *** Basic block 24

.SyntaxParseStatement_label_171:
	j           .SyntaxParseStatement_label_187

	// *** Basic block 25

.SyntaxParseStatement_label_173:
	mv          a1, s1
	mv          a0, s2
	call        SyntaxParseExpression

	// *** Basic block 26

	mv          s9, a0
	mv          a1, s6
	mv          a0, s9
	call        NewExpressionStatementASTNode

	// *** Basic block 27

	mv          s4, a0

	// *** Basic block 28

.SyntaxParseStatement_label_187:

	// *** Basic block 29

.SyntaxParseStatement_label_188:
	beqz        s5, .SyntaxParseStatement_label_195

	// *** Basic block 30

	mv          a1, s1
	mv          a0, s2
	call        SyntaxNeedSemicolon

	// *** Basic block 31

.SyntaxParseStatement_label_195:
	beq         s4, x0, .SyntaxParseStatement_label_203

	// *** Basic block 32

	lw          t0, 8(s4)
	ori         t0, t0, 4
	sw          t0, 8(s4)

	// *** Basic block 33

.SyntaxParseStatement_label_203:
	mv          a0, s4

	// *** Basic block 34

.SyntaxParseStatement_label_206:
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
.func_end_SyntaxParseStatement:
	.size SyntaxParseStatement, .func_end_SyntaxParseStatement-SyntaxParseStatement

.PCend:
	.data
statement_parsers:
	.type   statement_parsers,@object
	.global statement_parsers
	.size   statement_parsers,336
	.p2align  3
	.word   49
	.space  4
	.word   0
	.space  4
	.byte   0
	.space  7
	.word   24
	.space  4
	.global ParseCompoundStatement
	.long    ParseCompoundStatement
	.byte   0
	.space  7
	.word   73
	.space  4
	.global ParseIfStatement
	.long    ParseIfStatement
	.byte   0
	.space  7
	.word   94
	.space  4
	.global ParseWhileStatement
	.long    ParseWhileStatement
	.byte   0
	.space  7
	.word   64
	.space  4
	.global ParseDoStatement
	.long    ParseDoStatement
	.byte   1
	.space  7
	.word   71
	.space  4
	.global ParseForStatement
	.long    ParseForStatement
	.byte   0
	.space  7
	.word   86
	.space  4
	.global ParseSwitchStatement
	.long    ParseSwitchStatement
	.byte   0
	.space  7
	.word   56
	.space  4
	.global ParseBreakStatement
	.long    ParseBreakStatement
	.byte   1
	.space  7
	.word   62
	.space  4
	.global ParseContinueStatement
	.long    ParseContinueStatement
	.byte   1
	.space  7
	.word   58
	.space  4
	.global ParseCaseStatement
	.long    ParseCaseStatement
	.byte   0
	.space  7
	.word   63
	.space  4
	.global ParseDefaultStatement
	.long    ParseDefaultStatement
	.byte   0
	.space  7
	.word   80
	.space  4
	.global ParseReturnStatement
	.long    ParseReturnStatement
	.byte   1
	.space  7
	.word   72
	.space  4
	.global ParseGotoStatement
	.long    ParseGotoStatement
	.byte   1
	.space  7
	.word   95
	.space  4
	.global ParseAsmStatement
	.long    ParseAsmStatement
	.byte   1
	.space  7

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "Missing while in do loop"
	.type .str.1, @object
	.size .str.1, 25

.str.2:
	.asciz "(null)"
	.type .str.2, @object
	.size .str.2, 1

.str.3:
	.asciz "break outside loop or switch"
	.type .str.3, @object
	.size .str.3, 29

.str.4:
	.asciz "continue outside loop"
	.type .str.4, @object
	.size .str.4, 22

.str.5:
	.asciz "case outside switch"
	.type .str.5, @object
	.size .str.5, 20

.str.6:
	.asciz "Missing colon after case"
	.type .str.6, @object
	.size .str.6, 25

.str.7:
	.asciz "default outside switch"
	.type .str.7, @object
	.size .str.7, 23

.str.8:
	.asciz "Missing colon after default"
	.type .str.8, @object
	.size .str.8, 28

.str.9:
	.asciz "Label expected after goto"
	.type .str.9, @object
	.size .str.9, 26

