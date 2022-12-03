	.file   "libc_test.c"
	.text
	.option pic
.PCbegin:
	.global TestStrings
	.type TestStrings, @function

TestStrings:

	// *** Basic block 0

	.global strcpy
	.global strcat
	.global printf
	.global strcmp
	addi sp, sp, -544
	// Saved return address (offset 536) and frame pointer (offset 528)
	sd ra, 536(sp)
	sd s0, 528(sp)
	addi s0, sp, 544
	// Local vars at offset -528(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	addi        s1, s0, -528
	lla         a1, .str.1
	mv          a0, s1
	call        strcpy

	// *** Basic block 1

	addi        s2, s0, -272
	lla         a1, .str.2
	mv          a0, s2
	call        strcpy

	// *** Basic block 2

	lla         a1, .str.3
	mv          a0, s2
	call        strcat

	// *** Basic block 3

	lla         a0, .str.4
	mv          a2, s2
	mv          a1, s1
	call        printf

	// *** Basic block 4

	mv          a1, s2
	mv          a0, s1
	call        strcmp

	// *** Basic block 5

	mv          s1, a0
	lla         a0, .str.5
	lla         a2, .str.6
	mv          a1, s1
	call        printf

	// *** Basic block 6

	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TestStrings:
	.size TestStrings, .func_end_TestStrings-TestStrings

	.global main
	.type main, @function

main:

	// *** Basic block 0

	.global TestStrings
	.global printf
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	call        TestStrings

	// *** Basic block 1

	mv          s1, x0

	// *** Basic block 2

.main_label_13:
	lla         a0, .str.7
	mv          a2, s1
	mv          a1, s1
	call        printf

	// *** Basic block 3

.main_label_25:
	addi        s1, s1, 1
	li          t0, 30		// 0x1e ASCII \x1e
	bge         s1, t0, .main_label_13

	// *** Basic block 4

.main_label_30:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_main:
	.size main, .func_end_main-main

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "hello world"
	.type .str.1, @object
	.size .str.1, 12

.str.2:
	.asciz "hello "
	.type .str.2, @object
	.size .str.2, 7

.str.3:
	.asciz "world"
	.type .str.3, @object
	.size .str.3, 6

.str.4:
	.asciz "buf1: %s, buf2: %s\n"
	.type .str.4, @object
	.size .str.4, 20

.str.5:
	.asciz "%-20d %20sfoo\n"
	.type .str.5, @object
	.size .str.5, 15

.str.6:
	.asciz "foobar"
	.type .str.6, @object
	.size .str.6, 7

.str.7:
	.asciz "test: %08x (%d)\n"
	.type .str.7, @object
	.size .str.7, 17

