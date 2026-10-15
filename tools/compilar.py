#!/usr/bin/env python3
"""compilar.py - compila el exe dual con un candado compartido (varios agentes a la vez, uno compila).

  python tools/compilar.py            # espera su turno, compila dbz3 (out/build/win-amd64-dual) y copia las DLL canonicas

El candado es la carpeta D:\\DBZ3HD\\_compilando (mkdir atomico). Si un compilado anterior murio, el candado se
considera viejo a los 30 min. Las DLL canonicas (rexglue-sdk-0.10/out/win-amd64-baseline) se copian al terminar:
el build las pisa con otras antiguas (AGENTS.md s4/s7).
"""
import os
import shutil
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, "out", "build", "win-amd64-dual")
DLLS = os.path.join(ROOT, "rexglue-sdk-0.10", "out", "win-amd64-baseline")
LOCK = r"D:\DBZ3HD\_compilando"


def main():
    t0 = time.time()
    while True:
        try:
            os.mkdir(LOCK)
            break
        except FileExistsError:
            if time.time() - os.path.getmtime(LOCK) > 1800:   # candado de un compilado que murio
                os.rename(LOCK, LOCK + "_viejo_%d" % int(time.time()))
                continue
            if time.time() - t0 > 3600:
                print("compilar: una hora esperando el candado %s" % LOCK)
                return 2
            time.sleep(5)
    try:
        print("compilar: turno tras %.0f s" % (time.time() - t0), flush=True)
        r = subprocess.run(["cmake", "--build", BUILD, "--target", "dbz3"], capture_output=True, text=True)
        out = (r.stdout or "") + (r.stderr or "")
        errs = [l for l in out.splitlines() if "error" in l.lower()]
        for name in ("rexruntime.dll", "rexgpu-xenos.dll"):
            shutil.copy2(os.path.join(DLLS, name), os.path.join(BUILD, name))
        print("\n".join(errs[:40]) if errs else out.strip().splitlines()[-1] if out.strip() else "")
        print("compilar: %s (%.0f s)" % ("OK" if r.returncode == 0 else "FALLO", time.time() - t0))
        return r.returncode
    finally:
        os.rmdir(LOCK)


if __name__ == "__main__":
    sys.exit(main())
