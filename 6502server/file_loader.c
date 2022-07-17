//
//  loader.c
//  6502server
//
//  Created by David Allison on 11/2/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include "file_loader.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "loader.h"

void LoaderStateMachineInit(LoaderStateMachine* fsm) {
  fsm->state = kLoaderIdle;
  fsm->fd = -1;
  fsm->next_addr = 0;
  fsm->next_segment_addr = NULL;
  fsm->next_offset = 0;
  VectorInit(&fsm->regions);
}

void LoaderStateMachineDestruct(LoaderStateMachine* fsm) {
  VectorDestruct(&fsm->regions);
}

static bool SendLoadResult(Connection* conn, int16_t length, int16_t entry_addr,
                           const char* error) {
  Message msg;
  msg.command = kCommandLoadFileResult;
  msg.data.load_file_result.length = length;
  msg.data.load_file_result.entry_addr = entry_addr;
  strcpy(msg.data.load_file_result.error, error);
  msg.size = 4 + strlen(error) + 1;
  return ConnectionSend(conn, false, 0, &msg, MSG_SIZE(msg));
}

static bool SendBlock(Connection* conn, int16_t addr, int16_t length, const void* data) {
  Message msg;
  msg.command = kCommandFileBlock;
  msg.data.file_block.start_addr = addr;
  memcpy(msg.data.file_block.data, data, length);
  msg.size = length + 2;
  return ConnectionSend(conn, false, 0, &msg, MSG_SIZE(msg));
}

static ssize_t FirstRegion(LoaderStateMachine* fsm) {
  for (size_t i = 0; i < fsm->regions.length; i++) {
    Region* region = fsm->regions.value.p[i];
    if (region->segment != NULL) {
      return i;
    }
  }
  return -1;
}

static Region* CurrentRegion(LoaderStateMachine* fsm) {
  return fsm->regions.value.p[fsm->current_region];
}

static void* CurrentRegionStartAddress(LoaderStateMachine* fsm) {
  Region* region = CurrentRegion(fsm);
  return region->address + region->segment->offset;
}

static void* CurrentRegionEndAddress(LoaderStateMachine* fsm) {
  Region* region = CurrentRegion(fsm);
  return region->address + region->segment->offset + region->segment->memsz;
}

static int FullRegionsLength(LoaderStateMachine* fsm) {
  int len = 0;
  for (size_t i = 0; i < fsm->regions.length; i++) {
     Region* region = fsm->regions.value.p[i];
     if (region->segment != NULL) {
       len += region->segment->memsz;
     }
   }
  printf("Full length: 0x%x(%d)\n", len, len);
  return len;
}

static void AdvanceRegion(LoaderStateMachine* fsm, size_t length) {
  void* next_addr = fsm->next_segment_addr + (int)length;
  void* region_end_addr = CurrentRegionEndAddress(fsm);
  if (next_addr < region_end_addr) {
    fsm->next_segment_addr += length;
    fsm->next_addr += length;
    return;
  }
  // Move to next region.
  fsm->current_region++;
  while (fsm->current_region < fsm->regions.length) {
    Region* region = CurrentRegion(fsm);
    if (region->segment != NULL) {
      break;
    }
    fsm->current_region++;
  }
  
  if (fsm->current_region >= fsm->regions.length) {
    fsm->current_region = -1;
    return;
  }
  fsm->next_segment_addr = CurrentRegionStartAddress(fsm);
  fsm->next_addr = (int)CurrentRegion(fsm)->segment->vaddr;
}

static ssize_t ReadCurrentRegion(LoaderStateMachine* fsm, void* buffer, size_t len) {
  if (fsm->current_region == -1) {
    return 0;
  }
  ssize_t remaining = CurrentRegionEndAddress(fsm) - fsm->next_segment_addr;
  len = len < remaining ? len : remaining;
 
  memcpy(buffer, fsm->next_segment_addr, len);
  return len;
}

static bool OpenELFFile(LoaderStateMachine* fsm, Connection* conn, const char* filename) {
  // Initialize a 6502 architecture.
  W65C02LoaderArchitectureInit(&fsm->arch);
  
  // Initialize the loader from the given exe file.
  String fn = {0};
  StringPrintf(&fn, "%s/%s", root_dir, filename);
  printf("opening ELF file %s\n", fn.value);
  bool ok = LoaderInitFromFile(&fsm->loader,
                               &fn, 0,
                               &fsm->arch,
                               NULL,
                               ".");
  StringDestruct(&fn);
  if (!ok) {
    printf("Error Loading %s\n", filename);
    SendLoadResult(conn, -1, 0, "Cannot open file");
    return false;
  }
  printf("File open, reading regions\n");
  
  for (size_t i = 1; i < fsm->loader.regions.length; i++) {
    Region* region = fsm->loader.regions.value.p[i];
    if (region->segment != NULL) {
      if (region->segment->type == PT(load)) {
        printf("Region %" PRIx64 ", length %" PRId64 "\n", region->segment->vaddr, region->segment->memsz);
        VectorAppend(&fsm->regions, region);
      }
    }
  }
  printf("Regions loaded, finding first\n");
  ssize_t region = FirstRegion(fsm);
  // No regions, empty file.
  if (region == -1) {
    printf("No first region\n");
    fsm->state = kLoaderIdle;
    return SendLoadResult(conn, 0, 0, "");

  }
  fsm->state = kLoaderSendingFile;
  fsm->loading_elf = true;
  fsm->current_region = region;
  fsm->next_segment_addr = CurrentRegionStartAddress(fsm);
  fsm->next_addr = (int)CurrentRegion(fsm)->segment->vaddr;
  printf("starting at %x (addr %p)\n", fsm->next_addr, fsm->next_segment_addr);
  return SendLoadResult(conn, FullRegionsLength(fsm), fsm->loader.main_address, "");
}

static bool OpenFile(LoaderStateMachine* fsm, Connection* conn, const char* filename) {
  String path = {0};
  StringPrintf(&path, "%s/%s", root_dir, filename);
  printf("opening file %s\n", path.value);
  fsm->fd = open(path.value, O_RDONLY);
  if (fsm->fd == -1) {
    SendLoadResult(conn, -1, 0, "Cannot open file");
    StringDestruct(&path);
    return false;
  }
  fsm->state = kLoaderSendingFile;
  fsm->next_offset = 0;
  fsm->next_addr = 0;
  fsm->loading_elf = false;
  
  struct stat st;
  int e = lstat(path.value, &st);
  int length = 0;
  if (e == 0) {
    length = (int)st.st_size;
  }
  bool ok = SendLoadResult(conn, length, 0, "");
  StringDestruct(&path);
  return ok;
}

static void Reset(LoaderStateMachine* fsm) {
  if (fsm->loading_elf) {
    LoaderDestruct(&fsm->loader);
  } else if (fsm->fd >= 0) {
    close(fsm->fd);
  }
  fsm->state = kLoaderIdle;
}

static void SendNextBlock(LoaderStateMachine* fsm, Connection* conn) {
  char data[256];
  ssize_t n;
  if (fsm->loading_elf) {
    printf("Reading region memory\n");
    n = ReadCurrentRegion(fsm, data, kMaxBlockSize);
  } else {
     n = read(fsm->fd, data, kMaxBlockSize);
  }
  if (n < 0) {
    printf("Failed to read block\n");
    return;
  }
  if (n == 0) {
    // EOF.
    Reset(fsm);
  } else {
    bool ok = SendBlock(conn, fsm->next_addr, n, data);
    if (!ok) {
      Reset(fsm);
    }
    if (fsm->loading_elf) {
      AdvanceRegion(fsm, n);
    }
  }
}


void AdvanceLoader(LoaderStateMachine* fsm, Connection* conn, bool ack, Message* msg) {
  if (ack) {
    // For each packet sent we will get an ACK.  This is used to trigger
    // next send, if any.
    switch (fsm->state) {
      case kLoaderIdle:
        break;
      case kLoaderSendingFile:
        SendNextBlock(fsm, conn);
        break;
    }
  } else {
    if (fsm->state != kLoaderIdle) {
      printf("Unexpected message, aborting\n");
      Reset(fsm);
    } else {
      Reset(fsm);
      bool ok;
      switch (msg->data.load_file.mode) {
        case kLoadBinary:
          ok = OpenELFFile(fsm, conn,
                              msg->data.load_file.name);
          break;
        case kLoadRaw:
          ok = OpenFile(fsm, conn,
                              msg->data.load_file.name);
          break;
        default:
          printf("Invalid open mode %d\n", msg->data.load_file.mode);
          return;
      }
 
      if (!ok) {
        Reset(fsm);
      }
    }
  }
}

