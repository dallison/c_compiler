	.file   "expr_semantics.c"
	.text
	.option pic
.PCbegin:
	.local  AnalyzeIdentifier
	.type AnalyzeIdentifier, @function

AnalyzeIdentifier:

	// *** Basic block 0

	.global TypeIsStructOrUnion
	.global TypeIsIntegral
	.global NewIntConstantASTNode
	.global ASTNodeReplaceChild
	.global TypeIsFloatingPoint
	.global NewRealConstantASTNode
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
	ld          s2, 24(s1)
	sub         t1, s2, x0
	seqz        t0, t1
	beq         s2, x0, .AnalyzeIdentifier_label_38

	// *** Basic block 1

	lw          t1, 0(s2)
	addi        t1, t1, -86
	snez        t0, t1

	// *** Basic block 2

.AnalyzeIdentifier_label_38:
	beqz        t0, .AnalyzeIdentifier_label_47

	// *** Basic block 3

	ld          t0, 56(s1)
	lb          t1, 56(t0)
	andi        t1, t1, -129
	ori         t1, t1, 128
	sb          t1, 56(t0)

	// *** Basic block 4

.AnalyzeIdentifier_label_47:
	ld          s3, 16(s1)
	mv          a0, s3
	call        TypeIsStructOrUnion

	// *** Basic block 5

	mv          s2, a0
	bnez        a0, .AnalyzeIdentifier_label_67

	// *** Basic block 6

	lw          t0, 16(s3)
	addi        t0, t0, -2
	seqz        s3, t0

	// *** Basic block 7

.AnalyzeIdentifier_label_64:
	mv          s2, s3
	j           .AnalyzeIdentifier_label_67

	// *** Basic block 8

.AnalyzeIdentifier_label_67:
	bnez        s2, .AnalyzeIdentifier_label_80

	// *** Basic block 9

	mv          s4, s3
	lw          t0, 16(s4)
	addi        t0, t0, -3
	seqz        s5, t0

	// *** Basic block 10

.AnalyzeIdentifier_label_77:
	mv          s2, s5
	j           .AnalyzeIdentifier_label_80

	// *** Basic block 11

.AnalyzeIdentifier_label_80:
	beqz        s2, .AnalyzeIdentifier_label_87

	// *** Basic block 12

	lw          t0, 8(s1)
	ori         t0, t0, 1
	sw          t0, 8(s1)
	j           .AnalyzeIdentifier_label_208

	// *** Basic block 13

.AnalyzeIdentifier_label_87:
	lw          t0, 8(s1)
	andi        t0, t0, 16
	beqz        t0, .AnalyzeIdentifier_label_98

	// *** Basic block 14

	mv          a0, s1

	// *** Basic block 15

.AnalyzeIdentifier_label_95:
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

	// *** Basic block 16

.AnalyzeIdentifier_label_98:
	mv          s5, s3
	mv          a0, s5
	call        TypeIsIntegral

	// *** Basic block 17

	beqz        a0, .AnalyzeIdentifier_label_111

	// *** Basic block 18

	lw          t0, 12(s5)
	andi        t0, t0, 4
	snez        s6, t0

	// *** Basic block 19

.AnalyzeIdentifier_label_111:

	// *** Basic block 20

.AnalyzeIdentifier_label_113:
	beqz        s6, .AnalyzeIdentifier_label_153

	// *** Basic block 21

	j           .AnalyzeIdentifier_label_116

	// *** Basic block 22

.AnalyzeIdentifier_label_116:
	ld          t0, 56(s1)
	ld          a0, 112(t0)
	ld          a1, 40(t0)
	ld          a2, 40(s1)
	call        NewIntConstantASTNode

	// *** Basic block 23

	mv          s6, a0
	ld          a0, 24(s1)
	lw          a1, 32(s1)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, s6
	call        ASTNodeReplaceChild

	// *** Basic block 24

	lw          t0, 8(s6)
	ori         t0, t0, 8
	sw          t0, 8(s6)
	mv          a0, s6
	j           .AnalyzeIdentifier_label_95

	// *** Basic block 25

.AnalyzeIdentifier_label_153:
	mv          s7, s3
	mv          a0, s7
	call        TypeIsFloatingPoint

	// *** Basic block 26

	beqz        a0, .AnalyzeIdentifier_label_166

	// *** Basic block 27

	lw          t0, 12(s7)
	andi        t0, t0, 4
	snez        s3, t0

	// *** Basic block 28

.AnalyzeIdentifier_label_166:

	// *** Basic block 29

.AnalyzeIdentifier_label_168:
	beqz        s3, .AnalyzeIdentifier_label_206

	// *** Basic block 30

	j           .AnalyzeIdentifier_label_171

	// *** Basic block 31

.AnalyzeIdentifier_label_171:
	ld          t0, 56(s1)
	fld         fa0, 112(t0)
	ld          a0, 40(t0)
	ld          a1, 40(s1)
	call        NewRealConstantASTNode

	// *** Basic block 32

	mv          s3, a0
	ld          a0, 24(s1)
	lw          a1, 32(s1)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, s3
	call        ASTNodeReplaceChild

	// *** Basic block 33

	lw          t0, 8(s3)
	ori         t0, t0, 8
	sw          t0, 8(s3)
	mv          a0, s3
	j           .AnalyzeIdentifier_label_95

	// *** Basic block 34

.AnalyzeIdentifier_label_206:

	// *** Basic block 35

.AnalyzeIdentifier_label_207:

	// *** Basic block 36

.AnalyzeIdentifier_label_208:
	mv          a0, s1
	j           .AnalyzeIdentifier_label_95
.func_end_AnalyzeIdentifier:
	.size AnalyzeIdentifier, .func_end_AnalyzeIdentifier-AnalyzeIdentifier

	.local  FoldConstantExpression
	.type FoldConstantExpression, @function

FoldConstantExpression:

	// *** Basic block 0

	.global TypeIsIntegral
	.global EvaluateIntegerExpression
	.global NewIntConstantASTNode
	.global ASTNodeReplaceChild
	.global TypeIsFloatingPoint
	.global EvaluateFloatingPointExpression
	.global NewRealConstantASTNode
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// End of stack frame
	mv          s1, a0
	lw          t0, 0(s1)
	li          s2, 1		// 0x1 ASCII \x1
	beq         t0, s2, .FoldConstantExpression_label_83

	// *** Basic block 1

	li          t1, 2		// 0x2 ASCII \x2
	beq         t0, t1, .FoldConstantExpression_label_90

	// *** Basic block 2

	li          t1, 3		// 0x3 ASCII \x3
	beq         t0, t1, .FoldConstantExpression_label_87

	// *** Basic block 3

	li          t1, 4		// 0x4 ASCII \x4
	beq         t0, t1, .FoldConstantExpression_label_88

	// *** Basic block 4

	li          t1, 5		// 0x5 ASCII \x5
	beq         t0, t1, .FoldConstantExpression_label_84

	// *** Basic block 5

	li          t1, 6		// 0x6 ASCII \x6
	beq         t0, t1, .FoldConstantExpression_label_86

	// *** Basic block 6

	li          t1, 27		// 0x1b ASCII \x1b
	beq         t0, t1, .FoldConstantExpression_label_85

	// *** Basic block 7

	li          t1, 84		// 0x54 ASCII 'T'
	beq         t0, t1, .FoldConstantExpression_label_89

	// *** Basic block 8

	li          t1, 87		// 0x57 ASCII 'W'
	beq         t0, t1, .FoldConstantExpression_label_96

	// *** Basic block 9

.FoldConstantExpression_label_74:

	// *** Basic block 10

.FoldConstantExpression_label_75:
	ld          s3, 16(s1)
	mv          a0, s3
	call        TypeIsIntegral

	// *** Basic block 11

	beqz        a0, .FoldConstantExpression_label_145

	// *** Basic block 12

	j           .FoldConstantExpression_label_100

	// *** Basic block 13

.FoldConstantExpression_label_83:

	// *** Basic block 14

.FoldConstantExpression_label_84:

	// *** Basic block 15

.FoldConstantExpression_label_85:

	// *** Basic block 16

.FoldConstantExpression_label_86:

	// *** Basic block 17

.FoldConstantExpression_label_87:

	// *** Basic block 18

.FoldConstantExpression_label_88:

	// *** Basic block 19

.FoldConstantExpression_label_89:

	// *** Basic block 20

.FoldConstantExpression_label_90:
	mv          a0, x0

	// *** Basic block 21

.FoldConstantExpression_label_93:
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

	// *** Basic block 22

.FoldConstantExpression_label_96:
	mv          a0, x0
	j           .FoldConstantExpression_label_93

	// *** Basic block 23

.FoldConstantExpression_label_100:
	addi        a1, s0, -32
	mv          a0, s1
	call        EvaluateIntegerExpression

	// *** Basic block 24

	mv          s4, a0
	beqz        s4, .FoldConstantExpression_label_143

	// *** Basic block 25

	ld          a0, -32(s0)
	ld          a2, 40(s1)
	mv          a1, s3
	call        NewIntConstantASTNode

	// *** Basic block 26

	mv          s4, a0
	ld          a0, 24(s1)
	lw          a1, 32(s1)
	mv          a3, s2
	mv          a2, s4
	call        ASTNodeReplaceChild

	// *** Basic block 27

	lw          t0, 8(s4)
	ori         t0, t0, 8
	sw          t0, 8(s4)
	mv          a0, s4
	j           .FoldConstantExpression_label_93

	// *** Basic block 28

.FoldConstantExpression_label_143:
	j           .FoldConstantExpression_label_191

	// *** Basic block 29

.FoldConstantExpression_label_145:
	mv          a0, s3
	call        TypeIsFloatingPoint

	// *** Basic block 30

	beqz        a0, .FoldConstantExpression_label_190

	// *** Basic block 31

	addi        a1, s0, -24
	mv          a0, s1
	call        EvaluateFloatingPointExpression

	// *** Basic block 32

	mv          s5, a0
	beqz        s5, .FoldConstantExpression_label_189

	// *** Basic block 33

	fld         fa0, -24(s0)
	ld          a1, 40(s1)
	mv          a0, s3
	call        NewRealConstantASTNode

	// *** Basic block 34

	mv          s3, a0
	ld          a0, 24(s1)
	lw          a1, 32(s1)
	mv          a3, s2
	mv          a2, s3
	call        ASTNodeReplaceChild

	// *** Basic block 35

	lw          t0, 8(s3)
	ori         t0, t0, 8
	sw          t0, 8(s3)
	mv          a0, s3
	j           .FoldConstantExpression_label_93

	// *** Basic block 36

.FoldConstantExpression_label_189:

	// *** Basic block 37

.FoldConstantExpression_label_190:

	// *** Basic block 38

.FoldConstantExpression_label_191:
	mv          a0, x0
	j           .FoldConstantExpression_label_93
.func_end_FoldConstantExpression:
	.size FoldConstantExpression, .func_end_FoldConstantExpression-FoldConstantExpression

	.local  AnalyzeBinaryExpression
	.type AnalyzeBinaryExpression, @function

AnalyzeBinaryExpression:

	// *** Basic block 0

	.global AnalyzeExpression
	.global ASTNodeSetType
	.global SemanticCheckScalarType
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
	bne         s1, x0, .AnalyzeBinaryExpression_label_19

	// *** Basic block 1

.AnalyzeBinaryExpression_label_16:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.AnalyzeBinaryExpression_label_19:
	ld          a0, 56(s1)
	call        AnalyzeExpression

	// *** Basic block 3

	sd          a0, 56(s1)
	ld          a0, 64(s1)
	call        AnalyzeExpression

	// *** Basic block 4

	sd          a0, 64(s1)
	ld          s2, 56(s1)
	ld          a1, 16(s2)
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 5

	mv          a0, s2
	call        SemanticCheckScalarType

	// *** Basic block 6

	ld          a0, 64(s1)
	call        SemanticCheckScalarType

	// *** Basic block 7

	j           .AnalyzeBinaryExpression_label_16
.func_end_AnalyzeBinaryExpression:
	.size AnalyzeBinaryExpression, .func_end_AnalyzeBinaryExpression-AnalyzeBinaryExpression

	.local  AnalyzeUnaryExpression
	.type AnalyzeUnaryExpression, @function

AnalyzeUnaryExpression:

	// *** Basic block 0

	.global AnalyzeExpression
	.global SemanticCheckScalarType
	.global NormalConversion
	.global NewTypeRecord
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
	mv          s1, a0
	bne         s1, x0, .AnalyzeUnaryExpression_label_22

	// *** Basic block 1

.AnalyzeUnaryExpression_label_19:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.AnalyzeUnaryExpression_label_22:
	ld          a0, 56(s1)
	call        AnalyzeExpression

	// *** Basic block 3

	sd          a0, 56(s1)
	ld          s2, 56(s1)
	mv          a0, s2
	call        SemanticCheckScalarType

	// *** Basic block 4

	lw          t0, 0(s1)
	li          t1, 21		// 0x15 ASCII \x15
	bne         t0, t1, .AnalyzeUnaryExpression_label_51

	// *** Basic block 5

	mv          a1, x0
	li          t0, 256		// 0x100
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 6

	mv          a1, a0
	mv          a0, s2
	call        NormalConversion

	// *** Basic block 7

.AnalyzeUnaryExpression_label_51:
	ld          a1, 16(s2)
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 8

	j           .AnalyzeUnaryExpression_label_19
.func_end_AnalyzeUnaryExpression:
	.size AnalyzeUnaryExpression, .func_end_AnalyzeUnaryExpression-AnalyzeUnaryExpression

	.local  GetRank
	.type GetRank, @function

GetRank:

	// *** Basic block 0

	.global type_ranks
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
	la          t0, type_ranks
	ld          t0, 0(t0)
	beq         t0, x0, .GetRank_label_44

	// *** Basic block 1

.GetRank_label_19:
	slli        t0, s2, 3
	la          t1, type_ranks
	add         t0, t1, t0
	ld          t0, 0(t0)
	mv          a0, s1
	jalr         x1, t0, 0

	// *** Basic block 2

	beqz        a0, .GetRank_label_34

	// *** Basic block 3

	mv          a0, s2

	// *** Basic block 4

.GetRank_label_31:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.GetRank_label_34:

	// *** Basic block 6

.GetRank_label_35:
	addi        s2, s2, 1
	slli        t0, s2, 3
	la          t1, type_ranks
	add         t0, t1, t0
	ld          t0, 0(t0)
	beq         t0, x0, .GetRank_label_19

	// *** Basic block 7

.GetRank_label_44:
	li          a0, -1		// 0xffffffffffffffff
	j           .GetRank_label_31
.func_end_GetRank:
	.size GetRank, .func_end_GetRank-GetRank

	.local  IsIntConstant
	.type IsIntConstant, @function

IsIntConstant:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 0(a0)
	addi        t1, t0, -1
	seqz        a0, t1
	li          t1, 1		// 0x1 ASCII \x1
	beq         t0, t1, .IsIntConstant_label_19

	// *** Basic block 1

	addi        t0, t0, -5
	seqz        a0, t0

	// *** Basic block 2

.IsIntConstant_label_19:

	// *** Basic block 3

.IsIntConstant_label_21:
	ret         
.func_end_IsIntConstant:
	.size IsIntConstant, .func_end_IsIntConstant-IsIntConstant

	.local  InsertNumericConversions
	.type InsertNumericConversions, @function

InsertNumericConversions:

	// *** Basic block 0

	.global TypeIsStructOrUnion
	.global SemanticTypeConversionError
	.local IsIntConstant
	.local GetRank
	.global printf
	.global abort
	.global NormalConversion
	.global ASTNodeSetType
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
	ld          s3, 56(s1)
	ld          s4, 16(s3)
	mv          a0, s4
	call        TypeIsStructOrUnion

	// *** Basic block 1

	mv          s2, a0
	bnez        a0, .InsertNumericConversions_label_41

	// *** Basic block 2

	ld          t0, 64(s1)
	ld          a0, 16(t0)
	call        TypeIsStructOrUnion

	// *** Basic block 3

	mv          s2, a0

	// *** Basic block 4

.InsertNumericConversions_label_41:
	beqz        s2, .InsertNumericConversions_label_57

	// *** Basic block 5

	ld          t0, 64(s1)
	ld          a1, 16(t0)
	lla         a2, .str.1
	mv          a0, s3
	call        SemanticTypeConversionError

	// *** Basic block 6

	j           .InsertNumericConversions_label_192

	// *** Basic block 7

.InsertNumericConversions_label_57:
	lw          s7, 16(s4)
	addi        t0, s7, -1
	seqz        s6, t0
	li          s8, 1		// 0x1 ASCII \x1
	beq         s7, s8, .InsertNumericConversions_label_70

	// *** Basic block 8

	addi        t0, s7, -2
	seqz        s6, t0

	// *** Basic block 9

.InsertNumericConversions_label_70:

	// *** Basic block 10

.InsertNumericConversions_label_72:
	mv          s5, s6
	bnez        s6, .InsertNumericConversions_label_96

	// *** Basic block 11

	j           .InsertNumericConversions_label_76

	// *** Basic block 12

.InsertNumericConversions_label_76:
	ld          t0, 64(s1)
	ld          s6, 16(t0)
	lw          t0, 16(s6)
	addi        t1, t0, -1
	seqz        s7, t1
	beq         t0, s8, .InsertNumericConversions_label_91

	// *** Basic block 13

	addi        t0, t0, -2
	seqz        s7, t0

	// *** Basic block 14

.InsertNumericConversions_label_91:

	// *** Basic block 15

.InsertNumericConversions_label_93:
	mv          s5, s7
	j           .InsertNumericConversions_label_96

	// *** Basic block 16

.InsertNumericConversions_label_96:
	beqz        s5, .InsertNumericConversions_label_99

	// *** Basic block 17

	j           .InsertNumericConversions_label_191

	// *** Basic block 18

.InsertNumericConversions_label_99:
	mv          a0, s3
	call        IsIntConstant

	// *** Basic block 19

	beqz        a0, .InsertNumericConversions_label_108

	// *** Basic block 20

	mv          s6, x0
	j           .InsertNumericConversions_label_113

	// *** Basic block 21

.InsertNumericConversions_label_108:
	mv          a0, s4
	call        GetRank

	// *** Basic block 22

	mv          s6, a0

	// *** Basic block 23

.InsertNumericConversions_label_113:
	ld          s8, 64(s1)
	mv          a0, s8
	call        IsIntConstant

	// *** Basic block 24

	beqz        a0, .InsertNumericConversions_label_125

	// *** Basic block 25

	mv          s7, x0
	j           .InsertNumericConversions_label_131

	// *** Basic block 26

.InsertNumericConversions_label_125:
	ld          a0, 16(s8)
	call        GetRank

	// *** Basic block 27

	mv          s7, a0

	// *** Basic block 28

.InsertNumericConversions_label_131:
	addi        t1, s6, 1
	snez        t0, t1
	li          t1, -1		// 0xffffffffffffffff
	beq         s6, t1, .InsertNumericConversions_label_140

	// *** Basic block 29

	addi        t1, s7, 1
	snez        t0, t1

	// *** Basic block 30

.InsertNumericConversions_label_140:
	beqz        t0, .InsertNumericConversions_label_144

	// *** Basic block 31

	j           .InsertNumericConversions_label_160

	// *** Basic block 32

.InsertNumericConversions_label_144:
	lla         a0, .str.2
	lla         a1, .str.3
	lla         a3, .str.4
	li          t0, 181		// 0xb5 ASCII \xb5
	mv          a2, t0
	call        printf

	// *** Basic block 33

	call        abort

	// *** Basic block 34

.InsertNumericConversions_label_160:
	bge         s7, s6, .InsertNumericConversions_label_174

	// *** Basic block 35

	mv          a1, s4
	mv          a0, s8
	call        NormalConversion

	// *** Basic block 36

	mv          a1, s4
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 37

	j           .InsertNumericConversions_label_190

	// *** Basic block 38

.InsertNumericConversions_label_174:
	bge         s6, s7, .InsertNumericConversions_label_189

	// *** Basic block 39

	ld          s4, 16(s8)
	mv          a1, s4
	mv          a0, s3
	call        NormalConversion

	// *** Basic block 40

	mv          a1, s4
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 41

.InsertNumericConversions_label_189:

	// *** Basic block 42

.InsertNumericConversions_label_190:

	// *** Basic block 43

.InsertNumericConversions_label_191:

	// *** Basic block 44

.InsertNumericConversions_label_192:
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
.func_end_InsertNumericConversions:
	.size InsertNumericConversions, .func_end_InsertNumericConversions-InsertNumericConversions

	.local  AnalyzePlusOperator
	.type AnalyzePlusOperator, @function

AnalyzePlusOperator:

	// *** Basic block 0

	.local AnalyzeBinaryExpression
	.global TypeIsIntegral
	.global ASTNodeIsIntConstant
	.global ASTNodeConstantValue
	.global NewIntConstantASTNode
	.global NewPtrScaleASTNode
	.global ASTNodeSetType
	.global SemanticError
	.local InsertNumericConversions
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
	bne         s1, x0, .AnalyzePlusOperator_label_33

	// *** Basic block 1

.AnalyzePlusOperator_label_30:
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

	// *** Basic block 2

.AnalyzePlusOperator_label_33:
	mv          a0, s1
	call        AnalyzeBinaryExpression

	// *** Basic block 3

	ld          s2, 56(s1)
	ld          s3, 16(s2)
	lw          s3, 16(s3)
	addi        t0, s3, -1
	seqz        s4, t0
	li          s5, 1		// 0x1 ASCII \x1
	beq         s3, s5, .AnalyzePlusOperator_label_53

	// *** Basic block 4

	addi        t0, s3, -2
	seqz        s4, t0

	// *** Basic block 5

.AnalyzePlusOperator_label_53:

	// *** Basic block 6

.AnalyzePlusOperator_label_55:
	beqz        s4, .AnalyzePlusOperator_label_135

	// *** Basic block 7

	j           .AnalyzePlusOperator_label_58

	// *** Basic block 8

.AnalyzePlusOperator_label_58:
	ld          s4, 64(s1)
	ld          s6, 16(s4)
	mv          a0, s6
	call        TypeIsIntegral

	// *** Basic block 9

	beqz        a0, .AnalyzePlusOperator_label_126

	// *** Basic block 10

	mv          a0, s4
	call        ASTNodeIsIntConstant

	// *** Basic block 11

	beqz        a0, .AnalyzePlusOperator_label_96

	// *** Basic block 12

	ld          t0, 24(s3)
	lw          s7, 20(t0)
	mv          a0, s4
	call        ASTNodeConstantValue

	// *** Basic block 13

	mv          s8, a0
	mul         a0, s8, s7
	ld          a2, 40(s1)
	mv          a1, s6
	call        NewIntConstantASTNode

	// *** Basic block 14

	mv          s6, a0
	sd          s6, 64(s1)
	j           .AnalyzePlusOperator_label_124

	// *** Basic block 15

.AnalyzePlusOperator_label_96:
	ld          a0, 24(s3)
	ld          a3, 40(s4)
	mv          a2, s4
	li          t0, 68		// 0x44 ASCII 'D'
	mv          a1, t0
	call        NewPtrScaleASTNode

	// *** Basic block 16

	mv          s4, a0
	sd          s4, 64(s1)
	sd          s1, 24(s4)
	ld          t0, 56(s1)
	ld          a1, 16(t0)
	mv          a0, s4
	call        ASTNodeSetType

	// *** Basic block 17

.AnalyzePlusOperator_label_124:
	j           .AnalyzePlusOperator_label_133

	// *** Basic block 18

.AnalyzePlusOperator_label_126:
	lla         a1, .str.5
	mv          a0, s1
	call        SemanticError

	// *** Basic block 19

.AnalyzePlusOperator_label_133:
	j           .AnalyzePlusOperator_label_230

	// *** Basic block 20

.AnalyzePlusOperator_label_135:
	ld          t0, 64(s1)
	ld          s7, 16(t0)
	lw          s7, 16(s7)
	addi        t0, s7, -1
	seqz        s9, t0
	beq         s7, s5, .AnalyzePlusOperator_label_150

	// *** Basic block 21

	addi        t0, s7, -2
	seqz        s9, t0

	// *** Basic block 22

.AnalyzePlusOperator_label_150:

	// *** Basic block 23

.AnalyzePlusOperator_label_152:
	beqz        s9, .AnalyzePlusOperator_label_225

	// *** Basic block 24

	j           .AnalyzePlusOperator_label_155

	// *** Basic block 25

.AnalyzePlusOperator_label_155:
	mv          a0, s3
	call        TypeIsIntegral

	// *** Basic block 26

	beqz        a0, .AnalyzePlusOperator_label_216

	// *** Basic block 27

	mv          a0, s2
	call        ASTNodeIsIntConstant

	// *** Basic block 28

	beqz        a0, .AnalyzePlusOperator_label_187

	// *** Basic block 29

	ld          t0, 24(s7)
	lw          s5, 20(t0)
	mv          a0, s2
	call        ASTNodeConstantValue

	// *** Basic block 30

	mv          s9, a0
	mul         a0, s9, s5
	ld          a2, 40(s1)
	mv          a1, s3
	call        NewIntConstantASTNode

	// *** Basic block 31

	mv          s3, a0
	sd          s3, 56(s1)
	j           .AnalyzePlusOperator_label_214

	// *** Basic block 32

.AnalyzePlusOperator_label_187:
	ld          a0, 24(s7)
	ld          a3, 40(s2)
	mv          a2, s2
	li          t0, 68		// 0x44 ASCII 'D'
	mv          a1, t0
	call        NewPtrScaleASTNode

	// *** Basic block 33

	mv          s2, a0
	sd          s2, 56(s1)
	sd          s1, 24(s2)
	ld          t0, 64(s1)
	ld          a1, 16(t0)
	mv          a0, s2
	call        ASTNodeSetType

	// *** Basic block 34

.AnalyzePlusOperator_label_214:
	j           .AnalyzePlusOperator_label_223

	// *** Basic block 35

.AnalyzePlusOperator_label_216:
	lla         a1, .str.6
	mv          a0, s1
	call        SemanticError

	// *** Basic block 36

.AnalyzePlusOperator_label_223:
	j           .AnalyzePlusOperator_label_229

	// *** Basic block 37

.AnalyzePlusOperator_label_225:
	mv          a0, s1
	call        InsertNumericConversions

	// *** Basic block 38

.AnalyzePlusOperator_label_229:

	// *** Basic block 39

.AnalyzePlusOperator_label_230:
	j           .AnalyzePlusOperator_label_30
.func_end_AnalyzePlusOperator:
	.size AnalyzePlusOperator, .func_end_AnalyzePlusOperator-AnalyzePlusOperator

	.local  AnalyzeMinusOperator
	.type AnalyzeMinusOperator, @function

AnalyzeMinusOperator:

	// *** Basic block 0

	.local AnalyzeBinaryExpression
	.global TypeIsIntegral
	.global ASTNodeIsIntConstant
	.global ASTNodeConstantValue
	.global NewIntConstantASTNode
	.global NewPtrScaleASTNode
	.global ASTNodeSetType
	.global TypeIsPointerToSameType
	.global SemanticTypeConversionError
	.global ASTNodeMove
	.global ASTNodeReplaceChild
	.global NewTypeRecord
	.global SemanticError
	.local InsertNumericConversions
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
	bne         s1, x0, .AnalyzeMinusOperator_label_43

	// *** Basic block 1

	mv          a0, s1

	// *** Basic block 2

.AnalyzeMinusOperator_label_40:
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

.AnalyzeMinusOperator_label_43:
	mv          a0, s1
	call        AnalyzeBinaryExpression

	// *** Basic block 4

	ld          s2, 56(s1)
	ld          s3, 16(s2)
	lw          s3, 16(s3)
	addi        t0, s3, -1
	seqz        s4, t0
	li          s5, 1		// 0x1 ASCII \x1
	beq         s3, s5, .AnalyzeMinusOperator_label_63

	// *** Basic block 5

	addi        t0, s3, -2
	seqz        s4, t0

	// *** Basic block 6

.AnalyzeMinusOperator_label_63:

	// *** Basic block 7

.AnalyzeMinusOperator_label_65:
	beqz        s4, .AnalyzeMinusOperator_label_228

	// *** Basic block 8

	j           .AnalyzeMinusOperator_label_68

	// *** Basic block 9

.AnalyzeMinusOperator_label_68:
	ld          s4, 64(s1)
	ld          s6, 16(s4)
	mv          a0, s6
	call        TypeIsIntegral

	// *** Basic block 10

	beqz        a0, .AnalyzeMinusOperator_label_136

	// *** Basic block 11

	mv          a0, s4
	call        ASTNodeIsIntConstant

	// *** Basic block 12

	beqz        a0, .AnalyzeMinusOperator_label_106

	// *** Basic block 13

	ld          t0, 24(s3)
	lw          s7, 20(t0)
	mv          a0, s4
	call        ASTNodeConstantValue

	// *** Basic block 14

	mv          s8, a0
	mul         a0, s8, s7
	ld          a2, 40(s1)
	mv          a1, s3
	call        NewIntConstantASTNode

	// *** Basic block 15

	mv          s7, a0
	sd          s7, 64(s1)
	j           .AnalyzeMinusOperator_label_134

	// *** Basic block 16

.AnalyzeMinusOperator_label_106:
	ld          a0, 24(s3)
	ld          a3, 40(s4)
	mv          a2, s4
	li          t0, 68		// 0x44 ASCII 'D'
	mv          a1, t0
	call        NewPtrScaleASTNode

	// *** Basic block 17

	mv          s4, a0
	sd          s4, 64(s1)
	sd          s1, 24(s4)
	ld          t0, 56(s1)
	ld          a1, 16(t0)
	mv          a0, s4
	call        ASTNodeSetType

	// *** Basic block 18

.AnalyzeMinusOperator_label_134:
	j           .AnalyzeMinusOperator_label_226

	// *** Basic block 19

.AnalyzeMinusOperator_label_136:
	lw          s6, 16(s6)
	addi        t0, s6, -1
	seqz        s9, t0
	beq         s6, s5, .AnalyzeMinusOperator_label_147

	// *** Basic block 20

	addi        t0, s6, -2
	seqz        s9, t0

	// *** Basic block 21

.AnalyzeMinusOperator_label_147:

	// *** Basic block 22

.AnalyzeMinusOperator_label_149:
	beqz        s9, .AnalyzeMinusOperator_label_218

	// *** Basic block 23

	j           .AnalyzeMinusOperator_label_152

	// *** Basic block 24

.AnalyzeMinusOperator_label_152:
	mv          a1, s6
	mv          a0, s3
	call        TypeIsPointerToSameType

	// *** Basic block 25

	not         t0, a0
	beqz        t0, .AnalyzeMinusOperator_label_170

	// *** Basic block 26

	lla         a2, .str.7
	mv          a1, s6
	mv          a0, s2
	call        SemanticTypeConversionError

	// *** Basic block 27

.AnalyzeMinusOperator_label_168:
	j           .AnalyzeMinusOperator_label_225

	// *** Basic block 28

.AnalyzeMinusOperator_label_170:
	ld          s2, 24(s1)
	ld          s3, 24(s6)
	mv          a0, s1
	call        ASTNodeMove

	// *** Basic block 29

	ld          t0, 64(s1)
	ld          a3, 40(t0)
	mv          a2, a0
	li          t0, 66		// 0x42 ASCII 'B'
	mv          a1, t0
	mv          a0, s3
	call        NewPtrScaleASTNode

	// *** Basic block 30

	mv          s3, a0
	lw          a1, 32(s1)
	mv          a3, x0
	mv          a2, s3
	mv          a0, s2
	call        ASTNodeReplaceChild

	// *** Basic block 31

	mv          a1, x0
	li          t0, 16392		// 0x4008
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 32

	mv          a1, a0
	mv          a0, s3
	call        ASTNodeSetType

	// *** Basic block 33

	mv          a0, s3
	j           .AnalyzeMinusOperator_label_40

	// *** Basic block 34

.AnalyzeMinusOperator_label_218:
	lla         a1, .str.8
	mv          a0, s1
	call        SemanticError

	// *** Basic block 35

.AnalyzeMinusOperator_label_225:

	// *** Basic block 36

.AnalyzeMinusOperator_label_226:
	j           .AnalyzeMinusOperator_label_232

	// *** Basic block 37

.AnalyzeMinusOperator_label_228:
	mv          a0, s1
	call        InsertNumericConversions

	// *** Basic block 38

.AnalyzeMinusOperator_label_232:
	mv          a0, s1
	j           .AnalyzeMinusOperator_label_40
.func_end_AnalyzeMinusOperator:
	.size AnalyzeMinusOperator, .func_end_AnalyzeMinusOperator-AnalyzeMinusOperator

	.local  AnalyzeShift
	.type AnalyzeShift, @function

AnalyzeShift:

	// *** Basic block 0

	.local AnalyzeBinaryExpression
	.global TypeIsIntegral
	.global SemanticError
	.global TypeIsUnsigned
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
	call        AnalyzeBinaryExpression

	// *** Basic block 1

	ld          t0, 56(s1)
	ld          a0, 16(t0)
	call        TypeIsIntegral

	// *** Basic block 2

	not         s2, a0
	bnez        s2, .AnalyzeShift_label_37

	// *** Basic block 3

	ld          t0, 64(s1)
	ld          a0, 16(t0)
	call        TypeIsIntegral

	// *** Basic block 4

	not         s2, a0

	// *** Basic block 5

.AnalyzeShift_label_37:
	beqz        s2, .AnalyzeShift_label_46

	// *** Basic block 6

	lla         a1, .str.9
	mv          a0, s1
	call        SemanticError

	// *** Basic block 7

.AnalyzeShift_label_46:
	lw          t0, 0(s1)
	li          t1, 63		// 0x3f ASCII '?'
	bne         t0, t1, .AnalyzeShift_label_65

	// *** Basic block 8

	ld          a0, 16(s1)
	call        TypeIsUnsigned

	// *** Basic block 9

	beqz        a0, .AnalyzeShift_label_61

	// *** Basic block 10

	li          t0, 13		// 0xd ASCII \xd
	sw          t0, 0(s1)
	j           .AnalyzeShift_label_64

	// *** Basic block 11

.AnalyzeShift_label_61:
	li          t0, 14		// 0xe ASCII \xe
	sw          t0, 0(s1)

	// *** Basic block 12

.AnalyzeShift_label_64:

	// *** Basic block 13

.AnalyzeShift_label_65:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AnalyzeShift:
	.size AnalyzeShift, .func_end_AnalyzeShift-AnalyzeShift

	.local  AnalyzeBitwiseOperator
	.type AnalyzeBitwiseOperator, @function

AnalyzeBitwiseOperator:

	// *** Basic block 0

	.local AnalyzeBinaryExpression
	.global TypeIsIntegral
	.global SemanticError
	.local InsertNumericConversions
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
	call        AnalyzeBinaryExpression

	// *** Basic block 1

	ld          t0, 56(s1)
	ld          a0, 16(t0)
	call        TypeIsIntegral

	// *** Basic block 2

	not         s2, a0
	bnez        s2, .AnalyzeBitwiseOperator_label_33

	// *** Basic block 3

	ld          t0, 64(s1)
	ld          a0, 16(t0)
	call        TypeIsIntegral

	// *** Basic block 4

	not         s2, a0

	// *** Basic block 5

.AnalyzeBitwiseOperator_label_33:
	beqz        s2, .AnalyzeBitwiseOperator_label_43

	// *** Basic block 6

	lla         a1, .str.10
	mv          a0, s1
	call        SemanticError

	// *** Basic block 7

	j           .AnalyzeBitwiseOperator_label_48

	// *** Basic block 8

.AnalyzeBitwiseOperator_label_43:
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           InsertNumericConversions

	// *** Basic block 9

.AnalyzeBitwiseOperator_label_48:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AnalyzeBitwiseOperator:
	.size AnalyzeBitwiseOperator, .func_end_AnalyzeBitwiseOperator-AnalyzeBitwiseOperator

	.local  AnalyzeComparisonOperator
	.type AnalyzeComparisonOperator, @function

AnalyzeComparisonOperator:

	// *** Basic block 0

	.local AnalyzeBinaryExpression
	.local InsertNumericConversions
	.global ASTNodeSetType
	.global NewTypeRecord
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
	call        AnalyzeBinaryExpression

	// *** Basic block 1

	mv          a0, s1
	call        InsertNumericConversions

	// *** Basic block 2

	mv          a1, x0
	li          t0, 256		// 0x100
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 3

	mv          a1, a0
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeSetType
.func_end_AnalyzeComparisonOperator:
	.size AnalyzeComparisonOperator, .func_end_AnalyzeComparisonOperator-AnalyzeComparisonOperator

	.local  AnalyzeConditionalExpression
	.type AnalyzeConditionalExpression, @function

AnalyzeConditionalExpression:

	// *** Basic block 0

	.global AnalyzeExpression
	.global TypeIsStructOrUnion
	.global SemanticError
	.global ASTNodeSetType
	.local InsertNumericConversions
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
	ld          a0, 56(s1)
	call        AnalyzeExpression

	// *** Basic block 1

	sd          a0, 56(s1)
	ld          t0, 56(s1)
	ld          s2, 16(t0)
	mv          a0, s2
	call        TypeIsStructOrUnion

	// *** Basic block 2

	not         s3, a0

	// *** Basic block 3

.AnalyzeConditionalExpression_label_31:
	not         t0, s3
	beqz        t0, .AnalyzeConditionalExpression_label_52

	// *** Basic block 4

	j           .AnalyzeConditionalExpression_label_35

	// *** Basic block 5

.AnalyzeConditionalExpression_label_35:
	lla         a1, .str.11
	mv          a0, s1
	call        SemanticError

	// *** Basic block 6

	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeSetType

	// *** Basic block 7

.AnalyzeConditionalExpression_label_49:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 8

.AnalyzeConditionalExpression_label_52:
	ld          s2, 64(s1)
	ld          a0, 56(s2)
	call        AnalyzeExpression

	// *** Basic block 9

	sd          a0, 56(s2)
	ld          a0, 64(s2)
	call        AnalyzeExpression

	// *** Basic block 10

	sd          a0, 64(s2)
	mv          a0, s2
	call        InsertNumericConversions

	// *** Basic block 11

	ld          t0, 56(s2)
	ld          a1, 16(t0)
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 12

	ld          a1, 16(s2)
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 13

	j           .AnalyzeConditionalExpression_label_49
.func_end_AnalyzeConditionalExpression:
	.size AnalyzeConditionalExpression, .func_end_AnalyzeConditionalExpression-AnalyzeConditionalExpression

	.local  HasAddress
	.type HasAddress, @function

HasAddress:

	// *** Basic block 0

	.global IsBitfieldReference
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
	lw          s2, 0(s1)
	li          t0, 2		// 0x2 ASCII \x2
	beq         s2, t0, .HasAddress_label_50

	// *** Basic block 1

	li          t0, 11		// 0xb ASCII \xb
	beq         s2, t0, .HasAddress_label_60

	// *** Basic block 2

	li          t0, 19		// 0x13 ASCII \x13
	beq         s2, t0, .HasAddress_label_65

	// *** Basic block 3

	li          t0, 33		// 0x21 ASCII '!'
	beq         s2, t0, .HasAddress_label_64

	// *** Basic block 4

	li          t0, 50		// 0x32 ASCII '2'
	beq         s2, t0, .HasAddress_label_56

	// *** Basic block 5

	li          t0, 80		// 0x50 ASCII 'P'
	beq         s2, t0, .HasAddress_label_72

	// *** Basic block 6

.HasAddress_label_46:
	mv          a0, x0
	j           .HasAddress_label_53

	// *** Basic block 7

.HasAddress_label_50:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 8

.HasAddress_label_53:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 9

.HasAddress_label_56:
	li          a0, 1		// 0x1 ASCII \x1
	j           .HasAddress_label_53

	// *** Basic block 10

.HasAddress_label_60:
	li          a0, 1		// 0x1 ASCII \x1
	j           .HasAddress_label_53

	// *** Basic block 11

.HasAddress_label_64:

	// *** Basic block 12

.HasAddress_label_65:
	mv          a0, s1
	call        IsBitfieldReference

	// *** Basic block 13

	not         a0, a0
	j           .HasAddress_label_53

	// *** Basic block 14

.HasAddress_label_72:
	li          a0, 1		// 0x1 ASCII \x1
	j           .HasAddress_label_53
.func_end_HasAddress:
	.size HasAddress, .func_end_HasAddress-HasAddress

	.local  IsAssignable
	.type IsAssignable, @function

IsAssignable:

	// *** Basic block 0

	.global IsBitfieldReference
	.local HasAddress
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
	beqz        a1, .IsAssignable_label_36

	// *** Basic block 1

	ld          s2, 16(s1)
	lw          t0, 16(s2)
	addi        t0, t0, -3
	seqz        s2, t0
	j           .IsAssignable_label_30

	// *** Basic block 2

.IsAssignable_label_30:
	not         a0, s2

	// *** Basic block 3

.IsAssignable_label_33:
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

.IsAssignable_label_36:
	ld          t0, 16(s1)
	lw          t0, 12(t0)
	andi        t0, t0, 4
	snez        t1, t0

	// *** Basic block 5

.IsAssignable_label_45:
	beqz        t1, .IsAssignable_label_52

	// *** Basic block 6

	j           .IsAssignable_label_48

	// *** Basic block 7

.IsAssignable_label_48:
	mv          a0, x0
	j           .IsAssignable_label_33

	// *** Basic block 8

.IsAssignable_label_52:
	mv          t1, t0
	lw          t2, 16(t1)
	addi        t2, t2, -3
	seqz        t3, t2

	// *** Basic block 9

.IsAssignable_label_61:
	mv          s2, t3
	bnez        t3, .IsAssignable_label_77

	// *** Basic block 10

	j           .IsAssignable_label_65

	// *** Basic block 11

.IsAssignable_label_65:
	mv          s3, t0
	lw          t0, 16(s3)
	addi        t0, t0, -2
	seqz        s4, t0

	// *** Basic block 12

.IsAssignable_label_74:
	mv          s2, s4
	j           .IsAssignable_label_77

	// *** Basic block 13

.IsAssignable_label_77:
	beqz        s2, .IsAssignable_label_82

	// *** Basic block 14

	mv          a0, x0
	j           .IsAssignable_label_33

	// *** Basic block 15

.IsAssignable_label_82:
	lw          t0, 0(s1)
	li          t1, 20		// 0x14 ASCII \x14
	bne         t0, t1, .IsAssignable_label_92

	// *** Basic block 16

	li          a0, 1		// 0x1 ASCII \x1
	j           .IsAssignable_label_33

	// *** Basic block 17

.IsAssignable_label_92:
	mv          a0, s1
	call        IsBitfieldReference

	// *** Basic block 18

	bnez        a0, .IsAssignable_label_103

	// *** Basic block 19

	mv          a0, s1
	call        HasAddress

	// *** Basic block 21

.IsAssignable_label_103:
	j           .IsAssignable_label_33
.func_end_IsAssignable:
	.size IsAssignable, .func_end_IsAssignable-IsAssignable

	.local  AnalyzeInitialization
	.type AnalyzeInitialization, @function

AnalyzeInitialization:

	// *** Basic block 0

	.global AnalyzeExpression
	.global ASTNodeSetType
	.local IsAssignable
	.global SemanticError
	.global StorageIs
	.global TypeIsIntegral
	.global EvaluateIntegerExpression
	.global TypeIsFloatingPoint
	.global EvaluateFloatingPointExpression
	.global AnalyzeInitializer
	.global ASTNodeReplaceChild
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -32(s0)
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
	mv          s2, a2
	mv          s3, a0
	mv          a0, s1
	call        AnalyzeExpression

	// *** Basic block 1

	mv          a0, s2
	call        AnalyzeExpression

	// *** Basic block 2

	mv          s2, a0
	ld          s4, 16(s1)
	mv          a1, s4
	mv          a0, s3
	call        ASTNodeSetType

	// *** Basic block 3

	mv          a1, s4
	mv          a0, s2
	call        ASTNodeSetType

	// *** Basic block 4

	li          s4, 1		// 0x1 ASCII \x1
	mv          a1, s4
	mv          a0, s1
	call        IsAssignable

	// *** Basic block 5

	not         t0, a0
	beqz        t0, .AnalyzeInitialization_label_72

	// *** Basic block 6

	lla         a1, .str.12
	mv          a0, s1
	call        SemanticError

	// *** Basic block 7

	mv          a0, s2

	// *** Basic block 8

.AnalyzeInitialization_label_69:
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

	// *** Basic block 9

.AnalyzeInitialization_label_72:
	ld          s6, 56(s1)
	lw          s7, 48(s6)
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s7
	call        StorageIs

	// *** Basic block 10

	mv          s5, a0
	bnez        a0, .AnalyzeInitialization_label_93

	// *** Basic block 11

	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	mv          a0, s7
	call        StorageIs

	// *** Basic block 13

.AnalyzeInitialization_label_93:
	ld          s7, 40(s6)
	lw          t0, 12(s7)
	andi        t0, t0, 4
	snez        s6, t0

	// *** Basic block 14

.AnalyzeInitialization_label_103:
	beqz        s6, .AnalyzeInitialization_label_152

	// *** Basic block 15

	j           .AnalyzeInitialization_label_106

	// *** Basic block 16

.AnalyzeInitialization_label_106:
	ld          s6, 16(s2)
	mv          a0, s6
	call        TypeIsIntegral

	// *** Basic block 17

	beqz        a0, .AnalyzeInitialization_label_130

	// *** Basic block 18

	addi        a1, s0, -32
	mv          a0, s2
	call        EvaluateIntegerExpression

	// *** Basic block 19

	mv          s7, a0
	beqz        s7, .AnalyzeInitialization_label_128

	// *** Basic block 20

	ld          t0, 56(s1)
	ld          t1, -32(s0)
	sd          t1, 112(t0)

	// *** Basic block 21

.AnalyzeInitialization_label_128:
	j           .AnalyzeInitialization_label_151

	// *** Basic block 22

.AnalyzeInitialization_label_130:
	mv          a0, s6
	call        TypeIsFloatingPoint

	// *** Basic block 23

	beqz        a0, .AnalyzeInitialization_label_150

	// *** Basic block 24

	addi        a1, s0, -24
	mv          a0, s2
	call        EvaluateFloatingPointExpression

	// *** Basic block 25

	mv          s6, a0
	beqz        s6, .AnalyzeInitialization_label_149

	// *** Basic block 26

	ld          t0, 56(s1)
	fld         ft0, -24(s0)
	fsd         ft0, 112(t0)

	// *** Basic block 27

.AnalyzeInitialization_label_149:

	// *** Basic block 28

.AnalyzeInitialization_label_150:

	// *** Basic block 29

.AnalyzeInitialization_label_151:

	// *** Basic block 30

.AnalyzeInitialization_label_152:
	ld          a0, 16(s3)
	mv          a2, s5
	mv          a1, s2
	call        AnalyzeInitializer

	// *** Basic block 31

	mv          s8, a0
	mv          a3, s4
	mv          a2, s8
	mv          a1, s4
	mv          a0, s3
	call        ASTNodeReplaceChild

	// *** Basic block 32

	beqz        s5, .AnalyzeInitialization_label_180

	// *** Basic block 33

	lw          t0, 8(s3)
	ori         t0, t0, 2
	sw          t0, 8(s3)

	// *** Basic block 34

.AnalyzeInitialization_label_180:
	mv          a0, s8
	j           .AnalyzeInitialization_label_69
.func_end_AnalyzeInitialization:
	.size AnalyzeInitialization, .func_end_AnalyzeInitialization-AnalyzeInitialization

	.local  AnalyzeAssignmentExpression
	.type AnalyzeAssignmentExpression, @function

AnalyzeAssignmentExpression:

	// *** Basic block 0

	.global AnalyzeExpression
	.local IsAssignable
	.global SemanticError
	.global NormalConversion
	.global ASTNodeSetType
	.global TypeIsIntegral
	.global NewPtrScaleASTNode
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
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	ld          a0, 56(s1)
	call        AnalyzeExpression

	// *** Basic block 1

	sd          a0, 56(s1)
	ld          a0, 64(s1)
	call        AnalyzeExpression

	// *** Basic block 2

	sd          a0, 64(s1)
	ld          s2, 56(s1)
	mv          a1, x0
	mv          a0, s2
	call        IsAssignable

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .AnalyzeAssignmentExpression_label_69

	// *** Basic block 4

	lla         a1, .str.13
	mv          a0, s2
	call        SemanticError

	// *** Basic block 5

.AnalyzeAssignmentExpression_label_69:
	ld          s2, 56(s1)
	lw          t0, 8(s2)
	ori         t0, t0, 1
	sw          t0, 8(s2)
	lw          s3, 0(s1)
	li          t0, 18		// 0x12 ASCII \x12
	beq         s3, t0, .AnalyzeAssignmentExpression_label_258

	// *** Basic block 6

	li          t0, 20		// 0x14 ASCII \x14
	beq         s3, t0, .AnalyzeAssignmentExpression_label_151

	// *** Basic block 7

	li          t0, 25		// 0x19 ASCII \x19
	beq         s3, t0, .AnalyzeAssignmentExpression_label_260

	// *** Basic block 8

	li          t0, 49		// 0x31 ASCII '1'
	beq         s3, t0, .AnalyzeAssignmentExpression_label_257

	// *** Basic block 9

	li          t0, 52		// 0x34 ASCII '4'
	beq         s3, t0, .AnalyzeAssignmentExpression_label_167

	// *** Basic block 10

	li          t0, 55		// 0x37 ASCII '7'
	beq         s3, t0, .AnalyzeAssignmentExpression_label_259

	// *** Basic block 11

	li          t0, 57		// 0x39 ASCII '9'
	beq         s3, t0, .AnalyzeAssignmentExpression_label_256

	// *** Basic block 12

	li          t0, 59		// 0x3b ASCII ';'
	beq         s3, t0, .AnalyzeAssignmentExpression_label_166

	// *** Basic block 13

	li          t0, 64		// 0x40 ASCII '@'
	beq         s3, t0, .AnalyzeAssignmentExpression_label_299

	// *** Basic block 14

	li          t0, 67		// 0x43 ASCII 'C'
	beq         s3, t0, .AnalyzeAssignmentExpression_label_241

	// *** Basic block 15

	li          t0, 69		// 0x45 ASCII 'E'
	beq         s3, t0, .AnalyzeAssignmentExpression_label_240

	// *** Basic block 16

.AnalyzeAssignmentExpression_label_132:
	lla         a0, .str.17
	lla         a1, .str.18
	lla         a3, .str.19
	li          t0, 536		// 0x218
	mv          a2, t0
	call        printf

	// *** Basic block 17

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           abort

	// *** Basic block 19

.AnalyzeAssignmentExpression_label_151:
	ld          a0, 64(s1)
	ld          s3, 16(s2)
	mv          a1, s3
	call        NormalConversion

	// *** Basic block 20

	mv          a1, s3
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 21

	j           .AnalyzeAssignmentExpression_label_350

	// *** Basic block 22

.AnalyzeAssignmentExpression_label_166:

	// *** Basic block 23

.AnalyzeAssignmentExpression_label_167:
	ld          s3, 16(s2)
	lw          t0, 16(s3)
	addi        t0, t0, -1
	seqz        s3, t0

	// *** Basic block 24

.AnalyzeAssignmentExpression_label_177:
	beqz        s3, .AnalyzeAssignmentExpression_label_223

	// *** Basic block 25

	j           .AnalyzeAssignmentExpression_label_180

	// *** Basic block 26

.AnalyzeAssignmentExpression_label_180:
	ld          s4, 64(s1)
	ld          a0, 16(s4)
	call        TypeIsIntegral

	// *** Basic block 27

	not         t0, a0
	beqz        t0, .AnalyzeAssignmentExpression_label_195

	// *** Basic block 28

	lla         a1, .str.14
	mv          a0, s1
	call        SemanticError

	// *** Basic block 29

.AnalyzeAssignmentExpression_label_195:
	ld          a0, 24(s3)
	ld          a3, 40(s4)
	mv          a2, s4
	li          t0, 68		// 0x44 ASCII 'D'
	mv          a1, t0
	call        NewPtrScaleASTNode

	// *** Basic block 30

	mv          s4, a0
	sd          s4, 64(s1)
	sd          s1, 24(s4)
	ld          t0, 56(s1)
	ld          a1, 16(t0)
	mv          a0, s4
	call        ASTNodeSetType

	// *** Basic block 31

	j           .AnalyzeAssignmentExpression_label_230

	// *** Basic block 32

.AnalyzeAssignmentExpression_label_223:
	ld          a0, 64(s1)
	mv          a1, s3
	call        NormalConversion

	// *** Basic block 33

.AnalyzeAssignmentExpression_label_230:
	ld          t0, 56(s1)
	ld          a1, 16(t0)
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 34

	j           .AnalyzeAssignmentExpression_label_350

	// *** Basic block 35

.AnalyzeAssignmentExpression_label_240:

	// *** Basic block 36

.AnalyzeAssignmentExpression_label_241:
	ld          a0, 64(s1)
	ld          s3, 16(s2)
	mv          a1, s3
	call        NormalConversion

	// *** Basic block 37

	mv          a1, s3
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 38

	j           .AnalyzeAssignmentExpression_label_350

	// *** Basic block 39

.AnalyzeAssignmentExpression_label_256:

	// *** Basic block 40

.AnalyzeAssignmentExpression_label_257:

	// *** Basic block 41

.AnalyzeAssignmentExpression_label_258:

	// *** Basic block 42

.AnalyzeAssignmentExpression_label_259:

	// *** Basic block 43

.AnalyzeAssignmentExpression_label_260:
	ld          s2, 16(s2)
	mv          a0, s2
	call        TypeIsIntegral

	// *** Basic block 44

	not         s3, a0
	bnez        s3, .AnalyzeAssignmentExpression_label_276

	// *** Basic block 45

	ld          t0, 64(s1)
	ld          a0, 16(t0)
	call        TypeIsIntegral

	// *** Basic block 46

	not         s3, a0

	// *** Basic block 47

.AnalyzeAssignmentExpression_label_276:
	beqz        s3, .AnalyzeAssignmentExpression_label_285

	// *** Basic block 48

	lla         a1, .str.15
	mv          a0, s1
	call        SemanticError

	// *** Basic block 49

	j           .AnalyzeAssignmentExpression_label_292

	// *** Basic block 50

.AnalyzeAssignmentExpression_label_285:
	ld          a0, 64(s1)
	mv          a1, s2
	call        NormalConversion

	// *** Basic block 51

.AnalyzeAssignmentExpression_label_292:
	mv          a1, s2
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 52

	j           .AnalyzeAssignmentExpression_label_350

	// *** Basic block 53

.AnalyzeAssignmentExpression_label_299:
	ld          s4, 16(s2)
	mv          a0, s4
	call        TypeIsIntegral

	// *** Basic block 54

	not         s3, a0
	bnez        s3, .AnalyzeAssignmentExpression_label_315

	// *** Basic block 55

	ld          t0, 64(s1)
	ld          a0, 16(t0)
	call        TypeIsIntegral

	// *** Basic block 56

	not         s3, a0

	// *** Basic block 57

.AnalyzeAssignmentExpression_label_315:
	beqz        s3, .AnalyzeAssignmentExpression_label_324

	// *** Basic block 58

	lla         a1, .str.16
	mv          a0, s1
	call        SemanticError

	// *** Basic block 59

	j           .AnalyzeAssignmentExpression_label_331

	// *** Basic block 60

.AnalyzeAssignmentExpression_label_324:
	ld          a0, 64(s1)
	mv          a1, s4
	call        NormalConversion

	// *** Basic block 61

.AnalyzeAssignmentExpression_label_331:
	mv          a1, s4
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 62

	ld          a0, 16(s1)
	call        TypeIsUnsigned

	// *** Basic block 63

	beqz        a0, .AnalyzeAssignmentExpression_label_345

	// *** Basic block 64

	li          s3, 15		// 0xf ASCII \xf
	sw          s3, 0(s1)
	j           .AnalyzeAssignmentExpression_label_348

	// *** Basic block 65

.AnalyzeAssignmentExpression_label_345:
	li          s3, 16		// 0x10 ASCII \x10
	sw          s3, 0(s1)

	// *** Basic block 66

.AnalyzeAssignmentExpression_label_348:
	j           .AnalyzeAssignmentExpression_label_350

	// *** Basic block 67

.AnalyzeAssignmentExpression_label_350:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AnalyzeAssignmentExpression:
	.size AnalyzeAssignmentExpression, .func_end_AnalyzeAssignmentExpression-AnalyzeAssignmentExpression

	.local  AnalyzeIncDec
	.type AnalyzeIncDec, @function

AnalyzeIncDec:

	// *** Basic block 0

	.local AnalyzeUnaryExpression
	.local IsAssignable
	.global SemanticError
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
	call        AnalyzeUnaryExpression

	// *** Basic block 1

	ld          s2, 56(s1)
	mv          a1, x0
	mv          a0, s2
	call        IsAssignable

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .AnalyzeIncDec_label_34

	// *** Basic block 3

	lla         a1, .str.20
	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SemanticError

	// *** Basic block 4

.AnalyzeIncDec_label_34:
	ld          t0, 56(s1)
	lw          t1, 8(t0)
	ori         t1, t1, 1
	sw          t1, 8(t0)
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AnalyzeIncDec:
	.size AnalyzeIncDec, .func_end_AnalyzeIncDec-AnalyzeIncDec

	.local  AnalyzeArraySubscript
	.type AnalyzeArraySubscript, @function

AnalyzeArraySubscript:

	// *** Basic block 0

	.global AnalyzeExpression
	.global TypeIsIntegral
	.global SemanticError
	.global ASTNodeSetType
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
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	ld          a0, 56(s1)
	call        AnalyzeExpression

	// *** Basic block 1

	sd          a0, 56(s1)
	ld          a0, 64(s1)
	call        AnalyzeExpression

	// *** Basic block 2

	sd          a0, 64(s1)
	ld          s3, 64(s1)
	sub         t0, s3, x0
	snez        s2, t0
	beq         s3, x0, .AnalyzeArraySubscript_label_42

	// *** Basic block 3

	ld          a0, 16(s3)
	call        TypeIsIntegral

	// *** Basic block 4

	not         s2, a0

	// *** Basic block 5

.AnalyzeArraySubscript_label_42:
	beqz        s2, .AnalyzeArraySubscript_label_51

	// *** Basic block 6

	lla         a1, .str.21
	mv          a0, s3
	call        SemanticError

	// *** Basic block 7

.AnalyzeArraySubscript_label_51:
	ld          s2, 56(s1)
	sub         t1, s2, x0
	snez        t0, t1
	beq         s2, x0, .AnalyzeArraySubscript_label_77

	// *** Basic block 8

	ld          s3, 16(s2)
	lw          t1, 16(s3)
	addi        t2, t1, -1
	seqz        s4, t2
	li          t2, 1		// 0x1 ASCII \x1
	beq         t1, t2, .AnalyzeArraySubscript_label_72

	// *** Basic block 9

	addi        t1, t1, -2
	seqz        s4, t1

	// *** Basic block 10

.AnalyzeArraySubscript_label_72:

	// *** Basic block 11

.AnalyzeArraySubscript_label_74:
	not         t0, s4
	j           .AnalyzeArraySubscript_label_77

	// *** Basic block 12

.AnalyzeArraySubscript_label_77:
	beqz        t0, .AnalyzeArraySubscript_label_100

	// *** Basic block 13

	lla         a1, .str.22
	mv          a0, s2
	call        SemanticError

	// *** Basic block 14

	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 15

	mv          a1, a0
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeSetType

	// *** Basic block 16

.AnalyzeArraySubscript_label_97:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 17

.AnalyzeArraySubscript_label_100:
	ld          t0, 16(s2)
	ld          s2, 24(t0)
	mv          a1, s2
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 18

	j           .AnalyzeArraySubscript_label_97
.func_end_AnalyzeArraySubscript:
	.size AnalyzeArraySubscript, .func_end_AnalyzeArraySubscript-AnalyzeArraySubscript

	.local  InlineFunctionBodyStatement
	.type InlineFunctionBodyStatement, @function

InlineFunctionBodyStatement:

	// *** Basic block 0

	.global MapFind
	.global NewVector
	.global ASTNodeMove
	.global NewIdentifierASTNode
	.global NewBinaryASTNode
	.global NewExpressionStatementASTNode
	.global AnalyzeStatement
	.global VectorAppend
	.global NewGotoStatementASTNode
	.global NewString
	.global ASTNodeDelete
	.global NewCompoundStatementASTNode
	sd          a0, -0(s0)	// Spilled @62
	sd          a0, -0(s0)	// Spilled @176
	sd          a0, -0(s0)	// Spilled @189
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -32(s0)
	// Spilled register region: 8 bytes at -40(s0) to -32(s0)
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
	lw          s3, 0(s1)
	li          t0, 2		// 0x2 ASCII \x2
	bne         s3, t0, .InlineFunctionBodyStatement_label_67

	// *** Basic block 1

	mv          s4, s1
	ld          t0, 56(s4)
	sd          t0, -32(s0)
	ld          a1, -32(s0)
	mv          a0, s2
	call        MapFind

	// *** Basic block 2

	mv          s5, a0
	beq         s5, x0, .InlineFunctionBodyStatement_label_61

	// *** Basic block 3

	sd          s5, 56(s4)

	// *** Basic block 4

.InlineFunctionBodyStatement_label_61:
	mv          a0, s1

	// *** Basic block 5

.InlineFunctionBodyStatement_label_64:
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

	// *** Basic block 6

.InlineFunctionBodyStatement_label_67:
	li          t0, 62		// 0x3e ASCII '>'
	bne         s3, t0, .InlineFunctionBodyStatement_label_179

	// *** Basic block 7

	mv          s6, s1
	call        NewVector

	// *** Basic block 8

	mv          s7, a0
	ld          s8, 40(s2)
	sub         t1, s8, x0
	snez        t0, t1
	beq         s8, x0, .InlineFunctionBodyStatement_label_88

	// *** Basic block 9

	ld          t1, 56(s6)
	sub         t1, t1, x0
	snez        t0, t1

	// *** Basic block 10

.InlineFunctionBodyStatement_label_88:
	beqz        t0, .InlineFunctionBodyStatement_label_139

	// *** Basic block 11

	ld          a0, 56(s6)
	call        ASTNodeMove

	// *** Basic block 12

	mv          s9, a0
	ld          a1, 40(s6)
	mv          a0, s8
	call        NewIdentifierASTNode

	// *** Basic block 13

	mv          s8, a0
	ld          a1, 16(s9)
	ld          a2, 40(s9)
	mv          a4, s9
	mv          a3, s8
	li          t0, 20		// 0x14 ASCII \x14
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 14

	mv          s10, a0
	ld          a1, 40(s10)
	mv          a0, s10
	call        NewExpressionStatementASTNode

	// *** Basic block 15

	mv          s11, a0
	mv          a0, s11
	call        AnalyzeStatement

	// *** Basic block 16

	mv          a1, s11
	mv          a0, s7
	call        VectorAppend

	// *** Basic block 17

.InlineFunctionBodyStatement_label_139:
	ld          a0, 32(s2)
	addi        t0, a0, 64
	ld          a0, 16(t0)
	call        NewString

	// *** Basic block 18

	ld          a0, 40(s6)
	mv          a1, a0
	call        NewGotoStatementASTNode

	// *** Basic block 19

	sd          a0, 64(a0)
	ld          t0, 48(s2)
	sd          t0, 72(a0)
	mv          a1, a0
	mv          a0, s7
	call        VectorAppend

	// *** Basic block 20

	mv          a0, s1
	call        ASTNodeDelete

	// *** Basic block 21

	mv          a1, a0
	mv          a0, s7
	call        NewCompoundStatementASTNode

	// *** Basic block 22

	j           .InlineFunctionBodyStatement_label_64

	// *** Basic block 23

.InlineFunctionBodyStatement_label_179:
	li          t0, 70		// 0x46 ASCII 'F'
	bne         s3, t0, .InlineFunctionBodyStatement_label_188

	// *** Basic block 24

	mv          a0, s1
	call        AnalyzeStatement

	// *** Basic block 25

.InlineFunctionBodyStatement_label_188:
	mv          a0, s1
	j           .InlineFunctionBodyStatement_label_64
.func_end_InlineFunctionBodyStatement:
	.size InlineFunctionBodyStatement, .func_end_InlineFunctionBodyStatement-InlineFunctionBodyStatement

	.local  CopyArguments
	.type CopyArguments, @function

CopyArguments:

	// *** Basic block 0

	.global NewVector
	.global SymbolClone
	.global VectorAppend
	.global compiler
	.global MapInsert
	.global ASTNodeMove
	.global NewBinaryASTNode
	.global NewIdentifierASTNode
	.global AnalyzeExpression
	.global NewVariableDeclarationASTNode
	.global TypeIsVoid
	.global SyntaxNewTemporary
	.global NewDeclarationListASTNode
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -32(s0)
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
	mv          s3, a2
	ld          s4, 40(s1)
	call        NewVector

	// *** Basic block 1

	ld          t0, 64(s1)
	ld          s5, 0(t0)
	mv          s6, a0
	mv          s7, x0
	addi        t0, s2, 8
	ld          s8, 8(t0)
	bge         x0, s8, .CopyArguments_label_153

	// *** Basic block 2

	ld          s9, 8(s2)

	// *** Basic block 3

.CopyArguments_label_56:
	slli        s2, s7, 3
	add         t0, s9, s2
	ld          s9, 0(t0)
	mv          a0, s9
	call        SymbolClone

	// *** Basic block 4

	mv          s10, a0
	lb          t0, 56(s10)
	andi        t0, t0, -17
	sb          t0, 56(s10)
	lb          t0, 56(s10)
	andi        t0, t0, -9
	ori         t0, t0, 8
	sb          t0, 56(s10)
	la          t0, compiler
	ld          t0, 0(t0)
	addi        t0, t0, 448
	addi        a0, t0, 80
	mv          a1, s10
	call        VectorAppend

	// *** Basic block 5

	sd          s9, -32(s0)
	addi        t0, s0, -32
	sd          s10, 8(t0)
	addi        sp, sp, -16
	ld          t0, -32(s0)
	sd          t0, 0(sp)
	ld          t0, -24(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s3
	call        MapInsert

	// *** Basic block 6

	addi        sp, sp, 16
	add         t0, s5, s2
	ld          a0, 0(t0)
	call        ASTNodeMove

	// *** Basic block 7

	mv          s2, a0
	ld          s5, 16(s2)
	ld          s9, 40(s2)
	mv          a1, s4
	mv          a0, s10
	call        NewIdentifierASTNode

	// *** Basic block 8

	mv          a4, s2
	mv          a3, a0
	mv          a2, s9
	mv          a1, s5
	li          t0, 20		// 0x14 ASCII \x14
	mv          a0, t0
	call        NewBinaryASTNode

	// *** Basic block 9

	mv          s5, a0
	mv          a0, s5
	call        AnalyzeExpression

	// *** Basic block 10

	mv          s5, a0
	mv          a2, s4
	mv          a1, s5
	mv          a0, s10
	call        NewVariableDeclarationASTNode

	// *** Basic block 11

	mv          a1, a0
	mv          a0, s6
	call        VectorAppend

	// *** Basic block 12

.CopyArguments_label_149:
	addi        s7, s7, 1
	bge         s7, s8, .CopyArguments_label_56

	// *** Basic block 13

.CopyArguments_label_153:
	ld          s8, 16(s1)
	mv          a0, s8
	call        TypeIsVoid

	// *** Basic block 14

	not         t0, a0
	beqz        t0, .CopyArguments_label_185

	// *** Basic block 15

	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 448
	mv          a1, s8
	call        SyntaxNewTemporary

	// *** Basic block 16

	mv          s1, a0
	sd          s1, 40(s3)
	mv          a2, s4
	mv          a1, x0
	mv          a0, s1
	call        NewVariableDeclarationASTNode

	// *** Basic block 17

	mv          a1, a0
	mv          a0, s6
	call        VectorAppend

	// *** Basic block 18

	j           .CopyArguments_label_188

	// *** Basic block 19

.CopyArguments_label_185:
	sd          x0, 40(s3)

	// *** Basic block 20

.CopyArguments_label_188:
	mv          a1, s4
	mv          a0, s6
	call        NewDeclarationListASTNode

	// *** Basic block 21


	// *** Basic block 22

.CopyArguments_label_196:
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
.func_end_CopyArguments:
	.size CopyArguments, .func_end_CopyArguments-CopyArguments

	.local  InlineFunctionCall
	.type InlineFunctionCall, @function

InlineFunctionCall:

	// *** Basic block 0

	.global NewVector
	.global MapInitForPointerKeys
	.global VectorAppend
	.local CopyArguments
	.global NewCompoundStatementASTNode
	.global NewLabelASTNode
	.global SyntaxFakeName
	.global compiler
	.global ASTNodeClone
	.local InlineFunctionBodyStatement
	.global NewIdentifierASTNode
	.global MapDestruct
	.global NewInlineCallASTNode
	addi sp, sp, -160
	// Saved return address (offset 152) and frame pointer (offset 144)
	sd ra, 152(sp)
	sd s0, 144(sp)
	addi s0, sp, 160
	// Local vars at offset -80(s0)
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
	call        NewVector

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 40(s1)
	addi        a0, s0, -80
	call        MapInitForPointerKeys

	// *** Basic block 2

	addi        a2, s0, -80
	mv          a1, s1
	mv          a0, s2
	call        CopyArguments

	// *** Basic block 3

	ld          s5, 0(s3)
	mv          a1, a0
	mv          a0, s3
	call        VectorAppend

	// *** Basic block 4

	ld          s6, 40(s2)
	ld          a1, 40(s6)
	mv          a0, s3
	call        NewCompoundStatementASTNode

	// *** Basic block 5

	mv          s7, a0
	addi        t0, s0, -80
	sd          s7, 48(t0)
	addi        s8, s0, -80
	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 448
	call        SyntaxFakeName

	// *** Basic block 6

	mv          a3, s4
	mv          a2, x0
	mv          a1, x0
	call        NewLabelASTNode

	// *** Basic block 7

	sd          a0, 32(s8)
	addi        a2, s0, -80
	mv          a3, x0
	la          t0, InlineFunctionBodyStatement
	mv          a1, t0
	mv          a0, s6
	call        ASTNodeClone

	// *** Basic block 8

	mv          s6, a0
	mv          a1, s6
	mv          a0, s3
	call        VectorAppend

	// *** Basic block 9

	addi        t0, s0, -80
	ld          a1, 32(t0)
	mv          a0, s3
	call        VectorAppend

	// *** Basic block 10

	mv          s8, x0
	ld          s9, 8(s3)
	bge         x0, s9, .InlineFunctionCall_label_133

	// *** Basic block 11

.InlineFunctionCall_label_118:
	slli        t0, s8, 3
	add         t0, s5, t0
	ld          s2, 0(t0)
	sd          s7, 24(s2)
	sext.w      t0, s8
	sw          t0, 32(s2)

	// *** Basic block 12

.InlineFunctionCall_label_129:
	addi        s8, s8, 1
	bge         s8, s9, .InlineFunctionCall_label_118

	// *** Basic block 13

.InlineFunctionCall_label_133:
	mv          s2, x0
	addi        t0, s0, -80
	ld          s3, 40(t0)
	beq         s3, x0, .InlineFunctionCall_label_148

	// *** Basic block 14

	mv          a1, s4
	mv          a0, s3
	call        NewIdentifierASTNode

	// *** Basic block 15

	mv          s2, a0

	// *** Basic block 16

.InlineFunctionCall_label_148:
	addi        a0, s0, -80
	call        MapDestruct

	// *** Basic block 17

	ld          a0, 16(s1)
	mv          a3, s2
	mv          a2, s7
	mv          a1, s4
	call        NewInlineCallASTNode

	// *** Basic block 18


	// *** Basic block 19

.InlineFunctionCall_label_164:
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
.func_end_InlineFunctionCall:
	.size InlineFunctionCall, .func_end_InlineFunctionCall-InlineFunctionCall

	.local  ExamineBody
	.type ExamineBody, @function

ExamineBody:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	lw          t1, 0(t0)
	addi        t1, t1, 1
	sw          t1, 0(t0)
	lw          t1, 0(a0)
	li          t2, 36		// 0x24 ASCII '$'
	bne         t1, t2, .ExamineBody_label_27

	// *** Basic block 1

	li          t1, 1		// 0x1 ASCII \x1
	sb          t1, 4(t0)

	// *** Basic block 2

.ExamineBody_label_27:
	ret         
.func_end_ExamineBody:
	.size ExamineBody, .func_end_ExamineBody-ExamineBody

	.local  FunctionCanBeInlined
	.type FunctionCanBeInlined, @function

FunctionCanBeInlined:

	// *** Basic block 0

	.global OptLevel2
	.global compiler
	.global ASTNodeVisit
	.local ExamineBody
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	call        OptLevel2

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .FunctionCanBeInlined_label_28

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.FunctionCanBeInlined_label_25:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.FunctionCanBeInlined_label_28:
	lb          t1, 51(s1)
	not         t0, t1
	bnez        t0, .FunctionCanBeInlined_label_44

	// *** Basic block 5

	ld          t1, 0(s1)
	lb          t1, 56(t1)
	slli        t1, t1, 63
	srai        t1, t1, 63
	not         t0, t1

	// *** Basic block 6

.FunctionCanBeInlined_label_44:
	bnez        t0, .FunctionCanBeInlined_label_50

	// *** Basic block 7

	ld          t1, 40(s1)
	sub         t1, t1, x0
	seqz        t0, t1

	// *** Basic block 8

.FunctionCanBeInlined_label_50:
	bnez        t0, .FunctionCanBeInlined_label_61

	// *** Basic block 9

	ld          t1, 0(s1)
	la          t2, compiler
	ld          t2, 0(t2)
	ld          t2, 1096(t2)
	ld          t2, 32(t2)
	sub         t1, t1, t2
	seqz        t0, t1

	// *** Basic block 10

.FunctionCanBeInlined_label_61:
	bnez        t0, .FunctionCanBeInlined_label_65

	// *** Basic block 11

	lb          t0, 48(s1)

	// *** Basic block 12

.FunctionCanBeInlined_label_65:
	bnez        t0, .FunctionCanBeInlined_label_69

	// *** Basic block 13

	lb          t0, 32(s1)

	// *** Basic block 14

.FunctionCanBeInlined_label_69:
	beqz        t0, .FunctionCanBeInlined_label_74

	// *** Basic block 15

	mv          a0, x0
	j           .FunctionCanBeInlined_label_25

	// *** Basic block 16

.FunctionCanBeInlined_label_74:
	sd          x0, -32(s0)
	sw          x0, -32(s0)
	addi        t0, s0, -32
	sb          x0, 4(t0)
	ld          a0, 40(s1)
	addi        a3, s0, -32
	mv          a2, x0
	la          t0, ExamineBody
	mv          a1, t0
	call        ASTNodeVisit

	// *** Basic block 17

	addi        t0, s0, -32
	lb          t0, 4(t0)
	beqz        t0, .FunctionCanBeInlined_label_105

	// *** Basic block 18

	mv          a0, x0
	j           .FunctionCanBeInlined_label_25

	// *** Basic block 19

.FunctionCanBeInlined_label_105:
	lw          t0, -32(s0)
	slti        a0, t0, 100
	j           .FunctionCanBeInlined_label_25
.func_end_FunctionCanBeInlined:
	.size FunctionCanBeInlined, .func_end_FunctionCanBeInlined-FunctionCanBeInlined

	.local  AnalyzeFunctionCall
	.type AnalyzeFunctionCall, @function

AnalyzeFunctionCall:

	// *** Basic block 0

	.global AnalyzeExpression
	.global TypeIsFunctionPointer
	.global SemanticError
	.global ASTNodeSetType
	.global NewTypeRecord
	.global NormalConversion
	.global TypeIsStructOrUnion
	.local FunctionCanBeInlined
	.local InlineFunctionCall
	.global ASTNodeReplaceChild
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
	mv          s1, a0
	ld          a0, 56(s1)
	call        AnalyzeExpression

	// *** Basic block 1

	sd          a0, 56(s1)
	ld          t0, 64(s1)
	ld          s2, 8(t0)
	mv          s3, x0
	bge         x0, s2, .AnalyzeFunctionCall_label_65

	// *** Basic block 2

.AnalyzeFunctionCall_label_47:
	ld          t0, 64(s1)
	ld          t0, 0(t0)
	slli        t1, s3, 3
	add         s4, t0, t1
	ld          t0, 64(s1)
	ld          t0, 0(t0)
	add         t0, t0, t1
	ld          a0, 0(t0)
	call        AnalyzeExpression

	// *** Basic block 3

	sd          a0, 0(s4)

	// *** Basic block 4

.AnalyzeFunctionCall_label_61:
	addi        s3, s3, 1
	bge         s3, s2, .AnalyzeFunctionCall_label_47

	// *** Basic block 5

.AnalyzeFunctionCall_label_65:
	ld          s5, 56(s1)
	sub         t0, s5, x0
	snez        s4, t0
	beq         s5, x0, .AnalyzeFunctionCall_label_80

	// *** Basic block 6

	ld          t0, 64(s1)
	ld          s6, 0(t0)
	ld          a0, 16(s5)
	call        TypeIsFunctionPointer

	// *** Basic block 7

	not         s4, a0

	// *** Basic block 8

.AnalyzeFunctionCall_label_80:
	beqz        s4, .AnalyzeFunctionCall_label_105

	// *** Basic block 9

	lla         a1, .str.23
	mv          a0, s5
	call        SemanticError

	// *** Basic block 10

	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 11

	mv          a1, a0
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 12

	mv          a0, s1

	// *** Basic block 13

.AnalyzeFunctionCall_label_102:
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

	// *** Basic block 14

.AnalyzeFunctionCall_label_105:
	bne         s5, x0, .AnalyzeFunctionCall_label_112

	// *** Basic block 15

	mv          a0, s1
	j           .AnalyzeFunctionCall_label_102

	// *** Basic block 16

.AnalyzeFunctionCall_label_112:
	ld          s4, 16(s5)
	mv          s5, s4
	lw          t0, 16(s5)
	addi        t0, t0, -1
	seqz        s6, t0

	// *** Basic block 17

.AnalyzeFunctionCall_label_124:
	beqz        s6, .AnalyzeFunctionCall_label_130

	// *** Basic block 18

	j           .AnalyzeFunctionCall_label_127

	// *** Basic block 19

.AnalyzeFunctionCall_label_127:
	ld          s4, 24(s4)

	// *** Basic block 20

.AnalyzeFunctionCall_label_130:
	ld          a1, 24(s4)
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 21

	li          s5, 1		// 0x1 ASCII \x1
	addi        s4, s4, 32
	addi        t0, s4, 8
	ld          s6, 8(t0)
	addi        s7, s4, 32
	lb          t0, 48(s7)
	not         t0, t0
	beqz        t0, .AnalyzeFunctionCall_label_257

	// *** Basic block 22

	ld          s8, 8(s4)
	bge         s2, s6, .AnalyzeFunctionCall_label_165

	// *** Basic block 23

	lla         a1, .str.24
	mv          a3, s2
	mv          a2, s6
	mv          a0, s1
	call        SemanticError

	// *** Basic block 24

	mv          s5, x0

	// *** Basic block 25

.AnalyzeFunctionCall_label_165:
	lb          t1, 32(s7)
	not         t0, t1
	beqz        t0, .AnalyzeFunctionCall_label_173

	// *** Basic block 26

	sub         t1, s2, s6
	snez        t0, t1

	// *** Basic block 27

.AnalyzeFunctionCall_label_173:
	beqz        t0, .AnalyzeFunctionCall_label_186

	// *** Basic block 28

	lla         a1, .str.25
	mv          a3, s2
	mv          a2, s6
	mv          a0, s1
	call        SemanticError

	// *** Basic block 29

	mv          s5, x0

	// *** Basic block 30

.AnalyzeFunctionCall_label_186:
	mv          s7, x0
	slt         t0, x0, s6
	bge         x0, s6, .AnalyzeFunctionCall_label_193

	// *** Basic block 31

	slt         t0, x0, s2

	// *** Basic block 32

.AnalyzeFunctionCall_label_193:
	beqz        t0, .AnalyzeFunctionCall_label_256

	// *** Basic block 33

.AnalyzeFunctionCall_label_195:
	slli        t0, s7, 3
	add         t1, s6, t0
	ld          s9, 0(t1)
	add         t0, s8, t0
	ld          s8, 0(t0)
	ld          a1, 40(s8)
	mv          a0, s9
	call        NormalConversion

	// *** Basic block 34

	ld          s10, 16(s9)
	mv          a0, s10
	call        TypeIsStructOrUnion

	// *** Basic block 35

	mv          s8, a0
	bnez        a0, .AnalyzeFunctionCall_label_228

	// *** Basic block 36

	lw          t0, 16(s10)
	addi        t0, t0, -2
	seqz        s10, t0

	// *** Basic block 37

.AnalyzeFunctionCall_label_225:
	mv          s8, s10
	j           .AnalyzeFunctionCall_label_228

	// *** Basic block 38

.AnalyzeFunctionCall_label_228:
	bnez        s8, .AnalyzeFunctionCall_label_241

	// *** Basic block 39

	mv          s11, s10
	lw          t0, 16(s11)
	addi        t0, t0, -3
	seqz        s10, t0

	// *** Basic block 40

.AnalyzeFunctionCall_label_238:
	mv          s8, s10
	j           .AnalyzeFunctionCall_label_241

	// *** Basic block 41

.AnalyzeFunctionCall_label_241:
	beqz        s8, .AnalyzeFunctionCall_label_247

	// *** Basic block 42

	lw          t0, 8(s9)
	ori         t0, t0, 1
	sw          t0, 8(s9)

	// *** Basic block 43

.AnalyzeFunctionCall_label_247:

	// *** Basic block 44

.AnalyzeFunctionCall_label_248:
	addi        s7, s7, 1
	slt         t0, s7, s6
	bge         s7, s6, .AnalyzeFunctionCall_label_254

	// *** Basic block 45

	slt         t0, s7, s2

	// *** Basic block 46

.AnalyzeFunctionCall_label_254:
	beqz        t0, .AnalyzeFunctionCall_label_195

	// *** Basic block 47

.AnalyzeFunctionCall_label_256:

	// *** Basic block 48

.AnalyzeFunctionCall_label_257:
	mv          s2, s5
	beqz        s5, .AnalyzeFunctionCall_label_271

	// *** Basic block 49

	mv          s6, s4
	lw          t0, 16(s6)
	addi        t0, t0, -3
	seqz        s4, t0

	// *** Basic block 50

.AnalyzeFunctionCall_label_268:
	mv          s2, s4
	j           .AnalyzeFunctionCall_label_271

	// *** Basic block 51

.AnalyzeFunctionCall_label_271:
	beqz        s2, .AnalyzeFunctionCall_label_306

	// *** Basic block 52

	ld          t0, 56(s1)
	ld          t0, 16(t0)
	addi        s4, t0, 32
	mv          a0, s4
	call        FunctionCanBeInlined

	// *** Basic block 53

	beqz        a0, .AnalyzeFunctionCall_label_305

	// *** Basic block 54

	mv          a1, s1
	mv          a0, s4
	call        InlineFunctionCall

	// *** Basic block 55

	mv          s4, a0
	ld          a0, 24(s1)
	lw          a1, 32(s1)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, s4
	call        ASTNodeReplaceChild

	// *** Basic block 56

	mv          a0, s4
	j           .AnalyzeFunctionCall_label_102

	// *** Basic block 57

.AnalyzeFunctionCall_label_305:

	// *** Basic block 58

.AnalyzeFunctionCall_label_306:
	mv          a0, s1
	j           .AnalyzeFunctionCall_label_102
.func_end_AnalyzeFunctionCall:
	.size AnalyzeFunctionCall, .func_end_AnalyzeFunctionCall-AnalyzeFunctionCall

	.local  AnalyzeMemberReference
	.type AnalyzeMemberReference, @function

AnalyzeMemberReference:

	// *** Basic block 0

	.global AnalyzeExpression
	.global TypeIsStructOrUnionPointer
	.global SemanticError
	.global TypeIsStructOrUnion
	.global ASTNodeSetType
	.global NewTypeRecord
	.global FindStructMember
	.global NewStructMemberASTNode
	.global ASTNodeDelete
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
	ld          t0, 16(s1)
	beq         t0, x0, .AnalyzeMemberReference_label_38

	// *** Basic block 1

.AnalyzeMemberReference_label_35:
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

.AnalyzeMemberReference_label_38:
	mv          s2, x0
	ld          a0, 56(s1)
	call        AnalyzeExpression

	// *** Basic block 3

	sd          a0, 56(s1)
	ld          a0, 64(s1)
	call        AnalyzeExpression

	// *** Basic block 4

	sd          a0, 64(s1)
	lw          t0, 0(s1)
	li          t1, 19		// 0x13 ASCII \x13
	bne         t0, t1, .AnalyzeMemberReference_label_83

	// *** Basic block 5

	ld          t0, 56(s1)
	ld          s3, 16(t0)
	mv          a0, s3
	call        TypeIsStructOrUnionPointer

	// *** Basic block 6

	not         t0, a0
	beqz        t0, .AnalyzeMemberReference_label_76

	// *** Basic block 7

	lla         a1, .str.26
	mv          a0, s1
	call        SemanticError

	// *** Basic block 8

	j           .AnalyzeMemberReference_label_81

	// *** Basic block 9

.AnalyzeMemberReference_label_76:
	ld          t0, 24(s3)
	ld          s2, 32(t0)

	// *** Basic block 10

.AnalyzeMemberReference_label_81:
	j           .AnalyzeMemberReference_label_117

	// *** Basic block 11

.AnalyzeMemberReference_label_83:
	ld          t0, 56(s1)
	ld          s3, 16(t0)
	mv          a0, s3
	call        TypeIsStructOrUnion

	// *** Basic block 12

	not         t0, a0
	beqz        t0, .AnalyzeMemberReference_label_113

	// *** Basic block 13

	mv          a0, s3
	call        TypeIsStructOrUnionPointer

	// *** Basic block 14

	beqz        a0, .AnalyzeMemberReference_label_104

	// *** Basic block 15

	lla         a1, .str.27
	mv          a0, s1
	call        SemanticError

	// *** Basic block 16

	j           .AnalyzeMemberReference_label_111

	// *** Basic block 17

.AnalyzeMemberReference_label_104:
	lla         a1, .str.28
	mv          a0, s1
	call        SemanticError

	// *** Basic block 18

.AnalyzeMemberReference_label_111:
	j           .AnalyzeMemberReference_label_116

	// *** Basic block 19

.AnalyzeMemberReference_label_113:
	ld          s2, 32(s3)

	// *** Basic block 20

.AnalyzeMemberReference_label_116:

	// *** Basic block 21

.AnalyzeMemberReference_label_117:
	bne         s2, x0, .AnalyzeMemberReference_label_133

	// *** Basic block 22

	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 23

	mv          a1, a0
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 24

	j           .AnalyzeMemberReference_label_35

	// *** Basic block 25

.AnalyzeMemberReference_label_133:
	ld          t0, 56(s1)
	ld          a0, 16(t0)
	call        TypeIsStructOrUnion

	// *** Basic block 26

	beqz        a0, .AnalyzeMemberReference_label_147

	// *** Basic block 27

	ld          t0, 56(s1)
	lw          t1, 8(t0)
	ori         t1, t1, 1
	sw          t1, 8(t0)

	// *** Basic block 28

.AnalyzeMemberReference_label_147:
	ld          s3, 64(s1)
	ld          s4, 56(s3)
	mv          a1, s4
	mv          a0, s2
	call        FindStructMember

	// *** Basic block 29

	mv          s5, a0
	bne         s5, x0, .AnalyzeMemberReference_label_191

	// *** Basic block 30

	lla         a1, .str.29
	ld          a2, 16(s4)
	ld          t0, 8(s2)
	ld          a3, 16(t0)
	mv          a0, s1
	call        SemanticError

	// *** Basic block 31

	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 32

	mv          a1, a0
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 33

	j           .AnalyzeMemberReference_label_35

	// *** Basic block 34

.AnalyzeMemberReference_label_191:
	ld          a1, 40(s3)
	mv          a0, s5
	call        NewStructMemberASTNode

	// *** Basic block 35

	sd          a0, 64(s1)
	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 36

	ld          t0, 0(s5)
	ld          a1, 40(t0)
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 37

	j           .AnalyzeMemberReference_label_35
.func_end_AnalyzeMemberReference:
	.size AnalyzeMemberReference, .func_end_AnalyzeMemberReference-AnalyzeMemberReference

	.local  AnalyzeAddressOperator
	.type AnalyzeAddressOperator, @function

AnalyzeAddressOperator:

	// *** Basic block 0

	.global AnalyzeExpression
	.global NewPointerTypeRecord
	.local HasAddress
	.global SemanticError
	.global NewTypeRecord
	.global TypeRecordChain
	.global ASTNodeSetType
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
	ld          a0, 56(s1)
	call        AnalyzeExpression

	// *** Basic block 1

	sd          a0, 56(s1)
	mv          a0, x0
	call        NewPointerTypeRecord

	// *** Basic block 2

	mv          s2, a0
	ld          s3, 56(s1)
	mv          a0, s3
	call        HasAddress

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .AnalyzeAddressOperator_label_66

	// *** Basic block 4

	lla         a1, .str.30
	mv          a0, s3
	call        SemanticError

	// *** Basic block 5

	mv          a1, x0
	li          t0, 512		// 0x200
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 6

	mv          s4, a0
	mv          a1, s4
	mv          a0, s2
	call        TypeRecordChain

	// *** Basic block 7

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
	j           ASTNodeSetType

	// *** Basic block 8

.AnalyzeAddressOperator_label_63:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 9

.AnalyzeAddressOperator_label_66:
	ld          a1, 16(s3)
	mv          a0, s2
	call        TypeRecordChain

	// *** Basic block 10

	mv          a1, s2
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 11

	ld          t0, 56(s1)
	lw          t1, 8(t0)
	ori         t1, t1, 1
	sw          t1, 8(t0)
	j           .AnalyzeAddressOperator_label_63
.func_end_AnalyzeAddressOperator:
	.size AnalyzeAddressOperator, .func_end_AnalyzeAddressOperator-AnalyzeAddressOperator

	.local  AnalyzeContentsOperator
	.type AnalyzeContentsOperator, @function

AnalyzeContentsOperator:

	// *** Basic block 0

	.global AnalyzeExpression
	.global SemanticError
	.global ASTNodeSetType
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
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	ld          a0, 56(s1)
	call        AnalyzeExpression

	// *** Basic block 1

	sd          a0, 56(s1)
	ld          s2, 56(s1)
	ld          s3, 16(s2)
	lw          s3, 16(s3)
	addi        t0, s3, -1
	seqz        s4, t0
	li          t0, 1		// 0x1 ASCII \x1
	beq         s3, t0, .AnalyzeContentsOperator_label_38

	// *** Basic block 2

	addi        t0, s3, -2
	seqz        s4, t0

	// *** Basic block 3

.AnalyzeContentsOperator_label_38:

	// *** Basic block 4

.AnalyzeContentsOperator_label_40:
	not         t0, s4
	beqz        t0, .AnalyzeContentsOperator_label_67

	// *** Basic block 5

	j           .AnalyzeContentsOperator_label_44

	// *** Basic block 6

.AnalyzeContentsOperator_label_44:
	lla         a1, .str.31
	mv          a0, s2
	call        SemanticError

	// *** Basic block 7

	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeSetType

	// *** Basic block 9

.AnalyzeContentsOperator_label_64:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 10

.AnalyzeContentsOperator_label_67:
	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 11

	mv          a1, a0
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 12

	ld          a1, 24(s3)
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 13

	j           .AnalyzeContentsOperator_label_64
.func_end_AnalyzeContentsOperator:
	.size AnalyzeContentsOperator, .func_end_AnalyzeContentsOperator-AnalyzeContentsOperator

	.local  AnalyzeSizeofExpression
	.type AnalyzeSizeofExpression, @function

AnalyzeSizeofExpression:

	// *** Basic block 0

	.global AnalyzeExpression
	.global ASTNodeSetType
	.global NewSizeTypeRecord
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
	ld          s2, 64(s1)
	beq         s2, x0, .AnalyzeSizeofExpression_label_65

	// *** Basic block 1

	mv          a0, s2
	call        AnalyzeExpression

	// *** Basic block 2

	sd          a0, 64(s1)
	ld          t0, 64(s1)
	ld          s2, 16(t0)
	lw          t0, 16(s2)
	addi        t1, t0, -2
	seqz        s3, t1
	li          t1, 2		// 0x2 ASCII \x2
	beq         t0, t1, .AnalyzeSizeofExpression_label_45

	// *** Basic block 3

	addi        t0, t0, -1
	seqz        s3, t0

	// *** Basic block 4

.AnalyzeSizeofExpression_label_45:
	beqz        s3, .AnalyzeSizeofExpression_label_52

	// *** Basic block 5

	addi        t0, s2, 32
	lb          t0, 16(t0)
	slli        t0, t0, 61
	srai        s3, t0, 63

	// *** Basic block 6

.AnalyzeSizeofExpression_label_52:

	// *** Basic block 7

.AnalyzeSizeofExpression_label_54:
	beqz        s3, .AnalyzeSizeofExpression_label_59

	// *** Basic block 8

	j           .AnalyzeSizeofExpression_label_57

	// *** Basic block 9

.AnalyzeSizeofExpression_label_57:
	j           .AnalyzeSizeofExpression_label_64

	// *** Basic block 10

.AnalyzeSizeofExpression_label_59:
	lw          t0, 20(s2)
	sd          t0, 56(s1)

	// *** Basic block 11

.AnalyzeSizeofExpression_label_64:

	// *** Basic block 12

.AnalyzeSizeofExpression_label_65:
	call        NewSizeTypeRecord

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeSetType
.func_end_AnalyzeSizeofExpression:
	.size AnalyzeSizeofExpression, .func_end_AnalyzeSizeofExpression-AnalyzeSizeofExpression

	.local  AnalyzeCastExpression
	.type AnalyzeCastExpression, @function

AnalyzeCastExpression:

	// *** Basic block 0

	.global AnalyzeExpression
	.global SemanticConvertType
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
	mv          s1, a0
	ld          a0, 64(s1)
	call        AnalyzeExpression

	// *** Basic block 1

	sd          a0, 64(s1)
	ld          a0, 64(s1)
	ld          s2, 56(s1)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a1, s2
	call        SemanticConvertType

	// *** Basic block 2

	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeSetType
.func_end_AnalyzeCastExpression:
	.size AnalyzeCastExpression, .func_end_AnalyzeCastExpression-AnalyzeCastExpression

	.local  AnalyzeLogicalOperator
	.type AnalyzeLogicalOperator, @function

AnalyzeLogicalOperator:

	// *** Basic block 0

	.local AnalyzeBinaryExpression
	.global NewTypeRecord
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
	mv          s1, a0
	call        AnalyzeBinaryExpression

	// *** Basic block 1

	mv          a1, x0
	li          t0, 256		// 0x100
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 2

	mv          s2, a0
	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeSetType
.func_end_AnalyzeLogicalOperator:
	.size AnalyzeLogicalOperator, .func_end_AnalyzeLogicalOperator-AnalyzeLogicalOperator

	.local  AnalyzeVarargsBuiltin1
	.type AnalyzeVarargsBuiltin1, @function

AnalyzeVarargsBuiltin1:

	// *** Basic block 0

	.global AnalyzeExpression
	.global ASTNodeSetType
	.global NewTypeRecord
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
	mv          s2, x0
	ld          t0, 64(s1)
	ld          t0, 8(t0)
	bge         x0, t0, .AnalyzeVarargsBuiltin1_label_52

	// *** Basic block 1

.AnalyzeVarargsBuiltin1_label_24:
	ld          t0, 64(s1)
	ld          t0, 0(t0)
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s3, 0(t0)
	ld          s4, 64(s1)
	ld          t0, 0(s4)
	add         s5, t0, t1
	mv          a0, s3
	call        AnalyzeExpression

	// *** Basic block 2

	sd          a0, 0(s5)
	ld          s3, 0(s5)
	lw          t0, 8(s3)
	ori         t0, t0, 1
	sw          t0, 8(s3)

	// *** Basic block 3

.AnalyzeVarargsBuiltin1_label_46:
	addi        s2, s2, 1
	ld          t0, 8(s4)
	bge         s2, t0, .AnalyzeVarargsBuiltin1_label_24

	// *** Basic block 4

.AnalyzeVarargsBuiltin1_label_52:
	mv          a1, x0
	li          t0, 512		// 0x200
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 5

	mv          a1, a0
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
	j           ASTNodeSetType
.func_end_AnalyzeVarargsBuiltin1:
	.size AnalyzeVarargsBuiltin1, .func_end_AnalyzeVarargsBuiltin1-AnalyzeVarargsBuiltin1

	.local  AnalyzeVarargsBuiltin2
	.type AnalyzeVarargsBuiltin2, @function

AnalyzeVarargsBuiltin2:

	// *** Basic block 0

	.global ASTNodeSetType
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 64(t0)
	ld          t1, 0(t1)
	ld          t2, 0(t1)
	lw          t3, 8(t2)
	ori         t3, t3, 1
	sw          t3, 8(t2)
	ld          t2, 8(t1)
	ld          a1, 16(t2)
	j           ASTNodeSetType
.func_end_AnalyzeVarargsBuiltin2:
	.size AnalyzeVarargsBuiltin2, .func_end_AnalyzeVarargsBuiltin2-AnalyzeVarargsBuiltin2

	.global AnalyzeExpression
	.type AnalyzeExpression, @function

AnalyzeExpression:

	// *** Basic block 0

	.local AnalyzeIdentifier
	.local AnalyzePlusOperator
	.local AnalyzeMinusOperator
	.local AnalyzeBinaryExpression
	.local InsertNumericConversions
	.global TypeIsIntegral
	.global SemanticError
	.local AnalyzeShift
	.local AnalyzeBitwiseOperator
	.local AnalyzeComparisonOperator
	.local AnalyzeConditionalExpression
	.local AnalyzeAssignmentExpression
	.local AnalyzeIncDec
	.local AnalyzeUnaryExpression
	.local AnalyzeSizeofExpression
	.local AnalyzeCastExpression
	.local AnalyzeAddressOperator
	.local AnalyzeContentsOperator
	.local AnalyzeArraySubscript
	.local AnalyzeFunctionCall
	.local AnalyzeMemberReference
	.global AnalyzeExpression
	.global ASTNodeSetType
	.local AnalyzeLogicalOperator
	.local AnalyzeInitialization
	.local AnalyzeVarargsBuiltin1
	.local AnalyzeVarargsBuiltin2
	.local FoldConstantExpression
	.global NewTypeRecord
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
	sub         t1, s1, x0
	seqz        t0, t1
	beq         s1, x0, .AnalyzeExpression_label_52

	// *** Basic block 1

	lw          t1, 8(s1)
	andi        t1, t1, 8
	snez        t0, t1

	// *** Basic block 2

.AnalyzeExpression_label_52:
	beqz        t0, .AnalyzeExpression_label_59

	// *** Basic block 3

	mv          a0, s1

	// *** Basic block 4

.AnalyzeExpression_label_56:
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

.AnalyzeExpression_label_59:
	mv          s2, s1
	mv          s3, s1
	mv          s4, s1
	lw          s5, 0(s1)
	li          t0, 1		// 0x1 ASCII \x1
	blt         s5, t0, .AnalyzeExpression_label_404

	// *** Basic block 6

	li          t0, 89		// 0x59 ASCII 'Y'
	blt         t0, s5, .AnalyzeExpression_label_404

	// *** Basic block 7

	addi        t0, s5, -1
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 8

	j           .AnalyzeExpression_label_170

	// *** Basic block 9

	j           .AnalyzeExpression_label_178

	// *** Basic block 10

	j           .AnalyzeExpression_label_174

	// *** Basic block 11

	j           .AnalyzeExpression_label_175

	// *** Basic block 12

	j           .AnalyzeExpression_label_171

	// *** Basic block 13

	j           .AnalyzeExpression_label_173

	// *** Basic block 14

	j           .AnalyzeExpression_label_270

	// *** Basic block 15

	j           .AnalyzeExpression_label_271

	// *** Basic block 16

	j           .AnalyzeExpression_label_277

	// *** Basic block 17

	j           .AnalyzeExpression_label_276

	// *** Basic block 18

	j           .AnalyzeExpression_label_307

	// *** Basic block 19

	j           .AnalyzeExpression_label_302

	// *** Basic block 20

	j           .AnalyzeExpression_label_404

	// *** Basic block 21

	j           .AnalyzeExpression_label_404

	// *** Basic block 22

	j           .AnalyzeExpression_label_404

	// *** Basic block 23

	j           .AnalyzeExpression_label_404

	// *** Basic block 24

	j           .AnalyzeExpression_label_231

	// *** Basic block 25

	j           .AnalyzeExpression_label_261

	// *** Basic block 26

	j           .AnalyzeExpression_label_324

	// *** Basic block 27

	j           .AnalyzeExpression_label_253

	// *** Basic block 28

	j           .AnalyzeExpression_label_292

	// *** Basic block 29

	j           .AnalyzeExpression_label_232

	// *** Basic block 30

	j           .AnalyzeExpression_label_404

	// *** Basic block 31

	j           .AnalyzeExpression_label_233

	// *** Basic block 32

	j           .AnalyzeExpression_label_263

	// *** Basic block 33

	j           .AnalyzeExpression_label_404

	// *** Basic block 34

	j           .AnalyzeExpression_label_172

	// *** Basic block 35

	j           .AnalyzeExpression_label_404

	// *** Basic block 36

	j           .AnalyzeExpression_label_329

	// *** Basic block 37

	j           .AnalyzeExpression_label_404

	// *** Basic block 38

	j           .AnalyzeExpression_label_404

	// *** Basic block 39

	j           .AnalyzeExpression_label_404

	// *** Basic block 40

	j           .AnalyzeExpression_label_323

	// *** Basic block 41

	j           .AnalyzeExpression_label_242

	// *** Basic block 42

	j           .AnalyzeExpression_label_404

	// *** Basic block 43

	j           .AnalyzeExpression_label_404

	// *** Basic block 44

	j           .AnalyzeExpression_label_240

	// *** Basic block 45

	j           .AnalyzeExpression_label_241

	// *** Basic block 46

	j           .AnalyzeExpression_label_404

	// *** Basic block 47

	j           .AnalyzeExpression_label_404

	// *** Basic block 48

	j           .AnalyzeExpression_label_404

	// *** Basic block 49

	j           .AnalyzeExpression_label_238

	// *** Basic block 50

	j           .AnalyzeExpression_label_239

	// *** Basic block 51

	j           .AnalyzeExpression_label_350

	// *** Basic block 52

	j           .AnalyzeExpression_label_351

	// *** Basic block 53

	j           .AnalyzeExpression_label_317

	// *** Basic block 54

	j           .AnalyzeExpression_label_404

	// *** Basic block 55

	j           .AnalyzeExpression_label_225

	// *** Basic block 56

	j           .AnalyzeExpression_label_259

	// *** Basic block 57

	j           .AnalyzeExpression_label_312

	// *** Basic block 58

	j           .AnalyzeExpression_label_189

	// *** Basic block 59

	j           .AnalyzeExpression_label_255

	// *** Basic block 60

	j           .AnalyzeExpression_label_269

	// *** Basic block 61

	j           .AnalyzeExpression_label_243

	// *** Basic block 62

	j           .AnalyzeExpression_label_262

	// *** Basic block 63

	j           .AnalyzeExpression_label_204

	// *** Basic block 64

	j           .AnalyzeExpression_label_258

	// *** Basic block 65

	j           .AnalyzeExpression_label_184

	// *** Basic block 66

	j           .AnalyzeExpression_label_254

	// *** Basic block 67

	j           .AnalyzeExpression_label_268

	// *** Basic block 68

	j           .AnalyzeExpression_label_248

	// *** Basic block 69

	j           .AnalyzeExpression_label_404

	// *** Basic block 70

	j           .AnalyzeExpression_label_226

	// *** Basic block 71

	j           .AnalyzeExpression_label_260

	// *** Basic block 72

	j           .AnalyzeExpression_label_282

	// *** Basic block 73

	j           .AnalyzeExpression_label_196

	// *** Basic block 74

	j           .AnalyzeExpression_label_257

	// *** Basic block 75

	j           .AnalyzeExpression_label_195

	// *** Basic block 76

	j           .AnalyzeExpression_label_256

	// *** Basic block 77

	j           .AnalyzeExpression_label_404

	// *** Basic block 78

	j           .AnalyzeExpression_label_297

	// *** Basic block 79

	j           .AnalyzeExpression_label_404

	// *** Basic block 80

	j           .AnalyzeExpression_label_404

	// *** Basic block 81

	j           .AnalyzeExpression_label_404

	// *** Basic block 82

	j           .AnalyzeExpression_label_404

	// *** Basic block 83

	j           .AnalyzeExpression_label_392

	// *** Basic block 84

	j           .AnalyzeExpression_label_399

	// *** Basic block 85

	j           .AnalyzeExpression_label_393

	// *** Basic block 86

	j           .AnalyzeExpression_label_394

	// *** Basic block 87

	j           .AnalyzeExpression_label_287

	// *** Basic block 88

	j           .AnalyzeExpression_label_404

	// *** Basic block 89

	j           .AnalyzeExpression_label_404

	// *** Basic block 90

	j           .AnalyzeExpression_label_404

	// *** Basic block 91

	j           .AnalyzeExpression_label_176

	// *** Basic block 92

	j           .AnalyzeExpression_label_404

	// *** Basic block 93

	j           .AnalyzeExpression_label_356

	// *** Basic block 94

	j           .AnalyzeExpression_label_370

	// *** Basic block 95

	j           .AnalyzeExpression_label_387

	// *** Basic block 96

	j           .AnalyzeExpression_label_388

	// *** Basic block 97

.AnalyzeExpression_label_170:

	// *** Basic block 98

.AnalyzeExpression_label_171:

	// *** Basic block 99

.AnalyzeExpression_label_172:

	// *** Basic block 100

.AnalyzeExpression_label_173:

	// *** Basic block 101

.AnalyzeExpression_label_174:

	// *** Basic block 102

.AnalyzeExpression_label_175:

	// *** Basic block 103

.AnalyzeExpression_label_176:
	j           .AnalyzeExpression_label_406

	// *** Basic block 104

.AnalyzeExpression_label_178:
	mv          a0, s1
	call        AnalyzeIdentifier

	// *** Basic block 105

	mv          s1, a0
	j           .AnalyzeExpression_label_406

	// *** Basic block 106

.AnalyzeExpression_label_184:
	mv          a0, s2
	call        AnalyzePlusOperator

	// *** Basic block 107

	j           .AnalyzeExpression_label_406

	// *** Basic block 108

.AnalyzeExpression_label_189:
	mv          a0, s2
	call        AnalyzeMinusOperator

	// *** Basic block 109

	mv          s1, a0
	j           .AnalyzeExpression_label_406

	// *** Basic block 110

.AnalyzeExpression_label_195:

	// *** Basic block 111

.AnalyzeExpression_label_196:
	mv          a0, s2
	call        AnalyzeBinaryExpression

	// *** Basic block 112

	mv          a0, s2
	call        InsertNumericConversions

	// *** Basic block 113

	j           .AnalyzeExpression_label_406

	// *** Basic block 114

.AnalyzeExpression_label_204:
	mv          a0, s2
	call        AnalyzeBinaryExpression

	// *** Basic block 115

	ld          t0, 56(s2)
	ld          a0, 16(t0)
	call        TypeIsIntegral

	// *** Basic block 116

	not         t0, a0
	beqz        t0, .AnalyzeExpression_label_223

	// *** Basic block 117

	lla         a1, .str.32
	mv          a0, s1
	call        SemanticError

	// *** Basic block 118

.AnalyzeExpression_label_223:
	j           .AnalyzeExpression_label_406

	// *** Basic block 119

.AnalyzeExpression_label_225:

	// *** Basic block 120

.AnalyzeExpression_label_226:
	mv          a0, s2
	call        AnalyzeShift

	// *** Basic block 121

	j           .AnalyzeExpression_label_406

	// *** Basic block 122

.AnalyzeExpression_label_231:

	// *** Basic block 123

.AnalyzeExpression_label_232:

	// *** Basic block 124

.AnalyzeExpression_label_233:
	mv          a0, s2
	call        AnalyzeBitwiseOperator

	// *** Basic block 125

	j           .AnalyzeExpression_label_406

	// *** Basic block 126

.AnalyzeExpression_label_238:

	// *** Basic block 127

.AnalyzeExpression_label_239:

	// *** Basic block 128

.AnalyzeExpression_label_240:

	// *** Basic block 129

.AnalyzeExpression_label_241:

	// *** Basic block 130

.AnalyzeExpression_label_242:

	// *** Basic block 131

.AnalyzeExpression_label_243:
	mv          a0, s2
	call        AnalyzeComparisonOperator

	// *** Basic block 132

	j           .AnalyzeExpression_label_406

	// *** Basic block 133

.AnalyzeExpression_label_248:
	mv          a0, s2
	call        AnalyzeConditionalExpression

	// *** Basic block 134

	j           .AnalyzeExpression_label_406

	// *** Basic block 135

.AnalyzeExpression_label_253:

	// *** Basic block 136

.AnalyzeExpression_label_254:

	// *** Basic block 137

.AnalyzeExpression_label_255:

	// *** Basic block 138

.AnalyzeExpression_label_256:

	// *** Basic block 139

.AnalyzeExpression_label_257:

	// *** Basic block 140

.AnalyzeExpression_label_258:

	// *** Basic block 141

.AnalyzeExpression_label_259:

	// *** Basic block 142

.AnalyzeExpression_label_260:

	// *** Basic block 143

.AnalyzeExpression_label_261:

	// *** Basic block 144

.AnalyzeExpression_label_262:

	// *** Basic block 145

.AnalyzeExpression_label_263:
	mv          a0, s2
	call        AnalyzeAssignmentExpression

	// *** Basic block 146

	j           .AnalyzeExpression_label_406

	// *** Basic block 147

.AnalyzeExpression_label_268:

	// *** Basic block 148

.AnalyzeExpression_label_269:

	// *** Basic block 149

.AnalyzeExpression_label_270:

	// *** Basic block 150

.AnalyzeExpression_label_271:
	mv          a0, s3
	call        AnalyzeIncDec

	// *** Basic block 151

	j           .AnalyzeExpression_label_406

	// *** Basic block 152

.AnalyzeExpression_label_276:

	// *** Basic block 153

.AnalyzeExpression_label_277:
	mv          a0, s3
	call        AnalyzeUnaryExpression

	// *** Basic block 154

	j           .AnalyzeExpression_label_406

	// *** Basic block 155

.AnalyzeExpression_label_282:
	mv          a0, s1
	call        AnalyzeSizeofExpression

	// *** Basic block 156

	j           .AnalyzeExpression_label_406

	// *** Basic block 157

.AnalyzeExpression_label_287:
	mv          a0, s1
	call        AnalyzeCastExpression

	// *** Basic block 158

	j           .AnalyzeExpression_label_406

	// *** Basic block 159

.AnalyzeExpression_label_292:
	mv          a0, s3
	call        AnalyzeUnaryExpression

	// *** Basic block 160

	j           .AnalyzeExpression_label_406

	// *** Basic block 161

.AnalyzeExpression_label_297:
	mv          a0, s3
	call        AnalyzeUnaryExpression

	// *** Basic block 162

	j           .AnalyzeExpression_label_406

	// *** Basic block 163

.AnalyzeExpression_label_302:
	mv          a0, s3
	call        AnalyzeAddressOperator

	// *** Basic block 164

	j           .AnalyzeExpression_label_406

	// *** Basic block 165

.AnalyzeExpression_label_307:
	mv          a0, s3
	call        AnalyzeContentsOperator

	// *** Basic block 166

	j           .AnalyzeExpression_label_406

	// *** Basic block 167

.AnalyzeExpression_label_312:
	mv          a0, s2
	call        AnalyzeArraySubscript

	// *** Basic block 168

	j           .AnalyzeExpression_label_406

	// *** Basic block 169

.AnalyzeExpression_label_317:
	mv          a0, s4
	call        AnalyzeFunctionCall

	// *** Basic block 170

	mv          s1, a0
	j           .AnalyzeExpression_label_406

	// *** Basic block 171

.AnalyzeExpression_label_323:

	// *** Basic block 172

.AnalyzeExpression_label_324:
	mv          a0, s2
	call        AnalyzeMemberReference

	// *** Basic block 173

	j           .AnalyzeExpression_label_406

	// *** Basic block 174

.AnalyzeExpression_label_329:
	ld          a0, 56(s2)
	call        AnalyzeExpression

	// *** Basic block 175

	sd          a0, 56(s2)
	ld          a0, 64(s2)
	call        AnalyzeExpression

	// *** Basic block 176

	sd          a0, 64(s2)
	ld          t0, 64(s2)
	ld          a1, 16(t0)
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 177

	j           .AnalyzeExpression_label_406

	// *** Basic block 178

.AnalyzeExpression_label_350:

	// *** Basic block 179

.AnalyzeExpression_label_351:
	mv          a0, s2
	call        AnalyzeLogicalOperator

	// *** Basic block 180

	j           .AnalyzeExpression_label_406

	// *** Basic block 181

.AnalyzeExpression_label_356:
	ld          a1, 56(s2)
	ld          a2, 64(s2)
	mv          a0, s1
	call        AnalyzeInitialization

	// *** Basic block 182

	sd          a0, 64(s2)
	j           .AnalyzeExpression_label_406

	// *** Basic block 183

.AnalyzeExpression_label_370:
	mv          s5, s1
	ld          a0, 56(s5)
	call        AnalyzeExpression

	// *** Basic block 184

	sd          a0, 56(s5)
	ld          t0, 56(s5)
	ld          a1, 16(t0)
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 185

	j           .AnalyzeExpression_label_406

	// *** Basic block 186

.AnalyzeExpression_label_387:

	// *** Basic block 187

.AnalyzeExpression_label_388:
	mv          a0, s1
	j           .AnalyzeExpression_label_56

	// *** Basic block 188

.AnalyzeExpression_label_392:

	// *** Basic block 189

.AnalyzeExpression_label_393:

	// *** Basic block 190

.AnalyzeExpression_label_394:
	mv          a0, s4
	call        AnalyzeVarargsBuiltin1

	// *** Basic block 191

	j           .AnalyzeExpression_label_406

	// *** Basic block 192

.AnalyzeExpression_label_399:
	mv          a0, s4
	call        AnalyzeVarargsBuiltin2

	// *** Basic block 193

	j           .AnalyzeExpression_label_406

	// *** Basic block 194

.AnalyzeExpression_label_404:
	j           .AnalyzeExpression_label_406

	// *** Basic block 195

.AnalyzeExpression_label_406:
	mv          a0, s1
	call        FoldConstantExpression

	// *** Basic block 196

	mv          s6, a0
	beq         s6, x0, .AnalyzeExpression_label_418

	// *** Basic block 197

	mv          a0, s6
	j           .AnalyzeExpression_label_56

	// *** Basic block 198

.AnalyzeExpression_label_418:
	lw          t0, 8(s1)
	ori         t0, t0, 8
	sw          t0, 8(s1)
	ld          t0, 16(s1)
	bne         t0, x0, .AnalyzeExpression_label_439

	// *** Basic block 199

	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 200

	mv          a1, a0
	mv          a0, s1
	call        ASTNodeSetType

	// *** Basic block 201

.AnalyzeExpression_label_439:
	mv          a0, s1
	j           .AnalyzeExpression_label_56
.func_end_AnalyzeExpression:
	.size AnalyzeExpression, .func_end_AnalyzeExpression-AnalyzeExpression

	.global IsConstantExpression
	.type IsConstantExpression, @function

IsConstantExpression:

	// *** Basic block 0

	.global AnalyzeExpression
	.global TypeIsIntegral
	.global TypeIsFloatingPoint
	.global StorageIs
	.global IsConstantExpression
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
	call        AnalyzeExpression

	// *** Basic block 1

	mv          s1, a0
	lw          s2, 0(s1)
	li          t0, 1		// 0x1 ASCII \x1
	beq         s2, t0, .IsConstantExpression_label_80

	// *** Basic block 2

	li          s3, 2		// 0x2 ASCII \x2
	beq         s2, s3, .IsConstantExpression_label_91

	// *** Basic block 3

	li          t0, 3		// 0x3 ASCII \x3
	beq         s2, t0, .IsConstantExpression_label_84

	// *** Basic block 4

	li          t0, 4		// 0x4 ASCII \x4
	beq         s2, t0, .IsConstantExpression_label_85

	// *** Basic block 5

	li          t0, 5		// 0x5 ASCII \x5
	beq         s2, t0, .IsConstantExpression_label_81

	// *** Basic block 6

	li          t0, 6		// 0x6 ASCII \x6
	beq         s2, t0, .IsConstantExpression_label_83

	// *** Basic block 7

	li          t0, 12		// 0xc ASCII \xc
	beq         s2, t0, .IsConstantExpression_label_187

	// *** Basic block 8

	li          t0, 27		// 0x1b ASCII \x1b
	beq         s2, t0, .IsConstantExpression_label_82

	// *** Basic block 9

	li          t0, 80		// 0x50 ASCII 'P'
	beq         s2, t0, .IsConstantExpression_label_216

	// *** Basic block 10

.IsConstantExpression_label_76:
	mv          a0, x0
	j           .IsConstantExpression_label_88

	// *** Basic block 11

.IsConstantExpression_label_80:

	// *** Basic block 12

.IsConstantExpression_label_81:

	// *** Basic block 13

.IsConstantExpression_label_82:

	// *** Basic block 14

.IsConstantExpression_label_83:

	// *** Basic block 15

.IsConstantExpression_label_84:

	// *** Basic block 16

.IsConstantExpression_label_85:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 17

.IsConstantExpression_label_88:
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

.IsConstantExpression_label_91:
	mv          s3, s1
	ld          s6, 16(s3)
	mv          a0, s6
	call        TypeIsIntegral

	// *** Basic block 19

	beqz        a0, .IsConstantExpression_label_108

	// *** Basic block 20

	lw          t0, 12(s6)
	andi        t0, t0, 4
	snez        s7, t0

	// *** Basic block 21

.IsConstantExpression_label_108:

	// *** Basic block 22

.IsConstantExpression_label_110:
	mv          s5, s7
	bnez        s7, .IsConstantExpression_label_132

	// *** Basic block 23

	j           .IsConstantExpression_label_114

	// *** Basic block 24

.IsConstantExpression_label_114:
	mv          s7, s6
	mv          a0, s7
	call        TypeIsFloatingPoint

	// *** Basic block 25

	beqz        a0, .IsConstantExpression_label_127

	// *** Basic block 26

	lw          t0, 12(s7)
	andi        t0, t0, 4
	snez        s8, t0

	// *** Basic block 27

.IsConstantExpression_label_127:

	// *** Basic block 28

.IsConstantExpression_label_129:
	mv          s5, s8
	j           .IsConstantExpression_label_132

	// *** Basic block 29

.IsConstantExpression_label_132:
	beqz        s5, .IsConstantExpression_label_137

	// *** Basic block 30

	li          a0, 1		// 0x1 ASCII \x1
	j           .IsConstantExpression_label_88

	// *** Basic block 31

.IsConstantExpression_label_137:
	mv          s8, s6
	lw          t0, 16(s8)
	addi        t0, t0, -3
	seqz        s9, t0

	// *** Basic block 32

.IsConstantExpression_label_145:
	beqz        s9, .IsConstantExpression_label_152

	// *** Basic block 33

	j           .IsConstantExpression_label_148

	// *** Basic block 34

.IsConstantExpression_label_148:
	li          a0, 1		// 0x1 ASCII \x1
	j           .IsConstantExpression_label_88

	// *** Basic block 35

.IsConstantExpression_label_152:
	mv          s8, s6
	lw          t0, 16(s8)
	addi        t0, t0, -2
	seqz        s6, t0

	// *** Basic block 36

.IsConstantExpression_label_160:
	not         t0, s6
	beqz        t0, .IsConstantExpression_label_168

	// *** Basic block 37

	j           .IsConstantExpression_label_164

	// *** Basic block 38

.IsConstantExpression_label_164:
	mv          a0, x0
	j           .IsConstantExpression_label_88

	// *** Basic block 39

.IsConstantExpression_label_168:
	ld          t0, 56(s3)
	lw          a0, 48(t0)
	li          t0, 10		// 0xa ASCII \xa
	mv          a1, t0
	call        StorageIs

	// *** Basic block 40

	beqz        a0, .IsConstantExpression_label_183

	// *** Basic block 41

	li          a0, 1		// 0x1 ASCII \x1
	j           .IsConstantExpression_label_88

	// *** Basic block 42

.IsConstantExpression_label_183:
	mv          a0, x0
	j           .IsConstantExpression_label_88

	// *** Basic block 43

.IsConstantExpression_label_187:
	mv          s4, s1
	ld          s5, 56(s4)
	lw          t0, 0(s5)
	bne         t0, s3, .IsConstantExpression_label_212

	// *** Basic block 44

	ld          t0, 56(s5)
	lw          a0, 48(t0)
	li          t0, 10		// 0xa ASCII \xa
	mv          a1, t0
	call        StorageIs

	// *** Basic block 45

	beqz        a0, .IsConstantExpression_label_211

	// *** Basic block 46

	li          a0, 1		// 0x1 ASCII \x1
	j           .IsConstantExpression_label_88

	// *** Basic block 47

.IsConstantExpression_label_211:

	// *** Basic block 48

.IsConstantExpression_label_212:
	mv          a0, x0
	j           .IsConstantExpression_label_88

	// *** Basic block 49

.IsConstantExpression_label_216:
	mv          s2, s1
	ld          a0, 64(s2)
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
	j           IsConstantExpression
.func_end_IsConstantExpression:
	.size IsConstantExpression, .func_end_IsConstantExpression-IsConstantExpression

.PCend:
	.data
type_ranks:
	.type   type_ranks,@object
	.global type_ranks
	.size   type_ranks,88
	.p2align  3
	.global TypeIsBool
	.long    TypeIsBool
	.global TypeIsChar
	.long    TypeIsChar
	.global TypeIsShort
	.long    TypeIsShort
	.global TypeIsInt
	.long    TypeIsInt
	.global TypeIsLong
	.long    TypeIsLong
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.global TypeIsFloat
	.long    TypeIsFloat
	.global TypeIsDouble
	.long    TypeIsDouble
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.global TypeIsVoid
	.long    TypeIsVoid
	.word   0
	.space  4

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "Illegal binary operand types \'%s\' and \'%s\'"
	.type .str.1, @object
	.size .str.1, 43

.str.2:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.2, @object
	.size .str.2, 30

.str.3:
	.asciz "(null)"
	.type .str.3, @object
	.size .str.3, 1

.str.4:
	.asciz "left_rank != -1 && right_rank != -1"
	.type .str.4, @object
	.size .str.4, 36

.str.5:
	.asciz "Can only add an integer to a pointer"
	.type .str.5, @object
	.size .str.5, 37

.str.6:
	.asciz "Can only add an integer to a pointer"
	.type .str.6, @object
	.size .str.6, 37

.str.7:
	.asciz "Illegal pointer subtraction; pointers are not the same type: \'%s\' and \'%s\'"
	.type .str.7, @object
	.size .str.7, 75

.str.8:
	.asciz "Illegal pointer subtraction operation"
	.type .str.8, @object
	.size .str.8, 38

.str.9:
	.asciz "Shift operator needs integral types"
	.type .str.9, @object
	.size .str.9, 36

.str.10:
	.asciz "Bitwise operator needs integral types"
	.type .str.10, @object
	.size .str.10, 38

.str.11:
	.asciz "Condition for ? operator must be scalar"
	.type .str.11, @object
	.size .str.11, 40

.str.12:
	.asciz "Cannot initialize a variable of this type"
	.type .str.12, @object
	.size .str.12, 42

.str.13:
	.asciz "Cannot assign to this expression"
	.type .str.13, @object
	.size .str.13, 33

.str.14:
	.asciz "Cannot add or subtract non-integers from pointers"
	.type .str.14, @object
	.size .str.14, 50

.str.15:
	.asciz "Integer type expected"
	.type .str.15, @object
	.size .str.15, 22

.str.16:
	.asciz "Integer type expected"
	.type .str.16, @object
	.size .str.16, 22

.str.17:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.17, @object
	.size .str.17, 30

.str.18:
	.asciz "(null)"
	.type .str.18, @object
	.size .str.18, 1

.str.19:
	.asciz "false"
	.type .str.19, @object
	.size .str.19, 6

.str.20:
	.asciz "Cannot increment or decrement this value"
	.type .str.20, @object
	.size .str.20, 41

.str.21:
	.asciz "Subscripts must be integral types"
	.type .str.21, @object
	.size .str.21, 34

.str.22:
	.asciz "Can only subscript arrays and pointers"
	.type .str.22, @object
	.size .str.22, 39

.str.23:
	.asciz "Cannot call a non-function"
	.type .str.23, @object
	.size .str.23, 27

.str.24:
	.asciz "Too few arguments supplied to varargs function call; need %zd, got %zd"
	.type .str.24, @object
	.size .str.24, 71

.str.25:
	.asciz "Incorrect number of arguments supplied to function call; need %zd, got %zd"
	.type .str.25, @object
	.size .str.25, 75

.str.26:
	.asciz "Left of -> is not a pointer to a struct/union; did you mean to use \'.\'"
	.type .str.26, @object
	.size .str.26, 71

.str.27:
	.asciz "Left of \'.\' is a pointer; did you mean to use ->?"
	.type .str.27, @object
	.size .str.27, 50

.str.28:
	.asciz "Left of \'.\' is not a struct/union"
	.type .str.28, @object
	.size .str.28, 34

.str.29:
	.asciz "%s is not a member of struct/union %s"
	.type .str.29, @object
	.size .str.29, 38

.str.30:
	.asciz "Cannot take the address of this expression"
	.type .str.30, @object
	.size .str.30, 43

.str.31:
	.asciz "Cannot take contents of this expression"
	.type .str.31, @object
	.size .str.31, 40

.str.32:
	.asciz "Modulus operator needs an integral type"
	.type .str.32, @object
	.size .str.32, 40

