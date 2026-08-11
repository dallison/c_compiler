#!/usr/bin/env python3
import os
import struct
import sys


def be32(value):
    return struct.pack(">I", value & 0xFFFFFFFF)


def be64(value):
    return struct.pack(">Q", value & 0xFFFFFFFFFFFFFFFF)


def build_block(transitions, types, leaps, use64):
    time_count = len(transitions)
    type_count = len(types)
    abbr_blob = b"".join(abbr.encode("ascii") + b"\0" for _, _, abbr in types)
    char_count = len(abbr_blob)
    leap_count = len(leaps)

    header = b"TZif" + (b"2" if use64 else b"\0") + b"\0" * 15
    header += be32(0)
    header += be32(0)
    header += be32(time_count)
    header += be32(type_count)
    header += be32(char_count)
    header += be32(leap_count)

    body = b""
    for transition_time, _type_index in transitions:
        body += be64(transition_time) if use64 else be32(int(transition_time))
    for _transition_time, type_index in transitions:
        body += struct.pack("B", type_index)
    abbr_indices = []
    offset = 0
    for _, _, abbr in types:
        abbr_indices.append(offset)
        offset += len(abbr.encode("ascii")) + 1
    for offset_seconds, is_dst, _abbr in types:
        body += be32(int(offset_seconds))
        body += struct.pack("BB", 1 if is_dst else 0, abbr_indices.pop(0))
    body += abbr_blob
    for leap_time, correction in leaps:
        if use64:
            body += be64(int(leap_time))
            body += be32(int(correction))
        else:
            body += be32(int(leap_time))
            body += be32(int(correction))
    return header + body


def build_tzif_v2(transitions, types, leaps=()):
    v1 = build_block(transitions, types, leaps, False)
    v2 = build_block(transitions, types, leaps, True)
    return v1 + b"\n2\n" + v2 + b"\nTZif\n"


def write_fixture(directory, name, data):
    path = os.path.join(directory, name)
    with open(path, "wb") as handle:
        handle.write(data)


def main():
    output = sys.argv[1] if len(sys.argv) > 1 else "tests/fixtures/tzif"
    os.makedirs(output, exist_ok=True)
    os.makedirs(os.path.join(output, "tzdata"), exist_ok=True)

    write_fixture(
        output,
        "UTC",
        build_tzif_v2([], [(0, 0, "UTC")]),
    )
    write_fixture(
        output,
        "FixedOffset",
        build_tzif_v2([], [(3600, 0, "FIX")]),
    )
    spring = 1711843200
    fall = 1730592000
    write_fixture(
        output,
        "DST",
        build_tzif_v2(
            [(spring, 1), (fall, 0)],
            [(0, 0, "STD"), (3600, 1, "DST")],
        ),
    )
    write_fixture(
        output,
        "LeapZone",
        build_tzif_v2([], [(0, 0, "UTC")], leaps=[(78796800, 1)]),
    )

    alias_path = os.path.join(output, "Alias")
    if os.path.lexists(alias_path):
        os.remove(alias_path)
    os.symlink("FixedOffset", alias_path)

    with open(os.path.join(output, "tzdata", "version"), "w", encoding="ascii") as handle:
        handle.write("test2024a\n")


if __name__ == "__main__":
    main()
