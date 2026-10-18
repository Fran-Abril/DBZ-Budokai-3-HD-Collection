"""Comprobacion del reparto persistente de IDs (roster_build.plan_ids): python test_reparto_ids.py"""
import os
import shutil
import tempfile

import roster_build as rb


def mods(d, nombres, ids=None):
    for n in nombres:
        os.makedirs(os.path.join(d, n), exist_ok=True)
    return [(n, os.path.join(d, n), {"id": (ids or {}).get(n)}) for n in sorted(nombres)]


def plan(d, srcs):
    ids, mapa, orden = rb.plan_ids(d, srcs)
    rb.guardar_reparto(d, mapa, orden)
    return ids


d = tempfile.mkdtemp(dir="D:/tmp" if os.path.isdir("D:/tmp") else None)
try:
    a = plan(d, mods(d, ["cut_zarbon", "roshi", "black"], {"cut_zarbon": 25}))
    assert a == {"cut_zarbon": 25, "black": 22, "roshi": 23}, a
    # anadir uno que va antes en el alfabeto: nadie se mueve
    b = plan(d, mods(d, ["aaa", "cut_zarbon", "roshi", "black"], {"cut_zarbon": 25}))
    assert {k: b[k] for k in a} == a and b["aaa"] == 24, b
    # quitar black: su ID queda reservado; uno nuevo no lo toma; vuelve black y recupera el suyo
    shutil.rmtree(os.path.join(d, "black"))
    c = plan(d, mods(d, ["aaa", "cut_zarbon", "roshi", "zzz"], {"cut_zarbon": 25}))
    assert c["zzz"] == 26 and 22 not in c.values(), c
    e = plan(d, mods(d, ["aaa", "black", "cut_zarbon", "roshi", "zzz"], {"cut_zarbon": 25}))
    assert e["black"] == 22, e
    # pedir un ID ocupado: aviso y otro libre, sin mover al que lo tenia
    rb.AVISOS.clear()
    f = plan(d, mods(d, ["aaa", "black", "cut_zarbon", "roshi", "zzz", "pide25"], {"cut_zarbon": 25, "pide25": 25}))
    assert f["cut_zarbon"] == 25 and f["pide25"] not in (25, None), f
    assert any(m == "pide25" for m, _ in rb.AVISOS), rb.AVISOS
    # tope de la rueda: 24 casillas; el 25.o recibe aviso y no se monta
    rb.AVISOS.clear()
    g = plan(d, mods(d, ["n%02d" % i for i in range(30)]))
    assert sum(v is not None for v in g.values()) == rb.MAX_CELDAS, g
    assert any("desactiva otro" in t for _, t in rb.AVISOS), rb.AVISOS
    # orden de trajes: uno nuevo va al final aunque su nombre vaya antes
    t, o = rb.ordenar_trajes([("traje_b", "", {})], {})
    t, o = rb.ordenar_trajes([("traje_a", "", {}), ("traje_b", "", {})], o)
    assert [x[0] for x in t] == ["traje_b", "traje_a"], t
    print("OK reparto persistente")
finally:
    shutil.rmtree(d, ignore_errors=True)
