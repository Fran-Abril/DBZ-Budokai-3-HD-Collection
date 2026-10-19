#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""fighterz_moves.py - frame data de Dragon Ball FighterZ y tabla de adaptacion a B3 HD.

Tres cosas:

  1. `frame`   vuelca a TSV el frame data de Goku (base) de FighterZ tal como lo publica la
               comunidad de Dustloop (el juego NO lo trae en ficheros de texto: vive en los
               assets UE4 y en el visor de frame data del modo Entrenamiento).
  2. `b3`      vuelca a TSV el frame data REAL de B3 HD leido de nuestra copia del juego
               (`studies/_data/frame_data/0_0.json`, generado por `studies/tools/frame_data.py`).
  3. `mapa`    imprime la tabla de adaptacion movimiento de FighterZ -> hueco en un moveset de
               B3 (attack code / entrada / coste), cruzando las dos anteriores.

Todo son datos leidos; no ejecuta ni modifica nada del juego.

Fuentes (URL):
  * Dustloop, DBFZ/Goku/Frame Data: https://www.dustloop.com/w/DBFZ/Goku/Frame_Data
  * (los valores de B3 salen de `data_cmn.afs` de la copia del usuario: son nuestros)
"""
import argparse
import json
import os
import sys

# --- FighterZ: Goku (base). dmg, guardia, arranque, activo, recuperacion, onBlock (Dustloop)
FIGHTERZ = [
    # input, nombre, dmg, guar, arranque, activo, recup, onBlock, notas
    ("5L", "puñetazo ligero", "400", "All", 6, 3, 13, -4, "jab universal"),
    ("5LL", "segundo ligero", "700", "All", 7, 3, 18, -5, "lanza"),
    ("5LLL", "tercer ligero", "1000", "All", 15, 3, 26, -4, "smash U3+"),
    ("5M", "medio", "700", "All", 7, 3, 18, -5, "corta distancia"),
    ("5H", "fuerte", "850", "All", 12, 4, 17, -5, "smash U1, wallsplat"),
    ("5S", "ki 1~6", "300x6", "All", 13, 25, None, -4, "proyectiles"),
    ("2L", "ligero bajo", "400", "All", 6, 2, 14, 0, "no es low"),
    ("2M", "medio bajo", "700", "Low", 10, 6, 15, -5, "low, lanza"),
    ("2H", "antiareas", "850", "All", 14, 3, 30, -17, "invulnerable de cabeza 4-15"),
    ("2S", "ki bajo", "700", "All", 15, None, 43, -8, "total 43"),
    ("6M", "overhead", "850", "High", 24, 6, None, 0, "Overhead, 4+6L"),
    ("j.L", "aereo L", "400", "High", 6, 3, 11, None, ""),
    ("j.M", "aereo M", "700", "High", 10, 3, 16, None, ""),
    ("j.H", "aereo H", "850/1000", "High", 13, 4, 21, None, "smash D1+"),
    ("j.2H", "aereo antiareas", "850", "High", 11, 4, 23, None, "smash U1+"),
    ("236L", "codazo L", "900", "All", 10, 6, 19, -5, "avanza"),
    ("236M", "codazo M", "1100", "All", 26, 8, 20, 2, "smash U1+"),
    ("236H", "codazo H", "1300", "All", 18, 8, 20, 2, "smash U1+"),
    ("236S", "Kamehameha", "1000", "All", 18, 17, 18, -9, "haz"),
    ("214L", "agarre L", "700", "Throw", 20, 1, 25, None, "agarre por detras"),
    ("214M", "agarre M", "700,850", "Throw", 22, 8, 25, None, "smash U2, D1"),
    ("214H", "agarre H", "700,1000", "Throw", 18, 8, 19, None, "smash U1+"),
    ("214S", "Genki Dama (carga)", "Total 41~89", "All", 21, None, None, 3, "se carga con 214S"),
    ("236L+M", "Kaioken", "9+0", "All", None, None, None, None, "super: activa 4-9 All"),
    ("214L+M", "Súper Genki Dama", "60xN,5200", "All", 8, None, 96, -11, "invulnerable 1-final"),
]

# --- B3 HD: codigos de Goku leidos del #CSK (arranque = primer cuadro activo, dano normal)
B3_GOKU = {
    "200": ("P", 8, 30), "201": ("P, P", 11, 30), "202": ("P, P, P", 11, 40),
    "203": ("P, P, P, P", 9, 35), "205": ("P, P, P, >P", 11, 30), "210": ("K", 11, 50),
    "211": ("K, K", 11, 50), "212": ("K, K, K", 11, 40), "213": ("K, K, K, K", 9, 40),
    "21B": (">P", 9, 35), "21C": (">P, P", 9, 35), "226": (">K", 10, 65), "231": ("<P", 11, 60),
    "238": ("<K", 15, 30), "209": ("P, P, K, K, K", 13, 75), "20F": ("P, K", 14, 60),
    "26D": ("<E", 45, 30), "26E": ("<E, E", 11, 50), "271": ("<E, E, E, E, >E", 67, 160),
    "257": ("P+G", 10, 0), "272": (">K+G", 13, 85), "259": ("P+K+G+E (en hiper)", 30, 0),
}

# --- adaptacion: movimiento de FighterZ -> hueco en un moveset de B3
#     (code, entrada, dano B3, coste, nota). coste: 0 existe / S retocar / M nueva animacion /
#     L pide motor (SPX, propiedades nuevas) / X no existe en B3
MAPA = [
    ("5L", "200", "P", 30, "0", "el jab ya existe; solo igualar daño"),
    ("5LL", "201", "P, P", 30, "0", "segundo golpe de la cadena de P"),
    ("5LLL", "202", "P, P, P", 40, "0", "tercer golpe"),
    ("5M", "210", "K", 50, "0", "patada media (cadena de K)"),
    ("5H", "226", ">K", 65, "0", "patada fuerte adelante"),
    ("5S", "26D", "<E", 30, "S", "la onda de ki existe; los proyectiles 1~6 son #SPX (L)"),
    ("2L", "238", "<K", 30, "0", "patada baja"),
    ("2M", "231", "<P", 60, "0", "golpe bajo"),
    ("2H", "209", "P, P, K, K, K", 75, "0", "lanzador (stun tipo 2 = knockaway)"),
    ("6M", "-", "-", 0, "X", "B3 no tiene overhead como propiedad de AP: pide motor"),
    ("j.L/j.M/j.H", "300/310/326", "misma entrada en el bloque 0x3xx", 0, "M",
     "el bloque 0x3xx repite entradas/daño del 0x2xx: parece la version aerea [H]"),
    ("236L/M/H", "272", ">K+G", 85, "S", "un solo codigo para los tres; el H pediria animacion propia"),
    ("236S", "-", "técnica Kamehameha", 0, "M", "en B3 las tecnicas son capsulas + animacion, no #CSK"),
    ("214L/M/H", "257", "P+G", 0, "0", "agarre scripted (tipo 3) ya existe"),
    ("214S", "-", "técnica (Genki Dama)", 0, "M", "tecnica + SPX de carga"),
    ("214L+M", "259", "P+K+G+E (en hiper)", 0, "M", "definitiva scripted; necesita escena y animacion"),
    ("Asistencias A/B/C", "-", "Tag Team", 0, "S", "el Tag Team de la ronda 4 ya llama a un companero"),
]


def b3_buscar(json_path, code):
    """(entrada, arranque, dano) de un codigo en el JSON de frame_data de B3."""
    if not json_path or not os.path.isfile(json_path):
        return None
    d = json.load(open(json_path, encoding="utf-8"))
    for x in d.get("datos", []):
        if x.get("codigo", "").upper() == code.upper():
            g = x["golpes"][0] if x["golpes"] else {}
            return x.get("entrada", ""), g.get("cuadros", [None])[0], g.get("dano", {}).get("normal")
    return None


def cmd_frame(out):
    with open(out, "w", encoding="utf-8") as fh:
        fh.write("input\tnombre\tdano\tguardia\tarranque\tactivo\trecuperacion\tonBlock\tnotas\n")
        for r in FIGHTERZ:
            fh.write("\t".join("" if v is None else str(v) for v in r) + "\n")
    print("OK -> %s (%d movimientos)" % (out, len(FIGHTERZ)))


def cmd_b3(out):
    with open(out, "w", encoding="utf-8") as fh:
        fh.write("codigo\tentrada\tarranque\tdano\n")
        for c, (e, a, d) in sorted(B3_GOKU.items()):
            fh.write("%s\t%s\t%d\t%d\n" % (c, e, a, d))
    print("OK -> %s (%d codigos)" % (out, len(B3_GOKU)))


def cmd_mapa(out, b3json=None):
    with open(out, "w", encoding="utf-8") as fh:
        fh.write("fighterz\tb3_codigo\tb3_entrada\tb3_dano\tb3_arranque\tcoste\tnota\n")
        print("%-13s %-6s %-22s %5s %4s %2s  %s" % ("FighterZ", "code", "entrada B3", "daño", "arr", "C", "nota"))
        for fm, code, ent, dmg, cost, nota in MAPA:
            a = ""
            if code not in ("-", ""):
                info = b3_buscar(b3json, code) if b3json else None
                if info:
                    ent, a, dmg = info[0], info[1], info[2]
                elif code in B3_GOKU:
                    ent, a, dmg = B3_GOKU[code][0], B3_GOKU[code][1], B3_GOKU[code][2]
            fh.write("%s\t%s\t%s\t%s\t%s\t%s\t%s\n" % (fm, code, ent, dmg, a, cost, nota))
            print("%-13s %-6s %-22s %5s %4s %2s  %s" % (fm, code, ent, dmg, a, cost, nota))
    print("OK -> %s" % out)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="cmd")
    a = sub.add_parser("frame")
    a.add_argument("salida")
    a = sub.add_parser("b3")
    a.add_argument("salida")
    a = sub.add_parser("mapa")
    a.add_argument("salida")
    a.add_argument("--b3-json", help="studies/_data/frame_data/0_0.json (frame data real de B3)")
    x = ap.parse_args(argv)
    if x.cmd == "frame":
        cmd_frame(x.salida)
    elif x.cmd == "b3":
        cmd_b3(x.salida)
    elif x.cmd == "mapa":
        cmd_mapa(x.salida, x.b3_json)
    else:
        ap.print_help()
    return 0


if __name__ == "__main__":
    sys.exit(main())
