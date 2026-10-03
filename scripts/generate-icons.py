"""Generate the KINGFN brand icons (PNG set + Windows .ico).

Run from the repository root:
    python scripts/generate-icons.py

Requires Pillow. Output:
    resources/icons/kingfn-{16,32,48,64,128,256}.png
    src/browser/res/kingfn.ico
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parent.parent
ICON_DIR = ROOT / "resources" / "icons"
ICO_PATH = ROOT / "src" / "browser" / "res" / "kingfn.ico"
SIZE = 1024

# Brand palette: Regal Ink + Electric Mint-to-Cyan gradient
MINT = (61, 245, 196)
CYAN = (35, 170, 255)
GOLD = (255, 215, 0)
INK = (10, 14, 22)


def shield_points(s: int) -> list[tuple[float, float]]:
    """Crown-top shield outline for KINGFN."""
    pts: list[tuple[float, float]] = []
    left, right = 0.14 * s, 0.86 * s
    top_base = 0.16 * s
    peak_y = 0.08 * s
    tip = (0.5 * s, 0.94 * s)

    # Top edge with crown peaks: left point -> valley -> center peak -> valley -> right point
    pts.append((left, top_base))
    pts.append((0.30 * s, top_base + 0.04 * s))
    pts.append((0.50 * s, peak_y))
    pts.append((0.70 * s, top_base + 0.04 * s))
    pts.append((right, top_base))

    # Right side curving down to the tip
    steps = 40
    p0, p1, p2 = (right, top_base), (right, 0.68 * s), tip
    for i in range(1, steps + 1):
        t = i / steps
        x = (1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t ** 2 * p2[0]
        y = (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t ** 2 * p2[1]
        pts.append((x, y))

    # Left side curving back up
    p0, p1, p2 = tip, (left, 0.68 * s), (left, top_base)
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
            t = min(1.0, max(0.0, (x * 0.3 + y * 0.7) / s))
            px[x, y] = tuple(int(MINT[i] + (CYAN[i] - MINT[i]) * t) for i in range(3))
    return img


def build_master() -> Image.Image:
    s = SIZE
    canvas = Image.new("RGBA", (s, s), (0, 0, 0, 0))

    mask = Image.new("L", (s, s), 0)
    ImageDraw.Draw(mask).polygon(shield_points(s), fill=255)

    # Ambient outer glow
    glow = mask.filter(ImageFilter.GaussianBlur(s * 0.035))
    glow_layer = Image.new("RGBA", (s, s), CYAN + (0,))
    glow_layer.putalpha(glow.point(lambda v: int(v * 0.5)))
    canvas.alpha_composite(glow_layer)

    # Main Shield Body
    shield = gradient(s).convert("RGBA")
    shield.putalpha(mask)
    canvas.alpha_composite(shield)

    # Draw Crown Crownlets at the three peaks
    draw = ImageDraw.Draw(canvas)
    peak_pts = [(0.14 * s, 0.16 * s), (0.50 * s, 0.08 * s), (0.86 * s, 0.16 * s)]
    for px, py in peak_pts:
        draw.ellipse(
            [px - 0.022 * s, py - 0.022 * s, px + 0.022 * s, py + 0.022 * s],
            fill=GOLD + (255,),
            outline=INK + (255,),
            width=int(s * 0.006),
        )

    # Regal "K" monogram
    font_path = Path("C:/Windows/Fonts/bahnschrift.ttf")
    if not font_path.exists():
        font_path = Path("C:/Windows/Fonts/segoeuib.ttf")
    font = ImageFont.truetype(str(font_path), int(s * 0.52))
    try:
        font.set_variation_by_name("Bold")
    except Exception:
        pass

    bbox = draw.textbbox((0, 0), "K", font=font)
    w, h = bbox[2] - bbox[0], bbox[3] - bbox[1]
    x = (s - w) / 2 - bbox[0]
    y = s * 0.51 - h / 2 - bbox[1]
    draw.text((x, y), "K", font=font, fill=INK + (255,))
    return canvas


def main() -> None:
    ICON_DIR.mkdir(parents=True, exist_ok=True)
    ICO_PATH.parent.mkdir(parents=True, exist_ok=True)
    master = build_master()
    sizes = [16, 32, 48, 64, 128, 256]
    for size in sizes:
        master.resize((size, size), Image.LANCZOS).save(ICON_DIR / f"kingfn-{size}.png")
    master.resize((256, 256), Image.LANCZOS).save(
        ICO_PATH, sizes=[(n, n) for n in sizes]
    )
    print(f"Wrote {len(sizes)} PNGs to {ICON_DIR} and {ICO_PATH}")


if __name__ == "__main__":
    main()
