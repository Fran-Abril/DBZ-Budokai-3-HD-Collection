#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ropa_xeno.py - Una PRENDA de Dragon Ball Xenoverse (del data.cpk del jugador) injertada en el cuerpo del heroe (studies 9b).

  python ropa_xeno.py --cuerpo body.bin --salida out.bin [--item 500] [--malla dougi] [--lod 02] [--color h,s,v]

Camino: HUM_<item>_Bust[_LOD0n].emd (+.emb) del CPK -> vertices en espacio B3 (xeno_model) -> reposicionado por huesos (los
esqueletos de Xenoverse y B3 estan los dos en T con rotaciones identidad y proporciones casi iguales: cada vertice se mueve la
media ponderada de (cabeza del hueso B3 equivalente - cabeza del hueso XV)) -> hueso B3 de mayor peso por vertice ->
awo_tools/injertar_ropa.py (quita la zona nativa del draw anfitrion, mete la prenda con su textura).
La textura de Xenoverse guarda el dibujo (arrugas, logo) en el ALFA y el color en una paleta (dye): aqui se tine con el color
de la ficha y el dibujo se pone como lineas oscuras. Nada del juego de Xenoverse se guarda en el repo."""
import argparse
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
for _p in (HERE, os.path.join(HERE, ".."), os.path.join(HERE, "..", "..", "awo_tools")):
    if os.path.isdir(_p) and _p not in sys.path:
        sys.path.append(_p)

import xeno_model as xm  # noqa: E402
from awg_vertex_buffer import AwgVertexBuffer  # noqa: E402

# hueso de Xenoverse -> sufijo del hueso B3 (R/L se rellena segun el lado)
MAPA = {"Base": "WAIST", "Pelvis": "WAIST", "Spine1": "STMC", "Spine2": "CHEST", "Chest": "CHEST", "Neck1": "NECK",
        "Head": "HEAD", "Shoulder": "{s}CHN", "Arm1": "{s}ARM1", "ArmHelper": "{s}ARM1", "ArmorParts": "{s}ARM1",
        "Elbow": "{s}ARM2", "Arm2": "{s}ARM2", "ArmRoll": "{s}ARM2", "Hand": "{s}HANDROT", "Leg1": "{s}LEG1",
        "LegHelper": "{s}LEG1", "Leg2": "{s}LEG2", "Knee": "{s}LEG2", "Foot": "{s}FOOT1", "Toe": "{s}FOOT2"}


def hueso_b3(nombre_xv, labels):
    """Indice del hueso B3 equivalente a un hueso de Xenoverse (b_R_Arm1 -> *_RARM1), o None."""
    p = nombre_xv.split("_")
    if len(p) < 3 and not (len(p) == 2 and p[1] in ("Base", "Pelvis")):
        return None
    lado = {"R": "R", "L": "L"}.get(p[1], "")
    clave = "_".join(p[2:]) if len(p) > 2 else p[1]
    suf = MAPA.get(clave)
    if suf is None:
        return None
    suf = suf.format(s=lado)
    for i, l in enumerate(labels):
        if not l.startswith("X") and l.split("_", 1)[-1] == suf:
            return i
    return None


def cargar_prenda(cpk, item, malla, lod, esk_b, labels, world):
    """(P, N, UV, bones_b3, tris, alfa) de la submalla `malla` de HUM_<item>_Bust, ya reposicionada al cuerpo B3."""
    base = "data/chara/HUM/HUM_%s_Bust" % item
    subs = xm.leer_emd(cpk.get(base + ("_LOD%s" % lod if lod else "") + ".emd"))
    s = next(x for x in subs if x.name == malla)
    esk = xm.Esk(esk_b)
    delta, bmap = {}, {}
    for i, nm in enumerate(esk.names):
        j = hueso_b3(nm, labels)
        if j is not None:
            bmap[i] = j
            delta[i] = np.array(world[j])[:3, 3] - esk.world[i]
    nb = len(s.pos)
    P, bones = s.pos.copy(), np.zeros(nb, int)
    falta = set()
    for v in range(nb):
        off, wsum, best, bw = np.zeros(3), 0.0, None, -1.0
        for k in range(4):
            w = s.weights[v, k]
            if w <= 1e-4:
                continue
            xi = esk.index.get(s.bone_names[s.bones[v, k]])
            if xi is None or xi not in bmap:
                falta.add(s.bone_names[s.bones[v, k]])
                continue
            off += w * delta[xi]
            wsum += w
            if w > bw:
                bw, best = w, bmap[xi]
        P[v] += off / wsum if wsum > 0 else 0
        bones[v] = best if best is not None else labels.index(next(l for l in labels if l.endswith("_CHEST")))
    emb = xm.leer_emb(cpk.get(base + ".emb"))
    return P, s.nrm.copy(), s.uv.copy(), bones, s.tris.copy(), s, emb, sorted(falta)


def textura_tenida(emb_img, rgb, size=(256, 256)):
    """RGBA uint8 para el DXT3 de B3: color liso `rgb` (0..1) con el dibujo del alfa de Xenoverse como lineas oscuras."""
    from PIL import Image, ImageFilter  # noqa: PLC0415
    a = emb_img[..., 3].astype(np.float32) / 255.0
    pil = Image.fromarray((a * 255).astype(np.uint8)).resize(size, Image.LANCZOS)
    a = np.asarray(pil, np.float32) / 255.0
    halo = np.asarray(Image.fromarray((a * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(2)), np.float32) / 255.0
    linea = np.clip((a - 0.28) / 0.45, 0, 1)
    sombra = 1.0 - 0.72 * linea - 0.16 * np.clip(halo / 0.5, 0, 1)
    out = np.zeros(a.shape + (4,), np.float32)
    out[..., :3] = np.clip(np.array(rgb, np.float32) * sombra[..., None], 0, 1)
    return (out * 255 + 0.5).astype(np.uint8)   # alfa 0 = banda de rampa normal


def injertar_prenda(amb, rgb=(0.93, 0.45, 0.06), item="500", malla="dougi", lod="02", cpk=None, extra=("Mark_go",)):
    """Pone la prenda en el #AMB `amb` del cuerpo: devuelve (bytes, resumen). Solo el draw con mas triangulos de torso cuya
    textura es exclusiva (en Goku, el dougi: tex 9)."""
    import collections  # noqa: PLC0415
    import model_render as mr  # noqa: PLC0415
    import injertar_pelo as ip  # noqa: PLC0415
    import injertar_ropa as ir  # noqa: PLC0415
    import vista_heroe as vh  # noqa: PLC0415
    cpk = cpk or xm.Cpk()
    model = mr.Model(bytes(amb))
    P0, _ = model.posed(vh.pose_de(model))
    zonas = vh.zonas_ficha(model, P0)
    td = ip.triangulos_por_draw(model)
    draws = [d for d, _ in td]
    cuenta = collections.Counter(d for d, z in zip(draws, zonas) if z == 0)
    host = next((d for d, _ in cuenta.most_common() if ir.textura_exclusiva(model, draws, d)
                 and all(z == 0 for dd, z in zip(draws, zonas) if dd == d)), None)
    if host is None:
        return bytes(amb), {"ropa": "sin draw de torso con textura exclusiva: sin injerto"}
    quitar = [d == host for d in draws]
    avb = AwgVertexBuffer._parse(bytes(amb))
    world, _ = avb.bind_worlds()
    labels = avb.bone_labels()
    esk_b = cpk.get("data/chara/HUM/HUM_000.esk")
    P, N, UV, bones, tris, sub, emb, falta = cargar_prenda(cpk, item, malla, lod, esk_b, labels, world)
    parts = [(P, N, UV, bones, tris)]
    for m2 in extra:   # el logo (Mark_go): misma textura, otra submalla
        try:
            q = cargar_prenda(cpk, item, m2, lod, esk_b, labels, world)
            parts.append((q[0], q[1], q[2], q[3], q[4]))
        except StopIteration:
            pass
    off, Ps, Ns, Us, Bs, Ts = 0, [], [], [], [], []
    for p_, n_, u_, b_, t_ in parts:
        Ps.append(p_); Ns.append(n_); Us.append(u_ + np.array([0.0, 1.0])); Bs.append(b_); Ts.append(t_ + off)
        off += len(p_)
    rgba = textura_tenida(emb[sub.texdefs[0]][1], rgb)
    out, r = ir.injertar_ropa(amb, model, quitar, host, np.concatenate(Ps), np.concatenate(Ns), np.concatenate(Us),
                              np.concatenate(Bs), np.concatenate(Ts), rgba)
    r["huesos_sin_equivalente"] = falta
    return out, r


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--cuerpo", required=True)
    ap.add_argument("--salida", required=True)
    ap.add_argument("--item", default="500")
    ap.add_argument("--malla", default="dougi")
    ap.add_argument("--lod", default="02")
    ap.add_argument("--rgb", default="0.93,0.45,0.06")
    a = ap.parse_args()
    out, r = injertar_prenda(open(a.cuerpo, "rb").read(), tuple(float(x) for x in a.rgb.split(",")), a.item, a.malla, a.lod)
    open(a.salida, "wb").write(out)
    print(r)


if __name__ == "__main__":
    main()
