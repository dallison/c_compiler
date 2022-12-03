	.file   "optimizer.c"
	.text
	.option pic
.PCbegin:
	.local  IsIntConstantWithValue
	.type IsIntConstantWithValue, @function

IsIntConstantWithValue:

	// *** Basic block 0

	.global IRIsConst
	.global TypeIsIntegral
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
	bne         s1, x0, .IsIntConstantWithValue_label_22

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.IsIntConstantWithValue_label_19:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.IsIntConstantWithValue_label_22:
	mv          a0, s1
	call        IRIsConst

	// *** Basic block 4

	not         t0, a0
	beqz        t0, .IsIntConstantWithValue_label_31

	// *** Basic block 5

	mv          a0, x0
	j           .IsIntConstantWithValue_label_19

	// *** Basic block 6

.IsIntConstantWithValue_label_31:
	ld          s4, 80(s1)
	sub         t0, s4, x0
	seqz        s3, t0
	beq         s4, x0, .IsIntConstantWithValue_label_42

	// *** Basic block 7

	mv          a0, s4
	call        TypeIsIntegral

	// *** Basic block 8

	not         s3, a0

	// *** Basic block 9

.IsIntConstantWithValue_label_42:
	beqz        s3, .IsIntConstantWithValue_label_47

	// *** Basic block 10

	mv          a0, x0
	j           .IsIntConstantWithValue_label_19

	// *** Basic block 11

.IsIntConstantWithValue_label_47:
	mv          s3, s1
	ld          t0, 136(s3)
	sub         t0, t0, s2
	seqz        a0, t0
	j           .IsIntConstantWithValue_label_19
.func_end_IsIntConstantWithValue:
	.size IsIntConstantWithValue, .func_end_IsIntConstantWithValue-IsIntConstantWithValue

	.local  IsIntConstantPowerOf2
	.type IsIntConstantPowerOf2, @function

IsIntConstantPowerOf2:

	// *** Basic block 0

	.global IRIsConst
	.global TypeIsIntegral
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
	bne         s1, x0, .IsIntConstantPowerOf2_label_25

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.IsIntConstantPowerOf2_label_22:
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

	// *** Basic block 3

.IsIntConstantPowerOf2_label_25:
	mv          a0, s1
	call        IRIsConst

	// *** Basic block 4

	not         t0, a0
	beqz        t0, .IsIntConstantPowerOf2_label_34

	// *** Basic block 5

	mv          a0, x0
	j           .IsIntConstantPowerOf2_label_22

	// *** Basic block 6

.IsIntConstantPowerOf2_label_34:
	ld          s4, 80(s1)
	sub         t0, s4, x0
	seqz        s3, t0
	beq         s4, x0, .IsIntConstantPowerOf2_label_45

	// *** Basic block 7

	mv          a0, s4
	call        TypeIsIntegral

	// *** Basic block 8

	not         s3, a0

	// *** Basic block 9

.IsIntConstantPowerOf2_label_45:
	beqz        s3, .IsIntConstantPowerOf2_label_50

	// *** Basic block 10

	mv          a0, x0
	j           .IsIntConstantPowerOf2_label_22

	// *** Basic block 11

.IsIntConstantPowerOf2_label_50:
	mv          s3, s1
	li          t0, 1		// 0x1 ASCII \x1
	sll         t0, t0, s2
	addi        s4, t0, -1
	ld          t0, 136(s3)
	and         s5, t0, s4
	snez        a0, s5
	beqz        s5, .IsIntConstantPowerOf2_label_68

	// *** Basic block 12

	sra         t0, t0, s2
	seqz        a0, t0

	// *** Basic block 13

.IsIntConstantPowerOf2_label_68:
	beqz        a0, .IsIntConstantPowerOf2_label_73

	// *** Basic block 14

	addi        t0, s5, -1
	and         t0, s5, t0
	seqz        a0, t0

	// *** Basic block 15

.IsIntConstantPowerOf2_label_73:
	j           .IsIntConstantPowerOf2_label_22
.func_end_IsIntConstantPowerOf2:
	.size IsIntConstantPowerOf2, .func_end_IsIntConstantPowerOf2-IsIntConstantPowerOf2

	.local  LogBase2
	.type LogBase2, @function

LogBase2:

	// *** Basic block 0

	.local MultiplyDeBruijnBitPosition2
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 136(t0)
	li          t2, 125613361		// 0x77cb531
	mul         t1, t1, t2
	li          t2, 4294967295		// 0xffffffff
	and         t1, t1, t2
	srli        t1, t1, 27
	slli        t1, t1, 2
	lla         t2, MultiplyDeBruijnBitPosition2
	add         t1, t2, t1
	lw          a0, 0(t1)

	// *** Basic block 1

.LogBase2_label_29:
	ret         
.func_end_LogBase2:
	.size LogBase2, .func_end_LogBase2-LogBase2

	.local  BitMask
	.type BitMask, @function

BitMask:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 136(t0)
	addi        a0, t1, -1

	// *** Basic block 1

.BitMask_label_17:
	ret         
.func_end_BitMask:
	.size BitMask, .func_end_BitMask-BitMask

	.local  ReduceNodeStrength
	.type ReduceNodeStrength, @function

ReduceNodeStrength:

	// *** Basic block 0

	.local IsIntConstantWithValue
	.global BasicBlockReplaceInstruction
	.global IRRemoveInput
	.local IsIntConstantPowerOf2
	.global GeneratorGetIntConstant
	.local LogBase2
	.global IRAddInput
	.global IRReplaceInput
	.global TypeIsUnsigned
	.local BitMask
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
	mv          s1, a2
	mv          s2, a0
	mv          s3, a1
	lw          s4, 20(s1)
	li          t0, 37		// 0x25 ASCII '%'
	beq         s4, t0, .ReduceNodeStrength_label_75

	// *** Basic block 1

	li          t0, 40		// 0x28 ASCII '('
	beq         s4, t0, .ReduceNodeStrength_label_76

	// *** Basic block 2

	li          t0, 41		// 0x29 ASCII ')'
	beq         s4, t0, .ReduceNodeStrength_label_118

	// *** Basic block 3

	li          t0, 45		// 0x2d ASCII '-'
	beq         s4, t0, .ReduceNodeStrength_label_156

	// *** Basic block 4

	li          t0, 48		// 0x30 ASCII '0'
	beq         s4, t0, .ReduceNodeStrength_label_323

	// *** Basic block 5

	li          t0, 51		// 0x33 ASCII '3'
	beq         s4, t0, .ReduceNodeStrength_label_420

	// *** Basic block 6

.ReduceNodeStrength_label_73:
	j           .ReduceNodeStrength_label_487

	// *** Basic block 7

.ReduceNodeStrength_label_75:

	// *** Basic block 8

.ReduceNodeStrength_label_76:
	ld          s8, 24(s1)
	ld          s9, 0(s8)
	mv          a1, x0
	mv          a0, s9
	call        IsIntConstantWithValue

	// *** Basic block 9

	beqz        a0, .ReduceNodeStrength_label_98

	// *** Basic block 10

	ld          a3, 8(s8)
	mv          a2, s1
	mv          a1, s3
	mv          a0, s2
	call        BasicBlockReplaceInstruction

	// *** Basic block 11

	j           .ReduceNodeStrength_label_116

	// *** Basic block 12

.ReduceNodeStrength_label_98:
	ld          a0, 8(s8)
	mv          a1, x0
	call        IsIntConstantWithValue

	// *** Basic block 13

	beqz        a0, .ReduceNodeStrength_label_115

	// *** Basic block 14

	mv          a3, s9
	mv          a2, s1
	mv          a1, s3
	mv          a0, s2
	call        BasicBlockReplaceInstruction

	// *** Basic block 15

.ReduceNodeStrength_label_115:

	// *** Basic block 16

.ReduceNodeStrength_label_116:
	j           .ReduceNodeStrength_label_487

	// *** Basic block 17

.ReduceNodeStrength_label_118:
	ld          s8, 24(s1)
	ld          a0, 8(s8)
	mv          a1, x0
	call        IsIntConstantWithValue

	// *** Basic block 18

	beqz        a0, .ReduceNodeStrength_label_138

	// *** Basic block 19

	ld          a3, 0(s8)
	mv          a2, s1
	mv          a1, s3
	mv          a0, s2
	call        BasicBlockReplaceInstruction

	// *** Basic block 20

	j           .ReduceNodeStrength_label_154

	// *** Basic block 21

.ReduceNodeStrength_label_138:
	ld          a0, 0(s8)
	mv          a1, x0
	call        IsIntConstantWithValue

	// *** Basic block 22

	beqz        a0, .ReduceNodeStrength_label_153

	// *** Basic block 23

	li          t0, 61		// 0x3d ASCII '='
	sw          t0, 20(s1)
	mv          a1, x0
	mv          a0, s1
	call        IRRemoveInput

	// *** Basic block 24

.ReduceNodeStrength_label_153:

	// *** Basic block 25

.ReduceNodeStrength_label_154:
	j           .ReduceNodeStrength_label_487

	// *** Basic block 26

.ReduceNodeStrength_label_156:
	ld          s6, 24(s1)
	ld          s7, 0(s6)
	mv          a1, x0
	mv          a0, s7
	call        IsIntConstantWithValue

	// *** Basic block 27

	beqz        a0, .ReduceNodeStrength_label_176

	// *** Basic block 28

	mv          a3, s7
	mv          a2, s1
	mv          a1, s3
	mv          a0, s2
	call        BasicBlockReplaceInstruction

	// *** Basic block 29

	j           .ReduceNodeStrength_label_321

	// *** Basic block 30

.ReduceNodeStrength_label_176:
	ld          s6, 8(s6)
	mv          a1, x0
	mv          a0, s6
	call        IsIntConstantWithValue

	// *** Basic block 31

	beqz        a0, .ReduceNodeStrength_label_195

	// *** Basic block 32

	mv          a3, s6
	mv          a2, s1
	mv          a1, s3
	mv          a0, s2
	call        BasicBlockReplaceInstruction

	// *** Basic block 33

	j           .ReduceNodeStrength_label_320

	// *** Basic block 34

.ReduceNodeStrength_label_195:
	li          s8, 1		// 0x1 ASCII \x1
	mv          a1, s8
	mv          a0, s7
	call        IsIntConstantWithValue

	// *** Basic block 35

	beqz        a0, .ReduceNodeStrength_label_213

	// *** Basic block 36

	mv          a3, s6
	mv          a2, s1
	mv          a1, s3
	mv          a0, s2
	call        BasicBlockReplaceInstruction

	// *** Basic block 37

	j           .ReduceNodeStrength_label_319

	// *** Basic block 38

.ReduceNodeStrength_label_213:
	mv          a1, s8
	mv          a0, s6
	call        IsIntConstantWithValue

	// *** Basic block 39

	beqz        a0, .ReduceNodeStrength_label_231

	// *** Basic block 40

	mv          a3, s7
	mv          a2, s1
	mv          a1, s3
	mv          a0, s2
	call        BasicBlockReplaceInstruction

	// *** Basic block 41

	j           .ReduceNodeStrength_label_318

	// *** Basic block 42

.ReduceNodeStrength_label_231:
	ld          t0, 80(s1)
	lw          t0, 20(t0)
	slli        s9, t0, 3
	mv          a1, s9
	mv          a0, s7
	call        IsIntConstantPowerOf2

	// *** Basic block 43

	beqz        a0, .ReduceNodeStrength_label_279

	// *** Basic block 44

	li          t0, 54		// 0x36 ASCII '6'
	sw          t0, 20(s1)
	ld          t0, 24(s1)
	ld          s7, 0(t0)
	ld          s7, 80(s7)
	mv          a0, s7
	call        LogBase2

	// *** Basic block 45

	mv          a2, a0
	mv          a1, s7
	mv          a0, s2
	call        GeneratorGetIntConstant

	// *** Basic block 46

	mv          s7, a0
	mv          a1, x0
	mv          a0, s1
	call        IRRemoveInput

	// *** Basic block 47

	mv          a2, x0
	mv          a1, s7
	mv          a0, s1
	call        IRAddInput

	// *** Basic block 48

	j           .ReduceNodeStrength_label_317

	// *** Basic block 49

.ReduceNodeStrength_label_279:
	mv          a1, s9
	mv          a0, s6
	call        IsIntConstantPowerOf2

	// *** Basic block 50

	beqz        a0, .ReduceNodeStrength_label_316

	// *** Basic block 51

	li          t0, 54		// 0x36 ASCII '6'
	sw          t0, 20(s1)
	ld          t0, 24(s1)
	ld          s6, 8(t0)
	ld          s6, 80(s6)
	mv          a0, s6
	call        LogBase2

	// *** Basic block 52

	mv          a2, a0
	mv          a1, s6
	mv          a0, s2
	call        GeneratorGetIntConstant

	// *** Basic block 53

	mv          s6, a0
	mv          a2, s6
	mv          a1, s8
	mv          a0, s1
	call        IRReplaceInput

	// *** Basic block 54

.ReduceNodeStrength_label_316:

	// *** Basic block 55

.ReduceNodeStrength_label_317:

	// *** Basic block 56

.ReduceNodeStrength_label_318:

	// *** Basic block 57

.ReduceNodeStrength_label_319:

	// *** Basic block 58

.ReduceNodeStrength_label_320:

	// *** Basic block 59

.ReduceNodeStrength_label_321:
	j           .ReduceNodeStrength_label_487

	// *** Basic block 60

.ReduceNodeStrength_label_323:
	ld          s5, 24(s1)
	ld          s6, 8(s5)
	li          s7, 1		// 0x1 ASCII \x1
	mv          a1, s7
	mv          a0, s6
	call        IsIntConstantWithValue

	// *** Basic block 61

	beqz        a0, .ReduceNodeStrength_label_345

	// *** Basic block 62

	ld          a3, 0(s5)
	mv          a2, s1
	mv          a1, s3
	mv          a0, s2
	call        BasicBlockReplaceInstruction

	// *** Basic block 63

	j           .ReduceNodeStrength_label_418

	// *** Basic block 64

.ReduceNodeStrength_label_345:
	ld          s5, 0(s5)
	mv          a1, x0
	mv          a0, s5
	call        IsIntConstantWithValue

	// *** Basic block 65

	beqz        a0, .ReduceNodeStrength_label_363

	// *** Basic block 66

	mv          a3, s5
	mv          a2, s1
	mv          a1, s3
	mv          a0, s2
	call        BasicBlockReplaceInstruction

	// *** Basic block 67

	j           .ReduceNodeStrength_label_417

	// *** Basic block 68

.ReduceNodeStrength_label_363:
	ld          s5, 80(s1)
	lw          t0, 20(s5)
	slli        s8, t0, 3
	mv          a1, s8
	mv          a0, s6
	call        IsIntConstantPowerOf2

	// *** Basic block 69

	beqz        a0, .ReduceNodeStrength_label_416

	// *** Basic block 70

	mv          a0, s5
	call        TypeIsUnsigned

	// *** Basic block 71

	beqz        a0, .ReduceNodeStrength_label_385

	// *** Basic block 72

	li          s6, 52		// 0x34 ASCII '4'
	j           .ReduceNodeStrength_label_387

	// *** Basic block 73

.ReduceNodeStrength_label_385:
	li          s6, 53		// 0x35 ASCII '5'

	// *** Basic block 74

.ReduceNodeStrength_label_387:
	sw          s6, 20(s1)
	ld          t0, 24(s1)
	ld          s5, 8(t0)
	ld          s5, 80(s5)
	mv          a0, s5
	call        LogBase2

	// *** Basic block 75

	mv          a2, a0
	mv          a1, s5
	mv          a0, s2
	call        GeneratorGetIntConstant

	// *** Basic block 76

	mv          s5, a0
	mv          a2, s5
	mv          a1, s7
	mv          a0, s1
	call        IRReplaceInput

	// *** Basic block 77

.ReduceNodeStrength_label_416:

	// *** Basic block 78

.ReduceNodeStrength_label_417:

	// *** Basic block 79

.ReduceNodeStrength_label_418:
	j           .ReduceNodeStrength_label_487

	// *** Basic block 80

.ReduceNodeStrength_label_420:
	ld          s4, 24(s1)
	ld          s5, 0(s4)
	mv          a1, x0
	mv          a0, s5
	call        IsIntConstantWithValue

	// *** Basic block 81

	beqz        a0, .ReduceNodeStrength_label_440

	// *** Basic block 82

	mv          a3, s5
	mv          a2, s1
	mv          a1, s3
	mv          a0, s2
	call        BasicBlockReplaceInstruction

	// *** Basic block 83

	j           .ReduceNodeStrength_label_485

	// *** Basic block 84

.ReduceNodeStrength_label_440:
	ld          t0, 80(s1)
	lw          t0, 20(t0)
	slli        s5, t0, 3
	ld          a0, 8(s4)
	mv          a1, s5
	call        IsIntConstantPowerOf2

	// *** Basic block 85

	beqz        a0, .ReduceNodeStrength_label_484

	// *** Basic block 86

	li          t0, 56		// 0x38 ASCII '8'
	sw          t0, 20(s1)
	ld          t0, 24(s1)
	ld          s4, 8(t0)
	ld          s5, 80(s4)
	mv          a0, s4
	call        BitMask

	// *** Basic block 87

	mv          a2, a0
	mv          a1, s5
	mv          a0, s2
	call        GeneratorGetIntConstant

	// *** Basic block 88

	mv          s4, a0
	mv          a2, s4
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        IRReplaceInput

	// *** Basic block 89

.ReduceNodeStrength_label_484:

	// *** Basic block 90

.ReduceNodeStrength_label_485:
	j           .ReduceNodeStrength_label_487

	// *** Basic block 91

.ReduceNodeStrength_label_487:
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
.func_end_ReduceNodeStrength:
	.size ReduceNodeStrength, .func_end_ReduceNodeStrength-ReduceNodeStrength

	.global StrengthReductionOptimization
	.type StrengthReductionOptimization, @function

StrengthReductionOptimization:

	// *** Basic block 0

	.global BasicBlockBegin
	.global BasicBlockIsEmpty
	.global BasicBlockEnd
	.global IRNext
	.local ReduceNodeStrength
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
	addi        t0, s1, 168
	ld          s3, 8(t0)
	bge         x0, s3, .StrengthReductionOptimization_label_83

	// *** Basic block 1

	ld          s4, 168(s1)

	// *** Basic block 2

.StrengthReductionOptimization_label_24:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	mv          a0, s4
	call        BasicBlockBegin

	// *** Basic block 3

	mv          s5, a0
	mv          a0, s4
	call        BasicBlockIsEmpty

	// *** Basic block 4

	not         s6, a0
	beqz        s6, .StrengthReductionOptimization_label_46

	// *** Basic block 5

	mv          a0, s4
	call        BasicBlockEnd

	// *** Basic block 6

	sub         t0, s5, a0
	snez        s6, t0

	// *** Basic block 7

.StrengthReductionOptimization_label_46:
	beqz        s6, .StrengthReductionOptimization_label_78

	// *** Basic block 8

.StrengthReductionOptimization_label_48:
	mv          a0, s5
	call        IRNext

	// *** Basic block 9

	mv          s6, a0
	mv          a2, s5
	mv          a1, s4
	mv          a0, s1
	call        ReduceNodeStrength

	// *** Basic block 10

.StrengthReductionOptimization_label_63:
	mv          s5, s6
	mv          a0, s4
	call        BasicBlockIsEmpty

	// *** Basic block 11

	not         s7, a0
	beqz        s7, .StrengthReductionOptimization_label_76

	// *** Basic block 12

	mv          a0, s4
	call        BasicBlockEnd

	// *** Basic block 13

	sub         t0, s5, a0
	snez        s7, t0

	// *** Basic block 14

.StrengthReductionOptimization_label_76:
	beqz        s7, .StrengthReductionOptimization_label_48

	// *** Basic block 15

.StrengthReductionOptimization_label_78:

	// *** Basic block 16

.StrengthReductionOptimization_label_79:
	addi        s2, s2, 1
	bge         s2, s3, .StrengthReductionOptimization_label_24

	// *** Basic block 17

.StrengthReductionOptimization_label_83:
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
.func_end_StrengthReductionOptimization:
	.size StrengthReductionOptimization, .func_end_StrengthReductionOptimization-StrengthReductionOptimization

	.global FindTailCalls
	.type FindTailCalls, @function

FindTailCalls:

	// *** Basic block 0

	.global VectorGet
	.global BasicBlockRBegin
	.global BasicBlockIsEmpty
	.global BasicBlockREnd
	.global IRIsResult
	.global IRPrev
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
	lb          t0, 236(s1)
	not         t0, t0
	beqz        t0, .FindTailCalls_label_62

	// *** Basic block 1

	addi        t0, s1, 32
	ld          t0, 8(t0)
	li          t1, 1		// 0x1 ASCII \x1
	beq         t0, t1, .FindTailCalls_label_45

	// *** Basic block 2

.FindTailCalls_label_42:
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

	// *** Basic block 3

.FindTailCalls_label_45:
	ld          t0, 32(s1)
	ld          s3, 0(t0)
	addi        a0, s2, 168
	mv          a1, s3
	call        VectorGet

	// *** Basic block 4

	mv          s3, a0
	lb          t0, 236(s3)
	not         t0, t0
	beqz        t0, .FindTailCalls_label_61

	// *** Basic block 5

	j           .FindTailCalls_label_42

	// *** Basic block 6

.FindTailCalls_label_61:

	// *** Basic block 7

.FindTailCalls_label_62:
	mv          a0, s1
	call        BasicBlockRBegin

	// *** Basic block 8

	mv          s4, a0
	mv          a0, s1
	call        BasicBlockIsEmpty

	// *** Basic block 9

	not         s5, a0
	beqz        s5, .FindTailCalls_label_79

	// *** Basic block 10

	mv          a0, s1
	call        BasicBlockREnd

	// *** Basic block 11

	sub         t0, s4, a0
	snez        s5, t0

	// *** Basic block 12

.FindTailCalls_label_79:
	beqz        s5, .FindTailCalls_label_153

	// *** Basic block 13

.FindTailCalls_label_81:
	mv          a0, s4
	call        IRIsResult

	// *** Basic block 14

	beqz        a0, .FindTailCalls_label_103

	// *** Basic block 15

	ld          t0, 24(s4)
	ld          s5, 0(t0)
	lw          t0, 20(s5)
	li          t1, 92		// 0x5c ASCII '\'
	bne         t0, t1, .FindTailCalls_label_101

	// *** Basic block 16

	lw          t0, 88(s5)
	ori         t0, t0, 4
	sw          t0, 88(s5)

	// *** Basic block 17

.FindTailCalls_label_101:
	j           .FindTailCalls_label_42

	// *** Basic block 18

.FindTailCalls_label_103:
	lw          t0, 20(s4)
	li          t1, 120		// 0x78 ASCII 'x'
	bne         t0, t1, .FindTailCalls_label_124

	// *** Basic block 19

	ld          t1, 24(s4)
	ld          s5, 0(t1)
	lw          t1, 20(s5)
	li          t2, 102		// 0x66 ASCII 'f'
	bne         t1, t2, .FindTailCalls_label_123

	// *** Basic block 20

	j           .FindTailCalls_label_42

	// *** Basic block 21

.FindTailCalls_label_123:

	// *** Basic block 22

.FindTailCalls_label_124:
	li          t1, 92		// 0x5c ASCII '\'
	bne         t0, t1, .FindTailCalls_label_134

	// *** Basic block 23

	lw          t0, 88(s4)
	ori         t0, t0, 4
	sw          t0, 88(s4)
	j           .FindTailCalls_label_42

	// *** Basic block 24

.FindTailCalls_label_134:

	// *** Basic block 25

.FindTailCalls_label_135:
	mv          a0, s4
	call        IRPrev

	// *** Basic block 26

	mv          s4, a0
	mv          a0, s1
	call        BasicBlockIsEmpty

	// *** Basic block 27

	not         s5, a0
	beqz        s5, .FindTailCalls_label_151

	// *** Basic block 28

	mv          a0, s1
	call        BasicBlockREnd

	// *** Basic block 29

	sub         t0, s4, a0
	snez        s5, t0

	// *** Basic block 30

.FindTailCalls_label_151:
	beqz        s5, .FindTailCalls_label_81

	// *** Basic block 31

.FindTailCalls_label_153:
	j           .FindTailCalls_label_42
.func_end_FindTailCalls:
	.size FindTailCalls, .func_end_FindTailCalls-FindTailCalls

	.global TailCallOptimization
	.type TailCallOptimization, @function

TailCallOptimization:

	// *** Basic block 0

	.global BasicBlockTraverseDominatorTree
	.global FindTailCalls
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a1, 192(t0)
	mv          a4, t0
	li          a3, 1		// 0x1 ASCII \x1
	la          a2, FindTailCalls
	j           BasicBlockTraverseDominatorTree
.func_end_TailCallOptimization:
	.size TailCallOptimization, .func_end_TailCallOptimization-TailCallOptimization

.PCend:
	.data
MultiplyDeBruijnBitPosition2:
	.type   MultiplyDeBruijnBitPosition2,@object
	.local  MultiplyDeBruijnBitPosition2
	.size   MultiplyDeBruijnBitPosition2,128
	.p2align  2
	.word   0
	.word   1
	.word   28
	.word   2
	.word   29
	.word   14
	.word   24
	.word   3
	.word   30
	.word   22
	.word   20
	.word   15
	.word   25
	.word   17
	.word   4
	.word   8
	.word   31
	.word   27
	.word   13
	.word   23
	.word   21
	.word   19
	.word   16
	.word   7
	.word   26
	.word   12
	.word   18
	.word   6
	.word   11
	.word   5
	.word   10
	.word   9

	.section ".rodata", "aMS", @progbits
