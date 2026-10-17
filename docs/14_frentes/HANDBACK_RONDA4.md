# Entrega de la Ronda 4: pulido de los modos jugables (2026-10-16)

Sin publicar. Falta tu prueba en persona y tu visto bueno antes de subir nada.
El detalle técnico de cada equipo está en `RONDA4_PULIDO.md`; los minijuegos de correr, en la sección P4 de `PORT_EXACTO.md`.

## Qué hay nuevo, en corto

### Infinite World
- **Campaña:**
  - Mapa de Dragon Mission con sus iconos.
  - Los 22 vídeos originales (FD01A–FD20 y ENDING), con sus subtítulos y su audio. Las voces salen en japonés si el juego está en japonés.
  - Rango y Zenie con la fórmula del original.
  - Fighter's Road completo, con la rueda de selección del juego.
  - Entrenamientos 1, 6 y 9.
  - Misiones con Gotenks, Vegito, Janemba, Goku GT, Pan, Baby y Vegeta GT. Las definitivas de los personajes importados ya no cuelgan el juego.
- **Los 18 minijuegos:**
  - **Correr** (6): aros, Bubbles, Camino de la Serpiente, nave de Namek, huir de Piccolo, Radar.
  - **Vuelo** (2): Vegeta y la carrera con Goten.
  - **Disparo** (4): ladrillos de Kaio, Kienzan, rayos contra Piccolo, Ten Shin Han contra Cell.
  - **Botones** (6): entrenamientos, Sala del Tiempo, gravedad, lista 1-10, Videl.

  Comparten:
  - reglas, tiempos y puntos del código original;
  - el HUD y los objetos 3D de IW, con sombreado de dibujo animado y profundidad real;
  - personajes con modelos de B3;
  - el sprint de IW con su barra y la carrera de IW.
- **Campos de IW:** sus 8 mapas originales se generan solos en tu PC al pulsar JUGAR, a partir de tu copia de IW (la primera vez, ~1-2 min). Tienen cielo, niebla y cámaras originales, y el suelo y las paredes de la colisión de IW.

### Budokai 2
- **Dragon World:** las 9 fases se juegan con los scripts originales: diálogos, eventos, movimientos de los rivales y objetos.
- **Tablero:** Goku y las fichas en 3D.
- **Final:** Shenron en 3D en su escena, con los deseos. El portal abre la Nave de Babidi.
- **Nave de Babidi:** con su pantalla original, sus pruebas, el Kili y premios que se convierten en cápsulas de B3.

### Leyenda de Mr. Satán
- Prólogo "Mr. Satan's Style" y final como en B1.
- Escenas 3D antes de cada rival en el ring de Cell, con los modelos de B3, sus poses de entrada y victoria, la charla original y la voz japonesa.
- Puntos del original.
- No hace falta ningún mapa de B1.

### Modo Héroe (beta)
- Eliges un estilo de lucha, el moveset de otro luchador, para el cuerpo elegido.
- En el marcador sale la cara del cuerpo.
- Tiene su propio anfitrión.
- Sagas con niveles y Zenie.

### Interfaz
- Paneles alineados.
- Cifras y "VS" del propio juego.
- El marcador de los modos solo sale cuando el juego enseña el suyo.

### Tenkaichi 3
Es otro motor. De momento solo sirven sus voces y sus textos (informe en `E:\DBZ3HD_trabajo\estudio_t3\`).

### Pulido final (lo que pediste al cierre)
- **Marcador de los modos:** "Misión 1/3" y "Daño %" van abajo en el centro, lejos del reloj.
- **Tarjeta de enfrentamiento de la Pasarela:**
  - caras que llenan el marco;
  - nombres que no pisan la franja de la regla;
  - globo más compacto.
- **Minijuegos de Infinite World:**
  - **Movimiento:** Gohan y Piccolo ya no se mueven a saltos. Budokai 3 acercaba a la fuerza a los dos luchadores, y eso se desactiva durante los minijuegos.
  - **Arranque:** sin la presentación de combate de Budokai 3. En su lugar va la entrada propia de IW: animación del protagonista, cartel del objetivo con el reloj parado y "START!" después.
  - **Cámara:** la de seguimiento de IW.
- **Menú final "Luchar otra vez":** en todos estos modos no sale; se pasa a la pantalla propia del modo, comprobado en los registros.
- **Pendiente:**
  - La animación final de IW en los minijuegos está apagada, porque dejaba la pantalla en azul.
  - En la huida de Piccolo, con la loma detrás, la cámara a veces queda muy cerca de Gohan.
  - Los textos propios de IW salen en inglés porque tu copia de IW es la americana.

## Resultado de las pruebas (versión instalada)
- Batería de todos los equipos: 22/22 PASS (`E:\DBZ3HD_trabajo\integracion\resultado.txt`).
- Batería rápida: 5/5 PASS.
- Después del pulido se repitieron y pasaron:
  - la Nave (tarjeta);
  - el HUD de los modos;
  - el minijuego 0;
  - la entrada de IW.

## Herramientas instaladas con tu permiso
- Extensión PS2 de Ghidra 12.1.2.
- REA (`rea`; el servidor dentro de Claude estará en la próxima sesión).
- C descompilado de IW, B1 y B2 en `E:\DBZ3HD_trabajo\ghidra\salida\`.

## Cómo probarlo
- Tu carpeta de pruebas es `out\build\win-amd64-release` (exe y DLL nuevos instalados, con respaldo en `_respaldo_ronda4_*`).
- Entra por **HUB > Pasarela** para Infinite World, Budokai 2, la Nave y Satán, y por **HUB > Modo Héroe**.
- Pruebas automáticas (desde la carpeta del proyecto):

```bash
python tools/pruebas.py todo
```

```bash
python E:/DBZ3HD_trabajo/integracion/bateria.py
```

## Archivos
**Nuevos:**
- Fuentes en `src/`:
  - `cinematica.*`, `video_sfd.*` (+ `video_sfd.cmake`), `pasarela_nave.cpp`
  - `anim_iw.*`, `suelo_iw.*`, `ps2_modelo.*`
  - `iw_mini_vuelo.*`, `iw_mini_disparo.cpp`, `iw_mini_botones.cpp`
- `mod center hd/campos_iw.py` y `escenarios_convertir.py` (movidos desde `tools/`, que guarda dos lanzadores con esos nombres).
- `res/ingame_ui/hub/anfitrion_heroe.png`.
- Shaders del SDK: `dbz3_ui_depth.*` y `dbz3_depth_copy_ms.cs`.

**Modificados:**
- `src/`:
  - `pasarela*.{h,cpp}`, `iw_minijuegos.*`, `combate.*`, `clima.cpp`, `heroe.cpp`, `rpg.cpp`, `modos.*`
  - `ingame/game_style.*`, `ingame/hub_panel.cpp`, `ingame/native_ui.cpp`, `ingame/tagteam_panel.*`
  - `select_ext.cpp`, `net/netplay_hooks.cpp`, `ports_guard.cpp`, `escenarios.*`, `main.cpp`
  - `launcher/mod_pipeline.*`, `launcher/launcher_state.*`, `eu_map_gen.h`
- `CMakeLists.txt`.
- Scripts: `mod center hd/roster_build.py`, `awo_tools/amo2awo.py`, `assets_propios/generar.py`.
- `tools/`: `pruebas.py`, `eu_map.py`, `make_modpacks.py`, `verify_release.ps1`.
- `AGENTS.md` §7.
- SDK: presentador D3D12 y copia de profundidad.

**DLL canónicas nuevas:** `rexruntime.dll` 11158528 B y `rexgpu-xenos.dll` 6596608 B.

## Ronda de bugs y pulido con enjambre Haiku (10:00-11:00)
- **Caras de los minijuegos:** cada una es la del personaje que juega. La cara "de Gohan" de IW era Yamcha; ahora, si IW no la tiene, se usa la de B3.
- **Tablero de B2, leído del código del juego:**
  - Bolas: de 1 a 3 por fase con su número, y se recuperan las que no cogiste.
  - Objetos enterrados: se sacan de uno en uno; los que dependen de una condición aparecen cuando toca.
  - Rivales: cada uno se mueve según su tipo.
  - Botones como en B2: B pasa, Y sale.
  - Casillas y caminos ya coincidían con el original.
- **Idiomas:** 428 textos de los modos nuevos traducidos a francés, alemán e italiano (`ingame_ui/i18n_modos.toml`). Conviene que los revise alguien nativo.
- **Ladrillos de Kaio:** visibles y centrados bajo la mira.
- **Goku 3D del mapa de IW:** se carga, pero IW pinta la piel al dibujar y no se ha descubierto cómo; queda el 2D (el 3D se activa con `DBZ3_IW_GOKU3D=1`).
- **Hallazgo, no es de esta ronda:** si no hay ningún mando de Xbox conectado, el juego prueba los mandos SDL, y en este PC eso tarda 40 s al arrancar (fallo conocido con RivaTuner/OBS). Con un mando de Xbox encendido no pasa.
- **Pruebas:** 22/22 de los modos (Satán al repetirla sola; la primera vez se quedó sin tiempo por esos 40 s) y 5/5 de la batería rápida. Instalado en `out\build\win-amd64-release` con respaldo `_respaldo_ronda4b_1059`.

## Ronda final (11:40-12:10): batería, instalación y auditoría de ports y animaciones
- **Batería final con todo integrado:** 22/22 PASS y 5/5 PASS (`E:\DBZ3HD_trabajo\integracion\resultado3.txt`). Instalado en `out\build\win-amd64-release` con respaldo `_respaldo_ronda4c_1206`; el respaldo más antiguo (`_respaldo_ronda3_2321`) pasó a `D:\_PARA_BORRAR_CLAUDE`.
- **Arreglado:** los 4 personajes de IW instalados (Baby, Goku GT, Pan, Vegeta GT) no tenían la marca de origen IW en su ficha, y el montador no les aplicaba los arreglos de definitivas y técnicas. Fichas actualizadas; las anteriores están en `mods_respaldo\ronda4c_fichas_iw_*`.
- **Informes de la auditoría** (`E:\DBZ3HD_trabajo\auditoria5\`):
  - `A_ports_iw.md`
  - `B_ports_b1_b2_sb.md`
  - `C_animaciones_modos.md`
  - `D_goku3d_final_azul.md`
  - `E_modos_tenkaichi3.md`
- **Pendientes de la auditoría, por orden:**
  1. **Vuelo de IW:** usa el sprint aéreo; el original tiene poses de vuelo propias (0x320-0x32F).
  2. **Minijuegos de botones:** sin las poses de IW; el rival falla siempre con la patada de B3.
  3. **Final de los minijuegos:** sigue apagado por la pantalla azul. Medido a las 12:15: el azul sale desde la meta con cualquier animación, también con la de inicio, que se ve bien (prueba `--env DBZ3_IWMINI_FINAL=2`). Tampoco se ve el "CLEAR". El fallo no está en la animación: es del cuadro entero (cámara, niebla o presentación). Captura `d_final` añadida a la prueba `iw_minijuego`.
  4. **Ten contra Cell:** Cell sin sus animaciones (0x654-0x66C).
  5. **Entradas de los minijuegos 14 y 16:** cortadas a 300 cuadros cuando duran 420 y 330.
  6. **Vegeta GT:** 65 técnicas de 64 posibles (el juego quita una).
  7. **Pan:** pide la cámara 23 y solo tiene 21.
  8. **Zarbon y Dodoria:** sin confirmar si sigue la "patada que gasta ki".
  9. **Gohan del Futuro:** no coincide con su lista de pendientes.
  10. **Goku 3D del mapa de IW:** la piel la pone el juego al iluminar. Pendiente un tono de piel medido.
- **Tenkaichi 3:** 8 ideas que no necesitan datos de T3. Las primeras:
  - "Misión 100" con retos;
  - colección de esferas con deseos;
  - duelo por puntos;
  - ramas "¿Y si...?" en las campañas.

## Movimiento libre de los minijuegos de IW (13:00-13:30, auditorías 6 y 7 con enjambre Haiku)
Informes en `E:\DBZ3HD_trabajo\auditoria6\` y `auditoria7\`. Cambios:
- **Andar libre como IW:** al moverse, el personaje usa la carrera de IW (código 0x16 del banco común, elegida viéndola entre 5 candidatas) y, al parar, el reposo de IW (código 0). Antes iba con los pasos de guardia de B3. (`anim_iw.cpp` `Bucle`, `iw_minijuegos.cpp`)
- **Sin tirones:** B3 ya no mete su andar de combate (acciones 2/3) durante el andar libre: el gancho `sub_82103908` (`tagteam.cpp`) pregunta a `iwmini::AnularAndar`. Antes reiniciaba la carrera de IW en cada cuadro. Ahora se arranca una vez por carrera (comprobado en el registro).
- **Bucle cerrado:** la carrera y los reposos de IW ya no dan un salto de cadera al volver a empezar. `CerrarRaiz` resta la deriva de la cadera.
- **Cámara como IW:**
  - ya no la arrastra la marcha (eso la hacía girar sin fin al ir de lado y temblar al ir hacia atrás);
  - la gira la palanca izquierda de lado, a 0,008 por cuadro, y la derecha;
  - si una loma tapa al jugador, sube por encima antes de acercarse;
  - choca también con las paredes de los acantilados.
- **Vuelo:** se probó la pose 0x320 de 915, pero es una pose de pie. Se queda el sprint aéreo de IW.
- **Pantalla azul:** salía por el teletransporte de la prueba automática a la meta, no en partida real. Se añadió la captura `d_final` y la ráfaga `--par rafaga=1` a la prueba `iw_minijuego`.
- **Entradas de los minijuegos 14 y 16:** ya no se cortan (tope de 480 cuadros).
- **Pendiente:**
  - poses de los minijuegos de botones y de Ten contra Cell (falta confirmar qué código va con cada estado);
  - andar lento con la palanca a menos de 0,9 (falta identificar su animación);
  - animación de caída al bajar de una cornisa.

### Con vídeos de referencia (14:00)
Fotos de vídeos reales de IW en `E:\DBZ3HD_trabajouditoria7eferencias_video\` (con `LEEME.md` para los equipos). Aplicado:
- **Final de los minijuegos activado:** primer plano del personaje con su pose y CLEAR! sobre el escenario; el fundido llega después. `DBZ3_IWMINI_FINAL=0` lo apaga.
- **Al ganar:** se usa la animación de superado (0x52, 0x119...), no la de no superado.
- **Reposo de pie y relajado:** código 0 del banco 914, como en los vídeos; antes era la guardia.
- **Pruebas:**
  - batería completa 22/22 antes de estos cambios;
  - batería de minijuegos de IW 13/13 después;
  - 2 pruebas sueltas más tras el último cambio.
- **Instalado** con respaldo `_respaldo_ronda4d_1406`.
- **14:14:**
  - las figuras 3D de los modelos de PS2 (tablero de B2, Shenron, mapa de IW) interpolan entre posturas, sin saltos;
  - el Goku del tablero apoya los pies en cada cuadro;
  - los planos de pose de las escenas de Satán, más abiertos (46), ya muestran los pies;
  - escenarios de Satán y B2: 5/5 PASS;
  - instalado con respaldo `_respaldo_ronda4e_1414`.
- **Minijuegos de botones:** en los vídeos también van en guardia con golpes, como los nuestros. Sin cambios.
- **14:30:**
  - andar lento con la palanca por debajo de 0,9, como IW (0x15 del banco común, a 1,8); con el teclado siempre se corre;
  - caída con gravedad al bajar de una cornisa: el suelo del juego baja poco a poco con `SueloFijo` mientras suena la caída de IW (0x22 de 914);
  - batería de minijuegos de IW 13/13;
  - instalado; el respaldo previo es `_respaldo_ronda4e_1414`.
- **Ko-fi:** propuesta y borradores en `docs/14_frentes/KOFI_PROPUESTA.md`. No se ha creado nada.
- **14:56:**
  - vuelo horizontal de IW: pose 0x32C del visor de vuelo (DATA_CMN 915). Su almacén 0 es su propio #AMM, no el global, y lleva la raíz girada como el sprint;
  - caída con la pose "en el aire" (0x21 de 914);
  - prueba de vuelo PASS;
  - instalado.
- **15:20, cuatro equipos Haiku más.** Informes en `auditoria7`: I (Ten y Cell), J (vuelo), K (sprint), L (repositorios).
  - **Vuelo:**
    - pose de cada estado del visor de IW: reposo 0x320, crucero 0x322, impulso con R1 0x32C, subir 0x328, bajar 0x324, de lado 0x329/0x32A, planeo 0x323, frenazo 0x32E;
    - sin la mezcla entre poses ni la virada y media vuelta (estados 2 y 7).
  - **Sprint:**
    - la frenada se completa antes del cansancio;
    - el cansancio acaba con la recuperación 0x07 de IW, no con la guardia.
  - **Cell (minijuego de Ten Shin Han):**
    - sus animaciones de IW por estado (0x640-0x66C del #BSK de 914) y las duraciones reales de cada fase;
    - en el registro se ve el cambio de animación; en las capturas en primera persona no se ve a Cell.
  - **Repositorios:**
    - decomp.wtf está archivado y aporta poco;
    - BT3-Recomp (GPL-3.0) es una recompilación de Tenkaichi 3 y sirve para el punto 4.
  - **Pendiente:**
    - salto en los minijuegos a pie (IW salta con X: estados 0x0C-0x13);
    - polvo y sonido del sprint (no se conocen los ids de efecto y sonido);
    - giro progresivo durante el sprint.
  - **Pruebas:** batería de minijuegos de IW 13/13 y Cell PASS. Instalado.
- **15:42:**
  - **Salto con la cruz (A en el mando de Xbox)** en los minijuegos a pie: sube con la pose 0x20 de 914 y cae con la 0x21. Usa `SueloFijo` con el despegue de IW (7·k) y gravedad 0,15. Prueba nueva `--par salto=1`.
  - **Sprint:** gira poco a poco hacia la palanca (0,08 rad por cuadro, tasa a ojo).
  - **Pruebas:** batería de IW 13/13 antes del giro; sprint y andar PASS después. Instalado.
- **15:45:**
  - **Vuelo:** virada (0x321, 22 cuadros) al arrancar hacia delante y media vuelta (0x32D, 44 cuadros; R1 con la palanca atrás; el rumbo gira 180° al acabar). La prueba de vuelo pasa, pero el piloto automático no pasa por ellas: falta verlas jugando.
  - Instalado.
- **Investigado sin cambios (auditoria7 M y N):**
  - **Polvo del sprint:** es un efecto propio de IW (comando 0x34, un AMO de tipo 0x1A). Hay que portarlo; B3 no tiene uno equivalente fácil de llamar.
  - **Mezcla entre animaciones:** B3 no funde una animación con la siguiente por la vía que usamos; IW sí. Haría falta interpolar las poses en nuestro código.
- **16:10, mezcla entre animaciones de IW** (`anim_iw.cpp`: CrearTransicion, PrepararTransicion, Muestra):
  - al pasar de una animación de IW a otra (carrera, reposo, andar, caída, vuelo, Cell...) se pone una de transición de 8 cuadros: de la pose actual a la primera de la siguiente;
  - B3 no mezcla por la vía que usamos; IW sí (CharModel_SetAnimBlend);
  - no se aplica en el sprint, que va por cuadros;
  - `DBZ3_SIN_MEZCLA=1` la apaga;
  - pruebas: batería de IW 13/13 (la de Piccolo, al repetirla sola: la primera vez no llegó al menú por el arranque lento conocido). Instalado.
- **Polvo del sprint localizado** (`auditoria7/O_recurso_polvo.md`): es el #AME 934/26 de IW.
  - Tiene un cono con 4 partículas de malla y un rayo.
  - Para portarlo hay que sacar el reproductor de efectos `ame` a una cabecera y enseñarle a dibujar partículas de malla.
  - Sin hacer: espera al visto bueno del usuario.
- **16:40:**
  - **Lomas empinadas:** el sprint y el andar ya no meten al personaje por debajo de una loma más alta que su cabeza (`MoverCampo`: IW la trata como pared). Antes la cámara veía el terreno desde abajo.
  - **Polvo del sprint (#AME 934/26):** el reproductor `ame` pasa a `src/iw_ame.h`, compartido con el disparo, y admite partículas de malla. El polvo está montado, pero no se ve en las capturas (la malla 934/1 mide 120 de medio lado: puede que no sea la del polvo o que le falte la textura). Se queda apagado salvo con `DBZ3_IW_POLVO=1`.
  - Batería de IW 13/13. Instalado.
- **17:25 (correcciones del usuario: "Cell en forma semiperfecta", "esas poses con las piernas volando parecen erráticas"):**
  - **DATA_CMN 914 es Cell semiperfecto** (huesos CEL_*), no un humano:
    - el rival del minijuego de Ten Shin Han pasa a la forma 2 de Cell (`RequestBattleEx` form2 = 1);
    - sus animaciones usan solo los giros, salvo la cadera;
    - comprobado con la cámara de prueba `DBZ3_DISPARO_VER_CELL=1`: sin pinchos y con la cola bien.
  - **Goku ya no usa animaciones de Cell:**
    - reposo 0x800, el 0x00 común (la pose de espera de IW);
    - salto con el despegue común 0x29, quieto en su cuadro 6 (las rodillas recogidas) hasta aterrizar (`Reproducir(..., tope)`);
    - descartadas, vistas en capturas: 0x22, 0x104 y 0x105 (en picado o con voltereta).
  - Batería de IW 13/13. Instalado.

## Jornada de pulido con seis equipos Haiku (17:40-18:10; informes en `E:\DBZ3HD_trabajo\auditoria8\`)
- **Arreglado:**
  - **Tablero de B2:**
    - el Zenie de las casillas va al monedero del HUB;
    - la cápsula al azar da una cápsula de B3;
    - los deseos sin equivalente lo dicen en pantalla.
  - **Resultado de los minijuegos de IW:** usa el rango real del minijuego (antes se recalculaba por puntos, con umbrales sin confirmar).
  - **"Huir de Piccolo" (14):** sin la entrada ni el final del 914, que es Cell.
  - **Pruebas:** batería completa 22/22. Instalado con respaldo `_respaldo_ronda4f_1810`.
- **Propuestas (sin hacer: decide el usuario):**
  - **A, rangos estilo Tekken:** el "Escalafón del Dojo", con 11 peldaños de Kyu a Maestro.
    - Puntos por mejorar récords en IW, Héroe, Satán, la Nave y las torres.
    - Sin bajar de rango; prueba de ascenso desde Dan 1.
    - Insignia en el HUB y pantalla de ascenso.
    - Se guarda en `rango.toml`.
  - **B, minijuegos:**
    - "superado" o "no superado" en el resultado;
    - fila BEST;
    - reintentar;
    - sonidos del resultado;
    - releer los controles;
    - aviso al salir desde la pausa.
  - **C, tablero:**
    - aviso de turno de los rivales y su alcance;
    - el objetivo en el HUD;
    - guardar a mitad de fase;
    - esferas fijas;
    - auto-ruta opcional;
    - la Nave enseña el premio de cada prueba.
  - **D, historia y héroe:**
    - cambiar de estilo entre sagas;
    - maestros que se desbloquean;
    - tienda entre sagas;
    - saltar el diálogo línea a línea;
    - galería de escenas.
  - **E:** la lista de pulido completa (~70 cosas) con sus arreglos rápidos.
  - **F, HUB:**
    - progreso por modo;
    - perfil y estadísticas;
    - galería de los vídeos de IW;
    - "NUEVO" y "siguiente recomendado".
- **18:39, Escalafón del Dojo (rangos al estilo Tekken, versión 1).** Funciones en `pasarela.h/.cpp`: `RangoPE`, `RangoNivel`, `RangoNombre`, `RangoMejora`. Se guarda en progreso.toml (`rango_pe`).
  - **Peldaños:** 11 (Kyu 5…Kyu 1, Dan 1…Dan 5 y Maestro), con umbrales 0/100/250/450/700/1000/1400/1900/2500/3200/4000.
  - **Puntos, solo por mejorar lo guardado (repetir no da nada):**
    - misión de IW: 30 por cada rango que mejora y 20 la primera vez;
    - minijuego de IW superado: 10 por rango;
    - cada combate de Satán: 25;
    - cada prueba de la Nave: 25;
    - saga del Héroe: 15 + nivel.
  - **HUB:** "Rango: X (N PE)" en el bocadillo; al ascender, "¡Ascenso! X" unos segundos.
  - **Sin hacer:** la prueba de ascenso de los Dan, las torres, Tag Team y Online.
  - **Pruebas:** batería completa 22/22; los registros muestran los PE de Héroe, Satán y la misión de IW. Instalado con respaldo `_respaldo_ronda4g_1839`.
- **18:57:**
  - **Progreso de las campañas en la fila Pasarela del HUB:** "IW n/96 - B2 n/9 - Satán n/11" (progreso.toml). La prueba hub_lab pasa; la fila, que está en la página 2, no se ve en sus capturas.
  - **Polvo del sprint:** los conos ya se dibujan con el color de la partícula, pero salen oscuros. Falta la mezcla aditiva en `ps2::Escena`. Sigue apagado (`DBZ3_IW_POLVO=1`).
  - Instalado.
- **20:30, Misión 100 propia y lo aprovechable de BT3-Recomp.** De BT3-Recomp (GPL-3.0) solo se toman ideas de diseño; no se copia código.
  - **Misión 100** (nueva fila del HUB, `rpg.cpp`: `kPaginas`, `MisionDe`, `DibujarM100`; tipo `kMision` = 10 en `modos.h`):
    - 20 páginas de 5 misiones, una página por saga (de Raditz a Omega Shenron).
    - Cada página trae, por orden:
      1. combate;
      2. contrarreloj;
      3. aguantar con más daño recibido;
      4. regla especial: ki que se agota, rival que se regenera, rival blindado o rival gigante con tormenta;
      5. **Tag Team** (tú y un aliado contra un equipo de 2, o de 3 desde la página 11; SELECT releva). Cada 5 páginas es **supervivencia**: 3 seguidos con la misma vida.
    - Rango S/A/B/C por la vida que queda. Se guarda el mejor en `rpg.toml` (`mision100`). Da Zenie (100 + 25 por página) y PE del Escalafón (`rango_m100_N`).
    - La página siguiente se abre con 3 misiones superadas. `DBZ3_M100_TODO=1` las abre todas.
    - **Equipos:** `tagteam_slots.cpp` ya no los borra mientras dura una misión de equipo (`rpg::EquiposMision`). Al acabar se limpian.
  - **Escenas de Satán y de las campañas** (`cinematica.cpp`): A pasa a la frase siguiente (la voz nueva corta la anterior) y START salta la escena.
  - **Pruebas:**
    - `m100` (`--par n=N id=ID`): misión 10, Tag Team. Los dos lados llevan equipo, y al hacer ring out entra el relevo. PASS.
    - Misión 1: victoria, rango S guardado, premio de 100 Zenie y +50 PE. PASS.
    - Satán: PASS. La escena muestra "A Siguiente / START Saltar".
  - **Instalado** con respaldo `_respaldo_ronda4h_2025`. El respaldo más antiguo se movió a `D:\_PARA_BORRAR_CLAUDE`.
  - **Sin hacer de BT3-Recomp:**
    - la pista TCC/alfa del vértice para el polvo y el Goku blanco;
    - los ajustes de batalla;
    - el duelo por puntos.
- **20:40:**
  - **Modo Héroe:** la pantalla final de la saga muestra "¡Sube de nivel! X > Y (+N vida, +N% daño)" (`heroe.cpp` NivelAntes). La prueba r6_hud pasa.
  - **Cambiar el estilo (moveset) a mitad de saga:** no se hace. El combate siguiente ya está pedido con el luchador anterior (modos.cpp Encadenar). El estilo se sigue cambiando en la pantalla de creación, antes de cada saga.
  - **Polvo del sprint:**
    - Los conos usan ya la "suma" emulada: el alfa es el brillo, y el tono va a tope.
    - La franja oscura que se veía es la sombra de Goku, no el polvo: sale igual con el polvo apagado.
    - Los conos (azulados: 0.33/0.33/1.0) siguen sin verse, ni a ×5 (`DBZ3_POLVO_X=5`).
    - Siguiente paso: dibujarlos sin la prueba de profundidad ni las esferas del luchador.
    - Sigue apagado (`DBZ3_IW_POLVO=1`).
  - Instalado con respaldo `_respaldo_ronda4i_2040`. El respaldo más antiguo se movió a `D:\_PARA_BORRAR_CLAUDE`.
- **20:43, comprobado en pantalla** (prueba nueva `hub_filas`): en la página 2 del HUB, la fila Pasarela dice "IW 11/96 - B2 0/9 - Satán 2/11 - Rango: Kyu 5 (75 PE)". Misión 100 se abre desde su fila, en la primera misión sin superar. Las filas Escenarios y Minijuegos no salen en el HUB (hay 11 filas).
- **20:50:**
  - **Cámara del sprint (IW)** (`iw_minijuegos.cpp`, punto seguido):
    - Problema: con el scatto (35 u por cuadro) la cámara se quedaba a más de 150 u. Entonces saltaba de golpe al jugador, cruzaba lomas y acababa dentro del terreno o mirando desde arriba (ráfaga 204507).
    - Arreglo: a partir de 40 u alcanza más deprisa (0,12 + (dist − 40)/60). Al correr normal no cambia.
    - Ráfaga 204811: el plano sigue a Goku sin saltos.
  - **Polvo:** probado también sin la prueba de profundidad; sigue sin verse. Sigue apagado.
  - **Auditoría 9:** 6 equipos Haiku en `E:\DBZ3HD_trabajo\auditoria9\`:
    1. bugs;
    2. importar efectos, caras y sonidos en el Kit;
    3. issues de GitHub;
    4. comentarios de jugadores;
    5. Misión 100 frente a la de T3;
    6. material de vídeo del usuario.
  - Instalado con respaldo `_respaldo_ronda4j_2050`.
- **21:15, auditoría 9 aplicada (lo barato y seguro).** Los informes están en `E:\DBZ3HD_trabajo\auditoria9\`: RESUMEN_1_2_4_5.md, RESUMEN_3_github.md y 6_assets_video.md.
  - **Misión 100:**
    - un nombre propio para cada una de las 100 misiones (es/en);
    - rango Z: S sin recibir daño en el último combate;
    - caras del equipo en la ficha (tu aliado a tu lado y "VS" con los rivales);
    - el bocadillo dice el tipo de misión.
  - **Arreglos:**
    - "¡Sube de nivel!" del Héroe ya no sale al guardar o abandonar;
    - los equipos se limpian si el combate no se pudo pedir;
    - el `extra` de una misión se acota al leer rpg.toml;
    - en las escenas, A pulsada antes de 0,4 s ya no se pierde.
  - **Comentarios de jugadores:**
    - **Pantalla negra al arrancar:** `main.cpp` pone el cuadro de error antes de crear la GPU y captura `abort()` (SIGABRT). Ahora sale una ventana con la ruta del registro y la recomendación de probar Vulkan.
    - **"Using ISO, mods will not":** el aviso del lanzador explica cómo hacerlo (extraer el disco y elegir «Carpeta extraída»).
  - **Issue #20:** si el botón "partida al 100%" sale en gris, el lanzador explica que falta `mods_nativos\partida_100`.
  - **Packs visuales** (`construir_pack.py`): `[pack] desactivado = true` saca el pack con .disabled aunque traiga piezas (regla del arte de terceros). La prueba `--prueba` pasa.
  - **Pruebas:** m100 PASS (nombres y caras vistos), hub_filas PASS (arranque sin errores) y batería de IW 13/13 tras el cambio de cámara.
  - Instalado con respaldo `_respaldo_ronda4k_2114`.
  - **Pendiente (decide el usuario):**
    - responder los issues de GitHub (hay borradores);
    - origen de 4 imágenes de `assets_propios/png/add/` que no menciona el README;
    - voces propias desde WAV en el Kit (`voces = "wav:"`);
    - botón para crear un traje desde un port;
    - icono propio `modo_mision100`.
- **21:18, icono propio de Misión 100:**
  - `assets_propios/generar.py` dibuja un pictograma propio (sin arte de terceros): una tablilla con la lista de misiones.
  - Añade también la etiqueta en 5 idiomas.
  - Con `--solo-hub --al-juego` se generaron 6 piezas nuevas en `res/ingame_ui`, que se copiaron a la release.
  - Visto en la prueba hub_filas: la fila sale con su icono, como las demás.
- **21:35, Mod Kit: voces propias desde WAV** (auditoría 9, equipo 2, punto 1). Responde al comentario "Janemba con la voz de Krilin".
  - `voces = "wav:voces"`: un WAV por frase, `voces/NN_nombre.wav`. Las que falten, del donante.
  - Cambios: voces.py (`leer_wav`, `wav_block`, `resolve(base=)`) y roster_build.py (pasa la carpeta del mod). Las pistas de la ficha del Kit ya mencionan `wav:` y `sb2:`.
  - Prueba `voces.py --prueba-wav` OK (16 y 24 bits; un nombre sin número da aviso). Falta oírlo en el juego con un port.
- **21:30, pack local con el material del usuario** (equipo Haiku):
  - **Receta:** `mod center hd/packs_visuales/pack_material_dbz.toml`, con 8 iconos de modo del HUB:
    - Red Ribbon: torre;
    - Capsule Corp: laboratorio;
    - bola de Babidi: kumite;
    - bolas de dragón: ranking y supervivencia;
    - la Tierra: online;
    - insignia de Satán: satan;
    - scouter: Misión 100.
  - **Construido** en `out/build/win-amd64-release/mods/pack_material_dbz`, DESACTIVADO (.disabled), con créditos.
  - **Probado activado** en la copia combate_test (hub_filas 212410): los iconos se ven. Después se dejó otra vez desactivado.
  - **Evaluación para compartirlo:** `docs/13_assets_externos/COMPARTIR_PACK.md`. Se recomienda subir solo la receta (opción A); un enlace privado es decisión del usuario. No se ha subido nada.
- **21:45, Mod Kit:**
  - **Botón "Convertir en traje de su personaje base"** en la ficha de cada personaje importado (Mods). Responde al comentario "Vegeta GT como traje de Vegeta".
    - Llama a `importar.py traje <mod> --mods …` (mod nuevo `traje_<mod>`; el port no se toca).
    - Pregunta si el traje usa los golpes del personaje base (`--sin-enlace`) o los del port.
  - **construir_pack.py:** si faltan imágenes, las lista todas de una vez antes de tocar el pack anterior. Es lo que necesita la opción A de COMPARTIR_PACK.md.
  - **Pruebas:**
    - `modkit_gui.py --selftest` OK;
    - `construir_pack.py --prueba` OK;
    - con la raíz equivocada, lista las 8 imágenes que faltan.
  - El botón no se ha pulsado con un port real (la autoprueba no pulsa botones).
- **21:35, REVERTIDO por orden del usuario:** todo lo hecho hoy con su carpeta de material de edición se ha movido a `_PARA_BORRAR_CLAUDE/material_dbz_revertido_2131` (en D:):
  - la receta pack_material_dbz.toml;
  - el pack de la release y el de la copia combate_test;
  - COMPARTIR_PACK.md;
  - el inventario 6_assets_video.md y sus temporales.
  
  construir_pack.py vuelve a parar en la primera imagen que falta. Se queda `desactivado = true`, que es de la regla del arte de terceros (auditoría 9) y no del material.
  Las entradas 21:30 y 21:45 sobre el pack quedan sin efecto.
- **21:42, duelo por puntos en Tag Team** (idea de los DP de Tenkaichi 3, con valores propios; `ingame/tagteam_panel.cpp`):
  - **Cómo se usa:** en la pantalla TAG TEAM, LB/RB elige el presupuesto (libre/6/9/12). Cada luchador cuesta de 1 a 5 (`Coste`): Broly y Omega 5; Goku, Vegeta, Gohan, Freezer, Cell y Buu 4; Yamcha, Satán, Videl, Saibaman y Cell Jr. 1; los de mods 3.
  - **En la rueda:**
    - el rótulo dice "N/M puntos";
    - un luchador que no cabe (contando 1 punto por cada miembro que falta) se rechaza como un repetido, con el aviso "No quedan puntos para ese";
    - vale también para el COM.
  - **Prueba:** `tag_swap --env DBZ3_TAG_PUNTOS=1` (6 puntos) PASS: menú y rótulo vistos; equipos 0+2 y 2+10 (6 y 4 puntos) y combate con relevo.
  - **Sin probar:** el rechazo. El piloto de la prueba no eligió a nadie que no cupiera.
  - Instalado con respaldo `_respaldo_ronda4l_2141`.
- **22:25, auditoría 10:** 4 equipos con un líder Sonnet y ayudantes Haiku. Los informes están resumidos aquí; los scripts de medida, en `E:\DBZ3HD_trabajo\auditoria10\tmp\`.
  - **Animaciones de IW** (`anim_iw.cpp`, `iw_minijuegos.cpp`):
    - La mezcla entre animaciones duraba 2 cuadros, no 8: Tick va 4 veces por cuadro. Ahora cuenta por el cuadro de la propia transición y la 7FE dura unos 134 ms.
    - Entrada, comienzo y recuperación se sujetan, sin la guardia de B3 en medio.
    - El cansado es el de IW (0x805/0x806, con la cadera cerrada).
    - La prueba `salto=1` espera `codigo 829`.
    - Log 222115: 11D → 7FE → 051 y 805 → 806 → 807 → 800, sin 000 en medio.
  - **Escenas de Satán** (`cinematica.cpp`, `pasarela.cpp`):
    - El plano general lleva las poses de entrada (0x281/0x280).
    - Cada pose se pone una vez por paso: Satán repetía el gesto cada 4 s.
    - Sin probar con la prueba `_r3_cine`.
  - **Goku 3D pequeño** (`ps2_modelo.cpp`, `pasarela_b2.cpp`, `pasarela_iw.cpp`):
    - Texturas base opacas: la cara salía con agujeros.
    - Piel FFD2AA en las partes de piel de IW.
    - En B2, animaciones absolutas: reposo 0 y carrera 4 (salía de lado con un brazo arriba).
    - El andar de IW es la animación 3.
    - Vistos en tablero 221249 y en iw_mapa 221616 (con DBZ3_IW_GOKU3D=1): de pie, con cara y piel. La ropa aún sale con rayas; el equipo E lo compara con el rip de GameCube que dio el usuario (`auditoria10\ref_goku_map_gc`, solo como referencia).
  - **Ports:**
    - `awo_tools/rampas.py`: `es_b1` trata los 68 modelos de B1 (antes 28; Goku, Krilin, Vegeta... salían blancos). Medido con b1_todos.py.
    - `roster_build.py`: los ports de IW bajan al suelo todos los códigos en que el donante está apoyado (Goku GT flotaba en 185).
    - Los dos ficheros entran en la huella de construcción, así que los mods se rehacen al pulsar JUGAR. Falta verlo en el juego.
  - **Minijuego de Ten:**
    - El aro del tiempo, arriba en el centro: visto en r13_cell 222334.
    - Las manos siguen blancas, porque el color no está en el material (lo mira el equipo E).
  - **Pendiente, propuesto por los equipos:**
    - medidor DANGER/SAFE como el original;
    - fase de choque "machacar" del NB013;
    - ficha de misión reordenada como la de IW;
    - cámara de entrada en primer plano;
    - Baby en el Torneo;
    - cierre de Pan (`sub_82152890`);
    - escalar la cadera solo en y.
  - Instalado con respaldo `_respaldo_ronda4m_2226`.
- **22:45, Goku 3D pequeño arreglado** (equipo E, con el rip de GameCube del usuario como referencia; `ps2_modelo.cpp`):
  - **Causa de las rayas de la ropa y de los brazos pálidos:** `LeerAmo` leía los trozos de la lista de pesos de 16 en 16, pero miden 32 B `[peso, n32, loc32, n16, loc16, ...]` (awo_tools/amo2awo.py). Así se perdía la mitad de las listas y todas las de 16 B, y los vértices quedaban rígidos al hueso 0 (en el Goku de B2 se cargaban 1906 destinos de 2598).
  - **Arreglo:** lectura de 32 B más las listas de 16 B; con peso parcial, el resto va al padre en reposo; normal de respaldo.
  - **Piel:** `RampasPiel` detecta la rampa azulada de las partes de piel (en IW, el índice `+0x0C`), en lugar de suponer el índice 3. Las manos de Ten ya salen con piel.
  - **El Goku 3D del mapa de IW sale por defecto** (`DBZ3_IW_SIN_GOKU3D=1` vuelve al 2D).
  - **Pruebas:**
    - tablero 223804: Goku limpio, igual que el de GC;
    - iw_mapa 224114 y 224341 (ya sin variable): limpio;
    - r13_cell 223804: manos con piel;
    - r2_deseo (Shenron) y Bubbles: sin cambios visibles a peor.
  - Instalado con respaldo `_respaldo_ronda4n_2245`.
  - **Material de la comunidad:** "modding resources discord", leído por Haiku. Datos útiles para siguientes pasos:
    - +0x0C = shader;
    - formatos AMM/BSK de SB2;
    - lista de bins de IW;
    - LST/BPL del modo historia de IW.
    - No hay nada del tablero de B2.
- **22:50, escenas de Satán probadas** (`_r3_cine` 224834):
  - paso 1: Satán con su pose de entrada; el rival en guardia, porque con la suya se giraba de espaldas (224641);
  - paso 2: Satán con su pose de victoria, una vez;
  - paso 3: el rival con la suya, una vez.
  - Ya no se repiten cada 4 s.
  - Instalado con respaldo `_respaldo_ronda4o_*`.
- **22:57, Modo Héroe: estilo (moveset) cambiable entre combates de la saga** (objetivo 6).
  - **Cómo se usa:** en la pantalla entre combates, LB/RB cambia el estilo; se ve "LB/RB Estilo: X".
  - **Por dentro:**
    - `modos::SetJugador` cancela el combate ya encadenado con el luchador anterior (`hub::CancelChain`), y Continue lo pide con el nuevo.
    - `RivalHeroe` vuelve a elegir el rival con `heroe::Mision`. Si el rival es el luchador del estilo, toma el alternativo: con el mismo ID saldría con el cuerpo del héroe.
  - **Prueba:** `r6_hud --par estilo=1` (225615) PASS. Combate 1 con Vegeta y cuerpo de Goku; RB; combate 2 con Piccolo y cuerpo de Goku contra Ginyu.
  - Antes (225202), sin `RivalHeroe`, salía un "7 contra 7".
  - Instalado con respaldo `_respaldo_ronda4p_*`.
- **23:02, medidor DANGER/SAFE del minijuego de Ten, como el de IW** (`iw_mini_disparo.cpp`):
  - Piezas volcadas de DATA_USA 9/1 en `auditoria10/tmp/hud9` (t07 números, t08 triángulo, t09 arco, t10 SAFE, t11 DANGER).
  - **Cómo queda:**
    - el arco a la derecha (escala 1,75);
    - 11 segmentos de calor dibujados dentro del canal negro (blanco, amarillo, naranja y rojo), encendidos hasta el nivel;
    - los números fila a fila dentro del panel;
    - DANGER fijo arriba y SAFE fijo abajo;
    - el triángulo del nivel a la izquierda del canal.
  - **Prueba:** r13_cell 230155: visto, casa con la referencia del usuario.
  - Instalado con respaldo `_respaldo_ronda4q_*`.
- **23:06, entrada de los minijuegos a pie en primer plano** (`iw_minijuegos.cpp`, cámara durante el cartel):
  - Empieza a 11 u del jugador, a 4 u de alto y mirando a 7 u, y se aleja con suavidad hasta la de seguir mientras da la media vuelta (referencia 4_goku_pose_entrada).
  - **Prueba:** iw_minijuego 0x7DF PASS; en a_salida, Goku sale cerca y de tres cuartos.
  - Instalado con respaldo `_respaldo_ronda4r_*`.

## 23:58 — Mods desde la ISO, lanzador y pack Dragon HUB
- **Causa de "los mods no funcionan"**: jugando desde la ISO el juego ignoraba todos los mods (el override por entrada solo existia para archivos reales). Arreglo: al pulsar JUGAR (o en arranque rapido) se copian una vez los .afs que tocan los mods (todos menos las voces adx_*; con personajes nuevos tambien las adx) a `user_data/dbz3/iso_datos/<region>/` con sello de la ISO; `RegionDiscDevice` (src/region.cpp) los sirve desde ahi. `HostDataRoot()` (region.h) da esa copia al arte nativo (native_ui), la capsula unica (transformaciones.cpp) y el montaje de personajes (UsDir). Codigo: settings.cpp `IsoDataPending/PrepareIsoData/IsoDataCacheValid`, launcher `StartIsoPrep/DrawIsoPrepBox` (boton "Preparar la ISO para mods" en Mods, Modelos, Personajes e Inicio), main.cpp (arranque rapido).
  - Probado con la ISO retail (HD Collection): copia 7 archivos / 500 MB en 6,5 s; menu PASS; 22/22 piezas y 38 caras nativas; `du_freezer_nuevo` PASS con la historia del mod servida ("contenedor 6128 (del mod)").
  - Nova's Mods: la clave del release publicado (1.4.5.2) es la misma que la actual y el catalogo carga: esa parte funciona tambien en la publicada.
- **Lanzador**: ayuda al pasar el raton por cada pestana, textos de origen corregidos (la carpeta ya no es "la unica con mods"), botones Carpeta/ISO alineados, logo Dragon HUB en la cabecera. `style::Img("hub/*")` ya no espera a native_ui (en el launcher no estaba listo).
- **Pack Dragon HUB del autor** (assets_propios/png/add/dragon_hub -> generar.py `pack_dragon_hub` -> res/ingame_ui/hub): fondos por pantalla (Ranking = Torneo del Poder, Mision 100 = Namek, entre combates/final = Capsule Corp), anfitriones (Whis Ranking, Vegeta Mision 100, Goku entre combates, Trunks retos), placas de rango KYU5..MAESTRO en el HUB (y grande al ascender), vuelos de Trunks/C17 y guardado SSJ4 (antes solo en el pack opcional). Capturas: HUB, Ranking y Kumite OK.
- Pruebas: iso_test menu/hub_filas/du_freezer_nuevo PASS, du_test hub_filas/r6_ranking/m100 PASS. Respaldos ronda4q..u. Sin publicar.
- **Online (informe Haiku)**: nuestro online es por retardo (lockstep, retardo automatico), NO rollback; la desincronizacion solo compara el azar. Para usuario propio, "buscar partida" y salas cortas hace falta un servidor de citas (T3 Android usa Cloudflare Worker + D1, AGPL: solo ideas). Pendiente de decision del usuario.

## 2026-10-17 00:30 — Tanda "antes de subir" (Opus + Sonnet + Haiku)
- **Creditos y Ko-fi**: README/README_EN (raiz y github/) con insignia Ko-fi arriba, seccion "Apoya el proyecto" con el texto del usuario y credito a 🇮🇹⚠ 𝐒𝐏𝐈𝐃𝐄𝐑 ⚠🇮🇹 (decompilacion de IW); RELEASE_README.md tambien. Copias en D:\_PARA_BORRAR_CLAUDE\readme_backup_*.
- **Juegos de PS2 con un boton**: `pasarela::RegistrarIsoPs2/ElegirIsoPs2/IsoPs2` (pasarela_b1.cpp): el jugador elige su ISO (lanzador > Juego > "Contenido de tus juegos de PS2", o A en la campana/fila del HUB que la pide); se valida por la serie del SYSTEM.CNF (B1 SLES_512.33, B2 SLES_518.39, IW SLUS_218.42; otra region se acepta) y se guarda `ps2_games/<juego>.ruta` (no se copia nada). Los tres buscadores aceptan `.ruta`. b2::Disponible ya no se queda en "no" toda la sesion. Prueba DBZ3_TEST_PS2: B1 como B2 -> rechazada, B2 -> aceptada, win.ini -> "no es una ISO de PS2".
- **HUB**: marcas BETA (Tag Team, Online, Satan, Mision 100, Laboratorio, Escenarios) y EXPERIMENTAL (Pasarela IW/B2, Minijuegos IW) junto a cada fila.
- **Lanzador**: Nova's Mods en tarjetas (filtro por tipo, estado instalado/actualizacion, boton grande con ayuda); Mods con ACTIVO/APAGADO y ayuda en la casilla.
- **Audio para los videos**: cvar `dbz3_audio_dump` (SDK sdl_audio_driver.cpp; graba la mezcla aunque este silenciado) + `tools/grabar.py` (gdigrab + audio, juntados por la hora del log). Pendiente recompilar el SDK.
- **Equipos**: Haiku (Burst Limit Recompiled: su instalador copia la ISO entera; el nuestro juega de la ISO y copia 0,5 GB; lista de 34 fallos abiertos), Sonnet (traducciones: 128 -> 0 textos sin fr/de/it, Nova's Mods en 5 idiomas en tools/novas_mods.toml, SIN publicar; menu F1; campana B2; lote de fallos: Nova sha256, UPnP, campo IW 881...).
- Guion del video para OpenMontage: video/GUION_proxima_version.md (COM vs COM con relevos al inicio, changelog visual, tutorial ES/EN ISO y carpeta, creditos y Ko-fi).

## 2026-10-17 02:10 — Cierre de la tanda (todo instalado, nada publicado)
- Equipos Sonnet terminados: **menu F1** (8 secciones, BETA/EXPERIMENTAL/REINICIO, ayuda abajo; SDK quick_menu.cpp, structs sin cambiar de tamano), **traducciones** (0 textos sin fr/de/it; 100 titulos de Mision 100; Nova's Mods en 5 idiomas en tools/novas_mods.toml, sin publicar), **B2** (tablero OK, fases I-IX PASS, marcador/menu animados, EXPERIMENTAL en titulos y pantallas; radar "BOOM" no portado: el ELF no lo activa nunca), **fallos** (Nova sha256 opcional en catalogo + BCrypt; UPnP con cuenta por puerto; huella de mods del HELLO fuera de la UI; campo IW 881 ya arreglado en el generador; aviso de pruebas.py sin Campo IW).
- SDK recompilado: rexruntime **11172864** (menu F1 + `dbz3_audio_dump`), rexgpu-xenos 6596608; AGENTS.md y verify_release.ps1 actualizados; parches en github/patches/rexglue-sdk/. Respaldo de las anteriores en rexglue-sdk-0.10/out/_respaldo_canonicas/.
- Pruebas de integracion: iso_test menu/hub_filas, du_test tag_swap/m100 PASS; capturas del HUB con BETA/EXPERIMENTAL.
- **Clips con sonido** en video/clips/proxima (14 + tutorial ISO y carpeta); guion para OpenMontage en video/GUION_proxima_version.md. Gancho de prueba DBZ3_TEST_TOUR (launcher: recorre pestanas y pulsa JUGAR).
- Instalado en out/build/win-amd64-release (exe, DLL, ingame_ui). Respaldos ronda4r..v.
- Pendiente para publicar (con el OK del usuario): numero de version (version.rc 1.4.6; la ultima publicada es 1.4.5.2), publicar el catalogo de Nova's Mods (traducciones + sha256) con tools/novas_mods.py publicar, make_release + verify_release, push y release, video programado.

## 2026-10-17 05:40 — Segunda tanda de equipos (objetivo: IW, B2/Nave, T3, Heroe)
- **IW** (Sonnet): minijuegos de botones con las poses de IW en las dos familias (A: 0x7D4/0x7DB/0x7DC; B: 0x7DA/0x7D5/0x7E1; tablas del ELF, kQte/AliasQte en anim_iw); sin el menu azul de B3 al acabar (panel hasta el estado 8 + fundido); Entrenamiento 2 completable (VigilarEntreno2 reescrito, piloto PilotoE2). Pendiente: sonidos/efectos/camaras propias de IW en los de botones; r1_entreno2 a ciegas es estadistico.
- **Nave, prueba IV** (Sonnet): regla de B2 (modo 9: el rival no elige movimientos, guarda y devuelve toda onda; el jugador suma 1 por onda devuelta con la guardia < 5 cuadros; 3 Kili por onda) portada con el evento de B3 sub_82122488(1, 26) desde 0x82119D00 (combate.cpp OndasDevueltas/OndasModo). 97 ondas = 291 Kili en una vuelta; 23 a mano sin capsula.
- **Heroe** (Sonnet): carrusel de estilos (caras, estrella de maestro) al crear y entre combates, LB/RB + A, nivel/XP a la vista, sin espejos (4692 combinaciones comprobadas), vida proporcional al cambiar de estilo, aviso si no se puede empezar.
- **Tenkaichi 3** (Haiku): ROM del jugador (Retrobat .chd, CD MODE1 -> chdman extractcd) estudiada: voces ADX por personaje y textos de historia en 5 idiomas importables; modelos/mapas/guiones bloqueados. Ver docs/04_herramientas/TOOLS.md y E:\DBZ3HD_trabajo\t3\.
- Traducciones al dia (0 sin fr/de/it). Regresion: iso_test menu, du_test hub_filas, heroe_fix heroe, du_test tag_swap, iso_test m100, nave_fix r2_nave: PASS. Instalado (respaldo ronda4w). Clips regrabados: 04/04b heroe, 07b/07c IW, 09b nave.

## 2026-10-17 06:45 — Tercera tanda (Satan, sonidos de IW, recursos)
- **Leyenda de Mr. Satan** (Sonnet): dificultad = tabla de B1 `[dificultad][retador]` (#CPU, entrada 0x176) llevada a los 3 niveles de COM de B3 (mgr+35573 -> IA 1/4/7, medido); puntos de B1 con signo de LIFE y orden/topes del original + desglose en la pantalla de resultado; evaluacion escrita en docs/14_frentes/pasarela.md: todo con modelos/poses/camara de B3, los 11 combates en el Cell Ring = mapa 50 de B3, no hace falta ningun mapa de B1. Abierto: multiplicadores reales de A16/Satan herido/ki lento (no estan en el ejecutable de B1; quedan los propios), 8 sucesos de puntos sin medir.
- **Minijuegos de botones de IW** (Sonnet): golpes con efectos y gritos de B3 (IW no trae efectos de impacto propios; sus voces van por ids del IOP sin mapa a ADX). Camaras propias de IW sin portar (las pone un guion s_NB0xx_PRE_EXEC; los #AMC 878/879 estan en coordenadas del mundo de IW).
- **Recursos**: docs/14_frentes/assets_uso.md (que se usa, de donde sale y que queda por aprovechar).
- Regresion final: satan, iso_test menu, _r14_botones PASS. Instalado (respaldo ronda4x). Clip 05_satan regrabado con el desglose de puntos.

## 2026-10-17 08:00 — Cuarta tanda (camaras de IW, sucesos de Satan)
- **Camaras de IW en los minijuegos de botones** (Sonnet): `UiAnim_Start` reproduce clips de camara del #AMC (hijo 2 de DATA_CMN 915); portadas NB012, NB007 (familia B) y NB014, NB013, NB006 (familia A, giro sobre Y alineado al eje jugador->rival, separacion calibrada a ojo 17/7); MSN019 solo la de resultado (la de juego depende de la posicion de Videl en vuelo, sin leer). `DBZ3_IWMINI_CAMARA_B3=1` vuelve a la de B3. Plan y pendientes en docs/14_frentes/iw_camaras_minijuegos.md.
- **Sucesos de puntos de la Leyenda** (Sonnet): REFLECT, CUT-OFF, ESCAPE, DULL, RECOVERY y BURST! medidos en B3 (traza de accion/codigo de los dos luchadores) y contados como en B1; ANTI-THROW sin entrada de agarre propia medible y AROUSE (burla) sin equivalente en B3. Arreglado THROW (el jugador cobraba al ser agarrado). Mapa con evidencia en docs/14_frentes/pasarela.md.
- Regresion: satan, hub_filas, _r14_botones PASS. Instalado (respaldo ronda4y). Traducciones al dia.
