	.file   "assembler.c"
	.text
	.option pic
.PCbegin:
	.local  InitializeDirectives
	.type InitializeDirectives, @function

InitializeDirectives:

	// *** Basic block 0

	.local HandleDirective_globl
	.global MapInsert
	.local HandleDirective_global
	.local HandleDirective_type
	.local HandleDirective_size
	.local HandleDirective_align
	.local HandleDirective_byte
	.local HandleDirective_short
	.local HandleDirective_2byte
	.local HandleDirective_hword
	.local HandleDirective_word
	.local HandleDirective_4byte
	.local HandleDirective_long
	.local HandleDirective_8byte
	.local HandleDirective_space
	.local HandleDirective_p2align
	.local HandleDirective_ascii
	.local HandleDirective_asciz
	.local HandleDirective_string
	.local HandleDirective_set
	.local HandleDirective_section
	.local HandleDirective_text
	.local HandleDirective_data
	.local HandleDirective_comm
	.local HandleDirective_local
	.local HandleDirective_file
	.local HandleDirective_loc
	.local HandleDirective_option
	addi sp, sp, -480
	// Saved return address (offset 472) and frame pointer (offset 464)
	sd ra, 472(sp)
	sd s0, 464(sp)
	addi s0, sp, 480
	// Local vars at offset -464(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0

	// *** Basic block 1

.InitializeDirectives_label_62:
	lla         t0, .str.1
	sd          t0, -464(s0)
	addi        t0, s0, -464
	la          t1, HandleDirective_globl
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -464(s0)
	sd          t0, 0(sp)
	ld          t0, -456(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 2

	addi        sp, sp, 16

	// *** Basic block 3

.InitializeDirectives_label_87:

	// *** Basic block 4

.InitializeDirectives_label_88:

	// *** Basic block 5

.InitializeDirectives_label_89:
	lla         t0, .str.2
	sd          t0, -448(s0)
	addi        t0, s0, -448
	la          t1, HandleDirective_global
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -448(s0)
	sd          t0, 0(sp)
	ld          t0, -440(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 6

	addi        sp, sp, 16

	// *** Basic block 7

.InitializeDirectives_label_109:

	// *** Basic block 8

.InitializeDirectives_label_110:

	// *** Basic block 9

.InitializeDirectives_label_111:
	lla         t0, .str.3
	sd          t0, -432(s0)
	addi        t0, s0, -432
	la          t1, HandleDirective_type
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -432(s0)
	sd          t0, 0(sp)
	ld          t0, -424(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 10

	addi        sp, sp, 16

	// *** Basic block 11

.InitializeDirectives_label_131:

	// *** Basic block 12

.InitializeDirectives_label_132:

	// *** Basic block 13

.InitializeDirectives_label_133:
	lla         t0, .str.4
	sd          t0, -416(s0)
	addi        t0, s0, -416
	la          t1, HandleDirective_size
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -416(s0)
	sd          t0, 0(sp)
	ld          t0, -408(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 14

	addi        sp, sp, 16

	// *** Basic block 15

.InitializeDirectives_label_153:

	// *** Basic block 16

.InitializeDirectives_label_154:

	// *** Basic block 17

.InitializeDirectives_label_155:
	lla         t0, .str.5
	sd          t0, -400(s0)
	addi        t0, s0, -400
	la          t1, HandleDirective_align
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -400(s0)
	sd          t0, 0(sp)
	ld          t0, -392(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 18

	addi        sp, sp, 16

	// *** Basic block 19

.InitializeDirectives_label_175:

	// *** Basic block 20

.InitializeDirectives_label_176:

	// *** Basic block 21

.InitializeDirectives_label_177:
	lla         t0, .str.6
	sd          t0, -384(s0)
	addi        t0, s0, -384
	la          t1, HandleDirective_byte
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -384(s0)
	sd          t0, 0(sp)
	ld          t0, -376(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 22

	addi        sp, sp, 16

	// *** Basic block 23

.InitializeDirectives_label_197:

	// *** Basic block 24

.InitializeDirectives_label_198:

	// *** Basic block 25

.InitializeDirectives_label_199:
	lla         t0, .str.7
	sd          t0, -368(s0)
	addi        t0, s0, -368
	la          t1, HandleDirective_short
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -368(s0)
	sd          t0, 0(sp)
	ld          t0, -360(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 26

	addi        sp, sp, 16

	// *** Basic block 27

.InitializeDirectives_label_219:

	// *** Basic block 28

.InitializeDirectives_label_220:

	// *** Basic block 29

.InitializeDirectives_label_221:
	lla         t0, .str.8
	sd          t0, -352(s0)
	addi        t0, s0, -352
	la          t1, HandleDirective_2byte
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -352(s0)
	sd          t0, 0(sp)
	ld          t0, -344(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 30

	addi        sp, sp, 16

	// *** Basic block 31

.InitializeDirectives_label_241:

	// *** Basic block 32

.InitializeDirectives_label_242:

	// *** Basic block 33

.InitializeDirectives_label_243:
	lla         t0, .str.9
	sd          t0, -336(s0)
	addi        t0, s0, -336
	la          t1, HandleDirective_hword
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -336(s0)
	sd          t0, 0(sp)
	ld          t0, -328(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 34

	addi        sp, sp, 16

	// *** Basic block 35

.InitializeDirectives_label_263:

	// *** Basic block 36

.InitializeDirectives_label_264:

	// *** Basic block 37

.InitializeDirectives_label_265:
	lla         t0, .str.10
	sd          t0, -320(s0)
	addi        t0, s0, -320
	la          t1, HandleDirective_word
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -320(s0)
	sd          t0, 0(sp)
	ld          t0, -312(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 38

	addi        sp, sp, 16

	// *** Basic block 39

.InitializeDirectives_label_285:

	// *** Basic block 40

.InitializeDirectives_label_286:

	// *** Basic block 41

.InitializeDirectives_label_287:
	lla         t0, .str.11
	sd          t0, -304(s0)
	addi        t0, s0, -304
	la          t1, HandleDirective_4byte
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -304(s0)
	sd          t0, 0(sp)
	ld          t0, -296(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 42

	addi        sp, sp, 16

	// *** Basic block 43

.InitializeDirectives_label_307:

	// *** Basic block 44

.InitializeDirectives_label_308:

	// *** Basic block 45

.InitializeDirectives_label_309:
	lla         t0, .str.12
	sd          t0, -288(s0)
	addi        t0, s0, -288
	la          t1, HandleDirective_long
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -288(s0)
	sd          t0, 0(sp)
	ld          t0, -280(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 46

	addi        sp, sp, 16

	// *** Basic block 47

.InitializeDirectives_label_329:

	// *** Basic block 48

.InitializeDirectives_label_330:

	// *** Basic block 49

.InitializeDirectives_label_331:
	lla         t0, .str.13
	sd          t0, -272(s0)
	addi        t0, s0, -272
	la          t1, HandleDirective_8byte
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -272(s0)
	sd          t0, 0(sp)
	ld          t0, -264(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 50

	addi        sp, sp, 16

	// *** Basic block 51

.InitializeDirectives_label_351:

	// *** Basic block 52

.InitializeDirectives_label_352:

	// *** Basic block 53

.InitializeDirectives_label_353:
	lla         t0, .str.14
	sd          t0, -256(s0)
	addi        t0, s0, -256
	la          t1, HandleDirective_space
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -256(s0)
	sd          t0, 0(sp)
	ld          t0, -248(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 54

	addi        sp, sp, 16

	// *** Basic block 55

.InitializeDirectives_label_373:

	// *** Basic block 56

.InitializeDirectives_label_374:

	// *** Basic block 57

.InitializeDirectives_label_375:
	lla         t0, .str.15
	sd          t0, -240(s0)
	addi        t0, s0, -240
	la          t1, HandleDirective_p2align
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -240(s0)
	sd          t0, 0(sp)
	ld          t0, -232(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 58

	addi        sp, sp, 16

	// *** Basic block 59

.InitializeDirectives_label_395:

	// *** Basic block 60

.InitializeDirectives_label_396:

	// *** Basic block 61

.InitializeDirectives_label_397:
	lla         t0, .str.16
	sd          t0, -224(s0)
	addi        t0, s0, -224
	la          t1, HandleDirective_ascii
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -224(s0)
	sd          t0, 0(sp)
	ld          t0, -216(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 62

	addi        sp, sp, 16

	// *** Basic block 63

.InitializeDirectives_label_417:

	// *** Basic block 64

.InitializeDirectives_label_418:

	// *** Basic block 65

.InitializeDirectives_label_419:
	lla         t0, .str.17
	sd          t0, -208(s0)
	addi        t0, s0, -208
	la          t1, HandleDirective_asciz
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -208(s0)
	sd          t0, 0(sp)
	ld          t0, -200(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 66

	addi        sp, sp, 16

	// *** Basic block 67

.InitializeDirectives_label_439:

	// *** Basic block 68

.InitializeDirectives_label_440:

	// *** Basic block 69

.InitializeDirectives_label_441:
	lla         t0, .str.18
	sd          t0, -192(s0)
	addi        t0, s0, -192
	la          t1, HandleDirective_string
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -192(s0)
	sd          t0, 0(sp)
	ld          t0, -184(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 70

	addi        sp, sp, 16

	// *** Basic block 71

.InitializeDirectives_label_461:

	// *** Basic block 72

.InitializeDirectives_label_462:

	// *** Basic block 73

.InitializeDirectives_label_463:
	lla         t0, .str.19
	sd          t0, -176(s0)
	addi        t0, s0, -176
	la          t1, HandleDirective_set
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -176(s0)
	sd          t0, 0(sp)
	ld          t0, -168(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 74

	addi        sp, sp, 16

	// *** Basic block 75

.InitializeDirectives_label_483:

	// *** Basic block 76

.InitializeDirectives_label_484:

	// *** Basic block 77

.InitializeDirectives_label_485:
	lla         t0, .str.20
	sd          t0, -160(s0)
	addi        t0, s0, -160
	la          t1, HandleDirective_section
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -160(s0)
	sd          t0, 0(sp)
	ld          t0, -152(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 78

	addi        sp, sp, 16

	// *** Basic block 79

.InitializeDirectives_label_505:

	// *** Basic block 80

.InitializeDirectives_label_506:

	// *** Basic block 81

.InitializeDirectives_label_507:
	lla         t0, .str.21
	sd          t0, -144(s0)
	addi        t0, s0, -144
	la          t1, HandleDirective_text
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -144(s0)
	sd          t0, 0(sp)
	ld          t0, -136(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 82

	addi        sp, sp, 16

	// *** Basic block 83

.InitializeDirectives_label_527:

	// *** Basic block 84

.InitializeDirectives_label_528:

	// *** Basic block 85

.InitializeDirectives_label_529:
	lla         t0, .str.22
	sd          t0, -128(s0)
	addi        t0, s0, -128
	la          t1, HandleDirective_data
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -128(s0)
	sd          t0, 0(sp)
	ld          t0, -120(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 86

	addi        sp, sp, 16

	// *** Basic block 87

.InitializeDirectives_label_549:

	// *** Basic block 88

.InitializeDirectives_label_550:

	// *** Basic block 89

.InitializeDirectives_label_551:
	lla         t0, .str.23
	sd          t0, -112(s0)
	addi        t0, s0, -112
	la          t1, HandleDirective_comm
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -112(s0)
	sd          t0, 0(sp)
	ld          t0, -104(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 90

	addi        sp, sp, 16

	// *** Basic block 91

.InitializeDirectives_label_571:

	// *** Basic block 92

.InitializeDirectives_label_572:

	// *** Basic block 93

.InitializeDirectives_label_573:
	lla         t0, .str.24
	sd          t0, -96(s0)
	addi        t0, s0, -96
	la          t1, HandleDirective_local
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -96(s0)
	sd          t0, 0(sp)
	ld          t0, -88(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 94

	addi        sp, sp, 16

	// *** Basic block 95

.InitializeDirectives_label_593:

	// *** Basic block 96

.InitializeDirectives_label_594:

	// *** Basic block 97

.InitializeDirectives_label_595:
	lla         t0, .str.25
	sd          t0, -80(s0)
	addi        t0, s0, -80
	la          t1, HandleDirective_file
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -80(s0)
	sd          t0, 0(sp)
	ld          t0, -72(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 98

	addi        sp, sp, 16

	// *** Basic block 99

.InitializeDirectives_label_615:

	// *** Basic block 100

.InitializeDirectives_label_616:

	// *** Basic block 101

.InitializeDirectives_label_617:
	lla         t0, .str.26
	sd          t0, -64(s0)
	addi        t0, s0, -64
	la          t1, HandleDirective_loc
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -64(s0)
	sd          t0, 0(sp)
	ld          t0, -56(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 102

	addi        sp, sp, 16

	// *** Basic block 103

.InitializeDirectives_label_637:

	// *** Basic block 104

.InitializeDirectives_label_638:

	// *** Basic block 105

.InitializeDirectives_label_639:
	lla         t0, .str.27
	sd          t0, -48(s0)
	addi        t0, s0, -48
	la          t1, HandleDirective_option
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -48(s0)
	sd          t0, 0(sp)
	ld          t0, -40(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 106

	addi        sp, sp, 16

	// *** Basic block 107

.InitializeDirectives_label_659:

	// *** Basic block 108

.InitializeDirectives_label_660:

	// *** Basic block 109

.InitializeDirectives_label_661:
	lla         t0, .str.28
	sd          t0, -32(s0)
	addi        t0, s0, -32
	la          t1, HandleDirective_set
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -32(s0)
	sd          t0, 0(sp)
	ld          t0, -24(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 111

.InitializeDirectives_label_681:

	// *** Basic block 112

.InitializeDirectives_label_682:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InitializeDirectives:
	.size InitializeDirectives, .func_end_InitializeDirectives-InitializeDirectives

	.global NewAssemblerSection
	.type NewAssemblerSection, @function

NewAssemblerSection:

	// *** Basic block 0

	.global malloc
	.global ELFWriterSectionContentsInit
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
	mv          s3, a2
	mv          s4, a3
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	sd          s1, 0(s5)
	addi        s6, s5, 8
	li          t0, 8		// 0x8 ASCII \x8
	bne         s2, t0, .NewAssemblerSection_label_41

	// *** Basic block 2

	li          s7, 3		// 0x3 ASCII \x3
	j           .NewAssemblerSection_label_43

	// *** Basic block 3

.NewAssemblerSection_label_41:
	mv          s7, x0

	// *** Basic block 4

.NewAssemblerSection_label_43:
	mv          a1, s7
	mv          a0, s6
	call        ELFWriterSectionContentsInit

	// *** Basic block 5

	sd          x0, 48(s5)
	sw          s3, 56(s5)
	sw          s2, 60(s5)
	sw          s4, 64(s5)
	mv          a0, s5

	// *** Basic block 6

.NewAssemblerSection_label_59:
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
.func_end_NewAssemblerSection:
	.size NewAssemblerSection, .func_end_NewAssemblerSection-NewAssemblerSection

	.global AssemblerSectionDestruct
	.type AssemblerSectionDestruct, @function

AssemblerSectionDestruct:

	// *** Basic block 0

	.global ELFWriterSectionContentsDestruct
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 8
	j           ELFWriterSectionContentsDestruct
.func_end_AssemblerSectionDestruct:
	.size AssemblerSectionDestruct, .func_end_AssemblerSectionDestruct-AssemblerSectionDestruct

	.global AssemblerSectionDelete
	.type AssemblerSectionDelete, @function

AssemblerSectionDelete:

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
	.global AssemblerSectionDestruct
	.global free
	mv          s1, a0
	call        AssemblerSectionDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_AssemblerSectionDelete:
	.size AssemblerSectionDelete, .func_end_AssemblerSectionDelete-AssemblerSectionDelete

	.global AssemblerSectionAlign
	.type AssemblerSectionAlign, @function

AssemblerSectionAlign:

	// *** Basic block 0

	.global BufferAlignLength
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        t1, t0, 8
	addi        a0, t1, 8
	j           BufferAlignLength
.func_end_AssemblerSectionAlign:
	.size AssemblerSectionAlign, .func_end_AssemblerSectionAlign-AssemblerSectionAlign

	.global NewAssemblerRelocation
	.type NewAssemblerRelocation, @function

NewAssemblerRelocation:

	// *** Basic block 0

	.global malloc
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
	mv          s3, a2
	mv          s4, a3
	mv          s5, a4
	li          a0, 24		// 0x18 ASCII \x18
	call        malloc

	// *** Basic block 1

	mv          s6, a0
	sd          s1, 0(s6)
	sw          s2, 8(s6)
	sw          s3, 12(s6)
	sw          s4, 16(s6)
	sw          s5, 20(s6)
	mv          a0, s6

	// *** Basic block 2

.NewAssemblerRelocation_label_41:
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
.func_end_NewAssemblerRelocation:
	.size NewAssemblerRelocation, .func_end_NewAssemblerRelocation-NewAssemblerRelocation

	.global AssemblerRelocationDestruct
	.type AssemblerRelocationDestruct, @function

AssemblerRelocationDestruct:

	// *** Basic block 0

	ret         
.func_end_AssemblerRelocationDestruct:
	.size AssemblerRelocationDestruct, .func_end_AssemblerRelocationDestruct-AssemblerRelocationDestruct

	.global AssemblerRelocationDelete
	.type AssemblerRelocationDelete, @function

AssemblerRelocationDelete:

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
	.global AssemblerRelocationDestruct
	.global free
	mv          s1, a0
	call        AssemblerRelocationDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_AssemblerRelocationDelete:
	.size AssemblerRelocationDelete, .func_end_AssemblerRelocationDelete-AssemblerRelocationDelete

	.global NewAssemblerSymbol
	.type NewAssemblerSymbol, @function

NewAssemblerSymbol:

	// *** Basic block 0

	.global malloc
	.global StringInit
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
	mv          s2, a2
	mv          s3, a3
	mv          s4, a4
	mv          s5, a1
	li          a0, 80		// 0x50 ASCII 'P'
	call        malloc

	// *** Basic block 1

	mv          s6, a0
	mv          a1, s1
	mv          a0, s6
	call        StringInit

	// *** Basic block 2

	sw          s2, 40(s6)
	sw          s3, 44(s6)
	sd          s4, 48(s6)
	sw          x0, 56(s6)
	sb          x0, 60(s6)
	sw          s5, 64(s6)
	li          t0, -1		// 0xffffffffffffffff
	sw          t0, 68(s6)
	sb          x0, 72(s6)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 76(s6)
	sb          x0, 73(s6)
	mv          a0, s6

	// *** Basic block 3

.NewAssemblerSymbol_label_68:
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
.func_end_NewAssemblerSymbol:
	.size NewAssemblerSymbol, .func_end_NewAssemblerSymbol-NewAssemblerSymbol

	.global AssemblerSymbolDelete
	.type AssemblerSymbolDelete, @function

AssemblerSymbolDelete:

	// *** Basic block 0

	.global StringDestruct
	// Leaf procedure, no stack frame generated
	j           StringDestruct
.func_end_AssemblerSymbolDelete:
	.size AssemblerSymbolDelete, .func_end_AssemblerSymbolDelete-AssemblerSymbolDelete

	.local  HashSymbol
	.type HashSymbol, @function

HashSymbol:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	slli        t1, a2, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 1

	j           .HashSymbol_label_28

	// *** Basic block 2

	j           .HashSymbol_label_23

	// *** Basic block 3

.HashSymbol_label_23:
	ld          t1, 16(t0)
	j           .HashSymbol_label_31

	// *** Basic block 4

.HashSymbol_label_28:
	mv          t1, t0
	j           .HashSymbol_label_31

	// *** Basic block 5

.HashSymbol_label_31:
	li          t2, 5381		// 0x1505
	lb          t3, 0(t1)
	beqz        t3, .HashSymbol_label_48

	// *** Basic block 6

.HashSymbol_label_37:
	slli        t3, t2, 5
	add         t3, t3, t2
	mv          t4, t1
	addi        t1, t1, 1
	lb          t5, 0(t4)
	add         t2, t3, t5
	lb          t3, 0(t1)
	bnez        t3, .HashSymbol_label_37

	// *** Basic block 7

.HashSymbol_label_48:
	mv          a0, t2

	// *** Basic block 8

.HashSymbol_label_51:
	ret         
.func_end_HashSymbol:
	.size HashSymbol, .func_end_HashSymbol-HashSymbol

	.local  InsertSymbolInHashTable
	.type InsertSymbolInHashTable, @function

InsertSymbolInHashTable:

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
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a2
	mv          s3, a1
	bne         s1, x0, .InsertSymbolInHashTable_label_22

	// *** Basic block 1

	call        NewVector

	// *** Basic block 2

	mv          s1, a0
	sd          s1, 0(s2)

	// *** Basic block 3

.InsertSymbolInHashTable_label_22:
	mv          s4, s1
	mv          a1, s3
	mv          a0, s4
	call        VectorAppend

	// *** Basic block 4

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 5

.InsertSymbolInHashTable_label_32:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InsertSymbolInHashTable:
	.size InsertSymbolInHashTable, .func_end_InsertSymbolInHashTable-InsertSymbolInHashTable

	.local  FindSymbolInHashTable
	.type FindSymbolInHashTable, @function

FindSymbolInHashTable:

	// *** Basic block 0

	.global StringEqual
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
	mv          t0, a0
	mv          s1, a1
	bne         t0, x0, .FindSymbolInHashTable_label_22

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.FindSymbolInHashTable_label_19:
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

.FindSymbolInHashTable_label_22:
	mv          t1, t0
	mv          s2, x0
	ld          s3, 8(t1)
	bge         x0, s3, .FindSymbolInHashTable_label_52

	// *** Basic block 4

	ld          s4, 0(t1)

	// *** Basic block 5

.FindSymbolInHashTable_label_32:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	mv          a1, s1
	mv          a0, s4
	call        StringEqual

	// *** Basic block 6

	beqz        a0, .FindSymbolInHashTable_label_47

	// *** Basic block 7

	mv          a0, s4
	j           .FindSymbolInHashTable_label_19

	// *** Basic block 8

.FindSymbolInHashTable_label_47:

	// *** Basic block 9

.FindSymbolInHashTable_label_48:
	addi        s2, s2, 1
	bge         s2, s3, .FindSymbolInHashTable_label_32

	// *** Basic block 10

.FindSymbolInHashTable_label_52:
	mv          a0, x0
	j           .FindSymbolInHashTable_label_19
.func_end_FindSymbolInHashTable:
	.size FindSymbolInHashTable, .func_end_FindSymbolInHashTable-FindSymbolInHashTable

	.local  DeleteSymbolList
	.type DeleteSymbolList, @function

DeleteSymbolList:

	// *** Basic block 0

	.global AssemblerSymbolDelete
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
	mv          s2, x0
	ld          s3, 8(s1)
	bge         x0, s3, .DeleteSymbolList_label_34

	// *** Basic block 1

	ld          s4, 0(s1)

	// *** Basic block 2

.DeleteSymbolList_label_21:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	mv          a0, s4
	call        AssemblerSymbolDelete

	// *** Basic block 3

.DeleteSymbolList_label_30:
	addi        s2, s2, 1
	bge         s2, s3, .DeleteSymbolList_label_21

	// *** Basic block 4

.DeleteSymbolList_label_34:
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
.func_end_DeleteSymbolList:
	.size DeleteSymbolList, .func_end_DeleteSymbolList-DeleteSymbolList

	.local  ClearAssemblerSymbolTable
	.type ClearAssemblerSymbolTable, @function

ClearAssemblerSymbolTable:

	// *** Basic block 0

	.global HashTableTraverse
	.local DeleteSymbolList
	.global HashTableClear
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
	la          a1, DeleteSymbolList
	call        HashTableTraverse

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           HashTableClear
.func_end_ClearAssemblerSymbolTable:
	.size ClearAssemblerSymbolTable, .func_end_ClearAssemblerSymbolTable-ClearAssemblerSymbolTable

	.global AssemblerFindSymbol
	.type AssemblerFindSymbol, @function

AssemblerFindSymbol:

	// *** Basic block 0

	.global HashTableSearch
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 648
	j           HashTableSearch
.func_end_AssemblerFindSymbol:
	.size AssemblerFindSymbol, .func_end_AssemblerFindSymbol-AssemblerFindSymbol

	.global AssemblerExtractSymbolSuffix
	.type AssemblerExtractSymbolSuffix, @function

AssemblerExtractSymbolSuffix:

	// *** Basic block 0

	.global StringIndexOf
	.global StringSetString
	.global StringAppendSegment
	.global StringSet
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
	lla         a1, .str.29
	call        StringIndexOf

	// *** Basic block 1

	mv          s4, a0
	li          t0, -1		// 0xffffffffffffffff
	bne         s4, t0, .AssemblerExtractSymbolSuffix_label_40

	// *** Basic block 2

	mv          a1, s1
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
	j           StringSetString

	// *** Basic block 3

.AssemblerExtractSymbolSuffix_label_37:
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

.AssemblerExtractSymbolSuffix_label_40:
	ld          s5, 16(s1)
	mv          a2, s4
	mv          a1, s5
	mv          a0, s2
	call        StringAppendSegment

	// *** Basic block 5

	add         t0, s5, s4
	addi        a1, t0, 1
	mv          a0, s3
	call        StringSet

	// *** Basic block 6

	j           .AssemblerExtractSymbolSuffix_label_37
.func_end_AssemblerExtractSymbolSuffix:
	.size AssemblerExtractSymbolSuffix, .func_end_AssemblerExtractSymbolSuffix-AssemblerExtractSymbolSuffix

	.global AssemblerInsertSymbol
	.type AssemblerInsertSymbol, @function

AssemblerInsertSymbol:

	// *** Basic block 0

	.global HashTableInsert
	.global NewTypeRecord
	.global NewSymbol
	.global SyntaxAddSymbol
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
	lw          t0, 736(s1)
	li          s3, 2		// 0x2 ASCII \x2
	bne         t0, s3, .AssemblerInsertSymbol_label_31

	// *** Basic block 1

.AssemblerInsertSymbol_label_28:
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

.AssemblerInsertSymbol_label_31:
	addi        a0, s1, 648
	mv          a1, s2
	call        HashTableInsert

	// *** Basic block 3

	mv          a1, x0
	mv          a0, s3
	call        NewTypeRecord

	// *** Basic block 4

	mv          s3, a0
	ld          a0, 16(s2)
	li          t0, 32		// 0x20 ASCII ' '
	mv          a2, t0
	mv          a1, s3
	call        NewSymbol

	// *** Basic block 5

	mv          s4, a0
	sd          s2, 112(s4)
	addi        a0, s1, 360
	mv          a1, s4
	call        SyntaxAddSymbol

	// *** Basic block 6

	j           .AssemblerInsertSymbol_label_28
.func_end_AssemblerInsertSymbol:
	.size AssemblerInsertSymbol, .func_end_AssemblerInsertSymbol-AssemblerInsertSymbol

	.global AssemblerEmitWord
	.type AssemblerEmitWord, @function

AssemblerEmitWord:

	// *** Basic block 0

	.global BufferAppend
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a2, -24(s0)
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	ld          t0, 600(a0)
	slli        t1, a1, 3
	add         t0, t0, t1
	ld          s1, 0(t0)
	lw          t0, 736(a0)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .AssemblerEmitWord_label_49

	// *** Basic block 1

	addi        t0, s1, 8
	addi        a0, t0, 8
	addi        a1, s0, -24
	li          t0, 4		// 0x4 ASCII \x4
	mv          a2, t0
	call        BufferAppend

	// *** Basic block 2

	addi        t0, s1, 8
	ld          t1, 32(t0)
	addi        t1, t1, 4
	sd          t1, 32(t0)

	// *** Basic block 3

.AssemblerEmitWord_label_49:
	ld          t0, 48(s1)
	addi        t0, t0, 4
	sd          t0, 48(s1)
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AssemblerEmitWord:
	.size AssemblerEmitWord, .func_end_AssemblerEmitWord-AssemblerEmitWord

	.global AssemblerEmitByte
	.type AssemblerEmitByte, @function

AssemblerEmitByte:

	// *** Basic block 0

	.global BufferAppend
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a2, -24(s0)
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	ld          t0, 600(a0)
	slli        t1, a1, 3
	add         t0, t0, t1
	ld          s1, 0(t0)
	lw          t0, 736(a0)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .AssemblerEmitByte_label_49

	// *** Basic block 1

	addi        t0, s1, 8
	addi        a0, t0, 8
	addi        a1, s0, -24
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	call        BufferAppend

	// *** Basic block 2

	addi        t0, s1, 8
	ld          t1, 32(t0)
	addi        t1, t1, 1
	sd          t1, 32(t0)

	// *** Basic block 3

.AssemblerEmitByte_label_49:
	ld          t0, 48(s1)
	addi        t0, t0, 1
	sd          t0, 48(s1)
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AssemblerEmitByte:
	.size AssemblerEmitByte, .func_end_AssemblerEmitByte-AssemblerEmitByte

	.global AssemblerEmitHalf
	.type AssemblerEmitHalf, @function

AssemblerEmitHalf:

	// *** Basic block 0

	.global BufferAppend
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a2, -24(s0)
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	ld          t0, 600(a0)
	slli        t1, a1, 3
	add         t0, t0, t1
	ld          s1, 0(t0)
	lw          t0, 736(a0)
	li          s2, 2		// 0x2 ASCII \x2
	bne         t0, s2, .AssemblerEmitHalf_label_49

	// *** Basic block 1

	addi        t0, s1, 8
	addi        a0, t0, 8
	addi        a1, s0, -24
	mv          a2, s2
	call        BufferAppend

	// *** Basic block 2

	addi        t0, s1, 8
	ld          t1, 32(t0)
	addi        t1, t1, 2
	sd          t1, 32(t0)

	// *** Basic block 3

.AssemblerEmitHalf_label_49:
	ld          t0, 48(s1)
	addi        t0, t0, 2
	sd          t0, 48(s1)
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AssemblerEmitHalf:
	.size AssemblerEmitHalf, .func_end_AssemblerEmitHalf-AssemblerEmitHalf

	.global AssemblerEmitLong
	.type AssemblerEmitLong, @function

AssemblerEmitLong:

	// *** Basic block 0

	.global BufferAppend
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a2, -24(s0)
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	ld          t0, 600(a0)
	slli        t1, a1, 3
	add         t0, t0, t1
	ld          s1, 0(t0)
	lw          t0, 736(a0)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .AssemblerEmitLong_label_49

	// *** Basic block 1

	addi        t0, s1, 8
	addi        a0, t0, 8
	addi        a1, s0, -24
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	call        BufferAppend

	// *** Basic block 2

	addi        t0, s1, 8
	ld          t1, 32(t0)
	addi        t1, t1, 8
	sd          t1, 32(t0)

	// *** Basic block 3

.AssemblerEmitLong_label_49:
	ld          t0, 48(s1)
	addi        t0, t0, 8
	sd          t0, 48(s1)
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AssemblerEmitLong:
	.size AssemblerEmitLong, .func_end_AssemblerEmitLong-AssemblerEmitLong

	.global AssemblerEvaluateExpression
	.type AssemblerEvaluateExpression, @function

AssemblerEvaluateExpression:

	// *** Basic block 0

	.global SyntaxParseSingleExpression
	.global AnalyzeExpression
	.global EvaluateIntegerExpression
	.global ASTNodeDelete
	.global AssemblerError
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
	addi        a0, s1, 360
	mv          a1, x0
	call        SyntaxParseSingleExpression

	// *** Basic block 1

	mv          s2, a0
	bne         s2, x0, .AssemblerEvaluateExpression_label_33

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.AssemblerEvaluateExpression_label_30:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.AssemblerEvaluateExpression_label_33:
	mv          a0, s2
	call        AnalyzeExpression

	// *** Basic block 5

	mv          s2, a0
	addi        a1, s0, -32
	mv          a0, s2
	call        EvaluateIntegerExpression

	// *** Basic block 6

	beqz        a0, .AssemblerEvaluateExpression_label_52

	// *** Basic block 7

	mv          a0, s2
	call        ASTNodeDelete

	// *** Basic block 8

	ld          a0, -32(s0)
	j           .AssemblerEvaluateExpression_label_30

	// *** Basic block 9

.AssemblerEvaluateExpression_label_52:
	lw          t0, 736(s1)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .AssemblerEvaluateExpression_label_66

	// *** Basic block 10

	lla         a1, .str.30
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 11

.AssemblerEvaluateExpression_label_66:
	mv          a0, s2
	call        ASTNodeDelete

	// *** Basic block 12

	mv          a0, x0
	j           .AssemblerEvaluateExpression_label_30
.func_end_AssemblerEvaluateExpression:
	.size AssemblerEvaluateExpression, .func_end_AssemblerEvaluateExpression-AssemblerEvaluateExpression

	.global AssemblerGetDoubleConst
	.type AssemblerGetDoubleConst, @function

AssemblerGetDoubleConst:

	// *** Basic block 0

	.global LexLookingAt
	.global LexNextToken
	.global AssemblerError
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// Saved floating point registers.
	fsd fs0, 0(sp)
	// End of stack frame
	mv          s1, a0
	fmv.d.x     fs0, x0
	addi        a0, s1, 176
	li          a1, 8		// 0x8 ASCII \x8
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .AssemblerGetDoubleConst_label_31

	// *** Basic block 2

	fld         fs0, 128(a0)
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 3

	j           .AssemblerGetDoubleConst_label_54

	// *** Basic block 4

.AssemblerGetDoubleConst_label_31:
	addi        a0, s1, 176
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 5

	beqz        a0, .AssemblerGetDoubleConst_label_46

	// *** Basic block 6

	ld          t0, 120(a0)
	fcvt.d.l    fs0, t0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 7

	j           .AssemblerGetDoubleConst_label_53

	// *** Basic block 8

.AssemblerGetDoubleConst_label_46:
	lla         a1, .str.31
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 9

.AssemblerGetDoubleConst_label_53:

	// *** Basic block 10

.AssemblerGetDoubleConst_label_54:
	fmv.d       fa0, fs0

	// *** Basic block 11

.AssemblerGetDoubleConst_label_57:
	// Restored registers.
	fld fs0, 8(sp)
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AssemblerGetDoubleConst:
	.size AssemblerGetDoubleConst, .func_end_AssemblerGetDoubleConst-AssemblerGetDoubleConst

	.global NewLocationEntry
	.type NewLocationEntry, @function

NewLocationEntry:

	// *** Basic block 0

	.global malloc
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
	mv          s4, a3
	li          a0, 24		// 0x18 ASCII \x18
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	sw          s1, 0(s5)
	sw          s2, 4(s5)
	sw          s3, 8(s5)
	sd          s4, 16(s5)
	mv          a0, s5

	// *** Basic block 2

.NewLocationEntry_label_35:
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
.func_end_NewLocationEntry:
	.size NewLocationEntry, .func_end_NewLocationEntry-NewLocationEntry

	.local  CompareCharPointer
	.type CompareCharPointer, @function

CompareCharPointer:

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
.func_end_CompareCharPointer:
	.size CompareCharPointer, .func_end_CompareCharPointer-CompareCharPointer

	.global AssemblerInit
	.type AssemblerInit, @function

AssemblerInit:

	// *** Basic block 0

	.global fopen
	.global fprintf
	.global stderr
	.global PreprocessorInit
	.global LexInitFromFile
	.global SyntaxInit
	.global SyntaxOpenScope
	.global StringInit
	.global MapInit
	.local CompareCharPointer
	.local InitializeDirectives
	.global VectorInit
	.global HashTableInit
	.local HashSymbol
	.local InsertSymbolInHashTable
	.local FindSymbolInHashTable
	.global DwarfInit
	.local DefineLabel
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
	mv          s2, a4
	mv          s3, a1
	mv          s4, a2
	mv          s5, a3
	ld          s6, 16(a5)
	lla         a1, .str.32
	mv          a0, s6
	call        fopen

	// *** Basic block 1

	sd          a0, 560(s1)
	ld          t0, 560(s1)
	bne         t0, x0, .AssemblerInit_label_92

	// *** Basic block 2

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.33
	mv          a2, s6
	call        fprintf

	// *** Basic block 3

	mv          a0, x0

	// *** Basic block 4

.AssemblerInit_label_89:
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

.AssemblerInit_label_92:
	mv          a0, s1
	call        PreprocessorInit

	// *** Basic block 6

	addi        a0, s1, 176
	ld          a1, 16(s2)
	mv          a2, s1
	call        LexInitFromFile

	// *** Basic block 7

	addi        t0, s1, 176
	li          t1, 1		// 0x1 ASCII \x1
	sb          t1, 178(t0)
	addi        a0, s1, 360
	addi        a1, s1, 176
	call        SyntaxInit

	// *** Basic block 8

	addi        a0, s1, 360
	call        SyntaxOpenScope

	// *** Basic block 9

	addi        a0, s1, 520
	lla         a1, .str.34
	call        StringInit

	// *** Basic block 10

	addi        a0, s1, 568
	la          t0, CompareCharPointer
	mv          a1, t0
	call        MapInit

	// *** Basic block 11

	addi        a0, s1, 568
	call        InitializeDirectives

	// *** Basic block 12

	addi        a0, s1, 600
	call        VectorInit

	// *** Basic block 13

	addi        a0, s1, 624
	call        VectorInit

	// *** Basic block 14

	sw          x0, 736(s1)
	sw          x0, 740(s1)
	addi        a0, s1, 648
	lla         a1, .str.35
	la          t0, FindSymbolInHashTable
	mv          a5, t0
	la          t0, InsertSymbolInHashTable
	mv          a4, t0
	la          t0, HashSymbol
	mv          a3, t0
	li          t0, 1009		// 0x3f1
	mv          a2, t0
	call        HashTableInit

	// *** Basic block 15

	addi        t0, s1, 748
	sh          s3, 0(t0)
	addi        t0, s1, 750
	sh          s4, 0(t0)
	sd          s5, 752(s1)
	sb          x0, 760(s1)
	addi        a0, s1, 768
	call        DwarfInit

	// *** Basic block 16

	la          t0, DefineLabel
	sd          t0, 864(s1)
	li          a0, 1		// 0x1 ASCII \x1
	j           .AssemblerInit_label_89
.func_end_AssemblerInit:
	.size AssemblerInit, .func_end_AssemblerInit-AssemblerInit

	.global AssemblerDestruct
	.type AssemblerDestruct, @function

AssemblerDestruct:

	// *** Basic block 0

	.global fclose
	.global SyntaxCloseScope
	.global SyntaxDestruct
	.global LexDestruct
	.global StringDestruct
	.global PreprocessorDestruct
	.global VectorDestructWithContents
	.global AssemblerSectionDestruct
	.global AssemblerRelocationDestruct
	.local ClearAssemblerSymbolTable
	.global HashTableDestruct
	.global DwarfDestruct
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
	ld          a0, 560(s1)
	call        fclose

	// *** Basic block 1

	addi        a0, s1, 360
	call        SyntaxCloseScope

	// *** Basic block 2

	addi        a0, s1, 360
	call        SyntaxDestruct

	// *** Basic block 3

	addi        a0, s1, 176
	call        LexDestruct

	// *** Basic block 4

	addi        a0, s1, 520
	call        StringDestruct

	// *** Basic block 5

	mv          a0, s1
	call        PreprocessorDestruct

	// *** Basic block 6

	addi        a0, s1, 600
	la          t0, AssemblerSectionDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 7

	addi        a0, s1, 624
	la          t0, AssemblerRelocationDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 8

	addi        a0, s1, 648
	call        ClearAssemblerSymbolTable

	// *** Basic block 9

	addi        a0, s1, 648
	call        HashTableDestruct

	// *** Basic block 10

	addi        a0, s1, 768
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DwarfDestruct
.func_end_AssemblerDestruct:
	.size AssemblerDestruct, .func_end_AssemblerDestruct-AssemblerDestruct

	.global AssemblerAddRelocation
	.type AssemblerAddRelocation, @function

AssemblerAddRelocation:

	// *** Basic block 0

	.global AssemblerRelocationDelete
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
	mv          s1, a0
	mv          s2, a1
	lw          t0, 736(s1)
	li          s3, 1		// 0x1 ASCII \x1
	bne         t0, s3, .AssemblerAddRelocation_label_30

	// *** Basic block 1

	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssemblerRelocationDelete

	// *** Basic block 2

.AssemblerAddRelocation_label_27:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.AssemblerAddRelocation_label_30:
	ld          t0, 0(s2)
	sb          s3, 72(t0)
	addi        a0, s1, 624
	mv          a1, s2
	call        VectorAppend

	// *** Basic block 4

	j           .AssemblerAddRelocation_label_27
.func_end_AssemblerAddRelocation:
	.size AssemblerAddRelocation, .func_end_AssemblerAddRelocation-AssemblerAddRelocation

	.global AssemblerAddSection
	.type AssemblerAddSection, @function

AssemblerAddSection:

	// *** Basic block 0

	.global StringEqual
	.global NewAssemblerSection
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
	mv          s2, a3
	mv          s3, a2
	mv          s4, a4
	mv          s5, a0
	beq         s1, x0, .AssemblerAddSection_label_54

	// *** Basic block 1

	lla         a1, .str.36
	mv          a0, s1
	call        StringEqual

	// *** Basic block 2

	beqz        a0, .AssemblerAddSection_label_42

	// *** Basic block 3

	ori         s2, s2, 4
	li          s3, 1		// 0x1 ASCII \x1
	j           .AssemblerAddSection_label_53

	// *** Basic block 4

.AssemblerAddSection_label_42:
	lla         a1, .str.37
	mv          a0, s1
	call        StringEqual

	// *** Basic block 5

	beqz        a0, .AssemblerAddSection_label_52

	// *** Basic block 6

	ori         s2, s2, 1
	li          s3, 1		// 0x1 ASCII \x1

	// *** Basic block 7

.AssemblerAddSection_label_52:

	// *** Basic block 8

.AssemblerAddSection_label_53:

	// *** Basic block 9

.AssemblerAddSection_label_54:
	mv          a3, s4
	mv          a2, s2
	mv          a1, s3
	mv          a0, s1
	call        NewAssemblerSection

	// *** Basic block 10

	mv          s6, a0
	addi        a0, s5, 600
	mv          a1, s6
	call        VectorAppend

	// *** Basic block 11

	ld          t0, 8(a0)
	addi        a0, t0, -1

	// *** Basic block 12

.AssemblerAddSection_label_76:
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
.func_end_AssemblerAddSection:
	.size AssemblerAddSection, .func_end_AssemblerAddSection-AssemblerAddSection

	.global AssemblerFindSection
	.type AssemblerFindSection, @function

AssemblerFindSection:

	// *** Basic block 0

	.global StringEqualString
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
	mv          s1, a1
	mv          s2, x0
	addi        t0, a0, 600
	ld          s3, 8(t0)
	bge         x0, s3, .AssemblerFindSection_label_54

	// *** Basic block 1

	ld          t0, 600(a0)

	// *** Basic block 2

.AssemblerFindSection_label_25:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          t1, 0(t0)
	ld          s5, 0(t1)
	sub         t0, s5, x0
	snez        s4, t0
	beq         s5, x0, .AssemblerFindSection_label_42

	// *** Basic block 3

	mv          a1, s1
	mv          a0, s5
	call        StringEqualString

	// *** Basic block 4

	mv          s4, a0

	// *** Basic block 5

.AssemblerFindSection_label_42:
	beqz        s4, .AssemblerFindSection_label_49

	// *** Basic block 6

	sext.w      a0, s2

	// *** Basic block 7

.AssemblerFindSection_label_46:
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

	// *** Basic block 8

.AssemblerFindSection_label_49:

	// *** Basic block 9

.AssemblerFindSection_label_50:
	addi        s2, s2, 1
	bge         s2, s3, .AssemblerFindSection_label_25

	// *** Basic block 10

.AssemblerFindSection_label_54:
	li          a0, -1		// 0xffffffffffffffff
	j           .AssemblerFindSection_label_46
.func_end_AssemblerFindSection:
	.size AssemblerFindSection, .func_end_AssemblerFindSection-AssemblerFindSection

	.global AssemblerSetSectionSize
	.type AssemblerSetSectionSize, @function

AssemblerSetSectionSize:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a2
	addi        t2, a0, 600
	ld          t2, 8(t2)
	bge         t0, t2, .AssemblerSetSectionSize_label_30

	// *** Basic block 1

	ld          t2, 600(a0)
	slli        t3, t0, 3
	add         t2, t2, t3
	ld          t3, 0(t2)
	addi        t2, t3, 8
	sd          t1, 32(t2)

	// *** Basic block 2

.AssemblerSetSectionSize_label_30:
	ret         
.func_end_AssemblerSetSectionSize:
	.size AssemblerSetSectionSize, .func_end_AssemblerSetSectionSize-AssemblerSetSectionSize

	.global AssemblerCurrentAddress
	.type AssemblerCurrentAddress, @function

AssemblerCurrentAddress:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 600(a0)
	lw          t1, 744(a0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t1, 0(t0)
	ld          a0, 48(t1)

	// *** Basic block 1

.AssemblerCurrentAddress_label_23:
	ret         
.func_end_AssemblerCurrentAddress:
	.size AssemblerCurrentAddress, .func_end_AssemblerCurrentAddress-AssemblerCurrentAddress

	.local  SymbolTypeToELFType
	.type SymbolTypeToELFType, @function

SymbolTypeToELFType:

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
	// End of stack frame
	mv          s1, a0
	blt         s1, x0, .SymbolTypeToELFType_label_56

	// *** Basic block 1

	li          t0, 4		// 0x4 ASCII \x4
	blt         t0, s1, .SymbolTypeToELFType_label_56

	// *** Basic block 2

	slli        t0, s1, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .SymbolTypeToELFType_label_34

	// *** Basic block 4

	j           .SymbolTypeToELFType_label_40

	// *** Basic block 5

	j           .SymbolTypeToELFType_label_44

	// *** Basic block 6

	j           .SymbolTypeToELFType_label_48

	// *** Basic block 7

	j           .SymbolTypeToELFType_label_52

	// *** Basic block 8

.SymbolTypeToELFType_label_34:
	mv          a0, x0

	// *** Basic block 9

.SymbolTypeToELFType_label_37:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 10

.SymbolTypeToELFType_label_40:
	li          a0, 2		// 0x2 ASCII \x2
	j           .SymbolTypeToELFType_label_37

	// *** Basic block 11

.SymbolTypeToELFType_label_44:
	li          a0, 1		// 0x1 ASCII \x1
	j           .SymbolTypeToELFType_label_37

	// *** Basic block 12

.SymbolTypeToELFType_label_48:
	li          a0, 5		// 0x5 ASCII \x5
	j           .SymbolTypeToELFType_label_37

	// *** Basic block 13

.SymbolTypeToELFType_label_52:
	li          a0, 6		// 0x6 ASCII \x6
	j           .SymbolTypeToELFType_label_37

	// *** Basic block 14

.SymbolTypeToELFType_label_56:
	lla         a0, .str.38
	lla         a1, .str.39
	lla         a3, .str.40
	li          t0, 468		// 0x1d4
	mv          a2, t0
	call        printf

	// *** Basic block 15

	call        abort

	// *** Basic block 16

	mv          a0, x0
	j           .SymbolTypeToELFType_label_37
.func_end_SymbolTypeToELFType:
	.size SymbolTypeToELFType, .func_end_SymbolTypeToELFType-SymbolTypeToELFType

	.local  SymbolBindingToELFBinding
	.type SymbolBindingToELFBinding, @function

SymbolBindingToELFBinding:

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
	// End of stack frame
	mv          s1, a0
	blt         s1, x0, .SymbolBindingToELFBinding_label_38

	// *** Basic block 1

	li          t0, 1		// 0x1 ASCII \x1
	blt         t0, s1, .SymbolBindingToELFBinding_label_38

	// *** Basic block 2

	slli        t0, s1, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .SymbolBindingToELFBinding_label_28

	// *** Basic block 4

	j           .SymbolBindingToELFBinding_label_34

	// *** Basic block 5

.SymbolBindingToELFBinding_label_28:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 6

.SymbolBindingToELFBinding_label_31:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.SymbolBindingToELFBinding_label_34:
	mv          a0, x0
	j           .SymbolBindingToELFBinding_label_31

	// *** Basic block 8

.SymbolBindingToELFBinding_label_38:
	lla         a0, .str.41
	lla         a1, .str.42
	lla         a3, .str.43
	li          t0, 480		// 0x1e0
	mv          a2, t0
	call        printf

	// *** Basic block 9

	call        abort

	// *** Basic block 10

	mv          a0, x0
	j           .SymbolBindingToELFBinding_label_31
.func_end_SymbolBindingToELFBinding:
	.size SymbolBindingToELFBinding, .func_end_SymbolBindingToELFBinding-SymbolBindingToELFBinding

	.local  AddLocalSymbolToELFFile
	.type AddLocalSymbolToELFFile, @function

AddLocalSymbolToELFFile:

	// *** Basic block 0

	.global ELFWriterAddSymbol
	.local SymbolTypeToELFType
	.local SymbolBindingToELFBinding
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
	mv          t0, a0
	mv          s1, a1
	mv          s2, x0
	ld          s3, 8(t0)
	bge         x0, s3, .AddLocalSymbolToELFFile_label_92

	// *** Basic block 1

	ld          t1, 0(t0)

	// *** Basic block 2

.AddLocalSymbolToELFFile_label_35:
	slli        t0, s2, 3
	add         t0, t1, t0
	ld          s4, 0(t0)
	lb          t0, 72(s4)
	beqz        t0, .AddLocalSymbolToELFFile_label_50

	// *** Basic block 3

	lw          t1, 44(s4)
	addi        t1, t1, -1
	seqz        t0, t1

	// *** Basic block 4

.AddLocalSymbolToELFFile_label_50:
	beqz        t0, .AddLocalSymbolToELFFile_label_87

	// *** Basic block 5

	lw          s5, 64(s4)
	lw          a0, 40(s4)
	call        SymbolTypeToELFType

	// *** Basic block 6

	lw          a0, 44(s4)
	call        SymbolBindingToELFBinding

	// *** Basic block 7

	lw          a5, 56(s4)
	ld          a6, 48(s4)
	addi        a7, s4, 68
	mv          a4, a0
	mv          a3, a0
	mv          a2, s5
	mv          a1, s4
	mv          a0, s1
	call        ELFWriterAddSymbol

	// *** Basic block 8

.AddLocalSymbolToELFFile_label_87:

	// *** Basic block 9

.AddLocalSymbolToELFFile_label_88:
	addi        s2, s2, 1
	bge         s2, s3, .AddLocalSymbolToELFFile_label_35

	// *** Basic block 10

.AddLocalSymbolToELFFile_label_92:
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
.func_end_AddLocalSymbolToELFFile:
	.size AddLocalSymbolToELFFile, .func_end_AddLocalSymbolToELFFile-AddLocalSymbolToELFFile

	.local  AddGlobalSymbolToELFFile
	.type AddGlobalSymbolToELFFile, @function

AddGlobalSymbolToELFFile:

	// *** Basic block 0

	.global ELFWriterAddSymbol
	.local SymbolTypeToELFType
	.local SymbolBindingToELFBinding
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
	mv          t0, a0
	mv          s1, a1
	mv          s2, x0
	ld          s3, 8(t0)
	bge         x0, s3, .AddGlobalSymbolToELFFile_label_99

	// *** Basic block 1

	ld          t1, 0(t0)

	// *** Basic block 2

.AddGlobalSymbolToELFFile_label_35:
	slli        t0, s2, 3
	add         t0, t1, t0
	ld          s4, 0(t0)
	lb          t0, 72(s4)
	beqz        t0, .AddGlobalSymbolToELFFile_label_48

	// *** Basic block 3

	lw          t1, 44(s4)
	seqz        t0, t1

	// *** Basic block 4

.AddGlobalSymbolToELFFile_label_48:
	beqz        t0, .AddGlobalSymbolToELFFile_label_94

	// *** Basic block 5

	lb          t0, 60(s4)
	beqz        t0, .AddGlobalSymbolToELFFile_label_58

	// *** Basic block 6

	lw          s5, 64(s4)
	j           .AddGlobalSymbolToELFFile_label_60

	// *** Basic block 7

.AddGlobalSymbolToELFFile_label_58:
	mv          s5, x0

	// *** Basic block 8

.AddGlobalSymbolToELFFile_label_60:
	lw          a0, 40(s4)
	call        SymbolTypeToELFType

	// *** Basic block 9

	lw          a0, 44(s4)
	call        SymbolBindingToELFBinding

	// *** Basic block 10

	lw          a5, 56(s4)
	ld          a6, 48(s4)
	addi        a7, s4, 68
	mv          a4, a0
	mv          a3, a0
	mv          a2, s5
	mv          a1, s4
	mv          a0, s1
	call        ELFWriterAddSymbol

	// *** Basic block 11

.AddGlobalSymbolToELFFile_label_94:

	// *** Basic block 12

.AddGlobalSymbolToELFFile_label_95:
	addi        s2, s2, 1
	bge         s2, s3, .AddGlobalSymbolToELFFile_label_35

	// *** Basic block 13

.AddGlobalSymbolToELFFile_label_99:
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
.func_end_AddGlobalSymbolToELFFile:
	.size AddGlobalSymbolToELFFile, .func_end_AddGlobalSymbolToELFFile-AddGlobalSymbolToELFFile

	.local  DefineLabel
	.type DefineLabel, @function

DefineLabel:

	// *** Basic block 0

	.global AssemblerFindSymbol
	.global AssemblerCurrentAddress
	.global AssemblerError
	.global NewAssemblerSymbol
	.global AssemblerInsertSymbol
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
	mv          t0, a1
	ld          s2, 16(t0)
	mv          a1, s2
	call        AssemblerFindSymbol

	// *** Basic block 1

	mv          s3, a0
	beq         s3, x0, .DefineLabel_label_64

	// *** Basic block 2

	lb          t0, 60(s3)
	not         t0, t0
	beqz        t0, .DefineLabel_label_52

	// *** Basic block 3

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 60(s3)
	lw          t0, 744(s1)
	sw          t0, 64(s3)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 4

	sd          a0, 48(s3)
	j           .DefineLabel_label_62

	// *** Basic block 5

.DefineLabel_label_52:
	lla         a1, .str.44
	mv          a2, s2
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 6

.DefineLabel_label_62:
	j           .DefineLabel_label_96

	// *** Basic block 7

.DefineLabel_label_64:
	lw          s4, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 8

	mv          a4, a0
	li          s5, 1		// 0x1 ASCII \x1
	mv          a3, s5
	mv          a2, x0
	mv          a1, s4
	mv          a0, s2
	call        NewAssemblerSymbol

	// *** Basic block 9

	mv          s3, a0
	sb          s5, 73(s3)
	sb          s5, 60(s3)
	mv          a1, s3
	mv          a0, s1
	call        AssemblerInsertSymbol

	// *** Basic block 10

.DefineLabel_label_96:
	mv          a0, s3

	// *** Basic block 11

.DefineLabel_label_99:
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
.func_end_DefineLabel:
	.size DefineLabel, .func_end_DefineLabel-DefineLabel

	.local  Assemble
	.type Assemble, @function

Assemble:

	// *** Basic block 0

	.global LexEof
	.global LexLookingAt
	.global StringClear
	.global StringSetString
	.global LexNextToken
	.global LexMatch
	.global MapFindPointerKey
	.global LexReadLine
	.global StringDestruct
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	addi        a0, s1, 176
	call        LexEof

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .Assemble_label_128

	// *** Basic block 2

.Assemble_label_46:
	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 3

	beqz        a0, .Assemble_label_116

	// *** Basic block 4

	addi        a0, s0, -64
	call        StringClear

	// *** Basic block 5

	addi        a0, s0, -64
	addi        t0, s1, 176
	addi        a1, t0, 80
	call        StringSetString

	// *** Basic block 6

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 7

	addi        a0, s1, 176
	li          t0, 17		// 0x11 ASCII \x11
	mv          a1, t0
	call        LexMatch

	// *** Basic block 8

	beqz        a0, .Assemble_label_89

	// *** Basic block 9

	lw          t0, 736(s1)
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .Assemble_label_87

	// *** Basic block 10

	ld          t0, 864(s1)
	addi        a1, s0, -64
	mv          a0, s1
	jalr         x1, t0, 0

	// *** Basic block 11

.Assemble_label_87:
	j           .Assemble_label_115

	// *** Basic block 12

.Assemble_label_89:
	addi        a0, s1, 568
	addi        t0, s0, -64
	ld          a1, 16(t0)
	call        MapFindPointerKey

	// *** Basic block 13

	mv          s3, a0
	beq         s3, x0, .Assemble_label_108

	// *** Basic block 14

	mv          s4, s3
	mv          a0, s1
	jalr         x1, s4, 0

	// *** Basic block 15

	j           .Assemble_label_114

	// *** Basic block 16

.Assemble_label_108:
	addi        a1, s0, -64
	mv          a0, s1
	jalr         x1, s2, 0

	// *** Basic block 17

.Assemble_label_114:

	// *** Basic block 18

.Assemble_label_115:

	// *** Basic block 19

.Assemble_label_116:
	addi        a0, s1, 176
	call        LexReadLine

	// *** Basic block 20

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 21

	addi        a0, s1, 176
	call        LexEof

	// *** Basic block 22

	not         t0, a0
	bnez        t0, .Assemble_label_46

	// *** Basic block 23

.Assemble_label_128:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 24

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble:
	.size Assemble, .func_end_Assemble-Assemble

	.local  AddSections
	.type AddSections, @function

AddSections:

	// *** Basic block 0

	.global StringEqual
	.global ELFWriterSectionContentsInit
	.global DwarfBuildDebugLineContents
	.global AssemblerSectionAlign
	.global ELFWriterAddSection
	.global AssemblerFindSymbol
	.global DwarfDebugLineRelocation
	.global AssemblerAddRelocation
	.global NewAssemblerSymbol
	.global AssemblerInsertSymbol
	.global ELFWriterAddSectionSymbol
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
	mv          s3, x0
	addi        t0, s1, 600
	ld          t0, 8(t0)
	bge         x0, t0, .AddSections_label_190

	// *** Basic block 1

.AddSections_label_44:
	ld          t0, 600(s1)
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	li          s5, 1		// 0x1 ASCII \x1
	ld          a0, 0(s4)
	lla         a1, .str.45
	call        StringEqual

	// *** Basic block 2

	mv          s6, a0
	beqz        s6, .AddSections_label_78

	// *** Basic block 3

	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 64(s4)
	mv          s5, x0
	addi        a0, s4, 8
	mv          a1, x0
	call        ELFWriterSectionContentsInit

	// *** Basic block 4

	addi        a0, s1, 768
	addi        t0, s4, 8
	addi        a1, t0, 8
	call        DwarfBuildDebugLineContents

	// *** Basic block 5

.AddSections_label_78:
	beqz        s5, .AddSections_label_86

	// *** Basic block 6

	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	mv          a0, s4
	call        AssemblerSectionAlign

	// *** Basic block 7

.AddSections_label_86:
	ld          a1, 0(s4)
	lw          a2, 60(s4)
	lw          a3, 56(s4)
	lw          a4, 64(s4)
	addi        a5, s4, 8
	mv          a6, x0
	mv          a0, s2
	call        ELFWriterAddSection

	// *** Basic block 8

	mv          s7, a0
	beqz        s6, .AddSections_label_145

	// *** Basic block 9

	lla         a1, .str.46
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 10

	mv          s6, a0
	beq         s6, x0, .AddSections_label_144

	// *** Basic block 11

	addi        a0, s1, 768
	ld          t0, 752(s1)
	lw          a2, 8(t0)
	lw          a3, 112(s7)
	mv          a1, s6
	call        DwarfDebugLineRelocation

	// *** Basic block 12

	mv          s8, a0
	mv          a1, s8
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 13

.AddSections_label_144:

	// *** Basic block 14

.AddSections_label_145:
	ld          s4, 0(s4)
	beq         s4, x0, .AddSections_label_174

	// *** Basic block 15

	ld          a0, 16(s4)
	lw          a1, 112(s7)
	mv          a4, x0
	li          s4, 1		// 0x1 ASCII \x1
	mv          a3, s4
	mv          a2, x0
	call        NewAssemblerSymbol

	// *** Basic block 16

	mv          s9, a0
	sb          s4, 72(s9)
	mv          a1, s9
	mv          a0, s1
	call        AssemblerInsertSymbol

	// *** Basic block 17

.AddSections_label_174:
	lw          a2, 112(s7)
	mv          a1, s7
	mv          a0, s2
	call        ELFWriterAddSectionSymbol

	// *** Basic block 18

.AddSections_label_183:
	addi        s3, s3, 1
	addi        t0, s1, 600
	ld          t0, 8(t0)
	bge         s3, t0, .AddSections_label_44

	// *** Basic block 19

.AddSections_label_190:
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
.func_end_AddSections:
	.size AddSections, .func_end_AddSections-AddSections

	.local  AddRelocations
	.type AddRelocations, @function

AddRelocations:

	// *** Basic block 0

	.global ELFWriterAddRelocationWithAddend
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
	addi        t0, a0, 624
	ld          s3, 8(t0)
	bge         x0, s3, .AddRelocations_label_60

	// *** Basic block 1

	ld          s4, 624(a0)

	// *** Basic block 2

.AddRelocations_label_27:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	lw          a1, 12(s4)
	lw          a2, 16(s4)
	ld          t0, 0(s4)
	lw          a3, 68(t0)
	lw          a4, 20(s4)
	lw          a5, 8(s4)
	mv          a0, s1
	call        ELFWriterAddRelocationWithAddend

	// *** Basic block 3

.AddRelocations_label_56:
	addi        s2, s2, 1
	bge         s2, s3, .AddRelocations_label_27

	// *** Basic block 4

.AddRelocations_label_60:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AddRelocations:
	.size AddRelocations, .func_end_AddRelocations-AddRelocations

	.global AssemblerReset
	.type AssemblerReset, @function

AssemblerReset:

	// *** Basic block 0

	.global LexRewind
	.global LexNextToken
	.global PreprocessorReset
	.local ClearAssemblerSymbolTable
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
	addi        t0, s1, 600
	ld          s4, 8(t0)
	bge         x0, s4, .AssemblerReset_label_41

	// *** Basic block 1

	ld          t0, 600(s1)

	// *** Basic block 2

.AssemblerReset_label_29:
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s5, 0(t0)
	sd          x0, 48(s5)

	// *** Basic block 3

.AssemblerReset_label_37:
	addi        s3, s3, 1
	bge         s3, s4, .AssemblerReset_label_29

	// *** Basic block 4

.AssemblerReset_label_41:
	addi        a0, s1, 176
	call        LexRewind

	// *** Basic block 5

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 6

	mv          a0, s1
	call        PreprocessorReset

	// *** Basic block 7

	beqz        s2, .AssemblerReset_label_56

	// *** Basic block 8

	addi        a0, s1, 648
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ClearAssemblerSymbolTable

	// *** Basic block 9

.AssemblerReset_label_56:
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
.func_end_AssemblerReset:
	.size AssemblerReset, .func_end_AssemblerReset-AssemblerReset

	.global AssemblerRun
	.type AssemblerRun, @function

AssemblerRun:

	// *** Basic block 0

	.global LexNextToken
	.local Assemble
	.global AssemblerReset
	.global ELFWriterFileInit
	.global ELFWriterAddFileSymbol
	.local AddSections
	.global HashTableTraverse
	.local AddLocalSymbolToELFFile
	.local AddGlobalSymbolToELFFile
	.local AddRelocations
	.global ELFWriterFileWrite
	.global ELFWriterFileDestruct
	addi sp, sp, -320
	// Saved return address (offset 312) and frame pointer (offset 304)
	sd ra, 312(sp)
	sd s0, 304(sp)
	addi s0, sp, 320
	// Local vars at offset -288(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	li          s3, 1		// 0x1 ASCII \x1
	sw          s3, 736(s1)
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 1

	lw          t1, 740(s1)
	seqz        t0, t1
	bnez        t1, .AssemblerRun_label_52

	// *** Basic block 2

	lw          t1, 736(s1)
	slti        t0, t1, 3

	// *** Basic block 3

.AssemblerRun_label_52:
	beqz        t0, .AssemblerRun_label_77

	// *** Basic block 4

.AssemblerRun_label_54:
	mv          a1, s2
	mv          a0, s1
	call        Assemble

	// *** Basic block 5

	lw          s4, 736(s1)
	addi        t0, s4, 1
	sw          t0, 736(s1)
	mv          a1, x0
	mv          a0, s1
	call        AssemblerReset

	// *** Basic block 6

	lw          t1, 740(s1)
	seqz        t0, t1
	bnez        t1, .AssemblerRun_label_75

	// *** Basic block 7

	slti        t0, s4, 3

	// *** Basic block 8

.AssemblerRun_label_75:
	bnez        t0, .AssemblerRun_label_54

	// *** Basic block 9

.AssemblerRun_label_77:
	addi        a0, s0, -288
	lhu         a2, 748(s1)
	lhu         a3, 750(s1)
	mv          a6, s3
	mv          a5, s3
	mv          a4, x0
	mv          a1, s3
	call        ELFWriterFileInit

	// *** Basic block 10

	addi        t0, s1, 520
	ld          t0, 24(t0)
	beqz        t0, .AssemblerRun_label_115

	// *** Basic block 11

	addi        a0, s0, -288
	addi        a1, s1, 520
	call        ELFWriterAddFileSymbol

	// *** Basic block 12

.AssemblerRun_label_115:
	addi        a1, s0, -288
	mv          a0, s1
	call        AddSections

	// *** Basic block 13

	addi        a0, s1, 648
	addi        a2, s0, -288
	la          t0, AddLocalSymbolToELFFile
	mv          a1, t0
	call        HashTableTraverse

	// *** Basic block 14

	addi        t0, s0, -288
	addi        t1, s0, -288
	addi        t1, t1, 136
	ld          t1, 8(t1)
	sw          t1, 232(t0)
	addi        a0, s1, 648
	addi        a2, s0, -288
	la          t0, AddGlobalSymbolToELFFile
	mv          a1, t0
	call        HashTableTraverse

	// *** Basic block 15

	addi        a1, s0, -288
	mv          a0, s1
	call        AddRelocations

	// *** Basic block 16

	addi        a0, s0, -288
	ld          a1, 560(s1)
	call        ELFWriterFileWrite

	// *** Basic block 17

	addi        a0, s0, -288
	call        ELFWriterFileDestruct

	// *** Basic block 18

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AssemblerRun:
	.size AssemblerRun, .func_end_AssemblerRun-AssemblerRun

	.global AssemblerError
	.type AssemblerError, @function

AssemblerError:

	// *** Basic block 0

	.global VLexError
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, s0
	addi        a0, t0, 176
	mv          a2, t1
	j           VLexError
.func_end_AssemblerError:
	.size AssemblerError, .func_end_AssemblerError-AssemblerError

	.global AssemblerWarning
	.type AssemblerWarning, @function

AssemblerWarning:

	// *** Basic block 0

	.global VLexWarning
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, s0
	addi        a0, t0, 176
	mv          a3, t1
	j           VLexWarning
.func_end_AssemblerWarning:
	.size AssemblerWarning, .func_end_AssemblerWarning-AssemblerWarning

	.local  HandleDirective_align
	.type HandleDirective_align, @function

HandleDirective_align:

	// *** Basic block 0

	ret         
.func_end_HandleDirective_align:
	.size HandleDirective_align, .func_end_HandleDirective_align-HandleDirective_align

	.local  SymbolDirective
	.type SymbolDirective, @function

SymbolDirective:

	// *** Basic block 0

	.global LexLookingAt
	.global AssemblerFindSymbol
	.global NewAssemblerSymbol
	.global AssemblerInsertSymbol
	.global LexNextToken
	.global AssemblerError
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
	addi        a0, s1, 176
	li          a1, 3		// 0x3 ASCII \x3
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .SymbolDirective_label_81

	// *** Basic block 2

	addi        t0, a0, 80
	ld          s3, 16(t0)
	mv          a1, s3
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 3

	mv          s4, a0
	beq         s4, x0, .SymbolDirective_label_51

	// *** Basic block 4

	sw          s2, 44(s4)
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 72(s4)
	j           .SymbolDirective_label_76

	// *** Basic block 5

.SymbolDirective_label_51:
	lw          a1, 744(s1)
	mv          a4, x0
	mv          a3, s2
	mv          a2, x0
	mv          a0, s3
	call        NewAssemblerSymbol

	// *** Basic block 6

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        AssemblerInsertSymbol

	// *** Basic block 7

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 72(s4)

	// *** Basic block 8

.SymbolDirective_label_76:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 9

	j           .SymbolDirective_label_89

	// *** Basic block 10

.SymbolDirective_label_81:
	lla         a1, .str.47
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssemblerError

	// *** Basic block 11

.SymbolDirective_label_89:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SymbolDirective:
	.size SymbolDirective, .func_end_SymbolDirective-SymbolDirective

	.local  HandleDirective_globl
	.type HandleDirective_globl, @function

HandleDirective_globl:

	// *** Basic block 0

	.local SymbolDirective
	// Leaf procedure, no stack frame generated
	mv          a1, x0
	j           SymbolDirective
.func_end_HandleDirective_globl:
	.size HandleDirective_globl, .func_end_HandleDirective_globl-HandleDirective_globl

	.local  HandleDirective_global
	.type HandleDirective_global, @function

HandleDirective_global:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local HandleDirective_globl
	j           HandleDirective_globl
.func_end_HandleDirective_global:
	.size HandleDirective_global, .func_end_HandleDirective_global-HandleDirective_global

	.local  HandleDirective_local
	.type HandleDirective_local, @function

HandleDirective_local:

	// *** Basic block 0

	.local SymbolDirective
	// Leaf procedure, no stack frame generated
	li          a1, 1		// 0x1 ASCII \x1
	j           SymbolDirective
.func_end_HandleDirective_local:
	.size HandleDirective_local, .func_end_HandleDirective_local-HandleDirective_local

	.local  HandleDirective_comm
	.type HandleDirective_comm, @function

HandleDirective_comm:

	// *** Basic block 0

	.global LexLookingAt
	.global AssemblerFindSymbol
	.global NewAssemblerSymbol
	.global AssemblerInsertSymbol
	.global LexNextToken
	.global LexMatch
	.global AssemblerError
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
	addi        a0, s1, 176
	li          a1, 3		// 0x3 ASCII \x3
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .HandleDirective_comm_label_158

	// *** Basic block 2

	addi        t0, a0, 80
	ld          s2, 16(t0)
	mv          a1, s2
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 3

	mv          s3, a0
	beq         s3, x0, .HandleDirective_comm_label_67

	// *** Basic block 4

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 72(s3)
	sb          t0, 60(s3)
	li          t0, 65522		// 0xfff2
	sw          t0, 64(s3)
	sd          x0, 48(s3)
	sw          x0, 44(s3)
	j           .HandleDirective_comm_label_96

	// *** Basic block 5

.HandleDirective_comm_label_67:
	mv          a4, x0
	mv          a3, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 65522		// 0xfff2
	mv          a1, t0
	mv          a0, s2
	call        NewAssemblerSymbol

	// *** Basic block 6

	mv          s3, a0
	li          s2, 1		// 0x1 ASCII \x1
	sb          s2, 60(s3)
	mv          a1, s3
	mv          a0, s1
	call        AssemblerInsertSymbol

	// *** Basic block 7

	sb          s2, 72(s3)

	// *** Basic block 8

.HandleDirective_comm_label_96:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 9

	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 10

	not         t0, a0
	beqz        t0, .HandleDirective_comm_label_118

	// *** Basic block 11

	lla         a1, .str.48
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
	j           AssemblerError

	// *** Basic block 12

.HandleDirective_comm_label_115:
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

	// *** Basic block 13

.HandleDirective_comm_label_118:
	addi        a0, s1, 176
	li          s4, 2		// 0x2 ASCII \x2
	mv          a1, s4
	call        LexLookingAt

	// *** Basic block 14

	beqz        a0, .HandleDirective_comm_label_133

	// *** Basic block 15

	ld          t0, 120(a0)
	sw          t0, 56(s3)
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 16

.HandleDirective_comm_label_133:
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 17

	beqz        a0, .HandleDirective_comm_label_156

	// *** Basic block 18

	addi        a0, s1, 176
	mv          a1, s4
	call        LexLookingAt

	// *** Basic block 19

	beqz        a0, .HandleDirective_comm_label_155

	// *** Basic block 20

	ld          t0, 120(a0)
	sw          t0, 76(s3)
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 21

.HandleDirective_comm_label_155:

	// *** Basic block 22

.HandleDirective_comm_label_156:
	j           .HandleDirective_comm_label_165

	// *** Basic block 23

.HandleDirective_comm_label_158:
	lla         a1, .str.49
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 24

.HandleDirective_comm_label_165:
	j           .HandleDirective_comm_label_115
.func_end_HandleDirective_comm:
	.size HandleDirective_comm, .func_end_HandleDirective_comm-HandleDirective_comm

	.local  HandleDirective_type
	.type HandleDirective_type, @function

HandleDirective_type:

	// *** Basic block 0

	.global LexLookingAt
	.global StringInit
	.global LexNextToken
	.global LexMatch
	.global AssemblerFindSymbol
	.global NewAssemblerSymbol
	.global AssemblerInsertSymbol
	.global StringEqual
	.global AssemblerError
	.global StringDestruct
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
	addi        a0, s1, 176
	li          s2, 3		// 0x3 ASCII \x3
	mv          a1, s2
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .HandleDirective_type_label_243

	// *** Basic block 2

	addi        a0, s0, -96
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 3

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 4

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .HandleDirective_type_label_231

	// *** Basic block 6

	addi        t0, s0, -96
	ld          s3, 16(t0)
	mv          a1, s3
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 7

	mv          s4, a0
	bne         s4, x0, .HandleDirective_type_label_104

	// *** Basic block 8

	lw          a1, 744(s1)
	mv          a4, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, x0
	mv          a0, s3
	call        NewAssemblerSymbol

	// *** Basic block 9

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        AssemblerInsertSymbol

	// *** Basic block 10

.HandleDirective_type_label_104:
	li          s3, 1		// 0x1 ASCII \x1
	sb          s3, 72(s4)
	addi        a0, s1, 176
	mv          a1, s2
	call        LexLookingAt

	// *** Basic block 11

	beqz        a0, .HandleDirective_type_label_222

	// *** Basic block 12

	lw          s5, 64(s4)
	li          t0, 65522		// 0xfff2
	bne         s5, t0, .HandleDirective_type_label_125

	// *** Basic block 13

	j           .HandleDirective_type_label_131

	// *** Basic block 14

.HandleDirective_type_label_125:
	ld          t0, 600(s1)
	slli        t1, s5, 3
	add         t0, t0, t1
	ld          s2, 0(t0)

	// *** Basic block 15

.HandleDirective_type_label_131:
	addi        a0, s0, -56
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 16

	addi        a0, s0, -56
	lla         a1, .str.50
	call        StringEqual

	// *** Basic block 17

	mv          s5, a0
	bnez        a0, .HandleDirective_type_label_157

	// *** Basic block 18

	addi        a0, s0, -56
	lla         a1, .str.51
	call        StringEqual

	// *** Basic block 19

	mv          s5, a0

	// *** Basic block 20

.HandleDirective_type_label_157:
	beqz        s5, .HandleDirective_type_label_163

	// *** Basic block 21

	sw          s3, 40(s4)
	j           .HandleDirective_type_label_214

	// *** Basic block 22

.HandleDirective_type_label_163:
	addi        a0, s0, -56
	lla         a1, .str.52
	call        StringEqual

	// *** Basic block 23

	mv          s3, a0
	bnez        a0, .HandleDirective_type_label_180

	// *** Basic block 24

	addi        a0, s0, -56
	lla         a1, .str.53
	call        StringEqual

	// *** Basic block 25

	mv          s3, a0

	// *** Basic block 26

.HandleDirective_type_label_180:
	beqz        s3, .HandleDirective_type_label_202

	// *** Basic block 27

	sub         t1, s2, x0
	snez        t0, t1
	beq         s2, x0, .HandleDirective_type_label_190

	// *** Basic block 28

	lw          t1, 56(s2)
	andi        t1, t1, 1024
	snez        t0, t1

	// *** Basic block 29

.HandleDirective_type_label_190:
	beqz        t0, .HandleDirective_type_label_196

	// *** Basic block 30

	li          t0, 4		// 0x4 ASCII \x4
	sw          t0, 40(s4)
	j           .HandleDirective_type_label_200

	// *** Basic block 31

.HandleDirective_type_label_196:
	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, 40(s4)

	// *** Basic block 32

.HandleDirective_type_label_200:
	j           .HandleDirective_type_label_213

	// *** Basic block 33

.HandleDirective_type_label_202:
	lla         a1, .str.54
	addi        t0, s0, -56
	ld          a2, 16(t0)
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 34

.HandleDirective_type_label_213:

	// *** Basic block 35

.HandleDirective_type_label_214:
	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 36

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 37

	j           .HandleDirective_type_label_229

	// *** Basic block 38

.HandleDirective_type_label_222:
	lla         a1, .str.55
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 39

.HandleDirective_type_label_229:
	j           .HandleDirective_type_label_238

	// *** Basic block 40

.HandleDirective_type_label_231:
	lla         a1, .str.56
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 41

.HandleDirective_type_label_238:
	addi        a0, s0, -96
	call        StringDestruct

	// *** Basic block 42

	j           .HandleDirective_type_label_250

	// *** Basic block 43

.HandleDirective_type_label_243:
	lla         a1, .str.57
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 44

.HandleDirective_type_label_250:
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
.func_end_HandleDirective_type:
	.size HandleDirective_type, .func_end_HandleDirective_type-HandleDirective_type

	.local  HandleDirective_size
	.type HandleDirective_size, @function

HandleDirective_size:

	// *** Basic block 0

	.global LexLookingAt
	.global StringInit
	.global LexNextToken
	.global LexMatch
	.global AssemblerFindSymbol
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.global StringDestruct
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0
	addi        a0, s1, 176
	li          a1, 3		// 0x3 ASCII \x3
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .HandleDirective_size_label_100

	// *** Basic block 2

	addi        a0, s0, -64
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 3

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 4

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .HandleDirective_size_label_88

	// *** Basic block 6

	addi        t0, s0, -64
	ld          s2, 16(t0)
	mv          a1, s2
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 7

	mv          s3, a0
	bne         s3, x0, .HandleDirective_size_label_76

	// *** Basic block 8

	lla         a1, .str.58
	mv          a2, s2
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 9

	j           .HandleDirective_size_label_86

	// *** Basic block 10

.HandleDirective_size_label_76:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 11

	sext.w      t0, a0
	sw          t0, 56(s3)
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 72(s3)

	// *** Basic block 12

.HandleDirective_size_label_86:
	j           .HandleDirective_size_label_95

	// *** Basic block 13

.HandleDirective_size_label_88:
	lla         a1, .str.59
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 14

.HandleDirective_size_label_95:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 15

	j           .HandleDirective_size_label_107

	// *** Basic block 16

.HandleDirective_size_label_100:
	lla         a1, .str.60
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 17

.HandleDirective_size_label_107:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_HandleDirective_size:
	.size HandleDirective_size, .func_end_HandleDirective_size-HandleDirective_size

	.local  ExpressionPrimary
	.type ExpressionPrimary, @function

ExpressionPrimary:

	// *** Basic block 0

	.global LexLookingAt
	.global AssemblerFindSymbol
	.global LexNextToken
	.global AssemblerError
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
	addi        a0, s1, 176
	li          a1, 3		// 0x3 ASCII \x3
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .ExpressionPrimary_label_68

	// *** Basic block 2

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 3

	mv          s2, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 4

	bne         s2, x0, .ExpressionPrimary_label_64

	// *** Basic block 5

	lw          t0, 736(s1)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .ExpressionPrimary_label_63

	// *** Basic block 6

	lla         a1, .str.61
	addi        t0, a0, 80
	ld          a2, 16(t0)
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 7

	mv          a0, x0

	// *** Basic block 8

.ExpressionPrimary_label_60:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 9

.ExpressionPrimary_label_63:

	// *** Basic block 10

.ExpressionPrimary_label_64:
	mv          a0, s2
	j           .ExpressionPrimary_label_60

	// *** Basic block 11

.ExpressionPrimary_label_68:
	mv          a0, x0
	j           .ExpressionPrimary_label_60
.func_end_ExpressionPrimary:
	.size ExpressionPrimary, .func_end_ExpressionPrimary-ExpressionPrimary

	.local  SimpleSymbolExpression
	.type SimpleSymbolExpression, @function

SimpleSymbolExpression:

	// *** Basic block 0

	.local ExpressionPrimary
	.global abort
	.global NewAssemblerRelocation
	.global AssemblerCurrentAddress
	.global VectorAppend
	.global LexLookingAt
	.global LexNextToken
	.global printf
	.global VectorDestructWithContents
	.global AssemblerRelocationDestruct
	.global AssemblerAddRelocation
	.global VectorDestruct
	sd          a0, -8(s0)	// Spilled @401
	addi sp, sp, -176
	// Saved return address (offset 168) and frame pointer (offset 160)
	sd ra, 168(sp)
	sd s0, 160(sp)
	addi s0, sp, 176
	// Local vars at offset -48(s0)
	// Spilled register region: 40 bytes at -88(s0) to -48(s0)
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
	call        ExpressionPrimary

	// *** Basic block 1

	mv          s3, a0
	sd          s3, -80(s0)	// Spilled @50
	bne         s3, x0, .SimpleSymbolExpression_label_64

	// *** Basic block 2

	lw          t0, 40(s3)
	seqz        s4, t0
	mv          a0, x0

	// *** Basic block 3

.SimpleSymbolExpression_label_61:
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

	// *** Basic block 4

.SimpleSymbolExpression_label_64:
	li          s4, 1		// 0x1 ASCII \x1
	li          s5, 16		// 0x10 ASCII \x10
	beq         s2, s5, .SimpleSymbolExpression_label_89

	// *** Basic block 5

	li          t0, 32		// 0x20 ASCII ' '
	beq         s2, t0, .SimpleSymbolExpression_label_92

	// *** Basic block 6

	li          t0, 64		// 0x40 ASCII '@'
	beq         s2, t0, .SimpleSymbolExpression_label_95

	// *** Basic block 7

.SimpleSymbolExpression_label_86:
	call        abort

	// *** Basic block 8

	j           .SimpleSymbolExpression_label_98

	// *** Basic block 9

.SimpleSymbolExpression_label_89:
	mv          s4, x0
	sd          s4, -72(s0)	// Spilled @65
	j           .SimpleSymbolExpression_label_98

	// *** Basic block 10

.SimpleSymbolExpression_label_92:
	li          s4, 1		// 0x1 ASCII \x1
	j           .SimpleSymbolExpression_label_98

	// *** Basic block 11

.SimpleSymbolExpression_label_95:
	li          s4, 2		// 0x2 ASCII \x2
	j           .SimpleSymbolExpression_label_98

	// *** Basic block 12

.SimpleSymbolExpression_label_98:
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -48(s0)
	ld          t0, 752(s1)
	slli        t1, s4, 2
	add         t0, t0, t1
	lw          s6, 0(t0)
	lw          s7, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 13

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s7
	mv          a1, s6
	mv          a0, s3
	call        NewAssemblerRelocation

	// *** Basic block 14

	mv          s6, a0
	addi        a0, s0, -48
	mv          a1, s6
	call        VectorAppend

	// *** Basic block 15

	lb          s8, 73(s3)
	beqz        s8, .SimpleSymbolExpression_label_145

	// *** Basic block 16

	lw          t0, 40(s3)
	seqz        s8, t0

	// *** Basic block 17

.SimpleSymbolExpression_label_145:
	beqz        s8, .SimpleSymbolExpression_label_151

	// *** Basic block 18

	lw          t0, 64(s3)
	sub         t0, s7, t0
	seqz        s8, t0

	// *** Basic block 19

.SimpleSymbolExpression_label_151:
	addi        a0, s1, 176
	li          s7, 40		// 0x28 ASCII '('
	mv          a1, s7
	call        LexLookingAt

	// *** Basic block 20

	mv          s6, a0
	bnez        a0, .SimpleSymbolExpression_label_169

	// *** Basic block 21

	addi        a0, s1, 176
	li          t0, 33		// 0x21 ASCII '!'
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 22

	mv          s6, a0

	// *** Basic block 23

.SimpleSymbolExpression_label_169:
	beqz        s6, .SimpleSymbolExpression_label_308

	// *** Basic block 24

.SimpleSymbolExpression_label_171:
	addi        t0, s1, 176
	lw          s9, 72(t0)
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 25

	mv          a0, s1
	call        ExpressionPrimary

	// *** Basic block 26

	mv          s10, a0
	beq         s10, x0, .SimpleSymbolExpression_label_308

	// *** Basic block 27

.SimpleSymbolExpression_label_187:
	mv          s11, x0
	li          t0, 64		// 0x40 ASCII '@'
	bne         s2, t0, .SimpleSymbolExpression_label_203

	// *** Basic block 28

	bne         s9, s7, .SimpleSymbolExpression_label_199

	// *** Basic block 29

	li          s11, 5		// 0x5 ASCII \x5
	j           .SimpleSymbolExpression_label_201

	// *** Basic block 30

.SimpleSymbolExpression_label_199:
	li          s11, 8		// 0x8 ASCII \x8

	// *** Basic block 31

.SimpleSymbolExpression_label_201:
	j           .SimpleSymbolExpression_label_248

	// *** Basic block 32

.SimpleSymbolExpression_label_203:
	li          t0, 32		// 0x20 ASCII ' '
	bne         s2, t0, .SimpleSymbolExpression_label_217

	// *** Basic block 33

	bne         s9, s7, .SimpleSymbolExpression_label_213

	// *** Basic block 34

	li          s11, 4		// 0x4 ASCII \x4
	j           .SimpleSymbolExpression_label_215

	// *** Basic block 35

.SimpleSymbolExpression_label_213:
	li          s11, 7		// 0x7 ASCII \x7

	// *** Basic block 36

.SimpleSymbolExpression_label_215:
	j           .SimpleSymbolExpression_label_247

	// *** Basic block 37

.SimpleSymbolExpression_label_217:
	bne         s2, s5, .SimpleSymbolExpression_label_231

	// *** Basic block 38

	bne         s9, s7, .SimpleSymbolExpression_label_227

	// *** Basic block 39

	li          s11, 3		// 0x3 ASCII \x3
	j           .SimpleSymbolExpression_label_229

	// *** Basic block 40

.SimpleSymbolExpression_label_227:
	li          s11, 6		// 0x6 ASCII \x6

	// *** Basic block 41

.SimpleSymbolExpression_label_229:
	j           .SimpleSymbolExpression_label_246

	// *** Basic block 42

.SimpleSymbolExpression_label_231:
	lla         a0, .str.62
	lla         a1, .str.63
	lla         a3, .str.64
	li          t0, 947		// 0x3b3
	mv          a2, t0
	call        printf

	// *** Basic block 43

	call        abort

	// *** Basic block 44

.SimpleSymbolExpression_label_246:

	// *** Basic block 45

.SimpleSymbolExpression_label_247:

	// *** Basic block 46

.SimpleSymbolExpression_label_248:
	ld          t0, 752(s1)
	slli        t1, s11, 2
	add         t0, t0, t1
	lw          s5, 0(t0)
	lw          s9, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 47

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s9
	mv          a1, s5
	mv          a0, s10
	call        NewAssemblerRelocation

	// *** Basic block 48

	mv          s5, a0
	sd          s5, -88(s0)	// Spilled @270
	lb          s4, 73(s10)
	beqz        s4, .SimpleSymbolExpression_label_277

	// *** Basic block 49

.SimpleSymbolExpression_label_277:
	beqz        s4, .SimpleSymbolExpression_label_283

	// *** Basic block 50

	lw          t0, 64(s10)
	sub         t0, s9, t0
	seqz        s4, t0

	// *** Basic block 51

.SimpleSymbolExpression_label_283:
	or          s8, s8, s4
	addi        a0, s0, -48
	mv          a1, s5
	call        VectorAppend

	// *** Basic block 52

	addi        a0, s1, 176
	mv          a1, s7
	call        LexLookingAt

	// *** Basic block 53

	mv          s9, a0
	bnez        a0, .SimpleSymbolExpression_label_306

	// *** Basic block 54

	addi        a0, s1, 176
	li          t0, 33		// 0x21 ASCII '!'
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 55

	mv          s9, a0

	// *** Basic block 56

.SimpleSymbolExpression_label_306:
	bnez        s9, .SimpleSymbolExpression_label_171

	// *** Basic block 57

.SimpleSymbolExpression_label_308:
	mv          s7, x0
	beqz        s8, .SimpleSymbolExpression_label_376

	// *** Basic block 58

	ld          s8, -48(s0)
	ld          t0, 752(s1)
	sd          t0, -56(s0)	// Spilled @314
	lw          s9, 20(t0)
	lw          t0, 16(t0)
	ld          t1, -56(s0)	// Spilled @314
	lw          a0, 12(t1)
	ld          s4, -48(s0)
	ld          s7, 48(s3)
	li          s3, 1		// 0x1 ASCII \x1
	addi        t2, s0, -48
	ld          s5, 8(t2)
	li          t2, 1		// 0x1 ASCII \x1
	bge         t2, s5, .SimpleSymbolExpression_label_368

	// *** Basic block 59

.SimpleSymbolExpression_label_333:
	slli        t1, s3, 3
	add         t1, s8, t1
	ld          s8, 0(t1)
	lw          t2, 8(s8)
	sub         t3, t2, s9
	seqz        t1, t3
	beq         t2, s9, .SimpleSymbolExpression_label_347

	// *** Basic block 60

	sub         t0, t2, t0
	seqz        t1, t0

	// *** Basic block 61

.SimpleSymbolExpression_label_347:
	bnez        t1, .SimpleSymbolExpression_label_351

	// *** Basic block 62

	sub         t0, t2, a0
	seqz        t1, t0

	// *** Basic block 63

.SimpleSymbolExpression_label_351:
	beqz        t1, .SimpleSymbolExpression_label_358

	// *** Basic block 64

	ld          t0, 0(s8)
	ld          t0, 48(t0)
	add         s7, s7, t0
	j           .SimpleSymbolExpression_label_363

	// *** Basic block 65

.SimpleSymbolExpression_label_358:
	ld          t0, 0(s8)
	ld          t0, 48(t0)
	sub         s7, s7, t0

	// *** Basic block 66

.SimpleSymbolExpression_label_363:

	// *** Basic block 67

.SimpleSymbolExpression_label_364:
	addi        s3, s3, 1
	bge         s3, s5, .SimpleSymbolExpression_label_333

	// *** Basic block 68

.SimpleSymbolExpression_label_368:
	addi        a0, s0, -48
	la          t0, AssemblerRelocationDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 69

	j           .SimpleSymbolExpression_label_400

	// *** Basic block 70

.SimpleSymbolExpression_label_376:
	mv          s3, x0
	addi        t0, s0, -48
	ld          s4, 8(t0)
	bge         x0, s4, .SimpleSymbolExpression_label_396

	// *** Basic block 71

.SimpleSymbolExpression_label_384:
	slli        t0, s3, 3
	add         t0, s4, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 72

.SimpleSymbolExpression_label_392:
	addi        s3, s3, 1
	bge         s3, s4, .SimpleSymbolExpression_label_384

	// *** Basic block 73

.SimpleSymbolExpression_label_396:
	addi        a0, s0, -48
	call        VectorDestruct

	// *** Basic block 74

.SimpleSymbolExpression_label_400:
	mv          a0, s7
	j           .SimpleSymbolExpression_label_61
.func_end_SimpleSymbolExpression:
	.size SimpleSymbolExpression, .func_end_SimpleSymbolExpression-SimpleSymbolExpression

	.local  HandleDirective_p2align
	.type HandleDirective_p2align, @function

HandleDirective_p2align:

	// *** Basic block 0

	.global LexLookingAt
	.global LexNextToken
	.global BufferAddSpace
	.global AssemblerError
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
	addi        a0, s1, 176
	li          s2, 2		// 0x2 ASCII \x2
	mv          a1, s2
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .HandleDirective_p2align_label_82

	// *** Basic block 2

	ld          s3, 120(a0)
	li          t0, 1		// 0x1 ASCII \x1
	sll         t0, t0, s3
	addi        s3, t0, -1
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 3

	ld          t0, 600(s1)
	lw          t1, 744(s1)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	ld          t0, 48(s4)
	add         t1, t0, s3
	not         t2, s3
	and         s3, t1, t2
	sub         s5, s3, t0
	lw          t0, 736(s1)
	bne         t0, s2, .HandleDirective_p2align_label_76

	// *** Basic block 4

	addi        t0, s4, 8
	addi        a0, t0, 8
	mv          a1, s5
	call        BufferAddSpace

	// *** Basic block 5

	addi        t0, s4, 8
	ld          t1, 32(t0)
	add         t1, t1, s5
	sd          t1, 32(t0)

	// *** Basic block 6

.HandleDirective_p2align_label_76:
	ld          t0, 48(s4)
	add         t0, t0, s5
	sd          t0, 48(s4)
	j           .HandleDirective_p2align_label_90

	// *** Basic block 7

.HandleDirective_p2align_label_82:
	lla         a1, .str.65
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
	j           AssemblerError

	// *** Basic block 8

.HandleDirective_p2align_label_90:
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
.func_end_HandleDirective_p2align:
	.size HandleDirective_p2align, .func_end_HandleDirective_p2align-HandleDirective_p2align

	.local  HandleDataDirective
	.type HandleDataDirective, @function

HandleDataDirective:

	// *** Basic block 0

	.global LexEof
	.global AssemblerEvaluateExpression
	.global AssemblerEmitByte
	.global AssemblerEmitHalf
	.global AssemblerEmitWord
	.global printf
	.global abort
	.global LexMatch
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
	addi        a0, s1, 176
	call        LexEof

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .HandleDataDirective_label_148

	// *** Basic block 2

.HandleDataDirective_label_51:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 3

	mv          s3, a0
	li          t0, 8		// 0x8 ASCII \x8
	beq         s2, t0, .HandleDataDirective_label_83

	// *** Basic block 4

	li          t0, 16		// 0x10 ASCII \x10
	beq         s2, t0, .HandleDataDirective_label_95

	// *** Basic block 5

	li          t0, 32		// 0x20 ASCII ' '
	beq         s2, t0, .HandleDataDirective_label_106

	// *** Basic block 6

	li          t0, 64		// 0x40 ASCII '@'
	beq         s2, t0, .HandleDataDirective_label_117

	// *** Basic block 7

.HandleDataDirective_label_65:
	lla         a0, .str.69
	lla         a1, .str.70
	lla         a3, .str.71
	li          t0, 1023		// 0x3ff
	mv          a2, t0
	call        printf

	// *** Basic block 8

	call        abort

	// *** Basic block 9

	j           .HandleDataDirective_label_133

	// *** Basic block 10

.HandleDataDirective_label_83:
	lw          a1, 744(s1)
	andi        a2, s3, 255
	mv          a0, s1
	call        AssemblerEmitByte

	// *** Basic block 11

	j           .HandleDataDirective_label_133

	// *** Basic block 12

.HandleDataDirective_label_95:
	lw          a1, 744(s1)
	li          t0, 65535		// 0xffff
	and         a2, s3, t0
	mv          a0, s1
	call        AssemblerEmitHalf

	// *** Basic block 13

	j           .HandleDataDirective_label_133

	// *** Basic block 14

.HandleDataDirective_label_106:
	lw          a1, 744(s1)
	li          t0, 4294967295		// 0xffffffff
	and         a2, s3, t0
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 15

	j           .HandleDataDirective_label_133

	// *** Basic block 16

.HandleDataDirective_label_117:
	lla         a0, .str.66
	lla         a1, .str.67
	lla         a3, .str.68
	li          t0, 1020		// 0x3fc
	mv          a2, t0
	call        printf

	// *** Basic block 17

	call        abort

	// *** Basic block 18

	j           .HandleDataDirective_label_133

	// *** Basic block 19

.HandleDataDirective_label_133:
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 20

	not         t0, a0
	bnez        t0, .HandleDataDirective_label_148

	// *** Basic block 21

.HandleDataDirective_label_142:
	addi        a0, s1, 176
	call        LexEof

	// *** Basic block 22

	not         t0, a0
	bnez        t0, .HandleDataDirective_label_51

	// *** Basic block 23

.HandleDataDirective_label_148:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_HandleDataDirective:
	.size HandleDataDirective, .func_end_HandleDataDirective-HandleDataDirective

	.local  HandleDirective_space
	.type HandleDirective_space, @function

HandleDirective_space:

	// *** Basic block 0

	.global AssemblerEvaluateExpression
	.global BufferAddSpace
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
	call        AssemblerEvaluateExpression

	// *** Basic block 1

	mv          s2, a0
	ld          t0, 600(s1)
	lw          t1, 744(s1)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          s3, 0(t0)
	lw          t0, 736(s1)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .HandleDirective_space_label_49

	// *** Basic block 2

	addi        t0, s3, 8
	addi        a0, t0, 8
	mv          a1, s2
	call        BufferAddSpace

	// *** Basic block 3

	addi        t0, s3, 8
	ld          t1, 32(t0)
	add         t1, t1, s2
	sd          t1, 32(t0)

	// *** Basic block 4

.HandleDirective_space_label_49:
	ld          t0, 48(s3)
	add         t0, t0, s2
	sd          t0, 48(s3)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_HandleDirective_space:
	.size HandleDirective_space, .func_end_HandleDirective_space-HandleDirective_space

	.local  StringDirective
	.type StringDirective, @function

StringDirective:

	// *** Basic block 0

	.global LexLookingAt
	.global StringInit
	.global AssemblerEmitByte
	.global StringDestruct
	.global LexNextToken
	.global AssemblerError
	.global LexMatch
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

	// *** Basic block 1

.StringDirective_label_26:
	addi        a0, s1, 176
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 2

	lw          s3, 744(s1)
	beqz        a0, .StringDirective_label_79

	// *** Basic block 3

	addi        a0, s0, -64
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 4

	addi        t0, s0, -64
	ld          s4, 16(t0)
	mv          s5, x0
	addi        t0, s0, -64
	ld          t0, 24(t0)
	add         s6, t0, s2
	bge         x0, s6, .StringDirective_label_71

	// *** Basic block 5

.StringDirective_label_57:
	add         t0, s4, s5
	lb          a2, 0(t0)
	mv          a1, s3
	mv          a0, s1
	call        AssemblerEmitByte

	// *** Basic block 6

.StringDirective_label_67:
	addi        s5, s5, 1
	bge         s5, s6, .StringDirective_label_57

	// *** Basic block 7

.StringDirective_label_71:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 8

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 9

	j           .StringDirective_label_86

	// *** Basic block 10

.StringDirective_label_79:
	lla         a1, .str.72
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 11

.StringDirective_label_86:
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 12

	not         t0, a0
	bnez        t0, .StringDirective_label_98

	// *** Basic block 13

.StringDirective_label_95:

	// *** Basic block 14

.StringDirective_label_96:
	j           .StringDirective_label_26

	// *** Basic block 15

.StringDirective_label_98:
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
.func_end_StringDirective:
	.size StringDirective, .func_end_StringDirective-StringDirective

	.local  HandleDirective_asciz
	.type HandleDirective_asciz, @function

HandleDirective_asciz:

	// *** Basic block 0

	.local StringDirective
	// Leaf procedure, no stack frame generated
	li          a1, 1		// 0x1 ASCII \x1
	j           StringDirective
.func_end_HandleDirective_asciz:
	.size HandleDirective_asciz, .func_end_HandleDirective_asciz-HandleDirective_asciz

	.local  HandleDirective_string
	.type HandleDirective_string, @function

HandleDirective_string:

	// *** Basic block 0

	.local StringDirective
	// Leaf procedure, no stack frame generated
	li          a1, 1		// 0x1 ASCII \x1
	j           StringDirective
.func_end_HandleDirective_string:
	.size HandleDirective_string, .func_end_HandleDirective_string-HandleDirective_string

	.local  HandleDirective_ascii
	.type HandleDirective_ascii, @function

HandleDirective_ascii:

	// *** Basic block 0

	.local StringDirective
	// Leaf procedure, no stack frame generated
	mv          a1, x0
	j           StringDirective
.func_end_HandleDirective_ascii:
	.size HandleDirective_ascii, .func_end_HandleDirective_ascii-HandleDirective_ascii

	.local  HandleDirective_byte
	.type HandleDirective_byte, @function

HandleDirective_byte:

	// *** Basic block 0

	.local HandleDataDirective
	// Leaf procedure, no stack frame generated
	li          a1, 8		// 0x8 ASCII \x8
	j           HandleDataDirective
.func_end_HandleDirective_byte:
	.size HandleDirective_byte, .func_end_HandleDirective_byte-HandleDirective_byte

	.local  HandleDirective_hword
	.type HandleDirective_hword, @function

HandleDirective_hword:

	// *** Basic block 0

	.global LexEof
	.global LexLookingAt
	.local SimpleSymbolExpression
	.global AssemblerEmitHalf
	.global AssemblerEvaluateExpression
	.global LexMatch
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
	addi        a0, s1, 176
	call        LexEof

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .HandleDirective_hword_label_82

	// *** Basic block 2

.HandleDirective_hword_label_22:
	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 3

	beqz        a0, .HandleDirective_hword_label_52

	// *** Basic block 4

	li          t0, 16		// 0x10 ASCII \x10
	mv          a1, t0
	mv          a0, s1
	call        SimpleSymbolExpression

	// *** Basic block 5

	slli        t0, a0, 48
	srai        s2, t0, 48
	lw          a1, 744(s1)
	mv          a2, s2
	mv          a0, s1
	call        AssemblerEmitHalf

	// *** Basic block 6

	j           .HandleDirective_hword_label_67

	// *** Basic block 7

.HandleDirective_hword_label_52:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 8

	slli        t0, a0, 48
	srai        s2, t0, 48
	lw          a1, 744(s1)
	mv          a2, s2
	mv          a0, s1
	call        AssemblerEmitHalf

	// *** Basic block 9

.HandleDirective_hword_label_67:
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 10

	not         t0, a0
	bnez        t0, .HandleDirective_hword_label_82

	// *** Basic block 11

.HandleDirective_hword_label_76:
	addi        a0, s1, 176
	call        LexEof

	// *** Basic block 12

	not         t0, a0
	bnez        t0, .HandleDirective_hword_label_22

	// *** Basic block 13

.HandleDirective_hword_label_82:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_HandleDirective_hword:
	.size HandleDirective_hword, .func_end_HandleDirective_hword-HandleDirective_hword

	.local  HandleDirective_short
	.type HandleDirective_short, @function

HandleDirective_short:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local HandleDirective_hword
	j           HandleDirective_hword
.func_end_HandleDirective_short:
	.size HandleDirective_short, .func_end_HandleDirective_short-HandleDirective_short

	.local  HandleDirective_word
	.type HandleDirective_word, @function

HandleDirective_word:

	// *** Basic block 0

	.global LexEof
	.global LexLookingAt
	.local SimpleSymbolExpression
	.global AssemblerEmitWord
	.global AssemblerEvaluateExpression
	.global LexMatch
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
	addi        a0, s1, 176
	call        LexEof

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .HandleDirective_word_label_78

	// *** Basic block 2

.HandleDirective_word_label_21:
	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 3

	beqz        a0, .HandleDirective_word_label_49

	// *** Basic block 4

	li          t0, 32		// 0x20 ASCII ' '
	mv          a1, t0
	mv          a0, s1
	call        SimpleSymbolExpression

	// *** Basic block 5

	sext.w      s2, a0
	lw          a1, 744(s1)
	mv          a2, s2
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 6

	j           .HandleDirective_word_label_63

	// *** Basic block 7

.HandleDirective_word_label_49:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 8

	sext.w      s2, a0
	lw          a1, 744(s1)
	mv          a2, s2
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 9

.HandleDirective_word_label_63:
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 10

	not         t0, a0
	bnez        t0, .HandleDirective_word_label_78

	// *** Basic block 11

.HandleDirective_word_label_72:
	addi        a0, s1, 176
	call        LexEof

	// *** Basic block 12

	not         t0, a0
	bnez        t0, .HandleDirective_word_label_21

	// *** Basic block 13

.HandleDirective_word_label_78:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_HandleDirective_word:
	.size HandleDirective_word, .func_end_HandleDirective_word-HandleDirective_word

	.local  HandleDirective_2byte
	.type HandleDirective_2byte, @function

HandleDirective_2byte:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local HandleDirective_short
	j           HandleDirective_short
.func_end_HandleDirective_2byte:
	.size HandleDirective_2byte, .func_end_HandleDirective_2byte-HandleDirective_2byte

	.local  HandleDirective_4byte
	.type HandleDirective_4byte, @function

HandleDirective_4byte:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local HandleDirective_word
	j           HandleDirective_word
.func_end_HandleDirective_4byte:
	.size HandleDirective_4byte, .func_end_HandleDirective_4byte-HandleDirective_4byte

	.local  HandleDirective_8byte
	.type HandleDirective_8byte, @function

HandleDirective_8byte:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local HandleDirective_long
	j           HandleDirective_long
.func_end_HandleDirective_8byte:
	.size HandleDirective_8byte, .func_end_HandleDirective_8byte-HandleDirective_8byte

	.local  HandleDirective_long
	.type HandleDirective_long, @function

HandleDirective_long:

	// *** Basic block 0

	.global LexEof
	.global LexLookingAt
	.local SimpleSymbolExpression
	.global AssemblerEmitLong
	.global AssemblerEvaluateExpression
	.global LexMatch
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
	addi        a0, s1, 176
	call        LexEof

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .HandleDirective_long_label_78

	// *** Basic block 2

.HandleDirective_long_label_21:
	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 3

	beqz        a0, .HandleDirective_long_label_49

	// *** Basic block 4

	li          t0, 64		// 0x40 ASCII '@'
	mv          a1, t0
	mv          a0, s1
	call        SimpleSymbolExpression

	// *** Basic block 5

	mv          s2, a0
	lw          a1, 744(s1)
	mv          a2, s2
	mv          a0, s1
	call        AssemblerEmitLong

	// *** Basic block 6

	j           .HandleDirective_long_label_63

	// *** Basic block 7

.HandleDirective_long_label_49:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 8

	mv          s3, a0
	lw          a1, 744(s1)
	mv          a2, s3
	mv          a0, s1
	call        AssemblerEmitLong

	// *** Basic block 9

.HandleDirective_long_label_63:
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 10

	not         t0, a0
	bnez        t0, .HandleDirective_long_label_78

	// *** Basic block 11

.HandleDirective_long_label_72:
	addi        a0, s1, 176
	call        LexEof

	// *** Basic block 12

	not         t0, a0
	bnez        t0, .HandleDirective_long_label_21

	// *** Basic block 13

.HandleDirective_long_label_78:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_HandleDirective_long:
	.size HandleDirective_long, .func_end_HandleDirective_long-HandleDirective_long

	.local  HandleDirective_section
	.type HandleDirective_section, @function

HandleDirective_section:

	// *** Basic block 0

	.global LexLookingAt
	.global NewString
	.global LexNextToken
	.global LexMatch
	.global strcmp
	.global AssemblerFindSection
	.global AssemblerAddSection
	.global StringDelete
	.global AssemblerError
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
	addi        a0, s1, 176
	li          s3, 3		// 0x3 ASCII \x3
	mv          a1, s3
	call        LexLookingAt

	// *** Basic block 1

	mv          s2, a0
	bnez        a0, .HandleDirective_section_label_59

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 3

	mv          s2, a0

	// *** Basic block 4

.HandleDirective_section_label_59:
	beqz        s2, .HandleDirective_section_label_283

	// *** Basic block 5

	addi        t0, s1, 176
	addi        t0, t0, 80
	ld          a0, 16(t0)
	call        NewString

	// *** Basic block 6

	mv          s4, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 7

	mv          s5, x0
	mv          s6, x0
	addi        a0, s1, 176
	li          s7, 18		// 0x12 ASCII \x12
	mv          a1, s7
	call        LexMatch

	// *** Basic block 8

	beqz        a0, .HandleDirective_section_label_220

	// *** Basic block 9

	addi        a0, s1, 176
	mv          a1, s3
	call        LexLookingAt

	// *** Basic block 10

	mv          s8, a0
	bnez        a0, .HandleDirective_section_label_100

	// *** Basic block 11

	addi        a0, s1, 176
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 12

	mv          s8, a0

	// *** Basic block 13

.HandleDirective_section_label_100:
	beqz        s8, .HandleDirective_section_label_219

	// *** Basic block 14

	mv          s9, x0
	addi        t0, s1, 176
	addi        t0, t0, 80
	ld          s10, 16(t0)
	lb          t0, 0(s10)
	beqz        t0, .HandleDirective_section_label_171

	// *** Basic block 15

.HandleDirective_section_label_112:
	add         t0, s10, s9
	lb          t0, 0(t0)
	li          t1, 77		// 0x4d ASCII 'M'
	beq         t0, t1, .HandleDirective_section_label_155

	// *** Basic block 16

	li          t1, 83		// 0x53 ASCII 'S'
	beq         t0, t1, .HandleDirective_section_label_158

	// *** Basic block 17

	li          t1, 84		// 0x54 ASCII 'T'
	beq         t0, t1, .HandleDirective_section_label_161

	// *** Basic block 18

	li          t1, 97		// 0x61 ASCII 'a'
	beq         t0, t1, .HandleDirective_section_label_146

	// *** Basic block 19

	li          t1, 119		// 0x77 ASCII 'w'
	beq         t0, t1, .HandleDirective_section_label_149

	// *** Basic block 20

	li          t1, 120		// 0x78 ASCII 'x'
	beq         t0, t1, .HandleDirective_section_label_152

	// *** Basic block 21

	j           .HandleDirective_section_label_164

	// *** Basic block 22

.HandleDirective_section_label_146:
	ori         s5, s5, 2
	j           .HandleDirective_section_label_164

	// *** Basic block 23

.HandleDirective_section_label_149:
	ori         s5, s5, 1
	j           .HandleDirective_section_label_164

	// *** Basic block 24

.HandleDirective_section_label_152:
	ori         s5, s5, 4
	j           .HandleDirective_section_label_164

	// *** Basic block 25

.HandleDirective_section_label_155:
	ori         s5, s5, 16
	j           .HandleDirective_section_label_164

	// *** Basic block 26

.HandleDirective_section_label_158:
	ori         s5, s5, 32
	j           .HandleDirective_section_label_164

	// *** Basic block 27

.HandleDirective_section_label_161:
	ori         s5, s5, 1024
	j           .HandleDirective_section_label_164

	// *** Basic block 28

.HandleDirective_section_label_164:

	// *** Basic block 29

.HandleDirective_section_label_165:
	addi        s9, s9, 1
	add         t0, s10, s9
	lb          t0, 0(t0)
	beqz        t0, .HandleDirective_section_label_112

	// *** Basic block 30

.HandleDirective_section_label_171:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 31

	addi        a0, s1, 176
	mv          a1, s7
	call        LexMatch

	// *** Basic block 32

	beqz        a0, .HandleDirective_section_label_218

	// *** Basic block 33

	addi        a0, s1, 176
	mv          a1, s3
	call        LexLookingAt

	// *** Basic block 34

	beqz        a0, .HandleDirective_section_label_217

	// *** Basic block 35

	addi        t0, a0, 80
	ld          s3, 16(t0)
	lla         a1, .str.73
	mv          a0, s3
	call        strcmp

	// *** Basic block 36

	bnez        a0, .HandleDirective_section_label_202

	// *** Basic block 37

	li          s6, 1		// 0x1 ASCII \x1
	j           .HandleDirective_section_label_213

	// *** Basic block 38

.HandleDirective_section_label_202:
	lla         a1, .str.74
	mv          a0, s3
	call        strcmp

	// *** Basic block 39

	bnez        a0, .HandleDirective_section_label_212

	// *** Basic block 40

	li          s6, 8		// 0x8 ASCII \x8

	// *** Basic block 41

.HandleDirective_section_label_212:

	// *** Basic block 42

.HandleDirective_section_label_213:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 43

.HandleDirective_section_label_217:

	// *** Basic block 44

.HandleDirective_section_label_218:

	// *** Basic block 45

.HandleDirective_section_label_219:

	// *** Basic block 46

.HandleDirective_section_label_220:
	lw          t0, 736(s1)
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .HandleDirective_section_label_256

	// *** Basic block 47

	mv          a1, s4
	mv          a0, s1
	call        AssemblerFindSection

	// *** Basic block 48

	mv          s3, a0
	li          t0, -1		// 0xffffffffffffffff
	bne         s3, t0, .HandleDirective_section_label_254

	// *** Basic block 49

	li          t0, 8		// 0x8 ASCII \x8
	mv          a4, t0
	mv          a3, s5
	mv          a2, s6
	mv          a1, s4
	mv          a0, s1
	call        AssemblerAddSection

	// *** Basic block 50

	mv          s3, a0

	// *** Basic block 51

.HandleDirective_section_label_254:
	j           .HandleDirective_section_label_266

	// *** Basic block 52

.HandleDirective_section_label_256:
	mv          a1, s4
	mv          a0, s1
	call        AssemblerFindSection

	// *** Basic block 53

	mv          s3, a0
	mv          a0, s4
	call        StringDelete

	// *** Basic block 54

.HandleDirective_section_label_266:
	li          t0, -1		// 0xffffffffffffffff
	bne         s3, t0, .HandleDirective_section_label_278

	// *** Basic block 55

	lla         a1, .str.75
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 56

	j           .HandleDirective_section_label_281

	// *** Basic block 57

.HandleDirective_section_label_278:
	sw          s3, 744(s1)

	// *** Basic block 58

.HandleDirective_section_label_281:
	j           .HandleDirective_section_label_291

	// *** Basic block 59

.HandleDirective_section_label_283:
	lla         a1, .str.76
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
	j           AssemblerError

	// *** Basic block 60

.HandleDirective_section_label_291:
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
.func_end_HandleDirective_section:
	.size HandleDirective_section, .func_end_HandleDirective_section-HandleDirective_section

	.local  HandleDirective_text
	.type HandleDirective_text, @function

HandleDirective_text:

	// *** Basic block 0

	.global NewString
	.global AssemblerFindSection
	.global AssemblerAddSection
	.global StringDelete
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
	lla         a0, .str.77
	call        NewString

	// *** Basic block 1

	mv          s2, a0
	lw          t0, 736(s1)
	li          s3, 1		// 0x1 ASCII \x1
	bne         t0, s3, .HandleDirective_text_label_68

	// *** Basic block 2

	mv          a1, s2
	mv          a0, s1
	call        AssemblerFindSection

	// *** Basic block 3

	mv          s4, a0
	li          t0, -1		// 0xffffffffffffffff
	bne         s4, t0, .HandleDirective_text_label_66

	// *** Basic block 4

	li          t0, 8		// 0x8 ASCII \x8
	mv          a4, t0
	li          t0, 6		// 0x6 ASCII \x6
	mv          a3, t0
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        AssemblerAddSection

	// *** Basic block 5

	mv          s4, a0

	// *** Basic block 6

.HandleDirective_text_label_66:
	j           .HandleDirective_text_label_78

	// *** Basic block 7

.HandleDirective_text_label_68:
	mv          a1, s2
	mv          a0, s1
	call        AssemblerFindSection

	// *** Basic block 8

	mv          s4, a0
	mv          a0, s2
	call        StringDelete

	// *** Basic block 9

.HandleDirective_text_label_78:
	li          t0, -1		// 0xffffffffffffffff
	beq         s4, t0, .HandleDirective_text_label_84

	// *** Basic block 10

	j           .HandleDirective_text_label_100

	// *** Basic block 11

.HandleDirective_text_label_84:
	lla         a0, .str.78
	lla         a1, .str.79
	lla         a3, .str.80
	li          t0, 1223		// 0x4c7
	mv          a2, t0
	call        printf

	// *** Basic block 12

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           abort

	// *** Basic block 13

.HandleDirective_text_label_100:
	sw          s4, 744(s1)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_HandleDirective_text:
	.size HandleDirective_text, .func_end_HandleDirective_text-HandleDirective_text

	.local  HandleDirective_data
	.type HandleDirective_data, @function

HandleDirective_data:

	// *** Basic block 0

	.global NewString
	.global AssemblerFindSection
	.global AssemblerAddSection
	.global StringDelete
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
	lla         a0, .str.81
	call        NewString

	// *** Basic block 1

	mv          s2, a0
	lw          t0, 736(s1)
	li          s3, 1		// 0x1 ASCII \x1
	bne         t0, s3, .HandleDirective_data_label_68

	// *** Basic block 2

	mv          a1, s2
	mv          a0, s1
	call        AssemblerFindSection

	// *** Basic block 3

	mv          s4, a0
	li          t0, -1		// 0xffffffffffffffff
	bne         s4, t0, .HandleDirective_data_label_66

	// *** Basic block 4

	li          t0, 8		// 0x8 ASCII \x8
	mv          a4, t0
	li          t0, 3		// 0x3 ASCII \x3
	mv          a3, t0
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        AssemblerAddSection

	// *** Basic block 5

	mv          s4, a0

	// *** Basic block 6

.HandleDirective_data_label_66:
	j           .HandleDirective_data_label_78

	// *** Basic block 7

.HandleDirective_data_label_68:
	mv          a1, s2
	mv          a0, s1
	call        AssemblerFindSection

	// *** Basic block 8

	mv          s4, a0
	mv          a0, s2
	call        StringDelete

	// *** Basic block 9

.HandleDirective_data_label_78:
	li          t0, -1		// 0xffffffffffffffff
	beq         s4, t0, .HandleDirective_data_label_84

	// *** Basic block 10

	j           .HandleDirective_data_label_100

	// *** Basic block 11

.HandleDirective_data_label_84:
	lla         a0, .str.82
	lla         a1, .str.83
	lla         a3, .str.84
	li          t0, 1240		// 0x4d8
	mv          a2, t0
	call        printf

	// *** Basic block 12

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           abort

	// *** Basic block 13

.HandleDirective_data_label_100:
	sw          s4, 744(s1)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_HandleDirective_data:
	.size HandleDirective_data, .func_end_HandleDirective_data-HandleDirective_data

	.local  HandleDirective_file
	.type HandleDirective_file, @function

HandleDirective_file:

	// *** Basic block 0

	.global LexLookingAt
	.global LexNextToken
	.global StringSet
	.global AssemblerError
	.global DwarfAddFile
	.global StringDestruct
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	mv          s1, a0
	li          s2, -1		// 0xffffffffffffffff
	addi        a0, s1, 176
	li          a1, 2		// 0x2 ASCII \x2
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .HandleDirective_file_label_40

	// *** Basic block 2

	ld          s2, 120(a0)
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 3

.HandleDirective_file_label_40:
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	addi        a0, s1, 176
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 4

	beqz        a0, .HandleDirective_file_label_84

	// *** Basic block 5

	addi        a0, s0, -64
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringSet

	// *** Basic block 6

	li          t0, -1		// 0xffffffffffffffff
	bne         s2, t0, .HandleDirective_file_label_79

	// *** Basic block 7

	addi        a0, s1, 520
	addi        t0, s1, 176
	addi        t0, t0, 80
	ld          a1, 16(t0)
	call        StringSet

	// *** Basic block 8

.HandleDirective_file_label_79:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 9

	j           .HandleDirective_file_label_91

	// *** Basic block 10

.HandleDirective_file_label_84:
	lla         a1, .str.85
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 11

.HandleDirective_file_label_91:
	lw          t0, 736(s1)
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .HandleDirective_file_label_103

	// *** Basic block 12

	addi        a0, s1, 768
	addi        a1, s0, -64
	call        DwarfAddFile

	// *** Basic block 13

.HandleDirective_file_label_103:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 14

	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_HandleDirective_file:
	.size HandleDirective_file, .func_end_HandleDirective_file-HandleDirective_file

	.local  HandleDirective_loc
	.type HandleDirective_loc, @function

HandleDirective_loc:

	// *** Basic block 0

	.global LexLookingAt
	.global LexNextToken
	.global DwarfAddLocation
	.global AssemblerCurrentAddress
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
	mv          s3, x0
	mv          s4, x0
	addi        a0, s1, 176
	li          s5, 2		// 0x2 ASCII \x2
	mv          a1, s5
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .HandleDirective_loc_label_37

	// *** Basic block 2

	ld          s2, 120(a0)
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 3

.HandleDirective_loc_label_37:
	addi        a0, s1, 176
	mv          a1, s5
	call        LexLookingAt

	// *** Basic block 4

	beqz        a0, .HandleDirective_loc_label_50

	// *** Basic block 5

	ld          s3, 120(a0)
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 6

.HandleDirective_loc_label_50:
	addi        a0, s1, 176
	mv          a1, s5
	call        LexLookingAt

	// *** Basic block 7

	beqz        a0, .HandleDirective_loc_label_63

	// *** Basic block 8

	ld          s4, 120(a0)
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 9

.HandleDirective_loc_label_63:
	lw          t0, 736(s1)
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .HandleDirective_loc_label_90

	// *** Basic block 10

	addi        s5, s1, 768
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 11

	mv          a4, a0
	mv          a3, s4
	mv          a2, s3
	mv          a1, s2
	mv          a0, s5
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DwarfAddLocation

	// *** Basic block 12

.HandleDirective_loc_label_90:
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
.func_end_HandleDirective_loc:
	.size HandleDirective_loc, .func_end_HandleDirective_loc-HandleDirective_loc

	.local  HandleDirective_option
	.type HandleDirective_option, @function

HandleDirective_option:

	// *** Basic block 0

	.global LexLookingAt
	.global StringEqual
	.global LexNextToken
	.global LexMatch
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
	addi        a0, s1, 176
	li          s2, 3		// 0x3 ASCII \x3
	mv          a1, s2
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .HandleDirective_option_label_57

	// *** Basic block 2

.HandleDirective_option_label_24:
	addi        t0, s1, 176
	addi        a0, t0, 80
	lla         a1, .str.86
	call        StringEqual

	// *** Basic block 3

	beqz        a0, .HandleDirective_option_label_37

	// *** Basic block 4

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 760(s1)

	// *** Basic block 5

.HandleDirective_option_label_37:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	not         t0, a0
	bnez        t0, .HandleDirective_option_label_57

	// *** Basic block 8

.HandleDirective_option_label_49:
	addi        a0, s1, 176
	mv          a1, s2
	call        LexLookingAt

	// *** Basic block 9

	bnez        a0, .HandleDirective_option_label_24

	// *** Basic block 10

.HandleDirective_option_label_57:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_HandleDirective_option:
	.size HandleDirective_option, .func_end_HandleDirective_option-HandleDirective_option

	.local  HandleDirective_set
	.type HandleDirective_set, @function

HandleDirective_set:

	// *** Basic block 0

	.global LexLookingAt
	.global StringInit
	.global LexNextToken
	.global AssemblerEvaluateExpression
	.global AssemblerFindSymbol
	.global AssemblerCurrentAddress
	.global AssemblerError
	.global NewAssemblerSymbol
	.global AssemblerInsertSymbol
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
	// End of stack frame
	mv          s1, a0
	addi        a0, s1, 176
	li          a1, 3		// 0x3 ASCII \x3
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .HandleDirective_set_label_127

	// *** Basic block 2

	addi        a0, s0, -64
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 3

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 4

	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 5

	mv          s2, a0
	addi        t0, s0, -64
	ld          s3, 16(t0)
	mv          a1, s3
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 6

	mv          s4, a0
	lw          t0, 736(s1)
	li          s5, 1		// 0x1 ASCII \x1
	bne         t0, s5, .HandleDirective_set_label_126

	// *** Basic block 7

	beq         s4, x0, .HandleDirective_set_label_100

	// *** Basic block 8

	lb          t0, 60(s4)
	not         t0, t0
	beqz        t0, .HandleDirective_set_label_88

	// *** Basic block 9

	sb          s5, 60(s4)
	lw          t0, 744(s1)
	sw          t0, 64(s4)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 10

	sd          a0, 48(s4)
	j           .HandleDirective_set_label_98

	// *** Basic block 11

.HandleDirective_set_label_88:
	lla         a1, .str.87
	mv          a2, s3
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 12

.HandleDirective_set_label_98:
	j           .HandleDirective_set_label_125

	// *** Basic block 13

.HandleDirective_set_label_100:
	lw          a1, 744(s1)
	mv          a4, s2
	mv          a3, s5
	mv          a2, x0
	mv          a0, s3
	call        NewAssemblerSymbol

	// *** Basic block 14

	mv          s4, a0
	sb          s5, 60(s4)
	mv          a1, s4
	mv          a0, s1
	call        AssemblerInsertSymbol

	// *** Basic block 15

.HandleDirective_set_label_125:

	// *** Basic block 16

.HandleDirective_set_label_126:

	// *** Basic block 17

.HandleDirective_set_label_127:
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
.func_end_HandleDirective_set:
	.size HandleDirective_set, .func_end_HandleDirective_set-HandleDirective_set

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz ".globl"
	.type .str.1, @object
	.size .str.1, 7

.str.2:
	.asciz ".global"
	.type .str.2, @object
	.size .str.2, 8

.str.3:
	.asciz ".type"
	.type .str.3, @object
	.size .str.3, 6

.str.4:
	.asciz ".size"
	.type .str.4, @object
	.size .str.4, 6

.str.5:
	.asciz ".align"
	.type .str.5, @object
	.size .str.5, 7

.str.6:
	.asciz ".byte"
	.type .str.6, @object
	.size .str.6, 6

.str.7:
	.asciz ".short"
	.type .str.7, @object
	.size .str.7, 7

.str.8:
	.asciz ".2byte"
	.type .str.8, @object
	.size .str.8, 7

.str.9:
	.asciz ".hword"
	.type .str.9, @object
	.size .str.9, 7

.str.10:
	.asciz ".word"
	.type .str.10, @object
	.size .str.10, 6

.str.11:
	.asciz ".4byte"
	.type .str.11, @object
	.size .str.11, 7

.str.12:
	.asciz ".long"
	.type .str.12, @object
	.size .str.12, 6

.str.13:
	.asciz ".8byte"
	.type .str.13, @object
	.size .str.13, 7

.str.14:
	.asciz ".space"
	.type .str.14, @object
	.size .str.14, 7

.str.15:
	.asciz ".p2align"
	.type .str.15, @object
	.size .str.15, 9

.str.16:
	.asciz ".ascii"
	.type .str.16, @object
	.size .str.16, 7

.str.17:
	.asciz ".asciz"
	.type .str.17, @object
	.size .str.17, 7

.str.18:
	.asciz ".string"
	.type .str.18, @object
	.size .str.18, 8

.str.19:
	.asciz ".set"
	.type .str.19, @object
	.size .str.19, 5

.str.20:
	.asciz ".section"
	.type .str.20, @object
	.size .str.20, 9

.str.21:
	.asciz ".text"
	.type .str.21, @object
	.size .str.21, 6

.str.22:
	.asciz ".data"
	.type .str.22, @object
	.size .str.22, 6

.str.23:
	.asciz ".comm"
	.type .str.23, @object
	.size .str.23, 6

.str.24:
	.asciz ".local"
	.type .str.24, @object
	.size .str.24, 7

.str.25:
	.asciz ".file"
	.type .str.25, @object
	.size .str.25, 6

.str.26:
	.asciz ".loc"
	.type .str.26, @object
	.size .str.26, 5

.str.27:
	.asciz ".option"
	.type .str.27, @object
	.size .str.27, 8

.str.28:
	.asciz ".set"
	.type .str.28, @object
	.size .str.28, 5

.str.29:
	.asciz "@"
	.type .str.29, @object
	.size .str.29, 2

.str.30:
	.asciz "Invalid expression"
	.type .str.30, @object
	.size .str.30, 19

.str.31:
	.asciz "Floating point constant expected"
	.type .str.31, @object
	.size .str.31, 33

.str.32:
	.asciz "w"
	.type .str.32, @object
	.size .str.32, 2

.str.33:
	.asciz "Cannot open object file %s\n"
	.type .str.33, @object
	.size .str.33, 28

.str.34:
	.asciz "(null)"
	.type .str.34, @object
	.size .str.34, 1

.str.35:
	.asciz "assembler_symbols"
	.type .str.35, @object
	.size .str.35, 18

.str.36:
	.asciz ".text"
	.type .str.36, @object
	.size .str.36, 6

.str.37:
	.asciz ".data"
	.type .str.37, @object
	.size .str.37, 6

.str.38:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.38, @object
	.size .str.38, 30

.str.39:
	.asciz "assembler.c"
	.type .str.39, @object
	.size .str.39, 12

.str.40:
	.asciz "false"
	.type .str.40, @object
	.size .str.40, 6

.str.41:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.41, @object
	.size .str.41, 30

.str.42:
	.asciz "assembler.c"
	.type .str.42, @object
	.size .str.42, 12

.str.43:
	.asciz "false"
	.type .str.43, @object
	.size .str.43, 6

.str.44:
	.asciz "Duplicate symbol %s"
	.type .str.44, @object
	.size .str.44, 20

.str.45:
	.asciz ".debug_line"
	.type .str.45, @object
	.size .str.45, 12

.str.46:
	.asciz ".text"
	.type .str.46, @object
	.size .str.46, 6

.str.47:
	.asciz "Symbol name expected"
	.type .str.47, @object
	.size .str.47, 21

.str.48:
	.asciz "Missing size in .comm directive"
	.type .str.48, @object
	.size .str.48, 32

.str.49:
	.asciz "Symbol name expected"
	.type .str.49, @object
	.size .str.49, 21

.str.50:
	.asciz "function"
	.type .str.50, @object
	.size .str.50, 9

.str.51:
	.asciz "@function"
	.type .str.51, @object
	.size .str.51, 10

.str.52:
	.asciz "object"
	.type .str.52, @object
	.size .str.52, 7

.str.53:
	.asciz "@object"
	.type .str.53, @object
	.size .str.53, 8

.str.54:
	.asciz "Invalid .type syntax; unsupported type %s"
	.type .str.54, @object
	.size .str.54, 42

.str.55:
	.asciz "Invalid .type syntax; missing type"
	.type .str.55, @object
	.size .str.55, 35

.str.56:
	.asciz "Invalid .type syntax; no comma"
	.type .str.56, @object
	.size .str.56, 31

.str.57:
	.asciz "Symbol name expected"
	.type .str.57, @object
	.size .str.57, 21

.str.58:
	.asciz "Undefined symbol %s in .size"
	.type .str.58, @object
	.size .str.58, 29

.str.59:
	.asciz "Invalid .size syntax; no comma"
	.type .str.59, @object
	.size .str.59, 31

.str.60:
	.asciz "Symbol name expected"
	.type .str.60, @object
	.size .str.60, 21

.str.61:
	.asciz "No such symbol %s"
	.type .str.61, @object
	.size .str.61, 18

.str.62:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.62, @object
	.size .str.62, 30

.str.63:
	.asciz "assembler.c"
	.type .str.63, @object
	.size .str.63, 12

.str.64:
	.asciz "false"
	.type .str.64, @object
	.size .str.64, 6

.str.65:
	.asciz "Alignment expected"
	.type .str.65, @object
	.size .str.65, 19

.str.66:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.66, @object
	.size .str.66, 30

.str.67:
	.asciz "assembler.c"
	.type .str.67, @object
	.size .str.67, 12

.str.68:
	.asciz "false"
	.type .str.68, @object
	.size .str.68, 6

.str.69:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.69, @object
	.size .str.69, 30

.str.70:
	.asciz "assembler.c"
	.type .str.70, @object
	.size .str.70, 12

.str.71:
	.asciz "false"
	.type .str.71, @object
	.size .str.71, 6

.str.72:
	.asciz "Missing string for string directive"
	.type .str.72, @object
	.size .str.72, 36

.str.73:
	.asciz "@progbits"
	.type .str.73, @object
	.size .str.73, 10

.str.74:
	.asciz "@nobits"
	.type .str.74, @object
	.size .str.74, 8

.str.75:
	.asciz "Bad .section directive"
	.type .str.75, @object
	.size .str.75, 23

.str.76:
	.asciz "Missing section name"
	.type .str.76, @object
	.size .str.76, 21

.str.77:
	.asciz ".text"
	.type .str.77, @object
	.size .str.77, 6

.str.78:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.78, @object
	.size .str.78, 30

.str.79:
	.asciz "assembler.c"
	.type .str.79, @object
	.size .str.79, 12

.str.80:
	.asciz "section != -1"
	.type .str.80, @object
	.size .str.80, 14

.str.81:
	.asciz ".data"
	.type .str.81, @object
	.size .str.81, 6

.str.82:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.82, @object
	.size .str.82, 30

.str.83:
	.asciz "assembler.c"
	.type .str.83, @object
	.size .str.83, 12

.str.84:
	.asciz "section != -1"
	.type .str.84, @object
	.size .str.84, 14

.str.85:
	.asciz "Missing filename string in .file directive"
	.type .str.85, @object
	.size .str.85, 43

.str.86:
	.asciz "pic"
	.type .str.86, @object
	.size .str.86, 4

.str.87:
	.asciz "Duplicate symbol %s"
	.type .str.87, @object
	.size .str.87, 20

