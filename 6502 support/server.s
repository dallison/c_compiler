//
//  mainframe.s
//  c_compiler
//
//  Created by David Allison on 10/8/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include "addresses.h"
#include "device.h"
#include "comms.h"

.text

.global acknowledge

.global ping
ping:
  JSR console_write_string
  .asciz "building ping\r\n"
  JSR build_ping
  JSR console_write_string
  .asciz "sending ping request\r\n"
  JSR send_packet
  BCC bad_ping
  JSR console_write_string
  .asciz "reading ping response\r\n"
  JSR read_packet
  BCS ping_ok
  JSR console_write_string
  .asciz "Bad ping response\r\n"
  RTS
ping_ok:
  JSR console_write_string
  .asciz "pong\r\n"
  
  // Send ack for pong.
  LDA #ping_command_code
  JSR acknowledge
  JSR send_packet
bad_ping:
  RTS
  
.global ls
ls:
  JSR build_list_files
  JSR send_packet
  JSR read_packet
  BCC ls_failed
  // Packet will contain list_files_result_code
  
  // Send ack for list_files.
  LDA #list_files_result_code
  JSR acknowledge
  JSR send_packet

  JSR parse_list_files_result
  // X,Y: low, high number of files
  
  JSR list_files
  RTS
  
ls_failed:
  JSR console_write_string
  .asciz "Failed to list files\r\n"
  RTS
  
// temp_buffer: filename to cat
.global cat
cat:
  LDA #load_raw
  JSR build_load_file
  JSR send_packet
  JSR read_packet
  BCC cat_failed

// Send ack for load_file.
  LDA #load_file_code
  JSR acknowledge
  JSR send_packet

  JSR parse_load_file_result
  LDA #%lo(write_block_to_console)
  STA block_handler
  LDA #%hi(write_block_to_console)
  STA block_handler+1
  JMP load_blocks

cat_failed:
  JSR console_write_string
  .asciz "Failed to cat file\r\n"
  RTS

.global load
load:
  LDA #load_binary
  JSR build_load_file
  JSR send_packet
  JSR read_packet
  BCC load_failed

  // Send ack for load_file.
  LDA #load_file_code
  JSR acknowledge
  JSR send_packet

  JSR parse_load_file_result
  LDA #%lo(load_block)
  STA block_handler
  LDA #%hi(load_block)
  STA block_handler+1
  JMP load_blocks

load_failed:
  JSR console_write_string
  .asciz "Failed to load file\r\n"
  RTS

.global boot
boot:
  LDY #0
  // Copy name to temp_buffer.
boot_loop:
  LDA %abs(kernel),Y
  STA temp_buffer,Y
  BEQ boot_end_loop
  INY
  JMP boot_loop
boot_end_loop:
  JSR load
  JMP (exe_addr)
  
kernel:
  .asciz "kernel"
  
