#!/usr/bin/env python3
"""Titulos de las pantallas propias (TAG TEAM, ONLINE): res/ingame_ui/titulo_*.png.
Son nuestros (letra de Windows imitando la de los menus: redondeada, amarilla arriba y naranja
abajo, borde marron, letras que bailan); la compilacion los copia junto a dbz3.exe y make_release
los mete en la version. Las piezas del juego (marco, bocadillo, botones, fondos, anfitriones,
caras) NO van aqui: el juego las saca de los AFS del jugador al arrancar (src/ingame/native_ui.cpp),
asi no se publica nada del juego.
Opcional y solo para uso personal (no se publica): --anfitriones-locales crea en mods/ un mod
desactivado con los dibujos de Dragon Universe de la carpeta de material del usuario como
anfitriones (Goten y Trunks en TAG TEAM, Bulma en ONLINE).
Uso: python tools/ingame_ui.py [--anfitriones-locales [--material <carpeta>] [--mods <carpeta>]]
     python tools/ingame_ui.py --prueba
"""
import argparse
import glob
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "res", "ingame_ui")
LOCAL_MOD = "anfitriones_du_personal"
MATERIAL = r"E:\Videos\Efectos Canal\MATERIAL DE EDICIÓN\DRAGON BALL Z (Auras, Sprites, Paneles...)"

TITLES = {"titulo_tagteam": "Tag Team", "titulo_online": "Online"}
HOSTS = {"anfitrion_tagteam_a": "Dragon Ball Z/Great Saiyaman Saga/*Goten render [[]Budokai 3*",
         "anfitrion_tagteam_b": "Dragon Ball Z/Great Saiyaman Saga/*Kid trunks render [[]Budokai 3*",
         "anfitrion_online": "Dragon Ball Z/Great Saiyaman Saga/*Bulma - Great Saiyaman Saga render [[]Budokai 3*"}
# Medido en la textura "Dueling" (data_usi 2055): borde, relleno de arriba y de abajo.
EDGE, TOP, BOTTOM = (56, 41, 23), (246, 205, 60), (190, 82, 23)
DY = (-8, 7, -3, 8, -6, 4, -5, 7)      # % del cuerpo de letra
ROT = (-8, 6, -4, 9, -6, 5, -7, 4)     # grados


def title(text, size=112):
    """RGBA del titulo; size = cuerpo de letra en px (Arial Rounded, la mas parecida a la del juego)."""
    from PIL import Image, ImageDraw, ImageFilter, ImageFont
    font = ImageFont.truetype(os.path.join(os.environ.get("WINDIR", r"C:\Windows"), "Fonts", "ARLRDBD.TTF"), size)
    pad = size // 2
    glyphs, x, i = [], pad, 0
    for ch in text:
        if ch == " ":
            x += size * 0.32
            continue
        x0, y0, x1, y1 = font.getbbox(ch)
        g = Image.new("L", (x1 - x0 + 8, y1 - y0 + 8))
        ImageDraw.Draw(g).text((4 - x0, 4 - y0), ch, font=font, fill=255)
        r = g.rotate(ROT[i % len(ROT)], resample=Image.BICUBIC, expand=True)
        glyphs.append((r, int(x - (r.width - g.width) / 2),
                       int(pad + y0 + DY[i % len(DY)] * size / 100 - (r.height - g.height) / 2)))
        x += (x1 - x0) * 0.97
        i += 1
    w, h = int(x + pad), int(size * 1.25 + 2 * pad)
    mask, fill = Image.new("L", (w, h)), Image.new("RGB", (w, h))
    for g, gx, gy in glyphs:
        mask.paste(255, (gx, gy), g)
        grad = Image.new("RGB", g.size)   # por letra: amarillo hasta la mitad, naranja abajo
        d = ImageDraw.Draw(grad)
        for yy in range(g.height):
            t = min(1.0, max(0.0, (yy / max(1, g.height - 1) - 0.42) / 0.2))
            d.line([(0, yy), (g.width, yy)], fill=tuple(int(a + (b - a) * t) for a, b in zip(TOP, BOTTOM)))
        fill.paste(grad, (gx, gy), g)
    edge = mask.filter(ImageFilter.MaxFilter(2 * (size // 16) + 1))
    shadow = edge.filter(ImageFilter.MaxFilter(5)).filter(ImageFilter.GaussianBlur(size / 18))
    out = Image.new("RGBA", (w, h))
    out.paste((20, 12, 4, 255), (size // 30, size // 22), shadow.point(lambda v: v * 6 // 10))
    out.paste(EDGE + (255,), (0, 0), edge)
    out.paste(fill, (0, 0), mask)
    return out.crop(out.getbbox())


def build():
    os.makedirs(OUT, exist_ok=True)
    for name, text in TITLES.items():
        title(text).save(os.path.join(OUT, name + ".png"))
    print("titulos en %s" % OUT)


def local_hosts(material, mods):
    """Mod local desactivado con los anfitriones de la carpeta de material (uso personal)."""
    from PIL import Image
    d = os.path.join(mods, LOCAL_MOD + ".disabled", "ui_ingame")
    os.makedirs(d, exist_ok=True)
    missing = []
    for name, pat in HOSTS.items():
        f = sorted(glob.glob(os.path.join(material, pat)))
        if not f:
            missing.append(name)
            continue
        im = Image.open(f[0]).convert("RGBA")
        im.crop(im.getbbox()).save(os.path.join(d, name + ".png"))
    with open(os.path.join(mods, LOCAL_MOD + ".disabled", "manifest.txt"), "w", encoding="utf-8") as f:
        f.write("name=Anfitriones de Dragon Universe (uso personal)\n"
                "description=Goten y Trunks en TAG TEAM y Bulma en ONLINE en lugar de los Kaio. Dibujos oficiales "
                "de Budokai 3 de una coleccion externa: solo para tu PC, no lo compartas ni lo publiques.\n"
                "type=visual\n")
    print("mod local (desactivado) en %s%s" % (d, ("; falta: " + ", ".join(missing)) if missing else ""))


def selftest():
    im = title("Ab")
    assert im.mode == "RGBA" and im.width > im.height > 50, im.size
    cols = {im.getpixel((x, y))[:3] for x in range(im.width) for y in range(im.height) if im.getpixel((x, y))[3] == 255}
    assert EDGE in cols and TOP in cols, "faltan el borde o el amarillo"
    print("ok")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--anfitriones-locales", action="store_true")
    ap.add_argument("--material", default=MATERIAL)
    ap.add_argument("--mods", default=os.path.join(ROOT, "out", "build", "win-amd64-release", "mods"))
    ap.add_argument("--prueba", action="store_true")
    a = ap.parse_args()
    if a.prueba:
        return selftest()
    if a.anfitriones_locales:
        return local_hosts(a.material, a.mods)
    build()


if __name__ == "__main__":
    sys.exit(main())
