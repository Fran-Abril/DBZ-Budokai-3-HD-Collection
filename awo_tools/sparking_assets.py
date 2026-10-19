#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""sparking_assets.py - radiografia de una instalacion moddeada de Sparking! ZERO (UE5).

No lee assets (son UE5 empaquetados en IoStore: .utoc/.ucas + .pak, y no se instalan
herramientas de terceros): lo que hace es describir COMO esta MODEADO y sacar del ejecutable
las cadenas de interes (motor grafico, camara, creador de personaje, carga de mods), que es lo
aprovechable para nosotros como idea.

  python sparking_assets.py info <carpeta del juego>
  python sparking_assets.py mods <carpeta del juego> <salida.tsv>
  python sparking_assets.py strings <exe> [clave...]

Todo es lectura; no ejecuta el juego ni sus addons.
"""
import argparse
import json
import os
import re
import sys

CLAVES = ("Lumen", "Nanite", "RayTracing", "CineCamera", "CameraRig", "PostProcess", "PhotoMode",
          "CustomBattle", "CharacterCreate", "PoseableMesh", "ModLoader", "Unverum", "IoStore")


def ue_strings(exe, claves=CLAVES, max_hits=4):
    b = open(exe, "rb").read()
    out = {}
    for k in claves:
        hits = []
        for m in re.finditer(re.escape(k).encode("utf-16-le"), b):
            s, e = max(0, m.start() - 90), min(len(b), m.end() + 90)
            t = b[s:e].decode("utf-16-le", "ignore").replace("\x00", " ")
            t = re.sub(r"\s+", " ", t).strip()
            if t and t not in hits:
                hits.append(t)
            if len(hits) >= max_hits:
                break
        if hits:
            out[k] = hits
    return out


def cmd_info(root):
    paks = os.path.join(root, "SparkingZERO", "Content", "Paks")
    print("== paks ==")
    if os.path.isdir(paks):
        for f in sorted(os.listdir(paks)):
            p = os.path.join(paks, f)
            if os.path.isfile(p) and f.lower().endswith((".pak", ".utoc", ".ucas", ".sig")):
                print("   %-38s %12d" % (f, os.path.getsize(p)))
    for extra in ("SparkingZERO/Mods", "Unverum/Mods", "ModsBackup", "SaveGame"):
        d = os.path.join(root, *extra.split("/"))
        if os.path.isdir(d):
            n = sum(len(fs) for _, _, fs in os.walk(d))
            print("   %-38s %d ficheros" % (extra, n))
    cfg = os.path.join(root, "Unverum", "Config.json")
    if os.path.isfile(cfg):
        try:
            d = json.load(open(cfg, encoding="utf-8", errors="replace"))
            cur = d.get("CurrentGame")
            c = (d.get("Configs") or {}).get(cur, {})
            print("== Unverum ==  juego actual: %s | mods activos: %s" % (cur, c.get("ModList")))
        except ValueError as e:
            print("   Config.json ilegible: %s" % e)
    exe = os.path.join(root, "SparkingZERO", "Binaries", "Win64", "SparkingZERO-Win64-Shipping.exe")
    if os.path.isfile(exe):
        print("== motor (cadenas del exe) ==")
        for k, hs in ue_strings(exe).items():
            print("   %-14s %s" % (k, hs[0][:110]))


def cmd_mods(root, out):
    d = os.path.join(root, "Unverum", "Config.json")
    rows = []
    if os.path.isfile(d):
        j = json.load(open(d, encoding="utf-8", errors="replace"))
        for juego, c in (j.get("Configs") or {}).items():
            for m in (c.get("ModList") or []):
                rows.append((juego, m if isinstance(m, str) else json.dumps(m, ensure_ascii=False)))
    mdir = os.path.join(root, "SparkingZERO", "Mods")
    if os.path.isdir(mdir):
        for name in sorted(os.listdir(mdir)):
            rows.append(("SparkingZERO/Mods", name))
    with open(out, "w", encoding="utf-8") as fh:
        fh.write("juego/carpeta\tmod\n")
        for a, b in rows:
            fh.write("%s\t%s\n" % (a, b))
    print("OK -> %s (%d mods)" % (out, len(rows)))


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="cmd")
    a = sub.add_parser("info")
    a.add_argument("raiz")
    a = sub.add_parser("mods")
    a.add_argument("raiz")
    a.add_argument("salida")
    a = sub.add_parser("strings")
    a.add_argument("exe")
    a.add_argument("claves", nargs="*", default=list(CLAVES))
    x = ap.parse_args(argv)
    if x.cmd == "info":
        cmd_info(x.raiz)
    elif x.cmd == "mods":
        cmd_mods(x.raiz, x.salida)
    elif x.cmd == "strings":
        for k, hs in ue_strings(x.exe, x.claves).items():
            print("== %s" % k)
            for h in hs:
                print("   " + h[:160])
    else:
        ap.print_help()
    return 0


if __name__ == "__main__":
    sys.exit(main())
