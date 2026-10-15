#!/usr/bin/env python3
"""retos_predefinidos.py - Catalogo de retos nativos (torre, supervivencia, kumite) listos para jugar.

El juego (src/modos.cpp) lee UN reto por modo: <juego>/user_data/dbz3/modos/<tipo>.toml. Los retos del
catalogo son los campanas/retos/nativo_<tipo>[_<tema>].toml (formato de `modos_reto.py nativo`, con las claves
de catalogo tipo/nombre/name/descripcion/description que el juego ignora). Esta herramienta solo copia ficheros:
no toca el exe, la partida ni otros mods. Lo que sustituye va a user_data/dbz3/modos/_respaldo/ (nunca se borra).

  python retos_predefinidos.py lista [--juego CARPETA]          # catalogo (y cual esta puesto en ese juego)
  python retos_predefinidos.py instalar nativo_torre_namek --juego CARPETA
  python retos_predefinidos.py semana --juego CARPETA [--tipo torre] [--fecha 2026-10-15]
  python retos_predefinidos.py quitar torre --juego CARPETA      # vuelve el reto por defecto del juego
  python retos_predefinidos.py --selftest

`semana` pone la torre (o el modo que se pida) de la semana: la misma toda la semana ISO del reloj del PC, la
siguiente en la semana siguiente, sin azar (reiniciar no cambia la torre). El reto en curso no cambia: el juego
lee el fichero al empezar cada reto. El reloj local no sirve para ranking (studies/11_vision_futura/26).
"""
import argparse
import datetime
import os
import shutil
import sys
import tempfile

try:
    import tomllib
except ImportError:  # Python < 3.11
    import tomli as tomllib

HERE = os.path.dirname(os.path.abspath(__file__))
CATALOGO = os.path.join(HERE, "campanas", "retos")
TIPOS = ("torre", "supervivencia", "kumite")
SIN_MODELO = {22, 23, 24, 25, 26, 31}   # recortados de B3: solo existen con un mod de roster (modos.cpp Jugable)
VIDA_TOPE = 7000
ESCENARIOS = range(0, 17)


def destino(juego, tipo):
    return os.path.join(juego, "user_data", "dbz3", "modos", tipo + ".toml")


def catalogo(carpeta=CATALOGO):
    """{clave: (ruta, doc)} de los nativo_<tipo>*.toml que se leen bien (los demas se avisan y se saltan)."""
    out = {}
    for f in sorted(os.listdir(carpeta)):
        clave, ext = os.path.splitext(f)
        tipo = clave[len("nativo_"):].split("_")[0] if clave.startswith("nativo_") else ""
        if ext != ".toml" or tipo not in TIPOS:
            continue
        ruta = os.path.join(carpeta, f)
        try:
            with open(ruta, "rb") as h:
                doc = tomllib.load(h)
        except (OSError, tomllib.TOMLDecodeError) as e:
            print("aviso: %s no se puede leer (%s)" % (f, e))
            continue
        doc.setdefault("tipo", tipo)
        out[clave] = (ruta, doc)
    return out


def errores(clave, doc):
    """Mismos limites que aplica el juego (y modos_reto.comprobar_nativo) + coherencia del catalogo."""
    err = []
    tipo = clave[len("nativo_"):].split("_")[0]
    if doc.get("tipo") != tipo:
        err.append("tipo '%s' no coincide con el nombre del fichero (%s)" % (doc.get("tipo"), tipo))
    if not 1 <= doc.get("vida", 3000) <= VIDA_TOPE:
        err.append("vida fuera de 1..%d" % VIDA_TOPE)
    if not 0 <= doc.get("vida_inicial", 0) <= VIDA_TOPE:
        err.append("vida_inicial fuera de 0..%d" % VIDA_TOPE)
    cs = doc.get("combate", [])
    if not cs:
        err.append("sin combates")
    for i, c in enumerate(cs, 1):
        r = c.get("rival", -1)
        if not 0 <= r < 64:
            err.append("combate %d: rival fuera de 0..63" % i)
        elif r in SIN_MODELO or r >= 44:
            err.append("combate %d: el rival %d solo existe con un mod de roster" % (i, r))
        if c.get("escenario", 0) not in ESCENARIOS:
            err.append("combate %d: escenario fuera de 0..16" % i)
        if not 0 <= c.get("vida", 0) <= VIDA_TOPE:
            err.append("combate %d: vida fuera de 0..%d" % (i, VIDA_TOPE))
    return err


def mover_a_respaldo(ruta):
    if not os.path.exists(ruta):
        return None
    dir_r = os.path.join(os.path.dirname(ruta), "_respaldo")
    os.makedirs(dir_r, exist_ok=True)
    base = os.path.splitext(os.path.basename(ruta))[0]
    sello = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    nuevo = os.path.join(dir_r, "%s_%s.toml" % (base, sello))
    n = 1
    while os.path.exists(nuevo):
        n += 1
        nuevo = os.path.join(dir_r, "%s_%s_%d.toml" % (base, sello, n))
    shutil.move(ruta, nuevo)
    return nuevo


def leer(ruta):
    with open(ruta, "rb") as h:
        return h.read()


def instalar(clave, juego, cat=None):
    cat = cat or catalogo()
    if clave not in cat:
        raise SystemExit("no hay ningun reto '%s' en el catalogo (mira: retos_predefinidos.py lista)" % clave)
    ruta, doc = cat[clave]
    err = errores(clave, doc)
    if err:
        raise SystemExit("%s no es valido: %s" % (clave, "; ".join(err)))
    if not os.path.isdir(juego):
        raise SystemExit("no existe la carpeta del juego %s" % juego)
    dst = destino(juego, doc["tipo"])
    if os.path.exists(dst) and leer(dst) == leer(ruta):
        print("%s ya estaba puesto en %s" % (clave, dst))
        return dst
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    viejo = mover_a_respaldo(dst)
    shutil.copyfile(ruta, dst)
    print("%s -> %s (%d combates)%s" % (clave, dst, len(doc["combate"]),
                                        "; el anterior en %s" % viejo if viejo else ""))
    return dst


def quitar(tipo, juego):
    viejo = mover_a_respaldo(destino(juego, tipo))
    print("%s: vuelve el reto por defecto del juego%s" % (tipo, "; el anterior en %s" % viejo if viejo else ""))
    return viejo


def de_la_semana(tipo, fecha, cat=None):
    """Clave del reto de esa semana ISO: rota por el catalogo del tipo en orden alfabetico, sin azar."""
    cat = cat or catalogo()
    claves = sorted(k for k, (_, d) in cat.items() if d["tipo"] == tipo and not errores(k, d))
    if not claves:
        raise SystemExit("no hay retos de %s en el catalogo" % tipo)
    anio, semana, _ = fecha.isocalendar()
    return claves[(anio * 53 + semana) % len(claves)]


def puesto(juego, tipo, cat):
    dst = destino(juego, tipo)
    if not os.path.exists(dst):
        return "(reto por defecto)"
    datos = leer(dst)
    for k, (ruta, _) in cat.items():
        if leer(ruta) == datos:
            return k
    return "(reto propio)"


def lista(juego=None):
    cat = catalogo()
    for tipo in TIPOS:
        print("== %s%s" % (tipo, "  [puesto: %s]" % puesto(juego, tipo, cat) if juego else ""))
        for k, (_, d) in cat.items():
            if d["tipo"] != tipo:
                continue
            err = errores(k, d)
            print("  %-34s %3d combates  %s%s" % (k, len(d.get("combate", [])), d.get("nombre", "Reto por defecto"),
                                                  "  (ERROR: %s)" % "; ".join(err) if err else ""))
    return 0


def selftest():
    fallos = []
    cat = catalogo()
    for k, (_, d) in cat.items():
        fallos += ["%s: %s" % (k, e) for e in errores(k, d)]
    try:   # el mismo validador que el generador de retos nativos, si esta
        import modos_reto
        for k, (_, d) in cat.items():
            fallos += ["%s (modos_reto): %s" % (k, e) for e in modos_reto.comprobar_nativo(d)]
    except ImportError:
        print("aviso: sin modos_reto.py, solo se aplica la comprobacion propia")
    nuevos = [k for k, (_, d) in cat.items() if "nombre" in d]
    if len(nuevos) < 6:
        fallos.append("catalogo: solo %d retos con nombre" % len(nuevos))
    for k in nuevos:
        d = cat[k][1]
        for c in ("nombre", "name", "descripcion", "description"):
            if not str(d.get(c, "")).strip():
                fallos.append("%s: falta '%s'" % (k, c))
    with tempfile.TemporaryDirectory() as tmp:
        juego = os.path.join(tmp, "juego")
        os.makedirs(juego)
        a = next(k for k in nuevos if cat[k][1]["tipo"] == "torre")
        b = next(k for k in nuevos if cat[k][1]["tipo"] == "torre" and k != a)
        dst = instalar(a, juego, cat)
        if leer(dst) != leer(cat[a][0]):
            fallos.append("instalar: la copia no es igual al catalogo")
        instalar(a, juego, cat)   # repetido: no crea respaldo
        resp = os.path.join(os.path.dirname(dst), "_respaldo")
        if os.path.exists(resp) and os.listdir(resp):
            fallos.append("instalar dos veces lo mismo no deberia respaldar nada")
        instalar(b, juego, cat)
        if len(os.listdir(resp)) != 1 or leer(dst) != leer(cat[b][0]):
            fallos.append("instalar otro: el anterior debe ir a _respaldo y el nuevo quedar puesto")
        if puesto(juego, "torre", cat) != b:
            fallos.append("puesto: no reconoce el reto instalado")
        quitar("torre", juego)
        if os.path.exists(dst) or len(os.listdir(resp)) != 2:
            fallos.append("quitar: el reto debe ir a _respaldo (y el juego usa el de por defecto)")
    lunes = datetime.date(2026, 10, 12)
    for tipo in TIPOS:
        n = len([k for k, (_, d) in cat.items() if d["tipo"] == tipo and not errores(k, d)])
        sem = [de_la_semana(tipo, lunes + datetime.timedelta(weeks=w), cat) for w in range(n)]
        if len(set(sem)) != n:
            fallos.append("semana %s: en %d semanas seguidas no salen los %d retos" % (tipo, n, n))
        if any(de_la_semana(tipo, lunes + datetime.timedelta(days=d), cat) != sem[0] for d in range(7)):
            fallos.append("semana %s: cambia dentro de la misma semana" % tipo)
    for f in fallos:
        print("FALLO:", f)
    print("retos_predefinidos selftest: %d retos en el catalogo (%d nuevos), %s"
          % (len(cat), len(nuevos), "OK" if not fallos else "CON ERRORES"))
    return 0 if not fallos else 1


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    p.add_argument("--selftest", action="store_true", help="comprueba el catalogo y la instalacion en una carpeta temporal")
    sub = p.add_subparsers(dest="orden")
    l = sub.add_parser("lista", help="catalogo de retos")
    l.add_argument("--juego", help="carpeta del juego: dice que reto esta puesto en cada modo")
    i = sub.add_parser("instalar", help="pone un reto del catalogo en el juego")
    i.add_argument("reto", help="clave del catalogo (p. ej. nativo_torre_namek)")
    i.add_argument("--juego", required=True)
    s = sub.add_parser("semana", help="pone el reto de esta semana")
    s.add_argument("--juego", required=True)
    s.add_argument("--tipo", choices=TIPOS, default="torre")
    s.add_argument("--fecha", help="AAAA-MM-DD (por defecto hoy)")
    q = sub.add_parser("quitar", help="vuelve el reto por defecto del juego en ese modo")
    q.add_argument("tipo", choices=TIPOS)
    q.add_argument("--juego", required=True)
    a = p.parse_args(argv)
    if a.selftest:
        return selftest()
    if a.orden == "lista":
        return lista(a.juego)
    if a.orden == "instalar":
        instalar(a.reto, a.juego)
        return 0
    if a.orden == "semana":
        fecha = datetime.date.fromisoformat(a.fecha) if a.fecha else datetime.date.today()
        clave = de_la_semana(a.tipo, fecha)
        anio, semana, _ = fecha.isocalendar()
        print("semana %d-%02d: %s" % (anio, semana, catalogo()[clave][1].get("nombre", clave)))
        instalar(clave, a.juego)
        return 0
    if a.orden == "quitar":
        quitar(a.tipo, a.juego)
        return 0
    p.print_help()
    return 2


if __name__ == "__main__":
    sys.exit(main())
