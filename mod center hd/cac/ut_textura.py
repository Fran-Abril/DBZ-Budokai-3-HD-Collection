#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ut_textura.py - Texturas TX2D del creador de Ultimate Tenkaichi (X360) -> PNG.

El `.vram` guarda las texturas "tiled" (swizzle de Xbox 360); el `.spr` describe
cada TX2D (offset en el .vram, tamano, encoding). Aqui se des-swizzlea (algoritmo
publico XGAddress2DTiledOffset) y se vuelca a PNG para poder usarlas en el
conversor a B3 (`ut_to_b3.py --tex`).

Uso:
    python ut_textura.py lista <pieza.spr[.zpak]>
    python ut_textura.py tex   <pieza.spr[.zpak]> <pieza.vram> <nombre-o-indice> <salida.png>
    python ut_textura.py base  <pieza.spr[.zpak]> <pieza.vram> <salida.png>   # mayor DXT1/5 "color"
"""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.append(HERE)
import ut_formato  # noqa: E402

import numpy as np                      # noqa: E402
from PIL import Image                   # noqa: E402


def _xg_2d(axis, offset, width, texel_pitch):
    aw = (width + 31) & ~31
    log_bpp = (texel_pitch >> 2) + ((texel_pitch >> 1) >> (texel_pitch >> 2))
    ob = offset << log_bpp
    ot = ((ob & ~4095) >> 3) + ((ob & 1792) >> 2) + (ob & 63)
    om = ot >> (7 + log_bpp)
    if axis == "x":
        mx = (om % (aw >> 5)) << 2
        tile = ((((ot >> (5 + log_bpp)) & 2) + (ob >> 6)) & 3)
        macro = (mx + tile) << 3
        micro = ((((ot >> 1) & ~15) + (ot & 15)) & ((texel_pitch << 3) - 1)) >> log_bpp
        return macro + micro
    my = (int(om / (aw >> 5)) << 2)
    tile = ((ot >> (6 + log_bpp)) & 1) + ((ob & 2048) >> 10)
    macro = (my + tile) << 3
    micro = ((((ot & (((texel_pitch << 6) - 1) & ~31)) + ((ot & 15) << 1)) >> (3 + log_bpp)) & ~1)
    return macro + micro + ((ot & 16) >> 4)


def _deswizzle_dxt(data, width, height, texel_pitch):
    bw, bh = width // 4, height // 4
    out = bytearray(len(data))
    for j in range(bh):
        for i in range(bw):
            bo = j * bw + i
            x = _xg_2d("x", bo, bw, texel_pitch)
            y = _xg_2d("y", bo, bw, texel_pitch)
            src = y * bw * texel_pitch + x * texel_pitch
            dst = j * bw * texel_pitch + i * texel_pitch
            if src + texel_pitch <= len(data) and dst + texel_pitch <= len(out):
                blk = bytearray(data[src:src + texel_pitch])
                for k in range(0, texel_pitch, 2):
                    blk[k], blk[k + 1] = blk[k + 1], blk[k]
                out[dst:dst + texel_pitch] = blk
    return bytes(out)


def _dds(w, h, enc, data_len):
    fourcc = {"DXT1": b"DXT1", "DXT5": b"DXT5", "ATI2": b"ATI2"}[enc]
    flags = 0x81007
    hdr = (b"DDS " + struct.pack("<I", 124) + struct.pack("<I", flags) + struct.pack("<II", h, w)
           + struct.pack("<I", data_len) + struct.pack("<I", 0) + struct.pack("<I", 1) + b"\x00" * 44
           + struct.pack("<I", 32) + struct.pack("<I", 4) + fourcc + struct.pack("<IIIII", 0, 0, 0, 0, 0)
           + struct.pack("<IIIII", 0x1000, 0, 0, 0, 0))
    return hdr


def extraer(spr, vram, nombre):
    """Devuelve PIL.Image de la textura `nombre` (o indice) del .spr, o None."""
    tx = ut_formato.texturas(spr)
    if isinstance(nombre, int):
        t = tx[nombre]
    else:
        t = next((x for x in tx if x["nombre"] == nombre), None)
        if t is None:
            t = next((x for x in tx if nombre in x["nombre"]), None)
    if t is None or t["enc"] not in ("DXT1", "DXT5", "ATI2"):
        return None
    tp = 8 if t["enc"] == "DXT1" else 16
    mip0 = t["w"] * t["h"] // (2 if t["enc"] == "DXT1" else 1)
    crudo = vram[t["data_off"]:t["data_off"] + t["size"]][:mip0]
    datos = _deswizzle_dxt(crudo, t["w"], t["h"], tp)
    import io
    return Image.open(io.BytesIO(_dds(t["w"], t["h"], t["enc"], len(datos)) + datos)).convert("RGBA")


def textura_base(spr, vram):
    """La textura de COLOR: la DXT1/5 mas grande (excluye normal ATI2 y DXT5 de sombra
    por el nombre)."""
    cand = [t for t in ut_formato.texturas(spr)
            if t["enc"] in ("DXT1", "DXT5") and "shadow" not in t["nombre"].lower()
            and "_n" not in t["nombre"].lower() and "dummy" not in t["nombre"].lower()]
    if not cand:
        cand = [t for t in ut_formato.texturas(spr) if t["enc"] in ("DXT1", "DXT5")]
    return max(cand, key=lambda t: t["w"] * t["h"]) if cand else None


def _cli():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    cmd = sys.argv[1]
    spr_path = sys.argv[2]
    spr = ut_formato.cargar_spr(spr_path)
    s = ut_formato.Spr(spr)
    if cmd == "lista":
        for i, t in enumerate(ut_formato.texturas(s)):
            print("%2d %-28s off=0x%X %dx%d %s" % (i, t["nombre"], t["data_off"], t["w"], t["h"], t["enc"]))
        return 0
    vram = open(sys.argv[3], "rb").read()
    if cmd == "base":
        t = textura_base(s, vram)
        img = extraer(s, vram, t["nombre"])
        img.save(sys.argv[4])
        print("base ->", sys.argv[4], t["nombre"], img.size)
        return 0
    if cmd == "tex":
        clave = int(sys.argv[4]) if sys.argv[4].isdigit() else sys.argv[4]
        img = extraer(s, vram, clave)
        if img is None:
            print("[X] textura no encontrada o no soportada")
            return 2
        img.save(sys.argv[5])
        print("->", sys.argv[5], img.size)
        return 0
    print(__doc__)
    return 1


if __name__ == "__main__":
    raise SystemExit(_cli())
