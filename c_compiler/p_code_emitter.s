	.file   "p_code_emitter.c"
	.text
	.option pic
.PCbegin:
	.local  IsPrintable
	.type IsPrintable, @function

IsPrintable:

	// *** Basic block 0

	.global TargetIsConst
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
	call        TargetIsConst

	// *** Basic block 1

	beqz        a0, .IsPrintable_label_29

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.IsPrintable_label_26:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.IsPrintable_label_29:
	lw          t0, 16(s1)
	li          t1, 3		// 0x3 ASCII \x3
	beq         t0, t1, .IsPrintable_label_92

	// *** Basic block 5

	li          t1, 4		// 0x4 ASCII \x4
	beq         t0, t1, .IsPrintable_label_87

	// *** Basic block 6

	li          t1, 23		// 0x17 ASCII \x17
	beq         t0, t1, .IsPrintable_label_88

	// *** Basic block 7

	li          t1, 24		// 0x18 ASCII \x18
	beq         t0, t1, .IsPrintable_label_89

	// *** Basic block 8

	li          t1, 25		// 0x19 ASCII \x19
	beq         t0, t1, .IsPrintable_label_91

	// *** Basic block 9

	li          t1, 26		// 0x1a ASCII \x1a
	beq         t0, t1, .IsPrintable_label_94

	// *** Basic block 10

	li          t1, 27		// 0x1b ASCII \x1b
	beq         t0, t1, .IsPrintable_label_95

	// *** Basic block 11

	li          t1, 28		// 0x1c ASCII \x1c
	beq         t0, t1, .IsPrintable_label_96

	// *** Basic block 12

	li          t1, 29		// 0x1d ASCII \x1d
	beq         t0, t1, .IsPrintable_label_93

	// *** Basic block 13

	li          t1, 35		// 0x23 ASCII '#'
	beq         t0, t1, .IsPrintable_label_90

	// *** Basic block 14

.IsPrintable_label_82:

	// *** Basic block 15

.IsPrintable_label_83:
	li          a0, 1		// 0x1 ASCII \x1
	j           .IsPrintable_label_26

	// *** Basic block 16

.IsPrintable_label_87:

	// *** Basic block 17

.IsPrintable_label_88:

	// *** Basic block 18

.IsPrintable_label_89:

	// *** Basic block 19

.IsPrintable_label_90:

	// *** Basic block 20

.IsPrintable_label_91:

	// *** Basic block 21

.IsPrintable_label_92:

	// *** Basic block 22

.IsPrintable_label_93:

	// *** Basic block 23

.IsPrintable_label_94:

	// *** Basic block 24

.IsPrintable_label_95:

	// *** Basic block 25

.IsPrintable_label_96:
	mv          a0, x0
	j           .IsPrintable_label_26
.func_end_IsPrintable:
	.size IsPrintable, .func_end_IsPrintable-IsPrintable

	.local  PrintRmov
	.type PrintRmov, @function

PrintRmov:

	// *** Basic block 0

	.global printf
	.global abort
	.global fprintf
	.global PCodeRegisterName
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
	mv          s1, a1
	mv          s2, a2
	addi        s3, s1, 40
	ld          s4, 40(s1)
	beq         s4, x0, .PrintRmov_label_48

	// *** Basic block 1

	j           .PrintRmov_label_65

	// *** Basic block 2

.PrintRmov_label_48:
	lla         a0, .str.1
	lla         a1, .str.2
	lla         a3, .str.3
	li          t0, 51		// 0x33 ASCII '3'
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.PrintRmov_label_65:
	ld          s3, 8(s3)
	beq         s3, x0, .PrintRmov_label_72

	// *** Basic block 5

	j           .PrintRmov_label_87

	// *** Basic block 6

.PrintRmov_label_72:
	lla         a0, .str.4
	lla         a1, .str.5
	lla         a3, .str.6
	li          t0, 52		// 0x34 ASCII '4'
	mv          a2, t0
	call        printf

	// *** Basic block 7

	call        abort

	// *** Basic block 8

.PrintRmov_label_87:
	ld          a0, 32(s4)
	beq         a0, x0, .PrintRmov_label_94

	// *** Basic block 9

	j           .PrintRmov_label_109

	// *** Basic block 10

.PrintRmov_label_94:
	lla         a0, .str.7
	lla         a1, .str.8
	lla         a3, .str.9
	li          t0, 53		// 0x35 ASCII '5'
	mv          a2, t0
	call        printf

	// *** Basic block 11

	call        abort

	// *** Basic block 12

.PrintRmov_label_109:
	ld          a0, 32(s3)
	beq         a0, x0, .PrintRmov_label_116

	// *** Basic block 13

	j           .PrintRmov_label_131

	// *** Basic block 14

.PrintRmov_label_116:
	lla         a0, .str.10
	lla         a1, .str.11
	lla         a3, .str.12
	li          t0, 54		// 0x36 ASCII '6'
	mv          a2, t0
	call        printf

	// *** Basic block 15

	call        abort

	// *** Basic block 16

.PrintRmov_label_131:
	bne         a0, a0, .PrintRmov_label_138

	// *** Basic block 17

.PrintRmov_label_135:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 18

.PrintRmov_label_138:
	lla         s3, .str.13
	lw          s4, 16(s1)
	li          t0, 18		// 0x12 ASCII \x12
	blt         s4, t0, .PrintRmov_label_172

	// *** Basic block 19

	li          t0, 20		// 0x14 ASCII \x14
	blt         t0, s4, .PrintRmov_label_172

	// *** Basic block 20

	addi        t0, s4, -18
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 21

	j           .PrintRmov_label_160

	// *** Basic block 22

	j           .PrintRmov_label_164

	// *** Basic block 23

	j           .PrintRmov_label_168

	// *** Basic block 24

.PrintRmov_label_160:
	lla         s3, .str.14
	j           .PrintRmov_label_187

	// *** Basic block 25

.PrintRmov_label_164:
	lla         s3, .str.15
	j           .PrintRmov_label_187

	// *** Basic block 26

.PrintRmov_label_168:
	lla         s3, .str.16
	j           .PrintRmov_label_187

	// *** Basic block 27

.PrintRmov_label_172:
	lla         a0, .str.17
	lla         a1, .str.18
	lla         a3, .str.19
	li          t0, 73		// 0x49 ASCII 'I'
	mv          a2, t0
	call        printf

	// *** Basic block 28

	call        abort

	// *** Basic block 29

.PrintRmov_label_187:
	lla         s1, .str.20
	addi        a1, s0, -32
	li          s4, 8		// 0x8 ASCII \x8
	mv          a2, s4
	call        PCodeRegisterName

	// *** Basic block 30

	addi        a1, s0, -24
	mv          a2, s4
	call        PCodeRegisterName

	// *** Basic block 31

	mv          a4, a0
	mv          a3, a0
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        fprintf

	// *** Basic block 32

	j           .PrintRmov_label_135
.func_end_PrintRmov:
	.size PrintRmov, .func_end_PrintRmov-PrintRmov

	.local  SaveRegisters
	.type SaveRegisters, @function

SaveRegisters:

	// *** Basic block 0

	.global BitSetExpand
	.global fprintf
	.global VectorClear
	.global VectorDestruct
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -48(s0)
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
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -48(s0)
	ld          t0, 8(s1)
	li          t1, 18440		// 0x4808
	add         a0, t0, t1
	addi        a1, s0, -48
	call        BitSetExpand

	// *** Basic block 1

	ld          s3, -48(s0)
	mv          s4, x0
	addi        t0, s0, -48
	ld          s5, 8(t0)
	bge         x0, s5, .SaveRegisters_label_66

	// *** Basic block 2

.SaveRegisters_label_47:
	slli        t0, s4, 3
	add         t0, s3, t0
	ld          s3, 0(t0)
	lla         a1, .str.21
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 3

.SaveRegisters_label_62:
	addi        s4, s4, 1
	bge         s4, s5, .SaveRegisters_label_47

	// *** Basic block 4

.SaveRegisters_label_66:
	addi        a0, s0, -48
	call        VectorClear

	// *** Basic block 5

	ld          t0, 8(s1)
	li          t1, 18456		// 0x4818
	add         a0, t0, t1
	addi        a1, s0, -48
	call        BitSetExpand

	// *** Basic block 6

	ld          s3, -48(s0)
	mv          s5, x0
	addi        t0, s0, -48
	ld          s6, 8(t0)
	bge         x0, s6, .SaveRegisters_label_103

	// *** Basic block 7

.SaveRegisters_label_86:
	slli        t0, s5, 3
	add         t0, s3, t0
	ld          s3, 0(t0)
	lla         a1, .str.22
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 8

.SaveRegisters_label_99:
	addi        s5, s5, 1
	bge         s5, s6, .SaveRegisters_label_86

	// *** Basic block 9

.SaveRegisters_label_103:
	addi        a0, s0, -48
	call        VectorClear

	// *** Basic block 10

	ld          t0, 8(s1)
	li          t1, 18472		// 0x4828
	add         a0, t0, t1
	addi        a1, s0, -48
	call        BitSetExpand

	// *** Basic block 11

	ld          s3, -48(s0)
	mv          s6, x0
	addi        t0, s0, -48
	ld          s7, 8(t0)
	bge         x0, s7, .SaveRegisters_label_140

	// *** Basic block 12

.SaveRegisters_label_123:
	slli        t0, s6, 3
	add         t0, s3, t0
	ld          s1, 0(t0)
	lla         a1, .str.23
	mv          a2, s1
	mv          a0, s2
	call        fprintf

	// *** Basic block 13

.SaveRegisters_label_136:
	addi        s6, s6, 1
	bge         s6, s7, .SaveRegisters_label_123

	// *** Basic block 14

.SaveRegisters_label_140:
	addi        a0, s0, -48
	call        VectorDestruct

	// *** Basic block 15

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
.func_end_SaveRegisters:
	.size SaveRegisters, .func_end_SaveRegisters-SaveRegisters

	.local  RestoreRegisters
	.type RestoreRegisters, @function

RestoreRegisters:

	// *** Basic block 0

	.global BitSetExpand
	.global fprintf
	.global VectorClear
	.global VectorDestruct
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -48(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -48(s0)
	ld          t0, 8(s1)
	li          t1, 18472		// 0x4828
	add         a0, t0, t1
	addi        a1, s0, -48
	call        BitSetExpand

	// *** Basic block 1

	ld          s3, -48(s0)
	addi        t0, s0, -48
	ld          s4, 8(t0)
	bge         x0, s4, .RestoreRegisters_label_66

	// *** Basic block 2

.RestoreRegisters_label_46:
	addi        s4, s4, -1
	slli        t0, s4, 3
	add         t0, s3, t0
	ld          s3, 0(t0)
	lla         a1, .str.24
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 3

.RestoreRegisters_label_63:
	bge         x0, s4, .RestoreRegisters_label_46

	// *** Basic block 4

.RestoreRegisters_label_66:
	addi        a0, s0, -48
	call        VectorClear

	// *** Basic block 5

	ld          t0, 8(s1)
	li          t1, 18456		// 0x4818
	add         a0, t0, t1
	addi        a1, s0, -48
	call        BitSetExpand

	// *** Basic block 6

	ld          s3, -48(s0)
	addi        t0, s0, -48
	ld          s4, 8(t0)
	bge         x0, s4, .RestoreRegisters_label_102

	// *** Basic block 7

.RestoreRegisters_label_85:
	addi        s4, s4, -1
	slli        t0, s4, 3
	add         t0, s3, t0
	ld          s3, 0(t0)
	lla         a1, .str.25
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 8

.RestoreRegisters_label_99:
	bge         x0, s4, .RestoreRegisters_label_85

	// *** Basic block 9

.RestoreRegisters_label_102:
	addi        a0, s0, -48
	call        VectorClear

	// *** Basic block 10

	ld          t0, 8(s1)
	li          t1, 18440		// 0x4808
	add         a0, t0, t1
	addi        a1, s0, -48
	call        BitSetExpand

	// *** Basic block 11

	ld          s3, -48(s0)
	addi        t0, s0, -48
	ld          s4, 8(t0)
	bge         x0, s4, .RestoreRegisters_label_138

	// *** Basic block 12

.RestoreRegisters_label_121:
	addi        s4, s4, -1
	slli        t0, s4, 3
	add         t0, s3, t0
	ld          s1, 0(t0)
	lla         a1, .str.26
	mv          a2, s1
	mv          a0, s2
	call        fprintf

	// *** Basic block 13

.RestoreRegisters_label_135:
	bge         x0, s4, .RestoreRegisters_label_121

	// *** Basic block 14

.RestoreRegisters_label_138:
	addi        a0, s0, -48
	call        VectorDestruct

	// *** Basic block 15

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_RestoreRegisters:
	.size RestoreRegisters, .func_end_RestoreRegisters-RestoreRegisters

	.local  PrintInstruction
	.type PrintInstruction, @function

PrintInstruction:

	// *** Basic block 0

	.global fprintf
	.local IsPrintable
	.local PrintRmov
	.global StorageIs
	.global printf
	.global abort
	.global PCodeRegisterName
	.local SaveRegisters
	.local RestoreRegisters
	.global CompilerFindStringLiteral
	.global SourceLocationNumbers
	.global PCodeOpcodeName
	.global TargetIsConst
	.global TargetIntValue
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -48(s0)
	// Spilled register region: 8 bytes at -56(s0) to -48(s0)
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
	mv          s2, a3
	mv          s3, a2
	sd          s3, -56(s0)	// Spilled @135
	mv          s4, a0
	lw          s5, 16(s1)
	li          t0, 22		// 0x16 ASCII \x16
	bne         s5, t0, .PrintInstruction_label_167

	// *** Basic block 1

	addi        s6, s1, 40
	addi        s7, s0, -48
	lla         a1, .str.27
	lw          a3, 20(s1)
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 2

.PrintInstruction_label_164:
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

	// *** Basic block 3

.PrintInstruction_label_167:
	li          t0, 32		// 0x20 ASCII ' '
	bne         s5, t0, .PrintInstruction_label_185

	// *** Basic block 4

	mv          s6, s1
	lla         a1, .str.28
	ld          a2, 112(s6)
	mv          a0, s2
	call        fprintf

	// *** Basic block 5

	j           .PrintInstruction_label_164

	// *** Basic block 6

.PrintInstruction_label_185:
	mv          a0, s1
	call        IsPrintable

	// *** Basic block 7

	not         t0, a0
	beqz        t0, .PrintInstruction_label_192

	// *** Basic block 8

	j           .PrintInstruction_label_164

	// *** Basic block 9

.PrintInstruction_label_192:
	beqz        s5, .PrintInstruction_label_395

	// *** Basic block 10

	li          s7, 1		// 0x1 ASCII \x1
	beq         s5, s7, .PrintInstruction_label_402

	// *** Basic block 11

	li          s8, 2		// 0x2 ASCII \x2
	beq         s5, s8, .PrintInstruction_label_288

	// *** Basic block 12

	li          t0, 18		// 0x12 ASCII \x12
	beq         s5, t0, .PrintInstruction_label_277

	// *** Basic block 13

	li          t0, 19		// 0x13 ASCII \x13
	beq         s5, t0, .PrintInstruction_label_278

	// *** Basic block 14

	li          t0, 20		// 0x14 ASCII \x14
	beq         s5, t0, .PrintInstruction_label_279

	// *** Basic block 15

	li          t0, 30		// 0x1e ASCII \x1e
	beq         s5, t0, .PrintInstruction_label_409

	// *** Basic block 16

	li          t0, 31		// 0x1f ASCII \x1f
	beq         s5, t0, .PrintInstruction_label_453

	// *** Basic block 17

	li          t0, 129		// 0x81 ASCII \x81
	beq         s5, t0, .PrintInstruction_label_323

	// *** Basic block 18

	li          t0, 130		// 0x82 ASCII \x82
	beq         s5, t0, .PrintInstruction_label_324

	// *** Basic block 19

	li          t0, 131		// 0x83 ASCII \x83
	beq         s5, t0, .PrintInstruction_label_325

	// *** Basic block 20

	li          t0, 132		// 0x84 ASCII \x84
	beq         s5, t0, .PrintInstruction_label_367

	// *** Basic block 21

	li          t0, 133		// 0x85 ASCII \x85
	beq         s5, t0, .PrintInstruction_label_368

	// *** Basic block 22

	li          t0, 134		// 0x86 ASCII \x86
	beq         s5, t0, .PrintInstruction_label_369

	// *** Basic block 23

.PrintInstruction_label_260:

	// *** Basic block 24

.PrintInstruction_label_261:
	lla         a1, .str.43
	mv          a0, s5
	call        PCodeOpcodeName

	// *** Basic block 25

	mv          a2, a0
	mv          a0, s2
	call        fprintf

	// *** Basic block 26

	li          s9, 55		// 0x37 ASCII '7'
	blt         s5, s9, .PrintInstruction_label_531

	// *** Basic block 27

	j           .PrintInstruction_label_484

	// *** Basic block 28

.PrintInstruction_label_277:

	// *** Basic block 29

.PrintInstruction_label_278:

	// *** Basic block 30

.PrintInstruction_label_279:
	mv          a2, s2
	mv          a1, s1
	mv          a0, s4
	call        PrintRmov

	// *** Basic block 31

	j           .PrintInstruction_label_164

	// *** Basic block 32

.PrintInstruction_label_288:
	mv          s7, s1
	ld          s10, 112(s7)
	lw          a0, 48(s10)
	mv          a1, s8
	call        StorageIs

	// *** Basic block 33

	beqz        a0, .PrintInstruction_label_311

	// *** Basic block 34

	lla         a1, .str.29
	ld          a2, 16(s10)
	mv          a0, s2
	call        fprintf

	// *** Basic block 35

	j           .PrintInstruction_label_321

	// *** Basic block 36

.PrintInstruction_label_311:
	lla         a1, .str.30
	ld          a2, 16(s10)
	mv          a0, s2
	call        fprintf

	// *** Basic block 37

.PrintInstruction_label_321:
	j           .PrintInstruction_label_164

	// *** Basic block 38

.PrintInstruction_label_323:

	// *** Basic block 39

.PrintInstruction_label_324:

	// *** Basic block 40

.PrintInstruction_label_325:
	ld          s3, 40(s1)
	lw          t0, 16(s3)
	bne         t0, s8, .PrintInstruction_label_335

	// *** Basic block 41

	j           .PrintInstruction_label_350

	// *** Basic block 42

.PrintInstruction_label_335:
	lla         a0, .str.31
	lla         a1, .str.32
	lla         a3, .str.33
	li          t0, 176		// 0xb0 ASCII \xb0
	mv          a2, t0
	call        printf

	// *** Basic block 43

	call        abort

	// *** Basic block 44

.PrintInstruction_label_350:
	lla         a1, .str.34
	lla         a2, .str.35
	ld          t0, 112(s3)
	ld          a3, 16(t0)
	mv          a0, s2
	call        fprintf

	// *** Basic block 45

	j           .PrintInstruction_label_164

	// *** Basic block 46

.PrintInstruction_label_367:

	// *** Basic block 47

.PrintInstruction_label_368:

	// *** Basic block 48

.PrintInstruction_label_369:
	lla         s3, .str.36
	lla         s5, .str.37
	ld          t0, 40(s1)
	ld          a0, 32(t0)
	addi        a1, s0, -48
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	call        PCodeRegisterName

	// *** Basic block 49

	mv          a3, a0
	mv          a2, s5
	mv          a1, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 50

	j           .PrintInstruction_label_164

	// *** Basic block 51

.PrintInstruction_label_395:
	mv          a1, s2
	mv          a0, s4
	call        SaveRegisters

	// *** Basic block 52

	j           .PrintInstruction_label_164

	// *** Basic block 53

.PrintInstruction_label_402:
	mv          a1, s2
	mv          a0, s4
	call        RestoreRegisters

	// *** Basic block 54

	j           .PrintInstruction_label_164

	// *** Basic block 55

.PrintInstruction_label_409:
	ld          s5, 40(s1)
	lw          a0, 112(s5)
	call        CompilerFindStringLiteral

	// *** Basic block 56

	mv          s5, a0
	beq         s5, x0, .PrintInstruction_label_423

	// *** Basic block 57

	j           .PrintInstruction_label_438

	// *** Basic block 58

.PrintInstruction_label_423:
	lla         a0, .str.38
	lla         a1, .str.39
	lla         a3, .str.40
	li          t0, 201		// 0xc9 ASCII \xc9
	mv          a2, t0
	call        printf

	// *** Basic block 59

	call        abort

	// *** Basic block 60

.PrintInstruction_label_438:
	lla         a1, .str.41
	addi        t0, s5, 8
	ld          a2, 16(t0)
	mv          a0, s2
	call        fprintf

	// *** Basic block 61

	sb          s7, 48(s5)
	j           .PrintInstruction_label_164

	// *** Basic block 62

.PrintInstruction_label_453:
	mv          s3, s1
	ld          a0, 112(s3)
	addi        a1, s0, -40
	addi        a2, s0, -36
	addi        a3, s0, -32
	call        SourceLocationNumbers

	// *** Basic block 63

	lla         a1, .str.42
	lw          t0, -40(s0)
	addi        a2, t0, 1
	lw          a3, -36(s0)
	lw          t0, -32(s0)
	addi        a4, t0, 1
	mv          a0, s2
	call        fprintf

	// *** Basic block 64

	j           .PrintInstruction_label_164

	// *** Basic block 65

.PrintInstruction_label_484:
	beq         s5, s9, .PrintInstruction_label_728

	// *** Basic block 66

	li          t0, 56		// 0x38 ASCII '8'
	beq         s5, t0, .PrintInstruction_label_729

	// *** Basic block 67

	li          t0, 57		// 0x39 ASCII '9'
	beq         s5, t0, .PrintInstruction_label_730

	// *** Basic block 68

	li          t0, 58		// 0x3a ASCII ':'
	beq         s5, t0, .PrintInstruction_label_732

	// *** Basic block 69

	li          t0, 59		// 0x3b ASCII ';'
	beq         s5, t0, .PrintInstruction_label_733

	// *** Basic block 70

	li          t0, 60		// 0x3c ASCII '<'
	beq         s5, t0, .PrintInstruction_label_731

	// *** Basic block 71

	li          t0, 110		// 0x6e ASCII 'n'
	beq         s5, t0, .PrintInstruction_label_898

	// *** Basic block 72

	li          t0, 111		// 0x6f ASCII 'o'
	beq         s5, t0, .PrintInstruction_label_897

	// *** Basic block 73

	li          t0, 112		// 0x70 ASCII 'p'
	beq         s5, t0, .PrintInstruction_label_987

	// *** Basic block 74

	j           .PrintInstruction_label_1022

	// *** Basic block 75

.PrintInstruction_label_531:
	li          t0, 46		// 0x2e ASCII '.'
	beq         s5, t0, .PrintInstruction_label_577

	// *** Basic block 76

	li          t0, 47		// 0x2f ASCII '/'
	beq         s5, t0, .PrintInstruction_label_578

	// *** Basic block 77

	li          t0, 48		// 0x30 ASCII '0'
	beq         s5, t0, .PrintInstruction_label_579

	// *** Basic block 78

	li          t0, 49		// 0x31 ASCII '1'
	beq         s5, t0, .PrintInstruction_label_580

	// *** Basic block 79

	li          t0, 50		// 0x32 ASCII '2'
	beq         s5, t0, .PrintInstruction_label_581

	// *** Basic block 80

	li          t0, 51		// 0x33 ASCII '3'
	beq         s5, t0, .PrintInstruction_label_582

	// *** Basic block 81

	li          t0, 52		// 0x34 ASCII '4'
	beq         s5, t0, .PrintInstruction_label_584

	// *** Basic block 82

	li          t0, 53		// 0x35 ASCII '5'
	beq         s5, t0, .PrintInstruction_label_583

	// *** Basic block 83

	li          t0, 54		// 0x36 ASCII '6'
	beq         s5, t0, .PrintInstruction_label_585

	// *** Basic block 84

	j           .PrintInstruction_label_1022

	// *** Basic block 85

.PrintInstruction_label_577:

	// *** Basic block 86

.PrintInstruction_label_578:

	// *** Basic block 87

.PrintInstruction_label_579:

	// *** Basic block 88

.PrintInstruction_label_580:

	// *** Basic block 89

.PrintInstruction_label_581:

	// *** Basic block 90

.PrintInstruction_label_582:

	// *** Basic block 91

.PrintInstruction_label_583:

	// *** Basic block 92

.PrintInstruction_label_584:

	// *** Basic block 93

.PrintInstruction_label_585:
	addi        s5, s1, 40
	ld          s9, 40(s1)
	beq         s9, x0, .PrintInstruction_label_592

	// *** Basic block 94

	j           .PrintInstruction_label_607

	// *** Basic block 95

.PrintInstruction_label_592:
	lla         a0, .str.44
	lla         a1, .str.45
	lla         a3, .str.46
	li          t0, 237		// 0xed ASCII \xed
	mv          a2, t0
	call        printf

	// *** Basic block 96

	call        abort

	// *** Basic block 97

.PrintInstruction_label_607:
	ld          s5, 8(s5)
	beq         s5, x0, .PrintInstruction_label_614

	// *** Basic block 98

	j           .PrintInstruction_label_629

	// *** Basic block 99

.PrintInstruction_label_614:
	lla         a0, .str.47
	lla         a1, .str.48
	lla         a3, .str.49
	li          t0, 238		// 0xee ASCII \xee
	mv          a2, t0
	call        printf

	// *** Basic block 100

	call        abort

	// *** Basic block 101

.PrintInstruction_label_629:
	ld          a0, 32(s1)
	beq         a0, x0, .PrintInstruction_label_636

	// *** Basic block 102

	j           .PrintInstruction_label_651

	// *** Basic block 103

.PrintInstruction_label_636:
	lla         a0, .str.50
	lla         a1, .str.51
	lla         a3, .str.52
	li          t0, 239		// 0xef ASCII \xef
	mv          a2, t0
	call        printf

	// *** Basic block 104

	call        abort

	// *** Basic block 105

.PrintInstruction_label_651:
	ld          a0, 32(s9)
	beq         a0, x0, .PrintInstruction_label_658

	// *** Basic block 106

	j           .PrintInstruction_label_673

	// *** Basic block 107

.PrintInstruction_label_658:
	lla         a0, .str.53
	lla         a1, .str.54
	lla         a3, .str.55
	li          t0, 240		// 0xf0 ASCII \xf0
	mv          a2, t0
	call        printf

	// *** Basic block 108

	call        abort

	// *** Basic block 109

.PrintInstruction_label_673:
	mv          a0, s5
	call        TargetIsConst

	// *** Basic block 110

	beqz        a0, .PrintInstruction_label_680

	// *** Basic block 111

	j           .PrintInstruction_label_695

	// *** Basic block 112

.PrintInstruction_label_680:
	lla         a0, .str.56
	lla         a1, .str.57
	lla         a3, .str.58
	li          t0, 241		// 0xf1 ASCII \xf1
	mv          a2, t0
	call        printf

	// *** Basic block 113

	call        abort

	// *** Basic block 114

.PrintInstruction_label_695:
	lla         s9, .str.59
	addi        a1, s0, -28
	li          s10, 8		// 0x8 ASCII \x8
	mv          a2, s10
	call        PCodeRegisterName

	// *** Basic block 115

	addi        a1, s0, -48
	mv          a2, s10
	call        PCodeRegisterName

	// *** Basic block 116

	mv          a0, s5
	call        TargetIntValue

	// *** Basic block 117

	sext.w      a4, a0
	mv          a3, a0
	mv          a2, a0
	mv          a1, s9
	mv          a0, s2
	call        fprintf

	// *** Basic block 118

	j           .PrintInstruction_label_1175

	// *** Basic block 119

.PrintInstruction_label_728:

	// *** Basic block 120

.PrintInstruction_label_729:

	// *** Basic block 121

.PrintInstruction_label_730:

	// *** Basic block 122

.PrintInstruction_label_731:

	// *** Basic block 123

.PrintInstruction_label_732:

	// *** Basic block 124

.PrintInstruction_label_733:
	addi        s9, s1, 40
	ld          s10, 40(s1)
	beq         s10, x0, .PrintInstruction_label_740

	// *** Basic block 125

	j           .PrintInstruction_label_755

	// *** Basic block 126

.PrintInstruction_label_740:
	lla         a0, .str.60
	lla         a1, .str.61
	lla         a3, .str.62
	li          t0, 255		// 0xff
	mv          a2, t0
	call        printf

	// *** Basic block 127

	call        abort

	// *** Basic block 128

.PrintInstruction_label_755:
	ld          s11, 8(s9)
	beq         s11, x0, .PrintInstruction_label_762

	// *** Basic block 129

	j           .PrintInstruction_label_777

	// *** Basic block 130

.PrintInstruction_label_762:
	lla         a0, .str.63
	lla         a1, .str.64
	lla         a3, .str.65
	li          t0, 256		// 0x100
	mv          a2, t0
	call        printf

	// *** Basic block 131

	call        abort

	// *** Basic block 132

.PrintInstruction_label_777:
	ld          s9, 16(s9)
	beq         s9, x0, .PrintInstruction_label_784

	// *** Basic block 133

	j           .PrintInstruction_label_799

	// *** Basic block 134

.PrintInstruction_label_784:
	lla         a0, .str.66
	lla         a1, .str.67
	lla         a3, .str.68
	li          t0, 257		// 0x101
	mv          a2, t0
	call        printf

	// *** Basic block 135

	call        abort

	// *** Basic block 136

.PrintInstruction_label_799:
	ld          a0, 32(s10)
	beq         a0, x0, .PrintInstruction_label_806

	// *** Basic block 137

	j           .PrintInstruction_label_821

	// *** Basic block 138

.PrintInstruction_label_806:
	lla         a0, .str.69
	lla         a1, .str.70
	lla         a3, .str.71
	li          t0, 258		// 0x102
	mv          a2, t0
	call        printf

	// *** Basic block 139

	call        abort

	// *** Basic block 140

.PrintInstruction_label_821:
	ld          a0, 32(s11)
	beq         a0, x0, .PrintInstruction_label_828

	// *** Basic block 141

	j           .PrintInstruction_label_843

	// *** Basic block 142

.PrintInstruction_label_828:
	lla         a0, .str.72
	lla         a1, .str.73
	lla         a3, .str.74
	li          t0, 259		// 0x103
	mv          a2, t0
	call        printf

	// *** Basic block 143

	call        abort

	// *** Basic block 144

.PrintInstruction_label_843:
	mv          a0, s9
	call        TargetIsConst

	// *** Basic block 145

	beqz        a0, .PrintInstruction_label_850

	// *** Basic block 146

	j           .PrintInstruction_label_865

	// *** Basic block 147

.PrintInstruction_label_850:
	lla         a0, .str.75
	lla         a1, .str.76
	lla         a3, .str.77
	li          t0, 260		// 0x104
	mv          a2, t0
	call        printf

	// *** Basic block 148

	call        abort

	// *** Basic block 149

.PrintInstruction_label_865:
	lla         s10, .str.78
	addi        a1, s0, -28
	li          s11, 8		// 0x8 ASCII \x8
	mv          a2, s11
	call        PCodeRegisterName

	// *** Basic block 150

	addi        a1, s0, -48
	mv          a2, s11
	call        PCodeRegisterName

	// *** Basic block 151

	mv          a0, s9
	call        TargetIntValue

	// *** Basic block 152

	sext.w      a4, a0
	mv          a3, a0
	mv          a2, a0
	mv          a1, s10
	mv          a0, s2
	call        fprintf

	// *** Basic block 153

	j           .PrintInstruction_label_1175

	// *** Basic block 154

.PrintInstruction_label_897:

	// *** Basic block 155

.PrintInstruction_label_898:
	addi        s9, s1, 40
	ld          s10, 40(s1)
	beq         s10, x0, .PrintInstruction_label_905

	// *** Basic block 156

	j           .PrintInstruction_label_920

	// *** Basic block 157

.PrintInstruction_label_905:
	lla         a0, .str.79
	lla         a1, .str.80
	lla         a3, .str.81
	li          t0, 270		// 0x10e
	mv          a2, t0
	call        printf

	// *** Basic block 158

	call        abort

	// *** Basic block 159

.PrintInstruction_label_920:
	ld          s9, 8(s9)
	beq         s9, x0, .PrintInstruction_label_927

	// *** Basic block 160

	j           .PrintInstruction_label_942

	// *** Basic block 161

.PrintInstruction_label_927:
	lla         a0, .str.82
	lla         a1, .str.83
	lla         a3, .str.84
	li          t0, 271		// 0x10f
	mv          a2, t0
	call        printf

	// *** Basic block 162

	call        abort

	// *** Basic block 163

.PrintInstruction_label_942:
	ld          a0, 32(s10)
	beq         a0, x0, .PrintInstruction_label_949

	// *** Basic block 164

	j           .PrintInstruction_label_964

	// *** Basic block 165

.PrintInstruction_label_949:
	lla         a0, .str.85
	lla         a1, .str.86
	lla         a3, .str.87
	li          t0, 272		// 0x110
	mv          a2, t0
	call        printf

	// *** Basic block 166

	call        abort

	// *** Basic block 167

.PrintInstruction_label_964:
	lla         s10, .str.88
	addi        a1, s0, -28
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	call        PCodeRegisterName

	// *** Basic block 168

	lw          a4, 20(s9)
	mv          a3, s3
	mv          a2, a0
	mv          a1, s10
	mv          a0, s2
	call        fprintf

	// *** Basic block 169

	j           .PrintInstruction_label_1175

	// *** Basic block 170

.PrintInstruction_label_987:
	ld          s9, 40(s1)
	beq         s9, x0, .PrintInstruction_label_994

	// *** Basic block 171

	j           .PrintInstruction_label_1009

	// *** Basic block 172

.PrintInstruction_label_994:
	lla         a0, .str.89
	lla         a1, .str.90
	lla         a3, .str.91
	li          t0, 279		// 0x117
	mv          a2, t0
	call        printf

	// *** Basic block 173

	call        abort

	// *** Basic block 174

.PrintInstruction_label_1009:
	lla         a1, .str.92
	lw          a3, 20(s9)
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 175

	j           .PrintInstruction_label_1175

	// *** Basic block 176

.PrintInstruction_label_1022:
	lla         s5, .str.93
	ld          a0, 32(s1)
	beq         a0, x0, .PrintInstruction_label_1049

	// *** Basic block 177

	lla         s9, .str.94
	addi        a1, s0, -28
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	call        PCodeRegisterName

	// *** Basic block 178

	mv          a2, a0
	mv          a1, s9
	mv          a0, s2
	call        fprintf

	// *** Basic block 179

	lla         s5, .str.95

	// *** Basic block 180

.PrintInstruction_label_1049:
	mv          s9, x0

	// *** Basic block 181

.PrintInstruction_label_1053:
	slli        t0, s9, 3
	add         t0, s6, t0
	ld          s10, 0(t0)
	beq         s10, x0, .PrintInstruction_label_1162

	// *** Basic block 182

	mv          a0, s10
	call        TargetIsConst

	// *** Basic block 183

	beqz        a0, .PrintInstruction_label_1079

	// *** Basic block 184

	lla         s11, .str.96
	mv          a0, s10
	call        TargetIntValue

	// *** Basic block 185

	sext.w      a3, a0
	mv          a2, s5
	mv          a1, s11
	mv          a0, s2
	call        fprintf

	// *** Basic block 186

	j           .PrintInstruction_label_1159

	// *** Basic block 187

.PrintInstruction_label_1079:
	lw          s11, 16(s10)
	bne         s11, s8, .PrintInstruction_label_1116

	// *** Basic block 188

	lla         a1, .str.97
	ld          s10, 112(s10)
	ld          a3, 16(s10)
	mv          a2, s5
	mv          a0, s2
	call        fprintf

	// *** Basic block 189

	lw          a0, 48(s10)
	li          t0, 64		// 0x40 ASCII '@'
	mv          a1, t0
	call        StorageIs

	// *** Basic block 190

	beqz        a0, .PrintInstruction_label_1114

	// *** Basic block 191

	lla         a1, .str.98
	mv          a0, s2
	call        fprintf

	// *** Basic block 192

.PrintInstruction_label_1114:
	j           .PrintInstruction_label_1158

	// *** Basic block 193

.PrintInstruction_label_1116:
	li          t0, 3		// 0x3 ASCII \x3
	bne         s11, t0, .PrintInstruction_label_1136

	// *** Basic block 194

	mv          s11, s10
	lla         a1, .str.99
	lw          a3, 112(s11)
	mv          a2, s5
	mv          a0, s2
	call        fprintf

	// *** Basic block 195

	j           .PrintInstruction_label_1157

	// *** Basic block 196

.PrintInstruction_label_1136:
	lla         s3, .str.100
	ld          a0, 32(s10)
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	mv          a1, s7
	call        PCodeRegisterName

	// *** Basic block 197

	mv          a3, a0
	mv          a2, s5
	mv          a1, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 198

.PrintInstruction_label_1157:

	// *** Basic block 199

.PrintInstruction_label_1158:

	// *** Basic block 200

.PrintInstruction_label_1159:
	lla         s5, .str.101

	// *** Basic block 201

.PrintInstruction_label_1162:

	// *** Basic block 202

.PrintInstruction_label_1163:
	addi        s9, s9, 1
	bge         s9, s8, .PrintInstruction_label_1053

	// *** Basic block 203

.PrintInstruction_label_1168:
	lla         a1, .str.102
	mv          a0, s2
	call        fprintf

	// *** Basic block 204

.PrintInstruction_label_1175:
	j           .PrintInstruction_label_164
.func_end_PrintInstruction:
	.size PrintInstruction, .func_end_PrintInstruction-PrintInstruction

	.global PCodeEmitterInit
	.type PCodeEmitterInit, @function

PCodeEmitterInit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sd          a1, 0(a0)
	addi        t0, a1, 192
	sd          t0, 8(a0)
	ret         
.func_end_PCodeEmitterInit:
	.size PCodeEmitterInit, .func_end_PCodeEmitterInit-PCodeEmitterInit

	.global NewPCodeEmitter
	.type NewPCodeEmitter, @function

NewPCodeEmitter:

	// *** Basic block 0

	.global malloc
	.global PCodeEmitterInit
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
	li          a0, 16		// 0x10 ASCII \x10
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        PCodeEmitterInit

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.NewPCodeEmitter_label_21:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewPCodeEmitter:
	.size NewPCodeEmitter, .func_end_NewPCodeEmitter-NewPCodeEmitter

	.global PCodeEmitterDestruct
	.type PCodeEmitterDestruct, @function

PCodeEmitterDestruct:

	// *** Basic block 0

	ret         
.func_end_PCodeEmitterDestruct:
	.size PCodeEmitterDestruct, .func_end_PCodeEmitterDestruct-PCodeEmitterDestruct

	.global PCodeEmitterDelete
	.type PCodeEmitterDelete, @function

PCodeEmitterDelete:

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
	.global PCodeEmitterDestruct
	.global free
	mv          s1, a0
	call        PCodeEmitterDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_PCodeEmitterDelete:
	.size PCodeEmitterDelete, .func_end_PCodeEmitterDelete-PCodeEmitterDelete

	.global PCodePrintFunction
	.type PCodePrintFunction, @function

PCodePrintFunction:

	// *** Basic block 0

	.global fprintf
	.global TargetFirstInstruction
	.local PrintInstruction
	.global TargetNext
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
	ld          t0, 0(s1)
	ld          s3, 16(t0)
	lb          t0, 40(t0)
	beqz        t0, .PCodePrintFunction_label_39

	// *** Basic block 1

	lla         a1, .str.103
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 2

	j           .PCodePrintFunction_label_48

	// *** Basic block 3

.PCodePrintFunction_label_39:
	lla         a1, .str.104
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 4

.PCodePrintFunction_label_48:
	lla         a1, .str.105
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 5

	lla         a1, .str.106
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 6

	ld          a0, 0(s1)
	call        TargetFirstInstruction

	// *** Basic block 7

	mv          s4, a0
	beq         s4, x0, .PCodePrintFunction_label_91

	// *** Basic block 8

.PCodePrintFunction_label_73:
	mv          a3, s2
	mv          a2, s3
	mv          a1, s4
	mv          a0, s1
	call        PrintInstruction

	// *** Basic block 9

	mv          a0, s4
	call        TargetNext

	// *** Basic block 10

	mv          s4, a0
	bne         s4, x0, .PCodePrintFunction_label_73

	// *** Basic block 11

.PCodePrintFunction_label_91:
	lla         a1, .str.107
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 12

	lla         a1, .str.108
	mv          a4, s3
	mv          a3, s3
	mv          a2, s3
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           fprintf
.func_end_PCodePrintFunction:
	.size PCodePrintFunction, .func_end_PCodePrintFunction-PCodePrintFunction

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.1, @object
	.size .str.1, 30

.str.2:
	.asciz "(null)"
	.type .str.2, @object
	.size .str.2, 1

.str.3:
	.asciz "inst->operand[0] != NULL"
	.type .str.3, @object
	.size .str.3, 25

.str.4:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.4, @object
	.size .str.4, 30

.str.5:
	.asciz "(null)"
	.type .str.5, @object
	.size .str.5, 1

.str.6:
	.asciz "inst->operand[1] != NULL"
	.type .str.6, @object
	.size .str.6, 25

.str.7:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.7, @object
	.size .str.7, 30

.str.8:
	.asciz "(null)"
	.type .str.8, @object
	.size .str.8, 1

.str.9:
	.asciz "inst->operand[0]->reg != NULL"
	.type .str.9, @object
	.size .str.9, 30

.str.10:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.10, @object
	.size .str.10, 30

.str.11:
	.asciz "(null)"
	.type .str.11, @object
	.size .str.11, 1

.str.12:
	.asciz "inst->operand[1]->reg != NULL"
	.type .str.12, @object
	.size .str.12, 30

.str.13:
	.asciz "(null)"
	.type .str.13, @object
	.size .str.13, 1

.str.14:
	.asciz "mov"
	.type .str.14, @object
	.size .str.14, 4

.str.15:
	.asciz "movf"
	.type .str.15, @object
	.size .str.15, 5

.str.16:
	.asciz "movd"
	.type .str.16, @object
	.size .str.16, 5

.str.17:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.17, @object
	.size .str.17, 30

.str.18:
	.asciz "(null)"
	.type .str.18, @object
	.size .str.18, 1

.str.19:
	.asciz "false"
	.type .str.19, @object
	.size .str.19, 6

.str.20:
	.asciz "\t%-8s%s, %s\n"
	.type .str.20, @object
	.size .str.20, 13

.str.21:
	.asciz "\tpushx   r%d\n"
	.type .str.21, @object
	.size .str.21, 14

.str.22:
	.asciz "\tpushf    f%d\n"
	.type .str.22, @object
	.size .str.22, 15

.str.23:
	.asciz "\tpushd    d%d\n"
	.type .str.23, @object
	.size .str.23, 15

.str.24:
	.asciz "\tpopd    d%d\n"
	.type .str.24, @object
	.size .str.24, 14

.str.25:
	.asciz "\tpopf    f%d\n"
	.type .str.25, @object
	.size .str.25, 14

.str.26:
	.asciz "\tpopx    r%d\n"
	.type .str.26, @object
	.size .str.26, 14

.str.27:
	.asciz ".%s_label_%d:\n"
	.type .str.27, @object
	.size .str.27, 15

.str.28:
	.asciz "%s:\n"
	.type .str.28, @object
	.size .str.28, 5

.str.29:
	.asciz "\t.local %s\n"
	.type .str.29, @object
	.size .str.29, 12

.str.30:
	.asciz "\t.global %s\n"
	.type .str.30, @object
	.size .str.30, 13

.str.31:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.31, @object
	.size .str.31, 30

.str.32:
	.asciz "(null)"
	.type .str.32, @object
	.size .str.32, 1

.str.33:
	.asciz "inst->operand[0]->opcode == P_OP(symbol)"
	.type .str.33, @object
	.size .str.33, 41

.str.34:
	.asciz "\t%-8s %s\n"
	.type .str.34, @object
	.size .str.34, 10

.str.35:
	.asciz "call"
	.type .str.35, @object
	.size .str.35, 5

.str.36:
	.asciz "\t%-8s %s\n"
	.type .str.36, @object
	.size .str.36, 10

.str.37:
	.asciz "rcall"
	.type .str.37, @object
	.size .str.37, 6

.str.38:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.38, @object
	.size .str.38, 30

.str.39:
	.asciz "(null)"
	.type .str.39, @object
	.size .str.39, 1

.str.40:
	.asciz "lit != NULL"
	.type .str.40, @object
	.size .str.40, 12

.str.41:
	.asciz "\t%s\n"
	.type .str.41, @object
	.size .str.41, 5

.str.42:
	.asciz "\t.loc %d %d %d\n"
	.type .str.42, @object
	.size .str.42, 16

.str.43:
	.asciz "\t%-8s"
	.type .str.43, @object
	.size .str.43, 6

.str.44:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.44, @object
	.size .str.44, 30

.str.45:
	.asciz "(null)"
	.type .str.45, @object
	.size .str.45, 1

.str.46:
	.asciz "inst->operand[0] != NULL"
	.type .str.46, @object
	.size .str.46, 25

.str.47:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.47, @object
	.size .str.47, 30

.str.48:
	.asciz "(null)"
	.type .str.48, @object
	.size .str.48, 1

.str.49:
	.asciz "inst->operand[1] != NULL"
	.type .str.49, @object
	.size .str.49, 25

.str.50:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.50, @object
	.size .str.50, 30

.str.51:
	.asciz "(null)"
	.type .str.51, @object
	.size .str.51, 1

.str.52:
	.asciz "inst->reg != NULL"
	.type .str.52, @object
	.size .str.52, 18

.str.53:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.53, @object
	.size .str.53, 30

.str.54:
	.asciz "(null)"
	.type .str.54, @object
	.size .str.54, 1

.str.55:
	.asciz "inst->operand[0]->reg != NULL"
	.type .str.55, @object
	.size .str.55, 30

.str.56:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.56, @object
	.size .str.56, 30

.str.57:
	.asciz "(null)"
	.type .str.57, @object
	.size .str.57, 1

.str.58:
	.asciz "TargetIsConst(inst->operand[1])"
	.type .str.58, @object
	.size .str.58, 32

.str.59:
	.asciz "%s, [%s, #%d]\n"
	.type .str.59, @object
	.size .str.59, 15

.str.60:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.60, @object
	.size .str.60, 30

.str.61:
	.asciz "(null)"
	.type .str.61, @object
	.size .str.61, 1

.str.62:
	.asciz "inst->operand[0] != NULL"
	.type .str.62, @object
	.size .str.62, 25

.str.63:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.63, @object
	.size .str.63, 30

.str.64:
	.asciz "(null)"
	.type .str.64, @object
	.size .str.64, 1

.str.65:
	.asciz "inst->operand[1] != NULL"
	.type .str.65, @object
	.size .str.65, 25

.str.66:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.66, @object
	.size .str.66, 30

.str.67:
	.asciz "(null)"
	.type .str.67, @object
	.size .str.67, 1

.str.68:
	.asciz "inst->operand[2] != NULL"
	.type .str.68, @object
	.size .str.68, 25

.str.69:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.69, @object
	.size .str.69, 30

.str.70:
	.asciz "(null)"
	.type .str.70, @object
	.size .str.70, 1

.str.71:
	.asciz "inst->operand[0]->reg != NULL"
	.type .str.71, @object
	.size .str.71, 30

.str.72:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.72, @object
	.size .str.72, 30

.str.73:
	.asciz "(null)"
	.type .str.73, @object
	.size .str.73, 1

.str.74:
	.asciz "inst->operand[1]->reg != NULL"
	.type .str.74, @object
	.size .str.74, 30

.str.75:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.75, @object
	.size .str.75, 30

.str.76:
	.asciz "(null)"
	.type .str.76, @object
	.size .str.76, 1

.str.77:
	.asciz "TargetIsConst(inst->operand[2])"
	.type .str.77, @object
	.size .str.77, 32

.str.78:
	.asciz "%s, [%s, #%d]\n"
	.type .str.78, @object
	.size .str.78, 15

.str.79:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.79, @object
	.size .str.79, 30

.str.80:
	.asciz "(null)"
	.type .str.80, @object
	.size .str.80, 1

.str.81:
	.asciz "inst->operand[0] != NULL"
	.type .str.81, @object
	.size .str.81, 25

.str.82:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.82, @object
	.size .str.82, 30

.str.83:
	.asciz "(null)"
	.type .str.83, @object
	.size .str.83, 1

.str.84:
	.asciz "inst->operand[1] != NULL"
	.type .str.84, @object
	.size .str.84, 25

.str.85:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.85, @object
	.size .str.85, 30

.str.86:
	.asciz "(null)"
	.type .str.86, @object
	.size .str.86, 1

.str.87:
	.asciz "inst->operand[0]->reg != NULL"
	.type .str.87, @object
	.size .str.87, 30

.str.88:
	.asciz "%s, .%s_label_%d\n"
	.type .str.88, @object
	.size .str.88, 18

.str.89:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.89, @object
	.size .str.89, 30

.str.90:
	.asciz "(null)"
	.type .str.90, @object
	.size .str.90, 1

.str.91:
	.asciz "inst->operand[0] != NULL"
	.type .str.91, @object
	.size .str.91, 25

.str.92:
	.asciz ".%s_label_%d\n"
	.type .str.92, @object
	.size .str.92, 14

.str.93:
	.asciz "(null)"
	.type .str.93, @object
	.size .str.93, 1

.str.94:
	.asciz "%s"
	.type .str.94, @object
	.size .str.94, 3

.str.95:
	.asciz ", "
	.type .str.95, @object
	.size .str.95, 3

.str.96:
	.asciz "%s#%d"
	.type .str.96, @object
	.size .str.96, 6

.str.97:
	.asciz "%s%s"
	.type .str.97, @object
	.size .str.97, 5

.str.98:
	.asciz "@tls"
	.type .str.98, @object
	.size .str.98, 5

.str.99:
	.asciz "%s.str.%d"
	.type .str.99, @object
	.size .str.99, 10

.str.100:
	.asciz "%s%s"
	.type .str.100, @object
	.size .str.100, 5

.str.101:
	.asciz ", "
	.type .str.101, @object
	.size .str.101, 3

.str.102:
	.asciz "\n"
	.type .str.102, @object
	.size .str.102, 2

.str.103:
	.asciz "\t.global %s\n"
	.type .str.103, @object
	.size .str.103, 13

.str.104:
	.asciz "\t.local  %s\n"
	.type .str.104, @object
	.size .str.104, 13

.str.105:
	.asciz "\t.type %s, @function\n\n"
	.type .str.105, @object
	.size .str.105, 23

.str.106:
	.asciz "%s:\n"
	.type .str.106, @object
	.size .str.106, 5

.str.107:
	.asciz ".func_end_%s:\n"
	.type .str.107, @object
	.size .str.107, 15

.str.108:
	.asciz "\t.size %s, .func_end_%s-%s\n\n"
	.type .str.108, @object
	.size .str.108, 29

