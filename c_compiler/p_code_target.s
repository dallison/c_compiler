	.file   "p_code_target.c"
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
	.global NewPCodeGenerator
	.global PCodeLower
	.global PCodePrint
	mv          s1, a0
	call        NewPCodeGenerator

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        PCodeLower

	// *** Basic block 2

	mv          a0, s2
	call        PCodePrint

	// *** Basic block 3

	mv          a0, s2

	// *** Basic block 4

.GenerateCode_label_24:
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
	.global PCodeEmitterInit
	.global PCodePrintFunction
	.global PCodeEmitterDestruct
	mv          t0, a0
	mv          s1, a1
	addi        a0, s0, -32
	mv          a1, t0
	call        PCodeEmitterInit

	// *** Basic block 1

	addi        a0, s0, -32
	mv          a1, s1
	call        PCodePrintFunction

	// *** Basic block 2

	addi        a0, s0, -32
	call        PCodeEmitterDestruct

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
	lla         a1, .str.2
	mv          a0, s1
	call        StringEqual

	// *** Basic block 1

	beqz        a0, .CreateAssemblyFile_label_37

	// *** Basic block 2

	la          t0, stdout
	ld          s3, 0(t0)
	j           .CreateAssemblyFile_label_46

	// *** Basic block 3

.CreateAssemblyFile_label_37:
	ld          a0, 16(s1)
	lla         a1, .str.3
	call        fopen

	// *** Basic block 4

	mv          s3, a0

	// *** Basic block 5

.CreateAssemblyFile_label_46:
	bne         s3, x0, .CreateAssemblyFile_label_55

	// *** Basic block 6

	mv          a0, x0

	// *** Basic block 7

.CreateAssemblyFile_label_52:
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

.CreateAssemblyFile_label_55:
	lla         a1, .str.4
	mv          a0, s3
	call        fprintf

	// *** Basic block 9

	lla         a1, .str.5
	ld          a2, 16(s2)
	mv          a0, s3
	call        fprintf

	// *** Basic block 10

	la          t0, compiler
	ld          s4, 0(t0)
	lb          t0, 1228(s4)
	beqz        t0, .CreateAssemblyFile_label_83

	// *** Basic block 11

	la          t0, FilePrinter
	mv          a1, t0
	mv          a0, s3
	call        SourceTraverseFiles

	// *** Basic block 12

.CreateAssemblyFile_label_83:
	lb          t0, 1230(s4)
	beqz        t0, .CreateAssemblyFile_label_93

	// *** Basic block 13

	lla         a1, .str.6
	mv          a0, s3
	call        fprintf

	// *** Basic block 14

.CreateAssemblyFile_label_93:
	lla         a1, .str.7
	mv          a0, s3
	call        fprintf

	// *** Basic block 15

	mv          a0, s3
	j           .CreateAssemblyFile_label_52
.func_end_CreateAssemblyFile:
	.size CreateAssemblyFile, .func_end_CreateAssemblyFile-CreateAssemblyFile

	.local  Assemble
	.type Assemble, @function

Assemble:

	// *** Basic block 0

	.global PCodeAssemblerInit
	.global AssemblerRun
	.global AssemblePCodeInstruction
	.global PCodeAssemblerDestruct
	addi sp, sp, -928
	// Saved return address (offset 920) and frame pointer (offset 912)
	sd ra, 920(sp)
	sd s0, 912(sp)
	addi s0, sp, 928
	// Local vars at offset -928(s0)
	// End of stack frame
	mv          t0, a0
	mv          t1, a1
	addi        a0, s0, -928
	mv          a2, t1
	mv          a1, t0
	call        PCodeAssemblerInit

	// *** Basic block 1

	addi        a0, s0, -928
	la          t0, AssemblePCodeInstruction
	mv          a1, t0
	call        AssemblerRun

	// *** Basic block 2

	addi        a0, s0, -928
	call        PCodeAssemblerDestruct

	// *** Basic block 3

	addi        t0, s0, -928
	lw          t0, 740(t0)
	seqz        a0, t0

	// *** Basic block 4

.Assemble_label_39:
	// Restored registers.
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
	lla         a1, .str.8
	j           fprintf
.func_end_DataStart:
	.size DataStart, .func_end_DataStart-DataStart

	.local  TlsDataStart
	.type TlsDataStart, @function

TlsDataStart:

	// *** Basic block 0

	.global fprintf
	// Leaf procedure, no stack frame generated
	lla         a1, .str.9
	j           fprintf
.func_end_TlsDataStart:
	.size TlsDataStart, .func_end_TlsDataStart-TlsDataStart

	.local  TlsBSSStart
	.type TlsBSSStart, @function

TlsBSSStart:

	// *** Basic block 0

	.global fprintf
	// Leaf procedure, no stack frame generated
	lla         a1, .str.10
	j           fprintf
.func_end_TlsBSSStart:
	.size TlsBSSStart, .func_end_TlsBSSStart-TlsBSSStart

	.local  EmitP2Align
	.type EmitP2Align, @function

EmitP2Align:

	// *** Basic block 0

	.global fprintf
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	addi        t0, t0, -1
	mv          t2, x0
	mv          t3, x0

	// *** Basic block 1

.EmitP2Align_label_21:
	li          t4, 1		// 0x1 ASCII \x1
	sll         t4, t4, t3
	and         t4, t0, t4
	beqz        t4, .EmitP2Align_label_29

	// *** Basic block 2

	addi        t2, t2, 1
	j           .EmitP2Align_label_31

	// *** Basic block 3

.EmitP2Align_label_29:
	j           .EmitP2Align_label_37

	// *** Basic block 4

.EmitP2Align_label_31:

	// *** Basic block 5

.EmitP2Align_label_32:
	addi        t3, t3, 1
	li          t0, 16		// 0x10 ASCII \x10
	bge         t3, t0, .EmitP2Align_label_21

	// *** Basic block 6

.EmitP2Align_label_37:
	lla         a1, .str.11
	mv          a2, t2
	mv          a0, t1
	j           fprintf
.func_end_EmitP2Align:
	.size EmitP2Align, .func_end_EmitP2Align-EmitP2Align

	.local  StaticVariable
	.type StaticVariable, @function

StaticVariable:

	// *** Basic block 0

	.global fprintf
	.local EmitP2Align
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
	lla         a1, .str.12
	ld          s3, 16(s2)
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 1

	lla         a1, .str.13
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 2

	lb          t0, 40(s2)
	beqz        t0, .StaticVariable_label_79

	// *** Basic block 3

	lla         a1, .str.14
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 4

	j           .StaticVariable_label_88

	// *** Basic block 5

.StaticVariable_label_79:
	lla         a1, .str.15
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 6

.StaticVariable_label_88:
	lla         a1, .str.16
	ld          s4, 48(s2)
	mv          a3, s4
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 7

	lw          a0, 56(s2)
	mv          a1, s1
	call        EmitP2Align

	// *** Basic block 8

	mv          s3, x0
	mv          s5, x0
	addi        t0, s2, 64
	ld          s6, 8(t0)
	bge         x0, s6, .StaticVariable_label_324

	// *** Basic block 9

	ld          t0, 64(s2)

	// *** Basic block 10

.StaticVariable_label_118:
	slli        t1, s5, 3
	add         t0, t0, t1
	ld          s2, 0(t0)
	lw          s7, 4(s2)
	bge         s3, s7, .StaticVariable_label_139

	// *** Basic block 11

	sub         s8, s7, s3
	lla         a1, .str.17
	mv          a2, s8
	mv          a0, s1
	call        fprintf

	// *** Basic block 12

	add         s3, s3, s8

	// *** Basic block 13

.StaticVariable_label_139:
	lw          t0, 0(s2)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 14

	j           .StaticVariable_label_152

	// *** Basic block 15

	j           .StaticVariable_label_164

	// *** Basic block 16

	j           .StaticVariable_label_176

	// *** Basic block 17

	j           .StaticVariable_label_188

	// *** Basic block 18

	j           .StaticVariable_label_200

	// *** Basic block 19

	j           .StaticVariable_label_240

	// *** Basic block 20

	j           .StaticVariable_label_252

	// *** Basic block 21

.StaticVariable_label_152:
	lla         a1, .str.18
	lbu         a2, 8(s2)
	mv          a0, s1
	call        fprintf

	// *** Basic block 22

	addi        s3, s3, 1
	j           .StaticVariable_label_319

	// *** Basic block 23

.StaticVariable_label_164:
	lla         a1, .str.19
	lhu         a2, 8(s2)
	mv          a0, s1
	call        fprintf

	// *** Basic block 24

	addi        s3, s3, 2
	j           .StaticVariable_label_319

	// *** Basic block 25

.StaticVariable_label_176:
	lla         a1, .str.20
	lwu         a2, 8(s2)
	mv          a0, s1
	call        fprintf

	// *** Basic block 26

	addi        s3, s3, 4
	j           .StaticVariable_label_319

	// *** Basic block 27

.StaticVariable_label_188:
	lla         a1, .str.21
	lbu         a2, 8(s2)
	mv          a0, s1
	call        fprintf

	// *** Basic block 28

	addi        s3, s3, 8
	j           .StaticVariable_label_319

	// *** Basic block 29

.StaticVariable_label_200:
	ld          s7, 8(s2)
	lb          t0, 56(s7)
	slli        t0, t0, 60
	srai        t0, t0, 63
	beqz        t0, .StaticVariable_label_218

	// *** Basic block 30

	lla         a1, .str.22
	ld          a2, 16(s7)
	mv          a0, s1
	call        fprintf

	// *** Basic block 31

	j           .StaticVariable_label_228

	// *** Basic block 32

.StaticVariable_label_218:
	lla         a1, .str.23
	ld          a2, 16(s7)
	mv          a0, s1
	call        fprintf

	// *** Basic block 33

.StaticVariable_label_228:
	lla         a1, .str.24
	ld          a2, 16(s7)
	mv          a0, s1
	call        fprintf

	// *** Basic block 34

	addi        s3, s3, 8
	j           .StaticVariable_label_319

	// *** Basic block 35

.StaticVariable_label_240:
	lla         a1, .str.25
	lw          a2, 8(s2)
	mv          a0, s1
	call        fprintf

	// *** Basic block 36

	addi        s3, s3, 8
	j           .StaticVariable_label_319

	// *** Basic block 37

.StaticVariable_label_252:
	mv          s7, x0
	lla         s8, .str.26
	mv          s9, x0
	addi        t0, s2, 8
	ld          s10, 8(t0)
	bge         x0, s10, .StaticVariable_label_311

	// *** Basic block 38

	ld          s2, 8(s2)

	// *** Basic block 39

.StaticVariable_label_268:
	bnez        s7, .StaticVariable_label_277

	// *** Basic block 40

	lla         a1, .str.27
	mv          a0, s1
	call        fprintf

	// *** Basic block 41

.StaticVariable_label_277:
	lla         a1, .str.28
	add         t0, s2, s9
	lb          a3, 0(t0)
	mv          a2, s8
	mv          a0, s1
	call        fprintf

	// *** Basic block 42

	addi        s7, s7, 1
	li          t0, 16		// 0x10 ASCII \x10
	bne         s7, t0, .StaticVariable_label_306

	// *** Basic block 43

	mv          s7, x0
	lla         a1, .str.30
	mv          a0, s1
	call        fprintf

	// *** Basic block 45

.StaticVariable_label_306:

	// *** Basic block 46

.StaticVariable_label_307:
	addi        s9, s9, 1
	bge         s9, s10, .StaticVariable_label_268

	// *** Basic block 47

.StaticVariable_label_311:
	lla         a1, .str.32
	mv          a0, s1
	call        fprintf

	// *** Basic block 48

	j           .StaticVariable_label_319

	// *** Basic block 49

.StaticVariable_label_319:

	// *** Basic block 50

.StaticVariable_label_320:
	addi        s5, s5, 1
	bge         s5, s6, .StaticVariable_label_118

	// *** Basic block 51

.StaticVariable_label_324:
	sub         s6, s4, s3
	bge         x0, s6, .StaticVariable_label_337

	// *** Basic block 52

	lla         a1, .str.33
	mv          a2, s6
	mv          a0, s1
	call        fprintf

	// *** Basic block 53

.StaticVariable_label_337:
	lla         a1, .str.34
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
	j           fprintf
.func_end_StaticVariable:
	.size StaticVariable, .func_end_StaticVariable-StaticVariable

	.local  TlsVariable
	.type TlsVariable, @function

TlsVariable:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local StaticVariable
	j           StaticVariable
.func_end_TlsVariable:
	.size TlsVariable, .func_end_TlsVariable-TlsVariable

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
	lla         a1, .str.35
	ld          s3, 16(s2)
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 1

	lb          t0, 40(s2)
	beqz        t0, .BSSVariable_label_43

	// *** Basic block 2

	lla         a1, .str.36
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 3

	j           .BSSVariable_label_52

	// *** Basic block 4

.BSSVariable_label_43:
	lla         a1, .str.37
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 5

.BSSVariable_label_52:
	lla         a1, .str.38
	ld          a3, 48(s2)
	ld          a4, 56(s2)
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 6

	lla         a1, .str.39
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

	.local  TlsBSSVariable
	.type TlsBSSVariable, @function

TlsBSSVariable:

	// *** Basic block 0

	.global fprintf
	.local EmitP2Align
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
	lla         a1, .str.40
	ld          s3, 16(s2)
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 1

	lb          t0, 40(s2)
	beqz        t0, .TlsBSSVariable_label_45

	// *** Basic block 2

	lla         a1, .str.41
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 3

	j           .TlsBSSVariable_label_54

	// *** Basic block 4

.TlsBSSVariable_label_45:
	lla         a1, .str.42
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 5

.TlsBSSVariable_label_54:
	ld          a0, 56(s2)
	mv          a1, s1
	call        EmitP2Align

	// *** Basic block 6

	lla         a1, .str.43
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 7

	lla         a1, .str.44
	ld          a2, 48(s2)
	mv          a0, s1
	call        fprintf

	// *** Basic block 8

	lla         a1, .str.45
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           fprintf
.func_end_TlsBSSVariable:
	.size TlsBSSVariable, .func_end_TlsBSSVariable-TlsBSSVariable

	.local  StringLiteralSection
	.type StringLiteralSection, @function

StringLiteralSection:

	// *** Basic block 0

	.global fprintf
	// Leaf procedure, no stack frame generated
	lla         a1, .str.46
	j           fprintf
.func_end_StringLiteralSection:
	.size StringLiteralSection, .func_end_StringLiteralSection-StringLiteralSection

	.local  EmitLiteral
	.type EmitLiteral, @function

EmitLiteral:

	// *** Basic block 0

	.global fprintf
	.global StringInit
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
	beqz        t0, .EmitLiteral_label_29

	// *** Basic block 1

.EmitLiteral_label_26:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.EmitLiteral_label_29:
	lla         a1, .str.47
	lw          a2, 0(s1)
	mv          a0, s2
	call        fprintf

	// *** Basic block 3

	addi        a0, s0, -64
	mv          a1, x0
	call        StringInit

	// *** Basic block 4

	addi        a0, s1, 8
	addi        a1, s0, -64
	call        StringEscape

	// *** Basic block 5

	lla         a1, .str.48
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s2
	call        fprintf

	// *** Basic block 6

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 7

	lla         a1, .str.49
	lw          s3, 0(s1)
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 8

	lla         a1, .str.50
	ld          t0, 24(a0)
	addi        a3, t0, 1
	mv          a2, s3
	mv          a0, s2
	call        fprintf

	// *** Basic block 9

	lla         a1, .str.51
	mv          a0, s2
	call        fprintf

	// *** Basic block 10

	j           .EmitLiteral_label_26
.func_end_EmitLiteral:
	.size EmitLiteral, .func_end_EmitLiteral-EmitLiteral

	.local  EmitDebug
	.type EmitDebug, @function

EmitDebug:

	// *** Basic block 0

	ret         
.func_end_EmitDebug:
	.size EmitDebug, .func_end_EmitDebug-EmitDebug

	.local  Cleanup
	.type Cleanup, @function

Cleanup:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global PCodeGeneratorDelete
	j           PCodeGeneratorDelete
.func_end_Cleanup:
	.size Cleanup, .func_end_Cleanup-Cleanup

	.global NewPCodeTarget
	.type NewPCodeTarget, @function

NewPCodeTarget:

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
	.local TlsDataStart
	.local TlsBSSStart
	.local TlsVariable
	.local TlsBSSVariable
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
	lla         a1, .str.52
	mv          a0, s1
	call        StringInit

	// *** Basic block 2

	li          t0, 8		// 0x8 ASCII \x8
	sw          t0, 40(s1)
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
	la          t0, TlsDataStart
	sd          t0, 104(s1)
	la          t0, TlsBSSStart
	sd          t0, 112(s1)
	la          t0, TlsVariable
	sd          t0, 120(s1)
	la          t0, TlsBSSVariable
	sd          t0, 128(s1)
	la          t0, StringLiteralSection
	sd          t0, 136(s1)
	la          t0, EmitLiteral
	sd          t0, 144(s1)
	la          t0, EmitDebug
	sd          t0, 152(s1)
	mv          a0, s1

	// *** Basic block 3

.NewPCodeTarget_label_107:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewPCodeTarget:
	.size NewPCodeTarget, .func_end_NewPCodeTarget-NewPCodeTarget

.PCend:
	.data
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
	.asciz "// This is P-CODE.  It is not a real machine\n"
	.type .str.4, @object
	.size .str.4, 46

.str.5:
	.asciz "\t.file   \"%s\"\n"
	.type .str.5, @object
	.size .str.5, 15

.str.6:
	.asciz "\t.option pic\n"
	.type .str.6, @object
	.size .str.6, 14

.str.7:
	.asciz "\t.text\n"
	.type .str.7, @object
	.size .str.7, 8

.str.8:
	.asciz "\t.data\n"
	.type .str.8, @object
	.size .str.8, 8

.str.9:
	.asciz "\t.section \".tdata\", \"awT\", @progbits\n"
	.type .str.9, @object
	.size .str.9, 38

.str.10:
	.asciz "\t.section \".tbss\", \"awT\", @nobits\n"
	.type .str.10, @object
	.size .str.10, 35

.str.11:
	.asciz "\t.p2align  %d\n"
	.type .str.11, @object
	.size .str.11, 15

.str.12:
	.asciz "%s:\n"
	.type .str.12, @object
	.size .str.12, 5

.str.13:
	.asciz "\t.type   %s,@object\n"
	.type .str.13, @object
	.size .str.13, 21

.str.14:
	.asciz "\t.global %s\n"
	.type .str.14, @object
	.size .str.14, 13

.str.15:
	.asciz "\t.local  %s\n"
	.type .str.15, @object
	.size .str.15, 13

.str.16:
	.asciz "\t.size   %s,%zd\n"
	.type .str.16, @object
	.size .str.16, 17

.str.17:
	.asciz "\t.space  %d\n"
	.type .str.17, @object
	.size .str.17, 13

.str.18:
	.asciz "\t.byte   %d\n"
	.type .str.18, @object
	.size .str.18, 13

.str.19:
	.asciz "\t.half   %d\n"
	.type .str.19, @object
	.size .str.19, 13

.str.20:
	.asciz "\t.word   %d\n"
	.type .str.20, @object
	.size .str.20, 13

.str.21:
	.asciz "\t.long   %" PRId64 "\n"
	.type .str.21, @object
	.size .str.21, 15

.str.22:
	.asciz "\t.local %s\n"
	.type .str.22, @object
	.size .str.22, 12

.str.23:
	.asciz "\t.global %s\n"
	.type .str.23, @object
	.size .str.23, 13

.str.24:
	.asciz "\t.long    %s\n"
	.type .str.24, @object
	.size .str.24, 14

.str.25:
	.asciz "\t.long    .str.%d\n"
	.type .str.25, @object
	.size .str.25, 19

.str.26:
	.asciz "(null)"
	.type .str.26, @object
	.size .str.26, 1

.str.27:
	.asciz "\t.byte "
	.type .str.27, @object
	.size .str.27, 8

.str.28:
	.asciz "%s0x%02x"
	.type .str.28, @object
	.size .str.28, 9

.str.29:
	.asciz ","
	.type .str.29, @object
	.size .str.29, 2

.str.30:
	.asciz "\n"
	.type .str.30, @object
	.size .str.30, 2

.str.31:
	.asciz "(null)"
	.type .str.31, @object
	.size .str.31, 1

.str.32:
	.asciz "\n"
	.type .str.32, @object
	.size .str.32, 2

.str.33:
	.asciz "\t.space  %zd\n"
	.type .str.33, @object
	.size .str.33, 14

.str.34:
	.asciz "\n"
	.type .str.34, @object
	.size .str.34, 2

.str.35:
	.asciz "\t.type   %s,@object\n"
	.type .str.35, @object
	.size .str.35, 21

.str.36:
	.asciz "\t.global %s\n"
	.type .str.36, @object
	.size .str.36, 13

.str.37:
	.asciz "\t.local  %s\n"
	.type .str.37, @object
	.size .str.37, 13

.str.38:
	.asciz "\t.comm   %s,%zd,%zd\n"
	.type .str.38, @object
	.size .str.38, 21

.str.39:
	.asciz "\n"
	.type .str.39, @object
	.size .str.39, 2

.str.40:
	.asciz "\t.type   %s,@object\n"
	.type .str.40, @object
	.size .str.40, 21

.str.41:
	.asciz "\t.global %s\n"
	.type .str.41, @object
	.size .str.41, 13

.str.42:
	.asciz "\t.local  %s\n"
	.type .str.42, @object
	.size .str.42, 13

.str.43:
	.asciz "%s:\n"
	.type .str.43, @object
	.size .str.43, 5

.str.44:
	.asciz "\t.space   %zd\n"
	.type .str.44, @object
	.size .str.44, 15

.str.45:
	.asciz "\n"
	.type .str.45, @object
	.size .str.45, 2

.str.46:
	.asciz "\t.section \".rodata\", \"aMS\", @progbits\n"
	.type .str.46, @object
	.size .str.46, 39

.str.47:
	.asciz ".str.%d:\n"
	.type .str.47, @object
	.size .str.47, 10

.str.48:
	.asciz "\t.asciz \"%s\"\n"
	.type .str.48, @object
	.size .str.48, 14

.str.49:
	.asciz "\t.type .str.%d, @object\n"
	.type .str.49, @object
	.size .str.49, 25

.str.50:
	.asciz "\t.size .str.%d, %zd\n"
	.type .str.50, @object
	.size .str.50, 21

.str.51:
	.asciz "\n"
	.type .str.51, @object
	.size .str.51, 2

.str.52:
	.asciz "P-CODE"
	.type .str.52, @object
	.size .str.52, 7

