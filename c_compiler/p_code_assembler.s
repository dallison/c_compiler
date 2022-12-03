	.file   "p_code_assembler.c"
	.text
	.option pic
.PCbegin:
	.local  CompareString
	.type CompareString, @function

CompareString:

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
.func_end_CompareString:
	.size CompareString, .func_end_CompareString-CompareString

	.local  InitializeInstructions
	.type InitializeInstructions, @function

InitializeInstructions:

	// *** Basic block 0

	.local Assemble_decsp
	.global MapInsert
	.local Assemble_incsp
	.local Assemble_movc
	.local Assemble_movfc
	.local Assemble_movdc
	.local Assemble_movxc
	.local Assemble_mov
	.local Assemble_movf
	.local Assemble_movd
	.local Assemble_push
	.local Assemble_pushf
	.local Assemble_pushd
	.local Assemble_pushx
	.local Assemble_pop
	.local Assemble_popf
	.local Assemble_popd
	.local Assemble_popx
	.local Assemble_add
	.local Assemble_addf
	.local Assemble_addd
	.local Assemble_addc
	.local Assemble_ldw
	.local Assemble_ldh
	.local Assemble_ldb
	.local Assemble_lduw
	.local Assemble_ldub
	.local Assemble_lduh
	.local Assemble_ldx
	.local Assemble_ldf
	.local Assemble_ldd
	.local Assemble_stw
	.local Assemble_sth
	.local Assemble_stx
	.local Assemble_stf
	.local Assemble_std
	.local Assemble_stb
	.local Assemble_sub
	.local Assemble_subf
	.local Assemble_subd
	.local Assemble_mul
	.local Assemble_mulf
	.local Assemble_muld
	.local Assemble_div
	.local Assemble_divu
	.local Assemble_divf
	.local Assemble_divd
	.local Assemble_mod
	.local Assemble_modu
	.local Assemble_lsr
	.local Assemble_asr
	.local Assemble_lsl
	.local Assemble_or
	.local Assemble_and
	.local Assemble_xor
	.local Assemble_not
	.local Assemble_inv
	.local Assemble_neg
	.local Assemble_negf
	.local Assemble_negd
	.local Assemble_cmpeq
	.local Assemble_cmpne
	.local Assemble_cmplt
	.local Assemble_cmple
	.local Assemble_cmpgt
	.local Assemble_cmpge
	.local Assemble_cmpltu
	.local Assemble_cmpleu
	.local Assemble_cmpgtu
	.local Assemble_cmpgeu
	.local Assemble_cmpeqf
	.local Assemble_cmpnef
	.local Assemble_cmpltf
	.local Assemble_cmplef
	.local Assemble_cmpgtf
	.local Assemble_cmpgef
	.local Assemble_cmpeqd
	.local Assemble_cmpned
	.local Assemble_cmpltd
	.local Assemble_cmpled
	.local Assemble_cmpgtd
	.local Assemble_cmpged
	.local Assemble_bnz
	.local Assemble_bz
	.local Assemble_bra
	.local Assemble_cbra
	.local Assemble_i2f
	.local Assemble_i2d
	.local Assemble_ui2f
	.local Assemble_ui2d
	.local Assemble_f2d
	.local Assemble_d2f
	.local Assemble_f2i
	.local Assemble_d2i
	.local Assemble_f2ui
	.local Assemble_d2ui
	.local Assemble_jmp
	.local Assemble_cjmp
	.local Assemble_adr
	.local Assemble_adrs
	.local Assemble_adrtls
	.local Assemble_call
	.local Assemble_rcall
	.local Assemble_ret
	.local Assemble_esc
	addi sp, sp, -1696
	// Saved return address (offset 1688) and frame pointer (offset 1680)
	sd ra, 1688(sp)
	sd s0, 1680(sp)
	addi s0, sp, 1696
	// Local vars at offset -1680(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0

	// *** Basic block 1

.InitializeInstructions_label_215:
	lla         t0, .str.1
	sd          t0, -1680(s0)
	addi        t0, s0, -1680
	la          t1, Assemble_decsp
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1680(s0)
	sd          t0, 0(sp)
	ld          t0, -1672(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 2

	addi        sp, sp, 16

	// *** Basic block 3

.InitializeInstructions_label_240:

	// *** Basic block 4

.InitializeInstructions_label_241:

	// *** Basic block 5

.InitializeInstructions_label_242:
	lla         t0, .str.2
	sd          t0, -1664(s0)
	addi        t0, s0, -1664
	la          t1, Assemble_incsp
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1664(s0)
	sd          t0, 0(sp)
	ld          t0, -1656(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 6

	addi        sp, sp, 16

	// *** Basic block 7

.InitializeInstructions_label_262:

	// *** Basic block 8

.InitializeInstructions_label_263:

	// *** Basic block 9

.InitializeInstructions_label_264:
	lla         t0, .str.3
	sd          t0, -1648(s0)
	addi        t0, s0, -1648
	la          t1, Assemble_movc
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1648(s0)
	sd          t0, 0(sp)
	ld          t0, -1640(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 10

	addi        sp, sp, 16

	// *** Basic block 11

.InitializeInstructions_label_284:

	// *** Basic block 12

.InitializeInstructions_label_285:

	// *** Basic block 13

.InitializeInstructions_label_286:
	lla         t0, .str.4
	sd          t0, -1632(s0)
	addi        t0, s0, -1632
	la          t1, Assemble_movfc
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1632(s0)
	sd          t0, 0(sp)
	ld          t0, -1624(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 14

	addi        sp, sp, 16

	// *** Basic block 15

.InitializeInstructions_label_306:

	// *** Basic block 16

.InitializeInstructions_label_307:

	// *** Basic block 17

.InitializeInstructions_label_308:
	lla         t0, .str.5
	sd          t0, -1616(s0)
	addi        t0, s0, -1616
	la          t1, Assemble_movdc
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1616(s0)
	sd          t0, 0(sp)
	ld          t0, -1608(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 18

	addi        sp, sp, 16

	// *** Basic block 19

.InitializeInstructions_label_328:

	// *** Basic block 20

.InitializeInstructions_label_329:

	// *** Basic block 21

.InitializeInstructions_label_330:
	lla         t0, .str.6
	sd          t0, -1600(s0)
	addi        t0, s0, -1600
	la          t1, Assemble_movxc
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1600(s0)
	sd          t0, 0(sp)
	ld          t0, -1592(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 22

	addi        sp, sp, 16

	// *** Basic block 23

.InitializeInstructions_label_350:

	// *** Basic block 24

.InitializeInstructions_label_351:

	// *** Basic block 25

.InitializeInstructions_label_352:
	lla         t0, .str.7
	sd          t0, -1584(s0)
	addi        t0, s0, -1584
	la          t1, Assemble_mov
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1584(s0)
	sd          t0, 0(sp)
	ld          t0, -1576(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 26

	addi        sp, sp, 16

	// *** Basic block 27

.InitializeInstructions_label_372:

	// *** Basic block 28

.InitializeInstructions_label_373:

	// *** Basic block 29

.InitializeInstructions_label_374:
	lla         t0, .str.8
	sd          t0, -1568(s0)
	addi        t0, s0, -1568
	la          t1, Assemble_movf
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1568(s0)
	sd          t0, 0(sp)
	ld          t0, -1560(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 30

	addi        sp, sp, 16

	// *** Basic block 31

.InitializeInstructions_label_394:

	// *** Basic block 32

.InitializeInstructions_label_395:

	// *** Basic block 33

.InitializeInstructions_label_396:
	lla         t0, .str.9
	sd          t0, -1552(s0)
	addi        t0, s0, -1552
	la          t1, Assemble_movd
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1552(s0)
	sd          t0, 0(sp)
	ld          t0, -1544(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 34

	addi        sp, sp, 16

	// *** Basic block 35

.InitializeInstructions_label_416:

	// *** Basic block 36

.InitializeInstructions_label_417:

	// *** Basic block 37

.InitializeInstructions_label_418:
	lla         t0, .str.10
	sd          t0, -1536(s0)
	addi        t0, s0, -1536
	la          t1, Assemble_push
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1536(s0)
	sd          t0, 0(sp)
	ld          t0, -1528(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 38

	addi        sp, sp, 16

	// *** Basic block 39

.InitializeInstructions_label_438:

	// *** Basic block 40

.InitializeInstructions_label_439:

	// *** Basic block 41

.InitializeInstructions_label_440:
	lla         t0, .str.11
	sd          t0, -1520(s0)
	addi        t0, s0, -1520
	la          t1, Assemble_pushf
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1520(s0)
	sd          t0, 0(sp)
	ld          t0, -1512(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 42

	addi        sp, sp, 16

	// *** Basic block 43

.InitializeInstructions_label_460:

	// *** Basic block 44

.InitializeInstructions_label_461:

	// *** Basic block 45

.InitializeInstructions_label_462:
	lla         t0, .str.12
	sd          t0, -1504(s0)
	addi        t0, s0, -1504
	la          t1, Assemble_pushd
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1504(s0)
	sd          t0, 0(sp)
	ld          t0, -1496(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 46

	addi        sp, sp, 16

	// *** Basic block 47

.InitializeInstructions_label_482:

	// *** Basic block 48

.InitializeInstructions_label_483:

	// *** Basic block 49

.InitializeInstructions_label_484:
	lla         t0, .str.13
	sd          t0, -1488(s0)
	addi        t0, s0, -1488
	la          t1, Assemble_pushx
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1488(s0)
	sd          t0, 0(sp)
	ld          t0, -1480(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 50

	addi        sp, sp, 16

	// *** Basic block 51

.InitializeInstructions_label_504:

	// *** Basic block 52

.InitializeInstructions_label_505:

	// *** Basic block 53

.InitializeInstructions_label_506:
	lla         t0, .str.14
	sd          t0, -1472(s0)
	addi        t0, s0, -1472
	la          t1, Assemble_pop
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1472(s0)
	sd          t0, 0(sp)
	ld          t0, -1464(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 54

	addi        sp, sp, 16

	// *** Basic block 55

.InitializeInstructions_label_526:

	// *** Basic block 56

.InitializeInstructions_label_527:

	// *** Basic block 57

.InitializeInstructions_label_528:
	lla         t0, .str.15
	sd          t0, -1456(s0)
	addi        t0, s0, -1456
	la          t1, Assemble_popf
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1456(s0)
	sd          t0, 0(sp)
	ld          t0, -1448(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 58

	addi        sp, sp, 16

	// *** Basic block 59

.InitializeInstructions_label_548:

	// *** Basic block 60

.InitializeInstructions_label_549:

	// *** Basic block 61

.InitializeInstructions_label_550:
	lla         t0, .str.16
	sd          t0, -1440(s0)
	addi        t0, s0, -1440
	la          t1, Assemble_popd
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1440(s0)
	sd          t0, 0(sp)
	ld          t0, -1432(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 62

	addi        sp, sp, 16

	// *** Basic block 63

.InitializeInstructions_label_570:

	// *** Basic block 64

.InitializeInstructions_label_571:

	// *** Basic block 65

.InitializeInstructions_label_572:
	lla         t0, .str.17
	sd          t0, -1424(s0)
	addi        t0, s0, -1424
	la          t1, Assemble_popx
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1424(s0)
	sd          t0, 0(sp)
	ld          t0, -1416(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 66

	addi        sp, sp, 16

	// *** Basic block 67

.InitializeInstructions_label_592:

	// *** Basic block 68

.InitializeInstructions_label_593:

	// *** Basic block 69

.InitializeInstructions_label_594:
	lla         t0, .str.18
	sd          t0, -1408(s0)
	addi        t0, s0, -1408
	la          t1, Assemble_add
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1408(s0)
	sd          t0, 0(sp)
	ld          t0, -1400(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 70

	addi        sp, sp, 16

	// *** Basic block 71

.InitializeInstructions_label_614:

	// *** Basic block 72

.InitializeInstructions_label_615:

	// *** Basic block 73

.InitializeInstructions_label_616:
	lla         t0, .str.19
	sd          t0, -1392(s0)
	addi        t0, s0, -1392
	la          t1, Assemble_addf
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1392(s0)
	sd          t0, 0(sp)
	ld          t0, -1384(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 74

	addi        sp, sp, 16

	// *** Basic block 75

.InitializeInstructions_label_636:

	// *** Basic block 76

.InitializeInstructions_label_637:

	// *** Basic block 77

.InitializeInstructions_label_638:
	lla         t0, .str.20
	sd          t0, -1376(s0)
	addi        t0, s0, -1376
	la          t1, Assemble_addd
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1376(s0)
	sd          t0, 0(sp)
	ld          t0, -1368(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 78

	addi        sp, sp, 16

	// *** Basic block 79

.InitializeInstructions_label_658:

	// *** Basic block 80

.InitializeInstructions_label_659:

	// *** Basic block 81

.InitializeInstructions_label_660:
	lla         t0, .str.21
	sd          t0, -1360(s0)
	addi        t0, s0, -1360
	la          t1, Assemble_addc
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1360(s0)
	sd          t0, 0(sp)
	ld          t0, -1352(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 82

	addi        sp, sp, 16

	// *** Basic block 83

.InitializeInstructions_label_680:

	// *** Basic block 84

.InitializeInstructions_label_681:

	// *** Basic block 85

.InitializeInstructions_label_682:
	lla         t0, .str.22
	sd          t0, -1344(s0)
	addi        t0, s0, -1344
	la          t1, Assemble_ldw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1344(s0)
	sd          t0, 0(sp)
	ld          t0, -1336(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 86

	addi        sp, sp, 16

	// *** Basic block 87

.InitializeInstructions_label_702:

	// *** Basic block 88

.InitializeInstructions_label_703:

	// *** Basic block 89

.InitializeInstructions_label_704:
	lla         t0, .str.23
	sd          t0, -1328(s0)
	addi        t0, s0, -1328
	la          t1, Assemble_ldh
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1328(s0)
	sd          t0, 0(sp)
	ld          t0, -1320(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 90

	addi        sp, sp, 16

	// *** Basic block 91

.InitializeInstructions_label_724:

	// *** Basic block 92

.InitializeInstructions_label_725:

	// *** Basic block 93

.InitializeInstructions_label_726:
	lla         t0, .str.24
	sd          t0, -1312(s0)
	addi        t0, s0, -1312
	la          t1, Assemble_ldb
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1312(s0)
	sd          t0, 0(sp)
	ld          t0, -1304(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 94

	addi        sp, sp, 16

	// *** Basic block 95

.InitializeInstructions_label_746:

	// *** Basic block 96

.InitializeInstructions_label_747:

	// *** Basic block 97

.InitializeInstructions_label_748:
	lla         t0, .str.25
	sd          t0, -1296(s0)
	addi        t0, s0, -1296
	la          t1, Assemble_lduw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1296(s0)
	sd          t0, 0(sp)
	ld          t0, -1288(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 98

	addi        sp, sp, 16

	// *** Basic block 99

.InitializeInstructions_label_768:

	// *** Basic block 100

.InitializeInstructions_label_769:

	// *** Basic block 101

.InitializeInstructions_label_770:
	lla         t0, .str.26
	sd          t0, -1280(s0)
	addi        t0, s0, -1280
	la          t1, Assemble_ldub
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1280(s0)
	sd          t0, 0(sp)
	ld          t0, -1272(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 102

	addi        sp, sp, 16

	// *** Basic block 103

.InitializeInstructions_label_790:

	// *** Basic block 104

.InitializeInstructions_label_791:

	// *** Basic block 105

.InitializeInstructions_label_792:
	lla         t0, .str.27
	sd          t0, -1264(s0)
	addi        t0, s0, -1264
	la          t1, Assemble_lduh
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1264(s0)
	sd          t0, 0(sp)
	ld          t0, -1256(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 106

	addi        sp, sp, 16

	// *** Basic block 107

.InitializeInstructions_label_812:

	// *** Basic block 108

.InitializeInstructions_label_813:

	// *** Basic block 109

.InitializeInstructions_label_814:
	lla         t0, .str.28
	sd          t0, -1248(s0)
	addi        t0, s0, -1248
	la          t1, Assemble_ldx
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1248(s0)
	sd          t0, 0(sp)
	ld          t0, -1240(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 110

	addi        sp, sp, 16

	// *** Basic block 111

.InitializeInstructions_label_834:

	// *** Basic block 112

.InitializeInstructions_label_835:

	// *** Basic block 113

.InitializeInstructions_label_836:
	lla         t0, .str.29
	sd          t0, -1232(s0)
	addi        t0, s0, -1232
	la          t1, Assemble_ldf
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1232(s0)
	sd          t0, 0(sp)
	ld          t0, -1224(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 114

	addi        sp, sp, 16

	// *** Basic block 115

.InitializeInstructions_label_856:

	// *** Basic block 116

.InitializeInstructions_label_857:

	// *** Basic block 117

.InitializeInstructions_label_858:
	lla         t0, .str.30
	sd          t0, -1216(s0)
	addi        t0, s0, -1216
	la          t1, Assemble_ldd
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1216(s0)
	sd          t0, 0(sp)
	ld          t0, -1208(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 118

	addi        sp, sp, 16

	// *** Basic block 119

.InitializeInstructions_label_878:

	// *** Basic block 120

.InitializeInstructions_label_879:

	// *** Basic block 121

.InitializeInstructions_label_880:
	lla         t0, .str.31
	sd          t0, -1200(s0)
	addi        t0, s0, -1200
	la          t1, Assemble_stw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1200(s0)
	sd          t0, 0(sp)
	ld          t0, -1192(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 122

	addi        sp, sp, 16

	// *** Basic block 123

.InitializeInstructions_label_900:

	// *** Basic block 124

.InitializeInstructions_label_901:

	// *** Basic block 125

.InitializeInstructions_label_902:
	lla         t0, .str.32
	sd          t0, -1184(s0)
	addi        t0, s0, -1184
	la          t1, Assemble_sth
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1184(s0)
	sd          t0, 0(sp)
	ld          t0, -1176(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 126

	addi        sp, sp, 16

	// *** Basic block 127

.InitializeInstructions_label_922:

	// *** Basic block 128

.InitializeInstructions_label_923:

	// *** Basic block 129

.InitializeInstructions_label_924:
	lla         t0, .str.33
	sd          t0, -1168(s0)
	addi        t0, s0, -1168
	la          t1, Assemble_stx
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1168(s0)
	sd          t0, 0(sp)
	ld          t0, -1160(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 130

	addi        sp, sp, 16

	// *** Basic block 131

.InitializeInstructions_label_944:

	// *** Basic block 132

.InitializeInstructions_label_945:

	// *** Basic block 133

.InitializeInstructions_label_946:
	lla         t0, .str.34
	sd          t0, -1152(s0)
	addi        t0, s0, -1152
	la          t1, Assemble_stf
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1152(s0)
	sd          t0, 0(sp)
	ld          t0, -1144(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 134

	addi        sp, sp, 16

	// *** Basic block 135

.InitializeInstructions_label_966:

	// *** Basic block 136

.InitializeInstructions_label_967:

	// *** Basic block 137

.InitializeInstructions_label_968:
	lla         t0, .str.35
	sd          t0, -1136(s0)
	addi        t0, s0, -1136
	la          t1, Assemble_std
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1136(s0)
	sd          t0, 0(sp)
	ld          t0, -1128(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 138

	addi        sp, sp, 16

	// *** Basic block 139

.InitializeInstructions_label_988:

	// *** Basic block 140

.InitializeInstructions_label_989:

	// *** Basic block 141

.InitializeInstructions_label_990:
	lla         t0, .str.36
	sd          t0, -1120(s0)
	addi        t0, s0, -1120
	la          t1, Assemble_stb
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1120(s0)
	sd          t0, 0(sp)
	ld          t0, -1112(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 142

	addi        sp, sp, 16

	// *** Basic block 143

.InitializeInstructions_label_1010:

	// *** Basic block 144

.InitializeInstructions_label_1011:

	// *** Basic block 145

.InitializeInstructions_label_1012:
	lla         t0, .str.37
	sd          t0, -1104(s0)
	addi        t0, s0, -1104
	la          t1, Assemble_sub
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1104(s0)
	sd          t0, 0(sp)
	ld          t0, -1096(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 146

	addi        sp, sp, 16

	// *** Basic block 147

.InitializeInstructions_label_1032:

	// *** Basic block 148

.InitializeInstructions_label_1033:

	// *** Basic block 149

.InitializeInstructions_label_1034:
	lla         t0, .str.38
	sd          t0, -1088(s0)
	addi        t0, s0, -1088
	la          t1, Assemble_subf
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1088(s0)
	sd          t0, 0(sp)
	ld          t0, -1080(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 150

	addi        sp, sp, 16

	// *** Basic block 151

.InitializeInstructions_label_1054:

	// *** Basic block 152

.InitializeInstructions_label_1055:

	// *** Basic block 153

.InitializeInstructions_label_1056:
	lla         t0, .str.39
	sd          t0, -1072(s0)
	addi        t0, s0, -1072
	la          t1, Assemble_subd
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1072(s0)
	sd          t0, 0(sp)
	ld          t0, -1064(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 154

	addi        sp, sp, 16

	// *** Basic block 155

.InitializeInstructions_label_1076:

	// *** Basic block 156

.InitializeInstructions_label_1077:

	// *** Basic block 157

.InitializeInstructions_label_1078:
	lla         t0, .str.40
	sd          t0, -1056(s0)
	addi        t0, s0, -1056
	la          t1, Assemble_mul
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1056(s0)
	sd          t0, 0(sp)
	ld          t0, -1048(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 158

	addi        sp, sp, 16

	// *** Basic block 159

.InitializeInstructions_label_1098:

	// *** Basic block 160

.InitializeInstructions_label_1099:

	// *** Basic block 161

.InitializeInstructions_label_1100:
	lla         t0, .str.41
	sd          t0, -1040(s0)
	addi        t0, s0, -1040
	la          t1, Assemble_mulf
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1040(s0)
	sd          t0, 0(sp)
	ld          t0, -1032(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 162

	addi        sp, sp, 16

	// *** Basic block 163

.InitializeInstructions_label_1120:

	// *** Basic block 164

.InitializeInstructions_label_1121:

	// *** Basic block 165

.InitializeInstructions_label_1122:
	lla         t0, .str.42
	sd          t0, -1024(s0)
	addi        t0, s0, -1024
	la          t1, Assemble_muld
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1024(s0)
	sd          t0, 0(sp)
	ld          t0, -1016(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 166

	addi        sp, sp, 16

	// *** Basic block 167

.InitializeInstructions_label_1142:

	// *** Basic block 168

.InitializeInstructions_label_1143:

	// *** Basic block 169

.InitializeInstructions_label_1144:
	lla         t0, .str.43
	sd          t0, -1008(s0)
	addi        t0, s0, -1008
	la          t1, Assemble_div
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1008(s0)
	sd          t0, 0(sp)
	ld          t0, -1000(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 170

	addi        sp, sp, 16

	// *** Basic block 171

.InitializeInstructions_label_1164:

	// *** Basic block 172

.InitializeInstructions_label_1165:

	// *** Basic block 173

.InitializeInstructions_label_1166:
	lla         t0, .str.44
	sd          t0, -992(s0)
	addi        t0, s0, -992
	la          t1, Assemble_divu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -992(s0)
	sd          t0, 0(sp)
	ld          t0, -984(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 174

	addi        sp, sp, 16

	// *** Basic block 175

.InitializeInstructions_label_1186:

	// *** Basic block 176

.InitializeInstructions_label_1187:

	// *** Basic block 177

.InitializeInstructions_label_1188:
	lla         t0, .str.45
	sd          t0, -976(s0)
	addi        t0, s0, -976
	la          t1, Assemble_divf
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -976(s0)
	sd          t0, 0(sp)
	ld          t0, -968(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 178

	addi        sp, sp, 16

	// *** Basic block 179

.InitializeInstructions_label_1208:

	// *** Basic block 180

.InitializeInstructions_label_1209:

	// *** Basic block 181

.InitializeInstructions_label_1210:
	lla         t0, .str.46
	sd          t0, -960(s0)
	addi        t0, s0, -960
	la          t1, Assemble_divd
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -960(s0)
	sd          t0, 0(sp)
	ld          t0, -952(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 182

	addi        sp, sp, 16

	// *** Basic block 183

.InitializeInstructions_label_1230:

	// *** Basic block 184

.InitializeInstructions_label_1231:

	// *** Basic block 185

.InitializeInstructions_label_1232:
	lla         t0, .str.47
	sd          t0, -944(s0)
	addi        t0, s0, -944
	la          t1, Assemble_mod
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -944(s0)
	sd          t0, 0(sp)
	ld          t0, -936(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 186

	addi        sp, sp, 16

	// *** Basic block 187

.InitializeInstructions_label_1252:

	// *** Basic block 188

.InitializeInstructions_label_1253:

	// *** Basic block 189

.InitializeInstructions_label_1254:
	lla         t0, .str.48
	sd          t0, -928(s0)
	addi        t0, s0, -928
	la          t1, Assemble_modu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -928(s0)
	sd          t0, 0(sp)
	ld          t0, -920(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 190

	addi        sp, sp, 16

	// *** Basic block 191

.InitializeInstructions_label_1274:

	// *** Basic block 192

.InitializeInstructions_label_1275:

	// *** Basic block 193

.InitializeInstructions_label_1276:
	lla         t0, .str.49
	sd          t0, -912(s0)
	addi        t0, s0, -912
	la          t1, Assemble_lsr
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -912(s0)
	sd          t0, 0(sp)
	ld          t0, -904(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 194

	addi        sp, sp, 16

	// *** Basic block 195

.InitializeInstructions_label_1296:

	// *** Basic block 196

.InitializeInstructions_label_1297:

	// *** Basic block 197

.InitializeInstructions_label_1298:
	lla         t0, .str.50
	sd          t0, -896(s0)
	addi        t0, s0, -896
	la          t1, Assemble_asr
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -896(s0)
	sd          t0, 0(sp)
	ld          t0, -888(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 198

	addi        sp, sp, 16

	// *** Basic block 199

.InitializeInstructions_label_1318:

	// *** Basic block 200

.InitializeInstructions_label_1319:

	// *** Basic block 201

.InitializeInstructions_label_1320:
	lla         t0, .str.51
	sd          t0, -880(s0)
	addi        t0, s0, -880
	la          t1, Assemble_lsl
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -880(s0)
	sd          t0, 0(sp)
	ld          t0, -872(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 202

	addi        sp, sp, 16

	// *** Basic block 203

.InitializeInstructions_label_1340:

	// *** Basic block 204

.InitializeInstructions_label_1341:

	// *** Basic block 205

.InitializeInstructions_label_1342:
	lla         t0, .str.52
	sd          t0, -864(s0)
	addi        t0, s0, -864
	la          t1, Assemble_or
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -864(s0)
	sd          t0, 0(sp)
	ld          t0, -856(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 206

	addi        sp, sp, 16

	// *** Basic block 207

.InitializeInstructions_label_1362:

	// *** Basic block 208

.InitializeInstructions_label_1363:

	// *** Basic block 209

.InitializeInstructions_label_1364:
	lla         t0, .str.53
	sd          t0, -848(s0)
	addi        t0, s0, -848
	la          t1, Assemble_and
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -848(s0)
	sd          t0, 0(sp)
	ld          t0, -840(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 210

	addi        sp, sp, 16

	// *** Basic block 211

.InitializeInstructions_label_1384:

	// *** Basic block 212

.InitializeInstructions_label_1385:

	// *** Basic block 213

.InitializeInstructions_label_1386:
	lla         t0, .str.54
	sd          t0, -832(s0)
	addi        t0, s0, -832
	la          t1, Assemble_xor
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -832(s0)
	sd          t0, 0(sp)
	ld          t0, -824(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 214

	addi        sp, sp, 16

	// *** Basic block 215

.InitializeInstructions_label_1406:

	// *** Basic block 216

.InitializeInstructions_label_1407:

	// *** Basic block 217

.InitializeInstructions_label_1408:
	lla         t0, .str.55
	sd          t0, -816(s0)
	addi        t0, s0, -816
	la          t1, Assemble_not
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -816(s0)
	sd          t0, 0(sp)
	ld          t0, -808(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 218

	addi        sp, sp, 16

	// *** Basic block 219

.InitializeInstructions_label_1428:

	// *** Basic block 220

.InitializeInstructions_label_1429:

	// *** Basic block 221

.InitializeInstructions_label_1430:
	lla         t0, .str.56
	sd          t0, -800(s0)
	addi        t0, s0, -800
	la          t1, Assemble_inv
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -800(s0)
	sd          t0, 0(sp)
	ld          t0, -792(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 222

	addi        sp, sp, 16

	// *** Basic block 223

.InitializeInstructions_label_1450:

	// *** Basic block 224

.InitializeInstructions_label_1451:

	// *** Basic block 225

.InitializeInstructions_label_1452:
	lla         t0, .str.57
	sd          t0, -784(s0)
	addi        t0, s0, -784
	la          t1, Assemble_neg
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -784(s0)
	sd          t0, 0(sp)
	ld          t0, -776(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 226

	addi        sp, sp, 16

	// *** Basic block 227

.InitializeInstructions_label_1472:

	// *** Basic block 228

.InitializeInstructions_label_1473:

	// *** Basic block 229

.InitializeInstructions_label_1474:
	lla         t0, .str.58
	sd          t0, -768(s0)
	addi        t0, s0, -768
	la          t1, Assemble_negf
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -768(s0)
	sd          t0, 0(sp)
	ld          t0, -760(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 230

	addi        sp, sp, 16

	// *** Basic block 231

.InitializeInstructions_label_1494:

	// *** Basic block 232

.InitializeInstructions_label_1495:

	// *** Basic block 233

.InitializeInstructions_label_1496:
	lla         t0, .str.59
	sd          t0, -752(s0)
	addi        t0, s0, -752
	la          t1, Assemble_negd
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -752(s0)
	sd          t0, 0(sp)
	ld          t0, -744(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 234

	addi        sp, sp, 16

	// *** Basic block 235

.InitializeInstructions_label_1516:

	// *** Basic block 236

.InitializeInstructions_label_1517:

	// *** Basic block 237

.InitializeInstructions_label_1518:
	lla         t0, .str.60
	sd          t0, -736(s0)
	addi        t0, s0, -736
	la          t1, Assemble_cmpeq
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -736(s0)
	sd          t0, 0(sp)
	ld          t0, -728(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 238

	addi        sp, sp, 16

	// *** Basic block 239

.InitializeInstructions_label_1538:

	// *** Basic block 240

.InitializeInstructions_label_1539:

	// *** Basic block 241

.InitializeInstructions_label_1540:
	lla         t0, .str.61
	sd          t0, -720(s0)
	addi        t0, s0, -720
	la          t1, Assemble_cmpne
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -720(s0)
	sd          t0, 0(sp)
	ld          t0, -712(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 242

	addi        sp, sp, 16

	// *** Basic block 243

.InitializeInstructions_label_1560:

	// *** Basic block 244

.InitializeInstructions_label_1561:

	// *** Basic block 245

.InitializeInstructions_label_1562:
	lla         t0, .str.62
	sd          t0, -704(s0)
	addi        t0, s0, -704
	la          t1, Assemble_cmplt
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -704(s0)
	sd          t0, 0(sp)
	ld          t0, -696(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 246

	addi        sp, sp, 16

	// *** Basic block 247

.InitializeInstructions_label_1582:

	// *** Basic block 248

.InitializeInstructions_label_1583:

	// *** Basic block 249

.InitializeInstructions_label_1584:
	lla         t0, .str.63
	sd          t0, -688(s0)
	addi        t0, s0, -688
	la          t1, Assemble_cmple
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -688(s0)
	sd          t0, 0(sp)
	ld          t0, -680(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 250

	addi        sp, sp, 16

	// *** Basic block 251

.InitializeInstructions_label_1604:

	// *** Basic block 252

.InitializeInstructions_label_1605:

	// *** Basic block 253

.InitializeInstructions_label_1606:
	lla         t0, .str.64
	sd          t0, -672(s0)
	addi        t0, s0, -672
	la          t1, Assemble_cmpgt
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -672(s0)
	sd          t0, 0(sp)
	ld          t0, -664(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 254

	addi        sp, sp, 16

	// *** Basic block 255

.InitializeInstructions_label_1626:

	// *** Basic block 256

.InitializeInstructions_label_1627:

	// *** Basic block 257

.InitializeInstructions_label_1628:
	lla         t0, .str.65
	sd          t0, -656(s0)
	addi        t0, s0, -656
	la          t1, Assemble_cmpge
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -656(s0)
	sd          t0, 0(sp)
	ld          t0, -648(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 258

	addi        sp, sp, 16

	// *** Basic block 259

.InitializeInstructions_label_1648:

	// *** Basic block 260

.InitializeInstructions_label_1649:

	// *** Basic block 261

.InitializeInstructions_label_1650:
	lla         t0, .str.66
	sd          t0, -640(s0)
	addi        t0, s0, -640
	la          t1, Assemble_cmpltu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -640(s0)
	sd          t0, 0(sp)
	ld          t0, -632(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 262

	addi        sp, sp, 16

	// *** Basic block 263

.InitializeInstructions_label_1670:

	// *** Basic block 264

.InitializeInstructions_label_1671:

	// *** Basic block 265

.InitializeInstructions_label_1672:
	lla         t0, .str.67
	sd          t0, -624(s0)
	addi        t0, s0, -624
	la          t1, Assemble_cmpleu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -624(s0)
	sd          t0, 0(sp)
	ld          t0, -616(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 266

	addi        sp, sp, 16

	// *** Basic block 267

.InitializeInstructions_label_1692:

	// *** Basic block 268

.InitializeInstructions_label_1693:

	// *** Basic block 269

.InitializeInstructions_label_1694:
	lla         t0, .str.68
	sd          t0, -608(s0)
	addi        t0, s0, -608
	la          t1, Assemble_cmpgtu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -608(s0)
	sd          t0, 0(sp)
	ld          t0, -600(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 270

	addi        sp, sp, 16

	// *** Basic block 271

.InitializeInstructions_label_1714:

	// *** Basic block 272

.InitializeInstructions_label_1715:

	// *** Basic block 273

.InitializeInstructions_label_1716:
	lla         t0, .str.69
	sd          t0, -592(s0)
	addi        t0, s0, -592
	la          t1, Assemble_cmpgeu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -592(s0)
	sd          t0, 0(sp)
	ld          t0, -584(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 274

	addi        sp, sp, 16

	// *** Basic block 275

.InitializeInstructions_label_1736:

	// *** Basic block 276

.InitializeInstructions_label_1737:

	// *** Basic block 277

.InitializeInstructions_label_1738:
	lla         t0, .str.70
	sd          t0, -576(s0)
	addi        t0, s0, -576
	la          t1, Assemble_cmpeqf
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -576(s0)
	sd          t0, 0(sp)
	ld          t0, -568(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 278

	addi        sp, sp, 16

	// *** Basic block 279

.InitializeInstructions_label_1758:

	// *** Basic block 280

.InitializeInstructions_label_1759:

	// *** Basic block 281

.InitializeInstructions_label_1760:
	lla         t0, .str.71
	sd          t0, -560(s0)
	addi        t0, s0, -560
	la          t1, Assemble_cmpnef
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -560(s0)
	sd          t0, 0(sp)
	ld          t0, -552(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 282

	addi        sp, sp, 16

	// *** Basic block 283

.InitializeInstructions_label_1780:

	// *** Basic block 284

.InitializeInstructions_label_1781:

	// *** Basic block 285

.InitializeInstructions_label_1782:
	lla         t0, .str.72
	sd          t0, -544(s0)
	addi        t0, s0, -544
	la          t1, Assemble_cmpltf
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -544(s0)
	sd          t0, 0(sp)
	ld          t0, -536(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 286

	addi        sp, sp, 16

	// *** Basic block 287

.InitializeInstructions_label_1802:

	// *** Basic block 288

.InitializeInstructions_label_1803:

	// *** Basic block 289

.InitializeInstructions_label_1804:
	lla         t0, .str.73
	sd          t0, -528(s0)
	addi        t0, s0, -528
	la          t1, Assemble_cmplef
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -528(s0)
	sd          t0, 0(sp)
	ld          t0, -520(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 290

	addi        sp, sp, 16

	// *** Basic block 291

.InitializeInstructions_label_1824:

	// *** Basic block 292

.InitializeInstructions_label_1825:

	// *** Basic block 293

.InitializeInstructions_label_1826:
	lla         t0, .str.74
	sd          t0, -512(s0)
	addi        t0, s0, -512
	la          t1, Assemble_cmpgtf
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -512(s0)
	sd          t0, 0(sp)
	ld          t0, -504(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 294

	addi        sp, sp, 16

	// *** Basic block 295

.InitializeInstructions_label_1846:

	// *** Basic block 296

.InitializeInstructions_label_1847:

	// *** Basic block 297

.InitializeInstructions_label_1848:
	lla         t0, .str.75
	sd          t0, -496(s0)
	addi        t0, s0, -496
	la          t1, Assemble_cmpgef
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -496(s0)
	sd          t0, 0(sp)
	ld          t0, -488(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 298

	addi        sp, sp, 16

	// *** Basic block 299

.InitializeInstructions_label_1868:

	// *** Basic block 300

.InitializeInstructions_label_1869:

	// *** Basic block 301

.InitializeInstructions_label_1870:
	lla         t0, .str.76
	sd          t0, -480(s0)
	addi        t0, s0, -480
	la          t1, Assemble_cmpeqd
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -480(s0)
	sd          t0, 0(sp)
	ld          t0, -472(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 302

	addi        sp, sp, 16

	// *** Basic block 303

.InitializeInstructions_label_1890:

	// *** Basic block 304

.InitializeInstructions_label_1891:

	// *** Basic block 305

.InitializeInstructions_label_1892:
	lla         t0, .str.77
	sd          t0, -464(s0)
	addi        t0, s0, -464
	la          t1, Assemble_cmpned
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -464(s0)
	sd          t0, 0(sp)
	ld          t0, -456(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 306

	addi        sp, sp, 16

	// *** Basic block 307

.InitializeInstructions_label_1912:

	// *** Basic block 308

.InitializeInstructions_label_1913:

	// *** Basic block 309

.InitializeInstructions_label_1914:
	lla         t0, .str.78
	sd          t0, -448(s0)
	addi        t0, s0, -448
	la          t1, Assemble_cmpltd
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -448(s0)
	sd          t0, 0(sp)
	ld          t0, -440(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 310

	addi        sp, sp, 16

	// *** Basic block 311

.InitializeInstructions_label_1934:

	// *** Basic block 312

.InitializeInstructions_label_1935:

	// *** Basic block 313

.InitializeInstructions_label_1936:
	lla         t0, .str.79
	sd          t0, -432(s0)
	addi        t0, s0, -432
	la          t1, Assemble_cmpled
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -432(s0)
	sd          t0, 0(sp)
	ld          t0, -424(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 314

	addi        sp, sp, 16

	// *** Basic block 315

.InitializeInstructions_label_1956:

	// *** Basic block 316

.InitializeInstructions_label_1957:

	// *** Basic block 317

.InitializeInstructions_label_1958:
	lla         t0, .str.80
	sd          t0, -416(s0)
	addi        t0, s0, -416
	la          t1, Assemble_cmpgtd
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -416(s0)
	sd          t0, 0(sp)
	ld          t0, -408(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 318

	addi        sp, sp, 16

	// *** Basic block 319

.InitializeInstructions_label_1978:

	// *** Basic block 320

.InitializeInstructions_label_1979:

	// *** Basic block 321

.InitializeInstructions_label_1980:
	lla         t0, .str.81
	sd          t0, -400(s0)
	addi        t0, s0, -400
	la          t1, Assemble_cmpged
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -400(s0)
	sd          t0, 0(sp)
	ld          t0, -392(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 322

	addi        sp, sp, 16

	// *** Basic block 323

.InitializeInstructions_label_2000:

	// *** Basic block 324

.InitializeInstructions_label_2001:

	// *** Basic block 325

.InitializeInstructions_label_2002:
	lla         t0, .str.82
	sd          t0, -384(s0)
	addi        t0, s0, -384
	la          t1, Assemble_bnz
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -384(s0)
	sd          t0, 0(sp)
	ld          t0, -376(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 326

	addi        sp, sp, 16

	// *** Basic block 327

.InitializeInstructions_label_2022:

	// *** Basic block 328

.InitializeInstructions_label_2023:

	// *** Basic block 329

.InitializeInstructions_label_2024:
	lla         t0, .str.83
	sd          t0, -368(s0)
	addi        t0, s0, -368
	la          t1, Assemble_bz
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -368(s0)
	sd          t0, 0(sp)
	ld          t0, -360(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 330

	addi        sp, sp, 16

	// *** Basic block 331

.InitializeInstructions_label_2044:

	// *** Basic block 332

.InitializeInstructions_label_2045:

	// *** Basic block 333

.InitializeInstructions_label_2046:
	lla         t0, .str.84
	sd          t0, -352(s0)
	addi        t0, s0, -352
	la          t1, Assemble_bra
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -352(s0)
	sd          t0, 0(sp)
	ld          t0, -344(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 334

	addi        sp, sp, 16

	// *** Basic block 335

.InitializeInstructions_label_2066:

	// *** Basic block 336

.InitializeInstructions_label_2067:

	// *** Basic block 337

.InitializeInstructions_label_2068:
	lla         t0, .str.85
	sd          t0, -336(s0)
	addi        t0, s0, -336
	la          t1, Assemble_cbra
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -336(s0)
	sd          t0, 0(sp)
	ld          t0, -328(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 338

	addi        sp, sp, 16

	// *** Basic block 339

.InitializeInstructions_label_2088:

	// *** Basic block 340

.InitializeInstructions_label_2089:

	// *** Basic block 341

.InitializeInstructions_label_2090:
	lla         t0, .str.86
	sd          t0, -320(s0)
	addi        t0, s0, -320
	la          t1, Assemble_i2f
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -320(s0)
	sd          t0, 0(sp)
	ld          t0, -312(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 342

	addi        sp, sp, 16

	// *** Basic block 343

.InitializeInstructions_label_2110:

	// *** Basic block 344

.InitializeInstructions_label_2111:

	// *** Basic block 345

.InitializeInstructions_label_2112:
	lla         t0, .str.87
	sd          t0, -304(s0)
	addi        t0, s0, -304
	la          t1, Assemble_i2d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -304(s0)
	sd          t0, 0(sp)
	ld          t0, -296(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 346

	addi        sp, sp, 16

	// *** Basic block 347

.InitializeInstructions_label_2132:

	// *** Basic block 348

.InitializeInstructions_label_2133:

	// *** Basic block 349

.InitializeInstructions_label_2134:
	lla         t0, .str.88
	sd          t0, -288(s0)
	addi        t0, s0, -288
	la          t1, Assemble_ui2f
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -288(s0)
	sd          t0, 0(sp)
	ld          t0, -280(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 350

	addi        sp, sp, 16

	// *** Basic block 351

.InitializeInstructions_label_2154:

	// *** Basic block 352

.InitializeInstructions_label_2155:

	// *** Basic block 353

.InitializeInstructions_label_2156:
	lla         t0, .str.89
	sd          t0, -272(s0)
	addi        t0, s0, -272
	la          t1, Assemble_ui2d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -272(s0)
	sd          t0, 0(sp)
	ld          t0, -264(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 354

	addi        sp, sp, 16

	// *** Basic block 355

.InitializeInstructions_label_2176:

	// *** Basic block 356

.InitializeInstructions_label_2177:

	// *** Basic block 357

.InitializeInstructions_label_2178:
	lla         t0, .str.90
	sd          t0, -256(s0)
	addi        t0, s0, -256
	la          t1, Assemble_f2d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -256(s0)
	sd          t0, 0(sp)
	ld          t0, -248(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 358

	addi        sp, sp, 16

	// *** Basic block 359

.InitializeInstructions_label_2198:

	// *** Basic block 360

.InitializeInstructions_label_2199:

	// *** Basic block 361

.InitializeInstructions_label_2200:
	lla         t0, .str.91
	sd          t0, -240(s0)
	addi        t0, s0, -240
	la          t1, Assemble_d2f
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -240(s0)
	sd          t0, 0(sp)
	ld          t0, -232(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 362

	addi        sp, sp, 16

	// *** Basic block 363

.InitializeInstructions_label_2220:

	// *** Basic block 364

.InitializeInstructions_label_2221:

	// *** Basic block 365

.InitializeInstructions_label_2222:
	lla         t0, .str.92
	sd          t0, -224(s0)
	addi        t0, s0, -224
	la          t1, Assemble_f2i
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -224(s0)
	sd          t0, 0(sp)
	ld          t0, -216(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 366

	addi        sp, sp, 16

	// *** Basic block 367

.InitializeInstructions_label_2242:

	// *** Basic block 368

.InitializeInstructions_label_2243:

	// *** Basic block 369

.InitializeInstructions_label_2244:
	lla         t0, .str.93
	sd          t0, -208(s0)
	addi        t0, s0, -208
	la          t1, Assemble_d2i
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -208(s0)
	sd          t0, 0(sp)
	ld          t0, -200(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 370

	addi        sp, sp, 16

	// *** Basic block 371

.InitializeInstructions_label_2264:

	// *** Basic block 372

.InitializeInstructions_label_2265:

	// *** Basic block 373

.InitializeInstructions_label_2266:
	lla         t0, .str.94
	sd          t0, -192(s0)
	addi        t0, s0, -192
	la          t1, Assemble_f2ui
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -192(s0)
	sd          t0, 0(sp)
	ld          t0, -184(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 374

	addi        sp, sp, 16

	// *** Basic block 375

.InitializeInstructions_label_2286:

	// *** Basic block 376

.InitializeInstructions_label_2287:

	// *** Basic block 377

.InitializeInstructions_label_2288:
	lla         t0, .str.95
	sd          t0, -176(s0)
	addi        t0, s0, -176
	la          t1, Assemble_d2ui
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -176(s0)
	sd          t0, 0(sp)
	ld          t0, -168(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 378

	addi        sp, sp, 16

	// *** Basic block 379

.InitializeInstructions_label_2308:

	// *** Basic block 380

.InitializeInstructions_label_2309:

	// *** Basic block 381

.InitializeInstructions_label_2310:
	lla         t0, .str.96
	sd          t0, -160(s0)
	addi        t0, s0, -160
	la          t1, Assemble_jmp
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -160(s0)
	sd          t0, 0(sp)
	ld          t0, -152(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 382

	addi        sp, sp, 16

	// *** Basic block 383

.InitializeInstructions_label_2330:

	// *** Basic block 384

.InitializeInstructions_label_2331:

	// *** Basic block 385

.InitializeInstructions_label_2332:
	lla         t0, .str.97
	sd          t0, -144(s0)
	addi        t0, s0, -144
	la          t1, Assemble_cjmp
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -144(s0)
	sd          t0, 0(sp)
	ld          t0, -136(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 386

	addi        sp, sp, 16

	// *** Basic block 387

.InitializeInstructions_label_2352:

	// *** Basic block 388

.InitializeInstructions_label_2353:

	// *** Basic block 389

.InitializeInstructions_label_2354:
	lla         t0, .str.98
	sd          t0, -128(s0)
	addi        t0, s0, -128
	la          t1, Assemble_adr
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -128(s0)
	sd          t0, 0(sp)
	ld          t0, -120(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 390

	addi        sp, sp, 16

	// *** Basic block 391

.InitializeInstructions_label_2374:

	// *** Basic block 392

.InitializeInstructions_label_2375:

	// *** Basic block 393

.InitializeInstructions_label_2376:
	lla         t0, .str.99
	sd          t0, -112(s0)
	addi        t0, s0, -112
	la          t1, Assemble_adrs
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -112(s0)
	sd          t0, 0(sp)
	ld          t0, -104(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 394

	addi        sp, sp, 16

	// *** Basic block 395

.InitializeInstructions_label_2396:

	// *** Basic block 396

.InitializeInstructions_label_2397:

	// *** Basic block 397

.InitializeInstructions_label_2398:
	lla         t0, .str.100
	sd          t0, -96(s0)
	addi        t0, s0, -96
	la          t1, Assemble_adrtls
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -96(s0)
	sd          t0, 0(sp)
	ld          t0, -88(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 398

	addi        sp, sp, 16

	// *** Basic block 399

.InitializeInstructions_label_2418:

	// *** Basic block 400

.InitializeInstructions_label_2419:

	// *** Basic block 401

.InitializeInstructions_label_2420:
	lla         t0, .str.101
	sd          t0, -80(s0)
	addi        t0, s0, -80
	la          t1, Assemble_call
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -80(s0)
	sd          t0, 0(sp)
	ld          t0, -72(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 402

	addi        sp, sp, 16

	// *** Basic block 403

.InitializeInstructions_label_2440:

	// *** Basic block 404

.InitializeInstructions_label_2441:

	// *** Basic block 405

.InitializeInstructions_label_2442:
	lla         t0, .str.102
	sd          t0, -64(s0)
	addi        t0, s0, -64
	la          t1, Assemble_rcall
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -64(s0)
	sd          t0, 0(sp)
	ld          t0, -56(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 406

	addi        sp, sp, 16

	// *** Basic block 407

.InitializeInstructions_label_2462:

	// *** Basic block 408

.InitializeInstructions_label_2463:

	// *** Basic block 409

.InitializeInstructions_label_2464:
	lla         t0, .str.103
	sd          t0, -48(s0)
	addi        t0, s0, -48
	la          t1, Assemble_ret
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -48(s0)
	sd          t0, 0(sp)
	ld          t0, -40(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 410

	addi        sp, sp, 16

	// *** Basic block 411

.InitializeInstructions_label_2484:

	// *** Basic block 412

.InitializeInstructions_label_2485:

	// *** Basic block 413

.InitializeInstructions_label_2486:
	lla         t0, .str.104
	sd          t0, -32(s0)
	addi        t0, s0, -32
	la          t1, Assemble_esc
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -32(s0)
	sd          t0, 0(sp)
	ld          t0, -24(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 415

.InitializeInstructions_label_2506:

	// *** Basic block 416

.InitializeInstructions_label_2507:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InitializeInstructions:
	.size InitializeInstructions, .func_end_InitializeInstructions-InitializeInstructions

	.global PCodeAssemblerInit
	.type PCodeAssemblerInit, @function

PCodeAssemblerInit:

	// *** Basic block 0

	.global AssemblerInit
	.local reloc_types
	.global MapInit
	.local CompareString
	.local InitializeInstructions
	.global AssemblerAddSection
	.global NewString
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
	mv          a2, x0
	li          a1, 6500		// 0x1964
	call        AssemblerInit

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .PCodeAssemblerInit_label_51

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.PCodeAssemblerInit_label_48:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.PCodeAssemblerInit_label_51:
	addi        a0, s1, 872
	la          t0, CompareString
	mv          a1, t0
	call        MapInit

	// *** Basic block 5

	addi        a0, s1, 872
	call        InitializeInstructions

	// *** Basic block 6

	mv          a4, x0
	mv          a3, x0
	mv          a2, x0
	mv          a1, x0
	mv          a0, s1
	call        AssemblerAddSection

	// *** Basic block 7

	lla         a0, .str.105
	call        NewString

	// *** Basic block 8

	li          t0, 8		// 0x8 ASCII \x8
	mv          a4, t0
	li          t1, 3		// 0x3 ASCII \x3
	mv          a3, t1
	mv          a2, t0
	mv          a1, a0
	mv          a0, s1
	call        AssemblerAddSection

	// *** Basic block 9

	sw          a0, 904(s1)
	li          a0, 1		// 0x1 ASCII \x1
	j           .PCodeAssemblerInit_label_48
.func_end_PCodeAssemblerInit:
	.size PCodeAssemblerInit, .func_end_PCodeAssemblerInit-PCodeAssemblerInit

	.global NewPCodeAssembler
	.type NewPCodeAssembler, @function

NewPCodeAssembler:

	// *** Basic block 0

	.global malloc
	.global PCodeAssemblerInit
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
	li          a0, 912		// 0x390
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        PCodeAssemblerInit

	// *** Basic block 2

	mv          a0, s3

	// *** Basic block 3

.NewPCodeAssembler_label_26:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewPCodeAssembler:
	.size NewPCodeAssembler, .func_end_NewPCodeAssembler-NewPCodeAssembler

	.global PCodeAssemblerDestruct
	.type PCodeAssemblerDestruct, @function

PCodeAssemblerDestruct:

	// *** Basic block 0

	.global AssemblerDestruct
	.global MapDestruct
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
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           MapDestruct
.func_end_PCodeAssemblerDestruct:
	.size PCodeAssemblerDestruct, .func_end_PCodeAssemblerDestruct-PCodeAssemblerDestruct

	.global PCodeAssemblerDelete
	.type PCodeAssemblerDelete, @function

PCodeAssemblerDelete:

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
	.global PCodeAssemblerDestruct
	.global free
	mv          s1, a0
	call        PCodeAssemblerDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_PCodeAssemblerDelete:
	.size PCodeAssemblerDelete, .func_end_PCodeAssemblerDelete-PCodeAssemblerDelete

	.global AssemblePCodeInstruction
	.type AssemblePCodeInstruction, @function

AssemblePCodeInstruction:

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
	beq         s3, x0, .AssemblePCodeInstruction_label_35

	// *** Basic block 2

	mv          s4, s3
	mv          a0, s1
	jalr         x1, s4, 0

	// *** Basic block 3

	j           .AssemblePCodeInstruction_label_46

	// *** Basic block 4

.AssemblePCodeInstruction_label_35:
	lla         a1, .str.106
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

.AssemblePCodeInstruction_label_46:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AssemblePCodeInstruction:
	.size AssemblePCodeInstruction, .func_end_AssemblePCodeInstruction-AssemblePCodeInstruction

	.local  RegNumber
	.type RegNumber, @function

RegNumber:

	// *** Basic block 0

	.global AssemblerError
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
	li          s4, 1		// 0x1 ASCII \x1
	ld          s5, 24(s1)
	li          s6, 1		// 0x1 ASCII \x1
	bne         s5, s6, .RegNumber_label_39

	// *** Basic block 1

	ld          s7, 16(s1)
	lla         a1, .str.107
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 2

.RegNumber_label_39:
	bge         s6, s5, .RegNumber_label_57

	// *** Basic block 3

.RegNumber_label_44:
	slli        t0, s3, 1
	slli        t1, s3, 3
	add         t0, t0, t1
	add         t1, s7, s4
	lb          t1, 0(t1)
	add         t0, t0, t1
	addi        s3, t0, -48
	addi        s4, s4, 1
	blt         s4, s5, .RegNumber_label_44

	// *** Basic block 4

.RegNumber_label_57:
	li          t0, 255		// 0xff
	bge         t0, s3, .RegNumber_label_71

	// *** Basic block 5

	lla         a1, .str.108
	mv          a2, s3
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 6

.RegNumber_label_71:
	mv          a0, s3

	// *** Basic block 7

.RegNumber_label_74:
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
.func_end_RegNumber:
	.size RegNumber, .func_end_RegNumber-RegNumber

	.local  RegisterName
	.type RegisterName, @function

RegisterName:

	// *** Basic block 0

	.global LexLookingAt
	.global StringInit
	.global LexNextToken
	.global StringEqual
	.local RegNumber
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
	mv          s3, a2
	addi        a0, s1, 176
	li          a1, 3		// 0x3 ASCII \x3
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .RegisterName_label_185

	// *** Basic block 2

	addi        a0, s0, -64
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 3

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 4

	addi        a0, s0, -64
	lla         a1, .str.109
	call        StringEqual

	// *** Basic block 5

	beqz        a0, .RegisterName_label_73

	// *** Basic block 6

	li          t0, 30		// 0x1e ASCII \x1e
	sw          t0, 0(s2)
	li          t0, 105		// 0x69 ASCII 'i'
	sb          t0, 0(s3)
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 7

.RegisterName_label_70:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 8

.RegisterName_label_73:
	addi        a0, s0, -64
	lla         a1, .str.110
	call        StringEqual

	// *** Basic block 9

	beqz        a0, .RegisterName_label_88

	// *** Basic block 10

	li          t0, 31		// 0x1f ASCII \x1f
	sw          t0, 0(s2)
	li          t0, 105		// 0x69 ASCII 'i'
	sb          t0, 0(s3)
	li          a0, 1		// 0x1 ASCII \x1
	j           .RegisterName_label_70

	// *** Basic block 11

.RegisterName_label_88:
	addi        a0, s0, -64
	lla         a1, .str.111
	call        StringEqual

	// *** Basic block 12

	beqz        a0, .RegisterName_label_103

	// *** Basic block 13

	li          t0, 33		// 0x21 ASCII '!'
	sw          t0, 0(s2)
	li          t0, 105		// 0x69 ASCII 'i'
	sb          t0, 0(s3)
	li          a0, 1		// 0x1 ASCII \x1
	j           .RegisterName_label_70

	// *** Basic block 14

.RegisterName_label_103:
	addi        a0, s0, -64
	lla         a1, .str.112
	call        StringEqual

	// *** Basic block 15

	beqz        a0, .RegisterName_label_118

	// *** Basic block 16

	li          t0, 34		// 0x22 ASCII '"'
	sw          t0, 0(s2)
	li          t0, 105		// 0x69 ASCII 'i'
	sb          t0, 0(s3)
	li          a0, 1		// 0x1 ASCII \x1
	j           .RegisterName_label_70

	// *** Basic block 17

.RegisterName_label_118:
	addi        t0, s0, -64
	ld          t0, 16(t0)
	lb          s4, 0(t0)
	li          t0, 68		// 0x44 ASCII 'D'
	beq         s4, t0, .RegisterName_label_168

	// *** Basic block 18

	li          t0, 70		// 0x46 ASCII 'F'
	beq         s4, t0, .RegisterName_label_163

	// *** Basic block 19

	li          t0, 82		// 0x52 ASCII 'R'
	beq         s4, t0, .RegisterName_label_158

	// *** Basic block 20

	li          t0, 100		// 0x64 ASCII 'd'
	beq         s4, t0, .RegisterName_label_167

	// *** Basic block 21

	li          t0, 102		// 0x66 ASCII 'f'
	beq         s4, t0, .RegisterName_label_162

	// *** Basic block 22

	li          t0, 114		// 0x72 ASCII 'r'
	beq         s4, t0, .RegisterName_label_157

	// *** Basic block 23

.RegisterName_label_153:
	mv          a0, x0
	j           .RegisterName_label_70

	// *** Basic block 24

.RegisterName_label_157:

	// *** Basic block 25

.RegisterName_label_158:
	li          t0, 105		// 0x69 ASCII 'i'
	sb          t0, 0(s3)
	j           .RegisterName_label_172

	// *** Basic block 26

.RegisterName_label_162:

	// *** Basic block 27

.RegisterName_label_163:
	li          t0, 102		// 0x66 ASCII 'f'
	sb          t0, 0(s3)
	j           .RegisterName_label_172

	// *** Basic block 28

.RegisterName_label_167:

	// *** Basic block 29

.RegisterName_label_168:
	li          t0, 100		// 0x64 ASCII 'd'
	sb          t0, 0(s3)
	j           .RegisterName_label_172

	// *** Basic block 30

.RegisterName_label_172:
	addi        a1, s0, -64
	mv          a0, s1
	call        RegNumber

	// *** Basic block 31

	sw          a0, 0(s2)
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 32

	li          a0, 1		// 0x1 ASCII \x1
	j           .RegisterName_label_70

	// *** Basic block 33

.RegisterName_label_185:
	mv          a0, x0
	j           .RegisterName_label_70
.func_end_RegisterName:
	.size RegisterName, .func_end_RegisterName-RegisterName

	.local  Register
	.type Register, @function

Register:

	// *** Basic block 0

	.local RegisterName
	.global AssemblerError
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
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a2
	mv          s3, a1
	addi        a1, s0, -32
	addi        a2, s0, -28
	call        RegisterName

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .Register_label_42

	// *** Basic block 2

	lla         a1, .str.113
	mv          a2, s2
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 3

	mv          a0, x0

	// *** Basic block 4

.Register_label_39:
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

.Register_label_42:
	lb          s4, -28(s0)
	beq         s4, s3, .Register_label_61

	// *** Basic block 6

	lla         a1, .str.114
	mv          a3, s3
	mv          a2, s4
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 7

	mv          a0, x0
	j           .Register_label_39

	// *** Basic block 8

.Register_label_61:
	lw          a0, -32(s0)
	j           .Register_label_39
.func_end_Register:
	.size Register, .func_end_Register-Register

	.local  ParseRegisterTriple
	.type ParseRegisterTriple, @function

ParseRegisterTriple:

	// *** Basic block 0

	.local Register
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
	mv          s1, a3
	mv          s2, a0
	mv          s3, a1
	mv          s4, a2
	call        Register

	// *** Basic block 1

	sw          a0, 0(s1)
	addi        a0, s2, 176
	li          s5, 18		// 0x12 ASCII \x12
	mv          a1, s5
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .ParseRegisterTriple_label_54

	// *** Basic block 3

	lla         a1, .str.115
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 4

	mv          a0, x0

	// *** Basic block 5

.ParseRegisterTriple_label_51:
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

	// *** Basic block 6

.ParseRegisterTriple_label_54:
	mv          a2, s4
	mv          a1, s3
	mv          a0, s2
	call        Register

	// *** Basic block 7

	sw          a0, 4(s1)
	addi        a0, s2, 176
	mv          a1, s5
	call        LexMatch

	// *** Basic block 8

	not         t0, a0
	beqz        t0, .ParseRegisterTriple_label_81

	// *** Basic block 9

	lla         a1, .str.116
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 10

	mv          a0, x0
	j           .ParseRegisterTriple_label_51

	// *** Basic block 11

.ParseRegisterTriple_label_81:
	mv          a2, s4
	mv          a1, s3
	mv          a0, s2
	call        Register

	// *** Basic block 12

	sw          a0, 8(s1)
	li          a0, 1		// 0x1 ASCII \x1
	j           .ParseRegisterTriple_label_51
.func_end_ParseRegisterTriple:
	.size ParseRegisterTriple, .func_end_ParseRegisterTriple-ParseRegisterTriple

	.local  ParseComparisonRegisterTriple
	.type ParseComparisonRegisterTriple, @function

ParseComparisonRegisterTriple:

	// *** Basic block 0

	.local Register
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
	mv          s1, a3
	mv          s2, a0
	mv          s3, a1
	mv          s4, a2
	lla         a2, .str.117
	li          a1, 105		// 0x69 ASCII 'i'
	call        Register

	// *** Basic block 1

	sw          a0, 0(s1)
	addi        a0, s2, 176
	li          s5, 18		// 0x12 ASCII \x12
	mv          a1, s5
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .ParseComparisonRegisterTriple_label_57

	// *** Basic block 3

	lla         a1, .str.118
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 4

	mv          a0, x0

	// *** Basic block 5

.ParseComparisonRegisterTriple_label_54:
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

	// *** Basic block 6

.ParseComparisonRegisterTriple_label_57:
	mv          a2, s4
	mv          a1, s3
	mv          a0, s2
	call        Register

	// *** Basic block 7

	sw          a0, 4(s1)
	addi        a0, s2, 176
	mv          a1, s5
	call        LexMatch

	// *** Basic block 8

	not         t0, a0
	beqz        t0, .ParseComparisonRegisterTriple_label_84

	// *** Basic block 9

	lla         a1, .str.119
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 10

	mv          a0, x0
	j           .ParseComparisonRegisterTriple_label_54

	// *** Basic block 11

.ParseComparisonRegisterTriple_label_84:
	mv          a2, s4
	mv          a1, s3
	mv          a0, s2
	call        Register

	// *** Basic block 12

	sw          a0, 8(s1)
	li          a0, 1		// 0x1 ASCII \x1
	j           .ParseComparisonRegisterTriple_label_54
.func_end_ParseComparisonRegisterTriple:
	.size ParseComparisonRegisterTriple, .func_end_ParseComparisonRegisterTriple-ParseComparisonRegisterTriple

	.local  ParseRegisterPair
	.type ParseRegisterPair, @function

ParseRegisterPair:

	// *** Basic block 0

	.local Register
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
	mv          s1, a3
	mv          s2, a0
	mv          s3, a1
	mv          s4, a2
	call        Register

	// *** Basic block 1

	sw          a0, 0(s1)
	addi        a0, s2, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .ParseRegisterPair_label_53

	// *** Basic block 3

	lla         a1, .str.120
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 4

	mv          a0, x0

	// *** Basic block 5

.ParseRegisterPair_label_50:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.ParseRegisterPair_label_53:
	mv          a2, s4
	mv          a1, s3
	mv          a0, s2
	call        Register

	// *** Basic block 7

	sw          a0, 4(s1)
	sw          x0, 8(s1)
	li          a0, 1		// 0x1 ASCII \x1
	j           .ParseRegisterPair_label_50
.func_end_ParseRegisterPair:
	.size ParseRegisterPair, .func_end_ParseRegisterPair-ParseRegisterPair

	.local  AssembleALU
	.type AssembleALU, @function

AssembleALU:

	// *** Basic block 0

	.global AssemblerEmitWord
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a2
	mv          t2, a0
	slli        t3, t0, 24
	lw          t4, 0(t1)
	slli        t4, t4, 16
	or          t3, t3, t4
	lw          t4, 4(t1)
	slli        t4, t4, 8
	or          t3, t3, t4
	lw          t4, 8(t1)
	or          t5, t3, t4
	lw          a1, 744(t2)
	mv          a2, t5
	j           AssemblerEmitWord
.func_end_AssembleALU:
	.size AssembleALU, .func_end_AssembleALU-AssembleALU

	.local  Assemble_add
	.type Assemble_add, @function

Assemble_add:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.121
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_add_label_35

	// *** Basic block 2

	mv          a2, s2
	mv          a1, x0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_add_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_add:
	.size Assemble_add, .func_end_Assemble_add-Assemble_add

	.local  Assemble_sub
	.type Assemble_sub, @function

Assemble_sub:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.122
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_sub_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_sub_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_sub:
	.size Assemble_sub, .func_end_Assemble_sub-Assemble_sub

	.local  Assemble_addf
	.type Assemble_addf, @function

Assemble_addf:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.123
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 102		// 0x66 ASCII 'f'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_addf_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_addf_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_addf:
	.size Assemble_addf, .func_end_Assemble_addf-Assemble_addf

	.local  Assemble_addd
	.type Assemble_addd, @function

Assemble_addd:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.124
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 100		// 0x64 ASCII 'd'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_addd_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_addd_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_addd:
	.size Assemble_addd, .func_end_Assemble_addd-Assemble_addd

	.local  Assemble_subf
	.type Assemble_subf, @function

Assemble_subf:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.125
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 102		// 0x66 ASCII 'f'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_subf_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_subf_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_subf:
	.size Assemble_subf, .func_end_Assemble_subf-Assemble_subf

	.local  Assemble_subd
	.type Assemble_subd, @function

Assemble_subd:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.126
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 100		// 0x64 ASCII 'd'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_subd_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 5		// 0x5 ASCII \x5
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_subd_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_subd:
	.size Assemble_subd, .func_end_Assemble_subd-Assemble_subd

	.local  Assemble_mul
	.type Assemble_mul, @function

Assemble_mul:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.127
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_mul_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 6		// 0x6 ASCII \x6
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_mul_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_mul:
	.size Assemble_mul, .func_end_Assemble_mul-Assemble_mul

	.local  Assemble_mulf
	.type Assemble_mulf, @function

Assemble_mulf:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.128
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 102		// 0x66 ASCII 'f'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_mulf_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 7		// 0x7 ASCII \x7
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_mulf_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_mulf:
	.size Assemble_mulf, .func_end_Assemble_mulf-Assemble_mulf

	.local  Assemble_muld
	.type Assemble_muld, @function

Assemble_muld:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.129
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 100		// 0x64 ASCII 'd'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_muld_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_muld_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_muld:
	.size Assemble_muld, .func_end_Assemble_muld-Assemble_muld

	.local  Assemble_div
	.type Assemble_div, @function

Assemble_div:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.130
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_div_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 9		// 0x9 ASCII \x9
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_div_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_div:
	.size Assemble_div, .func_end_Assemble_div-Assemble_div

	.local  Assemble_divu
	.type Assemble_divu, @function

Assemble_divu:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.131
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_divu_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 10		// 0xa ASCII \xa
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_divu_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_divu:
	.size Assemble_divu, .func_end_Assemble_divu-Assemble_divu

	.local  Assemble_divf
	.type Assemble_divf, @function

Assemble_divf:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.132
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 102		// 0x66 ASCII 'f'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_divf_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 11		// 0xb ASCII \xb
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_divf_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_divf:
	.size Assemble_divf, .func_end_Assemble_divf-Assemble_divf

	.local  Assemble_divd
	.type Assemble_divd, @function

Assemble_divd:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.133
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 100		// 0x64 ASCII 'd'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_divd_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 12		// 0xc ASCII \xc
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_divd_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_divd:
	.size Assemble_divd, .func_end_Assemble_divd-Assemble_divd

	.local  Assemble_mod
	.type Assemble_mod, @function

Assemble_mod:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.134
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_mod_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 13		// 0xd ASCII \xd
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_mod_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_mod:
	.size Assemble_mod, .func_end_Assemble_mod-Assemble_mod

	.local  Assemble_modu
	.type Assemble_modu, @function

Assemble_modu:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.135
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_modu_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 14		// 0xe ASCII \xe
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_modu_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_modu:
	.size Assemble_modu, .func_end_Assemble_modu-Assemble_modu

	.local  Assemble_lsr
	.type Assemble_lsr, @function

Assemble_lsr:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.136
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_lsr_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 15		// 0xf ASCII \xf
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_lsr_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_lsr:
	.size Assemble_lsr, .func_end_Assemble_lsr-Assemble_lsr

	.local  Assemble_asr
	.type Assemble_asr, @function

Assemble_asr:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.137
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_asr_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 16		// 0x10 ASCII \x10
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_asr_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_asr:
	.size Assemble_asr, .func_end_Assemble_asr-Assemble_asr

	.local  Assemble_lsl
	.type Assemble_lsl, @function

Assemble_lsl:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.138
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_lsl_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 17		// 0x11 ASCII \x11
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_lsl_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_lsl:
	.size Assemble_lsl, .func_end_Assemble_lsl-Assemble_lsl

	.local  Assemble_or
	.type Assemble_or, @function

Assemble_or:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.139
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_or_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_or_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_or:
	.size Assemble_or, .func_end_Assemble_or-Assemble_or

	.local  Assemble_and
	.type Assemble_and, @function

Assemble_and:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.140
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_and_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_and_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_and:
	.size Assemble_and, .func_end_Assemble_and-Assemble_and

	.local  Assemble_xor
	.type Assemble_xor, @function

Assemble_xor:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.141
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_xor_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 20		// 0x14 ASCII \x14
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_xor_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_xor:
	.size Assemble_xor, .func_end_Assemble_xor-Assemble_xor

	.local  Assemble_not
	.type Assemble_not, @function

Assemble_not:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALU
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
	lla         a2, .str.142
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_not_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 21		// 0x15 ASCII \x15
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_not_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_not:
	.size Assemble_not, .func_end_Assemble_not-Assemble_not

	.local  Assemble_inv
	.type Assemble_inv, @function

Assemble_inv:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALU
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
	lla         a2, .str.143
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_inv_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 22		// 0x16 ASCII \x16
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_inv_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_inv:
	.size Assemble_inv, .func_end_Assemble_inv-Assemble_inv

	.local  Assemble_neg
	.type Assemble_neg, @function

Assemble_neg:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALU
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
	lla         a2, .str.144
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_neg_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 23		// 0x17 ASCII \x17
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_neg_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_neg:
	.size Assemble_neg, .func_end_Assemble_neg-Assemble_neg

	.local  Assemble_negf
	.type Assemble_negf, @function

Assemble_negf:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALU
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
	lla         a2, .str.145
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 102		// 0x66 ASCII 'f'
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_negf_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 24		// 0x18 ASCII \x18
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_negf_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_negf:
	.size Assemble_negf, .func_end_Assemble_negf-Assemble_negf

	.local  Assemble_negd
	.type Assemble_negd, @function

Assemble_negd:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALU
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
	lla         a2, .str.146
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 100		// 0x64 ASCII 'd'
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_negd_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 25		// 0x19 ASCII \x19
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_negd_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_negd:
	.size Assemble_negd, .func_end_Assemble_negd-Assemble_negd

	.local  Assemble_cmpeq
	.type Assemble_cmpeq, @function

Assemble_cmpeq:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.147
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpeq_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 26		// 0x1a ASCII \x1a
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpeq_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpeq:
	.size Assemble_cmpeq, .func_end_Assemble_cmpeq-Assemble_cmpeq

	.local  Assemble_cmpne
	.type Assemble_cmpne, @function

Assemble_cmpne:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.148
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpne_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 27		// 0x1b ASCII \x1b
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpne_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpne:
	.size Assemble_cmpne, .func_end_Assemble_cmpne-Assemble_cmpne

	.local  Assemble_cmplt
	.type Assemble_cmplt, @function

Assemble_cmplt:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.149
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmplt_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 28		// 0x1c ASCII \x1c
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmplt_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmplt:
	.size Assemble_cmplt, .func_end_Assemble_cmplt-Assemble_cmplt

	.local  Assemble_cmple
	.type Assemble_cmple, @function

Assemble_cmple:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.150
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmple_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmple_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmple:
	.size Assemble_cmple, .func_end_Assemble_cmple-Assemble_cmple

	.local  Assemble_cmpgt
	.type Assemble_cmpgt, @function

Assemble_cmpgt:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.151
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpgt_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 30		// 0x1e ASCII \x1e
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpgt_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpgt:
	.size Assemble_cmpgt, .func_end_Assemble_cmpgt-Assemble_cmpgt

	.local  Assemble_cmpge
	.type Assemble_cmpge, @function

Assemble_cmpge:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.152
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpge_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 31		// 0x1f ASCII \x1f
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpge_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpge:
	.size Assemble_cmpge, .func_end_Assemble_cmpge-Assemble_cmpge

	.local  Assemble_cmpltu
	.type Assemble_cmpltu, @function

Assemble_cmpltu:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.153
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpltu_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 32		// 0x20 ASCII ' '
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpltu_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpltu:
	.size Assemble_cmpltu, .func_end_Assemble_cmpltu-Assemble_cmpltu

	.local  Assemble_cmpleu
	.type Assemble_cmpleu, @function

Assemble_cmpleu:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.154
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpleu_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 33		// 0x21 ASCII '!'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpleu_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpleu:
	.size Assemble_cmpleu, .func_end_Assemble_cmpleu-Assemble_cmpleu

	.local  Assemble_cmpgtu
	.type Assemble_cmpgtu, @function

Assemble_cmpgtu:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.155
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpgtu_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 34		// 0x22 ASCII '"'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpgtu_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpgtu:
	.size Assemble_cmpgtu, .func_end_Assemble_cmpgtu-Assemble_cmpgtu

	.local  Assemble_cmpgeu
	.type Assemble_cmpgeu, @function

Assemble_cmpgeu:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.156
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpgeu_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 35		// 0x23 ASCII '#'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpgeu_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpgeu:
	.size Assemble_cmpgeu, .func_end_Assemble_cmpgeu-Assemble_cmpgeu

	.local  Assemble_cmpeqf
	.type Assemble_cmpeqf, @function

Assemble_cmpeqf:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.157
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 102		// 0x66 ASCII 'f'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpeqf_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 36		// 0x24 ASCII '$'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpeqf_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpeqf:
	.size Assemble_cmpeqf, .func_end_Assemble_cmpeqf-Assemble_cmpeqf

	.local  Assemble_cmpnef
	.type Assemble_cmpnef, @function

Assemble_cmpnef:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.158
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 102		// 0x66 ASCII 'f'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpnef_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 37		// 0x25 ASCII '%'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpnef_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpnef:
	.size Assemble_cmpnef, .func_end_Assemble_cmpnef-Assemble_cmpnef

	.local  Assemble_cmpltf
	.type Assemble_cmpltf, @function

Assemble_cmpltf:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.159
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 102		// 0x66 ASCII 'f'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpltf_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 38		// 0x26 ASCII '&'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpltf_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpltf:
	.size Assemble_cmpltf, .func_end_Assemble_cmpltf-Assemble_cmpltf

	.local  Assemble_cmplef
	.type Assemble_cmplef, @function

Assemble_cmplef:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.160
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 102		// 0x66 ASCII 'f'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmplef_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 39		// 0x27 ASCII '''
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmplef_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmplef:
	.size Assemble_cmplef, .func_end_Assemble_cmplef-Assemble_cmplef

	.local  Assemble_cmpgtf
	.type Assemble_cmpgtf, @function

Assemble_cmpgtf:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.161
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 102		// 0x66 ASCII 'f'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpgtf_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 40		// 0x28 ASCII '('
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpgtf_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpgtf:
	.size Assemble_cmpgtf, .func_end_Assemble_cmpgtf-Assemble_cmpgtf

	.local  Assemble_cmpgef
	.type Assemble_cmpgef, @function

Assemble_cmpgef:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.162
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 102		// 0x66 ASCII 'f'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpgef_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 41		// 0x29 ASCII ')'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpgef_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpgef:
	.size Assemble_cmpgef, .func_end_Assemble_cmpgef-Assemble_cmpgef

	.local  Assemble_cmpeqd
	.type Assemble_cmpeqd, @function

Assemble_cmpeqd:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.163
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 100		// 0x64 ASCII 'd'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpeqd_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 42		// 0x2a ASCII '*'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpeqd_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpeqd:
	.size Assemble_cmpeqd, .func_end_Assemble_cmpeqd-Assemble_cmpeqd

	.local  Assemble_cmpned
	.type Assemble_cmpned, @function

Assemble_cmpned:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.164
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 100		// 0x64 ASCII 'd'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpned_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 43		// 0x2b ASCII '+'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpned_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpned:
	.size Assemble_cmpned, .func_end_Assemble_cmpned-Assemble_cmpned

	.local  Assemble_cmpltd
	.type Assemble_cmpltd, @function

Assemble_cmpltd:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.165
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 100		// 0x64 ASCII 'd'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpltd_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 44		// 0x2c ASCII ','
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpltd_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpltd:
	.size Assemble_cmpltd, .func_end_Assemble_cmpltd-Assemble_cmpltd

	.local  Assemble_cmpled
	.type Assemble_cmpled, @function

Assemble_cmpled:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.166
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 100		// 0x64 ASCII 'd'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpled_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpled_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpled:
	.size Assemble_cmpled, .func_end_Assemble_cmpled-Assemble_cmpled

	.local  Assemble_cmpgtd
	.type Assemble_cmpgtd, @function

Assemble_cmpgtd:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.167
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 100		// 0x64 ASCII 'd'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpgtd_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 46		// 0x2e ASCII '.'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpgtd_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpgtd:
	.size Assemble_cmpgtd, .func_end_Assemble_cmpgtd-Assemble_cmpgtd

	.local  Assemble_cmpged
	.type Assemble_cmpged, @function

Assemble_cmpged:

	// *** Basic block 0

	.local ParseComparisonRegisterTriple
	.local AssembleALU
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
	lla         a2, .str.168
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 100		// 0x64 ASCII 'd'
	call        ParseComparisonRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_cmpged_label_35

	// *** Basic block 2

	mv          a2, s2
	li          t0, 47		// 0x2f ASCII '/'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALU

	// *** Basic block 3

.Assemble_cmpged_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_cmpged:
	.size Assemble_cmpged, .func_end_Assemble_cmpged-Assemble_cmpged

	.local  AssembleConversion
	.type AssembleConversion, @function

AssembleConversion:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global AssemblerEmitWord
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
	mv          s2, a2
	mv          s3, a3
	mv          s4, a1
	mv          a2, a5
	mv          a1, a4
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .AssembleConversion_label_61

	// *** Basic block 3

	lla         a1, .str.169
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 4

.AssembleConversion_label_58:
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

.AssembleConversion_label_61:
	addi        s5, s0, -32
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        Register

	// *** Basic block 6

	sw          a0, 4(s5)
	lw          a1, 744(s1)
	slli        t0, s4, 24
	lw          t1, -32(s0)
	slli        t1, t1, 16
	or          t0, t0, t1
	lw          t1, 4(s5)
	slli        t1, t1, 8
	or          a2, t0, t1
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 7

	j           .AssembleConversion_label_58
.func_end_AssembleConversion:
	.size AssembleConversion, .func_end_AssembleConversion-AssembleConversion

	.local  Assemble_i2f
	.type Assemble_i2f, @function

Assemble_i2f:

	// *** Basic block 0

	.local AssembleConversion
	// Leaf procedure, no stack frame generated
	lla         a3, .str.170
	lla         a5, .str.171
	li          a4, 102		// 0x66 ASCII 'f'
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 63		// 0x3f ASCII '?'
	j           AssembleConversion
.func_end_Assemble_i2f:
	.size Assemble_i2f, .func_end_Assemble_i2f-Assemble_i2f

	.local  Assemble_ui2f
	.type Assemble_ui2f, @function

Assemble_ui2f:

	// *** Basic block 0

	.local AssembleConversion
	// Leaf procedure, no stack frame generated
	lla         a3, .str.172
	lla         a5, .str.173
	li          a4, 102		// 0x66 ASCII 'f'
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 65		// 0x41 ASCII 'A'
	j           AssembleConversion
.func_end_Assemble_ui2f:
	.size Assemble_ui2f, .func_end_Assemble_ui2f-Assemble_ui2f

	.local  Assemble_i2d
	.type Assemble_i2d, @function

Assemble_i2d:

	// *** Basic block 0

	.local AssembleConversion
	// Leaf procedure, no stack frame generated
	lla         a3, .str.174
	lla         a5, .str.175
	li          a4, 100		// 0x64 ASCII 'd'
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 64		// 0x40 ASCII '@'
	j           AssembleConversion
.func_end_Assemble_i2d:
	.size Assemble_i2d, .func_end_Assemble_i2d-Assemble_i2d

	.local  Assemble_ui2d
	.type Assemble_ui2d, @function

Assemble_ui2d:

	// *** Basic block 0

	.local AssembleConversion
	// Leaf procedure, no stack frame generated
	lla         a3, .str.176
	lla         a5, .str.177
	li          a4, 100		// 0x64 ASCII 'd'
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 66		// 0x42 ASCII 'B'
	j           AssembleConversion
.func_end_Assemble_ui2d:
	.size Assemble_ui2d, .func_end_Assemble_ui2d-Assemble_ui2d

	.local  Assemble_d2f
	.type Assemble_d2f, @function

Assemble_d2f:

	// *** Basic block 0

	.local AssembleConversion
	// Leaf procedure, no stack frame generated
	lla         a3, .str.178
	lla         a5, .str.179
	li          a4, 102		// 0x66 ASCII 'f'
	li          a2, 100		// 0x64 ASCII 'd'
	li          a1, 68		// 0x44 ASCII 'D'
	j           AssembleConversion
.func_end_Assemble_d2f:
	.size Assemble_d2f, .func_end_Assemble_d2f-Assemble_d2f

	.local  Assemble_f2d
	.type Assemble_f2d, @function

Assemble_f2d:

	// *** Basic block 0

	.local AssembleConversion
	// Leaf procedure, no stack frame generated
	lla         a3, .str.180
	lla         a5, .str.181
	li          a4, 100		// 0x64 ASCII 'd'
	li          a2, 102		// 0x66 ASCII 'f'
	li          a1, 67		// 0x43 ASCII 'C'
	j           AssembleConversion
.func_end_Assemble_f2d:
	.size Assemble_f2d, .func_end_Assemble_f2d-Assemble_f2d

	.local  Assemble_f2i
	.type Assemble_f2i, @function

Assemble_f2i:

	// *** Basic block 0

	.local AssembleConversion
	// Leaf procedure, no stack frame generated
	lla         a3, .str.182
	lla         a5, .str.183
	li          a4, 105		// 0x69 ASCII 'i'
	li          a2, 102		// 0x66 ASCII 'f'
	li          a1, 69		// 0x45 ASCII 'E'
	j           AssembleConversion
.func_end_Assemble_f2i:
	.size Assemble_f2i, .func_end_Assemble_f2i-Assemble_f2i

	.local  Assemble_d2i
	.type Assemble_d2i, @function

Assemble_d2i:

	// *** Basic block 0

	.local AssembleConversion
	// Leaf procedure, no stack frame generated
	lla         a3, .str.184
	lla         a5, .str.185
	li          a4, 105		// 0x69 ASCII 'i'
	li          a2, 100		// 0x64 ASCII 'd'
	li          a1, 70		// 0x46 ASCII 'F'
	j           AssembleConversion
.func_end_Assemble_d2i:
	.size Assemble_d2i, .func_end_Assemble_d2i-Assemble_d2i

	.local  Assemble_f2ui
	.type Assemble_f2ui, @function

Assemble_f2ui:

	// *** Basic block 0

	.local AssembleConversion
	// Leaf procedure, no stack frame generated
	lla         a3, .str.186
	lla         a5, .str.187
	li          a4, 105		// 0x69 ASCII 'i'
	li          a2, 102		// 0x66 ASCII 'f'
	li          a1, 71		// 0x47 ASCII 'G'
	j           AssembleConversion
.func_end_Assemble_f2ui:
	.size Assemble_f2ui, .func_end_Assemble_f2ui-Assemble_f2ui

	.local  Assemble_d2ui
	.type Assemble_d2ui, @function

Assemble_d2ui:

	// *** Basic block 0

	.local AssembleConversion
	// Leaf procedure, no stack frame generated
	lla         a3, .str.188
	lla         a5, .str.189
	li          a4, 105		// 0x69 ASCII 'i'
	li          a2, 100		// 0x64 ASCII 'd'
	li          a1, 72		// 0x48 ASCII 'H'
	j           AssembleConversion
.func_end_Assemble_d2ui:
	.size Assemble_d2ui, .func_end_Assemble_d2ui-Assemble_d2ui

	.local  Assemble_decsp
	.type Assemble_decsp, @function

Assemble_decsp:

	// *** Basic block 0

	.global LexMatch
	.global AssemblerEvaluateExpression
	.global AssemblerEmitWord
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
	li          a1, 97		// 0x61 ASCII 'a'
	call        LexMatch

	// *** Basic block 1

	beqz        a0, .Assemble_decsp_label_44

	// *** Basic block 2

	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 3

	mv          s2, a0
	lw          a1, 744(s1)
	li          t0, 16777215		// 0xffffff
	and         t0, s2, t0
	sext.w      t0, t0
	li          t1, 805306368		// 0x30000000
	or          a2, t1, t0
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 4

	j           .Assemble_decsp_label_52

	// *** Basic block 5

.Assemble_decsp_label_44:
	lla         a1, .str.190
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssemblerError

	// *** Basic block 6

.Assemble_decsp_label_52:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_decsp:
	.size Assemble_decsp, .func_end_Assemble_decsp-Assemble_decsp

	.local  Assemble_incsp
	.type Assemble_incsp, @function

Assemble_incsp:

	// *** Basic block 0

	.global LexMatch
	.global AssemblerEvaluateExpression
	.global AssemblerEmitWord
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
	li          a1, 97		// 0x61 ASCII 'a'
	call        LexMatch

	// *** Basic block 1

	beqz        a0, .Assemble_incsp_label_44

	// *** Basic block 2

	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 3

	mv          s2, a0
	lw          a1, 744(s1)
	li          t0, 16777215		// 0xffffff
	and         t0, s2, t0
	sext.w      t0, t0
	li          t1, 822083584		// 0x31000000
	or          a2, t1, t0
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 4

	j           .Assemble_incsp_label_52

	// *** Basic block 5

.Assemble_incsp_label_44:
	lla         a1, .str.191
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssemblerError

	// *** Basic block 6

.Assemble_incsp_label_52:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_incsp:
	.size Assemble_incsp, .func_end_Assemble_incsp-Assemble_incsp

	.local  AssembleLoadStore
	.type AssembleLoadStore, @function

AssembleLoadStore:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.global AssemblerEmitWord
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
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          t0, a2
	mv          s2, a1
	mv          a2, a3
	mv          a1, t0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s3, 18		// 0x12 ASCII \x12
	mv          a1, s3
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .AssembleLoadStore_label_66

	// *** Basic block 3

	lla         a1, .str.192
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 4

.AssembleLoadStore_label_63:
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

.AssembleLoadStore_label_66:
	addi        a0, s1, 176
	li          t0, 32		// 0x20 ASCII ' '
	mv          a1, t0
	call        LexMatch

	// *** Basic block 6

	not         t0, a0
	beqz        t0, .AssembleLoadStore_label_82

	// *** Basic block 7

	lla         a1, .str.193
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 8

	j           .AssembleLoadStore_label_63

	// *** Basic block 9

.AssembleLoadStore_label_82:
	addi        s4, s0, -32
	lla         a2, .str.194
	li          t0, 105		// 0x69 ASCII 'i'
	mv          a1, t0
	mv          a0, s1
	call        Register

	// *** Basic block 10

	sw          a0, 4(s4)
	addi        a0, s1, 176
	mv          a1, s3
	call        LexMatch

	// *** Basic block 11

	not         t0, a0
	beqz        t0, .AssembleLoadStore_label_111

	// *** Basic block 12

	lla         a1, .str.195
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 13

	j           .AssembleLoadStore_label_63

	// *** Basic block 14

.AssembleLoadStore_label_111:
	addi        a0, s1, 176
	li          t0, 97		// 0x61 ASCII 'a'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 15

	not         t0, a0
	beqz        t0, .AssembleLoadStore_label_127

	// *** Basic block 16

	lla         a1, .str.196
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 17

	j           .AssembleLoadStore_label_63

	// *** Basic block 18

.AssembleLoadStore_label_127:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 19

	mv          s3, a0
	addi        a0, s1, 176
	li          t0, 48		// 0x30 ASCII '0'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 20

	not         t0, a0
	beqz        t0, .AssembleLoadStore_label_148

	// *** Basic block 21

	lla         a1, .str.197
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 22

	j           .AssembleLoadStore_label_63

	// *** Basic block 23

.AssembleLoadStore_label_148:
	lw          a1, 744(s1)
	slli        t0, s2, 24
	li          t1, 2147483648		// 0x80000000
	or          t0, t1, t0
	lw          t1, -32(s0)
	slli        t1, t1, 16
	or          t0, t0, t1
	lw          t1, 4(s4)
	slli        t1, t1, 8
	or          a2, t0, t1
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 24

	lw          a1, 744(s1)
	sext.w      a2, s3
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 25

	j           .AssembleLoadStore_label_63
.func_end_AssembleLoadStore:
	.size AssembleLoadStore, .func_end_AssembleLoadStore-AssembleLoadStore

	.local  Assemble_ldb
	.type Assemble_ldb, @function

Assemble_ldb:

	// *** Basic block 0

	.local AssembleLoadStore
	// Leaf procedure, no stack frame generated
	lla         a3, .str.198
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 2		// 0x2 ASCII \x2
	j           AssembleLoadStore
.func_end_Assemble_ldb:
	.size Assemble_ldb, .func_end_Assemble_ldb-Assemble_ldb

	.local  Assemble_ldh
	.type Assemble_ldh, @function

Assemble_ldh:

	// *** Basic block 0

	.local AssembleLoadStore
	// Leaf procedure, no stack frame generated
	lla         a3, .str.199
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 1		// 0x1 ASCII \x1
	j           AssembleLoadStore
.func_end_Assemble_ldh:
	.size Assemble_ldh, .func_end_Assemble_ldh-Assemble_ldh

	.local  Assemble_ldw
	.type Assemble_ldw, @function

Assemble_ldw:

	// *** Basic block 0

	.local AssembleLoadStore
	// Leaf procedure, no stack frame generated
	lla         a3, .str.200
	li          a2, 105		// 0x69 ASCII 'i'
	mv          a1, x0
	j           AssembleLoadStore
.func_end_Assemble_ldw:
	.size Assemble_ldw, .func_end_Assemble_ldw-Assemble_ldw

	.local  Assemble_ldub
	.type Assemble_ldub, @function

Assemble_ldub:

	// *** Basic block 0

	.local AssembleLoadStore
	// Leaf procedure, no stack frame generated
	lla         a3, .str.201
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 4		// 0x4 ASCII \x4
	j           AssembleLoadStore
.func_end_Assemble_ldub:
	.size Assemble_ldub, .func_end_Assemble_ldub-Assemble_ldub

	.local  Assemble_lduh
	.type Assemble_lduh, @function

Assemble_lduh:

	// *** Basic block 0

	.local AssembleLoadStore
	// Leaf procedure, no stack frame generated
	lla         a3, .str.202
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 5		// 0x5 ASCII \x5
	j           AssembleLoadStore
.func_end_Assemble_lduh:
	.size Assemble_lduh, .func_end_Assemble_lduh-Assemble_lduh

	.local  Assemble_lduw
	.type Assemble_lduw, @function

Assemble_lduw:

	// *** Basic block 0

	.local AssembleLoadStore
	// Leaf procedure, no stack frame generated
	lla         a3, .str.203
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 3		// 0x3 ASCII \x3
	j           AssembleLoadStore
.func_end_Assemble_lduw:
	.size Assemble_lduw, .func_end_Assemble_lduw-Assemble_lduw

	.local  Assemble_ldx
	.type Assemble_ldx, @function

Assemble_ldx:

	// *** Basic block 0

	.local AssembleLoadStore
	// Leaf procedure, no stack frame generated
	lla         a3, .str.204
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 6		// 0x6 ASCII \x6
	j           AssembleLoadStore
.func_end_Assemble_ldx:
	.size Assemble_ldx, .func_end_Assemble_ldx-Assemble_ldx

	.local  Assemble_ldf
	.type Assemble_ldf, @function

Assemble_ldf:

	// *** Basic block 0

	.local AssembleLoadStore
	// Leaf procedure, no stack frame generated
	lla         a3, .str.205
	li          a2, 102		// 0x66 ASCII 'f'
	li          a1, 7		// 0x7 ASCII \x7
	j           AssembleLoadStore
.func_end_Assemble_ldf:
	.size Assemble_ldf, .func_end_Assemble_ldf-Assemble_ldf

	.local  Assemble_ldd
	.type Assemble_ldd, @function

Assemble_ldd:

	// *** Basic block 0

	.local AssembleLoadStore
	// Leaf procedure, no stack frame generated
	lla         a3, .str.206
	li          a2, 100		// 0x64 ASCII 'd'
	li          a1, 8		// 0x8 ASCII \x8
	j           AssembleLoadStore
.func_end_Assemble_ldd:
	.size Assemble_ldd, .func_end_Assemble_ldd-Assemble_ldd

	.local  Assemble_stb
	.type Assemble_stb, @function

Assemble_stb:

	// *** Basic block 0

	.local AssembleLoadStore
	// Leaf procedure, no stack frame generated
	lla         a3, .str.207
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 14		// 0xe ASCII \xe
	j           AssembleLoadStore
.func_end_Assemble_stb:
	.size Assemble_stb, .func_end_Assemble_stb-Assemble_stb

	.local  Assemble_stw
	.type Assemble_stw, @function

Assemble_stw:

	// *** Basic block 0

	.local AssembleLoadStore
	// Leaf procedure, no stack frame generated
	lla         a3, .str.208
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 9		// 0x9 ASCII \x9
	j           AssembleLoadStore
.func_end_Assemble_stw:
	.size Assemble_stw, .func_end_Assemble_stw-Assemble_stw

	.local  Assemble_sth
	.type Assemble_sth, @function

Assemble_sth:

	// *** Basic block 0

	.local AssembleLoadStore
	// Leaf procedure, no stack frame generated
	lla         a3, .str.209
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 10		// 0xa ASCII \xa
	j           AssembleLoadStore
.func_end_Assemble_sth:
	.size Assemble_sth, .func_end_Assemble_sth-Assemble_sth

	.local  Assemble_stx
	.type Assemble_stx, @function

Assemble_stx:

	// *** Basic block 0

	.local AssembleLoadStore
	// Leaf procedure, no stack frame generated
	lla         a3, .str.210
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 11		// 0xb ASCII \xb
	j           AssembleLoadStore
.func_end_Assemble_stx:
	.size Assemble_stx, .func_end_Assemble_stx-Assemble_stx

	.local  Assemble_stf
	.type Assemble_stf, @function

Assemble_stf:

	// *** Basic block 0

	.local AssembleLoadStore
	// Leaf procedure, no stack frame generated
	lla         a3, .str.211
	li          a2, 102		// 0x66 ASCII 'f'
	li          a1, 12		// 0xc ASCII \xc
	j           AssembleLoadStore
.func_end_Assemble_stf:
	.size Assemble_stf, .func_end_Assemble_stf-Assemble_stf

	.local  Assemble_std
	.type Assemble_std, @function

Assemble_std:

	// *** Basic block 0

	.local AssembleLoadStore
	// Leaf procedure, no stack frame generated
	lla         a3, .str.212
	li          a2, 100		// 0x64 ASCII 'd'
	li          a1, 13		// 0xd ASCII \xd
	j           AssembleLoadStore
.func_end_Assemble_std:
	.size Assemble_std, .func_end_Assemble_std-Assemble_std

	.local  AssembleMoveConstant
	.type AssembleMoveConstant, @function

AssembleMoveConstant:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global LexLookingAt
	.global StringInit
	.global AssemblerExtractSymbolSuffix
	.global AssemblerFindSymbol
	.global NewAssemblerSymbol
	.global AssemblerInsertSymbol
	.global LexNextToken
	.global StringEqual
	.global NewAssemblerRelocation
	.global AssemblerCurrentAddress
	.global AssemblerAddRelocation
	.global AssemblerEmitWord
	.global AssemblerEmitLong
	.global StringDestruct
	.global AssemblerEvaluateExpression
	.global AssemblerGetDoubleConst
	.global printf
	.global abort
	addi sp, sp, -176
	// Saved return address (offset 168) and frame pointer (offset 160)
	sd ra, 168(sp)
	sd s0, 160(sp)
	addi s0, sp, 176
	// Local vars at offset -112(s0)
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
	mv          a2, a3
	mv          a1, s2
	call        Register

	// *** Basic block 1

	mv          s4, a0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .AssembleMoveConstant_label_90

	// *** Basic block 3

	lla         a1, .str.213
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 4

.AssembleMoveConstant_label_87:
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

	// *** Basic block 5

.AssembleMoveConstant_label_90:
	addi        a0, s1, 176
	li          t0, 97		// 0x61 ASCII 'a'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 6

	not         t0, a0
	beqz        t0, .AssembleMoveConstant_label_269

	// *** Basic block 7

	li          s5, 1		// 0x1 ASCII \x1
	beq         s3, s5, .AssembleMoveConstant_label_111

	// *** Basic block 8

	lla         a1, .str.214
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 9

	j           .AssembleMoveConstant_label_87

	// *** Basic block 10

.AssembleMoveConstant_label_111:
	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 11

	not         t0, a0
	beqz        t0, .AssembleMoveConstant_label_127

	// *** Basic block 12

	lla         a1, .str.215
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 13

	j           .AssembleMoveConstant_label_87

	// *** Basic block 14

.AssembleMoveConstant_label_127:
	addi        a0, s0, -112
	mv          a1, x0
	call        StringInit

	// *** Basic block 15

	addi        a0, s0, -72
	mv          a1, x0
	call        StringInit

	// *** Basic block 16

	addi        t0, s1, 176
	addi        a0, t0, 80
	addi        a1, s0, -112
	addi        a2, s0, -72
	call        AssemblerExtractSymbolSuffix

	// *** Basic block 17

	addi        t0, s0, -112
	ld          s6, 16(t0)
	mv          a1, s6
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 18

	mv          s7, a0
	bne         s7, x0, .AssembleMoveConstant_label_184

	// *** Basic block 19

	lw          a1, 744(s1)
	mv          a4, x0
	mv          a3, s5
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a0, s6
	call        NewAssemblerSymbol

	// *** Basic block 20

	mv          s7, a0
	mv          a1, s7
	mv          a0, s1
	call        AssemblerInsertSymbol

	// *** Basic block 21

.AssembleMoveConstant_label_184:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 22

	addi        a0, s0, -72
	lla         a1, .str.216
	call        StringEqual

	// *** Basic block 23

	beqz        a0, .AssembleMoveConstant_label_207

	// *** Basic block 24

	lb          t0, 760(s1)
	beqz        t0, .AssembleMoveConstant_label_202

	// *** Basic block 25

	li          s5, 17		// 0x11 ASCII \x11
	j           .AssembleMoveConstant_label_204

	// *** Basic block 26

.AssembleMoveConstant_label_202:
	li          s5, 19		// 0x13 ASCII \x13

	// *** Basic block 27

.AssembleMoveConstant_label_204:
	j           .AssembleMoveConstant_label_218

	// *** Basic block 28

.AssembleMoveConstant_label_207:
	lb          t0, 760(s1)
	beqz        t0, .AssembleMoveConstant_label_215

	// *** Basic block 29

	li          s5, 13		// 0xd ASCII \xd
	j           .AssembleMoveConstant_label_217

	// *** Basic block 30

.AssembleMoveConstant_label_215:
	li          s5, 2		// 0x2 ASCII \x2

	// *** Basic block 31

.AssembleMoveConstant_label_217:

	// *** Basic block 32

.AssembleMoveConstant_label_218:
	lw          s6, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 33

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s6
	mv          a1, s5
	mv          a0, s7
	call        NewAssemblerRelocation

	// *** Basic block 34

	mv          s5, a0
	mv          a1, s5
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 35

	lw          a1, 744(s1)
	slli        t0, s3, 24
	li          t1, 3221225472		// 0xc0000000
	or          t0, t1, t0
	slli        t1, s4, 16
	or          a2, t0, t1
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 36

	lw          a1, 744(s1)
	mv          a2, x0
	mv          a0, s1
	call        AssemblerEmitLong

	// *** Basic block 37

	addi        a0, s0, -112
	call        StringDestruct

	// *** Basic block 38

	addi        a0, s0, -72
	call        StringDestruct

	// *** Basic block 39

	j           .AssembleMoveConstant_label_87

	// *** Basic block 40

.AssembleMoveConstant_label_269:
	li          t0, 100		// 0x64 ASCII 'd'
	blt         s2, t0, .AssembleMoveConstant_label_404

	// *** Basic block 41

	li          t0, 105		// 0x69 ASCII 'i'
	blt         t0, s2, .AssembleMoveConstant_label_404

	// *** Basic block 42

	addi        t0, s2, -100
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 43

	j           .AssembleMoveConstant_label_375

	// *** Basic block 44

	j           .AssembleMoveConstant_label_404

	// *** Basic block 45

	j           .AssembleMoveConstant_label_345

	// *** Basic block 46

	j           .AssembleMoveConstant_label_404

	// *** Basic block 47

	j           .AssembleMoveConstant_label_404

	// *** Basic block 48

	j           .AssembleMoveConstant_label_291

	// *** Basic block 49

.AssembleMoveConstant_label_291:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 50

	mv          s2, a0
	li          t0, 1		// 0x1 ASCII \x1
	bne         s3, t0, .AssembleMoveConstant_label_322

	// *** Basic block 51

	lw          a1, 744(s1)
	slli        t0, s3, 24
	li          t1, 3221225472		// 0xc0000000
	or          t0, t1, t0
	slli        t1, s4, 16
	or          a2, t0, t1
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 52

	lw          a1, 744(s1)
	mv          a2, s2
	mv          a0, s1
	call        AssemblerEmitLong

	// *** Basic block 53

	j           .AssembleMoveConstant_label_343

	// *** Basic block 54

.AssembleMoveConstant_label_322:
	lw          a1, 744(s1)
	slli        t0, s3, 24
	li          t1, 2147483648		// 0x80000000
	or          t0, t1, t0
	slli        t1, s4, 16
	or          a2, t0, t1
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 55

	lw          a1, 744(s1)
	sext.w      a2, s2
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 56

.AssembleMoveConstant_label_343:
	j           .AssembleMoveConstant_label_419

	// *** Basic block 57

.AssembleMoveConstant_label_345:
	lw          a1, 744(s1)
	slli        t0, s3, 24
	li          t1, 2147483648		// 0x80000000
	or          t0, t1, t0
	slli        t1, s4, 16
	or          a2, t0, t1
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 58

	mv          a0, s1
	call        AssemblerGetDoubleConst

	// *** Basic block 59

	fcvt.s.d    ft0, fa0
	fsw         ft0, -32(s0)
	addi        s2, s0, -32
	lw          a1, 744(s1)
	lwu         a2, 0(s2)
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 60

	j           .AssembleMoveConstant_label_419

	// *** Basic block 61

.AssembleMoveConstant_label_375:
	mv          a0, s1
	call        AssemblerGetDoubleConst

	// *** Basic block 62

	fsd         fa0, -24(s0)
	addi        s2, s0, -24
	lw          a1, 744(s1)
	slli        t0, s3, 24
	li          t1, 3221225472		// 0xc0000000
	or          t0, t1, t0
	slli        t1, s4, 16
	or          a2, t0, t1
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 63

	lw          a1, 744(s1)
	ld          a2, 0(s2)
	mv          a0, s1
	call        AssemblerEmitLong

	// *** Basic block 64

	j           .AssembleMoveConstant_label_419

	// *** Basic block 65

.AssembleMoveConstant_label_404:
	lla         a0, .str.217
	lla         a1, .str.218
	lla         a3, .str.219
	li          t0, 818		// 0x332
	mv          a2, t0
	call        printf

	// *** Basic block 66

	call        abort

	// *** Basic block 67

.AssembleMoveConstant_label_419:
	j           .AssembleMoveConstant_label_87
.func_end_AssembleMoveConstant:
	.size AssembleMoveConstant, .func_end_AssembleMoveConstant-AssembleMoveConstant

	.local  Assemble_movc
	.type Assemble_movc, @function

Assemble_movc:

	// *** Basic block 0

	.local AssembleMoveConstant
	// Leaf procedure, no stack frame generated
	lla         a3, .str.220
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 15		// 0xf ASCII \xf
	j           AssembleMoveConstant
.func_end_Assemble_movc:
	.size Assemble_movc, .func_end_Assemble_movc-Assemble_movc

	.local  Assemble_movfc
	.type Assemble_movfc, @function

Assemble_movfc:

	// *** Basic block 0

	.local AssembleMoveConstant
	// Leaf procedure, no stack frame generated
	lla         a3, .str.221
	li          a2, 102		// 0x66 ASCII 'f'
	li          a1, 16		// 0x10 ASCII \x10
	j           AssembleMoveConstant
.func_end_Assemble_movfc:
	.size Assemble_movfc, .func_end_Assemble_movfc-Assemble_movfc

	.local  Assemble_movdc
	.type Assemble_movdc, @function

Assemble_movdc:

	// *** Basic block 0

	.local AssembleMoveConstant
	// Leaf procedure, no stack frame generated
	lla         a3, .str.222
	li          a2, 100		// 0x64 ASCII 'd'
	mv          a1, x0
	j           AssembleMoveConstant
.func_end_Assemble_movdc:
	.size Assemble_movdc, .func_end_Assemble_movdc-Assemble_movdc

	.local  Assemble_movxc
	.type Assemble_movxc, @function

Assemble_movxc:

	// *** Basic block 0

	.local AssembleMoveConstant
	// Leaf procedure, no stack frame generated
	lla         a3, .str.223
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 1		// 0x1 ASCII \x1
	j           AssembleMoveConstant
.func_end_Assemble_movxc:
	.size Assemble_movxc, .func_end_Assemble_movxc-Assemble_movxc

	.local  AssembleMove
	.type AssembleMove, @function

AssembleMove:

	// *** Basic block 0

	.local ParseRegisterPair
	.global AssemblerEmitWord
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
	mv          t0, a2
	mv          t1, a3
	mv          s2, a1
	addi        a3, s0, -32
	mv          a2, t1
	mv          a1, t0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .AssembleMove_label_52

	// *** Basic block 2

	lw          a1, 744(s1)
	slli        t0, s2, 24
	lw          t1, -32(s0)
	slli        t1, t1, 16
	or          t0, t0, t1
	addi        t1, s0, -32
	lw          t1, 4(t1)
	slli        t1, t1, 8
	or          a2, t0, t1
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 3

.AssembleMove_label_52:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AssembleMove:
	.size AssembleMove, .func_end_AssembleMove-AssembleMove

	.local  Assemble_mov
	.type Assemble_mov, @function

Assemble_mov:

	// *** Basic block 0

	.local AssembleMove
	// Leaf procedure, no stack frame generated
	lla         a3, .str.224
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 58		// 0x3a ASCII ':'
	j           AssembleMove
.func_end_Assemble_mov:
	.size Assemble_mov, .func_end_Assemble_mov-Assemble_mov

	.local  Assemble_movf
	.type Assemble_movf, @function

Assemble_movf:

	// *** Basic block 0

	.local AssembleMove
	// Leaf procedure, no stack frame generated
	lla         a3, .str.225
	li          a2, 102		// 0x66 ASCII 'f'
	li          a1, 59		// 0x3b ASCII ';'
	j           AssembleMove
.func_end_Assemble_movf:
	.size Assemble_movf, .func_end_Assemble_movf-Assemble_movf

	.local  Assemble_movd
	.type Assemble_movd, @function

Assemble_movd:

	// *** Basic block 0

	.local AssembleMove
	// Leaf procedure, no stack frame generated
	lla         a3, .str.226
	li          a2, 100		// 0x64 ASCII 'd'
	li          a1, 60		// 0x3c ASCII '<'
	j           AssembleMove
.func_end_Assemble_movd:
	.size Assemble_movd, .func_end_Assemble_movd-Assemble_movd

	.local  AssemblePushPPCODE_OP
	.type AssemblePushPPCODE_OP, @function

AssemblePushPPCODE_OP:

	// *** Basic block 0

	.local Register
	.global AssemblerEmitWord
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
	mv          t0, a2
	mv          s2, a1
	mv          a2, a3
	mv          a1, t0
	call        Register

	// *** Basic block 1

	mv          s3, a0
	lw          a1, 744(s1)
	slli        t0, s2, 24
	slli        t1, s3, 16
	or          a2, t0, t1
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssemblerEmitWord
.func_end_AssemblePushPPCODE_OP:
	.size AssemblePushPPCODE_OP, .func_end_AssemblePushPPCODE_OP-AssemblePushPPCODE_OP

	.local  Assemble_push
	.type Assemble_push, @function

Assemble_push:

	// *** Basic block 0

	.local AssemblePushPPCODE_OP
	// Leaf procedure, no stack frame generated
	lla         a3, .str.227
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 50		// 0x32 ASCII '2'
	j           AssemblePushPPCODE_OP
.func_end_Assemble_push:
	.size Assemble_push, .func_end_Assemble_push-Assemble_push

	.local  Assemble_pushf
	.type Assemble_pushf, @function

Assemble_pushf:

	// *** Basic block 0

	.local AssemblePushPPCODE_OP
	// Leaf procedure, no stack frame generated
	lla         a3, .str.228
	li          a2, 102		// 0x66 ASCII 'f'
	li          a1, 51		// 0x33 ASCII '3'
	j           AssemblePushPPCODE_OP
.func_end_Assemble_pushf:
	.size Assemble_pushf, .func_end_Assemble_pushf-Assemble_pushf

	.local  Assemble_pushd
	.type Assemble_pushd, @function

Assemble_pushd:

	// *** Basic block 0

	.local AssemblePushPPCODE_OP
	// Leaf procedure, no stack frame generated
	lla         a3, .str.229
	li          a2, 100		// 0x64 ASCII 'd'
	li          a1, 52		// 0x34 ASCII '4'
	j           AssemblePushPPCODE_OP
.func_end_Assemble_pushd:
	.size Assemble_pushd, .func_end_Assemble_pushd-Assemble_pushd

	.local  Assemble_pushx
	.type Assemble_pushx, @function

Assemble_pushx:

	// *** Basic block 0

	.local AssemblePushPPCODE_OP
	// Leaf procedure, no stack frame generated
	lla         a3, .str.230
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 53		// 0x35 ASCII '5'
	j           AssemblePushPPCODE_OP
.func_end_Assemble_pushx:
	.size Assemble_pushx, .func_end_Assemble_pushx-Assemble_pushx

	.local  Assemble_pop
	.type Assemble_pop, @function

Assemble_pop:

	// *** Basic block 0

	.local AssemblePushPPCODE_OP
	// Leaf procedure, no stack frame generated
	lla         a3, .str.231
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 54		// 0x36 ASCII '6'
	j           AssemblePushPPCODE_OP
.func_end_Assemble_pop:
	.size Assemble_pop, .func_end_Assemble_pop-Assemble_pop

	.local  Assemble_popf
	.type Assemble_popf, @function

Assemble_popf:

	// *** Basic block 0

	.local AssemblePushPPCODE_OP
	// Leaf procedure, no stack frame generated
	lla         a3, .str.232
	li          a2, 102		// 0x66 ASCII 'f'
	li          a1, 55		// 0x37 ASCII '7'
	j           AssemblePushPPCODE_OP
.func_end_Assemble_popf:
	.size Assemble_popf, .func_end_Assemble_popf-Assemble_popf

	.local  Assemble_popd
	.type Assemble_popd, @function

Assemble_popd:

	// *** Basic block 0

	.local AssemblePushPPCODE_OP
	// Leaf procedure, no stack frame generated
	lla         a3, .str.233
	li          a2, 100		// 0x64 ASCII 'd'
	li          a1, 56		// 0x38 ASCII '8'
	j           AssemblePushPPCODE_OP
.func_end_Assemble_popd:
	.size Assemble_popd, .func_end_Assemble_popd-Assemble_popd

	.local  Assemble_popx
	.type Assemble_popx, @function

Assemble_popx:

	// *** Basic block 0

	.local AssemblePushPPCODE_OP
	// Leaf procedure, no stack frame generated
	lla         a3, .str.234
	li          a2, 105		// 0x69 ASCII 'i'
	li          a1, 57		// 0x39 ASCII '9'
	j           AssemblePushPPCODE_OP
.func_end_Assemble_popx:
	.size Assemble_popx, .func_end_Assemble_popx-Assemble_popx

	.local  Assemble_ret
	.type Assemble_ret, @function

Assemble_ret:

	// *** Basic block 0

	.global AssemblerEmitWord
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	lw          a1, 744(t0)
	li          a2, 1023410176		// 0x3d000000
	j           AssemblerEmitWord
.func_end_Assemble_ret:
	.size Assemble_ret, .func_end_Assemble_ret-Assemble_ret

	.local  AssembleConditionalBranch
	.type AssembleConditionalBranch, @function

AssembleConditionalBranch:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.global AssemblerCurrentAddress
	.global AssemblerEmitWord
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
	lla         a2, .str.235
	li          a1, 105		// 0x69 ASCII 'i'
	call        Register

	// *** Basic block 1

	mv          s3, a0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .AssembleConditionalBranch_label_55

	// *** Basic block 3

	lla         a1, .str.236
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

	// *** Basic block 4

.AssembleConditionalBranch_label_52:
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

.AssembleConditionalBranch_label_55:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 6

	mv          s4, a0
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 7

	addi        t0, a0, 8
	sub         s5, s4, t0
	lw          a1, 744(s1)
	slli        t0, s2, 24
	li          t1, 2147483648		// 0x80000000
	or          t0, t1, t0
	slli        t1, s3, 16
	or          a2, t0, t1
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 8

	lw          a1, 744(s1)
	sext.w      a2, s5
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 9

	j           .AssembleConditionalBranch_label_52
.func_end_AssembleConditionalBranch:
	.size AssembleConditionalBranch, .func_end_AssembleConditionalBranch-AssembleConditionalBranch

	.local  Assemble_bz
	.type Assemble_bz, @function

Assemble_bz:

	// *** Basic block 0

	.local AssembleConditionalBranch
	// Leaf procedure, no stack frame generated
	li          a1, 17		// 0x11 ASCII \x11
	j           AssembleConditionalBranch
.func_end_Assemble_bz:
	.size Assemble_bz, .func_end_Assemble_bz-Assemble_bz

	.local  Assemble_bnz
	.type Assemble_bnz, @function

Assemble_bnz:

	// *** Basic block 0

	.local AssembleConditionalBranch
	// Leaf procedure, no stack frame generated
	li          a1, 18		// 0x12 ASCII \x12
	j           AssembleConditionalBranch
.func_end_Assemble_bnz:
	.size Assemble_bnz, .func_end_Assemble_bnz-Assemble_bnz

	.local  Assemble_bra
	.type Assemble_bra, @function

Assemble_bra:

	// *** Basic block 0

	.global AssemblerEvaluateExpression
	.global AssemblerCurrentAddress
	.global AssemblerEmitWord
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
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 2

	addi        t0, a0, 8
	sub         s3, s2, t0
	lw          a1, 744(s1)
	li          t0, 2466250752		// 0x93000000
	mv          a2, t0
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 3

	lw          a1, 744(s1)
	sext.w      a2, s3
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssemblerEmitWord
.func_end_Assemble_bra:
	.size Assemble_bra, .func_end_Assemble_bra-Assemble_bra

	.local  Assemble_addc
	.type Assemble_addc, @function

Assemble_addc:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.global AssemblerEmitWord
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
	lla         a2, .str.237
	addi        a3, s0, -32
	li          a1, 105		// 0x69 ASCII 'i'
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_addc_label_101

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .Assemble_addc_label_55

	// *** Basic block 4

	lla         a1, .str.238
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.Assemble_addc_label_55:
	addi        a0, s1, 176
	li          t0, 97		// 0x61 ASCII 'a'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 6

	not         t0, a0
	beqz        t0, .Assemble_addc_label_70

	// *** Basic block 7

	lla         a1, .str.239
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 8

.Assemble_addc_label_70:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 9

	mv          s2, a0
	lw          a1, 744(s1)
	lw          t0, -32(s0)
	slli        t0, t0, 16
	li          t1, 2483027968		// 0x94000000
	or          t0, t1, t0
	addi        t1, s0, -32
	lw          t1, 4(t1)
	slli        t1, t1, 8
	or          a2, t0, t1
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 10

	lw          a1, 744(s1)
	sext.w      a2, s2
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 11

.Assemble_addc_label_101:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_addc:
	.size Assemble_addc, .func_end_Assemble_addc-Assemble_addc

	.local  Assemble_cbra
	.type Assemble_cbra, @function

Assemble_cbra:

	// *** Basic block 0

	.local Register
	.global AssemblerEmitWord
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
	lla         a2, .str.240
	li          a1, 105		// 0x69 ASCII 'i'
	call        Register

	// *** Basic block 1

	mv          s2, a0
	lw          a1, 744(s1)
	slli        t0, s2, 16
	li          t1, 1040187392		// 0x3e000000
	or          a2, t1, t0
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssemblerEmitWord
.func_end_Assemble_cbra:
	.size Assemble_cbra, .func_end_Assemble_cbra-Assemble_cbra

	.local  Assemble_rcall
	.type Assemble_rcall, @function

Assemble_rcall:

	// *** Basic block 0

	.local Register
	.global AssemblerEmitWord
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
	lla         a2, .str.241
	li          a1, 105		// 0x69 ASCII 'i'
	call        Register

	// *** Basic block 1

	mv          s2, a0
	lw          a1, 744(s1)
	slli        t0, s2, 16
	li          t1, 1224736768		// 0x49000000
	or          a2, t1, t0
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssemblerEmitWord
.func_end_Assemble_rcall:
	.size Assemble_rcall, .func_end_Assemble_rcall-Assemble_rcall

	.local  Assemble_call
	.type Assemble_call, @function

Assemble_call:

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
	.global AssemblerEmitWord
	.global AssemblerEmitLong
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
	beqz        t0, .Assemble_call_label_47

	// *** Basic block 2

	lla         a1, .str.242
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

.Assemble_call_label_44:
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

.Assemble_call_label_47:
	addi        t0, s1, 176
	addi        t0, t0, 80
	ld          s2, 16(t0)
	mv          a1, s2
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 5

	mv          s3, a0
	bne         s3, x0, .Assemble_call_label_86

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

.Assemble_call_label_86:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 9

	lb          t0, 760(s1)
	beqz        t0, .Assemble_call_label_97

	// *** Basic block 10

	li          s2, 12		// 0xc ASCII \xc
	j           .Assemble_call_label_99

	// *** Basic block 11

.Assemble_call_label_97:
	li          s2, 1		// 0x1 ASCII \x1

	// *** Basic block 12

.Assemble_call_label_99:
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
	li          t0, 3271557120		// 0xc3000000
	mv          a2, t0
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 16

	lw          a1, 744(s1)
	mv          a2, x0
	mv          a0, s1
	call        AssemblerEmitLong

	// *** Basic block 17

	j           .Assemble_call_label_44
.func_end_Assemble_call:
	.size Assemble_call, .func_end_Assemble_call-Assemble_call

	.local  Assemble_jmp
	.type Assemble_jmp, @function

Assemble_jmp:

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
	.global AssemblerEmitWord
	.global AssemblerEmitLong
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
	li          s2, 3		// 0x3 ASCII \x3
	mv          a1, s2
	call        LexLookingAt

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .Assemble_jmp_label_46

	// *** Basic block 2

	lla         a1, .str.243
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

.Assemble_jmp_label_43:
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

.Assemble_jmp_label_46:
	addi        t0, s1, 176
	addi        t0, t0, 80
	ld          s3, 16(t0)
	mv          a1, s3
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 5

	mv          s4, a0
	bne         s4, x0, .Assemble_jmp_label_85

	// *** Basic block 6

	lw          a1, 744(s1)
	mv          a4, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, t0
	mv          a0, s3
	call        NewAssemblerSymbol

	// *** Basic block 7

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        AssemblerInsertSymbol

	// *** Basic block 8

.Assemble_jmp_label_85:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 9

	lw          s3, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 10

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s3
	mv          a1, s2
	mv          a0, s4
	call        NewAssemblerRelocation

	// *** Basic block 11

	mv          s2, a0
	mv          a1, s2
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 12

	lw          a1, 744(s1)
	li          t0, 3254779904		// 0xc2000000
	mv          a2, t0
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 13

	lw          a1, 744(s1)
	mv          a2, x0
	mv          a0, s1
	call        AssemblerEmitLong

	// *** Basic block 14

	j           .Assemble_jmp_label_43
.func_end_Assemble_jmp:
	.size Assemble_jmp, .func_end_Assemble_jmp-Assemble_jmp

	.local  Assemble_cjmp
	.type Assemble_cjmp, @function

Assemble_cjmp:

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
	.global AssemblerEmitWord
	.global AssemblerEmitLong
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
	li          s2, 3		// 0x3 ASCII \x3
	mv          a1, s2
	call        LexLookingAt

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .Assemble_cjmp_label_47

	// *** Basic block 2

	lla         a1, .str.244
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

.Assemble_cjmp_label_44:
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

.Assemble_cjmp_label_47:
	addi        t0, s1, 176
	addi        t0, t0, 80
	ld          s3, 16(t0)
	mv          a1, s3
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 5

	mv          s4, a0
	bne         s4, x0, .Assemble_cjmp_label_86

	// *** Basic block 6

	lw          a1, 744(s1)
	mv          a4, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a0, s3
	call        NewAssemblerSymbol

	// *** Basic block 7

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        AssemblerInsertSymbol

	// *** Basic block 8

.Assemble_cjmp_label_86:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 9

	lw          s3, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 10

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s3
	mv          a1, s2
	mv          a0, s4
	call        NewAssemblerRelocation

	// *** Basic block 11

	mv          s2, a0
	mv          a1, s2
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 12

	lw          a1, 744(s1)
	li          t0, 3288334336		// 0xc4000000
	mv          a2, t0
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 13

	lw          a1, 744(s1)
	mv          a2, x0
	mv          a0, s1
	call        AssemblerEmitLong

	// *** Basic block 14

	j           .Assemble_cjmp_label_44
.func_end_Assemble_cjmp:
	.size Assemble_cjmp, .func_end_Assemble_cjmp-Assemble_cjmp

	.local  Assemble_adr
	.type Assemble_adr, @function

Assemble_adr:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global LexLookingAt
	.global StringInit
	.global AssemblerExtractSymbolSuffix
	.global AssemblerFindSymbol
	.global NewAssemblerSymbol
	.global AssemblerInsertSymbol
	.global LexNextToken
	.global StringEqual
	.global NewAssemblerRelocation
	.global AssemblerCurrentAddress
	.global AssemblerAddRelocation
	.global AssemblerEmitWord
	.global AssemblerEmitLong
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
	lla         a2, .str.245
	li          a1, 105		// 0x69 ASCII 'i'
	call        Register

	// *** Basic block 1

	mv          s2, a0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .Assemble_adr_label_70

	// *** Basic block 3

	lla         a1, .str.246
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 4

.Assemble_adr_label_67:
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

.Assemble_adr_label_70:
	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 6

	not         t0, a0
	beqz        t0, .Assemble_adr_label_86

	// *** Basic block 7

	lla         a1, .str.247
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 8

	j           .Assemble_adr_label_67

	// *** Basic block 9

.Assemble_adr_label_86:
	addi        a0, s0, -96
	mv          a1, x0
	call        StringInit

	// *** Basic block 10

	addi        a0, s0, -56
	mv          a1, x0
	call        StringInit

	// *** Basic block 11

	addi        t0, s1, 176
	addi        a0, t0, 80
	addi        a1, s0, -96
	addi        a2, s0, -56
	call        AssemblerExtractSymbolSuffix

	// *** Basic block 12

	addi        t0, s0, -96
	ld          s3, 16(t0)
	mv          a1, s3
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 13

	mv          s4, a0
	bne         s4, x0, .Assemble_adr_label_144

	// *** Basic block 14

	lw          a1, 744(s1)
	mv          a4, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, t0
	mv          a0, s3
	call        NewAssemblerSymbol

	// *** Basic block 15

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        AssemblerInsertSymbol

	// *** Basic block 16

.Assemble_adr_label_144:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 17

	addi        a0, s0, -56
	lla         a1, .str.248
	call        StringEqual

	// *** Basic block 18

	beqz        a0, .Assemble_adr_label_158

	// *** Basic block 19

	li          s3, 18		// 0x12 ASCII \x12
	j           .Assemble_adr_label_169

	// *** Basic block 20

.Assemble_adr_label_158:
	lb          t0, 760(s1)
	beqz        t0, .Assemble_adr_label_166

	// *** Basic block 21

	li          s3, 13		// 0xd ASCII \xd
	j           .Assemble_adr_label_168

	// *** Basic block 22

.Assemble_adr_label_166:
	li          s3, 2		// 0x2 ASCII \x2

	// *** Basic block 23

.Assemble_adr_label_168:

	// *** Basic block 24

.Assemble_adr_label_169:
	lw          s5, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 25

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s5
	mv          a1, s3
	mv          a0, s4
	call        NewAssemblerRelocation

	// *** Basic block 26

	mv          s3, a0
	mv          a1, s3
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 27

	lw          a1, 744(s1)
	slli        t0, s2, 16
	li          t1, 3305111552		// 0xc5000000
	or          a2, t1, t0
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 28

	lw          a1, 744(s1)
	mv          a2, x0
	mv          a0, s1
	call        AssemblerEmitLong

	// *** Basic block 29

	addi        a0, s0, -96
	call        StringDestruct

	// *** Basic block 30

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 31

	j           .Assemble_adr_label_67
.func_end_Assemble_adr:
	.size Assemble_adr, .func_end_Assemble_adr-Assemble_adr

	.local  Assemble_adrs
	.type Assemble_adrs, @function

Assemble_adrs:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global LexLookingAt
	.global AssemblerFindSymbol
	.global NewAssemblerSymbol
	.global AssemblerInsertSymbol
	.global LexNextToken
	.global NewAssemblerRelocation
	.global AssemblerCurrentAddress
	.global AssemblerAddRelocation
	.global AssemblerEmitWord
	.global AssemblerEmitLong
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
	lla         a2, .str.249
	li          a1, 105		// 0x69 ASCII 'i'
	call        Register

	// *** Basic block 1

	mv          s2, a0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .Assemble_adrs_label_63

	// *** Basic block 3

	lla         a1, .str.250
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

	// *** Basic block 4

.Assemble_adrs_label_60:
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

.Assemble_adrs_label_63:
	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 6

	not         t0, a0
	beqz        t0, .Assemble_adrs_label_79

	// *** Basic block 7

	lla         a1, .str.251
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 8

	j           .Assemble_adrs_label_60

	// *** Basic block 9

.Assemble_adrs_label_79:
	addi        t0, s1, 176
	addi        t0, t0, 80
	ld          s3, 16(t0)
	mv          a1, s3
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 10

	mv          s4, a0
	bne         s4, x0, .Assemble_adrs_label_117

	// *** Basic block 11

	lw          a1, 744(s1)
	mv          a4, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, t0
	mv          a0, s3
	call        NewAssemblerSymbol

	// *** Basic block 12

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        AssemblerInsertSymbol

	// *** Basic block 13

.Assemble_adrs_label_117:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 14

	lw          s3, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 15

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s3
	li          t0, 16		// 0x10 ASCII \x10
	mv          a1, t0
	mv          a0, s4
	call        NewAssemblerRelocation

	// *** Basic block 16

	mv          s3, a0
	mv          a1, s3
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 17

	lw          a1, 744(s1)
	slli        t0, s2, 16
	li          t1, 3305111552		// 0xc5000000
	or          a2, t1, t0
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 18

	lw          a1, 744(s1)
	mv          a2, x0
	mv          a0, s1
	call        AssemblerEmitLong

	// *** Basic block 19

	j           .Assemble_adrs_label_60
.func_end_Assemble_adrs:
	.size Assemble_adrs, .func_end_Assemble_adrs-Assemble_adrs

	.local  Assemble_adrtls
	.type Assemble_adrtls, @function

Assemble_adrtls:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global LexLookingAt
	.global StringInit
	.global AssemblerExtractSymbolSuffix
	.global AssemblerFindSymbol
	.global NewAssemblerSymbol
	.global AssemblerInsertSymbol
	.global LexNextToken
	.global StringEqual
	.global NewAssemblerRelocation
	.global AssemblerCurrentAddress
	.global AssemblerAddRelocation
	.global AssemblerEmitWord
	.global AssemblerEmitLong
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
	lla         a2, .str.252
	li          a1, 105		// 0x69 ASCII 'i'
	call        Register

	// *** Basic block 1

	mv          s2, a0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .Assemble_adrtls_label_71

	// *** Basic block 3

	lla         a1, .str.253
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 4

.Assemble_adrtls_label_68:
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

.Assemble_adrtls_label_71:
	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 6

	not         t0, a0
	beqz        t0, .Assemble_adrtls_label_87

	// *** Basic block 7

	lla         a1, .str.254
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 8

	j           .Assemble_adrtls_label_68

	// *** Basic block 9

.Assemble_adrtls_label_87:
	addi        a0, s0, -96
	mv          a1, x0
	call        StringInit

	// *** Basic block 10

	addi        a0, s0, -56
	mv          a1, x0
	call        StringInit

	// *** Basic block 11

	addi        t0, s1, 176
	addi        a0, t0, 80
	addi        a1, s0, -96
	addi        a2, s0, -56
	call        AssemblerExtractSymbolSuffix

	// *** Basic block 12

	addi        t0, s0, -96
	ld          s3, 16(t0)
	mv          a1, s3
	mv          a0, s1
	call        AssemblerFindSymbol

	// *** Basic block 13

	mv          s4, a0
	bne         s4, x0, .Assemble_adrtls_label_145

	// *** Basic block 14

	lw          a1, 744(s1)
	mv          a4, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, t0
	mv          a0, s3
	call        NewAssemblerSymbol

	// *** Basic block 15

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        AssemblerInsertSymbol

	// *** Basic block 16

.Assemble_adrtls_label_145:
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 17

	addi        a0, s0, -56
	lla         a1, .str.255
	call        StringEqual

	// *** Basic block 18

	beqz        a0, .Assemble_adrtls_label_159

	// *** Basic block 19

	li          s3, 17		// 0x11 ASCII \x11
	j           .Assemble_adrtls_label_170

	// *** Basic block 20

.Assemble_adrtls_label_159:
	lb          t0, 760(s1)
	beqz        t0, .Assemble_adrtls_label_167

	// *** Basic block 21

	li          s3, 13		// 0xd ASCII \xd
	j           .Assemble_adrtls_label_169

	// *** Basic block 22

.Assemble_adrtls_label_167:
	li          s3, 2		// 0x2 ASCII \x2

	// *** Basic block 23

.Assemble_adrtls_label_169:

	// *** Basic block 24

.Assemble_adrtls_label_170:
	lw          s5, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 25

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s5
	mv          a1, s3
	mv          a0, s4
	call        NewAssemblerRelocation

	// *** Basic block 26

	mv          s3, a0
	mv          a1, s3
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 27

	lw          a1, 744(s1)
	slli        t0, s2, 16
	li          t1, 3305111552		// 0xc5000000
	or          a2, t1, t0
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 28

	lw          a1, 744(s1)
	mv          a2, x0
	mv          a0, s1
	call        AssemblerEmitLong

	// *** Basic block 29

	addi        a0, s0, -96
	call        StringDestruct

	// *** Basic block 30

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 31

	j           .Assemble_adrtls_label_68
.func_end_Assemble_adrtls:
	.size Assemble_adrtls, .func_end_Assemble_adrtls-Assemble_adrtls

	.local  Assemble_esc
	.type Assemble_esc, @function

Assemble_esc:

	// *** Basic block 0

	.global LexMatch
	.global AssemblerEvaluateExpression
	.global AssemblerEmitWord
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
	li          a1, 97		// 0x61 ASCII 'a'
	call        LexMatch

	// *** Basic block 1

	beqz        a0, .Assemble_esc_label_44

	// *** Basic block 2

	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 3

	mv          s2, a0
	lw          a1, 744(s1)
	li          t0, 16777215		// 0xffffff
	and         t0, s2, t0
	sext.w      t0, t0
	li          t1, 1241513984		// 0x4a000000
	or          a2, t1, t0
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 4

	j           .Assemble_esc_label_52

	// *** Basic block 5

.Assemble_esc_label_44:
	lla         a1, .str.256
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssemblerError

	// *** Basic block 6

.Assemble_esc_label_52:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_esc:
	.size Assemble_esc, .func_end_Assemble_esc-Assemble_esc

.PCend:
	.data
reloc_types:
	.type   reloc_types,@object
	.local  reloc_types
	.size   reloc_types,36
	.p2align  2
	.word   6
	.word   5
	.word   4
	.word   6
	.word   7
	.word   8
	.word   9
	.word   10
	.word   11

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "decsp"
	.type .str.1, @object
	.size .str.1, 6

.str.2:
	.asciz "incsp"
	.type .str.2, @object
	.size .str.2, 6

.str.3:
	.asciz "movc"
	.type .str.3, @object
	.size .str.3, 5

.str.4:
	.asciz "movfc"
	.type .str.4, @object
	.size .str.4, 6

.str.5:
	.asciz "movdc"
	.type .str.5, @object
	.size .str.5, 6

.str.6:
	.asciz "movxc"
	.type .str.6, @object
	.size .str.6, 6

.str.7:
	.asciz "mov"
	.type .str.7, @object
	.size .str.7, 4

.str.8:
	.asciz "movf"
	.type .str.8, @object
	.size .str.8, 5

.str.9:
	.asciz "movd"
	.type .str.9, @object
	.size .str.9, 5

.str.10:
	.asciz "push"
	.type .str.10, @object
	.size .str.10, 5

.str.11:
	.asciz "pushf"
	.type .str.11, @object
	.size .str.11, 6

.str.12:
	.asciz "pushd"
	.type .str.12, @object
	.size .str.12, 6

.str.13:
	.asciz "pushx"
	.type .str.13, @object
	.size .str.13, 6

.str.14:
	.asciz "pop"
	.type .str.14, @object
	.size .str.14, 4

.str.15:
	.asciz "popf"
	.type .str.15, @object
	.size .str.15, 5

.str.16:
	.asciz "popd"
	.type .str.16, @object
	.size .str.16, 5

.str.17:
	.asciz "popx"
	.type .str.17, @object
	.size .str.17, 5

.str.18:
	.asciz "add"
	.type .str.18, @object
	.size .str.18, 4

.str.19:
	.asciz "addf"
	.type .str.19, @object
	.size .str.19, 5

.str.20:
	.asciz "addd"
	.type .str.20, @object
	.size .str.20, 5

.str.21:
	.asciz "addc"
	.type .str.21, @object
	.size .str.21, 5

.str.22:
	.asciz "ldw"
	.type .str.22, @object
	.size .str.22, 4

.str.23:
	.asciz "ldh"
	.type .str.23, @object
	.size .str.23, 4

.str.24:
	.asciz "ldb"
	.type .str.24, @object
	.size .str.24, 4

.str.25:
	.asciz "lduw"
	.type .str.25, @object
	.size .str.25, 5

.str.26:
	.asciz "ldub"
	.type .str.26, @object
	.size .str.26, 5

.str.27:
	.asciz "lduh"
	.type .str.27, @object
	.size .str.27, 5

.str.28:
	.asciz "ldx"
	.type .str.28, @object
	.size .str.28, 4

.str.29:
	.asciz "ldf"
	.type .str.29, @object
	.size .str.29, 4

.str.30:
	.asciz "ldd"
	.type .str.30, @object
	.size .str.30, 4

.str.31:
	.asciz "stw"
	.type .str.31, @object
	.size .str.31, 4

.str.32:
	.asciz "sth"
	.type .str.32, @object
	.size .str.32, 4

.str.33:
	.asciz "stx"
	.type .str.33, @object
	.size .str.33, 4

.str.34:
	.asciz "stf"
	.type .str.34, @object
	.size .str.34, 4

.str.35:
	.asciz "std"
	.type .str.35, @object
	.size .str.35, 4

.str.36:
	.asciz "stb"
	.type .str.36, @object
	.size .str.36, 4

.str.37:
	.asciz "sub"
	.type .str.37, @object
	.size .str.37, 4

.str.38:
	.asciz "subf"
	.type .str.38, @object
	.size .str.38, 5

.str.39:
	.asciz "subd"
	.type .str.39, @object
	.size .str.39, 5

.str.40:
	.asciz "mul"
	.type .str.40, @object
	.size .str.40, 4

.str.41:
	.asciz "mulf"
	.type .str.41, @object
	.size .str.41, 5

.str.42:
	.asciz "muld"
	.type .str.42, @object
	.size .str.42, 5

.str.43:
	.asciz "div"
	.type .str.43, @object
	.size .str.43, 4

.str.44:
	.asciz "divu"
	.type .str.44, @object
	.size .str.44, 5

.str.45:
	.asciz "divf"
	.type .str.45, @object
	.size .str.45, 5

.str.46:
	.asciz "divd"
	.type .str.46, @object
	.size .str.46, 5

.str.47:
	.asciz "mod"
	.type .str.47, @object
	.size .str.47, 4

.str.48:
	.asciz "modu"
	.type .str.48, @object
	.size .str.48, 5

.str.49:
	.asciz "lsr"
	.type .str.49, @object
	.size .str.49, 4

.str.50:
	.asciz "asr"
	.type .str.50, @object
	.size .str.50, 4

.str.51:
	.asciz "lsl"
	.type .str.51, @object
	.size .str.51, 4

.str.52:
	.asciz "or"
	.type .str.52, @object
	.size .str.52, 3

.str.53:
	.asciz "and"
	.type .str.53, @object
	.size .str.53, 4

.str.54:
	.asciz "xor"
	.type .str.54, @object
	.size .str.54, 4

.str.55:
	.asciz "not"
	.type .str.55, @object
	.size .str.55, 4

.str.56:
	.asciz "inv"
	.type .str.56, @object
	.size .str.56, 4

.str.57:
	.asciz "neg"
	.type .str.57, @object
	.size .str.57, 4

.str.58:
	.asciz "negf"
	.type .str.58, @object
	.size .str.58, 5

.str.59:
	.asciz "negd"
	.type .str.59, @object
	.size .str.59, 5

.str.60:
	.asciz "cmpeq"
	.type .str.60, @object
	.size .str.60, 6

.str.61:
	.asciz "cmpne"
	.type .str.61, @object
	.size .str.61, 6

.str.62:
	.asciz "cmplt"
	.type .str.62, @object
	.size .str.62, 6

.str.63:
	.asciz "cmple"
	.type .str.63, @object
	.size .str.63, 6

.str.64:
	.asciz "cmpgt"
	.type .str.64, @object
	.size .str.64, 6

.str.65:
	.asciz "cmpge"
	.type .str.65, @object
	.size .str.65, 6

.str.66:
	.asciz "cmpltu"
	.type .str.66, @object
	.size .str.66, 7

.str.67:
	.asciz "cmpleu"
	.type .str.67, @object
	.size .str.67, 7

.str.68:
	.asciz "cmpgtu"
	.type .str.68, @object
	.size .str.68, 7

.str.69:
	.asciz "cmpgeu"
	.type .str.69, @object
	.size .str.69, 7

.str.70:
	.asciz "cmpeqf"
	.type .str.70, @object
	.size .str.70, 7

.str.71:
	.asciz "cmpnef"
	.type .str.71, @object
	.size .str.71, 7

.str.72:
	.asciz "cmpltf"
	.type .str.72, @object
	.size .str.72, 7

.str.73:
	.asciz "cmplef"
	.type .str.73, @object
	.size .str.73, 7

.str.74:
	.asciz "cmpgtf"
	.type .str.74, @object
	.size .str.74, 7

.str.75:
	.asciz "cmpgef"
	.type .str.75, @object
	.size .str.75, 7

.str.76:
	.asciz "cmpeqd"
	.type .str.76, @object
	.size .str.76, 7

.str.77:
	.asciz "cmpned"
	.type .str.77, @object
	.size .str.77, 7

.str.78:
	.asciz "cmpltd"
	.type .str.78, @object
	.size .str.78, 7

.str.79:
	.asciz "cmpled"
	.type .str.79, @object
	.size .str.79, 7

.str.80:
	.asciz "cmpgtd"
	.type .str.80, @object
	.size .str.80, 7

.str.81:
	.asciz "cmpged"
	.type .str.81, @object
	.size .str.81, 7

.str.82:
	.asciz "bnz"
	.type .str.82, @object
	.size .str.82, 4

.str.83:
	.asciz "bz"
	.type .str.83, @object
	.size .str.83, 3

.str.84:
	.asciz "bra"
	.type .str.84, @object
	.size .str.84, 4

.str.85:
	.asciz "cbra"
	.type .str.85, @object
	.size .str.85, 5

.str.86:
	.asciz "i2f"
	.type .str.86, @object
	.size .str.86, 4

.str.87:
	.asciz "i2d"
	.type .str.87, @object
	.size .str.87, 4

.str.88:
	.asciz "ui2f"
	.type .str.88, @object
	.size .str.88, 5

.str.89:
	.asciz "ui2d"
	.type .str.89, @object
	.size .str.89, 5

.str.90:
	.asciz "f2d"
	.type .str.90, @object
	.size .str.90, 4

.str.91:
	.asciz "d2f"
	.type .str.91, @object
	.size .str.91, 4

.str.92:
	.asciz "f2i"
	.type .str.92, @object
	.size .str.92, 4

.str.93:
	.asciz "d2i"
	.type .str.93, @object
	.size .str.93, 4

.str.94:
	.asciz "f2ui"
	.type .str.94, @object
	.size .str.94, 5

.str.95:
	.asciz "d2ui"
	.type .str.95, @object
	.size .str.95, 5

.str.96:
	.asciz "jmp"
	.type .str.96, @object
	.size .str.96, 4

.str.97:
	.asciz "cjmp"
	.type .str.97, @object
	.size .str.97, 5

.str.98:
	.asciz "adr"
	.type .str.98, @object
	.size .str.98, 4

.str.99:
	.asciz "adrs"
	.type .str.99, @object
	.size .str.99, 5

.str.100:
	.asciz "adrtls"
	.type .str.100, @object
	.size .str.100, 7

.str.101:
	.asciz "call"
	.type .str.101, @object
	.size .str.101, 5

.str.102:
	.asciz "rcall"
	.type .str.102, @object
	.size .str.102, 6

.str.103:
	.asciz "ret"
	.type .str.103, @object
	.size .str.103, 4

.str.104:
	.asciz "esc"
	.type .str.104, @object
	.size .str.104, 4

.str.105:
	.asciz ".bss"
	.type .str.105, @object
	.size .str.105, 5

.str.106:
	.asciz "Syntax error; unknown instruction: %s"
	.type .str.106, @object
	.size .str.106, 38

.str.107:
	.asciz "Illegal register name"
	.type .str.107, @object
	.size .str.107, 22

.str.108:
	.asciz "Illegal register number %d"
	.type .str.108, @object
	.size .str.108, 27

.str.109:
	.asciz "sp"
	.type .str.109, @object
	.size .str.109, 3

.str.110:
	.asciz "fp"
	.type .str.110, @object
	.size .str.110, 3

.str.111:
	.asciz "ap"
	.type .str.111, @object
	.size .str.111, 3

.str.112:
	.asciz "tp"
	.type .str.112, @object
	.size .str.112, 3

.str.113:
	.asciz "Expected %s register name"
	.type .str.113, @object
	.size .str.113, 26

.str.114:
	.asciz "Invalid register type; got %c expected %c"
	.type .str.114, @object
	.size .str.114, 42

.str.115:
	.asciz "Missing comma"
	.type .str.115, @object
	.size .str.115, 14

.str.116:
	.asciz "Missing comma"
	.type .str.116, @object
	.size .str.116, 14

.str.117:
	.asciz "integer"
	.type .str.117, @object
	.size .str.117, 8

.str.118:
	.asciz "Missing comma"
	.type .str.118, @object
	.size .str.118, 14

.str.119:
	.asciz "Missing comma"
	.type .str.119, @object
	.size .str.119, 14

.str.120:
	.asciz "Missing comma"
	.type .str.120, @object
	.size .str.120, 14

.str.121:
	.asciz "integer"
	.type .str.121, @object
	.size .str.121, 8

.str.122:
	.asciz "integer"
	.type .str.122, @object
	.size .str.122, 8

.str.123:
	.asciz "float"
	.type .str.123, @object
	.size .str.123, 6

.str.124:
	.asciz "double"
	.type .str.124, @object
	.size .str.124, 7

.str.125:
	.asciz "float"
	.type .str.125, @object
	.size .str.125, 6

.str.126:
	.asciz "double"
	.type .str.126, @object
	.size .str.126, 7

.str.127:
	.asciz "integer"
	.type .str.127, @object
	.size .str.127, 8

.str.128:
	.asciz "float"
	.type .str.128, @object
	.size .str.128, 6

.str.129:
	.asciz "double"
	.type .str.129, @object
	.size .str.129, 7

.str.130:
	.asciz "integer"
	.type .str.130, @object
	.size .str.130, 8

.str.131:
	.asciz "integer"
	.type .str.131, @object
	.size .str.131, 8

.str.132:
	.asciz "float"
	.type .str.132, @object
	.size .str.132, 6

.str.133:
	.asciz "double"
	.type .str.133, @object
	.size .str.133, 7

.str.134:
	.asciz "integer"
	.type .str.134, @object
	.size .str.134, 8

.str.135:
	.asciz "integer"
	.type .str.135, @object
	.size .str.135, 8

.str.136:
	.asciz "integer"
	.type .str.136, @object
	.size .str.136, 8

.str.137:
	.asciz "integer"
	.type .str.137, @object
	.size .str.137, 8

.str.138:
	.asciz "integer"
	.type .str.138, @object
	.size .str.138, 8

.str.139:
	.asciz "integer"
	.type .str.139, @object
	.size .str.139, 8

.str.140:
	.asciz "integer"
	.type .str.140, @object
	.size .str.140, 8

.str.141:
	.asciz "integer"
	.type .str.141, @object
	.size .str.141, 8

.str.142:
	.asciz "integer"
	.type .str.142, @object
	.size .str.142, 8

.str.143:
	.asciz "integer"
	.type .str.143, @object
	.size .str.143, 8

.str.144:
	.asciz "integer"
	.type .str.144, @object
	.size .str.144, 8

.str.145:
	.asciz "float"
	.type .str.145, @object
	.size .str.145, 6

.str.146:
	.asciz "double"
	.type .str.146, @object
	.size .str.146, 7

.str.147:
	.asciz "integer"
	.type .str.147, @object
	.size .str.147, 8

.str.148:
	.asciz "integer"
	.type .str.148, @object
	.size .str.148, 8

.str.149:
	.asciz "integer"
	.type .str.149, @object
	.size .str.149, 8

.str.150:
	.asciz "integer"
	.type .str.150, @object
	.size .str.150, 8

.str.151:
	.asciz "integer"
	.type .str.151, @object
	.size .str.151, 8

.str.152:
	.asciz "integer"
	.type .str.152, @object
	.size .str.152, 8

.str.153:
	.asciz "integer"
	.type .str.153, @object
	.size .str.153, 8

.str.154:
	.asciz "integer"
	.type .str.154, @object
	.size .str.154, 8

.str.155:
	.asciz "integer"
	.type .str.155, @object
	.size .str.155, 8

.str.156:
	.asciz "integer"
	.type .str.156, @object
	.size .str.156, 8

.str.157:
	.asciz "float"
	.type .str.157, @object
	.size .str.157, 6

.str.158:
	.asciz "float"
	.type .str.158, @object
	.size .str.158, 6

.str.159:
	.asciz "float"
	.type .str.159, @object
	.size .str.159, 6

.str.160:
	.asciz "float"
	.type .str.160, @object
	.size .str.160, 6

.str.161:
	.asciz "float"
	.type .str.161, @object
	.size .str.161, 6

.str.162:
	.asciz "float"
	.type .str.162, @object
	.size .str.162, 6

.str.163:
	.asciz "double"
	.type .str.163, @object
	.size .str.163, 7

.str.164:
	.asciz "double"
	.type .str.164, @object
	.size .str.164, 7

.str.165:
	.asciz "double"
	.type .str.165, @object
	.size .str.165, 7

.str.166:
	.asciz "double"
	.type .str.166, @object
	.size .str.166, 7

.str.167:
	.asciz "double"
	.type .str.167, @object
	.size .str.167, 7

.str.168:
	.asciz "double"
	.type .str.168, @object
	.size .str.168, 7

.str.169:
	.asciz "Missing comma"
	.type .str.169, @object
	.size .str.169, 14

.str.170:
	.asciz "integer"
	.type .str.170, @object
	.size .str.170, 8

.str.171:
	.asciz "float"
	.type .str.171, @object
	.size .str.171, 6

.str.172:
	.asciz "integer"
	.type .str.172, @object
	.size .str.172, 8

.str.173:
	.asciz "float"
	.type .str.173, @object
	.size .str.173, 6

.str.174:
	.asciz "integer"
	.type .str.174, @object
	.size .str.174, 8

.str.175:
	.asciz "double"
	.type .str.175, @object
	.size .str.175, 7

.str.176:
	.asciz "integer"
	.type .str.176, @object
	.size .str.176, 8

.str.177:
	.asciz "double"
	.type .str.177, @object
	.size .str.177, 7

.str.178:
	.asciz "double"
	.type .str.178, @object
	.size .str.178, 7

.str.179:
	.asciz "float"
	.type .str.179, @object
	.size .str.179, 6

.str.180:
	.asciz "float"
	.type .str.180, @object
	.size .str.180, 6

.str.181:
	.asciz "double"
	.type .str.181, @object
	.size .str.181, 7

.str.182:
	.asciz "float"
	.type .str.182, @object
	.size .str.182, 6

.str.183:
	.asciz "integer"
	.type .str.183, @object
	.size .str.183, 8

.str.184:
	.asciz "double"
	.type .str.184, @object
	.size .str.184, 7

.str.185:
	.asciz "integer"
	.type .str.185, @object
	.size .str.185, 8

.str.186:
	.asciz "float"
	.type .str.186, @object
	.size .str.186, 6

.str.187:
	.asciz "integer"
	.type .str.187, @object
	.size .str.187, 8

.str.188:
	.asciz "double"
	.type .str.188, @object
	.size .str.188, 7

.str.189:
	.asciz "integer"
	.type .str.189, @object
	.size .str.189, 8

.str.190:
	.asciz "Immediate expression expected"
	.type .str.190, @object
	.size .str.190, 30

.str.191:
	.asciz "Immediate expression expected"
	.type .str.191, @object
	.size .str.191, 30

.str.192:
	.asciz "Missing comma"
	.type .str.192, @object
	.size .str.192, 14

.str.193:
	.asciz "Missing ["
	.type .str.193, @object
	.size .str.193, 10

.str.194:
	.asciz "integer"
	.type .str.194, @object
	.size .str.194, 8

.str.195:
	.asciz "Missing comma"
	.type .str.195, @object
	.size .str.195, 14

.str.196:
	.asciz "Missing # offset"
	.type .str.196, @object
	.size .str.196, 17

.str.197:
	.asciz "Missing ]"
	.type .str.197, @object
	.size .str.197, 10

.str.198:
	.asciz "integer"
	.type .str.198, @object
	.size .str.198, 8

.str.199:
	.asciz "integer"
	.type .str.199, @object
	.size .str.199, 8

.str.200:
	.asciz "integer"
	.type .str.200, @object
	.size .str.200, 8

.str.201:
	.asciz "integer"
	.type .str.201, @object
	.size .str.201, 8

.str.202:
	.asciz "integer"
	.type .str.202, @object
	.size .str.202, 8

.str.203:
	.asciz "integer"
	.type .str.203, @object
	.size .str.203, 8

.str.204:
	.asciz "integer"
	.type .str.204, @object
	.size .str.204, 8

.str.205:
	.asciz "float"
	.type .str.205, @object
	.size .str.205, 6

.str.206:
	.asciz "double"
	.type .str.206, @object
	.size .str.206, 7

.str.207:
	.asciz "integer"
	.type .str.207, @object
	.size .str.207, 8

.str.208:
	.asciz "integer"
	.type .str.208, @object
	.size .str.208, 8

.str.209:
	.asciz "integer"
	.type .str.209, @object
	.size .str.209, 8

.str.210:
	.asciz "integer"
	.type .str.210, @object
	.size .str.210, 8

.str.211:
	.asciz "float"
	.type .str.211, @object
	.size .str.211, 6

.str.212:
	.asciz "double"
	.type .str.212, @object
	.size .str.212, 7

.str.213:
	.asciz "Missing comma"
	.type .str.213, @object
	.size .str.213, 14

.str.214:
	.asciz "Illegal symbol reference instruction"
	.type .str.214, @object
	.size .str.214, 37

.str.215:
	.asciz "Invalid mov operand"
	.type .str.215, @object
	.size .str.215, 20

.str.216:
	.asciz "tls"
	.type .str.216, @object
	.size .str.216, 4

.str.217:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.217, @object
	.size .str.217, 30

.str.218:
	.asciz "(null)"
	.type .str.218, @object
	.size .str.218, 1

.str.219:
	.asciz "false"
	.type .str.219, @object
	.size .str.219, 6

.str.220:
	.asciz "integer"
	.type .str.220, @object
	.size .str.220, 8

.str.221:
	.asciz "float"
	.type .str.221, @object
	.size .str.221, 6

.str.222:
	.asciz "double"
	.type .str.222, @object
	.size .str.222, 7

.str.223:
	.asciz "integer"
	.type .str.223, @object
	.size .str.223, 8

.str.224:
	.asciz "integer"
	.type .str.224, @object
	.size .str.224, 8

.str.225:
	.asciz "float"
	.type .str.225, @object
	.size .str.225, 6

.str.226:
	.asciz "double"
	.type .str.226, @object
	.size .str.226, 7

.str.227:
	.asciz "integer"
	.type .str.227, @object
	.size .str.227, 8

.str.228:
	.asciz "float"
	.type .str.228, @object
	.size .str.228, 6

.str.229:
	.asciz "double"
	.type .str.229, @object
	.size .str.229, 7

.str.230:
	.asciz "integer"
	.type .str.230, @object
	.size .str.230, 8

.str.231:
	.asciz "integer"
	.type .str.231, @object
	.size .str.231, 8

.str.232:
	.asciz "float"
	.type .str.232, @object
	.size .str.232, 6

.str.233:
	.asciz "double"
	.type .str.233, @object
	.size .str.233, 7

.str.234:
	.asciz "integer"
	.type .str.234, @object
	.size .str.234, 8

.str.235:
	.asciz "integer"
	.type .str.235, @object
	.size .str.235, 8

.str.236:
	.asciz "Missing comma"
	.type .str.236, @object
	.size .str.236, 14

.str.237:
	.asciz "integer"
	.type .str.237, @object
	.size .str.237, 8

.str.238:
	.asciz "Missing comma"
	.type .str.238, @object
	.size .str.238, 14

.str.239:
	.asciz "Missing #constant"
	.type .str.239, @object
	.size .str.239, 18

.str.240:
	.asciz "integer"
	.type .str.240, @object
	.size .str.240, 8

.str.241:
	.asciz "integer"
	.type .str.241, @object
	.size .str.241, 8

.str.242:
	.asciz "Missing symbol for call instruction"
	.type .str.242, @object
	.size .str.242, 36

.str.243:
	.asciz "Missing symbol for jmp instruction"
	.type .str.243, @object
	.size .str.243, 35

.str.244:
	.asciz "Missing symbol for cjmp instruction"
	.type .str.244, @object
	.size .str.244, 36

.str.245:
	.asciz "integer"
	.type .str.245, @object
	.size .str.245, 8

.str.246:
	.asciz "Missing comma"
	.type .str.246, @object
	.size .str.246, 14

.str.247:
	.asciz "Missing symbol for adr instruction"
	.type .str.247, @object
	.size .str.247, 35

.str.248:
	.asciz "tls"
	.type .str.248, @object
	.size .str.248, 4

.str.249:
	.asciz "integer"
	.type .str.249, @object
	.size .str.249, 8

.str.250:
	.asciz "Missing comma"
	.type .str.250, @object
	.size .str.250, 14

.str.251:
	.asciz "Missing symbol for adrs instruction"
	.type .str.251, @object
	.size .str.251, 36

.str.252:
	.asciz "integer"
	.type .str.252, @object
	.size .str.252, 8

.str.253:
	.asciz "Missing comma"
	.type .str.253, @object
	.size .str.253, 14

.str.254:
	.asciz "Missing symbol for adrtls instruction"
	.type .str.254, @object
	.size .str.254, 38

.str.255:
	.asciz "tls"
	.type .str.255, @object
	.size .str.255, 4

.str.256:
	.asciz "Missing #value for esc instruction"
	.type .str.256, @object
	.size .str.256, 35

