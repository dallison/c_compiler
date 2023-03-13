//
//  comms.h
//  6502server
//
//  Created by David Allison on 10/13/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef comms_h
#define comms_h

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include "packet.h"

#define FLAG_ACK 1

extern const char root_dir[];

typedef struct {
  uint8_t seqnum;     // Offset 0
  uint8_t acknum;     // Offset 1
  uint8_t flags;        // Offset 2
  char data[0];       // Offset 3
} Datagram;

#define kFlagAck 1

void DecodeDatagram(Datagram* d);

typedef struct {
  int fd;
  int next_seqnum;
  int last_ack;
  ssize_t (*read_func)(int, void*, size_t);
  ssize_t (*write_func)(int, const void*, size_t);
  char packet[256];
} Connection;

void ConnectionInit(Connection* conn, int fd);
bool ConnectionSend(Connection* conn, bool ack, int acknum, const void* data, int len);
int ConnectionReceive(Connection* conn, void* buffer, int buflen);

#define kCommandPingPong 1
#define kCommandListFiles 2
#define kCommandListFilesResult 3
#define kCommandDirEntry 4
#define kCommandLoadFile 5
#define kCommandLoadFileResult 6
#define kCommandFileBlock 7

typedef struct {
  char text[16];
} PingPongCommand;

typedef struct {
  char filter[64];
} ListFilesCommand;

typedef struct {
  int16_t num_files;
} ListFilesResult;

typedef enum {
  kFileDir,
  kFileFile,
} FileType;

typedef struct {
  int16_t length;
  int8_t type;      // File or dir.
  char name[64];
} DirEntryResult;

typedef enum {
  kLoadRaw,
  kLoadBinary,
} LoadMode;

typedef struct {
  int8_t mode;
  char name[64];
} LoadFileCommand;

typedef struct {
  int16_t length;   // -1 = error.
  int16_t entry_addr;
  char error[64];
} LoadFileResult;

// Max block size is 256 minus the message header (command and size)
// and also minus the start_addr field of FileBlockResult.
#define kMaxBlockSize (256-sizeof(Packet)-sizeof(Datagram) - 4)

// Address to place block is in start_addr.
// Length of data is message.length - 2.
typedef struct {
  uint16_t start_addr;
  char data[kMaxBlockSize];
} FileBlockResult;

#define MSG_SIZE(msg) (msg.size + 2)

typedef struct {
  uint8_t command;
  uint8_t size;         // Size of data.
  union {
    PingPongCommand pingpong;
    ListFilesCommand list_files;
    ListFilesResult list_files_result;
    DirEntryResult dir_entry;
    LoadFileCommand load_file;
    LoadFileResult load_file_result;
    FileBlockResult file_block;
  } data;
} Message;

void DecodeMessage(Message* msg);

#endif /* comms_h */
