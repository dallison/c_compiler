#!/usr/bin/env python3
"""Pack an ELF image into a single-sided DFS disc MOS can *RUN.

Shift-Break with boot option 3 *EXECs !BOOT, which is the text command
"*RUN PROG". PROG is the ELF PT_LOAD image, including zeroed BSS, with
the load and execution addresses taken from the ELF.
"""

import argparse
import struct
import sys

SHF_ALLOC = 2
SHT_NOBITS = 8
SECTOR = 256
TRACKS = 80
SECTORS_PER_TRACK = 10
DISC_SECTORS = TRACKS * SECTORS_PER_TRACK


def read_elf(path):
    data = open(path, "rb").read()
    if data[:4] != b"\x7fELF":
        raise SystemExit("%s is not an ELF file" % path)
    elf_class = data[4]
    sections = []
    if elf_class == 1:
        e_entry = struct.unpack_from("<I", data, 24)[0]
        e_shoff = struct.unpack_from("<I", data, 32)[0]
        e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", data, 46)

        def shdr(index):
            name, typ, flags, addr, offset, size, link, info, align, entsize = (
                struct.unpack_from("<IIIIIIIIII", data, e_shoff + index * e_shentsize)
            )
            return name, typ, flags, addr, offset, size
    elif elf_class == 2:
        e_entry = struct.unpack_from("<Q", data, 24)[0]
        e_shoff = struct.unpack_from("<Q", data, 40)[0]
        e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", data, 58)

        def shdr(index):
            name, typ, flags, addr, offset, size, _link, _info, _align, _entsize = (
                struct.unpack_from("<IIQQQQIIQQ", data, e_shoff + index * e_shentsize)
            )
            return name, typ, flags, addr, offset, size
    else:
        raise SystemExit("unsupported ELF class %s" % elf_class)
    name_off, _typ, _flags, _addr, str_off, str_size = shdr(e_shstrndx)
    del name_off
    names = data[str_off : str_off + str_size]
    for index in range(e_shnum):
        name_off, typ, flags, addr, offset, size = shdr(index)
        if (flags & SHF_ALLOC) == 0 or size == 0:
            continue
        name = names[name_off:].split(b"\0", 1)[0].decode()
        sections.append((name, typ, addr, offset, size))
    if not sections:
        raise SystemExit("ELF has no allocatable bytes")
    base = min(item[2] for item in sections)
    end = max(item[2] + item[4] for item in sections)
    image = bytearray(end - base)
    for _name, typ, addr, offset, size in sections:
        if typ == SHT_NOBITS:
            continue
        image[addr - base : addr - base + size] = data[offset : offset + size]
    return e_entry, base, bytes(image)


def catalogue_name(name):
    raw = name.encode("ascii")
    if len(raw) > 7:
        raise SystemExit("DFS names are at most 7 characters: %s" % name)
    return raw + b" " * (7 - len(raw))


def pack_address(value, which):
    if value < 0 or value > 0x3FFFF:
        raise SystemExit("address does not fit in 18 bits: %#x" % value)
    low = value & 0xFFFF
    high = (value >> 16) & 3
    return low, high << which


def build_ssd(prog, load, execute):
    files = [
        ("PROG", prog, load, execute),
        ("!BOOT", b"*RUN PROG\r", 0, 0),
    ]
    entries = []
    sector = 2
    for name, payload, file_load, file_exec in files:
        nsec = (len(payload) + SECTOR - 1) // SECTOR
        entries.append((name, payload, file_load, file_exec, sector, nsec))
        sector += nsec
    if sector > DISC_SECTORS:
        raise SystemExit("program does not fit on an 80-track DFS disc")
    entries.sort(key=lambda item: item[4], reverse=True)

    disc = bytearray(DISC_SECTORS * SECTOR)
    title = b"DAVECC"
    disc[0:8] = (title + b" " * 8)[:8]
    disc[0x100:0x104] = b" " * 4
    disc[0x104] = 1
    disc[0x105] = len(entries) * 8
    size_high = (DISC_SECTORS >> 8) & 3
    boot_exec = 3 << 4
    disc[0x106] = size_high | boot_exec
    disc[0x107] = DISC_SECTORS & 0xFF

    for index, (name, payload, file_load, file_exec, start, _nsec) in enumerate(entries):
        name_at = 8 + index * 8
        info_at = 0x100 + 8 + index * 8
        disc[name_at : name_at + 7] = catalogue_name(name)
        disc[name_at + 7] = ord("$")
        load_low, load_high = pack_address(file_load, 4)
        exec_low, exec_high = pack_address(file_exec, 0)
        length_low, length_high = pack_address(len(payload), 2)
        sector_high = (start >> 8) & 3
        disc[info_at : info_at + 2] = struct.pack("<H", load_low)
        disc[info_at + 2 : info_at + 4] = struct.pack("<H", exec_low)
        disc[info_at + 4 : info_at + 6] = struct.pack("<H", length_low)
        disc[info_at + 6] = exec_high | length_high | load_high | (sector_high << 6)
        disc[info_at + 7] = start & 0xFF
        disc[start * SECTOR : start * SECTOR + len(payload)] = payload
    return disc


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf")
    parser.add_argument("ssd")
    args = parser.parse_args()
    execute, load, image = read_elf(args.elf)
    if load > 0xFFFF or execute > 0xFFFF:
        raise SystemExit("BBC load/exec address is above 64K")
    disc = build_ssd(image, load, execute)
    open(args.ssd, "wb").write(disc)
    print(
        "PROG load=%#x exec=%#x bytes=%d end=%#x"
        % (load, execute, len(image), load + len(image))
    )


if __name__ == "__main__":
    sys.exit(main())
