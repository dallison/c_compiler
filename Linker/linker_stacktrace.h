#ifndef LINKER_STACKTRACE_H
#define LINKER_STACKTRACE_H

struct Linker;

void LinkerStacktracePrepare(struct Linker* linker);
void LinkerStacktraceFinalize(struct Linker* linker);
void LinkerStacktraceDestruct(struct Linker* linker);

#endif /* LINKER_STACKTRACE_H */
