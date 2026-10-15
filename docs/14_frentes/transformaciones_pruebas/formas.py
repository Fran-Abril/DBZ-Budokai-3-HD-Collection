#!/usr/bin/env python3
"""Pruebas del frente transformaciones (capsula de transformacion unica, ficha 14) en una copia aislada.

  python docs/14_frentes/transformaciones_pruebas/formas.py ESCENARIO [copia]   (copia: transformaciones_test)

Escenarios (combate nativo del HUB, 1P contra Vegeta COM; la prueba pulsa P+K+G de verdad y el juego decide):
  goku     capsula unica de Goku (1984) con sus 5 transformaciones desbloqueadas: sube una a una y baja sin ki
  parcial  idem con solo Kaioken y SSJ desbloqueados: se queda en SSJ aunque se pulse P+K+G
  sdbh     Goku (SDBH), port con 4 transformaciones (capsula 2034)
  inicial  forma inicial (DBZ3_START_FORM=3, SSJ2) con la capsula unica; control: sin ella empieza normal
  tag      Tag Team Goku + Goku (SDBH): relevo y luego el que entra sube con su capsula unica
Antes: la copia necesita mods/_forma_unica (forma_unica.py) y, para tag, mods/zz_prueba_forma_unica.
"""
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "tools"))
import pruebas as P  # noqa: E402

PKG = "L+Space+Backspace"        # X + A + B = P+K+G (el juego ve E000 pero no lo toma: ver 'teclas')
BASE_ENV = {"DBZ3_HUB_MODE": "5", "DBZ3_TEST_FORMAS": "1", "DBZ3_TEST_FORMAS_AUTO": "1"}


def subir(j, formas, prefijo):
    for f in range(1, formas + 1):
        j.log(rf"prueba: lista para la forma {f} \(.*daria la {f}\)", 60)
        j.log(rf"prueba: sube a la forma {f}\b", 40)
        time.sleep(2.0)
        j.captura(f"{prefijo}_forma{f}")


def bajar(j, prefijo):
    j.log(r"prueba: tope en la forma", 30)
    j.log(r"prueba: baja a la forma 0", 120)
    time.sleep(1.5)
    j.captura(f"{prefijo}_baja")


def _goku(j, par):
    j.log(r"capsula unica 1984 -> formas 3E", 90)
    j.log(r"prueba: lista para la forma 1 ", 60)
    j.captura("goku_forma0")
    subir(j, 5, "goku")
    bajar(j, "goku")


def _parcial(j, par):
    j.log(r"capsula unica 1984 -> formas 06", 90)
    subir(j, 2, "parcial")
    j.log(r"prueba: tope en la forma 2 de 6", 30)
    time.sleep(8)
    if "sube a la forma 3" in j.texto_log():
        raise P.Fallo("subio a SSJ2 sin tenerlo desbloqueado")
    j.captura("parcial_tope")


def _teclas(j, par):
    # control: P+K+G con 3 teclas a la vez y la capsula CLASICA de Kaioken (sin capsula unica ni AUTO)
    j.log(r"prueba: lista para la forma 1 \(.*daria la 1\)", 90)
    for _ in range(6):
        j.tecla(PKG, 2500)
    j.captura("teclas_control")
    if "sube a la forma 1" in j.texto_log():
        j.info("P+K+G con teclas SI transforma con la capsula clasica")
    else:
        j.info("P+K+G con teclas NO transforma ni con la capsula clasica: limite de la prueba, no de la capsula")


def _sdbh(j, par):
    j.log(r"id 50: capsula unica 2034 -> formas 1E", 90)
    j.log(r"prueba: lista para la forma 1 ", 60)
    j.captura("sdbh_forma0")
    subir(j, 4, "sdbh")
    bajar(j, "sdbh")


def _inicial(j, par):
    j.log(r"tag: empieza un combate", 90)
    time.sleep(3)
    j.captura("inicial")
    texto = j.texto_log()
    if par.get("control") == "1":
        if "pedida 3 aplicada 0" not in texto:
            raise P.Fallo("sin la capsula unica deberia empezar en la forma normal (motivo capsula)")
    elif "dbz3 forma: lado 1" in texto:
        raise P.Fallo("la forma inicial no se aplico: " + texto[texto.find("dbz3 forma: lado 1"):][:120])


def _tag(j, par):
    j.log(r"tag: empieza un combate", 90)
    time.sleep(4)
    j.captura("tag_goku")
    j.tecla_hasta_log("Z", r"relevo .* -> ", intentos=8, seg=4)
    j.log(r"id 50: capsula unica 2034 -> formas 1E", 30)
    time.sleep(3)
    j.captura("tag_entra_sdbh")
    subir(j, 4, "tag_sdbh")


# ---- WL-14: volver a Clasico sin cambiar la partida (studies/11_vision_futura/INTEGRACION_SEGURA.md) ----
# partida en memoria: bloque 0x824BA110 desde +0x150 (antes: pantalla y escena actuales, no es partida; los bloques
# por casilla empiezan en +336, ver ESTRUCTAS de pruebas.py) hasta el final del inventario (+0x2AFE9 + 2048)
SAVE_AT, SAVE_LEN = 0x824BA110 + 0x150, 0x2AFE9 + 2048 - 0x150
DATA_BIN = Path("user_data/dbz3/B13EBABEBABEBABE/4E4D0856/00000001/DBZ3/data.bin")


def partida(j):
    b = P.leer(j.h, P.GBASE + SAVE_AT, SAVE_LEN)
    if not b or len(b) != SAVE_LEN:
        raise P.Fallo("no pude leer la partida en memoria")
    return b


def misma_partida(j, antes, que):
    ahora = partida(j)
    dif = [i for i in range(SAVE_LEN) if antes[i] != ahora[i]]
    if dif:
        raise P.Fallo("%s: la partida en memoria cambio en %d bytes (primeros: %s)" % (
            que, len(dif), ", ".join("+0x%X" % i for i in dif[:8])))
    j.info("%s: partida en memoria igual byte a byte (%d B)" % (que, SAVE_LEN))


def _wl14_combate(j, par):
    antes = partida(j)
    j.log(r"capsula unica 1984 -> formas 3E", 90)
    subir(j, 4, "wl14")
    misma_partida(j, antes, "tras subir a SSJ3 con la capsula unica")


def _wl14_sin_mod(j, par):
    # _forma_unica apagado (como sin nombres / ISO) y una lista guardada con la capsula unica: fuera del combate
    antes = partida(j)
    j.log(r"capsula 1984 fuera del catalogo", 60)
    subir(j, 1, "wl14_sin_mod")                      # Kaioken suelta, P+K+G de verdad
    j.log(r"prueba: tope en la forma 1 de 6", 30)
    misma_partida(j, antes, "sin la capsula unica")


def _wl14_limites(j, par):
    j.log(r"prueba: P\+K\+G con ki insuficiente", 60)
    time.sleep(4)
    if "sube a la forma" in j.texto_log():
        raise P.Fallo("subio sin el ki que pide la forma")
    j.captura("wl14_ki_insuficiente")
    for f in (1, 2):                                  # luego Kaioken y SSJ, ya con su ki
        j.log(rf"prueba: sube a la forma {f}\b", 60)
        time.sleep(2.0)
        j.captura(f"wl14_limites_forma{f}")
    j.log(r"prueba: P\+K\+G en el tope \(forma 2", 30)
    time.sleep(5)
    if "sube a la forma 3" in j.texto_log():
        raise P.Fallo("paso a SSJ2 sin tenerlo desbloqueado")
    j.captura("wl14_tope_ssj")


def _wl14_edit(j, par):
    # Edit Skills: equipar la capsula unica y quitarla (vuelta a Clasico) y salir al menu principal
    antes = partida(j)
    pasos = ["tecla S 1200", "tecla Space 0", "esperar_pantalla 14 10", "esperar 2500", "tecla Space 8000",
             "tecla Space 5000", "tecla S 2000", "tecla S 2000", "tecla Space 5000", "tecla Space 4000",
             "tecla S 1500", "tecla S 1500", "captura e1_lista", "tecla Space 3000", "captura e2_equipada",
             "tecla P 3000", "tecla W 1200", "tecla W 1200", "tecla W 1200", "captura e3_bandeja",
             "tecla Backspace 3000", "captura e4_quitada"]
    time.sleep(2.5)
    P.ejecutar_dsl(j, pasos)
    misma_partida(j, antes, "con la capsula unica equipada y quitada en Edit Skills")
    for _ in range(4):
        j.tecla("P", 2500)
    j.captura("e5_fuera")
    misma_partida(j, antes, "al salir del editor")


def _wl14_ranuras(j, par):
    pasos = ["tecla S 1200", "tecla Space 0", "esperar_pantalla 14 10", "esperar 2500", "tecla Space 8000",
             "tecla Space 5000", "tecla S 2000", "tecla S 2000", "tecla Space 5000", "tecla Space 4000",
             "captura r1_lista"]
    pasos += ["tecla S 1200"] * 5 + ["tecla Space 2500", "captura r2_unica_3_huecos"]   # la unica: 3 de 7
    pasos += ["tecla W 1200"] * 5                      # vuelta a Kaioken
    for k in range(4):                                 # Kaioken, SSJ, SSJ2, SSJ3: 1 hueco cada una -> 7 de 7
        pasos += ["tecla Space 2500", "tecla S 1500"]
    pasos += ["captura r3_cursor_ssj4", "tecla Space 2500", "captura r4_ssj4_no_cabe",   # 8 de 7: no
              "tecla P 3000", "captura r5_bandeja"]
    time.sleep(2.5)
    P.ejecutar_dsl(j, pasos)


ESC = {
    "goku": (_goku, {"DBZ3_TEST_BATTLE": "0,7,2", "DBZ3_TEST_CAPS": "1984", "DBZ3_TEST_INV": "1,2,3,4,5"}),
    "parcial": (_parcial, {"DBZ3_TEST_BATTLE": "0,7,2", "DBZ3_TEST_CAPS": "1984", "DBZ3_TEST_INV": "1,2"}),
    "sdbh": (_sdbh, {"DBZ3_TEST_BATTLE": "50,7,2", "DBZ3_TEST_CAPS": "2034"}),
    "inicial": (_inicial, {"DBZ3_TEST_BATTLE": "0,7,2", "DBZ3_TEST_CAPS": "1984", "DBZ3_TEST_INV": "1,2,3",
                           "DBZ3_START_FORM": "3"}),
    "inicial_control": (lambda j, p: _inicial(j, {"control": "1"}),
                        {"DBZ3_TEST_BATTLE": "0,7,2", "DBZ3_TEST_CAPS": "1", "DBZ3_TEST_INV": "1,2,3",
                         "DBZ3_START_FORM": "3"}),
    "tag": (_tag, {"DBZ3_TEST_BATTLE": "0,7,2", "DBZ3_TAG_TEST": "0,50", "DBZ3_TEST_INV": "1,2,3,4,5",
                   "DBZ3_TEST_FORMAS": "50"}),
    "teclas": (_teclas, {"DBZ3_TEST_BATTLE": "0,7,2", "DBZ3_TEST_CAPS": "1", "DBZ3_TEST_FORMAS_AUTO": ""}),
    # P+K+G de verdad: pulsado en el registro del mando 1 (DBZ3_TEST_PKG, desde el gancho de combate.cpp) con el
    # rival quieto (muneco de combate.cpp); sin AUTO: sube el juego por su entrada nativa
    "pkg_unica": (lambda j, p: (j.log(r"capsula unica 1984 -> formas 3E", 90), subir(j, 4, "pkg_unica")),
                  {"DBZ3_TEST_BATTLE": "0,7,2", "DBZ3_TEST_CAPS": "1984", "DBZ3_TEST_INV": "1,2,3,4,5",
                   "DBZ3_TEST_FORMAS_AUTO": "", "DBZ3_TEST_PKG": "1", "DBZ3_COMBATE_MUNECO": "quieto"}),
    "pkg_kaioken": (lambda j, p: (subir(j, 1, "pkg_kaioken"), j.log(r"prueba: tope en la forma 1 de 6", 30),
                                  j.captura("pkg_kaioken_tope")),
                    {"DBZ3_TEST_BATTLE": "0,7,2", "DBZ3_TEST_CAPS": "1", "DBZ3_TEST_FORMAS_AUTO": "",
                     "DBZ3_TEST_PKG": "1", "DBZ3_COMBATE_MUNECO": "quieto"}),
    # WL-14 (con P+K+G de verdad; DBZ3_TEST_INV no toca la partida)
    "wl14_combate": (_wl14_combate, {"DBZ3_TEST_BATTLE": "0,7,2", "DBZ3_TEST_CAPS": "1984",
                                     "DBZ3_TEST_INV": "1,2,3,4,5", "DBZ3_TEST_FORMAS_AUTO": "", "DBZ3_TEST_PKG": "1",
                                     "DBZ3_COMBATE_MUNECO": "quieto"}),
    # modo 1 (Duelo): el modo 5 ya quita las >= 595 el solo (sub_820FE3D8); el 1 no, y ahi actua el gancho
    "wl14_sin_mod": (_wl14_sin_mod, {"DBZ3_TEST_BATTLE": "0,7,2", "DBZ3_TEST_CAPS": "1,1984", "DBZ3_HUB_MODE": "1",
                                     "DBZ3_TEST_FORMAS_AUTO": "", "DBZ3_TEST_PKG": "1",
                                     "DBZ3_COMBATE_MUNECO": "quieto"}),
    "wl14_limites": (_wl14_limites, {"DBZ3_TEST_BATTLE": "0,7,2", "DBZ3_TEST_CAPS": "1984", "DBZ3_TEST_INV": "1,2",
                                     "DBZ3_TEST_FORMAS_AUTO": "", "DBZ3_TEST_PKG": "1", "DBZ3_TEST_KI_FALTA": "1",
                                     "DBZ3_COMBATE_MUNECO": "quieto"}),
    "wl14_edit": (_wl14_edit, {"DBZ3_TEST_INV": "1,2,3,4,5", "DBZ3_TEST_FORMAS": ""}),
    "wl14_ranuras": (_wl14_ranuras, {"DBZ3_TEST_INV": "1,2,3,4,5", "DBZ3_TEST_OWN": "1,2,3,4,5",
                                     "DBZ3_TEST_FORMAS": ""}),
}


def main():
    import hashlib  # noqa: PLC0415
    import shutil  # noqa: PLC0415
    esc = sys.argv[1]
    copia = sys.argv[2] if len(sys.argv) > 2 else "transformaciones_test"
    fn, env = ESC[esc]
    dest = P.BASE / copia
    mod = dest / "mods" / "_forma_unica"
    if esc == "wl14_combate" and mod.exists():       # que el juego la rehaga (borrar/regenerar)
        aparte = dest / "mods_respaldo" / ("_forma_unica_wl14_%d" % int(time.time()))
        aparte.parent.mkdir(exist_ok=True)
        shutil.move(str(mod), str(aparte))
        print("  _forma_unica apartada a %s: el juego debe rehacerla" % aparte)
    marca = mod / ".disabled"
    if esc == "wl14_sin_mod":                         # sin capsula unica (como sin nombres o en ISO)
        marca.write_text("prueba WL-14\n")
    datos = dest / DATA_BIN
    h0 = hashlib.sha1(datos.read_bytes()).hexdigest() if datos.exists() else None
    P.ESCENARIOS["formas_" + esc] = ("transformaciones: " + esc, lambda p: dict(BASE_ENV, **env), fn)
    try:
        r = P.correr(copia, "formas_" + esc)
    finally:
        if esc == "wl14_sin_mod" and marca.exists():
            marca.unlink()                            # marcador propio de la prueba
    h1 = hashlib.sha1(datos.read_bytes()).hexdigest() if datos.exists() else None
    print("  data.bin antes %s despues %s: %s" % (h0, h1, "IGUAL" if h0 == h1 else "DISTINTO"))
    return 0 if r["ok"] and h0 == h1 else 1


if __name__ == "__main__":
    sys.exit(main())
