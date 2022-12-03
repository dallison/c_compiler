	.file   "6502/6502_target.c"
	.text
	.option pic
.PCbegin:
	.local  GenerateCode
	.type GenerateCode, @function

GenerateCode:

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
	.global New6502Generator
	.global W65C02Lower
	mv          s1, a0
	call        New6502Generator

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        W65C02Lower

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.GenerateCode_label_20:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GenerateCode:
	.size GenerateCode, .func_end_GenerateCode-GenerateCode

	.local  EmitFunctionAssembly
	.type EmitFunctionAssembly, @function

EmitFunctionAssembly:

	// *** Basic block 0

	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	.global W65C02EmitterInit
	.global W65C02PrintFunction
	.global W65C02EmitterDestruct
	mv          t0, a0
	mv          s1, a1
	addi        a0, s0, -32
	mv          a1, t0
	call        W65C02EmitterInit

	// *** Basic block 1

	addi        a0, s0, -32
	mv          a1, s1
	call        W65C02PrintFunction

	// *** Basic block 2

	addi        a0, s0, -32
	call        W65C02EmitterDestruct

	// *** Basic block 3

	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_EmitFunctionAssembly:
	.size EmitFunctionAssembly, .func_end_EmitFunctionAssembly-EmitFunctionAssembly

	.local  FilePrinter
	.type FilePrinter, @function

FilePrinter:

	// *** Basic block 0

	.global fprintf
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
	mv          s3, a2
	bnez        s1, .FilePrinter_label_22

	// *** Basic block 1

.FilePrinter_label_19:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.FilePrinter_label_22:
	lla         a1, .str.1
	ld          a3, 16(s2)
	mv          a2, s1
	mv          a0, s3
	call        fprintf

	// *** Basic block 3

	j           .FilePrinter_label_19
.func_end_FilePrinter:
	.size FilePrinter, .func_end_FilePrinter-FilePrinter

	.local  CreateAssemblyFile
	.type CreateAssemblyFile, @function

CreateAssemblyFile:

	// *** Basic block 0

	.global StringEqual
	.global stdout
	.global fopen
	.global fprintf
	.global compiler
	.global SourceTraverseFiles
	.local FilePrinter
	.local registers
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
	mv          s1, a1
	mv          s2, a0
	lla         a1, .str.2
	mv          a0, s1
	call        StringEqual

	// *** Basic block 1

	beqz        a0, .CreateAssemblyFile_label_64

	// *** Basic block 2

	la          t0, stdout
	ld          s3, 0(t0)
	j           .CreateAssemblyFile_label_73

	// *** Basic block 3

.CreateAssemblyFile_label_64:
	ld          a0, 16(s1)
	lla         a1, .str.3
	call        fopen

	// *** Basic block 4

	mv          s3, a0

	// *** Basic block 5

.CreateAssemblyFile_label_73:
	bne         s3, x0, .CreateAssemblyFile_label_82

	// *** Basic block 6

	mv          a0, x0

	// *** Basic block 7

.CreateAssemblyFile_label_79:
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

	// *** Basic block 8

.CreateAssemblyFile_label_82:
	lla         a1, .str.4
	ld          a2, 16(s2)
	mv          a0, s3
	call        fprintf

	// *** Basic block 9

	la          t0, compiler
	ld          s4, 0(t0)
	lb          t0, 1228(s4)
	beqz        t0, .CreateAssemblyFile_label_104

	// *** Basic block 10

	la          t0, FilePrinter
	mv          a1, t0
	mv          a0, s3
	call        SourceTraverseFiles

	// *** Basic block 11

.CreateAssemblyFile_label_104:
	lla         a1, .str.5
	mv          a0, s3
	call        fprintf

	// *** Basic block 12

	lb          t0, 1230(s4)
	beqz        t0, .CreateAssemblyFile_label_120

	// *** Basic block 13

	lla         a1, .str.6
	mv          a0, s3
	call        fprintf

	// *** Basic block 14

.CreateAssemblyFile_label_120:
	mv          s2, x0
	mv          s4, x0

	// *** Basic block 15

.CreateAssemblyFile_label_126:
	mv          s5, x0
	slli        t0, s4, 4
	lla         t1, registers
	add         s6, t1, t0
	lw          s7, 4(s6)
	bge         x0, s7, .CreateAssemblyFile_label_159

	// *** Basic block 16

	lb          s8, 0(s6)
	lw          t0, 8(s6)

	// *** Basic block 17

.CreateAssemblyFile_label_139:
	lla         a1, .str.7
	mv          a4, s2
	mv          a3, s5
	mv          a2, s8
	mv          a0, s3
	call        fprintf

	// *** Basic block 19

.CreateAssemblyFile_label_155:
	addi        s5, s5, 1
	bge         s5, s7, .CreateAssemblyFile_label_139

	// *** Basic block 20

.CreateAssemblyFile_label_159:

	// *** Basic block 21

.CreateAssemblyFile_label_160:
	addi        s4, s4, 1
	li          t0, 6		// 0x6 ASCII \x6
	bge         s4, t0, .CreateAssemblyFile_label_126

	// *** Basic block 22

.CreateAssemblyFile_label_165:
	lla         a1, .str.8
	li          t0, 30		// 0x1e ASCII \x1e
	mv          a2, t0
	mv          a0, s3
	call        fprintf

	// *** Basic block 23

	lla         a1, .str.9
	li          t0, 32		// 0x20 ASCII ' '
	mv          a2, t0
	mv          a0, s3
	call        fprintf

	// *** Basic block 24

	lla         a1, .str.10
	li          t0, 36		// 0x24 ASCII '$'
	mv          a2, t0
	mv          a0, s3
	call        fprintf

	// *** Basic block 25

	lla         a1, .str.11
	li          t0, 24		// 0x18 ASCII \x18
	mv          a2, t0
	mv          a0, s3
	call        fprintf

	// *** Basic block 26

	lla         a1, .str.12
	li          t0, 28		// 0x1c ASCII \x1c
	mv          a2, t0
	mv          a0, s3
	call        fprintf

	// *** Basic block 27

	lla         a1, .str.13
	li          t0, 26		// 0x1a ASCII \x1a
	mv          a2, t0
	mv          a0, s3
	call        fprintf

	// *** Basic block 28

	lla         a1, .str.14
	mv          a0, s3
	call        fprintf

	// *** Basic block 29

	lla         a1, .str.15
	mv          a0, s3
	call        fprintf

	// *** Basic block 30

	lla         a1, .str.16
	mv          a0, s3
	call        fprintf

	// *** Basic block 31

	lla         a1, .str.17
	mv          a0, s3
	call        fprintf

	// *** Basic block 32

	lla         a1, .str.18
	mv          a0, s3
	call        fprintf

	// *** Basic block 33

	lla         a1, .str.19
	mv          a0, s3
	call        fprintf

	// *** Basic block 34

	lla         a1, .str.20
	mv          a0, s3
	call        fprintf

	// *** Basic block 35

	lla         a1, .str.21
	mv          a0, s3
	call        fprintf

	// *** Basic block 36

	lla         a1, .str.22
	mv          a0, s3
	call        fprintf

	// *** Basic block 37

	lla         a1, .str.23
	mv          a0, s3
	call        fprintf

	// *** Basic block 38

	lla         a1, .str.24
	mv          a0, s3
	call        fprintf

	// *** Basic block 39

	lla         a1, .str.25
	mv          a0, s3
	call        fprintf

	// *** Basic block 40

	lla         a1, .str.26
	mv          a0, s3
	call        fprintf

	// *** Basic block 41

	lla         a1, .str.27
	mv          a0, s3
	call        fprintf

	// *** Basic block 42

	lla         a1, .str.28
	mv          a0, s3
	call        fprintf

	// *** Basic block 43

	lla         a1, .str.29
	mv          a0, s3
	call        fprintf

	// *** Basic block 44

	lla         a1, .str.30
	mv          a0, s3
	call        fprintf

	// *** Basic block 45

	lla         a1, .str.31
	mv          a0, s3
	call        fprintf

	// *** Basic block 46

	mv          a0, s3
	j           .CreateAssemblyFile_label_79
.func_end_CreateAssemblyFile:
	.size CreateAssemblyFile, .func_end_CreateAssemblyFile-CreateAssemblyFile

	.local  Assemble
	.type Assemble, @function

Assemble:

	// *** Basic block 0

	.global W65C02AssemblerInit
	.global AssemblerRun
	.global Assemble6502Instruction
	.global W65C02AssemblerDestruct
	addi sp, sp, -1024
	// Saved return address (offset 1016) and frame pointer (offset 1008)
	sd ra, 1016(sp)
	sd s0, 1008(sp)
	addi s0, sp, 1024
	// Local vars at offset -1008(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          t0, a0
	mv          t1, a1
	addi        a0, s0, -1008
	mv          a2, t1
	mv          a1, t0
	call        W65C02AssemblerInit

	// *** Basic block 1

	addi        a0, s0, -1008
	la          t0, Assemble6502Instruction
	mv          a1, t0
	call        AssemblerRun

	// *** Basic block 2

	addi        t0, s0, -1008
	lw          s1, 740(t0)
	addi        a0, s0, -1008
	call        W65C02AssemblerDestruct

	// *** Basic block 3

	seqz        a0, s1

	// *** Basic block 4

.Assemble_label_40:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Assemble:
	.size Assemble, .func_end_Assemble-Assemble

	.local  DataStart
	.type DataStart, @function

DataStart:

	// *** Basic block 0

	.global fprintf
	// Leaf procedure, no stack frame generated
	lla         a1, .str.32
	j           fprintf
.func_end_DataStart:
	.size DataStart, .func_end_DataStart-DataStart

	.local  StaticVariable
	.type StaticVariable, @function

StaticVariable:

	// *** Basic block 0

	.global fprintf
	.global printf
	.global abort
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
	sd          s1, -24(s0)	// Spilled @45
	mv          s2, a0
	lla         a1, .str.33
	ld          s3, 16(s2)
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 1

	lla         a1, .str.34
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 2

	lb          t0, 40(s2)
	beqz        t0, .StaticVariable_label_83

	// *** Basic block 3

	lla         a1, .str.35
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 4

	j           .StaticVariable_label_92

	// *** Basic block 5

.StaticVariable_label_83:
	lla         a1, .str.36
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 6

.StaticVariable_label_92:
	lla         a1, .str.37
	ld          s4, 48(s2)
	mv          a3, s4
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 7

	lw          t0, 56(s2)
	addi        s3, t0, -1
	blt         s3, x0, .StaticVariable_label_115

	// *** Basic block 8

	j           .StaticVariable_label_130

	// *** Basic block 9

.StaticVariable_label_115:
	lla         a0, .str.38
	lla         a1, .str.39
	lla         a3, .str.40
	li          t0, 132		// 0x84 ASCII \x84
	mv          a2, t0
	call        printf

	// *** Basic block 10

	call        abort

	// *** Basic block 11

.StaticVariable_label_130:
	mv          s5, x0
	mv          s6, x0

	// *** Basic block 12

.StaticVariable_label_136:
	li          t0, 1		// 0x1 ASCII \x1
	sll         t0, t0, s6
	and         t0, s3, t0
	beqz        t0, .StaticVariable_label_144

	// *** Basic block 13

	addi        s5, s5, 1
	j           .StaticVariable_label_146

	// *** Basic block 14

.StaticVariable_label_144:
	j           .StaticVariable_label_152

	// *** Basic block 15

.StaticVariable_label_146:

	// *** Basic block 16

.StaticVariable_label_147:
	addi        s6, s6, 1
	li          t0, 16		// 0x10 ASCII \x10
	bge         s6, t0, .StaticVariable_label_136

	// *** Basic block 17

.StaticVariable_label_152:
	lla         a1, .str.41
	mv          a2, s5
	mv          a0, s1
	call        fprintf

	// *** Basic block 18

	mv          s3, x0
	mv          s7, x0
	addi        t0, s2, 64
	ld          s8, 8(t0)
	bge         x0, s8, .StaticVariable_label_378

	// *** Basic block 19

	ld          t0, 64(s2)

	// *** Basic block 20

.StaticVariable_label_171:
	slli        t1, s7, 3
	add         t0, t0, t1
	ld          s2, 0(t0)
	lw          s9, 4(s2)
	bge         s3, s9, .StaticVariable_label_192

	// *** Basic block 21

	sub         s10, s9, s3
	lla         a1, .str.42
	mv          a2, s10
	mv          a0, s1
	call        fprintf

	// *** Basic block 22

	add         s3, s3, s10

	// *** Basic block 23

.StaticVariable_label_192:
	lw          t0, 0(s2)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 24

	j           .StaticVariable_label_206

	// *** Basic block 25

	j           .StaticVariable_label_218

	// *** Basic block 26

	j           .StaticVariable_label_230

	// *** Basic block 27

	j           .StaticVariable_label_242

	// *** Basic block 28

	j           .StaticVariable_label_254

	// *** Basic block 29

	j           .StaticVariable_label_294

	// *** Basic block 30

	j           .StaticVariable_label_306

	// *** Basic block 31

.StaticVariable_label_206:
	lla         a1, .str.43
	lbu         a2, 8(s2)
	mv          a0, s1
	call        fprintf

	// *** Basic block 32

	addi        s3, s3, 1
	j           .StaticVariable_label_373

	// *** Basic block 33

.StaticVariable_label_218:
	lla         a1, .str.44
	lhu         a2, 8(s2)
	mv          a0, s1
	call        fprintf

	// *** Basic block 34

	addi        s3, s3, 2
	j           .StaticVariable_label_373

	// *** Basic block 35

.StaticVariable_label_230:
	lla         a1, .str.45
	lwu         a2, 8(s2)
	mv          a0, s1
	call        fprintf

	// *** Basic block 36

	addi        s3, s3, 4
	j           .StaticVariable_label_373

	// *** Basic block 37

.StaticVariable_label_242:
	lla         a1, .str.46
	lbu         a2, 8(s2)
	mv          a0, s1
	call        fprintf

	// *** Basic block 38

	addi        s3, s3, 8
	j           .StaticVariable_label_373

	// *** Basic block 39

.StaticVariable_label_254:
	ld          s9, 8(s2)
	lb          t0, 56(s9)
	slli        t0, t0, 60
	srai        t0, t0, 63
	beqz        t0, .StaticVariable_label_272

	// *** Basic block 40

	lla         a1, .str.47
	ld          a2, 16(s9)
	mv          a0, s1
	call        fprintf

	// *** Basic block 41

	j           .StaticVariable_label_282

	// *** Basic block 42

.StaticVariable_label_272:
	lla         a1, .str.48
	ld          a2, 16(s9)
	mv          a0, s1
	call        fprintf

	// *** Basic block 43

.StaticVariable_label_282:
	lla         a1, .str.49
	ld          a2, 16(s9)
	mv          a0, s1
	call        fprintf

	// *** Basic block 44

	addi        s3, s3, 8
	j           .StaticVariable_label_373

	// *** Basic block 45

.StaticVariable_label_294:
	lla         a1, .str.50
	lw          a2, 8(s2)
	mv          a0, s1
	call        fprintf

	// *** Basic block 46

	addi        s3, s3, 8
	j           .StaticVariable_label_373

	// *** Basic block 47

.StaticVariable_label_306:
	mv          s9, x0
	lla         s10, .str.51
	mv          s11, x0
	addi        t0, s2, 8
	ld          s1, 8(t0)
	bge         x0, s1, .StaticVariable_label_365

	// *** Basic block 48

	ld          s2, 8(s2)

	// *** Basic block 49

.StaticVariable_label_322:
	bnez        s9, .StaticVariable_label_331

	// *** Basic block 50

	lla         a1, .str.52
	ld          a0, -24(s0)	// Spilled @45
	call        fprintf

	// *** Basic block 51

.StaticVariable_label_331:
	lla         a1, .str.53
	add         t0, s2, s11
	lb          a3, 0(t0)
	mv          a2, s10
	ld          a0, -24(s0)	// Spilled @45
	call        fprintf

	// *** Basic block 52

	addi        s9, s9, 1
	li          t0, 16		// 0x10 ASCII \x10
	bne         s9, t0, .StaticVariable_label_360

	// *** Basic block 53

	mv          s9, x0
	lla         a1, .str.55
	ld          a0, -24(s0)	// Spilled @45
	call        fprintf

	// *** Basic block 55

.StaticVariable_label_360:

	// *** Basic block 56

.StaticVariable_label_361:
	addi        s11, s11, 1
	bge         s11, s1, .StaticVariable_label_322

	// *** Basic block 57

.StaticVariable_label_365:
	lla         a1, .str.57
	ld          a0, -24(s0)	// Spilled @45
	call        fprintf

	// *** Basic block 58

	j           .StaticVariable_label_373

	// *** Basic block 59

.StaticVariable_label_373:

	// *** Basic block 60

.StaticVariable_label_374:
	addi        s7, s7, 1
	bge         s7, s8, .StaticVariable_label_171

	// *** Basic block 61

.StaticVariable_label_378:
	sub         s1, s4, s3
	bge         x0, s1, .StaticVariable_label_391

	// *** Basic block 62

	lla         a1, .str.58
	mv          a2, s1
	ld          a0, -24(s0)	// Spilled @45
	call        fprintf

	// *** Basic block 63

.StaticVariable_label_391:
	lla         a1, .str.59
	ld          a0, -24(s0)	// Spilled @45
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
	j           fprintf
.func_end_StaticVariable:
	.size StaticVariable, .func_end_StaticVariable-StaticVariable

	.local  BSSVariable
	.type BSSVariable, @function

BSSVariable:

	// *** Basic block 0

	.global fprintf
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
	lla         a1, .str.60
	ld          s3, 16(s2)
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 1

	lb          t0, 40(s2)
	beqz        t0, .BSSVariable_label_43

	// *** Basic block 2

	lla         a1, .str.61
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 3

	j           .BSSVariable_label_52

	// *** Basic block 4

.BSSVariable_label_43:
	lla         a1, .str.62
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 5

.BSSVariable_label_52:
	lla         a1, .str.63
	ld          a3, 48(s2)
	ld          a4, 56(s2)
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 6

	lla         a1, .str.64
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           fprintf
.func_end_BSSVariable:
	.size BSSVariable, .func_end_BSSVariable-BSSVariable

	.local  StringLiteralSection
	.type StringLiteralSection, @function

StringLiteralSection:

	// *** Basic block 0

	.global fprintf
	// Leaf procedure, no stack frame generated
	lla         a1, .str.65
	j           fprintf
.func_end_StringLiteralSection:
	.size StringLiteralSection, .func_end_StringLiteralSection-StringLiteralSection

	.local  EmitLiteral
	.type EmitLiteral, @function

EmitLiteral:

	// *** Basic block 0

	.global fprintf
	.global StringEscape
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
	mv          s2, a1
	lb          t0, 48(s1)
	beqz        t0, .EmitLiteral_label_30

	// *** Basic block 1

.EmitLiteral_label_27:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.EmitLiteral_label_30:
	lla         a1, .str.66
	lw          a2, 0(s1)
	mv          a0, s2
	call        fprintf

	// *** Basic block 3

	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	addi        a0, s1, 8
	addi        a1, s0, -64
	call        StringEscape

	// *** Basic block 4

	lla         a1, .str.67
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s2
	call        fprintf

	// *** Basic block 5

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 6

	lla         a1, .str.68
	lw          s3, 0(s1)
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 7

	lla         a1, .str.69
	ld          t0, 24(a0)
	addi        a3, t0, 1
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 8

	lla         a1, .str.70
	mv          a0, s2
	call        fprintf

	// *** Basic block 9

	j           .EmitLiteral_label_27
.func_end_EmitLiteral:
	.size EmitLiteral, .func_end_EmitLiteral-EmitLiteral

	.local  Cleanup
	.type Cleanup, @function

Cleanup:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global W65C02GeneratorDelete
	j           W65C02GeneratorDelete
.func_end_Cleanup:
	.size Cleanup, .func_end_Cleanup-Cleanup

	.local  EmitDebug
	.type EmitDebug, @function

EmitDebug:

	// *** Basic block 0

	.global fprintf
	// Leaf procedure, no stack frame generated
	lla         a1, .str.71
	j           fprintf
.func_end_EmitDebug:
	.size EmitDebug, .func_end_EmitDebug-EmitDebug

	.global New6502Target
	.type New6502Target, @function

New6502Target:

	// *** Basic block 0

	.global malloc
	.global StringInit
	.local GenerateCode
	.local EmitFunctionAssembly
	.local Assemble
	.local Cleanup
	.local CreateAssemblyFile
	.local StaticVariable
	.local BSSVariable
	.local DataStart
	.local StringLiteralSection
	.local EmitLiteral
	.local EmitDebug
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	li          a0, 168		// 0xa8 ASCII \xa8
	call        malloc

	// *** Basic block 1

	mv          s1, a0
	lla         a1, .str.72
	mv          a0, s1
	call        StringInit

	// *** Basic block 2

	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, 40(s1)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 44(s1)
	la          t0, GenerateCode
	sd          t0, 48(s1)
	la          t0, EmitFunctionAssembly
	sd          t0, 64(s1)
	la          t0, Assemble
	sd          t0, 72(s1)
	la          t0, Cleanup
	sd          t0, 160(s1)
	la          t0, CreateAssemblyFile
	sd          t0, 56(s1)
	la          t0, StaticVariable
	sd          t0, 88(s1)
	la          t0, BSSVariable
	sd          t0, 96(s1)
	la          t0, DataStart
	sd          t0, 80(s1)
	la          t0, StringLiteralSection
	sd          t0, 136(s1)
	la          t0, EmitLiteral
	sd          t0, 144(s1)
	la          t0, EmitDebug
	sd          t0, 152(s1)
	mv          a0, s1

	// *** Basic block 3

.New6502Target_label_87:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_New6502Target:
	.size New6502Target, .func_end_New6502Target-New6502Target

.PCend:
	.data
registers:
	.type   registers,@object
	.local  registers
	.size   registers,96
	.p2align  3
	.byte   98
	.space  3
	.word   4
	.word   1
	.space  4
	.byte   97
	.space  3
	.word   4
	.word   2
	.space  4
	.byte   105
	.space  3
	.word   4
	.word   4
	.space  4
	.byte   120
	.space  3
	.word   4
	.word   8
	.space  4
	.byte   102
	.space  3
	.word   4
	.word   4
	.space  4
	.byte   100
	.space  3
	.word   4
	.word   8
	.space  4

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "\t.file %d \"%s\"\n"
	.type .str.1, @object
	.size .str.1, 16

.str.2:
	.asciz "-"
	.type .str.2, @object
	.size .str.2, 2

.str.3:
	.asciz "w"
	.type .str.3, @object
	.size .str.3, 2

.str.4:
	.asciz "\t.file   \"%s\"\n"
	.type .str.4, @object
	.size .str.4, 15

.str.5:
	.asciz "\t.text\n"
	.type .str.5, @object
	.size .str.5, 8

.str.6:
	.asciz "\t.option pic\n"
	.type .str.6, @object
	.size .str.6, 14

.str.7:
	.asciz "\t.set %c%d %d\n"
	.type .str.7, @object
	.size .str.7, 15

.str.8:
	.asciz "\t.set t2 %d\n"
	.type .str.8, @object
	.size .str.8, 13

.str.9:
	.asciz "\t.set t4 %d\n"
	.type .str.9, @object
	.size .str.9, 13

.str.10:
	.asciz "\t.set t8 %d\n"
	.type .str.10, @object
	.size .str.10, 13

.str.11:
	.asciz "\t.set ap %d\n"
	.type .str.11, @object
	.size .str.11, 13

.str.12:
	.asciz "\t.set fp %d\n"
	.type .str.12, @object
	.size .str.12, 13

.str.13:
	.asciz "\t.set sp %d\n"
	.type .str.13, @object
	.size .str.13, 13

.str.14:
	.asciz "\t.global __move_reg_8\n"
	.type .str.14, @object
	.size .str.14, 23

.str.15:
	.asciz "\t.global __push_reg_1\n"
	.type .str.15, @object
	.size .str.15, 23

.str.16:
	.asciz "\t.global __push_reg_2\n"
	.type .str.16, @object
	.size .str.16, 23

.str.17:
	.asciz "\t.global __push_reg_4\n"
	.type .str.17, @object
	.size .str.17, 23

.str.18:
	.asciz "\t.global __push_reg_8\n"
	.type .str.18, @object
	.size .str.18, 23

.str.19:
	.asciz "\t.global __pull_reg_1\n"
	.type .str.19, @object
	.size .str.19, 23

.str.20:
	.asciz "\t.global __pull_reg_2\n"
	.type .str.20, @object
	.size .str.20, 23

.str.21:
	.asciz "\t.global __pull_reg_4\n"
	.type .str.21, @object
	.size .str.21, 23

.str.22:
	.asciz "\t.global __pull_reg_8\n"
	.type .str.22, @object
	.size .str.22, 23

.str.23:
	.asciz "\t.global __load_zero_2\n"
	.type .str.23, @object
	.size .str.23, 24

.str.24:
	.asciz "\t.global __load_zero_3\n"
	.type .str.24, @object
	.size .str.24, 24

.str.25:
	.asciz "\t.global __load_zero_4\n"
	.type .str.25, @object
	.size .str.25, 24

.str.26:
	.asciz "\t.global __load_zero_5\n"
	.type .str.26, @object
	.size .str.26, 24

.str.27:
	.asciz "\t.global __load_zero_6\n"
	.type .str.27, @object
	.size .str.27, 24

.str.28:
	.asciz "\t.global __load_zero_7\n"
	.type .str.28, @object
	.size .str.28, 24

.str.29:
	.asciz "\t.global __enter\n"
	.type .str.29, @object
	.size .str.29, 18

.str.30:
	.asciz "\t.global __rts\n"
	.type .str.30, @object
	.size .str.30, 16

.str.31:
	.asciz "\n\n"
	.type .str.31, @object
	.size .str.31, 3

.str.32:
	.asciz "\t.data\n"
	.type .str.32, @object
	.size .str.32, 8

.str.33:
	.asciz "%s:\n"
	.type .str.33, @object
	.size .str.33, 5

.str.34:
	.asciz "\t.type   %s,@object\n"
	.type .str.34, @object
	.size .str.34, 21

.str.35:
	.asciz "\t.global %s\n"
	.type .str.35, @object
	.size .str.35, 13

.str.36:
	.asciz "\t.local  %s\n"
	.type .str.36, @object
	.size .str.36, 13

.str.37:
	.asciz "\t.size   %s,%zd\n"
	.type .str.37, @object
	.size .str.37, 17

.str.38:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.38, @object
	.size .str.38, 30

.str.39:
	.asciz "(null)"
	.type .str.39, @object
	.size .str.39, 1

.str.40:
	.asciz "alignment >= 0"
	.type .str.40, @object
	.size .str.40, 15

.str.41:
	.asciz "\t.p2align  %d\n"
	.type .str.41, @object
	.size .str.41, 15

.str.42:
	.asciz "\t.space  %d\n"
	.type .str.42, @object
	.size .str.42, 13

.str.43:
	.asciz "\t.byte   %d\n"
	.type .str.43, @object
	.size .str.43, 13

.str.44:
	.asciz "\t.half   %d\n"
	.type .str.44, @object
	.size .str.44, 13

.str.45:
	.asciz "\t.word   %d\n"
	.type .str.45, @object
	.size .str.45, 13

.str.46:
	.asciz "\t.long   %" PRId64 "\n"
	.type .str.46, @object
	.size .str.46, 15

.str.47:
	.asciz "\t.local %s\n"
	.type .str.47, @object
	.size .str.47, 12

.str.48:
	.asciz "\t.global %s\n"
	.type .str.48, @object
	.size .str.48, 13

.str.49:
	.asciz "\t.hword    %s\n"
	.type .str.49, @object
	.size .str.49, 15

.str.50:
	.asciz "\t.hword    .str.%d\n"
	.type .str.50, @object
	.size .str.50, 20

.str.51:
	.asciz "(null)"
	.type .str.51, @object
	.size .str.51, 1

.str.52:
	.asciz "\t.byte "
	.type .str.52, @object
	.size .str.52, 8

.str.53:
	.asciz "%s0x%02x"
	.type .str.53, @object
	.size .str.53, 9

.str.54:
	.asciz ","
	.type .str.54, @object
	.size .str.54, 2

.str.55:
	.asciz "\n"
	.type .str.55, @object
	.size .str.55, 2

.str.56:
	.asciz "(null)"
	.type .str.56, @object
	.size .str.56, 1

.str.57:
	.asciz "\n"
	.type .str.57, @object
	.size .str.57, 2

.str.58:
	.asciz "\t.space  %zd\n"
	.type .str.58, @object
	.size .str.58, 14

.str.59:
	.asciz "\n"
	.type .str.59, @object
	.size .str.59, 2

.str.60:
	.asciz "\t.type   %s,@object\n"
	.type .str.60, @object
	.size .str.60, 21

.str.61:
	.asciz "\t.global %s\n"
	.type .str.61, @object
	.size .str.61, 13

.str.62:
	.asciz "\t.local  %s\n"
	.type .str.62, @object
	.size .str.62, 13

.str.63:
	.asciz "\t.comm   %s,%zd,%zd\n"
	.type .str.63, @object
	.size .str.63, 21

.str.64:
	.asciz "\n"
	.type .str.64, @object
	.size .str.64, 2

.str.65:
	.asciz "\t.section \".rodata\", \"aMS\", @progbits\n"
	.type .str.65, @object
	.size .str.65, 39

.str.66:
	.asciz ".str.%d:\n"
	.type .str.66, @object
	.size .str.66, 10

.str.67:
	.asciz "\t.asciz \"%s\"\n"
	.type .str.67, @object
	.size .str.67, 14

.str.68:
	.asciz "\t.type .str.%d, @object\n"
	.type .str.68, @object
	.size .str.68, 25

.str.69:
	.asciz "\t.size .str.%d, %zd\n"
	.type .str.69, @object
	.size .str.69, 21

.str.70:
	.asciz "\n"
	.type .str.70, @object
	.size .str.70, 2

.str.71:
	.asciz "\t.section \".debug_line\", \"aMS\", @progbits\n"
	.type .str.71, @object
	.size .str.71, 43

.str.72:
	.asciz "6502"
	.type .str.72, @object
	.size .str.72, 5

