# Frente: Escenarios (escenarios de otros juegos como casillas nuevas)

Fecha: 2026-10-15. Solo US (como hub.cpp). Estado vivo: este documento.

## Qué ve el jugador
- **Antes**: el escenario convertido de Budokai 2 cerraba el juego al empezar el combate, y solo podía entrar
  sustituyendo uno de serie.
- **Ahora**: cada mod de escenario añade una **casilla nueva** en "Selección de fase" (Duelo), con su bola, su
  vista redonda, su vista grande y su rótulo con la letra del juego, entre los 12 de serie y el "?". Se pelea
  en él con presentación, cámaras, suelo y ring out. Los 17 de serie siguen igual y en su sitio; sin mods el
  selector es el de siempre. También se lanza desde el HUB (`dbz3::hub::RequestBattle(id1, id2, 17+, ...)`).
- Probados en el juego: **Templo de Kami** y **Montañas nevadas** (Budokai 2) y **Páramo al atardecer**
  (Budokai 1). Capturas: `E:\DBZ3HD_trabajo\escenarios\capturas\informe_2026-10-15\`.

## Ficheros del frente
- `tools/escenarios_convertir.py` (nuevo): conversor B2/B1 PS2 → bin de escenario HD + mod opcional;
  `comprobar` reproduce los 13 escenarios nativos B3 PS2 → HD (grafo campo a campo, cámaras y script idénticos).
- `tools/gen_escenarios_sel.py` (nuevo) → `src/escenarios_sel_gen.inc` (generado): copias de 12 funciones del
  selector (mn_StageSel) con datos ampliados (656 → 1024 B, carrusel de 24) y tablas por casilla propias.
- `src/escenarios.cpp/.h` (nuevos): altas (registros 17+ desde `stage.toml` `[[escenario]]`), selector con
  casillas nuevas, `Nombre()`/`Total()` y sonda de colisión (`DBZ3_SONDA_ESCENARIO=1`).
- `src/stage_ext.cpp`: quitados los ganchos TEMPORALES de diagnóstico; llama a `escenarios::CargarDeMods`.
  Copia previa en `_respaldo/stage_ext.cpp.antes_escenarios_2026-10-15`.
- `CMakeLists.txt`: `src/escenarios.cpp` tras `src/hub.cpp` y en el REMOVE_ITEM solo-EU.
- `docs/14_frentes/escenarios_pruebas/*.toml`: pruebas de `tools/pruebas.py` (ver §6).

## 1. Causa raíz del cierre (B2 "Archipiélago", AFS 5000, lr 0x820CB134)
`sub_820CB0A0` es la consulta de altura del suelo: baja por el árbol st50 (BSP) del objeto y lee la lista de
registros de colisión rec30 de la hoja. Cada rec30/rec20 apunta **dentro del bloque de colisión** que cuelga
de la lista de un hueso (4.ª palabra) del modelo del escenario (cabeceras f4/f3, nodos, vértices y triángulos).
`amo2awo` (conversor de personajes) no conoce ese bloque y lo tiraba; el conversor viejo copiaba los rec como
si fueran del grafo (o los ponía a 0) → puntero NULL al caminar.
- **Medido con sonda** (gancho a `sub_820CBC90`, el reubicador): versión vieja "0 registros dentro de su
  modelo, 2 fuera" y cierre; versión nueva "183 dentro, 0 fuera" (Templo de Kami), 332 (Montañas), 409 (Páramo).
- **Arreglo**: `conv_colision` convierte el bloque (idéntico byte a byte en los 20 modelos con colisión de los
  13 pares nativos) y el grafo se copia con el esquema de punteros del reubicador (`Grafo`), traduciendo los
  punteros internos. `comprobar`: 13/13 idénticos.
- Segundo cierre (al lanzar desde el HUB/Duelo, con presentación): el script #SPX del donante pide las pistas
  0..2 del primer #AMC del objeto 0; B2 trae 1-7 pistas repartidas en 2 #AMC → se unen (`amc_pistas`).

## 2. Altas (registros 17+)
La tabla de 17 registros (0x82323760) está pegada a la de temas: no se puede alargar. Un alta usa el hueco del
registro **anfitrión 16 (R&R BASE)** solo mientras está cargada: `sub_820C26E0` (carga, obj+340) y
`sub_820C19B0` (id_SE) traducen 17+ → 16 con los datos del alta, y cualquier registro de serie devuelve el
original. CFG+340 conserva el 17+, así "Luchar otra vez" vuelve a cargarla. `stage.toml`:
```toml
[[escenario]]
nombre = "MONTANAS NEVADA"     # registro (<= 15)
titulo = "Montañas nevadas"    # para pantallas propias (Nombre())
entrada = 5007                 # data_cmn.afs, >= 5000 (B2: 5000+grupo, B1: 5020+grupo)
menu = 5107                    # imágenes de la casilla (entrada + 100)
id_se = 45                     # sonido/ambiente del donante
tema = 16                      # luz y cielo genéricos
```
**Nombres** (extra E1): `dbz3::escenarios::Nombre(registro, idioma)` da el nombre oficial de los 17 de serie en
inglés/alemán/francés/español/italiano (copiado de los rótulos del selector, data_<idioma> 2043) y el `titulo`
de las altas; `Total()` = 17 + altas. Las variantes (ISLAND B, TOWN B, NAMEC B, KAIOU B, EARTH) llevan el
nombre de su mapa.

## 3. Selector de escenarios con casillas nuevas
- Datos del selector ampliados (carrusel de 24, lista propia) en copias generadas de sus funciones; la lista
  es: casillas de serie desbloqueadas, altas y el "?" al final (el azar elige entre todas menos el "?").
- Cada alta trae su #AZT (entrada+100) con 4 texturas: círculo, vista, rótulo y bola de color (copiada del
  juego del jugador). Se carga antes que las del juego y se registra; mientras se dibuja una casilla nueva se
  pone su #AZT en D+4 (`ConBanco`) y luego vuelve el del juego (también como banco global).
- Cierre resuelto con el alta de B1 (medido): con el #AZT del alta como banco global, un sprite del juego pide
  su textura 37 → leía basura detrás del índice (con las de B2 salía un 0 por suerte). Arreglo: índice del
  #AZT relleno hasta 64 huecos (los sobrantes a una entrada vacía) y el banco del juego restaurado tras cada
  llamada envuelta.
- Imágenes de la casilla: foto en perspectiva del escenario sin GPU (`foto_escenario`), o `--foto captura.png`.

## 4. Herramienta
```
python tools/escenarios_convertir.py lista [--juego b1]
python tools/escenarios_convertir.py mod 7 --nombre "Montanas nevadas" --rotulo "Montañas nevadas" --mods <juego>\mods
python tools/escenarios_convertir.py mod 3 --juego b1 --nombre "Paramo al atardecer" --rotulo "Páramo al atardecer"
python tools/escenarios_convertir.py comprobar
```
Lee la ISO del jugador de `ps2_games/` y su `us/data_cmn.afs` (script, efectos y cámaras de presentación del
escenario donante 44); escribe solo en la carpeta de mods. **Nada de otro juego va en el repo ni en la release.**
Diferencias de B1 resueltas: objeto de 0xD4 B (sin cámaras: van las del donante), entradas de animación de 4 B
(`[bloque]` → `[1, bloque, parámetros, 0]`), sin parámetros (los de siempre).
`scenarios converter/b2stages.py` y `zdd_*.py` quedan superados (sus hipótesis de reubicación/#ACE no eran la
causa).

## 5. Viabilidad por juego
| Juego | Escenarios | Estado |
|---|---|---|
| Budokai 2 (PS2) | 9 grupos | Templo de Kami (g2) y Montañas nevadas (g7): selector + combate + presentación OK. g3-g6 entraron en combate en pruebas anteriores. g1 (BUDO) y g8 (TOWN) cierran al dibujar el modelo (`sub_8208E518`, lr 0x8208E5AC): pendiente. g0 sin probar. |
| Budokai 1 (PS2) | 13 grupos | 11 se convierten; Páramo al atardecer (g3) OK en Duelo y HUB. g9 y g11: cabecera de colisión distinta (no convierte). El resto, sin probar en el juego. |
| Infinite World (PS2) | 13 | Mismo formato y mismos 13 escenarios que B3: no aporta nada nuevo. |
| Shin Budokai (PSP) | BB*.AMB | Solo mallas y texturas (#AMO/#AMT/#AMM): sin grafo, colisión ni cámaras. Haría falta fabricarlos: no viable a corto plazo. |

## 6. Pruebas (copia `D:\DBZ3HD\escenarios_test`)
`python tools/pruebas.py correr escenarios_test docs/14_frentes/escenarios_pruebas/<prueba>.toml`
- Con los 3 mods (orden: 17 Montañas, 18 Páramo, 19 Templo): `final_tres` (selector con 3 casillas, combate en
  Montañas) PASS, `final_paramo` (combate en Páramo) PASS, `hub_alta` (HUB → alta 18 con presentación completa
  y ring out) PASS.
- Con 2 mods (17 Montañas, 18 Templo): `selector_combate` PASS, `selector_nieve` PASS, `revancha_nativo`
  (Luchar otra vez en el alta, reelegir y pelear en uno de serie: Llanuras) PASS.
- Solo el de B1: `selector_b1` PASS. Sin mods: `sin_mods` (selector de siempre) PASS.
- Build: `python tools/compilar.py` OK. Conversor: `comprobar` OK (13/13).

## 7. Pendiente
- B2 g1/g8 (cierre al dibujar el modelo) y B1 g9/g11 (colisión); probar el resto de grupos en el juego.
- Sonido y luz: usan los del donante (id_se 45, tema 16). Presentación de B1 con cámaras del donante (encuadres
  raros a veces). En Páramo, unas columnas del fondo salen blancas (transparencia/niebla de B1 sin convertir).
- Vista previa automática mejorable en algunos escenarios de B1 (se puede pasar `--foto`).
- R&R BASE (anfitrión) tras un alta: el código lo devuelve siempre; no se pudo elegir en el juego porque está
  bloqueado en la partida de prueba (sí se probó Llanuras tras un alta).
- Extra E2 (oscurecer poco a poco un escenario, interruptor apagado): **no hecho**. Plan: el tema del escenario
  (0x82323D80 → `sub_820C34B0` → `sub_820C11B0/1228/12A0/1320`) pone luz y niebla; falta medir qué globales lee
  cada fotograma y escalarlas con el tiempo desde un gancho por fotograma. Estimación: medio día.

## 8. Propuestas para otros frentes
- **campaigns.cpp** (`sub_82096FF0`): su control lee `REX_LOAD_U32(res)` aunque `res` sea basura (no mapeada)
  → cierre en vez de "sin efecto". Propuesta:
  ```diff
  -    const bool ok = entry && (!res || (res >= 0x10000 && REX_LOAD_U32(res) == kFxVtable &&
  +    const bool mapeado = res >= 0x40000000u && res < 0x80000000u;   // montones del juego
  +    const bool ok = entry && (!res || (mapeado && REX_LOAD_U32(res) == kFxVtable &&
  ```
- **HUB / modos / retos**: nombres de escenario con `#include "escenarios.h"` y
  `dbz3::escenarios::Nombre(stage, dbz3::settings::Language())` en vez de "escenario N"; `Total()` para admitir altas.
- **Mod Kit**: tarjeta "Escenarios de Budokai 1/2" que llame a `tools/escenarios_convertir.py mod ...` (lista los
  grupos con `lista`, el jugador pone el nombre).
- **Pruebas**: en esta máquina el arranque a veces tarda 40 s en "SDL input driver initialized" (menú a los
  55 s en vez de 18 s); no es del juego. A veces `pantallas` lee 0 con el título en pantalla: repetir.
