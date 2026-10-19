#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""pantalla.py - Pantalla de creacion con camara propia y vista en tiempo real.

Frente 02 (studies 6). Reutiliza la base del visor del Kit (`model_render.py`) y la
camara propia (`camara.py`) para:

  * pintar una VISTA PREVIA del modelo/pieza elegida (fondo + personaje),
  * montar la PANTALLA de creacion (marco, filas de opciones, botones) por codigo,
  * generar un MOSAICO de piezas (la "vista en tiempo real" al cambiar de pieza:
    cada celda = un render con la camara de orbita/zoom/giro).

No usa arte del juego: el fondo y los marcos son degradados y rectangulos pintados
por codigo (regla `assets_uso.md`). El resultado va a una carpeta de trabajo FUERA
del repo (`E:\\DBZ3HD_trabajo\\ut\\cac\\`).

Uso:
  python pantalla.py vista   <modelo.bin> <salida.png> [--cam tres_cuartos] [--size 512] [--head]
  python pantalla.py pantalla <modelo.bin> <salida.png> [--cam giro_auto] [--titulo "..."]
  python pantalla.py mosaico <salida.png> <bin1> [<bin2> ...] [--cols 4] [--size 256] [--head]
"""
import argparse
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
for p in (HERE, os.path.join(HERE, "..")):
    if os.path.isdir(p) and p not in sys.path:
        sys.path.append(p)

import numpy as np                       # noqa: E402
from PIL import Image, ImageDraw         # noqa: E402

import camara as cam_mod                 # noqa: E402
import model_render                      # noqa: E402

# Paleta de la pantalla (propia; el creador de UT usa azul/naranja de menu).
AZUL = (18, 34, 74)
AZUL_CLARO = (44, 78, 150)
NARANJA = (240, 150, 40)
BLANCO = (238, 242, 248)
GRIS = (120, 130, 150)


def fondo_degradado(w, h, arriba=(30, 52, 104), abajo=(8, 14, 34)):
    ys = np.linspace(0, 1, h)[:, None, None]
    a = np.array(arriba, float)[None, None, :]
    b = np.array(abajo, float)[None, None, :]
    col = a * (1 - ys) + b * ys
    return np.repeat(col, w, axis=1).astype(np.uint8)


def render_vista(bin_path, size=512, cam=None, head=False, outline=1.6):
    """Render del modelo con la camara propia (RGBA px x px)."""
    m = model_render.Model(bin_path)
    c = cam or cam_mod.preset("tres_cuartos")
    view = np.array(c.view(), float)
    P, _ = m.posed()
    if head:
        lo, hi, _ = model_render.head_info(m)
    else:
        # solo los vertices que se DIBUJAN (los AWG auxiliares ocultos siguen en P pero
        # no cuentan para el encuadre)
        usados = np.unique(m.tris.reshape(-1)) if len(m.tris) else np.arange(len(P))
        lo, hi = P[usados].min(0), P[usados].max(0)
    lo = np.asarray(lo, float)
    hi = np.asarray(hi, float)
    ctr = (lo + hi) / 2
    ancho = max(hi[1] - lo[1], hi[0] - lo[0], 1e-6)
    esc = size * 0.82 / ancho
    return model_render.render(m, (size, size), view, ctr, esc, outline=outline)


def _pegar(base, rgba, xy):
    """Pega un render RGBA (uint8) sobre una imagen PIL (base) en xy con alfa."""
    img = Image.fromarray(rgba)
    base.alpha_composite(img, xy)


def vista(bin_path, out, cam=None, size=512, head=False):
    rgba = render_vista(bin_path, size, cam, head)
    char = Image.fromarray(rgba)
    base = Image.fromarray(fondo_degradado(size, size)).convert("RGBA")
    _pegar(base, rgba, (0, 0))
    base.convert("RGB").save(out)
    return char


def pantalla(bin_path, out, cam=None, size=768, titulo="CREADOR DE PERSONAJE",
             filas=None, seleccion=0):
    """Pantalla completa: fondo, panel del personaje, lista de opciones y botones."""
    c = cam or cam_mod.preset("giro_auto")
    char = render_vista(bin_path, size, c, head=False)
    base = Image.fromarray(fondo_degradado(size, size)).convert("RGBA")
    # marco del visor (rectangulo redondeado de codigo)
    d = ImageDraw.Draw(base)
    d.rounded_rectangle([16, 60, size - 16, size - 90], radius=18, outline=AZUL_CLARO, width=3)
    _pegar(base, char, (16, 60))
    # cabecera
    d.rectangle([0, 0, size, 52], fill=(10, 20, 46))
    d.rectangle([0, 52, size, 56], fill=NARANJA)
    d.text((22, 18), titulo, fill=BLANCO)
    # lista de opciones (izquierda)
    filas = filas or ["Raza", "Cuerpo", "Cara", "Pelo", "Ropa", "Color", "Voz", "Estilo"]
    y = 96
    for i, f in enumerate(filas):
        col = NARANJA if i == seleccion else BLANCO
        d.text((size - 250, y), ("\u25b6 " if i == seleccion else "  ") + f, fill=col)
        y += 30
    # pie: ayuda de camara (orbita con el raton, zoom con la rueda)
    d.rectangle([0, size - 34, size, size], fill=(10, 20, 46))
    d.text((22, size - 26), "Raton: orbitar   Rueda: zoom   Q/E: girar   Enter: elegir",
           fill=GRIS)
    base.convert("RGB").save(out)
    return out


def mosaico(bins, out, cols=4, size=256, head=True):
    """Rejilla de renders (la vista previa "en tiempo real" al cambiar de pieza)."""
    n = len(bins)
    filas = (n + cols - 1) // cols
    W, H = cols * size, filas * size
    base = Image.fromarray(fondo_degradado(W, H, (26, 40, 82), (6, 10, 26))).convert("RGBA")
    for i, b in enumerate(bins):
        cam = cam_mod.preset("retrato" if head else "tres_cuartos")
        cam.actualizar(i * 0.4 / max(1, n) * 3.0)      # giro distinto por celda
        rgba = render_vista(b, size, cam, head=head)
        x, y = (i % cols) * size, (i // cols) * size
        _pegar(base, rgba, (x, y))
    base.convert("RGB").save(out)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("cmd", choices=("vista", "pantalla", "mosaico"))
    ap.add_argument("arg1")
    ap.add_argument("arg2", nargs="+")
    ap.add_argument("--cam", default=None)
    ap.add_argument("--size", type=int, default=512)
    ap.add_argument("--head", action="store_true")
    ap.add_argument("--cols", type=int, default=4)
    ap.add_argument("--titulo", default="CREADOR DE PERSONAJE")
    a = ap.parse_args()
    cam = cam_mod.preset(a.cam) if a.cam else None
    if a.cmd == "vista":
        vista(a.arg1, a.arg2[0], cam, a.size, a.head)
        print("->", a.arg2[0])
    elif a.cmd == "pantalla":
        pantalla(a.arg1, a.arg2[0], cam, a.size, a.titulo)
        print("->", a.arg2[0])
    else:
        mosaico(a.arg2, a.arg1, a.cols, a.size, a.head)
        print("->", a.arg1)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
