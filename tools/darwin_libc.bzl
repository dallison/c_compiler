def darwin_libc_archive(
        name,
        architecture,
        support_sources,
        output,
        libc_c_srcs,
        libc_cxx_srcs,
        extra_hdrs = [],
        host_c_srcs = []):
    support_commands = "\n".join([
        '"$$davecc" "$${cflags[@]}" "%s" -o "$$work/support_%d.o"' % (source, index)
        for index, source in enumerate(support_sources)
    ])
    host_commands = "\n".join([
        '"$$host_cc" -c -arch arm64 -O2 -std=c11 -Ic_compiler/Loader "%s" -o "$$work/host_%d.o"' % (source, index)
        for index, source in enumerate(host_c_srcs)
    ])
    native.genrule(
        name = name,
        srcs = libc_c_srcs + libc_cxx_srcs + extra_hdrs + host_c_srcs + [":libc_headers"] + [
            ":davecc",
        ] + support_sources,
        outs = ["libc/" + output],
        cmd = """
set -euo pipefail
out="$@"
work="$(@D)/%s.build"
rm -rf "$$work"
mkdir -p "$$work"
davecc="$$(pwd)/$(execpath :davecc)"
host_cc=/usr/bin/cc
cflags=(-target %s-apple-darwin-davecc -O0 -c -isystem libc/include -Ilibc -Ic_compiler/Loader)

%s

%s

for src in $(SRCS); do
  case "$$src" in
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

out_file="$$out"
case "$$out_file" in
  /*) ;;
  *) out_file="$$(pwd)/$$out_file" ;;
esac
cd "$$work"
rm -f "$$out_file"
/usr/bin/ar rcs "$$out_file" *.o
""" % (name, architecture, support_commands, host_commands),
    )
