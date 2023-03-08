//
//  packet.c
//  6502server
//
//  Created by David Allison on 10/12/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include "packet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define kPoly 0x1021

void Hexdump(const void* addr, int size) {
  char buf[16];
  const char* caddr = (const char*)(addr);
  const char* endaddr = caddr + size;
  int len = size;
  while (len > 0) {
    printf("%p  ", caddr);
    for (int i = 0; i < 16; i++) {
      if (caddr > endaddr) {
        buf[i] = 0xff;
      } else {
        buf[i] = *caddr++;
      }
      printf("%02X ", buf[i] & 0xff);
    }
    printf("  ");
    for (int i = 0; i < 16; i++) {
      if (buf[i] >= 0x20 && buf[i] < 0x7f) {
        printf("%c", buf[i]);
      } else {
        printf("%c", '.');
      }
    }
    len -= 16;
    printf("\n");
  }
}

/* On entry, addr=>start of data
             num = length of data
             crc = incoming CRC     */
int CRC16(const char *addr, int num, int crc) {
  for (; num>0; num--) {             /* Step through bytes in memory */
    crc = crc ^ (*addr++ << 8);      /* Fetch byte from memory, XOR into CRC top byte*/
    for (int i=0; i<8; i++) {             /* Prepare to rotate 8 bits */
      crc <<= 1;                      /* rotate */
      if (crc & 0x10000) {             /* bit 15 was set (now bit 16)... */
        crc = (crc ^ kPoly) & 0xFFFF; /* XOR with XMODEM polynomic */
                                     /* and ensure CRC remains 16-bit value */
      }
    }                              /* Loop for 8 bits */
  }                                /* Loop until num=0 */
  return crc;                     /* Return updated CRC */
}

void DecodePacket(Packet* packet) {
  printf("Packet: Len: %d, CRC: %x\n", packet->length & 0xff, packet->crc & 0xffff);
}

// Build a packet.  The data is already in place.
void BuildPacket(Packet* packet,  int datalen) {
  packet->length = (uint8_t)datalen + 2;
  packet->crc = CRC16(packet->data, datalen, 0);
}

int ProcessPacket(Packet* packet) {
  printf("Incoming... %d bytes\n", packet->length);
  Hexdump(packet, packet->length);
  uint16_t crc = CRC16(packet->data, packet->length - 2, 0);
  if (crc != packet->crc) {
    printf("Bad CRC: calc: %x, sent: %x\n", crc, packet->crc);
    return -1;
  }
  return packet->length;
}
