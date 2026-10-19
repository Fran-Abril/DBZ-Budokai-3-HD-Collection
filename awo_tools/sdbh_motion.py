#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""sdbh_motion.py - animaciones de Super Dragon Ball Heroes World Mission (y Xenoverse) -> B3.

Los bancos de animacion de Heroes NO son un formato propio: cada `motion/bcXXX/<CLAVE>_Cnn_Tnn.emb`
es un contenedor #EMB (el mismo de las texturas) con dos entradas:
   * `<nombre>.ean`                  la animacion (formato #EAN, little endian en PC)
   * `motion_merge_animation_name.csv`  tabla indice,nombre de clip (0 FTWK, 9 APPEAL, 30 WIN...)

#EAN (LE) - cabecera:
  0x00 "#EAN"; 0x04 u16 0xFFFE; 0x06 u16 0x20 (cabecera)
  0x08 u32; 0x0C u32; 0x10 u16 0x0400; 0x12 u16 n_anim
  0x14 u32 off_esqueleto; 0x18 u32 off_tabla_anims; 0x1C u32 off_tabla_nombres
  esqueleto: u16 n_huesos; +4 off registros; +8 off nombres; +0xC off matrices locales
  animacion: u16; u8 frameIDSize; u8 frameSize; u32 n_frames; u32 n_huesos;
             u32 off_datos_huesos (rel anim); u32 offsets[n_huesos] (rel anim)
  hueso: u16 id; u16 n_controladores; u32 off_tabla (rel hueso)
  controlador: u8 canal (0=pos,1=rot,2=escala); u8 tipo; u16; u32 n_frames;
               u32 off_ids (rel ctrl); u32 off_valores (rel ctrl)
  frameSize 1 = half floats, 2 = floats.

Este modulo SOLO lee/escribe ficheros. No ejecuta nada del juego.

  python sdbh_motion.py list <motion.emb|.ean>
  python sdbh_motion.py dump <ean> <salida.json> [--clip N]
  python sdbh_motion.py repack <ean> <json> <salida.ean>     (ida y vuelta: sin cambios = bytes identicos)
  python sdbh_motion.py to-b3 <ean> <clip> <plantilla.bin> <salida.json> [--fps 60]
  python sdbh_motion.py csv <motion.emb>                     (tabla indice,nombre de clips)
"""
import argparse
import hashlib
import json
import math
import os
import struct
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))


# ------------------------------------------------------------------ contenedor #EMB de motion
def emb_entries(path, only=None):
    """[(nombre, bytes)] de un #EMB (archivo de animacion de Heroes). only: sufijo a leer."""
    f = open(path, "rb")
    with f:
        h = f.read(0x20)
        if h[:4] != b"#EMB":
            raise ValueError("no es un #EMB: %s" % path)
        n = struct.unpack_from("<I", h, 0xC)[0]
        off = struct.unpack_from("<I", h, 0x18)[0]
        names = struct.unpack_from("<I", h, 0x1C)[0]
        f.seek(off)
        tab = f.read(8 * n)
        f.seek(names)
        nametab = f.read(4 * n)
        out = []
        for i in range(n):
            o, size = struct.unpack_from("<II", tab, 8 * i)
            q = struct.unpack_from("<I", nametab, 4 * i)[0]
            f.seek(q)
            nm = b""
            while True:
                ch = f.read(1)
                if not ch or ch == b"\0":
                    break
                nm += ch
            nm = nm.decode("latin1")
            if only and not nm.lower().endswith(only):
                continue
            f.seek(off + 8 * i + o)
            out.append((nm, f.read(size)))
        return out


def motion_clips(path):
    """{indice: nombre} de la tabla CSV que viaja dentro del .emb de motion."""
    out = {}
    for nm, d in emb_entries(path, only=".csv"):
            for ln in d.decode("latin1", "replace").splitlines():
                p = ln.split(",")
                if len(p) >= 2 and p[0].strip().isdigit():
                    out[int(p[0])] = p[1].strip()
    return out


# ------------------------------------------------------------------ #EAN
class Ctrl:
    __slots__ = ("canal", "tipo", "n", "ids", "vals", "off_ids", "off_vals", "size")

    def __init__(self, canal, tipo, ids, vals, off_ids, off_vals, size):
        self.canal, self.tipo, self.n = canal, tipo, len(ids)
        self.ids, self.vals = ids, vals
        self.off_ids, self.off_vals, self.size = off_ids, off_vals, size


class Bone:
    __slots__ = ("id", "name", "ctrls", "off")

    def __init__(self, bid, name, ctrls, off):
        self.id, self.name, self.ctrls, self.off = bid, name, ctrls, off


class Clip:
    __slots__ = ("name", "frames", "bones", "off", "id_size", "val_size")

    def __init__(self):
        self.name, self.frames, self.bones, self.off = "", 0, [], 0
        self.id_size = self.val_size = 0


class Ean:
    def __init__(self, path):
        self.path = path
        self.data = bytearray(open(path, "rb").read())
        b = self.data
        if b[:4] != b"#EAN":
            raise ValueError("no es #EAN: %s" % path)
        self.endian = "little" if struct.unpack_from("<H", b, 4)[0] == 0xFFFE else "big"
        e = "<" if self.endian == "little" else ">"
        self.e = e
        self.n_anim, self.off_skel, self.off_anims, self.off_names = struct.unpack_from(e + "HIII", b, 0x12)
        self._skel()
        self.clips = []
        tab = self.off_anims
        ntab = self.off_names
        for i in range(self.n_anim):
            c = Clip()
            c.off = struct.unpack_from(e + "I", b, tab + 4 * i)[0]
            o = struct.unpack_from(e + "I", b, ntab + 4 * i)[0]
            c.name = b[o:b.index(b"\0", o)].decode("latin1")
            self._clip(c)
            self.clips.append(c)

    def _skel(self):
        b, e, s = self.data, self.e, self.off_skel
        self.n_bones = struct.unpack_from(e + "H", b, s)[0]
        o_rec = s + struct.unpack_from(e + "I", b, s + 4)[0]
        o_nm = s + struct.unpack_from(e + "I", b, s + 8)[0]
        o_mat = s + struct.unpack_from(e + "I", b, s + 12)[0]
        self.bone_names = []
        self.bone_parent = []
        for i in range(self.n_bones):
            p = struct.unpack_from(e + "I", b, o_nm + 4 * i)[0] + s      # offset absoluto
            self.bone_names.append(b[p:b.index(b"\0", p)].decode("latin1"))
            self.bone_parent.append(struct.unpack_from(e + "h", b, o_rec + 8 * i)[0] + 1)   # 1-based (0 = raiz)
        self.bone_mat_off = o_mat

    def _clip(self, c):
        b, e = self.data, self.e
        a = c.off
        c.id_size, c.val_size = b[a + 2], b[a + 3]
        c.frames, nb = struct.unpack_from(e + "II", b, a + 4)
        ob = a + struct.unpack_from(e + "I", b, a + 12)[0]
        offs = list(struct.unpack_from(e + "%dI" % nb, b, a + 16))
        for k in range(nb):
            bo = a + offs[k]
            bid, nc = struct.unpack_from(e + "HH", b, bo)
            bid = bid - 1 if bid else -1          # 1-based en el fichero
            ot = bo + struct.unpack_from(e + "I", b, bo + 4)[0]
            ctrls = []
            for z in range(nc):
                co = bo + struct.unpack_from(e + "I", b, ot + 4 * z)[0]
                canal, tipo = b[co], b[co + 1]
                n = struct.unpack_from(e + "I", b, co + 4)[0]
                oi = co + struct.unpack_from(e + "I", b, co + 8)[0]
                ov = co + struct.unpack_from(e + "I", b, co + 12)[0]
                if c.id_size == 0:
                    ids = list(b[oi:oi + n])
                else:
                    ids = list(struct.unpack_from(e + "%dH" % n, b, oi))
                ncomp = {0: 4, 1: 4, 2: 3}[canal]
                fmt = "e" if c.val_size == 1 else "f"
                vals = np.array(struct.unpack_from(e + "%d%s" % (n * ncomp, fmt), b, ov),
                                np.float64).reshape(n, ncomp)
                ctrls.append(Ctrl(canal, tipo, ids, vals, oi, ov, n * ncomp * (2 if c.val_size == 1 else 4)))
            c.bones.append(Bone(bid, self.bone_names[bid] if bid < len(self.bone_names) else "hueso%d" % bid,
                                ctrls, bo))

    # -- escritura quirurgica: mismos offsets (sin cambios = bytes identicos)
    def save(self, out):
        data = bytearray(self.data)
        e = self.e
        for c in self.clips:
            for bo in c.bones:
                for ct in bo.ctrls:
                    fmt = "e" if c.val_size == 1 else "f"
                    struct.pack_into(e + "%d%s" % (ct.n * ct.vals.shape[1], fmt), data, ct.off_vals,
                                     *ct.vals.reshape(-1).tolist())
        open(out, "wb").write(data)

    def sha(self):
        return hashlib.sha256(self.data).hexdigest()


def _quat_from_euler(rx, ry, rz):
    """Euler (radianes, orden yaw->pitch->roll = Z*Y*X como el #ACM) -> cuaternion (x,y,z,w)."""
    cx, sx = math.cos(rx / 2), math.sin(rx / 2)
    cy, sy = math.cos(ry / 2), math.sin(ry / 2)
    cz, sz = math.cos(rz / 2), math.sin(rz / 2)
    return np.array([sx * cy * cz - cx * sy * sz, cx * sy * cz + sx * cy * sz,
                     cx * cy * sz - sx * sy * cz, cx * cy * cz + sx * sy * sz])


def _euler_from_quat(q):
    x, y, z, w = q
    # R = Rz*Ry*Rx  -> yaw (Z), pitch (Y), roll (X)
    roll = math.atan2(2 * (w * x + y * z), 1 - 2 * (x * x + y * y))
    s = max(-1.0, min(1.0, 2 * (w * y - z * x)))
    pitch = math.asin(s)
    yaw = math.atan2(2 * (w * z + x * y), 1 - 2 * (y * y + z * z))
    return roll, pitch, yaw


def _awo():
    """Carpeta de awo_tools (aqui mismo; DBZ3_AWO_TOOLS para probar desde otra carpeta)."""
    p = os.environ.get("DBZ3_AWO_TOOLS") or HERE
    if p not in sys.path:
        sys.path.insert(0, p)
    return p


def b3_bone_map(names):
    """{nombre EAN: etiqueta B3} por sufijo (SDBH/Xenoverse comparten los nombres de hueso)."""
    _awo()
    import sdbh_model as S
    out = {}
    for n in names:
        out[n] = S.sdbh_suffix(n)
    return out


def to_b3(ean, clip, plantilla, fps=60):
    """Clip #EAN -> canales angulares en el convenio del #ACM de B3 (roll/pitch/yaw u16 por frame)."""
    _awo()
    import sdbh_model as S
    tpl = S.Template(plantilla)
    labels = {S.bone_suffix(l): i for i, l in enumerate(tpl.labels)}
    c = ean.clips[clip]
    dur = c.frames
    step = 60.0 / fps
    out = {"clip": c.name, "frames": dur, "fps": fps, "huesos": {}, "sin_mapear": []}
    for bo in c.bones:
        try:
            suf = S.sdbh_suffix(bo.name)
        except Exception:
            suf = bo.name.upper()
        bid = labels.get(suf)
        if bid is None:
            out["sin_mapear"].append(bo.name)
            continue
        rot = next((ct for ct in bo.ctrls if ct.canal == 1), None)
        pos = next((ct for ct in bo.ctrls if ct.canal == 0), None)
        row = {"hueso_b3": tpl.labels[bid], "indice": bid, "ang": [], "pos": []}
        if rot is not None:
            for f, q in zip(rot.ids, rot.vals):
                r, p, y = _euler_from_quat(q)
                row["ang"].append([int(round(f * step)), int(round((r / (2 * math.pi)) % 1 * 65536)) & 0xFFFF,
                                   int(round((p / (2 * math.pi)) % 1 * 65536)) & 0xFFFF,
                                   int(round((y / (2 * math.pi)) % 1 * 65536)) & 0xFFFF])
        if pos is not None:
            row["pos"] = [[int(f), [round(float(v), 5) for v in p]] for f, p in zip(pos.ids, pos.vals[:, :3])]
        out["huesos"][tpl.labels[bid]] = row
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="cmd")
    a = sub.add_parser("list")
    a.add_argument("file")
    a = sub.add_parser("dump")
    a.add_argument("ean")
    a.add_argument("salida")
    a.add_argument("--clip", type=int, default=None)
    a = sub.add_parser("repack")
    a.add_argument("ean")
    a.add_argument("json")
    a.add_argument("salida")
    a = sub.add_parser("to-b3")
    a.add_argument("ean")
    a.add_argument("clip", type=int)
    a.add_argument("plantilla")
    a.add_argument("salida")
    a.add_argument("--fps", type=float, default=60.0)
    a = sub.add_parser("catalog")
    a.add_argument("raiz", help="carpeta motion/ (con bcXXX/ dentro)")
    a.add_argument("salida", help="TSV: personaje, fichero, n_clips, clips")
    a = sub.add_parser("csv")
    a.add_argument("emb")
    x = ap.parse_args(argv)
    if x.cmd == "list":
        p = x.file
        if p.lower().endswith(".emb"):
            ent = emb_entries(p)
            ean = EanBytes(next(d for n, d in ent if n.lower().endswith(".ean")), p)
        else:
            ean = Ean(p)
        print("%s: %d animaciones, %d huesos" % (os.path.basename(p), ean.n_anim, ean.n_bones))
        for i, c in enumerate(ean.clips):
            cn = sum(len(ct.ids) for b in c.bones for ct in b.ctrls)
            print("  %3d %-24s %5d frames  %3d huesos  %6d claves" % (i, c.name, c.frames, len(c.bones), cn))
            if i > 60:
                print("  ...")
                break
    elif x.cmd == "catalog":
        import glob
        rows = []
        for d in sorted(glob.glob(os.path.join(x.raiz, "bc*"))):
            for f in sorted(glob.glob(os.path.join(d, "*.emb"))):
                cl = motion_clips(f)
                named = ["%d:%s" % (k, v) for k, v in sorted(cl.items()) if not v.lower().startswith("dummy")]
                rows.append((os.path.basename(d), os.path.basename(f), len(cl), ";".join(named)))
        with open(x.salida, "w", encoding="utf-8") as fh:
            fh.write("personaje\tfichero\tn_clips\tclips\n")
            for r in rows:
                fh.write("\t".join(str(v) for v in r) + "\n")
        print("OK -> %s (%d bancos, %d personajes)" % (x.salida, len(rows), len({r[0] for r in rows})))
    elif x.cmd == "csv":
        for k, v in sorted(motion_clips(x.emb).items()):
            print("%d,%s" % (k, v))
    elif x.cmd == "dump":
        ean = Ean(x.ean)
        rng = [x.clip] if x.clip is not None else range(len(ean.clips))
        out = []
        for i in rng:
            c = ean.clips[i]
            out.append({"indice": i, "nombre": c.name, "frames": c.frames,
                        "huesos": [{"hueso": b.name, "id": b.id, "indice_hueso": bi,
                                    "canales": [{"canal": ct.canal, "tipo": ct.tipo,
                                                 "ids": ct.ids, "valores": ct.vals.tolist(),
                                                 "_off_vals": ct.off_vals} for ct in b.ctrls]}
                                   for bi, b in enumerate(c.bones)]})
        json.dump({"origen": os.path.basename(x.ean), "endian": ean.endian, "clips": out},
                  open(x.salida, "w", encoding="utf-8"), indent=1)
        print("OK -> %s (%d clips)" % (x.salida, len(out)))
    elif x.cmd == "repack":
        ean = Ean(x.ean)
        j = json.load(open(x.json, encoding="utf-8"))
        for cj in j["clips"]:
            c = ean.clips[cj["indice"]]
            for bj in cj["huesos"]:
                b = c.bones[bj["indice_hueso"]]
                for ctj in bj["canales"]:
                    ct = next(t for t in b.ctrls if t.canal == ctj["canal"])
                    ct.vals = np.array(ctj["valores"], np.float64)
        ean.save(x.salida)
        same = open(x.salida, "rb").read() == bytes(ean.data)
        print("OK -> %s | identico al original: %s" % (x.salida, "si" if same else "no"))
    elif x.cmd == "to-b3":
        ean = Ean(x.ean)
        d = to_b3(ean, x.clip, x.plantilla, x.fps)
        json.dump(d, open(x.salida, "w", encoding="utf-8"), indent=1)
        print("OK -> %s | %d huesos B3, %d sin mapear" % (x.salida, len(d["huesos"]), len(d["sin_mapear"])))
    else:
        ap.print_help()
    return 0


class EanBytes(Ean):
    """#EAN que vive dentro de un #EMB: se analiza en memoria."""

    def __init__(self, data, path="<emb>"):
        import tempfile
        fd, tmp = tempfile.mkstemp(suffix=".ean")
        os.close(fd)
        open(tmp, "wb").write(data)
        super().__init__(tmp)
        self.path = path
        os.unlink(tmp)


if __name__ == "__main__":
    sys.exit(main())



