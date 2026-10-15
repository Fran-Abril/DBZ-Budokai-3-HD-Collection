#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""cri.py - lectura minima de los contenedores de audio de CRI (frente Ports, 2026-10-15).

Super Dragon Ball Heroes World Mission (PC) guarda las voces de cada personaje en
data_07_win.cpk: bc<clave>/CueSheet_BC<CLAVE>.acb (nombres de los sonidos) + .awb (AFS2 con un
HCA de 16 kHz sin cifrar por sonido). Las voces son los sonidos "B<CLAVE>_nnn"; el resto
(KamehameCharge, DBZP_...) son efectos.

  utf(b)                    tabla @UTF -> (nombre, filas)
  cpk_toc(ruta)             [(ruta interna, tamano, offset)] de un .cpk
  awb_split(b)              {id: bytes} de un AFS2
  acb_waves(acb)            [(nombre del sonido, id en el awb)]
  voces_sdbh(cpks, clave)   [(nombre, int16 16 kHz)] de las voces del personaje (decodifica con ffmpeg)

No hay codificador ni descompresor CRILAYLA: los .acb/.awb de las voces no vienen comprimidos.
"""
import os
import shutil
import struct
import subprocess
import tempfile


def _xor(b):
    m, o = 0x655F, bytearray(b)
    for i in range(len(o)):
        o[i] ^= m & 0xFF
        m = (m * 0x4115) & 0xFFFFFFFF
    return bytes(o)


_FMT = {0: ">B", 1: ">b", 2: ">H", 3: ">h", 4: ">I", 5: ">i", 6: ">Q", 7: ">q", 8: ">f", 9: ">d"}


def utf(b):
    if b[:4] != b"@UTF":
        b = _xor(b)
    if b[:4] != b"@UTF":
        raise ValueError("no es una tabla @UTF")
    t = b[8:8 + struct.unpack(">I", b[4:8])[0]]
    rows_off = struct.unpack(">H", t[2:4])[0]
    str_off, data_off, name_off = struct.unpack(">III", t[4:16])
    ncol, roww, nrow = struct.unpack(">HHI", t[16:24])

    def s(o):
        a = str_off + o
        return t[a:t.index(b"\0", a)].decode("utf-8", "replace")

    def val(typ, p):
        if typ in _FMT:
            n = struct.calcsize(_FMT[typ])
            return struct.unpack(_FMT[typ], t[p:p + n])[0], n
        if typ == 0xA:
            return s(struct.unpack(">I", t[p:p + 4])[0]), 4
        if typ == 0xB:
            o, n = struct.unpack(">II", t[p:p + 8])
            return t[data_off + o:data_off + o + n], 8
        raise ValueError("tipo de columna %x" % typ)

    cols, p = [], 24
    for _ in range(ncol):
        fl, nm = t[p], s(struct.unpack(">I", t[p + 1:p + 5])[0])
        p += 5
        const = None
        if fl & 0xF0 in (0x30, 0x70):
            const, n = val(fl & 0x0F, p)
            p += n
        cols.append((fl, nm, const))
    rows = []
    for r in range(nrow):
        q, row = rows_off + r * roww, {}
        for fl, nm, const in cols:
            if fl & 0xF0 == 0x50:
                row[nm], n = val(fl & 0x0F, q)
                q += n
            else:
                row[nm] = const
        rows.append(row)
    return s(name_off), rows


def cpk_toc(path):
    with open(path, "rb") as f:
        h = f.read(16)
        if h[:4] != b"CPK ":
            raise ValueError("no es un CPK: %s" % path)
        hdr = utf(f.read(struct.unpack("<Q", h[8:16])[0]))[1][0]
        to, co = hdr.get("TocOffset") or 0, hdr.get("ContentOffset") or 0
        if not to:
            return []
        add = min(to, co) if co else to
        f.seek(to)
        n = struct.unpack("<Q", f.read(16)[8:16])[0]
        rows = utf(f.read(n))[1]
    return [(((r.get("DirName") or "") + "/" + (r.get("FileName") or "")).lstrip("/"), r["FileSize"],
             r["FileOffset"] + add) for r in rows]


def awb_split(d):
    if d[:4] != b"AFS2":
        raise ValueError("no es un AFS2")
    osz, isz = d[5], d[6]
    n, align = struct.unpack("<IH", d[8:14])
    ids = [int.from_bytes(d[16 + i * isz:16 + (i + 1) * isz], "little") for i in range(n)]
    p = 16 + n * isz
    offs = [int.from_bytes(d[p + i * osz:p + (i + 1) * osz], "little") for i in range(n + 1)]
    return {ids[i]: d[(offs[i] + align - 1) // align * align:offs[i + 1]] for i in range(n)}


def acb_waves(acb):
    """(sonido, id del awb) por nombre: sonido -> secuencia -> pista -> orden 0x07D0 (sintetizador) -> onda."""
    top = utf(acb)[1][0]

    def tab(k):
        v = top.get(k)
        return utf(v)[1] if isinstance(v, bytes) and v[:4] == b"@UTF" else []

    cue, names, seq, trk, syn, wav = (tab(k) for k in ("CueTable", "CueNameTable", "SequenceTable", "TrackTable",
                                                         "SynthTable", "WaveformTable"))
    ev = tab("EventTable") or tab("CommandTable")      # las versiones antiguas guardan los eventos en CommandTable
    out = []
    for r in names:
        c = cue[r["CueIndex"]]
        if c["ReferenceType"] != 3:
            continue
        s = seq[c["ReferenceIndex"]]
        for k in range(s["NumTracks"]):
            b = ev[trk[struct.unpack(">H", s["TrackIndex"][2 * k:2 * k + 2])[0]]["EventIndex"]]["Command"]
            p = 0
            while p + 3 <= len(b):
                code, n = struct.unpack(">HB", b[p:p + 3])
                if code == 0x07D0 and n >= 4 and struct.unpack(">H", b[p + 3:p + 5])[0] == 2:
                    ri = syn[struct.unpack(">H", b[p + 5:p + 7])[0]]["ReferenceItems"]
                    for j in range(0, len(ri) - 3, 4):
                        ty, ix = struct.unpack(">HH", ri[j:j + 4])
                        if ty == 1:
                            w = wav[ix]
                            out.append((r["CueName"], w["StreamAwbId"] if w.get("Streaming") else w["MemoryAwbId"]))
                p += 3 + n
    return out


def hca_pcm(hca, rate=16000):
    """HCA -> int16 mono a `rate` (ffmpeg; el modulo no trae decodificador HCA propio)."""
    import numpy as np  # noqa: PLC0415
    exe = shutil.which("ffmpeg")
    if not exe:
        raise RuntimeError("hace falta ffmpeg para leer las voces de Heroes (HCA)")
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "v.hca")
        open(src, "wb").write(hca)
        r = subprocess.run([exe, "-hide_banner", "-loglevel", "error", "-i", src, "-ac", "1", "-ar", str(rate),
                            "-f", "s16le", "-"], capture_output=True, check=True)
    return np.frombuffer(r.stdout, "<i2").astype(np.int16)


def voces_sdbh(cpks, clave):
    """Voces (sonidos B<CLAVE>_*) de los CueSheet_BC<CLAVE>*.acb/.awb de Heroes, sin repetir."""
    pre, want, files = "bc%s/" % clave.lower(), "B%s_" % clave.upper(), {}
    for cpk in cpks:
        for name, size, off in cpk_toc(cpk):
            if name.lower().startswith(pre) and name.lower().endswith((".acb", ".awb")):
                with open(cpk, "rb") as f:
                    f.seek(off)
                    files[name.lower()] = f.read(size)
        if files:
            break
    out, seen = [], set()
    for name in sorted(n for n in files if n.endswith(".acb")):
        awb = files.get(name[:-4] + ".awb")
        if not awb or files[name][:8] == b"CRILAYLA" or awb[:8] == b"CRILAYLA":
            continue
        waves, used = awb_split(awb), set()
        for cue, wid in acb_waves(files[name]):
            if cue.upper().startswith(want) and cue not in seen and wid in waves and wid not in used:
                seen.add(cue)
                used.add(wid)   # BKMS_003_0 y BKMS_003_1 suenan con la misma onda
                out.append((cue, hca_pcm(waves[wid])))
    return out


if __name__ == "__main__":
    # autocomprobacion: tabla @UTF minima (1 columna u16 por fila, 2 filas) y AFS2 de 2 ficheros
    strings = b"<NULL>\0T\0x\0"
    rows = struct.pack(">HH", 7, 9)
    head = struct.pack(">HHIIIHHI", 1, 24 + 5, 24 + 5 + 4, 24 + 5 + 4 + len(strings), 7, 1, 2, 2)
    body = head + bytes([0x52]) + struct.pack(">I", 9) + rows + strings
    nm, rr = utf(b"@UTF" + struct.pack(">I", len(body)) + body)
    assert nm == "T" and [r["x"] for r in rr] == [7, 9], (nm, rr)
    a = b"AFS2" + bytes([1, 4, 2, 0]) + struct.pack("<IH", 2, 4) + b"\0\0" + struct.pack("<HH", 5, 6)
    o0 = len(a) + 12
    a += struct.pack("<III", o0, o0 + 3, o0 + 8) + b"abc" + b"\0" + b"defg"
    assert awb_split(a) == {5: b"abc", 6: b"defg"}, awb_split(a)
    print("cri.py: @UTF y AFS2 OK")
