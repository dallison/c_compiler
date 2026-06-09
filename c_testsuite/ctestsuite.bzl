"""Bazel helpers for //c_testsuite single-exec regression tests."""

load("@rules_shell//shell:sh_test.bzl", "sh_test")

def ctestsuite_sh_test(
        name,
        davecc_target,
        compiler_target,
        libc_label,
        interpreter_label,
        skip_file,
        compile_args,
        test_data,
        interp_args = [],
        rom_label = None):
    """Run every single-exec test in sequence for one DaveCC target."""
    data = test_data + [
        skip_file,
        davecc_target,
        libc_label,
        interpreter_label,
    ]
    args = [
        "--davecc",
        "$(rootpath " + davecc_target + ")",
        "--target",
        compiler_target,
        "--libc",
        "$(rootpath " + libc_label + ")",
        "--interpreter",
        "$(rootpath " + interpreter_label + ")",
        "--skip",
        "$(rootpath " + skip_file + ")",
        # Per-test wall-clock limit.  A few tests (e.g. the 00040 8-queens
        # solver) are genuinely heavy: they take ~25s even when fully optimized
        # because the work is interpreted, so a 30s ceiling is too tight and
        # leaves no margin for an unoptimized (-O0) build or a slower CI host.
        # The output is correct at every optimization level; only the run time
        # exceeded the old limit.
        "--timeout",
        "90",
    ]
    for flag in compile_args:
        args += ["--compile-arg", flag]
    for flag in interp_args:
        args += ["--interp-arg", flag]
    if rom_label:
        data.append(rom_label)
        args += [
            "--rom",
            "$(rootpath " + rom_label + ")",
        ]

    sh_test(
        name = "single_exec_" + name,
        srcs = ["run_single_exec.sh"],
        args = args,
        data = data,
        tags = [
            "ctestsuite",
            "manual",
        ],
        timeout = "eternal",
    )
