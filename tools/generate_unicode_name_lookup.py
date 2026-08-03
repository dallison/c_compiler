#!/usr/bin/env python3
"""Generate the compact Unicode character-name lookup used by DaveCC."""

from __future__ import annotations

import argparse
import pathlib
import re
from dataclasses import dataclass, field


ALIAS_TYPES = {"alternate", "control", "correction"}
ALGORITHMIC_PREFIXES = (
    "CJK UNIFIED IDEOGRAPH-",
    "CJK COMPATIBILITY IDEOGRAPH-",
    "TANGUT IDEOGRAPH-",
    "NUSHU CHARACTER-",
    "KHITAN SMALL SCRIPT CHARACTER-",
)
SINGLE_CHARACTERS = " ABCDEFGHIJKLMNOPQRSTUVWXYZ-0123456789"
MAX_SEGMENT_LENGTH = 63


@dataclass
class Node:
    segment: str = ""
    value: int | None = None
    children: dict[str, "Node"] = field(default_factory=dict)


def insert(root: Node, name: str, value: int) -> None:
    node = root
    for character in name:
        node = node.children.setdefault(character, Node(character))
    if node.value is not None and node.value != value:
        raise ValueError(f"conflicting values for {name}")
    node.value = value


def compact(node: Node) -> None:
    for child in list(node.children.values()):
        compact(child)
    while (
        node.segment
        and node.value is None
        and len(node.children) == 1
    ):
        child = next(iter(node.children.values()))
        if len(node.segment) + len(child.segment) > MAX_SEGMENT_LENGTH:
            break
        node.segment += child.segment
        node.value = child.value
        node.children = child.children


def parse_unicode_data(
    path: pathlib.Path,
) -> tuple[dict[str, int], list[tuple[str, int, int]]]:
    names: dict[str, int] = {}
    ranges: list[tuple[str, int, int]] = []
    pending_range: tuple[str, int] | None = None
    explicit_algorithmic: dict[str, list[int]] = {
        prefix: [] for prefix in ALGORITHMIC_PREFIXES
    }

    for line in path.read_text(encoding="utf-8").splitlines():
        fields = line.split(";")
        codepoint = int(fields[0], 16)
        name = fields[1]
        if name.startswith("<") and name.endswith(", First>"):
            pending_range = (name[1:-8], codepoint)
            continue
        if name.startswith("<") and name.endswith(", Last>"):
            if pending_range is None or pending_range[0] != name[1:-7]:
                raise ValueError(f"unmatched UnicodeData range at U+{codepoint:X}")
            label, first = pending_range
            if label.startswith("CJK Ideograph"):
                ranges.append(("CJK UNIFIED IDEOGRAPH-", first, codepoint))
            elif label.startswith("Tangut Ideograph"):
                ranges.append(("TANGUT IDEOGRAPH-", first, codepoint))
            pending_range = None
            continue
        if not name or name.startswith("<"):
            continue
        algorithmic = False
        for prefix in ALGORITHMIC_PREFIXES:
            if name.startswith(prefix) and name[len(prefix):] == f"{codepoint:X}":
                explicit_algorithmic[prefix].append(codepoint)
                algorithmic = True
                break
        if not algorithmic:
            names[name] = codepoint

    for prefix, codepoints in explicit_algorithmic.items():
        if not codepoints:
            continue
        first = previous = codepoints[0]
        for codepoint in codepoints[1:]:
            if codepoint != previous + 1:
                ranges.append((prefix, first, previous))
                first = codepoint
            previous = codepoint
        ranges.append((prefix, first, previous))
    ranges.sort()
    return names, ranges


def parse_aliases(path: pathlib.Path, names: dict[str, int]) -> None:
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.partition("#")[0].strip()
        if not line:
            continue
        codepoint_text, name, alias_type = (
            field.strip() for field in line.split(";")
        )
        if alias_type not in ALIAS_TYPES:
            continue
        codepoint = int(codepoint_text, 16)
        previous = names.get(name)
        if previous is not None and previous != codepoint:
            raise ValueError(f"conflicting values for alias {name}")
        names[name] = codepoint


def make_dictionary(nodes: list[Node]) -> str:
    segments = sorted(
        {node.segment for node in nodes if len(node.segment) > 1},
        key=lambda segment: (-len(segment), segment),
    )
    dictionary = ""
    for segment in segments:
        if segment not in dictionary:
            dictionary += segment
    if len(dictionary) > 0xFFFF:
        raise ValueError("Unicode name dictionary exceeds 16-bit offsets")
    return dictionary


def flatten_breadth_first(root: Node) -> list[Node]:
    nodes: list[Node] = []
    pending = [root.children[key] for key in sorted(root.children)]
    while pending:
        node = pending.pop(0)
        nodes.append(node)
        pending.extend(node.children[key] for key in sorted(node.children))
    return nodes


def encode_nodes(nodes: list[Node], dictionary: str) -> bytes:
    offsets: dict[int, int] = {}
    child_patches: list[tuple[int, Node, bool]] = []
    output = bytearray()

    for node in nodes:
        offsets[id(node)] = len(output)
        children = [node.children[key] for key in sorted(node.children)]
        has_value = node.value is not None
        if len(node.segment) == 1:
            segment = SINGLE_CHARACTERS.index(node.segment)
        else:
            segment = 0x40 | len(node.segment)
        if has_value:
            segment |= 0x80
        output.append(segment)
        if len(node.segment) > 1:
            position = dictionary.index(node.segment)
            output.extend(position.to_bytes(2, "big"))

        if has_value:
            packed = node.value << 3
            if children:
                packed |= 0x2
            output.extend(packed.to_bytes(3, "big"))
            if children:
                child_patches.append((len(output), children[0], True))
                output.extend(b"\0\0\0")
        else:
            flags = 0x40 if children else 0
            output.append(flags)
            if children:
                child_patches.append((len(output) - 1, children[0], False))
                output.extend(b"\0\0")

    sibling_ids: set[int] = set()
    # Mark every child except the final child of a parent as having a sibling.
    for parent in nodes:
        children = [parent.children[key] for key in sorted(parent.children)]
        sibling_ids.update(id(child) for child in children[:-1])
    root_nodes = []
    referenced = {id(child) for node in nodes for child in node.children.values()}
    for node in nodes:
        if id(node) not in referenced:
            root_nodes.append(node)
    sibling_ids.update(id(node) for node in root_nodes[:-1])

    for index, node in enumerate(nodes):
        if id(node) not in sibling_ids:
            continue
        offset = offsets[id(node)]
        first = output[offset]
        cursor = offset + 1 + (2 if first & 0x40 else 0)
        if first & 0x80:
            output[cursor + 2] |= 0x1
        else:
            output[cursor] |= 0x80

    for position, child, value_node in child_patches:
        offset = offsets[id(child)]
        if value_node:
            output[position:position + 3] = offset.to_bytes(3, "big")
        else:
            output[position] |= (offset >> 16) & 0x3F
            output[position + 1:position + 3] = (offset & 0xFFFF).to_bytes(
                2, "big"
            )
    output.extend(b"\0" * 8)
    return bytes(output)
def c_string(text: str) -> str:
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def lookup_generated(
    name: str, dictionary: str, index: bytes, offset: int = 0,
    position: int = 0
) -> int | None:
    while offset < len(index):
        descriptor = index[offset]
        offset += 1
        has_value = bool(descriptor & 0x80)
        long_segment = bool(descriptor & 0x40)
        descriptor &= 0x3F
        if long_segment:
            dictionary_offset = int.from_bytes(index[offset:offset + 2], "big")
            offset += 2
            segment = dictionary[
                dictionary_offset:dictionary_offset + descriptor
            ]
        else:
            segment = SINGLE_CHARACTERS[descriptor]
        value = None
        children_offset = None
        if has_value:
            packed = int.from_bytes(index[offset:offset + 3], "big")
            offset += 3
            value = packed >> 3
            has_sibling = bool(packed & 1)
            has_children = bool(packed & 2)
            if has_children:
                children_offset = int.from_bytes(
                    index[offset:offset + 3], "big"
                )
                offset += 3
        else:
            flags = index[offset]
            offset += 1
            has_sibling = bool(flags & 0x80)
            has_children = bool(flags & 0x40)
            if has_children:
                children_offset = (
                    (flags & 0x3F) << 16
                    | int.from_bytes(index[offset:offset + 2], "big")
                )
                offset += 2
        if name.startswith(segment, position):
            next_position = position + len(segment)
            if next_position == len(name) and value is not None:
                return value
            if children_offset is not None and next_position < len(name):
                result = lookup_generated(
                    name, dictionary, index, children_offset, next_position
                )
                if result is not None:
                    return result
        if not has_sibling:
            return None
    return None


def verify_generated(
    names: dict[str, int], dictionary: str, index: bytes
) -> None:
    for name, expected in names.items():
        actual = lookup_generated(name, dictionary, index)
        if actual != expected:
            raise ValueError(
                f"generated lookup failed for {name}: "
                f"expected U+{expected:04X}, got {actual}"
            )


def write_generated(
    output_directory: pathlib.Path,
    version: str,
    dictionary: str,
    index: bytes,
    ranges: list[tuple[str, int, int]],
) -> None:
    header = output_directory / "unicode_name_data.h"
    source = output_directory / "unicode_name_data.c"
    header.write_text(
        """// Generated by tools/generate_unicode_name_lookup.py. Do not edit.
#ifndef unicode_name_data_h
#define unicode_name_data_h

#include <stddef.h>
#include <stdint.h>

typedef struct {
  const char* prefix;
  uint32_t first;
  uint32_t last;
} UnicodeAlgorithmicNameRange;

extern const char kUnicodeNameDictionary[];
extern const uint8_t kUnicodeNameIndex[];
extern const size_t kUnicodeNameIndexSize;
extern const UnicodeAlgorithmicNameRange kUnicodeAlgorithmicNameRanges[];
extern const size_t kUnicodeAlgorithmicNameRangeCount;
extern const char kUnicodeNameDataVersion[];

#endif
""",
        encoding="utf-8",
    )
    byte_lines = []
    for start in range(0, len(index), 16):
        byte_lines.append(
            "  " + ", ".join(f"0x{byte:02x}" for byte in index[start:start + 16])
        )
    range_lines = [
        f"  {{{c_string(prefix)}, 0x{first:x}, 0x{last:x}}}"
        for prefix, first, last in ranges
    ]
    dictionary_lines = [
        "  " + c_string(dictionary[start:start + 72])
        for start in range(0, len(dictionary), 72)
    ]
    source.write_text(
        """// Generated by tools/generate_unicode_name_lookup.py. Do not edit.
#include "unicode_name_data.h"

const char kUnicodeNameDictionary[] =
"""
        + "\n".join(dictionary_lines)
        + """;

const uint8_t kUnicodeNameIndex[] = {
"""
        + ",\n".join(byte_lines)
        + """
};

const size_t kUnicodeNameIndexSize = sizeof(kUnicodeNameIndex);

const UnicodeAlgorithmicNameRange kUnicodeAlgorithmicNameRanges[] = {
"""
        + ",\n".join(range_lines)
        + """
};

const size_t kUnicodeAlgorithmicNameRangeCount =
    sizeof(kUnicodeAlgorithmicNameRanges) /
    sizeof(kUnicodeAlgorithmicNameRanges[0]);

const char kUnicodeNameDataVersion[] = """
        + c_string(version)
        + """;
""",
        encoding="utf-8",
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ucd", required=True, type=pathlib.Path)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    parser.add_argument("--version", required=True)
    arguments = parser.parse_args()

    names, ranges = parse_unicode_data(arguments.ucd / "UnicodeData.txt")
    parse_aliases(arguments.ucd / "NameAliases.txt", names)
    root = Node()
    for name, codepoint in sorted(names.items()):
        if not re.fullmatch(r"[A-Z0-9 -]+", name):
            raise ValueError(f"name is not an n-char-sequence: {name}")
        insert(root, name, codepoint)
    for child in root.children.values():
        compact(child)
    nodes = flatten_breadth_first(root)
    dictionary = make_dictionary(nodes)
    index = encode_nodes(nodes, dictionary)
    verify_generated(names, dictionary, index)
    arguments.output.mkdir(parents=True, exist_ok=True)
    write_generated(
        arguments.output, arguments.version, dictionary, index, ranges
    )
    print(
        f"generated {len(names)} names: dictionary={len(dictionary)} bytes, "
        f"index={len(index)} bytes, ranges={len(ranges)}"
    )


if __name__ == "__main__":
    main()
