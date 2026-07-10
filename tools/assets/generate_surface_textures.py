#!/usr/bin/env python3
"""Generate deterministic prototype PBR texture sets without authoring dependencies."""

from __future__ import annotations

import argparse
import json
import math
import random
import struct
import zlib
from pathlib import Path


SIZE = 128


def clamp_byte(value: float) -> int:
    return max(0, min(255, round(value)))


def write_png(path: Path, pixels: list[tuple[int, int, int, int]], size: int = SIZE) -> None:
    if len(pixels) != size * size:
        raise ValueError(f"{path}: expected {size * size} pixels, got {len(pixels)}")

    def chunk(kind: bytes, payload: bytes) -> bytes:
        return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload))

    rows = bytearray()
    for y in range(size):
        rows.append(0)
        for pixel in pixels[y * size : (y + 1) * size]:
            rows.extend(pixel)
    header = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(rows, 9)) + chunk(b"IEND", b""))


def noise(seed: int, x: int, y: int) -> float:
    rng = random.Random(seed ^ (x * 73856093) ^ (y * 19349663))
    return rng.random() * 2.0 - 1.0


def polymer_base() -> list[tuple[int, int, int, int]]:
    result = []
    for y in range(SIZE):
        for x in range(SIZE):
            grain = noise(11, x // 2, y // 2) * 5.5
            weave = math.sin(x * 0.72) * math.sin(y * 0.61) * 2.2
            scratch = 10.0 if ((x * 17 + y * 31) % 509) < 2 else 0.0
            result.append((clamp_byte(20 + grain + scratch), clamp_byte(23 + grain + weave + scratch), clamp_byte(26 + grain + scratch), 255))
    return result


def polymer_normal() -> list[tuple[int, int, int, int]]:
    result = []
    for y in range(SIZE):
        for x in range(SIZE):
            nx = noise(19, x, y) * 7.0 + math.sin(y * 0.42) * 2.5
            ny = noise(23, x, y) * 7.0 + math.cos(x * 0.37) * 2.5
            result.append((clamp_byte(128 + nx), clamp_byte(128 + ny), 252, 255))
    return result


def polymer_mr() -> list[tuple[int, int, int, int]]:
    return [(0, clamp_byte(148 + noise(29, x, y) * 16), 112, 255) for y in range(SIZE) for x in range(SIZE)]


def panel_base() -> list[tuple[int, int, int, int]]:
    result = []
    for y in range(SIZE):
        for x in range(SIZE):
            seam = x % 32 in (0, 1) or y % 32 in (0, 1)
            bevel = x % 32 in (2, 31) or y % 32 in (2, 31)
            wear = noise(41, x // 3, y // 3) * 8.0
            base = (14, 68, 80) if not seam else (4, 14, 18)
            lift = 12 if bevel else 0
            result.append(tuple(clamp_byte(c + wear + lift) for c in base) + (255,))
    return result


def panel_normal() -> list[tuple[int, int, int, int]]:
    result = []
    for y in range(SIZE):
        for x in range(SIZE):
            local_x, local_y = x % 32, y % 32
            nx = -22 if local_x == 1 else 22 if local_x == 31 else noise(43, x, y) * 2.0
            ny = -22 if local_y == 1 else 22 if local_y == 31 else noise(47, x, y) * 2.0
            result.append((clamp_byte(128 + nx), clamp_byte(128 + ny), 250, 255))
    return result


def panel_mr() -> list[tuple[int, int, int, int]]:
    result = []
    for y in range(SIZE):
        for x in range(SIZE):
            seam = x % 32 < 2 or y % 32 < 2
            result.append((0, 182 if seam else clamp_byte(116 + noise(53, x, y) * 12), 54 if seam else 76, 255))
    return result


def panel_emissive() -> list[tuple[int, int, int, int]]:
    result = []
    for y in range(SIZE):
        for x in range(SIZE):
            strip = y % 32 in (3, 4)
            pulse = 0.72 + math.sin(x * 0.18) * 0.18
            result.append((4, clamp_byte(184 * pulse) if strip else 2, clamp_byte(238 * pulse) if strip else 3, 255))
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    args = parser.parse_args()
    root = args.root.resolve()

    outputs = {
        "textures/weapons/wpn_polymer_dark_basecolor.png": polymer_base(),
        "textures/weapons/wpn_polymer_dark_normal.png": polymer_normal(),
        "textures/weapons/wpn_polymer_dark_mr.png": polymer_mr(),
        "textures/environments/movement_accent_basecolor.png": panel_base(),
        "textures/environments/movement_accent_normal.png": panel_normal(),
        "textures/environments/movement_accent_mr.png": panel_mr(),
        "textures/environments/movement_accent_emissive.png": panel_emissive(),
    }
    for relative, pixels in outputs.items():
        write_png(root / relative, pixels)

    manifest = {
        "version": 1,
        "generator": "tools/assets/generate_surface_textures.py",
        "resolution": [SIZE, SIZE],
        "color_space": {"base_color": "sRGB", "normal": "linear", "metallic_roughness": "linear", "emissive": "sRGB"},
        "textures": sorted(outputs),
    }
    manifest_path = root / "textures" / "prototype_surface_textures.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="ascii")
    print(f"Generated {len(outputs)} textures at {root / 'textures'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
