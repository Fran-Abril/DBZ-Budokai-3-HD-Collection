#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""xeno_cpk.py - catalogo de assets de Dragon Ball Xenoverse (CPK de CRI), solo lectura.

Xenoverse guarda TODO en `data.cpk` (+ data2/datap1..3, movie*): 36 276 entradas en `data.cpk`
(medido). Dentro estan los personajes y el creador (CaC):
  data/chara/HUM|HUF|NMC|FRI|MAM|MAF/...   piezas del CaC por raza (cuerpo, pelo, cara, ropa)
  data/chara/APL/...                        piezas del avatar (protagonista)
  data/chara/<CLAVE>/...                    personajes (GOK, VGT, ...)
y los formatos son los MISMOS que los de SDBH Heroes (familia #EMD/#ESK/#EMM/#EMB/#EAN):
  .emd geometria | .esk esqueleto | .emm material | .emb texturas | .ean animacion
(lo confirma LibXenoverse: https://github.com/DarioSamo/LibXenoverse).

OJO: los ficheros del CPK van comprimidos con CRILAYLA (los 8 primeros bytes son "CRILAYLA").
Sin descomprimir solo se puede LEER el indice (nombres y tamanos); `extraer` deja el blob tal
cual y lo avisa. No forma parte de este frente escribir un descompresor CRILAYLA.

  python xeno_cpk.py info <cpk...>
  python xeno_cpk.py catalogo <cpk> <salida.tsv>
  python xeno_cpk.py buscar <cpk> <regex>
  python xeno_cpk.py extraer <cpk> <ruta interna> <salida>
"""
import argparse
import collections
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
try:
    import cri  # noqa: E402
except ImportError:                     # prueba desde otra carpeta
    sys.path.insert(0, os.environ.get("DBZ3_AWO_TOOLS", HERE))
    import cri  # noqa: E402,F811


def _leer(cpk, nombre, size, off):
    with open(cpk, "rb") as f:
        f.seek(off)
        return f.read(size)


def cmd_info(cpks):
    for cpk in cpks:
        rows = cri.cpk_toc(cpk)
        ext = collections.Counter(n.rsplit(".", 1)[-1].lower() for n, s, o in rows)
        chara = collections.Counter()
        for n, s, o in rows:
            m = re.match(r"data/chara/([^/]+)/", n, re.I)
            if m:
                chara[m.group(1).upper()] += 1
        print("%s: %d entradas" % (os.path.basename(cpk), len(rows)))
        print("   extensiones: " + ", ".join("%s=%d" % (k, v) for k, v in ext.most_common(12)))
        print("   chara: " + ", ".join("%s=%d" % (k, v) for k, v in chara.most_common(12)))
        for pat in ("_Body", "_Hair", "_Face", "_Boots", "_Bust"):
            hit = [n for n, s, o in rows if pat.lower() in n.lower()]
            if hit:
                print("   %s: %d (p.ej. %s)" % (pat, len(hit), hit[0]))


def cmd_catalogo(cpk, out):
    rows = cri.cpk_toc(cpk)
    with open(out, "w", encoding="utf-8") as fh:
        fh.write("ruta\ttamano\toffset\tcomprimido\n")
        for n, s, o in rows:
            comp = _leer(cpk, n, 8, o) == b"CRILAYLA"
            fh.write("%s\t%d\t%d\t%s\n" % (n, s, o, "si" if comp else "no"))
    print("OK -> %s (%d entradas)" % (out, len(rows)))


def cmd_buscar(cpk, patron):
    rx = re.compile(patron, re.I)
    n = 0
    for ruta, s, o in cri.cpk_toc(cpk):
        if rx.search(ruta):
            print("%-70s %9d" % (ruta, s))
            n += 1
    print("%d coincidencias" % n)


def cmd_extraer(cpk, nombre, out):
    for ruta, s, o in cri.cpk_toc(cpk):
        if ruta.lower() == nombre.lower() or os.path.basename(ruta).lower() == nombre.lower():
            d = _leer(cpk, ruta, s, o)
            open(out, "wb").write(d)
            comp = " CRILAYLA (hay que descomprimir)" if d[:8] == b"CRILAYLA" else ""
            print("OK -> %s (%d B) %s%s" % (out, len(d), d[:4], comp))
            return 0
    print("no encontrado: %s" % nombre)
    return 1


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="cmd")
    a = sub.add_parser("info")
    a.add_argument("cpks", nargs="+")
    a = sub.add_parser("catalogo")
    a.add_argument("cpk")
    a.add_argument("salida")
    a = sub.add_parser("buscar")
    a.add_argument("cpk")
    a.add_argument("patron")
    a = sub.add_parser("extraer")
    a.add_argument("cpk")
    a.add_argument("nombre")
    a.add_argument("salida")
    x = ap.parse_args(argv)
    if x.cmd == "info":
        cmd_info(x.cpks)
    elif x.cmd == "catalogo":
        cmd_catalogo(x.cpk, x.salida)
    elif x.cmd == "buscar":
        cmd_buscar(x.cpk, x.patron)
    elif x.cmd == "extraer":
        return cmd_extraer(x.cpk, x.nombre, x.salida)
    else:
        ap.print_help()
    return 0


if __name__ == "__main__":
    sys.exit(main())
