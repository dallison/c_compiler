	.file   "compiler.c"
	.text
	.option pic
.PCbegin:
	.global OptLevel0
	.type OptLevel0, @function

OptLevel0:

	// *** Basic block 0

	.global compiler
	// Leaf procedure, no stack frame generated
	la          t0, compiler
	ld          t0, 0(t0)
	lb          t1, 1229(t0)
	not         a0, t1
	bnez        a0, .OptLevel0_label_17

	// *** Basic block 1

	lw          t0, 1256(t0)
	seqz        a0, t0

	// *** Basic block 2

.OptLevel0_label_17:

	// *** Basic block 3

.OptLevel0_label_19:
	ret         
.func_end_OptLevel0:
	.size OptLevel0, .func_end_OptLevel0-OptLevel0

	.global OptLevel1
	.type OptLevel1, @function

OptLevel1:

	// *** Basic block 0

	.global compiler
	// Leaf procedure, no stack frame generated
	la          t0, compiler
	ld          t0, 0(t0)
	lb          a0, 1229(t0)
	beqz        a0, .OptLevel1_label_17

	// *** Basic block 1

	lw          t0, 1256(t0)
	slti        t0, t0, 1
	not         a0, t0

	// *** Basic block 2

.OptLevel1_label_17:

	// *** Basic block 3

.OptLevel1_label_19:
	ret         
.func_end_OptLevel1:
	.size OptLevel1, .func_end_OptLevel1-OptLevel1

	.global OptLevel2
	.type OptLevel2, @function

OptLevel2:

	// *** Basic block 0

	.global compiler
	// Leaf procedure, no stack frame generated
	la          t0, compiler
	ld          t0, 0(t0)
	lb          a0, 1229(t0)
	beqz        a0, .OptLevel2_label_17

	// *** Basic block 1

	lw          t0, 1256(t0)
	slti        t0, t0, 2
	not         a0, t0

	// *** Basic block 2

.OptLevel2_label_17:

	// *** Basic block 3

.OptLevel2_label_19:
	ret         
.func_end_OptLevel2:
	.size OptLevel2, .func_end_OptLevel2-OptLevel2

	.global OptLevel3
	.type OptLevel3, @function

OptLevel3:

	// *** Basic block 0

	.global compiler
	// Leaf procedure, no stack frame generated
	la          t0, compiler
	ld          t0, 0(t0)
	lb          a0, 1229(t0)
	beqz        a0, .OptLevel3_label_17

	// *** Basic block 1

	lw          t0, 1256(t0)
	slti        t0, t0, 3
	not         a0, t0

	// *** Basic block 2

.OptLevel3_label_17:

	// *** Basic block 3

.OptLevel3_label_19:
	ret         
.func_end_OptLevel3:
	.size OptLevel3, .func_end_OptLevel3-OptLevel3

	.global ParseOptions
	.type ParseOptions, @function

ParseOptions:

	// *** Basic block 0

	.global strchr
	.global NewString
	.global StringAppendSegment
	.global VectorAppend
	.global StringAppend
	.local compiler_options
	.global calloc
	.global StringInit
	.global StringEqual
	.global atoi
	.global VectorDestructWithContents
	.global StringDestruct
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -48(s0)
	// Spilled register region: 8 bytes at -56(s0) to -48(s0)
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
	mv          s3, a2
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -48(s0)
	li          s4, 1		// 0x1 ASCII \x1
	li          s5, 1		// 0x1 ASCII \x1
	bge         s5, s1, .ParseOptions_label_129

	// *** Basic block 1

	la          t0, compiler_options
	ld          s6, 0(t0)

	// *** Basic block 2

.ParseOptions_label_55:
	slli        t0, s4, 3
	add         t0, s2, t0
	ld          s8, 0(t0)
	lb          t0, 0(s8)
	li          t1, 45		// 0x2d ASCII '-'
	beq         t0, t1, .ParseOptions_label_69

	// *** Basic block 3

	mv          s7, x0
	j           .ParseOptions_label_77

	// *** Basic block 4

.ParseOptions_label_69:
	li          t0, 61		// 0x3d ASCII '='
	mv          a1, t0
	mv          a0, s8
	call        strchr

	// *** Basic block 5

	mv          s7, a0

	// *** Basic block 6

.ParseOptions_label_77:
	beq         s7, x0, .ParseOptions_label_114

	// *** Basic block 7

	mv          a0, x0
	call        NewString

	// *** Basic block 8

	mv          s2, a0
	sub         a2, s7, s8
	mv          a1, s8
	mv          a0, s2
	call        StringAppendSegment

	// *** Basic block 9

	addi        a0, s0, -48
	mv          a1, s2
	call        VectorAppend

	// *** Basic block 10

	mv          a0, x0
	call        NewString

	// *** Basic block 11

	mv          s2, a0
	addi        a1, s7, 1
	mv          a0, s2
	call        StringAppend

	// *** Basic block 12

	addi        a0, s0, -48
	mv          a1, s2
	call        VectorAppend

	// *** Basic block 13

	j           .ParseOptions_label_124

	// *** Basic block 14

.ParseOptions_label_114:
	addi        s9, s0, -48
	mv          a0, s8
	call        NewString

	// *** Basic block 15

	mv          a1, a0
	mv          a0, s9
	call        VectorAppend

	// *** Basic block 16

.ParseOptions_label_124:

	// *** Basic block 17

.ParseOptions_label_125:
	addi        s4, s4, 1
	bge         s4, s1, .ParseOptions_label_55

	// *** Basic block 18

.ParseOptions_label_129:
	mv          s4, x0
	sd          s4, -56(s0)	// Spilled @130
	addi        t0, s0, -48
	ld          s6, 8(t0)
	sd          s6, -56(s0)	// Spilled @134
	bge         x0, s6, .ParseOptions_label_313

	// *** Basic block 19

	ld          s7, -48(s0)

	// *** Basic block 20

.ParseOptions_label_138:
	slli        t0, s4, 3
	add         t0, s7, t0
	ld          s8, 0(t0)
	ld          s9, 16(s8)
	lb          t0, 0(s9)
	li          t1, 45		// 0x2d ASCII '-'
	bne         t0, t1, .ParseOptions_label_287

	// *** Basic block 21

	lb          s10, 1(s9)
	addi        s4, s4, 1
	addi        s4, s4, 1
	mv          s11, x0
	beq         s6, x0, .ParseOptions_label_285

	// *** Basic block 22

.ParseOptions_label_157:
	slli        t1, s11, 3
	slli        t2, s11, 4
	add         t1, t1, t2
	la          t2, compiler_options
	add         s6, t2, t1
	sd          s6, -56(s0)	// Spilled @163
	lb          t0, 16(s6)
	beqz        t0, .ParseOptions_label_173

	// *** Basic block 23

	ld          t1, 0(s6)
	lb          t1, 1(t1)
	sub         t1, s10, t1
	seqz        t0, t1

	// *** Basic block 24

.ParseOptions_label_173:
	beqz        t0, .ParseOptions_label_202

	// *** Basic block 25

	mv          a1, s5
	li          t0, 48		// 0x30 ASCII '0'
	mv          a0, t0
	call        calloc

	// *** Basic block 26

	mv          s10, a0
	lw          t0, 12(s6)
	sw          t0, 0(s10)
	mv          s6, s8
	addi        a0, s10, 8
	ld          t0, 16(s6)
	addi        a1, t0, 2
	call        StringInit

	// *** Basic block 27

	mv          a1, s10
	mv          a0, s3
	call        VectorAppend

	// *** Basic block 28

	j           .ParseOptions_label_285

	// *** Basic block 29

.ParseOptions_label_202:
	ld          t0, -56(s0)	// Spilled @163
	ld          a1, 0(t0)
	mv          a0, s8
	call        StringEqual

	// *** Basic block 30

	beqz        a0, .ParseOptions_label_272

	// *** Basic block 31

	mv          a1, s5
	li          t0, 48		// 0x30 ASCII '0'
	mv          a0, t0
	call        calloc

	// *** Basic block 32

	mv          s8, a0
	ld          t0, -56(s0)	// Spilled @163
	lw          t1, 12(t0)
	sw          t1, 0(s8)
	ld          t1, -56(s0)	// Spilled @163
	lw          t2, 8(t1)
	slli        t2, t2, 2
	auipc       t3, 0
	add         t2, t3, t2
	jalr        x0, t2, 12

	// *** Basic block 33

	j           .ParseOptions_label_230

	// *** Basic block 34

	j           .ParseOptions_label_250

	// *** Basic block 35

	j           .ParseOptions_label_245

	// *** Basic block 36

.ParseOptions_label_230:
	ld          t0, -56(s0)	// Spilled @134
	bge         s4, t0, .ParseOptions_label_243

	// *** Basic block 37

	slli        t0, s4, 3
	add         t0, s7, t0
	ld          s4, 0(t0)
	addi        a0, s8, 8
	ld          a1, 16(s4)
	call        StringInit

	// *** Basic block 38

.ParseOptions_label_243:
	j           .ParseOptions_label_265

	// *** Basic block 39

.ParseOptions_label_245:
	sb          s5, 8(s8)
	j           .ParseOptions_label_265

	// *** Basic block 40

.ParseOptions_label_250:
	ld          t0, -56(s0)	// Spilled @130
	ld          t1, -56(s0)	// Spilled @134
	bge         t0, t1, .ParseOptions_label_263

	// *** Basic block 41

	ld          t0, -56(s0)	// Spilled @130
	slli        t1, t0, 3
	add         t1, s7, t1
	ld          s4, 0(t1)
	ld          a0, 16(s4)
	call        atoi

	// *** Basic block 42

	sw          a0, 8(s8)

	// *** Basic block 43

.ParseOptions_label_263:
	j           .ParseOptions_label_265

	// *** Basic block 44

.ParseOptions_label_265:
	mv          a1, s8
	mv          a0, s3
	call        VectorAppend

	// *** Basic block 45

	j           .ParseOptions_label_285

	// *** Basic block 46

.ParseOptions_label_272:

	// *** Basic block 47

.ParseOptions_label_273:

	// *** Basic block 48

.ParseOptions_label_274:
	addi        s11, s11, 1
	slli        t0, s11, 3
	slli        t1, s11, 4
	add         t0, t0, t1
	la          t1, compiler_options
	add         t0, t1, t0
	ld          t0, 0(t0)
	beq         t0, x0, .ParseOptions_label_157

	// *** Basic block 49

.ParseOptions_label_285:
	j           .ParseOptions_label_308

	// *** Basic block 50

.ParseOptions_label_287:
	mv          a1, s5
	li          t0, 48		// 0x30 ASCII '0'
	mv          a0, t0
	call        calloc

	// *** Basic block 51

	mv          s4, a0
	sw          x0, 0(s4)
	addi        a0, s4, 8
	mv          a1, s9
	call        StringInit

	// *** Basic block 52

	mv          a1, s4
	mv          a0, s3
	call        VectorAppend

	// *** Basic block 53

.ParseOptions_label_308:

	// *** Basic block 54

.ParseOptions_label_309:
	ld          t0, -56(s0)	// Spilled @130
	addi        s4, t0, 1
	ld          t1, -56(s0)	// Spilled @130
	ld          t2, -56(s0)	// Spilled @134
	bge         t1, t2, .ParseOptions_label_138

	// *** Basic block 55

.ParseOptions_label_313:
	addi        a0, s0, -48
	la          t0, StringDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 56

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
.func_end_ParseOptions:
	.size ParseOptions, .func_end_ParseOptions-ParseOptions

	.local  OptionStringValue
	.type OptionStringValue, @function

OptionStringValue:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	ld          t2, 8(t0)
	bge         x0, t2, .OptionStringValue_label_40

	// *** Basic block 1

	ld          t3, 0(t0)

	// *** Basic block 2

.OptionStringValue_label_19:
	addi        t2, t2, -1
	slli        t0, t2, 3
	add         t0, t3, t0
	ld          t3, 0(t0)
	lw          t0, 0(t3)
	bne         t0, t1, .OptionStringValue_label_36

	// *** Basic block 3

	addi        a0, t3, 8

	// *** Basic block 4

.OptionStringValue_label_33:
	ret         

	// *** Basic block 5

.OptionStringValue_label_36:

	// *** Basic block 6

.OptionStringValue_label_37:
	bge         x0, t2, .OptionStringValue_label_19

	// *** Basic block 7

.OptionStringValue_label_40:
	mv          a0, x0
	ret         
.func_end_OptionStringValue:
	.size OptionStringValue, .func_end_OptionStringValue-OptionStringValue

	.local  OptionIntValue
	.type OptionIntValue, @function

OptionIntValue:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	mv          t2, a2
	ld          t3, 8(t0)
	bge         x0, t3, .OptionIntValue_label_44

	// *** Basic block 1

	ld          t4, 0(t0)

	// *** Basic block 2

.OptionIntValue_label_22:
	addi        t3, t3, -1
	slli        t0, t3, 3
	add         t0, t4, t0
	ld          t4, 0(t0)
	lw          t0, 0(t4)
	bne         t0, t1, .OptionIntValue_label_40

	// *** Basic block 3

	lw          a0, 8(t4)

	// *** Basic block 4

.OptionIntValue_label_37:
	ret         

	// *** Basic block 5

.OptionIntValue_label_40:

	// *** Basic block 6

.OptionIntValue_label_41:
	bge         x0, t3, .OptionIntValue_label_22

	// *** Basic block 7

.OptionIntValue_label_44:
	mv          a0, t2
	ret         
.func_end_OptionIntValue:
	.size OptionIntValue, .func_end_OptionIntValue-OptionIntValue

	.local  OptionBoolValue
	.type OptionBoolValue, @function

OptionBoolValue:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	mv          t2, a2
	ld          t3, 8(t0)
	bge         x0, t3, .OptionBoolValue_label_44

	// *** Basic block 1

	ld          t4, 0(t0)

	// *** Basic block 2

.OptionBoolValue_label_22:
	addi        t3, t3, -1
	slli        t0, t3, 3
	add         t0, t4, t0
	ld          t4, 0(t0)
	lw          t0, 0(t4)
	bne         t0, t1, .OptionBoolValue_label_40

	// *** Basic block 3

	lb          a0, 8(t4)

	// *** Basic block 4

.OptionBoolValue_label_37:
	ret         

	// *** Basic block 5

.OptionBoolValue_label_40:

	// *** Basic block 6

.OptionBoolValue_label_41:
	bge         x0, t3, .OptionBoolValue_label_22

	// *** Basic block 7

.OptionBoolValue_label_44:
	mv          a0, t2
	ret         
.func_end_OptionBoolValue:
	.size OptionBoolValue, .func_end_OptionBoolValue-OptionBoolValue

	.local  InitInteger
	.type InitInteger, @function

InitInteger:

	// *** Basic block 0

	.global EvaluateIntegerExpression
	.global TypeIsChar
	.global TypeIsBool
	.global TypeIsShort
	.global TypeIsInt
	.global TypeIsLong
	.global TypeIsLongLong
	.global printf
	.global abort
	.global VectorAppend
	.global SemanticError
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
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	mv          s4, a3
	sd          x0, -32(s0)
	ld          s5, 16(s1)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 1

	beqz        a0, .InitInteger_label_145

	// *** Basic block 2

	mv          a0, s5
	call        TypeIsChar

	// *** Basic block 3

	mv          s6, a0
	bnez        a0, .InitInteger_label_64

	// *** Basic block 4

	mv          a0, s5
	call        TypeIsBool

	// *** Basic block 5

	mv          s6, a0

	// *** Basic block 6

.InitInteger_label_64:
	beqz        s6, .InitInteger_label_73

	// *** Basic block 7

	sw          x0, 0(s2)
	ld          t0, -32(s0)
	andi        t0, t0, 255
	sb          t0, 8(s2)
	j           .InitInteger_label_136

	// *** Basic block 8

.InitInteger_label_73:
	mv          a0, s5
	call        TypeIsShort

	// *** Basic block 9

	beqz        a0, .InitInteger_label_86

	// *** Basic block 10

	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 0(s2)
	addi        t0, s2, 8
	ld          t1, -32(s0)
	li          t2, 65535		// 0xffff
	and         t1, t1, t2
	sh          t1, 0(t0)
	j           .InitInteger_label_135

	// *** Basic block 11

.InitInteger_label_86:
	mv          a0, s5
	call        TypeIsInt

	// *** Basic block 12

	beqz        a0, .InitInteger_label_99

	// *** Basic block 13

	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, 0(s2)
	ld          t0, -32(s0)
	li          t1, 4294967295		// 0xffffffff
	and         t0, t0, t1
	sw          t0, 8(s2)
	j           .InitInteger_label_134

	// *** Basic block 14

.InitInteger_label_99:
	mv          a0, s5
	call        TypeIsLong

	// *** Basic block 15

	mv          s7, a0
	bnez        a0, .InitInteger_label_110

	// *** Basic block 16

	mv          a0, s5
	call        TypeIsLongLong

	// *** Basic block 17

	mv          s7, a0

	// *** Basic block 18

.InitInteger_label_110:
	beqz        s7, .InitInteger_label_118

	// *** Basic block 19

	li          t0, 3		// 0x3 ASCII \x3
	sw          t0, 0(s2)
	ld          t0, -32(s0)
	sd          t0, 8(s2)
	j           .InitInteger_label_133

	// *** Basic block 20

.InitInteger_label_118:
	lla         a0, .str.25
	lla         a1, .str.26
	lla         a3, .str.27
	li          t0, 194		// 0xc2 ASCII \xc2
	mv          a2, t0
	call        printf

	// *** Basic block 21

	call        abort

	// *** Basic block 22

.InitInteger_label_133:

	// *** Basic block 23

.InitInteger_label_134:

	// *** Basic block 24

.InitInteger_label_135:

	// *** Basic block 25

.InitInteger_label_136:
	sw          s3, 4(s2)
	mv          a1, s2
	mv          a0, s4
	call        VectorAppend

	// *** Basic block 26

	j           .InitInteger_label_152

	// *** Basic block 27

.InitInteger_label_145:
	lla         a1, .str.28
	mv          a0, s1
	call        SemanticError

	// *** Basic block 28

.InitInteger_label_152:
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
.func_end_InitInteger:
	.size InitInteger, .func_end_InitInteger-InitInteger

	.local  InitFloatingPoint
	.type InitFloatingPoint, @function

InitFloatingPoint:

	// *** Basic block 0

	.global EvaluateFloatingPointExpression
	.global TypeIsFloat
	.global TypeIsDouble
	.global TypeIsLongDouble
	.global printf
	.global abort
	.global VectorAppend
	.global SemanticError
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
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	mv          s4, a3
	fmv.d.x     ft0, x0
	fsd         ft0, -32(s0)
	ld          s5, 16(s1)
	addi        a1, s0, -32
	call        EvaluateFloatingPointExpression

	// *** Basic block 1

	beqz        a0, .InitFloatingPoint_label_108

	// *** Basic block 2

	mv          a0, s5
	call        TypeIsFloat

	// *** Basic block 3

	beqz        a0, .InitFloatingPoint_label_63

	// *** Basic block 4

	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, 0(s2)
	fld         ft0, -32(s0)
	fcvt.s.d    ft0, ft0
	fsw         ft0, -24(s0)
	lw          t0, -24(s0)
	sw          t0, 8(s2)
	j           .InitFloatingPoint_label_99

	// *** Basic block 5

.InitFloatingPoint_label_63:
	mv          a0, s5
	call        TypeIsDouble

	// *** Basic block 6

	mv          s6, a0
	bnez        a0, .InitFloatingPoint_label_74

	// *** Basic block 7

	mv          a0, s5
	call        TypeIsLongDouble

	// *** Basic block 8

	mv          s6, a0

	// *** Basic block 9

.InitFloatingPoint_label_74:
	beqz        s6, .InitFloatingPoint_label_83

	// *** Basic block 10

	li          t0, 3		// 0x3 ASCII \x3
	sw          t0, 0(s2)
	lw          t0, -32(s0)
	sd          t0, 8(s2)
	j           .InitFloatingPoint_label_98

	// *** Basic block 11

.InitFloatingPoint_label_83:
	lla         a0, .str.29
	lla         a1, .str.30
	lla         a3, .str.31
	li          t0, 220		// 0xdc ASCII \xdc
	mv          a2, t0
	call        printf

	// *** Basic block 12

	call        abort

	// *** Basic block 13

.InitFloatingPoint_label_98:

	// *** Basic block 14

.InitFloatingPoint_label_99:
	sw          s3, 4(s2)
	mv          a1, s2
	mv          a0, s4
	call        VectorAppend

	// *** Basic block 15

	j           .InitFloatingPoint_label_115

	// *** Basic block 16

.InitFloatingPoint_label_108:
	lla         a1, .str.32
	mv          a0, s1
	call        SemanticError

	// *** Basic block 17

.InitFloatingPoint_label_115:
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
.func_end_InitFloatingPoint:
	.size InitFloatingPoint, .func_end_InitFloatingPoint-InitFloatingPoint

	.local  InitPointer
	.type InitPointer, @function

InitPointer:

	// *** Basic block 0

	.global CompilerAddStringLiteral
	.global VectorAppend
	.local InitInteger
	.local InitScalar
	.global SemanticError
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
	mv          s2, a2
	mv          s3, a3
	mv          s4, a4
	mv          s5, a1
	lw          s6, 0(s1)
	li          t0, 3		// 0x3 ASCII \x3
	bne         s6, t0, .InitPointer_label_64

	// *** Basic block 1

	mv          s7, s1
	ld          a0, 56(s7)
	call        CompilerAddStringLiteral

	// *** Basic block 2

	mv          s8, a0
	li          t0, 5		// 0x5 ASCII \x5
	sw          t0, 0(s2)
	sw          s8, 8(s2)
	sw          s3, 4(s2)
	mv          a1, s2
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
	j           VectorAppend

	// *** Basic block 3

.InitPointer_label_61:
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

.InitPointer_label_64:
	li          t0, 1		// 0x1 ASCII \x1
	bne         s6, t0, .InitPointer_label_80

	// *** Basic block 5

	mv          a3, s4
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        InitInteger

	// *** Basic block 6

	j           .InitPointer_label_61

	// *** Basic block 7

.InitPointer_label_80:
	li          t0, 12		// 0xc ASCII \xc
	bne         s6, t0, .InitPointer_label_92

	// *** Basic block 8

	mv          s6, s1
	ld          s9, 56(s6)
	j           .InitPointer_label_94

	// *** Basic block 9

.InitPointer_label_92:
	mv          s9, s1

	// *** Basic block 10

.InitPointer_label_94:
	lw          s10, 0(s9)
	li          t0, 80		// 0x50 ASCII 'P'
	bne         s10, t0, .InitPointer_label_114

	// *** Basic block 11

	mv          s11, s9
	ld          a0, 64(s11)
	mv          a3, s4
	mv          a2, s3
	mv          a1, s5
	call        InitScalar

	// *** Basic block 12

	j           .InitPointer_label_61

	// *** Basic block 13

.InitPointer_label_114:
	li          t0, 2		// 0x2 ASCII \x2
	bne         s10, t0, .InitPointer_label_136

	// *** Basic block 14

	mv          s10, s9
	li          t0, 4		// 0x4 ASCII \x4
	sw          t0, 0(s2)
	ld          t0, 56(s10)
	sd          t0, 8(s2)
	sw          s3, 4(s2)
	mv          a1, s2
	mv          a0, s4
	call        VectorAppend

	// *** Basic block 15

	j           .InitPointer_label_143

	// *** Basic block 16

.InitPointer_label_136:
	lla         a1, .str.33
	mv          a0, s9
	call        SemanticError

	// *** Basic block 17

.InitPointer_label_143:
	j           .InitPointer_label_61
.func_end_InitPointer:
	.size InitPointer, .func_end_InitPointer-InitPointer

	.local  InitScalar
	.type InitScalar, @function

InitScalar:

	// *** Basic block 0

	.global TypeRecordCalculateSize
	.global malloc
	.global TypeIsIntegral
	.local InitInteger
	.global TypeIsFloatingPoint
	.local InitFloatingPoint
	.global TypeIsStructOrUnion
	.global SemanticError
	.global TypeIsFunctionPointer
	.global VectorAppend
	.local InitPointer
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
	mv          s3, a2
	mv          s4, a3
	ld          s5, 16(s1)
	mv          a0, s5
	call        TypeRecordCalculateSize

	// *** Basic block 1

	li          t0, 32		// 0x20 ASCII ' '
	mv          a0, t0
	call        malloc

	// *** Basic block 2

	mv          s6, a0
	ld          s7, 16(s2)
	mv          a0, s7
	call        TypeIsIntegral

	// *** Basic block 3

	beqz        a0, .InitScalar_label_65

	// *** Basic block 4

	mv          a3, s4
	mv          a2, s3
	mv          a1, s6
	mv          a0, s1
	call        InitInteger

	// *** Basic block 5

	j           .InitScalar_label_169

	// *** Basic block 6

.InitScalar_label_65:
	mv          a0, s7
	call        TypeIsFloatingPoint

	// *** Basic block 7

	beqz        a0, .InitScalar_label_80

	// *** Basic block 8

	mv          a3, s4
	mv          a2, s3
	mv          a1, s6
	mv          a0, s1
	call        InitFloatingPoint

	// *** Basic block 9

	j           .InitScalar_label_168

	// *** Basic block 10

.InitScalar_label_80:
	mv          a0, s7
	call        TypeIsStructOrUnion

	// *** Basic block 11

	beqz        a0, .InitScalar_label_92

	// *** Basic block 12

	lla         a1, .str.34
	mv          a0, s2
	call        SemanticError

	// *** Basic block 13

	j           .InitScalar_label_167

	// *** Basic block 14

.InitScalar_label_92:
	mv          s8, s7
	lw          t0, 16(s8)
	addi        t0, t0, -3
	seqz        s9, t0

	// *** Basic block 15

.InitScalar_label_101:
	beqz        s9, .InitScalar_label_132

	// *** Basic block 16

	j           .InitScalar_label_104

	// *** Basic block 17

.InitScalar_label_104:
	mv          a0, s7
	call        TypeIsFunctionPointer

	// *** Basic block 18

	beqz        a0, .InitScalar_label_123

	// *** Basic block 19

	li          t0, 4		// 0x4 ASCII \x4
	sw          t0, 0(s6)
	ld          t0, 32(s5)
	sd          t0, 8(s6)
	sw          s3, 4(s6)
	mv          a1, s6
	mv          a0, s4
	call        VectorAppend

	// *** Basic block 20

	j           .InitScalar_label_130

	// *** Basic block 21

.InitScalar_label_123:
	lla         a1, .str.35
	mv          a0, s2
	call        SemanticError

	// *** Basic block 22

.InitScalar_label_130:
	j           .InitScalar_label_166

	// *** Basic block 23

.InitScalar_label_132:
	mv          s5, s7
	lw          t0, 16(s5)
	addi        t0, t0, -1
	seqz        s7, t0

	// *** Basic block 24

.InitScalar_label_141:
	beqz        s7, .InitScalar_label_158

	// *** Basic block 25

	j           .InitScalar_label_144

	// *** Basic block 26

.InitScalar_label_144:
	mv          a4, s4
	mv          a3, s3
	mv          a2, s6
	mv          a1, s2
	mv          a0, s1
	call        InitPointer

	// *** Basic block 27

	j           .InitScalar_label_165

	// *** Basic block 28

.InitScalar_label_158:
	lla         a1, .str.36
	mv          a0, s2
	call        SemanticError

	// *** Basic block 29

.InitScalar_label_165:

	// *** Basic block 30

.InitScalar_label_166:

	// *** Basic block 31

.InitScalar_label_167:

	// *** Basic block 32

.InitScalar_label_168:

	// *** Basic block 33

.InitScalar_label_169:
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
.func_end_InitScalar:
	.size InitScalar, .func_end_InitScalar-InitScalar

	.local  InitArray
	.type InitArray, @function

InitArray:

	// *** Basic block 0

	.global TypeRecordCalculateSize
	.global malloc
	.global BufferInit
	.global TypeIsInt
	.global BufferAppend
	.global BufferAddSpace
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
	mv          s2, a2
	mv          s3, a3
	ld          s4, 16(s1)
	mv          a0, s4
	call        TypeRecordCalculateSize

	// *** Basic block 1

	li          t0, 32		// 0x20 ASCII ' '
	mv          a0, t0
	call        malloc

	// *** Basic block 2

	mv          s5, a0
	mv          s6, s1
	li          t0, 6		// 0x6 ASCII \x6
	sw          t0, 0(s5)
	sw          s2, 4(s5)
	addi        a0, s5, 8
	call        BufferInit

	// *** Basic block 3

	lw          s7, 20(s4)
	mv          a0, s4
	call        TypeIsInt

	// *** Basic block 4

	beqz        a0, .InitArray_label_63

	// *** Basic block 5

	ld          t0, 56(s6)
	ld          s1, 24(t0)
	j           .InitArray_label_69

	// *** Basic block 6

.InitArray_label_63:
	ld          t0, 56(s6)
	ld          t0, 24(t0)
	addi        s1, t0, 1

	// *** Basic block 7

.InitArray_label_69:
	bge         s7, s1, .InitArray_label_73

	// *** Basic block 8

	mv          s1, s7

	// *** Basic block 9

.InitArray_label_73:
	addi        a0, s5, 8
	ld          t0, 56(s6)
	ld          a1, 16(t0)
	mv          a2, s1
	call        BufferAppend

	// *** Basic block 10

	sub         s7, s7, s1
	bge         x0, s7, .InitArray_label_93

	// *** Basic block 11

	addi        a0, s5, 8
	mv          a1, s7
	call        BufferAddSpace

	// *** Basic block 12

.InitArray_label_93:
	mv          a1, s5
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
	j           VectorAppend
.func_end_InitArray:
	.size InitArray, .func_end_InitArray-InitArray

	.local  ExpandBracedInitializer
	.type ExpandBracedInitializer, @function

ExpandBracedInitializer:

	// *** Basic block 0

	.global printf
	.global abort
	.local InitArray
	.local InitScalar
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
	mv          s2, a2
	mv          s3, x0
	ld          t0, 56(a0)
	ld          s4, 8(t0)
	bge         x0, s4, .ExpandBracedInitializer_label_163

	// *** Basic block 1

	ld          t0, 0(t0)

	// *** Basic block 2

.ExpandBracedInitializer_label_40:
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s5, 0(t0)
	lw          t0, 0(s5)
	li          t1, 89		// 0x59 ASCII 'Y'
	bne         t0, t1, .ExpandBracedInitializer_label_53

	// *** Basic block 3

	j           .ExpandBracedInitializer_label_69

	// *** Basic block 4

.ExpandBracedInitializer_label_53:
	lla         a0, .str.37
	lla         a1, .str.38
	lla         a3, .str.39
	li          t0, 369		// 0x171
	mv          a2, t0
	call        printf

	// *** Basic block 5

	call        abort

	// *** Basic block 6

.ExpandBracedInitializer_label_69:
	mv          s6, s5
	mv          s7, s1
	ld          s8, 56(s6)
	beq         s8, x0, .ExpandBracedInitializer_label_122

	// *** Basic block 7

	ld          t0, 0(s8)
	mv          s1, x0
	ld          t1, 8(s8)
	bge         x0, t1, .ExpandBracedInitializer_label_121

	// *** Basic block 8

.ExpandBracedInitializer_label_86:
	slli        t2, s1, 3
	add         t0, t0, t2
	ld          s8, 0(t0)
	lw          t0, 0(s8)
	slli        t0, t0, 2
	auipc       t2, 0
	add         t0, t2, t0
	jalr        x0, t0, 12

	// *** Basic block 9

	j           .ExpandBracedInitializer_label_99

	// *** Basic block 10

	j           .ExpandBracedInitializer_label_109

	// *** Basic block 11

.ExpandBracedInitializer_label_99:
	lw          t0, 16(s8)
	ld          t2, 8(s8)
	lw          t2, 20(t2)
	mul         t0, t0, t2
	add         s7, s7, t0
	j           .ExpandBracedInitializer_label_116

	// *** Basic block 12

.ExpandBracedInitializer_label_109:
	ld          t0, 16(s8)
	lw          t0, 8(t0)
	add         s7, s7, t0
	j           .ExpandBracedInitializer_label_116

	// *** Basic block 13

.ExpandBracedInitializer_label_116:

	// *** Basic block 14

.ExpandBracedInitializer_label_117:
	addi        s1, s1, 1
	bge         s1, t1, .ExpandBracedInitializer_label_86

	// *** Basic block 15

.ExpandBracedInitializer_label_121:

	// *** Basic block 16

.ExpandBracedInitializer_label_122:
	ld          s8, 16(s6)
	lw          t0, 16(s8)
	addi        t0, t0, -2
	seqz        s8, t0

	// *** Basic block 17

.ExpandBracedInitializer_label_132:
	beqz        s8, .ExpandBracedInitializer_label_147

	// *** Basic block 18

	j           .ExpandBracedInitializer_label_135

	// *** Basic block 19

.ExpandBracedInitializer_label_135:
	ld          a0, 64(s6)
	mv          a3, s2
	mv          a2, s7
	mv          a1, s5
	call        InitArray

	// *** Basic block 20

	j           .ExpandBracedInitializer_label_158

	// *** Basic block 21

.ExpandBracedInitializer_label_147:
	ld          a0, 64(s6)
	mv          a3, s2
	mv          a2, s7
	mv          a1, s5
	call        InitScalar

	// *** Basic block 22

.ExpandBracedInitializer_label_158:

	// *** Basic block 23

.ExpandBracedInitializer_label_159:
	addi        s3, s3, 1
	bge         s3, s4, .ExpandBracedInitializer_label_40

	// *** Basic block 24

.ExpandBracedInitializer_label_163:
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
.func_end_ExpandBracedInitializer:
	.size ExpandBracedInitializer, .func_end_ExpandBracedInitializer-ExpandBracedInitializer

	.local  AddInitializedStaticVariable
	.type AddInitializedStaticVariable, @function

AddInitializedStaticVariable:

	// *** Basic block 0

	.global malloc
	.global StringInit
	.global StorageIs
	.global VectorInit
	.global TypeRecordAlignment
	.local ExpandBracedInitializer
	.global VectorAppend
	.global compiler
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
	li          a0, 96		// 0x60 ASCII '`'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 56(s1)
	ld          a1, 16(s4)
	mv          a0, s3
	call        StringInit

	// *** Basic block 2

	lw          s5, 48(s4)
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s5
	call        StorageIs

	// *** Basic block 3

	not         t0, a0
	sb          t0, 40(s3)
	addi        a0, s3, 64
	call        VectorInit

	// *** Basic block 4

	ld          s4, 40(s4)
	lw          t0, 20(s4)
	sd          t0, 48(s3)
	li          t0, 64		// 0x40 ASCII '@'
	mv          a1, t0
	mv          a0, s5
	call        StorageIs

	// *** Basic block 5

	sb          a0, 88(s3)
	mv          a0, s4
	call        TypeRecordAlignment

	// *** Basic block 6

	sw          a0, 56(s3)
	addi        a2, s3, 64
	mv          a1, x0
	mv          a0, s2
	call        ExpandBracedInitializer

	// *** Basic block 7

	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 1152
	mv          a1, s3
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorAppend
.func_end_AddInitializedStaticVariable:
	.size AddInitializedStaticVariable, .func_end_AddInitializedStaticVariable-AddInitializedStaticVariable

	.global InitializerDelete
	.type InitializerDelete, @function

InitializerDelete:

	// *** Basic block 0

	.global BufferDestruct
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
	lw          t0, 0(s1)
	li          t1, 6		// 0x6 ASCII \x6
	bne         t0, t1, .InitializerDelete_label_20

	// *** Basic block 1

	addi        a0, s1, 8
	call        BufferDestruct

	// *** Basic block 2

.InitializerDelete_label_20:
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_InitializerDelete:
	.size InitializerDelete, .func_end_InitializerDelete-InitializerDelete

	.global InitializedStaticVariableDelete
	.type InitializedStaticVariableDelete, @function

InitializedStaticVariableDelete:

	// *** Basic block 0

	.global StringDestruct
	.global InitializerDelete
	.global VectorDestruct
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
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	call        StringDestruct

	// *** Basic block 1

	mv          s2, x0
	addi        t0, s1, 64
	ld          s3, 8(t0)
	bge         x0, s3, .InitializedStaticVariableDelete_label_37

	// *** Basic block 2

	ld          s4, 64(s1)

	// *** Basic block 3

.InitializedStaticVariableDelete_label_26:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          a0, 0(t0)
	call        InitializerDelete

	// *** Basic block 4

.InitializedStaticVariableDelete_label_33:
	addi        s2, s2, 1
	bge         s2, s3, .InitializedStaticVariableDelete_label_26

	// *** Basic block 5

.InitializedStaticVariableDelete_label_37:
	addi        a0, s1, 64
	call        VectorDestruct

	// *** Basic block 6

	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_InitializedStaticVariableDelete:
	.size InitializedStaticVariableDelete, .func_end_InitializedStaticVariableDelete-InitializedStaticVariableDelete

	.global UninitializedStaticVariableDelete
	.type UninitializedStaticVariableDelete, @function

UninitializedStaticVariableDelete:

	// *** Basic block 0

	.global StringDestruct
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
	call        StringDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_UninitializedStaticVariableDelete:
	.size UninitializedStaticVariableDelete, .func_end_UninitializedStaticVariableDelete-UninitializedStaticVariableDelete

	.global CompilerAddStringLiteral
	.type CompilerAddStringLiteral, @function

CompilerAddStringLiteral:

	// *** Basic block 0

	.global malloc
	.global StringInit
	.global compiler
	.global VectorAppend
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
	li          a0, 56		// 0x38 ASCII '8'
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	addi        a0, s2, 8
	ld          a1, 16(s1)
	call        StringInit

	// *** Basic block 2

	la          t0, compiler
	ld          t0, 0(t0)
	lw          t1, 1224(t0)
	addi        t1, t1, 1
	sw          t1, 1224(t0)
	sw          t1, 0(s2)
	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 1200
	mv          a1, s2
	call        VectorAppend

	// *** Basic block 3

	sb          x0, 48(s2)
	lw          a0, 0(s2)

	// *** Basic block 4

.CompilerAddStringLiteral_label_49:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CompilerAddStringLiteral:
	.size CompilerAddStringLiteral, .func_end_CompilerAddStringLiteral-CompilerAddStringLiteral

	.global CompilerFindStringLiteral
	.type CompilerFindStringLiteral, @function

CompilerFindStringLiteral:

	// *** Basic block 0

	.global compiler
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, x0
	la          t2, compiler
	ld          t2, 0(t2)
	addi        t3, t2, 1200
	ld          t3, 8(t3)
	bge         x0, t3, .CompilerFindStringLiteral_label_42

	// *** Basic block 1

	ld          t2, 1200(t2)

	// *** Basic block 2

.CompilerFindStringLiteral_label_22:
	slli        t4, t1, 3
	add         t2, t2, t4
	ld          t4, 0(t2)
	lw          t2, 0(t4)
	bne         t2, t0, .CompilerFindStringLiteral_label_37

	// *** Basic block 3

	mv          a0, t4

	// *** Basic block 4

.CompilerFindStringLiteral_label_34:
	ret         

	// *** Basic block 5

.CompilerFindStringLiteral_label_37:

	// *** Basic block 6

.CompilerFindStringLiteral_label_38:
	addi        t1, t1, 1
	bge         t1, t3, .CompilerFindStringLiteral_label_22

	// *** Basic block 7

.CompilerFindStringLiteral_label_42:
	mv          a0, x0
	ret         
.func_end_CompilerFindStringLiteral:
	.size CompilerFindStringLiteral, .func_end_CompilerFindStringLiteral-CompilerFindStringLiteral

	.global StringLiteralDelete
	.type StringLiteralDelete, @function

StringLiteralDelete:

	// *** Basic block 0

	.global StringDestruct
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
	addi        a0, s1, 8
	call        StringDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_StringLiteralDelete:
	.size StringLiteralDelete, .func_end_StringLiteralDelete-StringLiteralDelete

	.local  AddLocalStatics
	.type AddLocalStatics, @function

AddLocalStatics:

	// *** Basic block 0

	.global malloc
	.global StringInit
	.global TypeRecordAlignment
	.global StorageIs
	.global VectorAppend
	.global compiler
	.local AddInitializedStaticVariable
	.global VectorClear
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
	mv          s2, x0
	addi        t0, s1, 104
	ld          s3, 8(t0)
	bge         x0, s3, .AddLocalStatics_label_103

	// *** Basic block 1

	ld          t0, 104(s1)

	// *** Basic block 2

.AddLocalStatics_label_35:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	ld          s5, 64(s4)
	bne         s5, x0, .AddLocalStatics_label_90

	// *** Basic block 3

	li          t0, 72		// 0x48 ASCII 'H'
	mv          a0, t0
	call        malloc

	// *** Basic block 4

	mv          s6, a0
	ld          s7, 56(s4)
	ld          a1, 16(s7)
	mv          a0, s6
	call        StringInit

	// *** Basic block 5

	sb          x0, 40(s6)
	ld          a0, 40(s7)
	lw          t0, 20(a0)
	sd          t0, 48(s6)
	call        TypeRecordAlignment

	// *** Basic block 6

	sd          a0, 56(s6)
	lw          a0, 48(s7)
	li          t0, 64		// 0x40 ASCII '@'
	mv          a1, t0
	call        StorageIs

	// *** Basic block 7

	sb          a0, 64(s6)
	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 1176
	mv          a1, s6
	call        VectorAppend

	// *** Basic block 8

	j           .AddLocalStatics_label_98

	// *** Basic block 9

.AddLocalStatics_label_90:
	ld          a1, 64(s5)
	mv          a0, s4
	call        AddInitializedStaticVariable

	// *** Basic block 10

.AddLocalStatics_label_98:

	// *** Basic block 11

.AddLocalStatics_label_99:
	addi        s2, s2, 1
	bge         s2, s3, .AddLocalStatics_label_35

	// *** Basic block 12

.AddLocalStatics_label_103:
	addi        a0, s1, 104
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
	j           VectorClear
.func_end_AddLocalStatics:
	.size AddLocalStatics, .func_end_AddLocalStatics-AddLocalStatics

	.local  IsFunctionOrInlineDefinition
	.type IsFunctionOrInlineDefinition, @function

IsFunctionOrInlineDefinition:

	// *** Basic block 0

	.global TypeIsFunctionDefinition
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
	ld          s2, 40(s1)
	mv          a0, s2
	call        TypeIsFunctionDefinition

	// *** Basic block 1

	bnez        a0, .IsFunctionOrInlineDefinition_label_41

	// *** Basic block 2

	lw          t0, 16(s2)
	addi        t0, t0, -3
	seqz        s2, t0

	// *** Basic block 3

.IsFunctionOrInlineDefinition_label_30:
	beqz        s2, .IsFunctionOrInlineDefinition_label_40

	// *** Basic block 4

	j           .IsFunctionOrInlineDefinition_label_34

	// *** Basic block 5

.IsFunctionOrInlineDefinition_label_34:
	addi        t0, s1, 56
	lb          t0, 1(t0)
	slli        t0, t0, 62
	srai        a0, t0, 63

	// *** Basic block 6

.IsFunctionOrInlineDefinition_label_40:

	// *** Basic block 7

.IsFunctionOrInlineDefinition_label_41:

	// *** Basic block 8

.IsFunctionOrInlineDefinition_label_43:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_IsFunctionOrInlineDefinition:
	.size IsFunctionOrInlineDefinition, .func_end_IsFunctionOrInlineDefinition-IsFunctionOrInlineDefinition

	.local  CompileDeclaration
	.type CompileDeclaration, @function

CompileDeclaration:

	// *** Basic block 0

	.global SyntaxParseExternalDeclaration
	.local IsFunctionOrInlineDefinition
	.global compiler
	.global SemanticAnalyzeFunction
	.global SymbolPrintDetails
	.global NumErrors
	.global BuildDebugInfo
	.global GeneratorInit
	.global GenerateFunction
	.global VectorAppend
	.global BuildDebugInfoAfterCodegen
	.local AddLocalStatics
	.global GeneratorDestruct
	.global StorageIs
	.global TypeIsVoid
	.global SemanticError
	.global malloc
	.global StringInit
	.global TypeRecordAlignment
	.global AnalyzeInitializer
	.global fprintf
	.global ASTNodePrint
	.local AddInitializedStaticVariable
	.global VariableDIESetStatic
	addi sp, sp, -320
	// Saved return address (offset 312) and frame pointer (offset 304)
	sd ra, 312(sp)
	sd s0, 304(sp)
	addi s0, sp, 320
	// Local vars at offset -224(s0)
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
	call        SyntaxParseExternalDeclaration

	// *** Basic block 1

	mv          s2, a0
	beq         s2, x0, .CompileDeclaration_label_413

	// *** Basic block 2

	lw          t0, 0(s2)
	li          t1, 83		// 0x53 ASCII 'S'
	bne         t0, t1, .CompileDeclaration_label_412

	// *** Basic block 3

	mv          s3, s2
	ld          t0, 56(s3)
	ld          s4, 8(t0)
	mv          s5, x0
	bge         x0, s4, .CompileDeclaration_label_411

	// *** Basic block 4

	ld          s2, 0(t0)

	// *** Basic block 5

.CompileDeclaration_label_88:
	slli        t0, s5, 3
	add         t0, s2, t0
	ld          s2, 0(t0)
	ld          s3, 56(s2)
	mv          a0, s3
	call        IsFunctionOrInlineDefinition

	// *** Basic block 6

	beqz        a0, .CompileDeclaration_label_205

	// *** Basic block 7

	la          t0, compiler
	ld          s6, 0(t0)
	ld          s7, 16(s2)
	sd          s7, 1096(s6)
	mv          a1, s2
	mv          a0, s1
	call        SemanticAnalyzeFunction

	// *** Basic block 8

	lb          t0, 1231(s6)
	beqz        t0, .CompileDeclaration_label_124

	// *** Basic block 9

	ld          a2, 1248(s6)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s3
	call        SymbolPrintDetails

	// *** Basic block 10

.CompileDeclaration_label_124:
	call        NumErrors

	// *** Basic block 11

	seqz        s8, a0
	bnez        a0, .CompileDeclaration_label_141

	// *** Basic block 12

	addi        t0, s7, 32
	lb          t0, 51(t0)
	not         s8, t0
	bnez        s8, .CompileDeclaration_label_140

	// *** Basic block 13

	addi        t0, s3, 56
	lb          t0, 1(t0)
	slli        t0, t0, 62
	srai        s8, t0, 63

	// *** Basic block 14

.CompileDeclaration_label_140:

	// *** Basic block 15

.CompileDeclaration_label_141:
	beqz        s8, .CompileDeclaration_label_203

	// *** Basic block 16

	lb          t0, 1228(s6)
	beqz        t0, .CompileDeclaration_label_160

	// *** Basic block 17

	ld          s6, 56(s2)
	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 608
	li          t0, 46		// 0x2e ASCII '.'
	mv          a2, t0
	mv          a1, s3
	call        BuildDebugInfo

	// *** Basic block 18

	sd          a0, 128(s6)

	// *** Basic block 19

.CompileDeclaration_label_160:
	addi        a0, s0, -224
	la          t0, compiler
	ld          t0, 0(t0)
	ld          a2, 1096(t0)
	mv          a1, s1
	call        GeneratorInit

	// *** Basic block 20

	addi        a0, s0, -224
	call        GenerateFunction

	// *** Basic block 21

	mv          s6, a0
	la          t0, compiler
	ld          s7, 0(t0)
	addi        a0, s7, 1128
	mv          a1, s6
	call        VectorAppend

	// *** Basic block 22

	lb          t0, 1228(s7)
	beqz        t0, .CompileDeclaration_label_196

	// *** Basic block 23

	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 608
	ld          a1, 56(s2)
	call        BuildDebugInfoAfterCodegen

	// *** Basic block 24

.CompileDeclaration_label_196:
	mv          a0, s1
	call        AddLocalStatics

	// *** Basic block 25

	addi        a0, s0, -224
	call        GeneratorDestruct

	// *** Basic block 26

.CompileDeclaration_label_203:
	j           .CompileDeclaration_label_406

	// *** Basic block 27

.CompileDeclaration_label_205:
	ld          s7, 16(s2)
	lw          t0, 16(s7)
	addi        t0, t0, -3
	seqz        s7, t0

	// *** Basic block 28

.CompileDeclaration_label_215:
	beqz        s7, .CompileDeclaration_label_220

	// *** Basic block 29

	j           .CompileDeclaration_label_218

	// *** Basic block 30

.CompileDeclaration_label_218:
	j           .CompileDeclaration_label_405

	// *** Basic block 31

.CompileDeclaration_label_220:
	lw          s8, 48(s3)
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	mv          a0, s8
	call        StorageIs

	// *** Basic block 32

	beqz        a0, .CompileDeclaration_label_258

	// *** Basic block 33

	la          t0, compiler
	ld          t0, 0(t0)
	lb          t0, 1228(t0)
	beqz        t0, .CompileDeclaration_label_256

	// *** Basic block 34

	ld          s9, 56(s2)
	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 608
	li          t0, 22		// 0x16 ASCII \x16
	mv          a2, t0
	mv          a1, s3
	call        BuildDebugInfo

	// *** Basic block 35

	sd          a0, 128(s9)
	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 608
	mv          a1, s9
	call        BuildDebugInfoAfterCodegen

	// *** Basic block 36

.CompileDeclaration_label_256:
	j           .CompileDeclaration_label_407

	// *** Basic block 37

.CompileDeclaration_label_258:
	ld          s9, 40(s3)
	mv          a0, s9
	call        TypeIsVoid

	// *** Basic block 38

	beqz        a0, .CompileDeclaration_label_271

	// *** Basic block 39

	lla         a1, .str.40
	mv          a0, s2
	call        SemanticError

	// *** Basic block 40

.CompileDeclaration_label_271:
	lb          s10, 56(s3)
	slli        t1, s10, 63
	srai        t0, t1, 63
	bnez        t0, .CompileDeclaration_label_280

	// *** Basic block 41

	slli        t1, s10, 62
	srai        t0, t1, 63

	// *** Basic block 42

.CompileDeclaration_label_280:
	beqz        t0, .CompileDeclaration_label_371

	// *** Basic block 43

	ld          s10, 64(s2)
	bne         s10, x0, .CompileDeclaration_label_333

	// *** Basic block 44

	li          t0, 72		// 0x48 ASCII 'H'
	mv          a0, t0
	call        malloc

	// *** Basic block 45

	mv          s11, a0
	ld          a1, 16(s3)
	mv          a0, s11
	call        StringInit

	// *** Basic block 46

	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s8
	call        StorageIs

	// *** Basic block 47

	not         t0, a0
	sb          t0, 40(s11)
	lw          t0, 20(s9)
	sd          t0, 48(s11)
	mv          a0, s9
	call        TypeRecordAlignment

	// *** Basic block 48

	sd          a0, 56(s11)
	li          t0, 64		// 0x40 ASCII '@'
	mv          a1, t0
	mv          a0, s8
	call        StorageIs

	// *** Basic block 49

	sb          a0, 64(s11)
	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 1176
	mv          a1, s11
	call        VectorAppend

	// *** Basic block 50

	j           .CompileDeclaration_label_370

	// *** Basic block 51

.CompileDeclaration_label_333:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a1, s10
	mv          a0, s7
	call        AnalyzeInitializer

	// *** Basic block 52

	mv          s7, a0
	la          t0, compiler
	ld          s8, 0(t0)
	lb          t0, 1231(s8)
	beqz        t0, .CompileDeclaration_label_364

	// *** Basic block 53

	ld          s8, 1248(s8)
	lla         a1, .str.41
	mv          a0, s8
	call        fprintf

	// *** Basic block 54

	mv          a2, s8
	mv          a1, x0
	mv          a0, s7
	call        ASTNodePrint

	// *** Basic block 55

.CompileDeclaration_label_364:
	mv          a1, s7
	mv          a0, s2
	call        AddInitializedStaticVariable

	// *** Basic block 56

.CompileDeclaration_label_370:

	// *** Basic block 57

.CompileDeclaration_label_371:
	la          t0, compiler
	ld          t0, 0(t0)
	lb          t0, 1228(t0)
	beqz        t0, .CompileDeclaration_label_404

	// *** Basic block 58

	ld          s2, 56(s2)
	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 608
	li          t0, 52		// 0x34 ASCII '4'
	mv          a2, t0
	mv          a1, s3
	call        BuildDebugInfo

	// *** Basic block 59

	sd          a0, 128(s2)
	ld          a0, 128(s2)
	ld          a1, 16(s2)
	call        VariableDIESetStatic

	// *** Basic block 60

	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 608
	mv          a1, s2
	call        BuildDebugInfoAfterCodegen

	// *** Basic block 61

.CompileDeclaration_label_404:

	// *** Basic block 62

.CompileDeclaration_label_405:

	// *** Basic block 63

.CompileDeclaration_label_406:

	// *** Basic block 64

.CompileDeclaration_label_407:
	addi        s5, s5, 1
	bge         s5, s4, .CompileDeclaration_label_88

	// *** Basic block 65

.CompileDeclaration_label_411:

	// *** Basic block 66

.CompileDeclaration_label_412:

	// *** Basic block 67

.CompileDeclaration_label_413:
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
.func_end_CompileDeclaration:
	.size CompileDeclaration, .func_end_CompileDeclaration-CompileDeclaration

	.local  DeclarePredefinedTypesAndMacros
	.type DeclarePredefinedTypesAndMacros, @function

DeclarePredefinedTypesAndMacros:

	// *** Basic block 0

	.global NewString
	.global LexInitFromString
	.global LexNextToken
	.global SyntaxInit
	.global LexEof
	.local CompileDeclaration
	.global LexDestruct
	.global SyntaxDestruct
	addi sp, sp, -400
	// Saved return address (offset 392) and frame pointer (offset 384)
	sd ra, 392(sp)
	sd s0, 384(sp)
	addi s0, sp, 400
	// Local vars at offset -368(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0
	lla         a0, .str.42
	call        NewString

	// *** Basic block 1

	mv          s2, a0
	ld          s3, 160(s1)
	addi        a0, s0, -368
	lla         a1, .str.43
	mv          a3, s1
	mv          a2, s2
	call        LexInitFromString

	// *** Basic block 2

	addi        a0, s0, -368
	call        LexNextToken

	// *** Basic block 3

	addi        a0, s0, -184
	addi        a1, s0, -368
	call        SyntaxInit

	// *** Basic block 4

	addi        a0, s0, -368
	call        LexEof

	// *** Basic block 5

	not         t0, a0
	beqz        t0, .DeclarePredefinedTypesAndMacros_label_64

	// *** Basic block 6

.DeclarePredefinedTypesAndMacros_label_55:
	addi        a0, s0, -184
	call        CompileDeclaration

	// *** Basic block 7

	addi        a0, s0, -368
	call        LexEof

	// *** Basic block 8

	not         t0, a0
	bnez        t0, .DeclarePredefinedTypesAndMacros_label_55

	// *** Basic block 9

.DeclarePredefinedTypesAndMacros_label_64:
	addi        a0, s0, -368
	call        LexDestruct

	// *** Basic block 10

	sd          s3, 160(s1)
	addi        a0, s0, -184
	call        SyntaxDestruct

	// *** Basic block 11

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DeclarePredefinedTypesAndMacros:
	.size DeclarePredefinedTypesAndMacros, .func_end_DeclarePredefinedTypesAndMacros-DeclarePredefinedTypesAndMacros

	.local  CompareWarning
	.type CompareWarning, @function

CompareWarning:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global strcmp
	mv          t0, a0
	mv          t1, a1
	mv          t2, t0
	mv          t3, t1
	ld          a0, 0(t2)
	ld          a1, 0(t3)
	j           strcmp
.func_end_CompareWarning:
	.size CompareWarning, .func_end_CompareWarning-CompareWarning

	.global ParseTlsModelName
	.type ParseTlsModelName, @function

ParseTlsModelName:

	// *** Basic block 0

	.global StringEqual
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
	lla         a1, .str.44
	call        StringEqual

	// *** Basic block 1

	beqz        a0, .ParseTlsModelName_label_28

	// *** Basic block 2

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 3

.ParseTlsModelName_label_25:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.ParseTlsModelName_label_28:
	lla         a1, .str.45
	mv          a0, s1
	call        StringEqual

	// *** Basic block 5

	beqz        a0, .ParseTlsModelName_label_39

	// *** Basic block 6

	li          a0, 2		// 0x2 ASCII \x2
	j           .ParseTlsModelName_label_25

	// *** Basic block 7

.ParseTlsModelName_label_39:
	lla         a1, .str.46
	mv          a0, s1
	call        StringEqual

	// *** Basic block 8

	beqz        a0, .ParseTlsModelName_label_50

	// *** Basic block 9

	li          a0, 3		// 0x3 ASCII \x3
	j           .ParseTlsModelName_label_25

	// *** Basic block 10

.ParseTlsModelName_label_50:
	lla         a1, .str.47
	mv          a0, s1
	call        StringEqual

	// *** Basic block 11

	beqz        a0, .ParseTlsModelName_label_61

	// *** Basic block 12

	li          a0, 4		// 0x4 ASCII \x4
	j           .ParseTlsModelName_label_25

	// *** Basic block 13

.ParseTlsModelName_label_61:

	// *** Basic block 14

.ParseTlsModelName_label_62:

	// *** Basic block 15

.ParseTlsModelName_label_63:

	// *** Basic block 16

.ParseTlsModelName_label_64:
	mv          a0, x0
	j           .ParseTlsModelName_label_25
.func_end_ParseTlsModelName:
	.size ParseTlsModelName, .func_end_ParseTlsModelName-ParseTlsModelName

	.local  InitBasic
	.type InitBasic, @function

InitBasic:

	// *** Basic block 0

	.global StringInit
	.global VectorInit
	.global SetInit
	.local CompareWarning
	.global PreprocessorInit
	.global SyntaxInit
	.global getcwd
	.global DebugBuilderInit
	addi sp, sp, -16
	sd ra, 8(sp)
	sd s0, 0(sp)
	lui t0, 1
	addi t0, t0, 32
	sub sp, sp, t0
	addi t0, t0, 16
	add s0, sp, t0
	// Local vars at offset -4112(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	call        StringInit

	// *** Basic block 1

	addi        a0, s1, 1128
	call        VectorInit

	// *** Basic block 2

	addi        a0, s1, 1152
	call        VectorInit

	// *** Basic block 3

	addi        a0, s1, 1176
	call        VectorInit

	// *** Basic block 4

	addi        a0, s1, 1200
	call        VectorInit

	// *** Basic block 5

	addi        a0, s1, 56
	la          t0, CompareWarning
	mv          a1, t0
	call        SetInit

	// *** Basic block 6

	sw          x0, 40(s1)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 1224(s1)
	sd          x0, 1088(s1)
	addi        a0, s1, 88
	call        PreprocessorInit

	// *** Basic block 7

	addi        a0, s1, 448
	addi        a1, s1, 264
	call        SyntaxInit

	// *** Basic block 8

	li          t0, -4096		// 0xfffffffffffff000
	add         t0, s0, t0
	addi        a0, t0, -16
	li          t0, 4096		// 0x1000
	mv          a1, t0
	call        getcwd

	// *** Basic block 9

	mv          s3, a0
	bne         s3, x0, .InitBasic_label_91

	// *** Basic block 10

	lla         s3, .str.48

	// *** Basic block 11

.InitBasic_label_91:
	addi        a0, s1, 608
	lla         a2, .str.49
	mv          a3, s3
	mv          a1, s2
	call        DebugBuilderInit

	// *** Basic block 12

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InitBasic:
	.size InitBasic, .func_end_InitBasic-InitBasic

	.local  OpenSaveFiles
	.type OpenSaveFiles, @function

OpenSaveFiles:

	// *** Basic block 0

	.global stdout
	.global StringInit
	.global strstr
	.global StringAppend
	.global StringAppendChar
	.global fopen
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -96(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// End of stack frame
	mv          s1, a0
	la          t0, stdout
	ld          s2, 0(t0)
	sd          s2, 1240(s1)
	lb          t0, 1235(s1)
	beqz        t0, .OpenSaveFiles_label_93

	// *** Basic block 1

	addi        a0, s0, -96
	ld          a1, 16(s1)
	call        StringInit

	// *** Basic block 2

	addi        t0, s0, -96
	ld          a0, 16(t0)
	lla         a1, .str.50
	call        strstr

	// *** Basic block 3

	mv          s3, a0
	bne         s3, x0, .OpenSaveFiles_label_65

	// *** Basic block 4

	addi        a0, s0, -96
	lla         a1, .str.51
	call        StringAppend

	// *** Basic block 5

	j           .OpenSaveFiles_label_75

	// *** Basic block 6

.OpenSaveFiles_label_65:
	li          t0, 105		// 0x69 ASCII 'i'
	sb          t0, 1(s3)
	addi        a0, s0, -96
	li          t0, 114		// 0x72 ASCII 'r'
	mv          a1, t0
	call        StringAppendChar

	// *** Basic block 7

.OpenSaveFiles_label_75:
	addi        t0, s0, -96
	ld          a0, 16(t0)
	lla         a1, .str.52
	call        fopen

	// *** Basic block 8

	sd          a0, 1240(s1)
	ld          t0, 1240(s1)
	bne         t0, x0, .OpenSaveFiles_label_92

	// *** Basic block 9

	sd          s2, 1240(s1)

	// *** Basic block 10

.OpenSaveFiles_label_92:

	// *** Basic block 11

.OpenSaveFiles_label_93:
	sd          s2, 1248(s1)
	lb          t0, 1236(s1)
	beqz        t0, .OpenSaveFiles_label_154

	// *** Basic block 12

	addi        a0, s0, -56
	ld          a1, 16(s1)
	call        StringInit

	// *** Basic block 13

	addi        t0, s0, -56
	ld          a0, 16(t0)
	lla         a1, .str.53
	call        strstr

	// *** Basic block 14

	mv          s4, a0
	bne         s4, x0, .OpenSaveFiles_label_126

	// *** Basic block 15

	addi        a0, s0, -56
	lla         a1, .str.54
	call        StringAppend

	// *** Basic block 16

	j           .OpenSaveFiles_label_136

	// *** Basic block 17

.OpenSaveFiles_label_126:
	li          t0, 97		// 0x61 ASCII 'a'
	sb          t0, 1(s4)
	addi        a0, s0, -56
	lla         a1, .str.55
	call        StringAppend

	// *** Basic block 18

.OpenSaveFiles_label_136:
	addi        t0, s0, -56
	ld          a0, 16(t0)
	lla         a1, .str.56
	call        fopen

	// *** Basic block 19

	sd          a0, 1248(s1)
	ld          t0, 1248(s1)
	bne         t0, x0, .OpenSaveFiles_label_153

	// *** Basic block 20

	sd          s2, 1248(s1)

	// *** Basic block 21

.OpenSaveFiles_label_153:

	// *** Basic block 22

.OpenSaveFiles_label_154:
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
.func_end_OpenSaveFiles:
	.size OpenSaveFiles, .func_end_OpenSaveFiles-OpenSaveFiles

	.local  ParseOptimizationOption
	.type ParseOptimizationOption, @function

ParseOptimizationOption:

	// *** Basic block 0

	.local OptionStringValue
	.global fprintf
	.global stderr
	.global exit
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
	sb          x0, 1229(s1)
	li          s2, 2		// 0x2 ASCII \x2
	mv          a0, s2
	call        OptionStringValue

	// *** Basic block 1

	mv          s3, a0
	bne         s3, x0, .ParseOptimizationOption_label_40

	// *** Basic block 2

.ParseOptimizationOption_label_37:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.ParseOptimizationOption_label_40:
	ld          t0, 24(s3)
	bnez        t0, .ParseOptimizationOption_label_52

	// *** Basic block 4

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 1229(s1)
	sw          s2, 1256(s1)
	j           .ParseOptimizationOption_label_104

	// *** Basic block 5

.ParseOptimizationOption_label_52:
	ld          t0, 16(s3)
	lb          s2, 0(t0)
	li          t0, 48		// 0x30 ASCII '0'
	blt         s2, t0, .ParseOptimizationOption_label_87

	// *** Basic block 6

	li          t0, 51		// 0x33 ASCII '3'
	blt         t0, s2, .ParseOptimizationOption_label_87

	// *** Basic block 7

	addi        t0, s2, -48
	slli        t1, t0, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 8

	j           .ParseOptimizationOption_label_76

	// *** Basic block 9

	j           .ParseOptimizationOption_label_78

	// *** Basic block 10

	j           .ParseOptimizationOption_label_79

	// *** Basic block 11

	j           .ParseOptimizationOption_label_80

	// *** Basic block 12

.ParseOptimizationOption_label_76:
	j           .ParseOptimizationOption_label_103

	// *** Basic block 13

.ParseOptimizationOption_label_78:

	// *** Basic block 14

.ParseOptimizationOption_label_79:

	// *** Basic block 15

.ParseOptimizationOption_label_80:
	li          t1, 1		// 0x1 ASCII \x1
	sb          t1, 1229(s1)
	sw          t0, 1256(s1)
	j           .ParseOptimizationOption_label_103

	// *** Basic block 16

.ParseOptimizationOption_label_87:
	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.57
	mv          a2, s2
	call        fprintf

	// *** Basic block 17

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	call        exit

	// *** Basic block 18

	j           .ParseOptimizationOption_label_103

	// *** Basic block 19

.ParseOptimizationOption_label_103:

	// *** Basic block 20

.ParseOptimizationOption_label_104:
	j           .ParseOptimizationOption_label_37
.func_end_ParseOptimizationOption:
	.size ParseOptimizationOption, .func_end_ParseOptimizationOption-ParseOptimizationOption

	.local  InitBasicOptions
	.type InitBasicOptions, @function

InitBasicOptions:

	// *** Basic block 0

	.local OptionIntValue
	.local OptionBoolValue
	.local OptionStringValue
	.global fprintf
	.global stderr
	.global StringEqual
	.global NewPCodeTarget
	.global NewRVTarget
	.global New6502Target
	.local ParseOptimizationOption
	.global ParseTlsModelName
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
	li          s3, 20		// 0x14 ASCII \x14
	mv          a2, s3
	li          a0, 15		// 0xf ASCII \xf
	call        OptionIntValue

	// *** Basic block 1

	sw          a0, 44(s1)
	mv          a2, x0
	mv          a1, s2
	li          t0, 13		// 0xd ASCII \xd
	mv          a0, t0
	call        OptionBoolValue

	// *** Basic block 2

	sb          a0, 48(s1)
	mv          a2, x0
	mv          a1, s2
	li          t0, 14		// 0xe ASCII \xe
	mv          a0, t0
	call        OptionBoolValue

	// *** Basic block 3

	sb          a0, 49(s1)
	mv          a1, s2
	li          t0, 3		// 0x3 ASCII \x3
	mv          a0, t0
	call        OptionStringValue

	// *** Basic block 4

	sd          a0, 1120(s1)
	ld          s4, 1120(s1)
	bne         s4, x0, .InitBasicOptions_label_114

	// *** Basic block 5

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.58
	call        fprintf

	// *** Basic block 6

	mv          a0, x0

	// *** Basic block 7

.InitBasicOptions_label_111:
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

.InitBasicOptions_label_114:
	lla         a1, .str.59
	mv          a0, s4
	call        StringEqual

	// *** Basic block 9

	mv          s5, a0
	bnez        a0, .InitBasicOptions_label_131

	// *** Basic block 10

	lla         a1, .str.60
	mv          a0, s4
	call        StringEqual

	// *** Basic block 11

	mv          s5, a0

	// *** Basic block 12

.InitBasicOptions_label_131:
	beqz        s5, .InitBasicOptions_label_137

	// *** Basic block 13

	call        NewPCodeTarget

	// *** Basic block 14

	sd          a0, 1112(s1)
	j           .InitBasicOptions_label_188

	// *** Basic block 15

.InitBasicOptions_label_137:
	lla         a1, .str.61
	mv          a0, s4
	call        StringEqual

	// *** Basic block 16

	mv          s6, a0
	bnez        a0, .InitBasicOptions_label_154

	// *** Basic block 17

	lla         a1, .str.62
	mv          a0, s4
	call        StringEqual

	// *** Basic block 18

	mv          s6, a0

	// *** Basic block 19

.InitBasicOptions_label_154:
	beqz        s6, .InitBasicOptions_label_160

	// *** Basic block 20

	call        NewRVTarget

	// *** Basic block 21

	sd          a0, 1112(s1)
	j           .InitBasicOptions_label_187

	// *** Basic block 22

.InitBasicOptions_label_160:
	lla         a1, .str.63
	mv          a0, s4
	call        StringEqual

	// *** Basic block 23

	beqz        a0, .InitBasicOptions_label_172

	// *** Basic block 24

	call        New6502Target

	// *** Basic block 25

	sd          a0, 1112(s1)
	j           .InitBasicOptions_label_186

	// *** Basic block 26

.InitBasicOptions_label_172:
	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.64
	ld          a2, 16(s4)
	call        fprintf

	// *** Basic block 27

	mv          a0, x0
	j           .InitBasicOptions_label_111

	// *** Basic block 28

.InitBasicOptions_label_186:

	// *** Basic block 29

.InitBasicOptions_label_187:

	// *** Basic block 30

.InitBasicOptions_label_188:
	mv          a2, x0
	mv          a1, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	call        OptionBoolValue

	// *** Basic block 31

	sb          a0, 1228(s1)
	mv          a1, s2
	mv          a0, s1
	call        ParseOptimizationOption

	// *** Basic block 32

	mv          a2, x0
	mv          a1, s2
	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        OptionBoolValue

	// *** Basic block 33

	sb          a0, 1230(s1)
	lb          t0, 1230(s1)
	beqz        t0, .InitBasicOptions_label_221

	// *** Basic block 34

	li          s4, 1		// 0x1 ASCII \x1
	j           .InitBasicOptions_label_223

	// *** Basic block 35

.InitBasicOptions_label_221:
	li          s4, 4		// 0x4 ASCII \x4

	// *** Basic block 36

.InitBasicOptions_label_223:
	sw          s4, 1104(s1)
	ld          t0, 1112(s1)
	lw          t0, 40(t0)
	sw          t0, 1080(s1)
	mv          a2, x0
	mv          a1, s2
	li          t0, 18		// 0x12 ASCII \x12
	mv          a0, t0
	call        OptionBoolValue

	// *** Basic block 37

	sb          a0, 1231(s1)
	mv          a2, x0
	mv          a1, s2
	li          t0, 19		// 0x13 ASCII \x13
	mv          a0, t0
	call        OptionBoolValue

	// *** Basic block 38

	sb          a0, 1232(s1)
	mv          a2, x0
	mv          a1, s2
	li          t0, 22		// 0x16 ASCII \x16
	mv          a0, t0
	call        OptionBoolValue

	// *** Basic block 39

	sb          a0, 1235(s1)
	mv          a2, x0
	mv          a1, s2
	li          t0, 23		// 0x17 ASCII \x17
	mv          a0, t0
	call        OptionBoolValue

	// *** Basic block 40

	sb          a0, 1236(s1)
	mv          a2, x0
	mv          a1, s2
	mv          a0, s3
	call        OptionBoolValue

	// *** Basic block 41

	sb          a0, 1233(s1)
	mv          a2, x0
	mv          a1, s2
	li          t0, 21		// 0x15 ASCII \x15
	mv          a0, t0
	call        OptionBoolValue

	// *** Basic block 42

	sb          a0, 1234(s1)
	mv          a1, s2
	li          t0, 16		// 0x10 ASCII \x10
	mv          a0, t0
	call        OptionStringValue

	// *** Basic block 43

	mv          s3, a0
	beq         s3, x0, .InitBasicOptions_label_324

	// *** Basic block 44

	mv          a0, s3
	call        ParseTlsModelName

	// *** Basic block 45

	sw          a0, 1104(s1)
	lw          t0, 1104(s1)
	bnez        t0, .InitBasicOptions_label_323

	// *** Basic block 46

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.65
	ld          a2, 16(s3)
	call        fprintf

	// *** Basic block 47

	mv          a0, x0
	j           .InitBasicOptions_label_111

	// *** Basic block 48

.InitBasicOptions_label_323:

	// *** Basic block 49

.InitBasicOptions_label_324:
	li          a0, 1		// 0x1 ASCII \x1
	j           .InitBasicOptions_label_111
.func_end_InitBasicOptions:
	.size InitBasicOptions, .func_end_InitBasicOptions-InitBasicOptions

	.local  InitComplexOptions
	.type InitComplexOptions, @function

InitComplexOptions:

	// *** Basic block 0

	.global PreprocessorAddUserIncludePath
	.global PreprocessorInsertSystemIncludePath
	.global StringIndexOf
	.global StringSet
	.global StringSubstring
	.global PreprocessorDefineMacro
	.global StringDestruct
	.global PreprocessorUndefineMacro
	.global StringStartsWith
	.global DisableWarning
	.global EnableWarning
	.local OpenSaveFiles
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -96(s0)
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
	mv          s3, x0
	ld          s4, 8(s1)
	bge         x0, s4, .InitComplexOptions_label_212

	// *** Basic block 1

	ld          t0, 0(s1)

	// *** Basic block 2

.InitComplexOptions_label_42:
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s1, 0(t0)
	lw          t0, 0(s1)
	li          t1, 7		// 0x7 ASCII \x7
	blt         t0, t1, .InitComplexOptions_label_205

	// *** Basic block 3

	li          t1, 12		// 0xc ASCII \xc
	blt         t1, t0, .InitComplexOptions_label_205

	// *** Basic block 4

	addi        t0, t0, -7
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 5

	j           .InitComplexOptions_label_68

	// *** Basic block 6

	j           .InitComplexOptions_label_77

	// *** Basic block 7

	j           .InitComplexOptions_label_89

	// *** Basic block 8

	j           .InitComplexOptions_label_177

	// *** Basic block 9

	j           .InitComplexOptions_label_205

	// *** Basic block 10

	j           .InitComplexOptions_label_184

	// *** Basic block 11

.InitComplexOptions_label_68:
	addi        a0, s2, 88
	addi        t0, s1, 8
	ld          a1, 16(t0)
	call        PreprocessorAddUserIncludePath

	// *** Basic block 12

	j           .InitComplexOptions_label_207

	// *** Basic block 13

.InitComplexOptions_label_77:
	addi        a0, s2, 88
	addi        t0, s1, 8
	ld          a2, 16(t0)
	mv          a1, x0
	call        PreprocessorInsertSystemIncludePath

	// *** Basic block 14

	j           .InitComplexOptions_label_207

	// *** Basic block 15

.InitComplexOptions_label_89:
	addi        a0, s1, 8
	lla         a1, .str.66
	call        StringIndexOf

	// *** Basic block 16

	mv          s5, a0
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
	bge         s5, x0, .InitComplexOptions_label_136

	// *** Basic block 17

	addi        a0, s0, -96
	ld          a1, 16(a0)
	call        StringSet

	// *** Basic block 18

	addi        a0, s0, -56
	lla         a1, .str.67
	call        StringSet

	// *** Basic block 19

	j           .InitComplexOptions_label_158

	// *** Basic block 20

.InitComplexOptions_label_136:
	addi        a0, s1, 8
	addi        a3, s0, -96
	mv          a2, s5
	mv          a1, x0
	call        StringSubstring

	// *** Basic block 21

	addi        a0, s1, 8
	addi        a1, s5, 1
	ld          s6, 24(a0)
	sub         a2, s6, s5
	addi        a3, s0, -96
	call        StringSubstring

	// *** Basic block 22

.InitComplexOptions_label_158:
	addi        a0, s2, 88
	addi        s6, s0, -96
	ld          a1, 16(s6)
	addi        t0, s0, -56
	ld          a2, 16(t0)
	call        PreprocessorDefineMacro

	// *** Basic block 23

	addi        a0, s0, -96
	call        StringDestruct

	// *** Basic block 24

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 25

	j           .InitComplexOptions_label_207

	// *** Basic block 26

.InitComplexOptions_label_177:
	addi        a0, s2, 88
	addi        a1, s1, 8
	call        PreprocessorUndefineMacro

	// *** Basic block 27

	j           .InitComplexOptions_label_207

	// *** Basic block 28

.InitComplexOptions_label_184:
	addi        a0, s1, 8
	lla         a1, .str.68
	call        StringStartsWith

	// *** Basic block 29

	beqz        a0, .InitComplexOptions_label_198

	// *** Basic block 30

	ld          t0, 16(a0)
	addi        a0, t0, 3
	call        DisableWarning

	// *** Basic block 31

	j           .InitComplexOptions_label_203

	// *** Basic block 32

.InitComplexOptions_label_198:
	ld          a0, 16(a0)
	call        EnableWarning

	// *** Basic block 33

.InitComplexOptions_label_203:
	j           .InitComplexOptions_label_207

	// *** Basic block 34

.InitComplexOptions_label_205:
	j           .InitComplexOptions_label_207

	// *** Basic block 35

.InitComplexOptions_label_207:

	// *** Basic block 36

.InitComplexOptions_label_208:
	addi        s3, s3, 1
	bge         s3, s4, .InitComplexOptions_label_42

	// *** Basic block 37

.InitComplexOptions_label_212:
	mv          a0, s2
	call        OpenSaveFiles

	// *** Basic block 38

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
.func_end_InitComplexOptions:
	.size InitComplexOptions, .func_end_InitComplexOptions-InitComplexOptions

	.local  CompilerInitCommon
	.type CompilerInitCommon, @function

CompilerInitCommon:

	// *** Basic block 0

	.local InitBasic
	.local InitBasicOptions
	.local OptionStringValue
	.global chdir
	.global fprintf
	.global stderr
	.global strerror
	.global errno
	.global PreprocessorDefineArchitectureMacros
	.local InitComplexOptions
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
	mv          s2, a2
	call        InitBasic

	// *** Basic block 1

	mv          a1, s2
	mv          a0, s1
	call        InitBasicOptions

	// *** Basic block 2

	mv          s3, a0
	not         t0, s3
	beqz        t0, .CompilerInitCommon_label_48

	// *** Basic block 3

	mv          a0, x0

	// *** Basic block 4

.CompilerInitCommon_label_45:
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

.CompilerInitCommon_label_48:
	mv          a1, s2
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        OptionStringValue

	// *** Basic block 6

	mv          s3, a0
	beq         s3, x0, .CompilerInitCommon_label_93

	// *** Basic block 7

	ld          s4, 16(s3)
	mv          a0, s4
	call        chdir

	// *** Basic block 8

	mv          s5, a0
	li          t0, -1		// 0xffffffffffffffff
	bne         s5, t0, .CompilerInitCommon_label_92

	// *** Basic block 9

	la          t0, stderr
	ld          s3, 0(t0)
	lla         s5, .str.69
	la          t0, errno
	lw          a0, 0(t0)
	call        strerror

	// *** Basic block 10

	mv          a3, a0
	mv          a2, s4
	mv          a1, s5
	mv          a0, s3
	call        fprintf

	// *** Basic block 11

	mv          a0, x0
	j           .CompilerInitCommon_label_45

	// *** Basic block 12

.CompilerInitCommon_label_92:

	// *** Basic block 13

.CompilerInitCommon_label_93:
	addi        a0, s1, 88
	call        PreprocessorDefineArchitectureMacros

	// *** Basic block 14

	mv          a1, s2
	mv          a0, s1
	call        InitComplexOptions

	// *** Basic block 15

	li          a0, 1		// 0x1 ASCII \x1
	j           .CompilerInitCommon_label_45
.func_end_CompilerInitCommon:
	.size CompilerInitCommon, .func_end_CompilerInitCommon-CompilerInitCommon

	.global CompilerDestruct
	.type CompilerDestruct, @function

CompilerDestruct:

	// *** Basic block 0

	.global stdout
	.global fclose
	.global DebugBuilderDestruct
	.global ClearSymbolTable
	.global StringDestruct
	.global VectorDestruct
	.global InitializedStaticVariableDelete
	.global UninitializedStaticVariableDelete
	.global StringLiteralDelete
	.global PreprocessorDestruct
	.global SyntaxDestruct
	.global LexDestruct
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
	ld          s2, 1240(s1)
	la          t0, stdout
	ld          s3, 0(t0)
	beq         s2, s3, .CompilerDestruct_label_47

	// *** Basic block 1

	mv          a0, s2
	call        fclose

	// *** Basic block 2

.CompilerDestruct_label_47:
	ld          s2, 1248(s1)
	beq         s2, s3, .CompilerDestruct_label_56

	// *** Basic block 3

	mv          a0, s2
	call        fclose

	// *** Basic block 4

.CompilerDestruct_label_56:
	addi        a0, s1, 608
	call        DebugBuilderDestruct

	// *** Basic block 5

	addi        a0, s1, 904
	li          s2, 1		// 0x1 ASCII \x1
	mv          a1, s2
	call        ClearSymbolTable

	// *** Basic block 6

	addi        a0, s1, 992
	mv          a1, s2
	call        ClearSymbolTable

	// *** Basic block 7

	mv          a0, s1
	call        StringDestruct

	// *** Basic block 8

	ld          t0, 1112(s1)
	ld          s2, 160(t0)
	mv          s3, x0
	addi        t0, s1, 1128
	ld          s4, 8(t0)
	bge         x0, s4, .CompilerDestruct_label_99

	// *** Basic block 9

	ld          s5, 1128(s1)

	// *** Basic block 10

.CompilerDestruct_label_88:
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          a0, 0(t0)
	jalr         x1, s2, 0

	// *** Basic block 11

.CompilerDestruct_label_95:
	addi        s3, s3, 1
	bge         s3, s4, .CompilerDestruct_label_88

	// *** Basic block 12

.CompilerDestruct_label_99:
	addi        a0, s1, 1128
	call        VectorDestruct

	// *** Basic block 13

	mv          s2, x0
	addi        t0, s1, 1152
	ld          s4, 8(t0)
	bge         x0, s4, .CompilerDestruct_label_121

	// *** Basic block 14

	ld          s5, 1152(s1)

	// *** Basic block 15

.CompilerDestruct_label_111:
	slli        t0, s2, 3
	add         t0, s5, t0
	ld          a0, 0(t0)
	call        InitializedStaticVariableDelete

	// *** Basic block 16

.CompilerDestruct_label_117:
	addi        s2, s2, 1
	bge         s2, s4, .CompilerDestruct_label_111

	// *** Basic block 17

.CompilerDestruct_label_121:
	addi        a0, s1, 1152
	call        VectorDestruct

	// *** Basic block 18

	ld          s4, 1176(s1)
	mv          s5, x0
	ld          s6, 8(a0)
	bge         x0, s6, .CompilerDestruct_label_143

	// *** Basic block 19

.CompilerDestruct_label_133:
	slli        t0, s5, 3
	add         t0, s4, t0
	ld          a0, 0(t0)
	call        UninitializedStaticVariableDelete

	// *** Basic block 20

.CompilerDestruct_label_139:
	addi        s5, s5, 1
	bge         s5, s6, .CompilerDestruct_label_133

	// *** Basic block 21

.CompilerDestruct_label_143:
	addi        a0, s1, 1176
	call        VectorDestruct

	// *** Basic block 22

	mv          s4, x0
	addi        t0, s1, 1200
	ld          s6, 8(t0)
	bge         x0, s6, .CompilerDestruct_label_165

	// *** Basic block 23

	ld          s7, 1200(s1)

	// *** Basic block 24

.CompilerDestruct_label_155:
	slli        t0, s4, 3
	add         t0, s7, t0
	ld          a0, 0(t0)
	call        StringLiteralDelete

	// *** Basic block 25

.CompilerDestruct_label_161:
	addi        s4, s4, 1
	bge         s4, s6, .CompilerDestruct_label_155

	// *** Basic block 26

.CompilerDestruct_label_165:
	addi        a0, s1, 1200
	call        VectorDestruct

	// *** Basic block 27

	addi        a0, s1, 88
	call        PreprocessorDestruct

	// *** Basic block 28

	addi        a0, s1, 448
	call        SyntaxDestruct

	// *** Basic block 29

	addi        a0, s1, 264
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
	j           LexDestruct
.func_end_CompilerDestruct:
	.size CompilerDestruct, .func_end_CompilerDestruct-CompilerDestruct

	.global CompilerDelete
	.type CompilerDelete, @function

CompilerDelete:

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
	.global CompilerDestruct
	.global free
	mv          s1, a0
	call        CompilerDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_CompilerDelete:
	.size CompilerDelete, .func_end_CompilerDelete-CompilerDelete

	.global CompilerInitFromFile
	.type CompilerInitFromFile, @function

CompilerInitFromFile:

	// *** Basic block 0

	.local CompilerInitCommon
	.global LexInitFromFile
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
	call        CompilerInitCommon

	// *** Basic block 1

	mv          s3, a0
	not         t0, s3
	beqz        t0, .CompilerInitFromFile_label_34

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.CompilerInitFromFile_label_31:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.CompilerInitFromFile_label_34:
	addi        a0, s1, 264
	addi        a2, s1, 88
	mv          a1, s2
	call        LexInitFromFile

	// *** Basic block 5

	mv          s3, a0
	not         t0, s3
	beqz        t0, .CompilerInitFromFile_label_48

	// *** Basic block 6

	mv          a0, x0
	j           .CompilerInitFromFile_label_31

	// *** Basic block 7

.CompilerInitFromFile_label_48:
	li          a0, 1		// 0x1 ASCII \x1
	j           .CompilerInitFromFile_label_31
.func_end_CompilerInitFromFile:
	.size CompilerInitFromFile, .func_end_CompilerInitFromFile-CompilerInitFromFile

	.global CompilerInitFromString
	.type CompilerInitFromString, @function

CompilerInitFromString:

	// *** Basic block 0

	.local CompilerInitCommon
	.global NewString
	.global LexInitFromString
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
	mv          a2, a3
	call        CompilerInitCommon

	// *** Basic block 1

	mv          s4, a0
	not         t0, s4
	beqz        t0, .CompilerInitFromString_label_38

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.CompilerInitFromString_label_35:
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

.CompilerInitFromString_label_38:
	mv          a0, s3
	call        NewString

	// *** Basic block 5

	mv          s4, a0
	addi        a0, s1, 264
	addi        a3, s1, 88
	mv          a2, s4
	mv          a1, s2
	call        LexInitFromString

	// *** Basic block 6

	li          a0, 1		// 0x1 ASCII \x1
	j           .CompilerInitFromString_label_35
.func_end_CompilerInitFromString:
	.size CompilerInitFromString, .func_end_CompilerInitFromString-CompilerInitFromString

	.global CompilerInitForAssembler
	.type CompilerInitForAssembler, @function

CompilerInitForAssembler:

	// *** Basic block 0

	.global compiler
	.global malloc
	.local CompilerInitCommon
	.global LexInitFromFile
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
	li          a0, 1264		// 0x4f0
	call        malloc

	// *** Basic block 1

	la          t0, compiler
	sd          a0, 0(t0)
	la          t0, compiler
	ld          a0, 0(t0)
	mv          a2, s2
	mv          a1, s1
	call        CompilerInitCommon

	// *** Basic block 2

	mv          s3, a0
	not         t0, s3
	beqz        t0, .CompilerInitForAssembler_label_41

	// *** Basic block 3

	mv          a0, x0

	// *** Basic block 4

.CompilerInitForAssembler_label_38:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.CompilerInitForAssembler_label_41:
	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 264
	la          t0, compiler
	ld          t0, 0(t0)
	addi        a2, t0, 88
	mv          a1, s1
	call        LexInitFromFile

	// *** Basic block 6

	mv          s3, a0
	not         t0, s3
	beqz        t0, .CompilerInitForAssembler_label_59

	// *** Basic block 7

	mv          a0, x0
	j           .CompilerInitForAssembler_label_38

	// *** Basic block 8

.CompilerInitForAssembler_label_59:
	li          a0, 1		// 0x1 ASCII \x1
	j           .CompilerInitForAssembler_label_38
.func_end_CompilerInitForAssembler:
	.size CompilerInitForAssembler, .func_end_CompilerInitForAssembler-CompilerInitForAssembler

	.local  EmitAssemblyFile
	.type EmitAssemblyFile, @function

EmitAssemblyFile:

	// *** Basic block 0

	.global fprintf
	.global stderr
	.global stdout
	.global DebugBuilderEmitDebugInfo
	.global DebugBuilderEmitAbbreviations
	.global fclose
	sd          a0, -0(s0)	// Spilled @80
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -16(s0)
	// Spilled register region: 32 bytes at -48(s0) to -16(s0)
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
	sd          s2, -32(s0)	// Spilled @39
	ld          t0, 1112(s1)
	ld          t0, 56(t0)
	jalr         x1, t0, 0

	// *** Basic block 1

	lb          s3, 1232(s1)
	ld          t0, 1112(s1)
	ld          s4, 64(t0)
	la          t0, stdout
	ld          s5, 0(t0)
	ld          t0, 1112(s1)
	ld          s6, 64(t0)
	mv          s7, a0
	bne         s7, x0, .EmitAssemblyFile_label_85

	// *** Basic block 2

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.70
	ld          a2, 16(s2)
	call        fprintf

	// *** Basic block 3

	mv          a0, x0

	// *** Basic block 4

.EmitAssemblyFile_label_82:
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

	// *** Basic block 5

.EmitAssemblyFile_label_85:
	mv          s8, x0
	addi        t0, s1, 1128
	ld          s9, 8(t0)
	bge         x0, s9, .EmitAssemblyFile_label_117

	// *** Basic block 6

	ld          s10, 1128(s1)
	ld          s11, 1128(s1)

	// *** Basic block 7

.EmitAssemblyFile_label_95:
	beqz        s3, .EmitAssemblyFile_label_105

	// *** Basic block 8

	slli        t0, s8, 3
	add         t0, s10, t0
	ld          a0, 0(t0)
	mv          a1, s5
	jalr         x1, s4, 0

	// *** Basic block 9

.EmitAssemblyFile_label_105:
	slli        t0, s8, 3
	add         t0, s11, t0
	ld          a0, 0(t0)
	mv          a1, s7
	jalr         x1, s6, 0

	// *** Basic block 10

.EmitAssemblyFile_label_113:
	addi        s8, s8, 1
	bge         s8, s9, .EmitAssemblyFile_label_95

	// *** Basic block 11

.EmitAssemblyFile_label_117:
	ld          s3, 1112(s1)
	sd          s3, -48(s0)	// Spilled @119
	ld          t0, 80(s3)
	mv          a0, s7
	jalr         x1, t0, 0

	// *** Basic block 12

	ld          s4, 88(s3)
	ld          s5, 96(s3)
	ld          s6, 144(s3)
	sd          s6, -40(s0)	// Spilled @130
	ld          s9, 120(s3)
	ld          s10, 128(s3)
	mv          s11, x0
	sd          s11, -32(s0)	// Spilled @135
	mv          a0, x0
	addi        t0, s1, 1152
	ld          s2, 8(t0)
	bge         x0, s2, .EmitAssemblyFile_label_166

	// *** Basic block 13

	ld          t0, 1152(s1)
	sd          t0, -24(s0)	// Spilled @144
	ld          t0, 1152(s1)
	sd          t0, -32(s0)	// Spilled @145

	// *** Basic block 14

.EmitAssemblyFile_label_146:
	slli        t1, a0, 3
	ld          t2, -24(s0)	// Spilled @144
	add         t1, t2, t1
	ld          t0, 0(t1)
	lb          s6, 88(t0)
	not         t1, s6
	beqz        t1, .EmitAssemblyFile_label_160

	// *** Basic block 15

	mv          a1, s7
	mv          a0, t0
	jalr         x1, s4, 0

	// *** Basic block 16

.EmitAssemblyFile_label_160:
	or          s11, s11, s6

	// *** Basic block 17

.EmitAssemblyFile_label_162:
	addi        a0, a0, 1
	bge         a0, s2, .EmitAssemblyFile_label_146

	// *** Basic block 18

.EmitAssemblyFile_label_166:
	mv          s4, x0
	addi        t0, s1, 1176
	ld          s6, 8(t0)
	bge         x0, s6, .EmitAssemblyFile_label_196

	// *** Basic block 19

	ld          t0, 1176(s1)
	sd          t0, -32(s0)	// Spilled @174
	ld          t0, 1176(s1)
	sd          t0, -40(s0)	// Spilled @175

	// *** Basic block 20

.EmitAssemblyFile_label_176:
	slli        t1, s4, 3
	ld          t2, -32(s0)	// Spilled @174
	add         t1, t2, t1
	ld          t0, 0(t1)
	lb          s3, 64(t0)
	not         t1, s3
	beqz        t1, .EmitAssemblyFile_label_190

	// *** Basic block 21

	mv          a1, s7
	mv          a0, t0
	jalr         x1, s5, 0

	// *** Basic block 22

.EmitAssemblyFile_label_190:
	or          s11, s11, s3

	// *** Basic block 23

.EmitAssemblyFile_label_192:
	addi        s4, s4, 1
	bge         s4, s6, .EmitAssemblyFile_label_176

	// *** Basic block 24

.EmitAssemblyFile_label_196:
	ld          t0, -48(s0)	// Spilled @119
	ld          t1, 136(t0)
	mv          a0, s7
	jalr         x1, t1, 0

	// *** Basic block 25

	mv          s3, x0
	addi        t0, s1, 1200
	ld          s5, 8(t0)
	bge         x0, s5, .EmitAssemblyFile_label_222

	// *** Basic block 26

	ld          s11, 1200(s1)

	// *** Basic block 27

.EmitAssemblyFile_label_210:
	slli        t0, s3, 3
	add         t0, s11, t0
	ld          a0, 0(t0)
	mv          a1, s7
	ld          t0, -40(s0)	// Spilled @130
	jalr         x1, t0, 0

	// *** Basic block 28

.EmitAssemblyFile_label_218:
	addi        s3, s3, 1
	bge         s3, s5, .EmitAssemblyFile_label_210

	// *** Basic block 29

.EmitAssemblyFile_label_222:
	ld          t0, -32(s0)	// Spilled @135
	beqz        t0, .EmitAssemblyFile_label_280

	// *** Basic block 30

	ld          t0, -48(s0)	// Spilled @119
	ld          t1, 104(t0)
	mv          a0, s7
	jalr         x1, t1, 0

	// *** Basic block 31

	mv          s5, x0
	bge         x0, s2, .EmitAssemblyFile_label_251

	// *** Basic block 32

.EmitAssemblyFile_label_233:
	slli        t0, s5, 3
	ld          t1, -32(s0)	// Spilled @145
	add         t0, t1, t0
	ld          s11, 0(t0)
	lb          t0, 88(s11)
	beqz        t0, .EmitAssemblyFile_label_246

	// *** Basic block 33

	mv          a1, s7
	mv          a0, s11
	jalr         x1, s9, 0

	// *** Basic block 34

.EmitAssemblyFile_label_246:

	// *** Basic block 35

.EmitAssemblyFile_label_247:
	addi        s5, s5, 1
	bge         s5, s2, .EmitAssemblyFile_label_233

	// *** Basic block 36

.EmitAssemblyFile_label_251:
	ld          t0, -48(s0)	// Spilled @119
	ld          t1, 112(t0)
	mv          a0, s7
	jalr         x1, t1, 0

	// *** Basic block 37

	mv          s2, x0
	bge         x0, s6, .EmitAssemblyFile_label_279

	// *** Basic block 38

.EmitAssemblyFile_label_261:
	slli        t0, s2, 3
	ld          t1, -40(s0)	// Spilled @175
	add         t0, t1, t0
	ld          s9, 0(t0)
	lb          t0, 64(s9)
	beqz        t0, .EmitAssemblyFile_label_274

	// *** Basic block 39

	mv          a1, s7
	mv          a0, s9
	jalr         x1, s10, 0

	// *** Basic block 40

.EmitAssemblyFile_label_274:

	// *** Basic block 41

.EmitAssemblyFile_label_275:
	addi        s2, s2, 1
	bge         s2, s6, .EmitAssemblyFile_label_261

	// *** Basic block 42

.EmitAssemblyFile_label_279:

	// *** Basic block 43

.EmitAssemblyFile_label_280:
	lb          t0, 1228(s1)
	beqz        t0, .EmitAssemblyFile_label_298

	// *** Basic block 44

	ld          t0, -48(s0)	// Spilled @119
	ld          t1, 152(t0)
	mv          a0, s7
	jalr         x1, t1, 0

	// *** Basic block 45

	addi        t0, s1, 608
	sd          s7, 264(t0)
	addi        a0, s1, 608
	call        DebugBuilderEmitDebugInfo

	// *** Basic block 46

	addi        a0, s1, 608
	call        DebugBuilderEmitAbbreviations

	// *** Basic block 47

.EmitAssemblyFile_label_298:
	la          t0, stdout
	ld          t0, 0(t0)
	beq         s7, t0, .EmitAssemblyFile_label_307

	// *** Basic block 48

	mv          a0, s7
	call        fclose

	// *** Basic block 49

.EmitAssemblyFile_label_307:
	li          a0, 1		// 0x1 ASCII \x1
	j           .EmitAssemblyFile_label_82
.func_end_EmitAssemblyFile:
	.size EmitAssemblyFile, .func_end_EmitAssemblyFile-EmitAssemblyFile

	.local  Assemble
	.type Assemble, @function

Assemble:

	// *** Basic block 0

	.local OptionStringValue
	.global NewString
	.global strstr
	.global StringAppend
	.global StringDelete
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
	mv          a1, a2
	li          a0, 6		// 0x6 ASCII \x6
	call        OptionStringValue

	// *** Basic block 1

	mv          s3, a0
	beq         s3, x0, .Assemble_label_42

	// *** Basic block 2

	ld          a0, 16(s3)
	call        NewString

	// *** Basic block 3

	mv          s4, a0
	j           .Assemble_label_72

	// *** Basic block 4

.Assemble_label_42:
	ld          a0, 16(s1)
	call        NewString

	// *** Basic block 5

	mv          s4, a0
	ld          a0, 16(s4)
	lla         a1, .str.71
	call        strstr

	// *** Basic block 6

	mv          s5, a0
	bne         s5, x0, .Assemble_label_67

	// *** Basic block 7

	lla         a1, .str.72
	mv          a0, s4
	call        StringAppend

	// *** Basic block 8

	j           .Assemble_label_71

	// *** Basic block 9

.Assemble_label_67:
	li          t0, 111		// 0x6f ASCII 'o'
	sb          t0, 1(s5)

	// *** Basic block 10

.Assemble_label_71:

	// *** Basic block 11

.Assemble_label_72:
	ld          t0, 1112(s1)
	ld          t0, 72(t0)
	mv          a1, s4
	mv          a0, s2
	jalr         x1, t0, 0

	// *** Basic block 12

	mv          s6, a0
	not         t0, s6
	beqz        t0, .Assemble_label_94

	// *** Basic block 13

	mv          a0, s4
	call        StringDelete

	// *** Basic block 14

	mv          a0, x0

	// *** Basic block 15

.Assemble_label_91:
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

	// *** Basic block 16

.Assemble_label_94:
	mv          a0, s4
	j           .Assemble_label_91
.func_end_Assemble:
	.size Assemble, .func_end_Assemble-Assemble

	.local  Compile
	.type Compile, @function

Compile:

	// *** Basic block 0

	.global CreateGlobalSymbolTables
	.local DeclarePredefinedTypesAndMacros
	.global LexNextToken
	.global LexEof
	.global SyntaxResetForNewDeclaration
	.local CompileDeclaration
	.global HashTablePrintStats
	.global PreprocessorPrintStats
	.global NumErrors
	.local OptionBoolValue
	.local OptionStringValue
	.global StringInit
	.global strstr
	.global StringAppend
	.local EmitAssemblyFile
	.global StringDestruct
	.local Assemble
	.global remove
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -64(s0)
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
	call        CreateGlobalSymbolTables

	// *** Basic block 1

	addi        a0, s1, 88
	call        DeclarePredefinedTypesAndMacros

	// *** Basic block 2

	addi        a0, s1, 264
	call        LexNextToken

	// *** Basic block 3

	addi        a0, s1, 264
	call        LexEof

	// *** Basic block 4

	not         t0, a0
	beqz        t0, .Compile_label_67

	// *** Basic block 5

.Compile_label_55:
	addi        a0, s1, 448
	call        SyntaxResetForNewDeclaration

	// *** Basic block 6

	addi        a0, s1, 448
	call        CompileDeclaration

	// *** Basic block 7

	addi        a0, s1, 264
	call        LexEof

	// *** Basic block 8

	not         t0, a0
	bnez        t0, .Compile_label_55

	// *** Basic block 9

.Compile_label_67:
	lb          t0, 1231(s1)
	beqz        t0, .Compile_label_90

	// *** Basic block 10

	addi        a0, s1, 904
	ld          a1, 1248(s1)
	call        HashTablePrintStats

	// *** Basic block 11

	addi        a0, s1, 992
	ld          a1, 1248(s1)
	call        HashTablePrintStats

	// *** Basic block 12

	addi        a0, s1, 88
	ld          a1, 1248(s1)
	call        PreprocessorPrintStats

	// *** Basic block 13

.Compile_label_90:
	call        NumErrors

	// *** Basic block 14

	beqz        a0, .Compile_label_99

	// *** Basic block 15

	mv          a0, x0

	// *** Basic block 16

.Compile_label_96:
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

	// *** Basic block 17

.Compile_label_99:
	mv          a2, x0
	mv          a1, s2
	li          t0, 5		// 0x5 ASCII \x5
	mv          a0, t0
	call        OptionBoolValue

	// *** Basic block 18

	mv          s3, a0
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	mv          a1, s2
	li          t0, 6		// 0x6 ASCII \x6
	mv          a0, t0
	call        OptionStringValue

	// *** Basic block 19

	mv          s4, a0
	mv          t0, s3
	beqz        s3, .Compile_label_136

	// *** Basic block 20

	sub         t1, s4, x0
	snez        t0, t1

	// *** Basic block 21

.Compile_label_136:
	beqz        t0, .Compile_label_145

	// *** Basic block 22

	addi        a0, s0, -64
	ld          a1, 16(s4)
	call        StringInit

	// *** Basic block 23

	j           .Compile_label_177

	// *** Basic block 24

.Compile_label_145:
	addi        a0, s0, -64
	ld          a1, 16(s1)
	call        StringInit

	// *** Basic block 25

	addi        t0, s0, -64
	ld          a0, 16(t0)
	lla         a1, .str.73
	call        strstr

	// *** Basic block 26

	mv          s5, a0
	bne         s5, x0, .Compile_label_172

	// *** Basic block 27

	addi        a0, s0, -64
	lla         a1, .str.74
	call        StringAppend

	// *** Basic block 28

	j           .Compile_label_176

	// *** Basic block 29

.Compile_label_172:
	li          t0, 115		// 0x73 ASCII 's'
	sb          t0, 1(s5)

	// *** Basic block 30

.Compile_label_176:

	// *** Basic block 31

.Compile_label_177:
	addi        a1, s0, -64
	mv          a0, s1
	call        EmitAssemblyFile

	// *** Basic block 32

	mv          s6, a0
	not         t0, s6
	beqz        t0, .Compile_label_193

	// *** Basic block 33

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 34

	mv          a0, x0
	j           .Compile_label_96

	// *** Basic block 35

.Compile_label_193:
	beqz        s3, .Compile_label_201

	// *** Basic block 36

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 37

	mv          a0, s4
	j           .Compile_label_96

	// *** Basic block 38

.Compile_label_201:
	addi        a1, s0, -64
	mv          a2, s2
	mv          a0, s1
	call        Assemble

	// *** Basic block 39

	mv          s3, a0
	lb          t0, 1234(s1)
	not         t0, t0
	beqz        t0, .Compile_label_220

	// *** Basic block 40

	addi        t0, s0, -64
	ld          a0, 16(t0)
	call        remove

	// *** Basic block 41

.Compile_label_220:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 42

	mv          a0, s3
	j           .Compile_label_96
.func_end_Compile:
	.size Compile, .func_end_Compile-Compile

	.global CompileTranslationUnit
	.type CompileTranslationUnit, @function

CompileTranslationUnit:

	// *** Basic block 0

	.global ClearAllFiles
	.global compiler
	.global malloc
	.global CompilerInitFromFile
	.global fprintf
	.global stderr
	.local Compile
	.global CompilerDelete
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
	call        ClearAllFiles

	// *** Basic block 1

	li          t0, 1264		// 0x4f0
	mv          a0, t0
	call        malloc

	// *** Basic block 2

	la          t0, compiler
	sd          a0, 0(t0)
	la          t0, compiler
	ld          s3, 0(t0)
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        CompilerInitFromFile

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .CompileTranslationUnit_label_53

	// *** Basic block 4

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.75
	mv          a2, s1
	call        fprintf

	// *** Basic block 5

	mv          a0, x0

	// *** Basic block 6

.CompileTranslationUnit_label_50:
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

.CompileTranslationUnit_label_53:
	mv          a1, s2
	mv          a0, s3
	call        Compile

	// *** Basic block 8

	mv          s4, a0
	mv          a0, s3
	call        CompilerDelete

	// *** Basic block 9

	la          t0, compiler
	sd          x0, 0(t0)
	mv          a0, s4
	j           .CompileTranslationUnit_label_50
.func_end_CompileTranslationUnit:
	.size CompileTranslationUnit, .func_end_CompileTranslationUnit-CompileTranslationUnit

	.global CompileTranslationUnitFromString
	.type CompileTranslationUnitFromString, @function

CompileTranslationUnitFromString:

	// *** Basic block 0

	.global ClearAllFiles
	.global compiler
	.global malloc
	.global CompilerInitFromString
	.local Compile
	.global CompilerDelete
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
	call        ClearAllFiles

	// *** Basic block 1

	li          t0, 1264		// 0x4f0
	mv          a0, t0
	call        malloc

	// *** Basic block 2

	la          t0, compiler
	sd          a0, 0(t0)
	la          t0, compiler
	ld          s4, 0(t0)
	mv          a3, s3
	mv          a2, s2
	mv          a1, s1
	mv          a0, s4
	call        CompilerInitFromString

	// *** Basic block 3

	mv          a1, s3
	mv          a0, s4
	call        Compile

	// *** Basic block 4

	mv          s5, a0
	mv          a0, s4
	call        CompilerDelete

	// *** Basic block 5

	la          t0, compiler
	sd          x0, 0(t0)
	mv          a0, s5

	// *** Basic block 6

.CompileTranslationUnitFromString_label_53:
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
.func_end_CompileTranslationUnitFromString:
	.size CompileTranslationUnitFromString, .func_end_CompileTranslationUnitFromString-CompileTranslationUnitFromString

.PCend:
	.data
compiler_options:
	.type   compiler_options,@object
	.local  compiler_options
	.size   compiler_options,600
	.p2align  3
	.long    .str.1
	.word   2
	.word   1
	.byte   0
	.space  7
	.long    .str.2
	.word   0
	.word   2
	.byte   1
	.space  7
	.long    .str.3
	.word   0
	.word   3
	.byte   0
	.space  7
	.long    .str.4
	.word   2
	.word   4
	.byte   0
	.space  7
	.long    .str.5
	.word   2
	.word   5
	.byte   0
	.space  7
	.long    .str.6
	.word   0
	.word   6
	.byte   0
	.space  7
	.long    .str.7
	.word   0
	.word   8
	.byte   0
	.space  7
	.long    .str.8
	.word   0
	.word   7
	.byte   1
	.space  7
	.long    .str.9
	.word   0
	.word   9
	.byte   1
	.space  7
	.long    .str.10
	.word   0
	.word   10
	.byte   1
	.space  7
	.long    .str.11
	.word   2
	.word   11
	.byte   0
	.space  7
	.long    .str.12
	.word   2
	.word   11
	.byte   0
	.space  7
	.long    .str.13
	.word   2
	.word   13
	.byte   0
	.space  7
	.long    .str.14
	.word   2
	.word   14
	.byte   0
	.space  7
	.long    .str.15
	.word   0
	.word   12
	.byte   1
	.space  7
	.long    .str.16
	.word   1
	.word   15
	.byte   0
	.space  7
	.long    .str.17
	.word   0
	.word   16
	.byte   0
	.space  7
	.long    .str.18
	.word   0
	.word   17
	.byte   0
	.space  7
	.long    .str.19
	.word   2
	.word   18
	.byte   0
	.space  7
	.long    .str.20
	.word   2
	.word   19
	.byte   0
	.space  7
	.long    .str.21
	.word   2
	.word   20
	.byte   0
	.space  7
	.long    .str.22
	.word   2
	.word   21
	.byte   0
	.space  7
	.long    .str.23
	.word   2
	.word   22
	.byte   0
	.space  7
	.long    .str.24
	.word   2
	.word   23
	.byte   0
	.space  7
	.word   0
	.space  4
	.word   0
	.word   0
	.byte   0
	.space  7

	.type   compiler,@object
	.global compiler
	.comm   compiler,8,8

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "-g"
	.type .str.1, @object
	.size .str.1, 3

.str.2:
	.asciz "-O"
	.type .str.2, @object
	.size .str.2, 3

.str.3:
	.asciz "-target"
	.type .str.3, @object
	.size .str.3, 8

.str.4:
	.asciz "-c"
	.type .str.4, @object
	.size .str.4, 3

.str.5:
	.asciz "-S"
	.type .str.5, @object
	.size .str.5, 3

.str.6:
	.asciz "-o"
	.type .str.6, @object
	.size .str.6, 3

.str.7:
	.asciz "-isystem"
	.type .str.7, @object
	.size .str.7, 9

.str.8:
	.asciz "-I"
	.type .str.8, @object
	.size .str.8, 3

.str.9:
	.asciz "-D"
	.type .str.9, @object
	.size .str.9, 3

.str.10:
	.asciz "-U"
	.type .str.10, @object
	.size .str.10, 3

.str.11:
	.asciz "-fPIC"
	.type .str.11, @object
	.size .str.11, 6

.str.12:
	.asciz "-fpic"
	.type .str.12, @object
	.size .str.12, 6

.str.13:
	.asciz "-Werror"
	.type .str.13, @object
	.size .str.13, 8

.str.14:
	.asciz "-Wall"
	.type .str.14, @object
	.size .str.14, 6

.str.15:
	.asciz "-W"
	.type .str.15, @object
	.size .str.15, 3

.str.16:
	.asciz "-error-limit"
	.type .str.16, @object
	.size .str.16, 13

.str.17:
	.asciz "-ftls-model"
	.type .str.17, @object
	.size .str.17, 12

.str.18:
	.asciz "-chdir"
	.type .str.18, @object
	.size .str.18, 7

.str.19:
	.asciz "-Xfe-print"
	.type .str.19, @object
	.size .str.19, 11

.str.20:
	.asciz "-Xbe-print"
	.type .str.20, @object
	.size .str.20, 11

.str.21:
	.asciz "-Xpp-print"
	.type .str.21, @object
	.size .str.21, 11

.str.22:
	.asciz "-Xkeep-asm"
	.type .str.22, @object
	.size .str.22, 11

.str.23:
	.asciz "-Xsave-ir"
	.type .str.23, @object
	.size .str.23, 10

.str.24:
	.asciz "-Xsave-ast"
	.type .str.24, @object
	.size .str.24, 11

.str.25:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.25, @object
	.size .str.25, 30

.str.26:
	.asciz "compiler.c"
	.type .str.26, @object
	.size .str.26, 11

.str.27:
	.asciz "false"
	.type .str.27, @object
	.size .str.27, 6

.str.28:
	.asciz "Invalid static initialization; need a constant expression"
	.type .str.28, @object
	.size .str.28, 58

.str.29:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.29, @object
	.size .str.29, 30

.str.30:
	.asciz "compiler.c"
	.type .str.30, @object
	.size .str.30, 11

.str.31:
	.asciz "false"
	.type .str.31, @object
	.size .str.31, 6

.str.32:
	.asciz "Invalid static initialization; need a constant expression"
	.type .str.32, @object
	.size .str.32, 58

.str.33:
	.asciz "Illegal static initializer: need address of variable"
	.type .str.33, @object
	.size .str.33, 53

.str.34:
	.asciz "Cannot initialize a static struct/union here"
	.type .str.34, @object
	.size .str.34, 45

.str.35:
	.asciz "Invalid use of function in initialization"
	.type .str.35, @object
	.size .str.35, 42

.str.36:
	.asciz "Invalid static initialization"
	.type .str.36, @object
	.size .str.36, 30

.str.37:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.37, @object
	.size .str.37, 30

.str.38:
	.asciz "compiler.c"
	.type .str.38, @object
	.size .str.38, 11

.str.39:
	.asciz "subinit->op == AST_OP(designated_init)"
	.type .str.39, @object
	.size .str.39, 39

.str.40:
	.asciz "Cannot declare a variable with void type"
	.type .str.40, @object
	.size .str.40, 41

.str.41:
	.asciz "Initializer\n"
	.type .str.41, @object
	.size .str.41, 13

.str.42:
	.asciz "typedef void* __builtin_va_list;\n#define __asm asm\n#define __asm__ asm\n#define __attribute __attribute__\n#define __inline inline\n#define __signed signed\n\n"
	.type .str.42, @object
	.size .str.42, 155

.str.43:
	.asciz "builtin"
	.type .str.43, @object
	.size .str.43, 8

.str.44:
	.asciz "global-dynamic"
	.type .str.44, @object
	.size .str.44, 15

.str.45:
	.asciz "local-dynamic"
	.type .str.45, @object
	.size .str.45, 14

.str.46:
	.asciz "initial-exec"
	.type .str.46, @object
	.size .str.46, 13

.str.47:
	.asciz "local-exec"
	.type .str.47, @object
	.size .str.47, 11

.str.48:
	.asciz "unknown"
	.type .str.48, @object
	.size .str.48, 8

.str.49:
	.asciz "davecc"
	.type .str.49, @object
	.size .str.49, 7

.str.50:
	.asciz ".c"
	.type .str.50, @object
	.size .str.50, 3

.str.51:
	.asciz ".ir"
	.type .str.51, @object
	.size .str.51, 4

.str.52:
	.asciz "w"
	.type .str.52, @object
	.size .str.52, 2

.str.53:
	.asciz ".c"
	.type .str.53, @object
	.size .str.53, 3

.str.54:
	.asciz ".ast"
	.type .str.54, @object
	.size .str.54, 5

.str.55:
	.asciz "st"
	.type .str.55, @object
	.size .str.55, 3

.str.56:
	.asciz "w"
	.type .str.56, @object
	.size .str.56, 2

.str.57:
	.asciz "Invalid optimization level -O%c\n"
	.type .str.57, @object
	.size .str.57, 33

.str.58:
	.asciz "No target specified; please specify -target option\n"
	.type .str.58, @object
	.size .str.58, 52

.str.59:
	.asciz "pcode"
	.type .str.59, @object
	.size .str.59, 6

.str.60:
	.asciz "p-code"
	.type .str.60, @object
	.size .str.60, 7

.str.61:
	.asciz "riscv"
	.type .str.61, @object
	.size .str.61, 6

.str.62:
	.asciz "risc-v"
	.type .str.62, @object
	.size .str.62, 7

.str.63:
	.asciz "6502"
	.type .str.63, @object
	.size .str.63, 5

.str.64:
	.asciz "Unknown -target value %s\n"
	.type .str.64, @object
	.size .str.64, 26

.str.65:
	.asciz "Invalid TLS model value: %s\n"
	.type .str.65, @object
	.size .str.65, 29

.str.66:
	.asciz "="
	.type .str.66, @object
	.size .str.66, 2

.str.67:
	.asciz "(null)"
	.type .str.67, @object
	.size .str.67, 1

.str.68:
	.asciz "no-"
	.type .str.68, @object
	.size .str.68, 4

.str.69:
	.asciz "Failed to change directory to %s: %s\n"
	.type .str.69, @object
	.size .str.69, 38

.str.70:
	.asciz "Unable to open assembler file %s\n"
	.type .str.70, @object
	.size .str.70, 34

.str.71:
	.asciz ".c"
	.type .str.71, @object
	.size .str.71, 3

.str.72:
	.asciz ".o"
	.type .str.72, @object
	.size .str.72, 3

.str.73:
	.asciz ".c"
	.type .str.73, @object
	.size .str.73, 3

.str.74:
	.asciz ".s"
	.type .str.74, @object
	.size .str.74, 3

.str.75:
	.asciz "Cannot open file %s\n"
	.type .str.75, @object
	.size .str.75, 21

