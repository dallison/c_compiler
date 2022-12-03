	.file   "6502/6502_assembler.c"
	.text
	.option pic
.PCbegin:
	.local  CompareCharPointers
	.type CompareCharPointers, @function

CompareCharPointers:

	// *** Basic block 0

	.global strcasecmp
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, t0
	mv          t3, t1
	ld          a0, 0(t2)
	ld          a1, 0(t3)
	j           strcasecmp
.func_end_CompareCharPointers:
	.size CompareCharPointers, .func_end_CompareCharPointers-CompareCharPointers

	.local  CompareString
	.type CompareString, @function

CompareString:

	// *** Basic block 0

	.global StringCompare
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, t0
	mv          t3, t1
	ld          a0, 0(t2)
	ld          a1, 0(t3)
	j           StringCompare
.func_end_CompareString:
	.size CompareString, .func_end_CompareString-CompareString

	.local  CompareSourceLocation
	.type CompareSourceLocation, @function

CompareSourceLocation:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	ld          t2, 0(t0)
	ld          t3, 0(t1)
	sub         t2, t2, t3
	sext.w      a0, t2

	// *** Basic block 1

.CompareSourceLocation_label_20:
	ret         
.func_end_CompareSourceLocation:
	.size CompareSourceLocation, .func_end_CompareSourceLocation-CompareSourceLocation

	.local  InitializeInstructions
	.type InitializeInstructions, @function

InitializeInstructions:

	// *** Basic block 0

	.local Assemble_brk
	.global MapInsert
	.local Assemble_bpl
	.local Assemble_bmi
	.local Assemble_bvc
	.local Assemble_bvs
	.local Assemble_bcc
	.local Assemble_bcs
	.local Assemble_bne
	.local Assemble_beq
	.local Assemble_jsr
	.local Assemble_jmp
	.local Assemble_rti
	.local Assemble_rts
	.local Assemble_lda
	.local Assemble_ldx
	.local Assemble_ldy
	.local Assemble_sta
	.local Assemble_stx
	.local Assemble_sty
	.local Assemble_cmp
	.local Assemble_cpy
	.local Assemble_cpx
	.local Assemble_bit
	.local Assemble_ora
	.local Assemble_and
	.local Assemble_eor
	.local Assemble_adc
	.local Assemble_sbc
	.local Assemble_asl
	.local Assemble_rol
	.local Assemble_lsr
	.local Assemble_ror
	.local Assemble_dec
	.local Assemble_inc
	.local Assemble_dey
	.local Assemble_dex
	.local Assemble_iny
	.local Assemble_inx
	.local Assemble_php
	.local Assemble_clc
	.local Assemble_plp
	.local Assemble_sec
	.local Assemble_pha
	.local Assemble_cli
	.local Assemble_pla
	.local Assemble_sei
	.local Assemble_tay
	.local Assemble_clv
	.local Assemble_cld
	.local Assemble_sed
	.local Assemble_tya
	.local Assemble_txa
	.local Assemble_txs
	.local Assemble_tax
	.local Assemble_tsx
	.local Assemble_nop
	.local Assemble_tsb
	.local Assemble_trb
	.local Assemble_stz
	.local Assemble_phy
	.local Assemble_ply
	.local Assemble_phx
	.local Assemble_plx
	.local Assemble_bra
	addi sp, sp, -1056
	// Saved return address (offset 1048) and frame pointer (offset 1040)
	sd ra, 1048(sp)
	sd s0, 1040(sp)
	addi s0, sp, 1056
	// Local vars at offset -1040(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0

	// *** Basic block 1

.InitializeInstructions_label_135:
	lla         t0, .str.1
	sd          t0, -1040(s0)
	addi        t0, s0, -1040
	la          t1, Assemble_brk
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1040(s0)
	sd          t0, 0(sp)
	ld          t0, -1032(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 2

	addi        sp, sp, 16

	// *** Basic block 3

.InitializeInstructions_label_160:

	// *** Basic block 4

.InitializeInstructions_label_161:

	// *** Basic block 5

.InitializeInstructions_label_162:
	lla         t0, .str.2
	sd          t0, -1024(s0)
	addi        t0, s0, -1024
	la          t1, Assemble_bpl
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1024(s0)
	sd          t0, 0(sp)
	ld          t0, -1016(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 6

	addi        sp, sp, 16

	// *** Basic block 7

.InitializeInstructions_label_182:

	// *** Basic block 8

.InitializeInstructions_label_183:

	// *** Basic block 9

.InitializeInstructions_label_184:
	lla         t0, .str.3
	sd          t0, -1008(s0)
	addi        t0, s0, -1008
	la          t1, Assemble_bmi
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1008(s0)
	sd          t0, 0(sp)
	ld          t0, -1000(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 10

	addi        sp, sp, 16

	// *** Basic block 11

.InitializeInstructions_label_204:

	// *** Basic block 12

.InitializeInstructions_label_205:

	// *** Basic block 13

.InitializeInstructions_label_206:
	lla         t0, .str.4
	sd          t0, -992(s0)
	addi        t0, s0, -992
	la          t1, Assemble_bvc
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -992(s0)
	sd          t0, 0(sp)
	ld          t0, -984(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 14

	addi        sp, sp, 16

	// *** Basic block 15

.InitializeInstructions_label_226:

	// *** Basic block 16

.InitializeInstructions_label_227:

	// *** Basic block 17

.InitializeInstructions_label_228:
	lla         t0, .str.5
	sd          t0, -976(s0)
	addi        t0, s0, -976
	la          t1, Assemble_bvs
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -976(s0)
	sd          t0, 0(sp)
	ld          t0, -968(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 18

	addi        sp, sp, 16

	// *** Basic block 19

.InitializeInstructions_label_248:

	// *** Basic block 20

.InitializeInstructions_label_249:

	// *** Basic block 21

.InitializeInstructions_label_250:
	lla         t0, .str.6
	sd          t0, -960(s0)
	addi        t0, s0, -960
	la          t1, Assemble_bcc
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -960(s0)
	sd          t0, 0(sp)
	ld          t0, -952(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 22

	addi        sp, sp, 16

	// *** Basic block 23

.InitializeInstructions_label_270:

	// *** Basic block 24

.InitializeInstructions_label_271:

	// *** Basic block 25

.InitializeInstructions_label_272:
	lla         t0, .str.7
	sd          t0, -944(s0)
	addi        t0, s0, -944
	la          t1, Assemble_bcs
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -944(s0)
	sd          t0, 0(sp)
	ld          t0, -936(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 26

	addi        sp, sp, 16

	// *** Basic block 27

.InitializeInstructions_label_292:

	// *** Basic block 28

.InitializeInstructions_label_293:

	// *** Basic block 29

.InitializeInstructions_label_294:
	lla         t0, .str.8
	sd          t0, -928(s0)
	addi        t0, s0, -928
	la          t1, Assemble_bne
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -928(s0)
	sd          t0, 0(sp)
	ld          t0, -920(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 30

	addi        sp, sp, 16

	// *** Basic block 31

.InitializeInstructions_label_314:

	// *** Basic block 32

.InitializeInstructions_label_315:

	// *** Basic block 33

.InitializeInstructions_label_316:
	lla         t0, .str.9
	sd          t0, -912(s0)
	addi        t0, s0, -912
	la          t1, Assemble_beq
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -912(s0)
	sd          t0, 0(sp)
	ld          t0, -904(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 34

	addi        sp, sp, 16

	// *** Basic block 35

.InitializeInstructions_label_336:

	// *** Basic block 36

.InitializeInstructions_label_337:

	// *** Basic block 37

.InitializeInstructions_label_338:
	lla         t0, .str.10
	sd          t0, -896(s0)
	addi        t0, s0, -896
	la          t1, Assemble_jsr
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -896(s0)
	sd          t0, 0(sp)
	ld          t0, -888(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 38

	addi        sp, sp, 16

	// *** Basic block 39

.InitializeInstructions_label_358:

	// *** Basic block 40

.InitializeInstructions_label_359:

	// *** Basic block 41

.InitializeInstructions_label_360:
	lla         t0, .str.11
	sd          t0, -880(s0)
	addi        t0, s0, -880
	la          t1, Assemble_jmp
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -880(s0)
	sd          t0, 0(sp)
	ld          t0, -872(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 42

	addi        sp, sp, 16

	// *** Basic block 43

.InitializeInstructions_label_380:

	// *** Basic block 44

.InitializeInstructions_label_381:

	// *** Basic block 45

.InitializeInstructions_label_382:
	lla         t0, .str.12
	sd          t0, -864(s0)
	addi        t0, s0, -864
	la          t1, Assemble_rti
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -864(s0)
	sd          t0, 0(sp)
	ld          t0, -856(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 46

	addi        sp, sp, 16

	// *** Basic block 47

.InitializeInstructions_label_402:

	// *** Basic block 48

.InitializeInstructions_label_403:

	// *** Basic block 49

.InitializeInstructions_label_404:
	lla         t0, .str.13
	sd          t0, -848(s0)
	addi        t0, s0, -848
	la          t1, Assemble_rts
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -848(s0)
	sd          t0, 0(sp)
	ld          t0, -840(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 50

	addi        sp, sp, 16

	// *** Basic block 51

.InitializeInstructions_label_424:

	// *** Basic block 52

.InitializeInstructions_label_425:

	// *** Basic block 53

.InitializeInstructions_label_426:
	lla         t0, .str.14
	sd          t0, -832(s0)
	addi        t0, s0, -832
	la          t1, Assemble_lda
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -832(s0)
	sd          t0, 0(sp)
	ld          t0, -824(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 54

	addi        sp, sp, 16

	// *** Basic block 55

.InitializeInstructions_label_446:

	// *** Basic block 56

.InitializeInstructions_label_447:

	// *** Basic block 57

.InitializeInstructions_label_448:
	lla         t0, .str.15
	sd          t0, -816(s0)
	addi        t0, s0, -816
	la          t1, Assemble_ldx
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -816(s0)
	sd          t0, 0(sp)
	ld          t0, -808(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 58

	addi        sp, sp, 16

	// *** Basic block 59

.InitializeInstructions_label_468:

	// *** Basic block 60

.InitializeInstructions_label_469:

	// *** Basic block 61

.InitializeInstructions_label_470:
	lla         t0, .str.16
	sd          t0, -800(s0)
	addi        t0, s0, -800
	la          t1, Assemble_ldy
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -800(s0)
	sd          t0, 0(sp)
	ld          t0, -792(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 62

	addi        sp, sp, 16

	// *** Basic block 63

.InitializeInstructions_label_490:

	// *** Basic block 64

.InitializeInstructions_label_491:

	// *** Basic block 65

.InitializeInstructions_label_492:
	lla         t0, .str.17
	sd          t0, -784(s0)
	addi        t0, s0, -784
	la          t1, Assemble_sta
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -784(s0)
	sd          t0, 0(sp)
	ld          t0, -776(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 66

	addi        sp, sp, 16

	// *** Basic block 67

.InitializeInstructions_label_512:

	// *** Basic block 68

.InitializeInstructions_label_513:

	// *** Basic block 69

.InitializeInstructions_label_514:
	lla         t0, .str.18
	sd          t0, -768(s0)
	addi        t0, s0, -768
	la          t1, Assemble_stx
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -768(s0)
	sd          t0, 0(sp)
	ld          t0, -760(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 70

	addi        sp, sp, 16

	// *** Basic block 71

.InitializeInstructions_label_534:

	// *** Basic block 72

.InitializeInstructions_label_535:

	// *** Basic block 73

.InitializeInstructions_label_536:
	lla         t0, .str.19
	sd          t0, -752(s0)
	addi        t0, s0, -752
	la          t1, Assemble_sty
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -752(s0)
	sd          t0, 0(sp)
	ld          t0, -744(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 74

	addi        sp, sp, 16

	// *** Basic block 75

.InitializeInstructions_label_556:

	// *** Basic block 76

.InitializeInstructions_label_557:

	// *** Basic block 77

.InitializeInstructions_label_558:
	lla         t0, .str.20
	sd          t0, -736(s0)
	addi        t0, s0, -736
	la          t1, Assemble_cmp
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -736(s0)
	sd          t0, 0(sp)
	ld          t0, -728(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 78

	addi        sp, sp, 16

	// *** Basic block 79

.InitializeInstructions_label_578:

	// *** Basic block 80

.InitializeInstructions_label_579:

	// *** Basic block 81

.InitializeInstructions_label_580:
	lla         t0, .str.21
	sd          t0, -720(s0)
	addi        t0, s0, -720
	la          t1, Assemble_cpy
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -720(s0)
	sd          t0, 0(sp)
	ld          t0, -712(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 82

	addi        sp, sp, 16

	// *** Basic block 83

.InitializeInstructions_label_600:

	// *** Basic block 84

.InitializeInstructions_label_601:

	// *** Basic block 85

.InitializeInstructions_label_602:
	lla         t0, .str.22
	sd          t0, -704(s0)
	addi        t0, s0, -704
	la          t1, Assemble_cpx
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -704(s0)
	sd          t0, 0(sp)
	ld          t0, -696(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 86

	addi        sp, sp, 16

	// *** Basic block 87

.InitializeInstructions_label_622:

	// *** Basic block 88

.InitializeInstructions_label_623:

	// *** Basic block 89

.InitializeInstructions_label_624:
	lla         t0, .str.23
	sd          t0, -688(s0)
	addi        t0, s0, -688
	la          t1, Assemble_bit
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -688(s0)
	sd          t0, 0(sp)
	ld          t0, -680(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 90

	addi        sp, sp, 16

	// *** Basic block 91

.InitializeInstructions_label_644:

	// *** Basic block 92

.InitializeInstructions_label_645:

	// *** Basic block 93

.InitializeInstructions_label_646:
	lla         t0, .str.24
	sd          t0, -672(s0)
	addi        t0, s0, -672
	la          t1, Assemble_ora
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -672(s0)
	sd          t0, 0(sp)
	ld          t0, -664(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 94

	addi        sp, sp, 16

	// *** Basic block 95

.InitializeInstructions_label_666:

	// *** Basic block 96

.InitializeInstructions_label_667:

	// *** Basic block 97

.InitializeInstructions_label_668:
	lla         t0, .str.25
	sd          t0, -656(s0)
	addi        t0, s0, -656
	la          t1, Assemble_and
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -656(s0)
	sd          t0, 0(sp)
	ld          t0, -648(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 98

	addi        sp, sp, 16

	// *** Basic block 99

.InitializeInstructions_label_688:

	// *** Basic block 100

.InitializeInstructions_label_689:

	// *** Basic block 101

.InitializeInstructions_label_690:
	lla         t0, .str.26
	sd          t0, -640(s0)
	addi        t0, s0, -640
	la          t1, Assemble_eor
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -640(s0)
	sd          t0, 0(sp)
	ld          t0, -632(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 102

	addi        sp, sp, 16

	// *** Basic block 103

.InitializeInstructions_label_710:

	// *** Basic block 104

.InitializeInstructions_label_711:

	// *** Basic block 105

.InitializeInstructions_label_712:
	lla         t0, .str.27
	sd          t0, -624(s0)
	addi        t0, s0, -624
	la          t1, Assemble_adc
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -624(s0)
	sd          t0, 0(sp)
	ld          t0, -616(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 106

	addi        sp, sp, 16

	// *** Basic block 107

.InitializeInstructions_label_732:

	// *** Basic block 108

.InitializeInstructions_label_733:

	// *** Basic block 109

.InitializeInstructions_label_734:
	lla         t0, .str.28
	sd          t0, -608(s0)
	addi        t0, s0, -608
	la          t1, Assemble_sbc
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -608(s0)
	sd          t0, 0(sp)
	ld          t0, -600(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 110

	addi        sp, sp, 16

	// *** Basic block 111

.InitializeInstructions_label_754:

	// *** Basic block 112

.InitializeInstructions_label_755:

	// *** Basic block 113

.InitializeInstructions_label_756:
	lla         t0, .str.29
	sd          t0, -592(s0)
	addi        t0, s0, -592
	la          t1, Assemble_asl
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -592(s0)
	sd          t0, 0(sp)
	ld          t0, -584(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 114

	addi        sp, sp, 16

	// *** Basic block 115

.InitializeInstructions_label_776:

	// *** Basic block 116

.InitializeInstructions_label_777:

	// *** Basic block 117

.InitializeInstructions_label_778:
	lla         t0, .str.30
	sd          t0, -576(s0)
	addi        t0, s0, -576
	la          t1, Assemble_rol
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -576(s0)
	sd          t0, 0(sp)
	ld          t0, -568(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 118

	addi        sp, sp, 16

	// *** Basic block 119

.InitializeInstructions_label_798:

	// *** Basic block 120

.InitializeInstructions_label_799:

	// *** Basic block 121

.InitializeInstructions_label_800:
	lla         t0, .str.31
	sd          t0, -560(s0)
	addi        t0, s0, -560
	la          t1, Assemble_lsr
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -560(s0)
	sd          t0, 0(sp)
	ld          t0, -552(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 122

	addi        sp, sp, 16

	// *** Basic block 123

.InitializeInstructions_label_820:

	// *** Basic block 124

.InitializeInstructions_label_821:

	// *** Basic block 125

.InitializeInstructions_label_822:
	lla         t0, .str.32
	sd          t0, -544(s0)
	addi        t0, s0, -544
	la          t1, Assemble_ror
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -544(s0)
	sd          t0, 0(sp)
	ld          t0, -536(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 126

	addi        sp, sp, 16

	// *** Basic block 127

.InitializeInstructions_label_842:

	// *** Basic block 128

.InitializeInstructions_label_843:

	// *** Basic block 129

.InitializeInstructions_label_844:
	lla         t0, .str.33
	sd          t0, -528(s0)
	addi        t0, s0, -528
	la          t1, Assemble_dec
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -528(s0)
	sd          t0, 0(sp)
	ld          t0, -520(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 130

	addi        sp, sp, 16

	// *** Basic block 131

.InitializeInstructions_label_864:

	// *** Basic block 132

.InitializeInstructions_label_865:

	// *** Basic block 133

.InitializeInstructions_label_866:
	lla         t0, .str.34
	sd          t0, -512(s0)
	addi        t0, s0, -512
	la          t1, Assemble_inc
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -512(s0)
	sd          t0, 0(sp)
	ld          t0, -504(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 134

	addi        sp, sp, 16

	// *** Basic block 135

.InitializeInstructions_label_886:

	// *** Basic block 136

.InitializeInstructions_label_887:

	// *** Basic block 137

.InitializeInstructions_label_888:
	lla         t0, .str.35
	sd          t0, -496(s0)
	addi        t0, s0, -496
	la          t1, Assemble_dey
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -496(s0)
	sd          t0, 0(sp)
	ld          t0, -488(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 138

	addi        sp, sp, 16

	// *** Basic block 139

.InitializeInstructions_label_908:

	// *** Basic block 140

.InitializeInstructions_label_909:

	// *** Basic block 141

.InitializeInstructions_label_910:
	lla         t0, .str.36
	sd          t0, -480(s0)
	addi        t0, s0, -480
	la          t1, Assemble_dex
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -480(s0)
	sd          t0, 0(sp)
	ld          t0, -472(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 142

	addi        sp, sp, 16

	// *** Basic block 143

.InitializeInstructions_label_930:

	// *** Basic block 144

.InitializeInstructions_label_931:

	// *** Basic block 145

.InitializeInstructions_label_932:
	lla         t0, .str.37
	sd          t0, -464(s0)
	addi        t0, s0, -464
	la          t1, Assemble_iny
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -464(s0)
	sd          t0, 0(sp)
	ld          t0, -456(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 146

	addi        sp, sp, 16

	// *** Basic block 147

.InitializeInstructions_label_952:

	// *** Basic block 148

.InitializeInstructions_label_953:

	// *** Basic block 149

.InitializeInstructions_label_954:
	lla         t0, .str.38
	sd          t0, -448(s0)
	addi        t0, s0, -448
	la          t1, Assemble_inx
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -448(s0)
	sd          t0, 0(sp)
	ld          t0, -440(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 150

	addi        sp, sp, 16

	// *** Basic block 151

.InitializeInstructions_label_974:

	// *** Basic block 152

.InitializeInstructions_label_975:

	// *** Basic block 153

.InitializeInstructions_label_976:
	lla         t0, .str.39
	sd          t0, -432(s0)
	addi        t0, s0, -432
	la          t1, Assemble_php
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -432(s0)
	sd          t0, 0(sp)
	ld          t0, -424(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 154

	addi        sp, sp, 16

	// *** Basic block 155

.InitializeInstructions_label_996:

	// *** Basic block 156

.InitializeInstructions_label_997:

	// *** Basic block 157

.InitializeInstructions_label_998:
	lla         t0, .str.40
	sd          t0, -416(s0)
	addi        t0, s0, -416
	la          t1, Assemble_clc
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -416(s0)
	sd          t0, 0(sp)
	ld          t0, -408(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 158

	addi        sp, sp, 16

	// *** Basic block 159

.InitializeInstructions_label_1018:

	// *** Basic block 160

.InitializeInstructions_label_1019:

	// *** Basic block 161

.InitializeInstructions_label_1020:
	lla         t0, .str.41
	sd          t0, -400(s0)
	addi        t0, s0, -400
	la          t1, Assemble_plp
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -400(s0)
	sd          t0, 0(sp)
	ld          t0, -392(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 162

	addi        sp, sp, 16

	// *** Basic block 163

.InitializeInstructions_label_1040:

	// *** Basic block 164

.InitializeInstructions_label_1041:

	// *** Basic block 165

.InitializeInstructions_label_1042:
	lla         t0, .str.42
	sd          t0, -384(s0)
	addi        t0, s0, -384
	la          t1, Assemble_sec
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -384(s0)
	sd          t0, 0(sp)
	ld          t0, -376(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 166

	addi        sp, sp, 16

	// *** Basic block 167

.InitializeInstructions_label_1062:

	// *** Basic block 168

.InitializeInstructions_label_1063:

	// *** Basic block 169

.InitializeInstructions_label_1064:
	lla         t0, .str.43
	sd          t0, -368(s0)
	addi        t0, s0, -368
	la          t1, Assemble_pha
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -368(s0)
	sd          t0, 0(sp)
	ld          t0, -360(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 170

	addi        sp, sp, 16

	// *** Basic block 171

.InitializeInstructions_label_1084:

	// *** Basic block 172

.InitializeInstructions_label_1085:

	// *** Basic block 173

.InitializeInstructions_label_1086:
	lla         t0, .str.44
	sd          t0, -352(s0)
	addi        t0, s0, -352
	la          t1, Assemble_cli
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -352(s0)
	sd          t0, 0(sp)
	ld          t0, -344(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 174

	addi        sp, sp, 16

	// *** Basic block 175

.InitializeInstructions_label_1106:

	// *** Basic block 176

.InitializeInstructions_label_1107:

	// *** Basic block 177

.InitializeInstructions_label_1108:
	lla         t0, .str.45
	sd          t0, -336(s0)
	addi        t0, s0, -336
	la          t1, Assemble_pla
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -336(s0)
	sd          t0, 0(sp)
	ld          t0, -328(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 178

	addi        sp, sp, 16

	// *** Basic block 179

.InitializeInstructions_label_1128:

	// *** Basic block 180

.InitializeInstructions_label_1129:

	// *** Basic block 181

.InitializeInstructions_label_1130:
	lla         t0, .str.46
	sd          t0, -320(s0)
	addi        t0, s0, -320
	la          t1, Assemble_sei
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -320(s0)
	sd          t0, 0(sp)
	ld          t0, -312(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 182

	addi        sp, sp, 16

	// *** Basic block 183

.InitializeInstructions_label_1150:

	// *** Basic block 184

.InitializeInstructions_label_1151:

	// *** Basic block 185

.InitializeInstructions_label_1152:
	lla         t0, .str.47
	sd          t0, -304(s0)
	addi        t0, s0, -304
	la          t1, Assemble_tay
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -304(s0)
	sd          t0, 0(sp)
	ld          t0, -296(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 186

	addi        sp, sp, 16

	// *** Basic block 187

.InitializeInstructions_label_1172:

	// *** Basic block 188

.InitializeInstructions_label_1173:

	// *** Basic block 189

.InitializeInstructions_label_1174:
	lla         t0, .str.48
	sd          t0, -288(s0)
	addi        t0, s0, -288
	la          t1, Assemble_clv
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -288(s0)
	sd          t0, 0(sp)
	ld          t0, -280(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 190

	addi        sp, sp, 16

	// *** Basic block 191

.InitializeInstructions_label_1194:

	// *** Basic block 192

.InitializeInstructions_label_1195:

	// *** Basic block 193

.InitializeInstructions_label_1196:
	lla         t0, .str.49
	sd          t0, -272(s0)
	addi        t0, s0, -272
	la          t1, Assemble_cld
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -272(s0)
	sd          t0, 0(sp)
	ld          t0, -264(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 194

	addi        sp, sp, 16

	// *** Basic block 195

.InitializeInstructions_label_1216:

	// *** Basic block 196

.InitializeInstructions_label_1217:

	// *** Basic block 197

.InitializeInstructions_label_1218:
	lla         t0, .str.50
	sd          t0, -256(s0)
	addi        t0, s0, -256
	la          t1, Assemble_sed
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -256(s0)
	sd          t0, 0(sp)
	ld          t0, -248(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 198

	addi        sp, sp, 16

	// *** Basic block 199

.InitializeInstructions_label_1238:

	// *** Basic block 200

.InitializeInstructions_label_1239:

	// *** Basic block 201

.InitializeInstructions_label_1240:
	lla         t0, .str.51
	sd          t0, -240(s0)
	addi        t0, s0, -240
	la          t1, Assemble_tya
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -240(s0)
	sd          t0, 0(sp)
	ld          t0, -232(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 202

	addi        sp, sp, 16

	// *** Basic block 203

.InitializeInstructions_label_1260:

	// *** Basic block 204

.InitializeInstructions_label_1261:

	// *** Basic block 205

.InitializeInstructions_label_1262:
	lla         t0, .str.52
	sd          t0, -224(s0)
	addi        t0, s0, -224
	la          t1, Assemble_txa
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -224(s0)
	sd          t0, 0(sp)
	ld          t0, -216(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 206

	addi        sp, sp, 16

	// *** Basic block 207

.InitializeInstructions_label_1282:

	// *** Basic block 208

.InitializeInstructions_label_1283:

	// *** Basic block 209

.InitializeInstructions_label_1284:
	lla         t0, .str.53
	sd          t0, -208(s0)
	addi        t0, s0, -208
	la          t1, Assemble_txs
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -208(s0)
	sd          t0, 0(sp)
	ld          t0, -200(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 210

	addi        sp, sp, 16

	// *** Basic block 211

.InitializeInstructions_label_1304:

	// *** Basic block 212

.InitializeInstructions_label_1305:

	// *** Basic block 213

.InitializeInstructions_label_1306:
	lla         t0, .str.54
	sd          t0, -192(s0)
	addi        t0, s0, -192
	la          t1, Assemble_tax
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -192(s0)
	sd          t0, 0(sp)
	ld          t0, -184(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 214

	addi        sp, sp, 16

	// *** Basic block 215

.InitializeInstructions_label_1326:

	// *** Basic block 216

.InitializeInstructions_label_1327:

	// *** Basic block 217

.InitializeInstructions_label_1328:
	lla         t0, .str.55
	sd          t0, -176(s0)
	addi        t0, s0, -176
	la          t1, Assemble_tsx
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -176(s0)
	sd          t0, 0(sp)
	ld          t0, -168(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 218

	addi        sp, sp, 16

	// *** Basic block 219

.InitializeInstructions_label_1348:

	// *** Basic block 220

.InitializeInstructions_label_1349:

	// *** Basic block 221

.InitializeInstructions_label_1350:
	lla         t0, .str.56
	sd          t0, -160(s0)
	addi        t0, s0, -160
	la          t1, Assemble_nop
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -160(s0)
	sd          t0, 0(sp)
	ld          t0, -152(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 222

	addi        sp, sp, 16

	// *** Basic block 223

.InitializeInstructions_label_1370:

	// *** Basic block 224

.InitializeInstructions_label_1371:

	// *** Basic block 225

.InitializeInstructions_label_1372:
	lla         t0, .str.57
	sd          t0, -144(s0)
	addi        t0, s0, -144
	la          t1, Assemble_tsb
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -144(s0)
	sd          t0, 0(sp)
	ld          t0, -136(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 226

	addi        sp, sp, 16

	// *** Basic block 227

.InitializeInstructions_label_1392:

	// *** Basic block 228

.InitializeInstructions_label_1393:

	// *** Basic block 229

.InitializeInstructions_label_1394:
	lla         t0, .str.58
	sd          t0, -128(s0)
	addi        t0, s0, -128
	la          t1, Assemble_trb
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -128(s0)
	sd          t0, 0(sp)
	ld          t0, -120(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 230

	addi        sp, sp, 16

	// *** Basic block 231

.InitializeInstructions_label_1414:

	// *** Basic block 232

.InitializeInstructions_label_1415:

	// *** Basic block 233

.InitializeInstructions_label_1416:
	lla         t0, .str.59
	sd          t0, -112(s0)
	addi        t0, s0, -112
	la          t1, Assemble_stz
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -112(s0)
	sd          t0, 0(sp)
	ld          t0, -104(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 234

	addi        sp, sp, 16

	// *** Basic block 235

.InitializeInstructions_label_1436:

	// *** Basic block 236

.InitializeInstructions_label_1437:

	// *** Basic block 237

.InitializeInstructions_label_1438:
	lla         t0, .str.60
	sd          t0, -96(s0)
	addi        t0, s0, -96
	la          t1, Assemble_phy
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -96(s0)
	sd          t0, 0(sp)
	ld          t0, -88(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 238

	addi        sp, sp, 16

	// *** Basic block 239

.InitializeInstructions_label_1458:

	// *** Basic block 240

.InitializeInstructions_label_1459:

	// *** Basic block 241

.InitializeInstructions_label_1460:
	lla         t0, .str.61
	sd          t0, -80(s0)
	addi        t0, s0, -80
	la          t1, Assemble_ply
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -80(s0)
	sd          t0, 0(sp)
	ld          t0, -72(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 242

	addi        sp, sp, 16

	// *** Basic block 243

.InitializeInstructions_label_1480:

	// *** Basic block 244

.InitializeInstructions_label_1481:

	// *** Basic block 245

.InitializeInstructions_label_1482:
	lla         t0, .str.62
	sd          t0, -64(s0)
	addi        t0, s0, -64
	la          t1, Assemble_phx
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -64(s0)
	sd          t0, 0(sp)
	ld          t0, -56(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 246

	addi        sp, sp, 16

	// *** Basic block 247

.InitializeInstructions_label_1502:

	// *** Basic block 248

.InitializeInstructions_label_1503:

	// *** Basic block 249

.InitializeInstructions_label_1504:
	lla         t0, .str.63
	sd          t0, -48(s0)
	addi        t0, s0, -48
	la          t1, Assemble_plx
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -48(s0)
	sd          t0, 0(sp)
	ld          t0, -40(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 250

	addi        sp, sp, 16

	// *** Basic block 251

.InitializeInstructions_label_1524:

	// *** Basic block 252

.InitializeInstructions_label_1525:

	// *** Basic block 253

.InitializeInstructions_label_1526:
	lla         t0, .str.64
	sd          t0, -32(s0)
	addi        t0, s0, -32
	la          t1, Assemble_bra
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -32(s0)
	sd          t0, 0(sp)
	ld          t0, -24(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 255

.InitializeInstructions_label_1546:

	// *** Basic block 256

.InitializeInstructions_label_1547:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InitializeInstructions:
	.size InitializeInstructions, .func_end_InitializeInstructions-InitializeInstructions

	.global W65C02AssemblerInit
	.type W65C02AssemblerInit, @function

W65C02AssemblerInit:

	// *** Basic block 0

	.global AssemblerInit
	.local reloc_types
	.global MapInitForCaseBlindCharPointerKeys
	.global MapInitForStringKeys
	.global MapInitForInt64Keys
	.local InitializeInstructions
	.global AssemblerAddSection
	.global NewString
	.local DefineLabel
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
	lla         a3, reloc_types
	mv          a5, t1
	mv          a4, t0
	li          a2, 1		// 0x1 ASCII \x1
	li          a1, 6502		// 0x1966
	call        AssemblerInit

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .W65C02AssemblerInit_label_57

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.W65C02AssemblerInit_label_54:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.W65C02AssemblerInit_label_57:
	addi        a0, s1, 872
	call        MapInitForCaseBlindCharPointerKeys

	// *** Basic block 5

	addi        a0, s1, 944
	call        MapInitForStringKeys

	// *** Basic block 6

	addi        a0, s1, 912
	call        MapInitForInt64Keys

	// *** Basic block 7

	addi        a0, s1, 872
	call        InitializeInstructions

	// *** Basic block 8

	mv          a4, x0
	mv          a3, x0
	mv          a2, x0
	mv          a1, x0
	mv          a0, s1
	call        AssemblerAddSection

	// *** Basic block 9

	lla         a0, .str.65
	call        NewString

	// *** Basic block 10

	li          t0, 8		// 0x8 ASCII \x8
	mv          a4, t0
	li          t1, 3		// 0x3 ASCII \x3
	mv          a3, t1
	mv          a2, t0
	mv          a1, a0
	mv          a0, s1
	call        AssemblerAddSection

	// *** Basic block 11

	sw          a0, 904(s1)
	ld          t0, 864(s1)
	sd          t0, 976(s1)
	la          t0, DefineLabel
	sd          t0, 864(s1)
	li          a0, 1		// 0x1 ASCII \x1
	j           .W65C02AssemblerInit_label_54
.func_end_W65C02AssemblerInit:
	.size W65C02AssemblerInit, .func_end_W65C02AssemblerInit-W65C02AssemblerInit

	.local  BranchDelete
	.type BranchDelete, @function

BranchDelete:

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
.func_end_BranchDelete:
	.size BranchDelete, .func_end_BranchDelete-BranchDelete

	.local  BranchMapDestruct
	.type BranchMapDestruct, @function

BranchMapDestruct:

	// *** Basic block 0

	.local BranchDelete
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 8(t0)
	j           BranchDelete
.func_end_BranchMapDestruct:
	.size BranchMapDestruct, .func_end_BranchMapDestruct-BranchMapDestruct

	.local  LabelMapDestruct
	.type LabelMapDestruct, @function

LabelMapDestruct:

	// *** Basic block 0

	.global StringDelete
	.local LabelDestruct
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
	call        StringDelete

	// *** Basic block 1

	ld          a0, 8(s1)
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LabelDestruct
.func_end_LabelMapDestruct:
	.size LabelMapDestruct, .func_end_LabelMapDestruct-LabelMapDestruct

	.global New6502Assembler
	.type New6502Assembler, @function

New6502Assembler:

	// *** Basic block 0

	.global malloc
	.global W65C02AssemblerInit
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
	li          a0, 984		// 0x3d8
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        W65C02AssemblerInit

	// *** Basic block 2

	mv          a0, s3

	// *** Basic block 3

.New6502Assembler_label_26:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_New6502Assembler:
	.size New6502Assembler, .func_end_New6502Assembler-New6502Assembler

	.global W65C02AssemblerDestruct
	.type W65C02AssemblerDestruct, @function

W65C02AssemblerDestruct:

	// *** Basic block 0

	.global AssemblerDestruct
	.global MapDestruct
	.global MapDestructWithContents
	.local BranchMapDestruct
	.local LabelMapDestruct
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
	call        AssemblerDestruct

	// *** Basic block 1

	addi        a0, s1, 872
	call        MapDestruct

	// *** Basic block 2

	addi        a0, s1, 912
	la          t0, BranchMapDestruct
	mv          a1, t0
	call        MapDestructWithContents

	// *** Basic block 3

	addi        a0, s1, 944
	la          t0, LabelMapDestruct
	mv          a1, t0
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           MapDestructWithContents
.func_end_W65C02AssemblerDestruct:
	.size W65C02AssemblerDestruct, .func_end_W65C02AssemblerDestruct-W65C02AssemblerDestruct

	.global W65C02AssemblerDelete
	.type W65C02AssemblerDelete, @function

W65C02AssemblerDelete:

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
	.global W65C02AssemblerDestruct
	.global free
	mv          s1, a0
	call        W65C02AssemblerDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_W65C02AssemblerDelete:
	.size W65C02AssemblerDelete, .func_end_W65C02AssemblerDelete-W65C02AssemblerDelete

	.global Assemble6502Instruction
	.type Assemble6502Instruction, @function

Assemble6502Instruction:

	// *** Basic block 0

	.global MapFindPointerKey
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
	mv          t0, a0
	mv          t1, a1
	mv          s1, t0
	addi        a0, s1, 872
	ld          s2, 16(t1)
	mv          a1, s2
	call        MapFindPointerKey

	// *** Basic block 1

	mv          s3, a0
	beq         s3, x0, .Assemble6502Instruction_label_35

	// *** Basic block 2

	mv          s4, s3
	mv          a0, s1
	jalr         x1, s4, 0

	// *** Basic block 3

	j           .Assemble6502Instruction_label_46

	// *** Basic block 4

.Assemble6502Instruction_label_35:
	lla         a1, .str.66
	mv          a2, s2
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

	// *** Basic block 5

.Assemble6502Instruction_label_46:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble6502Instruction:
	.size Assemble6502Instruction, .func_end_Assemble6502Instruction-Assemble6502Instruction

	.local  GetOrCreateSymbol
	.type GetOrCreateSymbol, @function

GetOrCreateSymbol:

	// *** Basic block 0

	.global AssemblerFindSymbol
	.global NewAssemblerSymbol
	.global AssemblerInsertSymbol
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
	call        AssemblerFindSymbol

	// *** Basic block 1

	mv          s3, a0
	bne         s3, x0, .GetOrCreateSymbol_label_49

	// *** Basic block 2

	lw          a1, 744(s1)
	mv          a4, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, t0
	mv          a0, s2
	call        NewAssemblerSymbol

	// *** Basic block 3

	mv          s3, a0
	mv          a1, s3
	mv          a0, s1
	call        AssemblerInsertSymbol

	// *** Basic block 4

.GetOrCreateSymbol_label_49:
	mv          a0, s3

	// *** Basic block 5

.GetOrCreateSymbol_label_52:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GetOrCreateSymbol:
	.size GetOrCreateSymbol, .func_end_GetOrCreateSymbol-GetOrCreateSymbol

	.local  NewBranch
	.type NewBranch, @function

NewBranch:

	// *** Basic block 0

	.global malloc
	.global StringInit
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
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	sw          s1, 0(s3)
	addi        a0, s3, 8
	mv          a1, s2
	call        StringInit

	// *** Basic block 2

	sd          x0, 48(s3)
	sw          x0, 4(s3)
	sd          x0, 56(s3)
	mv          a0, s3

	// *** Basic block 3

.NewBranch_label_37:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewBranch:
	.size NewBranch, .func_end_NewBranch-NewBranch

	.local  NewLabel
	.type NewLabel, @function

NewLabel:

	// *** Basic block 0

	.global malloc
	.global VectorInit
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
	li          a0, 40		// 0x28 ASCII '('
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	sd          s1, 0(s3)
	addi        a0, s3, 8
	call        VectorInit

	// *** Basic block 2

	sd          s2, 32(s3)
	mv          a0, s3

	// *** Basic block 3

.NewLabel_label_28:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewLabel:
	.size NewLabel, .func_end_NewLabel-NewLabel

	.local  LabelDestruct
	.type LabelDestruct, @function

LabelDestruct:

	// *** Basic block 0

	.global VectorDestruct
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 8
	j           VectorDestruct
.func_end_LabelDestruct:
	.size LabelDestruct, .func_end_LabelDestruct-LabelDestruct

	.local  InsertBranch
	.type InsertBranch, @function

InsertBranch:

	// *** Basic block 0

	.global MapInsert
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -32(s0)
	// End of stack frame
	mv          t0, a0
	sd          a1, -32(s0)
	addi        t1, s0, -32
	sd          a2, 8(t1)
	addi        a0, t0, 912
	addi        sp, sp, -16
	ld          t1, -32(s0)
	sd          t1, 0(sp)
	ld          t1, -24(s0)
	sd          t1, 8(sp)
	addi        a1, sp, 0
	call        MapInsert

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InsertBranch:
	.size InsertBranch, .func_end_InsertBranch-InsertBranch

	.local  FindBranch
	.type FindBranch, @function

FindBranch:

	// *** Basic block 0

	.global MapFindPointerKey
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 912
	j           MapFindPointerKey
.func_end_FindBranch:
	.size FindBranch, .func_end_FindBranch-FindBranch

	.local  FixupBranch
	.type FixupBranch, @function

FixupBranch:

	// *** Basic block 0

	.global StringEqualString
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
	ld          s1, 8(a0)
	mv          s2, a1
	ld          t0, 48(s1)
	sub         t1, t0, x0
	seqz        s3, t1
	bne         t0, x0, .FixupBranch_label_31

	// *** Basic block 1

	addi        a0, s1, 8
	ld          a1, 0(s2)
	call        StringEqualString

	// *** Basic block 2

	mv          s3, a0

	// *** Basic block 3

.FixupBranch_label_31:
	beqz        s3, .FixupBranch_label_41

	// *** Basic block 4

	sd          s2, 48(s1)
	addi        a0, s2, 8
	mv          a1, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorAppend

	// *** Basic block 5

.FixupBranch_label_41:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_FixupBranch:
	.size FixupBranch, .func_end_FixupBranch-FixupBranch

	.local  FixupBranches
	.type FixupBranches, @function

FixupBranches:

	// *** Basic block 0

	.global MapTraverse
	.local FixupBranch
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	addi        a0, t0, 912
	mv          a2, t1
	la          a1, FixupBranch
	j           MapTraverse
.func_end_FixupBranches:
	.size FixupBranches, .func_end_FixupBranches-FixupBranches

	.local  InsertLabel
	.type InsertLabel, @function

InsertLabel:

	// *** Basic block 0

	.local NewLabel
	.global MapInsert
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	mv          a1, a2
	mv          a0, s1
	call        NewLabel

	// *** Basic block 1

	mv          s3, a0
	sd          s1, -32(s0)
	addi        t0, s0, -32
	sd          s3, 8(t0)
	addi        a0, s2, 944
	addi        sp, sp, -16
	ld          t0, -32(s0)
	sd          t0, 0(sp)
	ld          t0, -24(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	call        MapInsert

	// *** Basic block 2

	mv          a0, s3

	// *** Basic block 3

.InsertLabel_label_46:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InsertLabel:
	.size InsertLabel, .func_end_InsertLabel-InsertLabel

	.local  FindLabel
	.type FindLabel, @function

FindLabel:

	// *** Basic block 0

	.global MapFindPointerKey
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 944
	j           MapFindPointerKey
.func_end_FindLabel:
	.size FindLabel, .func_end_FindLabel-FindLabel

	.local  CalculateBranchType
	.type CalculateBranchType, @function

CalculateBranchType:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 56(a0)
	ld          t1, 48(a0)
	ld          t1, 32(t1)
	ld          t1, 48(t1)
	sub         t2, t0, t1
	slti        t0, t2, -128
	li          t1, -128		// 0xffffffffffffff80
	blt         t2, t1, .CalculateBranchType_label_31

	// *** Basic block 1

	li          t1, 127		// 0x7f ASCII \x7f
	slt         t0, t1, t2

	// *** Basic block 2

.CalculateBranchType_label_31:
	beqz        t0, .CalculateBranchType_label_36

	// *** Basic block 3

	li          a0, 1		// 0x1 ASCII \x1
	j           .CalculateBranchType_label_38

	// *** Basic block 4

.CalculateBranchType_label_36:

	// *** Basic block 5

.CalculateBranchType_label_38:

	// *** Basic block 6

.CalculateBranchType_label_40:
	ret         
.func_end_CalculateBranchType:
	.size CalculateBranchType, .func_end_CalculateBranchType-CalculateBranchType

	.local  DefineLabel
	.type DefineLabel, @function

DefineLabel:

	// *** Basic block 0

	.local FindLabel
	.local InsertLabel
	.global NewString
	.local FixupBranches
	.local CalculateBranchType
	.global AssemblerReset
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
	mv          s2, a1
	mv          s3, s1
	ld          t0, 976(s3)
	jalr         x1, t0, 0

	// *** Basic block 1

	mv          s4, a0
	mv          a1, s2
	mv          a0, s3
	call        FindLabel

	// *** Basic block 2

	mv          s5, a0
	bne         s5, x0, .DefineLabel_label_60

	// *** Basic block 3

	ld          a0, 16(s2)
	call        NewString

	// *** Basic block 4

	mv          a2, s4
	mv          a1, a0
	mv          a0, s3
	call        InsertLabel

	// *** Basic block 5

	mv          s5, a0
	j           .DefineLabel_label_63

	// *** Basic block 6

.DefineLabel_label_60:
	sd          s4, 32(s5)

	// *** Basic block 7

.DefineLabel_label_63:
	lw          t0, 736(s1)
	li          s6, 1		// 0x1 ASCII \x1
	bne         t0, s6, .DefineLabel_label_115

	// *** Basic block 8

	mv          a1, s5
	mv          a0, s3
	call        FixupBranches

	// *** Basic block 9

	mv          s7, x0
	addi        t0, s5, 8
	ld          s8, 8(t0)
	bge         x0, s8, .DefineLabel_label_114

	// *** Basic block 10

	ld          s9, 8(s5)

	// *** Basic block 11

.DefineLabel_label_84:
	slli        t0, s7, 3
	add         t0, s9, t0
	ld          s9, 0(t0)
	mv          a0, s9
	call        CalculateBranchType

	// *** Basic block 12

	mv          s10, a0
	lw          t0, 4(s9)
	beq         s10, t0, .DefineLabel_label_109

	// *** Basic block 13

	sw          s10, 4(s9)
	mv          a1, s6
	mv          a0, s1
	call        AssemblerReset

	// *** Basic block 14

	j           .DefineLabel_label_114

	// *** Basic block 15

.DefineLabel_label_109:

	// *** Basic block 16

.DefineLabel_label_110:
	addi        s7, s7, 1
	bge         s7, s8, .DefineLabel_label_84

	// *** Basic block 17

.DefineLabel_label_114:

	// *** Basic block 18

.DefineLabel_label_115:
	mv          a0, s4

	// *** Basic block 19

.DefineLabel_label_118:
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
.func_end_DefineLabel:
	.size DefineLabel, .func_end_DefineLabel-DefineLabel

	.local  AssemblerFunction
	.type AssemblerFunction, @function

AssemblerFunction:

	// *** Basic block 0

	.global LexLookingAt
	.global StringSet
	.global LexNextToken
	.global LexMatch
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
	mv          s3, a2
	addi        a0, s1, 176
	li          s4, 3		// 0x3 ASCII \x3
	mv          a1, s4
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .AssemblerFunction_label_116

	// *** Basic block 2

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s2
	call        StringSet

	// *** Basic block 3

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 4

	addi        a0, s1, 176
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .AssemblerFunction_label_106

	// *** Basic block 6

	addi        a0, s1, 176
	mv          a1, s4
	call        LexLookingAt

	// *** Basic block 7

	beqz        a0, .AssemblerFunction_label_96

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s3
	call        StringSet

	// *** Basic block 9

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

	addi        a0, s1, 176
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 11

	not         t0, a0
	beqz        t0, .AssemblerFunction_label_89

	// *** Basic block 12

	lla         a1, .str.67
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 13

	mv          a0, x0

	// *** Basic block 14

.AssemblerFunction_label_86:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 15

.AssemblerFunction_label_89:

	// *** Basic block 16

.AssemblerFunction_label_90:

	// *** Basic block 17

.AssemblerFunction_label_91:

	// *** Basic block 18

.AssemblerFunction_label_92:
	li          a0, 1		// 0x1 ASCII \x1
	j           .AssemblerFunction_label_86

	// *** Basic block 19

.AssemblerFunction_label_96:
	lla         a1, .str.68
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 20

	mv          a0, x0
	j           .AssemblerFunction_label_86

	// *** Basic block 21

.AssemblerFunction_label_106:
	lla         a1, .str.69
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 22

	mv          a0, x0
	j           .AssemblerFunction_label_86

	// *** Basic block 23

.AssemblerFunction_label_116:
	lla         a1, .str.70
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 24

	mv          a0, x0
	j           .AssemblerFunction_label_86
.func_end_AssemblerFunction:
	.size AssemblerFunction, .func_end_AssemblerFunction-AssemblerFunction

	.local  CheckWidth
	.type CheckWidth, @function

CheckWidth:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a2
	mv          t2, a0
	mv          t3, t0
	bge         t3, x0, .CheckWidth_label_22

	// *** Basic block 1

	neg         t3, t3

	// *** Basic block 2

.CheckWidth_label_22:
	li          t4, 1		// 0x1 ASCII \x1
	sll         t5, t4, t1
	addi        t4, t5, -1
	bge         t4, t3, .CheckWidth_label_42

	// *** Basic block 3

	lla         a1, .str.71
	mv          a3, t1
	mv          a2, t0
	mv          a0, t2
	j           AssemblerError

	// *** Basic block 4

.CheckWidth_label_42:
	ret         
.func_end_CheckWidth:
	.size CheckWidth, .func_end_CheckWidth-CheckWidth

	.local  CheckZeroPage
	.type CheckZeroPage, @function

CheckZeroPage:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	mv          t2, t0
	bge         t2, x0, .CheckZeroPage_label_19

	// *** Basic block 1

	neg         t2, t2

	// *** Basic block 2

.CheckZeroPage_label_19:
	li          t3, 255		// 0xff
	bge         t3, t2, .CheckZeroPage_label_34

	// *** Basic block 3

	lla         a1, .str.72
	mv          a2, t0
	mv          a0, t1
	j           AssemblerError

	// *** Basic block 4

.CheckZeroPage_label_34:
	ret         
.func_end_CheckZeroPage:
	.size CheckZeroPage, .func_end_CheckZeroPage-CheckZeroPage

	.local  NeedIndexReg
	.type NeedIndexReg, @function

NeedIndexReg:

	// *** Basic block 0

	.global LexLookingAt
	.global AssemblerError
	.global StringEqualCaseBlind
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
	addi        a0, s1, 176
	li          a1, 3		// 0x3 ASCII \x3
	call        LexLookingAt

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .NeedIndexReg_label_35

	// *** Basic block 2

	lla         a1, .str.73
	mv          a2, s2
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 3

	j           .NeedIndexReg_label_62

	// *** Basic block 4

.NeedIndexReg_label_35:
	addi        t0, s1, 176
	addi        a0, t0, 80
	mv          a1, s2
	call        StringEqualCaseBlind

	// *** Basic block 5

	not         t0, a0
	beqz        t0, .NeedIndexReg_label_57

	// *** Basic block 6

	lla         a1, .str.74
	addi        t0, s1, 176
	addi        a3, t0, 80
	mv          a2, s2
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 7

	j           .NeedIndexReg_label_61

	// *** Basic block 8

.NeedIndexReg_label_57:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 9

.NeedIndexReg_label_61:

	// *** Basic block 10

.NeedIndexReg_label_62:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NeedIndexReg:
	.size NeedIndexReg, .func_end_NeedIndexReg-NeedIndexReg

	.local  IsAccumulator
	.type IsAccumulator, @function

IsAccumulator:

	// *** Basic block 0

	.global LexLookingAt
	.global StringEqualCaseBlind
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
	addi        a0, s1, 176
	li          a1, 3		// 0x3 ASCII \x3
	call        LexLookingAt

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .IsAccumulator_label_28

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.IsAccumulator_label_25:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.IsAccumulator_label_28:
	addi        t0, s1, 176
	addi        a0, t0, 80
	lla         a1, .str.75
	call        StringEqualCaseBlind

	// *** Basic block 5

	not         t0, a0
	beqz        t0, .IsAccumulator_label_41

	// *** Basic block 6

	mv          a0, x0
	j           .IsAccumulator_label_25

	// *** Basic block 7

.IsAccumulator_label_41:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 8

	li          a0, 1		// 0x1 ASCII \x1
	j           .IsAccumulator_label_25
.func_end_IsAccumulator:
	.size IsAccumulator, .func_end_IsAccumulator-IsAccumulator

	.local  NeedCloseParenthesis
	.type NeedCloseParenthesis, @function

NeedCloseParenthesis:

	// *** Basic block 0

	.global LexMatch
	.global AssemblerError
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
	addi        a0, s1, 176
	li          a1, 45		// 0x2d ASCII '-'
	call        LexMatch

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .NeedCloseParenthesis_label_26

	// *** Basic block 2

	lla         a1, .str.76
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssemblerError

	// *** Basic block 3

.NeedCloseParenthesis_label_26:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NeedCloseParenthesis:
	.size NeedCloseParenthesis, .func_end_NeedCloseParenthesis-NeedCloseParenthesis

	.local  AssembleSingleByteInstruction
	.type AssembleSingleByteInstruction, @function

AssembleSingleByteInstruction:

	// *** Basic block 0

	.global AssemblerEmitByte
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	lw          a1, 744(t0)
	andi        a2, t1, 255
	j           AssemblerEmitByte
.func_end_AssembleSingleByteInstruction:
	.size AssembleSingleByteInstruction, .func_end_AssembleSingleByteInstruction-AssembleSingleByteInstruction

	.local  EmitBranchBinary
	.type EmitBranchBinary, @function

EmitBranchBinary:

	// *** Basic block 0

	.global printf
	.global abort
	.global AssemblerEmitByte
	.global NewAssemblerRelocation
	.global AssemblerCurrentAddress
	.global AssemblerAddRelocation
	.global AssemblerEmitHalf
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
	mv          s2, a0
	lw          t0, 4(s1)
	li          s3, 1		// 0x1 ASCII \x1
	bne         t0, s3, .EmitBranchBinary_label_218

	// *** Basic block 1

	ld          s4, 48(s1)
	beq         s4, x0, .EmitBranchBinary_label_56

	// *** Basic block 2

	j           .EmitBranchBinary_label_73

	// *** Basic block 3

.EmitBranchBinary_label_56:
	lla         a0, .str.77
	lla         a1, .str.78
	lla         a3, .str.79
	li          t0, 528		// 0x210
	mv          a2, t0
	call        printf

	// *** Basic block 4

	call        abort

	// *** Basic block 5

.EmitBranchBinary_label_73:
	lw          s5, 0(s1)
	li          t0, 16		// 0x10 ASCII \x10
	beq         s5, t0, .EmitBranchBinary_label_123

	// *** Basic block 6

	li          t0, 48		// 0x30 ASCII '0'
	beq         s5, t0, .EmitBranchBinary_label_127

	// *** Basic block 7

	li          t0, 80		// 0x50 ASCII 'P'
	beq         s5, t0, .EmitBranchBinary_label_130

	// *** Basic block 8

	li          t0, 112		// 0x70 ASCII 'p'
	beq         s5, t0, .EmitBranchBinary_label_133

	// *** Basic block 9

	li          t0, 128		// 0x80 ASCII \x80
	beq         s5, t0, .EmitBranchBinary_label_148

	// *** Basic block 10

	li          t0, 144		// 0x90 ASCII \x90
	beq         s5, t0, .EmitBranchBinary_label_136

	// *** Basic block 11

	li          t0, 176		// 0xb0 ASCII \xb0
	beq         s5, t0, .EmitBranchBinary_label_139

	// *** Basic block 12

	li          t0, 208		// 0xd0 ASCII \xd0
	beq         s5, t0, .EmitBranchBinary_label_142

	// *** Basic block 13

	li          t0, 240		// 0xf0 ASCII \xf0
	beq         s5, t0, .EmitBranchBinary_label_145

	// *** Basic block 14

.EmitBranchBinary_label_120:
	call        abort

	// *** Basic block 15

	j           .EmitBranchBinary_label_151

	// *** Basic block 16

.EmitBranchBinary_label_123:
	li          s5, 48		// 0x30 ASCII '0'
	j           .EmitBranchBinary_label_151

	// *** Basic block 17

.EmitBranchBinary_label_127:
	li          s5, 16		// 0x10 ASCII \x10
	j           .EmitBranchBinary_label_151

	// *** Basic block 18

.EmitBranchBinary_label_130:
	li          s5, 112		// 0x70 ASCII 'p'
	j           .EmitBranchBinary_label_151

	// *** Basic block 19

.EmitBranchBinary_label_133:
	li          s5, 80		// 0x50 ASCII 'P'
	j           .EmitBranchBinary_label_151

	// *** Basic block 20

.EmitBranchBinary_label_136:
	li          s5, 176		// 0xb0 ASCII \xb0
	j           .EmitBranchBinary_label_151

	// *** Basic block 21

.EmitBranchBinary_label_139:
	li          s5, 144		// 0x90 ASCII \x90
	j           .EmitBranchBinary_label_151

	// *** Basic block 22

.EmitBranchBinary_label_142:
	li          s5, 240		// 0xf0 ASCII \xf0
	j           .EmitBranchBinary_label_151

	// *** Basic block 23

.EmitBranchBinary_label_145:
	li          s5, 208		// 0xd0 ASCII \xd0
	j           .EmitBranchBinary_label_151

	// *** Basic block 24

.EmitBranchBinary_label_148:
	li          s5, 128		// 0x80 ASCII \x80
	j           .EmitBranchBinary_label_151

	// *** Basic block 25

.EmitBranchBinary_label_151:
	lw          a1, 744(s2)
	andi        a2, s5, 255
	mv          a0, s2
	call        AssemblerEmitByte

	// *** Basic block 26

	lw          s5, 744(s2)
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	mv          a1, s5
	mv          a0, s2
	call        AssemblerEmitByte

	// *** Basic block 27

	ld          s6, 32(s4)
	sb          s3, 72(s6)
	mv          a0, s2
	call        AssemblerCurrentAddress

	// *** Basic block 28

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s5
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s6
	call        NewAssemblerRelocation

	// *** Basic block 29

	mv          s3, a0
	mv          a1, s3
	mv          a0, s2
	call        AssemblerAddRelocation

	// *** Basic block 30

	lw          a1, 744(s2)
	li          t0, 76		// 0x4c ASCII 'L'
	mv          a2, t0
	mv          a0, s2
	call        AssemblerEmitByte

	// *** Basic block 31

	lw          a1, 744(s2)
	mv          a2, x0
	mv          a0, s2
	call        AssemblerEmitHalf

	// *** Basic block 32

	j           .EmitBranchBinary_label_256

	// *** Basic block 33

.EmitBranchBinary_label_218:
	lw          a1, 744(s2)
	lw          t0, 0(s1)
	andi        a2, t0, 255
	mv          a0, s2
	call        AssemblerEmitByte

	// *** Basic block 34

	ld          t0, 48(s1)
	bne         t0, x0, .EmitBranchBinary_label_236

	// *** Basic block 35

	j           .EmitBranchBinary_label_244

	// *** Basic block 36

.EmitBranchBinary_label_236:
	ld          t0, 32(t0)
	ld          t0, 48(t0)
	ld          t1, 56(s1)
	sub         s4, t0, t1

	// *** Basic block 37

.EmitBranchBinary_label_244:
	lw          a1, 744(s2)
	slli        t0, s4, 56
	srai        a2, t0, 56
	mv          a0, s2
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
	j           AssemblerEmitByte

	// *** Basic block 38

.EmitBranchBinary_label_256:
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
.func_end_EmitBranchBinary:
	.size EmitBranchBinary, .func_end_EmitBranchBinary-EmitBranchBinary

	.local  AssembleBranch
	.type AssembleBranch, @function

AssembleBranch:

	// *** Basic block 0

	.global LexLookingAt
	.global NewString
	.local FindBranch
	.local NewBranch
	.local InsertBranch
	.global AssemblerCurrentAddress
	.local FindLabel
	.local CalculateBranchType
	.global VectorAppend
	.global printf
	.global abort
	.global LexNextToken
	.local EmitBranchBinary
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
	addi        a0, s1, 176
	li          a1, 3		// 0x3 ASCII \x3
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .AssembleBranch_label_139

	// *** Basic block 2

	addi        t0, a0, 80
	ld          a0, 16(t0)
	call        NewString

	// *** Basic block 3

	mv          s4, a0
	ld          s5, 56(a0)
	mv          a1, s5
	mv          a0, s1
	call        FindBranch

	// *** Basic block 4

	mv          s3, a0
	lw          t0, 736(s1)
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .AssembleBranch_label_116

	// *** Basic block 5

	bne         s3, x0, .AssembleBranch_label_84

	// *** Basic block 6

	ld          a1, 16(s4)
	mv          a0, s2
	call        NewBranch

	// *** Basic block 7

	mv          s3, a0
	mv          a2, s3
	mv          a1, s5
	mv          a0, s1
	call        InsertBranch

	// *** Basic block 8

.AssembleBranch_label_84:
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 9

	sd          a0, 56(s3)
	mv          a1, s4
	mv          a0, s1
	call        FindLabel

	// *** Basic block 10

	mv          s5, a0
	beq         s5, x0, .AssembleBranch_label_113

	// *** Basic block 11

	sd          s5, 48(s3)
	mv          a0, s3
	call        CalculateBranchType

	// *** Basic block 12

	sw          a0, 4(s3)
	addi        a0, s5, 8
	mv          a1, s3
	call        VectorAppend

	// *** Basic block 13

	j           .AssembleBranch_label_114

	// *** Basic block 14

.AssembleBranch_label_113:

	// *** Basic block 15

.AssembleBranch_label_114:
	j           .AssembleBranch_label_138

	// *** Basic block 16

.AssembleBranch_label_116:
	beq         s3, x0, .AssembleBranch_label_121

	// *** Basic block 17

	j           .AssembleBranch_label_137

	// *** Basic block 18

.AssembleBranch_label_121:
	lla         a0, .str.80
	lla         a1, .str.81
	lla         a3, .str.82
	li          t0, 605		// 0x25d
	mv          a2, t0
	call        printf

	// *** Basic block 19

	call        abort

	// *** Basic block 20

.AssembleBranch_label_137:

	// *** Basic block 21

.AssembleBranch_label_138:

	// *** Basic block 22

.AssembleBranch_label_139:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 23

	mv          a1, s3
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
	j           EmitBranchBinary
.func_end_AssembleBranch:
	.size AssembleBranch, .func_end_AssembleBranch-AssembleBranch

	.local  AssembleJump
	.type AssembleJump, @function

AssembleJump:

	// *** Basic block 0

	.global LexMatch
	.global LexLookingAt
	.global AssemblerError
	.global AssemblerFindSymbol
	.global NewAssemblerSymbol
	.global AssemblerInsertSymbol
	.global LexNextToken
	.global NewAssemblerRelocation
	.global AssemblerCurrentAddress
	.global AssemblerAddRelocation
	.local NeedIndexReg
	.global AssemblerEmitByte
	.global AssemblerEmitHalf
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
	li          s2, 64		// 0x40 ASCII '@'
	addi        a0, s1, 176
	li          a1, 29		// 0x1d ASCII \x1d
	call        LexMatch

	// *** Basic block 1

	beqz        a0, .AssembleJump_label_51

	// *** Basic block 2

	li          s2, 96		// 0x60 ASCII '`'

	// *** Basic block 3

.AssembleJump_label_51:
	ori         s2, s2, 12
	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 4

	not         t0, a0
	beqz        t0, .AssembleJump_label_71

	// *** Basic block 5

	lla         a1, .str.83
	mv          a0, s1
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
	j           AssemblerError

	// *** Basic block 6

.AssembleJump_label_68:
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

	// *** Basic block 7

.AssembleJump_label_71:
	addi        t0, s1, 176
	addi        t0, t0, 80
	ld          s3, 16(t0)
	mv          a1, s3
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 8

	mv          s4, a0
	bne         s4, x0, .AssembleJump_label_110

	// *** Basic block 9

	lw          a1, 744(s1)
	mv          a4, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, t0
	mv          a0, s3
	call        NewAssemblerSymbol

	// *** Basic block 10

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        AssemblerInsertSymbol

	// *** Basic block 11

.AssembleJump_label_110:
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 72(s4)
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 12

	lw          s3, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 13

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s3
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s4
	call        NewAssemblerRelocation

	// *** Basic block 14

	mv          s3, a0
	mv          a1, s3
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 15

	addi        t0, s2, -96
	seqz        s5, t0
	li          s6, 96		// 0x60 ASCII '`'
	bne         s2, s6, .AssembleJump_label_154

	// *** Basic block 16

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 17

	mv          s5, a0

	// *** Basic block 18

.AssembleJump_label_154:
	beqz        s5, .AssembleJump_label_163

	// *** Basic block 19

	li          s2, 124		// 0x7c ASCII '|'
	lla         a1, .str.84
	mv          a0, s1
	call        NeedIndexReg

	// *** Basic block 20

.AssembleJump_label_163:
	lw          a1, 744(s1)
	andi        a2, s2, 255
	mv          a0, s1
	call        AssemblerEmitByte

	// *** Basic block 21

	lw          a1, 744(s1)
	mv          a2, x0
	mv          a0, s1
	call        AssemblerEmitHalf

	// *** Basic block 22

	bne         s2, s6, .AssembleJump_label_200

	// *** Basic block 23

	addi        a0, s1, 176
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 24

	not         t0, a0
	beqz        t0, .AssembleJump_label_199

	// *** Basic block 25

	lla         a1, .str.85
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 26

.AssembleJump_label_199:

	// *** Basic block 27

.AssembleJump_label_200:
	j           .AssembleJump_label_68
.func_end_AssembleJump:
	.size AssembleJump, .func_end_AssembleJump-AssembleJump

	.local  AssembleAbsouteAddress
	.type AssembleAbsouteAddress, @function

AssembleAbsouteAddress:

	// *** Basic block 0

	.global StringInit
	.local AssemblerFunction
	.local GetOrCreateSymbol
	.global StringEqual
	.local assembler_functions
	.global AssemblerError
	.global NewAssemblerRelocation
	.global AssemblerCurrentAddress
	.global AssemblerAddRelocation
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
	addi        a0, s0, -96
	lla         a1, .str.94
	call        StringInit

	// *** Basic block 1

	addi        a0, s0, -56
	lla         a1, .str.95
	call        StringInit

	// *** Basic block 2

	addi        a1, s0, -96
	addi        a2, s0, -56
	mv          a0, s1
	call        AssemblerFunction

	// *** Basic block 3

	mv          s2, a0
	not         t0, s2
	bnez        t0, .AssembleAbsouteAddress_label_134

	// *** Basic block 4

.AssembleAbsouteAddress_label_56:
	addi        t0, s0, -56
	ld          a1, 16(t0)
	mv          a0, s1
	call        GetOrCreateSymbol

	// *** Basic block 5

	mv          s2, a0
	li          s3, -1		// 0xffffffffffffffff
	mv          s4, x0

	// *** Basic block 6

.AssembleAbsouteAddress_label_72:
	addi        a0, s0, -96
	slli        t0, s4, 4
	la          t1, assembler_functions
	add         s5, t1, t0
	ld          a1, 0(s5)
	call        StringEqual

	// *** Basic block 7

	beqz        a0, .AssembleAbsouteAddress_label_86

	// *** Basic block 8

	lw          s3, 8(s5)
	j           .AssembleAbsouteAddress_label_92

	// *** Basic block 9

.AssembleAbsouteAddress_label_86:

	// *** Basic block 10

.AssembleAbsouteAddress_label_87:
	addi        s4, s4, 1
	li          t0, 8		// 0x8 ASCII \x8
	bge         s4, t0, .AssembleAbsouteAddress_label_72

	// *** Basic block 11

.AssembleAbsouteAddress_label_92:
	li          t0, -1		// 0xffffffffffffffff
	bne         s3, t0, .AssembleAbsouteAddress_label_108

	// *** Basic block 12

	lla         a1, .str.96
	addi        t0, s0, -96
	ld          a2, 16(t0)
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 13

	j           .AssembleAbsouteAddress_label_134

	// *** Basic block 14

.AssembleAbsouteAddress_label_108:
	lw          s5, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 15

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s5
	mv          a1, s3
	mv          a0, s2
	call        NewAssemblerRelocation

	// *** Basic block 16

	mv          s3, a0
	mv          a1, s3
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 17

.AssembleAbsouteAddress_label_134:
	addi        a0, s0, -96
	call        StringDestruct

	// *** Basic block 18

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 19

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
.func_end_AssembleAbsouteAddress:
	.size AssembleAbsouteAddress, .func_end_AssembleAbsouteAddress-AssembleAbsouteAddress

	.local  AssembleMemoryInstruction
	.type AssembleMemoryInstruction, @function

AssembleMemoryInstruction:

	// *** Basic block 0

	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local CheckWidth
	.local AssembleAbsouteAddress
	.local NeedIndexReg
	.local CheckZeroPage
	.local NeedCloseParenthesis
	.global LexLookingAt
	.global StringEqualCaseBlind
	.global LexNextToken
	.local IsAccumulator
	.global printf
	.global abort
	.global AssemblerEmitByte
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
	mv          s1, a1
	mv          s2, a0
	mv          s3, x0
	andi        s4, s1, 3
	li          s5, 1		// 0x1 ASCII \x1
	mv          s6, x0
	addi        a0, s2, 176
	li          a1, 97		// 0x61 ASCII 'a'
	call        LexMatch

	// *** Basic block 1

	beqz        a0, .AssembleMemoryInstruction_label_179

	// *** Basic block 2

	blt         s4, x0, .AssembleMemoryInstruction_label_162

	// *** Basic block 3

	li          t0, 2		// 0x2 ASCII \x2
	blt         t0, s4, .AssembleMemoryInstruction_label_162

	// *** Basic block 4

	slli        t0, s4, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 5

	j           .AssembleMemoryInstruction_label_105

	// *** Basic block 6

	j           .AssembleMemoryInstruction_label_132

	// *** Basic block 7

	j           .AssembleMemoryInstruction_label_147

	// *** Basic block 8

.AssembleMemoryInstruction_label_105:
	mv          s3, x0
	addi        t1, s1, -160
	snez        t0, t1
	li          t1, 160		// 0xa0 ASCII \xa0
	beq         s1, t1, .AssembleMemoryInstruction_label_117

	// *** Basic block 9

	addi        t1, s1, -192
	snez        t0, t1

	// *** Basic block 10

.AssembleMemoryInstruction_label_117:
	beqz        t0, .AssembleMemoryInstruction_label_122

	// *** Basic block 11

	addi        t1, s1, -224
	snez        t0, t1

	// *** Basic block 12

.AssembleMemoryInstruction_label_122:
	beqz        t0, .AssembleMemoryInstruction_label_130

	// *** Basic block 13

	lla         a1, .str.97
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 14

.AssembleMemoryInstruction_label_130:
	j           .AssembleMemoryInstruction_label_162

	// *** Basic block 15

.AssembleMemoryInstruction_label_132:
	li          s3, 2		// 0x2 ASCII \x2
	li          t0, 129		// 0x81 ASCII \x81
	bne         s1, t0, .AssembleMemoryInstruction_label_145

	// *** Basic block 16

	lla         a1, .str.98
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 17

.AssembleMemoryInstruction_label_145:
	j           .AssembleMemoryInstruction_label_162

	// *** Basic block 18

.AssembleMemoryInstruction_label_147:
	mv          s3, x0
	li          t0, 162		// 0xa2 ASCII \xa2
	beq         s1, t0, .AssembleMemoryInstruction_label_160

	// *** Basic block 19

	lla         a1, .str.99
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 20

.AssembleMemoryInstruction_label_160:
	j           .AssembleMemoryInstruction_label_162

	// *** Basic block 21

.AssembleMemoryInstruction_label_162:
	mv          a0, s2
	call        AssemblerEvaluateExpression

	// *** Basic block 22

	mv          s7, a0
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	mv          a1, s7
	mv          a0, s2
	call        CheckWidth

	// *** Basic block 23

	sext.w      s6, s7
	j           .AssembleMemoryInstruction_label_623

	// *** Basic block 24

.AssembleMemoryInstruction_label_179:
	blt         s4, x0, .AssembleMemoryInstruction_label_606

	// *** Basic block 25

	li          s8, 2		// 0x2 ASCII \x2
	blt         s8, s4, .AssembleMemoryInstruction_label_606

	// *** Basic block 26

	slli        t0, s4, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 27

	j           .AssembleMemoryInstruction_label_193

	// *** Basic block 28

	j           .AssembleMemoryInstruction_label_282

	// *** Basic block 29

	j           .AssembleMemoryInstruction_label_452

	// *** Basic block 30

.AssembleMemoryInstruction_label_193:
	addi        a0, s2, 176
	li          t0, 38		// 0x26 ASCII '&'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 31

	beqz        a0, .AssembleMemoryInstruction_label_205

	// *** Basic block 32

	mv          a0, s2
	call        AssembleAbsouteAddress

	// *** Basic block 33

	j           .AssembleMemoryInstruction_label_210

	// *** Basic block 34

.AssembleMemoryInstruction_label_205:
	mv          a0, s2
	call        AssemblerEvaluateExpression

	// *** Basic block 35

	sext.w      s6, a0

	// *** Basic block 36

.AssembleMemoryInstruction_label_210:
	addi        a0, s2, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 37

	beqz        a0, .AssembleMemoryInstruction_label_266

	// *** Basic block 38

	lla         a1, .str.100
	mv          a0, s2
	call        NeedIndexReg

	// *** Basic block 39

	sltz        t1, s6
	not         t0, t1
	blt         s6, x0, .AssembleMemoryInstruction_label_229

	// *** Basic block 40

	slti        t0, s6, 256

	// *** Basic block 41

.AssembleMemoryInstruction_label_229:
	beqz        t0, .AssembleMemoryInstruction_label_250

	// *** Basic block 42

	addi        t1, s1, -128
	snez        t0, t1
	li          t1, 128		// 0x80 ASCII \x80
	beq         s1, t1, .AssembleMemoryInstruction_label_239

	// *** Basic block 43

	addi        t1, s1, -160
	snez        t0, t1

	// *** Basic block 44

.AssembleMemoryInstruction_label_239:
	beqz        t0, .AssembleMemoryInstruction_label_247

	// *** Basic block 45

	lla         a1, .str.101
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 46

.AssembleMemoryInstruction_label_247:
	li          s3, 5		// 0x5 ASCII \x5
	j           .AssembleMemoryInstruction_label_264

	// *** Basic block 47

.AssembleMemoryInstruction_label_250:
	li          t0, 160		// 0xa0 ASCII \xa0
	beq         s1, t0, .AssembleMemoryInstruction_label_261

	// *** Basic block 48

	lla         a1, .str.102
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 49

.AssembleMemoryInstruction_label_261:
	li          s3, 7		// 0x7 ASCII \x7
	li          s5, 2		// 0x2 ASCII \x2

	// *** Basic block 50

.AssembleMemoryInstruction_label_264:
	j           .AssembleMemoryInstruction_label_280

	// *** Basic block 51

.AssembleMemoryInstruction_label_266:
	sltz        t1, s6
	not         t0, t1
	blt         s6, x0, .AssembleMemoryInstruction_label_272

	// *** Basic block 52

	slti        t0, s6, 256

	// *** Basic block 53

.AssembleMemoryInstruction_label_272:
	beqz        t0, .AssembleMemoryInstruction_label_276

	// *** Basic block 54

	li          s3, 1		// 0x1 ASCII \x1
	j           .AssembleMemoryInstruction_label_279

	// *** Basic block 55

.AssembleMemoryInstruction_label_276:
	li          s3, 3		// 0x3 ASCII \x3
	li          s5, 2		// 0x2 ASCII \x2

	// *** Basic block 56

.AssembleMemoryInstruction_label_279:

	// *** Basic block 57

.AssembleMemoryInstruction_label_280:
	j           .AssembleMemoryInstruction_label_622

	// *** Basic block 58

.AssembleMemoryInstruction_label_282:
	addi        a0, s2, 176
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	call        LexMatch

	// *** Basic block 59

	beqz        a0, .AssembleMemoryInstruction_label_354

	// *** Basic block 60

	mv          a0, s2
	call        AssemblerEvaluateExpression

	// *** Basic block 61

	mv          s4, a0
	mv          a1, s4
	mv          a0, s2
	call        CheckZeroPage

	// *** Basic block 62

	addi        a0, s2, 176
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 63

	beqz        a0, .AssembleMemoryInstruction_label_332

	// *** Basic block 64

	li          s3, 4		// 0x4 ASCII \x4
	addi        a0, s2, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 65

	not         t0, a0
	beqz        t0, .AssembleMemoryInstruction_label_323

	// *** Basic block 66

	lla         a1, .str.103
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 67

	j           .AssembleMemoryInstruction_label_330

	// *** Basic block 68

.AssembleMemoryInstruction_label_323:
	lla         a1, .str.104
	mv          a0, s2
	call        NeedIndexReg

	// *** Basic block 69

.AssembleMemoryInstruction_label_330:
	j           .AssembleMemoryInstruction_label_351

	// *** Basic block 70

.AssembleMemoryInstruction_label_332:
	addi        a0, s2, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 71

	beqz        a0, .AssembleMemoryInstruction_label_350

	// *** Basic block 72

	mv          s3, x0
	lla         a1, .str.105
	mv          a0, s2
	call        NeedIndexReg

	// *** Basic block 73

	mv          a0, s2
	call        NeedCloseParenthesis

	// *** Basic block 74

.AssembleMemoryInstruction_label_350:

	// *** Basic block 75

.AssembleMemoryInstruction_label_351:
	sext.w      s6, s4
	j           .AssembleMemoryInstruction_label_450

	// *** Basic block 76

.AssembleMemoryInstruction_label_354:
	addi        a0, s2, 176
	li          t0, 38		// 0x26 ASCII '&'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 77

	beqz        a0, .AssembleMemoryInstruction_label_366

	// *** Basic block 78

	mv          a0, s2
	call        AssembleAbsouteAddress

	// *** Basic block 79

	j           .AssembleMemoryInstruction_label_379

	// *** Basic block 80

.AssembleMemoryInstruction_label_366:
	mv          a0, s2
	call        AssemblerEvaluateExpression

	// *** Basic block 81

	sext.w      s6, a0
	li          t0, 16		// 0x10 ASCII \x10
	mv          a2, t0
	mv          a1, s6
	mv          a0, s2
	call        CheckWidth

	// *** Basic block 82

.AssembleMemoryInstruction_label_379:
	addi        a0, s2, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 83

	beqz        a0, .AssembleMemoryInstruction_label_435

	// *** Basic block 84

	addi        a0, s2, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 85

	beqz        a0, .AssembleMemoryInstruction_label_433

	// *** Basic block 86

	sltz        t0, s6
	not         s9, t0
	blt         s6, x0, .AssembleMemoryInstruction_label_399

	// *** Basic block 87

	slti        s9, s6, 256

	// *** Basic block 88

.AssembleMemoryInstruction_label_399:
	li          s5, 2		// 0x2 ASCII \x2
	addi        t0, s2, 176
	addi        a0, t0, 80
	lla         a1, .str.106
	call        StringEqualCaseBlind

	// *** Basic block 89

	beqz        a0, .AssembleMemoryInstruction_label_418

	// *** Basic block 90

	beqz        s9, .AssembleMemoryInstruction_label_414

	// *** Basic block 91

	li          s3, 5		// 0x5 ASCII \x5
	li          s5, 1		// 0x1 ASCII \x1
	j           .AssembleMemoryInstruction_label_416

	// *** Basic block 92

.AssembleMemoryInstruction_label_414:
	li          s3, 7		// 0x7 ASCII \x7

	// *** Basic block 93

.AssembleMemoryInstruction_label_416:
	j           .AssembleMemoryInstruction_label_429

	// *** Basic block 94

.AssembleMemoryInstruction_label_418:
	addi        t0, s2, 176
	addi        a0, t0, 80
	lla         a1, .str.107
	call        StringEqualCaseBlind

	// *** Basic block 95

	beqz        a0, .AssembleMemoryInstruction_label_428

	// *** Basic block 96

	li          s3, 6		// 0x6 ASCII \x6

	// *** Basic block 97

.AssembleMemoryInstruction_label_428:

	// *** Basic block 98

.AssembleMemoryInstruction_label_429:
	addi        a0, s2, 176
	call        LexNextToken

	// *** Basic block 99

.AssembleMemoryInstruction_label_433:
	j           .AssembleMemoryInstruction_label_449

	// *** Basic block 100

.AssembleMemoryInstruction_label_435:
	sltz        t1, s6
	not         t0, t1
	blt         s6, x0, .AssembleMemoryInstruction_label_441

	// *** Basic block 101

	slti        t0, s6, 256

	// *** Basic block 102

.AssembleMemoryInstruction_label_441:
	beqz        t0, .AssembleMemoryInstruction_label_445

	// *** Basic block 103

	li          s3, 1		// 0x1 ASCII \x1
	j           .AssembleMemoryInstruction_label_448

	// *** Basic block 104

.AssembleMemoryInstruction_label_445:
	li          s3, 3		// 0x3 ASCII \x3
	li          s5, 2		// 0x2 ASCII \x2

	// *** Basic block 105

.AssembleMemoryInstruction_label_448:

	// *** Basic block 106

.AssembleMemoryInstruction_label_449:

	// *** Basic block 107

.AssembleMemoryInstruction_label_450:
	j           .AssembleMemoryInstruction_label_622

	// *** Basic block 108

.AssembleMemoryInstruction_label_452:
	addi        a0, s2, 176
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	call        LexMatch

	// *** Basic block 109

	beqz        a0, .AssembleMemoryInstruction_label_474

	// *** Basic block 110

	li          s3, 4		// 0x4 ASCII \x4
	mv          a0, s2
	call        AssemblerEvaluateExpression

	// *** Basic block 111

	sext.w      s6, a0
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	mv          a1, s6
	mv          a0, s2
	call        CheckWidth

	// *** Basic block 112

	j           .AssembleMemoryInstruction_label_604

	// *** Basic block 113

.AssembleMemoryInstruction_label_474:
	mv          a0, s2
	call        IsAccumulator

	// *** Basic block 114

	beqz        a0, .AssembleMemoryInstruction_label_528

	// *** Basic block 115

	li          s3, 2		// 0x2 ASCII \x2
	beq         s1, s8, .AssembleMemoryInstruction_label_518

	// *** Basic block 116

	li          t0, 34		// 0x22 ASCII '"'
	beq         s1, t0, .AssembleMemoryInstruction_label_519

	// *** Basic block 117

	li          t0, 66		// 0x42 ASCII 'B'
	beq         s1, t0, .AssembleMemoryInstruction_label_520

	// *** Basic block 118

	li          t0, 98		// 0x62 ASCII 'b'
	beq         s1, t0, .AssembleMemoryInstruction_label_521

	// *** Basic block 119

	li          t0, 194		// 0xc2 ASCII \xc2
	beq         s1, t0, .AssembleMemoryInstruction_label_523

	// *** Basic block 120

	li          t0, 226		// 0xe2 ASCII \xe2
	beq         s1, t0, .AssembleMemoryInstruction_label_522

	// *** Basic block 121

.AssembleMemoryInstruction_label_510:
	lla         a1, .str.108
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 122

	j           .AssembleMemoryInstruction_label_525

	// *** Basic block 123

.AssembleMemoryInstruction_label_518:

	// *** Basic block 124

.AssembleMemoryInstruction_label_519:

	// *** Basic block 125

.AssembleMemoryInstruction_label_520:

	// *** Basic block 126

.AssembleMemoryInstruction_label_521:

	// *** Basic block 127

.AssembleMemoryInstruction_label_522:

	// *** Basic block 128

.AssembleMemoryInstruction_label_523:
	j           .AssembleMemoryInstruction_label_525

	// *** Basic block 129

.AssembleMemoryInstruction_label_525:
	mv          s5, x0
	j           .AssembleMemoryInstruction_label_603

	// *** Basic block 130

.AssembleMemoryInstruction_label_528:
	addi        a0, s2, 176
	li          t0, 38		// 0x26 ASCII '&'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 131

	beqz        a0, .AssembleMemoryInstruction_label_540

	// *** Basic block 132

	mv          a0, s2
	call        AssembleAbsouteAddress

	// *** Basic block 133

	j           .AssembleMemoryInstruction_label_545

	// *** Basic block 134

.AssembleMemoryInstruction_label_540:
	mv          a0, s2
	call        AssemblerEvaluateExpression

	// *** Basic block 135

	sext.w      s6, a0

	// *** Basic block 136

.AssembleMemoryInstruction_label_545:
	addi        a0, s2, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 137

	beqz        a0, .AssembleMemoryInstruction_label_588

	// *** Basic block 138

	lla         s8, .str.109
	addi        t1, s1, -162
	seqz        t0, t1
	li          t1, 162		// 0xa2 ASCII \xa2
	beq         s1, t1, .AssembleMemoryInstruction_label_564

	// *** Basic block 139

	addi        t1, s1, -130
	seqz        t0, t1

	// *** Basic block 140

.AssembleMemoryInstruction_label_564:
	beqz        t0, .AssembleMemoryInstruction_label_568

	// *** Basic block 141

	lla         s8, .str.110

	// *** Basic block 142

.AssembleMemoryInstruction_label_568:
	mv          a1, s8
	mv          a0, s2
	call        NeedIndexReg

	// *** Basic block 143

	sltz        t1, s6
	not         t0, t1
	blt         s6, x0, .AssembleMemoryInstruction_label_579

	// *** Basic block 144

	slti        t0, s6, 256

	// *** Basic block 145

.AssembleMemoryInstruction_label_579:
	beqz        t0, .AssembleMemoryInstruction_label_583

	// *** Basic block 146

	li          s3, 5		// 0x5 ASCII \x5
	j           .AssembleMemoryInstruction_label_586

	// *** Basic block 147

.AssembleMemoryInstruction_label_583:
	li          s3, 7		// 0x7 ASCII \x7
	li          s5, 2		// 0x2 ASCII \x2

	// *** Basic block 148

.AssembleMemoryInstruction_label_586:
	j           .AssembleMemoryInstruction_label_602

	// *** Basic block 149

.AssembleMemoryInstruction_label_588:
	sltz        t1, s6
	not         t0, t1
	blt         s6, x0, .AssembleMemoryInstruction_label_594

	// *** Basic block 150

	slti        t0, s6, 256

	// *** Basic block 151

.AssembleMemoryInstruction_label_594:
	beqz        t0, .AssembleMemoryInstruction_label_598

	// *** Basic block 152

	li          s3, 1		// 0x1 ASCII \x1
	j           .AssembleMemoryInstruction_label_601

	// *** Basic block 153

.AssembleMemoryInstruction_label_598:
	li          s3, 3		// 0x3 ASCII \x3
	li          s5, 2		// 0x2 ASCII \x2

	// *** Basic block 154

.AssembleMemoryInstruction_label_601:

	// *** Basic block 155

.AssembleMemoryInstruction_label_602:

	// *** Basic block 156

.AssembleMemoryInstruction_label_603:

	// *** Basic block 157

.AssembleMemoryInstruction_label_604:
	j           .AssembleMemoryInstruction_label_622

	// *** Basic block 158

.AssembleMemoryInstruction_label_606:
	lla         a0, .str.111
	lla         a1, .str.112
	lla         a3, .str.113
	li          t0, 906		// 0x38a
	mv          a2, t0
	call        printf

	// *** Basic block 159

	call        abort

	// *** Basic block 160

.AssembleMemoryInstruction_label_622:

	// *** Basic block 161

.AssembleMemoryInstruction_label_623:
	slli        t0, s3, 2
	or          s1, s1, t0
	lw          a1, 744(s2)
	andi        a2, s1, 255
	mv          a0, s2
	call        AssemblerEmitByte

	// *** Basic block 162

	blt         s5, x0, .AssembleMemoryInstruction_label_648

	// *** Basic block 163

	li          t0, 2		// 0x2 ASCII \x2
	blt         t0, s5, .AssembleMemoryInstruction_label_648

	// *** Basic block 164

	slli        t0, s5, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 165

	j           .AssembleMemoryInstruction_label_664

	// *** Basic block 166

	j           .AssembleMemoryInstruction_label_676

	// *** Basic block 167

	j           .AssembleMemoryInstruction_label_666

	// *** Basic block 168

.AssembleMemoryInstruction_label_648:
	lla         a0, .str.114
	lla         a1, .str.115
	lla         a3, .str.116
	li          t0, 915		// 0x393
	mv          a2, t0
	call        printf

	// *** Basic block 169

	call        abort

	// *** Basic block 170

	j           .AssembleMemoryInstruction_label_686

	// *** Basic block 171

.AssembleMemoryInstruction_label_664:
	j           .AssembleMemoryInstruction_label_686

	// *** Basic block 172

.AssembleMemoryInstruction_label_666:
	lw          a1, 744(s2)
	andi        a2, s6, 255
	mv          a0, s2
	call        AssemblerEmitByte

	// *** Basic block 173

	srai        s6, s6, 8

	// *** Basic block 174

.AssembleMemoryInstruction_label_676:
	lw          a1, 744(s2)
	andi        a2, s6, 255
	mv          a0, s2
	call        AssemblerEmitByte

	// *** Basic block 175

	j           .AssembleMemoryInstruction_label_686

	// *** Basic block 176

.AssembleMemoryInstruction_label_686:
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
.func_end_AssembleMemoryInstruction:
	.size AssembleMemoryInstruction, .func_end_AssembleMemoryInstruction-AssembleMemoryInstruction

	.local  Assemble_brk
	.type Assemble_brk, @function

Assemble_brk:

	// *** Basic block 0

	.global LexMatch
	.global AssemblerEvaluateExpression
	.global AssemblerEmitByte
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
	addi        a0, s1, 176
	li          a1, 97		// 0x61 ASCII 'a'
	call        LexMatch

	// *** Basic block 1

	beqz        a0, .Assemble_brk_label_31

	// *** Basic block 2

	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 3

	mv          s2, a0

	// *** Basic block 4

.Assemble_brk_label_31:
	lw          a1, 744(s1)
	mv          a2, x0
	mv          a0, s1
	call        AssemblerEmitByte

	// *** Basic block 5

	lw          a1, 744(s1)
	mv          a2, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssemblerEmitByte
.func_end_Assemble_brk:
	.size Assemble_brk, .func_end_Assemble_brk-Assemble_brk

	.local  Assemble_rti
	.type Assemble_rti, @function

Assemble_rti:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 64		// 0x40 ASCII '@'
	j           AssembleSingleByteInstruction
.func_end_Assemble_rti:
	.size Assemble_rti, .func_end_Assemble_rti-Assemble_rti

	.local  Assemble_rts
	.type Assemble_rts, @function

Assemble_rts:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 96		// 0x60 ASCII '`'
	j           AssembleSingleByteInstruction
.func_end_Assemble_rts:
	.size Assemble_rts, .func_end_Assemble_rts-Assemble_rts

	.local  Assemble_php
	.type Assemble_php, @function

Assemble_php:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 8		// 0x8 ASCII \x8
	j           AssembleSingleByteInstruction
.func_end_Assemble_php:
	.size Assemble_php, .func_end_Assemble_php-Assemble_php

	.local  Assemble_plp
	.type Assemble_plp, @function

Assemble_plp:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 40		// 0x28 ASCII '('
	j           AssembleSingleByteInstruction
.func_end_Assemble_plp:
	.size Assemble_plp, .func_end_Assemble_plp-Assemble_plp

	.local  Assemble_pha
	.type Assemble_pha, @function

Assemble_pha:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 72		// 0x48 ASCII 'H'
	j           AssembleSingleByteInstruction
.func_end_Assemble_pha:
	.size Assemble_pha, .func_end_Assemble_pha-Assemble_pha

	.local  Assemble_pla
	.type Assemble_pla, @function

Assemble_pla:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 104		// 0x68 ASCII 'h'
	j           AssembleSingleByteInstruction
.func_end_Assemble_pla:
	.size Assemble_pla, .func_end_Assemble_pla-Assemble_pla

	.local  Assemble_dey
	.type Assemble_dey, @function

Assemble_dey:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 136		// 0x88 ASCII \x88
	j           AssembleSingleByteInstruction
.func_end_Assemble_dey:
	.size Assemble_dey, .func_end_Assemble_dey-Assemble_dey

	.local  Assemble_tay
	.type Assemble_tay, @function

Assemble_tay:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 168		// 0xa8 ASCII \xa8
	j           AssembleSingleByteInstruction
.func_end_Assemble_tay:
	.size Assemble_tay, .func_end_Assemble_tay-Assemble_tay

	.local  Assemble_iny
	.type Assemble_iny, @function

Assemble_iny:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 200		// 0xc8 ASCII \xc8
	j           AssembleSingleByteInstruction
.func_end_Assemble_iny:
	.size Assemble_iny, .func_end_Assemble_iny-Assemble_iny

	.local  Assemble_inx
	.type Assemble_inx, @function

Assemble_inx:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 232		// 0xe8 ASCII \xe8
	j           AssembleSingleByteInstruction
.func_end_Assemble_inx:
	.size Assemble_inx, .func_end_Assemble_inx-Assemble_inx

	.local  Assemble_clc
	.type Assemble_clc, @function

Assemble_clc:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 24		// 0x18 ASCII \x18
	j           AssembleSingleByteInstruction
.func_end_Assemble_clc:
	.size Assemble_clc, .func_end_Assemble_clc-Assemble_clc

	.local  Assemble_sec
	.type Assemble_sec, @function

Assemble_sec:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 56		// 0x38 ASCII '8'
	j           AssembleSingleByteInstruction
.func_end_Assemble_sec:
	.size Assemble_sec, .func_end_Assemble_sec-Assemble_sec

	.local  Assemble_cli
	.type Assemble_cli, @function

Assemble_cli:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 88		// 0x58 ASCII 'X'
	j           AssembleSingleByteInstruction
.func_end_Assemble_cli:
	.size Assemble_cli, .func_end_Assemble_cli-Assemble_cli

	.local  Assemble_sei
	.type Assemble_sei, @function

Assemble_sei:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 120		// 0x78 ASCII 'x'
	j           AssembleSingleByteInstruction
.func_end_Assemble_sei:
	.size Assemble_sei, .func_end_Assemble_sei-Assemble_sei

	.local  Assemble_tya
	.type Assemble_tya, @function

Assemble_tya:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 152		// 0x98 ASCII \x98
	j           AssembleSingleByteInstruction
.func_end_Assemble_tya:
	.size Assemble_tya, .func_end_Assemble_tya-Assemble_tya

	.local  Assemble_clv
	.type Assemble_clv, @function

Assemble_clv:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 184		// 0xb8 ASCII \xb8
	j           AssembleSingleByteInstruction
.func_end_Assemble_clv:
	.size Assemble_clv, .func_end_Assemble_clv-Assemble_clv

	.local  Assemble_cld
	.type Assemble_cld, @function

Assemble_cld:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 216		// 0xd8 ASCII \xd8
	j           AssembleSingleByteInstruction
.func_end_Assemble_cld:
	.size Assemble_cld, .func_end_Assemble_cld-Assemble_cld

	.local  Assemble_sed
	.type Assemble_sed, @function

Assemble_sed:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 248		// 0xf8 ASCII \xf8
	j           AssembleSingleByteInstruction
.func_end_Assemble_sed:
	.size Assemble_sed, .func_end_Assemble_sed-Assemble_sed

	.local  Assemble_txa
	.type Assemble_txa, @function

Assemble_txa:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 138		// 0x8a ASCII \x8a
	j           AssembleSingleByteInstruction
.func_end_Assemble_txa:
	.size Assemble_txa, .func_end_Assemble_txa-Assemble_txa

	.local  Assemble_txs
	.type Assemble_txs, @function

Assemble_txs:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 154		// 0x9a ASCII \x9a
	j           AssembleSingleByteInstruction
.func_end_Assemble_txs:
	.size Assemble_txs, .func_end_Assemble_txs-Assemble_txs

	.local  Assemble_dex
	.type Assemble_dex, @function

Assemble_dex:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 206		// 0xce ASCII \xce
	j           AssembleSingleByteInstruction
.func_end_Assemble_dex:
	.size Assemble_dex, .func_end_Assemble_dex-Assemble_dex

	.local  Assemble_nop
	.type Assemble_nop, @function

Assemble_nop:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 234		// 0xea ASCII \xea
	j           AssembleSingleByteInstruction
.func_end_Assemble_nop:
	.size Assemble_nop, .func_end_Assemble_nop-Assemble_nop

	.local  Assemble_tax
	.type Assemble_tax, @function

Assemble_tax:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 170		// 0xaa ASCII \xaa
	j           AssembleSingleByteInstruction
.func_end_Assemble_tax:
	.size Assemble_tax, .func_end_Assemble_tax-Assemble_tax

	.local  Assemble_tsx
	.type Assemble_tsx, @function

Assemble_tsx:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 186		// 0xba ASCII \xba
	j           AssembleSingleByteInstruction
.func_end_Assemble_tsx:
	.size Assemble_tsx, .func_end_Assemble_tsx-Assemble_tsx

	.local  Assemble_bpl
	.type Assemble_bpl, @function

Assemble_bpl:

	// *** Basic block 0

	.local AssembleBranch
	// Leaf procedure, no stack frame generated
	li          a1, 16		// 0x10 ASCII \x10
	j           AssembleBranch
.func_end_Assemble_bpl:
	.size Assemble_bpl, .func_end_Assemble_bpl-Assemble_bpl

	.local  Assemble_bmi
	.type Assemble_bmi, @function

Assemble_bmi:

	// *** Basic block 0

	.local AssembleBranch
	// Leaf procedure, no stack frame generated
	li          a1, 48		// 0x30 ASCII '0'
	j           AssembleBranch
.func_end_Assemble_bmi:
	.size Assemble_bmi, .func_end_Assemble_bmi-Assemble_bmi

	.local  Assemble_bvc
	.type Assemble_bvc, @function

Assemble_bvc:

	// *** Basic block 0

	.local AssembleBranch
	// Leaf procedure, no stack frame generated
	li          a1, 80		// 0x50 ASCII 'P'
	j           AssembleBranch
.func_end_Assemble_bvc:
	.size Assemble_bvc, .func_end_Assemble_bvc-Assemble_bvc

	.local  Assemble_bvs
	.type Assemble_bvs, @function

Assemble_bvs:

	// *** Basic block 0

	.local AssembleBranch
	// Leaf procedure, no stack frame generated
	li          a1, 112		// 0x70 ASCII 'p'
	j           AssembleBranch
.func_end_Assemble_bvs:
	.size Assemble_bvs, .func_end_Assemble_bvs-Assemble_bvs

	.local  Assemble_bcc
	.type Assemble_bcc, @function

Assemble_bcc:

	// *** Basic block 0

	.local AssembleBranch
	// Leaf procedure, no stack frame generated
	li          a1, 144		// 0x90 ASCII \x90
	j           AssembleBranch
.func_end_Assemble_bcc:
	.size Assemble_bcc, .func_end_Assemble_bcc-Assemble_bcc

	.local  Assemble_bcs
	.type Assemble_bcs, @function

Assemble_bcs:

	// *** Basic block 0

	.local AssembleBranch
	// Leaf procedure, no stack frame generated
	li          a1, 176		// 0xb0 ASCII \xb0
	j           AssembleBranch
.func_end_Assemble_bcs:
	.size Assemble_bcs, .func_end_Assemble_bcs-Assemble_bcs

	.local  Assemble_bne
	.type Assemble_bne, @function

Assemble_bne:

	// *** Basic block 0

	.local AssembleBranch
	// Leaf procedure, no stack frame generated
	li          a1, 208		// 0xd0 ASCII \xd0
	j           AssembleBranch
.func_end_Assemble_bne:
	.size Assemble_bne, .func_end_Assemble_bne-Assemble_bne

	.local  Assemble_beq
	.type Assemble_beq, @function

Assemble_beq:

	// *** Basic block 0

	.local AssembleBranch
	// Leaf procedure, no stack frame generated
	li          a1, 240		// 0xf0 ASCII \xf0
	j           AssembleBranch
.func_end_Assemble_beq:
	.size Assemble_beq, .func_end_Assemble_beq-Assemble_beq

	.local  Assemble_bra
	.type Assemble_bra, @function

Assemble_bra:

	// *** Basic block 0

	.local AssembleBranch
	// Leaf procedure, no stack frame generated
	li          a1, 128		// 0x80 ASCII \x80
	j           AssembleBranch
.func_end_Assemble_bra:
	.size Assemble_bra, .func_end_Assemble_bra-Assemble_bra

	.local  Assemble_jmp
	.type Assemble_jmp, @function

Assemble_jmp:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local AssembleJump
	j           AssembleJump
.func_end_Assemble_jmp:
	.size Assemble_jmp, .func_end_Assemble_jmp-Assemble_jmp

	.local  Assemble_jsr
	.type Assemble_jsr, @function

Assemble_jsr:

	// *** Basic block 0

	.global LexLookingAt
	.global AssemblerError
	.global AssemblerFindSymbol
	.global NewAssemblerSymbol
	.global AssemblerInsertSymbol
	.global LexNextToken
	.global NewAssemblerRelocation
	.global AssemblerCurrentAddress
	.global AssemblerAddRelocation
	.global AssemblerEmitByte
	.global AssemblerEmitHalf
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
	addi        a0, s1, 176
	li          a1, 3		// 0x3 ASCII \x3
	call        LexLookingAt

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .Assemble_jsr_label_52

	// *** Basic block 2

	lla         a1, .str.117
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

	// *** Basic block 3

.Assemble_jsr_label_49:
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

.Assemble_jsr_label_52:
	addi        t0, s1, 176
	addi        t0, t0, 80
	ld          s2, 16(t0)
	mv          a1, s2
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 5

	mv          s3, a0
	bne         s3, x0, .Assemble_jsr_label_91

	// *** Basic block 6

	lw          a1, 744(s1)
	mv          a4, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, t0
	mv          a0, s2
	call        NewAssemblerSymbol

	// *** Basic block 7

	mv          s3, a0
	mv          a1, s3
	mv          a0, s1
	call        AssemblerInsertSymbol

	// *** Basic block 8

.Assemble_jsr_label_91:
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 72(s3)
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 9

	lb          t0, 760(s1)
	beqz        t0, .Assemble_jsr_label_105

	// *** Basic block 10

	li          s2, 4		// 0x4 ASCII \x4
	j           .Assemble_jsr_label_107

	// *** Basic block 11

.Assemble_jsr_label_105:
	li          s2, 1		// 0x1 ASCII \x1

	// *** Basic block 12

.Assemble_jsr_label_107:
	lw          s4, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 13

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s4
	mv          a1, s2
	mv          a0, s3
	call        NewAssemblerRelocation

	// *** Basic block 14

	mv          s2, a0
	mv          a1, s2
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 15

	lw          a1, 744(s1)
	li          t0, 32		// 0x20 ASCII ' '
	mv          a2, t0
	mv          a0, s1
	call        AssemblerEmitByte

	// *** Basic block 16

	lw          a1, 744(s1)
	mv          a2, x0
	mv          a0, s1
	call        AssemblerEmitHalf

	// *** Basic block 17

	j           .Assemble_jsr_label_49
.func_end_Assemble_jsr:
	.size Assemble_jsr, .func_end_Assemble_jsr-Assemble_jsr

	.local  Assemble_stz
	.type Assemble_stz, @function

Assemble_stz:

	// *** Basic block 0

	.global LexMatch
	.local AssembleAbsouteAddress
	.global AssemblerEvaluateExpression
	.local CheckWidth
	.global LexLookingAt
	.global StringEqualCaseBlind
	.global AssemblerError
	.global LexNextToken
	.global AssemblerEmitByte
	.global AssemblerEmitHalf
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
	li          s3, 1		// 0x1 ASCII \x1
	mv          s4, x0
	addi        a0, s1, 176
	li          a1, 38		// 0x26 ASCII '&'
	call        LexMatch

	// *** Basic block 1

	beqz        a0, .Assemble_stz_label_54

	// *** Basic block 2

	mv          a0, s1
	call        AssembleAbsouteAddress

	// *** Basic block 3

	j           .Assemble_stz_label_68

	// *** Basic block 4

.Assemble_stz_label_54:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 5

	sext.w      s2, a0
	li          t0, 16		// 0x10 ASCII \x10
	mv          a2, t0
	mv          a1, s2
	mv          a0, s1
	call        CheckWidth

	// *** Basic block 6

.Assemble_stz_label_68:
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_stz_label_129

	// *** Basic block 8

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 9

	beqz        a0, .Assemble_stz_label_127

	// *** Basic block 10

	sltz        t0, s2
	not         s5, t0
	blt         s2, x0, .Assemble_stz_label_88

	// *** Basic block 11

	slti        s5, s2, 256

	// *** Basic block 12

.Assemble_stz_label_88:
	li          s3, 2		// 0x2 ASCII \x2
	addi        t0, s1, 176
	addi        a0, t0, 80
	lla         a1, .str.118
	call        StringEqualCaseBlind

	// *** Basic block 13

	beqz        a0, .Assemble_stz_label_107

	// *** Basic block 14

	beqz        s5, .Assemble_stz_label_103

	// *** Basic block 15

	li          s4, 116		// 0x74 ASCII 't'
	li          s3, 1		// 0x1 ASCII \x1
	j           .Assemble_stz_label_105

	// *** Basic block 16

.Assemble_stz_label_103:
	li          s4, 158		// 0x9e ASCII \x9e

	// *** Basic block 17

.Assemble_stz_label_105:
	j           .Assemble_stz_label_123

	// *** Basic block 18

.Assemble_stz_label_107:
	addi        t0, s1, 176
	addi        a0, t0, 80
	lla         a1, .str.119
	call        StringEqualCaseBlind

	// *** Basic block 19

	beqz        a0, .Assemble_stz_label_122

	// *** Basic block 20

	lla         a1, .str.120
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 21

.Assemble_stz_label_122:

	// *** Basic block 22

.Assemble_stz_label_123:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 23

.Assemble_stz_label_127:
	j           .Assemble_stz_label_143

	// *** Basic block 24

.Assemble_stz_label_129:
	sltz        t1, s2
	not         t0, t1
	blt         s2, x0, .Assemble_stz_label_135

	// *** Basic block 25

	slti        t0, s2, 256

	// *** Basic block 26

.Assemble_stz_label_135:
	beqz        t0, .Assemble_stz_label_139

	// *** Basic block 27

	li          s4, 100		// 0x64 ASCII 'd'
	j           .Assemble_stz_label_142

	// *** Basic block 28

.Assemble_stz_label_139:
	li          s4, 156		// 0x9c ASCII \x9c
	li          s3, 2		// 0x2 ASCII \x2

	// *** Basic block 29

.Assemble_stz_label_142:

	// *** Basic block 30

.Assemble_stz_label_143:
	lw          a1, 744(s1)
	andi        a2, s4, 255
	mv          a0, s1
	call        AssemblerEmitByte

	// *** Basic block 31

	li          t0, 1		// 0x1 ASCII \x1
	bne         s3, t0, .Assemble_stz_label_167

	// *** Basic block 32

	lw          a1, 744(s1)
	andi        a2, s2, 255
	mv          a0, s1
	call        AssemblerEmitByte

	// *** Basic block 33

	j           .Assemble_stz_label_178

	// *** Basic block 34

.Assemble_stz_label_167:
	lw          a1, 744(s1)
	li          t0, 65535		// 0xffff
	and         a2, s2, t0
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
	j           AssemblerEmitHalf

	// *** Basic block 35

.Assemble_stz_label_178:
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
.func_end_Assemble_stz:
	.size Assemble_stz, .func_end_Assemble_stz-Assemble_stz

	.local  Assemble_lda
	.type Assemble_lda, @function

Assemble_lda:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 161		// 0xa1 ASCII \xa1
	j           AssembleMemoryInstruction
.func_end_Assemble_lda:
	.size Assemble_lda, .func_end_Assemble_lda-Assemble_lda

	.local  Assemble_ldx
	.type Assemble_ldx, @function

Assemble_ldx:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 162		// 0xa2 ASCII \xa2
	j           AssembleMemoryInstruction
.func_end_Assemble_ldx:
	.size Assemble_ldx, .func_end_Assemble_ldx-Assemble_ldx

	.local  Assemble_ldy
	.type Assemble_ldy, @function

Assemble_ldy:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 160		// 0xa0 ASCII \xa0
	j           AssembleMemoryInstruction
.func_end_Assemble_ldy:
	.size Assemble_ldy, .func_end_Assemble_ldy-Assemble_ldy

	.local  Assemble_sta
	.type Assemble_sta, @function

Assemble_sta:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 129		// 0x81 ASCII \x81
	j           AssembleMemoryInstruction
.func_end_Assemble_sta:
	.size Assemble_sta, .func_end_Assemble_sta-Assemble_sta

	.local  Assemble_stx
	.type Assemble_stx, @function

Assemble_stx:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 130		// 0x82 ASCII \x82
	j           AssembleMemoryInstruction
.func_end_Assemble_stx:
	.size Assemble_stx, .func_end_Assemble_stx-Assemble_stx

	.local  Assemble_sty
	.type Assemble_sty, @function

Assemble_sty:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 128		// 0x80 ASCII \x80
	j           AssembleMemoryInstruction
.func_end_Assemble_sty:
	.size Assemble_sty, .func_end_Assemble_sty-Assemble_sty

	.local  Assemble_cmp
	.type Assemble_cmp, @function

Assemble_cmp:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 193		// 0xc1 ASCII \xc1
	j           AssembleMemoryInstruction
.func_end_Assemble_cmp:
	.size Assemble_cmp, .func_end_Assemble_cmp-Assemble_cmp

	.local  Assemble_cpy
	.type Assemble_cpy, @function

Assemble_cpy:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 192		// 0xc0 ASCII \xc0
	j           AssembleMemoryInstruction
.func_end_Assemble_cpy:
	.size Assemble_cpy, .func_end_Assemble_cpy-Assemble_cpy

	.local  Assemble_cpx
	.type Assemble_cpx, @function

Assemble_cpx:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 224		// 0xe0 ASCII \xe0
	j           AssembleMemoryInstruction
.func_end_Assemble_cpx:
	.size Assemble_cpx, .func_end_Assemble_cpx-Assemble_cpx

	.local  Assemble_bit
	.type Assemble_bit, @function

Assemble_bit:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 32		// 0x20 ASCII ' '
	j           AssembleMemoryInstruction
.func_end_Assemble_bit:
	.size Assemble_bit, .func_end_Assemble_bit-Assemble_bit

	.local  Assemble_ora
	.type Assemble_ora, @function

Assemble_ora:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 1		// 0x1 ASCII \x1
	j           AssembleMemoryInstruction
.func_end_Assemble_ora:
	.size Assemble_ora, .func_end_Assemble_ora-Assemble_ora

	.local  Assemble_and
	.type Assemble_and, @function

Assemble_and:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 33		// 0x21 ASCII '!'
	j           AssembleMemoryInstruction
.func_end_Assemble_and:
	.size Assemble_and, .func_end_Assemble_and-Assemble_and

	.local  Assemble_eor
	.type Assemble_eor, @function

Assemble_eor:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 65		// 0x41 ASCII 'A'
	j           AssembleMemoryInstruction
.func_end_Assemble_eor:
	.size Assemble_eor, .func_end_Assemble_eor-Assemble_eor

	.local  Assemble_adc
	.type Assemble_adc, @function

Assemble_adc:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 97		// 0x61 ASCII 'a'
	j           AssembleMemoryInstruction
.func_end_Assemble_adc:
	.size Assemble_adc, .func_end_Assemble_adc-Assemble_adc

	.local  Assemble_sbc
	.type Assemble_sbc, @function

Assemble_sbc:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 225		// 0xe1 ASCII \xe1
	j           AssembleMemoryInstruction
.func_end_Assemble_sbc:
	.size Assemble_sbc, .func_end_Assemble_sbc-Assemble_sbc

	.local  Assemble_asl
	.type Assemble_asl, @function

Assemble_asl:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 2		// 0x2 ASCII \x2
	j           AssembleMemoryInstruction
.func_end_Assemble_asl:
	.size Assemble_asl, .func_end_Assemble_asl-Assemble_asl

	.local  Assemble_rol
	.type Assemble_rol, @function

Assemble_rol:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 34		// 0x22 ASCII '"'
	j           AssembleMemoryInstruction
.func_end_Assemble_rol:
	.size Assemble_rol, .func_end_Assemble_rol-Assemble_rol

	.local  Assemble_lsr
	.type Assemble_lsr, @function

Assemble_lsr:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 66		// 0x42 ASCII 'B'
	j           AssembleMemoryInstruction
.func_end_Assemble_lsr:
	.size Assemble_lsr, .func_end_Assemble_lsr-Assemble_lsr

	.local  Assemble_ror
	.type Assemble_ror, @function

Assemble_ror:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 98		// 0x62 ASCII 'b'
	j           AssembleMemoryInstruction
.func_end_Assemble_ror:
	.size Assemble_ror, .func_end_Assemble_ror-Assemble_ror

	.local  Assemble_dec
	.type Assemble_dec, @function

Assemble_dec:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 194		// 0xc2 ASCII \xc2
	j           AssembleMemoryInstruction
.func_end_Assemble_dec:
	.size Assemble_dec, .func_end_Assemble_dec-Assemble_dec

	.local  Assemble_inc
	.type Assemble_inc, @function

Assemble_inc:

	// *** Basic block 0

	.local AssembleMemoryInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 226		// 0xe2 ASCII \xe2
	j           AssembleMemoryInstruction
.func_end_Assemble_inc:
	.size Assemble_inc, .func_end_Assemble_inc-Assemble_inc

	.local  Assemble_tsb
	.type Assemble_tsb, @function

Assemble_tsb:

	// *** Basic block 0

	ret         
.func_end_Assemble_tsb:
	.size Assemble_tsb, .func_end_Assemble_tsb-Assemble_tsb

	.local  Assemble_trb
	.type Assemble_trb, @function

Assemble_trb:

	// *** Basic block 0

	ret         
.func_end_Assemble_trb:
	.size Assemble_trb, .func_end_Assemble_trb-Assemble_trb

	.local  Assemble_phy
	.type Assemble_phy, @function

Assemble_phy:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 90		// 0x5a ASCII 'Z'
	j           AssembleSingleByteInstruction
.func_end_Assemble_phy:
	.size Assemble_phy, .func_end_Assemble_phy-Assemble_phy

	.local  Assemble_ply
	.type Assemble_ply, @function

Assemble_ply:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 122		// 0x7a ASCII 'z'
	j           AssembleSingleByteInstruction
.func_end_Assemble_ply:
	.size Assemble_ply, .func_end_Assemble_ply-Assemble_ply

	.local  Assemble_phx
	.type Assemble_phx, @function

Assemble_phx:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 218		// 0xda ASCII \xda
	j           AssembleSingleByteInstruction
.func_end_Assemble_phx:
	.size Assemble_phx, .func_end_Assemble_phx-Assemble_phx

	.local  Assemble_plx
	.type Assemble_plx, @function

Assemble_plx:

	// *** Basic block 0

	.local AssembleSingleByteInstruction
	// Leaf procedure, no stack frame generated
	li          a1, 250		// 0xfa ASCII \xfa
	j           AssembleSingleByteInstruction
.func_end_Assemble_plx:
	.size Assemble_plx, .func_end_Assemble_plx-Assemble_plx

.PCend:
	.data
reloc_types:
	.type   reloc_types,@object
	.local  reloc_types
	.size   reloc_types,40
	.p2align  2
	.word   21
	.word   22
	.word   14
	.word   15
	.word   16
	.word   17
	.word   18
	.word   19
	.word   4
	.word   0

assembler_functions:
	.type   assembler_functions,@object
	.local  assembler_functions
	.size   assembler_functions,128
	.p2align  3
	.long    .str.86
	.word   23
	.space  4
	.long    .str.87
	.word   24
	.space  4
	.long    .str.88
	.word   25
	.space  4
	.long    .str.89
	.word   26
	.space  4
	.long    .str.90
	.word   27
	.space  4
	.long    .str.91
	.word   28
	.space  4
	.long    .str.92
	.word   29
	.space  4
	.long    .str.93
	.word   30
	.space  4

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "brk"
	.type .str.1, @object
	.size .str.1, 4

.str.2:
	.asciz "bpl"
	.type .str.2, @object
	.size .str.2, 4

.str.3:
	.asciz "bmi"
	.type .str.3, @object
	.size .str.3, 4

.str.4:
	.asciz "bvc"
	.type .str.4, @object
	.size .str.4, 4

.str.5:
	.asciz "bvs"
	.type .str.5, @object
	.size .str.5, 4

.str.6:
	.asciz "bcc"
	.type .str.6, @object
	.size .str.6, 4

.str.7:
	.asciz "bcs"
	.type .str.7, @object
	.size .str.7, 4

.str.8:
	.asciz "bne"
	.type .str.8, @object
	.size .str.8, 4

.str.9:
	.asciz "beq"
	.type .str.9, @object
	.size .str.9, 4

.str.10:
	.asciz "jsr"
	.type .str.10, @object
	.size .str.10, 4

.str.11:
	.asciz "jmp"
	.type .str.11, @object
	.size .str.11, 4

.str.12:
	.asciz "rti"
	.type .str.12, @object
	.size .str.12, 4

.str.13:
	.asciz "rts"
	.type .str.13, @object
	.size .str.13, 4

.str.14:
	.asciz "lda"
	.type .str.14, @object
	.size .str.14, 4

.str.15:
	.asciz "ldx"
	.type .str.15, @object
	.size .str.15, 4

.str.16:
	.asciz "ldy"
	.type .str.16, @object
	.size .str.16, 4

.str.17:
	.asciz "sta"
	.type .str.17, @object
	.size .str.17, 4

.str.18:
	.asciz "stx"
	.type .str.18, @object
	.size .str.18, 4

.str.19:
	.asciz "sty"
	.type .str.19, @object
	.size .str.19, 4

.str.20:
	.asciz "cmp"
	.type .str.20, @object
	.size .str.20, 4

.str.21:
	.asciz "cpy"
	.type .str.21, @object
	.size .str.21, 4

.str.22:
	.asciz "cpx"
	.type .str.22, @object
	.size .str.22, 4

.str.23:
	.asciz "bit"
	.type .str.23, @object
	.size .str.23, 4

.str.24:
	.asciz "ora"
	.type .str.24, @object
	.size .str.24, 4

.str.25:
	.asciz "and"
	.type .str.25, @object
	.size .str.25, 4

.str.26:
	.asciz "eor"
	.type .str.26, @object
	.size .str.26, 4

.str.27:
	.asciz "adc"
	.type .str.27, @object
	.size .str.27, 4

.str.28:
	.asciz "sbc"
	.type .str.28, @object
	.size .str.28, 4

.str.29:
	.asciz "asl"
	.type .str.29, @object
	.size .str.29, 4

.str.30:
	.asciz "rol"
	.type .str.30, @object
	.size .str.30, 4

.str.31:
	.asciz "lsr"
	.type .str.31, @object
	.size .str.31, 4

.str.32:
	.asciz "ror"
	.type .str.32, @object
	.size .str.32, 4

.str.33:
	.asciz "dec"
	.type .str.33, @object
	.size .str.33, 4

.str.34:
	.asciz "inc"
	.type .str.34, @object
	.size .str.34, 4

.str.35:
	.asciz "dey"
	.type .str.35, @object
	.size .str.35, 4

.str.36:
	.asciz "dex"
	.type .str.36, @object
	.size .str.36, 4

.str.37:
	.asciz "iny"
	.type .str.37, @object
	.size .str.37, 4

.str.38:
	.asciz "inx"
	.type .str.38, @object
	.size .str.38, 4

.str.39:
	.asciz "php"
	.type .str.39, @object
	.size .str.39, 4

.str.40:
	.asciz "clc"
	.type .str.40, @object
	.size .str.40, 4

.str.41:
	.asciz "plp"
	.type .str.41, @object
	.size .str.41, 4

.str.42:
	.asciz "sec"
	.type .str.42, @object
	.size .str.42, 4

.str.43:
	.asciz "pha"
	.type .str.43, @object
	.size .str.43, 4

.str.44:
	.asciz "cli"
	.type .str.44, @object
	.size .str.44, 4

.str.45:
	.asciz "pla"
	.type .str.45, @object
	.size .str.45, 4

.str.46:
	.asciz "sei"
	.type .str.46, @object
	.size .str.46, 4

.str.47:
	.asciz "tay"
	.type .str.47, @object
	.size .str.47, 4

.str.48:
	.asciz "clv"
	.type .str.48, @object
	.size .str.48, 4

.str.49:
	.asciz "cld"
	.type .str.49, @object
	.size .str.49, 4

.str.50:
	.asciz "sed"
	.type .str.50, @object
	.size .str.50, 4

.str.51:
	.asciz "tya"
	.type .str.51, @object
	.size .str.51, 4

.str.52:
	.asciz "txa"
	.type .str.52, @object
	.size .str.52, 4

.str.53:
	.asciz "txs"
	.type .str.53, @object
	.size .str.53, 4

.str.54:
	.asciz "tax"
	.type .str.54, @object
	.size .str.54, 4

.str.55:
	.asciz "tsx"
	.type .str.55, @object
	.size .str.55, 4

.str.56:
	.asciz "nop"
	.type .str.56, @object
	.size .str.56, 4

.str.57:
	.asciz "tsb"
	.type .str.57, @object
	.size .str.57, 4

.str.58:
	.asciz "trb"
	.type .str.58, @object
	.size .str.58, 4

.str.59:
	.asciz "stz"
	.type .str.59, @object
	.size .str.59, 4

.str.60:
	.asciz "phy"
	.type .str.60, @object
	.size .str.60, 4

.str.61:
	.asciz "ply"
	.type .str.61, @object
	.size .str.61, 4

.str.62:
	.asciz "phx"
	.type .str.62, @object
	.size .str.62, 4

.str.63:
	.asciz "plx"
	.type .str.63, @object
	.size .str.63, 4

.str.64:
	.asciz "bra"
	.type .str.64, @object
	.size .str.64, 4

.str.65:
	.asciz ".bss"
	.type .str.65, @object
	.size .str.65, 5

.str.66:
	.asciz "Syntax error; unknown instruction: %s"
	.type .str.66, @object
	.size .str.66, 38

.str.67:
	.asciz "Syntax error in assembler function: missing )"
	.type .str.67, @object
	.size .str.67, 46

.str.68:
	.asciz "Syntax error in assembler function: missing symbol name"
	.type .str.68, @object
	.size .str.68, 56

.str.69:
	.asciz "Syntax error in assembler function: missing ("
	.type .str.69, @object
	.size .str.69, 46

.str.70:
	.asciz "Syntax error in assembler function: missing name"
	.type .str.70, @object
	.size .str.70, 49

.str.71:
	.asciz "Value 0x%x won\'t fit in %d bits"
	.type .str.71, @object
	.size .str.71, 32

.str.72:
	.asciz "0x%x is not a zero page address"
	.type .str.72, @object
	.size .str.72, 32

.str.73:
	.asciz "Expected index register %s"
	.type .str.73, @object
	.size .str.73, 27

.str.74:
	.asciz "Expected index register %s, not %s"
	.type .str.74, @object
	.size .str.74, 35

.str.75:
	.asciz "A"
	.type .str.75, @object
	.size .str.75, 2

.str.76:
	.asciz "Missing )"
	.type .str.76, @object
	.size .str.76, 10

.str.77:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.77, @object
	.size .str.77, 30

.str.78:
	.asciz "(null)"
	.type .str.78, @object
	.size .str.78, 1

.str.79:
	.asciz "branch->label != NULL"
	.type .str.79, @object
	.size .str.79, 22

.str.80:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.80, @object
	.size .str.80, 30

.str.81:
	.asciz "(null)"
	.type .str.81, @object
	.size .str.81, 1

.str.82:
	.asciz "branch != NULL"
	.type .str.82, @object
	.size .str.82, 15

.str.83:
	.asciz "Missing symbol for jmp instruction"
	.type .str.83, @object
	.size .str.83, 35

.str.84:
	.asciz "X"
	.type .str.84, @object
	.size .str.84, 2

.str.85:
	.asciz "Missing close paren for jmp instruction"
	.type .str.85, @object
	.size .str.85, 40

.str.86:
	.asciz "byte0"
	.type .str.86, @object
	.size .str.86, 6

.str.87:
	.asciz "byte1"
	.type .str.87, @object
	.size .str.87, 6

.str.88:
	.asciz "byte2"
	.type .str.88, @object
	.size .str.88, 6

.str.89:
	.asciz "byte3"
	.type .str.89, @object
	.size .str.89, 6

.str.90:
	.asciz "byte4"
	.type .str.90, @object
	.size .str.90, 6

.str.91:
	.asciz "byte5"
	.type .str.91, @object
	.size .str.91, 6

.str.92:
	.asciz "byte6"
	.type .str.92, @object
	.size .str.92, 6

.str.93:
	.asciz "byte7"
	.type .str.93, @object
	.size .str.93, 6

.str.94:
	.asciz "(null)"
	.type .str.94, @object
	.size .str.94, 1

.str.95:
	.asciz "(null)"
	.type .str.95, @object
	.size .str.95, 1

.str.96:
	.asciz "Unknown function %%%s"
	.type .str.96, @object
	.size .str.96, 22

.str.97:
	.asciz "Invalid immediate operand for this instruction"
	.type .str.97, @object
	.size .str.97, 47

.str.98:
	.asciz "Invalid immediate operand for STA instruction"
	.type .str.98, @object
	.size .str.98, 46

.str.99:
	.asciz "Invalid immediate operand for this instruction"
	.type .str.99, @object
	.size .str.99, 47

.str.100:
	.asciz "X"
	.type .str.100, @object
	.size .str.100, 2

.str.101:
	.asciz "Illegal zeropage,X instruction"
	.type .str.101, @object
	.size .str.101, 31

.str.102:
	.asciz "Illegal abs,X instruction"
	.type .str.102, @object
	.size .str.102, 26

.str.103:
	.asciz "Syntax error for (xx),Y"
	.type .str.103, @object
	.size .str.103, 24

.str.104:
	.asciz "Y"
	.type .str.104, @object
	.size .str.104, 2

.str.105:
	.asciz "X"
	.type .str.105, @object
	.size .str.105, 2

.str.106:
	.asciz "X"
	.type .str.106, @object
	.size .str.106, 2

.str.107:
	.asciz "Y"
	.type .str.107, @object
	.size .str.107, 2

.str.108:
	.asciz "Illegal accumulator instruction"
	.type .str.108, @object
	.size .str.108, 32

.str.109:
	.asciz "X"
	.type .str.109, @object
	.size .str.109, 2

.str.110:
	.asciz "Y"
	.type .str.110, @object
	.size .str.110, 2

.str.111:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.111, @object
	.size .str.111, 30

.str.112:
	.asciz "(null)"
	.type .str.112, @object
	.size .str.112, 1

.str.113:
	.asciz "false"
	.type .str.113, @object
	.size .str.113, 6

.str.114:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.114, @object
	.size .str.114, 30

.str.115:
	.asciz "(null)"
	.type .str.115, @object
	.size .str.115, 1

.str.116:
	.asciz "false"
	.type .str.116, @object
	.size .str.116, 6

.str.117:
	.asciz "Missing symbol for jsr instruction"
	.type .str.117, @object
	.size .str.117, 35

.str.118:
	.asciz "X"
	.type .str.118, @object
	.size .str.118, 2

.str.119:
	.asciz "Y"
	.type .str.119, @object
	.size .str.119, 2

.str.120:
	.asciz "Invalid STZ addressing mode"
	.type .str.120, @object
	.size .str.120, 28

