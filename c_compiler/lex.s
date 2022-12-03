	.file   "lex.c"
	.text
	.option pic
.PCbegin:
	.local  CompareReservedWord
	.type CompareReservedWord, @function

CompareReservedWord:

	// *** Basic block 0

	.global strcmp
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, t0
	mv          t3, t1
	ld          a0, 0(t2)
	ld          a1, 0(t3)
	j           strcmp
.func_end_CompareReservedWord:
	.size CompareReservedWord, .func_end_CompareReservedWord-CompareReservedWord

	.local  IsReservedWord
	.type IsReservedWord, @function

IsReservedWord:

	// *** Basic block 0

	.global bsearch
	.local reserved_words
	.local CompareReservedWord
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
	mv          t0, a0
	mv          s1, a1
	sd          t0, -32(s0)
	addi        a0, s0, -32
	la          a1, reserved_words
	la          a4, CompareReservedWord
	li          a3, 16		// 0x10 ASCII \x10
	li          a2, 40		// 0x28 ASCII '('
	call        bsearch

	// *** Basic block 1

	mv          s2, a0
	beq         s2, x0, .IsReservedWord_label_47

	// *** Basic block 2

	lw          t0, 8(s2)
	sw          t0, 0(s1)
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 3

.IsReservedWord_label_44:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.IsReservedWord_label_47:
	mv          a0, x0
	j           .IsReservedWord_label_44
.func_end_IsReservedWord:
	.size IsReservedWord, .func_end_IsReservedWord-IsReservedWord

	.local  GetCharInComment
	.type GetCharInComment, @function

GetCharInComment:

	// *** Basic block 0

	.global LexReadLine
	.global StringCharAt
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
	ld          t0, 48(s1)
	addi        t1, s1, 8
	ld          t1, 24(t1)
	bne         t0, t1, .GetCharInComment_label_29

	// *** Basic block 1

	mv          a0, s1
	call        LexReadLine

	// *** Basic block 2

	li          a0, 10		// 0xa ASCII \xa

	// *** Basic block 3

.GetCharInComment_label_26:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.GetCharInComment_label_29:
	addi        a0, s1, 8
	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)
	mv          a1, t0
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           StringCharAt
.func_end_GetCharInComment:
	.size GetCharInComment, .func_end_GetCharInComment-GetCharInComment

	.local  CurrentChar
	.type CurrentChar, @function

CurrentChar:

	// *** Basic block 0

	.global StringCharAt
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 8
	ld          a1, 48(t0)
	j           StringCharAt
.func_end_CurrentChar:
	.size CurrentChar, .func_end_CurrentChar-CurrentChar

	.local  LookaheadChar
	.type LookaheadChar, @function

LookaheadChar:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	addi        t0, a0, 8
	ld          t0, 16(t0)
	ld          t1, 48(a0)
	addi        t1, t1, 1
	add         t0, t0, t1
	lb          a0, 0(t0)

	// *** Basic block 1

.LookaheadChar_label_20:
	ret         
.func_end_LookaheadChar:
	.size LookaheadChar, .func_end_LookaheadChar-LookaheadChar

	.local  EscapeChar
	.type EscapeChar, @function

EscapeChar:

	// *** Basic block 0

	.global isxdigit
	.global isalpha
	.global tolower
	.global LexError
	sd          a0, -0(s0)	// Spilled @421
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
	mv          s1, a1
	mv          s2, a0
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 0(s1)
	addi        s3, s2, 8
	ld          t0, 16(s3)
	ld          s4, 48(s2)
	add         t0, t0, s4
	lb          s5, 0(t0)
	addi        t1, s5, -120
	seqz        t0, t1
	li          t1, 120		// 0x78 ASCII 'x'
	beq         s5, t1, .EscapeChar_label_68

	// *** Basic block 1

	addi        t1, s5, -88
	seqz        t0, t1

	// *** Basic block 2

.EscapeChar_label_68:
	beqz        t0, .EscapeChar_label_138

	// *** Basic block 3

	ld          s6, 48(s2)
	addi        t0, s6, 1
	sd          t0, 48(s2)
	mv          s7, x0
	addi        s9, s2, 8
	ld          t0, 24(s9)
	slt         s8, s6, t0
	bge         s6, t0, .EscapeChar_label_89

	// *** Basic block 4

	ld          t0, 16(s9)
	add         t0, t0, s6
	lb          a0, 0(t0)
	call        isxdigit

	// *** Basic block 5

	mv          s8, a0

	// *** Basic block 6

.EscapeChar_label_89:
	beqz        s8, .EscapeChar_label_132

	// *** Basic block 7

.EscapeChar_label_91:
	addi        s6, s2, 8
	ld          s9, 16(s6)
	ld          t0, 48(s2)
	addi        t0, t0, 1
	sd          t0, 48(s2)
	add         t0, s9, t0
	lb          s5, 0(t0)
	slli        s7, s7, 4
	mv          a0, s5
	call        isalpha

	// *** Basic block 8

	beqz        a0, .EscapeChar_label_115

	// *** Basic block 9

	mv          a0, s5
	call        tolower

	// *** Basic block 10

	addi        t1, a0, -97
	addi        t1, t1, 10
	or          s7, s7, t1
	j           .EscapeChar_label_119

	// *** Basic block 11

.EscapeChar_label_115:
	addi        t1, s5, -48
	or          s7, s7, t1

	// *** Basic block 12

.EscapeChar_label_119:
	ld          t1, 24(s6)
	slt         s10, t0, t1
	bge         t0, t1, .EscapeChar_label_130

	// *** Basic block 13

	add         t0, s9, t0
	lb          a0, 0(t0)
	call        isxdigit

	// *** Basic block 14

	mv          s10, a0

	// *** Basic block 15

.EscapeChar_label_130:
	bnez        s10, .EscapeChar_label_91

	// *** Basic block 16

.EscapeChar_label_132:
	mv          a0, s7

	// *** Basic block 17

.EscapeChar_label_135:
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

	// *** Basic block 18

.EscapeChar_label_138:
	addi        t1, s5, -117
	seqz        t0, t1
	li          t1, 117		// 0x75 ASCII 'u'
	beq         s5, t1, .EscapeChar_label_148

	// *** Basic block 19

	addi        t1, s5, -85
	seqz        t0, t1

	// *** Basic block 20

.EscapeChar_label_148:
	beqz        t0, .EscapeChar_label_238

	// *** Basic block 21

	mv          s6, x0
	li          s8, 4		// 0x4 ASCII \x4
	ld          s9, 48(s2)
	addi        t0, s9, 1
	sd          t0, 48(s2)
	li          s10, 1		// 0x1 ASCII \x1
	addi        t0, s2, 8
	ld          t0, 24(t0)
	slt         s10, s9, t0

	// *** Basic block 22

.EscapeChar_label_166:
	beqz        s10, .EscapeChar_label_176

	// *** Basic block 23

	addi        t0, s2, 8
	ld          t0, 16(t0)
	add         t0, t0, s9
	lb          a0, 0(t0)
	call        isxdigit

	// *** Basic block 24

	mv          s10, a0

	// *** Basic block 25

.EscapeChar_label_176:
	beqz        s10, .EscapeChar_label_223

	// *** Basic block 26

.EscapeChar_label_178:
	addi        s9, s2, 8
	ld          s11, 16(s9)
	ld          t0, 48(s2)
	addi        t0, t0, 1
	sd          t0, 48(s2)
	add         t0, s11, t0
	lb          s5, 0(t0)
	slli        s6, s6, 4
	mv          a0, s5
	call        isalpha

	// *** Basic block 27

	beqz        a0, .EscapeChar_label_201

	// *** Basic block 28

	mv          a0, s5
	call        tolower

	// *** Basic block 29

	addi        t1, a0, -97
	addi        t1, t1, 10
	or          s6, s6, t1
	j           .EscapeChar_label_204

	// *** Basic block 30

.EscapeChar_label_201:
	addi        t1, s5, -48
	or          s6, s6, t1

	// *** Basic block 31

.EscapeChar_label_204:
	addi        s8, s8, -1
	slt         a0, x0, s8
	bge         x0, s8, .EscapeChar_label_214

	// *** Basic block 32

	ld          t1, 24(s9)
	slt         a0, t0, t1

	// *** Basic block 33

.EscapeChar_label_214:
	beqz        a0, .EscapeChar_label_221

	// *** Basic block 34

	add         t0, s11, t0
	lb          a0, 0(t0)
	call        isxdigit

	// *** Basic block 35


	// *** Basic block 36

.EscapeChar_label_221:
	bnez        a0, .EscapeChar_label_178

	// *** Basic block 37

.EscapeChar_label_223:
	beqz        s8, .EscapeChar_label_232

	// *** Basic block 38

	lla         a1, .str.41
	mv          a0, s2
	call        LexError

	// *** Basic block 39

.EscapeChar_label_232:
	li          t0, 4		// 0x4 ASCII \x4
	sw          t0, 0(s1)
	mv          a0, s6
	j           .EscapeChar_label_135

	// *** Basic block 40

.EscapeChar_label_238:
	slti        t1, s5, 48
	not         t0, t1
	li          t1, 48		// 0x30 ASCII '0'
	blt         s5, t1, .EscapeChar_label_247

	// *** Basic block 41

	li          t1, 55		// 0x37 ASCII '7'
	slt         t1, t1, s5
	not         t0, t1

	// *** Basic block 42

.EscapeChar_label_247:
	beqz        t0, .EscapeChar_label_303

	// *** Basic block 43

	mv          s8, x0
	ld          t1, 24(s3)
	slt         t0, s4, t1
	bge         s4, t1, .EscapeChar_label_259

	// *** Basic block 44

	slti        t1, s5, 48
	not         t0, t1

	// *** Basic block 45

.EscapeChar_label_259:
	beqz        t0, .EscapeChar_label_264

	// *** Basic block 46

	li          t1, 55		// 0x37 ASCII '7'
	slt         t1, t1, s5
	not         t0, t1

	// *** Basic block 47

.EscapeChar_label_264:
	beqz        t0, .EscapeChar_label_299

	// *** Basic block 48

.EscapeChar_label_266:
	addi        t0, s2, 8
	ld          t1, 16(t0)
	ld          t2, 48(s2)
	addi        t2, t2, 1
	sd          t2, 48(s2)
	add         t2, t1, t2
	lb          s5, 0(t2)
	slli        t2, s8, 3
	addi        t3, s5, -48
	or          s8, t2, t3
	ld          t0, 24(t0)
	slt         t2, t2, t0
	bge         t2, t0, .EscapeChar_label_290

	// *** Basic block 49

	add         t0, t1, t2
	lb          t0, 0(t0)
	slti        t0, t0, 48
	not         t2, t0

	// *** Basic block 50

.EscapeChar_label_290:
	beqz        t2, .EscapeChar_label_297

	// *** Basic block 51

	add         t0, t1, t2
	lb          t0, 0(t0)
	li          t1, 55		// 0x37 ASCII '7'
	slt         t0, t1, t0
	not         t2, t0

	// *** Basic block 52

.EscapeChar_label_297:
	bnez        t2, .EscapeChar_label_266

	// *** Basic block 53

.EscapeChar_label_299:
	mv          a0, s8
	j           .EscapeChar_label_135

	// *** Basic block 54

.EscapeChar_label_303:
	ld          t0, 48(s2)
	addi        t0, t0, 1
	sd          t0, 48(s2)
	li          t0, 34		// 0x22 ASCII '"'
	beq         s5, t0, .EscapeChar_label_393

	// *** Basic block 55

	li          t0, 39		// 0x27 ASCII '''
	beq         s5, t0, .EscapeChar_label_389

	// *** Basic block 56

	li          t0, 63		// 0x3f ASCII '?'
	beq         s5, t0, .EscapeChar_label_397

	// *** Basic block 57

	li          t0, 92		// 0x5c ASCII '\'
	beq         s5, t0, .EscapeChar_label_385

	// *** Basic block 58

	li          t0, 97		// 0x61 ASCII 'a'
	beq         s5, t0, .EscapeChar_label_401

	// *** Basic block 59

	li          t0, 98		// 0x62 ASCII 'b'
	beq         s5, t0, .EscapeChar_label_381

	// *** Basic block 60

	li          t0, 102		// 0x66 ASCII 'f'
	beq         s5, t0, .EscapeChar_label_405

	// *** Basic block 61

	li          t0, 110		// 0x6e ASCII 'n'
	beq         s5, t0, .EscapeChar_label_373

	// *** Basic block 62

	li          t0, 114		// 0x72 ASCII 'r'
	beq         s5, t0, .EscapeChar_label_377

	// *** Basic block 63

	li          t0, 116		// 0x74 ASCII 't'
	beq         s5, t0, .EscapeChar_label_409

	// *** Basic block 64

	li          t0, 118		// 0x76 ASCII 'v'
	beq         s5, t0, .EscapeChar_label_413

	// *** Basic block 65

.EscapeChar_label_362:
	lla         a1, .str.42
	mv          a2, s5
	mv          a0, s2
	call        LexError

	// *** Basic block 66

	j           .EscapeChar_label_417

	// *** Basic block 67

.EscapeChar_label_373:
	li          a0, 10		// 0xa ASCII \xa
	j           .EscapeChar_label_135

	// *** Basic block 68

.EscapeChar_label_377:
	li          a0, 13		// 0xd ASCII \xd
	j           .EscapeChar_label_135

	// *** Basic block 69

.EscapeChar_label_381:
	li          a0, 8		// 0x8 ASCII \x8
	j           .EscapeChar_label_135

	// *** Basic block 70

.EscapeChar_label_385:
	li          a0, 92		// 0x5c ASCII '\'
	j           .EscapeChar_label_135

	// *** Basic block 71

.EscapeChar_label_389:
	li          a0, 39		// 0x27 ASCII '''
	j           .EscapeChar_label_135

	// *** Basic block 72

.EscapeChar_label_393:
	li          a0, 34		// 0x22 ASCII '"'
	j           .EscapeChar_label_135

	// *** Basic block 73

.EscapeChar_label_397:
	li          a0, 63		// 0x3f ASCII '?'
	j           .EscapeChar_label_135

	// *** Basic block 74

.EscapeChar_label_401:
	li          a0, 7		// 0x7 ASCII \x7
	j           .EscapeChar_label_135

	// *** Basic block 75

.EscapeChar_label_405:
	li          a0, 12		// 0xc ASCII \xc
	j           .EscapeChar_label_135

	// *** Basic block 76

.EscapeChar_label_409:
	li          a0, 9		// 0x9 ASCII \x9
	j           .EscapeChar_label_135

	// *** Basic block 77

.EscapeChar_label_413:
	li          a0, 11		// 0xb ASCII \xb
	j           .EscapeChar_label_135

	// *** Basic block 78

.EscapeChar_label_417:

	// *** Basic block 79

.EscapeChar_label_418:

	// *** Basic block 80

.EscapeChar_label_419:

	// *** Basic block 81

.EscapeChar_label_420:
	mv          a0, s5
	j           .EscapeChar_label_135
.func_end_EscapeChar:
	.size EscapeChar, .func_end_EscapeChar-EscapeChar

	.local  CollectIntegerSuffix
	.type CollectIntegerSuffix, @function

CollectIntegerSuffix:

	// *** Basic block 0

	.global StringClear
	.global toupper
	.global StringAppendChar
	.global strcpy
	.global StringSet
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
	addi        a0, s1, 136
	call        StringClear

	// *** Basic block 1

	addi        t0, s1, 8
	ld          t0, 16(t0)
	ld          t1, 48(s1)
	add         t0, t0, t1
	lb          a0, 0(t0)
	call        toupper

	// *** Basic block 2

	mv          s2, a0
	mv          s3, x0
	li          s4, 85		// 0x55 ASCII 'U'
	bne         s2, s4, .CollectIntegerSuffix_label_57

	// *** Basic block 3

	addi        a0, s1, 136
	mv          a1, s4
	call        StringAppendChar

	// *** Basic block 4

	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)
	li          s3, 1		// 0x1 ASCII \x1

	// *** Basic block 5

.CollectIntegerSuffix_label_57:
	addi        t0, s1, 8
	ld          t0, 16(t0)
	ld          t1, 48(s1)
	add         t0, t0, t1
	lb          a0, 0(t0)
	call        toupper

	// *** Basic block 6

	mv          s2, a0
	li          s5, 76		// 0x4c ASCII 'L'
	bne         s2, s5, .CollectIntegerSuffix_label_85

	// *** Basic block 7

	addi        a0, s1, 136
	mv          a1, s5
	call        StringAppendChar

	// *** Basic block 8

	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)

	// *** Basic block 9

.CollectIntegerSuffix_label_85:
	addi        t0, s1, 8
	ld          t0, 16(t0)
	ld          t1, 48(s1)
	add         t0, t0, t1
	lb          a0, 0(t0)
	call        toupper

	// *** Basic block 10

	mv          s2, a0
	bne         s2, s5, .CollectIntegerSuffix_label_112

	// *** Basic block 11

	addi        a0, s1, 136
	mv          a1, s5
	call        StringAppendChar

	// *** Basic block 12

	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)

	// *** Basic block 13

.CollectIntegerSuffix_label_112:
	not         t0, s3
	beqz        t0, .CollectIntegerSuffix_label_153

	// *** Basic block 14

	addi        t0, s1, 8
	ld          t0, 16(t0)
	ld          t1, 48(s1)
	add         t0, t0, t1
	lb          a0, 0(t0)
	call        toupper

	// *** Basic block 15

	mv          s2, a0
	bne         s2, s4, .CollectIntegerSuffix_label_152

	// *** Basic block 16

	sb          s4, -32(s0)
	addi        t0, s0, -32
	addi        a0, t0, 1
	addi        t0, s1, 136
	ld          a1, 16(t0)
	call        strcpy

	// *** Basic block 17

	addi        a0, s1, 136
	addi        a1, s0, -32
	call        StringSet

	// *** Basic block 18

	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)

	// *** Basic block 19

.CollectIntegerSuffix_label_152:

	// *** Basic block 20

.CollectIntegerSuffix_label_153:
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
.func_end_CollectIntegerSuffix:
	.size CollectIntegerSuffix, .func_end_CollectIntegerSuffix-CollectIntegerSuffix

	.local  CollectFloatingSuffix
	.type CollectFloatingSuffix, @function

CollectFloatingSuffix:

	// *** Basic block 0

	.global StringClear
	.global toupper
	.global StringAppendChar
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
	addi        a0, s1, 136
	call        StringClear

	// *** Basic block 1

	addi        t0, s1, 8
	ld          t0, 16(t0)
	ld          t1, 48(s1)
	add         t0, t0, t1
	lb          s2, 0(t0)
	mv          a0, s2
	call        toupper

	// *** Basic block 2

	mv          s3, a0
	li          s4, 70		// 0x46 ASCII 'F'
	bne         s3, s4, .CollectFloatingSuffix_label_53

	// *** Basic block 3

	addi        a0, s1, 136
	mv          a1, s4
	call        StringAppendChar

	// *** Basic block 4

	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)
	j           .CollectFloatingSuffix_label_76

	// *** Basic block 5

.CollectFloatingSuffix_label_53:
	mv          a0, s2
	call        toupper

	// *** Basic block 6

	mv          s3, a0
	li          s2, 76		// 0x4c ASCII 'L'
	bne         s3, s2, .CollectFloatingSuffix_label_75

	// *** Basic block 7

	addi        a0, s1, 136
	mv          a1, s2
	call        StringAppendChar

	// *** Basic block 8

	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)

	// *** Basic block 9

.CollectFloatingSuffix_label_75:

	// *** Basic block 10

.CollectFloatingSuffix_label_76:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CollectFloatingSuffix:
	.size CollectFloatingSuffix, .func_end_CollectFloatingSuffix-CollectFloatingSuffix

	.local  CollectStringLiteral
	.type CollectStringLiteral, @function

CollectStringLiteral:

	// *** Basic block 0

	.global StringClear
	.local EscapeChar
	.global StringAppendChar
	.global LexError
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
	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)
	addi        a0, s1, 80
	call        StringClear

	// *** Basic block 1

	ld          t0, 48(s1)
	addi        t1, s1, 8
	ld          t1, 24(t1)
	sub         t2, t0, t1
	seqz        s2, t2
	bge         t0, t1, .CollectStringLiteral_label_120

	// *** Basic block 2

.CollectStringLiteral_label_41:
	addi        t0, s1, 8
	ld          t0, 16(t0)
	ld          t1, 48(s1)
	addi        t1, t1, 1
	sd          t1, 48(s1)
	add         t0, t0, t1
	lb          s3, 0(t0)
	li          t0, 10		// 0xa ASCII \xa
	bne         s3, t0, .CollectStringLiteral_label_60

	// *** Basic block 3

	li          s2, 1		// 0x1 ASCII \x1
	j           .CollectStringLiteral_label_120

	// *** Basic block 4

.CollectStringLiteral_label_60:
	li          t0, 92		// 0x5c ASCII '\'
	bne         s3, t0, .CollectStringLiteral_label_99

	// *** Basic block 5

	addi        a1, s0, -32
	mv          a0, s1
	call        EscapeChar

	// *** Basic block 6

	mv          s4, a0
	mv          s5, x0
	lw          s6, -32(s0)
	bge         x0, s6, .CollectStringLiteral_label_97

	// *** Basic block 7

.CollectStringLiteral_label_81:
	addi        a0, s1, 80
	slli        t0, s5, 3
	sra         t0, s4, t0
	andi        t0, t0, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	call        StringAppendChar

	// *** Basic block 8

.CollectStringLiteral_label_93:
	addi        s5, s5, 1
	bge         s5, s6, .CollectStringLiteral_label_81

	// *** Basic block 9

.CollectStringLiteral_label_97:
	j           .CollectStringLiteral_label_112

	// *** Basic block 10

.CollectStringLiteral_label_99:
	li          t0, 34		// 0x22 ASCII '"'
	beq         s3, t0, .CollectStringLiteral_label_120

	// *** Basic block 11

.CollectStringLiteral_label_105:
	addi        a0, s1, 80
	mv          a1, s3
	call        StringAppendChar

	// *** Basic block 12

.CollectStringLiteral_label_111:

	// *** Basic block 13

.CollectStringLiteral_label_112:
	ld          t0, 48(s1)
	addi        t1, s1, 8
	ld          t1, 24(t1)
	blt         t0, t1, .CollectStringLiteral_label_41

	// *** Basic block 14

.CollectStringLiteral_label_120:
	beqz        s2, .CollectStringLiteral_label_128

	// *** Basic block 15

	lla         a1, .str.43
	mv          a0, s1
	call        LexError

	// *** Basic block 16

.CollectStringLiteral_label_128:
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
.func_end_CollectStringLiteral:
	.size CollectStringLiteral, .func_end_CollectStringLiteral-CollectStringLiteral

	.local  CollectWideStringLiteral
	.type CollectWideStringLiteral, @function

CollectWideStringLiteral:

	// *** Basic block 0

	.global StringClear
	.local EscapeChar
	.global StringAppendChar
	.global LexError
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
	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)
	addi        a0, s1, 80
	call        StringClear

	// *** Basic block 1

	ld          t0, 48(s1)
	addi        t1, s1, 8
	ld          t1, 24(t1)
	sub         t2, t0, t1
	seqz        s2, t2
	bge         t0, t1, .CollectWideStringLiteral_label_136

	// *** Basic block 2

.CollectWideStringLiteral_label_44:
	addi        t0, s1, 8
	ld          t0, 16(t0)
	ld          t1, 48(s1)
	addi        t1, t1, 1
	sd          t1, 48(s1)
	add         t0, t0, t1
	lb          s3, 0(t0)
	li          t0, 10		// 0xa ASCII \xa
	bne         s3, t0, .CollectWideStringLiteral_label_63

	// *** Basic block 3

	li          s2, 1		// 0x1 ASCII \x1
	j           .CollectWideStringLiteral_label_136

	// *** Basic block 4

.CollectWideStringLiteral_label_63:
	li          t0, 92		// 0x5c ASCII '\'
	bne         s3, t0, .CollectWideStringLiteral_label_100

	// *** Basic block 5

	addi        a1, s0, -32
	mv          a0, s1
	call        EscapeChar

	// *** Basic block 6

	mv          s4, a0
	mv          s5, x0

	// *** Basic block 7

.CollectWideStringLiteral_label_82:
	addi        a0, s1, 80
	slli        t0, s5, 3
	sra         t0, s4, t0
	andi        t0, t0, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	call        StringAppendChar

	// *** Basic block 8

.CollectWideStringLiteral_label_93:
	addi        s5, s5, 1
	li          t0, 4		// 0x4 ASCII \x4
	bge         s5, t0, .CollectWideStringLiteral_label_82

	// *** Basic block 9

.CollectWideStringLiteral_label_98:
	j           .CollectWideStringLiteral_label_128

	// *** Basic block 10

.CollectWideStringLiteral_label_100:
	li          t0, 34		// 0x22 ASCII '"'
	beq         s3, t0, .CollectWideStringLiteral_label_136

	// *** Basic block 11

.CollectWideStringLiteral_label_106:
	addi        a0, s1, 80
	mv          a1, s3
	call        StringAppendChar

	// *** Basic block 12

	mv          s3, x0

	// *** Basic block 13

.CollectWideStringLiteral_label_115:
	addi        a0, s1, 80
	mv          a1, x0
	call        StringAppendChar

	// *** Basic block 14

.CollectWideStringLiteral_label_121:
	addi        s3, s3, 1
	li          t0, 3		// 0x3 ASCII \x3
	bge         s3, t0, .CollectWideStringLiteral_label_115

	// *** Basic block 15

.CollectWideStringLiteral_label_126:

	// *** Basic block 16

.CollectWideStringLiteral_label_127:

	// *** Basic block 17

.CollectWideStringLiteral_label_128:
	ld          t0, 48(s1)
	addi        t1, s1, 8
	ld          t1, 24(t1)
	blt         t0, t1, .CollectWideStringLiteral_label_44

	// *** Basic block 18

.CollectWideStringLiteral_label_136:
	beqz        s2, .CollectWideStringLiteral_label_144

	// *** Basic block 19

	lla         a1, .str.44
	mv          a0, s1
	call        LexError

	// *** Basic block 20

.CollectWideStringLiteral_label_144:
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
.func_end_CollectWideStringLiteral:
	.size CollectWideStringLiteral, .func_end_CollectWideStringLiteral-CollectWideStringLiteral

	.local  CollectCharConst
	.type CollectCharConst, @function

CollectCharConst:

	// *** Basic block 0

	.local EscapeChar
	.global LexError
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
	mv          s1, a0
	ld          t0, 48(s1)
	addi        t1, t0, 1
	sd          t1, 48(s1)
	mv          s2, x0
	mv          s3, x0
	mv          s4, x0
	addi        t1, s1, 8
	ld          t1, 24(t1)
	bge         t0, t1, .CollectCharConst_label_107

	// *** Basic block 1

.CollectCharConst_label_37:
	addi        s5, s1, 8
	ld          t0, 16(s5)
	ld          t1, 48(s1)
	addi        t1, t1, 1
	sd          t1, 48(s1)
	add         t0, t0, t1
	lb          s6, 0(t0)
	li          t0, 10		// 0xa ASCII \xa
	bne         s6, t0, .CollectCharConst_label_56

	// *** Basic block 2

	li          s4, 1		// 0x1 ASCII \x1
	j           .CollectCharConst_label_107

	// *** Basic block 3

.CollectCharConst_label_56:
	li          t0, 92		// 0x5c ASCII '\'
	bne         s6, t0, .CollectCharConst_label_91

	// *** Basic block 4

	addi        a1, s0, -32
	mv          a0, s1
	call        EscapeChar

	// *** Basic block 5

	mv          s7, a0
	mv          s8, x0
	lw          t0, -32(s0)
	bge         x0, t0, .CollectCharConst_label_88

	// *** Basic block 6

.CollectCharConst_label_77:
	slli        t2, s2, 8
	slli        t3, s8, 3
	sra         t3, s7, t3
	andi        t3, t3, 255
	or          s2, t2, t3

	// *** Basic block 7

.CollectCharConst_label_84:
	addi        s8, s8, 1
	bge         s8, t0, .CollectCharConst_label_77

	// *** Basic block 8

.CollectCharConst_label_88:
	add         s3, s3, t0
	j           .CollectCharConst_label_102

	// *** Basic block 9

.CollectCharConst_label_91:
	li          t0, 39		// 0x27 ASCII '''
	beq         s6, t0, .CollectCharConst_label_107

	// *** Basic block 10

.CollectCharConst_label_97:
	slli        t0, s2, 8
	or          s2, t0, s6
	addi        s3, s3, 1

	// *** Basic block 11

.CollectCharConst_label_101:

	// *** Basic block 12

.CollectCharConst_label_102:
	ld          t0, 24(s5)
	blt         t1, t0, .CollectCharConst_label_37

	// *** Basic block 13

.CollectCharConst_label_107:
	li          t0, 4		// 0x4 ASCII \x4
	bge         t0, s3, .CollectCharConst_label_118

	// *** Basic block 14

	lla         a1, .str.45
	mv          a0, s1
	call        LexError

	// *** Basic block 15

.CollectCharConst_label_118:
	seqz        s5, s3
	beqz        s3, .CollectCharConst_label_123

	// *** Basic block 16

	mv          s5, s4

	// *** Basic block 17

.CollectCharConst_label_123:
	beqz        s5, .CollectCharConst_label_131

	// *** Basic block 18

	lla         a1, .str.46
	mv          a0, s1
	call        LexError

	// *** Basic block 19

.CollectCharConst_label_131:
	mv          a0, s2

	// *** Basic block 20

.CollectCharConst_label_134:
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
.func_end_CollectCharConst:
	.size CollectCharConst, .func_end_CollectCharConst-CollectCharConst

	.local  CollectWideCharConst
	.type CollectWideCharConst, @function

CollectWideCharConst:

	// *** Basic block 0

	.local EscapeChar
	.global LexError
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
	ld          t0, 48(s1)
	addi        t1, t0, 1
	sd          t1, 48(s1)
	mv          s2, x0
	mv          s3, x0
	mv          s4, x0
	addi        t1, s1, 8
	ld          t1, 24(t1)
	bge         t0, t1, .CollectWideCharConst_label_87

	// *** Basic block 1

.CollectWideCharConst_label_34:
	addi        s5, s1, 8
	ld          t0, 16(s5)
	ld          t1, 48(s1)
	addi        t1, t1, 1
	sd          t1, 48(s1)
	add         t0, t0, t1
	lb          s6, 0(t0)
	li          t0, 10		// 0xa ASCII \xa
	bne         s6, t0, .CollectWideCharConst_label_53

	// *** Basic block 2

	li          s4, 1		// 0x1 ASCII \x1
	j           .CollectWideCharConst_label_87

	// *** Basic block 3

.CollectWideCharConst_label_53:
	li          t0, 92		// 0x5c ASCII '\'
	bne         s6, t0, .CollectWideCharConst_label_72

	// *** Basic block 4

	addi        a1, s0, -32
	mv          a0, s1
	call        EscapeChar

	// *** Basic block 5

	mv          s7, a0
	mv          s2, s7
	addi        s3, s3, 1
	j           .CollectWideCharConst_label_82

	// *** Basic block 6

.CollectWideCharConst_label_72:
	li          t0, 39		// 0x27 ASCII '''
	beq         s6, t0, .CollectWideCharConst_label_87

	// *** Basic block 7

.CollectWideCharConst_label_78:
	mv          s2, s6
	addi        s3, s3, 1

	// *** Basic block 8

.CollectWideCharConst_label_81:

	// *** Basic block 9

.CollectWideCharConst_label_82:
	ld          t0, 24(s5)
	blt         t1, t0, .CollectWideCharConst_label_34

	// *** Basic block 10

.CollectWideCharConst_label_87:
	li          t0, 1		// 0x1 ASCII \x1
	bge         t0, s3, .CollectWideCharConst_label_98

	// *** Basic block 11

	lla         a1, .str.47
	mv          a0, s1
	call        LexError

	// *** Basic block 12

.CollectWideCharConst_label_98:
	seqz        s5, s3
	beqz        s3, .CollectWideCharConst_label_103

	// *** Basic block 13

	mv          s5, s4

	// *** Basic block 14

.CollectWideCharConst_label_103:
	beqz        s5, .CollectWideCharConst_label_111

	// *** Basic block 15

	lla         a1, .str.48
	mv          a0, s1
	call        LexError

	// *** Basic block 16

.CollectWideCharConst_label_111:
	mv          a0, s2

	// *** Basic block 17

.CollectWideCharConst_label_114:
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
.func_end_CollectWideCharConst:
	.size CollectWideCharConst, .func_end_CollectWideCharConst-CollectWideCharConst

	.global IsIdentifierChar
	.type IsIdentifierChar, @function

IsIdentifierChar:

	// *** Basic block 0

	.global isalpha
	.global isalnum
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
	beqz        a2, .IsIdentifierChar_label_38

	// *** Basic block 1

	mv          a0, s1
	call        isalpha

	// *** Basic block 2

	mv          s3, a0
	bnez        a0, .IsIdentifierChar_label_29

	// *** Basic block 3

	addi        t0, s1, -95
	seqz        s3, t0

	// *** Basic block 4

.IsIdentifierChar_label_29:
	beqz        s3, .IsIdentifierChar_label_36

	// *** Basic block 5

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 6

.IsIdentifierChar_label_33:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.IsIdentifierChar_label_36:
	j           .IsIdentifierChar_label_53

	// *** Basic block 8

.IsIdentifierChar_label_38:
	mv          a0, s1
	call        isalnum

	// *** Basic block 9

	mv          s3, a0
	bnez        a0, .IsIdentifierChar_label_47

	// *** Basic block 10

	addi        t0, s1, -95
	seqz        s3, t0

	// *** Basic block 11

.IsIdentifierChar_label_47:
	beqz        s3, .IsIdentifierChar_label_52

	// *** Basic block 12

	li          a0, 1		// 0x1 ASCII \x1
	j           .IsIdentifierChar_label_33

	// *** Basic block 13

.IsIdentifierChar_label_52:

	// *** Basic block 14

.IsIdentifierChar_label_53:
	lb          t0, 178(s2)
	beqz        t0, .IsIdentifierChar_label_69

	// *** Basic block 15

	addi        t1, s1, -46
	seqz        t0, t1
	li          t1, 46		// 0x2e ASCII '.'
	beq         s1, t1, .IsIdentifierChar_label_68

	// *** Basic block 16

	addi        t1, s1, -64
	seqz        t0, t1

	// *** Basic block 17

.IsIdentifierChar_label_68:

	// *** Basic block 18

.IsIdentifierChar_label_69:
	beqz        t0, .IsIdentifierChar_label_74

	// *** Basic block 19

	li          a0, 1		// 0x1 ASCII \x1
	j           .IsIdentifierChar_label_33

	// *** Basic block 20

.IsIdentifierChar_label_74:
	mv          a0, x0
	j           .IsIdentifierChar_label_33
.func_end_IsIdentifierChar:
	.size IsIdentifierChar, .func_end_IsIdentifierChar-IsIdentifierChar

	.local  CollectIdentifierOrWide
	.type CollectIdentifierOrWide, @function

CollectIdentifierOrWide:

	// *** Basic block 0

	.local CurrentChar
	.local LookaheadChar
	.local CollectWideStringLiteral
	.local CollectWideCharConst
	.global StringClear
	.global IsIdentifierChar
	.global StringAppendChar
	.local IsReservedWord
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
	call        CurrentChar

	// *** Basic block 1

	li          t0, 76		// 0x4c ASCII 'L'
	bne         a0, t0, .CollectIdentifierOrWide_label_85

	// *** Basic block 2

	mv          a0, s1
	call        LookaheadChar

	// *** Basic block 3

	li          t0, 34		// 0x22 ASCII '"'
	bne         a0, t0, .CollectIdentifierOrWide_label_61

	// *** Basic block 4

	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)
	mv          a0, s1
	call        CollectWideStringLiteral

	// *** Basic block 5

	li          t0, 5		// 0x5 ASCII \x5
	sw          t0, 72(s1)

	// *** Basic block 6

.CollectIdentifierOrWide_label_58:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.CollectIdentifierOrWide_label_61:
	mv          a0, s1
	call        LookaheadChar

	// *** Basic block 8

	li          t0, 39		// 0x27 ASCII '''
	bne         a0, t0, .CollectIdentifierOrWide_label_83

	// *** Basic block 9

	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)
	mv          a0, s1
	call        CollectWideCharConst

	// *** Basic block 10

	sd          a0, 120(s1)
	li          t0, 7		// 0x7 ASCII \x7
	sw          t0, 72(s1)
	j           .CollectIdentifierOrWide_label_58

	// *** Basic block 11

.CollectIdentifierOrWide_label_83:

	// *** Basic block 12

.CollectIdentifierOrWide_label_84:

	// *** Basic block 13

.CollectIdentifierOrWide_label_85:
	addi        a0, s1, 80
	call        StringClear

	// *** Basic block 14

	ld          t0, 48(s1)
	addi        t1, s1, 8
	ld          t1, 24(t1)
	bge         t0, t1, .CollectIdentifierOrWide_label_131

	// *** Basic block 15

.CollectIdentifierOrWide_label_96:
	addi        t0, s1, 8
	ld          t0, 16(t0)
	ld          t1, 48(s1)
	add         t0, t0, t1
	lb          s2, 0(t0)
	mv          a2, x0
	mv          a1, s2
	mv          a0, s1
	call        IsIdentifierChar

	// *** Basic block 16

	not         t0, a0
	bnez        t0, .CollectIdentifierOrWide_label_131

	// *** Basic block 17

.CollectIdentifierOrWide_label_116:
	addi        a0, s1, 80
	mv          a1, s2
	call        StringAppendChar

	// *** Basic block 18

	ld          t0, 48(s1)
	addi        t1, t0, 1
	sd          t1, 48(s1)
	addi        t1, s1, 8
	ld          t1, 24(t1)
	blt         t0, t1, .CollectIdentifierOrWide_label_96

	// *** Basic block 19

.CollectIdentifierOrWide_label_131:
	lb          t0, 176(s1)
	bnez        t0, .CollectIdentifierOrWide_label_138

	// *** Basic block 20

	lb          t0, 178(s1)

	// *** Basic block 21

.CollectIdentifierOrWide_label_138:
	beqz        t0, .CollectIdentifierOrWide_label_144

	// *** Basic block 22

	li          t0, 3		// 0x3 ASCII \x3
	sw          t0, 72(s1)
	j           .CollectIdentifierOrWide_label_58

	// *** Basic block 23

.CollectIdentifierOrWide_label_144:
	addi        t0, s1, 80
	ld          a0, 16(t0)
	addi        a1, s1, 72
	call        IsReservedWord

	// *** Basic block 24

	not         t0, a0
	beqz        t0, .CollectIdentifierOrWide_label_157

	// *** Basic block 25

	li          t0, 3		// 0x3 ASCII \x3
	sw          t0, 72(s1)

	// *** Basic block 26

.CollectIdentifierOrWide_label_157:
	j           .CollectIdentifierOrWide_label_58
.func_end_CollectIdentifierOrWide:
	.size CollectIdentifierOrWide, .func_end_CollectIdentifierOrWide-CollectIdentifierOrWide

	.local  CollectOperator
	.type CollectOperator, @function

CollectOperator:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        t1, t0, 8
	ld          t1, 16(t1)
	ld          t2, 48(t0)
	add         t1, t1, t2
	lb          t2, 0(t1)
	li          t1, 58		// 0x3a ASCII ':'
	blt         t2, t1, .CollectOperator_label_162

	// *** Basic block 1

	beq         t2, t1, .CollectOperator_label_803

	// *** Basic block 2

	li          t1, 59		// 0x3b ASCII ';'
	beq         t2, t1, .CollectOperator_label_812

	// *** Basic block 3

	li          t1, 60		// 0x3c ASCII '<'
	beq         t2, t1, .CollectOperator_label_521

	// *** Basic block 4

	li          t3, 61		// 0x3d ASCII '='
	beq         t2, t3, .CollectOperator_label_432

	// *** Basic block 5

	li          t4, 62		// 0x3e ASCII '>'
	beq         t2, t4, .CollectOperator_label_460

	// *** Basic block 6

	li          t5, 63		// 0x3f ASCII '?'
	beq         t2, t5, .CollectOperator_label_794

	// *** Basic block 7

	li          t5, 91		// 0x5b ASCII '['
	beq         t2, t5, .CollectOperator_label_396

	// *** Basic block 8

	li          t5, 93		// 0x5d ASCII ']'
	beq         t2, t5, .CollectOperator_label_405

	// *** Basic block 9

	li          t5, 94		// 0x5e ASCII '^'
	beq         t2, t5, .CollectOperator_label_680

	// *** Basic block 10

	li          t5, 123		// 0x7b ASCII '{'
	beq         t2, t5, .CollectOperator_label_414

	// *** Basic block 11

	li          t5, 124		// 0x7c ASCII '|'
	beq         t2, t5, .CollectOperator_label_708

	// *** Basic block 12

	li          t6, 125		// 0x7d ASCII '}'
	beq         t2, t6, .CollectOperator_label_423

	// *** Basic block 13

	li          t6, 126		// 0x7e ASCII '~'
	beq         t2, t6, .CollectOperator_label_821

	// *** Basic block 14

	j           .CollectOperator_label_843

	// *** Basic block 15

.CollectOperator_label_162:
	li          t1, 33		// 0x21 ASCII '!'
	beq         t2, t1, .CollectOperator_label_652

	// *** Basic block 16

	li          t3, 35		// 0x23 ASCII '#'
	beq         t2, t3, .CollectOperator_label_830

	// *** Basic block 17

	li          t4, 37		// 0x25 ASCII '%'
	beq         t2, t4, .CollectOperator_label_582

	// *** Basic block 18

	li          t4, 38		// 0x26 ASCII '&'
	beq         t2, t4, .CollectOperator_label_610

	// *** Basic block 19

	li          t5, 40		// 0x28 ASCII '('
	beq         t2, t5, .CollectOperator_label_378

	// *** Basic block 20

	li          t6, 41		// 0x29 ASCII ')'
	beq         t2, t6, .CollectOperator_label_387

	// *** Basic block 21

	li          a0, 42		// 0x2a ASCII '*'
	beq         t2, a0, .CollectOperator_label_322

	// *** Basic block 22

	li          a1, 43		// 0x2b ASCII '+'
	beq         t2, a1, .CollectOperator_label_224

	// *** Basic block 23

	li          a2, 44		// 0x2c ASCII ','
	beq         t2, a2, .CollectOperator_label_750

	// *** Basic block 24

	li          a2, 45		// 0x2d ASCII '-'
	beq         t2, a2, .CollectOperator_label_266

	// *** Basic block 25

	li          a3, 46		// 0x2e ASCII '.'
	beq         t2, a3, .CollectOperator_label_759

	// *** Basic block 26

	li          a4, 47		// 0x2f ASCII '/'
	beq         t2, a4, .CollectOperator_label_350

	// *** Basic block 27

	j           .CollectOperator_label_843

	// *** Basic block 28

.CollectOperator_label_224:
	addi        t1, t0, 8
	ld          t1, 16(t1)
	ld          t3, 48(t0)
	addi        t3, t3, 1
	sd          t3, 48(t0)
	add         t1, t1, t3
	lb          t2, 0(t1)
	li          t1, 61		// 0x3d ASCII '='
	bne         t2, t1, .CollectOperator_label_246

	// *** Basic block 29

	sw          t6, 72(t0)
	ld          t1, 48(t0)
	addi        t1, t1, 1
	sd          t1, 48(t0)
	j           .CollectOperator_label_264

	// *** Basic block 30

.CollectOperator_label_246:
	bne         t2, a1, .CollectOperator_label_259

	// *** Basic block 31

	sw          a0, 72(t0)
	ld          t1, 48(t0)
	addi        t1, t1, 1
	sd          t1, 48(t0)
	j           .CollectOperator_label_263

	// *** Basic block 32

.CollectOperator_label_259:
	sw          t5, 72(t0)

	// *** Basic block 33

.CollectOperator_label_263:

	// *** Basic block 34

.CollectOperator_label_264:
	j           .CollectOperator_label_845

	// *** Basic block 35

.CollectOperator_label_266:
	addi        a3, t0, 8
	ld          a3, 16(a3)
	ld          a4, 48(t0)
	addi        a4, a4, 1
	sd          a4, 48(t0)
	add         a3, a3, a4
	lb          t2, 0(a3)
	li          a3, 61		// 0x3d ASCII '='
	bne         t2, a3, .CollectOperator_label_288

	// *** Basic block 36

	li          a3, 34		// 0x22 ASCII '"'
	sw          a3, 72(t0)
	ld          a3, 48(t0)
	addi        a3, a3, 1
	sd          a3, 48(t0)
	j           .CollectOperator_label_320

	// *** Basic block 37

.CollectOperator_label_288:
	bne         t2, a2, .CollectOperator_label_301

	// *** Basic block 38

	sw          t3, 72(t0)
	ld          t3, 48(t0)
	addi        t3, t3, 1
	sd          t3, 48(t0)
	j           .CollectOperator_label_319

	// *** Basic block 39

.CollectOperator_label_301:
	li          t3, 62		// 0x3e ASCII '>'
	bne         t2, t3, .CollectOperator_label_314

	// *** Basic block 40

	li          t3, 11		// 0xb ASCII \xb
	sw          t3, 72(t0)
	ld          t3, 48(t0)
	addi        t3, t3, 1
	sd          t3, 48(t0)
	j           .CollectOperator_label_318

	// *** Basic block 41

.CollectOperator_label_314:
	sw          t1, 72(t0)

	// *** Basic block 42

.CollectOperator_label_318:

	// *** Basic block 43

.CollectOperator_label_319:

	// *** Basic block 44

.CollectOperator_label_320:
	j           .CollectOperator_label_845

	// *** Basic block 45

.CollectOperator_label_322:
	addi        t1, t0, 8
	ld          t1, 16(t1)
	ld          t3, 48(t0)
	addi        t3, t3, 1
	sd          t3, 48(t0)
	add         t1, t1, t3
	lb          t2, 0(t1)
	li          t1, 61		// 0x3d ASCII '='
	bne         t2, t1, .CollectOperator_label_344

	// *** Basic block 46

	li          t1, 53		// 0x35 ASCII '5'
	sw          t1, 72(t0)
	ld          t1, 48(t0)
	addi        t1, t1, 1
	sd          t1, 48(t0)
	j           .CollectOperator_label_348

	// *** Basic block 47

.CollectOperator_label_344:
	li          t1, 52		// 0x34 ASCII '4'
	sw          t1, 72(t0)

	// *** Basic block 48

.CollectOperator_label_348:
	j           .CollectOperator_label_845

	// *** Basic block 49

.CollectOperator_label_350:
	addi        a4, t0, 8
	ld          a4, 16(a4)
	ld          a5, 48(t0)
	addi        a5, a5, 1
	sd          a5, 48(t0)
	add         a4, a4, a5
	lb          t2, 0(a4)
	li          a4, 61		// 0x3d ASCII '='
	bne         t2, a4, .CollectOperator_label_372

	// *** Basic block 50

	li          a4, 51		// 0x33 ASCII '3'
	sw          a4, 72(t0)
	ld          a4, 48(t0)
	addi        a4, a4, 1
	sd          a4, 48(t0)
	j           .CollectOperator_label_376

	// *** Basic block 51

.CollectOperator_label_372:
	li          a4, 50		// 0x32 ASCII '2'
	sw          a4, 72(t0)

	// *** Basic block 52

.CollectOperator_label_376:
	j           .CollectOperator_label_845

	// *** Basic block 53

.CollectOperator_label_378:
	li          t1, 29		// 0x1d ASCII \x1d
	sw          t1, 72(t0)
	ld          t1, 48(t0)
	addi        t1, t1, 1
	sd          t1, 48(t0)
	j           .CollectOperator_label_845

	// *** Basic block 54

.CollectOperator_label_387:
	li          t1, 45		// 0x2d ASCII '-'
	sw          t1, 72(t0)
	ld          t1, 48(t0)
	addi        t1, t1, 1
	sd          t1, 48(t0)
	j           .CollectOperator_label_845

	// *** Basic block 55

.CollectOperator_label_396:
	li          t5, 32		// 0x20 ASCII ' '
	sw          t5, 72(t0)
	ld          t5, 48(t0)
	addi        t5, t5, 1
	sd          t5, 48(t0)
	j           .CollectOperator_label_845

	// *** Basic block 56

.CollectOperator_label_405:
	li          t5, 48		// 0x30 ASCII '0'
	sw          t5, 72(t0)
	ld          t5, 48(t0)
	addi        t5, t5, 1
	sd          t5, 48(t0)
	j           .CollectOperator_label_845

	// *** Basic block 57

.CollectOperator_label_414:
	li          t5, 24		// 0x18 ASCII \x18
	sw          t5, 72(t0)
	ld          t5, 48(t0)
	addi        t5, t5, 1
	sd          t5, 48(t0)
	j           .CollectOperator_label_845

	// *** Basic block 58

.CollectOperator_label_423:
	li          t6, 44		// 0x2c ASCII ','
	sw          t6, 72(t0)
	ld          t6, 48(t0)
	addi        t6, t6, 1
	sd          t6, 48(t0)
	j           .CollectOperator_label_845

	// *** Basic block 59

.CollectOperator_label_432:
	addi        t4, t0, 8
	ld          t4, 16(t4)
	ld          t5, 48(t0)
	addi        t5, t5, 1
	sd          t5, 48(t0)
	add         t4, t4, t5
	lb          t2, 0(t4)
	bne         t2, t3, .CollectOperator_label_454

	// *** Basic block 60

	li          t3, 21		// 0x15 ASCII \x15
	sw          t3, 72(t0)
	ld          t3, 48(t0)
	addi        t3, t3, 1
	sd          t3, 48(t0)
	j           .CollectOperator_label_458

	// *** Basic block 61

.CollectOperator_label_454:
	li          t3, 12		// 0xc ASCII \xc
	sw          t3, 72(t0)

	// *** Basic block 62

.CollectOperator_label_458:
	j           .CollectOperator_label_845

	// *** Basic block 63

.CollectOperator_label_460:
	addi        t5, t0, 8
	ld          t5, 16(t5)
	ld          t6, 48(t0)
	addi        t6, t6, 1
	sd          t6, 48(t0)
	add         t5, t5, t6
	lb          t2, 0(t5)
	bne         t2, t3, .CollectOperator_label_482

	// *** Basic block 64

	li          t5, 23		// 0x17 ASCII \x17
	sw          t5, 72(t0)
	ld          t5, 48(t0)
	addi        t5, t5, 1
	sd          t5, 48(t0)
	j           .CollectOperator_label_519

	// *** Basic block 65

.CollectOperator_label_482:
	bne         t2, t4, .CollectOperator_label_514

	// *** Basic block 66

	addi        t4, t0, 8
	ld          t4, 16(t4)
	ld          t5, 48(t0)
	addi        t5, t5, 1
	sd          t5, 48(t0)
	add         t4, t4, t5
	lb          t2, 0(t4)
	bne         t2, t3, .CollectOperator_label_508

	// *** Basic block 67

	li          t4, 47		// 0x2f ASCII '/'
	sw          t4, 72(t0)
	ld          t4, 48(t0)
	addi        t4, t4, 1
	sd          t4, 48(t0)
	j           .CollectOperator_label_512

	// *** Basic block 68

.CollectOperator_label_508:
	li          t4, 46		// 0x2e ASCII '.'
	sw          t4, 72(t0)

	// *** Basic block 69

.CollectOperator_label_512:
	j           .CollectOperator_label_518

	// *** Basic block 70

.CollectOperator_label_514:
	li          t4, 22		// 0x16 ASCII \x16
	sw          t4, 72(t0)

	// *** Basic block 71

.CollectOperator_label_518:

	// *** Basic block 72

.CollectOperator_label_519:
	j           .CollectOperator_label_845

	// *** Basic block 73

.CollectOperator_label_521:
	addi        t3, t0, 8
	ld          t3, 16(t3)
	ld          t4, 48(t0)
	addi        t4, t4, 1
	sd          t4, 48(t0)
	add         t3, t3, t4
	lb          t2, 0(t3)
	li          t3, 61		// 0x3d ASCII '='
	bne         t2, t3, .CollectOperator_label_543

	// *** Basic block 74

	li          t4, 26		// 0x1a ASCII \x1a
	sw          t4, 72(t0)
	ld          t4, 48(t0)
	addi        t4, t4, 1
	sd          t4, 48(t0)
	j           .CollectOperator_label_580

	// *** Basic block 75

.CollectOperator_label_543:
	bne         t2, t1, .CollectOperator_label_575

	// *** Basic block 76

	addi        t1, t0, 8
	ld          t1, 16(t1)
	ld          t4, 48(t0)
	addi        t4, t4, 1
	sd          t4, 48(t0)
	add         t1, t1, t4
	lb          t2, 0(t1)
	bne         t2, t3, .CollectOperator_label_569

	// *** Basic block 77

	li          t1, 31		// 0x1f ASCII \x1f
	sw          t1, 72(t0)
	ld          t1, 48(t0)
	addi        t1, t1, 1
	sd          t1, 48(t0)
	j           .CollectOperator_label_573

	// *** Basic block 78

.CollectOperator_label_569:
	li          t1, 30		// 0x1e ASCII \x1e
	sw          t1, 72(t0)

	// *** Basic block 79

.CollectOperator_label_573:
	j           .CollectOperator_label_579

	// *** Basic block 80

.CollectOperator_label_575:
	li          t1, 25		// 0x19 ASCII \x19
	sw          t1, 72(t0)

	// *** Basic block 81

.CollectOperator_label_579:

	// *** Basic block 82

.CollectOperator_label_580:
	j           .CollectOperator_label_845

	// *** Basic block 83

.CollectOperator_label_582:
	addi        t1, t0, 8
	ld          t1, 16(t1)
	ld          t3, 48(t0)
	addi        t3, t3, 1
	sd          t3, 48(t0)
	add         t1, t1, t3
	lb          t2, 0(t1)
	li          t1, 61		// 0x3d ASCII '='
	bne         t2, t1, .CollectOperator_label_604

	// *** Basic block 84

	li          t1, 39		// 0x27 ASCII '''
	sw          t1, 72(t0)
	ld          t1, 48(t0)
	addi        t1, t1, 1
	sd          t1, 48(t0)
	j           .CollectOperator_label_608

	// *** Basic block 85

.CollectOperator_label_604:
	li          t1, 38		// 0x26 ASCII '&'
	sw          t1, 72(t0)

	// *** Basic block 86

.CollectOperator_label_608:
	j           .CollectOperator_label_845

	// *** Basic block 87

.CollectOperator_label_610:
	addi        t1, t0, 8
	ld          t1, 16(t1)
	ld          t3, 48(t0)
	addi        t3, t3, 1
	sd          t3, 48(t0)
	add         t1, t1, t3
	lb          t2, 0(t1)
	li          t1, 61		// 0x3d ASCII '='
	bne         t2, t1, .CollectOperator_label_632

	// *** Basic block 88

	li          t1, 10		// 0xa ASCII \xa
	sw          t1, 72(t0)
	ld          t1, 48(t0)
	addi        t1, t1, 1
	sd          t1, 48(t0)
	j           .CollectOperator_label_650

	// *** Basic block 89

.CollectOperator_label_632:
	bne         t2, t4, .CollectOperator_label_645

	// *** Basic block 90

	li          t1, 27		// 0x1b ASCII \x1b
	sw          t1, 72(t0)
	ld          t1, 48(t0)
	addi        t1, t1, 1
	sd          t1, 48(t0)
	j           .CollectOperator_label_649

	// *** Basic block 91

.CollectOperator_label_645:
	li          t1, 9		// 0x9 ASCII \x9
	sw          t1, 72(t0)

	// *** Basic block 92

.CollectOperator_label_649:

	// *** Basic block 93

.CollectOperator_label_650:
	j           .CollectOperator_label_845

	// *** Basic block 94

.CollectOperator_label_652:
	addi        t1, t0, 8
	ld          t1, 16(t1)
	ld          t3, 48(t0)
	addi        t3, t3, 1
	sd          t3, 48(t0)
	add         t1, t1, t3
	lb          t2, 0(t1)
	li          t1, 61		// 0x3d ASCII '='
	bne         t2, t1, .CollectOperator_label_674

	// *** Basic block 95

	li          t1, 36		// 0x24 ASCII '$'
	sw          t1, 72(t0)
	ld          t1, 48(t0)
	addi        t1, t1, 1
	sd          t1, 48(t0)
	j           .CollectOperator_label_678

	// *** Basic block 96

.CollectOperator_label_674:
	li          t1, 13		// 0xd ASCII \xd
	sw          t1, 72(t0)

	// *** Basic block 97

.CollectOperator_label_678:
	j           .CollectOperator_label_845

	// *** Basic block 98

.CollectOperator_label_680:
	addi        t5, t0, 8
	ld          t5, 16(t5)
	ld          t6, 48(t0)
	addi        t6, t6, 1
	sd          t6, 48(t0)
	add         t5, t5, t6
	lb          t2, 0(t5)
	bne         t2, t3, .CollectOperator_label_702

	// *** Basic block 99

	li          t5, 16		// 0x10 ASCII \x10
	sw          t5, 72(t0)
	ld          t5, 48(t0)
	addi        t5, t5, 1
	sd          t5, 48(t0)
	j           .CollectOperator_label_706

	// *** Basic block 100

.CollectOperator_label_702:
	li          t5, 15		// 0xf ASCII \xf
	sw          t5, 72(t0)

	// *** Basic block 101

.CollectOperator_label_706:
	j           .CollectOperator_label_845

	// *** Basic block 102

.CollectOperator_label_708:
	addi        t6, t0, 8
	ld          t6, 16(t6)
	ld          a0, 48(t0)
	addi        a0, a0, 1
	sd          a0, 48(t0)
	add         t6, t6, a0
	lb          t2, 0(t6)
	bne         t2, t3, .CollectOperator_label_730

	// *** Basic block 103

	li          t6, 37		// 0x25 ASCII '%'
	sw          t6, 72(t0)
	ld          t6, 48(t0)
	addi        t6, t6, 1
	sd          t6, 48(t0)
	j           .CollectOperator_label_748

	// *** Basic block 104

.CollectOperator_label_730:
	bne         t2, t5, .CollectOperator_label_743

	// *** Basic block 105

	li          t5, 28		// 0x1c ASCII \x1c
	sw          t5, 72(t0)
	ld          t5, 48(t0)
	addi        t5, t5, 1
	sd          t5, 48(t0)
	j           .CollectOperator_label_747

	// *** Basic block 106

.CollectOperator_label_743:
	li          t5, 14		// 0xe ASCII \xe
	sw          t5, 72(t0)

	// *** Basic block 107

.CollectOperator_label_747:

	// *** Basic block 108

.CollectOperator_label_748:
	j           .CollectOperator_label_845

	// *** Basic block 109

.CollectOperator_label_750:
	li          t1, 18		// 0x12 ASCII \x12
	sw          t1, 72(t0)
	ld          t1, 48(t0)
	addi        t1, t1, 1
	sd          t1, 48(t0)
	j           .CollectOperator_label_845

	// *** Basic block 110

.CollectOperator_label_759:
	addi        a4, t0, 8
	ld          a4, 16(a4)
	ld          a5, 48(t0)
	addi        a5, a5, 1
	sd          a5, 48(t0)
	add         a6, a4, a6
	lb          t2, 0(a6)
	addi        a7, t2, -46
	seqz        a6, a7
	bne         t2, a3, .CollectOperator_label_778

	// *** Basic block 111

	add         a3, a4, a5
	lb          a3, 0(a3)
	addi        a3, a3, -46
	seqz        a6, a3

	// *** Basic block 112

.CollectOperator_label_778:
	beqz        a6, .CollectOperator_label_788

	// *** Basic block 113

	li          a3, 20		// 0x14 ASCII \x14
	sw          a3, 72(t0)
	ld          a3, 48(t0)
	addi        a3, a3, 2
	sd          a3, 48(t0)
	j           .CollectOperator_label_792

	// *** Basic block 114

.CollectOperator_label_788:
	li          a3, 19		// 0x13 ASCII \x13
	sw          a3, 72(t0)

	// *** Basic block 115

.CollectOperator_label_792:
	j           .CollectOperator_label_845

	// *** Basic block 116

.CollectOperator_label_794:
	li          t5, 43		// 0x2b ASCII '+'
	sw          t5, 72(t0)
	ld          t5, 48(t0)
	addi        t5, t5, 1
	sd          t5, 48(t0)
	j           .CollectOperator_label_845

	// *** Basic block 117

.CollectOperator_label_803:
	li          t1, 17		// 0x11 ASCII \x11
	sw          t1, 72(t0)
	ld          t1, 48(t0)
	addi        t1, t1, 1
	sd          t1, 48(t0)
	j           .CollectOperator_label_845

	// *** Basic block 118

.CollectOperator_label_812:
	li          t1, 49		// 0x31 ASCII '1'
	sw          t1, 72(t0)
	ld          t1, 48(t0)
	addi        t1, t1, 1
	sd          t1, 48(t0)
	j           .CollectOperator_label_845

	// *** Basic block 119

.CollectOperator_label_821:
	li          t6, 54		// 0x36 ASCII '6'
	sw          t6, 72(t0)
	ld          t6, 48(t0)
	addi        t6, t6, 1
	sd          t6, 48(t0)
	j           .CollectOperator_label_845

	// *** Basic block 120

.CollectOperator_label_830:
	lb          t1, 178(t0)
	beqz        t1, .CollectOperator_label_841

	// *** Basic block 121

	li          t1, 97		// 0x61 ASCII 'a'
	sw          t1, 72(t0)
	ld          t1, 48(t0)
	addi        t1, t1, 1
	sd          t1, 48(t0)

	// *** Basic block 122

.CollectOperator_label_841:
	j           .CollectOperator_label_845

	// *** Basic block 123

.CollectOperator_label_843:
	j           .CollectOperator_label_845

	// *** Basic block 124

.CollectOperator_label_845:
	ret         
.func_end_CollectOperator:
	.size CollectOperator, .func_end_CollectOperator-CollectOperator

	.local  LexInitCommon
	.type LexInitCommon, @function

LexInitCommon:

	// *** Basic block 0

	.global StringInit
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
	sw          x0, 72(s1)
	addi        a0, s1, 8
	mv          a1, x0
	call        StringInit

	// *** Basic block 1

	addi        a0, s1, 80
	mv          a1, x0
	call        StringInit

	// *** Basic block 2

	addi        a0, s1, 136
	mv          a1, x0
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           StringInit
.func_end_LexInitCommon:
	.size LexInitCommon, .func_end_LexInitCommon-LexInitCommon

	.global LexInitFromFile
	.type LexInitFromFile, @function

LexInitFromFile:

	// *** Basic block 0

	.global fopen
	.global NewSourceFromFile
	.local LexInitCommon
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
	lla         a1, .str.49
	mv          a0, s1
	call        fopen

	// *** Basic block 1

	mv          s4, a0
	bne         s4, x0, .LexInitFromFile_label_34

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.LexInitFromFile_label_31:
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

.LexInitFromFile_label_34:
	mv          a1, s4
	mv          a0, s1
	call        NewSourceFromFile

	// *** Basic block 5

	sd          a0, 0(s2)
	mv          a1, s3
	mv          a0, s2
	call        LexInitCommon

	// *** Basic block 6

	li          a0, 1		// 0x1 ASCII \x1
	j           .LexInitFromFile_label_31
.func_end_LexInitFromFile:
	.size LexInitFromFile, .func_end_LexInitFromFile-LexInitFromFile

	.global LexInitFromString
	.type LexInitFromString, @function

LexInitFromString:

	// *** Basic block 0

	.global NewSourceFromString
	.local LexInitCommon
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
	mv          t0, a1
	mv          s2, a3
	mv          a1, a2
	mv          a0, t0
	call        NewSourceFromString

	// *** Basic block 1

	sd          a0, 0(s1)
	mv          a1, s2
	mv          a0, s1
	call        LexInitCommon

	// *** Basic block 2

	ld          t0, 0(s1)
	ld          t1, 160(s2)
	ld          t1, 0(t1)
	ld          t1, 120(t1)
	sd          t1, 120(t0)
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 3

.LexInitFromString_label_42:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LexInitFromString:
	.size LexInitFromString, .func_end_LexInitFromString-LexInitFromString

	.global LexDestruct
	.type LexDestruct, @function

LexDestruct:

	// *** Basic block 0

	.global SourceDestruct
	.global free
	.global StringDestruct
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
	ld          s2, 0(s1)
	beq         s2, x0, .LexDestruct_label_22

	// *** Basic block 1

	mv          a0, s2
	call        SourceDestruct

	// *** Basic block 2

	mv          a0, s2
	call        free

	// *** Basic block 3

.LexDestruct_label_22:
	addi        a0, s1, 8
	call        StringDestruct

	// *** Basic block 4

	addi        a0, s1, 80
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           StringDestruct
.func_end_LexDestruct:
	.size LexDestruct, .func_end_LexDestruct-LexDestruct

	.local  CollectHexOrOctal
	.type CollectHexOrOctal, @function

CollectHexOrOctal:

	// *** Basic block 0

	.global toupper
	.global isxdigit
	.global isalpha
	.global tolower
	.local CollectIntegerSuffix
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
	mv          s1, a0
	mv          s2, x0
	ld          s3, 48(s1)
	addi        t0, s3, 1
	sd          t0, 48(s1)
	addi        s5, s1, 8
	ld          s6, 24(s5)
	slt         s4, s3, s6
	bge         s3, s6, .CollectHexOrOctal_label_49

	// *** Basic block 1

	ld          t0, 16(s5)
	add         t0, t0, s3
	lb          a0, 0(t0)
	call        toupper

	// *** Basic block 2

	addi        t0, a0, -88
	seqz        s4, t0

	// *** Basic block 3

.CollectHexOrOctal_label_49:
	beqz        s4, .CollectHexOrOctal_label_114

	// *** Basic block 4

	ld          s7, 48(s1)
	addi        t0, s7, 1
	sd          t0, 48(s1)
	addi        s9, s1, 8
	ld          t0, 24(s9)
	slt         s8, s7, t0
	bge         s7, t0, .CollectHexOrOctal_label_68

	// *** Basic block 5

	ld          t0, 16(s9)
	add         t0, t0, s7
	lb          a0, 0(t0)
	call        isxdigit

	// *** Basic block 6

	mv          s8, a0

	// *** Basic block 7

.CollectHexOrOctal_label_68:
	beqz        s8, .CollectHexOrOctal_label_112

	// *** Basic block 8

.CollectHexOrOctal_label_70:
	addi        s7, s1, 8
	ld          s9, 16(s7)
	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)
	add         t0, s9, t0
	lb          s10, 0(t0)
	slli        s2, s2, 4
	mv          a0, s10
	call        isalpha

	// *** Basic block 9

	beqz        a0, .CollectHexOrOctal_label_95

	// *** Basic block 10

	mv          a0, s10
	call        tolower

	// *** Basic block 11

	addi        t1, a0, -97
	addi        t1, t1, 10
	or          s2, s2, t1
	j           .CollectHexOrOctal_label_99

	// *** Basic block 12

.CollectHexOrOctal_label_95:
	addi        t1, s10, -48
	or          s2, s2, t1

	// *** Basic block 13

.CollectHexOrOctal_label_99:
	ld          t1, 24(s7)
	slt         s10, t0, t1
	bge         t0, t1, .CollectHexOrOctal_label_110

	// *** Basic block 14

	add         t0, s9, t0
	lb          a0, 0(t0)
	call        isxdigit

	// *** Basic block 15

	mv          s10, a0

	// *** Basic block 16

.CollectHexOrOctal_label_110:
	bnez        s10, .CollectHexOrOctal_label_70

	// *** Basic block 17

.CollectHexOrOctal_label_112:
	j           .CollectHexOrOctal_label_166

	// *** Basic block 18

.CollectHexOrOctal_label_114:
	mv          t0, s4
	bge         s3, s6, .CollectHexOrOctal_label_131

	// *** Basic block 19

	ld          t1, 16(s5)
	add         t1, t1, s3
	lb          t1, 0(t1)
	slti        t2, t1, 48
	not         t0, t2
	li          t2, 48		// 0x30 ASCII '0'
	blt         t1, t2, .CollectHexOrOctal_label_130

	// *** Basic block 20

	li          t2, 55		// 0x37 ASCII '7'
	slt         t1, t2, t1
	not         t0, t1

	// *** Basic block 21

.CollectHexOrOctal_label_130:

	// *** Basic block 22

.CollectHexOrOctal_label_131:
	beqz        t0, .CollectHexOrOctal_label_165

	// *** Basic block 23

.CollectHexOrOctal_label_133:
	slli        t0, s2, 3
	addi        t1, s1, 8
	ld          t2, 16(t1)
	ld          t3, 48(s1)
	addi        t3, t3, 1
	sd          t3, 48(s1)
	add         t3, t2, t3
	lb          t3, 0(t3)
	addi        t3, t3, -48
	or          s2, t0, t3
	ld          t1, 24(t1)
	slt         t0, t3, t1
	bge         t3, t1, .CollectHexOrOctal_label_163

	// *** Basic block 24

	add         t1, t2, t3
	lb          t1, 0(t1)
	slti        t2, t1, 48
	not         t0, t2
	li          t2, 48		// 0x30 ASCII '0'
	blt         t1, t2, .CollectHexOrOctal_label_162

	// *** Basic block 25

	li          t2, 55		// 0x37 ASCII '7'
	slt         t1, t2, t1
	not         t0, t1

	// *** Basic block 26

.CollectHexOrOctal_label_162:

	// *** Basic block 27

.CollectHexOrOctal_label_163:
	bnez        t0, .CollectHexOrOctal_label_133

	// *** Basic block 28

.CollectHexOrOctal_label_165:

	// *** Basic block 29

.CollectHexOrOctal_label_166:
	sd          s2, 120(s1)
	mv          a0, s1
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
	j           CollectIntegerSuffix
.func_end_CollectHexOrOctal:
	.size CollectHexOrOctal, .func_end_CollectHexOrOctal-CollectHexOrOctal

	.local  CollectNumber
	.type CollectNumber, @function

CollectNumber:

	// *** Basic block 0

	.local LookaheadChar
	.local CollectHexOrOctal
	.global StringClear
	.global StringAppendChar
	.global SourceEof
	.global isdigit
	.global toupper
	.local CurrentChar
	.local CollectFloatingSuffix
	.local CollectIntegerSuffix
	.global strtod
	.global strtoll
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
	addi        t0, s1, -48
	seqz        s3, t0
	li          t0, 48		// 0x30 ASCII '0'
	bne         s1, t0, .CollectNumber_label_53

	// *** Basic block 1

	mv          a0, s2
	call        LookaheadChar

	// *** Basic block 2

	addi        t0, a0, -46
	snez        s3, t0

	// *** Basic block 3

.CollectNumber_label_53:
	beqz        s3, .CollectNumber_label_59

	// *** Basic block 4

	mv          a0, s2
	call        CollectHexOrOctal

	// *** Basic block 5

	j           .CollectNumber_label_239

	// *** Basic block 6

.CollectNumber_label_59:
	mv          s3, x0
	addi        t0, s1, -46
	seqz        s4, t0
	mv          s5, x0
	addi        a0, s2, 80
	call        StringClear

	// *** Basic block 7

	addi        a0, s2, 80
	mv          a1, s1
	call        StringAppendChar

	// *** Basic block 8

	ld          s6, 48(s2)
	addi        t0, s6, 1
	sd          t0, 48(s2)
	ld          a0, 0(s2)
	call        SourceEof

	// *** Basic block 9

	not         s7, a0
	beqz        s7, .CollectNumber_label_90

	// *** Basic block 10

	addi        t0, s2, 8
	ld          t0, 24(t0)
	slt         s7, s6, t0

	// *** Basic block 11

.CollectNumber_label_90:
	beqz        s7, .CollectNumber_label_178

	// *** Basic block 12

.CollectNumber_label_92:
	addi        t0, s2, 8
	ld          t0, 16(t0)
	ld          t1, 48(s2)
	add         t0, t0, t1
	lb          s1, 0(t0)
	li          t0, 46		// 0x2e ASCII '.'
	bne         s1, t0, .CollectNumber_label_108

	// *** Basic block 13

	bnez        s4, .CollectNumber_label_178

	// *** Basic block 14

.CollectNumber_label_105:
	li          s4, 1		// 0x1 ASCII \x1
	j           .CollectNumber_label_156

	// *** Basic block 15

.CollectNumber_label_108:
	addi        t1, s1, -101
	seqz        t0, t1
	li          t1, 101		// 0x65 ASCII 'e'
	beq         s1, t1, .CollectNumber_label_118

	// *** Basic block 16

	addi        t1, s1, -69
	seqz        t0, t1

	// *** Basic block 17

.CollectNumber_label_118:
	beqz        t0, .CollectNumber_label_124

	// *** Basic block 18

	bnez        s3, .CollectNumber_label_178

	// *** Basic block 19

.CollectNumber_label_121:
	li          s3, 1		// 0x1 ASCII \x1
	j           .CollectNumber_label_155

	// *** Basic block 20

.CollectNumber_label_124:
	addi        t1, s1, -43
	seqz        t0, t1
	li          t1, 43		// 0x2b ASCII '+'
	beq         s1, t1, .CollectNumber_label_134

	// *** Basic block 21

	addi        t1, s1, -45
	seqz        t0, t1

	// *** Basic block 22

.CollectNumber_label_134:
	beqz        t0, .CollectNumber_label_145

	// *** Basic block 23

	not         t0, s3
	bnez        t0, .CollectNumber_label_140

	// *** Basic block 24

	mv          t0, s5

	// *** Basic block 25

.CollectNumber_label_140:
	bnez        t0, .CollectNumber_label_178

	// *** Basic block 26

.CollectNumber_label_142:
	li          s5, 1		// 0x1 ASCII \x1
	j           .CollectNumber_label_154

	// *** Basic block 27

.CollectNumber_label_145:
	mv          a0, s1
	call        isdigit

	// *** Basic block 28

	slli        t0, a0, 56
	srai        t0, t0, 56
	not         t0, t0
	bnez        t0, .CollectNumber_label_178

	// *** Basic block 29

.CollectNumber_label_153:

	// *** Basic block 30

.CollectNumber_label_154:

	// *** Basic block 31

.CollectNumber_label_155:

	// *** Basic block 32

.CollectNumber_label_156:
	addi        a0, s2, 80
	mv          a1, s1
	call        StringAppendChar

	// *** Basic block 33

	ld          s6, 48(s2)
	addi        t0, s6, 1
	sd          t0, 48(s2)
	ld          a0, 0(s2)
	call        SourceEof

	// *** Basic block 34

	not         s7, a0
	beqz        s7, .CollectNumber_label_176

	// *** Basic block 35

	addi        t0, s2, 8
	ld          t0, 24(t0)
	slt         s7, s6, t0

	// *** Basic block 36

.CollectNumber_label_176:
	bnez        s7, .CollectNumber_label_92

	// *** Basic block 37

.CollectNumber_label_178:
	mv          s6, s4
	bnez        s4, .CollectNumber_label_184

	// *** Basic block 38

	mv          s6, s3

	// *** Basic block 39

.CollectNumber_label_184:
	bnez        s6, .CollectNumber_label_195

	// *** Basic block 40

	mv          a0, s2
	call        CurrentChar

	// *** Basic block 41

	call        toupper

	// *** Basic block 42

	addi        t0, a0, -70
	seqz        s6, t0

	// *** Basic block 43

.CollectNumber_label_195:
	beqz        s6, .CollectNumber_label_202

	// *** Basic block 44

	mv          a0, s2
	call        CollectFloatingSuffix

	// *** Basic block 45

	j           .CollectNumber_label_206

	// *** Basic block 46

.CollectNumber_label_202:
	mv          a0, s2
	call        CollectIntegerSuffix

	// *** Basic block 47

.CollectNumber_label_206:
	beqz        s6, .CollectNumber_label_221

	// *** Basic block 48

	addi        t0, s2, 80
	ld          a0, 16(t0)
	mv          a1, x0
	call        strtod

	// *** Basic block 49

	fsd         fa0, 128(s2)
	li          t0, 8		// 0x8 ASCII \x8
	sw          t0, 72(s2)
	j           .CollectNumber_label_238

	// *** Basic block 50

.CollectNumber_label_221:
	addi        t0, s2, 80
	ld          a0, 16(t0)
	li          t0, 10		// 0xa ASCII \xa
	mv          a2, t0
	mv          a1, x0
	call        strtoll

	// *** Basic block 51

	sd          a0, 120(s2)
	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, 72(s2)

	// *** Basic block 52

.CollectNumber_label_238:

	// *** Basic block 53

.CollectNumber_label_239:
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
.func_end_CollectNumber:
	.size CollectNumber, .func_end_CollectNumber-CollectNumber

	.global LexNextToken
	.type LexNextToken, @function

LexNextToken:

	// *** Basic block 0

	.global LexSkipSpacesAndComments
	.global LexEof
	.local CurrentChar
	.global IsIdentifierChar
	.local CollectIdentifierOrWide
	.global isdigit
	.local LookaheadChar
	.local CollectNumber
	.local CollectStringLiteral
	.local CollectCharConst
	.local CollectOperator
	.global NewSourceLocation
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
	call        LexSkipSpacesAndComments

	// *** Basic block 1

	ld          s2, 48(s1)
	ld          t0, 0(s1)
	lw          s3, 80(t0)
	sw          x0, 72(s1)
	mv          a0, s1
	call        LexEof

	// *** Basic block 2

	beqz        a0, .LexNextToken_label_51

	// *** Basic block 3

	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 72(s1)
	j           .LexNextToken_label_133

	// *** Basic block 4

.LexNextToken_label_51:
	mv          a0, s1
	call        CurrentChar

	// *** Basic block 5

	mv          s4, a0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a1, s4
	mv          a0, s1
	call        IsIdentifierChar

	// *** Basic block 6

	beqz        a0, .LexNextToken_label_72

	// *** Basic block 7

	mv          a0, s1
	call        CollectIdentifierOrWide

	// *** Basic block 8

	j           .LexNextToken_label_133

	// *** Basic block 9

.LexNextToken_label_72:
	mv          a0, s4
	call        isdigit

	// *** Basic block 10

	mv          s5, a0
	bnez        a0, .LexNextToken_label_93

	// *** Basic block 11

	addi        t0, s4, -46
	seqz        s5, t0
	li          t0, 46		// 0x2e ASCII '.'
	bne         s4, t0, .LexNextToken_label_92

	// *** Basic block 12

	mv          a0, s1
	call        LookaheadChar

	// *** Basic block 13

	call        isdigit

	// *** Basic block 15

.LexNextToken_label_92:

	// *** Basic block 16

.LexNextToken_label_93:
	beqz        s5, .LexNextToken_label_101

	// *** Basic block 17

	mv          a1, s4
	mv          a0, s1
	call        CollectNumber

	// *** Basic block 18

	j           .LexNextToken_label_133

	// *** Basic block 19

.LexNextToken_label_101:
	li          t0, 34		// 0x22 ASCII '"'
	bne         s4, t0, .LexNextToken_label_114

	// *** Basic block 20

	mv          a0, s1
	call        CollectStringLiteral

	// *** Basic block 21

	li          t0, 4		// 0x4 ASCII \x4
	sw          t0, 72(s1)
	j           .LexNextToken_label_133

	// *** Basic block 22

.LexNextToken_label_114:
	li          t0, 39		// 0x27 ASCII '''
	bne         s4, t0, .LexNextToken_label_129

	// *** Basic block 23

	mv          a0, s1
	call        CollectCharConst

	// *** Basic block 24

	sd          a0, 120(s1)
	li          t0, 6		// 0x6 ASCII \x6
	sw          t0, 72(s1)
	j           .LexNextToken_label_133

	// *** Basic block 25

.LexNextToken_label_129:
	mv          a0, s1
	call        CollectOperator

	// *** Basic block 26

.LexNextToken_label_133:
	lw          t0, 72(s1)
	bnez        t0, .LexNextToken_label_142

	// *** Basic block 27

	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)

	// *** Basic block 28

.LexNextToken_label_142:
	ld          a0, 0(s1)
	ld          a3, 48(s1)
	mv          a2, s2
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
	j           NewSourceLocation
.func_end_LexNextToken:
	.size LexNextToken, .func_end_LexNextToken-LexNextToken

	.global LexMatch
	.type LexMatch, @function

LexMatch:

	// *** Basic block 0

	.global LexNextToken
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
	lw          t0, 72(s1)
	bne         t0, a1, .LexMatch_label_26

	// *** Basic block 1

	mv          a0, s1
	call        LexNextToken

	// *** Basic block 2

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 3

.LexMatch_label_23:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.LexMatch_label_26:
	mv          a0, x0
	j           .LexMatch_label_23
.func_end_LexMatch:
	.size LexMatch, .func_end_LexMatch-LexMatch

	.global LexMatchIdentifier
	.type LexMatchIdentifier, @function

LexMatchIdentifier:

	// *** Basic block 0

	.global StringSetString
	.global LexNextToken
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
	lw          t0, 72(s1)
	li          t1, 3		// 0x3 ASCII \x3
	bne         t0, t1, .LexMatchIdentifier_label_36

	// *** Basic block 1

	addi        a1, s1, 80
	mv          a0, s2
	call        StringSetString

	// *** Basic block 2

	mv          a0, s1
	call        LexNextToken

	// *** Basic block 3

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 4

.LexMatchIdentifier_label_33:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.LexMatchIdentifier_label_36:
	mv          a0, x0
	j           .LexMatchIdentifier_label_33
.func_end_LexMatchIdentifier:
	.size LexMatchIdentifier, .func_end_LexMatchIdentifier-LexMatchIdentifier

	.global LexLookingAt
	.type LexLookingAt, @function

LexLookingAt:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 72(a0)
	sub         t0, t0, a1
	seqz        a0, t0

	// *** Basic block 1

.LexLookingAt_label_15:
	ret         
.func_end_LexLookingAt:
	.size LexLookingAt, .func_end_LexLookingAt-LexLookingAt

	.global LexReadLine
	.type LexReadLine, @function

LexReadLine:

	// *** Basic block 0

	.global StringClear
	.global SourceEof
	.global SourceReadLine
	.global PreprocessorParseDirective
	.global PreprocessorLineIsCompiledIn
	.global PreprocessorReplaceMacros
	.global SourceDelete
	.global compiler
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
	addi        a0, s1, 8
	call        StringClear

	// *** Basic block 1

	sd          x0, 48(s1)
	ld          a0, 0(s1)
	call        SourceEof

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .LexReadLine_label_105

	// *** Basic block 3

.LexReadLine_label_32:
	ld          a0, 0(s1)
	call        SourceEof

	// *** Basic block 4

	not         t0, a0
	beqz        t0, .LexReadLine_label_79

	// *** Basic block 5

.LexReadLine_label_38:
	ld          a0, 0(s1)
	addi        a1, s1, 8
	call        SourceReadLine

	// *** Basic block 6

	ld          a0, 64(s1)
	addi        a1, s1, 8
	call        PreprocessorParseDirective

	// *** Basic block 7

	mv          s2, a0
	not         t0, s2
	beqz        t0, .LexReadLine_label_68

	// *** Basic block 8

	ld          s2, 64(s1)
	mv          a0, s2
	call        PreprocessorLineIsCompiledIn

	// *** Basic block 9

	beqz        a0, .LexReadLine_label_67

	// *** Basic block 10

	addi        a1, s1, 8
	mv          a0, s2
	call        PreprocessorReplaceMacros

	// *** Basic block 11

	j           .LexReadLine_label_79

	// *** Basic block 12

.LexReadLine_label_67:

	// *** Basic block 13

.LexReadLine_label_68:
	addi        a0, s1, 8
	call        StringClear

	// *** Basic block 14

	sd          x0, 48(s1)
	ld          a0, 0(s1)
	call        SourceEof

	// *** Basic block 15

	not         t0, a0
	bnez        t0, .LexReadLine_label_38

	// *** Basic block 16

.LexReadLine_label_79:
	ld          s3, 0(s1)
	mv          a0, s3
	call        SourceEof

	// *** Basic block 17

	beqz        a0, .LexReadLine_label_103

	// *** Basic block 18

	ld          s4, 88(s3)
	beq         s4, x0, .LexReadLine_label_102

	// *** Basic block 19

	mv          a0, s3
	call        SourceDelete

	// *** Basic block 20

	sd          s4, 0(s1)
	la          t0, compiler
	ld          t0, 0(t0)
	ld          t1, 120(s4)
	sd          t1, 1088(t0)
	j           .LexReadLine_label_32

	// *** Basic block 21

.LexReadLine_label_102:

	// *** Basic block 22

.LexReadLine_label_103:
	j           .LexReadLine_label_105

	// *** Basic block 23

.LexReadLine_label_105:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LexReadLine:
	.size LexReadLine, .func_end_LexReadLine-LexReadLine

	.global LexSkipSpacesAndComments
	.type LexSkipSpacesAndComments, @function

LexSkipSpacesAndComments:

	// *** Basic block 0

	.global SourceEof
	.global LexReadLine
	.local GetCharInComment
	.global isspace
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
	ld          a0, 0(s1)
	call        SourceEof

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .LexSkipSpacesAndComments_label_198

	// *** Basic block 2

.LexSkipSpacesAndComments_label_29:
	ld          a0, 0(s1)
	call        SourceEof

	// *** Basic block 3

	not         s2, a0
	beqz        s2, .LexSkipSpacesAndComments_label_42

	// *** Basic block 4

	ld          t0, 48(s1)
	addi        t1, s1, 8
	ld          t1, 24(t1)
	slt         s2, t0, t1

	// *** Basic block 5

.LexSkipSpacesAndComments_label_42:
	beqz        s2, .LexSkipSpacesAndComments_label_167

	// *** Basic block 6

.LexSkipSpacesAndComments_label_44:
	addi        s2, s1, 8
	ld          s3, 16(s2)
	ld          s4, 48(s1)
	add         t0, s3, s4
	lb          s5, 0(t0)
	li          t0, 47		// 0x2f ASCII '/'
	bne         s5, t0, .LexSkipSpacesAndComments_label_142

	// *** Basic block 7

	ld          s2, 24(s2)
	slt         t0, s4, s2
	bge         s4, s2, .LexSkipSpacesAndComments_label_68

	// *** Basic block 8

	addi        t1, s4, 1
	add         t1, s3, t1
	lb          t1, 0(t1)
	addi        t1, t1, -47
	seqz        t0, t1

	// *** Basic block 9

.LexSkipSpacesAndComments_label_68:
	beqz        t0, .LexSkipSpacesAndComments_label_81

	// *** Basic block 10

	lb          t0, 178(s1)
	beqz        t0, .LexSkipSpacesAndComments_label_76

	// *** Basic block 11

	sd          s2, 48(s1)
	j           .LexSkipSpacesAndComments_label_167

	// *** Basic block 12

.LexSkipSpacesAndComments_label_76:
	mv          a0, s1
	call        LexReadLine

	// *** Basic block 13

	j           .LexSkipSpacesAndComments_label_44

	// *** Basic block 14

.LexSkipSpacesAndComments_label_81:
	addi        t1, s2, -1
	slt         t0, s4, t1
	bge         s4, t1, .LexSkipSpacesAndComments_label_93

	// *** Basic block 15

	addi        t1, s4, 1
	add         t1, s3, t1
	lb          t1, 0(t1)
	addi        t1, t1, -42
	seqz        t0, t1

	// *** Basic block 16

.LexSkipSpacesAndComments_label_93:
	beqz        t0, .LexSkipSpacesAndComments_label_140

	// *** Basic block 17

	ld          t0, 48(s1)
	addi        t0, t0, 2
	sd          t0, 48(s1)
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 177(s1)
	ld          s2, 0(s1)

	// *** Basic block 18

.LexSkipSpacesAndComments_label_103:

	// *** Basic block 19

.LexSkipSpacesAndComments_label_104:
	mv          a0, s1
	call        GetCharInComment

	// *** Basic block 20

	mv          s5, a0

	// *** Basic block 21

.LexSkipSpacesAndComments_label_109:
	mv          a0, s2
	call        SourceEof

	// *** Basic block 22

	not         s3, a0
	beqz        s3, .LexSkipSpacesAndComments_label_118

	// *** Basic block 23

	addi        t0, s5, -42
	snez        s3, t0

	// *** Basic block 24

.LexSkipSpacesAndComments_label_118:
	bnez        s3, .LexSkipSpacesAndComments_label_104

	// *** Basic block 25

.LexSkipSpacesAndComments_label_120:
	mv          a0, s1
	call        GetCharInComment

	// *** Basic block 26

	mv          s5, a0

	// *** Basic block 27

.LexSkipSpacesAndComments_label_125:
	mv          a0, s2
	call        SourceEof

	// *** Basic block 28

	not         s3, a0
	beqz        s3, .LexSkipSpacesAndComments_label_134

	// *** Basic block 29

	addi        t0, s5, -47
	snez        s3, t0

	// *** Basic block 30

.LexSkipSpacesAndComments_label_134:
	bnez        s3, .LexSkipSpacesAndComments_label_103

	// *** Basic block 31

.LexSkipSpacesAndComments_label_136:
	sb          x0, 177(s1)
	j           .LexSkipSpacesAndComments_label_44

	// *** Basic block 32

.LexSkipSpacesAndComments_label_140:

	// *** Basic block 33

.LexSkipSpacesAndComments_label_141:

	// *** Basic block 34

.LexSkipSpacesAndComments_label_142:
	mv          a0, s5
	call        isspace

	// *** Basic block 35

	slli        t0, a0, 56
	srai        t0, t0, 56
	not         t0, t0
	bnez        t0, .LexSkipSpacesAndComments_label_167

	// *** Basic block 36

.LexSkipSpacesAndComments_label_150:
	ld          s2, 48(s1)
	addi        t0, s2, 1
	sd          t0, 48(s1)
	ld          a0, 0(s1)
	call        SourceEof

	// *** Basic block 37

	not         s3, a0
	beqz        s3, .LexSkipSpacesAndComments_label_165

	// *** Basic block 38

	addi        t0, s1, 8
	ld          t0, 24(t0)
	slt         s3, s2, t0

	// *** Basic block 39

.LexSkipSpacesAndComments_label_165:
	bnez        s3, .LexSkipSpacesAndComments_label_44

	// *** Basic block 40

.LexSkipSpacesAndComments_label_167:
	ld          s2, 0(s1)
	mv          a0, s2
	call        SourceEof

	// *** Basic block 41

	bnez        a0, .LexSkipSpacesAndComments_label_198

	// *** Basic block 42

.LexSkipSpacesAndComments_label_173:
	ld          t0, 48(s1)
	addi        t1, s1, 8
	ld          t1, 24(t1)
	blt         t0, t1, .LexSkipSpacesAndComments_label_198

	// *** Basic block 43

.LexSkipSpacesAndComments_label_181:
	slt         t1, x0, t0
	bge         x0, t0, .LexSkipSpacesAndComments_label_187

	// *** Basic block 44

	lb          t1, 178(s1)

	// *** Basic block 45

.LexSkipSpacesAndComments_label_187:
	bnez        t1, .LexSkipSpacesAndComments_label_198

	// *** Basic block 46

.LexSkipSpacesAndComments_label_189:
	mv          a0, s1
	call        LexReadLine

	// *** Basic block 47

	mv          a0, s2
	call        SourceEof

	// *** Basic block 48

	not         s2, a0
	bnez        s2, .LexSkipSpacesAndComments_label_29

	// *** Basic block 49

.LexSkipSpacesAndComments_label_198:
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
.func_end_LexSkipSpacesAndComments:
	.size LexSkipSpacesAndComments, .func_end_LexSkipSpacesAndComments-LexSkipSpacesAndComments

	.global LexEof
	.type LexEof, @function

LexEof:

	// *** Basic block 0

	.global SourceEof
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 0(t0)
	j           SourceEof
.func_end_LexEof:
	.size LexEof, .func_end_LexEof-LexEof

	.local  ReportSourceStack
	.type ReportSourceStack, @function

ReportSourceStack:

	// *** Basic block 0

	.global ReportNote
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	ld          t0, 0(a0)
	ld          s1, 88(t0)
	beq         s1, x0, .ReportSourceStack_label_37

	// *** Basic block 1

.ReportSourceStack_label_19:
	ld          a0, 16(s1)
	lw          a1, 80(s1)
	lla         a2, .str.50
	call        ReportNote

	// *** Basic block 2

	ld          s1, 88(s1)
	bne         s1, x0, .ReportSourceStack_label_19

	// *** Basic block 3

.ReportSourceStack_label_37:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ReportSourceStack:
	.size ReportSourceStack, .func_end_ReportSourceStack-ReportSourceStack

	.global LexError
	.type LexError, @function

LexError:

	// *** Basic block 0

	.global VReportError
	.local ReportSourceStack
	addi sp, sp, -96
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// varargs function with 0 declared args
	sd a0, 0(s0)
	sd a1, 8(s0)
	sd a2, 16(s0)
	sd a3, 24(s0)
	sd a4, 32(s0)
	sd a5, 40(s0)
	sd a6, 48(s0)
	sd a7, 56(s0)
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          t0, a1
	mv          s1, a0
	mv          t1, s0
	ld          t2, 0(s1)
	ld          a0, 16(t2)
	lw          a1, 80(t2)
	mv          a3, t1
	mv          a2, t0
	call        VReportError

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 64
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ReportSourceStack
.func_end_LexError:
	.size LexError, .func_end_LexError-LexError

	.global VLexError
	.type VLexError, @function

VLexError:

	// *** Basic block 0

	.global VReportError
	.local ReportSourceStack
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
	mv          t0, a1
	mv          t1, a2
	ld          t2, 0(s1)
	ld          a0, 16(t2)
	lw          a1, 80(t2)
	mv          a3, t1
	mv          a2, t0
	call        VReportError

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ReportSourceStack
.func_end_VLexError:
	.size VLexError, .func_end_VLexError-VLexError

	.global LexWarning
	.type LexWarning, @function

LexWarning:

	// *** Basic block 0

	.global VReportWarning
	.local ReportSourceStack
	addi sp, sp, -96
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// varargs function with 0 declared args
	sd a0, 0(s0)
	sd a1, 8(s0)
	sd a2, 16(s0)
	sd a3, 24(s0)
	sd a4, 32(s0)
	sd a5, 40(s0)
	sd a6, 48(s0)
	sd a7, 56(s0)
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          t0, a2
	mv          s1, a0
	mv          t1, a1
	mv          t2, s0
	ld          t3, 0(s1)
	ld          a0, 16(t3)
	lw          a1, 80(t3)
	mv          a4, t2
	mv          a3, t0
	mv          a2, t1
	call        VReportWarning

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 64
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ReportSourceStack
.func_end_LexWarning:
	.size LexWarning, .func_end_LexWarning-LexWarning

	.global VLexWarning
	.type VLexWarning, @function

VLexWarning:

	// *** Basic block 0

	.global VReportWarning
	.local ReportSourceStack
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
	mv          t0, a1
	mv          t1, a2
	mv          t2, a3
	ld          t3, 0(s1)
	ld          a0, 16(t3)
	lw          a1, 80(t3)
	mv          a4, t2
	mv          a3, t1
	mv          a2, t0
	call        VReportWarning

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ReportSourceStack
.func_end_VLexWarning:
	.size VLexWarning, .func_end_VLexWarning-VLexWarning

	.global LexReadAttributes
	.type LexReadAttributes, @function

LexReadAttributes:

	// *** Basic block 0

	.global LexSkipSpacesAndComments
	.global LexEof
	.global StringAppendChar
	.local GetCharInComment
	.global LexNextToken
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
	call        LexSkipSpacesAndComments

	// *** Basic block 1

	mv          s3, x0
	mv          a0, s1
	call        LexEof

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .LexReadAttributes_label_83

	// *** Basic block 3

.LexReadAttributes_label_32:
	addi        t0, s1, 8
	ld          t0, 16(t0)
	ld          t1, 48(s1)
	add         t0, t0, t1
	lb          s4, 0(t0)
	li          t0, 40		// 0x28 ASCII '('
	bne         s4, t0, .LexReadAttributes_label_51

	// *** Basic block 4

	addi        s3, s3, 1
	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)
	j           .LexReadAttributes_label_77

	// *** Basic block 5

.LexReadAttributes_label_51:
	li          t0, 41		// 0x29 ASCII ')'
	bne         s4, t0, .LexReadAttributes_label_67

	// *** Basic block 6

	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)
	addi        s3, s3, -1
	beqz        s3, .LexReadAttributes_label_83

	// *** Basic block 7

.LexReadAttributes_label_65:
	j           .LexReadAttributes_label_76

	// *** Basic block 8

.LexReadAttributes_label_67:
	mv          a0, s1
	call        GetCharInComment

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s2
	call        StringAppendChar

	// *** Basic block 10

.LexReadAttributes_label_76:

	// *** Basic block 11

.LexReadAttributes_label_77:
	mv          a0, s1
	call        LexEof

	// *** Basic block 12

	not         t0, a0
	bnez        t0, .LexReadAttributes_label_32

	// *** Basic block 13

.LexReadAttributes_label_83:
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LexNextToken
.func_end_LexReadAttributes:
	.size LexReadAttributes, .func_end_LexReadAttributes-LexReadAttributes

	.global LexRewind
	.type LexRewind, @function

LexRewind:

	// *** Basic block 0

	.global SourceRewind
	.global StringSet
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
	ld          a0, 0(s1)
	call        SourceRewind

	// *** Basic block 1

	sd          x0, 48(s1)
	sw          x0, 72(s1)
	addi        a0, s1, 8
	lla         a1, .str.51
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           StringSet
.func_end_LexRewind:
	.size LexRewind, .func_end_LexRewind-LexRewind

.PCend:
	.data
reserved_words:
	.type   reserved_words,@object
	.local  reserved_words
	.size   reserved_words,640
	.p2align  3
	.long    .str.1
	.word   57
	.space  4
	.long    .str.2
	.word   60
	.space  4
	.long    .str.3
	.word   74
	.space  4
	.long    .str.4
	.word   96
	.space  4
	.long    .str.5
	.word   88
	.space  4
	.long    .str.6
	.word   95
	.space  4
	.long    .str.7
	.word   55
	.space  4
	.long    .str.8
	.word   56
	.space  4
	.long    .str.9
	.word   58
	.space  4
	.long    .str.10
	.word   59
	.space  4
	.long    .str.11
	.word   61
	.space  4
	.long    .str.12
	.word   62
	.space  4
	.long    .str.13
	.word   63
	.space  4
	.long    .str.14
	.word   64
	.space  4
	.long    .str.15
	.word   65
	.space  4
	.long    .str.16
	.word   66
	.space  4
	.long    .str.17
	.word   67
	.space  4
	.long    .str.18
	.word   68
	.space  4
	.long    .str.19
	.word   70
	.space  4
	.long    .str.20
	.word   71
	.space  4
	.long    .str.21
	.word   72
	.space  4
	.long    .str.22
	.word   73
	.space  4
	.long    .str.23
	.word   75
	.space  4
	.long    .str.24
	.word   76
	.space  4
	.long    .str.25
	.word   77
	.space  4
	.long    .str.26
	.word   78
	.space  4
	.long    .str.27
	.word   79
	.space  4
	.long    .str.28
	.word   80
	.space  4
	.long    .str.29
	.word   81
	.space  4
	.long    .str.30
	.word   82
	.space  4
	.long    .str.31
	.word   83
	.space  4
	.long    .str.32
	.word   84
	.space  4
	.long    .str.33
	.word   85
	.space  4
	.long    .str.34
	.word   86
	.space  4
	.long    .str.35
	.word   87
	.space  4
	.long    .str.36
	.word   89
	.space  4
	.long    .str.37
	.word   90
	.space  4
	.long    .str.38
	.word   91
	.space  4
	.long    .str.39
	.word   92
	.space  4
	.long    .str.40
	.word   94
	.space  4

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "_Bool"
	.type .str.1, @object
	.size .str.1, 6

.str.2:
	.asciz "_Complex"
	.type .str.2, @object
	.size .str.2, 9

.str.3:
	.asciz "_Imaginary"
	.type .str.3, @object
	.size .str.3, 11

.str.4:
	.asciz "__attribute__"
	.type .str.4, @object
	.size .str.4, 14

.str.5:
	.asciz "__thread"
	.type .str.5, @object
	.size .str.5, 9

.str.6:
	.asciz "asm"
	.type .str.6, @object
	.size .str.6, 4

.str.7:
	.asciz "auto"
	.type .str.7, @object
	.size .str.7, 5

.str.8:
	.asciz "break"
	.type .str.8, @object
	.size .str.8, 6

.str.9:
	.asciz "case"
	.type .str.9, @object
	.size .str.9, 5

.str.10:
	.asciz "char"
	.type .str.10, @object
	.size .str.10, 5

.str.11:
	.asciz "const"
	.type .str.11, @object
	.size .str.11, 6

.str.12:
	.asciz "continue"
	.type .str.12, @object
	.size .str.12, 9

.str.13:
	.asciz "default"
	.type .str.13, @object
	.size .str.13, 8

.str.14:
	.asciz "do"
	.type .str.14, @object
	.size .str.14, 3

.str.15:
	.asciz "double"
	.type .str.15, @object
	.size .str.15, 7

.str.16:
	.asciz "else"
	.type .str.16, @object
	.size .str.16, 5

.str.17:
	.asciz "enum"
	.type .str.17, @object
	.size .str.17, 5

.str.18:
	.asciz "extern"
	.type .str.18, @object
	.size .str.18, 7

.str.19:
	.asciz "float"
	.type .str.19, @object
	.size .str.19, 6

.str.20:
	.asciz "for"
	.type .str.20, @object
	.size .str.20, 4

.str.21:
	.asciz "goto"
	.type .str.21, @object
	.size .str.21, 5

.str.22:
	.asciz "if"
	.type .str.22, @object
	.size .str.22, 3

.str.23:
	.asciz "inline"
	.type .str.23, @object
	.size .str.23, 7

.str.24:
	.asciz "int"
	.type .str.24, @object
	.size .str.24, 4

.str.25:
	.asciz "long"
	.type .str.25, @object
	.size .str.25, 5

.str.26:
	.asciz "register"
	.type .str.26, @object
	.size .str.26, 9

.str.27:
	.asciz "restrict"
	.type .str.27, @object
	.size .str.27, 9

.str.28:
	.asciz "return"
	.type .str.28, @object
	.size .str.28, 7

.str.29:
	.asciz "short"
	.type .str.29, @object
	.size .str.29, 6

.str.30:
	.asciz "signed"
	.type .str.30, @object
	.size .str.30, 7

.str.31:
	.asciz "sizeof"
	.type .str.31, @object
	.size .str.31, 7

.str.32:
	.asciz "static"
	.type .str.32, @object
	.size .str.32, 7

.str.33:
	.asciz "struct"
	.type .str.33, @object
	.size .str.33, 7

.str.34:
	.asciz "switch"
	.type .str.34, @object
	.size .str.34, 7

.str.35:
	.asciz "typedef"
	.type .str.35, @object
	.size .str.35, 8

.str.36:
	.asciz "union"
	.type .str.36, @object
	.size .str.36, 6

.str.37:
	.asciz "unsigned"
	.type .str.37, @object
	.size .str.37, 9

.str.38:
	.asciz "void"
	.type .str.38, @object
	.size .str.38, 5

.str.39:
	.asciz "volatile"
	.type .str.39, @object
	.size .str.39, 9

.str.40:
	.asciz "while"
	.type .str.40, @object
	.size .str.40, 6

.str.41:
	.asciz "A universal-character must have 4 hex digits"
	.type .str.41, @object
	.size .str.41, 45

.str.42:
	.asciz "Illegal escape sequence \\%c"
	.type .str.42, @object
	.size .str.42, 28

.str.43:
	.asciz "Newline in string literal"
	.type .str.43, @object
	.size .str.43, 26

.str.44:
	.asciz "Newline in string literal"
	.type .str.44, @object
	.size .str.44, 26

.str.45:
	.asciz "Max of 4 characters allowed in character constant"
	.type .str.45, @object
	.size .str.45, 50

.str.46:
	.asciz "Newline in character constant"
	.type .str.46, @object
	.size .str.46, 30

.str.47:
	.asciz "Max of 1 character allowed in wide character constant"
	.type .str.47, @object
	.size .str.47, 54

.str.48:
	.asciz "Newline in character constant"
	.type .str.48, @object
	.size .str.48, 30

.str.49:
	.asciz "r"
	.type .str.49, @object
	.size .str.49, 2

.str.50:
	.asciz "Included from here"
	.type .str.50, @object
	.size .str.50, 19

.str.51:
	.asciz "(null)"
	.type .str.51, @object
	.size .str.51, 1

