	.file   "statement_semantics.c"
	.text
	.option pic
.PCbegin:
	.local  AnalyzeExpressionStatement
	.type AnalyzeExpressionStatement, @function

AnalyzeExpressionStatement:

	// *** Basic block 0

	.global AnalyzeExpression
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 56(t0)
	j           AnalyzeExpression
.func_end_AnalyzeExpressionStatement:
	.size AnalyzeExpressionStatement, .func_end_AnalyzeExpressionStatement-AnalyzeExpressionStatement

	.local  AnalyzeIfStatement
	.type AnalyzeIfStatement, @function

AnalyzeIfStatement:

	// *** Basic block 0

	.global AnalyzeExpression
	.global SemanticCheckScalarType
	.global AnalyzeStatement
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
	ld          a0, 56(s1)
	call        AnalyzeExpression

	// *** Basic block 1

	sd          a0, 56(s1)
	ld          s2, 56(s1)
	mv          a0, s2
	call        SemanticCheckScalarType

	// *** Basic block 2

	ld          a0, 64(s1)
	call        AnalyzeStatement

	// *** Basic block 3

	ld          a0, 72(s1)
	call        AnalyzeStatement

	// *** Basic block 4

	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SemanticCheckScalarType
.func_end_AnalyzeIfStatement:
	.size AnalyzeIfStatement, .func_end_AnalyzeIfStatement-AnalyzeIfStatement

	.local  AnalyzeWhileStatement
	.type AnalyzeWhileStatement, @function

AnalyzeWhileStatement:

	// *** Basic block 0

	.global AnalyzeExpression
	.global SemanticCheckScalarType
	.global AnalyzeStatement
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
	ld          a0, 56(s1)
	call        AnalyzeExpression

	// *** Basic block 1

	sd          a0, 56(s1)
	ld          s2, 56(s1)
	mv          a0, s2
	call        SemanticCheckScalarType

	// *** Basic block 2

	ld          a0, 64(s1)
	call        AnalyzeStatement

	// *** Basic block 3

	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SemanticCheckScalarType
.func_end_AnalyzeWhileStatement:
	.size AnalyzeWhileStatement, .func_end_AnalyzeWhileStatement-AnalyzeWhileStatement

	.local  AnalyzeDoStatement
	.type AnalyzeDoStatement, @function

AnalyzeDoStatement:

	// *** Basic block 0

	.global AnalyzeExpression
	.global SemanticCheckScalarType
	.global AnalyzeStatement
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
	ld          a0, 56(s1)
	call        AnalyzeExpression

	// *** Basic block 1

	sd          a0, 56(s1)
	ld          s2, 56(s1)
	mv          a0, s2
	call        SemanticCheckScalarType

	// *** Basic block 2

	ld          a0, 64(s1)
	call        AnalyzeStatement

	// *** Basic block 3

	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SemanticCheckScalarType
.func_end_AnalyzeDoStatement:
	.size AnalyzeDoStatement, .func_end_AnalyzeDoStatement-AnalyzeDoStatement

	.local  CompareCaseValue
	.type CompareCaseValue, @function

CompareCaseValue:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 0(a0)
	ld          t1, 0(a1)
	ld          t0, 72(t0)
	ld          t1, 72(t1)
	sub         t0, t0, t1
	sext.w      a0, t0

	// *** Basic block 1

.CompareCaseValue_label_22:
	ret         
.func_end_CompareCaseValue:
	.size CompareCaseValue, .func_end_CompareCaseValue-CompareCaseValue

	.local  ResolveSwitchStatement
	.type ResolveSwitchStatement, @function

ResolveSwitchStatement:

	// *** Basic block 0

	.global SemanticError
	.global NormalConversion
	.global EvaluateIntegerExpression
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
	mv          s2, a3
	mv          s3, a1
	ld          s4, 0(s3)
	lw          s5, 0(s1)
	li          t0, 70		// 0x46 ASCII 'F'
	bne         s5, t0, .ResolveSwitchStatement_label_57

	// *** Basic block 1

	bnez        s2, .ResolveSwitchStatement_label_45

	// *** Basic block 2

	lw          t0, 8(s3)
	addi        t0, t0, 1
	sw          t0, 8(s3)
	j           .ResolveSwitchStatement_label_56

	// *** Basic block 3

.ResolveSwitchStatement_label_45:
	li          t0, 1		// 0x1 ASCII \x1
	bne         s2, t0, .ResolveSwitchStatement_label_55

	// *** Basic block 4

	lw          t0, 8(s3)
	addi        t0, t0, -1
	sw          t0, 8(s3)

	// *** Basic block 5

.ResolveSwitchStatement_label_55:

	// *** Basic block 6

.ResolveSwitchStatement_label_56:

	// *** Basic block 7

.ResolveSwitchStatement_label_57:
	lw          t0, 8(s3)
	li          t1, 1		// 0x1 ASCII \x1
	bge         t1, t0, .ResolveSwitchStatement_label_67

	// *** Basic block 8

.ResolveSwitchStatement_label_64:
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

.ResolveSwitchStatement_label_67:
	seqz        t0, s2
	bnez        s2, .ResolveSwitchStatement_label_74

	// *** Basic block 10

	addi        t1, s5, -26
	seqz        t0, t1

	// *** Basic block 11

.ResolveSwitchStatement_label_74:
	beqz        t0, .ResolveSwitchStatement_label_141

	// *** Basic block 12

	mv          s5, s1
	ld          s6, 56(s5)
	bne         s6, x0, .ResolveSwitchStatement_label_98

	// *** Basic block 13

	ld          t0, 96(s4)
	beq         t0, x0, .ResolveSwitchStatement_label_94

	// *** Basic block 14

	lla         a1, .str.1
	mv          a0, s1
	call        SemanticError

	// *** Basic block 15

.ResolveSwitchStatement_label_94:
	sd          s5, 96(s4)
	j           .ResolveSwitchStatement_label_64

	// *** Basic block 16

.ResolveSwitchStatement_label_98:
	ld          s7, 56(s4)
	ld          a1, 16(s7)
	mv          a0, s6
	call        NormalConversion

	// *** Basic block 17

	mv          a0, s6
	call        EvaluateIntegerExpression

	// *** Basic block 18

	not         t0, a0
	beqz        t0, .ResolveSwitchStatement_label_120

	// *** Basic block 19

	lla         a1, .str.2
	mv          a0, s7
	call        SemanticError

	// *** Basic block 20

.ResolveSwitchStatement_label_120:
	ld          s6, 72(s5)
	ld          t0, 112(s4)
	bge         s6, t0, .ResolveSwitchStatement_label_128

	// *** Basic block 21

	sd          s6, 112(s4)

	// *** Basic block 22

.ResolveSwitchStatement_label_128:
	ld          t0, 120(s4)
	bge         t0, s6, .ResolveSwitchStatement_label_135

	// *** Basic block 23

	sd          s6, 120(s4)

	// *** Basic block 24

.ResolveSwitchStatement_label_135:
	addi        a0, s4, 72
	mv          a1, s1
	call        VectorAppend

	// *** Basic block 25

.ResolveSwitchStatement_label_141:
	j           .ResolveSwitchStatement_label_64
.func_end_ResolveSwitchStatement:
	.size ResolveSwitchStatement, .func_end_ResolveSwitchStatement-ResolveSwitchStatement

	.local  AnalyzeEnumSwitch
	.type AnalyzeEnumSwitch, @function

AnalyzeEnumSwitch:

	// *** Basic block 0

	.global printf
	.global abort
	.global BitSetInsert
	.global BitSetContains
	.global SemanticWarning
	.global VectorAppend
	.global VectorDestruct
	.global BitSetDestruct
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -80(s0)
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
	sd          x0, -80(s0)
	sd          x0, -72(s0)
	sd          x0, -80(s0)
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -64(s0)
	ld          t0, 56(s1)
	ld          t0, 16(t0)
	ld          s2, 32(t0)
	beq         s2, x0, .AnalyzeEnumSwitch_label_62

	// *** Basic block 1

	ld          t0, 8(s2)
	ld          t0, 16(t0)
	j           .AnalyzeEnumSwitch_label_80

	// *** Basic block 2

.AnalyzeEnumSwitch_label_62:
	lla         a0, .str.3
	lla         a1, .str.4
	lla         a3, .str.5
	li          t0, 120		// 0x78 ASCII 'x'
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.AnalyzeEnumSwitch_label_80:
	mv          s3, x0
	addi        t0, s2, 16
	ld          s4, 8(t0)
	bge         x0, s4, .AnalyzeEnumSwitch_label_105

	// *** Basic block 5

	ld          s5, 16(s2)
	ld          s2, 16(s2)

	// *** Basic block 6

.AnalyzeEnumSwitch_label_90:
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          s5, 0(t0)
	addi        a0, s0, -80
	ld          a1, 112(s5)
	call        BitSetInsert

	// *** Basic block 7

.AnalyzeEnumSwitch_label_101:
	addi        s3, s3, 1
	bge         s3, s4, .AnalyzeEnumSwitch_label_90

	// *** Basic block 8

.AnalyzeEnumSwitch_label_105:
	addi        t0, s1, 72
	ld          s2, 8(t0)
	mv          s5, x0
	bge         x0, s2, .AnalyzeEnumSwitch_label_155

	// *** Basic block 9

.AnalyzeEnumSwitch_label_114:
	ld          t0, 72(s1)
	slli        t1, s5, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	ld          s6, 72(t0)
	addi        a0, s0, -80
	mv          a1, s6
	call        BitSetContains

	// *** Basic block 10

	beqz        a0, .AnalyzeEnumSwitch_label_135

	// *** Basic block 11

	addi        a0, s0, -64
	mv          a1, s6
	call        BitSetInsert

	// *** Basic block 12

	j           .AnalyzeEnumSwitch_label_150

	// *** Basic block 13

.AnalyzeEnumSwitch_label_135:
	lla         a1, .str.6
	lla         a2, .str.7
	mv          a4, t0
	mv          a3, s6
	mv          a0, s1
	call        SemanticWarning

	// *** Basic block 14

.AnalyzeEnumSwitch_label_150:

	// *** Basic block 15

.AnalyzeEnumSwitch_label_151:
	addi        s5, s5, 1
	bge         s5, s2, .AnalyzeEnumSwitch_label_114

	// *** Basic block 16

.AnalyzeEnumSwitch_label_155:
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -48(s0)
	mv          s2, x0
	bge         x0, s4, .AnalyzeEnumSwitch_label_190

	// *** Basic block 17

.AnalyzeEnumSwitch_label_167:
	slli        t0, s2, 3
	add         t0, s2, t0
	ld          s6, 0(t0)
	addi        a0, s0, -64
	ld          a1, 112(s6)
	call        BitSetContains

	// *** Basic block 18

	not         t0, a0
	beqz        t0, .AnalyzeEnumSwitch_label_185

	// *** Basic block 19

	addi        a0, s0, -48
	mv          a1, s6
	call        VectorAppend

	// *** Basic block 20

.AnalyzeEnumSwitch_label_185:

	// *** Basic block 21

.AnalyzeEnumSwitch_label_186:
	addi        s2, s2, 1
	bge         s2, s4, .AnalyzeEnumSwitch_label_167

	// *** Basic block 22

.AnalyzeEnumSwitch_label_190:
	ld          s4, 96(s1)
	sub         t1, s4, x0
	seqz        t0, t1
	bne         s4, x0, .AnalyzeEnumSwitch_label_202

	// *** Basic block 23

	ld          t1, -48(s0)
	addi        t2, s0, -48
	ld          t2, 8(t2)
	slt         t0, x0, t2

	// *** Basic block 24

.AnalyzeEnumSwitch_label_202:
	beqz        t0, .AnalyzeEnumSwitch_label_259

	// *** Basic block 25

	addi        t0, s0, -48
	ld          s6, 8(t0)
	li          t0, 4		// 0x4 ASCII \x4
	bge         t0, s6, .AnalyzeEnumSwitch_label_230

	// *** Basic block 26

	ld          t0, -48(s0)
	ld          s7, 0(t0)
	lla         a1, .str.8
	lla         a2, .str.9
	ld          a3, 16(s7)
	addi        a4, s6, -1
	mv          a0, s1
	call        SemanticWarning

	// *** Basic block 27

	j           .AnalyzeEnumSwitch_label_257

	// *** Basic block 28

.AnalyzeEnumSwitch_label_230:
	mv          s7, x0
	bge         x0, s6, .AnalyzeEnumSwitch_label_256

	// *** Basic block 29

.AnalyzeEnumSwitch_label_235:
	slli        t0, s7, 3
	add         t0, t1, t0
	ld          s8, 0(t0)
	lla         a1, .str.10
	lla         a2, .str.11
	ld          a3, 16(s8)
	mv          a0, s1
	call        SemanticWarning

	// *** Basic block 30

.AnalyzeEnumSwitch_label_252:
	addi        s7, s7, 1
	bge         s7, s6, .AnalyzeEnumSwitch_label_235

	// *** Basic block 31

.AnalyzeEnumSwitch_label_256:

	// *** Basic block 32

.AnalyzeEnumSwitch_label_257:
	j           .AnalyzeEnumSwitch_label_265

	// *** Basic block 33

.AnalyzeEnumSwitch_label_259:
	bne         s4, x0, .AnalyzeEnumSwitch_label_264

	// *** Basic block 34

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 128(s1)

	// *** Basic block 35

.AnalyzeEnumSwitch_label_264:

	// *** Basic block 36

.AnalyzeEnumSwitch_label_265:
	addi        a0, s0, -48
	call        VectorDestruct

	// *** Basic block 37

	addi        a0, s0, -80
	call        BitSetDestruct

	// *** Basic block 38

	addi        a0, s0, -64
	call        BitSetDestruct

	// *** Basic block 39

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
.func_end_AnalyzeEnumSwitch:
	.size AnalyzeEnumSwitch, .func_end_AnalyzeEnumSwitch-AnalyzeEnumSwitch

	.local  AnalyzeSwitchStatement
	.type AnalyzeSwitchStatement, @function

AnalyzeSwitchStatement:

	// *** Basic block 0

	.global AnalyzeExpression
	.global AnalyzeStatement
	.global SemanticCheckScalarType
	.global TypeIsIntegral
	.global SemanticError
	.global ASTNodeVisit
	.local ResolveSwitchStatement
	.global qsort
	.local CompareCaseValue
	.global DecodeSourceLocation
	.global TypeIsEnum
	.local AnalyzeEnumSwitch
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
	// Saved floating point registers.
	fsd fs0, 8(sp)
	fsd fs1, 0(sp)
	// End of stack frame
	mv          s1, a0
	ld          a0, 56(s1)
	call        AnalyzeExpression

	// *** Basic block 1

	sd          a0, 56(s1)
	ld          s2, 64(s1)
	mv          a0, s2
	call        AnalyzeStatement

	// *** Basic block 2

	ld          s3, 56(s1)
	mv          a0, s3
	call        SemanticCheckScalarType

	// *** Basic block 3

	ld          a0, 16(s3)
	call        TypeIsIntegral

	// *** Basic block 4

	not         t0, a0
	beqz        t0, .AnalyzeSwitchStatement_label_68

	// *** Basic block 5

	lla         a1, .str.12
	mv          a0, s3
	call        SemanticError

	// *** Basic block 6

.AnalyzeSwitchStatement_label_65:
	// Restored registers.
	fld fs0, 72(sp)
	fld fs1, 64(sp)
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

.AnalyzeSwitchStatement_label_68:
	lw          t0, 0(s2)
	li          t1, 41		// 0x29 ASCII ')'
	beq         t0, t1, .AnalyzeSwitchStatement_label_76

	// *** Basic block 8

	j           .AnalyzeSwitchStatement_label_65

	// *** Basic block 9

.AnalyzeSwitchStatement_label_76:
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          s1, -64(s0)
	addi        t0, s0, -64
	sw          x0, 8(t0)
	addi        a3, s0, -64
	mv          a2, x0
	la          t0, ResolveSwitchStatement
	mv          a1, t0
	mv          a0, s1
	call        ASTNodeVisit

	// *** Basic block 10

	addi        t0, s1, 72
	ld          t0, 8(t0)
	ld          t1, 96(s1)
	beq         t1, x0, .AnalyzeSwitchStatement_label_109

	// *** Basic block 11

	li          s2, 1		// 0x1 ASCII \x1
	j           .AnalyzeSwitchStatement_label_111

	// *** Basic block 12

.AnalyzeSwitchStatement_label_109:
	mv          s2, x0

	// *** Basic block 13

.AnalyzeSwitchStatement_label_111:
	add         t0, t0, s2
	fcvt.s.l    fs0, t0
	ld          t0, 120(s1)
	ld          t1, 112(s1)
	sub         t0, t0, t1
	fcvt.s.l    fs1, t0
	fmv.w.x     ft0, x0
	feq.s       t0, fs1, ft0
	not         t0, t0
	beqz        t0, .AnalyzeSwitchStatement_label_129

	// *** Basic block 14

	fdiv.s      ft0, fs0, fs1
	fsw         ft0, 104(s1)

	// *** Basic block 15

.AnalyzeSwitchStatement_label_129:
	addi        t0, s1, 72
	ld          s3, 72(s1)
	ld          s4, 8(t0)
	la          t0, CompareCaseValue
	mv          a3, t0
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	mv          a1, s4
	mv          a0, s3
	call        qsort

	// *** Basic block 16

	mv          s5, x0
	addi        s4, s4, -1
	bge         x0, s4, .AnalyzeSwitchStatement_label_206

	// *** Basic block 17

.AnalyzeSwitchStatement_label_152:
	slli        t0, s5, 3
	add         t0, s3, t0
	ld          s6, 0(t0)
	ld          s7, 72(s6)
	addi        s5, s5, 1
	slli        t0, s5, 3
	add         t0, s3, t0
	ld          s3, 0(t0)
	ld          s8, 72(s3)
	bne         s7, s8, .AnalyzeSwitchStatement_label_202

	// *** Basic block 18

	ld          t0, 56(s6)
	ld          a0, 40(t0)
	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	call        DecodeSourceLocation

	// *** Basic block 19

	ld          a0, 56(s3)
	lla         a1, .str.13
	ld          a3, -48(s0)
	lw          a4, -40(s0)
	mv          a2, s8
	call        SemanticError

	// *** Basic block 20

.AnalyzeSwitchStatement_label_202:

	// *** Basic block 21

.AnalyzeSwitchStatement_label_203:
	bge         s5, s4, .AnalyzeSwitchStatement_label_152

	// *** Basic block 22

.AnalyzeSwitchStatement_label_206:
	ld          t0, 56(s1)
	ld          a0, 16(t0)
	call        TypeIsEnum

	// *** Basic block 23

	beqz        a0, .AnalyzeSwitchStatement_label_217

	// *** Basic block 24

	mv          a0, s1
	call        AnalyzeEnumSwitch

	// *** Basic block 25

.AnalyzeSwitchStatement_label_217:
	j           .AnalyzeSwitchStatement_label_65
.func_end_AnalyzeSwitchStatement:
	.size AnalyzeSwitchStatement, .func_end_AnalyzeSwitchStatement-AnalyzeSwitchStatement

	.local  AnalyzeForStatement
	.type AnalyzeForStatement, @function

AnalyzeForStatement:

	// *** Basic block 0

	.global AnalyzeStatement
	.global StorageIs
	.global SemanticError
	.global AnalyzeExpression
	.global SemanticCheckScalarType
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
	ld          s2, 56(s1)
	beq         s2, x0, .AnalyzeForStatement_label_101

	// *** Basic block 1

	lw          t0, 0(s2)
	li          t1, 83		// 0x53 ASCII 'S'
	bne         t0, t1, .AnalyzeForStatement_label_94

	// *** Basic block 2

	mv          a0, s2
	call        AnalyzeStatement

	// *** Basic block 3

	mv          s3, x0
	ld          t0, 56(s2)
	ld          s4, 8(t0)
	bge         x0, s4, .AnalyzeForStatement_label_92

	// *** Basic block 4

	ld          s5, 0(t0)

	// *** Basic block 5

.AnalyzeForStatement_label_48:
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          s5, 0(t0)
	ld          t0, 56(s5)
	lw          s7, 48(t0)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s7
	call        StorageIs

	// *** Basic block 6

	not         s6, a0
	beqz        s6, .AnalyzeForStatement_label_76

	// *** Basic block 7

	li          t0, 16		// 0x10 ASCII \x10
	mv          a1, t0
	mv          a0, s7
	call        StorageIs

	// *** Basic block 8

	not         s6, a0

	// *** Basic block 9

.AnalyzeForStatement_label_76:
	beqz        s6, .AnalyzeForStatement_label_79

	// *** Basic block 10

	snez        s6, s7

	// *** Basic block 11

.AnalyzeForStatement_label_79:
	beqz        s6, .AnalyzeForStatement_label_87

	// *** Basic block 12

	lla         a1, .str.14
	mv          a0, s5
	call        SemanticError

	// *** Basic block 13

.AnalyzeForStatement_label_87:

	// *** Basic block 14

.AnalyzeForStatement_label_88:
	addi        s3, s3, 1
	bge         s3, s4, .AnalyzeForStatement_label_48

	// *** Basic block 15

.AnalyzeForStatement_label_92:
	j           .AnalyzeForStatement_label_100

	// *** Basic block 16

.AnalyzeForStatement_label_94:
	mv          a0, s2
	call        AnalyzeExpression

	// *** Basic block 17

	sd          a0, 56(s1)

	// *** Basic block 18

.AnalyzeForStatement_label_100:

	// *** Basic block 19

.AnalyzeForStatement_label_101:
	ld          a0, 64(s1)
	call        AnalyzeExpression

	// *** Basic block 20

	sd          a0, 64(s1)
	ld          s2, 64(s1)
	beq         s2, x0, .AnalyzeForStatement_label_115

	// *** Basic block 21

	mv          a0, s2
	call        SemanticCheckScalarType

	// *** Basic block 22

.AnalyzeForStatement_label_115:
	ld          a0, 72(s1)
	call        AnalyzeExpression

	// *** Basic block 23

	sd          a0, 72(s1)
	ld          a0, 80(s1)
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
	j           AnalyzeStatement
.func_end_AnalyzeForStatement:
	.size AnalyzeForStatement, .func_end_AnalyzeForStatement-AnalyzeForStatement

	.local  AnalyzeCompoundStatement
	.type AnalyzeCompoundStatement, @function

AnalyzeCompoundStatement:

	// *** Basic block 0

	.global AnalyzeStatement
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
	mv          s1, x0
	ld          t0, 56(a0)
	ld          s2, 8(t0)
	bge         x0, s2, .AnalyzeCompoundStatement_label_32

	// *** Basic block 1

	ld          s3, 0(t0)

	// *** Basic block 2

.AnalyzeCompoundStatement_label_21:
	slli        t0, s1, 3
	add         t0, s3, t0
	ld          a0, 0(t0)
	call        AnalyzeStatement

	// *** Basic block 3

.AnalyzeCompoundStatement_label_28:
	addi        s1, s1, 1
	bge         s1, s2, .AnalyzeCompoundStatement_label_21

	// *** Basic block 4

.AnalyzeCompoundStatement_label_32:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AnalyzeCompoundStatement:
	.size AnalyzeCompoundStatement, .func_end_AnalyzeCompoundStatement-AnalyzeCompoundStatement

	.local  AnalyzeTailRecursion
	.type AnalyzeTailRecursion, @function

AnalyzeTailRecursion:

	// *** Basic block 0

	.global compiler
	.global snprintf
	.local tail_label_name
	.local tail_label_num
	.global NewLabelASTNode
	.global CompoundASTNodeInsertStatement
	.global NewVector
	.global ASTNodeMove
	.global SyntaxNewTemporary
	.global NewBinaryASTNode
	.global NewIdentifierASTNode
	.global NewVariableDeclarationASTNode
	.global VectorAppend
	.global NewDeclarationListASTNode
	.global NewExpressionStatementASTNode
	.global NewGotoStatementASTNode
	.global NewString
	.global NewCompoundStatementASTNode
	.global ASTNodeReplaceChild
	.global AnalyzeStatement
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
	mv          s2, a0
	la          t0, compiler
	ld          t0, 0(t0)
	ld          t0, 1096(t0)
	addi        s3, t0, 32
	ld          s4, 40(s3)
	lb          t0, 32(s3)
	bnez        t0, .AnalyzeTailRecursion_label_60

	// *** Basic block 1

	lb          t0, 48(s3)

	// *** Basic block 2

.AnalyzeTailRecursion_label_60:
	beqz        t0, .AnalyzeTailRecursion_label_65

	// *** Basic block 3

.AnalyzeTailRecursion_label_62:
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

	// *** Basic block 4

.AnalyzeTailRecursion_label_65:
	ld          s3, 40(s1)
	lla         s5, tail_label_name
	lla         a2, .str.15
	lla         t0, tail_label_num
	lw          t1, 0(t0)
	addi        t0, t1, 1
	lla         t1, tail_label_num
	sw          t0, 0(t1)
	mv          a3, t1
	li          t0, 32		// 0x20 ASCII ' '
	mv          a1, t0
	mv          a0, s5
	call        snprintf

	// *** Basic block 5

	mv          a3, s3
	mv          a2, x0
	mv          a1, x0
	mv          a0, s5
	call        NewLabelASTNode

	// *** Basic block 6

	mv          s5, a0
	mv          a2, x0
	mv          a1, s5
	mv          a0, s4
	call        CompoundASTNodeInsertStatement

	// *** Basic block 7

	call        NewVector

	// *** Basic block 8

	mv          s4, a0
	ld          s6, 64(s1)
	ld          s7, 8(s6)
	ld          t0, 56(s1)
	ld          s8, 16(t0)
	call        NewVector

	// *** Basic block 9

	ld          s6, 0(s6)
	addi        t0, s8, 32
	ld          s9, 8(t0)
	addi        t0, s8, 32
	ld          s8, 8(t0)
	mv          s10, a0
	mv          s11, x0
	bge         x0, s7, .AnalyzeTailRecursion_label_195

	// *** Basic block 10

.AnalyzeTailRecursion_label_134:
	slli        s1, s11, 3
	add         t0, s6, s1
	ld          a0, 0(t0)
	call        ASTNodeMove
	sd          a0, -24(s0)	// Spilled @140

	// *** Basic block 11

	mv          s6, a0
	sd          s6, -32(s0)	// Spilled @141
	add         t0, s9, s1
	ld          s1, 0(t0)
	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 448
	ld          s1, 40(s1)
	mv          a1, s1
	call        SyntaxNewTemporary

	// *** Basic block 12

	mv          s9, a0
	mv          a1, s3
	mv          a0, s9
	call        NewIdentifierASTNode

	// *** Basic block 13

	mv          a4, s6
	mv          a3, a0
	mv          a2, s3
	mv          a1, s1
	li          t0, 20		// 0x14 ASCII \x14
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 14

	mv          s1, a0
	sd          s1, -24(s0)	// Spilled @175
	mv          a2, s3
	mv          a1, s1
	mv          a0, s9
	call        NewVariableDeclarationASTNode

	// *** Basic block 15

	sd          a0, -24(s0)	// Spilled @184
	mv          a1, a0
	mv          a0, s10
	call        VectorAppend

	// *** Basic block 16

.AnalyzeTailRecursion_label_191:
	addi        s11, s11, 1
	bge         s11, s7, .AnalyzeTailRecursion_label_134

	// *** Basic block 17

.AnalyzeTailRecursion_label_195:
	mv          a1, s3
	mv          a0, s10
	call        NewDeclarationListASTNode
	sd          a0, -24(s0)	// Spilled @200

	// *** Basic block 18

	ld          a0, 0(s10)
	mv          a1, a0
	mv          a0, s4
	call        VectorAppend

	// *** Basic block 19

	mv          a0, x0
	bge         x0, s7, .AnalyzeTailRecursion_label_262

	// *** Basic block 20

.AnalyzeTailRecursion_label_211:
	slli        s1, a0, 3
	add         t0, a0, s1
	ld          s6, 0(t0)
	ld          a0, 56(s6)
	mv          a1, s3
	call        NewIdentifierASTNode

	// *** Basic block 21

	mv          s6, a0
	add         t0, s8, s1
	ld          s1, 0(t0)
	ld          s8, 40(s1)
	mv          a1, s3
	mv          a0, s1
	call        NewIdentifierASTNode

	// *** Basic block 22

	mv          a4, s6
	mv          a3, a0
	mv          a2, s3
	mv          a1, s8
	li          t0, 20		// 0x14 ASCII \x14
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 23

	mv          s1, a0
	mv          a1, s3
	mv          a0, s1
	call        NewExpressionStatementASTNode

	// *** Basic block 24

	mv          a1, a0
	mv          a0, s4
	call        VectorAppend

	// *** Basic block 25

.AnalyzeTailRecursion_label_258:
	addi        a0, a0, 1
	bge         a0, s7, .AnalyzeTailRecursion_label_211

	// *** Basic block 26

.AnalyzeTailRecursion_label_262:
	addi        t0, s5, 64
	ld          a0, 16(t0)
	call        NewString

	// *** Basic block 27

	mv          a1, s3
	call        NewGotoStatementASTNode

	// *** Basic block 28

	mv          s7, a0
	mv          a1, s7
	mv          a0, s4
	call        VectorAppend

	// *** Basic block 29

	mv          a1, s3
	mv          a0, s4
	call        NewCompoundStatementASTNode

	// *** Basic block 30

	mv          s3, a0
	ld          a0, 24(s2)
	lw          a1, 32(s2)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, s3
	call        ASTNodeReplaceChild

	// *** Basic block 31

	mv          a0, s3
	call        AnalyzeStatement

	// *** Basic block 32

	j           .AnalyzeTailRecursion_label_62
.func_end_AnalyzeTailRecursion:
	.size AnalyzeTailRecursion, .func_end_AnalyzeTailRecursion-AnalyzeTailRecursion

	.local  AnalyzeReturnStatement
	.type AnalyzeReturnStatement, @function

AnalyzeReturnStatement:

	// *** Basic block 0

	.global ASTNodeSetType
	.global compiler
	.global AnalyzeExpression
	.global TypeIsVoid
	.global SemanticError
	.global NormalConversion
	.global TypeIsStructOrUnion
	.global OptLevel2
	.local AnalyzeTailRecursion
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
	ld          s2, 56(s1)
	sub         t1, s2, x0
	snez        t0, t1
	beq         s2, x0, .AnalyzeReturnStatement_label_39

	// *** Basic block 1

	lw          t1, 0(s2)
	addi        t1, t1, -74
	seqz        t0, t1

	// *** Basic block 2

.AnalyzeReturnStatement_label_39:
	beqz        t0, .AnalyzeReturnStatement_label_56

	// *** Basic block 3

	la          t0, compiler
	ld          t0, 0(t0)
	ld          t0, 1096(t0)
	ld          a1, 24(t0)
	mv          a0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeSetType

	// *** Basic block 4

.AnalyzeReturnStatement_label_53:
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

.AnalyzeReturnStatement_label_56:
	mv          a0, s2
	call        AnalyzeExpression

	// *** Basic block 6

	mv          s2, a0
	la          t0, compiler
	ld          s3, 0(t0)
	ld          s4, 1096(s3)
	ld          s5, 24(s4)
	mv          a0, s5
	call        TypeIsVoid

	// *** Basic block 7

	beqz        a0, .AnalyzeReturnStatement_label_89

	// *** Basic block 8

	beq         s2, x0, .AnalyzeReturnStatement_label_87

	// *** Basic block 9

	ld          a0, 16(s2)
	call        TypeIsVoid

	// *** Basic block 10

	not         t0, a0
	beqz        t0, .AnalyzeReturnStatement_label_86

	// *** Basic block 11

	lla         a1, .str.16
	mv          a0, s2
	call        SemanticError

	// *** Basic block 12

.AnalyzeReturnStatement_label_86:

	// *** Basic block 13

.AnalyzeReturnStatement_label_87:
	j           .AnalyzeReturnStatement_label_107

	// *** Basic block 14

.AnalyzeReturnStatement_label_89:
	bne         s2, x0, .AnalyzeReturnStatement_label_100

	// *** Basic block 15

	lla         a1, .str.17
	mv          a0, s1
	call        SemanticError

	// *** Basic block 16

	j           .AnalyzeReturnStatement_label_106

	// *** Basic block 17

.AnalyzeReturnStatement_label_100:
	mv          a1, s5
	mv          a0, s2
	call        NormalConversion

	// *** Basic block 18

.AnalyzeReturnStatement_label_106:

	// *** Basic block 19

.AnalyzeReturnStatement_label_107:
	lb          t0, 1229(s3)
	beqz        t0, .AnalyzeReturnStatement_label_139

	// *** Basic block 20

	mv          a0, s5
	call        TypeIsStructOrUnion

	// *** Basic block 21

	beqz        a0, .AnalyzeReturnStatement_label_138

	// *** Basic block 22

	lw          t0, 0(s2)
	li          t1, 46		// 0x2e ASCII '.'
	bne         t0, t1, .AnalyzeReturnStatement_label_126

	// *** Basic block 23

	lw          t1, 8(s2)
	ori         t1, t1, 32
	sw          t1, 8(s2)
	j           .AnalyzeReturnStatement_label_137

	// *** Basic block 24

.AnalyzeReturnStatement_label_126:
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .AnalyzeReturnStatement_label_136

	// *** Basic block 25

	lw          t0, 8(s2)
	ori         t0, t0, 64
	sw          t0, 8(s2)

	// *** Basic block 26

.AnalyzeReturnStatement_label_136:

	// *** Basic block 27

.AnalyzeReturnStatement_label_137:

	// *** Basic block 28

.AnalyzeReturnStatement_label_138:

	// *** Basic block 29

.AnalyzeReturnStatement_label_139:
	call        OptLevel2

	// *** Basic block 30

	mv          s3, a0
	beqz        a0, .AnalyzeReturnStatement_label_147

	// *** Basic block 31

	sub         t0, s2, x0
	snez        s3, t0

	// *** Basic block 32

.AnalyzeReturnStatement_label_147:
	beqz        s3, .AnalyzeReturnStatement_label_152

	// *** Basic block 33

	lw          t0, 0(s2)
	addi        t0, t0, -46
	seqz        s3, t0

	// *** Basic block 34

.AnalyzeReturnStatement_label_152:
	beqz        s3, .AnalyzeReturnStatement_label_178

	// *** Basic block 35

	mv          s3, s2
	ld          s5, 56(s3)
	lw          t0, 0(s5)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .AnalyzeReturnStatement_label_177

	// *** Basic block 36

	ld          t0, 56(s5)
	ld          t1, 32(s4)
	bne         t0, t1, .AnalyzeReturnStatement_label_176

	// *** Basic block 37

	mv          a1, s3
	mv          a0, s1
	call        AnalyzeTailRecursion

	// *** Basic block 38

.AnalyzeReturnStatement_label_176:

	// *** Basic block 39

.AnalyzeReturnStatement_label_177:

	// *** Basic block 40

.AnalyzeReturnStatement_label_178:
	j           .AnalyzeReturnStatement_label_53
.func_end_AnalyzeReturnStatement:
	.size AnalyzeReturnStatement, .func_end_AnalyzeReturnStatement-AnalyzeReturnStatement

	.local  AnalyzeCaseLabel
	.type AnalyzeCaseLabel, @function

AnalyzeCaseLabel:

	// *** Basic block 0

	.global printf
	.global AnalyzeExpression
	.global AnalyzeStatement
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
	lw          t0, 4(s1)
	li          t1, 4368		// 0x1110
	bne         t0, t1, .AnalyzeCaseLabel_label_26

	// *** Basic block 1

	lla         a0, .str.18
	call        printf

	// *** Basic block 2

.AnalyzeCaseLabel_label_26:
	ld          s2, 56(s1)
	beq         s2, x0, .AnalyzeCaseLabel_label_37

	// *** Basic block 3

	mv          a0, s2
	call        AnalyzeExpression

	// *** Basic block 4

	sd          a0, 56(s1)

	// *** Basic block 5

.AnalyzeCaseLabel_label_37:
	ld          t0, 64(s1)
	beq         t0, x0, .AnalyzeCaseLabel_label_47

	// *** Basic block 6

	mv          a0, t0
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AnalyzeStatement

	// *** Basic block 7

.AnalyzeCaseLabel_label_47:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AnalyzeCaseLabel:
	.size AnalyzeCaseLabel, .func_end_AnalyzeCaseLabel-AnalyzeCaseLabel

	.global AnalyzeVariableDeclaration
	.type AnalyzeVariableDeclaration, @function

AnalyzeVariableDeclaration:

	// *** Basic block 0

	.global AnalyzeExpression
	.global NormalConversion
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
	ld          a0, 64(s1)
	call        AnalyzeExpression

	// *** Basic block 1

	sd          a0, 64(s1)
	ld          t0, 64(s1)
	beq         t0, x0, .AnalyzeVariableDeclaration_label_32

	// *** Basic block 2

	ld          t1, 56(s1)
	ld          a1, 40(t1)
	mv          a0, t0
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NormalConversion

	// *** Basic block 3

.AnalyzeVariableDeclaration_label_32:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AnalyzeVariableDeclaration:
	.size AnalyzeVariableDeclaration, .func_end_AnalyzeVariableDeclaration-AnalyzeVariableDeclaration

	.global AnalyzeDeclarationList
	.type AnalyzeDeclarationList, @function

AnalyzeDeclarationList:

	// *** Basic block 0

	.global AnalyzeStatement
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
	ld          t0, 56(a0)
	ld          s1, 8(t0)
	mv          s2, x0
	bge         x0, s1, .AnalyzeDeclarationList_label_33

	// *** Basic block 1

	ld          s3, 0(t0)

	// *** Basic block 2

.AnalyzeDeclarationList_label_22:
	slli        t0, s2, 3
	add         t0, s3, t0
	ld          a0, 0(t0)
	call        AnalyzeStatement

	// *** Basic block 3

.AnalyzeDeclarationList_label_29:
	addi        s2, s2, 1
	bge         s2, s1, .AnalyzeDeclarationList_label_22

	// *** Basic block 4

.AnalyzeDeclarationList_label_33:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AnalyzeDeclarationList:
	.size AnalyzeDeclarationList, .func_end_AnalyzeDeclarationList-AnalyzeDeclarationList

	.local  FindLabel
	.type FindLabel, @function

FindLabel:

	// *** Basic block 0

	.global StringEqualString
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
	beqz        a3, .FindLabel_label_23

	// *** Basic block 1

.FindLabel_label_20:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.FindLabel_label_23:
	ld          t0, 64(s2)
	beq         t0, x0, .FindLabel_label_31

	// *** Basic block 3

	j           .FindLabel_label_20

	// *** Basic block 4

.FindLabel_label_31:
	lw          t0, 0(s1)
	li          t1, 81		// 0x51 ASCII 'Q'
	bne         t0, t1, .FindLabel_label_50

	// *** Basic block 5

	mv          s3, s1
	ld          a0, 56(s2)
	addi        a1, s3, 64
	call        StringEqualString

	// *** Basic block 6

	beqz        a0, .FindLabel_label_49

	// *** Basic block 7

	sd          s1, 64(s2)

	// *** Basic block 8

.FindLabel_label_49:

	// *** Basic block 9

.FindLabel_label_50:
	j           .FindLabel_label_20
.func_end_FindLabel:
	.size FindLabel, .func_end_FindLabel-FindLabel

	.local  ContainsVLA
	.type ContainsVLA, @function

ContainsVLA:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	lw          t1, 0(t0)
	li          t2, 83		// 0x53 ASCII 'S'
	beq         t1, t2, .ContainsVLA_label_29

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.ContainsVLA_label_26:
	ret         

	// *** Basic block 3

.ContainsVLA_label_29:
	mv          t1, t0
	mv          t2, x0
	ld          t3, 56(t1)
	ld          t4, 8(t3)
	bge         x0, t4, .ContainsVLA_label_83

	// *** Basic block 4

	ld          t0, 0(t3)

	// *** Basic block 5

.ContainsVLA_label_41:
	slli        t1, t2, 3
	add         t0, t0, t1
	ld          t1, 0(t0)
	ld          t0, 16(t1)
	lw          t3, 16(t0)
	addi        t5, t3, -2
	seqz        t1, t5
	li          t5, 2		// 0x2 ASCII \x2
	beq         t3, t5, .ContainsVLA_label_62

	// *** Basic block 6

	addi        t3, t3, -1
	seqz        t1, t3

	// *** Basic block 7

.ContainsVLA_label_62:
	beqz        t1, .ContainsVLA_label_69

	// *** Basic block 8

	addi        t0, t0, 32
	lb          t0, 16(t0)
	slli        t0, t0, 61
	srai        t1, t0, 63

	// *** Basic block 9

.ContainsVLA_label_69:

	// *** Basic block 10

.ContainsVLA_label_71:
	beqz        t1, .ContainsVLA_label_78

	// *** Basic block 11

	j           .ContainsVLA_label_74

	// *** Basic block 12

.ContainsVLA_label_74:
	li          a0, 1		// 0x1 ASCII \x1
	ret         

	// *** Basic block 13

.ContainsVLA_label_78:

	// *** Basic block 14

.ContainsVLA_label_79:
	addi        t2, t2, 1
	bge         t2, t4, .ContainsVLA_label_41

	// *** Basic block 15

.ContainsVLA_label_83:
	mv          a0, x0
	ret         
.func_end_ContainsVLA:
	.size ContainsVLA, .func_end_ContainsVLA-ContainsVLA

	.local  GetVLAs
	.type GetVLAs, @function

GetVLAs:

	// *** Basic block 0

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
	mv          s1, a1
	mv          s2, x0
	ld          t0, 56(a0)
	ld          s3, 8(t0)
	bge         x0, s3, .GetVLAs_label_74

	// *** Basic block 1

	ld          t0, 0(t0)

	// *** Basic block 2

.GetVLAs_label_30:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	ld          s5, 16(s4)
	lw          s6, 16(s5)
	addi        t1, s6, -2
	seqz        t0, t1
	li          t1, 2		// 0x2 ASCII \x2
	beq         s6, t1, .GetVLAs_label_51

	// *** Basic block 3

	addi        t1, s6, -1
	seqz        t0, t1

	// *** Basic block 4

.GetVLAs_label_51:
	beqz        t0, .GetVLAs_label_58

	// *** Basic block 5

	addi        t1, s5, 32
	lb          t1, 16(t1)
	slli        t1, t1, 61
	srai        t0, t1, 63

	// *** Basic block 6

.GetVLAs_label_58:

	// *** Basic block 7

.GetVLAs_label_60:
	beqz        t0, .GetVLAs_label_69

	// *** Basic block 8

	j           .GetVLAs_label_63

	// *** Basic block 9

.GetVLAs_label_63:
	mv          a1, s4
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 10

.GetVLAs_label_69:

	// *** Basic block 11

.GetVLAs_label_70:
	addi        s2, s2, 1
	bge         s2, s3, .GetVLAs_label_30

	// *** Basic block 12

.GetVLAs_label_74:
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
.func_end_GetVLAs:
	.size GetVLAs, .func_end_GetVLAs-GetVLAs

	.local  ReportJumpError
	.type ReportJumpError, @function

ReportJumpError:

	// *** Basic block 0

	.global SemanticError
	.global DecodeSourceLocation
	.global ReportNote
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
	mv          s1, a1
	mv          s2, a2
	lla         a1, .str.19
	call        SemanticError

	// *** Basic block 1

	ld          a0, 40(s1)
	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	call        DecodeSourceLocation

	// *** Basic block 2

	ld          a0, -48(s0)
	lw          a1, -40(s0)
	lla         a2, .str.20
	call        ReportNote

	// *** Basic block 3

	mv          s3, x0
	ld          s4, 8(s2)
	bge         x0, s4, .ReportJumpError_label_100

	// *** Basic block 4

	ld          s1, 0(s2)

	// *** Basic block 5

.ReportJumpError_label_65:
	slli        t0, s3, 3
	add         t0, s1, t0
	ld          s1, 0(t0)
	ld          a0, 40(s1)
	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	call        DecodeSourceLocation

	// *** Basic block 6

	ld          a0, -48(s0)
	lw          a1, -40(s0)
	lla         a2, .str.21
	ld          t0, 56(s1)
	ld          a3, 16(t0)
	call        ReportNote

	// *** Basic block 7

.ReportJumpError_label_96:
	addi        s3, s3, 1
	bge         s3, s4, .ReportJumpError_label_65

	// *** Basic block 8

.ReportJumpError_label_100:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ReportJumpError:
	.size ReportJumpError, .func_end_ReportJumpError-ReportJumpError

	.local  CheckGoto
	.type CheckGoto, @function

CheckGoto:

	// *** Basic block 0

	.global VectorAppend
	.global printf
	.global abort
	.local ContainsVLA
	.local GetVLAs
	.local ReportJumpError
	.global VectorDestruct
	addi sp, sp, -192
	// Saved return address (offset 184) and frame pointer (offset 176)
	sd ra, 184(sp)
	sd s0, 176(sp)
	addi s0, sp, 192
	// Local vars at offset -96(s0)
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
	sd          x0, -96(s0)
	sd          x0, -88(s0)
	sd          x0, -80(s0)
	sd          x0, -96(s0)
	sd          x0, -72(s0)
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -72(s0)
	mv          s4, s1
	beq         s4, x0, .CheckGoto_label_67

	// *** Basic block 1

.CheckGoto_label_56:
	addi        a0, s0, -96
	mv          a1, s4
	call        VectorAppend

	// *** Basic block 2

	ld          s4, 24(s4)
	bne         s4, x0, .CheckGoto_label_56

	// *** Basic block 3

.CheckGoto_label_67:
	mv          s4, s2
	beq         s4, x0, .CheckGoto_label_84

	// *** Basic block 4

	ld          s5, -96(s0)

	// *** Basic block 5

.CheckGoto_label_73:
	addi        a0, s0, -72
	mv          a1, s4
	call        VectorAppend

	// *** Basic block 6

	ld          s4, 24(s4)
	bne         s4, x0, .CheckGoto_label_73

	// *** Basic block 7

.CheckGoto_label_84:
	mv          s5, x0
	addi        t0, s0, -96
	ld          t0, 8(t0)
	addi        s6, t0, -1
	addi        t0, s0, -72
	ld          t0, 8(t0)
	addi        s7, t0, -1
	sltz        t1, s6
	not         t0, t1
	blt         s6, x0, .CheckGoto_label_105

	// *** Basic block 8

	ld          t1, -72(s0)
	sltz        t2, s7
	not         t0, t2

	// *** Basic block 9

.CheckGoto_label_105:
	beqz        t0, .CheckGoto_label_134

	// *** Basic block 10

.CheckGoto_label_107:
	slli        t0, s6, 3
	add         t0, s5, t0
	ld          t0, 0(t0)
	slli        t1, s7, 3
	add         t1, t1, t1
	ld          t1, 0(t1)
	beq         t0, t1, .CheckGoto_label_123

	// *** Basic block 11

	addi        t0, s6, 1
	slli        t0, t0, 3
	add         t0, s5, t0
	ld          s5, 0(t0)
	j           .CheckGoto_label_134

	// *** Basic block 12

.CheckGoto_label_123:
	addi        s6, s6, -1
	addi        s7, s7, -1
	sltz        t1, s6
	not         t0, t1
	blt         s6, x0, .CheckGoto_label_132

	// *** Basic block 13

	sltz        t1, s7
	not         t0, t1

	// *** Basic block 14

.CheckGoto_label_132:
	bnez        t0, .CheckGoto_label_107

	// *** Basic block 15

.CheckGoto_label_134:
	beq         s5, x0, .CheckGoto_label_139

	// *** Basic block 16

	j           .CheckGoto_label_155

	// *** Basic block 17

.CheckGoto_label_139:
	lla         a0, .str.22
	lla         a1, .str.23
	lla         a3, .str.24
	li          t0, 569		// 0x239
	mv          a2, t0
	call        printf

	// *** Basic block 18

	call        abort

	// *** Basic block 19

.CheckGoto_label_155:
	sd          s5, 72(s3)
	ld          t0, -72(s0)
	slli        t1, s7, 3
	add         t0, t0, t1
	ld          s7, 0(t0)
	ld          s8, -96(s0)
	slli        t0, s6, 3
	add         t0, s8, t0
	ld          s9, 0(t0)
	lw          t0, 0(s5)
	li          s10, 41		// 0x29 ASCII ')'
	bne         t0, s10, .CheckGoto_label_175

	// *** Basic block 20

	j           .CheckGoto_label_190

	// *** Basic block 21

.CheckGoto_label_175:
	lla         a0, .str.25
	lla         a1, .str.26
	lla         a3, .str.27
	li          t0, 575		// 0x23f
	mv          a2, t0
	call        printf

	// *** Basic block 22

	call        abort

	// *** Basic block 23

.CheckGoto_label_190:
	mv          s3, s5
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -48(s0)
	lw          t0, 32(s7)
	addi        s7, t0, 1
	lw          s11, 32(s9)
	bge         s7, s11, .CheckGoto_label_230

	// *** Basic block 24

	ld          t0, 56(s3)
	ld          s5, 0(t0)

	// *** Basic block 25

.CheckGoto_label_211:
	slli        t0, s7, 3
	add         t0, s5, t0
	ld          s3, 0(t0)
	mv          a0, s3
	call        ContainsVLA

	// *** Basic block 26

	beqz        a0, .CheckGoto_label_225

	// *** Basic block 27

	addi        a1, s0, -48
	mv          a0, s3
	call        GetVLAs

	// *** Basic block 28

.CheckGoto_label_225:

	// *** Basic block 29

.CheckGoto_label_226:
	addi        s7, s7, 1
	bge         s7, s11, .CheckGoto_label_211

	// *** Basic block 30

.CheckGoto_label_230:
	mv          s5, s6
	bge         x0, s5, .CheckGoto_label_284

	// *** Basic block 31

.CheckGoto_label_235:
	slli        t0, s5, 3
	add         t0, s8, t0
	ld          s6, 0(t0)
	addi        s5, s5, -1
	slli        t0, s5, 3
	add         t0, s8, t0
	ld          s9, 0(t0)
	lw          t0, 0(s6)
	bne         t0, s10, .CheckGoto_label_280

	// *** Basic block 32

	mv          s7, s6
	mv          s6, x0
	lw          s8, 32(s9)
	bge         x0, s8, .CheckGoto_label_279

	// *** Basic block 33

	ld          t0, 56(s7)
	ld          s9, 0(t0)

	// *** Basic block 34

.CheckGoto_label_260:
	slli        t0, s6, 3
	add         t0, s9, t0
	ld          s7, 0(t0)
	mv          a0, s7
	call        ContainsVLA

	// *** Basic block 35

	beqz        a0, .CheckGoto_label_274

	// *** Basic block 36

	addi        a1, s0, -48
	mv          a0, s7
	call        GetVLAs

	// *** Basic block 37

.CheckGoto_label_274:

	// *** Basic block 38

.CheckGoto_label_275:
	addi        s6, s6, 1
	bge         s6, s8, .CheckGoto_label_260

	// *** Basic block 39

.CheckGoto_label_279:

	// *** Basic block 40

.CheckGoto_label_280:

	// *** Basic block 41

.CheckGoto_label_281:
	bge         x0, s5, .CheckGoto_label_235

	// *** Basic block 42

.CheckGoto_label_284:
	addi        t0, s0, -48
	ld          t0, 8(t0)
	beqz        t0, .CheckGoto_label_297

	// *** Basic block 43

	addi        a2, s0, -48
	mv          a1, s1
	mv          a0, s2
	call        ReportJumpError

	// *** Basic block 44

.CheckGoto_label_297:
	addi        a0, s0, -48
	call        VectorDestruct

	// *** Basic block 45

	addi        a0, s0, -96
	call        VectorDestruct

	// *** Basic block 46

	addi        a0, s0, -72
	call        VectorDestruct

	// *** Basic block 47

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
.func_end_CheckGoto:
	.size CheckGoto, .func_end_CheckGoto-CheckGoto

	.global AnalyzeGotoStatement
	.type AnalyzeGotoStatement, @function

AnalyzeGotoStatement:

	// *** Basic block 0

	.global ASTNodeVisit
	.global compiler
	.local FindLabel
	.global SemanticError
	.local CheckGoto
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
	la          t0, compiler
	ld          t0, 0(t0)
	ld          t0, 1096(t0)
	addi        t0, t0, 32
	ld          a0, 40(t0)
	mv          a3, s1
	mv          a2, x0
	la          a1, FindLabel
	call        ASTNodeVisit

	// *** Basic block 1

	ld          s2, 64(s1)
	bne         s2, x0, .AnalyzeGotoStatement_label_54

	// *** Basic block 2

	lla         a1, .str.28
	ld          t0, 56(s1)
	ld          a2, 16(t0)
	mv          a0, s1
	call        SemanticError

	// *** Basic block 3

	j           .AnalyzeGotoStatement_label_63

	// *** Basic block 4

.AnalyzeGotoStatement_label_54:
	mv          a2, s1
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           CheckGoto

	// *** Basic block 5

.AnalyzeGotoStatement_label_63:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AnalyzeGotoStatement:
	.size AnalyzeGotoStatement, .func_end_AnalyzeGotoStatement-AnalyzeGotoStatement

	.local  FindDuplicateLabel
	.type FindDuplicateLabel, @function

FindDuplicateLabel:

	// *** Basic block 0

	.global StringEqualString
	.global SemanticError
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
	beqz        a3, .FindDuplicateLabel_label_27

	// *** Basic block 1

.FindDuplicateLabel_label_24:
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

.FindDuplicateLabel_label_27:
	lw          t0, 0(s1)
	li          t1, 81		// 0x51 ASCII 'Q'
	bne         t0, t1, .FindDuplicateLabel_label_62

	// *** Basic block 3

	mv          s3, s1
	ld          s4, 0(s2)
	addi        a1, s3, 64
	mv          a0, s4
	call        StringEqualString

	// *** Basic block 4

	beqz        a0, .FindDuplicateLabel_label_61

	// *** Basic block 5

	lb          t0, 8(s2)
	beqz        t0, .FindDuplicateLabel_label_57

	// *** Basic block 6

	lla         a1, .str.29
	ld          a2, 16(s4)
	mv          a0, s3
	call        SemanticError

	// *** Basic block 7

.FindDuplicateLabel_label_57:
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 8(s2)

	// *** Basic block 8

.FindDuplicateLabel_label_61:

	// *** Basic block 9

.FindDuplicateLabel_label_62:
	j           .FindDuplicateLabel_label_24
.func_end_FindDuplicateLabel:
	.size FindDuplicateLabel, .func_end_FindDuplicateLabel-FindDuplicateLabel

	.global AnalyzeLabel
	.type AnalyzeLabel, @function

AnalyzeLabel:

	// *** Basic block 0

	.global ASTNodeVisit
	.global compiler
	.local FindDuplicateLabel
	.global AnalyzeStatement
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
	mv          s1, a0
	sd          x0, -32(s0)
	sd          x0, -24(s0)
	addi        t0, s1, 64
	sd          t0, -32(s0)
	addi        t0, s0, -32
	sb          x0, 8(t0)
	la          t0, compiler
	ld          t0, 0(t0)
	ld          t0, 1096(t0)
	addi        t0, t0, 32
	ld          a0, 40(t0)
	addi        a3, s0, -32
	mv          a2, x0
	la          a1, FindDuplicateLabel
	call        ASTNodeVisit

	// *** Basic block 1

	ld          s2, 56(s1)
	beq         s2, x0, .AnalyzeLabel_label_53

	// *** Basic block 2

	mv          a0, s2
	call        AnalyzeStatement

	// *** Basic block 3

.AnalyzeLabel_label_53:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AnalyzeLabel:
	.size AnalyzeLabel, .func_end_AnalyzeLabel-AnalyzeLabel

	.global AnalyzeStatement
	.type AnalyzeStatement, @function

AnalyzeStatement:

	// *** Basic block 0

	.global AnalyzeDeclarationList
	.global AnalyzeVariableDeclaration
	.local AnalyzeExpressionStatement
	.local AnalyzeCompoundStatement
	.local AnalyzeIfStatement
	.local AnalyzeWhileStatement
	.local AnalyzeDoStatement
	.local AnalyzeSwitchStatement
	.local AnalyzeForStatement
	.local AnalyzeReturnStatement
	.local AnalyzeCaseLabel
	.global AnalyzeGotoStatement
	.global AnalyzeLabel
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
	sub         t1, s1, x0
	seqz        t0, t1
	beq         s1, x0, .AnalyzeStatement_label_49

	// *** Basic block 1

	lw          t1, 8(s1)
	andi        t1, t1, 8
	snez        t0, t1

	// *** Basic block 2

.AnalyzeStatement_label_49:
	beqz        t0, .AnalyzeStatement_label_54

	// *** Basic block 3

.AnalyzeStatement_label_51:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.AnalyzeStatement_label_54:
	lw          s2, 0(s1)
	li          s3, 62		// 0x3e ASCII '>'
	blt         s2, s3, .AnalyzeStatement_label_100

	// *** Basic block 5

	beq         s2, s3, .AnalyzeStatement_label_187

	// *** Basic block 6

	li          t0, 70		// 0x46 ASCII 'F'
	beq         s2, t0, .AnalyzeStatement_label_177

	// *** Basic block 7

	li          t0, 72		// 0x48 ASCII 'H'
	beq         s2, t0, .AnalyzeStatement_label_167

	// *** Basic block 8

	li          t0, 74		// 0x4a ASCII 'J'
	beq         s2, t0, .AnalyzeStatement_label_209

	// *** Basic block 9

	li          t0, 81		// 0x51 ASCII 'Q'
	beq         s2, t0, .AnalyzeStatement_label_202

	// *** Basic block 10

	li          t0, 82		// 0x52 ASCII 'R'
	beq         s2, t0, .AnalyzeStatement_label_147

	// *** Basic block 11

	li          t0, 83		// 0x53 ASCII 'S'
	beq         s2, t0, .AnalyzeStatement_label_142

	// *** Basic block 12

	li          t0, 85		// 0x55 ASCII 'U'
	beq         s2, t0, .AnalyzeStatement_label_152

	// *** Basic block 13

	j           .AnalyzeStatement_label_211

	// *** Basic block 14

.AnalyzeStatement_label_100:
	li          t0, 23		// 0x17 ASCII \x17
	beq         s2, t0, .AnalyzeStatement_label_207

	// *** Basic block 15

	li          t0, 26		// 0x1a ASCII \x1a
	beq         s2, t0, .AnalyzeStatement_label_192

	// *** Basic block 16

	li          t0, 31		// 0x1f ASCII \x1f
	beq         s2, t0, .AnalyzeStatement_label_208

	// *** Basic block 17

	li          t0, 32		// 0x20 ASCII ' '
	beq         s2, t0, .AnalyzeStatement_label_172

	// *** Basic block 18

	li          t0, 35		// 0x23 ASCII '#'
	beq         s2, t0, .AnalyzeStatement_label_182

	// *** Basic block 19

	li          t0, 36		// 0x24 ASCII '$'
	beq         s2, t0, .AnalyzeStatement_label_197

	// *** Basic block 20

	li          t0, 39		// 0x27 ASCII '''
	beq         s2, t0, .AnalyzeStatement_label_162

	// *** Basic block 21

	li          t0, 41		// 0x29 ASCII ')'
	beq         s2, t0, .AnalyzeStatement_label_157

	// *** Basic block 22

	j           .AnalyzeStatement_label_211

	// *** Basic block 23

.AnalyzeStatement_label_142:
	mv          a0, s1
	call        AnalyzeDeclarationList

	// *** Basic block 24

	j           .AnalyzeStatement_label_229

	// *** Basic block 25

.AnalyzeStatement_label_147:
	mv          a0, s1
	call        AnalyzeVariableDeclaration

	// *** Basic block 26

	j           .AnalyzeStatement_label_229

	// *** Basic block 27

.AnalyzeStatement_label_152:
	mv          a0, s1
	call        AnalyzeExpressionStatement

	// *** Basic block 28

	j           .AnalyzeStatement_label_229

	// *** Basic block 29

.AnalyzeStatement_label_157:
	mv          a0, s1
	call        AnalyzeCompoundStatement

	// *** Basic block 30

	j           .AnalyzeStatement_label_229

	// *** Basic block 31

.AnalyzeStatement_label_162:
	mv          a0, s1
	call        AnalyzeIfStatement

	// *** Basic block 32

	j           .AnalyzeStatement_label_229

	// *** Basic block 33

.AnalyzeStatement_label_167:
	mv          a0, s1
	call        AnalyzeWhileStatement

	// *** Basic block 34

	j           .AnalyzeStatement_label_229

	// *** Basic block 35

.AnalyzeStatement_label_172:
	mv          a0, s1
	call        AnalyzeDoStatement

	// *** Basic block 36

	j           .AnalyzeStatement_label_229

	// *** Basic block 37

.AnalyzeStatement_label_177:
	mv          a0, s1
	call        AnalyzeSwitchStatement

	// *** Basic block 38

	j           .AnalyzeStatement_label_229

	// *** Basic block 39

.AnalyzeStatement_label_182:
	mv          a0, s1
	call        AnalyzeForStatement

	// *** Basic block 40

	j           .AnalyzeStatement_label_229

	// *** Basic block 41

.AnalyzeStatement_label_187:
	mv          a0, s1
	call        AnalyzeReturnStatement

	// *** Basic block 42

	j           .AnalyzeStatement_label_229

	// *** Basic block 43

.AnalyzeStatement_label_192:
	mv          a0, s1
	call        AnalyzeCaseLabel

	// *** Basic block 44

	j           .AnalyzeStatement_label_229

	// *** Basic block 45

.AnalyzeStatement_label_197:
	mv          a0, s1
	call        AnalyzeGotoStatement

	// *** Basic block 46

	j           .AnalyzeStatement_label_229

	// *** Basic block 47

.AnalyzeStatement_label_202:
	mv          a0, s1
	call        AnalyzeLabel

	// *** Basic block 48

	j           .AnalyzeStatement_label_229

	// *** Basic block 49

.AnalyzeStatement_label_207:

	// *** Basic block 50

.AnalyzeStatement_label_208:

	// *** Basic block 51

.AnalyzeStatement_label_209:
	j           .AnalyzeStatement_label_229

	// *** Basic block 52

.AnalyzeStatement_label_211:
	lla         a0, .str.30
	lla         a1, .str.31
	lla         a3, .str.32
	li          t0, 717		// 0x2cd
	mv          a2, t0
	call        printf

	// *** Basic block 53

	call        abort

	// *** Basic block 54

.AnalyzeStatement_label_229:
	lw          t0, 8(s1)
	ori         t0, t0, 8
	sw          t0, 8(s1)
	j           .AnalyzeStatement_label_51
.func_end_AnalyzeStatement:
	.size AnalyzeStatement, .func_end_AnalyzeStatement-AnalyzeStatement

.PCend:
	.data
tail_label_num:
	.type   tail_label_num,@object
	.local  tail_label_num
	.size   tail_label_num,4
	.p2align  2
	.word   0

	.type   tail_label_name,@object
	.local  tail_label_name
	.comm   tail_label_name,32,1

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "Duplicate default in switch statement"
	.type .str.1, @object
	.size .str.1, 38

.str.2:
	.asciz "Case labels must be constant integral expressions"
	.type .str.2, @object
	.size .str.2, 50

.str.3:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.3, @object
	.size .str.3, 30

.str.4:
	.asciz "(null)"
	.type .str.4, @object
	.size .str.4, 1

.str.5:
	.asciz "info != NULL"
	.type .str.5, @object
	.size .str.5, 13

.str.6:
	.asciz "switch-bad-case"
	.type .str.6, @object
	.size .str.6, 16

.str.7:
	.asciz "Case value %" PRId64 " is not valid for enumeration %s"
	.type .str.7, @object
	.size .str.7, 48

.str.8:
	.asciz "missing-switch-enum"
	.type .str.8, @object
	.size .str.8, 20

.str.9:
	.asciz "Enum constant %s and %" PRId64 " others are not present in switch statement"
	.type .str.9, @object
	.size .str.9, 69

.str.10:
	.asciz "missing-switch-enum"
	.type .str.10, @object
	.size .str.10, 20

.str.11:
	.asciz "Enum constant %s is not present in switch statement"
	.type .str.11, @object
	.size .str.11, 52

.str.12:
	.asciz "Switch statements need an integer type"
	.type .str.12, @object
	.size .str.12, 39

.str.13:
	.asciz "Duplicate case value %d; previous is at %s:%d"
	.type .str.13, @object
	.size .str.13, 46

.str.14:
	.asciz "Only auto or register variables allowed in a for statement declaration"
	.type .str.14, @object
	.size .str.14, 71

.str.15:
	.asciz "__tail_label_%d"
	.type .str.15, @object
	.size .str.15, 16

.str.16:
	.asciz "Cannot return a value from a void function"
	.type .str.16, @object
	.size .str.16, 43

.str.17:
	.asciz "Must return a value from a non-void function"
	.type .str.17, @object
	.size .str.17, 45

.str.18:
	.asciz "(null)"
	.type .str.18, @object
	.size .str.18, 1

.str.19:
	.asciz "Goto cannot jump to this label as it would bypass a variable length array definition"
	.type .str.19, @object
	.size .str.19, 85

.str.20:
	.asciz "Label is here"
	.type .str.20, @object
	.size .str.20, 14

.str.21:
	.asciz "Variable length array \'%s\' is bypassed"
	.type .str.21, @object
	.size .str.21, 39

.str.22:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.22, @object
	.size .str.22, 30

.str.23:
	.asciz "(null)"
	.type .str.23, @object
	.size .str.23, 1

.str.24:
	.asciz "lca != NULL"
	.type .str.24, @object
	.size .str.24, 12

.str.25:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.25, @object
	.size .str.25, 30

.str.26:
	.asciz "(null)"
	.type .str.26, @object
	.size .str.26, 1

.str.27:
	.asciz "lca->op == AST_OP(compound)"
	.type .str.27, @object
	.size .str.27, 28

.str.28:
	.asciz "Undefined label \'%s\'"
	.type .str.28, @object
	.size .str.28, 21

.str.29:
	.asciz "Duplicate label \'%s\'"
	.type .str.29, @object
	.size .str.29, 21

.str.30:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.30, @object
	.size .str.30, 30

.str.31:
	.asciz "(null)"
	.type .str.31, @object
	.size .str.31, 1

.str.32:
	.asciz "false"
	.type .str.32, @object
	.size .str.32, 6

