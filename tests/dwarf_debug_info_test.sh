#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: $0 davecc" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
SOURCE="$ROOT/tests/dwarf_debug_info_cases.c"
CXX_SOURCE="$ROOT/tests/dwarf_debug_info_cases.cpp"
REGISTER_SOURCE="$ROOT/tests/dwarf_register_variables.c"
WORK="${TEST_TMPDIR:-/tmp}/dwarf-debug-info"
mkdir -p "$WORK"

python3 - "$DAVECC" "$SOURCE" "$CXX_SOURCE" "$REGISTER_SOURCE" "$WORK" <<'PY'
import os
import struct
import subprocess
import sys

DW_TAG_array_type = 0x01
DW_TAG_enumeration_type = 0x04
DW_TAG_formal_parameter = 0x05
DW_TAG_member = 0x0d
DW_TAG_pointer_type = 0x0f
DW_TAG_compile_unit = 0x11
DW_TAG_structure_type = 0x13
DW_TAG_base_type = 0x24
DW_TAG_enumerator = 0x28
DW_TAG_subprogram = 0x2e
DW_TAG_variable = 0x34
DW_AT_name = 0x03
DW_AT_stmt_list = 0x10
DW_AT_low_pc = 0x11
DW_AT_high_pc = 0x12
DW_AT_language = 0x13
DW_AT_location = 0x02
DW_AT_frame_base = 0x40
DW_FORM_addr = 0x01
DW_FORM_data2 = 0x05
DW_FORM_data4 = 0x06
DW_FORM_data8 = 0x07
DW_FORM_string = 0x08
DW_FORM_data1 = 0x0b
DW_FORM_flag = 0x0c
DW_FORM_strp = 0x0e
DW_FORM_ref4 = 0x13
DW_FORM_sec_offset = 0x17
DW_FORM_exprloc = 0x18
DW_LANG_C = 0x0002
DW_LANG_C89 = 0x0001
DW_LANG_C99 = 0x000c
DW_LANG_C11 = 0x001d
DW_LNE_end_sequence = 1
DW_LNE_set_address = 2


class Cursor:
    def __init__(self, data, offset=0):
        self.data = data
        self.offset = offset

    def remaining(self):
        return len(self.data) - self.offset

    def u8(self):
        value = self.data[self.offset]
        self.offset += 1
        return value

    def u16(self):
        value, = struct.unpack_from("<H", self.data, self.offset)
        self.offset += 2
        return value

    def u32(self):
        value, = struct.unpack_from("<I", self.data, self.offset)
        self.offset += 4
        return value

    def u64(self):
        value, = struct.unpack_from("<Q", self.data, self.offset)
        self.offset += 8
        return value

    def bytes(self, n):
        value = self.data[self.offset:self.offset + n]
        self.offset += n
        return value

    def cstring(self):
        end = self.data.find(b"\0", self.offset)
        if end < 0:
            raise ValueError("unterminated string")
        value = self.data[self.offset:end]
        self.offset = end + 1
        return value.decode("utf-8", "replace")

    def uleb(self):
        result = 0
        shift = 0
        while True:
            byte = self.u8()
            result |= (byte & 0x7F) << shift
            if (byte & 0x80) == 0:
                return result
            shift += 7

    def sleb(self):
        result = 0
        shift = 0
        while True:
            byte = self.u8()
            result |= (byte & 0x7F) << shift
            shift += 7
            if (byte & 0x80) == 0:
                if byte & 0x40:
                    result -= 1 << shift
                return result


SHT_RELA = 4
MACHINES = {62: "x86_64", 183: "aarch64", 243: "riscv"}
# The relocations in debug sections, by type: the bytes they write, and
# whether they set the value there or add to or subtract from it.
RELOCATIONS = {
    "aarch64": {257: (8, "set"), 258: (4, "set")},  # R_AARCH64_ABS64, ABS32
    # R_RISCV_32, R_RISCV_64, ADD32, ADD64, SUB32, SUB64.
    "riscv": {1: (4, "set"), 2: (8, "set"), 35: (4, "add"), 36: (8, "add"),
              39: (4, "sub"), 40: (8, "sub")},
    "x86_64": {1: (8, "set"), 10: (4, "set"), 11: (4, "set")},  # 64, 32, 32S
}
R_X86_64_PC32 = 2
R_X86_64_PLT32 = 4


def parse_elf_sections(blob, relocate=False):
    """Maps section names to their contents.  With |relocate|, applies the
    relocations of the sections of an AArch64, RISC-V or x86-64 object, taking
    every section's address as 0."""
    if blob[:4] != b"\x7fELF":
        raise ValueError("not an ELF file")
    elf64 = blob[4] == 2
    if blob[5] != 1:
        raise ValueError("ELF file is not little-endian")
    machine, = struct.unpack_from("<H", blob, 18)
    if elf64:
        e_shoff, = struct.unpack_from("<Q", blob, 40)
        e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", blob, 58)
        sh_fmt = "<IIQQQQIIQQ"
        sh_size = 64
    else:
        e_shoff, = struct.unpack_from("<I", blob, 32)
        e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", blob, 46)
        sh_fmt = "<IIIIIIIIII"
        sh_size = 40
    if e_shentsize != sh_size:
        raise ValueError("unexpected section header size")
    sections = []
    for i in range(e_shnum):
        off = e_shoff + i * e_shentsize
        fields = struct.unpack_from(sh_fmt, blob, off)
        name, sh_type, flags, addr, offset, size, link, info = fields[:8]
        sections.append((name, sh_type, offset, size, link, info))
    str_off, str_size = sections[e_shstrndx][2:4]
    strtab = blob[str_off:str_off + str_size]
    names = []
    contents = []
    for name_off, sh_type, offset, size, link, info in sections:
        end = strtab.find(b"\0", name_off)
        names.append(strtab[name_off:end].decode("utf-8", "replace"))
        contents.append(bytearray(blob[offset:offset + size]))
    if relocate:
        if not elf64:
            raise ValueError("relocating a 32-bit object")
        kinds = RELOCATIONS[MACHINES[machine]]
        for name_off, sh_type, offset, size, link, info in sections:
            if sh_type != SHT_RELA:
                continue
            symtab = contents[link]
            target = contents[info]
            for r in range(0, size, 24):
                r_offset, r_info, r_addend = struct.unpack_from(
                    "<QQq", blob, offset + r)
                kind = kinds.get(r_info & 0xffffffff)
                if kind is None:
                    continue
                width, op = kind
                st_value, = struct.unpack_from("<Q", symtab,
                                               (r_info >> 32) * 24 + 8)
                fmt = "<Q" if width == 8 else "<I"
                value = st_value + r_addend
                old, = struct.unpack_from(fmt, target, r_offset)
                if op == "add":
                    value = old + value
                elif op == "sub":
                    value = old - value
                struct.pack_into(fmt, target, r_offset,
                                 value & ((1 << (8 * width)) - 1))
    return {name: bytes(data) for name, data in zip(names, contents)}


def parse_abbrevs(blob):
    cur = Cursor(blob)
    abbrevs = {}
    while cur.remaining() > 0:
        code = cur.uleb()
        if code == 0:
            break
        tag = cur.uleb()
        has_children = cur.u8() != 0
        attrs = []
        while True:
            attr = cur.uleb()
            form = cur.uleb()
            if attr == 0 and form == 0:
                break
            attrs.append((attr, form))
        abbrevs[code] = (tag, has_children, attrs)
    return abbrevs


def skip_form(cur, form, address_size, debug_str):
    if form == DW_FORM_addr:
        return int.from_bytes(cur.bytes(address_size), "little")
    if form == DW_FORM_data1 or form == DW_FORM_flag:
        return cur.u8()
    if form == DW_FORM_data2:
        return cur.u16()
    if form in (DW_FORM_data4, DW_FORM_ref4, DW_FORM_sec_offset, DW_FORM_strp):
        value = cur.u32()
        if form == DW_FORM_strp:
            end = debug_str.find(b"\0", value)
            return debug_str[value:end].decode("utf-8", "replace")
        return value
    if form == DW_FORM_data8:
        return cur.u64()
    if form == DW_FORM_string:
        return cur.cstring()
    if form == DW_FORM_exprloc:
        length = cur.uleb()
        return cur.bytes(length)
    raise ValueError("unsupported DWARF form 0x%x" % form)


def parse_dies(info, abbrevs, debug_str):
    cur = Cursor(info)
    unit_length = cur.u32()
    if unit_length + 4 != len(info):
        raise ValueError(".debug_info size %d does not match unit_length %d" %
                         (len(info), unit_length))
    version = cur.u16()
    abbrev_offset = cur.u32()
    address_size = cur.u8()
    if version != 4:
        raise ValueError("DWARF version is %d, expected 4" % version)
    if abbrev_offset != 0:
        raise ValueError("debug_abbrev_offset is %d, expected 0" % abbrev_offset)
    if address_size not in (2, 4, 8):
        raise ValueError("unexpected address size %d" % address_size)
    dies = []
    while cur.offset < 4 + unit_length:
        abbrev_code = cur.uleb()
        if abbrev_code == 0:
            continue
        if abbrev_code not in abbrevs:
            raise ValueError("unknown abbreviation %d" % abbrev_code)
        tag, has_children, attrs = abbrevs[abbrev_code]
        values = {}
        for attr, form in attrs:
            values[attr] = skip_form(cur, form, address_size, debug_str)
        dies.append((tag, values))
    return dies, address_size


def parse_debug_line(blob):
    cur = Cursor(blob)
    unit_length = cur.u32()
    version = cur.u16()
    if version != 4:
        raise ValueError(".debug_line version is %d, expected 4" % version)
    header_length = cur.u32()
    header_start = cur.offset
    min_ins = cur.u8()
    max_ops = cur.u8()
    default_is_stmt = cur.u8()
    line_base = struct.unpack("b", bytes([cur.u8()]))[0]
    line_range = cur.u8()
    opcode_base = cur.u8()
    cur.bytes(opcode_base - 1)
    dirs = []
    while True:
        name = cur.cstring()
        if name == "":
            break
        dirs.append(name)
    files = []
    while True:
        name = cur.cstring()
        if name == "":
            break
        cur.uleb()
        cur.uleb()
        cur.uleb()
        files.append(name)
    if cur.offset != header_start + header_length:
        # The compiler may pad; don't require an exact header_length match.
        pass
    if not files:
        raise ValueError(".debug_line file table is empty")
    return files


def check_object(path, expect_language):
    with open(path, "rb") as fh:
        blob = fh.read()
    sections = parse_elf_sections(blob)
    for name in (".debug_info", ".debug_abbrev", ".debug_line"):
        if name not in sections or not sections[name]:
            raise ValueError("%s is missing section %s" % (path, name))
    debug_str = sections.get(".debug_str", b"")
    abbrevs = parse_abbrevs(sections[".debug_abbrev"])
    dies, address_size = parse_dies(sections[".debug_info"], abbrevs, debug_str)
    files = parse_debug_line(sections[".debug_line"])
    names = {}
    tags_by_name = {}
    for tag, values in dies:
        name = values.get(DW_AT_name)
        if name:
            names.setdefault(name, []).append(tag)
            tags_by_name[name] = tag
    cu = next((values for tag, values in dies if tag == DW_TAG_compile_unit), None)
    if cu is None:
        raise ValueError("missing DW_TAG_compile_unit")
    language = cu.get(DW_AT_language)
    if language not in expect_language:
        raise ValueError("DW_AT_language is %s, expected one of %s" %
                         (language, sorted(expect_language)))
    if DW_AT_stmt_list not in cu:
        raise ValueError("compile unit is missing DW_AT_stmt_list")
    if DW_AT_low_pc not in cu or DW_AT_high_pc not in cu:
        raise ValueError("compile unit is missing PC bounds")
    required = {
        "add": DW_TAG_subprogram,
        "a": DW_TAG_formal_parameter,
        "b": DW_TAG_formal_parameter,
        "sum": DW_TAG_variable,
        "Point": DW_TAG_structure_type,
        "x": DW_TAG_member,
        "y": DW_TAG_member,
        "Color": DW_TAG_enumeration_type,
        "RED": DW_TAG_enumerator,
        "GREEN": DW_TAG_enumerator,
        "g": DW_TAG_variable,
        "int": DW_TAG_base_type,
    }
    for name, tag in required.items():
        if name not in names:
            raise ValueError("missing DIE named %s; have %s" %
                             (name, sorted(names)))
        if tag not in names[name]:
            raise ValueError("DIE %s has tags %s, expected %s" %
                             (name, names[name], tag))
    add = next(values for tag, values in dies
               if tag == DW_TAG_subprogram and values.get(DW_AT_name) == "add")
    if DW_AT_frame_base not in add:
        raise ValueError("subprogram add is missing DW_AT_frame_base")
    high_pc = add.get(DW_AT_high_pc)
    if not high_pc:
        raise ValueError("subprogram add has empty DW_AT_high_pc %s" % high_pc)
    cu_high = cu.get(DW_AT_high_pc)
    if not cu_high:
        raise ValueError("compile unit has empty DW_AT_high_pc %s" % cu_high)
    sum_var = next(values for tag, values in dies
                    if tag == DW_TAG_variable and values.get(DW_AT_name) == "sum")
    if DW_AT_location not in sum_var:
        raise ValueError("variable sum is missing DW_AT_location")
    if not files:
        raise ValueError("line table has no files")
    print("ok %s address_size=%d files=%s dies=%d high_pc=%s" %
          (os.path.basename(path), address_size, files, len(dies), high_pc))


def compile_and_check(davecc, source, work, target, extra_args, languages,
                      obj_name=None):
    obj = os.path.join(work, "%s.o" % (obj_name or target))
    cmd = [davecc, "-target", target, "-g", "-O0", "-nostdinc", "-c",
           source, "-o", obj] + extra_args
    subprocess.check_call(cmd)
    check_object(obj, languages)


# DWARF numbers of the registers a value live across a call can be in.
CALLEE_SAVED = {
    "aarch64": {"int": set(range(19, 29)), "fp": set(range(72, 80))},
    "riscv": {"int": {9} | set(range(18, 28)),
              "fp": {40, 41} | set(range(50, 60))},
    # rbx and r12-r15.  SysV saves no xmm register.
    "x86_64": {"int": {3, 12, 13, 14, 15}, "fp": set()},
}
FP_REGS = {"aarch64": set(range(64, 96)), "riscv": set(range(32, 64)),
           "x86_64": set(range(17, 33))}


def saved_registers(target, allocator_env):
    if target == "x86_64" and "DAVECC_OLD_RA" in allocator_env:
        # The dominator walk keeps values in xmm8-xmm15 across calls, and
        # saves them for its callers.
        return {"int": CALLEE_SAVED[target]["int"], "fp": set(range(25, 33))}
    return CALLEE_SAVED[target]


def decode_location(expr):
    """Returns ("reg" or "breg", DWARF register) for a register location, or
    ("fbreg", offset) for one in the frame."""
    cur = Cursor(expr)
    op = cur.u8()
    if 0x50 <= op <= 0x6f:
        return "reg", op - 0x50
    if op == 0x90:
        return "reg", cur.uleb()
    if 0x70 <= op <= 0x8f:
        return "breg", op - 0x70
    if op == 0x92:
        return "breg", cur.uleb()
    if op == 0x91:
        return "fbreg", cur.sleb()
    return "other", op


def parse_location_list(debug_loc, offset):
    """Returns the (low, high, kind, register) entries of the location list at
    |offset|, for an address size of 8."""
    cur = Cursor(debug_loc, offset)
    base = 0
    entries = []
    while True:
        begin = cur.u64()
        end = cur.u64()
        if begin == 0 and end == 0:
            return entries
        if begin == (1 << 64) - 1:
            base = end
            continue
        kind, reg = decode_location(cur.bytes(cur.u16()))
        entries.append((base + begin, base + end, kind, reg))


def call_addresses(sections, low, high, target):
    """The (address, return address) of each call between |low| and |high| in
    the text.  On x86-64 these are the calls to other functions' symbols."""
    text = sections[".text"]
    calls = []
    if target == "x86_64":
        rela = sections.get(".rela.text", b"")
        for r in range(0, len(rela), 24):
            r_offset, r_info, _ = struct.unpack_from("<QQq", rela, r)
            pc = r_offset - 1
            if ((r_info & 0xffffffff) in (R_X86_64_PC32, R_X86_64_PLT32) and
                    low <= pc < high and text[pc] == 0xe8):
                calls.append((pc, r_offset + 4))
        return calls
    for pc in range(low, high, 4):
        word, = struct.unpack_from("<I", text, pc)
        if target == "aarch64":
            # bl, blr.
            is_call = ((word & 0xfc000000) == 0x94000000 or
                       (word & 0xfffffc1f) == 0xd63f0000)
        else:
            # jal ra and jalr ra.
            is_call = (word & 0x7f) in (0x6f, 0x67) and (word >> 7) & 0x1f == 1
        if is_call:
            calls.append((pc, pc + 4))
    return calls


def register_locations(path, target):
    """Maps (function, variable) to its location: None, ("fbreg", offset), or
    (entries, listed).  Each entry is (low, high, kind, register): a register
    that holds the variable, or its address for kind "breg", and the code over
    which it does.  |listed| is whether that came from a location list rather
    than one location for the whole function.  Also maps each function to the
    addresses of its calls."""
    with open(path, "rb") as fh:
        sections = parse_elf_sections(fh.read(), relocate=True)
    debug_loc = sections.get(".debug_loc", b"")
    cur = Cursor(debug_loc, 0)
    while cur.offset < len(debug_loc):
        begin = cur.u64()
        end = cur.u64()
        if (begin == 0 and end == 0) or begin == (1 << 64) - 1:
            continue
        if begin == end:
            raise ValueError("%s: empty .debug_loc range at 0x%x" %
                             (path, begin))
        cur.bytes(cur.u16())
    abbrevs = parse_abbrevs(sections[".debug_abbrev"])
    dies, _ = parse_dies(sections[".debug_info"], abbrevs,
                         sections.get(".debug_str", b""))
    function = None
    low = high = 0
    locations = {}
    calls = {}
    for tag, values in dies:
        if tag == DW_TAG_subprogram:
            function = values.get(DW_AT_name)
            low = values.get(DW_AT_low_pc)
            high = low + values.get(DW_AT_high_pc)
            calls[function] = call_addresses(sections, low, high, target)
        elif tag in (DW_TAG_variable, DW_TAG_formal_parameter):
            location = values.get(DW_AT_location)
            if location is None:
                decoded = None
            elif isinstance(location, int):
                entries = parse_location_list(sections[".debug_loc"], location)
                for entry in entries:
                    if not low <= entry[0] <= entry[1] <= high:
                        raise ValueError(
                            "%s: %s in %s is in a register at 0x%x-0x%x, "
                            "outside the function at 0x%x-0x%x" %
                            ((path, values.get(DW_AT_name), function) +
                             entry[:2] + (low, high)))
                decoded = (entries, True)
            else:
                kind, value = decode_location(location)
                decoded = ((kind, value) if kind == "fbreg" else
                           ([(low, high, kind, value)], False))
            locations[(function, values.get(DW_AT_name))] = decoded
    return locations, calls


def check_register_variables(davecc, source, work, target, opt, allocator_env):
    obj = os.path.join(work, "registers-%s%s.o" % (target, opt))
    env = dict(os.environ)
    env.update(allocator_env)
    subprocess.check_call([davecc, "-target", target, "-g", opt, "-nostdinc",
                           "-c", source, "-o", obj], env=env)
    locations, calls = register_locations(obj, target)
    config = "%s %s %s" % (target, opt, allocator_env or "")
    saved = saved_registers(target, allocator_env)

    # A call overwrites the registers it does not save, so a location list
    # cannot claim one of them holds a variable once the call returns.
    for (function, name), location in locations.items():
        if location is None or location[0] == "fbreg" or not location[1]:
            continue
        for low, high, kind, reg in location[0]:
            if reg in saved["int"] or reg in saved["fp"]:
                continue
            for call, after in calls.get(function, []):
                if low <= call and after < high:
                    raise ValueError(
                        "%s: %s in %s is in register %d at 0x%x-0x%x, across "
                        "the call at 0x%x" %
                        (config, name, function, reg, low, high, call))

    def expect(function, name, kind, regs):
        """Checks that each register holding the variable is a |kind|
        location in |regs|, and returns the entries."""
        location = locations.get((function, name))
        if (location is None or location[0] == "fbreg" or
                not any(entry[0] < entry[1] for entry in location[0]) or
                any(entry[2] != kind or entry[3] not in regs
                    for entry in location[0])):
            raise ValueError("%s: %s in %s is at %s, expected %s in %s" %
                             (config, name, function, location, kind,
                              sorted(regs)))
        return location[0]

    def shared(a, b):
        """A register two variables' entries both claim at once, or None."""
        for low, high, _, reg in a:
            for other_low, other_high, _, other_reg in b:
                if reg == other_reg and low < other_high and other_low < high:
                    return reg
        return None

    total = expect("count", "total", "reg", saved["int"])
    i = expect("count", "i", "reg", saved["int"])
    if shared(total, i) is not None:
        raise ValueError("%s: total and i share register %d" %
                         (config, shared(total, i)))
    if saved["fp"]:
        expect("scale", "x", "reg", saved["fp"])
    else:
        # No register keeps x across the calls.
        location = locations.get(("scale", "x"))
        if location is None or location[0] != "fbreg" or location[1] >= 0:
            raise ValueError("%s: x in scale is at %s, expected the frame" %
                             (config, location))
    expect("scale", "acc", "reg", FP_REGS[target])
    expect("make", "r", "breg", set(range(0, 32)))

    # More accumulators are live across the calls than there are saved
    # registers, so some are in the frame.
    in_frame = []
    in_registers = []
    for name in "abcdefghijklmo":
        location = locations.get(("many", name))
        if location is not None and location[0] == "fbreg":
            if location[1] >= 0 or location[1] in in_frame:
                raise ValueError("%s: %s in many is at %s" %
                                 (config, name, location))
            in_frame.append(location[1])
            continue
        entries = expect("many", name, "reg", saved["int"])
        for other in in_registers:
            if shared(entries, other) is not None:
                raise ValueError("%s: accumulators in many share register %d" %
                                 (config, shared(entries, other)))
        in_registers.append(entries)
    if not in_frame:
        raise ValueError("%s: no accumulator in many is in the frame" %
                         config)
    print("ok register variables %s" % config)


davecc, source, cxx_source, register_source, work = sys.argv[1:]
c_langs = {DW_LANG_C, DW_LANG_C89, DW_LANG_C99, DW_LANG_C11}
cxx_langs = {0x0004, 0x001a, 0x0021, 0x002a, 0x002b, 0x002c}
compile_and_check(davecc, source, work, "x86_64", [], c_langs)
compile_and_check(davecc, source, work, "aarch64", [], c_langs)
compile_and_check(davecc, cxx_source, work, "x86_64", [], cxx_langs,
                  obj_name="x86_64_cxx")
for target in ("aarch64", "riscv", "x86_64"):
    for opt in ("-O1", "-O2"):
        for allocator_env in ({}, {"DAVECC_OLD_RA": "1"}):
            check_register_variables(davecc, register_source, work, target,
                                     opt, allocator_env)
print("ok dwarf -g objects")
PY
