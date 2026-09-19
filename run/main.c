#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "elf_reader.h"

typedef struct {
  int machine;
  const char* name;
  const char* environment_name;
  const char* interpreter;
  bool force_interpret;
  bool needs_rom;
} RunTarget;

static const RunTarget run_targets[] = {
    {ELF_MACHINE_TYPE_PCODE, "pcode", "PCODE", "pcode", false, false},
    {ELF_MACHINE_TYPE_BPF, "bpf", "BPF", "bpf", true, false},
    {ELF_MACHINE_TYPE_RISC_V, "riscv", "RISCV", "riscv", false, false},
    {ELF_MACHINE_TYPEW65C02, "6502", "6502", "6502", false, true},
    {ELF_MACHINE_TYPE_AARCH64, "aarch64", "AARCH64", "aarch64", true,
     false},
    {ELF_MACHINE_TYPE_ARM, "arm", "ARM", "arm", true, false},
    {ELF_MACHINE_TYPE_X86_64, "x86_64", "X86_64", "x86_64", true, false},
    {ELF_MACHINE_TYPE_X86, "x86", "X86", "x86_64", true, false},
};

static void Usage(FILE* fp, const char* program) {
  fprintf(fp, "usage: %s executable [argument ...]\n", program);
}

static bool CopyPath(char* out, size_t out_size, const char* path) {
  if (path == NULL || path[0] == '\0') {
    return false;
  }
  int n = snprintf(out, out_size, "%s", path);
  return n >= 0 && (size_t)n < out_size;
}

static bool JoinPath(char* out, size_t out_size, const char* directory,
                     const char* relative) {
  if (directory == NULL || directory[0] == '\0') {
    return false;
  }
  int n = snprintf(out, out_size, "%s/%s", directory, relative);
  return n >= 0 && (size_t)n < out_size;
}

static bool PathIsExecutable(const char* path) {
  return path != NULL && access(path, X_OK) == 0;
}

static bool PathIsReadable(const char* path) {
  return path != NULL && access(path, R_OK) == 0;
}

static bool PathDirectory(const char* path, char* out, size_t out_size) {
  const char* slash = strrchr(path, '/');
  if (slash == NULL) {
    return CopyPath(out, out_size, ".");
  }
  if (slash == path) {
    return CopyPath(out, out_size, "/");
  }
  size_t length = (size_t)(slash - path);
  if (length >= out_size) {
    return false;
  }
  memcpy(out, path, length);
  out[length] = '\0';
  return true;
}

static bool FindOnPath(const char* name, char* out, size_t out_size) {
  const char* path_env = getenv("PATH");
  if (path_env == NULL) {
    return false;
  }
  char* paths = strdup(path_env);
  if (paths == NULL) {
    return false;
  }
  bool found = false;
  char* save = NULL;
  for (char* directory = strtok_r(paths, ":", &save); directory != NULL;
       directory = strtok_r(NULL, ":", &save)) {
    if (directory[0] == '\0') {
      directory = ".";
    }
    char candidate[PATH_MAX];
    if (JoinPath(candidate, sizeof(candidate), directory, name) &&
        PathIsExecutable(candidate)) {
      found = CopyPath(out, out_size, candidate);
      break;
    }
  }
  free(paths);
  return found;
}

static bool InvocationDirectory(const char* argv0, char* out,
                                size_t out_size) {
  char invocation[PATH_MAX];
  if (strchr(argv0, '/') != NULL) {
    if (argv0[0] == '/') {
      if (!CopyPath(invocation, sizeof(invocation), argv0)) {
        return false;
      }
    } else {
      char cwd[PATH_MAX];
      if (getcwd(cwd, sizeof(cwd)) == NULL ||
          !JoinPath(invocation, sizeof(invocation), cwd, argv0)) {
        return false;
      }
    }
  } else if (!FindOnPath(argv0, invocation, sizeof(invocation))) {
    return false;
  }

  char resolved[PATH_MAX];
  const char* path = realpath(invocation, resolved);
  return PathDirectory(path != NULL ? path : invocation, out, out_size);
}

static bool TryExecutable(char* out, size_t out_size, const char* path) {
  return PathIsExecutable(path) && CopyPath(out, out_size, path);
}

static bool TryExecutableIn(char* out, size_t out_size, const char* directory,
                            const char* name) {
  char candidate[PATH_MAX];
  return JoinPath(candidate, sizeof(candidate), directory, name) &&
         TryExecutable(out, out_size, candidate);
}

static bool FindInterpreter(const RunTarget* target,
                            const char* invocation_directory, char* out,
                            size_t out_size) {
  char env_name[64];
  int env_length = snprintf(env_name, sizeof(env_name),
                            "DAVECC_%s_INTERPRETER",
                            target->environment_name);
  if (env_length > 0 && (size_t)env_length < sizeof(env_name) &&
      TryExecutable(out, out_size, getenv(env_name))) {
    return true;
  }
  if (TryExecutable(out, out_size, getenv("DAVECC_INTERPRETER"))) {
    return true;
  }

  const char* libexec = getenv("DAVECC_LIBEXEC_DIR");
  if (TryExecutableIn(out, out_size, libexec, target->interpreter) ||
      TryExecutableIn(out, out_size, invocation_directory,
                      target->interpreter)) {
    return true;
  }

  char relative[PATH_MAX];
  if (JoinPath(relative, sizeof(relative), invocation_directory,
               "../libexec/davecc") &&
      TryExecutableIn(out, out_size, relative, target->interpreter)) {
    return true;
  }

  const char* root = getenv("DAVECC_ROOT");
  if (JoinPath(relative, sizeof(relative), root, "bazel-bin") &&
      TryExecutableIn(out, out_size, relative, target->interpreter)) {
    return true;
  }
  if (JoinPath(relative, sizeof(relative), root, "libexec/davecc") &&
      TryExecutableIn(out, out_size, relative, target->interpreter)) {
    return true;
  }

  if (FindOnPath(target->interpreter, out, out_size)) {
    return true;
  }
  return CopyPath(out, out_size, target->interpreter);
}

static bool TryReadable(char* out, size_t out_size, const char* path) {
  return PathIsReadable(path) && CopyPath(out, out_size, path);
}

static bool TryReadableIn(char* out, size_t out_size, const char* directory,
                          const char* relative) {
  char candidate[PATH_MAX];
  return JoinPath(candidate, sizeof(candidate), directory, relative) &&
         TryReadable(out, out_size, candidate);
}

static bool Find6502ROM(const char* invocation_directory,
                        const char* interpreter, char* out, size_t out_size) {
  if (TryReadable(out, out_size, getenv("DAVECC_6502_ROM"))) {
    return true;
  }

  char directory[PATH_MAX];
  if (PathDirectory(interpreter, directory, sizeof(directory)) &&
      (TryReadableIn(out, out_size, directory,
                     "6502_support/6502rom.exe") ||
       TryReadableIn(out, out_size, directory,
                     "../../lib/davecc/6502rom.exe"))) {
    return true;
  }
  if (TryReadableIn(out, out_size, invocation_directory,
                    "6502_support/6502rom.exe") ||
      TryReadableIn(out, out_size, invocation_directory,
                    "../lib/davecc/6502rom.exe")) {
    return true;
  }

  const char* root = getenv("DAVECC_ROOT");
  return TryReadableIn(out, out_size, root,
                       "bazel-bin/6502_support/6502rom.exe") ||
         TryReadableIn(out, out_size, root, "lib/davecc/6502rom.exe");
}

static const RunTarget* TargetForMachine(int machine) {
  for (size_t i = 0; i < sizeof(run_targets) / sizeof(run_targets[0]); ++i) {
    if (run_targets[i].machine == machine) {
      return &run_targets[i];
    }
  }
  return NULL;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    Usage(stderr, argv[0]);
    return 2;
  }
  if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
    Usage(stdout, argv[0]);
    return 0;
  }

  String filename = {0};
  StringInit(&filename, argv[1]);
  ELFReaderFile elf;
  ELFReaderFileInit(&elf, &filename);
  if (!ELFReaderFileRead(&elf, 0, 0)) {
    fprintf(stderr, "%s: unable to read ELF executable '%s'\n", argv[0],
            argv[1]);
    ELFReaderFileDestruct(&elf);
    StringDestruct(&filename);
    return 1;
  }
  int machine = elf.header->machine;
  int type = elf.header->type;
  ELFReaderFileDestruct(&elf);
  StringDestruct(&filename);

  if (type != ET(exec) && type != ET(dyn)) {
    fprintf(stderr, "%s: '%s' is not an executable ELF file\n", argv[0],
            argv[1]);
    return 1;
  }

  const RunTarget* target = TargetForMachine(machine);
  if (target == NULL) {
    fprintf(stderr, "%s: unsupported ELF machine %d in '%s'\n", argv[0],
            machine, argv[1]);
    return 1;
  }

  char invocation_directory[PATH_MAX] = ".";
  InvocationDirectory(argv[0], invocation_directory,
                      sizeof(invocation_directory));

  char interpreter[PATH_MAX];
  if (!FindInterpreter(target, invocation_directory, interpreter,
                       sizeof(interpreter))) {
    fprintf(stderr, "%s: unable to locate the %s interpreter\n", argv[0],
            target->name);
    return 1;
  }

  char rom[PATH_MAX];
  bool have_rom =
      target->needs_rom &&
      Find6502ROM(invocation_directory, interpreter, rom, sizeof(rom));
  size_t extra_args = (target->force_interpret ? 1 : 0) + (have_rom ? 2 : 0);
  char** run_argv =
      calloc((size_t)argc + extra_args + 1, sizeof(*run_argv));
  if (run_argv == NULL) {
    fprintf(stderr, "%s: out of memory\n", argv[0]);
    return 1;
  }

  size_t next = 0;
  run_argv[next++] = interpreter;
  if (target->force_interpret) {
    run_argv[next++] = "-i";
  }
  if (have_rom) {
    run_argv[next++] = "-rom";
    run_argv[next++] = rom;
  }
  for (int i = 1; i < argc; ++i) {
    run_argv[next++] = argv[i];
  }
  run_argv[next] = NULL;

  execvp(interpreter, run_argv);
  fprintf(stderr, "%s: unable to execute %s interpreter '%s': %s\n", argv[0],
          target->name, interpreter, strerror(errno));
  free(run_argv);
  return 1;
}
