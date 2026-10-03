"""Generate the Bravelike brand icons (PNG set + Windows .ico).

Run from the repository root:
    python scripts/generate-icons.py

Requires Pillow. Output:
    resources/icons/bravelike-{16,32,48,64,128,256}.png
    src/browser/res/bravelike.ico
"""
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parent.parent
ICON_DIR = ROOT / "resources" / "icons"
ICO_PATH = ROOT / "src" / "browser" / "res" / "bravelike.ico"
SIZE = 1024

# Brand palette: deep ink + "signal" mint-to-cyan.
MINT = (61, 245, 196)
CYAN = (35, 170, 255)
INK = (10, 14, 22)


def shield_points(s: int) -> list[tuple[float, float]]:
    """A shield outline built from a flat top and a curved, pointed base."""
    pts: list[tuple[float, float]] = []
    left, right, top = 0.16 * s, 0.84 * s, 0.10 * s
    tip = (0.5 * s, 0.93 * s)
    # Top edge with a subtle peak in the middle.
    steps = 40
    for i in range(steps + 1):
        t = i / steps
        x = left + (right - left) * t
        pts.append((x, top + 0.03 * s * abs(2 * t - 1)))
    # Right side curving down to the tip (quadratic Bezier).
    p0, p1, p2 = (right, top), (right, 0.66 * s), tip
    for i in range(1, steps + 1):
        t = i / steps
        x = (1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t ** 2 * p2[0]
        y = (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t ** 2 * p2[1]
        pts.append((x, y))
    # Left side back up.
    p0, p1, p2 = tip, (left, 0.66 * s), (left, top)
    for i in range(1, steps + 1):
        t = i / steps
        x = (1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t ** 2 * p2[0]
        y = (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t ** 2 * p2[1]
        pts.append((x, y))
    return pts


def gradient(s: int) -> Image.Image:
    img = Image.new("RGB", (s, s))
    px = img.load()
    for y in range(s):
        for x in range(s):
            t = min(1.0, max(0.0, (x * 0.35 + y * 0.65) / s))
            px[x, y] = tuple(int(MINT[i] + (CYAN[i] - MINT[i]) * t) for i in range(3))
    return img


def build_master() -> Image.Image:
    s = SIZE
    canvas = Image.new("RGBA", (s, s), (0, 0, 0, 0))

    mask = Image.new("L", (s, s), 0)
    ImageDraw.Draw(mask).polygon(shield_points(s), fill=255)

    # Soft outer glow.
    glow = mask.filter(ImageFilter.GaussianBlur(s * 0.03))
    glow_layer = Image.new("RGBA", (s, s), CYAN + (0,))
    glow_layer.putalpha(glow.point(lambda v: int(v * 0.45)))
    canvas.alpha_composite(glow_layer)

    shield = gradient(s).convert("RGBA")
    shield.putalpha(mask)
    canvas.alpha_composite(shield)

    # Ink "B" monogram.
    font_path = Path("C:/Windows/Fonts/bahnschrift.ttf")
    if not font_path.exists():
        font_path = Path("C:/Windows/Fonts/segoeuib.ttf")
    font = ImageFont.truetype(str(font_path), int(s * 0.56))
    try:
        font.set_variation_by_name("Bold")
    except Exception:
        pass
    draw = ImageDraw.Draw(canvas)
    bbox = draw.textbbox((0, 0), "B", font=font)
    w, h = bbox[2] - bbox[0], bbox[3] - bbox[1]
    x = (s - w) / 2 - bbox[0]
    y = s * 0.47 - h / 2 - bbox[1]
    draw.text((x, y), "B", font=font, fill=INK + (255,))
    return canvas


def main() -> None:
    ICON_DIR.mkdir(parents=True, exist_ok=True)
    ICO_PATH.parent.mkdir(parents=True, exist_ok=True)
    master = build_master()
    sizes = [16, 32, 48, 64, 128, 256]
    for size in sizes:
        master.resize((size, size), Image.LANCZOS).save(ICON_DIR / f"bravelike-{size}.png")
    master.resize((256, 256), Image.LANCZOS).save(
        ICO_PATH, sizes=[(n, n) for n in sizes])
    print(f"Wrote {len(sizes)} PNGs to {ICON_DIR} and {ICO_PATH}")


if __name__ == "__main__":
    main()
