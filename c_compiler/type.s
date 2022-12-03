	.file   "type.c"
	.text
	.option pic
.PCbegin:
	.global SizeofType
	.type SizeofType, @function

SizeofType:

	// *** Basic block 0

	.local type_sizes
	.global compiler
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, x0
	la          t2, type_sizes
	lw          t2, 0(t2)
	beqz        t2, .SizeofType_label_56

	// *** Basic block 1

	la          t2, compiler
	ld          t2, 0(t2)
	lw          a0, 1080(t2)

	// *** Basic block 2

.SizeofType_label_25:
	slli        t2, t1, 3
	la          t3, type_sizes
	add         t2, t3, t2
	lw          t3, 0(t2)
	and         t3, t0, t3
	beqz        t3, .SizeofType_label_47

	// *** Basic block 3

	lw          a0, 4(t2)
	li          t0, -1		// 0xffffffffffffffff
	bne         a0, t0, .SizeofType_label_44

	// *** Basic block 4

.SizeofType_label_41:
	ret         

	// *** Basic block 5

.SizeofType_label_44:
	ret         

	// *** Basic block 6

.SizeofType_label_47:

	// *** Basic block 7

.SizeofType_label_48:
	addi        t1, t1, 1
	slli        t0, t1, 3
	la          t2, type_sizes
	add         t0, t2, t0
	lw          t0, 0(t0)
	beqz        t0, .SizeofType_label_25

	// *** Basic block 8

.SizeofType_label_56:
	mv          a0, x0
	ret         
.func_end_SizeofType:
	.size SizeofType, .func_end_SizeofType-SizeofType

	.global SizeofPointer
	.type SizeofPointer, @function

SizeofPointer:

	// *** Basic block 0

	.global compiler
	// Leaf procedure, no stack frame generated
	la          t0, compiler
	ld          t0, 0(t0)
	lw          a0, 1080(t0)

	// *** Basic block 1

.SizeofPointer_label_10:
	ret         
.func_end_SizeofPointer:
	.size SizeofPointer, .func_end_SizeofPointer-SizeofPointer

	.global NewTypeRecord
	.type NewTypeRecord, @function

NewTypeRecord:

	// *** Basic block 0

	.global malloc
	.local next_type_id
	.global memset
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
	li          a0, 88		// 0x58 ASCII 'X'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	la          t0, next_type_id
	lw          t1, 0(t0)
	addi        t0, t1, 1
	la          t1, next_type_id
	sw          t0, 0(t1)
	sw          t1, 0(s3)
	sw          s1, 8(s3)
	sw          s2, 12(s3)
	sw          x0, 20(s3)
	sw          x0, 4(s3)
	sd          x0, 24(s3)
	sw          x0, 16(s3)
	addi        a0, s3, 32
	li          t0, 56		// 0x38 ASCII '8'
	mv          a2, t0
	mv          a1, x0
	call        memset

	// *** Basic block 2

	mv          a0, s3

	// *** Basic block 3

.NewTypeRecord_label_58:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewTypeRecord:
	.size NewTypeRecord, .func_end_NewTypeRecord-NewTypeRecord

	.global TypeRecordDelete
	.type TypeRecordDelete, @function

TypeRecordDelete:

	// *** Basic block 0

	.global TypeRecordDecRef
	.global TypeRecordDelete
	.global TypeIsStructOrUnion
	.global StructDelete
	.global TypeIsEnum
	.global EnumDelete
	.global VectorDestructWithContents
	.global SymbolDestruct
	.global ASTNodeDelete
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
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	bne         s1, x0, .TypeRecordDelete_label_33

	// *** Basic block 1

.TypeRecordDelete_label_30:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.TypeRecordDelete_label_33:
	mv          a0, s1
	call        TypeRecordDecRef

	// *** Basic block 3

	lw          t0, 4(s1)
	bnez        t0, .TypeRecordDelete_label_146

	// *** Basic block 4

	ld          s2, 24(s1)
	beq         s2, x0, .TypeRecordDelete_label_51

	// *** Basic block 5

	mv          a0, s2
	call        TypeRecordDelete

	// *** Basic block 6

	sd          x0, 24(s1)

	// *** Basic block 7

.TypeRecordDelete_label_51:
	mv          a0, s1
	call        TypeIsStructOrUnion

	// *** Basic block 8

	beqz        a0, .TypeRecordDelete_label_69

	// *** Basic block 9

	ld          s2, 32(s1)
	lw          t0, 0(s2)
	addi        t0, t0, -1
	sw          t0, 0(s2)
	bnez        t0, .TypeRecordDelete_label_67

	// *** Basic block 10

	mv          a0, s2
	call        StructDelete

	// *** Basic block 11

.TypeRecordDelete_label_67:
	j           .TypeRecordDelete_label_142

	// *** Basic block 12

.TypeRecordDelete_label_69:
	mv          a0, s1
	call        TypeIsEnum

	// *** Basic block 13

	beqz        a0, .TypeRecordDelete_label_86

	// *** Basic block 14

	ld          s2, 32(s1)
	lw          t0, 0(s2)
	addi        t0, t0, -1
	sw          t0, 0(s2)
	bnez        t0, .TypeRecordDelete_label_84

	// *** Basic block 15

	mv          a0, s2
	call        EnumDelete

	// *** Basic block 16

.TypeRecordDelete_label_84:
	j           .TypeRecordDelete_label_141

	// *** Basic block 17

.TypeRecordDelete_label_86:
	mv          s2, s1
	lw          t0, 16(s2)
	addi        t0, t0, -3
	seqz        s3, t0

	// *** Basic block 18

.TypeRecordDelete_label_95:
	beqz        s3, .TypeRecordDelete_label_108

	// *** Basic block 19

	j           .TypeRecordDelete_label_98

	// *** Basic block 20

.TypeRecordDelete_label_98:
	addi        t0, s1, 32
	addi        a0, t0, 8
	la          t0, SymbolDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 21

	j           .TypeRecordDelete_label_140

	// *** Basic block 22

.TypeRecordDelete_label_108:
	mv          s2, s1
	lw          s4, 16(s2)
	addi        t0, s4, -2
	seqz        s3, t0
	li          t0, 2		// 0x2 ASCII \x2
	beq         s4, t0, .TypeRecordDelete_label_122

	// *** Basic block 23

	addi        t0, s4, -1
	seqz        s3, t0

	// *** Basic block 24

.TypeRecordDelete_label_122:
	beqz        s3, .TypeRecordDelete_label_129

	// *** Basic block 25

	addi        t0, s2, 32
	lb          t0, 16(t0)
	slli        t0, t0, 61
	srai        s3, t0, 63

	// *** Basic block 26

.TypeRecordDelete_label_129:

	// *** Basic block 27

.TypeRecordDelete_label_131:
	beqz        s3, .TypeRecordDelete_label_139

	// *** Basic block 28

	j           .TypeRecordDelete_label_134

	// *** Basic block 29

.TypeRecordDelete_label_134:
	ld          a0, 32(s1)
	call        ASTNodeDelete

	// *** Basic block 30

.TypeRecordDelete_label_139:

	// *** Basic block 31

.TypeRecordDelete_label_140:

	// *** Basic block 32

.TypeRecordDelete_label_141:

	// *** Basic block 33

.TypeRecordDelete_label_142:
	mv          a0, s1
	call        free

	// *** Basic block 34

.TypeRecordDelete_label_146:
	j           .TypeRecordDelete_label_30
.func_end_TypeRecordDelete:
	.size TypeRecordDelete, .func_end_TypeRecordDelete-TypeRecordDelete

	.global TypeRecordAlignment
	.type TypeRecordAlignment, @function

TypeRecordAlignment:

	// *** Basic block 0

	.global TypeRecordAlignment
	.global SizeofPointer
	.global TypeIsStructOrUnion
	.global SizeofType
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
	lw          t0, 16(s1)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .TypeRecordAlignment_label_50

	// *** Basic block 2

	j           .TypeRecordAlignment_label_38

	// *** Basic block 3

	j           .TypeRecordAlignment_label_27

	// *** Basic block 4

	j           .TypeRecordAlignment_label_44

	// *** Basic block 5

.TypeRecordAlignment_label_27:
	ld          a0, 24(s1)
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           TypeRecordAlignment

	// *** Basic block 8

.TypeRecordAlignment_label_38:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SizeofPointer

	// *** Basic block 10

.TypeRecordAlignment_label_44:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SizeofPointer

	// *** Basic block 12

.TypeRecordAlignment_label_50:
	mv          a0, s1
	call        TypeIsStructOrUnion

	// *** Basic block 13

	beqz        a0, .TypeRecordAlignment_label_60

	// *** Basic block 14

	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SizeofPointer

	// *** Basic block 16

.TypeRecordAlignment_label_60:
	lw          a0, 8(s1)
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SizeofType
.func_end_TypeRecordAlignment:
	.size TypeRecordAlignment, .func_end_TypeRecordAlignment-TypeRecordAlignment

	.global TypeRecordIncRef
	.type TypeRecordIncRef, @function

TypeRecordIncRef:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	bne         t0, x0, .TypeRecordIncRef_label_15

	// *** Basic block 1

.TypeRecordIncRef_label_12:
	ret         

	// *** Basic block 2

.TypeRecordIncRef_label_15:
	lw          t1, 4(t0)
	addi        t1, t1, 1
	sw          t1, 4(t0)
	j           .TypeRecordIncRef_label_12
.func_end_TypeRecordIncRef:
	.size TypeRecordIncRef, .func_end_TypeRecordIncRef-TypeRecordIncRef

	.global TypeRecordDecRef
	.type TypeRecordDecRef, @function

TypeRecordDecRef:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	bne         t0, x0, .TypeRecordDecRef_label_15

	// *** Basic block 1

.TypeRecordDecRef_label_12:
	ret         

	// *** Basic block 2

.TypeRecordDecRef_label_15:
	lw          t1, 4(t0)
	addi        t1, t1, -1
	sw          t1, 4(t0)
	j           .TypeRecordDecRef_label_12
.func_end_TypeRecordDecRef:
	.size TypeRecordDecRef, .func_end_TypeRecordDecRef-TypeRecordDecRef

	.global TypeRecordCalculateSize
	.type TypeRecordCalculateSize, @function

TypeRecordCalculateSize:

	// *** Basic block 0

	.global TypeRecordCalculateSize
	.global SizeofPointer
	.global TypeIsStructOrUnion
	.global printf
	.global abort
	.global SizeofType
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
	bne         s1, x0, .TypeRecordCalculateSize_label_31

	// *** Basic block 1

.TypeRecordCalculateSize_label_28:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.TypeRecordCalculateSize_label_31:
	ld          s2, 24(s1)
	mv          a0, s2
	call        TypeRecordCalculateSize

	// *** Basic block 3

	lw          t0, 20(s1)
	bnez        t0, .TypeRecordCalculateSize_label_123

	// *** Basic block 4

	lw          t0, 16(s1)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 5

	j           .TypeRecordCalculateSize_label_78

	// *** Basic block 6

	j           .TypeRecordCalculateSize_label_68

	// *** Basic block 7

	j           .TypeRecordCalculateSize_label_52

	// *** Basic block 8

	j           .TypeRecordCalculateSize_label_73

	// *** Basic block 9

.TypeRecordCalculateSize_label_52:
	addi        t0, s1, 32
	lb          t0, 16(t0)
	slli        t0, t0, 61
	srai        t0, t0, 63
	not         t0, t0
	beqz        t0, .TypeRecordCalculateSize_label_66

	// *** Basic block 10

	lw          t0, 32(s1)
	lw          t1, 20(s2)
	mul         t0, t0, t1
	sw          t0, 20(s1)

	// *** Basic block 11

.TypeRecordCalculateSize_label_66:
	j           .TypeRecordCalculateSize_label_122

	// *** Basic block 12

.TypeRecordCalculateSize_label_68:
	call        SizeofPointer

	// *** Basic block 13

	sw          a0, 20(s1)
	j           .TypeRecordCalculateSize_label_122

	// *** Basic block 14

.TypeRecordCalculateSize_label_73:
	call        SizeofPointer

	// *** Basic block 15

	sw          a0, 20(s1)
	j           .TypeRecordCalculateSize_label_122

	// *** Basic block 16

.TypeRecordCalculateSize_label_78:
	mv          a0, s1
	call        TypeIsStructOrUnion

	// *** Basic block 17

	beqz        a0, .TypeRecordCalculateSize_label_113

	// *** Basic block 18

	ld          s3, 32(s1)
	beq         s3, x0, .TypeRecordCalculateSize_label_89

	// *** Basic block 19

	j           .TypeRecordCalculateSize_label_107

	// *** Basic block 20

.TypeRecordCalculateSize_label_89:
	lla         a0, .str.1
	lla         a1, .str.2
	lla         a3, .str.3
	li          t0, 157		// 0x9d ASCII \x9d
	mv          a2, t0
	call        printf

	// *** Basic block 21

	call        abort

	// *** Basic block 22

.TypeRecordCalculateSize_label_107:
	lw          t0, 76(s3)
	sw          t0, 20(s1)
	j           .TypeRecordCalculateSize_label_120

	// *** Basic block 23

.TypeRecordCalculateSize_label_113:
	lw          a0, 8(s1)
	call        SizeofType

	// *** Basic block 24

	sw          a0, 20(s1)

	// *** Basic block 25

.TypeRecordCalculateSize_label_120:
	j           .TypeRecordCalculateSize_label_122

	// *** Basic block 26

.TypeRecordCalculateSize_label_122:

	// *** Basic block 27

.TypeRecordCalculateSize_label_123:
	j           .TypeRecordCalculateSize_label_28
.func_end_TypeRecordCalculateSize:
	.size TypeRecordCalculateSize, .func_end_TypeRecordCalculateSize-TypeRecordCalculateSize

	.global TypeRecordChain
	.type TypeRecordChain, @function

TypeRecordChain:

	// *** Basic block 0

	.global TypeRecordIncRef
	.global printf
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
	call        TypeRecordIncRef

	// *** Basic block 1

	bne         s1, x0, .TypeRecordChain_label_25

	// *** Basic block 2

	lla         a0, .str.4
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           printf

	// *** Basic block 3

.TypeRecordChain_label_25:
	sd          s1, 24(s2)
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeRecordChain:
	.size TypeRecordChain, .func_end_TypeRecordChain-TypeRecordChain

	.local  CloneVLAExpr
	.type CloneVLAExpr, @function

CloneVLAExpr:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated

	// *** Basic block 1

.CloneVLAExpr_label_7:
	ret         
.func_end_CloneVLAExpr:
	.size CloneVLAExpr, .func_end_CloneVLAExpr-CloneVLAExpr

	.global TypeRecordCopy
	.type TypeRecordCopy, @function

TypeRecordCopy:

	// *** Basic block 0

	.global malloc
	.global memcpy
	.local next_type_id
	.global TypeRecordIncRef
	.global TypeIsStructOrUnion
	.global TypeIsEnum
	.global ASTNodeClone
	.local CloneVLAExpr
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
	li          s2, 88		// 0x58 ASCII 'X'
	mv          a0, s2
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        memcpy

	// *** Basic block 2

	la          t0, next_type_id
	lw          t0, 0(t0)
	sw          t0, 0(s3)
	sw          x0, 4(s3)
	ld          s2, 24(s3)
	beq         s2, x0, .TypeRecordCopy_label_53

	// *** Basic block 3

	mv          a0, s2
	call        TypeRecordIncRef

	// *** Basic block 4

.TypeRecordCopy_label_53:
	mv          a0, s1
	call        TypeIsStructOrUnion

	// *** Basic block 5

	beqz        a0, .TypeRecordCopy_label_64

	// *** Basic block 6

	ld          t0, 32(s1)
	lw          t1, 0(t0)
	addi        t1, t1, 1
	sw          t1, 0(t0)
	j           .TypeRecordCopy_label_119

	// *** Basic block 7

.TypeRecordCopy_label_64:
	mv          a0, s1
	call        TypeIsEnum

	// *** Basic block 8

	beqz        a0, .TypeRecordCopy_label_75

	// *** Basic block 9

	ld          t0, 32(s1)
	lw          t1, 0(t0)
	addi        t1, t1, 1
	sw          t1, 0(t0)
	j           .TypeRecordCopy_label_118

	// *** Basic block 10

.TypeRecordCopy_label_75:
	mv          s2, s1
	lw          s5, 16(s2)
	addi        t0, s5, -2
	seqz        s4, t0
	li          t0, 2		// 0x2 ASCII \x2
	beq         s5, t0, .TypeRecordCopy_label_90

	// *** Basic block 11

	addi        t0, s5, -1
	seqz        s4, t0

	// *** Basic block 12

.TypeRecordCopy_label_90:
	beqz        s4, .TypeRecordCopy_label_97

	// *** Basic block 13

	addi        t0, s2, 32
	lb          t0, 16(t0)
	slli        t0, t0, 61
	srai        s4, t0, 63

	// *** Basic block 14

.TypeRecordCopy_label_97:

	// *** Basic block 15

.TypeRecordCopy_label_99:
	beqz        s4, .TypeRecordCopy_label_117

	// *** Basic block 16

	j           .TypeRecordCopy_label_102

	// *** Basic block 17

.TypeRecordCopy_label_102:
	ld          a0, 32(s1)
	mv          a3, x0
	mv          a2, x0
	la          t0, CloneVLAExpr
	mv          a1, t0
	call        ASTNodeClone

	// *** Basic block 18

	sd          a0, 32(s3)

	// *** Basic block 19

.TypeRecordCopy_label_117:

	// *** Basic block 20

.TypeRecordCopy_label_118:

	// *** Basic block 21

.TypeRecordCopy_label_119:
	mv          a0, s3

	// *** Basic block 22

.TypeRecordCopy_label_122:
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
.func_end_TypeRecordCopy:
	.size TypeRecordCopy, .func_end_TypeRecordCopy-TypeRecordCopy

	.global NewPointerTypeRecord
	.type NewPointerTypeRecord, @function

NewPointerTypeRecord:

	// *** Basic block 0

	.global NewTypeRecord
	.global compiler
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
	mv          a1, t0
	mv          a0, x0
	call        NewTypeRecord

	// *** Basic block 1

	mv          s1, a0
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 16(s1)
	la          t0, compiler
	ld          t0, 0(t0)
	lw          t0, 1080(t0)
	sw          t0, 20(s1)
	mv          a0, s1

	// *** Basic block 2

.NewPointerTypeRecord_label_32:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewPointerTypeRecord:
	.size NewPointerTypeRecord, .func_end_NewPointerTypeRecord-NewPointerTypeRecord

	.global NewPointerTo
	.type NewPointerTo, @function

NewPointerTo:

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
	.global NewPointerTypeRecord
	.global TypeRecordChain
	mv          s1, a1
	call        NewPointerTypeRecord

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        TypeRecordChain

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.NewPointerTo_label_22:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewPointerTo:
	.size NewPointerTo, .func_end_NewPointerTo-NewPointerTo

	.global NewArrayTypeRecord
	.type NewArrayTypeRecord, @function

NewArrayTypeRecord:

	// *** Basic block 0

	.global NewTypeRecord
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
	mv          a1, t0
	mv          a0, x0
	call        NewTypeRecord

	// *** Basic block 1

	mv          s2, a0
	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, 16(s2)
	sw          x0, 32(s2)
	addi        t0, s2, 32
	lb          t1, 16(t0)
	andi        t1, t1, -2
	sb          t1, 16(t0)
	addi        t0, s2, 32
	lb          t1, 16(t0)
	andi        t1, t1, -3
	andi        t2, s1, 1
	slli        t2, t2, 1
	or          t1, t1, t2
	sb          t1, 16(t0)
	sd          x0, 32(s2)
	addi        t0, s2, 32
	sd          x0, 8(t0)
	addi        t0, s2, 32
	lb          t1, 16(t0)
	andi        t1, t1, -5
	sb          t1, 16(t0)
	addi        t0, s2, 32
	lb          t1, 16(t0)
	andi        t1, t1, -9
	sb          t1, 16(t0)
	sw          x0, 20(s2)
	mv          a0, s2

	// *** Basic block 2

.NewArrayTypeRecord_label_65:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewArrayTypeRecord:
	.size NewArrayTypeRecord, .func_end_NewArrayTypeRecord-NewArrayTypeRecord

	.global NewBasicArrayTypeRecord
	.type NewBasicArrayTypeRecord, @function

NewBasicArrayTypeRecord:

	// *** Basic block 0

	.global NewArrayTypeRecord
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
	mv          a1, x0
	call        NewArrayTypeRecord

	// *** Basic block 1

	mv          s3, a0
	sw          s1, 32(s3)
	addi        t0, s3, 32
	lb          t1, 16(t0)
	andi        t1, t1, -2
	andi        t2, s2, 1
	or          t1, t1, t2
	sb          t1, 16(t0)
	mv          a0, s3

	// *** Basic block 2

.NewBasicArrayTypeRecord_label_36:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewBasicArrayTypeRecord:
	.size NewBasicArrayTypeRecord, .func_end_NewBasicArrayTypeRecord-NewBasicArrayTypeRecord

	.global NewFunctionTypeRecord
	.type NewFunctionTypeRecord, @function

NewFunctionTypeRecord:

	// *** Basic block 0

	.global NewTypeRecord
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
	mv          a1, x0
	mv          a0, x0
	call        NewTypeRecord

	// *** Basic block 1

	mv          s1, a0
	li          t0, 3		// 0x3 ASCII \x3
	sw          t0, 16(s1)
	sw          x0, 20(s1)
	sd          x0, 32(s1)
	addi        t0, s1, 32
	sb          x0, 32(t0)
	addi        t0, s1, 32
	sb          x0, 48(t0)
	addi        t0, s1, 32
	sb          x0, 49(t0)
	addi        t0, s1, 32
	sb          x0, 52(t0)
	addi        t0, s1, 32
	sb          x0, 53(t0)
	addi        t0, s1, 32
	sb          x0, 50(t0)
	addi        t0, s1, 32
	sb          x0, 51(t0)
	addi        t0, s1, 32
	sd          x0, 40(t0)
	addi        t0, s1, 32
	addi        a0, t0, 8
	call        VectorInit

	// *** Basic block 2

	mv          a0, s1

	// *** Basic block 3

.NewFunctionTypeRecord_label_64:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewFunctionTypeRecord:
	.size NewFunctionTypeRecord, .func_end_NewFunctionTypeRecord-NewFunctionTypeRecord

	.global NewSizeTypeRecord
	.type NewSizeTypeRecord, @function

NewSizeTypeRecord:

	// *** Basic block 0

	.global compiler
	.global NewTypeRecord
	// Leaf procedure, no stack frame generated
	la          t0, compiler
	ld          t0, 0(t0)
	lw          t0, 1080(t0)
	li          t1, 8		// 0x8 ASCII \x8
	bne         t0, t1, .NewSizeTypeRecord_label_33

	// *** Basic block 1

	mv          a1, x0
	li          t0, 16400		// 0x4010
	mv          a0, t0
	j           NewTypeRecord

	// *** Basic block 4

.NewSizeTypeRecord_label_33:
	mv          a1, x0
	li          t0, 16386		// 0x4002
	mv          a0, t0
	j           NewTypeRecord
.func_end_NewSizeTypeRecord:
	.size NewSizeTypeRecord, .func_end_NewSizeTypeRecord-NewSizeTypeRecord

	.global NewStructMember
	.type NewStructMember, @function

NewStructMember:

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
	li          a0, 32		// 0x20 ASCII ' '
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	sd          s1, 0(s2)
	sw          x0, 8(s2)
	sw          x0, 12(s2)
	sw          x0, 16(s2)
	mv          a0, s2

	// *** Basic block 2

.NewStructMember_label_26:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewStructMember:
	.size NewStructMember, .func_end_NewStructMember-NewStructMember

	.global StructMemberDelete
	.type StructMemberDelete, @function

StructMemberDelete:

	// *** Basic block 0

	.global SymbolDelete
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
	ld          a0, 0(s1)
	call        SymbolDelete

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_StructMemberDelete:
	.size StructMemberDelete, .func_end_StructMemberDelete-StructMemberDelete

	.global StructMemberIsBitField
	.type StructMemberIsBitField, @function

StructMemberIsBitField:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 16(a0)
	slt         a0, x0, t0

	// *** Basic block 1

.StructMemberIsBitField_label_12:
	ret         
.func_end_StructMemberIsBitField:
	.size StructMemberIsBitField, .func_end_StructMemberIsBitField-StructMemberIsBitField

	.local  CompareStructMember
	.type CompareStructMember, @function

CompareStructMember:

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
.func_end_CompareStructMember:
	.size CompareStructMember, .func_end_CompareStructMember-CompareStructMember

	.global NewStruct
	.type NewStruct, @function

NewStruct:

	// *** Basic block 0

	.global malloc
	.global VectorInit
	.global MapInit
	.local CompareStructMember
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
	li          a0, 96		// 0x60 ASCII '`'
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 0(s2)
	addi        a0, s2, 16
	call        VectorInit

	// *** Basic block 2

	addi        a0, s2, 40
	la          t0, CompareStructMember
	mv          a1, t0
	call        MapInit

	// *** Basic block 3

	sb          s1, 80(s2)
	sw          x0, 72(s2)
	sw          x0, 88(s2)
	sw          x0, 76(s2)
	li          t0, 65		// 0x41 ASCII 'A'
	sw          t0, 84(s2)
	mv          a0, s2

	// *** Basic block 4

.NewStruct_label_51:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewStruct:
	.size NewStruct, .func_end_NewStruct-NewStruct

	.global StructDelete
	.type StructDelete, @function

StructDelete:

	// *** Basic block 0

	.global VectorDestructWithContents
	.global StructMemberDelete
	.global MapDestruct
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
	addi        a0, s1, 16
	la          t0, StructMemberDelete
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 1

	addi        a0, s1, 40
	call        MapDestruct

	// *** Basic block 2

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_StructDelete:
	.size StructDelete, .func_end_StructDelete-StructDelete

	.global NewEnumConstant
	.type NewEnumConstant, @function

NewEnumConstant:

	// *** Basic block 0

	.global NewTypeRecord
	.global NewSymbol
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
	li          a1, 4		// 0x4 ASCII \x4
	li          a0, 2		// 0x2 ASCII \x2
	call        NewTypeRecord

	// *** Basic block 1

	mv          s3, a0
	mv          a2, x0
	mv          a1, s3
	mv          a0, s1
	call        NewSymbol

	// *** Basic block 2

	mv          s4, a0
	sd          s2, 112(s4)
	mv          a0, s4

	// *** Basic block 3

.NewEnumConstant_label_36:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewEnumConstant:
	.size NewEnumConstant, .func_end_NewEnumConstant-NewEnumConstant

	.global NewEnum
	.type NewEnum, @function

NewEnum:

	// *** Basic block 0

	.global malloc
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
	li          a0, 48		// 0x30 ASCII '0'
	call        malloc

	// *** Basic block 1

	mv          s1, a0
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 0(s1)
	addi        a0, s1, 16
	call        VectorInit

	// *** Basic block 2

	sw          x0, 40(s1)
	mv          a0, s1

	// *** Basic block 3

.NewEnum_label_25:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewEnum:
	.size NewEnum, .func_end_NewEnum-NewEnum

	.global EnumDelete
	.type EnumDelete, @function

EnumDelete:

	// *** Basic block 0

	.global VectorDestruct
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
	addi        a0, s1, 16
	call        VectorDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_EnumDelete:
	.size EnumDelete, .func_end_EnumDelete-EnumDelete

	.local  TypeToString
	.type TypeToString, @function

TypeToString:

	// *** Basic block 0

	.global StringAppend
	.local type_names
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
	li          t0, 8192		// 0x2000
	and         t0, s1, t0
	beqz        t0, .TypeToString_label_29

	// *** Basic block 1

	lla         a1, .str.19
	mv          a0, s2
	call        StringAppend

	// *** Basic block 2

.TypeToString_label_29:
	li          t0, 16384		// 0x4000
	and         t0, s1, t0
	beqz        t0, .TypeToString_label_40

	// *** Basic block 3

	lla         a1, .str.20
	mv          a0, s2
	call        StringAppend

	// *** Basic block 4

.TypeToString_label_40:
	mv          s3, x0
	la          t0, type_names
	lw          t0, 0(t0)
	beqz        t0, .TypeToString_label_79

	// *** Basic block 5

.TypeToString_label_49:
	slli        t0, s3, 4
	la          t1, type_names
	add         s4, t1, t0
	lw          t0, 0(s4)
	and         t0, s1, t0
	beqz        t0, .TypeToString_label_70

	// *** Basic block 6

	ld          a1, 8(s4)
	mv          a0, s2
	call        StringAppend

	// *** Basic block 7

	lla         a1, .str.21
	mv          a0, s2
	call        StringAppend

	// *** Basic block 8

.TypeToString_label_70:

	// *** Basic block 9

.TypeToString_label_71:
	addi        s3, s3, 1
	slli        t0, s3, 4
	la          t1, type_names
	add         t0, t1, t0
	lw          t0, 0(t0)
	beqz        t0, .TypeToString_label_49

	// *** Basic block 10

.TypeToString_label_79:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeToString:
	.size TypeToString, .func_end_TypeToString-TypeToString

	.local  QualifiersToString
	.type QualifiersToString, @function

QualifiersToString:

	// *** Basic block 0

	.global StringAppend
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
	andi        t0, s1, 4
	beqz        t0, .QualifiersToString_label_25

	// *** Basic block 1

	lla         a1, .str.22
	mv          a0, s2
	call        StringAppend

	// *** Basic block 2

.QualifiersToString_label_25:
	andi        t0, s1, 8
	beqz        t0, .QualifiersToString_label_35

	// *** Basic block 3

	lla         a1, .str.23
	mv          a0, s2
	call        StringAppend

	// *** Basic block 4

.QualifiersToString_label_35:
	andi        t0, s1, 16
	beqz        t0, .QualifiersToString_label_46

	// *** Basic block 5

	lla         a1, .str.24
	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           StringAppend

	// *** Basic block 6

.QualifiersToString_label_46:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_QualifiersToString:
	.size QualifiersToString, .func_end_QualifiersToString-QualifiersToString

	.global TypeRecordPrintDetails
	.type TypeRecordPrintDetails, @function

TypeRecordPrintDetails:

	// *** Basic block 0

	.global fprintf
	.local QualifiersToString
	.local TypeToString
	.global SymbolPrint
	.global ASTNodePrint
	.global StringDestruct
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -64(s0)
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
	mv          s2, a2
	mv          s3, a1
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	mv          s4, x0
	beq         s1, x0, .TypeRecordPrintDetails_label_262

	// *** Basic block 1

.TypeRecordPrintDetails_label_64:
	lw          s5, 16(s1)
	li          t0, 1		// 0x1 ASCII \x1
	bne         s5, t0, .TypeRecordPrintDetails_label_78

	// *** Basic block 2

	lla         a1, .str.25
	mv          a0, s2
	call        fprintf

	// *** Basic block 3

.TypeRecordPrintDetails_label_78:
	lw          a0, 12(s1)
	addi        a1, s0, -64
	call        QualifiersToString

	// *** Basic block 4

	lw          a0, 8(s1)
	addi        a1, s0, -64
	call        TypeToString

	// *** Basic block 5

	lla         a1, .str.26
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s2
	call        fprintf

	// *** Basic block 6

	li          s6, 2		// 0x2 ASCII \x2
	bne         s5, s6, .TypeRecordPrintDetails_label_130

	// *** Basic block 7

	addi        t0, s1, 32
	lb          t0, 16(t0)
	slli        t0, t0, 61
	srai        t0, t0, 63
	beqz        t0, .TypeRecordPrintDetails_label_119

	// *** Basic block 8

	lla         a1, .str.27
	mv          a0, s2
	call        fprintf

	// *** Basic block 9

	j           .TypeRecordPrintDetails_label_128

	// *** Basic block 10

.TypeRecordPrintDetails_label_119:
	lla         a1, .str.28
	lw          a2, 32(s1)
	mv          a0, s2
	call        fprintf

	// *** Basic block 11

.TypeRecordPrintDetails_label_128:
	j           .TypeRecordPrintDetails_label_256

	// *** Basic block 12

.TypeRecordPrintDetails_label_130:
	li          t0, 3		// 0x3 ASCII \x3
	bne         s5, t0, .TypeRecordPrintDetails_label_255

	// *** Basic block 13

	lla         a1, .str.29
	mv          a0, s2
	call        fprintf

	// *** Basic block 14

	lla         s5, .str.30
	addi        s7, s1, 32
	addi        t0, s7, 8
	ld          s8, 8(t0)
	mv          s9, x0
	bge         x0, s8, .TypeRecordPrintDetails_label_179

	// *** Basic block 15

	ld          s10, 8(s7)

	// *** Basic block 16

.TypeRecordPrintDetails_label_155:
	slli        t0, s9, 3
	add         t0, s10, t0
	ld          s10, 0(t0)
	lla         a1, .str.31
	mv          a2, s5
	mv          a0, s2
	call        fprintf

	// *** Basic block 17

	lla         s5, .str.32
	mv          a1, s2
	mv          a0, s10
	call        SymbolPrint

	// *** Basic block 18

.TypeRecordPrintDetails_label_175:
	addi        s9, s9, 1
	bge         s9, s8, .TypeRecordPrintDetails_label_155

	// *** Basic block 19

.TypeRecordPrintDetails_label_179:
	lb          t0, 32(s7)
	beqz        t0, .TypeRecordPrintDetails_label_191

	// *** Basic block 20

	lla         a1, .str.33
	mv          a2, s5
	mv          a0, s2
	call        fprintf

	// *** Basic block 21

.TypeRecordPrintDetails_label_191:
	beqz        s3, .TypeRecordPrintDetails_label_247

	// *** Basic block 22

	lla         a1, .str.34
	mv          a0, s2
	call        fprintf

	// *** Basic block 23

	ld          s3, 40(s7)
	ld          s3, 56(s3)
	ld          s5, 8(s3)
	bge         x0, s5, .TypeRecordPrintDetails_label_217

	// *** Basic block 24

	ld          s3, 0(s3)
	lla         a1, .str.35
	mv          a0, s2
	call        fprintf

	// *** Basic block 25

	li          s4, 1		// 0x1 ASCII \x1

	// *** Basic block 26

.TypeRecordPrintDetails_label_217:
	mv          s3, x0
	bge         x0, s5, .TypeRecordPrintDetails_label_239

	// *** Basic block 27

.TypeRecordPrintDetails_label_222:
	slli        t0, s3, 3
	add         t0, s3, t0
	ld          s7, 0(t0)
	mv          a2, s2
	mv          a1, s6
	mv          a0, s7
	call        ASTNodePrint

	// *** Basic block 28

.TypeRecordPrintDetails_label_235:
	addi        s3, s3, 1
	bge         s3, s5, .TypeRecordPrintDetails_label_222

	// *** Basic block 29

.TypeRecordPrintDetails_label_239:
	lla         a1, .str.36
	mv          a0, s2
	call        fprintf

	// *** Basic block 30

	j           .TypeRecordPrintDetails_label_254

	// *** Basic block 31

.TypeRecordPrintDetails_label_247:
	lla         a1, .str.37
	mv          a0, s2
	call        fprintf

	// *** Basic block 32

.TypeRecordPrintDetails_label_254:

	// *** Basic block 33

.TypeRecordPrintDetails_label_255:

	// *** Basic block 34

.TypeRecordPrintDetails_label_256:
	ld          s1, 24(s1)
	bne         s1, x0, .TypeRecordPrintDetails_label_64

	// *** Basic block 35

.TypeRecordPrintDetails_label_262:
	beqz        s4, .TypeRecordPrintDetails_label_270

	// *** Basic block 36

	lla         a1, .str.38
	mv          a0, s2
	call        fprintf

	// *** Basic block 37

.TypeRecordPrintDetails_label_270:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 38

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
.func_end_TypeRecordPrintDetails:
	.size TypeRecordPrintDetails, .func_end_TypeRecordPrintDetails-TypeRecordPrintDetails

	.global TypeRecordPrint
	.type TypeRecordPrint, @function

TypeRecordPrint:

	// *** Basic block 0

	.global TypeRecordPrintDetails
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          a2, t0
	mv          a1, x0
	j           TypeRecordPrintDetails
.func_end_TypeRecordPrint:
	.size TypeRecordPrint, .func_end_TypeRecordPrint-TypeRecordPrint

	.global TypeRecordToString
	.type TypeRecordToString, @function

TypeRecordToString:

	// *** Basic block 0

	.local QualifiersToString
	.local TypeToString
	.global TypeIsStructOrUnion
	.global StringAppendString
	.global TypeIsEnum
	.global TypeRecordToString
	.global StringAppend
	.global StringAppendChar
	.global StringPrintf
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
	lw          t0, 16(s1)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .TypeRecordToString_label_45

	// *** Basic block 2

	j           .TypeRecordToString_label_87

	// *** Basic block 3

	j           .TypeRecordToString_label_142

	// *** Basic block 4

	j           .TypeRecordToString_label_160

	// *** Basic block 5

.TypeRecordToString_label_45:
	lw          a0, 12(s1)
	mv          a1, s2
	call        QualifiersToString

	// *** Basic block 6

	lw          a0, 8(s1)
	mv          a1, s2
	call        TypeToString

	// *** Basic block 7

	mv          a0, s1
	call        TypeIsStructOrUnion

	// *** Basic block 8

	beqz        a0, .TypeRecordToString_label_71

	// *** Basic block 9

	ld          t0, 32(s1)
	ld          a1, 8(t0)
	mv          a0, s2
	call        StringAppendString

	// *** Basic block 10

	j           .TypeRecordToString_label_85

	// *** Basic block 11

.TypeRecordToString_label_71:
	mv          a0, s1
	call        TypeIsEnum

	// *** Basic block 12

	beqz        a0, .TypeRecordToString_label_84

	// *** Basic block 13

	ld          t0, 32(s1)
	ld          a1, 8(t0)
	mv          a0, s2
	call        StringAppendString

	// *** Basic block 14

.TypeRecordToString_label_84:

	// *** Basic block 15

.TypeRecordToString_label_85:
	j           .TypeRecordToString_label_216

	// *** Basic block 16

.TypeRecordToString_label_87:
	ld          s3, 24(s1)
	mv          a1, s2
	mv          a0, s3
	call        TypeRecordToString

	// *** Basic block 17

	lw          s3, 16(s3)
	addi        t1, s3, -2
	seqz        t0, t1
	li          t1, 2		// 0x2 ASCII \x2
	beq         s3, t1, .TypeRecordToString_label_106

	// *** Basic block 18

	addi        t1, s3, -3
	seqz        t0, t1

	// *** Basic block 19

.TypeRecordToString_label_106:
	beqz        t0, .TypeRecordToString_label_127

	// *** Basic block 20

	lla         a1, .str.39
	mv          a0, s2
	call        StringAppend

	// *** Basic block 21

	lw          a0, 12(s1)
	mv          a1, s2
	call        QualifiersToString

	// *** Basic block 22

	li          t0, 41		// 0x29 ASCII ')'
	mv          a1, t0
	mv          a0, s2
	call        StringAppendChar

	// *** Basic block 23

	j           .TypeRecordToString_label_140

	// *** Basic block 24

.TypeRecordToString_label_127:
	li          t0, 42		// 0x2a ASCII '*'
	mv          a1, t0
	mv          a0, s2
	call        StringAppendChar

	// *** Basic block 25

	lw          a0, 12(s1)
	mv          a1, s2
	call        QualifiersToString

	// *** Basic block 26

.TypeRecordToString_label_140:
	j           .TypeRecordToString_label_216

	// *** Basic block 27

.TypeRecordToString_label_142:
	ld          a0, 24(s1)
	mv          a1, s2
	call        TypeRecordToString

	// *** Basic block 28

	lla         a1, .str.40
	lw          a2, 32(s1)
	mv          a0, s2
	call        StringPrintf

	// *** Basic block 29

	j           .TypeRecordToString_label_216

	// *** Basic block 30

.TypeRecordToString_label_160:
	ld          a0, 24(s1)
	mv          a1, s2
	call        TypeRecordToString

	// *** Basic block 31

	li          t0, 40		// 0x28 ASCII '('
	mv          a1, t0
	mv          a0, s2
	call        StringAppendChar

	// *** Basic block 32

	lla         s3, .str.41
	addi        s4, s1, 32
	addi        t0, s4, 8
	ld          s5, 8(t0)
	mv          s6, x0
	bge         x0, s5, .TypeRecordToString_label_208

	// *** Basic block 33

	ld          s4, 8(s4)

	// *** Basic block 34

.TypeRecordToString_label_186:
	slli        t0, s6, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	mv          a1, s3
	mv          a0, s2
	call        StringAppend

	// *** Basic block 35

	ld          a0, 40(s4)
	mv          a1, s2
	call        TypeRecordToString

	// *** Basic block 37

.TypeRecordToString_label_204:
	addi        s6, s6, 1
	bge         s6, s5, .TypeRecordToString_label_186

	// *** Basic block 38

.TypeRecordToString_label_208:
	li          t0, 41		// 0x29 ASCII ')'
	mv          a1, t0
	mv          a0, s2
	call        StringAppendChar

	// *** Basic block 39

	j           .TypeRecordToString_label_216

	// *** Basic block 40

.TypeRecordToString_label_216:
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
.func_end_TypeRecordToString:
	.size TypeRecordToString, .func_end_TypeRecordToString-TypeRecordToString

	.global TypeParserInit
	.type TypeParserInit, @function

TypeParserInit:

	// *** Basic block 0

	.global VectorInit
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	sd          a1, 0(t0)
	sd          a2, 8(t0)
	sw          a3, 56(t0)
	addi        a0, t0, 24
	j           VectorInit
.func_end_TypeParserInit:
	.size TypeParserInit, .func_end_TypeParserInit-TypeParserInit

	.global TypeParserReset
	.type TypeParserReset, @function

TypeParserReset:

	// *** Basic block 0

	.global VectorDestruct
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
	sd          x0, 16(s1)
	sw          x0, 56(s1)
	sb          x0, 60(s1)
	sw          x0, 64(s1)
	addi        a0, s1, 24
	call        VectorDestruct

	// *** Basic block 1

	addi        a0, s1, 24
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorInit
.func_end_TypeParserReset:
	.size TypeParserReset, .func_end_TypeParserReset-TypeParserReset

	.local  ParseTypeSpecifier
	.type ParseTypeSpecifier, @function

ParseTypeSpecifier:

	// *** Basic block 0

	.local type_map
	.global LexNextToken
	.global LexMatch
	.global StringInit
	.global SyntaxFindSymbol
	.global StorageIs
	.global TypeRecordCopy
	.global StringDestruct
	.global TypeParserInit
	.global TypeParserParseStruct
	.global TypeParserParseEnum
	addi sp, sp, -224
	// Saved return address (offset 216) and frame pointer (offset 208)
	sd ra, 216(sp)
	sd s0, 208(sp)
	addi s0, sp, 224
	// Local vars at offset -144(s0)
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
	sb          x0, 16(s1)
	mv          s3, x0
	mv          s4, x0
	ld          s5, 0(s2)
	mv          s6, x0
	lw          s7, 72(s5)
	mv          s8, x0
	lb          t0, 60(s2)
	beqz        t0, .ParseTypeSpecifier_label_63

	// *** Basic block 1

	li          s3, 512		// 0x200
	sb          x0, 60(s2)
	li          s8, 1		// 0x1 ASCII \x1
	j           .ParseTypeSpecifier_label_97

	// *** Basic block 2

.ParseTypeSpecifier_label_63:
	mv          s9, x0
	la          t0, type_map
	lw          t0, 0(t0)
	beqz        t0, .ParseTypeSpecifier_label_96

	// *** Basic block 3

.ParseTypeSpecifier_label_71:
	slli        t0, s9, 3
	la          t1, type_map
	add         s10, t1, t0
	lw          t0, 0(s10)
	bne         t0, s7, .ParseTypeSpecifier_label_87

	// *** Basic block 4

	mv          a0, s5
	call        LexNextToken

	// *** Basic block 5

	lw          t0, 4(s10)
	or          s3, x0, t0
	li          s8, 1		// 0x1 ASCII \x1
	j           .ParseTypeSpecifier_label_96

	// *** Basic block 6

.ParseTypeSpecifier_label_87:

	// *** Basic block 7

.ParseTypeSpecifier_label_88:
	addi        s9, s9, 1
	slli        t0, s9, 3
	la          t1, type_map
	add         t0, t1, t0
	lw          t0, 0(t0)
	beqz        t0, .ParseTypeSpecifier_label_71

	// *** Basic block 8

.ParseTypeSpecifier_label_96:

	// *** Basic block 9

.ParseTypeSpecifier_label_97:
	not         t0, s8
	beqz        t0, .ParseTypeSpecifier_label_183

	// *** Basic block 10

	li          t0, 61		// 0x3d ASCII '='
	mv          a1, t0
	mv          a0, s5
	call        LexMatch

	// *** Basic block 11

	beqz        a0, .ParseTypeSpecifier_label_109

	// *** Basic block 12

	li          s4, 4		// 0x4 ASCII \x4
	j           .ParseTypeSpecifier_label_182

	// *** Basic block 13

.ParseTypeSpecifier_label_109:
	li          t0, 92		// 0x5c ASCII '\'
	mv          a1, t0
	mv          a0, s5
	call        LexMatch

	// *** Basic block 14

	beqz        a0, .ParseTypeSpecifier_label_119

	// *** Basic block 15

	li          s4, 8		// 0x8 ASCII \x8
	j           .ParseTypeSpecifier_label_181

	// *** Basic block 16

.ParseTypeSpecifier_label_119:
	li          t0, 79		// 0x4f ASCII 'O'
	mv          a1, t0
	mv          a0, s5
	call        LexMatch

	// *** Basic block 17

	beqz        a0, .ParseTypeSpecifier_label_129

	// *** Basic block 18

	li          s4, 16		// 0x10 ASCII \x10
	j           .ParseTypeSpecifier_label_180

	// *** Basic block 19

.ParseTypeSpecifier_label_129:
	li          t0, 3		// 0x3 ASCII \x3
	bne         s7, t0, .ParseTypeSpecifier_label_179

	// *** Basic block 20

	addi        a0, s0, -144
	addi        t0, s5, 80
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 21

	ld          a0, 8(s2)
	addi        a1, s0, -144
	call        SyntaxFindSymbol

	// *** Basic block 22

	mv          s7, a0
	beq         s7, x0, .ParseTypeSpecifier_label_175

	// *** Basic block 23

	lw          a0, 48(s7)
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	call        StorageIs

	// *** Basic block 24

	beqz        a0, .ParseTypeSpecifier_label_174

	// *** Basic block 25

	mv          a0, s5
	call        LexNextToken

	// *** Basic block 26

	ld          a0, 40(s7)
	call        TypeRecordCopy

	// *** Basic block 27

	mv          s6, a0
	lw          t0, 8(s6)
	or          s3, s3, t0

	// *** Basic block 28

.ParseTypeSpecifier_label_174:

	// *** Basic block 29

.ParseTypeSpecifier_label_175:
	addi        a0, s0, -144
	call        StringDestruct

	// *** Basic block 30

.ParseTypeSpecifier_label_179:

	// *** Basic block 31

.ParseTypeSpecifier_label_180:

	// *** Basic block 32

.ParseTypeSpecifier_label_181:

	// *** Basic block 33

.ParseTypeSpecifier_label_182:

	// *** Basic block 34

.ParseTypeSpecifier_label_183:
	sub         t1, s6, x0
	seqz        t0, t1
	bne         s6, x0, .ParseTypeSpecifier_label_191

	// *** Basic block 35

	li          t1, 7168		// 0x1c00
	and         t1, s3, t1
	snez        t0, t1

	// *** Basic block 36

.ParseTypeSpecifier_label_191:
	beqz        t0, .ParseTypeSpecifier_label_244

	// *** Basic block 37

	addi        a0, s0, -104
	ld          a1, 0(s2)
	ld          a2, 8(s2)
	li          t0, 3		// 0x3 ASCII \x3
	mv          a4, t0
	mv          a3, x0
	call        TypeParserInit

	// *** Basic block 38

	li          t0, 3072		// 0xc00
	and         t0, s3, t0
	beqz        t0, .ParseTypeSpecifier_label_226

	// *** Basic block 39

	li          t0, 2048		// 0x800
	and         t0, s3, t0
	snez        s5, t0
	addi        a0, s0, -104
	mv          a1, s5
	call        TypeParserParseStruct

	// *** Basic block 40

	mv          s5, a0
	j           .ParseTypeSpecifier_label_231

	// *** Basic block 41

.ParseTypeSpecifier_label_226:
	addi        a0, s0, -104
	call        TypeParserParseEnum

	// *** Basic block 42

	mv          s5, a0

	// *** Basic block 43

.ParseTypeSpecifier_label_231:
	beq         s5, x0, .ParseTypeSpecifier_label_243

	// *** Basic block 44

	ld          a0, 40(s5)
	call        TypeRecordCopy

	// *** Basic block 45

	mv          s6, a0
	lw          t0, 8(s6)
	or          s3, s3, t0

	// *** Basic block 46

.ParseTypeSpecifier_label_243:

	// *** Basic block 47

.ParseTypeSpecifier_label_244:
	sw          s3, 0(s1)
	sw          s4, 4(s1)
	sd          s6, 8(s1)

	// *** Basic block 48

.ParseTypeSpecifier_label_250:
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
.func_end_ParseTypeSpecifier:
	.size ParseTypeSpecifier, .func_end_ParseTypeSpecifier-ParseTypeSpecifier

	.local  IsValidType
	.type IsValidType, @function

IsValidType:

	// *** Basic block 0

	.local valid_types
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, x0

	// *** Basic block 1

.IsValidType_label_16:
	slli        t2, t1, 2
	la          t3, valid_types
	add         t2, t3, t2
	lw          t2, 0(t2)
	bne         t0, t2, .IsValidType_label_30

	// *** Basic block 2

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 3

.IsValidType_label_27:
	ret         

	// *** Basic block 4

.IsValidType_label_30:

	// *** Basic block 5

.IsValidType_label_31:
	addi        t1, t1, 1
	li          t0, 34		// 0x22 ASCII '"'
	bge         t1, t0, .IsValidType_label_16

	// *** Basic block 6

.IsValidType_label_37:
	mv          a0, x0
	ret         
.func_end_IsValidType:
	.size IsValidType, .func_end_IsValidType-IsValidType

	.local  IsValidQualiferCombo
	.type IsValidQualiferCombo, @function

IsValidQualiferCombo:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	and         t0, a0, a1
	seqz        a0, t0

	// *** Basic block 1

.IsValidQualiferCombo_label_12:
	ret         
.func_end_IsValidQualiferCombo:
	.size IsValidQualiferCombo, .func_end_IsValidQualiferCombo-IsValidQualiferCombo

	.local  TypeComboError1
	.type TypeComboError1, @function

TypeComboError1:

	// *** Basic block 0

	.global StringInit
	.local TypeToString
	.global StringAppend
	.global SyntaxError
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
	mv          s1, a1
	mv          s2, a2
	mv          s3, a0
	addi        a0, s0, -64
	lla         a1, .str.43
	call        StringInit

	// *** Basic block 1

	addi        a1, s0, -64
	mv          a0, s1
	call        TypeToString

	// *** Basic block 2

	beqz        s2, .TypeComboError1_label_47

	// *** Basic block 3

	addi        a0, s0, -64
	lla         a1, .str.44
	call        StringAppend

	// *** Basic block 4

	addi        a1, s0, -64
	mv          a0, s2
	call        TypeToString

	// *** Basic block 5

.TypeComboError1_label_47:
	lla         a1, .str.45
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s3
	call        SyntaxError

	// *** Basic block 6

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 7

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeComboError1:
	.size TypeComboError1, .func_end_TypeComboError1-TypeComboError1

	.local  TypeComboError2
	.type TypeComboError2, @function

TypeComboError2:

	// *** Basic block 0

	.global StringInit
	.global TypeRecordToString
	.global StringAppend
	.local TypeToString
	.global SyntaxError
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
	mv          s1, a1
	mv          s2, a2
	mv          s3, a0
	addi        a0, s0, -64
	lla         a1, .str.46
	call        StringInit

	// *** Basic block 1

	addi        a1, s0, -64
	mv          a0, s1
	call        TypeRecordToString

	// *** Basic block 2

	addi        a0, s0, -64
	lla         a1, .str.47
	call        StringAppend

	// *** Basic block 3

	addi        a1, s0, -64
	mv          a0, s2
	call        TypeToString

	// *** Basic block 4

	lla         a1, .str.48
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s3
	call        SyntaxError

	// *** Basic block 5

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 6

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeComboError2:
	.size TypeComboError2, .func_end_TypeComboError2-TypeComboError2

	.local  TypeComboError3
	.type TypeComboError3, @function

TypeComboError3:

	// *** Basic block 0

	.global StringInit
	.global TypeRecordToString
	.global StringAppend
	.global SyntaxError
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
	mv          s1, a1
	mv          s2, a2
	mv          s3, a0
	addi        a0, s0, -64
	lla         a1, .str.49
	call        StringInit

	// *** Basic block 1

	addi        a1, s0, -64
	mv          a0, s1
	call        TypeRecordToString

	// *** Basic block 2

	addi        a0, s0, -64
	lla         a1, .str.50
	call        StringAppend

	// *** Basic block 3

	addi        a1, s0, -64
	mv          a0, s2
	call        TypeRecordToString

	// *** Basic block 4

	lla         a1, .str.51
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s3
	call        SyntaxError

	// *** Basic block 5

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 6

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeComboError3:
	.size TypeComboError3, .func_end_TypeComboError3-TypeComboError3

	.local  QualifierComboError
	.type QualifierComboError, @function

QualifierComboError:

	// *** Basic block 0

	.global StringInit
	.local QualifiersToString
	.global StringAppend
	.global SyntaxError
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
	mv          s1, a1
	mv          s2, a2
	mv          s3, a0
	addi        a0, s0, -64
	lla         a1, .str.52
	call        StringInit

	// *** Basic block 1

	addi        a1, s0, -64
	mv          a0, s1
	call        QualifiersToString

	// *** Basic block 2

	addi        a0, s0, -64
	lla         a1, .str.53
	call        StringAppend

	// *** Basic block 3

	addi        a1, s0, -64
	mv          a0, s2
	call        QualifiersToString

	// *** Basic block 4

	lla         a1, .str.54
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s3
	call        SyntaxError

	// *** Basic block 5

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 6

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_QualifierComboError:
	.size QualifierComboError, .func_end_QualifierComboError-QualifierComboError

	.local  CombineTypeSpecifiers
	.type CombineTypeSpecifiers, @function

CombineTypeSpecifiers:

	// *** Basic block 0

	.local IsValidType
	.local TypeComboError1
	.local IsValidQualiferCombo
	.local QualifierComboError
	.local TypeComboError3
	.local TypeComboError2
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
	mv          s2, a2
	mv          s3, a3
	mv          s4, a1
	sd          x0, 0(s1)
	sd          x0, 8(s1)
	sd          x0, 16(s1)
	sw          x0, 0(s1)
	lw          t1, 0(s2)
	andi        t1, t1, 8
	snez        t0, t1
	beqz        t1, .CombineTypeSpecifiers_label_44

	// *** Basic block 1

	lw          t1, 0(s3)
	andi        t1, t1, 8
	snez        t0, t1

	// *** Basic block 2

.CombineTypeSpecifiers_label_44:
	beqz        t0, .CombineTypeSpecifiers_label_53

	// *** Basic block 3

	lw          t0, 0(s2)
	andi        t0, t0, -9
	sw          t0, 0(s2)
	lw          t0, 0(s2)
	ori         t0, t0, 16
	sw          t0, 0(s2)
	sw          x0, 0(s3)

	// *** Basic block 4

.CombineTypeSpecifiers_label_53:
	lw          s5, 0(s2)
	lw          s6, 0(s3)
	or          t0, s5, s6
	sw          t0, 0(s1)
	lw          s8, 0(s1)
	seqz        s7, s8
	beqz        s8, .CombineTypeSpecifiers_label_64

	// *** Basic block 5

	and         t0, s5, s6
	seqz        s7, t0

	// *** Basic block 6

.CombineTypeSpecifiers_label_64:
	beqz        s7, .CombineTypeSpecifiers_label_71

	// *** Basic block 7

	mv          a0, s8
	call        IsValidType

	// *** Basic block 8

	mv          s7, a0

	// *** Basic block 9

.CombineTypeSpecifiers_label_71:
	not         t0, s7
	beqz        t0, .CombineTypeSpecifiers_label_84

	// *** Basic block 10

	mv          a2, s6
	mv          a1, s5
	mv          a0, s4
	call        TypeComboError1

	// *** Basic block 11

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 16(s1)

	// *** Basic block 12

.CombineTypeSpecifiers_label_84:
	lw          t0, 0(s1)
	andi        t0, t0, 72
	li          t1, 72		// 0x48 ASCII 'H'
	bne         t0, t1, .CombineTypeSpecifiers_label_98

	// *** Basic block 13

	lw          t0, 0(s1)
	andi        t0, t0, -73
	sw          t0, 0(s1)
	lw          t0, 0(s1)
	ori         t0, t0, 128
	sw          t0, 0(s1)

	// *** Basic block 14

.CombineTypeSpecifiers_label_98:
	lw          s5, 4(s2)
	lw          s6, 4(s3)
	mv          a1, s6
	mv          a0, s5
	call        IsValidQualiferCombo

	// *** Basic block 15

	not         t0, a0
	beqz        t0, .CombineTypeSpecifiers_label_120

	// *** Basic block 16

	mv          a2, s6
	mv          a1, s5
	mv          a0, s4
	call        QualifierComboError

	// *** Basic block 17

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 16(s1)

	// *** Basic block 18

.CombineTypeSpecifiers_label_120:
	or          t0, s5, s6
	sw          t0, 4(s1)
	sd          x0, 8(s1)
	lb          t0, 16(s1)
	beqz        t0, .CombineTypeSpecifiers_label_132

	// *** Basic block 19

.CombineTypeSpecifiers_label_129:
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

	// *** Basic block 20

.CombineTypeSpecifiers_label_132:
	ld          t1, 8(s2)
	sub         t2, t1, x0
	snez        t0, t2
	bne         t1, x0, .CombineTypeSpecifiers_label_143

	// *** Basic block 21

	ld          t2, 8(s3)
	sub         t2, t2, x0
	snez        t0, t2

	// *** Basic block 22

.CombineTypeSpecifiers_label_143:
	beqz        t0, .CombineTypeSpecifiers_label_215

	// *** Basic block 23

	bne         t1, x0, .CombineTypeSpecifiers_label_152

	// *** Basic block 24

	mv          s5, s2
	mv          s2, s3
	mv          s3, s5

	// *** Basic block 25

.CombineTypeSpecifiers_label_152:
	ld          s6, 8(s3)
	beq         s6, x0, .CombineTypeSpecifiers_label_170

	// *** Basic block 26

	ld          a1, 8(s2)
	mv          a2, s6
	mv          a0, s4
	call        TypeComboError3

	// *** Basic block 27

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 16(s1)
	j           .CombineTypeSpecifiers_label_186

	// *** Basic block 28

.CombineTypeSpecifiers_label_170:
	lw          s6, 0(s3)
	beqz        s6, .CombineTypeSpecifiers_label_185

	// *** Basic block 29

	ld          a1, 8(s2)
	mv          a2, s6
	mv          a0, s4
	call        TypeComboError2

	// *** Basic block 30

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 16(s1)

	// *** Basic block 31

.CombineTypeSpecifiers_label_185:

	// *** Basic block 32

.CombineTypeSpecifiers_label_186:
	lw          t0, 0(s2)
	sw          t0, 0(s1)
	lw          s6, 4(s2)
	lw          s8, 4(s3)
	mv          a1, s8
	mv          a0, s6
	call        IsValidQualiferCombo

	// *** Basic block 33

	not         t0, a0
	beqz        t0, .CombineTypeSpecifiers_label_207

	// *** Basic block 34

	mv          a2, s8
	mv          a1, s6
	mv          a0, s4
	call        QualifierComboError

	// *** Basic block 35

.CombineTypeSpecifiers_label_207:
	or          t0, s6, s8
	sw          t0, 4(s1)
	ld          t0, 8(s2)
	sd          t0, 8(s1)

	// *** Basic block 36

.CombineTypeSpecifiers_label_215:
	j           .CombineTypeSpecifiers_label_129
.func_end_CombineTypeSpecifiers:
	.size CombineTypeSpecifiers, .func_end_CombineTypeSpecifiers-CombineTypeSpecifiers

	.global TypeParserParseAndCombineTypes
	.type TypeParserParseAndCombineTypes, @function

TypeParserParseAndCombineTypes:

	// *** Basic block 0

	.local ParseTypeSpecifier
	.local CombineTypeSpecifiers
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
	mv          s3, a1
	mv          s4, a2
	mv          s2, a0
	addi        t0, s0, -48
	mv          a0, t0
	call        ParseTypeSpecifier

	// *** Basic block 1

	ld          t0, -48(s0)
	sd          t0, 0(s1)
	ld          t0, -40(s0)
	sd          t0, 8(s1)
	ld          t0, -32(s0)
	sd          t0, 16(s1)
	lw          t1, 0(s4)
	seqz        t0, t1
	bnez        t1, .TypeParserParseAndCombineTypes_label_42

	// *** Basic block 2

	lw          t1, 4(s4)
	seqz        t0, t1

	// *** Basic block 3

.TypeParserParseAndCombineTypes_label_42:
	beqz        t0, .TypeParserParseAndCombineTypes_label_47

	// *** Basic block 4

.TypeParserParseAndCombineTypes_label_44:
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

.TypeParserParseAndCombineTypes_label_47:
	ld          a1, 8(s3)
	mv          a3, s1
	mv          a2, s4
	mv          a0, s2
	call        CombineTypeSpecifiers

	// *** Basic block 6

	j           .TypeParserParseAndCombineTypes_label_44
.func_end_TypeParserParseAndCombineTypes:
	.size TypeParserParseAndCombineTypes, .func_end_TypeParserParseAndCombineTypes-TypeParserParseAndCombineTypes

	.global TypeParserBuildTypeRecord
	.type TypeParserBuildTypeRecord, @function

TypeParserBuildTypeRecord:

	// *** Basic block 0

	.global NewTypeRecord
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	lb          t1, 16(t0)
	beqz        t1, .TypeParserBuildTypeRecord_label_29

	// *** Basic block 1

	mv          a1, x0
	li          t1, 2		// 0x2 ASCII \x2
	mv          a0, t1
	j           NewTypeRecord

	// *** Basic block 3

.TypeParserBuildTypeRecord_label_26:
	ret         

	// *** Basic block 4

.TypeParserBuildTypeRecord_label_29:
	ld          t1, 8(t0)
	bne         t1, x0, .TypeParserBuildTypeRecord_label_52

	// *** Basic block 5

	lw          t1, 0(t0)
	bnez        t1, .TypeParserBuildTypeRecord_label_41

	// *** Basic block 6

	mv          a0, x0
	j           .TypeParserBuildTypeRecord_label_26

	// *** Basic block 7

.TypeParserBuildTypeRecord_label_41:
	lw          a1, 4(t0)
	mv          a0, t1
	j           NewTypeRecord

	// *** Basic block 9

.TypeParserBuildTypeRecord_label_52:
	lw          t1, 4(t0)
	ld          a0, 8(t0)
	lw          t2, 12(a0)
	or          t1, t2, t1
	sw          t1, 12(a0)
	ret         
.func_end_TypeParserBuildTypeRecord:
	.size TypeParserBuildTypeRecord, .func_end_TypeParserBuildTypeRecord-TypeParserBuildTypeRecord

	.global TypeParserParseType
	.type TypeParserParseType, @function

TypeParserParseType:

	// *** Basic block 0

	.global SyntaxLookingAtType
	.local ParseTypeSpecifier
	.local CombineTypeSpecifiers
	.global SyntaxError
	.global SyntaxRecover
	.global NewTypeRecord
	.global TypeParserBuildTypeRecord
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -96(s0)
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
	ld          s3, 8(s1)
	sd          x0, -96(s0)
	sd          x0, -88(s0)
	sd          x0, -80(s0)
	sw          x0, -96(s0)
	addi        t0, s0, -96
	sw          x0, 4(t0)
	addi        t0, s0, -96
	sd          x0, 8(t0)
	addi        t0, s0, -96
	sb          x0, 16(t0)
	lb          s4, 60(s1)
	bnez        s4, .TypeParserParseType_label_56

	// *** Basic block 1

	addi        s5, s0, -48
	addi        s6, s0, -96
	mv          a0, s3
	call        SyntaxLookingAtType

	// *** Basic block 2

	mv          s4, a0

	// *** Basic block 3

.TypeParserParseType_label_56:
	beqz        s4, .TypeParserParseType_label_113

	// *** Basic block 4

.TypeParserParseType_label_58:
	mv          a1, s1
	mv          a0, s5
	call        ParseTypeSpecifier

	// *** Basic block 5

	ld          t0, -48(s0)
	sd          t0, -72(s0)
	ld          t0, -40(s0)
	sd          t0, -64(s0)
	ld          t0, -32(s0)
	sd          t0, -56(s0)
	lw          t1, -96(s0)
	seqz        t0, t1
	bnez        t1, .TypeParserParseType_label_81

	// *** Basic block 6

	addi        t1, s0, -96
	lw          t1, 4(t1)
	seqz        t0, t1

	// *** Basic block 7

.TypeParserParseType_label_81:
	beqz        t0, .TypeParserParseType_label_91

	// *** Basic block 8

	ld          t0, -72(s0)
	sd          t0, -96(s0)
	ld          t0, -64(s0)
	sd          t0, -88(s0)
	ld          t0, -56(s0)
	sd          t0, -80(s0)
	j           .TypeParserParseType_label_103

	// *** Basic block 9

.TypeParserParseType_label_91:
	addi        a2, s0, -96
	addi        a3, s0, -72
	mv          a1, s3
	mv          a0, s6
	call        CombineTypeSpecifiers

	// *** Basic block 10

.TypeParserParseType_label_103:
	mv          s5, s4
	bnez        s4, .TypeParserParseType_label_111

	// *** Basic block 11

	mv          a0, s3
	call        SyntaxLookingAtType

	// *** Basic block 12

	mv          s5, a0

	// *** Basic block 13

.TypeParserParseType_label_111:
	bnez        s5, .TypeParserParseType_label_58

	// *** Basic block 14

.TypeParserParseType_label_113:
	lw          t0, -96(s0)
	bnez        t0, .TypeParserParseType_label_145

	// *** Basic block 15

	beqz        s2, .TypeParserParseType_label_141

	// *** Basic block 16

	lla         a1, .str.55
	mv          a0, s3
	call        SyntaxError

	// *** Basic block 17

	li          t0, 258		// 0x102
	mv          a1, t0
	mv          a0, s3
	call        SyntaxRecover

	// *** Basic block 18

	mv          a1, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 19


	// *** Basic block 20

.TypeParserParseType_label_138:
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

	// *** Basic block 21

.TypeParserParseType_label_141:
	mv          a0, x0
	j           .TypeParserParseType_label_138

	// *** Basic block 22

.TypeParserParseType_label_145:
	addi        a1, s0, -96
	mv          a0, s1
	call        TypeParserBuildTypeRecord

	// *** Basic block 23

	j           .TypeParserParseType_label_138
.func_end_TypeParserParseType:
	.size TypeParserParseType, .func_end_TypeParserParseType-TypeParserParseType

	.global TypeParserParseDeclarator
	.type TypeParserParseDeclarator, @function

TypeParserParseDeclarator:

	// *** Basic block 0

	.global VectorClear
	.global TypeParserParsePointer
	.global TypeRecordChain
	.global TypeRecordCalculateSize
	.global SymbolSetType
	.global NewSymbol
	.global SyntaxFakeName
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
	bne         s1, x0, .TypeParserParseDeclarator_label_34

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.TypeParserParseDeclarator_label_31:
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

.TypeParserParseDeclarator_label_34:
	addi        a0, s2, 24
	call        VectorClear

	// *** Basic block 4

	sd          x0, 16(s2)
	sd          s1, 48(s2)
	mv          a0, s2
	call        TypeParserParsePointer

	// *** Basic block 5

	addi        t0, s2, 24
	ld          s3, 8(t0)
	ld          s4, 48(s2)
	bge         x0, s3, .TypeParserParseDeclarator_label_74

	// *** Basic block 6

	ld          s1, 24(s2)

	// *** Basic block 7

.TypeParserParseDeclarator_label_54:
	addi        s3, s3, -1
	slli        t0, s3, 3
	add         t0, s1, t0
	ld          s1, 0(t0)
	mv          a1, s4
	mv          a0, s1
	call        TypeRecordChain

	// *** Basic block 8

	lw          t0, 8(s4)
	sw          t0, 8(s1)
	mv          s4, s1
	blt         x0, s3, .TypeParserParseDeclarator_label_54

	// *** Basic block 9

.TypeParserParseDeclarator_label_74:
	mv          a0, s4
	call        TypeRecordCalculateSize

	// *** Basic block 10

	ld          s1, 16(s2)
	beq         s1, x0, .TypeParserParseDeclarator_label_89

	// *** Basic block 11

	mv          a1, s4
	mv          a0, s1
	call        SymbolSetType

	// *** Basic block 12

	j           .TypeParserParseDeclarator_label_113

	// *** Basic block 13

.TypeParserParseDeclarator_label_89:
	ld          a0, 8(s2)
	call        SyntaxFakeName

	// *** Basic block 14

	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a1, s4
	call        NewSymbol

	// *** Basic block 15

	sd          a0, 16(s2)
	ld          t0, 16(s2)
	addi        t0, t0, 56
	lb          t1, 1(t0)
	andi        t1, t1, -2
	ori         t1, t1, 1
	sb          t1, 1(t0)

	// *** Basic block 16

.TypeParserParseDeclarator_label_113:
	ld          a0, 16(s2)
	j           .TypeParserParseDeclarator_label_31
.func_end_TypeParserParseDeclarator:
	.size TypeParserParseDeclarator, .func_end_TypeParserParseDeclarator-TypeParserParseDeclarator

	.local  ParseQualifiers
	.type ParseQualifiers, @function

ParseQualifiers:

	// *** Basic block 0

	.global LexEof
	.global LexMatch
	.global SyntaxError
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
	mv          s3, x0
	mv          s4, x0
	mv          s5, x0
	ld          s6, 0(s1)
	mv          a0, s6
	call        LexEof

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .ParseQualifiers_label_76

	// *** Basic block 2

.ParseQualifiers_label_32:
	li          t0, 61		// 0x3d ASCII '='
	mv          a1, t0
	mv          a0, s6
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .ParseQualifiers_label_44

	// *** Basic block 4

	ori         s2, s2, 4
	addi        s3, s3, 1
	j           .ParseQualifiers_label_70

	// *** Basic block 5

.ParseQualifiers_label_44:
	li          t0, 92		// 0x5c ASCII '\'
	mv          a1, t0
	mv          a0, s6
	call        LexMatch

	// *** Basic block 6

	beqz        a0, .ParseQualifiers_label_55

	// *** Basic block 7

	addi        s4, s4, 1
	ori         s2, s2, 8
	j           .ParseQualifiers_label_69

	// *** Basic block 8

.ParseQualifiers_label_55:
	li          t0, 79		// 0x4f ASCII 'O'
	mv          a1, t0
	mv          a0, s6
	call        LexMatch

	// *** Basic block 9

	beqz        a0, .ParseQualifiers_label_66

	// *** Basic block 10

	addi        s5, s5, 1
	ori         s2, s2, 16
	j           .ParseQualifiers_label_68

	// *** Basic block 11

.ParseQualifiers_label_66:
	j           .ParseQualifiers_label_76

	// *** Basic block 12

.ParseQualifiers_label_68:

	// *** Basic block 13

.ParseQualifiers_label_69:

	// *** Basic block 14

.ParseQualifiers_label_70:
	mv          a0, s6
	call        LexEof

	// *** Basic block 15

	not         t0, a0
	bnez        t0, .ParseQualifiers_label_32

	// *** Basic block 16

.ParseQualifiers_label_76:
	li          s6, 1		// 0x1 ASCII \x1
	slt         t0, s6, s3
	blt         s6, s3, .ParseQualifiers_label_85

	// *** Basic block 17

	slt         t0, s6, s4

	// *** Basic block 18

.ParseQualifiers_label_85:
	bnez        t0, .ParseQualifiers_label_89

	// *** Basic block 19

	slt         t0, s6, s5

	// *** Basic block 20

.ParseQualifiers_label_89:
	beqz        t0, .ParseQualifiers_label_98

	// *** Basic block 21

	ld          a0, 8(s1)
	lla         a1, .str.56
	call        SyntaxError

	// *** Basic block 22

.ParseQualifiers_label_98:
	mv          a0, s2

	// *** Basic block 23

.ParseQualifiers_label_101:
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
.func_end_ParseQualifiers:
	.size ParseQualifiers, .func_end_ParseQualifiers-ParseQualifiers

	.global TypeParserParsePointer
	.type TypeParserParsePointer, @function

TypeParserParsePointer:

	// *** Basic block 0

	.global LexMatch
	.local ParseQualifiers
	.global TypeParserParsePointer
	.global NewPointerTypeRecord
	.global VectorAppend
	.global TypeParserParseFuncOrArray
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
	ld          a0, 0(s1)
	li          a1, 52		// 0x34 ASCII '4'
	call        LexMatch

	// *** Basic block 1

	beqz        a0, .TypeParserParsePointer_label_41

	// *** Basic block 2

	mv          a0, s1
	call        ParseQualifiers

	// *** Basic block 3

	mv          s2, a0
	mv          a0, s1
	call        TypeParserParsePointer

	// *** Basic block 4

	mv          a0, s2
	call        NewPointerTypeRecord

	// *** Basic block 5

	mv          s3, a0
	addi        a0, s1, 24
	mv          a1, s3
	call        VectorAppend

	// *** Basic block 6

	j           .TypeParserParsePointer_label_46

	// *** Basic block 7

.TypeParserParsePointer_label_41:
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           TypeParserParseFuncOrArray

	// *** Basic block 8

.TypeParserParsePointer_label_46:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeParserParsePointer:
	.size TypeParserParsePointer, .func_end_TypeParserParsePointer-TypeParserParsePointer

	.local  CheckFormalName
	.type CheckFormalName, @function

CheckFormalName:

	// *** Basic block 0

	.global StringEqualString
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
	mv          t0, a0
	ld          t1, 24(s1)
	bnez        t1, .CheckFormalName_label_25

	// *** Basic block 1

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 2

.CheckFormalName_label_22:
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

.CheckFormalName_label_25:
	mv          s2, x0
	ld          s3, 8(t0)
	bge         x0, s3, .CheckFormalName_label_53

	// *** Basic block 4

	ld          s4, 0(t0)

	// *** Basic block 5

.CheckFormalName_label_33:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          s4, 0(t0)
	mv          a1, s1
	mv          a0, s4
	call        StringEqualString

	// *** Basic block 6

	beqz        a0, .CheckFormalName_label_48

	// *** Basic block 7

	mv          a0, x0
	j           .CheckFormalName_label_22

	// *** Basic block 8

.CheckFormalName_label_48:

	// *** Basic block 9

.CheckFormalName_label_49:
	addi        s2, s2, 1
	bge         s2, s3, .CheckFormalName_label_33

	// *** Basic block 10

.CheckFormalName_label_53:
	li          a0, 1		// 0x1 ASCII \x1
	j           .CheckFormalName_label_22
.func_end_CheckFormalName:
	.size CheckFormalName, .func_end_CheckFormalName-CheckFormalName

	.local  ParseFormalArgument
	.type ParseFormalArgument, @function

ParseFormalArgument:

	// *** Basic block 0

	.local CheckFormalName
	.global NewPointerTypeRecord
	.global TypeRecordChain
	.global SymbolSetType
	.global TypeRecordCopy
	.global VectorAppend
	.global InsertLocalSymbol
	.global SyntaxError
	.global SymbolDelete
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
	mv          s2, a2
	mv          s3, a3
	mv          s4, a0
	addi        t0, s1, 32
	addi        a0, t0, 8
	mv          a1, s2
	call        CheckFormalName

	// *** Basic block 1

	beqz        a0, .ParseFormalArgument_label_132

	// *** Basic block 2

	ld          s5, 40(s2)
	lw          t0, 16(s5)
	addi        t0, t0, -3
	seqz        s5, t0

	// *** Basic block 3

.ParseFormalArgument_label_53:
	beqz        s5, .ParseFormalArgument_label_73

	// *** Basic block 4

	j           .ParseFormalArgument_label_56

	// *** Basic block 5

.ParseFormalArgument_label_56:
	mv          a0, x0
	call        NewPointerTypeRecord

	// *** Basic block 6

	mv          s6, a0
	mv          a1, s5
	mv          a0, s6
	call        TypeRecordChain

	// *** Basic block 7

	mv          a1, s6
	mv          a0, s2
	call        SymbolSetType

	// *** Basic block 8

	j           .ParseFormalArgument_label_99

	// *** Basic block 9

.ParseFormalArgument_label_73:
	mv          s7, s5
	lw          t0, 16(s7)
	addi        t0, t0, -2
	seqz        s8, t0

	// *** Basic block 10

.ParseFormalArgument_label_81:
	beqz        s8, .ParseFormalArgument_label_98

	// *** Basic block 11

	j           .ParseFormalArgument_label_84

	// *** Basic block 12

.ParseFormalArgument_label_84:
	mv          a0, s5
	call        TypeRecordCopy

	// *** Basic block 13

	mv          s5, a0
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 16(s5)
	mv          a1, s5
	mv          a0, s2
	call        SymbolSetType

	// *** Basic block 14

.ParseFormalArgument_label_98:

	// *** Basic block 15

.ParseFormalArgument_label_99:
	addi        t0, s1, 32
	addi        a0, t0, 8
	mv          a1, s2
	call        VectorAppend

	// *** Basic block 16

	lb          t0, 56(s2)
	andi        t0, t0, -2
	ori         t0, t0, 1
	sb          t0, 56(s2)
	lb          t0, 56(s2)
	andi        t0, t0, -17
	ori         t0, t0, 16
	sb          t0, 56(s2)
	sw          s3, 112(s2)
	ld          t0, 8(s4)
	ld          s8, 16(t0)
	beq         s8, x0, .ParseFormalArgument_label_130

	// *** Basic block 17

	mv          a1, s2
	mv          a0, s8
	call        InsertLocalSymbol

	// *** Basic block 18

.ParseFormalArgument_label_130:
	j           .ParseFormalArgument_label_147

	// *** Basic block 19

.ParseFormalArgument_label_132:
	ld          a0, 8(s4)
	lla         a1, .str.57
	ld          a2, 16(s2)
	call        SyntaxError

	// *** Basic block 20

	mv          a0, s2
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
	j           SymbolDelete

	// *** Basic block 21

.ParseFormalArgument_label_147:
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
.func_end_ParseFormalArgument:
	.size ParseFormalArgument, .func_end_ParseFormalArgument-ParseFormalArgument

	.local  ParseFunctionParameter
	.type ParseFunctionParameter, @function

ParseFunctionParameter:

	// *** Basic block 0

	.global SyntaxLookingAtType
	.global TypeParserParseType
	.global SyntaxError
	.global TypeParserParseDeclarator
	.global printf
	.global abort
	.local ParseFormalArgument
	.global LexLookingAt
	.global SyntaxRecover
	.global NewTypeRecord
	.global NewSymbol
	.global LexNextToken
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
	mv          s1, a0
	mv          s2, a2
	mv          s3, a1
	mv          s4, a3
	lb          s5, 60(s1)
	bnez        s5, .ParseFunctionParameter_label_50

	// *** Basic block 1

	ld          a0, 8(s1)
	call        SyntaxLookingAtType

	// *** Basic block 2

	mv          s5, a0

	// *** Basic block 3

.ParseFunctionParameter_label_50:
	beqz        s5, .ParseFunctionParameter_label_114

	// *** Basic block 4

	li          s6, 1		// 0x1 ASCII \x1
	mv          a1, s6
	mv          a0, s1
	call        TypeParserParseType

	// *** Basic block 5

	mv          s7, a0
	bnez        s2, .ParseFunctionParameter_label_63

	// *** Basic block 6

	li          s2, 2		// 0x2 ASCII \x2

	// *** Basic block 7

.ParseFunctionParameter_label_63:
	bne         s2, s6, .ParseFunctionParameter_label_76

	// *** Basic block 8

	ld          a0, 8(s1)
	lla         a1, .str.58
	call        SyntaxError

	// *** Basic block 9

.ParseFunctionParameter_label_76:
	mv          a1, s7
	mv          a0, s1
	call        TypeParserParseDeclarator

	// *** Basic block 10

	mv          s6, a0
	beq         s6, x0, .ParseFunctionParameter_label_88

	// *** Basic block 11

	j           .ParseFunctionParameter_label_103

	// *** Basic block 12

.ParseFunctionParameter_label_88:
	lla         a0, .str.59
	lla         a1, .str.60
	lla         a3, .str.61
	li          t0, 1008		// 0x3f0
	mv          a2, t0
	call        printf

	// *** Basic block 13

	call        abort

	// *** Basic block 14

.ParseFunctionParameter_label_103:
	mv          a3, s4
	mv          a2, s6
	mv          a1, s3
	mv          a0, s1
	call        ParseFormalArgument

	// *** Basic block 15

	j           .ParseFunctionParameter_label_198

	// *** Basic block 16

.ParseFunctionParameter_label_114:
	ld          s8, 0(s1)
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s8
	call        LexLookingAt

	// *** Basic block 17

	beqz        a0, .ParseFunctionParameter_label_182

	// *** Basic block 18

	bnez        s2, .ParseFunctionParameter_label_126

	// *** Basic block 19

	li          s2, 1		// 0x1 ASCII \x1

	// *** Basic block 20

.ParseFunctionParameter_label_126:
	li          s9, 2		// 0x2 ASCII \x2
	bne         s2, s9, .ParseFunctionParameter_label_147

	// *** Basic block 21

	ld          s10, 8(s1)
	lla         a1, .str.62
	mv          a0, s10
	call        SyntaxError

	// *** Basic block 22

	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	mv          a0, s10
	call        SyntaxRecover

	// *** Basic block 23

	j           .ParseFunctionParameter_label_180

	// *** Basic block 24

.ParseFunctionParameter_label_147:
	mv          a1, x0
	mv          a0, s9
	call        NewTypeRecord

	// *** Basic block 25

	mv          s9, a0
	addi        t0, s8, 80
	ld          a0, 16(t0)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a1, s9
	call        NewSymbol

	// *** Basic block 26

	mv          s10, a0
	mv          a0, s8
	call        LexNextToken

	// *** Basic block 27

	mv          a3, s4
	mv          a2, s10
	mv          a1, s3
	mv          a0, s1
	call        ParseFormalArgument

	// *** Basic block 28

.ParseFunctionParameter_label_180:
	j           .ParseFunctionParameter_label_197

	// *** Basic block 29

.ParseFunctionParameter_label_182:
	ld          s8, 8(s1)
	lla         a1, .str.63
	mv          a0, s8
	call        SyntaxError

	// *** Basic block 30

	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	mv          a0, s8
	call        SyntaxRecover

	// *** Basic block 31

.ParseFunctionParameter_label_197:

	// *** Basic block 32

.ParseFunctionParameter_label_198:
	mv          a0, s2

	// *** Basic block 33

.ParseFunctionParameter_label_201:
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
.func_end_ParseFunctionParameter:
	.size ParseFunctionParameter, .func_end_ParseFunctionParameter-ParseFunctionParameter

	.local  ParseFunctionPrototype
	.type ParseFunctionPrototype, @function

ParseFunctionPrototype:

	// *** Basic block 0

	.global LexLookingAt
	.global LexMatch
	.global SyntaxError
	.global SyntaxRecover
	.local ParseFunctionParameter
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
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	mv          s3, x0
	addi        s4, s1, 32
	mv          s5, x0
	mv          s6, x0
	sb          x0, 50(s4)
	ld          a0, 0(s2)
	li          s7, 45		// 0x2d ASCII '-'
	mv          a1, s7
	call        LexLookingAt

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .ParseFunctionPrototype_label_150

	// *** Basic block 2

.ParseFunctionPrototype_label_47:
	ld          s8, 0(s2)
	li          t0, 20		// 0x14 ASCII \x14
	mv          a1, t0
	mv          a0, s8
	call        LexMatch

	// *** Basic block 3

	beqz        a0, .ParseFunctionPrototype_label_83

	// *** Basic block 4

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 32(s4)
	mv          a1, s7
	mv          a0, s8
	call        LexLookingAt

	// *** Basic block 5

	not         t0, a0
	beqz        t0, .ParseFunctionPrototype_label_81

	// *** Basic block 6

	ld          s9, 8(s2)
	lla         a1, .str.64
	mv          a0, s9
	call        SyntaxError

	// *** Basic block 7

	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	mv          a0, s9
	call        SyntaxRecover

	// *** Basic block 8

.ParseFunctionPrototype_label_81:
	j           .ParseFunctionPrototype_label_150

	// *** Basic block 9

.ParseFunctionPrototype_label_83:
	seqz        s9, s6
	bnez        s6, .ParseFunctionPrototype_label_94

	// *** Basic block 10

	li          t0, 91		// 0x5b ASCII '['
	mv          a1, t0
	mv          a0, s8
	call        LexMatch

	// *** Basic block 11

	mv          s9, a0

	// *** Basic block 12

.ParseFunctionPrototype_label_94:
	beqz        s9, .ParseFunctionPrototype_label_109

	// *** Basic block 13

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 60(s2)
	ld          a0, 0(s2)
	mv          a1, s7
	call        LexLookingAt

	// *** Basic block 14

	beqz        a0, .ParseFunctionPrototype_label_108

	// *** Basic block 15

	li          s3, 1		// 0x1 ASCII \x1
	j           .ParseFunctionPrototype_label_150

	// *** Basic block 16

.ParseFunctionPrototype_label_108:

	// *** Basic block 17

.ParseFunctionPrototype_label_109:
	ld          s8, 0(s2)
	li          t0, 78		// 0x4e ASCII 'N'
	mv          a1, t0
	mv          a0, s8
	call        LexMatch

	// *** Basic block 18

	beqz        a0, .ParseFunctionPrototype_label_119

	// *** Basic block 19

	li          s5, 2		// 0x2 ASCII \x2

	// *** Basic block 20

.ParseFunctionPrototype_label_119:
	mv          a3, s6
	mv          a2, s5
	mv          a1, s1
	mv          a0, s2
	call        ParseFunctionParameter

	// *** Basic block 21

	mv          s5, a0
	addi        s6, s6, 1
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	mv          a0, s8
	call        LexMatch

	// *** Basic block 22

	not         t0, a0
	bnez        t0, .ParseFunctionPrototype_label_150

	// *** Basic block 23

.ParseFunctionPrototype_label_141:
	mv          a1, s7
	mv          a0, s8
	call        LexLookingAt

	// *** Basic block 24

	not         t0, a0
	bnez        t0, .ParseFunctionPrototype_label_47

	// *** Basic block 25

.ParseFunctionPrototype_label_150:
	li          t0, 1		// 0x1 ASCII \x1
	bne         s5, t0, .ParseFunctionPrototype_label_159

	// *** Basic block 26

	sb          t0, 50(s4)

	// *** Basic block 27

.ParseFunctionPrototype_label_159:
	addi        t2, s4, 8
	ld          t2, 8(t2)
	seqz        t1, t2
	bnez        t2, .ParseFunctionPrototype_label_167

	// *** Basic block 28

	not         t1, s3

	// *** Basic block 29

.ParseFunctionPrototype_label_167:
	beqz        t1, .ParseFunctionPrototype_label_172

	// *** Basic block 30

	sb          t0, 48(s4)

	// *** Basic block 31

.ParseFunctionPrototype_label_172:
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
.func_end_ParseFunctionPrototype:
	.size ParseFunctionPrototype, .func_end_ParseFunctionPrototype-ParseFunctionPrototype

	.local  ParseFunctionDecl
	.type ParseFunctionDecl, @function

ParseFunctionDecl:

	// *** Basic block 0

	.global TypeParserInit
	.global NewFunctionTypeRecord
	.local ParseFunctionPrototype
	.global SyntaxNeedBracket
	.global VectorAppend
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -96(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	addi        a0, s0, -96
	ld          a1, 0(s1)
	ld          s2, 8(s1)
	li          a4, 2		// 0x2 ASCII \x2
	li          a3, 1		// 0x1 ASCII \x1
	mv          a2, s2
	call        TypeParserInit

	// *** Basic block 1

	call        NewFunctionTypeRecord

	// *** Basic block 2

	mv          s3, a0
	addi        t0, s3, 32
	lb          t1, 68(s1)
	sb          t1, 51(t0)
	ld          s4, 16(s1)
	beq         s4, x0, .ParseFunctionDecl_label_56

	// *** Basic block 3

	sd          s4, 32(s3)

	// *** Basic block 4

.ParseFunctionDecl_label_56:
	addi        a0, s0, -96
	mv          a1, s3
	call        ParseFunctionPrototype

	// *** Basic block 5

	li          t0, 64		// 0x40 ASCII '@'
	mv          a2, t0
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s2
	call        SyntaxNeedBracket

	// *** Basic block 6

	addi        a0, s1, 24
	mv          a1, s3
	call        VectorAppend

	// *** Basic block 7

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ParseFunctionDecl:
	.size ParseFunctionDecl, .func_end_ParseFunctionDecl-ParseFunctionDecl

	.local  ParseArrayDecl
	.type ParseArrayDecl, @function

ParseArrayDecl:

	// *** Basic block 0

	.global LexMatch
	.local ParseQualifiers
	.global SyntaxError
	.global NewArrayTypeRecord
	.global VectorAppend
	.global LexLookingAt
	.global SyntaxParseSingleExpression
	.global NewUnaryASTNode
	.global AnalyzeExpression
	.global EvaluateIntegerExpression
	.global TypeIsIntegral
	.global StorageIs
	.global ASTNodeDelete
	.global LexError
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -32(s0)
	// Spilled register region: 16 bytes at -48(s0) to -32(s0)
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
	sd          s1, -40(s0)	// Spilled @43
	mv          s2, x0
	mv          s3, x0
	ld          s4, 0(s1)
	li          s5, 84		// 0x54 ASCII 'T'
	mv          a1, s5
	mv          a0, s4
	call        LexMatch

	// *** Basic block 1

	beqz        a0, .ParseArrayDecl_label_66

	// *** Basic block 2

	li          s2, 1		// 0x1 ASCII \x1
	mv          a0, s1
	call        ParseQualifiers

	// *** Basic block 3

	mv          s3, a0
	j           .ParseArrayDecl_label_78

	// *** Basic block 4

.ParseArrayDecl_label_66:
	mv          a0, s1
	call        ParseQualifiers

	// *** Basic block 5

	mv          s3, a0
	mv          a1, s5
	mv          a0, s4
	call        LexMatch

	// *** Basic block 6

	mv          s2, a0

	// *** Basic block 7

.ParseArrayDecl_label_78:
	lw          t0, 72(s1)
	li          s4, 2		// 0x2 ASCII \x2
	beq         t0, s4, .ParseArrayDecl_label_99

	// *** Basic block 8

	mv          t0, s2
	bnez        s2, .ParseArrayDecl_label_89

	// *** Basic block 9

	snez        t0, s3

	// *** Basic block 10

.ParseArrayDecl_label_89:
	beqz        t0, .ParseArrayDecl_label_98

	// *** Basic block 11

	ld          a0, 8(s1)
	lla         a1, .str.65
	call        SyntaxError

	// *** Basic block 12

.ParseArrayDecl_label_98:

	// *** Basic block 13

.ParseArrayDecl_label_99:
	lw          t0, 64(s1)
	addi        t0, t0, 1
	sw          t0, 64(s1)
	mv          a1, s2
	mv          a0, s3
	call        NewArrayTypeRecord

	// *** Basic block 14

	mv          s5, a0
	addi        a0, s1, 24
	mv          a1, s5
	call        VectorAppend

	// *** Basic block 15

	mv          s6, x0
	mv          s7, x0
	ld          s8, 0(s1)
	ld          s9, 56(s8)
	li          s10, 48		// 0x30 ASCII '0'
	mv          a1, s10
	mv          a0, s8
	call        LexMatch

	// *** Basic block 16

	beqz        a0, .ParseArrayDecl_label_162

	// *** Basic block 17

	lw          t0, 64(s1)
	li          t1, 1		// 0x1 ASCII \x1
	beq         t0, t1, .ParseArrayDecl_label_145

	// *** Basic block 18

	ld          a0, 8(s1)
	lla         a1, .str.66
	call        SyntaxError

	// *** Basic block 19

.ParseArrayDecl_label_145:
	lw          t0, 72(s1)
	beq         t0, s4, .ParseArrayDecl_label_158

	// *** Basic block 20

	addi        t0, s5, 32
	lb          t1, 16(t0)
	andi        t1, t1, -2
	ori         t1, t1, 1
	sb          t1, 16(t0)

	// *** Basic block 21

.ParseArrayDecl_label_158:

	// *** Basic block 22

.ParseArrayDecl_label_159:
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

	// *** Basic block 23

.ParseArrayDecl_label_162:
	li          t0, 52		// 0x34 ASCII '4'
	mv          a1, t0
	mv          a0, s8
	call        LexMatch

	// *** Basic block 24

	beqz        a0, .ParseArrayDecl_label_171

	// *** Basic block 25

	li          s7, 1		// 0x1 ASCII \x1

	// *** Basic block 26

.ParseArrayDecl_label_171:
	mv          s11, s7
	beqz        s7, .ParseArrayDecl_label_182

	// *** Basic block 27

	mv          a1, s10
	mv          a0, s8
	call        LexLookingAt

	// *** Basic block 28

	mv          s11, a0

	// *** Basic block 29

.ParseArrayDecl_label_182:
	beqz        s11, .ParseArrayDecl_label_208

	// *** Basic block 30

	lw          t0, 72(s1)
	beq         t0, s4, .ParseArrayDecl_label_198

	// *** Basic block 31

	ld          a0, 8(s1)
	lla         a1, .str.67
	call        SyntaxError

	// *** Basic block 32

	j           .ParseArrayDecl_label_206

	// *** Basic block 33

.ParseArrayDecl_label_198:
	li          s6, 1		// 0x1 ASCII \x1
	addi        t0, s5, 32
	lb          t1, 16(t0)
	andi        t1, t1, -9
	ori         t1, t1, 8
	sb          t1, 16(t0)

	// *** Basic block 34

.ParseArrayDecl_label_206:
	j           .ParseArrayDecl_label_329

	// *** Basic block 35

.ParseArrayDecl_label_208:
	ld          s4, 8(s1)
	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	mv          a0, s4
	call        SyntaxParseSingleExpression
	sd          a0, -40(s0)	// Spilled @216

	// *** Basic block 36

	beqz        s7, .ParseArrayDecl_label_233

	// *** Basic block 37

	mv          a3, a0
	mv          a2, s9
	mv          a1, x0
	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewUnaryASTNode

	// *** Basic block 38


	// *** Basic block 39

.ParseArrayDecl_label_233:
	call        AnalyzeExpression

	// *** Basic block 40

	li          s7, 1		// 0x1 ASCII \x1
	addi        a1, s0, -32
	call        EvaluateIntegerExpression

	// *** Basic block 41

	mv          s9, a0
	not         t0, s9
	beqz        t0, .ParseArrayDecl_label_306

	// *** Basic block 42

	ld          a0, 16(a0)
	call        TypeIsIntegral

	// *** Basic block 43

	not         t0, a0
	beqz        t0, .ParseArrayDecl_label_263

	// *** Basic block 44

	lla         a1, .str.68
	mv          a0, s4
	call        SyntaxError

	// *** Basic block 45

.ParseArrayDecl_label_263:
	lw          s9, 72(s1)
	addi        t1, s9, -1
	snez        t0, t1
	li          t1, 1		// 0x1 ASCII \x1
	beq         s9, t1, .ParseArrayDecl_label_273

	// *** Basic block 46

	addi        t1, s9, -2
	snez        t0, t1

	// *** Basic block 47

.ParseArrayDecl_label_273:
	beqz        t0, .ParseArrayDecl_label_282

	// *** Basic block 48

	lla         a1, .str.69
	mv          a0, s4
	call        SyntaxError

	// *** Basic block 49

	j           .ParseArrayDecl_label_304

	// *** Basic block 50

.ParseArrayDecl_label_282:
	lw          a0, 56(s1)
	li          s9, 10		// 0xa ASCII \xa
	mv          a1, s9
	call        StorageIs

	// *** Basic block 51

	beqz        a0, .ParseArrayDecl_label_298

	// *** Basic block 52

	lla         a1, .str.70
	mv          a0, s4
	call        SyntaxError

	// *** Basic block 53

	j           .ParseArrayDecl_label_303

	// *** Basic block 54

.ParseArrayDecl_label_298:
	sd          a0, 32(s5)
	mv          s7, x0
	li          s6, 1		// 0x1 ASCII \x1

	// *** Basic block 55

.ParseArrayDecl_label_303:

	// *** Basic block 56

.ParseArrayDecl_label_304:
	j           .ParseArrayDecl_label_323

	// *** Basic block 57

.ParseArrayDecl_label_306:
	ld          t0, -32(s0)
	bge         t0, x0, .ParseArrayDecl_label_319

	// *** Basic block 58

	lla         a1, .str.71
	mv          a0, s4
	call        SyntaxError

	// *** Basic block 59

	li          t0, 1		// 0x1 ASCII \x1
	sd          t0, -32(s0)

	// *** Basic block 60

.ParseArrayDecl_label_319:
	ld          t0, -32(s0)
	sw          t0, 32(s5)

	// *** Basic block 61

.ParseArrayDecl_label_323:
	beqz        s7, .ParseArrayDecl_label_328

	// *** Basic block 62

	call        ASTNodeDelete

	// *** Basic block 63

.ParseArrayDecl_label_328:

	// *** Basic block 64

.ParseArrayDecl_label_329:
	addi        s4, s5, 32
	lb          s1, 16(s4)
	andi        s1, s1, -5
	sd          s1, -48(s0)	// Spilled @333
	andi        s1, s6, 1
	slli        s1, s1, 2
	ld          t0, -48(s0)	// Spilled @333
	or          s1, t0, s1
	sb          s1, 16(s4)
	mv          a1, s10
	mv          a0, s8
	call        LexMatch

	// *** Basic block 65

	not         s1, a0
	beqz        s1, .ParseArrayDecl_label_352

	// *** Basic block 66

	lla         a1, .str.72
	mv          a0, s8
	call        LexError

	// *** Basic block 67

.ParseArrayDecl_label_352:
	j           .ParseArrayDecl_label_159
.func_end_ParseArrayDecl:
	.size ParseArrayDecl, .func_end_ParseArrayDecl-ParseArrayDecl

	.global TypeParserParseFuncOrArray
	.type TypeParserParseFuncOrArray, @function

TypeParserParseFuncOrArray:

	// *** Basic block 0

	.global TypeParserParseBase
	.global LexLookingAt
	.global LexMatch
	.local ParseFunctionDecl
	.local ParseArrayDecl
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
	call        TypeParserParseBase

	// *** Basic block 1

	ld          s3, 0(s1)
	li          s4, 29		// 0x1d ASCII \x1d
	mv          a1, s4
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 2

	mv          s2, a0
	bnez        a0, .TypeParserParseFuncOrArray_label_35

	// *** Basic block 3

	li          t0, 32		// 0x20 ASCII ' '
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 4

	mv          s2, a0

	// *** Basic block 5

.TypeParserParseFuncOrArray_label_35:
	beqz        s2, .TypeParserParseFuncOrArray_label_80

	// *** Basic block 6

.TypeParserParseFuncOrArray_label_37:
	mv          a1, s4
	mv          a0, s3
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .TypeParserParseFuncOrArray_label_49

	// *** Basic block 8

	mv          a0, s1
	call        ParseFunctionDecl

	// *** Basic block 9

	j           .TypeParserParseFuncOrArray_label_61

	// *** Basic block 10

.TypeParserParseFuncOrArray_label_49:
	li          t0, 32		// 0x20 ASCII ' '
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 11

	beqz        a0, .TypeParserParseFuncOrArray_label_60

	// *** Basic block 12

	mv          a0, s1
	call        ParseArrayDecl

	// *** Basic block 13

.TypeParserParseFuncOrArray_label_60:

	// *** Basic block 14

.TypeParserParseFuncOrArray_label_61:
	mv          a1, s4
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 15

	mv          s5, a0
	bnez        a0, .TypeParserParseFuncOrArray_label_78

	// *** Basic block 16

	li          t0, 32		// 0x20 ASCII ' '
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 17

	mv          s5, a0

	// *** Basic block 18

.TypeParserParseFuncOrArray_label_78:
	bnez        s5, .TypeParserParseFuncOrArray_label_37

	// *** Basic block 19

.TypeParserParseFuncOrArray_label_80:
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
.func_end_TypeParserParseFuncOrArray:
	.size TypeParserParseFuncOrArray, .func_end_TypeParserParseFuncOrArray-TypeParserParseFuncOrArray

	.global TypeParserParseBase
	.type TypeParserParseBase, @function

TypeParserParseBase:

	// *** Basic block 0

	.global LexMatch
	.global TypeParserParsePointer
	.global LexError
	.global LexLookingAt
	.global LexNextToken
	.global NewSymbol
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
	ld          s2, 0(s1)
	li          a1, 29		// 0x1d ASCII \x1d
	mv          a0, s2
	call        LexMatch

	// *** Basic block 1

	beqz        a0, .TypeParserParseBase_label_49

	// *** Basic block 2

	mv          a0, s1
	call        TypeParserParsePointer

	// *** Basic block 3

	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s2
	call        LexMatch

	// *** Basic block 4

	not         t0, a0
	beqz        t0, .TypeParserParseBase_label_47

	// *** Basic block 5

	lla         a1, .str.73
	mv          a0, s2
	call        LexError

	// *** Basic block 6

.TypeParserParseBase_label_47:
	j           .TypeParserParseBase_label_83

	// *** Basic block 7

.TypeParserParseBase_label_49:
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s2
	call        LexLookingAt

	// *** Basic block 8

	beqz        a0, .TypeParserParseBase_label_82

	// *** Basic block 9

	ld          s3, 56(s2)
	ld          a0, 0(s1)
	addi        s2, a0, 80
	call        LexNextToken

	// *** Basic block 10

	ld          a0, 16(s2)
	ld          a1, 48(s1)
	lw          a2, 56(s1)
	call        NewSymbol

	// *** Basic block 11

	sd          a0, 16(s1)
	ld          t0, 16(s1)
	sd          s3, 104(t0)

	// *** Basic block 12

.TypeParserParseBase_label_82:

	// *** Basic block 13

.TypeParserParseBase_label_83:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeParserParseBase:
	.size TypeParserParseBase, .func_end_TypeParserParseBase-TypeParserParseBase

	.local  PrintStructMember
	.type PrintStructMember, @function

PrintStructMember:

	// *** Basic block 0

	.global printf
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 0(t0)
	lla         a0, .str.74
	ld          a1, 16(t1)
	j           printf
.func_end_PrintStructMember:
	.size PrintStructMember, .func_end_PrintStructMember-PrintStructMember

	.global FindStructMember
	.type FindStructMember, @function

FindStructMember:

	// *** Basic block 0

	.global MapFindPointerKey
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 40
	j           MapFindPointerKey
.func_end_FindStructMember:
	.size FindStructMember, .func_end_FindStructMember-FindStructMember

	.local  CheckStructMember
	.type CheckStructMember, @function

CheckStructMember:

	// *** Basic block 0

	.global FindStructMember
	addi sp, sp, -16
	// Saved return address (offset 8) and frame pointer (offset 0)
	sd ra, 8(sp)
	sd s0, 0(sp)
	addi s0, sp, 16
	// Local vars at offset -16(s0)
	// End of stack frame
	call        FindStructMember

	// *** Basic block 1

	sub         t0, a0, x0
	seqz        a0, t0

	// *** Basic block 2

.CheckStructMember_label_19:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CheckStructMember:
	.size CheckStructMember, .func_end_CheckStructMember-CheckStructMember

	.local  AlignNextOffset
	.type AlignNextOffset, @function

AlignNextOffset:

	// *** Basic block 0

	.global TypeRecordAlignment
	// Leaf procedure, no stack frame generated
	mv          a0, a1
	j           TypeRecordAlignment
.func_end_AlignNextOffset:
	.size AlignNextOffset, .func_end_AlignNextOffset-AlignNextOffset

	.local  ParseBitField
	.type ParseBitField, @function

ParseBitField:

	// *** Basic block 0

	.global SyntaxParseSingleExpression
	.global snprintf
	.global EvaluateIntegerExpression
	.global ASTNodeDelete
	.global TypeIsIntegral
	.local AlignNextOffset
	.global SyntaxError
	addi sp, sp, -368
	// Saved return address (offset 360) and frame pointer (offset 352)
	sd ra, 360(sp)
	sd s0, 352(sp)
	addi s0, sp, 368
	// Local vars at offset -288(s0)
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
	mv          t0, a0
	mv          s1, a3
	mv          s2, a2
	mv          s3, a4
	mv          s4, a1
	ld          s5, 8(t0)
	li          a1, 256		// 0x100
	mv          a0, s5
	call        SyntaxParseSingleExpression

	// *** Basic block 1

	mv          s6, a0
	bne         s6, x0, .ParseBitField_label_69

	// *** Basic block 2

	addi        a0, s0, -288
	lla         a2, .str.75
	li          t0, 256		// 0x100
	mv          a1, t0
	call        snprintf

	// *** Basic block 3

	j           .ParseBitField_label_207

	// *** Basic block 4

.ParseBitField_label_69:
	sd          x0, -32(s0)
	addi        a1, s0, -32
	mv          a0, s6
	call        EvaluateIntegerExpression

	// *** Basic block 5

	not         t0, a0
	beqz        t0, .ParseBitField_label_92

	// *** Basic block 6

	mv          a0, s6
	call        ASTNodeDelete

	// *** Basic block 7

	addi        a0, s0, -288
	lla         a2, .str.76
	li          t0, 256		// 0x100
	mv          a1, t0
	call        snprintf

	// *** Basic block 8

	j           .ParseBitField_label_207

	// *** Basic block 9

.ParseBitField_label_92:
	mv          a0, s6
	call        ASTNodeDelete

	// *** Basic block 10

	ld          s7, 40(s1)
	mv          a0, s7
	call        TypeIsIntegral

	// *** Basic block 11

	not         t0, a0
	beqz        t0, .ParseBitField_label_113

	// *** Basic block 12

	addi        a0, s0, -288
	lla         a2, .str.77
	li          t0, 256		// 0x100
	mv          a1, t0
	call        snprintf

	// *** Basic block 13

	j           .ParseBitField_label_207

	// *** Basic block 14

.ParseBitField_label_113:
	lw          s1, 20(s7)
	slli        s8, s1, 3
	ld          s9, -32(s0)
	slt         t1, x0, s9
	not         t0, t1
	blt         s9, x0, .ParseBitField_label_125

	// *** Basic block 15

	slt         t0, s8, s9

	// *** Basic block 16

.ParseBitField_label_125:
	beqz        t0, .ParseBitField_label_141

	// *** Basic block 17

	addi        a0, s0, -288
	lla         a2, .str.78
	mv          a4, s8
	mv          a3, s9
	li          t0, 256		// 0x100
	mv          a1, t0
	call        snprintf

	// *** Basic block 18

	j           .ParseBitField_label_207

	// *** Basic block 19

.ParseBitField_label_141:
	lw          t0, 84(s2)
	add         t0, t0, s9
	bge         s8, t0, .ParseBitField_label_185

	// *** Basic block 20

	mv          a1, s7
	mv          a0, s2
	call        AlignNextOffset

	// *** Basic block 21

	lw          t0, 72(s2)
	sw          t0, 8(s3)
	addi        t0, s2, 16
	ld          t0, 8(t0)
	addi        t0, t0, -1
	sd          t0, 24(s3)
	sw          x0, 84(s2)
	not         t0, s4
	beqz        t0, .ParseBitField_label_175

	// *** Basic block 22

	lw          t0, 72(s2)
	add         t1, t0, s1
	sw          t1, 72(s2)
	sw          t0, 76(s2)
	j           .ParseBitField_label_183

	// *** Basic block 23

.ParseBitField_label_175:
	lw          t0, 76(s2)
	bge         t0, s1, .ParseBitField_label_182

	// *** Basic block 24

	sw          s1, 76(s2)

	// *** Basic block 25

.ParseBitField_label_182:

	// *** Basic block 26

.ParseBitField_label_183:
	j           .ParseBitField_label_190

	// *** Basic block 27

.ParseBitField_label_185:
	lw          t0, 88(s2)
	sw          t0, 8(s3)

	// *** Basic block 28

.ParseBitField_label_190:
	sw          s9, 16(s3)
	lw          t0, 84(s2)
	sw          t0, 12(s3)
	not         t0, s4
	beqz        t0, .ParseBitField_label_203

	// *** Basic block 29

	lw          t0, 84(s2)
	add         t0, t0, s9
	sw          t0, 84(s2)

	// *** Basic block 30

.ParseBitField_label_203:

	// *** Basic block 31

.ParseBitField_label_204:
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

	// *** Basic block 32

.ParseBitField_label_207:
	lla         a1, .str.79
	addi        a2, s0, -288
	mv          a0, s5
	call        SyntaxError

	// *** Basic block 33

	j           .ParseBitField_label_204
.func_end_ParseBitField:
	.size ParseBitField, .func_end_ParseBitField-ParseBitField

	.local  ParseStructMembers
	.type ParseStructMembers, @function

ParseStructMembers:

	// *** Basic block 0

	.global LexLookingAt
	.global TypeParserParseType
	.global LexEof
	.global TypeParserParseDeclarator
	.global SyntaxError
	.local CheckStructMember
	.global SymbolDelete
	.global NewStructMember
	.global VectorAppend
	.global MapInsert
	.global LexMatch
	.local ParseBitField
	.local AlignNextOffset
	.global SyntaxNeedSemicolon
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
	mv          s3, a2
	ld          s4, 0(s1)
	li          s5, 44		// 0x2c ASCII ','
	mv          a1, s5
	mv          a0, s4
	call        LexLookingAt

	// *** Basic block 1

	ld          s6, 8(s1)
	ld          s7, 8(s1)
	not         s8, s3
	ld          s9, 8(s1)
	not         t0, a0
	beqz        t0, .ParseStructMembers_label_235

	// *** Basic block 2

.ParseStructMembers_label_58:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s1
	call        TypeParserParseType

	// *** Basic block 3

	mv          s10, a0
	mv          a0, s4
	call        LexEof

	// *** Basic block 4

	not         t0, a0
	beqz        t0, .ParseStructMembers_label_220

	// *** Basic block 5

.ParseStructMembers_label_72:
	mv          a1, s10
	mv          a0, s1
	call        TypeParserParseDeclarator

	// *** Basic block 6

	mv          s11, a0
	bne         s11, x0, .ParseStructMembers_label_90

	// *** Basic block 7

	lla         a1, .str.80
	mv          a0, s6
	call        SyntaxError

	// *** Basic block 8

	j           .ParseStructMembers_label_205

	// *** Basic block 9

.ParseStructMembers_label_90:
	mv          a1, s11
	mv          a0, s2
	call        CheckStructMember

	// *** Basic block 10

	not         t0, a0
	beqz        t0, .ParseStructMembers_label_111

	// *** Basic block 11

	lla         a1, .str.81
	ld          a2, 16(s11)
	mv          a0, s7
	call        SyntaxError

	// *** Basic block 12

	mv          a0, s11
	call        SymbolDelete

	// *** Basic block 13

	j           .ParseStructMembers_label_204

	// *** Basic block 14

.ParseStructMembers_label_111:
	mv          a0, s11
	call        NewStructMember

	// *** Basic block 15

	mv          s6, a0
	addi        a0, s2, 16
	mv          a1, s6
	call        VectorAppend

	// *** Basic block 16

	ld          t0, 0(s6)
	sd          t0, -32(s0)
	addi        t0, s0, -32
	sd          s6, 8(t0)
	addi        a0, s2, 40
	addi        sp, sp, -16
	ld          t0, -32(s0)
	sd          t0, 0(sp)
	ld          t0, -24(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	call        MapInsert

	// *** Basic block 17

	addi        sp, sp, 16
	li          t0, 17		// 0x11 ASCII \x11
	mv          a1, t0
	mv          a0, s4
	call        LexMatch

	// *** Basic block 18

	beqz        a0, .ParseStructMembers_label_163

	// *** Basic block 19

	mv          a4, s6
	mv          a3, s11
	mv          a2, s2
	mv          a1, s3
	mv          a0, s1
	call        ParseBitField

	// *** Basic block 20

	j           .ParseStructMembers_label_203

	// *** Basic block 21

.ParseStructMembers_label_163:
	ld          s7, 40(s11)
	mv          a1, s7
	mv          a0, s2
	call        AlignNextOffset

	// *** Basic block 22

	lw          t0, 72(s2)
	sw          t0, 8(s6)
	addi        t0, s2, 16
	ld          t0, 8(t0)
	addi        t0, t0, -1
	sd          t0, 24(s6)
	beqz        s8, .ParseStructMembers_label_192

	// *** Basic block 23

	lw          t0, 20(s7)
	lw          t1, 72(s2)
	add         t0, t1, t0
	sw          t0, 72(s2)
	sw          t1, 76(s2)
	j           .ParseStructMembers_label_202

	// *** Basic block 24

.ParseStructMembers_label_192:
	lw          t0, 20(s7)
	lw          t1, 76(s2)
	bge         t1, t0, .ParseStructMembers_label_201

	// *** Basic block 25

	sw          t0, 76(s2)

	// *** Basic block 26

.ParseStructMembers_label_201:

	// *** Basic block 27

.ParseStructMembers_label_202:

	// *** Basic block 28

.ParseStructMembers_label_203:

	// *** Basic block 29

.ParseStructMembers_label_204:

	// *** Basic block 30

.ParseStructMembers_label_205:
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	mv          a0, s4
	call        LexMatch

	// *** Basic block 31

	not         t0, a0
	bnez        t0, .ParseStructMembers_label_220

	// *** Basic block 32

.ParseStructMembers_label_214:
	mv          a0, s4
	call        LexEof

	// *** Basic block 33

	not         t0, a0
	bnez        t0, .ParseStructMembers_label_72

	// *** Basic block 34

.ParseStructMembers_label_220:
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s9
	call        SyntaxNeedSemicolon

	// *** Basic block 35

	mv          a1, s5
	mv          a0, s4
	call        LexLookingAt

	// *** Basic block 36

	not         t0, a0
	bnez        t0, .ParseStructMembers_label_58

	// *** Basic block 37

.ParseStructMembers_label_235:
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
.func_end_ParseStructMembers:
	.size ParseStructMembers, .func_end_ParseStructMembers-ParseStructMembers

	.local  CheckFlexibleArrays
	.type CheckFlexibleArrays, @function

CheckFlexibleArrays:

	// *** Basic block 0

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
	mv          t0, a2
	mv          t1, a0
	mv          s1, x0
	addi        t2, a1, 16
	ld          s2, 8(t2)
	bge         x0, s2, .CheckFlexibleArrays_label_111

	// *** Basic block 1

	ld          t2, 16(a1)
	ld          s3, 8(t1)
	ld          s4, 8(t1)
	addi        s5, s2, -1
	ld          s6, 8(t1)

	// *** Basic block 2

.CheckFlexibleArrays_label_43:
	slli        t1, s1, 3
	add         t1, t2, t1
	ld          t2, 0(t1)
	ld          s7, 0(t2)
	ld          t1, 40(s7)
	lw          t1, 16(t1)
	addi        t1, t1, -2
	seqz        t2, t1

	// *** Basic block 3

.CheckFlexibleArrays_label_59:
	beqz        t2, .CheckFlexibleArrays_label_106

	// *** Basic block 4

	j           .CheckFlexibleArrays_label_62

	// *** Basic block 5

.CheckFlexibleArrays_label_62:
	addi        t1, t1, 32
	lb          t1, 16(t1)
	slli        t1, t1, 63
	srai        t1, t1, 63
	beqz        t1, .CheckFlexibleArrays_label_105

	// *** Basic block 6

	beqz        t0, .CheckFlexibleArrays_label_77

	// *** Basic block 7

	lla         a1, .str.82
	mv          a0, s3
	call        SyntaxError

	// *** Basic block 8

	j           .CheckFlexibleArrays_label_111

	// *** Basic block 9

.CheckFlexibleArrays_label_77:
	li          t0, 1		// 0x1 ASCII \x1
	bne         s2, t0, .CheckFlexibleArrays_label_90

	// *** Basic block 10

	lla         a1, .str.83
	ld          a2, 16(s7)
	mv          a0, s4
	call        SyntaxError

	// *** Basic block 11

	j           .CheckFlexibleArrays_label_111

	// *** Basic block 12

.CheckFlexibleArrays_label_90:
	beq         s1, s5, .CheckFlexibleArrays_label_104

	// *** Basic block 13

	lla         a1, .str.84
	ld          a2, 16(s7)
	mv          a0, s6
	call        SyntaxError

	// *** Basic block 14

	j           .CheckFlexibleArrays_label_111

	// *** Basic block 15

.CheckFlexibleArrays_label_104:

	// *** Basic block 16

.CheckFlexibleArrays_label_105:

	// *** Basic block 17

.CheckFlexibleArrays_label_106:

	// *** Basic block 18

.CheckFlexibleArrays_label_107:
	addi        s1, s1, 1
	bge         s1, s2, .CheckFlexibleArrays_label_43

	// *** Basic block 19

.CheckFlexibleArrays_label_111:
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
.func_end_CheckFlexibleArrays:
	.size CheckFlexibleArrays, .func_end_CheckFlexibleArrays-CheckFlexibleArrays

	.local  CheckTagType
	.type CheckTagType, @function

CheckTagType:

	// *** Basic block 0

	.global TypeIsEnum
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
	mv          s1, a1
	mv          s2, a3
	mv          s3, a2
	mv          s4, a0
	mv          s5, x0
	ld          s6, 40(s1)
	mv          a0, s6
	call        TypeIsEnum

	// *** Basic block 1

	beqz        a0, .CheckTagType_label_34

	// *** Basic block 2

	not         s5, s2
	j           .CheckTagType_label_42

	// *** Basic block 3

.CheckTagType_label_34:
	ld          s7, 32(s6)
	lb          t0, 80(s7)
	sub         t0, t0, s3
	snez        s5, t0

	// *** Basic block 4

.CheckTagType_label_42:
	beqz        s5, .CheckTagType_label_55

	// *** Basic block 5

	ld          a0, 8(s4)
	lla         a1, .str.85
	ld          a2, 16(s1)
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
	j           SyntaxError

	// *** Basic block 6

.CheckTagType_label_55:
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
.func_end_CheckTagType:
	.size CheckTagType, .func_end_CheckTagType-CheckTagType

	.local  ParseStructBody
	.type ParseStructBody, @function

ParseStructBody:

	// *** Basic block 0

	.global StringSet
	.global SyntaxFakeName
	.global SyntaxFindTopScopeTag
	.global SyntaxError
	.local CheckTagType
	.global NewStruct
	.global NewTypeRecord
	.global NewSymbol
	.global SyntaxAddTag
	.local ParseStructMembers
	.global SyntaxNeedBracket
	.local CheckFlexibleArrays
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
	mv          s3, a2
	mv          s4, x0
	ld          t0, 24(s1)
	seqz        s5, t0
	beqz        s5, .ParseStructBody_label_61

	// *** Basic block 1

	ld          a0, 8(s2)
	call        SyntaxFakeName

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        StringSet

	// *** Basic block 3

.ParseStructBody_label_61:
	ld          s6, 8(s2)
	mv          a1, s1
	mv          a0, s6
	call        SyntaxFindTopScopeTag

	// *** Basic block 4

	mv          s7, a0
	beq         s7, x0, .ParseStructBody_label_107

	// *** Basic block 5

	lb          t0, 56(s7)
	slli        t0, t0, 61
	srai        t0, t0, 63
	not         t0, t0
	beqz        t0, .ParseStructBody_label_90

	// *** Basic block 6

	lla         a1, .str.86
	ld          a2, 16(s1)
	mv          a0, s6
	call        SyntaxError

	// *** Basic block 7

	j           .ParseStructBody_label_101

	// *** Basic block 8

.ParseStructBody_label_90:
	mv          a3, x0
	mv          a2, s3
	mv          a1, s7
	mv          a0, s2
	call        CheckTagType

	// *** Basic block 9

.ParseStructBody_label_101:
	ld          t0, 40(s7)
	ld          s4, 32(t0)
	j           .ParseStructBody_label_153

	// *** Basic block 10

.ParseStructBody_label_107:
	mv          a0, s3
	call        NewStruct

	// *** Basic block 11

	mv          s4, a0
	beqz        s3, .ParseStructBody_label_117

	// *** Basic block 12

	li          s8, 2048		// 0x800
	j           .ParseStructBody_label_119

	// *** Basic block 13

.ParseStructBody_label_117:
	li          s8, 1024		// 0x400

	// *** Basic block 14

.ParseStructBody_label_119:
	mv          a1, x0
	mv          a0, s8
	call        NewTypeRecord

	// *** Basic block 15

	mv          s8, a0
	sd          s4, 32(s8)
	ld          a0, 16(s1)
	mv          a2, x0
	mv          a1, s8
	call        NewSymbol

	// *** Basic block 16

	mv          s7, a0
	sd          s7, 8(s4)
	beqz        s5, .ParseStructBody_label_147

	// *** Basic block 17

	addi        t0, s7, 56
	lb          t1, 1(t0)
	andi        t1, t1, -2
	ori         t1, t1, 1
	sb          t1, 1(t0)

	// *** Basic block 18

.ParseStructBody_label_147:
	mv          a1, s7
	mv          a0, s6
	call        SyntaxAddTag

	// *** Basic block 19

.ParseStructBody_label_153:
	lb          t0, 56(s7)
	andi        t0, t0, -5
	sb          t0, 56(s7)
	lb          t0, 56(s7)
	andi        t0, t0, -2
	ori         t0, t0, 1
	sb          t0, 56(s7)
	mv          a2, s3
	mv          a1, s4
	mv          a0, s2
	call        ParseStructMembers

	// *** Basic block 20

	lw          t0, 76(s4)
	addi        t0, t0, 7
	andi        t0, t0, -8
	sw          t0, 76(s4)
	li          t0, 64		// 0x40 ASCII '@'
	mv          a2, t0
	li          t0, 44		// 0x2c ASCII ','
	mv          a1, t0
	mv          a0, s6
	call        SyntaxNeedBracket

	// *** Basic block 21

	mv          a2, s3
	mv          a1, s4
	mv          a0, s2
	call        CheckFlexibleArrays

	// *** Basic block 22

	mv          a0, s7

	// *** Basic block 23

.ParseStructBody_label_194:
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
.func_end_ParseStructBody:
	.size ParseStructBody, .func_end_ParseStructBody-ParseStructBody

	.global TypeParserParseStruct
	.type TypeParserParseStruct, @function

TypeParserParseStruct:

	// *** Basic block 0

	.global LexLookingAt
	.global StringSetString
	.global LexNextToken
	.global LexMatch
	.local ParseStructBody
	.global SyntaxFindTag
	.global NewStruct
	.global NewTypeRecord
	.global NewSymbol
	.global SyntaxAddTag
	.local CheckTagType
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -64(s0)
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
	ld          s3, 0(s1)
	li          a1, 49		// 0x31 ASCII '1'
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .TypeParserParseStruct_label_47

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.TypeParserParseStruct_label_44:
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

	// *** Basic block 4

.TypeParserParseStruct_label_47:
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 5

	beqz        a0, .TypeParserParseStruct_label_76

	// *** Basic block 6

	addi        a0, s0, -64
	ld          s3, 0(s1)
	addi        a1, s3, 80
	call        StringSetString

	// *** Basic block 7

	mv          a0, s3
	call        LexNextToken

	// *** Basic block 8

.TypeParserParseStruct_label_76:
	mv          s3, x0
	ld          a0, 0(s1)
	li          t0, 24		// 0x18 ASCII \x18
	mv          a1, t0
	call        LexMatch

	// *** Basic block 9

	beqz        a0, .TypeParserParseStruct_label_96

	// *** Basic block 10

	addi        a1, s0, -64
	mv          a2, s2
	mv          a0, s1
	call        ParseStructBody

	// *** Basic block 11

	mv          s3, a0
	j           .TypeParserParseStruct_label_167

	// *** Basic block 12

.TypeParserParseStruct_label_96:
	addi        t0, s0, -64
	ld          t0, 24(t0)
	bnez        t0, .TypeParserParseStruct_label_105

	// *** Basic block 13

	mv          a0, x0
	j           .TypeParserParseStruct_label_44

	// *** Basic block 14

.TypeParserParseStruct_label_105:
	ld          s4, 8(s1)
	addi        a1, s0, -64
	mv          a0, s4
	call        SyntaxFindTag

	// *** Basic block 15

	mv          s3, a0
	bne         s3, x0, .TypeParserParseStruct_label_155

	// *** Basic block 16

	mv          a0, s2
	call        NewStruct

	// *** Basic block 17

	mv          s5, a0
	mv          a1, x0
	li          t0, 1024		// 0x400
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 18

	mv          s6, a0
	sd          s5, 32(s6)
	addi        t0, s0, -64
	ld          a0, 16(t0)
	mv          a2, x0
	mv          a1, s6
	call        NewSymbol

	// *** Basic block 19

	mv          s3, a0
	lb          t0, 56(s3)
	andi        t0, t0, -5
	ori         t0, t0, 4
	sb          t0, 56(s3)
	sd          s3, 8(s5)
	mv          a1, s3
	mv          a0, s4
	call        SyntaxAddTag

	// *** Basic block 20

	j           .TypeParserParseStruct_label_166

	// *** Basic block 21

.TypeParserParseStruct_label_155:
	mv          a3, x0
	mv          a2, s2
	mv          a1, s3
	mv          a0, s1
	call        CheckTagType

	// *** Basic block 22

.TypeParserParseStruct_label_166:

	// *** Basic block 23

.TypeParserParseStruct_label_167:
	mv          a0, s3
	j           .TypeParserParseStruct_label_44
.func_end_TypeParserParseStruct:
	.size TypeParserParseStruct, .func_end_TypeParserParseStruct-TypeParserParseStruct

	.local  ParseEnumConstants
	.type ParseEnumConstants, @function

ParseEnumConstants:

	// *** Basic block 0

	.global LexLookingAt
	.global StringInit
	.global LexNextToken
	.global LexMatch
	.global SyntaxParseSingleExpression
	.global AnalyzeExpression
	.global EvaluateIntegerExpression
	.global SyntaxError
	.global ASTNodeDelete
	.global NewEnumConstant
	.global StringDestruct
	.global SyntaxAddSymbol
	.global SymbolDelete
	.global VectorAppend
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -64(s0)
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
	ld          s3, 0(s1)
	li          s4, 44		// 0x2c ASCII ','
	mv          a1, s4
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 1

	addi        t0, s3, 80
	ld          s5, 16(t0)
	ld          s6, 8(s1)
	ld          s7, 8(s1)
	not         t0, a0
	beqz        t0, .ParseEnumConstants_label_185

	// *** Basic block 2

.ParseEnumConstants_label_53:
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 3

	beqz        a0, .ParseEnumConstants_label_167

	// *** Basic block 4

	addi        a0, s0, -64
	mv          a1, s5
	call        StringInit

	// *** Basic block 5

	mv          a0, s3
	call        LexNextToken

	// *** Basic block 6

	li          t0, 12		// 0xc ASCII \xc
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 7

	beqz        a0, .ParseEnumConstants_label_120

	// *** Basic block 8

	li          t0, 256		// 0x100
	mv          a1, t0
	mv          a0, s6
	call        SyntaxParseSingleExpression

	// *** Basic block 9

	mv          s1, a0
	mv          a0, s1
	call        AnalyzeExpression

	// *** Basic block 10

	mv          s1, a0
	lw          s5, 40(s2)
	sd          s5, -24(s0)
	addi        a1, s0, -24
	mv          a0, s1
	call        EvaluateIntegerExpression

	// *** Basic block 11

	not         t0, a0
	beqz        t0, .ParseEnumConstants_label_113

	// *** Basic block 12

	lla         a1, .str.87
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s6
	call        SyntaxError

	// *** Basic block 13

	sd          s5, -24(s0)

	// *** Basic block 14

.ParseEnumConstants_label_113:
	ld          t0, -24(s0)
	sw          t0, 40(s2)
	mv          a0, s1
	call        ASTNodeDelete

	// *** Basic block 15

.ParseEnumConstants_label_120:
	addi        t0, s0, -64
	ld          a0, 16(t0)
	lw          a1, 40(s2)
	call        NewEnumConstant

	// *** Basic block 16

	mv          s5, a0
	lw          t0, 40(s2)
	addi        t0, t0, 1
	sw          t0, 40(s2)
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 17

	mv          a1, s5
	mv          a0, s7
	call        SyntaxAddSymbol

	// *** Basic block 18

	mv          s6, a0
	not         t0, s6
	beqz        t0, .ParseEnumConstants_label_160

	// *** Basic block 19

	lla         a1, .str.88
	ld          a2, 16(s5)
	mv          a0, s7
	call        SyntaxError

	// *** Basic block 20

	mv          a0, s5
	call        SymbolDelete

	// *** Basic block 21

	j           .ParseEnumConstants_label_166

	// *** Basic block 22

.ParseEnumConstants_label_160:
	addi        a0, s2, 16
	mv          a1, s5
	call        VectorAppend

	// *** Basic block 23

.ParseEnumConstants_label_166:

	// *** Basic block 24

.ParseEnumConstants_label_167:
	li          t0, 18		// 0x12 ASCII \x12
	mv          a1, t0
	mv          a0, s3
	call        LexMatch

	// *** Basic block 25

	not         t0, a0
	bnez        t0, .ParseEnumConstants_label_185

	// *** Basic block 26

.ParseEnumConstants_label_176:
	mv          a1, s4
	mv          a0, s3
	call        LexLookingAt

	// *** Basic block 27

	not         t0, a0
	bnez        t0, .ParseEnumConstants_label_53

	// *** Basic block 28

.ParseEnumConstants_label_185:
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
.func_end_ParseEnumConstants:
	.size ParseEnumConstants, .func_end_ParseEnumConstants-ParseEnumConstants

	.local  ParseEnumBody
	.type ParseEnumBody, @function

ParseEnumBody:

	// *** Basic block 0

	.global StringSet
	.global SyntaxFakeName
	.global SyntaxFindTopScopeTag
	.global SyntaxError
	.local CheckTagType
	.global NewEnum
	.global NewTypeRecord
	.global NewSymbol
	.global SyntaxAddTag
	.local ParseEnumConstants
	.global SyntaxNeedBracket
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
	mv          s3, x0
	ld          t0, 24(s1)
	seqz        s4, t0
	beqz        s4, .ParseEnumBody_label_53

	// *** Basic block 1

	ld          a0, 8(s2)
	call        SyntaxFakeName

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        StringSet

	// *** Basic block 3

.ParseEnumBody_label_53:
	ld          s5, 8(s2)
	mv          a1, s1
	mv          a0, s5
	call        SyntaxFindTopScopeTag

	// *** Basic block 4

	mv          s6, a0
	beq         s6, x0, .ParseEnumBody_label_101

	// *** Basic block 5

	lb          t0, 56(s6)
	slli        t0, t0, 61
	srai        t0, t0, 63
	not         t0, t0
	beqz        t0, .ParseEnumBody_label_83

	// *** Basic block 6

	lla         a1, .str.89
	ld          a2, 16(s1)
	mv          a0, s5
	call        SyntaxError

	// *** Basic block 7

	j           .ParseEnumBody_label_95

	// *** Basic block 8

.ParseEnumBody_label_83:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, x0
	mv          a1, s6
	mv          a0, s2
	call        CheckTagType

	// *** Basic block 9

.ParseEnumBody_label_95:
	ld          t0, 40(s6)
	ld          s3, 32(t0)
	j           .ParseEnumBody_label_138

	// *** Basic block 10

.ParseEnumBody_label_101:
	call        NewEnum

	// *** Basic block 11

	mv          s3, a0
	mv          a1, x0
	li          t0, 4096		// 0x1000
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 12

	mv          s7, a0
	sd          s3, 32(s7)
	ld          a0, 16(s1)
	mv          a2, x0
	mv          a1, s7
	call        NewSymbol

	// *** Basic block 13

	mv          s6, a0
	sd          s6, 8(s3)
	beqz        s4, .ParseEnumBody_label_132

	// *** Basic block 14

	addi        t0, s6, 56
	lb          t1, 1(t0)
	andi        t1, t1, -2
	ori         t1, t1, 1
	sb          t1, 1(t0)

	// *** Basic block 15

.ParseEnumBody_label_132:
	mv          a1, s6
	mv          a0, s5
	call        SyntaxAddTag

	// *** Basic block 16

.ParseEnumBody_label_138:
	lb          t0, 56(s6)
	andi        t0, t0, -5
	sb          t0, 56(s6)
	lb          t0, 56(s6)
	andi        t0, t0, -2
	ori         t0, t0, 1
	sb          t0, 56(s6)
	mv          a1, s3
	mv          a0, s2
	call        ParseEnumConstants

	// *** Basic block 17

	li          t0, 4		// 0x4 ASCII \x4
	mv          a2, t0
	li          t0, 44		// 0x2c ASCII ','
	mv          a1, t0
	mv          a0, s5
	call        SyntaxNeedBracket

	// *** Basic block 18

	mv          a0, s6

	// *** Basic block 19

.ParseEnumBody_label_164:
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
.func_end_ParseEnumBody:
	.size ParseEnumBody, .func_end_ParseEnumBody-ParseEnumBody

	.global TypeParserParseEnum
	.type TypeParserParseEnum, @function

TypeParserParseEnum:

	// *** Basic block 0

	.global LexLookingAt
	.global StringSetString
	.global LexNextToken
	.global LexMatch
	.local ParseEnumBody
	.global SyntaxFindTag
	.global NewEnum
	.global NewTypeRecord
	.global NewSymbol
	.global SyntaxAddTag
	.local CheckTagType
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// End of stack frame
	mv          s1, a0
	ld          s2, 0(s1)
	li          a1, 49		// 0x31 ASCII '1'
	mv          a0, s2
	call        LexLookingAt

	// *** Basic block 1

	beqz        a0, .TypeParserParseEnum_label_45

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.TypeParserParseEnum_label_42:
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

	// *** Basic block 4

.TypeParserParseEnum_label_45:
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s2
	call        LexLookingAt

	// *** Basic block 5

	beqz        a0, .TypeParserParseEnum_label_74

	// *** Basic block 6

	addi        a0, s0, -64
	ld          s2, 0(s1)
	addi        a1, s2, 80
	call        StringSetString

	// *** Basic block 7

	mv          a0, s2
	call        LexNextToken

	// *** Basic block 8

.TypeParserParseEnum_label_74:
	mv          s2, x0
	ld          a0, 0(s1)
	li          t0, 24		// 0x18 ASCII \x18
	mv          a1, t0
	call        LexMatch

	// *** Basic block 9

	beqz        a0, .TypeParserParseEnum_label_91

	// *** Basic block 10

	addi        a1, s0, -64
	mv          a0, s1
	call        ParseEnumBody

	// *** Basic block 11

	mv          s2, a0
	j           .TypeParserParseEnum_label_162

	// *** Basic block 12

.TypeParserParseEnum_label_91:
	addi        t0, s0, -64
	ld          t0, 24(t0)
	bnez        t0, .TypeParserParseEnum_label_100

	// *** Basic block 13

	mv          a0, x0
	j           .TypeParserParseEnum_label_42

	// *** Basic block 14

.TypeParserParseEnum_label_100:
	ld          s3, 8(s1)
	addi        a1, s0, -64
	mv          a0, s3
	call        SyntaxFindTag

	// *** Basic block 15

	mv          s2, a0
	bne         s2, x0, .TypeParserParseEnum_label_149

	// *** Basic block 16

	call        NewEnum

	// *** Basic block 17

	mv          s4, a0
	mv          a1, x0
	li          t0, 4096		// 0x1000
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 18

	mv          s5, a0
	sd          s4, 32(s5)
	addi        t0, s0, -64
	ld          a0, 16(t0)
	mv          a2, x0
	mv          a1, s5
	call        NewSymbol

	// *** Basic block 19

	mv          s2, a0
	lb          t0, 56(s2)
	andi        t0, t0, -5
	ori         t0, t0, 4
	sb          t0, 56(s2)
	sd          s2, 8(s4)
	mv          a1, s2
	mv          a0, s3
	call        SyntaxAddTag

	// *** Basic block 20

	j           .TypeParserParseEnum_label_161

	// *** Basic block 21

.TypeParserParseEnum_label_149:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, x0
	mv          a1, s2
	mv          a0, s1
	call        CheckTagType

	// *** Basic block 22

.TypeParserParseEnum_label_161:

	// *** Basic block 23

.TypeParserParseEnum_label_162:
	mv          a0, s2
	j           .TypeParserParseEnum_label_42
.func_end_TypeParserParseEnum:
	.size TypeParserParseEnum, .func_end_TypeParserParseEnum-TypeParserParseEnum

	.global TypeIsInt
	.type TypeIsInt, @function

TypeIsInt:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 1

.TypeIsInt_label_17:
	beqz        t3, .TypeIsInt_label_27

	// *** Basic block 2

	j           .TypeIsInt_label_21

	// *** Basic block 3

.TypeIsInt_label_21:
	lw          t1, 8(t0)
	li          t2, 4098		// 0x1002
	and         t1, t1, t2
	snez        a0, t1

	// *** Basic block 4

.TypeIsInt_label_27:

	// *** Basic block 5

.TypeIsInt_label_29:
	ret         
.func_end_TypeIsInt:
	.size TypeIsInt, .func_end_TypeIsInt-TypeIsInt

	.global TypeIsChar
	.type TypeIsChar, @function

TypeIsChar:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 1

.TypeIsChar_label_17:
	beqz        t3, .TypeIsChar_label_26

	// *** Basic block 2

	j           .TypeIsChar_label_21

	// *** Basic block 3

.TypeIsChar_label_21:
	lw          t1, 8(t0)
	andi        t1, t1, 1
	snez        a0, t1

	// *** Basic block 4

.TypeIsChar_label_26:

	// *** Basic block 5

.TypeIsChar_label_28:
	ret         
.func_end_TypeIsChar:
	.size TypeIsChar, .func_end_TypeIsChar-TypeIsChar

	.global TypeIsShort
	.type TypeIsShort, @function

TypeIsShort:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 1

.TypeIsShort_label_17:
	beqz        t3, .TypeIsShort_label_26

	// *** Basic block 2

	j           .TypeIsShort_label_21

	// *** Basic block 3

.TypeIsShort_label_21:
	lw          t1, 8(t0)
	andi        t1, t1, 4
	snez        a0, t1

	// *** Basic block 4

.TypeIsShort_label_26:

	// *** Basic block 5

.TypeIsShort_label_28:
	ret         
.func_end_TypeIsShort:
	.size TypeIsShort, .func_end_TypeIsShort-TypeIsShort

	.global TypeIsLong
	.type TypeIsLong, @function

TypeIsLong:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 1

.TypeIsLong_label_16:
	beqz        t3, .TypeIsLong_label_25

	// *** Basic block 2

	j           .TypeIsLong_label_20

	// *** Basic block 3

.TypeIsLong_label_20:
	lw          t1, 8(t0)
	andi        t1, t1, 8
	snez        a0, t1

	// *** Basic block 4

.TypeIsLong_label_25:

	// *** Basic block 5

.TypeIsLong_label_27:
	ret         
.func_end_TypeIsLong:
	.size TypeIsLong, .func_end_TypeIsLong-TypeIsLong

	.global TypeIsLongLong
	.type TypeIsLongLong, @function

TypeIsLongLong:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 1

.TypeIsLongLong_label_16:
	beqz        t3, .TypeIsLongLong_label_25

	// *** Basic block 2

	j           .TypeIsLongLong_label_20

	// *** Basic block 3

.TypeIsLongLong_label_20:
	lw          t1, 8(t0)
	andi        t1, t1, 16
	snez        a0, t1

	// *** Basic block 4

.TypeIsLongLong_label_25:

	// *** Basic block 5

.TypeIsLongLong_label_27:
	ret         
.func_end_TypeIsLongLong:
	.size TypeIsLongLong, .func_end_TypeIsLongLong-TypeIsLongLong

	.global TypeIsUnsignedInt
	.type TypeIsUnsignedInt, @function

TypeIsUnsignedInt:

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
	.global TypeIsUnsigned
	.global TypeIsInt
	mv          s1, a0
	call        TypeIsUnsigned

	// *** Basic block 1

	beqz        a0, .TypeIsUnsignedInt_label_17

	// *** Basic block 2

	mv          a0, s1
	call        TypeIsInt

	// *** Basic block 4

.TypeIsUnsignedInt_label_17:

	// *** Basic block 5

.TypeIsUnsignedInt_label_19:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeIsUnsignedInt:
	.size TypeIsUnsignedInt, .func_end_TypeIsUnsignedInt-TypeIsUnsignedInt

	.global TypeIsUnsignedChar
	.type TypeIsUnsignedChar, @function

TypeIsUnsignedChar:

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
	.global TypeIsUnsigned
	.global TypeIsChar
	mv          s1, a0
	call        TypeIsUnsigned

	// *** Basic block 1

	beqz        a0, .TypeIsUnsignedChar_label_17

	// *** Basic block 2

	mv          a0, s1
	call        TypeIsChar

	// *** Basic block 4

.TypeIsUnsignedChar_label_17:

	// *** Basic block 5

.TypeIsUnsignedChar_label_19:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeIsUnsignedChar:
	.size TypeIsUnsignedChar, .func_end_TypeIsUnsignedChar-TypeIsUnsignedChar

	.global TypeIsUnsignedShort
	.type TypeIsUnsignedShort, @function

TypeIsUnsignedShort:

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
	.global TypeIsUnsigned
	.global TypeIsShort
	mv          s1, a0
	call        TypeIsUnsigned

	// *** Basic block 1

	beqz        a0, .TypeIsUnsignedShort_label_17

	// *** Basic block 2

	mv          a0, s1
	call        TypeIsShort

	// *** Basic block 4

.TypeIsUnsignedShort_label_17:

	// *** Basic block 5

.TypeIsUnsignedShort_label_19:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeIsUnsignedShort:
	.size TypeIsUnsignedShort, .func_end_TypeIsUnsignedShort-TypeIsUnsignedShort

	.global TypeIsUnsignedLong
	.type TypeIsUnsignedLong, @function

TypeIsUnsignedLong:

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
	.global TypeIsUnsigned
	.global TypeIsLong
	mv          s1, a0
	call        TypeIsUnsigned

	// *** Basic block 1

	beqz        a0, .TypeIsUnsignedLong_label_17

	// *** Basic block 2

	mv          a0, s1
	call        TypeIsLong

	// *** Basic block 4

.TypeIsUnsignedLong_label_17:

	// *** Basic block 5

.TypeIsUnsignedLong_label_19:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeIsUnsignedLong:
	.size TypeIsUnsignedLong, .func_end_TypeIsUnsignedLong-TypeIsUnsignedLong

	.global TypeIsUnsignedLongLong
	.type TypeIsUnsignedLongLong, @function

TypeIsUnsignedLongLong:

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
	.global TypeIsUnsigned
	.global TypeIsLongLong
	mv          s1, a0
	call        TypeIsUnsigned

	// *** Basic block 1

	beqz        a0, .TypeIsUnsignedLongLong_label_17

	// *** Basic block 2

	mv          a0, s1
	call        TypeIsLongLong

	// *** Basic block 4

.TypeIsUnsignedLongLong_label_17:

	// *** Basic block 5

.TypeIsUnsignedLongLong_label_19:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeIsUnsignedLongLong:
	.size TypeIsUnsignedLongLong, .func_end_TypeIsUnsignedLongLong-TypeIsUnsignedLongLong

	.global TypeIsFloat
	.type TypeIsFloat, @function

TypeIsFloat:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 1

.TypeIsFloat_label_17:
	beqz        t3, .TypeIsFloat_label_26

	// *** Basic block 2

	j           .TypeIsFloat_label_21

	// *** Basic block 3

.TypeIsFloat_label_21:
	lw          t1, 8(t0)
	andi        t1, t1, 32
	snez        a0, t1

	// *** Basic block 4

.TypeIsFloat_label_26:

	// *** Basic block 5

.TypeIsFloat_label_28:
	ret         
.func_end_TypeIsFloat:
	.size TypeIsFloat, .func_end_TypeIsFloat-TypeIsFloat

	.global TypeIsDouble
	.type TypeIsDouble, @function

TypeIsDouble:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 1

.TypeIsDouble_label_17:
	beqz        t3, .TypeIsDouble_label_26

	// *** Basic block 2

	j           .TypeIsDouble_label_21

	// *** Basic block 3

.TypeIsDouble_label_21:
	lw          t1, 8(t0)
	andi        t1, t1, 64
	snez        a0, t1

	// *** Basic block 4

.TypeIsDouble_label_26:

	// *** Basic block 5

.TypeIsDouble_label_28:
	ret         
.func_end_TypeIsDouble:
	.size TypeIsDouble, .func_end_TypeIsDouble-TypeIsDouble

	.global TypeIsLongDouble
	.type TypeIsLongDouble, @function

TypeIsLongDouble:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 1

.TypeIsLongDouble_label_17:
	beqz        t3, .TypeIsLongDouble_label_26

	// *** Basic block 2

	j           .TypeIsLongDouble_label_21

	// *** Basic block 3

.TypeIsLongDouble_label_21:
	lw          t1, 8(t0)
	andi        t1, t1, 128
	snez        a0, t1

	// *** Basic block 4

.TypeIsLongDouble_label_26:

	// *** Basic block 5

.TypeIsLongDouble_label_28:
	ret         
.func_end_TypeIsLongDouble:
	.size TypeIsLongDouble, .func_end_TypeIsLongDouble-TypeIsLongDouble

	.global TypeIsBool
	.type TypeIsBool, @function

TypeIsBool:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 1

.TypeIsBool_label_17:
	beqz        t3, .TypeIsBool_label_26

	// *** Basic block 2

	j           .TypeIsBool_label_21

	// *** Basic block 3

.TypeIsBool_label_21:
	lw          t1, 8(t0)
	andi        t1, t1, 256
	snez        a0, t1

	// *** Basic block 4

.TypeIsBool_label_26:

	// *** Basic block 5

.TypeIsBool_label_28:
	ret         
.func_end_TypeIsBool:
	.size TypeIsBool, .func_end_TypeIsBool-TypeIsBool

	.global TypeIsVoid
	.type TypeIsVoid, @function

TypeIsVoid:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 1

.TypeIsVoid_label_17:
	beqz        t3, .TypeIsVoid_label_26

	// *** Basic block 2

	j           .TypeIsVoid_label_21

	// *** Basic block 3

.TypeIsVoid_label_21:
	lw          t1, 8(t0)
	andi        t1, t1, 512
	snez        a0, t1

	// *** Basic block 4

.TypeIsVoid_label_26:

	// *** Basic block 5

.TypeIsVoid_label_28:
	ret         
.func_end_TypeIsVoid:
	.size TypeIsVoid, .func_end_TypeIsVoid-TypeIsVoid

	.global TypeIsPointer
	.type TypeIsPointer, @function

TypeIsPointer:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 16(a0)
	addi        t0, t0, -1
	seqz        a0, t0

	// *** Basic block 1

.TypeIsPointer_label_14:
	ret         
.func_end_TypeIsPointer:
	.size TypeIsPointer, .func_end_TypeIsPointer-TypeIsPointer

	.global TypeIsPrimitive
	.type TypeIsPrimitive, @function

TypeIsPrimitive:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 16(a0)
	seqz        a0, t0

	// *** Basic block 1

.TypeIsPrimitive_label_12:
	ret         
.func_end_TypeIsPrimitive:
	.size TypeIsPrimitive, .func_end_TypeIsPrimitive-TypeIsPrimitive

	.global TypeIsPointerOrArray
	.type TypeIsPointerOrArray, @function

TypeIsPointerOrArray:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 16(a0)
	addi        t1, t0, -1
	seqz        a0, t1
	li          t1, 1		// 0x1 ASCII \x1
	beq         t0, t1, .TypeIsPointerOrArray_label_20

	// *** Basic block 1

	addi        t0, t0, -2
	seqz        a0, t0

	// *** Basic block 2

.TypeIsPointerOrArray_label_20:

	// *** Basic block 3

.TypeIsPointerOrArray_label_22:
	ret         
.func_end_TypeIsPointerOrArray:
	.size TypeIsPointerOrArray, .func_end_TypeIsPointerOrArray-TypeIsPointerOrArray

	.global TypeIsIntegral
	.type TypeIsIntegral, @function

TypeIsIntegral:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 1

.TypeIsIntegral_label_17:
	beqz        t3, .TypeIsIntegral_label_27

	// *** Basic block 2

	j           .TypeIsIntegral_label_21

	// *** Basic block 3

.TypeIsIntegral_label_21:
	lw          t1, 8(t0)
	li          t2, 4383		// 0x111f
	and         t1, t1, t2
	snez        a0, t1

	// *** Basic block 4

.TypeIsIntegral_label_27:

	// *** Basic block 5

.TypeIsIntegral_label_29:
	ret         
.func_end_TypeIsIntegral:
	.size TypeIsIntegral, .func_end_TypeIsIntegral-TypeIsIntegral

	.global TypeIsFloatingPoint
	.type TypeIsFloatingPoint, @function

TypeIsFloatingPoint:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 1

.TypeIsFloatingPoint_label_17:
	beqz        t3, .TypeIsFloatingPoint_label_26

	// *** Basic block 2

	j           .TypeIsFloatingPoint_label_21

	// *** Basic block 3

.TypeIsFloatingPoint_label_21:
	lw          t1, 8(t0)
	andi        t1, t1, 224
	snez        a0, t1

	// *** Basic block 4

.TypeIsFloatingPoint_label_26:

	// *** Basic block 5

.TypeIsFloatingPoint_label_28:
	ret         
.func_end_TypeIsFloatingPoint:
	.size TypeIsFloatingPoint, .func_end_TypeIsFloatingPoint-TypeIsFloatingPoint

	.global TypeIsFunction
	.type TypeIsFunction, @function

TypeIsFunction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 16(a0)
	addi        t0, t0, -3
	seqz        a0, t0

	// *** Basic block 1

.TypeIsFunction_label_14:
	ret         
.func_end_TypeIsFunction:
	.size TypeIsFunction, .func_end_TypeIsFunction-TypeIsFunction

	.global TypeIsFunctionDefinition
	.type TypeIsFunctionDefinition, @function

TypeIsFunctionDefinition:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	addi        t2, t2, -3
	seqz        t3, t2

	// *** Basic block 1

.TypeIsFunctionDefinition_label_19:
	not         t1, t3
	beqz        t1, .TypeIsFunctionDefinition_label_29

	// *** Basic block 2

	j           .TypeIsFunctionDefinition_label_23

	// *** Basic block 3

.TypeIsFunctionDefinition_label_23:
	mv          a0, x0

	// *** Basic block 4

.TypeIsFunctionDefinition_label_26:
	ret         

	// *** Basic block 5

.TypeIsFunctionDefinition_label_29:
	addi        t1, t0, 32
	lb          a0, 49(t1)
	ret         
.func_end_TypeIsFunctionDefinition:
	.size TypeIsFunctionDefinition, .func_end_TypeIsFunctionDefinition-TypeIsFunctionDefinition

	.global TypeIsFunctionPointer
	.type TypeIsFunctionPointer, @function

TypeIsFunctionPointer:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	addi        t2, t2, -1
	seqz        t3, t2

	// *** Basic block 1

.TypeIsFunctionPointer_label_18:
	beqz        t3, .TypeIsFunctionPointer_label_40

	// *** Basic block 2

	j           .TypeIsFunctionPointer_label_21

	// *** Basic block 3

.TypeIsFunctionPointer_label_21:
	ld          t1, 24(t0)
	mv          t2, t1
	lw          t1, 16(t2)
	addi        t1, t1, -3
	seqz        t3, t1
	j           .TypeIsFunctionPointer_label_34

	// *** Basic block 4

.TypeIsFunctionPointer_label_34:
	mv          a0, t3

	// *** Basic block 5

.TypeIsFunctionPointer_label_37:
	ret         

	// *** Basic block 6

.TypeIsFunctionPointer_label_40:
	mv          t1, t0
	lw          t3, 16(t1)
	addi        t3, t3, -3
	seqz        t4, t3

	// *** Basic block 7

.TypeIsFunctionPointer_label_48:
	mv          a0, t4
	ret         
.func_end_TypeIsFunctionPointer:
	.size TypeIsFunctionPointer, .func_end_TypeIsFunctionPointer-TypeIsFunctionPointer

	.global TypeIsStructOrUnionPointer
	.type TypeIsStructOrUnionPointer, @function

TypeIsStructOrUnionPointer:

	// *** Basic block 0

	.global TypeIsStructOrUnion
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
	mv          t0, s1
	lw          t1, 16(t0)
	addi        t1, t1, -1
	seqz        t2, t1

	// *** Basic block 1

.TypeIsStructOrUnionPointer_label_19:
	mv          a0, t2
	beqz        t2, .TypeIsStructOrUnionPointer_label_29

	// *** Basic block 2

	j           .TypeIsStructOrUnionPointer_label_23

	// *** Basic block 3

.TypeIsStructOrUnionPointer_label_23:
	ld          a0, 24(s1)
	call        TypeIsStructOrUnion

	// *** Basic block 5

.TypeIsStructOrUnionPointer_label_29:

	// *** Basic block 6

.TypeIsStructOrUnionPointer_label_31:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeIsStructOrUnionPointer:
	.size TypeIsStructOrUnionPointer, .func_end_TypeIsStructOrUnionPointer-TypeIsStructOrUnionPointer

	.global TypeIsFunctionReturningStructOrUnion
	.type TypeIsFunctionReturningStructOrUnion, @function

TypeIsFunctionReturningStructOrUnion:

	// *** Basic block 0

	.global TypeIsStructOrUnion
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	addi        t2, t2, -3
	seqz        t3, t2

	// *** Basic block 1

.TypeIsFunctionReturningStructOrUnion_label_20:
	beqz        t3, .TypeIsFunctionReturningStructOrUnion_label_34

	// *** Basic block 2

	j           .TypeIsFunctionReturningStructOrUnion_label_23

	// *** Basic block 3

.TypeIsFunctionReturningStructOrUnion_label_23:
	ld          a0, 24(t0)
	j           TypeIsStructOrUnion

	// *** Basic block 6

.TypeIsFunctionReturningStructOrUnion_label_34:
	mv          t2, t0
	lw          t3, 16(t2)
	addi        t3, t3, -1
	seqz        t4, t3

	// *** Basic block 7

.TypeIsFunctionReturningStructOrUnion_label_44:
	mv          t1, t4
	beqz        t4, .TypeIsFunctionReturningStructOrUnion_label_60

	// *** Basic block 8

	j           .TypeIsFunctionReturningStructOrUnion_label_48

	// *** Basic block 9

.TypeIsFunctionReturningStructOrUnion_label_48:
	ld          t2, 24(t0)
	lw          t2, 16(t2)
	addi        t2, t2, -3
	seqz        t3, t2

	// *** Basic block 10

.TypeIsFunctionReturningStructOrUnion_label_57:
	mv          t1, t3
	j           .TypeIsFunctionReturningStructOrUnion_label_60

	// *** Basic block 11

.TypeIsFunctionReturningStructOrUnion_label_60:
	beqz        t1, .TypeIsFunctionReturningStructOrUnion_label_72

	// *** Basic block 12

	ld          t2, 24(t0)
	ld          a0, 24(t2)
	j           TypeIsStructOrUnion

	// *** Basic block 14

.TypeIsFunctionReturningStructOrUnion_label_72:
	mv          a0, x0
	ret         
.func_end_TypeIsFunctionReturningStructOrUnion:
	.size TypeIsFunctionReturningStructOrUnion, .func_end_TypeIsFunctionReturningStructOrUnion-TypeIsFunctionReturningStructOrUnion

	.global TypeIsVoidFunction
	.type TypeIsVoidFunction, @function

TypeIsVoidFunction:

	// *** Basic block 0

	.global TypeIsVoid
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
	mv          t0, s1
	lw          t1, 16(t0)
	addi        t1, t1, -3
	seqz        t2, t1

	// *** Basic block 1

.TypeIsVoidFunction_label_19:
	mv          a0, t2
	beqz        t2, .TypeIsVoidFunction_label_29

	// *** Basic block 2

	j           .TypeIsVoidFunction_label_23

	// *** Basic block 3

.TypeIsVoidFunction_label_23:
	ld          a0, 24(s1)
	call        TypeIsVoid

	// *** Basic block 5

.TypeIsVoidFunction_label_29:

	// *** Basic block 6

.TypeIsVoidFunction_label_31:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeIsVoidFunction:
	.size TypeIsVoidFunction, .func_end_TypeIsVoidFunction-TypeIsVoidFunction

	.global TypeIsPointerToSameType
	.type TypeIsPointerToSameType, @function

TypeIsPointerToSameType:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, t0
	lw          t3, 16(t2)
	addi        t3, t3, -1
	seqz        t4, t3

	// *** Basic block 1

.TypeIsPointerToSameType_label_23:
	mv          a0, t4
	beqz        t4, .TypeIsPointerToSameType_label_34

	// *** Basic block 2

	j           .TypeIsPointerToSameType_label_27

	// *** Basic block 3

.TypeIsPointerToSameType_label_27:
	lw          t2, 16(t0)
	lw          t3, 16(t1)
	sub         t2, t2, t3
	seqz        a0, t2

	// *** Basic block 4

.TypeIsPointerToSameType_label_34:
	beqz        a0, .TypeIsPointerToSameType_label_46

	// *** Basic block 5

	ld          t2, 24(t0)
	lw          t2, 8(t2)
	ld          t3, 24(t1)
	lw          t3, 8(t3)
	sub         t2, t2, t3
	seqz        a0, t2

	// *** Basic block 6

.TypeIsPointerToSameType_label_46:

	// *** Basic block 7

.TypeIsPointerToSameType_label_48:
	ret         
.func_end_TypeIsPointerToSameType:
	.size TypeIsPointerToSameType, .func_end_TypeIsPointerToSameType-TypeIsPointerToSameType

	.global TypeIsStructOrUnion
	.type TypeIsStructOrUnion, @function

TypeIsStructOrUnion:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 1

.TypeIsStructOrUnion_label_17:
	beqz        t3, .TypeIsStructOrUnion_label_27

	// *** Basic block 2

	j           .TypeIsStructOrUnion_label_21

	// *** Basic block 3

.TypeIsStructOrUnion_label_21:
	lw          t1, 8(t0)
	li          t2, 3072		// 0xc00
	and         t1, t1, t2
	snez        a0, t1

	// *** Basic block 4

.TypeIsStructOrUnion_label_27:

	// *** Basic block 5

.TypeIsStructOrUnion_label_29:
	ret         
.func_end_TypeIsStructOrUnion:
	.size TypeIsStructOrUnion, .func_end_TypeIsStructOrUnion-TypeIsStructOrUnion

	.global TypeIsScalar
	.type TypeIsScalar, @function

TypeIsScalar:

	// *** Basic block 0

	addi sp, sp, -16
	// Saved return address (offset 8) and frame pointer (offset 0)
	sd ra, 8(sp)
	sd s0, 0(sp)
	addi s0, sp, 16
	// Local vars at offset -16(s0)
	// End of stack frame
	.global TypeIsStructOrUnion
	call        TypeIsStructOrUnion

	// *** Basic block 1

	not         a0, a0

	// *** Basic block 2

.TypeIsScalar_label_11:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeIsScalar:
	.size TypeIsScalar, .func_end_TypeIsScalar-TypeIsScalar

	.global TypeIsVoidPointer
	.type TypeIsVoidPointer, @function

TypeIsVoidPointer:

	// *** Basic block 0

	.global TypeIsVoid
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
	mv          t0, s1
	lw          t1, 16(t0)
	addi        t1, t1, -1
	seqz        t2, t1

	// *** Basic block 1

.TypeIsVoidPointer_label_21:
	mv          a0, t2
	beqz        t2, .TypeIsVoidPointer_label_30

	// *** Basic block 2

	j           .TypeIsVoidPointer_label_25

	// *** Basic block 3

.TypeIsVoidPointer_label_25:
	ld          t0, 24(s1)
	sub         t0, t0, x0
	snez        a0, t0

	// *** Basic block 4

.TypeIsVoidPointer_label_30:
	beqz        a0, .TypeIsVoidPointer_label_37

	// *** Basic block 5

	ld          a0, 24(s1)
	call        TypeIsVoid

	// *** Basic block 7

.TypeIsVoidPointer_label_37:

	// *** Basic block 8

.TypeIsVoidPointer_label_39:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeIsVoidPointer:
	.size TypeIsVoidPointer, .func_end_TypeIsVoidPointer-TypeIsVoidPointer

	.global TypeIsArray
	.type TypeIsArray, @function

TypeIsArray:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 16(a0)
	addi        t0, t0, -2
	seqz        a0, t0

	// *** Basic block 1

.TypeIsArray_label_14:
	ret         
.func_end_TypeIsArray:
	.size TypeIsArray, .func_end_TypeIsArray-TypeIsArray

	.global TypeIsConst
	.type TypeIsConst, @function

TypeIsConst:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 12(a0)
	andi        t0, t0, 4
	snez        a0, t0

	// *** Basic block 1

.TypeIsConst_label_14:
	ret         
.func_end_TypeIsConst:
	.size TypeIsConst, .func_end_TypeIsConst-TypeIsConst

	.global TypeIsVolatile
	.type TypeIsVolatile, @function

TypeIsVolatile:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	bne         t0, x0, .TypeIsVolatile_label_17

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.TypeIsVolatile_label_14:
	ret         

	// *** Basic block 3

.TypeIsVolatile_label_17:
	lw          t1, 12(t0)
	andi        t1, t1, 8
	snez        a0, t1
	ret         
.func_end_TypeIsVolatile:
	.size TypeIsVolatile, .func_end_TypeIsVolatile-TypeIsVolatile

	.global TypeIsEnum
	.type TypeIsEnum, @function

TypeIsEnum:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 1

.TypeIsEnum_label_17:
	beqz        t3, .TypeIsEnum_label_27

	// *** Basic block 2

	j           .TypeIsEnum_label_21

	// *** Basic block 3

.TypeIsEnum_label_21:
	lw          t1, 8(t0)
	li          t2, 4096		// 0x1000
	and         t1, t1, t2
	snez        a0, t1

	// *** Basic block 4

.TypeIsEnum_label_27:

	// *** Basic block 5

.TypeIsEnum_label_29:
	ret         
.func_end_TypeIsEnum:
	.size TypeIsEnum, .func_end_TypeIsEnum-TypeIsEnum

	.global TypeIsUnsigned
	.type TypeIsUnsigned, @function

TypeIsUnsigned:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	bne         t0, x0, .TypeIsUnsigned_label_18

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.TypeIsUnsigned_label_15:
	ret         

	// *** Basic block 3

.TypeIsUnsigned_label_18:
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 4

.TypeIsUnsigned_label_26:
	beqz        t3, .TypeIsUnsigned_label_36

	// *** Basic block 5

	j           .TypeIsUnsigned_label_30

	// *** Basic block 6

.TypeIsUnsigned_label_30:
	lw          t1, 8(t0)
	li          t2, 16384		// 0x4000
	and         t1, t1, t2
	snez        a0, t1

	// *** Basic block 7

.TypeIsUnsigned_label_36:
	ret         
.func_end_TypeIsUnsigned:
	.size TypeIsUnsigned, .func_end_TypeIsUnsigned-TypeIsUnsigned

	.global TypeIsSigned
	.type TypeIsSigned, @function

TypeIsSigned:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, t0
	lw          t2, 16(t1)
	seqz        t3, t2

	// *** Basic block 1

.TypeIsSigned_label_17:
	beqz        t3, .TypeIsSigned_label_27

	// *** Basic block 2

	j           .TypeIsSigned_label_21

	// *** Basic block 3

.TypeIsSigned_label_21:
	lw          t1, 8(t0)
	li          t2, 8192		// 0x2000
	and         t1, t1, t2
	snez        a0, t1

	// *** Basic block 4

.TypeIsSigned_label_27:

	// *** Basic block 5

.TypeIsSigned_label_29:
	ret         
.func_end_TypeIsSigned:
	.size TypeIsSigned, .func_end_TypeIsSigned-TypeIsSigned

	.global TypeIsIntConstant
	.type TypeIsIntConstant, @function

TypeIsIntConstant:

	// *** Basic block 0

	.global TypeIsIntegral
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
	call        TypeIsIntegral

	// *** Basic block 1

	beqz        a0, .TypeIsIntConstant_label_20

	// *** Basic block 2

	lw          t0, 12(s1)
	andi        t0, t0, 4
	snez        a0, t0

	// *** Basic block 3

.TypeIsIntConstant_label_20:

	// *** Basic block 4

.TypeIsIntConstant_label_22:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeIsIntConstant:
	.size TypeIsIntConstant, .func_end_TypeIsIntConstant-TypeIsIntConstant

	.global TypeIsFloatingPointConstant
	.type TypeIsFloatingPointConstant, @function

TypeIsFloatingPointConstant:

	// *** Basic block 0

	.global TypeIsFloatingPoint
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
	call        TypeIsFloatingPoint

	// *** Basic block 1

	beqz        a0, .TypeIsFloatingPointConstant_label_20

	// *** Basic block 2

	lw          t0, 12(s1)
	andi        t0, t0, 4
	snez        a0, t0

	// *** Basic block 3

.TypeIsFloatingPointConstant_label_20:

	// *** Basic block 4

.TypeIsFloatingPointConstant_label_22:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TypeIsFloatingPointConstant:
	.size TypeIsFloatingPointConstant, .func_end_TypeIsFloatingPointConstant-TypeIsFloatingPointConstant

	.global TypeIsUnknown
	.type TypeIsUnknown, @function

TypeIsUnknown:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 8(a0)
	li          t1, 32768		// 0x8000
	and         t0, t0, t1
	snez        a0, t0

	// *** Basic block 1

.TypeIsUnknown_label_15:
	ret         
.func_end_TypeIsUnknown:
	.size TypeIsUnknown, .func_end_TypeIsUnknown-TypeIsUnknown

	.global TypeIsFixedArray
	.type TypeIsFixedArray, @function

TypeIsFixedArray:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	lw          t1, 16(t0)
	addi        t2, t1, -2
	seqz        a0, t2
	li          t2, 2		// 0x2 ASCII \x2
	bne         t1, t2, .TypeIsFixedArray_label_26

	// *** Basic block 1

	addi        t1, t0, 32
	lb          t1, 16(t1)
	slli        t1, t1, 61
	srai        t1, t1, 63
	not         a0, t1

	// *** Basic block 2

.TypeIsFixedArray_label_26:

	// *** Basic block 3

.TypeIsFixedArray_label_28:
	ret         
.func_end_TypeIsFixedArray:
	.size TypeIsFixedArray, .func_end_TypeIsFixedArray-TypeIsFixedArray

	.global TypeIsVLA
	.type TypeIsVLA, @function

TypeIsVLA:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	lw          t1, 16(t0)
	addi        t2, t1, -2
	seqz        a0, t2
	li          t2, 2		// 0x2 ASCII \x2
	beq         t1, t2, .TypeIsVLA_label_25

	// *** Basic block 1

	addi        t1, t1, -1
	seqz        a0, t1

	// *** Basic block 2

.TypeIsVLA_label_25:
	beqz        a0, .TypeIsVLA_label_32

	// *** Basic block 3

	addi        t1, t0, 32
	lb          t1, 16(t1)
	slli        t1, t1, 61
	srai        a0, t1, 63

	// *** Basic block 4

.TypeIsVLA_label_32:

	// *** Basic block 5

.TypeIsVLA_label_34:
	ret         
.func_end_TypeIsVLA:
	.size TypeIsVLA, .func_end_TypeIsVLA-TypeIsVLA

	.local  FunctionPrototypesEqual
	.type FunctionPrototypesEqual, @function

FunctionPrototypesEqual:

	// *** Basic block 0

	.global TypeEqual
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
	addi        t0, a0, 8
	ld          s1, 8(t0)
	addi        t0, a1, 8
	ld          t0, 8(t0)
	beq         s1, t0, .FunctionPrototypesEqual_label_32

	// *** Basic block 1

	ld          t0, 8(a0)
	ld          t1, 8(a1)
	mv          a0, x0

	// *** Basic block 2

.FunctionPrototypesEqual_label_29:
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

.FunctionPrototypesEqual_label_32:
	mv          s2, x0
	bge         x0, s1, .FunctionPrototypesEqual_label_63

	// *** Basic block 4

.FunctionPrototypesEqual_label_37:
	slli        t0, s2, 3
	add         t1, t0, t0
	ld          s3, 0(t1)
	add         t0, t1, t0
	ld          s4, 0(t0)
	ld          a0, 40(s3)
	ld          a1, 40(s4)
	call        TypeEqual

	// *** Basic block 5

	not         t0, a0
	beqz        t0, .FunctionPrototypesEqual_label_58

	// *** Basic block 6

	mv          a0, x0
	j           .FunctionPrototypesEqual_label_29

	// *** Basic block 7

.FunctionPrototypesEqual_label_58:

	// *** Basic block 8

.FunctionPrototypesEqual_label_59:
	addi        s2, s2, 1
	bge         s2, s1, .FunctionPrototypesEqual_label_37

	// *** Basic block 9

.FunctionPrototypesEqual_label_63:
	li          a0, 1		// 0x1 ASCII \x1
	j           .FunctionPrototypesEqual_label_29
.func_end_FunctionPrototypesEqual:
	.size FunctionPrototypesEqual, .func_end_FunctionPrototypesEqual-FunctionPrototypesEqual

	.global TypeEqual
	.type TypeEqual, @function

TypeEqual:

	// *** Basic block 0

	.global TypeEqual
	.local FunctionPrototypesEqual
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
	lw          t0, 16(s1)
	lw          t1, 16(s2)
	beq         t0, t1, .TypeEqual_label_30

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.TypeEqual_label_27:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.TypeEqual_label_30:
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 4

	j           .TypeEqual_label_97

	// *** Basic block 5

	j           .TypeEqual_label_62

	// *** Basic block 6

	j           .TypeEqual_label_40

	// *** Basic block 7

	j           .TypeEqual_label_74

	// *** Basic block 8

.TypeEqual_label_40:
	ld          a0, 24(s1)
	ld          a1, 24(s2)
	call        TypeEqual

	// *** Basic block 9

	not         t0, a0
	beqz        t0, .TypeEqual_label_53

	// *** Basic block 10

	mv          a0, x0
	j           .TypeEqual_label_27

	// *** Basic block 11

.TypeEqual_label_53:
	lw          t0, 32(s1)
	lw          t1, 32(s2)
	sub         t0, t0, t1
	seqz        a0, t0
	j           .TypeEqual_label_27

	// *** Basic block 12

.TypeEqual_label_62:
	ld          a0, 24(s1)
	ld          a1, 24(s2)
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           TypeEqual

	// *** Basic block 14

.TypeEqual_label_74:
	ld          a0, 24(s1)
	ld          a1, 24(s2)
	call        TypeEqual

	// *** Basic block 15

	not         t0, a0
	beqz        t0, .TypeEqual_label_87

	// *** Basic block 16

	mv          a0, x0
	j           .TypeEqual_label_27

	// *** Basic block 17

.TypeEqual_label_87:
	addi        a0, s1, 32
	addi        a1, s2, 32
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           FunctionPrototypesEqual

	// *** Basic block 19

.TypeEqual_label_97:
	lw          t0, 8(s1)
	lw          t1, 8(s2)
	sub         t2, t0, t1
	seqz        a0, t2
	bne         t0, t1, .TypeEqual_label_112

	// *** Basic block 20

	lw          t0, 12(s1)
	lw          t1, 12(s2)
	sub         t0, t0, t1
	seqz        a0, t0

	// *** Basic block 21

.TypeEqual_label_112:
	j           .TypeEqual_label_27
.func_end_TypeEqual:
	.size TypeEqual, .func_end_TypeEqual-TypeEqual

	.global TypeAssignmentCompatible
	.type TypeAssignmentCompatible, @function

TypeAssignmentCompatible:

	// *** Basic block 0

	.global TypeEqual
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
	mv          a1, s2
	mv          a0, s1
	call        TypeEqual

	// *** Basic block 1

	beqz        a0, .TypeAssignmentCompatible_label_27

	// *** Basic block 2

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 3

.TypeAssignmentCompatible_label_24:
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

.TypeAssignmentCompatible_label_27:
	mv          s3, s1
	lw          t0, 16(s3)
	addi        t1, t0, -1
	seqz        s4, t1
	li          t1, 1		// 0x1 ASCII \x1
	beq         t0, t1, .TypeAssignmentCompatible_label_42

	// *** Basic block 5

	addi        t0, t0, -2
	seqz        s4, t0

	// *** Basic block 6

.TypeAssignmentCompatible_label_42:

	// *** Basic block 7

.TypeAssignmentCompatible_label_44:
	beqz        s4, .TypeAssignmentCompatible_label_67

	// *** Basic block 8

	j           .TypeAssignmentCompatible_label_47

	// *** Basic block 9

.TypeAssignmentCompatible_label_47:
	ld          t0, 24(s1)
	lw          t0, 12(t0)
	andi        s3, t0, -5
	ld          t0, 24(s2)
	lw          t0, 12(t0)
	andi        s4, t0, -5
	bne         s3, s4, .TypeAssignmentCompatible_label_66

	// *** Basic block 10

	li          a0, 1		// 0x1 ASCII \x1
	j           .TypeAssignmentCompatible_label_24

	// *** Basic block 11

.TypeAssignmentCompatible_label_66:

	// *** Basic block 12

.TypeAssignmentCompatible_label_67:
	mv          a0, x0
	j           .TypeAssignmentCompatible_label_24
.func_end_TypeAssignmentCompatible:
	.size TypeAssignmentCompatible, .func_end_TypeAssignmentCompatible-TypeAssignmentCompatible

	.global TypeEqualIgnoringSign
	.type TypeEqualIgnoringSign, @function

TypeEqualIgnoringSign:

	// *** Basic block 0

	.global TypeEqual
	.local FunctionPrototypesEqual
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
	lw          t0, 16(s1)
	lw          t1, 16(s2)
	beq         t0, t1, .TypeEqualIgnoringSign_label_30

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.TypeEqualIgnoringSign_label_27:
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

.TypeEqualIgnoringSign_label_30:
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 4

	j           .TypeEqualIgnoringSign_label_98

	// *** Basic block 5

	j           .TypeEqualIgnoringSign_label_63

	// *** Basic block 6

	j           .TypeEqualIgnoringSign_label_41

	// *** Basic block 7

	j           .TypeEqualIgnoringSign_label_75

	// *** Basic block 8

.TypeEqualIgnoringSign_label_41:
	ld          a0, 24(s1)
	ld          a1, 24(s2)
	call        TypeEqual

	// *** Basic block 9

	not         t0, a0
	beqz        t0, .TypeEqualIgnoringSign_label_54

	// *** Basic block 10

	mv          a0, x0
	j           .TypeEqualIgnoringSign_label_27

	// *** Basic block 11

.TypeEqualIgnoringSign_label_54:
	lw          t0, 32(s1)
	lw          t1, 32(s2)
	sub         t0, t0, t1
	seqz        a0, t0
	j           .TypeEqualIgnoringSign_label_27

	// *** Basic block 12

.TypeEqualIgnoringSign_label_63:
	ld          a0, 24(s1)
	ld          a1, 24(s2)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           TypeEqual

	// *** Basic block 14

.TypeEqualIgnoringSign_label_75:
	ld          a0, 24(s1)
	ld          a1, 24(s2)
	call        TypeEqual

	// *** Basic block 15

	not         t0, a0
	beqz        t0, .TypeEqualIgnoringSign_label_88

	// *** Basic block 16

	mv          a0, x0
	j           .TypeEqualIgnoringSign_label_27

	// *** Basic block 17

.TypeEqualIgnoringSign_label_88:
	addi        a0, s1, 32
	addi        a1, s2, 32
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           FunctionPrototypesEqual

	// *** Basic block 19

.TypeEqualIgnoringSign_label_98:
	lw          t0, 8(s1)
	li          t1, -24577		// 0xffffffffffff9fff
	and         s3, t0, t1
	lw          t0, 8(s2)
	and         s4, t0, t1
	sub         t0, s3, s4
	seqz        a0, t0
	j           .TypeEqualIgnoringSign_label_27
.func_end_TypeEqualIgnoringSign:
	.size TypeEqualIgnoringSign, .func_end_TypeEqualIgnoringSign-TypeEqualIgnoringSign

	.local  FunctionPrototypesDetails
	.type FunctionPrototypesDetails, @function

FunctionPrototypesDetails:

	// *** Basic block 0

	.global DecodeSourceLocation
	.global ReportNote
	.global TypeEqual
	.global TypeErrorDetails
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
	mv          s2, a1
	mv          s3, a2
	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	call        DecodeSourceLocation

	// *** Basic block 1

	ld          s4, -48(s0)
	lw          s5, -40(s0)
	addi        t0, s2, 8
	ld          s6, 8(t0)
	addi        t0, s3, 8
	ld          s7, 8(t0)
	beq         s6, s7, .FunctionPrototypesDetails_label_69

	// *** Basic block 2

	ld          s8, 8(s2)
	ld          s9, 8(s3)
	ld          a0, -48(s0)
	lw          a1, -40(s0)
	lla         a2, .str.90
	mv          a4, s7
	mv          a3, s6
	call        ReportNote

	// *** Basic block 3

.FunctionPrototypesDetails_label_66:
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

	// *** Basic block 4

.FunctionPrototypesDetails_label_69:
	mv          s7, x0
	bge         x0, s6, .FunctionPrototypesDetails_label_116

	// *** Basic block 5

.FunctionPrototypesDetails_label_74:
	slli        t0, s7, 3
	add         t1, s8, t0
	ld          s8, 0(t1)
	add         t0, s9, t0
	ld          s9, 0(t0)
	ld          s8, 40(s8)
	ld          s9, 40(s9)
	mv          a1, s9
	mv          a0, s8
	call        TypeEqual

	// *** Basic block 6

	not         t0, a0
	beqz        t0, .FunctionPrototypesDetails_label_111

	// *** Basic block 7

	mv          a2, s9
	mv          a1, s8
	mv          a0, s1
	call        TypeErrorDetails

	// *** Basic block 8

	lla         a2, .str.91
	addi        a3, s7, 1
	mv          a1, s5
	mv          a0, s4
	call        ReportNote

	// *** Basic block 9

.FunctionPrototypesDetails_label_111:

	// *** Basic block 10

.FunctionPrototypesDetails_label_112:
	addi        s7, s7, 1
	bge         s7, s6, .FunctionPrototypesDetails_label_74

	// *** Basic block 11

.FunctionPrototypesDetails_label_116:
	j           .FunctionPrototypesDetails_label_66
.func_end_FunctionPrototypesDetails:
	.size FunctionPrototypesDetails, .func_end_FunctionPrototypesDetails-FunctionPrototypesDetails

	.global TypeErrorDetails
	.type TypeErrorDetails, @function

TypeErrorDetails:

	// *** Basic block 0

	.global TypeRecordToString
	.global DecodeSourceLocation
	.global ReportNote
	.global TypeErrorDetails
	.local FunctionPrototypesDetails
	.global StringDestruct
	addi sp, sp, -160
	// Saved return address (offset 152) and frame pointer (offset 144)
	sd ra, 152(sp)
	sd s0, 144(sp)
	addi s0, sp, 160
	// Local vars at offset -128(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a2
	mv          s3, a0
	sd          x0, -128(s0)
	sd          x0, -120(s0)
	sd          x0, -112(s0)
	sd          x0, -104(s0)
	sd          x0, -96(s0)
	sb          x0, -128(s0)
	sd          x0, -88(s0)
	sd          x0, -80(s0)
	sd          x0, -72(s0)
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sb          x0, -88(s0)
	addi        a1, s0, -128
	mv          a0, s1
	call        TypeRecordToString

	// *** Basic block 1

	addi        a1, s0, -88
	mv          a0, s2
	call        TypeRecordToString

	// *** Basic block 2

	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	mv          a0, s3
	call        DecodeSourceLocation

	// *** Basic block 3

	lw          s4, 16(s1)
	lw          t0, 16(s2)
	beq         s4, t0, .TypeErrorDetails_label_106

	// *** Basic block 4

	ld          a0, -48(s0)
	lw          a1, -40(s0)
	lla         a2, .str.92
	addi        t0, s0, -128
	ld          a3, 16(t0)
	addi        t0, s0, -88
	ld          a4, 16(t0)
	call        ReportNote

	// *** Basic block 5

.TypeErrorDetails_label_103:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.TypeErrorDetails_label_106:
	slli        t0, s4, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 7

	j           .TypeErrorDetails_label_180

	// *** Basic block 8

	j           .TypeErrorDetails_label_117

	// *** Basic block 9

	j           .TypeErrorDetails_label_116

	// *** Basic block 10

	j           .TypeErrorDetails_label_144

	// *** Basic block 11

.TypeErrorDetails_label_116:

	// *** Basic block 12

.TypeErrorDetails_label_117:
	ld          a0, -48(s0)
	lw          a1, -40(s0)
	lla         a2, .str.93
	addi        t0, s0, -128
	ld          a3, 16(t0)
	addi        t0, s0, -88
	ld          a4, 16(t0)
	call        ReportNote

	// *** Basic block 13

	ld          a1, 24(s1)
	ld          a2, 24(s2)
	mv          a0, s3
	call        TypeErrorDetails

	// *** Basic block 14

	j           .TypeErrorDetails_label_214

	// *** Basic block 15

.TypeErrorDetails_label_144:
	ld          a0, -48(s0)
	lw          a1, -40(s0)
	lla         a2, .str.94
	addi        t0, s0, -128
	ld          a3, 16(t0)
	addi        t0, s0, -88
	ld          a4, 16(t0)
	call        ReportNote

	// *** Basic block 16

	ld          a1, 24(s1)
	ld          a2, 24(s2)
	mv          a0, s3
	call        TypeErrorDetails

	// *** Basic block 17

	addi        a1, s1, 32
	addi        a2, s2, 32
	mv          a0, s3
	call        FunctionPrototypesDetails

	// *** Basic block 18

	j           .TypeErrorDetails_label_103

	// *** Basic block 19

.TypeErrorDetails_label_180:
	lw          t1, 8(s1)
	lw          t2, 8(s2)
	sub         t3, t1, t2
	snez        t0, t3
	bne         t1, t2, .TypeErrorDetails_label_195

	// *** Basic block 20

	lw          t1, 12(s1)
	lw          t2, 12(s2)
	sub         t1, t1, t2
	snez        t0, t1

	// *** Basic block 21

.TypeErrorDetails_label_195:
	beqz        t0, .TypeErrorDetails_label_213

	// *** Basic block 22

	ld          a0, -48(s0)
	lw          a1, -40(s0)
	lla         a2, .str.95
	addi        t0, s0, -128
	ld          a3, 16(t0)
	addi        t0, s0, -88
	ld          a4, 16(t0)
	call        ReportNote

	// *** Basic block 23

.TypeErrorDetails_label_213:

	// *** Basic block 24

.TypeErrorDetails_label_214:
	addi        a0, s0, -128
	call        StringDestruct

	// *** Basic block 25

	addi        a0, s0, -128
	call        StringDestruct

	// *** Basic block 26

	j           .TypeErrorDetails_label_103
.func_end_TypeErrorDetails:
	.size TypeErrorDetails, .func_end_TypeErrorDetails-TypeErrorDetails

.PCend:
	.data
next_type_id:
	.type   next_type_id,@object
	.local  next_type_id
	.size   next_type_id,4
	.p2align  2
	.word   0

type_sizes:
	.type   type_sizes,@object
	.local  type_sizes
	.size   type_sizes,96
	.p2align  3
	.word   1
	.word   1
	.word   2
	.word   4
	.word   4
	.word   2
	.word   8
	.word   -1
	.word   16
	.word   8
	.word   32
	.word   4
	.word   64
	.word   8
	.word   128
	.word   8
	.word   512
	.word   0
	.word   256
	.word   1
	.word   4096
	.word   4
	.word   0
	.word   0

type_names:
	.type   type_names,@object
	.local  type_names
	.size   type_names,224
	.p2align  3
	.word   1
	.space  4
	.long    .str.5
	.word   4
	.space  4
	.long    .str.6
	.word   8
	.space  4
	.long    .str.7
	.word   16
	.space  4
	.long    .str.8
	.word   2
	.space  4
	.long    .str.9
	.word   32
	.space  4
	.long    .str.10
	.word   64
	.space  4
	.long    .str.11
	.word   128
	.space  4
	.long    .str.12
	.word   1024
	.space  4
	.long    .str.13
	.word   2048
	.space  4
	.long    .str.14
	.word   512
	.space  4
	.long    .str.15
	.word   256
	.space  4
	.long    .str.16
	.word   4096
	.space  4
	.long    .str.17
	.word   0
	.space  4
	.long    .str.18

type_map:
	.type   type_map,@object
	.local  type_map
	.size   type_map,112
	.p2align  3
	.word   59
	.word   1
	.word   76
	.word   2
	.word   81
	.word   4
	.word   77
	.word   8
	.word   70
	.word   32
	.word   65
	.word   64
	.word   85
	.word   1024
	.word   89
	.word   2048
	.word   67
	.word   4096
	.word   91
	.word   512
	.word   57
	.word   256
	.word   82
	.word   8192
	.word   90
	.word   16384
	.word   0
	.word   0

valid_types:
	.type   valid_types,@object
	.local  valid_types
	.size   valid_types,136
	.p2align  2
	.word   512
	.word   1
	.word   8193
	.word   16385
	.word   4
	.word   8196
	.word   6
	.word   8198
	.word   16388
	.word   16390
	.word   2
	.word   8194
	.word   8192
	.word   16384
	.word   16386
	.word   8
	.word   10
	.word   8200
	.word   8202
	.word   16392
	.word   16394
	.word   16
	.word   8208
	.word   18
	.word   8210
	.word   16400
	.word   16402
	.word   32
	.word   64
	.word   72
	.word   256
	.word   1024
	.word   2048
	.word   4096

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.1, @object
	.size .str.1, 30

.str.2:
	.asciz "type.c"
	.type .str.2, @object
	.size .str.2, 7

.str.3:
	.asciz "record->info.struct_info != NULL"
	.type .str.3, @object
	.size .str.3, 33

.str.4:
	.asciz "(null)"
	.type .str.4, @object
	.size .str.4, 1

.str.5:
	.asciz "char"
	.type .str.5, @object
	.size .str.5, 5

.str.6:
	.asciz "short"
	.type .str.6, @object
	.size .str.6, 6

.str.7:
	.asciz "long"
	.type .str.7, @object
	.size .str.7, 5

.str.8:
	.asciz "long long"
	.type .str.8, @object
	.size .str.8, 10

.str.9:
	.asciz "int"
	.type .str.9, @object
	.size .str.9, 4

.str.10:
	.asciz "float"
	.type .str.10, @object
	.size .str.10, 6

.str.11:
	.asciz "double"
	.type .str.11, @object
	.size .str.11, 7

.str.12:
	.asciz "long double"
	.type .str.12, @object
	.size .str.12, 12

.str.13:
	.asciz "struct"
	.type .str.13, @object
	.size .str.13, 7

.str.14:
	.asciz "union"
	.type .str.14, @object
	.size .str.14, 6

.str.15:
	.asciz "void"
	.type .str.15, @object
	.size .str.15, 5

.str.16:
	.asciz "bool"
	.type .str.16, @object
	.size .str.16, 5

.str.17:
	.asciz "enum"
	.type .str.17, @object
	.size .str.17, 5

.str.18:
	.asciz "(null)"
	.type .str.18, @object
	.size .str.18, 1

.str.19:
	.asciz "signed "
	.type .str.19, @object
	.size .str.19, 8

.str.20:
	.asciz "unsigned "
	.type .str.20, @object
	.size .str.20, 10

.str.21:
	.asciz " "
	.type .str.21, @object
	.size .str.21, 2

.str.22:
	.asciz "const "
	.type .str.22, @object
	.size .str.22, 7

.str.23:
	.asciz "volatile "
	.type .str.23, @object
	.size .str.23, 10

.str.24:
	.asciz "restrict "
	.type .str.24, @object
	.size .str.24, 10

.str.25:
	.asciz "pointer to "
	.type .str.25, @object
	.size .str.25, 12

.str.26:
	.asciz "%s"
	.type .str.26, @object
	.size .str.26, 3

.str.27:
	.asciz "variable length array "
	.type .str.27, @object
	.size .str.27, 23

.str.28:
	.asciz "array of size %d "
	.type .str.28, @object
	.size .str.28, 18

.str.29:
	.asciz "function ("
	.type .str.29, @object
	.size .str.29, 11

.str.30:
	.asciz "(null)"
	.type .str.30, @object
	.size .str.30, 1

.str.31:
	.asciz "%s"
	.type .str.31, @object
	.size .str.31, 3

.str.32:
	.asciz ","
	.type .str.32, @object
	.size .str.32, 2

.str.33:
	.asciz "%s..."
	.type .str.33, @object
	.size .str.33, 6

.str.34:
	.asciz ") {"
	.type .str.34, @object
	.size .str.34, 4

.str.35:
	.asciz "\n"
	.type .str.35, @object
	.size .str.35, 2

.str.36:
	.asciz "} returning "
	.type .str.36, @object
	.size .str.36, 13

.str.37:
	.asciz ") returning "
	.type .str.37, @object
	.size .str.37, 13

.str.38:
	.asciz "\n"
	.type .str.38, @object
	.size .str.38, 2

.str.39:
	.asciz "(*"
	.type .str.39, @object
	.size .str.39, 3

.str.40:
	.asciz "[%d]"
	.type .str.40, @object
	.size .str.40, 5

.str.41:
	.asciz "(null)"
	.type .str.41, @object
	.size .str.41, 1

.str.42:
	.asciz ","
	.type .str.42, @object
	.size .str.42, 2

.str.43:
	.asciz "(null)"
	.type .str.43, @object
	.size .str.43, 1

.str.44:
	.asciz "and "
	.type .str.44, @object
	.size .str.44, 5

.str.45:
	.asciz "Invalid type combination; can\'t combine %s"
	.type .str.45, @object
	.size .str.45, 43

.str.46:
	.asciz "defined type "
	.type .str.46, @object
	.size .str.46, 14

.str.47:
	.asciz "and "
	.type .str.47, @object
	.size .str.47, 5

.str.48:
	.asciz "Invalid type combination; can\'t combine %s"
	.type .str.48, @object
	.size .str.48, 43

.str.49:
	.asciz "defined type "
	.type .str.49, @object
	.size .str.49, 14

.str.50:
	.asciz "and defined type "
	.type .str.50, @object
	.size .str.50, 18

.str.51:
	.asciz "Invalid type combination; can\'t combine %s"
	.type .str.51, @object
	.size .str.51, 43

.str.52:
	.asciz "(null)"
	.type .str.52, @object
	.size .str.52, 1

.str.53:
	.asciz "and "
	.type .str.53, @object
	.size .str.53, 5

.str.54:
	.asciz "Invalid type combination; can\'t combine %s"
	.type .str.54, @object
	.size .str.54, 43

.str.55:
	.asciz "Type expected"
	.type .str.55, @object
	.size .str.55, 14

.str.56:
	.asciz "Invalid pointer qualifier declaration"
	.type .str.56, @object
	.size .str.56, 38

.str.57:
	.asciz "Duplicate function argument \'%s\'"
	.type .str.57, @object
	.size .str.57, 33

.str.58:
	.asciz "Cannot mix function prototype with old-style function args"
	.type .str.58, @object
	.size .str.58, 59

.str.59:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.59, @object
	.size .str.59, 30

.str.60:
	.asciz "type.c"
	.type .str.60, @object
	.size .str.60, 7

.str.61:
	.asciz "formal != NULL"
	.type .str.61, @object
	.size .str.61, 15

.str.62:
	.asciz "Type expected for function arg"
	.type .str.62, @object
	.size .str.62, 31

.str.63:
	.asciz "Expected type or identifier in function prototype"
	.type .str.63, @object
	.size .str.63, 50

.str.64:
	.asciz "... must be at the end of a function prototype"
	.type .str.64, @object
	.size .str.64, 47

.str.65:
	.asciz "static or qualifiers used in array declarator outside function prototype"
	.type .str.65, @object
	.size .str.65, 73

.str.66:
	.asciz "Array dimension required after first dimension"
	.type .str.66, @object
	.size .str.66, 47

.str.67:
	.asciz "VLA placeholder \'*\' is only valid in a function prototype"
	.type .str.67, @object
	.size .str.67, 58

.str.68:
	.asciz "Variable length array size must be integral"
	.type .str.68, @object
	.size .str.68, 44

.str.69:
	.asciz "Variable length array is only allowed inside a function"
	.type .str.69, @object
	.size .str.69, 56

.str.70:
	.asciz "Variable length array cannot be static or extern"
	.type .str.70, @object
	.size .str.70, 49

.str.71:
	.asciz "Array with negative or zero size"
	.type .str.71, @object
	.size .str.71, 33

.str.72:
	.asciz "Missing ]"
	.type .str.72, @object
	.size .str.72, 10

.str.73:
	.asciz "Missing close parenthesis in declaration"
	.type .str.73, @object
	.size .str.73, 41

.str.74:
	.asciz "%s"
	.type .str.74, @object
	.size .str.74, 3

.str.75:
	.asciz "constant expression needed"
	.type .str.75, @object
	.size .str.75, 27

.str.76:
	.asciz "constant expression needed"
	.type .str.76, @object
	.size .str.76, 27

.str.77:
	.asciz "only integer types can be used for bitfields"
	.type .str.77, @object
	.size .str.77, 45

.str.78:
	.asciz "width of %" PRId64 " is out of bounds for type of size %d bite"
	.type .str.78, @object
	.size .str.78, 56

.str.79:
	.asciz "Invalid bitfield; %s"
	.type .str.79, @object
	.size .str.79, 21

.str.80:
	.asciz "Invalid type for struct member"
	.type .str.80, @object
	.size .str.80, 31

.str.81:
	.asciz "Duplicate struct/union member %s"
	.type .str.81, @object
	.size .str.81, 33

.str.82:
	.asciz "No flexible arrays allowed in unions"
	.type .str.82, @object
	.size .str.82, 37

.str.83:
	.asciz "Flexible array \'%s\' cannot be the only member in a struct"
	.type .str.83, @object
	.size .str.83, 58

.str.84:
	.asciz "Flexible array \'%s\' needs to be the last member in a struct"
	.type .str.84, @object
	.size .str.84, 60

.str.85:
	.asciz "Tag %s declared with different tag type"
	.type .str.85, @object
	.size .str.85, 40

.str.86:
	.asciz "Duplicate definition of struct/union %s"
	.type .str.86, @object
	.size .str.86, 40

.str.87:
	.asciz "Constant integer expression required for value of enum constant %s"
	.type .str.87, @object
	.size .str.87, 67

.str.88:
	.asciz "Enum constant %s is already defined in this scope"
	.type .str.88, @object
	.size .str.88, 50

.str.89:
	.asciz "Duplicate definition of enum %s"
	.type .str.89, @object
	.size .str.89, 32

.str.90:
	.asciz "Different number of arguments: %zd vs %zd"
	.type .str.90, @object
	.size .str.90, 42

.str.91:
	.asciz "  for argument #%zd"
	.type .str.91, @object
	.size .str.91, 20

.str.92:
	.asciz "Declarators \'%s\' and \'%s\' are different"
	.type .str.92, @object
	.size .str.92, 40

.str.93:
	.asciz "Declaration of \'%s\' and \'%s\' are different"
	.type .str.93, @object
	.size .str.93, 43

.str.94:
	.asciz "Declaration of \'%s\' and \'%s\' are different"
	.type .str.94, @object
	.size .str.94, 43

.str.95:
	.asciz "Types \'%s\' and \'%s\' are different"
	.type .str.95, @object
	.size .str.95, 34

