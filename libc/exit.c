//
//  exit.c
//  c_compiler
//
//  Created by David Allison on 6/17/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <davecc_lifecycle.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct ExitFunction {
  struct ExitFunction* next;
  union {
    void (*plain)(void);
    void (*cxx)(void*);
  } function;
  void* argument;
  void* dso;
  unsigned char has_argument;
} ExitFunction;

static ExitFunction* exit_functions;
static unsigned char exit_lock;

#if defined(__x86_64__)
void __davecc_atexit_lock(unsigned char* lock);
void __davecc_atexit_unlock(unsigned char* lock);
#else
static void __davecc_atexit_lock(unsigned char* lock) {
  (void)lock;
}
static void __davecc_atexit_unlock(unsigned char* lock) {
  (void)lock;
}
#endif

static int RegisterExitFunction(ExitFunction* entry) {
  if (entry == NULL) {
    return -1;
  }
  __davecc_atexit_lock(&exit_lock);
  entry->next = exit_functions;
  exit_functions = entry;
  __davecc_atexit_unlock(&exit_lock);
  return 0;
}

int atexit(void (*p)(void)) {
  ExitFunction* entry = malloc(sizeof(ExitFunction));
  if (entry == NULL) {
    return -1;
  }
  entry->function.plain = p;
  entry->argument = NULL;
  entry->dso = NULL;
  entry->has_argument = 0;
  return RegisterExitFunction(entry);
}

int __cxa_atexit(void (*p)(void*), void* argument, void* dso) {
  ExitFunction* entry = malloc(sizeof(ExitFunction));
  if (entry == NULL) {
    return -1;
  }
  entry->function.cxx = p;
  entry->argument = argument;
  entry->dso = dso;
  entry->has_argument = 1;
  return RegisterExitFunction(entry);
}

typedef struct {
  void* destructor;
  unsigned char* object;
  unsigned long count;
  unsigned long stride;
  unsigned char complete_object_argument;
} CXXDestructorContext;

static void RunCXXDestructors(void* argument) {
  CXXDestructorContext* context = argument;
  while (context->count != 0) {
    context->count--;
    void* object = context->object + context->count * context->stride;
    if (context->complete_object_argument) {
      ((void (*)(void*, int))context->destructor)(object, 1);
    } else {
      ((void (*)(void*))context->destructor)(object);
    }
  }
  free(context);
}

int __davecc_cxa_atexit(void* destructor, void* object, unsigned long count,
                        unsigned long stride, int complete_object_argument) {
  CXXDestructorContext* context = malloc(sizeof(CXXDestructorContext));
  if (context == NULL) {
    return -1;
  }
  context->destructor = destructor;
  context->object = object;
  context->count = count;
  context->stride = stride;
  context->complete_object_argument = complete_object_argument != 0;
  if (__cxa_atexit(RunCXXDestructors, context, NULL) != 0) {
    free(context);
    return -1;
  }
  return 0;
}

static ExitFunction* DetachExitFunctions(void* dso) {
  __davecc_atexit_lock(&exit_lock);
  if (dso == NULL) {
    ExitFunction* entries = exit_functions;
    exit_functions = NULL;
    __davecc_atexit_unlock(&exit_lock);
    return entries;
  }

  ExitFunction* entries = NULL;
  ExitFunction** entries_tail = &entries;
  ExitFunction** link = &exit_functions;
  while (*link != NULL) {
    ExitFunction* entry = *link;
    if (entry->dso == dso) {
      *link = entry->next;
      entry->next = NULL;
      *entries_tail = entry;
      entries_tail = &entry->next;
    } else {
      link = &entry->next;
    }
  }
  __davecc_atexit_unlock(&exit_lock);
  return entries;
}

static void InvokeExitFunction(ExitFunction* entry) {
  if (entry->has_argument) {
    entry->function.cxx(entry->argument);
  } else {
    entry->function.plain();
  }
  free(entry);
}

void __cxa_finalize(void* dso) {
  ExitFunction* entries;
  while ((entries = DetachExitFunctions(dso)) != NULL) {
    while (entries != NULL) {
      ExitFunction* entry = entries;
      entries = entry->next;
      InvokeExitFunction(entry);
    }
  }
}

#if !defined(__x86_64__)
void __davecc_finalize(void) {
  __cxa_finalize(NULL);
}
#endif

// If atexit is called from an atexit func it must be called after all
// currently registered atexit funcs.
void exit(int status) {
  __davecc_run_fini();
  syscall(SYS_EXIT_CLEAN, status);
  _Exit(status);
}
