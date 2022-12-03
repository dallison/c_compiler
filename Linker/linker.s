	.file   "linker.c"
	.text
	.option pic
.PCbegin:
	.global LinkerError
	.type LinkerError, @function

LinkerError:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global VLinkerError
	mv          t0, s0
	mv          a2, t0
	j           VLinkerError
.func_end_LinkerError:
	.size LinkerError, .func_end_LinkerError-LinkerError

	.global VLinkerError
	.type VLinkerError, @function

VLinkerError:

	// *** Basic block 0

	.global vsnprintf
	.global fprintf
	.global stderr
	addi sp, sp, -1088
	// Saved return address (offset 1080) and frame pointer (offset 1072)
	sd ra, 1080(sp)
	sd s0, 1072(sp)
	addi s0, sp, 1088
	// Local vars at offset -1040(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	lla         s4, .str.1
	beq         s1, x0, .VLinkerError_label_33

	// *** Basic block 1

	ld          t0, 40(s1)
	addi        t0, t0, 8
	ld          s4, 16(t0)

	// *** Basic block 2

.VLinkerError_label_33:
	addi        s5, s0, -1040
	mv          a3, s3
	mv          a2, s2
	li          t0, 1024		// 0x400
	mv          a1, t0
	mv          a0, s5
	call        vsnprintf

	// *** Basic block 3

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.2
	mv          a3, s5
	mv          a2, s4
	call        fprintf

	// *** Basic block 4

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
.func_end_VLinkerError:
	.size VLinkerError, .func_end_VLinkerError-VLinkerError

	.global LinkerWarning
	.type LinkerWarning, @function

LinkerWarning:

	// *** Basic block 0

	.global VLinkerWarning
	// Leaf procedure, no stack frame generated
	mv          t0, a2
	mv          t1, a0
	mv          t2, a1
	mv          t3, s0
	beq         t1, x0, .LinkerWarning_label_32

	// *** Basic block 2

.LinkerWarning_label_32:
	mv          a3, t3
	mv          a2, t0
	mv          a1, t2
	mv          a0, t1
	j           VLinkerWarning
.func_end_LinkerWarning:
	.size LinkerWarning, .func_end_LinkerWarning-LinkerWarning

	.global VLinkerWarning
	.type VLinkerWarning, @function

VLinkerWarning:

	// *** Basic block 0

	.global vsnprintf
	.global fprintf
	.global stderr
	addi sp, sp, -1088
	// Saved return address (offset 1080) and frame pointer (offset 1072)
	sd ra, 1080(sp)
	sd s0, 1072(sp)
	addi s0, sp, 1088
	// Local vars at offset -1040(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a2
	mv          s3, a3
	mv          s4, a1
	lla         s5, .str.4
	beq         s1, x0, .VLinkerWarning_label_36

	// *** Basic block 1

	ld          t0, 40(s1)
	addi        t0, t0, 8
	ld          s5, 16(t0)

	// *** Basic block 2

.VLinkerWarning_label_36:
	addi        s6, s0, -1040
	mv          a3, s3
	mv          a2, s2
	li          t0, 1024		// 0x400
	mv          a1, t0
	mv          a0, s6
	call        vsnprintf

	// *** Basic block 3

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.5
	mv          a4, s6
	mv          a3, s4
	mv          a2, s5
	call        fprintf

	// *** Basic block 4

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
.func_end_VLinkerWarning:
	.size VLinkerWarning, .func_end_VLinkerWarning-VLinkerWarning

	.global LinkerInit
	.type LinkerInit, @function

LinkerInit:

	// *** Basic block 0

	.global StringInit
	.global VectorInit
	.global HashTableInit
	.global SymbolHash
	.global SymbolInsertInHashTable
	.global SymbolFindInHashTable
	.global VectorAppend
	.global NewString
	.global NewPCodeLinkerArchitecture
	.global NewRISCVLinkerArchitecture
	.global New6502LinkerArchitecture
	.global getenv
	.global StringAppendSegment
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
	lla         a1, .str.6
	call        StringInit

	// *** Basic block 1

	addi        a0, s1, 40
	call        VectorInit

	// *** Basic block 2

	addi        a0, s1, 64
	call        VectorInit

	// *** Basic block 3

	addi        a0, s1, 160
	lla         a1, .str.7
	la          t0, SymbolFindInHashTable
	mv          a5, t0
	la          t0, SymbolInsertInHashTable
	mv          a4, t0
	la          t0, SymbolHash
	mv          a3, t0
	li          t0, 1009		// 0x3f1
	mv          a2, t0
	call        HashTableInit

	// *** Basic block 4

	addi        a0, s1, 248
	call        VectorInit

	// *** Basic block 5

	addi        a0, s1, 448
	call        VectorInit

	// *** Basic block 6

	addi        a0, s1, 472
	call        VectorInit

	// *** Basic block 7

	addi        a0, s1, 496
	call        VectorInit

	// *** Basic block 8

	addi        a0, s1, 544
	call        VectorInit

	// *** Basic block 9

	addi        a0, s1, 96
	call        VectorInit

	// *** Basic block 10

	addi        a0, s1, 120
	lla         a1, .str.8
	call        StringInit

	// *** Basic block 11

	addi        s2, s1, 448
	lla         a0, .str.9
	call        NewString

	// *** Basic block 12

	mv          a1, a0
	mv          a0, s2
	call        VectorAppend

	// *** Basic block 13

	addi        s2, s1, 448
	lla         a0, .str.10
	call        NewString

	// *** Basic block 14

	mv          a1, a0
	mv          a0, s2
	call        VectorAppend

	// *** Basic block 15

	sw          x0, 520(s1)
	sw          x0, 524(s1)
	sb          x0, 528(s1)
	li          t0, -1		// 0xffffffffffffffff
	sw          t0, 568(s1)
	sb          x0, 529(s1)
	sd          x0, 576(s1)
	sb          x0, 585(s1)
	sb          x0, 584(s1)
	sb          x0, 586(s1)
	addi        s2, s1, 64
	call        NewPCodeLinkerArchitecture

	// *** Basic block 16

	mv          a1, a0
	mv          a0, s2
	call        VectorAppend

	// *** Basic block 17

	addi        s2, s1, 64
	call        NewRISCVLinkerArchitecture

	// *** Basic block 18

	mv          a1, a0
	mv          a0, s2
	call        VectorAppend

	// *** Basic block 19

	addi        s2, s1, 64
	call        New6502LinkerArchitecture

	// *** Basic block 20

	mv          a1, a0
	mv          a0, s2
	call        VectorAppend

	// *** Basic block 21

	lla         a0, .str.11
	call        getenv

	// *** Basic block 22

	mv          s2, a0
	beq         s2, x0, .LinkerInit_label_240

	// *** Basic block 23

	mv          s3, s2
	lb          t0, 0(s3)
	beqz        t0, .LinkerInit_label_239

	// *** Basic block 24

.LinkerInit_label_185:
	mv          s4, s2
	lb          s5, 0(s4)
	addi        t1, s5, -58
	snez        t0, t1
	li          s6, 58		// 0x3a ASCII ':'
	beq         s5, s6, .LinkerInit_label_196

	// *** Basic block 25

	snez        t0, s5

	// *** Basic block 26

.LinkerInit_label_196:
	beqz        t0, .LinkerInit_label_209

	// *** Basic block 27

.LinkerInit_label_198:
	addi        s4, s4, 1
	lb          t1, 0(s4)
	addi        t2, t1, -58
	snez        t0, t2
	beq         t1, s6, .LinkerInit_label_207

	// *** Basic block 28

	snez        t0, t1

	// *** Basic block 29

.LinkerInit_label_207:
	bnez        t0, .LinkerInit_label_198

	// *** Basic block 30

.LinkerInit_label_209:
	lla         a0, .str.12
	call        NewString

	// *** Basic block 31

	mv          s2, a0
	sub         a2, s4, s3
	mv          a1, s3
	mv          a0, s2
	call        StringAppendSegment

	// *** Basic block 32

	addi        a0, s1, 448
	mv          a1, s2
	call        VectorAppend

	// *** Basic block 33

	mv          s3, s4
	lb          t0, 0(s3)
	bne         t0, s6, .LinkerInit_label_235

	// *** Basic block 34

	addi        s3, s3, 1

	// *** Basic block 35

.LinkerInit_label_235:
	lb          t0, 0(s3)
	bnez        t0, .LinkerInit_label_185

	// *** Basic block 36

.LinkerInit_label_239:

	// *** Basic block 37

.LinkerInit_label_240:
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
.func_end_LinkerInit:
	.size LinkerInit, .func_end_LinkerInit-LinkerInit

	.global LinkerInitDynamic
	.type LinkerInitDynamic, @function

LinkerInitDynamic:

	// *** Basic block 0

	.global NewDynamicLinker
	// Leaf procedure, no stack frame generated
	j           NewDynamicLinker
.func_end_LinkerInitDynamic:
	.size LinkerInitDynamic, .func_end_LinkerInitDynamic-LinkerInitDynamic

	.global LinkerDestruct
	.type LinkerDestruct, @function

LinkerDestruct:

	// *** Basic block 0

	.global StringDestruct
	.global VectorDestructWithContents
	.global ObjectFileDestruct
	.global LinkerClearSymbolTable
	.global HashTableDestruct
	.global SectionGroupDestruct
	.global ARArchiveDestruct
	.global LoadedDynamicLibraryDestruct
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
	call        StringDestruct

	// *** Basic block 1

	addi        a0, s1, 40
	la          t0, ObjectFileDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 2

	addi        a0, s1, 64
	mv          a1, x0
	call        VectorDestructWithContents

	// *** Basic block 3

	addi        a0, s1, 160
	call        LinkerClearSymbolTable

	// *** Basic block 4

	addi        a0, s1, 160
	call        HashTableDestruct

	// *** Basic block 5

	addi        a0, s1, 248
	la          t0, SectionGroupDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 6

	addi        a0, s1, 472
	la          t0, ARArchiveDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 7

	addi        a0, s1, 496
	la          t0, LoadedDynamicLibraryDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 8

	addi        a0, s1, 448
	la          t0, StringDestruct
	ld          s2, 0(t0)
	mv          a1, s2
	call        VectorDestructWithContents

	// *** Basic block 9

	addi        a0, s1, 544
	mv          a1, s2
	call        VectorDestructWithContents

	// *** Basic block 10

	addi        a0, s1, 96
	mv          a1, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorDestructWithContents
.func_end_LinkerDestruct:
	.size LinkerDestruct, .func_end_LinkerDestruct-LinkerDestruct

	.global LinkerInitArchitecture
	.type LinkerInitArchitecture, @function

LinkerInitArchitecture:

	// *** Basic block 0

	.global fprintf
	.global stderr
	.global exit
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
	addi        t0, s1, 64
	ld          s3, 8(t0)
	bge         x0, s3, .LinkerInitArchitecture_label_48

	// *** Basic block 1

	ld          t0, 64(s1)
	lw          t1, 520(s1)

	// *** Basic block 2

.LinkerInitArchitecture_label_28:
	slli        t2, s2, 3
	add         t0, t0, t2
	ld          s4, 0(t0)
	lw          t0, 0(s4)
	bne         t0, t1, .LinkerInitArchitecture_label_43

	// *** Basic block 3

	sd          s4, 88(s1)

	// *** Basic block 4

.LinkerInitArchitecture_label_40:
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

.LinkerInitArchitecture_label_43:

	// *** Basic block 6

.LinkerInitArchitecture_label_44:
	addi        s2, s2, 1
	bge         s2, s3, .LinkerInitArchitecture_label_28

	// *** Basic block 7

.LinkerInitArchitecture_label_48:
	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.13
	lw          a2, 520(s1)
	call        fprintf

	// *** Basic block 8

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	call        exit

	// *** Basic block 9

	j           .LinkerInitArchitecture_label_40
.func_end_LinkerInitArchitecture:
	.size LinkerInitArchitecture, .func_end_LinkerInitArchitecture-LinkerInitArchitecture

	.global LinkerAddLibrarySearchDir
	.type LinkerAddLibrarySearchDir, @function

LinkerAddLibrarySearchDir:

	// *** Basic block 0

	.global VectorAppend
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 448
	j           VectorAppend
.func_end_LinkerAddLibrarySearchDir:
	.size LinkerAddLibrarySearchDir, .func_end_LinkerAddLibrarySearchDir-LinkerAddLibrarySearchDir

	.local  FindLibrary
	.type FindLibrary, @function

FindLibrary:

	// *** Basic block 0

	.global StringInit
	.global StringPrintf
	.global stat
	.global StringSet
	.global StringDestruct
	addi sp, sp, -256
	// Saved return address (offset 248) and frame pointer (offset 240)
	sd ra, 248(sp)
	sd s0, 240(sp)
	addi s0, sp, 256
	// Local vars at offset -184(s0)
	// Saved integer registers.
	sd s1, 64(sp)
	sd s2, 56(sp)
	sd s3, 48(sp)
	sd s4, 40(sp)
	sd s5, 32(sp)
	sd s6, 24(sp)
	sd s7, 16(sp)
	sd s8, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a2
	mv          s3, a3
	mv          s4, x0
	mv          s5, x0
	addi        t0, a0, 448
	ld          s6, 8(t0)
	bge         x0, s6, .FindLibrary_label_95

	// *** Basic block 1

	ld          s7, 448(a0)

	// *** Basic block 2

.FindLibrary_label_39:
	slli        t0, s5, 3
	add         t0, s7, t0
	ld          s7, 0(t0)
	addi        a0, s0, -184
	lla         a1, .str.14
	call        StringInit

	// *** Basic block 3

	addi        a0, s0, -184
	lla         a1, .str.15
	ld          a2, 16(s7)
	mv          a4, s2
	mv          a3, s1
	call        StringPrintf

	// *** Basic block 4

	addi        t0, s0, -184
	ld          s7, 16(t0)
	addi        a1, s0, -144
	mv          a0, s7
	call        stat

	// *** Basic block 5

	mv          s8, a0
	bnez        s8, .FindLibrary_label_87

	// *** Basic block 6

	mv          a1, s7
	mv          a0, s3
	call        StringSet

	// *** Basic block 7

	li          s4, 1		// 0x1 ASCII \x1
	j           .FindLibrary_label_95

	// *** Basic block 8

.FindLibrary_label_87:
	addi        a0, s0, -184
	call        StringDestruct

	// *** Basic block 9

.FindLibrary_label_91:
	addi        s5, s5, 1
	bge         s5, s6, .FindLibrary_label_39

	// *** Basic block 10

.FindLibrary_label_95:
	mv          a0, s4

	// *** Basic block 11

.FindLibrary_label_98:
	// Restored registers.
	ld s1, 64(sp)
	ld s2, 56(sp)
	ld s3, 48(sp)
	ld s4, 40(sp)
	ld s5, 32(sp)
	ld s6, 24(sp)
	ld s7, 16(sp)
	ld s8, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_FindLibrary:
	.size FindLibrary, .func_end_FindLibrary-FindLibrary

	.global LinkerAddStaticLibrary
	.type LinkerAddStaticLibrary, @function

LinkerAddStaticLibrary:

	// *** Basic block 0

	.global NewARArchive
	.global fopen
	.global ARArchiveOpen
	.global VectorAppend
	.global fclose
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
	mv          a0, s1
	call        NewARArchive

	// *** Basic block 1

	mv          s3, a0
	lla         a1, .str.16
	mv          a0, s1
	call        fopen

	// *** Basic block 2

	mv          s4, a0
	bne         s4, x0, .LinkerAddStaticLibrary_label_36

	// *** Basic block 3

.LinkerAddStaticLibrary_label_33:
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

.LinkerAddStaticLibrary_label_36:
	mv          a1, s4
	mv          a0, s3
	call        ARArchiveOpen

	// *** Basic block 5

	mv          s1, a0
	not         t0, s1
	beqz        t0, .LinkerAddStaticLibrary_label_47

	// *** Basic block 6

	j           .LinkerAddStaticLibrary_label_33

	// *** Basic block 7

.LinkerAddStaticLibrary_label_47:
	addi        a0, s2, 472
	mv          a1, s3
	call        VectorAppend

	// *** Basic block 8

	mv          a0, s4
	call        fclose

	// *** Basic block 9

	j           .LinkerAddStaticLibrary_label_33
.func_end_LinkerAddStaticLibrary:
	.size LinkerAddStaticLibrary, .func_end_LinkerAddStaticLibrary-LinkerAddStaticLibrary

	.global LinkerAddDynamicLibrary
	.type LinkerAddDynamicLibrary, @function

LinkerAddDynamicLibrary:

	// *** Basic block 0

	.global StringInit
	.global DynamicLoaderFindLibrary
	.global NewLoadedDynamicLibrary
	.global DynamicLibraryRegistryInsert
	.global LoadedDynamicLibraryLoad
	.global StringDestruct
	.global VectorAppend
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -56(s0)
	// Saved integer registers.
	sd s1, 32(sp)
	sd s2, 24(sp)
	sd s3, 16(sp)
	sd s4, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	addi        a0, s0, -56
	call        StringInit

	// *** Basic block 1

	ld          t0, 536(s2)
	addi        a0, t0, 320
	addi        a1, s0, -56
	call        DynamicLoaderFindLibrary

	// *** Basic block 2

	mv          s3, a0
	bne         s3, x0, .LinkerAddDynamicLibrary_label_86

	// *** Basic block 3

	mv          a1, x0
	mv          a0, s1
	call        NewLoadedDynamicLibrary

	// *** Basic block 4

	mv          s3, a0
	ld          t0, 536(s2)
	addi        a0, t0, 320
	mv          a1, s3
	call        DynamicLibraryRegistryInsert

	// *** Basic block 5

	ld          t0, 536(s2)
	addi        a1, t0, 320
	addi        a2, s2, 448
	mv          a4, x0
	mv          a3, x0
	mv          a0, s3
	call        LoadedDynamicLibraryLoad

	// *** Basic block 6

	mv          s4, a0
	not         t0, s4
	beqz        t0, .LinkerAddDynamicLibrary_label_80

	// *** Basic block 7

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 8

.LinkerAddDynamicLibrary_label_77:
	// Restored registers.
	ld s1, 32(sp)
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 9

.LinkerAddDynamicLibrary_label_80:
	addi        a0, s2, 496
	mv          a1, s3
	call        VectorAppend

	// *** Basic block 10

.LinkerAddDynamicLibrary_label_86:
	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 11

	j           .LinkerAddDynamicLibrary_label_77
.func_end_LinkerAddDynamicLibrary:
	.size LinkerAddDynamicLibrary, .func_end_LinkerAddDynamicLibrary-LinkerAddDynamicLibrary

	.global LinkerAddLibrary
	.type LinkerAddLibrary, @function

LinkerAddLibrary:

	// *** Basic block 0

	.global StringInit
	.local FindLibrary
	.global LinkerAddStaticLibrary
	.global LinkerAddDynamicLibrary
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -56(s0)
	// Saved integer registers.
	sd s1, 16(sp)
	sd s2, 8(sp)
	sd s3, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	addi        a0, s0, -56
	lla         a1, .str.17
	call        StringInit

	// *** Basic block 1

	lla         a2, .str.18
	addi        a3, s0, -56
	mv          a1, s2
	mv          a0, s1
	call        FindLibrary

	// *** Basic block 2

	mv          s3, a0
	beqz        s3, .LinkerAddLibrary_label_55

	// *** Basic block 3

	addi        t0, s0, -56
	ld          a1, 16(t0)
	mv          a0, s1
	call        LinkerAddStaticLibrary

	// *** Basic block 4

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0

	// *** Basic block 5

.LinkerAddLibrary_label_52:
	// Restored registers.
	ld s1, 16(sp)
	ld s2, 8(sp)
	ld s3, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.LinkerAddLibrary_label_55:
	lla         a2, .str.19
	addi        a3, s0, -56
	mv          a1, s2
	mv          a0, s1
	call        FindLibrary

	// *** Basic block 7

	mv          s3, a0
	beqz        s3, .LinkerAddLibrary_label_79

	// *** Basic block 8

	addi        t0, s0, -56
	ld          a1, 16(t0)
	mv          a0, s1
	call        LinkerAddDynamicLibrary

	// *** Basic block 9

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .LinkerAddLibrary_label_52

	// *** Basic block 10

.LinkerAddLibrary_label_79:
	mv          a0, x0
	j           .LinkerAddLibrary_label_52
.func_end_LinkerAddLibrary:
	.size LinkerAddLibrary, .func_end_LinkerAddLibrary-LinkerAddLibrary

	.global LinkerFindSymbolInStaticLibraries
	.type LinkerFindSymbolInStaticLibraries, @function

LinkerFindSymbolInStaticLibraries:

	// *** Basic block 0

	.global ARArchiveFindSymbol
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
	mv          s2, a1
	mv          s3, a3
	mv          s4, x0
	addi        t0, a0, 472
	ld          s5, 8(t0)
	bge         x0, s5, .LinkerFindSymbolInStaticLibraries_label_62

	// *** Basic block 1

	ld          s6, 472(a0)
	ld          s7, 0(s1)

	// *** Basic block 2

.LinkerFindSymbolInStaticLibraries_label_32:
	slli        t0, s4, 3
	add         t0, s6, t0
	ld          t0, 0(t0)
	sd          t0, 0(s1)
	mv          a1, s2
	mv          a0, s7
	call        ARArchiveFindSymbol

	// *** Basic block 3

	mv          s6, a0
	beq         s6, x0, .LinkerFindSymbolInStaticLibraries_label_57

	// *** Basic block 4

	ld          t0, 40(s6)
	sd          t0, 0(s3)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0

	// *** Basic block 5

.LinkerFindSymbolInStaticLibraries_label_54:
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

.LinkerFindSymbolInStaticLibraries_label_57:

	// *** Basic block 7

.LinkerFindSymbolInStaticLibraries_label_58:
	addi        s4, s4, 1
	bge         s4, s5, .LinkerFindSymbolInStaticLibraries_label_32

	// *** Basic block 8

.LinkerFindSymbolInStaticLibraries_label_62:
	mv          a0, x0
	j           .LinkerFindSymbolInStaticLibraries_label_54
.func_end_LinkerFindSymbolInStaticLibraries:
	.size LinkerFindSymbolInStaticLibraries, .func_end_LinkerFindSymbolInStaticLibraries-LinkerFindSymbolInStaticLibraries

	.global LinkerFindSymbol
	.type LinkerFindSymbol, @function

LinkerFindSymbol:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global HashTableSearch
	j           HashTableSearch
.func_end_LinkerFindSymbol:
	.size LinkerFindSymbol, .func_end_LinkerFindSymbol-LinkerFindSymbol

	.global LinkerInsertSymbol
	.type LinkerInsertSymbol, @function

LinkerInsertSymbol:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global HashTableInsert
	j           HashTableInsert
.func_end_LinkerInsertSymbol:
	.size LinkerInsertSymbol, .func_end_LinkerInsertSymbol-LinkerInsertSymbol

	.global NewSectionGroup
	.type NewSectionGroup, @function

NewSectionGroup:

	// *** Basic block 0

	.global malloc
	.global StringInit
	.global VectorInit
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
	mv          s3, a2
	mv          s4, a3
	li          a0, 104		// 0x68 ASCII 'h'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	ld          a1, 16(s1)
	mv          a0, s5
	call        StringInit

	// *** Basic block 2

	addi        a0, s5, 40
	call        VectorInit

	// *** Basic block 3

	sw          s2, 72(s5)
	sd          s3, 80(s5)
	sd          s4, 88(s5)
	sd          x0, 64(s5)
	sd          x0, 96(s5)
	mv          a0, s5

	// *** Basic block 4

.NewSectionGroup_label_54:
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
.func_end_NewSectionGroup:
	.size NewSectionGroup, .func_end_NewSectionGroup-NewSectionGroup

	.global SectionGroupDestruct
	.type SectionGroupDestruct, @function

SectionGroupDestruct:

	// *** Basic block 0

	.global StringDestruct
	.global VectorDestructWithContents
	.global GroupedSectionDestruct
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
	call        StringDestruct

	// *** Basic block 1

	addi        a0, s1, 40
	la          t0, GroupedSectionDestruct
	ld          a1, 0(t0)
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorDestructWithContents
.func_end_SectionGroupDestruct:
	.size SectionGroupDestruct, .func_end_SectionGroupDestruct-SectionGroupDestruct

	.global SectionGroupDelete
	.type SectionGroupDelete, @function

SectionGroupDelete:

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
	.global SectionGroupDestruct
	.global free
	mv          s1, a0
	call        SectionGroupDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_SectionGroupDelete:
	.size SectionGroupDelete, .func_end_SectionGroupDelete-SectionGroupDelete

	.global NewExistingGroupedSection
	.type NewExistingGroupedSection, @function

NewExistingGroupedSection:

	// *** Basic block 0

	.global malloc
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
	sw          x0, 0(s2)
	sd          s1, 8(s2)
	mv          a0, s2

	// *** Basic block 2

.NewExistingGroupedSection_label_20:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewExistingGroupedSection:
	.size NewExistingGroupedSection, .func_end_NewExistingGroupedSection-NewExistingGroupedSection

	.global NewGroupedSection
	.type NewGroupedSection, @function

NewGroupedSection:

	// *** Basic block 0

	.global malloc
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
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 0(s2)
	sd          s1, 8(s2)
	mv          a0, s2

	// *** Basic block 2

.NewGroupedSection_label_22:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewGroupedSection:
	.size NewGroupedSection, .func_end_NewGroupedSection-NewGroupedSection

	.global GroupedSectionDestruct
	.type GroupedSectionDestruct, @function

GroupedSectionDestruct:

	// *** Basic block 0

	.global ELFWriterSectionDelete
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	lw          t1, 0(t0)
	li          t2, 1		// 0x1 ASCII \x1
	bne         t1, t2, .GroupedSectionDestruct_label_21

	// *** Basic block 1

	ld          a0, 8(t0)
	j           ELFWriterSectionDelete

	// *** Basic block 2

.GroupedSectionDestruct_label_21:
	ret         
.func_end_GroupedSectionDestruct:
	.size GroupedSectionDestruct, .func_end_GroupedSectionDestruct-GroupedSectionDestruct

	.global SegmentInit
	.type SegmentInit, @function

SegmentInit:

	// *** Basic block 0

	.global VectorInit
	// Leaf procedure, no stack frame generated
	j           VectorInit
.func_end_SegmentInit:
	.size SegmentInit, .func_end_SegmentInit-SegmentInit

	.global SegmentDestruct
	.type SegmentDestruct, @function

SegmentDestruct:

	// *** Basic block 0

	.global VectorDestruct
	// Leaf procedure, no stack frame generated
	j           VectorDestruct
.func_end_SegmentDestruct:
	.size SegmentDestruct, .func_end_SegmentDestruct-SegmentDestruct

	.local  ReadSymbolTables
	.type ReadSymbolTables, @function

ReadSymbolTables:

	// *** Basic block 0

	.global VectorInit
	.global ELFReaderFileFindSectionsByType
	.global LinkerError
	.global LinkerReadSymbol
	.global VectorDestruct
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -40(s0)
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
	// End of stack frame
	mv          s1, a1
	mv          s2, a2
	mv          s3, a0
	addi        a0, s0, -40
	call        VectorInit

	// *** Basic block 1

	addi        a2, s0, -40
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s1
	call        ELFReaderFileFindSectionsByType

	// *** Basic block 2

	ld          s4, -40(s0)
	addi        t0, s1, 56
	ld          s5, 8(t0)
	ld          s6, 56(s1)
	ld          s7, 0(s1)
	mv          s8, x0
	addi        t0, s0, -40
	ld          s9, 8(t0)
	bge         x0, s9, .ReadSymbolTables_label_118

	// *** Basic block 3

.ReadSymbolTables_label_54:
	slli        t0, s8, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	ld          s4, 0(s4)
	lwu         s10, 40(s4)
	blt         s10, s5, .ReadSymbolTables_label_73

	// *** Basic block 4

	lla         a1, .str.20
	mv          a0, s2
	call        LinkerError

	// *** Basic block 5

	j           .ReadSymbolTables_label_114

	// *** Basic block 6

.ReadSymbolTables_label_73:
	slli        t0, s10, 3
	add         t0, s6, t0
	ld          s5, 0(t0)
	ld          t0, 32(s4)
	ld          t1, 56(s4)
	div         s6, t0, t1
	ld          t0, 24(s4)
	add         s4, s7, t0
	mv          s7, x0
	bge         x0, s6, .ReadSymbolTables_label_113

	// *** Basic block 7

.ReadSymbolTables_label_92:
	mv          s10, s4
	mv          a4, s10
	mv          a3, s5
	mv          a2, s1
	mv          a1, s2
	mv          a0, s3
	call        LinkerReadSymbol

	// *** Basic block 9

.ReadSymbolTables_label_109:
	addi        s7, s7, 1
	bge         s7, s6, .ReadSymbolTables_label_92

	// *** Basic block 10

.ReadSymbolTables_label_113:

	// *** Basic block 11

.ReadSymbolTables_label_114:
	addi        s8, s8, 1
	bge         s8, s9, .ReadSymbolTables_label_54

	// *** Basic block 12

.ReadSymbolTables_label_118:
	addi        a0, s0, -40
	call        VectorDestruct

	// *** Basic block 13

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
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ReadSymbolTables:
	.size ReadSymbolTables, .func_end_ReadSymbolTables-ReadSymbolTables

	.local  ReadRelocations
	.type ReadRelocations, @function

ReadRelocations:

	// *** Basic block 0

	.global VectorInit
	.global ELFReaderFileFindSectionsByType
	.global LinkerError
	.global LinkerReadRelocation
	.global VectorDestruct
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -40(s0)
	// Spilled register region: 16 bytes at -56(s0) to -40(s0)
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
	mv          s2, a2
	mv          s3, a0
	addi        a0, s0, -40
	call        VectorInit

	// *** Basic block 1

	addi        a2, s0, -40
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	mv          a0, s1
	call        ELFReaderFileFindSectionsByType

	// *** Basic block 2

	addi        t0, s1, 56
	ld          s4, 8(t0)
	sd          s4, -56(s0)	// Spilled @45
	ld          s5, 56(s1)
	ld          s6, 0(s1)
	addi        a2, s0, -40
	li          t0, 9		// 0x9 ASCII \x9
	mv          a1, t0
	mv          a0, s1
	call        ELFReaderFileFindSectionsByType

	// *** Basic block 3

	ld          s7, -40(s0)
	mv          s8, x0
	addi        t0, s0, -40
	ld          s9, 8(t0)
	sd          s9, -48(s0)	// Spilled @61
	bge         x0, s9, .ReadRelocations_label_160

	// *** Basic block 4

.ReadRelocations_label_64:
	slli        t0, s8, 3
	add         t0, s7, t0
	ld          s7, 0(t0)
	ld          s10, 0(s7)
	lwu         s11, 40(s10)
	blt         s11, s4, .ReadRelocations_label_84

	// *** Basic block 5

	lla         a1, .str.21
	mv          a0, s2
	call        LinkerError

	// *** Basic block 6

	j           .ReadRelocations_label_156

	// *** Basic block 7

.ReadRelocations_label_84:
	slli        t0, s11, 3
	add         t0, s5, t0
	ld          s11, 0(t0)
	ld          s9, 0(s11)
	lwu         s4, 40(s9)
	ld          t0, -56(s0)	// Spilled @45
	blt         s4, t0, .ReadRelocations_label_102

	// *** Basic block 8

	lla         a1, .str.22
	mv          a0, s2
	call        LinkerError

	// *** Basic block 9

	j           .ReadRelocations_label_156

	// *** Basic block 10

.ReadRelocations_label_102:
	slli        t0, s4, 3
	add         t0, s5, t0
	ld          s4, 0(t0)
	ld          t0, 24(s9)
	add         s5, s6, t0
	ld          t0, 32(s10)
	ld          t1, 56(s10)
	div         s9, t0, t1
	ld          t0, 24(s10)
	add         s10, s6, t0
	mv          s6, x0
	bge         x0, s9, .ReadRelocations_label_155

	// *** Basic block 11

.ReadRelocations_label_125:
	mv          s4, s10
	ld          a7, -48(s0)	// Spilled @106
	sd          s4, -48(s0)	// Spilled @106
	mv          a6, s11
	mv          a5, s7
	mv          a4, s5
	mv          a3, s4
	mv          a2, s1
	mv          a1, s2
	mv          a0, s3
	call        LinkerReadRelocation

	// *** Basic block 13

.ReadRelocations_label_151:
	addi        s6, s6, 1
	bge         s6, s9, .ReadRelocations_label_125

	// *** Basic block 14

.ReadRelocations_label_155:

	// *** Basic block 15

.ReadRelocations_label_156:
	addi        s8, s8, 1
	ld          t0, -48(s0)	// Spilled @61
	bge         s8, t0, .ReadRelocations_label_64

	// *** Basic block 16

.ReadRelocations_label_160:
	addi        a0, s0, -40
	call        VectorDestruct

	// *** Basic block 17

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
.func_end_ReadRelocations:
	.size ReadRelocations, .func_end_ReadRelocations-ReadRelocations

	.local  ReadELFContents
	.type ReadELFContents, @function

ReadELFContents:

	// *** Basic block 0

	.global MapInsert
	.global MapFindInt64Key
	.global NewVector
	.global VectorAppend
	.local ReadSymbolTables
	.local ReadRelocations
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
	sd s8, 0(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a2
	mv          s3, a0
	mv          s4, x0
	addi        t0, s1, 56
	ld          s5, 8(t0)
	bge         x0, s5, .ReadELFContents_label_102

	// *** Basic block 1

	ld          s6, 56(s1)

	// *** Basic block 2

.ReadELFContents_label_34:
	slli        t0, s4, 3
	add         t0, s6, t0
	ld          s6, 0(t0)
	addi        t0, s6, 8
	sd          t0, -48(s0)
	addi        t0, s0, -48
	sd          s6, 8(t0)
	addi        a0, s2, 168
	addi        sp, sp, -16
	ld          t0, -48(s0)
	sd          t0, 0(sp)
	ld          t0, -40(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	call        MapInsert

	// *** Basic block 3

	addi        sp, sp, 16
	addi        a0, s2, 200
	ld          t0, 0(s6)
	lwu         s7, 4(t0)
	mv          a1, s7
	call        MapFindInt64Key

	// *** Basic block 4

	mv          s8, a0
	bne         s8, x0, .ReadELFContents_label_92

	// *** Basic block 5

	call        NewVector

	// *** Basic block 6

	mv          s8, a0
	sd          s7, -32(s0)
	addi        t0, s0, -32
	sd          s8, 8(t0)
	addi        a0, s2, 200
	addi        sp, sp, -16
	ld          t0, -32(s0)
	sd          t0, 0(sp)
	ld          t0, -24(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	call        MapInsert

	// *** Basic block 8

.ReadELFContents_label_92:
	mv          a1, s6
	mv          a0, s8
	call        VectorAppend

	// *** Basic block 9

.ReadELFContents_label_98:
	addi        s4, s4, 1
	bge         s4, s5, .ReadELFContents_label_34

	// *** Basic block 10

.ReadELFContents_label_102:
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        ReadSymbolTables

	// *** Basic block 11

	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        ReadRelocations

	// *** Basic block 12

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
.func_end_ReadELFContents:
	.size ReadELFContents, .func_end_ReadELFContents-ReadELFContents

	.local  CheckMachineType
	.type CheckMachineType, @function

CheckMachineType:

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
	mv          s1, a0
	mv          s2, a1
	addi        t0, s1, 40
	ld          t0, 8(t0)
	li          s3, 1		// 0x1 ASCII \x1
	bne         t0, s3, .CheckMachineType_label_40

	// *** Basic block 1

	ld          t0, 40(s2)
	ld          t0, 0(t0)
	lhu         t1, 18(t0)
	sw          t1, 520(s1)
	lwu         t0, 48(t0)
	sw          t0, 524(s1)
	j           .CheckMachineType_label_63

	// *** Basic block 2

.CheckMachineType_label_40:
	lw          t0, 520(s1)
	ld          t1, 40(s2)
	ld          t1, 0(t1)
	lhu         t1, 18(t1)
	beq         t0, t1, .CheckMachineType_label_62

	// *** Basic block 3

	lla         a1, .str.23
	mv          a0, s2
	call        LinkerError

	// *** Basic block 4

	mv          a0, x0

	// *** Basic block 5

.CheckMachineType_label_59:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.CheckMachineType_label_62:

	// *** Basic block 7

.CheckMachineType_label_63:
	mv          a0, s3
	j           .CheckMachineType_label_59
.func_end_CheckMachineType:
	.size CheckMachineType, .func_end_CheckMachineType-CheckMachineType

	.global LinkerReadObjectFile
	.type LinkerReadObjectFile, @function

LinkerReadObjectFile:

	// *** Basic block 0

	.global NewELFReaderFile
	.global NewObjectFile
	.global VectorAppend
	.global ELFReaderFileRead
	.local CheckMachineType
	.local ReadELFContents
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
	mv          s1, a1
	mv          s2, a0
	mv          a0, s1
	call        NewELFReaderFile

	// *** Basic block 1

	mv          s3, a0
	ld          a2, 16(s1)
	mv          a1, s2
	mv          a0, s3
	call        NewObjectFile

	// *** Basic block 2

	mv          s4, a0
	addi        a0, s2, 40
	mv          a1, s4
	call        VectorAppend

	// *** Basic block 3

	mv          a2, x0
	mv          a1, x0
	mv          a0, s3
	call        ELFReaderFileRead

	// *** Basic block 4

	mv          s5, a0
	not         t0, s5
	beqz        t0, .LinkerReadObjectFile_label_57

	// *** Basic block 5

	mv          a0, x0

	// *** Basic block 6

.LinkerReadObjectFile_label_54:
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

	// *** Basic block 7

.LinkerReadObjectFile_label_57:
	mv          a1, s4
	mv          a0, s2
	call        CheckMachineType

	// *** Basic block 8

	not         t0, a0
	beqz        t0, .LinkerReadObjectFile_label_68

	// *** Basic block 9

	mv          a0, x0
	j           .LinkerReadObjectFile_label_54

	// *** Basic block 10

.LinkerReadObjectFile_label_68:
	mv          a2, s4
	mv          a1, s3
	mv          a0, s2
	call        ReadELFContents

	// *** Basic block 11

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .LinkerReadObjectFile_label_54
.func_end_LinkerReadObjectFile:
	.size LinkerReadObjectFile, .func_end_LinkerReadObjectFile-LinkerReadObjectFile

	.global LinkerReadObjectFileFromArchive
	.type LinkerReadObjectFileFromArchive, @function

LinkerReadObjectFileFromArchive:

	// *** Basic block 0

	.global NewELFReaderFile
	.global NewObjectFile
	.global VectorAppend
	.global ELFReaderFileRead
	.global ELFReaderFileDelete
	.local CheckMachineType
	.local ReadELFContents
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
	mv          s2, a2
	mv          a0, a1
	call        NewELFReaderFile

	// *** Basic block 1

	mv          s3, a0
	ld          a2, 16(s2)
	mv          a1, s1
	mv          a0, s3
	call        NewObjectFile

	// *** Basic block 2

	mv          s4, a0
	addi        a0, s1, 40
	mv          a1, s4
	call        VectorAppend

	// *** Basic block 3

	ld          a1, 48(s2)
	ld          a2, 40(s2)
	mv          a0, s3
	call        ELFReaderFileRead

	// *** Basic block 4

	mv          s5, a0
	not         t0, s5
	beqz        t0, .LinkerReadObjectFileFromArchive_label_65

	// *** Basic block 5

	mv          a0, s3
	call        ELFReaderFileDelete

	// *** Basic block 6

	mv          a0, x0

	// *** Basic block 7

.LinkerReadObjectFileFromArchive_label_62:
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

	// *** Basic block 8

.LinkerReadObjectFileFromArchive_label_65:
	mv          a1, s4
	mv          a0, s1
	call        CheckMachineType

	// *** Basic block 9

	not         t0, a0
	beqz        t0, .LinkerReadObjectFileFromArchive_label_79

	// *** Basic block 10

	mv          a0, s3
	call        ELFReaderFileDelete

	// *** Basic block 11

	mv          a0, x0
	j           .LinkerReadObjectFileFromArchive_label_62

	// *** Basic block 12

.LinkerReadObjectFileFromArchive_label_79:
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	call        ReadELFContents

	// *** Basic block 13

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .LinkerReadObjectFileFromArchive_label_62
.func_end_LinkerReadObjectFileFromArchive:
	.size LinkerReadObjectFileFromArchive, .func_end_LinkerReadObjectFileFromArchive-LinkerReadObjectFileFromArchive

	.local  ResolveUndefined
	.type ResolveUndefined, @function

ResolveUndefined:

	// *** Basic block 0

	.global LinkerFindSymbolInStaticLibraries
	.global LinkerReadObjectFileFromArchive
	.global DynamicLoaderFindSymbol
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -48(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	sd s6, 0(sp)
	// End of stack frame
	mv          t0, a0
	mv          s1, a1
	mv          s2, x0
	ld          s3, 8(t0)
	bge         x0, s3, .ResolveUndefined_label_96

	// *** Basic block 1

	ld          t1, 0(t0)

	// *** Basic block 2

.ResolveUndefined_label_32:
	slli        t0, s2, 3
	add         t0, t1, t0
	ld          s4, 0(t0)
	lb          t0, 48(s4)
	not         t0, t0
	beqz        t0, .ResolveUndefined_label_91

	// *** Basic block 3

	addi        t0, s4, 8
	ld          s5, 16(t0)
	addi        a2, s0, -48
	addi        a3, s0, -40
	mv          a1, s5
	mv          a0, s1
	call        LinkerFindSymbolInStaticLibraries

	// *** Basic block 4

	mv          s6, a0
	beqz        s6, .ResolveUndefined_label_70

	// *** Basic block 5

	ld          a1, -48(s0)
	ld          a2, -40(s0)
	mv          a0, s1
	call        LinkerReadObjectFileFromArchive

	// *** Basic block 6

	j           .ResolveUndefined_label_92

	// *** Basic block 7

.ResolveUndefined_label_70:
	ld          t0, 536(s1)
	addi        a0, t0, 320
	addi        a2, s0, -32
	addi        a3, s0, -24
	mv          a1, s5
	call        DynamicLoaderFindSymbol

	// *** Basic block 8

	mv          s5, a0
	beqz        s5, .ResolveUndefined_label_90

	// *** Basic block 9

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 48(s4)

	// *** Basic block 10

.ResolveUndefined_label_90:

	// *** Basic block 11

.ResolveUndefined_label_91:

	// *** Basic block 12

.ResolveUndefined_label_92:
	addi        s2, s2, 1
	bge         s2, s3, .ResolveUndefined_label_32

	// *** Basic block 13

.ResolveUndefined_label_96:
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
.func_end_ResolveUndefined:
	.size ResolveUndefined, .func_end_ResolveUndefined-ResolveUndefined

	.local  ResolveUndefinedSymbols
	.type ResolveUndefinedSymbols, @function

ResolveUndefinedSymbols:

	// *** Basic block 0

	.global HashTableTraverse
	.local ResolveUndefined
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

	// *** Basic block 1

.ResolveUndefinedSymbols_label_9:
	addi        t0, s1, 160
	ld          s2, 56(t0)
	addi        a0, s1, 160
	mv          a2, s1
	la          t0, ResolveUndefined
	mv          a1, t0
	call        HashTableTraverse

	// *** Basic block 2

	ld          s3, 56(a0)

	// *** Basic block 3

.ResolveUndefinedSymbols_label_28:
	bne         s2, s3, .ResolveUndefinedSymbols_label_9

	// *** Basic block 4

.ResolveUndefinedSymbols_label_32:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ResolveUndefinedSymbols:
	.size ResolveUndefinedSymbols, .func_end_ResolveUndefinedSymbols-ResolveUndefinedSymbols

	.local  BuildSectionGroup
	.type BuildSectionGroup, @function

BuildSectionGroup:

	// *** Basic block 0

	.global NewSectionGroup
	.global VectorAppend
	.global NewExistingGroupedSection
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
	ld          t2, 0(t0)
	ld          s1, 8(t0)
	ld          s2, 0(t1)
	lw          t3, 8(t1)
	ld          s3, 0(s1)
	ld          t4, 0(s3)
	ld          t4, 0(t4)
	ld          t5, 8(t4)
	ld          t6, 48(t4)
	mv          a3, t6
	mv          a2, t5
	mv          a1, t3
	mv          a0, t2
	call        NewSectionGroup

	// *** Basic block 1

	mv          s4, a0
	addi        a0, s2, 248
	mv          a1, s4
	call        VectorAppend

	// *** Basic block 2

	mv          s2, x0
	ld          s1, 8(s1)
	bge         x0, s1, .BuildSectionGroup_label_83

	// *** Basic block 3

.BuildSectionGroup_label_63:
	slli        t0, s2, 3
	add         t0, s3, t0
	ld          s3, 0(t0)
	mv          a0, s3
	call        NewExistingGroupedSection

	// *** Basic block 4

	mv          s3, a0
	addi        a0, s4, 40
	mv          a1, s3
	call        VectorAppend

	// *** Basic block 5

.BuildSectionGroup_label_79:
	addi        s2, s2, 1
	bge         s2, s1, .BuildSectionGroup_label_63

	// *** Basic block 6

.BuildSectionGroup_label_83:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BuildSectionGroup:
	.size BuildSectionGroup, .func_end_BuildSectionGroup-BuildSectionGroup

	.local  PrintSectionMapKV
	.type PrintSectionMapKV, @function

PrintSectionMapKV:

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
	mv          t0, a0
	ld          t1, 0(t0)
	ld          s1, 8(t0)
	lla         a0, .str.24
	ld          a1, 16(t1)
	call        printf

	// *** Basic block 1

	mv          s2, x0
	ld          s3, 8(s1)
	bge         x0, s3, .PrintSectionMapKV_label_65

	// *** Basic block 2

	ld          s1, 0(s1)

	// *** Basic block 3

.PrintSectionMapKV_label_36:
	slli        t0, s2, 3
	add         t0, s1, t0
	ld          s1, 0(t0)
	lla         a0, .str.25
	addi        t0, s1, 8
	ld          a2, 16(t0)
	ld          a3, 48(s1)
	ld          a4, 56(s1)
	mv          a1, s2
	call        printf

	// *** Basic block 4

.PrintSectionMapKV_label_61:
	addi        s2, s2, 1
	bge         s2, s3, .PrintSectionMapKV_label_36

	// *** Basic block 5

.PrintSectionMapKV_label_65:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PrintSectionMapKV:
	.size PrintSectionMapKV, .func_end_PrintSectionMapKV-PrintSectionMapKV

	.local  PrintSectionMap
	.type PrintSectionMap, @function

PrintSectionMap:

	// *** Basic block 0

	.global MapTraverse
	.local PrintSectionMapKV
	// Leaf procedure, no stack frame generated
	mv          a2, x0
	la          a1, PrintSectionMapKV
	j           MapTraverse
.func_end_PrintSectionMap:
	.size PrintSectionMap, .func_end_PrintSectionMap-PrintSectionMap

	.local  GroupSections
	.type GroupSections, @function

GroupSections:

	// *** Basic block 0

	.global MapInitForStringKeys
	.global MapFindInt64Key
	.global MapFindPointerKey
	.global NewVector
	.global MapInsert
	.global VectorAppend
	.local PrintSectionMap
	.global MapTraverse
	.local BuildSectionGroup
	.global MapDestruct
	addi sp, sp, -160
	// Saved return address (offset 152) and frame pointer (offset 144)
	sd ra, 152(sp)
	sd s0, 144(sp)
	addi s0, sp, 160
	// Local vars at offset -80(s0)
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
	mv          s2, a1
	mv          s3, a2
	addi        a0, s0, -80
	call        MapInitForStringKeys

	// *** Basic block 1

	snez        t0, s3
	mv          s4, x0
	addi        t1, s1, 40
	ld          s5, 8(t1)
	bge         x0, s5, .GroupSections_label_129

	// *** Basic block 2

	ld          s6, 40(s1)

	// *** Basic block 3

.GroupSections_label_43:
	slli        t0, s4, 3
	add         t0, s6, t0
	ld          s6, 0(t0)
	addi        a0, s6, 200
	mv          a1, s2
	call        MapFindInt64Key

	// *** Basic block 4

	mv          s6, a0
	beq         s6, x0, .GroupSections_label_125

	// *** Basic block 5

	ld          t0, 0(s6)

	// *** Basic block 6

.GroupSections_label_60:
	mv          s7, x0
	ld          s8, 8(s6)
	bge         x0, s8, .GroupSections_label_124

	// *** Basic block 7

.GroupSections_label_67:
	slli        t1, s7, 3
	add         t0, t0, t1
	ld          s6, 0(t0)
	beqz        s3, .GroupSections_label_79

	// *** Basic block 8

	ld          t1, 0(s6)
	ld          t1, 8(t1)
	and         t1, t1, s3
	seqz        t0, t1

	// *** Basic block 9

.GroupSections_label_79:
	bnez        t0, .GroupSections_label_120

	// *** Basic block 10

.GroupSections_label_81:
	addi        a0, s0, -80
	addi        a1, s6, 8
	call        MapFindPointerKey

	// *** Basic block 11

	mv          s9, a0
	bne         s9, x0, .GroupSections_label_114

	// *** Basic block 12

	call        NewVector

	// *** Basic block 13

	mv          s9, a0
	addi        t0, s6, 8
	sd          t0, -48(s0)
	addi        t0, s0, -48
	sd          s9, 8(t0)
	addi        a0, s0, -80
	addi        sp, sp, -16
	ld          t0, -48(s0)
	sd          t0, 0(sp)
	ld          t0, -40(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	call        MapInsert

	// *** Basic block 15

.GroupSections_label_114:
	mv          a1, s6
	mv          a0, s9
	call        VectorAppend

	// *** Basic block 16

.GroupSections_label_120:
	addi        s7, s7, 1
	bge         s7, s8, .GroupSections_label_67

	// *** Basic block 17

.GroupSections_label_124:

	// *** Basic block 18

.GroupSections_label_125:
	addi        s4, s4, 1
	bge         s4, s5, .GroupSections_label_43

	// *** Basic block 19

.GroupSections_label_129:
	lb          t0, 586(s1)
	beqz        t0, .GroupSections_label_136

	// *** Basic block 20

	addi        a0, s0, -80
	call        PrintSectionMap

	// *** Basic block 21

.GroupSections_label_136:
	sd          x0, -32(s0)
	sd          x0, -24(s0)
	sd          s1, -32(s0)
	addi        t0, s0, -32
	sw          s2, 8(t0)
	addi        a0, s0, -80
	addi        a2, s0, -32
	la          t0, BuildSectionGroup
	mv          a1, t0
	call        MapTraverse

	// *** Basic block 22

	addi        a0, s0, -80
	call        MapDestruct

	// *** Basic block 23

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
.func_end_GroupSections:
	.size GroupSections, .func_end_GroupSections-GroupSections

	.local  AssignSectionGroupsToSegments
	.type AssignSectionGroupsToSegments, @function

AssignSectionGroupsToSegments:

	// *** Basic block 0

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
	mv          s2, x0
	addi        t0, s1, 248
	ld          t0, 8(t0)
	bge         x0, t0, .AssignSectionGroupsToSegments_label_64

	// *** Basic block 1

.AssignSectionGroupsToSegments_label_25:
	ld          t0, 248(s1)
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s3, 0(t0)
	addi        s4, s1, 272
	ld          s5, 80(s3)
	andi        t0, s5, 1024
	beqz        t0, .AssignSectionGroupsToSegments_label_42

	// *** Basic block 2

	addi        s4, s1, 400
	j           .AssignSectionGroupsToSegments_label_48

	// *** Basic block 3

.AssignSectionGroupsToSegments_label_42:
	andi        t0, s5, 1
	beqz        t0, .AssignSectionGroupsToSegments_label_47

	// *** Basic block 4

	addi        s4, s1, 304

	// *** Basic block 5

.AssignSectionGroupsToSegments_label_47:

	// *** Basic block 6

.AssignSectionGroupsToSegments_label_48:
	sd          s4, 64(s3)
	mv          a1, s3
	mv          a0, s4
	call        VectorAppend

	// *** Basic block 7

.AssignSectionGroupsToSegments_label_57:
	addi        s2, s2, 1
	addi        t0, s1, 248
	ld          t0, 8(t0)
	bge         s2, t0, .AssignSectionGroupsToSegments_label_25

	// *** Basic block 8

.AssignSectionGroupsToSegments_label_64:
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
.func_end_AssignSectionGroupsToSegments:
	.size AssignSectionGroupsToSegments, .func_end_AssignSectionGroupsToSegments-AssignSectionGroupsToSegments

	.local  AssignSegmentSectionAddresses
	.type AssignSegmentSectionAddresses, @function

AssignSegmentSectionAddresses:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, x0
	mv          t3, x0
	ld          t4, 8(t0)
	bge         x0, t4, .AssignSegmentSectionAddresses_label_90

	// *** Basic block 1

	ld          t5, 0(t0)

	// *** Basic block 2

.AssignSegmentSectionAddresses_label_31:
	slli        t0, t3, 3
	add         t0, t5, t0
	ld          t5, 0(t0)
	sd          t1, 96(t5)
	mv          t0, x0
	addi        t6, t5, 40
	ld          t6, 8(t6)
	bge         x0, t6, .AssignSegmentSectionAddresses_label_85

	// *** Basic block 3

	ld          t5, 40(t5)

	// *** Basic block 4

.AssignSegmentSectionAddresses_label_47:
	slli        a0, t0, 3
	add         t5, t5, a0
	ld          a0, 0(t5)
	lw          t5, 0(a0)
	bnez        t5, .AssignSegmentSectionAddresses_label_68

	// *** Basic block 5

	ld          t5, 8(a0)
	sd          t1, 56(t5)
	sd          t2, 64(t5)
	ld          t5, 0(t5)
	ld          t5, 32(t5)
	add         t1, t1, t5
	j           .AssignSegmentSectionAddresses_label_80

	// *** Basic block 6

.AssignSegmentSectionAddresses_label_68:
	ld          t5, 8(a0)
	sd          t1, 128(t5)
	ld          t5, 104(t5)
	addi        t5, t5, 8
	ld          t5, 8(t5)
	add         t1, t1, t5

	// *** Basic block 7

.AssignSegmentSectionAddresses_label_80:

	// *** Basic block 8

.AssignSegmentSectionAddresses_label_81:
	addi        t0, t0, 1
	bge         t0, t6, .AssignSegmentSectionAddresses_label_47

	// *** Basic block 9

.AssignSegmentSectionAddresses_label_85:

	// *** Basic block 10

.AssignSegmentSectionAddresses_label_86:
	addi        t3, t3, 1
	bge         t3, t4, .AssignSegmentSectionAddresses_label_31

	// *** Basic block 11

.AssignSegmentSectionAddresses_label_90:
	addi        t4, t1, 7
	andi        t4, t4, -8
	mv          a0, t4

	// *** Basic block 12

.AssignSegmentSectionAddresses_label_95:
	ret         
.func_end_AssignSegmentSectionAddresses:
	.size AssignSegmentSectionAddresses, .func_end_AssignSegmentSectionAddresses-AssignSegmentSectionAddresses

	.global LinkerLinkAllFiles
	.type LinkerLinkAllFiles, @function

LinkerLinkAllFiles:

	// *** Basic block 0

	.global SegmentInit
	.local ResolveUndefinedSymbols
	.local GroupSections
	.local AssignSectionGroupsToSegments
	.global DynamicLinkerCreateDynamicLinkerGroups
	.local AssignSegmentSectionAddresses
	.global LinkerAssignSymbolAddresses
	.global LinkerAssignSectionSymbolAddresses
	.global LinkerAssignCommonSymbolAddresses
	.global LinkerAssignBSSSymbolAddresses
	.global DynamicLinkerDefineSymbols
	.global DynamicLinkerFixupGOT
	.global DynamicLinkerFixupPLT
	.global LinkerPrintSymbolTables
	.global LinkerApplyAllRelocations
	.global LinkerCheckForUndefinedSymbols
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Local vars at offset -24(s0)
	// Saved integer registers.
	sd s1, 32(sp)
	sd s2, 24(sp)
	sd s3, 16(sp)
	sd s4, 8(sp)
	// End of stack frame
	mv          s1, a0
	addi        a0, s1, 272
	call        SegmentInit

	// *** Basic block 1

	addi        a0, s1, 304
	call        SegmentInit

	// *** Basic block 2

	addi        a0, s1, 400
	call        SegmentInit

	// *** Basic block 3

	lb          t0, 529(s1)
	not         t0, t0
	beqz        t0, .LinkerLinkAllFiles_label_55

	// *** Basic block 4

	addi        a0, s1, 336
	call        SegmentInit

	// *** Basic block 5

.LinkerLinkAllFiles_label_55:
	lb          t1, 529(s1)
	not         t0, t1
	beqz        t0, .LinkerLinkAllFiles_label_64

	// *** Basic block 6

	lb          t1, 528(s1)
	not         t0, t1

	// *** Basic block 7

.LinkerLinkAllFiles_label_64:
	beqz        t0, .LinkerLinkAllFiles_label_69

	// *** Basic block 8

	addi        a0, s1, 368
	call        SegmentInit

	// *** Basic block 9

.LinkerLinkAllFiles_label_69:
	mv          a0, s1
	call        ResolveUndefinedSymbols

	// *** Basic block 10

	mv          a2, x0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        GroupSections

	// *** Basic block 11

	li          t0, 1024		// 0x400
	mv          a2, t0
	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	mv          a0, s1
	call        GroupSections

	// *** Basic block 12

	mv          a0, s1
	call        AssignSectionGroupsToSegments

	// *** Basic block 13

	lb          t0, 529(s1)
	not         t0, t0
	beqz        t0, .LinkerLinkAllFiles_label_102

	// *** Basic block 14

	mv          a0, s1
	call        DynamicLinkerCreateDynamicLinkerGroups

	// *** Basic block 15

.LinkerLinkAllFiles_label_102:
	ld          t0, 88(s1)
	ld          t0, 8(t0)
	mv          a0, s1
	jalr         x1, t0, 0

	// *** Basic block 16

	sd          a0, -24(s0)
	addi        a0, s1, 272
	ld          s2, -24(s0)
	mv          a1, s2
	call        AssignSegmentSectionAddresses

	// *** Basic block 17

	mv          s3, a0
	sub         s4, s3, s2
	ld          t0, 88(s1)
	ld          t0, 16(t0)
	mv          a2, s4
	mv          a1, s2
	mv          a0, s1
	jalr         x1, t0, 0

	// *** Basic block 18

	sd          a0, -24(s0)
	lb          t0, 529(s1)
	not         t0, t0
	beqz        t0, .LinkerLinkAllFiles_label_145

	// *** Basic block 19

	addi        a0, s1, 336
	ld          a1, -24(s0)
	call        AssignSegmentSectionAddresses

	// *** Basic block 20

	sd          a0, -24(s0)

	// *** Basic block 21

.LinkerLinkAllFiles_label_145:
	lb          t1, 529(s1)
	not         t0, t1
	beqz        t0, .LinkerLinkAllFiles_label_154

	// *** Basic block 22

	lb          t1, 528(s1)
	not         t0, t1

	// *** Basic block 23

.LinkerLinkAllFiles_label_154:
	beqz        t0, .LinkerLinkAllFiles_label_162

	// *** Basic block 24

	addi        a0, s1, 368
	ld          a1, -24(s0)
	call        AssignSegmentSectionAddresses

	// *** Basic block 25

	sd          a0, -24(s0)

	// *** Basic block 26

.LinkerLinkAllFiles_label_162:
	addi        a0, s1, 304
	ld          a1, -24(s0)
	call        AssignSegmentSectionAddresses

	// *** Basic block 27

	sd          a0, -24(s0)
	addi        a0, s1, 400
	mv          a1, x0
	call        AssignSegmentSectionAddresses

	// *** Basic block 28

	mv          a0, s1
	call        LinkerAssignSymbolAddresses

	// *** Basic block 29

	mv          a0, s1
	call        LinkerAssignSectionSymbolAddresses

	// *** Basic block 30

	ld          t0, -24(s0)
	sd          t0, 432(s1)
	addi        a1, s0, -24
	mv          a0, s1
	call        LinkerAssignCommonSymbolAddresses

	// *** Basic block 31

	ld          t0, -24(s0)
	ld          t1, 432(s1)
	sub         t0, t0, t1
	sd          t0, 440(s1)
	mv          a0, s1
	call        LinkerAssignBSSSymbolAddresses

	// *** Basic block 32

	lb          t0, 529(s1)
	not         t0, t0
	beqz        t0, .LinkerLinkAllFiles_label_209

	// *** Basic block 33

	mv          a0, s1
	call        DynamicLinkerDefineSymbols

	// *** Basic block 34

	mv          a0, s1
	call        DynamicLinkerFixupGOT

	// *** Basic block 35

	mv          a0, s1
	call        DynamicLinkerFixupPLT

	// *** Basic block 36

.LinkerLinkAllFiles_label_209:
	lb          t0, 584(s1)
	beqz        t0, .LinkerLinkAllFiles_label_216

	// *** Basic block 37

	mv          a0, s1
	call        LinkerPrintSymbolTables

	// *** Basic block 38

.LinkerLinkAllFiles_label_216:
	mv          a0, s1
	call        LinkerApplyAllRelocations

	// *** Basic block 39

	lb          t0, 528(s1)
	not         t0, t0
	beqz        t0, .LinkerLinkAllFiles_label_227

	// *** Basic block 40

	mv          a0, s1
	call        LinkerCheckForUndefinedSymbols

	// *** Basic block 41

.LinkerLinkAllFiles_label_227:
	// Restored registers.
	ld s1, 32(sp)
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LinkerLinkAllFiles:
	.size LinkerLinkAllFiles, .func_end_LinkerLinkAllFiles-LinkerLinkAllFiles

	.local  BuildOutputSection
	.type BuildOutputSection, @function

BuildOutputSection:

	// *** Basic block 0

	.global NewELFWriterSectionContents
	.global ELFWriterAddSection
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
	sd s8, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	li          a0, 2		// 0x2 ASCII \x2
	call        NewELFWriterSectionContents

	// *** Basic block 1

	mv          s3, a0
	lw          a2, 72(s2)
	ld          a3, 80(s2)
	ld          a4, 88(s2)
	ld          a6, 96(s2)
	mv          a5, s3
	mv          a1, s2
	mv          a0, s1
	call        ELFWriterAddSection

	// *** Basic block 2

	mv          s4, a0
	mv          s5, x0
	addi        t0, s2, 40
	ld          s6, 8(t0)
	bge         x0, s6, .BuildOutputSection_label_148

	// *** Basic block 3

	ld          t0, 40(s2)

	// *** Basic block 4

.BuildOutputSection_label_69:
	slli        t1, s5, 3
	add         t0, t0, t1
	ld          s1, 0(t0)
	lw          t0, 0(s1)
	bnez        t0, .BuildOutputSection_label_115

	// *** Basic block 5

	ld          s2, 8(s1)
	ld          s7, 0(s2)
	lwu         t0, 4(s7)
	li          s8, 1		// 0x1 ASCII \x1
	bne         t0, s8, .BuildOutputSection_label_103

	// *** Basic block 6

	mv          a0, s8
	call        NewELFWriterSectionContents

	// *** Basic block 7

	mv          s8, a0
	ld          t0, 32(s7)
	sd          t0, 32(s8)
	ld          t0, 48(s2)
	sd          t0, 8(s8)
	j           .BuildOutputSection_label_113

	// *** Basic block 8

.BuildOutputSection_label_103:
	li          t0, 3		// 0x3 ASCII \x3
	mv          a0, t0
	call        NewELFWriterSectionContents

	// *** Basic block 9

	mv          s8, a0
	ld          t0, 32(s7)
	sd          t0, 32(s8)

	// *** Basic block 10

.BuildOutputSection_label_113:
	j           .BuildOutputSection_label_138

	// *** Basic block 11

.BuildOutputSection_label_115:
	ld          t0, 8(s1)
	ld          s8, 104(t0)
	addi        t1, t0, 40
	ld          s1, 56(t1)
	beqz        s1, .BuildOutputSection_label_129

	// *** Basic block 12

	addi        t1, s4, 40
	sd          s1, 56(t1)

	// *** Basic block 13

.BuildOutputSection_label_129:
	ld          t0, 144(t0)
	beq         t0, x0, .BuildOutputSection_label_137

	// *** Basic block 14

	sd          t0, 144(s4)

	// *** Basic block 15

.BuildOutputSection_label_137:

	// *** Basic block 16

.BuildOutputSection_label_138:
	addi        a0, s3, 8
	mv          a1, s8
	call        VectorAppend

	// *** Basic block 17

.BuildOutputSection_label_144:
	addi        s5, s5, 1
	bge         s5, s6, .BuildOutputSection_label_69

	// *** Basic block 18

.BuildOutputSection_label_148:
	mv          a0, s4

	// *** Basic block 19

.BuildOutputSection_label_151:
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
.func_end_BuildOutputSection:
	.size BuildOutputSection, .func_end_BuildOutputSection-BuildOutputSection

	.local  FindDynamicSymbolTableBuffer
	.type FindDynamicSymbolTableBuffer, @function

FindDynamicSymbolTableBuffer:

	// *** Basic block 0

	.global ELFWriterFindSection
	.global printf
	.global abort
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
	lla         a1, .str.26
	call        ELFWriterFindSection

	// *** Basic block 1

	mv          s1, a0
	beq         s1, x0, .FindDynamicSymbolTableBuffer_label_30

	// *** Basic block 2

	j           .FindDynamicSymbolTableBuffer_label_47

	// *** Basic block 3

.FindDynamicSymbolTableBuffer_label_30:
	lla         a0, .str.27
	lla         a1, .str.28
	lla         a3, .str.29
	li          t0, 831		// 0x33f
	mv          a2, t0
	call        printf

	// *** Basic block 4

	call        abort

	// *** Basic block 5

.FindDynamicSymbolTableBuffer_label_47:
	ld          t0, 104(s1)
	ld          t0, 8(t0)
	ld          s2, 0(t0)
	addi        t0, s2, 8
	mv          a0, t0

	// *** Basic block 6

.FindDynamicSymbolTableBuffer_label_57:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_FindDynamicSymbolTableBuffer:
	.size FindDynamicSymbolTableBuffer, .func_end_FindDynamicSymbolTableBuffer-FindDynamicSymbolTableBuffer

	.local  AssignGroupSectionIndexes
	.type AssignGroupSectionIndexes, @function

AssignGroupSectionIndexes:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, x0
	addi        t2, a0, 40
	ld          t2, 8(t2)
	bge         x0, t2, .AssignGroupSectionIndexes_label_52

	// *** Basic block 1

	ld          t3, 40(a0)
	lw          t4, 0(t0)
	lw          t5, 0(t0)

	// *** Basic block 2

.AssignGroupSectionIndexes_label_27:
	slli        t6, t1, 3
	add         t3, t3, t6
	ld          t6, 0(t3)
	lw          t3, 0(t6)
	bnez        t3, .AssignGroupSectionIndexes_label_42

	// *** Basic block 3

	ld          t3, 8(t6)
	sw          t4, 72(t3)
	j           .AssignGroupSectionIndexes_label_47

	// *** Basic block 4

.AssignGroupSectionIndexes_label_42:
	ld          t3, 8(t6)
	sw          t5, 112(t3)

	// *** Basic block 5

.AssignGroupSectionIndexes_label_47:

	// *** Basic block 6

.AssignGroupSectionIndexes_label_48:
	addi        t1, t1, 1
	bge         t1, t2, .AssignGroupSectionIndexes_label_27

	// *** Basic block 7

.AssignGroupSectionIndexes_label_52:
	lw          t2, 0(t0)
	addi        t2, t2, 1
	sw          t2, 0(t0)
	ret         
.func_end_AssignGroupSectionIndexes:
	.size AssignGroupSectionIndexes, .func_end_AssignGroupSectionIndexes-AssignGroupSectionIndexes

	.local  AddSymbolListToOutput
	.type AddSymbolListToOutput, @function

AddSymbolListToOutput:

	// *** Basic block 0

	.global ELFWriterAddSymbol
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
	mv          t0, a0
	mv          s1, a1
	mv          s2, x0
	ld          s3, 8(t0)
	bge         x0, s3, .AddSymbolListToOutput_label_94

	// *** Basic block 1

	ld          t1, 0(t0)
	addi        t2, s1, 64
	ld          t2, 8(t2)
	addi        s4, t2, -1

	// *** Basic block 2

.AddSymbolListToOutput_label_40:
	slli        t0, s2, 3
	add         t0, t1, t0
	ld          s5, 0(t0)
	ld          t0, 0(s5)
	lbu         t0, 4(t0)
	andi        s6, t0, 15
	srli        s7, t0, 4
	ld          s8, 56(s5)
	bne         s8, x0, .AddSymbolListToOutput_label_60

	// *** Basic block 3

	j           .AddSymbolListToOutput_label_63

	// *** Basic block 4

.AddSymbolListToOutput_label_60:
	lw          s4, 72(s8)

	// *** Basic block 5

.AddSymbolListToOutput_label_63:
	addi        a1, s5, 8
	ld          t0, 0(s5)
	ld          a5, 16(t0)
	ld          a6, 80(s5)
	addi        a7, s5, 96
	mv          a4, s7
	mv          a3, s6
	mv          a2, s4
	mv          a0, s1
	call        ELFWriterAddSymbol

	// *** Basic block 6

.AddSymbolListToOutput_label_90:
	addi        s2, s2, 1
	bge         s2, s3, .AddSymbolListToOutput_label_40

	// *** Basic block 7

.AddSymbolListToOutput_label_94:
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
.func_end_AddSymbolListToOutput:
	.size AddSymbolListToOutput, .func_end_AddSymbolListToOutput-AddSymbolListToOutput

	.local  BuildSections
	.type BuildSections, @function

BuildSections:

	// *** Basic block 0

	.global NewELFWriterSectionContents
	.global ELFWriterAddSection
	.local BuildOutputSection
	.global StringInit
	.global StringDestruct
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -56(s0)
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
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	li          a0, 1		// 0x1 ASCII \x1
	call        NewELFWriterSectionContents

	// *** Basic block 1

	mv          s3, a0
	mv          a6, x0
	mv          a5, s3
	li          s4, 8		// 0x8 ASCII \x8
	mv          a4, s4
	mv          a3, x0
	mv          a2, x0
	mv          a1, x0
	mv          a0, s1
	call        ELFWriterAddSection

	// *** Basic block 2

	mv          s5, x0
	addi        t0, s2, 272
	ld          s6, 8(t0)
	bge         x0, s6, .BuildSections_label_79

	// *** Basic block 3

	ld          s3, 272(s2)

	// *** Basic block 4

.BuildSections_label_67:
	slli        t0, s5, 3
	add         t0, s3, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        BuildOutputSection

	// *** Basic block 5

.BuildSections_label_75:
	addi        s5, s5, 1
	bge         s5, s6, .BuildSections_label_67

	// *** Basic block 6

.BuildSections_label_79:
	lb          t0, 529(s2)
	not         t1, t0
	beqz        t1, .BuildSections_label_105

	// *** Basic block 7

	mv          s3, x0
	addi        t0, s2, 336
	ld          s6, 8(t0)
	bge         x0, s6, .BuildSections_label_104

	// *** Basic block 8

	ld          s7, 336(s2)

	// *** Basic block 9

.BuildSections_label_92:
	slli        t0, s3, 3
	add         t0, s7, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        BuildOutputSection

	// *** Basic block 10

.BuildSections_label_100:
	addi        s3, s3, 1
	bge         s3, s6, .BuildSections_label_92

	// *** Basic block 11

.BuildSections_label_104:

	// *** Basic block 12

.BuildSections_label_105:
	beqz        t1, .BuildSections_label_111

	// *** Basic block 13

	lb          t0, 528(s2)
	not         t1, t0

	// *** Basic block 14

.BuildSections_label_111:
	beqz        t1, .BuildSections_label_134

	// *** Basic block 15

	mv          s6, x0
	addi        t0, s2, 368
	ld          s7, 8(t0)
	bge         x0, s7, .BuildSections_label_133

	// *** Basic block 16

	ld          s8, 368(s2)

	// *** Basic block 17

.BuildSections_label_121:
	slli        t0, s6, 3
	add         t0, s8, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        BuildOutputSection

	// *** Basic block 18

.BuildSections_label_129:
	addi        s6, s6, 1
	bge         s6, s7, .BuildSections_label_121

	// *** Basic block 19

.BuildSections_label_133:

	// *** Basic block 20

.BuildSections_label_134:
	mv          s7, x0
	addi        t0, s2, 304
	ld          s8, 8(t0)
	bge         x0, s8, .BuildSections_label_155

	// *** Basic block 21

	ld          s9, 304(s2)

	// *** Basic block 22

.BuildSections_label_143:
	slli        t0, s7, 3
	add         t0, s9, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        BuildOutputSection

	// *** Basic block 23

.BuildSections_label_151:
	addi        s7, s7, 1
	bge         s7, s8, .BuildSections_label_143

	// *** Basic block 24

.BuildSections_label_155:
	mv          s8, x0
	addi        t0, s2, 400
	ld          s9, 8(t0)
	bge         x0, s9, .BuildSections_label_176

	// *** Basic block 25

	ld          s10, 400(s2)

	// *** Basic block 26

.BuildSections_label_164:
	slli        t0, s8, 3
	add         t0, s10, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        BuildOutputSection

	// *** Basic block 27

.BuildSections_label_172:
	addi        s8, s8, 1
	bge         s8, s9, .BuildSections_label_164

	// *** Basic block 28

.BuildSections_label_176:
	li          s9, 3		// 0x3 ASCII \x3
	mv          a0, s9
	call        NewELFWriterSectionContents

	// *** Basic block 29

	mv          s10, a0
	ld          t0, 440(s2)
	sd          t0, 32(s10)
	addi        a0, s0, -56
	lla         a1, .str.30
	call        StringInit

	// *** Basic block 30

	addi        a1, s0, -56
	ld          a6, 432(s2)
	mv          a5, s10
	mv          a4, s4
	mv          a3, s9
	mv          a2, s4
	mv          a0, s1
	call        ELFWriterAddSection

	// *** Basic block 31

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 32

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
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BuildSections:
	.size BuildSections, .func_end_BuildSections-BuildSections

	.local  AssignSectionIndexes
	.type AssignSectionIndexes, @function

AssignSectionIndexes:

	// *** Basic block 0

	.local AssignGroupSectionIndexes
	.global DynamicLinkerFixupDynamicSymbolTable
	.local FindDynamicSymbolTableBuffer
	.global ELFWriterAddSectionFixupByName
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -24(s0)
	// Saved integer registers.
	sd s1, 64(sp)
	sd s2, 56(sp)
	sd s3, 48(sp)
	sd s4, 40(sp)
	sd s5, 32(sp)
	sd s6, 24(sp)
	sd s7, 16(sp)
	sd s8, 8(sp)
	sd s9, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	li          s3, 1		// 0x1 ASCII \x1
	sw          s3, -24(s0)
	lb          t1, 529(s1)
	not         t0, t1
	beqz        t0, .AssignSectionIndexes_label_46

	// *** Basic block 1

	lb          t1, 528(s1)
	not         t0, t1

	// *** Basic block 2

.AssignSectionIndexes_label_46:
	beqz        t0, .AssignSectionIndexes_label_70

	// *** Basic block 3

	mv          s4, x0
	addi        t1, s1, 368
	ld          s5, 8(t1)
	bge         x0, s5, .AssignSectionIndexes_label_69

	// *** Basic block 4

	ld          s6, 368(s1)

	// *** Basic block 5

.AssignSectionIndexes_label_56:
	slli        t1, s4, 3
	add         t1, s6, t1
	ld          a0, 0(t1)
	addi        a1, s0, -24
	call        AssignGroupSectionIndexes

	// *** Basic block 6

.AssignSectionIndexes_label_65:
	addi        s4, s4, 1
	bge         s4, s5, .AssignSectionIndexes_label_56

	// *** Basic block 7

.AssignSectionIndexes_label_69:

	// *** Basic block 8

.AssignSectionIndexes_label_70:
	mv          s5, x0
	addi        t1, s1, 272
	ld          s6, 8(t1)
	bge         x0, s6, .AssignSectionIndexes_label_91

	// *** Basic block 9

	ld          s7, 272(s1)

	// *** Basic block 10

.AssignSectionIndexes_label_79:
	slli        t1, s5, 3
	add         t1, s7, t1
	ld          a0, 0(t1)
	addi        a1, s0, -24
	call        AssignGroupSectionIndexes

	// *** Basic block 11

.AssignSectionIndexes_label_87:
	addi        s5, s5, 1
	bge         s5, s6, .AssignSectionIndexes_label_79

	// *** Basic block 12

.AssignSectionIndexes_label_91:
	beqz        t0, .AssignSectionIndexes_label_175

	// *** Basic block 13

	mv          s6, x0
	addi        t0, s1, 336
	ld          s7, 8(t0)
	bge         x0, s7, .AssignSectionIndexes_label_113

	// *** Basic block 14

	ld          s8, 336(s1)

	// *** Basic block 15

.AssignSectionIndexes_label_101:
	slli        t0, s6, 3
	add         t0, s8, t0
	ld          a0, 0(t0)
	addi        a1, s0, -24
	call        AssignGroupSectionIndexes

	// *** Basic block 16

.AssignSectionIndexes_label_109:
	addi        s6, s6, 1
	bge         s6, s7, .AssignSectionIndexes_label_101

	// *** Basic block 17

.AssignSectionIndexes_label_113:
	mv          a0, s2
	call        FindDynamicSymbolTableBuffer

	// *** Basic block 18

	addi        t0, s2, 64
	ld          t0, 8(t0)
	addi        a1, t0, -1
	call        DynamicLinkerFixupDynamicSymbolTable

	// *** Basic block 19

	lla         a2, .str.31
	lla         a3, .str.32
	mv          a1, s3
	mv          a0, s2
	call        ELFWriterAddSectionFixupByName

	// *** Basic block 20

	lla         a2, .str.33
	lla         a3, .str.34
	mv          a1, s3
	mv          a0, s2
	call        ELFWriterAddSectionFixupByName

	// *** Basic block 21

	lla         a2, .str.35
	lla         a3, .str.36
	mv          a1, s3
	mv          a0, s2
	call        ELFWriterAddSectionFixupByName

	// *** Basic block 22

	lla         a2, .str.37
	lla         a3, .str.38
	mv          a1, x0
	mv          a0, s2
	call        ELFWriterAddSectionFixupByName

	// *** Basic block 23

.AssignSectionIndexes_label_175:
	mv          s3, x0
	addi        t0, s1, 304
	ld          s7, 8(t0)
	bge         x0, s7, .AssignSectionIndexes_label_196

	// *** Basic block 24

	ld          s8, 304(s1)

	// *** Basic block 25

.AssignSectionIndexes_label_184:
	slli        t0, s3, 3
	add         t0, s8, t0
	ld          a0, 0(t0)
	addi        a1, s0, -24
	call        AssignGroupSectionIndexes

	// *** Basic block 26

.AssignSectionIndexes_label_192:
	addi        s3, s3, 1
	bge         s3, s7, .AssignSectionIndexes_label_184

	// *** Basic block 27

.AssignSectionIndexes_label_196:
	mv          s7, x0
	addi        t0, s1, 400
	ld          s8, 8(t0)
	bge         x0, s8, .AssignSectionIndexes_label_217

	// *** Basic block 28

	ld          s9, 400(s1)

	// *** Basic block 29

.AssignSectionIndexes_label_205:
	slli        t0, s7, 3
	add         t0, s9, t0
	ld          a0, 0(t0)
	addi        a1, s0, -24
	call        AssignGroupSectionIndexes

	// *** Basic block 30

.AssignSectionIndexes_label_213:
	addi        s7, s7, 1
	bge         s7, s8, .AssignSectionIndexes_label_205

	// *** Basic block 31

.AssignSectionIndexes_label_217:
	// Restored registers.
	ld s1, 64(sp)
	ld s2, 56(sp)
	ld s3, 48(sp)
	ld s4, 40(sp)
	ld s5, 32(sp)
	ld s6, 24(sp)
	ld s7, 16(sp)
	ld s8, 8(sp)
	ld s9, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AssignSectionIndexes:
	.size AssignSectionIndexes, .func_end_AssignSectionIndexes-AssignSectionIndexes

	.local  InsertSegments
	.type InsertSegments, @function

InsertSegments:

	// *** Basic block 0

	.global NewELFWriterSegment
	.global ELFWriterSegmentAddSection
	.global StringEqual
	.global VectorAppend
	.global ELFWriterSegmentDelete
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -16(s0)
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
	lb          s4, 528(s1)
	beqz        s4, .InsertSegments_label_43

	// *** Basic block 1

	li          s3, 4096		// 0x1000
	j           .InsertSegments_label_45

	// *** Basic block 2

.InsertSegments_label_43:
	li          s3, 4096		// 0x1000

	// *** Basic block 3

.InsertSegments_label_45:
	sext.w      s5, s3
	mv          a2, s5
	li          t0, 5		// 0x5 ASCII \x5
	mv          a1, t0
	li          s3, 1		// 0x1 ASCII \x1
	mv          a0, s3
	call        NewELFWriterSegment

	// *** Basic block 4

	mv          s6, a0
	mv          a2, s5
	li          s7, 6		// 0x6 ASCII \x6
	mv          a1, s7
	mv          a0, s3
	call        NewELFWriterSegment

	// *** Basic block 5

	mv          s8, a0
	mv          a2, s5
	li          s5, 4		// 0x4 ASCII \x4
	mv          a1, s5
	li          t0, 7		// 0x7 ASCII \x7
	mv          a0, t0
	call        NewELFWriterSegment

	// *** Basic block 6

	mv          s9, a0
	mv          s10, x0
	mv          s11, x0
	lb          t0, 529(s1)
	not         t1, t0
	beqz        t1, .InsertSegments_label_101

	// *** Basic block 7

	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	mv          a1, s7
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewELFWriterSegment

	// *** Basic block 8

	mv          s10, a0

	// *** Basic block 9

.InsertSegments_label_101:
	beqz        t1, .InsertSegments_label_105

	// *** Basic block 10

	not         t1, s4

	// *** Basic block 11

.InsertSegments_label_105:
	beqz        t1, .InsertSegments_label_118

	// *** Basic block 12

	mv          a2, s3
	mv          a1, s5
	li          t0, 3		// 0x3 ASCII \x3
	mv          a0, t0
	call        NewELFWriterSegment

	// *** Basic block 13

	mv          s11, a0

	// *** Basic block 14

.InsertSegments_label_118:
	li          s1, 1		// 0x1 ASCII \x1
	addi        t0, s2, 64
	ld          s5, 8(t0)
	bge         s3, s5, .InsertSegments_label_199

	// *** Basic block 15

	ld          t0, 64(s2)

	// *** Basic block 16

.InsertSegments_label_129:
	slli        t2, s1, 3
	add         t0, t0, t2
	ld          s3, 0(t0)
	addi        t0, s3, 40
	lwu         t0, 4(t0)
	bne         t0, s7, .InsertSegments_label_148

	// *** Basic block 17

	mv          a1, s3
	mv          a0, s10
	call        ELFWriterSegmentAddSection

	// *** Basic block 18

	j           .InsertSegments_label_194

	// *** Basic block 19

.InsertSegments_label_148:
	lla         a1, .str.39
	mv          a0, s3
	call        StringEqual

	// *** Basic block 20

	beqz        a0, .InsertSegments_label_162

	// *** Basic block 21

	mv          a1, s3
	mv          a0, s11
	call        ELFWriterSegmentAddSection

	// *** Basic block 22

	j           .InsertSegments_label_193

	// *** Basic block 23

.InsertSegments_label_162:
	addi        t0, s3, 40
	ld          s7, 8(t0)
	andi        t0, s7, 1024
	beqz        t0, .InsertSegments_label_175

	// *** Basic block 24

	mv          a1, s3
	mv          a0, s9
	call        ELFWriterSegmentAddSection

	// *** Basic block 25

	j           .InsertSegments_label_192

	// *** Basic block 26

.InsertSegments_label_175:
	andi        t0, s7, 1
	bnez        t0, .InsertSegments_label_185

	// *** Basic block 27

	mv          a1, s3
	mv          a0, s6
	call        ELFWriterSegmentAddSection

	// *** Basic block 28

	j           .InsertSegments_label_191

	// *** Basic block 29

.InsertSegments_label_185:
	mv          a1, s3
	mv          a0, s8
	call        ELFWriterSegmentAddSection

	// *** Basic block 30

.InsertSegments_label_191:

	// *** Basic block 31

.InsertSegments_label_192:

	// *** Basic block 32

.InsertSegments_label_193:

	// *** Basic block 33

.InsertSegments_label_194:

	// *** Basic block 34

.InsertSegments_label_195:
	addi        s1, s1, 1
	bge         s1, s5, .InsertSegments_label_129

	// *** Basic block 35

.InsertSegments_label_199:
	addi        a0, s2, 88
	mv          a1, s6
	call        VectorAppend

	// *** Basic block 36

	addi        a0, s2, 88
	mv          a1, s8
	call        VectorAppend

	// *** Basic block 37

	addi        t0, s9, 56
	ld          t0, 8(t0)
	bge         x0, t0, .InsertSegments_label_221

	// *** Basic block 38

	addi        a0, s2, 88
	mv          a1, s9
	call        VectorAppend

	// *** Basic block 39

	j           .InsertSegments_label_225

	// *** Basic block 40

.InsertSegments_label_221:
	mv          a0, s9
	call        ELFWriterSegmentDelete

	// *** Basic block 41

.InsertSegments_label_225:
	beqz        t1, .InsertSegments_label_232

	// *** Basic block 42

	addi        a0, s2, 88
	mv          a1, s10
	call        VectorAppend

	// *** Basic block 43

.InsertSegments_label_232:
	mv          t0, t1
	beqz        t1, .InsertSegments_label_237

	// *** Basic block 44

	not         t0, s4

	// *** Basic block 45

.InsertSegments_label_237:
	beqz        t0, .InsertSegments_label_245

	// *** Basic block 46

	addi        a0, s2, 88
	mv          a1, s11
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
	j           VectorAppend

	// *** Basic block 47

.InsertSegments_label_245:
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
.func_end_InsertSegments:
	.size InsertSegments, .func_end_InsertSegments-InsertSegments

	.local  SetEntryAddress
	.type SetEntryAddress, @function

SetEntryAddress:

	// *** Basic block 0

	.global ELFWriterFindSection
	.global LinkerFindSymbol
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
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	lb          t0, 528(s1)
	beqz        t0, .SetEntryAddress_label_47

	// *** Basic block 1

	lla         a1, .str.40
	mv          a0, s2
	call        ELFWriterFindSection

	// *** Basic block 2

	mv          s3, a0
	bne         s3, x0, .SetEntryAddress_label_41

	// *** Basic block 3

	mv          a0, x0

	// *** Basic block 4

.SetEntryAddress_label_38:
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

.SetEntryAddress_label_41:
	ld          t0, 128(s3)
	sd          t0, 24(s2)
	j           .SetEntryAddress_label_73

	// *** Basic block 6

.SetEntryAddress_label_47:
	addi        a0, s1, 160
	lla         a1, .str.41
	call        LinkerFindSymbol

	// *** Basic block 7

	mv          s4, a0
	bne         s4, x0, .SetEntryAddress_label_68

	// *** Basic block 8

	lla         a1, .str.42
	mv          a0, x0
	call        LinkerError

	// *** Basic block 9

	mv          a0, x0
	j           .SetEntryAddress_label_38

	// *** Basic block 10

.SetEntryAddress_label_68:
	ld          t0, 80(s4)
	sd          t0, 24(s2)

	// *** Basic block 11

.SetEntryAddress_label_73:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .SetEntryAddress_label_38
.func_end_SetEntryAddress:
	.size SetEntryAddress, .func_end_SetEntryAddress-SetEntryAddress

	.global LinkerWriteOutput
	.type LinkerWriteOutput, @function

LinkerWriteOutput:

	// *** Basic block 0

	.global ELFWriterFileInit
	.global DynamicLinkerFixupDynamicSectionContents
	.local BuildSections
	.local SetEntryAddress
	.global ELFWriterFileDestruct
	.local AssignSectionIndexes
	.global HashTableTraverse
	.local AddSymbolListToOutput
	.global DynamicLinkerBuildDynamicRelocations
	.global DynamicLinkerBuildPLTRelocations
	.local InsertSegments
	.global ELFWriterFileWrite
	addi sp, sp, -352
	// Saved return address (offset 344) and frame pointer (offset 336)
	sd ra, 344(sp)
	sd s0, 336(sp)
	addi s0, sp, 352
	// Local vars at offset -288(s0)
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
	addi        s3, s0, -288
	lb          t0, 528(s1)
	beqz        t0, .LinkerWriteOutput_label_40

	// *** Basic block 1

	li          s4, 3		// 0x3 ASCII \x3
	j           .LinkerWriteOutput_label_42

	// *** Basic block 2

.LinkerWriteOutput_label_40:
	li          s4, 2		// 0x2 ASCII \x2

	// *** Basic block 3

.LinkerWriteOutput_label_42:
	lw          s5, 520(s1)
	lw          s6, 524(s1)
	lb          t0, 529(s1)
	beqz        t0, .LinkerWriteOutput_label_54

	// *** Basic block 4

	mv          s7, x0
	j           .LinkerWriteOutput_label_56

	// *** Basic block 5

.LinkerWriteOutput_label_54:
	la          s7, DynamicLinkerFixupDynamicSectionContents

	// *** Basic block 6

.LinkerWriteOutput_label_56:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a6, t0
	mv          a5, t0
	mv          a4, s7
	mv          a3, s6
	mv          a2, s5
	mv          a1, s4
	mv          a0, s3
	call        ELFWriterFileInit

	// *** Basic block 7

	addi        a1, s0, -288
	mv          a0, s1
	call        BuildSections

	// *** Basic block 8

	addi        a1, s0, -288
	mv          a0, s1
	call        SetEntryAddress

	// *** Basic block 9

	not         t0, a0
	beqz        t0, .LinkerWriteOutput_label_97

	// *** Basic block 10

	addi        a0, s0, -288
	call        ELFWriterFileDestruct

	// *** Basic block 11

.LinkerWriteOutput_label_94:
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

	// *** Basic block 12

.LinkerWriteOutput_label_97:
	addi        a1, s0, -288
	mv          a0, s1
	call        AssignSectionIndexes

	// *** Basic block 13

	addi        a0, s1, 160
	addi        a2, s0, -288
	la          t0, AddSymbolListToOutput
	mv          a1, t0
	call        HashTableTraverse

	// *** Basic block 14

	lb          t0, 529(s1)
	not         t0, t0
	beqz        t0, .LinkerWriteOutput_label_121

	// *** Basic block 15

	mv          a0, s1
	call        DynamicLinkerBuildDynamicRelocations

	// *** Basic block 16

	mv          a0, s1
	call        DynamicLinkerBuildPLTRelocations

	// *** Basic block 17

.LinkerWriteOutput_label_121:
	addi        a1, s0, -288
	mv          a0, s1
	call        InsertSegments

	// *** Basic block 18

	addi        a0, s0, -288
	mv          a1, s2
	call        ELFWriterFileWrite

	// *** Basic block 19

	addi        a0, s0, -288
	call        ELFWriterFileDestruct

	// *** Basic block 20

	j           .LinkerWriteOutput_label_94
.func_end_LinkerWriteOutput:
	.size LinkerWriteOutput, .func_end_LinkerWriteOutput-LinkerWriteOutput

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "(null)"
	.type .str.1, @object
	.size .str.1, 1

.str.2:
	.asciz "%s: linker error: %s\n"
	.type .str.2, @object
	.size .str.2, 22

.str.3:
	.asciz "(null)"
	.type .str.3, @object
	.size .str.3, 1

.str.4:
	.asciz "(null)"
	.type .str.4, @object
	.size .str.4, 1

.str.5:
	.asciz "%s: linker warning: %s: %s\n"
	.type .str.5, @object
	.size .str.5, 28

.str.6:
	.asciz "a.out"
	.type .str.6, @object
	.size .str.6, 6

.str.7:
	.asciz "global-symbol-table"
	.type .str.7, @object
	.size .str.7, 20

.str.8:
	.asciz "/lib/ld.so"
	.type .str.8, @object
	.size .str.8, 11

.str.9:
	.asciz "/usr/lib"
	.type .str.9, @object
	.size .str.9, 9

.str.10:
	.asciz "/lib"
	.type .str.10, @object
	.size .str.10, 5

.str.11:
	.asciz "LD_LIBRARY_PATH"
	.type .str.11, @object
	.size .str.11, 16

.str.12:
	.asciz "(null)"
	.type .str.12, @object
	.size .str.12, 1

.str.13:
	.asciz "Unsupported linker architecture %d\n"
	.type .str.13, @object
	.size .str.13, 36

.str.14:
	.asciz "(null)"
	.type .str.14, @object
	.size .str.14, 1

.str.15:
	.asciz "%s/lib%s%s"
	.type .str.15, @object
	.size .str.15, 11

.str.16:
	.asciz "r"
	.type .str.16, @object
	.size .str.16, 2

.str.17:
	.asciz "(null)"
	.type .str.17, @object
	.size .str.17, 1

.str.18:
	.asciz ".a"
	.type .str.18, @object
	.size .str.18, 3

.str.19:
	.asciz ".so"
	.type .str.19, @object
	.size .str.19, 4

.str.20:
	.asciz "Corrupt symbol table link value"
	.type .str.20, @object
	.size .str.20, 32

.str.21:
	.asciz "Corrupt relocation symbol table section index"
	.type .str.21, @object
	.size .str.21, 46

.str.22:
	.asciz "Corrupt relocation symbol section index"
	.type .str.22, @object
	.size .str.22, 40

.str.23:
	.asciz "Inconsistent ELF machine type"
	.type .str.23, @object
	.size .str.23, 30

.str.24:
	.asciz "Name: %s\n"
	.type .str.24, @object
	.size .str.24, 10

.str.25:
	.asciz "  [%zd]: %s %p @%llx\n"
	.type .str.25, @object
	.size .str.25, 22

.str.26:
	.asciz ".dynsym"
	.type .str.26, @object
	.size .str.26, 8

.str.27:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.27, @object
	.size .str.27, 30

.str.28:
	.asciz "linker.c"
	.type .str.28, @object
	.size .str.28, 9

.str.29:
	.asciz "section != NULL"
	.type .str.29, @object
	.size .str.29, 16

.str.30:
	.asciz ".bss"
	.type .str.30, @object
	.size .str.30, 5

.str.31:
	.asciz ".dynsym"
	.type .str.31, @object
	.size .str.31, 8

.str.32:
	.asciz ".dynstr"
	.type .str.32, @object
	.size .str.32, 8

.str.33:
	.asciz ".rela.dyn"
	.type .str.33, @object
	.size .str.33, 10

.str.34:
	.asciz ".dynsym"
	.type .str.34, @object
	.size .str.34, 8

.str.35:
	.asciz ".rela.plt"
	.type .str.35, @object
	.size .str.35, 10

.str.36:
	.asciz ".dynsym"
	.type .str.36, @object
	.size .str.36, 8

.str.37:
	.asciz ".rela.plt"
	.type .str.37, @object
	.size .str.37, 10

.str.38:
	.asciz ".plt"
	.type .str.38, @object
	.size .str.38, 5

.str.39:
	.asciz ".interp"
	.type .str.39, @object
	.size .str.39, 8

.str.40:
	.asciz ".text"
	.type .str.40, @object
	.size .str.40, 6

.str.41:
	.asciz "main"
	.type .str.41, @object
	.size .str.41, 5

.str.42:
	.asciz "No main function found"
	.type .str.42, @object
	.size .str.42, 23

