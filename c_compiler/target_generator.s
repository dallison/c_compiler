	.file   "target_generator.c"
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

	.local  TrapInstruction
	.type TrapInstruction, @function

TrapInstruction:

	// *** Basic block 0

	.local Trap
	// Leaf procedure, no stack frame generated
	lw          t0, 20(a0)
	li          t1, 32		// 0x20 ASCII ' '
	bne         t0, t1, .TrapInstruction_label_18

	// *** Basic block 1

	j           Trap

	// *** Basic block 2

.TrapInstruction_label_18:
	ret         
.func_end_TrapInstruction:
	.size TrapInstruction, .func_end_TrapInstruction-TrapInstruction

	.global TargetPrintInstruction
	.type TargetPrintInstruction, @function

TargetPrintInstruction:

	// *** Basic block 0

	.global fprintf
	.global DecodeSourceLocation
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -48(s0)
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
	mv          s2, a2
	mv          s3, a1
	bne         s1, x0, .TargetPrintInstruction_label_51

	// *** Basic block 1

	addi        t0, s1, 40

	// *** Basic block 2

.TargetPrintInstruction_label_48:
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

.TargetPrintInstruction_label_51:
	lla         s4, .str.1
	lw          s5, 20(s1)
	lw          s6, 16(s1)
	mv          a0, s6
	jalr         x1, s3, 0

	// *** Basic block 4

	mv          a3, a0
	mv          a2, s5
	mv          a1, s4
	mv          a0, s2
	call        fprintf

	// *** Basic block 5

	mv          s4, s1
	li          t0, 2		// 0x2 ASCII \x2
	beq         s6, t0, .TargetPrintInstruction_label_158

	// *** Basic block 6

	li          t0, 5		// 0x5 ASCII \x5
	beq         s6, t0, .TargetPrintInstruction_label_131

	// *** Basic block 7

	li          t0, 6		// 0x6 ASCII \x6
	beq         s6, t0, .TargetPrintInstruction_label_132

	// *** Basic block 8

	li          t0, 7		// 0x7 ASCII \x7
	beq         s6, t0, .TargetPrintInstruction_label_133

	// *** Basic block 9

	li          t0, 8		// 0x8 ASCII \x8
	beq         s6, t0, .TargetPrintInstruction_label_134

	// *** Basic block 10

	li          t0, 9		// 0x9 ASCII \x9
	beq         s6, t0, .TargetPrintInstruction_label_145

	// *** Basic block 11

	li          t0, 10		// 0xa ASCII \xa
	beq         s6, t0, .TargetPrintInstruction_label_146

	// *** Basic block 12

	li          t0, 31		// 0x1f ASCII \x1f
	beq         s6, t0, .TargetPrintInstruction_label_173

	// *** Basic block 13

	li          t0, 33		// 0x21 ASCII '!'
	beq         s6, t0, .TargetPrintInstruction_label_159

	// *** Basic block 14

	li          t0, 34		// 0x22 ASCII '"'
	beq         s6, t0, .TargetPrintInstruction_label_160

	// *** Basic block 15

.TargetPrintInstruction_label_123:
	lla         s3, .str.6
	mv          s5, x0
	j           .TargetPrintInstruction_label_210

	// *** Basic block 16

.TargetPrintInstruction_label_131:

	// *** Basic block 17

.TargetPrintInstruction_label_132:

	// *** Basic block 18

.TargetPrintInstruction_label_133:

	// *** Basic block 19

.TargetPrintInstruction_label_134:
	lla         a1, .str.2
	ld          a2, 120(s4)
	mv          a0, s2
	call        fprintf

	// *** Basic block 20

	j           .TargetPrintInstruction_label_238

	// *** Basic block 21

.TargetPrintInstruction_label_145:

	// *** Basic block 22

.TargetPrintInstruction_label_146:
	lla         a1, .str.3
	fld         fa0, 120(s4)
	mv          a0, s2
	call        fprintf

	// *** Basic block 23

	j           .TargetPrintInstruction_label_238

	// *** Basic block 24

.TargetPrintInstruction_label_158:

	// *** Basic block 25

.TargetPrintInstruction_label_159:

	// *** Basic block 26

.TargetPrintInstruction_label_160:
	lla         a1, .str.4
	ld          t0, 112(s1)
	ld          a2, 16(t0)
	mv          a0, s2
	call        fprintf

	// *** Basic block 27

	j           .TargetPrintInstruction_label_238

	// *** Basic block 28

.TargetPrintInstruction_label_173:
	mv          s3, s1
	ld          a0, 112(s3)
	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	call        DecodeSourceLocation

	// *** Basic block 29

	lla         a1, .str.5
	ld          a2, -48(s0)
	lw          a3, -40(s0)
	lw          a4, -36(s0)
	lw          a5, -32(s0)
	mv          a0, s2
	call        fprintf

	// *** Basic block 30

	j           .TargetPrintInstruction_label_238

	// *** Basic block 31

.TargetPrintInstruction_label_210:
	slli        t0, s5, 3
	add         t0, t0, t0
	ld          s6, 0(t0)
	beq         s6, x0, .TargetPrintInstruction_label_230

	// *** Basic block 32

	lla         a1, .str.7
	lw          a3, 20(s6)
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 34

.TargetPrintInstruction_label_230:

	// *** Basic block 35

.TargetPrintInstruction_label_231:
	addi        s5, s5, 1
	li          t0, 3		// 0x3 ASCII \x3
	bge         s5, t0, .TargetPrintInstruction_label_210

	// *** Basic block 36

.TargetPrintInstruction_label_236:
	j           .TargetPrintInstruction_label_238

	// *** Basic block 37

.TargetPrintInstruction_label_238:
	lla         a1, .str.9
	mv          a0, s2
	call        fprintf

	// *** Basic block 38

	ld          s6, 24(s1)
	beq         s6, x0, .TargetPrintInstruction_label_259

	// *** Basic block 39

	lla         a1, .str.10
	lw          a2, 20(s6)
	mv          a0, s2
	call        fprintf

	// *** Basic block 40

.TargetPrintInstruction_label_259:
	lla         a1, .str.11
	addi        t0, s1, 64
	ld          s6, 8(t0)
	mv          a2, s6
	mv          a0, s2
	call        fprintf

	// *** Basic block 41

	ld          s7, 64(s1)
	bge         x0, s6, .TargetPrintInstruction_label_316

	// *** Basic block 42

	lla         a1, .str.12
	mv          a0, s2
	call        fprintf

	// *** Basic block 43

	lla         s8, .str.13
	mv          s9, x0
	bge         x0, s6, .TargetPrintInstruction_label_309

	// *** Basic block 44

.TargetPrintInstruction_label_287:
	slli        t0, s9, 3
	add         t0, s7, t0
	ld          s7, 0(t0)
	lla         a1, .str.14
	lw          a3, 20(s7)
	mv          a2, s8
	mv          a0, s2
	call        fprintf

	// *** Basic block 46

.TargetPrintInstruction_label_305:
	addi        s9, s9, 1
	bge         s9, s6, .TargetPrintInstruction_label_287

	// *** Basic block 47

.TargetPrintInstruction_label_309:
	lla         a1, .str.16
	mv          a0, s2
	call        fprintf

	// *** Basic block 48

.TargetPrintInstruction_label_316:
	lla         a1, .str.17
	mv          a0, s2
	call        fprintf

	// *** Basic block 49

	j           .TargetPrintInstruction_label_48
.func_end_TargetPrintInstruction:
	.size TargetPrintInstruction, .func_end_TargetPrintInstruction-TargetPrintInstruction

	.global TargetOpcodeName
	.type TargetOpcodeName, @function

TargetOpcodeName:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	slli        t1, t0, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 1

	j           .TargetOpcodeName_label_85

	// *** Basic block 2

	j           .TargetOpcodeName_label_92

	// *** Basic block 3

	j           .TargetOpcodeName_label_97

	// *** Basic block 4

	j           .TargetOpcodeName_label_102

	// *** Basic block 5

	j           .TargetOpcodeName_label_107

	// *** Basic block 6

	j           .TargetOpcodeName_label_112

	// *** Basic block 7

	j           .TargetOpcodeName_label_117

	// *** Basic block 8

	j           .TargetOpcodeName_label_122

	// *** Basic block 9

	j           .TargetOpcodeName_label_127

	// *** Basic block 10

	j           .TargetOpcodeName_label_132

	// *** Basic block 11

	j           .TargetOpcodeName_label_137

	// *** Basic block 12

	j           .TargetOpcodeName_label_142

	// *** Basic block 13

	j           .TargetOpcodeName_label_147

	// *** Basic block 14

	j           .TargetOpcodeName_label_152

	// *** Basic block 15

	j           .TargetOpcodeName_label_157

	// *** Basic block 16

	j           .TargetOpcodeName_label_162

	// *** Basic block 17

	j           .TargetOpcodeName_label_167

	// *** Basic block 18

	j           .TargetOpcodeName_label_172

	// *** Basic block 19

	j           .TargetOpcodeName_label_177

	// *** Basic block 20

	j           .TargetOpcodeName_label_182

	// *** Basic block 21

	j           .TargetOpcodeName_label_187

	// *** Basic block 22

	j           .TargetOpcodeName_label_192

	// *** Basic block 23

	j           .TargetOpcodeName_label_197

	// *** Basic block 24

	j           .TargetOpcodeName_label_202

	// *** Basic block 25

	j           .TargetOpcodeName_label_207

	// *** Basic block 26

	j           .TargetOpcodeName_label_212

	// *** Basic block 27

	j           .TargetOpcodeName_label_217

	// *** Basic block 28

	j           .TargetOpcodeName_label_222

	// *** Basic block 29

	j           .TargetOpcodeName_label_227

	// *** Basic block 30

	j           .TargetOpcodeName_label_232

	// *** Basic block 31

	j           .TargetOpcodeName_label_237

	// *** Basic block 32

	j           .TargetOpcodeName_label_242

	// *** Basic block 33

	j           .TargetOpcodeName_label_247

	// *** Basic block 34

	j           .TargetOpcodeName_label_252

	// *** Basic block 35

	j           .TargetOpcodeName_label_257

	// *** Basic block 36

.TargetOpcodeName_label_85:
	lla         a0, .str.18

	// *** Basic block 37

.TargetOpcodeName_label_89:
	ret         

	// *** Basic block 38

.TargetOpcodeName_label_92:
	lla         a0, .str.19
	ret         

	// *** Basic block 39

.TargetOpcodeName_label_97:
	lla         a0, .str.20
	ret         

	// *** Basic block 40

.TargetOpcodeName_label_102:
	lla         a0, .str.21
	ret         

	// *** Basic block 41

.TargetOpcodeName_label_107:
	lla         a0, .str.22
	ret         

	// *** Basic block 42

.TargetOpcodeName_label_112:
	lla         a0, .str.23
	ret         

	// *** Basic block 43

.TargetOpcodeName_label_117:
	lla         a0, .str.24
	ret         

	// *** Basic block 44

.TargetOpcodeName_label_122:
	lla         a0, .str.25
	ret         

	// *** Basic block 45

.TargetOpcodeName_label_127:
	lla         a0, .str.26
	ret         

	// *** Basic block 46

.TargetOpcodeName_label_132:
	lla         a0, .str.27
	ret         

	// *** Basic block 47

.TargetOpcodeName_label_137:
	lla         a0, .str.28
	ret         

	// *** Basic block 48

.TargetOpcodeName_label_142:
	lla         a0, .str.29
	ret         

	// *** Basic block 49

.TargetOpcodeName_label_147:
	lla         a0, .str.30
	ret         

	// *** Basic block 50

.TargetOpcodeName_label_152:
	lla         a0, .str.31
	ret         

	// *** Basic block 51

.TargetOpcodeName_label_157:
	lla         a0, .str.32
	ret         

	// *** Basic block 52

.TargetOpcodeName_label_162:
	lla         a0, .str.33
	ret         

	// *** Basic block 53

.TargetOpcodeName_label_167:
	lla         a0, .str.34
	ret         

	// *** Basic block 54

.TargetOpcodeName_label_172:
	lla         a0, .str.35
	ret         

	// *** Basic block 55

.TargetOpcodeName_label_177:
	lla         a0, .str.36
	ret         

	// *** Basic block 56

.TargetOpcodeName_label_182:
	lla         a0, .str.37
	ret         

	// *** Basic block 57

.TargetOpcodeName_label_187:
	lla         a0, .str.38
	ret         

	// *** Basic block 58

.TargetOpcodeName_label_192:
	lla         a0, .str.39
	ret         

	// *** Basic block 59

.TargetOpcodeName_label_197:
	lla         a0, .str.40
	ret         

	// *** Basic block 60

.TargetOpcodeName_label_202:
	lla         a0, .str.41
	ret         

	// *** Basic block 61

.TargetOpcodeName_label_207:
	lla         a0, .str.42
	ret         

	// *** Basic block 62

.TargetOpcodeName_label_212:
	lla         a0, .str.43
	ret         

	// *** Basic block 63

.TargetOpcodeName_label_217:
	lla         a0, .str.44
	ret         

	// *** Basic block 64

.TargetOpcodeName_label_222:
	lla         a0, .str.45
	ret         

	// *** Basic block 65

.TargetOpcodeName_label_227:
	lla         a0, .str.46
	ret         

	// *** Basic block 66

.TargetOpcodeName_label_232:
	lla         a0, .str.47
	ret         

	// *** Basic block 67

.TargetOpcodeName_label_237:
	lla         a0, .str.48
	ret         

	// *** Basic block 68

.TargetOpcodeName_label_242:
	lla         a0, .str.49
	ret         

	// *** Basic block 69

.TargetOpcodeName_label_247:
	lla         a0, .str.50
	ret         

	// *** Basic block 70

.TargetOpcodeName_label_252:
	lla         a0, .str.51
	ret         

	// *** Basic block 71

.TargetOpcodeName_label_257:
	lla         a0, .str.52
	ret         
.func_end_TargetOpcodeName:
	.size TargetOpcodeName, .func_end_TargetOpcodeName-TargetOpcodeName

	.global TargetGeneratorInit
	.type TargetGeneratorInit, @function

TargetGeneratorInit:

	// *** Basic block 0

	.global StringInit
	.global StorageIs
	.global GeneratorNumCalls
	.global ListInit
	.global VectorInit
	.local next_instruction_id
	.global NewTypeRecord
	.global NewFunctionTypeRecord
	.global TypeRecordChain
	.global NewSymbol
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
	sd s10, 0(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	ld          t0, 8(s1)
	ld          s3, 32(t0)
	ld          t0, 16(s3)
	mv          a1, t0
	call        StringInit

	// *** Basic block 1

	lw          a0, 48(s3)
	li          s3, 2		// 0x2 ASCII \x2
	mv          a1, s3
	call        StorageIs

	// *** Basic block 2

	not         t1, a0
	sb          t1, 40(s2)
	mv          a0, s1
	call        GeneratorNumCalls

	// *** Basic block 3

	sw          a0, 44(s2)
	addi        t0, t0, 32
	lb          t0, 32(t0)
	sb          t0, 48(s2)
	addi        a0, s2, 56
	call        ListInit

	// *** Basic block 4

	sw          x0, 128(s2)
	sd          x0, 80(s2)
	sd          x0, 88(s2)
	sd          x0, 96(s2)
	addi        a0, s2, 136
	call        VectorInit

	// *** Basic block 5

	sd          x0, 104(s2)
	sd          x0, 112(s2)
	sd          x0, 120(s2)
	la          t0, next_instruction_id
	li          s4, 1		// 0x1 ASCII \x1
	sw          s4, 0(t0)
	mv          a1, x0
	mv          a0, s3
	call        NewTypeRecord

	// *** Basic block 6

	mv          s5, a0
	call        NewFunctionTypeRecord

	// *** Basic block 7

	mv          s6, a0
	addi        t0, s6, 32
	sb          s4, 48(t0)
	mv          a1, s5
	mv          a0, s6
	call        TypeRecordChain

	// *** Basic block 8

	lla         a0, .str.53
	li          s7, 8		// 0x8 ASCII \x8
	mv          a2, s7
	mv          a1, s6
	call        NewSymbol

	// *** Basic block 9

	sd          a0, 160(s2)
	mv          a1, x0
	mv          a0, s3
	call        NewTypeRecord

	// *** Basic block 10

	mv          s8, a0
	call        NewFunctionTypeRecord

	// *** Basic block 11

	mv          s9, a0
	addi        t0, s6, 32
	sb          s4, 48(t0)
	mv          a1, s8
	mv          a0, s9
	call        TypeRecordChain

	// *** Basic block 12

	lla         a0, .str.54
	mv          a2, s7
	mv          a1, s9
	call        NewSymbol

	// *** Basic block 13

	sd          a0, 168(s2)
	mv          a1, x0
	mv          a0, s3
	call        NewTypeRecord

	// *** Basic block 14

	mv          s3, a0
	call        NewFunctionTypeRecord

	// *** Basic block 15

	mv          s10, a0
	addi        t0, s10, 32
	sb          s4, 48(t0)
	mv          a1, s3
	mv          a0, s10
	call        TypeRecordChain

	// *** Basic block 16

	lla         a0, .str.55
	mv          a2, s7
	mv          a1, s10
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
	j           NewSymbol
.func_end_TargetGeneratorInit:
	.size TargetGeneratorInit, .func_end_TargetGeneratorInit-TargetGeneratorInit

	.global TargetGeneratorDestruct
	.type TargetGeneratorDestruct, @function

TargetGeneratorDestruct:

	// *** Basic block 0

	.global ListDestruct
	.global SymbolDelete
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
	addi        a0, s1, 56
	call        ListDestruct

	// *** Basic block 1

	ld          a0, 160(s1)
	call        SymbolDelete

	// *** Basic block 2

	ld          a0, 168(s1)
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SymbolDelete
.func_end_TargetGeneratorDestruct:
	.size TargetGeneratorDestruct, .func_end_TargetGeneratorDestruct-TargetGeneratorDestruct

	.global TargetFirstInstruction
	.type TargetFirstInstruction, @function

TargetFirstInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          a0, 56(a0)

	// *** Basic block 1

.TargetFirstInstruction_label_11:
	ret         
.func_end_TargetFirstInstruction:
	.size TargetFirstInstruction, .func_end_TargetFirstInstruction-TargetFirstInstruction

	.global TargetLastInstruction
	.type TargetLastInstruction, @function

TargetLastInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	addi        t0, a0, 56
	ld          a0, 8(t0)

	// *** Basic block 1

.TargetLastInstruction_label_12:
	ret         
.func_end_TargetLastInstruction:
	.size TargetLastInstruction, .func_end_TargetLastInstruction-TargetLastInstruction

	.global TargetLastConstant
	.type TargetLastConstant, @function

TargetLastConstant:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          a0, 80(a0)

	// *** Basic block 1

.TargetLastConstant_label_10:
	ret         
.func_end_TargetLastConstant:
	.size TargetLastConstant, .func_end_TargetLastConstant-TargetLastConstant

	.global TargetFirstSymbol
	.type TargetFirstSymbol, @function

TargetFirstSymbol:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          a0, 88(a0)

	// *** Basic block 1

.TargetFirstSymbol_label_10:
	ret         
.func_end_TargetFirstSymbol:
	.size TargetFirstSymbol, .func_end_TargetFirstSymbol-TargetFirstSymbol

	.global TargetLastSymbol
	.type TargetLastSymbol, @function

TargetLastSymbol:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          a0, 96(a0)

	// *** Basic block 1

.TargetLastSymbol_label_10:
	ret         
.func_end_TargetLastSymbol:
	.size TargetLastSymbol, .func_end_TargetLastSymbol-TargetLastSymbol

	.global TargetNext
	.type TargetNext, @function

TargetNext:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          a0, 8(a0)

	// *** Basic block 1

.TargetNext_label_11:
	ret         
.func_end_TargetNext:
	.size TargetNext, .func_end_TargetNext-TargetNext

	.global TargetPrev
	.type TargetPrev, @function

TargetPrev:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          a0, 0(a0)

	// *** Basic block 1

.TargetPrev_label_9:
	ret         
.func_end_TargetPrev:
	.size TargetPrev, .func_end_TargetPrev-TargetPrev

	.global TargetDeleteInstruction
	.type TargetDeleteInstruction, @function

TargetDeleteInstruction:

	// *** Basic block 0

	.global TargetRemoveUser
	.global VectorDestruct
	.global ListDeleteElement
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
	addi        t0, s1, 40

	// *** Basic block 1

.TargetDeleteInstruction_label_23:
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	beq         s4, x0, .TargetDeleteInstruction_label_36

	// *** Basic block 2

	mv          a1, s1
	mv          a0, s4
	call        TargetRemoveUser

	// *** Basic block 3

.TargetDeleteInstruction_label_36:

	// *** Basic block 4

.TargetDeleteInstruction_label_37:
	addi        s3, s3, 1
	li          t0, 3		// 0x3 ASCII \x3
	bge         s3, t0, .TargetDeleteInstruction_label_23

	// *** Basic block 5

.TargetDeleteInstruction_label_42:
	addi        a0, s1, 64
	call        VectorDestruct

	// *** Basic block 6

	addi        a0, s2, 56
	mv          a1, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ListDeleteElement
.func_end_TargetDeleteInstruction:
	.size TargetDeleteInstruction, .func_end_TargetDeleteInstruction-TargetDeleteInstruction

	.global TargetAddUser
	.type TargetAddUser, @function

TargetAddUser:

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
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, x0
	addi        t0, s1, 64
	ld          s4, 8(t0)
	bge         x0, s4, .TargetAddUser_label_40

	// *** Basic block 1

	ld          t0, 64(s1)

	// *** Basic block 2

.TargetAddUser_label_23:
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s5, 0(t0)
	bne         s5, s2, .TargetAddUser_label_35

	// *** Basic block 3

.TargetAddUser_label_32:
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

.TargetAddUser_label_35:

	// *** Basic block 5

.TargetAddUser_label_36:
	addi        s3, s3, 1
	bge         s3, s4, .TargetAddUser_label_23

	// *** Basic block 6

.TargetAddUser_label_40:
	addi        a0, s1, 64
	mv          a1, s2
	call        VectorAppend

	// *** Basic block 7

	j           .TargetAddUser_label_32
.func_end_TargetAddUser:
	.size TargetAddUser, .func_end_TargetAddUser-TargetAddUser

	.global TargetRemoveUser
	.type TargetRemoveUser, @function

TargetRemoveUser:

	// *** Basic block 0

	.global VectorDeleteElement
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, x0
	addi        t3, t0, 64
	ld          t3, 8(t3)
	bge         x0, t3, .TargetRemoveUser_label_46

	// *** Basic block 1

	ld          t4, 64(t0)

	// *** Basic block 2

.TargetRemoveUser_label_23:
	slli        t5, t2, 3
	add         t4, t4, t5
	ld          t5, 0(t4)
	bne         t5, t1, .TargetRemoveUser_label_41

	// *** Basic block 3

	addi        a0, t0, 64
	mv          a1, t2
	j           VectorDeleteElement

	// *** Basic block 4

.TargetRemoveUser_label_38:
	ret         

	// *** Basic block 5

.TargetRemoveUser_label_41:

	// *** Basic block 6

.TargetRemoveUser_label_42:
	addi        t2, t2, 1
	bge         t2, t3, .TargetRemoveUser_label_23

	// *** Basic block 7

.TargetRemoveUser_label_46:
	j           .TargetRemoveUser_label_38
.func_end_TargetRemoveUser:
	.size TargetRemoveUser, .func_end_TargetRemoveUser-TargetRemoveUser

	.global TargetRetargetInstruction
	.type TargetRetargetInstruction, @function

TargetRetargetInstruction:

	// *** Basic block 0

	.global TargetAddUser
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
	mv          s2, a1
	mv          s3, x0
	addi        t0, s1, 64
	ld          s4, 8(t0)
	bge         x0, s4, .TargetRetargetInstruction_label_64

	// *** Basic block 1

	ld          t0, 64(s1)

	// *** Basic block 2

.TargetRetargetInstruction_label_28:
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s5, 0(t0)
	mv          s6, x0

	// *** Basic block 3

.TargetRetargetInstruction_label_37:
	addi        t0, s5, 40
	slli        s7, s6, 3
	add         t0, t0, s7
	ld          t0, 0(t0)
	bne         t0, s1, .TargetRetargetInstruction_label_53

	// *** Basic block 4

	addi        t0, s5, 40
	add         t0, t0, s7
	sd          s2, 0(t0)
	mv          a1, s5
	mv          a0, s2
	call        TargetAddUser

	// *** Basic block 5

.TargetRetargetInstruction_label_53:

	// *** Basic block 6

.TargetRetargetInstruction_label_54:
	addi        s6, s6, 1
	li          t0, 3		// 0x3 ASCII \x3
	bge         s6, t0, .TargetRetargetInstruction_label_37

	// *** Basic block 7

.TargetRetargetInstruction_label_59:

	// *** Basic block 8

.TargetRetargetInstruction_label_60:
	addi        s3, s3, 1
	bge         s3, s4, .TargetRetargetInstruction_label_28

	// *** Basic block 9

.TargetRetargetInstruction_label_64:
	addi        a0, s1, 64
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
.func_end_TargetRetargetInstruction:
	.size TargetRetargetInstruction, .func_end_TargetRetargetInstruction-TargetRetargetInstruction

	.global TargetRetargetInstructionIf
	.type TargetRetargetInstructionIf, @function

TargetRetargetInstructionIf:

	// *** Basic block 0

	.global TargetAddUser
	.global VectorClear
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
	mv          s2, a2
	mv          s3, a1
	mv          s4, x0
	addi        t0, s1, 64
	ld          s5, 8(t0)
	bge         x0, s5, .TargetRetargetInstructionIf_label_74

	// *** Basic block 1

	ld          t0, 64(s1)

	// *** Basic block 2

.TargetRetargetInstructionIf_label_31:
	slli        t1, s4, 3
	add         t0, t0, t1
	ld          s6, 0(t0)
	mv          s7, x0

	// *** Basic block 3

.TargetRetargetInstructionIf_label_40:
	addi        t0, s6, 40
	slli        s9, s7, 3
	add         t0, t0, s9
	ld          t0, 0(t0)
	sub         t1, t0, s1
	seqz        s8, t1
	bne         t0, s1, .TargetRetargetInstructionIf_label_53

	// *** Basic block 4

	mv          a0, s6
	jalr         x1, s2, 0

	// *** Basic block 5

	mv          s8, a0

	// *** Basic block 6

.TargetRetargetInstructionIf_label_53:
	beqz        s8, .TargetRetargetInstructionIf_label_63

	// *** Basic block 7

	addi        t0, s6, 40
	add         t0, t0, s9
	sd          s3, 0(t0)
	mv          a1, s6
	mv          a0, s3
	call        TargetAddUser

	// *** Basic block 8

.TargetRetargetInstructionIf_label_63:

	// *** Basic block 9

.TargetRetargetInstructionIf_label_64:
	addi        s7, s7, 1
	li          t0, 3		// 0x3 ASCII \x3
	bge         s7, t0, .TargetRetargetInstructionIf_label_40

	// *** Basic block 10

.TargetRetargetInstructionIf_label_69:

	// *** Basic block 11

.TargetRetargetInstructionIf_label_70:
	addi        s4, s4, 1
	bge         s4, s5, .TargetRetargetInstructionIf_label_31

	// *** Basic block 12

.TargetRetargetInstructionIf_label_74:
	addi        a0, s1, 64
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
	j           VectorClear
.func_end_TargetRetargetInstructionIf:
	.size TargetRetargetInstructionIf, .func_end_TargetRetargetInstructionIf-TargetRetargetInstructionIf

	.global TargetReplaceInstruction
	.type TargetReplaceInstruction, @function

TargetReplaceInstruction:

	// *** Basic block 0

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
	.global TargetRetargetInstruction
	.global TargetDeleteInstruction
	mv          s1, a1
	mv          s2, a0
	mv          a1, a2
	mv          a0, s1
	call        TargetRetargetInstruction

	// *** Basic block 1

	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           TargetDeleteInstruction
.func_end_TargetReplaceInstruction:
	.size TargetReplaceInstruction, .func_end_TargetReplaceInstruction-TargetReplaceInstruction

	.global TargetReplaceOperand
	.type TargetReplaceOperand, @function

TargetReplaceOperand:

	// *** Basic block 0

	.global printf
	.global abort
	.global TargetRemoveUser
	.global TargetAddUser
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
	addi        t0, s1, 40
	slli        s3, a1, 3
	add         t0, t0, s3
	ld          s4, 0(t0)
	bne         s4, s2, .TargetReplaceOperand_label_35

	// *** Basic block 1

.TargetReplaceOperand_label_32:
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

.TargetReplaceOperand_label_35:
	beq         s1, s2, .TargetReplaceOperand_label_40

	// *** Basic block 3

	j           .TargetReplaceOperand_label_56

	// *** Basic block 4

.TargetReplaceOperand_label_40:
	lla         a0, .str.56
	lla         a1, .str.57
	lla         a3, .str.58
	li          t0, 331		// 0x14b
	mv          a2, t0
	call        printf

	// *** Basic block 5

	call        abort

	// *** Basic block 6

.TargetReplaceOperand_label_56:
	mv          a1, s1
	mv          a0, s4
	call        TargetRemoveUser

	// *** Basic block 7

	addi        t0, s1, 40
	add         t0, t0, s3
	sd          s2, 0(t0)
	mv          a1, s1
	mv          a0, s2
	call        TargetAddUser

	// *** Basic block 8

	j           .TargetReplaceOperand_label_32
.func_end_TargetReplaceOperand:
	.size TargetReplaceOperand, .func_end_TargetReplaceOperand-TargetReplaceOperand

	.global TargetIntValue
	.type TargetIntValue, @function

TargetIntValue:

	// *** Basic block 0

	.global TargetIsConst
	.global printf
	.global abort
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
	call        TargetIsConst

	// *** Basic block 1

	beqz        a0, .TargetIntValue_label_21

	// *** Basic block 2

	j           .TargetIntValue_label_39

	// *** Basic block 3

.TargetIntValue_label_21:
	lla         a0, .str.59
	lla         a1, .str.60
	lla         a3, .str.61
	li          t0, 338		// 0x152
	mv          a2, t0
	call        printf

	// *** Basic block 4

	call        abort

	// *** Basic block 5

.TargetIntValue_label_39:
	mv          s2, s1
	ld          a0, 120(s2)

	// *** Basic block 6

.TargetIntValue_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetIntValue:
	.size TargetIntValue, .func_end_TargetIntValue-TargetIntValue

	.global TargetGetLoweredNode
	.type TargetGetLoweredNode, @function

TargetGetLoweredNode:

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
	ld          a0, 96(a0)
	beq         a0, x0, .TargetGetLoweredNode_label_20

	// *** Basic block 1

	j           .TargetGetLoweredNode_label_38

	// *** Basic block 2

.TargetGetLoweredNode_label_20:
	lla         a0, .str.62
	lla         a1, .str.63
	lla         a3, .str.64
	li          t0, 347		// 0x15b
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.TargetGetLoweredNode_label_38:

	// *** Basic block 5

.TargetGetLoweredNode_label_40:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetGetLoweredNode:
	.size TargetGetLoweredNode, .func_end_TargetGetLoweredNode-TargetGetLoweredNode

	.global TargetSetLoweredNode
	.type TargetSetLoweredNode, @function

TargetSetLoweredNode:

	// *** Basic block 0

	.global printf
	.global abort
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
	ld          t0, 96(s1)
	bne         t0, x0, .TargetSetLoweredNode_label_23

	// *** Basic block 1

	j           .TargetSetLoweredNode_label_40

	// *** Basic block 2

.TargetSetLoweredNode_label_23:
	lla         a0, .str.65
	lla         a1, .str.66
	lla         a3, .str.67
	li          t0, 352		// 0x160
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.TargetSetLoweredNode_label_40:
	sd          s2, 96(s1)
	mv          a0, s2

	// *** Basic block 5

.TargetSetLoweredNode_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetSetLoweredNode:
	.size TargetSetLoweredNode, .func_end_TargetSetLoweredNode-TargetSetLoweredNode

	.global TargetInitInstruction
	.type TargetInitInstruction, @function

TargetInitInstruction:

	// *** Basic block 0

	.global ListElementInit
	.local next_instruction_id
	.global VectorInit
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
	call        ListElementInit

	// *** Basic block 1

	la          t0, next_instruction_id
	lw          t1, 0(t0)
	addi        t0, t1, 1
	la          t1, next_instruction_id
	sw          t0, 0(t1)
	sw          t1, 20(s1)
	sw          s2, 16(s1)
	sw          x0, 88(s1)
	sd          x0, 24(s1)
	sd          x0, 32(s1)
	sd          x0, 40(s1)
	addi        t0, s1, 40
	sd          x0, 8(t0)
	addi        t0, s1, 40
	sd          x0, 16(t0)
	addi        a0, s1, 64
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorInit
.func_end_TargetInitInstruction:
	.size TargetInitInstruction, .func_end_TargetInitInstruction-TargetInitInstruction

	.global TargetUpdateOperandUsers
	.type TargetUpdateOperandUsers, @function

TargetUpdateOperandUsers:

	// *** Basic block 0

	.global TargetAddUser
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
	addi        t0, s1, 40

	// *** Basic block 1

.TargetUpdateOperandUsers_label_18:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s3, 0(t0)
	beq         s3, x0, .TargetUpdateOperandUsers_label_32

	// *** Basic block 2

	mv          a1, s1
	mv          a0, s3
	call        TargetAddUser

	// *** Basic block 3

.TargetUpdateOperandUsers_label_32:

	// *** Basic block 4

.TargetUpdateOperandUsers_label_33:
	addi        s2, s2, 1
	li          t0, 3		// 0x3 ASCII \x3
	bge         s2, t0, .TargetUpdateOperandUsers_label_18

	// *** Basic block 5

.TargetUpdateOperandUsers_label_38:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetUpdateOperandUsers:
	.size TargetUpdateOperandUsers, .func_end_TargetUpdateOperandUsers-TargetUpdateOperandUsers

	.global TargetNewInstruction
	.type TargetNewInstruction, @function

TargetNewInstruction:

	// *** Basic block 0

	.global malloc
	.global TargetInitInstruction
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
	li          a0, 112		// 0x70 ASCII 'p'
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        TargetInitInstruction

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.TargetNewInstruction_label_21:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetNewInstruction:
	.size TargetNewInstruction, .func_end_TargetNewInstruction-TargetNewInstruction

	.global TargetNewInstruction1
	.type TargetNewInstruction1, @function

TargetNewInstruction1:

	// *** Basic block 0

	.global TargetNewInstruction
	.global printf
	.global abort
	.global TargetUpdateOperandUsers
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
	call        TargetNewInstruction

	// *** Basic block 1

	mv          s2, a0
	beq         s2, s1, .TargetNewInstruction1_label_28

	// *** Basic block 2

	j           .TargetNewInstruction1_label_45

	// *** Basic block 3

.TargetNewInstruction1_label_28:
	lla         a0, .str.68
	lla         a1, .str.69
	lla         a3, .str.70
	li          t0, 390		// 0x186
	mv          a2, t0
	call        printf

	// *** Basic block 4

	call        abort

	// *** Basic block 5

.TargetNewInstruction1_label_45:
	sd          s1, 40(s2)
	mv          a0, s2
	call        TargetUpdateOperandUsers

	// *** Basic block 6

	mv          a0, s2

	// *** Basic block 7

.TargetNewInstruction1_label_53:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetNewInstruction1:
	.size TargetNewInstruction1, .func_end_TargetNewInstruction1-TargetNewInstruction1

	.global TargetNewInstruction2
	.type TargetNewInstruction2, @function

TargetNewInstruction2:

	// *** Basic block 0

	.global TargetNewInstruction
	.global printf
	.global abort
	.global TargetUpdateOperandUsers
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
	mv          s2, a2
	call        TargetNewInstruction

	// *** Basic block 1

	mv          s3, a0
	beq         s3, s1, .TargetNewInstruction2_label_37

	// *** Basic block 2

	j           .TargetNewInstruction2_label_53

	// *** Basic block 3

.TargetNewInstruction2_label_37:
	lla         a0, .str.71
	lla         a1, .str.72
	lla         a3, .str.73
	li          t0, 400		// 0x190
	mv          a2, t0
	call        printf

	// *** Basic block 4

	call        abort

	// *** Basic block 5

.TargetNewInstruction2_label_53:
	beq         s3, s2, .TargetNewInstruction2_label_58

	// *** Basic block 6

	j           .TargetNewInstruction2_label_73

	// *** Basic block 7

.TargetNewInstruction2_label_58:
	lla         a0, .str.74
	lla         a1, .str.75
	lla         a3, .str.76
	li          t0, 401		// 0x191
	mv          a2, t0
	call        printf

	// *** Basic block 8

	call        abort

	// *** Basic block 9

.TargetNewInstruction2_label_73:
	sd          s1, 40(s3)
	addi        t0, s3, 40
	sd          s2, 8(t0)
	mv          a0, s3
	call        TargetUpdateOperandUsers

	// *** Basic block 10

	mv          a0, s3

	// *** Basic block 11

.TargetNewInstruction2_label_84:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetNewInstruction2:
	.size TargetNewInstruction2, .func_end_TargetNewInstruction2-TargetNewInstruction2

	.global TargetNewInstruction3
	.type TargetNewInstruction3, @function

TargetNewInstruction3:

	// *** Basic block 0

	.global TargetNewInstruction
	.global printf
	.global abort
	.global TargetUpdateOperandUsers
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
	mv          s3, a3
	call        TargetNewInstruction

	// *** Basic block 1

	mv          s4, a0
	beq         s4, s1, .TargetNewInstruction3_label_46

	// *** Basic block 2

	j           .TargetNewInstruction3_label_61

	// *** Basic block 3

.TargetNewInstruction3_label_46:
	lla         a0, .str.77
	lla         a1, .str.78
	lla         a3, .str.79
	li          t0, 413		// 0x19d
	mv          a2, t0
	call        printf

	// *** Basic block 4

	call        abort

	// *** Basic block 5

.TargetNewInstruction3_label_61:
	beq         s4, s2, .TargetNewInstruction3_label_66

	// *** Basic block 6

	j           .TargetNewInstruction3_label_81

	// *** Basic block 7

.TargetNewInstruction3_label_66:
	lla         a0, .str.80
	lla         a1, .str.81
	lla         a3, .str.82
	li          t0, 414		// 0x19e
	mv          a2, t0
	call        printf

	// *** Basic block 8

	call        abort

	// *** Basic block 9

.TargetNewInstruction3_label_81:
	beq         s4, s3, .TargetNewInstruction3_label_86

	// *** Basic block 10

	j           .TargetNewInstruction3_label_101

	// *** Basic block 11

.TargetNewInstruction3_label_86:
	lla         a0, .str.83
	lla         a1, .str.84
	lla         a3, .str.85
	li          t0, 415		// 0x19f
	mv          a2, t0
	call        printf

	// *** Basic block 12

	call        abort

	// *** Basic block 13

.TargetNewInstruction3_label_101:
	sd          s1, 40(s4)
	addi        t0, s4, 40
	sd          s2, 8(t0)
	addi        t0, s4, 40
	sd          s3, 16(t0)
	mv          a0, s4
	call        TargetUpdateOperandUsers

	// *** Basic block 14

	mv          a0, s4

	// *** Basic block 15

.TargetNewInstruction3_label_115:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetNewInstruction3:
	.size TargetNewInstruction3, .func_end_TargetNewInstruction3-TargetNewInstruction3

	.global TargetSetDest
	.type TargetSetDest, @function

TargetSetDest:

	// *** Basic block 0

	.global printf
	.global abort
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
	ld          t0, 24(s1)
	bne         t0, x0, .TargetSetDest_label_23

	// *** Basic block 1

	j           .TargetSetDest_label_40

	// *** Basic block 2

.TargetSetDest_label_23:
	lla         a0, .str.86
	lla         a1, .str.87
	lla         a3, .str.88
	li          t0, 425		// 0x1a9
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.TargetSetDest_label_40:
	sd          s2, 24(s1)
	mv          a0, s1

	// *** Basic block 5

.TargetSetDest_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetSetDest:
	.size TargetSetDest, .func_end_TargetSetDest-TargetSetDest

	.global TargetEmit
	.type TargetEmit, @function

TargetEmit:

	// *** Basic block 0

	.global TargetNext
	.global TargetPrev
	.global ListAppend
	.local TrapInstruction
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
	mv          a0, s1
	call        TargetNext

	// *** Basic block 1

	sub         t0, a0, x0
	snez        s3, t0
	bne         a0, x0, .TargetEmit_label_27

	// *** Basic block 2

	mv          a0, s1
	call        TargetPrev

	// *** Basic block 3

	sub         t0, a0, x0
	snez        s3, t0

	// *** Basic block 4

.TargetEmit_label_27:
	beqz        s3, .TargetEmit_label_34

	// *** Basic block 5

	mv          a0, s1

	// *** Basic block 6

.TargetEmit_label_31:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.TargetEmit_label_34:
	addi        a0, s2, 56
	mv          a1, s1
	call        ListAppend

	// *** Basic block 8

	mv          a0, s1
	call        TrapInstruction

	// *** Basic block 9

	mv          a0, s1
	j           .TargetEmit_label_31
.func_end_TargetEmit:
	.size TargetEmit, .func_end_TargetEmit-TargetEmit

	.global TargetEmitBefore
	.type TargetEmitBefore, @function

TargetEmitBefore:

	// *** Basic block 0

	.global TargetNext
	.global TargetPrev
	.global TargetEmit
	.local TrapInstruction
	.global ListInsertBefore
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
	mv          a0, s1
	call        TargetNext

	// *** Basic block 1

	sub         t0, a0, x0
	snez        s4, t0
	bne         a0, x0, .TargetEmitBefore_label_31

	// *** Basic block 2

	mv          a0, s1
	call        TargetPrev

	// *** Basic block 3

	sub         t0, a0, x0
	snez        s4, t0

	// *** Basic block 4

.TargetEmitBefore_label_31:
	beqz        s4, .TargetEmitBefore_label_38

	// *** Basic block 5

	mv          a0, s1

	// *** Basic block 6

.TargetEmitBefore_label_35:
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

.TargetEmitBefore_label_38:
	bne         s2, x0, .TargetEmitBefore_label_51

	// *** Basic block 8

	mv          a1, s1
	mv          a0, s3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           TargetEmit

	// *** Basic block 10

.TargetEmitBefore_label_51:
	mv          a0, s1
	call        TrapInstruction

	// *** Basic block 11

	addi        a0, s3, 56
	mv          a2, s2
	mv          a1, s1
	call        ListInsertBefore

	// *** Basic block 12

	mv          a0, s1
	j           .TargetEmitBefore_label_35
.func_end_TargetEmitBefore:
	.size TargetEmitBefore, .func_end_TargetEmitBefore-TargetEmitBefore

	.global TargetEmitAfter
	.type TargetEmitAfter, @function

TargetEmitAfter:

	// *** Basic block 0

	.global TargetNext
	.global TargetPrev
	.local TrapInstruction
	.global ListInsertAfter
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
	mv          s3, a2
	mv          a0, s1
	call        TargetNext

	// *** Basic block 1

	sub         t0, a0, x0
	snez        s4, t0
	bne         a0, x0, .TargetEmitAfter_label_30

	// *** Basic block 2

	mv          a0, s1
	call        TargetPrev

	// *** Basic block 3

	sub         t0, a0, x0
	snez        s4, t0

	// *** Basic block 4

.TargetEmitAfter_label_30:
	beqz        s4, .TargetEmitAfter_label_37

	// *** Basic block 5

	mv          a0, s1

	// *** Basic block 6

.TargetEmitAfter_label_34:
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

.TargetEmitAfter_label_37:
	mv          a0, s1
	call        TrapInstruction

	// *** Basic block 8

	addi        a0, s2, 56
	mv          a2, s3
	mv          a1, s1
	call        ListInsertAfter

	// *** Basic block 9

	mv          a0, s1
	j           .TargetEmitAfter_label_34
.func_end_TargetEmitAfter:
	.size TargetEmitAfter, .func_end_TargetEmitAfter-TargetEmitAfter

	.global TargetEmitConstant
	.type TargetEmitConstant, @function

TargetEmitConstant:

	// *** Basic block 0

	.global TargetEmitAfter
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
	ld          a2, 80(s1)
	call        TargetEmitAfter

	// *** Basic block 1

	sd          a0, 80(s1)
	mv          a0, t0

	// *** Basic block 2

.TargetEmitConstant_label_24:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetEmitConstant:
	.size TargetEmitConstant, .func_end_TargetEmitConstant-TargetEmitConstant

	.global TargetEmitSymbol
	.type TargetEmitSymbol, @function

TargetEmitSymbol:

	// *** Basic block 0

	.global TargetEmitAfter
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
	ld          s3, 96(s1)
	bne         s3, x0, .TargetEmitSymbol_label_38

	// *** Basic block 1

	ld          t0, 88(s1)
	sd          t0, 96(s1)
	ld          a2, 80(s1)
	mv          a1, s2
	mv          a0, s1
	call        TargetEmitAfter

	// *** Basic block 2

	sd          a0, 0(t0)
	mv          a0, t0

	// *** Basic block 3

.TargetEmitSymbol_label_35:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.TargetEmitSymbol_label_38:
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        TargetEmitAfter

	// *** Basic block 5

	sd          a0, 96(s1)
	mv          a0, t0
	j           .TargetEmitSymbol_label_35
.func_end_TargetEmitSymbol:
	.size TargetEmitSymbol, .func_end_TargetEmitSymbol-TargetEmitSymbol

	.global TargetFramePointer
	.type TargetFramePointer, @function

TargetFramePointer:

	// *** Basic block 0

	.global TargetEmit
	.global TargetNewInstruction
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
	ld          t0, 104(s1)
	bne         t0, x0, .TargetFramePointer_label_28

	// *** Basic block 1

	li          t0, 23		// 0x17 ASCII \x17
	mv          a0, t0
	call        TargetNewInstruction

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        TargetEmit

	// *** Basic block 3

	sd          a0, 104(s1)

	// *** Basic block 4

.TargetFramePointer_label_28:
	ld          a0, 104(s1)

	// *** Basic block 5

.TargetFramePointer_label_32:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetFramePointer:
	.size TargetFramePointer, .func_end_TargetFramePointer-TargetFramePointer

	.global TargetStackPointer
	.type TargetStackPointer, @function

TargetStackPointer:

	// *** Basic block 0

	.global TargetEmit
	.global TargetNewInstruction
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
	ld          t0, 112(s1)
	bne         t0, x0, .TargetStackPointer_label_28

	// *** Basic block 1

	li          t0, 24		// 0x18 ASCII \x18
	mv          a0, t0
	call        TargetNewInstruction

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        TargetEmit

	// *** Basic block 3

	sd          a0, 112(s1)

	// *** Basic block 4

.TargetStackPointer_label_28:
	ld          a0, 112(s1)

	// *** Basic block 5

.TargetStackPointer_label_32:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetStackPointer:
	.size TargetStackPointer, .func_end_TargetStackPointer-TargetStackPointer

	.global TargetThreadPointer
	.type TargetThreadPointer, @function

TargetThreadPointer:

	// *** Basic block 0

	.global TargetEmit
	.global TargetNewInstruction
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
	ld          t0, 120(s1)
	bne         t0, x0, .TargetThreadPointer_label_28

	// *** Basic block 1

	li          t0, 25		// 0x19 ASCII \x19
	mv          a0, t0
	call        TargetNewInstruction

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        TargetEmit

	// *** Basic block 3

	sd          a0, 120(s1)

	// *** Basic block 4

.TargetThreadPointer_label_28:
	ld          a0, 120(s1)

	// *** Basic block 5

.TargetThreadPointer_label_32:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetThreadPointer:
	.size TargetThreadPointer, .func_end_TargetThreadPointer-TargetThreadPointer

	.global TargetNewLiteral
	.type TargetNewLiteral, @function

TargetNewLiteral:

	// *** Basic block 0

	.global malloc
	.global TargetInitInstruction
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
	li          a0, 120		// 0x78 ASCII 'x'
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s2
	call        TargetInitInstruction

	// *** Basic block 2

	sw          s1, 112(s2)
	mv          a0, s2

	// *** Basic block 3

.TargetNewLiteral_label_28:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetNewLiteral:
	.size TargetNewLiteral, .func_end_TargetNewLiteral-TargetNewLiteral

	.global TargetIsConst
	.type TargetIsConst, @function

TargetIsConst:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 16(a0)
	slti        t1, t0, 5
	not         a0, t1
	li          t1, 5		// 0x5 ASCII \x5
	blt         t0, t1, .TargetIsConst_label_19

	// *** Basic block 1

	li          t1, 10		// 0xa ASCII \xa
	slt         t0, t1, t0
	not         a0, t0

	// *** Basic block 2

.TargetIsConst_label_19:

	// *** Basic block 3

.TargetIsConst_label_21:
	ret         
.func_end_TargetIsConst:
	.size TargetIsConst, .func_end_TargetIsConst-TargetIsConst

	.global TargetIsZero
	.type TargetIsZero, @function

TargetIsZero:

	// *** Basic block 0

	.global TargetIntValue
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
	lw          s2, 16(s1)
	slti        t1, s2, 5
	not         t0, t1
	li          t1, 5		// 0x5 ASCII \x5
	blt         s2, t1, .TargetIsZero_label_22

	// *** Basic block 1

	li          t1, 10		// 0xa ASCII \xa
	slt         t1, t1, s2
	not         t0, t1

	// *** Basic block 2

.TargetIsZero_label_22:
	beqz        t0, .TargetIsZero_label_32

	// *** Basic block 3

	mv          a0, s1
	call        TargetIntValue

	// *** Basic block 4

	seqz        a0, a0

	// *** Basic block 5

.TargetIsZero_label_29:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.TargetIsZero_label_32:
	mv          a0, x0
	j           .TargetIsZero_label_29
.func_end_TargetIsZero:
	.size TargetIsZero, .func_end_TargetIsZero-TargetIsZero

	.global TargetNewIntConstant
	.type TargetNewIntConstant, @function

TargetNewIntConstant:

	// *** Basic block 0

	.global malloc
	.global TargetInitInstruction
	.local constant_ops
	.global TargetSetLoweredNode
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
	li          a0, 128		// 0x80 ASCII \x80
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	slli        t0, s1, 2
	la          t1, constant_ops
	add         t0, t1, t0
	lw          a1, 0(t0)
	mv          a0, s4
	call        TargetInitInstruction

	// *** Basic block 2

	sw          s1, 112(s4)
	sd          s2, 120(s4)
	beq         s3, x0, .TargetNewIntConstant_label_47

	// *** Basic block 3

	mv          a1, s4
	mv          a0, s3
	call        TargetSetLoweredNode

	// *** Basic block 4

.TargetNewIntConstant_label_47:
	mv          a0, s4

	// *** Basic block 5

.TargetNewIntConstant_label_50:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetNewIntConstant:
	.size TargetNewIntConstant, .func_end_TargetNewIntConstant-TargetNewIntConstant

	.global TargetNewFloatingPointConstant
	.type TargetNewFloatingPointConstant, @function

TargetNewFloatingPointConstant:

	// *** Basic block 0

	.global malloc
	.global TargetInitInstruction
	.local constant_ops
	.global TargetSetLoweredNode
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
	mv          s1, a1
	fmv.d       fs0, fa0
	mv          s2, a0
	li          a0, 128		// 0x80 ASCII \x80
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	slli        t0, s1, 2
	la          t1, constant_ops
	add         t0, t1, t0
	lw          a1, 0(t0)
	mv          a0, s3
	call        TargetInitInstruction

	// *** Basic block 2

	sw          s1, 112(s3)
	fsd         fs0, 120(s3)
	mv          a1, s3
	mv          a0, s2
	call        TargetSetLoweredNode

	// *** Basic block 3

	mv          a0, s3

	// *** Basic block 4

.TargetNewFloatingPointConstant_label_46:
	// Restored registers.
	fld fs0, 24(sp)
	ld s1, 16(sp)
	ld s2, 8(sp)
	ld s3, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetNewFloatingPointConstant:
	.size TargetNewFloatingPointConstant, .func_end_TargetNewFloatingPointConstant-TargetNewFloatingPointConstant

	.global TargetNewLocation
	.type TargetNewLocation, @function

TargetNewLocation:

	// *** Basic block 0

	.global malloc
	.global TargetInitInstruction
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
	li          a0, 120		// 0x78 ASCII 'x'
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	li          t0, 31		// 0x1f ASCII \x1f
	mv          a1, t0
	mv          a0, s2
	call        TargetInitInstruction

	// *** Basic block 2

	ld          t0, 136(s1)
	sd          t0, 112(s2)
	mv          a0, s2

	// *** Basic block 3

.TargetNewLocation_label_31:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetNewLocation:
	.size TargetNewLocation, .func_end_TargetNewLocation-TargetNewLocation

	.global TargetNewNamedLabel
	.type TargetNewNamedLabel, @function

TargetNewNamedLabel:

	// *** Basic block 0

	.global malloc
	.global TargetInitInstruction
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
	li          a0, 120		// 0x78 ASCII 'x'
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	li          t0, 32		// 0x20 ASCII ' '
	mv          a1, t0
	mv          a0, s2
	call        TargetInitInstruction

	// *** Basic block 2

	sd          s1, 112(s2)
	mv          a0, s2

	// *** Basic block 3

.TargetNewNamedLabel_label_28:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetNewNamedLabel:
	.size TargetNewNamedLabel, .func_end_TargetNewNamedLabel-TargetNewNamedLabel

	.global TargetGetIntConstant
	.type TargetGetIntConstant, @function

TargetGetIntConstant:

	// *** Basic block 0

	.global TargetFirstInstruction
	.global TargetPrev
	.global TargetLastConstant
	.global TargetIsConst
	.global TargetSetLoweredNode
	.global TargetNext
	.global TargetEmitConstant
	.global TargetNewIntConstant
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
	mv          s3, a3
	mv          s4, a1
	call        TargetFirstInstruction

	// *** Basic block 1

	ld          s5, 96(s4)
	mv          s6, a0
	sub         t0, s6, x0
	snez        s7, t0
	beq         s6, x0, .TargetGetIntConstant_label_54

	// *** Basic block 2

	sub         t0, s4, x0
	snez        t1, t0
	sub         t0, s5, x0
	seqz        t1, t0
	mv          a0, s6
	call        TargetPrev

	// *** Basic block 3

	mv          s5, a0
	mv          a0, s1
	call        TargetLastConstant

	// *** Basic block 4

	mv          t0, a0
	sub         t1, s5, t0
	snez        s7, t1

	// *** Basic block 5

.TargetGetIntConstant_label_54:
	beqz        s7, .TargetGetIntConstant_label_114

	// *** Basic block 6

.TargetGetIntConstant_label_56:
	mv          a0, s6
	call        TargetIsConst

	// *** Basic block 7

	beqz        a0, .TargetGetIntConstant_label_91

	// *** Basic block 8

	mv          s5, s6
	lw          t1, 112(s5)
	sub         t2, t1, s2
	seqz        t0, t2
	bne         t1, s2, .TargetGetIntConstant_label_73

	// *** Basic block 9

	ld          t1, 120(s5)
	sub         t1, t1, s3
	seqz        t0, t1

	// *** Basic block 10

.TargetGetIntConstant_label_73:
	beqz        t0, .TargetGetIntConstant_label_90

	// *** Basic block 11

	beq         s4, x0, .TargetGetIntConstant_label_77

	// *** Basic block 12

.TargetGetIntConstant_label_77:
	beqz        t1, .TargetGetIntConstant_label_84

	// *** Basic block 13

	mv          a1, s6
	mv          a0, s4
	call        TargetSetLoweredNode

	// *** Basic block 14

.TargetGetIntConstant_label_84:
	mv          a0, s6

	// *** Basic block 15

.TargetGetIntConstant_label_87:
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

	// *** Basic block 16

.TargetGetIntConstant_label_90:

	// *** Basic block 17

.TargetGetIntConstant_label_91:
	mv          a0, s6
	call        TargetNext

	// *** Basic block 18

	mv          s6, a0
	sub         t0, s6, x0
	snez        s7, t0
	beq         s6, x0, .TargetGetIntConstant_label_112

	// *** Basic block 19

	mv          a0, s6
	call        TargetPrev

	// *** Basic block 20

	mv          s8, a0
	mv          a0, s1
	call        TargetLastConstant

	// *** Basic block 21

	mv          t0, a0
	sub         t1, s8, t0
	snez        s7, t1

	// *** Basic block 22

.TargetGetIntConstant_label_112:
	bnez        s7, .TargetGetIntConstant_label_56

	// *** Basic block 23

.TargetGetIntConstant_label_114:
	mv          a2, s3
	mv          a1, s2
	mv          a0, s4
	call        TargetNewIntConstant

	// *** Basic block 24

	mv          a1, a0
	mv          a0, s1
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
	j           TargetEmitConstant
.func_end_TargetGetIntConstant:
	.size TargetGetIntConstant, .func_end_TargetGetIntConstant-TargetGetIntConstant

	.global TargetGetFloatingPointConstant
	.type TargetGetFloatingPointConstant, @function

TargetGetFloatingPointConstant:

	// *** Basic block 0

	.global TargetFirstInstruction
	.global TargetPrev
	.global TargetLastConstant
	.global TargetIsConst
	.global TargetSetLoweredNode
	.global TargetNext
	.global TargetEmitConstant
	.global TargetNewFloatingPointConstant
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
	// Saved floating point registers.
	fsd fs0, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a2
	fmv.d       fs0, fa0
	mv          s3, a1
	call        TargetFirstInstruction

	// *** Basic block 1

	ld          s4, 96(s3)
	mv          s5, a0
	sub         t0, s5, x0
	snez        s6, t0
	beq         s5, x0, .TargetGetFloatingPointConstant_label_54

	// *** Basic block 2

	sub         t0, s3, x0
	snez        t1, t0
	sub         t0, s4, x0
	seqz        t1, t0
	mv          a0, s5
	call        TargetPrev

	// *** Basic block 3

	mv          s4, a0
	mv          a0, s1
	call        TargetLastConstant

	// *** Basic block 4

	mv          t0, a0
	sub         t1, s4, t0
	snez        s6, t1

	// *** Basic block 5

.TargetGetFloatingPointConstant_label_54:
	beqz        s6, .TargetGetFloatingPointConstant_label_113

	// *** Basic block 6

.TargetGetFloatingPointConstant_label_56:
	mv          s4, s5
	mv          a0, s5
	call        TargetIsConst

	// *** Basic block 7

	beqz        a0, .TargetGetFloatingPointConstant_label_90

	// *** Basic block 8

	lw          t1, 112(s4)
	sub         t2, t1, s2
	seqz        t0, t2
	bne         t1, s2, .TargetGetFloatingPointConstant_label_72

	// *** Basic block 9

	fld         ft0, 120(s4)
	feq.d       t0, ft0, fs0

	// *** Basic block 10

.TargetGetFloatingPointConstant_label_72:
	beqz        t0, .TargetGetFloatingPointConstant_label_89

	// *** Basic block 11

	beq         s3, x0, .TargetGetFloatingPointConstant_label_76

	// *** Basic block 12

.TargetGetFloatingPointConstant_label_76:
	beqz        t1, .TargetGetFloatingPointConstant_label_83

	// *** Basic block 13

	mv          a1, s5
	mv          a0, s3
	call        TargetSetLoweredNode

	// *** Basic block 14

.TargetGetFloatingPointConstant_label_83:
	mv          a0, s5

	// *** Basic block 15

.TargetGetFloatingPointConstant_label_86:
	// Restored registers.
	fld fs0, 56(sp)
	ld s1, 48(sp)
	ld s2, 40(sp)
	ld s3, 32(sp)
	ld s4, 24(sp)
	ld s5, 16(sp)
	ld s6, 8(sp)
	ld s7, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 16

.TargetGetFloatingPointConstant_label_89:

	// *** Basic block 17

.TargetGetFloatingPointConstant_label_90:
	mv          a0, s5
	call        TargetNext

	// *** Basic block 18

	mv          s5, a0
	sub         t0, s5, x0
	snez        s6, t0
	beq         s5, x0, .TargetGetFloatingPointConstant_label_111

	// *** Basic block 19

	mv          a0, s5
	call        TargetPrev

	// *** Basic block 20

	mv          s7, a0
	mv          a0, s1
	call        TargetLastConstant

	// *** Basic block 21

	mv          t0, a0
	sub         t1, s7, t0
	snez        s6, t1

	// *** Basic block 22

.TargetGetFloatingPointConstant_label_111:
	bnez        s6, .TargetGetFloatingPointConstant_label_56

	// *** Basic block 23

.TargetGetFloatingPointConstant_label_113:
	fmv.d       fa0, fs0
	mv          a1, s2
	mv          a0, s3
	call        TargetNewFloatingPointConstant

	// *** Basic block 24

	mv          a1, a0
	mv          a0, s1
	// Restored registers.
	fld fs0, 56(sp)
	ld s1, 48(sp)
	ld s2, 40(sp)
	ld s3, 32(sp)
	ld s4, 24(sp)
	ld s5, 16(sp)
	ld s6, 8(sp)
	ld s7, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           TargetEmitConstant
.func_end_TargetGetFloatingPointConstant:
	.size TargetGetFloatingPointConstant, .func_end_TargetGetFloatingPointConstant-TargetGetFloatingPointConstant

	.global NewTargetSymbol
	.type NewTargetSymbol, @function

NewTargetSymbol:

	// *** Basic block 0

	.global malloc
	.global TargetInitInstruction
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
	li          a0, 120		// 0x78 ASCII 'x'
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s2
	call        TargetInitInstruction

	// *** Basic block 2

	sd          s1, 112(s2)
	mv          a0, s2

	// *** Basic block 3

.NewTargetSymbol_label_28:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewTargetSymbol:
	.size NewTargetSymbol, .func_end_NewTargetSymbol-NewTargetSymbol

	.global TargetGetSymbol
	.type TargetGetSymbol, @function

TargetGetSymbol:

	// *** Basic block 0

	.global TargetFirstSymbol
	.global TargetPrev
	.global TargetLastSymbol
	.global TargetSetLoweredNode
	.global TargetNext
	.global TargetEmitSymbol
	.global NewTargetSymbol
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
	mv          s3, a1
	call        TargetFirstSymbol

	// *** Basic block 1

	ld          s4, 96(s3)
	mv          s5, a0
	sub         t0, s5, x0
	snez        s6, t0
	beq         s5, x0, .TargetGetSymbol_label_51

	// *** Basic block 2

	sub         t0, s3, x0
	snez        t1, t0
	sub         t0, s4, x0
	seqz        t1, t0
	mv          a0, s5
	call        TargetPrev

	// *** Basic block 3

	mv          s4, a0
	mv          a0, s1
	call        TargetLastSymbol

	// *** Basic block 4

	mv          t0, a0
	sub         t1, s4, t0
	snez        s6, t1

	// *** Basic block 5

.TargetGetSymbol_label_51:
	beqz        s6, .TargetGetSymbol_label_107

	// *** Basic block 6

.TargetGetSymbol_label_53:
	mv          s4, s5
	lw          t0, 16(s5)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .TargetGetSymbol_label_84

	// *** Basic block 7

	ld          t0, 112(s4)
	bne         t0, s2, .TargetGetSymbol_label_83

	// *** Basic block 8

	beq         s3, x0, .TargetGetSymbol_label_70

	// *** Basic block 9

.TargetGetSymbol_label_70:
	beqz        t1, .TargetGetSymbol_label_77

	// *** Basic block 10

	mv          a1, s5
	mv          a0, s3
	call        TargetSetLoweredNode

	// *** Basic block 11

.TargetGetSymbol_label_77:
	mv          a0, s5

	// *** Basic block 12

.TargetGetSymbol_label_80:
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

	// *** Basic block 13

.TargetGetSymbol_label_83:

	// *** Basic block 14

.TargetGetSymbol_label_84:
	mv          a0, s5
	call        TargetNext

	// *** Basic block 15

	mv          s5, a0
	sub         t0, s5, x0
	snez        s6, t0
	beq         s5, x0, .TargetGetSymbol_label_105

	// *** Basic block 16

	mv          a0, s5
	call        TargetPrev

	// *** Basic block 17

	mv          s7, a0
	mv          a0, s1
	call        TargetLastSymbol

	// *** Basic block 18

	mv          t0, a0
	sub         t1, s7, t0
	snez        s6, t1

	// *** Basic block 19

.TargetGetSymbol_label_105:
	bnez        s6, .TargetGetSymbol_label_53

	// *** Basic block 20

.TargetGetSymbol_label_107:
	mv          a0, s2
	call        NewTargetSymbol

	// *** Basic block 21

	mv          a1, a0
	mv          a0, s1
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
	j           TargetEmitSymbol
.func_end_TargetGetSymbol:
	.size TargetGetSymbol, .func_end_TargetGetSymbol-TargetGetSymbol

	.global NewBranchFixup
	.type NewBranchFixup, @function

NewBranchFixup:

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
	sd          s2, 8(s4)
	sw          s3, 16(s4)
	mv          a0, s4

	// *** Basic block 2

.NewBranchFixup_label_29:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewBranchFixup:
	.size NewBranchFixup, .func_end_NewBranchFixup-NewBranchFixup

	.global TargetApplyFixups
	.type TargetApplyFixups, @function

TargetApplyFixups:

	// *** Basic block 0

	.global TargetGetLoweredNode
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
	mv          s2, x0
	addi        t0, a0, 136
	ld          s3, 8(t0)
	bge         x0, s3, .TargetApplyFixups_label_51

	// *** Basic block 1

	ld          t0, 136(a0)

	// *** Basic block 2

.TargetApplyFixups_label_25:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	ld          t0, 8(s4)
	bne         t0, s1, .TargetApplyFixups_label_46

	// *** Basic block 3

	ld          t0, 0(s4)
	addi        t0, t0, 40
	lw          t1, 16(s4)
	slli        t1, t1, 3
	add         s4, t0, t1
	mv          a0, s1
	call        TargetGetLoweredNode

	// *** Basic block 4

	sd          a0, 0(s4)

	// *** Basic block 5

.TargetApplyFixups_label_46:

	// *** Basic block 6

.TargetApplyFixups_label_47:
	addi        s2, s2, 1
	bge         s2, s3, .TargetApplyFixups_label_25

	// *** Basic block 7

.TargetApplyFixups_label_51:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TargetApplyFixups:
	.size TargetApplyFixups, .func_end_TargetApplyFixups-TargetApplyFixups

	.global TargetRegisterInit
	.type TargetRegisterInit, @function

TargetRegisterInit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sw          a1, 0(a0)
	sd          x0, 8(a0)
	sb          x0, 4(a0)
	ret         
.func_end_TargetRegisterInit:
	.size TargetRegisterInit, .func_end_TargetRegisterInit-TargetRegisterInit

.PCend:
	.data
next_instruction_id:
	.type   next_instruction_id,@object
	.local  next_instruction_id
	.size   next_instruction_id,4
	.p2align  2
	.word   1

constant_ops:
	.type   constant_ops,@object
	.local  constant_ops
	.size   constant_ops,28
	.p2align  2
	.word   5
	.word   6
	.word   7
	.word   8
	.word   9
	.word   10
	.word   8

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "@%d %s("
	.type .str.1, @object
	.size .str.1, 8

.str.2:
	.asciz "#%" PRId64 ""
	.type .str.2, @object
	.size .str.2, 6

.str.3:
	.asciz "#%g"
	.type .str.3, @object
	.size .str.3, 4

.str.4:
	.asciz "%s"
	.type .str.4, @object
	.size .str.4, 3

.str.5:
	.asciz "%s %d %d %d"
	.type .str.5, @object
	.size .str.5, 12

.str.6:
	.asciz "(null)"
	.type .str.6, @object
	.size .str.6, 1

.str.7:
	.asciz "%s@%d"
	.type .str.7, @object
	.size .str.7, 6

.str.8:
	.asciz ", "
	.type .str.8, @object
	.size .str.8, 3

.str.9:
	.asciz ")"
	.type .str.9, @object
	.size .str.9, 2

.str.10:
	.asciz " -> @%d"
	.type .str.10, @object
	.size .str.10, 8

.str.11:
	.asciz " *%zd"
	.type .str.11, @object
	.size .str.11, 6

.str.12:
	.asciz " ["
	.type .str.12, @object
	.size .str.12, 3

.str.13:
	.asciz "(null)"
	.type .str.13, @object
	.size .str.13, 1

.str.14:
	.asciz "%s@%d"
	.type .str.14, @object
	.size .str.14, 6

.str.15:
	.asciz ","
	.type .str.15, @object
	.size .str.15, 2

.str.16:
	.asciz "]"
	.type .str.16, @object
	.size .str.16, 2

.str.17:
	.asciz "\n"
	.type .str.17, @object
	.size .str.17, 2

.str.18:
	.asciz "save"
	.type .str.18, @object
	.size .str.18, 5

.str.19:
	.asciz "restore"
	.type .str.19, @object
	.size .str.19, 8

.str.20:
	.asciz "symbol"
	.type .str.20, @object
	.size .str.20, 7

.str.21:
	.asciz "literal"
	.type .str.21, @object
	.size .str.21, 8

.str.22:
	.asciz "tmp"
	.type .str.22, @object
	.size .str.22, 4

.str.23:
	.asciz "constb"
	.type .str.23, @object
	.size .str.23, 7

.str.24:
	.asciz "const16"
	.type .str.24, @object
	.size .str.24, 7

.str.25:
	.asciz "const32"
	.type .str.25, @object
	.size .str.25, 7

.str.26:
	.asciz "const64"
	.type .str.26, @object
	.size .str.26, 7

.str.27:
	.asciz "constf"
	.type .str.27, @object
	.size .str.27, 7

.str.28:
	.asciz "constd"
	.type .str.28, @object
	.size .str.28, 7

.str.29:
	.asciz "mov"
	.type .str.29, @object
	.size .str.29, 4

.str.30:
	.asciz "movf"
	.type .str.30, @object
	.size .str.30, 5

.str.31:
	.asciz "movd"
	.type .str.31, @object
	.size .str.31, 5

.str.32:
	.asciz "movc"
	.type .str.32, @object
	.size .str.32, 5

.str.33:
	.asciz "movfc"
	.type .str.33, @object
	.size .str.33, 6

.str.34:
	.asciz "movdc"
	.type .str.34, @object
	.size .str.34, 6

.str.35:
	.asciz "movxc"
	.type .str.35, @object
	.size .str.35, 6

.str.36:
	.asciz "rmov"
	.type .str.36, @object
	.size .str.36, 5

.str.37:
	.asciz "rmovf"
	.type .str.37, @object
	.size .str.37, 6

.str.38:
	.asciz "rmovd"
	.type .str.38, @object
	.size .str.38, 6

.str.39:
	.asciz "ret"
	.type .str.39, @object
	.size .str.39, 4

.str.40:
	.asciz "label"
	.type .str.40, @object
	.size .str.40, 6

.str.41:
	.asciz "fp"
	.type .str.41, @object
	.size .str.41, 3

.str.42:
	.asciz "sp"
	.type .str.42, @object
	.size .str.42, 3

.str.43:
	.asciz "tp"
	.type .str.43, @object
	.size .str.43, 3

.str.44:
	.asciz "resulti"
	.type .str.44, @object
	.size .str.44, 8

.str.45:
	.asciz "resultf"
	.type .str.45, @object
	.size .str.45, 8

.str.46:
	.asciz "resultd"
	.type .str.46, @object
	.size .str.46, 8

.str.47:
	.asciz "structreturn"
	.type .str.47, @object
	.size .str.47, 13

.str.48:
	.asciz "asm"
	.type .str.48, @object
	.size .str.48, 4

.str.49:
	.asciz "loc"
	.type .str.49, @object
	.size .str.49, 4

.str.50:
	.asciz "namedlabel"
	.type .str.50, @object
	.size .str.50, 11

.str.51:
	.asciz "ivarreg"
	.type .str.51, @object
	.size .str.51, 8

.str.52:
	.asciz "fvarreg"
	.type .str.52, @object
	.size .str.52, 8

.str.53:
	.asciz "memcpy"
	.type .str.53, @object
	.size .str.53, 7

.str.54:
	.asciz "memset"
	.type .str.54, @object
	.size .str.54, 7

.str.55:
	.asciz "__tls_get_addr"
	.type .str.55, @object
	.size .str.55, 15

.str.56:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.56, @object
	.size .str.56, 30

.str.57:
	.asciz "(null)"
	.type .str.57, @object
	.size .str.57, 1

.str.58:
	.asciz "inst != new"
	.type .str.58, @object
	.size .str.58, 12

.str.59:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.59, @object
	.size .str.59, 30

.str.60:
	.asciz "(null)"
	.type .str.60, @object
	.size .str.60, 1

.str.61:
	.asciz "TargetIsConst(inst)"
	.type .str.61, @object
	.size .str.61, 20

.str.62:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.62, @object
	.size .str.62, 30

.str.63:
	.asciz "(null)"
	.type .str.63, @object
	.size .str.63, 1

.str.64:
	.asciz "node->data.ptr != NULL"
	.type .str.64, @object
	.size .str.64, 23

.str.65:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.65, @object
	.size .str.65, 30

.str.66:
	.asciz "(null)"
	.type .str.66, @object
	.size .str.66, 1

.str.67:
	.asciz "node->data.ptr == NULL"
	.type .str.67, @object
	.size .str.67, 23

.str.68:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.68, @object
	.size .str.68, 30

.str.69:
	.asciz "(null)"
	.type .str.69, @object
	.size .str.69, 1

.str.70:
	.asciz "inst != op1"
	.type .str.70, @object
	.size .str.70, 12

.str.71:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.71, @object
	.size .str.71, 30

.str.72:
	.asciz "(null)"
	.type .str.72, @object
	.size .str.72, 1

.str.73:
	.asciz "inst != op1"
	.type .str.73, @object
	.size .str.73, 12

.str.74:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.74, @object
	.size .str.74, 30

.str.75:
	.asciz "(null)"
	.type .str.75, @object
	.size .str.75, 1

.str.76:
	.asciz "inst != op2"
	.type .str.76, @object
	.size .str.76, 12

.str.77:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.77, @object
	.size .str.77, 30

.str.78:
	.asciz "(null)"
	.type .str.78, @object
	.size .str.78, 1

.str.79:
	.asciz "inst != op1"
	.type .str.79, @object
	.size .str.79, 12

.str.80:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.80, @object
	.size .str.80, 30

.str.81:
	.asciz "(null)"
	.type .str.81, @object
	.size .str.81, 1

.str.82:
	.asciz "inst != op2"
	.type .str.82, @object
	.size .str.82, 12

.str.83:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.83, @object
	.size .str.83, 30

.str.84:
	.asciz "(null)"
	.type .str.84, @object
	.size .str.84, 1

.str.85:
	.asciz "inst != op3"
	.type .str.85, @object
	.size .str.85, 12

.str.86:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.86, @object
	.size .str.86, 30

.str.87:
	.asciz "(null)"
	.type .str.87, @object
	.size .str.87, 1

.str.88:
	.asciz "inst->dest == NULL"
	.type .str.88, @object
	.size .str.88, 19

