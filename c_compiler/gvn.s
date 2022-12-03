	.file   "gvn.c"
	.text
	.option pic
.PCbegin:
	.global NewValue
	.type NewValue, @function

NewValue:

	// *** Basic block 0

	.global malloc
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
	li          a0, 24		// 0x18 ASCII \x18
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	sd          s1, 0(s4)
	sw          s2, 8(s4)
	sd          s3, 16(s4)
	mv          a0, s4

	// *** Basic block 2

.NewValue_label_29:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewValue:
	.size NewValue, .func_end_NewValue-NewValue

	.global ValueDelete
	.type ValueDelete, @function

ValueDelete:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global free
	j           free
.func_end_ValueDelete:
	.size ValueDelete, .func_end_ValueDelete-ValueDelete

	.global ValueCopy
	.type ValueCopy, @function

ValueCopy:

	// *** Basic block 0

	.global NewValue
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 0(t0)
	lw          a1, 8(t0)
	ld          a2, 16(t0)
	j           NewValue
.func_end_ValueCopy:
	.size ValueCopy, .func_end_ValueCopy-ValueCopy

	.local  HashInstruction
	.type HashInstruction, @function

HashInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	slli        t1, a2, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 1

	j           .HashInstruction_label_23

	// *** Basic block 2

	j           .HashInstruction_label_19

	// *** Basic block 3

.HashInstruction_label_19:
	ld          t1, 0(t0)
	j           .HashInstruction_label_26

	// *** Basic block 4

.HashInstruction_label_23:
	mv          t1, t0
	j           .HashInstruction_label_26

	// *** Basic block 5

.HashInstruction_label_26:
	mv          a0, t1

	// *** Basic block 6

.HashInstruction_label_29:
	ret         
.func_end_HashInstruction:
	.size HashInstruction, .func_end_HashInstruction-HashInstruction

	.local  InsertValue
	.type InsertValue, @function

InsertValue:

	// *** Basic block 0

	.global NewVector
	.global VectorAppend
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
	mv          s1, a2
	mv          s2, a1
	mv          s3, a0
	bne         s3, x0, .InsertValue_label_24

	// *** Basic block 1

	call        NewVector

	// *** Basic block 2

	mv          s3, a0
	sd          s3, 0(s1)

	// *** Basic block 3

.InsertValue_label_24:
	mv          a1, s2
	mv          a0, s3
	call        VectorAppend

	// *** Basic block 4

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 5

.InsertValue_label_32:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InsertValue:
	.size InsertValue, .func_end_InsertValue-InsertValue

	.local  FindValue
	.type FindValue, @function

FindValue:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	bne         t1, x0, .FindValue_label_24

	// *** Basic block 1

	ld          t2, 0(t1)
	mv          a0, x0

	// *** Basic block 2

.FindValue_label_21:
	ret         

	// *** Basic block 3

.FindValue_label_24:
	mv          t2, t0
	mv          t3, x0
	ld          t4, 8(t1)
	bge         x0, t4, .FindValue_label_51

	// *** Basic block 4

.FindValue_label_33:
	slli        t0, t3, 3
	add         t0, t2, t0
	ld          t5, 0(t0)
	ld          t0, 0(t5)
	bne         t2, t0, .FindValue_label_46

	// *** Basic block 5

	mv          a0, t5
	ret         

	// *** Basic block 6

.FindValue_label_46:

	// *** Basic block 7

.FindValue_label_47:
	addi        t3, t3, 1
	bge         t3, t4, .FindValue_label_33

	// *** Basic block 8

.FindValue_label_51:
	mv          a0, x0
	ret         
.func_end_FindValue:
	.size FindValue, .func_end_FindValue-FindValue

	.global ValueSetInit
	.type ValueSetInit, @function

ValueSetInit:

	// *** Basic block 0

	.global HashTableInit
	.local HashInstruction
	.local InsertValue
	.local FindValue
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 1		// 0x1 ASCII \x1
	sw          t1, 88(t0)
	lla         a1, .str.1
	la          a5, FindValue
	la          a4, InsertValue
	la          a3, HashInstruction
	li          a2, 111		// 0x6f ASCII 'o'
	j           HashTableInit
.func_end_ValueSetInit:
	.size ValueSetInit, .func_end_ValueSetInit-ValueSetInit

	.global NewValueSet
	.type NewValueSet, @function

NewValueSet:

	// *** Basic block 0

	.global malloc
	.global ValueSetInit
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	li          a0, 96		// 0x60 ASCII '`'
	call        malloc

	// *** Basic block 1

	mv          s1, a0
	mv          a0, s1
	call        ValueSetInit

	// *** Basic block 2

	mv          a0, s1

	// *** Basic block 3

.NewValueSet_label_16:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewValueSet:
	.size NewValueSet, .func_end_NewValueSet-NewValueSet

	.local  DeleteValueList
	.type DeleteValueList, @function

DeleteValueList:

	// *** Basic block 0

	.global ValueDelete
	.global VectorDelete
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
	mv          t0, s1
	mv          s2, x0
	ld          s3, 8(t0)
	bge         x0, s3, .DeleteValueList_label_34

	// *** Basic block 1

	ld          s4, 0(t0)

	// *** Basic block 2

.DeleteValueList_label_21:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	mv          a0, s4
	call        ValueDelete

	// *** Basic block 3

.DeleteValueList_label_30:
	addi        s2, s2, 1
	bge         s2, s3, .DeleteValueList_label_21

	// *** Basic block 4

.DeleteValueList_label_34:
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorDelete
.func_end_DeleteValueList:
	.size DeleteValueList, .func_end_DeleteValueList-DeleteValueList

	.global ValueSetDelete
	.type ValueSetDelete, @function

ValueSetDelete:

	// *** Basic block 0

	.global HashTableTraverse
	.local DeleteValueList
	.global HashTableDestruct
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
	mv          a2, x0
	la          a1, DeleteValueList
	call        HashTableTraverse

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           HashTableDestruct
.func_end_ValueSetDelete:
	.size ValueSetDelete, .func_end_ValueSetDelete-ValueSetDelete

	.local  CalculateInstructionKey
	.type CalculateInstructionKey, @function

CalculateInstructionKey:

	// *** Basic block 0

	.global printf
	.global abort
	.local CalculateInstructionKey
	.global HashTableSearch
	.global IRIsCommutative
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
	mv          s2, a0
	lw          t0, 20(s1)
	li          s3, 34		// 0x22 ASCII '"'
	blt         t0, s3, .CalculateInstructionKey_label_117

	// *** Basic block 1

	beq         t0, s3, .CalculateInstructionKey_label_200

	// *** Basic block 2

	li          t1, 35		// 0x23 ASCII '#'
	beq         t0, t1, .CalculateInstructionKey_label_201

	// *** Basic block 3

	li          t1, 36		// 0x24 ASCII '$'
	beq         t0, t1, .CalculateInstructionKey_label_202

	// *** Basic block 4

	li          t1, 92		// 0x5c ASCII '\'
	beq         t0, t1, .CalculateInstructionKey_label_194

	// *** Basic block 5

	li          t1, 96		// 0x60 ASCII '`'
	beq         t0, t1, .CalculateInstructionKey_label_187

	// *** Basic block 6

	li          t1, 97		// 0x61 ASCII 'a'
	beq         t0, t1, .CalculateInstructionKey_label_188

	// *** Basic block 7

	li          t1, 98		// 0x62 ASCII 'b'
	beq         t0, t1, .CalculateInstructionKey_label_189

	// *** Basic block 8

	li          t1, 99		// 0x63 ASCII 'c'
	beq         t0, t1, .CalculateInstructionKey_label_190

	// *** Basic block 9

	li          t1, 100		// 0x64 ASCII 'd'
	beq         t0, t1, .CalculateInstructionKey_label_191

	// *** Basic block 10

	li          t1, 101		// 0x65 ASCII 'e'
	beq         t0, t1, .CalculateInstructionKey_label_179

	// *** Basic block 11

	li          t1, 104		// 0x68 ASCII 'h'
	beq         t0, t1, .CalculateInstructionKey_label_192

	// *** Basic block 12

	li          t1, 122		// 0x7a ASCII 'z'
	beq         t0, t1, .CalculateInstructionKey_label_193

	// *** Basic block 13

	j           .CalculateInstructionKey_label_208

	// *** Basic block 14

.CalculateInstructionKey_label_117:
	li          t1, 1		// 0x1 ASCII \x1
	beq         t0, t1, .CalculateInstructionKey_label_195

	// *** Basic block 15

	li          t1, 2		// 0x2 ASCII \x2
	beq         t0, t1, .CalculateInstructionKey_label_181

	// *** Basic block 16

	li          t1, 3		// 0x3 ASCII \x3
	beq         t0, t1, .CalculateInstructionKey_label_182

	// *** Basic block 17

	li          t1, 4		// 0x4 ASCII \x4
	beq         t0, t1, .CalculateInstructionKey_label_180

	// *** Basic block 18

	li          t1, 5		// 0x5 ASCII \x5
	beq         t0, t1, .CalculateInstructionKey_label_183

	// *** Basic block 19

	li          t1, 6		// 0x6 ASCII \x6
	beq         t0, t1, .CalculateInstructionKey_label_185

	// *** Basic block 20

	li          t1, 7		// 0x7 ASCII \x7
	beq         t0, t1, .CalculateInstructionKey_label_184

	// *** Basic block 21

	li          t1, 8		// 0x8 ASCII \x8
	beq         t0, t1, .CalculateInstructionKey_label_186

	// *** Basic block 22

	li          t1, 30		// 0x1e ASCII \x1e
	beq         t0, t1, .CalculateInstructionKey_label_196

	// *** Basic block 23

	li          t1, 31		// 0x1f ASCII \x1f
	beq         t0, t1, .CalculateInstructionKey_label_197

	// *** Basic block 24

	li          t1, 32		// 0x20 ASCII ' '
	beq         t0, t1, .CalculateInstructionKey_label_198

	// *** Basic block 25

	li          t1, 33		// 0x21 ASCII '!'
	beq         t0, t1, .CalculateInstructionKey_label_199

	// *** Basic block 26

	j           .CalculateInstructionKey_label_208

	// *** Basic block 27

.CalculateInstructionKey_label_179:

	// *** Basic block 28

.CalculateInstructionKey_label_180:

	// *** Basic block 29

.CalculateInstructionKey_label_181:

	// *** Basic block 30

.CalculateInstructionKey_label_182:

	// *** Basic block 31

.CalculateInstructionKey_label_183:

	// *** Basic block 32

.CalculateInstructionKey_label_184:

	// *** Basic block 33

.CalculateInstructionKey_label_185:

	// *** Basic block 34

.CalculateInstructionKey_label_186:

	// *** Basic block 35

.CalculateInstructionKey_label_187:

	// *** Basic block 36

.CalculateInstructionKey_label_188:

	// *** Basic block 37

.CalculateInstructionKey_label_189:

	// *** Basic block 38

.CalculateInstructionKey_label_190:

	// *** Basic block 39

.CalculateInstructionKey_label_191:

	// *** Basic block 40

.CalculateInstructionKey_label_192:

	// *** Basic block 41

.CalculateInstructionKey_label_193:

	// *** Basic block 42

.CalculateInstructionKey_label_194:

	// *** Basic block 43

.CalculateInstructionKey_label_195:

	// *** Basic block 44

.CalculateInstructionKey_label_196:

	// *** Basic block 45

.CalculateInstructionKey_label_197:

	// *** Basic block 46

.CalculateInstructionKey_label_198:

	// *** Basic block 47

.CalculateInstructionKey_label_199:

	// *** Basic block 48

.CalculateInstructionKey_label_200:

	// *** Basic block 49

.CalculateInstructionKey_label_201:

	// *** Basic block 50

.CalculateInstructionKey_label_202:
	lw          t1, 16(s1)
	addi        t0, t1, 132
	j           .CalculateInstructionKey_label_210

	// *** Basic block 51

.CalculateInstructionKey_label_208:
	j           .CalculateInstructionKey_label_210

	// *** Basic block 52

.CalculateInstructionKey_label_210:
	mv          s3, t0
	li          t1, 122		// 0x7a ASCII 'z'
	bne         t0, t1, .CalculateInstructionKey_label_222

	// *** Basic block 53

	mv          a0, s3

	// *** Basic block 54

.CalculateInstructionKey_label_219:
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

	// *** Basic block 55

.CalculateInstructionKey_label_222:
	li          t1, 92		// 0x5c ASCII '\'
	bne         t0, t1, .CalculateInstructionKey_label_230

	// *** Basic block 56

	mv          a0, s3
	j           .CalculateInstructionKey_label_219

	// *** Basic block 57

.CalculateInstructionKey_label_230:
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .CalculateInstructionKey_label_238

	// *** Basic block 58

	mv          a0, s3
	j           .CalculateInstructionKey_label_219

	// *** Basic block 59

.CalculateInstructionKey_label_238:
	ld          s4, 80(s1)
	bne         s4, x0, .CalculateInstructionKey_label_248

	// *** Basic block 60

	mv          s5, x0
	j           .CalculateInstructionKey_label_254

	// *** Basic block 61

.CalculateInstructionKey_label_248:
	lw          t0, 12(s4)
	andi        t0, t0, 8
	snez        s5, t0
	j           .CalculateInstructionKey_label_254

	// *** Basic block 62

.CalculateInstructionKey_label_254:
	beqz        s5, .CalculateInstructionKey_label_259

	// *** Basic block 63

	mv          a0, s3
	j           .CalculateInstructionKey_label_219

	// *** Basic block 64

.CalculateInstructionKey_label_259:
	addi        t0, s1, 24
	ld          s4, 8(t0)
	li          t0, 2		// 0x2 ASCII \x2
	bge         s4, t0, .CalculateInstructionKey_label_270

	// *** Basic block 65

	ld          t0, 24(s1)
	j           .CalculateInstructionKey_label_287

	// *** Basic block 66

.CalculateInstructionKey_label_270:
	lla         a0, .str.2
	lla         a1, .str.3
	lla         a3, .str.4
	li          t0, 186		// 0xba ASCII \xba
	mv          a2, t0
	call        printf

	// *** Basic block 67

	call        abort

	// *** Basic block 68

.CalculateInstructionKey_label_287:
	sd          x0, -32(s0)
	sw          x0, -32(s0)
	addi        t0, s0, -32
	sw          x0, 4(t0)
	mv          s6, x0
	bge         x0, s4, .CalculateInstructionKey_label_347

	// *** Basic block 69

.CalculateInstructionKey_label_298:
	slli        t0, s6, 3
	add         t0, t0, t0
	ld          s7, 0(t0)
	mv          a1, s7
	mv          a0, s2
	call        CalculateInstructionKey

	// *** Basic block 70

	mv          s7, a0
	mv          a1, s7
	mv          a0, s2
	call        HashTableSearch

	// *** Basic block 71

	mv          s8, a0
	beq         s8, x0, .CalculateInstructionKey_label_321

	// *** Basic block 72

	j           .CalculateInstructionKey_label_336

	// *** Basic block 73

.CalculateInstructionKey_label_321:
	lla         a0, .str.5
	lla         a1, .str.6
	lla         a3, .str.7
	li          t0, 194		// 0xc2 ASCII \xc2
	mv          a2, t0
	call        printf

	// *** Basic block 74

	call        abort

	// *** Basic block 75

.CalculateInstructionKey_label_336:
	slli        t0, s6, 2
	addi        t1, s0, -32
	add         t0, t1, t0
	lw          t1, 8(s8)
	sw          t1, 0(t0)

	// *** Basic block 76

.CalculateInstructionKey_label_343:
	addi        s6, s6, 1
	bge         s6, s4, .CalculateInstructionKey_label_298

	// *** Basic block 77

.CalculateInstructionKey_label_347:
	mv          a0, s1
	call        IRIsCommutative

	// *** Basic block 78

	beqz        a0, .CalculateInstructionKey_label_364

	// *** Basic block 79

	lw          s4, -32(s0)
	addi        t0, s0, -32
	lw          t0, 4(t0)
	bge         t0, s4, .CalculateInstructionKey_label_363

	// *** Basic block 80

	sw          t0, -32(s0)
	addi        t0, s0, -32
	sw          s4, 4(t0)

	// *** Basic block 81

.CalculateInstructionKey_label_363:

	// *** Basic block 82

.CalculateInstructionKey_label_364:
	lw          t0, -32(s0)
	slli        t0, t0, 20
	or          s3, s3, t0
	addi        t0, s0, -32
	lw          t0, 4(t0)
	slli        t0, t0, 42
	or          s3, s3, t0
	mv          a0, s3
	j           .CalculateInstructionKey_label_219
.func_end_CalculateInstructionKey:
	.size CalculateInstructionKey, .func_end_CalculateInstructionKey-CalculateInstructionKey

	.local  LookupInstruction
	.type LookupInstruction, @function

LookupInstruction:

	// *** Basic block 0

	.local CalculateInstructionKey
	.global HashTableSearch
	.global NewValue
	.global HashTableInsert
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
	call        CalculateInstructionKey

	// *** Basic block 1

	mv          s3, a0
	mv          a1, s3
	mv          a0, s1
	call        HashTableSearch

	// *** Basic block 2

	mv          s4, a0
	beq         s4, x0, .LookupInstruction_label_40

	// *** Basic block 3

	ld          a0, 16(s4)

	// *** Basic block 4

.LookupInstruction_label_37:
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

.LookupInstruction_label_40:
	lw          t0, 88(s1)
	addi        t0, t0, 1
	sw          t0, 88(s1)
	mv          a2, s2
	mv          a1, t0
	mv          a0, s3
	call        NewValue

	// *** Basic block 6

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        HashTableInsert

	// *** Basic block 7

	mv          a0, s2
	j           .LookupInstruction_label_37
.func_end_LookupInstruction:
	.size LookupInstruction, .func_end_LookupInstruction-LookupInstruction

	.local  CopyValueList
	.type CopyValueList, @function

CopyValueList:

	// *** Basic block 0

	.global NewVector
	.global VectorCopy
	.global ValueCopy
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
	call        NewVector

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        VectorCopy

	// *** Basic block 2

	ld          s3, 0(s1)
	mv          s4, x0
	ld          t0, 8(s2)
	bge         x0, t0, .CopyValueList_label_49

	// *** Basic block 3

.CopyValueList_label_31:
	slli        s1, s4, 3
	add         t0, s3, s1
	ld          a0, 0(t0)
	call        ValueCopy

	// *** Basic block 4

	mv          s3, a0
	ld          t0, 0(s2)
	add         t0, t0, s1
	sd          s3, 0(t0)

	// *** Basic block 5

.CopyValueList_label_43:
	addi        s4, s4, 1
	ld          t0, 8(s2)
	bge         s4, t0, .CopyValueList_label_31

	// *** Basic block 6

.CopyValueList_label_49:
	mv          a0, s2

	// *** Basic block 7

.CopyValueList_label_52:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CopyValueList:
	.size CopyValueList, .func_end_CopyValueList-CopyValueList

	.local  PrintValue
	.type PrintValue, @function

PrintValue:

	// *** Basic block 0

	.global printf
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	lla         a0, .str.8
	ld          a1, 0(t0)
	lw          a2, 8(t0)
	ld          t1, 16(t0)
	lw          a3, 16(t1)
	j           printf
.func_end_PrintValue:
	.size PrintValue, .func_end_PrintValue-PrintValue

	.local  PrintValueList
	.type PrintValueList, @function

PrintValueList:

	// *** Basic block 0

	.local PrintValue
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
	mv          s1, x0
	ld          s2, 8(t0)
	bge         x0, s2, .PrintValueList_label_31

	// *** Basic block 1

	ld          s3, 0(t0)

	// *** Basic block 2

.PrintValueList_label_20:
	slli        t0, s1, 3
	add         t0, s3, t0
	ld          a0, 0(t0)
	call        PrintValue

	// *** Basic block 3

.PrintValueList_label_27:
	addi        s1, s1, 1
	bge         s1, s2, .PrintValueList_label_20

	// *** Basic block 4

.PrintValueList_label_31:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PrintValueList:
	.size PrintValueList, .func_end_PrintValueList-PrintValueList

	.local  PrintValueSet
	.type PrintValueSet, @function

PrintValueSet:

	// *** Basic block 0

	.global HashTableTraverse
	.local PrintValueList
	// Leaf procedure, no stack frame generated
	mv          a2, x0
	la          a1, PrintValueList
	j           HashTableTraverse
.func_end_PrintValueSet:
	.size PrintValueSet, .func_end_PrintValueSet-PrintValueSet

	.local  DoLocalValueNumbering
	.type DoLocalValueNumbering, @function

DoLocalValueNumbering:

	// *** Basic block 0

	.global IRPrev
	.global IRNext
	.global IRIsExpression
	.local LookupInstruction
	.global GeneratorReplaceInstruction
	.global BasicBlockRemoveInstruction
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
	mv          s4, x0
	ld          s5, 56(s1)
	sub         t0, s5, x0
	snez        s6, t0
	beq         s5, x0, .DoLocalValueNumbering_label_41

	// *** Basic block 1

	ld          t0, 64(s1)
	sub         t1, t0, x0
	snez        s7, t1
	ld          t1, 64(s1)
	sub         t1, t1, x0
	snez        s6, t1

	// *** Basic block 2

.DoLocalValueNumbering_label_41:
	beqz        s6, .DoLocalValueNumbering_label_50

	// *** Basic block 3

	mv          a0, s5
	call        IRPrev

	// *** Basic block 4

	ld          t0, 64(s1)
	sub         t0, a0, t0
	snez        s6, t0

	// *** Basic block 5

.DoLocalValueNumbering_label_50:
	beqz        s6, .DoLocalValueNumbering_label_110

	// *** Basic block 6

.DoLocalValueNumbering_label_52:
	bne         t0, x0, .DoLocalValueNumbering_label_57

	// *** Basic block 7

	mv          s4, x0
	j           .DoLocalValueNumbering_label_62

	// *** Basic block 8

.DoLocalValueNumbering_label_57:
	mv          a0, s5
	call        IRNext

	// *** Basic block 9

	mv          s4, a0

	// *** Basic block 10

.DoLocalValueNumbering_label_62:
	mv          a0, s5
	call        IRIsExpression

	// *** Basic block 11

	beqz        a0, .DoLocalValueNumbering_label_92

	// *** Basic block 12

	mv          a1, s5
	mv          a0, s2
	call        LookupInstruction

	// *** Basic block 13

	mv          s6, a0
	beq         s6, s5, .DoLocalValueNumbering_label_91

	// *** Basic block 14

	mv          a2, s6
	mv          a1, s5
	mv          a0, s3
	call        GeneratorReplaceInstruction

	// *** Basic block 15

	mv          a2, s5
	mv          a1, s1
	mv          a0, s3
	call        BasicBlockRemoveInstruction

	// *** Basic block 16

.DoLocalValueNumbering_label_91:

	// *** Basic block 17

.DoLocalValueNumbering_label_92:
	mv          s5, s4

	// *** Basic block 18

.DoLocalValueNumbering_label_94:
	mv          s5, s4
	sub         t0, s5, x0
	snez        s7, t0
	beq         s5, x0, .DoLocalValueNumbering_label_101

	// *** Basic block 19

.DoLocalValueNumbering_label_101:
	beqz        s7, .DoLocalValueNumbering_label_108

	// *** Basic block 20

	mv          a0, s5
	call        IRPrev

	// *** Basic block 21

	sub         t0, a0, t0
	snez        s7, t0

	// *** Basic block 22

.DoLocalValueNumbering_label_108:
	beqz        s7, .DoLocalValueNumbering_label_52

	// *** Basic block 23

.DoLocalValueNumbering_label_110:
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
.func_end_DoLocalValueNumbering:
	.size DoLocalValueNumbering, .func_end_DoLocalValueNumbering-DoLocalValueNumbering

	.local  DoGlobalValueNumbering
	.type DoGlobalValueNumbering, @function

DoGlobalValueNumbering:

	// *** Basic block 0

	.global NewValueSet
	.global HashTableCopy
	.local CopyValueList
	.local DoLocalValueNumbering
	.global VectorGet
	.local DoGlobalValueNumbering
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
	ld          s4, 136(s1)
	sub         t0, s4, x0
	snez        t1, t0
	beq         s4, x0, .DoGlobalValueNumbering_label_33

	// *** Basic block 1

	ld          s3, 224(s4)

	// *** Basic block 2

.DoGlobalValueNumbering_label_33:
	mv          s5, x0
	beq         s4, x0, .DoGlobalValueNumbering_label_44

	// *** Basic block 3

	addi        t0, s4, 96
	ld          t0, 8(t0)
	addi        t0, t0, -1
	seqz        t1, t0

	// *** Basic block 4

.DoGlobalValueNumbering_label_44:
	beqz        t1, .DoGlobalValueNumbering_label_54

	// *** Basic block 5

	mv          s5, s3
	ld          t0, 136(s1)
	sd          x0, 224(t0)
	sd          s5, 224(s1)
	j           .DoGlobalValueNumbering_label_76

	// *** Basic block 6

.DoGlobalValueNumbering_label_54:
	call        NewValueSet

	// *** Basic block 7

	mv          s5, a0
	sd          s5, 224(s1)
	beq         s3, x0, .DoGlobalValueNumbering_label_75

	// *** Basic block 8

	la          t0, CopyValueList
	mv          a2, t0
	mv          a1, s3
	mv          a0, s5
	call        HashTableCopy

	// *** Basic block 9

	lw          t0, 88(s3)
	sw          t0, 88(s5)

	// *** Basic block 10

.DoGlobalValueNumbering_label_75:

	// *** Basic block 11

.DoGlobalValueNumbering_label_76:
	mv          a2, s1
	mv          a1, s5
	mv          a0, s2
	call        DoLocalValueNumbering

	// *** Basic block 12

	mv          s4, x0
	addi        t0, s1, 96
	ld          s6, 8(t0)
	bge         x0, s6, .DoGlobalValueNumbering_label_114

	// *** Basic block 13

	ld          s7, 96(s1)

	// *** Basic block 14

.DoGlobalValueNumbering_label_92:
	slli        t0, s4, 3
	add         t0, s7, t0
	ld          s7, 0(t0)
	addi        a0, s2, 168
	mv          a1, s7
	call        VectorGet

	// *** Basic block 15

	mv          s7, a0
	mv          a1, s7
	mv          a0, s2
	call        DoGlobalValueNumbering

	// *** Basic block 16

.DoGlobalValueNumbering_label_110:
	addi        s4, s4, 1
	bge         s4, s6, .DoGlobalValueNumbering_label_92

	// *** Basic block 17

.DoGlobalValueNumbering_label_114:
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
.func_end_DoGlobalValueNumbering:
	.size DoGlobalValueNumbering, .func_end_DoGlobalValueNumbering-DoGlobalValueNumbering

	.global GlobalValueNumberingOptimization
	.type GlobalValueNumberingOptimization, @function

GlobalValueNumberingOptimization:

	// *** Basic block 0

	.local DoGlobalValueNumbering
	.global ValueSetDelete
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
	ld          a1, 192(s1)
	call        DoGlobalValueNumbering

	// *** Basic block 1

	mv          s2, x0
	addi        t0, s1, 168
	ld          s3, 8(t0)
	bge         x0, s3, .GlobalValueNumberingOptimization_label_50

	// *** Basic block 2

	ld          t0, 168(s1)

	// *** Basic block 3

.GlobalValueNumberingOptimization_label_30:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s1, 0(t0)
	ld          a0, 224(s1)
	beq         a0, x0, .GlobalValueNumberingOptimization_label_45

	// *** Basic block 4

	call        ValueSetDelete

	// *** Basic block 5

	sd          x0, 224(s1)

	// *** Basic block 6

.GlobalValueNumberingOptimization_label_45:

	// *** Basic block 7

.GlobalValueNumberingOptimization_label_46:
	addi        s2, s2, 1
	bge         s2, s3, .GlobalValueNumberingOptimization_label_30

	// *** Basic block 8

.GlobalValueNumberingOptimization_label_50:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GlobalValueNumberingOptimization:
	.size GlobalValueNumberingOptimization, .func_end_GlobalValueNumberingOptimization-GlobalValueNumberingOptimization

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "values"
	.type .str.1, @object
	.size .str.1, 7

.str.2:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.2, @object
	.size .str.2, 30

.str.3:
	.asciz "gvn.c"
	.type .str.3, @object
	.size .str.3, 6

.str.4:
	.asciz "inst->inputs.length <= 2"
	.type .str.4, @object
	.size .str.4, 25

.str.5:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.5, @object
	.size .str.5, 30

.str.6:
	.asciz "gvn.c"
	.type .str.6, @object
	.size .str.6, 6

.str.7:
	.asciz "v != NULL"
	.type .str.7, @object
	.size .str.7, 10

.str.8:
	.asciz "key: 0x%" PRIx64 ", value number: %d, node: $%d\n"
	.type .str.8, @object
	.size .str.8, 42

