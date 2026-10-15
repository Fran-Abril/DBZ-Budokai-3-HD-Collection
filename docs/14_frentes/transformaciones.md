# Frente transformaciones (ronda 2, 2026-10-15)

Petición: "más transformaciones, poder poner (como en Infinite World) todas en una única técnica" (ficha 14).

## Qué ve el jugador
- **Antes:** cada forma es una cápsula (Goku: Kaioken, SSJ, SSJ2, SSJ3, SSJ4 = 5 huecos de 7) y hay que
  equiparlas en cadena (SSJ2 pide SSJ...).
- **Ahora:** cada personaje con 2 o más transformaciones tiene además una cápsula nueva, **"Todas las
  transformaciones"** (EN "All Transformations", FR/DE/IT traducidas), en Edit Skills junto a las suyas.
  Equipada, P+K+G sube por todas sus formas como siempre (cada forma pide su ki de siempre) y se baja igual que
  siempre (sin ki, un golpe). Las cápsulas originales siguen igual y se pueden seguir usando.
- **Llega a todos los jugadores:** el propio juego hace su nombre y su panel al arrancar, desde los archivos del
  jugador (nada de Kit ni Python). La release no lleva nada de esto.
- **Solo da las formas desbloqueadas** (las que tienes en el inventario, en orden: sin SSJ2 se queda en SSJ),
  como pide la ficha ("conserva formas desbloqueadas"; no regala SSJ4). Las de los personajes nuevos (mods)
  cuentan siempre como tuyas.
- Personajes del juego que la tienen: Goku, Gohan joven, Gohan adulto, Vegeta, Trunks, Piccolo, Freezer, Cell.
  Ports con 2+ formas (Goku SDBH, Goku GT, Vegeta GT, Gohan del Futuro...) también. No entran las formas
  especiales (nave de Freezer/Cooler, Babidi, absorciones de Buu) ni las fusiones (reglas propias, como dice la ficha).

## Coste en huecos (criterio)
- Nativo: cada transformación cuesta 1 hueco (#SKC +8 = 0x11). La cadena completa de Goku son 5 de 7.
- Cápsula única: **la mitad de la cadena, redondeando arriba** (`forms::UnifiedCost`): 2 formas → 1 hueco,
  3 → 2, 4 → 2, 5 → 3 (Goku: 3 en vez de 5). Como en IW, la técnica única libera huecos para golpes, pero no es
  gratis; el ki que pide cada forma no cambia.

## Cómo está hecho (para el coordinador)
- `src/transformaciones.cpp` (nuevo):
  - gancho `sub_82112260` (preparación de cápsulas del luchador, también en cada relevo del Tag Team): si lleva
    su cápsula única (1984 + ID), enciende `X+8156+f` (forma permitida) de las formas que agrupa. La subida nativa
    (`sub_82119108`) y la bajada (`sub_82119258`) usan esos bytes. Online no hace nada (cada PC tiene su partida).
  - `PrepareNamesMod` (lo llama `roster_ext` al arrancar, antes de que el guest lea nada): genera
    `mods/_forma_unica` desde los AFS del jugador. Lee el banco de nombres de cada personaje con formas (el del
    `_roster` si lo sobrescribe) y 2663/2684, les añade "Todas las transformaciones" (texto con stb_truetype, la
    copia de ImGui; DXT3 y #AZT como `capsulas.py`) y hace el panel de 5 líneas. Solo el `data_*.afs` del idioma
    activo (los 6 ocupaban ~110 MB; uno, ~18 MB), **sin comprimir**: el cargador del juego (`sub_82084210`) solo
    descomprime si ve la cabecera LZX (`sub_820A4700`) y si no, copia tal cual (medido; no hay codificador LZX en
    C++). Entradas **nuevas** (nº de entradas + 1024): no pisa nada ni choca con el `_roster`. Se rehace solo si
    cambian los AFS, el idioma, el `_roster` o los bancos (`stamp.txt`); tarda ~0,1 s. Va en `mods/` y no en
    `user_data` porque el lector de AFS del runtime (`rexruntime.dll`, canónica) solo sirve ficheros de `mods/`.
    El LZX del `_roster` va por bloques de 2 MB (cabecera +0x28): se descomprime bloque a bloque.
  - prueba: `DBZ3_TEST_FORMAS`/`_AUTO`/`DBZ3_TEST_PKG` (gancho `sub_82110B08` y `PruebaMando`, solo con variable).
- `src/roster_ext.cpp` (secciones de cápsulas): registros 1984 + ID en el catálogo ampliado (plantilla = su
  primera transformación; sin requisito ni efecto; coste); `UnifiedForms`/`UnifiedListed` (la cápsula única se
  ve como tuya en Edit Skills sin escribirla en el inventario: la partida no cambia); `[forma_unica]`; los
  bancos de nombres se cambian por sus copias al cargarse (`sub_82083968`); en `sub_82146430` una cápsula sin
  nombre en el banco ya no lee fuera de él.
- `src/form_selection.h`: `UnifiedId`, `Groupable`, `UnifiedCost`; `MissingCapsule` acepta la cápsula única →
  la forma inicial con LT/RT y el Tag Team la tienen en cuenta sin más cambios.
- `mod center hd/forma_unica.py`: **respaldo** (los 6 idiomas, comprimido). El juego respeta su salida mientras
  el `_roster` sea el mismo; ni el Kit ni `roster_build` necesitan llamarlo (el juego se rehace solo con el
  `build.hash` nuevo). `--autoprueba` comprueba los bancos.

## Límite de formas por personaje (medido)
- **8 formas (normal + 7)** y no se puede ampliar sin rehacer el luchador: el registro de combate (char372,
  372 B) acaba justo en la tabla de 8 formas (+212 + 20·8 = 372), el luchador tiene 8 bytes de "forma
  permitida" (`X+8156..8163`), 8 huecos de modelo (`X+7608`) y de moveset (`X+7672`), y la subida nativa para
  en la 7 (`sub_82119108`: `f == 7`, bucle < 160 B).
- La cápsula única cubre las 7. Ports con más formas que B3 (Goku SDBH, 5 formas) las recorre todas (probado).
- Tag Team usa los huecos 6 y 7 para el relevo: un miembro solo puede relevarse en las formas 0-5.

## Pruebas (copia `D:\DBZ3HD\transformaciones_test`, capturas en `D:\DBZ3HD\_pruebas\`)
`python docs/14_frentes/transformaciones_pruebas/formas.py ESCENARIO` (combate nativo del HUB, 1P contra Vegeta).
- **P+K+G de verdad** (`pkg_*`): `DBZ3_TEST_PKG=1` pulsa P, K y G en el registro del mando 1 (los botones que la
  configuración da a cada uno, recién pulsados en el mismo cuadro), desde el gancho de mando de `combate.cpp`
  (`sub_8208FD58`, **2 líneas añadidas** al principio, avisado). Rival quieto con el muñeco de combate. El juego
  hace todo lo demás por su entrada nativa.
- Las de `AUTO` dan el ki y hacen lo que sigue a P+K+G (decisión nativa + carga + cambio nativo `sub_82115330`).
- El P+K+G de 3 **teclas** a la vez (teclado → XInput) llega al juego (E000) pero no transforma ni con la
  cápsula clásica: límite de la inyección por teclado, no de la cápsula (por eso el registro del mando).

| Escenario | Resultado | Carpeta (`D:\DBZ3HD\_pruebas\...`) |
|---|---|---|
| **pkg_unica** (P+K+G real, cápsula única) | PASS: normal → Kaioken → SSJ → SSJ2 → SSJ3 por la entrada nativa | 20261015-130005_transformaciones_test_formas_pkg_unica (mosaico.png) |
| **pkg_kaioken** (P+K+G real, cápsula suelta de Kaioken) | PASS: normal → Kaioken y ahí se queda | 20261015-130435_transformaciones_test_formas_pkg_kaioken (mosaico.png) |
| goku (5 formas, AUTO) | PASS: normal → Kaioken → SSJ → SSJ2 → SSJ3 → SSJ4 → baja a normal | 20261015-104330_transformaciones_test_formas_goku (mosaico.png) |
| sdbh (port ID 50, 5 formas) | PASS: normal → sus 4 transformaciones → baja | 20261015-104548_transformaciones_test_formas_sdbh (mosaico.png) |
| parcial (solo Kaioken y SSJ) | PASS: llega a SSJ y no pasa a SSJ2 | 20261015-104723_transformaciones_test_formas_parcial (mosaico.png) |
| inicial (DBZ3_START_FORM=3) | PASS: empieza en SSJ2 solo con la cápsula única | 20261015-111543_transformaciones_test_formas_inicial |
| inicial_control (sin cápsula única) | PASS: empieza normal | 20261015-105846_transformaciones_test_formas_inicial_control |
| tag (equipo Goku + Goku SDBH) | PASS: relevo; el que entra sube por sus 4 formas | 20261015-110011_transformaciones_test_formas_tag (mosaico.png) |
| Edit Skills, nombres **hechos por el juego** (inglés) | "All Transformations" en la lista y en la bandeja (3 huecos); `_forma_unica` generada en 0,06 s | 20261015-132509_transformaciones_test_edit_skills (mosaico.png) |
| Edit Skills, nombres hechos por el juego (español) | al cambiar de idioma se rehace con data_spn: "Todas las transformaciones" | 20261015-132823_transformaciones_test_edit_es (mosaico.png) |

Con el código final (nombres hechos por el juego, inyección P+K+G): `pkg_unica` otra vez PASS
(20261015-133444) y batería `python tools/pruebas.py todo` **5/5 PASS** (las copias smoke generan su
`_forma_unica` al arrancar sin problema). `forma_unica.py --autoprueba`: OK.

**Panel de descripción:** en esta pantalla (Edit Skills de la selección) no sale ni con las cápsulas originales:
la ayuda del propio juego (botón BACK, captura `14_back`) solo ofrece elegir hueco, confirmar, quitar y cancelar;
X en la lista o en la bandeja no lo abre (probado con Kaioken y con la única). La función del panel
(`sub_821BB680`) la usan las pantallas de cápsulas fuera del combate (tienda / colección: `sub_8214C090`,
`sub_82159C30`, `sub_8215BE28`...), que solo recorren las 594 originales: ahí no salen ni las de los ports ni la
única. El panel está generado (entrada `descripcion`) y enganchado como el de las cápsulas de los ports.

Variables de prueba (solo copias): `DBZ3_TEST_INV="1,2"` (formas desbloqueadas sin tocar la partida),
`DBZ3_TEST_OWN="1,2"` (cápsulas tuyas en Edit Skills), `DBZ3_TEST_FORMAS[=ID]`, `DBZ3_TEST_FORMAS_AUTO=1`,
`DBZ3_TEST_PKG=1`. Para `tag`: `transformaciones_pruebas/zz_prueba_forma_unica` en `<copia>\mods\`.

## WL-14: volver a Clásico sin cambiar la partida (`studies/11_vision_futura/INTEGRACION_SEGURA.md`)
Cómo se garantiza: la cápsula única **no se escribe nunca en la partida** (en Edit Skills se ve como tuya de forma
virtual, `UnifiedListed`; antes se escribía un 1 en el inventario) y, al preparar cada luchador (`sub_82112260`),
una cápsula que ya no está en el catálogo (lista guardada con la única y luego jugada sin ella: mod apagado o ISO)
sale de la **copia** de la lista del combate, no de la partida. Prueba: partida en memoria (`0x824BA110 + 0x150`,
177 817 B, inventario incluido) comparada byte a byte antes y después, y `data.bin` (sha1) antes y después.

| Caso | Resultado | Carpeta (`D:\DBZ3HD\_pruebas\...`) |
|---|---|---|
| `wl14_combate`: `_forma_unica` apartada a `mods_respaldo`, el juego la rehace, sube a SSJ3 con P+K+G real | **PASS**: memoria igual, `data.bin` igual | 20261015-180253_transformaciones_test_formas_wl14_combate (con el arreglo; antes 161235) |
| `wl14_sin_mod`: cápsula única apagada (`.disabled`, = sin nombres / ISO) y lista guardada `1, 1984` | **PASS**: «1984 fuera del catálogo: no entra en el combate»; la Kaioken suelta sube con P+K+G real y se para ahí; memoria y `data.bin` iguales | 20261015-173841_transformaciones_test_formas_wl14_sin_mod (con el arreglo de abajo; antes 161412) |
| `wl14_limites`: ki y fase bloqueada (solo Kaioken y SSJ desbloqueados) con P+K+G real | **PASS**: sin ki no sube; con ki Kaioken → SSJ; en el tope no pasa a SSJ2 | 20261015-180436_transformaciones_test_formas_wl14_limites (con el arreglo; antes 161752) |
| `wl14_edit`: equipar la única en Edit Skills, quitarla y salir | **PASS**: memoria igual en cada paso, `data.bin` igual | 20261015-153053_transformaciones_test_formas_wl14_edit |
| `wl14_ranuras`: hueco y requisitos en Edit Skills (única = 3 huecos + Kaioken, SSJ, SSJ2, SSJ3) | **PASS**: bandeja 7/7 (r5_bandeja); SSJ4 ya no cabe (r4); las sueltas siguen pidiendo su requisito (grises hasta tener la anterior) | 20261015-174502_transformaciones_test_formas_wl14_ranuras |

`data.bin` = `44005a9a…` en todas. **ISO:** sin `data_*.afs` sueltos el juego no genera `_forma_unica` y la
cápsula queda apagada: es el mismo camino que `wl14_sin_mod` (probado así, no con una ISO de verdad).

**Arreglo 2026-10-15 17:30 (lista de cápsulas doblada, aviso de campanas):** la limpieza de arriba escribía con
`REX_STORE_U16(rec + 54 + 2 * kept++, cap)` y esa macro evalúa la dirección **dos veces**: cada pase doblaba la
cuenta y pisaba la lista (Súper Bu `1 [124]` → `4 [124 0 0 0]`, Piccolo `3 [57 59 61]` → `12 [57 59 59 0 59…]`:
perdía Special Beam Cannon). Encontrado con un punto de parada de datos en `+52` del registro de combate
(`E:\DBZ3HD_trabajo\transformaciones\watch52.py`: la escritura salía de `dbz3_hook_sub_82112260`). Ya arreglado
(`kept` fuera de la macro). Afectó a **todos los combates** con cápsulas de los exe compilados entre ~15:24 y
17:30 (pruebas de otros frentes de esa franja con técnicas o cápsulas: repetir si dependían de ellas).
Comprobado: en combate Súper Bu `1 [124]` y Piccolo `3 [57 59 61]`, igual que la CFG
(20261015-173648_transformaciones_test_cap_probe). La **tabla** `0x8234ABB8 + 96·ID` no la cambia nadie al
arrancar: de las 105 entradas solo difieren las de los ports del `_roster` (IDs 22-26, 31, 44-50, que el roster
escribe a propósito) y, en mi copia, el parche de prueba `zz_prueba_forma_unica` (0 y 50); Súper Bu (35) = 1 [124]
como el ejecutable (`char96_cmp.py`, con mi `_roster` y con el de campanas).
Tras el arreglo: compilado OK, `wl14_*` otra vez PASS y `python tools/pruebas.py todo` 4/5 + `tag_swap` repetido
PASS (20261015-181656; ese relevo falla a ratos desde antes: 14:53, 15:21, 17:05).

## Puntuación
- **Ficha 14: 1.** Cápsula nueva por personaje, todas sus formas con la subida (P+K+G real) y bajada nativas,
  nombre hecho por el juego para todos los jugadores (probado en inglés y español), en Edit Skills como las de los
  ports, coste documentado, Tag Team y forma inicial (LT/RT) probados.
- Más transformaciones: límite medido (8 formas) y la cápsula única recorre las 5 formas del port SDBH. No se
  puede ampliar por encima de 8 sin rehacer el luchador.

## Lo que queda / propuestas a otros frentes
- **eu**: mis cambios de WL-14 (propiedad virtual en `roster_ext.cpp`, limpieza de la lista y el arreglo de la
  macro en `transformaciones.cpp`, gancho `sub_82112260` / `dbz3eu_sub_82112360`) van encima de vuestra
  conversión; no he tocado `eu_map*`. Si regeneráis borradores de esos ficheros, partid de la versión actual.
- **hub**: el filtro de huecos a 0 en `Launch` ya no hace falta (la tabla estaba bien; el fallo era la limpieza
  de arriba), pero no estorba.
- **campanas**: la nota «Cápsulas de Súper Bu» de `campanas.md` queda resuelta (ver WL-14, arreglo).
- **combate**: en su gancho de mando (`sub_8208FD58`) añadí 2 líneas (declaración + `dbz3::formas::PruebaMando`)
  que solo actúan con `DBZ3_TEST_PKG=1`. `sub_821100B0` es suyo; mi prueba usa `sub_82110B08`.
- **online**: `mods/_forma_unica` aparece como un mod más (los dos PCs lo generan igual desde el mismo juego); si
  la comprobación de mods del online da problemas, excluirlo como los packs de texturas (solo es aspecto; la
  cápsula no hace nada online).
- **launcher**: verá `_forma_unica` en la lista de mods (como `_roster`); se puede ocultar igual que este.
- **native_ui.cpp**: su `ReadEntry` decodifica de una tira y falla con entradas LZX de varios bloques (las que
  hace `xbcompress` de más de 2 MB); en transformaciones.cpp hay la versión por bloques (+0x28). Propuesta: usar
  una sola función para los dos.
- Modo ISO (juego dentro de una imagen de disco): no hay `data_*.afs` sueltos y el juego no puede hacer el nombre
  (log "imagen de disco?"); la cápsula no se activa sin él. EU: inventario `0x824BA100 + 0x2AFE9` deducido, sin probar.
- La lista de habilidades de la pausa sigue enseñando las transformaciones de la ficha nativa del personaje.
