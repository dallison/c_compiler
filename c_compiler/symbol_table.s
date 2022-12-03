	.file   "symbol_table.c"
	.text
	.option pic
.PCbegin:
	.local  SymbolNodeInsertCompare
	.type SymbolNodeInsertCompare, @function

SymbolNodeInsertCompare:

	// *** Basic block 0

	.global StringCompareString
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, t0
	mv          t3, t1
	ld          a0, 40(t2)
	ld          a1, 40(t3)
	j           StringCompareString
.func_end_SymbolNodeInsertCompare:
	.size SymbolNodeInsertCompare, .func_end_SymbolNodeInsertCompare-SymbolNodeInsertCompare

	.local  SymbolNodeSearchCompare
	.type SymbolNodeSearchCompare, @function

SymbolNodeSearchCompare:

	// *** Basic block 0

	.global StringCompareString
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	ld          a0, 40(t1)
	j           StringCompareString
.func_end_SymbolNodeSearchCompare:
	.size SymbolNodeSearchCompare, .func_end_SymbolNodeSearchCompare-SymbolNodeSearchCompare

	.local  SymbolNodeDestructor
	.type SymbolNodeDestructor, @function

SymbolNodeDestructor:

	// *** Basic block 0

	.global SymbolDelete
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	beq         a1, x0, .SymbolNodeDestructor_label_22

	// *** Basic block 1

	mv          t1, t0
	ld          a0, 40(t1)
	j           SymbolDelete

	// *** Basic block 2

.SymbolNodeDestructor_label_22:
	ret         
.func_end_SymbolNodeDestructor:
	.size SymbolNodeDestructor, .func_end_SymbolNodeDestructor-SymbolNodeDestructor

	.local  DeleteSymbolTable
	.type DeleteSymbolTable, @function

DeleteSymbolTable:

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
	.global BinaryTreeDestruct
	.global free
	mv          s1, a0
	call        BinaryTreeDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_DeleteSymbolTable:
	.size DeleteSymbolTable, .func_end_DeleteSymbolTable-DeleteSymbolTable

	.global ClearSymbolTable
	.type ClearSymbolTable, @function

ClearSymbolTable:

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
	.global HashTableTraverse
	.local DeleteSymbolTable
	.global HashTableClear
	mv          s1, a0
	mv          t0, a1
	mv          a2, t0
	la          a1, DeleteSymbolTable
	call        HashTableTraverse

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           HashTableClear
.func_end_ClearSymbolTable:
	.size ClearSymbolTable, .func_end_ClearSymbolTable-ClearSymbolTable

	.global NewLocalSymbolTable
	.type NewLocalSymbolTable, @function

NewLocalSymbolTable:

	// *** Basic block 0

	.global malloc
	.global BinaryTreeInit
	.local SymbolNodeInsertCompare
	.local SymbolNodeSearchCompare
	.local SymbolNodeDestructor
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	li          a0, 48		// 0x30 ASCII '0'
	call        malloc

	// *** Basic block 1

	mv          s1, a0
	la          t0, SymbolNodeDestructor
	mv          a3, t0
	la          t0, SymbolNodeSearchCompare
	mv          a2, t0
	la          t0, SymbolNodeInsertCompare
	mv          a1, t0
	mv          a0, s1
	call        BinaryTreeInit

	// *** Basic block 2

	sd          x0, 40(s1)
	mv          a0, s1

	// *** Basic block 3

.NewLocalSymbolTable_label_36:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewLocalSymbolTable:
	.size NewLocalSymbolTable, .func_end_NewLocalSymbolTable-NewLocalSymbolTable

	.local  PrintSymbolNode
	.type PrintSymbolNode, @function

PrintSymbolNode:

	// *** Basic block 0

	.global printf
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
	slli        s3, a1, 1
	bge         x0, s3, .PrintSymbolNode_label_37

	// *** Basic block 1

.PrintSymbolNode_label_25:
	lla         a0, .str.1
	lla         a1, .str.2
	call        printf

	// *** Basic block 2

.PrintSymbolNode_label_33:
	addi        s2, s2, 1
	bge         s2, s3, .PrintSymbolNode_label_25

	// *** Basic block 3

.PrintSymbolNode_label_37:
	ld          s3, 24(s1)
	lla         t0, .str.3
	ld          t1, 40(s1)
	ld          t1, 16(t1)
	lw          t3, 0(s1)
	bnez        t3, .PrintSymbolNode_label_54

	// *** Basic block 4

	lla         t2, .str.4
	j           .PrintSymbolNode_label_57

	// *** Basic block 5

.PrintSymbolNode_label_54:
	lla         t2, .str.5

	// *** Basic block 6

.PrintSymbolNode_label_57:
	bne         s3, x0, .PrintSymbolNode_label_65

	// *** Basic block 7

	lla         a3, .str.6
	j           .PrintSymbolNode_label_70

	// *** Basic block 8

.PrintSymbolNode_label_65:
	ld          t3, 40(s3)
	ld          a3, 16(t3)

	// *** Basic block 9

.PrintSymbolNode_label_70:
	mv          a2, t2
	mv          a1, t1
	mv          a0, t0
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           printf
.func_end_PrintSymbolNode:
	.size PrintSymbolNode, .func_end_PrintSymbolNode-PrintSymbolNode

	.local  Printer
	.type Printer, @function

Printer:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local PrintSymbolNode
	j           PrintSymbolNode
.func_end_Printer:
	.size Printer, .func_end_Printer-Printer

	.global LocalSymbolTableDelete
	.type LocalSymbolTableDelete, @function

LocalSymbolTableDelete:

	// *** Basic block 0

	.global BinaryTreeDestruct
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
	mv          a1, x0
	call        BinaryTreeDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_LocalSymbolTableDelete:
	.size LocalSymbolTableDelete, .func_end_LocalSymbolTableDelete-LocalSymbolTableDelete

	.global InsertGlobalSymbol
	.type InsertGlobalSymbol, @function

InsertGlobalSymbol:

	// *** Basic block 0

	.global NewSymbolNode
	.global HashTableInsert
	.global compiler
	.global free
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
	call        NewSymbolNode

	// *** Basic block 1

	mv          s1, a0
	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 904
	mv          a1, s1
	call        HashTableInsert

	// *** Basic block 2

	mv          s2, a0
	not         t0, s2
	beqz        t0, .InsertGlobalSymbol_label_31

	// *** Basic block 3

	mv          a0, s1
	call        free

	// *** Basic block 4

.InsertGlobalSymbol_label_31:
	mv          a0, s2

	// *** Basic block 5

.InsertGlobalSymbol_label_34:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InsertGlobalSymbol:
	.size InsertGlobalSymbol, .func_end_InsertGlobalSymbol-InsertGlobalSymbol

	.global InsertGlobalTag
	.type InsertGlobalTag, @function

InsertGlobalTag:

	// *** Basic block 0

	.global NewSymbolNode
	.global HashTableInsert
	.global compiler
	.global free
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
	call        NewSymbolNode

	// *** Basic block 1

	mv          s1, a0
	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 992
	mv          a1, s1
	call        HashTableInsert

	// *** Basic block 2

	mv          s2, a0
	not         t0, s2
	beqz        t0, .InsertGlobalTag_label_31

	// *** Basic block 3

	mv          a0, s1
	call        free

	// *** Basic block 4

.InsertGlobalTag_label_31:
	mv          a0, s2

	// *** Basic block 5

.InsertGlobalTag_label_34:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InsertGlobalTag:
	.size InsertGlobalTag, .func_end_InsertGlobalTag-InsertGlobalTag

	.global FindGlobalSymbol
	.type FindGlobalSymbol, @function

FindGlobalSymbol:

	// *** Basic block 0

	.global HashTableSearch
	.global compiler
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	la          t1, compiler
	ld          t1, 0(t1)
	addi        a0, t1, 904
	mv          a1, t0
	j           HashTableSearch
.func_end_FindGlobalSymbol:
	.size FindGlobalSymbol, .func_end_FindGlobalSymbol-FindGlobalSymbol

	.global FindGlobalTag
	.type FindGlobalTag, @function

FindGlobalTag:

	// *** Basic block 0

	.global HashTableSearch
	.global compiler
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	la          t1, compiler
	ld          t1, 0(t1)
	addi        a0, t1, 992
	mv          a1, t0
	j           HashTableSearch
.func_end_FindGlobalTag:
	.size FindGlobalTag, .func_end_FindGlobalTag-FindGlobalTag

	.global InsertLocalSymbol
	.type InsertLocalSymbol, @function

InsertLocalSymbol:

	// *** Basic block 0

	.global NewSymbolNode
	.global BinaryTreeInsert
	.global free
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
	mv          a0, a1
	call        NewSymbolNode

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s2
	mv          a0, s1
	call        BinaryTreeInsert

	// *** Basic block 2

	mv          s3, a0
	not         t0, s3
	beqz        t0, .InsertLocalSymbol_label_29

	// *** Basic block 3

	mv          a0, s2
	call        free

	// *** Basic block 4

.InsertLocalSymbol_label_29:
	mv          a0, s3

	// *** Basic block 5

.InsertLocalSymbol_label_32:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InsertLocalSymbol:
	.size InsertLocalSymbol, .func_end_InsertLocalSymbol-InsertLocalSymbol

	.global FindLocalSymbol
	.type FindLocalSymbol, @function

FindLocalSymbol:

	// *** Basic block 0

	.global FindSymbol
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
	beq         s1, x0, .FindLocalSymbol_label_37

	// *** Basic block 1

.FindLocalSymbol_label_15:
	mv          a1, s2
	mv          a0, s1
	call        FindSymbol

	// *** Basic block 2

	mv          s3, a0
	beq         s3, x0, .FindLocalSymbol_label_31

	// *** Basic block 3

	mv          a0, s3

	// *** Basic block 4

.FindLocalSymbol_label_28:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.FindLocalSymbol_label_31:
	ld          s1, 40(s1)
	bne         s1, x0, .FindLocalSymbol_label_15

	// *** Basic block 6

.FindLocalSymbol_label_37:
	mv          a0, x0
	j           .FindLocalSymbol_label_28
.func_end_FindLocalSymbol:
	.size FindLocalSymbol, .func_end_FindLocalSymbol-FindLocalSymbol

	.global FindTopLocalSymbol
	.type FindTopLocalSymbol, @function

FindTopLocalSymbol:

	// *** Basic block 0

	.global FindSymbol
	// Leaf procedure, no stack frame generated
	j           FindSymbol
.func_end_FindTopLocalSymbol:
	.size FindTopLocalSymbol, .func_end_FindTopLocalSymbol-FindTopLocalSymbol

	.global NewSymbolNode
	.type NewSymbolNode, @function

NewSymbolNode:

	// *** Basic block 0

	.global malloc
	.global BinaryTreeNodeInit
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
	li          a0, 48		// 0x30 ASCII '0'
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a0, s2
	call        BinaryTreeNodeInit

	// *** Basic block 2

	sd          s1, 40(s2)
	mv          a0, s2

	// *** Basic block 3

.NewSymbolNode_label_23:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewSymbolNode:
	.size NewSymbolNode, .func_end_NewSymbolNode-NewSymbolNode

	.local  InsertSymbolIntoHashTable
	.type InsertSymbolIntoHashTable, @function

InsertSymbolIntoHashTable:

	// *** Basic block 0

	.global NewBinaryTree
	.local SymbolNodeInsertCompare
	.local SymbolNodeSearchCompare
	.local SymbolNodeDestructor
	.global BinaryTreeInsert
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
	mv          s1, a2
	mv          s2, a1
	mv          s3, a0
	bne         a0, x0, .InsertSymbolIntoHashTable_label_35

	// *** Basic block 1

	la          t0, SymbolNodeDestructor
	mv          a2, t0
	la          t0, SymbolNodeSearchCompare
	mv          a1, t0
	la          t0, SymbolNodeInsertCompare
	mv          a0, t0
	call        NewBinaryTree

	// *** Basic block 2

	mv          s3, a0
	sd          s3, 0(s1)

	// *** Basic block 3

.InsertSymbolIntoHashTable_label_35:
	mv          a1, s2
	mv          a0, s3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           BinaryTreeInsert
.func_end_InsertSymbolIntoHashTable:
	.size InsertSymbolIntoHashTable, .func_end_InsertSymbolIntoHashTable-InsertSymbolIntoHashTable

	.global FindSymbol
	.type FindSymbol, @function

FindSymbol:

	// *** Basic block 0

	.global BinaryTreeSearch
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	call        BinaryTreeSearch

	// *** Basic block 1

	mv          s1, a0
	bne         s1, x0, .FindSymbol_label_27

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.FindSymbol_label_24:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.FindSymbol_label_27:
	ld          a0, 40(s1)
	j           .FindSymbol_label_24
.func_end_FindSymbol:
	.size FindSymbol, .func_end_FindSymbol-FindSymbol

	.local  FindSymbolInHashTable
	.type FindSymbolInHashTable, @function

FindSymbolInHashTable:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global FindSymbol
	mv          t0, a0
	mv          t1, t0
	mv          a0, t1
	j           FindSymbol
.func_end_FindSymbolInHashTable:
	.size FindSymbolInHashTable, .func_end_FindSymbolInHashTable-FindSymbolInHashTable

	.local  HashSymbol
	.type HashSymbol, @function

HashSymbol:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	slli        t1, a2, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 1

	j           .HashSymbol_label_29

	// *** Basic block 2

	j           .HashSymbol_label_24

	// *** Basic block 3

.HashSymbol_label_24:
	ld          t1, 40(t0)
	j           .HashSymbol_label_32

	// *** Basic block 4

.HashSymbol_label_29:
	mv          t1, t0
	j           .HashSymbol_label_32

	// *** Basic block 5

.HashSymbol_label_32:
	ld          t2, 16(t1)
	li          t3, 5381		// 0x1505
	lb          t4, 0(t2)
	beqz        t4, .HashSymbol_label_52

	// *** Basic block 6

.HashSymbol_label_41:
	slli        t4, t3, 5
	add         t4, t4, t3
	mv          t5, t2
	addi        t2, t2, 1
	lb          t6, 0(t5)
	add         t3, t4, t6
	lb          t2, 0(t2)
	bnez        t2, .HashSymbol_label_41

	// *** Basic block 7

.HashSymbol_label_52:
	mv          a0, t3

	// *** Basic block 8

.HashSymbol_label_55:
	ret         
.func_end_HashSymbol:
	.size HashSymbol, .func_end_HashSymbol-HashSymbol

	.global CreateGlobalSymbolTables
	.type CreateGlobalSymbolTables, @function

CreateGlobalSymbolTables:

	// *** Basic block 0

	.global HashTableInit
	.global compiler
	.local HashSymbol
	.local InsertSymbolIntoHashTable
	.local FindSymbolInHashTable
	addi sp, sp, -16
	// Saved return address (offset 8) and frame pointer (offset 0)
	sd ra, 8(sp)
	sd s0, 0(sp)
	addi s0, sp, 16
	// Local vars at offset -16(s0)
	// End of stack frame
	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 904
	lla         a1, .str.7
	la          t0, FindSymbolInHashTable
	mv          a5, t0
	la          t0, InsertSymbolIntoHashTable
	mv          a4, t0
	la          t0, HashSymbol
	mv          a3, t0
	li          a2, 1009		// 0x3f1
	call        HashTableInit

	// *** Basic block 1

	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 992
	lla         a1, .str.8
	la          t0, FindSymbolInHashTable
	mv          a5, t0
	la          t0, InsertSymbolIntoHashTable
	mv          a4, t0
	la          t0, HashSymbol
	mv          a3, t0
	li          t0, 101		// 0x65 ASCII 'e'
	mv          a2, t0
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           HashTableInit
.func_end_CreateGlobalSymbolTables:
	.size CreateGlobalSymbolTables, .func_end_CreateGlobalSymbolTables-CreateGlobalSymbolTables

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s"
	.type .str.1, @object
	.size .str.1, 3

.str.2:
	.asciz " "
	.type .str.2, @object
	.size .str.2, 2

.str.3:
	.asciz "%s: %s (%s)\n"
	.type .str.3, @object
	.size .str.3, 13

.str.4:
	.asciz "RED"
	.type .str.4, @object
	.size .str.4, 4

.str.5:
	.asciz "BLACK"
	.type .str.5, @object
	.size .str.5, 6

.str.6:
	.asciz "(null)"
	.type .str.6, @object
	.size .str.6, 1

.str.7:
	.asciz "global-symbol-table"
	.type .str.7, @object
	.size .str.7, 20

.str.8:
	.asciz "global-tag-table"
	.type .str.8, @object
	.size .str.8, 17

