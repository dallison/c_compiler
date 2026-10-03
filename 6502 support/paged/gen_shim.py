#!/usr/bin/env python3
"""Build the sideways vector table and the main-RAM libc shims.

exports.list is append-only. New public functions are added at the end so
existing vector addresses stay put. Pass --update-exports to append names
that the objects define but the list does not mention yet.
"""

import argparse
import sys

VECTOR_BASE = 0x8100
VECTOR_STRIDE = 3
# Header is 256 bytes, so this many JMP slots still leave the table inside
# the sideways window. The image itself must finish before 0xC000.
VECTOR_MAX = 512

# Compiler and libc internals are global so the ROM can call them, but user
# programs do not. The exceptions are the calls the compiler and headers emit.
SHIM_PREFIXES = (
    "__printf_",
    "__fprintf_",
    "__sprintf_",
    "__snprintf_",
    "__vprintf_",
    "__vfprintf_",
    "__assert_fail",
    "__davecc_assert_fail",
    "__cxa_",
)


def parse_funcs(path):
    names = []
    seen = set()
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            parts = line.split()
            # elfdump: index value size type bind section(shndx) name
            if len(parts) < 7:
                continue
            if parts[3] != "func" or parts[4] != "global":
                continue
            section = parts[5]
            # Undefined references are printed with an empty section and a
            # bare "(0)" shndx. Only definitions in a text section are entries.
            if not section.startswith(".text"):
                continue
            name = parts[-1]
            if name.startswith(".") or name == "":
                continue
            if name.startswith("__") and not name.startswith(SHIM_PREFIXES):
                continue
            if name in seen:
                continue
            seen.add(name)
            names.append(name)
    return names


def read_exports(path):
    names = []
    with open(path, "r", encoding="utf-8") as handle:
        for line in handle:
            line = line.split("#", 1)[0].strip()
            if line:
                names.append(line)
    return names


def write_exports(path, names):
    with open(path, "w", encoding="utf-8") as handle:
        handle.write(
            "# Append-only libc entry points. Vector 0 is __paged_rom_init;\n"
            "# the first name here is vector 1 at 0x8103. Do not reorder.\n"
        )
        for name in names:
            handle.write(name + "\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--symbols", required=True)
    parser.add_argument("--exports", required=True)
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--shim", required=True)
    parser.add_argument("--rom-index", type=int, default=0)
    parser.add_argument("--cross-exports")
    parser.add_argument("--cross-rom", type=int, default=0)
    parser.add_argument("--cross", action="append", default=[])
    parser.add_argument("--cross-out")
    parser.add_argument("--export-subset", action="store_true")
    parser.add_argument("--update-exports", action="store_true")
    args = parser.parse_args()

    defined = parse_funcs(args.symbols)
    defined_set = set(defined)
    try:
        exports = read_exports(args.exports)
    except FileNotFoundError:
        exports = []

    if not exports and not args.update_exports and not args.export_subset:
        args.update_exports = True

    known = set(exports)
    if args.export_subset:
        missing = []
    else:
        missing = [name for name in defined if name not in known]
    if missing:
        if not args.update_exports:
            sys.stderr.write(
                "paged libc exports.list is missing %d functions "
                "(append with --update-exports):\n" % len(missing)
            )
            for name in missing:
                sys.stderr.write("  %s\n" % name)
            return 1
        exports.extend(sorted(missing))

    stale = [name for name in exports if name not in defined_set]
    if stale:
        sys.stderr.write(
            "exports.list names functions the libc objects do not define:\n"
        )
        for name in stale:
            sys.stderr.write("  %s\n" % name)
        return 1

    if len(exports) + 1 > VECTOR_MAX:
        sys.stderr.write(
            "paged libc has %d vectors; the table holds %d\n"
            % (len(exports) + 1, VECTOR_MAX)
        )
        return 1

    if args.update_exports:
        write_exports(args.exports, exports)

    with open(args.vectors, "w", encoding="utf-8") as out:
        out.write('.section ".rom_vectors", "ax", @progbits\n')
        out.write(".global __paged_vectors\n")
        out.write("__paged_vectors:\n")
        out.write("  JMP __paged_rom_init\n")
        for name in exports:
            out.write("  JMP %s\n" % name)

    with open(args.shim, "w", encoding="utf-8") as out:
        out.write('#include "layout.h"\n')
        for index, name in enumerate(exports):
            address = VECTOR_BASE + (index + 1) * VECTOR_STRIDE
            out.write('.section ".text.%s", "ax", @progbits\n' % name)
            out.write(".global %s\n" % name)
            out.write(".type %s, @function\n" % name)
            out.write("%s:\n" % name)
            out.write("  LDA #%d\n" % args.rom_index)
            out.write("  JSR __libc_paged_enter\n")
            out.write("  JSR 0x%x\n" % address)
            out.write("  JMP __libc_paged_leave\n")

    if args.cross_out:
        crosses = []
        if args.cross_exports:
            crosses.append((args.cross_rom, args.cross_exports))
        for item in args.cross:
            rom_text, path = item.split(":", 1)
            crosses.append((int(rom_text), path))
        with open(args.cross_out, "w", encoding="utf-8") as out:
            out.write('#include "layout.h"\n')
            seen = set()
            for rom, path in crosses:
                for index, name in enumerate(read_exports(path)):
                    if name in defined_set or name in seen:
                        continue
                    seen.add(name)
                    address = VECTOR_BASE + (index + 1) * VECTOR_STRIDE
                    out.write('.section ".text.cross_%s", "ax", @progbits\n' % name)
                    out.write(".global %s\n" % name)
                    out.write("%s:\n" % name)
                    out.write("  JSR PAGED_GATE\n")
                    out.write("  .byte %d\n" % rom)
                    out.write("  .hword 0x%x\n" % address)
                    out.write("  RTS\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
