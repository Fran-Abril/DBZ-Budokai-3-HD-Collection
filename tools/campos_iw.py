#!/usr/bin/env python3
r"""Movido a "mod center hd/campos_iw.py" (viaja en el Kit de modding y lo usa el launcher al pulsar Jugar; frente R8,
2026-10-16). Este archivo solo lo lanza, para que los comandos de siempre (python tools\campos_iw.py ...) sigan valiendo."""
import runpy
import sys
from pathlib import Path

_REAL = Path(__file__).resolve().parent.parent / "mod center hd" / "campos_iw.py"
if __name__ == "__main__":        # no en los procesos hijos de "auto" (multiprocessing vuelve a cargar este archivo)
    sys.argv[0] = str(_REAL)
    runpy.run_path(str(_REAL), run_name="__main__")
