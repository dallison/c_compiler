#include "test_framework.h"

#include <cxxabi.h>
#include <exception>
#include <cstdlib>

#include "eh_cxa_internal.h"

static int destructor_calls;

static void TestDestructor(void* object) {
  (void)object;
  destructor_calls++;
}

static int terminate_hook_calls;

static void TestTerminateHook(void) {
  terminate_hook_calls++;
  std::abort();
}

int TestEHCxaAllocateFree(void) {
  int failures = 0;
  void* object = __cxa_allocate_exception(16);
  CHECK(object != NULL, failures);

  struct __cxa_exception* header = __davecc_eh_header_from_object(object);
  CHECK(header != NULL, failures);
  CHECK(__davecc_eh_object_from_header(header) == object, failures);

  char* bytes = (char*)object;
  bytes[0] = 'a';
  __cxa_free_exception(object);
  return failures;
}

int TestEHCxaGlobalsTLS(void) {
  int failures = 0;
  struct __cxa_eh_globals* globals = __cxa_get_globals();
  CHECK(globals != NULL, failures);
  CHECK_EQ((long)globals->uncaughtExceptions, 0, failures);
  CHECK(globals->caughtExceptions == NULL, failures);
  CHECK(__cxa_get_globals() == globals, failures);
  return failures;
}

int TestEHCxaHandlerCount(void) {
  int failures = 0;
  destructor_calls = 0;

  void* object = __cxa_allocate_exception(sizeof(int));
  struct __cxa_exception* header = __davecc_eh_header_from_object(object);
  header->exceptionDestructor = TestDestructor;
  *(int*)object = 7;

  void* caught = __cxa_begin_catch(&header->unwindHeader);
  CHECK(caught == object, failures);
  CHECK_EQ((long)__davecc_eh_current_caught_header()->handlerCount, 1,
           failures);

  void* caught_again = __cxa_begin_catch(&header->unwindHeader);
  CHECK(caught_again == object, failures);
  CHECK_EQ((long)__davecc_eh_current_caught_header()->handlerCount, 2,
           failures);

  __cxa_end_catch();
  CHECK_EQ((long)__davecc_eh_current_caught_header()->handlerCount, 1,
           failures);
  CHECK_EQ((long)destructor_calls, 0, failures);

  __cxa_end_catch();
  CHECK(__davecc_eh_current_caught_header() == NULL, failures);
  CHECK_EQ((long)destructor_calls, 1, failures);
  return failures;
}

int TestEHCxaRethrowState(void) {
  int failures = 0;

  void* object = __cxa_allocate_exception(sizeof(int));
  struct __cxa_exception* header = __davecc_eh_header_from_object(object);
  header->exceptionDestructor = (void (*)(void*))0;
  *(int*)object = 99;

  __cxa_begin_catch(&header->unwindHeader);
  CHECK_EQ((long)__davecc_eh_uncaught_exceptions(), 0, failures);

  header->handlerCount--;
  struct __cxa_eh_globals* globals = __cxa_get_globals();
  globals->caughtExceptions = (struct __cxa_exception*)0;
  __davecc_eh_install_active_exception(header, object);
  CHECK_EQ((long)__davecc_eh_uncaught_exceptions(), 1, failures);
  CHECK(__davecc_eh_current_caught_header() == NULL, failures);
  __cxa_free_exception(object);
  return failures;
}

int TestEHCxaTerminateHooks(void) {
  int failures = 0;
  terminate_hook_calls = 0;
  std::terminate_handler previous = std::set_terminate(TestTerminateHook);
  CHECK(std::get_terminate() == TestTerminateHook, failures);
  std::terminate_handler restored = std::set_terminate(previous);
  CHECK(restored == TestTerminateHook, failures);
  return failures;
}

int TestUnwindEndOfStack(void) {
  int failures = 0;
  _Unwind_Exception exception = {};
  exception.exception_class = DAVECC_EH_EXCEPTION_CLASS;
  CHECK_EQ((long)_Unwind_RaiseException(&exception),
           (long)_URC_END_OF_STACK, failures);
  return failures;
}

extern "C" int TestEHCxa(void) {
  return TestEHCxaAllocateFree() + TestEHCxaGlobalsTLS() +
         TestEHCxaHandlerCount() + TestEHCxaRethrowState() +
         TestEHCxaTerminateHooks() + TestUnwindEndOfStack();
}
