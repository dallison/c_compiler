	.file   "risc_v_assembler.c"
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

	.local Assemble_mv
	.global MapInsert
	.local Assemble_fmv_s
	.local Assemble_fmv_d
	.local Assemble_ret
	.local Assemble_lui
	.local Assemble_auipc
	.local Assemble_jal
	.local Assemble_jalr
	.local Assemble_beq
	.local Assemble_bne
	.local Assemble_blt
	.local Assemble_bge
	.local Assemble_bltu
	.local Assemble_bgeu
	.local Assemble_lb
	.local Assemble_lh
	.local Assemble_lw
	.local Assemble_lbu
	.local Assemble_lhu
	.local Assemble_sb
	.local Assemble_sh
	.local Assemble_sw
	.local Assemble_addi
	.local Assemble_slti
	.local Assemble_sltiu
	.local Assemble_xori
	.local Assemble_ori
	.local Assemble_andi
	.local Assemble_slli
	.local Assemble_srli
	.local Assemble_srai
	.local Assemble_add
	.local Assemble_sub
	.local Assemble_sll
	.local Assemble_slt
	.local Assemble_sltu
	.local Assemble_xor
	.local Assemble_srl
	.local Assemble_sra
	.local Assemble_or
	.local Assemble_and
	.local Assemble_fence
	.local Assemble_fence_i
	.local Assemble_ecall
	.local Assemble_ebreak
	.local Assemble_csrrw
	.local Assemble_csrrs
	.local Assemble_csrrc
	.local Assemble_csrrwi
	.local Assemble_csrrsi
	.local Assemble_csrrci
	.local Assemble_lwu
	.local Assemble_ld
	.local Assemble_sd
	.local Assemble_addiw
	.local Assemble_slliw
	.local Assemble_srliw
	.local Assemble_sraiw
	.local Assemble_addw
	.local Assemble_subw
	.local Assemble_sllw
	.local Assemble_srlw
	.local Assemble_sraw
	.local Assemble_mul
	.local Assemble_mulh
	.local Assemble_mulhsu
	.local Assemble_mulhu
	.local Assemble_div
	.local Assemble_divu
	.local Assemble_rem
	.local Assemble_remu
	.local Assemble_mulw
	.local Assemble_divw
	.local Assemble_divuw
	.local Assemble_remw
	.local Assemble_remuw
	.local Assemble_flw
	.local Assemble_fsw
	.local Assemble_fmadd_s
	.local Assemble_fmsub_s
	.local Assemble_fnmsub_s
	.local Assemble_fnmadd_s
	.local Assemble_fadd_s
	.local Assemble_fsub_s
	.local Assemble_fmul_s
	.local Assemble_fdiv_s
	.local Assemble_fsqrt_s
	.local Assemble_fsgnj_s
	.local Assemble_fsgnjn_s
	.local Assemble_fsgnjx_s
	.local Assemble_fmin_s
	.local Assemble_fmax_s
	.local Assemble_fcvt_w_s
	.local Assemble_fcvt_wu_s
	.local Assemble_fmv_x_w
	.local Assemble_feq_s
	.local Assemble_flt_s
	.local Assemble_fle_s
	.local Assemble_fclass_s
	.local Assemble_fcvt_s_w
	.local Assemble_fcvt_s_wu
	.local Assemble_fmv_w_x
	.local Assemble_fcvt_l_s
	.local Assemble_fcvt_lu_s
	.local Assemble_fcvt_s_l
	.local Assemble_fcvt_s_lu
	.local Assemble_fld
	.local Assemble_fsd
	.local Assemble_fmadd_d
	.local Assemble_fmsub_d
	.local Assemble_fnmsub_d
	.local Assemble_fnmadd_d
	.local Assemble_fadd_d
	.local Assemble_fsub_d
	.local Assemble_fmul_d
	.local Assemble_fdiv_d
	.local Assemble_fsqrt_d
	.local Assemble_fsgnj_d
	.local Assemble_fsgnjn_d
	.local Assemble_fsgnjx_d
	.local Assemble_fmin_d
	.local Assemble_fmax_d
	.local Assemble_fcvt_s_d
	.local Assemble_fcvt_d_s
	.local Assemble_feq_d
	.local Assemble_flt_d
	.local Assemble_fle_d
	.local Assemble_fclass_d
	.local Assemble_fcvt_w_d
	.local Assemble_fcvt_wu_d
	.local Assemble_fcvt_d_w
	.local Assemble_fcvt_d_wu
	.local Assemble_fcvt_l_d
	.local Assemble_fcvt_lu_d
	.local Assemble_fmv_x_d
	.local Assemble_fcvt_d_l
	.local Assemble_fcvt_d_lu
	.local Assemble_fmv_d_x
	.local Assemble_nop
	.local Assemble_not
	.local Assemble_neg
	.local Assemble_fneg_s
	.local Assemble_fneg_d
	.local Assemble_li
	.local Assemble_la
	.local Assemble_lla
	.local Assemble_sext_w
	.local Assemble_seqz
	.local Assemble_snez
	.local Assemble_sltz
	.local Assemble_sgtz
	.local Assemble_beqz
	.local Assemble_bnez
	.local Assemble_j
	.local Assemble_jr
	.local Assemble_call
	.local Assemble_rcall
	.local Assemble_callf
	.local Assemble_rcallf
	addi sp, sp, -16
	sd ra, 8(sp)
	sd s0, 0(sp)
	lui t0, 0
	addi t0, t0, 2576
	sub sp, sp, t0
	addi t0, t0, 16
	add s0, sp, t0
	// Local vars at offset -2560(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0

	// *** Basic block 1

.InitializeInstructions_label_325:
	lla         t0, .str.1
	li          s2, -2048		// 0xfffffffffffff800
	add         s3, s0, s2
	sd          t0, -512(s3)
	addi        t0, s3, -512
	la          t1, Assemble_mv
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -512(s3)
	sd          t0, 0(sp)
	ld          t0, -504(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 2

	addi        sp, sp, 16

	// *** Basic block 3

.InitializeInstructions_label_353:

	// *** Basic block 4

.InitializeInstructions_label_354:

	// *** Basic block 5

.InitializeInstructions_label_355:
	lla         t0, .str.2
	sd          t0, -496(s3)
	addi        t0, s3, -496
	la          t1, Assemble_fmv_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -496(s3)
	sd          t0, 0(sp)
	ld          t0, -488(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 6

	addi        sp, sp, 16

	// *** Basic block 7

.InitializeInstructions_label_375:

	// *** Basic block 8

.InitializeInstructions_label_376:

	// *** Basic block 9

.InitializeInstructions_label_377:
	lla         t0, .str.3
	sd          t0, -480(s3)
	addi        t0, s3, -480
	la          t1, Assemble_fmv_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -480(s3)
	sd          t0, 0(sp)
	ld          t0, -472(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 10

	addi        sp, sp, 16

	// *** Basic block 11

.InitializeInstructions_label_397:

	// *** Basic block 12

.InitializeInstructions_label_398:

	// *** Basic block 13

.InitializeInstructions_label_399:
	lla         t0, .str.4
	sd          t0, -464(s3)
	addi        t0, s3, -464
	la          t1, Assemble_ret
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -464(s3)
	sd          t0, 0(sp)
	ld          t0, -456(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 14

	addi        sp, sp, 16

	// *** Basic block 15

.InitializeInstructions_label_419:

	// *** Basic block 16

.InitializeInstructions_label_420:

	// *** Basic block 17

.InitializeInstructions_label_421:
	lla         t0, .str.5
	sd          t0, -448(s3)
	addi        t0, s3, -448
	la          t1, Assemble_lui
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -448(s3)
	sd          t0, 0(sp)
	ld          t0, -440(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 18

	addi        sp, sp, 16

	// *** Basic block 19

.InitializeInstructions_label_441:

	// *** Basic block 20

.InitializeInstructions_label_442:

	// *** Basic block 21

.InitializeInstructions_label_443:
	lla         t0, .str.6
	sd          t0, -432(s3)
	addi        t0, s3, -432
	la          t1, Assemble_auipc
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -432(s3)
	sd          t0, 0(sp)
	ld          t0, -424(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 22

	addi        sp, sp, 16

	// *** Basic block 23

.InitializeInstructions_label_463:

	// *** Basic block 24

.InitializeInstructions_label_464:

	// *** Basic block 25

.InitializeInstructions_label_465:
	lla         t0, .str.7
	sd          t0, -416(s3)
	addi        t0, s3, -416
	la          t1, Assemble_jal
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -416(s3)
	sd          t0, 0(sp)
	ld          t0, -408(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 26

	addi        sp, sp, 16

	// *** Basic block 27

.InitializeInstructions_label_485:

	// *** Basic block 28

.InitializeInstructions_label_486:

	// *** Basic block 29

.InitializeInstructions_label_487:
	lla         t0, .str.8
	sd          t0, -400(s3)
	addi        t0, s3, -400
	la          t1, Assemble_jalr
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -400(s3)
	sd          t0, 0(sp)
	ld          t0, -392(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 30

	addi        sp, sp, 16

	// *** Basic block 31

.InitializeInstructions_label_507:

	// *** Basic block 32

.InitializeInstructions_label_508:

	// *** Basic block 33

.InitializeInstructions_label_509:
	lla         t0, .str.9
	sd          t0, -384(s3)
	addi        t0, s3, -384
	la          t1, Assemble_beq
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -384(s3)
	sd          t0, 0(sp)
	ld          t0, -376(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 34

	addi        sp, sp, 16

	// *** Basic block 35

.InitializeInstructions_label_529:

	// *** Basic block 36

.InitializeInstructions_label_530:

	// *** Basic block 37

.InitializeInstructions_label_531:
	lla         t0, .str.10
	sd          t0, -368(s3)
	addi        t0, s3, -368
	la          t1, Assemble_bne
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -368(s3)
	sd          t0, 0(sp)
	ld          t0, -360(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 38

	addi        sp, sp, 16

	// *** Basic block 39

.InitializeInstructions_label_551:

	// *** Basic block 40

.InitializeInstructions_label_552:

	// *** Basic block 41

.InitializeInstructions_label_553:
	lla         t0, .str.11
	sd          t0, -352(s3)
	addi        t0, s3, -352
	la          t1, Assemble_blt
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -352(s3)
	sd          t0, 0(sp)
	ld          t0, -344(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 42

	addi        sp, sp, 16

	// *** Basic block 43

.InitializeInstructions_label_573:

	// *** Basic block 44

.InitializeInstructions_label_574:

	// *** Basic block 45

.InitializeInstructions_label_575:
	lla         t0, .str.12
	sd          t0, -336(s3)
	addi        t0, s3, -336
	la          t1, Assemble_bge
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -336(s3)
	sd          t0, 0(sp)
	ld          t0, -328(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 46

	addi        sp, sp, 16

	// *** Basic block 47

.InitializeInstructions_label_595:

	// *** Basic block 48

.InitializeInstructions_label_596:

	// *** Basic block 49

.InitializeInstructions_label_597:
	lla         t0, .str.13
	sd          t0, -320(s3)
	addi        t0, s3, -320
	la          t1, Assemble_bltu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -320(s3)
	sd          t0, 0(sp)
	ld          t0, -312(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 50

	addi        sp, sp, 16

	// *** Basic block 51

.InitializeInstructions_label_617:

	// *** Basic block 52

.InitializeInstructions_label_618:

	// *** Basic block 53

.InitializeInstructions_label_619:
	lla         t0, .str.14
	sd          t0, -304(s3)
	addi        t0, s3, -304
	la          t1, Assemble_bgeu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -304(s3)
	sd          t0, 0(sp)
	ld          t0, -296(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 54

	addi        sp, sp, 16

	// *** Basic block 55

.InitializeInstructions_label_639:

	// *** Basic block 56

.InitializeInstructions_label_640:

	// *** Basic block 57

.InitializeInstructions_label_641:
	lla         t0, .str.15
	sd          t0, -288(s3)
	addi        t0, s3, -288
	la          t1, Assemble_lb
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -288(s3)
	sd          t0, 0(sp)
	ld          t0, -280(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 58

	addi        sp, sp, 16

	// *** Basic block 59

.InitializeInstructions_label_661:

	// *** Basic block 60

.InitializeInstructions_label_662:

	// *** Basic block 61

.InitializeInstructions_label_663:
	lla         t0, .str.16
	sd          t0, -272(s3)
	addi        t0, s3, -272
	la          t1, Assemble_lh
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -272(s3)
	sd          t0, 0(sp)
	ld          t0, -264(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 62

	addi        sp, sp, 16

	// *** Basic block 63

.InitializeInstructions_label_683:

	// *** Basic block 64

.InitializeInstructions_label_684:

	// *** Basic block 65

.InitializeInstructions_label_685:
	lla         t0, .str.17
	sd          t0, -256(s3)
	addi        t0, s3, -256
	la          t1, Assemble_lw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -256(s3)
	sd          t0, 0(sp)
	ld          t0, -248(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 66

	addi        sp, sp, 16

	// *** Basic block 67

.InitializeInstructions_label_705:

	// *** Basic block 68

.InitializeInstructions_label_706:

	// *** Basic block 69

.InitializeInstructions_label_707:
	lla         t0, .str.18
	sd          t0, -240(s3)
	addi        t0, s3, -240
	la          t1, Assemble_lbu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -240(s3)
	sd          t0, 0(sp)
	ld          t0, -232(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 70

	addi        sp, sp, 16

	// *** Basic block 71

.InitializeInstructions_label_727:

	// *** Basic block 72

.InitializeInstructions_label_728:

	// *** Basic block 73

.InitializeInstructions_label_729:
	lla         t0, .str.19
	sd          t0, -224(s3)
	addi        t0, s3, -224
	la          t1, Assemble_lhu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -224(s3)
	sd          t0, 0(sp)
	ld          t0, -216(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 74

	addi        sp, sp, 16

	// *** Basic block 75

.InitializeInstructions_label_749:

	// *** Basic block 76

.InitializeInstructions_label_750:

	// *** Basic block 77

.InitializeInstructions_label_751:
	lla         t0, .str.20
	sd          t0, -208(s3)
	addi        t0, s3, -208
	la          t1, Assemble_sb
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -208(s3)
	sd          t0, 0(sp)
	ld          t0, -200(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 78

	addi        sp, sp, 16

	// *** Basic block 79

.InitializeInstructions_label_771:

	// *** Basic block 80

.InitializeInstructions_label_772:

	// *** Basic block 81

.InitializeInstructions_label_773:
	lla         t0, .str.21
	sd          t0, -192(s3)
	addi        t0, s3, -192
	la          t1, Assemble_sh
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -192(s3)
	sd          t0, 0(sp)
	ld          t0, -184(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 82

	addi        sp, sp, 16

	// *** Basic block 83

.InitializeInstructions_label_793:

	// *** Basic block 84

.InitializeInstructions_label_794:

	// *** Basic block 85

.InitializeInstructions_label_795:
	lla         t0, .str.22
	sd          t0, -176(s3)
	addi        t0, s3, -176
	la          t1, Assemble_sw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -176(s3)
	sd          t0, 0(sp)
	ld          t0, -168(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 86

	addi        sp, sp, 16

	// *** Basic block 87

.InitializeInstructions_label_815:

	// *** Basic block 88

.InitializeInstructions_label_816:

	// *** Basic block 89

.InitializeInstructions_label_817:
	lla         t0, .str.23
	sd          t0, -160(s3)
	addi        t0, s3, -160
	la          t1, Assemble_addi
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -160(s3)
	sd          t0, 0(sp)
	ld          t0, -152(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 90

	addi        sp, sp, 16

	// *** Basic block 91

.InitializeInstructions_label_837:

	// *** Basic block 92

.InitializeInstructions_label_838:

	// *** Basic block 93

.InitializeInstructions_label_839:
	lla         t0, .str.24
	sd          t0, -144(s3)
	addi        t0, s3, -144
	la          t1, Assemble_slti
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -144(s3)
	sd          t0, 0(sp)
	ld          t0, -136(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 94

	addi        sp, sp, 16

	// *** Basic block 95

.InitializeInstructions_label_859:

	// *** Basic block 96

.InitializeInstructions_label_860:

	// *** Basic block 97

.InitializeInstructions_label_861:
	lla         t0, .str.25
	sd          t0, -128(s3)
	addi        t0, s3, -128
	la          t1, Assemble_sltiu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -128(s3)
	sd          t0, 0(sp)
	ld          t0, -120(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 98

	addi        sp, sp, 16

	// *** Basic block 99

.InitializeInstructions_label_881:

	// *** Basic block 100

.InitializeInstructions_label_882:

	// *** Basic block 101

.InitializeInstructions_label_883:
	lla         t0, .str.26
	sd          t0, -112(s3)
	addi        t0, s3, -112
	la          t1, Assemble_xori
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -112(s3)
	sd          t0, 0(sp)
	ld          t0, -104(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 102

	addi        sp, sp, 16

	// *** Basic block 103

.InitializeInstructions_label_903:

	// *** Basic block 104

.InitializeInstructions_label_904:

	// *** Basic block 105

.InitializeInstructions_label_905:
	lla         t0, .str.27
	sd          t0, -96(s3)
	addi        t0, s3, -96
	la          t1, Assemble_ori
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -96(s3)
	sd          t0, 0(sp)
	ld          t0, -88(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 106

	addi        sp, sp, 16

	// *** Basic block 107

.InitializeInstructions_label_925:

	// *** Basic block 108

.InitializeInstructions_label_926:

	// *** Basic block 109

.InitializeInstructions_label_927:
	lla         t0, .str.28
	sd          t0, -80(s3)
	addi        t0, s3, -80
	la          t1, Assemble_andi
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -80(s3)
	sd          t0, 0(sp)
	ld          t0, -72(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 110

	addi        sp, sp, 16

	// *** Basic block 111

.InitializeInstructions_label_947:

	// *** Basic block 112

.InitializeInstructions_label_948:

	// *** Basic block 113

.InitializeInstructions_label_949:
	lla         t0, .str.29
	sd          t0, -64(s3)
	addi        t0, s3, -64
	la          t1, Assemble_slli
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -64(s3)
	sd          t0, 0(sp)
	ld          t0, -56(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 114

	addi        sp, sp, 16

	// *** Basic block 115

.InitializeInstructions_label_969:

	// *** Basic block 116

.InitializeInstructions_label_970:

	// *** Basic block 117

.InitializeInstructions_label_971:
	lla         t0, .str.30
	sd          t0, -48(s3)
	addi        t0, s3, -48
	la          t1, Assemble_srli
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -48(s3)
	sd          t0, 0(sp)
	ld          t0, -40(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 118

	addi        sp, sp, 16

	// *** Basic block 119

.InitializeInstructions_label_991:

	// *** Basic block 120

.InitializeInstructions_label_992:

	// *** Basic block 121

.InitializeInstructions_label_993:
	lla         t0, .str.31
	sd          t0, -32(s3)
	addi        t0, s3, -32
	la          t1, Assemble_srai
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -32(s3)
	sd          t0, 0(sp)
	ld          t0, -24(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 122

	addi        sp, sp, 16

	// *** Basic block 123

.InitializeInstructions_label_1013:

	// *** Basic block 124

.InitializeInstructions_label_1014:

	// *** Basic block 125

.InitializeInstructions_label_1015:
	lla         t0, .str.32
	sd          t0, -16(s3)
	addi        t0, s3, -16
	la          t1, Assemble_add
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -16(s3)
	sd          t0, 0(sp)
	ld          t0, -8(s3)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 126

	addi        sp, sp, 16

	// *** Basic block 127

.InitializeInstructions_label_1034:

	// *** Basic block 128

.InitializeInstructions_label_1035:

	// *** Basic block 129

.InitializeInstructions_label_1036:
	lla         t0, .str.33
	sd          t0, -2048(s0)
	add         t0, s0, s2
	la          t1, Assemble_sub
	sd          t1, 8(t0)
	add         t0, s0, s2
	addi        sp, sp, -16
	ld          t1, 0(t0)
	sd          t1, 0(sp)
	ld          t0, 8(t0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 130

	addi        sp, sp, 16

	// *** Basic block 131

.InitializeInstructions_label_1057:

	// *** Basic block 132

.InitializeInstructions_label_1058:

	// *** Basic block 133

.InitializeInstructions_label_1059:
	lla         t0, .str.34
	sd          t0, -2032(s0)
	addi        t0, s0, -2032
	la          t1, Assemble_sll
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -2032(s0)
	sd          t0, 0(sp)
	ld          t0, -2024(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 134

	addi        sp, sp, 16

	// *** Basic block 135

.InitializeInstructions_label_1079:

	// *** Basic block 136

.InitializeInstructions_label_1080:

	// *** Basic block 137

.InitializeInstructions_label_1081:
	lla         t0, .str.35
	sd          t0, -2016(s0)
	addi        t0, s0, -2016
	la          t1, Assemble_slt
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -2016(s0)
	sd          t0, 0(sp)
	ld          t0, -2008(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 138

	addi        sp, sp, 16

	// *** Basic block 139

.InitializeInstructions_label_1101:

	// *** Basic block 140

.InitializeInstructions_label_1102:

	// *** Basic block 141

.InitializeInstructions_label_1103:
	lla         t0, .str.36
	sd          t0, -2000(s0)
	addi        t0, s0, -2000
	la          t1, Assemble_sltu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -2000(s0)
	sd          t0, 0(sp)
	ld          t0, -1992(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 142

	addi        sp, sp, 16

	// *** Basic block 143

.InitializeInstructions_label_1123:

	// *** Basic block 144

.InitializeInstructions_label_1124:

	// *** Basic block 145

.InitializeInstructions_label_1125:
	lla         t0, .str.37
	sd          t0, -1984(s0)
	addi        t0, s0, -1984
	la          t1, Assemble_xor
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1984(s0)
	sd          t0, 0(sp)
	ld          t0, -1976(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 146

	addi        sp, sp, 16

	// *** Basic block 147

.InitializeInstructions_label_1145:

	// *** Basic block 148

.InitializeInstructions_label_1146:

	// *** Basic block 149

.InitializeInstructions_label_1147:
	lla         t0, .str.38
	sd          t0, -1968(s0)
	addi        t0, s0, -1968
	la          t1, Assemble_srl
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1968(s0)
	sd          t0, 0(sp)
	ld          t0, -1960(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 150

	addi        sp, sp, 16

	// *** Basic block 151

.InitializeInstructions_label_1167:

	// *** Basic block 152

.InitializeInstructions_label_1168:

	// *** Basic block 153

.InitializeInstructions_label_1169:
	lla         t0, .str.39
	sd          t0, -1952(s0)
	addi        t0, s0, -1952
	la          t1, Assemble_sra
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1952(s0)
	sd          t0, 0(sp)
	ld          t0, -1944(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 154

	addi        sp, sp, 16

	// *** Basic block 155

.InitializeInstructions_label_1189:

	// *** Basic block 156

.InitializeInstructions_label_1190:

	// *** Basic block 157

.InitializeInstructions_label_1191:
	lla         t0, .str.40
	sd          t0, -1936(s0)
	addi        t0, s0, -1936
	la          t1, Assemble_or
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1936(s0)
	sd          t0, 0(sp)
	ld          t0, -1928(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 158

	addi        sp, sp, 16

	// *** Basic block 159

.InitializeInstructions_label_1211:

	// *** Basic block 160

.InitializeInstructions_label_1212:

	// *** Basic block 161

.InitializeInstructions_label_1213:
	lla         t0, .str.41
	sd          t0, -1920(s0)
	addi        t0, s0, -1920
	la          t1, Assemble_and
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1920(s0)
	sd          t0, 0(sp)
	ld          t0, -1912(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 162

	addi        sp, sp, 16

	// *** Basic block 163

.InitializeInstructions_label_1233:

	// *** Basic block 164

.InitializeInstructions_label_1234:

	// *** Basic block 165

.InitializeInstructions_label_1235:
	lla         t0, .str.42
	sd          t0, -1904(s0)
	addi        t0, s0, -1904
	la          t1, Assemble_fence
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1904(s0)
	sd          t0, 0(sp)
	ld          t0, -1896(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 166

	addi        sp, sp, 16

	// *** Basic block 167

.InitializeInstructions_label_1255:

	// *** Basic block 168

.InitializeInstructions_label_1256:

	// *** Basic block 169

.InitializeInstructions_label_1257:
	lla         t0, .str.43
	sd          t0, -1888(s0)
	addi        t0, s0, -1888
	la          t1, Assemble_fence_i
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1888(s0)
	sd          t0, 0(sp)
	ld          t0, -1880(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 170

	addi        sp, sp, 16

	// *** Basic block 171

.InitializeInstructions_label_1277:

	// *** Basic block 172

.InitializeInstructions_label_1278:

	// *** Basic block 173

.InitializeInstructions_label_1279:
	lla         t0, .str.44
	sd          t0, -1872(s0)
	addi        t0, s0, -1872
	la          t1, Assemble_ecall
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1872(s0)
	sd          t0, 0(sp)
	ld          t0, -1864(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 174

	addi        sp, sp, 16

	// *** Basic block 175

.InitializeInstructions_label_1299:

	// *** Basic block 176

.InitializeInstructions_label_1300:

	// *** Basic block 177

.InitializeInstructions_label_1301:
	lla         t0, .str.45
	sd          t0, -1856(s0)
	addi        t0, s0, -1856
	la          t1, Assemble_ebreak
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1856(s0)
	sd          t0, 0(sp)
	ld          t0, -1848(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 178

	addi        sp, sp, 16

	// *** Basic block 179

.InitializeInstructions_label_1321:

	// *** Basic block 180

.InitializeInstructions_label_1322:

	// *** Basic block 181

.InitializeInstructions_label_1323:
	lla         t0, .str.46
	sd          t0, -1840(s0)
	addi        t0, s0, -1840
	la          t1, Assemble_csrrw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1840(s0)
	sd          t0, 0(sp)
	ld          t0, -1832(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 182

	addi        sp, sp, 16

	// *** Basic block 183

.InitializeInstructions_label_1343:

	// *** Basic block 184

.InitializeInstructions_label_1344:

	// *** Basic block 185

.InitializeInstructions_label_1345:
	lla         t0, .str.47
	sd          t0, -1824(s0)
	addi        t0, s0, -1824
	la          t1, Assemble_csrrs
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1824(s0)
	sd          t0, 0(sp)
	ld          t0, -1816(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 186

	addi        sp, sp, 16

	// *** Basic block 187

.InitializeInstructions_label_1365:

	// *** Basic block 188

.InitializeInstructions_label_1366:

	// *** Basic block 189

.InitializeInstructions_label_1367:
	lla         t0, .str.48
	sd          t0, -1808(s0)
	addi        t0, s0, -1808
	la          t1, Assemble_csrrc
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1808(s0)
	sd          t0, 0(sp)
	ld          t0, -1800(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 190

	addi        sp, sp, 16

	// *** Basic block 191

.InitializeInstructions_label_1387:

	// *** Basic block 192

.InitializeInstructions_label_1388:

	// *** Basic block 193

.InitializeInstructions_label_1389:
	lla         t0, .str.49
	sd          t0, -1792(s0)
	addi        t0, s0, -1792
	la          t1, Assemble_csrrwi
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1792(s0)
	sd          t0, 0(sp)
	ld          t0, -1784(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 194

	addi        sp, sp, 16

	// *** Basic block 195

.InitializeInstructions_label_1409:

	// *** Basic block 196

.InitializeInstructions_label_1410:

	// *** Basic block 197

.InitializeInstructions_label_1411:
	lla         t0, .str.50
	sd          t0, -1776(s0)
	addi        t0, s0, -1776
	la          t1, Assemble_csrrsi
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1776(s0)
	sd          t0, 0(sp)
	ld          t0, -1768(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 198

	addi        sp, sp, 16

	// *** Basic block 199

.InitializeInstructions_label_1431:

	// *** Basic block 200

.InitializeInstructions_label_1432:

	// *** Basic block 201

.InitializeInstructions_label_1433:
	lla         t0, .str.51
	sd          t0, -1760(s0)
	addi        t0, s0, -1760
	la          t1, Assemble_csrrci
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1760(s0)
	sd          t0, 0(sp)
	ld          t0, -1752(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 202

	addi        sp, sp, 16

	// *** Basic block 203

.InitializeInstructions_label_1453:

	// *** Basic block 204

.InitializeInstructions_label_1454:

	// *** Basic block 205

.InitializeInstructions_label_1455:
	lla         t0, .str.52
	sd          t0, -1744(s0)
	addi        t0, s0, -1744
	la          t1, Assemble_lwu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1744(s0)
	sd          t0, 0(sp)
	ld          t0, -1736(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 206

	addi        sp, sp, 16

	// *** Basic block 207

.InitializeInstructions_label_1475:

	// *** Basic block 208

.InitializeInstructions_label_1476:

	// *** Basic block 209

.InitializeInstructions_label_1477:
	lla         t0, .str.53
	sd          t0, -1728(s0)
	addi        t0, s0, -1728
	la          t1, Assemble_ld
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1728(s0)
	sd          t0, 0(sp)
	ld          t0, -1720(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 210

	addi        sp, sp, 16

	// *** Basic block 211

.InitializeInstructions_label_1497:

	// *** Basic block 212

.InitializeInstructions_label_1498:

	// *** Basic block 213

.InitializeInstructions_label_1499:
	lla         t0, .str.54
	sd          t0, -1712(s0)
	addi        t0, s0, -1712
	la          t1, Assemble_sd
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1712(s0)
	sd          t0, 0(sp)
	ld          t0, -1704(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 214

	addi        sp, sp, 16

	// *** Basic block 215

.InitializeInstructions_label_1519:

	// *** Basic block 216

.InitializeInstructions_label_1520:

	// *** Basic block 217

.InitializeInstructions_label_1521:
	lla         t0, .str.55
	sd          t0, -1696(s0)
	addi        t0, s0, -1696
	la          t1, Assemble_addiw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1696(s0)
	sd          t0, 0(sp)
	ld          t0, -1688(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 218

	addi        sp, sp, 16

	// *** Basic block 219

.InitializeInstructions_label_1541:

	// *** Basic block 220

.InitializeInstructions_label_1542:

	// *** Basic block 221

.InitializeInstructions_label_1543:
	lla         t0, .str.56
	sd          t0, -1680(s0)
	addi        t0, s0, -1680
	la          t1, Assemble_slliw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1680(s0)
	sd          t0, 0(sp)
	ld          t0, -1672(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 222

	addi        sp, sp, 16

	// *** Basic block 223

.InitializeInstructions_label_1563:

	// *** Basic block 224

.InitializeInstructions_label_1564:

	// *** Basic block 225

.InitializeInstructions_label_1565:
	lla         t0, .str.57
	sd          t0, -1664(s0)
	addi        t0, s0, -1664
	la          t1, Assemble_srliw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1664(s0)
	sd          t0, 0(sp)
	ld          t0, -1656(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 226

	addi        sp, sp, 16

	// *** Basic block 227

.InitializeInstructions_label_1585:

	// *** Basic block 228

.InitializeInstructions_label_1586:

	// *** Basic block 229

.InitializeInstructions_label_1587:
	lla         t0, .str.58
	sd          t0, -1648(s0)
	addi        t0, s0, -1648
	la          t1, Assemble_sraiw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1648(s0)
	sd          t0, 0(sp)
	ld          t0, -1640(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 230

	addi        sp, sp, 16

	// *** Basic block 231

.InitializeInstructions_label_1607:

	// *** Basic block 232

.InitializeInstructions_label_1608:

	// *** Basic block 233

.InitializeInstructions_label_1609:
	lla         t0, .str.59
	sd          t0, -1632(s0)
	addi        t0, s0, -1632
	la          t1, Assemble_addw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1632(s0)
	sd          t0, 0(sp)
	ld          t0, -1624(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 234

	addi        sp, sp, 16

	// *** Basic block 235

.InitializeInstructions_label_1629:

	// *** Basic block 236

.InitializeInstructions_label_1630:

	// *** Basic block 237

.InitializeInstructions_label_1631:
	lla         t0, .str.60
	sd          t0, -1616(s0)
	addi        t0, s0, -1616
	la          t1, Assemble_subw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1616(s0)
	sd          t0, 0(sp)
	ld          t0, -1608(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 238

	addi        sp, sp, 16

	// *** Basic block 239

.InitializeInstructions_label_1651:

	// *** Basic block 240

.InitializeInstructions_label_1652:

	// *** Basic block 241

.InitializeInstructions_label_1653:
	lla         t0, .str.61
	sd          t0, -1600(s0)
	addi        t0, s0, -1600
	la          t1, Assemble_sllw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1600(s0)
	sd          t0, 0(sp)
	ld          t0, -1592(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 242

	addi        sp, sp, 16

	// *** Basic block 243

.InitializeInstructions_label_1673:

	// *** Basic block 244

.InitializeInstructions_label_1674:

	// *** Basic block 245

.InitializeInstructions_label_1675:
	lla         t0, .str.62
	sd          t0, -1584(s0)
	addi        t0, s0, -1584
	la          t1, Assemble_srlw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1584(s0)
	sd          t0, 0(sp)
	ld          t0, -1576(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 246

	addi        sp, sp, 16

	// *** Basic block 247

.InitializeInstructions_label_1695:

	// *** Basic block 248

.InitializeInstructions_label_1696:

	// *** Basic block 249

.InitializeInstructions_label_1697:
	lla         t0, .str.63
	sd          t0, -1568(s0)
	addi        t0, s0, -1568
	la          t1, Assemble_sraw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1568(s0)
	sd          t0, 0(sp)
	ld          t0, -1560(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 250

	addi        sp, sp, 16

	// *** Basic block 251

.InitializeInstructions_label_1717:

	// *** Basic block 252

.InitializeInstructions_label_1718:

	// *** Basic block 253

.InitializeInstructions_label_1719:
	lla         t0, .str.64
	sd          t0, -1552(s0)
	addi        t0, s0, -1552
	la          t1, Assemble_mul
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1552(s0)
	sd          t0, 0(sp)
	ld          t0, -1544(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 254

	addi        sp, sp, 16

	// *** Basic block 255

.InitializeInstructions_label_1739:

	// *** Basic block 256

.InitializeInstructions_label_1740:

	// *** Basic block 257

.InitializeInstructions_label_1741:
	lla         t0, .str.65
	sd          t0, -1536(s0)
	addi        t0, s0, -1536
	la          t1, Assemble_mulh
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1536(s0)
	sd          t0, 0(sp)
	ld          t0, -1528(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 258

	addi        sp, sp, 16

	// *** Basic block 259

.InitializeInstructions_label_1761:

	// *** Basic block 260

.InitializeInstructions_label_1762:

	// *** Basic block 261

.InitializeInstructions_label_1763:
	lla         t0, .str.66
	sd          t0, -1520(s0)
	addi        t0, s0, -1520
	la          t1, Assemble_mulhsu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1520(s0)
	sd          t0, 0(sp)
	ld          t0, -1512(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 262

	addi        sp, sp, 16

	// *** Basic block 263

.InitializeInstructions_label_1783:

	// *** Basic block 264

.InitializeInstructions_label_1784:

	// *** Basic block 265

.InitializeInstructions_label_1785:
	lla         t0, .str.67
	sd          t0, -1504(s0)
	addi        t0, s0, -1504
	la          t1, Assemble_mulhu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1504(s0)
	sd          t0, 0(sp)
	ld          t0, -1496(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 266

	addi        sp, sp, 16

	// *** Basic block 267

.InitializeInstructions_label_1805:

	// *** Basic block 268

.InitializeInstructions_label_1806:

	// *** Basic block 269

.InitializeInstructions_label_1807:
	lla         t0, .str.68
	sd          t0, -1488(s0)
	addi        t0, s0, -1488
	la          t1, Assemble_div
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1488(s0)
	sd          t0, 0(sp)
	ld          t0, -1480(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 270

	addi        sp, sp, 16

	// *** Basic block 271

.InitializeInstructions_label_1827:

	// *** Basic block 272

.InitializeInstructions_label_1828:

	// *** Basic block 273

.InitializeInstructions_label_1829:
	lla         t0, .str.69
	sd          t0, -1472(s0)
	addi        t0, s0, -1472
	la          t1, Assemble_divu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1472(s0)
	sd          t0, 0(sp)
	ld          t0, -1464(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 274

	addi        sp, sp, 16

	// *** Basic block 275

.InitializeInstructions_label_1849:

	// *** Basic block 276

.InitializeInstructions_label_1850:

	// *** Basic block 277

.InitializeInstructions_label_1851:
	lla         t0, .str.70
	sd          t0, -1456(s0)
	addi        t0, s0, -1456
	la          t1, Assemble_rem
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1456(s0)
	sd          t0, 0(sp)
	ld          t0, -1448(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 278

	addi        sp, sp, 16

	// *** Basic block 279

.InitializeInstructions_label_1871:

	// *** Basic block 280

.InitializeInstructions_label_1872:

	// *** Basic block 281

.InitializeInstructions_label_1873:
	lla         t0, .str.71
	sd          t0, -1440(s0)
	addi        t0, s0, -1440
	la          t1, Assemble_remu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1440(s0)
	sd          t0, 0(sp)
	ld          t0, -1432(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 282

	addi        sp, sp, 16

	// *** Basic block 283

.InitializeInstructions_label_1893:

	// *** Basic block 284

.InitializeInstructions_label_1894:

	// *** Basic block 285

.InitializeInstructions_label_1895:
	lla         t0, .str.72
	sd          t0, -1424(s0)
	addi        t0, s0, -1424
	la          t1, Assemble_mulw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1424(s0)
	sd          t0, 0(sp)
	ld          t0, -1416(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 286

	addi        sp, sp, 16

	// *** Basic block 287

.InitializeInstructions_label_1915:

	// *** Basic block 288

.InitializeInstructions_label_1916:

	// *** Basic block 289

.InitializeInstructions_label_1917:
	lla         t0, .str.73
	sd          t0, -1408(s0)
	addi        t0, s0, -1408
	la          t1, Assemble_divw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1408(s0)
	sd          t0, 0(sp)
	ld          t0, -1400(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 290

	addi        sp, sp, 16

	// *** Basic block 291

.InitializeInstructions_label_1937:

	// *** Basic block 292

.InitializeInstructions_label_1938:

	// *** Basic block 293

.InitializeInstructions_label_1939:
	lla         t0, .str.74
	sd          t0, -1392(s0)
	addi        t0, s0, -1392
	la          t1, Assemble_divuw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1392(s0)
	sd          t0, 0(sp)
	ld          t0, -1384(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 294

	addi        sp, sp, 16

	// *** Basic block 295

.InitializeInstructions_label_1959:

	// *** Basic block 296

.InitializeInstructions_label_1960:

	// *** Basic block 297

.InitializeInstructions_label_1961:
	lla         t0, .str.75
	sd          t0, -1376(s0)
	addi        t0, s0, -1376
	la          t1, Assemble_remw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1376(s0)
	sd          t0, 0(sp)
	ld          t0, -1368(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 298

	addi        sp, sp, 16

	// *** Basic block 299

.InitializeInstructions_label_1981:

	// *** Basic block 300

.InitializeInstructions_label_1982:

	// *** Basic block 301

.InitializeInstructions_label_1983:
	lla         t0, .str.76
	sd          t0, -1360(s0)
	addi        t0, s0, -1360
	la          t1, Assemble_remuw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1360(s0)
	sd          t0, 0(sp)
	ld          t0, -1352(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 302

	addi        sp, sp, 16

	// *** Basic block 303

.InitializeInstructions_label_2003:

	// *** Basic block 304

.InitializeInstructions_label_2004:

	// *** Basic block 305

.InitializeInstructions_label_2005:
	lla         t0, .str.77
	sd          t0, -1344(s0)
	addi        t0, s0, -1344
	la          t1, Assemble_flw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1344(s0)
	sd          t0, 0(sp)
	ld          t0, -1336(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 306

	addi        sp, sp, 16

	// *** Basic block 307

.InitializeInstructions_label_2025:

	// *** Basic block 308

.InitializeInstructions_label_2026:

	// *** Basic block 309

.InitializeInstructions_label_2027:
	lla         t0, .str.78
	sd          t0, -1328(s0)
	addi        t0, s0, -1328
	la          t1, Assemble_fsw
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1328(s0)
	sd          t0, 0(sp)
	ld          t0, -1320(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 310

	addi        sp, sp, 16

	// *** Basic block 311

.InitializeInstructions_label_2047:

	// *** Basic block 312

.InitializeInstructions_label_2048:

	// *** Basic block 313

.InitializeInstructions_label_2049:
	lla         t0, .str.79
	sd          t0, -1312(s0)
	addi        t0, s0, -1312
	la          t1, Assemble_fmadd_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1312(s0)
	sd          t0, 0(sp)
	ld          t0, -1304(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 314

	addi        sp, sp, 16

	// *** Basic block 315

.InitializeInstructions_label_2069:

	// *** Basic block 316

.InitializeInstructions_label_2070:

	// *** Basic block 317

.InitializeInstructions_label_2071:
	lla         t0, .str.80
	sd          t0, -1296(s0)
	addi        t0, s0, -1296
	la          t1, Assemble_fmsub_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1296(s0)
	sd          t0, 0(sp)
	ld          t0, -1288(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 318

	addi        sp, sp, 16

	// *** Basic block 319

.InitializeInstructions_label_2091:

	// *** Basic block 320

.InitializeInstructions_label_2092:

	// *** Basic block 321

.InitializeInstructions_label_2093:
	lla         t0, .str.81
	sd          t0, -1280(s0)
	addi        t0, s0, -1280
	la          t1, Assemble_fnmsub_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1280(s0)
	sd          t0, 0(sp)
	ld          t0, -1272(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 322

	addi        sp, sp, 16

	// *** Basic block 323

.InitializeInstructions_label_2113:

	// *** Basic block 324

.InitializeInstructions_label_2114:

	// *** Basic block 325

.InitializeInstructions_label_2115:
	lla         t0, .str.82
	sd          t0, -1264(s0)
	addi        t0, s0, -1264
	la          t1, Assemble_fnmadd_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1264(s0)
	sd          t0, 0(sp)
	ld          t0, -1256(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 326

	addi        sp, sp, 16

	// *** Basic block 327

.InitializeInstructions_label_2135:

	// *** Basic block 328

.InitializeInstructions_label_2136:

	// *** Basic block 329

.InitializeInstructions_label_2137:
	lla         t0, .str.83
	sd          t0, -1248(s0)
	addi        t0, s0, -1248
	la          t1, Assemble_fadd_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1248(s0)
	sd          t0, 0(sp)
	ld          t0, -1240(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 330

	addi        sp, sp, 16

	// *** Basic block 331

.InitializeInstructions_label_2157:

	// *** Basic block 332

.InitializeInstructions_label_2158:

	// *** Basic block 333

.InitializeInstructions_label_2159:
	lla         t0, .str.84
	sd          t0, -1232(s0)
	addi        t0, s0, -1232
	la          t1, Assemble_fsub_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1232(s0)
	sd          t0, 0(sp)
	ld          t0, -1224(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 334

	addi        sp, sp, 16

	// *** Basic block 335

.InitializeInstructions_label_2179:

	// *** Basic block 336

.InitializeInstructions_label_2180:

	// *** Basic block 337

.InitializeInstructions_label_2181:
	lla         t0, .str.85
	sd          t0, -1216(s0)
	addi        t0, s0, -1216
	la          t1, Assemble_fmul_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1216(s0)
	sd          t0, 0(sp)
	ld          t0, -1208(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 338

	addi        sp, sp, 16

	// *** Basic block 339

.InitializeInstructions_label_2201:

	// *** Basic block 340

.InitializeInstructions_label_2202:

	// *** Basic block 341

.InitializeInstructions_label_2203:
	lla         t0, .str.86
	sd          t0, -1200(s0)
	addi        t0, s0, -1200
	la          t1, Assemble_fdiv_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1200(s0)
	sd          t0, 0(sp)
	ld          t0, -1192(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 342

	addi        sp, sp, 16

	// *** Basic block 343

.InitializeInstructions_label_2223:

	// *** Basic block 344

.InitializeInstructions_label_2224:

	// *** Basic block 345

.InitializeInstructions_label_2225:
	lla         t0, .str.87
	sd          t0, -1184(s0)
	addi        t0, s0, -1184
	la          t1, Assemble_fsqrt_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1184(s0)
	sd          t0, 0(sp)
	ld          t0, -1176(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 346

	addi        sp, sp, 16

	// *** Basic block 347

.InitializeInstructions_label_2245:

	// *** Basic block 348

.InitializeInstructions_label_2246:

	// *** Basic block 349

.InitializeInstructions_label_2247:
	lla         t0, .str.88
	sd          t0, -1168(s0)
	addi        t0, s0, -1168
	la          t1, Assemble_fsgnj_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1168(s0)
	sd          t0, 0(sp)
	ld          t0, -1160(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 350

	addi        sp, sp, 16

	// *** Basic block 351

.InitializeInstructions_label_2267:

	// *** Basic block 352

.InitializeInstructions_label_2268:

	// *** Basic block 353

.InitializeInstructions_label_2269:
	lla         t0, .str.89
	sd          t0, -1152(s0)
	addi        t0, s0, -1152
	la          t1, Assemble_fsgnjn_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1152(s0)
	sd          t0, 0(sp)
	ld          t0, -1144(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 354

	addi        sp, sp, 16

	// *** Basic block 355

.InitializeInstructions_label_2289:

	// *** Basic block 356

.InitializeInstructions_label_2290:

	// *** Basic block 357

.InitializeInstructions_label_2291:
	lla         t0, .str.90
	sd          t0, -1136(s0)
	addi        t0, s0, -1136
	la          t1, Assemble_fsgnjx_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1136(s0)
	sd          t0, 0(sp)
	ld          t0, -1128(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 358

	addi        sp, sp, 16

	// *** Basic block 359

.InitializeInstructions_label_2311:

	// *** Basic block 360

.InitializeInstructions_label_2312:

	// *** Basic block 361

.InitializeInstructions_label_2313:
	lla         t0, .str.91
	sd          t0, -1120(s0)
	addi        t0, s0, -1120
	la          t1, Assemble_fmin_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1120(s0)
	sd          t0, 0(sp)
	ld          t0, -1112(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 362

	addi        sp, sp, 16

	// *** Basic block 363

.InitializeInstructions_label_2333:

	// *** Basic block 364

.InitializeInstructions_label_2334:

	// *** Basic block 365

.InitializeInstructions_label_2335:
	lla         t0, .str.92
	sd          t0, -1104(s0)
	addi        t0, s0, -1104
	la          t1, Assemble_fmax_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1104(s0)
	sd          t0, 0(sp)
	ld          t0, -1096(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 366

	addi        sp, sp, 16

	// *** Basic block 367

.InitializeInstructions_label_2355:

	// *** Basic block 368

.InitializeInstructions_label_2356:

	// *** Basic block 369

.InitializeInstructions_label_2357:
	lla         t0, .str.93
	sd          t0, -1088(s0)
	addi        t0, s0, -1088
	la          t1, Assemble_fcvt_w_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1088(s0)
	sd          t0, 0(sp)
	ld          t0, -1080(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 370

	addi        sp, sp, 16

	// *** Basic block 371

.InitializeInstructions_label_2377:

	// *** Basic block 372

.InitializeInstructions_label_2378:

	// *** Basic block 373

.InitializeInstructions_label_2379:
	lla         t0, .str.94
	sd          t0, -1072(s0)
	addi        t0, s0, -1072
	la          t1, Assemble_fcvt_wu_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1072(s0)
	sd          t0, 0(sp)
	ld          t0, -1064(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 374

	addi        sp, sp, 16

	// *** Basic block 375

.InitializeInstructions_label_2399:

	// *** Basic block 376

.InitializeInstructions_label_2400:

	// *** Basic block 377

.InitializeInstructions_label_2401:
	lla         t0, .str.95
	sd          t0, -1056(s0)
	addi        t0, s0, -1056
	la          t1, Assemble_fmv_x_w
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1056(s0)
	sd          t0, 0(sp)
	ld          t0, -1048(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 378

	addi        sp, sp, 16

	// *** Basic block 379

.InitializeInstructions_label_2421:

	// *** Basic block 380

.InitializeInstructions_label_2422:

	// *** Basic block 381

.InitializeInstructions_label_2423:
	lla         t0, .str.96
	sd          t0, -1040(s0)
	addi        t0, s0, -1040
	la          t1, Assemble_feq_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1040(s0)
	sd          t0, 0(sp)
	ld          t0, -1032(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 382

	addi        sp, sp, 16

	// *** Basic block 383

.InitializeInstructions_label_2443:

	// *** Basic block 384

.InitializeInstructions_label_2444:

	// *** Basic block 385

.InitializeInstructions_label_2445:
	lla         t0, .str.97
	sd          t0, -1024(s0)
	addi        t0, s0, -1024
	la          t1, Assemble_flt_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1024(s0)
	sd          t0, 0(sp)
	ld          t0, -1016(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 386

	addi        sp, sp, 16

	// *** Basic block 387

.InitializeInstructions_label_2465:

	// *** Basic block 388

.InitializeInstructions_label_2466:

	// *** Basic block 389

.InitializeInstructions_label_2467:
	lla         t0, .str.98
	sd          t0, -1008(s0)
	addi        t0, s0, -1008
	la          t1, Assemble_fle_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -1008(s0)
	sd          t0, 0(sp)
	ld          t0, -1000(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 390

	addi        sp, sp, 16

	// *** Basic block 391

.InitializeInstructions_label_2487:

	// *** Basic block 392

.InitializeInstructions_label_2488:

	// *** Basic block 393

.InitializeInstructions_label_2489:
	lla         t0, .str.99
	sd          t0, -992(s0)
	addi        t0, s0, -992
	la          t1, Assemble_fclass_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -992(s0)
	sd          t0, 0(sp)
	ld          t0, -984(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 394

	addi        sp, sp, 16

	// *** Basic block 395

.InitializeInstructions_label_2509:

	// *** Basic block 396

.InitializeInstructions_label_2510:

	// *** Basic block 397

.InitializeInstructions_label_2511:
	lla         t0, .str.100
	sd          t0, -976(s0)
	addi        t0, s0, -976
	la          t1, Assemble_fcvt_s_w
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -976(s0)
	sd          t0, 0(sp)
	ld          t0, -968(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 398

	addi        sp, sp, 16

	// *** Basic block 399

.InitializeInstructions_label_2531:

	// *** Basic block 400

.InitializeInstructions_label_2532:

	// *** Basic block 401

.InitializeInstructions_label_2533:
	lla         t0, .str.101
	sd          t0, -960(s0)
	addi        t0, s0, -960
	la          t1, Assemble_fcvt_s_wu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -960(s0)
	sd          t0, 0(sp)
	ld          t0, -952(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 402

	addi        sp, sp, 16

	// *** Basic block 403

.InitializeInstructions_label_2553:

	// *** Basic block 404

.InitializeInstructions_label_2554:

	// *** Basic block 405

.InitializeInstructions_label_2555:
	lla         t0, .str.102
	sd          t0, -944(s0)
	addi        t0, s0, -944
	la          t1, Assemble_fmv_w_x
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -944(s0)
	sd          t0, 0(sp)
	ld          t0, -936(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 406

	addi        sp, sp, 16

	// *** Basic block 407

.InitializeInstructions_label_2575:

	// *** Basic block 408

.InitializeInstructions_label_2576:

	// *** Basic block 409

.InitializeInstructions_label_2577:
	lla         t0, .str.103
	sd          t0, -928(s0)
	addi        t0, s0, -928
	la          t1, Assemble_fcvt_l_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -928(s0)
	sd          t0, 0(sp)
	ld          t0, -920(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 410

	addi        sp, sp, 16

	// *** Basic block 411

.InitializeInstructions_label_2597:

	// *** Basic block 412

.InitializeInstructions_label_2598:

	// *** Basic block 413

.InitializeInstructions_label_2599:
	lla         t0, .str.104
	sd          t0, -912(s0)
	addi        t0, s0, -912
	la          t1, Assemble_fcvt_lu_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -912(s0)
	sd          t0, 0(sp)
	ld          t0, -904(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 414

	addi        sp, sp, 16

	// *** Basic block 415

.InitializeInstructions_label_2619:

	// *** Basic block 416

.InitializeInstructions_label_2620:

	// *** Basic block 417

.InitializeInstructions_label_2621:
	lla         t0, .str.105
	sd          t0, -896(s0)
	addi        t0, s0, -896
	la          t1, Assemble_fcvt_s_l
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -896(s0)
	sd          t0, 0(sp)
	ld          t0, -888(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 418

	addi        sp, sp, 16

	// *** Basic block 419

.InitializeInstructions_label_2641:

	// *** Basic block 420

.InitializeInstructions_label_2642:

	// *** Basic block 421

.InitializeInstructions_label_2643:
	lla         t0, .str.106
	sd          t0, -880(s0)
	addi        t0, s0, -880
	la          t1, Assemble_fcvt_s_lu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -880(s0)
	sd          t0, 0(sp)
	ld          t0, -872(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 422

	addi        sp, sp, 16

	// *** Basic block 423

.InitializeInstructions_label_2663:

	// *** Basic block 424

.InitializeInstructions_label_2664:

	// *** Basic block 425

.InitializeInstructions_label_2665:
	lla         t0, .str.107
	sd          t0, -864(s0)
	addi        t0, s0, -864
	la          t1, Assemble_fld
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -864(s0)
	sd          t0, 0(sp)
	ld          t0, -856(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 426

	addi        sp, sp, 16

	// *** Basic block 427

.InitializeInstructions_label_2685:

	// *** Basic block 428

.InitializeInstructions_label_2686:

	// *** Basic block 429

.InitializeInstructions_label_2687:
	lla         t0, .str.108
	sd          t0, -848(s0)
	addi        t0, s0, -848
	la          t1, Assemble_fsd
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -848(s0)
	sd          t0, 0(sp)
	ld          t0, -840(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 430

	addi        sp, sp, 16

	// *** Basic block 431

.InitializeInstructions_label_2707:

	// *** Basic block 432

.InitializeInstructions_label_2708:

	// *** Basic block 433

.InitializeInstructions_label_2709:
	lla         t0, .str.109
	sd          t0, -832(s0)
	addi        t0, s0, -832
	la          t1, Assemble_fmadd_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -832(s0)
	sd          t0, 0(sp)
	ld          t0, -824(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 434

	addi        sp, sp, 16

	// *** Basic block 435

.InitializeInstructions_label_2729:

	// *** Basic block 436

.InitializeInstructions_label_2730:

	// *** Basic block 437

.InitializeInstructions_label_2731:
	lla         t0, .str.110
	sd          t0, -816(s0)
	addi        t0, s0, -816
	la          t1, Assemble_fmsub_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -816(s0)
	sd          t0, 0(sp)
	ld          t0, -808(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 438

	addi        sp, sp, 16

	// *** Basic block 439

.InitializeInstructions_label_2751:

	// *** Basic block 440

.InitializeInstructions_label_2752:

	// *** Basic block 441

.InitializeInstructions_label_2753:
	lla         t0, .str.111
	sd          t0, -800(s0)
	addi        t0, s0, -800
	la          t1, Assemble_fnmsub_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -800(s0)
	sd          t0, 0(sp)
	ld          t0, -792(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 442

	addi        sp, sp, 16

	// *** Basic block 443

.InitializeInstructions_label_2773:

	// *** Basic block 444

.InitializeInstructions_label_2774:

	// *** Basic block 445

.InitializeInstructions_label_2775:
	lla         t0, .str.112
	sd          t0, -784(s0)
	addi        t0, s0, -784
	la          t1, Assemble_fnmadd_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -784(s0)
	sd          t0, 0(sp)
	ld          t0, -776(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 446

	addi        sp, sp, 16

	// *** Basic block 447

.InitializeInstructions_label_2795:

	// *** Basic block 448

.InitializeInstructions_label_2796:

	// *** Basic block 449

.InitializeInstructions_label_2797:
	lla         t0, .str.113
	sd          t0, -768(s0)
	addi        t0, s0, -768
	la          t1, Assemble_fadd_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -768(s0)
	sd          t0, 0(sp)
	ld          t0, -760(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 450

	addi        sp, sp, 16

	// *** Basic block 451

.InitializeInstructions_label_2817:

	// *** Basic block 452

.InitializeInstructions_label_2818:

	// *** Basic block 453

.InitializeInstructions_label_2819:
	lla         t0, .str.114
	sd          t0, -752(s0)
	addi        t0, s0, -752
	la          t1, Assemble_fsub_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -752(s0)
	sd          t0, 0(sp)
	ld          t0, -744(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 454

	addi        sp, sp, 16

	// *** Basic block 455

.InitializeInstructions_label_2839:

	// *** Basic block 456

.InitializeInstructions_label_2840:

	// *** Basic block 457

.InitializeInstructions_label_2841:
	lla         t0, .str.115
	sd          t0, -736(s0)
	addi        t0, s0, -736
	la          t1, Assemble_fmul_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -736(s0)
	sd          t0, 0(sp)
	ld          t0, -728(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 458

	addi        sp, sp, 16

	// *** Basic block 459

.InitializeInstructions_label_2861:

	// *** Basic block 460

.InitializeInstructions_label_2862:

	// *** Basic block 461

.InitializeInstructions_label_2863:
	lla         t0, .str.116
	sd          t0, -720(s0)
	addi        t0, s0, -720
	la          t1, Assemble_fdiv_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -720(s0)
	sd          t0, 0(sp)
	ld          t0, -712(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 462

	addi        sp, sp, 16

	// *** Basic block 463

.InitializeInstructions_label_2883:

	// *** Basic block 464

.InitializeInstructions_label_2884:

	// *** Basic block 465

.InitializeInstructions_label_2885:
	lla         t0, .str.117
	sd          t0, -704(s0)
	addi        t0, s0, -704
	la          t1, Assemble_fsqrt_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -704(s0)
	sd          t0, 0(sp)
	ld          t0, -696(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 466

	addi        sp, sp, 16

	// *** Basic block 467

.InitializeInstructions_label_2905:

	// *** Basic block 468

.InitializeInstructions_label_2906:

	// *** Basic block 469

.InitializeInstructions_label_2907:
	lla         t0, .str.118
	sd          t0, -688(s0)
	addi        t0, s0, -688
	la          t1, Assemble_fsgnj_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -688(s0)
	sd          t0, 0(sp)
	ld          t0, -680(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 470

	addi        sp, sp, 16

	// *** Basic block 471

.InitializeInstructions_label_2927:

	// *** Basic block 472

.InitializeInstructions_label_2928:

	// *** Basic block 473

.InitializeInstructions_label_2929:
	lla         t0, .str.119
	sd          t0, -672(s0)
	addi        t0, s0, -672
	la          t1, Assemble_fsgnjn_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -672(s0)
	sd          t0, 0(sp)
	ld          t0, -664(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 474

	addi        sp, sp, 16

	// *** Basic block 475

.InitializeInstructions_label_2949:

	// *** Basic block 476

.InitializeInstructions_label_2950:

	// *** Basic block 477

.InitializeInstructions_label_2951:
	lla         t0, .str.120
	sd          t0, -656(s0)
	addi        t0, s0, -656
	la          t1, Assemble_fsgnjx_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -656(s0)
	sd          t0, 0(sp)
	ld          t0, -648(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 478

	addi        sp, sp, 16

	// *** Basic block 479

.InitializeInstructions_label_2971:

	// *** Basic block 480

.InitializeInstructions_label_2972:

	// *** Basic block 481

.InitializeInstructions_label_2973:
	lla         t0, .str.121
	sd          t0, -640(s0)
	addi        t0, s0, -640
	la          t1, Assemble_fmin_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -640(s0)
	sd          t0, 0(sp)
	ld          t0, -632(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 482

	addi        sp, sp, 16

	// *** Basic block 483

.InitializeInstructions_label_2993:

	// *** Basic block 484

.InitializeInstructions_label_2994:

	// *** Basic block 485

.InitializeInstructions_label_2995:
	lla         t0, .str.122
	sd          t0, -624(s0)
	addi        t0, s0, -624
	la          t1, Assemble_fmax_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -624(s0)
	sd          t0, 0(sp)
	ld          t0, -616(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 486

	addi        sp, sp, 16

	// *** Basic block 487

.InitializeInstructions_label_3015:

	// *** Basic block 488

.InitializeInstructions_label_3016:

	// *** Basic block 489

.InitializeInstructions_label_3017:
	lla         t0, .str.123
	sd          t0, -608(s0)
	addi        t0, s0, -608
	la          t1, Assemble_fcvt_s_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -608(s0)
	sd          t0, 0(sp)
	ld          t0, -600(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 490

	addi        sp, sp, 16

	// *** Basic block 491

.InitializeInstructions_label_3037:

	// *** Basic block 492

.InitializeInstructions_label_3038:

	// *** Basic block 493

.InitializeInstructions_label_3039:
	lla         t0, .str.124
	sd          t0, -592(s0)
	addi        t0, s0, -592
	la          t1, Assemble_fcvt_d_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -592(s0)
	sd          t0, 0(sp)
	ld          t0, -584(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 494

	addi        sp, sp, 16

	// *** Basic block 495

.InitializeInstructions_label_3059:

	// *** Basic block 496

.InitializeInstructions_label_3060:

	// *** Basic block 497

.InitializeInstructions_label_3061:
	lla         t0, .str.125
	sd          t0, -576(s0)
	addi        t0, s0, -576
	la          t1, Assemble_feq_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -576(s0)
	sd          t0, 0(sp)
	ld          t0, -568(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 498

	addi        sp, sp, 16

	// *** Basic block 499

.InitializeInstructions_label_3081:

	// *** Basic block 500

.InitializeInstructions_label_3082:

	// *** Basic block 501

.InitializeInstructions_label_3083:
	lla         t0, .str.126
	sd          t0, -560(s0)
	addi        t0, s0, -560
	la          t1, Assemble_flt_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -560(s0)
	sd          t0, 0(sp)
	ld          t0, -552(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 502

	addi        sp, sp, 16

	// *** Basic block 503

.InitializeInstructions_label_3103:

	// *** Basic block 504

.InitializeInstructions_label_3104:

	// *** Basic block 505

.InitializeInstructions_label_3105:
	lla         t0, .str.127
	sd          t0, -544(s0)
	addi        t0, s0, -544
	la          t1, Assemble_fle_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -544(s0)
	sd          t0, 0(sp)
	ld          t0, -536(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 506

	addi        sp, sp, 16

	// *** Basic block 507

.InitializeInstructions_label_3125:

	// *** Basic block 508

.InitializeInstructions_label_3126:

	// *** Basic block 509

.InitializeInstructions_label_3127:
	lla         t0, .str.128
	sd          t0, -528(s0)
	addi        t0, s0, -528
	la          t1, Assemble_fclass_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -528(s0)
	sd          t0, 0(sp)
	ld          t0, -520(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 510

	addi        sp, sp, 16

	// *** Basic block 511

.InitializeInstructions_label_3147:

	// *** Basic block 512

.InitializeInstructions_label_3148:

	// *** Basic block 513

.InitializeInstructions_label_3149:
	lla         t0, .str.129
	sd          t0, -512(s0)
	addi        t0, s0, -512
	la          t1, Assemble_fcvt_w_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -512(s0)
	sd          t0, 0(sp)
	ld          t0, -504(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 514

	addi        sp, sp, 16

	// *** Basic block 515

.InitializeInstructions_label_3168:

	// *** Basic block 516

.InitializeInstructions_label_3169:

	// *** Basic block 517

.InitializeInstructions_label_3170:
	lla         t0, .str.130
	sd          t0, -496(s0)
	addi        t0, s0, -496
	la          t1, Assemble_fcvt_wu_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -496(s0)
	sd          t0, 0(sp)
	ld          t0, -488(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 518

	addi        sp, sp, 16

	// *** Basic block 519

.InitializeInstructions_label_3189:

	// *** Basic block 520

.InitializeInstructions_label_3190:

	// *** Basic block 521

.InitializeInstructions_label_3191:
	lla         t0, .str.131
	sd          t0, -480(s0)
	addi        t0, s0, -480
	la          t1, Assemble_fcvt_d_w
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -480(s0)
	sd          t0, 0(sp)
	ld          t0, -472(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 522

	addi        sp, sp, 16

	// *** Basic block 523

.InitializeInstructions_label_3210:

	// *** Basic block 524

.InitializeInstructions_label_3211:

	// *** Basic block 525

.InitializeInstructions_label_3212:
	lla         t0, .str.132
	sd          t0, -464(s0)
	addi        t0, s0, -464
	la          t1, Assemble_fcvt_d_wu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -464(s0)
	sd          t0, 0(sp)
	ld          t0, -456(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 526

	addi        sp, sp, 16

	// *** Basic block 527

.InitializeInstructions_label_3231:

	// *** Basic block 528

.InitializeInstructions_label_3232:

	// *** Basic block 529

.InitializeInstructions_label_3233:
	lla         t0, .str.133
	sd          t0, -448(s0)
	addi        t0, s0, -448
	la          t1, Assemble_fcvt_l_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -448(s0)
	sd          t0, 0(sp)
	ld          t0, -440(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 530

	addi        sp, sp, 16

	// *** Basic block 531

.InitializeInstructions_label_3252:

	// *** Basic block 532

.InitializeInstructions_label_3253:

	// *** Basic block 533

.InitializeInstructions_label_3254:
	lla         t0, .str.134
	sd          t0, -432(s0)
	addi        t0, s0, -432
	la          t1, Assemble_fcvt_lu_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -432(s0)
	sd          t0, 0(sp)
	ld          t0, -424(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 534

	addi        sp, sp, 16

	// *** Basic block 535

.InitializeInstructions_label_3273:

	// *** Basic block 536

.InitializeInstructions_label_3274:

	// *** Basic block 537

.InitializeInstructions_label_3275:
	lla         t0, .str.135
	sd          t0, -416(s0)
	addi        t0, s0, -416
	la          t1, Assemble_fmv_x_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -416(s0)
	sd          t0, 0(sp)
	ld          t0, -408(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 538

	addi        sp, sp, 16

	// *** Basic block 539

.InitializeInstructions_label_3294:

	// *** Basic block 540

.InitializeInstructions_label_3295:

	// *** Basic block 541

.InitializeInstructions_label_3296:
	lla         t0, .str.136
	sd          t0, -400(s0)
	addi        t0, s0, -400
	la          t1, Assemble_fcvt_d_l
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -400(s0)
	sd          t0, 0(sp)
	ld          t0, -392(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 542

	addi        sp, sp, 16

	// *** Basic block 543

.InitializeInstructions_label_3315:

	// *** Basic block 544

.InitializeInstructions_label_3316:

	// *** Basic block 545

.InitializeInstructions_label_3317:
	lla         t0, .str.137
	sd          t0, -384(s0)
	addi        t0, s0, -384
	la          t1, Assemble_fcvt_d_lu
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -384(s0)
	sd          t0, 0(sp)
	ld          t0, -376(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 546

	addi        sp, sp, 16

	// *** Basic block 547

.InitializeInstructions_label_3336:

	// *** Basic block 548

.InitializeInstructions_label_3337:

	// *** Basic block 549

.InitializeInstructions_label_3338:
	lla         t0, .str.138
	sd          t0, -368(s0)
	addi        t0, s0, -368
	la          t1, Assemble_fmv_d_x
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -368(s0)
	sd          t0, 0(sp)
	ld          t0, -360(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 550

	addi        sp, sp, 16

	// *** Basic block 551

.InitializeInstructions_label_3357:

	// *** Basic block 552

.InitializeInstructions_label_3358:

	// *** Basic block 553

.InitializeInstructions_label_3359:
	lla         t0, .str.139
	sd          t0, -352(s0)
	addi        t0, s0, -352
	la          t1, Assemble_nop
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -352(s0)
	sd          t0, 0(sp)
	ld          t0, -344(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 554

	addi        sp, sp, 16

	// *** Basic block 555

.InitializeInstructions_label_3378:

	// *** Basic block 556

.InitializeInstructions_label_3379:

	// *** Basic block 557

.InitializeInstructions_label_3380:
	lla         t0, .str.140
	sd          t0, -336(s0)
	addi        t0, s0, -336
	la          t1, Assemble_not
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -336(s0)
	sd          t0, 0(sp)
	ld          t0, -328(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 558

	addi        sp, sp, 16

	// *** Basic block 559

.InitializeInstructions_label_3399:

	// *** Basic block 560

.InitializeInstructions_label_3400:

	// *** Basic block 561

.InitializeInstructions_label_3401:
	lla         t0, .str.141
	sd          t0, -320(s0)
	addi        t0, s0, -320
	la          t1, Assemble_neg
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -320(s0)
	sd          t0, 0(sp)
	ld          t0, -312(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 562

	addi        sp, sp, 16

	// *** Basic block 563

.InitializeInstructions_label_3420:

	// *** Basic block 564

.InitializeInstructions_label_3421:

	// *** Basic block 565

.InitializeInstructions_label_3422:
	lla         t0, .str.142
	sd          t0, -304(s0)
	addi        t0, s0, -304
	la          t1, Assemble_fneg_s
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -304(s0)
	sd          t0, 0(sp)
	ld          t0, -296(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 566

	addi        sp, sp, 16

	// *** Basic block 567

.InitializeInstructions_label_3441:

	// *** Basic block 568

.InitializeInstructions_label_3442:

	// *** Basic block 569

.InitializeInstructions_label_3443:
	lla         t0, .str.143
	sd          t0, -288(s0)
	addi        t0, s0, -288
	la          t1, Assemble_fneg_d
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -288(s0)
	sd          t0, 0(sp)
	ld          t0, -280(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 570

	addi        sp, sp, 16

	// *** Basic block 571

.InitializeInstructions_label_3462:

	// *** Basic block 572

.InitializeInstructions_label_3463:

	// *** Basic block 573

.InitializeInstructions_label_3464:
	lla         t0, .str.144
	sd          t0, -272(s0)
	addi        t0, s0, -272
	la          t1, Assemble_li
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -272(s0)
	sd          t0, 0(sp)
	ld          t0, -264(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 574

	addi        sp, sp, 16

	// *** Basic block 575

.InitializeInstructions_label_3483:

	// *** Basic block 576

.InitializeInstructions_label_3484:

	// *** Basic block 577

.InitializeInstructions_label_3485:
	lla         t0, .str.145
	sd          t0, -256(s0)
	addi        t0, s0, -256
	la          t1, Assemble_la
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -256(s0)
	sd          t0, 0(sp)
	ld          t0, -248(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 578

	addi        sp, sp, 16

	// *** Basic block 579

.InitializeInstructions_label_3504:

	// *** Basic block 580

.InitializeInstructions_label_3505:

	// *** Basic block 581

.InitializeInstructions_label_3506:
	lla         t0, .str.146
	sd          t0, -240(s0)
	addi        t0, s0, -240
	la          t1, Assemble_lla
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -240(s0)
	sd          t0, 0(sp)
	ld          t0, -232(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 582

	addi        sp, sp, 16

	// *** Basic block 583

.InitializeInstructions_label_3525:

	// *** Basic block 584

.InitializeInstructions_label_3526:

	// *** Basic block 585

.InitializeInstructions_label_3527:
	lla         t0, .str.147
	sd          t0, -224(s0)
	addi        t0, s0, -224
	la          t1, Assemble_sext_w
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -224(s0)
	sd          t0, 0(sp)
	ld          t0, -216(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 586

	addi        sp, sp, 16

	// *** Basic block 587

.InitializeInstructions_label_3546:

	// *** Basic block 588

.InitializeInstructions_label_3547:

	// *** Basic block 589

.InitializeInstructions_label_3548:
	lla         t0, .str.148
	sd          t0, -208(s0)
	addi        t0, s0, -208
	la          t1, Assemble_seqz
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -208(s0)
	sd          t0, 0(sp)
	ld          t0, -200(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 590

	addi        sp, sp, 16

	// *** Basic block 591

.InitializeInstructions_label_3567:

	// *** Basic block 592

.InitializeInstructions_label_3568:

	// *** Basic block 593

.InitializeInstructions_label_3569:
	lla         t0, .str.149
	sd          t0, -192(s0)
	addi        t0, s0, -192
	la          t1, Assemble_snez
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -192(s0)
	sd          t0, 0(sp)
	ld          t0, -184(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 594

	addi        sp, sp, 16

	// *** Basic block 595

.InitializeInstructions_label_3588:

	// *** Basic block 596

.InitializeInstructions_label_3589:

	// *** Basic block 597

.InitializeInstructions_label_3590:
	lla         t0, .str.150
	sd          t0, -176(s0)
	addi        t0, s0, -176
	la          t1, Assemble_sltz
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -176(s0)
	sd          t0, 0(sp)
	ld          t0, -168(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 598

	addi        sp, sp, 16

	// *** Basic block 599

.InitializeInstructions_label_3609:

	// *** Basic block 600

.InitializeInstructions_label_3610:

	// *** Basic block 601

.InitializeInstructions_label_3611:
	lla         t0, .str.151
	sd          t0, -160(s0)
	addi        t0, s0, -160
	la          t1, Assemble_sgtz
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -160(s0)
	sd          t0, 0(sp)
	ld          t0, -152(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 602

	addi        sp, sp, 16

	// *** Basic block 603

.InitializeInstructions_label_3630:

	// *** Basic block 604

.InitializeInstructions_label_3631:

	// *** Basic block 605

.InitializeInstructions_label_3632:
	lla         t0, .str.152
	sd          t0, -144(s0)
	addi        t0, s0, -144
	la          t1, Assemble_beqz
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -144(s0)
	sd          t0, 0(sp)
	ld          t0, -136(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 606

	addi        sp, sp, 16

	// *** Basic block 607

.InitializeInstructions_label_3651:

	// *** Basic block 608

.InitializeInstructions_label_3652:

	// *** Basic block 609

.InitializeInstructions_label_3653:
	lla         t0, .str.153
	sd          t0, -128(s0)
	addi        t0, s0, -128
	la          t1, Assemble_bnez
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -128(s0)
	sd          t0, 0(sp)
	ld          t0, -120(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 610

	addi        sp, sp, 16

	// *** Basic block 611

.InitializeInstructions_label_3672:

	// *** Basic block 612

.InitializeInstructions_label_3673:

	// *** Basic block 613

.InitializeInstructions_label_3674:
	lla         t0, .str.154
	sd          t0, -112(s0)
	addi        t0, s0, -112
	la          t1, Assemble_j
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -112(s0)
	sd          t0, 0(sp)
	ld          t0, -104(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 614

	addi        sp, sp, 16

	// *** Basic block 615

.InitializeInstructions_label_3693:

	// *** Basic block 616

.InitializeInstructions_label_3694:

	// *** Basic block 617

.InitializeInstructions_label_3695:
	lla         t0, .str.155
	sd          t0, -96(s0)
	addi        t0, s0, -96
	la          t1, Assemble_jr
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -96(s0)
	sd          t0, 0(sp)
	ld          t0, -88(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 618

	addi        sp, sp, 16

	// *** Basic block 619

.InitializeInstructions_label_3714:

	// *** Basic block 620

.InitializeInstructions_label_3715:

	// *** Basic block 621

.InitializeInstructions_label_3716:
	lla         t0, .str.156
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

	// *** Basic block 622

	addi        sp, sp, 16

	// *** Basic block 623

.InitializeInstructions_label_3735:

	// *** Basic block 624

.InitializeInstructions_label_3736:

	// *** Basic block 625

.InitializeInstructions_label_3737:
	lla         t0, .str.157
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

	// *** Basic block 626

	addi        sp, sp, 16

	// *** Basic block 627

.InitializeInstructions_label_3756:

	// *** Basic block 628

.InitializeInstructions_label_3757:

	// *** Basic block 629

.InitializeInstructions_label_3758:
	lla         t0, .str.158
	sd          t0, -48(s0)
	addi        t0, s0, -48
	la          t1, Assemble_callf
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -48(s0)
	sd          t0, 0(sp)
	ld          t0, -40(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 630

	addi        sp, sp, 16

	// *** Basic block 631

.InitializeInstructions_label_3777:

	// *** Basic block 632

.InitializeInstructions_label_3778:

	// *** Basic block 633

.InitializeInstructions_label_3779:
	lla         t0, .str.159
	sd          t0, -32(s0)
	addi        t0, s0, -32
	la          t1, Assemble_rcallf
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -32(s0)
	sd          t0, 0(sp)
	ld          t0, -24(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s1
	call        MapInsert

	// *** Basic block 635

.InitializeInstructions_label_3798:

	// *** Basic block 636

.InitializeInstructions_label_3799:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InitializeInstructions:
	.size InitializeInstructions, .func_end_InitializeInstructions-InitializeInstructions

	.global RVAssemblerInit
	.type RVAssemblerInit, @function

RVAssemblerInit:

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
	li          a2, 4		// 0x4 ASCII \x4
	li          a1, 243		// 0xf3 ASCII \xf3
	call        AssemblerInit

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .RVAssemblerInit_label_52

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.RVAssemblerInit_label_49:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.RVAssemblerInit_label_52:
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

	lla         a0, .str.160
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
	sw          x0, 908(s1)
	li          a0, 1		// 0x1 ASCII \x1
	j           .RVAssemblerInit_label_49
.func_end_RVAssemblerInit:
	.size RVAssemblerInit, .func_end_RVAssemblerInit-RVAssemblerInit

	.global NewRVAssembler
	.type NewRVAssembler, @function

NewRVAssembler:

	// *** Basic block 0

	.global malloc
	.global RVAssemblerInit
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
	call        RVAssemblerInit

	// *** Basic block 2

	mv          a0, s3

	// *** Basic block 3

.NewRVAssembler_label_26:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewRVAssembler:
	.size NewRVAssembler, .func_end_NewRVAssembler-NewRVAssembler

	.global RVAssemblerDestruct
	.type RVAssemblerDestruct, @function

RVAssemblerDestruct:

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
.func_end_RVAssemblerDestruct:
	.size RVAssemblerDestruct, .func_end_RVAssemblerDestruct-RVAssemblerDestruct

	.global RVAssemblerDelete
	.type RVAssemblerDelete, @function

RVAssemblerDelete:

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
	.global RVAssemblerDestruct
	.global free
	mv          s1, a0
	call        RVAssemblerDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_RVAssemblerDelete:
	.size RVAssemblerDelete, .func_end_RVAssemblerDelete-RVAssemblerDelete

	.global AssembleRVInstruction
	.type AssembleRVInstruction, @function

AssembleRVInstruction:

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
	beq         s3, x0, .AssembleRVInstruction_label_35

	// *** Basic block 2

	mv          s4, s3
	mv          a0, s1
	jalr         x1, s4, 0

	// *** Basic block 3

	j           .AssembleRVInstruction_label_46

	// *** Basic block 4

.AssembleRVInstruction_label_35:
	lla         a1, .str.161
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

.AssembleRVInstruction_label_46:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AssembleRVInstruction:
	.size AssembleRVInstruction, .func_end_AssembleRVInstruction-AssembleRVInstruction

	.local  ExtractRegNumber
	.type ExtractRegNumber, @function

ExtractRegNumber:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, x0
	ld          t2, 16(a0)
	add         t3, t2, t0
	lb          t3, 0(t3)
	beqz        t3, .ExtractRegNumber_label_41

	// *** Basic block 1

.ExtractRegNumber_label_24:
	slli        t3, t1, 1
	slli        t4, t1, 3
	add         t3, t3, t4
	mv          t4, t0
	addi        t0, t0, 1
	add         t5, t2, t4
	lb          t5, 0(t5)
	add         t3, t3, t5
	addi        t1, t3, -48
	add         t2, t2, t0
	lb          t2, 0(t2)
	bnez        t2, .ExtractRegNumber_label_24

	// *** Basic block 2

.ExtractRegNumber_label_41:
	mv          a0, t1

	// *** Basic block 3

.ExtractRegNumber_label_44:
	ret         
.func_end_ExtractRegNumber:
	.size ExtractRegNumber, .func_end_ExtractRegNumber-ExtractRegNumber

	.local  RegNumber
	.type RegNumber, @function

RegNumber:

	// *** Basic block 0

	.local known_reg_names
	.global StringEqual
	.local prefixed_reg_names
	.global strncmp
	.local ExtractRegNumber
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
	mv          s2, a1
	mv          s3, a2
	mv          s4, x0
	la          t0, known_reg_names
	ld          t0, 0(t0)
	beq         t0, x0, .RegNumber_label_67

	// *** Basic block 1

	sw          x0, 0(s3)
	ld          s5, 16(s1)

	// *** Basic block 2

.RegNumber_label_38:
	slli        t0, s4, 4
	la          t1, known_reg_names
	add         s6, t1, t0
	ld          a1, 0(s6)
	mv          a0, s1
	call        StringEqual

	// *** Basic block 3

	beqz        a0, .RegNumber_label_57

	// *** Basic block 4

	lw          t0, 8(s6)
	sw          t0, 0(s2)
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 5

.RegNumber_label_54:
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

	// *** Basic block 6

.RegNumber_label_57:

	// *** Basic block 7

.RegNumber_label_58:
	addi        s4, s4, 1
	slli        t0, s4, 4
	la          t1, known_reg_names
	add         t0, t1, t0
	ld          t0, 0(t0)
	beq         t0, x0, .RegNumber_label_38

	// *** Basic block 8

.RegNumber_label_67:
	mv          s5, x0
	mv          s6, x0
	mv          s7, x0
	la          t0, prefixed_reg_names
	ld          t0, 0(t0)
	beq         t0, x0, .RegNumber_label_144

	// *** Basic block 9

.RegNumber_label_80:
	slli        t0, s7, 5
	la          t1, prefixed_reg_names
	add         s8, t1, t0
	ld          a0, 0(s8)
	lw          s5, 8(s8)
	mv          a2, s5
	mv          a1, s5
	call        strncmp

	// *** Basic block 10

	bnez        a0, .RegNumber_label_134

	// *** Basic block 11

	beq         s5, s5, .RegNumber_label_106

	// *** Basic block 12

	mv          a1, s5
	mv          a0, s1
	call        ExtractRegNumber

	// *** Basic block 13

	mv          s6, a0

	// *** Basic block 14

.RegNumber_label_106:
	lw          t0, 20(s8)
	lw          t1, 16(s8)
	sub         t0, t0, t1
	addi        s5, t0, 1
	lw          t2, 12(s8)
	slt         t3, s6, t2
	not         t0, t3
	blt         s6, t2, .RegNumber_label_122

	// *** Basic block 15

	sub         t3, s6, t2
	slt         t0, t3, s5

	// *** Basic block 16

.RegNumber_label_122:
	beqz        t0, .RegNumber_label_133

	// *** Basic block 17

	add         t0, t1, s6
	sub         t0, t0, t2
	sw          t0, 0(s2)
	lw          t0, 24(s8)
	sw          t0, 0(s3)
	li          a0, 1		// 0x1 ASCII \x1
	j           .RegNumber_label_54

	// *** Basic block 18

.RegNumber_label_133:

	// *** Basic block 19

.RegNumber_label_134:

	// *** Basic block 20

.RegNumber_label_135:
	addi        s7, s7, 1
	slli        t0, s7, 5
	la          t1, prefixed_reg_names
	add         t0, t1, t0
	ld          t0, 0(t0)
	beq         t0, x0, .RegNumber_label_80

	// *** Basic block 21

.RegNumber_label_144:
	mv          a0, x0
	j           .RegNumber_label_54
.func_end_RegNumber:
	.size RegNumber, .func_end_RegNumber-RegNumber

	.local  RegisterName
	.type RegisterName, @function

RegisterName:

	// *** Basic block 0

	.global LexLookingAt
	.global StringInit
	.global LexNextToken
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

	beqz        a0, .RegisterName_label_57

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
	mv          a2, s3
	mv          a1, s2
	call        RegNumber

	// *** Basic block 5

	mv          s4, a0
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 6

	mv          a0, s4

	// *** Basic block 7

.RegisterName_label_54:
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

.RegisterName_label_57:
	mv          a0, x0
	j           .RegisterName_label_54
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
	// End of stack frame
	mv          s1, a0
	mv          s2, a2
	mv          s3, a1
	addi        a1, s0, -32
	addi        a2, s0, -28
	call        RegisterName

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .Register_label_44

	// *** Basic block 2

	lla         a1, .str.178
	mv          a2, s2
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 3

	mv          a0, x0

	// *** Basic block 4

.Register_label_41:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.Register_label_44:
	lw          t0, -28(s0)
	beq         t0, s3, .Register_label_74

	// *** Basic block 6

	bnez        t0, .Register_label_55

	// *** Basic block 7

	lla         s3, .str.179
	j           .Register_label_58

	// *** Basic block 8

.Register_label_55:
	lla         s3, .str.180

	// *** Basic block 9

.Register_label_58:
	lla         a1, .str.181
	mv          a3, s2
	mv          a2, s3
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 10

	mv          a0, x0
	j           .Register_label_41

	// *** Basic block 11

.Register_label_74:
	lw          a0, -32(s0)
	j           .Register_label_41
.func_end_Register:
	.size Register, .func_end_Register-Register

	.local  OptionalRegister
	.type OptionalRegister, @function

OptionalRegister:

	// *** Basic block 0

	.global LexLookingAt
	.global StringInit
	.local RegNumber
	.global LexNextToken
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
	addi        a0, s1, 176
	li          a1, 3		// 0x3 ASCII \x3
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .OptionalRegister_label_65

	// *** Basic block 2

	addi        a0, s0, -64
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 3

	addi        a0, s0, -64
	addi        a1, s0, -24
	addi        a2, s0, -20
	call        RegNumber

	// *** Basic block 4

	mv          s2, a0
	beqz        s2, .OptionalRegister_label_49

	// *** Basic block 5

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 6

.OptionalRegister_label_49:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 7

	beqz        s2, .OptionalRegister_label_58

	// *** Basic block 8

	lw          a0, -24(s0)
	j           .OptionalRegister_label_60

	// *** Basic block 9

.OptionalRegister_label_58:
	li          a0, -1		// 0xffffffffffffffff

	// *** Basic block 10

.OptionalRegister_label_60:

	// *** Basic block 11

.OptionalRegister_label_62:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 12

.OptionalRegister_label_65:
	li          a0, -1		// 0xffffffffffffffff
	j           .OptionalRegister_label_62
.func_end_OptionalRegister:
	.size OptionalRegister, .func_end_OptionalRegister-OptionalRegister

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

	lla         a1, .str.182
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

	lla         a1, .str.183
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
	beqz        t0, .ParseRegisterPair_label_51

	// *** Basic block 3

	lla         a1, .str.184
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 4

	mv          a0, x0

	// *** Basic block 5

.ParseRegisterPair_label_48:
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

.ParseRegisterPair_label_51:
	mv          a2, s4
	mv          a1, s3
	mv          a0, s2
	call        Register

	// *** Basic block 7

	sw          a0, 4(s1)
	li          a0, 1		// 0x1 ASCII \x1
	j           .ParseRegisterPair_label_48
.func_end_ParseRegisterPair:
	.size ParseRegisterPair, .func_end_ParseRegisterPair-ParseRegisterPair

	.local  RTypeInstruction
	.type RTypeInstruction, @function

RTypeInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	slli        t0, a5, 25
	slli        t1, a3, 20
	or          t0, t0, t1
	slli        t1, a2, 15
	or          t0, t0, t1
	slli        t1, a4, 12
	or          t0, t0, t1
	slli        t1, a1, 7
	or          t0, t0, t1
	or          a0, t0, a0

	// *** Basic block 1

.RTypeInstruction_label_36:
	ret         
.func_end_RTypeInstruction:
	.size RTypeInstruction, .func_end_RTypeInstruction-RTypeInstruction

	.local  ITypeInstruction
	.type ITypeInstruction, @function

ITypeInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	slli        t0, a4, 20
	slli        t1, a2, 15
	or          t0, t0, t1
	slli        t1, a3, 12
	or          t0, t0, t1
	slli        t1, a1, 7
	or          t0, t0, t1
	or          a0, t0, a0

	// *** Basic block 1

.ITypeInstruction_label_30:
	ret         
.func_end_ITypeInstruction:
	.size ITypeInstruction, .func_end_ITypeInstruction-ITypeInstruction

	.local  STypeInstruction
	.type STypeInstruction, @function

STypeInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	srai        t0, a4, 5
	slli        t0, t0, 25
	slli        t1, a2, 20
	or          t0, t0, t1
	slli        t1, a1, 15
	or          t0, t0, t1
	slli        t1, a3, 12
	or          t0, t0, t1
	andi        t1, a4, 31
	slli        t1, t1, 7
	or          t0, t0, t1
	or          a0, t0, a0

	// *** Basic block 1

.STypeInstruction_label_37:
	ret         
.func_end_STypeInstruction:
	.size STypeInstruction, .func_end_STypeInstruction-STypeInstruction

	.local  UTypeInstruction
	.type UTypeInstruction, @function

UTypeInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	slli        t0, a2, 12
	slli        t1, a1, 7
	or          t0, t0, t1
	or          a0, t0, a0

	// *** Basic block 1

.UTypeInstruction_label_18:
	ret         
.func_end_UTypeInstruction:
	.size UTypeInstruction, .func_end_UTypeInstruction-UTypeInstruction

	.local  BTypeInstruction
	.type BTypeInstruction, @function

BTypeInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	srai        t0, a4, 12
	andi        t0, t0, 1
	slli        t0, t0, 31
	srai        t1, a4, 5
	andi        t1, t1, 63
	slli        t1, t1, 25
	or          t0, t0, t1
	slli        t1, a2, 20
	or          t0, t0, t1
	slli        t1, a1, 15
	or          t0, t0, t1
	slli        t1, a3, 12
	or          t0, t0, t1
	srai        t1, a4, 1
	andi        t1, t1, 15
	slli        t1, t1, 8
	or          t0, t0, t1
	srai        t1, a4, 11
	andi        t1, t1, 1
	slli        t1, t1, 7
	or          t0, t0, t1
	or          a0, t0, a0

	// *** Basic block 1

.BTypeInstruction_label_51:
	ret         
.func_end_BTypeInstruction:
	.size BTypeInstruction, .func_end_BTypeInstruction-BTypeInstruction

	.local  JTypeInstruction
	.type JTypeInstruction, @function

JTypeInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	srai        t0, a2, 20
	andi        t0, t0, 1
	slli        t0, t0, 31
	srai        t1, a2, 1
	andi        t1, t1, 1023
	slli        t1, t1, 21
	or          t0, t0, t1
	srai        t1, a2, 11
	andi        t1, t1, 1
	slli        t1, t1, 20
	or          t0, t0, t1
	srai        t1, a2, 12
	andi        t1, t1, 255
	slli        t1, t1, 12
	or          t0, t0, t1
	slli        t1, a1, 7
	or          t0, t0, t1
	or          a0, t0, a0

	// *** Basic block 1

.JTypeInstruction_label_39:
	ret         
.func_end_JTypeInstruction:
	.size JTypeInstruction, .func_end_JTypeInstruction-JTypeInstruction

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

	.local  AssembleLoadImmediateConstant
	.type AssembleLoadImmediateConstant, @function

AssembleLoadImmediateConstant:

	// *** Basic block 0

	.global AssemblerEmitWord
	.local ITypeInstruction
	.local UTypeInstruction
	.local AssembleLoadImmediateConstant
	.local RTypeInstruction
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
	mv          s2, a0
	mv          s3, a1
	mv          s4, x0
	bge         s1, x0, .AssembleLoadImmediateConstant_label_56

	// *** Basic block 1

	li          s5, 63		// 0x3f ASCII '?'

	// *** Basic block 2

.AssembleLoadImmediateConstant_label_39:
	li          t0, 1		// 0x1 ASCII \x1
	sll         t0, t0, s5
	and         t0, s1, t0
	bnez        t0, .AssembleLoadImmediateConstant_label_47

	// *** Basic block 3

	addi        s4, s5, 1
	j           .AssembleLoadImmediateConstant_label_54

	// *** Basic block 4

.AssembleLoadImmediateConstant_label_47:

	// *** Basic block 5

.AssembleLoadImmediateConstant_label_48:
	addi        s5, s5, -1
	blt         s5, x0, .AssembleLoadImmediateConstant_label_39

	// *** Basic block 6

.AssembleLoadImmediateConstant_label_54:
	j           .AssembleLoadImmediateConstant_label_75

	// *** Basic block 7

.AssembleLoadImmediateConstant_label_56:
	li          s5, 63		// 0x3f ASCII '?'

	// *** Basic block 8

.AssembleLoadImmediateConstant_label_60:
	li          t0, 1		// 0x1 ASCII \x1
	sll         t0, t0, s5
	and         t0, s1, t0
	beqz        t0, .AssembleLoadImmediateConstant_label_68

	// *** Basic block 9

	addi        s4, s5, 1
	j           .AssembleLoadImmediateConstant_label_74

	// *** Basic block 10

.AssembleLoadImmediateConstant_label_68:

	// *** Basic block 11

.AssembleLoadImmediateConstant_label_69:
	addi        s5, s5, -1
	blt         s5, x0, .AssembleLoadImmediateConstant_label_60

	// *** Basic block 12

.AssembleLoadImmediateConstant_label_74:

	// *** Basic block 13

.AssembleLoadImmediateConstant_label_75:
	li          t0, 12		// 0xc ASCII \xc
	bge         s4, t0, .AssembleLoadImmediateConstant_label_103

	// *** Basic block 14

	lw          s5, 744(s2)
	sext.w      a4, s1
	mv          a3, x0
	mv          a2, x0
	mv          a1, s3
	li          t0, 19		// 0x13 ASCII \x13
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 15

	mv          a2, a0
	mv          a1, s5
	mv          a0, s2
	call        AssemblerEmitWord

	// *** Basic block 16

	j           .AssembleLoadImmediateConstant_label_225

	// *** Basic block 17

.AssembleLoadImmediateConstant_label_103:
	li          s5, 32		// 0x20 ASCII ' '
	bge         s4, s5, .AssembleLoadImmediateConstant_label_155

	// *** Basic block 18

	lw          s4, 744(s2)
	sext.w      s6, s1
	srai        a2, s6, 12
	mv          a1, s3
	li          t0, 55		// 0x37 ASCII '7'
	mv          a0, t0
	call        UTypeInstruction

	// *** Basic block 19

	mv          a2, a0
	mv          a1, s4
	mv          a0, s2
	call        AssemblerEmitWord

	// *** Basic block 20

	li          s4, 4095		// 0xfff
	and         t0, s1, s4
	beqz        t0, .AssembleLoadImmediateConstant_label_153

	// *** Basic block 21

	lw          s7, 744(s2)
	and         a4, s6, s4
	mv          a3, x0
	mv          a2, s3
	mv          a1, s3
	li          t0, 19		// 0x13 ASCII \x13
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 22

	mv          a2, a0
	mv          a1, s7
	mv          a0, s2
	call        AssemblerEmitWord

	// *** Basic block 23

.AssembleLoadImmediateConstant_label_153:
	j           .AssembleLoadImmediateConstant_label_224

	// *** Basic block 24

.AssembleLoadImmediateConstant_label_155:
	srai        a2, s1, 32
	li          s4, 6		// 0x6 ASCII \x6
	mv          a1, s4
	mv          a0, s2
	call        AssembleLoadImmediateConstant

	// *** Basic block 25

	lw          s6, 744(s2)
	mv          a5, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a4, t0
	mv          a3, s5
	mv          a2, s4
	mv          a1, s4
	li          s5, 51		// 0x33 ASCII '3'
	mv          a0, s5
	call        RTypeInstruction

	// *** Basic block 26

	mv          a2, a0
	mv          a1, s6
	mv          a0, s2
	call        AssemblerEmitWord

	// *** Basic block 27

	andi        a2, s1, -1
	mv          a1, s3
	mv          a0, s2
	call        AssembleLoadImmediateConstant

	// *** Basic block 28

	lw          s6, 744(s2)
	mv          a5, x0
	mv          a4, s4
	mv          a3, s4
	mv          a2, s3
	mv          a1, s3
	mv          a0, s5
	call        RTypeInstruction

	// *** Basic block 29

	mv          a2, a0
	mv          a1, s6
	mv          a0, s2
	call        AssemblerEmitWord

	// *** Basic block 30

.AssembleLoadImmediateConstant_label_224:

	// *** Basic block 31

.AssembleLoadImmediateConstant_label_225:
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
.func_end_AssembleLoadImmediateConstant:
	.size AssembleLoadImmediateConstant, .func_end_AssembleLoadImmediateConstant-AssembleLoadImmediateConstant

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

	lla         a1, .str.185
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
	lla         a1, .str.186
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 20

	mv          a0, x0
	j           .AssemblerFunction_label_86

	// *** Basic block 21

.AssemblerFunction_label_106:
	lla         a1, .str.187
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 22

	mv          a0, x0
	j           .AssemblerFunction_label_86

	// *** Basic block 23

.AssemblerFunction_label_116:
	lla         a1, .str.188
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 24

	mv          a0, x0
	j           .AssemblerFunction_label_86
.func_end_AssemblerFunction:
	.size AssemblerFunction, .func_end_AssemblerFunction-AssemblerFunction

	.local  AssembleALUReg
	.type AssembleALUReg, @function

AssembleALUReg:

	// *** Basic block 0

	.global AssemblerEmitWord
	.local RTypeInstruction
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
	mv          t1, a4
	mv          t2, a2
	mv          t3, a3
	lw          s2, 744(s1)
	lw          a1, 0(t1)
	lw          a2, 4(t1)
	lw          a3, 8(t1)
	mv          a5, t3
	mv          a4, t2
	mv          a0, t0
	call        RTypeInstruction

	// *** Basic block 1

	mv          a2, a0
	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssemblerEmitWord
.func_end_AssembleALUReg:
	.size AssembleALUReg, .func_end_AssembleALUReg-AssembleALUReg

	.local  AssembleALUImm
	.type AssembleALUImm, @function

AssembleALUImm:

	// *** Basic block 0

	.global AssemblerEmitWord
	.local ITypeInstruction
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
	mv          t1, a4
	mv          t2, a2
	mv          t3, a3
	lw          s2, 744(s1)
	lw          a1, 0(t1)
	lw          a2, 4(t1)
	mv          a4, t3
	mv          a3, t2
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 1

	mv          a2, a0
	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssemblerEmitWord
.func_end_AssembleALUImm:
	.size AssembleALUImm, .func_end_AssembleALUImm-AssembleALUImm

	.local  CheckLoadStoreOffset
	.type CheckLoadStoreOffset, @function

CheckLoadStoreOffset:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	li          t2, 1		// 0x1 ASCII \x1
	bge         t0, x0, .CheckLoadStoreOffset_label_22

	// *** Basic block 1

	slti        t3, t0, -2048
	not         t2, t3
	j           .CheckLoadStoreOffset_label_25

	// *** Basic block 2

.CheckLoadStoreOffset_label_22:
	li          t3, 2048		// 0x800
	slt         t2, t0, t3

	// *** Basic block 3

.CheckLoadStoreOffset_label_25:
	not         t2, t2
	beqz        t2, .CheckLoadStoreOffset_label_38

	// *** Basic block 4

	lla         a1, .str.189
	mv          a2, t0
	mv          a0, t1
	j           AssemblerError

	// *** Basic block 5

.CheckLoadStoreOffset_label_38:
	ret         
.func_end_CheckLoadStoreOffset:
	.size CheckLoadStoreOffset, .func_end_CheckLoadStoreOffset-CheckLoadStoreOffset

	.local  AssembleLoadStore
	.type AssembleLoadStore, @function

AssembleLoadStore:

	// *** Basic block 0

	.local CheckLoadStoreOffset
	.global AssemblerEmitWord
	.local ITypeInstruction
	.local STypeInstruction
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
	mv          s2, a3
	mv          s3, a1
	mv          s4, a4
	mv          s5, a2
	mv          a1, s2
	call        CheckLoadStoreOffset

	// *** Basic block 1

	beqz        s3, .AssembleLoadStore_label_57

	// *** Basic block 2

	lw          s3, 744(s1)
	lw          a1, 0(s4)
	lw          a2, 4(s4)
	mv          a4, s2
	mv          a3, s5
	li          t0, 3		// 0x3 ASCII \x3
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 3

	mv          a2, a0
	mv          a1, s3
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 4

	j           .AssembleLoadStore_label_81

	// *** Basic block 5

.AssembleLoadStore_label_57:
	lw          s3, 744(s1)
	lw          a1, 4(s4)
	lw          a2, 0(s4)
	mv          a4, s2
	mv          a3, s5
	li          t0, 35		// 0x23 ASCII '#'
	mv          a0, t0
	call        STypeInstruction

	// *** Basic block 6

	mv          a2, a0
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
	j           AssemblerEmitWord

	// *** Basic block 7

.AssembleLoadStore_label_81:
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
.func_end_AssembleLoadStore:
	.size AssembleLoadStore, .func_end_AssembleLoadStore-AssembleLoadStore

	.local  AssembleLoadStoreSymbol
	.type AssembleLoadStoreSymbol, @function

AssembleLoadStoreSymbol:

	// *** Basic block 0

	.local GetOrCreateSymbol
	.global StringEqual
	.global NewAssemblerRelocation
	.global AssemblerCurrentAddress
	.global AssemblerAddRelocation
	.global AssemblerEmitWord
	.local ITypeInstruction
	.local STypeInstruction
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
	mv          s2, a4
	mv          s3, a1
	mv          s4, a5
	mv          s5, a2
	ld          a1, 16(a3)
	call        GetOrCreateSymbol

	// *** Basic block 1

	mv          s6, a0
	lla         a1, .str.190
	mv          a0, s2
	call        StringEqual

	// *** Basic block 2

	mv          s7, a0
	beqz        s3, .AssembleLoadStoreSymbol_label_69

	// *** Basic block 3

	beqz        s7, .AssembleLoadStoreSymbol_label_64

	// *** Basic block 4

	li          s2, 24		// 0x18 ASCII \x18
	j           .AssembleLoadStoreSymbol_label_66

	// *** Basic block 5

.AssembleLoadStoreSymbol_label_64:
	li          s2, 27		// 0x1b ASCII \x1b

	// *** Basic block 6

.AssembleLoadStoreSymbol_label_66:
	j           .AssembleLoadStoreSymbol_label_78

	// *** Basic block 7

.AssembleLoadStoreSymbol_label_69:
	beqz        s7, .AssembleLoadStoreSymbol_label_75

	// *** Basic block 8

	li          s2, 25		// 0x19 ASCII \x19
	j           .AssembleLoadStoreSymbol_label_77

	// *** Basic block 9

.AssembleLoadStoreSymbol_label_75:
	li          s2, 28		// 0x1c ASCII \x1c

	// *** Basic block 10

.AssembleLoadStoreSymbol_label_77:

	// *** Basic block 11

.AssembleLoadStoreSymbol_label_78:
	lw          s8, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 12

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s8
	mv          a1, s2
	mv          a0, s6
	call        NewAssemblerRelocation

	// *** Basic block 13

	mv          s2, a0
	mv          a1, s2
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 14

	beqz        s3, .AssembleLoadStoreSymbol_label_126

	// *** Basic block 15

	lw          s2, 744(s1)
	lw          a1, 0(s4)
	lw          a2, 4(s4)
	mv          a4, x0
	mv          a3, s5
	li          t0, 3		// 0x3 ASCII \x3
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 16

	mv          a2, a0
	mv          a1, s2
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 17

	j           .AssembleLoadStoreSymbol_label_150

	// *** Basic block 18

.AssembleLoadStoreSymbol_label_126:
	lw          s2, 744(s1)
	lw          a1, 4(s4)
	lw          a2, 0(s4)
	mv          a4, x0
	mv          a3, s5
	li          t0, 35		// 0x23 ASCII '#'
	mv          a0, t0
	call        STypeInstruction

	// *** Basic block 19

	mv          a2, a0
	mv          a1, s2
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
	j           AssemblerEmitWord

	// *** Basic block 20

.AssembleLoadStoreSymbol_label_150:
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
.func_end_AssembleLoadStoreSymbol:
	.size AssembleLoadStoreSymbol, .func_end_AssembleLoadStoreSymbol-AssembleLoadStoreSymbol

	.local  AssembleLoadStoreInstruction
	.type AssembleLoadStoreInstruction, @function

AssembleLoadStoreInstruction:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global StringInit
	.local AssemblerFunction
	.global StringEqual
	.local AssembleLoadStoreSymbol
	.global StringDestruct
	.global AssemblerEvaluateExpression
	.local AssembleLoadStore
	addi sp, sp, -160
	// Saved return address (offset 152) and frame pointer (offset 144)
	sd ra, 152(sp)
	sd s0, 144(sp)
	addi s0, sp, 160
	// Local vars at offset -112(s0)
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
	lla         a2, .str.191
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -112(s0)
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .AssembleLoadStoreInstruction_label_73

	// *** Basic block 3

	lla         a1, .str.192
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 4

.AssembleLoadStoreInstruction_label_70:
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

.AssembleLoadStoreInstruction_label_73:
	addi        a0, s1, 176
	li          t0, 38		// 0x26 ASCII '&'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 6

	beqz        a0, .AssembleLoadStoreInstruction_label_204

	// *** Basic block 7

	addi        a0, s0, -104
	lla         a1, .str.193
	call        StringInit

	// *** Basic block 8

	addi        a0, s0, -64
	lla         a1, .str.194
	call        StringInit

	// *** Basic block 9

	addi        a1, s0, -104
	addi        a2, s0, -64
	mv          a0, s1
	call        AssemblerFunction

	// *** Basic block 10

	mv          s4, a0
	not         t0, s4
	bnez        t0, .AssembleLoadStoreInstruction_label_196

	// *** Basic block 11

.AssembleLoadStoreInstruction_label_106:
	addi        a0, s0, -104
	lla         a1, .str.195
	call        StringEqual

	// *** Basic block 12

	not         s4, a0
	beqz        s4, .AssembleLoadStoreInstruction_label_123

	// *** Basic block 13

	addi        a0, s0, -104
	lla         a1, .str.196
	call        StringEqual

	// *** Basic block 14

	not         s4, a0

	// *** Basic block 15

.AssembleLoadStoreInstruction_label_123:
	beqz        s4, .AssembleLoadStoreInstruction_label_136

	// *** Basic block 16

	lla         a1, .str.197
	addi        t0, s0, -104
	ld          a2, 16(t0)
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 17

	j           .AssembleLoadStoreInstruction_label_196

	// *** Basic block 18

.AssembleLoadStoreInstruction_label_136:
	addi        a0, s1, 176
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	call        LexMatch

	// *** Basic block 19

	not         t0, a0
	beqz        t0, .AssembleLoadStoreInstruction_label_152

	// *** Basic block 20

	lla         a1, .str.198
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 21

	j           .AssembleLoadStoreInstruction_label_196

	// *** Basic block 22

.AssembleLoadStoreInstruction_label_152:
	addi        s4, s0, -112
	lla         a2, .str.199
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 23

	sw          a0, 4(s4)
	addi        a0, s1, 176
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 24

	not         t0, a0
	beqz        t0, .AssembleLoadStoreInstruction_label_179

	// *** Basic block 25

	lla         a1, .str.200
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 26

	j           .AssembleLoadStoreInstruction_label_196

	// *** Basic block 27

.AssembleLoadStoreInstruction_label_179:
	addi        a3, s0, -64
	addi        a4, s0, -104
	addi        a5, s0, -112
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        AssembleLoadStoreSymbol

	// *** Basic block 28

.AssembleLoadStoreInstruction_label_196:
	addi        a0, s0, -104
	call        StringDestruct

	// *** Basic block 29

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 30

	j           .AssembleLoadStoreInstruction_label_264

	// *** Basic block 31

.AssembleLoadStoreInstruction_label_204:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 32

	sext.w      s5, a0
	addi        a0, s1, 176
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	call        LexMatch

	// *** Basic block 33

	not         t0, a0
	beqz        t0, .AssembleLoadStoreInstruction_label_225

	// *** Basic block 34

	lla         a1, .str.201
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 35

	j           .AssembleLoadStoreInstruction_label_70

	// *** Basic block 36

.AssembleLoadStoreInstruction_label_225:
	addi        s6, s0, -112
	lla         a2, .str.202
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 37

	sw          a0, 4(s6)
	addi        a0, s1, 176
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 38

	not         t0, a0
	beqz        t0, .AssembleLoadStoreInstruction_label_252

	// *** Basic block 39

	lla         a1, .str.203
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 40

	j           .AssembleLoadStoreInstruction_label_70

	// *** Basic block 41

.AssembleLoadStoreInstruction_label_252:
	addi        a4, s0, -112
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        AssembleLoadStore

	// *** Basic block 42

.AssembleLoadStoreInstruction_label_264:
	j           .AssembleLoadStoreInstruction_label_70
.func_end_AssembleLoadStoreInstruction:
	.size AssembleLoadStoreInstruction, .func_end_AssembleLoadStoreInstruction-AssembleLoadStoreInstruction

	.local  Assemble_mv
	.type Assemble_mv, @function

Assemble_mv:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALUImm
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
	lla         a2, .str.204
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_mv_label_41

	// *** Basic block 2

	mv          a4, s2
	mv          a3, x0
	mv          a2, x0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUImm

	// *** Basic block 3

.Assemble_mv_label_41:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_mv:
	.size Assemble_mv, .func_end_Assemble_mv-Assemble_mv

	.local  Assemble_fmv_s
	.type Assemble_fmv_s, @function

Assemble_fmv_s:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALUReg
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	lla         a2, .str.205
	addi        a3, s0, -32
	li          a1, 1		// 0x1 ASCII \x1
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_fmv_s_label_52

	// *** Basic block 2

	addi        t0, s0, -32
	addi        t1, s0, -32
	lw          t1, 4(t1)
	sw          t1, 8(t0)
	addi        a4, s0, -32
	li          t0, 16		// 0x10 ASCII \x10
	mv          a3, t0
	mv          a2, x0
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_fmv_s_label_52:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fmv_s:
	.size Assemble_fmv_s, .func_end_Assemble_fmv_s-Assemble_fmv_s

	.local  Assemble_fmv_d
	.type Assemble_fmv_d, @function

Assemble_fmv_d:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALUReg
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	lla         a2, .str.206
	addi        a3, s0, -32
	li          a1, 1		// 0x1 ASCII \x1
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_fmv_d_label_52

	// *** Basic block 2

	addi        t0, s0, -32
	addi        t1, s0, -32
	lw          t1, 4(t1)
	sw          t1, 8(t0)
	addi        a4, s0, -32
	li          t0, 17		// 0x11 ASCII \x11
	mv          a3, t0
	mv          a2, x0
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_fmv_d_label_52:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fmv_d:
	.size Assemble_fmv_d, .func_end_Assemble_fmv_d-Assemble_fmv_d

	.local  Assemble_ret
	.type Assemble_ret, @function

Assemble_ret:

	// *** Basic block 0

	.global AssemblerEmitWord
	.local ITypeInstruction
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
	lw          s2, 744(s1)
	mv          a4, x0
	mv          a3, x0
	li          a2, 1		// 0x1 ASCII \x1
	mv          a1, x0
	li          a0, 103		// 0x67 ASCII 'g'
	call        ITypeInstruction

	// *** Basic block 1

	mv          a2, a0
	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssemblerEmitWord
.func_end_Assemble_ret:
	.size Assemble_ret, .func_end_Assemble_ret-Assemble_ret

	.local  AssembleJType
	.type AssembleJType, @function

AssembleJType:

	// *** Basic block 0

	.local GetOrCreateSymbol
	.global NewAssemblerRelocation
	.global AssemblerCurrentAddress
	.global AssemblerAddRelocation
	.global AssemblerEmitWord
	.local JTypeInstruction
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
	mv          s2, a4
	mv          s3, a1
	mv          s4, a2
	ld          a1, 16(a3)
	call        GetOrCreateSymbol

	// *** Basic block 1

	mv          s5, a0
	lw          s6, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 2

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s6
	mv          a1, s2
	mv          a0, s5
	call        NewAssemblerRelocation

	// *** Basic block 3

	mv          s6, a0
	mv          a1, s6
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 4

	lw          s7, 744(s1)
	mv          a2, x0
	mv          a1, s4
	mv          a0, s3
	call        JTypeInstruction

	// *** Basic block 5

	mv          a2, a0
	mv          a1, s7
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
	j           AssemblerEmitWord
.func_end_AssembleJType:
	.size AssembleJType, .func_end_AssembleJType-AssembleJType

	.local  AssembleUType
	.type AssembleUType, @function

AssembleUType:

	// *** Basic block 0

	.local GetOrCreateSymbol
	.global NewAssemblerRelocation
	.global AssemblerCurrentAddress
	.global AssemblerAddRelocation
	.global AssemblerEmitWord
	.local UTypeInstruction
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
	mv          s2, a4
	mv          s3, a5
	mv          s4, a1
	mv          s5, a2
	ld          a1, 16(a3)
	call        GetOrCreateSymbol

	// *** Basic block 1

	mv          s6, a0
	mv          s7, s2
	lb          t0, 760(s1)
	beqz        t0, .AssembleUType_label_50

	// *** Basic block 2

	lw          t1, 44(s6)
	seqz        t0, t1

	// *** Basic block 3

.AssembleUType_label_50:
	beqz        t0, .AssembleUType_label_53

	// *** Basic block 4

	mv          s7, s3

	// *** Basic block 5

.AssembleUType_label_53:
	lw          s2, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 6

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s2
	mv          a1, s7
	mv          a0, s6
	call        NewAssemblerRelocation

	// *** Basic block 7

	mv          s2, a0
	mv          a1, s2
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 8

	lw          s8, 744(s1)
	mv          a2, x0
	mv          a1, s5
	mv          a0, s4
	call        UTypeInstruction

	// *** Basic block 9

	mv          a2, a0
	mv          a1, s8
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
	j           AssemblerEmitWord
.func_end_AssembleUType:
	.size AssembleUType, .func_end_AssembleUType-AssembleUType

	.local  Assemble_lui
	.type Assemble_lui, @function

Assemble_lui:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global StringInit
	.local AssemblerFunction
	.global StringDestruct
	.global StringEqual
	.local AssembleUType
	.global LexLookingAt
	.global StringSet
	.global LexNextToken
	.global AssemblerEvaluateExpression
	.global AssemblerEmitWord
	.local UTypeInstruction
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
	lla         a2, .str.207
	mv          a1, x0
	call        Register

	// *** Basic block 1

	mv          s2, a0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .Assemble_lui_label_67

	// *** Basic block 3

	lla         a1, .str.208
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 4

.Assemble_lui_label_64:
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

.Assemble_lui_label_67:
	addi        a0, s0, -96
	lla         a1, .str.209
	call        StringInit

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 38		// 0x26 ASCII '&'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_lui_label_151

	// *** Basic block 8

	addi        a0, s0, -56
	lla         a1, .str.210
	call        StringInit

	// *** Basic block 9

	addi        a1, s0, -56
	addi        a2, s0, -96
	mv          a0, s1
	call        AssemblerFunction

	// *** Basic block 10

	mv          s3, a0
	not         t0, s3
	beqz        t0, .Assemble_lui_label_105

	// *** Basic block 11

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 12

	j           .Assemble_lui_label_64

	// *** Basic block 13

.Assemble_lui_label_105:
	addi        a0, s0, -56
	lla         a1, .str.211
	call        StringEqual

	// *** Basic block 14

	not         t0, a0
	beqz        t0, .Assemble_lui_label_128

	// *** Basic block 15

	lla         a1, .str.212
	addi        t0, s0, -56
	ld          a2, 16(t0)
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 16

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 17

	j           .Assemble_lui_label_64

	// *** Basic block 18

.Assemble_lui_label_128:
	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 19

	addi        a3, s0, -96
	li          t0, 20		// 0x14 ASCII \x14
	mv          a5, t0
	li          t0, 26		// 0x1a ASCII \x1a
	mv          a4, t0
	mv          a2, s2
	li          t0, 55		// 0x37 ASCII '7'
	mv          a1, t0
	mv          a0, s1
	call        AssembleUType

	// *** Basic block 20

.Assemble_lui_label_151:
	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 21

	beqz        a0, .Assemble_lui_label_186

	// *** Basic block 22

	addi        a0, s0, -96
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringSet

	// *** Basic block 23

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 24

	addi        a3, s0, -96
	li          t0, 20		// 0x14 ASCII \x14
	mv          a5, t0
	li          t0, 26		// 0x1a ASCII \x1a
	mv          a4, t0
	mv          a2, s2
	li          t0, 55		// 0x37 ASCII '7'
	mv          a1, t0
	mv          a0, s1
	call        AssembleUType

	// *** Basic block 25

	j           .Assemble_lui_label_209

	// *** Basic block 26

.Assemble_lui_label_186:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 27

	mv          s4, a0
	lw          s5, 744(s1)
	sext.w      a2, s4
	mv          a1, s2
	li          t0, 55		// 0x37 ASCII '7'
	mv          a0, t0
	call        UTypeInstruction

	// *** Basic block 28

	mv          a2, a0
	mv          a1, s5
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 29

.Assemble_lui_label_209:
	j           .Assemble_lui_label_64
.func_end_Assemble_lui:
	.size Assemble_lui, .func_end_Assemble_lui-Assemble_lui

	.local  Assemble_auipc
	.type Assemble_auipc, @function

Assemble_auipc:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global StringInit
	.local AssemblerFunction
	.global StringDestruct
	.global StringEqual
	.global LexLookingAt
	.global StringSet
	.global LexNextToken
	.global AssemblerEvaluateExpression
	.global AssemblerEmitWord
	.local UTypeInstruction
	.local AssembleUType
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
	lla         a2, .str.213
	mv          a1, x0
	call        Register

	// *** Basic block 1

	mv          s2, a0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .Assemble_auipc_label_66

	// *** Basic block 3

	lla         a1, .str.214
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 4

.Assemble_auipc_label_63:
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

.Assemble_auipc_label_66:
	addi        a0, s0, -96
	lla         a1, .str.215
	call        StringInit

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 38		// 0x26 ASCII '&'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_auipc_label_132

	// *** Basic block 8

	addi        a0, s0, -56
	lla         a1, .str.216
	call        StringInit

	// *** Basic block 9

	addi        a1, s0, -56
	addi        a2, s0, -96
	mv          a0, s1
	call        AssemblerFunction

	// *** Basic block 10

	mv          s3, a0
	not         t0, s3
	beqz        t0, .Assemble_auipc_label_104

	// *** Basic block 11

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 12

	j           .Assemble_auipc_label_63

	// *** Basic block 13

.Assemble_auipc_label_104:
	addi        a0, s0, -56
	lla         a1, .str.217
	call        StringEqual

	// *** Basic block 14

	not         t0, a0
	beqz        t0, .Assemble_auipc_label_127

	// *** Basic block 15

	lla         a1, .str.218
	addi        t0, s0, -56
	ld          a2, 16(t0)
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 16

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 17

	j           .Assemble_auipc_label_63

	// *** Basic block 18

.Assemble_auipc_label_127:
	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 19

	j           .Assemble_auipc_label_176

	// *** Basic block 20

.Assemble_auipc_label_132:
	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 21

	beqz        a0, .Assemble_auipc_label_151

	// *** Basic block 22

	addi        a0, s0, -96
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringSet

	// *** Basic block 23

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 24

	j           .Assemble_auipc_label_175

	// *** Basic block 25

.Assemble_auipc_label_151:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 26

	mv          s4, a0
	lw          s5, 744(s1)
	sext.w      a2, s4
	mv          a1, s2
	li          t0, 23		// 0x17 ASCII \x17
	mv          a0, t0
	call        UTypeInstruction

	// *** Basic block 27

	mv          a2, a0
	mv          a1, s5
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 28

	j           .Assemble_auipc_label_63

	// *** Basic block 29

.Assemble_auipc_label_175:

	// *** Basic block 30

.Assemble_auipc_label_176:
	addi        a3, s0, -96
	li          t0, 20		// 0x14 ASCII \x14
	mv          a5, t0
	li          t0, 23		// 0x17 ASCII \x17
	mv          a4, t0
	mv          a2, s2
	mv          a1, t0
	mv          a0, s1
	call        AssembleUType

	// *** Basic block 31

	j           .Assemble_auipc_label_63
.func_end_Assemble_auipc:
	.size Assemble_auipc, .func_end_Assemble_auipc-Assemble_auipc

	.local  Assemble_jal
	.type Assemble_jal, @function

Assemble_jal:

	// *** Basic block 0

	.local OptionalRegister
	.global LexMatch
	.global AssemblerError
	.global LexLookingAt
	.global StringInit
	.global LexNextToken
	.local AssembleJType
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
	call        OptionalRegister

	// *** Basic block 1

	mv          s2, a0
	li          t0, -1		// 0xffffffffffffffff
	bne         s2, t0, .Assemble_jal_label_35

	// *** Basic block 2

	li          s2, 1		// 0x1 ASCII \x1
	j           .Assemble_jal_label_55

	// *** Basic block 3

.Assemble_jal_label_35:
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 4

	not         t0, a0
	beqz        t0, .Assemble_jal_label_54

	// *** Basic block 5

	lla         a1, .str.219
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 6

.Assemble_jal_label_51:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.Assemble_jal_label_54:

	// *** Basic block 8

.Assemble_jal_label_55:
	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 9

	not         t0, a0
	beqz        t0, .Assemble_jal_label_71

	// *** Basic block 10

	lla         a1, .str.220
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 11

	j           .Assemble_jal_label_51

	// *** Basic block 12

.Assemble_jal_label_71:
	addi        a0, s0, -64
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 13

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 14

	addi        a3, s0, -64
	mv          a4, x0
	mv          a2, s2
	li          t0, 111		// 0x6f ASCII 'o'
	mv          a1, t0
	mv          a0, s1
	call        AssembleJType

	// *** Basic block 15

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 16

	j           .Assemble_jal_label_51
.func_end_Assemble_jal:
	.size Assemble_jal, .func_end_Assemble_jal-Assemble_jal

	.local  Assemble_j
	.type Assemble_j, @function

Assemble_j:

	// *** Basic block 0

	.global LexLookingAt
	.global AssemblerError
	.global StringInit
	.global LexNextToken
	.local GetOrCreateSymbol
	.global AssemblerCurrentAddress
	.global AssemblerEmitWord
	.local JTypeInstruction
	.global NewAssemblerRelocation
	.global AssemblerAddRelocation
	.local UTypeInstruction
	.local ITypeInstruction
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
	addi        a0, s1, 176
	li          a1, 3		// 0x3 ASCII \x3
	call        LexLookingAt

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .Assemble_j_label_54

	// *** Basic block 2

	lla         a1, .str.221
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 3

.Assemble_j_label_51:
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

.Assemble_j_label_54:
	addi        a0, s0, -64
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 5

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 6

	addi        t0, s0, -64
	ld          a1, 16(t0)
	mv          a0, s1
	call        GetOrCreateSymbol

	// *** Basic block 7

	mv          s2, a0
	lb          t0, 73(s2)
	beqz        t0, .Assemble_j_label_86

	// *** Basic block 8

	lw          t1, 44(s2)
	addi        t1, t1, -1
	seqz        t0, t1

	// *** Basic block 9

.Assemble_j_label_86:
	beqz        t0, .Assemble_j_label_116

	// *** Basic block 10

	ld          s3, 48(s2)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 11

	sub         t0, s3, a0
	sext.w      s3, t0
	lw          s4, 744(s1)
	mv          a2, s3
	mv          a1, x0
	li          t0, 111		// 0x6f ASCII 'o'
	mv          a0, t0
	call        JTypeInstruction

	// *** Basic block 12

	mv          a2, a0
	mv          a1, s4
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 13

	j           .Assemble_j_label_220

	// *** Basic block 14

.Assemble_j_label_116:
	li          s3, 18		// 0x12 ASCII \x12
	lw          t1, 44(s2)
	seqz        t0, t1
	bnez        t1, .Assemble_j_label_126

	// *** Basic block 15

	lb          t0, 760(s1)

	// *** Basic block 16

.Assemble_j_label_126:
	beqz        t0, .Assemble_j_label_129

	// *** Basic block 17

	li          s3, 19		// 0x13 ASCII \x13

	// *** Basic block 18

.Assemble_j_label_129:
	lw          s4, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 19

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s4
	mv          a1, s3
	mv          a0, s2
	call        NewAssemblerRelocation

	// *** Basic block 20

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 21

	li          t0, 19		// 0x13 ASCII \x13
	bne         s3, t0, .Assemble_j_label_201

	// *** Basic block 22

	lw          s3, 744(s1)
	mv          a2, x0
	li          s4, 5		// 0x5 ASCII \x5
	mv          a1, s4
	li          t0, 23		// 0x17 ASCII \x17
	mv          a0, t0
	call        UTypeInstruction

	// *** Basic block 23

	mv          a2, a0
	mv          a1, s3
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 24

	lw          s3, 744(s1)
	mv          a4, x0
	mv          a3, x0
	mv          a2, s4
	mv          a1, x0
	li          t0, 103		// 0x67 ASCII 'g'
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 25

	mv          a2, a0
	mv          a1, s3
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 26

	j           .Assemble_j_label_219

	// *** Basic block 27

.Assemble_j_label_201:
	lw          s3, 744(s1)
	mv          a2, x0
	mv          a1, x0
	li          t0, 111		// 0x6f ASCII 'o'
	mv          a0, t0
	call        JTypeInstruction

	// *** Basic block 28

	mv          a2, a0
	mv          a1, s3
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 29

.Assemble_j_label_219:

	// *** Basic block 30

.Assemble_j_label_220:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 31

	j           .Assemble_j_label_51
.func_end_Assemble_j:
	.size Assemble_j, .func_end_Assemble_j-Assemble_j

	.local  Assemble_jr
	.type Assemble_jr, @function

Assemble_jr:

	// *** Basic block 0

	.local Register
	.global AssemblerEmitWord
	.local ITypeInstruction
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
	lla         a2, .str.222
	mv          a1, x0
	call        Register

	// *** Basic block 1

	mv          s2, a0
	lw          s3, 744(s1)
	mv          a4, x0
	mv          a3, x0
	mv          a2, s2
	mv          a1, x0
	li          t0, 103		// 0x67 ASCII 'g'
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 2

	mv          a2, a0
	mv          a1, s3
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AssemblerEmitWord
.func_end_Assemble_jr:
	.size Assemble_jr, .func_end_Assemble_jr-Assemble_jr

	.local  Assemble_call
	.type Assemble_call, @function

Assemble_call:

	// *** Basic block 0

	.global LexLookingAt
	.global AssemblerError
	.global StringInit
	.global LexNextToken
	.local GetOrCreateSymbol
	.global NewAssemblerRelocation
	.global AssemblerCurrentAddress
	.global AssemblerAddRelocation
	.global AssemblerEmitWord
	.local UTypeInstruction
	.local ITypeInstruction
	.global StringDestruct
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

	not         t0, a0
	beqz        t0, .Assemble_call_label_50

	// *** Basic block 2

	lla         a1, .str.223
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 3

.Assemble_call_label_47:
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

.Assemble_call_label_50:
	addi        a0, s0, -64
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 5

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 6

	addi        t0, s0, -64
	ld          a1, 16(t0)
	mv          a0, s1
	call        GetOrCreateSymbol

	// *** Basic block 7

	mv          s2, a0
	li          s3, 18		// 0x12 ASCII \x12
	lw          t1, 44(s2)
	seqz        t0, t1
	bnez        t1, .Assemble_call_label_82

	// *** Basic block 8

	lb          t0, 760(s1)

	// *** Basic block 9

.Assemble_call_label_82:
	beqz        t0, .Assemble_call_label_85

	// *** Basic block 10

	li          s3, 19		// 0x13 ASCII \x13

	// *** Basic block 11

.Assemble_call_label_85:
	lw          s4, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 12

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s4
	mv          a1, s3
	mv          a0, s2
	call        NewAssemblerRelocation

	// *** Basic block 13

	mv          s3, a0
	mv          a1, s3
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 14

	lw          s4, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 15

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s4
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s2
	call        NewAssemblerRelocation

	// *** Basic block 16

	mv          s3, a0
	mv          a1, s3
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 17

	lw          s4, 744(s1)
	mv          a2, x0
	li          s5, 1		// 0x1 ASCII \x1
	mv          a1, s5
	li          t0, 23		// 0x17 ASCII \x17
	mv          a0, t0
	call        UTypeInstruction

	// *** Basic block 18

	mv          a2, a0
	mv          a1, s4
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 19

	lw          s4, 744(s1)
	mv          a4, x0
	mv          a3, x0
	mv          a2, s5
	mv          a1, s5
	li          t0, 103		// 0x67 ASCII 'g'
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 20

	mv          a2, a0
	mv          a1, s4
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 21

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 22

	j           .Assemble_call_label_47
.func_end_Assemble_call:
	.size Assemble_call, .func_end_Assemble_call-Assemble_call

	.local  Assemble_jalr
	.type Assemble_jalr, @function

Assemble_jalr:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.global AssemblerEmitWord
	.local ITypeInstruction
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
	mv          s1, a0
	lla         a2, .str.224
	addi        a3, s0, -32
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_jalr_label_85

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .Assemble_jalr_label_55

	// *** Basic block 4

	lla         a1, .str.225
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.Assemble_jalr_label_52:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.Assemble_jalr_label_55:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 7

	mv          s2, a0
	lw          s3, 744(s1)
	lw          a1, -32(s0)
	addi        t0, s0, -32
	lw          a2, 4(t0)
	sext.w      a4, s2
	mv          a3, x0
	li          t0, 103		// 0x67 ASCII 'g'
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 8

	mv          a2, a0
	mv          a1, s3
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 9

.Assemble_jalr_label_85:
	j           .Assemble_jalr_label_52
.func_end_Assemble_jalr:
	.size Assemble_jalr, .func_end_Assemble_jalr-Assemble_jalr

	.local  AssembleConditionalBranch
	.type AssembleConditionalBranch, @function

AssembleConditionalBranch:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.global AssemblerCurrentAddress
	.global AssemblerEmitWord
	.local BTypeInstruction
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
	mv          s2, a1
	lla         a2, .str.226
	addi        a3, s0, -32
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .AssembleConditionalBranch_label_94

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .AssembleConditionalBranch_label_58

	// *** Basic block 4

	lla         a1, .str.227
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.AssembleConditionalBranch_label_55:
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

.AssembleConditionalBranch_label_58:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 7

	mv          s3, a0
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 8

	sub         t0, s3, a0
	sext.w      s4, t0
	lw          s5, 744(s1)
	lw          a1, -32(s0)
	addi        t0, s0, -32
	lw          a2, 4(t0)
	mv          a4, s4
	mv          a3, s2
	li          t0, 99		// 0x63 ASCII 'c'
	mv          a0, t0
	call        BTypeInstruction

	// *** Basic block 9

	mv          a2, a0
	mv          a1, s5
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 10

.AssembleConditionalBranch_label_94:
	j           .AssembleConditionalBranch_label_55
.func_end_AssembleConditionalBranch:
	.size AssembleConditionalBranch, .func_end_AssembleConditionalBranch-AssembleConditionalBranch

	.local  Assemble_beq
	.type Assemble_beq, @function

Assemble_beq:

	// *** Basic block 0

	.local AssembleConditionalBranch
	// Leaf procedure, no stack frame generated
	mv          a1, x0
	j           AssembleConditionalBranch
.func_end_Assemble_beq:
	.size Assemble_beq, .func_end_Assemble_beq-Assemble_beq

	.local  Assemble_bne
	.type Assemble_bne, @function

Assemble_bne:

	// *** Basic block 0

	.local AssembleConditionalBranch
	// Leaf procedure, no stack frame generated
	li          a1, 1		// 0x1 ASCII \x1
	j           AssembleConditionalBranch
.func_end_Assemble_bne:
	.size Assemble_bne, .func_end_Assemble_bne-Assemble_bne

	.local  Assemble_blt
	.type Assemble_blt, @function

Assemble_blt:

	// *** Basic block 0

	.local AssembleConditionalBranch
	// Leaf procedure, no stack frame generated
	li          a1, 4		// 0x4 ASCII \x4
	j           AssembleConditionalBranch
.func_end_Assemble_blt:
	.size Assemble_blt, .func_end_Assemble_blt-Assemble_blt

	.local  Assemble_bge
	.type Assemble_bge, @function

Assemble_bge:

	// *** Basic block 0

	.local AssembleConditionalBranch
	// Leaf procedure, no stack frame generated
	li          a1, 5		// 0x5 ASCII \x5
	j           AssembleConditionalBranch
.func_end_Assemble_bge:
	.size Assemble_bge, .func_end_Assemble_bge-Assemble_bge

	.local  Assemble_bltu
	.type Assemble_bltu, @function

Assemble_bltu:

	// *** Basic block 0

	.local AssembleConditionalBranch
	// Leaf procedure, no stack frame generated
	li          a1, 6		// 0x6 ASCII \x6
	j           AssembleConditionalBranch
.func_end_Assemble_bltu:
	.size Assemble_bltu, .func_end_Assemble_bltu-Assemble_bltu

	.local  Assemble_bgeu
	.type Assemble_bgeu, @function

Assemble_bgeu:

	// *** Basic block 0

	.local AssembleConditionalBranch
	// Leaf procedure, no stack frame generated
	li          a1, 7		// 0x7 ASCII \x7
	j           AssembleConditionalBranch
.func_end_Assemble_bgeu:
	.size Assemble_bgeu, .func_end_Assemble_bgeu-Assemble_bgeu

	.local  Assemble_lb
	.type Assemble_lb, @function

Assemble_lb:

	// *** Basic block 0

	.local AssembleLoadStoreInstruction
	// Leaf procedure, no stack frame generated
	mv          a2, x0
	li          a1, 1		// 0x1 ASCII \x1
	j           AssembleLoadStoreInstruction
.func_end_Assemble_lb:
	.size Assemble_lb, .func_end_Assemble_lb-Assemble_lb

	.local  Assemble_lh
	.type Assemble_lh, @function

Assemble_lh:

	// *** Basic block 0

	.local AssembleLoadStoreInstruction
	// Leaf procedure, no stack frame generated
	li          a2, 1		// 0x1 ASCII \x1
	li          a1, 1		// 0x1 ASCII \x1
	j           AssembleLoadStoreInstruction
.func_end_Assemble_lh:
	.size Assemble_lh, .func_end_Assemble_lh-Assemble_lh

	.local  Assemble_lw
	.type Assemble_lw, @function

Assemble_lw:

	// *** Basic block 0

	.local AssembleLoadStoreInstruction
	// Leaf procedure, no stack frame generated
	li          a2, 2		// 0x2 ASCII \x2
	li          a1, 1		// 0x1 ASCII \x1
	j           AssembleLoadStoreInstruction
.func_end_Assemble_lw:
	.size Assemble_lw, .func_end_Assemble_lw-Assemble_lw

	.local  Assemble_lbu
	.type Assemble_lbu, @function

Assemble_lbu:

	// *** Basic block 0

	.local AssembleLoadStoreInstruction
	// Leaf procedure, no stack frame generated
	li          a2, 4		// 0x4 ASCII \x4
	li          a1, 1		// 0x1 ASCII \x1
	j           AssembleLoadStoreInstruction
.func_end_Assemble_lbu:
	.size Assemble_lbu, .func_end_Assemble_lbu-Assemble_lbu

	.local  Assemble_lhu
	.type Assemble_lhu, @function

Assemble_lhu:

	// *** Basic block 0

	.local AssembleLoadStoreInstruction
	// Leaf procedure, no stack frame generated
	li          a2, 5		// 0x5 ASCII \x5
	li          a1, 1		// 0x1 ASCII \x1
	j           AssembleLoadStoreInstruction
.func_end_Assemble_lhu:
	.size Assemble_lhu, .func_end_Assemble_lhu-Assemble_lhu

	.local  Assemble_sb
	.type Assemble_sb, @function

Assemble_sb:

	// *** Basic block 0

	.local AssembleLoadStoreInstruction
	// Leaf procedure, no stack frame generated
	mv          a2, x0
	mv          a1, x0
	j           AssembleLoadStoreInstruction
.func_end_Assemble_sb:
	.size Assemble_sb, .func_end_Assemble_sb-Assemble_sb

	.local  Assemble_sh
	.type Assemble_sh, @function

Assemble_sh:

	// *** Basic block 0

	.local AssembleLoadStoreInstruction
	// Leaf procedure, no stack frame generated
	li          a2, 1		// 0x1 ASCII \x1
	mv          a1, x0
	j           AssembleLoadStoreInstruction
.func_end_Assemble_sh:
	.size Assemble_sh, .func_end_Assemble_sh-Assemble_sh

	.local  Assemble_sw
	.type Assemble_sw, @function

Assemble_sw:

	// *** Basic block 0

	.local AssembleLoadStoreInstruction
	// Leaf procedure, no stack frame generated
	li          a2, 2		// 0x2 ASCII \x2
	mv          a1, x0
	j           AssembleLoadStoreInstruction
.func_end_Assemble_sw:
	.size Assemble_sw, .func_end_Assemble_sw-Assemble_sw

	.local  Assemble_addi
	.type Assemble_addi, @function

Assemble_addi:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleALUImm
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
	mv          s1, a0
	lla         a2, .str.228
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_addi_label_71

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .Assemble_addi_label_52

	// *** Basic block 4

	lla         a1, .str.229
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.Assemble_addi_label_49:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.Assemble_addi_label_52:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 7

	sext.w      s3, a0
	mv          a4, s2
	mv          a3, s3
	mv          a2, x0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUImm

	// *** Basic block 8

.Assemble_addi_label_71:
	j           .Assemble_addi_label_49
.func_end_Assemble_addi:
	.size Assemble_addi, .func_end_Assemble_addi-Assemble_addi

	.local  Assemble_slti
	.type Assemble_slti, @function

Assemble_slti:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleALUImm
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
	mv          s1, a0
	lla         a2, .str.230
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_slti_label_73

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .Assemble_slti_label_53

	// *** Basic block 4

	lla         a1, .str.231
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.Assemble_slti_label_50:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.Assemble_slti_label_53:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 7

	sext.w      s3, a0
	mv          a4, s2
	mv          a3, s3
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUImm

	// *** Basic block 8

.Assemble_slti_label_73:
	j           .Assemble_slti_label_50
.func_end_Assemble_slti:
	.size Assemble_slti, .func_end_Assemble_slti-Assemble_slti

	.local  Assemble_sltiu
	.type Assemble_sltiu, @function

Assemble_sltiu:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleALUImm
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
	mv          s1, a0
	lla         a2, .str.232
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_sltiu_label_73

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .Assemble_sltiu_label_53

	// *** Basic block 4

	lla         a1, .str.233
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.Assemble_sltiu_label_50:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.Assemble_sltiu_label_53:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 7

	sext.w      s3, a0
	mv          a4, s2
	mv          a3, s3
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUImm

	// *** Basic block 8

.Assemble_sltiu_label_73:
	j           .Assemble_sltiu_label_50
.func_end_Assemble_sltiu:
	.size Assemble_sltiu, .func_end_Assemble_sltiu-Assemble_sltiu

	.local  Assemble_xori
	.type Assemble_xori, @function

Assemble_xori:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleALUImm
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
	mv          s1, a0
	lla         a2, .str.234
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_xori_label_73

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .Assemble_xori_label_53

	// *** Basic block 4

	lla         a1, .str.235
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.Assemble_xori_label_50:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.Assemble_xori_label_53:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 7

	sext.w      s3, a0
	mv          a4, s2
	mv          a3, s3
	li          t0, 4		// 0x4 ASCII \x4
	mv          a2, t0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUImm

	// *** Basic block 8

.Assemble_xori_label_73:
	j           .Assemble_xori_label_50
.func_end_Assemble_xori:
	.size Assemble_xori, .func_end_Assemble_xori-Assemble_xori

	.local  Assemble_ori
	.type Assemble_ori, @function

Assemble_ori:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleALUImm
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
	mv          s1, a0
	lla         a2, .str.236
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_ori_label_73

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .Assemble_ori_label_53

	// *** Basic block 4

	lla         a1, .str.237
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.Assemble_ori_label_50:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.Assemble_ori_label_53:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 7

	sext.w      s3, a0
	mv          a4, s2
	mv          a3, s3
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUImm

	// *** Basic block 8

.Assemble_ori_label_73:
	j           .Assemble_ori_label_50
.func_end_Assemble_ori:
	.size Assemble_ori, .func_end_Assemble_ori-Assemble_ori

	.local  Assemble_andi
	.type Assemble_andi, @function

Assemble_andi:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleALUImm
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
	mv          s1, a0
	lla         a2, .str.238
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_andi_label_73

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .Assemble_andi_label_53

	// *** Basic block 4

	lla         a1, .str.239
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.Assemble_andi_label_50:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.Assemble_andi_label_53:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 7

	sext.w      s3, a0
	mv          a4, s2
	mv          a3, s3
	li          t0, 7		// 0x7 ASCII \x7
	mv          a2, t0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUImm

	// *** Basic block 8

.Assemble_andi_label_73:
	j           .Assemble_andi_label_50
.func_end_Assemble_andi:
	.size Assemble_andi, .func_end_Assemble_andi-Assemble_andi

	.local  Assemble_slli
	.type Assemble_slli, @function

Assemble_slli:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleALUReg
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
	lla         a2, .str.240
	addi        a3, s0, -32
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_slli_label_79

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .Assemble_slli_label_55

	// *** Basic block 4

	lla         a1, .str.241
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.Assemble_slli_label_52:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.Assemble_slli_label_55:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 7

	sext.w      s2, a0
	andi        s2, s2, 63
	addi        t0, s0, -32
	sw          s2, 8(t0)
	addi        a4, s0, -32
	mv          a3, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 8

.Assemble_slli_label_79:
	j           .Assemble_slli_label_52
.func_end_Assemble_slli:
	.size Assemble_slli, .func_end_Assemble_slli-Assemble_slli

	.local  Assemble_srli
	.type Assemble_srli, @function

Assemble_srli:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleALUReg
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
	lla         a2, .str.242
	addi        a3, s0, -32
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_srli_label_79

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .Assemble_srli_label_55

	// *** Basic block 4

	lla         a1, .str.243
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.Assemble_srli_label_52:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.Assemble_srli_label_55:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 7

	sext.w      s2, a0
	andi        s2, s2, 63
	addi        t0, s0, -32
	sw          s2, 8(t0)
	addi        a4, s0, -32
	mv          a3, x0
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 8

.Assemble_srli_label_79:
	j           .Assemble_srli_label_52
.func_end_Assemble_srli:
	.size Assemble_srli, .func_end_Assemble_srli-Assemble_srli

	.local  Assemble_srai
	.type Assemble_srai, @function

Assemble_srai:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleALUReg
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
	lla         a2, .str.244
	addi        a3, s0, -32
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_srai_label_80

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .Assemble_srai_label_55

	// *** Basic block 4

	lla         a1, .str.245
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.Assemble_srai_label_52:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.Assemble_srai_label_55:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 7

	sext.w      s2, a0
	andi        s2, s2, 63
	addi        t0, s0, -32
	sw          s2, 8(t0)
	addi        a4, s0, -32
	li          t0, 32		// 0x20 ASCII ' '
	mv          a3, t0
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 8

.Assemble_srai_label_80:
	j           .Assemble_srai_label_52
.func_end_Assemble_srai:
	.size Assemble_srai, .func_end_Assemble_srai-Assemble_srai

	.local  Assemble_add
	.type Assemble_add, @function

Assemble_add:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.246
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_add_label_41

	// *** Basic block 2

	mv          a4, s2
	mv          a3, x0
	mv          a2, x0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_add_label_41:
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
	.local AssembleALUReg
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
	lla         a2, .str.247
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_sub_label_43

	// *** Basic block 2

	mv          a4, s2
	li          t0, 32		// 0x20 ASCII ' '
	mv          a3, t0
	mv          a2, x0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_sub_label_43:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_sub:
	.size Assemble_sub, .func_end_Assemble_sub-Assemble_sub

	.local  Assemble_sll
	.type Assemble_sll, @function

Assemble_sll:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.248
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_sll_label_43

	// *** Basic block 2

	mv          a4, s2
	mv          a3, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_sll_label_43:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_sll:
	.size Assemble_sll, .func_end_Assemble_sll-Assemble_sll

	.local  Assemble_slt
	.type Assemble_slt, @function

Assemble_slt:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.249
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_slt_label_43

	// *** Basic block 2

	mv          a4, s2
	mv          a3, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_slt_label_43:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_slt:
	.size Assemble_slt, .func_end_Assemble_slt-Assemble_slt

	.local  Assemble_sltu
	.type Assemble_sltu, @function

Assemble_sltu:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.250
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_sltu_label_43

	// *** Basic block 2

	mv          a4, s2
	mv          a3, x0
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_sltu_label_43:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_sltu:
	.size Assemble_sltu, .func_end_Assemble_sltu-Assemble_sltu

	.local  Assemble_xor
	.type Assemble_xor, @function

Assemble_xor:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.251
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_xor_label_43

	// *** Basic block 2

	mv          a4, s2
	mv          a3, x0
	li          t0, 4		// 0x4 ASCII \x4
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_xor_label_43:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_xor:
	.size Assemble_xor, .func_end_Assemble_xor-Assemble_xor

	.local  Assemble_srl
	.type Assemble_srl, @function

Assemble_srl:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.252
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_srl_label_43

	// *** Basic block 2

	mv          a4, s2
	mv          a3, x0
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_srl_label_43:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_srl:
	.size Assemble_srl, .func_end_Assemble_srl-Assemble_srl

	.local  Assemble_sra
	.type Assemble_sra, @function

Assemble_sra:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.253
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_sra_label_45

	// *** Basic block 2

	mv          a4, s2
	li          t0, 32		// 0x20 ASCII ' '
	mv          a3, t0
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_sra_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_sra:
	.size Assemble_sra, .func_end_Assemble_sra-Assemble_sra

	.local  Assemble_or
	.type Assemble_or, @function

Assemble_or:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.254
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_or_label_43

	// *** Basic block 2

	mv          a4, s2
	mv          a3, x0
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_or_label_43:
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
	.local AssembleALUReg
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
	lla         a2, .str.255
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_and_label_43

	// *** Basic block 2

	mv          a4, s2
	mv          a3, x0
	li          t0, 7		// 0x7 ASCII \x7
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_and_label_43:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_and:
	.size Assemble_and, .func_end_Assemble_and-Assemble_and

	.local  Assemble_fence
	.type Assemble_fence, @function

Assemble_fence:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.256
	lla         a2, .str.257
	j           AssemblerError
.func_end_Assemble_fence:
	.size Assemble_fence, .func_end_Assemble_fence-Assemble_fence

	.local  Assemble_fence_i
	.type Assemble_fence_i, @function

Assemble_fence_i:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.258
	lla         a2, .str.259
	j           AssemblerError
.func_end_Assemble_fence_i:
	.size Assemble_fence_i, .func_end_Assemble_fence_i-Assemble_fence_i

	.local  Assemble_ecall
	.type Assemble_ecall, @function

Assemble_ecall:

	// *** Basic block 0

	.global AssemblerEmitWord
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	lw          a1, 744(t0)
	li          a2, 115		// 0x73 ASCII 's'
	j           AssemblerEmitWord
.func_end_Assemble_ecall:
	.size Assemble_ecall, .func_end_Assemble_ecall-Assemble_ecall

	.local  Assemble_ebreak
	.type Assemble_ebreak, @function

Assemble_ebreak:

	// *** Basic block 0

	.global AssemblerEmitWord
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	lw          a1, 744(t0)
	li          a2, 1048691		// 0x100073
	j           AssemblerEmitWord
.func_end_Assemble_ebreak:
	.size Assemble_ebreak, .func_end_Assemble_ebreak-Assemble_ebreak

	.local  Assemble_csrrw
	.type Assemble_csrrw, @function

Assemble_csrrw:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.260
	lla         a2, .str.261
	j           AssemblerError
.func_end_Assemble_csrrw:
	.size Assemble_csrrw, .func_end_Assemble_csrrw-Assemble_csrrw

	.local  Assemble_csrrs
	.type Assemble_csrrs, @function

Assemble_csrrs:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.262
	lla         a2, .str.263
	j           AssemblerError
.func_end_Assemble_csrrs:
	.size Assemble_csrrs, .func_end_Assemble_csrrs-Assemble_csrrs

	.local  Assemble_csrrc
	.type Assemble_csrrc, @function

Assemble_csrrc:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.264
	lla         a2, .str.265
	j           AssemblerError
.func_end_Assemble_csrrc:
	.size Assemble_csrrc, .func_end_Assemble_csrrc-Assemble_csrrc

	.local  Assemble_csrrwi
	.type Assemble_csrrwi, @function

Assemble_csrrwi:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.266
	lla         a2, .str.267
	j           AssemblerError
.func_end_Assemble_csrrwi:
	.size Assemble_csrrwi, .func_end_Assemble_csrrwi-Assemble_csrrwi

	.local  Assemble_csrrsi
	.type Assemble_csrrsi, @function

Assemble_csrrsi:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.268
	lla         a2, .str.269
	j           AssemblerError
.func_end_Assemble_csrrsi:
	.size Assemble_csrrsi, .func_end_Assemble_csrrsi-Assemble_csrrsi

	.local  Assemble_csrrci
	.type Assemble_csrrci, @function

Assemble_csrrci:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.270
	lla         a2, .str.271
	j           AssemblerError
.func_end_Assemble_csrrci:
	.size Assemble_csrrci, .func_end_Assemble_csrrci-Assemble_csrrci

	.local  Assemble_lwu
	.type Assemble_lwu, @function

Assemble_lwu:

	// *** Basic block 0

	.local AssembleLoadStoreInstruction
	// Leaf procedure, no stack frame generated
	li          a2, 6		// 0x6 ASCII \x6
	li          a1, 1		// 0x1 ASCII \x1
	j           AssembleLoadStoreInstruction
.func_end_Assemble_lwu:
	.size Assemble_lwu, .func_end_Assemble_lwu-Assemble_lwu

	.local  Assemble_ld
	.type Assemble_ld, @function

Assemble_ld:

	// *** Basic block 0

	.local AssembleLoadStoreInstruction
	// Leaf procedure, no stack frame generated
	li          a2, 3		// 0x3 ASCII \x3
	li          a1, 1		// 0x1 ASCII \x1
	j           AssembleLoadStoreInstruction
.func_end_Assemble_ld:
	.size Assemble_ld, .func_end_Assemble_ld-Assemble_ld

	.local  Assemble_sd
	.type Assemble_sd, @function

Assemble_sd:

	// *** Basic block 0

	.local AssembleLoadStoreInstruction
	// Leaf procedure, no stack frame generated
	li          a2, 3		// 0x3 ASCII \x3
	mv          a1, x0
	j           AssembleLoadStoreInstruction
.func_end_Assemble_sd:
	.size Assemble_sd, .func_end_Assemble_sd-Assemble_sd

	.local  Assemble_addiw
	.type Assemble_addiw, @function

Assemble_addiw:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleALUImm
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
	mv          s1, a0
	lla         a2, .str.272
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_addiw_label_71

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .Assemble_addiw_label_52

	// *** Basic block 4

	lla         a1, .str.273
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.Assemble_addiw_label_49:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.Assemble_addiw_label_52:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 7

	sext.w      s3, a0
	mv          a4, s2
	mv          a3, s3
	mv          a2, x0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUImm

	// *** Basic block 8

.Assemble_addiw_label_71:
	j           .Assemble_addiw_label_49
.func_end_Assemble_addiw:
	.size Assemble_addiw, .func_end_Assemble_addiw-Assemble_addiw

	.local  Assemble_slliw
	.type Assemble_slliw, @function

Assemble_slliw:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleALUReg
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
	lla         a2, .str.274
	addi        a3, s0, -32
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_slliw_label_79

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .Assemble_slliw_label_55

	// *** Basic block 4

	lla         a1, .str.275
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.Assemble_slliw_label_52:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.Assemble_slliw_label_55:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 7

	sext.w      s2, a0
	andi        s2, s2, 63
	addi        t0, s0, -32
	sw          s2, 8(t0)
	addi        a4, s0, -32
	mv          a3, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 8

.Assemble_slliw_label_79:
	j           .Assemble_slliw_label_52
.func_end_Assemble_slliw:
	.size Assemble_slliw, .func_end_Assemble_slliw-Assemble_slliw

	.local  Assemble_srliw
	.type Assemble_srliw, @function

Assemble_srliw:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleALUReg
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
	lla         a2, .str.276
	addi        a3, s0, -32
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_srliw_label_79

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .Assemble_srliw_label_55

	// *** Basic block 4

	lla         a1, .str.277
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.Assemble_srliw_label_52:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.Assemble_srliw_label_55:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 7

	sext.w      s2, a0
	andi        s2, s2, 63
	addi        t0, s0, -32
	sw          s2, 8(t0)
	addi        a4, s0, -32
	mv          a3, x0
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 8

.Assemble_srliw_label_79:
	j           .Assemble_srliw_label_52
.func_end_Assemble_srliw:
	.size Assemble_srliw, .func_end_Assemble_srliw-Assemble_srliw

	.local  Assemble_sraiw
	.type Assemble_sraiw, @function

Assemble_sraiw:

	// *** Basic block 0

	.local ParseRegisterPair
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleALUReg
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
	lla         a2, .str.278
	addi        a3, s0, -32
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_sraiw_label_80

	// *** Basic block 2

	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .Assemble_sraiw_label_55

	// *** Basic block 4

	lla         a1, .str.279
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 5

.Assemble_sraiw_label_52:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.Assemble_sraiw_label_55:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 7

	sext.w      s2, a0
	andi        s2, s2, 63
	addi        t0, s0, -32
	sw          s2, 8(t0)
	addi        a4, s0, -32
	li          t0, 32		// 0x20 ASCII ' '
	mv          a3, t0
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 8

.Assemble_sraiw_label_80:
	j           .Assemble_sraiw_label_52
.func_end_Assemble_sraiw:
	.size Assemble_sraiw, .func_end_Assemble_sraiw-Assemble_sraiw

	.local  Assemble_addw
	.type Assemble_addw, @function

Assemble_addw:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.280
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_addw_label_41

	// *** Basic block 2

	mv          a4, s2
	mv          a3, x0
	mv          a2, x0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_addw_label_41:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_addw:
	.size Assemble_addw, .func_end_Assemble_addw-Assemble_addw

	.local  Assemble_subw
	.type Assemble_subw, @function

Assemble_subw:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.281
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_subw_label_43

	// *** Basic block 2

	mv          a4, s2
	li          t0, 32		// 0x20 ASCII ' '
	mv          a3, t0
	mv          a2, x0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_subw_label_43:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_subw:
	.size Assemble_subw, .func_end_Assemble_subw-Assemble_subw

	.local  Assemble_sllw
	.type Assemble_sllw, @function

Assemble_sllw:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.282
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_sllw_label_43

	// *** Basic block 2

	mv          a4, s2
	mv          a3, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_sllw_label_43:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_sllw:
	.size Assemble_sllw, .func_end_Assemble_sllw-Assemble_sllw

	.local  Assemble_srlw
	.type Assemble_srlw, @function

Assemble_srlw:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.283
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_srlw_label_43

	// *** Basic block 2

	mv          a4, s2
	mv          a3, x0
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_srlw_label_43:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_srlw:
	.size Assemble_srlw, .func_end_Assemble_srlw-Assemble_srlw

	.local  Assemble_sraw
	.type Assemble_sraw, @function

Assemble_sraw:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.284
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_sraw_label_45

	// *** Basic block 2

	mv          a4, s2
	li          t0, 32		// 0x20 ASCII ' '
	mv          a3, t0
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_sraw_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_sraw:
	.size Assemble_sraw, .func_end_Assemble_sraw-Assemble_sraw

	.local  Assemble_mul
	.type Assemble_mul, @function

Assemble_mul:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.285
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_mul_label_43

	// *** Basic block 2

	mv          a4, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, x0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_mul_label_43:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_mul:
	.size Assemble_mul, .func_end_Assemble_mul-Assemble_mul

	.local  Assemble_mulh
	.type Assemble_mulh, @function

Assemble_mulh:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.286
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_mulh_label_44

	// *** Basic block 2

	mv          a4, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_mulh_label_44:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_mulh:
	.size Assemble_mulh, .func_end_Assemble_mulh-Assemble_mulh

	.local  Assemble_mulhsu
	.type Assemble_mulhsu, @function

Assemble_mulhsu:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.287
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_mulhsu_label_45

	// *** Basic block 2

	mv          a4, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_mulhsu_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_mulhsu:
	.size Assemble_mulhsu, .func_end_Assemble_mulhsu-Assemble_mulhsu

	.local  Assemble_mulhu
	.type Assemble_mulhu, @function

Assemble_mulhu:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.288
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_mulhu_label_45

	// *** Basic block 2

	mv          a4, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_mulhu_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_mulhu:
	.size Assemble_mulhu, .func_end_Assemble_mulhu-Assemble_mulhu

	.local  Assemble_div
	.type Assemble_div, @function

Assemble_div:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.289
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_div_label_45

	// *** Basic block 2

	mv          a4, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	li          t0, 4		// 0x4 ASCII \x4
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_div_label_45:
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
	.local AssembleALUReg
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
	lla         a2, .str.290
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_divu_label_45

	// *** Basic block 2

	mv          a4, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_divu_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_divu:
	.size Assemble_divu, .func_end_Assemble_divu-Assemble_divu

	.local  Assemble_rem
	.type Assemble_rem, @function

Assemble_rem:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.291
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_rem_label_45

	// *** Basic block 2

	mv          a4, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_rem_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_rem:
	.size Assemble_rem, .func_end_Assemble_rem-Assemble_rem

	.local  Assemble_remu
	.type Assemble_remu, @function

Assemble_remu:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.292
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_remu_label_45

	// *** Basic block 2

	mv          a4, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	li          t0, 7		// 0x7 ASCII \x7
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_remu_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_remu:
	.size Assemble_remu, .func_end_Assemble_remu-Assemble_remu

	.local  Assemble_mulw
	.type Assemble_mulw, @function

Assemble_mulw:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.293
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_mulw_label_43

	// *** Basic block 2

	mv          a4, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, x0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_mulw_label_43:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_mulw:
	.size Assemble_mulw, .func_end_Assemble_mulw-Assemble_mulw

	.local  Assemble_divw
	.type Assemble_divw, @function

Assemble_divw:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.294
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_divw_label_45

	// *** Basic block 2

	mv          a4, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	li          t0, 4		// 0x4 ASCII \x4
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_divw_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_divw:
	.size Assemble_divw, .func_end_Assemble_divw-Assemble_divw

	.local  Assemble_divuw
	.type Assemble_divuw, @function

Assemble_divuw:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.295
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_divuw_label_45

	// *** Basic block 2

	mv          a4, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_divuw_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_divuw:
	.size Assemble_divuw, .func_end_Assemble_divuw-Assemble_divuw

	.local  Assemble_remw
	.type Assemble_remw, @function

Assemble_remw:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.296
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_remw_label_45

	// *** Basic block 2

	mv          a4, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_remw_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_remw:
	.size Assemble_remw, .func_end_Assemble_remw-Assemble_remw

	.local  Assemble_remuw
	.type Assemble_remuw, @function

Assemble_remuw:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.297
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_remuw_label_45

	// *** Basic block 2

	mv          a4, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	li          t0, 7		// 0x7 ASCII \x7
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_remuw_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_remuw:
	.size Assemble_remuw, .func_end_Assemble_remuw-Assemble_remuw

	.local  AssembleFpLoadStore
	.type AssembleFpLoadStore, @function

AssembleFpLoadStore:

	// *** Basic block 0

	.global AssemblerEmitWord
	.local ITypeInstruction
	.local STypeInstruction
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
	mv          s2, a4
	mv          s3, a2
	mv          s4, a3
	beqz        a1, .AssembleFpLoadStore_label_51

	// *** Basic block 1

	lw          s5, 744(s1)
	lw          a1, 0(s2)
	lw          a2, 4(s2)
	mv          a4, s4
	mv          a3, s3
	li          t0, 7		// 0x7 ASCII \x7
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 2

	mv          a2, a0
	mv          a1, s5
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 3

	j           .AssembleFpLoadStore_label_75

	// *** Basic block 4

.AssembleFpLoadStore_label_51:
	lw          s5, 744(s1)
	lw          a1, 4(s2)
	lw          a2, 0(s2)
	mv          a4, s4
	mv          a3, s3
	li          t0, 39		// 0x27 ASCII '''
	mv          a0, t0
	call        STypeInstruction

	// *** Basic block 5

	mv          a2, a0
	mv          a1, s5
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
	j           AssemblerEmitWord

	// *** Basic block 6

.AssembleFpLoadStore_label_75:
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
.func_end_AssembleFpLoadStore:
	.size AssembleFpLoadStore, .func_end_AssembleFpLoadStore-AssembleFpLoadStore

	.local  RoundingMode
	.type RoundingMode, @function

RoundingMode:

	// *** Basic block 0

	.local rounding_modes
	.global strcmp
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
	mv          s1, a1
	mv          s2, a0
	mv          s3, x0
	la          t0, rounding_modes
	ld          t0, 0(t0)
	beq         t0, x0, .RoundingMode_label_53

	// *** Basic block 1

.RoundingMode_label_25:
	slli        t0, s3, 4
	la          t1, rounding_modes
	add         s4, t1, t0
	ld          a1, 0(s4)
	mv          a0, s1
	call        strcmp

	// *** Basic block 2

	bnez        a0, .RoundingMode_label_43

	// *** Basic block 3

	lw          a0, 8(s4)

	// *** Basic block 4

.RoundingMode_label_40:
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

.RoundingMode_label_43:

	// *** Basic block 6

.RoundingMode_label_44:
	addi        s3, s3, 1
	slli        t0, s3, 4
	la          t1, rounding_modes
	add         t0, t1, t0
	ld          t0, 0(t0)
	beq         t0, x0, .RoundingMode_label_25

	// *** Basic block 7

.RoundingMode_label_53:
	lla         a1, .str.304
	mv          a2, s1
	mv          a0, s2
	call        AssemblerError

	// *** Basic block 8

	mv          a0, x0
	j           .RoundingMode_label_40
.func_end_RoundingMode:
	.size RoundingMode, .func_end_RoundingMode-RoundingMode

	.local  Assemble_flw
	.type Assemble_flw, @function

Assemble_flw:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleFpLoadStore
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
	mv          s1, a0
	lla         a2, .str.305
	li          a1, 1		// 0x1 ASCII \x1
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .Assemble_flw_label_54

	// *** Basic block 3

	lla         a1, .str.306
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 4

.Assemble_flw_label_51:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.Assemble_flw_label_54:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 6

	sext.w      s2, a0
	addi        a0, s1, 176
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	not         t0, a0
	beqz        t0, .Assemble_flw_label_75

	// *** Basic block 8

	lla         a1, .str.307
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 9

	j           .Assemble_flw_label_51

	// *** Basic block 10

.Assemble_flw_label_75:
	addi        s3, s0, -32
	lla         a2, .str.308
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 11

	sw          a0, 4(s3)
	addi        a0, s1, 176
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 12

	not         t0, a0
	beqz        t0, .Assemble_flw_label_103

	// *** Basic block 13

	lla         a1, .str.309
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 14

	j           .Assemble_flw_label_51

	// *** Basic block 15

.Assemble_flw_label_103:
	addi        a4, s0, -32
	mv          a3, s2
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        AssembleFpLoadStore

	// *** Basic block 16

	j           .Assemble_flw_label_51
.func_end_Assemble_flw:
	.size Assemble_flw, .func_end_Assemble_flw-Assemble_flw

	.local  Assemble_fsw
	.type Assemble_fsw, @function

Assemble_fsw:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleFpLoadStore
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
	mv          s1, a0
	lla         a2, .str.310
	li          a1, 1		// 0x1 ASCII \x1
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .Assemble_fsw_label_54

	// *** Basic block 3

	lla         a1, .str.311
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 4

.Assemble_fsw_label_51:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.Assemble_fsw_label_54:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 6

	sext.w      s2, a0
	addi        a0, s1, 176
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	not         t0, a0
	beqz        t0, .Assemble_fsw_label_75

	// *** Basic block 8

	lla         a1, .str.312
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 9

	j           .Assemble_fsw_label_51

	// *** Basic block 10

.Assemble_fsw_label_75:
	addi        s3, s0, -32
	lla         a2, .str.313
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 11

	sw          a0, 4(s3)
	addi        a0, s1, 176
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 12

	not         t0, a0
	beqz        t0, .Assemble_fsw_label_103

	// *** Basic block 13

	lla         a1, .str.314
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 14

	j           .Assemble_fsw_label_51

	// *** Basic block 15

.Assemble_fsw_label_103:
	addi        a4, s0, -32
	mv          a3, s2
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        AssembleFpLoadStore

	// *** Basic block 16

	j           .Assemble_fsw_label_51
.func_end_Assemble_fsw:
	.size Assemble_fsw, .func_end_Assemble_fsw-Assemble_fsw

	.local  Assemble_fmadd_s
	.type Assemble_fmadd_s, @function

Assemble_fmadd_s:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.315
	lla         a2, .str.316
	j           AssemblerError
.func_end_Assemble_fmadd_s:
	.size Assemble_fmadd_s, .func_end_Assemble_fmadd_s-Assemble_fmadd_s

	.local  Assemble_fmsub_s
	.type Assemble_fmsub_s, @function

Assemble_fmsub_s:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.317
	lla         a2, .str.318
	j           AssemblerError
.func_end_Assemble_fmsub_s:
	.size Assemble_fmsub_s, .func_end_Assemble_fmsub_s-Assemble_fmsub_s

	.local  Assemble_fnmsub_s
	.type Assemble_fnmsub_s, @function

Assemble_fnmsub_s:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.319
	lla         a2, .str.320
	j           AssemblerError
.func_end_Assemble_fnmsub_s:
	.size Assemble_fnmsub_s, .func_end_Assemble_fnmsub_s-Assemble_fnmsub_s

	.local  Assemble_fnmadd_s
	.type Assemble_fnmadd_s, @function

Assemble_fnmadd_s:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.321
	lla         a2, .str.322
	j           AssemblerError
.func_end_Assemble_fnmadd_s:
	.size Assemble_fnmadd_s, .func_end_Assemble_fnmadd_s-Assemble_fnmadd_s

	.local  Assemble_fadd_s
	.type Assemble_fadd_s, @function

Assemble_fadd_s:

	// *** Basic block 0

	.local ParseRegisterTriple
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.323
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 1		// 0x1 ASCII \x1
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fadd_s_label_79

	// *** Basic block 2

	mv          s3, x0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .Assemble_fadd_s_label_65

	// *** Basic block 4

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fadd_s_label_64

	// *** Basic block 6

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 7

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 8

.Assemble_fadd_s_label_64:

	// *** Basic block 9

.Assemble_fadd_s_label_65:
	mv          a4, s2
	mv          a3, x0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 10

.Assemble_fadd_s_label_79:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fadd_s:
	.size Assemble_fadd_s, .func_end_Assemble_fadd_s-Assemble_fadd_s

	.local  Assemble_fsub_s
	.type Assemble_fsub_s, @function

Assemble_fsub_s:

	// *** Basic block 0

	.local ParseRegisterTriple
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.324
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 1		// 0x1 ASCII \x1
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fsub_s_label_81

	// *** Basic block 2

	mv          s3, x0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .Assemble_fsub_s_label_66

	// *** Basic block 4

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fsub_s_label_65

	// *** Basic block 6

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 7

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 8

.Assemble_fsub_s_label_65:

	// *** Basic block 9

.Assemble_fsub_s_label_66:
	mv          a4, s2
	li          t0, 4		// 0x4 ASCII \x4
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 10

.Assemble_fsub_s_label_81:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fsub_s:
	.size Assemble_fsub_s, .func_end_Assemble_fsub_s-Assemble_fsub_s

	.local  Assemble_fmul_s
	.type Assemble_fmul_s, @function

Assemble_fmul_s:

	// *** Basic block 0

	.local ParseRegisterTriple
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.325
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 1		// 0x1 ASCII \x1
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fmul_s_label_81

	// *** Basic block 2

	mv          s3, x0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .Assemble_fmul_s_label_66

	// *** Basic block 4

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fmul_s_label_65

	// *** Basic block 6

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 7

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 8

.Assemble_fmul_s_label_65:

	// *** Basic block 9

.Assemble_fmul_s_label_66:
	mv          a4, s2
	li          t0, 8		// 0x8 ASCII \x8
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 10

.Assemble_fmul_s_label_81:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fmul_s:
	.size Assemble_fmul_s, .func_end_Assemble_fmul_s-Assemble_fmul_s

	.local  Assemble_fdiv_s
	.type Assemble_fdiv_s, @function

Assemble_fdiv_s:

	// *** Basic block 0

	.local ParseRegisterTriple
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.326
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 1		// 0x1 ASCII \x1
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fdiv_s_label_81

	// *** Basic block 2

	mv          s3, x0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .Assemble_fdiv_s_label_66

	// *** Basic block 4

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fdiv_s_label_65

	// *** Basic block 6

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 7

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 8

.Assemble_fdiv_s_label_65:

	// *** Basic block 9

.Assemble_fdiv_s_label_66:
	mv          a4, s2
	li          t0, 12		// 0xc ASCII \xc
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 10

.Assemble_fdiv_s_label_81:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fdiv_s:
	.size Assemble_fdiv_s, .func_end_Assemble_fdiv_s-Assemble_fdiv_s

	.local  Assemble_fsqrt_s
	.type Assemble_fsqrt_s, @function

Assemble_fsqrt_s:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.327
	lla         a2, .str.328
	j           AssemblerError
.func_end_Assemble_fsqrt_s:
	.size Assemble_fsqrt_s, .func_end_Assemble_fsqrt_s-Assemble_fsqrt_s

	.local  Assemble_fsgnj_s
	.type Assemble_fsgnj_s, @function

Assemble_fsgnj_s:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.329
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 1		// 0x1 ASCII \x1
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fsgnj_s_label_44

	// *** Basic block 2

	mv          a4, s2
	li          t0, 16		// 0x10 ASCII \x10
	mv          a3, t0
	mv          a2, x0
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_fsgnj_s_label_44:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fsgnj_s:
	.size Assemble_fsgnj_s, .func_end_Assemble_fsgnj_s-Assemble_fsgnj_s

	.local  Assemble_fsgnjn_s
	.type Assemble_fsgnjn_s, @function

Assemble_fsgnjn_s:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.330
	addi        s2, s0, -32
	mv          a3, s2
	li          s3, 1		// 0x1 ASCII \x1
	mv          a1, s3
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fsgnjn_s_label_44

	// *** Basic block 2

	mv          a4, s2
	li          t0, 16		// 0x10 ASCII \x10
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_fsgnjn_s_label_44:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fsgnjn_s:
	.size Assemble_fsgnjn_s, .func_end_Assemble_fsgnjn_s-Assemble_fsgnjn_s

	.local  Assemble_fsgnjx_s
	.type Assemble_fsgnjx_s, @function

Assemble_fsgnjx_s:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.331
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 1		// 0x1 ASCII \x1
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fsgnjx_s_label_44

	// *** Basic block 2

	mv          a4, s2
	li          t0, 16		// 0x10 ASCII \x10
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_fsgnjx_s_label_44:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fsgnjx_s:
	.size Assemble_fsgnjx_s, .func_end_Assemble_fsgnjx_s-Assemble_fsgnjx_s

	.local  Assemble_fmin_s
	.type Assemble_fmin_s, @function

Assemble_fmin_s:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.332
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 1		// 0x1 ASCII \x1
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fmin_s_label_44

	// *** Basic block 2

	mv          a4, s2
	li          t0, 20		// 0x14 ASCII \x14
	mv          a3, t0
	mv          a2, x0
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_fmin_s_label_44:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fmin_s:
	.size Assemble_fmin_s, .func_end_Assemble_fmin_s-Assemble_fmin_s

	.local  Assemble_fmax_s
	.type Assemble_fmax_s, @function

Assemble_fmax_s:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.333
	addi        s2, s0, -32
	mv          a3, s2
	li          s3, 1		// 0x1 ASCII \x1
	mv          a1, s3
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fmax_s_label_44

	// *** Basic block 2

	mv          a4, s2
	li          t0, 20		// 0x14 ASCII \x14
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_fmax_s_label_44:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fmax_s:
	.size Assemble_fmax_s, .func_end_Assemble_fmax_s-Assemble_fmax_s

	.local  Assemble_fcvt_w_s
	.type Assemble_fcvt_w_s, @function

Assemble_fcvt_w_s:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.334
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_w_s_label_104

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.335
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	sw          x0, 8(t0)
	mv          s3, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_w_s_label_88

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_w_s_label_87

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_w_s_label_87:

	// *** Basic block 11

.Assemble_fcvt_w_s_label_88:
	addi        a4, s0, -32
	li          t0, 96		// 0x60 ASCII '`'
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_w_s_label_104:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_w_s:
	.size Assemble_fcvt_w_s, .func_end_Assemble_fcvt_w_s-Assemble_fcvt_w_s

	.local  Assemble_fcvt_wu_s
	.type Assemble_fcvt_wu_s, @function

Assemble_fcvt_wu_s:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	lla         a2, .str.336
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_wu_s_label_105

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.337
	li          s4, 1		// 0x1 ASCII \x1
	mv          a1, s4
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	sw          s4, 8(t0)
	mv          s3, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_wu_s_label_89

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_wu_s_label_88

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_wu_s_label_88:

	// *** Basic block 11

.Assemble_fcvt_wu_s_label_89:
	addi        a4, s0, -32
	li          t0, 96		// 0x60 ASCII '`'
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_wu_s_label_105:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_wu_s:
	.size Assemble_fcvt_wu_s, .func_end_Assemble_fcvt_wu_s-Assemble_fcvt_wu_s

	.local  Assemble_fmv_x_w
	.type Assemble_fmv_x_w, @function

Assemble_fmv_x_w:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALUReg
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	lla         a2, .str.338
	addi        a3, s0, -32
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	addi        t0, s0, -32
	sw          x0, 8(t0)
	addi        a4, s0, -32
	li          t0, 112		// 0x70 ASCII 'p'
	mv          a3, t0
	mv          a2, x0
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 2

	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fmv_x_w:
	.size Assemble_fmv_x_w, .func_end_Assemble_fmv_x_w-Assemble_fmv_x_w

	.local  Assemble_feq_s
	.type Assemble_feq_s, @function

Assemble_feq_s:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local ParseRegisterPair
	.local AssembleALUReg
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	lla         a2, .str.339
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_feq_s_label_68

	// *** Basic block 3

	lla         a2, .str.340
	addi        t0, s0, -32
	addi        a3, t0, 4
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        ParseRegisterPair

	// *** Basic block 4

	addi        a4, s0, -32
	li          t0, 80		// 0x50 ASCII 'P'
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 5

.Assemble_feq_s_label_68:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_feq_s:
	.size Assemble_feq_s, .func_end_Assemble_feq_s-Assemble_feq_s

	.local  Assemble_flt_s
	.type Assemble_flt_s, @function

Assemble_flt_s:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local ParseRegisterPair
	.local AssembleALUReg
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
	lla         a2, .str.341
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_flt_s_label_67

	// *** Basic block 3

	lla         a2, .str.342
	addi        t0, s0, -32
	addi        a3, t0, 4
	li          s2, 1		// 0x1 ASCII \x1
	mv          a1, s2
	mv          a0, s1
	call        ParseRegisterPair

	// *** Basic block 4

	addi        a4, s0, -32
	li          t0, 80		// 0x50 ASCII 'P'
	mv          a3, t0
	mv          a2, s2
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 5

.Assemble_flt_s_label_67:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_flt_s:
	.size Assemble_flt_s, .func_end_Assemble_flt_s-Assemble_flt_s

	.local  Assemble_fle_s
	.type Assemble_fle_s, @function

Assemble_fle_s:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local ParseRegisterPair
	.local AssembleALUReg
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	lla         a2, .str.343
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fle_s_label_66

	// *** Basic block 3

	lla         a2, .str.344
	addi        t0, s0, -32
	addi        a3, t0, 4
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        ParseRegisterPair

	// *** Basic block 4

	addi        a4, s0, -32
	li          t0, 80		// 0x50 ASCII 'P'
	mv          a3, t0
	mv          a2, x0
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 5

.Assemble_fle_s_label_66:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fle_s:
	.size Assemble_fle_s, .func_end_Assemble_fle_s-Assemble_fle_s

	.local  Assemble_fclass_s
	.type Assemble_fclass_s, @function

Assemble_fclass_s:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.345
	lla         a2, .str.346
	j           AssemblerError
.func_end_Assemble_fclass_s:
	.size Assemble_fclass_s, .func_end_Assemble_fclass_s-Assemble_fclass_s

	.local  Assemble_fcvt_s_w
	.type Assemble_fcvt_s_w, @function

Assemble_fcvt_s_w:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.347
	li          a1, 1		// 0x1 ASCII \x1
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_s_w_label_103

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.348
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	sw          x0, 8(t0)
	mv          s3, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_s_w_label_87

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_s_w_label_86

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_s_w_label_86:

	// *** Basic block 11

.Assemble_fcvt_s_w_label_87:
	addi        a4, s0, -32
	li          t0, 104		// 0x68 ASCII 'h'
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_s_w_label_103:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_s_w:
	.size Assemble_fcvt_s_w, .func_end_Assemble_fcvt_s_w-Assemble_fcvt_s_w

	.local  Assemble_fcvt_s_wu
	.type Assemble_fcvt_s_wu, @function

Assemble_fcvt_s_wu:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	lla         a2, .str.349
	li          s2, 1		// 0x1 ASCII \x1
	mv          a1, s2
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s3, 18		// 0x12 ASCII \x12
	mv          a1, s3
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_s_wu_label_105

	// *** Basic block 3

	addi        s4, s0, -32
	lla         a2, .str.350
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s4)
	addi        t0, s0, -32
	sw          s2, 8(t0)
	mv          s2, x0
	addi        a0, s1, 176
	mv          a1, s3
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_s_wu_label_89

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_s_wu_label_88

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s2, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_s_wu_label_88:

	// *** Basic block 11

.Assemble_fcvt_s_wu_label_89:
	addi        a4, s0, -32
	li          t0, 104		// 0x68 ASCII 'h'
	mv          a3, t0
	mv          a2, s2
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_s_wu_label_105:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_s_wu:
	.size Assemble_fcvt_s_wu, .func_end_Assemble_fcvt_s_wu-Assemble_fcvt_s_wu

	.local  Assemble_fmv_w_x
	.type Assemble_fmv_w_x, @function

Assemble_fmv_w_x:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.351
	li          a1, 1		// 0x1 ASCII \x1
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fmv_w_x_label_103

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.352
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	sw          x0, 8(t0)
	mv          s3, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fmv_w_x_label_87

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fmv_w_x_label_86

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fmv_w_x_label_86:

	// *** Basic block 11

.Assemble_fmv_w_x_label_87:
	addi        a4, s0, -32
	li          t0, 120		// 0x78 ASCII 'x'
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fmv_w_x_label_103:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fmv_w_x:
	.size Assemble_fmv_w_x, .func_end_Assemble_fmv_w_x-Assemble_fmv_w_x

	.local  Assemble_fcvt_l_s
	.type Assemble_fcvt_l_s, @function

Assemble_fcvt_l_s:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.353
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_l_s_label_105

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.354
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	li          t1, 2		// 0x2 ASCII \x2
	sw          t1, 8(t0)
	mv          s3, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_l_s_label_89

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_l_s_label_88

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_l_s_label_88:

	// *** Basic block 11

.Assemble_fcvt_l_s_label_89:
	addi        a4, s0, -32
	li          t0, 96		// 0x60 ASCII '`'
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_l_s_label_105:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_l_s:
	.size Assemble_fcvt_l_s, .func_end_Assemble_fcvt_l_s-Assemble_fcvt_l_s

	.local  Assemble_fcvt_lu_s
	.type Assemble_fcvt_lu_s, @function

Assemble_fcvt_lu_s:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	lla         a2, .str.355
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_lu_s_label_105

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.356
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	li          s3, 3		// 0x3 ASCII \x3
	sw          s3, 8(t0)
	mv          s4, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_lu_s_label_89

	// *** Basic block 6

	addi        a0, s1, 176
	mv          a1, s3
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_lu_s_label_88

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s4, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_lu_s_label_88:

	// *** Basic block 11

.Assemble_fcvt_lu_s_label_89:
	addi        a4, s0, -32
	li          t0, 96		// 0x60 ASCII '`'
	mv          a3, t0
	mv          a2, s4
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_lu_s_label_105:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_lu_s:
	.size Assemble_fcvt_lu_s, .func_end_Assemble_fcvt_lu_s-Assemble_fcvt_lu_s

	.local  Assemble_fcvt_s_l
	.type Assemble_fcvt_s_l, @function

Assemble_fcvt_s_l:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.357
	li          a1, 1		// 0x1 ASCII \x1
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_s_l_label_104

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.358
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	li          t1, 2		// 0x2 ASCII \x2
	sw          t1, 8(t0)
	mv          s3, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_s_l_label_88

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_s_l_label_87

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_s_l_label_87:

	// *** Basic block 11

.Assemble_fcvt_s_l_label_88:
	addi        a4, s0, -32
	li          t0, 104		// 0x68 ASCII 'h'
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_s_l_label_104:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_s_l:
	.size Assemble_fcvt_s_l, .func_end_Assemble_fcvt_s_l-Assemble_fcvt_s_l

	.local  Assemble_fcvt_s_lu
	.type Assemble_fcvt_s_lu, @function

Assemble_fcvt_s_lu:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	lla         a2, .str.359
	li          a1, 1		// 0x1 ASCII \x1
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_s_lu_label_104

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.360
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	li          s3, 3		// 0x3 ASCII \x3
	sw          s3, 8(t0)
	mv          s4, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_s_lu_label_88

	// *** Basic block 6

	addi        a0, s1, 176
	mv          a1, s3
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_s_lu_label_87

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s4, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_s_lu_label_87:

	// *** Basic block 11

.Assemble_fcvt_s_lu_label_88:
	addi        a4, s0, -32
	li          t0, 104		// 0x68 ASCII 'h'
	mv          a3, t0
	mv          a2, s4
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_s_lu_label_104:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_s_lu:
	.size Assemble_fcvt_s_lu, .func_end_Assemble_fcvt_s_lu-Assemble_fcvt_s_lu

	.local  Assemble_fld
	.type Assemble_fld, @function

Assemble_fld:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleFpLoadStore
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
	mv          s1, a0
	lla         a2, .str.361
	li          a1, 1		// 0x1 ASCII \x1
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .Assemble_fld_label_54

	// *** Basic block 3

	lla         a1, .str.362
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 4

.Assemble_fld_label_51:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.Assemble_fld_label_54:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 6

	sext.w      s2, a0
	addi        a0, s1, 176
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	not         t0, a0
	beqz        t0, .Assemble_fld_label_75

	// *** Basic block 8

	lla         a1, .str.363
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 9

	j           .Assemble_fld_label_51

	// *** Basic block 10

.Assemble_fld_label_75:
	addi        s3, s0, -32
	lla         a2, .str.364
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 11

	sw          a0, 4(s3)
	addi        a0, s1, 176
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 12

	not         t0, a0
	beqz        t0, .Assemble_fld_label_103

	// *** Basic block 13

	lla         a1, .str.365
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 14

	j           .Assemble_fld_label_51

	// *** Basic block 15

.Assemble_fld_label_103:
	addi        a4, s0, -32
	mv          a3, s2
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        AssembleFpLoadStore

	// *** Basic block 16

	j           .Assemble_fld_label_51
.func_end_Assemble_fld:
	.size Assemble_fld, .func_end_Assemble_fld-Assemble_fld

	.local  Assemble_fsd
	.type Assemble_fsd, @function

Assemble_fsd:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.local AssembleFpLoadStore
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
	mv          s1, a0
	lla         a2, .str.366
	li          a1, 1		// 0x1 ASCII \x1
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .Assemble_fsd_label_54

	// *** Basic block 3

	lla         a1, .str.367
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 4

.Assemble_fsd_label_51:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.Assemble_fsd_label_54:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 6

	sext.w      s2, a0
	addi        a0, s1, 176
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	not         t0, a0
	beqz        t0, .Assemble_fsd_label_75

	// *** Basic block 8

	lla         a1, .str.368
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 9

	j           .Assemble_fsd_label_51

	// *** Basic block 10

.Assemble_fsd_label_75:
	addi        s3, s0, -32
	lla         a2, .str.369
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 11

	sw          a0, 4(s3)
	addi        a0, s1, 176
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	call        LexMatch

	// *** Basic block 12

	not         t0, a0
	beqz        t0, .Assemble_fsd_label_103

	// *** Basic block 13

	lla         a1, .str.370
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 14

	j           .Assemble_fsd_label_51

	// *** Basic block 15

.Assemble_fsd_label_103:
	addi        a4, s0, -32
	mv          a3, s2
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        AssembleFpLoadStore

	// *** Basic block 16

	j           .Assemble_fsd_label_51
.func_end_Assemble_fsd:
	.size Assemble_fsd, .func_end_Assemble_fsd-Assemble_fsd

	.local  Assemble_fmadd_d
	.type Assemble_fmadd_d, @function

Assemble_fmadd_d:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.371
	lla         a2, .str.372
	j           AssemblerError
.func_end_Assemble_fmadd_d:
	.size Assemble_fmadd_d, .func_end_Assemble_fmadd_d-Assemble_fmadd_d

	.local  Assemble_fmsub_d
	.type Assemble_fmsub_d, @function

Assemble_fmsub_d:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.373
	lla         a2, .str.374
	j           AssemblerError
.func_end_Assemble_fmsub_d:
	.size Assemble_fmsub_d, .func_end_Assemble_fmsub_d-Assemble_fmsub_d

	.local  Assemble_fnmsub_d
	.type Assemble_fnmsub_d, @function

Assemble_fnmsub_d:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.375
	lla         a2, .str.376
	j           AssemblerError
.func_end_Assemble_fnmsub_d:
	.size Assemble_fnmsub_d, .func_end_Assemble_fnmsub_d-Assemble_fnmsub_d

	.local  Assemble_fnmadd_d
	.type Assemble_fnmadd_d, @function

Assemble_fnmadd_d:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.377
	lla         a2, .str.378
	j           AssemblerError
.func_end_Assemble_fnmadd_d:
	.size Assemble_fnmadd_d, .func_end_Assemble_fnmadd_d-Assemble_fnmadd_d

	.local  Assemble_fadd_d
	.type Assemble_fadd_d, @function

Assemble_fadd_d:

	// *** Basic block 0

	.local ParseRegisterTriple
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	lla         a2, .str.379
	addi        s2, s0, -32
	mv          a3, s2
	li          s3, 1		// 0x1 ASCII \x1
	mv          a1, s3
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fadd_d_label_81

	// *** Basic block 2

	mv          s4, x0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .Assemble_fadd_d_label_66

	// *** Basic block 4

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fadd_d_label_65

	// *** Basic block 6

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 7

	mv          s4, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 8

.Assemble_fadd_d_label_65:

	// *** Basic block 9

.Assemble_fadd_d_label_66:
	mv          a4, s2
	mv          a3, s3
	mv          a2, s4
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 10

.Assemble_fadd_d_label_81:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fadd_d:
	.size Assemble_fadd_d, .func_end_Assemble_fadd_d-Assemble_fadd_d

	.local  Assemble_fsub_d
	.type Assemble_fsub_d, @function

Assemble_fsub_d:

	// *** Basic block 0

	.local ParseRegisterTriple
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.380
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 1		// 0x1 ASCII \x1
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fsub_d_label_81

	// *** Basic block 2

	mv          s3, x0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .Assemble_fsub_d_label_66

	// *** Basic block 4

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fsub_d_label_65

	// *** Basic block 6

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 7

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 8

.Assemble_fsub_d_label_65:

	// *** Basic block 9

.Assemble_fsub_d_label_66:
	mv          a4, s2
	li          t0, 5		// 0x5 ASCII \x5
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 10

.Assemble_fsub_d_label_81:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fsub_d:
	.size Assemble_fsub_d, .func_end_Assemble_fsub_d-Assemble_fsub_d

	.local  Assemble_fmul_d
	.type Assemble_fmul_d, @function

Assemble_fmul_d:

	// *** Basic block 0

	.local ParseRegisterTriple
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.381
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 1		// 0x1 ASCII \x1
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fmul_d_label_81

	// *** Basic block 2

	mv          s3, x0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .Assemble_fmul_d_label_66

	// *** Basic block 4

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fmul_d_label_65

	// *** Basic block 6

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 7

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 8

.Assemble_fmul_d_label_65:

	// *** Basic block 9

.Assemble_fmul_d_label_66:
	mv          a4, s2
	li          t0, 9		// 0x9 ASCII \x9
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 10

.Assemble_fmul_d_label_81:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fmul_d:
	.size Assemble_fmul_d, .func_end_Assemble_fmul_d-Assemble_fmul_d

	.local  Assemble_fdiv_d
	.type Assemble_fdiv_d, @function

Assemble_fdiv_d:

	// *** Basic block 0

	.local ParseRegisterTriple
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.382
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 1		// 0x1 ASCII \x1
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fdiv_d_label_81

	// *** Basic block 2

	mv          s3, x0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .Assemble_fdiv_d_label_66

	// *** Basic block 4

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fdiv_d_label_65

	// *** Basic block 6

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 7

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 8

.Assemble_fdiv_d_label_65:

	// *** Basic block 9

.Assemble_fdiv_d_label_66:
	mv          a4, s2
	li          t0, 13		// 0xd ASCII \xd
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 10

.Assemble_fdiv_d_label_81:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fdiv_d:
	.size Assemble_fdiv_d, .func_end_Assemble_fdiv_d-Assemble_fdiv_d

	.local  Assemble_fsqrt_d
	.type Assemble_fsqrt_d, @function

Assemble_fsqrt_d:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.383
	lla         a2, .str.384
	j           AssemblerError
.func_end_Assemble_fsqrt_d:
	.size Assemble_fsqrt_d, .func_end_Assemble_fsqrt_d-Assemble_fsqrt_d

	.local  Assemble_fsgnj_d
	.type Assemble_fsgnj_d, @function

Assemble_fsgnj_d:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.385
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 1		// 0x1 ASCII \x1
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fsgnj_d_label_44

	// *** Basic block 2

	mv          a4, s2
	li          t0, 17		// 0x11 ASCII \x11
	mv          a3, t0
	mv          a2, x0
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_fsgnj_d_label_44:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fsgnj_d:
	.size Assemble_fsgnj_d, .func_end_Assemble_fsgnj_d-Assemble_fsgnj_d

	.local  Assemble_fsgnjn_d
	.type Assemble_fsgnjn_d, @function

Assemble_fsgnjn_d:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.386
	addi        s2, s0, -32
	mv          a3, s2
	li          s3, 1		// 0x1 ASCII \x1
	mv          a1, s3
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fsgnjn_d_label_44

	// *** Basic block 2

	mv          a4, s2
	li          t0, 17		// 0x11 ASCII \x11
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_fsgnjn_d_label_44:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fsgnjn_d:
	.size Assemble_fsgnjn_d, .func_end_Assemble_fsgnjn_d-Assemble_fsgnjn_d

	.local  Assemble_fsgnjx_d
	.type Assemble_fsgnjx_d, @function

Assemble_fsgnjx_d:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.387
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 1		// 0x1 ASCII \x1
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fsgnjx_d_label_44

	// *** Basic block 2

	mv          a4, s2
	li          t0, 17		// 0x11 ASCII \x11
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_fsgnjx_d_label_44:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fsgnjx_d:
	.size Assemble_fsgnjx_d, .func_end_Assemble_fsgnjx_d-Assemble_fsgnjx_d

	.local  Assemble_fmin_d
	.type Assemble_fmin_d, @function

Assemble_fmin_d:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	lla         a2, .str.388
	addi        s2, s0, -32
	mv          a3, s2
	li          a1, 1		// 0x1 ASCII \x1
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fmin_d_label_44

	// *** Basic block 2

	mv          a4, s2
	li          t0, 21		// 0x15 ASCII \x15
	mv          a3, t0
	mv          a2, x0
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_fmin_d_label_44:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fmin_d:
	.size Assemble_fmin_d, .func_end_Assemble_fmin_d-Assemble_fmin_d

	.local  Assemble_fmax_d
	.type Assemble_fmax_d, @function

Assemble_fmax_d:

	// *** Basic block 0

	.local ParseRegisterTriple
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.389
	addi        s2, s0, -32
	mv          a3, s2
	li          s3, 1		// 0x1 ASCII \x1
	mv          a1, s3
	call        ParseRegisterTriple

	// *** Basic block 1

	beqz        a0, .Assemble_fmax_d_label_44

	// *** Basic block 2

	mv          a4, s2
	li          t0, 21		// 0x15 ASCII \x15
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_fmax_d_label_44:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fmax_d:
	.size Assemble_fmax_d, .func_end_Assemble_fmax_d-Assemble_fmax_d

	.local  Assemble_fcvt_s_d
	.type Assemble_fcvt_s_d, @function

Assemble_fcvt_s_d:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	lla         a2, .str.390
	li          s2, 1		// 0x1 ASCII \x1
	mv          a1, s2
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s3, 18		// 0x12 ASCII \x12
	mv          a1, s3
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_s_d_label_106

	// *** Basic block 3

	addi        s4, s0, -32
	lla         a2, .str.391
	mv          a1, s2
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s4)
	addi        t0, s0, -32
	sw          s2, 8(t0)
	mv          s2, x0
	addi        a0, s1, 176
	mv          a1, s3
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_s_d_label_90

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_s_d_label_89

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s2, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_s_d_label_89:

	// *** Basic block 11

.Assemble_fcvt_s_d_label_90:
	addi        a4, s0, -32
	li          t0, 32		// 0x20 ASCII ' '
	mv          a3, t0
	mv          a2, s2
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_s_d_label_106:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_s_d:
	.size Assemble_fcvt_s_d, .func_end_Assemble_fcvt_s_d-Assemble_fcvt_s_d

	.local  Assemble_fcvt_d_s
	.type Assemble_fcvt_d_s, @function

Assemble_fcvt_d_s:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	lla         a2, .str.392
	li          s2, 1		// 0x1 ASCII \x1
	mv          a1, s2
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s3, 18		// 0x12 ASCII \x12
	mv          a1, s3
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_d_s_label_105

	// *** Basic block 3

	addi        s4, s0, -32
	lla         a2, .str.393
	mv          a1, s2
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s4)
	addi        t0, s0, -32
	sw          x0, 8(t0)
	mv          s2, x0
	addi        a0, s1, 176
	mv          a1, s3
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_d_s_label_89

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_d_s_label_88

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s2, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_d_s_label_88:

	// *** Basic block 11

.Assemble_fcvt_d_s_label_89:
	addi        a4, s0, -32
	li          t0, 33		// 0x21 ASCII '!'
	mv          a3, t0
	mv          a2, s2
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_d_s_label_105:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_d_s:
	.size Assemble_fcvt_d_s, .func_end_Assemble_fcvt_d_s-Assemble_fcvt_d_s

	.local  Assemble_feq_d
	.type Assemble_feq_d, @function

Assemble_feq_d:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local ParseRegisterPair
	.local AssembleALUReg
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	lla         a2, .str.394
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_feq_d_label_68

	// *** Basic block 3

	lla         a2, .str.395
	addi        t0, s0, -32
	addi        a3, t0, 4
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        ParseRegisterPair

	// *** Basic block 4

	addi        a4, s0, -32
	li          t0, 81		// 0x51 ASCII 'Q'
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 5

.Assemble_feq_d_label_68:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_feq_d:
	.size Assemble_feq_d, .func_end_Assemble_feq_d-Assemble_feq_d

	.local  Assemble_flt_d
	.type Assemble_flt_d, @function

Assemble_flt_d:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local ParseRegisterPair
	.local AssembleALUReg
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
	lla         a2, .str.396
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_flt_d_label_67

	// *** Basic block 3

	lla         a2, .str.397
	addi        t0, s0, -32
	addi        a3, t0, 4
	li          s2, 1		// 0x1 ASCII \x1
	mv          a1, s2
	mv          a0, s1
	call        ParseRegisterPair

	// *** Basic block 4

	addi        a4, s0, -32
	li          t0, 81		// 0x51 ASCII 'Q'
	mv          a3, t0
	mv          a2, s2
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 5

.Assemble_flt_d_label_67:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_flt_d:
	.size Assemble_flt_d, .func_end_Assemble_flt_d-Assemble_flt_d

	.local  Assemble_fle_d
	.type Assemble_fle_d, @function

Assemble_fle_d:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local ParseRegisterPair
	.local AssembleALUReg
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	lla         a2, .str.398
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fle_d_label_66

	// *** Basic block 3

	lla         a2, .str.399
	addi        t0, s0, -32
	addi        a3, t0, 4
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        ParseRegisterPair

	// *** Basic block 4

	addi        a4, s0, -32
	li          t0, 81		// 0x51 ASCII 'Q'
	mv          a3, t0
	mv          a2, x0
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 5

.Assemble_fle_d_label_66:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fle_d:
	.size Assemble_fle_d, .func_end_Assemble_fle_d-Assemble_fle_d

	.local  Assemble_fclass_d
	.type Assemble_fclass_d, @function

Assemble_fclass_d:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.400
	lla         a2, .str.401
	j           AssemblerError
.func_end_Assemble_fclass_d:
	.size Assemble_fclass_d, .func_end_Assemble_fclass_d-Assemble_fclass_d

	.local  Assemble_fcvt_w_d
	.type Assemble_fcvt_w_d, @function

Assemble_fcvt_w_d:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.402
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_w_d_label_104

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.403
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	sw          x0, 8(t0)
	mv          s3, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_w_d_label_88

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_w_d_label_87

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_w_d_label_87:

	// *** Basic block 11

.Assemble_fcvt_w_d_label_88:
	addi        a4, s0, -32
	li          t0, 97		// 0x61 ASCII 'a'
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_w_d_label_104:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_w_d:
	.size Assemble_fcvt_w_d, .func_end_Assemble_fcvt_w_d-Assemble_fcvt_w_d

	.local  Assemble_fcvt_wu_d
	.type Assemble_fcvt_wu_d, @function

Assemble_fcvt_wu_d:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	lla         a2, .str.404
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_wu_d_label_105

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.405
	li          s4, 1		// 0x1 ASCII \x1
	mv          a1, s4
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	sw          s4, 8(t0)
	mv          s3, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_wu_d_label_89

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_wu_d_label_88

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_wu_d_label_88:

	// *** Basic block 11

.Assemble_fcvt_wu_d_label_89:
	addi        a4, s0, -32
	li          t0, 97		// 0x61 ASCII 'a'
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_wu_d_label_105:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_wu_d:
	.size Assemble_fcvt_wu_d, .func_end_Assemble_fcvt_wu_d-Assemble_fcvt_wu_d

	.local  Assemble_fcvt_d_w
	.type Assemble_fcvt_d_w, @function

Assemble_fcvt_d_w:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.406
	li          a1, 1		// 0x1 ASCII \x1
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_d_w_label_103

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.407
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	sw          x0, 8(t0)
	mv          s3, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_d_w_label_87

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_d_w_label_86

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_d_w_label_86:

	// *** Basic block 11

.Assemble_fcvt_d_w_label_87:
	addi        a4, s0, -32
	li          t0, 105		// 0x69 ASCII 'i'
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_d_w_label_103:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_d_w:
	.size Assemble_fcvt_d_w, .func_end_Assemble_fcvt_d_w-Assemble_fcvt_d_w

	.local  Assemble_fcvt_d_wu
	.type Assemble_fcvt_d_wu, @function

Assemble_fcvt_d_wu:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	lla         a2, .str.408
	li          s2, 1		// 0x1 ASCII \x1
	mv          a1, s2
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s3, 18		// 0x12 ASCII \x12
	mv          a1, s3
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_d_wu_label_105

	// *** Basic block 3

	addi        s4, s0, -32
	lla         a2, .str.409
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s4)
	addi        t0, s0, -32
	sw          s2, 8(t0)
	mv          s2, x0
	addi        a0, s1, 176
	mv          a1, s3
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_d_wu_label_89

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_d_wu_label_88

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s2, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_d_wu_label_88:

	// *** Basic block 11

.Assemble_fcvt_d_wu_label_89:
	addi        a4, s0, -32
	li          t0, 105		// 0x69 ASCII 'i'
	mv          a3, t0
	mv          a2, s2
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_d_wu_label_105:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_d_wu:
	.size Assemble_fcvt_d_wu, .func_end_Assemble_fcvt_d_wu-Assemble_fcvt_d_wu

	.local  Assemble_fcvt_l_d
	.type Assemble_fcvt_l_d, @function

Assemble_fcvt_l_d:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.410
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_l_d_label_105

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.411
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	li          t1, 2		// 0x2 ASCII \x2
	sw          t1, 8(t0)
	mv          s3, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_l_d_label_89

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_l_d_label_88

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_l_d_label_88:

	// *** Basic block 11

.Assemble_fcvt_l_d_label_89:
	addi        a4, s0, -32
	li          t0, 97		// 0x61 ASCII 'a'
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_l_d_label_105:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_l_d:
	.size Assemble_fcvt_l_d, .func_end_Assemble_fcvt_l_d-Assemble_fcvt_l_d

	.local  Assemble_fcvt_lu_d
	.type Assemble_fcvt_lu_d, @function

Assemble_fcvt_lu_d:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	lla         a2, .str.412
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_lu_d_label_105

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.413
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	li          s3, 3		// 0x3 ASCII \x3
	sw          s3, 8(t0)
	mv          s4, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_lu_d_label_89

	// *** Basic block 6

	addi        a0, s1, 176
	mv          a1, s3
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_lu_d_label_88

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s4, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_lu_d_label_88:

	// *** Basic block 11

.Assemble_fcvt_lu_d_label_89:
	addi        a4, s0, -32
	li          t0, 97		// 0x61 ASCII 'a'
	mv          a3, t0
	mv          a2, s4
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_lu_d_label_105:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_lu_d:
	.size Assemble_fcvt_lu_d, .func_end_Assemble_fcvt_lu_d-Assemble_fcvt_lu_d

	.local  Assemble_fmv_x_d
	.type Assemble_fmv_x_d, @function

Assemble_fmv_x_d:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.414
	mv          a1, x0
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fmv_x_d_label_104

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.415
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	sw          x0, 8(t0)
	mv          s3, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fmv_x_d_label_88

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fmv_x_d_label_87

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fmv_x_d_label_87:

	// *** Basic block 11

.Assemble_fmv_x_d_label_88:
	addi        a4, s0, -32
	li          t0, 113		// 0x71 ASCII 'q'
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fmv_x_d_label_104:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fmv_x_d:
	.size Assemble_fmv_x_d, .func_end_Assemble_fmv_x_d-Assemble_fmv_x_d

	.local  Assemble_fcvt_d_l
	.type Assemble_fcvt_d_l, @function

Assemble_fcvt_d_l:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.416
	li          a1, 1		// 0x1 ASCII \x1
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_d_l_label_104

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.417
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	li          t1, 2		// 0x2 ASCII \x2
	sw          t1, 8(t0)
	mv          s3, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_d_l_label_88

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_d_l_label_87

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_d_l_label_87:

	// *** Basic block 11

.Assemble_fcvt_d_l_label_88:
	addi        a4, s0, -32
	li          t0, 105		// 0x69 ASCII 'i'
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_d_l_label_104:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_d_l:
	.size Assemble_fcvt_d_l, .func_end_Assemble_fcvt_d_l-Assemble_fcvt_d_l

	.local  Assemble_fcvt_d_lu
	.type Assemble_fcvt_d_lu, @function

Assemble_fcvt_d_lu:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	lla         a2, .str.418
	li          a1, 1		// 0x1 ASCII \x1
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fcvt_d_lu_label_104

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.419
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	li          s3, 3		// 0x3 ASCII \x3
	sw          s3, 8(t0)
	mv          s4, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fcvt_d_lu_label_88

	// *** Basic block 6

	addi        a0, s1, 176
	mv          a1, s3
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fcvt_d_lu_label_87

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s4, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fcvt_d_lu_label_87:

	// *** Basic block 11

.Assemble_fcvt_d_lu_label_88:
	addi        a4, s0, -32
	li          t0, 105		// 0x69 ASCII 'i'
	mv          a3, t0
	mv          a2, s4
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fcvt_d_lu_label_104:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fcvt_d_lu:
	.size Assemble_fcvt_d_lu, .func_end_Assemble_fcvt_d_lu-Assemble_fcvt_d_lu

	.local  Assemble_fmv_d_x
	.type Assemble_fmv_d_x, @function

Assemble_fmv_d_x:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.local RoundingMode
	.global LexNextToken
	.local AssembleALUReg
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
	mv          s1, a0
	lla         a2, .str.420
	li          a1, 1		// 0x1 ASCII \x1
	call        Register

	// *** Basic block 1

	sw          a0, -32(s0)
	addi        a0, s1, 176
	li          s2, 18		// 0x12 ASCII \x12
	mv          a1, s2
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .Assemble_fmv_d_x_label_103

	// *** Basic block 3

	addi        s3, s0, -32
	lla         a2, .str.421
	mv          a1, x0
	mv          a0, s1
	call        Register

	// *** Basic block 4

	sw          a0, 4(s3)
	addi        t0, s0, -32
	sw          x0, 8(t0)
	mv          s3, x0
	addi        a0, s1, 176
	mv          a1, s2
	call        LexMatch

	// *** Basic block 5

	beqz        a0, .Assemble_fmv_d_x_label_87

	// *** Basic block 6

	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .Assemble_fmv_d_x_label_86

	// *** Basic block 8

	addi        t0, a0, 80
	ld          a1, 16(t0)
	mv          a0, s1
	call        RoundingMode

	// *** Basic block 9

	mv          s3, a0
	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 10

.Assemble_fmv_d_x_label_86:

	// *** Basic block 11

.Assemble_fmv_d_x_label_87:
	addi        a4, s0, -32
	li          t0, 121		// 0x79 ASCII 'y'
	mv          a3, t0
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 12

.Assemble_fmv_d_x_label_103:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fmv_d_x:
	.size Assemble_fmv_d_x, .func_end_Assemble_fmv_d_x-Assemble_fmv_d_x

	.local  Assemble_nop
	.type Assemble_nop, @function

Assemble_nop:

	// *** Basic block 0

	.local AssembleALUImm
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -32(s0)
	// End of stack frame
	sd          x0, -32(s0)
	sw          x0, -32(s0)
	addi        t0, s0, -32
	sw          x0, 4(t0)
	addi        a4, s0, -32
	mv          a3, x0
	mv          a2, x0
	li          a1, 19		// 0x13 ASCII \x13
	call        AssembleALUImm

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_nop:
	.size Assemble_nop, .func_end_Assemble_nop-Assemble_nop

	.local  Assemble_not
	.type Assemble_not, @function

Assemble_not:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALUImm
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
	lla         a2, .str.422
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_not_label_45

	// *** Basic block 2

	mv          a4, s2
	li          t0, -1		// 0xffffffffffffffff
	mv          a3, t0
	li          t0, 4		// 0x4 ASCII \x4
	mv          a2, t0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUImm

	// *** Basic block 3

.Assemble_not_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_not:
	.size Assemble_not, .func_end_Assemble_not-Assemble_not

	.local  Assemble_neg
	.type Assemble_neg, @function

Assemble_neg:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALUReg
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	lla         a2, .str.423
	addi        a3, s0, -32
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_neg_label_55

	// *** Basic block 2

	addi        t0, s0, -32
	addi        t1, s0, -32
	lw          t1, 4(t1)
	sw          t1, 8(t0)
	addi        t0, s0, -32
	sw          x0, 4(t0)
	addi        a4, s0, -32
	li          t0, 32		// 0x20 ASCII ' '
	mv          a3, t0
	mv          a2, x0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_neg_label_55:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_neg:
	.size Assemble_neg, .func_end_Assemble_neg-Assemble_neg

	.local  Assemble_fneg_s
	.type Assemble_fneg_s, @function

Assemble_fneg_s:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALUReg
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
	lla         a2, .str.424
	addi        a3, s0, -32
	li          s2, 1		// 0x1 ASCII \x1
	mv          a1, s2
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_fneg_s_label_53

	// *** Basic block 2

	addi        t0, s0, -32
	addi        t1, s0, -32
	lw          t1, 4(t1)
	sw          t1, 8(t0)
	addi        a4, s0, -32
	li          t0, 16		// 0x10 ASCII \x10
	mv          a3, t0
	mv          a2, s2
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_fneg_s_label_53:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fneg_s:
	.size Assemble_fneg_s, .func_end_Assemble_fneg_s-Assemble_fneg_s

	.local  Assemble_fneg_d
	.type Assemble_fneg_d, @function

Assemble_fneg_d:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALUReg
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
	lla         a2, .str.425
	addi        a3, s0, -32
	li          s2, 1		// 0x1 ASCII \x1
	mv          a1, s2
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_fneg_d_label_53

	// *** Basic block 2

	addi        t0, s0, -32
	addi        t1, s0, -32
	lw          t1, 4(t1)
	sw          t1, 8(t0)
	addi        a4, s0, -32
	li          t0, 17		// 0x11 ASCII \x11
	mv          a3, t0
	mv          a2, s2
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_fneg_d_label_53:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_fneg_d:
	.size Assemble_fneg_d, .func_end_Assemble_fneg_d-Assemble_fneg_d

	.local  Assemble_li
	.type Assemble_li, @function

Assemble_li:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global LexLookingAt
	.global StringInit
	.global LexNextToken
	.local GetOrCreateSymbol
	.local AssembleUType
	.global NewAssemblerRelocation
	.global AssemblerCurrentAddress
	.global AssemblerAddRelocation
	.global AssemblerEmitWord
	.local ITypeInstruction
	.global StringDestruct
	.global AssemblerEvaluateExpression
	.local AssembleLoadImmediateConstant
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
	lla         a2, .str.426
	mv          a1, x0
	call        Register

	// *** Basic block 1

	mv          s2, a0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .Assemble_li_label_66

	// *** Basic block 3

	lla         a1, .str.427
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 4

.Assemble_li_label_63:
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

.Assemble_li_label_66:
	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 6

	beqz        a0, .Assemble_li_label_163

	// *** Basic block 7

	addi        a0, s0, -64
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 8

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 9

	addi        t0, s0, -64
	ld          a1, 16(t0)
	mv          a0, s1
	call        GetOrCreateSymbol

	// *** Basic block 10

	mv          s3, a0
	addi        a3, s0, -64
	li          t0, 20		// 0x14 ASCII \x14
	mv          a5, t0
	li          t0, 26		// 0x1a ASCII \x1a
	mv          a4, t0
	mv          a2, s2
	li          t0, 23		// 0x17 ASCII \x17
	mv          a1, t0
	mv          a0, s1
	call        AssembleUType

	// *** Basic block 11

	lw          s4, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 12

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s4
	li          t0, 27		// 0x1b ASCII \x1b
	mv          a1, t0
	mv          a0, s3
	call        NewAssemblerRelocation

	// *** Basic block 13

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 14

	lw          s5, 744(s1)
	mv          a4, x0
	mv          a3, x0
	mv          a2, s2
	mv          a1, s2
	li          t0, 19		// 0x13 ASCII \x13
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 15

	mv          a2, a0
	mv          a1, s5
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 16

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 17

	j           .Assemble_li_label_176

	// *** Basic block 18

.Assemble_li_label_163:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 19

	mv          s5, a0
	mv          a2, s5
	mv          a1, s2
	mv          a0, s1
	call        AssembleLoadImmediateConstant

	// *** Basic block 20

.Assemble_li_label_176:
	j           .Assemble_li_label_63
.func_end_Assemble_li:
	.size Assemble_li, .func_end_Assemble_li-Assemble_li

	.local  Assemble_la
	.type Assemble_la, @function

Assemble_la:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global LexLookingAt
	.global StringInit
	.global LexNextToken
	.global snprintf
	.global AssemblerCurrentAddress
	.local GetOrCreateSymbol
	.local AssembleUType
	.global NewAssemblerRelocation
	.global AssemblerAddRelocation
	.global AssemblerEmitWord
	.local ITypeInstruction
	.global StringDestruct
	addi sp, sp, -384
	// Saved return address (offset 376) and frame pointer (offset 368)
	sd ra, 376(sp)
	sd s0, 368(sp)
	addi s0, sp, 384
	// Local vars at offset -320(s0)
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
	lla         a2, .str.428
	mv          a1, x0
	call        Register

	// *** Basic block 1

	mv          s2, a0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .Assemble_la_label_72

	// *** Basic block 3

	lla         a1, .str.429
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 4

.Assemble_la_label_69:
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

.Assemble_la_label_72:
	addi        a0, s1, 176
	li          s3, 3		// 0x3 ASCII \x3
	mv          a1, s3
	call        LexLookingAt

	// *** Basic block 6

	beqz        a0, .Assemble_la_label_259

	// *** Basic block 7

	addi        a0, s0, -320
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 8

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 9

	addi        s4, s0, -280
	lla         s5, .str.430
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 10

	mv          a3, a0
	mv          a2, s5
	li          t0, 256		// 0x100
	mv          a1, t0
	mv          a0, s4
	call        snprintf

	// *** Basic block 11

	mv          a1, s4
	mv          a0, s1
	call        GetOrCreateSymbol

	// *** Basic block 12

	mv          s4, a0
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 13

	sd          a0, 48(s4)
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 72(s4)
	addi        a3, s0, -320
	li          t0, 20		// 0x14 ASCII \x14
	mv          a5, t0
	li          t0, 23		// 0x17 ASCII \x17
	mv          a4, t0
	mv          a2, s2
	mv          a1, t0
	mv          a0, s1
	call        AssembleUType

	// *** Basic block 14

	addi        t0, s0, -320
	ld          a1, 16(t0)
	mv          a0, s1
	call        GetOrCreateSymbol

	// *** Basic block 15

	mv          s5, a0
	lb          t0, 760(s1)
	beqz        t0, .Assemble_la_label_159

	// *** Basic block 16

	lw          t1, 44(s5)
	seqz        t0, t1

	// *** Basic block 17

.Assemble_la_label_159:
	beqz        t0, .Assemble_la_label_208

	// *** Basic block 18

	lw          s6, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 19

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s6
	li          t0, 24		// 0x18 ASCII \x18
	mv          a1, t0
	mv          a0, s4
	call        NewAssemblerRelocation

	// *** Basic block 20

	mv          s6, a0
	mv          a1, s6
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 21

	lw          s7, 744(s1)
	mv          a4, x0
	mv          a3, s3
	mv          a2, s2
	mv          a1, s2
	mv          a0, s3
	call        ITypeInstruction

	// *** Basic block 22

	mv          a2, a0
	mv          a1, s7
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 23

	j           .Assemble_la_label_254

	// *** Basic block 24

.Assemble_la_label_208:
	lw          s3, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 25

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s3
	li          t0, 24		// 0x18 ASCII \x18
	mv          a1, t0
	mv          a0, s4
	call        NewAssemblerRelocation

	// *** Basic block 26

	mv          s3, a0
	mv          a1, s3
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 27

	lw          s7, 744(s1)
	mv          a4, x0
	mv          a3, x0
	mv          a2, s2
	mv          a1, s2
	li          t0, 19		// 0x13 ASCII \x13
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 28

	mv          a2, a0
	mv          a1, s7
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 29

.Assemble_la_label_254:
	addi        a0, s0, -320
	call        StringDestruct

	// *** Basic block 30

	j           .Assemble_la_label_266

	// *** Basic block 31

.Assemble_la_label_259:
	lla         a1, .str.431
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 32

.Assemble_la_label_266:
	j           .Assemble_la_label_69
.func_end_Assemble_la:
	.size Assemble_la, .func_end_Assemble_la-Assemble_la

	.local  Assemble_lla
	.type Assemble_lla, @function

Assemble_lla:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global LexLookingAt
	.global StringInit
	.global LexNextToken
	.global snprintf
	.global AssemblerCurrentAddress
	.local GetOrCreateSymbol
	.local AssembleUType
	.global NewAssemblerRelocation
	.global AssemblerAddRelocation
	.global AssemblerEmitWord
	.local ITypeInstruction
	.global StringDestruct
	addi sp, sp, -368
	// Saved return address (offset 360) and frame pointer (offset 352)
	sd ra, 360(sp)
	sd s0, 352(sp)
	addi s0, sp, 368
	// Local vars at offset -320(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// End of stack frame
	mv          s1, a0
	lla         a2, .str.432
	mv          a1, x0
	call        Register

	// *** Basic block 1

	mv          s2, a0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .Assemble_lla_label_69

	// *** Basic block 3

	lla         a1, .str.433
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 4

.Assemble_lla_label_66:
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

.Assemble_lla_label_69:
	addi        a0, s1, 176
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        LexLookingAt

	// *** Basic block 6

	beqz        a0, .Assemble_lla_label_189

	// *** Basic block 7

	addi        a0, s0, -320
	addi        t0, a0, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 8

	addi        a0, s1, 176
	call        LexNextToken

	// *** Basic block 9

	addi        s3, s0, -280
	lla         s4, .str.434
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 10

	mv          a3, a0
	mv          a2, s4
	li          t0, 256		// 0x100
	mv          a1, t0
	mv          a0, s3
	call        snprintf

	// *** Basic block 11

	mv          a1, s3
	mv          a0, s1
	call        GetOrCreateSymbol

	// *** Basic block 12

	mv          s3, a0
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 13

	sd          a0, 48(s3)
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 72(s3)
	addi        a3, s0, -320
	li          t0, 23		// 0x17 ASCII \x17
	mv          a5, t0
	mv          a4, t0
	mv          a2, s2
	mv          a1, t0
	mv          a0, s1
	call        AssembleUType

	// *** Basic block 14

	lw          s4, 744(s1)
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 15

	sext.w      a3, a0
	mv          a4, x0
	mv          a2, s4
	li          t0, 24		// 0x18 ASCII \x18
	mv          a1, t0
	mv          a0, s3
	call        NewAssemblerRelocation

	// *** Basic block 16

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        AssemblerAddRelocation

	// *** Basic block 17

	lw          s5, 744(s1)
	mv          a4, x0
	mv          a3, x0
	mv          a2, s2
	mv          a1, s2
	li          t0, 19		// 0x13 ASCII \x13
	mv          a0, t0
	call        ITypeInstruction

	// *** Basic block 18

	mv          a2, a0
	mv          a1, s5
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 19

	addi        a0, s0, -320
	call        StringDestruct

	// *** Basic block 20

	j           .Assemble_lla_label_196

	// *** Basic block 21

.Assemble_lla_label_189:
	lla         a1, .str.435
	mv          a0, s1
	call        AssemblerError

	// *** Basic block 22

.Assemble_lla_label_196:
	j           .Assemble_lla_label_66
.func_end_Assemble_lla:
	.size Assemble_lla, .func_end_Assemble_lla-Assemble_lla

	.local  Assemble_sext_w
	.type Assemble_sext_w, @function

Assemble_sext_w:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALUImm
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
	lla         a2, .str.436
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_sext_w_label_41

	// *** Basic block 2

	mv          a4, s2
	mv          a3, x0
	mv          a2, x0
	li          t0, 27		// 0x1b ASCII \x1b
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUImm

	// *** Basic block 3

.Assemble_sext_w_label_41:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_sext_w:
	.size Assemble_sext_w, .func_end_Assemble_sext_w-Assemble_sext_w

	.local  Assemble_seqz
	.type Assemble_seqz, @function

Assemble_seqz:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALUImm
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
	lla         a2, .str.437
	addi        s2, s0, -32
	mv          a3, s2
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_seqz_label_45

	// *** Basic block 2

	mv          a4, s2
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUImm

	// *** Basic block 3

.Assemble_seqz_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_seqz:
	.size Assemble_seqz, .func_end_Assemble_seqz-Assemble_seqz

	.local  Assemble_snez
	.type Assemble_snez, @function

Assemble_snez:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALUReg
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	lla         a2, .str.438
	addi        a3, s0, -32
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_snez_label_55

	// *** Basic block 2

	addi        t0, s0, -32
	addi        t1, s0, -32
	lw          t1, 4(t1)
	sw          t1, 8(t0)
	addi        t0, s0, -32
	sw          x0, 4(t0)
	addi        a4, s0, -32
	mv          a3, x0
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_snez_label_55:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_snez:
	.size Assemble_snez, .func_end_Assemble_snez-Assemble_snez

	.local  Assemble_sltz
	.type Assemble_sltz, @function

Assemble_sltz:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALUReg
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	lla         a2, .str.439
	addi        a3, s0, -32
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_sltz_label_46

	// *** Basic block 2

	addi        t0, s0, -32
	sw          x0, 8(t0)
	addi        a4, s0, -32
	mv          a3, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_sltz_label_46:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_sltz:
	.size Assemble_sltz, .func_end_Assemble_sltz-Assemble_sltz

	.local  Assemble_sgtz
	.type Assemble_sgtz, @function

Assemble_sgtz:

	// *** Basic block 0

	.local ParseRegisterPair
	.local AssembleALUReg
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	lla         a2, .str.440
	addi        a3, s0, -32
	mv          a1, x0
	call        ParseRegisterPair

	// *** Basic block 1

	beqz        a0, .Assemble_sgtz_label_54

	// *** Basic block 2

	addi        t0, s0, -32
	addi        t1, s0, -32
	lw          t1, 4(t1)
	sw          t1, 8(t0)
	addi        t0, s0, -32
	sw          x0, 4(t0)
	addi        a4, s0, -32
	mv          a3, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a1, t0
	mv          a0, s1
	call        AssembleALUReg

	// *** Basic block 3

.Assemble_sgtz_label_54:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble_sgtz:
	.size Assemble_sgtz, .func_end_Assemble_sgtz-Assemble_sgtz

	.local  AssembleConditionalBranchZero
	.type AssembleConditionalBranchZero, @function

AssembleConditionalBranchZero:

	// *** Basic block 0

	.local Register
	.global LexMatch
	.global AssemblerError
	.global AssemblerEvaluateExpression
	.global AssemblerCurrentAddress
	.global AssemblerEmitWord
	.local BTypeInstruction
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
	lla         a2, .str.441
	mv          a1, x0
	call        Register

	// *** Basic block 1

	mv          s3, a0
	addi        a0, s1, 176
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	call        LexMatch

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .AssembleConditionalBranchZero_label_53

	// *** Basic block 3

	lla         a1, .str.442
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

	// *** Basic block 4

.AssembleConditionalBranchZero_label_50:
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

.AssembleConditionalBranchZero_label_53:
	mv          a0, s1
	call        AssemblerEvaluateExpression

	// *** Basic block 6

	mv          s4, a0
	mv          a0, s1
	call        AssemblerCurrentAddress

	// *** Basic block 7

	sub         t0, s4, a0
	sext.w      s5, t0
	lw          s6, 744(s1)
	mv          a4, s5
	mv          a3, s2
	mv          a2, x0
	mv          a1, s3
	li          t0, 99		// 0x63 ASCII 'c'
	mv          a0, t0
	call        BTypeInstruction

	// *** Basic block 8

	mv          a2, a0
	mv          a1, s6
	mv          a0, s1
	call        AssemblerEmitWord

	// *** Basic block 9

	j           .AssembleConditionalBranchZero_label_50
.func_end_AssembleConditionalBranchZero:
	.size AssembleConditionalBranchZero, .func_end_AssembleConditionalBranchZero-AssembleConditionalBranchZero

	.local  Assemble_beqz
	.type Assemble_beqz, @function

Assemble_beqz:

	// *** Basic block 0

	.local AssembleConditionalBranchZero
	// Leaf procedure, no stack frame generated
	mv          a1, x0
	j           AssembleConditionalBranchZero
.func_end_Assemble_beqz:
	.size Assemble_beqz, .func_end_Assemble_beqz-Assemble_beqz

	.local  Assemble_bnez
	.type Assemble_bnez, @function

Assemble_bnez:

	// *** Basic block 0

	.local AssembleConditionalBranchZero
	// Leaf procedure, no stack frame generated
	li          a1, 1		// 0x1 ASCII \x1
	j           AssembleConditionalBranchZero
.func_end_Assemble_bnez:
	.size Assemble_bnez, .func_end_Assemble_bnez-Assemble_bnez

	.local  Assemble_rcall
	.type Assemble_rcall, @function

Assemble_rcall:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.443
	lla         a2, .str.444
	j           AssemblerError
.func_end_Assemble_rcall:
	.size Assemble_rcall, .func_end_Assemble_rcall-Assemble_rcall

	.local  Assemble_callf
	.type Assemble_callf, @function

Assemble_callf:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.445
	lla         a2, .str.446
	j           AssemblerError
.func_end_Assemble_callf:
	.size Assemble_callf, .func_end_Assemble_callf-Assemble_callf

	.local  Assemble_rcallf
	.type Assemble_rcallf, @function

Assemble_rcallf:

	// *** Basic block 0

	.global AssemblerError
	// Leaf procedure, no stack frame generated
	lla         a1, .str.447
	lla         a2, .str.448
	j           AssemblerError
.func_end_Assemble_rcallf:
	.size Assemble_rcallf, .func_end_Assemble_rcallf-Assemble_rcallf

.PCend:
	.data
reloc_types:
	.type   reloc_types,@object
	.local  reloc_types
	.size   reloc_types,44
	.p2align  2
	.word   34
	.word   1
	.word   2
	.word   34
	.word   35
	.word   36
	.word   38
	.word   39
	.word   40
	.word   19
	.word   20

known_reg_names:
	.type   known_reg_names,@object
	.local  known_reg_names
	.size   known_reg_names,80
	.p2align  3
	.long    .str.162
	.word   2
	.space  4
	.long    .str.163
	.word   8
	.space  4
	.long    .str.164
	.word   1
	.space  4
	.long    .str.165
	.word   0
	.space  4
	.word   0
	.space  4
	.word   0
	.space  4

prefixed_reg_names:
	.type   prefixed_reg_names,@object
	.local  prefixed_reg_names
	.size   prefixed_reg_names,416
	.p2align  3
	.long    .str.166
	.word   1
	.word   0
	.word   0
	.word   32
	.word   0
	.space  4
	.long    .str.167
	.word   1
	.word   0
	.word   10
	.word   17
	.word   0
	.space  4
	.long    .str.168
	.word   1
	.word   0
	.word   8
	.word   9
	.word   0
	.space  4
	.long    .str.169
	.word   1
	.word   2
	.word   18
	.word   27
	.word   0
	.space  4
	.long    .str.170
	.word   1
	.word   0
	.word   5
	.word   7
	.word   0
	.space  4
	.long    .str.171
	.word   1
	.word   3
	.word   28
	.word   31
	.word   0
	.space  4
	.long    .str.172
	.word   2
	.word   0
	.word   10
	.word   17
	.word   1
	.space  4
	.long    .str.173
	.word   2
	.word   0
	.word   8
	.word   9
	.word   1
	.space  4
	.long    .str.174
	.word   2
	.word   2
	.word   18
	.word   27
	.word   1
	.space  4
	.long    .str.175
	.word   2
	.word   0
	.word   0
	.word   7
	.word   1
	.space  4
	.long    .str.176
	.word   2
	.word   2
	.word   28
	.word   31
	.word   1
	.space  4
	.long    .str.177
	.word   1
	.word   0
	.word   0
	.word   32
	.word   1
	.space  4
	.word   0
	.space  4
	.word   0
	.word   0
	.word   0
	.word   0
	.word   0
	.space  4

rounding_modes:
	.type   rounding_modes,@object
	.local  rounding_modes
	.size   rounding_modes,112
	.p2align  3
	.long    .str.298
	.word   0
	.space  4
	.long    .str.299
	.word   1
	.space  4
	.long    .str.300
	.word   2
	.space  4
	.long    .str.301
	.word   3
	.space  4
	.long    .str.302
	.word   4
	.space  4
	.long    .str.303
	.word   7
	.space  4
	.word   0
	.space  4
	.word   0
	.space  4

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "mv"
	.type .str.1, @object
	.size .str.1, 3

.str.2:
	.asciz "fmv.s"
	.type .str.2, @object
	.size .str.2, 6

.str.3:
	.asciz "fmv.d"
	.type .str.3, @object
	.size .str.3, 6

.str.4:
	.asciz "ret"
	.type .str.4, @object
	.size .str.4, 4

.str.5:
	.asciz "lui"
	.type .str.5, @object
	.size .str.5, 4

.str.6:
	.asciz "auipc"
	.type .str.6, @object
	.size .str.6, 6

.str.7:
	.asciz "jal"
	.type .str.7, @object
	.size .str.7, 4

.str.8:
	.asciz "jalr"
	.type .str.8, @object
	.size .str.8, 5

.str.9:
	.asciz "beq"
	.type .str.9, @object
	.size .str.9, 4

.str.10:
	.asciz "bne"
	.type .str.10, @object
	.size .str.10, 4

.str.11:
	.asciz "blt"
	.type .str.11, @object
	.size .str.11, 4

.str.12:
	.asciz "bge"
	.type .str.12, @object
	.size .str.12, 4

.str.13:
	.asciz "bltu"
	.type .str.13, @object
	.size .str.13, 5

.str.14:
	.asciz "bgeu"
	.type .str.14, @object
	.size .str.14, 5

.str.15:
	.asciz "lb"
	.type .str.15, @object
	.size .str.15, 3

.str.16:
	.asciz "lh"
	.type .str.16, @object
	.size .str.16, 3

.str.17:
	.asciz "lw"
	.type .str.17, @object
	.size .str.17, 3

.str.18:
	.asciz "lbu"
	.type .str.18, @object
	.size .str.18, 4

.str.19:
	.asciz "lhu"
	.type .str.19, @object
	.size .str.19, 4

.str.20:
	.asciz "sb"
	.type .str.20, @object
	.size .str.20, 3

.str.21:
	.asciz "sh"
	.type .str.21, @object
	.size .str.21, 3

.str.22:
	.asciz "sw"
	.type .str.22, @object
	.size .str.22, 3

.str.23:
	.asciz "addi"
	.type .str.23, @object
	.size .str.23, 5

.str.24:
	.asciz "slti"
	.type .str.24, @object
	.size .str.24, 5

.str.25:
	.asciz "sltiu"
	.type .str.25, @object
	.size .str.25, 6

.str.26:
	.asciz "xori"
	.type .str.26, @object
	.size .str.26, 5

.str.27:
	.asciz "ori"
	.type .str.27, @object
	.size .str.27, 4

.str.28:
	.asciz "andi"
	.type .str.28, @object
	.size .str.28, 5

.str.29:
	.asciz "slli"
	.type .str.29, @object
	.size .str.29, 5

.str.30:
	.asciz "srli"
	.type .str.30, @object
	.size .str.30, 5

.str.31:
	.asciz "srai"
	.type .str.31, @object
	.size .str.31, 5

.str.32:
	.asciz "add"
	.type .str.32, @object
	.size .str.32, 4

.str.33:
	.asciz "sub"
	.type .str.33, @object
	.size .str.33, 4

.str.34:
	.asciz "sll"
	.type .str.34, @object
	.size .str.34, 4

.str.35:
	.asciz "slt"
	.type .str.35, @object
	.size .str.35, 4

.str.36:
	.asciz "sltu"
	.type .str.36, @object
	.size .str.36, 5

.str.37:
	.asciz "xor"
	.type .str.37, @object
	.size .str.37, 4

.str.38:
	.asciz "srl"
	.type .str.38, @object
	.size .str.38, 4

.str.39:
	.asciz "sra"
	.type .str.39, @object
	.size .str.39, 4

.str.40:
	.asciz "or"
	.type .str.40, @object
	.size .str.40, 3

.str.41:
	.asciz "and"
	.type .str.41, @object
	.size .str.41, 4

.str.42:
	.asciz "fence"
	.type .str.42, @object
	.size .str.42, 6

.str.43:
	.asciz "fence_i"
	.type .str.43, @object
	.size .str.43, 8

.str.44:
	.asciz "ecall"
	.type .str.44, @object
	.size .str.44, 6

.str.45:
	.asciz "ebreak"
	.type .str.45, @object
	.size .str.45, 7

.str.46:
	.asciz "csrrw"
	.type .str.46, @object
	.size .str.46, 6

.str.47:
	.asciz "csrrs"
	.type .str.47, @object
	.size .str.47, 6

.str.48:
	.asciz "csrrc"
	.type .str.48, @object
	.size .str.48, 6

.str.49:
	.asciz "csrrwi"
	.type .str.49, @object
	.size .str.49, 7

.str.50:
	.asciz "csrrsi"
	.type .str.50, @object
	.size .str.50, 7

.str.51:
	.asciz "csrrci"
	.type .str.51, @object
	.size .str.51, 7

.str.52:
	.asciz "lwu"
	.type .str.52, @object
	.size .str.52, 4

.str.53:
	.asciz "ld"
	.type .str.53, @object
	.size .str.53, 3

.str.54:
	.asciz "sd"
	.type .str.54, @object
	.size .str.54, 3

.str.55:
	.asciz "addiw"
	.type .str.55, @object
	.size .str.55, 6

.str.56:
	.asciz "slliw"
	.type .str.56, @object
	.size .str.56, 6

.str.57:
	.asciz "srliw"
	.type .str.57, @object
	.size .str.57, 6

.str.58:
	.asciz "sraiw"
	.type .str.58, @object
	.size .str.58, 6

.str.59:
	.asciz "addw"
	.type .str.59, @object
	.size .str.59, 5

.str.60:
	.asciz "subw"
	.type .str.60, @object
	.size .str.60, 5

.str.61:
	.asciz "sllw"
	.type .str.61, @object
	.size .str.61, 5

.str.62:
	.asciz "srlw"
	.type .str.62, @object
	.size .str.62, 5

.str.63:
	.asciz "sraw"
	.type .str.63, @object
	.size .str.63, 5

.str.64:
	.asciz "mul"
	.type .str.64, @object
	.size .str.64, 4

.str.65:
	.asciz "mulh"
	.type .str.65, @object
	.size .str.65, 5

.str.66:
	.asciz "mulhsu"
	.type .str.66, @object
	.size .str.66, 7

.str.67:
	.asciz "mulhu"
	.type .str.67, @object
	.size .str.67, 6

.str.68:
	.asciz "div"
	.type .str.68, @object
	.size .str.68, 4

.str.69:
	.asciz "divu"
	.type .str.69, @object
	.size .str.69, 5

.str.70:
	.asciz "rem"
	.type .str.70, @object
	.size .str.70, 4

.str.71:
	.asciz "remu"
	.type .str.71, @object
	.size .str.71, 5

.str.72:
	.asciz "mulw"
	.type .str.72, @object
	.size .str.72, 5

.str.73:
	.asciz "divw"
	.type .str.73, @object
	.size .str.73, 5

.str.74:
	.asciz "divuw"
	.type .str.74, @object
	.size .str.74, 6

.str.75:
	.asciz "remw"
	.type .str.75, @object
	.size .str.75, 5

.str.76:
	.asciz "remuw"
	.type .str.76, @object
	.size .str.76, 6

.str.77:
	.asciz "flw"
	.type .str.77, @object
	.size .str.77, 4

.str.78:
	.asciz "fsw"
	.type .str.78, @object
	.size .str.78, 4

.str.79:
	.asciz "fmadd.s"
	.type .str.79, @object
	.size .str.79, 8

.str.80:
	.asciz "fmsub.s"
	.type .str.80, @object
	.size .str.80, 8

.str.81:
	.asciz "fnmsub.s"
	.type .str.81, @object
	.size .str.81, 9

.str.82:
	.asciz "fnmadd.s"
	.type .str.82, @object
	.size .str.82, 9

.str.83:
	.asciz "fadd.s"
	.type .str.83, @object
	.size .str.83, 7

.str.84:
	.asciz "fsub.s"
	.type .str.84, @object
	.size .str.84, 7

.str.85:
	.asciz "fmul.s"
	.type .str.85, @object
	.size .str.85, 7

.str.86:
	.asciz "fdiv.s"
	.type .str.86, @object
	.size .str.86, 7

.str.87:
	.asciz "fsqrt.s"
	.type .str.87, @object
	.size .str.87, 8

.str.88:
	.asciz "fsgnj.s"
	.type .str.88, @object
	.size .str.88, 8

.str.89:
	.asciz "fsgnjs.s"
	.type .str.89, @object
	.size .str.89, 9

.str.90:
	.asciz "fsgnjx.s"
	.type .str.90, @object
	.size .str.90, 9

.str.91:
	.asciz "fmin.s"
	.type .str.91, @object
	.size .str.91, 7

.str.92:
	.asciz "fmax.s"
	.type .str.92, @object
	.size .str.92, 7

.str.93:
	.asciz "fcvt.w.s"
	.type .str.93, @object
	.size .str.93, 9

.str.94:
	.asciz "fcvt.wu.s"
	.type .str.94, @object
	.size .str.94, 10

.str.95:
	.asciz "fmv.x.w"
	.type .str.95, @object
	.size .str.95, 8

.str.96:
	.asciz "feq.s"
	.type .str.96, @object
	.size .str.96, 6

.str.97:
	.asciz "flt.s"
	.type .str.97, @object
	.size .str.97, 6

.str.98:
	.asciz "fle.s"
	.type .str.98, @object
	.size .str.98, 6

.str.99:
	.asciz "fclass.s"
	.type .str.99, @object
	.size .str.99, 9

.str.100:
	.asciz "fcvt.s.w"
	.type .str.100, @object
	.size .str.100, 9

.str.101:
	.asciz "fcvt.w.su"
	.type .str.101, @object
	.size .str.101, 10

.str.102:
	.asciz "fmv.w.x"
	.type .str.102, @object
	.size .str.102, 8

.str.103:
	.asciz "fcvt.l.s"
	.type .str.103, @object
	.size .str.103, 9

.str.104:
	.asciz "fcvt.lu.s"
	.type .str.104, @object
	.size .str.104, 10

.str.105:
	.asciz "fcvt.s.l"
	.type .str.105, @object
	.size .str.105, 9

.str.106:
	.asciz "fcvt.s.lu"
	.type .str.106, @object
	.size .str.106, 10

.str.107:
	.asciz "fld"
	.type .str.107, @object
	.size .str.107, 4

.str.108:
	.asciz "fsd"
	.type .str.108, @object
	.size .str.108, 4

.str.109:
	.asciz "fmadd.d"
	.type .str.109, @object
	.size .str.109, 8

.str.110:
	.asciz "fmsub.d"
	.type .str.110, @object
	.size .str.110, 8

.str.111:
	.asciz "fnmsub.d"
	.type .str.111, @object
	.size .str.111, 9

.str.112:
	.asciz "fnmadd.d"
	.type .str.112, @object
	.size .str.112, 9

.str.113:
	.asciz "fadd.d"
	.type .str.113, @object
	.size .str.113, 7

.str.114:
	.asciz "fsub.d"
	.type .str.114, @object
	.size .str.114, 7

.str.115:
	.asciz "fmul.d"
	.type .str.115, @object
	.size .str.115, 7

.str.116:
	.asciz "fdiv.d"
	.type .str.116, @object
	.size .str.116, 7

.str.117:
	.asciz "fsqrt.d"
	.type .str.117, @object
	.size .str.117, 8

.str.118:
	.asciz "fsgnj.d"
	.type .str.118, @object
	.size .str.118, 8

.str.119:
	.asciz "fsgnjn.d"
	.type .str.119, @object
	.size .str.119, 9

.str.120:
	.asciz "fsgnjx.d"
	.type .str.120, @object
	.size .str.120, 9

.str.121:
	.asciz "fmin.d"
	.type .str.121, @object
	.size .str.121, 7

.str.122:
	.asciz "fmax.d"
	.type .str.122, @object
	.size .str.122, 7

.str.123:
	.asciz "fcvt.s.d"
	.type .str.123, @object
	.size .str.123, 9

.str.124:
	.asciz "fcvt.d.s"
	.type .str.124, @object
	.size .str.124, 9

.str.125:
	.asciz "feq.d"
	.type .str.125, @object
	.size .str.125, 6

.str.126:
	.asciz "flt.d"
	.type .str.126, @object
	.size .str.126, 6

.str.127:
	.asciz "fle.d"
	.type .str.127, @object
	.size .str.127, 6

.str.128:
	.asciz "fclass.d"
	.type .str.128, @object
	.size .str.128, 9

.str.129:
	.asciz "fcvt.w.d"
	.type .str.129, @object
	.size .str.129, 9

.str.130:
	.asciz "fcvt.wu.d"
	.type .str.130, @object
	.size .str.130, 10

.str.131:
	.asciz "fcvt.d.w"
	.type .str.131, @object
	.size .str.131, 9

.str.132:
	.asciz "fcvt.d.wu"
	.type .str.132, @object
	.size .str.132, 10

.str.133:
	.asciz "fcvt.l.d"
	.type .str.133, @object
	.size .str.133, 9

.str.134:
	.asciz "fcvt.lu.d"
	.type .str.134, @object
	.size .str.134, 10

.str.135:
	.asciz "fmv.x.d"
	.type .str.135, @object
	.size .str.135, 8

.str.136:
	.asciz "fcvt.d.l"
	.type .str.136, @object
	.size .str.136, 9

.str.137:
	.asciz "fcvt.d.lu"
	.type .str.137, @object
	.size .str.137, 10

.str.138:
	.asciz "fmv.d.x"
	.type .str.138, @object
	.size .str.138, 8

.str.139:
	.asciz "nop"
	.type .str.139, @object
	.size .str.139, 4

.str.140:
	.asciz "not"
	.type .str.140, @object
	.size .str.140, 4

.str.141:
	.asciz "neg"
	.type .str.141, @object
	.size .str.141, 4

.str.142:
	.asciz "fneg.s"
	.type .str.142, @object
	.size .str.142, 7

.str.143:
	.asciz "fneg.d"
	.type .str.143, @object
	.size .str.143, 7

.str.144:
	.asciz "li"
	.type .str.144, @object
	.size .str.144, 3

.str.145:
	.asciz "la"
	.type .str.145, @object
	.size .str.145, 3

.str.146:
	.asciz "lla"
	.type .str.146, @object
	.size .str.146, 4

.str.147:
	.asciz "sext.w"
	.type .str.147, @object
	.size .str.147, 7

.str.148:
	.asciz "seqz"
	.type .str.148, @object
	.size .str.148, 5

.str.149:
	.asciz "snez"
	.type .str.149, @object
	.size .str.149, 5

.str.150:
	.asciz "sltz"
	.type .str.150, @object
	.size .str.150, 5

.str.151:
	.asciz "sgtz"
	.type .str.151, @object
	.size .str.151, 5

.str.152:
	.asciz "beqz"
	.type .str.152, @object
	.size .str.152, 5

.str.153:
	.asciz "bnez"
	.type .str.153, @object
	.size .str.153, 5

.str.154:
	.asciz "j"
	.type .str.154, @object
	.size .str.154, 2

.str.155:
	.asciz "jr"
	.type .str.155, @object
	.size .str.155, 3

.str.156:
	.asciz "call"
	.type .str.156, @object
	.size .str.156, 5

.str.157:
	.asciz "rcall"
	.type .str.157, @object
	.size .str.157, 6

.str.158:
	.asciz "callf"
	.type .str.158, @object
	.size .str.158, 6

.str.159:
	.asciz "rcallf"
	.type .str.159, @object
	.size .str.159, 7

.str.160:
	.asciz ".bss"
	.type .str.160, @object
	.size .str.160, 5

.str.161:
	.asciz "Syntax error; unknown instruction: %s"
	.type .str.161, @object
	.size .str.161, 38

.str.162:
	.asciz "sp"
	.type .str.162, @object
	.size .str.162, 3

.str.163:
	.asciz "float"
	.type .str.163, @object
	.size .str.163, 6

.str.164:
	.asciz "ra"
	.type .str.164, @object
	.size .str.164, 3

.str.165:
	.asciz "zero"
	.type .str.165, @object
	.size .str.165, 5

.str.166:
	.asciz "x"
	.type .str.166, @object
	.size .str.166, 2

.str.167:
	.asciz "a"
	.type .str.167, @object
	.size .str.167, 2

.str.168:
	.asciz "s"
	.type .str.168, @object
	.size .str.168, 2

.str.169:
	.asciz "s"
	.type .str.169, @object
	.size .str.169, 2

.str.170:
	.asciz "t"
	.type .str.170, @object
	.size .str.170, 2

.str.171:
	.asciz "t"
	.type .str.171, @object
	.size .str.171, 2

.str.172:
	.asciz "fa"
	.type .str.172, @object
	.size .str.172, 3

.str.173:
	.asciz "fs"
	.type .str.173, @object
	.size .str.173, 3

.str.174:
	.asciz "fs"
	.type .str.174, @object
	.size .str.174, 3

.str.175:
	.asciz "ft"
	.type .str.175, @object
	.size .str.175, 3

.str.176:
	.asciz "ft"
	.type .str.176, @object
	.size .str.176, 3

.str.177:
	.asciz "f"
	.type .str.177, @object
	.size .str.177, 2

.str.178:
	.asciz "Expected %s register name"
	.type .str.178, @object
	.size .str.178, 26

.str.179:
	.asciz "integer"
	.type .str.179, @object
	.size .str.179, 8

.str.180:
	.asciz "float"
	.type .str.180, @object
	.size .str.180, 6

.str.181:
	.asciz "Invalid register type; got %s expected %s"
	.type .str.181, @object
	.size .str.181, 42

.str.182:
	.asciz "Missing comma"
	.type .str.182, @object
	.size .str.182, 14

.str.183:
	.asciz "Missing comma"
	.type .str.183, @object
	.size .str.183, 14

.str.184:
	.asciz "Missing comma"
	.type .str.184, @object
	.size .str.184, 14

.str.185:
	.asciz "Syntax error in assembler function: missing )"
	.type .str.185, @object
	.size .str.185, 46

.str.186:
	.asciz "Syntax error in assembler function: missing symbol name"
	.type .str.186, @object
	.size .str.186, 56

.str.187:
	.asciz "Syntax error in assembler function: missing ("
	.type .str.187, @object
	.size .str.187, 46

.str.188:
	.asciz "Syntax error in assembler function: missing name"
	.type .str.188, @object
	.size .str.188, 49

.str.189:
	.asciz "Invalid load/store offset %d"
	.type .str.189, @object
	.size .str.189, 29

.str.190:
	.asciz "pcrel_lo"
	.type .str.190, @object
	.size .str.190, 9

.str.191:
	.asciz "integer"
	.type .str.191, @object
	.size .str.191, 8

.str.192:
	.asciz "Missing comma"
	.type .str.192, @object
	.size .str.192, 14

.str.193:
	.asciz "(null)"
	.type .str.193, @object
	.size .str.193, 1

.str.194:
	.asciz "(null)"
	.type .str.194, @object
	.size .str.194, 1

.str.195:
	.asciz "lo"
	.type .str.195, @object
	.size .str.195, 3

.str.196:
	.asciz "pcrel_lo"
	.type .str.196, @object
	.size .str.196, 9

.str.197:
	.asciz "Bad assembler function %%%s() for load/store instruction"
	.type .str.197, @object
	.size .str.197, 57

.str.198:
	.asciz "Expected offset(reg) for load/store"
	.type .str.198, @object
	.size .str.198, 36

.str.199:
	.asciz "integer"
	.type .str.199, @object
	.size .str.199, 8

.str.200:
	.asciz "Missing close paren"
	.type .str.200, @object
	.size .str.200, 20

.str.201:
	.asciz "Expected offset(reg) for load/store"
	.type .str.201, @object
	.size .str.201, 36

.str.202:
	.asciz "integer"
	.type .str.202, @object
	.size .str.202, 8

.str.203:
	.asciz "Missing close paren"
	.type .str.203, @object
	.size .str.203, 20

.str.204:
	.asciz "integer"
	.type .str.204, @object
	.size .str.204, 8

.str.205:
	.asciz "float"
	.type .str.205, @object
	.size .str.205, 6

.str.206:
	.asciz "float"
	.type .str.206, @object
	.size .str.206, 6

.str.207:
	.asciz "integer"
	.type .str.207, @object
	.size .str.207, 8

.str.208:
	.asciz "Missing comma"
	.type .str.208, @object
	.size .str.208, 14

.str.209:
	.asciz "(null)"
	.type .str.209, @object
	.size .str.209, 1

.str.210:
	.asciz "(null)"
	.type .str.210, @object
	.size .str.210, 1

.str.211:
	.asciz "hi"
	.type .str.211, @object
	.size .str.211, 3

.str.212:
	.asciz "Bad assembler function %%%s() for lui instruction"
	.type .str.212, @object
	.size .str.212, 50

.str.213:
	.asciz "integer"
	.type .str.213, @object
	.size .str.213, 8

.str.214:
	.asciz "Missing comma"
	.type .str.214, @object
	.size .str.214, 14

.str.215:
	.asciz "(null)"
	.type .str.215, @object
	.size .str.215, 1

.str.216:
	.asciz "(null)"
	.type .str.216, @object
	.size .str.216, 1

.str.217:
	.asciz "pcrel_hi"
	.type .str.217, @object
	.size .str.217, 9

.str.218:
	.asciz "Bad assembler function %%%s() for auipc instruction"
	.type .str.218, @object
	.size .str.218, 52

.str.219:
	.asciz "Missing comma"
	.type .str.219, @object
	.size .str.219, 14

.str.220:
	.asciz "Missing symbol for jal instruction"
	.type .str.220, @object
	.size .str.220, 35

.str.221:
	.asciz "Missing symbol for j instruction"
	.type .str.221, @object
	.size .str.221, 33

.str.222:
	.asciz "integer"
	.type .str.222, @object
	.size .str.222, 8

.str.223:
	.asciz "Missing symbol for call instruction"
	.type .str.223, @object
	.size .str.223, 36

.str.224:
	.asciz "integer"
	.type .str.224, @object
	.size .str.224, 8

.str.225:
	.asciz "Missing comma"
	.type .str.225, @object
	.size .str.225, 14

.str.226:
	.asciz "integer"
	.type .str.226, @object
	.size .str.226, 8

.str.227:
	.asciz "Missing comma"
	.type .str.227, @object
	.size .str.227, 14

.str.228:
	.asciz "integer"
	.type .str.228, @object
	.size .str.228, 8

.str.229:
	.asciz "Missing comma"
	.type .str.229, @object
	.size .str.229, 14

.str.230:
	.asciz "integer"
	.type .str.230, @object
	.size .str.230, 8

.str.231:
	.asciz "Missing comma"
	.type .str.231, @object
	.size .str.231, 14

.str.232:
	.asciz "integer"
	.type .str.232, @object
	.size .str.232, 8

.str.233:
	.asciz "Missing comma"
	.type .str.233, @object
	.size .str.233, 14

.str.234:
	.asciz "integer"
	.type .str.234, @object
	.size .str.234, 8

.str.235:
	.asciz "Missing comma"
	.type .str.235, @object
	.size .str.235, 14

.str.236:
	.asciz "integer"
	.type .str.236, @object
	.size .str.236, 8

.str.237:
	.asciz "Missing comma"
	.type .str.237, @object
	.size .str.237, 14

.str.238:
	.asciz "integer"
	.type .str.238, @object
	.size .str.238, 8

.str.239:
	.asciz "Missing comma"
	.type .str.239, @object
	.size .str.239, 14

.str.240:
	.asciz "integer"
	.type .str.240, @object
	.size .str.240, 8

.str.241:
	.asciz "Missing comma"
	.type .str.241, @object
	.size .str.241, 14

.str.242:
	.asciz "integer"
	.type .str.242, @object
	.size .str.242, 8

.str.243:
	.asciz "Missing comma"
	.type .str.243, @object
	.size .str.243, 14

.str.244:
	.asciz "integer"
	.type .str.244, @object
	.size .str.244, 8

.str.245:
	.asciz "Missing comma"
	.type .str.245, @object
	.size .str.245, 14

.str.246:
	.asciz "integer"
	.type .str.246, @object
	.size .str.246, 8

.str.247:
	.asciz "integer"
	.type .str.247, @object
	.size .str.247, 8

.str.248:
	.asciz "integer"
	.type .str.248, @object
	.size .str.248, 8

.str.249:
	.asciz "integer"
	.type .str.249, @object
	.size .str.249, 8

.str.250:
	.asciz "integer"
	.type .str.250, @object
	.size .str.250, 8

.str.251:
	.asciz "integer"
	.type .str.251, @object
	.size .str.251, 8

.str.252:
	.asciz "integer"
	.type .str.252, @object
	.size .str.252, 8

.str.253:
	.asciz "integer"
	.type .str.253, @object
	.size .str.253, 8

.str.254:
	.asciz "integer"
	.type .str.254, @object
	.size .str.254, 8

.str.255:
	.asciz "integer"
	.type .str.255, @object
	.size .str.255, 8

.str.256:
	.asciz "Unimplemented instruction %s"
	.type .str.256, @object
	.size .str.256, 29

.str.257:
	.asciz "fence"
	.type .str.257, @object
	.size .str.257, 6

.str.258:
	.asciz "Unimplemented instruction %s"
	.type .str.258, @object
	.size .str.258, 29

.str.259:
	.asciz "fence_i"
	.type .str.259, @object
	.size .str.259, 8

.str.260:
	.asciz "Unimplemented instruction %s"
	.type .str.260, @object
	.size .str.260, 29

.str.261:
	.asciz "csrrw"
	.type .str.261, @object
	.size .str.261, 6

.str.262:
	.asciz "Unimplemented instruction %s"
	.type .str.262, @object
	.size .str.262, 29

.str.263:
	.asciz "csrrs"
	.type .str.263, @object
	.size .str.263, 6

.str.264:
	.asciz "Unimplemented instruction %s"
	.type .str.264, @object
	.size .str.264, 29

.str.265:
	.asciz "csrrc"
	.type .str.265, @object
	.size .str.265, 6

.str.266:
	.asciz "Unimplemented instruction %s"
	.type .str.266, @object
	.size .str.266, 29

.str.267:
	.asciz "csrrwi"
	.type .str.267, @object
	.size .str.267, 7

.str.268:
	.asciz "Unimplemented instruction %s"
	.type .str.268, @object
	.size .str.268, 29

.str.269:
	.asciz "csrrsi"
	.type .str.269, @object
	.size .str.269, 7

.str.270:
	.asciz "Unimplemented instruction %s"
	.type .str.270, @object
	.size .str.270, 29

.str.271:
	.asciz "csrrci"
	.type .str.271, @object
	.size .str.271, 7

.str.272:
	.asciz "integer"
	.type .str.272, @object
	.size .str.272, 8

.str.273:
	.asciz "Missing comma"
	.type .str.273, @object
	.size .str.273, 14

.str.274:
	.asciz "integer"
	.type .str.274, @object
	.size .str.274, 8

.str.275:
	.asciz "Missing comma"
	.type .str.275, @object
	.size .str.275, 14

.str.276:
	.asciz "integer"
	.type .str.276, @object
	.size .str.276, 8

.str.277:
	.asciz "Missing comma"
	.type .str.277, @object
	.size .str.277, 14

.str.278:
	.asciz "integer"
	.type .str.278, @object
	.size .str.278, 8

.str.279:
	.asciz "Missing comma"
	.type .str.279, @object
	.size .str.279, 14

.str.280:
	.asciz "integer"
	.type .str.280, @object
	.size .str.280, 8

.str.281:
	.asciz "integer"
	.type .str.281, @object
	.size .str.281, 8

.str.282:
	.asciz "integer"
	.type .str.282, @object
	.size .str.282, 8

.str.283:
	.asciz "integer"
	.type .str.283, @object
	.size .str.283, 8

.str.284:
	.asciz "integer"
	.type .str.284, @object
	.size .str.284, 8

.str.285:
	.asciz "integer"
	.type .str.285, @object
	.size .str.285, 8

.str.286:
	.asciz "integer"
	.type .str.286, @object
	.size .str.286, 8

.str.287:
	.asciz "integer"
	.type .str.287, @object
	.size .str.287, 8

.str.288:
	.asciz "integer"
	.type .str.288, @object
	.size .str.288, 8

.str.289:
	.asciz "integer"
	.type .str.289, @object
	.size .str.289, 8

.str.290:
	.asciz "integer"
	.type .str.290, @object
	.size .str.290, 8

.str.291:
	.asciz "integer"
	.type .str.291, @object
	.size .str.291, 8

.str.292:
	.asciz "integer"
	.type .str.292, @object
	.size .str.292, 8

.str.293:
	.asciz "integer"
	.type .str.293, @object
	.size .str.293, 8

.str.294:
	.asciz "integer"
	.type .str.294, @object
	.size .str.294, 8

.str.295:
	.asciz "integer"
	.type .str.295, @object
	.size .str.295, 8

.str.296:
	.asciz "integer"
	.type .str.296, @object
	.size .str.296, 8

.str.297:
	.asciz "integer"
	.type .str.297, @object
	.size .str.297, 8

.str.298:
	.asciz "rne"
	.type .str.298, @object
	.size .str.298, 4

.str.299:
	.asciz "rtz"
	.type .str.299, @object
	.size .str.299, 4

.str.300:
	.asciz "rdn"
	.type .str.300, @object
	.size .str.300, 4

.str.301:
	.asciz "rup"
	.type .str.301, @object
	.size .str.301, 4

.str.302:
	.asciz "rmm"
	.type .str.302, @object
	.size .str.302, 4

.str.303:
	.asciz "dyn"
	.type .str.303, @object
	.size .str.303, 4

.str.304:
	.asciz "Invalid floating point rounding mode: %s"
	.type .str.304, @object
	.size .str.304, 41

.str.305:
	.asciz "float"
	.type .str.305, @object
	.size .str.305, 6

.str.306:
	.asciz "Missing comma"
	.type .str.306, @object
	.size .str.306, 14

.str.307:
	.asciz "Expected offset(reg) for load/store"
	.type .str.307, @object
	.size .str.307, 36

.str.308:
	.asciz "integer"
	.type .str.308, @object
	.size .str.308, 8

.str.309:
	.asciz "Missing close paren"
	.type .str.309, @object
	.size .str.309, 20

.str.310:
	.asciz "float"
	.type .str.310, @object
	.size .str.310, 6

.str.311:
	.asciz "Missing comma"
	.type .str.311, @object
	.size .str.311, 14

.str.312:
	.asciz "Expected offset(reg) for load/store"
	.type .str.312, @object
	.size .str.312, 36

.str.313:
	.asciz "integer"
	.type .str.313, @object
	.size .str.313, 8

.str.314:
	.asciz "Missing close paren"
	.type .str.314, @object
	.size .str.314, 20

.str.315:
	.asciz "Unimplemented instruction %s"
	.type .str.315, @object
	.size .str.315, 29

.str.316:
	.asciz "fmadd_s"
	.type .str.316, @object
	.size .str.316, 8

.str.317:
	.asciz "Unimplemented instruction %s"
	.type .str.317, @object
	.size .str.317, 29

.str.318:
	.asciz "fmsub_s"
	.type .str.318, @object
	.size .str.318, 8

.str.319:
	.asciz "Unimplemented instruction %s"
	.type .str.319, @object
	.size .str.319, 29

.str.320:
	.asciz "fnmsub_s"
	.type .str.320, @object
	.size .str.320, 9

.str.321:
	.asciz "Unimplemented instruction %s"
	.type .str.321, @object
	.size .str.321, 29

.str.322:
	.asciz "fnmadd_s"
	.type .str.322, @object
	.size .str.322, 9

.str.323:
	.asciz "float"
	.type .str.323, @object
	.size .str.323, 6

.str.324:
	.asciz "float"
	.type .str.324, @object
	.size .str.324, 6

.str.325:
	.asciz "float"
	.type .str.325, @object
	.size .str.325, 6

.str.326:
	.asciz "float"
	.type .str.326, @object
	.size .str.326, 6

.str.327:
	.asciz "Unimplemented instruction %s"
	.type .str.327, @object
	.size .str.327, 29

.str.328:
	.asciz "fsqrt_s"
	.type .str.328, @object
	.size .str.328, 8

.str.329:
	.asciz "float"
	.type .str.329, @object
	.size .str.329, 6

.str.330:
	.asciz "float"
	.type .str.330, @object
	.size .str.330, 6

.str.331:
	.asciz "float"
	.type .str.331, @object
	.size .str.331, 6

.str.332:
	.asciz "float"
	.type .str.332, @object
	.size .str.332, 6

.str.333:
	.asciz "float"
	.type .str.333, @object
	.size .str.333, 6

.str.334:
	.asciz "integer"
	.type .str.334, @object
	.size .str.334, 8

.str.335:
	.asciz "float"
	.type .str.335, @object
	.size .str.335, 6

.str.336:
	.asciz "integer"
	.type .str.336, @object
	.size .str.336, 8

.str.337:
	.asciz "float"
	.type .str.337, @object
	.size .str.337, 6

.str.338:
	.asciz "integer"
	.type .str.338, @object
	.size .str.338, 8

.str.339:
	.asciz "integer"
	.type .str.339, @object
	.size .str.339, 8

.str.340:
	.asciz "float"
	.type .str.340, @object
	.size .str.340, 6

.str.341:
	.asciz "integer"
	.type .str.341, @object
	.size .str.341, 8

.str.342:
	.asciz "float"
	.type .str.342, @object
	.size .str.342, 6

.str.343:
	.asciz "integer"
	.type .str.343, @object
	.size .str.343, 8

.str.344:
	.asciz "float"
	.type .str.344, @object
	.size .str.344, 6

.str.345:
	.asciz "Unimplemented instruction %s"
	.type .str.345, @object
	.size .str.345, 29

.str.346:
	.asciz "fclass_s"
	.type .str.346, @object
	.size .str.346, 9

.str.347:
	.asciz "float"
	.type .str.347, @object
	.size .str.347, 6

.str.348:
	.asciz "integer"
	.type .str.348, @object
	.size .str.348, 8

.str.349:
	.asciz "float"
	.type .str.349, @object
	.size .str.349, 6

.str.350:
	.asciz "integer"
	.type .str.350, @object
	.size .str.350, 8

.str.351:
	.asciz "float"
	.type .str.351, @object
	.size .str.351, 6

.str.352:
	.asciz "integer"
	.type .str.352, @object
	.size .str.352, 8

.str.353:
	.asciz "integer"
	.type .str.353, @object
	.size .str.353, 8

.str.354:
	.asciz "float"
	.type .str.354, @object
	.size .str.354, 6

.str.355:
	.asciz "integer"
	.type .str.355, @object
	.size .str.355, 8

.str.356:
	.asciz "float"
	.type .str.356, @object
	.size .str.356, 6

.str.357:
	.asciz "float"
	.type .str.357, @object
	.size .str.357, 6

.str.358:
	.asciz "integer"
	.type .str.358, @object
	.size .str.358, 8

.str.359:
	.asciz "float"
	.type .str.359, @object
	.size .str.359, 6

.str.360:
	.asciz "integer"
	.type .str.360, @object
	.size .str.360, 8

.str.361:
	.asciz "float"
	.type .str.361, @object
	.size .str.361, 6

.str.362:
	.asciz "Missing comma"
	.type .str.362, @object
	.size .str.362, 14

.str.363:
	.asciz "Expected offset(reg) for load/store"
	.type .str.363, @object
	.size .str.363, 36

.str.364:
	.asciz "integer"
	.type .str.364, @object
	.size .str.364, 8

.str.365:
	.asciz "Missing close paren"
	.type .str.365, @object
	.size .str.365, 20

.str.366:
	.asciz "float"
	.type .str.366, @object
	.size .str.366, 6

.str.367:
	.asciz "Missing comma"
	.type .str.367, @object
	.size .str.367, 14

.str.368:
	.asciz "Expected offset(reg) for load/store"
	.type .str.368, @object
	.size .str.368, 36

.str.369:
	.asciz "integer"
	.type .str.369, @object
	.size .str.369, 8

.str.370:
	.asciz "Missing close paren"
	.type .str.370, @object
	.size .str.370, 20

.str.371:
	.asciz "Unimplemented instruction %s"
	.type .str.371, @object
	.size .str.371, 29

.str.372:
	.asciz "fmadd_d"
	.type .str.372, @object
	.size .str.372, 8

.str.373:
	.asciz "Unimplemented instruction %s"
	.type .str.373, @object
	.size .str.373, 29

.str.374:
	.asciz "fmsub_d"
	.type .str.374, @object
	.size .str.374, 8

.str.375:
	.asciz "Unimplemented instruction %s"
	.type .str.375, @object
	.size .str.375, 29

.str.376:
	.asciz "fnmsub_d"
	.type .str.376, @object
	.size .str.376, 9

.str.377:
	.asciz "Unimplemented instruction %s"
	.type .str.377, @object
	.size .str.377, 29

.str.378:
	.asciz "fnmadd_d"
	.type .str.378, @object
	.size .str.378, 9

.str.379:
	.asciz "float"
	.type .str.379, @object
	.size .str.379, 6

.str.380:
	.asciz "float"
	.type .str.380, @object
	.size .str.380, 6

.str.381:
	.asciz "float"
	.type .str.381, @object
	.size .str.381, 6

.str.382:
	.asciz "float"
	.type .str.382, @object
	.size .str.382, 6

.str.383:
	.asciz "Unimplemented instruction %s"
	.type .str.383, @object
	.size .str.383, 29

.str.384:
	.asciz "fsqrt_d"
	.type .str.384, @object
	.size .str.384, 8

.str.385:
	.asciz "float"
	.type .str.385, @object
	.size .str.385, 6

.str.386:
	.asciz "float"
	.type .str.386, @object
	.size .str.386, 6

.str.387:
	.asciz "float"
	.type .str.387, @object
	.size .str.387, 6

.str.388:
	.asciz "float"
	.type .str.388, @object
	.size .str.388, 6

.str.389:
	.asciz "float"
	.type .str.389, @object
	.size .str.389, 6

.str.390:
	.asciz "float"
	.type .str.390, @object
	.size .str.390, 6

.str.391:
	.asciz "float"
	.type .str.391, @object
	.size .str.391, 6

.str.392:
	.asciz "float"
	.type .str.392, @object
	.size .str.392, 6

.str.393:
	.asciz "float"
	.type .str.393, @object
	.size .str.393, 6

.str.394:
	.asciz "integer"
	.type .str.394, @object
	.size .str.394, 8

.str.395:
	.asciz "float"
	.type .str.395, @object
	.size .str.395, 6

.str.396:
	.asciz "integer"
	.type .str.396, @object
	.size .str.396, 8

.str.397:
	.asciz "float"
	.type .str.397, @object
	.size .str.397, 6

.str.398:
	.asciz "integer"
	.type .str.398, @object
	.size .str.398, 8

.str.399:
	.asciz "float"
	.type .str.399, @object
	.size .str.399, 6

.str.400:
	.asciz "Unimplemented instruction %s"
	.type .str.400, @object
	.size .str.400, 29

.str.401:
	.asciz "fclass_d"
	.type .str.401, @object
	.size .str.401, 9

.str.402:
	.asciz "integer"
	.type .str.402, @object
	.size .str.402, 8

.str.403:
	.asciz "float"
	.type .str.403, @object
	.size .str.403, 6

.str.404:
	.asciz "integer"
	.type .str.404, @object
	.size .str.404, 8

.str.405:
	.asciz "float"
	.type .str.405, @object
	.size .str.405, 6

.str.406:
	.asciz "float"
	.type .str.406, @object
	.size .str.406, 6

.str.407:
	.asciz "integer"
	.type .str.407, @object
	.size .str.407, 8

.str.408:
	.asciz "float"
	.type .str.408, @object
	.size .str.408, 6

.str.409:
	.asciz "integer"
	.type .str.409, @object
	.size .str.409, 8

.str.410:
	.asciz "integer"
	.type .str.410, @object
	.size .str.410, 8

.str.411:
	.asciz "float"
	.type .str.411, @object
	.size .str.411, 6

.str.412:
	.asciz "integer"
	.type .str.412, @object
	.size .str.412, 8

.str.413:
	.asciz "float"
	.type .str.413, @object
	.size .str.413, 6

.str.414:
	.asciz "integer"
	.type .str.414, @object
	.size .str.414, 8

.str.415:
	.asciz "float"
	.type .str.415, @object
	.size .str.415, 6

.str.416:
	.asciz "float"
	.type .str.416, @object
	.size .str.416, 6

.str.417:
	.asciz "integer"
	.type .str.417, @object
	.size .str.417, 8

.str.418:
	.asciz "float"
	.type .str.418, @object
	.size .str.418, 6

.str.419:
	.asciz "integer"
	.type .str.419, @object
	.size .str.419, 8

.str.420:
	.asciz "float"
	.type .str.420, @object
	.size .str.420, 6

.str.421:
	.asciz "integer"
	.type .str.421, @object
	.size .str.421, 8

.str.422:
	.asciz "integer"
	.type .str.422, @object
	.size .str.422, 8

.str.423:
	.asciz "integer"
	.type .str.423, @object
	.size .str.423, 8

.str.424:
	.asciz "float"
	.type .str.424, @object
	.size .str.424, 6

.str.425:
	.asciz "float"
	.type .str.425, @object
	.size .str.425, 6

.str.426:
	.asciz "integer"
	.type .str.426, @object
	.size .str.426, 8

.str.427:
	.asciz "Missing comma"
	.type .str.427, @object
	.size .str.427, 14

.str.428:
	.asciz "integer"
	.type .str.428, @object
	.size .str.428, 8

.str.429:
	.asciz "Missing comma"
	.type .str.429, @object
	.size .str.429, 14

.str.430:
	.asciz ".la_label_%" PRId64 ""
	.type .str.430, @object
	.size .str.430, 15

.str.431:
	.asciz "Expected symbol name for la instruction"
	.type .str.431, @object
	.size .str.431, 40

.str.432:
	.asciz "integer"
	.type .str.432, @object
	.size .str.432, 8

.str.433:
	.asciz "Missing comma"
	.type .str.433, @object
	.size .str.433, 14

.str.434:
	.asciz ".la_label_%" PRId64 ""
	.type .str.434, @object
	.size .str.434, 15

.str.435:
	.asciz "Expected symbol name for lla instruction"
	.type .str.435, @object
	.size .str.435, 41

.str.436:
	.asciz "integer"
	.type .str.436, @object
	.size .str.436, 8

.str.437:
	.asciz "integer"
	.type .str.437, @object
	.size .str.437, 8

.str.438:
	.asciz "integer"
	.type .str.438, @object
	.size .str.438, 8

.str.439:
	.asciz "integer"
	.type .str.439, @object
	.size .str.439, 8

.str.440:
	.asciz "integer"
	.type .str.440, @object
	.size .str.440, 8

.str.441:
	.asciz "integer"
	.type .str.441, @object
	.size .str.441, 8

.str.442:
	.asciz "Missing comma"
	.type .str.442, @object
	.size .str.442, 14

.str.443:
	.asciz "Unimplemented instruction %s"
	.type .str.443, @object
	.size .str.443, 29

.str.444:
	.asciz "rcall"
	.type .str.444, @object
	.size .str.444, 6

.str.445:
	.asciz "Unimplemented instruction %s"
	.type .str.445, @object
	.size .str.445, 29

.str.446:
	.asciz "callf"
	.type .str.446, @object
	.size .str.446, 6

.str.447:
	.asciz "Unimplemented instruction %s"
	.type .str.447, @object
	.size .str.447, 29

.str.448:
	.asciz "rcallf"
	.type .str.448, @object
	.size .str.448, 7

