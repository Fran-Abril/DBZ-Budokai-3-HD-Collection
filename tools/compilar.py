r"""Compila el juego (out/build/win-amd64-dual) de uno en uno cuando varios agentes trabajan a la vez.

  python tools\compilar.py            (espera su turno, compila y deja solo las lineas de error)

Turno: carpeta E:\DBZ3HD_trabajo\coord\compilando (mkdir es atomico); si lleva mas de 20 min, se da por muerta y se
aparta (nunca se borra). Codigo de salida = el de cmake. Si falla en un archivo que no es tuyo, espera y reintenta:
otro agente lo esta editando.
"""
import os
import subprocess
import sys
import time
from pathlib import Path

R = Path(__file__).resolve().parent.parent
TURNO = Path(r"E:\DBZ3HD_trabajo\coord\compilando")


def main():
    TURNO.parent.mkdir(parents=True, exist_ok=True)
    t0 = time.time()
    while True:
        try:
            TURNO.mkdir()
            break
        except FileExistsError:
            if time.time() - TURNO.stat().st_mtime > 20 * 60:
                try:
                    TURNO.rename(TURNO.with_name(f"compilando_viejo_{int(time.time())}"))
                except OSError:
                    pass
            time.sleep(3)
    try:
        print(f"turno de compilar tras {time.time() - t0:.0f} s", flush=True)
        r = subprocess.run(["cmake", "--build", str(R / "out" / "build" / "win-amd64-dual")], capture_output=True,
                           text=True, errors="replace")
        lineas = [l for l in (r.stdout + r.stderr).splitlines() if "error" in l.lower() or "FAILED" in l]
        print("\n".join(lineas[:40]) if lineas else "sin errores", flush=True)
        print(f"codigo {r.returncode}", flush=True)
        return r.returncode
    finally:
        TURNO.rmdir()


if __name__ == "__main__":
    sys.exit(main())
