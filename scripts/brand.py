#!/usr/bin/env python3
"""Trackpad Anything brand assets (KUGNUS LAB) from the designer's PNGs.

  Resources/brand/source/icon.png  -> Resources/AppIcon.icns, Resources/AppIcon.png
  Resources/brand/source/logo.png  -> GUI/public/assets/logo-light.png, logo-dark.png

Icon: keeps only the rounded tile (drops stray pixels around it) and places it on the
macOS icon grid (824 px tile in a 1024 px canvas). Logo: trimmed and recoloured per theme.
Requires Pillow + numpy.
"""
import subprocess, tempfile
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / 'Resources/brand/source'
ASSETS = ROOT / 'GUI/public/assets'


def hex_rgb(h): return np.array([int(h[i:i + 2], 16) for i in (1, 3, 5)], dtype=np.float32)


def clean_icon(img):
    a = np.asarray(img.convert('RGBA')).astype(np.float32)
    alpha = a[..., 3]
    # Walk outward from the centre along the middle row/column until the tile ends (stray pixels sit beyond).
    h, w = alpha.shape; my, mx = h // 2, w // 2
    def run(line, start, step):
        i = start
        while 0 <= i + step < len(line) and line[i + step] > 128: i += step
        return i
    left, right = run(alpha[my], mx, -1), run(alpha[my], mx, 1)
    top, bottom = run(alpha[:, mx], my, -1), run(alpha[:, mx], my, 1)
    # Corner radius: how far down the left edge the tile needs to reach its full width.
    def inset(y):
        xs = np.where(alpha[y, left:right] > 128)[0]
        return xs[0] if len(xs) else right - left
    radius = next(dy for dy in range(1, (bottom - top) // 2) if inset(top + dy) <= 1)
    yy, xx = np.mgrid[0:alpha.shape[0], 0:alpha.shape[1]]
    cx = np.clip(xx, left + radius, right - radius); cy = np.clip(yy, top + radius, bottom - radius)
    inside = np.hypot(xx - cx, yy - cy) <= radius + 3
    a[..., 3] = np.where(inside, alpha, 0)
    tile = Image.fromarray(a.astype(np.uint8), 'RGBA').crop((left - 2, top - 2, right + 3, bottom + 3))
    canvas = Image.new('RGBA', (1024, 1024), (0, 0, 0, 0))
    canvas.paste(tile.resize((824, 824), Image.LANCZOS), (100, 100))
    return canvas, (int(left), int(top), int(right), int(bottom), radius)


def recolor_logo(img, ink, fill):
    a = np.asarray(img.convert('RGBA')).astype(np.float32)
    lum = (a[..., :3] @ np.array([.299, .587, .114], dtype=np.float32)) / 255   # 0 = ink, 1 = fill
    rgb = hex_rgb(ink) * (1 - lum[..., None]) + hex_rgb(fill) * lum[..., None]
    out = np.dstack([rgb, a[..., 3]]).clip(0, 255).astype(np.uint8)
    im = Image.fromarray(out, 'RGBA')
    return im.crop(im.getbbox())


def menubar_mascot(img, height_pt=17):
    """Mascot from the logo as a menu bar template image: ink only (white body -> transparent)."""
    a = np.asarray(img.convert('RGBA')).astype(np.float32)
    lum = (a[..., :3] @ np.array([.299, .587, .114], dtype=np.float32)) / 255
    ink = (1 - lum) * a[..., 3] / 255
    cols = ink.sum(0) > 2
    # The mascot is the first run of inked columns; the wordmark starts after a blank gap.
    start = int(np.argmax(cols)); end = start
    while end < len(cols) and cols[end:end + 6].any(): end += 1
    mask = Image.fromarray((ink[:, start:end] * 255).clip(0, 255).astype(np.uint8), 'L')
    mask = mask.crop(mask.getbbox())
    # Thicken the outline so it matches the battery's ~1.4 pt stroke at menu bar size.
    from PIL import ImageFilter
    grow = max(3, round(mask.height / height_pt * .6)) | 1
    mask = mask.filter(ImageFilter.MaxFilter(grow))
    out = {}
    for scale in (1, 2):
        h = height_pt * scale; w = round(mask.width * h / mask.height)
        m = mask.resize((w, h), Image.LANCZOS)
        im = Image.new('RGBA', m.size, (0, 0, 0, 0)); im.putalpha(m)
        out[scale] = im
    return out


def build_icns(icon):
    with tempfile.TemporaryDirectory() as tmp:
        iconset = Path(tmp) / 'AppIcon.iconset'; iconset.mkdir()
        for s in (16, 32, 128, 256, 512):
            for scale, suffix in ((1, ''), (2, '@2x')):
                icon.resize((s * scale,) * 2, Image.LANCZOS).save(iconset / f'icon_{s}x{s}{suffix}.png')
        subprocess.run(['iconutil', '-c', 'icns', str(iconset), '-o', str(ROOT / 'Resources/AppIcon.icns')], check=True)
    icon.save(ROOT / 'Resources/AppIcon.png')


if __name__ == '__main__':
    icon, box = clean_icon(Image.open(SRC / 'icon.png'))
    build_icns(icon)
    print('icon tile (left, top, right, bottom, radius)', box, '-> AppIcon.icns, AppIcon.png')
    logo = Image.open(SRC / 'logo.png')
    # Original black-and-white logo; dark mode only inverts it so it stays visible.
    recolor_logo(logo, '#000000', '#FFFFFF').save(ASSETS / 'logo-light.png')
    recolor_logo(logo, '#FFFFFF', '#1B1B1B').save(ASSETS / 'logo-dark.png')
    print('logo -> logo-light.png, logo-dark.png')
    mm = menubar_mascot(logo)
    mm[1].save(ROOT / 'Resources/MenuBarMascot.png'); mm[2].save(ROOT / 'Resources/MenuBarMascot@2x.png')
    print('menu bar mascot', mm[1].size, '-> MenuBarMascot.png, @2x')
