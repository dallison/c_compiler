//
//  addresses.h
//  c_compiler
//
//  Created by David Allison on 9/29/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef zeropage_h
#define zeropage_h


.set ROM_START 0xc000
.set PERM_ROM 0xff00
.set ROM_VECTORS 0xfffa

// Static memory.
// We have a page of RAM at 0xfd00.  It could be used for vectors or something.
// Vectors at 0xfd00.  Each is a JMP instruction to an address.
// These are in RAM and can be overwritten by second stage OS.
.set vector_ram 0xfd00

// Console buffer.
.set console_input_buffer_addr 0x300
.set temp_buffer 0x380

// Serial input output buffers, 256 bytes long.
.set input_buffer 0x400
.set output_buffer 0x500

// Zero page.  We use some zero page locations for ths ROM.  They
// start above the C ABI's reserved space, at 0x7e.
.set zp_start 0x7e

// Temporary address.
.set temp_addr zp_start + 0x00 //,1
.set rega zp_start + 0x02              // Shadow for REGA.

.set console_input_buffer zp_start + 0x03 // 0x04

// Temporary register work locations.
.set addrA zp_start + 0x05 // and 0x06
.set addrB zp_start + 0x07 // and 0x08
.set addrC zp_start + 0x09 // and 0x0a

.set byteA zp_start + 0x0b
.set byteB zp_start + 0x0c
.set byteC zp_start + 0x0d

.set device_ptr zp_start + 0x0e // and 0x0f
.set crc zp_start + 0x10 // and 0x11
.set crc_addr zp_start + 0x12 // and 0x13
.set crc_num zp_start + 0x14   // and 0x15
.set seqnum zp_start + 0x16

.set ls_count zp_start + 0x17 // and 0x18
.set dump_addr zp_start + 0x19 // and 0x20
.set dump_end_addr zp_start + 0x21 // and 0x22
.set dump_length zp_start + 0x23 // and 0x24

// Serial input buffer indexes and count.
.set serial_read_index zp_start + 0x25         // First unread byte.
.set serial_write_index zp_start + 0x26        // Next byte to write.

.set serial_num_bytes zp_start + 0x27
// 0x28 free

.set decimal_out zp_start + 0x29 // and 0x2a and 0x2b
.set decimal_in zp_start + 0x2c // and 0x2d

.set file_length zp_start + 0x2e // and 0x2f

// Function to call for incoming file block.
.set block_handler zp_start + 0x30 // and 0x31
.set load_addr zp_start + 0x32 // and 0x33

// Start address of loaded file.
.set exe_addr zp_start + 0x34 // and 0x35

// System call scratch space.
.set syscall_vector zp_start + 0x36 // and 0x37
.set syscall_code zp_start + 0x38 // and 0x39
// End is at 0xf4

// IRQ scratch space
.set irq_accum 0xfd
.set irq_x 0xfe
.set irq_y 0xff
#endif /* zeropage_h */
