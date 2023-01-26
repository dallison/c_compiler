//
//  main.c
//  filecopy
//
//  Created by David Allison on 12/10/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
int CopyFile(const char* from, const char* to) {
  int infd = open(from, O_RDONLY);
  if (infd == -1) {
    return errno;
  }
  int outfd = open(to, O_WRONLY|O_CREAT|O_TRUNC, 0444);
  if (outfd == -1) {
    close(infd);
    return errno;
  }
  char buf[256];
  for (;;) {
    ssize_t n = read(infd, buf, sizeof(buf));
    if (n < 0) {
      return errno;
    }
    if (n == 0) {
      break;
    }
    ssize_t bytes_written = write(outfd, buf, n);
    if (bytes_written < 0) {
      return errno;
    }
    if (bytes_written != n) {
      return EIO;
    }
  }
  close(infd);
  close(outfd);
  return 0;
}

int main(int argc, const char * argv[]) {
  int e = CopyFile(argv[1], argv[2]);
  if (e != 0) {
    printf("Copy failed: %d", errno);
  }
  return 0;
}
