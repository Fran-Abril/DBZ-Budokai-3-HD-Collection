#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""xeno_model.py - lector de piezas de Dragon Ball Xenoverse (CPK del jugador) para el creador del Modo Heroe (studies 9b).

  Cpk(ruta_data_cpk).get("data/chara/HUM/HUM_300_Bust.emd") -> bytes (CRILAYLA descomprimido)
  leer_emd(bytes)  -> [Sub] con pos/nrm (n,3) YA en el espacio de B3 (z espejada, x10), uv, indices de hueso y pesos
                      (4 por vertice; los 3 primeros guardados, el 4.o es el resto, como SDBH), nombres de hueso por vertice
  Esk(bytes)       -> names, parent, world (posiciones de bind en espacio B3: x10, z espejada)
  leer_emb(bytes)  -> [(nombre, RGBA uint8)]
Los EMD de Xenoverse llevan los vertices COMPRIMIDOS (flag 0x8000): pos 3 f32, normal 4 f16, uv 2 f16, 4 u8 de hueso y
3 f16 de peso (36 B). Los sin comprimir son los de sdbh_model (48 B). No se guarda nada del juego en el repo.
"""
import io
import os
import struct
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
import cri       # noqa: E402
import crilayla  # noqa: E402

MIRROR = np.diag([1.0, 1.0, -1.0])
SCALE = 10.0
RUTA_STEAM = r"D:\SteamLibrary\steamapps\common\DB Xenoverse\data.cpk"


def buscar_cpk():
    """data.cpk de Xenoverse del jugador: DBZ3_XENO_CPK, la carpeta de Steam o las bibliotecas habituales."""
    cands = [os.environ.get("DBZ3_XENO_CPK", ""), RUTA_STEAM]
    for drv in "CDEFG":
        cands.append(r"%s:\SteamLibrary\steamapps\common\DB Xenoverse\data.cpk" % drv)
        cands.append(r"%s:\Program Files (x86)\Steam\steamapps\common\DB Xenoverse\data.cpk" % drv)
    return next((c for c in cands if c and os.path.isfile(c)), None)


class Cpk:
    def __init__(self, ruta=None):
        self.ruta = ruta or buscar_cpk()
        if not self.ruta:
            raise FileNotFoundError("no encuentro el data.cpk de Dragon Ball Xenoverse (DBZ3_XENO_CPK)")
        self.toc = {n: (s, o) for n, s, o in cri.cpk_toc(self.ruta)}
        self.f = open(self.ruta, "rb")

    def get(self, nombre):
        s, o = self.toc[nombre]
        self.f.seek(o)
        return crilayla.decompress(self.f.read(s))

    def buscar(self, patron):
        import re
        r = re.compile(patron)
        return sorted(n for n in self.toc if r.search(n))


def _u32(b, o):
    return struct.unpack_from("<I", b, o)[0]


def _u16(b, o):
    return struct.unpack_from("<H", b, o)[0]


def _cstr(b, o):
    return b[o:b.index(b"\0", o)].decode("latin1")


class Esk:
    def __init__(self, b):
        if b[:4] != b"#ESK":
            raise ValueError("no es ESK")
        so = _u32(b, 0x10)
        n = _u16(b, so)
        o_idx, o_nm, o_trs = (_u32(b, so + 4 + 4 * k) for k in range(3))
        self.names, self.parent, local = [], [], []
        for i in range(n):
            self.parent.append(struct.unpack_from("<4h", b, so + o_idx + 8 * i)[0])
            self.names.append(_cstr(b, so + _u32(b, so + o_nm + 4 * i)))
            t = struct.unpack_from("<12f", b, so + o_trs + 48 * i)
            local.append(np.array(t[0:3]))
        # solo posiciones (los huesos de Xenoverse y B3 estan en T con rotaciones ~identidad: lo comprobado en HUM_000)
        self.world = []
        for i in range(n):
            p = self.parent[i]
            self.world.append((self.world[p] if p >= 0 else np.zeros(3)) + local[i])
        self.world = [(w @ MIRROR) * SCALE for w in self.world]
        self.index = {nm: i for i, nm in enumerate(self.names)}


class Sub:
    pass


def leer_emd(b):
    if b[:4] != b"#EMD":
        raise ValueError("no es EMD")
    subs = []
    for mi in range(_u16(b, 0x12)):
        mo = _u32(b, _u32(b, 0x14) + 4 * mi)
        for j in range(_u16(b, mo + 2)):
            me = mo + _u32(b, mo + _u32(b, mo + 4) + 4 * j)
            for k in range(_u16(b, me + 0x36)):
                s = me + _u32(b, me + _u32(b, me + 0x38) + 4 * k)
                subs.append(_sub(b, s))
    return subs


def _sub(b, s):
    flags, vsz, nv, vo = struct.unpack_from("<4I", b, s + 0x30)
    sm = Sub()
    sm.name = _cstr(b, s + _u32(b, s + 0x40))
    sm.flags = flags
    ntd, to = b[s + 0x45], s + _u32(b, s + 0x48)
    sm.texdefs = [b[to + 12 * t + 1] for t in range(ntd)]
    raw = np.frombuffer(b, np.uint8, nv * vsz, s + vo).reshape(nv, vsz)
    comp = bool(flags & 0x8000)
    o = 0

    def take(cnt, dt):
        nonlocal o
        sz = np.dtype(dt).itemsize * cnt
        v = raw[:, o:o + sz].copy().view(dt).reshape(nv, cnt)
        o += sz
        return v
    P = take(3, "<f4").astype(np.float64)
    if flags & 0x2:
        N = take(4, "<f2")[:, :3].astype(np.float64) if comp else take(3, "<f4").astype(np.float64)
    else:
        N = np.zeros((nv, 3))
    UV = (take(2, "<f2") if comp else take(2, "<f4")).astype(np.float64) if flags & 0x4 else np.zeros((nv, 2))
    if flags & 0x8:
        take(2, "<f2" if comp else "<f4")
    if flags & 0x40:
        take(4, "u1")
    if flags & 0x80:
        take(4, "<f2") if comp else take(3, "<f4")
    if flags & 0x200:
        BI = take(4, "u1").astype(int)
        BW = (take(3, "<f2") if comp else take(3, "<f4")).astype(np.float64)
        BW = np.c_[BW, np.clip(1 - BW.sum(1), 0, 1)]
    else:
        BI, BW = np.zeros((nv, 4), int), np.c_[np.ones(nv), np.zeros((nv, 3))]
    sm.pos = (P @ MIRROR) * SCALE
    sm.nrm = N @ MIRROR
    sm.uv = UV
    sm.bone_names, bones, tris = [], np.zeros((nv, 4), int), []
    for g in range(_u16(b, s + 0x46)):
        go = s + _u32(b, s + _u32(b, s + 0x4C) + 4 * g)
        ni, nb, io_, no = struct.unpack_from("<4I", b, go)
        names = [_cstr(b, go + _u32(b, go + no + 4 * q)) for q in range(nb)]
        base = len(sm.bone_names)
        sm.bone_names += names
        idx = np.frombuffer(b, "<u2" if nv <= 0xFFFF else "<u4", ni, go + io_).astype(int)
        used = np.unique(idx)
        bones[used] = BI[used] + base
        tris += [(idx[i], idx[i + 2], idx[i + 1]) for i in range(0, ni - 2, 3)]   # espejo: winding invertido
    sm.bones, sm.weights = bones, BW
    sm.tris = np.array(tris, int).reshape(-1, 3)
    return sm


def leer_emb(b):
    from PIL import Image
    n, off, names = _u32(b, 0xC), _u32(b, 0x18), _u32(b, 0x1C)
    out = []
    for i in range(n):
        o, size = struct.unpack_from("<II", b, off + 8 * i)
        d = b[off + 8 * i + o:off + 8 * i + o + size]
        nm = _cstr(b, _u32(b, names + 4 * i)) if names else "tex%d" % i
        img = np.array(Image.open(io.BytesIO(d)).convert("RGBA")) if d[:4] == b"DDS " else None
        out.append((nm, img))
    return out
