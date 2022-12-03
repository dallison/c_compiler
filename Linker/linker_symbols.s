	.file   "linker_symbols.c"
	.text
	.option pic
.PCbegin:
	.global NewSymbol
	.type NewSymbol, @function

NewSymbol:

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
	li          a0, 104		// 0x68 ASCII 'h'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	sd          s1, 0(s3)
	addi        a0, s3, 8
	lla         a1, .str.1
	call        StringInit

	// *** Basic block 2

	sb          x0, 48(s3)
	sb          x0, 49(s3)
	sd          x0, 56(s3)
	sd          s2, 64(s3)
	sd          x0, 80(s3)
	ld          t0, 16(s1)
	sd          t0, 72(s3)
	li          t0, -1		// 0xffffffffffffffff
	sw          t0, 88(s3)
	sw          t0, 92(s3)
	sw          t0, 96(s3)
	sw          t0, 100(s3)
	mv          a0, s3

	// *** Basic block 3

.NewSymbol_label_68:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewSymbol:
	.size NewSymbol, .func_end_NewSymbol-NewSymbol

	.global SymbolDelete
	.type SymbolDelete, @function

SymbolDelete:

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
.func_end_SymbolDelete:
	.size SymbolDelete, .func_end_SymbolDelete-SymbolDelete

	.global SymbolHash
	.type SymbolHash, @function

SymbolHash:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	slli        t1, a2, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 1

	j           .SymbolHash_label_30

	// *** Basic block 2

	j           .SymbolHash_label_24

	// *** Basic block 3

.SymbolHash_label_24:
	addi        t2, t0, 8
	ld          t1, 16(t2)
	j           .SymbolHash_label_33

	// *** Basic block 4

.SymbolHash_label_30:
	mv          t1, t0
	j           .SymbolHash_label_33

	// *** Basic block 5

.SymbolHash_label_33:
	li          t2, 5381		// 0x1505
	lb          t3, 0(t1)
	beqz        t3, .SymbolHash_label_50

	// *** Basic block 6

.SymbolHash_label_39:
	slli        t3, t2, 5
	add         t3, t3, t2
	mv          t4, t1
	addi        t1, t1, 1
	lb          t5, 0(t4)
	add         t2, t3, t5
	lb          t3, 0(t1)
	bnez        t3, .SymbolHash_label_39

	// *** Basic block 7

.SymbolHash_label_50:
	mv          a0, t2

	// *** Basic block 8

.SymbolHash_label_53:
	ret         
.func_end_SymbolHash:
	.size SymbolHash, .func_end_SymbolHash-SymbolHash

	.global SymbolInsertInHashTable
	.type SymbolInsertInHashTable, @function

SymbolInsertInHashTable:

	// *** Basic block 0

	.global NewVector
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
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a2
	mv          s3, a1
	bne         s1, x0, .SymbolInsertInHashTable_label_22

	// *** Basic block 1

	call        NewVector

	// *** Basic block 2

	mv          s1, a0
	sd          s1, 0(s2)

	// *** Basic block 3

.SymbolInsertInHashTable_label_22:
	mv          s4, s1
	mv          a1, s3
	mv          a0, s4
	call        VectorAppend

	// *** Basic block 4

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0

	// *** Basic block 5

.SymbolInsertInHashTable_label_33:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SymbolInsertInHashTable:
	.size SymbolInsertInHashTable, .func_end_SymbolInsertInHashTable-SymbolInsertInHashTable

	.global SymbolFindInHashTable
	.type SymbolFindInHashTable, @function

SymbolFindInHashTable:

	// *** Basic block 0

	.global StringEqual
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
	mv          s1, a1
	bne         t0, x0, .SymbolFindInHashTable_label_22

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.SymbolFindInHashTable_label_19:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.SymbolFindInHashTable_label_22:
	mv          t1, t0
	mv          s2, x0
	ld          s3, 8(t1)
	bge         x0, s3, .SymbolFindInHashTable_label_52

	// *** Basic block 4

	ld          s4, 0(t1)

	// *** Basic block 5

.SymbolFindInHashTable_label_32:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	addi        a0, s4, 8
	mv          a1, s1
	call        StringEqual

	// *** Basic block 6

	beqz        a0, .SymbolFindInHashTable_label_47

	// *** Basic block 7

	mv          a0, s4
	j           .SymbolFindInHashTable_label_19

	// *** Basic block 8

.SymbolFindInHashTable_label_47:

	// *** Basic block 9

.SymbolFindInHashTable_label_48:
	addi        s2, s2, 1
	bge         s2, s3, .SymbolFindInHashTable_label_32

	// *** Basic block 10

.SymbolFindInHashTable_label_52:
	mv          a0, x0
	j           .SymbolFindInHashTable_label_19
.func_end_SymbolFindInHashTable:
	.size SymbolFindInHashTable, .func_end_SymbolFindInHashTable-SymbolFindInHashTable

	.local  DeleteSymbolList
	.type DeleteSymbolList, @function

DeleteSymbolList:

	// *** Basic block 0

	.global SymbolDelete
	.global VectorDelete
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
	ld          s3, 8(s1)
	bge         x0, s3, .DeleteSymbolList_label_34

	// *** Basic block 1

	ld          s4, 0(s1)

	// *** Basic block 2

.DeleteSymbolList_label_21:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	mv          a0, s4
	call        SymbolDelete

	// *** Basic block 3

.DeleteSymbolList_label_30:
	addi        s2, s2, 1
	bge         s2, s3, .DeleteSymbolList_label_21

	// *** Basic block 4

.DeleteSymbolList_label_34:
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorDelete
.func_end_DeleteSymbolList:
	.size DeleteSymbolList, .func_end_DeleteSymbolList-DeleteSymbolList

	.global LinkerClearSymbolTable
	.type LinkerClearSymbolTable, @function

LinkerClearSymbolTable:

	// *** Basic block 0

	.global HashTableTraverse
	.local DeleteSymbolList
	// Leaf procedure, no stack frame generated
	mv          a2, x0
	la          a1, DeleteSymbolList
	j           HashTableTraverse
.func_end_LinkerClearSymbolTable:
	.size LinkerClearSymbolTable, .func_end_LinkerClearSymbolTable-LinkerClearSymbolTable

	.local  PrintSymbolList
	.type PrintSymbolList, @function

PrintSymbolList:

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
	sd s4, 0(sp)
	// End of stack frame
	mv          t0, a0
	mv          s1, x0
	ld          s2, 8(t0)
	bge         x0, s2, .PrintSymbolList_label_73

	// *** Basic block 1

	ld          t1, 0(t0)

	// *** Basic block 2

.PrintSymbolList_label_27:
	slli        t0, s1, 3
	add         t0, t1, t0
	ld          s3, 0(t0)
	lla         s4, .str.2
	lb          t0, 48(s3)
	not         t0, t0
	beqz        t0, .PrintSymbolList_label_42

	// *** Basic block 3

	lla         s4, .str.3
	j           .PrintSymbolList_label_52

	// *** Basic block 4

.PrintSymbolList_label_42:
	ld          t0, 56(s3)
	beq         t0, x0, .PrintSymbolList_label_51

	// *** Basic block 5

	addi        t0, t0, 8
	ld          s4, 16(t0)

	// *** Basic block 6

.PrintSymbolList_label_51:

	// *** Basic block 7

.PrintSymbolList_label_52:
	lla         a0, .str.4
	ld          a1, 80(s3)
	addi        t0, s3, 8
	ld          a2, 16(t0)
	mv          a3, s4
	call        printf

	// *** Basic block 8

.PrintSymbolList_label_69:
	addi        s1, s1, 1
	bge         s1, s2, .PrintSymbolList_label_27

	// *** Basic block 9

.PrintSymbolList_label_73:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PrintSymbolList:
	.size PrintSymbolList, .func_end_PrintSymbolList-PrintSymbolList

	.local  PrintSymbolTable
	.type PrintSymbolTable, @function

PrintSymbolTable:

	// *** Basic block 0

	.global HashTableTraverse
	.local PrintSymbolList
	// Leaf procedure, no stack frame generated
	mv          a2, x0
	la          a1, PrintSymbolList
	j           HashTableTraverse
.func_end_PrintSymbolTable:
	.size PrintSymbolTable, .func_end_PrintSymbolTable-PrintSymbolTable

	.global LinkerPrintSymbolTables
	.type LinkerPrintSymbolTables, @function

LinkerPrintSymbolTables:

	// *** Basic block 0

	.global printf
	.local PrintSymbolTable
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
	lla         a0, .str.5
	call        printf

	// *** Basic block 1

	addi        a0, s1, 160
	call        PrintSymbolTable

	// *** Basic block 2

	mv          s2, x0
	addi        t0, s1, 40
	ld          s3, 8(t0)
	bge         x0, s3, .LinkerPrintSymbolTables_label_54

	// *** Basic block 3

	ld          s4, 40(s1)

	// *** Basic block 4

.LinkerPrintSymbolTables_label_33:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          s1, 0(t0)
	lla         a0, .str.6
	ld          a1, 16(s1)
	call        printf

	// *** Basic block 5

	addi        a0, s1, 48
	call        PrintSymbolTable

	// *** Basic block 6

.LinkerPrintSymbolTables_label_50:
	addi        s2, s2, 1
	bge         s2, s3, .LinkerPrintSymbolTables_label_33

	// *** Basic block 7

.LinkerPrintSymbolTables_label_54:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LinkerPrintSymbolTables:
	.size LinkerPrintSymbolTables, .func_end_LinkerPrintSymbolTables-LinkerPrintSymbolTables

	.local  CheckUndefined
	.type CheckUndefined, @function

CheckUndefined:

	// *** Basic block 0

	.global LinkerError
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
	mv          s1, x0
	ld          s2, 8(t0)
	bge         x0, s2, .CheckUndefined_label_52

	// *** Basic block 1

	ld          t1, 0(t0)

	// *** Basic block 2

.CheckUndefined_label_24:
	slli        t0, s1, 3
	add         t0, t1, t0
	ld          s3, 0(t0)
	lb          t0, 48(s3)
	not         t0, t0
	beqz        t0, .CheckUndefined_label_47

	// *** Basic block 3

	ld          a0, 64(s3)
	lla         a1, .str.7
	addi        t0, s3, 8
	ld          a2, 16(t0)
	call        LinkerError

	// *** Basic block 4

.CheckUndefined_label_47:

	// *** Basic block 5

.CheckUndefined_label_48:
	addi        s1, s1, 1
	bge         s1, s2, .CheckUndefined_label_24

	// *** Basic block 6

.CheckUndefined_label_52:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CheckUndefined:
	.size CheckUndefined, .func_end_CheckUndefined-CheckUndefined

	.global LinkerCheckForUndefinedSymbols
	.type LinkerCheckForUndefinedSymbols, @function

LinkerCheckForUndefinedSymbols:

	// *** Basic block 0

	.global HashTableTraverse
	.local CheckUndefined
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 160
	mv          a2, x0
	la          a1, CheckUndefined
	j           HashTableTraverse
.func_end_LinkerCheckForUndefinedSymbols:
	.size LinkerCheckForUndefinedSymbols, .func_end_LinkerCheckForUndefinedSymbols-LinkerCheckForUndefinedSymbols

	.local  AssignSymbolListAddresses
	.type AssignSymbolListAddresses, @function

AssignSymbolListAddresses:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, x0
	ld          t2, 8(t0)
	bge         x0, t2, .AssignSymbolListAddresses_label_44

	// *** Basic block 1

	ld          t3, 0(t0)

	// *** Basic block 2

.AssignSymbolListAddresses_label_21:
	slli        t0, t1, 3
	add         t0, t3, t0
	ld          t3, 0(t0)
	ld          t0, 56(t3)
	beq         t0, x0, .AssignSymbolListAddresses_label_39

	// *** Basic block 3

	ld          t0, 56(t0)
	ld          t4, 80(t3)
	add         t0, t4, t0
	sd          t0, 80(t3)

	// *** Basic block 4

.AssignSymbolListAddresses_label_39:

	// *** Basic block 5

.AssignSymbolListAddresses_label_40:
	addi        t1, t1, 1
	bge         t1, t2, .AssignSymbolListAddresses_label_21

	// *** Basic block 6

.AssignSymbolListAddresses_label_44:
	ret         
.func_end_AssignSymbolListAddresses:
	.size AssignSymbolListAddresses, .func_end_AssignSymbolListAddresses-AssignSymbolListAddresses

	.global LinkerAssignSymbolAddresses
	.type LinkerAssignSymbolAddresses, @function

LinkerAssignSymbolAddresses:

	// *** Basic block 0

	.global HashTableTraverse
	.local AssignSymbolListAddresses
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
	addi        t0, s1, 40
	ld          s3, 8(t0)
	bge         x0, s3, .LinkerAssignSymbolAddresses_label_43

	// *** Basic block 1

	ld          s4, 40(s1)

	// *** Basic block 2

.LinkerAssignSymbolAddresses_label_23:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	addi        a0, s4, 48
	mv          a2, x0
	la          t0, AssignSymbolListAddresses
	mv          a1, t0
	call        HashTableTraverse

	// *** Basic block 3

.LinkerAssignSymbolAddresses_label_39:
	addi        s2, s2, 1
	bge         s2, s3, .LinkerAssignSymbolAddresses_label_23

	// *** Basic block 4

.LinkerAssignSymbolAddresses_label_43:
	addi        a0, s1, 160
	mv          a2, x0
	la          t0, AssignSymbolListAddresses
	mv          a1, t0
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           HashTableTraverse
.func_end_LinkerAssignSymbolAddresses:
	.size LinkerAssignSymbolAddresses, .func_end_LinkerAssignSymbolAddresses-LinkerAssignSymbolAddresses

	.global LinkerAssignCommonSymbolAddresses
	.type LinkerAssignCommonSymbolAddresses, @function

LinkerAssignCommonSymbolAddresses:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, x0
	addi        t2, a0, 40
	ld          t2, 8(t2)
	bge         x0, t2, .LinkerAssignCommonSymbolAddresses_label_60

	// *** Basic block 1

	ld          t3, 40(a0)
	ld          t4, 0(t0)

	// *** Basic block 2

.LinkerAssignCommonSymbolAddresses_label_26:
	slli        t5, t1, 3
	add         t3, t3, t5
	ld          t5, 0(t3)
	mv          t3, x0
	addi        t6, t5, 232
	ld          t6, 8(t6)
	bge         x0, t6, .LinkerAssignCommonSymbolAddresses_label_55

	// *** Basic block 3

	ld          t5, 232(t5)

	// *** Basic block 4

.LinkerAssignCommonSymbolAddresses_label_40:
	slli        a0, t3, 3
	add         t5, t5, a0
	ld          a0, 0(t5)
	sd          t4, 80(a0)
	ld          t5, 72(a0)
	add         t4, t4, t5
	sd          t4, 0(t0)

	// *** Basic block 5

.LinkerAssignCommonSymbolAddresses_label_51:
	addi        t3, t3, 1
	bge         t3, t6, .LinkerAssignCommonSymbolAddresses_label_40

	// *** Basic block 6

.LinkerAssignCommonSymbolAddresses_label_55:

	// *** Basic block 7

.LinkerAssignCommonSymbolAddresses_label_56:
	addi        t1, t1, 1
	bge         t1, t2, .LinkerAssignCommonSymbolAddresses_label_26

	// *** Basic block 8

.LinkerAssignCommonSymbolAddresses_label_60:
	ret         
.func_end_LinkerAssignCommonSymbolAddresses:
	.size LinkerAssignCommonSymbolAddresses, .func_end_LinkerAssignCommonSymbolAddresses-LinkerAssignCommonSymbolAddresses

	.local  AssignSymbolSectionIndex
	.type AssignSymbolSectionIndex, @function

AssignSymbolSectionIndex:

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
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a2
	mv          s3, a1
	lhu         s4, 6(a3)
	snez        t0, s4
	beqz        s4, .AssignSymbolSectionIndex_label_31

	// *** Basic block 1

	slti        t0, s4, -256

	// *** Basic block 2

.AssignSymbolSectionIndex_label_31:
	beqz        t0, .AssignSymbolSectionIndex_label_43

	// *** Basic block 3

	ld          t0, 56(s2)
	slli        t1, s4, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, 56(s1)
	j           .AssignSymbolSectionIndex_label_55

	// *** Basic block 4

.AssignSymbolSectionIndex_label_43:
	li          t0, -14		// 0xfffffffffffffff2
	bne         s4, t0, .AssignSymbolSectionIndex_label_54

	// *** Basic block 5

	addi        a0, s3, 232
	mv          a1, s1
	call        VectorAppend

	// *** Basic block 6

.AssignSymbolSectionIndex_label_54:

	// *** Basic block 7

.AssignSymbolSectionIndex_label_55:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AssignSymbolSectionIndex:
	.size AssignSymbolSectionIndex, .func_end_AssignSymbolSectionIndex-AssignSymbolSectionIndex

	.local  ReadLocalSymbol
	.type ReadLocalSymbol, @function

ReadLocalSymbol:

	// *** Basic block 0

	.global NewSymbol
	.global StringSet
	.local AssignSymbolSectionIndex
	.global LinkerInsertSymbol
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
	mv          s2, a1
	mv          s3, a0
	mv          s4, a2
	mv          a0, s1
	call        NewSymbol

	// *** Basic block 1

	mv          s5, a0
	addi        a0, s5, 8
	mv          a1, s3
	call        StringSet

	// *** Basic block 2

	mv          a3, s1
	mv          a2, s4
	mv          a1, s2
	mv          a0, s5
	call        AssignSymbolSectionIndex

	// *** Basic block 3

	addi        a0, s2, 48
	mv          a1, s5
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LinkerInsertSymbol
.func_end_ReadLocalSymbol:
	.size ReadLocalSymbol, .func_end_ReadLocalSymbol-ReadLocalSymbol

	.local  RedefineSymbol
	.type RedefineSymbol, @function

RedefineSymbol:

	// *** Basic block 0

	.global LinkerError
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
	lhu         s5, 6(s1)
	snez        t0, s5
	beqz        t0, .RedefineSymbol_label_96

	// *** Basic block 1

	lb          t0, 48(s2)
	beqz        t0, .RedefineSymbol_label_73

	// *** Basic block 2

	ld          t1, 0(s2)
	lhu         t1, 6(t1)
	addi        t2, t1, 14
	seqz        t0, t2
	li          t2, -14		// 0xfffffffffffffff2
	bne         t1, t2, .RedefineSymbol_label_47

	// *** Basic block 3

	addi        t1, s5, 14
	seqz        t0, t1

	// *** Basic block 4

.RedefineSymbol_label_47:
	beqz        t0, .RedefineSymbol_label_61

	// *** Basic block 5

	ld          t0, 16(s1)
	ld          t1, 72(s2)
	bge         t1, t0, .RedefineSymbol_label_57

	// *** Basic block 6

	sd          t0, 72(s2)

	// *** Basic block 7

.RedefineSymbol_label_57:

	// *** Basic block 8

.RedefineSymbol_label_58:
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

	// *** Basic block 9

.RedefineSymbol_label_61:
	lla         a1, .str.8
	addi        t0, s2, 8
	ld          a2, 16(t0)
	mv          a0, s3
	call        LinkerError

	// *** Basic block 10

	j           .RedefineSymbol_label_58

	// *** Basic block 11

.RedefineSymbol_label_73:
	li          t0, -256		// 0xffffffffffffff00
	bge         s5, t0, .RedefineSymbol_label_87

	// *** Basic block 12

	ld          t0, 56(s4)
	slli        t1, s5, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, 56(s2)

	// *** Basic block 13

.RedefineSymbol_label_87:
	sd          s1, 0(s2)
	ld          t0, 8(s1)
	sd          t0, 80(s2)
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 48(s2)

	// *** Basic block 14

.RedefineSymbol_label_96:
	j           .RedefineSymbol_label_58
.func_end_RedefineSymbol:
	.size RedefineSymbol, .func_end_RedefineSymbol-RedefineSymbol

	.global LinkerReadSymbol
	.type LinkerReadSymbol, @function

LinkerReadSymbol:

	// *** Basic block 0

	.global LinkerError
	.local ReadLocalSymbol
	.global LinkerFindSymbol
	.local RedefineSymbol
	.global NewSymbol
	.global StringSet
	.local AssignSymbolSectionIndex
	.global LinkerInsertSymbol
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
	mv          s1, a4
	mv          t0, a3
	mv          s2, a1
	mv          s3, a2
	mv          s4, a0
	lbu         t1, 4(s1)
	srai        t2, t1, 4
	lwu         t1, 0(s1)
	ld          t3, 0(t0)
	ld          t3, 32(t3)
	bge         t3, t1, .LinkerReadSymbol_label_60

	// *** Basic block 1

	lla         a1, .str.9
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
	j           LinkerError

	// *** Basic block 2

.LinkerReadSymbol_label_57:
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

	// *** Basic block 3

.LinkerReadSymbol_label_60:
	ld          t3, 48(t0)
	add         s5, t3, t1
	lb          t1, 0(s5)
	bnez        t1, .LinkerReadSymbol_label_69

	// *** Basic block 4

	j           .LinkerReadSymbol_label_57

	// *** Basic block 5

.LinkerReadSymbol_label_69:
	seqz        t0, t2
	beqz        t0, .LinkerReadSymbol_label_83

	// *** Basic block 6

	mv          a3, s1
	mv          a2, s3
	mv          a1, s2
	mv          a0, s5
	call        ReadLocalSymbol

	// *** Basic block 7

	j           .LinkerReadSymbol_label_57

	// *** Basic block 8

.LinkerReadSymbol_label_83:
	addi        a0, s4, 160
	mv          a1, s5
	call        LinkerFindSymbol

	// *** Basic block 9

	mv          s6, a0
	beq         s6, x0, .LinkerReadSymbol_label_104

	// *** Basic block 10

	mv          a3, s1
	mv          a2, s3
	mv          a1, s2
	mv          a0, s6
	call        RedefineSymbol

	// *** Basic block 11

	j           .LinkerReadSymbol_label_57

	// *** Basic block 12

.LinkerReadSymbol_label_104:
	mv          a1, s2
	mv          a0, s1
	call        NewSymbol

	// *** Basic block 13

	mv          s6, a0
	addi        a0, s6, 8
	mv          a1, s5
	call        StringSet

	// *** Basic block 14

	lhu         t0, 6(s1)
	snez        t0, t0
	sb          t0, 48(s6)
	ld          t0, 0(s6)
	ld          t0, 8(t0)
	sd          t0, 80(s6)
	mv          a3, s1
	mv          a2, s3
	mv          a1, s2
	mv          a0, s6
	call        AssignSymbolSectionIndex

	// *** Basic block 15

	addi        a0, s4, 160
	mv          a1, s6
	call        LinkerInsertSymbol

	// *** Basic block 16

	j           .LinkerReadSymbol_label_57
.func_end_LinkerReadSymbol:
	.size LinkerReadSymbol, .func_end_LinkerReadSymbol-LinkerReadSymbol

	.global LinkerInventSymbol
	.type LinkerInventSymbol, @function

LinkerInventSymbol:

	// *** Basic block 0

	.global malloc
	.global NewSymbol
	.global StringSet
	.global LinkerInsertSymbol
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
	mv          s1, a2
	mv          s2, a1
	mv          s3, a0
	li          a0, 24		// 0x18 ASCII \x18
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	sd          s1, 16(s4)
	mv          a1, x0
	mv          a0, s4
	call        NewSymbol

	// *** Basic block 2

	mv          s5, a0
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 48(s5)
	sb          t0, 49(s5)
	addi        a0, s5, 8
	mv          a1, s2
	call        StringSet

	// *** Basic block 3

	addi        a0, s3, 160
	mv          a1, s5
	call        LinkerInsertSymbol

	// *** Basic block 4

	mv          a0, s5

	// *** Basic block 5

.LinkerInventSymbol_label_56:
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
.func_end_LinkerInventSymbol:
	.size LinkerInventSymbol, .func_end_LinkerInventSymbol-LinkerInventSymbol

	.local  AssignSectionAddress
	.type AssignSectionAddress, @function

AssignSectionAddress:

	// *** Basic block 0

	.global ObjectFileFindSymbol
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
	mv          t1, a1
	ld          s1, 8(t0)
	mv          t2, t1
	addi        t3, s1, 8
	ld          a1, 16(t3)
	mv          a0, t2
	call        ObjectFileFindSymbol

	// *** Basic block 1

	mv          s2, a0
	beq         s2, x0, .AssignSectionAddress_label_36

	// *** Basic block 2

	ld          t0, 56(s1)
	sd          t0, 80(s2)

	// *** Basic block 3

.AssignSectionAddress_label_36:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AssignSectionAddress:
	.size AssignSectionAddress, .func_end_AssignSectionAddress-AssignSectionAddress

	.global LinkerAssignSectionSymbolAddresses
	.type LinkerAssignSectionSymbolAddresses, @function

LinkerAssignSectionSymbolAddresses:

	// *** Basic block 0

	.global MapTraverse
	.local AssignSectionAddress
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
	addi        t0, a0, 40
	ld          s2, 8(t0)
	bge         x0, s2, .LinkerAssignSectionSymbolAddresses_label_42

	// *** Basic block 1

	ld          s3, 40(a0)

	// *** Basic block 2

.LinkerAssignSectionSymbolAddresses_label_22:
	slli        t0, s1, 3
	add         t0, s3, t0
	ld          s3, 0(t0)
	addi        a0, s3, 168
	mv          a2, s3
	la          t0, AssignSectionAddress
	mv          a1, t0
	call        MapTraverse

	// *** Basic block 3

.LinkerAssignSectionSymbolAddresses_label_38:
	addi        s1, s1, 1
	bge         s1, s2, .LinkerAssignSectionSymbolAddresses_label_22

	// *** Basic block 4

.LinkerAssignSectionSymbolAddresses_label_42:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LinkerAssignSectionSymbolAddresses:
	.size LinkerAssignSectionSymbolAddresses, .func_end_LinkerAssignSectionSymbolAddresses-LinkerAssignSectionSymbolAddresses

	.global LinkerAssignBSSSymbolAddresses
	.type LinkerAssignBSSSymbolAddresses, @function

LinkerAssignBSSSymbolAddresses:

	// *** Basic block 0

	.global ObjectFileFindSymbol
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
	mv          s1, x0
	addi        t1, t0, 40
	ld          s2, 8(t1)
	bge         x0, s2, .LinkerAssignBSSSymbolAddresses_label_50

	// *** Basic block 1

	ld          s3, 40(t0)
	ld          s4, 432(t0)

	// *** Basic block 2

.LinkerAssignBSSSymbolAddresses_label_25:
	slli        t0, s1, 3
	add         t0, s3, t0
	ld          s3, 0(t0)
	lla         a1, .str.10
	mv          a0, s3
	call        ObjectFileFindSymbol

	// *** Basic block 3

	mv          s3, a0
	beq         s3, x0, .LinkerAssignBSSSymbolAddresses_label_45

	// *** Basic block 4

	sd          s4, 80(s3)

	// *** Basic block 5

.LinkerAssignBSSSymbolAddresses_label_45:

	// *** Basic block 6

.LinkerAssignBSSSymbolAddresses_label_46:
	addi        s1, s1, 1
	bge         s1, s2, .LinkerAssignBSSSymbolAddresses_label_25

	// *** Basic block 7

.LinkerAssignBSSSymbolAddresses_label_50:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LinkerAssignBSSSymbolAddresses:
	.size LinkerAssignBSSSymbolAddresses, .func_end_LinkerAssignBSSSymbolAddresses-LinkerAssignBSSSymbolAddresses

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "(null)"
	.type .str.1, @object
	.size .str.1, 1

.str.2:
	.asciz "(null)"
	.type .str.2, @object
	.size .str.2, 1

.str.3:
	.asciz "undefined"
	.type .str.3, @object
	.size .str.3, 10

.str.4:
	.asciz "0x%016llx: %-20s %s\n"
	.type .str.4, @object
	.size .str.4, 21

.str.5:
	.asciz "Global symbol table\n"
	.type .str.5, @object
	.size .str.5, 21

.str.6:
	.asciz "Local symbols in file %s\n"
	.type .str.6, @object
	.size .str.6, 26

.str.7:
	.asciz "Undefined symbol %s"
	.type .str.7, @object
	.size .str.7, 20

.str.8:
	.asciz "Multiple definition of symbol %s"
	.type .str.8, @object
	.size .str.8, 33

.str.9:
	.asciz "Corrupt symbol name"
	.type .str.9, @object
	.size .str.9, 20

.str.10:
	.asciz ".bss"
	.type .str.10, @object
	.size .str.10, 5

