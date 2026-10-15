#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""forma_unica.py - Capsula de transformacion unica (ficha 14, como Infinite World). RESPALDO.

Desde 2026-10-15 lo hace el propio juego al arrancar (src/transformaciones.cpp PrepareNamesMod: misma carpeta,
solo el idioma activo, sin comprimir y rehecho solo si cambian los AFS, el _roster o el idioma), asi que ni el
Kit ni roster_build necesitan llamarlo. Queda para depurar o hacerlo a mano en los 6 idiomas: el juego respeta
esta version mientras el _roster sea el mismo (roster_hash); si cambia, la aparta a mods_respaldo y hace la suya.

El registro de la capsula lo crea el juego (src/roster_ext.cpp: ID 1984 + personaje, para todo personaje
con 2 o mas transformaciones normales). Esto genera mods/_forma_unica con lo que se ve:
  * su nombre en los bancos de nombres de capsulas (el de cada personaje y los generales 2663/2684),
    como COPIAS nuevas en data_*.afs: el juego carga la copia en lugar del banco original;
  * el panel de descripcion de "Edit Skills" (5 rotulos) en los 6 data_*.afs (5 idiomas);
  * roster.toml con [forma_unica] (sin el mod, no hay capsula unica).
Se hace en el PC del jugador con sus AFS: los bancos llevan arte del juego y nunca van en la release.
Los bancos de los personajes nuevos salen del _roster: si se vuelve a montar (roster_build), hay que
volver a ejecutar esto (el juego lo detecta por build.hash y, mientras, los deja sin nombre).

  python "mod center hd/forma_unica.py" [--mods DIR] [--us DIR] [--todas]
  --todas: tambien da las formas que no tengas desbloqueadas (por defecto, solo las tuyas).
"""
import argparse
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import capsulas  # noqa: E402
import roster_build as rb  # noqa: E402
from hornada_support import TreePublish  # noqa: E402

MOD = "_forma_unica"
BASE_ID = 1984                  # 1984 + ID de personaje (0..63), como forms::kUnifiedBase
GAP = 1024                      # entradas nuevas de data_*.afs: lejos de las que anade el _roster

TEXTOS = {   # nombre, quien, que hace, botones, nota
    "en": ("All Transformations", "Characters who transform",
           "Can use every transformation\nyou have unlocked", "P+K+G\nWith the Ki each form needs",
           "Takes half the slots of\nits separate capsules"),
    "es": ("Todas las transformaciones", "Personajes que se transforman",
           "Puede usar todas las\ntransformaciones desbloqueadas", "P+K+G\nCon el Ki que pide cada forma",
           "Ocupa la mitad de huecos\nque sus cápsulas sueltas"),
    "fr": ("Toutes les transformations", "Personnages qui se transforment",
           "Peut utiliser toutes les\ntransformations débloquées", "P+K+G\nAvec le Ki de chaque forme",
           "Prend la moitié des places\nde ses capsules séparées"),
    "de": ("Alle Verwandlungen", "Charaktere mit Verwandlungen",
           "Kann alle freigeschalteten\nVerwandlungen nutzen", "P+K+G\nMit dem Ki jeder Form",
           "Braucht die Hälfte der Plätze\nseiner einzelnen Kapseln"),
    "it": ("Tutte le trasformazioni", "Personaggi che si trasformano",
           "Può usare tutte le\ntrasformazioni sbloccate", "P+K+G\nCon il Ki di ogni forma",
           "Occupa metà degli spazi\ndelle sue capsule separate"),
}


def native_banks():
    """Banco de nombres (data_usi) de cada personaje del juego que tiene transformaciones."""
    out = {}
    for cid, e in rb.IDS.items():
        if e.get("bank", 0xFFFF) != 0xFFFF and len([c for c in (e.get("form_caps") or [])[1:] if c]) >= 2:
            out[cid] = e["bank"]
    return out


def roster_banks(mods):
    """{ID: fid del banco} de los personajes nuevos del _roster (hud = [alto, bajo])."""
    path = os.path.join(mods, rb.OUT_MOD, "roster.toml")
    if not os.path.isfile(path):
        return {}
    out, cid = {}, None
    for ln in open(path, encoding="utf-8"):
        m = re.match(r"\s*id\s*=\s*(\d+)", ln)
        if m:
            cid = int(m.group(1))
        m = re.match(r"\s*hud\s*=\s*\[\s*(\d+)\s*,\s*(\d+)\s*\]", ln)
        if m and cid is not None and cid < 64:
            out[cid] = (int(m.group(1)) << 8) | int(m.group(2))
    return out


def roster_entry(mods, afs_name, fid, work):
    """Entrada fid de data_*.afs tal como la sirve el _roster (None si no la trae)."""
    d = os.path.join(mods, rb.OUT_MOD, "us", afs_name, str(fid))
    files = sorted(f for f in os.listdir(d) if os.path.isfile(os.path.join(d, f))) if os.path.isdir(d) else []
    if not files:
        return None
    return rb.unlzx(open(os.path.join(d, files[0]), "rb").read(), work)


def build(mods, us, todas=False):
    work = rb.work_dir("forma_unica_")
    afss = [("data_usi.afs", os.path.join(us, "data_usi.afs"))] + \
           [("data_%s.afs" % lk, os.path.join(us, "data_%s.afs" % lk)) for lk in rb.DATA_LANGS]
    afss = [(n, rb.Afs(p, work)) for n, p in afss if os.path.isfile(p)]
    if not afss:
        raise SystemExit("forma_unica: no encuentro data_usi.afs ni data_eng.afs en %s" % us)
    roster = os.path.isfile(os.path.join(mods, rb.OUT_MOD, "build.hash")) and not rb.mod_off(mods, rb.OUT_MOD)
    stamp = open(os.path.join(mods, rb.OUT_MOD, "build.hash")).read().strip() if roster else ""
    natives = native_banks()
    ports = roster_banks(mods) if roster else {}
    # bancos: (fid original, IDs de las capsulas unicas que lleva, del _roster?)
    banks = [(fid, [BASE_ID + cid], False) for cid, fid in sorted(natives.items())]
    banks += [(fid, [BASE_ID + cid], True) for cid, fid in sorted(ports.items())]
    every = sorted(BASE_ID + cid for cid in set(natives) | set(ports))
    banks += [(capsulas.NAMES_SHORT, every, roster), (capsulas.NAMES_LONG, every, roster)]
    next_fid = max(len(a.index) for _, a in afss) + GAP
    desc = next_fid
    plan = [(fid, ids, from_roster, desc + 1 + k) for k, (fid, ids, from_roster) in enumerate(banks)]

    txn = TreePublish(os.path.join(mods, MOD))
    try:
        out = str(txn.stage)
        for afs_name, afs in afss:
            lang = capsulas.lang_of(afs_name)
            name, who, what, cmd, note = TEXTOS.get(lang, TEXTOS["en"])
            rb.write_entry(out, afs_name, desc, capsulas.build_desc(name, who, what, cmd, note), work)
            img = capsulas.render_name(name)
            for fid, ids, from_roster, new in plan:
                b = roster_entry(mods, afs_name, fid, work) if from_roster else None
                b = b if b is not None else afs.entry(fid)
                rb.write_entry(out, afs_name, new, capsulas.extend_bank(b, {i: img for i in ids}), work)
            rb.log("%s: %s, descripcion %d y %d bancos" % (afs_name, lang, desc, len(plan)))
        pairs = lambda r: ", ".join("%d, %d" % (fid, new) for fid, _, fr, new in plan if fr == r)  # noqa: E731
        toml = ["# GENERADO por mod center hd/forma_unica.py (no editar: se regenera).",
                "# Capsula de transformacion unica: el juego la crea (1984 + ID); aqui su nombre y descripcion.",
                "[forma_unica]", "descripcion = %d" % desc,
                "bancos = [%s]" % pairs(False), "bancos_roster = [%s]" % pairs(True),
                'roster_hash = "%s"' % stamp, "todas = %s" % ("true" if todas else "false"), ""]
        open(os.path.join(out, "roster.toml"), "w", encoding="utf-8").write("\n".join(toml))
        txn.commit(lambda st: os.path.isfile(os.path.join(st, "roster.toml")))
    finally:
        txn.finish()
    rb.log("forma_unica: %s listo (%d personajes del juego, %d nuevos)" % (MOD, len(natives), len(ports)))
    return 0


def _selftest():
    assert set(TEXTOS) == {"en", "es", "fr", "de", "it"} and all(len(t) == 5 for t in TEXTOS.values())
    img = capsulas.render_name(TEXTOS["es"][0])
    b = capsulas.build_bank(10, {10: img, 12: img})
    e = capsulas.extend_bank(b, {BASE_ID + 7: img})
    n, idx, first = struct.unpack(">III", e[0x10:0x1C])
    assert first == 10 and n == BASE_ID + 7 - 10 + 1
    assert struct.unpack(">I", e[idx + 4 * (n - 1):idx + 4 * n])[0] != 0          # la nueva tiene entrada
    assert capsulas.azt_entries(e)[:3] == capsulas.azt_entries(b)[:3]              # las de antes, igual
    assert 0x82 < len(capsulas.build_desc(*TEXTOS["de"]))
    print("forma_unica: autoprueba OK")


def main():
    ap = argparse.ArgumentParser(description="Capsula de transformacion unica: nombres y descripcion")
    ap.add_argument("--mods")
    ap.add_argument("--us")
    ap.add_argument("--todas", action="store_true", help="tambien las formas sin desbloquear")
    ap.add_argument("--autoprueba", action="store_true")
    a = ap.parse_args()
    if a.autoprueba:
        return _selftest()
    return build(a.mods or rb.default_mods(), rb.us_dir(a.us), a.todas)


if __name__ == "__main__":
    sys.exit(main())
