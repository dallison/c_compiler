//
//  main.c
//  mmap_test
//
//  Created by David Allison on 3/11/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int main(int argc, const char * argv[]) {
  int page_size = sysconf(_SC_PAGESIZE);
  printf("Page size: %d\n", page_size);
  int fd = open("/tmp/foo", O_RDWR);
  void* segment_ptr = mmap((void*)0x400000000, page_size*2, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_FIXED|MAP_PRIVATE|MAP_JIT, fd, 0);
                           
  printf("mapped: %p\n", segment_ptr);
  if (segment_ptr == MAP_FAILED) {
    printf("error %s\n", strerror(errno));
  }
}
