#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ut_to_b3.py - Convierte PIEZAS del creador de Ultimate Tenkaichi (X360) al motor
de Budokai 3 HD (bin #AMB), con mapeo de huesos UT -> B3 y skin CPU-side.

El skinning de B3 HD es CPU (AGENTS.md 3.4.10): el bin guarda, por vertice, la
posicion LOCAL del hueso + el indice de hueso (ventana de 44 B). Por eso este
conversor NO lleva pesos a GPU: elige UN hueso B3 por vertice (mapeo de huesos) y
escribe `pos = inv(world_B3[hueso]) * model` mediante la pasarela `b3_gateway.py`
(ventana 44 B + tabla de draws verificada 2026-10-03).

Mapeo de huesos UT -> B3 (dos vias, ver `datos_MAPEO`):
  * por NOMBRE: el nombre-clave del grupo UT (`H_HAIR_00`, `S_FACE_02`,
    `C_DOUGI_UE_03`...) se traduce a la etiqueta B3 por reglas de prefijo/token.
  * por POSICION (fallback): si el nombre no encaja, se toma la etiqueta B3 cuyo
    hueso bind esta mas cerca del vertice (model-space de la plantilla B3).

Uso:
  python ut_to_b3.py --spr <pieza.spr[.zpak]> --ioram <pieza.ioram> \\
      [--tpl <hd.bin>] --out <pieza_b3.bin> [--solo PATRON] [--excluir PATRON] \\
      [--tex base.png] [--modo multi|atlas] [--escala E] [--yaw G] [--eje y|z]
      [--report r.json]

`--solo`/`--excluir` filtran los grupos por el nombre-clave (substring, separados
por comas). Ej.: la cabeza = `--solo S_FACE,S_EAR,A_EYE,A_EYEBROW,T_TOOTH,`
`T_TONGUE,E_EYE_SHADOW,S_SWEAT,S_SKIN,S_PURPLE_HEAD` (la cara/ojos/boca).
"""
import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
for p in (HERE, os.path.join(HERE, "..", "..", "awo_tools")):
    if os.path.isdir(p) and p not in sys.path:
        sys.path.append(p)

import ut_formato                       # noqa: E402
import b3_gateway                       # noqa: E402
from awg_vertex_buffer import AwgVertexBuffer  # noqa: E402


# ---------------------------------------------------------------------------
# Mapeo de huesos UT -> B3
# ---------------------------------------------------------------------------
# Reglas por PREFIJO del nombre-clave UT (el que va tras el ultimo ':' del grupo).
# A la derecha, el SUFIJO de la etiqueta B3 (se resuelve contra la plantilla).
PREFIJO_B3 = [
    ("H_HAIR", "HEAD"), ("H_EYEBROW", "HEAD_EB"), ("H_MAYU", "HEAD_EB"),
    ("S_FACE", "HEAD"), ("S_EAR", "HEAD"), ("S_HEAD", "HEAD"), ("S_SKIN", "HEAD_NECK"),
    ("S_PURPLE_HEAD", "HEAD"), ("S_SWEAT", "HEAD"), ("E_EYE", "HEAD"), ("A_EYE", "HEAD"),
    ("A_EYEBROW", "HEAD"), ("T_TOOTH", "HEAD"), ("T_TONGUE", "HEAD"), ("T_TEETH", "HEAD"),
    ("G_SCOUTER", "HEAD"), ("A_ACCE", "HEAD"), ("A_METAL", "HEAD"),
    ("S_NECK", "NECK"),
    ("C_MANTO", "CHEST"), ("C_DOUGI_UE", "CHEST"), ("C_DOUGI_YABURE", "CHEST"),
    ("C_BODY", "CHEST"), ("D_CLOTH_IN", "CHEST"), ("LOGO", "CHEST"), ("panel", "CHEST"),
    ("E_OBI", "WAIST"), ("C_DOUGI_SHITA", "WAIST"), ("D_CLOTH", "WAIST"),
    ("S_SKIN_02", "WAIST"),
    ("A_TAIL", "WAIST"),
    ("F_BOOTS", "FOOT"), ("D_CLOTH_ASHI", "FOOT"), ("S_LEG", "LEG2"),
    ("C_ARM", "ARM2"), ("S_ARM", "ARM2"), ("S_HAND", "HAND"),
    ("aura", "BODY"),
]
# Tokens (en cualquier parte) -> sufijo B3; se prueban si no encaja el prefijo.
TOKEN_B3 = [("HEAD", "HEAD"), ("HAIR", "HEAD"), ("FACE", "HEAD"), ("EAR", "HEAD"),
            ("EYE", "HEAD"), ("TOOTH", "HEAD"), ("TONGUE", "HEAD"), ("MOUTH", "HEAD"),
            ("NECK", "NECK"), ("CHEST", "CHEST"), ("BODY", "CHEST"), ("DOUGI", "CHEST"),
            ("OBI", "WAIST"), ("WAIST", "WAIST"), ("CLOTH", "CHEST"),
            ("BOOT", "FOOT"), ("FOOT", "FOOT"), ("LEG", "LEG2"), ("ARM", "ARM2"),
            ("HAND", "HAND"), ("TAIL", "WAIST"), ("WING", "CHEST")]


def _sufijo_de_label(label):
    s = label.lstrip("X")
    return s.split("_", 1)[1] if "_" in s else s


def suffix_index(labels):
    """{sufijo_exacto: label} y {sufijo_sin_trailing_numero: label}."""
    exacto, base = {}, {}
    for l in labels:
        suf = _sufijo_de_label(l)
        exacto.setdefault(suf, l)
        clave = suf.rstrip("0123456789_")
        base.setdefault(clave, l)
    return exacto, base


def resolver_por_sufijo(labels, sufijo):
    """Etiqueta B3 cuyo sufijo casa con `sufijo` (exacto, base, o token)."""
    exacto, base = suffix_index(labels)
    if sufijo in exacto:
        return exacto[sufijo]
    if sufijo in base:
        return base[sufijo]
    # 'HEAD_NECK', 'ARM2', 'FOOT', 'HAND'...: buscar por token
    for tok, suf2 in dict(TOKEN_B3).items():
        if tok in sufijo:
            if suf2 in base:
                return base[suf2]
            if suf2 in exacto:
                return exacto[suf2]
    for l in labels:
        if sufijo.lower() in _sufijo_de_label(l).lower():
            return l
    return None


def hueso_por_nombre(labels, clave):
    """Nombre-clave del grupo UT -> etiqueta B3 (o None)."""
    for pref, suf in PREFIJO_B3:
        if clave.startswith(pref):
            lab = resolver_por_sufijo(labels, suf)
            if lab:
                return lab
    for tok, suf in TOKEN_B3:
        if tok in clave.upper():
            lab = resolver_por_sufijo(labels, suf)
            if lab:
                return lab
    return None


class MapaPosicion:
    """Fallback por POSICION: hueso B3 mas cercano al vertice (model-space)."""

    def __init__(self, template):
        avb = AwgVertexBuffer.load(template)
        world, parents = avb.bind_worlds()
        self.labels = avb.bone_labels()
        self.heads = [(w[0][3], w[1][3], w[2][3]) for w in world]
        self.parents = parents

    def hueso(self, p):
        best, bd = self.labels[0], 1e18
        px, py, pz = p
        for i, h in enumerate(self.heads):
            d = (px - h[0]) ** 2 + (py - h[1]) ** 2 + (pz - h[2]) ** 2
            if d < bd:
                bd, best = d, self.labels[i]
        return best


# ---------------------------------------------------------------------------
def normalizar(verts):
    """UT (x,y,z) -> B3 (x,y,z). UT ya es Y-arriba como B3; se deja tal cual salvo
    que `eje == 'z'` (intercambio y/z) o `yaw` != 0."""
    return verts


def construir_grupos(spr, solo, excluir, labels, mapa_pos, escala, yaw, tex):
    """Lista de grupos con el formato de b3_gateway (bone = etiqueta B3)."""
    import math
    out = []
    for g in ut_formato.grupos(spr):
        clave = g.hueso_del_nombre()
        if solo and not any(s in clave for s in solo):
            continue
        if excluir and any(e in clave for e in excluir):
            continue
        vd = g.vertices()
        if not vd:
            continue
        bone = hueso_por_nombre(labels, clave)
        verts, pos_ut = [], []
        for (x, y, z, nx, ny, nz, u, v, _b, _w) in vd:
            if yaw:
                a = math.radians(yaw)
                x, z = x * math.cos(a) + z * math.sin(a), -x * math.sin(a) + z * math.cos(a)
            x, y, z = x * escala, y * escala, z * escala
            nx, ny, nz = (nx, ny, nz)
            pos_ut.append((x, y, z))
            verts.append({"pos": (x, y, z), "nrm": (nx, ny, nz), "uv": (u, v),
                          "bone": bone, "w": 1.0})
        if bone is None:                     # fallback por posicion
            for i, v in enumerate(verts):
                v["bone"] = mapa_pos.hueso(pos_ut[i])
        out.append({"name": g.nombre, "mat_key": "ut0", "rigid": None,
                    "verts": verts, "tris": g.triangulos(), "texture": tex})
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--spr", required=True)
    ap.add_argument("--ioram", default=None)
    ap.add_argument("--tpl", required=True, help="bin #AMB HD de plantilla (esqueleto)")
    ap.add_argument("--out", required=True)
    ap.add_argument("--solo", default=None, help="substrings separados por comas")
    ap.add_argument("--excluir", default=None)
    ap.add_argument("--tex", default=None, help="PNG de la textura base (todas las partes)")
    ap.add_argument("--modo", default="multi", choices=("multi", "atlas"))
    ap.add_argument("--escala", type=float, default=1.0)
    ap.add_argument("--yaw", type=float, default=0.0)
    ap.add_argument("--report", default=None)
    ap.add_argument("--listar-huesos", action="store_true")
    a = ap.parse_args()

    avb = AwgVertexBuffer.load(a.tpl)
    labels = avb.bone_labels()
    if a.listar_huesos:
        print("\n".join("%2d %s" % (i, l) for i, l in enumerate(labels)))
        return 0

    solo = [s.strip() for s in a.solo.split(",")] if a.solo else None
    excluir = [s.strip() for s in a.excluir.split(",")] if a.excluir else None
    spr = ut_formato.cargar(a.spr, a.ioram)
    mapa_pos = MapaPosicion(a.tpl)
    grupos = construir_grupos(spr, solo, excluir, labels, mapa_pos, a.escala, a.yaw, a.tex)
    if not grupos:
        print("[X] ningun grupo tras el filtro")
        return 2
    nv = sum(len(g["verts"]) for g in grupos)
    nt = sum(len(g["tris"]) for g in grupos)
    huesos = sorted({v["bone"] for g in grupos for v in g["verts"] if v["bone"]})
    print("[*] grupos=%d verts=%d tris=%d huesos B3=%s" % (len(grupos), nv, nt, huesos))

    stats = b3_gateway.emit(a.tpl, grupos, a.out, mode=a.modo, aux="hide", weight1=True,
                            report=a.report)
    print("[*] emitido %s (ventanas=%d IB=%d)" % (a.out, stats["windows"], stats["ib"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
