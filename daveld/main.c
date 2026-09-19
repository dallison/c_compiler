//
//  main.c
//  daveld
//
//  Standalone DaveCC linker.  Same engine davecc uses; the usual extra
//  is -r / --relocatable, which concatenates objects into one ET_REL file.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dstring.h"
#include "linker_main.h"

static void Usage(FILE* fp, const char* program_name) {
  fprintf(fp,
          "usage: %s [option] file...\n"
          "\n"
          "Link ELF object files and archives.  Without -r the output is an\n"
          "executable or shared object; with -r it is a relocatable object.\n"
          "\n"
          "Options:\n"
          "  -h, --help            Show this help message\n"
          "  -o FILE               Output file (default a.out, or a.o with -r)\n"
          "  -r, --relocatable     Combine inputs into one relocatable object\n"
          "  -shared               Build a shared object\n"
          "  -static               Fully static executable\n"
          "  -dynamic              Dynamic executable\n"
          "  -e SYMBOL             Entry symbol\n"
          "  -lNAME                Search for libNAME\n"
          "  -LDIR                 Add DIR to the library search path\n"
          "  -IINTERP              Dynamic interpreter path\n"
          "  -T SCRIPT, --script   GNU ld / LLVM lld linker script\n"
          "  -tLAYOUT, --layout=   Built-in layout (program, rom, introm)\n"
          "  -rpath PATH           Runtime library search path\n"
          "  -origin ADDR          Load address origin\n"
          "  -bind-now             Resolve dynamic symbols at load\n"
          "  -whole-archive        Include every archive member\n"
          "  -no-whole-archive     Restore archive member extraction\n"
          "  --gc-sections         Discard unreferenced sections\n"
          "  --no-gc-sections      Keep unreferenced sections\n"
          "  --print-gc-sections   Report discarded sections\n",
          program_name);
}

int main(int argc, char** argv) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      Usage(stdout, argv[0]);
      return 0;
    }
  }

  String* output = Link(argc, argv);
  if (output == NULL) {
    return 1;
  }
  StringDelete(output);
  return 0;
}
