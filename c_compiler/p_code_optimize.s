	.file   "p_code_optimize.c"
	.text
	.option pic
.PCbegin:
	.local  RemoveUnusedExpressions
	.type RemoveUnusedExpressions, @function

RemoveUnusedExpressions:

	// *** Basic block 0

	.global TargetLastInstruction
	.global TargetPrev
	.global printf
	.global abort
	.global PCodeIsExpression
	.global TargetDeleteInstruction
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
	call        TargetLastInstruction

	// *** Basic block 1

	mv          s2, a0
	beq         s2, x0, .RemoveUnusedExpressions_label_158

	// *** Basic block 2

.RemoveUnusedExpressions_label_36:
	mv          a0, s2
	call        TargetPrev

	// *** Basic block 3

	mv          s3, a0
	beq         s2, s3, .RemoveUnusedExpressions_label_46

	// *** Basic block 4

	j           .RemoveUnusedExpressions_label_64

	// *** Basic block 5

.RemoveUnusedExpressions_label_46:
	lla         a0, .str.1
	lla         a1, .str.2
	lla         a3, .str.3
	li          t0, 24		// 0x18 ASCII \x18
	mv          a2, t0
	call        printf

	// *** Basic block 6

	call        abort

	// *** Basic block 7

.RemoveUnusedExpressions_label_64:
	lw          s4, 16(s2)
	mv          a0, s4
	call        PCodeIsExpression

	// *** Basic block 8

	beqz        a0, .RemoveUnusedExpressions_label_83

	// *** Basic block 9

	addi        t0, s2, 64
	ld          t0, 8(t0)
	bnez        t0, .RemoveUnusedExpressions_label_81

	// *** Basic block 10

	mv          a1, s2
	mv          a0, s1
	call        TargetDeleteInstruction

	// *** Basic block 11

.RemoveUnusedExpressions_label_81:
	j           .RemoveUnusedExpressions_label_153

	// *** Basic block 12

.RemoveUnusedExpressions_label_83:
	li          t0, 18		// 0x12 ASCII \x12
	bne         s4, t0, .RemoveUnusedExpressions_label_126

	// *** Basic block 13

	ld          s5, 40(s2)
	addi        t1, s2, 64
	ld          t1, 8(t1)
	seqz        t0, t1
	bnez        t1, .RemoveUnusedExpressions_label_102

	// *** Basic block 14

	sub         t1, s5, x0
	snez        t0, t1

	// *** Basic block 15

.RemoveUnusedExpressions_label_102:
	beqz        t0, .RemoveUnusedExpressions_label_109

	// *** Basic block 16

	lw          t1, 16(s5)
	addi        t1, t1, -4
	seqz        t0, t1

	// *** Basic block 17

.RemoveUnusedExpressions_label_109:
	beqz        t0, .RemoveUnusedExpressions_label_117

	// *** Basic block 18

	addi        t1, s5, 64
	ld          t1, 8(t1)
	addi        t1, t1, -1
	seqz        t0, t1

	// *** Basic block 19

.RemoveUnusedExpressions_label_117:
	beqz        t0, .RemoveUnusedExpressions_label_124

	// *** Basic block 20

	mv          a1, s2
	mv          a0, s1
	call        TargetDeleteInstruction

	// *** Basic block 21

.RemoveUnusedExpressions_label_124:
	j           .RemoveUnusedExpressions_label_152

	// *** Basic block 22

.RemoveUnusedExpressions_label_126:
	addi        t1, s4, -36
	seqz        t0, t1
	li          t1, 36		// 0x24 ASCII '$'
	beq         s4, t1, .RemoveUnusedExpressions_label_136

	// *** Basic block 23

	addi        t1, s4, -37
	seqz        t0, t1

	// *** Basic block 24

.RemoveUnusedExpressions_label_136:
	beqz        t0, .RemoveUnusedExpressions_label_151

	// *** Basic block 25

	ld          s4, 40(s2)
	ld          t0, 120(s4)
	bnez        t0, .RemoveUnusedExpressions_label_150

	// *** Basic block 26

	mv          a1, s2
	mv          a0, s1
	call        TargetDeleteInstruction

	// *** Basic block 27

.RemoveUnusedExpressions_label_150:

	// *** Basic block 28

.RemoveUnusedExpressions_label_151:

	// *** Basic block 29

.RemoveUnusedExpressions_label_152:

	// *** Basic block 30

.RemoveUnusedExpressions_label_153:
	mv          s2, s3
	bne         s2, x0, .RemoveUnusedExpressions_label_36

	// *** Basic block 31

.RemoveUnusedExpressions_label_158:
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
.func_end_RemoveUnusedExpressions:
	.size RemoveUnusedExpressions, .func_end_RemoveUnusedExpressions-RemoveUnusedExpressions

	.global PCodeOptimize
	.type PCodeOptimize, @function

PCodeOptimize:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local RemoveUnusedExpressions
	j           RemoveUnusedExpressions
.func_end_PCodeOptimize:
	.size PCodeOptimize, .func_end_PCodeOptimize-PCodeOptimize

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
	.asciz "inst != prev"
	.type .str.3, @object
	.size .str.3, 13

