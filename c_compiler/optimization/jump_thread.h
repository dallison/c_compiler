#ifndef jump_thread_h
#define jump_thread_h

#include <stdbool.h>

struct Generator;

bool JumpThreadingOptimization(struct Generator* gen);

#endif /* jump_thread_h */
