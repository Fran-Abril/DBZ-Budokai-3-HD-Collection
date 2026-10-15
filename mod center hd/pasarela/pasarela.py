#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""pasarela.py - Pasarela de modos: lee de la ISO del jugador los datos de modos de otros juegos de la saga.

Modo 1: "La leyenda de Mr. Satan" (Dragon Ball Z Budokai 1, PS2). Lee la ISO tal cual (sin extraerla) y saca:
  - la tabla de los 11 combates del ELF de arranque (registros de 128 B, por firma: vale cualquier region);
  - el banco de charlas (#AMT de 22 imagenes de texto: charla de Mr. Satan + aviso del combate) en cada idioma;
  - la pantalla del modo (#AMT de 10: fondo, dibujo de Mr. Satan, logo).
El juego (src/pasarela_b1.cpp) hace lo mismo en tiempo de ejecucion; esta herramienta es la referencia
reproducible y sirve para revisar lo que se lee. Lo que escribe es de la copia del jugador: NUNCA va al repo.

  python pasarela.py info   [--iso ISO]                  # combates y bancos encontrados
  python pasarela.py extraer [--iso ISO] --salida DIR    # hercule.toml + PNG por idioma en DIR (para revisar)
  python pasarela.py prueba                              # autocomprobacion (con la ISO europea si esta)

Formato (docs/16_pasarela_modos/01_leyenda_mr_satan.md):
  registro (128 B): lado 1 +0x00 vec4, +0x10 vec4 posicion, +0x20 u8 personaje, +0x21 u8 variante, +0x22 u8 forma,
  +0x26 u16 cantidad, +0x28 s16, +0x2A u16 vida (0xFFFF = no se recupera), +0x2C u16 ki, +0x2E u16 n tecnicas,
  +0x30 n x u16 tecnicas; lado 2 igual desde +0x40 (sin el primer vec4: personaje en +0x50); +0x70 u8 modo
  (3 = Mr. Satan), +0x71 u8 escenario, +0x72 u8 musica (hipotesis), +0x74 s32 tiempo en cuadros (-1 sin limite).
"""
import argparse
import os
import struct
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))   # mod center hd/ (iso.py)
import iso as isomod  # noqa: E402

SATAN_B1 = 21          # ID de Mr. Satan en B1
MODO_SATAN = 3         # byte +0x70 de los registros del modo
REC = 128
N_COMBATES = 11

# B1 -> B3 (como importar.B1_DONOR: el personaje de B3 que hace de el)
B1_A_B3 = [0, 2, 3, 7, 10, 8, 11, 12, 13, 18, 19, 20, 21, 38, 19, 27, 28, 29, 30, 32, 33, 14, 42, 43, 5]
B1_NOMBRES = ["Goku", "Kid Gohan", "Teen Gohan", "Vegeta", "Krillin", "Trunks", "Piccolo", "Tien", "Yamcha",
              "Raditz", "Nappa", "Ginyu", "Recoome", "Zarbon", "Dodoria", "Frieza", "Android 16", "Android 17",
              "Android 18", "Android 19", "Cell", "Hercule", "Saibaman", "Cell Jr.", "Great Saiyaman"]
# escenario de B1 (nombre del registro, sin CHG_/HAK) -> entrada de data_cmn del mapa de B3 (el juego busca el
# registro de escenario de B3 con ese mapa: 0x82323760 + 92*i, +0x10)
ESCENARIO_B3 = {"CELL RING": 50, "NAMEC": 60, "RETTOU": 56, "SEISHIN": 58, "TENKAITI": 46, "BOKUSOUTI": 62,
                "IWAYAMA": 64, "SANKAN": 64, "SUMMER SAN": 62, "SHINDEN": 46, "TIKYUU": 68}
# idioma del juego -> sufijo del AFS de B1 (europea); la americana trae uno solo
IDIOMAS = {"es": "SP", "en": "EN", "fr": "FR", "de": "GE", "it": "IT"}


# ------------------------------------------------------------------ ISO y AFS
def buscar_iso():
    return isomod.find_game("b1")


def leer_elf(img):
    return img.read(img.boot_elf(), 0, img.files()[img.boot_elf()][1])


def tabla_satan(elf):
    """Offset de los 11 registros del modo (firma: personaje 21 en +0x20 y modo 3 en +0x70 en los 11)."""
    for p in range(0, len(elf) - REC * N_COMBATES, 16):
        if elf[p + 0x20] != SATAN_B1 or elf[p + 0x70] != MODO_SATAN:
            continue
        if all(elf[p + REC * k + 0x20] == SATAN_B1 and elf[p + REC * k + 0x70] == MODO_SATAN and
               elf[p + REC * k + 0x0F] == 0x3F for k in range(N_COMBATES)):
            return p
    return -1


def nombres_escenario(elf):
    """Registros de escenario de B1 (0x98 B, nombre al principio): se buscan por 'CELL RING'."""
    i = elf.find(b"CELL RING\x00")
    if i < 0:
        return []
    while i - 0x98 >= 0 and 3 <= len(elf[i - 0x98:i - 0x98 + 16].split(b"\0")[0]) and \
            all(32 <= c < 127 for c in elf[i - 0x98:i - 0x98 + 16].split(b"\0")[0]):
        i -= 0x98
    out = []
    while True:
        nm = elf[i:i + 16].split(b"\0")[0]
        if len(nm) < 2 or not all(32 <= c < 127 for c in nm):
            return out
        out.append(nm.decode())
        i += 0x98


def escenario_b3(nombre):
    n = nombre.replace("CHG_", "").replace(" HAK", "").strip()
    return ESCENARIO_B3.get(n, 50)


def combates(elf):
    p = tabla_satan(elf)
    if p < 0:
        raise SystemExit("no encuentro la tabla de la leyenda de Mr. Satan en el ELF")
    esc = nombres_escenario(elf)
    out = []
    for k in range(N_COMBATES):
        r = elf[p + REC * k:p + REC * (k + 1)]
        rival = r[0x50]
        cnt, = struct.unpack_from("<H", r, 0x56)
        vj, = struct.unpack_from("<H", r, 0x2A)
        vr, kr = struct.unpack_from("<HH", r, 0x5A)
        t, = struct.unpack_from("<i", r, 0x74)
        e = r[0x71]
        out.append({"rival_b1": rival, "rival": B1_A_B3[rival] if rival < len(B1_A_B3) else 0,
                    "nombre": B1_NOMBRES[rival] if rival < len(B1_NOMBRES) else "#%d" % rival,
                    "variante": r[0x51], "forma": r[0x52], "cantidad": cnt or 1,
                    "vida_jugador": vj, "arrastra_vida": vj == 0xFFFF, "vida_rival": vr, "ki_rival": kr,
                    "escenario_b1": e, "escenario_nombre": esc[e] if e < len(esc) else "",
                    "mapa_b3": escenario_b3(esc[e] if e < len(esc) else ""), "musica": r[0x72], "tiempo": t})
    return p, out


class Afs:
    def __init__(self, img, path):
        self.img, self.path = img, path
        n, = struct.unpack_from("<I", img.read(path, 4, 4))
        t = img.read(path, 8, 8 * n)
        self.tab = [struct.unpack_from("<II", t, 8 * k) for k in range(n)]

    def head(self, k, n=0x20):
        o, s = self.tab[k]
        return self.img.read(self.path, o, min(n, s)) if s else b""

    def entry(self, k):
        o, s = self.tab[k]
        return self.img.read(self.path, o, s)


def amt_n(h):
    return struct.unpack_from("<I", h, 0x10)[0] if h[:4] == b"#AMT" else -1


def banco_charlas(afs):
    """Indice del banco de charlas: #AMA, #AMT(10), #AMA, #AMT(26), #AMT(22) seguidos (en la europea, 665-669)."""
    def ok(k):
        if k < 4 or k >= len(afs.tab):
            return False
        h = [afs.head(k - j) for j in (4, 3, 2, 1, 0)]
        return (h[0][:4] == b"#AMA" and amt_n(h[1]) == 10 and h[2][:4] == b"#AMA" and amt_n(h[3]) == 26 and
                amt_n(h[4]) == 22)
    if ok(669):
        return 669
    for k in range(4, len(afs.tab)):
        if ok(k):
            return k
    return -1


# ------------------------------------------------------------------ #AMT (PS2) -> RGBA
def amt_texturas(amt):
    n, idx = struct.unpack_from("<II", amt, 0x10)
    out = []
    for k in range(n):
        o, = struct.unpack_from("<I", amt, idx + 4 * k)
        if not o:
            out.append(None)
            continue
        e = struct.unpack_from("<3I2H2H7I", amt, o)
        out.append({"psm": e[2], "w": e[5], "h": e[6], "data": e[7], "dsize": e[8], "clut": e[11], "csize": e[12]})
    return out


def amt_rgba(amt, t):
    """Pixeles lineales (4 bpp: nibble bajo = pixel par), CLUT RGBA con alfa 0..0x80 (x2); la de 8 bpp en CSM1."""
    w, h = t["w"], t["h"]
    n = t["csize"] // 4
    c = amt[t["clut"] + 0x20:t["clut"] + 0x20 + 4 * n]
    pal = [bytes((c[4 * i], c[4 * i + 1], c[4 * i + 2], min(255, c[4 * i + 3] * 2))) for i in range(n)]
    if n == 256:
        pal = [pal[(i & ~0x18) | ((i & 0x08) << 1) | ((i & 0x10) >> 1)] for i in range(256)]
    raw = amt[t["data"] + 0x20:t["data"] + 0x20 + t["dsize"]]
    out = bytearray(w * h * 4)
    for i in range(w * h):
        if t["psm"] == 0x14:
            b = raw[i >> 1] if (i >> 1) < len(raw) else 0
            v = (b >> 4) if i & 1 else (b & 0xF)
        else:
            v = raw[i] if i < len(raw) else 0
        out[4 * i:4 * i + 4] = pal[v] if v < len(pal) else b"\0\0\0\0"
    return bytes(out)


def png(path, w, h, rgba):
    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    rows = b"".join(b"\0" + rgba[y * w * 4:(y + 1) * w * 4] for y in range(h))
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)) +
                chunk(b"IDAT", zlib.compress(rows, 6)) + chunk(b"IEND", b""))


def afs_idioma(img, lang):
    p = img.find("DATA_%s.AFS" % IDIOMAS.get(lang, "EN"))
    if p:
        return p
    for suf in ("EN", "US", "UK"):
        p = img.find("DATA_%s.AFS" % suf) or p
    return p or next((f for f in img.files() if f.upper().endswith(".AFS") and "/DATA" in f.upper()), None)


# ------------------------------------------------------------------ ordenes
def info(iso_path):
    img = isomod.Iso(iso_path)
    elf = leer_elf(img)
    p, cs = combates(elf)
    print("ISO %s, ELF %s, tabla en 0x%X" % (os.path.basename(iso_path), img.boot_elf(), p))
    for k, c in enumerate(cs):
        print("  %2d %-11s -> B3 %2d  forma %d  x%d  vida %s/%d  ki %d  escenario %d %s  tiempo %d" % (
            k + 1, c["nombre"], c["rival"], c["forma"], c["cantidad"],
            "arrastra" if c["arrastra_vida"] else c["vida_jugador"], c["vida_rival"], c["ki_rival"],
            c["escenario_b1"], c["escenario_nombre"], c["tiempo"]))
    for lang in IDIOMAS:
        path = afs_idioma(img, lang)
        if path:
            afs = Afs(img, path)
            print("  %s: %s banco de charlas %d" % (lang, path, banco_charlas(afs)))
    return cs


def extraer(iso_path, salida):
    img = isomod.Iso(iso_path)
    elf = leer_elf(img)
    p, cs = combates(elf)
    os.makedirs(salida, exist_ok=True)
    hechos = set()
    for lang in IDIOMAS:
        path = afs_idioma(img, lang)
        if not path or path in hechos:
            continue
        hechos.add(path)
        afs = Afs(img, path)
        k = banco_charlas(afs)
        if k < 0:
            print("  %s: sin banco de charlas" % lang)
            continue
        d = os.path.join(salida, "hercule", lang)
        os.makedirs(d, exist_ok=True)
        for idx, nombre in ((k, "charla"), (k - 3, "pantalla")):
            amt = afs.entry(idx)
            for j, t in enumerate(amt_texturas(amt)):
                if t and (nombre == "charla" or j in (0, 1, 2)):
                    png(os.path.join(d, "%s_%02d.png" % (nombre, j)), t["w"], t["h"], amt_rgba(amt, t))
        print("  %s: banco %d -> %s" % (lang, k, d))
    with open(os.path.join(salida, "hercule.toml"), "w", encoding="utf-8") as f:
        f.write("# La leyenda de Mr. Satan (Budokai 1), leida de %s (tabla 0x%X del ELF). Solo para revisar:\n"
                "# es de la copia del jugador, no va al repositorio. El juego lee la ISO directamente.\n"
                % (os.path.basename(iso_path), p))
        f.write('modo = "hercule"\njuego = "b1"\njugador = 14\n\n')
        for k, c in enumerate(cs):
            f.write("[[combate]]  # %d: %s\n" % (k + 1, c["nombre"]))
            for key in ("rival", "rival_b1", "forma", "variante", "cantidad", "vida_rival", "ki_rival", "mapa_b3",
                        "escenario_b1", "musica", "tiempo"):
                f.write("%s = %d\n" % (key, c[key]))
            f.write('escenario_nombre = "%s"\narrastra_vida = %s\n' % (c["escenario_nombre"],
                                                                        str(c["arrastra_vida"]).lower()))
            if k:
                f.write('charla = "charla_%02d.png"\naviso = "charla_%02d.png"\n' % (2 * k - 2, 2 * k - 1))
            f.write("\n")
    print("escrito %s" % os.path.join(salida, "hercule.toml"))


def prueba():
    # registro sintetico: la firma y la lectura de campos
    r = bytearray(REC * N_COMBATES)
    for k in range(N_COMBATES):
        o = REC * k
        r[o + 0x0F] = 0x3F
        r[o + 0x20] = SATAN_B1
        r[o + 0x70] = MODO_SATAN
        r[o + 0x50] = [8, 7, 4, 16, 5, 3, 6, 23, 0, 2, 20][k]
        struct.pack_into("<H", r, o + 0x56, 5 if k == 7 else 1)
        struct.pack_into("<H", r, o + 0x2A, 0xFFFF if k in (2, 9) else 3000)
        struct.pack_into("<i", r, o + 0x74, 3600 if k == 5 else 5940)
    elf = bytes(64) + bytes(r) + b"CELL RING\x00"
    p, cs = combates(elf)
    assert p == 64, p
    assert [c["rival"] for c in cs] == [13, 12, 10, 28, 8, 7, 11, 43, 0, 3, 33], [c["rival"] for c in cs]
    assert cs[7]["cantidad"] == 5 and cs[2]["arrastra_vida"] and cs[5]["tiempo"] == 3600
    # 8 bpp: CSM1 cambia 8-15 <-> 16-23
    t = {"psm": 0x13, "w": 1, "h": 1, "data": 0, "dsize": 1, "clut": 0x40, "csize": 1024}
    amt = bytearray(0x40 + 0x20 + 1024)
    amt[0x20] = 8                                       # pixel = 8 -> entrada 16 de la CLUT guardada
    amt[0x60 + 16 * 4:0x60 + 16 * 4 + 4] = b"\x11\x22\x33\x80"
    assert amt_rgba(bytes(amt), t) == b"\x11\x22\x33\xff"
    print("prueba sintetica OK")
    iso_path = buscar_iso()
    if iso_path:
        cs = info(iso_path)
        assert len(cs) == N_COMBATES and cs[0]["rival_b1"] == 8 and cs[10]["rival_b1"] == 20, cs
        print("prueba con la ISO OK")
    else:
        print("(sin ISO de Budokai 1 en ps2_games: solo la prueba sintetica)")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("orden", choices=["info", "extraer", "prueba"])
    ap.add_argument("--iso")
    ap.add_argument("--salida")
    a = ap.parse_args(argv)
    if a.orden == "prueba":
        return prueba()
    iso_path = a.iso or buscar_iso()
    if not iso_path:
        raise SystemExit("no hay ISO de Budokai 1 (ponla en ps2_games o usa --iso)")
    if a.orden == "info":
        info(iso_path)
    else:
        if not a.salida:
            raise SystemExit("falta --salida")
        extraer(iso_path, a.salida)


if __name__ == "__main__":
    main()
