#!/usr/bin/env python3
"""png2pic.py - imagen (png/jpg/webp/avif...) -> entrada #AZT de imagen de escena (como data_cmn 1388).

Formato medido en las nativas (1388, 1396, 1411):
  - una textura BGRA sin comprimir de 1024x512 (DDS de 128 B en 0x60), el arte arriba a la izquierda
  - contenido (cw,ch) en +0x2C: 660x420 (1388), 900x420 (1396)
  - tamano en pantalla (dw,dh) en +0x34: contenido / 1.5 -> 440x280 (1388), 600x280 (1396)
  - bits en +0x30 = bit_length(dw-1), bit_length(dh-1)
  - entrada en 0x24 (48 B), indice en 0x20, cabecera #AZT con n=1, base=0 y crc en 0x1C

  python png2pic.py imagen.webp salida.bin [--caja 660x420] [--ajustar]
  python png2pic.py --test     ida y vuelta sobre la 1388 nativa (necesita el AFS del juego)

--caja: contenido en px (por defecto 660x420, como 1388; maximo 1024x512). Por defecto la imagen
llena la caja y se recorta por el centro; --ajustar la encaja con bandas negras.
"""
import io
import struct
import sys
import zlib

import numpy as np
from PIL import Image

TEX_W, TEX_H = 1024, 512
DDS_HEAD = (b"DDS " + struct.pack("<I", 124) + struct.pack("<I", 0x1007)
            + struct.pack("<IIII", TEX_H, TEX_W, TEX_W * 4, 0) + struct.pack("<I", 0) + bytes(44)
            + struct.pack("<II4s", 32, 0x41, bytes(4)) + struct.pack("<IIIII", 32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000)
            + struct.pack("<I", 0x1000) + bytes(16))
assert len(DDS_HEAD) == 128


def fit(img, caja, ajustar=False):
    """RGBA de tamano `caja` (w, h). Sin transparencia (fondo negro opaco)."""
    img = Image.alpha_composite(Image.new("RGBA", img.size, (0, 0, 0, 255)), img.convert("RGBA"))
    w, h = caja
    k = min(w / img.width, h / img.height) if ajustar else max(w / img.width, h / img.height)
    im = img.resize((max(1, round(img.width * k)), max(1, round(img.height * k))), Image.LANCZOS)
    if ajustar:
        canvas = Image.new("RGBA", (w, h), (0, 0, 0, 255))
        canvas.paste(im, ((w - im.width) // 2, (h - im.height) // 2))
    else:
        x, y = (im.width - w) // 2, (im.height - h) // 2
        canvas = im.crop((x, y, x + w, y + h))
    return np.array(canvas)


def build_picture(src, caja=(660, 420), ajustar=False):
    """Bytes del #AZT (2 MB) con la imagen `src` (PIL o array HxWx4 ya a la caja) en la caja dada."""
    cw, ch = caja
    assert cw <= TEX_W and ch <= TEX_H and cw % 2 == 0 and ch % 2 == 0, "caja: maximo 1024x512, pares"
    if isinstance(src, np.ndarray) and src.shape[:2] == (ch, cw):
        art = src
    else:
        art = fit(src if isinstance(src, Image.Image) else Image.fromarray(src), caja, ajustar)
    dw, dh = round(cw / 1.5), round(ch / 1.5)          # en pantalla 1.5x menos, como las nativas
    tex = np.zeros((TEX_H, TEX_W, 4), np.uint8)
    tex[:, :, 3] = 255
    tex[:ch, :cw] = art
    blob = DDS_HEAD + np.ascontiguousarray(tex[..., [2, 1, 0, 3]]).tobytes()   # BGRA, como el DDS nativo
    ent = struct.pack(">12I", 0, 0x21, cw << 16 | ch, (dw - 1).bit_length() << 16 | (dh - 1).bit_length(),
                      dw << 16 | dh, 0x60, len(blob), 1, 0, 0x1E1C0, 0x400, 0)
    hdr = (b"#AZT" + bytes(12) + struct.pack(">IIII", 1, 0x20, 0, zlib.crc32(blob) & 0xFFFFFFFF)
           + struct.pack(">I", 0x24) + ent)
    return hdr + bytes(0x60 - len(hdr)) + blob


def _test():
    import tempfile
    sys.path.insert(0, __import__("os").path.dirname(__import__("os").path.abspath(__file__)))
    import campana
    import capsulas
    import roster_build as rb
    native = campana.read_entry(rb.default_us(), "data_cmn.afs", 1388, tempfile.mkdtemp())
    nat_tex = np.array(Image.open(io.BytesIO(native[0x60:])).convert("RGBA"))        # 1024x512
    ours = build_picture(nat_tex[:420, :660])
    print("tamano: nuevo", len(ours), "nativo", len(native), "| DDS 128 B iguales:", ours[0x60:0xE0] == native[0x60:0xE0])
    print("cabecera 0x00-0x60, bytes distintos:", [i for i in range(0x60) if ours[i] != native[i]], "(0x1C = crc)")
    print("entrada 0x24-0x54 nativa:", native[0x24:0x54].hex(" "))
    print("entrada 0x24-0x54 nueva :", ours[0x24:0x54].hex(" "))
    our_tex = np.array(Image.open(io.BytesIO(ours[0x60:])).convert("RGBA"))
    print("pixeles distintos de 1024x512:", int(np.sum(np.any(our_tex[:420, :660] != nat_tex[:420, :660], axis=2))),
          "(en 660x420) y", int(np.sum(np.any(our_tex != nat_tex, axis=2))), "(en toda la textura)")
    print("lector del proyecto (azt_read, recorta a la pantalla 440x280):", capsulas.azt_read(ours, 0).shape)
    assert ours[0x60:0xE0] == native[0x60:0xE0] and our_tex[:420, :660].tobytes() == nat_tex[:420, :660].tobytes()
    print("OK")


if __name__ == "__main__":
    if sys.argv[1:] == ["--test"]:
        _test()
        sys.exit(0)
    import argparse
    p = argparse.ArgumentParser()
    p.add_argument("entrada")
    p.add_argument("salida")
    p.add_argument("--caja", default="660x420")
    p.add_argument("--ajustar", action="store_true")
    a = p.parse_args()
    caja = tuple(int(v) for v in a.caja.lower().split("x"))
    data = build_picture(Image.open(a.entrada), caja, a.ajustar)
    open(a.salida, "wb").write(data)
    print("escrito", a.salida, len(data), "bytes")
