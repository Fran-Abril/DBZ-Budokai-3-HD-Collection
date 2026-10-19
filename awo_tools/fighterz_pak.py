#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""fighterz_pak.py - lector minimo del .pak de Dragon Ball FighterZ (UE4, solo lectura).

FighterZ guarda todo en RED/Content/Paks/pakchunkN-WindowsNoEditor.pak (UE4 **pak v4**, con el
**indice cifrado** [V: los bytes del indice no son texto]; sin la clave AES solo se puede leer la
cabecera FPakInfo). El indice esta al final: huella FPakInfo de 44 B
  magic 0x5A6F12E1, version(4), index_offset(8), index_size(8), sha1(20)

    info(pak)             -> {version, index_offset, index_size}
    names(pak)            -> [ruta interna]  (sin descomprimir nada)
    find(pak, patron)     -> [rutas que casan]
    read(pak, nombre)     -> bytes del fichero (descomprime Zlib si hace falta)
    mount(paks)           -> {ruta: (pak, entrada)}

Solo lee. No extrae texturas ni ejecuta nada del juego.
"""
import os
import re
import struct
import zlib

MAGIC = 0x5A6F12E1


class Entry:
    __slots__ = ("offset", "size", "usize", "method", "blocks")

    def __init__(self, offset, size, usize, method, blocks):
        self.offset, self.size, self.usize, self.method, self.blocks = offset, size, usize, method, blocks


def _fstring(b, p):
    n = struct.unpack_from("<i", b, p)[0]
    p += 4
    if n == 0:
        return "", p
    if n > 0:
        s = b[p:p + n - 1].decode("latin1", "replace")
        return s, p + n
    n = -n
    s = b[p:p + 2 * (n - 1)].decode("utf-16-le", "replace")
    return s, p + 2 * n


def info(pak):
    with open(pak, "rb") as f:
        f.seek(-44, os.SEEK_END)
        magic, ver, off, size = struct.unpack("<IIQQ", f.read(24))
    if magic != MAGIC:
        raise ValueError("no es un .pak UE4: %s" % pak)
    return {"version": ver, "index_offset": off, "index_size": size}


class Cifrado(Exception):
    """El indice del .pak viene cifrado (AES): sin clave solo se puede leer la cabecera."""


def _index(pak):
    inf = info(pak)
    with open(pak, "rb") as f:
        f.seek(inf["index_offset"])
        b = f.read(inf["index_size"])
    mount, p = _fstring(b, 0)
    if not mount.startswith("/"):
        raise Cifrado("%s: el indice esta cifrado o no es el de un pak de UE4 v%d (solo cabecera)"
                      % (pak, inf["version"]))
    n = struct.unpack_from("<i", b, p)[0]
    p += 4
    if not (0 <= n <= 4000000):
        raise Cifrado("%s: %d entradas en el indice (cifrado o formato distinto)" % (pak, n))
    out = {}
    ver = inf["version"]
    for _ in range(n):
        name, p = _fstring(b, p)
        if not name.startswith("/") or p + 28 > len(b):
            raise Cifrado("%s: indice no interpretable a partir de %d (cifrado o v%d no soportada)"
                          % (pak, p, ver))
        off, size, usize, method = struct.unpack_from("<qqqi", b, p)
        p += 8 + 8 + 8 + 4
        p += 8 + 20                       # timestamp + sha1
        blocks = []
        if method != 0:
            nb = struct.unpack_from("<i", b, p)[0]
            p += 4
            blocks = [struct.unpack_from("<qq", b, p + 16 * i) for i in range(nb)]
            p += 16 * nb
        if ver >= 8:                      # flags + sha1 de bloques (no en v4)
            p += 1
        out[name] = Entry(off, size, usize, method, blocks)
    return mount, out


def names(pak):
    mount, idx = _index(pak)
    return [mount + k for k in idx]


def find(pak, patron):
    """Rutas internas que casan (lanza Cifrado si el indice esta cifrado)."""
    rx = re.compile(patron, re.I)
    return [n for n in names(pak) if rx.search(n)]


def read(pak, nombre, mount=""):
    mount, idx = _index(pak)
    key = nombre[len(mount):] if mount and nombre.startswith(mount) else nombre
    e = idx[key]
    with open(pak, "rb") as f:
        f.seek(e.offset)
        head = f.read(e.size)
    # la cabecera del dato repite offset/size/usize/method/sha1 (v4); el bloque empieza tras ella
    _, _, _, method = struct.unpack_from("<qqqi", head, 0)
    p = 8 + 8 + 8 + 4 + 8 + 20
    if method == 0:
        return head[p:p + e.usize]
    nb = struct.unpack_from("<i", head, p)[0]
    p += 4 + 16 * nb
    out = bytearray()
    for _ in range(nb):
        csize = struct.unpack_from("<q", head, p)[0]
        p += 8
        out += zlib.decompress(head[p:p + csize])
        p += csize
    return bytes(out)


def mount(paks):
    out = {}
    for pak in paks:
        for n in names(pak):
            out.setdefault(n, pak)
    return out


def _cli(paquetes):
    """Cabecera de cada .pak y, si el indice se puede leer, cuantas rutas internas tiene."""
    for p in paquetes:
        inf = info(p)
        print("%s: UE4 pak v%d, indice %.1f MB en 0x%X" % (os.path.basename(p), inf["version"],
                                                           inf["index_size"] / 1e6, inf["index_offset"]))
        try:
            nm = names(p)
            print("   %d ficheros; p.ej. %s" % (len(nm), nm[0]))
        except Cifrado as e:
            print("   AVISO: %s" % e)


if __name__ == "__main__":
    import sys
    _cli(sys.argv[1:])
