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
// Vectors at 0xfd00.  Each is a JMP instruction to an address.
// These are in RAM and can be overwritten by second stage OS.
.set vector_ram 0xfd00
.set irqv 0xfd00
.set nmiv 0xfd03
.set brkv 0xfd06
.set console_writev 0xfd09

// Console buffer.
.set console_input_buffer_addr 0x300

.set temp_buffer 0x400
.set input_ring_buffer 0x500
.set input_buffer 0x600
.set output_buffer 0x700

// Zero page.

// Temporary address.
.set temp_addr 0x00 //,1
.set rega 0x02              // Shadow for REGA.

.set console_input_buffer 0x03 // 0x04

// Temporary register work locations.
.set addrA 0x05 // and 0x06
.set addrB 0x07 // and 0x08
.set addrC 0x09 // and 0x0a

.set byteA 0x0b
.set byteB 0x0c
.set byteC 0x0d

.set device_ptr 0x0e // and 0x0f
.set crc 0x10 // and 0x11
.set crc_addr 0x12 // and 0x13
.set crc_num 0x14   // and 0x15
.set seqnum 0x16

.set ls_count 0x17 // and 0x18
.set dump_addr 0x19 // and 0x20
.set dump_end_addr 0x21 // and 0x22
.set dump_length 0x23 // and 0x24

// Serial input buffer indexes and count.
.set serial_read_index 0x25         // First unread byte.
.set serial_write_index 0x26        // Next byte to write.
.set serial_num_bytes 0x27          // Num bytes available.

.set brk_addr 0x28 // and 0x29

.set decimal_out 0x2a // and 0x2b and 0x2c
.set decimal_in 0x2d // and 0x2e

.set file_length 0x2f // and 0x30

// Function to call for incoming file block.
.set block_handler 0x31 // and 0x32
.set load_addr 0x33 // and 0x34

// Start address of loaded file.
.set exe_addr 0x35 // and 0x36

// ACIA2 interrupt serial data and status, read early on IRQ.
.set serial_input_data 0x37
.set serial_input_status 0x38

// Place to save A and X on IRQ.
.set irq_accum 0xfe
.set irq_x 0xff

#endif /* zeropage_h */
