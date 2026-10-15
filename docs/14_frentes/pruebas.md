# Frente: pruebas en el juego (rapidas y fiables)

Archivos de este frente: `tools/pruebas.py`, `tools/test_pruebas.py` (comprobacion sin juego), `tools/compilar.py`
(sin cambios: el candado de compilado ya funcionaba). Nada de C++ tocado: los hooks nuevos van como propuestas (seccion 6).

## 1. Uso

```
python tools\test_pruebas.py                       # sin juego, 1 s: escenarios, deteccion de fallos, tabla
python tools\pruebas.py biblioteca                 # crea la instantanea perfil_nuevo (1 arranque, ~45 s)
python tools\pruebas.py todo                       # bateria rapida, 3 a la vez, tabla PASS/FAIL con tiempos
python tools\pruebas.py todo --solo menu,batalla --max 2
python tools\pruebas.py correr smoke_a batalla --par ids=0,7,2
python tools\pruebas.py correr smoke_a campana     # (con --instantanea perfil_nuevo para empezar de cero)
```

La bateria `todo` corre en sus propias copias `D:\DBZ3HD\smoke_a|b|c` (no pisa du_test/tag_test de otros agentes).
Cada prueba carga su instantanea antes de arrancar, asi el resultado no depende de lo que dejo la anterior.

| Escenario | Instantanea | Que comprueba (PASS) |
|---|---|---|
| `tag_swap` | freezer_cap2 | Tag Team 1P vs COM: empieza el combate y el relevo con SELECT sale en el log |
| `campana` | freezer_cap2 | DU > Continuar > Freezer y Space hasta `tag: combate acabado` (ring out a los 5 s) |
| `du_continuar` | freezer_cap2 | DU > Continuar > Freezer: mapa (8) o combate (9), historias de mods recuperadas, personaje del mapa (casilla 22) |
| `batalla` | freezer_cap2 | `DBZ3_TEST_BATTLE=0,7,2` + ring out: combate nativo, fin, vuelta al menu principal (5) |
| `menu` | freezer_cap2 | llega al menu principal (5) |

Resultado: tabla en pantalla y en `D:\DBZ3HD\_pruebas\<fecha>_todo.txt`. Por prueba, en su carpeta de `_pruebas`:
capturas, `dbz3.log` (copia del log de esa partida), `pantallas.txt` (cada cambio de pantalla y cada tecla con su
segundo) y, si falla, `fallo.txt` (motivo + lineas de error + final del log) y `zz_fallo.png` (como quedo la pantalla).

## 2. Robustez (lo que cambia en pruebas.py)

- **Cupo de 3 juegos atomico**: contar `dbz3.exe` y lanzar va dentro del candado `D:\DBZ3HD\_lanzando` (mkdir).
  Antes dos agentes podian ver 2 juegos a la vez y abrir 4. Candado de mas de 60 s = lanzador muerto: se mueve a
  `_respaldo`. El tiempo en cola no cuenta en la duracion de la prueba (se muestra aparte: `cola N s`).
- **Titulo**: la pantalla 4 ya incluye el video de apertura y no acepta teclas hasta ~15 s despues de abrirlo.
  Se pulsa por turnos cada 1,5 s (antes Space cada 6 s). Perfil sin partida: el titulo ofrece New Game / Load Game
  con el cursor en Load; `biblioteca` usa `titulo=("Return","W","Space")`.
- **Primera tecla que se pierde**: `Juego.tecla_hasta(tecla, pantallas, intentos, seg)` repite hasta que la pantalla
  cambia (usado al entrar en DU); `Juego.tecla_hasta_log(tecla, regex)` igual pero mirando el log (menus que no
  cambian el byte). `tag_swap` espera el log `tag: empieza un combate` + 8 s y repite SELECT hasta ver el relevo
  (un SELECT a los 4 s de empezar se perdia en la presentacion). "Pulsa START" de DU necesita 3 s tras aparecer.
- **Cuelgues y muertes**: si el proceso muere solo se informa el codigo de salida (`0xC0000005` = access violation).
  Errores del log con los tres mensajes reales del runtime: `UNHANDLED EXCEPTION` (main.cpp:976),
  `Unhandled guest access violation` (xmemory.cpp:547), `physical write fault` (xmemory.cpp:628).
- **Cierre**: 2 s de cortesia y cierre forzado. Antes el cierre amable esperaba 10 s y luego mataba (el juego no
  siempre sale tras `Window close requested`, ver 6.2); eso eran ~20 s por prueba. Las copias son desechables y el juego guarda al
  pedirlo, no al salir. Despues se espera a que el proceso desaparezca (hasta 30 s): tras `/F` a veces tardaba
  >5 s y la siguiente prueba en esa copia se negaba a arrancar ("la copia tiene un juego abierto").
- **Copia nueva colgada**: `robocopy` sin `/R` reintentaba 1 millon de veces cada 30 s un archivo bloqueado si la
  copia origen (du_test) tenia un juego abierto. Visto hoy: smoke_b, smoke_c y el `hub_test` de otro frente parados
  4+ min. Ahora `/R:1 /W:1` y aviso con los archivos no copiados (exe, DLL y toml se ponen igual despues).
- `dbz3_texture_packs_on = false` en las copias de prueba (el toml heredado apuntaba a un pack de otra copia).

## 3. Biblioteca de instantaneas (`D:\DBZ3HD\_instantaneas`)

| Nombre | Como se obtiene | Estado |
|---|---|---|
| `perfil_nuevo` | `pruebas.py biblioteca`: arranque con user_data vacio, New Game, menu principal. El juego crea la partida (data.bin 56 KB) | creada 2026-10-15, menu en 27 s. Ojo: su rueda de DU es otra (W x2 desde Goku cae en Bardock), por eso `campana` usa freezer_cap2 |
| `freezer_cap2` | a mano (2026-10-15 00:09): historia de Freezer en el capitulo 2. Para rehacerla: `correr <copia> du_freezer_nuevo --instantanea perfil_nuevo` hasta el cap. 2 y `instantanea guardar <copia> freezer_cap2` | existe |
| `todo_desbloqueado` | no se puede crear aun: no hay herramienta ni formato documentado de los desbloqueos de la partida. Propuesta 6.3 | falta |

Lo reemplazado nunca se borra: va a `_instantaneas\_respaldo`.

## 4. Tiempos (antes / despues, mismo PC, misma partida)

| Medida | Antes | Despues |
|---|---|---|
| `correr smoke_a menu`: llegar al menu principal | 28,4 s | 20,3 s |
| `correr smoke_a menu`: cierre del juego | ~23 s (10 s de espera + forzado) | 0,7-4 s |
| `correr smoke_a menu`: total | 54,3 s | 26,1 s (en `todo`: 21-28 s) |
| copia nueva si du_test tiene un juego abierto | colgada sin limite (visto 8+ min) | 1 reintento de 1 s y sigue |
| `todo` (5 pruebas, 3 a la vez) | no existia (a mano: 5 comandos) | ~4 min de reloj sin cola (suma: 139+100+100+66+28 s) |

Lo que queda caro: el titulo no acepta teclas hasta ~15 s tras abrir el video (propuesta 6.1, ahorraria ~12 s por
prueba) y la cola del cupo de 3 juegos compartido (hoy 3-130 s por prueba, segun lo que corran otros frentes).

## 5. Ejecuciones de `todo` (2026-10-15)

| Prueba | Resultado | Total | Menu | Notas |
|---|---|---|---|---|
| menu | PASS | 28,3 s | 19,7 s | |
| du_continuar | PASS | 65,9 s | 22,3 s | freezer_cap2 hoy entra directo en combate (escena 5522), antes ponia mapa (8) |
| campana | PASS | 99,6 s | 24,0 s | combate acabado en el paso 13 |
| tag_swap | PASS | 138,9 s | 25,6 s | relevo en el log 9 s despues de empezar |
| batalla | **FAIL (fallo real)** | 100,5 s | 26,6 s | ver abajo |

Resultados en `D:\DBZ3HD\_pruebas\20261015-0133*` y `20261015-0143*` (la tabla de cada pasada en `*_todo.txt`).
Hicieron falta 3 pasadas: la 1a destapo el cuelgue de robocopy y 3 errores mios de tiempos (corregidos); la 2a y
la 3a dieron lo de la tabla. No se ha repetido la bateria entera de una vez para no gastar cupo de juegos.

**Fallo real de `batalla` (para el frente de hub.cpp)**: con `DBZ3_TEST_BATTLE=0,7,2` el combate empieza y acaba
(`tag: combate acabado ... motivo 4`), pero despues sale el menu del Duelo (Battle Again? / Reselect Characters /
Return to Main Menu) sin cursor y no responde a Space (6 intentos en 25 s): nunca llega
`hub: fin del combate nativo` ni el menu principal. El comentario de `src/hub.cpp:98` dice que con un combate
propio no deberia salir ese menu. Captura: `D:\DBZ3HD\_pruebas\20261015-013726_smoke_a_batalla\zz_fallo.png`.
El log de otro agente (`du_test\logs\dbz3_053.log`, 01:02) muestra lo mismo: tampoco hay "fin del combate nativo".

## 6. Propuestas de hooks del runtime (los C++ son de otros frentes)

### 6.1 `DBZ3_TEST_FAST=1`: sin video de apertura  [?] sin probar

El titulo no acepta teclas hasta ~15 s despues de abrir `opening.sfd`; es el mayor coste fijo de cada prueba.
En `rexglue-sdk-0.10/src/filesystem/devices/host_path_entry.cpp`, `HostPathEntry::Open`, justo antes de
`auto file_handle = rex::filesystem::FileHandle::OpenExisting(`:

```cpp
  // dbz3 PRUEBA (DBZ3_TEST_FAST=1): sin video de apertura (las pruebas esperaban ~15 s en el titulo)
  if (static const bool fast = std::getenv("DBZ3_TEST_FAST") != nullptr; fast) {
    std::string name = rex::path_to_utf8(open_path.filename());
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    if (name == "opening.sfd") {
      REXFS_INFO("dbz3: PRUEBA: video de apertura omitido (DBZ3_TEST_FAST)");
      return X_STATUS_NO_SUCH_FILE;
    }
  }
```

Verificar: que el titulo (pantalla 4) siga apareciendo sin el video. Si el juego se queda esperando, plan B:
servir `Ending01.sfd` u otro video corto en su lugar. `pruebas.py` ya pasa variables con `--env DBZ3_TEST_FAST=1`;
cuando exista, anadirla a `OVERRIDES`/env por defecto de `todo`.

### 6.2 Cierre que a veces no termina (observacion, sin codigo)

`OnWindowCloseRequested` (`src/main.cpp:914-921`) ya hace `_Exit(0)`, pero en 1 de 3 cierres medidos el proceso
seguia vivo a los 2 s del WM_CLOSE (y antes, con 10 s de espera, la prueba perdia ~20 s). Sospechosos entre el log
y el `_Exit`: `dbz3::SessionEnd(true)` y `dbz3::net::UpnpCloseMapped()` (red). `pruebas.py` ya lo resuelve
matando a los 2 s; para el frente online/runtime: medir con un `REXLOG_INFO` entre esas dos llamadas.

### 6.3 `DBZ3_TEST_UNLOCK=1`: todo desbloqueado  (idea, falta la direccion)

Para la instantanea `todo_desbloqueado` hace falta saber donde guarda la partida los desbloqueos (personajes,
escenarios, capsulas). La estructura de perfil es `0x824BA110` (`roster_trace.cpp:40`); el inventario de capsulas es
u8 por ID en esa estructura (HANDOFF 2026-10-04). Con la direccion, el hook pondria los bits al llegar al menu
principal y `pruebas.py biblioteca` guardaria la instantanea tras un guardado.
