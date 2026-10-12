# Estudio 02: modo Tag Team (2 contra 2 con cambio de personaje)

Solo lectura. No se ha editado ningún fichero del proyecto.

## Resumen

- **Viable en fases.** El cambio de forma ya cambia modelo y moveset en mitad del combate, y los datos de forma pueden apuntar al modelo de **otro ID**. Eso es la mitad del cambio de personaje. La otra mitad (HP, ki y estado por miembro del equipo) no está en los docs y hay que medirla.
- **No hay ningún dúo en los datos.** El "Father-Son Kamehameha" de B3 es un ataque **solo de Gohan joven**, no de Goku + Gohan. No sirve como ultimate de equipo sin una cinemática nueva.
- **Un 3er luchador en pantalla no es realista.** Lo más viable es un cambio de luchador en la misma casilla (2 luchadores vivos a la vez por lado, como el sistema actual).
- **MVP recomendado:** botón de cambio que intercambia al compañero con su propia barra de vida y sin cambio a mitad de combo. Esfuerzo estimado: 3 a 5 sesiones de trabajo, con riesgo alto en el estado de cada luchador.

## 1. Cómo cambia de forma el personaje en combate

Lo que ya existe (todo en datos y en el código recompilado; `src/` solo carga datos, no ejecuta el cambio):

- Registro de combate por forma: `char372` (0x82329CF0), entrada de 20 B en `+212 + 20·f`. Campos relevantes (docs/07_ports/GOHAN_FUTURO_EXPLORACION_2026-10-06/04_transformaciones/informe.md:43-54):
  - `+0` cápsula que exige la forma.
  - `+2` **ID cuyo `char96` aporta el modelo** (sub_820F9970).
  - `+4` índice en `char96.formas[]` de ese ID.
  - `+6` tipo de transformación (reversible, irreversible, última, especial).
  - `+9` nivel base de ki en barras.
- Ya hay un caso de **modelo de otro ID**: `src/roster_ext.cpp:298-310` dice que las formas de otro ID (p. ej. Goku SSJ4 = ID 91) reciben el modelo en ese ID. Es decir, el motor ya sabe cargar el modelo de un personaje distinto dentro de una forma.
- Subir de forma: sub_82119108 (informe 04, §1.5). Comprueba la cápsula equipada y **ki ≥ N barras sin gastarlo**.
- Bajar de forma: sub_82119258. Con ki < 1 barra (1000), un golpe devuelve a la forma normal.
- Límite: `docs/03_formatos/FORMAS_Y_KI.md` dice que el runtime solo puede **reducir** las formas del donante (`src/roster_ext.cpp:600-603`, `nforms` en `char372+0xD0`, líneas 338 y 583).

Lo que falta para un cambio a **otro ID** (no encontrado en docs):
- Que el mismo camino de "cambiar modelo + moveset" acepte un ID distinto del propio. Hoy el `+2` apunta a otro ID solo para el modelo; el moveset (`anm` `+0x00..+0x1C`, `char372`) y la voz (`0x823280B0`) siguen en el ID propio. Habría que comprobar si el moveset se toma del ID de `+2` o del propio.
- Hay que confirmar la lectura de la forma en tiempo real (no solo al cargar el combate). Ver punto 2.

## 2. Cómo se dibujan dos personajes y límites de carga

- Hay al menos dos luchadores en memoria: `docs/07_ports/SESION_GPU_DRAW_2026-09-11.md:71-72` identifica dos buffers de modelo "Cell y Goku" a la vez. La selección también usa un bloque por jugador (`src/select_ext.cpp:1-20`, campo `+36`).
- Los luchadores se cargan al empezar el combate. El AFS virtual sirve overrides en memoria (`AGENTS.md:543-546`), por lo que los modelos de otro personaje se pueden leer en mitad del combate sin archivos nuevos. **No hay medida** de cuánto tarda la lectura ni de cuánta memoria del Xbox 360 se usa para un tercer luchador. Eso hay que medir.
- Voces: 50 índices ADX por ID (`0x823280B0`, `0x82328298`, `src/roster_ext.cpp:10-21`). Cambiar de personaje cambia la voz si se lee el ID del luchador activo.
- Cara de la barra de vida: `0x82373D68`, un par de bytes por ID (`src/roster_ext.cpp:10`). Hay una cara por ID; con dos luchadores por lado harían falta dos caras por barra.

## 3. Asistencia con un 3er luchador

- **Probablemente no.** Todo el código de combate va por luchador (`fighter+...`, struct de cada luchador, `docs/07_ports/GOHAN_FUTURO_EXPLORACION_2026-10-06/04_transformaciones/informe.md:72-76`) y el beam struggle (`02_tecnicas/informe.md:281`) asume dos luchadores enfrentados. Un tercero que aparezca un momento y se vaya (asistencia) obligaría a tocar la lógica de colisiones y de cámara. Esto es inferencia a partir del código documentado, no una prueba.
- **Alternativa barata:** usar el propio cambio de forma como asistencia. El compañero entra en la casilla, ejecuta un ataque de entrada (ataque de un ID distinto, cápsula propia) y vuelve. Esto es "switch con ataque al entrar", no un 3er personaje.

## 4. Ultimate de equipo

- **No hay ningún dúo en los datos de B3, SB ni Another Road.** `docs/07_ports/GOHAN_FUTURO_EXPLORACION_2026-10-06/04_transformaciones/b3_caps.json` (entrada "22", línea ~1310) y `skills_ocr.json` (línea ~157) lo confirman: "Father-Son Kamehameha" tiene propietario **Teen Gohan**, es Ultimate, requiere **P+K+G+E en Hyper Mode**, cuesta 5 barras y solo está disponible en SSJ2. Las animaciones son del propio Gohan joven (ranura SPX 0, anims 0x4A0–0x4B6; `02_tecnicas/informe.md:92`).
- **Reuso posible del camino Ultimate/SPX:** el de definitivos (`docs/02_mods/STUDIO_CAMARAS.md`: ranura 0 = modo hiper, 20 = agarre). Una cinemática de dúo necesitaría un guion SPX nuevo con dos actores, y eso no lo he visto hecho en ningún sitio.
- **Coste:** cámara y guion de dos actores desde cero. Es lo más caro del proyecto, así que no entra en el MVP.

## 5. UI: entrada en "Dueling" y selección de equipo

- La entrada de menú no aparece en el código de `src/` ni en `i18n.cpp` (busqué "Duel" y no encontré nada). Hay que mirar la tabla del menú en el guest antes de diseñarla.
- Selección de equipo: reutilizar la rueda de `src/select_ext.cpp` (celdas nuevas `celda = true`, dos slots por jugador). Esto ya funciona para personajes nuevos.
- HUD: hacen falta dos barras de vida y dos caras por lado. Ya existe `hud_cara` por ID (`roster_ext.cpp:60`). La parte de barras no está documentada; hay que localizar el dibujo de la barra.
- Un punto que puede romper algo: `docs/03_formatos/CAPSULAS_B3.md:72-73` dice que en Versus y Práctica no se llama a `sub_820FE3D8`. Un modo de equipo nuevo tendría que replicar la limpieza de listas que hace esa función.

## 6. Qué estado hay que guardar por miembro

Según lo visto, por cada luchador:
- HP propio (no encontré el campo; hay que localizarlo en el registro de luchador).
- Ki: `ki_base` y ki actual. Lo lleva el luchador, así que habría que decidir si el ki es compartido por equipo o por miembro.
- Forma actual (tope de forma en `fighter+7412`, +24; cápsulas equipadas en `fighter+8156+k`, según `04_transformaciones/informe.md:72-76`), cápsulas equipadas, combo activo.
- Qué miembro está en la casilla, y si el compañero está vivo.

## 7. Fases propuestas

| Fase | Qué hace | Esfuerzo | Riesgo |
|---|---|---|---|
| F0 Medir | Confirmar en el juego: cómo se lee `+2` de la forma en tiempo real, memoria libre con 3 modelos, dónde está el HP del luchador | 1 sesión | Bajo |
| F1 MVP | Botón de cambio: intercambia el luchador activo por su compañero, con HP y ki propios y sin cambio a mitad de combo | 3 a 5 sesiones | Alto (estado por miembro) |
| F2 Cambio con combo | Cambiar en mitad de combo y ataque de entrada | 2 a 3 sesiones | Alto (animación y colisiones) |
| F3 Asistencia | Compañero que entra, ataca y sale | 2 a 4 sesiones | Medio |
| F4 Ultimate de equipo | Cinemática SPX con dos actores y cámara | 4 a 8 sesiones | Muy alto |

## Riesgos

- Estado mal separado entre miembros (HP o ki compartido por error). Puede dejar a un luchador muerto con barra viva.
- Memoria: no medida para tres modelos a la vez en el Xbox 360 emulado.
- Cambio de moveset en mitad de combo: el combo puede quedar en una animación del otro personaje. La transformación actual sí se probó sin cierre al cambiar de modelo (`04_transformaciones/informe.md:275`), pero solo en la misma familia de personajes; un cambio entre personajes distintos no está probado.
- Voces y cara de barra por personaje: si no se cambian, el HUD muestra a un luchador y suena otro.

## Lo que no he podido verificar

- Dónde está el HP del luchador y si el ki es por luchador o por equipo.
- Nombre y ID del menú "Dueling" en la tabla del guest.
- Si el moveset de una forma con `+2` de otro ID usa el moveset de ese ID o del propio.
- Coste de memoria y tiempo de carga de un modelo extra en combate.
