# Roadmap ronda 3 (2026-10-15 noche)

## Hecho en el bloque 1 (23:20, sin publicar; build `out/build/win-amd64-dual`, instalado en win-amd64-release)
- **Sin menú final del juego** en ningún combate del HUB (retos, pasarela, laboratorio): `modos.cpp` gancho
  `sub_820FF750` elige solo la salida. Perder un reto con revancha: pantalla propia "Derrota" con A Reintentar /
  B Dejar el reto (`FinCombate` → `Encadenar(false)`, `DibujarEntre`). Los retos de rpg.cpp acaban al perder.
  Prueba `batalla` PASS (log `sin menu final`). La revancha propia no se probó perdiendo de verdad.
- **Laboratorio**: filas Luchador y Rival (izquierda/derecha, cualquier jugable) y fondo `hub/fondo_lab`.
- **Bordes dorados**: opción F1 > Imagen > "Bordes dorados" (`dbz3_ui_side_borders`, apagada de serie). Sin ella,
  nuestras pantallas no pintan tiras y los fondos llenan la ventana (`style::Borders`, `Backdrop`, `Host`). Las
  tiras de los menús del propio juego siguen: textura `borde` (XXH3 0x4B735AD364D513BA) → pendiente en el SDK.
- **F2** enciende/apaga el pack HD (`main.cpp`, bind `bind_dbz3_hd_pack`).
- **Fondos del autor** (`assets_propios/png/add`, `generar.py fondos_hub`): HUB (`hub/fondo`), Laboratorio,
  Modo Héroe (`rpg::Heroe`). El fondo de Namek de `pack_hub` se apartó a `mods_respaldo/ronda3_2026-10-15`.
  El logo ya estaba integrado (hub/logo).
- Prueba nueva `hub_lab` PASS (capturas `D:\DBZ3HD\_pruebas\20261015-232021_hub_test_hub_lab`).

- **Tienda como la Skill Shop** (23:40): botellas de cápsula, caja de Zenie y rótulo ZENIE sacados en marcha del
  data_usi del jugador (`native_ui.cpp`: 2064 capsulas, 2053 caja_zenie/letras_zenie; `rpg.cpp` ListaTienda/CajaZenie).
  Prueba `tienda` PASS. Volcado de piezas con hash: `E:\DBZ3HD_trabajo\ui_dump2` (hojas `_hoja_<entrada>.jpg`).
  La tira dorada `borde` es la misma textura en 6 pantallas (2033, 2039, 2043, 2049, 2055, 2062).
- Fusiones en HD (estudio Haiku): no son formas ni una tabla; la fusión es una técnica especial que cambia de
  personaje en combate. Plan Yamhan: técnica de Yamcha/Ten que hace el relevo a id 54 con el mecanismo del Tag Team.

## Pendiente (siguientes bloques)
- Yamhan como fusión: en HD las fusiones (Gogeta 167/171, Gotenks 150/158, Vegito 187/192) no son formas del
  luchador ni se encontró la tabla (A,C,B,F) con u32; falta localizar su lector (probable u16 o tabla por ID 64-104).
- Tiras doradas nativas, estilo de tiendas como la Skill Shop, Leyenda de Satán con cinemáticas, B2, IW, Héroe beta.

# Borrador para debatir

Origen: feedback del usuario tras probar la 1.4.6 (ver `FEEDBACK_USUARIO_1.4.6.md`). Regla nueva: los modos de otros
juegos se **portean** (datos y reglas del original), no se imitan. Nada empezado; la 1.4.6 sigue sin publicar.
Estudio previo: 6 agentes Haiku de solo lectura (resultados resumidos abajo).

## Bloque 1 (90 min): lo que el usuario vio roto + bases
| Tarea | Causa localizada | Arreglo |
|---|---|---|
| Laboratorio solo deja a Goku | `src/modos.cpp` `DibujarLab` (~1567) no tiene selector; usa `UltimoDuelo()` (~1727) y cae a id 0 | Fila "Personaje" y "Rival" con el selector de `src/rpg.cpp` (`Siguiente`, ~825) |
| Al ganar sale "Volver a elegir personajes" | El salto del menú final (`sub_820FF750`, modos.cpp ~1945) exige `g_st.fase == kCombate`, que solo pone `Lanzar`; la pasarela (`pasarela.cpp` ~399-520, `pasarela_misiones.cpp` ~579) y el Lab no lo ponen | Marcar todo combate lanzado desde el HUB (en `hub::Launch`) y quitar/rellenar las filas que no proceden |
| Imágenes que no llegan a los bordes | `src/ingame/game_style.cpp` `GameArea` (~64): caja 4:3 centrada (1200x900); marcos, fondos y banners se cortan en 16:9 | Fondos/marcos/banners anclados a la ventana; el contenido sigue en la caja |
| Mensajes in-game pobres | Textos escritos a mano por modo | Repaso de todos los textos de los modos (cortos, claros, 6 idiomas) |
| Yamhan como fusión de Yamcha/Tenshinhan | Hoy es personaje aparte (ids 54/55). La tabla nativa de fusiones (filas A,C,B,F; PS2 0x2E9430) no está localizada en HD | Buscar la tabla en el xex HD por su contenido conocido; si sale, cápsula "Fusión" para Yamcha y Ten → Yamhan |

En paralelo (Haiku, solo lectura): preparar los bloques 2-4.

## Bloque 2: Leyenda de Mr. Satán fiel al original
- Hoy: los 11 combates SÍ salen de la ISO de B1 del jugador (`pasarela_b1.cpp` `TablaSatan`), pero multiplicadores,
  reglas y puntos por golpe/combo/lanzamiento son nuestros (`pasarela.cpp` `ReglasDe` ~674, `PuntosB1` ~666).
- Plan: usar el port de B1 HD (`PROYECTOS IA\DBZ Budokai HD Collection`, mismo procesador que B3 HD) como laboratorio:
  jugar la Leyenda allí con vigilancia de memoria y medir reglas y puntos exactos; luego leerlos igual que el original.
  Imágenes y voces del modo desde la copia HD del jugador (la HD Collection trae B1), no desde la PS2.
- Falta comprobar que el modo está en B1 HD y descomprimir sus AFS (`tools/xbdecompress.exe`).

## Bloque 3: misiones de Infinite World reales
- Hoy los "retos de IW" son 2 combates con condiciones escritas a mano (`pasarela_misiones.cpp` ~166-200).
- Paso 1: decodificar las tablas de misiones `#DMD` (DATA_CMN entradas 1092: 106 filas, 1093: 149) y leer de la ISO
  rival, condición, tiempo y texto de cada misión → todas las misiones de combate del original.
- Paso 2 (aparte, semanas): minijuegos que no son combate. Hay pruebas de **carreras de vuelo** (`# FLIGHT #`, CHECK/GOAL)
  y **disparo** (`# SHOOT #`). B3 no tiene vuelo libre: hay que construirlo. La decomp de Ghidra es de la versión PAL
  (SLES_553.47); nuestras direcciones son de la USA.

## Bloque 4: tablero de Budokai 2 real y Nave de Babidi
- Localizados los 9 tableros originales (`#YDW`, DATA_CMN 387-395) y los módulos `dw_*`; sin decodificar su cargador.
- Fase 0: dibujar casillas reales sobre una captura de B2; fase 1: jugar con casillas y rivales reales; fase 2-3:
  cartas, turnos, Shenlong. Nave de Babidi: sin datos (solo la palabra "BABIDI" en el ejecutable) → investigar primero.

## Modo Héroe (crear personaje + runs)
- Hoy: Goku con 3 trajes, 2 técnicas y 3 combates (`src/rpg.cpp`).
- v1: raza = cuerpo de un personaje (saiyan, humano, namekiano, raza de Freezer, Bu, androide), cara/pelo de una lista
  ya hecha, colores por parte + aura/ki (texturas recoloreadas al crear; el cargador de texturas HD ya cambia texturas
  en marcha), maestro (estilo y técnicas como en Ultimate Tenkaichi), y bucle de runs con lo que ya existe (tienda,
  cartas, torres). Fuentes: B3, IW (56), SDBH (391).
- v2: mezclar piezas sueltas (pelo de uno, cuerpo de otro): exige montar modelos por piezas, trabajo nuevo.

## Otros
- Supervivencia: el proyecto de Shin Budokai 2 no la tiene decodificada; solo sirve de referencia de reglas y textos.
- Interfaz/gráficos: repaso con piezas nativas de B3 (ya se extraen en marcha, `native_ui.cpp`), transiciones y cajas de
  diálogo como las del juego. Pedir al usuario 2-3 ejemplos concretos de gráficos pobres.
- Tag Team: al usuario le gusta el rework; no tocar.
