#!/usr/bin/env python3
"""Run GCC DejaGNU frontend tests against davecc.

The GCC sources stay outside this repository. Point --root (or
$GCC_TEST_ROOT) at a gcc/testsuite checkout. The runner understands the
frontend-relevant subset of dg-do, dg-options, dg-additional-options,
dg-error, and dg-warning. Unsupported target and runtime directives are
reported but do not prevent crash-only testing.
"""

from __future__ import annotations

import argparse
import os
import re
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
from collections import Counter
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass, field
from pathlib import Path


CXX_EXTS = {".C", ".cc", ".cpp", ".cxx"}
TEST_EXTS = CXX_EXTS | {".c"}
DEFAULT_SUITES = (
    "g++.dg,c-c++-common,gcc.dg,gcc.c-torture/compile"
)
SKIP_PATH_PARTS = {
    "analyzer",
    "asan",
    "atomic",
    "autopar",
    "dfp",
    "goacc",
    "gomp",
    "lto",
    "modules",
    "pch",
    "plugin",
    "tsan",
    "ubsan",
    "vect",
    "vmx",
}
UNSUPPORTED_DIRECTIVES = {
    "dg-additional-files",
    "dg-additional-sources",
    "dg-lto-do",
    "dg-lto-options",
}
DROP_FLAGS = {
    "-ansi",
    "-pedantic-errors",
    "-save-temps",
    "-ftime-report",
    # davecc does not accept this flag, and it only relaxes what GCC rejects, so
    # passing it turned every test that uses it into a failure to parse the
    # command line rather than a statement about the source.
    "-fpermissive",
}
DROP_VALUE_FLAGS = {
    "-dumpbase",
    "-dumpdir",
    "-o",
}
KEEP_EXACT_FLAGS = {
    "-fchar8_t",
    "-fconcepts",
    "-fcontracts",
    "-fcoroutines",
    "-fexceptions",
    "-fno-char8_t",
    "-fno-concepts",
    "-fno-exceptions",
    "-fno-rtti",
    "-frtti",
    "-pedantic",
    "-pthread",
}
KEEP_PREFIX_FLAGS = (
    "-D",
    "-U",
    "-I",
    "-W",
    "-fabi-version=",
    "-fcontracts=",
    "-fdiagnostics-show-caret=",
    "-fvisibility=",
)
UNSUPPORTED_EFFECTIVE_TARGET_RE = re.compile(
    r"^(?:aarch|arm|avx|bitint575|dfp|float128|int128|mips|nvptx|"
    r"offload|openmp|powerpc|riscv|sse|vect|vmx|x86|i.86)"
)
STD_MAP = {
    "c89": "c89",
    "c90": "c89",
    "gnu89": "c89",
    "gnu90": "c89",
    "iso9899:1990": "c89",
    "c99": "c99",
    "gnu99": "c99",
    "iso9899:1999": "c99",
    "c11": "c11",
    "gnu11": "c11",
    "c17": "c17",
    "c18": "c17",
    "gnu17": "c17",
    "gnu18": "c17",
    "c23": "c23",
    "c2x": "c23",
    "gnu23": "c23",
    "gnu2x": "c23",
    "c++98": "c++11",
    "gnu++98": "c++11",
    "c++03": "c++11",
    "gnu++03": "c++11",
    "c++11": "c++11",
    "gnu++11": "c++11",
    "c++0x": "c++11",
    "gnu++0x": "c++11",
    "c++14": "c++17",
    "gnu++14": "c++17",
    "c++1y": "c++17",
    "gnu++1y": "c++17",
    "c++17": "c++17",
    "gnu++17": "c++17",
    "c++1z": "c++17",
    "gnu++1z": "c++17",
    "c++20": "c++20",
    "gnu++20": "c++20",
    "c++2a": "c++20",
    "gnu++2a": "c++20",
    "c++23": "c++23",
    "gnu++23": "c++23",
    "c++2b": "c++23",
    "gnu++2b": "c++23",
    "c++26": "c++26",
    "gnu++26": "c++26",
    "c++2c": "c++26",
    "gnu++2c": "c++26",
}
DIAG_RE = re.compile(
    r"^(?P<kind>error|fatal|warning(?:\[[^\]]*\])?|note): "
    r"(?P<file>.+?):(?P<line>\d+): (?P<message>.*)$"
)
QUOTED_ARG_RE = re.compile(r'"((?:\\.|[^"\\])*)"')


@dataclass
class Directive:
    name: str
    body: str
    line: int


@dataclass
class ExpectedDiagnostic:
    kind: str
    pattern: str
    line: int | None


@dataclass
class TestSpec:
    options: list[str] = field(default_factory=list)
    expected: list[ExpectedDiagnostic] = field(default_factory=list)
    do_kind: str = "compile"
    skip_reason: str | None = None
    timeout_factor: float = 1.0

    @property
    def expects_error(self) -> bool:
        return any(diag.kind == "error" for diag in self.expected)


@dataclass
class Job:
    src: str
    rel: str
    suite: str
    language: str
    options: list[str]
    expected: list[ExpectedDiagnostic]
    expects_error: bool
    timeout_factor: float


def default_gcc_test_root() -> str:
    env = os.environ.get("GCC_TEST_ROOT", "")
    if env:
        return env
    sibling = Path.home() / "gcc-test" / "gcc" / "testsuite"
    return str(sibling) if sibling.is_dir() else ""


def extract_directives(text: str) -> list[Directive]:
    """Extract balanced `{ dg-* ... }` directives, including nested selectors."""
    directives: list[Directive] = []
    cursor = 0
    while True:
        start = text.find("{ dg-", cursor)
        if start < 0:
            break
        depth = 0
        quote = ""
        escaped = False
        end = start
        while end < len(text):
            ch = text[end]
            if escaped:
                escaped = False
            elif ch == "\\":
                escaped = True
            elif quote:
                if ch == quote:
                    quote = ""
            elif ch in ('"', "'"):
                quote = ch
            elif ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
                if depth == 0:
                    break
            end += 1
        if depth != 0:
            cursor = start + 1
            continue
        body = text[start + 1 : end].strip()
        match = re.match(r"(dg-[\w-]+)\b", body)
        if match:
            directives.append(
                Directive(
                    name=match.group(1),
                    body=body[match.end() :].strip(),
                    line=text.count("\n", 0, start) + 1,
                )
            )
        cursor = end + 1
    return directives


def quoted_args(body: str) -> list[str]:
    args: list[str] = []
    for match in QUOTED_ARG_RE.finditer(body):
        raw = match.group(1)
        args.append(raw.replace(r"\"", '"').replace(r"\\", "\\"))
    return args


def directive_applies_to_language(body: str, language: str) -> bool:
    target = re.search(r"\{\s*target\s+([^{}]+)\}", body)
    if target is None:
        return True
    selector = target.group(1)
    excludes_cxx = (
        re.search(r"!\s*c\+\+(?:\d+)?(?![\w+])", selector) is not None
    )
    requires_cxx = (
        not excludes_cxx
        and re.search(
            r"(?<![!\w])c\+\+(?:\d+)?(?![\w+])", selector
        )
        is not None
    )
    if excludes_cxx and language == "c++":
        return False
    if requires_cxx and language != "c++":
        return False
    excludes_c = re.search(r"!\s*c\b", selector) is not None
    requires_c = (
        not excludes_c
        and re.search(r"(?<![!\w+])c\b", selector) is not None
    )
    if excludes_c and language == "c":
        return False
    if requires_c and language != "c":
        return False
    return True


def diagnostic_line(directive: Directive) -> int | None:
    body = re.sub(r"\{\s*(?:target|xfail)\s+[^{}]+\}", " ", directive.body)
    tokens = body.split()
    if not tokens:
        return directive.line
    token = tokens[-1]
    if token == ".":
        return directive.line
    if re.fullmatch(r"\.\-\d+", token):
        return directive.line - int(token[2:])
    if re.fullmatch(r"\.\+\d+", token):
        return directive.line + int(token[2:])
    if token.isdigit():
        value = int(token)
        return value if value > 0 else None
    return directive.line


def unsupported_target(body: str) -> bool:
    target = re.search(r"\{\s*target\s+([^{}]+)\}", body)
    if target is None:
        return False
    words = re.findall(r"[A-Za-z_][\w+.-]*", target.group(1))
    return any(UNSUPPORTED_EFFECTIVE_TARGET_RE.match(word) for word in words)


def parse_spec(text: str, language: str) -> TestSpec:
    spec = TestSpec()
    for directive in extract_directives(text):
        if directive.name in UNSUPPORTED_DIRECTIVES:
            spec.skip_reason = directive.name
            continue
        if not directive_applies_to_language(directive.body, language):
            continue
        if directive.name == "dg-do":
            if unsupported_target(directive.body):
                spec.skip_reason = "unsupported-effective-target"
                continue
            tokens = directive.body.split()
            if tokens:
                spec.do_kind = tokens[0]
            if spec.do_kind == "preprocess":
                spec.skip_reason = "preprocess"
        elif directive.name == "dg-require-effective-target":
            target = directive.body.split(maxsplit=1)[0]
            if UNSUPPORTED_EFFECTIVE_TARGET_RE.match(target):
                spec.skip_reason = "unsupported-effective-target"
        elif directive.name == "dg-timeout-factor":
            try:
                spec.timeout_factor = max(1.0, float(directive.body.split()[0]))
            except (ValueError, IndexError):
                pass
        elif directive.name in ("dg-options", "dg-additional-options"):
            args = quoted_args(directive.body)
            if args:
                try:
                    spec.options.extend(shlex.split(args[0]))
                except ValueError:
                    spec.options.extend(args[0].split())
        elif directive.name in ("dg-error", "dg-warning", "dg-message"):
            args = quoted_args(directive.body)
            if not args:
                continue
            kind = {
                "dg-error": "error",
                "dg-warning": "warning",
                "dg-message": "note",
            }[directive.name]
            spec.expected.append(
                ExpectedDiagnostic(
                    kind=kind,
                    pattern=args[0],
                    line=diagnostic_line(directive),
                )
            )
    return spec


def translate_options(options: list[str], testdir: Path) -> list[str]:
    translated: list[str] = []
    index = 0
    while index < len(options):
        option = options[index]
        if option in DROP_VALUE_FLAGS:
            index += 2
            continue
        if option in DROP_FLAGS or option.startswith("-O"):
            index += 1
            continue
        if option.startswith("-std="):
            mapped = STD_MAP.get(option[5:])
            if mapped:
                translated.append("-std=" + mapped)
            index += 1
            continue
        if option in ("-I", "-isystem", "-D", "-U") and index + 1 < len(options):
            value = options[index + 1]
            if option in ("-I", "-isystem") and not os.path.isabs(value):
                value = str((testdir / value).resolve())
            translated.extend([option, value])
            index += 2
            continue
        if option.startswith("-I") and option != "-I":
            value = option[2:]
            if not os.path.isabs(value):
                value = str((testdir / value).resolve())
            translated.append("-I" + value)
            index += 1
            continue
        if option in KEEP_EXACT_FLAGS or any(
            option.startswith(prefix) for prefix in KEEP_PREFIX_FLAGS
        ):
            translated.append(option)
        index += 1
    return translated


def path_skip_reason(
    path: Path, root: Path, include_unsupported_dirs: bool = False
) -> str | None:
    rel = path.relative_to(root)
    if not include_unsupported_dirs and any(
        part in SKIP_PATH_PARTS for part in rel.parts[:-1]
    ):
        return "unsupported-directory"
    return None


def default_std(language: str) -> str:
    return "-std=c++17" if language == "c++" else "-std=c17"


def run_davecc(
    command: list[str], env: dict[str, str], timeout: int
) -> tuple[int, str]:
    """Run davecc and always kill its process group on timeout."""
    proc = subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        env=env,
        start_new_session=True,
    )
    try:
        output, _ = proc.communicate(timeout=timeout)
        return proc.returncode, output or ""
    except subprocess.TimeoutExpired:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except OSError:
            proc.kill()
        try:
            output, _ = proc.communicate(timeout=2)
        except Exception:
            output = ""
        return 124, (output or "") + "\nTIMEOUT\n"


def actual_diagnostics(log: str) -> list[ExpectedDiagnostic]:
    diagnostics: list[ExpectedDiagnostic] = []
    for raw in log.splitlines():
        match = DIAG_RE.match(raw.strip())
        if not match:
            continue
        kind = match.group("kind")
        if kind.startswith("warning"):
            kind = "warning"
        elif kind == "fatal":
            kind = "error"
        diagnostics.append(
            ExpectedDiagnostic(
                kind=kind,
                pattern=match.group("message"),
                line=int(match.group("line")),
            )
        )
    return diagnostics


def pattern_matches(pattern: str, message: str) -> bool:
    if not pattern:
        return True
    try:
        return re.search(pattern, message) is not None
    except re.error:
        return pattern in message


def diagnostic_problems(
    expected: list[ExpectedDiagnostic], actual: list[ExpectedDiagnostic]
) -> list[str]:
    problems: list[str] = []
    used: set[int] = set()
    for wanted in expected:
        found = None
        for index, got in enumerate(actual):
            if index in used or got.kind != wanted.kind:
                continue
            if wanted.line is not None and got.line != wanted.line:
                continue
            if pattern_matches(wanted.pattern, got.pattern):
                found = index
                break
        if found is None:
            problems.append(
                f"missing {wanted.kind} at {wanted.line or '*'}: "
                f"{wanted.pattern[:100]}"
            )
        else:
            used.add(found)
    return problems


def prepare_source(job: Job, directory: Path) -> Path:
    source = Path(job.src)
    source_is_cxx = source.suffix in CXX_EXTS
    if (job.language == "c++") == source_is_cxx:
        return source
    destination = directory / (
        source.stem + (".cpp" if job.language == "c++" else ".c")
    )
    shutil.copyfile(source, destination)
    return destination


def compile_and_check(
    davecc: str, job: Job, timeout: int, mode: str
) -> tuple[str, str, str, int, str]:
    with tempfile.TemporaryDirectory(prefix="davecc-gcc-dg-") as work:
        source = prepare_source(job, Path(work))
        # DejaGNU compiles each test from its own directory, so a test that
        # includes a sibling ("chk.h") or a helper below it ("lib/chk.c") needs
        # that directory and its suite root on the include path.  Without them
        # the test fails to open its include and the outcome says nothing about
        # the compiler.
        original = Path(job.src).resolve()
        command = [
            davecc,
            "-target",
            "pcode",
            "-fsyntax-only",
            "-error-limit=0",
            default_std(job.language),
            # davecc only honors the joined spelling; it ignores "-I dir".
            "-I" + str(original.parent),
            "-I" + str(original.parent.parent),
            *job.options,
            str(source),
        ]
        env = os.environ.copy()
        env["NO_COLOR"] = "1"
        job_timeout = max(1, int(timeout * job.timeout_factor))
        rc, log = run_davecc(command, env, job_timeout)

    first = next((line for line in log.splitlines() if line.strip()), "")
    if rc == 124:
        return job.rel, "crash", "TIMEOUT", rc, first[:200]
    if rc < 0 or "Assertion failed" in log or "internal compiler error" in log.lower():
        reason = first or f"signal {rc}"
        return job.rel, "crash", reason[:200], rc, first[:200]
    if mode == "crashes":
        return job.rel, "pass", "", rc, first[:200]

    got_error = rc != 0 or any(
        diagnostic.kind == "error" for diagnostic in actual_diagnostics(log)
    )
    if got_error != job.expects_error:
        wanted = "error" if job.expects_error else "success"
        got = "error" if got_error else "success"
        return job.rel, "fail", f"expected {wanted}, got {got}", rc, first[:200]
    if mode == "diagnostics":
        problems = diagnostic_problems(job.expected, actual_diagnostics(log))
        if problems:
            return job.rel, "fail", problems[0], rc, first[:200]
    return job.rel, "pass", "", rc, first[:200]


def iter_tests(root: Path, suites: list[str]) -> list[tuple[str, Path]]:
    tests: list[tuple[str, Path]] = []
    for suite in suites:
        base = root / suite
        if not base.is_dir():
            continue
        for path in sorted(base.rglob("*")):
            if path.is_file() and path.suffix in TEST_EXTS:
                tests.append((suite, path))
    return tests


def languages_for(suite: str, path: Path) -> list[str]:
    if suite == "c-c++-common" and path.suffix == ".c":
        return ["c", "c++"]
    return ["c++"] if path.suffix in CXX_EXTS else ["c"]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--davecc", required=True)
    parser.add_argument(
        "--root",
        default=default_gcc_test_root(),
        help="gcc/testsuite directory (or $GCC_TEST_ROOT)",
    )
    parser.add_argument("--suites", default=DEFAULT_SUITES)
    parser.add_argument(
        "--mode",
        choices=("crashes", "outcome", "diagnostics"),
        default="crashes",
    )
    parser.add_argument("--jobs", type=int, default=min(4, os.cpu_count() or 4))
    parser.add_argument(
        "--timeout",
        type=int,
        default=8,
        help="seconds per davecc invocation; the process group is SIGKILL'd",
    )
    parser.add_argument("--limit", type=int, default=0)
    parser.add_argument(
        "--include-unsupported-dirs",
        action="store_true",
        help="also run the directories whose feature davecc does not implement "
        "(OpenMP, sanitizers, LTO, vectorization, ...).  Their expectations are "
        "meaningless here, but the sources are still valid input that must not "
        "crash or hang the compiler, so this is useful with --mode crashes",
    )
    parser.add_argument("--match", default="", help="substring filter for test paths")
    parser.add_argument("--print-fails", type=int, default=50)
    args = parser.parse_args()

    if not args.root:
        print(
            "No GCC test root. Clone gcc/testsuite outside this repository and "
            "pass --root or set GCC_TEST_ROOT.",
            file=sys.stderr,
        )
        return 2
    root = Path(args.root).resolve()
    if not root.is_dir():
        print(f"GCC test root not found: {root}", file=sys.stderr)
        return 2
    davecc = str(Path(args.davecc).resolve())
    if not Path(davecc).is_file():
        print(f"davecc not found: {davecc}", file=sys.stderr)
        return 2

    suites = [suite.strip() for suite in args.suites.split(",") if suite.strip()]
    tests = iter_tests(root, suites)
    if args.match:
        tests = [
            (suite, path)
            for suite, path in tests
            if args.match in str(path.relative_to(root))
        ]
    if args.limit:
        tests = tests[: args.limit]

    skips: Counter[str] = Counter()
    jobs: list[Job] = []
    for suite, source in tests:
        path_skip = path_skip_reason(source, root, args.include_unsupported_dirs)
        if path_skip:
            skips[path_skip] += 1
            continue
        try:
            text = source.read_text(encoding="utf-8", errors="replace")
        except OSError:
            skips["read-error"] += 1
            continue
        for language in languages_for(suite, source):
            spec = parse_spec(text, language)
            if spec.skip_reason:
                skips[spec.skip_reason] += 1
                continue
            rel = str(source.relative_to(root))
            if len(languages_for(suite, source)) > 1:
                rel += f"::{language}"
            jobs.append(
                Job(
                    src=str(source),
                    rel=rel,
                    suite=suite,
                    language=language,
                    options=translate_options(spec.options, source.parent),
                    expected=spec.expected,
                    expects_error=spec.expects_error,
                    timeout_factor=spec.timeout_factor,
                )
            )

    workers = max(1, min(args.jobs, len(jobs) or 1))
    timeout = max(1, args.timeout)
    print(f"davecc={davecc}")
    print(f"root={root}")
    print(f"tests_scanned={len(tests)} jobs={len(jobs)} "
          f"skipped={sum(skips.values())}")
    print(f"mode={args.mode} workers={workers} timeout={timeout}s")
    print()
    sys.stdout.flush()

    counts: Counter[str] = Counter()
    suite_counts: dict[str, Counter[str]] = {
        suite: Counter() for suite in suites
    }
    fail_samples: list[str] = []
    crash_samples: list[str] = []

    def record(
        job: Job, status: str, problem: str, rc: int, first: str
    ) -> None:
        counts[status] += 1
        counts["compile_error" if rc != 0 else "compile_success"] += 1
        suite_counts.setdefault(job.suite, Counter())[status] += 1
        if status == "fail" and len(fail_samples) < args.print_fails:
            fail_samples.append(f"{job.rel}: {problem}; {first}")
        if status == "crash" and len(crash_samples) < args.print_fails:
            crash_samples.append(f"{job.rel}: rc={rc} {problem}; {first}")

    if workers == 1 or len(jobs) < 8:
        for job in jobs:
            rel, status, problem, rc, first = compile_and_check(
                davecc, job, timeout, args.mode
            )
            del rel
            record(job, status, problem, rc, first)
    else:
        with ProcessPoolExecutor(max_workers=workers) as executor:
            futures = {
                executor.submit(
                    compile_and_check, davecc, job, timeout, args.mode
                ): job
                for job in jobs
            }
            done = 0
            for future in as_completed(futures):
                job = futures[future]
                try:
                    rel, status, problem, rc, first = future.result()
                    del rel
                except Exception as error:  # noqa: BLE001
                    status = "crash"
                    problem = str(error)
                    rc = -1
                    first = str(error)
                record(job, status, problem, rc, first)
                done += 1
                if done % 500 == 0:
                    print(f"  ... {done}/{len(jobs)}", flush=True)

    print("=== counts ===")
    for key in ("pass", "fail", "crash", "compile_success", "compile_error"):
        print(f"{key:16s} {counts[key]}")
    print("\n=== skip reasons ===")
    for reason, count in skips.most_common():
        print(f"{reason:28s} {count}")
    print("\n=== per suite ===")
    print(f"{'suite':28s} {'pass':>7s} {'fail':>7s} {'crash':>7s}")
    for suite in suites:
        count = suite_counts.get(suite, Counter())
        print(
            f"{suite:28s} {count['pass']:7d} "
            f"{count['fail']:7d} {count['crash']:7d}"
        )
    if fail_samples:
        print("\n=== sample failures ===")
        for sample in fail_samples:
            print(" ", sample)
    if crash_samples:
        print("\n=== crashes and timeouts ===")
        for sample in crash_samples:
            print(" ", sample)

    if counts["crash"]:
        return 1
    if args.mode != "crashes" and counts["fail"]:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
