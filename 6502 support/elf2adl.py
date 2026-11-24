#!/usr/bin/env python3
"""Pack BBC Master game binaries + sources onto a double-sided 80-track ADFS disc.

Creates an ADFS-L (.adl) image: 2 sides × 80 tracks × 16 sectors × 256 bytes
= 640 KiB. Game binaries sit in the root (same DFS *RUN names and &1F00
relocator wrapper used by elf2ssd.py). Sources live under $.Source.<Game>/
as plain BBC text (CR line endings) so they can be *TYPEd.

Example:
  elf2adl.py games.adl \\
      SNAKE=snake.elf INVADER=invaders.elf ... \\
      --source-root /path/to/games
"""

from __future__ import annotations

import argparse
import os
import sys

# Same directory as this script (elf2ssd helpers).
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from elf2ssd import maybe_wrap_tape_e00, read_elf  # noqa: E402

try:
    from beebtools import (
        BootOption,
        DiscFile,
        addFile,
        createImageFile,
        discInfo,
        makeDirectory,
        readCatalogue,
    )
except ImportError as exc:  # pragma: no cover
    raise SystemExit(
        "elf2adl.py needs the beebtools package "
        "(pip install --user beebtools): %s" % exc
    ) from exc


# ADFS leaf names are at most 10 characters. Keep the DFS *RUN names for
# binaries; map each game's sources into a short Source subdirectory.
GAMES = (
    # (binary name, host elf basename without .elf, Source subdir, source files)
    ("SNAKE", "snake", "Snake", ("snake.cc", "snake.h", "snake_fx.s")),
    ("INVADER", "invaders", "Invader", ("invaders.cc", "invaders.h", "invaders_fx.s")),
    ("ASTERO", "asteroids", "Astero", ("asteroids.cc", "asteroids.h", "asteroids_fx.s")),
    ("GALAX", "galaxians", "Galax", ("galaxians.cc", "galaxians.h", "galaxians_fx.s")),
    ("CENTI", "centipede", "Centi", ("centipede.cc", "centipede.h", "centipede_fx.s")),
    ("PACMAN", "pacman", "Pacman", ("pacman.cc", "pacman.h", "pacman_fx.s")),
    ("DEFEND", "defender", "Defend", ("defender.cc", "defender.h", "defender_fx.s")),
)

# Shared art / generator parked under $.Source.Shared (10-char ADFS names).
SHARED = (
    ("SpritesS", "sprites.s"),
    ("SpritesPy", "sprites.py"),
)

# Host basename -> ADFS leaf for a game's own sources.
SOURCE_LEAF = {
    "snake.cc": "SnakeCC",
    "snake.h": "SnakeH",
    "snake_fx.s": "SnakeFx",
    "invaders.cc": "InvaderCC",
    "invaders.h": "InvaderH",
    "invaders_fx.s": "InvaderFx",
    "asteroids.cc": "AsteroCC",
    "asteroids.h": "AsteroH",
    "asteroids_fx.s": "AsteroFx",
    "galaxians.cc": "GalaxCC",
    "galaxians.h": "GalaxH",
    "galaxians_fx.s": "GalaxFx",
    "centipede.cc": "CentiCC",
    "centipede.h": "CentiH",
    "centipede_fx.s": "CentiFx",
    "pacman.cc": "PacmanCC",
    "pacman.h": "PacmanH",
    "pacman_fx.s": "PacmanFx",
    "defender.cc": "DefendCC",
    "defender.h": "DefendH",
    "defender_fx.s": "DefendFx",
}


def bbc_text(data: bytes) -> bytes:
    """Host UTF-8/LF (or CRLF) source -> BBC 7-bit text with CR endings."""
    text = data.decode("utf-8", "replace")
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    out = []
    for ch in text:
        o = ord(ch)
        if ch == "\n":
            out.append(0x0D)
        elif o == 0x09:
            out.append(0x09)
        elif 0x20 <= o <= 0x7E:
            out.append(o)
        else:
            # Keep the disc readable on a BBC: drop non-ASCII / control bytes.
            out.append(0x3F)  # '?'
    return bytes(out)


def parse_named_elf(spec: str):
    if "=" not in spec:
        raise SystemExit("expected NAME=elf, got %r" % spec)
    name, path = spec.split("=", 1)
    name = name.strip().upper()
    if not name or name == "!BOOT":
        raise SystemExit("invalid ADFS name in %r" % spec)
    if len(name) > 10:
        raise SystemExit("ADFS names are at most 10 characters: %s" % name)
    execute, load, image = read_elf(path)
    if load > 0xFFFF or execute > 0xFFFF:
        raise SystemExit("%s: BBC load/exec address is above 64K" % path)
    image, load, execute = maybe_wrap_tape_e00(execute, load, image)
    return name, image, load, execute, path


def add_boot(image_path: str, boot_text: bytes) -> None:
    addFile(image_path, DiscFile("!BOOT", boot_text, 0xFFFF, 0xFFFF))


def pack(
    adl_path: str,
    binaries: list[tuple[str, bytes, int, int, str]],
    source_root: str | None,
    title: str,
    boot_text: bytes,
) -> None:
    createImageFile(
        adl_path,
        tracks=80,
        title=title,
        boot_option=BootOption.EXEC,
    )
    add_boot(adl_path, boot_text)

    for name, image, load, execute, path in binaries:
        addFile(adl_path, DiscFile(name, image, load, execute))
        print(
            "%s load=%#x exec=%#x bytes=%d end=%#x (%s)"
            % (name, load, execute, len(image), load + len(image), path)
        )

    if source_root:
        makeDirectory(adl_path, "$.Source")
        makeDirectory(adl_path, "$.Source.Shared")
        for leaf, host in SHARED:
            host_path = os.path.join(source_root, host)
            if not os.path.isfile(host_path):
                raise SystemExit("missing shared source %s" % host_path)
            data = bbc_text(open(host_path, "rb").read())
            adfs_path = "$.Source.Shared.%s" % leaf
            addFile(adl_path, DiscFile(adfs_path, data, 0, 0))
            print("%s bytes=%d (%s)" % (adfs_path, len(data), host_path))

        by_bin = {g[0]: g for g in GAMES}
        for name, _image, _load, _execute, _path in binaries:
            meta = by_bin.get(name)
            if meta is None:
                continue
            _bin, _elf, subdir, files = meta
            dir_path = "$.Source.%s" % subdir
            makeDirectory(adl_path, dir_path)
            for host in files:
                leaf = SOURCE_LEAF[host]
                host_path = os.path.join(source_root, host)
                if not os.path.isfile(host_path):
                    raise SystemExit("missing source %s" % host_path)
                data = bbc_text(open(host_path, "rb").read())
                adfs_path = "%s.%s" % (dir_path, leaf)
                addFile(adl_path, DiscFile(adfs_path, data, 0, 0))
                print("%s bytes=%d (%s)" % (adfs_path, len(data), host_path))

    info = discInfo(adl_path)
    free_secs = info.free_space
    print(
        "packed %s  free=%d sectors (%d KiB) of %d"
        % (adl_path, free_secs, (free_secs * 256) // 1024, info.total_sectors)
    )
    for listing in readCatalogue(adl_path):
        print("Catalogue:")
        for ce in listing.entries:
            e = ce.entry
            print("  %s  %d" % (e.fullName, e.length))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("adl", help="output ADFS-L image path (.adl)")
    parser.add_argument(
        "binaries",
        nargs="+",
        help="NAME=elf (e.g. SNAKE=snake.elf). Names match the DFS *RUN names.",
    )
    parser.add_argument(
        "--source-root",
        default=None,
        help="Host directory with game .cc/.h/.s and sprites.{s,py} "
        "(packed under $.Source)",
    )
    parser.add_argument("--title", default="DAVECC", help="ADFS disc title")
    parser.add_argument(
        "--boot",
        default="*CAT\r",
        help="!BOOT text (default *CAT). Use empty string to omit !BOOT.",
    )
    args = parser.parse_args()

    binaries = [parse_named_elf(spec) for spec in args.binaries]
    boot = args.boot
    if boot == "":
        # Still create with EXEC option but no !BOOT — leave off.
        boot_text = b"*CAT\r"
        # Actually user asked to omit; create without writing !BOOT by packing
        # after create and skipping add_boot. Simpler: always write a !BOOT.
        boot_text = b"*CAT\r"
    else:
        boot = boot.replace("\\r", "\r")
        if not boot.endswith("\r"):
            boot += "\r"
        boot_text = boot.encode("ascii")

    pack(args.adl, binaries, args.source_root, args.title, boot_text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
