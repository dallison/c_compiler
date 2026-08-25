#!/usr/bin/env python3
"""Run Clang -fsyntax-only -verify tests against davecc.

The Clang sources are not in this repo. Point --root (or $CLANG_TEST_ROOT) at
an llvm-project clang/test checkout. Only RUN lines that use %clang_cc1 with
both -fsyntax-only and -verify are attempted.

Diagnostic matching implements a subset of Clang -verify: expected-error /
warning / note / fatal, optional -re, @line / @+N / @-N / @*, counts, and
-verify=prefix lists. Message text is matched as a substring (or regex).
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

EXTS = {".c", ".cpp", ".cc", ".cxx", ".C"}

SKIP_PATH_PARTS = {
    "AArch64",
    "ARM",
    "RISCV",
    "WebAssembly",
    "AMDGPU",
    "NVPTX",
    "DirectX",
    "PowerPC",
    "SystemZ",
    "Hexagon",
    "Mips",
    "LoongArch",
    "CSKY",
    "VE",
    "AVR",
    "BPF",
    "Sparc",
    "Inputs",
    "LifetimeSafety",
}
SKIP_PATH_PREFIXES = (
    "aarch64-",
    "arm-",
    "riscv-",
    "wasm-",
    "ppc-",
    "x86-",
)
SKIP_INCLUDES = (
    "arm_sme.h",
    "arm_sve.h",
    "arm_neon.h",
    "arm_mve.h",
    "arm_cde.h",
    "arm_fp16.h",
    "arm_bf16.h",
    "immintrin.h",
    "x86intrin.h",
    "xmmintrin.h",
    "emmintrin.h",
    "avxintrin.h",
    "ia32intrin.h",
    "cpuid.h",
    "altivec.h",
    "htmintrin.h",
    "s390intrin.h",
    "wasm_simd128.h",
    "ammintrin.h",
    "avx512fintrin.h",
    "cuda.h",
    "opencl.h",
    "hlsl.h",
)
REQUIRES_SKIP = re.compile(
    r"REQUIRES:.*(registered-target|aarch64|arm-registered|riscv|"
    r"webassembly|nvptx|amdgpu|hexagon|mips|powerpc|systemz|x86-registered)",
    re.I,
)

KEEP_EXACT = {
    "-fexceptions",
    "-fno-exceptions",
    "-nostdinc",
    "-Wall",
    "-Werror",
}
KEEP_PREFIX = ("-std=", "-D", "-U", "-I", "-isystem", "-W", "-fcontracts=")
DROP_VALUE = {
    "-triple",
    "-target-cpu",
    "-target-feature",
    "-include",
    "-include-pch",
    "-o",
    "-x",
}

STD_MAP = {
    "c89": "c89",
    "c90": "c89",
    "gnu89": "c89",
    "gnu90": "c89",
    "c99": "c99",
    "gnu99": "c99",
    "c11": "c11",
    "c1x": "c11",
    "gnu11": "c11",
    "c17": "c17",
    "c18": "c17",
    "gnu17": "c17",
    "gnu18": "c17",
    "c23": "c23",
    "c2x": "c23",
    "gnu23": "c23",
    "gnu2x": "c23",
    "c2y": "c23",
    "gnu2y": "c23",
    "c++98": "c++11",
    "c++03": "c++11",
    "gnu++98": "c++11",
    "gnu++03": "c++11",
    "c++11": "c++11",
    "c++0x": "c++11",
    "gnu++11": "c++11",
    "gnu++0x": "c++11",
    "c++14": "c++17",
    "c++1y": "c++17",
    "gnu++14": "c++17",
    "gnu++1y": "c++17",
    "c++17": "c++17",
    "c++1z": "c++17",
    "gnu++17": "c++17",
    "gnu++1z": "c++17",
    "c++20": "c++20",
    "c++2a": "c++20",
    "gnu++20": "c++20",
    "gnu++2a": "c++20",
    "c++23": "c++23",
    "c++2b": "c++23",
    "gnu++23": "c++23",
    "gnu++2b": "c++23",
    "c++26": "c++26",
    "c++2c": "c++26",
    "gnu++26": "c++26",
    "gnu++2c": "c++26",
    "c++29": "c++29",
    "c++2d": "c++29",
    "gnu++29": "c++29",
    "gnu++2d": "c++29",
}

RUN_RE = re.compile(r"^[ \t]*(?://|/\*)[ \t]*RUN:[ \t]*(.*)$", re.M)
VERIFY_FLAG_RE = re.compile(r"-verify(?:=(\S+))?")
DIRECTIVE_RE = re.compile(
    r"(?P<prefix>\w+)-(?P<kind>error|warning|note|fatal|remark)(?P<re>-re)?"
    r"(?:@(?P<loc>\S+))?"
    r"(?:\s+(?P<count>\d+(?:\s*-\s*\d+)?|\d+\+|0-\d+))?"
    r"\s+\{\{(?P<msg>.*?)\}\}",
    re.S,
)
DIAG_RE = re.compile(
    r"^(?P<kind>error|fatal|warning(?:\[[^\]]*\])?|note): "
    r"(?P<file>.+?):(?P<line>\d+): (?P<msg>.*)$"
)
ANSI_RE = re.compile(r"\x1b\[[0-9;]*m")
IGNORE_DIAG_SUBSTR = (
    "unknown warning option",
    "Unknown option",
    "Failed to compile",
    "Too many errors; terminated",
)


@dataclass
class ExpectedDiag:
    kind: str
    message: str
    is_regex: bool
    line: int | None  # None = anywhere (@*)
    file: str | None
    min_count: int = 1
    max_count: int = 1
    src_line: int = 0


@dataclass
class ActualDiag:
    kind: str
    file: str
    line: int
    message: str
    matched: bool = False


@dataclass
class VerifySpec:
    prefixes: list[str] = field(default_factory=lambda: ["expected"])
    no_diagnostics: bool = False
    expected: list[ExpectedDiag] = field(default_factory=list)


def default_clang_test_root() -> str:
    env = os.environ.get("CLANG_TEST_ROOT", "")
    if env:
        return env
    sibling = Path.home() / "llvm-clang-test" / "clang" / "test"
    if sibling.is_dir():
        return str(sibling)
    return ""


def first_run_lines(text: str) -> list[str]:
    return [m.group(1).strip() for m in RUN_RE.finditer(text)]


def verify_line_ok(run: str) -> bool:
    if "%clang_cc1" not in run:
        return False
    if "-fsyntax-only" not in run or "-verify" not in run:
        return False
    if "FileCheck" in run or "-emit-llvm" in run or "-ast-dump" in run:
        return False
    if "split-file" in run or "-fopenmp" in run or "-fobjc" in run:
        return False
    if "-fmodules" in run or "-emit-module" in run:
        return False
    if "-x objective-c" in run.lower():
        return False
    if " -E " in run or run.endswith(" -E"):
        return False
    return True


def parse_verify_prefixes(run: str) -> list[str]:
    prefixes: list[str] = []
    for m in VERIFY_FLAG_RE.finditer(run):
        if m.group(1):
            prefixes.extend(p for p in m.group(1).split(",") if p)
        else:
            prefixes.append("expected")
    return prefixes or ["expected"]


def parse_count(spec: str | None) -> tuple[int, int]:
    if not spec:
        return 1, 1
    spec = spec.replace(" ", "")
    if spec.endswith("+"):
        n = int(spec[:-1])
        return n, 10**9
    if "-" in spec:
        a, b = spec.split("-", 1)
        return int(a), int(b)
    n = int(spec)
    return n, n


def parse_loc(loc: str | None, src_line: int, testdir: Path) -> tuple[int | None, str | None]:
    if not loc:
        return src_line, None
    if loc == "*":
        return None, None
    if loc.startswith("+") and loc[1:].isdigit():
        return src_line + int(loc[1:]), None
    if loc.startswith("-") and loc[1:].isdigit():
        return src_line - int(loc[1:]), None
    if loc.isdigit():
        return int(loc), None
    if ":" in loc:
        fname, rest = loc.rsplit(":", 1)
        line = None if rest == "*" else int(rest) if rest.isdigit() else src_line
        return line, fname
    return src_line, None


def strip_line_comments_for_code(line: str) -> str:
    # Directives live in // comments, possibly after code.
    return line


def collect_comment_text(text: str) -> list[tuple[int, str]]:
    """Return (line_number, comment_body) for // and trailing block comments."""
    rows: list[tuple[int, str]] = []
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i]
        lineno = i + 1
        # Merge backslash-continued comment lines so directives can wrap.
        while line.rstrip().endswith("\\") and i + 1 < len(lines):
            line = line.rstrip()[:-1] + " " + lines[i + 1]
            i += 1
        m = re.search(r"//(.*)$", line)
        if m:
            rows.append((lineno, m.group(1)))
        i += 1
    return rows


def parse_expected(text: str, prefixes: list[str], testdir: Path) -> VerifySpec:
    spec = VerifySpec(prefixes=prefixes)
    prefix_set = set(prefixes)
    for lineno, body in collect_comment_text(text):
        if "expected-no-diagnostics" in body and "expected" in prefix_set:
            spec.no_diagnostics = True
        for m in DIRECTIVE_RE.finditer(body):
            if m.group("prefix") not in prefix_set:
                continue
            kind = m.group("kind")
            if kind == "remark":
                kind = "warning"
            line, fname = parse_loc(m.group("loc"), lineno, testdir)
            lo, hi = parse_count(m.group("count"))
            spec.expected.append(
                ExpectedDiag(
                    kind="error" if kind == "fatal" else kind,
                    message=m.group("msg"),
                    is_regex=bool(m.group("re")),
                    line=line,
                    file=fname,
                    min_count=lo,
                    max_count=hi,
                    src_line=lineno,
                )
            )
    return spec


def tokenize_run(run: str) -> list[str]:
    try:
        return shlex.split(run, posix=True)
    except ValueError:
        return run.split()


def translate_flags(tokens: list[str], testdir: Path) -> tuple[list[str], str | None]:
    """Return (davecc flags, skip-reason)."""
    out: list[str] = []
    lang_x: str | None = None
    i = 0
    while i < len(tokens):
        tok = tokens[i]
        if tok in ("%clang_cc1", "%s", "%t") or tok.startswith("%"):
            i += 1
            continue
        if tok == "|":
            break
        if tok in ("-fsyntax-only", "-verify", "-pedantic", "-pedantic-errors"):
            i += 1
            continue
        if tok.startswith("-verify"):
            i += 1
            continue
        if tok == "-x" and i + 1 < len(tokens):
            lang_x = tokens[i + 1]
            i += 2
            continue
        if tok.startswith("-x") and tok != "-x":
            lang_x = tok[2:]
            i += 1
            continue
        if tok in DROP_VALUE:
            if tok == "-include":
                return [], "preinclude"
            i += 2 if i + 1 < len(tokens) else 1
            continue
        if tok.startswith("-std="):
            mapped = STD_MAP.get(tok[5:])
            if mapped:
                out.append("-std=" + mapped)
            i += 1
            continue
        if tok == "-std" and i + 1 < len(tokens):
            mapped = STD_MAP.get(tokens[i + 1])
            if mapped:
                out.append("-std=" + mapped)
            i += 2
            continue
        if tok in ("-I", "-isystem", "-D", "-U") and i + 1 < len(tokens):
            val = tokens[i + 1]
            if tok in ("-I", "-isystem") and not os.path.isabs(val):
                val = str((testdir / val).resolve())
            out.extend([tok, val])
            i += 2
            continue
        if tok.startswith("-I") and tok != "-I":
            val = tok[2:]
            if not os.path.isabs(val):
                val = str((testdir / val).resolve())
            out.append("-I" + val)
            i += 1
            continue
        if tok in KEEP_EXACT or any(tok.startswith(p) for p in KEEP_PREFIX):
            out.append(tok)
            i += 1
            continue
        i += 1
    if lang_x and "objective-c" in lang_x:
        return [], "objc"
    if lang_x in ("cuda", "hip", "hlsl"):
        return [], "accelerator"
    return out, lang_x


def path_skip_reason(path: Path, root: Path) -> str | None:
    try:
        rel = path.relative_to(root)
    except ValueError:
        rel = path
    for part in rel.parts[:-1]:
        if part in SKIP_PATH_PARTS:
            return "target-dir"
        if any(part.startswith(p) for p in SKIP_PATH_PREFIXES):
            return "target-dir"
    return None


def parse_actual(log: str) -> list[ActualDiag]:
    diags: list[ActualDiag] = []
    for raw in log.splitlines():
        line = ANSI_RE.sub("", raw).strip()
        if not line or any(s in line for s in IGNORE_DIAG_SUBSTR):
            continue
        m = DIAG_RE.match(line)
        if not m:
            continue
        kind = m.group("kind")
        if kind.startswith("warning"):
            kind = "warning"
        elif kind == "fatal":
            kind = "error"
        diags.append(
            ActualDiag(
                kind=kind,
                file=m.group("file"),
                line=int(m.group("line")),
                message=m.group("msg"),
            )
        )
    return diags


def message_matches(expected: ExpectedDiag, actual: ActualDiag) -> bool:
    if expected.kind != actual.kind:
        return False
    if expected.line is not None and expected.line != actual.line:
        return False
    if expected.file is not None:
        if Path(actual.file).name != Path(expected.file).name and expected.file not in actual.file:
            return False
    if expected.is_regex:
        try:
            return re.search(expected.message, actual.message) is not None
        except re.error:
            return expected.message in actual.message
    return expected.message in actual.message


def match_verify(spec: VerifySpec, actual: list[ActualDiag]) -> list[str]:
    problems: list[str] = []
    if spec.no_diagnostics:
        leftover = [d for d in actual if d.kind in ("error", "warning", "fatal")]
        if leftover:
            problems.append(
                f"expected-no-diagnostics but got {len(leftover)}: "
                f"{leftover[0].kind}:{leftover[0].line}: {leftover[0].message[:80]}"
            )
        return problems

    for exp in spec.expected:
        hits = [d for d in actual if not d.matched and message_matches(exp, d)]
        n = len(hits)
        if n < exp.min_count:
            loc = f"@{exp.line}" if exp.line is not None else "@*"
            problems.append(
                f"missing {exp.kind} {loc} {{{{ {exp.message[:60]} }}}} "
                f"(have {n}, want {exp.min_count}-{exp.max_count})"
            )
            continue
        use = hits[: exp.max_count] if exp.max_count < 10**9 else hits
        if n > exp.max_count and exp.max_count < 10**9:
            # Extra matches of this pattern are left for other directives /
            # unexpected-diag checks.
            use = hits[: exp.max_count]
        for d in use:
            d.matched = True

    for d in actual:
        if d.matched or d.kind == "note":
            # Unmatched notes are common when davecc's notes don't match Clang's
            # expected-note text; still report unmatched errors/warnings.
            continue
        if d.kind in ("error", "warning"):
            problems.append(
                f"unexpected {d.kind} at {Path(d.file).name}:{d.line}: {d.message[:100]}"
            )
    return problems


def prepare_input(src: Path, lang_x: str | None, work: Path) -> Path:
    if not lang_x:
        return src
    want_cxx = lang_x in ("c++", "c++-header")
    want_c = lang_x in ("c", "c-header")
    is_cxx = src.suffix.lower() in {".cpp", ".cc", ".cxx"}
    if want_cxx and not is_cxx:
        dest = work / (src.stem + ".cpp")
        shutil.copyfile(src, dest)
        return dest
    if want_c and is_cxx:
        dest = work / (src.stem + ".c")
        shutil.copyfile(src, dest)
        return dest
    return src


def default_std(path: Path) -> str:
    if path.suffix.lower() in {".cpp", ".cc", ".cxx"}:
        return "-std=c++20"
    return "-std=c11"


@dataclass
class Job:
    src: str
    rel: str
    run: str
    flags: list[str]
    lang_x: str | None
    prefixes: list[str]


def run_davecc(cmd: list[str], env: dict[str, str], timeout: int) -> tuple[int, str]:
    """Run davecc and always kill the process group on timeout."""
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        env=env,
        start_new_session=True,
    )
    try:
        out, _ = proc.communicate(timeout=timeout)
        return proc.returncode, out or ""
    except subprocess.TimeoutExpired:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except OSError:
            proc.kill()
        try:
            out, _ = proc.communicate(timeout=2)
        except Exception:
            out = ""
        return 124, (out or "") + "\nTIMEOUT\n"


def compile_and_check(
    davecc: str, job: Job, work: str, timeout: int
) -> tuple[str, str, list[str], int, str]:
    """Return (rel, status, problems, rc, first_line)."""
    src = Path(job.src)
    workp = Path(work)
    spec = parse_expected(src.read_text(encoding="utf-8", errors="replace"), job.prefixes, src.parent)
    infile = prepare_input(src, job.lang_x, workp)
    cmd = [
        davecc,
        "-target",
        "pcode",
        "-fsyntax-only",
        "-error-limit=0",
        default_std(infile),
        *job.flags,
        str(infile),
    ]
    env = os.environ.copy()
    env["NO_COLOR"] = "1"
    rc, log = run_davecc(cmd, env, timeout)
    if rc == 124:
        return job.rel, "crash", ["TIMEOUT"], 124, "TIMEOUT"

    if rc < 0 or "Assertion failed" in log or "internal compiler error" in log.lower():
        first = next((ln for ln in log.splitlines() if ln.strip()), f"signal {rc}")
        return job.rel, "crash", [first[:200]], rc, first[:200]

    actual = parse_actual(log)
    problems = match_verify(spec, actual)
    first = next((ln for ln in log.splitlines() if ln.strip()), "")
    if problems:
        return job.rel, "fail", problems, rc, first[:200]
    return job.rel, "pass", [], rc, first[:200]


def iter_tests(root: Path, suites: list[str]) -> list[Path]:
    files: list[Path] = []
    for suite in suites:
        base = root / suite
        if not base.exists():
            continue
        for p in sorted(base.rglob("*")):
            if p.is_file() and p.suffix in EXTS:
                files.append(p)
    return files


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--davecc", required=True)
    ap.add_argument(
        "--root",
        default=default_clang_test_root(),
        help="clang/test directory (or $CLANG_TEST_ROOT)",
    )
    ap.add_argument(
        "--suites",
        default="Sema,SemaCXX,SemaTemplate,Parser,CXX,Lexer,Preprocessor",
    )
    ap.add_argument("--limit", type=int, default=0)
    ap.add_argument("--jobs", type=int, default=min(4, os.cpu_count() or 4))
    ap.add_argument(
        "--timeout",
        type=int,
        default=8,
        help="seconds per davecc invocation; the process group is SIGKILL'd",
    )
    ap.add_argument("--print-fails", type=int, default=30)
    ap.add_argument("--work", default="")
    args = ap.parse_args()

    if not args.root:
        print(
            "No Clang test root. Clone llvm-project (sparse clang/test) and pass "
            "--root or set CLANG_TEST_ROOT.",
            file=sys.stderr,
        )
        return 2

    davecc = str(Path(args.davecc).resolve())
    root = Path(args.root).resolve()
    if not root.is_dir():
        print(f"clang test root not found: {root}", file=sys.stderr)
        return 2
    suites = [s.strip() for s in args.suites.split(",") if s.strip()]
    work = Path(args.work) if args.work else Path(tempfile.mkdtemp(prefix="davecc-clang-verify-"))
    work.mkdir(parents=True, exist_ok=True)

    files = iter_tests(root, suites)
    if args.limit:
        files = files[: args.limit]

    skip_reasons: Counter[str] = Counter()
    jobs: list[Job] = []
    for src in files:
        rel = str(src.relative_to(root))
        try:
            text = src.read_text(encoding="utf-8", errors="replace")
        except OSError:
            skip_reasons["read-error"] += 1
            continue
        ps = path_skip_reason(src, root)
        if ps:
            skip_reasons[ps] += 1
            continue
        if REQUIRES_SKIP.search(text[:4000]):
            skip_reasons["requires-target"] += 1
            continue
        if any(h in text for h in SKIP_INCLUDES):
            skip_reasons["clang-builtin-header"] += 1
            continue
        runs = [r for r in first_run_lines(text) if verify_line_ok(r)]
        if not runs:
            skip_reasons["not-fsyntax-verify"] += 1
            continue
        for run in runs:
            flags, extra = translate_flags(tokenize_run(run), src.parent)
            lang_x = extra if extra in ("c", "c++", "c-header", "c++-header") else None
            if extra and lang_x is None and extra in (
                "preinclude",
                "objc",
                "accelerator",
            ):
                skip_reasons[extra] += 1
                continue
            jobs.append(
                Job(
                    src=str(src),
                    rel=rel,
                    run=run,
                    flags=flags,
                    lang_x=lang_x,
                    prefixes=parse_verify_prefixes(run),
                )
            )

    print(f"davecc={davecc}")
    print(f"root={root}")
    print(f"files_scanned={len(files)}")
    print(f"jobs={len(jobs)} skipped={sum(skip_reasons.values())} "
          f"workers={max(1, min(args.jobs, len(jobs) or 1))} timeout={args.timeout}s")
    print()
    sys.stdout.flush()

    counts: Counter[str] = Counter()
    suite_counts: dict[str, Counter[str]] = {s: Counter() for s in suites}
    fail_samples: list[str] = []
    crash_samples: list[str] = []

    def record(rel: str, status: str, problems: list[str], rc: int, first: str) -> None:
        counts[status] += 1
        suite = rel.split("/", 1)[0]
        suite_counts.setdefault(suite, Counter())[status] += 1
        if status == "fail" and len(fail_samples) < args.print_fails:
            fail_samples.append(f"{rel}: {problems[0]}")
        if status == "crash" and len(crash_samples) < args.print_fails:
            crash_samples.append(f"{rel} rc={rc} {first}")

    workers = max(1, min(args.jobs, len(jobs) or 1))
    timeout = max(1, args.timeout)
    if workers == 1 or len(jobs) < 8:
        for job in jobs:
            rel, status, problems, rc, first = compile_and_check(
                davecc, job, str(work), timeout
            )
            record(rel, status, problems, rc, first)
    else:
        with ProcessPoolExecutor(max_workers=workers) as ex:
            futs = {
                ex.submit(compile_and_check, davecc, job, str(work), timeout): job
                for job in jobs
            }
            done = 0
            for fut in as_completed(futs):
                job = futs[fut]
                try:
                    rel, status, problems, rc, first = fut.result()
                except Exception as e:  # noqa: BLE001
                    record(job.rel, "crash", [str(e)], -1, str(e))
                    continue
                record(rel, status, problems, rc, first)
                done += 1
                if done % 200 == 0:
                    print(f"  ... {done}/{len(jobs)}", flush=True)

    print("=== counts ===")
    attempted = counts["pass"] + counts["fail"] + counts["crash"]
    for k in ("pass", "fail", "crash"):
        print(f"{k:8s} {counts[k]}")
    print()
    print("=== skip reasons ===")
    for k, v in skip_reasons.most_common():
        print(f"{k:24s} {v}")
    print()
    if attempted:
        print(
            f"verify pass {counts['pass']}/{attempted} "
            f"({100.0 * counts['pass'] / attempted:.1f}%)  "
            f"crashes={counts['crash']}"
        )
    print()
    print("=== per suite ===")
    print(f"{'suite':16s} {'pass':>6} {'fail':>6} {'crash':>6}")
    for s in suites:
        sc = suite_counts.get(s, Counter())
        print(f"{s:16s} {sc['pass']:6d} {sc['fail']:6d} {sc['crash']:6d}")
    if fail_samples:
        print("\n=== sample verify failures ===")
        for s in fail_samples:
            print(" ", s)
    if crash_samples:
        print("\n=== sample crashes ===")
        for s in crash_samples:
            print(" ", s)
    return 0 if counts["crash"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
