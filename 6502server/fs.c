//
//  fs.c
//  6502server
//
//  Created by David Allison on 10/14/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include "fs.h"
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>


File* NewFile(FileType type, const char* name, int size) {
  File* file = malloc(sizeof(File));
  file->type = type;
  StringInit(&file->name, name);
  file->size = size;
  return file;
}
void FileDestruct(File* file) {
  StringDestruct(&file->name);
}

static bool SendNumFiles(Connection* conn, int16_t num_files) {
  Message msg;
  msg.command = kCommandListFilesResult;
  msg.size = 2;
  msg.data.list_files_result.num_files = num_files;
  return ConnectionSend(conn, false, 0, &msg, MSG_SIZE(msg));
}

static bool SendDirEntry(Connection* conn, int8_t type,
                         int16_t length, const char* filename) {
  Message msg;
  msg.command = kCommandDirEntry;
  msg.data.dir_entry.type = type;
  msg.data.dir_entry.length = length;
  strncpy(msg.data.dir_entry.name, filename, sizeof(msg.data.dir_entry.name) - 1);
  msg.size = 3 + strlen(msg.data.dir_entry.name)+1;
  return ConnectionSend(conn, false, 0, &msg, MSG_SIZE(msg));
}

void FileSystemStateMachineInit(FileSystemStateMachine* fsm) {
  VectorInit(&fsm->files);
  fsm->next_file = 0;
  fsm->state = kFsIdle;
}

void FileSystemStateMachineDestruct(FileSystemStateMachine* fsm) {
  VectorDestructWithContents(&fsm->files, (VectorElementDestructor)FileDestruct, /*free_element=*/true);
}

static bool ListFiles(FileSystemStateMachine* fsm, Connection* conn, ListFilesCommand* command) {
  String path = {0};
  StringPrintf(&path, "%s/%s", root_dir, command->filter);
  DIR* dir = opendir(path.value);
  if (dir == NULL) {
    if (!SendNumFiles(conn, 0)) {
      fprintf(stderr, "Failed to send num files\n");
    }
    StringDestruct(&path);
    return false;
  }
  struct dirent* entry;
  
  // Read all entries into the fsm.
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.' && entry->d_name[1] == '\0') {
      continue;
    }
    if (entry->d_name[0] == '.' && entry->d_name[1] == '.' && entry->d_name[2] == '\0') {
      continue;
    }
    String fullpath = {0};
    StringPrintf(&fullpath, "%s/%s", path.value, entry->d_name);
    FileType type = kFileFile;
    int length = 0;
    struct stat st;
    int e = lstat(fullpath.value, &st);
    if (e == 0) {
      if (S_ISDIR(st.st_mode)) {
        type = kFileDir;
      }
      length = (int)st.st_size;
    }
    StringDestruct(&fullpath);
    VectorAppend(&fsm->files, NewFile(type, entry->d_name, length));
  }
  closedir(dir);
  
  if (!SendNumFiles(conn, fsm->files.length)) {
    fprintf(stderr, "Failed to send num files\n");
    StringDestruct(&path);
    closedir(dir);
    return false;
  }

  StringDestruct(&path);
  fsm->state = kFsSendingEntries;
  fsm->next_file = 0;
  return true;
}

static void Reset(FileSystemStateMachine* fsm) {
  for (size_t i = 0; i < fsm->files.length; i++) {
    File* file = fsm->files.value.p[i];
    FileDestruct(file);
    free(file);
  }
  VectorClear(&fsm->files);
  fsm->state = kFsIdle;
  fsm->next_file = 0;
}

static void SendNextFile(FileSystemStateMachine* fsm, Connection* conn) {
  if (fsm->next_file >= fsm->files.length) {
    fsm->state = kFsAllSent;
    return;
  }
  File* file = fsm->files.value.p[fsm->next_file++];
  printf("sending %s\n", file->name.value);
  bool ok = SendDirEntry(conn, file->type, file->size, file->name.value);
  if (!ok) {
    Reset(fsm);
  }
  printf("%d %zd\n", fsm->next_file, fsm->files.length);
  if (fsm->next_file == fsm->files.length) {
    printf("Sent last file\n");
    fsm->state = kFsAllSent;
  }
}

void AdvanceFileSystem(FileSystemStateMachine* fsm, Connection* conn, bool ack, Message* msg) {
  if (ack) {
    // For each packet sent we will get an ACK.  This is used to trigger
    // next send, if any.
    switch (fsm->state) {
      case kFsIdle:
        break;
      case kFsSendingEntries:
        SendNextFile(fsm, conn);
        break;
      case kFsAllSent:
        printf("All files sent\n");
        Reset(fsm);
        fsm->state = kFsIdle;
        break;
    }
  } else {
    if (fsm->state != kFsIdle) {
      printf("Unexpected message, aborting\n");
      Reset(fsm);
    } else {
      // Send number of files:
      Reset(fsm);
      bool ok = ListFiles(fsm, conn, &msg->data.list_files);
      if (!ok) {
        Reset(fsm);
      }
    }
  }
}
