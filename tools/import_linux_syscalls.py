#!/usr/bin/env python3
"""Generate DaveCC Linux syscall headers from a pinned Linux release."""

from __future__ import annotations

import argparse
import pathlib
import re
import urllib.request

LINUX_TAG = "v6.8"
RAW_ROOT = f"https://raw.githubusercontent.com/torvalds/linux/{LINUX_TAG}"


def download(path: str) -> str:
    with urllib.request.urlopen(f"{RAW_ROOT}/{path}") as response:
        return response.read().decode("utf-8")


def parse_table(text: str, accepted_abis: set[str]) -> dict[str, int]:
    result: dict[str, int] = {}
    for raw_line in text.splitlines():
        line = raw_line.split("#", 1)[0].strip()
        if not line:
            continue
        fields = line.split()
        if len(fields) < 3 or fields[1] not in accepted_abis:
            continue
        if fields[0].isdigit():
            result.setdefault(fields[2], int(fields[0]))
    return result


DEFINE_RE = re.compile(r"^\s*#\s*define\s+(__NR(?:3264)?_[A-Za-z0-9_]+)\s+(.+?)\s*$")
TOKEN_RE = re.compile(r"\b__NR(?:3264)?_[A-Za-z0-9_]+\b")


def evaluate_expression(expression: str, values: dict[str, int]) -> int | None:
    expression = expression.split("/*", 1)[0].strip()
    expression = expression.replace("UL", "").replace("U", "").replace("L", "")
    for token in set(TOKEN_RE.findall(expression)):
        if token not in values:
            return None
        expression = expression.replace(token, str(values[token]))
    if not re.fullmatch(r"[0-9xXa-fA-F()+\-*/<>&|~\s]+", expression):
        return None
    try:
        return int(eval(expression, {"__builtins__": {}}, {}))
    except (SyntaxError, TypeError, ValueError, ZeroDivisionError):
        return None


def parse_defines(*texts: str) -> dict[str, int]:
    expressions: dict[str, str] = {}
    for text in texts:
        logical = text.replace("\\\n", "")
        for line in logical.splitlines():
            match = DEFINE_RE.match(line)
            if match:
                expressions[match.group(1)] = match.group(2)

    values: dict[str, int] = {}
    changed = True
    while changed:
        changed = False
        for name, expression in expressions.items():
            if name in values:
                continue
            value = evaluate_expression(expression, values)
            if value is not None:
                values[name] = value
                changed = True

    result: dict[str, int] = {}
    for macro, number in values.items():
        if macro.startswith("__NR3264_") or macro == "__NR_syscalls":
            continue
        result[macro.removeprefix("__NR_")] = number
    for macro, expression in expressions.items():
        if not macro.startswith("__NR_") or macro.startswith("__NR3264_"):
            continue
        alias = macro.removeprefix("__NR_")
        if alias in result:
            continue
        value = evaluate_expression(expression, values)
        if value is not None:
            result[alias] = value
    return result


def write_header(path: pathlib.Path, architecture: str,
                 syscalls: dict[str, int]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    guard = f"davecc_linux_{architecture}_syscall_numbers_h"
    lines = [
        f"// Generated from Linux {LINUX_TAG}; do not edit.",
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
    ]
    for name, number in sorted(syscalls.items(), key=lambda item: (item[1], item[0])):
        lines.append(f"#define __NR_{name} {number}")
        lines.append(f"#define SYS_{name} __NR_{name}")
    lines.extend(["", f"#endif  // {guard}", ""])
    path.write_text("\n".join(lines))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output-root",
        type=pathlib.Path,
        default=pathlib.Path("libc/include/davecc/linux"),
    )
    args = parser.parse_args()

    generic = download("include/uapi/asm-generic/unistd.h")
    riscv = download("arch/riscv/include/uapi/asm/unistd.h")
    tables = {
        "x86_64": parse_table(
            download("arch/x86/entry/syscalls/syscall_64.tbl"), {"common", "64"}
        ),
        "arm": parse_table(
            download("arch/arm/tools/syscall.tbl"), {"common", "eabi"}
        ),
        "aarch64": parse_defines(generic),
        "riscv": parse_defines(generic, riscv),
    }
    for architecture, syscalls in tables.items():
        if not syscalls:
            raise SystemExit(f"no syscalls parsed for {architecture}")
        write_header(
            args.output_root / architecture / "syscall_numbers.h",
            architecture,
            syscalls,
        )


if __name__ == "__main__":
    main()
