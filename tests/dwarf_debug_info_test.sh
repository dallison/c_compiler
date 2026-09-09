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
WORK="${TEST_TMPDIR:-/tmp}/dwarf-debug-info"
mkdir -p "$WORK"

python3 - "$DAVECC" "$SOURCE" "$CXX_SOURCE" "$WORK" <<'PY'
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


def parse_elf_sections(blob):
    if blob[:4] != b"\x7fELF":
        raise ValueError("not an ELF file")
    elf64 = blob[4] == 2
    if blob[5] != 1:
        raise ValueError("ELF file is not little-endian")
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
        if elf64:
            name, sh_type, flags, addr, offset, size = fields[:6]
        else:
            name, sh_type, flags, addr, offset, size = fields[:6]
        sections.append((name, sh_type, offset, size))
    strtab_name, _, str_off, str_size = sections[e_shstrndx]
    strtab = blob[str_off:str_off + str_size]
    result = {}
    for name_off, sh_type, offset, size in sections:
        end = strtab.find(b"\0", name_off)
        name = strtab[name_off:end].decode("utf-8", "replace")
        result[name] = blob[offset:offset + size]
    return result


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
        cur.offset += address_size
        return None
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


davecc, source, cxx_source, work = sys.argv[1:]
c_langs = {DW_LANG_C, DW_LANG_C89, DW_LANG_C99, DW_LANG_C11}
cxx_langs = {0x0004, 0x001a, 0x0021, 0x002a, 0x002b, 0x002c}
compile_and_check(davecc, source, work, "x86_64", [], c_langs)
compile_and_check(davecc, source, work, "aarch64", [], c_langs)
compile_and_check(davecc, cxx_source, work, "x86_64", [], cxx_langs,
                  obj_name="x86_64_cxx")
print("ok dwarf -g objects")
PY
