# Campañas de Dragon Universe (DU)

Estado: 2026-10-11 (noche). Medido en el juego (US) y en el código recompilado. Lo marcado [?] está sin medir.

## Piezas del juego

| Pieza | Dónde | Qué es |
|---|---|---|
| Registro de escenas | `sub_82156F30` (EU `sub_82156EF0`) | Traduce un *código de historia* a un contenedor `#LST` de `data_cmn`. La tabla se rellena en la pila de la función; `scratchpad/du_registry.py` la extrae (`mod center hd/du_codigos.json`). |
| Rangos de códigos | misma función | 1-99, 100-106, 2000-2399 (4 bloques), 2400+, 2800+, 3000+, **5000-6199 (12 bloques de 38 escenas)**, 8192+, 16384+. |
| Bloques por historia | 5000, 5100 … 6000, 6100 | 12 bloques de 38 códigos. El 6100 (contenedores 1569-1606) comparte tramo con 8192+. [?] Si es la historia recortada o escenas de la saga de Buu: el handback del 10-10 dice Buu; sin medir. |
| Rueda de DU | `sub_8217D2D0` modo 1 | Máscara u64 de casillas fija en el código (no hay tabla): `0x4_00023C9D` = casillas 0, 2, 3, 4, 7, 10, 11, 12, 13, 17 y 34 (Broly). Una casilla nueva = un bit libre (1, 5, 6, 8, 9, 14-16, 18-33, 35-63). Un segundo bucle de 44 IDs usa la tabla ID -> casilla `0x82020668`. |
| Personaje de DU elegido | `0x824A58A0` (u8) | Casilla del personaje (sub_8215E9C8). Su bloque de partida: `0x8247A9B0 + 0x80 + 4624*k` (empieza en +0x80); caben k = 0..37 antes de ese byte [?] (otros campos de la partida sin comprobar). |
| Inicio de historia | medido | Goku, "Nueva historia": carga el contenedor 1727 (código 100). |
| Avance de la historia | `0x8234DD28` (palabra pendiente: código << 16 \| sub) | La escriben los eventos de las escenas: `sub_8215EC68` pide el código 8192+N (parámetros del evento) y `sub_82156B38` el 3000 (vuelta al mapa); `sub_8215EAB0` la consume. Una historia nueva es una cadena de escenas cuyos eventos piden las siguientes. |
| Registro | log del juego | Cada escena cargada deja `dbz3 campanas: codigo N -> contenedor M (desde LR)`: así se ve qué código pide cada momento de una historia. |
| Textos | `#AZT` en `data_usi` y por idioma (`data_eng`, `data_spn`…) | Cada línea de diálogo o narración es **una imagen** (no texto). Las escenas las piden con el evento 10 (`DATA_USI#n`). En español la imagen es más ancha (p. ej. 192 frente a 160 de ancho lógico). |

## Lo que ya permite el port

- **`campana.toml` en un mod** (runtime, `src/campaigns.cpp`): cualquier código de historia apunta a otro contenedor, sin tocar los demás.

  ```toml
  [codigos]
  5500 = 3990
  5501 = 3991
  ```

- **Textos nuevos**: imágenes generadas con el mismo estilo (texto oscuro en los bocadillos, blanco con borde verde en la narración) dentro de un `#AZT` por idioma (`roster_build.azt_append`, sin mover datos).

## Lo que falta para un personaje de DU nuevo

1. Añadir su casilla a la máscara de la rueda (gancho tras `sub_8217D2D0`).
2. Darle una casilla libre (el bit 12 NO está libre: es de la máscara) y su bloque de partida de 4624 bytes; comprobar que no pisa otros datos.
2b. Medir el código con el que empieza cada historia (log `dbz3 campanas`) y el enlace casilla -> bloque de códigos (no es 5000 + 100*casilla: Broly daría 8400).
3. Decir qué luchador usa esa casilla en los combates y en el mapa. La nave de Freezer es la forma 5 de Freezer (ID 27, cápsula 405), `FREEZA_SPACE_SHIP` Pal 508 (HD 514 [?]).

## Prueba prevista: la campaña recortada de Freezer

- Bloque 6100 (contenedores 1569-1606) [?] y BPL sin usar 654-667, 582, 730 y 799 [?] (no aparecen en AUDITORIA_DATA_CMN.md).
- Camino mínimo que ya funciona: una campaña que sustituye la historia de un personaje existente (`campanas/ejemplo_freezer.toml` cambia la de Goku: 5500 -> 2138 y el rival 27, Freezer).
- Nave: `FREEZA_SPACE_SHIP` (Pal 508, HD probable 514).


## Cómo encadena el juego una historia (medido la noche del 2026-10-11)

- **Códigos por personaje**: casilla de DU `k` (= casilla del select: Goku 0, Vegeta 7, Uub 17, **Freezer 22**, Broly 34). Intro de saga `100/102/104/106` + entrada `k` (`sub_82156DC8` da la de "Nueva historia" por casilla; Freezer 102 = Namek). Escenas del mapa de la saga `5000 + 100*saga + k` (Goku 5000, Uub 5317, Freezer 5122) y combates `5400 + 100*saga + k` (Freezer 5522).
- **Palabra de escena** = `código << 16 | entrada`. `0x8234DD1C` = siguiente escena pendiente (la lanza `sub_8215EB10`), `0x8234DD28` = palabra de evento, guardada del personaje `+4564/+4568`.
- **Eventos de una entrada #LST** (12 bytes salvo los de mapa): 10 recurso, 11/12 imagen, 13/14 voces/BGM, **15** guion tras el combate, **16** combate (#BPC), **19** premio (cápsula), **20** escena siguiente (al ganar, o al acabar), **21** diálogo en el escenario al empezar el combate (una entrada de diálogo SIN op 16), **52** escena si pierde, **47** planeta del mapa (0 Tierra, 1 Namek), **50** pantalla de guardar, **22** borra todas las marcas del mapa, **23** borra la marca de una palabra, **24** marca del mapa: `(24, n, palabra, icono, zona, x, y, z, escala)` = 9 u32, **29** objeto del mapa por nombre `(29, 0, "NOMBRE" 16 B, estado)` = 7 u32, **35** = 4 u32, **36** = 5 u32, **44** salida en el mapa `(44, 0, x, z, y)` = 5 u32, **48** = 10 u32, 54 = 3 u32. La cuenta de la tabla es de eventos, no de bytes: un evento de mapa mal medido hace que no se ejecuten los siguientes.
- **Cabecera**: el último u32 es el **guion de la escena**, no un id único (narración 0xADB, combate del modo historia 2866). En las narraciones el tercero es páginas + 1.
- **Forma de una saga en el juego** (Goku): la introducción (`100/102/104/106`.k, panel) deja en el mapa la marca del primer combate. Cada combate (`54xx`, entrada 2i) lleva `16, 15, 21` (diálogo de antes) y `20` a su escena de victoria (2i+1): un diálogo en el escenario que además deja la marca del combate siguiente (`44`, `24`). Entre combates no hay paneles. El último combate de la saga va a su cierre (`101/103/105`.k: panel, `50` guardar, `47` planeta y `20` a la introducción de la saga siguiente); el último de la historia va directo al final (`1.n`).
- Las marcas del mapa viven en la partida (bloque del personaje + 128, 64 x 64 B). Tocar una la lanza (`sub_821C4648` -> `sub_8215E9C8`).
- Un combate lanzado desde una narración (sin pasar por el mapa) cuelga el juego (`sub_8209F568`, objeto ya liberado): los combates van siempre en marcas del mapa, como en el juego.

## La historia de Freezer (mod `historia_freezer`, `mod center hd/campanas/historia_freezer.toml`)

Estaba a medias en los datos: casilla 22 sin rueda, la intro de Namek desde su punto de vista (textos 661, panel 1411 con una etiqueta de pruebas en japonés), escenas de relleno (5122) y combates sin usar. El mod (`python campana.py montar campanas/historia_freezer.toml`) la monta como las del juego: dos sagas (Namek, 10 combates; la Tierra, 2), cada combate con su charla antes y después, cierre de saga con "¿guardar?" y final propio. En el mapa Freezer vuela con su modelo normal (la nave está pendiente, ver abajo; el gancho que lo intentaba ponía Mecha Freezer y se quitó).

Lo que hizo falta para que no se cuelgue (medido 2026-10-11):
- **Retratos sin terminar**: 1464, 1465 y 1486 (Freezer 2.ª y 3.ª forma, Mecha) son de media resolución y cuelgan el diálogo del combate (`sub_820AAD30` con recurso 0x20). Se usan 1463 y 1466.
- **#BPC viejos**: los combates sin usar (627-667) son de un formato anterior (1216 B; el juego lee 1248): escenario equivocado. Se rehacen sobre uno del juego (`plantilla`: 709 Namek, 717 la Tierra) con su rival (+0x10) y `forma_rival` (+0x3c). Escenario en +0x90.
- **Escena de victoria** como 2138.1: cabecera de 28 B con el guion op 15 + 1 y las imágenes en orden fondo (1536 Namek, 1549 la Tierra), jugador, rival.
- **Vuelta al mapa** por una escena del mapa (una frase del personaje, guion 2899), como 2138.1 -> 5100.5.
- Tras ganar sale la pantalla de subir de nivel (puntos a repartir) antes de la charla.
- Seguro: `sub_820AAD30` ignora un recurso imposible (< 0x10000) en vez de cerrar el juego.

**Nave de Freezer (space pod) para el mapa: dónde está y qué falta**
- Modelo: `FREEZA_SPACE_SHIP.amb`, data_cmn **514** en HD (508 en la lista de PS2; `out/analysis/data_cmn_pal_afl.txt:509`). Va con los efectos de combate (junto a `COOLER_SPACE_SHIP`, 505/511), no con los modelos de personaje.
- Cápsula 405 "Frieza's Space Ship" (equipo). En la tabla de formas de Freezer (char372 de ID 27) la forma 5 es tipo 07 con esa cápsula, y su modelo (char96 formas[5], modelo 5) es **Mecha Freezer**, no la nave.
- El mapa carga el personaje con `sub_82133B88(ID, modelo, traje)` desde 0x821C2530 (`sub_821C6BB8`); ahí solo hay modelos de personaje. Llevar la nave al mapa requiere dibujar ese modelo de efectos como objeto del mapa. **Sin hacer.**
- Estudio 2026-10-11: no basta con datos (514 no tiene huesos; los modelos de mapa son personajes completos y ningún personaje del juego vuela en vehículo). Vía posible: el objeto del mapa `FSS` (evento 29, ya visible en la intro de Namek) que siga al jugador en `sub_821C4DA0`, ocultando el modelo de Freezer. Personaje en el mapa: global 0x82479A60, puntero +180, campos +4577 ID / +4578 modelo / +4579 traje (`sub_821C1A00`).

Probado con `DBZ3_TEST_KO=1` (el rival cae a los 10 s; tu vida no baja): 9 combates seguidos en Namek sin cuelgues.

Herramientas de prueba en `src/campaigns.cpp`: el registro `dbz3 campanas: ...` de cada escena y `DBZ3_DU_TRACE=1`.

## Opciones nuevas (2026-10-11)

Ejemplos comentados en `mod center hd/campanas/plantilla_historia.toml`. Valores por defecto y código en `campana.py` (`STYLES`, `story()`).

- `[historia] idioma = "en"`: ese idioma para todos los idiomas del juego. Sin él, cada idioma usa su texto y, si falta, el inglés.
- `musica_narracion` (por defecto 135269, la música de las introducciones nativas): música de los paneles de narración.
- `imagenes_desde` (por defecto 4510): primera entrada libre de `data_cmn` para las imágenes propias. Cada imagen nueva usa la siguiente.
- `imagen` / `cierre_imagen` (en un capítulo con `saga = true`): imagen .png/.webp/.jpg/.avif en lugar del panel verde de la intro o del cierre. `png2pic.py` la convierte a 660x420 (recorte centrado).
- `[[imagen]] entrada = N, archivo = "..."`: imagen en la entrada N de `data_cmn` para una `[[escena]]` hecha a mano (la intro del primer personaje usa el evento `[10, 0, N]`).
- `[historia.estilos.dialogo | narracion | final]` con `fuente`, `tamano`, `interlineado`, `ancho`, `borde`, `color`, `color_borde`. Son tres estilos separados; narración y final salen como los nativos (verde oscuro con borde claro).
- `[historia.final] fondo = "heroe" | "villano"`: fondo del final, estilo Goku (1728.0, azul) o Broly (1729.20).
- Editor de historias (`editor_historias.py`, pestaña «Historias» del Mod Kit): selector de idioma; botón «Rellenar traducciones que faltan» (copia el texto del otro idioma y lo anota en `traducir_pendiente`); pestaña «5. Estilos de texto» con vista previa en vivo; selector de imagen para la narración; vista previa de la colocación de los retratos.

Nota técnica (corregida hoy): la marca del mapa (evento 24) se escribe siempre con 9 palabras: el número de evento, la casilla, la palabra de la escena y los 6 valores de `marca` (icono, zona, x, y, z, escala). Con menos, los eventos siguientes de la escena se leen mal.

## Cambios 2026-10-12
- `modelo_mapa` (por capítulo): modelo con el que se vuela por el mapa mientras la marca de ese combate espera. Gancho en `sub_821C24B8` (`src/campaigns.cpp`): pone el modelo en +4578 del registro del mapa solo al crear el personaje y lo devuelve a su valor, así la partida guardada no cambia. Freezer 5 = Mecha (los capítulos de la Tierra de `historia_freezer.toml`). Solo US. Sin probar en el juego.
- Trajes con armadura de Namek: Vegeta 3, Krilin 2, Gohan niño 1 (orden del catálogo; el de Krilin y Gohan sin probar en el juego).
- Escenario: el byte de escenario de un #BPC es big-endian en +0x90. 709-714 = 7 (Namek), 715/716 = 14 (Namek a punto de explotar, los de Goku SSJ contra Freezer), 717 = 8 (la Tierra). El combate contra Goku SSJ usa `plantilla = 715` y `forma_jugador = 4` (Freezer al 100%).
- La narración admite una sola imagen por escena: el cierre de Namek usa ahora a Freezer en su forma final. Cambiar de imagen en mitad de una narración requiere partirla en dos escenas (pendiente).
- **Nave de Freezer en el mapa:** Freezer (ID 27) tiene 3 trajes de 6 modelos: 181-186, 187-192 y 193 + 182-186. El traje 2 con modelo 0 es el bin 193 `FREEZA_MINI_SHIP_01` («Forma 1 con nave»), un personaje completo con la nave en la malla. La opción `traje_mapa` (por capítulo o en `[historia]`) pone +4579 en el gancho de `sub_821C24B8`, igual que `modelo_mapa` pone +4578. En `historia_freezer.toml`, Namek usa `traje_mapa = 2` y la Tierra usa `modelo_mapa = 5` con `traje_mapa = 0`. **Probado en el juego el 2026-10-12: Freezer vuela en su nave por Namek.** El personaje del mapa se crea mientras está pendiente la introducción de la saga (palabra guardada 102.22, 104.22...), no un combate. Por eso campana.py también apunta el modelo y el traje del primer capítulo de cada saga a la palabra `(100 + 2*saga) << 16 | casilla`. El gancho busca por orden: la palabra guardada del personaje (+4564/+4568 del bloque de su casilla), el último combate lanzado y las marcas. Si no encuentra la palabra exacta, vale cualquier combate del mismo código, es decir, de la misma saga. Cada creación deja una línea en el registro (`personaje del mapa (casilla ..., guardada ..., ultimo ...)`).
- Altura al empezar: ver «Llegada al mapa» más abajo (la causa era la falta de escena del mapa tras la introducción, no el evento 44).
- `campanas/historia_freezer_prueba_tierra.toml`: un solo combate en Namek y después la Tierra (para probar Mecha Freezer). Se monta sobre el mismo mod; para volver a la completa: `python campana.py montar campanas/historia_freezer.toml`.
- **Locución de la narración:** el evento es `(14, página, 0x20000 | N)`, con N la entrada de adx_usa/adx_jpn. La intro de Namek usa 1231-1233 y otras narraciones del juego, 1198-1202. Las claves `voz` y `cierre_voz` (una por página) aceptan un número de voz del juego, "2.N" o un fichero propio (.wav/.mp3/.ogg/.flac). El fichero se convierte a ADX y se añade como entrada nueva (`voces.VoiceWriter`). Esto requiere ffmpeg.
- **Llegada al mapa (arreglo de la altura):** las introducciones del juego no van directas al mapa: acaban en `(20, 0, 5100.0)` (102.0) o `(20, 0, 5200.4)` (104.0), una escena del mapa con una frase del personaje. Sin ella el personaje salía volando a ras del suelo al empezar y al llegar a la Tierra (tras cada combate salía bien porque ya pasaba por una escena del mapa). `campana.py` crea ahora la entrada 0 de las escenas del mapa de cada saga como su llegada (frase `llegada_es` / `llegada_en` del primer capítulo de la saga) y las introducciones de saga acaban en ella; la de la primera saga (`[[escena]]` a mano) debe acabar en `[20, 0, "<codigo>.0"]`. El 5.º valor del evento 44 vale 100 en las del juego (no es la altura).
- **Mecha Freezer en la Tierra:** al cambiar de planeta el juego no rehace al personaje del mapa (sigue la nave hasta volver del primer combate de la Tierra). Rehacerlo a mano al llegar (`sub_821C24B8` desde el estado de volar) cerró el juego: no se hace. **Probado por el usuario 2026-10-12: tras el combate contra Trunks, Mecha Freezer vuela de pie.**
- **Vuelo de otra casilla sin cambiar la casilla:** `animacion_mapa` cambia, solo mientras dura la creación, dos entradas de la casilla del personaje: el vuelo (0x823742C8 + (casilla·5 + k)·4) y las animaciones del mapa (0x823745C0[casilla]; con solo el vuelo Mecha seguía sentado). La casilla (+4577) no se toca (con ella cambiaban los datos por casilla y modelo de `sub_821C0BE0`).
- **R1 en la Tierra:** el modo de búsqueda pedía un efecto que no está en el banco (id fuera de la tabla) y el juego leía memoria sin mapear en `sub_82096FF0` (0x06A459B2). El gancho de `sub_82096FF0` deja sin efecto un id fuera de la tabla. **Probado por el usuario: R1 funciona.** La saga de la Tierra copia además los objetos del mapa de 104.0 (`objetos_mapa_de = "104.0"` en el capítulo que abre saga) y la marca de Trunks usa la zona del juego (25).
- **Cápsula 405 gris:** es su color en el juego. El byte +10 del registro #SKC (nibble bajo) da la categoría: 0 habilidad (rojo), 1 equipo (gris), 2 objeto (verde), 3 sistema. 405 es equipo (0x31); la Senzu es objeto (0x32). Ponerla verde la convertiría en un objeto de usar: se deja como en el juego.
- **«I found something!»:** escena 2100 (contenedor 1980), compartida por las historias. `[historia] eventos_mapa = { codigos = [2100], retrato = 1463, desde = 4600 }` copia el contenedor con la cara del protagonista a una entrada nueva. Se usa con `[codigos_historia]` y solo mientras se juega una casilla de `[rueda]`. Esa escena regala además la cápsula 405 (la nave), que sale gris en vez de verde. El gris viene del dato del juego: el mod no toca esa cápsula. Pendiente de estudiar.
