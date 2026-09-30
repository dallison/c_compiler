// Hosted Darwin names the standard streams ___stdinp / ___stdoutp / ___stderrp.
// davecc's <stdio.h> declares stdin / stdout / stderr, so user objects reference
// _stdin / _stdout / _stderr.  Alias those to the libSystem symbols.
//
// __builtin_return_address is not lowered on this target yet.  A null return
// keeps callers that only record a frame (coroutine wait traces) linkable.

__asm__(
    ".globl _stdin\n"
    ".set _stdin, ___stdinp\n"
    ".globl _stdout\n"
    ".set _stdout, ___stdoutp\n"
    ".globl _stderr\n"
    ".set _stderr, ___stderrp\n");

void* __davecc_builtin_return_address(unsigned long level)
    __asm__("___builtin_return_address");

void* __davecc_builtin_return_address(unsigned long level) {
  (void)level;
  return 0;
}
