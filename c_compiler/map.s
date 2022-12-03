	.file   "map.c"
	.text
	.option pic
.PCbegin:
	.global MapInit
	.type MapInit, @function

MapInit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sd          x0, 16(a0)
	sd          x0, 8(a0)
	sd          x0, 0(a0)
	sd          a1, 24(a0)
	ret         
.func_end_MapInit:
	.size MapInit, .func_end_MapInit-MapInit

	.local  CompareStrings
	.type CompareStrings, @function

CompareStrings:

	// *** Basic block 0

	.global StringCompareString
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, t0
	mv          t3, t1
	ld          a0, 0(t2)
	ld          a1, 0(t3)
	j           StringCompareString
.func_end_CompareStrings:
	.size CompareStrings, .func_end_CompareStrings-CompareStrings

	.local  CompareStringsCaseBlind
	.type CompareStringsCaseBlind, @function

CompareStringsCaseBlind:

	// *** Basic block 0

	.global StringCompareStringCaseBlind
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, t0
	mv          t3, t1
	ld          a0, 0(t2)
	ld          a1, 0(t3)
	j           StringCompareStringCaseBlind
.func_end_CompareStringsCaseBlind:
	.size CompareStringsCaseBlind, .func_end_CompareStringsCaseBlind-CompareStringsCaseBlind

	.local  CompareCharPointers
	.type CompareCharPointers, @function

CompareCharPointers:

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
.func_end_CompareCharPointers:
	.size CompareCharPointers, .func_end_CompareCharPointers-CompareCharPointers

	.local  CompareCharPointersCaseBlind
	.type CompareCharPointersCaseBlind, @function

CompareCharPointersCaseBlind:

	// *** Basic block 0

	.global strcasecmp
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, t0
	mv          t3, t1
	ld          a0, 0(t2)
	ld          a1, 0(t3)
	j           strcasecmp
.func_end_CompareCharPointersCaseBlind:
	.size CompareCharPointersCaseBlind, .func_end_CompareCharPointersCaseBlind-CompareCharPointersCaseBlind

	.local  CompareMappedInt64s
	.type CompareMappedInt64s, @function

CompareMappedInt64s:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	ld          t2, 0(t0)
	ld          t3, 0(t1)
	sub         t2, t2, t3
	sext.w      a0, t2

	// *** Basic block 1

.CompareMappedInt64s_label_20:
	ret         
.func_end_CompareMappedInt64s:
	.size CompareMappedInt64s, .func_end_CompareMappedInt64s-CompareMappedInt64s

	.local  CompareMappedPointers
	.type CompareMappedPointers, @function

CompareMappedPointers:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	ld          t2, 0(t0)
	ld          t3, 0(t1)
	sub         t2, t2, t3
	div         t2, t2, x0
	sext.w      a0, t2

	// *** Basic block 1

.CompareMappedPointers_label_21:
	ret         
.func_end_CompareMappedPointers:
	.size CompareMappedPointers, .func_end_CompareMappedPointers-CompareMappedPointers

	.global MapInitForStringKeys
	.type MapInitForStringKeys, @function

MapInitForStringKeys:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global MapInit
	.local CompareStrings
	la          a1, CompareStrings
	j           MapInit
.func_end_MapInitForStringKeys:
	.size MapInitForStringKeys, .func_end_MapInitForStringKeys-MapInitForStringKeys

	.global MapInitForCaseBlindStringKeys
	.type MapInitForCaseBlindStringKeys, @function

MapInitForCaseBlindStringKeys:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global MapInit
	.local CompareStringsCaseBlind
	la          a1, CompareStringsCaseBlind
	j           MapInit
.func_end_MapInitForCaseBlindStringKeys:
	.size MapInitForCaseBlindStringKeys, .func_end_MapInitForCaseBlindStringKeys-MapInitForCaseBlindStringKeys

	.global MapInitForPointerKeys
	.type MapInitForPointerKeys, @function

MapInitForPointerKeys:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global MapInit
	.local CompareMappedPointers
	la          a1, CompareMappedPointers
	j           MapInit
.func_end_MapInitForPointerKeys:
	.size MapInitForPointerKeys, .func_end_MapInitForPointerKeys-MapInitForPointerKeys

	.global MapInitForCharPointerKeys
	.type MapInitForCharPointerKeys, @function

MapInitForCharPointerKeys:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global MapInit
	.local CompareCharPointers
	la          a1, CompareCharPointers
	j           MapInit
.func_end_MapInitForCharPointerKeys:
	.size MapInitForCharPointerKeys, .func_end_MapInitForCharPointerKeys-MapInitForCharPointerKeys

	.global MapInitForCaseBlindCharPointerKeys
	.type MapInitForCaseBlindCharPointerKeys, @function

MapInitForCaseBlindCharPointerKeys:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global MapInit
	.local CompareCharPointersCaseBlind
	la          a1, CompareCharPointersCaseBlind
	j           MapInit
.func_end_MapInitForCaseBlindCharPointerKeys:
	.size MapInitForCaseBlindCharPointerKeys, .func_end_MapInitForCaseBlindCharPointerKeys-MapInitForCaseBlindCharPointerKeys

	.global MapInitForInt64Keys
	.type MapInitForInt64Keys, @function

MapInitForInt64Keys:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global MapInit
	.local CompareMappedInt64s
	la          a1, CompareMappedInt64s
	j           MapInit
.func_end_MapInitForInt64Keys:
	.size MapInitForInt64Keys, .func_end_MapInitForInt64Keys-MapInitForInt64Keys

	.global NewMap
	.type NewMap, @function

NewMap:

	// *** Basic block 0

	.global malloc
	.global MapInit
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
	li          a0, 32		// 0x20 ASCII ' '
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        MapInit

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.NewMap_label_21:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewMap:
	.size NewMap, .func_end_NewMap-NewMap

	.global MapDestruct
	.type MapDestruct, @function

MapDestruct:

	// *** Basic block 0

	.global free
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 0(t0)
	j           free
.func_end_MapDestruct:
	.size MapDestruct, .func_end_MapDestruct-MapDestruct

	.global MapDelete
	.type MapDelete, @function

MapDelete:

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
	.global MapDestruct
	.global free
	mv          s1, a0
	call        MapDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_MapDelete:
	.size MapDelete, .func_end_MapDelete-MapDelete

	.global MapClone
	.type MapClone, @function

MapClone:

	// *** Basic block 0

	.global malloc
	.global memcpy
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
	ld          t0, 16(s2)
	sd          t0, 16(s1)
	ld          t0, 8(s2)
	sd          t0, 8(s1)
	ld          s3, 0(s2)
	bne         s3, x0, .MapClone_label_30

	// *** Basic block 1

	sd          x0, 0(s1)
	j           .MapClone_label_49

	// *** Basic block 2

.MapClone_label_30:
	ld          t0, 16(s1)
	slli        a0, t0, 4
	call        malloc

	// *** Basic block 3

	sd          a0, 0(s1)
	ld          a0, 0(s1)
	ld          t0, 16(s1)
	slli        a2, t0, 4
	mv          a1, s3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           memcpy

	// *** Basic block 4

.MapClone_label_49:
	ld          t0, 24(s2)
	sd          t0, 24(s1)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_MapClone:
	.size MapClone, .func_end_MapClone-MapClone

	.global MapDestructWithContents
	.type MapDestructWithContents, @function

MapDestructWithContents:

	// *** Basic block 0

	.global MapDestruct
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
	mv          s3, x0
	ld          t0, 8(s1)
	bge         x0, t0, .MapDestructWithContents_label_38

	// *** Basic block 2

.MapDestructWithContents_label_23:
	beq         s2, x0, .MapDestructWithContents_label_31

	// *** Basic block 3

	ld          t0, 0(s1)
	slli        t1, s3, 4
	add         a0, t0, t1
	jalr         x1, s2, 0

	// *** Basic block 4

.MapDestructWithContents_label_31:

	// *** Basic block 5

.MapDestructWithContents_label_32:
	addi        s3, s3, 1
	ld          t0, 8(s1)
	bge         s3, t0, .MapDestructWithContents_label_23

	// *** Basic block 6

.MapDestructWithContents_label_38:
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           MapDestruct
.func_end_MapDestructWithContents:
	.size MapDestructWithContents, .func_end_MapDestructWithContents-MapDestructWithContents

	.global MapDeleteWithContents
	.type MapDeleteWithContents, @function

MapDeleteWithContents:

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
	.global MapDestructWithContents
	.global free
	mv          s1, a0
	call        MapDestructWithContents

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_MapDeleteWithContents:
	.size MapDeleteWithContents, .func_end_MapDeleteWithContents-MapDeleteWithContents

	.global MapClear
	.type MapClear, @function

MapClear:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sd          x0, 8(a0)
	ret         
.func_end_MapClear:
	.size MapClear, .func_end_MapClear-MapClear

	.local  MakeSpace
	.type MakeSpace, @function

MakeSpace:

	// *** Basic block 0

	.global malloc
	.global memset
	.global realloc
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
	ld          t0, 0(s1)
	bne         t0, x0, .MakeSpace_label_39

	// *** Basic block 1

	li          t0, 2		// 0x2 ASCII \x2
	sd          t0, 16(s1)
	ld          t0, 16(s1)
	slli        a0, t0, 4
	call        malloc

	// *** Basic block 2

	sd          a0, 0(s1)
	ld          a0, 0(s1)
	ld          t0, 16(s1)
	slli        a2, t0, 4
	mv          a1, x0
	call        memset

	// *** Basic block 3

.MakeSpace_label_39:
	ld          t0, 8(s1)
	addi        t0, t0, 1
	ld          s2, 16(s1)
	bge         s2, t0, .MakeSpace_label_72

	// *** Basic block 4

	ld          t0, 16(s1)
	slli        t1, t0, 1
	sd          t1, 16(s1)
	ld          a0, 0(s1)
	slli        a1, t0, 4
	call        realloc

	// *** Basic block 5

	sd          a0, 0(s1)
	ld          t0, 0(s1)
	slli        t1, s2, 4
	add         a0, t0, t1
	ld          t0, 16(s1)
	sub         t0, t0, s2
	slli        a2, t0, 4
	mv          a1, x0
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           memset

	// *** Basic block 6

.MakeSpace_label_72:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_MakeSpace:
	.size MakeSpace, .func_end_MakeSpace-MakeSpace

	.local  Append
	.type Append, @function

Append:

	// *** Basic block 0

	.local MakeSpace
	// Leaf procedure, no stack frame generated
	j           MakeSpace
.func_end_Append:
	.size Append, .func_end_Append-Append

	.local  InsertBefore
	.type InsertBefore, @function

InsertBefore:

	// *** Basic block 0

	.global printf
	.global abort
	.local MakeSpace
	.global memmove
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
	ld          s3, 8(s2)
	bge         s1, s3, .InsertBefore_label_30

	// *** Basic block 1

	j           .InsertBefore_label_46

	// *** Basic block 2

.InsertBefore_label_30:
	lla         a0, .str.1
	lla         a1, .str.2
	lla         a3, .str.3
	li          t0, 160		// 0xa0 ASCII \xa0
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.InsertBefore_label_46:
	mv          a0, s2
	call        MakeSpace

	// *** Basic block 5

	sub         s4, s3, s1
	ld          t0, 0(s2)
	slli        t1, s1, 4
	add         a1, t0, t1
	addi        a0, a1, 16
	slli        a2, s4, 4
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           memmove
.func_end_InsertBefore:
	.size InsertBefore, .func_end_InsertBefore-InsertBefore

	.local  Remove
	.type Remove, @function

Remove:

	// *** Basic block 0

	.global printf
	.global abort
	.global memmove
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
	ld          s3, 8(s2)
	bge         s1, s3, .Remove_label_26

	// *** Basic block 1

	j           .Remove_label_43

	// *** Basic block 2

.Remove_label_26:
	lla         a0, .str.4
	lla         a1, .str.5
	lla         a3, .str.6
	li          t0, 171		// 0xab ASCII \xab
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.Remove_label_43:
	sub         t0, s3, s1
	addi        s3, t0, -1
	ld          t0, 0(s2)
	slli        t1, s1, 4
	add         a0, t0, t1
	addi        a1, a0, 16
	slli        a2, s3, 4
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           memmove
.func_end_Remove:
	.size Remove, .func_end_Remove-Remove

	.local  FindLocation
	.type FindLocation, @function

FindLocation:

	// *** Basic block 0

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
	mv          t0, a2
	mv          s1, a0
	mv          s2, a1
	sb          x0, 0(t0)
	mv          s3, x0
	ld          s4, 8(s1)
	bge         x0, s4, .FindLocation_label_65

	// *** Basic block 1

	li          t1, 1		// 0x1 ASCII \x1
	sb          t1, 0(t0)

	// *** Basic block 2

.FindLocation_label_30:
	sub         t0, s4, s3
	srli        t0, t0, 1
	add         s5, s3, t0
	ld          t0, 24(s1)
	ld          t1, 0(s1)
	slli        s6, s5, 4
	add         a1, t1, s6
	mv          a0, s2
	jalr         x1, t0, 0

	// *** Basic block 3

	mv          s7, a0
	bnez        s7, .FindLocation_label_55

	// *** Basic block 4

	ld          t0, 0(s1)
	add         a0, t0, s6

	// *** Basic block 5

.FindLocation_label_52:
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

.FindLocation_label_55:
	bge         s7, x0, .FindLocation_label_60

	// *** Basic block 7

	mv          s4, s5
	j           .FindLocation_label_62

	// *** Basic block 8

.FindLocation_label_60:
	addi        s3, s5, 1

	// *** Basic block 9

.FindLocation_label_62:
	blt         s3, s4, .FindLocation_label_30

	// *** Basic block 10

.FindLocation_label_65:
	ld          t1, 8(s1)
	bne         s4, t1, .FindLocation_label_74

	// *** Basic block 11

	mv          a0, x0
	j           .FindLocation_label_52

	// *** Basic block 12

.FindLocation_label_74:
	ld          t1, 0(s1)
	slli        t2, s4, 4
	add         a0, t1, t2
	j           .FindLocation_label_52
.func_end_FindLocation:
	.size FindLocation, .func_end_FindLocation-FindLocation

	.local  LinearInsert
	.type LinearInsert, @function

LinearInsert:

	// *** Basic block 0

	.local InsertBefore
	.local Append
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
	mv          s3, x0
	ld          t0, 8(s1)
	bge         x0, t0, .LinearInsert_label_73

	// *** Basic block 2

.LinearInsert_label_24:
	ld          t0, 24(s1)
	ld          t1, 0(s1)
	slli        s4, s3, 4
	add         a1, t1, s4
	mv          a0, s2
	jalr         x1, t0, 0

	// *** Basic block 3

	mv          s5, a0
	bnez        s5, .LinearInsert_label_52

	// *** Basic block 4

	ld          s6, 8(a1)
	ld          t0, 0(s1)
	add         t0, t0, s4
	ld          t1, 8(s2)
	sd          t1, 8(t0)
	mv          a0, s6

	// *** Basic block 5

.LinearInsert_label_49:
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

	// *** Basic block 6

.LinearInsert_label_52:
	bge         s5, x0, .LinearInsert_label_66

	// *** Basic block 7

	mv          a2, s2
	mv          a1, s3
	mv          a0, s1
	call        InsertBefore

	// *** Basic block 8

	mv          a0, x0
	j           .LinearInsert_label_49

	// *** Basic block 9

.LinearInsert_label_66:

	// *** Basic block 10

.LinearInsert_label_67:
	addi        s3, s3, 1
	ld          t0, 8(s1)
	bge         s3, t0, .LinearInsert_label_24

	// *** Basic block 11

.LinearInsert_label_73:
	mv          a1, s2
	mv          a0, s1
	call        Append

	// *** Basic block 12

	mv          a0, x0
	j           .LinearInsert_label_49
.func_end_LinearInsert:
	.size LinearInsert, .func_end_LinearInsert-LinearInsert

	.local  BinaryInsert
	.type BinaryInsert, @function

BinaryInsert:

	// *** Basic block 0

	.local FindLocation
	.local Append
	.local InsertBefore
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
	mv          s2, a1
	addi        a2, s0, -32
	call        FindLocation

	// *** Basic block 1

	mv          s3, a0
	bne         s3, x0, .BinaryInsert_label_40

	// *** Basic block 2

	mv          a1, s2
	mv          a0, s1
	call        Append

	// *** Basic block 3

	mv          a0, x0

	// *** Basic block 4

.BinaryInsert_label_37:
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

.BinaryInsert_label_40:
	lb          t0, -32(s0)
	beqz        t0, .BinaryInsert_label_53

	// *** Basic block 6

	ld          s4, 8(s3)
	ld          t0, 8(s2)
	sd          t0, 8(s3)
	mv          a0, s4
	j           .BinaryInsert_label_37

	// *** Basic block 7

.BinaryInsert_label_53:
	ld          t0, 0(s1)
	sub         t0, s3, t0
	srai        a1, t0, 4
	mv          a2, s2
	mv          a0, s1
	call        InsertBefore

	// *** Basic block 8

	mv          a0, x0
	j           .BinaryInsert_label_37
.func_end_BinaryInsert:
	.size BinaryInsert, .func_end_BinaryInsert-BinaryInsert

	.global MapInsert
	.type MapInsert, @function

MapInsert:

	// *** Basic block 0

	.local LinearInsert
	.local BinaryInsert
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 8(t0)
	li          t2, 5		// 0x5 ASCII \x5
	bge         t1, t2, .MapInsert_label_30

	// *** Basic block 1

	addi        a1, s0, 0
	mv          a0, t0
	j           LinearInsert

	// *** Basic block 4

.MapInsert_label_30:
	addi        a1, s0, 0
	mv          a0, t0
	j           BinaryInsert
.func_end_MapInsert:
	.size MapInsert, .func_end_MapInsert-MapInsert

	.global MapFind
	.type MapFind, @function

MapFind:

	// *** Basic block 0

	.global bsearch
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          t0, a1
	mv          t1, a0
	ld          t2, 0(t0)
	sd          t2, -32(s0)
	addi        a0, s0, -32
	ld          a1, 0(t1)
	ld          a2, 8(t1)
	ld          a4, 24(t1)
	li          a3, 16		// 0x10 ASCII \x10
	call        bsearch

	// *** Basic block 1

	mv          s1, a0
	bne         s1, x0, .MapFind_label_44

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.MapFind_label_41:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.MapFind_label_44:
	ld          a0, 8(s1)
	j           .MapFind_label_41
.func_end_MapFind:
	.size MapFind, .func_end_MapFind-MapFind

	.global MapSearch
	.type MapSearch, @function

MapSearch:

	// *** Basic block 0

	.global bsearch
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          t0, a1
	mv          t1, a0
	ld          t2, 0(t0)
	sd          t2, -32(s0)
	addi        a0, s0, -32
	ld          a1, 0(t1)
	ld          a2, 8(t1)
	ld          a4, 24(t1)
	li          a3, 16		// 0x10 ASCII \x10
	call        bsearch

	// *** Basic block 1

	mv          s1, a0
	bne         s1, x0, .MapSearch_label_44

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.MapSearch_label_41:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.MapSearch_label_44:
	addi        a0, s1, 8
	j           .MapSearch_label_41
.func_end_MapSearch:
	.size MapSearch, .func_end_MapSearch-MapSearch

	.global MapFindPointerKey
	.type MapFindPointerKey, @function

MapFindPointerKey:

	// *** Basic block 0

	.global MapFind
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -32(s0)
	// End of stack frame
	mv          t0, a1
	sd          t0, -32(s0)
	ld          a1, -32(s0)
	call        MapFind

	// *** Basic block 1


	// *** Basic block 2

.MapFindPointerKey_label_21:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_MapFindPointerKey:
	.size MapFindPointerKey, .func_end_MapFindPointerKey-MapFindPointerKey

	.global MapFindInt64Key
	.type MapFindInt64Key, @function

MapFindInt64Key:

	// *** Basic block 0

	.global MapFind
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -32(s0)
	// End of stack frame
	mv          t0, a1
	sd          t0, -32(s0)
	ld          a1, -32(s0)
	call        MapFind

	// *** Basic block 1


	// *** Basic block 2

.MapFindInt64Key_label_21:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_MapFindInt64Key:
	.size MapFindInt64Key, .func_end_MapFindInt64Key-MapFindInt64Key

	.global MapRemove
	.type MapRemove, @function

MapRemove:

	// *** Basic block 0

	.global bsearch
	.local Remove
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
	mv          t0, a1
	mv          s1, a0
	ld          t1, 0(t0)
	sd          t1, -32(s0)
	addi        a0, s0, -32
	ld          s2, 0(s1)
	ld          a2, 8(s1)
	ld          a4, 24(s1)
	li          a3, 16		// 0x10 ASCII \x10
	mv          a1, s2
	call        bsearch

	// *** Basic block 1

	mv          s3, a0
	bne         s3, x0, .MapRemove_label_47

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.MapRemove_label_44:
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

.MapRemove_label_47:
	ld          s4, 8(s3)
	sub         t0, s3, s2
	srai        a1, t0, 4
	mv          a0, s1
	call        Remove

	// *** Basic block 5

	mv          a0, s4
	j           .MapRemove_label_44
.func_end_MapRemove:
	.size MapRemove, .func_end_MapRemove-MapRemove

	.global MapCopy
	.type MapCopy, @function

MapCopy:

	// *** Basic block 0

	.global MapInsert
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
	ld          t0, 8(s1)
	bge         x0, t0, .MapCopy_label_45

	// *** Basic block 1

.MapCopy_label_21:
	ld          t0, 0(s1)
	slli        t1, s3, 4
	add         s4, t0, t1
	addi        sp, sp, -16
	ld          t0, 0(s4)
	sd          t0, 0(sp)
	ld          t0, 8(s4)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s2
	call        MapInsert

	// *** Basic block 3

.MapCopy_label_39:
	addi        s3, s3, 1
	ld          t0, 8(s1)
	bge         s3, t0, .MapCopy_label_21

	// *** Basic block 4

.MapCopy_label_45:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_MapCopy:
	.size MapCopy, .func_end_MapCopy-MapCopy

	.global MapPrint
	.type MapPrint, @function

MapPrint:

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
	mv          s1, a0
	mv          s2, a1
	lla         a0, .str.7
	call        printf

	// *** Basic block 1

	lla         s3, .str.8
	mv          s4, x0
	ld          t0, 8(s1)
	bge         x0, t0, .MapPrint_label_53

	// *** Basic block 2

.MapPrint_label_32:
	lla         a0, .str.9
	mv          a1, s3
	call        printf

	// *** Basic block 3

	ld          t0, 0(s1)
	slli        t1, s4, 4
	add         a0, t0, t1
	jalr         x1, s2, 0

	// *** Basic block 5

.MapPrint_label_47:
	addi        s4, s4, 1
	ld          t0, 8(s1)
	bge         s4, t0, .MapPrint_label_32

	// *** Basic block 6

.MapPrint_label_53:
	lla         a0, .str.11
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           printf
.func_end_MapPrint:
	.size MapPrint, .func_end_MapPrint-MapPrint

	.global MapTraverse
	.type MapTraverse, @function

MapTraverse:

	// *** Basic block 0

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
	mv          s3, a1
	mv          s4, x0
	ld          t0, 8(s1)
	bge         x0, t0, .MapTraverse_label_37

	// *** Basic block 1

.MapTraverse_label_22:
	ld          t0, 0(s1)
	slli        t1, s4, 4
	add         a0, t0, t1
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 2

.MapTraverse_label_31:
	addi        s4, s4, 1
	ld          t0, 8(s1)
	bge         s4, t0, .MapTraverse_label_22

	// *** Basic block 3

.MapTraverse_label_37:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_MapTraverse:
	.size MapTraverse, .func_end_MapTraverse-MapTraverse

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.1, @object
	.size .str.1, 30

.str.2:
	.asciz "map.c"
	.type .str.2, @object
	.size .str.2, 6

.str.3:
	.asciz "index < map->length"
	.type .str.3, @object
	.size .str.3, 20

.str.4:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.4, @object
	.size .str.4, 30

.str.5:
	.asciz "map.c"
	.type .str.5, @object
	.size .str.5, 6

.str.6:
	.asciz "index < map->length"
	.type .str.6, @object
	.size .str.6, 20

.str.7:
	.asciz "{"
	.type .str.7, @object
	.size .str.7, 2

.str.8:
	.asciz "(null)"
	.type .str.8, @object
	.size .str.8, 1

.str.9:
	.asciz "%s"
	.type .str.9, @object
	.size .str.9, 3

.str.10:
	.asciz ", "
	.type .str.10, @object
	.size .str.10, 3

.str.11:
	.asciz "}"
	.type .str.11, @object
	.size .str.11, 2

