"""Shared (PIC) guest libc for interpreter-profile targets that support DSOs."""

def _script_cmd(target, extra_args):
    return """
set -euo pipefail
davecc="$$(pwd)/$(execpath :davecc)"
archivist="$$(pwd)/$(execpath :archivist)"
script="$$(pwd)/$(execpath tools/build_guest_libc.sh)"
out="$$(pwd)/$@"
"$$script" --davecc "$$davecc" --archivist "$$archivist" --output "$$out" \
  --target %s %s
""" % (target, extra_args)

def guest_libc_shared(
        name,
        target,
        output,
        cflags,
        runtime_srcs,
        libc_srcs,
        libc_cxx_srcs,
        extra_c = [],
        exclude = [],
        crt_srcs = [],
        crt_cxx_srcs = [],
        crt_runtime_srcs = [],
        no_cxx = False):
    extra_flags = " ".join(['--cflags "%s"' % flag for flag in cflags])
    runtime_flags = " ".join(['--runtime "%s"' % src for src in runtime_srcs])
    source_flags = " ".join([
        '--source "%s"' % src
        for src in libc_srcs + extra_c
        if src.endswith(".c")
    ])
    cxx_flags = "--no-cxx"
    if not no_cxx and libc_cxx_srcs:
        cxx_flags = " ".join(['--cxx-source "%s"' % src for src in libc_cxx_srcs])
    exclude_flags = " ".join([
        '--exclude "%s"' % pattern
        for pattern in exclude + crt_srcs + crt_cxx_srcs
    ])
    native.genrule(
        name = name,
        srcs = depset(direct = libc_srcs + libc_cxx_srcs + extra_c + runtime_srcs + [
            ":libc_headers",
            "tools/build_guest_libc.sh",
        ]).to_list(),
        outs = [output],
        tools = [":davecc", ":archivist"],
        cmd = _script_cmd(
            target,
            "--shared %s --cflags \"-ftls-model=local-exec\" %s %s %s %s" % (
                extra_flags, runtime_flags, source_flags, cxx_flags, exclude_flags,
            ),
        ),
    )
    if crt_srcs or crt_cxx_srcs or crt_runtime_srcs:
        crt_output = output.replace(".so", "_crt.a")
        crt_cflags = extra_flags + ' --cflags "-fPIC" --cflags "-ftls-model=local-exec"'
        crt_source_flags = " ".join([
            '--source "%s"' % src for src in crt_srcs if src.endswith(".c")
        ])
        crt_runtime_flags = " ".join([
            '--runtime "%s"' % src for src in crt_runtime_srcs
        ])
        crt_cxx_flags = "--no-cxx"
        if crt_cxx_srcs:
            crt_cxx_flags = " ".join([
                '--cxx-source "%s"' % src for src in crt_cxx_srcs
            ])
        native.genrule(
            name = name + "_crt",
            srcs = depset(direct = crt_srcs + crt_cxx_srcs + crt_runtime_srcs + [
                ":libc_headers",
                "tools/build_guest_libc.sh",
            ]).to_list(),
            outs = [crt_output],
            tools = [":davecc", ":archivist"],
            cmd = _script_cmd(
                target,
                "%s %s %s %s" % (
                    crt_cflags, crt_runtime_flags, crt_source_flags, crt_cxx_flags,
                ),
            ),
        )
