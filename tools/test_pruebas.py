"""Comprobacion sin juego de tools/pruebas.py:  python tools\\test_pruebas.py"""
import re
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import pruebas as P  # noqa: E402

# escenarios integrados y de la bateria: todos cargan y piden los hooks que tocan
for nombre in P.ESCENARIOS:
    desc, fn, env = P.cargar_escenario(nombre, {})
    assert desc and callable(fn), nombre
assert P.cargar_escenario("batalla", {})[2] == {"DBZ3_TEST_BATTLE": "0,7,2", "DBZ3_TEST_RINGOUT": "1"}
assert P.cargar_escenario("batalla", {"ids": "3,4,1"})[2]["DBZ3_TEST_BATTLE"] == "3,4,1"
assert P.cargar_escenario("tag_swap", {"swap": "0"})[2] == {"DBZ3_TAG_SLOW": "3", "DBZ3_TEST_KO": "1"}
assert P.cargar_escenario("campana", {})[2] == {"DBZ3_TEST_RINGOUT": "1"}
assert all(i in ("freezer_cap2", "perfil_nuevo") for _, i, _ in P.SMOKE)
assert {s[0] for s in P.SMOKE} <= set(P.ESCENARIOS)

# .toml: pasos validos se aceptan, un verbo desconocido se rechaza antes de tocar el juego
with tempfile.TemporaryDirectory() as d:
    ok = Path(d) / "ok.toml"
    ok.write_text('env = ["DBZ3_TAG_SLOW=2"]\npasos = ["tecla Space 100", "esperar_pantalla 5,7 3"]\n')
    assert P.cargar_escenario(str(ok), {})[2] == {"DBZ3_TAG_SLOW": "2"}
    malo = Path(d) / "malo.toml"
    malo.write_text('pasos = ["saltar 3"]\n')
    try:
        P.cargar_escenario(str(malo), {})
        raise AssertionError("verbo desconocido aceptado")
    except SystemExit:
        pass

# deteccion de fallos: los tres mensajes reales del runtime (main.cpp:976, xmemory.cpp:547/628)
for l in ["UNHANDLED EXCEPTION: Code=0xC0000005 Addr=0x1",
          "Unhandled guest access violation: read of guest 0x00000000 (host 0x1) on thread 0x2",
          "Unhandled guest physical write fault: guest=1 host=2 phys=3"]:
    assert re.search(P.ERRORES, l), l
assert not re.search(P.ERRORES, "dbz3: perf fps=60.0 frames=300")
log = "\n".join([f"linea {i}" for i in range(40)] + ["dbz3: perf fps=1", "UNHANDLED EXCEPTION: Code=0xC0000005"])
ext = P.informe_fallo(log, ["UNHANDLED EXCEPTION: Code=0xC0000005"])
assert ext.startswith("-- errores --") and "perf fps" not in ext and "linea 39" in ext and "linea 5\n" not in ext
assert P.informe_fallo("", []) == "(sin log del juego)"

# tabla: una fila por prueba y el recuento final
t = P.tabla([{"ok": True, "nombre": "smoke_a/menu", "dur": 20.0, "menu": 12.0, "cierre": 1.0, "msg": ""},
             {"ok": False, "nombre": "smoke_b/batalla", "dur": 0.0, "msg": "copia ocupada"}], 21)
assert "PASS  smoke_a/menu" in t and "FAIL  smoke_b/batalla" in t and "1/2 PASS en 21 s" in t
print("test_pruebas: OK")
