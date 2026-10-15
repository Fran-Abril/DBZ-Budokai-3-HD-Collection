#!/usr/bin/env python3
r"""ranking.py - Ranking Elo por personaje desde combates.csv (el juego anota cada combate en user_data\dbz3).

  python tools\ranking.py D:\DBZ3HD\modos_test\user_data\dbz3\combates.csv
  python tools\ranking.py RUTA\combates.csv --csv ranking.csv
  python tools\ranking.py --selftest

Mismo calculo que el del juego (src/modos.cpp, dbz3::modos::Ranking): K = 32, todos empiezan en 1000, los
combates en el orden del archivo y sin espejos (mismo ID en los dos lados). ganador: 1 = lado 1, 2 = lado 2,
0 = empate. Si al lado esta retos.csv (torre, supervivencia, kumite), tambien sale el mejor de cada reto.
"""
import argparse
import csv
import math
import os
import sys
import tempfile

NOMBRES = {0: "Goku", 1: "Kid Goku", 2: "Kid Gohan", 3: "Teen Gohan", 4: "Gohan", 5: "Great Saiyaman", 6: "Goten",
           7: "Vegeta", 8: "Trunks", 9: "Kid Trunks", 10: "Krillin", 11: "Piccolo", 12: "Tien", 13: "Yamcha",
           14: "Hercule", 15: "Videl", 16: "Supreme Kai", 17: "Uub", 18: "Raditz", 19: "Nappa", 20: "Ginyu",
           21: "Recoome", 27: "Frieza", 28: "Android 16", 29: "Android 17", 30: "Android 18", 32: "Dr. Gero",
           33: "Cell", 34: "Majin Buu", 35: "Super Buu", 36: "Kid Buu", 37: "Dabura", 38: "Cooler", 39: "Bardock",
           40: "Broly", 41: "Omega Shenron", 42: "Saibaman", 43: "Cell Jr."}


def nombre(i):
    return NOMBRES.get(i, "extra %d" % i if i >= 44 else "id %d" % i)


def leer(ruta):
    """(id1, id2, ganador) de cada combate; las lineas que no son de datos se saltan."""
    with open(ruta, encoding="utf-8", errors="replace", newline="") as f:
        for fila in csv.reader(f):
            try:
                yield int(fila[1]), int(fila[2]), int(fila[3])
            except (IndexError, ValueError):
                continue


def ranking(filas):
    """[(id, elo, ganados, perdidos, empates)] de mas a menos Elo (empates de Elo: por ID)."""
    elo, tab = {}, {}
    for a, b, w in filas:
        if a == b or a < 0 or b < 0:
            continue
        ra, rb = elo.setdefault(a, 1000.0), elo.setdefault(b, 1000.0)
        sa = 1.0 if w == 1 else 0.0 if w == 2 else 0.5
        ea = 1 / (1 + 10 ** ((rb - ra) / 400))
        elo[a], elo[b] = ra + 32 * (sa - ea), rb - 32 * (sa - ea)
        for i, s in ((a, sa), (b, 1 - sa)):
            t = tab.setdefault(i, [0, 0, 0])
            t[0 if s == 1 else 1 if s == 0 else 2] += 1
    out = [(i, int(math.floor(elo[i] + 0.5)), *tab[i]) for i in tab]   # lround del juego
    return sorted(out, key=lambda r: (-r[1], r[0]))


def mejores_retos(ruta):
    """Mejor resultado de cada reto en retos.csv: {tipo: (ganados, total, jugador)}."""
    best = {}
    if not os.path.isfile(ruta):
        return best
    with open(ruta, encoding="utf-8", errors="replace", newline="") as f:
        for r in csv.DictReader(f):
            try:
                g, t, j = int(r["ganados"]), int(r["total"]), int(r["jugador"])
            except (KeyError, ValueError):
                continue
            if g > best.get(r["tipo"], (-1,))[0]:
                best[r["tipo"]] = (g, t, j)
    return best


def selftest():
    r = ranking([(0, 7, 1)])
    assert r == [(0, 1016, 1, 0, 0), (7, 984, 0, 1, 0)], r
    r = ranking([(0, 7, 1), (0, 7, 1), (7, 0, 1)])            # el Elo se conserva y el que pierde mas abajo
    assert sum(x[1] for x in r) == 2000 and r[0][0] == 0 and r[0][2:] == (2, 1, 0), r
    assert ranking([(5, 5, 1)]) == []                           # espejo fuera
    r = ranking([(1, 2, 0)])                                    # empate entre iguales: nadie se mueve
    assert r == [(1, 1000, 0, 0, 1), (2, 1000, 0, 0, 1)], r
    with tempfile.TemporaryDirectory() as d:
        p = os.path.join(d, "combates.csv")
        with open(p, "w", encoding="utf-8") as f:
            f.write("fecha,id1,id2,ganador,motivo\n2026-10-15 00:00:27,27,7,1,4\nmal,linea\n2026-10-15 01:02:52,0,7,2,0\n")
        r = ranking(leer(p))
        assert [x[0] for x in r] == [7, 27, 0] or [x[0] for x in r] == [27, 7, 0], r
        with open(os.path.join(d, "retos.csv"), "w", encoding="utf-8") as f:
            f.write("fecha,tipo,jugador,ganados,total,resultado\nx,torre,0,2,8,derrota\nx,torre,7,5,8,derrota\n")
        assert mejores_retos(os.path.join(d, "retos.csv")) == {"torre": (5, 8, 7)}
    print("selftest OK")
    return 0


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("combates", nargs="?", help="ruta de combates.csv")
    p.add_argument("--csv", help="escribe el ranking en este CSV")
    p.add_argument("--selftest", action="store_true")
    a = p.parse_args(argv)
    if a.selftest:
        return selftest()
    if not a.combates or not os.path.isfile(a.combates):
        p.error("indica la ruta de combates.csv (user_data\\dbz3 de la copia del juego)")
    tabla = ranking(leer(a.combates))
    print("%3s  %-16s %5s  %s" % ("#", "personaje", "Elo", "G-P-E"))
    for k, (i, e, g, pe, em) in enumerate(tabla, 1):
        print("%3d  %-16s %5d  %d-%d-%d" % (k, nombre(i), e, g, pe, em))
    for tipo, (g, t, j) in sorted(mejores_retos(os.path.join(os.path.dirname(a.combates), "retos.csv")).items()):
        print("mejor %s: %d de %d (%s)" % (tipo, g, t, nombre(j)))
    if a.csv:
        with open(a.csv, "w", encoding="utf-8", newline="") as f:
            w = csv.writer(f)
            w.writerow(["puesto", "id", "personaje", "elo", "ganados", "perdidos", "empates"])
            w.writerows([k, i, nombre(i), e, g, pe, em] for k, (i, e, g, pe, em) in enumerate(tabla, 1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
