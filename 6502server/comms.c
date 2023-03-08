//
//  comms.c
//  6502server
//
//  Created by David Allison on 10/13/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include "comms.h"
#include "packet.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

#define kMaxPacket 256

const char root_dir[] = "../6502root";

// Write to the serial port with an ack for every byte written.
ssize_t SlowWrite(int fd, const char* s, size_t n) {
  char buf[1];
  for (size_t i = 0; i < n; i++) {
    ssize_t r = write(fd, s, 1);
    if (r <= 0) {
      return r;
    }
#if 0
    // Read an ACK that is the inverse of byte sent.
    r = read(fd, buf, 1);
    if (r <= 0) {
      return r;
    }
    printf("ACK: %x\n", buf[0]);
    if (buf[0] != *s) {
      printf("Bad byte ACK: recv: %x, expected: %x\n", buf[0], *s);
    }
#endif
    //usleep(500);
    s++;
  }
  return n;
}

void ConnectionInit(Connection* conn, int fd) {
  conn->fd = fd;
  conn->next_seqnum = 1;
  conn->last_ack = 0;
  conn->read_func = read;
  conn->write_func = SlowWrite;
  
  // Configure port in raw mode.
  struct termios tty = {0};
  const int kBaud = B115200;
  
  tcgetattr(fd, &tty);
  
  cfsetospeed(&tty, (speed_t)kBaud);
  cfsetispeed(&tty, (speed_t)kBaud);

  tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                      | INLCR | IGNCR | ICRNL | IXON | IXOFF);
  tty.c_oflag &= ~OPOST;
  tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  tty.c_cflag &= ~(CSIZE | PARENB);
  tty.c_cflag |= CS8;
  
  cfmakeraw(&tty);
  
  if (tcsetattr(fd, TCSANOW, &tty) == -1) {
    fprintf(stderr, "Unable to set TTY to raw: %s", strerror(errno));
  }
}

static void InitDatagram(Datagram* gram) {
  memset(gram, 0, sizeof(*gram));
}

bool ConnectionSend(Connection* conn, bool ack, int acknum, const void* data, int len) {
  Packet* packet = (Packet*)(conn->packet);
  Datagram* gram = (Datagram*)(packet->data);
  InitDatagram(gram);
  gram->seqnum = conn->next_seqnum;
  if (ack) {
    gram->acknum = acknum;
    gram->flags = 1;
  }
  memcpy(&gram->data, data, len);
  BuildPacket(packet, sizeof(*gram) + len);
  int nbytes = packet->length+1;
  printf("sending %d bytes\n", nbytes);
  Hexdump(conn->packet, nbytes);
  
  // Send length byte.
  uint8_t lenbuf[1];
  lenbuf[0] = packet->length;
  ssize_t n = conn->write_func(conn->fd, lenbuf, 1);
  
  // Wait for ACK of length and check its value.
  printf("waiting for len ack\n");
  n = conn->read_func(conn->fd, lenbuf, 1);
  if (n != 1) {
    printf("Failed to read len ack\n");
    return false;
  }
  if ((~lenbuf[0] & 0xff) != (packet->length & 0xff)) {
    printf("Invalid len ack value %x/%x\n", lenbuf[0], (uint8_t)packet->length);
    return false;
  }
  
  // Now send the rest of the packet.
  n = conn->write_func(conn->fd, conn->packet+1, packet->length);
  if (n != packet->length) {
    printf("Failed to send %d bytes\n", packet->length);
    return false;
  }
  conn->next_seqnum += n;
  conn->next_seqnum &= 0xff;
  DecodePacket(packet);
  DecodeDatagram(gram);
  return true;
}

int ConnectionReceive(Connection* conn, void* buffer, int buflen) {
  printf("Waiting for packet\n");
  ssize_t n = conn->read_func(conn->fd, conn->packet, 1);        // Read length.
  if (n != 1) {
    printf("Failed to read packet length\n");
    return -1;
  }
  uint8_t datalen = *(uint8_t*)(conn->packet);
  printf("got length %d\n", datalen);
  // Ack length by writing inverted value.
  // If we don't ack here we might miss the first byte of the next read
  // because they are done using separate read calls.  There's no hardware
  // flow control.
  char ack[1];
  ack[0] = ~datalen;
  printf("sending ACK %x\n", ack[0]);
  n = conn->write_func(conn->fd, ack, 1);
  
  // Now we can read the rest of the packet.
  n = conn->read_func(conn->fd, conn->packet+1, datalen);    // Read data.
  if (n <= 0) {
    printf("Failed to read packet data (length: %d)\n", datalen);
    return -1;
  }
  Packet* packet = (Packet*)(conn->packet);
  int length = ProcessPacket(packet);
  if (length == -1) {
    // Bad packet.
    printf("Bad packet\n");
    return length;
  }
  Datagram* gram = (Datagram*)(packet->data);
  DecodePacket(packet);
  DecodeDatagram(gram);
  if ((gram->flags & kFlagAck) != 0) {
    // Acknowlege: return acknum with bit 30 set.
    return 0x40000000 | gram->acknum;
  }  
  int gram_length = length - sizeof(*gram);
  memcpy(buffer, gram->data, gram_length);
  return gram_length;
}

static void DecodeFlags(uint8_t flags, char* buffer) {
  buffer[0] = '*';
  if ((flags & 1) != 0) {
    buffer[0] = 'A';
  }
  buffer[1] = '\0';
}


void DecodeDatagram(Datagram* d) {
  char buffer[4];
  DecodeFlags(d->flags, buffer);
  printf("  Datagram: Seq: %d, Ack: %d, Flags: %s\n", d->seqnum, d->acknum, buffer);
  if ((d->flags & kFlagAck) == 0) {
    DecodeMessage((Message*)d->data);
  }
}

void DecodeMessage(Message* msg) {
  const char* command = "unknown";
  switch (msg->command) {
  case kCommandPingPong:
    command = "ping-pong";
    break;
  case kCommandListFiles:
    command = "list-files";
    break;
  case kCommandListFilesResult:
    command = "list-files-result";
    break;
  case kCommandDirEntry:
    command = "dir-entry";
    break;
  case kCommandLoadFile:
    command = "load-file";
    break;
  case kCommandLoadFileResult:
    command = "load-file-result";
    break;
  case kCommandFileBlock:
    command = "file-block";
    break;
  }
  printf("Cmd: %s, size: %d: ", command, msg->size);
  switch (msg->command) {
    case kCommandPingPong:
      printf("text: %s\n", msg->data.pingpong.text);
      break;
    case kCommandListFiles:
      printf("filter: %s\n", msg->data.list_files.filter);
      break;
    case kCommandListFilesResult:
      printf("num_files: %d\n", msg->data.list_files_result.num_files);
      break;
    case kCommandDirEntry: {
      char type[32];
      switch (msg->data.dir_entry.type) {
        case kFileDir:
          snprintf(type, sizeof(type), "dir");
          break;
        case kFileFile:
          snprintf(type, sizeof(type), "file");
          break;
        default:
          snprintf(type, sizeof(type), "unkown");
        break;
      }
      printf("length: %d, type: %s, name: %s\n", msg->data.dir_entry.length,
             type, msg->data.dir_entry.name);
      break;
    }
    case kCommandLoadFile: {
      char mode[32];
      switch (msg->data.load_file.mode) {
        case kLoadRaw:
          snprintf(mode, sizeof(mode), "raw");
          break;
          case kLoadBinary:
            snprintf(mode, sizeof(mode), "binary");
            break;
        default:
          snprintf(mode, sizeof(mode), "unknown");
            break;

      }
      printf("mode: %s, name: %s\n", mode, msg->data.load_file.name);
      break;
    }
    case kCommandLoadFileResult:
      printf("length: %d, error: %s\n", msg->data.load_file_result.length,
            msg->data.load_file_result.error);
      break;
    case kCommandFileBlock:
      printf("start_addr: %x\n", msg->data.file_block.start_addr);
      break;
    }
}

