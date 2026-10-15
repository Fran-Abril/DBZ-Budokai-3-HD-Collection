# Frente: Tag Team (2026-10-15)

Objetivo: que TAG TEAM se sienta 100 % del juego. Plan de partida: `docs/12_conectores/13_tagteam_nativo.md`.

## Hecho (compilado y probado en `D:\DBZ3HD\tag_test`)

### 1. Entrada nativa a la rueda (solo US; EU sigue con el piloto)
- Al pulsar A en TAG TEAM ya no se pilota el menu ni se pone nuestro negro largo. El menu principal sale
  **con su propio sonido y su fundido** y el juego abre **directamente la rueda** (pantalla 14 con argumento 1),
  sin pasar por el menu de Duelo.
- Como: gancho en `sub_8218BBC0` (lo llama el menu principal al empezar cada fotograma; se distingue de sus otros
  dos llamadores por `lr == 0x8218C5F4`). Repite la rama de A del menu (`10:14940-15003`): tipo/fila guardados
  (`0x823734E8/EC`), mapa de mandos (`sub_821D98B8`), `sub_821D2778(14, 1)`, cierre del menu, `UI+37/38`,
  `sub_821D27B8(10, 0)` y estado "saliendo" `0x8218C4C8`. El panel TAG TEAM se queda encima y funde a negro en
  los mismos 10 fotogramas que el juego.
- Interruptor de prueba: `DBZ3_TAG_PILOT=1` fuerza el camino viejo (piloto).
- Evidencia: log `dbz3 tag nativo: menu principal -> rueda directa (fila del Duelo 0, mando 0)`; captura
  `02_rueda` = rueda 1P VS COM con las tiras del equipo, sin menu de Duelo
  (`D:\DBZ3HD\_pruebas\20261015-012011_tag_test_tag_nativo`).

### 2. "Atras" en la rueda vuelve a TAG TEAM
- Primer intento (saltar la creacion del menu de Duelo y pedir la pantalla 5) **crasheaba**: el principio de
  `sub_821AEBA8` coge una referencia del recurso compartido `0x823BF224` (`sub_82186220`) que la limpieza de la
  rueda suelta despues (`sub_8217AA30 -> sub_821866B0`); sin ella se liberaba dos veces (lectura de 0x8).
- Solucion final (plan B del estudio, 100 % del juego): `sub_821AEBA8(0, fila)` crea el menu de Duelo entero y en
  su primer fotograma de espera (`sub_821AE520`) sale por **su propio atras** (`sub_821AE440`, pantalla 5 y fundido,
  rama `41:16905-16960` sin el sonido). La pantalla TAG TEAM se abre encima desde ese momento: el Duelo no se ve.
- Evidencia: log `atras en la rueda -> TAG TEAM`, sin errores; captura `05_quitado` = panel TAG TEAM tras el atras;
  segunda entrada nativa correcta (`menu principal -> rueda` otra vez).

### 3. Medida del paso 0 (para el paso 2 del plan, varios miembros nativos)
Gancho de solo lectura en `sub_821ACCA0` (log al cambiar): fase, resultado, bandera y estado de cada rueda.
Medido: entrada 1 -> 3 (`sub_8217CFC8`) -> 4 eligiendo (`sub_8217CC68`); al confirmar, fase 5 en el menu de
capsulas (`sub_8217CB10`); el "atras" del menu de capsulas devuelve fase 4 + `sub_8217CC68`.
Del codigo: "elegido" (`sub_8217ACB0`) pone `+2 = 1` y **no cambia de estado** (la tarea sigue en `sub_8217CB10`
con `+10 == 1`); la vuelta a eligiendo es `sub_8217CF60(tarea)` (fase 4, resultado -1, `sub_8217BC20`,
animacion y estado `0x8217CC68`). Para el paso 2 hay que llamar a `sub_8217CF60`, no basta `+0 = 4`.

### 4. Prueba completa: PASS
`D:\DBZ3HD\_pruebas\20261015-012658_tag_test_tag_nativo` (124 s): TAG TEAM -> rueda directa -> 1 miembro ->
atras -> TAG TEAM -> rueda directa otra vez -> 2 miembros por lado (ids 0+2 contra 2+10, capsulas apuntadas) ->
combate con los dos equipos (`tag: empieza un combate`, captura `12_combate` con las fichas de compañero).
Sin errores en el log.

## Bloque 2 (2026-10-15, mismo dia)

### 5. Varios miembros por lado sin convertir A en Y (US, via nativa)
- El jugador elige personaje y capsulas con el menu del juego de verdad. En el orquestador `sub_821ACCA0`
  (antes que el juego decida el turno), si el lado tiene `+2 == 1` y a su equipo le faltan miembros
  (`TeamWantsMore`, panel), se llama a `sub_8217CF60(rueda)`: fase 4, resultado -1, su animacion y estado
  `0x8217CC68`. El juego no da el lado por terminado ni pasa al rival. El ultimo miembro sigue el flujo del juego.
- La opcion de capsulas que se apunta es la que elige el juego (Normal/Custom), no la adivinada por el mando.
- Repetido: ya no se pilota Y; su menu de capsulas sigue y al elegir vuelve a la rueda sin sumar.
- "Atras" en el menu de capsulas del **ultimo** miembro ahora tambien lo quita (antes el equipo quedaba "completo"
  con la rueda otra vez eligiendo).
- EU / `DBZ3_TAG_PILOT`: sigue la conversion A -> Y de antes.
- Evidencia: `D:\DBZ3HD\_pruebas\20261015-014049_tag_test_tag_multi` (PASS): log
  `lado 1: miembro elegido, la rueda vuelve a eligiendo` y lo mismo para el lado 2; `03_m1` = rueda eligiendo con
  "1P 1/2" y el COM bloqueado; equipos 0+2 contra 2+10 y combate.

### 6. Detalles
- **Tira sobre Edit Skills**: medido que la bandeja "Custom Tray" deja el menu en `+10 = 2`, asi que se oculta con la
  opcion 2 (Edit Skills) del menu de capsulas resaltada o abierta (y con las pantallas 5-7). Tambien la ayuda de
  abajo. Evidencia: `D:\DBZ3HD\_pruebas\20261015-014759_tag_test_tag_ability\04_ability.png` (bandeja limpia).
  Limite: con el cursor sobre "Edit Skills" (sin abrirla) la tira tambien se oculta.
- **Fichas durante la presentacion**: `HudSide.shown = !PreFight` (BattleState 1-3). Evidencia: `07_presentacion`
  sin fichas, `08/09_pelea` con fichas (run tag_multi).

### 7. Fichas del HUD con aspecto nativo
- Marco propio (assets_propios, CC0) copiado a `res/ingame_ui/tt_ficha.png`, `tt_ficha_ko.png`, `tt_swap_listo.png`,
  `tt_swap_espera.png` (y a la copia de prueba). `NativeCard` en `tagteam_panel.cpp`: cara del marcador del juego,
  marco, nombre con la letra de los menus, vida en el surco, aviso de relevo (listo pulsando / espera) y boton BACK.
  Sin los PNG se dibuja la ficha de antes (`style::PartnerCard`).
- Evidencia: `09_pelea.png` del run tag_multi (ficha dorada/azul bajo el marcador, como el resto del HUD).

## Pendiente
- **Salida del relevo (objetivo 2)**: sin tocar en esta tanda. Ideas: `DBZ3_TAG_EXIT_BTN` con guardia/paso lateral
  o la carga de ki (Y+B) en la salida; medir con `tag_swap --par rafaga=20 --env DBZ3_TAG_SLOW=2`.
- Bandeja de Edit Skills: buscar el campo que la distingue (la rama de `sub_8217C5C0` lee el global `0x8245F190`)
  para no ocultar la tira con solo resaltar la opcion.
- EU: sin tocar (piloto). Faltan los `dbz3eu_*` de `sub_8218BBC0`, `sub_821AEBA8`, `sub_821AE520`, `sub_821ACCA0`.

## Propuestas para archivos de otros
- `tools/pruebas.py` (`preparar`): copiar `res/ingame_ui/*` a `<copia>/ingame_ui/`. Hoy una copia vieja no recibe
  los PNG nuevos (los copie a mano en tag_test).

## Archivos cambiados
- `src/tagteam.cpp` (al final: `NativeEnter/NativeEnd`, ganchos `sub_8218BBC0`, `sub_821AEBA8`, `sub_821AE520`,
  `sub_821ACCA0`).
- `src/tagteam.h` (`NativeEnter`, `NativeEnd`, `WheelMenu`, `HudSide.shown`).
- `src/ingame/tagteam_panel.cpp` (A usa `NativeEnter`; fundido sincronizado; fin de modo llama `NativeEnd`;
  el atras pilotado del Duelo queda solo para el camino del piloto; bloque 2: `TeamWantsMore`, opcion real de
  capsulas, repetidos sin piloto, tira oculta sobre Edit Skills, `NativeCard`, fichas solo con el marcador).
- `src/ingame/tagteam_panel.h` (`TeamWantsMore`).
- Nuevos: `res/ingame_ui/tt_ficha.png`, `tt_ficha_ko.png`, `tt_swap_listo.png`, `tt_swap_espera.png`.

## Pruebas
- Compilar: `python tools/compilar.py` -> OK.
- Escenarios (fuera del repo, en el scratchpad de la sesion): `tag_nativo.toml`, `tag_multi.toml`, `tag_ability.toml` con
  `python tools/pruebas.py correr tag_test <ruta>\tag_nativo.toml`.

## Ronda 3 (2026-10-15, madrugada): relevo con el teletransporte del juego, fichas y bandeja

Capturas y videos: `E:\DBZ3HD_trabajo\tagteam\resumen\` (`antes_despues_relevo.jpg`, `antes_salto.mp4`,
`relevo_ida.mp4`, `relevo_vuelta.mp4`); pruebas con rafagas fotograma a fotograma en `E:\DBZ3HD_trabajo\tagteam\pruebas\`;
el resto en `D:\DBZ3HD\_pruebas\20261015-0[34]*_tag_test_*`. Escenarios: `E:\DBZ3HD_trabajo\tagteam\tt_drive.py`
(`tt_rafaga`, `tt_ko`, `tt_reto`, sobre `tools/pruebas.py`) y `esc\tt_salida.toml`, `esc\tt_menus.toml`.

### 8. Salida y entrada del relevo: el teletransporte del propio juego (US)
RE (codigo recompilado; sin pruebas a ciegas):
- **Ocultar al luchador**: el dibujo del luchador `sub_82109D90` se salta el cuerpo A (X+1616) si `X+3260` o
  **`X+3261`** no son 0; el cuerpo B (X+3280) solo esta encendido a mitad de una transformacion (`X+4924`). El juego
  usa `X+3261 = 1` para el perdedor en la pose de victoria (`sub_821017B8`) y en las lineas AP7 clase 0 valores
  **0x50/0x51** (`sub_820F9B88` casos 80/81). Cell y Cooler (codigo 0x433) parpadean asi al aparecer: oculto 0-5,
  visible 5-7, oculto 7-9, visible 9-11, oculto 11-13.
- **El contraataque de teletransporte** (acciones 39/40, ticks `sub_8210D9E8` / `sub_82108208` de la tabla
  0x8201EFC0): estela de 6 sombras en los huesos con `sub_820D85A8(13 o 14, {+12 = 1, +16 = lado})` (tabla
  0x8201D8F8) y su sonido `sub_82122488(banco 1, efecto 83, 0.0, 1.0)`.

Lo que hace ahora (`TeleportOn()`: US y fuera del modo F3; `DBZ3_TAG_SALIDA=salto` vuelve al de antes):
- Con SELECT el que sale **sigue peleando** mientras carga el compañero (la ficha muestra "espera"). Cuando esta
  listo y libre, en un mismo tick: el que sale se va con la estela y el sonido del teletransporte y el que entra
  aparece **en su sitio** con el parpadeo de Cell/Cooler (13 ticks; su estela a los 3) y despues la carga de ki.
- Antes: el que sale subia flotando sin animacion y la pantalla se quedaba ~0,8 s con el rival pegando al aire
  hasta que el otro bajaba deslizando (`antes_salto.mp4`).
- KO y ring out: sin cambios (cae, y el compañero llega desde arriba con el destello).
- Pruebas: `pruebas\20261015-032205` (ida y vuelta, rafagas), 27 relevos seguidos sin fallo
  (`D:\DBZ3HD\_pruebas\20261015-033540` 7 y `...033859` 20), `tag_swap` oficial PASS (`...042304`), KO PASS
  (`pruebas\20261015-035222`).

### 9. El modelo que se estaba cargando se perdia
Medido: con el que sale en combate durante la carga, una vez el modelo del hueco 7 quedo a 0 y el relevo tardo 10 s
(reintento por tiempo). El juego suelta las "formas sin usar" con `sub_82109380` (-> `sub_82100EB8`: todas menos la
actual y `sel+20/22`) cuando el cuerpo cambia de modelo (`sub_82111458`), al transformarse o tras un ring out.
- Impedirlo (no soltar el hueco) **cerro el juego** (lectura de 0x10 en `sub_8208D2D0` justo despues): descartado.
- Arreglo: si el modelo del hueco 7 se vacia durante la carga, se pide otra vez a los 30 ticks (antes solo tras un ring
  out; si no, 10 s). Queda una sonda que solo registra: `dbz3 tag sonda: el juego suelta el modelo del hueco ...`
  (ganchos `sub_82100D60` y `sub_82109380`).

### 10. Caras y nombres de los personajes de mods (IDs 22-26, 31, 44-63) y nombres en cada idioma
- **Marcador del juego tras el relevo**: los de mods solo estan en la tabla de caras (0x82324468) mientras el juego la
  lee al empezar (`roster_ext.cpp`, `SkillTable`). Ahora el relevo la amplia igual (`SkillTable(true/false)` en
  `StartSwap`): el que entra pone sus ojos y su cara en la barra (antes seguian los del que salia).
- **Fichas y tira de la rueda**: cara de `mods/<mod>/ui/_vista/hud.png` (o `icono.png`) y el nombre del autor, de
  `mods/_roster/roster.toml` (comentario `# Nombre (mod carpeta)` + `id = N`; no lee memoria del juego, vale en US y EU).
  Antes salia una letra o el hueco vacio.
- **Nombres en EU / idiomas**: los 38 del juego son ahora los de **sus propios rotulos de la rueda** en cada idioma
  (texturas 26-64 del #AZT 2027 de data_usi/eng/spn/fra/ger/ita; EU en ingles = US): p. ej. "C-16" en frances,
  "Cyborg Nr.16" en aleman, "Monstruo Bu" en espanol. Antes eran nuestros ("Krilin", "Freezer", "Goku (nino)") y en
  frances/aleman/italiano salian en ingles. Prueba en frances: `D:\DBZ3HD\_pruebas\20261015-042826`.

### 11. Bandeja "Custom Tray" de Edit Skills
Sonda + capturas: el menu de opciones del lado es una tarea en `wst+52` con su interior en `+48`: `+0` opcion,
`+2` paso: 1 abriendose, 2 eligiendo (tambien al mover el cursor), **3 bandeja abierta** (estado de la tarea
0x82182930), 4-5 el menu se cierra. `WheelTray(lado)` (tagteam.h). La tira solo se oculta con la bandeja abierta de
verdad, ya no con "Edit Skills" solo resaltado (EU: como antes). Prueba `...20261015-042124` (08 tira visible,
09 bandeja limpia).

### 12. TAG TEAM no se pinta encima del HUB ni del launcher
- `TagTeamOpen` no hace nada con el launcher o el menu rapido abiertos; con la pantalla HUB abierta, una X del menu
  principal se descarta y no se pinta encima (la entrada Tag Team del propio HUB sigue funcionando).
- Pruebas `...20261015-034525` y `...042124`: 02 = HUB tras X (sin TAG TEAM encima), 03 = TAG TEAM desde el HUB,
  04 = B vuelve al HUB.

### 13. Reto con el equipo (T1 de vision_futura) y equipo recordado (bugs.md P5)
- Torre nativa (`DBZ3_TEST_MODO=torre,0` + `DBZ3_TAG_TEST=0,45`): los 8 pisos con el equipo, relevo con SELECT en
  combate y victoria contada (`...20261015-035601` y `...040155`). Medido: cada piso empieza con el miembro 1; la vida
  que arrastra el reto es la del luchador que acaba el combate (puede ser el compañero) y pasa al miembro 1 del piso
  siguiente; los demas empiezan llenos; el ranking apunta al que acaba. Fuera de pruebas el reto **no usa** el equipo
  (se vacia al no estar en TAG TEAM): propuesta para modos abajo.
- P5 (`tagteam_slots.cpp`): un combate fuera de TAG TEAM (Duelo, HUB, retos, demo) olvida el ultimo equipo (el que
  se llevaba al online). De TAG TEAM directo al online se sigue llevando.

### 14. bugs.md P1 (cierre ~20 s despues de un combate con un port)
No viene de tagteam.cpp: en las dos pruebas que murieron (`015122`, `015446`) no hay equipo ni relevo (1 contra 1,
id 44 contra id 26), asi que `SwapAura` y las hojas del relevo no llegan a ejecutarse. El puntero que se libera
(r4 0x474F7720) cae dentro del ultimo #AZT cargado en ese combate (0x474F7540 + 0x1E0): recurso de un port (cara o
aura). Para el frente de ports.

### Senzu y Mecha Freezer
- Mecha Freezer: lo nativo ("al llegar a 0 se cura y se transforma", capsulas 405/406) sigue sin disparador
  localizado. Ojo: nuestro KO del relevo (`sub_8211A5A8`) se adelanta a cualquier "revivir" del juego mientras quede
  compañero. No se toca sin medirlo antes (riesgo de acabar el combate con compañeros vivos).
- Senzu: en B3 es un objeto que se usa a mano; no hay escena de "revivir". Idea compatible (Tenkaichi 3): el
  compañero en la reserva recupera poco a poco la vida perdida. Cambia el equilibrio: solo si el usuario lo quiere.

### Pendiente
- EU: sigue con el piloto y el salto de antes (faltan los `dbz3eu_*`).
- Online: el teletransporte es determinista (mismo tick en los dos PCs), pero sin probar con dos copias.
- Probar el relevo con un personaje de 7 u 8 formas (los huecos 6 y 7 son los de la carga del que entra).

### Propuestas para otros frentes
- **modos** (`modos.cpp`): opcion "con mi equipo" en Torre/Supervivencia/Kumite con `dbz3::tag::MyLastTeam()` y
  `SetDynamicTeam(0, equipo)` antes de cada combate (hoy solo con `DBZ3_TAG_TEST`); decidir si la vida arrastrada es
  del equipo o de cada miembro.
- **pruebas** (`tools/pruebas.py`): verbo `rafaga N K` (escribe "N K" en `dbz3_shot.req` y pasa los BMP a PNG):
  fotograma a fotograma a 640x360. Ya hecho fuera, en `E:\DBZ3HD_trabajo\tagteam\tt_drive.py` (`rafaga`).
- **ports**: P1 de bugs.md (ver 14).

### Archivos cambiados (ronda 3)
- `src/tagteam.cpp`: teletransporte (`TeleportOn`, `Afterimage`, `Veil`, `TeleportIn`, parpadeo en `Tick`), salida en
  `FinishSwap`, reintento rapido si se vacia el hueco, sonda `sub_82100D60`/`sub_82109380`, caras de mods en
  `StartSwap`, `WheelTray`.
- `src/tagteam.h`: `WheelTray`.
- `src/ingame/tagteam_panel.cpp`: nombres por idioma, caras y nombres de mods, bandeja, guarda HUB/launcher.
- `src/tagteam_slots.cpp`: P5 (olvidar el equipo tras un combate normal).

### Pruebas (ronda 3)
`python tools/compilar.py` OK. Copia `D:\DBZ3HD\tag_test` (con los ports de `aura_test` copiados en `mods/`).
`python E:\DBZ3HD_trabajo\tagteam\tt_drive.py correr tag_test tt_rafaga [--par extra=20]`, `tt_ko`, `tt_reto`;
`python tools/pruebas.py correr tag_test tag_swap` y `... E:/DBZ3HD_trabajo/tagteam/esc/tt_menus.toml`: todas PASS.
