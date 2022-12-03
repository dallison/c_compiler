	.file   "linker_main.c"
	.text
	.option pic
.PCbegin:
	.global Link
	.type Link, @function

Link:

	// *** Basic block 0

	.global LinkerInit
	.global strcmp
	.global fprintf
	.global stderr
	.global VectorAppend
	.global NewString
	.global chdir
	.global strtoll
	.global StringSet
	.global StringInit
	.global StringEndsWith
	.global StringDestruct
	.global StringSubstring
	.global StringAppend
	.global LinkerReadObjectFile
	.global printf
	.global LinkerInitArchitecture
	.global LinkerInitDynamic
	.global LinkerAddLibrarySearchDir
	.global LinkerAddLibrary
	.global StringDelete
	.global LinkerAddStaticLibrary
	.global LinkerAddDynamicLibrary
	.global VectorDestruct
	.global DynamicLinkerInventSymbols
	.global DynamicLinkerGatherDynamicRelocations
	.global LinkerLinkAllFiles
	.global fopen
	.global strerror
	.global errno
	.global LinkerWriteOutput
	.global fclose
	.global LinkerDestruct
	addi sp, sp, -880
	// Saved return address (offset 872) and frame pointer (offset 864)
	sd ra, 872(sp)
	sd s0, 864(sp)
	addi s0, sp, 880
	// Local vars at offset -768(s0)
	// Spilled register region: 16 bytes at -784(s0) to -768(s0)
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
	sd          s1, -776(s0)	// Spilled @78
	mv          s2, a1
	sd          s2, -776(s0)	// Spilled @81
	addi        a0, s0, -768
	call        LinkerInit

	// *** Basic block 1

	la          t0, stderr
	ld          s3, 0(t0)
	la          t0, stderr
	ld          s4, 0(t0)
	la          t0, stderr
	ld          s5, 0(t0)
	la          t0, stderr
	ld          s6, 0(t0)
	la          t0, stderr
	ld          s7, 0(t0)
	la          t0, stderr
	ld          s8, 0(t0)
	la          t0, stderr
	ld          s9, 0(t0)
	mv          s10, x0
	sd          x0, -176(s0)
	sd          x0, -168(s0)
	sd          x0, -160(s0)
	sd          x0, -176(s0)
	sd          x0, -152(s0)
	sd          x0, -144(s0)
	sd          x0, -136(s0)
	sd          x0, -152(s0)
	sd          x0, -128(s0)
	sd          x0, -120(s0)
	sd          x0, -112(s0)
	sd          x0, -128(s0)
	sd          x0, -104(s0)
	sd          x0, -96(s0)
	sd          x0, -88(s0)
	sd          x0, -104(s0)
	sd          x0, -80(s0)
	sd          x0, -72(s0)
	sd          x0, -64(s0)
	sd          x0, -80(s0)
	li          s11, 1		// 0x1 ASCII \x1
	li          s1, 1		// 0x1 ASCII \x1
	ld          t0, -776(s0)	// Spilled @78
	bge         s1, t0, .Link_label_511

	// *** Basic block 2

.Link_label_147:
	slli        t0, s11, 3
	add         t0, s2, t0
	ld          a1, 0(t0)
	sd          a1, -776(s0)	// Spilled @150
	lb          t0, 0(a1)
	li          t1, 45		// 0x2d ASCII '-'
	bne         t0, t1, .Link_label_439

	// *** Basic block 3

	mv          a1, x0
	sd          a1, -776(s0)	// Spilled @157
	lla         a1, .str.1
	ld          a0, -776(s0)	// Spilled @150
	call        strcmp

	// *** Basic block 4

	bnez        a0, .Link_label_173

	// *** Basic block 5

	addi        t0, s0, -768
	sb          s1, 528(t0)
	li          a1, 1		// 0x1 ASCII \x1
	j           .Link_label_338

	// *** Basic block 6

.Link_label_173:
	lla         a1, .str.2
	ld          a0, -776(s0)	// Spilled @150
	call        strcmp

	// *** Basic block 7

	bnez        a0, .Link_label_188

	// *** Basic block 8

	addi        t0, s0, -768
	sb          s1, 584(t0)
	li          a1, 1		// 0x1 ASCII \x1
	j           .Link_label_337

	// *** Basic block 9

.Link_label_188:
	lla         a1, .str.3
	ld          a0, -776(s0)	// Spilled @150
	call        strcmp

	// *** Basic block 10

	bnez        a0, .Link_label_203

	// *** Basic block 11

	addi        t0, s0, -768
	sb          s1, 585(t0)
	li          a1, 1		// 0x1 ASCII \x1
	j           .Link_label_336

	// *** Basic block 12

.Link_label_203:
	lla         a1, .str.4
	ld          a0, -776(s0)	// Spilled @150
	call        strcmp

	// *** Basic block 13

	bnez        a0, .Link_label_218

	// *** Basic block 14

	addi        t0, s0, -768
	sb          s1, 586(t0)
	li          a1, 1		// 0x1 ASCII \x1
	j           .Link_label_335

	// *** Basic block 15

.Link_label_218:
	lla         a1, .str.5
	ld          a0, -776(s0)	// Spilled @150
	call        strcmp

	// *** Basic block 16

	bnez        a0, .Link_label_233

	// *** Basic block 17

	addi        t0, s0, -768
	sb          s1, 529(t0)
	li          a1, 1		// 0x1 ASCII \x1
	j           .Link_label_334

	// *** Basic block 18

.Link_label_233:
	lla         a1, .str.6
	ld          a0, -776(s0)	// Spilled @150
	call        strcmp

	// *** Basic block 19

	bnez        a0, .Link_label_268

	// *** Basic block 20

	addi        s11, s11, 1
	ld          t0, -776(s0)	// Spilled @78
	blt         s11, t0, .Link_label_253

	// *** Basic block 21

	lla         a1, .str.7
	mv          a0, s3
	call        fprintf

	// *** Basic block 22

	j           .Link_label_511

	// *** Basic block 23

.Link_label_253:
	addi        t0, s0, -768
	addi        s1, t0, 96
	slli        t0, s11, 3
	add         t0, s2, t0
	ld          a0, 0(t0)
	call        NewString

	// *** Basic block 24

	mv          a1, a0
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 25

	li          a1, 1		// 0x1 ASCII \x1
	j           .Link_label_333

	// *** Basic block 26

.Link_label_268:
	lla         a1, .str.8
	ld          a0, -776(s0)	// Spilled @150
	call        strcmp

	// *** Basic block 27

	bnez        a0, .Link_label_296

	// *** Basic block 28

	addi        s11, s11, 1
	ld          t0, -776(s0)	// Spilled @78
	blt         s11, t0, .Link_label_288

	// *** Basic block 29

	lla         a1, .str.9
	mv          a0, s4
	call        fprintf

	// *** Basic block 30

	j           .Link_label_511

	// *** Basic block 31

.Link_label_288:
	slli        t0, s11, 3
	add         t0, s2, t0
	ld          a0, 0(t0)
	call        chdir

	// *** Basic block 32

	li          a1, 1		// 0x1 ASCII \x1
	j           .Link_label_332

	// *** Basic block 33

.Link_label_296:
	lla         a1, .str.10
	ld          a0, -776(s0)	// Spilled @150
	call        strcmp

	// *** Basic block 34

	bnez        a0, .Link_label_331

	// *** Basic block 35

	addi        s11, s11, 1
	ld          t0, -776(s0)	// Spilled @78
	blt         s11, t0, .Link_label_316

	// *** Basic block 36

	lla         a1, .str.11
	mv          a0, s5
	call        fprintf

	// *** Basic block 37

	j           .Link_label_511

	// *** Basic block 38

.Link_label_316:
	addi        s1, s0, -768
	slli        t0, s11, 3
	add         t0, s2, t0
	ld          a0, 0(t0)
	mv          a2, x0
	mv          a1, x0
	call        strtoll

	// *** Basic block 39

	sd          a0, 576(s1)
	li          a1, 1		// 0x1 ASCII \x1

	// *** Basic block 40

.Link_label_331:

	// *** Basic block 41

.Link_label_332:

	// *** Basic block 42

.Link_label_333:

	// *** Basic block 43

.Link_label_334:

	// *** Basic block 44

.Link_label_335:

	// *** Basic block 45

.Link_label_336:

	// *** Basic block 46

.Link_label_337:

	// *** Basic block 47

.Link_label_338:
	not         t0, a1
	beqz        t0, .Link_label_437

	// *** Basic block 48

	slli        s1, s11, 3
	add         t0, s2, s1
	ld          s3, 0(t0)
	lb          s4, 1(s3)
	li          t0, 73		// 0x49 ASCII 'I'
	beq         s4, t0, .Link_label_426

	// *** Basic block 49

	li          t0, 76		// 0x4c ASCII 'L'
	beq         s4, t0, .Link_label_376

	// *** Basic block 50

	li          t0, 108		// 0x6c ASCII 'l'
	beq         s4, t0, .Link_label_390

	// *** Basic block 51

	li          t0, 111		// 0x6f ASCII 'o'
	beq         s4, t0, .Link_label_404

	// *** Basic block 52

.Link_label_366:
	lla         a1, .str.13
	mv          a2, s3
	mv          a0, s6
	call        fprintf

	// *** Basic block 53

	j           .Link_label_436

	// *** Basic block 54

.Link_label_376:
	add         t0, s2, s1
	ld          t0, 0(t0)
	addi        a0, t0, 2
	call        NewString

	// *** Basic block 55

	mv          s4, a0
	sd          s4, -776(s0)	// Spilled @382
	addi        a0, s0, -104
	mv          a1, s4
	call        VectorAppend

	// *** Basic block 56

	j           .Link_label_436

	// *** Basic block 57

.Link_label_390:
	add         t0, s2, s1
	ld          t0, 0(t0)
	addi        a0, t0, 2
	call        NewString

	// *** Basic block 58

	mv          s3, a0
	sd          s3, -776(s0)	// Spilled @396
	addi        a0, s0, -80
	mv          a1, s3
	call        VectorAppend

	// *** Basic block 59

	j           .Link_label_436

	// *** Basic block 60

.Link_label_404:
	addi        s11, s11, 1
	ld          t0, -776(s0)	// Spilled @78
	blt         s11, t0, .Link_label_416

	// *** Basic block 61

	lla         a1, .str.12
	mv          a0, s7
	call        fprintf

	// *** Basic block 62

	j           .Link_label_436

	// *** Basic block 63

.Link_label_416:
	addi        a0, s0, -768
	slli        t0, s11, 3
	add         t0, s2, t0
	ld          a1, 0(t0)
	call        StringSet

	// *** Basic block 64

	li          s10, 1		// 0x1 ASCII \x1
	j           .Link_label_436

	// *** Basic block 65

.Link_label_426:
	addi        t0, s0, -768
	addi        a0, t0, 120
	add         t0, s2, s1
	ld          t0, 0(t0)
	addi        a1, t0, 2
	call        StringSet

	// *** Basic block 66

	j           .Link_label_436

	// *** Basic block 67

.Link_label_436:

	// *** Basic block 68

.Link_label_437:
	j           .Link_label_506

	// *** Basic block 69

.Link_label_439:
	addi        a0, s0, -56
	ld          a1, -776(s0)	// Spilled @150
	call        StringInit

	// *** Basic block 70

	addi        a0, s0, -56
	lla         a1, .str.14
	call        StringEndsWith

	// *** Basic block 71

	beqz        a0, .Link_label_458

	// *** Basic block 72

	addi        a0, s0, -152
	call        VectorAppend

	// *** Basic block 73

	j           .Link_label_502

	// *** Basic block 74

.Link_label_458:
	addi        a0, s0, -56
	lla         a1, .str.15
	call        StringEndsWith

	// *** Basic block 75

	beqz        a0, .Link_label_477

	// *** Basic block 76

	ld          a0, -776(s0)	// Spilled @150
	call        NewString

	// *** Basic block 77

	mv          s1, a0
	sd          s1, -784(s0)	// Spilled @469
	addi        a0, s0, -176
	mv          a1, s1
	call        VectorAppend

	// *** Basic block 78

	j           .Link_label_501

	// *** Basic block 79

.Link_label_477:
	addi        a0, s0, -56
	lla         a1, .str.16
	call        StringEndsWith

	// *** Basic block 80

	beqz        a0, .Link_label_491

	// *** Basic block 81

	addi        a0, s0, -128
	ld          a1, -776(s0)	// Spilled @150
	call        VectorAppend

	// *** Basic block 82

	j           .Link_label_500

	// *** Basic block 83

.Link_label_491:
	lla         a1, .str.17
	ld          a2, -776(s0)	// Spilled @150
	mv          a0, s8
	call        fprintf

	// *** Basic block 84

.Link_label_500:

	// *** Basic block 85

.Link_label_501:

	// *** Basic block 86

.Link_label_502:
	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 87

.Link_label_506:

	// *** Basic block 88

.Link_label_507:
	addi        s11, s11, 1
	ld          t0, -776(s0)	// Spilled @78
	bge         s11, t0, .Link_label_147

	// *** Basic block 89

.Link_label_511:
	not         t0, s10
	beqz        t0, .Link_label_570

	// *** Basic block 90

	ld          s5, -176(s0)
	ld          s6, -176(s0)
	ld          s7, -104(s0)
	ld          s8, -80(s0)
	ld          s10, -152(s0)
	ld          s11, -128(s0)
	addi        t0, s0, -768
	lb          t0, 528(t0)
	beqz        t0, .Link_label_569

	// *** Basic block 91

	mv          a1, x0
	addi        t0, s0, -176
	ld          s1, 8(t0)
	bge         x0, s1, .Link_label_568

	// *** Basic block 92

.Link_label_531:
	slli        t0, a1, 3
	add         t0, s5, t0
	ld          s5, 0(t0)
	lla         a1, .str.18
	mv          a0, s5
	call        StringEndsWith

	// *** Basic block 93

	beqz        a0, .Link_label_563

	// *** Basic block 94

	ld          t0, 24(s5)
	addi        a2, t0, -2
	addi        a3, s0, -768
	mv          a1, x0
	mv          a0, s5
	call        StringSubstring

	// *** Basic block 95

	addi        a0, s0, -768
	lla         a1, .str.19
	call        StringAppend

	// *** Basic block 96

	j           .Link_label_568

	// *** Basic block 97

.Link_label_563:

	// *** Basic block 98

.Link_label_564:
	addi        a1, a1, 1
	bge         a1, s1, .Link_label_531

	// *** Basic block 99

.Link_label_568:

	// *** Basic block 100

.Link_label_569:

	// *** Basic block 101

.Link_label_570:
	mv          s1, x0
	addi        t0, s0, -176
	ld          s5, 8(t0)
	bge         x0, s5, .Link_label_607

	// *** Basic block 102

.Link_label_578:
	slli        t0, s1, 3
	add         t0, s6, t0
	ld          s6, 0(t0)
	addi        a0, s0, -768
	mv          a1, s6
	call        LinkerReadObjectFile

	// *** Basic block 103

	mv          s7, a0
	not         t0, s7
	beqz        t0, .Link_label_599

	// *** Basic block 104

	lla         a0, .str.20
	ld          a1, 16(s6)
	call        printf

	// *** Basic block 105

.Link_label_599:
	mv          a0, s6
	call        StringDestruct

	// *** Basic block 106

.Link_label_603:
	addi        s1, s1, 1
	bge         s1, s5, .Link_label_578

	// *** Basic block 107

.Link_label_607:
	addi        t0, s0, -768
	lw          t0, 520(t0)
	bnez        t0, .Link_label_625

	// *** Basic block 108

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.21
	call        fprintf

	// *** Basic block 109

	mv          a0, x0

	// *** Basic block 110

.Link_label_622:
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

	// *** Basic block 111

.Link_label_625:
	addi        a0, s0, -768
	call        LinkerInitArchitecture

	// *** Basic block 112

	addi        a0, s0, -768
	call        LinkerInitDynamic

	// *** Basic block 113

	mv          s5, x0
	addi        t0, s0, -104
	ld          s6, 8(t0)
	bge         x0, s6, .Link_label_651

	// *** Basic block 114

.Link_label_639:
	addi        a0, s0, -768
	slli        t0, s5, 3
	add         t0, s7, t0
	ld          a1, 0(t0)
	call        LinkerAddLibrarySearchDir

	// *** Basic block 115

.Link_label_647:
	addi        s5, s5, 1
	bge         s5, s6, .Link_label_639

	// *** Basic block 116

.Link_label_651:
	mv          s6, x0
	mv          s8, x0
	addi        t0, s0, -80
	ld          s10, 8(t0)
	bge         x0, s10, .Link_label_695

	// *** Basic block 117

.Link_label_661:
	slli        s11, s8, 3
	add         t0, s8, s11
	ld          s3, 0(t0)
	addi        a0, s0, -768
	ld          a1, 16(s3)
	call        LinkerAddLibrary

	// *** Basic block 118

	mv          s4, a0
	not         t0, s4
	beqz        t0, .Link_label_687

	// *** Basic block 119

	lla         a1, .str.22
	add         t0, s2, s11
	ld          t0, 0(t0)
	addi        a2, t0, 2
	mv          a0, s9
	call        fprintf

	// *** Basic block 120

	addi        s6, s6, 1

	// *** Basic block 121

.Link_label_687:
	mv          a0, s3
	call        StringDelete

	// *** Basic block 122

.Link_label_691:
	addi        s8, s8, 1
	bge         s8, s10, .Link_label_661

	// *** Basic block 123

.Link_label_695:
	mv          s3, x0
	addi        t0, s0, -152
	ld          s9, 8(t0)
	bge         x0, s9, .Link_label_715

	// *** Basic block 124

.Link_label_703:
	addi        a0, s0, -768
	slli        t0, s3, 3
	add         t0, s10, t0
	ld          a1, 0(t0)
	call        LinkerAddStaticLibrary

	// *** Basic block 125

.Link_label_711:
	addi        s3, s3, 1
	bge         s3, s9, .Link_label_703

	// *** Basic block 126

.Link_label_715:
	mv          s9, x0
	addi        t0, s0, -128
	ld          s10, 8(t0)
	bge         x0, s10, .Link_label_735

	// *** Basic block 127

.Link_label_723:
	addi        a0, s0, -768
	slli        t0, s9, 3
	add         t0, s11, t0
	ld          a1, 0(t0)
	call        LinkerAddDynamicLibrary

	// *** Basic block 128

.Link_label_731:
	addi        s9, s9, 1
	bge         s9, s10, .Link_label_723

	// *** Basic block 129

.Link_label_735:
	addi        a0, s0, -176
	call        VectorDestruct

	// *** Basic block 130

	addi        a0, s0, -152
	call        VectorDestruct

	// *** Basic block 131

	addi        a0, s0, -128
	call        VectorDestruct

	// *** Basic block 132

	addi        a0, s0, -104
	call        VectorDestruct

	// *** Basic block 133

	addi        a0, s0, -80
	call        VectorDestruct

	// *** Basic block 134

	beqz        s6, .Link_label_756

	// *** Basic block 135

	mv          a0, x0
	j           .Link_label_622

	// *** Basic block 136

.Link_label_756:
	addi        t0, s0, -768
	lb          t0, 529(t0)
	not         t0, t0
	beqz        t0, .Link_label_772

	// *** Basic block 137

	addi        a0, s0, -768
	addi        t0, s0, -768
	ld          a1, 536(t0)
	call        DynamicLinkerInventSymbols

	// *** Basic block 138

	addi        a0, s0, -768
	call        DynamicLinkerGatherDynamicRelocations

	// *** Basic block 139

.Link_label_772:
	addi        a0, s0, -768
	call        LinkerLinkAllFiles

	// *** Basic block 140

	addi        t0, s0, -768
	ld          s10, 16(t0)
	mv          a0, s10
	call        NewString
	sd          a0, -776(s0)	// Spilled @781

	// *** Basic block 141

	mv          s11, a0
	lla         a1, .str.23
	mv          a0, s10
	call        fopen

	// *** Basic block 142

	bne         a0, x0, .Link_label_815

	// *** Basic block 143

	la          t0, stderr
	ld          s2, 0(t0)
	sd          s2, -784(s0)	// Spilled @796
	lla         s2, .str.24
	la          t0, errno
	lw          a0, 0(t0)
	call        strerror

	// *** Basic block 144

	mv          a3, a0
	mv          a2, s10
	mv          a1, s2
	ld          a0, -784(s0)	// Spilled @796
	call        fprintf

	// *** Basic block 145

	mv          a0, x0
	j           .Link_label_622

	// *** Basic block 146

.Link_label_815:
	addi        a0, s0, -768
	mv          a1, a0
	call        LinkerWriteOutput

	// *** Basic block 147

	call        fclose

	// *** Basic block 148

	addi        a0, s0, -768
	call        LinkerDestruct

	// *** Basic block 149

	mv          a0, s11
	j           .Link_label_622
.func_end_Link:
	.size Link, .func_end_Link-Link

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "-shared"
	.type .str.1, @object
	.size .str.1, 8

.str.2:
	.asciz "-Xsymbol-tables"
	.type .str.2, @object
	.size .str.2, 16

.str.3:
	.asciz "-Xrelocations"
	.type .str.3, @object
	.size .str.3, 14

.str.4:
	.asciz "-Xsections"
	.type .str.4, @object
	.size .str.4, 11

.str.5:
	.asciz "-static"
	.type .str.5, @object
	.size .str.5, 8

.str.6:
	.asciz "-rpath"
	.type .str.6, @object
	.size .str.6, 7

.str.7:
	.asciz "-rpath needs a path\n"
	.type .str.7, @object
	.size .str.7, 21

.str.8:
	.asciz "-chdir"
	.type .str.8, @object
	.size .str.8, 7

.str.9:
	.asciz "-chdir needs a dir\n"
	.type .str.9, @object
	.size .str.9, 20

.str.10:
	.asciz "-origin"
	.type .str.10, @object
	.size .str.10, 8

.str.11:
	.asciz "-origin needs a value\n"
	.type .str.11, @object
	.size .str.11, 23

.str.12:
	.asciz "-o needs an output filename\n"
	.type .str.12, @object
	.size .str.12, 29

.str.13:
	.asciz "Unknown option %s\n"
	.type .str.13, @object
	.size .str.13, 19

.str.14:
	.asciz ".a"
	.type .str.14, @object
	.size .str.14, 3

.str.15:
	.asciz ".o"
	.type .str.15, @object
	.size .str.15, 3

.str.16:
	.asciz ".so"
	.type .str.16, @object
	.size .str.16, 4

.str.17:
	.asciz "Unknown file type %s\n"
	.type .str.17, @object
	.size .str.17, 22

.str.18:
	.asciz ".o"
	.type .str.18, @object
	.size .str.18, 3

.str.19:
	.asciz ".so"
	.type .str.19, @object
	.size .str.19, 4

.str.20:
	.asciz "Failed to read object file %s"
	.type .str.20, @object
	.size .str.20, 30

.str.21:
	.asciz "No files to link\n"
	.type .str.21, @object
	.size .str.21, 18

.str.22:
	.asciz "Unable to find library %s\n"
	.type .str.22, @object
	.size .str.22, 27

.str.23:
	.asciz "w"
	.type .str.23, @object
	.size .str.23, 2

.str.24:
	.asciz "Can\'t open output file %s: %s\n"
	.type .str.24, @object
	.size .str.24, 31

