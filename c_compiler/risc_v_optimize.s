	.file   "risc_v_optimize.c"
	.text
	.option pic
.PCbegin:
	.local  Trap
	.type Trap, @function

Trap:

	// *** Basic block 0

	ret         
.func_end_Trap:
	.size Trap, .func_end_Trap-Trap

	.local  TrapRemoveInstruction
	.type TrapRemoveInstruction, @function

TrapRemoveInstruction:

	// *** Basic block 0

	.local Trap
	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	li          t1, 53		// 0x35 ASCII '5'
	bne         t0, t1, .TrapRemoveInstruction_label_18

	// *** Basic block 1

	j           Trap

	// *** Basic block 2

.TrapRemoveInstruction_label_18:
	ret         
.func_end_TrapRemoveInstruction:
	.size TrapRemoveInstruction, .func_end_TrapRemoveInstruction-TrapRemoveInstruction

	.local  TrapRemoveInstructionBlock
	.type TrapRemoveInstructionBlock, @function

TrapRemoveInstructionBlock:

	// *** Basic block 0

	.local Trap
	// Leaf procedure, no stack frame generated
	ld          t0, 0(a0)
	li          t1, 53		// 0x35 ASCII '5'
	bne         t0, t1, .TrapRemoveInstructionBlock_label_17

	// *** Basic block 1

	j           Trap

	// *** Basic block 2

.TrapRemoveInstructionBlock_label_17:
	ret         
.func_end_TrapRemoveInstructionBlock:
	.size TrapRemoveInstructionBlock, .func_end_TrapRemoveInstructionBlock-TrapRemoveInstructionBlock

	.local  TrapCombineLoadStore
	.type TrapCombineLoadStore, @function

TrapCombineLoadStore:

	// *** Basic block 0

	.local Trap
	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	li          t1, 480		// 0x1e0
	bne         t0, t1, .TrapCombineLoadStore_label_18

	// *** Basic block 1

	j           Trap

	// *** Basic block 2

.TrapCombineLoadStore_label_18:
	ret         
.func_end_TrapCombineLoadStore:
	.size TrapCombineLoadStore, .func_end_TrapCombineLoadStore-TrapCombineLoadStore

	.local  TrapPropagateZero
	.type TrapPropagateZero, @function

TrapPropagateZero:

	// *** Basic block 0

	.local Trap
	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	li          t1, 480		// 0x1e0
	bne         t0, t1, .TrapPropagateZero_label_18

	// *** Basic block 1

	j           Trap

	// *** Basic block 2

.TrapPropagateZero_label_18:
	ret         
.func_end_TrapPropagateZero:
	.size TrapPropagateZero, .func_end_TrapPropagateZero-TrapPropagateZero

	.local  TrapPoolConstant
	.type TrapPoolConstant, @function

TrapPoolConstant:

	// *** Basic block 0

	.local Trap
	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	li          t1, 480		// 0x1e0
	bne         t0, t1, .TrapPoolConstant_label_18

	// *** Basic block 1

	j           Trap

	// *** Basic block 2

.TrapPoolConstant_label_18:
	ret         
.func_end_TrapPoolConstant:
	.size TrapPoolConstant, .func_end_TrapPoolConstant-TrapPoolConstant

	.local  RemoveBlockUnusedExpressions
	.type RemoveBlockUnusedExpressions, @function

RemoveBlockUnusedExpressions:

	// *** Basic block 0

	.global BitSetCopy
	.local TrapRemoveInstructionBlock
	.global TargetBasicBlockRBegin
	.global TargetBasicBlockIsEmpty
	.global TargetBasicBlockREnd
	.global TargetPrev
	.local TrapRemoveInstruction
	.global RVIsExpression
	.global RVIsSymbol
	.global RVIsConst
	.global RVIsFixedRegister
	.global BitSetContains
	.global RVIsResult
	.global TargetBasicBlockRemoveInstruction
	.global RVIsArgRegister
	.global TargetRetargetInstruction
	.global BitSetInsert
	.global BitSetDestruct
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
	mv          t0, a1
	mv          s1, a0
	mv          t1, t0
	ld          s2, 0(t1)
	sd          x0, -32(s0)
	sd          x0, -24(s0)
	sd          x0, -32(s0)
	addi        a0, s0, -32
	addi        a1, s1, 192
	call        BitSetCopy

	// *** Basic block 1

	mv          a0, s1
	call        TrapRemoveInstructionBlock

	// *** Basic block 2

	mv          s3, x0
	mv          a0, s1
	call        TargetBasicBlockRBegin

	// *** Basic block 3

	mv          s4, a0
	mv          a0, s1
	call        TargetBasicBlockIsEmpty

	// *** Basic block 4

	not         s5, a0
	beqz        s5, .RemoveBlockUnusedExpressions_label_74

	// *** Basic block 5

	mv          a0, s1
	call        TargetBasicBlockREnd

	// *** Basic block 6

	sub         t0, s4, a0
	snez        s5, t0

	// *** Basic block 7

.RemoveBlockUnusedExpressions_label_74:
	beqz        s5, .RemoveBlockUnusedExpressions_label_326

	// *** Basic block 8

.RemoveBlockUnusedExpressions_label_76:
	mv          a0, s4
	call        TargetPrev

	// *** Basic block 9

	addi        s5, s4, 40
	mv          s3, a0
	mv          a0, s4
	call        TrapRemoveInstruction

	// *** Basic block 10

	lw          s6, 16(s4)
	mv          a0, s6
	call        RVIsExpression

	// *** Basic block 11

	mv          s7, a0
	beqz        a0, .RemoveBlockUnusedExpressions_label_101

	// *** Basic block 12

	mv          a0, s6
	call        RVIsSymbol

	// *** Basic block 13

	not         s7, a0

	// *** Basic block 14

.RemoveBlockUnusedExpressions_label_101:
	beqz        s7, .RemoveBlockUnusedExpressions_label_107

	// *** Basic block 15

	mv          a0, s6
	call        RVIsConst

	// *** Basic block 16

	not         s7, a0

	// *** Basic block 17

.RemoveBlockUnusedExpressions_label_107:
	beqz        s7, .RemoveBlockUnusedExpressions_label_112

	// *** Basic block 18

	addi        t0, s6, -4
	snez        s7, t0

	// *** Basic block 19

.RemoveBlockUnusedExpressions_label_112:
	beqz        s7, .RemoveBlockUnusedExpressions_label_116

	// *** Basic block 20

	addi        t0, s6, -24
	snez        s7, t0

	// *** Basic block 21

.RemoveBlockUnusedExpressions_label_116:
	beqz        s7, .RemoveBlockUnusedExpressions_label_194

	// *** Basic block 22

	ld          s7, 24(s4)
	li          s8, 1		// 0x1 ASCII \x1
	sub         t0, s7, x0
	snez        s9, t0
	beq         s7, x0, .RemoveBlockUnusedExpressions_label_155

	// *** Basic block 23

	lw          a0, 16(s7)
	addi        t0, a0, -4
	seqz        s9, t0
	li          t0, 4		// 0x4 ASCII \x4
	beq         a0, t0, .RemoveBlockUnusedExpressions_label_139

	// *** Basic block 24

	call        RVIsFixedRegister

	// *** Basic block 25

	mv          s9, a0

	// *** Basic block 26

.RemoveBlockUnusedExpressions_label_139:
	bnez        s9, .RemoveBlockUnusedExpressions_label_148

	// *** Basic block 27

	addi        a0, s0, -32
	lw          a1, 20(s7)
	call        BitSetContains

	// *** Basic block 28

	mv          s9, a0

	// *** Basic block 29

.RemoveBlockUnusedExpressions_label_148:
	bnez        s9, .RemoveBlockUnusedExpressions_label_154

	// *** Basic block 30

	call        RVIsResult

	// *** Basic block 32

.RemoveBlockUnusedExpressions_label_154:

	// *** Basic block 33

.RemoveBlockUnusedExpressions_label_155:
	beqz        s9, .RemoveBlockUnusedExpressions_label_158

	// *** Basic block 34

	mv          s8, x0

	// *** Basic block 35

.RemoveBlockUnusedExpressions_label_158:
	mv          s7, s8
	beqz        s8, .RemoveBlockUnusedExpressions_label_166

	// *** Basic block 36

	addi        t0, s6, -4
	snez        s7, t0

	// *** Basic block 37

.RemoveBlockUnusedExpressions_label_166:
	beqz        s7, .RemoveBlockUnusedExpressions_label_172

	// *** Basic block 38

	mv          a0, s6
	call        RVIsFixedRegister

	// *** Basic block 39

	not         s7, a0

	// *** Basic block 40

.RemoveBlockUnusedExpressions_label_172:
	beqz        s7, .RemoveBlockUnusedExpressions_label_181

	// *** Basic block 41

	addi        a0, s0, -32
	lw          a1, 20(s4)
	call        BitSetContains

	// *** Basic block 42

	not         s7, a0

	// *** Basic block 43

.RemoveBlockUnusedExpressions_label_181:
	beqz        s7, .RemoveBlockUnusedExpressions_label_192

	// *** Basic block 44

	mv          a2, s4
	mv          a1, s1
	mv          a0, s2
	call        TargetBasicBlockRemoveInstruction

	// *** Basic block 45

	j           .RemoveBlockUnusedExpressions_label_311

	// *** Basic block 46

.RemoveBlockUnusedExpressions_label_192:
	j           .RemoveBlockUnusedExpressions_label_286

	// *** Basic block 47

.RemoveBlockUnusedExpressions_label_194:
	li          t0, 18		// 0x12 ASCII \x12
	bne         s6, t0, .RemoveBlockUnusedExpressions_label_285

	// *** Basic block 48

	addi        s6, s4, 40
	ld          s7, 40(s4)
	ld          s7, 24(s4)
	beq         s7, x0, .RemoveBlockUnusedExpressions_label_208

	// *** Basic block 49

.RemoveBlockUnusedExpressions_label_208:
	lw          s9, 16(s7)
	mv          a0, s9
	call        RVIsResult

	// *** Basic block 50

	bnez        a0, .RemoveBlockUnusedExpressions_label_287

	// *** Basic block 51

.RemoveBlockUnusedExpressions_label_215:
	li          t0, 24		// 0x18 ASCII \x18
	beq         s9, t0, .RemoveBlockUnusedExpressions_label_287

	// *** Basic block 52

.RemoveBlockUnusedExpressions_label_220:
	ld          s9, 8(s6)
	addi        a0, s0, -32
	lw          s6, 20(s7)
	mv          a1, s6
	call        BitSetContains

	// *** Basic block 53

	not         t0, a0
	beqz        t0, .RemoveBlockUnusedExpressions_label_241

	// *** Basic block 54

	mv          a2, s4
	mv          a1, s1
	mv          a0, s2
	call        TargetBasicBlockRemoveInstruction

	// *** Basic block 55

	j           .RemoveBlockUnusedExpressions_label_311

	// *** Basic block 56

.RemoveBlockUnusedExpressions_label_241:
	lw          a0, 16(s9)
	call        RVIsArgRegister

	// *** Basic block 57

	mv          s10, a0
	beqz        a0, .RemoveBlockUnusedExpressions_label_257

	// *** Basic block 58

	addi        a0, s0, -32
	lw          a1, 20(s9)
	call        BitSetContains

	// *** Basic block 59

	not         s10, a0

	// *** Basic block 60

.RemoveBlockUnusedExpressions_label_257:
	beqz        s10, .RemoveBlockUnusedExpressions_label_265

	// *** Basic block 61

	addi        a0, s1, 192
	mv          a1, s6
	call        BitSetContains

	// *** Basic block 62

	not         s10, a0

	// *** Basic block 63

.RemoveBlockUnusedExpressions_label_265:
	beqz        s10, .RemoveBlockUnusedExpressions_label_284

	// *** Basic block 64

	mv          a1, s9
	mv          a0, s7
	call        TargetRetargetInstruction

	// *** Basic block 65

	addi        a0, s0, -32
	mv          a1, s6
	call        BitSetInsert

	// *** Basic block 66

	mv          a2, s4
	mv          a1, s1
	mv          a0, s2
	call        TargetBasicBlockRemoveInstruction

	// *** Basic block 67

.RemoveBlockUnusedExpressions_label_284:

	// *** Basic block 68

.RemoveBlockUnusedExpressions_label_285:

	// *** Basic block 69

.RemoveBlockUnusedExpressions_label_286:

	// *** Basic block 70

.RemoveBlockUnusedExpressions_label_287:
	mv          s2, x0

	// *** Basic block 71

.RemoveBlockUnusedExpressions_label_291:
	slli        t0, s2, 3
	add         t0, s5, t0
	ld          s5, 0(t0)
	beq         s5, x0, .RemoveBlockUnusedExpressions_label_304

	// *** Basic block 72

	addi        a0, s0, -32
	lw          a1, 20(s5)
	call        BitSetInsert

	// *** Basic block 73

.RemoveBlockUnusedExpressions_label_304:

	// *** Basic block 74

.RemoveBlockUnusedExpressions_label_305:
	addi        s2, s2, 1
	li          t0, 3		// 0x3 ASCII \x3
	bge         s2, t0, .RemoveBlockUnusedExpressions_label_291

	// *** Basic block 75

.RemoveBlockUnusedExpressions_label_310:

	// *** Basic block 76

.RemoveBlockUnusedExpressions_label_311:
	mv          s4, s3
	mv          a0, s1
	call        TargetBasicBlockIsEmpty

	// *** Basic block 77

	not         s5, a0
	beqz        s5, .RemoveBlockUnusedExpressions_label_324

	// *** Basic block 78

	mv          a0, s1
	call        TargetBasicBlockREnd

	// *** Basic block 79

	sub         t0, s4, a0
	snez        s5, t0

	// *** Basic block 80

.RemoveBlockUnusedExpressions_label_324:
	beqz        s5, .RemoveBlockUnusedExpressions_label_76

	// *** Basic block 81

.RemoveBlockUnusedExpressions_label_326:
	addi        a0, s0, -32
	call        BitSetDestruct

	// *** Basic block 82

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
.func_end_RemoveBlockUnusedExpressions:
	.size RemoveBlockUnusedExpressions, .func_end_RemoveBlockUnusedExpressions-RemoveBlockUnusedExpressions

	.local  RemoveUnusedExpressions
	.type RemoveUnusedExpressions, @function

RemoveUnusedExpressions:

	// *** Basic block 0

	.global TargetTraverseDominatorTree
	.local RemoveBlockUnusedExpressions
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -32(s0)
	// End of stack frame
	mv          t0, a0
	sd          x0, -32(s0)
	sd          t0, -32(s0)
	addi        a3, s0, -32
	li          a2, 1		// 0x1 ASCII \x1
	la          a1, RemoveBlockUnusedExpressions
	call        TargetTraverseDominatorTree

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_RemoveUnusedExpressions:
	.size RemoveUnusedExpressions, .func_end_RemoveUnusedExpressions-RemoveUnusedExpressions

	.local  CombineLoadOrStoresInBlock
	.type CombineLoadOrStoresInBlock, @function

CombineLoadOrStoresInBlock:

	// *** Basic block 0

	.global TargetBasicBlockRBegin
	.global TargetBasicBlockIsEmpty
	.global TargetBasicBlockREnd
	.global TargetPrev
	.local TrapCombineLoadStore
	.global RVIsLoad
	.global compiler
	.global TargetNewInstruction
	.global TargetBasicBlockEmitBefore
	.global TargetRetargetInstruction
	.global TargetReplaceOperand
	.global RVIntValue
	.global RVIsPossibleImmediate
	.global TargetGetIntConstant
	.global TargetBasicBlockRemoveInstruction
	.global RVIsStore
	.global TargetEmitBefore
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
	mv          t0, a1
	ld          s2, 0(t0)
	mv          s3, x0
	call        TargetBasicBlockRBegin

	// *** Basic block 1

	la          t0, compiler
	ld          t0, 0(t0)
	lb          t0, 1230(t0)
	not         t1, t0
	la          t0, compiler
	ld          t0, 0(t0)
	lb          t0, 1230(t0)
	not         t2, t0
	mv          s4, a0
	mv          a0, s1
	call        TargetBasicBlockIsEmpty

	// *** Basic block 2

	not         s5, a0
	beqz        s5, .CombineLoadOrStoresInBlock_label_76

	// *** Basic block 3

	mv          a0, s1
	call        TargetBasicBlockREnd

	// *** Basic block 4

	sub         t0, s4, a0
	snez        s5, t0

	// *** Basic block 5

.CombineLoadOrStoresInBlock_label_76:
	beqz        s5, .CombineLoadOrStoresInBlock_label_359

	// *** Basic block 6

.CombineLoadOrStoresInBlock_label_78:
	mv          a0, s4
	call        TargetPrev

	// *** Basic block 7

	mv          s3, a0
	mv          a0, s4
	call        TrapCombineLoadStore

	// *** Basic block 8

	lw          s5, 16(s4)
	mv          a0, s5
	call        RVIsLoad

	// *** Basic block 9

	beqz        a0, .CombineLoadOrStoresInBlock_label_220

	// *** Basic block 10

	addi        s6, s4, 40
	ld          s7, 40(s4)
	lw          s8, 16(s7)
	addi        t0, s8, -187
	seqz        t1, t0
	li          t0, 187		// 0xbb ASCII \xbb
	bne         s8, t0, .CombineLoadOrStoresInBlock_label_103

	// *** Basic block 11

.CombineLoadOrStoresInBlock_label_103:
	beqz        t1, .CombineLoadOrStoresInBlock_label_153

	// *** Basic block 12

	li          t0, 22		// 0x16 ASCII \x16
	mv          a0, t0
	call        TargetNewInstruction

	// *** Basic block 13

	mv          s9, a0
	li          t0, 65536		// 0x10000
	sw          t0, 104(s9)
	mv          a3, s7
	mv          a2, s9
	mv          a1, s1
	mv          a0, s2
	sd          s2, -24(s0)	// Spilled @47
	call        TargetBasicBlockEmitBefore

	// *** Basic block 14

	li          t0, 36		// 0x24 ASCII '$'
	sw          t0, 16(s7)
	lw          t0, 104(s7)
	li          t1, 16384		// 0x4000
	or          t0, t0, t1
	sw          t0, 104(s7)
	ld          a0, 8(s6)
	mv          a1, s9
	call        TargetRetargetInstruction

	// *** Basic block 15

	mv          a2, s9
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s4
	call        TargetReplaceOperand

	// *** Basic block 16

	lw          t0, 104(s4)
	li          t1, 32768		// 0x8000
	or          t0, t0, t1
	sw          t0, 104(s4)
	j           .CombineLoadOrStoresInBlock_label_218

	// *** Basic block 17

.CombineLoadOrStoresInBlock_label_153:
	li          t0, 53		// 0x35 ASCII '5'
	bne         s8, t0, .CombineLoadOrStoresInBlock_label_217

	// *** Basic block 18

	ld          a0, 8(s6)
	call        RVIntValue

	// *** Basic block 19

	mv          s6, a0
	addi        t0, s7, 40
	ld          a0, 8(t0)
	call        RVIntValue

	// *** Basic block 20

	mv          s8, a0
	add         s10, s6, s8
	mv          a0, s10
	call        RVIsPossibleImmediate

	// *** Basic block 21

	beqz        a0, .CombineLoadOrStoresInBlock_label_216

	// *** Basic block 22

	ld          a2, 40(s7)
	mv          a1, x0
	mv          a0, s4
	call        TargetReplaceOperand

	// *** Basic block 23

	mv          a3, s10
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        TargetGetIntConstant

	// *** Basic block 24

	mv          a2, a0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s4
	call        TargetReplaceOperand

	// *** Basic block 25

	addi        t0, s7, 64
	ld          t0, 8(t0)
	bnez        t0, .CombineLoadOrStoresInBlock_label_215

	// *** Basic block 26

	ld          a1, 96(s7)
	mv          a2, s7
	mv          a0, s2
	call        TargetBasicBlockRemoveInstruction

	// *** Basic block 27

.CombineLoadOrStoresInBlock_label_215:

	// *** Basic block 28

.CombineLoadOrStoresInBlock_label_216:

	// *** Basic block 29

.CombineLoadOrStoresInBlock_label_217:

	// *** Basic block 30

.CombineLoadOrStoresInBlock_label_218:
	j           .CombineLoadOrStoresInBlock_label_343

	// *** Basic block 31

.CombineLoadOrStoresInBlock_label_220:
	mv          a0, s5
	call        RVIsStore

	// *** Basic block 32

	beqz        a0, .CombineLoadOrStoresInBlock_label_342

	// *** Basic block 33

	addi        s5, s4, 40
	ld          s7, 8(s5)
	lw          s10, 16(s7)
	addi        t0, s10, -187
	seqz        t2, t0
	li          t0, 187		// 0xbb ASCII \xbb
	bne         s10, t0, .CombineLoadOrStoresInBlock_label_236

	// *** Basic block 34

.CombineLoadOrStoresInBlock_label_236:
	beqz        t2, .CombineLoadOrStoresInBlock_label_276

	// *** Basic block 35

	li          t0, 22		// 0x16 ASCII \x16
	mv          a0, t0
	call        TargetNewInstruction

	// *** Basic block 36

	mv          s11, a0
	li          t0, 65536		// 0x10000
	sw          t0, 104(s11)
	mv          a2, s7
	mv          a1, s11
	mv          a0, s2
	call        TargetEmitBefore

	// *** Basic block 37

	li          t0, 36		// 0x24 ASCII '$'
	sw          t0, 16(s7)
	lw          t0, 104(s7)
	li          t1, 16384		// 0x4000
	or          t0, t0, t1
	sw          t0, 104(s7)
	mv          a2, s11
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s4
	call        TargetReplaceOperand

	// *** Basic block 38

	lw          t0, 104(s4)
	li          t1, 32768		// 0x8000
	or          t0, t0, t1
	sw          t0, 104(s4)
	j           .CombineLoadOrStoresInBlock_label_341

	// *** Basic block 39

.CombineLoadOrStoresInBlock_label_276:
	li          t0, 53		// 0x35 ASCII '5'
	bne         s10, t0, .CombineLoadOrStoresInBlock_label_340

	// *** Basic block 40

	ld          a0, 16(s5)
	call        RVIntValue

	// *** Basic block 41

	mv          s5, a0
	addi        t0, s7, 40
	ld          a0, 8(t0)
	call        RVIntValue

	// *** Basic block 42

	mv          s10, a0
	add         s2, s5, s10
	mv          a0, s2
	call        RVIsPossibleImmediate

	// *** Basic block 43

	beqz        a0, .CombineLoadOrStoresInBlock_label_339

	// *** Basic block 44

	ld          a2, 40(s7)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s4
	call        TargetReplaceOperand

	// *** Basic block 45

	mv          a3, s2
	li          s2, 2		// 0x2 ASCII \x2
	mv          a2, s2
	mv          a1, x0
	ld          a0, -24(s0)	// Spilled @47
	call        TargetGetIntConstant

	// *** Basic block 46

	mv          a2, a0
	mv          a1, s2
	mv          a0, s4
	call        TargetReplaceOperand

	// *** Basic block 47

	addi        t0, s7, 64
	ld          t0, 8(t0)
	bnez        t0, .CombineLoadOrStoresInBlock_label_338

	// *** Basic block 48

	ld          a1, 96(s7)
	mv          a2, s7
	ld          a0, -24(s0)	// Spilled @47
	call        TargetBasicBlockRemoveInstruction

	// *** Basic block 49

.CombineLoadOrStoresInBlock_label_338:

	// *** Basic block 50

.CombineLoadOrStoresInBlock_label_339:

	// *** Basic block 51

.CombineLoadOrStoresInBlock_label_340:

	// *** Basic block 52

.CombineLoadOrStoresInBlock_label_341:

	// *** Basic block 53

.CombineLoadOrStoresInBlock_label_342:

	// *** Basic block 54

.CombineLoadOrStoresInBlock_label_343:

	// *** Basic block 55

.CombineLoadOrStoresInBlock_label_344:
	mv          s4, s3
	mv          a0, s1
	call        TargetBasicBlockIsEmpty

	// *** Basic block 56

	not         s2, a0
	beqz        s2, .CombineLoadOrStoresInBlock_label_357

	// *** Basic block 57

	mv          a0, s1
	call        TargetBasicBlockREnd

	// *** Basic block 58

	sub         t0, s4, a0
	snez        s2, t0

	// *** Basic block 59

.CombineLoadOrStoresInBlock_label_357:
	beqz        s2, .CombineLoadOrStoresInBlock_label_78

	// *** Basic block 60

.CombineLoadOrStoresInBlock_label_359:
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
.func_end_CombineLoadOrStoresInBlock:
	.size CombineLoadOrStoresInBlock, .func_end_CombineLoadOrStoresInBlock-CombineLoadOrStoresInBlock

	.local  CombineLoadOrStores
	.type CombineLoadOrStores, @function

CombineLoadOrStores:

	// *** Basic block 0

	.global TargetTraverseDominatorTree
	.local CombineLoadOrStoresInBlock
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -32(s0)
	// End of stack frame
	mv          t0, a0
	sd          x0, -32(s0)
	sd          t0, -32(s0)
	addi        a3, s0, -32
	mv          a2, x0
	la          a1, CombineLoadOrStoresInBlock
	call        TargetTraverseDominatorTree

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CombineLoadOrStores:
	.size CombineLoadOrStores, .func_end_CombineLoadOrStores-CombineLoadOrStores

	.local  PropagateZeroesInBlock
	.type PropagateZeroesInBlock, @function

PropagateZeroesInBlock:

	// *** Basic block 0

	.global TargetBasicBlockRBegin
	.global TargetBasicBlockIsEmpty
	.global TargetBasicBlockREnd
	.global TargetPrev
	.local TrapPropagateZero
	.global TargetReplaceOperand
	.global TargetBasicBlockRemoveInstruction
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
	mv          t0, a1
	ld          s2, 0(t0)
	mv          s3, x0
	call        TargetBasicBlockRBegin

	// *** Basic block 1

	mv          s4, a0
	mv          a0, s1
	call        TargetBasicBlockIsEmpty

	// *** Basic block 2

	not         s5, a0
	beqz        s5, .PropagateZeroesInBlock_label_48

	// *** Basic block 3

	mv          a0, s1
	call        TargetBasicBlockREnd

	// *** Basic block 4

	sub         t0, s4, a0
	snez        s5, t0

	// *** Basic block 5

.PropagateZeroesInBlock_label_48:
	beqz        s5, .PropagateZeroesInBlock_label_136

	// *** Basic block 6

.PropagateZeroesInBlock_label_50:
	mv          a0, s4
	call        TargetPrev

	// *** Basic block 7

	addi        s5, s4, 40
	mv          s3, a0
	mv          a0, s4
	call        TrapPropagateZero

	// *** Basic block 8

	mv          s6, x0

	// *** Basic block 9

.PropagateZeroesInBlock_label_62:
	slli        t0, s6, 3
	add         t0, s5, t0
	ld          s5, 0(t0)
	beq         s5, x0, .PropagateZeroesInBlock_label_114

	// *** Basic block 10

	lw          t1, 16(s5)
	addi        t2, t1, -11
	seqz        t0, t2
	li          t2, 11		// 0xb ASCII \xb
	bne         t1, t2, .PropagateZeroesInBlock_label_84

	// *** Basic block 11

	addi        t1, s5, 64
	ld          t1, 8(t1)
	addi        t1, t1, -1
	seqz        t0, t1

	// *** Basic block 12

.PropagateZeroesInBlock_label_84:
	beqz        t0, .PropagateZeroesInBlock_label_113

	// *** Basic block 13

	mv          s7, s5
	ld          s5, 40(s7)
	lw          t0, 16(s5)
	li          t1, 207		// 0xcf ASCII \xcf
	bne         t0, t1, .PropagateZeroesInBlock_label_112

	// *** Basic block 14

	mv          a2, s5
	mv          a1, s6
	mv          a0, s4
	call        TargetReplaceOperand

	// *** Basic block 15

	mv          a2, s7
	mv          a1, s1
	mv          a0, s2
	call        TargetBasicBlockRemoveInstruction

	// *** Basic block 16

.PropagateZeroesInBlock_label_112:

	// *** Basic block 17

.PropagateZeroesInBlock_label_113:

	// *** Basic block 18

.PropagateZeroesInBlock_label_114:

	// *** Basic block 19

.PropagateZeroesInBlock_label_115:
	addi        s6, s6, 1
	li          t0, 3		// 0x3 ASCII \x3
	bge         s6, t0, .PropagateZeroesInBlock_label_62

	// *** Basic block 20

.PropagateZeroesInBlock_label_120:

	// *** Basic block 21

.PropagateZeroesInBlock_label_121:
	mv          s4, s3
	mv          a0, s1
	call        TargetBasicBlockIsEmpty

	// *** Basic block 22

	not         s2, a0
	beqz        s2, .PropagateZeroesInBlock_label_134

	// *** Basic block 23

	mv          a0, s1
	call        TargetBasicBlockREnd

	// *** Basic block 24

	sub         t0, s4, a0
	snez        s2, t0

	// *** Basic block 25

.PropagateZeroesInBlock_label_134:
	beqz        s2, .PropagateZeroesInBlock_label_50

	// *** Basic block 26

.PropagateZeroesInBlock_label_136:
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
.func_end_PropagateZeroesInBlock:
	.size PropagateZeroesInBlock, .func_end_PropagateZeroesInBlock-PropagateZeroesInBlock

	.local  PropagateZeroes
	.type PropagateZeroes, @function

PropagateZeroes:

	// *** Basic block 0

	.global TargetTraverseDominatorTree
	.local PropagateZeroesInBlock
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -32(s0)
	// End of stack frame
	mv          t0, a0
	sd          x0, -32(s0)
	sd          t0, -32(s0)
	addi        a3, s0, -32
	li          a2, 1		// 0x1 ASCII \x1
	la          a1, PropagateZeroesInBlock
	call        TargetTraverseDominatorTree

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PropagateZeroes:
	.size PropagateZeroes, .func_end_PropagateZeroes-PropagateZeroes

	.local  PoolConstantsInBlock
	.type PoolConstantsInBlock, @function

PoolConstantsInBlock:

	// *** Basic block 0

	.global TargetBasicBlockBegin
	.global TargetBasicBlockIsEmpty
	.global TargetBasicBlockEnd
	.global TargetNext
	.local TrapPoolConstant
	.global RVIntValue
	.global MapFind
	.global TargetRetargetInstruction
	.global TargetBasicBlockRemoveInstruction
	.global TargetBasicBlockPropagateExpression
	.global MapInsert
	.global MapClone
	.local PoolConstantsInBlock
	.global MapDestruct
	addi sp, sp, -176
	// Saved return address (offset 168) and frame pointer (offset 160)
	sd ra, 168(sp)
	sd s0, 160(sp)
	addi s0, sp, 176
	// Local vars at offset -80(s0)
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
	ld          s3, 0(s2)
	call        TargetBasicBlockBegin

	// *** Basic block 1

	ld          s4, 424(s3)
	mv          s5, a0
	mv          a0, s1
	call        TargetBasicBlockIsEmpty

	// *** Basic block 2

	not         s6, a0
	beqz        s6, .PoolConstantsInBlock_label_55

	// *** Basic block 3

	mv          a0, s1
	call        TargetBasicBlockEnd

	// *** Basic block 4

	sub         t0, s5, a0
	snez        s6, t0

	// *** Basic block 5

.PoolConstantsInBlock_label_55:
	beqz        s6, .PoolConstantsInBlock_label_164

	// *** Basic block 6

.PoolConstantsInBlock_label_57:
	mv          a0, s5
	call        TargetNext

	// *** Basic block 7

	mv          s6, a0
	mv          a0, s5
	call        TrapPoolConstant

	// *** Basic block 8

	lw          t1, 16(s5)
	addi        t2, t1, -174
	seqz        t0, t2
	li          t2, 174		// 0xae ASCII \xae
	bne         t1, t2, .PoolConstantsInBlock_label_78

	// *** Basic block 9

	ld          t1, 24(s5)
	sub         t1, t1, x0
	seqz        t0, t1

	// *** Basic block 10

.PoolConstantsInBlock_label_78:
	beqz        t0, .PoolConstantsInBlock_label_148

	// *** Basic block 11

	ld          a0, 40(s5)
	call        RVIntValue

	// *** Basic block 12

	mv          s7, a0
	sd          x0, -80(s0)
	sd          s7, -80(s0)
	addi        a0, s2, 8
	ld          a1, -80(s0)
	call        MapFind

	// *** Basic block 13

	mv          s8, a0
	beq         s8, x0, .PoolConstantsInBlock_label_126

	// *** Basic block 14

	mv          a1, s8
	mv          a0, s5
	call        TargetRetargetInstruction

	// *** Basic block 15

	mv          a2, s5
	mv          a1, s1
	mv          a0, s3
	call        TargetBasicBlockRemoveInstruction

	// *** Basic block 16

	ld          t0, 96(s8)
	beq         t0, s1, .PoolConstantsInBlock_label_124

	// *** Basic block 17

	mv          a1, s1
	mv          a0, s8
	call        TargetBasicBlockPropagateExpression

	// *** Basic block 18

.PoolConstantsInBlock_label_124:
	j           .PoolConstantsInBlock_label_149

	// *** Basic block 19

.PoolConstantsInBlock_label_126:
	sd          x0, -72(s0)
	sd          x0, -64(s0)
	sd          s7, -72(s0)
	addi        t0, s0, -72
	sd          s5, 8(t0)
	addi        a0, s2, 8
	addi        sp, sp, -16
	ld          t0, -72(s0)
	sd          t0, 0(sp)
	ld          t0, -64(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	call        MapInsert

	// *** Basic block 21

.PoolConstantsInBlock_label_148:

	// *** Basic block 22

.PoolConstantsInBlock_label_149:
	mv          s5, s6
	mv          a0, s1
	call        TargetBasicBlockIsEmpty

	// *** Basic block 23

	not         s9, a0
	beqz        s9, .PoolConstantsInBlock_label_162

	// *** Basic block 24

	mv          a0, s1
	call        TargetBasicBlockEnd

	// *** Basic block 25

	sub         t0, s5, a0
	snez        s9, t0

	// *** Basic block 26

.PoolConstantsInBlock_label_162:
	beqz        s9, .PoolConstantsInBlock_label_57

	// *** Basic block 27

.PoolConstantsInBlock_label_164:
	mv          s9, x0
	addi        t0, s1, 96
	ld          s10, 8(t0)
	bge         x0, s10, .PoolConstantsInBlock_label_213

	// *** Basic block 28

	ld          s11, 96(s1)

	// *** Basic block 29

.PoolConstantsInBlock_label_173:
	slli        t0, s9, 3
	add         t0, s11, t0
	ld          s11, 0(t0)
	slli        t0, s11, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -24(s0)
	sd          s3, -56(s0)
	addi        t0, s0, -56
	addi        a0, t0, 8
	addi        a1, s2, 8
	call        MapClone

	// *** Basic block 30

	addi        a1, s0, -56
	mv          a0, s4
	call        PoolConstantsInBlock

	// *** Basic block 31

	addi        t0, s0, -56
	addi        a0, t0, 8
	call        MapDestruct

	// *** Basic block 32

.PoolConstantsInBlock_label_209:
	addi        s9, s9, 1
	bge         s9, s10, .PoolConstantsInBlock_label_173

	// *** Basic block 33

.PoolConstantsInBlock_label_213:
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
.func_end_PoolConstantsInBlock:
	.size PoolConstantsInBlock, .func_end_PoolConstantsInBlock-PoolConstantsInBlock

	.local  PoolConstants
	.type PoolConstants, @function

PoolConstants:

	// *** Basic block 0

	.global MapInitForInt64Keys
	.local PoolConstantsInBlock
	.global MapDestruct
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          s1, -64(s0)
	addi        t0, s0, -64
	addi        a0, t0, 8
	call        MapInitForInt64Keys

	// *** Basic block 1

	ld          a0, 448(s1)
	addi        a1, s0, -64
	call        PoolConstantsInBlock

	// *** Basic block 2

	addi        t0, s0, -64
	addi        a0, t0, 8
	call        MapDestruct

	// *** Basic block 3

	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PoolConstants:
	.size PoolConstants, .func_end_PoolConstants-PoolConstants

	.local  EliminateMovesInBlock
	.type EliminateMovesInBlock, @function

EliminateMovesInBlock:

	// *** Basic block 0

	.global TargetBasicBlockBegin
	.global TargetBasicBlockIsEmpty
	.global TargetBasicBlockEnd
	.global TargetNext
	.global TargetPrev
	.global TargetBasicBlockRemoveInstruction
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
	mv          s2, a1
	call        TargetBasicBlockBegin

	// *** Basic block 1

	ld          s3, 56(s1)
	ld          s4, 56(s1)
	mv          s5, a0
	mv          a0, s1
	call        TargetBasicBlockIsEmpty

	// *** Basic block 2

	not         s6, a0
	beqz        s6, .EliminateMovesInBlock_label_45

	// *** Basic block 3

	mv          a0, s1
	call        TargetBasicBlockEnd

	// *** Basic block 4

	sub         t0, s5, a0
	snez        s6, t0

	// *** Basic block 5

.EliminateMovesInBlock_label_45:
	beqz        s6, .EliminateMovesInBlock_label_163

	// *** Basic block 6

.EliminateMovesInBlock_label_47:
	mv          a0, s5
	call        TargetNext

	// *** Basic block 7

	ld          s6, 24(s5)
	mv          s7, a0
	lw          t0, 16(s5)
	li          s8, 18		// 0x12 ASCII \x12
	bne         t0, s8, .EliminateMovesInBlock_label_147

	// *** Basic block 8

	addi        t0, s5, 40
	ld          s9, 40(s5)
	ld          s10, 8(t0)
	mv          a0, s5
	call        TargetPrev

	// *** Basic block 9

	sub         t0, s6, s9
	seqz        t1, t0
	mv          s6, a0
	sub         t0, s6, x0
	snez        s11, t0
	beq         s6, x0, .EliminateMovesInBlock_label_84

	// *** Basic block 10

	mv          a0, s6
	call        TargetNext

	// *** Basic block 11

	sub         t0, a0, s3
	snez        s11, t0

	// *** Basic block 12

.EliminateMovesInBlock_label_84:
	beqz        s11, .EliminateMovesInBlock_label_146

	// *** Basic block 13

.EliminateMovesInBlock_label_86:
	mv          a0, s6
	call        TargetPrev

	// *** Basic block 14

	mv          s3, a0
	ld          t0, 24(s6)
	sub         t1, t0, s10
	seqz        t1, t1
	beq         t0, s10, .EliminateMovesInBlock_label_98

	// *** Basic block 15

.EliminateMovesInBlock_label_98:
	bnez        t1, .EliminateMovesInBlock_label_133

	// *** Basic block 16

.EliminateMovesInBlock_label_100:
	lw          t0, 16(s6)
	bne         t0, s8, .EliminateMovesInBlock_label_131

	// *** Basic block 17

	addi        t0, s6, 40
	ld          s8, 40(s6)
	ld          s11, 8(t0)
	sub         t1, s9, s11
	seqz        t0, t1
	bne         s9, s11, .EliminateMovesInBlock_label_119

	// *** Basic block 18

	sub         t1, s10, s8
	seqz        t0, t1

	// *** Basic block 19

.EliminateMovesInBlock_label_119:
	beqz        t0, .EliminateMovesInBlock_label_130

	// *** Basic block 20

	mv          a2, s5
	mv          a1, s1
	mv          a0, s2
	call        TargetBasicBlockRemoveInstruction

	// *** Basic block 21

	j           .EliminateMovesInBlock_label_146

	// *** Basic block 22

.EliminateMovesInBlock_label_130:

	// *** Basic block 23

.EliminateMovesInBlock_label_131:
	mv          s6, s3

	// *** Basic block 24

.EliminateMovesInBlock_label_133:
	mv          s6, s3
	sub         t0, s6, x0
	snez        s8, t0
	beq         s6, x0, .EliminateMovesInBlock_label_144

	// *** Basic block 25

	mv          a0, s6
	call        TargetNext

	// *** Basic block 26

	sub         t0, a0, s4
	snez        s8, t0

	// *** Basic block 27

.EliminateMovesInBlock_label_144:
	beqz        s8, .EliminateMovesInBlock_label_86

	// *** Basic block 28

.EliminateMovesInBlock_label_146:

	// *** Basic block 29

.EliminateMovesInBlock_label_147:

	// *** Basic block 30

.EliminateMovesInBlock_label_148:
	mv          s5, s7
	mv          a0, s1
	call        TargetBasicBlockIsEmpty

	// *** Basic block 31

	not         s4, a0
	beqz        s4, .EliminateMovesInBlock_label_161

	// *** Basic block 32

	mv          a0, s1
	call        TargetBasicBlockEnd

	// *** Basic block 33

	sub         s7, s5, a0
	snez        s4, s7

	// *** Basic block 34

.EliminateMovesInBlock_label_161:
	beqz        s4, .EliminateMovesInBlock_label_47

	// *** Basic block 35

.EliminateMovesInBlock_label_163:
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
.func_end_EliminateMovesInBlock:
	.size EliminateMovesInBlock, .func_end_EliminateMovesInBlock-EliminateMovesInBlock

	.local  EliminateMoves
	.type EliminateMoves, @function

EliminateMoves:

	// *** Basic block 0

	.local EliminateMovesInBlock
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 448(t0)
	mv          a1, t0
	j           EliminateMovesInBlock
.func_end_EliminateMoves:
	.size EliminateMoves, .func_end_EliminateMoves-EliminateMoves

	.global RVOptimize
	.type RVOptimize, @function

RVOptimize:

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
	.local EliminateMoves
	.local RemoveUnusedExpressions
	.local CombineLoadOrStores
	.local PropagateZeroes
	.local PoolConstants
	.global RVBuildBasicBlockInputsAndOutputs
	mv          s1, a0
	call        EliminateMoves

	// *** Basic block 1

	mv          a0, s1
	call        RemoveUnusedExpressions

	// *** Basic block 2

	mv          a0, s1
	call        CombineLoadOrStores

	// *** Basic block 3

	mv          a0, s1
	call        PropagateZeroes

	// *** Basic block 4

	mv          a0, s1
	call        PoolConstants

	// *** Basic block 5

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           RVBuildBasicBlockInputsAndOutputs
.func_end_RVOptimize:
	.size RVOptimize, .func_end_RVOptimize-RVOptimize

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
