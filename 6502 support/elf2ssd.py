#!/usr/bin/env python3
"""Pack ELF image(s) into a single-sided DFS disc MOS can *RUN.

Examples:
  elf2ssd.py game.elf disc.ssd
      → PROG + !BOOT (*RUN PROG); Shift-Break boot option 3
  elf2ssd.py disc.ssd SNAKE=snake.elf INVADER=invaders.elf
      → named files + !BOOT (*CAT); *RUN SNAKE / *RUN INVADER

ELFs linked at &E00 (bbc_mode2_e00.ld) are wrapped automatically: DFS loads a
small stub at &1F00 that copies the image down to &E00 and jumps to the ELF
entry. That reclaims the DFS workspace (~4K). No *TAPE (OSCLI TAPE can BRK to
BASIC on a Master); the game must not call the filing system afterward.
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

# DFS PAGE on a Master with filing-system workspace claimed.
DFS_PAGE = 0x1F00
TAPE_DEST = 0x0E00


def build_reloc_stub(entry, payload_len, dfs_load=DFS_PAGE, dest=TAPE_DEST):
    """Two-stage relocator.

    Stage A runs at dfs_load (PAGE): copies this stub to STUB_PARK and jumps.
    Stage B runs at STUB_PARK (above the &E00 image): copies the payload down
    to dest and JMPs to entry.

    A single in-place copy from PAGE into &E00..end would overwrite the running
    stub once dest reached &1F00 — that crashed straight back to BASIC.
    """
    # Park below &E00 so neither the dest image nor the PAGE-resident source
    # overlaps the running stub (&7900 sat inside the source and got trashed).
    stub_park = 0x0900

    # Build stage-B body as a blob we can copy; stage A prepends a trampoline.
    # We iterate until parked addresses and lengths are self-consistent.
    stub_len = 0
    while True:
        # --- Stage B: runs at stub_park ---
        b = bytearray()

        def bemit(*bs):
            for x in bs:
                b.append(x & 0xFF)

        def b_lda_imm(v):
            bemit(0xA9, v)

        def b_ldy_imm(v):
            bemit(0xA0, v)

        def b_sta_zp(z):
            bemit(0x85, z)

        def b_jmp(addr):
            bemit(0x4C, addr & 0xFF, (addr >> 8) & 0xFF)

        # After stage A, payload sits at dfs_load + stub_len
        src_addr = dfs_load + stub_len
        b_lda_imm(src_addr & 0xFF)
        b_sta_zp(0x70)
        b_lda_imm((src_addr >> 8) & 0xFF)
        b_sta_zp(0x71)
        b_lda_imm(dest & 0xFF)
        b_sta_zp(0x72)
        b_lda_imm((dest >> 8) & 0xFF)
        b_sta_zp(0x73)
        b_lda_imm(payload_len & 0xFF)
        b_sta_zp(0x74)
        b_lda_imm((payload_len >> 8) & 0xFF)
        b_sta_zp(0x75)
        b_ldy_imm(0)
        loop = len(b)
        bemit(0xA5, 0x74)
        bemit(0x05, 0x75)
        beq_at = len(b)
        bemit(0xF0, 0x00)
        bemit(0xB1, 0x70)
        bemit(0x91, 0x72)
        bemit(0xE6, 0x70)
        bemit(0xD0, 0x02)
        bemit(0xE6, 0x71)
        bemit(0xE6, 0x72)
        bemit(0xD0, 0x02)
        bemit(0xE6, 0x73)
        bemit(0xA5, 0x74)
        bemit(0xD0, 0x02)
        bemit(0xC6, 0x75)
        bemit(0xC6, 0x74)
        b_jmp(stub_park + loop)
        done = len(b)
        b[beq_at + 1] = (done - (beq_at + 2)) & 0xFF
        b_jmp(entry)
        stage_b = bytes(b)

        # --- Stage A: runs at dfs_load, copies stage_b to stub_park, jumps ---
        a = bytearray()

        def aemit(*bs):
            for x in bs:
                a.append(x & 0xFF)

        def a_lda_imm(v):
            aemit(0xA9, v)

        def a_ldy_imm(v):
            aemit(0xA0, v)

        def a_sta_zp(z):
            aemit(0x85, z)

        def a_jmp(addr):
            aemit(0x4C, addr & 0xFF, (addr >> 8) & 0xFF)

        # Source of stage B within the loaded file: dfs_load + len(stage_a)
        # stage_a length depends on this block — iterate.
        stage_a_len_guess = 40
        while True:
            a = bytearray()
            stage_b_src = dfs_load + stage_a_len_guess

            def aemit2(*bs):
                for x in bs:
                    a.append(x & 0xFF)

            def a_lda_imm(v):
                aemit2(0xA9, v)

            def a_ldy_imm(v):
                aemit2(0xA0, v)

            def a_sta_zp(z):
                aemit2(0x85, z)

            def a_jmp(addr):
                aemit2(0x4C, addr & 0xFF, (addr >> 8) & 0xFF)

            a_lda_imm(stage_b_src & 0xFF)
            a_sta_zp(0x70)
            a_lda_imm((stage_b_src >> 8) & 0xFF)
            a_sta_zp(0x71)
            a_lda_imm(stub_park & 0xFF)
            a_sta_zp(0x72)
            a_lda_imm((stub_park >> 8) & 0xFF)
            a_sta_zp(0x73)
            a_lda_imm(len(stage_b) & 0xFF)
            a_sta_zp(0x74)
            a_lda_imm((len(stage_b) >> 8) & 0xFF)
            a_sta_zp(0x75)
            a_ldy_imm(0)
            aloop = len(a)
            aemit2(0xA5, 0x74)
            aemit2(0x05, 0x75)
            abeq = len(a)
            aemit2(0xF0, 0x00)
            aemit2(0xB1, 0x70)
            aemit2(0x91, 0x72)
            aemit2(0xE6, 0x70)
            aemit2(0xD0, 0x02)
            aemit2(0xE6, 0x71)
            aemit2(0xE6, 0x72)
            aemit2(0xD0, 0x02)
            aemit2(0xE6, 0x73)
            aemit2(0xA5, 0x74)
            aemit2(0xD0, 0x02)
            aemit2(0xC6, 0x75)
            aemit2(0xC6, 0x74)
            a_jmp(dfs_load + aloop)
            adone = len(a)
            a[abeq + 1] = (adone - (abeq + 2)) & 0xFF
            a_jmp(stub_park)
            if len(a) == stage_a_len_guess:
                stage_a = bytes(a)
                break
            stage_a_len_guess = len(a)

        stub = stage_a + stage_b
        # Payload begins immediately after stub
        if len(stub) == stub_len and (dfs_load + len(stub)) == src_addr:
            # Also require stage_b_src == dfs_load + len(stage_a)
            if stage_b_src == dfs_load + len(stage_a):
                return stub
        stub_len = len(stub)


def wrap_tape_e00(entry, load, image, dfs_load=DFS_PAGE, dest=TAPE_DEST):
    """Prepend &E00 relocator. Returns (payload, dfs_load, dfs_exec)."""
    if load != dest:
        raise SystemExit(
            "e00 wrap expects ELF load %#x, got %#x" % (dest, load)
        )
    if entry < load or entry >= load + len(image):
        raise SystemExit("ELF entry %#x is outside the image" % entry)
    stub = build_reloc_stub(entry, len(image), dfs_load=dfs_load, dest=dest)
    return stub + image, dfs_load, dfs_load


def maybe_wrap_tape_e00(entry, load, image):
    """Auto-wrap images linked at &E00 for DFS *RUN from PAGE."""
    if load == TAPE_DEST:
        return wrap_tape_e00(entry, load, image)
    return image, load, entry


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


def build_ssd(files, boot_text):
    """files: list of (name, payload, load, exec). boot_text: !BOOT body or None."""
    packed = list(files)
    if boot_text is not None:
        packed.append(("!BOOT", boot_text, 0, 0))
    entries = []
    sector = 2
    for name, payload, file_load, file_exec in packed:
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
    boot_exec = (3 << 4) if boot_text is not None else 0
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


def parse_named_elf(spec):
    if "=" not in spec:
        raise SystemExit("expected NAME=elf, got %r" % spec)
    name, path = spec.split("=", 1)
    name = name.strip().upper()
    if not name or name == "!BOOT":
        raise SystemExit("invalid DFS name in %r" % spec)
    execute, load, image = read_elf(path)
    if load > 0xFFFF or execute > 0xFFFF:
        raise SystemExit("%s: BBC load/exec address is above 64K" % path)
    image, load, execute = maybe_wrap_tape_e00(execute, load, image)
    return name, image, load, execute, path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "args",
        nargs="+",
        help="elf ssd  OR  ssd NAME=elf [NAME=elf ...]",
    )
    parser.add_argument(
        "--boot",
        default=None,
        help="!BOOT text (default: *RUN PROG for single PROG, else *CAT)",
    )
    parser.add_argument(
        "--name",
        default="PROG",
        help="DFS name when packing a single elf ssd pair (default PROG)",
    )
    args = parser.parse_args()
    argv = args.args

    if len(argv) == 2 and "=" not in argv[0] and "=" not in argv[1]:
        elf_path, ssd_path = argv
        execute, load, image = read_elf(elf_path)
        if load > 0xFFFF or execute > 0xFFFF:
            raise SystemExit("BBC load/exec address is above 64K")
        image, load, execute = maybe_wrap_tape_e00(execute, load, image)
        name = args.name.upper()
        files = [(name, image, load, execute)]
        boot = args.boot
        if boot is None:
            boot = ("*RUN %s\r" % name).encode("ascii")
        elif boot == "":
            boot = None
        else:
            boot = boot.replace("\\r", "\r")
            if not boot.endswith("\r"):
                boot += "\r"
            boot = boot.encode("ascii")
        disc = build_ssd(files, boot)
        open(ssd_path, "wb").write(disc)
        print(
            "%s load=%#x exec=%#x bytes=%d end=%#x"
            % (name, load, execute, len(image), load + len(image))
        )
        return 0

    ssd_path = argv[0]
    specs = argv[1:]
    if not specs:
        raise SystemExit("need at least one NAME=elf")
    files = []
    for spec in specs:
        name, image, load, execute, path = parse_named_elf(spec)
        files.append((name, image, load, execute))
        print(
            "%s load=%#x exec=%#x bytes=%d end=%#x (%s)"
            % (name, load, execute, len(image), load + len(image), path)
        )
    boot = args.boot
    if boot is None:
        boot = b"*CAT\r"
    elif boot == "":
        boot = None
    else:
        boot = boot.replace("\\r", "\r")
        if not boot.endswith("\r"):
            boot += "\r"
        boot = boot.encode("ascii")
    disc = build_ssd(files, boot)
    open(ssd_path, "wb").write(disc)
    return 0


if __name__ == "__main__":
    sys.exit(main())
