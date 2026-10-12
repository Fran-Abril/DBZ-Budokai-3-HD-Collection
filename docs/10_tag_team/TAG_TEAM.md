# Tag Team

Estado 2026-10-11 (noche): equipos nativos (los personajes del juego tal cual, sin mods de equipo ni personajes nuevos) con relevo real de personaje, probado en Práctica y en Duelo contra el COM. Hoy: KO con compañero vivo arreglado (el caído aguanta con vida 1 y el relevo espera a que aterrice), relevo con deslizamiento en vez de destellos, y Tag Team online probado entre dos copias en un mismo PC.

**Problema abierto (feedback del usuario):** el relevo todavía no se ve bien. El primer rival sigue "inmortal", sin animación de caída antes del relevo, y el que entra aparece casi de golpe. Ver `16_ko_select_2026-10-11.md`, sección "Actualización (noche)".

## Dentro del juego
- **Fila TAG TEAM** bajo Dragon Arena en el menú principal (o X) abre la pantalla **TAG TEAM** (estilo del menú de Duelo):
  - filas **1P VS COM / 1P VS 2P / COM VS COM**;
  - formato con izquierda/derecha: **2 VS 1, 1 VS 2, 2 VS 2, 3 VS 3**;
  - el A que la abre no cuenta: se espera a soltar todos los botones (antes elegía 1P VS COM solo).
- Al elegir, el piloto de menús lleva al juego a esa opción del Duelo y a su **rueda de personajes nativa**:
  - cada **A** en la rueda añade el personaje a la **tira de su lado** (casillas numeradas, como Tenkaichi 3); sale el **menú de cápsulas del juego** (Normal / Custom) y, al elegir, el piloto vuelve a la rueda a por el siguiente. Cada miembro lleva sus cápsulas (`select_ext.cpp`, gancho de `sub_8217CB10`: estado +2 resultado, +13 opción, +4 casilla, +120 copia con las listas Custom en +68 + 14·casilla; `SetMemberCapsules` / `PutCapsules`). Atrás en ese menú quita al miembro. Online siempre las Normal (los dos PCs deben ver lo mismo);
  - **Y** en la rueda quita el último miembro (sin miembros, Y es el "atrás" del juego);
  - el último miembro del formato es la confirmación normal del juego (sigue a la elección del COM o del escenario);
  - en 1P VS COM y COM VS COM el mando 1 elige los dos equipos (como Tenkaichi 3); en 1P VS 2P cada uno el suyo.
- Al empezar el combate cada lado entra con su primer miembro (`tagteam_slots.cpp` pone su ID en la selección).

## En combate (`src/tagteam.cpp`)
- **SELECT** = relevo al siguiente miembro vivo; **SELECT + L** (LB o gatillo) = al tercero (3 VS 3).
- **Reglas de SELECT**: acepta con pelea en marcha, luchador propio libre (de pie, guardia o andando), sin relevo en curso y 90 ticks desde el último relevo. Buffer de 30 ticks. Si rechaza, la ficha del compañero parpadea en gris.
- **KO con compañeros vivos**: el golpe letal deja al caído con vida 1 (no termina el asalto). Al caer se fuerza la acción 25 (salir despedido). El relevo se lanza al aterrizar y 90 ticks después de la caída.
- **Relevo nativo** (estudios 11-15):
  1. el que entra se pide con el cargador del juego (`sub_82109148`) en el hueco de forma 7, con un registro de 372 B provisional cuya forma 7 es su forma normal; su cámara (golpes) e idioma quedan aparte. El que está en pantalla no se toca;
  2. con todo cargado (nodo de `sub_82083968`: byte +1 a 0; modelo +36 a 0; ~16 ticks), en un mismo tick: `sub_821092C8` libera al que sale, el modelo/animaciones del nuevo pasan al hueco 0, ID/registro/selección/cápsulas nuevos, `sub_82113818/82113870/82112260` (cápsulas por luchador), `sub_8210FDE0` (golpes y cámara), `sub_82109638` (reinicio), forma 0 (`sub_8210E690` + `sub_82115270`) y en guardia (X+6048 = 0, acción 0).
  3. **Salto de relevo** (2026-10-12, como Tenkaichi 3): el que sale salta hacia atrás y arriba acelerando (15 unidades atrás, 60 arriba, 12 ticks) y el que entra baja desde ahí frenando y aterriza en su sitio (18 ticks). En un KO baja desde encima del caído. La posición está en tres copias (X+48/64/80; altura en +4). `DBZ3_TAG_SLOW=N` lo alarga N veces para verlo en capturas.
  4. **Efectos de técnica** (2026-10-12): la ranura A procesada del que sale (544 B en `G2+1152k+80`) se **guarda** y no se suelta hasta el final del combate (`sub_82126A10`, gancho que suelta las guardadas). Soltarla dejaba efectos vivos leyendo memoria liberada (cierre en el choque de poderes, `sub_82150C00`). El que vuelve recupera la suya sin cargarla otra vez. Después se reenlazan S1/S2 con `sub_82125808` / `sub_8212AEC0` con r4 = -1, como al empezar (`sub_82125888`).
  5. **Espera de efectos**: mientras el juego usa un banco de efectos de la ranura del lado (gancho de `sub_82092C58`), el relevo no termina (8 ticks sin uso, como mucho 4 s): los efectos de un rayo en pantalla buscan sus datos en la ranura del lado. Seguro aparte en `sub_82096FF0`: un efecto que no está en el banco se queda sin dibujar en vez de llamar a una función basura (parada 0x80000003 medida con relevos automáticos).
- **Vida por miembro**: la del que sale se guarda; la del que entra se restaura.
- **El COM** se releva solo con menos del 30 % de vida si un compañero tiene más del 50 %.
- **Marcador**: fichas de los compañeros bajo la barra de cada jugador (estilo FighterZ / Tenkaichi 3) con el botón que los hace entrar.

## Online
- Cada PC manda su equipo al empezar (`kTeam`). El relevo termina en el mismo tick en los dos PCs (bits 0x0400/0x0800 del mando sincronizado).
- Probado: Tag Team 2v2 online entre dos copias en el mismo PC, con relevo en los dos y el mismo final. Pendiente: dos casas por internet.

## Pendiente
- **Relevo visible (prioridad):** animación real de caída antes del relevo y de entrada del que llega. Candidatos: acciones 28/30 (entrada) y 27/31/32 (salida). Ver las hipótesis en el estudio 16.
- **Ring out:** el combate termina aunque el compañero esté vivo. Condición no localizada (estudio 16).
- Técnicas: los datos de técnicas del que entra coinciden con los del original (medido en memoria). Falta confirmarlo con el mando ("E + flecha" no hacía nada en ninguno de los dos casos).
- Caras reales en las fichas y la tira (hoy, inicial del nombre).
- Medir la frecuencia del bucle (los 14, 30, 90 ticks se suponen a 60 Hz).

Estudios: `02`-`09` (primeras versiones), `10_bt3_seleccion_hud_animaciones.md`, `11_carga_luchador.md` … `19_cara_combate.md`.

Últimos: `16_ko_select_2026-10-11.md` (bandera de pelea 0x823F39A0, acciones, KO medido, reglas de SELECT y actualización de la noche).
