	.file   "syntax.c"
	.text
	.option pic
.PCbegin:
	.global SyntaxInit
	.type SyntaxInit, @function

SyntaxInit:

	// *** Basic block 0

	.global VectorInit
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
	sd          x0, 8(s1)
	sd          a1, 0(s1)
	sd          x0, 16(s1)
	sd          x0, 24(s1)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 64(s1)
	sb          x0, 68(s1)
	sw          x0, 72(s1)
	sw          x0, 76(s1)
	addi        a0, s1, 80
	call        VectorInit

	// *** Basic block 1

	addi        a0, s1, 104
	call        VectorInit

	// *** Basic block 2

	addi        a0, s1, 128
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorInit
.func_end_SyntaxInit:
	.size SyntaxInit, .func_end_SyntaxInit-SyntaxInit

	.global SyntaxDestruct
	.type SyntaxDestruct, @function

SyntaxDestruct:

	// *** Basic block 0

	.global VectorDestructWithContents
	.global SymbolDestruct
	.global VectorDestruct
	.global ASTNodeDelete
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
	addi        a0, s1, 80
	la          t0, SymbolDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 1

	addi        a0, s1, 104
	call        VectorDestruct

	// *** Basic block 2

	ld          a0, 8(s1)
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeDelete
.func_end_SyntaxDestruct:
	.size SyntaxDestruct, .func_end_SyntaxDestruct-SyntaxDestruct

	.global SyntaxResetForNewDeclaration
	.type SyntaxResetForNewDeclaration, @function

SyntaxResetForNewDeclaration:

	// *** Basic block 0

	.global VectorCopy
	.global VectorDestruct
	.global ASTNodeDelete
	.global VectorInit
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
	addi        a0, s1, 128
	addi        a1, s1, 80
	call        VectorCopy

	// *** Basic block 1

	addi        a0, s1, 80
	call        VectorDestruct

	// *** Basic block 2

	addi        a0, s1, 104
	call        VectorDestruct

	// *** Basic block 3

	ld          a0, 8(s1)
	call        ASTNodeDelete

	// *** Basic block 4

	sd          x0, 8(s1)
	sd          x0, 16(s1)
	sd          x0, 24(s1)
	sb          x0, 68(s1)
	sw          x0, 72(s1)
	sw          x0, 76(s1)
	addi        a0, s1, 80
	call        VectorInit

	// *** Basic block 5

	addi        a0, s1, 104
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorInit
.func_end_SyntaxResetForNewDeclaration:
	.size SyntaxResetForNewDeclaration, .func_end_SyntaxResetForNewDeclaration-SyntaxResetForNewDeclaration

	.global SyntaxNewPCLabel
	.type SyntaxNewPCLabel, @function

SyntaxNewPCLabel:

	// *** Basic block 0

	.global StringInit
	.local next_pc_label
	.global StringPrintf
	.local next_pc_label_id
	.global NewLabelASTNode
	.global StringDestruct
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
	la          a0, next_pc_label
	mv          a1, x0
	call        StringInit

	// *** Basic block 1

	la          a0, next_pc_label
	lla         a1, .str.1
	la          t0, next_pc_label_id
	lw          t1, 0(t0)
	addi        t0, t1, 1
	la          t1, next_pc_label_id
	sw          t0, 0(t1)
	mv          a2, t1
	call        StringPrintf

	// *** Basic block 2

	la          t0, next_pc_label
	ld          a0, 16(t0)
	mv          a3, s1
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a1, x0
	call        NewLabelASTNode

	// *** Basic block 3

	mv          s2, a0
	la          a0, next_pc_label
	call        StringDestruct

	// *** Basic block 4

	mv          a0, s2

	// *** Basic block 5

.SyntaxNewPCLabel_label_56:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SyntaxNewPCLabel:
	.size SyntaxNewPCLabel, .func_end_SyntaxNewPCLabel-SyntaxNewPCLabel

	.global SyntaxFindSymbol
	.type SyntaxFindSymbol, @function

SyntaxFindSymbol:

	// *** Basic block 0

	.global FindLocalSymbol
	.global FindGlobalSymbol
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
	mv          t0, a0
	mv          s1, a1
	ld          t1, 16(t0)
	mv          a0, t1
	call        FindLocalSymbol

	// *** Basic block 1

	mv          s2, a0
	beq         s2, x0, .SyntaxFindSymbol_label_31

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.SyntaxFindSymbol_label_28:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.SyntaxFindSymbol_label_31:
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           FindGlobalSymbol
.func_end_SyntaxFindSymbol:
	.size SyntaxFindSymbol, .func_end_SyntaxFindSymbol-SyntaxFindSymbol

	.global SyntaxAddSymbol
	.type SyntaxAddSymbol, @function

SyntaxAddSymbol:

	// *** Basic block 0

	.global InsertGlobalSymbol
	.global InsertLocalSymbol
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
	ld          s3, 16(s1)
	bne         s3, x0, .SyntaxAddSymbol_label_29

	// *** Basic block 1

	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           InsertGlobalSymbol

	// *** Basic block 3

.SyntaxAddSymbol_label_26:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.SyntaxAddSymbol_label_29:
	mv          a1, s2
	mv          a0, s3
	call        InsertLocalSymbol

	// *** Basic block 5

	mv          s3, a0
	beqz        s3, .SyntaxAddSymbol_label_43

	// *** Basic block 6

	addi        a0, s1, 80
	mv          a1, s2
	call        VectorAppend

	// *** Basic block 7

.SyntaxAddSymbol_label_43:
	mv          a0, s3
	j           .SyntaxAddSymbol_label_26
.func_end_SyntaxAddSymbol:
	.size SyntaxAddSymbol, .func_end_SyntaxAddSymbol-SyntaxAddSymbol

	.global SyntaxFindTag
	.type SyntaxFindTag, @function

SyntaxFindTag:

	// *** Basic block 0

	.global FindLocalSymbol
	.global FindGlobalTag
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
	mv          t0, a0
	mv          s1, a1
	ld          t1, 24(t0)
	mv          a0, t1
	call        FindLocalSymbol

	// *** Basic block 1

	mv          s2, a0
	beq         s2, x0, .SyntaxFindTag_label_31

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.SyntaxFindTag_label_28:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.SyntaxFindTag_label_31:
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           FindGlobalTag
.func_end_SyntaxFindTag:
	.size SyntaxFindTag, .func_end_SyntaxFindTag-SyntaxFindTag

	.global SyntaxFindTopScopeTag
	.type SyntaxFindTopScopeTag, @function

SyntaxFindTopScopeTag:

	// *** Basic block 0

	.global FindSymbol
	.global FindGlobalTag
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
	ld          s2, 24(a0)
	beq         s2, x0, .SyntaxFindTopScopeTag_label_35

	// *** Basic block 1

	mv          a1, s1
	mv          a0, s2
	call        FindSymbol

	// *** Basic block 2

	mv          s2, a0
	beq         s2, x0, .SyntaxFindTopScopeTag_label_34

	// *** Basic block 3

	mv          a0, s2

	// *** Basic block 4

.SyntaxFindTopScopeTag_label_31:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.SyntaxFindTopScopeTag_label_34:

	// *** Basic block 6

.SyntaxFindTopScopeTag_label_35:
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           FindGlobalTag
.func_end_SyntaxFindTopScopeTag:
	.size SyntaxFindTopScopeTag, .func_end_SyntaxFindTopScopeTag-SyntaxFindTopScopeTag

	.global SyntaxAddTag
	.type SyntaxAddTag, @function

SyntaxAddTag:

	// *** Basic block 0

	.global InsertGlobalTag
	.global InsertLocalSymbol
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
	ld          s3, 24(s1)
	bne         s3, x0, .SyntaxAddTag_label_29

	// *** Basic block 1

	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           InsertGlobalTag

	// *** Basic block 3

.SyntaxAddTag_label_26:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.SyntaxAddTag_label_29:
	mv          a1, s2
	mv          a0, s3
	call        InsertLocalSymbol

	// *** Basic block 5

	mv          s3, a0
	beqz        s3, .SyntaxAddTag_label_43

	// *** Basic block 6

	addi        a0, s1, 80
	mv          a1, s2
	call        VectorAppend

	// *** Basic block 7

.SyntaxAddTag_label_43:
	mv          a0, s3
	j           .SyntaxAddTag_label_26
.func_end_SyntaxAddTag:
	.size SyntaxAddTag, .func_end_SyntaxAddTag-SyntaxAddTag

	.global SyntaxError
	.type SyntaxError, @function

SyntaxError:

	// *** Basic block 0

	.global abort_on_error
	.global longjmp
	.global error_abort_state
	.global VLexError
	addi sp, sp, -112
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// varargs function with 0 declared args
	sd a0, 0(s0)
	sd a1, 8(s0)
	sd a2, 16(s0)
	sd a3, 24(s0)
	sd a4, 32(s0)
	sd a5, 40(s0)
	sd a6, 48(s0)
	sd a7, 56(s0)
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	la          t0, abort_on_error
	lb          t0, 0(t0)
	beqz        t0, .SyntaxError_label_24

	// *** Basic block 1

	la          a0, error_abort_state
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	call        longjmp

	// *** Basic block 2

.SyntaxError_label_24:
	mv          s3, s0
	ld          a0, 0(s2)
	mv          a2, s3
	mv          a1, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 64
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VLexError
.func_end_SyntaxError:
	.size SyntaxError, .func_end_SyntaxError-SyntaxError

	.global SyntaxWarning
	.type SyntaxWarning, @function

SyntaxWarning:

	// *** Basic block 0

	.global VLexWarning
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, s0
	ld          a0, 0(t0)
	mv          a3, t1
	j           VLexWarning
.func_end_SyntaxWarning:
	.size SyntaxWarning, .func_end_SyntaxWarning-SyntaxWarning

	.global SyntaxNeedSemicolon
	.type SyntaxNeedSemicolon, @function

SyntaxNeedSemicolon:

	// *** Basic block 0

	.global LexMatch
	.global SyntaxError
	.global SyntaxRecover
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
	ld          a0, 0(s1)
	li          a1, 49		// 0x31 ASCII '1'
	call        LexMatch

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .SyntaxNeedSemicolon_label_35

	// *** Basic block 2

	lla         a1, .str.2
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 3

	ori         a1, s2, 256
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SyntaxRecover

	// *** Basic block 4

.SyntaxNeedSemicolon_label_35:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SyntaxNeedSemicolon:
	.size SyntaxNeedSemicolon, .func_end_SyntaxNeedSemicolon-SyntaxNeedSemicolon

	.global SyntaxFakeName
	.type SyntaxFakeName, @function

SyntaxFakeName:

	// *** Basic block 0

	.global snprintf
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
	addi        a0, s1, 32
	lla         a2, .str.3
	lw          a3, 64(s1)
	li          a1, 32		// 0x20 ASCII ' '
	call        snprintf

	// *** Basic block 1

	lw          t0, 64(s1)
	addi        t0, t0, 1
	sw          t0, 64(s1)
	addi        a0, s1, 32

	// *** Basic block 2

.SyntaxFakeName_label_32:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SyntaxFakeName:
	.size SyntaxFakeName, .func_end_SyntaxFakeName-SyntaxFakeName

	.global SyntaxParseStorage
	.type SyntaxParseStorage, @function

SyntaxParseStorage:

	// *** Basic block 0

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
	ld          s1, 0(a0)
	lw          s2, 72(s1)
	li          t0, 55		// 0x37 ASCII '7'
	beq         s2, t0, .SyntaxParseStorage_label_74

	// *** Basic block 1

	li          t0, 68		// 0x44 ASCII 'D'
	beq         s2, t0, .SyntaxParseStorage_label_58

	// *** Basic block 2

	li          t0, 78		// 0x4e ASCII 'N'
	beq         s2, t0, .SyntaxParseStorage_label_88

	// *** Basic block 3

	li          t0, 84		// 0x54 ASCII 'T'
	beq         s2, t0, .SyntaxParseStorage_label_81

	// *** Basic block 4

	li          t0, 87		// 0x57 ASCII 'W'
	beq         s2, t0, .SyntaxParseStorage_label_67

	// *** Basic block 5

	li          t0, 88		// 0x58 ASCII 'X'
	beq         s2, t0, .SyntaxParseStorage_label_95

	// *** Basic block 6

.SyntaxParseStorage_label_54:
	mv          a0, x0
	j           .SyntaxParseStorage_label_64

	// *** Basic block 7

.SyntaxParseStorage_label_58:
	mv          a0, s1
	call        LexNextToken

	// *** Basic block 8

	li          a0, 8		// 0x8 ASCII \x8

	// *** Basic block 9

.SyntaxParseStorage_label_64:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 10

.SyntaxParseStorage_label_67:
	mv          a0, s1
	call        LexNextToken

	// *** Basic block 11

	li          a0, 4		// 0x4 ASCII \x4
	j           .SyntaxParseStorage_label_64

	// *** Basic block 12

.SyntaxParseStorage_label_74:
	mv          a0, s1
	call        LexNextToken

	// *** Basic block 13

	li          a0, 1		// 0x1 ASCII \x1
	j           .SyntaxParseStorage_label_64

	// *** Basic block 14

.SyntaxParseStorage_label_81:
	mv          a0, s1
	call        LexNextToken

	// *** Basic block 15

	li          a0, 2		// 0x2 ASCII \x2
	j           .SyntaxParseStorage_label_64

	// *** Basic block 16

.SyntaxParseStorage_label_88:
	mv          a0, s1
	call        LexNextToken

	// *** Basic block 17

	li          a0, 16		// 0x10 ASCII \x10
	j           .SyntaxParseStorage_label_64

	// *** Basic block 18

.SyntaxParseStorage_label_95:
	mv          a0, s1
	call        LexNextToken

	// *** Basic block 19

	li          a0, 64		// 0x40 ASCII '@'
	j           .SyntaxParseStorage_label_64
.func_end_SyntaxParseStorage:
	.size SyntaxParseStorage, .func_end_SyntaxParseStorage-SyntaxParseStorage

	.local  IsDefinition
	.type IsDefinition, @function

IsDefinition:

	// *** Basic block 0

	.global StorageIs
	.global LexLookingAt
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
	li          a1, 8		// 0x8 ASCII \x8
	mv          a0, a2
	call        StorageIs

	// *** Basic block 1

	beqz        a0, .IsDefinition_label_33

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.IsDefinition_label_30:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.IsDefinition_label_33:
	lb          t0, 56(s1)
	slli        t0, t0, 59
	srai        t0, t0, 63
	beqz        t0, .IsDefinition_label_42

	// *** Basic block 5

	li          a0, 1		// 0x1 ASCII \x1
	j           .IsDefinition_label_30

	// *** Basic block 6

.IsDefinition_label_42:
	ld          s1, 0(s2)
	li          t0, 12		// 0xc ASCII \xc
	mv          a1, t0
	mv          a0, s1
	call        LexLookingAt

	// *** Basic block 7

	bnez        a0, .IsDefinition_label_60

	// *** Basic block 8

	li          t0, 24		// 0x18 ASCII \x18
	mv          a1, t0
	mv          a0, s1
	call        LexLookingAt

	// *** Basic block 10

.IsDefinition_label_60:
	j           .IsDefinition_label_30
.func_end_IsDefinition:
	.size IsDefinition, .func_end_IsDefinition-IsDefinition

	.local  ParseDesignatedInitializer
	.type ParseDesignatedInitializer, @function

ParseDesignatedInitializer:

	// *** Basic block 0

	.global NewVector
	.global LexLookingAt
	.global LexMatch
	.global SyntaxParseExpression
	.global AnalyzeExpression
	.global EvaluateIntegerExpression
	.global SyntaxError
	.global SyntaxNeedBracket
	.global VectorAppend
	.global NewArrayDesignator
	.global NewString
	.global LexNextToken
	.global SyntaxFakeName
	.global NewStructDesignator
	.global SyntaxRecover
	.local ParseBracedInitializer
	.global NewExpressionInitializerASTNode
	.global SyntaxParseSingleExpression
	.global NewDesignatedInitializerASTNode
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -32(s0)
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
	mv          s2, a1
	call        NewVector

	// *** Basic block 1

	mv          s3, a0
	ld          s4, 0(s1)
	ld          s5, 56(s4)
	li          s7, 32		// 0x20 ASCII ' '
	mv          a1, s7
	mv          a0, s4
	call        LexLookingAt

	// *** Basic block 2

	addi        t0, s4, 80
	ld          s8, 16(t0)
	mv          s6, a0
	bnez        a0, .ParseDesignatedInitializer_label_71

	// *** Basic block 3

	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s4
	call        LexLookingAt

	// *** Basic block 4

	mv          s6, a0

	// *** Basic block 5

.ParseDesignatedInitializer_label_71:
	beqz        s6, .ParseDesignatedInitializer_label_200

	// *** Basic block 6

.ParseDesignatedInitializer_label_73:
	mv          a1, s7
	mv          a0, s4
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .ParseDesignatedInitializer_label_133

	// *** Basic block 8

	li          t0, 256		// 0x100
	mv          a1, t0
	mv          a0, s1
	call        SyntaxParseExpression

	// *** Basic block 9

	mv          s9, a0
	mv          a0, s9
	call        AnalyzeExpression

	// *** Basic block 10

	mv          s9, a0
	addi        a1, s0, -32
	mv          a0, s9
	call        EvaluateIntegerExpression

	// *** Basic block 11

	mv          s10, a0
	not         t0, s10
	beqz        t0, .ParseDesignatedInitializer_label_111

	// *** Basic block 12

	lla         a1, .str.4
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 13

	sd          x0, -32(s0)

	// *** Basic block 14

.ParseDesignatedInitializer_label_111:
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	li          t0, 48		// 0x30 ASCII '0'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 15

	ld          a1, -32(s0)
	mv          a0, x0
	call        NewArrayDesignator

	// *** Basic block 16

	mv          a1, a0
	mv          a0, s3
	call        VectorAppend

	// *** Basic block 17

	j           .ParseDesignatedInitializer_label_181

	// *** Basic block 18

.ParseDesignatedInitializer_label_133:
	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s4
	call        LexMatch

	// *** Basic block 19

	beqz        a0, .ParseDesignatedInitializer_label_180

	// *** Basic block 20

	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s4
	call        LexLookingAt

	// *** Basic block 21

	beqz        a0, .ParseDesignatedInitializer_label_157

	// *** Basic block 22

	mv          a0, s8
	call        NewString

	// *** Basic block 23

	mv          s8, a0
	mv          a0, s4
	call        LexNextToken

	// *** Basic block 24

	j           .ParseDesignatedInitializer_label_171

	// *** Basic block 25

.ParseDesignatedInitializer_label_157:
	lla         a1, .str.5
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 26

	mv          a0, s1
	call        SyntaxFakeName

	// *** Basic block 27

	call        NewString

	// *** Basic block 28

	mv          s8, a0

	// *** Basic block 29

.ParseDesignatedInitializer_label_171:
	mv          a0, s8
	call        NewStructDesignator

	// *** Basic block 30

	mv          a1, a0
	mv          a0, s3
	call        VectorAppend

	// *** Basic block 31

.ParseDesignatedInitializer_label_180:

	// *** Basic block 32

.ParseDesignatedInitializer_label_181:
	mv          a1, s7
	mv          a0, s4
	call        LexLookingAt

	// *** Basic block 33

	mv          s11, a0
	bnez        a0, .ParseDesignatedInitializer_label_198

	// *** Basic block 34

	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s4
	call        LexLookingAt

	// *** Basic block 35

	mv          s11, a0

	// *** Basic block 36

.ParseDesignatedInitializer_label_198:
	bnez        s11, .ParseDesignatedInitializer_label_73

	// *** Basic block 37

.ParseDesignatedInitializer_label_200:
	li          t0, 12		// 0xc ASCII \xc
	mv          a1, t0
	mv          a0, s4
	call        LexMatch

	// *** Basic block 38

	not         t0, a0
	beqz        t0, .ParseDesignatedInitializer_label_222

	// *** Basic block 39

	lla         a1, .str.6
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 40

	li          t0, 64		// 0x40 ASCII '@'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxRecover

	// *** Basic block 41

	j           .ParseDesignatedInitializer_label_262

	// *** Basic block 42

.ParseDesignatedInitializer_label_222:
	li          t0, 24		// 0x18 ASCII \x18
	mv          a1, t0
	mv          a0, s4
	call        LexMatch

	// *** Basic block 43

	beqz        a0, .ParseDesignatedInitializer_label_236

	// *** Basic block 44

	mv          a0, s1
	call        ParseBracedInitializer

	// *** Basic block 45

	mv          s4, a0
	j           .ParseDesignatedInitializer_label_249

	// *** Basic block 46

.ParseDesignatedInitializer_label_236:
	li          t0, 64		// 0x40 ASCII '@'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxParseSingleExpression

	// *** Basic block 47

	mv          a1, s5
	call        NewExpressionInitializerASTNode

	// *** Basic block 48

	mv          s4, a0

	// *** Basic block 49

.ParseDesignatedInitializer_label_249:
	mv          a2, s5
	mv          a1, s4
	mv          a0, s3
	call        NewDesignatedInitializerASTNode

	// *** Basic block 50

	mv          a1, a0
	mv          a0, s2
	call        VectorAppend

	// *** Basic block 51

.ParseDesignatedInitializer_label_262:
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
.func_end_ParseDesignatedInitializer:
	.size ParseDesignatedInitializer, .func_end_ParseDesignatedInitializer-ParseDesignatedInitializer

	.local  ParseBracedInitializer
	.type ParseBracedInitializer, @function

ParseBracedInitializer:

	// *** Basic block 0

	.global NewVector
	.global LexLookingAt
	.global LexMatch
	.global VectorAppend
	.local ParseBracedInitializer
	.local ParseDesignatedInitializer
	.global SyntaxParseSingleExpression
	.global NewExpressionInitializerASTNode
	.global SyntaxNeedBracket
	.global NewBracedInitializerASTNode
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
	call        NewVector

	// *** Basic block 1

	mv          s2, a0
	ld          s3, 0(s1)
	ld          s4, 56(s3)
	li          s5, 44		// 0x2c ASCII ','
	mv          a1, s5
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .ParseBracedInitializer_label_121

	// *** Basic block 3

.ParseBracedInitializer_label_41:
	li          t0, 24		// 0x18 ASCII \x18
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 4

	beqz        a0, .ParseBracedInitializer_label_58

	// *** Basic block 5

	mv          a0, s1
	call        ParseBracedInitializer

	// *** Basic block 6

	mv          a1, a0
	mv          a0, s2
	call        VectorAppend

	// *** Basic block 7

	j           .ParseBracedInitializer_label_103

	// *** Basic block 8

.ParseBracedInitializer_label_58:
	li          t0, 32		// 0x20 ASCII ' '
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 9

	mv          s6, a0
	bnez        a0, .ParseBracedInitializer_label_75

	// *** Basic block 10

	li          t0, 19		// 0x13 ASCII \x13
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 11

	mv          s6, a0

	// *** Basic block 12

.ParseBracedInitializer_label_75:
	beqz        s6, .ParseBracedInitializer_label_83

	// *** Basic block 13

	mv          a1, s2
	mv          a0, s1
	call        ParseDesignatedInitializer

	// *** Basic block 14

	j           .ParseBracedInitializer_label_102

	// *** Basic block 15

.ParseBracedInitializer_label_83:
	li          t0, 64		// 0x40 ASCII '@'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxParseSingleExpression

	// *** Basic block 16

	mv          s7, a0
	mv          a1, s4
	mv          a0, s7
	call        NewExpressionInitializerASTNode

	// *** Basic block 17

	mv          a1, a0
	mv          a0, s2
	call        VectorAppend

	// *** Basic block 18

.ParseBracedInitializer_label_102:

	// *** Basic block 19

.ParseBracedInitializer_label_103:
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 20

	not         t0, a0
	bnez        t0, .ParseBracedInitializer_label_121

	// *** Basic block 21

.ParseBracedInitializer_label_112:
	mv          a1, s5
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 22

	not         t0, a0
	bnez        t0, .ParseBracedInitializer_label_41

	// *** Basic block 23

.ParseBracedInitializer_label_121:
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	mv          a1, s5
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 24

	mv          a1, s4
	mv          a0, s2
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
	j           NewBracedInitializerASTNode
.func_end_ParseBracedInitializer:
	.size ParseBracedInitializer, .func_end_ParseBracedInitializer-ParseBracedInitializer

	.local  ParseInitializer
	.type ParseInitializer, @function

ParseInitializer:

	// *** Basic block 0

	.global StorageIs
	.global SyntaxWarning
	.global SyntaxError
	.global LexMatch
	.local ParseBracedInitializer
	.global NewExpressionInitializerASTNode
	.global SyntaxParseSingleExpression
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
	mv          s1, a2
	mv          s2, a0
	mv          s3, a1
	li          s4, 8		// 0x8 ASCII \x8
	mv          a1, s4
	mv          a0, s1
	call        StorageIs

	// *** Basic block 1

	beqz        a0, .ParseInitializer_label_44

	// *** Basic block 2

	lla         a1, .str.7
	lla         a2, .str.8
	mv          a0, s2
	call        SyntaxWarning

	// *** Basic block 3

.ParseInitializer_label_44:
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	mv          a0, s1
	call        StorageIs

	// *** Basic block 4

	beqz        a0, .ParseInitializer_label_58

	// *** Basic block 5

	lla         a1, .str.9
	mv          a0, s2
	call        SyntaxError

	// *** Basic block 6

.ParseInitializer_label_58:
	mv          a1, s4
	mv          a0, s1
	call        StorageIs

	// *** Basic block 7

	not         t0, a0
	beqz        t0, .ParseInitializer_label_73

	// *** Basic block 8

	lb          t0, 56(s3)
	andi        t0, t0, -2
	ori         t0, t0, 1
	sb          t0, 56(s3)

	// *** Basic block 9

.ParseInitializer_label_73:
	ld          s1, 0(s2)
	li          t0, 24		// 0x18 ASCII \x18
	mv          a1, t0
	mv          a0, s1
	call        LexMatch

	// *** Basic block 10

	beqz        a0, .ParseInitializer_label_91

	// *** Basic block 11

	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ParseBracedInitializer

	// *** Basic block 14

.ParseInitializer_label_91:
	ld          s4, 56(s1)
	li          t0, 257		// 0x101
	mv          a1, t0
	mv          a0, s2
	call        SyntaxParseSingleExpression

	// *** Basic block 15

	mv          a1, s4
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewExpressionInitializerASTNode
.func_end_ParseInitializer:
	.size ParseInitializer, .func_end_ParseInitializer-ParseInitializer

	.global ParseAttribute
	.type ParseAttribute, @function

ParseAttribute:

	// *** Basic block 0

	.global LexReadAttributes
	.global StringSplit
	.global SyntaxNeedBracket
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
	mv          s2, a1
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	ld          a0, 0(s1)
	addi        a1, s0, -64
	call        LexReadAttributes

	// *** Basic block 1

	addi        a0, s0, -64
	mv          a2, s2
	li          t0, 44		// 0x2c ASCII ','
	mv          a1, t0
	call        StringSplit

	// *** Basic block 2

	mv          a2, x0
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedBracket

	// *** Basic block 3

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 4

	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ParseAttribute:
	.size ParseAttribute, .func_end_ParseAttribute-ParseAttribute

	.local  ResolveOldStyleFormalArgument
	.type ResolveOldStyleFormalArgument, @function

ResolveOldStyleFormalArgument:

	// *** Basic block 0

	.global StringEqualString
	.global SymbolDelete
	.global SyntaxError
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
	addi        t0, a1, 32
	addi        s3, t0, 8
	mv          s4, x0
	ld          s5, 8(s3)
	bge         x0, s5, .ResolveOldStyleFormalArgument_label_72

	// *** Basic block 1

	ld          s6, 0(s3)

	// *** Basic block 2

.ResolveOldStyleFormalArgument_label_36:
	slli        s7, s4, 3
	add         t0, s6, s7
	ld          s6, 0(t0)
	mv          a1, s1
	mv          a0, s6
	call        StringEqualString

	// *** Basic block 3

	beqz        a0, .ResolveOldStyleFormalArgument_label_67

	// *** Basic block 4

	ld          t0, 0(s3)
	add         t0, t0, s7
	sd          s1, 0(t0)
	lb          t0, 56(s1)
	andi        t0, t0, -17
	ori         t0, t0, 16
	sb          t0, 56(s1)
	lw          t0, 112(s6)
	sw          t0, 112(s1)
	mv          a0, s6
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
	j           SymbolDelete

	// *** Basic block 5

.ResolveOldStyleFormalArgument_label_64:
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

	// *** Basic block 6

.ResolveOldStyleFormalArgument_label_67:

	// *** Basic block 7

.ResolveOldStyleFormalArgument_label_68:
	addi        s4, s4, 1
	bge         s4, s5, .ResolveOldStyleFormalArgument_label_36

	// *** Basic block 8

.ResolveOldStyleFormalArgument_label_72:
	lla         a1, .str.10
	ld          a2, 16(s1)
	mv          a0, s2
	call        SyntaxError

	// *** Basic block 9

	j           .ResolveOldStyleFormalArgument_label_64
.func_end_ResolveOldStyleFormalArgument:
	.size ResolveOldStyleFormalArgument, .func_end_ResolveOldStyleFormalArgument-ResolveOldStyleFormalArgument

	.local  DeclareOrDefineFunction
	.type DeclareOrDefineFunction, @function

DeclareOrDefineFunction:

	// *** Basic block 0

	.global LexLookingAt
	.global TypeParserInit
	.global TypeParserParseType
	.global SyntaxError
	.global SyntaxRecover
	.global TypeParserParseDeclarator
	.local ResolveOldStyleFormalArgument
	.global LexMatch
	.global SyntaxNeedSemicolon
	.global NewVector
	.global compiler
	.global VectorAppend
	.global SyntaxNewPCLabel
	.global SyntaxLookingAtDeclaration
	.global SyntaxParseLocalDeclaration
	.global SyntaxParseStatement
	.global NewCompoundStatementASTNode
	.global SyntaxNeedBracket
	.global NewVariableDeclarationASTNode
	.global NewDeclarationListASTNode
	sd          a0, -0(s0)	// Spilled @351
	sd          a0, -0(s0)	// Spilled @376
	addi sp, sp, -192
	// Saved return address (offset 184) and frame pointer (offset 176)
	sd ra, 184(sp)
	sd s0, 176(sp)
	addi s0, sp, 192
	// Local vars at offset -96(s0)
	// Spilled register region: 8 bytes at -104(s0) to -96(s0)
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
	mv          s3, a3
	mv          s4, a1
	ld          s5, 40(s1)
	addi        t0, s5, 32
	lb          t0, 50(t0)
	beqz        t0, .DeclareOrDefineFunction_label_192

	// *** Basic block 1

	ld          s6, 0(s2)
	li          s7, 24		// 0x18 ASCII \x18
	mv          a1, s7
	mv          a0, s6
	call        LexLookingAt

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .DeclareOrDefineFunction_label_176

	// *** Basic block 3

.DeclareOrDefineFunction_label_74:
	addi        a0, s0, -96
	li          s8, 1		// 0x1 ASCII \x1
	mv          a4, s8
	mv          a3, s8
	mv          a2, s2
	mv          a1, s6
	call        TypeParserInit

	// *** Basic block 4

	addi        a0, s0, -96
	mv          a1, s8
	call        TypeParserParseType

	// *** Basic block 5

	mv          s8, a0
	bne         s8, x0, .DeclareOrDefineFunction_label_115

	// *** Basic block 6

	lla         a1, .str.11
	mv          a0, s2
	call        SyntaxError

	// *** Basic block 7

	li          t0, 256		// 0x100
	mv          a1, t0
	mv          a0, s2
	call        SyntaxRecover

	// *** Basic block 8

	j           .DeclareOrDefineFunction_label_161

	// *** Basic block 9

.DeclareOrDefineFunction_label_115:
	li          s9, 49		// 0x31 ASCII '1'
	mv          a1, s9
	mv          a0, s6
	call        LexLookingAt

	// *** Basic block 10

	not         t0, a0
	beqz        t0, .DeclareOrDefineFunction_label_160

	// *** Basic block 11

.DeclareOrDefineFunction_label_124:
	addi        a0, s0, -96
	mv          a1, s8
	call        TypeParserParseDeclarator

	// *** Basic block 12

	mv          s10, a0
	beq         s10, x0, .DeclareOrDefineFunction_label_142

	// *** Basic block 13

	mv          a2, s10
	mv          a1, s5
	mv          a0, s2
	call        ResolveOldStyleFormalArgument

	// *** Basic block 14

.DeclareOrDefineFunction_label_142:
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	mv          a0, s6
	call        LexMatch

	// *** Basic block 15

	not         t0, a0
	bnez        t0, .DeclareOrDefineFunction_label_160

	// *** Basic block 16

.DeclareOrDefineFunction_label_151:
	mv          a1, s9
	mv          a0, s6
	call        LexLookingAt

	// *** Basic block 17

	not         t0, a0
	bnez        t0, .DeclareOrDefineFunction_label_124

	// *** Basic block 18

.DeclareOrDefineFunction_label_160:

	// *** Basic block 19

.DeclareOrDefineFunction_label_161:
	li          t0, 32		// 0x20 ASCII ' '
	mv          a1, t0
	mv          a0, s2
	call        SyntaxNeedSemicolon

	// *** Basic block 20

	mv          a1, s7
	mv          a0, s6
	call        LexLookingAt

	// *** Basic block 21

	not         t0, a0
	bnez        t0, .DeclareOrDefineFunction_label_74

	// *** Basic block 22

.DeclareOrDefineFunction_label_176:
	mv          a1, s7
	mv          a0, s6
	call        LexLookingAt

	// *** Basic block 23

	not         t0, a0
	beqz        t0, .DeclareOrDefineFunction_label_191

	// *** Basic block 24

	lla         a1, .str.12
	mv          a0, s2
	call        SyntaxError

	// *** Basic block 25

.DeclareOrDefineFunction_label_191:

	// *** Basic block 26

.DeclareOrDefineFunction_label_192:
	ld          a0, 0(s2)
	li          t0, 24		// 0x18 ASCII \x18
	mv          a1, t0
	call        LexMatch

	// *** Basic block 27

	beqz        a0, .DeclareOrDefineFunction_label_356

	// *** Basic block 28

	sub         t0, s3, x0
	snez        s5, t0
	beq         s3, x0, .DeclareOrDefineFunction_label_215

	// *** Basic block 29

	ld          s6, 40(s3)
	lw          t0, 16(s6)
	addi        t0, t0, -3
	seqz        s6, t0

	// *** Basic block 30

.DeclareOrDefineFunction_label_212:
	mv          s5, s6
	j           .DeclareOrDefineFunction_label_215

	// *** Basic block 31

.DeclareOrDefineFunction_label_215:
	beqz        s5, .DeclareOrDefineFunction_label_219

	// *** Basic block 32

	sd          s1, 112(s3)

	// *** Basic block 33

.DeclareOrDefineFunction_label_219:
	lw          s6, 152(s2)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 152(s2)
	lb          t1, 56(s1)
	andi        t1, t1, -2
	ori         t1, t1, 1
	sb          t1, 56(s1)
	ld          t1, 40(s1)
	addi        t1, t1, 32
	sb          t0, 49(t1)
	call        NewVector

	// *** Basic block 34

	mv          s7, a0
	la          t0, compiler
	ld          t0, 0(t0)
	lb          s9, 1228(t0)
	beqz        s9, .DeclareOrDefineFunction_label_255

	// *** Basic block 35

	ld          t0, 0(s2)
	ld          a0, 56(t0)
	call        SyntaxNewPCLabel

	// *** Basic block 36

	mv          a1, a0
	mv          a0, s7
	call        VectorAppend

	// *** Basic block 37

.DeclareOrDefineFunction_label_255:
	ld          t0, 0(s2)
	lw          s11, 72(t0)
	li          a0, 44		// 0x2c ASCII ','
	beq         s11, a0, .DeclareOrDefineFunction_label_295

	// *** Basic block 38

.DeclareOrDefineFunction_label_264:
	mv          a0, s2
	call        SyntaxLookingAtDeclaration

	// *** Basic block 39

	beqz        a0, .DeclareOrDefineFunction_label_275

	// *** Basic block 40

	mv          a0, s2
	call        SyntaxParseLocalDeclaration

	// *** Basic block 41

	j           .DeclareOrDefineFunction_label_283

	// *** Basic block 42

.DeclareOrDefineFunction_label_275:
	li          t0, 256		// 0x100
	mv          a1, t0
	mv          a0, s2
	call        SyntaxParseStatement

	// *** Basic block 43


	// *** Basic block 44

.DeclareOrDefineFunction_label_283:
	beq         a0, x0, .DeclareOrDefineFunction_label_292

	// *** Basic block 45

	mv          a1, a0
	mv          a0, s7
	call        VectorAppend

	// *** Basic block 46

.DeclareOrDefineFunction_label_292:
	bne         s11, a0, .DeclareOrDefineFunction_label_264

	// *** Basic block 47

.DeclareOrDefineFunction_label_295:
	sw          s6, 152(s2)
	beqz        s9, .DeclareOrDefineFunction_label_309

	// *** Basic block 48

	ld          t0, 0(s2)
	ld          a0, 56(t0)
	call        SyntaxNewPCLabel

	// *** Basic block 49

	mv          a1, a0
	mv          a0, s7
	call        VectorAppend

	// *** Basic block 50

.DeclareOrDefineFunction_label_309:
	ld          t0, 40(s1)
	addi        s6, t0, 32
	ld          t0, 0(s2)
	ld          s9, 56(t0)
	mv          a1, s9
	mv          a0, s7
	call        NewCompoundStatementASTNode

	// *** Basic block 51

	sd          a0, 40(s6)
	li          t0, 128		// 0x80 ASCII \x80
	mv          a2, t0
	mv          a1, a0
	mv          a0, s2
	call        SyntaxNeedBracket

	// *** Basic block 52

	mv          a2, s9
	mv          a1, x0
	mv          a0, s1
	call        NewVariableDeclarationASTNode

	// *** Basic block 53

	mv          s6, a0
	mv          a1, s6
	mv          a0, s4
	call        VectorAppend

	// *** Basic block 54

	mv          a1, s9
	mv          a0, s4
	call        NewDeclarationListASTNode

	// *** Basic block 55


	// *** Basic block 56

.DeclareOrDefineFunction_label_353:
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

	// *** Basic block 57

.DeclareOrDefineFunction_label_356:
	sub         t1, s3, x0
	snez        t0, t1
	beq         s3, x0, .DeclareOrDefineFunction_label_366

	// *** Basic block 58

	ld          t1, 40(s3)
	addi        t1, t1, 32
	lb          t0, 51(t1)

	// *** Basic block 59

.DeclareOrDefineFunction_label_366:
	beqz        t0, .DeclareOrDefineFunction_label_374

	// *** Basic block 60

	addi        t0, s3, 56
	lb          t1, 1(t0)
	andi        t1, t1, -3
	ori         t1, t1, 2
	sb          t1, 1(t0)

	// *** Basic block 61

.DeclareOrDefineFunction_label_374:

	// *** Basic block 62

.DeclareOrDefineFunction_label_375:
	mv          a0, x0
	j           .DeclareOrDefineFunction_label_353
.func_end_DeclareOrDefineFunction:
	.size DeclareOrDefineFunction, .func_end_DeclareOrDefineFunction-DeclareOrDefineFunction

	.local  IsPowerOf2OrZero
	.type IsPowerOf2OrZero, @function

IsPowerOf2OrZero:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	addi        t0, a0, -1
	and         t0, a0, t0
	seqz        a0, t0

	// *** Basic block 1

.IsPowerOf2OrZero_label_12:
	ret         
.func_end_IsPowerOf2OrZero:
	.size IsPowerOf2OrZero, .func_end_IsPowerOf2OrZero-IsPowerOf2OrZero

	.local  CheckThreadLocal
	.type CheckThreadLocal, @function

CheckThreadLocal:

	// *** Basic block 0

	.global StorageIs
	.global SyntaxError
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
	lw          s3, 48(s1)
	li          a1, 64		// 0x40 ASCII '@'
	mv          a0, s3
	call        StorageIs

	// *** Basic block 1

	beqz        a0, .CheckThreadLocal_label_56

	// *** Basic block 2

	ld          s4, 40(s1)
	lw          t0, 16(s4)
	addi        t0, t0, -3
	seqz        s4, t0

	// *** Basic block 3

.CheckThreadLocal_label_35:
	mv          s1, s4
	li          t0, 10		// 0xa ASCII \xa
	mv          a1, t0
	mv          a0, s3
	call        StorageIs

	// *** Basic block 4

	not         t0, a0
	or          s1, s1, t0
	beqz        s1, .CheckThreadLocal_label_55

	// *** Basic block 5

	j           .CheckThreadLocal_label_48

	// *** Basic block 6

.CheckThreadLocal_label_48:
	lla         a1, .str.13
	mv          a0, s2
	call        SyntaxError

	// *** Basic block 7

.CheckThreadLocal_label_55:

	// *** Basic block 8

.CheckThreadLocal_label_56:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CheckThreadLocal:
	.size CheckThreadLocal, .func_end_CheckThreadLocal-CheckThreadLocal

	.local  ParseDeclarationSpecifier
	.type ParseDeclarationSpecifier, @function

ParseDeclarationSpecifier:

	// *** Basic block 0

	.global TypeParserInit
	.global LexEof
	.global SyntaxParseStorage
	.global SyntaxWarning
	.local IsPowerOf2OrZero
	.global SyntaxError
	.global LexMatch
	.global SyntaxLookingAtType
	.global TypeParserParseAndCombineTypes
	.global ParseAttribute
	.global TypeParserBuildTypeRecord
	addi sp, sp, -208
	// Saved return address (offset 200) and frame pointer (offset 192)
	sd ra, 200(sp)
	sd s0, 192(sp)
	addi s0, sp, 208
	// Local vars at offset -128(s0)
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
	mv          s3, a2
	mv          s4, a4
	mv          s5, a3
	sd          x0, -128(s0)
	sd          x0, -120(s0)
	sd          x0, -112(s0)
	sw          x0, -128(s0)
	addi        t0, s0, -128
	sw          x0, 4(t0)
	addi        t0, s0, -128
	sd          x0, 8(t0)
	addi        t0, s0, -128
	sb          x0, 16(t0)
	addi        a0, s0, -104
	ld          s6, 0(s1)
	mv          a4, a5
	mv          a3, x0
	mv          a2, s1
	mv          a1, s6
	call        TypeParserInit

	// *** Basic block 1

	lw          s7, 0(s2)
	lb          s8, 0(s3)
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 0(s3)
	addi        s9, s0, -128
	mv          a0, s6
	call        LexEof

	// *** Basic block 2

	not         t0, a0
	beqz        t0, .ParseDeclarationSpecifier_label_199

	// *** Basic block 3

.ParseDeclarationSpecifier_label_85:
	mv          a0, s1
	call        SyntaxParseStorage

	// *** Basic block 4

	mv          s3, a0
	beqz        s3, .ParseDeclarationSpecifier_label_125

	// *** Basic block 5

	mv          s10, s3
	and         t0, s10, s7
	beqz        t0, .ParseDeclarationSpecifier_label_108

	// *** Basic block 6

	lla         a1, .str.14
	lla         a2, .str.15
	mv          a0, s1
	call        SyntaxWarning

	// *** Basic block 7

.ParseDeclarationSpecifier_label_108:
	or          t0, s10, s7
	andi        a0, t0, -65
	call        IsPowerOf2OrZero

	// *** Basic block 8

	not         t0, a0
	beqz        t0, .ParseDeclarationSpecifier_label_121

	// *** Basic block 9

	lla         a1, .str.16
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 10

.ParseDeclarationSpecifier_label_121:
	or          t0, s7, s3
	sw          t0, 0(s2)
	j           .ParseDeclarationSpecifier_label_193

	// *** Basic block 11

.ParseDeclarationSpecifier_label_125:
	li          t0, 75		// 0x4b ASCII 'K'
	mv          a1, t0
	mv          a0, s6
	call        LexMatch

	// *** Basic block 12

	beqz        a0, .ParseDeclarationSpecifier_label_145

	// *** Basic block 13

	beqz        s8, .ParseDeclarationSpecifier_label_143

	// *** Basic block 14

	lla         a1, .str.17
	lla         a2, .str.18
	mv          a0, s1
	call        SyntaxWarning

	// *** Basic block 15

.ParseDeclarationSpecifier_label_143:
	j           .ParseDeclarationSpecifier_label_192

	// *** Basic block 16

.ParseDeclarationSpecifier_label_145:
	mv          a0, s1
	call        SyntaxLookingAtType

	// *** Basic block 17

	mv          s7, a0
	beqz        a0, .ParseDeclarationSpecifier_label_156

	// *** Basic block 18

	lw          t0, -128(s0)
	li          t1, 7168		// 0x1c00
	and         t0, t0, t1
	seqz        s7, t0

	// *** Basic block 19

.ParseDeclarationSpecifier_label_156:
	beqz        s7, .ParseDeclarationSpecifier_label_166

	// *** Basic block 20

	addi        a1, s0, -104
	addi        a2, s0, -128
	mv          a0, s9
	call        TypeParserParseAndCombineTypes

	// *** Basic block 21

	j           .ParseDeclarationSpecifier_label_191

	// *** Basic block 22

.ParseDeclarationSpecifier_label_166:
	li          t0, 96		// 0x60 ASCII '`'
	mv          a1, t0
	mv          a0, s6
	call        LexMatch

	// *** Basic block 23

	beqz        a0, .ParseDeclarationSpecifier_label_180

	// *** Basic block 24

	mv          a1, s4
	mv          a0, s1
	call        ParseAttribute

	// *** Basic block 25

	j           .ParseDeclarationSpecifier_label_190

	// *** Basic block 26

.ParseDeclarationSpecifier_label_180:
	addi        a0, s0, -104
	addi        a1, s0, -128
	call        TypeParserBuildTypeRecord

	// *** Basic block 27

	sd          a0, 0(s5)

	// *** Basic block 28

.ParseDeclarationSpecifier_label_187:
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

	// *** Basic block 29

.ParseDeclarationSpecifier_label_190:

	// *** Basic block 30

.ParseDeclarationSpecifier_label_191:

	// *** Basic block 31

.ParseDeclarationSpecifier_label_192:

	// *** Basic block 32

.ParseDeclarationSpecifier_label_193:
	mv          a0, s6
	call        LexEof

	// *** Basic block 33

	not         t0, a0
	bnez        t0, .ParseDeclarationSpecifier_label_85

	// *** Basic block 34

.ParseDeclarationSpecifier_label_199:
	j           .ParseDeclarationSpecifier_label_187
.func_end_ParseDeclarationSpecifier:
	.size ParseDeclarationSpecifier, .func_end_ParseDeclarationSpecifier-ParseDeclarationSpecifier

	.local  ParseExternalDeclarationList
	.type ParseExternalDeclarationList, @function

ParseExternalDeclarationList:

	// *** Basic block 0

	.global LexEof
	.global LexLookingAt
	.global TypeParserParseDeclarator
	.global FindGlobalSymbol
	.global StorageIs
	.global SyntaxError
	.local IsDefinition
	.global TypeEqual
	.global TypeErrorDetails
	.global DecodeSourceLocation
	.global ReportNote
	.global InsertGlobalSymbol
	.global printf
	.global abort
	.global LexMatch
	.global TypeParserReset
	.global ParseAttribute
	.global VectorCopy
	.global VectorClear
	.local DeclareOrDefineFunction
	.global SymbolDelete
	.global SyntaxNeedBracket
	.global LexNextToken
	.local ParseInitializer
	.global NewVariableDeclarationASTNode
	.global VectorAppend
	.global TypeIsEnum
	.global TypeIsStructOrUnion
	.global compiler
	.global BuildTypeDebugInfo
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
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	sd          s3, -64(s0)	// Spilled @76
	mv          s4, a3
	mv          s5, a4
	sd          s5, -72(s0)	// Spilled @82
	ld          s6, 8(s1)
	ld          s7, 0(s6)
	mv          a0, s7
	call        LexEof

	// *** Basic block 1

	ld          s8, 0(s6)
	sd          s6, -56(s0)	// Spilled @88
	ld          s9, 0(s1)
	ld          s10, 56(s8)
	lb          s11, 68(s1)
	ld          s6, 56(s8)
	not         t0, a0
	beqz        t0, .ParseExternalDeclarationList_label_550

	// *** Basic block 2

.ParseExternalDeclarationList_label_103:
	li          t0, 49		// 0x31 ASCII '1'
	mv          a1, t0
	mv          a0, s8
	call        LexLookingAt

	// *** Basic block 3

	bnez        a0, .ParseExternalDeclarationList_label_550

	// *** Basic block 4

.ParseExternalDeclarationList_label_111:
	mv          a1, s2
	mv          a0, s1
	call        TypeParserParseDeclarator

	// *** Basic block 5

	mv          s3, a0
	mv          s5, x0
	sd          s5, -64(s0)	// Spilled @119
	beq         s3, x0, .ParseExternalDeclarationList_label_358

	// *** Basic block 6

	mv          a0, s3
	call        FindGlobalSymbol

	// *** Basic block 7

	mv          s5, a0
	li          s5, 1		// 0x1 ASCII \x1
	ld          t0, -64(s0)	// Spilled @119
	beq         t0, x0, .ParseExternalDeclarationList_label_302

	// *** Basic block 8

	ld          t0, -64(s0)	// Spilled @119
	lb          t1, 56(t0)
	slli        t1, t1, 63
	srai        t1, t1, 63
	beqz        t1, .ParseExternalDeclarationList_label_186

	// *** Basic block 9

	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	ld          a0, -64(s0)	// Spilled @76
	call        StorageIs

	// *** Basic block 10

	beqz        a0, .ParseExternalDeclarationList_label_164

	// *** Basic block 11

	li          t0, 12		// 0xc ASCII \xc
	mv          a1, t0
	mv          a0, s9
	call        LexLookingAt

	// *** Basic block 12

	beqz        a0, .ParseExternalDeclarationList_label_162

	// *** Basic block 13

	lla         a1, .str.19
	ld          a2, 16(s3)
	ld          a0, -56(s0)	// Spilled @88
	call        SyntaxError

	// *** Basic block 14

	mv          s5, x0

	// *** Basic block 15

.ParseExternalDeclarationList_label_162:
	j           .ParseExternalDeclarationList_label_184

	// *** Basic block 16

.ParseExternalDeclarationList_label_164:
	ld          a2, -64(s0)	// Spilled @76
	ld          a1, -64(s0)	// Spilled @119
	mv          a0, s1
	call        IsDefinition

	// *** Basic block 17

	beqz        a0, .ParseExternalDeclarationList_label_183

	// *** Basic block 18

	lla         a1, .str.20
	ld          a2, 16(s3)
	ld          a0, -56(s0)	// Spilled @88
	call        SyntaxError

	// *** Basic block 19

	mv          s5, x0

	// *** Basic block 20

.ParseExternalDeclarationList_label_183:

	// *** Basic block 21

.ParseExternalDeclarationList_label_184:
	j           .ParseExternalDeclarationList_label_217

	// *** Basic block 22

.ParseExternalDeclarationList_label_186:
	ld          a2, -64(s0)	// Spilled @76
	mv          a1, s3
	mv          a0, s1
	call        IsDefinition

	// *** Basic block 23

	beqz        a0, .ParseExternalDeclarationList_label_201

	// *** Basic block 24

	ld          t0, -64(s0)	// Spilled @119
	lb          t1, 56(t0)
	andi        t1, t1, -2
	ori         t1, t1, 1
	ld          t2, -64(s0)	// Spilled @119
	sb          t1, 56(t2)
	j           .ParseExternalDeclarationList_label_216

	// *** Basic block 25

.ParseExternalDeclarationList_label_201:
	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	ld          a0, -64(s0)	// Spilled @76
	call        StorageIs

	// *** Basic block 26

	not         t0, a0
	beqz        t0, .ParseExternalDeclarationList_label_215

	// *** Basic block 27

	ld          t0, -64(s0)	// Spilled @119
	lb          t1, 56(t0)
	andi        t1, t1, -3
	ori         t1, t1, 2
	ld          t2, -64(s0)	// Spilled @119
	sb          t1, 56(t2)

	// *** Basic block 28

.ParseExternalDeclarationList_label_215:

	// *** Basic block 29

.ParseExternalDeclarationList_label_216:

	// *** Basic block 30

.ParseExternalDeclarationList_label_217:
	ld          s9, 40(s3)
	sd          s9, -64(s0)	// Spilled @219
	ld          t0, -64(s0)	// Spilled @119
	ld          s9, 40(t0)
	mv          a1, s9
	ld          a0, -64(s0)	// Spilled @219
	call        TypeEqual

	// *** Basic block 31

	not         t0, a0
	beqz        t0, .ParseExternalDeclarationList_label_272

	// *** Basic block 32

	lla         a1, .str.21
	ld          a2, 16(s3)
	ld          a0, -56(s0)	// Spilled @88
	call        SyntaxError

	// *** Basic block 33

	mv          a2, s9
	ld          a1, -64(s0)	// Spilled @219
	mv          a0, s10
	call        TypeErrorDetails

	// *** Basic block 34

	ld          t0, -64(s0)	// Spilled @119
	ld          a0, 104(t0)
	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	call        DecodeSourceLocation

	// *** Basic block 35

	ld          a0, -48(s0)
	lw          a1, -40(s0)
	lla         a2, .str.22
	call        ReportNote

	// *** Basic block 36

	mv          s5, x0
	j           .ParseExternalDeclarationList_label_300

	// *** Basic block 37

.ParseExternalDeclarationList_label_272:
	ld          t0, -64(s0)	// Spilled @119
	lw          t1, 48(t0)
	andi        s9, t1, -9
	lw          t1, 48(s3)
	andi        s10, t1, -9
	beq         s9, s10, .ParseExternalDeclarationList_label_294

	// *** Basic block 38

	lla         a1, .str.23
	ld          a2, 16(s3)
	ld          a0, -56(s0)	// Spilled @88
	call        SyntaxError

	// *** Basic block 39

	mv          s5, x0

	// *** Basic block 40

.ParseExternalDeclarationList_label_294:
	lb          t0, 56(s3)
	andi        t0, t0, -2
	ori         t0, t0, 1
	sb          t0, 56(s3)

	// *** Basic block 41

.ParseExternalDeclarationList_label_300:
	j           .ParseExternalDeclarationList_label_357

	// *** Basic block 42

.ParseExternalDeclarationList_label_302:
	mv          a0, s3
	call        InsertGlobalSymbol

	// *** Basic block 43

	mv          s9, a0
	beqz        s9, .ParseExternalDeclarationList_label_311

	// *** Basic block 44

	j           .ParseExternalDeclarationList_label_326

	// *** Basic block 45

.ParseExternalDeclarationList_label_311:
	lla         a0, .str.24
	lla         a1, .str.25
	lla         a3, .str.26
	li          t0, 556		// 0x22c
	mv          a2, t0
	call        printf

	// *** Basic block 46

	call        abort

	// *** Basic block 47

.ParseExternalDeclarationList_label_326:
	ld          a2, -64(s0)	// Spilled @76
	mv          a1, s3
	mv          a0, s1
	call        IsDefinition

	// *** Basic block 48

	beqz        a0, .ParseExternalDeclarationList_label_341

	// *** Basic block 49

	lb          t0, 56(s3)
	andi        t0, t0, -2
	ori         t0, t0, 1
	sb          t0, 56(s3)
	j           .ParseExternalDeclarationList_label_356

	// *** Basic block 50

.ParseExternalDeclarationList_label_341:
	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	ld          a0, -64(s0)	// Spilled @76
	call        StorageIs

	// *** Basic block 51

	not         t0, a0
	beqz        t0, .ParseExternalDeclarationList_label_355

	// *** Basic block 52

	lb          t0, 56(s3)
	andi        t0, t0, -3
	ori         t0, t0, 2
	sb          t0, 56(s3)

	// *** Basic block 53

.ParseExternalDeclarationList_label_355:

	// *** Basic block 54

.ParseExternalDeclarationList_label_356:

	// *** Basic block 55

.ParseExternalDeclarationList_label_357:

	// *** Basic block 56

.ParseExternalDeclarationList_label_358:
	bne         s3, x0, .ParseExternalDeclarationList_label_375

	// *** Basic block 57

	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	mv          a0, s8
	call        LexMatch

	// *** Basic block 58

	not         t0, a0
	bnez        t0, .ParseExternalDeclarationList_label_550

	// *** Basic block 59

.ParseExternalDeclarationList_label_370:
	mv          a0, s1
	call        TypeParserReset

	// *** Basic block 60

	j           .ParseExternalDeclarationList_label_103

	// *** Basic block 61

.ParseExternalDeclarationList_label_375:
	li          s10, 96		// 0x60 ASCII '`'
	mv          a1, s10
	mv          a0, s8
	call        LexMatch

	// *** Basic block 62

	beqz        a0, .ParseExternalDeclarationList_label_396

	// *** Basic block 63

.ParseExternalDeclarationList_label_383:
	mv          a1, s4
	ld          a0, -56(s0)	// Spilled @88
	call        ParseAttribute

	// *** Basic block 64

	mv          a1, s10
	mv          a0, s8
	call        LexMatch

	// *** Basic block 65

	bnez        a0, .ParseExternalDeclarationList_label_383

	// *** Basic block 66

.ParseExternalDeclarationList_label_396:
	addi        a0, s3, 80
	mv          a1, s4
	call        VectorCopy

	// *** Basic block 67

	mv          a0, s4
	call        VectorClear

	// *** Basic block 68

	ld          s10, 40(s3)
	lw          t0, 16(s10)
	addi        t0, t0, -3
	seqz        s10, t0

	// *** Basic block 69

.ParseExternalDeclarationList_label_413:
	beqz        s10, .ParseExternalDeclarationList_label_438

	// *** Basic block 70

	j           .ParseExternalDeclarationList_label_416

	// *** Basic block 71

.ParseExternalDeclarationList_label_416:
	ld          a3, -64(s0)	// Spilled @119
	mv          a2, s3
	ld          a1, -72(s0)	// Spilled @82
	ld          a0, -56(s0)	// Spilled @88
	call        DeclareOrDefineFunction

	// *** Basic block 72

	mv          s10, a0
	beq         s10, x0, .ParseExternalDeclarationList_label_436

	// *** Basic block 73

	mv          a0, s10

	// *** Basic block 74

.ParseExternalDeclarationList_label_433:
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

	// *** Basic block 75

.ParseExternalDeclarationList_label_436:
	j           .ParseExternalDeclarationList_label_447

	// *** Basic block 76

.ParseExternalDeclarationList_label_438:
	beqz        s11, .ParseExternalDeclarationList_label_446

	// *** Basic block 77

	lla         a1, .str.27
	ld          a0, -56(s0)	// Spilled @88
	call        SyntaxError

	// *** Basic block 78

.ParseExternalDeclarationList_label_446:

	// *** Basic block 79

.ParseExternalDeclarationList_label_447:
	ld          t0, -64(s0)	// Spilled @119
	beq         t0, x0, .ParseExternalDeclarationList_label_455

	// *** Basic block 80

	mv          a0, s3
	call        SymbolDelete

	// *** Basic block 81

	ld          s3, -64(s0)	// Spilled @119

	// *** Basic block 82

.ParseExternalDeclarationList_label_455:
	li          t0, 95		// 0x5f ASCII '_'
	mv          a1, t0
	mv          a0, s8
	call        LexMatch

	// *** Basic block 83

	beqz        a0, .ParseExternalDeclarationList_label_500

	// *** Basic block 84

	li          t0, 32		// 0x20 ASCII ' '
	mv          a2, t0
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	ld          a0, -56(s0)	// Spilled @88
	call        SyntaxNeedBracket

	// *** Basic block 85

	li          s11, 4		// 0x4 ASCII \x4
	mv          a1, s11
	mv          a0, s8
	call        LexLookingAt

	// *** Basic block 86

	beqz        a0, .ParseExternalDeclarationList_label_490

	// *** Basic block 87

.ParseExternalDeclarationList_label_479:
	mv          a0, s8
	call        LexNextToken

	// *** Basic block 88

	mv          a1, s11
	mv          a0, s8
	call        LexLookingAt

	// *** Basic block 89

	bnez        a0, .ParseExternalDeclarationList_label_479

	// *** Basic block 90

.ParseExternalDeclarationList_label_490:
	li          t0, 192		// 0xc0 ASCII \xc0
	mv          a2, t0
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	ld          a0, -56(s0)	// Spilled @88
	call        SyntaxNeedBracket

	// *** Basic block 91

.ParseExternalDeclarationList_label_500:
	mv          s11, x0
	li          t0, 12		// 0xc ASCII \xc
	mv          a1, t0
	mv          a0, s8
	call        LexMatch

	// *** Basic block 92

	beqz        a0, .ParseExternalDeclarationList_label_518

	// *** Basic block 93

	ld          a2, -64(s0)	// Spilled @76
	mv          a1, s3
	ld          a0, -56(s0)	// Spilled @88
	call        ParseInitializer

	// *** Basic block 94

	mv          s11, a0

	// *** Basic block 95

.ParseExternalDeclarationList_label_518:
	mv          a2, s6
	mv          a1, s11
	mv          a0, s3
	call        NewVariableDeclarationASTNode

	// *** Basic block 96

	mv          s6, a0
	mv          a1, s6
	ld          a0, -72(s0)	// Spilled @82
	call        VectorAppend

	// *** Basic block 97

	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	mv          a0, s8
	call        LexMatch

	// *** Basic block 98

	not         t0, a0
	bnez        t0, .ParseExternalDeclarationList_label_550

	// *** Basic block 99

.ParseExternalDeclarationList_label_541:
	mv          a0, s1
	call        TypeParserReset

	// *** Basic block 100

	mv          a0, s7
	call        LexEof

	// *** Basic block 101

	not         t0, a0
	bnez        t0, .ParseExternalDeclarationList_label_103

	// *** Basic block 102

.ParseExternalDeclarationList_label_550:
	sub         t0, s2, x0
	snez        s7, t0
	beq         s2, x0, .ParseExternalDeclarationList_label_566

	// *** Basic block 103

	mv          a0, s2
	call        TypeIsEnum

	// *** Basic block 104

	mv          s7, a0
	bnez        a0, .ParseExternalDeclarationList_label_565

	// *** Basic block 105

	mv          a0, s2
	call        TypeIsStructOrUnion

	// *** Basic block 107

.ParseExternalDeclarationList_label_565:

	// *** Basic block 108

.ParseExternalDeclarationList_label_566:
	beqz        s7, .ParseExternalDeclarationList_label_581

	// *** Basic block 109

	la          t0, compiler
	ld          t0, 0(t0)
	lb          t0, 1228(t0)
	beqz        t0, .ParseExternalDeclarationList_label_580

	// *** Basic block 110

	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 608
	mv          a1, s2
	call        BuildTypeDebugInfo

	// *** Basic block 111

.ParseExternalDeclarationList_label_580:

	// *** Basic block 112

.ParseExternalDeclarationList_label_581:
	mv          a0, x0
	j           .ParseExternalDeclarationList_label_433
.func_end_ParseExternalDeclarationList:
	.size ParseExternalDeclarationList, .func_end_ParseExternalDeclarationList-ParseExternalDeclarationList

	.global SyntaxParseExternalDeclaration
	.type SyntaxParseExternalDeclaration, @function

SyntaxParseExternalDeclaration:

	// *** Basic block 0

	.global NewVector
	.global LexMatch
	.global ParseAttribute
	.local ParseDeclarationSpecifier
	.global StorageIs
	.global SyntaxError
	.global TypeParserInit
	.global SyntaxOpenScope
	.local ParseExternalDeclarationList
	.global SyntaxCloseScope
	.global ASTNodeDelete
	.global VectorDelete
	.global VectorDestruct
	.global SyntaxNeedSemicolon
	.global NewDeclarationListASTNode
	addi sp, sp, -208
	// Saved return address (offset 200) and frame pointer (offset 192)
	sd ra, 200(sp)
	sd s0, 192(sp)
	addi s0, sp, 208
	// Local vars at offset -144(s0)
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
	call        NewVector

	// *** Basic block 1

	mv          s2, a0
	sd          x0, -144(s0)
	sd          x0, -136(s0)
	sd          x0, -128(s0)
	sd          x0, -144(s0)
	sw          x0, 152(s1)
	ld          s3, 0(s1)
	li          s4, 96		// 0x60 ASCII '`'
	mv          a1, s4
	mv          a0, s3
	call        LexMatch

	// *** Basic block 2

	beqz        a0, .SyntaxParseExternalDeclaration_label_71

	// *** Basic block 3

.SyntaxParseExternalDeclaration_label_58:
	addi        a1, s0, -144
	mv          a0, s1
	call        ParseAttribute

	// *** Basic block 4

	mv          a1, s4
	mv          a0, s3
	call        LexMatch

	// *** Basic block 5

	bnez        a0, .SyntaxParseExternalDeclaration_label_58

	// *** Basic block 6

.SyntaxParseExternalDeclaration_label_71:
	sw          x0, -120(s0)
	sb          x0, -116(s0)
	sd          x0, -112(s0)
	addi        a1, s0, -120
	addi        a2, s0, -116
	addi        a3, s0, -112
	addi        a4, s0, -144
	mv          a5, x0
	mv          a0, s1
	call        ParseDeclarationSpecifier

	// *** Basic block 7

	lw          s5, -120(s0)
	li          t0, 17		// 0x11 ASCII \x11
	mv          a1, t0
	mv          a0, s5
	call        StorageIs

	// *** Basic block 8

	beqz        a0, .SyntaxParseExternalDeclaration_label_128

	// *** Basic block 9

	lla         s6, .str.28
	li          t0, 16		// 0x10 ASCII \x10
	mv          a1, t0
	mv          a0, s5
	call        StorageIs

	// *** Basic block 10

	beqz        a0, .SyntaxParseExternalDeclaration_label_117

	// *** Basic block 11

	lla         a2, .str.29
	j           .SyntaxParseExternalDeclaration_label_120

	// *** Basic block 12

.SyntaxParseExternalDeclaration_label_117:
	lla         a2, .str.30

	// *** Basic block 13

.SyntaxParseExternalDeclaration_label_120:
	mv          a1, s6
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 14

	sw          x0, -120(s0)

	// *** Basic block 15

.SyntaxParseExternalDeclaration_label_128:
	mv          a1, s4
	mv          a0, s3
	call        LexMatch

	// *** Basic block 16

	beqz        a0, .SyntaxParseExternalDeclaration_label_149

	// *** Basic block 17

.SyntaxParseExternalDeclaration_label_136:
	addi        a1, s0, -144
	mv          a0, s1
	call        ParseAttribute

	// *** Basic block 18

	mv          a1, s4
	mv          a0, s3
	call        LexMatch

	// *** Basic block 19

	bnez        a0, .SyntaxParseExternalDeclaration_label_136

	// *** Basic block 20

.SyntaxParseExternalDeclaration_label_149:
	addi        a0, s0, -104
	lw          s4, -120(s0)
	mv          a4, x0
	mv          a3, s4
	mv          a2, s1
	mv          a1, s3
	call        TypeParserInit

	// *** Basic block 21

	addi        t0, s0, -104
	lb          t1, -116(s0)
	sb          t1, 68(t0)
	mv          a0, s1
	call        SyntaxOpenScope

	// *** Basic block 22

	addi        a0, s0, -104
	ld          a1, -112(s0)
	addi        a3, s0, -144
	mv          a4, s2
	mv          a2, s4
	call        ParseExternalDeclarationList

	// *** Basic block 23

	ld          s4, 0(s2)
	mv          s5, a0
	mv          a0, s1
	call        SyntaxCloseScope

	// *** Basic block 24

	beq         s5, x0, .SyntaxParseExternalDeclaration_label_231

	// *** Basic block 25

	ld          s6, 8(s2)
	li          t0, 1		// 0x1 ASCII \x1
	beq         s6, t0, .SyntaxParseExternalDeclaration_label_222

	// *** Basic block 26

	lla         a1, .str.31
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 27

	mv          s7, x0
	bge         x0, s6, .SyntaxParseExternalDeclaration_label_218

	// *** Basic block 28

.SyntaxParseExternalDeclaration_label_207:
	slli        t0, s7, 3
	add         t0, s4, t0
	ld          a0, 0(t0)
	call        ASTNodeDelete

	// *** Basic block 29

.SyntaxParseExternalDeclaration_label_214:
	addi        s7, s7, 1
	bge         s7, s6, .SyntaxParseExternalDeclaration_label_207

	// *** Basic block 30

.SyntaxParseExternalDeclaration_label_218:
	mv          a0, s2
	call        VectorDelete

	// *** Basic block 31

.SyntaxParseExternalDeclaration_label_222:
	addi        a0, s0, -144
	call        VectorDestruct

	// *** Basic block 32

	mv          a0, s5

	// *** Basic block 33

.SyntaxParseExternalDeclaration_label_228:
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

	// *** Basic block 34

.SyntaxParseExternalDeclaration_label_231:
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedSemicolon

	// *** Basic block 35

	addi        a0, s0, -144
	call        VectorDestruct

	// *** Basic block 36

	ld          a1, 56(s3)
	mv          a0, s2
	call        NewDeclarationListASTNode

	// *** Basic block 37

	j           .SyntaxParseExternalDeclaration_label_228
.func_end_SyntaxParseExternalDeclaration:
	.size SyntaxParseExternalDeclaration, .func_end_SyntaxParseExternalDeclaration-SyntaxParseExternalDeclaration

	.local  SkipFunctionBody
	.type SkipFunctionBody, @function

SkipFunctionBody:

	// *** Basic block 0

	.global LexEof
	.global LexLookingAt
	.global LexNextToken
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
	mv          t0, a0
	ld          s1, 0(t0)
	li          s2, 1		// 0x1 ASCII \x1
	li          s3, 1		// 0x1 ASCII \x1
	mv          a0, s1
	call        LexEof

	// *** Basic block 1

	not         s3, a0

	// *** Basic block 2

.SkipFunctionBody_label_24:
	beqz        s3, .SkipFunctionBody_label_61

	// *** Basic block 3

.SkipFunctionBody_label_26:
	li          t0, 24		// 0x18 ASCII \x18
	mv          a1, t0
	mv          a0, s1
	call        LexLookingAt

	// *** Basic block 4

	beqz        a0, .SkipFunctionBody_label_37

	// *** Basic block 5

	addi        s2, s2, 1
	j           .SkipFunctionBody_label_48

	// *** Basic block 6

.SkipFunctionBody_label_37:
	li          t0, 44		// 0x2c ASCII ','
	mv          a1, t0
	mv          a0, s1
	call        LexLookingAt

	// *** Basic block 7

	beqz        a0, .SkipFunctionBody_label_47

	// *** Basic block 8

	addi        s2, s2, -1

	// *** Basic block 9

.SkipFunctionBody_label_47:

	// *** Basic block 10

.SkipFunctionBody_label_48:
	mv          a0, s1
	call        LexNextToken

	// *** Basic block 11

	slt         s3, x0, s2
	bge         x0, s2, .SkipFunctionBody_label_59

	// *** Basic block 12

	mv          a0, s1
	call        LexEof

	// *** Basic block 13

	not         s3, a0

	// *** Basic block 14

.SkipFunctionBody_label_59:
	bnez        s3, .SkipFunctionBody_label_26

	// *** Basic block 15

.SkipFunctionBody_label_61:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SkipFunctionBody:
	.size SkipFunctionBody, .func_end_SkipFunctionBody-SkipFunctionBody

	.local  ParseLocalDeclarationList
	.type ParseLocalDeclarationList, @function

ParseLocalDeclarationList:

	// *** Basic block 0

	.global LexEof
	.global LexLookingAt
	.global TypeParserParseDeclarator
	.global FindTopLocalSymbol
	.global StorageIs
	.global SyntaxError
	.local IsDefinition
	.global TypeEqual
	.global TypeErrorDetails
	.global DecodeSourceLocation
	.global ReportNote
	.global SymbolDelete
	.global SyntaxAddSymbol
	.global printf
	.global abort
	.global LexMatch
	.global TypeParserReset
	.global VectorCopy
	.global VectorClear
	.local CheckThreadLocal
	.local SkipFunctionBody
	.local ParseInitializer
	.global NewIdentifierASTNode
	.global NewBinaryASTNode
	.global NewVariableDeclarationASTNode
	.global VectorAppend
	.global SemanticAnalyzeVariableDefinition
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
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	mv          s4, a3
	mv          s5, a4
	ld          s6, 8(s1)
	ld          a0, 0(s6)
	call        LexEof

	// *** Basic block 1

	ld          s7, 0(s1)
	lb          s8, 68(s1)
	not         t0, a0
	beqz        t0, .ParseLocalDeclarationList_label_524

	// *** Basic block 2

.ParseLocalDeclarationList_label_88:
	ld          s9, 0(s6)
	li          t0, 49		// 0x31 ASCII '1'
	mv          a1, t0
	mv          a0, s9
	call        LexLookingAt

	// *** Basic block 3

	bnez        a0, .ParseLocalDeclarationList_label_524

	// *** Basic block 4

.ParseLocalDeclarationList_label_97:
	mv          a1, s2
	mv          a0, s1
	call        TypeParserParseDeclarator

	// *** Basic block 5

	mv          s10, a0
	sd          s10, -56(s0)	// Spilled @103
	beq         s10, x0, .ParseLocalDeclarationList_label_320

	// *** Basic block 6

	ld          a0, 16(s6)
	mv          a1, s10
	call        FindTopLocalSymbol

	// *** Basic block 7

	mv          s2, a0
	li          s11, 1		// 0x1 ASCII \x1
	beq         s2, x0, .ParseLocalDeclarationList_label_292

	// *** Basic block 8

	lb          t0, 56(s2)
	slli        t0, t0, 63
	srai        t0, t0, 63
	beqz        t0, .ParseLocalDeclarationList_label_174

	// *** Basic block 9

	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	mv          a0, s3
	call        StorageIs

	// *** Basic block 10

	beqz        a0, .ParseLocalDeclarationList_label_152

	// *** Basic block 11

	li          t0, 12		// 0xc ASCII \xc
	mv          a1, t0
	mv          a0, s7
	call        LexLookingAt

	// *** Basic block 12

	beqz        a0, .ParseLocalDeclarationList_label_150

	// *** Basic block 13

	lla         a1, .str.32
	ld          a2, 16(s10)
	mv          a0, s6
	call        SyntaxError

	// *** Basic block 14

	mv          s11, x0

	// *** Basic block 15

.ParseLocalDeclarationList_label_150:
	j           .ParseLocalDeclarationList_label_172

	// *** Basic block 16

.ParseLocalDeclarationList_label_152:
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        IsDefinition

	// *** Basic block 17

	beqz        a0, .ParseLocalDeclarationList_label_171

	// *** Basic block 18

	lla         a1, .str.33
	ld          a2, 16(s10)
	mv          a0, s6
	call        SyntaxError

	// *** Basic block 19

	mv          s11, x0

	// *** Basic block 20

.ParseLocalDeclarationList_label_171:

	// *** Basic block 21

.ParseLocalDeclarationList_label_172:
	j           .ParseLocalDeclarationList_label_205

	// *** Basic block 22

.ParseLocalDeclarationList_label_174:
	mv          a2, s3
	mv          a1, s10
	mv          a0, s1
	call        IsDefinition

	// *** Basic block 23

	beqz        a0, .ParseLocalDeclarationList_label_189

	// *** Basic block 24

	lb          t0, 56(s2)
	andi        t0, t0, -2
	ori         t0, t0, 1
	sb          t0, 56(s2)
	j           .ParseLocalDeclarationList_label_204

	// *** Basic block 25

.ParseLocalDeclarationList_label_189:
	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	mv          a0, s3
	call        StorageIs

	// *** Basic block 26

	not         t0, a0
	beqz        t0, .ParseLocalDeclarationList_label_203

	// *** Basic block 27

	lb          t0, 56(s2)
	andi        t0, t0, -3
	ori         t0, t0, 2
	sb          t0, 56(s2)

	// *** Basic block 28

.ParseLocalDeclarationList_label_203:

	// *** Basic block 29

.ParseLocalDeclarationList_label_204:

	// *** Basic block 30

.ParseLocalDeclarationList_label_205:
	ld          s7, 40(s10)
	ld          s10, 40(s2)
	mv          a1, s10
	mv          a0, s7
	call        TypeEqual

	// *** Basic block 31

	not         t0, a0
	beqz        t0, .ParseLocalDeclarationList_label_261

	// *** Basic block 32

	lla         a1, .str.34
	ld          t0, -56(s0)	// Spilled @103
	ld          a2, 16(t0)
	mv          a0, s6
	call        SyntaxError

	// *** Basic block 33

	ld          a0, 56(s9)
	mv          a2, s10
	mv          a1, s7
	call        TypeErrorDetails

	// *** Basic block 34

	ld          a0, 104(s2)
	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	call        DecodeSourceLocation

	// *** Basic block 35

	ld          a0, -48(s0)
	lw          a1, -40(s0)
	lla         a2, .str.35
	call        ReportNote

	// *** Basic block 36

	mv          s11, x0
	j           .ParseLocalDeclarationList_label_284

	// *** Basic block 37

.ParseLocalDeclarationList_label_261:
	lw          t0, 48(s2)
	andi        s7, t0, -9
	ld          t0, -56(s0)	// Spilled @103
	lw          t1, 48(t0)
	andi        s10, t1, -9
	beq         s7, s10, .ParseLocalDeclarationList_label_283

	// *** Basic block 38

	lla         a1, .str.36
	ld          t0, -56(s0)	// Spilled @103
	ld          a2, 16(t0)
	mv          a0, s6
	call        SyntaxError

	// *** Basic block 39

	mv          s11, x0

	// *** Basic block 40

.ParseLocalDeclarationList_label_283:

	// *** Basic block 41

.ParseLocalDeclarationList_label_284:
	beqz        s11, .ParseLocalDeclarationList_label_290

	// *** Basic block 42

	ld          a0, -56(s0)	// Spilled @103
	call        SymbolDelete

	// *** Basic block 43

	mv          s10, s2

	// *** Basic block 44

.ParseLocalDeclarationList_label_290:
	j           .ParseLocalDeclarationList_label_319

	// *** Basic block 45

.ParseLocalDeclarationList_label_292:
	ld          a1, -56(s0)	// Spilled @103
	mv          a0, s6
	call        SyntaxAddSymbol

	// *** Basic block 46

	mv          s7, a0
	beqz        s7, .ParseLocalDeclarationList_label_303

	// *** Basic block 47

	j           .ParseLocalDeclarationList_label_318

	// *** Basic block 48

.ParseLocalDeclarationList_label_303:
	lla         a0, .str.37
	lla         a1, .str.38
	lla         a3, .str.39
	li          t0, 792		// 0x318
	mv          a2, t0
	call        printf

	// *** Basic block 49

	call        abort

	// *** Basic block 50

.ParseLocalDeclarationList_label_318:

	// *** Basic block 51

.ParseLocalDeclarationList_label_319:

	// *** Basic block 52

.ParseLocalDeclarationList_label_320:
	ld          t0, -56(s0)	// Spilled @103
	bne         t0, x0, .ParseLocalDeclarationList_label_337

	// *** Basic block 53

	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	mv          a0, s9
	call        LexMatch

	// *** Basic block 54

	not         t0, a0
	bnez        t0, .ParseLocalDeclarationList_label_524

	// *** Basic block 55

.ParseLocalDeclarationList_label_332:
	mv          a0, s1
	call        TypeParserReset

	// *** Basic block 56

	j           .ParseLocalDeclarationList_label_88

	// *** Basic block 57

.ParseLocalDeclarationList_label_337:
	ld          t0, -56(s0)	// Spilled @103
	lw          a0, 48(t0)
	li          t1, 8		// 0x8 ASCII \x8
	mv          a1, t1
	call        StorageIs

	// *** Basic block 58

	not         t0, a0
	beqz        t0, .ParseLocalDeclarationList_label_352

	// *** Basic block 59

	ld          t0, -56(s0)	// Spilled @103
	lb          t1, 56(t0)
	andi        t1, t1, -9
	ori         t1, t1, 8
	ld          t2, -56(s0)	// Spilled @103
	sb          t1, 56(t2)

	// *** Basic block 60

.ParseLocalDeclarationList_label_352:
	ld          t0, -56(s0)	// Spilled @103
	addi        a0, t0, 80
	mv          a1, s4
	call        VectorCopy

	// *** Basic block 61

	mv          a0, s4
	call        VectorClear

	// *** Basic block 62

	ld          a1, -56(s0)	// Spilled @103
	mv          a0, s6
	call        CheckThreadLocal

	// *** Basic block 63

	ld          t0, -56(s0)	// Spilled @103
	ld          s10, 40(t0)
	lw          t1, 16(s10)
	addi        t1, t1, -3
	seqz        s10, t1

	// *** Basic block 64

.ParseLocalDeclarationList_label_374:
	beqz        s10, .ParseLocalDeclarationList_label_396

	// *** Basic block 65

	j           .ParseLocalDeclarationList_label_377

	// *** Basic block 66

.ParseLocalDeclarationList_label_377:
	li          t0, 24		// 0x18 ASCII \x18
	mv          a1, t0
	mv          a0, s9
	call        LexMatch

	// *** Basic block 67

	beqz        a0, .ParseLocalDeclarationList_label_394

	// *** Basic block 68

	lla         a1, .str.40
	mv          a0, s6
	call        SyntaxError

	// *** Basic block 69

	mv          a0, s6
	call        SkipFunctionBody

	// *** Basic block 70

.ParseLocalDeclarationList_label_394:
	j           .ParseLocalDeclarationList_label_505

	// *** Basic block 71

.ParseLocalDeclarationList_label_396:
	ld          t0, -56(s0)	// Spilled @103
	lb          t1, 56(t0)
	andi        t1, t1, -2
	ori         t1, t1, 1
	ld          t2, -56(s0)	// Spilled @103
	sb          t1, 56(t2)
	beqz        s8, .ParseLocalDeclarationList_label_409

	// *** Basic block 72

	lla         a1, .str.41
	mv          a0, s6
	call        SyntaxError

	// *** Basic block 73

.ParseLocalDeclarationList_label_409:
	mv          s4, x0
	li          t0, 12		// 0xc ASCII \xc
	mv          a1, t0
	mv          a0, s9
	call        LexMatch

	// *** Basic block 74

	beqz        a0, .ParseLocalDeclarationList_label_458

	// *** Basic block 75

	mv          a2, s3
	ld          a1, -56(s0)	// Spilled @103
	mv          a0, s6
	call        ParseInitializer

	// *** Basic block 76

	mv          s4, a0
	ld          s8, 56(s9)
	mv          a1, s8
	ld          a0, -56(s0)	// Spilled @103
	call        NewIdentifierASTNode

	// *** Basic block 77

	mv          s10, a0
	lw          t0, 8(s10)
	ori         t0, t0, 1
	sw          t0, 8(s10)
	ld          t0, -56(s0)	// Spilled @103
	ld          a1, 40(t0)
	mv          a4, s4
	mv          a3, s10
	mv          a2, s8
	li          t1, 86		// 0x56 ASCII 'V'
	mv          a0, t1
	call        NewBinaryASTNode

	// *** Basic block 78

	mv          s4, a0
	lw          t0, 8(s10)
	ori         t0, t0, 16
	sw          t0, 8(s10)

	// *** Basic block 79

.ParseLocalDeclarationList_label_458:
	ld          a2, 56(s9)
	mv          a1, s4
	ld          a0, -56(s0)	// Spilled @103
	call        NewVariableDeclarationASTNode

	// *** Basic block 80

	mv          s8, a0
	mv          a1, s8
	mv          a0, s5
	call        VectorAppend

	// *** Basic block 81

	ld          t0, -56(s0)	// Spilled @103
	ld          s9, 40(t0)
	lw          t1, 12(s9)
	andi        t1, t1, 4
	snez        s9, t1

	// *** Basic block 82

.ParseLocalDeclarationList_label_482:
	beqz        s9, .ParseLocalDeclarationList_label_491

	// *** Basic block 83

	j           .ParseLocalDeclarationList_label_485

	// *** Basic block 84

.ParseLocalDeclarationList_label_485:
	mv          a1, s8
	mv          a0, s6
	call        SemanticAnalyzeVariableDefinition

	// *** Basic block 85

.ParseLocalDeclarationList_label_491:
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s3
	call        StorageIs

	// *** Basic block 86

	beqz        a0, .ParseLocalDeclarationList_label_504

	// *** Basic block 87

	addi        a0, s6, 104
	mv          a1, s8
	call        VectorAppend

	// *** Basic block 88

.ParseLocalDeclarationList_label_504:

	// *** Basic block 89

.ParseLocalDeclarationList_label_505:
	ld          s6, 0(s6)
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	mv          a0, s6
	call        LexMatch

	// *** Basic block 90

	not         t0, a0
	bnez        t0, .ParseLocalDeclarationList_label_524

	// *** Basic block 91

.ParseLocalDeclarationList_label_515:
	mv          a0, s1
	call        TypeParserReset

	// *** Basic block 92

	mv          a0, s6
	call        LexEof

	// *** Basic block 93

	not         t0, a0
	bnez        t0, .ParseLocalDeclarationList_label_88

	// *** Basic block 94

.ParseLocalDeclarationList_label_524:
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
.func_end_ParseLocalDeclarationList:
	.size ParseLocalDeclarationList, .func_end_ParseLocalDeclarationList-ParseLocalDeclarationList

	.global SyntaxParseLocalDeclaration
	.type SyntaxParseLocalDeclaration, @function

SyntaxParseLocalDeclaration:

	// *** Basic block 0

	.global NewVector
	.local ParseDeclarationSpecifier
	.global SyntaxError
	.global TypeParserInit
	.local ParseLocalDeclarationList
	.global SyntaxNeedSemicolon
	.global VectorDestruct
	.global NewDeclarationListASTNode
	addi sp, sp, -192
	// Saved return address (offset 184) and frame pointer (offset 176)
	sd ra, 184(sp)
	sd s0, 176(sp)
	addi s0, sp, 192
	// Local vars at offset -144(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// End of stack frame
	mv          s1, a0
	call        NewVector

	// *** Basic block 1

	mv          s2, a0
	sw          x0, -144(s0)
	sb          x0, -140(s0)
	sd          x0, -136(s0)
	sd          x0, -128(s0)
	sd          x0, -120(s0)
	sd          x0, -112(s0)
	sd          x0, -128(s0)
	li          s3, 1		// 0x1 ASCII \x1
	sw          s3, 152(s1)
	addi        a1, s0, -144
	addi        a2, s0, -140
	addi        a3, s0, -136
	addi        a4, s0, -128
	mv          a5, s3
	mv          a0, s1
	call        ParseDeclarationSpecifier

	// *** Basic block 2

	lb          t0, -140(s0)
	beqz        t0, .SyntaxParseLocalDeclaration_label_67

	// *** Basic block 3

	lla         a1, .str.42
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 4

.SyntaxParseLocalDeclaration_label_67:
	addi        a0, s0, -104
	ld          s4, 0(s1)
	lw          s5, -144(s0)
	mv          a4, s3
	mv          a3, s5
	mv          a2, s1
	mv          a1, s4
	call        TypeParserInit

	// *** Basic block 5

	addi        a0, s0, -104
	ld          a1, -136(s0)
	addi        a3, s0, -128
	mv          a4, s2
	mv          a2, s5
	call        ParseLocalDeclarationList

	// *** Basic block 6

	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s1
	call        SyntaxNeedSemicolon

	// *** Basic block 7

	addi        a0, s0, -128
	call        VectorDestruct

	// *** Basic block 8

	ld          a1, 56(s4)
	mv          a0, s2
	call        NewDeclarationListASTNode

	// *** Basic block 9


	// *** Basic block 10

.SyntaxParseLocalDeclaration_label_111:
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
.func_end_SyntaxParseLocalDeclaration:
	.size SyntaxParseLocalDeclaration, .func_end_SyntaxParseLocalDeclaration-SyntaxParseLocalDeclaration

	.global SyntaxNeedBracket
	.type SyntaxNeedBracket, @function

SyntaxNeedBracket:

	// *** Basic block 0

	.global LexMatch
	.global SyntaxError
	.global TokenName
	.global SyntaxRecover
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
	ld          a0, 0(s1)
	call        LexMatch

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .SyntaxNeedBracket_label_43

	// *** Basic block 2

	lla         s4, .str.43
	mv          a0, s2
	call        TokenName

	// *** Basic block 3

	mv          a2, a0
	mv          a1, s4
	mv          a0, s1
	call        SyntaxError

	// *** Basic block 4

	mv          a1, s3
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SyntaxRecover

	// *** Basic block 5

.SyntaxNeedBracket_label_43:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SyntaxNeedBracket:
	.size SyntaxNeedBracket, .func_end_SyntaxNeedBracket-SyntaxNeedBracket

	.global SyntaxRecover
	.type SyntaxRecover, @function

SyntaxRecover:

	// *** Basic block 0

	.global LexEof
	.global ClassifyToken
	.global LexNextToken
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
	mv          t0, a0
	mv          s1, a1
	ld          s2, 0(t0)
	mv          a0, s2
	call        LexEof

	// *** Basic block 1

	lw          s3, 72(s2)
	not         t0, a0
	beqz        t0, .SyntaxRecover_label_40

	// *** Basic block 2

.SyntaxRecover_label_22:
	mv          a0, s3
	call        ClassifyToken

	// *** Basic block 3

	mv          s3, a0
	and         t0, s3, s1
	bnez        t0, .SyntaxRecover_label_40

	// *** Basic block 4

.SyntaxRecover_label_31:
	mv          a0, s2
	call        LexNextToken

	// *** Basic block 5

	mv          a0, s2
	call        LexEof

	// *** Basic block 6

	not         t0, a0
	bnez        t0, .SyntaxRecover_label_22

	// *** Basic block 7

.SyntaxRecover_label_40:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SyntaxRecover:
	.size SyntaxRecover, .func_end_SyntaxRecover-SyntaxRecover

	.global SyntaxLookingAtType
	.type SyntaxLookingAtType, @function

SyntaxLookingAtType:

	// *** Basic block 0

	.global SyntaxFindSymbol
	.global StorageIs
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
	ld          t0, 0(s1)
	lw          s2, 72(t0)
	li          s3, 77		// 0x4d ASCII 'M'
	blt         s2, s3, .SyntaxLookingAtType_label_83

	// *** Basic block 1

	beq         s2, s3, .SyntaxLookingAtType_label_128

	// *** Basic block 2

	li          t0, 79		// 0x4f ASCII 'O'
	beq         s2, t0, .SyntaxLookingAtType_label_139

	// *** Basic block 3

	li          t0, 81		// 0x51 ASCII 'Q'
	beq         s2, t0, .SyntaxLookingAtType_label_127

	// *** Basic block 4

	li          t0, 82		// 0x52 ASCII 'R'
	beq         s2, t0, .SyntaxLookingAtType_label_135

	// *** Basic block 5

	li          t0, 85		// 0x55 ASCII 'U'
	beq         s2, t0, .SyntaxLookingAtType_label_131

	// *** Basic block 6

	li          t0, 89		// 0x59 ASCII 'Y'
	beq         s2, t0, .SyntaxLookingAtType_label_132

	// *** Basic block 7

	li          t0, 90		// 0x5a ASCII 'Z'
	beq         s2, t0, .SyntaxLookingAtType_label_136

	// *** Basic block 8

	li          t0, 91		// 0x5b ASCII '['
	beq         s2, t0, .SyntaxLookingAtType_label_140

	// *** Basic block 9

	li          t0, 92		// 0x5c ASCII '\'
	beq         s2, t0, .SyntaxLookingAtType_label_138

	// *** Basic block 10

	j           .SyntaxLookingAtType_label_178

	// *** Basic block 11

.SyntaxLookingAtType_label_83:
	li          t0, 3		// 0x3 ASCII \x3
	beq         s2, t0, .SyntaxLookingAtType_label_146

	// *** Basic block 12

	li          t0, 57		// 0x39 ASCII '9'
	beq         s2, t0, .SyntaxLookingAtType_label_134

	// *** Basic block 13

	li          t0, 59		// 0x3b ASCII ';'
	beq         s2, t0, .SyntaxLookingAtType_label_125

	// *** Basic block 14

	li          t0, 61		// 0x3d ASCII '='
	beq         s2, t0, .SyntaxLookingAtType_label_137

	// *** Basic block 15

	li          t0, 65		// 0x41 ASCII 'A'
	beq         s2, t0, .SyntaxLookingAtType_label_130

	// *** Basic block 16

	li          t0, 67		// 0x43 ASCII 'C'
	beq         s2, t0, .SyntaxLookingAtType_label_133

	// *** Basic block 17

	li          t0, 70		// 0x46 ASCII 'F'
	beq         s2, t0, .SyntaxLookingAtType_label_129

	// *** Basic block 18

	li          t0, 76		// 0x4c ASCII 'L'
	beq         s2, t0, .SyntaxLookingAtType_label_126

	// *** Basic block 19

	j           .SyntaxLookingAtType_label_178

	// *** Basic block 20

.SyntaxLookingAtType_label_125:

	// *** Basic block 21

.SyntaxLookingAtType_label_126:

	// *** Basic block 22

.SyntaxLookingAtType_label_127:

	// *** Basic block 23

.SyntaxLookingAtType_label_128:

	// *** Basic block 24

.SyntaxLookingAtType_label_129:

	// *** Basic block 25

.SyntaxLookingAtType_label_130:

	// *** Basic block 26

.SyntaxLookingAtType_label_131:

	// *** Basic block 27

.SyntaxLookingAtType_label_132:

	// *** Basic block 28

.SyntaxLookingAtType_label_133:

	// *** Basic block 29

.SyntaxLookingAtType_label_134:

	// *** Basic block 30

.SyntaxLookingAtType_label_135:

	// *** Basic block 31

.SyntaxLookingAtType_label_136:

	// *** Basic block 32

.SyntaxLookingAtType_label_137:

	// *** Basic block 33

.SyntaxLookingAtType_label_138:

	// *** Basic block 34

.SyntaxLookingAtType_label_139:

	// *** Basic block 35

.SyntaxLookingAtType_label_140:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 36

.SyntaxLookingAtType_label_143:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 37

.SyntaxLookingAtType_label_146:
	ld          t0, 0(s1)
	addi        a1, t0, 80
	mv          a0, s1
	call        SyntaxFindSymbol

	// *** Basic block 38

	mv          s2, a0
	bne         s2, x0, .SyntaxLookingAtType_label_162

	// *** Basic block 39

	mv          a0, x0
	j           .SyntaxLookingAtType_label_143

	// *** Basic block 40

.SyntaxLookingAtType_label_162:
	lw          a0, 48(s2)
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	call        StorageIs

	// *** Basic block 41

	beqz        a0, .SyntaxLookingAtType_label_174

	// *** Basic block 42

	li          a0, 1		// 0x1 ASCII \x1
	j           .SyntaxLookingAtType_label_143

	// *** Basic block 43

.SyntaxLookingAtType_label_174:
	mv          a0, x0
	j           .SyntaxLookingAtType_label_143

	// *** Basic block 44

.SyntaxLookingAtType_label_178:
	mv          a0, x0
	j           .SyntaxLookingAtType_label_143
.func_end_SyntaxLookingAtType:
	.size SyntaxLookingAtType, .func_end_SyntaxLookingAtType-SyntaxLookingAtType

	.global SyntaxLookingAtDeclaration
	.type SyntaxLookingAtDeclaration, @function

SyntaxLookingAtDeclaration:

	// *** Basic block 0

	.global SyntaxLookingAtType
	addi sp, sp, -16
	// Saved return address (offset 8) and frame pointer (offset 0)
	sd ra, 8(sp)
	sd s0, 0(sp)
	addi s0, sp, 16
	// Local vars at offset -16(s0)
	// End of stack frame
	mv          t0, a0
	ld          t1, 0(t0)
	lw          t1, 72(t1)
	li          t2, 55		// 0x37 ASCII '7'
	beq         t1, t2, .SyntaxLookingAtDeclaration_label_53

	// *** Basic block 1

	li          t2, 68		// 0x44 ASCII 'D'
	beq         t1, t2, .SyntaxLookingAtDeclaration_label_51

	// *** Basic block 2

	li          t2, 78		// 0x4e ASCII 'N'
	beq         t1, t2, .SyntaxLookingAtDeclaration_label_54

	// *** Basic block 3

	li          t2, 84		// 0x54 ASCII 'T'
	beq         t1, t2, .SyntaxLookingAtDeclaration_label_52

	// *** Basic block 4

	li          t2, 87		// 0x57 ASCII 'W'
	beq         t1, t2, .SyntaxLookingAtDeclaration_label_55

	// *** Basic block 5

.SyntaxLookingAtDeclaration_label_43:
	mv          a0, t0
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SyntaxLookingAtType

	// *** Basic block 7

.SyntaxLookingAtDeclaration_label_51:

	// *** Basic block 8

.SyntaxLookingAtDeclaration_label_52:

	// *** Basic block 9

.SyntaxLookingAtDeclaration_label_53:

	// *** Basic block 10

.SyntaxLookingAtDeclaration_label_54:

	// *** Basic block 11

.SyntaxLookingAtDeclaration_label_55:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 12

.SyntaxLookingAtDeclaration_label_58:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SyntaxLookingAtDeclaration:
	.size SyntaxLookingAtDeclaration, .func_end_SyntaxLookingAtDeclaration-SyntaxLookingAtDeclaration

	.global SyntaxOpenScope
	.type SyntaxOpenScope, @function

SyntaxOpenScope:

	// *** Basic block 0

	.global NewLocalSymbolTable
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
	call        NewLocalSymbolTable

	// *** Basic block 1

	mv          s2, a0
	ld          t0, 16(s1)
	sd          t0, 40(s2)
	sd          s2, 16(s1)
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewLocalSymbolTable
.func_end_SyntaxOpenScope:
	.size SyntaxOpenScope, .func_end_SyntaxOpenScope-SyntaxOpenScope

	.global SyntaxCloseScope
	.type SyntaxCloseScope, @function

SyntaxCloseScope:

	// *** Basic block 0

	.global LocalSymbolTableDelete
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
	ld          a0, 16(s1)
	ld          s2, 40(a0)
	call        LocalSymbolTableDelete

	// *** Basic block 1

	sd          s2, 16(s1)
	ld          a0, 24(s1)
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LocalSymbolTableDelete
.func_end_SyntaxCloseScope:
	.size SyntaxCloseScope, .func_end_SyntaxCloseScope-SyntaxCloseScope

	.global SyntaxNewTemporary
	.type SyntaxNewTemporary, @function

SyntaxNewTemporary:

	// *** Basic block 0

	.global NewSymbol
	.global SyntaxFakeName
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
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	call        SyntaxFakeName

	// *** Basic block 1

	mv          a2, x0
	mv          a1, s2
	call        NewSymbol

	// *** Basic block 2

	mv          s3, a0
	lb          t0, 56(s3)
	andi        t0, t0, -33
	ori         t0, t0, 32
	sb          t0, 56(s3)
	addi        t0, s3, 56
	lb          t1, 1(t0)
	andi        t1, t1, -2
	ori         t1, t1, 1
	sb          t1, 1(t0)
	mv          a1, s3
	mv          a0, s1
	call        SyntaxAddSymbol

	// *** Basic block 3

	mv          a0, s3

	// *** Basic block 4

.SyntaxNewTemporary_label_49:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SyntaxNewTemporary:
	.size SyntaxNewTemporary, .func_end_SyntaxNewTemporary-SyntaxNewTemporary

	.global ClassifyToken
	.type ClassifyToken, @function

ClassifyToken:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	blt         t0, x0, .ClassifyToken_label_272

	// *** Basic block 1

	li          t1, 97		// 0x61 ASCII 'a'
	blt         t1, t0, .ClassifyToken_label_272

	// *** Basic block 2

	slli        t1, t0, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 3

	j           .ClassifyToken_label_129

	// *** Basic block 4

	j           .ClassifyToken_label_130

	// *** Basic block 5

	j           .ClassifyToken_label_136

	// *** Basic block 6

	j           .ClassifyToken_label_150

	// *** Basic block 7

	j           .ClassifyToken_label_137

	// *** Basic block 8

	j           .ClassifyToken_label_138

	// *** Basic block 9

	j           .ClassifyToken_label_139

	// *** Basic block 10

	j           .ClassifyToken_label_140

	// *** Basic block 11

	j           .ClassifyToken_label_141

	// *** Basic block 12

	j           .ClassifyToken_label_154

	// *** Basic block 13

	j           .ClassifyToken_label_155

	// *** Basic block 14

	j           .ClassifyToken_label_156

	// *** Basic block 15

	j           .ClassifyToken_label_167

	// *** Basic block 16

	j           .ClassifyToken_label_157

	// *** Basic block 17

	j           .ClassifyToken_label_158

	// *** Basic block 18

	j           .ClassifyToken_label_142

	// *** Basic block 19

	j           .ClassifyToken_label_159

	// *** Basic block 20

	j           .ClassifyToken_label_224

	// *** Basic block 21

	j           .ClassifyToken_label_161

	// *** Basic block 22

	j           .ClassifyToken_label_160

	// *** Basic block 23

	j           .ClassifyToken_label_143

	// *** Basic block 24

	j           .ClassifyToken_label_162

	// *** Basic block 25

	j           .ClassifyToken_label_163

	// *** Basic block 26

	j           .ClassifyToken_label_228

	// *** Basic block 27

	j           .ClassifyToken_label_254

	// *** Basic block 28

	j           .ClassifyToken_label_229

	// *** Basic block 29

	j           .ClassifyToken_label_230

	// *** Basic block 30

	j           .ClassifyToken_label_231

	// *** Basic block 31

	j           .ClassifyToken_label_232

	// *** Basic block 32

	j           .ClassifyToken_label_258

	// *** Basic block 33

	j           .ClassifyToken_label_233

	// *** Basic block 34

	j           .ClassifyToken_label_234

	// *** Basic block 35

	j           .ClassifyToken_label_259

	// *** Basic block 36

	j           .ClassifyToken_label_235

	// *** Basic block 37

	j           .ClassifyToken_label_236

	// *** Basic block 38

	j           .ClassifyToken_label_237

	// *** Basic block 39

	j           .ClassifyToken_label_238

	// *** Basic block 40

	j           .ClassifyToken_label_239

	// *** Basic block 41

	j           .ClassifyToken_label_240

	// *** Basic block 42

	j           .ClassifyToken_label_241

	// *** Basic block 43

	j           .ClassifyToken_label_242

	// *** Basic block 44

	j           .ClassifyToken_label_243

	// *** Basic block 45

	j           .ClassifyToken_label_244

	// *** Basic block 46

	j           .ClassifyToken_label_245

	// *** Basic block 47

	j           .ClassifyToken_label_263

	// *** Basic block 48

	j           .ClassifyToken_label_267

	// *** Basic block 49

	j           .ClassifyToken_label_246

	// *** Basic block 50

	j           .ClassifyToken_label_272

	// *** Basic block 51

	j           .ClassifyToken_label_268

	// *** Basic block 52

	j           .ClassifyToken_label_189

	// *** Basic block 53

	j           .ClassifyToken_label_247

	// *** Basic block 54

	j           .ClassifyToken_label_248

	// *** Basic block 55

	j           .ClassifyToken_label_249

	// *** Basic block 56

	j           .ClassifyToken_label_250

	// *** Basic block 57

	j           .ClassifyToken_label_146

	// *** Basic block 58

	j           .ClassifyToken_label_171

	// *** Basic block 59

	j           .ClassifyToken_label_172

	// *** Basic block 60

	j           .ClassifyToken_label_201

	// *** Basic block 61

	j           .ClassifyToken_label_173

	// *** Basic block 62

	j           .ClassifyToken_label_202

	// *** Basic block 63

	j           .ClassifyToken_label_203

	// *** Basic block 64

	j           .ClassifyToken_label_204

	// *** Basic block 65

	j           .ClassifyToken_label_174

	// *** Basic block 66

	j           .ClassifyToken_label_175

	// *** Basic block 67

	j           .ClassifyToken_label_176

	// *** Basic block 68

	j           .ClassifyToken_label_205

	// *** Basic block 69

	j           .ClassifyToken_label_206

	// *** Basic block 70

	j           .ClassifyToken_label_208

	// *** Basic block 71

	j           .ClassifyToken_label_193

	// *** Basic block 72

	j           .ClassifyToken_label_144

	// *** Basic block 73

	j           .ClassifyToken_label_207

	// *** Basic block 74

	j           .ClassifyToken_label_177

	// *** Basic block 75

	j           .ClassifyToken_label_179

	// *** Basic block 76

	j           .ClassifyToken_label_178

	// *** Basic block 77

	j           .ClassifyToken_label_209

	// *** Basic block 78

	j           .ClassifyToken_label_194

	// *** Basic block 79

	j           .ClassifyToken_label_210

	// *** Basic block 80

	j           .ClassifyToken_label_211

	// *** Basic block 81

	j           .ClassifyToken_label_195

	// *** Basic block 82

	j           .ClassifyToken_label_212

	// *** Basic block 83

	j           .ClassifyToken_label_180

	// *** Basic block 84

	j           .ClassifyToken_label_213

	// *** Basic block 85

	j           .ClassifyToken_label_214

	// *** Basic block 86

	j           .ClassifyToken_label_145

	// *** Basic block 87

	j           .ClassifyToken_label_196

	// *** Basic block 88

	j           .ClassifyToken_label_220

	// *** Basic block 89

	j           .ClassifyToken_label_181

	// *** Basic block 90

	j           .ClassifyToken_label_197

	// *** Basic block 91

	j           .ClassifyToken_label_272

	// *** Basic block 92

	j           .ClassifyToken_label_215

	// *** Basic block 93

	j           .ClassifyToken_label_216

	// *** Basic block 94

	j           .ClassifyToken_label_217

	// *** Basic block 95

	j           .ClassifyToken_label_218

	// *** Basic block 96

	j           .ClassifyToken_label_219

	// *** Basic block 97

	j           .ClassifyToken_label_182

	// *** Basic block 98

	j           .ClassifyToken_label_183

	// *** Basic block 99

	j           .ClassifyToken_label_184

	// *** Basic block 100

	j           .ClassifyToken_label_185

	// *** Basic block 101

.ClassifyToken_label_129:

	// *** Basic block 102

.ClassifyToken_label_130:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 103

.ClassifyToken_label_133:
	ret         

	// *** Basic block 104

.ClassifyToken_label_136:

	// *** Basic block 105

.ClassifyToken_label_137:

	// *** Basic block 106

.ClassifyToken_label_138:

	// *** Basic block 107

.ClassifyToken_label_139:

	// *** Basic block 108

.ClassifyToken_label_140:

	// *** Basic block 109

.ClassifyToken_label_141:

	// *** Basic block 110

.ClassifyToken_label_142:

	// *** Basic block 111

.ClassifyToken_label_143:

	// *** Basic block 112

.ClassifyToken_label_144:

	// *** Basic block 113

.ClassifyToken_label_145:

	// *** Basic block 114

.ClassifyToken_label_146:
	li          a0, 4		// 0x4 ASCII \x4
	ret         

	// *** Basic block 115

.ClassifyToken_label_150:
	li          a0, 5		// 0x5 ASCII \x5
	ret         

	// *** Basic block 116

.ClassifyToken_label_154:

	// *** Basic block 117

.ClassifyToken_label_155:

	// *** Basic block 118

.ClassifyToken_label_156:

	// *** Basic block 119

.ClassifyToken_label_157:

	// *** Basic block 120

.ClassifyToken_label_158:

	// *** Basic block 121

.ClassifyToken_label_159:

	// *** Basic block 122

.ClassifyToken_label_160:

	// *** Basic block 123

.ClassifyToken_label_161:

	// *** Basic block 124

.ClassifyToken_label_162:

	// *** Basic block 125

.ClassifyToken_label_163:
	li          a0, 64		// 0x40 ASCII '@'
	ret         

	// *** Basic block 126

.ClassifyToken_label_167:
	li          a0, 65		// 0x41 ASCII 'A'
	ret         

	// *** Basic block 127

.ClassifyToken_label_171:

	// *** Basic block 128

.ClassifyToken_label_172:

	// *** Basic block 129

.ClassifyToken_label_173:

	// *** Basic block 130

.ClassifyToken_label_174:

	// *** Basic block 131

.ClassifyToken_label_175:

	// *** Basic block 132

.ClassifyToken_label_176:

	// *** Basic block 133

.ClassifyToken_label_177:

	// *** Basic block 134

.ClassifyToken_label_178:

	// *** Basic block 135

.ClassifyToken_label_179:

	// *** Basic block 136

.ClassifyToken_label_180:

	// *** Basic block 137

.ClassifyToken_label_181:

	// *** Basic block 138

.ClassifyToken_label_182:

	// *** Basic block 139

.ClassifyToken_label_183:

	// *** Basic block 140

.ClassifyToken_label_184:

	// *** Basic block 141

.ClassifyToken_label_185:
	li          a0, 1		// 0x1 ASCII \x1
	ret         

	// *** Basic block 142

.ClassifyToken_label_189:
	li          a0, 257		// 0x101
	ret         

	// *** Basic block 143

.ClassifyToken_label_193:

	// *** Basic block 144

.ClassifyToken_label_194:

	// *** Basic block 145

.ClassifyToken_label_195:

	// *** Basic block 146

.ClassifyToken_label_196:

	// *** Basic block 147

.ClassifyToken_label_197:
	li          a0, 129		// 0x81 ASCII \x81
	ret         

	// *** Basic block 148

.ClassifyToken_label_201:

	// *** Basic block 149

.ClassifyToken_label_202:

	// *** Basic block 150

.ClassifyToken_label_203:

	// *** Basic block 151

.ClassifyToken_label_204:

	// *** Basic block 152

.ClassifyToken_label_205:

	// *** Basic block 153

.ClassifyToken_label_206:

	// *** Basic block 154

.ClassifyToken_label_207:

	// *** Basic block 155

.ClassifyToken_label_208:

	// *** Basic block 156

.ClassifyToken_label_209:

	// *** Basic block 157

.ClassifyToken_label_210:

	// *** Basic block 158

.ClassifyToken_label_211:

	// *** Basic block 159

.ClassifyToken_label_212:

	// *** Basic block 160

.ClassifyToken_label_213:

	// *** Basic block 161

.ClassifyToken_label_214:

	// *** Basic block 162

.ClassifyToken_label_215:

	// *** Basic block 163

.ClassifyToken_label_216:

	// *** Basic block 164

.ClassifyToken_label_217:

	// *** Basic block 165

.ClassifyToken_label_218:

	// *** Basic block 166

.ClassifyToken_label_219:

	// *** Basic block 167

.ClassifyToken_label_220:
	li          a0, 131		// 0x83 ASCII \x83
	ret         

	// *** Basic block 168

.ClassifyToken_label_224:
	li          a0, 65		// 0x41 ASCII 'A'
	ret         

	// *** Basic block 169

.ClassifyToken_label_228:

	// *** Basic block 170

.ClassifyToken_label_229:

	// *** Basic block 171

.ClassifyToken_label_230:

	// *** Basic block 172

.ClassifyToken_label_231:

	// *** Basic block 173

.ClassifyToken_label_232:

	// *** Basic block 174

.ClassifyToken_label_233:

	// *** Basic block 175

.ClassifyToken_label_234:

	// *** Basic block 176

.ClassifyToken_label_235:

	// *** Basic block 177

.ClassifyToken_label_236:

	// *** Basic block 178

.ClassifyToken_label_237:

	// *** Basic block 179

.ClassifyToken_label_238:

	// *** Basic block 180

.ClassifyToken_label_239:

	// *** Basic block 181

.ClassifyToken_label_240:

	// *** Basic block 182

.ClassifyToken_label_241:

	// *** Basic block 183

.ClassifyToken_label_242:

	// *** Basic block 184

.ClassifyToken_label_243:

	// *** Basic block 185

.ClassifyToken_label_244:

	// *** Basic block 186

.ClassifyToken_label_245:

	// *** Basic block 187

.ClassifyToken_label_246:

	// *** Basic block 188

.ClassifyToken_label_247:

	// *** Basic block 189

.ClassifyToken_label_248:

	// *** Basic block 190

.ClassifyToken_label_249:

	// *** Basic block 191

.ClassifyToken_label_250:
	li          a0, 64		// 0x40 ASCII '@'
	ret         

	// *** Basic block 192

.ClassifyToken_label_254:
	li          a0, 1		// 0x1 ASCII \x1
	ret         

	// *** Basic block 193

.ClassifyToken_label_258:

	// *** Basic block 194

.ClassifyToken_label_259:
	li          a0, 96		// 0x60 ASCII '`'
	ret         

	// *** Basic block 195

.ClassifyToken_label_263:
	li          a0, 16		// 0x10 ASCII \x10
	ret         

	// *** Basic block 196

.ClassifyToken_label_267:

	// *** Basic block 197

.ClassifyToken_label_268:
	li          a0, 8		// 0x8 ASCII \x8
	ret         

	// *** Basic block 198

.ClassifyToken_label_272:
	mv          a0, x0
	ret         
.func_end_ClassifyToken:
	.size ClassifyToken, .func_end_ClassifyToken-ClassifyToken

.PCend:
	.data
next_pc_label_id:
	.type   next_pc_label_id,@object
	.local  next_pc_label_id
	.size   next_pc_label_id,4
	.p2align  2
	.word   0

	.type   error_abort_state,@object
	.global error_abort_state
	.comm   error_abort_state,256,8

	.type   abort_on_error,@object
	.global abort_on_error
	.comm   abort_on_error,1,1

	.type   next_pc_label,@object
	.local  next_pc_label
	.comm   next_pc_label,40,8

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz ".PC.%d"
	.type .str.1, @object
	.size .str.1, 7

.str.2:
	.asciz "Expected semicolon"
	.type .str.2, @object
	.size .str.2, 19

.str.3:
	.asciz "__invented__%d"
	.type .str.3, @object
	.size .str.3, 15

.str.4:
	.asciz "Need constant expression inside [] in designated initializer"
	.type .str.4, @object
	.size .str.4, 61

.str.5:
	.asciz "Need member name after . in designated initializer"
	.type .str.5, @object
	.size .str.5, 51

.str.6:
	.asciz "Expected = in designated initializer"
	.type .str.6, @object
	.size .str.6, 37

.str.7:
	.asciz "extern-with-init"
	.type .str.7, @object
	.size .str.7, 17

.str.8:
	.asciz "extern with initializer"
	.type .str.8, @object
	.size .str.8, 24

.str.9:
	.asciz "typdefs can\'t have initializers"
	.type .str.9, @object
	.size .str.9, 32

.str.10:
	.asciz "No such function parameter %s"
	.type .str.10, @object
	.size .str.10, 30

.str.11:
	.asciz "Type expected"
	.type .str.11, @object
	.size .str.11, 14

.str.12:
	.asciz "Function body expected"
	.type .str.12, @object
	.size .str.12, 23

.str.13:
	.asciz "Illegal use of __thread"
	.type .str.13, @object
	.size .str.13, 24

.str.14:
	.asciz "dup-storage"
	.type .str.14, @object
	.size .str.14, 12

.str.15:
	.asciz "Duplicate storage specifier"
	.type .str.15, @object
	.size .str.15, 28

.str.16:
	.asciz "Multiple incompatible storage specifiers"
	.type .str.16, @object
	.size .str.16, 41

.str.17:
	.asciz "dup-inline"
	.type .str.17, @object
	.size .str.17, 11

.str.18:
	.asciz "Duplicate \'inline\' specifier"
	.type .str.18, @object
	.size .str.18, 29

.str.19:
	.asciz "Duplicate definition of symbol %s"
	.type .str.19, @object
	.size .str.19, 34

.str.20:
	.asciz "Duplicate definition of symbol %s"
	.type .str.20, @object
	.size .str.20, 34

.str.21:
	.asciz "Symbol %s redeclared with different type"
	.type .str.21, @object
	.size .str.21, 41

.str.22:
	.asciz "Previously declared here"
	.type .str.22, @object
	.size .str.22, 25

.str.23:
	.asciz "Symbol %s redeclared with different linkage"
	.type .str.23, @object
	.size .str.23, 44

.str.24:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.24, @object
	.size .str.24, 30

.str.25:
	.asciz "syntax.c"
	.type .str.25, @object
	.size .str.25, 9

.str.26:
	.asciz "ok"
	.type .str.26, @object
	.size .str.26, 3

.str.27:
	.asciz "inline can only be applied to functions"
	.type .str.27, @object
	.size .str.27, 40

.str.28:
	.asciz "Illegal global storage specified: %s"
	.type .str.28, @object
	.size .str.28, 37

.str.29:
	.asciz "register"
	.type .str.29, @object
	.size .str.29, 9

.str.30:
	.asciz "auto"
	.type .str.30, @object
	.size .str.30, 5

.str.31:
	.asciz "Cannot mix function definition with declaration"
	.type .str.31, @object
	.size .str.31, 48

.str.32:
	.asciz "Extern variable can\'t have an initializer: %s"
	.type .str.32, @object
	.size .str.32, 46

.str.33:
	.asciz "Duplicate definition of local symbol %s"
	.type .str.33, @object
	.size .str.33, 40

.str.34:
	.asciz "Symbol %s redeclared with different type"
	.type .str.34, @object
	.size .str.34, 41

.str.35:
	.asciz "Previously declared here"
	.type .str.35, @object
	.size .str.35, 25

.str.36:
	.asciz "Symbol %s redeclared with different linkage"
	.type .str.36, @object
	.size .str.36, 44

.str.37:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.37, @object
	.size .str.37, 30

.str.38:
	.asciz "syntax.c"
	.type .str.38, @object
	.size .str.38, 9

.str.39:
	.asciz "ok"
	.type .str.39, @object
	.size .str.39, 3

.str.40:
	.asciz "Function definition not allowed here"
	.type .str.40, @object
	.size .str.40, 37

.str.41:
	.asciz "inline can only be applied to functions"
	.type .str.41, @object
	.size .str.41, 40

.str.42:
	.asciz "inline is not allowed here"
	.type .str.42, @object
	.size .str.42, 27

.str.43:
	.asciz "Missing %s"
	.type .str.43, @object
	.size .str.43, 11

