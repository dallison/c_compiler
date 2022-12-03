	.file   "expr_evaluator.c"
	.text
	.option pic
.PCbegin:
	.global EvaluateIntegerExpression
	.type EvaluateIntegerExpression, @function

EvaluateIntegerExpression:

	// *** Basic block 0

	.global TypeIsIntegral
	.global TypeIsFloatingPoint
	.global StorageIs
	.global EvaluateIntegerExpression
	sd          a0, -8(s0)	// Spilled @1795
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -32(s0)
	// Spilled register region: 16 bytes at -48(s0) to -32(s0)
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
	bne         s1, x0, .EvaluateIntegerExpression_label_102

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.EvaluateIntegerExpression_label_99:
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

	// *** Basic block 3

.EvaluateIntegerExpression_label_102:
	ld          s3, 16(s1)
	bne         s3, x0, .EvaluateIntegerExpression_label_111

	// *** Basic block 4

	mv          a0, x0
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 5

.EvaluateIntegerExpression_label_111:
	mv          a0, s3
	call        TypeIsIntegral

	// *** Basic block 6

	not         s4, a0
	beqz        s4, .EvaluateIntegerExpression_label_122

	// *** Basic block 7

	mv          a0, s3
	call        TypeIsFloatingPoint

	// *** Basic block 8

	not         s4, a0

	// *** Basic block 9

.EvaluateIntegerExpression_label_122:
	beqz        s4, .EvaluateIntegerExpression_label_127

	// *** Basic block 10

	mv          a0, x0
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 11

.EvaluateIntegerExpression_label_127:
	mv          s3, s1
	mv          s4, s1
	mv          s5, s1
	mv          s6, s1
	lw          s7, 0(s1)
	li          s8, 93		// 0x5d ASCII ']'
	blt         s7, s8, .EvaluateIntegerExpression_label_326

	// *** Basic block 12

	li          s9, 134		// 0x86 ASCII \x86
	blt         s7, s9, .EvaluateIntegerExpression_label_234

	// *** Basic block 13

	li          s10, 147		// 0x93 ASCII \x93
	blt         s7, s10, .EvaluateIntegerExpression_label_192

	// *** Basic block 14

	beq         s7, s10, .EvaluateIntegerExpression_label_1402

	// *** Basic block 15

	li          t0, 149		// 0x95 ASCII \x95
	beq         s7, t0, .EvaluateIntegerExpression_label_1507

	// *** Basic block 16

	li          t0, 150		// 0x96 ASCII \x96
	beq         s7, t0, .EvaluateIntegerExpression_label_1612

	// *** Basic block 17

	li          t0, 151		// 0x97 ASCII \x97
	beq         s7, t0, .EvaluateIntegerExpression_label_1717

	// *** Basic block 18

	li          t0, 154		// 0x9a ASCII \x9a
	beq         s7, t0, .EvaluateIntegerExpression_label_1289

	// *** Basic block 19

	li          t0, 155		// 0x9b ASCII \x9b
	beq         s7, t0, .EvaluateIntegerExpression_label_1312

	// *** Basic block 20

	li          t0, 157		// 0x9d ASCII \x9d
	beq         s7, t0, .EvaluateIntegerExpression_label_1417

	// *** Basic block 21

	li          t0, 158		// 0x9e ASCII \x9e
	beq         s7, t0, .EvaluateIntegerExpression_label_1522

	// *** Basic block 22

	li          t0, 159		// 0x9f ASCII \x9f
	beq         s7, t0, .EvaluateIntegerExpression_label_1627

	// *** Basic block 23

	j           .EvaluateIntegerExpression_label_1790

	// *** Basic block 24

.EvaluateIntegerExpression_label_192:
	beq         s7, s9, .EvaluateIntegerExpression_label_1582

	// *** Basic block 25

	li          t0, 135		// 0x87 ASCII \x87
	beq         s7, t0, .EvaluateIntegerExpression_label_1687

	// *** Basic block 26

	li          t0, 138		// 0x8a ASCII \x8a
	beq         s7, t0, .EvaluateIntegerExpression_label_1287

	// *** Basic block 27

	li          t0, 139		// 0x8b ASCII \x8b
	beq         s7, t0, .EvaluateIntegerExpression_label_1387

	// *** Basic block 28

	li          t0, 141		// 0x8d ASCII \x8d
	beq         s7, t0, .EvaluateIntegerExpression_label_1492

	// *** Basic block 29

	li          t0, 142		// 0x8e ASCII \x8e
	beq         s7, t0, .EvaluateIntegerExpression_label_1597

	// *** Basic block 30

	li          t0, 143		// 0x8f ASCII \x8f
	beq         s7, t0, .EvaluateIntegerExpression_label_1702

	// *** Basic block 31

	li          t0, 146		// 0x92 ASCII \x92
	beq         s7, t0, .EvaluateIntegerExpression_label_1288

	// *** Basic block 32

	j           .EvaluateIntegerExpression_label_1790

	// *** Basic block 33

.EvaluateIntegerExpression_label_234:
	li          s9, 117		// 0x75 ASCII 'u'
	blt         s7, s9, .EvaluateIntegerExpression_label_284

	// *** Basic block 34

	beq         s7, s9, .EvaluateIntegerExpression_label_1447

	// *** Basic block 35

	li          t0, 118		// 0x76 ASCII 'v'
	beq         s7, t0, .EvaluateIntegerExpression_label_1672

	// *** Basic block 36

	li          t0, 122		// 0x7a ASCII 'z'
	beq         s7, t0, .EvaluateIntegerExpression_label_1285

	// *** Basic block 37

	li          t0, 123		// 0x7b ASCII '{'
	beq         s7, t0, .EvaluateIntegerExpression_label_1357

	// *** Basic block 38

	li          t0, 125		// 0x7d ASCII '}'
	beq         s7, t0, .EvaluateIntegerExpression_label_1462

	// *** Basic block 39

	li          t0, 126		// 0x7e ASCII '~'
	beq         s7, t0, .EvaluateIntegerExpression_label_1567

	// *** Basic block 40

	li          t0, 130		// 0x82 ASCII \x82
	beq         s7, t0, .EvaluateIntegerExpression_label_1286

	// *** Basic block 41

	li          t0, 131		// 0x83 ASCII \x83
	beq         s7, t0, .EvaluateIntegerExpression_label_1372

	// *** Basic block 42

	li          t0, 133		// 0x85 ASCII \x85
	beq         s7, t0, .EvaluateIntegerExpression_label_1477

	// *** Basic block 43

	j           .EvaluateIntegerExpression_label_1790

	// *** Basic block 44

.EvaluateIntegerExpression_label_284:
	beq         s7, s8, .EvaluateIntegerExpression_label_1537

	// *** Basic block 45

	li          t0, 94		// 0x5e ASCII '^'
	beq         s7, t0, .EvaluateIntegerExpression_label_1642

	// *** Basic block 46

	li          t0, 98		// 0x62 ASCII 'b'
	beq         s7, t0, .EvaluateIntegerExpression_label_1283

	// *** Basic block 47

	li          t0, 107		// 0x6b ASCII 'k'
	beq         s7, t0, .EvaluateIntegerExpression_label_1327

	// *** Basic block 48

	li          t0, 109		// 0x6d ASCII 'm'
	beq         s7, t0, .EvaluateIntegerExpression_label_1552

	// *** Basic block 49

	li          t0, 110		// 0x6e ASCII 'n'
	beq         s7, t0, .EvaluateIntegerExpression_label_1657

	// *** Basic block 50

	li          t0, 114		// 0x72 ASCII 'r'
	beq         s7, t0, .EvaluateIntegerExpression_label_1284

	// *** Basic block 51

	li          t0, 115		// 0x73 ASCII 's'
	beq         s7, t0, .EvaluateIntegerExpression_label_1342

	// *** Basic block 52

	j           .EvaluateIntegerExpression_label_1790

	// *** Basic block 53

.EvaluateIntegerExpression_label_326:
	li          s8, 43		// 0x2b ASCII '+'
	blt         s7, s8, .EvaluateIntegerExpression_label_421

	// *** Basic block 54

	li          s9, 61		// 0x3d ASCII '='
	blt         s7, s9, .EvaluateIntegerExpression_label_379

	// *** Basic block 55

	beq         s7, s9, .EvaluateIntegerExpression_label_1230

	// *** Basic block 56

	li          t0, 65		// 0x41 ASCII 'A'
	beq         s7, t0, .EvaluateIntegerExpression_label_1732

	// *** Basic block 57

	li          t0, 66		// 0x42 ASCII 'B'
	beq         s7, t0, .EvaluateIntegerExpression_label_696

	// *** Basic block 58

	li          t0, 68		// 0x44 ASCII 'D'
	beq         s7, t0, .EvaluateIntegerExpression_label_668

	// *** Basic block 59

	li          t0, 71		// 0x47 ASCII 'G'
	beq         s7, t0, .EvaluateIntegerExpression_label_1151

	// *** Basic block 60

	li          t0, 80		// 0x50 ASCII 'P'
	beq         s7, t0, .EvaluateIntegerExpression_label_1266

	// *** Basic block 61

	li          t0, 84		// 0x54 ASCII 'T'
	beq         s7, t0, .EvaluateIntegerExpression_label_605

	// *** Basic block 62

	li          t0, 87		// 0x57 ASCII 'W'
	beq         s7, t0, .EvaluateIntegerExpression_label_1778

	// *** Basic block 63

	li          t0, 91		// 0x5b ASCII '['
	beq         s7, t0, .EvaluateIntegerExpression_label_1432

	// *** Basic block 64

	j           .EvaluateIntegerExpression_label_1790

	// *** Basic block 65

.EvaluateIntegerExpression_label_379:
	beq         s7, s8, .EvaluateIntegerExpression_label_876

	// *** Basic block 66

	li          t0, 44		// 0x2c ASCII ','
	beq         s7, t0, .EvaluateIntegerExpression_label_1167

	// *** Basic block 67

	li          t0, 45		// 0x2d ASCII '-'
	beq         s7, t0, .EvaluateIntegerExpression_label_1195

	// *** Basic block 68

	li          t0, 48		// 0x30 ASCII '0'
	beq         s7, t0, .EvaluateIntegerExpression_label_764

	// *** Basic block 69

	li          t0, 51		// 0x33 ASCII '3'
	beq         s7, t0, .EvaluateIntegerExpression_label_640

	// *** Basic block 70

	li          t0, 54		// 0x36 ASCII '6'
	beq         s7, t0, .EvaluateIntegerExpression_label_991

	// *** Basic block 71

	li          t0, 56		// 0x38 ASCII '8'
	beq         s7, t0, .EvaluateIntegerExpression_label_730

	// *** Basic block 72

	li          t0, 58		// 0x3a ASCII ':'
	beq         s7, t0, .EvaluateIntegerExpression_label_610

	// *** Basic block 73

	j           .EvaluateIntegerExpression_label_1790

	// *** Basic block 74

.EvaluateIntegerExpression_label_421:
	li          s8, 17		// 0x11 ASCII \x11
	blt         s7, s8, .EvaluateIntegerExpression_label_471

	// *** Basic block 75

	beq         s7, s8, .EvaluateIntegerExpression_label_1020

	// *** Basic block 76

	li          t0, 21		// 0x15 ASCII \x15
	beq         s7, t0, .EvaluateIntegerExpression_label_1135

	// *** Basic block 77

	li          t0, 22		// 0x16 ASCII \x16
	beq         s7, t0, .EvaluateIntegerExpression_label_1048

	// *** Basic block 78

	li          t0, 24		// 0x18 ASCII \x18
	beq         s7, t0, .EvaluateIntegerExpression_label_1076

	// *** Basic block 79

	li          t0, 27		// 0x1b ASCII \x1b
	beq         s7, t0, .EvaluateIntegerExpression_label_515

	// *** Basic block 80

	li          t0, 34		// 0x22 ASCII '"'
	beq         s7, t0, .EvaluateIntegerExpression_label_962

	// *** Basic block 81

	li          t0, 37		// 0x25 ASCII '%'
	beq         s7, t0, .EvaluateIntegerExpression_label_905

	// *** Basic block 82

	li          t0, 38		// 0x26 ASCII '&'
	beq         s7, t0, .EvaluateIntegerExpression_label_933

	// *** Basic block 83

	li          t0, 42		// 0x2a ASCII '*'
	beq         s7, t0, .EvaluateIntegerExpression_label_848

	// *** Basic block 84

	j           .EvaluateIntegerExpression_label_1790

	// *** Basic block 85

.EvaluateIntegerExpression_label_471:
	li          t0, 1		// 0x1 ASCII \x1
	beq         s7, t0, .EvaluateIntegerExpression_label_513

	// *** Basic block 86

	li          t0, 2		// 0x2 ASCII \x2
	beq         s7, t0, .EvaluateIntegerExpression_label_530

	// *** Basic block 87

	li          t0, 5		// 0x5 ASCII \x5
	beq         s7, t0, .EvaluateIntegerExpression_label_514

	// *** Basic block 88

	li          t0, 6		// 0x6 ASCII \x6
	beq         s7, t0, .EvaluateIntegerExpression_label_522

	// *** Basic block 89

	li          t0, 9		// 0x9 ASCII \x9
	beq         s7, t0, .EvaluateIntegerExpression_label_1104

	// *** Basic block 90

	li          t0, 10		// 0xa ASCII \xa
	beq         s7, t0, .EvaluateIntegerExpression_label_1120

	// *** Basic block 91

	li          t0, 13		// 0xd ASCII \xd
	beq         s7, t0, .EvaluateIntegerExpression_label_820

	// *** Basic block 92

	li          t0, 14		// 0xe ASCII \xe
	beq         s7, t0, .EvaluateIntegerExpression_label_792

	// *** Basic block 93

	j           .EvaluateIntegerExpression_label_1790

	// *** Basic block 94

.EvaluateIntegerExpression_label_513:

	// *** Basic block 95

.EvaluateIntegerExpression_label_514:

	// *** Basic block 96

.EvaluateIntegerExpression_label_515:
	ld          t0, 56(s3)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 97

.EvaluateIntegerExpression_label_522:
	fld         ft0, 56(s3)
	fcvt.l.d    t0, ft0
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 98

.EvaluateIntegerExpression_label_530:
	ld          s7, 56(s4)
	lw          a0, 48(s7)
	li          t0, 32		// 0x20 ASCII ' '
	mv          a1, t0
	call        StorageIs

	// *** Basic block 99

	beqz        a0, .EvaluateIntegerExpression_label_565

	// *** Basic block 100

	ld          s4, 112(s7)
	bne         s4, x0, .EvaluateIntegerExpression_label_550

	// *** Basic block 101

	mv          a0, x0
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 102

.EvaluateIntegerExpression_label_550:
	lb          t0, 60(s4)
	not         t0, t0
	beqz        t0, .EvaluateIntegerExpression_label_558

	// *** Basic block 103

	mv          a0, x0
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 104

.EvaluateIntegerExpression_label_558:
	ld          t0, 48(s4)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 105

.EvaluateIntegerExpression_label_565:
	ld          s4, 40(s7)
	lw          t0, 12(s4)
	andi        t0, t0, 4
	bnez        t0, .EvaluateIntegerExpression_label_577

	// *** Basic block 106

	mv          a0, x0
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 107

.EvaluateIntegerExpression_label_577:
	mv          a0, s4
	call        TypeIsIntegral

	// *** Basic block 108

	beqz        a0, .EvaluateIntegerExpression_label_586

	// *** Basic block 109

	ld          t0, 112(s7)
	sd          t0, 0(s2)
	j           .EvaluateIntegerExpression_label_601

	// *** Basic block 110

.EvaluateIntegerExpression_label_586:
	mv          a0, s4
	call        TypeIsFloatingPoint

	// *** Basic block 111

	beqz        a0, .EvaluateIntegerExpression_label_596

	// *** Basic block 112

	fld         ft0, 112(s7)
	fcvt.l.d    t0, ft0
	sd          t0, 0(s2)
	j           .EvaluateIntegerExpression_label_600

	// *** Basic block 113

.EvaluateIntegerExpression_label_596:
	mv          a0, x0
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 114

.EvaluateIntegerExpression_label_600:

	// *** Basic block 115

.EvaluateIntegerExpression_label_601:
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 116

.EvaluateIntegerExpression_label_605:
	sd          x0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 117

.EvaluateIntegerExpression_label_610:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 118

	mv          s8, a0
	beqz        a0, .EvaluateIntegerExpression_label_629

	// *** Basic block 119

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 120

	mv          s8, a0

	// *** Basic block 121

.EvaluateIntegerExpression_label_629:
	beqz        s8, .EvaluateIntegerExpression_label_638

	// *** Basic block 122

	ld          t0, -32(s0)
	ld          t1, -24(s0)
	add         t0, t0, t1
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 123

.EvaluateIntegerExpression_label_638:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 124

.EvaluateIntegerExpression_label_640:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 125

	mv          s8, a0
	beqz        a0, .EvaluateIntegerExpression_label_657

	// *** Basic block 126

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 127

	mv          s8, a0

	// *** Basic block 128

.EvaluateIntegerExpression_label_657:
	beqz        s8, .EvaluateIntegerExpression_label_666

	// *** Basic block 129

	ld          t0, -32(s0)
	ld          t1, -24(s0)
	sub         t0, t0, t1
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 130

.EvaluateIntegerExpression_label_666:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 131

.EvaluateIntegerExpression_label_668:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 132

	mv          s11, a0
	beqz        a0, .EvaluateIntegerExpression_label_685

	// *** Basic block 133

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 134

	mv          s11, a0

	// *** Basic block 135

.EvaluateIntegerExpression_label_685:
	beqz        s11, .EvaluateIntegerExpression_label_694

	// *** Basic block 136

	ld          t0, -32(s0)
	ld          t1, -24(s0)
	mul         t0, t0, t1
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 137

.EvaluateIntegerExpression_label_694:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 138

.EvaluateIntegerExpression_label_696:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 139

	mv          s11, a0
	beqz        a0, .EvaluateIntegerExpression_label_713

	// *** Basic block 140

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 141

	mv          s11, a0

	// *** Basic block 142

.EvaluateIntegerExpression_label_713:
	beqz        s11, .EvaluateIntegerExpression_label_728

	// *** Basic block 143

	ld          t0, -24(s0)
	bnez        t0, .EvaluateIntegerExpression_label_721

	// *** Basic block 144

	mv          a0, x0
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 145

.EvaluateIntegerExpression_label_721:
	ld          t1, -32(s0)
	div         t0, t1, t0
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 146

.EvaluateIntegerExpression_label_728:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 147

.EvaluateIntegerExpression_label_730:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 148

	mv          s8, a0
	beqz        a0, .EvaluateIntegerExpression_label_747

	// *** Basic block 149

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 150

	mv          s8, a0

	// *** Basic block 151

.EvaluateIntegerExpression_label_747:
	beqz        s8, .EvaluateIntegerExpression_label_762

	// *** Basic block 152

	ld          t0, -24(s0)
	bnez        t0, .EvaluateIntegerExpression_label_755

	// *** Basic block 153

	mv          a0, x0
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 154

.EvaluateIntegerExpression_label_755:
	ld          t1, -32(s0)
	rem         t0, t1, t0
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 155

.EvaluateIntegerExpression_label_762:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 156

.EvaluateIntegerExpression_label_764:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 157

	mv          s8, a0
	beqz        a0, .EvaluateIntegerExpression_label_781

	// *** Basic block 158

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 159

	mv          s8, a0

	// *** Basic block 160

.EvaluateIntegerExpression_label_781:
	beqz        s8, .EvaluateIntegerExpression_label_790

	// *** Basic block 161

	ld          t0, -32(s0)
	ld          t1, -24(s0)
	sll         t0, t0, t1
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 162

.EvaluateIntegerExpression_label_790:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 163

.EvaluateIntegerExpression_label_792:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 164

	mv          s7, a0
	beqz        a0, .EvaluateIntegerExpression_label_809

	// *** Basic block 165

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 166

	mv          s7, a0

	// *** Basic block 167

.EvaluateIntegerExpression_label_809:
	beqz        s7, .EvaluateIntegerExpression_label_818

	// *** Basic block 168

	ld          t0, -32(s0)
	ld          t1, -24(s0)
	sll         t0, t0, t1
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 169

.EvaluateIntegerExpression_label_818:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 170

.EvaluateIntegerExpression_label_820:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 171

	mv          s7, a0
	beqz        a0, .EvaluateIntegerExpression_label_837

	// *** Basic block 172

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 173

	mv          s7, a0

	// *** Basic block 174

.EvaluateIntegerExpression_label_837:
	beqz        s7, .EvaluateIntegerExpression_label_846

	// *** Basic block 175

	ld          t0, -32(s0)
	ld          t1, -24(s0)
	srl         t0, t0, t1
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 176

.EvaluateIntegerExpression_label_846:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 177

.EvaluateIntegerExpression_label_848:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 178

	mv          s8, a0
	beqz        a0, .EvaluateIntegerExpression_label_865

	// *** Basic block 179

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 180

	mv          s8, a0

	// *** Basic block 181

.EvaluateIntegerExpression_label_865:
	beqz        s8, .EvaluateIntegerExpression_label_874

	// *** Basic block 182

	ld          t0, -32(s0)
	ld          t1, -24(s0)
	slt         t0, t0, t1
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 183

.EvaluateIntegerExpression_label_874:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 184

.EvaluateIntegerExpression_label_876:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 185

	mv          s8, a0
	beqz        a0, .EvaluateIntegerExpression_label_893

	// *** Basic block 186

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 187

	mv          s8, a0

	// *** Basic block 188

.EvaluateIntegerExpression_label_893:
	beqz        s8, .EvaluateIntegerExpression_label_903

	// *** Basic block 189

	ld          t0, -32(s0)
	ld          t1, -24(s0)
	slt         t0, t1, t0
	not         t0, t0
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 190

.EvaluateIntegerExpression_label_903:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 191

.EvaluateIntegerExpression_label_905:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 192

	mv          s8, a0
	beqz        a0, .EvaluateIntegerExpression_label_922

	// *** Basic block 193

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 194

	mv          s8, a0

	// *** Basic block 195

.EvaluateIntegerExpression_label_922:
	beqz        s8, .EvaluateIntegerExpression_label_931

	// *** Basic block 196

	ld          t0, -32(s0)
	ld          t1, -24(s0)
	slt         t0, t1, t0
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 197

.EvaluateIntegerExpression_label_931:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 198

.EvaluateIntegerExpression_label_933:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 199

	mv          s8, a0
	beqz        a0, .EvaluateIntegerExpression_label_950

	// *** Basic block 200

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 201

	mv          s8, a0

	// *** Basic block 202

.EvaluateIntegerExpression_label_950:
	beqz        s8, .EvaluateIntegerExpression_label_960

	// *** Basic block 203

	ld          t0, -32(s0)
	ld          t1, -24(s0)
	slt         t0, t0, t1
	not         t0, t0
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 204

.EvaluateIntegerExpression_label_960:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 205

.EvaluateIntegerExpression_label_962:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 206

	mv          s8, a0
	beqz        a0, .EvaluateIntegerExpression_label_979

	// *** Basic block 207

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 208

	mv          s8, a0

	// *** Basic block 209

.EvaluateIntegerExpression_label_979:
	beqz        s8, .EvaluateIntegerExpression_label_989

	// *** Basic block 210

	ld          t0, -32(s0)
	ld          t1, -24(s0)
	sub         t0, t0, t1
	seqz        t0, t0
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 211

.EvaluateIntegerExpression_label_989:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 212

.EvaluateIntegerExpression_label_991:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 213

	mv          s8, a0
	beqz        a0, .EvaluateIntegerExpression_label_1008

	// *** Basic block 214

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 215

	mv          s8, a0

	// *** Basic block 216

.EvaluateIntegerExpression_label_1008:
	beqz        s8, .EvaluateIntegerExpression_label_1018

	// *** Basic block 217

	ld          t0, -32(s0)
	ld          t1, -24(s0)
	sub         t0, t0, t1
	snez        t0, t0
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 218

.EvaluateIntegerExpression_label_1018:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 219

.EvaluateIntegerExpression_label_1020:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 220

	mv          s8, a0
	beqz        a0, .EvaluateIntegerExpression_label_1037

	// *** Basic block 221

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 222

	mv          s8, a0

	// *** Basic block 223

.EvaluateIntegerExpression_label_1037:
	beqz        s8, .EvaluateIntegerExpression_label_1046

	// *** Basic block 224

	ld          t0, -32(s0)
	ld          t1, -24(s0)
	and         t0, t0, t1
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 225

.EvaluateIntegerExpression_label_1046:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 226

.EvaluateIntegerExpression_label_1048:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 227

	mv          s8, a0
	beqz        a0, .EvaluateIntegerExpression_label_1065

	// *** Basic block 228

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 229

	mv          s8, a0

	// *** Basic block 230

.EvaluateIntegerExpression_label_1065:
	beqz        s8, .EvaluateIntegerExpression_label_1074

	// *** Basic block 231

	ld          t0, -32(s0)
	ld          t1, -24(s0)
	or          t0, t0, t1
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 232

.EvaluateIntegerExpression_label_1074:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 233

.EvaluateIntegerExpression_label_1076:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 234

	mv          s8, a0
	beqz        a0, .EvaluateIntegerExpression_label_1093

	// *** Basic block 235

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 236

	mv          s8, a0

	// *** Basic block 237

.EvaluateIntegerExpression_label_1093:
	beqz        s8, .EvaluateIntegerExpression_label_1102

	// *** Basic block 238

	ld          t0, -32(s0)
	ld          t1, -24(s0)
	xor         t0, t0, t1
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 239

.EvaluateIntegerExpression_label_1102:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 240

.EvaluateIntegerExpression_label_1104:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 241

	beqz        a0, .EvaluateIntegerExpression_label_1118

	// *** Basic block 242

	ld          t0, -32(s0)
	neg         t0, t0
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 243

.EvaluateIntegerExpression_label_1118:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 244

.EvaluateIntegerExpression_label_1120:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 245

	beqz        a0, .EvaluateIntegerExpression_label_1133

	// *** Basic block 246

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 247

.EvaluateIntegerExpression_label_1133:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 248

.EvaluateIntegerExpression_label_1135:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 249

	beqz        a0, .EvaluateIntegerExpression_label_1149

	// *** Basic block 250

	ld          t0, -32(s0)
	not         t0, t0
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 251

.EvaluateIntegerExpression_label_1149:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 252

.EvaluateIntegerExpression_label_1151:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 253

	beqz        a0, .EvaluateIntegerExpression_label_1165

	// *** Basic block 254

	ld          t0, -32(s0)
	not         t0, t0
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 255

.EvaluateIntegerExpression_label_1165:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 256

.EvaluateIntegerExpression_label_1167:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 257

	beqz        a0, .EvaluateIntegerExpression_label_1193

	// *** Basic block 258

	ld          t0, -32(s0)
	beqz        t0, .EvaluateIntegerExpression_label_1192

	// *** Basic block 259

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 260

	beqz        a0, .EvaluateIntegerExpression_label_1191

	// *** Basic block 261

	ld          t0, -24(s0)
	snez        t0, t0
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 262

.EvaluateIntegerExpression_label_1191:

	// *** Basic block 263

.EvaluateIntegerExpression_label_1192:

	// *** Basic block 264

.EvaluateIntegerExpression_label_1193:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 265

.EvaluateIntegerExpression_label_1195:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 266

	beqz        a0, .EvaluateIntegerExpression_label_1228

	// *** Basic block 267

	ld          t0, -32(s0)
	bnez        t0, .EvaluateIntegerExpression_label_1221

	// *** Basic block 268

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 269

	beqz        a0, .EvaluateIntegerExpression_label_1219

	// *** Basic block 270

	ld          t0, -24(s0)
	snez        t0, t0
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 271

.EvaluateIntegerExpression_label_1219:
	j           .EvaluateIntegerExpression_label_1227

	// *** Basic block 272

.EvaluateIntegerExpression_label_1221:
	li          t0, 1		// 0x1 ASCII \x1
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 273

.EvaluateIntegerExpression_label_1227:

	// *** Basic block 274

.EvaluateIntegerExpression_label_1228:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 275

.EvaluateIntegerExpression_label_1230:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 276

	beqz        a0, .EvaluateIntegerExpression_label_1264

	// *** Basic block 277

	ld          t0, -32(s0)
	beqz        t0, .EvaluateIntegerExpression_label_1246

	// *** Basic block 278

	ld          t0, 64(s5)
	ld          s1, 56(t0)
	j           .EvaluateIntegerExpression_label_1251

	// *** Basic block 279

.EvaluateIntegerExpression_label_1246:
	ld          t0, 64(s5)
	ld          s1, 64(t0)

	// *** Basic block 280

.EvaluateIntegerExpression_label_1251:
	addi        a1, s0, -32
	mv          a0, s1
	call        EvaluateIntegerExpression

	// *** Basic block 281

	beqz        a0, .EvaluateIntegerExpression_label_1263

	// *** Basic block 282

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 283

.EvaluateIntegerExpression_label_1263:

	// *** Basic block 284

.EvaluateIntegerExpression_label_1264:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 285

.EvaluateIntegerExpression_label_1266:
	mv          s10, s1
	ld          a0, 64(s10)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 286

	beqz        a0, .EvaluateIntegerExpression_label_1281

	// *** Basic block 287

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 288

.EvaluateIntegerExpression_label_1281:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 289

.EvaluateIntegerExpression_label_1283:

	// *** Basic block 290

.EvaluateIntegerExpression_label_1284:

	// *** Basic block 291

.EvaluateIntegerExpression_label_1285:

	// *** Basic block 292

.EvaluateIntegerExpression_label_1286:

	// *** Basic block 293

.EvaluateIntegerExpression_label_1287:

	// *** Basic block 294

.EvaluateIntegerExpression_label_1288:

	// *** Basic block 295

.EvaluateIntegerExpression_label_1289:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 296

	beqz        a0, .EvaluateIntegerExpression_label_1310

	// *** Basic block 297

	ld          t1, -32(s0)
	beqz        t1, .EvaluateIntegerExpression_label_1303

	// *** Basic block 298

	li          t0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_1305

	// *** Basic block 299

.EvaluateIntegerExpression_label_1303:
	mv          t0, x0

	// *** Basic block 300

.EvaluateIntegerExpression_label_1305:
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 301

.EvaluateIntegerExpression_label_1310:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 302

.EvaluateIntegerExpression_label_1312:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 303

	beqz        a0, .EvaluateIntegerExpression_label_1325

	// *** Basic block 304

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 305

.EvaluateIntegerExpression_label_1325:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 306

.EvaluateIntegerExpression_label_1327:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 307

	beqz        a0, .EvaluateIntegerExpression_label_1340

	// *** Basic block 308

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 309

.EvaluateIntegerExpression_label_1340:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 310

.EvaluateIntegerExpression_label_1342:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 311

	beqz        a0, .EvaluateIntegerExpression_label_1355

	// *** Basic block 312

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 313

.EvaluateIntegerExpression_label_1355:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 314

.EvaluateIntegerExpression_label_1357:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 315

	beqz        a0, .EvaluateIntegerExpression_label_1370

	// *** Basic block 316

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 317

.EvaluateIntegerExpression_label_1370:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 318

.EvaluateIntegerExpression_label_1372:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 319

	beqz        a0, .EvaluateIntegerExpression_label_1385

	// *** Basic block 320

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 321

.EvaluateIntegerExpression_label_1385:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 322

.EvaluateIntegerExpression_label_1387:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 323

	beqz        a0, .EvaluateIntegerExpression_label_1400

	// *** Basic block 324

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 325

.EvaluateIntegerExpression_label_1400:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 326

.EvaluateIntegerExpression_label_1402:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 327

	beqz        a0, .EvaluateIntegerExpression_label_1415

	// *** Basic block 328

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 329

.EvaluateIntegerExpression_label_1415:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 330

.EvaluateIntegerExpression_label_1417:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 331

	beqz        a0, .EvaluateIntegerExpression_label_1430

	// *** Basic block 332

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 333

.EvaluateIntegerExpression_label_1430:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 334

.EvaluateIntegerExpression_label_1432:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 335

	beqz        a0, .EvaluateIntegerExpression_label_1445

	// *** Basic block 336

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 337

.EvaluateIntegerExpression_label_1445:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 338

.EvaluateIntegerExpression_label_1447:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 339

	beqz        a0, .EvaluateIntegerExpression_label_1460

	// *** Basic block 340

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 341

.EvaluateIntegerExpression_label_1460:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 342

.EvaluateIntegerExpression_label_1462:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 343

	beqz        a0, .EvaluateIntegerExpression_label_1475

	// *** Basic block 344

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 345

.EvaluateIntegerExpression_label_1475:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 346

.EvaluateIntegerExpression_label_1477:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 347

	beqz        a0, .EvaluateIntegerExpression_label_1490

	// *** Basic block 348

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 349

.EvaluateIntegerExpression_label_1490:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 350

.EvaluateIntegerExpression_label_1492:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 351

	beqz        a0, .EvaluateIntegerExpression_label_1505

	// *** Basic block 352

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 353

.EvaluateIntegerExpression_label_1505:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 354

.EvaluateIntegerExpression_label_1507:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 355

	beqz        a0, .EvaluateIntegerExpression_label_1520

	// *** Basic block 356

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 357

.EvaluateIntegerExpression_label_1520:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 358

.EvaluateIntegerExpression_label_1522:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 359

	beqz        a0, .EvaluateIntegerExpression_label_1535

	// *** Basic block 360

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 361

.EvaluateIntegerExpression_label_1535:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 362

.EvaluateIntegerExpression_label_1537:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 363

	beqz        a0, .EvaluateIntegerExpression_label_1550

	// *** Basic block 364

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 365

.EvaluateIntegerExpression_label_1550:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 366

.EvaluateIntegerExpression_label_1552:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 367

	beqz        a0, .EvaluateIntegerExpression_label_1565

	// *** Basic block 368

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 369

.EvaluateIntegerExpression_label_1565:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 370

.EvaluateIntegerExpression_label_1567:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 371

	beqz        a0, .EvaluateIntegerExpression_label_1580

	// *** Basic block 372

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 373

.EvaluateIntegerExpression_label_1580:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 374

.EvaluateIntegerExpression_label_1582:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 375

	beqz        a0, .EvaluateIntegerExpression_label_1595

	// *** Basic block 376

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 377

.EvaluateIntegerExpression_label_1595:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 378

.EvaluateIntegerExpression_label_1597:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 379

	beqz        a0, .EvaluateIntegerExpression_label_1610

	// *** Basic block 380

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 381

.EvaluateIntegerExpression_label_1610:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 382

.EvaluateIntegerExpression_label_1612:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 383

	beqz        a0, .EvaluateIntegerExpression_label_1625

	// *** Basic block 384

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 385

.EvaluateIntegerExpression_label_1625:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 386

.EvaluateIntegerExpression_label_1627:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 387

	beqz        a0, .EvaluateIntegerExpression_label_1640

	// *** Basic block 388

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 389

.EvaluateIntegerExpression_label_1640:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 390

.EvaluateIntegerExpression_label_1642:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 391

	beqz        a0, .EvaluateIntegerExpression_label_1655

	// *** Basic block 392

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 393

.EvaluateIntegerExpression_label_1655:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 394

.EvaluateIntegerExpression_label_1657:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 395

	beqz        a0, .EvaluateIntegerExpression_label_1670

	// *** Basic block 396

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 397

.EvaluateIntegerExpression_label_1670:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 398

.EvaluateIntegerExpression_label_1672:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 399

	beqz        a0, .EvaluateIntegerExpression_label_1685

	// *** Basic block 400

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 401

.EvaluateIntegerExpression_label_1685:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 402

.EvaluateIntegerExpression_label_1687:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 403

	beqz        a0, .EvaluateIntegerExpression_label_1700

	// *** Basic block 404

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 405

.EvaluateIntegerExpression_label_1700:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 406

.EvaluateIntegerExpression_label_1702:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 407

	beqz        a0, .EvaluateIntegerExpression_label_1715

	// *** Basic block 408

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 409

.EvaluateIntegerExpression_label_1715:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 410

.EvaluateIntegerExpression_label_1717:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 411

	beqz        a0, .EvaluateIntegerExpression_label_1730

	// *** Basic block 412

	ld          t0, -32(s0)
	sd          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 413

.EvaluateIntegerExpression_label_1730:
	j           .EvaluateIntegerExpression_label_1794

	// *** Basic block 414

.EvaluateIntegerExpression_label_1732:
	mv          s11, s1
	ld          t1, 64(s11)
	sd          t1, -40(s0)	// Spilled @1737
	sub         t2, t1, x0
	snez        t0, t2
	beq         t1, x0, .EvaluateIntegerExpression_label_1766

	// *** Basic block 415

	ld          t1, 16(t1)
	lw          t2, 16(t1)
	addi        t3, t2, -2
	seqz        a0, t3
	li          t3, 2		// 0x2 ASCII \x2
	beq         t2, t3, .EvaluateIntegerExpression_label_1754

	// *** Basic block 416

	addi        t2, t2, -1
	seqz        a0, t2

	// *** Basic block 417

.EvaluateIntegerExpression_label_1754:
	beqz        a0, .EvaluateIntegerExpression_label_1761

	// *** Basic block 418

	addi        t1, t1, 32
	lb          t1, 16(t1)
	slli        t1, t1, 61
	srai        a0, t1, 63

	// *** Basic block 419

.EvaluateIntegerExpression_label_1761:

	// *** Basic block 420

.EvaluateIntegerExpression_label_1763:
	mv          t0, a0
	j           .EvaluateIntegerExpression_label_1766

	// *** Basic block 421

.EvaluateIntegerExpression_label_1766:
	beqz        t0, .EvaluateIntegerExpression_label_1771

	// *** Basic block 422

	mv          a0, x0
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 423

.EvaluateIntegerExpression_label_1771:
	ld          t1, 56(s11)
	sd          t1, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 424

.EvaluateIntegerExpression_label_1778:
	mv          s9, s1
	ld          a0, 56(s9)
	mv          a1, s2
	call        EvaluateIntegerExpression

	// *** Basic block 425

	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 426

.EvaluateIntegerExpression_label_1790:
	mv          a0, x0
	j           .EvaluateIntegerExpression_label_99

	// *** Basic block 427

.EvaluateIntegerExpression_label_1794:
	mv          a0, x0
	j           .EvaluateIntegerExpression_label_99
.func_end_EvaluateIntegerExpression:
	.size EvaluateIntegerExpression, .func_end_EvaluateIntegerExpression-EvaluateIntegerExpression

	.global EvaluateFloatingPointExpression
	.type EvaluateFloatingPointExpression, @function

EvaluateFloatingPointExpression:

	// *** Basic block 0

	.global TypeIsIntegral
	.global TypeIsFloatingPoint
	.global EvaluateIntegerExpression
	.global EvaluateFloatingPointExpression
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
	mv          s1, a0
	mv          s2, a1
	bne         s1, x0, .EvaluateFloatingPointExpression_label_74

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.EvaluateFloatingPointExpression_label_71:
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

	// *** Basic block 3

.EvaluateFloatingPointExpression_label_74:
	ld          s3, 16(s1)
	bne         s3, x0, .EvaluateFloatingPointExpression_label_83

	// *** Basic block 4

	mv          a0, x0
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 5

.EvaluateFloatingPointExpression_label_83:
	mv          a0, s3
	call        TypeIsIntegral

	// *** Basic block 6

	not         s4, a0
	beqz        s4, .EvaluateFloatingPointExpression_label_94

	// *** Basic block 7

	mv          a0, s3
	call        TypeIsFloatingPoint

	// *** Basic block 8

	not         s4, a0

	// *** Basic block 9

.EvaluateFloatingPointExpression_label_94:
	beqz        s4, .EvaluateFloatingPointExpression_label_99

	// *** Basic block 10

	mv          a0, x0
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 11

.EvaluateFloatingPointExpression_label_99:
	mv          s3, s1
	mv          s4, s1
	mv          s5, s1
	mv          s6, s1
	lw          s7, 0(s1)
	li          s8, 87		// 0x57 ASCII 'W'
	blt         s7, s8, .EvaluateFloatingPointExpression_label_228

	// *** Basic block 12

	li          s9, 128		// 0x80 ASCII \x80
	blt         s7, s9, .EvaluateFloatingPointExpression_label_171

	// *** Basic block 13

	beq         s7, s9, .EvaluateFloatingPointExpression_label_1056

	// *** Basic block 14

	li          t0, 129		// 0x81 ASCII \x81
	beq         s7, t0, .EvaluateFloatingPointExpression_label_1161

	// *** Basic block 15

	li          t0, 136		// 0x88 ASCII \x88
	beq         s7, t0, .EvaluateFloatingPointExpression_label_1071

	// *** Basic block 16

	li          t0, 137		// 0x89 ASCII \x89
	beq         s7, t0, .EvaluateFloatingPointExpression_label_1176

	// *** Basic block 17

	li          t0, 144		// 0x90 ASCII \x90
	beq         s7, t0, .EvaluateFloatingPointExpression_label_962

	// *** Basic block 18

	li          t0, 145		// 0x91 ASCII \x91
	beq         s7, t0, .EvaluateFloatingPointExpression_label_1191

	// *** Basic block 19

	li          t0, 152		// 0x98 ASCII \x98
	beq         s7, t0, .EvaluateFloatingPointExpression_label_979

	// *** Basic block 20

	li          t0, 153		// 0x99 ASCII \x99
	beq         s7, t0, .EvaluateFloatingPointExpression_label_1086

	// *** Basic block 21

	li          t0, 160		// 0xa0 ASCII \xa0
	beq         s7, t0, .EvaluateFloatingPointExpression_label_877

	// *** Basic block 22

	li          t0, 161		// 0xa1 ASCII \xa1
	beq         s7, t0, .EvaluateFloatingPointExpression_label_996

	// *** Basic block 23

	li          t0, 162		// 0xa2 ASCII \xa2
	beq         s7, t0, .EvaluateFloatingPointExpression_label_1101

	// *** Basic block 24

	j           .EvaluateFloatingPointExpression_label_1218

	// *** Basic block 25

.EvaluateFloatingPointExpression_label_171:
	beq         s7, s8, .EvaluateFloatingPointExpression_label_1206

	// *** Basic block 26

	li          t0, 95		// 0x5f ASCII '_'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_894

	// *** Basic block 27

	li          t0, 96		// 0x60 ASCII '`'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_1011

	// *** Basic block 28

	li          t0, 97		// 0x61 ASCII 'a'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_1116

	// *** Basic block 29

	li          t0, 111		// 0x6f ASCII 'o'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_911

	// *** Basic block 30

	li          t0, 112		// 0x70 ASCII 'p'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_1026

	// *** Basic block 31

	li          t0, 113		// 0x71 ASCII 'q'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_1131

	// *** Basic block 32

	li          t0, 119		// 0x77 ASCII 'w'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_928

	// *** Basic block 33

	li          t0, 120		// 0x78 ASCII 'x'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_1041

	// *** Basic block 34

	li          t0, 121		// 0x79 ASCII 'y'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_1146

	// *** Basic block 35

	li          t0, 127		// 0x7f ASCII \x7f
	beq         s7, t0, .EvaluateFloatingPointExpression_label_945

	// *** Basic block 36

	j           .EvaluateFloatingPointExpression_label_1218

	// *** Basic block 37

.EvaluateFloatingPointExpression_label_228:
	li          s9, 42		// 0x2a ASCII '*'
	blt         s7, s9, .EvaluateFloatingPointExpression_label_288

	// *** Basic block 38

	beq         s7, s9, .EvaluateFloatingPointExpression_label_522

	// *** Basic block 39

	li          t0, 43		// 0x2b ASCII '+'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_551

	// *** Basic block 40

	li          t0, 44		// 0x2c ASCII ','
	beq         s7, t0, .EvaluateFloatingPointExpression_label_748

	// *** Basic block 41

	li          t0, 45		// 0x2d ASCII '-'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_781

	// *** Basic block 42

	li          t0, 51		// 0x33 ASCII '3'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_431

	// *** Basic block 43

	li          t0, 54		// 0x36 ASCII '6'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_667

	// *** Basic block 44

	li          t0, 58		// 0x3a ASCII ':'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_400

	// *** Basic block 45

	li          t0, 61		// 0x3d ASCII '='
	beq         s7, t0, .EvaluateFloatingPointExpression_label_822

	// *** Basic block 46

	li          t0, 66		// 0x42 ASCII 'B'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_487

	// *** Basic block 47

	li          t0, 68		// 0x44 ASCII 'D'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_459

	// *** Basic block 48

	li          t0, 80		// 0x50 ASCII 'P'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_860

	// *** Basic block 49

	j           .EvaluateFloatingPointExpression_label_1218

	// *** Basic block 50

.EvaluateFloatingPointExpression_label_288:
	li          t0, 1		// 0x1 ASCII \x1
	beq         s7, t0, .EvaluateFloatingPointExpression_label_340

	// *** Basic block 51

	li          t0, 2		// 0x2 ASCII \x2
	beq         s7, t0, .EvaluateFloatingPointExpression_label_358

	// *** Basic block 52

	li          t0, 5		// 0x5 ASCII \x5
	beq         s7, t0, .EvaluateFloatingPointExpression_label_341

	// *** Basic block 53

	li          t0, 6		// 0x6 ASCII \x6
	beq         s7, t0, .EvaluateFloatingPointExpression_label_349

	// *** Basic block 54

	li          t0, 9		// 0x9 ASCII \x9
	beq         s7, t0, .EvaluateFloatingPointExpression_label_697

	// *** Basic block 55

	li          t0, 10		// 0xa ASCII \xa
	beq         s7, t0, .EvaluateFloatingPointExpression_label_713

	// *** Basic block 56

	li          t0, 21		// 0x15 ASCII \x15
	beq         s7, t0, .EvaluateFloatingPointExpression_label_728

	// *** Basic block 57

	li          t0, 34		// 0x22 ASCII '"'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_638

	// *** Basic block 58

	li          t0, 37		// 0x25 ASCII '%'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_580

	// *** Basic block 59

	li          t0, 38		// 0x26 ASCII '&'
	beq         s7, t0, .EvaluateFloatingPointExpression_label_609

	// *** Basic block 60

	j           .EvaluateFloatingPointExpression_label_1218

	// *** Basic block 61

.EvaluateFloatingPointExpression_label_340:

	// *** Basic block 62

.EvaluateFloatingPointExpression_label_341:
	ld          t0, 56(s3)
	fcvt.d.l    ft0, t0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 63

.EvaluateFloatingPointExpression_label_349:
	fld         ft0, 56(s3)
	fcvt.l.d    t0, ft0
	fcvt.d.l    ft0, t0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 64

.EvaluateFloatingPointExpression_label_358:
	ld          s7, 56(s4)
	ld          s10, 40(s7)
	lw          t0, 12(s10)
	andi        t0, t0, 4
	bnez        t0, .EvaluateFloatingPointExpression_label_372

	// *** Basic block 65

	mv          a0, x0
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 66

.EvaluateFloatingPointExpression_label_372:
	mv          a0, s10
	call        TypeIsIntegral

	// *** Basic block 67

	beqz        a0, .EvaluateFloatingPointExpression_label_382

	// *** Basic block 68

	ld          t0, 112(s7)
	fcvt.d.l    ft0, t0
	fsd         ft0, 0(s2)
	j           .EvaluateFloatingPointExpression_label_396

	// *** Basic block 69

.EvaluateFloatingPointExpression_label_382:
	mv          a0, s10
	call        TypeIsFloatingPoint

	// *** Basic block 70

	beqz        a0, .EvaluateFloatingPointExpression_label_391

	// *** Basic block 71

	fld         ft0, 112(s7)
	fsd         ft0, 0(s2)
	j           .EvaluateFloatingPointExpression_label_395

	// *** Basic block 72

.EvaluateFloatingPointExpression_label_391:
	mv          a0, x0
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 73

.EvaluateFloatingPointExpression_label_395:

	// *** Basic block 74

.EvaluateFloatingPointExpression_label_396:
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 75

.EvaluateFloatingPointExpression_label_400:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 76

	mv          s10, a0
	beqz        a0, .EvaluateFloatingPointExpression_label_420

	// *** Basic block 77

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 78

	mv          s10, a0

	// *** Basic block 79

.EvaluateFloatingPointExpression_label_420:
	beqz        s10, .EvaluateFloatingPointExpression_label_429

	// *** Basic block 80

	fld         ft0, -32(s0)
	fld         ft1, -24(s0)
	fadd.d      ft0, ft0, ft1
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 81

.EvaluateFloatingPointExpression_label_429:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 82

.EvaluateFloatingPointExpression_label_431:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 83

	mv          s10, a0
	beqz        a0, .EvaluateFloatingPointExpression_label_448

	// *** Basic block 84

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 85

	mv          s10, a0

	// *** Basic block 86

.EvaluateFloatingPointExpression_label_448:
	beqz        s10, .EvaluateFloatingPointExpression_label_457

	// *** Basic block 87

	fld         ft0, -32(s0)
	fld         ft1, -24(s0)
	fsub.d      ft0, ft0, ft1
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 88

.EvaluateFloatingPointExpression_label_457:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 89

.EvaluateFloatingPointExpression_label_459:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 90

	mv          s10, a0
	beqz        a0, .EvaluateFloatingPointExpression_label_476

	// *** Basic block 91

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 92

	mv          s10, a0

	// *** Basic block 93

.EvaluateFloatingPointExpression_label_476:
	beqz        s10, .EvaluateFloatingPointExpression_label_485

	// *** Basic block 94

	fld         ft0, -32(s0)
	fld         ft1, -24(s0)
	fmul.d      ft0, ft0, ft1
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 95

.EvaluateFloatingPointExpression_label_485:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 96

.EvaluateFloatingPointExpression_label_487:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateFloatingPointExpression

	// *** Basic block 97

	mv          s10, a0
	beqz        a0, .EvaluateFloatingPointExpression_label_504

	// *** Basic block 98

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateFloatingPointExpression

	// *** Basic block 99

	mv          s10, a0

	// *** Basic block 100

.EvaluateFloatingPointExpression_label_504:
	beqz        s10, .EvaluateFloatingPointExpression_label_520

	// *** Basic block 101

	fld         ft0, -24(s0)
	fmv.d.x     ft1, x0
	feq.d       t0, ft0, ft1
	beqz        t0, .EvaluateFloatingPointExpression_label_513

	// *** Basic block 102

	mv          a0, x0
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 103

.EvaluateFloatingPointExpression_label_513:
	fld         ft1, -32(s0)
	fdiv.d      ft0, ft1, ft0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 104

.EvaluateFloatingPointExpression_label_520:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 105

.EvaluateFloatingPointExpression_label_522:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 106

	mv          s10, a0
	beqz        a0, .EvaluateFloatingPointExpression_label_539

	// *** Basic block 107

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 108

	mv          s10, a0

	// *** Basic block 109

.EvaluateFloatingPointExpression_label_539:
	beqz        s10, .EvaluateFloatingPointExpression_label_549

	// *** Basic block 110

	fld         ft0, -32(s0)
	fld         ft1, -24(s0)
	flt.d       t0, ft0, ft1
	fcvt.d.l    ft0, t0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 111

.EvaluateFloatingPointExpression_label_549:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 112

.EvaluateFloatingPointExpression_label_551:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 113

	mv          s10, a0
	beqz        a0, .EvaluateFloatingPointExpression_label_568

	// *** Basic block 114

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 115

	mv          s10, a0

	// *** Basic block 116

.EvaluateFloatingPointExpression_label_568:
	beqz        s10, .EvaluateFloatingPointExpression_label_578

	// *** Basic block 117

	fld         ft0, -32(s0)
	fld         ft1, -24(s0)
	fle.d       t0, ft0, ft1
	fcvt.d.l    ft0, t0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 118

.EvaluateFloatingPointExpression_label_578:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 119

.EvaluateFloatingPointExpression_label_580:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 120

	mv          s7, a0
	beqz        a0, .EvaluateFloatingPointExpression_label_597

	// *** Basic block 121

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 122

	mv          s7, a0

	// *** Basic block 123

.EvaluateFloatingPointExpression_label_597:
	beqz        s7, .EvaluateFloatingPointExpression_label_607

	// *** Basic block 124

	fld         ft0, -32(s0)
	fld         ft1, -24(s0)
	flt.d       t0, ft1, ft0
	fcvt.d.l    ft0, t0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 125

.EvaluateFloatingPointExpression_label_607:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 126

.EvaluateFloatingPointExpression_label_609:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 127

	mv          s7, a0
	beqz        a0, .EvaluateFloatingPointExpression_label_626

	// *** Basic block 128

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 129

	mv          s7, a0

	// *** Basic block 130

.EvaluateFloatingPointExpression_label_626:
	beqz        s7, .EvaluateFloatingPointExpression_label_636

	// *** Basic block 131

	fld         ft0, -32(s0)
	fld         ft1, -24(s0)
	fle.d       t0, ft1, ft0
	fcvt.d.l    ft0, t0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 132

.EvaluateFloatingPointExpression_label_636:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 133

.EvaluateFloatingPointExpression_label_638:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 134

	mv          s7, a0
	beqz        a0, .EvaluateFloatingPointExpression_label_655

	// *** Basic block 135

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 136

	mv          s7, a0

	// *** Basic block 137

.EvaluateFloatingPointExpression_label_655:
	beqz        s7, .EvaluateFloatingPointExpression_label_665

	// *** Basic block 138

	fld         ft0, -32(s0)
	fld         ft1, -24(s0)
	feq.d       t0, ft0, ft1
	fcvt.d.l    ft0, t0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 139

.EvaluateFloatingPointExpression_label_665:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 140

.EvaluateFloatingPointExpression_label_667:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 141

	mv          s10, a0
	beqz        a0, .EvaluateFloatingPointExpression_label_684

	// *** Basic block 142

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateIntegerExpression

	// *** Basic block 143

	mv          s10, a0

	// *** Basic block 144

.EvaluateFloatingPointExpression_label_684:
	beqz        s10, .EvaluateFloatingPointExpression_label_695

	// *** Basic block 145

	fld         ft0, -32(s0)
	fld         ft1, -24(s0)
	feq.d       t0, ft0, ft1
	not         t0, t0
	fcvt.d.l    ft0, t0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 146

.EvaluateFloatingPointExpression_label_695:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 147

.EvaluateFloatingPointExpression_label_697:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 148

	beqz        a0, .EvaluateFloatingPointExpression_label_711

	// *** Basic block 149

	fld         ft0, -32(s0)
	fneg.d      ft0, ft0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 150

.EvaluateFloatingPointExpression_label_711:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 151

.EvaluateFloatingPointExpression_label_713:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 152

	beqz        a0, .EvaluateFloatingPointExpression_label_726

	// *** Basic block 153

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 154

.EvaluateFloatingPointExpression_label_726:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 155

.EvaluateFloatingPointExpression_label_728:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 156

	beqz        a0, .EvaluateFloatingPointExpression_label_746

	// *** Basic block 157

	fld         ft0, -32(s0)
	fcvt.l.d    t0, ft0
	andi        t0, t0, 255
	not         t0, t0
	fcvt.d.l    ft0, t0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 158

.EvaluateFloatingPointExpression_label_746:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 159

.EvaluateFloatingPointExpression_label_748:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateFloatingPointExpression

	// *** Basic block 160

	beqz        a0, .EvaluateFloatingPointExpression_label_779

	// *** Basic block 161

	fld         ft0, -32(s0)
	fmv.d.x     ft1, x0
	feq.d       t0, ft0, ft1
	not         t0, t0
	beqz        t0, .EvaluateFloatingPointExpression_label_778

	// *** Basic block 162

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateFloatingPointExpression

	// *** Basic block 163

	beqz        a0, .EvaluateFloatingPointExpression_label_777

	// *** Basic block 164

	fld         ft0, -24(s0)
	fmv.d.x     ft1, x0
	feq.d       t0, ft0, ft1
	not         t0, t0
	fcvt.d.l    ft0, t0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 165

.EvaluateFloatingPointExpression_label_777:

	// *** Basic block 166

.EvaluateFloatingPointExpression_label_778:

	// *** Basic block 167

.EvaluateFloatingPointExpression_label_779:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 168

.EvaluateFloatingPointExpression_label_781:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateFloatingPointExpression

	// *** Basic block 169

	beqz        a0, .EvaluateFloatingPointExpression_label_820

	// *** Basic block 170

	fld         ft0, -32(s0)
	fmv.d.x     ft1, x0
	feq.d       t0, ft0, ft1
	beqz        t0, .EvaluateFloatingPointExpression_label_811

	// *** Basic block 171

	ld          a0, 64(s5)
	addi        a1, s0, -24
	call        EvaluateFloatingPointExpression

	// *** Basic block 172

	beqz        a0, .EvaluateFloatingPointExpression_label_809

	// *** Basic block 173

	fld         ft0, -24(s0)
	fmv.d.x     ft1, x0
	feq.d       t0, ft0, ft1
	not         t0, t0
	fcvt.d.l    ft0, t0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 174

.EvaluateFloatingPointExpression_label_809:
	j           .EvaluateFloatingPointExpression_label_819

	// *** Basic block 175

.EvaluateFloatingPointExpression_label_811:
	li          t0, 4607182418800017408		// 0x3ff0000000000000
	fmv.d.x     ft0, t0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 176

.EvaluateFloatingPointExpression_label_819:

	// *** Basic block 177

.EvaluateFloatingPointExpression_label_820:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 178

.EvaluateFloatingPointExpression_label_822:
	ld          a0, 56(s5)
	addi        a1, s0, -32
	call        EvaluateFloatingPointExpression

	// *** Basic block 179

	beqz        a0, .EvaluateFloatingPointExpression_label_858

	// *** Basic block 180

	fld         ft0, -32(s0)
	fmv.d.x     ft1, x0
	feq.d       t0, ft0, ft1
	not         t0, t0
	beqz        t0, .EvaluateFloatingPointExpression_label_840

	// *** Basic block 181

	ld          t0, 64(s5)
	ld          s1, 56(t0)
	j           .EvaluateFloatingPointExpression_label_845

	// *** Basic block 182

.EvaluateFloatingPointExpression_label_840:
	ld          t0, 64(s5)
	ld          s1, 64(t0)

	// *** Basic block 183

.EvaluateFloatingPointExpression_label_845:
	addi        a1, s0, -32
	mv          a0, s1
	call        EvaluateFloatingPointExpression

	// *** Basic block 184

	beqz        a0, .EvaluateFloatingPointExpression_label_857

	// *** Basic block 185

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 186

.EvaluateFloatingPointExpression_label_857:

	// *** Basic block 187

.EvaluateFloatingPointExpression_label_858:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 188

.EvaluateFloatingPointExpression_label_860:
	mv          s9, s1
	ld          a0, 64(s9)
	addi        a1, s0, -32
	call        EvaluateFloatingPointExpression

	// *** Basic block 189

	beqz        a0, .EvaluateFloatingPointExpression_label_875

	// *** Basic block 190

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 191

.EvaluateFloatingPointExpression_label_875:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 192

.EvaluateFloatingPointExpression_label_877:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 193

	beqz        a0, .EvaluateFloatingPointExpression_label_892

	// *** Basic block 194

	fld         ft0, -32(s0)
	fcvt.s.d    ft0, ft0
	fcvt.d.s    ft0, ft0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 195

.EvaluateFloatingPointExpression_label_892:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 196

.EvaluateFloatingPointExpression_label_894:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 197

	beqz        a0, .EvaluateFloatingPointExpression_label_909

	// *** Basic block 198

	fld         ft0, -32(s0)
	fcvt.s.d    ft0, ft0
	fcvt.d.s    ft0, ft0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 199

.EvaluateFloatingPointExpression_label_909:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 200

.EvaluateFloatingPointExpression_label_911:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 201

	beqz        a0, .EvaluateFloatingPointExpression_label_926

	// *** Basic block 202

	fld         ft0, -32(s0)
	fcvt.s.d    ft0, ft0
	fcvt.d.s    ft0, ft0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 203

.EvaluateFloatingPointExpression_label_926:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 204

.EvaluateFloatingPointExpression_label_928:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 205

	beqz        a0, .EvaluateFloatingPointExpression_label_943

	// *** Basic block 206

	fld         ft0, -32(s0)
	fcvt.s.d    ft0, ft0
	fcvt.d.s    ft0, ft0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 207

.EvaluateFloatingPointExpression_label_943:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 208

.EvaluateFloatingPointExpression_label_945:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 209

	beqz        a0, .EvaluateFloatingPointExpression_label_960

	// *** Basic block 210

	fld         ft0, -32(s0)
	fcvt.s.d    ft0, ft0
	fcvt.d.s    ft0, ft0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 211

.EvaluateFloatingPointExpression_label_960:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 212

.EvaluateFloatingPointExpression_label_962:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 213

	beqz        a0, .EvaluateFloatingPointExpression_label_977

	// *** Basic block 214

	fld         ft0, -32(s0)
	fcvt.s.d    ft0, ft0
	fcvt.d.s    ft0, ft0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 215

.EvaluateFloatingPointExpression_label_977:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 216

.EvaluateFloatingPointExpression_label_979:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 217

	beqz        a0, .EvaluateFloatingPointExpression_label_994

	// *** Basic block 218

	fld         ft0, -32(s0)
	fcvt.s.d    ft0, ft0
	fcvt.d.s    ft0, ft0
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 219

.EvaluateFloatingPointExpression_label_994:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 220

.EvaluateFloatingPointExpression_label_996:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 221

	beqz        a0, .EvaluateFloatingPointExpression_label_1009

	// *** Basic block 222

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 223

.EvaluateFloatingPointExpression_label_1009:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 224

.EvaluateFloatingPointExpression_label_1011:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 225

	beqz        a0, .EvaluateFloatingPointExpression_label_1024

	// *** Basic block 226

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 227

.EvaluateFloatingPointExpression_label_1024:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 228

.EvaluateFloatingPointExpression_label_1026:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 229

	beqz        a0, .EvaluateFloatingPointExpression_label_1039

	// *** Basic block 230

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 231

.EvaluateFloatingPointExpression_label_1039:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 232

.EvaluateFloatingPointExpression_label_1041:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 233

	beqz        a0, .EvaluateFloatingPointExpression_label_1054

	// *** Basic block 234

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 235

.EvaluateFloatingPointExpression_label_1054:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 236

.EvaluateFloatingPointExpression_label_1056:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 237

	beqz        a0, .EvaluateFloatingPointExpression_label_1069

	// *** Basic block 238

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 239

.EvaluateFloatingPointExpression_label_1069:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 240

.EvaluateFloatingPointExpression_label_1071:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 241

	beqz        a0, .EvaluateFloatingPointExpression_label_1084

	// *** Basic block 242

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 243

.EvaluateFloatingPointExpression_label_1084:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 244

.EvaluateFloatingPointExpression_label_1086:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 245

	beqz        a0, .EvaluateFloatingPointExpression_label_1099

	// *** Basic block 246

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 247

.EvaluateFloatingPointExpression_label_1099:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 248

.EvaluateFloatingPointExpression_label_1101:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 249

	beqz        a0, .EvaluateFloatingPointExpression_label_1114

	// *** Basic block 250

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 251

.EvaluateFloatingPointExpression_label_1114:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 252

.EvaluateFloatingPointExpression_label_1116:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 253

	beqz        a0, .EvaluateFloatingPointExpression_label_1129

	// *** Basic block 254

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 255

.EvaluateFloatingPointExpression_label_1129:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 256

.EvaluateFloatingPointExpression_label_1131:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 257

	beqz        a0, .EvaluateFloatingPointExpression_label_1144

	// *** Basic block 258

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 259

.EvaluateFloatingPointExpression_label_1144:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 260

.EvaluateFloatingPointExpression_label_1146:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 261

	beqz        a0, .EvaluateFloatingPointExpression_label_1159

	// *** Basic block 262

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 263

.EvaluateFloatingPointExpression_label_1159:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 264

.EvaluateFloatingPointExpression_label_1161:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 265

	beqz        a0, .EvaluateFloatingPointExpression_label_1174

	// *** Basic block 266

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 267

.EvaluateFloatingPointExpression_label_1174:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 268

.EvaluateFloatingPointExpression_label_1176:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 269

	beqz        a0, .EvaluateFloatingPointExpression_label_1189

	// *** Basic block 270

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 271

.EvaluateFloatingPointExpression_label_1189:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 272

.EvaluateFloatingPointExpression_label_1191:
	ld          a0, 56(s6)
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 273

	beqz        a0, .EvaluateFloatingPointExpression_label_1204

	// *** Basic block 274

	fld         ft0, -32(s0)
	fsd         ft0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 275

.EvaluateFloatingPointExpression_label_1204:
	j           .EvaluateFloatingPointExpression_label_1222

	// *** Basic block 276

.EvaluateFloatingPointExpression_label_1206:
	mv          s8, s1
	ld          a0, 56(s8)
	mv          a1, s2
	call        EvaluateFloatingPointExpression

	// *** Basic block 277

	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 278

.EvaluateFloatingPointExpression_label_1218:
	mv          a0, x0
	j           .EvaluateFloatingPointExpression_label_71

	// *** Basic block 279

.EvaluateFloatingPointExpression_label_1222:
	mv          a0, x0
	j           .EvaluateFloatingPointExpression_label_71
.func_end_EvaluateFloatingPointExpression:
	.size EvaluateFloatingPointExpression, .func_end_EvaluateFloatingPointExpression-EvaluateFloatingPointExpression

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
