//
//  file_loader.h
//  6502server
//
//  Created by David Allison on 11/2/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef file_loader_h
#define file_loader_h

#include "comms.h"
#include "dstring.h"
#include "vector.h"
#include <stdio.h>
#include "elf_reader.h"
#include "loader.h"
#include "loader_arch_6502.h"

typedef struct {
  enum {
    kLoaderIdle,
    kLoaderSendingFile,
  } state;
  bool loading_elf;
  Loader loader;
  LoaderArchitecture arch;

  int fd;             // File descriptor for file being sent.
  int next_addr;      // Next region address to send.
  void* next_segment_addr;      // Next address in local memory.
  Vector regions;
  ssize_t current_region;
  int next_offset;                // Next offset into file.
} LoaderStateMachine;

void LoaderStateMachineInit(LoaderStateMachine* fsm);
void LoaderStateMachineDestruct(LoaderStateMachine* fsm);

void AdvanceLoader(LoaderStateMachine* fsm, Connection* conn, bool ack, Message* msg);
#endif /* file_loader_h */
