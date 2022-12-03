	.file   "codegen.c"
	.text
	.option pic
.PCbegin:
	.local  Trap
	.type Trap, @function

Trap:

	// *** Basic block 0

	ret         
.func_end_Trap:
	.size Trap, .func_end_Trap-Trap

	.local  TrapInstruction
	.type TrapInstruction, @function

TrapInstruction:

	// *** Basic block 0

	.local Trap
	// Leaf procedure, no stack frame generated
	lw          t0, 16(a0)
	li          t1, 22		// 0x16 ASCII \x16
	bne         t0, t1, .TrapInstruction_label_18

	// *** Basic block 1

	j           Trap

	// *** Basic block 2

.TrapInstruction_label_18:
	ret         
.func_end_TrapInstruction:
	.size TrapInstruction, .func_end_TrapInstruction-TrapInstruction

	.local  TrapFunctionBeforeCodegen
	.type TrapFunctionBeforeCodegen, @function

TrapFunctionBeforeCodegen:

	// *** Basic block 0

	.global StringEqual
	.local Trap
	addi sp, sp, -16
	// Saved return address (offset 8) and frame pointer (offset 0)
	sd ra, 8(sp)
	sd s0, 0(sp)
	addi s0, sp, 16
	// Local vars at offset -16(s0)
	// End of stack frame
	mv          t0, a0
	ld          t1, 8(t0)
	ld          a0, 32(t1)
	lla         a1, .str.1
	call        StringEqual

	// *** Basic block 1

	beqz        a0, .TrapFunctionBeforeCodegen_label_25

	// *** Basic block 2

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Trap

	// *** Basic block 3

.TrapFunctionBeforeCodegen_label_25:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TrapFunctionBeforeCodegen:
	.size TrapFunctionBeforeCodegen, .func_end_TrapFunctionBeforeCodegen-TrapFunctionBeforeCodegen

	.local  TrapFunctionAfterCodegen
	.type TrapFunctionAfterCodegen, @function

TrapFunctionAfterCodegen:

	// *** Basic block 0

	.global StringEqual
	.local Trap
	addi sp, sp, -16
	// Saved return address (offset 8) and frame pointer (offset 0)
	sd ra, 8(sp)
	sd s0, 0(sp)
	addi s0, sp, 16
	// Local vars at offset -16(s0)
	// End of stack frame
	mv          t0, a0
	ld          t1, 8(t0)
	ld          a0, 32(t1)
	lla         a1, .str.2
	call        StringEqual

	// *** Basic block 1

	beqz        a0, .TrapFunctionAfterCodegen_label_25

	// *** Basic block 2

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Trap

	// *** Basic block 3

.TrapFunctionAfterCodegen_label_25:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TrapFunctionAfterCodegen:
	.size TrapFunctionAfterCodegen, .func_end_TrapFunctionAfterCodegen-TrapFunctionAfterCodegen

	.global GeneratorInit
	.type GeneratorInit, @function

GeneratorInit:

	// *** Basic block 0

	.global TypeRecordIncRef
	.global ListInit
	.global VectorInit
	.global IRResetNodeId
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
	sd          a1, 0(s1)
	sd          a2, 8(s1)
	mv          a0, a2
	call        TypeRecordIncRef

	// *** Basic block 1

	addi        a0, s1, 16
	call        ListInit

	// *** Basic block 2

	sd          x0, 40(s1)
	sd          x0, 48(s1)
	sd          x0, 56(s1)
	sd          x0, 64(s1)
	sd          x0, 72(s1)
	sd          x0, 80(s1)
	sd          x0, 88(s1)
	addi        a0, s1, 96
	call        VectorInit

	// *** Basic block 3

	addi        a0, s1, 120
	call        VectorInit

	// *** Basic block 4

	addi        a0, s1, 144
	call        VectorInit

	// *** Basic block 5

	addi        a0, s1, 168
	call        VectorInit

	// *** Basic block 6

	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           IRResetNodeId
.func_end_GeneratorInit:
	.size GeneratorInit, .func_end_GeneratorInit-GeneratorInit

	.local  PrintIR
	.type PrintIR, @function

PrintIR:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global IRPrint
	mv          t0, a0
	mv          t1, t0
	mv          a0, t1
	j           IRPrint
.func_end_PrintIR:
	.size PrintIR, .func_end_PrintIR-PrintIR

	.global GeneratorPrintIR
	.type GeneratorPrintIR, @function

GeneratorPrintIR:

	// *** Basic block 0

	.global fprintf
	.global ListTraverse
	.local PrintIR
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
	mv          s3, x0

	// *** Basic block 1

.GeneratorPrintIR_label_24:
	lla         a1, .str.3
	mv          a0, s1
	call        fprintf

	// *** Basic block 2

.GeneratorPrintIR_label_31:
	addi        s3, s3, 1
	li          t0, 80		// 0x50 ASCII 'P'
	bge         s3, t0, .GeneratorPrintIR_label_24

	// *** Basic block 3

.GeneratorPrintIR_label_36:
	lla         a1, .str.4
	mv          a0, s1
	call        fprintf

	// *** Basic block 4

	lla         a1, .str.5
	ld          t0, 8(s2)
	ld          t0, 32(t0)
	ld          a2, 16(t0)
	mv          a0, s1
	call        fprintf

	// *** Basic block 5

	addi        a0, s2, 16
	mv          a2, s1
	la          t0, PrintIR
	mv          a1, t0
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ListTraverse
.func_end_GeneratorPrintIR:
	.size GeneratorPrintIR, .func_end_GeneratorPrintIR-GeneratorPrintIR

	.local  DestructIRNode
	.type DestructIRNode, @function

DestructIRNode:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global IRDestruct
	mv          t0, a0
	mv          t1, t0
	mv          a0, t1
	j           IRDestruct
.func_end_DestructIRNode:
	.size DestructIRNode, .func_end_DestructIRNode-DestructIRNode

	.global GeneratorDestruct
	.type GeneratorDestruct, @function

GeneratorDestruct:

	// *** Basic block 0

	.global TypeRecordDelete
	.global ListTraverse
	.local DestructIRNode
	.global ListDestruct
	.global VectorDestruct
	.global BasicBlockDelete
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
	ld          a0, 8(s1)
	call        TypeRecordDelete

	// *** Basic block 1

	addi        a0, s1, 16
	mv          a2, x0
	la          t0, DestructIRNode
	mv          a1, t0
	call        ListTraverse

	// *** Basic block 2

	addi        a0, s1, 16
	call        ListDestruct

	// *** Basic block 3

	addi        a0, s1, 96
	call        VectorDestruct

	// *** Basic block 4

	addi        a0, s1, 120
	call        VectorDestruct

	// *** Basic block 5

	addi        a0, s1, 144
	call        VectorDestruct

	// *** Basic block 6

	mv          s2, x0
	addi        t0, s1, 168
	ld          s3, 8(t0)
	bge         x0, s3, .GeneratorDestruct_label_68

	// *** Basic block 7

	ld          s4, 168(s1)

	// *** Basic block 8

.GeneratorDestruct_label_55:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          s1, 0(t0)
	mv          a0, s1
	call        BasicBlockDelete

	// *** Basic block 9

.GeneratorDestruct_label_64:
	addi        s2, s2, 1
	bge         s2, s3, .GeneratorDestruct_label_55

	// *** Basic block 10

.GeneratorDestruct_label_68:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GeneratorDestruct:
	.size GeneratorDestruct, .func_end_GeneratorDestruct-GeneratorDestruct

	.global CheckForVarUse
	.type CheckForVarUse, @function

CheckForVarUse:

	// *** Basic block 0

	.global IRSetVarUse
	.global CheckForVarUse
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
	lw          s3, 0(s1)
	li          t0, 2		// 0x2 ASCII \x2
	beq         s3, t0, .CheckForVarUse_label_40

	// *** Basic block 1

	li          t0, 19		// 0x13 ASCII \x13
	beq         s3, t0, .CheckForVarUse_label_51

	// *** Basic block 2

	li          t0, 33		// 0x21 ASCII '!'
	beq         s3, t0, .CheckForVarUse_label_50

	// *** Basic block 3

	li          t0, 50		// 0x32 ASCII '2'
	beq         s3, t0, .CheckForVarUse_label_52

	// *** Basic block 4

.CheckForVarUse_label_38:
	j           .CheckForVarUse_label_62

	// *** Basic block 5

.CheckForVarUse_label_40:
	mv          s4, s1
	ld          a1, 56(s4)
	mv          a0, s2
	call        IRSetVarUse

	// *** Basic block 6

	j           .CheckForVarUse_label_62

	// *** Basic block 7

.CheckForVarUse_label_50:

	// *** Basic block 8

.CheckForVarUse_label_51:

	// *** Basic block 9

.CheckForVarUse_label_52:
	mv          s3, s1
	ld          a1, 56(s3)
	mv          a0, s2
	call        CheckForVarUse

	// *** Basic block 10

	j           .CheckForVarUse_label_62

	// *** Basic block 11

.CheckForVarUse_label_62:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CheckForVarUse:
	.size CheckForVarUse, .func_end_CheckForVarUse-CheckForVarUse

	.global CheckForVarDef
	.type CheckForVarDef, @function

CheckForVarDef:

	// *** Basic block 0

	.global IRSetVarDef
	.global CheckForVarDef
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
	lw          s3, 0(s1)
	li          t0, 2		// 0x2 ASCII \x2
	beq         s3, t0, .CheckForVarDef_label_82

	// *** Basic block 1

	li          t0, 12		// 0xc ASCII \xc
	beq         s3, t0, .CheckForVarDef_label_105

	// *** Basic block 2

	li          t0, 19		// 0x13 ASCII \x13
	beq         s3, t0, .CheckForVarDef_label_95

	// *** Basic block 3

	li          t0, 33		// 0x21 ASCII '!'
	beq         s3, t0, .CheckForVarDef_label_93

	// *** Basic block 4

	li          t0, 50		// 0x32 ASCII '2'
	beq         s3, t0, .CheckForVarDef_label_94

	// *** Basic block 5

	li          t0, 62		// 0x3e ASCII '>'
	beq         s3, t0, .CheckForVarDef_label_130

	// *** Basic block 6

	li          t0, 76		// 0x4c ASCII 'L'
	beq         s3, t0, .CheckForVarDef_label_116

	// *** Basic block 7

	li          t0, 77		// 0x4d ASCII 'M'
	beq         s3, t0, .CheckForVarDef_label_117

	// *** Basic block 8

	li          t0, 78		// 0x4e ASCII 'N'
	beq         s3, t0, .CheckForVarDef_label_115

	// *** Basic block 9

	li          t0, 79		// 0x4f ASCII 'O'
	beq         s3, t0, .CheckForVarDef_label_118

	// *** Basic block 10

	li          t0, 86		// 0x56 ASCII 'V'
	beq         s3, t0, .CheckForVarDef_label_92

	// *** Basic block 11

	j           .CheckForVarDef_label_139

	// *** Basic block 12

.CheckForVarDef_label_82:
	mv          s7, s1
	ld          a1, 56(s7)
	mv          a0, s2
	call        IRSetVarDef

	// *** Basic block 13

	j           .CheckForVarDef_label_141

	// *** Basic block 14

.CheckForVarDef_label_92:

	// *** Basic block 15

.CheckForVarDef_label_93:

	// *** Basic block 16

.CheckForVarDef_label_94:

	// *** Basic block 17

.CheckForVarDef_label_95:
	mv          s5, s1
	ld          a1, 56(s5)
	mv          a0, s2
	call        CheckForVarDef

	// *** Basic block 18

	j           .CheckForVarDef_label_141

	// *** Basic block 19

.CheckForVarDef_label_105:
	mv          s6, s1
	ld          a1, 56(s6)
	mv          a0, s2
	call        CheckForVarDef

	// *** Basic block 20

	j           .CheckForVarDef_label_141

	// *** Basic block 21

.CheckForVarDef_label_115:

	// *** Basic block 22

.CheckForVarDef_label_116:

	// *** Basic block 23

.CheckForVarDef_label_117:

	// *** Basic block 24

.CheckForVarDef_label_118:
	mv          s3, s1
	ld          t0, 64(s3)
	ld          t0, 0(t0)
	ld          a1, 0(t0)
	mv          a0, s2
	call        CheckForVarDef

	// *** Basic block 25

	j           .CheckForVarDef_label_141

	// *** Basic block 26

.CheckForVarDef_label_130:
	mv          s4, s1
	ld          a1, 56(s4)
	mv          a0, s2
	call        CheckForVarDef

	// *** Basic block 27

.CheckForVarDef_label_139:
	j           .CheckForVarDef_label_141

	// *** Basic block 28

.CheckForVarDef_label_141:
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
.func_end_CheckForVarDef:
	.size CheckForVarDef, .func_end_CheckForVarDef-CheckForVarDef

	.global GeneratorGetReturnLabel
	.type GeneratorGetReturnLabel, @function

GeneratorGetReturnLabel:

	// *** Basic block 0

	.global GeneratorEmit
	.global NewIR
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
	ld          t0, 88(s1)
	bne         t0, x0, .GeneratorGetReturnLabel_label_48

	// *** Basic block 1

	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewIR

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 3

	sd          a0, 88(s1)
	li          t0, 95		// 0x5f ASCII '_'
	mv          a0, t0
	call        NewIR

	// *** Basic block 4

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 5

	li          t0, 93		// 0x5d ASCII ']'
	mv          a0, t0
	call        NewIR

	// *** Basic block 6

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 7

.GeneratorGetReturnLabel_label_48:
	ld          a0, 88(s1)

	// *** Basic block 8

.GeneratorGetReturnLabel_label_52:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GeneratorGetReturnLabel:
	.size GeneratorGetReturnLabel, .func_end_GeneratorGetReturnLabel-GeneratorGetReturnLabel

	.global GeneratorFirstInstruction
	.type GeneratorFirstInstruction, @function

GeneratorFirstInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          a0, 16(a0)

	// *** Basic block 1

.GeneratorFirstInstruction_label_11:
	ret         
.func_end_GeneratorFirstInstruction:
	.size GeneratorFirstInstruction, .func_end_GeneratorFirstInstruction-GeneratorFirstInstruction

	.global GeneratorLastInstruction
	.type GeneratorLastInstruction, @function

GeneratorLastInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	addi        t0, a0, 16
	ld          a0, 8(t0)

	// *** Basic block 1

.GeneratorLastInstruction_label_12:
	ret         
.func_end_GeneratorLastInstruction:
	.size GeneratorLastInstruction, .func_end_GeneratorLastInstruction-GeneratorLastInstruction

	.global GeneratorError
	.type GeneratorError, @function

GeneratorError:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global VGeneratorError
	mv          t0, s0
	mv          a3, t0
	j           VGeneratorError
.func_end_GeneratorError:
	.size GeneratorError, .func_end_GeneratorError-GeneratorError

	.global VGeneratorError
	.type VGeneratorError, @function

VGeneratorError:

	// *** Basic block 0

	.global DecodeSourceLocation
	.global VReportError
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Local vars at offset -48(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	mv          t0, a1
	mv          s1, a2
	mv          s2, a3
	ld          a0, 40(t0)
	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	call        DecodeSourceLocation

	// *** Basic block 1

	ld          a0, -48(s0)
	lw          a1, -40(s0)
	mv          a3, s2
	mv          a2, s1
	call        VReportError

	// *** Basic block 2

	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_VGeneratorError:
	.size VGeneratorError, .func_end_VGeneratorError-VGeneratorError

	.global GeneratorWarning
	.type GeneratorWarning, @function

GeneratorWarning:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global VGeneratorWarning
	mv          t0, s0
	mv          a4, t0
	j           VGeneratorWarning
.func_end_GeneratorWarning:
	.size GeneratorWarning, .func_end_GeneratorWarning-GeneratorWarning

	.global VGeneratorWarning
	.type VGeneratorWarning, @function

VGeneratorWarning:

	// *** Basic block 0

	.global DecodeSourceLocation
	.global VReportWarning
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
	// End of stack frame
	mv          t0, a1
	mv          s1, a2
	mv          s2, a3
	mv          s3, a4
	ld          a0, 40(t0)
	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	call        DecodeSourceLocation

	// *** Basic block 1

	ld          a0, -48(s0)
	lw          a1, -40(s0)
	mv          a4, s3
	mv          a3, s2
	mv          a2, s1
	call        VReportWarning

	// *** Basic block 2

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_VGeneratorWarning:
	.size VGeneratorWarning, .func_end_VGeneratorWarning-VGeneratorWarning

	.global GeneratorEmit
	.type GeneratorEmit, @function

GeneratorEmit:

	// *** Basic block 0

	.local TrapInstruction
	.global IRInList
	.global ListAppend
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
	mv          s1, a1
	mv          s2, a0
	mv          a0, s1
	call        TrapInstruction

	// *** Basic block 1

	mv          a0, s1
	call        IRInList

	// *** Basic block 2

	beqz        a0, .GeneratorEmit_label_25

	// *** Basic block 3

	mv          a0, s1

	// *** Basic block 4

.GeneratorEmit_label_22:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.GeneratorEmit_label_25:
	addi        a0, s2, 16
	mv          a1, s1
	call        ListAppend

	// *** Basic block 6

	mv          a0, s1
	j           .GeneratorEmit_label_22
.func_end_GeneratorEmit:
	.size GeneratorEmit, .func_end_GeneratorEmit-GeneratorEmit

	.global GeneratorEmitBefore
	.type GeneratorEmitBefore, @function

GeneratorEmitBefore:

	// *** Basic block 0

	.local TrapInstruction
	.global IRInList
	.global ListInsertBefore
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
	mv          s3, a2
	mv          a0, s1
	call        TrapInstruction

	// *** Basic block 1

	mv          a0, s1
	call        IRInList

	// *** Basic block 2

	beqz        a0, .GeneratorEmitBefore_label_28

	// *** Basic block 3

	mv          a0, s1

	// *** Basic block 4

.GeneratorEmitBefore_label_25:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.GeneratorEmitBefore_label_28:
	addi        a0, s2, 16
	mv          a2, s3
	mv          a1, s1
	call        ListInsertBefore

	// *** Basic block 6

	mv          a0, s1
	j           .GeneratorEmitBefore_label_25
.func_end_GeneratorEmitBefore:
	.size GeneratorEmitBefore, .func_end_GeneratorEmitBefore-GeneratorEmitBefore

	.global GeneratorEmitAfter
	.type GeneratorEmitAfter, @function

GeneratorEmitAfter:

	// *** Basic block 0

	.local TrapInstruction
	.global GeneratorEmit
	.global IRInList
	.global ListInsertAfter
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
	mv          s2, a2
	mv          s3, a0
	mv          a0, s1
	call        TrapInstruction

	// *** Basic block 1

	bne         s2, x0, .GeneratorEmitAfter_label_35

	// *** Basic block 2

	mv          a1, s1
	mv          a0, s3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GeneratorEmit

	// *** Basic block 4

.GeneratorEmitAfter_label_32:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.GeneratorEmitAfter_label_35:
	mv          a0, s1
	call        IRInList

	// *** Basic block 6

	beqz        a0, .GeneratorEmitAfter_label_43

	// *** Basic block 7

	mv          a0, s1
	j           .GeneratorEmitAfter_label_32

	// *** Basic block 8

.GeneratorEmitAfter_label_43:
	addi        a0, s3, 16
	mv          a2, s2
	mv          a1, s1
	call        ListInsertAfter

	// *** Basic block 9

	mv          a0, s1
	j           .GeneratorEmitAfter_label_32
.func_end_GeneratorEmitAfter:
	.size GeneratorEmitAfter, .func_end_GeneratorEmitAfter-GeneratorEmitAfter

	.global GeneratorEmitConstant
	.type GeneratorEmitConstant, @function

GeneratorEmitConstant:

	// *** Basic block 0

	.global IRInList
	.global GeneratorEmitBefore
	.global GeneratorEmitAfter
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
	mv          a0, s1
	call        IRInList

	// *** Basic block 1

	beqz        a0, .GeneratorEmitConstant_label_23

	// *** Basic block 2

	mv          a0, s1

	// *** Basic block 3

.GeneratorEmitConstant_label_20:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.GeneratorEmitConstant_label_23:
	ld          s3, 40(s2)
	bne         s3, x0, .GeneratorEmitConstant_label_42

	// *** Basic block 5

	ld          a2, 16(s2)
	mv          a1, s1
	mv          a0, s2
	call        GeneratorEmitBefore

	// *** Basic block 6

	sd          a0, 40(s2)
	j           .GeneratorEmitConstant_label_52

	// *** Basic block 7

.GeneratorEmitConstant_label_42:
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        GeneratorEmitAfter

	// *** Basic block 8

	sd          a0, 40(s2)

	// *** Basic block 9

.GeneratorEmitConstant_label_52:
	mv          a0, s1
	j           .GeneratorEmitConstant_label_20
.func_end_GeneratorEmitConstant:
	.size GeneratorEmitConstant, .func_end_GeneratorEmitConstant-GeneratorEmitConstant

	.global GeneratorEmitVariable
	.type GeneratorEmitVariable, @function

GeneratorEmitVariable:

	// *** Basic block 0

	.global IRInList
	.global GeneratorEmitBefore
	.global GeneratorEmitAfter
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
	mv          a0, s1
	call        IRInList

	// *** Basic block 1

	beqz        a0, .GeneratorEmitVariable_label_23

	// *** Basic block 2

	mv          a0, s1

	// *** Basic block 3

.GeneratorEmitVariable_label_20:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.GeneratorEmitVariable_label_23:
	ld          s3, 48(s2)
	bne         s3, x0, .GeneratorEmitVariable_label_42

	// *** Basic block 5

	ld          a2, 16(s2)
	mv          a1, s1
	mv          a0, s2
	call        GeneratorEmitBefore

	// *** Basic block 6

	sd          a0, 48(s2)
	j           .GeneratorEmitVariable_label_52

	// *** Basic block 7

.GeneratorEmitVariable_label_42:
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        GeneratorEmitAfter

	// *** Basic block 8

	sd          a0, 48(s2)

	// *** Basic block 9

.GeneratorEmitVariable_label_52:
	mv          a0, s1
	j           .GeneratorEmitVariable_label_20
.func_end_GeneratorEmitVariable:
	.size GeneratorEmitVariable, .func_end_GeneratorEmitVariable-GeneratorEmitVariable

	.global GeneratorRemoveInstruction
	.type GeneratorRemoveInstruction, @function

GeneratorRemoveInstruction:

	// *** Basic block 0

	.global ListDeleteElement
	.global IRRemoveNode
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
	mv          t0, a0
	mv          s1, a1
	addi        a0, t0, 16
	call        ListDeleteElement

	// *** Basic block 1

	mv          a0, s1
	call        IRRemoveNode

	// *** Basic block 2

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_GeneratorRemoveInstruction:
	.size GeneratorRemoveInstruction, .func_end_GeneratorRemoveInstruction-GeneratorRemoveInstruction

	.global GeneratorMoveInstructionBefore
	.type GeneratorMoveInstructionBefore, @function

GeneratorMoveInstructionBefore:

	// *** Basic block 0

	.global ListDeleteElement
	.global ListInsertBefore
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
	addi        a0, s1, 16
	call        ListDeleteElement

	// *** Basic block 1

	addi        a0, s1, 16
	mv          a2, s3
	mv          a1, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ListInsertBefore
.func_end_GeneratorMoveInstructionBefore:
	.size GeneratorMoveInstructionBefore, .func_end_GeneratorMoveInstructionBefore-GeneratorMoveInstructionBefore

	.global GeneratorMoveInstructionAfter
	.type GeneratorMoveInstructionAfter, @function

GeneratorMoveInstructionAfter:

	// *** Basic block 0

	.global ListDeleteElement
	.global ListInsertAfter
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
	addi        a0, s1, 16
	call        ListDeleteElement

	// *** Basic block 1

	addi        a0, s1, 16
	mv          a2, s3
	mv          a1, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ListInsertAfter
.func_end_GeneratorMoveInstructionAfter:
	.size GeneratorMoveInstructionAfter, .func_end_GeneratorMoveInstructionAfter-GeneratorMoveInstructionAfter

	.global GeneratorReplaceInstruction
	.type GeneratorReplaceInstruction, @function

GeneratorReplaceInstruction:

	// *** Basic block 0

	.global VectorAppend
	.global VectorClear
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
	mv          s2, a2
	mv          s3, x0
	addi        t0, s1, 48
	ld          s4, 8(t0)
	bge         x0, s4, .GeneratorReplaceInstruction_label_71

	// *** Basic block 1

	ld          t0, 48(s1)

	// *** Basic block 2

.GeneratorReplaceInstruction_label_25:
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s5, 0(t0)
	mv          s6, x0
	addi        t0, s5, 24
	ld          t0, 8(t0)
	bge         x0, t0, .GeneratorReplaceInstruction_label_66

	// *** Basic block 3

.GeneratorReplaceInstruction_label_38:
	ld          t0, 24(s5)
	slli        s7, s6, 3
	add         t0, t0, s7
	ld          t1, 0(t0)
	bne         t1, s1, .GeneratorReplaceInstruction_label_58

	// *** Basic block 4

	ld          t0, 24(s5)
	add         t0, t0, s7
	sd          s2, 0(t0)
	addi        a0, s2, 48
	mv          a1, s5
	call        VectorAppend

	// *** Basic block 5

.GeneratorReplaceInstruction_label_58:

	// *** Basic block 6

.GeneratorReplaceInstruction_label_59:
	addi        s6, s6, 1
	addi        t0, s5, 24
	ld          t0, 8(t0)
	bge         s6, t0, .GeneratorReplaceInstruction_label_38

	// *** Basic block 7

.GeneratorReplaceInstruction_label_66:

	// *** Basic block 8

.GeneratorReplaceInstruction_label_67:
	addi        s3, s3, 1
	bge         s3, s4, .GeneratorReplaceInstruction_label_25

	// *** Basic block 9

.GeneratorReplaceInstruction_label_71:
	addi        a0, s1, 48
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
	j           VectorClear
.func_end_GeneratorReplaceInstruction:
	.size GeneratorReplaceInstruction, .func_end_GeneratorReplaceInstruction-GeneratorReplaceInstruction

	.global GeneratorGetIntConstant
	.type GeneratorGetIntConstant, @function

GeneratorGetIntConstant:

	// *** Basic block 0

	.global malloc
	.global NewTypeRecord
	.global GeneratorEmitConstant
	.global NewIntIRConstant
	.global VectorAppend
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
	mv          s3, a2
	li          s4, 2		// 0x2 ASCII \x2
	beq         s1, x0, .GeneratorGetIntConstant_label_32

	// *** Basic block 1

	lw          s4, 8(s1)

	// *** Basic block 2

.GeneratorGetIntConstant_label_32:
	mv          s5, x0
	addi        t0, s2, 96
	ld          s6, 8(t0)
	bge         x0, s6, .GeneratorGetIntConstant_label_69

	// *** Basic block 3

	ld          t0, 96(s2)

	// *** Basic block 4

.GeneratorGetIntConstant_label_41:
	slli        t1, s5, 3
	add         t0, t0, t1
	ld          s7, 0(t0)
	ld          t1, 8(s7)
	sub         t2, t1, s3
	seqz        t0, t2
	bne         t1, s3, .GeneratorGetIntConstant_label_56

	// *** Basic block 5

	lw          t1, 0(s7)
	sub         t1, t1, s4
	seqz        t0, t1

	// *** Basic block 6

.GeneratorGetIntConstant_label_56:
	beqz        t0, .GeneratorGetIntConstant_label_64

	// *** Basic block 7

	ld          a0, 16(s7)

	// *** Basic block 8

.GeneratorGetIntConstant_label_61:
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

	// *** Basic block 9

.GeneratorGetIntConstant_label_64:

	// *** Basic block 10

.GeneratorGetIntConstant_label_65:
	addi        s5, s5, 1
	bge         s5, s6, .GeneratorGetIntConstant_label_41

	// *** Basic block 11

.GeneratorGetIntConstant_label_69:
	li          t0, 24		// 0x18 ASCII \x18
	mv          a0, t0
	call        malloc

	// *** Basic block 12

	mv          s7, a0
	sd          s3, 8(s7)
	beq         s1, x0, .GeneratorGetIntConstant_label_82

	// *** Basic block 13

	lw          t0, 8(s1)
	sw          t0, 0(s7)
	j           .GeneratorGetIntConstant_label_85

	// *** Basic block 14

.GeneratorGetIntConstant_label_82:
	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, 0(s7)

	// *** Basic block 15

.GeneratorGetIntConstant_label_85:
	bne         s1, x0, .GeneratorGetIntConstant_label_96

	// *** Basic block 16

	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 17

	mv          s1, a0

	// *** Basic block 18

.GeneratorGetIntConstant_label_96:
	mv          a1, s3
	mv          a0, s1
	call        NewIntIRConstant

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmitConstant

	// *** Basic block 20

	sd          a0, 16(s7)
	addi        a0, s2, 96
	mv          a1, s7
	call        VectorAppend

	// *** Basic block 21

	ld          a0, 16(s7)
	j           .GeneratorGetIntConstant_label_61
.func_end_GeneratorGetIntConstant:
	.size GeneratorGetIntConstant, .func_end_GeneratorGetIntConstant-GeneratorGetIntConstant

	.global GeneratorGetFloatingPointConstant
	.type GeneratorGetFloatingPointConstant, @function

GeneratorGetFloatingPointConstant:

	// *** Basic block 0

	.global malloc
	.global GeneratorEmitConstant
	.global NewFloatingPointIRConstant
	.global VectorAppend
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
	// Saved floating point registers.
	fsd fs0, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	fmv.d       fs0, fa0
	li          s3, 64		// 0x40 ASCII '@'
	beq         s1, x0, .GeneratorGetFloatingPointConstant_label_32

	// *** Basic block 1

	lw          s3, 8(s1)

	// *** Basic block 2

.GeneratorGetFloatingPointConstant_label_32:
	mv          s4, x0
	addi        t0, s2, 120
	ld          s5, 8(t0)
	bge         x0, s5, .GeneratorGetFloatingPointConstant_label_68

	// *** Basic block 3

	ld          t0, 120(s2)

	// *** Basic block 4

.GeneratorGetFloatingPointConstant_label_41:
	slli        t1, s4, 3
	add         t0, t0, t1
	ld          s6, 0(t0)
	fld         ft0, 8(s6)
	feq.d       t0, ft0, fs0
	beqz        t0, .GeneratorGetFloatingPointConstant_label_55

	// *** Basic block 5

	lw          t1, 0(s6)
	sub         t1, t1, s3
	seqz        t0, t1

	// *** Basic block 6

.GeneratorGetFloatingPointConstant_label_55:
	beqz        t0, .GeneratorGetFloatingPointConstant_label_63

	// *** Basic block 7

	ld          a0, 16(s6)

	// *** Basic block 8

.GeneratorGetFloatingPointConstant_label_60:
	// Restored registers.
	fld fs0, 56(sp)
	ld s1, 48(sp)
	ld s2, 40(sp)
	ld s3, 32(sp)
	ld s4, 24(sp)
	ld s5, 16(sp)
	ld s6, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 9

.GeneratorGetFloatingPointConstant_label_63:

	// *** Basic block 10

.GeneratorGetFloatingPointConstant_label_64:
	addi        s4, s4, 1
	bge         s4, s5, .GeneratorGetFloatingPointConstant_label_41

	// *** Basic block 11

.GeneratorGetFloatingPointConstant_label_68:
	li          t0, 24		// 0x18 ASCII \x18
	mv          a0, t0
	call        malloc

	// *** Basic block 12

	mv          s6, a0
	fsd         fs0, 8(s6)
	beq         s1, x0, .GeneratorGetFloatingPointConstant_label_81

	// *** Basic block 13

	lw          t0, 8(s1)
	sw          t0, 0(s6)
	j           .GeneratorGetFloatingPointConstant_label_84

	// *** Basic block 14

.GeneratorGetFloatingPointConstant_label_81:
	li          t0, 64		// 0x40 ASCII '@'
	sw          t0, 0(s6)

	// *** Basic block 15

.GeneratorGetFloatingPointConstant_label_84:
	fmv.d       fa0, fs0
	mv          a0, s1
	call        NewFloatingPointIRConstant

	// *** Basic block 16

	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmitConstant

	// *** Basic block 17

	sd          a0, 16(s6)
	addi        a0, s2, 96
	mv          a1, s6
	call        VectorAppend

	// *** Basic block 18

	ld          a0, 16(s6)
	j           .GeneratorGetFloatingPointConstant_label_60
.func_end_GeneratorGetFloatingPointConstant:
	.size GeneratorGetFloatingPointConstant, .func_end_GeneratorGetFloatingPointConstant-GeneratorGetFloatingPointConstant

	.global GeneratorGetVariable
	.type GeneratorGetVariable, @function

GeneratorGetVariable:

	// *** Basic block 0

	.global malloc
	.global GeneratorEmitVariable
	.global NewIRVariable
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
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, x0
	addi        t0, s1, 144
	ld          s4, 8(t0)
	bge         x0, s4, .GeneratorGetVariable_label_51

	// *** Basic block 1

	ld          t0, 144(s1)

	// *** Basic block 2

.GeneratorGetVariable_label_29:
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s5, 0(t0)
	ld          t0, 8(s5)
	bne         t0, s2, .GeneratorGetVariable_label_46

	// *** Basic block 3

	ld          a0, 16(s5)

	// *** Basic block 4

.GeneratorGetVariable_label_43:
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

.GeneratorGetVariable_label_46:

	// *** Basic block 6

.GeneratorGetVariable_label_47:
	addi        s3, s3, 1
	bge         s3, s4, .GeneratorGetVariable_label_29

	// *** Basic block 7

.GeneratorGetVariable_label_51:
	li          t0, 24		// 0x18 ASCII \x18
	mv          a0, t0
	call        malloc

	// *** Basic block 8

	mv          s4, a0
	sd          s2, 8(s4)
	ld          t0, 40(s2)
	lw          t0, 8(t0)
	sw          t0, 0(s4)
	mv          a0, s2
	call        NewIRVariable

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmitVariable

	// *** Basic block 10

	sd          a0, 16(s4)
	addi        a0, s1, 144
	mv          a1, s4
	call        VectorAppend

	// *** Basic block 11

	ld          a0, 16(s4)
	j           .GeneratorGetVariable_label_43
.func_end_GeneratorGetVariable:
	.size GeneratorGetVariable, .func_end_GeneratorGetVariable-GeneratorGetVariable

	.local  GeneratorNewBasicBlock
	.type GeneratorNewBasicBlock, @function

GeneratorNewBasicBlock:

	// *** Basic block 0

	.global NewBasicBlock
	.global VectorAppend
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
	addi        t0, s1, 168
	ld          a0, 8(t0)
	call        NewBasicBlock

	// *** Basic block 1

	mv          s2, a0
	addi        a0, s1, 168
	mv          a1, s2
	call        VectorAppend

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.GeneratorNewBasicBlock_label_25:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GeneratorNewBasicBlock:
	.size GeneratorNewBasicBlock, .func_end_GeneratorNewBasicBlock-GeneratorNewBasicBlock

	.local  FindBasicBlock
	.type FindBasicBlock, @function

FindBasicBlock:

	// *** Basic block 0

	.global VectorGet
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 168
	j           VectorGet
.func_end_FindBasicBlock:
	.size FindBasicBlock, .func_end_FindBasicBlock-FindBasicBlock

	.local  PrintBasicBlocks
	.type PrintBasicBlocks, @function

PrintBasicBlocks:

	// *** Basic block 0

	.global BasicBlockPrint
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
	mv          s3, x0
	addi        t0, s1, 168
	ld          s4, 8(t0)
	bge         x0, s4, .PrintBasicBlocks_label_53

	// *** Basic block 1

	ld          s5, 168(s1)
	ld          s6, 192(s1)
	ld          s7, 200(s1)

	// *** Basic block 2

.PrintBasicBlocks_label_29:
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          s5, 0(t0)
	mv          a4, s2
	mv          a3, s7
	mv          a2, s6
	mv          a1, s5
	mv          a0, s1
	call        BasicBlockPrint

	// *** Basic block 3

.PrintBasicBlocks_label_49:
	addi        s3, s3, 1
	bge         s3, s4, .PrintBasicBlocks_label_29

	// *** Basic block 4

.PrintBasicBlocks_label_53:
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
.func_end_PrintBasicBlocks:
	.size PrintBasicBlocks, .func_end_PrintBasicBlocks-PrintBasicBlocks

	.local  CreateBasicBlocks
	.type CreateBasicBlocks, @function

CreateBasicBlocks:

	// *** Basic block 0

	.local GeneratorNewBasicBlock
	.global GeneratorFirstInstruction
	.global IRIsVarDef
	.global MapInsert
	.global IRIsVarRef
	.global SetInsert
	.global IRPrev
	.global IRIsBranch
	.global IRIsReturn
	.global IRIsCall
	.global VectorAppend
	.global IRNext
	.global GeneratorLastInstruction
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
	mv          s2, a1
	call        GeneratorNewBasicBlock

	// *** Basic block 1

	sd          a0, 192(s1)
	ld          s3, 192(s1)
	mv          a0, s1
	call        GeneratorFirstInstruction

	// *** Basic block 2

	sd          a0, 56(s3)
	ld          s4, 56(s3)
	beq         s4, x0, .CreateBasicBlocks_label_211

	// *** Basic block 3

.CreateBasicBlocks_label_55:
	mv          a0, s4
	call        IRIsVarDef

	// *** Basic block 4

	beqz        a0, .CreateBasicBlocks_label_82

	// *** Basic block 5

	ld          t0, 120(s4)
	sd          t0, -32(s0)
	addi        t0, s0, -32
	sd          x0, 8(t0)
	addi        a0, s3, 160
	addi        sp, sp, -16
	ld          t0, -32(s0)
	sd          t0, 0(sp)
	ld          t0, -24(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	call        MapInsert

	// *** Basic block 7

.CreateBasicBlocks_label_82:
	mv          a0, s4
	call        IRIsVarRef

	// *** Basic block 8

	beqz        a0, .CreateBasicBlocks_label_93

	// *** Basic block 9

	addi        a0, s3, 192
	ld          a1, 120(s4)
	call        SetInsert

	// *** Basic block 10

.CreateBasicBlocks_label_93:
	lw          s5, 20(s4)
	addi        t1, s5, -92
	seqz        t0, t1
	li          t1, 92		// 0x5c ASCII '\'
	beq         s5, t1, .CreateBasicBlocks_label_106

	// *** Basic block 11

	addi        t1, s5, -120
	seqz        t0, t1

	// *** Basic block 12

.CreateBasicBlocks_label_106:
	bnez        t0, .CreateBasicBlocks_label_111

	// *** Basic block 13

	addi        t1, s5, -119
	seqz        t0, t1

	// *** Basic block 14

.CreateBasicBlocks_label_111:
	beqz        t0, .CreateBasicBlocks_label_117

	// *** Basic block 15

	lw          t0, 232(s3)
	addi        t0, t0, 1
	sw          t0, 232(s3)

	// *** Basic block 16

.CreateBasicBlocks_label_117:
	li          t0, 17		// 0x11 ASCII \x11
	bne         s5, t0, .CreateBasicBlocks_label_139

	// *** Basic block 17

	mv          a0, s4
	call        IRPrev

	// *** Basic block 18

	sd          a0, 64(s3)
	mv          a0, s1
	call        GeneratorNewBasicBlock

	// *** Basic block 19

	mv          s5, a0
	sd          s5, 72(s4)
	sd          s4, 56(s5)
	mv          s3, s5
	j           .CreateBasicBlocks_label_202

	// *** Basic block 20

.CreateBasicBlocks_label_139:
	mv          a0, s4
	call        IRIsBranch

	// *** Basic block 21

	mv          s6, a0
	bnez        a0, .CreateBasicBlocks_label_151

	// *** Basic block 22

	mv          a0, s4
	call        IRIsReturn

	// *** Basic block 23

	mv          s6, a0

	// *** Basic block 24

.CreateBasicBlocks_label_151:
	bnez        s6, .CreateBasicBlocks_label_157

	// *** Basic block 25

	mv          a0, s4
	call        IRIsCall

	// *** Basic block 26

	mv          s6, a0

	// *** Basic block 27

.CreateBasicBlocks_label_157:
	beqz        s6, .CreateBasicBlocks_label_198

	// *** Basic block 28

	mv          a0, s4
	call        IRIsReturn

	// *** Basic block 29

	sb          a0, 236(s3)
	sd          s4, 64(s3)
	sd          s3, 72(s4)
	mv          a1, s4
	mv          a0, s2
	call        VectorAppend

	// *** Basic block 30

	mv          a0, s4
	call        IRNext

	// *** Basic block 31

	mv          s7, a0
	sub         t1, s7, x0
	snez        t0, t1
	beq         s7, x0, .CreateBasicBlocks_label_186

	// *** Basic block 32

	lw          t1, 20(s7)
	addi        t1, t1, -17
	snez        t0, t1

	// *** Basic block 33

.CreateBasicBlocks_label_186:
	beqz        t0, .CreateBasicBlocks_label_196

	// *** Basic block 34

	mv          a0, s1
	call        GeneratorNewBasicBlock

	// *** Basic block 35

	mv          s2, a0
	sd          s7, 56(s2)
	mv          s3, s2

	// *** Basic block 36

.CreateBasicBlocks_label_196:
	j           .CreateBasicBlocks_label_201

	// *** Basic block 37

.CreateBasicBlocks_label_198:
	sd          s3, 72(s4)

	// *** Basic block 38

.CreateBasicBlocks_label_201:

	// *** Basic block 39

.CreateBasicBlocks_label_202:

	// *** Basic block 40

.CreateBasicBlocks_label_203:
	mv          a0, s4
	call        IRNext

	// *** Basic block 41

	mv          s4, a0
	beq         s4, x0, .CreateBasicBlocks_label_55

	// *** Basic block 42

.CreateBasicBlocks_label_211:
	mv          a0, s1
	call        GeneratorLastInstruction

	// *** Basic block 43

	sd          a0, 64(s3)
	mv          a0, s1
	call        GeneratorNewBasicBlock

	// *** Basic block 44

	sd          a0, 200(s1)
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
.func_end_CreateBasicBlocks:
	.size CreateBasicBlocks, .func_end_CreateBasicBlocks-CreateBasicBlocks

	.local  BuildBasicBlockGraph
	.type BuildBasicBlockGraph, @function

BuildBasicBlockGraph:

	// *** Basic block 0

	.global IRIsConditionalBranch
	.global IRNext
	.global BasicBlockAddEdge
	.global IRIsReturn
	.global IRIsCall
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
	mv          t0, a1
	mv          t1, a0
	mv          s1, x0
	ld          s2, 8(t0)
	bge         x0, s2, .BuildBasicBlockGraph_label_156

	// *** Basic block 1

	ld          s3, 0(t0)
	ld          s4, 200(t1)

	// *** Basic block 2

.BuildBasicBlockGraph_label_34:
	slli        t0, s1, 3
	add         t0, s3, t0
	ld          s3, 0(t0)
	ld          s5, 72(s3)
	mv          a0, s3
	call        IRIsConditionalBranch

	// *** Basic block 3

	beqz        a0, .BuildBasicBlockGraph_label_70

	// *** Basic block 4

	mv          a0, s3
	call        IRNext

	// *** Basic block 5

	mv          s6, a0
	ld          t0, 24(s3)
	ld          s7, 8(t0)
	ld          a1, 72(s6)
	mv          a0, s5
	call        BasicBlockAddEdge

	// *** Basic block 6

	ld          a1, 72(s7)
	mv          a0, s5
	call        BasicBlockAddEdge

	// *** Basic block 7

	j           .BuildBasicBlockGraph_label_151

	// *** Basic block 8

.BuildBasicBlockGraph_label_70:
	lw          t0, 20(s3)
	li          t1, 91		// 0x5b ASCII '['
	bne         t0, t1, .BuildBasicBlockGraph_label_109

	// *** Basic block 9

	mv          a0, s3
	call        IRNext

	// *** Basic block 10

	mv          s7, a0
	lw          t0, 20(s7)
	li          s8, 90		// 0x5a ASCII 'Z'
	bne         t0, s8, .BuildBasicBlockGraph_label_107

	// *** Basic block 11

.BuildBasicBlockGraph_label_90:
	ld          a1, 72(s7)
	mv          a0, s5
	call        BasicBlockAddEdge

	// *** Basic block 12

	mv          a0, s7
	call        IRNext

	// *** Basic block 13

	mv          s7, a0
	lw          t0, 20(s7)
	beq         t0, s8, .BuildBasicBlockGraph_label_90

	// *** Basic block 14

.BuildBasicBlockGraph_label_107:
	j           .BuildBasicBlockGraph_label_150

	// *** Basic block 15

.BuildBasicBlockGraph_label_109:
	mv          a0, s3
	call        IRIsReturn

	// *** Basic block 16

	beqz        a0, .BuildBasicBlockGraph_label_120

	// *** Basic block 17

	mv          a1, s4
	mv          a0, s5
	call        BasicBlockAddEdge

	// *** Basic block 18

	j           .BuildBasicBlockGraph_label_149

	// *** Basic block 19

.BuildBasicBlockGraph_label_120:
	mv          a0, s3
	call        IRIsCall

	// *** Basic block 20

	beqz        a0, .BuildBasicBlockGraph_label_137

	// *** Basic block 21

	mv          a0, s3
	call        IRNext

	// *** Basic block 22

	mv          s4, a0
	ld          a1, 72(s4)
	mv          a0, s5
	call        BasicBlockAddEdge

	// *** Basic block 23

	j           .BuildBasicBlockGraph_label_148

	// *** Basic block 24

.BuildBasicBlockGraph_label_137:
	ld          t0, 24(s3)
	ld          s3, 0(t0)
	ld          a1, 72(s3)
	mv          a0, s5
	call        BasicBlockAddEdge

	// *** Basic block 25

.BuildBasicBlockGraph_label_148:

	// *** Basic block 26

.BuildBasicBlockGraph_label_149:

	// *** Basic block 27

.BuildBasicBlockGraph_label_150:

	// *** Basic block 28

.BuildBasicBlockGraph_label_151:

	// *** Basic block 29

.BuildBasicBlockGraph_label_152:
	addi        s1, s1, 1
	bge         s1, s2, .BuildBasicBlockGraph_label_34

	// *** Basic block 30

.BuildBasicBlockGraph_label_156:
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
.func_end_BuildBasicBlockGraph:
	.size BuildBasicBlockGraph, .func_end_BuildBasicBlockGraph-BuildBasicBlockGraph

	.local  AddMissingLinks
	.type AddMissingLinks, @function

AddMissingLinks:

	// *** Basic block 0

	.global BasicBlockEndsInBranchReturnOrCall
	.global BasicBlockAddEdge
	.local FindBasicBlock
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
	addi        t0, s1, 168
	ld          s3, 8(t0)
	bge         x0, s3, .AddMissingLinks_label_69

	// *** Basic block 1

	ld          t0, 168(s1)
	ld          s4, 200(s1)

	// *** Basic block 2

.AddMissingLinks_label_26:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s5, 0(t0)
	beq         s5, s4, .AddMissingLinks_label_65

	// *** Basic block 3

.AddMissingLinks_label_35:
	mv          a0, s5
	call        BasicBlockEndsInBranchReturnOrCall

	// *** Basic block 4

	not         t0, a0
	beqz        t0, .AddMissingLinks_label_53

	// *** Basic block 5

	ld          t0, 0(s5)
	addi        a1, t0, 1
	mv          a0, s1
	call        FindBasicBlock

	// *** Basic block 6

	mv          a1, a0
	mv          a0, s5
	call        BasicBlockAddEdge

	// *** Basic block 7

.AddMissingLinks_label_53:
	addi        t0, s5, 32
	ld          t0, 8(t0)
	bnez        t0, .AddMissingLinks_label_64

	// *** Basic block 8

	mv          a1, s4
	mv          a0, s5
	call        BasicBlockAddEdge

	// *** Basic block 9

.AddMissingLinks_label_64:

	// *** Basic block 10

.AddMissingLinks_label_65:
	addi        s2, s2, 1
	bge         s2, s3, .AddMissingLinks_label_26

	// *** Basic block 11

.AddMissingLinks_label_69:
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
.func_end_AddMissingLinks:
	.size AddMissingLinks, .func_end_AddMissingLinks-AddMissingLinks

	.local  CalculateDominators
	.type CalculateDominators, @function

CalculateDominators:

	// *** Basic block 0

	.global BasicBlockInitDominators
	.global BasicBlockCalculateDominators
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
	addi        t0, s1, 168
	ld          s3, 8(t0)
	bge         x0, s3, .CalculateDominators_label_45

	// *** Basic block 1

	ld          s4, 168(s1)
	ld          s5, 192(s1)

	// *** Basic block 2

.CalculateDominators_label_25:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	sub         t0, s4, s5
	seqz        a1, t0
	mv          a2, s3
	mv          a0, s4
	call        BasicBlockInitDominators

	// *** Basic block 3

.CalculateDominators_label_41:
	addi        s2, s2, 1
	bge         s2, s3, .CalculateDominators_label_25

	// *** Basic block 4

.CalculateDominators_label_45:
	li          s3, 1		// 0x1 ASCII \x1

	// *** Basic block 5

.CalculateDominators_label_49:
	mv          s3, x0
	mv          s4, x0
	addi        t0, s1, 168
	ld          t0, 8(t0)
	bge         x0, t0, .CalculateDominators_label_79

	// *** Basic block 6

.CalculateDominators_label_58:
	ld          t0, 168(s1)
	slli        t1, s4, 3
	add         t0, t0, t1
	ld          s5, 0(t0)
	addi        a2, s1, 168
	mv          a1, s5
	mv          a0, s1
	call        BasicBlockCalculateDominators

	// *** Basic block 7

	or          s3, s3, a0

	// *** Basic block 8

.CalculateDominators_label_73:
	addi        s4, s4, 1
	ld          t0, 8(a2)
	bge         s4, t0, .CalculateDominators_label_58

	// *** Basic block 9

.CalculateDominators_label_79:
	bnez        s3, .CalculateDominators_label_49

	// *** Basic block 10

.CalculateDominators_label_81:
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
.func_end_CalculateDominators:
	.size CalculateDominators, .func_end_CalculateDominators-CalculateDominators

	.local  CalculateImmediateDominator
	.type CalculateImmediateDominator, @function

CalculateImmediateDominator:

	// *** Basic block 0

	.global BasicBlockCalculateImmediateDominator
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
	mv          s2, x0
	addi        t0, s1, 168
	ld          t0, 8(t0)
	bge         x0, t0, .CalculateImmediateDominator_label_39

	// *** Basic block 1

.CalculateImmediateDominator_label_19:
	ld          t0, 168(s1)
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s3, 0(t0)
	addi        a1, s1, 168
	mv          a0, s3
	call        BasicBlockCalculateImmediateDominator

	// *** Basic block 2

.CalculateImmediateDominator_label_33:
	addi        s2, s2, 1
	ld          t0, 8(a1)
	bge         s2, t0, .CalculateImmediateDominator_label_19

	// *** Basic block 3

.CalculateImmediateDominator_label_39:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CalculateImmediateDominator:
	.size CalculateImmediateDominator, .func_end_CalculateImmediateDominator-CalculateImmediateDominator

	.local  CalculateDominanceFrontier
	.type CalculateDominanceFrontier, @function

CalculateDominanceFrontier:

	// *** Basic block 0

	.global BasicBlockCalculateDominanceFrontier
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
	mv          s2, x0
	addi        t0, s1, 168
	ld          t0, 8(t0)
	bge         x0, t0, .CalculateDominanceFrontier_label_42

	// *** Basic block 1

.CalculateDominanceFrontier_label_19:
	ld          t0, 168(s1)
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s3, 0(t0)
	addi        a2, s1, 168
	mv          a1, s3
	mv          a0, s1
	call        BasicBlockCalculateDominanceFrontier

	// *** Basic block 2

.CalculateDominanceFrontier_label_36:
	addi        s2, s2, 1
	ld          t0, 8(a2)
	bge         s2, t0, .CalculateDominanceFrontier_label_19

	// *** Basic block 3

.CalculateDominanceFrontier_label_42:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CalculateDominanceFrontier:
	.size CalculateDominanceFrontier, .func_end_CalculateDominanceFrontier-CalculateDominanceFrontier

	.local  BuildDominatorTree
	.type BuildDominatorTree, @function

BuildDominatorTree:

	// *** Basic block 0

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
	mv          s1, x0
	addi        t0, a0, 168
	ld          s2, 8(t0)
	bge         x0, s2, .BuildDominatorTree_label_46

	// *** Basic block 1

	ld          t0, 168(a0)

	// *** Basic block 2

.BuildDominatorTree_label_22:
	slli        t1, s1, 3
	add         t0, t0, t1
	ld          s3, 0(t0)
	ld          t0, 136(s3)
	beq         t0, x0, .BuildDominatorTree_label_41

	// *** Basic block 3

	ld          t0, 136(s3)
	addi        a0, t0, 96
	ld          a1, 0(s3)
	call        VectorAppend

	// *** Basic block 4

.BuildDominatorTree_label_41:

	// *** Basic block 5

.BuildDominatorTree_label_42:
	addi        s1, s1, 1
	bge         s1, s2, .BuildDominatorTree_label_22

	// *** Basic block 6

.BuildDominatorTree_label_46:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BuildDominatorTree:
	.size BuildDominatorTree, .func_end_BuildDominatorTree-BuildDominatorTree

	.local  IncrementLoopNesting
	.type IncrementLoopNesting, @function

IncrementLoopNesting:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 240(a0)
	addi        t0, t0, 1
	sw          t0, 240(a0)
	ret         
.func_end_IncrementLoopNesting:
	.size IncrementLoopNesting, .func_end_IncrementLoopNesting-IncrementLoopNesting

	.local  DetectLoops
	.type DetectLoops, @function

DetectLoops:

	// *** Basic block 0

	.global BitSetContains
	.global VectorGet
	.global BasicBlockAddBackEdge
	.global BasicBlockTraverseDominatorTree
	.local IncrementLoopNesting
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
	addi        t0, s1, 168
	ld          t0, 8(t0)
	bge         x0, t0, .DetectLoops_label_97

	// *** Basic block 1

.DetectLoops_label_25:
	ld          t0, 168(s1)
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s3, 0(t0)
	mv          s4, x0
	addi        t0, s3, 32
	ld          t0, 8(t0)
	bge         x0, t0, .DetectLoops_label_89

	// *** Basic block 2

.DetectLoops_label_40:
	ld          t0, 32(s3)
	slli        t1, s4, 3
	add         t0, t0, t1
	ld          s5, 0(t0)
	addi        a0, s3, 72
	mv          a1, s5
	call        BitSetContains

	// *** Basic block 3

	beqz        a0, .DetectLoops_label_81

	// *** Basic block 4

	addi        a0, s1, 168
	mv          a1, s5
	call        VectorGet

	// *** Basic block 5

	mv          s5, a0
	mv          a1, s5
	mv          a0, s3
	call        BasicBlockAddBackEdge

	// *** Basic block 6

	mv          a4, x0
	mv          a3, x0
	la          t0, IncrementLoopNesting
	mv          a2, t0
	mv          a1, s5
	mv          a0, s1
	call        BasicBlockTraverseDominatorTree

	// *** Basic block 7

.DetectLoops_label_81:

	// *** Basic block 8

.DetectLoops_label_82:
	addi        s4, s4, 1
	addi        t0, s3, 32
	ld          t0, 8(t0)
	bge         s4, t0, .DetectLoops_label_40

	// *** Basic block 9

.DetectLoops_label_89:

	// *** Basic block 10

.DetectLoops_label_90:
	addi        s2, s2, 1
	addi        t0, s1, 168
	ld          t0, 8(t0)
	bge         s2, t0, .DetectLoops_label_25

	// *** Basic block 11

.DetectLoops_label_97:
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
.func_end_DetectLoops:
	.size DetectLoops, .func_end_DetectLoops-DetectLoops

	.local  CoalesceBlocks
	.type CoalesceBlocks, @function

CoalesceBlocks:

	// *** Basic block 0

	.global BasicBlockBegin
	.global BasicBlockIsEmpty
	.global BasicBlockEnd
	.global IRNext
	.global BasicBlockMoveInstructionBefore
	.global MapCopy
	.global SetCopy
	.global MapClear
	.global SetClear
	.global IRIsConditionalBranch
	.global printf
	.global abort
	.global VectorGet
	.global GeneratorEmitBefore
	.global NewIR
	.global NewIR1
	.global IRIsUnconditionalBranch
	.global IRIsReturn
	.global BasicBlockRemoveInstruction
	.global BasicBlockRemoveEdge
	.global VectorCopy
	.global BasicBlockAddEdge
	.global VectorDestruct
	addi sp, sp, -160
	// Saved return address (offset 152) and frame pointer (offset 144)
	sd ra, 152(sp)
	sd s0, 144(sp)
	addi s0, sp, 160
	// Local vars at offset -48(s0)
	// Spilled register region: 24 bytes at -72(s0) to -48(s0)
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
	mv          s1, a2
	mv          s2, a0
	mv          s3, a1
	mv          s4, x0
	ld          s5, 56(s1)
	mv          a0, s1
	call        BasicBlockBegin

	// *** Basic block 1

	ld          s6, 64(s3)
	mv          s7, a0
	mv          a0, s1
	call        BasicBlockIsEmpty

	// *** Basic block 2

	not         s8, a0
	beqz        s8, .CoalesceBlocks_label_84

	// *** Basic block 3

	mv          a0, s1
	call        BasicBlockEnd

	// *** Basic block 4

	sub         t0, s7, a0
	snez        s8, t0

	// *** Basic block 5

.CoalesceBlocks_label_84:
	beqz        s8, .CoalesceBlocks_label_114

	// *** Basic block 6

.CoalesceBlocks_label_86:
	mv          a0, s7
	call        IRNext

	// *** Basic block 7

	mv          s4, a0
	mv          s5, s7
	sd          s5, -56(s0)	// Spilled @65
	mv          a2, s6
	mv          a1, s7
	mv          a0, s2
	call        BasicBlockMoveInstructionBefore

	// *** Basic block 8

.CoalesceBlocks_label_99:
	mv          s7, s4
	mv          a0, s1
	call        BasicBlockIsEmpty

	// *** Basic block 9

	not         s6, a0
	beqz        s6, .CoalesceBlocks_label_112

	// *** Basic block 10

	mv          a0, s1
	call        BasicBlockEnd

	// *** Basic block 11

	sub         t0, s7, a0
	snez        s6, t0

	// *** Basic block 12

.CoalesceBlocks_label_112:
	beqz        s6, .CoalesceBlocks_label_86

	// *** Basic block 13

.CoalesceBlocks_label_114:
	addi        a0, s3, 160
	addi        a1, s1, 160
	call        MapCopy

	// *** Basic block 14

	addi        a0, s3, 192
	addi        a1, s1, 192
	call        SetCopy

	// *** Basic block 15

	addi        a0, s1, 160
	call        MapClear

	// *** Basic block 16

	addi        a0, s1, 192
	call        SetClear

	// *** Basic block 17

	lw          t0, 232(s1)
	lw          t1, 232(s3)
	add         t0, t1, t0
	sw          t0, 232(s3)
	lb          t0, 236(s1)
	beqz        t0, .CoalesceBlocks_label_143

	// *** Basic block 18

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 236(s3)

	// *** Basic block 19

.CoalesceBlocks_label_143:
	mv          a0, s5
	call        IRIsConditionalBranch

	// *** Basic block 20

	beqz        a0, .CoalesceBlocks_label_245

	// *** Basic block 21

	addi        t0, s1, 32
	ld          t0, 8(t0)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .CoalesceBlocks_label_157

	// *** Basic block 22

	j           .CoalesceBlocks_label_173

	// *** Basic block 23

.CoalesceBlocks_label_157:
	lla         a0, .str.6
	lla         a1, .str.7
	lla         a3, .str.8
	li          t0, 626		// 0x272
	mv          a2, t0
	call        printf

	// *** Basic block 24

	call        abort

	// *** Basic block 25

.CoalesceBlocks_label_173:
	ld          t0, 24(s5)
	ld          s6, 8(t0)
	addi        a0, s2, 168
	ld          s8, 32(s1)
	ld          a1, 0(s8)
	call        VectorGet

	// *** Basic block 26

	mv          s9, a0
	ld          t0, 72(s6)
	bne         t0, s9, .CoalesceBlocks_label_199

	// *** Basic block 27

	addi        a0, s2, 168
	ld          a1, 8(s8)
	call        VectorGet

	// *** Basic block 28

	mv          s9, a0

	// *** Basic block 29

.CoalesceBlocks_label_199:
	ld          s6, 56(s9)
	lw          t0, 20(s6)
	li          s8, 17		// 0x11 ASCII \x11
	beq         t0, s8, .CoalesceBlocks_label_224

	// *** Basic block 30

	mv          a0, s8
	call        NewIR

	// *** Basic block 31

	mv          a2, s6
	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmitBefore

	// *** Basic block 32

	mv          s6, a0
	sd          s6, -64(s0)	// Spilled @220
	sd          s6, 56(s9)

	// *** Basic block 33

.CoalesceBlocks_label_224:
	ld          a1, 56(s9)
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 34

	ld          a2, 64(s3)
	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmitBefore

	// *** Basic block 35

	mv          s8, a0
	sd          s8, -72(s0)	// Spilled @240
	sd          s3, 72(s8)
	j           .CoalesceBlocks_label_311

	// *** Basic block 36

.CoalesceBlocks_label_245:
	mv          a0, s5
	call        IRIsUnconditionalBranch

	// *** Basic block 37

	not         s10, a0
	beqz        s10, .CoalesceBlocks_label_256

	// *** Basic block 38

	mv          a0, s5
	call        IRIsReturn

	// *** Basic block 39

	not         s10, a0

	// *** Basic block 40

.CoalesceBlocks_label_256:
	beqz        s10, .CoalesceBlocks_label_310

	// *** Basic block 41

	addi        t0, s1, 32
	ld          t0, 8(t0)
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .CoalesceBlocks_label_267

	// *** Basic block 42

	j           .CoalesceBlocks_label_282

	// *** Basic block 43

.CoalesceBlocks_label_267:
	lla         a0, .str.9
	lla         a1, .str.10
	lla         a3, .str.11
	li          t0, 643		// 0x283
	mv          a2, t0
	call        printf

	// *** Basic block 44

	call        abort

	// *** Basic block 45

.CoalesceBlocks_label_282:
	addi        a0, s2, 168
	ld          t0, 32(s1)
	ld          a1, 0(t0)
	call        VectorGet

	// *** Basic block 46

	mv          s10, a0
	ld          a1, 56(s10)
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 47

	ld          a2, 64(s3)
	mv          a1, a0
	mv          a0, s2
	call        GeneratorEmitBefore

	// *** Basic block 48

	mv          s11, a0
	sd          s3, 72(s11)

	// *** Basic block 49

.CoalesceBlocks_label_310:

	// *** Basic block 50

.CoalesceBlocks_label_311:
	ld          a2, 64(s3)
	mv          a1, s3
	mv          a0, s2
	call        BasicBlockRemoveInstruction

	// *** Basic block 51

	mv          a1, s1
	mv          a0, s3
	call        BasicBlockRemoveEdge

	// *** Basic block 52

	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -48(s0)
	addi        a0, s0, -48
	addi        a1, s1, 32
	call        VectorCopy

	// *** Basic block 53

	ld          s5, -48(s0)
	mv          s6, x0
	addi        t0, s0, -48
	ld          s8, 8(t0)
	bge         x0, s8, .CoalesceBlocks_label_371

	// *** Basic block 54

.CoalesceBlocks_label_346:
	addi        a0, s2, 168
	slli        t0, s6, 3
	add         t0, s5, t0
	ld          a1, 0(t0)
	call        VectorGet

	// *** Basic block 55

	mv          s5, a0
	mv          a1, s5
	mv          a0, s1
	call        BasicBlockRemoveEdge

	// *** Basic block 56

	mv          a1, s5
	mv          a0, s3
	call        BasicBlockAddEdge

	// *** Basic block 57

.CoalesceBlocks_label_367:
	addi        s6, s6, 1
	bge         s6, s8, .CoalesceBlocks_label_346

	// *** Basic block 58

.CoalesceBlocks_label_371:
	addi        a0, s0, -48
	call        VectorDestruct

	// *** Basic block 59

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
.func_end_CoalesceBlocks:
	.size CoalesceBlocks, .func_end_CoalesceBlocks-CoalesceBlocks

	.local  StraightenGraph
	.type StraightenGraph, @function

StraightenGraph:

	// *** Basic block 0

	.global VectorGet
	.global printf
	.global abort
	.global IRIsUnconditionalBranch
	.global IRIsResult
	.local CoalesceBlocks
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
	mv          s2, x0
	addi        t0, s1, 168
	ld          t0, 8(t0)
	bge         x0, t0, .StraightenGraph_label_150

	// *** Basic block 1

.StraightenGraph_label_33:
	ld          t0, 168(s1)
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s3, 0(t0)
	addi        t0, s3, 8
	ld          t0, 8(t0)
	li          s4, 1		// 0x1 ASCII \x1
	bne         t0, s4, .StraightenGraph_label_142

	// *** Basic block 2

	addi        a0, s1, 168
	ld          t0, 8(s3)
	ld          a1, 0(t0)
	call        VectorGet

	// *** Basic block 3

	mv          s5, a0
	addi        t0, s5, 32
	ld          t0, 8(t0)
	bne         t0, s4, .StraightenGraph_label_141

	// *** Basic block 4

	addi        a0, s1, 168
	ld          t0, 32(s5)
	ld          a1, 0(t0)
	call        VectorGet

	// *** Basic block 5

	mv          s4, a0
	bne         s4, s3, .StraightenGraph_label_77

	// *** Basic block 6

	j           .StraightenGraph_label_94

	// *** Basic block 7

.StraightenGraph_label_77:
	lla         a0, .str.12
	lla         a1, .str.13
	lla         a3, .str.14
	li          t0, 680		// 0x2a8
	mv          a2, t0
	call        printf

	// *** Basic block 8

	call        abort

	// *** Basic block 9

.StraightenGraph_label_94:
	ld          s4, 64(s5)
	sub         t0, s4, x0
	snez        s6, t0
	beq         s4, x0, .StraightenGraph_label_107

	// *** Basic block 10

	mv          a0, s4
	call        IRIsUnconditionalBranch

	// *** Basic block 11

	mv          s6, a0

	// *** Basic block 12

.StraightenGraph_label_107:
	beqz        s6, .StraightenGraph_label_113

	// *** Basic block 13

	lw          t0, 88(s4)
	andi        t0, t0, 64
	seqz        s6, t0

	// *** Basic block 14

.StraightenGraph_label_113:
	beqz        s6, .StraightenGraph_label_131

	// *** Basic block 15

	ld          s4, 64(s3)
	lw          t0, 20(s4)
	addi        t1, t0, -17
	snez        s6, t1
	li          t1, 17		// 0x11 ASCII \x11
	beq         t0, t1, .StraightenGraph_label_130

	// *** Basic block 16

	mv          a0, s4
	call        IRIsResult

	// *** Basic block 17

	not         s6, a0

	// *** Basic block 18

.StraightenGraph_label_130:

	// *** Basic block 19

.StraightenGraph_label_131:
	beqz        s6, .StraightenGraph_label_140

	// *** Basic block 20

	mv          a2, s3
	mv          a1, s5
	mv          a0, s1
	call        CoalesceBlocks

	// *** Basic block 21

.StraightenGraph_label_140:

	// *** Basic block 22

.StraightenGraph_label_141:

	// *** Basic block 23

.StraightenGraph_label_142:

	// *** Basic block 24

.StraightenGraph_label_143:
	addi        s2, s2, 1
	addi        t0, s1, 168
	ld          t0, 8(t0)
	bge         s2, t0, .StraightenGraph_label_33

	// *** Basic block 25

.StraightenGraph_label_150:
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
.func_end_StraightenGraph:
	.size StraightenGraph, .func_end_StraightenGraph-StraightenGraph

	.local  BuildBasicBlocks
	.type BuildBasicBlocks, @function

BuildBasicBlocks:

	// *** Basic block 0

	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Local vars at offset -48(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	.global VectorInit
	.local CreateBasicBlocks
	.local BuildBasicBlockGraph
	.local AddMissingLinks
	.local StraightenGraph
	.local CalculateDominators
	.local CalculateImmediateDominator
	.local CalculateDominanceFrontier
	.local BuildDominatorTree
	.local DetectLoops
	.global VectorDestruct
	mv          s1, a0
	addi        a0, s0, -48
	call        VectorInit

	// *** Basic block 1

	addi        a1, s0, -48
	mv          a0, s1
	call        CreateBasicBlocks

	// *** Basic block 2

	addi        a1, s0, -48
	mv          a0, s1
	call        BuildBasicBlockGraph

	// *** Basic block 3

	mv          a0, s1
	call        AddMissingLinks

	// *** Basic block 4

	mv          a0, s1
	call        StraightenGraph

	// *** Basic block 5

	mv          a0, s1
	call        CalculateDominators

	// *** Basic block 6

	mv          a0, s1
	call        CalculateImmediateDominator

	// *** Basic block 7

	mv          a0, s1
	call        CalculateDominanceFrontier

	// *** Basic block 8

	mv          a0, s1
	call        BuildDominatorTree

	// *** Basic block 9

	mv          a0, s1
	call        DetectLoops

	// *** Basic block 10

	addi        a0, s0, -48
	call        VectorDestruct

	// *** Basic block 11

	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BuildBasicBlocks:
	.size BuildBasicBlocks, .func_end_BuildBasicBlocks-BuildBasicBlocks

	.local  RemoveUnreachableBlocks
	.type RemoveUnreachableBlocks, @function

RemoveUnreachableBlocks:

	// *** Basic block 0

	.global BasicBlockIsUnreachable
	.global BasicBlockClear
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
	addi        t0, s1, 168
	ld          s3, 8(t0)
	bge         x0, s3, .RemoveUnreachableBlocks_label_44

	// *** Basic block 1

	ld          s4, 168(s1)

	// *** Basic block 2

.RemoveUnreachableBlocks_label_21:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	mv          a1, s4
	mv          a0, s1
	call        BasicBlockIsUnreachable

	// *** Basic block 3

	beqz        a0, .RemoveUnreachableBlocks_label_39

	// *** Basic block 4

	mv          a1, s4
	mv          a0, s1
	call        BasicBlockClear

	// *** Basic block 5

.RemoveUnreachableBlocks_label_39:

	// *** Basic block 6

.RemoveUnreachableBlocks_label_40:
	addi        s2, s2, 1
	bge         s2, s3, .RemoveUnreachableBlocks_label_21

	// *** Basic block 7

.RemoveUnreachableBlocks_label_44:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_RemoveUnreachableBlocks:
	.size RemoveUnreachableBlocks, .func_end_RemoveUnreachableBlocks-RemoveUnreachableBlocks

	.global GeneratorNumCalls
	.type GeneratorNumCalls, @function

GeneratorNumCalls:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, x0
	mv          t1, x0
	addi        t2, a0, 168
	ld          t2, 8(t2)
	bge         x0, t2, .GeneratorNumCalls_label_35

	// *** Basic block 1

	ld          t3, 168(a0)

	// *** Basic block 2

.GeneratorNumCalls_label_22:
	slli        t4, t1, 3
	add         t3, t3, t4
	ld          t4, 0(t3)
	lw          t3, 232(t4)
	add         t0, t0, t3

	// *** Basic block 3

.GeneratorNumCalls_label_31:
	addi        t1, t1, 1
	bge         t1, t2, .GeneratorNumCalls_label_22

	// *** Basic block 4

.GeneratorNumCalls_label_35:
	mv          a0, t0

	// *** Basic block 5

.GeneratorNumCalls_label_38:
	ret         
.func_end_GeneratorNumCalls:
	.size GeneratorNumCalls, .func_end_GeneratorNumCalls-GeneratorNumCalls

	.local  VisitBlockForResultInstruction
	.type VisitBlockForResultInstruction, @function

VisitBlockForResultInstruction:

	// *** Basic block 0

	.global IRIsResult
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
	bne         s1, x0, .VisitBlockForResultInstruction_label_28

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.VisitBlockForResultInstruction_label_25:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.VisitBlockForResultInstruction_label_28:
	mv          a0, s1
	call        IRIsResult

	// *** Basic block 4

	beqz        a0, .VisitBlockForResultInstruction_label_42

	// *** Basic block 5

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 17(s2)
	sb          t0, 16(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .VisitBlockForResultInstruction_label_25

	// *** Basic block 6

.VisitBlockForResultInstruction_label_42:
	lw          t0, 20(s1)
	li          t1, 120		// 0x78 ASCII 'x'
	bne         t0, t1, .VisitBlockForResultInstruction_label_71

	// *** Basic block 7

	ld          t0, 24(s1)
	ld          s3, 0(t0)
	lw          t0, 20(s3)
	li          t1, 102		// 0x66 ASCII 'f'
	bne         t0, t1, .VisitBlockForResultInstruction_label_70

	// *** Basic block 8

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 17(s2)
	sb          t0, 16(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .VisitBlockForResultInstruction_label_25

	// *** Basic block 9

.VisitBlockForResultInstruction_label_70:

	// *** Basic block 10

.VisitBlockForResultInstruction_label_71:
	lw          t0, 88(s1)
	andi        t0, t0, 48
	beqz        t0, .VisitBlockForResultInstruction_label_86

	// *** Basic block 11

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 17(s2)
	sb          t0, 16(s2)
	li          a0, 1		// 0x1 ASCII \x1
	j           .VisitBlockForResultInstruction_label_25

	// *** Basic block 12

.VisitBlockForResultInstruction_label_86:
	mv          a0, x0
	j           .VisitBlockForResultInstruction_label_25
.func_end_VisitBlockForResultInstruction:
	.size VisitBlockForResultInstruction, .func_end_VisitBlockForResultInstruction-VisitBlockForResultInstruction

	.local  VisitBlockForResult
	.type VisitBlockForResult, @function

VisitBlockForResult:

	// *** Basic block 0

	.global BitSetContains
	.global BitSetInsert
	.global IRIsReturn
	.global BasicBlockBegin
	.global BasicBlockIsEmpty
	.global BasicBlockEnd
	.local VisitBlockForResultInstruction
	.global IRNext
	.global BitSetCopy
	.global VectorGet
	.local VisitBlockForResult
	.global BitSetDestruct
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
	mv          s1, a1
	mv          s2, a2
	mv          s3, a0
	bne         s1, x0, .VisitBlockForResult_label_41

	// *** Basic block 1

.VisitBlockForResult_label_38:
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

	// *** Basic block 2

.VisitBlockForResult_label_41:
	lb          t0, 16(s2)
	beqz        t0, .VisitBlockForResult_label_46

	// *** Basic block 3

	j           .VisitBlockForResult_label_38

	// *** Basic block 4

.VisitBlockForResult_label_46:
	ld          s4, 0(s1)
	mv          a1, s4
	mv          a0, s2
	call        BitSetContains

	// *** Basic block 5

	beqz        a0, .VisitBlockForResult_label_55

	// *** Basic block 6

	j           .VisitBlockForResult_label_38

	// *** Basic block 7

.VisitBlockForResult_label_55:
	mv          a1, s4
	mv          a0, s2
	call        BitSetInsert

	// *** Basic block 8

	ld          s4, 64(s1)
	bne         s4, x0, .VisitBlockForResult_label_68

	// *** Basic block 9

	j           .VisitBlockForResult_label_38

	// *** Basic block 10

.VisitBlockForResult_label_68:
	mv          a0, s4
	call        IRIsReturn

	// *** Basic block 11

	beqz        a0, .VisitBlockForResult_label_77

	// *** Basic block 12

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 16(s2)
	j           .VisitBlockForResult_label_38

	// *** Basic block 13

.VisitBlockForResult_label_77:
	mv          a0, s1
	call        BasicBlockBegin

	// *** Basic block 14

	mv          s4, a0
	mv          a0, s1
	call        BasicBlockIsEmpty

	// *** Basic block 15

	not         s5, a0
	beqz        s5, .VisitBlockForResult_label_94

	// *** Basic block 16

	mv          a0, s1
	call        BasicBlockEnd

	// *** Basic block 17

	sub         t0, s4, a0
	snez        s5, t0

	// *** Basic block 18

.VisitBlockForResult_label_94:
	beqz        s5, .VisitBlockForResult_label_123

	// *** Basic block 19

.VisitBlockForResult_label_96:
	mv          a1, s2
	mv          a0, s4
	call        VisitBlockForResultInstruction

	// *** Basic block 20

	beqz        a0, .VisitBlockForResult_label_104

	// *** Basic block 21

	j           .VisitBlockForResult_label_38

	// *** Basic block 22

.VisitBlockForResult_label_104:

	// *** Basic block 23

.VisitBlockForResult_label_105:
	mv          a0, s4
	call        IRNext

	// *** Basic block 24

	mv          s4, a0
	mv          a0, s1
	call        BasicBlockIsEmpty

	// *** Basic block 25

	not         s5, a0
	beqz        s5, .VisitBlockForResult_label_121

	// *** Basic block 26

	mv          a0, s1
	call        BasicBlockEnd

	// *** Basic block 27

	sub         t0, s4, a0
	snez        s5, t0

	// *** Basic block 28

.VisitBlockForResult_label_121:
	beqz        s5, .VisitBlockForResult_label_96

	// *** Basic block 29

.VisitBlockForResult_label_123:
	ld          t0, 200(s3)
	bne         s1, t0, .VisitBlockForResult_label_133

	// *** Basic block 30

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 16(s2)
	j           .VisitBlockForResult_label_38

	// *** Basic block 31

.VisitBlockForResult_label_133:
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -48(s0)
	addi        t0, s0, -48
	lb          t1, 16(s2)
	sb          t1, 16(t0)
	addi        t0, s0, -48
	lb          t1, 17(s2)
	sb          t1, 17(t0)
	addi        a0, s0, -48
	mv          a1, s2
	call        BitSetCopy

	// *** Basic block 32

	mv          s5, x0
	addi        t0, s1, 32
	ld          s6, 8(t0)
	bge         x0, s6, .VisitBlockForResult_label_213

	// *** Basic block 33

	ld          s7, 32(s1)

	// *** Basic block 34

.VisitBlockForResult_label_165:
	addi        a1, s0, -48
	mv          a0, s2
	call        BitSetCopy

	// *** Basic block 35

	addi        t0, s0, -48
	lb          t0, 16(t0)
	sb          t0, 16(s2)
	addi        t0, s0, -48
	lb          t0, 17(t0)
	sb          t0, 17(s2)
	slli        t0, s5, 3
	add         t0, s7, t0
	ld          s7, 0(t0)
	addi        a0, s3, 168
	mv          a1, s7
	call        VectorGet

	// *** Basic block 36

	mv          s7, a0
	mv          a2, s2
	mv          a1, s7
	mv          a0, s3
	call        VisitBlockForResult

	// *** Basic block 37

	lb          t0, 16(s2)
	beqz        t0, .VisitBlockForResult_label_206

	// *** Basic block 38

	lb          t1, 17(s2)
	not         t0, t1

	// *** Basic block 39

.VisitBlockForResult_label_206:
	bnez        t0, .VisitBlockForResult_label_213

	// *** Basic block 40

.VisitBlockForResult_label_208:

	// *** Basic block 41

.VisitBlockForResult_label_209:
	addi        s5, s5, 1
	bge         s5, s6, .VisitBlockForResult_label_165

	// *** Basic block 42

.VisitBlockForResult_label_213:
	addi        a0, s0, -48
	call        BitSetDestruct

	// *** Basic block 43

	j           .VisitBlockForResult_label_38
.func_end_VisitBlockForResult:
	.size VisitBlockForResult, .func_end_VisitBlockForResult-VisitBlockForResult

	.local  CheckReturn
	.type CheckReturn, @function

CheckReturn:

	// *** Basic block 0

	.global TypeIsVoidFunction
	.global StringEqual
	.local VisitBlockForResult
	.global SyntaxError
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Local vars at offset -48(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	ld          t0, 8(s1)
	ld          t0, 32(t0)
	ld          a0, 40(t0)
	call        TypeIsVoidFunction

	// *** Basic block 1

	beqz        a0, .CheckReturn_label_30

	// *** Basic block 2

.CheckReturn_label_27:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.CheckReturn_label_30:
	ld          t0, 8(s1)
	ld          a0, 32(t0)
	lla         a1, .str.15
	call        StringEqual

	// *** Basic block 4

	beqz        a0, .CheckReturn_label_43

	// *** Basic block 5

	j           .CheckReturn_label_27

	// *** Basic block 6

.CheckReturn_label_43:
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -48(s0)
	addi        t0, s0, -48
	sb          x0, 16(t0)
	addi        t0, s0, -48
	sb          x0, 17(t0)
	ld          t0, 168(s1)
	ld          a1, 0(t0)
	addi        a2, s0, -48
	mv          a0, s1
	call        VisitBlockForResult

	// *** Basic block 7

	addi        t1, s0, -48
	lb          t0, 16(t1)
	beqz        t0, .CheckReturn_label_77

	// *** Basic block 8

	addi        t1, s0, -48
	lb          t1, 17(t1)
	not         t0, t1

	// *** Basic block 9

.CheckReturn_label_77:
	beqz        t0, .CheckReturn_label_88

	// *** Basic block 10

	ld          a0, 0(s1)
	lla         a1, .str.16
	ld          a2, 16(a0)
	call        SyntaxError

	// *** Basic block 11

.CheckReturn_label_88:
	j           .CheckReturn_label_27
.func_end_CheckReturn:
	.size CheckReturn, .func_end_CheckReturn-CheckReturn

	.local  DetectUninitializedVars
	.type DetectUninitializedVars, @function

DetectUninitializedVars:

	// *** Basic block 0

	.global BasicBlockBegin
	.global BasicBlockIsEmpty
	.global BasicBlockEnd
	.global TypeIsStructOrUnion
	.global IRIsVarDef
	.global DecodeSourceLocation
	.global ReportWarning
	.global IRNext
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -48(s0)
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
	ld          s2, 192(s1)
	mv          a0, s2
	call        BasicBlockBegin

	// *** Basic block 1

	ld          t0, 8(s1)
	ld          t0, 32(t0)
	ld          s3, 16(t0)
	mv          s4, a0
	mv          a0, s2
	call        BasicBlockIsEmpty

	// *** Basic block 2

	not         s5, a0
	beqz        s5, .DetectUninitializedVars_label_55

	// *** Basic block 3

	mv          a0, s2
	call        BasicBlockEnd

	// *** Basic block 4

	sub         t0, s4, a0
	snez        s5, t0

	// *** Basic block 5

.DetectUninitializedVars_label_55:
	beqz        s5, .DetectUninitializedVars_label_176

	// *** Basic block 6

.DetectUninitializedVars_label_57:
	lw          t0, 20(s4)
	li          t1, 96		// 0x60 ASCII '`'
	bne         t0, t1, .DetectUninitializedVars_label_157

	// *** Basic block 7

	addi        t0, s4, 48
	ld          s1, 8(t0)
	bge         x0, s1, .DetectUninitializedVars_label_156

	// *** Basic block 8

	ld          s5, 48(s4)
	mv          s6, s4
	ld          s8, 136(s6)
	ld          s9, 40(s8)
	lw          t0, 16(s9)
	addi        t0, t0, -2
	seqz        s9, t0

	// *** Basic block 9

.DetectUninitializedVars_label_85:
	mv          s7, s9
	bnez        s9, .DetectUninitializedVars_label_96

	// *** Basic block 10

	ld          s6, 16(s8)
	j           .DetectUninitializedVars_label_91

	// *** Basic block 11

.DetectUninitializedVars_label_91:
	mv          a0, s9
	call        TypeIsStructOrUnion

	// *** Basic block 12

	mv          s7, a0

	// *** Basic block 13

.DetectUninitializedVars_label_96:
	bnez        s7, .DetectUninitializedVars_label_158

	// *** Basic block 14

.DetectUninitializedVars_label_98:
	mv          s6, x0
	bge         x0, s1, .DetectUninitializedVars_label_155

	// *** Basic block 15

.DetectUninitializedVars_label_103:
	slli        t0, s6, 3
	add         t0, s5, t0
	ld          s5, 0(t0)
	mv          a0, s5
	call        IRIsVarDef

	// *** Basic block 16

	bnez        a0, .DetectUninitializedVars_label_151

	// *** Basic block 17

.DetectUninitializedVars_label_113:
	ld          a0, 128(s5)
	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	call        DecodeSourceLocation

	// *** Basic block 18

	ld          a0, -48(s0)
	lw          a1, -40(s0)
	lla         a2, .str.17
	lla         a3, .str.18
	mv          a5, s3
	mv          a4, s6
	call        ReportWarning

	// *** Basic block 19

.DetectUninitializedVars_label_151:
	addi        s6, s6, 1
	bge         s6, s1, .DetectUninitializedVars_label_103

	// *** Basic block 20

.DetectUninitializedVars_label_155:

	// *** Basic block 21

.DetectUninitializedVars_label_156:

	// *** Basic block 22

.DetectUninitializedVars_label_157:

	// *** Basic block 23

.DetectUninitializedVars_label_158:
	mv          a0, s4
	call        IRNext

	// *** Basic block 24

	mv          s4, a0
	mv          a0, s2
	call        BasicBlockIsEmpty

	// *** Basic block 25

	not         s1, a0
	beqz        s1, .DetectUninitializedVars_label_174

	// *** Basic block 26

	mv          a0, s2
	call        BasicBlockEnd

	// *** Basic block 27

	sub         t0, s4, a0
	snez        s1, t0

	// *** Basic block 28

.DetectUninitializedVars_label_174:
	beqz        s1, .DetectUninitializedVars_label_57

	// *** Basic block 29

.DetectUninitializedVars_label_176:
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
.func_end_DetectUninitializedVars:
	.size DetectUninitializedVars, .func_end_DetectUninitializedVars-DetectUninitializedVars

	.local  GenerateVLASizeExpressions
	.type GenerateVLASizeExpressions, @function

GenerateVLASizeExpressions:

	// *** Basic block 0

	.global GenerateVLASize
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
	mv          t0, a1
	mv          s1, a0
	mv          s2, x0
	ld          s3, 8(t0)
	bge         x0, s3, .GenerateVLASizeExpressions_label_72

	// *** Basic block 1

	ld          t1, 0(t0)

	// *** Basic block 2

.GenerateVLASizeExpressions_label_28:
	slli        t0, s2, 3
	add         t0, t1, t0
	ld          t1, 0(t0)
	ld          s4, 40(t1)
	lw          s5, 16(s4)
	addi        t1, s5, -2
	seqz        t0, t1
	li          t1, 2		// 0x2 ASCII \x2
	beq         s5, t1, .GenerateVLASizeExpressions_label_49

	// *** Basic block 3

	addi        t1, s5, -1
	seqz        t0, t1

	// *** Basic block 4

.GenerateVLASizeExpressions_label_49:
	beqz        t0, .GenerateVLASizeExpressions_label_56

	// *** Basic block 5

	addi        t1, s4, 32
	lb          t1, 16(t1)
	slli        t1, t1, 61
	srai        t0, t1, 63

	// *** Basic block 6

.GenerateVLASizeExpressions_label_56:

	// *** Basic block 7

.GenerateVLASizeExpressions_label_58:
	beqz        t0, .GenerateVLASizeExpressions_label_67

	// *** Basic block 8

	j           .GenerateVLASizeExpressions_label_61

	// *** Basic block 9

.GenerateVLASizeExpressions_label_61:
	mv          a1, s4
	mv          a0, s1
	call        GenerateVLASize

	// *** Basic block 10

.GenerateVLASizeExpressions_label_67:

	// *** Basic block 11

.GenerateVLASizeExpressions_label_68:
	addi        s2, s2, 1
	bge         s2, s3, .GenerateVLASizeExpressions_label_28

	// *** Basic block 12

.GenerateVLASizeExpressions_label_72:
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
.func_end_GenerateVLASizeExpressions:
	.size GenerateVLASizeExpressions, .func_end_GenerateVLASizeExpressions-GenerateVLASizeExpressions

	.global GenerateFunction
	.type GenerateFunction, @function

GenerateFunction:

	// *** Basic block 0

	.global GeneratorEmit
	.global NewIR
	.global TypeIsStructOrUnion
	.global IRSetType
	.local TrapFunctionBeforeCodegen
	.local GenerateVLASizeExpressions
	.global GenerateStatement
	.global GeneratorGetReturnLabel
	.global NewIR1
	.global compiler
	.global stdout
	.global GeneratorPrintIR
	.local BuildBasicBlocks
	.global fprintf
	.local PrintBasicBlocks
	.local CheckReturn
	.local RemoveUnreachableBlocks
	.global GeneratorConvertToSSA
	.local DetectUninitializedVars
	.global OptLevel2
	.global StrengthReductionOptimization
	.global GlobalValueNumberingOptimization
	.global ConstantPropagationOptimization
	.global CodeMotionOptimization
	.global GeneratorRemoveSSA
	.global TailCallOptimization
	.local TrapFunctionAfterCodegen
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
	ld          s2, 8(s1)
	addi        t0, s2, 32
	ld          s3, 40(t0)
	ld          t0, 56(s3)
	ld          t0, 8(t0)
	bnez        t0, .GenerateFunction_label_77

	// *** Basic block 1

	li          t0, 93		// 0x5d ASCII ']'
	mv          a0, t0
	call        NewIR

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 3

	j           .GenerateFunction_label_170

	// *** Basic block 4

.GenerateFunction_label_77:
	li          t0, 94		// 0x5e ASCII '^'
	mv          a0, t0
	call        NewIR

	// *** Basic block 5

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 6

	ld          a0, 24(s2)
	call        TypeIsStructOrUnion

	// *** Basic block 7

	beqz        a0, .GenerateFunction_label_111

	// *** Basic block 8

	li          t0, 102		// 0x66 ASCII 'f'
	mv          a0, t0
	call        NewIR

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 10

	sd          a0, 72(s1)
	ld          a0, 72(s1)
	ld          t0, 8(s1)
	ld          a1, 24(t0)
	call        IRSetType

	// *** Basic block 11

.GenerateFunction_label_111:
	mv          a0, s1
	call        TrapFunctionBeforeCodegen

	// *** Basic block 12

	ld          t0, 8(s1)
	addi        t0, t0, 32
	addi        a1, t0, 8
	mv          a0, s1
	call        GenerateVLASizeExpressions

	// *** Basic block 13

	mv          a1, s3
	mv          a0, s1
	call        GenerateStatement

	// *** Basic block 14

	ld          t0, 88(s1)
	bne         t0, x0, .GenerateFunction_label_152

	// *** Basic block 15

	li          t0, 95		// 0x5f ASCII '_'
	mv          a0, t0
	call        NewIR

	// *** Basic block 16

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 17

	li          t0, 93		// 0x5d ASCII ']'
	mv          a0, t0
	call        NewIR

	// *** Basic block 18

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 19

	j           .GenerateFunction_label_169

	// *** Basic block 20

.GenerateFunction_label_152:
	mv          a0, s1
	call        GeneratorGetReturnLabel

	// *** Basic block 21

	mv          s2, a0
	mv          a1, s2
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a0, t0
	call        NewIR1

	// *** Basic block 22

	mv          a1, a0
	mv          a0, s1
	call        GeneratorEmit

	// *** Basic block 23

.GenerateFunction_label_169:

	// *** Basic block 24

.GenerateFunction_label_170:
	la          t1, compiler
	ld          s3, 0(t1)
	lb          t0, 1232(s3)
	bnez        t0, .GenerateFunction_label_183

	// *** Basic block 25

	ld          t1, 1240(s3)
	la          t2, stdout
	ld          t2, 0(t2)
	sub         t1, t1, t2
	snez        t0, t1

	// *** Basic block 26

.GenerateFunction_label_183:
	beqz        t0, .GenerateFunction_label_191

	// *** Basic block 27

	ld          a1, 1240(s3)
	mv          a0, s1
	call        GeneratorPrintIR

	// *** Basic block 28

.GenerateFunction_label_191:
	mv          a0, s1
	call        BuildBasicBlocks

	// *** Basic block 29

	mv          t1, t0
	bnez        t0, .GenerateFunction_label_204

	// *** Basic block 30

	ld          t2, 1240(s3)
	la          t3, stdout
	ld          t3, 0(t3)
	sub         t2, t2, t3
	snez        t1, t2

	// *** Basic block 31

.GenerateFunction_label_204:
	beqz        t1, .GenerateFunction_label_219

	// *** Basic block 32

	ld          s4, 1240(s3)
	lla         a1, .str.19
	mv          a0, s4
	call        fprintf

	// *** Basic block 33

	mv          a1, s4
	mv          a0, s1
	call        PrintBasicBlocks

	// *** Basic block 34

.GenerateFunction_label_219:
	mv          a0, s1
	call        CheckReturn

	// *** Basic block 35

	mv          a0, s1
	call        RemoveUnreachableBlocks

	// *** Basic block 36

	mv          a0, s1
	call        GeneratorConvertToSSA

	// *** Basic block 37

	mv          t1, t0
	bnez        t0, .GenerateFunction_label_238

	// *** Basic block 38

	ld          t2, 1240(s3)
	la          t3, stdout
	ld          t3, 0(t3)
	sub         t2, t2, t3
	snez        t1, t2

	// *** Basic block 39

.GenerateFunction_label_238:
	beqz        t1, .GenerateFunction_label_253

	// *** Basic block 40

	ld          s4, 1240(s3)
	lla         a1, .str.20
	mv          a0, s4
	call        fprintf

	// *** Basic block 41

	mv          a1, s4
	mv          a0, s1
	call        PrintBasicBlocks

	// *** Basic block 42

.GenerateFunction_label_253:
	mv          a0, s1
	call        DetectUninitializedVars

	// *** Basic block 43

	call        OptLevel2

	// *** Basic block 44

	beqz        a0, .GenerateFunction_label_271

	// *** Basic block 45

	mv          a0, s1
	call        StrengthReductionOptimization

	// *** Basic block 46

	mv          a0, s1
	call        GlobalValueNumberingOptimization

	// *** Basic block 47

	mv          a0, s1
	call        ConstantPropagationOptimization

	// *** Basic block 48

	mv          a0, s1
	call        CodeMotionOptimization

	// *** Basic block 49

.GenerateFunction_label_271:
	mv          a0, s1
	call        GeneratorRemoveSSA

	// *** Basic block 50

	mv          a0, s1
	call        RemoveUnreachableBlocks

	// *** Basic block 51

	call        OptLevel2

	// *** Basic block 52

	beqz        a0, .GenerateFunction_label_283

	// *** Basic block 53

	mv          a0, s1
	call        TailCallOptimization

	// *** Basic block 54

.GenerateFunction_label_283:
	beqz        t0, .GenerateFunction_label_292

	// *** Basic block 55

	ld          a0, 1240(s3)
	lla         a1, .str.21
	call        fprintf

	// *** Basic block 56

.GenerateFunction_label_292:
	mv          t1, t0
	bnez        t0, .GenerateFunction_label_302

	// *** Basic block 57

	ld          t0, 1240(s3)
	la          t2, stdout
	ld          t2, 0(t2)
	sub         t0, t0, t2
	snez        t1, t0

	// *** Basic block 58

.GenerateFunction_label_302:
	beqz        t1, .GenerateFunction_label_310

	// *** Basic block 59

	ld          a1, 1240(s3)
	mv          a0, s1
	call        PrintBasicBlocks

	// *** Basic block 60

.GenerateFunction_label_310:
	mv          a0, s1
	call        TrapFunctionAfterCodegen

	// *** Basic block 61

	ld          t0, 1112(s3)
	ld          t0, 48(t0)
	mv          a0, s1
	jalr         x1, t0, 0

	// *** Basic block 62

	mv          s3, a0
	mv          a0, s3

	// *** Basic block 63

.GenerateFunction_label_325:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GenerateFunction:
	.size GenerateFunction, .func_end_GenerateFunction-GenerateFunction

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "InitializeInstructions"
	.type .str.1, @object
	.size .str.1, 23

.str.2:
	.asciz "foobar"
	.type .str.2, @object
	.size .str.2, 7

.str.3:
	.asciz "-"
	.type .str.3, @object
	.size .str.3, 2

.str.4:
	.asciz "\n"
	.type .str.4, @object
	.size .str.4, 2

.str.5:
	.asciz "**** IR for function %s\n\n"
	.type .str.5, @object
	.size .str.5, 26

.str.6:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.6, @object
	.size .str.6, 30

.str.7:
	.asciz "codegen.c"
	.type .str.7, @object
	.size .str.7, 10

.str.8:
	.asciz "src->out_edges.length == 2"
	.type .str.8, @object
	.size .str.8, 27

.str.9:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.9, @object
	.size .str.9, 30

.str.10:
	.asciz "codegen.c"
	.type .str.10, @object
	.size .str.10, 10

.str.11:
	.asciz "src->out_edges.length == 1"
	.type .str.11, @object
	.size .str.11, 27

.str.12:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.12, @object
	.size .str.12, 30

.str.13:
	.asciz "codegen.c"
	.type .str.13, @object
	.size .str.13, 10

.str.14:
	.asciz "out == block"
	.type .str.14, @object
	.size .str.14, 13

.str.15:
	.asciz "main"
	.type .str.15, @object
	.size .str.15, 5

.str.16:
	.asciz "Control reaches the end of non-void function \'%s\'"
	.type .str.16, @object
	.size .str.16, 50

.str.17:
	.asciz "uninit-var"
	.type .str.17, @object
	.size .str.17, 11

.str.18:
	.asciz "Variable \'%s\' is used uninitialized here in function \'%s\'"
	.type .str.18, @object
	.size .str.18, 58

.str.19:
	.asciz "Before SSA conversion\n"
	.type .str.19, @object
	.size .str.19, 23

.str.20:
	.asciz "After SSA conversion\n"
	.type .str.20, @object
	.size .str.20, 22

.str.21:
	.asciz "After SSA has been removed\n"
	.type .str.21, @object
	.size .str.21, 28

