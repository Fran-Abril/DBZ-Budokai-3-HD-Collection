#!/usr/bin/env python3
"""frame_data.py - Lector de frame data de Budokai 3 HD (ficha 11, estilo wiki de Budokai 4 / Budokai Reader).

Lee del data_cmn.afs del juego el moveset de un personaje (#CSK del bin anm + #CCM del bin cam) y saca, por
codigo de ataque: entrada (P/K/G/E con direccion, cadenas de combo), cuadros activos (lineas AP1), dano por
situacion del bloque HR (normal, contra, malabar, espalda, suelo, guardia, aturdido), tipo/codigo de aturdimiento,
empuje y marcas (A cargable, ! inesquivable, [0x04] linea sin dano medido: no es armadura). Con --medido junta lo medido en el juego por la sonda
(src/combate.cpp: logs/combate_golpes.csv + logs/combate_sonda.csv): arranque observado, dano aplicado y ventaja
en golpe/guardia por lado (ficha 12). Las medidas salen como "medido"; lo leido del fichero, como "datos".

Uso:
  python studies/tools/frame_data.py 0                        # Goku (ID 0), forma normal
  python studies/tools/frame_data.py 0 --codigos 200 210 21b  # solo esos codigos
  python studies/tools/frame_data.py 0 --usos 16 30            # que codigos usan esos bloques HR
  python studies/tools/frame_data.py 0 --medido D:/DBZ3HD/combate_test/logs --codigos 210 21b
  python studies/tools/frame_data.py --prueba                  # comprobacion sin juego (CSK sintetico)
Salida: tabla en pantalla y studies/_data/frame_data/<id>_<forma>.json (--salida para otra carpeta).
"""
from __future__ import annotations

import argparse
import csv
import json
import os
import struct
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SITUACIONES = ("normal", "contra", "malabar", "espalda", "suelo", "guardia", "otra", "aturdido")
BOTONES = ((1, "P"), (2, "K"), (4, "G"), (8, "E"))
DIRS = {0: "", 1: ">", 2: "<", 0x10: "^"}
NEUTRAS = {0, 1, 2}          # acciones X+3244 en las que el luchador ya puede actuar (de pie, guardia, andar)


# ---------------------------------------------------------------- #CSK
def csk_head(csk):
    n, lst, nhr, hr = struct.unpack_from(">4I", csk, 0x10)
    return n, lst, nhr, hr


def csk_lineas(csk, code):
    """[(tipo AP, [lineas 16 B])] del sub-bloque de 48 B del codigo (o [] si no tiene)."""
    n, lst, _, _ = csk_head(csk)
    if not 0 <= code < n:
        return []
    a = struct.unpack_from(">I", csk, lst + 4 * code)[0]
    if not a or a + 48 > len(csk):
        return []
    _, nap, apo = struct.unpack_from(">3I", csk, a + 0x24)
    if nap > 16 or apo + 8 * nap > len(csk):
        return []
    out = []
    for k in range(nap):
        t, nl, do = struct.unpack_from(">HHI", csk, apo + 8 * k)
        if do + 16 * nl <= len(csk):
            out.append((t, [csk[do + 16 * i:do + 16 * i + 16] for i in range(nl)]))
    return out


def hr_bloque(csk, hr):
    _, _, nhr, off = csk_head(csk)
    if not 0 <= hr < nhr:
        return None
    o = off + 128 * hr
    return [struct.unpack_from(">hBBHHff", csk, o + 16 * i) for i in range(8)]


def ventanas(csk, code):
    """[{inicio, fin, hr, props}] de las lineas AP1 (golpe): una ventana por linea con HR hasta su cierre."""
    out = []
    for t, lines in csk_lineas(csk, code):
        if t != 1:
            continue
        abiertas = []
        for l in lines:
            fr, lid = struct.unpack_from(">HH", l, 0)
            hr, props = struct.unpack_from(">HH", l, 6)
            if hr == 0xFFFF:
                for v in abiertas:
                    v["fin"] = fr
                out += abiertas
                abiertas = []
            else:
                abiertas.append({"inicio": fr, "fin": None, "hr": hr, "props": props, "hueso": l[10], "radio": l[11]})
        out += abiertas
    return out


def usos_hr(csk):
    n = csk_head(csk)[0]
    usos = {}
    for code in range(n):
        for v in ventanas(csk, code):
            usos.setdefault(v["hr"], []).append(code)
    return usos


def marcas(props):
    m = ""
    if props & 0x20:
        m += "A"
    if props & 0x40:
        m += "!"
    if props & 0x04:
        m += "[0x04]"   # medido 2026-10-15: no hace dano ni da armadura (P+K golpea con su otra linea)
    return m


def ficha(csk, code, usos=None):
    vs = ventanas(csk, code)
    if not vs:
        return None
    golpes = []
    for v in vs:
        b = hr_bloque(csk, v["hr"])
        if not b:
            continue
        golpes.append({
            "cuadros": [v["inicio"], v["fin"]], "hr": v["hr"], "marcas": marcas(v["props"]),
            "props": "0x%02X" % v["props"], "compartido": len((usos or {}).get(v["hr"], [code])) > 1,
            "dano": {s: b[i][0] for i, s in enumerate(SITUACIONES)},
            "aturdimiento": {"tipo": b[0][3], "codigo": b[0][4]}, "empuje": round(b[0][5], 3)})
    return {"codigo": "%X" % code, "arranque": vs[0]["inicio"], "golpes": golpes, "fuente": "datos"}


# ---------------------------------------------------------------- #CCM (entradas)
def w(ccm, o, i):
    return struct.unpack_from(">H", ccm, o + 2 * i)[0]


def entradas(ccm):
    """{codigo: notacion} recorriendo el arbol del #CCM (cadenas con coma)."""
    n = struct.unpack_from(">H", ccm, 0x1E)[0]
    todo = [(struct.unpack_from(">I", ccm, 0x50 + 4 * k)[0], "") for k in range(n)]
    visto, out = set(), {}
    while todo:
        o, pre = todo.pop(0)
        if o in visto or not 0x50 <= o <= len(ccm) - 0x40:
            continue
        visto.add(o)
        btn = "+".join(s for b, s in BOTONES if w(ccm, o, 1) & b)
        nota = (pre + ", " if pre else "") + DIRS.get(w(ccm, o, 0), "?") + btn
        if w(ccm, o, 5) & 0x0400:
            nota += " (hiper)"
        if w(ccm, o, 6) & 0x8000:
            nota += " (en hiper)"
        for c in (w(ccm, o, 12), w(ccm, o, 13)):
            if c and c not in out:
                out[c] = nota
        for k in range(min(w(ccm, o, 7), 64)):
            q = o + 0x40 + 4 * k
            if q + 4 <= len(ccm):
                todo.append((struct.unpack_from(">I", ccm, q)[0], nota))
    return out


# ---------------------------------------------------------------- juego (data_cmn.afs)
def cargar(cid, forma, us, trabajo):
    sys.path.insert(0, os.path.join(ROOT, "mod center hd"))
    import roster_build as rb  # noqa: PLC0415
    import capsulas as cp  # noqa: PLC0415
    db = json.load(open(os.path.join(ROOT, "mod center hd", "roster_db.json"), encoding="utf-8"))
    rec = next(r for r in db["ids"].values() if r["id"] == cid) if isinstance(db["ids"], dict) else db["ids"][cid]
    anm = [a for a in rec["anm"] if a != 0xFFFFFFFF]
    afs = rb.Afs(os.path.join(us, "data_cmn.afs"), trabajo)
    amb = afs.entry(anm[min(forma, len(anm) - 1)] if forma else rec["anm"][0])
    n = struct.unpack_from(">I", amb, 0x10)[0]
    csk = None
    for i in range(n):
        off, size, typ, _ = struct.unpack_from(">IIII", amb, 0x20 + 16 * i)
        if typ == 0xFFFFFFFF and amb[off:off + 4] == b"#CSK":
            csk = amb[off:off + size]
    cam = afs.entry(rec["cam"])
    o, s = cp.ccm_child(cam)
    return rec["name"], csk, cam[o:o + s]


# ---------------------------------------------------------------- medido (sonda del juego)
def medir(carpeta, inicio_ventana=None):
    """Lo medido por la sonda del juego (src/combate.cpp: logs/combate_sonda.csv + combate_golpes.csv).
    Por golpe: codigo, lado, resultado (golpe / guardia / proyectil), arranque (cuadro de la animacion del atacante al
    contacto), dano aplicado, ventaja (cuadro en que el defensor vuelve a una accion neutra menos el del atacante;
    positiva = favorece al atacante), parada (hitstop: cuadros tras el contacto en que la animacion del atacante no
    avanza) y tardio (contacto despues del primer cuadro activo del #CSK, o proyectil que llega cuando el atacante
    ya se recupero). Ademas, los ataques que no tocaron nada (al aire) con su duracion hasta volver a estar libre.
    inicio_ventana = {codigo: primer cuadro activo} (de ficha()). La geometria (esferas, cajas) no se mide: no se
    inventa; el radio y el hueso del AP1 salen como datos del fichero."""
    sonda = list(csv.DictReader(open(os.path.join(carpeta, "combate_sonda.csv"), encoding="utf-8")))
    eventos = [g for g in csv.DictReader(open(os.path.join(carpeta, "combate_golpes.csv"), encoding="utf-8"))
               if g["tipo"] in ("golpe", "proyectil") and g["atacante"] not in ("0", "")]
    por_cuadro = {int(r["cuadro"]): r for r in sonda}
    cuadros = sorted(por_cuadro)
    acc = lambda c, p: int(por_cuadro[c]["%d_accion" % p])  # noqa: E731

    def listo(p, f):
        for c in cuadros:
            if c > f and acc(c, p) in NEUTRAS:
                return c
        return None

    # ataques de cada lado: desde que entra en la accion 15 hasta que vuelve a una neutra (o le golpean)
    tramos = []
    for p in (1, 2):
        ini = None
        for c in cuadros:
            a_ = acc(c, p)
            if a_ == 15 and ini is None:
                ini = (c, por_cuadro[c]["%d_codigo" % p])
            elif ini is not None and a_ != 15:
                tramos.append({"lado": p, "inicio": ini[0], "fin": c, "codigo": ini[1].upper().lstrip("0") or "0"})
                ini = None
    out = []
    # danos que no pasan por el golpe directo (ki blasts): bajadas de vida en la sonda sin evento en ese cuadro
    con_evento = {(int(g["cuadro"]), int(g["victima"])) for g in eventos}
    for v in (1, 2):
        for c in cuadros[1:]:
            antes = por_cuadro.get(c - 1)
            if not antes or int(por_cuadro[c]["combate"]) != 4:
                continue
            baja = float(antes["%d_vida" % v]) - float(por_cuadro[c]["%d_vida" % v])
            if baja > 0 and not any((c + d, v) in con_evento for d in (-2, -1, 0, 1)):
                a_ = 3 - v
                previos = [t for t in tramos if t["lado"] == a_ and t["inicio"] <= c]
                code = previos[-1]["codigo"] if previos else "0"
                eventos.append({"cuadro": str(c - 1), "tipo": "proyectil", "victima": str(v), "atacante": str(a_),
                                "codigo": code, "cuadro_anim": "0", "dano_hr": "0", "accion_antes": "0",
                                "vida_antes": antes["%d_vida" % v], "vida_despues": por_cuadro[c]["%d_vida" % v],
                                "regla": "0"})
    for g in eventos:
        f, a_, v = int(g["cuadro"]), int(g["atacante"]), int(g["victima"])
        la, lv = listo(a_, f), listo(v, f)
        dano = float(g["vida_antes"]) - float(g["vida_despues"])
        code = g["codigo"].upper().lstrip("0") or "0"
        proy = g["tipo"] == "proyectil"
        tramo = next((t for t in tramos if t["lado"] == a_ and t["inicio"] <= f and (proy or f <= t["fin"])
                      and (not proy or t["codigo"] == code)), None)
        if proy:   # el ultimo ataque de ese codigo antes del contacto
            cand = [t for t in tramos if t["lado"] == a_ and t["codigo"] == code and t["inicio"] <= f]
            tramo = cand[-1] if cand else None
        parada = 0
        if not proy:
            prev = None
            for c in range(f, f + 4):   # la parada (hitstop) es justo al tocar; luego es la animacion
                if c in por_cuadro:
                    an = float(por_cuadro[c]["%d_cuadro_anim" % a_])
                    if prev is not None and an <= prev and acc(c, a_) == 15:
                        parada += 1
                    prev = an
        arranque = int(float(g["cuadro_anim"])) if not proy else (f - tramo["inicio"] + 1 if tramo else None)
        if proy:
            tardio = bool(tramo) and tramo["fin"] <= f   # el atacante ya estaba libre cuando llego
        else:
            ini_v = (inicio_ventana or {}).get(code)
            tardio = ini_v is not None and arranque > ini_v
        guardia = int(g["accion_antes"]) == 1 or (int(g["dano_hr"] or 0) > 0 and dano < 0.75 * int(g["dano_hr"]))
        out.append({"codigo": code, "lado": a_, "resultado": "proyectil" if proy else "guardia" if guardia else "golpe",
                    "cuadro": f, "arranque": arranque, "dano": dano, "dano_hr": int(g["dano_hr"] or 0),
                    "regla": g.get("regla") == "1", "parada": parada, "tardio": tardio,
                    "ventaja": (lv - la) if la is not None and lv is not None else None, "fuente": "medido"})
    usados = {(t["lado"], t["inicio"]) for t in tramos for m in out
              if m["lado"] == t["lado"] and t["inicio"] <= m["cuadro"] and m["codigo"] == t["codigo"]
              and (m["cuadro"] <= t["fin"] or m["resultado"] == "proyectil")}
    for t in tramos:
        if (t["lado"], t["inicio"]) not in usados:
            l_ = listo(t["lado"], t["fin"] - 1)
            out.append({"codigo": t["codigo"], "lado": t["lado"], "resultado": "al aire", "cuadro": t["inicio"],
                        "arranque": None, "dano": 0.0, "dano_hr": 0, "regla": False, "parada": 0, "tardio": False,
                        "ventaja": None, "duracion": (l_ or t["fin"]) - t["inicio"], "fuente": "medido"})
    return sorted(out, key=lambda m: m["cuadro"])


# ---------------------------------------------------------------- comprobacion sin juego
def prueba():
    hr_off = 0x200
    csk = bytearray(hr_off + 128 * 2)
    struct.pack_into(">4I", csk, 0x10, 2, 0x40, 2, hr_off)
    struct.pack_into(">II", csk, 0x40, 0, 0x60)              # codigo 1 -> bloque en 0x60
    struct.pack_into(">3I", csk, 0x60 + 0x24, 0, 1, 0x100)   # una tabla AP
    struct.pack_into(">HHI", csk, 0x100, 1, 2, 0x140)        # AP1 con 2 lineas
    struct.pack_into(">HHBBHH", csk, 0x140, 8, 0, 1, 0, 1, 0x20)
    struct.pack_into(">HHBBHH", csk, 0x150, 11, 0, 1, 0, 0xFFFF, 0)
    struct.pack_into(">hBBHHff", csk, hr_off + 128, 30, 0, 0, 0, 7, 1.5, 0)
    f = ficha(bytes(csk), 1, usos_hr(bytes(csk)))
    assert f["arranque"] == 8 and f["golpes"][0]["cuadros"] == [8, 11], f
    assert f["golpes"][0]["dano"]["normal"] == 30 and f["golpes"][0]["marcas"] == "A", f
    assert ficha(bytes(csk), 0) is None
    print("frame_data: prueba OK")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("id", type=int, nargs="?")
    ap.add_argument("--forma", type=int, default=0, help="indice del bin anm (0 = forma normal)")
    ap.add_argument("--codigos", nargs="*", default=None, help="codigos en hex (por defecto todos)")
    ap.add_argument("--usos", nargs="*", type=int, default=None, help="bloques HR: que codigos los usan")
    ap.add_argument("--medido", default=None, help="carpeta logs/ con combate_golpes.csv y combate_sonda.csv")
    ap.add_argument("--us", default=os.path.join(ROOT, "us"))
    ap.add_argument("--trabajo", default=None)
    ap.add_argument("--salida", default=os.path.join(ROOT, "studies", "_data", "frame_data"))
    ap.add_argument("--prueba", action="store_true")
    a = ap.parse_args()
    if a.prueba:
        return prueba()
    if a.id is None:
        ap.error("falta el ID del personaje")
    trabajo = a.trabajo or tempfile.mkdtemp(prefix="frame_data_")
    nombre, csk, ccm = cargar(a.id, a.forma, a.us, trabajo)
    usos = usos_hr(csk)
    if a.usos is not None:
        for h in a.usos:
            print("HR %d: %s" % (h, ", ".join("%X" % c for c in usos.get(h, [])) or "-"))
        return 0
    notas = entradas(ccm)
    codigos = [int(c, 16) for c in a.codigos] if a.codigos else range(csk_head(csk)[0])
    filas = [f for f in (ficha(csk, c, usos) for c in codigos) if f]
    for f in filas:
        f["entrada"] = notas.get(int(f["codigo"], 16), "")
    medidas = []
    if a.medido:
        medidas = medir(a.medido, {f["codigo"]: f["arranque"]
                                   for f in (ficha(csk, c, usos) for c in range(csk_head(csk)[0])) if f})
    print("%s (ID %d, forma %d): %d codigos con golpe" % (nombre, a.id, a.forma, len(filas)))
    print("%-5s %-16s %-9s %-6s %s" % ("cod", "entrada", "activo", "HR", "dano normal/guardia  aturd.  marcas"))
    for f in filas:
        for g in f["golpes"]:
            print("%-5s %-16s %-9s %-6s %4d/%-4d  t%d c%-3d %s%s" % (
                f["codigo"], f["entrada"][:16], "%s-%s" % tuple(g["cuadros"]), g["hr"], g["dano"]["normal"],
                g["dano"]["guardia"], g["aturdimiento"]["tipo"], g["aturdimiento"]["codigo"], g["marcas"],
                " (HR compartido)" if g["compartido"] else ""))
    if medidas:
        print("\nmedido en el juego (cuadros a 60 por segundo):")
        print("%-5s %-5s %-10s %-8s %-6s %-6s %-7s %-6s %-7s %s" % (
            "cod", "lado", "result.", "arranque", "dano", "HR", "ventaja", "parada", "tardio", "regla"))
        quiero = [c.upper().lstrip("0") or "0" for c in a.codigos] if a.codigos else None
        for m in medidas:
            if not quiero or m["codigo"] in quiero or m["resultado"] == "proyectil":
                extra = "dura %d" % m["duracion"] if m["resultado"] == "al aire" else ""
                print("%-5s P%-4d %-10s %-8s %-6.0f %-6d %-7s %-6d %-7s %s %s" % (
                    m["codigo"], m["lado"], m["resultado"], m["arranque"], m["dano"], m["dano_hr"], m["ventaja"],
                    m["parada"], "si" if m["tardio"] else "", "v0" if m["regla"] else "", extra))
    os.makedirs(a.salida, exist_ok=True)
    dst = os.path.join(a.salida, "%d_%d.json" % (a.id, a.forma))
    json.dump({"personaje": nombre, "id": a.id, "forma": a.forma, "datos": filas, "medido": medidas}, open(dst, "w"),
              indent=1)
    print("\n->", dst)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
