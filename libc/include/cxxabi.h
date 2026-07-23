#ifndef __DAVECC_CXXABI_H
#define __DAVECC_CXXABI_H

#include <stddef.h>
#include <stdint.h>
#include <unwind.h>

#ifdef __cplusplus
namespace std {
class type_info;
}
extern "C" {
#else
struct type_info;
#endif

typedef void (*unexpected_handler)(void);
typedef void (*terminate_handler)(void);

struct __cxa_exception;
struct __cxa_eh_globals;

struct __cxa_exception {
#ifdef __cplusplus
  std::type_info* exceptionType;
#else
  struct type_info* exceptionType;
#endif
  void (*exceptionDestructor)(void*);
  unexpected_handler unexpectedHandler;
  terminate_handler terminateHandler;
  struct __cxa_exception* nextException;
  int handlerCount;
  int handlerSwitchValue;
  const unsigned char* actionRecord;
  const unsigned char* languageSpecificData;
  void* catchType;
  void* adjustedPtr;
  _Unwind_Exception unwindHeader;
};

struct __cxa_eh_globals {
  struct __cxa_exception* caughtExceptions;
  unsigned int uncaughtExceptions;
  struct __cxa_exception* nextPropagatingException;
  unexpected_handler unexpectedHandler;
  terminate_handler terminateHandler;
};

void* __cxa_allocate_exception(size_t thrown_size);
void __cxa_free_exception(void* thrown_exception);

void __cxa_throw(void* thrown_exception, struct type_info* tinfo,
                 void (*dest)(void*));

void* __cxa_begin_catch(void* exceptionObject);
void __cxa_end_catch(void);
void __cxa_rethrow(void);

struct __cxa_eh_globals* __cxa_get_globals(void);
struct __cxa_eh_globals* __cxa_get_globals_fast(void);

#ifdef __cplusplus
}
#endif

#endif /* __DAVECC_CXXABI_H */
