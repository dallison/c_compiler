//
//  main.c
//  6502server
//
//  Created by David Allison on 10/12/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "comms.h"
#include "fs.h"
#include "file_loader.h"

#undef TEST

static void PingPong(Connection* conn, Message* msg) {
  printf("incoming ping\n");
  strcpy(msg->data.pingpong.text, "pong");
  msg->size = 5;
  bool ok = ConnectionSend(conn, false, 0, (char*)msg, msg->size + 2);
  if (!ok) {
    fprintf(stderr, "Failed to write to connection");
  } else {
    printf("sent ping response\n");
  }
}

char mock_buffer[256];
int mock_buffer_offset = 0;

ssize_t MockRead(int fd, void* buf, size_t len) {
  memcpy(buf, mock_buffer+mock_buffer_offset, len);
  mock_buffer_offset += len;
  return len;
}

ssize_t MockWrite(int fd, const void* buf, size_t len) {
  memcpy(mock_buffer, buf, len);
  return len;
}


int main(int argc, const char * argv[]) {
#ifdef TEST
  Connection s_conn;
  ConnectionInit(&s_conn, 0);
  s_conn.read_func = MockRead;
  s_conn.write_func = MockWrite;
  Message s_msg;
  s_msg.command = kCommandPingPong;
  strcpy(s_msg.data.pingpong.text, "ping");
  s_msg.size = 5;
  bool ok = ConnectionSend(&s_conn, false, 0, &s_msg, MSG_SIZE(s_msg));
  assert(ok);
  
  Connection r_conn;
  ConnectionInit(&r_conn, 0);
  r_conn.read_func = MockRead;
  r_conn.write_func = MockWrite;
  char buffer[256];
  int len = ConnectionReceive(&r_conn, buffer, sizeof(buffer));
  assert(len > 0);
  Message* r_msg = (Message*)buffer;
  assert(r_msg->command == kCommandPingPong);
  assert(strcmp("ping", r_msg->data.pingpong.text) == 0);
#else
  for (;;) {
    int fd = open("/dev/ttyUSB1", O_RDWR);
    if (fd == -1) {
      fprintf(stderr, "Failed to open serial device\n");
      exit(1);
    }
    Connection conn;
    ConnectionInit(&conn, fd);
    FileSystemStateMachine fs_fsm;
    FileSystemStateMachineInit(&fs_fsm);
    
    LoaderStateMachine load_fsm;
    LoaderStateMachineInit(&load_fsm);
    
    char buffer[256];
    for (;;) {
      int len = ConnectionReceive(&conn, buffer, sizeof(buffer));
      if (len <= 0) {
        printf("Error from connection\n");
        break;
      }
      Message* msg = (Message*)buffer;
      
      // ACK:
      if ((len & 0x40000000) != 0) {
        switch (msg->command) {
          case kCommandPingPong:
            // Nothing to do on a ping pong ack.
            break;
          case kCommandListFiles:
          case kCommandListFilesResult:
          case kCommandDirEntry:
            AdvanceFileSystem(&fs_fsm, &conn, true, NULL);
            break;
          case kCommandLoadFile:
          case kCommandLoadFileResult:
          case kCommandFileBlock:
            AdvanceLoader(&load_fsm, &conn, true, NULL);
            break;
        }
        continue;
      }
      // Message.
      switch (msg->command) {
        case kCommandPingPong:
          PingPong(&conn, msg);
          break;
        case kCommandListFiles:
          AdvanceFileSystem(&fs_fsm, &conn, false, msg);
          break;
        case kCommandLoadFile:
          AdvanceLoader(&load_fsm, &conn, false, msg);
          break;
      }
    }
    close(fd);
    FileSystemStateMachineDestruct(&fs_fsm);
    LoaderStateMachineDestruct(&load_fsm);
  }
#endif
}
