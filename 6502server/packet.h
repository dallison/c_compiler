//
//  packet.h
//  6502server
//
//  Created by David Allison on 10/12/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef packet_h
#define packet_h

#include <stdint.h>

// A packet is variable length with max 256 bytes long.

typedef struct {
  unsigned int length:8;     // Length of data and crc.
  unsigned int crc:16;       // CRC 16
  char data[0];     // Data.
} Packet;

void BuildPacket(Packet* packet, int datalen);
int ProcessPacket(Packet* packet);
void Hexdump(const void* data, int len);
void DecodePacket(Packet* packet);
#endif /* packet_h */
