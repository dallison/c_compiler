//
//  device.h
//  c_compiler
//
//  Created by David Allison on 9/27/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef device_h
#define device_h

.set device_base_addr 0xfe00

.set device_size 12
.set device_open_func 0
.set device_close_func 2
.set device_read_func 4
.set device_write_func 6
.set device_ioctl_func 8
.set device_poll_func 10


// For all device functions, Y contains device id.

// Hardwired devices.
.set acia1_device_id 0     // 6850 ACIA #1
.set acia2_device_id 1     // 6850 ACIA #2

// 6850 ACIA

// 6580 ACIA Control and Status Register
// On write (control register):
// +--------+---------------------------+
// | Bits   |   Name   |  Meaning       |
// +--------+---------------------------+
// |   0-1  |   CR0-1  |  Clock divider |
// |   2-4  |   CR2-4  |  Format select |
// |   5-6  |   CR5-6  |  Output IRQ    |
// |   7    |   CR7    |  Input IRQ     |
// +--------+---------------------------+
//
// CR0-1 Clock divider
// -------------------
// CR1    CR0    Meaning
// ----------------------
// 0       0     /1
// 0       1     /16
// 1       0     /64
// 1       1     Master reset

// CR2-4 Format selector
// ---------------------
// Format:
// A B C:
// A = number of data bits
// B = parity:
//     e = even
//     o = odd
//     n = none
// C = number of stop bits
//
// CRn       Meaning
// 4 3 2     A B C
// ---------------
// 0 0 0     7 e 2
// 0 0 1     7 o 2
// 0 1 0     7 e 1
// 0 1 1     7 o 1
// 1 0 0     8 n 2
// 1 0 1     8 n 1
// 1 1 0     8 e 1
// 1 1 1     8 o 1
//
// Output (transmitter) IRQ control
// --------------------------------
// Controls RTS and IRQ on Transmission Data Register Empty
// CR5 CR6   Meaning
// 0    0    RTS = 0 IRQ disabled
// 0    1    RTS = 0 IRQ enabled
// 1    0    RTS = 1 IRQ disabled
// 1    1    RTS = 0 transmits BREAK, IRQ disabled
//
// Input (receiver) IRQ control
// CR7 = 1 => IRQ on receive enabled
//
// On Read (status register)
// +-----+------------------------------------+
// | Bit | Name  |   Meaning                  |
// +-----+------------------------------------+
// | 0   | RDRF  | RX Data Register Full      |
// | 1   | TDRE  | TX data register empty     |
// | 2   | DCD   | Data carrier detect        |
// | 3   | CTS   | Clear to Send              |
// | 4   | FE    | Framing error              |
// | 5   | OVRN  | Receiver overrun           |
// | 6   | PE    | Parity error               |
// | 7   | IRQ   | Interrrupt request         |
// +-----+------------------------------------+

.set ACIA1 0xfe00
.set ACIA1_BASE 0x00
.set ACIA1_CSR 0xfe00    // Control/status register
.set ACIA1_DATA 0xfe01     // Tx/rx data register

.set ACIA2 0xfe40
.set ACIA2_BASE 0x40
.set ACIA2_CSR 0xfe40    // Control/status register
.set ACIA2_DATA 0xfe41     // Tx/rx data register

// VIA (6522 at address 0xfe00)
.set VIA_ORB 0xfe80
.set VIA_ORA 0xfe81
.set VIA_DDRB 0xfe82
.set VIA_DDRA 0xfe83

// Debug Registers
.set REGA 0xfec0

#endif /* device_h */
