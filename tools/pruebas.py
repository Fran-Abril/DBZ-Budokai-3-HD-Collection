r"""Pruebas del juego en copias aisladas (D:\DBZ3HD\<copia>). Tu partida real no se toca.

Comandos (desde la carpeta del proyecto):
  python tools\pruebas.py prep du_test
  python tools\pruebas.py instantanea guardar du_test NOMBRE
  python tools\pruebas.py instantanea cargar du_test NOMBRE
  python tools\pruebas.py correr du_test menu
  python tools\pruebas.py correr du_test du_freezer_nuevo --par up=2 --par iter=20
  python tools\pruebas.py correr tag_test tag_swap --env DBZ3_TAG_SLOW=2
  python tools\pruebas.py correr du_test mi_prueba.toml --instantanea NOMBRE
  python tools\pruebas.py paralelo menu@du_test tag_swap@tag_test   (hasta 3 a la vez)
  python tools\pruebas.py todo                      (bateria rapida en smoke_a/b/c, tabla PASS/FAIL)
  python tools\pruebas.py todo --solo menu,batalla --max 2
  python tools\pruebas.py biblioteca                (crea las instantaneas base y perfil_nuevo)

Escenarios: menu, batalla, campana, du_freezer_nuevo, du_continuar, tag_swap, o un .toml con:
  descripcion = "..."
  env = ["DBZ3_TAG_SLOW=3"]
  pasos = ["tecla Space 4000", "esperar_pantalla 5,7 30", "captura rueda", 'log "texto" 20']
Pasos: tecla TECLA [ms] (TECLA:400 = mantener 400 ms) | esperar ms | esperar_pantalla N[,M] [seg]
       captura NOMBRE | dump | write | log "regex" [seg]
Nunca borra: lo que reemplaza se mueve a D:\DBZ3HD\_instantaneas\_respaldo\.
Nunca cierra juegos ajenos: si una copia tiene un juego abierto, se para y avisa.
"""
import argparse
import ctypes
import ctypes.wintypes as wt
import glob
import os
import queue
import re
import shlex
import shutil
import subprocess
import sys
import time
import tomllib
from concurrent.futures import ThreadPoolExecutor
from contextlib import contextmanager
from datetime import datetime
from pathlib import Path

try:
    from PIL import Image
except ImportError:  # sin Pillow la captura se guarda como .bmp
    Image = None

R = Path(__file__).resolve().parent.parent          # carpeta del proyecto
BASE = Path(r"D:\DBZ3HD")                           # donde viven las copias
INST = BASE / "_instantaneas"
RESPALDO = INST / "_respaldo"
PRUEBAS = BASE / "_pruebas"                         # capturas y resultados de cada prueba
EXE = R / "out" / "build" / "win-amd64-dual" / "dbz3.exe"
DLLS = R / "rexglue-sdk-0.10" / "out" / "win-amd64-baseline"
GBASE = 0x200000000
PANTALLA = GBASE + 0x824BA110                       # byte con el id de pantalla (4 titulo, 5 menu, 7 rueda...)
ESTRUCTAS = (0x8247A8E0, 0x824BA110)                # bloques de DU (antes en du_mem.py)
BLOQUE, RANURA = 4624, 22
MAX_PARALELO = 3
LANZANDO = BASE / "_lanzando"                       # candado entre procesos: contar juegos + lanzar es atomico
ERRORES = (r"access violation|UNHANDLED EXCEPTION|physical write fault"   # main.cpp:976, xmemory.cpp:547/628
           r"|std::terminate called")                                        # main.cpp (set_terminate)
PAPELERA = Path(r"D:\_PARA_BORRAR_CLAUDE") / "pruebas_respaldos"   # respaldos automaticos viejos (no se borran)
MAX_RESPALDOS = 5                                   # por copia: los 5 mas recientes se quedan en _respaldo
VERBOS = {"tecla", "esperar", "esperar_pantalla", "captura", "dump", "write", "log"}

# Ajustes de la copia de prueba (sobrescriben dbz3_user.toml; el resto se conserva)
OVERRIDES = {
    "dbz3_skip_launcher": "true", "audio_mute": "true", "dbz3_mute": "true",
    "mnk_background_input": "true", "mnk_mode": "true", "dbz3_dim_unfocused": "false",
    "dbz3_update_check": "false", "fullscreen": "false", "dbz3_fullscreen_mode": '"windowed"',
    "dbz3_resolution_scale": "1", "dbz3_tag_team_enabled": "true",
    "dbz3_texture_packs_on": "false",               # el pack HD no se prueba aqui y alarga la carga
}

K32 = ctypes.WinDLL("kernel32", use_last_error=True)
K32.OpenProcess.argtypes = [wt.DWORD, wt.BOOL, wt.DWORD]
K32.OpenProcess.restype = ctypes.c_void_p
K32.WaitForSingleObject.argtypes = [ctypes.c_void_p, wt.DWORD]
K32.WaitForSingleObject.restype = wt.DWORD
K32.CloseHandle.argtypes = [ctypes.c_void_p]


class Fallo(Exception):
    pass


def ts():
    return datetime.now().strftime("%Y%m%d-%H%M%S")


def _kv(x):
    k, sep, v = x.partition("=")
    if not sep:
        raise SystemExit(f"se esperaba CLAVE=VALOR, recibido: {x}")
    return k.strip(), v


def leer(h, addr, n):
    if not h:
        return None
    buf = ctypes.create_string_buffer(n)
    got = ctypes.c_size_t()
    if not K32.ReadProcessMemory(ctypes.c_void_p(h), ctypes.c_void_p(addr), buf, n, ctypes.byref(got)):
        return None
    return buf.raw[:got.value]


def cerrar_pid(pid, espera=10):
    """Cierre amable (como CloseMainWindow); si no sale en 'espera' s, se mata."""
    subprocess.run(["taskkill", "/PID", str(pid)], capture_output=True)
    h = K32.OpenProcess(0x00100000, False, pid)     # SYNCHRONIZE
    if not h:
        return
    try:
        if K32.WaitForSingleObject(ctypes.c_void_p(h), int(espera * 1000)) == 0x102:   # WAIT_TIMEOUT
            subprocess.run(["taskkill", "/PID", str(pid), "/F"], capture_output=True)
            K32.WaitForSingleObject(ctypes.c_void_p(h), 5000)
    finally:
        K32.CloseHandle(ctypes.c_void_p(h))


def pids_de_copia(dest):
    """Solo los dbz3.exe que corren desde esa copia (nunca los de otra copia)."""
    ps = ("Get-CimInstance Win32_Process -Filter \"Name='dbz3.exe'\" | "
          "ForEach-Object { '{0}|{1}' -f $_.ProcessId, $_.ExecutablePath }")
    out = subprocess.run(["powershell", "-NoProfile", "-Command", ps], capture_output=True, text=True).stdout
    pref = str(dest).rstrip("\\").lower() + "\\"
    pids = []
    for linea in out.splitlines():
        pid, sep, ruta = linea.partition("|")
        if sep and ruta.strip().lower().startswith(pref):
            pids.append(int(pid))
    return pids


def _vivo(pid):
    h = K32.OpenProcess(0x1000, False, pid)                     # QUERY_LIMITED_INFORMATION
    if not h:
        return False
    code = wt.DWORD()
    K32.GetExitCodeProcess(h, ctypes.byref(code))
    K32.CloseHandle(h)
    return code.value == 259                                    # STILL_ACTIVE


@contextmanager
def copia_en_uso(dest):
    """Una prueba por copia a la vez, de principio a fin. comprobar_libre mira antes de la cola: dos agentes con la
    misma copia esperaban turno y abrian los dos (teclas y ficheros pisados, tag_swap en FAIL, 2026-10-15)."""
    lock, t0 = BASE / f"_en_uso_{dest.name}", time.time()
    while True:
        try:
            lock.mkdir()
            (lock / "pid").write_text(str(os.getpid()))
            break
        except FileExistsError:
            try:
                pid = int((lock / "pid").read_text())
            except (OSError, ValueError):
                pid = 0
            viejo = not _vivo(pid) if pid else time.time() - lock.stat().st_mtime > 60
            if viejo:                                           # de una prueba que murio: se aparta, no se borra
                try:
                    RESPALDO.mkdir(parents=True, exist_ok=True)
                    lock.rename(RESPALDO / f"_en_uso_viejo_{dest.name}_{ts()}_{os.getpid()}")
                except OSError:
                    pass
                continue
            if time.time() - t0 > 3600:
                raise SystemExit(f"una hora esperando la copia {dest.name} (la usa el proceso {pid})")
            time.sleep(5)
    try:
        yield
    finally:
        (lock / "pid").unlink(missing_ok=True)
        lock.rmdir()


def comprobar_libre(dest):
    """No cierra juegos ajenos: si la copia esta en uso (otra prueba o agente), se para y avisa."""
    pids = pids_de_copia(dest)
    if pids:
        raise SystemExit(f"la copia {dest.name} tiene un juego abierto (pid {pids}). No lo cierro: "
                         "espera a que termine o cierralo tu, y repite.")


def mover(src, dst):
    """Lo reemplazado no se borra: se mueve a _respaldo."""
    RESPALDO.mkdir(parents=True, exist_ok=True)
    shutil.move(str(src), str(dst))
    print(f"  movido a respaldo: {dst}", flush=True)


def escribir_toml(ruta):
    ov = dict(OVERRIDES, dbz3_game_dir=f"'{R}'")
    lineas = ruta.read_text(encoding="utf-8", errors="surrogateescape").splitlines() if ruta.exists() else []
    out, vistos = [], set()
    for l in lineas:
        m = re.match(r"\s*([A-Za-z0-9_\-]+)\s*=", l)
        if m and m.group(1) in ov:
            out.append(f"{m.group(1)} = {ov[m.group(1)]}")
            vistos.add(m.group(1))
        else:
            out.append(l)
    out += [f"{k} = {v}" for k, v in ov.items() if k not in vistos]
    ruta.write_text("\n".join(out) + "\n", encoding="utf-8", errors="surrogateescape")


def preparar(dest, desde=None, exe=EXE):
    """Crea o refresca la copia: exe y DLL canonicas nuevas, toml de pruebas, controles limpios."""
    if not exe.exists():
        raise SystemExit(f"no existe el exe {exe}: compila primero")
    comprobar_libre(dest)
    if not (dest / "dbz3.exe").exists():
        src = Path(desde) if desde else BASE / "du_test"
        if not src.exists():
            raise SystemExit(f"no existe la copia origen {src}")
        print(f"  copiando {src} -> {dest} (sin logs ni shader_cache)", flush=True)
        # /R:1 /W:1: si el origen tiene un juego abierto, robocopy reintentaba un archivo bloqueado 1 millon de
        # veces cada 30 s (visto 2026-10-15: smoke_b, smoke_c y hub_test colgados copiando de du_test)
        r = subprocess.run(["robocopy", str(src), str(dest), "/E", "/R:1", "/W:1", "/XD", str(src / "logs"),
                            str(src / "shader_cache"), "/NDL", "/NJH", "/NJS", "/NP"],
                           capture_output=True, text=True)
        if r.returncode >= 8:
            fallidos = [l.strip() for l in r.stdout.splitlines() if "ERROR" in l][:5]
            print(f"  aviso: robocopy codigo {r.returncode}, archivos en uso sin copiar (el exe, las DLL y el toml "
                  f"se ponen igual): {fallidos}", flush=True)
    (dest / "logs").mkdir(exist_ok=True)
    shutil.copy2(exe, dest / "dbz3.exe")
    for n in ("rexruntime.dll", "rexgpu-xenos.dll"):
        shutil.copy2(DLLS / n, dest / n)
    for n in ("dbz3_input.req", "dbz3_shot.req", "dbz3_shot.bmp"):   # senales de control viejas
        (dest / n).unlink(missing_ok=True)
    escribir_toml(dest / "dbz3_user.toml")


def podar_respaldos(copia):
    """Respaldos automaticos de una copia (_auto y _antes_de_cargar_*): se quedan los MAX_RESPALDOS mas recientes;
    los demas se mueven (nunca se borran) a D:\\_PARA_BORRAR_CLAUDE\\pruebas_respaldos."""
    if not RESPALDO.exists():
        return
    suyos = sorted(d for d in RESPALDO.iterdir() if d.is_dir() and re.fullmatch(
        r"\d{8}-\d{6}_" + re.escape(copia) + r"_(auto|antes_de_cargar_.+)", d.name))
    for d in suyos[:-MAX_RESPALDOS]:
        try:
            PAPELERA.mkdir(parents=True, exist_ok=True)
            shutil.move(str(d), str(PAPELERA / d.name))
        except OSError:
            pass                                    # otro agente lo esta usando o ya lo movio


def respaldar(dest):
    """Copia (no mueve) los datos de partida antes de correr: por si un escenario guarda encima."""
    ud = dest / "user_data"
    if ud.exists():
        shutil.copytree(ud, RESPALDO / f"{ts()}_{dest.name}_auto")
    podar_respaldos(dest.name)


def guardar_instantanea(copia, nombre):
    ud = BASE / copia / "user_data"
    if not ud.exists():
        raise SystemExit(f"no hay datos de partida en {ud}")
    comprobar_libre(BASE / copia)
    dst = INST / nombre / "user_data"
    if dst.exists():
        mover(dst, RESPALDO / f"{ts()}_instantanea_previa_{nombre}")
    shutil.copytree(ud, dst)
    print(f"instantanea '{nombre}' guardada desde {copia}", flush=True)


def cargar_instantanea(copia, nombre):
    src = INST / nombre / "user_data"
    if not src.exists():
        raise SystemExit(f"no existe la instantanea '{nombre}' ({src})")
    comprobar_libre(BASE / copia)
    ud = BASE / copia / "user_data"
    if ud.exists():
        mover(ud, RESPALDO / f"{ts()}_{copia}_antes_de_cargar_{nombre}")
    shutil.copytree(src, ud)
    podar_respaldos(copia)
    print(f"instantanea '{nombre}' cargada en {copia}", flush=True)


class Juego:
    """Un juego lanzado en una copia: lectura de pantalla, teclas, capturas, log."""

    def __init__(self, copia, nombre):
        self.dest = BASE / copia
        self.nombre = nombre
        self.run = PRUEBAS / f"{ts()}_{nombre.replace('/', '_')}"
        self.run.mkdir(parents=True, exist_ok=True)
        self.proc = None
        self.h = None
        self.t0 = time.time()
        self.vistas = set()
        self.traza = []
        self.t_cupo = 0.0
        self.logs_antes = None                          # logs de la copia antes de lanzar (texto_log)

    def info(self, msg):
        print(f"  [{time.time() - self.t0:6.1f} s] {self.nombre}: {msg}", flush=True)

    def lanzar(self, env_extra):
        # como mucho 3 juegos abiertos a la vez en el PC (varios agentes probando): espera su turno.
        # El candado LANZANDO hace atomico "contar + lanzar" entre agentes (antes dos podian ver 2 y abrir 4).
        env = {k: v for k, v in os.environ.items() if not k.startswith("DBZ3_")}
        env.update(SDL_WINDOW_ACTIVATE_WHEN_SHOWN="0", SDL_WINDOW_ACTIVATE_WHEN_RAISED="0", DBZ3_NO_DIALOGS="1")
        env.update(env_extra)
        t_espera = time.time()
        while True:
            if time.time() - t_espera > 1800:
                raise Fallo("30 min esperando: ya hay 3 juegos abiertos")
            try:
                LANZANDO.mkdir()
            except FileExistsError:
                try:
                    if time.time() - LANZANDO.stat().st_mtime > 60:  # candado de un lanzador que murio
                        RESPALDO.mkdir(parents=True, exist_ok=True)
                        LANZANDO.rename(RESPALDO / f"_lanzando_viejo_{ts()}_{os.getpid()}")
                except OSError:                                       # otro agente lo movio o lo solto
                    pass
                time.sleep(1)
                continue
            try:
                n = subprocess.run(["tasklist", "/FI", "IMAGENAME eq dbz3.exe", "/NH"], capture_output=True,
                                   text=True).stdout.count("dbz3.exe")
                if n < MAX_PARALELO:
                    self.logs_antes = self._logs()      # el log de esta partida sera uno nuevo o uno que cambie
                    self.t0 = time.time()
                    # sin heredar la consola: si no, la llamada tarda hasta que el juego suelta esa salida
                    self.proc = subprocess.Popen([str(self.dest / "dbz3.exe"), f"--game_directory={R}"],
                                                 cwd=str(self.dest), env=env, stdin=subprocess.DEVNULL,
                                                 stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                    break
            finally:
                LANZANDO.rmdir()                                      # candado propio (carpeta vacia)
            time.sleep(5)
        self.t_cupo = self.t0 - t_espera
        self.h = K32.OpenProcess(0x0410, False, self.proc.pid)     # QUERY_INFORMATION | VM_READ
        if not self.h:
            raise Fallo("no pude leer la memoria del juego")

    def vivo(self):
        return self.proc is not None and self.proc.poll() is None

    def cerrar(self):
        if self.proc and self.proc.poll() is None:
            # copia desechable: 2 s de cortesia y fuera (el cierre amable tardaba >10 s tras
            # "Window close requested"); el juego guarda al pedirlo, no al salir
            cerrar_pid(self.proc.pid, 2)
            try:        # tras /F a veces tarda >5 s en desaparecer; la siguiente prueba en la copia lo veia vivo
                self.proc.wait(30)
            except subprocess.TimeoutExpired:
                self.info("el juego sigue vivo 30 s despues de cerrarlo")
        if self.h:
            K32.CloseHandle(ctypes.c_void_p(self.h))
            self.h = None

    def pantalla(self):
        b = leer(self.h, PANTALLA, 1)
        sc = b[0] if b else None
        if sc is not None:
            self.vistas.add(sc)
        if not self.traza or self.traza[-1][1] != sc:     # cambios de pantalla con su segundo (pantallas.txt)
            self.traza.append((round(time.time() - self.t0, 1), sc))
        return sc

    def tecla(self, k, espera=600):
        if not self.vivo():
            raise Fallo("el juego se cerro")
        req = self.dest / "dbz3_input.req"
        fin = time.time() + 2.5
        while req.exists() and time.time() < fin:       # esperar a que el juego consuma la anterior
            time.sleep(0.05)
        tmp = self.dest / "dbz3_input.req.tmp"
        tmp.write_text(k + "\n", encoding="ascii")
        os.replace(tmp, req)
        self.traza.append((round(time.time() - self.t0, 1), f"tecla {k}"))
        time.sleep(espera / 1000)

    def captura(self, nombre):
        bmp = self.dest / "dbz3_shot.bmp"
        bmp.unlink(missing_ok=True)
        (self.dest / "dbz3_shot.req").write_text("1", encoding="ascii")
        fin = time.time() + 6
        while time.time() < fin:
            if bmp.exists() and bmp.stat().st_size > 0:
                tam = bmp.stat().st_size
                time.sleep(0.15)                          # que el juego termine de escribirlo
                if bmp.exists() and bmp.stat().st_size == tam:
                    break
            time.sleep(0.05)
        else:
            raise Fallo(f"captura '{nombre}' sin respuesta del juego")
        out = self.run / f"{nombre}.png"
        if Image:
            with Image.open(bmp) as im:
                im.save(out)
        else:
            out = out.with_suffix(".bmp")
            shutil.copy(bmp, out)
        bmp.unlink(missing_ok=True)
        return out

    def captura_fallo(self):
        """Foto de como quedo la pantalla al fallar (si el juego sigue vivo); nunca tapa el fallo original."""
        try:
            if self.vivo():
                self.info(f"captura del fallo: {self.captura('zz_fallo')}")
        except Exception:
            pass

    def esperar_cambio(self, previa, seg=6):
        fin = time.time() + seg
        while time.time() < fin:
            sc = self.pantalla()
            if sc is not None and sc != previa:
                return sc
            time.sleep(0.5)
        return self.pantalla()

    def esperar_pantalla(self, ids, seg=60):
        fin = time.time() + seg
        while True:
            sc = self.pantalla()
            if sc in ids:
                return sc
            if not self.vivo():
                raise Fallo("el juego se cerro")
            if time.time() > fin:
                raise Fallo(f"no llego a la pantalla {sorted(ids)} en {seg} s (estaba en {sc})")
            time.sleep(0.25)

    def tecla_hasta(self, k, ids, intentos=4, seg=4):
        """Pulsa k hasta que la pantalla sea una de ids (la primera pulsacion a veces se pierde)."""
        for _ in range(intentos):
            if self.pantalla() in ids:
                return
            self.tecla(k, 0)
            try:
                self.esperar_pantalla(ids, seg)
                return
            except Fallo:
                if not self.vivo():
                    raise
        raise Fallo(f"'{k}' x{intentos} no llevo a la pantalla {sorted(ids)} (estaba en {self.pantalla()})")

    def tecla_hasta_log(self, k, regex, intentos=4, seg=4):
        """Pulsa k hasta que el log contenga regex (menus que no cambian el byte de pantalla)."""
        for _ in range(intentos):
            self.tecla(k, 0)
            fin = time.time() + seg
            while time.time() < fin:
                if re.search(regex, self.texto_log()):
                    return
                time.sleep(0.25)
        raise Fallo(f"'{k}' x{intentos} y el log sigue sin '{regex}'")

    def arrancar(self, seg=120, rapido=True, titulo=("Return", "Space")):
        """Espera el menu principal (pantalla 5) sondeando; en el titulo (4, el video de apertura ya va en ella)
        pulsa las teclas de 'titulo' por turnos cada 1,5 s (antes Space cada 6 s: el menu llegaba ~7 s tarde).
        Perfil sin partida: el titulo ofrece New Game / Load Game (cursor en Load): titulo=("Return","W","Space")."""
        fin = time.time() + seg
        n = 0
        while True:
            sc = self.pantalla()
            if sc == 5:
                return time.time() - self.t0
            if not self.vivo():
                raise Fallo("el juego se cerro al arrancar")
            if time.time() > fin:
                raise Fallo(f"no llego al menu principal en {seg} s (pantalla {sc})")
            if sc in (None, 0):                             # cargando
                time.sleep(0.25)
                continue
            if sc == 4 and rapido:
                self.tecla(titulo[n % len(titulo)], 0)
                n += 1
                self.esperar_cambio(sc, 1.5)
                continue
            self.tecla("Space" if sc == 4 else "Return", 0)
            self.esperar_cambio(sc, 6)

    def _logs(self):
        out = {}
        for f in glob.glob(str(self.dest / "logs" / "dbz3_*.log")):
            try:
                st = os.stat(f)
                out[f] = (st.st_size, st.st_mtime_ns)
            except OSError:
                pass
        return out

    def texto_log(self):
        """Log de ESTA partida: uno que no existia al lanzar o que ha cambiado desde entonces. Antes se filtraba
        por fecha (t0 - 5 s) y dos pruebas seguidas en la misma copia podian leer el log de la anterior."""
        ahora = self._logs()
        if self.logs_antes is None:                     # sin lanzar: nada de esta partida
            return ""
        fs = [f for f, v in ahora.items() if self.logs_antes.get(f) != v]
        if not fs:
            return ""
        try:
            return Path(max(fs, key=lambda f: ahora[f][1])).read_text(encoding="utf-8", errors="replace")
        except OSError:
            return ""

    def log(self, regex, seg=20):
        fin = time.time() + seg
        while not re.search(regex, self.texto_log()):
            if time.time() > fin:
                raise Fallo(f"el log no contiene '{regex}' tras {seg} s")
            time.sleep(0.5)
        self.info(f"log ok: {regex}")

    def bloque(self, cmd):
        """dump: guarda el bloque de DU de la casilla 22 en la carpeta de la prueba; write: lo restaura."""
        f = self.run / "bloque22.bin"
        h = K32.OpenProcess(0x0438, False, self.proc.pid)   # QUERY | VM_READ | VM_WRITE | VM_OPERATION
        if not h:
            raise Fallo("no pude abrir la memoria del juego para dump/write")
        try:
            addr = GBASE + ESTRUCTAS[0] + 336 + BLOQUE * RANURA
            if cmd == "dump":
                b = leer(h, addr, BLOQUE)
                if not b or len(b) < BLOQUE:
                    raise Fallo("dump sin respuesta del juego")
                f.write_bytes(b)
                self.info(f"dump {len(b)} bytes -> {f.name}")
            else:
                data = f.read_bytes()
                for s in ESTRUCTAS:
                    n = ctypes.c_size_t()
                    dir_s = GBASE + s + 336 + BLOQUE * RANURA
                    if not K32.WriteProcessMemory(ctypes.c_void_p(h), ctypes.c_void_p(dir_s), data, len(data),
                                                  ctypes.byref(n)):
                        raise Fallo(f"write en {hex(s)} fallo")
                self.info("write en las dos estructuras")
        finally:
            K32.CloseHandle(ctypes.c_void_p(h))


# ---------- escenarios integrados (orden y tiempos tomados de du_full.ps1 y tt_iso.ps1) ----------

def _menu(j, par):
    j.captura("menu_principal")


def _ir_a_rueda(j):
    for _ in range(12):
        sc = j.pantalla()
        if sc == 7:
            return
        if sc == 5:
            j.tecla("Space", 4000)
        elif sc == 6:
            j.tecla("Return", 3000)
            if j.pantalla() == 6:
                j.tecla("Space", 7000)
        else:
            j.tecla("Return", 3000)
    raise Fallo(f"no llegue a la rueda de personajes (pantalla {j.pantalla()})")


def _du_freezer_nuevo(j, par):
    _ir_a_rueda(j)
    for _ in range(int(par.get("up", 2))):              # desde Goku: Broly, Freezer
        j.tecla("W", 2500)
    j.captura("00_rueda")
    ultimo = None
    for i in range(1, int(par.get("iter", 40)) + 1):
        sc = j.pantalla()
        if sc == 10:                                    # subir de nivel: repartir puntos
            for _ in range(5):
                j.tecla("D", 400)
            j.tecla("Space", 2500)
        else:
            j.tecla("Space", int(par.get("paso_ms", 5000)))
        if i % 3 == 0:
            j.captura(f"{i:03d}")
        if sc != ultimo:
            j.info(f"paso {i}: pantalla {sc}")
            ultimo = sc
    if not ({8, 9} & j.vistas):
        raise Fallo("nunca entro en escenas (8) ni en combate (9)")
    for l in [l for l in j.texto_log().splitlines() if "dbz3 campanas" in l][-3:]:
        j.info(re.sub(r"^(\[[^\]]*\] ){3}", "", l)[:170])


def _batalla(j, par):
    # DBZ3_TEST_BATTLE (src/hub.cpp): 3 s despues del menu principal lanza el combate; RINGOUT lo acaba a los 5 s
    j.log(r"hub: combate nativo", 30)
    j.log(r"tag: empieza un combate", 40)
    j.captura("01_combate")
    j.log(r"tag: combate acabado", 40)
    # estados del combate (tagteam.cpp, con RINGOUT): 5 acabado, 6 pose del ganador, 7 menu final (Battle
    # Again? / Reselect Characters / Return to Main Menu, cursor en el primero; medido 2026-10-15). A
    # ciegas, una A antes de tiempo elegia la revancha con el PC cargado.
    j.log(r"tag: prueba: estado del combate 7", 40)
    time.sleep(1)
    for k in ("S", "S", "Space", "W"):                  # Return to Main Menu -> "Exit?" -> Yes
        j.tecla(k, 1200)
    j.tecla_hasta_log("Space", r"hub: fin del combate nativo", 3, 4)
    j.esperar_pantalla([5], 40)                         # vuelta al menu principal
    j.captura("02_vuelta_menu")


def _du_continuar(j, par):
    # verificado 2026-10-15: DU (titulo, pantalla 6) -> Return -> menu (New Story) -> S = Continue -> rueda (7)
    # -> W x up (1 = Freezer desde Goku) -> Space -> mapa/escenas (8)
    j.tecla_hasta("Space", [6], 5, 4)                   # menu principal -> DU (la primera a veces no entra)
    time.sleep(3)                                       # "Pulsa START" no responde nada mas aparecer
    j.tecla("Return", 3000)                             # "Press START" -> menu de DU
    j.tecla("S", 1500)                                  # Continue
    j.tecla("Space", 3000)
    j.esperar_pantalla([7], int(par.get("seg", 40)))
    time.sleep(2.5)                                     # la rueda aun gira al aparecer: la W se perdia
    for _ in range(int(par.get("up", 1))):
        j.tecla("W", 2000)
    j.captura("00_rueda")
    j.tecla("Space", 4000)
    j.esperar_pantalla([8, 9], int(par.get("seg", 40)))  # mapa, o combate directo (freezer_cap2 hoy: escena 5522)
    j.captura("01_continuar")
    j.log(r"historias de mods recuperadas", 5)          # la casilla de mods vuelve de historias_extra.bin
    j.log(r"personaje del mapa al crear \(casilla 22", 40)
    for i in range(1, int(par.get("iter", 10)) + 1):
        j.tecla("Space", int(par.get("paso_ms", 3000)))
        if i % 3 == 0:
            j.captura(f"{i:03d}")
        if par.get("hasta_combate") and "tag: combate acabado" in j.texto_log():
            j.info(f"combate de la campana acabado en el paso {i}")
            return
    if par.get("hasta_combate"):
        raise Fallo("la campana no llego a acabar un combate")


def _capsulas_custom(j, abiertos, seg=25):
    """Menu de capsulas del jugador 1 (Tag Team): lleva el cursor a Custom (opcion 1) mirando el log y elige.
    A ciegas (S + Space) fallaba con 3 juegos abiertos: el menu se abria en otra opcion y el Space abria la
    bandeja de Edit Skills (2026-10-15)."""
    ya = j.texto_log().count("menu de capsulas del jugador 1: elegido")
    fin = time.time() + seg
    while j.texto_log().count("menu de capsulas del jugador 1 abierto") < abiertos:   # el de este miembro
        if time.time() > fin:
            raise Fallo("no se abrio el menu de capsulas del jugador 1")
        time.sleep(0.25)
    time.sleep(1.0)                                     # su primera "opcion N" llega justo despues
    while time.time() < fin:
        t = j.texto_log()
        if t.count("menu de capsulas del jugador 1: elegido") > ya:
            return
        ops = re.findall(r"menu de capsulas del jugador 1: opcion (\d)", t)
        cur = int(ops[-1]) if ops else 1
        j.tecla("W" if cur > 1 else "S" if cur < 1 else "Space", 2500)
    raise Fallo("el menu de capsulas del jugador 1 no llego a elegir Custom")


def _esperar_pelea(j, seg=60):
    """Espera el estado 4 del combate (pelea: mgr = *0x823D6CF4, estado mgr+35128; misma direccion en US y EU).
    Con 3 juegos abiertos la presentacion tarda mas y 8 s fijos dejaban el SELECT en ella (tag_swap en FAIL)."""
    fin = time.time() + seg
    while time.time() < fin:
        b = leer(j.h, GBASE + 0x823D6CF4, 4)
        mgr = int.from_bytes(b, "big") if b and len(b) == 4 else 0
        e = leer(j.h, GBASE + mgr + 35128, 4) if mgr else None
        if e and len(e) == 4 and int.from_bytes(e, "big") == 4:
            return
        time.sleep(0.25)
    raise Fallo("el combate no llego a la pelea (estado 4)")


def _tag_swap(j, par):
    swap = par.get("swap", "1") == "1"
    j.tecla("L", 3000)
    j.captura("02_tagteam")
    j.tecla("Space", 12000)
    j.captura("03_rueda")
    j.tecla("Space", 4000)
    j.captura("04_capsulas")
    _capsulas_custom(j, 1)
    j.captura("05_m1")
    j.tecla("S", 2000)
    j.tecla("Space", 4000)
    _capsulas_custom(j, 2)
    j.captura("06_m2")
    for _ in range(int(par.get("rival_skip", 0))):
        j.tecla("S", 1500)
    j.tecla("S", 2000)
    j.tecla("Space", 4000)
    j.tecla("Space", 6000)
    j.captura("07_r1")
    j.tecla("S", 2000)
    j.tecla("Space", 4000)
    j.tecla("Space", 6000)
    j.captura("08_r2")
    for _ in range(5):
        j.tecla("Space", 3000)
    j.log("tag: empieza un combate", 40)                # antes: 12 s fijos y luego el log
    _esperar_pelea(j)                                   # el SELECT durante la presentacion no releva
    time.sleep(1)
    j.captura("09_combate")
    if swap:
        j.tecla_hasta_log("Z", r"relevo .* -> ")        # relevo pedido con SELECT, repetido si no llega
    else:
        for _ in range(60):                             # golpear hasta el KO del rival
            j.tecla("D 400", 100)
            for _ in range(3):
                j.tecla("L", 150)
            if re.search(r"tag: KO de|relevo .* -> ", j.texto_log()):
                break
    j.log(r"tag: KO de|relevo .* -> ", 15)
    for i in range(int(par.get("rafaga", 12)) + 1):
        j.captura(f"10_{i:02d}")
        time.sleep(0.12)
    time.sleep(3)
    j.captura("11_despues")


def _a_duelo(j):
    """Menu principal (cursor en Dragon Universe) -> Duelo (pantalla 14). La primera tecla nada mas llegar al menu
    se pierde (medido 2026-10-15: la S se perdio y el Space de despues entro en Dragon Universe)."""
    time.sleep(2.5)
    j.tecla("S", 1200)
    j.tecla("Space", 0)
    j.esperar_pantalla([14, 6], 8)
    if j.pantalla() != 14:
        raise Fallo(f"la S no movio el cursor del menu principal: entro en la pantalla {j.pantalla()}")
    time.sleep(1.5)


def _online_crear(j, par):
    # Bug 2026-10-15: tras elegir un equipo en el Tag Team de 1P, CREAR PARTIDA fallaba hasta reiniciar
    # ("F4: Tag Team necesita prueba..."). DBZ3_NET_TEAM hace de ese equipo; DBZ3_NET_LOCAL: sin UPnP ni STUN
    # (la prueba cierra el juego a la fuerza y no debe dejar puertos abiertos en el router).
    _a_duelo(j)
    for _ in range(4):                                  # 1P VS COM ... COM VS COM -> fila ONLINE
        j.tecla("S", 700)
    j.captura("01_fila_online")
    j.tecla("Space", 1500)                              # abre la pantalla ONLINE
    j.captura("02_online")
    j.tecla("Space", 2000)                              # CREAR PARTIDA (primera fila)
    j.log(r"dbz3 online: (partida creada|no se pudo crear)", 10)
    j.captura("03_crear")
    texto = j.texto_log()
    if "no se pudo crear" in texto:
        raise Fallo("CREAR PARTIDA fallo: " + re.findall(r"no se pudo crear la partida: (.*)", texto)[-1][:120])
    if par.get("equipo", "1") == "1" and "no viaja al online" not in texto:
        raise Fallo("el equipo de prueba (DBZ3_NET_TEAM) no llego a Host()")


def _rafaga(j, prefijo, seg):
    """Capturas seguidas durante seg (el nombre lleva los ms desde el principio de la rafaga)."""
    t0 = time.time()
    while time.time() - t0 < seg:
        j.captura(f"{prefijo}_{int((time.time() - t0) * 1000):05d}")


def _duelo_fundido(j, par):
    # Fallo visto por el frente HUB (2026-10-15): el recuadro ONLINE del Duelo salia sobre el negro del fundido al
    # entrar y al salir. Rafagas de capturas: entrada desde el menu principal, salida con Y (P) y con B
    # (Backspace) desde la fila ONLINE elegida.
    time.sleep(2.5)                                     # la primera tecla en el menu principal se pierde
    j.tecla("S", 1200)
    j.tecla("Space", 0)
    j.esperar_pantalla([14], 8)
    _rafaga(j, "a_entrada", float(par.get("seg", 3.5)))
    time.sleep(1)
    j.tecla("P", 0)                                     # Y: atras
    _rafaga(j, "b_salida_y", 2.5)
    j.esperar_pantalla([5], 10)
    time.sleep(2.5)
    j.tecla("Space", 0)                                 # el menu principal recuerda la fila del Duelo
    j.esperar_pantalla([14], 8)
    time.sleep(3.5)
    for _ in range(4):                                  # 1P VS COM ... COM VS COM -> fila ONLINE
        j.tecla("S", 700)
    j.captura("c_fila_online")
    j.tecla("Backspace", 1500)                          # B desde la fila ONLINE: solo la suelta (el Duelo sigue)
    j.captura("d_fila_suelta")                          # la fila ONLINE sigue a la vista
    j.tecla("P", 0)                                     # Y: fuera del Duelo
    _rafaga(j, "e_salida_y", 2.5)
    j.esperar_pantalla([5], 10)


def _hub_online_fundido(j, par):
    # HUB > Online: el Duelo llega en negro y la pantalla ONLINE se abria encima del negro; ahora espera a que el
    # Duelo se vea y entra fundida. Luego B (cierra la pantalla) e Y (sale del Duelo) con rafagas.
    time.sleep(2.5)
    j.tecla("W", 1200)                                  # desde la fila 0, arriba = fila HUB (la ultima)
    j.tecla("Space", 3500)                              # pantalla HUB (cursor en Tag Team)
    j.captura("a_hub")
    j.tecla("S", 1200)                                  # Online
    j.tecla("Space", 0)
    j.esperar_pantalla([14], 10)
    _rafaga(j, "b_llegada", float(par.get("seg", 4.5)))
    j.log(r"se abre la pantalla ONLINE", 5)
    time.sleep(1)
    j.tecla("Backspace", 1500)                          # cierra la pantalla ONLINE
    j.captura("c_duelo")
    j.tecla("P", 0)                                     # Y: fuera del Duelo
    _rafaga(j, "d_salida", 2.5)


def _historia_y_duelo(j, par):
    # Bug 2026-10-15: la forma inicial y el traje del rival de un combate de historia de mod ([forma_jugador],
    # [traje_rival]) se quedaban para el siguiente combate fuera de Dragon Universe. Salir de DU a mano no es
    # practicable (la historia de Freezer encadena combates), asi que DBZ3_TEST_STORY_WORD hace como si se volviera
    # de DU tras el combate 5522.6 (Freezer forma 3 contra Vegeta con traje 3) al llegar al menu principal, y
    # DBZ3_TEST_BATTLE lanza alli un combate normal (Goku contra Vegeta). Con --par antes=1 (DBZ3_TEST_KEEP_STORY) se
    # ve el fallo de antes. Que los combates normales no tocan esa palabra lo dicen los logs: 0 "se lanza la escena".
    j.log(r"hub: combate nativo", 30)
    j.log(r"tag: empieza un combate", 40)
    time.sleep(2)
    j.captura("01_combate_duelo")
    texto = j.texto_log()
    despues = texto[texto.rfind("hub: combate nativo"):]
    heredado = re.findall(r"tu personaje empieza en la forma \d+|el rival lleva el traje \d+", despues)
    if par.get("antes") == "1":
        j.info(f"comportamiento de antes: {heredado or 'nada heredado'}")
        if not heredado:
            raise Fallo("con DBZ3_TEST_KEEP_STORY el combate deberia heredar la forma/traje (la prueba no reproduce)")
    elif heredado:
        raise Fallo(f"el combate normal hereda del combate de historia: {heredado}")
    elif "se olvida el ultimo combate de historia" not in texto:
        raise Fallo("el menu principal no olvido el combate de historia")


ESCENARIOS = {
    "menu": ("llegar al menu principal y captura", lambda p: {}, _menu),
    "du_freezer_nuevo": ("DU > nueva historia > rueda > Freezer > bucle de combate",
                         lambda p: {}, _du_freezer_nuevo),
    "du_continuar": ("DU > Continuar (Freezer: con --instantanea freezer_cap2)", lambda p: {}, _du_continuar),
    "batalla": ("combate nativo desde el menu (DBZ3_TEST_BATTLE) y vuelta al menu",
                lambda p: {"DBZ3_TEST_BATTLE": p.get("ids", "0,7,2"), "DBZ3_TEST_RINGOUT": "1"}, _batalla),
    # con perfil_nuevo la rueda es otra (W x2 caia en Bardock y se quedaba en la pantalla de capsulas):
    # se reutiliza el camino verificado de du_continuar y se avanza hasta acabar el siguiente combate
    "campana": ("DU > Continuar > Freezer > hasta acabar el siguiente combate (ring out a los 5 s)",
                lambda p: {"DBZ3_TEST_RINGOUT": "1"},
                lambda j, p: _du_continuar(j, dict({"hasta_combate": "1", "iter": "40"}, **p))),
    "online_crear": ("Duelo > ONLINE > CREAR PARTIDA con un equipo de Tag Team elegido antes",
                     lambda p: {"DBZ3_NET_LOCAL": "1", **({"DBZ3_NET_TEAM": "0,11"} if p.get("equipo", "1") == "1" else {})},
                     _online_crear),
    "historia_y_duelo": ("combate normal tras volver de un combate de historia de mod: sin su forma ni su traje",
                         lambda p: {"DBZ3_TEST_STORY_WORD": p.get("palabra", "0x15920006"),
                                    "DBZ3_TEST_BATTLE": p.get("ids", "0,7,2"), "DBZ3_TEST_RINGOUT": "1",
                                    **({"DBZ3_TEST_KEEP_STORY": "1"} if p.get("antes") == "1" else {})},
                         _historia_y_duelo),
    "duelo_fundido": ("entrar y salir del Duelo con rafagas de capturas (recuadro ONLINE y fundido del juego)",
                      lambda p: {}, _duelo_fundido),
    "hub_online_fundido": ("HUB > Online: llegada al Duelo y pantalla ONLINE con rafagas de capturas",
                           lambda p: {}, _hub_online_fundido),
    "tag_swap": ("Tag Team 1P vs COM, relevo con SELECT",
                 lambda p: dict({"DBZ3_TAG_SLOW": "3"},
                                **({} if p.get("swap", "1") == "1" else {"DBZ3_TEST_KO": "1"})),
                 _tag_swap),
}


def ejecutar_dsl(j, pasos):
    for p in pasos:
        v, *a = shlex.split(p)
        if v == "tecla":
            j.tecla(a[0].replace(":", " "), int(a[1]) if len(a) > 1 else 600)
        elif v == "esperar":
            time.sleep(int(a[0]) / 1000)
        elif v == "esperar_pantalla":
            j.esperar_pantalla([int(x) for x in a[0].split(",")], int(a[1]) if len(a) > 1 else 60)
        elif v == "captura":
            j.captura(a[0])
        elif v in ("dump", "write"):
            j.bloque(v)
        elif v == "log":
            j.log(a[0], int(a[1]) if len(a) > 1 else 20)


def cargar_escenario(esc, par):
    """Devuelve (descripcion, funcion, env). Valida antes de tocar el juego."""
    if esc.lower().endswith(".toml"):
        ruta = Path(esc)
        if not ruta.exists():
            raise SystemExit(f"no existe el escenario {ruta}")
        d = tomllib.loads(ruta.read_text(encoding="utf-8"))
        pasos = d.get("pasos", [])
        if not pasos:
            raise SystemExit(f"{ruta.name} no tiene 'pasos'")
        for p in pasos:
            if shlex.split(p)[0] not in VERBOS:
                raise SystemExit(f"paso desconocido en {ruta.name}: {p!r}")
        env = dict(_kv(x) for x in d.get("env", []))
        return d.get("descripcion", ruta.name), (lambda j, _p: ejecutar_dsl(j, pasos)), env
    if esc not in ESCENARIOS:
        raise SystemExit(f"escenario desconocido '{esc}'. Opciones: {', '.join(ESCENARIOS)} o un .toml")
    desc, env_fn, fn = ESCENARIOS[esc]
    return desc, fn, env_fn(par)


def correr(copia, escenario, env_extra=(), instantanea=None, par=None):
    with copia_en_uso(BASE / copia):
        return _correr(copia, escenario, env_extra, instantanea, par)


def _correr(copia, escenario, env_extra=(), instantanea=None, par=None):
    par = par or {}
    desc, fn, env = cargar_escenario(escenario, par)
    env.update(_kv(x) for x in env_extra)
    nombre = f"{copia}/{Path(escenario).stem}"
    print(f"== {nombre}: {desc}", flush=True)
    t0 = time.time()
    preparar(BASE / copia)
    if instantanea:
        cargar_instantanea(copia, instantanea)
    else:
        respaldar(BASE / copia)
    j = Juego(copia, nombre)
    ok, msg, t_menu, t_cupo = True, "", None, 0.0
    try:
        j.lanzar(env)
        t_cupo = j.t_cupo
        t_menu = j.arrancar(rapido=par.get("titulo_rapido", "1") == "1")
        j.info(f"menu principal en {t_menu:.1f} s")
        fn(j, par)
        if not j.vivo():
            raise Fallo("el juego se cerro durante la prueba")
    except Exception as e:
        ok, msg = False, str(e) or type(e).__name__
        j.captura_fallo()
    finally:
        muerto = j.proc is not None and j.proc.poll() is not None    # murio solo (no lo cerramos nosotros)
        t_fin = time.time()
        j.cerrar()
        t_cierre = time.time() - t_fin
    texto = j.texto_log()
    malos = [l for l in texto.splitlines() if re.search(ERRORES, l)]
    if muerto:
        rc = j.proc.returncode & 0xFFFFFFFF
        ok, msg = False, f"el juego murio (codigo 0x{rc:08X}{', access violation' if rc == 0xC0000005 else ''}). {msg}"
    if malos and ok:
        ok, msg = False, f"log con errores ({len(malos)}): {malos[0][:150]}"
    if texto:
        (j.run / "dbz3.log").write_text(texto, encoding="utf-8")
    (j.run / "pantallas.txt").write_text("".join(f"{t:6.1f} s  {sc}\n" for t, sc in j.traza), encoding="utf-8")
    if not ok:
        extracto = informe_fallo(texto, malos)
        (j.run / "fallo.txt").write_text(f"{msg}\n\n{extracto}\n", encoding="utf-8")
        print("\n".join("      | " + l for l in extracto.splitlines()[-12:]), flush=True)
    dur = time.time() - t0 - t_cupo
    print(f"{'PASS' if ok else 'FAIL'}  {nombre}  {dur:.1f} s (menu {t_menu or 0:.1f} s, cierre {t_cierre:.1f} s"
          f"{f', cola {t_cupo:.0f} s' if t_cupo > 1 else ''})  {msg}".rstrip(), flush=True)
    print(f"      capturas y resultados: {j.run}", flush=True)
    return {"ok": ok, "nombre": nombre, "dur": dur, "menu": t_menu, "cierre": t_cierre, "msg": msg, "run": j.run}


def informe_fallo(texto, malos):
    """Extracto del log para el fallo: lineas de error y las ultimas 25 (sin las de rendimiento)."""
    lineas = [l for l in texto.splitlines() if "perf fps=" not in l and "audio pico=" not in l]
    partes = []
    if malos:
        partes += ["-- errores --"] + [l[:220] for l in malos[:8]]
    partes += ["-- final del log --"] + [l[:220] for l in lineas[-25:]]
    return "\n".join(partes) if texto else "(sin log del juego)"


def paralelo(pares, env_extra=()):
    trabajos = []
    for p in pares:
        esc, _, cop = p.partition("@")
        if not esc or not cop:
            raise SystemExit(f"formato esc@copia, recibido: {p}")
        trabajos.append((cop, esc))
    if len(trabajos) > MAX_PARALELO:
        raise SystemExit(f"como maximo {MAX_PARALELO} a la vez")
    if len({c for c, _ in trabajos}) < len(trabajos):
        raise SystemExit("cada copia solo puede usarse una vez")
    with ThreadPoolExecutor(len(trabajos)) as ex:
        res = [f.result() for f in [ex.submit(correr, c, e, env_extra) for c, e in trabajos]]
    print(tabla(res), flush=True)
    return all(r["ok"] for r in res)


# Bateria rapida: (escenario, instantanea, parametros). Las largas primero: asi el total es menor.
SMOKE = [
    ("tag_swap", "freezer_cap2", {"rafaga": "4"}),
    ("campana", "freezer_cap2", {}),
    ("du_continuar", "freezer_cap2", {"iter": "3"}),
    ("batalla", "freezer_cap2", {}),
    ("menu", "freezer_cap2", {}),
]
COPIAS_SMOKE = ["smoke_a", "smoke_b", "smoke_c"]


def tabla(res, total=None):
    out = ["", "== RESUMEN ==", f"{'':4}  {'prueba':<24} {'total':>7} {'menu':>6} {'cierre':>6}  detalle"]
    for r in res:
        menu = f"{r['menu']:5.1f}s" if r.get("menu") else "     -"
        cierre = f"{r['cierre']:5.1f}s" if r.get("cierre") is not None else "     -"
        out.append(f"{'PASS' if r['ok'] else 'FAIL'}  {r['nombre']:<24} {r['dur']:6.1f}s {menu} {cierre}  "
                   f"{r['msg'][:110]}".rstrip())
    if total is not None:
        out.append(f"{sum(r['ok'] for r in res)}/{len(res)} PASS en {total:.0f} s de reloj "
                   f"(suma de pruebas {sum(r['dur'] for r in res):.0f} s)")
    return "\n".join(out)


def todo(solo=None, maxp=MAX_PARALELO, env_extra=()):
    lista = [s for s in SMOKE if not solo or s[0] in solo]
    if not lista:
        raise SystemExit(f"nada que correr; escenarios de la bateria: {', '.join(s[0] for s in SMOKE)}")
    faltan = sorted({i for _, i, _ in lista if i and not (INST / i / "user_data").exists()})
    if faltan:
        raise SystemExit(f"faltan instantaneas {faltan}: python tools\\pruebas.py biblioteca")
    libres = queue.Queue()
    for c in COPIAS_SMOKE[:max(1, min(maxp, MAX_PARALELO))]:
        libres.put(c)

    def uno(s):
        c = libres.get()
        try:
            return correr(c, s[0], env_extra, s[1], dict(s[2]))
        except (Exception, SystemExit) as e:          # copia ocupada, exe sin compilar...
            return {"ok": False, "nombre": f"{c}/{s[0]}", "dur": 0.0, "msg": str(e), "run": None}
        finally:
            libres.put(c)

    t0 = time.time()
    with ThreadPoolExecutor(libres.qsize()) as ex:
        res = list(ex.map(uno, lista))
    txt = tabla(res, time.time() - t0)
    print(txt, flush=True)
    PRUEBAS.mkdir(parents=True, exist_ok=True)
    (PRUEBAS / f"{ts()}_todo.txt").write_text(txt + "\n", encoding="utf-8")
    return all(r["ok"] for r in res)


def biblioteca(copia="smoke_a", seg=120):
    """Instantaneas de partida para las pruebas: perfil_nuevo (la que crea el juego en el primer arranque).
    freezer_cap2 se hizo a mano (ver docs/14_frentes/pruebas.md); todo_desbloqueado no se puede crear aun."""
    dest = BASE / copia
    preparar(dest)
    ud = dest / "user_data"
    if ud.exists():
        mover(ud, RESPALDO / f"{ts()}_{copia}_antes_de_perfil_nuevo")
    j = Juego(copia, "biblioteca_perfil_nuevo")
    try:
        j.lanzar({})
        j.info(f"menu principal en {j.arrancar(seg, titulo=('Return', 'W', 'Space')):.1f} s (perfil nuevo)")
        j.captura("menu_perfil_nuevo")
        time.sleep(3)                                   # que termine de escribir la partida nueva
    except Exception:
        j.captura_fallo()
        raise
    finally:
        j.cerrar()
        (j.run / "pantallas.txt").write_text("".join(f"{t:6.1f} s  {sc}\n" for t, sc in j.traza), encoding="utf-8")
    if not ud.exists():
        ud.mkdir()                                      # el juego no guardo nada: perfil vacio igualmente valido
    guardar_instantanea(copia, "perfil_nuevo")
    for n in ("perfil_nuevo", "freezer_cap2", "todo_desbloqueado"):
        print(f"  {n:<18} {'OK' if (INST / n / 'user_data').exists() else 'falta'}", flush=True)


def main(argv=None):
    p = argparse.ArgumentParser(description="Pruebas del juego en copias aisladas (ver cabecera del archivo).")
    sub = p.add_subparsers(dest="cmd", required=True)
    a = sub.add_parser("prep", help="crear o refrescar una copia")
    a.add_argument("copia")
    a.add_argument("--desde", help="copia origen si la copia no existe (por defecto du_test)")
    a.add_argument("--exe", default=str(EXE))
    a = sub.add_parser("instantanea", help="guardar o cargar los datos de partida de una copia")
    a.add_argument("accion", choices=["guardar", "cargar"])
    a.add_argument("copia")
    a.add_argument("nombre")
    a = sub.add_parser("correr", help="lanzar el juego y ejecutar un escenario")
    a.add_argument("copia")
    a.add_argument("escenario")
    a.add_argument("--env", action="append", default=[], metavar="K=V")
    a.add_argument("--par", action="append", default=[], metavar="K=V", help="parametros del escenario")
    a.add_argument("--instantanea", help="cargar esta instantanea antes de correr")
    a = sub.add_parser("paralelo", help="hasta 3 pruebas a la vez, cada una en su copia")
    a.add_argument("pares", nargs="+", help="escenario@copia")
    a.add_argument("--env", action="append", default=[], metavar="K=V")
    a = sub.add_parser("todo", help="bateria rapida (smoke) en smoke_a/b/c, tabla PASS/FAIL")
    a.add_argument("--solo", help="lista separada por comas (" + ",".join(s[0] for s in SMOKE) + ")")
    a.add_argument("--max", type=int, default=MAX_PARALELO, help="juegos a la vez (1-3)")
    a.add_argument("--env", action="append", default=[], metavar="K=V")
    sub.add_parser("biblioteca", help="crea las instantaneas de prueba (perfil_nuevo)")
    a = p.parse_args(argv)

    if a.cmd == "todo":
        return 0 if todo(a.solo.split(",") if a.solo else None, a.max, a.env) else 1
    if a.cmd == "biblioteca":
        biblioteca()
        return 0

    if a.cmd == "prep":
        preparar(BASE / a.copia, a.desde, Path(a.exe))
        print(f"copia lista: {BASE / a.copia}", flush=True)
        return 0
    if a.cmd == "instantanea":
        (guardar_instantanea if a.accion == "guardar" else cargar_instantanea)(a.copia, a.nombre)
        return 0
    if a.cmd == "correr":
        r = correr(a.copia, a.escenario, a.env, a.instantanea, dict(_kv(x) for x in a.par))
        return 0 if r["ok"] else 1
    return 0 if paralelo(a.pares, a.env) else 1


if __name__ == "__main__":
    sys.exit(main())
