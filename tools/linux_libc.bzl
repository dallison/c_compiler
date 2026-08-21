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
