#!/usr/bin/env python3
r"""grabar.py - clips de video CON SONIDO del juego para los videos de las versiones.

Lanza una prueba de tools/pruebas.py (copia aislada en D:\DBZ3HD) y, mientras corre, graba la ventana del juego con
ffmpeg (gdigrab + NVENC). El sonido no se puede grabar del sistema (no hay dispositivo de "lo que suena"), asi que el
propio juego lo guarda (cvar dbz3_audio_dump del SDK, float32 estereo 48 kHz, aunque este silenciado) y al final se
junta con la imagen usando las horas de inicio de los dos (la del audio sale en el log del juego).

  python tools\grabar.py COPIA ESCENARIO NOMBRE SEGUNDOS [--retraso S] [--par k=v] [--env K=V]
  python tools\grabar.py iso_test D:\tmp\rec\com_tag.toml 01_com_tagteam_a 90 --retraso 20

SEGUNDOS es el maximo (para antes si el juego se cierra). Salida: video/clips/proxima/NOMBRE.mp4 (y NOMBRE_v.mp4 + NOMBRE.f32 de trabajo). Nunca mata ffmpeg a medias (el mp4 se
quedaria sin indice): espera a que acabe sus SEGUNDOS. La ventana del juego tiene que estar a la vista mientras graba
(gdigrab copia la pantalla: si otra ventana la tapa, sale esa ventana).
"""
import argparse
import ctypes
import ctypes.wintypes
import os
import re
import subprocess
import sys
import time
from pathlib import Path

R = Path(__file__).resolve().parent.parent
SALIDA = R / "video" / "clips" / "proxima"
TITULO = "Dragon Ball Z Budokai 3 HD"
U32 = ctypes.WinDLL("user32", use_last_error=True)


def ventana():
    return U32.FindWindowW(None, TITULO)


def zona(h):
    """Area cliente de la ventana en pixeles fisicos de la pantalla (x, y, ancho, alto pares). gdigrab con title=
    da negro con el swapchain de D3D12: se graba esa zona del escritorio."""
    try:
        ctypes.WinDLL("shcore").SetProcessDpiAwareness(2)   # coordenadas reales con la pantalla al 125 %
    except OSError:
        pass
    r = ctypes.wintypes.RECT()
    U32.GetClientRect(h, ctypes.byref(r))
    pt = ctypes.wintypes.POINT(0, 0)
    U32.ClientToScreen(h, ctypes.byref(pt))
    return pt.x, pt.y, (r.right - r.left) // 2 * 2, (r.bottom - r.top) // 2 * 2


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("copia")
    ap.add_argument("escenario")
    ap.add_argument("nombre")
    ap.add_argument("segundos", type=int)
    ap.add_argument("--retraso", type=float, default=0.0, help="segundos tras abrirse la ventana antes de grabar")
    ap.add_argument("--par", action="append", default=[])
    ap.add_argument("--env", action="append", default=[])
    ap.add_argument("--fps", type=int, default=60)
    a = ap.parse_args()
    SALIDA.mkdir(parents=True, exist_ok=True)
    audio = SALIDA / (a.nombre + ".f32")
    video = SALIDA / (a.nombre + "_v.mp4")
    final = SALIDA / (a.nombre + ".mp4")
    cmd = [sys.executable, str(R / "tools" / "pruebas.py"), "correr", a.copia, a.escenario,
           "--env", "REX_DBZ3_AUDIO_DUMP=" + str(audio)]
    for p in a.par:
        cmd += ["--par", p]
    for e in a.env:
        cmd += ["--env", e]
    prueba = subprocess.Popen(cmd, cwd=str(R))
    t0 = time.time()
    while not ventana():
        if prueba.poll() is not None or time.time() - t0 > 600:
            sys.exit("no se abrio la ventana del juego")
        time.sleep(0.25)
    time.sleep(a.retraso)
    U32.SetForegroundWindow(ventana())
    x, y, w, h = zona(ventana())
    inicio_v = int(time.time() * 1000)
    rec = subprocess.Popen(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-f", "gdigrab", "-framerate",
                            str(a.fps), "-draw_mouse", "0", "-offset_x", str(x), "-offset_y", str(y), "-video_size",
                            "%dx%d" % (w, h), "-i", "desktop", "-t", str(a.segundos),
                            "-c:v", "h264_nvenc", "-cq", "19", "-pix_fmt", "yuv420p", str(video)],
                           stdin=subprocess.PIPE)
    # SEGUNDOS es el maximo: si el juego se cierra antes, ffmpeg para con "q" (cierra bien el mp4; matarlo lo
    # dejaria sin indice) y no graba lo que haya detras de la ventana (el escritorio del usuario)
    while rec.poll() is None:
        if not ventana():
            try:
                rec.stdin.write(b"q")
                rec.stdin.flush()
            except OSError:
                pass
            break
        time.sleep(0.2)
    rec.wait()          # nunca se corta a medias
    prueba.wait()
    # la hora de inicio del audio: la linea del volcado en el log mas nuevo de la copia
    logs = sorted((Path(r"D:\DBZ3HD") / a.copia / "logs").glob("dbz3_*.log"), key=lambda p: p.stat().st_mtime)
    inicio_a = None
    for log in reversed(logs[-3:]):
        m = re.search(r"dbz3 audio: volcado abierto .*?\(inicio (\d+) ms", log.read_text("utf-8", "replace"))
        if m:
            inicio_a = int(m.group(1))
            break
    if inicio_a is None or not audio.exists() or audio.stat().st_size == 0:
        print("sin audio (cvar dbz3_audio_dump no activo?): queda solo la imagen en", video)
        return
    desfase = (inicio_v - inicio_a) / 1000.0   # cuanto audio sobra al principio
    # desfase > 0: el audio empezo antes (se recorta); < 0: empezo despues (el juego arranca tras el launcher: se
    # retrasa con silencio delante)
    corte = ["-ss", "%.3f" % desfase] if desfase >= 0 else ["-itsoffset", "%.3f" % -desfase]
    subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-i", str(video), "-f", "f32le", "-ar", "48000",
                    "-ac", "2", *corte, "-i", str(audio), "-map", "0:v", "-map", "1:a", "-c:v", "copy",
                    "-af", "apad", "-c:a", "aac", "-b:a", "192k", "-shortest", str(final)], check=True)
    print("clip:", final, "(desfase audio %.3f s)" % desfase)


if __name__ == "__main__":
    main()
