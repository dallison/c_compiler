//
//  fs.h
//  6502server
//
//  Created by David Allison on 10/14/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef fs_h
#define fs_h

#include "comms.h"
#include "dstring.h"
#include "vector.h"


typedef struct {
  FileType type;
  String name;
  int size;
} File;

File* NewFile(FileType type, const char* name, int size);
void FileDestruct(File* file);

typedef struct {
  enum {
    kFsIdle,
    kFsSendingEntries,
    kFsAllSent,
  } state;
  Vector files;       // Vector of File*
  size_t next_file;
} FileSystemStateMachine;

void FileSystemStateMachineInit(FileSystemStateMachine* fsm);
void FileSystemStateMachineDestruct(FileSystemStateMachine* fsm);

void AdvanceFileSystem(FileSystemStateMachine* fsm, Connection* conn, bool ack, Message* msg);

#endif /* fs_h */
