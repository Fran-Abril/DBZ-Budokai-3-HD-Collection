#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""comprobar_assets.py - Verificador reutilizable de bins de modelo de DBZ Budokai 3 HD.

Dado un bin (ficha de personaje #AMB HD, un #AWO suelto, o un #AMO0 PS2 de IW/B1/B2)
dice si cumple las REGLAS del motor que documenta `USO_DE_ASSETS.md`:

  * contenedor #AMB: cabecera BE (+0x0C nº hijos, +0x14 tabla; cada entrada 16 B
    (off,size,type,pad)); magics de hijos (#AWO/#AWG/#AZT ... en HD, #AMO0/#AMG/#AMT en PS2).
  * vertice HD = VENTANA de 44 B: pos@0, weight@12, bone@16, nrm@20, FFFFFFFF@32, uv@36;
    region `[ib - g(0x2C), ib)`, `ib = awg0 + g(0x30)`.
  * detecta el layout "sec34" (FFFF@0, bone@28) como el error historico y el
    formato C de Babidi (FFFF@12, bone@40).
  * IB: indices < nº ventanas, `n_ib = g(0x34)/2`.
  * materiales (stride 0x50): base@+0x30, rampa@+0x34, tipo@+0x38 (0xB5 normal,
    0xF5/0x1F5 "cristal"/matcap = bit 0x40; 0x140101B doble cara), flags@+0x00.
  * texturas #AZT: entradas DDS; formato (DXT1/DXT3/DXT5/RGBA8), tamano y MIPS.
  * regla de rampa: material normal (0xB5) DEBE llevar rampa (si no, sale blanco/plano).
  * PS2 #AMO0: las LISTAS DE PESOS son de 32 B (no 16): se comprueba el nodo de
    pesos de cada hueso (a+br+8), el registro `[peso,n32,loc32,n16,loc16]` y que
    los destinos (u32 a +12 de cada trozo) caigan dentro del fichero, contando
    trozos por 32 B frente a 16 B (evidencia del bug historico).

Uso:
  python comprobar_assets.py <bin> [--json] [--pesos] [--quiet]
  python comprobar_assets.py --sumario <carpeta> [--patron *.bin]
  python comprobar_assets.py --selftest

Salida: issues con nivel ERROR / AVISO / INFO. Codigo 0 = sin ERROR; 1 = con ERROR.
"""
import argparse
import json
import math
import os
import struct
import sys

WINDOW = 44
MAT_STRIDE = 0x50
DDS_FOURCC = {b"DXT1", b"DXT2", b"DXT3", b"DXT4", b"DXT5", b"DX10", b"ATI2", b"BC5U"}


# ----------------------------------------------------------------- lectura BE
def be32(b, o):
    return struct.unpack_from(">I", b, o)[0]


def be16(b, o):
    return struct.unpack_from(">H", b, o)[0]


def be_f32(b, o):
    return struct.unpack_from(">f", b, o)[0]


def le32(b, o):
    return struct.unpack_from("<I", b, o)[0]


def _finit(x):
    return isinstance(x, float) and math.isfinite(x)


class Hallazgo(dict):
    def __init__(self, nivel, regla, mensaje, donde=""):
        super().__init__(nivel=nivel, regla=regla, mensaje=mensaje, donde=donde)


def _h(nivel, regla, mensaje, donde=""):
    return Hallazgo(nivel, regla, mensaje, donde)


# ----------------------------------------------------- lector de contenedor
def leer_contenedor(data):
    """(endian, [(magic, off, size)]) de un #AMB. Prueba BE (HD) y LE (PS2)."""
    if data[:4] != b"#AMB" or len(data) < 0x40:
        return None, []
    for endian, rd in (("BE", be32), ("LE", le32)):
        try:
            # tabla en +0x14 (absoluta rel base 0); nº en +0x0C; fallback +0x04.
            n = rd(data, 0x0C)
            tbl = rd(data, 0x14)
            if not (0 < n <= 8192) or not (0x20 <= tbl < len(data)):
                tbl = rd(data, 0x04)
            if not (0x20 <= tbl < len(data)):
                continue
            hijos, ok = [], 0
            for k in range(n):
                e = tbl + 16 * k
                if e + 16 > len(data):
                    break
                off, size = rd(data, e), rd(data, e + 4)
                if off == 0 and size == 0:
                    continue
                if off < len(data) and off + 4 <= len(data) and data[off:off + 1] == b"#":
                    hijos.append((data[off:off + 4], off, size))
                    ok += 1
            if ok:
                return endian, hijos
        except Exception:
            continue
    return None, []


# --------------------------------------------------------------- región AWG
def _awg_tabla(data, awo, rd):
    n_awg = rd(data, awo + 0x18)
    tbl = awo + rd(data, awo + 0x1C)
    out = []
    for i in range(min(n_awg, 512)):
        g = awo + rd(data, tbl + 4 * i)
        out.append((g, rd(data, g + 0x10) if 0 <= g < len(data) else 0))
    return n_awg, out


def _region_awg(data, g):
    """(vb0, n, ib_abs, n_ib, ini, fin) de las ventanas de un #AWG."""
    win_bytes = be32(data, g + 0x2C)
    ib_rel = be32(data, g + 0x30)
    ib_abs = g + ib_rel
    n = win_bytes // WINDOW
    n_ib_a = be32(data, g + 0x34) // 2           # regla de grow/awg_diff
    fin = g + be32(data, g + 0x38)
    n_ib_b = (fin - ib_abs) // 2
    vb0 = ib_abs - n * WINDOW
    return vb0, n, ib_abs, n_ib_a, n_ib_b, win_bytes, fin


def _clasifica_layout(data, vb0, n):
    """'ventana' (FFFF@32) | 'formatoC' (FFFF@12) | 'sec34' (FFFF@0) | 'desconocido'."""
    if n <= 0:
        return "vacio"
    m32 = m12 = m0 = b28 = 0
    for k in range(min(n, 128)):
        o = vb0 + k * WINDOW
        if be32(data, o + 32) == 0xFFFFFFFF:
            m32 += 1
        if be32(data, o + 12) == 0xFFFFFFFF:
            m12 += 1
        if be32(data, o + 0) == 0xFFFFFFFF:
            m0 += 1
        if 0 < be32(data, o + 28) < 256:
            b28 += 1
    if m32 >= m12 and m32 >= m0 and m32 > 0:
        return "ventana"
    if m12 >= m0 and m12 > 0:
        return "formatoC"
    if m0 > 0 and b28 > 0:
        return "sec34"
    return "desconocido"


def _comprueba_awg(data, g, n_bones, n_tex, h):
    if g + 0x40 > len(data) or data[g:g + 4] != b"#AWG":
        return
    vb0, n, ib_abs, n_ib_a, n_ib_b, win_bytes, fin = _region_awg(data, g)
    if not (0 <= vb0 and ib_abs <= len(data) and n >= 0):
        h.append(_h("ERROR", "awg.region", "region de ventanas fuera del fichero", "awg@%X" % g))
        return
    lay = _clasifica_layout(data, vb0, n)
    if win_bytes % WINDOW != 0:
        h.append(_h("ERROR", "awg.stride", "g(0x2C)=%d no es multiplo de 44" % win_bytes, "awg@%X" % g))
    if lay == "sec34":
        h.append(_h("ERROR", "awg.layout",
                    "layout 'sec34' (FFFF@0/bone@28): error historico; el HD usa la ventana "
                    "(bone@16, FFFF@32)", "awg@%X" % g))
    # g(0x38) es el puntero a la tabla de paleta (alineado a 4); su diferencia con
    # g(0x30) admite 0..1 indice de relleno. Solo avisa si descuadra mas.
    if abs(n_ib_a - n_ib_b) > 1:
        h.append(_h("AVISO", "awg.ib", "n_ib g(0x34)/2=%d != (g(0x38)-g(0x30))/2=%d"
                    % (n_ib_a, n_ib_b), "awg@%X" % g))
    # IF en ventana: comprobar campos por vertice
    if lay == "ventana" and n > 0:
        bad_marker = bad_bone = bad_w = bad_nrm = 0
        for k in range(n):
            o = vb0 + k * WINDOW
            if be32(data, o + 32) != 0xFFFFFFFF:
                bad_marker += 1
            bone = be32(data, o + 16)
            if not (bone < n_bones or bone == 0xFFFFFFFF):
                bad_bone += 1
            w = be_f32(data, o + 12)
            if not _finit(w) or w < -0.001 or w > 1.001:
                bad_w += 1
            nx, ny, nz = be_f32(data, o + 20), be_f32(data, o + 24), be_f32(data, o + 28)
            if not (_finit(nx) and _finit(ny) and _finit(nz)):
                bad_nrm += 1
            else:
                ln = math.sqrt(nx * nx + ny * ny + nz * nz)
                if 1e-3 < ln and abs(ln - 1.0) > 0.2:
                    bad_nrm += 1
        if bad_marker:
            h.append(_h("ERROR", "vert.marker", "%d/%d ventanas sin FFFFFFFF en +32 (layout roto)"
                        % (bad_marker, n), "awg@%X" % g))
        if bad_bone:
            h.append(_h("ERROR", "vert.bone", "%d/%d bones fuera de [0,%d) en +16"
                        % (bad_bone, n, n_bones), "awg@%X" % g))
        if bad_w:
            h.append(_h("AVISO", "vert.weight", "%d/%d pesos no finitos o fuera de [0,1]" % (bad_w, n),
                        "awg@%X" % g))
        if bad_nrm > n * 0.5:
            h.append(_h("AVISO", "vert.nrm", "%d/%d normales no unitarias (manchas de brillo)"
                        % (bad_nrm, n), "awg@%X" % g))
    # IB
    if ib_abs + 2 * max(n_ib_a, n_ib_b) <= len(data) and max(n_ib_a, n_ib_b) > 0:
        idx = [be16(data, ib_abs + 2 * k) for k in range(max(n_ib_a, n_ib_b))]
        reales = [v for v in idx if v != 0xFFFF]
        if reales and max(reales) >= max(n, 1):
            h.append(_h("ERROR", "ib.rango", "IB max=%d >= n ventanas=%d" % (max(reales), n),
                        "awg@%X" % g))
    # materiales
    mats = be32(data, g + 0x20)
    nmat = be32(data, g + 0x24)
    if mats and 0 < nmat <= 256:
        mo0 = g + mats
        for m in range(nmat):
            mo = mo0 + MAT_STRIDE * m
            if mo + MAT_STRIDE > len(data):
                h.append(_h("ERROR", "mat.rango", "material %d fuera del fichero" % m, "awg@%X" % g))
                break
            base = be32(data, mo + 0x30)
            ramp = be32(data, mo + 0x34)
            tipo = be32(data, mo + 0x38)
            tbyte = tipo & 0xFF
            es_matcap = bool(tipo & 0x40) or tbyte == 0xF5
            if base != 0xFFFFFFFF and n_tex and base >= n_tex:
                h.append(_h("AVISO", "mat.base", "material %d base tex %d >= %d" % (m, base, n_tex),
                            "awg@%X" % g))
            # regla de rampa: material normal con base debe llevar rampa
            if tbyte == 0xB5 and base != 0xFFFFFFFF and ramp == 0xFFFFFFFF:
                h.append(_h("ERROR", "mat.rampa", "material %d tipo 0xB5 sin rampa (saldra blanco/plano)"
                            % m, "awg@%X" % g))
            if ramp != 0xFFFFFFFF and n_tex and ramp >= n_tex:
                h.append(_h("AVISO", "mat.rampa.idx", "material %d rampa %d >= %d" % (m, ramp, n_tex),
                            "awg@%X" % g))


# ----------------------------------------------------------------- texturas
def _comprueba_azt(data, z, h):
    if data[z:z + 4] != b"#AZT":
        return 0
    n = be32(data, z + 0x10)
    idx = be32(data, z + 0x14)
    if not (0 < n <= 4096) or idx == 0:
        return 0
    for t in range(n):
        o = be32(data, z + idx + 4 * t)
        if not o:
            continue
        ent = z + o
        if ent + 0x20 > len(data):
            h.append(_h("ERROR", "azt.entrada", "textura %d fuera del #AZT" % t))
            continue
        d_off = be32(data, ent + 0x14)
        d_size = be32(data, ent + 0x18)
        if not d_off or z + d_off + 4 > len(data):
            continue
        if data[z + d_off:z + d_off + 4] != b"DDS ":
            h.append(_h("ERROR", "azt.dds", "textura %d sin cabecera DDS" % t))
            continue
        flags = be32(data, z + d_off + 8)
        hh = be32(data, z + d_off + 12)
        ww = be32(data, z + d_off + 16)
        fourcc = data[z + d_off + 84:z + d_off + 88]
        mips = be32(data, z + d_off + 28) if (flags & 0x20000) else 1
        fmt = fourcc.decode("latin1") if fourcc[:4] in DDS_FOURCC else (
            "RGBA8" if (be32(data, z + d_off + 80) & 0x41) else "?")
        if fourcc[:4] not in DDS_FOURCC and fmt == "?":
            h.append(_h("INFO", "azt.formato", "textura %d formato 4CC %r sin clasificar" % (t, fourcc)))
        if mips > 1 and d_size and d_size < 128 + ww * hh:
            h.append(_h("AVISO", "azt.mips",
                        "textura %d declara %d mips pero el tamano es del nivel 0 (regla B3: mipmaps=0)"
                        % (t, mips)))
        if ww == 0 or hh == 0:
            h.append(_h("ERROR", "azt.dims", "textura %d 0x0" % t))
    return n


# -------------------------------------------------------------- PS2 #AMO0
def _check_pesos_ps2(data, h):
    """Comprueba la regla de las listas de PESOS de 32 B de un #AMO0/#AMG PS2 (LE).

    Reproduce `src/ps2_modelo.cpp:196` (registro [peso,n32,loc32,n16,loc16],
    chunks de 32 B en `+8` y de 16 B en `+16`; destino = u32 a +12 del chunk).
    Devuelve (destinos32, destinos16, fuera_de_rango).
    """
    if data[:4] != b"#AMO" or len(data) < 0x20:
        return 0, 0, 0
    try:
        amgs = le32(data, 0x1C)
        a = le32(data, amgs) if amgs and amgs + 4 <= len(data) else 0
    except Exception:
        return 0, 0, 0
    if not a or a + 0x20 > len(data) or data[a:a + 4] != b"#AMG":
        return 0, 0, 0
    nb = min(le32(data, a + 0x10), 255)
    ejes = le32(data, a + 0x14)
    d32 = d16 = fuera = 0
    for i in range(1, nb):
        br = le32(data, a + ejes + 80 * i + 0x34) if a + ejes + 80 * i + 0x38 <= len(data) else 0
        lst = le32(data, a + br + 8) if br and a + br + 12 <= len(data) else 0
        if not lst:
            continue
        ne = min(le32(data, a + lst + 12), 64)
        for e in range(ne):
            en = a + lst + 16 + 32 * e
            if en + 20 > len(data):
                fuera += 1
                continue
            n32 = min(le32(data, en + 4), 4096)
            off = le32(data, en + 8)
            n16 = min(le32(data, en + 12), 4096)
            off16 = le32(data, en + 16)
            for k in range(n32):
                q = a + off + 32 * k
                if q + 16 > len(data):
                    fuera += 1
                    break
                dst = le32(data, q + 12)
                d32 += 1
                if dst and dst >= len(data):
                    fuera += 1
            for k in range(n16):
                q = a + off16 + 16 * k
                if q + 16 > len(data):
                    fuera += 1
                    break
                dst = le32(data, q + 12)
                d16 += 1
                if dst and dst >= len(data):
                    fuera += 1
    if d32 or d16:
        h.append(_h("INFO", "ps2.pesos",
                    "listas de pesos: %d destinos de 32 B + %d de 16 B; fuera de rango=%d. "
                    "Regla: leer SIEMPRE en tramos de 32 B (leer de 16 en 16 rigide al hueso 0)."
                    % (d32, d16, fuera)))
        if fuera:
            h.append(_h("ERROR", "ps2.pesos.rango",
                        "%d destinos fuera del fichero (listas desalineadas/truncadas)" % fuera))
    return d32, d16, fuera


# ------------------------------------------------------------------- informe
def comprobar_bytes(data, nombre="<memoria>", pesos=True):
    h = []
    info = {"nombre": nombre, "bytes": len(data), "tipo": "?", "n_awg": 0,
            "layouts": {}, "n_tex": 0, "estado": "OK"}
    if len(data) < 0x10:
        h.append(_h("ERROR", "tamano", "fichero demasiado corto"))
        info["estado"] = "ERROR"
        return info, h
    magic = data[:4]
    if magic == b"#AMB":
        endian, hijos = leer_contenedor(data)
        info["tipo"] = "AMB_" + (endian or "?")
        magics = sorted({m.decode("latin1") for m, _, _ in hijos})
        info["hijos"] = magics
        if not hijos:
            h.append(_h("ERROR", "amb.tabla", "no se pudo leer la tabla de hijos del #AMB"))
        awo_off = next((o for m, o, s in hijos if m in (b"#AWO", b"#AMO")), None)
        azt_off = next((o for m, o, s in hijos if m == b"#AZT"), None)
        if awo_off is not None and magic == b"#AMB":
            info["n_tex"] = _comprueba_azt(data, azt_off, h) if azt_off else 0
            if data[awo_off:awo_off + 4] == b"#AWO":
                _comprueba_awo(data, awo_off, info, h)
            elif pesos:  # #AMO0 PS2: la regla de las listas de pesos de 32 B
                _check_pesos_ps2(data[awo_off:], h)
        elif azt_off is not None:
            info["n_tex"] = _comprueba_azt(data, azt_off, h)
    elif magic == b"#AWO":
        info["tipo"] = "AWO"
        _comprueba_awo(data, 0, info, h)
    elif magic == b"#AMO":
        info["tipo"] = "AMO0_PS2"
        if pesos:
            _check_pesos_ps2(data, h)
    elif magic == b"#AZT":
        info["tipo"] = "AZT"
        info["n_tex"] = _comprueba_azt(data, 0, h)
    else:
        m = magic
        if m == b"\x0f\xf5\x12\xee":
            info["tipo"] = "LZX"
            h.append(_h("INFO", "tipo", "bin comprimido LZX: descomprimir (xbdecompress) antes de revisar"))
        else:
            info["tipo"] = m.decode("latin1")
            conocidos = (b"#ACM", b"#CSK", b"#BFC", b"#BSK", b"#AMM", b"#MDD", b"#MCF", b"#WCF",
                         b"#SPX", b"#AMA", b"#AZA", b"#AZS", b"#AZE", b"#AME", b"#SPT", b"#AMP",
                         b"#ZDD", b"#CAD", b"#CAS", b"#ACE", b"#AMC", b"#AMT", b"#SKA")
            if m in conocidos or not all(32 <= c < 127 for c in m):
                h.append(_h("INFO", "tipo", "contenido no-modelo (%s): la revision de modelo no aplica"
                            % info["tipo"]))
            else:
                h.append(_h("AVISO", "tipo", "magic desconocido %r" % m))
    if any(x["nivel"] == "ERROR" for x in h):
        info["estado"] = "ERROR"
    elif any(x["nivel"] == "AVISO" for x in h):
        info["estado"] = "AVISO"
    return info, h


def _comprueba_awo(data, awo, info, h):
    if data[awo:awo + 4] != b"#AWO":
        return
    n_bones = be32(data, awo + 0x10)
    n_awg = be32(data, awo + 0x18)
    info["n_awg"] = n_awg
    info["n_bones"] = n_bones
    if not (1 <= n_bones <= 255):
        h.append(_h("ERROR", "awo.bones", "n_bones=%d fuera de rango" % n_bones))
    if not (1 <= n_awg <= 256):
        h.append(_h("ERROR", "awo.awgs", "n_awg=%d fuera de rango" % n_awg))
        return
    _, tabla = _awg_tabla(data, awo, be32)
    conteo = {}
    for g, _nb in tabla:
        if 0 <= g < len(data) and data[g:g + 4] == b"#AWG":
            vb0, n, ib_abs, n_ib_a, n_ib_b, wb, fin = _region_awg(data, g)
            lay = _clasifica_layout(data, vb0, n)
            conteo[lay] = conteo.get(lay, 0) + 1
            _comprueba_awg(data, g, n_bones, info.get("n_tex", 0), h)
    info["layouts"] = conteo


# ------------------------------------------------------------------- selftest
def _dds_dxt3(w, h, fourcc=b"DXT3"):
    """Cabecera DDS de 128 B + un bloque DXT3 de 4x4 (16 B), mipmaps=0."""
    hdr = bytearray(128)
    hdr[0:4] = b"DDS "
    struct.pack_into("<I", hdr, 4, 124)
    struct.pack_into("<I", hdr, 8, 0x81007)          # CAPS|HEIGHT|WIDTH|PIXELFORMAT|LINEARSIZE
    struct.pack_into("<I", hdr, 12, h)
    struct.pack_into("<I", hdr, 16, w)
    struct.pack_into("<I", hdr, 76, 32)
    struct.pack_into("<I", hdr, 80, 0x4)
    hdr[84:88] = fourcc
    return bytes(hdr) + b"\x00" * 16


def _fixture(valido=True):
    """#AMB HD minimo: 1 #AWO (1 AWG, 3 verts, 1 triangulo, 1 material con rampa)
    + 1 #AZT (2 texturas: base y rampa)."""
    amb_hdr = 0x40
    awo_off = 0x40
    # --- #AWO ---
    awo = bytearray(0x300)
    struct.pack_into(">I", awo, 0x00, struct.unpack(">I", b"#AWO")[0])
    struct.pack_into(">I", awo, 0x10, 2)             # n_bones
    axes_rel = 0x200
    struct.pack_into(">I", awo, 0x14, axes_rel)      # ejes
    struct.pack_into(">I", awo, 0x18, 1)             # n_awg
    tbl_rel = 0x180
    struct.pack_into(">I", awo, 0x1C, tbl_rel)       # tabla AWG
    awg_rel = 0x240
    mats_rel = 0x100
    if awg_rel + mats_rel + MAT_STRIDE > len(awo):
        awo += bytearray(awg_rel + mats_rel + MAT_STRIDE - len(awo))
    struct.pack_into(">I", awo, tbl_rel, awg_rel)
    # --- #AWG ---
    g = awg_rel
    struct.pack_into(">I", awo, g, struct.unpack(">I", b"#AWG")[0])
    struct.pack_into(">I", awo, g + 0x10, 2)
    struct.pack_into(">I", awo, g + 0x14, axes_rel)
    struct.pack_into(">I", awo, g + 0x20, mats_rel)  # materiales (rel awg)
    struct.pack_into(">I", awo, g + 0x24, 1)         # n_mats
    n = 3
    win_rel = 0x40                                   # ventanas (rel awg)
    ib_rel = win_rel + n * WINDOW                    # IB justo tras las ventanas
    struct.pack_into(">I", awo, g + 0x2C, n * WINDOW)
    struct.pack_into(">I", awo, g + 0x30, ib_rel)
    struct.pack_into(">I", awo, g + 0x34, 6)         # 3 indices * 2
    struct.pack_into(">I", awo, g + 0x38, ib_rel + 6)
    struct.pack_into(">I", awo, g + 0x3C, 0)
    for k in range(n):                               # 3 ventanas validas
        o = g + win_rel + k * WINDOW
        struct.pack_into(">3f", awo, o + 0, 0.0, 0.0, 0.0)
        struct.pack_into(">f", awo, o + 12, 1.0)
        struct.pack_into(">I", awo, o + 16, k % 2)
        struct.pack_into(">3f", awo, o + 20, 0.0, 1.0, 0.0)
        struct.pack_into(">I", awo, o + 32, 0xFFFFFFFF)
        struct.pack_into(">2f", awo, o + 36, 0.0, 0.0)
    struct.pack_into(">3H", awo, g + ib_rel, 0, 1, 2)
    # material: base=0, rampa=1, tipo 0x1B5
    mo = g + mats_rel
    struct.pack_into(">I", awo, mo + 0x30, 0)
    struct.pack_into(">I", awo, mo + 0x34, 1)
    struct.pack_into(">I", awo, mo + 0x38, 0x1B5)
    if not valido:
        struct.pack_into(">I", awo, g + win_rel + 16, 99)   # bone fuera de rango
    # --- #AZT ---
    azt = bytearray(0x400)
    struct.pack_into(">I", azt, 0, struct.unpack(">I", b"#AZT")[0])
    struct.pack_into(">II", azt, 0x10, 2, 0x60)      # n=2, idx loc
    base_dds = 0x100
    ramp_dds = 0x200
    for t, (ent, doff) in enumerate(((0x80, base_dds), (0x90, ramp_dds))):
        struct.pack_into(">I", azt, 0x60 + 4 * t, ent)
        struct.pack_into(">II", azt, ent + 0x14, doff, 144)
        struct.pack_into(">HH", azt, ent + 0x10, 4, 4)
    azt[base_dds:base_dds + 144] = _dds_dxt3(4, 4)
    azt[ramp_dds:ramp_dds + 144] = _dds_dxt3(4, 4)
    # --- #AMB ---
    azt_off = awo_off + len(awo)
    total = bytearray(amb_hdr + len(awo) + len(azt))
    total[0:4] = b"#AMB"
    struct.pack_into(">I", total, 0x04, 0x20)
    struct.pack_into(">I", total, 0x0C, 2)
    struct.pack_into(">I", total, 0x10, 2)
    struct.pack_into(">I", total, 0x14, 0x20)
    struct.pack_into(">I", total, 0x18, 0x40)
    struct.pack_into(">IIII", total, 0x20, awo_off, len(awo), 0xFFFFFFFF, 0)
    struct.pack_into(">IIII", total, 0x30, azt_off, len(azt), 0xFFFFFFFF, 0)
    total[awo_off:awo_off + len(awo)] = awo
    total[azt_off:azt_off + len(azt)] = azt
    return bytes(total)


def selftest():
    ok = True
    info, h = comprobar_bytes(_fixture(True), "fixture_valido")
    errs = [x for x in h if x["nivel"] == "ERROR"]
    print("[selftest] fixture valido: estado=%s n_awg=%s layouts=%s errores=%d"
          % (info["estado"], info.get("n_awg"), info.get("layouts"), len(errs)))
    if errs:
        ok = False
        for e in errs:
            print("   ", e["regla"], e["mensaje"])
    if info["layouts"].get("ventana") != 1:
        ok = False
        print("    FAIL: no se clasifico el AWG como 'ventana'")
    info2, h2 = comprobar_bytes(_fixture(False), "fixture_bone_malo")
    if not any(x["nivel"] == "ERROR" and x["regla"] == "vert.bone" for x in h2):
        ok = False
        print("    FAIL: no se detecto el bone fuera de rango")
    # layout sec34
    d = bytearray(_fixture(True))
    for k in range(3):
        o = 0x40 + 0x240 + 0x40 + k * WINDOW
        struct.pack_into(">I", d, o + 32, 0)          # quita el marker de ventana
        struct.pack_into(">I", d, o + 0, 0xFFFFFFFF)  # lo pone en +0 (sec34)
        struct.pack_into(">I", d, o + 28, 5)          # bone en +28
    if not any(x["regla"] == "awg.layout" for x in comprobar_bytes(bytes(d), "sec34")[1]):
        ok = False
        print("    FAIL: no se detecto el layout sec34")
    print("[selftest] %s" % ("OK" if ok else "FALLO"))
    return 0 if ok else 1


# ------------------------------------------------------------------- sumario
def cmd_sumario(carpeta, patron):
    import fnmatch
    filas = []
    for dp, _, fns in os.walk(carpeta):
        for f in sorted(fns):
            if fnmatch.fnmatch(f, patron):
                p = os.path.join(dp, f)
                try:
                    data = open(p, "rb").read()
                except OSError:
                    continue
                info, h = comprobar_bytes(data, os.path.relpath(p, carpeta))
                filas.append((p, info, h))
    print("%-44s %-10s %5s %-16s %5s %s" % ("fichero", "tipo", "awg", "layouts", "tex", "estado"))
    for p, info, h in filas:
        print("%-44s %-10s %5s %-16s %5s %s"
              % (info["nombre"][-44:], info["tipo"], info.get("n_awg", 0),
                 json.dumps(info.get("layouts", {}), ensure_ascii=False),
                 info.get("n_tex", 0), info["estado"]))
    malos = sum(1 for _, i, _ in filas if i["estado"] == "ERROR")
    print("\n%d ficheros, %d con ERROR" % (len(filas), malos))
    return 1 if malos else 0


def main(argv=None):
    ap = argparse.ArgumentParser(description="Verificador de bins de modelo de DBZ B3 HD")
    ap.add_argument("bin", nargs="?", help="fichero a comprobar")
    ap.add_argument("--json", action="store_true", help="informe en JSON")
    ap.add_argument("--pesos", action="store_true", default=True, help="comprobar listas de pesos PS2")
    ap.add_argument("--no-pesos", dest="pesos", action="store_false")
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--sumario", metavar="CARPETA")
    ap.add_argument("--patron", default="*.bin")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    if args.sumario:
        return cmd_sumario(args.sumario, args.patron)
    if not args.bin:
        ap.print_help()
        return 2
    if not os.path.isfile(args.bin):
        print("ERROR: no existe %s" % args.bin)
        return 2
    data = open(args.bin, "rb").read()
    info, h = comprobar_bytes(data, os.path.basename(args.bin), pesos=args.pesos)
    if args.json:
        print(json.dumps({"info": info, "hallazgos": h}, ensure_ascii=False, indent=2))
    else:
        print("Fichero : %s (%d B)" % (info["nombre"], info["bytes"]))
        print("Tipo    : %s   AWGs=%s   tex=%s   layouts=%s"
              % (info["tipo"], info.get("n_awg"), info.get("n_tex"),
                 json.dumps(info.get("layouts", {}), ensure_ascii=False)))
        print("Estado  : %s" % info["estado"])
        if not args.quiet:
            for x in h:
                print("  [%s] %-16s %s %s" % (x["nivel"], x["regla"], x["mensaje"],
                                              ("(%s)" % x["donde"]) if x["donde"] else ""))
    return 1 if info["estado"] == "ERROR" else 0


if __name__ == "__main__":
    raise SystemExit(main())
