//
//  main.c
//  davecc
//
//  Created by David Allison on 6/27/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

// This is a driver for the C compiler, assembler, linker and interpreter.

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "dstring.h"
#include "vector.h"
#include "risc_v_assembler.h"
#include "risc_v32_assembler.h"
#include "xtensa_assembler.h"
#include "6502_assembler.h"
#include "p_code_assembler.h"
#include "x86_64_assembler.h"
#include "arm_assembler.h"
#include "bpf_assembler.h"
#include "compiler.h"
#include "errors.h"
#include "linker_main.h"
#include "lto_archive.h"
#include "module_archive.h"
#include "module_import.h"
#include "module_install.h"
#include "module_reachability.h"
#include "options.h"
#include "preprocessor.h"
#include "source.h"
#include "symbol_table.h"
#include "wasm32_link.h"

Assembler* NewAARCH64Assembler(String* infile, String* outfile);
void AARCH64AssemblerDestruct(Assembler* assembler);
void AssembleAARCH64Instruction(Assembler* assembler, String* word);
static bool DriverImportModule(void* ctx, const char* module_name);

typedef struct {
  String invocation_dir;
  String executable_dir;
  String include_dir;
  String lib_dir;
  String module_root;
  String module_dir;
} DriverResources;

static bool PathIsDirectory(const char* path) {
  struct stat st;
  return path != NULL && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool PathIsFile(const char* path) {
  struct stat st;
  return path != NULL && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static bool SetCanonicalDirectory(String* out, const char* path) {
  if (!PathIsDirectory(path)) {
    return false;
  }
  char resolved[PATH_MAX];
  if (realpath(path, resolved) == NULL) {
    return false;
  }
  StringSet(out, resolved);
  return true;
}

static void SetPathDirectory(String* out, const char* path) {
  const char* slash = strrchr(path, '/');
  if (slash == NULL) {
    StringSet(out, ".");
  } else if (slash == path) {
    StringSet(out, "/");
  } else {
    StringInitFromSegment(out, path, (size_t)(slash - path));
  }
}

static bool SetInvocationPath(String* out, const char* argv0) {
  if (strchr(argv0, '/') != NULL) {
    if (argv0[0] == '/') {
      StringSet(out, argv0);
      return true;
    }
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
      return false;
    }
    StringPrintf(out, "%s/%s", cwd, argv0);
    return true;
  }

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
  for (char* dir = strtok_r(paths, ":", &save); dir != NULL;
       dir = strtok_r(NULL, ":", &save)) {
    String candidate = {0};
    StringPrintf(&candidate, "%s/%s", dir, argv0);
    if (access(candidate.value, X_OK) == 0) {
      StringSetString(out, &candidate);
      found = true;
      StringDestruct(&candidate);
      break;
    }
    StringDestruct(&candidate);
  }
  free(paths);
  return found;
}

static bool TryResourceDirectory(String* out, const char* base,
                                 const char* relative) {
  if (base == NULL || base[0] == '\0') {
    return false;
  }
  String candidate = {0};
  if (relative == NULL || relative[0] == '\0') {
    StringInit(&candidate, base);
  } else {
    StringPrintf(&candidate, "%s/%s", base, relative);
  }
  bool found = SetCanonicalDirectory(out, candidate.value);
  StringDestruct(&candidate);
  return found;
}

static bool FindIncludeDirectory(DriverResources* resources) {
  const char* include_env = getenv("DAVECC_INCLUDE_DIR");
  if (TryResourceDirectory(&resources->include_dir, include_env, NULL)) {
    return true;
  }
  const char* root_env = getenv("DAVECC_ROOT");
  if (TryResourceDirectory(&resources->include_dir, root_env, "libc/include")) {
    return true;
  }

  // bazel-bin is a symlink. Resolve its lexical parent before probing the
  // source tree; appending ".." to the symlink would walk the output tree.
  String invocation_parent = {0};
  if (resources->invocation_dir.length != 0) {
    SetPathDirectory(&invocation_parent, resources->invocation_dir.value);
    if (TryResourceDirectory(&resources->include_dir,
                             invocation_parent.value, "libc/include")) {
      StringDestruct(&invocation_parent);
      return true;
    }
  }
  StringDestruct(&invocation_parent);

  String* roots[] = {
      &resources->invocation_dir,
      &resources->executable_dir,
  };
  const char* relatives[] = {
      "libc/include",
      "../libc/include",
      "../../include/davecc",
      "../include/davecc",
      "../include",
  };
  for (size_t i = 0; i < sizeof(roots) / sizeof(roots[0]); i++) {
    for (size_t j = 0; j < sizeof(relatives) / sizeof(relatives[0]); j++) {
      if (TryResourceDirectory(&resources->include_dir, roots[i]->value,
                               relatives[j])) {
        return true;
      }
    }
  }
  return TryResourceDirectory(&resources->include_dir, ".", "libc/include");
}

static bool FindLibraryDirectory(DriverResources* resources) {
  const char* lib_env = getenv("DAVECC_LIB_DIR");
  if (TryResourceDirectory(&resources->lib_dir, lib_env, NULL)) {
    return true;
  }
  const char* root_env = getenv("DAVECC_ROOT");
  if (TryResourceDirectory(&resources->lib_dir, root_env,
                           "bazel-bin/libc")) {
    return true;
  }

  String* roots[] = {
      &resources->invocation_dir,
      &resources->executable_dir,
  };
  const char* relatives[] = {
      "libc",
      "../../lib/davecc",
      "../lib/davecc",
  };
  for (size_t i = 0; i < sizeof(roots) / sizeof(roots[0]); i++) {
    for (size_t j = 0; j < sizeof(relatives) / sizeof(relatives[0]); j++) {
      if (TryResourceDirectory(&resources->lib_dir, roots[i]->value,
                               relatives[j])) {
        return true;
      }
    }
  }
  return TryResourceDirectory(&resources->lib_dir, ".", "bazel-bin/libc");
}

static bool FindModuleRoot(DriverResources* resources) {
  const char* module_env = getenv("DAVECC_MODULE_DIR");
  if (TryResourceDirectory(&resources->module_root, module_env, NULL)) {
    return true;
  }
  const char* root_env = getenv("DAVECC_ROOT");
  if (TryResourceDirectory(&resources->module_root, root_env,
                           "bazel-bin/modules")) {
    return true;
  }

  String* roots[] = {
      &resources->invocation_dir,
      &resources->executable_dir,
      &resources->lib_dir,
  };
  const char* relatives[] = {
      "modules",
      "../modules",
  };
  for (size_t i = 0; i < sizeof(roots) / sizeof(roots[0]); i++) {
    for (size_t j = 0; j < sizeof(relatives) / sizeof(relatives[0]); j++) {
      if (TryResourceDirectory(&resources->module_root, roots[i]->value,
                               relatives[j])) {
        return true;
      }
    }
  }
  return TryResourceDirectory(&resources->module_root, ".",
                              "bazel-bin/modules");
}

static void DriverResourcesInit(DriverResources* resources,
                                const char* argv0) {
  StringInit(&resources->invocation_dir, "");
  StringInit(&resources->executable_dir, "");
  StringInit(&resources->include_dir, "");
  StringInit(&resources->lib_dir, "");
  StringInit(&resources->module_root, "");
  StringInit(&resources->module_dir, "");

  String invocation;
  StringInit(&invocation, "");
  if (SetInvocationPath(&invocation, argv0)) {
    SetPathDirectory(&resources->invocation_dir, invocation.value);
    char resolved[PATH_MAX];
    if (realpath(invocation.value, resolved) != NULL) {
      SetPathDirectory(&resources->executable_dir, resolved);
    }
  }
  StringDestruct(&invocation);

  FindIncludeDirectory(resources);
  FindLibraryDirectory(resources);
  FindModuleRoot(resources);
}

typedef struct {
  const char* canonical_name;
  TargetOS os;
  const char* archive_name;
  const char* bazel_target;
  const char* startup_name;
  const char* startup_target;
  const char* shared_bazel_target;
  bool use_main_entry;
  bool static_only;
} TargetRuntime;

static const TargetRuntime target_runtimes[] = {
    {"pcode", kTargetOSNone, "libcpcode.a", "//:libc_pcode", NULL, NULL,
     "//:libc_pcode_shared", true, false},
    {"riscv", kTargetOSNone, "libcriscv.a", "//:libc_riscv", NULL, NULL,
     "//:libc_riscv_shared", false, false},
    {"riscv32", kTargetOSNone, "libcriscv32.a", "//:libc_riscv32", NULL, NULL,
     "//:libc_riscv32_shared", false, false},
    {"esp32", kTargetOSNone, "libcxtensa.a", "//:libc_xtensa",
     "esp32_start.o", "//:esp32_start", NULL, false, true},
    {"aarch64", kTargetOSNone, "libcaarch64.a", "//:libc_aarch64", NULL, NULL,
     "//:libc_aarch64_shared", true, false},
    {"arm", kTargetOSNone, "libcarm.a", "//:libc_arm", NULL, NULL,
     "//:libc_arm_shared", true, false},
    {"x86_64", kTargetOSNone, "libcx86_64.a", "//:libc_x86_64", NULL, NULL,
     "//:libc_x86_64_shared", true, false},
    {"x86", kTargetOSNone, "libcx86.a", "//:libc_x86", NULL, NULL,
     "//:libc_x86_shared", true, false},
    {"6502", kTargetOSNone, "libc65c02.a", "//:libc_65c02", NULL, NULL,
     NULL, false, true},
    {"65c02", kTargetOSNone, "libc65c02.a", "//:libc_65c02", NULL, NULL,
     NULL, false, true},
    // A wasm module is linked whole every time, and its startup lives in the
    // archive rather than in an object of its own, so there is nothing to
    // name here beyond the library itself.
    {"wasm32", kTargetOSNone, "libcwasm32.a", "//:libc_wasm32", NULL, NULL,
     NULL, false, true},
    {"aarch64", kTargetOSLinux, "libcaarch64_linux.a",
     "//:libc_aarch64_linux", "aarch64_linux_start.o",
     "//:aarch64_linux_start", "//:aarch64_linux_dynamic_runtime", false, true},
    {"x86_64", kTargetOSLinux, "libcx86_64_linux.a",
     "//:libc_x86_64_linux", "x86_64_linux_start.o",
     "//:x86_64_linux_start", "//:x86_64_linux_dynamic_runtime", false, true},
    {"arm", kTargetOSLinux, "libcarm_linux.a", "//:libc_arm_linux",
     "arm_linux_start.o", "//:arm_linux_start",
     "//:arm_linux_dynamic_runtime", false, true},
    {"riscv", kTargetOSLinux, "libcriscv_linux.a", "//:libc_riscv_linux",
     "riscv_linux_start.o", "//:riscv_linux_start",
     "//:riscv_linux_dynamic_runtime", false, true},
    {"riscv32", kTargetOSLinux, "libcriscv32_linux.a", "//:libc_riscv32_linux",
     "riscv32_linux_start.o", "//:riscv32_linux_start", NULL, false, true},
};

static bool TargetNameMatches(const char* target, const char* canonical) {
  if (strcmp(target, canonical) == 0) {
    return true;
  }
  if (strcmp(canonical, "pcode") == 0) {
    return strcmp(target, "p-code") == 0;
  }
  if (strcmp(canonical, "riscv") == 0) {
    return strcmp(target, "risc-v") == 0;
  }
  if (strcmp(canonical, "riscv32") == 0) {
    return strcmp(target, "risc-v32") == 0;
  }
  if (strcmp(canonical, "esp32") == 0) {
    return strcmp(target, "xtensa-esp32") == 0;
  }
  if (strcmp(canonical, "aarch64") == 0) {
    return strcmp(target, "armv8") == 0;
  }
  if (strcmp(canonical, "arm") == 0) {
    return strcmp(target, "armv7") == 0 ||
           strcmp(target, "armv7-a") == 0 ||
           strcmp(target, "arm32") == 0;
  }
  if (strcmp(canonical, "x86_64") == 0) {
    return strcmp(target, "x86-64") == 0;
  }
  if (strcmp(canonical, "x86") == 0) {
    return strcmp(target, "i386") == 0 || strcmp(target, "i486") == 0 ||
           strcmp(target, "i586") == 0 || strcmp(target, "i686") == 0 ||
           strcmp(target, "x86-32") == 0;
  }
  if (strcmp(canonical, "65c02") == 0) {
    return strcmp(target, "65C02") == 0;
  }
  if (strcmp(canonical, "wasm32") == 0) {
    return strcmp(target, "wasm") == 0;
  }
  if (strcmp(canonical, "bpf") == 0) {
    return strcmp(target, "bpfel") == 0 || strcmp(target, "ebpf") == 0;
  }
  return false;
}

static const TargetRuntime* FindTargetRuntime(const char* target) {
  if (target == NULL) {
    return NULL;
  }
  CompilerTargetTriple triple;
  CompilerTargetTripleInit(&triple);
  char error[256];
  if (!CompilerTargetTripleParse(&triple, target, error, sizeof(error))) {
    CompilerTargetTripleDestruct(&triple);
    return NULL;
  }
  const TargetRuntime* result = NULL;
  for (size_t i = 0;
       i < sizeof(target_runtimes) / sizeof(target_runtimes[0]); i++) {
    if (triple.os == target_runtimes[i].os &&
        TargetNameMatches(triple.architecture.value,
                          target_runtimes[i].canonical_name)) {
      result = &target_runtimes[i];
      break;
    }
  }
  CompilerTargetTripleDestruct(&triple);
  return result;
}

static bool VectorContainsCString(Vector* values, const char* value) {
  for (size_t i = 0; i < values->length; i++) {
    if (strcmp(values->value.p[i], value) == 0) {
      return true;
    }
  }
  return false;
}

static bool LinkerArgsContainArchive(Vector* linker_args,
                                     const char* archive_name) {
  for (size_t i = 1; i < linker_args->length; i++) {
    const char* arg = linker_args->value.p[i];
    const char* basename = strrchr(arg, '/');
    basename = basename == NULL ? arg : basename + 1;
    if (strcmp(basename, archive_name) == 0) {
      return true;
    }
  }
  return false;
}

static void AddDefaultSystemInclude(Vector* compiler_args,
                                    DriverResources* resources,
                                    bool needs_compiler) {
  if (!needs_compiler ||
      VectorContainsCString(compiler_args, "-nostdinc")) {
    return;
  }

  bool has_explicit_system_path =
      VectorContainsCString(compiler_args, "-isystem");
  if (resources->include_dir.length == 0 && !has_explicit_system_path) {
    fprintf(stderr,
            "unable to find DaveCC system headers; set DAVECC_INCLUDE_DIR "
            "or use -nostdinc\n");
    exit(1);
  }

  // Suppress the CWD-relative compiled-in fallback. Explicit command-line
  // -isystem paths are parsed later and therefore remain higher priority.
  VectorInsertBefore(compiler_args, 1, "-nostdinc");
  if (resources->include_dir.length != 0) {
    VectorInsertBefore(compiler_args, 2, "-isystem");
    VectorInsertBefore(compiler_args, 3, resources->include_dir.value);
  }
}

static void DriverTargetName(Vector* compiler_args, char* out,
                             size_t out_size) {
  const char* target = "x86_64";
  for (size_t i = 1; i < compiler_args->length; i++) {
    const char* arg = (const char*)VectorGet(compiler_args, i);
    if (strcmp(arg, "-target") == 0 && i + 1 < compiler_args->length) {
      target = (const char*)VectorGet(compiler_args, i + 1);
    } else if (strncmp(arg, "-target=", 8) == 0) {
      target = arg + 8;
    }
  }
  CompilerTargetTriple triple;
  CompilerTargetTripleInit(&triple);
  char error[256];
  if (!CompilerTargetTripleParse(&triple, target, error, sizeof(error))) {
    snprintf(out, out_size, "%s", target);
  } else if (strcmp(triple.architecture.value, "6502") == 0) {
    snprintf(out, out_size, "65c02");
  } else {
    snprintf(out, out_size, "%s", triple.architecture.value);
  }
  CompilerTargetTripleDestruct(&triple);
}

static void AddDefaultStandardModulePath(Vector* compiler_args,
                                         DriverResources* resources,
                                         bool needs_compiler) {
  if (!needs_compiler ||
      VectorContainsCString(compiler_args, "-nostdinc") ||
      resources->module_root.length == 0) {
    return;
  }
  char target[64];
  DriverTargetName(compiler_args, target, sizeof(target));
  if (!TryResourceDirectory(&resources->module_dir,
                            resources->module_root.value, target)) {
    return;
  }
  VectorAppend(compiler_args, "-fprebuilt-module-path");
  VectorAppend(compiler_args, resources->module_dir.value);
}

static bool SharedLibraryName(const char* archive, char* out, size_t out_size) {
  size_t n = strlen(archive);
  if (n < 2 || strcmp(archive + n - 2, ".a") != 0 || n + 2 > out_size) {
    return false;
  }
  memcpy(out, archive, n - 2);
  memcpy(out + n - 2, ".so", 4);
  return true;
}

static bool SharedCrtName(const char* archive, char* out, size_t out_size) {
  size_t n = strlen(archive);
  if (n < 2 || strcmp(archive + n - 2, ".a") != 0 || n + 6 > out_size) {
    return false;
  }
  memcpy(out, archive, n - 2);
  memcpy(out + n - 2, "_crt.a", 7);
  return true;
}

static void AddRuntimeFileFromDirectory(Vector* linker_args,
                                        Vector* owned_paths,
                                        const char* directory,
                                        const char* filename,
                                        const char* target) {
  String* path = NewEmptyString();
  StringPrintf(path, "%s/%s", directory, filename);
  if (!PathIsFile(path->value)) {
    fprintf(stderr,
            "unable to find DaveCC runtime file '%s'; build %s, set "
            "DAVECC_LIB_DIR, or use -nostdlib\n",
            path->value, target);
    StringDelete(path);
    exit(1);
  }
  VectorAppend(owned_paths, path);
  VectorAppend(linker_args, path->value);
}

static void AddDefaultRuntime(Vector* linker_args, Vector* owned_paths,
                              Vector* compiler_options,
                              DriverResources* resources) {
  if (OptionBoolValue(kOptionNoStandardLibraries, compiler_options, false) ||
      VectorContainsCString(linker_args, "-shared") ||
      VectorContainsCString(linker_args, "-r") ||
      VectorContainsCString(linker_args, "--relocatable")) {
    return;
  }

  String* target = OptionStringValue(kOptionTarget, compiler_options);
  const TargetRuntime* runtime =
      FindTargetRuntime(target == NULL ? NULL : target->value);
  if (runtime == NULL) {
    return;
  }

  bool dynamic = VectorContainsCString(linker_args, "-dynamic");
  if (dynamic) {
    if (VectorContainsCString(linker_args, "-static")) {
      fprintf(stderr, "-dynamic and -static cannot be used together\n");
      exit(1);
    }
    if (runtime->os == kTargetOSNone) {
      if (runtime->shared_bazel_target == NULL || runtime->static_only) {
        fprintf(stderr, "-dynamic is not supported on this target\n");
        exit(1);
      }
      if (resources->lib_dir.length == 0) {
        fprintf(stderr,
                "unable to find DaveCC shared libc; set DAVECC_LIB_DIR "
                "or use -nostdlib\n");
        exit(1);
      }
      char shared_name[64];
      char crt_name[64];
      if (!SharedLibraryName(runtime->archive_name, shared_name,
                             sizeof(shared_name)) ||
          !SharedCrtName(runtime->archive_name, crt_name, sizeof(crt_name))) {
        fprintf(stderr, "unable to derive shared libc name from '%s'\n",
                runtime->archive_name);
        exit(1);
      }
      if (!VectorContainsCString(linker_args, "-e")) {
        // Interpreter-profile images enter at main.  RISC-V's static CRT
        // _start wrapper is not part of the dynamic CRT, and a second
        // _start from a leftover DSO object would fail the link.
        VectorAppend(linker_args, "-e");
        VectorAppend(linker_args, "main");
      }
      if (!VectorContainsCString(linker_args, "-rpath")) {
        VectorAppend(linker_args, "-rpath");
        VectorAppend(linker_args, "$ORIGIN");
      }
      VectorAppend(linker_args, "-whole-archive");
      AddRuntimeFileFromDirectory(linker_args, owned_paths,
                                  resources->lib_dir.value, crt_name,
                                  runtime->shared_bazel_target);
      VectorAppend(linker_args, "-no-whole-archive");
      AddRuntimeFileFromDirectory(linker_args, owned_paths,
                                  resources->lib_dir.value, shared_name,
                                  runtime->shared_bazel_target);
      return;
    }
    bool is_x86_64 = strcmp(runtime->canonical_name, "x86_64") == 0;
    bool is_arm = strcmp(runtime->canonical_name, "arm") == 0;
    bool is_riscv = strcmp(runtime->canonical_name, "riscv") == 0;
    bool is_aarch64 = strcmp(runtime->canonical_name, "aarch64") == 0;
    if (runtime->os != kTargetOSLinux ||
        (!is_x86_64 && !is_arm && !is_riscv && !is_aarch64)) {
      fprintf(stderr,
              "-dynamic is currently supported only for x86_64, ARM, "
              "AArch64, and RISC-V Linux targets\n");
      exit(1);
    }
    if (resources->lib_dir.length == 0) {
      fprintf(stderr,
              "unable to find DaveCC dynamic runtime; set DAVECC_LIB_DIR "
              "or use -nostdlib\n");
      exit(1);
    }
    const char* interpreter =
        is_arm ? "-I/lib/ld-linux-armhf.so.3"
               : is_riscv ? "-I/lib/ld-linux-riscv64-lp64d.so.1"
               : is_aarch64 ? "-I/lib/ld-linux-aarch64.so.1"
                          : "-I/lib64/ld-linux-x86-64.so.2";
    const char* startup =
        is_arm ? "arm_linux_dynamic_start.o"
               : is_riscv ? "riscv_linux_dynamic_start.o"
               : is_aarch64 ? "aarch64_linux_dynamic_start.o"
                          : "x86_64_linux_dynamic_start.o";
    const char* runtime_target =
        is_arm ? "//:arm_linux_dynamic_runtime"
               : is_riscv ? "//:riscv_linux_dynamic_runtime"
               : is_aarch64 ? "//:aarch64_linux_dynamic_runtime"
                          : "//:x86_64_linux_dynamic_runtime";
    String runtime_dir = {0};
    StringInit(&runtime_dir, resources->lib_dir.value);
    const char* architecture_dir =
        is_arm ? "arm" : is_riscv ? "riscv" : is_aarch64 ? "aarch64" : NULL;
    if (architecture_dir != NULL) {
      String startup_path = {0};
      StringPrintf(&startup_path, "%s/%s", runtime_dir.value, startup);
      if (!PathIsFile(startup_path.value)) {
        StringAppend(&runtime_dir, "/");
        StringAppend(&runtime_dir, architecture_dir);
      }
      StringDestruct(&startup_path);
    }
    VectorAppend(linker_args, "-e");
    VectorAppend(linker_args, "_start");
    VectorAppend(linker_args, (void*)interpreter);
    VectorAppend(linker_args, "-bind-now");
    VectorAppend(linker_args, "-defer-init");
    VectorAppend(linker_args, "-rpath");
    VectorAppend(linker_args, "$ORIGIN");
    AddRuntimeFileFromDirectory(linker_args, owned_paths, runtime_dir.value,
                                startup, runtime_target);
    VectorAppend(linker_args, "-whole-archive");
    AddRuntimeFileFromDirectory(linker_args, owned_paths, runtime_dir.value,
                                "libdavecc_crt.a", runtime_target);
    VectorAppend(linker_args, "-no-whole-archive");
    AddRuntimeFileFromDirectory(linker_args, owned_paths, runtime_dir.value,
                                "libdavecc.so", runtime_target);
    StringDestruct(&runtime_dir);
    return;
  }

  if (runtime->static_only &&
      !VectorContainsCString(linker_args, "-static")) {
    VectorAppend(linker_args, "-static");
  }

  // Function-level ELF sections only shrink the image if the linker
  // discards the ones that nothing reachable references.
  if (!VectorContainsCString(linker_args, "--gc-sections") &&
      !VectorContainsCString(linker_args, "--no-gc-sections") &&
      (runtime->static_only ||
       OptionBoolValue(kOptionFunctionSections, compiler_options, false))) {
    VectorAppend(linker_args, "--gc-sections");
  }

  if (runtime->use_main_entry &&
      !VectorContainsCString(linker_args, "-e")) {
    VectorAppend(linker_args, "-e");
    VectorAppend(linker_args, "main");
  } else if (runtime->startup_name != NULL &&
             !VectorContainsCString(linker_args, "-e")) {
    VectorAppend(linker_args, "-e");
    VectorAppend(linker_args, "_start");
  }

  if (resources->lib_dir.length == 0) {
    fprintf(stderr,
            "unable to find DaveCC system libraries; set DAVECC_LIB_DIR "
            "or use -nostdlib\n");
    exit(1);
  }

  if (runtime->startup_name != NULL &&
      !LinkerArgsContainArchive(linker_args, runtime->startup_name)) {
    String* startup = NewEmptyString();
    StringPrintf(startup, "%s/%s", resources->lib_dir.value,
                 runtime->startup_name);
    if (!PathIsFile(startup->value)) {
      fprintf(stderr,
              "unable to find DaveCC startup object '%s'; build %s, set "
              "DAVECC_LIB_DIR, or use -nostdlib\n",
              startup->value, runtime->startup_target);
      StringDelete(startup);
      exit(1);
    }
    VectorAppend(owned_paths, startup);
    VectorAppend(linker_args, startup->value);
  }

  if (LinkerArgsContainArchive(linker_args, runtime->archive_name)) {
    return;
  }
  String* archive = NewEmptyString();
  StringPrintf(archive, "%s/%s", resources->lib_dir.value,
               runtime->archive_name);
  if (!PathIsFile(archive->value)) {
    fprintf(stderr,
            "unable to find DaveCC system library '%s'; build %s, set "
            "DAVECC_LIB_DIR, or use -nostdlib\n",
            archive->value, runtime->bazel_target);
    StringDelete(archive);
    exit(1);
  }
  VectorAppend(owned_paths, archive);
  VectorAppend(linker_args, archive->value);
}

// Flags the driver consumes itself rather than passing to the compiler.  They
// are parsed by ParseArg below; this table exists so that -help documents them
// alongside the compiler's own options.
static CompilerOptionDefinition driver_options[] = {
    {"-help", kCompilerOptionBool, kOptionDriver, false,
     "Print this help and exit; -h and --help do the same", kOptionGroupOverall},
    {"-", kCompilerOptionBool, kOptionDriver, false,
     "Read the translation unit from standard input; must be the only input",
     kOptionGroupOverall},
    {"@", kCompilerOptionString, kOptionDriver, true,
     "Read further command line options from <file>, one or more per line",
     kOptionGroupOverall, "file"},
    {"-l", kCompilerOptionString, kOptionDriver, true,
     "Link against library <name>", kOptionGroupLinking, "name"},
    {"-L", kCompilerOptionString, kOptionDriver, true,
     "Add <dir> to the library search path", kOptionGroupLinking, "dir"},
    {"-static", kCompilerOptionBool, kOptionDriver, false, "Link statically",
     kOptionGroupLinking},
    {"-dynamic", kCompilerOptionBool, kOptionDriver, false,
     "Link dynamically; implies -fPIC and -ftls-model=local-exec",
     kOptionGroupLinking},
    {"-shared", kCompilerOptionBool, kOptionDriver, false,
     "Create a shared library; implies -fPIC", kOptionGroupLinking},
    {"-r", kCompilerOptionBool, kOptionDriver, false,
     "Link a relocatable object (daveld -r); do not add CRT or libc",
     kOptionGroupLinking},
    {"-rpath", kCompilerOptionString, kOptionDriver, false,
     "Add <dir> to the runtime library search path", kOptionGroupLinking, "dir"},
    {"-e", kCompilerOptionString, kOptionDriver, false,
     "Use <symbol> as the entry point", kOptionGroupLinking, "symbol"},
    {"-origin", kCompilerOptionString, kOptionDriver, false,
     "Load the output at <address>", kOptionGroupLinking, "address"},
    {"--gc-sections", kCompilerOptionBool, kOptionDriver, false,
     "Discard sections no live symbol reaches", kOptionGroupLinking},
    {"--no-gc-sections", kCompilerOptionBool, kOptionDriver, false,
     "Keep unreferenced sections (default)", kOptionGroupLinking},
    {"--print-gc-sections", kCompilerOptionBool, kOptionDriver, false,
     "Report the sections --gc-sections discards", kOptionGroupLinking},
    {"-Wl,", kCompilerOptionString, kOptionDriver, true,
     "Pass <arg> straight through to the linker", kOptionGroupLinking, "arg"},
    {NULL, 0, 0, false, NULL},
};

static void PrintDriverHelp(void) {
  printf("DaveCC: a C and C++ compiler, assembler, linker and interpreter.\n");
  printf("\nUsage: davecc [options] file...\n\n");
  PrintHelpParagraph(
      "Inputs are handled by suffix: .c, .cc, .cpp, .cxx, .cppm, .ixx, .h, "
      ".hpp and .hxx are compiled, .s is assembled, and .o and everything "
      "else is given to the linker.  Without -c, -S, -fsyntax-only or -r the "
      "result is linked into an executable.  -r writes a relocatable object.",
      0);
  PrintCompilerHelp(driver_options);
  printf("\n");
  PrintHelpParagraph(
      "An option that takes a value accepts it either as the next argument or "
      "joined with '=', so -std=c++20 and '-std c++20' are the same.  Prefixed "
      "options such as -I, -D and -L are written joined: -Ipath.",
      0);
}

static int ParseArg(int i, int argc, char** argv,
                    Vector* compiler_args,
                    Vector* linker_args,
                    Vector* object_files,
                    Vector* asm_files, Vector* args_from_file,
                    bool* run_compiler, bool* compile_only,
                    bool* read_stdin, int* num_inputs) {
  if (strcmp(argv[i], "-") == 0) {
    // A lone "-" means: read the translation unit from standard input.  It is
    // only valid as the sole input file (enforced by the caller).
    VectorAppend(compiler_args, argv[i]);
    *run_compiler = true;
    *read_stdin = true;
    (*num_inputs)++;
    return i + 1;
  }
  if (argv[i][0] == '-') {
    // Option.
    String* option = NewString(argv[i]);
    if (StringStartsWith(option, "-Wl,")) {
      // Arg passed through to linker.
      VectorAppend(linker_args, argv[i]+4);
    } else if (StringEqual(option, "-c") || StringEqual(option, "-S") ||
               StringEqual(option, "-fsyntax-only")) {
      // Compile only flag.
      *compile_only = true;
      VectorAppend(compiler_args, argv[i]);
    } else if (StringEqual(option, "-o")) {
      // -o option is followed by a filename
      if (i == argc-1) {
        fprintf(stderr, "-o needs the name of a file");
        exit(1);
      }
      if (*compile_only) {
        // Pass through to compiler.
        VectorAppend(compiler_args, argv[i]);
        // Get next arg into compiler_args too.
        VectorAppend(compiler_args, argv[i+1]);
      } else {
        // Linker option
        VectorAppend(linker_args, argv[i]);
        // Get next arg into linker_args too.
        VectorAppend(linker_args, argv[i+1]);
      }
      i++;
    } else if (StringEqual(option, "-target")) {
      // -target option is followed by a target name
      if (i == argc-1) {
        fprintf(stderr, "-target needs a target name");
        exit(1);
      }
      // Pass through to compiler.
      VectorAppend(compiler_args, argv[i]);
      // Get next arg into compiler_args too.
      VectorAppend(compiler_args, argv[i+1]);
      i++;
    } else if (StringEqual(option, "-isystem")) {
      // -isystem option is followed by an include dir
      if (i == argc-1) {
        fprintf(stderr, "-isystem needs a directory");
        exit(1);
      }
      // Pass through to compiler.
      VectorAppend(compiler_args, argv[i]);
      // Get next arg into compiler_args too.
      VectorAppend(compiler_args, argv[i+1]);
        i++;
    } else if (StringEqual(option, "-I") || StringEqual(option, "-D") ||
               StringEqual(option, "-U")) {
      // GCC and Clang take the value of these either joined to the flag or as
      // the following argument, so "-I dir" is as good as "-Idir".  Route both
      // to the compiler: passed on alone the flag would carry an empty value,
      // and the directory or macro, having no source extension, would fall
      // through to the linker as an input file and be lost with no diagnostic.
      if (i == argc-1) {
        fprintf(stderr, "%s needs a value\n", option->value);
        exit(1);
      }
      VectorAppend(compiler_args, argv[i]);
      VectorAppend(compiler_args, argv[i+1]);
      i++;
    } else if (StringEqual(option, "-Xemit-module") ||
               StringEqual(option, "-Xload-module") ||
               StringEqual(option, "-fprebuilt-module-path") ||
               StringEqual(option, "-fmodule-file") ||
               StringEqual(option, "-fmodule-name") ||
               StringEqual(option, "-fmodule-output") ||
               StringEqual(option, "-fdeps-file") ||
               StringEqual(option, "-fdeps-format")) {
      // These C++20-module options are followed by a path.  The path does not
      // carry a recognized source extension, so route both the flag and its
      // value into compiler_args explicitly; otherwise the path token falls
      // through to the linker and the option ends up swallowing the following
      // source file as its value.
      if (i == argc-1) {
        fprintf(stderr, "%s needs a module path\n", option->value);
        exit(1);
      }
      VectorAppend(compiler_args, argv[i]);
      VectorAppend(compiler_args, argv[i+1]);
      i++;
    } else if (StringEqual(option, "-rpath")) {
      // -rpath option is followed by an include dir
      if (i == argc-1) {
        fprintf(stderr, "-rpath needs a directory");
        exit(1);
      }
      // Pass through to linker.
      VectorAppend(linker_args, argv[i]);
      // Get next arg into linker_args too.
      VectorAppend(linker_args, argv[i+1]);
      i++;
    } else if (StringEqual(option, "-error-limit")) {
      if (i == argc-1) {
        fprintf(stderr, "-error-limit needs an integer\n");
        exit(1);
      }
      VectorAppend(compiler_args, argv[i]);
      VectorAppend(compiler_args, argv[i+1]);
      i++;
    } else if (StringEqual(option, "-flisting-file")) {
      if (i == argc - 1) {
        fprintf(stderr, "-flisting-file needs a path\n");
        exit(1);
      }
      VectorAppend(compiler_args, argv[i]);
      VectorAppend(compiler_args, argv[i + 1]);
      i++;
    } else if (StringEqual(option, "-chdir")) {
      // -chdir option is followed by an include dir
      if (i == argc-1) {
        fprintf(stderr, "-chdir needs a directory");
        exit(1);
      }
      // Pass through to compiler and linker.
      VectorAppend(compiler_args, argv[i]);
      VectorAppend(linker_args, argv[i]);
      // Get next arg into compiler_args too.
      VectorAppend(compiler_args, argv[i+1]);
      VectorAppend(linker_args, argv[i+1]);
      i++;
          
    } else if (StringEqual(option, "-origin")) {
      // -origin option is followed by an address
      if (i == argc-1) {
        fprintf(stderr, "-origin needs a value");
        exit(1);
      }
      // Pass through to linker.
      VectorAppend(linker_args, argv[i]);
      // Get next arg into linker too.
      VectorAppend(linker_args, argv[i+1]);
      i++;
    } else if (StringEqual(option, "--gc-sections") ||
               StringEqual(option, "--no-gc-sections") ||
               StringEqual(option, "--print-gc-sections")) {
      VectorAppend(linker_args, argv[i]);
    } else if (StringEqual(option, "-e")) {
      if (i == argc - 1) {
        fprintf(stderr, "-e needs a symbol name\n");
        exit(1);
      }
      VectorAppend(linker_args, argv[i]);
      VectorAppend(linker_args, argv[i + 1]);
      i++;
    } else if (StringEqual(option, "-static")) {
      VectorAppend(linker_args, argv[i]);
    } else if (StringEqual(option, "-dynamic")) {
      VectorAppend(linker_args, argv[i]);
      VectorAppend(compiler_args, "-fPIC");
      VectorAppend(compiler_args, "-ftls-model=local-exec");
    } else if (StringEqual(option, "-shared")) {
      VectorAppend(linker_args, argv[i]);
      VectorAppend(compiler_args, "-fPIC");
    } else if (StringEqual(option, "-r") ||
               StringEqual(option, "--relocatable")) {
      VectorAppend(linker_args, argv[i]);
    } else if (StringStartsWith(option, "-l")) {
      VectorAppend(linker_args, argv[i]);
    } else if (StringStartsWith(option, "-L")) {
      VectorAppend(linker_args, argv[i]);
    } else {
      VectorAppend(compiler_args, argv[i]);
    }
    StringDelete(option);
  } else if (argv[i][0] == '@') {
    char* arg = &argv[i][1];
    // Args from file.
    FILE* fp = fopen(arg, "r");
    if (fp == NULL) {
      fprintf(stderr, "Unable to open compiler args file %s\n", argv[i]);
    } else {
      char buf[1024];
      while (fgets(buf, sizeof(buf), fp) != NULL) {
        String s;
        StringInit(&s, buf);
        StringTrim(&s);     // Contains newline.
        if (s.length == 0) {
          continue;
        }
        if (s.value[0] == '#') {
          continue;
        }
        // Split string into parts separated by space.  Each element
        // of the vector will be a String pointer which will be added
        // to the args_from_file vector.
        Vector parts = {0};
        StringSplit(&s, ' ', &parts);
        for (size_t i = 0; i < parts.length; i++) {
          String* part = parts.value.p[i];
          while (StringEndsWith(part, "\\")) {
            // If it ends in \ then append next part.
            StringReplace(part, part->length - 1, 1, "", 0);
            StringAppend(part, " ");
            i++;
            String* tail = parts.value.p[i];
            StringAppend(part, tail->value);
          }
          VectorAppend(args_from_file, part);
        }
        StringDestruct(&s);
        VectorDestruct(&parts);
      }
      fclose(fp);
      
      // Build a new argv vector pointing to the strings in args_from_file.
      int new_argc = (int)args_from_file->length;
      char** new_argv = malloc(sizeof(char*) * new_argc);
      char** p = new_argv;
      for (size_t i = 0; i < new_argc; i++) {
        String* s = args_from_file->value.p[i];
        *p++ = s->value;
      }
      int j = 0;
      while (j < new_argc) {
        j = ParseArg(j,
                     new_argc, new_argv, compiler_args,
                     linker_args, object_files, asm_files,
                     args_from_file, run_compiler, compile_only,
                     read_stdin, num_inputs);
      }
      free(new_argv);
    }
  } else {
    String arg;
    StringInit(&arg, argv[i]);
    if (StringEndsWith(&arg, ".c") ||
        StringEndsWith(&arg, ".cc") ||
        StringEndsWith(&arg, ".cpp") ||
        StringEndsWith(&arg, ".cxx") ||
        StringEndsWith(&arg, ".cppm") ||
        StringEndsWith(&arg, ".ixx") ||
        StringEndsWith(&arg, ".h") ||
        StringEndsWith(&arg, ".hpp") ||
        StringEndsWith(&arg, ".hxx")) {
      VectorAppend(compiler_args, argv[i]);
      *run_compiler = true;
      (*num_inputs)++;
    } else if (StringEndsWith(&arg, ".s")) {
      VectorAppend(asm_files, NewString(argv[i]));
      (*num_inputs)++;
    } else if (StringEndsWith(&arg, ".o")) {
      VectorAppend(linker_args, argv[i]);
      (*num_inputs)++;
    } else {
      // Unknown extension, add to linker args.
      VectorAppend(linker_args, argv[i]);
    }
    StringDestruct(&arg);
  }
  return i + 1;
}

static bool BoolOptionValue(Vector* options, int opt, bool def) {
  for (size_t i = 0; i < options->length; i++) {
    CompilerOptionValue* option = options->value.p[i];
    if (option->opt == opt) {
      return option->value.bvalue;
    }
  }
  return def;
}

static const char* StringOptionValue(Vector* options, int opt) {
  for (size_t i = 0; i < options->length; i++) {
    CompilerOptionValue* option = options->value.p[i];
    if (option->opt == opt) {
      return option->value.svalue.value;
    }
  }
  return NULL;
}

static void AppendUniqueModuleId(Vector* names, const ModuleId* id) {
  String formatted;
  StringInit(&formatted, "");
  ModuleIdFormat(id, &formatted);
  for (size_t i = 0; i < names->length; i++) {
    String* existing = (String*)VectorGet(names, i);
    if (StringEqualString(existing, &formatted)) {
      StringDestruct(&formatted);
      return;
    }
  }
  VectorAppend(names, NewString(formatted.value));
  StringDestruct(&formatted);
}

typedef struct {
  String provided;
  String primary_module;
  Vector required;  // owned String*
  bool provides;
  bool is_interface;
} ModuleDependencyScan;

static void ModuleDependencyScanInit(ModuleDependencyScan* scan) {
  StringInit(&scan->provided, "");
  StringInit(&scan->primary_module, "");
  VectorInit(&scan->required);
  scan->provides = false;
  scan->is_interface = false;
}

static void ModuleDependencyScanDestruct(ModuleDependencyScan* scan) {
  StringDestruct(&scan->provided);
  StringDestruct(&scan->primary_module);
  VectorDestructWithContents(&scan->required,
                             (VectorElementDestructor)StringDelete,
                             /*free_element=*/false);
}

static bool LexAtContextualKeyword(Lex* lex, const char* spelling) {
  return LexLookingAt(lex, TOK(identifier)) &&
         StringEqual(&lex->spelling, spelling);
}

static void AppendUniqueModuleName(Vector* names, const char* name) {
  for (size_t i = 0; i < names->length; i++) {
    if (StringEqual((String*)VectorGet(names, i), name)) {
      return;
    }
  }
  VectorAppend(names, NewString(name));
}

static bool ScanDottedModuleName(Lex* lex, String* out) {
  if (!LexLookingAt(lex, TOK(identifier))) {
    return false;
  }
  StringAppendString(out, &lex->spelling);
  LexNextToken(lex);
  while (LexMatch(lex, TOK(dot))) {
    if (!LexLookingAt(lex, TOK(identifier))) {
      return false;
    }
    StringAppendChar(out, '.');
    StringAppendString(out, &lex->spelling);
    LexNextToken(lex);
  }
  return true;
}

static void AppendScannedHeaderToken(Lex* lex, String* out) {
  if (LexLookingAt(lex, TOK(identifier))) {
    StringAppendString(out, &lex->spelling);
  } else if (LexLookingAt(lex, TOK(number)) ||
             LexLookingAt(lex, TOK(fnumber))) {
    StringAppendString(out, &lex->literal_spelling);
  } else {
    StringAppend(out, TokenName(lex->current_token));
  }
}

static bool ScanModuleName(Lex* lex, String* out, bool allow_header_name,
                           bool* relative_partition) {
  StringInit(out, "");
  if (relative_partition != NULL) {
    *relative_partition = false;
  }
  if (allow_header_name && LexLookingAt(lex, TOK(string))) {
    StringAppendChar(out, '"');
    StringAppendString(out, &lex->spelling);
    StringAppendChar(out, '"');
    LexNextToken(lex);
    return true;
  }
  if (allow_header_name && LexMatch(lex, TOK(less))) {
    StringAppendChar(out, '<');
    while (!LexEof(lex) && !LexLookingAt(lex, TOK(greater))) {
      AppendScannedHeaderToken(lex, out);
      LexNextToken(lex);
    }
    if (!LexMatch(lex, TOK(greater))) {
      return false;
    }
    StringAppendChar(out, '>');
    return true;
  }
  if (LexMatch(lex, TOK(colon))) {
    if (relative_partition != NULL) {
      *relative_partition = true;
    }
    StringAppendChar(out, ':');
    if (!LexLookingAt(lex, TOK(identifier))) {
      return false;
    }
    StringAppendString(out, &lex->spelling);
    LexNextToken(lex);
    return true;
  }
  if (!ScanDottedModuleName(lex, out)) {
    return false;
  }
  if (LexMatch(lex, TOK(colon))) {
    if (!LexLookingAt(lex, TOK(identifier))) {
      return false;
    }
    StringAppendChar(out, ':');
    StringAppendString(out, &lex->spelling);
    LexNextToken(lex);
  }
  return true;
}

static void SetPrimaryModuleName(ModuleDependencyScan* scan,
                                 const String* module_name) {
  const char* colon = strchr(module_name->value, ':');
  if (colon == NULL) {
    StringSetString(&scan->primary_module, (String*)module_name);
    return;
  }
  StringClear(&scan->primary_module);
  StringAppendSegment(&scan->primary_module, module_name->value,
                      (size_t)(colon - module_name->value));
}

static bool ScanModuleDependencies(const char* input, Vector* options,
                                   Vector* target_opts,
                                   ModuleDependencyScan* scan) {
  ClearAllFiles();
  compiler = malloc(sizeof(Compiler));
  if (!CompilerInitFromFile(compiler, input, options, target_opts)) {
    fprintf(stderr, "Cannot open file %s\n", input);
    free(compiler);
    compiler = NULL;
    ClearAllFiles();
    return false;
  }

  bool header_unit = BoolOptionValue(options, kOptionModuleHeader, false);
  const char* requested_name = StringOptionValue(options, kOptionModuleName);
  if (header_unit) {
    if (requested_name == NULL || requested_name[0] == '\0') {
      fprintf(stderr,
              "-fmodule-header requires -fmodule-name with the header name\n");
      CompilerDelete(compiler);
      compiler = NULL;
      ClearAllFiles();
      return false;
    }
    StringSet(&scan->provided, requested_name);
    scan->provides = true;
    scan->is_interface = true;
  }

  Lex* lex = &compiler->lex;
  LexNextToken(lex);
  while (!LexEof(lex)) {
    bool exported = false;
    if (LexLookingAt(lex, TOK(export))) {
      exported = true;
      LexNextToken(lex);
    }

    if (LexAtContextualKeyword(lex, "module")) {
      LexNextToken(lex);
      if (LexMatch(lex, TOK(semicolon))) {
        continue;  // Global module fragment.
      }
      if (LexLookingAt(lex, TOK(colon))) {
        LexNextToken(lex);
        if (LexLookingAt(lex, TOK(private))) {
          LexNextToken(lex);
          LexMatch(lex, TOK(semicolon));
          continue;
        }
      }
      String name;
      bool relative = false;
      if (ScanModuleName(lex, &name, false, &relative) &&
          LexMatch(lex, TOK(semicolon))) {
        bool partition = strchr(name.value, ':') != NULL;
        StringSetString(&scan->provided, &name);
        SetPrimaryModuleName(scan, &name);
        scan->provides = exported || partition;
        scan->is_interface = exported;
        if (!exported && !partition) {
          AppendUniqueModuleName(&scan->required, name.value);
        }
      }
      StringDestruct(&name);
      continue;
    }

    if (LexAtContextualKeyword(lex, "import")) {
      LexNextToken(lex);
      String name;
      bool relative = false;
      if (ScanModuleName(lex, &name, true, &relative) &&
          LexMatch(lex, TOK(semicolon))) {
        if (relative && scan->primary_module.length > 0) {
          String resolved;
          StringInit(&resolved, scan->primary_module.value);
          StringAppendString(&resolved, &name);
          AppendUniqueModuleName(&scan->required, resolved.value);
          StringDestruct(&resolved);
        } else {
          AppendUniqueModuleName(&scan->required, name.value);
        }
      }
      StringDestruct(&name);
      continue;
    }

    if (!exported) {
      LexNextToken(lex);
    }
  }

  bool ok = NumErrors() == 0;
  CompilerDelete(compiler);
  compiler = NULL;
  ClearAllFiles();
  return ok;
}

static void WriteJsonString(FILE* out, const char* value) {
  fputc('"', out);
  for (const unsigned char* p = (const unsigned char*)value; *p != '\0'; p++) {
    switch (*p) {
      case '"':
        fputs("\\\"", out);
        break;
      case '\\':
        fputs("\\\\", out);
        break;
      case '\n':
        fputs("\\n", out);
        break;
      case '\r':
        fputs("\\r", out);
        break;
      case '\t':
        fputs("\\t", out);
        break;
      default:
        if (*p < 0x20) {
          fprintf(out, "\\u%04x", *p);
        } else {
          fputc(*p, out);
        }
        break;
    }
  }
  fputc('"', out);
}

static void DefaultObjectPath(const char* input, String* output) {
  StringInit(output, input);
  char* slash = strrchr(output->value, '/');
  char* dot = strrchr(output->value, '.');
  if (dot != NULL && (slash == NULL || dot > slash)) {
    StringReplace(output, (size_t)(dot - output->value),
                  output->length - (size_t)(dot - output->value), ".o", 2);
  } else {
    StringAppend(output, ".o");
  }
}

static void CollectCompilerInputFiles(Vector* compiler_options,
                                      Vector* sources) {
  for (size_t i = 0; i < compiler_options->length; i++) {
    CompilerOptionValue* opt = compiler_options->value.p[i];
    if (opt->opt == kOptionInputFile) {
      VectorAppend(sources, opt->value.svalue.value);
    }
  }
}

static bool WriteModuleDependencies(const char* path, const char* input,
                                    const char* primary_output,
                                    const ModuleDependencyScan* scan) {
  FILE* out = fopen(path, "w");
  if (out == NULL) {
    fprintf(stderr, "Cannot write dependency file %s\n", path);
    return false;
  }
  fputs("{\n  \"version\": 1,\n  \"revision\": 0,\n  \"rules\": [\n"
        "    {\n      \"primary-output\": ",
        out);
  WriteJsonString(out, primary_output);
  if (scan->provides) {
    fputs(",\n      \"provides\": [\n        {\n"
          "          \"logical-name\": ",
          out);
    WriteJsonString(out, scan->provided.value);
    fputs(",\n          \"source-path\": ", out);
    WriteJsonString(out, input);
    fprintf(out, ",\n          \"is-interface\": %s\n        }\n      ]",
            scan->is_interface ? "true" : "false");
  }
  if (scan->required.length > 0) {
    fputs(",\n      \"requires\": [\n", out);
    for (size_t i = 0; i < scan->required.length; i++) {
      fputs("        { \"logical-name\": ", out);
      WriteJsonString(out,
                      ((String*)VectorGet((Vector*)&scan->required, i))->value);
      fputs(i + 1 == scan->required.length ? " }\n" : " },\n", out);
    }
    fputs("      ]", out);
  }
  fputs("\n    }\n  ]\n}\n", out);
  bool ok = fclose(out) == 0;
  if (!ok) {
    fprintf(stderr, "Failed to finish dependency file %s\n", path);
  }
  return ok;
}

// Hidden -Xemit-module hook: compile the front end of `input` and serialize the
// resulting module interface (the global namespace tree plus exported
// file-scope symbols) to a module archive at `module_path`.  Returns true on
// success.
static bool EmitModule(const char* input, Vector* options, Vector* target_opts,
                       const char* module_path) {
  ClearAllFiles();
  compiler = malloc(sizeof(Compiler));
  if (!CompilerInitFromFile(compiler, input, options, target_opts)) {
    fprintf(stderr, "Cannot open file %s\n", input);
    free(compiler);
    compiler = NULL;
    ClearAllFiles();
    return false;
  }
  TranslationUnitImportState* import_state =
      TranslationUnitImportStateCreate(options);
  CompilerSetImportState(
      import_state,
      (TranslationUnitImportReleaseFn)TranslationUnitImportStateRelease);
  SetModuleImportHandler(DriverImportModule, import_state);
  bool header_unit =
      BoolOptionValue(options, kOptionModuleHeader, false);
  if (header_unit) {
    compiler->syntax.export_depth = 1;
  }
  bool ok = CompileFrontEndOnly(compiler);
  if (ok) {
    if (!header_unit &&
        !ModuleUnitIsInterfaceUnit(&compiler->module_unit) &&
        compiler->module_unit.kind != kModuleUnitKindInternalPartition) {
      fprintf(stderr,
              "Cannot emit module artifact: input is not an importable module "
              "unit\n");
      ok = false;
    }
  }
  if (ok) {
    ModuleReachability reachability;
    ModuleReachabilityInit(&reachability);
    if (!ModuleReachabilityBuild(&reachability, header_unit)) {
      fprintf(stderr, "Cannot emit module interface: %s\n",
              reachability.error);
      ok = false;
    }

    Vector ns_roots;
    VectorInit(&ns_roots);
    VectorAppend(&ns_roots, compiler->global_namespace);

    Vector dependencies;
    Vector reexports;
    Vector header_macros;
    VectorInit(&dependencies);
    VectorInit(&reexports);
    VectorInit(&header_macros);
    for (size_t i = 0; i < compiler->module_unit.imports.length; i++) {
      ModuleImportRef* import =
          (ModuleImportRef*)VectorGet(&compiler->module_unit.imports, i);
      AppendUniqueModuleId(&dependencies, &import->id);
      if (import->is_export_import) {
        AppendUniqueModuleId(&reexports, &import->id);
      }
    }

    String module_name;
    StringInit(&module_name, input);
    const char* requested_module_name =
        StringOptionValue(options, kOptionModuleName);
    if (requested_module_name != NULL) {
      StringSet(&module_name, requested_module_name);
    } else if (compiler->module_unit.id.name.length > 0) {
      StringClear(&module_name);
      ModuleIdFormat(&compiler->module_unit.id, &module_name);
    }
    if (header_unit && compiler->syntax.lex != NULL &&
        compiler->syntax.lex->preprocessor != NULL) {
      PreprocessorCollectHeaderUnitMacros(
          compiler->syntax.lex->preprocessor, &header_macros);
    }
    ModuleWriteRequest req = {
        .module_name = module_name.value,
        .target_triple = compiler->target_triple.canonical.value,
        .compiler_version = "davecc",
        .flags = header_unit
            ? kModuleArchiveHeaderUnit
            : compiler->module_unit.kind == kModuleUnitKindPrimaryInterface
                ? kModuleArchivePrimaryInterface
                : compiler->module_unit.kind ==
                          kModuleUnitKindInterfacePartition
                      ? kModuleArchiveInterfacePartition
                      : kModuleArchiveInternalPartition,
        .root_symbols = &reachability.exported_roots,
        .root_namespaces = &ns_roots,
        .dependencies = &dependencies,
        .reexports = &reexports,
        .header_macros = &header_macros,
    };
    if (ok) {
      ok = ModuleWrite(module_path, &req);
    }
    if (!ok) {
      const char* detail = ModuleWriteLastError();
      if (detail != NULL) {
        fprintf(stderr, "Failed to write module %s: %s\n", module_path,
                detail);
      } else {
        fprintf(stderr, "Failed to write module %s\n", module_path);
      }
    }
    VectorDestruct(&ns_roots);
    VectorDestructWithContents(
        &dependencies, (VectorElementDestructor)StringDelete,
        /*free_element=*/false);
    VectorDestructWithContents(&reexports,
                               (VectorElementDestructor)StringDelete,
                               /*free_element=*/false);
    VectorDestruct(&header_macros);
    StringDestruct(&module_name);
    ModuleReachabilityDestruct(&reachability);
  } else {
    fprintf(stderr, "Front end failed for %s\n", input);
  }
  CompilerDelete(compiler);
  compiler = NULL;
  SetModuleImportHandler(NULL, NULL);
  CompilerSetImportState(NULL, NULL);
  TranslationUnitImportStateDelete(import_state);
  ClearAllFiles();
  return ok;
}

static bool EmitModuleInChild(const char* input, Vector* options,
                              Vector* target_opts, const char* module_path) {
  pid_t pid = fork();
  if (pid < 0) {
    perror("Cannot start module compiler");
    return false;
  }
  if (pid == 0) {
    bool ok = EmitModule(input, options, target_opts, module_path);
    fflush(NULL);
    _exit(ok ? 0 : 1);
  }

  int status;
  while (waitpid(pid, &status, 0) < 0) {
    if (errno != EINTR) {
      perror("Cannot wait for module compiler");
      return false;
    }
  }
  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

// Hidden -Xload-module hook: load and verify a module archive, printing a short
// summary.  Requires a compiler global for the deserialized objects' arenas.
static bool LoadModule(const char* module_path, Vector* options) {
  ClearAllFiles();
  compiler = malloc(sizeof(Compiler));
  if (!CompilerInitFromString(compiler, module_path, "", options)) {
    return false;
  }
  CreateGlobalSymbolTables();

  LoadedModule loaded;
  bool ok = ModuleLoad(module_path, &loaded);
  if (ok) {
    printf("module %s: format v%u, target %s, %zu root symbol(s), "
           "%zu root namespace(s)\n",
           loaded.module_name.value, loaded.format_version,
           loaded.target_triple.value, loaded.root_symbols.length,
           loaded.root_namespaces.length);
    LoadedModuleReleaseGraph(&loaded);
  } else {
    fprintf(stderr, "Failed to load module %s\n", module_path);
  }
  CompilerDelete(compiler);
  compiler = NULL;
  if (ok) {
    LoadedModuleDestruct(&loaded);
  }
  ClearAllFiles();
  return ok;
}

// C++20 module import support for normal compilation.  Each translation unit
// gets its own TranslationUnitImportState so loaded module graphs are never
// reused after CompilerDelete.
static bool DriverImportModule(void* ctx, const char* module_name) {
  TranslationUnitImportState* state = (TranslationUnitImportState*)ctx;
  bool ok = TranslationUnitImportStateImport(state, module_name);
  if (!ok) {
    CompilerSetLastImportError(TranslationUnitImportStateLastError(state));
  }
  return ok;
}

int main(int argc, char * argv[]) {
  DriverResources resources;
  DriverResourcesInit(&resources, argv[0]);

  Vector asm_files = {0};
  Vector compiler_args = {0};
  Vector linker_args = {0};
  Vector object_files = {0};

  VectorAppend(&compiler_args, "");   // argv[0]
  VectorAppend(&linker_args, "");   // argv[0]

  // Storage for strings read from @file.
  Vector args_from_file = {0};
  
  bool compile_only = false;
  bool run_compiler = false;
  bool read_stdin = false;
  int num_inputs = 0;
  
  int i = 1;
  bool help = false;
  while (i < argc) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0 ||
        strcmp(argv[i], "--help") == 0) {
      help = true;
      break;
    }
    i = ParseArg(i, argc, argv, &compiler_args, &linker_args, &object_files,
                 &asm_files, &args_from_file, &run_compiler, &compile_only,
                 &read_stdin, &num_inputs);
  }
  
  if (help) {
    PrintDriverHelp();
    exit(0);
  }

  if (read_stdin && num_inputs > 1) {
    fprintf(stderr,
            "'-' (standard input) must be the only input file\n");
    exit(1);
  }

  AddDefaultStandardModulePath(&compiler_args, &resources,
                               run_compiler || asm_files.length > 0);
  AddDefaultSystemInclude(&compiler_args, &resources,
                          run_compiler || asm_files.length > 0);

  // Parse compiler options for C and asm files.
  Vector compiler_options;
  VectorInit(&compiler_options);
  Vector* target_opts = NULL;
  // Parse compiler options whenever any were supplied (compiler_args always
  // holds argv[0]); this also covers standalone module hooks like
  // -Xload-module that have no C input file.
  if (run_compiler || asm_files.length > 0 || compiler_args.length > 1) {
    target_opts = ParseOptions((int)compiler_args.length,
                 (char**)compiler_args.value.p,
                 &compiler_options);
  }
  
  String* deps_file = OptionStringValue(kOptionDepsFile, &compiler_options);
  String* deps_format = OptionStringValue(kOptionDepsFormat, &compiler_options);
  bool deps_scan_only =
      BoolOptionValue(&compiler_options, kOptionDepsScanOnly, false);
  if (deps_format != NULL && !StringEqual(deps_format, "p1689r5")) {
    fprintf(stderr, "Unsupported dependency format '%s'; use p1689r5\n",
            deps_format->value);
    exit(1);
  }
  if (deps_scan_only && deps_file == NULL) {
    fprintf(stderr, "-fdeps-scan-only requires -fdeps-file\n");
    exit(1);
  }
  if (deps_file != NULL) {
    const char* scan_input = NULL;
    size_t input_count = 0;
    for (size_t i = 0; i < compiler_options.length; i++) {
      CompilerOptionValue* opt = compiler_options.value.p[i];
      if (opt->opt == kOptionInputFile) {
        scan_input = opt->value.svalue.value;
        input_count++;
      }
    }
    if (input_count != 1) {
      fprintf(stderr, "-fdeps-file requires exactly one source input\n");
      exit(1);
    }
    ModuleDependencyScan scan;
    ModuleDependencyScanInit(&scan);
    bool ok =
        ScanModuleDependencies(scan_input, &compiler_options, target_opts, &scan);
    String default_output;
    String* output = OptionStringValue(kOptionOutputFile, &compiler_options);
    if (output == NULL) {
      DefaultObjectPath(scan_input, &default_output);
      output = &default_output;
    }
    if (ok) {
      ok = WriteModuleDependencies(deps_file->value, scan_input, output->value,
                                   &scan);
    }
    if (OptionStringValue(kOptionOutputFile, &compiler_options) == NULL) {
      StringDestruct(&default_output);
    }
    ModuleDependencyScanDestruct(&scan);
    if (!ok || deps_scan_only) {
      exit(ok ? 0 : 1);
    }
  }

  // Hidden C++20-module hooks.  -Xemit-module compiles the front end and writes
  // a module instead of an object file; -Xload-module loads and verifies one.
  String* emit_module = OptionStringValue(kOptionEmitModule, &compiler_options);
  String* load_module = OptionStringValue(kOptionLoadModule, &compiler_options);
  if (emit_module != NULL) {
    bool ok = true;
    for (size_t i = 0; i < compiler_options.length; i++) {
      CompilerOptionValue* opt = compiler_options.value.p[i];
      if (opt->opt == kOptionInputFile) {
        ok = EmitModule(opt->value.svalue.value, &compiler_options, target_opts,
                        emit_module->value) &&
             ok;
      }
    }
    exit(ok ? 0 : 1);
  }
  if (load_module != NULL) {
    exit(LoadModule(load_module->value, &compiler_options) ? 0 : 1);
  }

  // Public coordinated mode.  The module and object paths compile the
  // translation unit independently.  Isolate module emission in a child so its
  // process-global compiler arenas cannot contaminate object generation.
  String* module_output =
      OptionStringValue(kOptionModuleOutput, &compiler_options);
  const char* module_input = NULL;
  if (module_output != NULL) {
    size_t input_count = 0;
    for (size_t i = 0; i < compiler_options.length; i++) {
      CompilerOptionValue* opt = compiler_options.value.p[i];
      if (opt->opt == kOptionInputFile) {
        module_input = opt->value.svalue.value;
        input_count++;
      }
    }
    if (input_count != 1) {
      fprintf(stderr, "-fmodule-output requires exactly one source input\n");
      exit(1);
    }
    if (!EmitModuleInChild(module_input, &compiler_options, target_opts,
                           module_output->value)) {
      exit(1);
    }
  }

  // Any C files to compile, or LTO objects to lower at link time?
  Vector c_sources = {0};
  CollectCompilerInputFiles(&compiler_options, &c_sources);
  bool lto_flag = OptionBoolValue(kOptionLTO, &compiler_options, false);
  bool assembly_only =
      OptionBoolValue(kOptionAssemblyOutput, &compiler_options, false);
  bool syntax_only =
      OptionBoolValue(kOptionSyntaxOnly, &compiler_options, false);
  bool emit_lto_ir =
      compile_only && lto_flag && !assembly_only && !syntax_only;

  if (emit_lto_ir) {
    if (c_sources.length == 0) {
      fprintf(stderr, "-c -flto requires a source file\n");
      exit(1);
    }
    String* output = OptionStringValue(kOptionOutputFile, &compiler_options);
    if (output != NULL && c_sources.length != 1) {
      fprintf(stderr, "-c -flto -o requires exactly one source file\n");
      exit(1);
    }
    for (size_t i = 0; i < c_sources.length; i++) {
      const char* source = (const char*)c_sources.value.p[i];
      String object_path;
      if (output != NULL) {
        StringInit(&object_path, output->value);
      } else {
        DefaultObjectPath(source, &object_path);
      }
      TranslationUnitImportState* import_state =
          TranslationUnitImportStateCreate(&compiler_options);
      CompilerSetImportState(import_state,
                             (TranslationUnitImportReleaseFn)
                                 TranslationUnitImportStateRelease);
      SetModuleImportHandler(DriverImportModule, import_state);
      String* written = CompileLTOIRObject(source, &compiler_options,
                                           target_opts, object_path.value);
      SetModuleImportHandler(NULL, NULL);
      CompilerSetImportState(NULL, NULL);
      TranslationUnitImportStateDelete(import_state);
      StringDestruct(&object_path);
      if (written == NULL) {
        fprintf(stderr, "Failed to compile LTO object\n");
        exit(1);
      }
      StringDelete(written);
    }
  } else {
    Vector lto_files = {0};
    for (size_t i = 1; i < linker_args.length;) {
      const char* arg = (const char*)linker_args.value.p[i];
      if (arg == NULL || arg[0] == '-') {
        i++;
        continue;
      }
      if (LTOArchiveIsLTOObject(arg)) {
        VectorAppend(&lto_files, (void*)arg);
        VectorDeleteElement(&linker_args, i);
        continue;
      }
      size_t n = strlen(arg);
      // Opening every archive member just to look for LTO IR is only useful
      // when the user asked for LTO.  Doing it before a normal compile has a
      // side effect on ARM: C++ inline template bodies (iostream, vector
      // helpers) are omitted from the object and the later link reports them
      // as undefined.
      if (lto_flag && n >= 2 && strcmp(arg + n - 2, ".a") == 0) {
        bool has_native = false;
        Vector tmp_blobs = {0};
        Vector tmp_lens = {0};
        int nmem = LTOArchiveExtractLTOMembers(arg, &tmp_blobs, &tmp_lens,
                                               &has_native);
        for (size_t b = 0; b < tmp_blobs.length; b++) {
          free(tmp_blobs.value.p[b]);
        }
        VectorDestruct(&tmp_blobs);
        VectorDestruct(&tmp_lens);
        if (nmem > 0) {
          VectorAppend(&lto_files, (void*)arg);
          if (!has_native) {
            VectorDeleteElement(&linker_args, i);
            continue;
          }
        }
      }
      i++;
    }
    if (lto_flag || lto_files.length > 0) {
      for (size_t i = 0; i < c_sources.length; i++) {
        VectorAppend(&lto_files, c_sources.value.p[i]);
      }
    }

    if (lto_files.length > 0) {
      TranslationUnitImportState* import_state =
          TranslationUnitImportStateCreate(&compiler_options);
      CompilerSetImportState(import_state,
                             (TranslationUnitImportReleaseFn)
                                 TranslationUnitImportStateRelease);
      SetModuleImportHandler(DriverImportModule, import_state);

      Vector extra_roots = {0};
      bool mixed_native = false;
      bool shared_output = false;
      for (size_t i = 1; i < linker_args.length; i++) {
        const char* arg = (const char*)linker_args.value.p[i];
        if (arg == NULL) {
          continue;
        }
        if (strcmp(arg, "-shared") == 0) {
          shared_output = true;
        }
        if (arg[0] == '-') {
          if ((strcmp(arg, "-e") == 0 || strcmp(arg, "--entry") == 0) &&
              i + 1 < linker_args.length) {
            VectorAppend(&extra_roots, linker_args.value.p[i + 1]);
          } else if ((strcmp(arg, "-u") == 0 ||
                      strcmp(arg, "--undefined") == 0) &&
                     i + 1 < linker_args.length) {
            VectorAppend(&extra_roots, linker_args.value.p[i + 1]);
          } else if (strncmp(arg, "--undefined=", 12) == 0) {
            VectorAppend(&extra_roots, (void*)(arg + 12));
          }
          continue;
        }
        size_t n = strlen(arg);
        if ((n >= 2 && strcmp(arg + n - 2, ".o") == 0) ||
            (n >= 2 && strcmp(arg + n - 2, ".a") == 0)) {
          mixed_native = true;
        }
      }

      String* object_file =
          CompileLTOIRModules(&lto_files, &compiler_options, target_opts,
                              !mixed_native && !shared_output, &extra_roots);
      VectorDestruct(&extra_roots);
      SetModuleImportHandler(NULL, NULL);
      CompilerSetImportState(NULL, NULL);
      TranslationUnitImportStateDelete(import_state);

      if (object_file != NULL) {
        VectorAppend(&linker_args, object_file->value);
      } else {
        if (!syntax_only) {
          fprintf(stderr, "Failed to compile\n");
        }
        exit(1);
      }
    } else if (run_compiler) {
      for (size_t i = 0; i < compiler_options.length; i++) {
        CompilerOptionValue* opt = compiler_options.value.p[i];
        if (opt->opt == kOptionInputFile) {
          // One import store per translation unit: LoadedModule graphs must stay
          // alive through that compile and be released before CompilerDelete.
          TranslationUnitImportState* import_state =
              TranslationUnitImportStateCreate(&compiler_options);
          CompilerSetImportState(import_state,
                                 (TranslationUnitImportReleaseFn)
                                     TranslationUnitImportStateRelease);
          SetModuleImportHandler(DriverImportModule, import_state);

          String* object_file = CompileTranslationUnit(opt->value.svalue.value,
                                                       &compiler_options,
                                                       target_opts);
          SetModuleImportHandler(NULL, NULL);
          CompilerSetImportState(NULL, NULL);
          TranslationUnitImportStateDelete(import_state);

          if (object_file != NULL) {
            VectorAppend(&linker_args, object_file->value);
          } else {
            if (!syntax_only) {
              fprintf(stderr, "Failed to compile\n");
            }
            exit(1);
          }
        }
      }
    }
    VectorDestruct(&lto_files);
  }
  VectorDestruct(&c_sources);
  
  if (asm_files.length > 0) {
    String object_filename = {0};
    String target = {0};
    
    // See if we've been given an output filename as a compiler option.
    // Get the target from the options too.
    for (size_t i = 0; i < compiler_options.length; i++) {
      CompilerOptionValue* opt = compiler_options.value.p[i];
      if (opt->opt == kOptionOutputFile) {
        StringSet(&object_filename, opt->value.svalue.value);
      } else if (opt->opt == kOptionTarget) {
        StringSet(&target, opt->value.svalue.value);
      }
    }
    
    for (size_t i = 0; i < asm_files.length; i++) {
      String* asm_filename = asm_files.value.p[i];
      String output_filename;
      StringInit(&output_filename, object_filename.value);
      
      if (object_filename.length == 0) {
        StringSet(&output_filename, asm_filename->value);
        // No output file specified (no -o) so work it out.
        // If the file ends in ".s", make it ".o", otherwise append ".o".
        char* suffix = strstr(output_filename.value, ".s");
        if (suffix == NULL) {
          StringAppend(&output_filename, ".o");
        } else {
          // Overwrite 's' with 'o'.
          suffix[1] = 'o';
        }
      }
      
      // Create and run the assembler.
      // We need to initialize the compiler because we use it for expressions.  This
      // also handles options like -D, -I, etc.
      bool ok = CompilerInitForAssembler(asm_filename->value, &compiler_options);
      if (!ok) {
        fprintf(stderr, "Failed to assemble\n");
        exit(1);
      }
      StringSet(&target, compiler->target_name->value);
      // Create the global symbol tables.
      CreateGlobalSymbolTables();
      
      Assembler* assembler = NULL;
      void (*asm_run)(Assembler*, String*);
      typedef void (*AssemblerDestructor)(Assembler*);
      typedef void (*AssemblerFinalizer)(Assembler*);
      AssemblerFinalizer finalizer = NULL;
      AssemblerDestructor destructor;

      if (StringEqual(&target, "6502") || StringEqual(&target, "65c02")) {
        assembler = (Assembler*)New6502Assembler(asm_filename, &output_filename);
        asm_run = Assemble6502Instruction;
        destructor = (AssemblerDestructor)W65C02AssemblerDestruct;
        finalizer = (AssemblerFinalizer)W65C02AssemblerFinalize;
      } else if (StringEqual(&target, "riscv") || StringEqual(&target, "risc-v")) {
        assembler = (Assembler*)NewRVAssembler(asm_filename, &output_filename);
        asm_run = AssembleRVInstruction;
        destructor = (AssemblerDestructor)RVAssemblerDestruct;
      } else if (StringEqual(&target, "riscv32") || StringEqual(&target, "risc-v32")) {
        assembler = (Assembler*)NewRV32Assembler(asm_filename, &output_filename);
        asm_run = AssembleRV32Instruction;
        destructor = (AssemblerDestructor)RV32AssemblerDestruct;
      } else if (StringEqual(&target, "esp32") ||
                 StringEqual(&target, "xtensa-esp32")) {
        assembler =
            (Assembler*)NewXTENSAAssembler(asm_filename, &output_filename);
        asm_run = AssembleXTENSAInstruction;
        destructor = (AssemblerDestructor)XTENSAAssemblerDestruct;
      } else if (StringEqual(&target, "aarch64")) {
        assembler = NewAARCH64Assembler(asm_filename, &output_filename);
        asm_run = AssembleAARCH64Instruction;
        destructor = (AssemblerDestructor)AARCH64AssemblerDestruct;
      } else if (StringEqual(&target, "arm") || StringEqual(&target, "armv7") ||
                 StringEqual(&target, "armv7-a") || StringEqual(&target, "arm32")) {
        assembler = (Assembler*)NewARMAssembler(asm_filename, &output_filename);
        asm_run = AssembleARMInstruction;
        destructor = (AssemblerDestructor)ARMAssemblerDestruct;
      } else if (StringEqual(&target, "x86_64") || StringEqual(&target, "x86-64")) {
        assembler = (Assembler*)NewX86_64Assembler(asm_filename, &output_filename);
        asm_run = AssembleX86_64Instruction;
        destructor = (AssemblerDestructor)X86_64AssemblerDestruct;
      } else if (StringEqual(&target, "x86") ||
                 StringEqual(&target, "i386") ||
                 StringEqual(&target, "i486") ||
                 StringEqual(&target, "i586") ||
                 StringEqual(&target, "i686") ||
                 StringEqual(&target, "x86-32")) {
        X86Assembler* x86_assembler = malloc(sizeof(*x86_assembler));
        X86AssemblerInitWithProfile(x86_assembler, asm_filename,
                                    &output_filename, &kX86ProfileI386);
        assembler = (Assembler*)x86_assembler;
        asm_run = AssembleX86Instruction;
        destructor = (AssemblerDestructor)X86AssemblerDestruct;
      } else if (StringEqual(&target, "pcode")) {
        assembler = (Assembler*)NewPCodeAssembler(asm_filename, &output_filename);
        asm_run = AssemblePCodeInstruction;
        destructor = (AssemblerDestructor)PCodeAssemblerDestruct;
      } else if (StringEqual(&target, "bpf") || StringEqual(&target, "bpfel") ||
                 StringEqual(&target, "ebpf")) {
        assembler = (Assembler*)NewBPFAssembler(asm_filename, &output_filename);
        asm_run = AssembleBPFInstruction;
        destructor = (AssemblerDestructor)BPFAssemblerDestruct;
      } else {
        fprintf(stderr, "Unknown assembler architecture %s\n", target.value);
        exit(1);
      }
      assembler->object.pic = compiler->pic;
      PreprocessorCopyOptions(&assembler->preprocessor, &compiler->preprocessor);
      AssemblerRun(assembler, asm_run);
      if (finalizer != NULL) {
        finalizer(assembler);
      }
      int num_errors = assembler->num_errors;
      destructor(assembler);
      if (num_errors != 0) {
        exit(1);
      }
      CompilerDelete(compiler);
      String* object_file = NewString(output_filename.value);
      VectorAppend(&object_files, object_file);
      VectorAppend(&linker_args, object_file->value);
    }
  }

  int status = 0;
  if (!compile_only) {
    AddDefaultRuntime(&linker_args, &object_files, &compiler_options,
                      &resources);
    // Wasm objects are modules rather than ELF, so they need their own
    // linker; the argument surface is the same one the driver already built.
    String* target_option = OptionStringValue(kOptionTarget, &compiler_options);
    String* output =
        Wasm32IsTargetName(target_option == NULL ? NULL : target_option->value)
            ? Wasm32Link((int)linker_args.length,
                         (char**)linker_args.value.p)
            : Link((int)linker_args.length, (char**)linker_args.value.p);
    if (output == NULL) {
      status = 1;
    } else {
      StringDelete(output);
    }
  }
  
  // TODO: tidyup
  exit(status);
}


