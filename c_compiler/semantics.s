	.file   "semantics.c"
	.text
	.option pic
.PCbegin:
	.global SemanticError
	.type SemanticError, @function

SemanticError:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global VSemanticError
	mv          t0, s0
	mv          a2, t0
	j           VSemanticError
.func_end_SemanticError:
	.size SemanticError, .func_end_SemanticError-SemanticError

	.global VSemanticError
	.type VSemanticError, @function

VSemanticError:

	// *** Basic block 0

	.global abort_on_error
	.global longjmp
	.global error_abort_state
	.global DecodeSourceLocation
	.global VReportError
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
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	la          t0, abort_on_error
	lb          t0, 0(t0)
	beqz        t0, .VSemanticError_label_28

	// *** Basic block 1

	la          a0, error_abort_state
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	call        longjmp

	// *** Basic block 2

.VSemanticError_label_28:
	ld          a0, 40(s1)
	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	call        DecodeSourceLocation

	// *** Basic block 3

	ld          a0, -48(s0)
	lw          a1, -40(s0)
	mv          a3, s3
	mv          a2, s2
	call        VReportError

	// *** Basic block 4

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_VSemanticError:
	.size VSemanticError, .func_end_VSemanticError-VSemanticError

	.global SemanticWarning
	.type SemanticWarning, @function

SemanticWarning:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global VSemanticWarning
	mv          t0, s0
	mv          a3, t0
	j           VSemanticWarning
.func_end_SemanticWarning:
	.size SemanticWarning, .func_end_SemanticWarning-SemanticWarning

	.global SemanticSymbolWarning
	.type SemanticSymbolWarning, @function

SemanticSymbolWarning:

	// *** Basic block 0

	.global DecodeSourceLocation
	.global VReportWarning
	addi sp, sp, -144
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// varargs function with 0 declared args
	sd a0, 0(s0)
	sd a1, 8(s0)
	sd a2, 16(s0)
	sd a3, 24(s0)
	sd a4, 32(s0)
	sd a5, 40(s0)
	sd a6, 48(s0)
	sd a7, 56(s0)
	// Local vars at offset -48(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a2
	mv          t0, a0
	mv          s2, a1
	mv          s3, s0
	ld          a0, 104(t0)
	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	call        DecodeSourceLocation

	// *** Basic block 1

	ld          a0, -48(s0)
	lw          a1, -40(s0)
	mv          a4, s3
	mv          a3, s1
	mv          a2, s2
	call        VReportWarning

	// *** Basic block 2

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 64
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SemanticSymbolWarning:
	.size SemanticSymbolWarning, .func_end_SemanticSymbolWarning-SemanticSymbolWarning

	.global VSemanticWarning
	.type VSemanticWarning, @function

VSemanticWarning:

	// *** Basic block 0

	.global DecodeSourceLocation
	.global VReportWarning
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
	// End of stack frame
	mv          t0, a0
	mv          s1, a1
	mv          s2, a2
	mv          s3, a3
	ld          a0, 40(t0)
	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	call        DecodeSourceLocation

	// *** Basic block 1

	ld          a0, -48(s0)
	lw          a1, -40(s0)
	mv          a4, s3
	mv          a3, s2
	mv          a2, s1
	call        VReportWarning

	// *** Basic block 2

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_VSemanticWarning:
	.size VSemanticWarning, .func_end_VSemanticWarning-VSemanticWarning

	.global SemanticCheckScalarType
	.type SemanticCheckScalarType, @function

SemanticCheckScalarType:

	// *** Basic block 0

	.global TypeIsVoid
	.global SemanticError
	.global TypeIsStructOrUnion
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
	bne         s1, x0, .SemanticCheckScalarType_label_19

	// *** Basic block 1

.SemanticCheckScalarType_label_16:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.SemanticCheckScalarType_label_19:
	ld          s2, 16(s1)
	mv          a0, s2
	call        TypeIsVoid

	// *** Basic block 3

	beqz        a0, .SemanticCheckScalarType_label_34

	// *** Basic block 4

	lla         a1, .str.1
	mv          a0, s1
	call        SemanticError

	// *** Basic block 5

	j           .SemanticCheckScalarType_label_16

	// *** Basic block 6

.SemanticCheckScalarType_label_34:
	mv          a0, s2
	call        TypeIsStructOrUnion

	// *** Basic block 7

	beqz        a0, .SemanticCheckScalarType_label_45

	// *** Basic block 8

	lla         a1, .str.2
	mv          a0, s1
	call        SemanticError

	// *** Basic block 9

.SemanticCheckScalarType_label_45:
	j           .SemanticCheckScalarType_label_16
.func_end_SemanticCheckScalarType:
	.size SemanticCheckScalarType, .func_end_SemanticCheckScalarType-SemanticCheckScalarType

	.local  CheckForUnusedLocalSymbols
	.type CheckForUnusedLocalSymbols, @function

CheckForUnusedLocalSymbols:

	// *** Basic block 0

	.global SemanticSymbolWarning
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
	mv          t0, a1
	mv          s1, x0
	addi        t1, a0, 80
	ld          s2, 8(t1)
	bge         x0, s2, .CheckForUnusedLocalSymbols_label_91

	// *** Basic block 1

	ld          t1, 80(a0)
	ld          t2, 16(t0)
	ld          t2, 32(t2)
	ld          s3, 16(t2)

	// *** Basic block 2

.CheckForUnusedLocalSymbols_label_38:
	slli        t0, s1, 3
	add         t0, t1, t0
	ld          s4, 0(t0)
	addi        s5, s4, 56
	lb          s6, 56(s4)
	slli        t1, s6, 56
	srai        t1, t1, 63
	not         t0, t1
	beqz        t0, .CheckForUnusedLocalSymbols_label_55

	// *** Basic block 3

	slli        t1, s6, 59
	srai        t1, t1, 63
	not         t0, t1

	// *** Basic block 4

.CheckForUnusedLocalSymbols_label_55:
	beqz        t0, .CheckForUnusedLocalSymbols_label_60

	// *** Basic block 5

	slli        t1, s6, 58
	srai        t1, t1, 63
	not         t0, t1

	// *** Basic block 6

.CheckForUnusedLocalSymbols_label_60:
	beqz        t0, .CheckForUnusedLocalSymbols_label_67

	// *** Basic block 7

	lb          t1, 1(s5)
	slli        t1, t1, 63
	srai        t1, t1, 63
	not         t0, t1

	// *** Basic block 8

.CheckForUnusedLocalSymbols_label_67:
	beqz        t0, .CheckForUnusedLocalSymbols_label_86

	// *** Basic block 9

	lla         a1, .str.3
	lla         a2, .str.4
	ld          a3, 16(s4)
	mv          a4, s3
	mv          a0, s4
	call        SemanticSymbolWarning

	// *** Basic block 10

.CheckForUnusedLocalSymbols_label_86:

	// *** Basic block 11

.CheckForUnusedLocalSymbols_label_87:
	addi        s1, s1, 1
	bge         s1, s2, .CheckForUnusedLocalSymbols_label_38

	// *** Basic block 12

.CheckForUnusedLocalSymbols_label_91:
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
.func_end_CheckForUnusedLocalSymbols:
	.size CheckForUnusedLocalSymbols, .func_end_CheckForUnusedLocalSymbols-CheckForUnusedLocalSymbols

	.local  CheckVLAArgs
	.type CheckVLAArgs, @function

CheckVLAArgs:

	// *** Basic block 0

	.global SemanticError
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
	ld          t0, 16(s1)
	addi        t0, t0, 32
	addi        t1, t0, 8
	mv          s2, x0
	ld          s3, 8(t1)
	bge         x0, s3, .CheckVLAArgs_label_89

	// *** Basic block 1

	ld          t0, 0(t1)

	// *** Basic block 2

.CheckVLAArgs_label_32:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	ld          s5, 40(s4)
	lw          s6, 16(s5)
	addi        t1, s6, -2
	seqz        t0, t1
	li          t1, 2		// 0x2 ASCII \x2
	beq         s6, t1, .CheckVLAArgs_label_53

	// *** Basic block 3

	addi        t1, s6, -1
	seqz        t0, t1

	// *** Basic block 4

.CheckVLAArgs_label_53:
	beqz        t0, .CheckVLAArgs_label_60

	// *** Basic block 5

	addi        t1, s5, 32
	lb          t1, 16(t1)
	slli        t1, t1, 61
	srai        t0, t1, 63

	// *** Basic block 6

.CheckVLAArgs_label_60:

	// *** Basic block 7

.CheckVLAArgs_label_62:
	beqz        t0, .CheckVLAArgs_label_84

	// *** Basic block 8

	j           .CheckVLAArgs_label_65

	// *** Basic block 9

.CheckVLAArgs_label_65:
	addi        t0, s5, 32
	lb          t0, 16(t0)
	slli        t0, t0, 60
	srai        t0, t0, 63
	beqz        t0, .CheckVLAArgs_label_83

	// *** Basic block 10

	lla         a1, .str.5
	ld          a2, 16(s4)
	mv          a0, s1
	call        SemanticError

	// *** Basic block 11

.CheckVLAArgs_label_83:

	// *** Basic block 12

.CheckVLAArgs_label_84:

	// *** Basic block 13

.CheckVLAArgs_label_85:
	addi        s2, s2, 1
	bge         s2, s3, .CheckVLAArgs_label_32

	// *** Basic block 14

.CheckVLAArgs_label_89:
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
.func_end_CheckVLAArgs:
	.size CheckVLAArgs, .func_end_CheckVLAArgs-CheckVLAArgs

	.global SemanticAnalyzeFunction
	.type SemanticAnalyzeFunction, @function

SemanticAnalyzeFunction:

	// *** Basic block 0

	.local CheckVLAArgs
	.global AnalyzeStatement
	.local CheckForUnusedLocalSymbols
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
	call        CheckVLAArgs

	// *** Basic block 1

	ld          t0, 16(s2)
	addi        t0, t0, 32
	ld          a0, 40(t0)
	call        AnalyzeStatement

	// *** Basic block 2

	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           CheckForUnusedLocalSymbols
.func_end_SemanticAnalyzeFunction:
	.size SemanticAnalyzeFunction, .func_end_SemanticAnalyzeFunction-SemanticAnalyzeFunction

	.global SemanticTypeConversionError
	.type SemanticTypeConversionError, @function

SemanticTypeConversionError:

	// *** Basic block 0

	.global TypeRecordToString
	.global SemanticError
	.global StringDestruct
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -96(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	sd          x0, -96(s0)
	sd          x0, -88(s0)
	sd          x0, -80(s0)
	sd          x0, -72(s0)
	sd          x0, -64(s0)
	sb          x0, -96(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -24(s0)
	sb          x0, -56(s0)
	ld          a0, 16(s1)
	addi        a1, s0, -96
	call        TypeRecordToString

	// *** Basic block 1

	addi        a1, s0, -56
	mv          a0, s2
	call        TypeRecordToString

	// *** Basic block 2

	addi        t0, s0, -96
	ld          a2, 16(t0)
	addi        t0, s0, -56
	ld          a3, 16(t0)
	mv          a1, s3
	mv          a0, s1
	call        SemanticError

	// *** Basic block 3

	addi        a0, s0, -96
	call        StringDestruct

	// *** Basic block 4

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 5

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SemanticTypeConversionError:
	.size SemanticTypeConversionError, .func_end_SemanticTypeConversionError-SemanticTypeConversionError

	.global SemanticTypeConversionWarning
	.type SemanticTypeConversionWarning, @function

SemanticTypeConversionWarning:

	// *** Basic block 0

	.global TypeRecordToString
	.global SemanticWarning
	.global StringDestruct
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -96(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	mv          s4, a3
	sd          x0, -96(s0)
	sd          x0, -88(s0)
	sd          x0, -80(s0)
	sd          x0, -72(s0)
	sd          x0, -64(s0)
	sb          x0, -96(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -24(s0)
	sb          x0, -56(s0)
	ld          a0, 16(s1)
	addi        a1, s0, -96
	call        TypeRecordToString

	// *** Basic block 1

	addi        a1, s0, -56
	mv          a0, s2
	call        TypeRecordToString

	// *** Basic block 2

	addi        t0, s0, -96
	ld          a3, 16(t0)
	addi        t0, s0, -56
	ld          a4, 16(t0)
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	call        SemanticWarning

	// *** Basic block 3

	addi        a0, s0, -96
	call        StringDestruct

	// *** Basic block 4

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 5

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SemanticTypeConversionWarning:
	.size SemanticTypeConversionWarning, .func_end_SemanticTypeConversionWarning-SemanticTypeConversionWarning

	.local  SignExtendIntConstant
	.type SignExtendIntConstant, @function

SignExtendIntConstant:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	li          t0, 64		// 0x40 ASCII '@'
	sub         t0, t0, a1
	ld          t1, 56(a0)
	sll         t1, t1, t0
	sd          t1, 56(a0)
	ld          t1, 56(a0)
	sra         t0, t1, t0
	sd          t0, 56(a0)

	// *** Basic block 1

.SignExtendIntConstant_label_24:
	ret         
.func_end_SignExtendIntConstant:
	.size SignExtendIntConstant, .func_end_SignExtendIntConstant-SignExtendIntConstant

	.local  ConvertIntToDouble
	.type ConvertIntToDouble, @function

ConvertIntToDouble:

	// *** Basic block 0

	.global TypeIsUnsigned
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
	ld          a0, 16(s1)
	call        TypeIsUnsigned

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .ConvertIntToDouble_label_32

	// *** Basic block 2

	li          t0, 1		// 0x1 ASCII \x1
	sll         t0, t0, s2
	addi        s3, t0, -1
	ld          t0, 56(s1)
	and         t0, t0, s3
	sd          t0, 56(s1)
	j           .ConvertIntToDouble_label_43

	// *** Basic block 3

.ConvertIntToDouble_label_32:
	li          t0, 64		// 0x40 ASCII '@'
	sub         t0, t0, s2
	ld          t1, 56(s1)
	sll         t1, t1, t0
	sd          t1, 56(s1)
	ld          t1, 56(s1)
	sra         t0, t1, t0
	sd          t0, 56(s1)

	// *** Basic block 4

.ConvertIntToDouble_label_43:
	ld          t0, 56(s1)
	fcvt.d.l    ft0, t0
	fsd         ft0, 56(s1)
	li          t0, 6		// 0x6 ASCII \x6
	sw          t0, 0(s1)
	mv          a0, s1

	// *** Basic block 5

.ConvertIntToDouble_label_53:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ConvertIntToDouble:
	.size ConvertIntToDouble, .func_end_ConvertIntToDouble-ConvertIntToDouble

	.local  ConvertDoubleToInt
	.type ConvertDoubleToInt, @function

ConvertDoubleToInt:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	fld         ft0, 56(a0)
	fcvt.l.d    t0, ft0
	sd          t0, 56(a0)
	li          t0, 64		// 0x40 ASCII '@'
	sub         t0, t0, a1
	ld          t1, 56(a0)
	sll         t1, t1, t0
	sd          t1, 56(a0)
	ld          t1, 56(a0)
	sra         t0, t1, t0
	sd          t0, 56(a0)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 0(a0)

	// *** Basic block 1

.ConvertDoubleToInt_label_32:
	ret         
.func_end_ConvertDoubleToInt:
	.size ConvertDoubleToInt, .func_end_ConvertDoubleToInt-ConvertDoubleToInt

	.local  ConvertPotentialConstant
	.type ConvertPotentialConstant, @function

ConvertPotentialConstant:

	// *** Basic block 0

	.local SignExtendIntConstant
	.local ConvertIntToDouble
	.local ConvertDoubleToInt
	addi sp, sp, -16
	// Saved return address (offset 8) and frame pointer (offset 0)
	sd ra, 8(sp)
	sd s0, 0(sp)
	addi s0, sp, 16
	// Local vars at offset -16(s0)
	// End of stack frame
	mv          t0, a0
	mv          t1, a2
	mv          t2, t0
	lw          t3, 0(t0)
	li          t4, 1		// 0x1 ASCII \x1
	blt         t3, t4, .ConvertPotentialConstant_label_490

	// *** Basic block 1

	li          t4, 6		// 0x6 ASCII \x6
	blt         t4, t3, .ConvertPotentialConstant_label_490

	// *** Basic block 2

	addi        t3, t3, -1
	slli        t3, t3, 2
	auipc       t4, 0
	add         t3, t4, t3
	jalr        x0, t3, 12

	// *** Basic block 3

	j           .ConvertPotentialConstant_label_50

	// *** Basic block 4

	j           .ConvertPotentialConstant_label_490

	// *** Basic block 5

	j           .ConvertPotentialConstant_label_490

	// *** Basic block 6

	j           .ConvertPotentialConstant_label_490

	// *** Basic block 7

	j           .ConvertPotentialConstant_label_490

	// *** Basic block 8

	j           .ConvertPotentialConstant_label_372

	// *** Basic block 9

.ConvertPotentialConstant_label_50:
	li          t3, 91		// 0x5b ASCII '['
	blt         t1, t3, .ConvertPotentialConstant_label_136

	// *** Basic block 10

	li          t3, 162		// 0xa2 ASCII \xa2
	blt         t3, t1, .ConvertPotentialConstant_label_136

	// *** Basic block 11

	addi        t3, t1, -91
	slli        t3, t3, 2
	auipc       t4, 0
	add         t3, t4, t3
	jalr        x0, t3, 12

	// *** Basic block 12

	j           .ConvertPotentialConstant_label_139

	// *** Basic block 13

	j           .ConvertPotentialConstant_label_153

	// *** Basic block 14

	j           .ConvertPotentialConstant_label_164

	// *** Basic block 15

	j           .ConvertPotentialConstant_label_165

	// *** Basic block 16

	j           .ConvertPotentialConstant_label_176

	// *** Basic block 17

	j           .ConvertPotentialConstant_label_177

	// *** Basic block 18

	j           .ConvertPotentialConstant_label_178

	// *** Basic block 19

	j           .ConvertPotentialConstant_label_189

	// *** Basic block 20

	j           .ConvertPotentialConstant_label_202

	// *** Basic block 21

	j           .ConvertPotentialConstant_label_213

	// *** Basic block 22

	j           .ConvertPotentialConstant_label_224

	// *** Basic block 23

	j           .ConvertPotentialConstant_label_225

	// *** Basic block 24

	j           .ConvertPotentialConstant_label_236

	// *** Basic block 25

	j           .ConvertPotentialConstant_label_237

	// *** Basic block 26

	j           .ConvertPotentialConstant_label_238

	// *** Basic block 27

	j           .ConvertPotentialConstant_label_190

	// *** Basic block 28

	j           .ConvertPotentialConstant_label_249

	// *** Basic block 29

	j           .ConvertPotentialConstant_label_260

	// *** Basic block 30

	j           .ConvertPotentialConstant_label_271

	// *** Basic block 31

	j           .ConvertPotentialConstant_label_272

	// *** Basic block 32

	j           .ConvertPotentialConstant_label_283

	// *** Basic block 33

	j           .ConvertPotentialConstant_label_284

	// *** Basic block 34

	j           .ConvertPotentialConstant_label_285

	// *** Basic block 35

	j           .ConvertPotentialConstant_label_191

	// *** Basic block 36

	j           .ConvertPotentialConstant_label_296

	// *** Basic block 37

	j           .ConvertPotentialConstant_label_308

	// *** Basic block 38

	j           .ConvertPotentialConstant_label_320

	// *** Basic block 39

	j           .ConvertPotentialConstant_label_333

	// *** Basic block 40

	j           .ConvertPotentialConstant_label_337

	// *** Basic block 41

	j           .ConvertPotentialConstant_label_340

	// *** Basic block 42

	j           .ConvertPotentialConstant_label_342

	// *** Basic block 43

	j           .ConvertPotentialConstant_label_193

	// *** Basic block 44

	j           .ConvertPotentialConstant_label_297

	// *** Basic block 45

	j           .ConvertPotentialConstant_label_309

	// *** Basic block 46

	j           .ConvertPotentialConstant_label_321

	// *** Basic block 47

	j           .ConvertPotentialConstant_label_332

	// *** Basic block 48

	j           .ConvertPotentialConstant_label_338

	// *** Basic block 49

	j           .ConvertPotentialConstant_label_339

	// *** Basic block 50

	j           .ConvertPotentialConstant_label_341

	// *** Basic block 51

	j           .ConvertPotentialConstant_label_192

	// *** Basic block 52

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 53

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 54

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 55

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 56

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 57

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 58

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 59

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 60

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 61

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 62

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 63

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 64

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 65

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 66

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 67

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 68

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 69

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 70

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 71

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 72

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 73

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 74

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 75

	j           .ConvertPotentialConstant_label_136

	// *** Basic block 76

	j           .ConvertPotentialConstant_label_353

	// *** Basic block 77

	j           .ConvertPotentialConstant_label_354

	// *** Basic block 78

	j           .ConvertPotentialConstant_label_355

	// *** Basic block 79

	j           .ConvertPotentialConstant_label_356

	// *** Basic block 80

	j           .ConvertPotentialConstant_label_360

	// *** Basic block 81

	j           .ConvertPotentialConstant_label_361

	// *** Basic block 82

	j           .ConvertPotentialConstant_label_362

	// *** Basic block 83

	j           .ConvertPotentialConstant_label_363

	// *** Basic block 84

.ConvertPotentialConstant_label_136:

	// *** Basic block 85

.ConvertPotentialConstant_label_137:
	j           .ConvertPotentialConstant_label_494

	// *** Basic block 86

.ConvertPotentialConstant_label_139:
	li          t3, 16		// 0x10 ASCII \x10
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SignExtendIntConstant

	// *** Basic block 88

.ConvertPotentialConstant_label_150:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 89

.ConvertPotentialConstant_label_153:
	li          t3, 8		// 0x8 ASCII \x8
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SignExtendIntConstant

	// *** Basic block 91

.ConvertPotentialConstant_label_164:

	// *** Basic block 92

.ConvertPotentialConstant_label_165:
	li          t3, 32		// 0x20 ASCII ' '
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SignExtendIntConstant

	// *** Basic block 94

.ConvertPotentialConstant_label_176:

	// *** Basic block 95

.ConvertPotentialConstant_label_177:

	// *** Basic block 96

.ConvertPotentialConstant_label_178:
	li          t3, 32		// 0x20 ASCII ' '
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ConvertIntToDouble

	// *** Basic block 98

.ConvertPotentialConstant_label_189:

	// *** Basic block 99

.ConvertPotentialConstant_label_190:

	// *** Basic block 100

.ConvertPotentialConstant_label_191:

	// *** Basic block 101

.ConvertPotentialConstant_label_192:

	// *** Basic block 102

.ConvertPotentialConstant_label_193:
	ld          t3, 56(t2)
	snez        t3, t3
	sd          t3, 56(t2)
	mv          a0, t0
	j           .ConvertPotentialConstant_label_150

	// *** Basic block 103

.ConvertPotentialConstant_label_202:
	li          t3, 8		// 0x8 ASCII \x8
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SignExtendIntConstant

	// *** Basic block 105

.ConvertPotentialConstant_label_213:
	li          t3, 8		// 0x8 ASCII \x8
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SignExtendIntConstant

	// *** Basic block 107

.ConvertPotentialConstant_label_224:

	// *** Basic block 108

.ConvertPotentialConstant_label_225:
	li          t3, 8		// 0x8 ASCII \x8
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SignExtendIntConstant

	// *** Basic block 110

.ConvertPotentialConstant_label_236:

	// *** Basic block 111

.ConvertPotentialConstant_label_237:

	// *** Basic block 112

.ConvertPotentialConstant_label_238:
	li          t3, 8		// 0x8 ASCII \x8
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ConvertIntToDouble

	// *** Basic block 114

.ConvertPotentialConstant_label_249:
	li          t3, 16		// 0x10 ASCII \x10
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SignExtendIntConstant

	// *** Basic block 116

.ConvertPotentialConstant_label_260:
	li          t3, 16		// 0x10 ASCII \x10
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SignExtendIntConstant

	// *** Basic block 118

.ConvertPotentialConstant_label_271:

	// *** Basic block 119

.ConvertPotentialConstant_label_272:
	li          t3, 16		// 0x10 ASCII \x10
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SignExtendIntConstant

	// *** Basic block 121

.ConvertPotentialConstant_label_283:

	// *** Basic block 122

.ConvertPotentialConstant_label_284:

	// *** Basic block 123

.ConvertPotentialConstant_label_285:
	li          t3, 16		// 0x10 ASCII \x10
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ConvertIntToDouble

	// *** Basic block 125

.ConvertPotentialConstant_label_296:

	// *** Basic block 126

.ConvertPotentialConstant_label_297:
	li          t3, 32		// 0x20 ASCII ' '
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SignExtendIntConstant

	// *** Basic block 128

.ConvertPotentialConstant_label_308:

	// *** Basic block 129

.ConvertPotentialConstant_label_309:
	li          t3, 8		// 0x8 ASCII \x8
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SignExtendIntConstant

	// *** Basic block 131

.ConvertPotentialConstant_label_320:

	// *** Basic block 132

.ConvertPotentialConstant_label_321:
	li          t3, 16		// 0x10 ASCII \x10
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SignExtendIntConstant

	// *** Basic block 134

.ConvertPotentialConstant_label_332:

	// *** Basic block 135

.ConvertPotentialConstant_label_333:
	mv          a0, t0
	j           .ConvertPotentialConstant_label_150

	// *** Basic block 136

.ConvertPotentialConstant_label_337:

	// *** Basic block 137

.ConvertPotentialConstant_label_338:

	// *** Basic block 138

.ConvertPotentialConstant_label_339:

	// *** Basic block 139

.ConvertPotentialConstant_label_340:

	// *** Basic block 140

.ConvertPotentialConstant_label_341:

	// *** Basic block 141

.ConvertPotentialConstant_label_342:
	li          t3, 64		// 0x40 ASCII '@'
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ConvertIntToDouble

	// *** Basic block 143

.ConvertPotentialConstant_label_353:

	// *** Basic block 144

.ConvertPotentialConstant_label_354:

	// *** Basic block 145

.ConvertPotentialConstant_label_355:

	// *** Basic block 146

.ConvertPotentialConstant_label_356:
	mv          a0, t0
	j           .ConvertPotentialConstant_label_150

	// *** Basic block 147

.ConvertPotentialConstant_label_360:

	// *** Basic block 148

.ConvertPotentialConstant_label_361:

	// *** Basic block 149

.ConvertPotentialConstant_label_362:

	// *** Basic block 150

.ConvertPotentialConstant_label_363:
	ld          t3, 56(t2)
	fcvt.d.l    ft0, t3
	fsd         ft0, 56(t2)
	mv          a0, t0
	j           .ConvertPotentialConstant_label_150

	// *** Basic block 151

.ConvertPotentialConstant_label_372:
	li          t3, 131		// 0x83 ASCII \x83
	blt         t1, t3, .ConvertPotentialConstant_label_410

	// *** Basic block 152

	li          t3, 154		// 0x9a ASCII \x9a
	blt         t3, t1, .ConvertPotentialConstant_label_410

	// *** Basic block 153

	addi        t3, t1, -131
	slli        t3, t3, 2
	auipc       t4, 0
	add         t3, t4, t3
	jalr        x0, t3, 12

	// *** Basic block 154

	j           .ConvertPotentialConstant_label_413

	// *** Basic block 155

	j           .ConvertPotentialConstant_label_426

	// *** Basic block 156

	j           .ConvertPotentialConstant_label_439

	// *** Basic block 157

	j           .ConvertPotentialConstant_label_452

	// *** Basic block 158

	j           .ConvertPotentialConstant_label_453

	// *** Basic block 159

	j           .ConvertPotentialConstant_label_468

	// *** Basic block 160

	j           .ConvertPotentialConstant_label_469

	// *** Basic block 161

	j           .ConvertPotentialConstant_label_477

	// *** Basic block 162

	j           .ConvertPotentialConstant_label_414

	// *** Basic block 163

	j           .ConvertPotentialConstant_label_427

	// *** Basic block 164

	j           .ConvertPotentialConstant_label_440

	// *** Basic block 165

	j           .ConvertPotentialConstant_label_454

	// *** Basic block 166

	j           .ConvertPotentialConstant_label_455

	// *** Basic block 167

	j           .ConvertPotentialConstant_label_470

	// *** Basic block 168

	j           .ConvertPotentialConstant_label_471

	// *** Basic block 169

	j           .ConvertPotentialConstant_label_478

	// *** Basic block 170

	j           .ConvertPotentialConstant_label_415

	// *** Basic block 171

	j           .ConvertPotentialConstant_label_428

	// *** Basic block 172

	j           .ConvertPotentialConstant_label_441

	// *** Basic block 173

	j           .ConvertPotentialConstant_label_456

	// *** Basic block 174

	j           .ConvertPotentialConstant_label_457

	// *** Basic block 175

	j           .ConvertPotentialConstant_label_472

	// *** Basic block 176

	j           .ConvertPotentialConstant_label_473

	// *** Basic block 177

	j           .ConvertPotentialConstant_label_479

	// *** Basic block 178

.ConvertPotentialConstant_label_410:

	// *** Basic block 179

.ConvertPotentialConstant_label_411:
	j           .ConvertPotentialConstant_label_494

	// *** Basic block 180

.ConvertPotentialConstant_label_413:

	// *** Basic block 181

.ConvertPotentialConstant_label_414:

	// *** Basic block 182

.ConvertPotentialConstant_label_415:
	li          t3, 32		// 0x20 ASCII ' '
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ConvertDoubleToInt

	// *** Basic block 184

.ConvertPotentialConstant_label_426:

	// *** Basic block 185

.ConvertPotentialConstant_label_427:

	// *** Basic block 186

.ConvertPotentialConstant_label_428:
	li          t3, 8		// 0x8 ASCII \x8
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ConvertDoubleToInt

	// *** Basic block 188

.ConvertPotentialConstant_label_439:

	// *** Basic block 189

.ConvertPotentialConstant_label_440:

	// *** Basic block 190

.ConvertPotentialConstant_label_441:
	li          t3, 16		// 0x10 ASCII \x10
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ConvertDoubleToInt

	// *** Basic block 192

.ConvertPotentialConstant_label_452:

	// *** Basic block 193

.ConvertPotentialConstant_label_453:

	// *** Basic block 194

.ConvertPotentialConstant_label_454:

	// *** Basic block 195

.ConvertPotentialConstant_label_455:

	// *** Basic block 196

.ConvertPotentialConstant_label_456:

	// *** Basic block 197

.ConvertPotentialConstant_label_457:
	li          t3, 64		// 0x40 ASCII '@'
	mv          a1, t3
	mv          a0, t2
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ConvertDoubleToInt

	// *** Basic block 199

.ConvertPotentialConstant_label_468:

	// *** Basic block 200

.ConvertPotentialConstant_label_469:

	// *** Basic block 201

.ConvertPotentialConstant_label_470:

	// *** Basic block 202

.ConvertPotentialConstant_label_471:

	// *** Basic block 203

.ConvertPotentialConstant_label_472:

	// *** Basic block 204

.ConvertPotentialConstant_label_473:
	mv          a0, t0
	ret         

	// *** Basic block 205

.ConvertPotentialConstant_label_477:

	// *** Basic block 206

.ConvertPotentialConstant_label_478:

	// *** Basic block 207

.ConvertPotentialConstant_label_479:
	fld         ft0, 56(t2)
	fmv.d.x     ft1, x0
	feq.d       t3, ft0, ft1
	not         t3, t3
	sd          t3, 56(t2)
	mv          a0, t0
	ret         

	// *** Basic block 208

.ConvertPotentialConstant_label_490:
	mv          a0, x0
	ret         

	// *** Basic block 209

.ConvertPotentialConstant_label_494:
	mv          a0, x0
	ret         
.func_end_ConvertPotentialConstant:
	.size ConvertPotentialConstant, .func_end_ConvertPotentialConstant-ConvertPotentialConstant

	.global NormalConversion
	.type NormalConversion, @function

NormalConversion:

	// *** Basic block 0

	.global SemanticConvertType
	// Leaf procedure, no stack frame generated
	mv          a2, x0
	j           SemanticConvertType
.func_end_NormalConversion:
	.size NormalConversion, .func_end_NormalConversion-NormalConversion

	.global SemanticConvertType
	.type SemanticConvertType, @function

SemanticConvertType:

	// *** Basic block 0

	.global TypeEqual
	.global TypeEqualIgnoringSign
	.global type_conversions
	.global SemanticTypeConversionWarning
	.local ConvertPotentialConstant
	.global ASTNodeSetType
	.global TypeRecordIncRef
	.global NewUnaryASTNode
	.global ASTNodeReplaceChild
	.global TypeIsVoid
	.global TypeIsStructOrUnion
	.global SemanticTypeConversionError
	.global TypeIsVoidPointer
	.global TypeAssignmentCompatible
	.global TypeIsEnum
	.global TypeIsInt
	.global TypeIsFunctionPointer
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
	mv          s1, a0
	sd          s1, -24(s0)	// Spilled @38
	mv          s2, a1
	sd          s2, -24(s0)	// Spilled @41
	mv          s3, a2
	ld          s4, 16(s1)
	sd          s4, -32(s0)	// Spilled @49
	mv          a0, s4
	call        TypeEqual

	// *** Basic block 1

	ld          s5, 24(s1)
	lw          s6, 32(s1)
	ld          s7, 40(s1)
	beqz        a0, .SemanticConvertType_label_65

	// *** Basic block 2

.SemanticConvertType_label_62:
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

	// *** Basic block 3

.SemanticConvertType_label_65:
	mv          a1, s2
	mv          a0, s4
	call        TypeEqualIgnoringSign

	// *** Basic block 4

	beqz        a0, .SemanticConvertType_label_73

	// *** Basic block 5

	j           .SemanticConvertType_label_62

	// *** Basic block 6

.SemanticConvertType_label_73:
	mv          s8, x0

	// *** Basic block 7

.SemanticConvertType_label_77:
	slli        t0, s8, 3
	slli        t1, s8, 4
	add         t0, t0, t1
	la          t1, type_conversions
	add         s10, t1, t0
	ld          t0, 0(s10)
	mv          a0, s4
	jalr         x1, t0, 0

	// *** Basic block 8

	mv          s9, a0
	beqz        a0, .SemanticConvertType_label_97

	// *** Basic block 9

	ld          t0, 8(s10)
	mv          a0, s2
	jalr         x1, t0, 0

	// *** Basic block 10

	mv          s9, a0

	// *** Basic block 11

.SemanticConvertType_label_97:
	beqz        s9, .SemanticConvertType_label_165

	// *** Basic block 12

	lw          s11, 16(s10)
	bnez        s11, .SemanticConvertType_label_104

	// *** Basic block 13

	j           .SemanticConvertType_label_62

	// *** Basic block 14

.SemanticConvertType_label_104:
	lb          t0, 20(s10)
	beqz        t0, .SemanticConvertType_label_120

	// *** Basic block 15

	lla         a2, .str.6
	lla         a3, .str.7
	mv          a1, s2
	mv          a0, s1
	call        SemanticTypeConversionWarning

	// *** Basic block 16

.SemanticConvertType_label_120:
	mv          a2, s11
	mv          a1, s2
	mv          a0, s1
	call        ConvertPotentialConstant

	// *** Basic block 17

	mv          s10, a0
	beq         s10, x0, .SemanticConvertType_label_139

	// *** Basic block 18

	mv          a1, s2
	mv          a0, s10
	call        ASTNodeSetType

	// *** Basic block 19

	j           .SemanticConvertType_label_62

	// *** Basic block 20

.SemanticConvertType_label_139:
	mv          a0, s2
	call        TypeRecordIncRef

	// *** Basic block 21

	mv          a3, s1
	mv          a2, s7
	mv          a1, s2
	mv          a0, s11
	call        NewUnaryASTNode

	// *** Basic block 22

	mv          s10, a0
	mv          a3, x0
	mv          a2, s10
	mv          a1, s6
	mv          a0, s5
	call        ASTNodeReplaceChild

	// *** Basic block 23

	j           .SemanticConvertType_label_62

	// *** Basic block 24

.SemanticConvertType_label_165:

	// *** Basic block 25

.SemanticConvertType_label_166:
	addi        s8, s8, 1
	li          t0, 74		// 0x4a ASCII 'J'
	bge         s8, t0, .SemanticConvertType_label_77

	// *** Basic block 26

.SemanticConvertType_label_171:
	slli        t0, s3, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 27

	j           .SemanticConvertType_label_222

	// *** Basic block 28

	j           .SemanticConvertType_label_179

	// *** Basic block 29

.SemanticConvertType_label_179:
	ld          a0, -24(s0)	// Spilled @41
	call        TypeIsVoid

	// *** Basic block 30

	beqz        a0, .SemanticConvertType_label_185

	// *** Basic block 31

	j           .SemanticConvertType_label_220

	// *** Basic block 32

.SemanticConvertType_label_185:
	mv          s6, x0
	ld          a0, -32(s0)	// Spilled @49
	call        TypeIsStructOrUnion

	// *** Basic block 33

	mv          s8, a0
	bnez        a0, .SemanticConvertType_label_198

	// *** Basic block 34

	ld          a0, -24(s0)	// Spilled @41
	call        TypeIsStructOrUnion

	// *** Basic block 35

	mv          s8, a0

	// *** Basic block 36

.SemanticConvertType_label_198:
	beqz        s8, .SemanticConvertType_label_202

	// *** Basic block 37

	li          s6, 1		// 0x1 ASCII \x1
	j           .SemanticConvertType_label_209

	// *** Basic block 38

.SemanticConvertType_label_202:
	ld          a0, -32(s0)	// Spilled @49
	call        TypeIsVoid

	// *** Basic block 39

	beqz        a0, .SemanticConvertType_label_208

	// *** Basic block 40

	li          s6, 1		// 0x1 ASCII \x1

	// *** Basic block 41

.SemanticConvertType_label_208:

	// *** Basic block 42

.SemanticConvertType_label_209:
	beqz        s6, .SemanticConvertType_label_219

	// *** Basic block 43

	lla         a2, .str.8
	ld          a1, -24(s0)	// Spilled @41
	ld          a0, -24(s0)	// Spilled @38
	call        SemanticTypeConversionError

	// *** Basic block 44

.SemanticConvertType_label_219:

	// *** Basic block 45

.SemanticConvertType_label_220:
	j           .SemanticConvertType_label_465

	// *** Basic block 46

.SemanticConvertType_label_222:
	mv          a0, s2
	call        TypeIsVoidPointer

	// *** Basic block 47

	beqz        a0, .SemanticConvertType_label_276

	// *** Basic block 48

	lw          t1, 16(s4)
	addi        t1, t1, -3
	seqz        s3, t1

	// *** Basic block 49

.SemanticConvertType_label_235:
	mv          t0, s3
	bnez        s3, .SemanticConvertType_label_258

	// *** Basic block 50

	j           .SemanticConvertType_label_239

	// *** Basic block 51

.SemanticConvertType_label_239:
	mv          s3, s4
	lw          t1, 16(s3)
	addi        t2, t1, -1
	seqz        s5, t2
	li          t2, 1		// 0x1 ASCII \x1
	beq         t1, t2, .SemanticConvertType_label_253

	// *** Basic block 52

	addi        t1, t1, -2
	seqz        s5, t1

	// *** Basic block 53

.SemanticConvertType_label_253:

	// *** Basic block 54

.SemanticConvertType_label_255:
	mv          t0, s5
	j           .SemanticConvertType_label_258

	// *** Basic block 55

.SemanticConvertType_label_258:
	beqz        t0, .SemanticConvertType_label_261

	// *** Basic block 56

	j           .SemanticConvertType_label_62

	// *** Basic block 57

.SemanticConvertType_label_261:
	lw          t1, 0(s1)
	li          t2, 1		// 0x1 ASCII \x1
	bne         t1, t2, .SemanticConvertType_label_275

	// *** Basic block 58

	mv          s5, s1
	ld          t1, 56(s5)
	bnez        t1, .SemanticConvertType_label_274

	// *** Basic block 59

	j           .SemanticConvertType_label_62

	// *** Basic block 60

.SemanticConvertType_label_274:

	// *** Basic block 61

.SemanticConvertType_label_275:

	// *** Basic block 62

.SemanticConvertType_label_276:
	mv          s7, s4
	lw          s11, 16(s7)
	addi        t0, s11, -1
	seqz        s8, t0
	li          s1, 1		// 0x1 ASCII \x1
	beq         s11, s1, .SemanticConvertType_label_289

	// *** Basic block 63

	addi        t0, s11, -2
	seqz        s8, t0

	// *** Basic block 64

.SemanticConvertType_label_289:

	// *** Basic block 65

.SemanticConvertType_label_291:
	mv          s6, s8
	beqz        s8, .SemanticConvertType_label_312

	// *** Basic block 66

	j           .SemanticConvertType_label_295

	// *** Basic block 67

.SemanticConvertType_label_295:
	mv          s7, s2
	lw          t0, 16(s7)
	addi        t1, t0, -1
	seqz        s8, t1
	beq         t0, s1, .SemanticConvertType_label_307

	// *** Basic block 68

	addi        t0, t0, -2
	seqz        s8, t0

	// *** Basic block 69

.SemanticConvertType_label_307:

	// *** Basic block 70

.SemanticConvertType_label_309:
	mv          s6, s8
	j           .SemanticConvertType_label_312

	// *** Basic block 71

.SemanticConvertType_label_312:
	beqz        s6, .SemanticConvertType_label_334

	// *** Basic block 72

	mv          a1, s2
	mv          a0, s4
	call        TypeAssignmentCompatible

	// *** Basic block 73

	not         t0, a0
	beqz        t0, .SemanticConvertType_label_332

	// *** Basic block 74

	lla         a2, .str.9
	lla         a3, .str.10
	mv          a1, s2
	ld          a0, -24(s0)	// Spilled @38
	call        SemanticTypeConversionWarning

	// *** Basic block 75

.SemanticConvertType_label_332:
	j           .SemanticConvertType_label_62

	// *** Basic block 76

.SemanticConvertType_label_334:
	mv          a0, s4
	call        TypeIsEnum

	// *** Basic block 77

	mv          s8, a0
	beqz        a0, .SemanticConvertType_label_346

	// *** Basic block 78

	mv          a0, s2
	call        TypeIsInt

	// *** Basic block 79

	mv          s8, a0

	// *** Basic block 80

.SemanticConvertType_label_346:
	bnez        s8, .SemanticConvertType_label_359

	// *** Basic block 81

	mv          a0, s4
	call        TypeIsInt

	// *** Basic block 82

	mv          s8, a0
	beqz        a0, .SemanticConvertType_label_358

	// *** Basic block 83

	mv          a0, s2
	call        TypeIsEnum

	// *** Basic block 85

.SemanticConvertType_label_358:

	// *** Basic block 86

.SemanticConvertType_label_359:
	beqz        s8, .SemanticConvertType_label_362

	// *** Basic block 87

	j           .SemanticConvertType_label_62

	// *** Basic block 88

.SemanticConvertType_label_362:
	ld          t0, -24(s0)	// Spilled @38
	lw          t1, 0(t0)
	addi        t2, t1, -1
	seqz        s11, t2
	bne         t1, s1, .SemanticConvertType_label_379

	// *** Basic block 89

	mv          s1, s2
	sd          s1, -40(s0)	// Spilled @369
	lw          t0, 16(s1)
	addi        t0, t0, -1
	seqz        s2, t0

	// *** Basic block 90

.SemanticConvertType_label_376:
	mv          s11, s2
	j           .SemanticConvertType_label_379

	// *** Basic block 91

.SemanticConvertType_label_379:
	beqz        s11, .SemanticConvertType_label_397

	// *** Basic block 92

	ld          s2, -24(s0)	// Spilled @38
	sd          s2, -24(s0)	// Spilled @381
	ld          t0, -24(s0)	// Spilled @381
	ld          a0, 16(t0)
	call        TypeIsInt

	// *** Basic block 93

	mv          s2, a0
	beqz        a0, .SemanticConvertType_label_393

	// *** Basic block 94

	ld          t0, -24(s0)	// Spilled @381
	ld          t1, 56(t0)
	seqz        s2, t1

	// *** Basic block 95

.SemanticConvertType_label_393:
	beqz        s2, .SemanticConvertType_label_396

	// *** Basic block 96

	j           .SemanticConvertType_label_62

	// *** Basic block 97

.SemanticConvertType_label_396:

	// *** Basic block 98

.SemanticConvertType_label_397:
	sd          s2, -24(s0)	// Spilled @398
	mv          s2, s4
	lw          t0, 16(s2)
	addi        t0, t0, -3
	seqz        s4, t0

	// *** Basic block 99

.SemanticConvertType_label_406:
	mv          s2, s4
	beqz        s4, .SemanticConvertType_label_415

	// *** Basic block 100

	j           .SemanticConvertType_label_410

	// *** Basic block 101

.SemanticConvertType_label_410:
	ld          a0, -24(s0)	// Spilled @41
	call        TypeIsFunctionPointer

	// *** Basic block 102

	mv          s2, a0

	// *** Basic block 103

.SemanticConvertType_label_415:
	ld          t0, -24(s0)	// Spilled @398
	beqz        t0, .SemanticConvertType_label_426

	// *** Basic block 104

	ld          t0, -24(s0)	// Spilled @41
	ld          a1, 24(t0)
	ld          a0, -32(s0)	// Spilled @49
	call        TypeEqual

	// *** Basic block 105

	beqz        a0, .SemanticConvertType_label_425

	// *** Basic block 106

	j           .SemanticConvertType_label_62

	// *** Basic block 107

.SemanticConvertType_label_425:

	// *** Basic block 108

.SemanticConvertType_label_426:
	sd          s2, -32(s0)	// Spilled @427
	ld          s4, -24(s0)	// Spilled @41
	lw          t0, 8(s4)
	li          s2, 32768		// 0x8000
	and         t0, t0, s2
	snez        s1, t0

	// *** Basic block 109

.SemanticConvertType_label_436:
	mv          s2, s1
	bnez        s1, .SemanticConvertType_label_452

	// *** Basic block 110

	j           .SemanticConvertType_label_440

	// *** Basic block 111

.SemanticConvertType_label_440:
	ld          s1, -32(s0)	// Spilled @49
	lw          t0, 8(s1)
	and         t0, t0, s2
	snez        s2, t0

	// *** Basic block 112

.SemanticConvertType_label_449:
	j           .SemanticConvertType_label_452

	// *** Basic block 113

.SemanticConvertType_label_452:
	ld          t0, -32(s0)	// Spilled @427
	beqz        t0, .SemanticConvertType_label_455

	// *** Basic block 114

	j           .SemanticConvertType_label_62

	// *** Basic block 115

.SemanticConvertType_label_455:
	lla         a2, .str.11
	ld          a1, -24(s0)	// Spilled @41
	ld          a0, -24(s0)	// Spilled @38
	call        SemanticTypeConversionError

	// *** Basic block 116

	j           .SemanticConvertType_label_465

	// *** Basic block 117

.SemanticConvertType_label_465:
	j           .SemanticConvertType_label_62
.func_end_SemanticConvertType:
	.size SemanticConvertType, .func_end_SemanticConvertType-SemanticConvertType

	.global SemanticAnalyzeVariableDefinition
	.type SemanticAnalyzeVariableDefinition, @function

SemanticAnalyzeVariableDefinition:

	// *** Basic block 0

	.global TypeIsVoid
	.global SemanticError
	.global AnalyzeExpression
	.global NormalConversion
	.global ASTNodeSetType
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
	ld          t0, 56(s1)
	ld          a0, 40(t0)
	call        TypeIsVoid

	// *** Basic block 1

	beqz        a0, .SemanticAnalyzeVariableDefinition_label_30

	// *** Basic block 2

	lla         a1, .str.12
	mv          a0, s1
	call        SemanticError

	// *** Basic block 3

.SemanticAnalyzeVariableDefinition_label_30:
	ld          s2, 64(s1)
	bne         s2, x0, .SemanticAnalyzeVariableDefinition_label_39

	// *** Basic block 4

.SemanticAnalyzeVariableDefinition_label_36:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.SemanticAnalyzeVariableDefinition_label_39:
	mv          a0, s2
	call        AnalyzeExpression

	// *** Basic block 6

	sd          a0, 64(s1)
	ld          a0, 64(s1)
	ld          t0, 56(s1)
	ld          s2, 40(t0)
	mv          a1, s2
	call        NormalConversion

	// *** Basic block 7

	mv          a1, s2
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 8

	j           .SemanticAnalyzeVariableDefinition_label_36
.func_end_SemanticAnalyzeVariableDefinition:
	.size SemanticAnalyzeVariableDefinition, .func_end_SemanticAnalyzeVariableDefinition-SemanticAnalyzeVariableDefinition

.PCend:
	.data
type_conversions:
	.type   type_conversions,@object
	.global type_conversions
	.size   type_conversions,1776
	.p2align  3
	.global TypeIsPointer
	.long    TypeIsPointer
	.global TypeIsVoidPointer
	.long    TypeIsVoidPointer
	.word   0
	.space  4
	.global TypeIsVoidPointer
	.long    TypeIsVoidPointer
	.global TypeIsPointer
	.long    TypeIsPointer
	.word   0
	.space  4
	.global TypeIsInt
	.long    TypeIsInt
	.global TypeIsShort
	.long    TypeIsShort
	.word   91
	.space  4
	.global TypeIsInt
	.long    TypeIsInt
	.global TypeIsLong
	.long    TypeIsLong
	.word   93
	.space  4
	.global TypeIsInt
	.long    TypeIsInt
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.word   94
	.space  4
	.global TypeIsInt
	.long    TypeIsInt
	.global TypeIsChar
	.long    TypeIsChar
	.word   92
	.space  4
	.global TypeIsInt
	.long    TypeIsInt
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   95
	.space  4
	.global TypeIsInt
	.long    TypeIsInt
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   96
	.space  4
	.global TypeIsInt
	.long    TypeIsInt
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.word   97
	.space  4
	.global TypeIsInt
	.long    TypeIsInt
	.global TypeIsBool
	.long    TypeIsBool
	.word   98
	.space  4
	.global TypeIsChar
	.long    TypeIsChar
	.global TypeIsShort
	.long    TypeIsShort
	.word   100
	.space  4
	.global TypeIsChar
	.long    TypeIsChar
	.global TypeIsLong
	.long    TypeIsLong
	.word   101
	.space  4
	.global TypeIsChar
	.long    TypeIsChar
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.word   102
	.space  4
	.global TypeIsChar
	.long    TypeIsChar
	.global TypeIsInt
	.long    TypeIsInt
	.word   99
	.space  4
	.global TypeIsChar
	.long    TypeIsChar
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   103
	.space  4
	.global TypeIsChar
	.long    TypeIsChar
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   104
	.space  4
	.global TypeIsChar
	.long    TypeIsChar
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.word   105
	.space  4
	.global TypeIsChar
	.long    TypeIsChar
	.global TypeIsBool
	.long    TypeIsBool
	.word   106
	.space  4
	.global TypeIsShort
	.long    TypeIsShort
	.global TypeIsChar
	.long    TypeIsChar
	.word   108
	.space  4
	.global TypeIsShort
	.long    TypeIsShort
	.global TypeIsLong
	.long    TypeIsLong
	.word   109
	.space  4
	.global TypeIsShort
	.long    TypeIsShort
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.word   110
	.space  4
	.global TypeIsShort
	.long    TypeIsShort
	.global TypeIsInt
	.long    TypeIsInt
	.word   107
	.space  4
	.global TypeIsShort
	.long    TypeIsShort
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   111
	.space  4
	.global TypeIsShort
	.long    TypeIsShort
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   112
	.space  4
	.global TypeIsShort
	.long    TypeIsShort
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.word   113
	.space  4
	.global TypeIsShort
	.long    TypeIsShort
	.global TypeIsBool
	.long    TypeIsBool
	.word   114
	.space  4
	.global TypeIsLong
	.long    TypeIsLong
	.global TypeIsChar
	.long    TypeIsChar
	.word   116
	.space  4
	.global TypeIsLong
	.long    TypeIsLong
	.global TypeIsShort
	.long    TypeIsShort
	.word   117
	.space  4
	.global TypeIsLong
	.long    TypeIsLong
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.word   118
	.space  4
	.global TypeIsLong
	.long    TypeIsLong
	.global TypeIsInt
	.long    TypeIsInt
	.word   115
	.space  4
	.global TypeIsLong
	.long    TypeIsLong
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   119
	.space  4
	.global TypeIsLong
	.long    TypeIsLong
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   120
	.space  4
	.global TypeIsLong
	.long    TypeIsLong
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.word   121
	.space  4
	.global TypeIsLong
	.long    TypeIsLong
	.global TypeIsBool
	.long    TypeIsBool
	.word   122
	.space  4
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.global TypeIsChar
	.long    TypeIsChar
	.word   124
	.space  4
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.global TypeIsShort
	.long    TypeIsShort
	.word   125
	.space  4
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.global TypeIsLong
	.long    TypeIsLong
	.word   126
	.space  4
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.global TypeIsInt
	.long    TypeIsInt
	.word   123
	.space  4
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   127
	.space  4
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   128
	.space  4
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.word   129
	.space  4
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.global TypeIsBool
	.long    TypeIsBool
	.word   130
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.global TypeIsChar
	.long    TypeIsChar
	.word   132
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.global TypeIsShort
	.long    TypeIsShort
	.word   133
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.word   135
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.global TypeIsInt
	.long    TypeIsInt
	.word   131
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.global TypeIsLong
	.long    TypeIsLong
	.word   134
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   136
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.word   137
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.global TypeIsBool
	.long    TypeIsBool
	.word   138
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.global TypeIsChar
	.long    TypeIsChar
	.word   140
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.global TypeIsLong
	.long    TypeIsLong
	.word   142
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.word   143
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.global TypeIsInt
	.long    TypeIsInt
	.word   139
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   144
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.global TypeIsShort
	.long    TypeIsShort
	.word   141
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.word   145
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.global TypeIsBool
	.long    TypeIsBool
	.word   146
	.space  4
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.global TypeIsChar
	.long    TypeIsChar
	.word   148
	.space  4
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.global TypeIsLong
	.long    TypeIsLong
	.word   150
	.space  4
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.word   151
	.space  4
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.global TypeIsInt
	.long    TypeIsInt
	.word   147
	.space  4
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   152
	.space  4
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.global TypeIsShort
	.long    TypeIsShort
	.word   149
	.space  4
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   153
	.space  4
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.global TypeIsBool
	.long    TypeIsBool
	.word   154
	.space  4
	.global TypeIsBool
	.long    TypeIsBool
	.global TypeIsChar
	.long    TypeIsChar
	.word   156
	.space  4
	.global TypeIsBool
	.long    TypeIsBool
	.global TypeIsShort
	.long    TypeIsShort
	.word   157
	.space  4
	.global TypeIsBool
	.long    TypeIsBool
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.word   159
	.space  4
	.global TypeIsBool
	.long    TypeIsBool
	.global TypeIsInt
	.long    TypeIsInt
	.word   155
	.space  4
	.global TypeIsBool
	.long    TypeIsBool
	.global TypeIsLong
	.long    TypeIsLong
	.word   158
	.space  4
	.global TypeIsBool
	.long    TypeIsBool
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   161
	.space  4
	.global TypeIsBool
	.long    TypeIsBool
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.word   162
	.space  4
	.global TypeIsBool
	.long    TypeIsBool
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   160
	.space  4

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "Illegal use of void type"
	.type .str.1, @object
	.size .str.1, 25

.str.2:
	.asciz "Illegal use of composite type (struct or union)"
	.type .str.2, @object
	.size .str.2, 48

.str.3:
	.asciz "unused-var"
	.type .str.3, @object
	.size .str.3, 11

.str.4:
	.asciz "Local variable \'%s\' is not used in function \'%s\'"
	.type .str.4, @object
	.size .str.4, 49

.str.5:
	.asciz "Variable length array \'%s\' dimensions must be bound to a variable in function definition"
	.type .str.5, @object
	.size .str.5, 89

.str.6:
	.asciz "type-conversion"
	.type .str.6, @object
	.size .str.6, 16

.str.7:
	.asciz "Dangerous type conversion from \'%s\' to \'%s\'"
	.type .str.7, @object
	.size .str.7, 44

.str.8:
	.asciz "Illegal cast"
	.type .str.8, @object
	.size .str.8, 13

.str.9:
	.asciz "ptr-conversion"
	.type .str.9, @object
	.size .str.9, 15

.str.10:
	.asciz "Illegal pointer conversion; from \'%s\' to \'%s\'"
	.type .str.10, @object
	.size .str.10, 46

.str.11:
	.asciz "Illegal conversion; cannot convert from \'%s\' to \'%s\'"
	.type .str.11, @object
	.size .str.11, 53

.str.12:
	.asciz "Cannot define a variable with void type"
	.type .str.12, @object
	.size .str.12, 40

