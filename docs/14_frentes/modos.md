# Frente: Modos (Torre, Supervivencia, Kumite, Torneo con bots, Ranking, Laboratorio) — 2026-10-15

Solo US (con `g_guest_eu` todo queda apagado). Copia de pruebas: `D:\DBZ3HD\modos_test` (con los mods de
`oficiales_test` para probar personajes de mods). Escenarios de prueba: `E:\DBZ3HD_trabajo\modos\escenarios\`.

## Qué ve el jugador
- **HUB > Torre / Supervivencia / Kumite**: empieza el reto con su último luchador del Duelo. Cada combate es el
  del juego (escena 25 del conector `hub.cpp`). Durante el combate, bajo el reloj, el marcador del HUB
  (`retos/marcador_<modo>` + cifras doradas): "Piso 2/8", "Oleada 3/10", "Jefe 1/5" (+ "¡Aguanta!").
- **Tras ganar** no sale el menú "Battle Again?": del WINNER! se pasa a una pantalla propia con el aspecto del
  Duelo (fondo, Kaio, cinta con el nombre del reto, marco, bocadillo): "Piso 1 de 8: ¡superado! / Siguiente:
  Raditz / Vida 2700/3000" y a los ~2,5 s el combate siguiente. **Tras perder** sí sale el menú del juego:
  "Battle Again?" = reintentar ese combate; las otras dos opciones acaban el reto.
- **Salir desde la pausa o cerrar el juego** a mitad: el reto queda guardado y la próxima vez sigue en ese
  combate, con su luchador y su vida.
- **Al acabar**: pantalla final ("¡Torre completada!", "Derrota", "Reto guardado") con la marca y el récord;
  A vuelve al HUB.
- **HUB > Ranking**: 4 páginas (LB/RB o izquierda/derecha): Elo de cada luchador con medalla (bronce < 1000,
  plata 1000, oro 1100, platino 1200, diamante 1300) y victorias-derrotas; récords de Torre, Supervivencia y
  Kumite por luchador (mejor marca, veces completado con medalla) y el reto guardado si lo hay. B vuelve.
- **HUB > Laboratorio**: resumen del último combate jugado (cualquier modo): duración, daño, golpes y combo más
  largo de cada luchador. A activa/desactiva la captura por fotograma (`user_data/dbz3/laboratorio.csv`).
- **Torneo Mundial con bots elegidos**: con `user_data/dbz3/modos/torneo.toml` (`participantes = [ids]`) los COM
  del cuadro del Torneo del juego son esos (en orden; los que falten, al azar). Un ID sin casilla propia (mods
  22-26/31, 44-63) sale en el cuadro con la casilla de su anfitrión y pelea con su propio ID.
- Modificadores por TOML (reto o combate): `tiempo` (límite en s), `regen` (vida/s que recupera el rival),
  `objetivo_tiempo` (aguantar esos segundos = victoria). `nombre`/`name` se ven en la cinta de las pantallas.

## Ficheros (de este frente)
- `src/modos.cpp` (en `DBZ3_SOURCES` y en el `REMOVE_ITEM` de la build solo EU).
- `mod center hd/modos_reto.py`: `nativo <tipo>` (retos nativos, con `--nombre/--name/--tiempo/--regen/--objetivo`),
  `torneo --participantes ...` y `--selftest` (ya no exige las imágenes de Freezer: el Kit no lleva arte de
  terceros y `campana.py` usa el del juego; cada modo se monta en su propia carpeta temporal; valida también
  los 15 `nativo_*.toml` del catálogo).
- `mod center hd/campanas/retos/nativo_{torre,supervivencia,kumite}.toml` (los retos por defecto del juego).
- `tools/ranking.py` (mismo Elo que el juego; `--selftest`; resume `retos.csv`).

## Ganchos de `modos.cpp` (nadie más debe engancharlos)
| Función | Para qué |
|---|---|
| `sub_821D7400` | espera de la escena 24: con la 25 pedida y un combate del reto, completa la CFG 0x8245EFC0 (traje, vida rec+36/+40 de los dos lados, tiempo +348) |
| `sub_820FEBA8` | fin del combate (antes de soltarlo): ganador `mgr+35136`, motivo `mgr+35132`, vida `X+6080`; cierra el resumen del laboratorio |
| `sub_820FF750` | estado 7 (menú final) cada fotograma: tras ganar un reto elige "Reselect" sola (`*0x82324064 +4272` bit 0 = eligiendo, `+4292` = fila) |
| `sub_820FD678` | comprobación de cada fotograma de pelea (estado 4): laboratorio y `regen` |
| `sub_821A28A8` | sorteo del Torneo: los COM de la lista |
| `sub_821A5A30` | preparación de cada combate del Torneo: ID propio de los participantes sin casilla |

## API (para el HUB y otros frentes; declarar `extern` igual)
```cpp
namespace dbz3::modos {
struct Estado { int fase = 0, tipo = 0; int ganados = 0, total = 0; int rival = -1; int resultado = -1;
                int vida = 0, vida_max = 0; int jugador = 0; };   // fase 0 nada, 1 combate, 2 entre, 3 fin
struct Puesto { int id = 0, elo = 1000, ganados = 0, perdidos = 0, empates = 0; };
bool Start(int tipo, int jugador, int traje);   // 0 torre, 1 supervivencia, 2 kumite (sigue el guardado si hay)
bool Continue();                                // con `pausa = true` en el TOML
void Abort();                                   // deja el reto sin guardarlo
Estado Get();
void SetTorneo(const int* ids, int n);          // n = 0: torneo.toml o sorteo nativo
int Ranking(Puesto* out, int max);
bool Record(int tipo, int* id, int* mejor, int* total, int* completados);   // mejor marca del modo
bool Guardado(int tipo, int* id, int* ganados);                             // reto a medias
}
```
Ya registrados en el HUB (`RegisterMode`): `torre`, `supervivencia`, `kumite` (busy = reto en curso o pantalla
final abierta), `ranking` y `laboratorio` (busy = su pantalla abierta).

## Datos guardados
- `user_data/dbz3/hub_modos.toml` (versión 1; escritura `.tmp` + rename, la versión anterior en `.bak`; si el
  principal no se lee, se usa el `.bak`): `[[marca]]` por modo y luchador (mejor, total, intentos, completados,
  reto), `[[en_curso]]` (reto a medias), `laboratorio` (captura sí/no) y `[ultimo_combate]` (resumen que enseña
  el Laboratorio también tras reabrir el juego). Va junto a la partida (el juego solo
  tiene una ranura, `<perfil>/4E4D0856/00000001`), como `historias_extra.bin`.
- `user_data/dbz3/retos.csv`: una línea por reto (fecha, tipo, jugador, ganados, total, resultado).
- El ranking se calcula de `combates.csv` (LabTick, `tagteam.cpp`): no hace falta guardarlo aparte.
- Probado: jugar, cerrar, abrir y ver el récord y el reto guardado (`20261015-032704_..._guardar_a`,
  `20261015-032907_..._guardar_b`).
  Laboratorio tras reabrir: `20261015-035542_..._lab_a` + `20261015-035954_..._lab_pantalla`.

## Medido (2026-10-15, US)
- Vida: `rec+36` = la del principio, `rec+40` = máxima (`sub_82109638`). Saibaman (42) y Cell Jr. (43): la vida
  son unidades de 1000 (3000 = tres Saibaman).
- Tiempo: CFG `+348` en **segundos** (`sub_820FE668` x60). Con segundos x60 (como deja `sub_82179408` las
  opciones) el combate duraba 60 veces más. Tiempo agotado = motivo 3. El marcador del juego sigue con ∞.
- Menú final: `sub_820DF568` devuelve la fila (2 = Reselect → R 4; 3 = Return → R 2; 0/1/4 = Battle Again).
  Estados del combate: 5 acabado, 6 WINNER!, 7 menú, 8 salida (`sub_820FF820` + fundido → `sub_820FEBA8`).
- Tabla `0x8201EDA8`: función al entrar en cada estado (4 `sub_820FD2B8`, 5 `sub_820FD408`, 6 `sub_820FF338`,
  7 `sub_820FD560`, 8 `sub_820FF820`); `sub_820FD678` es la comprobación de cada fotograma de pelea en el modo 5.
- CFG modo 2 (reglas del Torneo) en la escena 25: se queda en READY? (no sirve).
- Escenarios 0-7 válidos (0 Torneo con ring, 1 Templo de Kami, 2 isla, 3 ciudad, 4 rocas, 5 prado, 6 montañas, 7).
- Torneo: el humano puede ser el lado 2 (`partida+31+p` sigue a `G+36688+p`).

## Pendiente
- ~~Encadenar sin pasar por el menú principal~~: hecho en la ronda 2 (frente rpg, `hub::ChainBattleEx` del
  coordinador; ver `rpg.md`). Elegir luchador, premios y retos nuevos: también en `rpg.md` (selector del HUB).
- El marcador del juego enseña ∞ aunque haya límite de tiempo (el combate sí acaba a su hora).
- `regen` sube la barra en curso (la vida del luchador es por barra), no devuelve barras perdidas.
- Elegir luchador para el reto (hoy: el último del Duelo); premios y desbloqueos por completar.
- Torneo: no hay entrada en el HUB para elegir el torneo; se pone con `modos_reto.py torneo` (o el Kit).

## Propuestas para ficheros de otros frentes (no tocados)
1. **`src/hub.cpp` (coordinador)**: `bool ChainBattle(int id1, int id2, int stage, bool p2_com)` llamable con
   `OwnBattle()`: en `sub_821D3628`, si hay uno pedido, en vez de r3 = 2 dejar `T+28 = 0x821D7400` y pedir la 25
   otra vez (como dice §9 de `10_arranque_combate_nativo.md`), sin menú principal entre combates. Y un
   `void SetCfgPatch(void (*fn)(uint8_t* base, uint32_t cfg))` llamado tras rellenar la CFG: con él `modos.cpp`
   dejaría su gancho de `sub_821D7400`.
2. **HUB (`hub_panel.cpp`)**: en el bocadillo de Torre/Supervivencia/Kumite, el récord y el reto guardado con
   `dbz3::modos::Record()` y `Guardado()` ("Récord: piso 5/8 con Goku · Guardado: piso 3").
3. **Tag Team (`tagteam.cpp`)**: `LabTick` solo captura con `DBZ3_LAB`; el laboratorio de `modos.cpp` ya hace su
   propia captura, así que no hace falta cambiarlo.
4. **Ports**: Vegeta GT (en estas copias es el **ID 26**, mod `imp_iw_vegeta_gt`; el 44 es Trunks SB2 y va
   bien) cierra el juego **al salir del combate** (estado 8, tras el menú final), también sin `modos.cpp`
   (`D:\DBZ3HD\_pruebas\20261015-015446_modos_test_vegeta_gt_sin_modos\dbz3.log`, combate de `hub.cpp` 44 contra 26):
   lectura de guest 0x10, `lr=0x82089C34`, `r3=0x401099F0`, `r4=r31=0x474F7720`, `r11=0x474F7700`.
   Primera vez: `...20261015-015122_modos_test_torre_mods` (piso 3).
5. **Mod Kit**: la barra "Modos de reto" puede llamar a `modos_reto.py nativo <tipo> --juego <carpeta>` y a
   `modos_reto.py torneo --participantes ... --juego <carpeta>`.

## Incidente (avisado al coordinador)
Al mover mis escenarios del scratchpad a `E:` moví por error el contenido del scratchpad **compartido** de la
sesión (52 carpetas de otros agentes, 4,4 GB) durante ~2 min. Se devolvió todo (robocopy sin sobrescribir lo
más nuevo; ninguna falta) y la copia duplicada quedó en `E:\DBZ3HD_trabajo\modos\_movido_por_error_desde_scratchpad`
(se puede llevar a `D:\_PARA_BORRAR_CLAUDE\` cuando se quiera).
