# Frente: Pasarela de modos (modos de B1, B2 e IW en el HUB)

Fecha: 2026-10-15. Solo US (con `g_guest_eu` no hay modo; los dos .cpp están en el `REMOVE_ITEM` solo-EU).
Estudio por modo y formatos: `docs/16_pasarela_modos/` (README con la tabla de viabilidad).

## Ficheros del frente
- `src/pasarela.cpp`: motor del modo y su pantalla (ImGui con piezas del juego). Sin ganchos `REX_HOOK_RAW`:
  combates por el conector `dbz3::hub::RequestBattle` (hub.cpp) y resultado leído de memoria (estado
  `mgr+35128`, ganador `mgr+35136`, los de `tagteam.cpp` LabTick). Entrada del HUB `satan` (la reserva
  `hub_panel.h`: "Leyenda de Mr. Satán").
- `src/pasarela_b1.cpp`, `src/pasarela.h`: lectura en memoria de la ISO de Budokai 1 del jugador (ISO 9660,
  AFS, #AMT de PS2, tabla del ELF por firma). Nada se escribe en disco salvo el avance.
- `mod center hd/pasarela/pasarela.py`: herramienta reproducible (`info`, `extraer --salida DIR`, `prueba`), misma
  lógica que el lector del juego; lo que extrae es de la copia del jugador (no va al repo).
- `docs/16_pasarela_modos/` (estudio, `pruebas/*.toml`, `parche_hub_satan.diff`).
- `CMakeLists.txt`: `src/pasarela.cpp` y `src/pasarela_b1.cpp` tras `src/hub.cpp`, y en el `REMOVE_ITEM` solo-EU.

## Qué ve el jugador
- Antes: nada de otros juegos en el HUB.
- Ahora: con su ISO de **Budokai 1** en `ps2_games` (junto a `dbz3.exe`), el HUB tiene en su página 2 la entrada
  **"Leyenda de Mr. Satán"**: presentación con el logo, el fondo y el dibujo del original (en el idioma del juego),
  11 retadores con las charlas y avisos del original (imágenes de texto de su copia, 5 idiomas), tarjeta
  "Retador N de 11", combate nativo en el ring de los Juegos de Cell **con las reglas del original** (60 s contra
  Vegeta, sin tiempo contra Cell, Cell Jr. de 1000, vida que no se recupera, Goku y Gohan en SSJ, Cell perfecto),
  reintento al perder, avance guardado y final. Sin la ISO, la entrada sale en gris y el bocadillo explica qué falta
  y dónde ponerlo (parche del HUB aplicado por el coordinador).
- La ISO se lee en ~15 ms al llegar al menú principal (registro: `dbz3 pasarela: ... leida en N ms`).

## Pruebas (copia `D:\DBZ3HD\pasarela_test`, juego en español, `ps2_games` = enlace a `D:\DBZ3HD\ps2_games`)
`python tools\pruebas.py correr pasarela_test docs\16_pasarela_modos\pruebas\<escenario>.toml`. Con
`DBZ3_TEST_PASARELA=auto[,desde=N]` el modo avanza solo (4 s por pantalla); `DBZ3_TEST_RINGOUT=1` acaba cada
combate a los 5 s; `DBZ3_PS2_GAMES=<carpeta>` sustituye a `ps2_games` (una vacía = sin ISO).

| Escenario | Resultado | Capturas (`D:\DBZ3HD\_pruebas\`, pasada con las reglas, 04:06-04:15) |
|---|---|---|
| `leyenda_inicio` (presentación, Yamcha, charla, Ten Shin Han) | PASS | `20261015-040644_pasarela_test_leyenda_inicio` |
| `leyenda_final` (charla y tarjeta de Cell, Cell perfecto sin tiempo, final, vuelta) | PASS | `20261015-040858_pasarela_test_leyenda_final` |
| `leyenda_derrota` (sin ring out: gana el COM, "ha caído", reintento, otro combate) | PASS | `20261015-041003_pasarela_test_leyenda_derrota` |
| `leyenda_sin_iso` (carpeta vacía: no disponible, no empieza) | PASS | `20261015-041239_pasarela_test_leyenda_sin_iso` |
| `leyenda_hub` (como un jugador: fila HUB, entrada, presentación, tarjeta, B, vuelta al HUB) | PASS | `20261015-041509_pasarela_test_leyenda_hub` |
| `leyenda_hub_gris` (sin ISO: entrada en gris con "Necesita tu copia...") | PASS | `20261015-041422_pasarela_test_leyenda_hub_gris` |
| `leyenda_reglas` (Vegeta 60 s, Piccolo, Cell Jr. 1000 y su vida que sigue) | PASS | `20261015-040350_pasarela_test_leyenda_reglas` |
| `leyenda_formas` (Goku SSJ, Gohan SSJ con la vida que le quedó a Mr. Satán: 2803) | PASS | `20261015-040233_pasarela_test_leyenda_formas` |
| formas de B3 medidas con `DBZ3_TEST_BATTLE` (Goku 2, Gohan 1, Cell 2, Piccolo 2 sin cambio) | capturas | `20261015-035731_pasarela_test_forma_a`, `20261015-035828_pasarela_test_forma_b` |

Además: `python "mod center hd\pasarela\pasarela.py" prueba` OK (prueba sintética + ISO europea); los 3 ficheros
del parche del HUB compilan (`clang++ -fsyntax-only` con las opciones del build) y el parche se aplica limpio.
Compilado con `tools/compilar.py` (sin errores). 0 `UNHANDLED` en los registros de las pruebas.

## Propuestas para otros frentes (no tocados)
1. **HUB: entrada en gris con explicación** (APLICADO por el coordinador 2026-10-15). Diff:
   `docs/16_pasarela_modos/parche_hub_satan.diff` (aplicar con `patch -p1 --binary` desde la raíz; ~40 líneas).
   Añade `RegisterGate(clave, ready, why)` y `EntryWhy(e)`: `EntryReady` = registrada y `ready()`; el bocadillo de una
   entrada en gris dice `why()` en vez de "Muy pronto". Incluye el cambio en `pasarela.cpp` (registro al arrancar
   con la puerta, en vez de esperar a la ISO). Probar con `pruebas/leyenda_hub_gris.toml`.
   Nota: `hub.md` dice que `pasarela.cpp` registra `pasarela`; ya no: registra `satan`. El texto de `pasarela`
   ("Trae luchadores de B1/B2/IW") es del importador, no de este frente.
2. **Conector (`hub.cpp`): tiempo y vida por combate** (HECHO: `RequestBattleEx`, ya en uso) (para las reglas del original: 60 s contra Vegeta, sin tiempo
   contra Cell, vida que no se recupera, 5 Cell Jr. de 1000). Boceto:
   ```cpp
   struct Request { ...; int time_s = 0; int hp[2] = {0, 0}; };   // 0 = lo de serie
   bool RequestBattleEx(int id1, int id2, int stage, bool p2_com, int time_s, int hp1, int hp2);
   // en el gancho de sub_821D7C30, tras escribir ID/traje/mando de cada lado:
   if (g_req.time_s) REX_STORE_U32(kCfg + 348, uint32_t(g_req.time_s));   // s; <= 0 sin tiempo (doc 10 s2.2)
   if (g_req.hp[k] > 0) { REX_STORE_U32(rec + 36, g_req.hp[k]); REX_STORE_U32(rec + 40, g_req.hp[k]); }
   ```
   (`modos.cpp` escribe la vida en la espera de la escena 24; medir si escrita aquí se respeta.) Con eso, la
   pasarela pasaría `vida_jugador` (la que quedó, `mgr+2096+6080` al acabar) y el tiempo de la tabla de B1.
3. **Forma inicial pedida por código** (HECHO: `RequestBattleEx`, campo forma = rec+20) (`tagteam_panel.cpp`, dueño del selector LT/RT): un `SetStartForm(lado, forma)`
   que valga para el siguiente combate; la tabla de B1 trae Goku SSJ, Gohan SSJ y Cell perfecto (hoy Cell sale en
   su primera forma).
4. **Índices de docs**: una línea en `docs/README.md` (`16_pasarela_modos/`) y otra en `docs/04_herramientas/TOOLS.md`
   (`mod center hd/pasarela/pasarela.py`).
5. **Release / Kit** (`tools/modpacks/LEEME_PS2_GAMES*.txt`): decir que la ISO de Budokai 1 en `ps2_games` activa
   la "Leyenda de Mr. Satán" del HUB (y `make_modpacks.py`: incluir `mod center hd/pasarela/`).
6. **Ideas del coordinador** (`vision_futura.md` G2/G3): el motor de pasos de `pasarela.cpp` (charla → tarjeta →
   combate → resultado) sirve tal cual para una misión con Gotenks gordo y para un "tablero de 6 casillas" propio
   (falta solo la pantalla del tablero); ver `docs/16_pasarela_modos/02_budokai2.md` §2.

## Lo que queda (de este frente)
- Reglas en código del original (armadura de A16, Mr. Satán herido, ki lento contra Piccolo), ki inicial del rival,
  dificultad Novato/Experto/Avanzado (nivel del COM, sin localizar en B3),
  puntos y pantalla de resultado del original (entrada 668), voces.
- Textos propios en francés, alemán e italiano (hoy salen en inglés; las charlas del original sí van en los 5).
- ISO americana de B1 (sin copia en disco: la firma de la tabla y del banco deberían valer; sin probar).
- Siguientes modos: Torneo de B2 (bajo, con el torneo nativo), misiones de IW que son combates (medio), Dragon World
  (alto). Detalle en `docs/16_pasarela_modos/`.
