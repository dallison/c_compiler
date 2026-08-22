def linux_libc_archive(
        name,
        architecture,
        syscall_source,
        clone_source,
        support_sources,
        output,
        libc_srcs,
        libc_cxx_srcs):
    support_commands = "\n".join([
        '"$$davecc" "$${cflags[@]}" "%s" -o "$$work/support_%d.o"' % (source, index)
        for index, source in enumerate(support_sources)
    ])
    native.genrule(
        name = name,
        srcs = libc_srcs + libc_cxx_srcs + [":libc_headers"] + [
            syscall_source,
            clone_source,
            "libc/linux_syscall.c",
            "libc/linux_syscall_result.c",
            "libc/linux_fs.c",
            "libc/linux_tls.c",
            "libc/guest_heap.c",
            ":davecc",
            ":archivist",
        ] + support_sources,
        outs = ["libc/" + output],
        cmd = """
set -euo pipefail
out="$@"
work="$(@D)/%s.build"
rm -rf "$$work"
mkdir -p "$$work"
davecc="$$(pwd)/$(execpath :davecc)"
archivist="$$(pwd)/$(execpath :archivist)"
cflags=(-target %s-unknown-linux-davecc -O0 -c -isystem libc/include -Ilibc)
syscall_source="%s"
clone_source="%s"

"$$davecc" "$${cflags[@]}" "%s" -o "$$work/linux_syscall.o"
"$$davecc" "$${cflags[@]}" "%s" -o "$$work/linux_clone.o"
%s
"$$davecc" "$${cflags[@]}" libc/linux_syscall.c \
  -o "$$work/linux_syscall_wrapper.o"
"$$davecc" "$${cflags[@]}" libc/linux_syscall_result.c \
  -o "$$work/linux_syscall_result.o"

for src in $(SRCS); do
  case "$$src" in
    "$$syscall_source"|"$$clone_source")
      continue
      ;;
    libc/libc_test.c|libc/linux_syscall.c|libc/linux_syscall_result.c|libc/linux_fs.c|libc/linux_tls.c)
      continue
      ;;
    libc/*.c)
      obj="$$work/c_$$(basename "$${src%%.c}").o"
      "$$davecc" "$${cflags[@]}" "$$src" -o "$$obj"
      ;;
    libc/*.cc)
      obj="$$work/cxx_$$(basename "$${src%%.cc}").o"
      "$$davecc" "$${cflags[@]}" -std=c++20 "$$src" -o "$$obj"
      ;;
  esac
done

"$$davecc" "$${cflags[@]}" libc/linux_fs.c -o "$$work/linux_fs.o"
"$$davecc" "$${cflags[@]}" libc/linux_tls.c -o "$$work/linux_tls.o"

out_file="$$out"
case "$$out_file" in
  /*) ;;
  *) out_file="$$(pwd)/$$out_file" ;;
esac
cd "$$work"
"$$archivist" r "$$out_file" *.o
""" % (name, architecture, syscall_source, clone_source, syscall_source, clone_source, support_commands),
    )

def linux_start_object(name, architecture, source, output):
    native.genrule(
        name = name,
        srcs = [source, ":davecc"],
        outs = ["libc/" + output],
        cmd = """
set -euo pipefail
davecc="$$(pwd)/$(execpath :davecc)"
"$$davecc" -target %s-unknown-linux-davecc -nostdinc -nostdlib -c \
  "%s" -o "$@"
""" % (architecture, source),
    )

def linux_dynamic_runtime(
        name,
        architecture,
        syscall_source,
        clone_source,
        startup_source,
        support_sources,
        dso_sources,
        crt_sources,
        libc_cxx_srcs):
    support_commands = "\n".join([
        '"$$davecc" "$${dso_cflags[@]}" "%s" -o "$$work/dso_support_%d.o"' % (source, index)
        for index, source in enumerate(support_sources)
    ])
    native.genrule(
        name = name,
        srcs = dso_sources + crt_sources + libc_cxx_srcs +
               [":libc_headers", syscall_source, clone_source, startup_source,
                "libc/linux_syscall.c", "libc/linux_syscall_result.c",
                "libc/linux_fs.c", ":davecc", ":archivist"] + support_sources,
        outs = [
            "libc/libdavecc.so",
            "libc/libdavecc.so.1",
            "libc/libdavecc_crt.a",
            "libc/x86_64_linux_dynamic_start.o",
        ],
        cmd = """
set -euo pipefail
out_dir="$(@D)/libc"
work="$(@D)/%s.build"
rm -rf "$$work"
mkdir -p "$$work"
davecc="$$(pwd)/$(execpath :davecc)"
archivist="$$(pwd)/$(execpath :archivist)"
dso_cflags=(-target %s-unknown-linux-davecc -O0 -c -fPIC \
  -D__DAVECC_DYNAMIC_LIBC__ -isystem libc/include -Ilibc)
crt_cflags=(-target %s-unknown-linux-davecc -O0 -c -fPIC \
  -ftls-model=local-exec -D__DAVECC_DYNAMIC_LIBC__ \
  -isystem libc/include -Ilibc)
syscall_source="%s"
clone_source="%s"
startup_source="%s"

"$$davecc" "$${dso_cflags[@]}" "$$syscall_source" -o "$$work/dso_linux_syscall.o"
"$$davecc" "$${dso_cflags[@]}" "$$clone_source" -o "$$work/dso_linux_clone.o"
%s
"$$davecc" "$${dso_cflags[@]}" libc/linux_syscall.c \
  -o "$$work/dso_linux_syscall_wrapper.o"
"$$davecc" "$${dso_cflags[@]}" libc/linux_syscall_result.c \
  -o "$$work/dso_linux_syscall_result.o"
"$$davecc" "$${dso_cflags[@]}" libc/linux_fs.c \
  -o "$$work/dso_linux_fs.o"

for src in %s; do
  case "$$src" in
    *.c)
      obj="$$work/dso_c_$$(basename "$${src%%.c}").o"
      "$$davecc" "$${dso_cflags[@]}" "$$src" -o "$$obj"
      ;;
    *.cc)
      obj="$$work/dso_cxx_$$(basename "$${src%%.cc}").o"
      "$$davecc" "$${dso_cflags[@]}" -std=c++20 "$$src" -o "$$obj"
      ;;
  esac
done

"$$davecc" -target %s-unknown-linux-davecc -nostdinc -nostdlib -c -fPIC \
  "$$startup_source" -o "$$out_dir/x86_64_linux_dynamic_start.o"

for src in %s; do
  obj="$$work/crt_$$(basename "$${src%%.c}").o"
  "$$davecc" "$${crt_cflags[@]}" "$$src" -o "$$obj"
done

"$$davecc" -target %s-unknown-linux-davecc -nostdlib -shared \
  -Wl,-bind-now -Wl,-defer-init "$$work"/dso_*.o \
  -o "$$out_dir/libdavecc.so.1"
cp "$$out_dir/libdavecc.so.1" "$$out_dir/libdavecc.so"

crt_archive="$$out_dir/libdavecc_crt.a"
rm -f "$$crt_archive"
"$$archivist" r "$$crt_archive" "$$work"/crt_*.o
""" % (
            name,
            architecture,
            architecture,
            syscall_source,
            clone_source,
            startup_source,
            support_commands,
            " ".join(dso_sources + libc_cxx_srcs),
            architecture,
            " ".join(crt_sources),
            architecture,
        ),
    )
