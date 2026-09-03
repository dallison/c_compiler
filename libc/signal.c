//
//  signal.c
//  c_compiler
//

#include <signal.h>
#include <stdlib.h>

static void (*handlers[SIGTERM + 1])(int);

static int valid_signal(int signal_number) {
  return signal_number >= SIGABRT && signal_number <= SIGTERM;
}

void (*signal(int signal_number, void (*handler)(int)))(int) {
  void (*previous)(int);
  if (!valid_signal(signal_number) || handler == SIG_ERR) {
    return SIG_ERR;
  }
  previous = handlers[signal_number];
  handlers[signal_number] = handler;
  return previous;
}

int raise(int signal_number) {
  void (*handler)(int);
  if (!valid_signal(signal_number)) {
    return 1;
  }
  handler = handlers[signal_number];
  if (handler == SIG_IGN) {
    return 0;
  }
  if (handler == SIG_DFL) {
    abort();
  }
  handler(signal_number);
  return 0;
}
