//
//  comms.s
//  c_compiler
//
//  Created by David Allison on 10/13/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include "addresses.h"
#include "device.h"
#include "comms.h"

.text


// Calculate an XMODEM 16-bit CRC from data in memory. This code is as
// tight and as fast as it can be, moving as much code out of inner
// loops as possible.
//
// On entry, crc..crc+1   =  incoming CRC
//           addr..addr+1 => start address of data
//           num..num+1   =  number of bytes
// On exit,  crc..crc+1   =  updated CRC
//           addr..addr+1 => end of data+1
//           num..num+1   =  0
//
// Multiple passes over data in memory can be made to update the CRC.
// For XMODEM, initial CRC must be &0000. For Acorn CFS/RFS the CRC
// is stored in the header high-byte/low-byte.
// Opimisation based on Greg Cook's 6502 CRC-32 optimisation.
// Total 47 bytes
//

.global crc16
crc16:
crc_bytelp:
  LDX #8                         // Prepare to rotate CRC 8 bits
  LDA (crc_addr-8 & 0xff,X)          // Fetch byte from memory

  // The following code updates the CRC with the byte in A ---------+
  // If used in isolation, requires LDX #8 here                     |
  EOR crc+1                      // EOR byte into CRC top byte     |
crc_rotlp:                       //
  ASL crc+0
  ROL A                          // Rotate CRC clearing bit 0      |
  BCC crc_clear                  // b15 was clear, skip past       |
  TAY                            // Hold CRC high byte in Y        |
  LDA crc+0
  EOR #0x21
  STA crc+0   // CRC=CRC EOR &1021, XMODEM polynomic
  TYA
  EOR #0x10                      // Get CRC high byte back from Y  |
crc_clear:                       // b15 was zero                   |
  DEX
  BNE crc_rotlp                  // Loop for 8 bits                |
  STA crc+1                      // Store CRC high byte            |

  INC crc_addr+0
  
crc_next:
  // Now do a 16-bit decrement
  LDA crc_num+0
  BNE crc_skip                    // num.lo<>0, not wrapping from 00 to FF
  DEC crc_num+1                       // Wrapping from 00 to FF, dec. high byte
crc_skip:
  DEC crc_num+0
  BNE crc_bytelp                 // Dec. low byte, loop until num.lo=0
  LDA crc_num+1
  BNE crc_bytelp                 // Loop until num=0
  RTS

// A packet is:
.set packet_length 0
.set packet_crc 1
.set packet_data 3

// Y: packet length
// output_buffer: address of packet with data already populated
build_packet:
  STY crc_num       // Length in crc_num (num for crc16)
  STZ crc_num+1     // High byte of crc_num is zero.
  STZ crc           // CRC starts as zero
  STZ crc+1
  INY
  INY
  STY output_buffer+packet_length     // Packet length is length + 2
  
  LDA #(output_buffer & 0xff) + packet_data
  STA crc_addr
  LDA #(output_buffer >> 8)
  STA crc_addr+1
  
  // Calculate CRC of crc_addr with length in crc_num.
  JSR crc16
  
  LDA crc
  STA output_buffer+packet_crc
  LDA crc+1
  STA output_buffer+packet_crc+1
  RTS

// entry:
// input_buffer: address of packet (must be page aligned)
// exit:
// Y: length of data
// Carry: set = packet OK
process_packet:
  LDY input_buffer    // Load length of packet.
  DEY
  DEY
  PHY               // Pushed on to stack for later.
  STY crc_num         // Low byte of length for crc
  STZ crc_num+1       // Zero high byte.
  STZ crc           // CRC initially zero
  STZ crc+1
  LDA #(input_buffer & 0xff) + packet_data
  STA crc_addr
  LDA #(input_buffer >> 8)
  STA crc_addr+1
  
  // Calculate CRC
  JSR crc16
  
  // Compare calculated CRC with packet.
  LDA input_buffer+packet_crc     // CRC low byte.
  CMP crc           // Compare with packet crc
  BNE bad_crc
  LDA input_buffer+packet_crc+1     // CRC high byte.
  CMP crc+1         // Compare.
  BNE bad_crc
  PLY               // Length in Y.
  RTS               // Carry is set.
bad_crc:
  PLY               // Length doesn't matter.
  JSR console_write_string
  .asciz "Bad CRC\r\n"
  LDA input_buffer+packet_crc+1
  JSR print_as_hex
  LDA input_buffer+packet_crc+0
  JSR print_as_hex
  LDA #'/'
  JSR console_write_char
  LDA crc+1
  JSR print_as_hex
  LDA crc+0
  JSR print_as_hex
  JSR console_crlf
  CLC
  RTS

// Y: packet length
// output_buffer: packet
// Carry: set = good
.global send_packet
send_packet:
  JSR build_packet
  LDA #(output_buffer & 0xff)
  STA temp_addr     // Move output_buffer into temp_addr for device_write
  LDA #(output_buffer >> 8)
  STA temp_addr+1
  
  // First send length byte.
  LDY #1                  // One byte.
  LDA #acia2_device_id
  JSR device_write        // Write first byte in output_buffer.

  // Wait for ACK of length.  This will be the inverted value sent.
  LDY #1
  JSR wait_for_input
  JSR next_input_byte
  EOR #0xff
  CMP output_buffer+packet_length   // Compare against length.
  BNE bad_len_ack
  
  // ACK the ACK
  // JSR ack_byte
  
  // Move to first byte of rest of packet.
  INC temp_addr
  
  // Send rest of packet.
  LDY output_buffer+packet_length
  LDA #acia2_device_id
  JSR device_write
  SEC
  RTS

bad_len_ack:
  JSR console_write_string
  .asciz "Protocol error: invalid len ACK\r\n"
  CLC
  RTS
  
// Ackknowledge the receipt of an packet.  The input buffer
// contains the packet.
// Entry:
// input_buffer: packet to ack.
// A: command code
// Exit:
// Y: length of datagram.
// output_buffer: datagram ready to send
.global acknowledge
acknowledge:
  // Datagram header
  STA output_buffer+datagram_data+message_command
  LDA input_buffer+datagram_seqnum
  STA output_buffer+datagram_acknum
  LDA #0                    // seqnum
  STA output_buffer+datagram_seqnum
  LDA #1
  STA output_buffer+datagram_ack
  STZ output_buffer+datagram_data+message_length
  LDY #5      // 3 bytes datagram header + 2 bytes message header.
  RTS
  
  
// Serial input is added to the input ring buffer on an IRQ interrrupt
// from the ACIA.  It uses:
// serial_num_bytes: number of bytes in the ring buffer
// serial_read_index: index of first unread byte
// serial_write_index: index of next byte to write.

// This subroutine waits for the ring buffer to contain the specified
// number of bytes.
// Entry:
// Y: number of bytes to wait for.
// Saves X, corrupts A.
wait_for_input:
  PHX
  // Timeout in X,A
  
#if 0
  TYA
  JSR print_as_hex
  LDA #':'
  JSR console_write_char
#endif

  LDA #255
  LDX #50
wait_for_input_loop:
#if 0
  PHA
  LDA serial_num_bytes
  JSR print_as_hex
  LDA #'/'
  JSR console_write_char
  PLA
#endif
  CPY serial_num_bytes
  BCC input_wait_done
  BEQ input_wait_done
  DEC A
  BNE wfi
  DEX
wfi:
  CPX #0
  BNE wait_for_input_loop
  CMP #0
  BNE wait_for_input_loop
  JSR console_write_string
  .asciz "Timeout waiting for serial data\r\n"
input_wait_done:
  PLX
  RTS

  
// Read next input byte, result in A.
.global next_input_byte
next_input_byte:
  PHX
  LDX serial_read_index
  TXA
  INC A
  AND #0X7F                 // Wrap to buffer size (0x80)
  STA serial_read_index
  LDA input_ring_buffer,X
  DEC serial_num_bytes
  PLX
  RTS


// Read the specified number of bytes from the input ring buffer and
// place them in the input buffer starting at index 1.
// Y: num bytes to read
read_ring_buffer:
  STY byteA                     // Number of bytes to read.
  PHX
  PHY
  LDX serial_read_index         // First unread index.
  LDY #1                        // Start at index 1.
read_ring_loop:
  LDA input_ring_buffer,X       // Load next byte from ring buffer.
  STA input_buffer,Y            // Store in input buffer.
  // JSR print_as_hex
  TXA
  INC A
  AND #0x7f
  TAX
  INY
  DEC serial_num_bytes          // One byte less to read.
  DEC byteA
  BNE read_ring_loop
  STX serial_read_index         // Update read index.
  PLY
  PLX
  RTS
  
// place packet in input_buffer.
// Exit as in process_packet.
.global read_packet
read_packet:
  // Read length of packet.
  LDY #1                  // 1 byte
  JSR wait_for_input
  JSR next_input_byte
  STA input_buffer        // Store in first byte of input buffer.
  PHA
  
  // Send ACK of len (inverted value).
  EOR #0xff
  STA output_buffer       // In first byte of output_buffer.
  LDA #(output_buffer & 0xff)
  STA temp_addr     // Move output_buffer into temp_addr for device_write
  LDA #(output_buffer >> 8)
  STA temp_addr+1
  
  // First send length byte.
  LDY #1                  // One byte.
  LDA #acia2_device_id
  JSR device_write        // Write first byte in output_buffer.
  
  PLY                     // Number of bytes to read.
  JSR wait_for_input      // Wait for this number of bytes.
  JSR read_ring_buffer    // Read ring buffer into input buffer
  JMP process_packet
  
  
// Build a ping command in the output buffer.
// Exit:
// Y: length of packet.
.global build_ping
build_ping:
  // Datagram header
  LDA #1          // seqnum
  STA output_buffer+datagram_seqnum
  LDA #0                    // acknum
  STA output_buffer+datagram_acknum
  STA output_buffer+datagram_ack
  
  LDY #datagram_data
  LDA #ping_command_code   // ping command code
  STA output_buffer,Y            // Command code in packet data
  INY
  LDA #5
  STA output_buffer,Y       // Size is 5
  INY
  LDA #'p'
  STA output_buffer,Y
  INY
  LDA #'i'
  STA output_buffer,Y
  INY
  LDA #'n'
  STA output_buffer,Y
  INY
  LDA #'g'
  STA output_buffer,Y
  INY
  LDA #0
  STA output_buffer,Y
  LDY #10             // 3 bytes header + 7 bytes data
  RTS
  
// Build a list_file command in the output buffer.
.global build_list_files
build_list_files:
  LDA #1          // seqnum
  STA output_buffer+datagram_seqnum
  LDA #0                    // acknum
  STA output_buffer+datagram_acknum
  STA output_buffer+datagram_ack
  
  LDY #datagram_data
  LDA #list_files_command_code   // list_files command code
  STA output_buffer,Y            // Command code in packet data
  
  // Put a filter of "." (2 bytes long).
  INY
  LDA #2
  STA output_buffer,Y       // Size is 2
  INY
  LDA #'.'
  STA output_buffer,Y
  INY
  LDA #0
  STA output_buffer,Y
  LDY #7             // 3 bytes header + 4 bytes data
  RTS


// Entry:
// input_buffer contains packet.
// Y: length of data
// Exit:
// X: low byte of number of file
// Y: high byte
.global parse_list_files_result
parse_list_files_result:
  LDA input_buffer+datagram_data+message_command
  CMP #list_files_result_code
  BNE bad_list_files_result
  LDX input_buffer+datagram_data+message_data   // Low byte
  LDY input_buffer+datagram_data+message_data+1 // High byte
  RTS
bad_list_files_result:
  LDX #0
  LDY #0
  RTS
  

// Entry:
// X,Y: low, high of number of files to expect, one per message.
// Uses ls_count as counter.
.global list_files
list_files:
  STX ls_count
  STY ls_count+1
  TXA
  ORA ls_count+1
  BEQ no_files
list_files_loop:  
  // Read dir_entry message.
  JSR read_packet             // Read packet.
  BCC list_files_error
  
  LDA input_buffer+datagram_data+message_command
  CMP #dir_entry_code
  BNE list_files_error
  
  // Print file name and info to console.
  LDA input_buffer+datagram_data+message_data+dir_entry_type
  CMP #dir_entry_dir
  BEQ isdir
  LDA #'F'
  BNE print_type
isdir:
  LDA #'D'
print_type:
  JSR console_write_char
  LDA #' '
  JSR console_write_char
  
  LDX input_buffer+datagram_data+message_data+dir_entry_length
  LDY input_buffer+datagram_data+message_data+dir_entry_length+1
  JSR print_as_hex2
  
  LDA #' '
  JSR console_write_char

  LDY #datagram_data+message_data+dir_entry_name
lf_name_loop:
  LDA input_buffer,Y
  BEQ lf_name_done
  JSR console_write_char
  INY
  JMP lf_name_loop
lf_name_done:
  JSR console_crlf

  // Acknowledge dir entry.
  LDA #dir_entry_code
  JSR acknowledge
  JSR send_packet

  // Decrement file count.
  LDA ls_count
  BNE lf1
  DEC ls_count+1
lf1:
  DEC ls_count
  
  // Back for another file.
  LDA ls_count
  ORA ls_count+1
  BNE list_files_loop
  RTS

list_files_error:
  JSR console_write_string
  .asciz "Bad dir entry result\r\n"
list_files_done:
  RTS
  
no_files:
  JSR console_write_string
  .asciz "No files\r\n"
  RTS
  

// temp_buffer: filename
// A: load mode
.global build_load_file
build_load_file:
  PHA
  LDA #1          // seqnum
  STA output_buffer+datagram_seqnum
  LDA #0                    // acknum
  STA output_buffer+datagram_acknum
  STA output_buffer+datagram_ack

  LDX #datagram_data
  LDA #load_file_code   // load_file command code
  STA output_buffer,X            // Command code in packet data
  INX                            // Space for message_length
  
  // Load mode
  PLA
  INX
  STA output_buffer,X       // Load mode.
  INX
  
  // Filename
  LDY #0
load_file_loop:
  LDA temp_buffer,Y
  STA output_buffer, X
  BEQ load_file_end
  INY
  INX
  BNE load_file_loop
load_file_end:
  INY           // +1 for load_mode
  STY output_buffer+datagram_data+message_length     // Store length in message
  TYA     // Y is length of filename
  CLC
  ADC #6    // 1 byte mode field + 3 bytes header.
  TAY
  RTS

// Exit:
// X,Y: file length (or -1 for error)
.global parse_load_file_result
parse_load_file_result:
  LDA input_buffer+datagram_data+message_command
  CMP #load_file_result_code
  BNE bad_load_file_result
  LDX input_buffer+datagram_data+message_data+load_file_result_length   // Low byte
  LDY input_buffer+datagram_data+message_data+load_file_result_length+1 // High byte
  LDA input_buffer+datagram_data+message_data+load_file_result_entry_addr
  STA exe_addr
  LDA input_buffer+datagram_data+message_data+load_file_result_entry_addr+1
  STA exe_addr+1
  RTS
bad_load_file_result:
  JSR console_write_string
  .asciz "Bad load result\r\n"
  LDX #0
  LDY #0
  RTS
    
// Entry:
// X,Y: low, high size of file in bytes.
// block_handler: address of block handler function.
// Uses file_length, byteA, temp_buffer.
// Corrupts X,Y, A.
.global load_blocks
load_blocks:
  STX file_length
  STY file_length+1

  // Check for both X and Y being 0xff (error).
  TXA
  AND file_length+1
  CMP #0xff
  BNE load_blocks_loop
  
  // Print error returned in message.
  LDA #datagram_data+message_data+2    // error string
  STA temp_addr
  LDA #(input_buffer >> 8)
  STA temp_addr+1
  JSR console_print_string
  JMP console_crlf
  
load_blocks_loop:
  LDA file_length
  ORA file_length+1
  BEQ load_blocks_end               // Check for file_length == 0
  
  // Read a block from the file.
  JSR read_packet
  BCC load_blocks_error
  
  // Check for correct command code for a file block.
  LDA input_buffer+datagram_data+message_command
  CMP #file_block_code
  BNE load_blocks_error
  
  // Length of data is message_length - 2, but the data starts 2 bytes
  // into the message.

  // Decrement file length by length of data (-2)
  DEC input_buffer+datagram_data+message_length
  DEC input_buffer+datagram_data+message_length
  LDA file_length
  SEC
  SBC input_buffer+datagram_data+message_length
  STA file_length
  LDA file_length+1
  SBC #0
  STA file_length+1

  // Handle the block
  JSR load_blocks_handle_block

  // End of block, acknowledge the block.
  LDA #file_block_code
  JSR acknowledge
  JSR send_packet
  JMP load_blocks_loop

load_blocks_handle_block:
  JMP (block_handler)
  
load_blocks_error:
  JSR console_write_string
  .asciz "Error reading file\r\n"
load_blocks_end:
  RTS
  

// Write the block in the input buffer to the console.
// Corrupts X, A.
.global write_block_to_console
write_block_to_console:
  // Print contents of message data to console, dealing with linefeeds.
  LDX #0
cat_block_loop:
  CPX input_buffer+datagram_data+message_length
  BEQ cat_end_block
  LDA input_buffer+datagram_data+message_data+file_block_result_data,X
  CMP #'\n'
  BNE cat_not_lf
  PHA
  LDA #'\r'
  JSR console_write_char
  PLA
cat_not_lf:
  JSR console_write_char
cat_next_byte:
  INX
  JMP cat_block_loop
cat_end_block:
  RTS
  
// Load the next block into memory.  Block is in the message in the
// input buffer.
// Uses load_addr as address being written.
// Corrupts A,Y.
.global load_block
load_block:
  LDA input_buffer+datagram_data+message_data+file_block_result_addr
  STA load_addr
  LDA input_buffer+datagram_data+message_data+file_block_result_addr+1
  STA load_addr+1
  LDY #0
load_block_loop:
  CPY input_buffer+datagram_data+message_length
  BEQ load_end_block
  LDA input_buffer+datagram_data+message_data+file_block_result_data,Y
  STA (load_addr),Y
  INY
  JMP load_block_loop
load_end_block:
  RTS


