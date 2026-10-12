#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""novas_mods.py - Nova's Mods: el repo privado de mods que descarga el launcher.

  python tools/novas_mods.py clave
      Pide la clave de solo lectura del repo (fine-grained, Contents: read) y escribe
      novas_mods.key en la raiz del proyecto (make_release la copia junto al exe). La clave
      va en hex con XOR 0x5A, como la lee src/launcher/novas_mods.cpp.

  python tools/novas_mods.py publicar [--mods DIR] [--solo id ...]
      Empaqueta los mods de tools/novas_mods.toml (carpeta mods/ del juego), escribe la
      version en su manifest.txt, genera catalogo.toml y lo sube al repo con git.
"""
import argparse
import getpass
import os
import shutil
import subprocess
import sys
import tomllib
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPO = "novapowers0/DBZ3HD-Novas-Mods"
LIST = os.path.join(ROOT, "tools", "novas_mods.toml")
KEY = os.path.join(ROOT, "novas_mods.key")
WORK = os.path.join(ROOT, "novas_mods_repo")
MODS = os.path.join(ROOT, "out", "build", "win-amd64-release", "mods")


def obfuscate(token):
    return "".join("%02x" % (ord(c) ^ 0x5A) for c in token)


def reveal(hexs):
    return "".join(chr(int(hexs[i:i + 2], 16) ^ 0x5A) for i in range(0, len(hexs) - 1, 2))


def cmd_clave(_a):
    # primero el portapapeles (copiada de donde sea), si no se pide a mano
    t = open(KEY).read().strip() if os.path.isfile(KEY) else ""     # pegada tal cual en el fichero
    if not t.startswith("github_pat_"):
        t = subprocess.run(["powershell", "-NoProfile", "-Command", "Get-Clipboard"], capture_output=True,
                           text=True).stdout.strip()
    if not t.startswith("github_pat_"):
        t = getpass.getpass("Clave de solo lectura del repo de mods (no se ve al escribir): ").strip()
    if not t.startswith("github_pat_"):
        sys.exit("Eso no parece una clave fine-grained (github_pat_...).")
    open(KEY, "w").write(obfuscate(t))
    print("Clave guardada en", KEY)


def manifest_with_version(path, version, name):
    lines = open(path, encoding="utf-8").read().splitlines() if os.path.isfile(path) else ["name=" + name]
    lines = [ln for ln in lines if not ln.startswith("version=")] + ["version=" + version]
    return "\n".join(lines) + "\n"


def pack(src, mod_id, version, name, out_zip):
    """Zip con la carpeta <id>/ (InstallModFromZip la desenvuelve); el manifest lleva la version."""
    with zipfile.ZipFile(out_zip, "w", zipfile.ZIP_DEFLATED) as z:
        for d, _, fs in os.walk(src):
            for f in sorted(fs):
                p = os.path.join(d, f)
                rel = os.path.relpath(p, src).replace("\\", "/")
                if rel == "manifest.txt" or rel == ".disabled" or f.endswith((".bak", ".log")):
                    continue
                z.write(p, mod_id + "/" + rel)
        z.writestr(mod_id + "/manifest.txt", manifest_with_version(os.path.join(src, "manifest.txt"), version, name))


def toml_str(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def cmd_publicar(a):
    cfg = tomllib.load(open(LIST, "rb"))
    if not os.path.isdir(os.path.join(WORK, ".git")):
        subprocess.run(["gh", "repo", "clone", REPO, WORK], check=True)
    os.makedirs(os.path.join(WORK, "mods"), exist_ok=True)
    out = ["# Nova's Mods: lo lee el launcher (src/launcher/novas_mods.cpp). Generado por tools/novas_mods.py.", ""]
    for m in cfg["mod"]:
        mid = m["id"]
        src = os.path.join(a.mods, mid)
        if not os.path.isdir(src) and os.path.isdir(src + ".disabled"):
            src += ".disabled"
        zrel = "mods/%s.zip" % mid
        zpath = os.path.join(WORK, zrel)
        if not a.solo or mid in a.solo:
            if not os.path.isdir(src):
                sys.exit("No encuentro el mod %s en %s" % (mid, a.mods))
            pack(src, mid, m["version"], m["nombre_es"], zpath)
            print("empaquetado %s v%s (%.1f MB)" % (mid, m["version"], os.path.getsize(zpath) / 1048576))
        out.append("[[mod]]")
        for k in ("id", "version", "tipo"):
            out.append("%s = %s" % (k, toml_str(m[k])))
        out.append("zip = " + toml_str(zrel))
        out.append("size = %d" % os.path.getsize(zpath))
        for k, v in m.items():
            if k.startswith(("nombre_", "descripcion_")):
                out.append("%s = %s" % (k, toml_str(v)))
        out.append("")
    open(os.path.join(WORK, "catalogo.toml"), "w", encoding="utf-8", newline="\n").write("\n".join(out))
    tomllib.load(open(os.path.join(WORK, "catalogo.toml"), "rb"))     # se lee bien
    git = ["git", "-C", WORK]
    subprocess.run(git + ["add", "-A"], check=True)
    if subprocess.run(git + ["diff", "--cached", "--quiet"]).returncode:
        subprocess.run(git + ["commit", "-q", "-m", a.mensaje], check=True)
        subprocess.run(git + ["push", "-q", "origin", "HEAD"], check=True)
        print("subido a", REPO)
    else:
        print("sin cambios")


def prueba():
    assert reveal(obfuscate("github_pat_X1")) == "github_pat_X1"
    assert "version=2" in manifest_with_version("/no/existe", "2", "A")
    print("novas_mods OK")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("clave")
    p = sub.add_parser("publicar")
    p.add_argument("--mods", default=MODS)
    p.add_argument("--solo", nargs="*")
    p.add_argument("--mensaje", default="Actualiza Nova's Mods")
    sub.add_parser("prueba")
    a = ap.parse_args()
    {"clave": cmd_clave, "publicar": cmd_publicar, "prueba": lambda _a: prueba()}[a.cmd](a)


if __name__ == "__main__":
    main()
