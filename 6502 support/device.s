//
//  device.s
//  c_compiler
//
//  Created by David Allison on 10/12/20.
//  Copyright © 2020 David Allison. All rights reserved.
//
#include "addresses.h"
#include "device.h"

.text

// temp_addr: address to write
// Y: length to write
// A: device id
// Corrupts A, addrA and addrB
.global device_write
device_write:
  PHY
  PHX
  PHY
  ASL A
  TAX
  LDA %abs(devices), X    // LO byte of device
  STA addrA
  INX
  LDA %abs(devices), X
  STA addrA+1
  LDY #device_write_func
  LDA (addrA), Y
  STA addrB
  INY
  LDA (addrA), Y
  STA addrB+1
  PLY
device_invoke:
  JSR device_invoke_indirect
  PLX
  PLY
  RTS
  
device_invoke_indirect:
  JMP (addrB)

  
// temp_addr: address to read to
// Y: max length to read
// A: device id.
// Corrupts A, addrA and addrB
.global device_read
device_read:
  PHY
  PHX
  PHY
  ASL A
  TAX
  LDA %abs(devices), X    // LO byte of device
  STA addrA
  INX
  LDA %abs(devices), X
  STA addrA+1
  LDY #device_read_func
  LDA (addrA), Y
  STA addrB
  INY
  LDA (addrA), Y
  STA addrB+1
  PLY
  JMP device_invoke

  
.global devices
.global acia1_device
.global acia2_device
devices:
  .hword acia1_device
  .hword acia2_device


