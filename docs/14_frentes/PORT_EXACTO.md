# Port exacto de los modos de B1, B2 e IW (2026-10-16)

Pedido del usuario: portear los modos con los MISMOS gráficos y el mismo funcionamiento del original (sus assets
leídos de la copia del jugador), no reconstruirlos con piezas de B3. La Leyenda de Satán actual es "reutilizado
básico". Estudios (enjambre Haiku, solo lectura) en `E:\DBZ3HD_trabajo\estudio_ports\`:
`b2_assets\INFORME.md`, `b2_reglas.md`, `iw_assets\INFORME.md`, `iw_reglas.md`, `b1_hercule.md`.
Reglas del enjambre: las de `RONDA3_ENJAMBRE.md` (compilar con `tools/compilar.py`, pruebas con `tools/pruebas.py`,
no publicar, no tocar `out/build/win-amd64-release`).

## Pieza común (hecha)
`src/pasarela.h`: `AfsPs2` (AFS de la ISO o de la carpeta extraída), `HijoAmb`, `CuantasAmt`, `TexturaAmt` (RGBA de
un #AMT 4/8 bits con CLUT), `BuscarJuego("budokai 2" | "infinite world" | ...)`. Las texturas 2D de B1, B2 e IW son
#AMT sin swizzle (decodificador de referencia en Python: `awo_tools/amt_ps2.py`). Herramientas extra (MIT) en
`PROYECTOS IA\Editor_ IA\rom_translator` (TIM2, swizzle GS, ELF/MIPS, desensamblador con rabbitizer).

## Infinite World: Dragon Mission (mapa 2D)
- Mapa del mundo: 4 texturas 640x480 de DATA_USA 334 (subentrada 3, t09-t12) = lienzo 1280x960; variantes verde
  (t22-t25) y sepia (t48-t51). Iconos de misión: `#DMD` DATA_CMN 1092 (106 casillas con posición; hitos cada 15).
  1093 (149, sepia) = probable Fighter's Road.
- Panel y botones: 334/1 (MISSION START, BEST SCORE, dificultades); títulos/textos: DATA_USA 132-144, 333/2, 334/6;
  retratos 333/0; resultados: 334/1 (TIME, RANK, SCORE, TOTAL SCORE) + dígitos, EARNED!/ZEN en 333/0.
- Misiones: tabla del ELF (USA 0x376D50, 96 B; 1-96 Dragon Mission, 97-201 Fighter's Road = 30+30+30+15).
- Minijuegos a pie (aros con rayos, Snake Way, Bubbles, Tri-Beam, ritmo, gravedad): anillos 3D DATA_CMN 880-896.

## Budokai 2: Dragon World (tablero 3D cenital)
- Fondo: mapas cenitales DATA_CMN 381 (ciudad) y 386 (hierba/ríos), 25 piezas 256x256 (5x5); adornos 3D 380/385.
- Fichas y cartas: retratos en marco hexagonal 377/382 (impares: marcos de carta con logo Z); iconos e interfaz 398.
- Tablero: `#YDW` 387-395 (casillas 52 B con x,y y 4 vecinas; piezas 40 B: personaje, bando, casilla).
- Reglas (FAQ GameFAQs): 9 fases; en tu turno mueves un personaje 1 casilla (Nut/Onion +1); buscar gasta el
  movimiento; contacto = combate 1 contra 1; ganar quita una barra al enemigo; la vida se conserva; Dende cura;
  una Bola por fase I-VII; Shenron tras la IX. Nave de Babidi: 4 pruebas que dan Kili (tienda de cápsulas).

## Budokai 1: Leyenda de Mr. Satán (lo que falta para ser port)
Puntos por golpe/combo/lanzamiento (rutina MIPS, sin valores aún), voces (solo japonés, adx_jp.afs), Cell Jr.
con el tiempo de las opciones (-2), número de rivales en +0x56, desbloqueo real, 4 imágenes sin uso (666 #3,
668 #20, #23, #25).

## Frentes de esta ronda
| Frente | Archivos | Copia |
|---|---|---|
| P1 IW Dragon Mission | `src/pasarela_iw.cpp/.h`, zona IW de `src/pasarela_misiones.cpp` | modos_test |
| P2 B2 Dragon World | nuevo `src/pasarela_b2.cpp` (registrado), zona tablero de `src/pasarela_misiones.cpp` | escenarios_test |
| P3 B1 Satán | `src/pasarela.cpp`, `src/pasarela_b1.cpp` (sin cambiar la API AfsPs2) | camp_test |

## Resultados

### P2 Dragon World de B2 (2026-10-16, 01:00)
- **Hecho:** el tablero de la Pasarela es ahora el Dragon World de B2 leído en marcha de la ISO del jugador (`AfsPs2`):
  terreno = mapa cenital de la fase (**346+5k**, no 381/386: esas son las fases VIII y IX; la cabecera del `#YDW` da el
  tamaño de su textura y cuadra con las piezas de cada 346+5k), en perspectiva inclinada a pantalla completa
  (`AddImageQuad` en 4x4 trozos por pieza), caminos verdes y discos de casilla (398 #11) en sus coordenadas `#YDW`.
  Fichas = retratos hexagonales de **342+5k** (retrato = 2 x índice de la pieza; 342 = los 13 compañeros + Raditz y
  Nappa), Saibaman con su sprite (398 #10). HUD con piezas de 398: corazones de 8 niveles (barras y vida), cifras,
  7 Bolas, zenie, "!!" al tocar, "?!" al buscar, flecha de destino, luz azul de la ficha elegida; GAME OVER de 404 #0.
- **Reglas:** preparación (arriba/abajo = las 9 fases con su saga, izq/der = compañero de entre los 13); en tu turno
  cada ficha mueve 1 (cruceta elige la vecina en esa dirección, A mueve), X busca (gasta el movimiento, una vez por
  casilla), B pasa (el circulo de B2), Y sale (el triangulo), RB cambia de ficha; tocar a un rival = combate 1 contra 1 por `RequestBattleEx` (vida que le queda
  al tuyo); ganar quita una barra; el jefe (rival con más barras) al caer acaba la fase; Bola de la fase en su casilla.
  Objetos de la tabla 8 del `#YDW` (casilla, enterrado/a la vista, tipo, valor): 9 Bola, 8 semilla, 5 cebolla (+1
  movimiento), 1 zenie (x50), 3 daga, 7 cápsula, 6 Mr. Popo.
- **Sin confirmar (lo más simple, marcado `ponytail` en el código):** significado exacto de los tipos de objeto; rivales
  se mueven cada dos turnos un paso hacia tu ficha más cercana (Saibaman y Dr. Gero quietos); perder quita una barra
  al tuyo (sin barras queda fuera; si cae Goku, GAME OVER); un solo compañero por fase; Goku no tiene retrato en 342
  (se dibuja un hexágono azul con su cara de B3); la daga, las cápsulas y Mr. Popo solo avisan; el avance guarda fase
  y Bolas (`tablero_b2_fase`, `tablero_b2_bolas`), no la partida a medias. Fases II-IX: cargan y se juegan, sin revisar.
- **Archivos:** `src/pasarela_b2.cpp` (todo el tablero); `src/pasarela_misiones.cpp` (quitado el tablero viejo: quedan
  `b2::Entrar/Dibujar/Resultado`, `CombateTablero`, aviso "necesita Budokai 2"); `tools/pruebas.py` (escenario
  `tablero`). Respaldo de antes y sondas (`scan*.py`, `t*.py`, `png/`): `E:\DBZ3HD_trabajo\P2\`.
- **Prueba:** compila (`tools/compilar.py`); `correr escenarios_test tablero` PASS:
  `D:\DBZ3HD\_pruebas\20261016-005848_escenarios_test_tablero` (preparación, tablero, movimiento + "500 zenie",
  contacto "!!", tarjeta, combate Goku contra Saibaman en las rocas).
- **Pendiente:** el resultado tras el combate (barra quitada) no se ha visto en juego; adornos 3D (345+5k, tabla 7);
  textos del tablero de B2 (`#AMK` sin descifrar: los avisos usan la letra de B3); turnos y eventos reales (`dw_*.c`).

### P3 · Leyenda de Mr. Satán (2026-10-16 01:05)
- **Puntos del original:** desensamblada la rutina de B1 (`0x1AF6F0`, 18 llamadas; valores en
  `docs/16_pasarela_modos/01_leyenda_mr_satan.md` §2.1). En combate se cuentan como B1 HIT (30 × golpe del combo, hasta
  300), DAMAGE... (−20 × golpe recibido), COMBO FIN (60 × golpes del combo del KO), SPECIAL (técnica que acierta, 450),
  SPECIAL FIN (KO con técnica, 2000) y THROW (agarre, 300); al final LIFE + TIME y la leyenda suma **la mitad de todo**
  (antes solo vida y tiempo). Aviso en pantalla con las piezas del modo: etiquetas con las letras amarillas (668 #3/#4),
  puntos con las cifras pequeñas (668 #23), franja 666 #3 y marcador del combate en el tablón pequeño 668 #20
  ("Total ... Pts."). El combo cuenta como el "N HITS" de B3 (acaba cuando el rival se recupera; los golpes parados no cuentan).
- **Reglas:** Cell Jr. con el tiempo de las opciones (-2: `G+0x2C08C`, antes quedaba el del combate anterior); número de
  rivales ya se leía de +0x56 (corregido el doc). **Voces:** la voz japonesa del original suena en cada charla (ADX_JP
  1269 + 2(r−2) y la siguiente: lo que dice y lo que piensa) y en el título (1261), decodificada en marcha y tocada por
  Windows (winmm) aparte de la mezcla del juego; respeta silencio/volumen al empezar. El SDK no admite sonido propio.
- **Archivos:** `src/pasarela.cpp`, `src/pasarela_b1.cpp`, `src/pasarela.h` (campos nuevos de B1, `Reglas::puntos_b1`,
  `Fin::puntos`, `OirVozB1`; la API común no cambia), `tools/pruebas.py` (`satan --par golpes=1`). Desensamblador:
  `E:\DBZ3HD_trabajo\P3\dis.py` y `calls.py`.
- **Prueba:** compila. `correr camp_test satan --par golpes=1` PASS: `D:\DBZ3HD\_pruebas\20261016-005402_camp_test_satan`
  (`c_puntos.png`: 2 combos de 3 = HIT +30/+60/+90, igual que el "3 HITS" de B3; combate 1180 pts; voces 19,3 s y 19,7 s
  decodificadas). `--par desde=25` PASS: `...\20261016-005603_camp_test_satan`. `desde=15`: Cell Jr. con "tiempo de las
  opciones: -1 (sin límite)" en esta copia (`...\20261016-010010_...`; la prueba falla al final solo porque espera un
  resultado y los 5 Cell Jr. tardan más de 90 s).
- **Pendiente:** ANTI-THROW, REFLECT, BURST!, CUT-OFF, RECOVERY, AROUSE, ESCAPE y DULL (sin forma de verlos sin ganchos);
  la voz no se ha oído (las pruebas van en silencio) y en Linux no suena; contraseña final (668 #25 tablón "Password":
  se genera en código y la web ya no existe; no se usa); desbloqueo real sin comprobar (fuentes en conflicto); música
  (+0x72) sin mapear.

### P1 Dragon Mission de IW (2026-10-16, 01:10)
- **Hecho:** "Misiones del Dragón (Infinite World)" (Pasarela del HUB) abre ahora el **mapa de Dragon Mission** de IW,
  todo leído en marcha de la copia del jugador (`LeerDragonMission`, API `AfsPs2`): mapa de 1280x960 (334/3) a pantalla
  completa con 640x480 px del mapa a la vista (como en la PS2) y cámara suave tras el Goku del mapa (334/5 t90), que anda
  hasta la misión elegida. Un mapa por saga (Tierra azul; Namek verde en Freezer; sepia en GT; espacio en Otra
  historia); LB/RB cambia de saga, la cruceta salta a la misión más cercana en esa dirección. Iconos en sus posiciones
  del `#DMD` con las insignias "VS" originales, aro verde (t89) en la elegida, "New!" en la abierta, estrella en las
  superadas, en gris las que B3 no puede jugar (con el motivo); los minijuegos salen con su dibujo (no se juegan).
  Abajo, el título de la misión (DATA_USA 131+n) y su frase original (334/6) en el globo de 333/0.
- **Comprobado:** el `#DMD` de Dragon Mission es el **1093** (el 1092 es Dragon Road): las 96 misiones con clave
  0x7E1+n, saga en +0x04 (corregido en `docs/16_pasarela_modos/03_infinite_world.md` §2). 334/6 = frase por fila.
- **Ficha** (334/1): panel t00 con título y texto, marco t03 con la insignia del rival en grande y su nombre, BEST
  SCORE (récord), DIFFICULTY (Very Easy..Very Hard → IA del COM), MISSION START y la regla en el idioma del juego.
  **Combate** como antes (tabla del ELF y condiciones). **Resultado** con piezas originales: TIME (m'ss"), SCORE, RANK
  (C-Z), TOTAL SCORE (suma de récords), EARNED! ... ZENI; el Zenie va al monedero del HUB (`dbz3::rpg::Premio`).
  Avance en progreso.toml: `iw_hecho` (se abren en orden hasta la siguiente jugable) e `iw_mejor_N`.
- **Archivos:** `src/pasarela_iw.cpp/.h` (lector del mapa, `PantallaDM`), `src/pasarela_misiones.cpp` (zona IW: fase
  `kMapaIW`, `DrawMapaIW`, `GanarIW`, `PasoIW`/`MotivoIW`; una línea en Salir/Siguiente/Entrar/Lanzar y en el fin del
  combate), `tools/pruebas.py` (escenario `iw_mapa`; `iw_misiones` acepta el mapa). Sondas: `E:\DBZ3HD_trabajo\P1\`
  (`dmd.py` cruza el `#DMD` con el ELF, `badges.png`, `ov_*.png`).
- **Prueba:** compila (`tools/compilar.py`). `correr modos_test iw_mapa` PASS:
  `D:\DBZ3HD\_pruebas\20261016-010112_modos_test_iw_mapa` (mapa con Goku y Raditz, ficha, combate ganado por ring out,
  resultado rango S + 300 Zenie, mapa con la 12 abierta, cursor movido, vuelta al menú) y `--par desde=30` PASS:
  `D:\DBZ3HD\_pruebas\20261016-010250_modos_test_iw_mapa` (mapa de Namek, saga de Freezer). Con el build final, otra
  vez PASS: `D:\DBZ3HD\_pruebas\20261016-010453_modos_test_iw_mapa` (`c_combate.png`: entrada de Goku en la pradera).
- **Pendiente (`ponytail` en el código):** la tabla real valor→insignia está en el `#AMA` 334/4 sin descifrar (hoy
  emparejada a ojo, `kInsignia`); fórmula de puntos y de Zenie de IW sin descifrar (vida + rapidez; +0x0E x 10);
  marcas 2-4 del `#DMD` sin usar; misiones con personajes solo de IW, entrenamientos 2-10 y minijuegos siguen sin
  jugarse; fondo del resultado original no localizado (se usa el panel t00).
- **01:20, igualado a las capturas del original que mandó el usuario:** el HUD del mapa estaba en **334/3** (además de
  los mapas; hoja `E:\DBZ3HD_trabajo\P1\h334_3.png`): robot con la cinta (t00) y el nombre de la saga en letras rojas
  (t02, t26, t36, t40, t42, t43) arriba a la izquierda; Bulma de la guía abajo a la derecha (t38, la del coche 32; por
  saga t30/t41/t39/t44, sin confirmar) y SELECT "Guía" (la enseña u oculta); barra de las 7 Bolas (t19 + t20), CLEAR
  (t69) y "hechas/total" de la saga con las cifras rosadas (t18) abajo a la izquierda; globo del mapa (t01) a lo ancho
  encima; flechas azules (t17) en los bordes hacia donde se puede desplazar. Zoom 1,7 veces mayor (cuadrícula gruesa,
  iconos grandes); Goku grande junto a la casilla (t90 a 4x con Scale2x: no hay sprite de cuerpo entero, el del juego
  es un modelo 3D). Minijuegos (+0x18 = 0) = "?" en su peana (334/5 t00), visibles y elegibles; esferas de cristal
  (+0x18 = 2, t06). Para el frente P4: `RegistrarMinijuegoIW(bool (*)(int clave, int fila))` en `pasarela_iw.h`; si no
  hay nada registrado, el mapa dice "Minijuego de Infinite World: aún no disponible". Bolas conseguidas = una por cada
  séptimo de la saga superado (ponytail). Prueba PASS: `D:\DBZ3HD\_pruebas\20261016-011921_modos_test_iw_mapa`
  (`a_mapa`, `e_mapa_despues`: "02/30", estrella y "New!").

### P2 Dragon World, segunda vuelta con las capturas del original y los mapas de la guía (2026-10-16, 01:35)
- **Como las capturas (piezas de la ISO):** cámara casi cenital (1 px del mapa = 1 px de una pantalla de 480; se
  conservan las copias en espejo del coordinador); adornos de la fase en su sitio (345+5k recortados con el `#AMA`
  344+5k, colocados por la tabla 7 del `#YDW`: torre de Karin, mesetas, naves Saiyan, Templo de Kamisama...); caminos
  finos verde claro; casillas = disco azul (398 #0), Dende = disco con aro dorado (#11), Mr. Popo en su alfombra
  (#16, #18), casilla de combate = botón morado con 闘 y en su hexágono el icono de la condición (#14 + #15/#19).
  Saibaman = cabeza verde que asoma del agujero (DATA_<idioma> 146 #13 y #16); Goku de cuerpo entero (DATA_<idioma> 155,
  el sprite de 64x88 de la intro); rivales en su hexágono rojo y compañeros en el azul, de pie sobre una peana
  hexagonal (el dorso con la Z de su ficha, plano en el suelo); tramos verdes de vida apilados (398 #9); cifra azul
  de movimientos junto al personaje activo (#6); nube con remolino sobre el rival aturdido (#3, #4); rombo con el turno
  arriba a la derecha; abajo a la izquierda radar (#26) + barra con las 7 Bolas + "DRAGON WORLD". Preparación con
  "Fase 1" y el objetivo **originales en el idioma del juego** (DATA_SP 146: "¡Obtén las Bolas de Dragón antes que
  Nappa!").
- **Mapas de la guía (PAndrews):** los 9 `#YDW` cuadran (casillas y caminos; comprobados a mano 1-5, 7 y 9). Formato
  corregido: casilla +0x10 tipo (2 Dende, 3 alfombra de Popo → casilla +0x14, 4 sitio al que lleva), +0x19 condición
  (1 media vida, 2 ki a 0, 3 vida que baja, 4 ki que baja, 5 sin bloqueo), +0x1C 0x0A casilla de combate. Objetos: 0
  sitio posible de la Bola (se elige uno al empezar; el de la fase I, sobre el suelo), 1 daga, 3 armadura, 5 cebolla
  (2 casillas por turno), 7 cápsula al azar, 8 zenie x100, 9 radar (dice hacia dónde está la Bola). Comprobación:
  `E:\DBZ3HD_trabajo\P2\comprobar_mapas.py` (`demo ok`).
- **Reglas nuevas:** Dende cura (vida llena y +1 barra); la alfombra de Popo te lleva a su casilla; ganar aturde al
  rival un turno (no se mueve) y vencerlo aturdido quita 2 barras (FAQ). Condiciones del combate con las `Reglas`
  que hay: media vida (vida inicial de los dos a la mitad), ki a 0 (el del rival), ki que baja (el jugador no gana ki).
- **Pendiente / aproximado:** los personajes del original son **modelos 3D**: en la ISO solo hay de cuerpo entero el
  Goku de la intro. Renderizarlos en CPU exige portar a C++ el lector de modelos de PS2 (geometría VIF, huesos,
  texturas) y una pose de sus animaciones: hoy solo existe en Python y fuera de línea (`awo_tools/ps2hd.py`,
  `b1port.py`, `model_render.py` para los de B3 HD). Son varias horas; queda el retrato sobre peana. Sin pieza
  encontrada en la ISO: el rombo, la barra de madera y el letrero (dibujados con sus colores y la letra de B3).
  Vida que baja y sin bloqueo solo se avisan, y el ki a 0 no vacía el del jugador: hacen falta campos nuevos en
  `Reglas` (`pasarela.h/.cpp`, frente P3). El vuelo de Popo no está animado. La tarjeta VS antes del combate sigue
  siendo la de la Pasarela.
- **Archivos:** `src/pasarela_b2.cpp`; `src/pasarela_misiones.cpp` (`CombateTablero` aplica la condición; `c[6]`).
  Respaldo previo: `E:\DBZ3HD_trabajo\P2\respaldo\pasarela_b2_antes_capturas.cpp`.
- **Prueba:** compila (`tools/compilar.py`; un reintento por un error temporal en `iw_minijuegos.cpp` de otro frente);
  `correr escenarios_test tablero` PASS: `D:\DBZ3HD\_pruebas\20261016-012908_escenarios_test_tablero` (preparación
  con textos originales en español, tablero, movimiento + "Daga +10 %", contacto, tarjeta, combate).

### P4 Minijuegos de IW (2026-10-16, 01:40; modo libre 02:10)
- **Descifrado (ELF USA con `E:\DBZ3HD_trabajo\P4\iwdis.py`, rabbitizer; el `.gzf` de Ghidra necesita la extensión
  r5900, que no está instalada):** los 18 "?" del `#DMD` 1093 (claves 0x7D0-0x7E1) llevan en **+0x1C el índice 0..18**
  de 6 tablas paralelas de 19 clases (ELF 0x472B10, 0x472B70, ...). El módulo **MSNMW** (campo a pie, 0x316FF0) carga su
  paquete de DATA_CMN por índice (0x323A40) y su `#ITM` (0x322CE0). Catálogo (fila del `#DMD` = frase de 334/6):

  | Índice (clave) | Frase (fila) | Módulo | Datos | Captura del usuario |
  |---|---|---|---|---|
  | 0 (0x7D0) | Raditz kidnapped Gohan! Go after them! (5) | MSNMW | DATA_CMN 887 (aros RING_A01, monedas) | 85 (casa, TIME) |
  | 1 (0x7D1) | Capture Bubbles under heavy gravity! Watch out for bananas! (8) | MSNMW | 888 | 83 arriba izq. |
  | 2 (0x7D2) | Snake Way is very long, but don't give up! (7) | MSNMW | 889 (10 puntos a lo largo) | 87 |
  | 3 (0x7D3) | Training with King Kai! Use Ki Blasts to destroy the bricks! (9) | `# SHOOT #` | (BRICKS DESTROYED) | |
  | 5 (0x7D4) | Go Gohan! Training with Piccolo will make you stronger! (18) | MSN_NB006 | | |
  | 6 (0x7D5) | Keep training until you reach Planet Namek! (30) | MSN_NB007 | 880 (cápsula) | |
  | 7 (0x7D6) | Go to Guru's House! Don't get caught by Vegeta! (43) | `# FLIGHT #` | 891 `#MSF` 0 | |
  | 8 (0x7D7) | Attack Frieza with Destructo-Discs! (46) | MSN009 | | |
  | 9 (0x7D8) | Fire beams at Piccolo! Go easy on him, though! (48) | MSN010 | 892 | 83 arriba der. ("130") |
  | 10 (0x7D9) | Find the spaceship! You have to get out of here, fast! (35) | MSNMW | 893 | |
  | 11 (0x7DA) | Why do you want to fight an android...? (56) | MSN_NB012 | | 88 (Vegeta, lista 1-10)? |
  | 12 (0x7DB) | This is the Time Chamber. You really love training, don't you? (58) | MSN_NB013 | | |
  | 13 (0x7DC) | I know you like training, but don't overdo it. (49) | MSN_NB014 | | |
  | 14 (0x7DD) | Gotta run from Piccolo! He'll catch up eventually, though... (62) | MSNMW | 894 | |
  | 15 (0x7DE) | Tien! Stop Cell! It's your duty! (69) | MSN016 | 895 | 83 abajo izq. (PELIGRO) |
  | 16 (0x7DF) | Use my Dragon Radar to find the Dragon Ball! (51) | MSNMW | 896 | 86 (radar 01/08) |
  | 17 (0x7E0) | Race with Goten? Well, do your best! (73) | `# FLIGHT #` | 891 `#MSF` 1 | |
  | 18 (0x7E1) | Flight training for the Tournament? Well, do your best! (83) | MSN019 | | 83 abajo der. (Videl) |

  (Los MSN0nn son índice + 1. El 4 no tiene casilla.)
  **HUD de los minijuegos = DATA_USA 6/0** (igual en 7/0): TIME y cifras lila (t03), aro plateado (t00) con la cara
  (t29 Goku, t32 Krilin, t33 Vegeta, t35 Piccolo), barra (t15 + relleno t01), cifras cian "01/08" (t12), radar (t4-t11,
  t14); PELIGRO/DANGER (6/0 t23, SAFE t22) y escala 50-350 (9/1 t7). Medidas: `E:\DBZ3HD_trabajo\P4\hud\`.
- **Reglas del MSNMW leídas del código (segunda vuelta, 02:10; sustituye a lo de antes, que eran supuestos del `#MSF`):**
  - Campo propio de IW por índice (0x296BC0): 0 → DATA_CMN 903+904 (FFPLNA, llanura con la nave de Raditz), 1 → 897+898,
    2 → 884+885, 10 → 900+901, 14 → 906+907 (FFPLNB), 16 → 909+910 (FFRCK, rocoso). Son `#MCF` + modelo + `#WCF` de
    colisión, **no** escenarios de combate: el convertidor (`tools/escenarios_convertir.py`, solo B1/B2 con `#MDD`/`#MAD`/
    `#SPX`) no los lee. Hasta portarlos se usa el escenario de B3 parecido (0: Casa del abuelo, 10: Namek, 16: rocoso).
  - Límite (0x2984D0): 5 min (0, 2, 10, 16), 4 (1), 6 (14), cuenta atrás. Salida = punto 0 del `#ITM`; Bubbles empieza
    en (-464, 0, -502).
  - Objetos por dificultad 0..5 (0x3224B0): A (aros RING_A01) {6,8,10,12,14,16} (Snake Way 10 − 2d), B (monedas)
    {16,16,24,24,32,32}, C (bebidas) {8,8,8,10,12,16}. Sitio en el `#ITM` (0x31EF90): meta +0x30, A +0x40, B +0x140,
    C +0x380, D +0x4A0. Se cogen al acercarse: A y meta d² < 600, B y C d² < 180 (unidades de IW).
  - Meta: se abre al coger todos los aros (0x31F520; en el 10 aparece la nave). Total (0x3237D0) = A+B+C+D+2. Rango
    (0x322640) por cogidos/total: ≥0,95 Z, ≥0,8 S, ≥0,65 A, ≥0,5 B, si no C. Puntos (0x322E60): {200,300,500,750,1000}
    × (d+1)² + segundos que sobran × (d+1) × 10, redondeado a la centena (+ especiales: bola, 200·(d+1) .. 10000).
- **Hecho, modo libre (rehecho tras la queja "era free roam"):** `src/iw_minijuegos.cpp`. Un solo luchador: el otro es
  el muñeco quieto de `combate.cpp`, **invisible** (X+3261 y X+4925, como el relevo del Tag Team) e invulnerable, y hace
  de "pivote": se pone 40 unidades delante del jugador en la dirección de la palanca relativa a la cámara, y la palanca
  del jugador se cambia por "hacia el rival" (registro del mando +108/+26; antes de leer se le devuelve lo que dejó el
  juego). Resultado: el luchador **corre libre hacia donde se empuja**, con la **cámara de tercera persona detrás**
  (mgr+0x60 con `cine::Camara` cada fotograma, girando poco a poco tras la marcha), sin tiempo de combate ni KO.
  Aros, monedas y bebidas del `#ITM` de 887 con sus texturas originales proyectados en 3D; la meta (círculo en el
  suelo) sale al coger los 10 aros; TIME en cuenta atrás con las letras de IW y aro con la cara + barra abajo a la
  izquierda. Al acabar, fundido a negro con **CLEAR** del juego (334/3 t69) que tapa el final de combate de B3, y el
  resultado (tiempo, rango, puntos) va al mapa por `TomarResultado` (ya conectado por el coordinador). Hechos así el
  0 (Raditz), el 2 (Snake Way) y el 10 (nave de Namek).
- **Archivos:** `src/iw_minijuegos.cpp/.h`; una línea en `src/clima.cpp` (gancho sub_820C2058: `iwmini::Fotograma`) y
  dos en `src/combate.cpp` (gancho sub_8208FD58, antes y después de leer el mando: `iwmini::Mando`); `tools/pruebas.py`
  (escenario `iw_minijuego`). Respaldo de la versión de combate: `E:\DBZ3HD_trabajo\P4\respaldo\`.
- **Prueba:** compila. `correr combate_test iw_minijuego` PASS (piloto automático de aro en aro):
  `D:\DBZ3HD\_pruebas\20261016-021111_combate_test_iw_minijuego` (10/10 aros, meta, CLEAR). `--par auto=0` (teclas
  Up/Right/W) PASS: `D:\DBZ3HD\_pruebas\20261016-020957_combate_test_iw_minijuego` (Goku corre hacia delante y gira;
  `d_adelante2.png`: cámara detrás como en las capturas 85/87) y `...-020723_...`. `--par clave=0x7D9` (nave) PASS:
  `D:\DBZ3HD\_pruebas\20261016-020820_combate_test_iw_minijuego`.
- **Pendiente:** el **marcador de combate de B3 sigue visible** (barras, V 0, ∞): no hay forma conocida de quitarlo
  (no está en src ni en docs; la función que lo dibuja va por tabla de tareas: candidatas en el módulo 0x820DB000-
  0x820E9000 que solo se llaman por puntero, listadas con `E:\DBZ3HD_trabajo\P4\hudroots.py`); el **campo de IW**
  (903/884/900...) necesita un convertidor nuevo (`#MCF`+`#AMO`+`#WCF`); dificultad fija en Normal (el mapa no la pasa);
  escala del curso a mano (radio 85); los objetos se cogen en el plano (B3 apenas salta); la barra del HUD va llena;
  Bubbles (1), huir de Piccolo (14) y el Radar (16) con sus reglas propias; el resto de módulos (SHOOT, FLIGHT,
  MSN009/010/016/019, NB006-014).
- **Ronda 4 (02:25): marcador de B3 oculto, por RE.** El objeto del marcador (*0x82324064) registra al crearse
  (sub_820DCBC0) las tareas "FIX MAIN" (sub_820DCA10), "FIX PREDRAW" (sub_820DC080), **"FIX DRAW"** (sub_820DC1A8;
  sub_820DC230 en el modo 6) y "FIX POSTDRAW" (sub_820DBB00) (nombres leídos de la imagen del juego,
  `out/analysis/guest_image/dbz3_us_image.bin`, script `E:\DBZ3HD_trabajo\P4\b3tasks.py`). FIX DRAW pinta barras, caras,
  "V", reloj y avisos solo si mgr+35253 != 1 y 0x823CD870 tiene el bit alto: así lo apaga el juego en la presentación.
  **Función compartida: `dbz3::combate::OcultarHud(bool)`** (`src/combate.h`): salta esa tarea mientras está a true
  (ganchos `DBZ3_HOOK_EU(sub_820DC1A8/sub_820DC230)`, parejas EU regeneradas con `tools/eu_map.py`). Los minijuegos la
  ponen al empezar y la quitan al acabar. Comprobado: `D:\DBZ3HD\_pruebas\20261016-022453_combate_test_iw_minijuego`
  (`b_aros.png`: sin barras ni ∞, solo el HUD de IW). Sondas descartadas (volcado presentación/pelea con
  `DBZ3_SONDA_HUD`, `DBZ3_SONDA_HUD_PON`): hud+0x21E/+0x9AE y mgr+0x220 cambian pero no lo ocultan.
- **Ronda 4 (02:45), corrección del usuario ("port nativo de IW"):**
  - **Modelos 3D de IW** en vez de dibujos planos: lector de `#AMO` de PS2 en C++ (el mismo formato que
    `awo_tools/amo2awo.py`: huesos con cuaternión + traslación + escala, partes en tiras VIF) y pintado con la cámara
    del combate, con sus texturas originales. Aros = `RING_A01` + barrera translúcida (887/1 + 887/0), monedas
    `ITEMA01_COIN` (887/5, variante oro), bebidas `ITEMA06` (887/9). **Meta del 0 = la nave de Raditz del campo**
    (DATA_CMN 903/7 + 903/8: `C2_SAS`, `PSAS01`, `SASBALL01`, `PODKAGE`), en el punto de meta del `#ITM`; Raditz ya no se
    ve en ningún momento (el luchador pivote es invisible). Los modelos van a su tamaño de IW (mismo motor: el aro mide
    24 de radio, la nave 13) y los radios de recogida son los del código (24,5 y 13,4).
  - **Escala:** en un escenario de B3 se corre hasta ~255 del centro (medido con `--par lejos=1`) y el campo de IW mide
    ~3000: las posiciones del `#ITM` se juntan (×0,16, curso a radio 230); los modelos no. Lo arregla portar el campo.
  - Lo que queda detrás del jugador no se pinta encima de él (su caja en pantalla). ponytail: sin z-buffer del juego, el
    escenario de B3 no tapa los modelos.
  - **TIME en su sitio de IW** (arriba a la derecha, piezas de 6/0) ahora que el marcador de B3 no se ve.
  - **Dificultad:** `dbz3::iwmini::Dificultad(nivel_com)` y una línea en `pasarela_misiones.cpp` (antes de
    `pantalla_.Mapa`) con `pantalla_.NivelCom()`. ponytail: la ficha da 5 niveles y `NivelCom` los junta en 3 (0→Easy,
    1→Normal, 2→Hard del MSNMW); el valor exacto necesita un `Dificultad()` en `PantallaDM` (R1).
- **Campos de IW (estudio, no portados aún):** `amo_a_awo` de `awo_tools` convierte el terreno 903/5 (473 huesos, 1
  grupo, sin piel) a `#AWO` en 0,5 s. `#WCF` 904/0 = colisión (589 registros de 0x90 B con nombres `FFPLNACOL_ROOT`,
  losetas A1..J10 `JIMEN01-04` enlazadas por nombre con los huesos `FFPLNA02_<casilla>JIMEN` del terreno; esfera
  -600,100,-1200 r 424); `#MCF` 903/0 = distancias de niebla/dibujo (1800, 800...), color de cielo y velocidades de
  nubes/agua; `#AMC` 904/1 = 3 pistas de cámara. El convertidor (`tools/escenarios_convertir.py`) solo sabe partir de
  un `#MDD` (grafo) y de colisión en `#AMO`: faltan el grafo `#ZDD` sin `#MDD` y la colisión `#WCF` → HD. Estimación:
  fase 1 (malla, texturas, cámaras, suelo plano) 3-4 días; fase 2 (colisión real del `#WCF`) 4-6 días más.
- **Decomp de Ghidra (EU, SLES_553.47) disponible** en `E:\DBZ3HD_trabajo\ghidra\salida\iw_sles55347.c` (10055
  funciones; con nombres como `GroundMinigameCharacterRotationWhileMoving`, `MiniGame_CalcRank`, `MapLoader`,
  `Coll_GetGroundHeightAtXZ`, `Stage_DrawTiledGround`): siguiente paso para confirmar las reglas del MSNMW y leer el
  `#WCF`. Web (`E:\DBZ3HD_trabajo\P4\web_minijuegos.md`): el 0 son aros hexagonales azules giratorios con marco de
  metal, monedas flotantes, rayos y tiempo; Snake Way con puntos de control y rango Z (0'57"66 en un foro); GameFAQs,
  fandom y GameSpot bloqueados.
- **Archivos (ronda 4):** `src/iw_minijuegos.cpp/.h`; `src/combate.cpp/.h` (`OcultarHud` y sus 2 ganchos);
  `src/eu_map_gen.h` y `docs/14_frentes/eu_mapa.txt` regenerados con `tools/eu_map.py`; una línea en
  `src/pasarela_misiones.cpp`; `tools/pruebas.py` (`--par meta=1`, `--par lejos=1`).
- **Prueba:** compila. `correr combate_test iw_minijuego` PASS: `D:\DBZ3HD\_pruebas\20261016-024357_combate_test_iw_minijuego`
  (`b_aros.png`: aro RING_A01 y monedas 3D de IW, sin marcador de B3, TIME arriba); `--par auto=0 --par meta=1` PASS:
  `D:\DBZ3HD\_pruebas\20261016-024022_combate_test_iw_minijuego` (`b_nave.png`: la nave de Raditz detrás de Goku;
  `c_nave_cerca.png`: CLEAR al llegar).
- **Pendiente:** portar el campo de IW (lo principal ahora); Bubbles (1), huir de Piccolo (14) y el Radar (16) con su
  HUD "01/08"; animación real de aros y monedas (`#AMM` 887/2 y 887/6); objetos C y especiales (bola, radar) del
  `#ITM`; tapar con el escenario.
- **Ronda 4, segunda vuelta (03:05):**
  - **Profundidad frente al luchador (RE):** el luchador se tapa con sus **22 esferas de cuerpo** (las que el juego usa
    para los golpes: centros X+1616 + 16 i, radios [[X+2996] + 24 + 4 i], documentado en `gigantes.cpp`), proyectadas
    con la cámara del combate; un triángulo de un modelo de IW más lejano que una esfera y con su centro dentro de ella no
    se pinta. Comprobado: `D:\DBZ3HD\_pruebas\20261016-025243_combate_test_iw_minijuego\b_nave.png` (la nave queda detrás
    de Goku). Meterlos en la pasada 3D del juego con su z-buffer exige crear buffers y llamadas de dibujo del juego
    (como sus efectos): no hecho; el escenario de B3 sigue sin tapar los modelos.
  - **Giro real de aros y monedas** de sus `#AMM` (887/2 y 887/6): pista de claves (valor en 1/65536 de vuelta, cuadro);
    el aro da media vuelta cada 120 cuadros con la velocidad cambiando (cierre 32769), la moneda una vuelta cada 100.
    Todos a la vez, como el cuadro común de `MgDraw_*` (decomp EU 0x320F30).
  - **Radar del Dragón (16) hecho:** marcas D (`#ITM` +0x4A0, 2d+6 = 10 en Normal, como mucho el +0x10; decomp EU
    `GroundMinigameObjectsAroundTheMap` 0x3227A0 = las mismas tablas que leí del ELF USA) con su modelo ITEMA20 (896/1),
    la bola de dragón DB_BLV_BALL (896/16) en la meta (se abre al coger las marcas), escenario rocoso de B3. HUD del
    original (6/0): marco con asa t04, pantalla rosa t10 + cuadrícula t05, rombos rosas t14 (marcas que faltan), punto
    naranja t08 (la bola), triángulo t07 (el jugador, girado con la cámara), flecha t09 y "NN/MM" con las cifras cian
    t12; alcance del código d² < 2.89e6. Prueba: `D:\DBZ3HD\_pruebas\20261016-030133_combate_test_iw_minijuego`
    (`--par clave=0x7DF`: `b_aros.png` y `c_aros.png` con el radar y "09/10"; las 10 marcas y la bola, CLEAR).
  - Decomp EU leída para confirmar: `s_SPACE` (0x31F280, el `[ITM] POST EXEC`) = mismas distancias (600, 180, 2.89e6,
    120000), meta (punto 1) con `LoadWaitGroundMinigameObjective`, mitad de aros → +0x16C0/+0x1D8, todos → +0x1DC.
  - `tools/eu_map.py` vuelto a pasar (R6 usaba 0x823CD870 en `combate.cpp` sin pareja EU y no compilaba).
- **Pendiente:** Bubbles (1: Bubbles que huye con su IA `MgPly_Calc*` del decomp, plátanos 888/9, gravedad) y huir de
  Piccolo (14) con sus reglas; el resto de objetos especiales (bola de dragón aleatoria G+0x1630 del 0, radar
  DBLADER 16A4); pasada 3D con z-buffer; el campo de IW (frente R8).
- **Ronda 4, tercera vuelta (03:15): animación de carrera (RE, sin portar aún).**
  - **B3 no tiene carrera:** registrando la acción (X+3244) y el código (X+2928 & 0xFFF) del jugador en el minijuego:
    andar = acción 2/3, códigos 3/4 (los pone `sub_82103C48`, "entrar a andar": X+3244 = 2, X+2928 = 3 (0x103 en el
    aire) y arranca la animación con **`sub_82114FB8(X)`**, igual que el `BonesMovement` de IW); el doble toque da la
    acción 15, código 0x226 (paso rápido, ~11 unidades) y vuelve a andar. No hay ciclo de carrera que encadenar.
  - **IW:** el jugador del minijuego corre en el estado 9 (`MgPly_Enter_State09`, decomp EU 0x318910): código de
    animación **0xE2** y velocidad **4.5 por cuadro** (+0xB4). El 0xE2 no está en el `#BSK` propio de Goku (DATA_CMN 477:
    `#BSK` + 3 `#AMM`; su código 3 tampoco): como en B3, andar y correr vienen del banco común de animaciones
    (tabla +0x46C del personaje en `MiniGokuAnimations`, 0x31BFA0). Moveset de IW Goku = DATA_CMN 477 (labios 474;
    el importador lo cuenta como "ID -1": su tabla empieza un registro tarde, a revisar en `mod center hd/importar.py`).
  - **Plan para portarla (siguiente paso):** sacar la animación 0xE2 del banco común de IW (#AMM de PS2 = #ACM de HD
    con otro orden de bytes, `ACM_FORMAT.md` §5), copiarla a memoria del juego (`dbz3::roster::GuestAlloc`) y, solo en
    el minijuego, en un gancho a `sub_82103C48` poner X+2928 al código cuyo bloque del `#CSK` cargado se haya apuntado a
    ella y llamar a `sub_82114FB8`; devolverlo al acabar. Falta localizar el banco común de B3 en memoria y comprobar
    que los esqueletos (50 huesos) casan.
  - **Hecho ya:** la velocidad de la carrera de IW (4.5 por cuadro × la escala del curso; se suma a lo que anda B3,
    medido ~0.33): Goku cruza el curso como en IW (~39 unidades/s medido). El doble toque se quitó (`DBZ3_IWMINI_DOBLE=1`
    lo deja para pruebas). Prueba: `D:\DBZ3HD\_pruebas\20261016-031323_combate_test_iw_minijuego` (`--par auto=0`).
  - **Pendiente:** portar la animación 0xE2; Bubbles (1) y huir de Piccolo (14).
- **Ronda 4, cuarta vuelta (04:00-04:30): Bubbles (1), huir de Piccolo (14), scatto del jugador y campo de R8.**
  - **IA del NPC, del decomp** (EU `s_BBR_POST_EXEC` 0x3172E0 y `MgPly_Calc*Vel` 0x316B90-0x3170D0): cada decisión
    elige al azar el modo 1..3 con su duración (1: azar % (20d + 100) + 35 cuadros, 2: azar % 100 + 100 y a veces
    marca +0x62, 3: azar % (20d + 100) + 60); velocidad = alejarse del jugador desviada hacia un lado azar % 10 × 0,1.
    Bubbles y Piccolo van en el estado 9 (código 0xE2, +0xB4 = 4,5): Piccolo es el luchador 1 del minijuego 0xE
    (`Minigame_MovePlayerByStick` EU 0x299EE0) y va derecho hacia el jugador.
  - **Bubbles = el modelo de IW `BBR_*`** (cola `BBR_T_TAIL1-5`): DATA_CMN 118 (+ texturas 119), 7,3 de alto; se le
    atrapa al tocarlo (CLEAR). Plátanos `BANANA_A02` (888/9) en los puntos del grupo D del 888 (no cuentan en el total)
    y los que suelta en el modo 2; al pisar uno, un segundo sin mando. **Piccolo = `PIC_*`** (573 + 574) persigue a
    Gohan niño (ID 2 de B3); al alcanzarte acaba con el rango de lo cogido.
  - **Velocidades en proporción a IW:** el andar de IW (estados 1-4, +0xB4 = 4,0) es el andar de B3 (0,33 por cuadro,
    medido), así que cada velocidad de IW va × 0,33 / 4: IA 4,5 → 0,37 (un poco más que andar: hay que correr para
    alcanzar a Bubbles o escapar de Piccolo), scatto 35 → 2,9 por cuadro. Quitado el extra de 4,5 que llevaba el
    jugador (era la velocidad de la IA, no la suya: hallazgo de R10).
  - **Scatto del jugador (botón círculo = B del mando de Xbox), del decomp:** `Minigame_ReadPlayerButtons` (EU
    0x299520) pide el scatto con el círculo recién pulsado si la energía (+0x185C) es > 0 y no está agotado (+0x1C0).
    Fases: 0x14 arranque (anim 0x19, 12 cuadros, no se mueve), 0x15 carrera (0x1A, 60 cuadros, a 35 por la palanca o,
    sin palanca, hacia delante de la cámara), 0x16 frenada (0x1B, 22 cuadros, no se mueve). Energía −0,5 por cuadro en
    las tres (≈ 47 por scatto), +0,02 por cuadro siempre (`ExplorationMinigameBodyAnimationsMerge`), tope 100; con ≤ 0
    en la frenada queda agotado (energía 1) hasta coger una bebida. **Bebidas** (grupo C): suman la energía de su
    `#ITM` (+0x480 + 2k, s16) y quitan el agotado (EU 0x3237D0). FOV: 35 → 50 (+2,5/cuadro), baja a 40 (−0,38) en la
    carrera y a 35 en la frenada (cámara +0x120); en B3 en proporción sobre mgr+0x1C4. La barra amarilla del HUD es
    esa energía. Animaciones con `dbz3::anim::Scatto` de R10 (kInicio → kBucle → kFin). El círculo no le llega al
    luchador de B3 (sería su ráfaga de ki).
  - **Campo de IW de R8:** `EmpezarCombate` usa `escenarios::CampoIW(f->campo)` (alta 5060+n) si está instalado y si no
    el escenario de B3. `Campo IW 903` copiado a la copia `combate_test`. Los objetos siguen × 0,16 mientras la
    colisión sea la caja de Llanuras (R8 dirá el factor nuevo).
  - **Pruebas:** `correr combate_test iw_minijuego --par clave=0x7D0 --par auto=0 --par scatto=1` PASS
    (`D:\DBZ3HD\_pruebas\20261016-040841_combate_test_iw_minijuego`): escenario 17 = "Llanura de Raditz (IW)" (campo
    903 de R8, con su camino, mesetas y árboles de IW); la tecla del círculo (Quote = B) llega como 0x2000; tres
    scatto seguidos con la energía 100 → 55,9 → 11,8 → agotado, y el cuarto no sale; `b_scatto1..3`: Goku en la
    postura de carrera de IW, campo de visión abierto y la barra bajando. Bubbles (0x7D1, `npc=1`, piloto que ahora
    coge también las bebidas) PASS `...-041732_...`: bebidas +100/+50 de energía (las del #ITM), Bubbles huye (modos
    1/2) y atrapado → CLEAR; su campo (897) aún no está instalado → Llanuras de B3. Piccolo (0x7DD, `npc=1`) PASS
    `...-034532_...`: va detrás del jugador (por eso no sale en la captura con la cámara de espaldas).
  - **Para R10:** en el scatto Goku corre **de lado** (90° respecto a la marcha; en la prueba de R10 también,
    `...-040424_anim_test_r10_scatto\c_bucle2.png`), mientras que con el andar de B3 mira hacia delante: la raíz de la
    animación de IW no casa con la orientación del luchador de B3. **Para R8:** el cielo del campo sale negro.
  - **"Gravedad fuerte" de Bubbles (planeta de Kaio), del decomp:** en el minijuego tipo 1 las velocidades son las
    mismas (andar +0xB4 = 4,0, salto +0xB0 = 5,0); solo cambian los códigos de animación: quieto 0xFB (en vez de 1),
    andar 0xFE/0xFF (0x15/0x16), 0x101 (0x18), salto 0x103 (0x20), caída 0x105 (0x22) (`Char_EnterState1..4`,
    `Minigame_State0D_JumpRiseEnter`, `State0E_AirUpdate`). Es de animación: queda para R10 (mismo #BSK común de IW).
  - Cara del HUD en el 14: IW no trae Gohan niño (6/0 t29..t38 = Goku, SSJ, Gohan SSJ2, Krilin, Vegeta, Trunks,
    Piccolo, Gohan, Ten, Cell): va la de Gohan (t36).
  - ponytail: el factor por personaje +0xCAC se toma 1; el estado 7 de agotado (su animación) no se hace; el salto del
    modo 2 de Bubbles (25 unidades) tampoco; Bubbles y Piccolo se pintan en la pose del modelo (su animación 0xE2 es la
    12 del AMM de Bubbles, DATA_CMN 912, sin esqueleto en el pintado de P4); "gravedad fuerte" sin leer; lo que puntúa
    atraparle tampoco. Fuera del scatto IW repone el FOV en otro sitio (aquí baja a 35 a −0,38).
- **Ronda 4, quinta vuelta (04:30-05:00): Piccolo de B3, Bubbles animado, API final de R10.**
  - **Piccolo (14) es el Piccolo de B3** (ID 11, el rival del combate, visible): su sitio lo lleva la IA del panel
    (estado 9, 4,5 → 0,37 por cuadro hacia el jugador) y anda con el andar de B3 porque se le da el mando 1 (sin muñeco:
    `SetMuneco(0)`; X+5028 = 1 y `Mando` le inyecta, después de leerlo, conectado y "hacia el rival"). Empieza 150 por
    detrás de la salida (ponytail: su sitio de IW no está leído). El jugador no mira al rival: **RE de hacia dónde mira el
    luchador** con una sonda (el pivote girando alrededor del jugador quieto, 300 volcados del luchador; script
    `E:\DBZ3HD_trabajo\P4\giro_analiza.py`): **X+6062 (u16) = ángulo − 15759 y X+6074 = ángulo − 15968**, ángulo =
    atan2(dx, dz) hacia el rival en 1/65536 de vuelta, rehecho cada cuadro. En el 14 se escribe con la dirección de la
    marcha después del juego, y la marcha del jugador es la de IW (andar de B3 0,33 hacia la palanca, desde donde quedó).
  - **Ver a Piccolo:** la palanca derecha (o el ratón con mouse look; flechas en el teclado de pruebas) gira la cámara
    alrededor del jugador, como `Camera_UpdateStickOrbit` (EU 0x297330: zona muerta 0x20, (v − 0x20) / 95 × 0,04).
  - **Bubbles animado con su carrera de IW:** código 0xE2 del #BSK 912/0 → animación 12 del #AMM 912/1 (36 cuadros,
    47 huesos `BBR_*` en el orden del #AMO 118). Ángulos u16 en 1/65536 de vuelta con **R = Rz·Ry·Rx** (comprobado: el
    reposo de `BBR_LLEGROT`, cuaternión (0,707, 0,707, 0, 0), sale exacto con (180°, 0, 90°)); la pista posicional de
    `BBR_WAIST` da la altura. Pieles en CPU: cada vértice × mundo_animado · inversa(mundo_reposo) de su hueso. Las piezas de
    cada hueso (cabeza, manos, pies, cola) van a su hueso; **el cuerpo (grupo del hueso 0, en mundo) va pesado y sus pesos
    no están leídos**: ponytail, cada vértice al segmento padre→hijo más cercano en reposo (prototipo
    `E:\DBZ3HD_trabajo\P4\bubbles_pose.py` → `bubbles_pose.png`: corre con brazos y piernas bien). Portar los pesos del
    #AMO cuando se lean.
  - **API final de R10:** `Scatto(x0, kInicio)` al pulsar, `kBucle` a los 12 cuadros, `kFin` a los 60 de carrera, y
    `Scatto(mgr + kX0, kApagado)` al acabar junto a `OcultarHud(false)`. Sin el gancho automático, la palanca anda con el
    andar de B3. Diferencias con lo que leyó R10 (las dejo, del decomp): el scatto empieza al **pulsar** el círculo y dura
    la animación 0x1A (60 cuadros: `State15_DashUpdate` solo pasa a 0x16 al acabarla, sin mirar el botón; el botón del
    salto doble obliga a que esa palabra sea "recién pulsado"), no "mientras se mantiene"; y la velocidad va en
    proporción al andar (35 / 4,0 × 0,33 = 2,9 por cuadro) en vez de 35 × 0,4 × escala (2,2), porque el andar de B3 es
    fijo (0,33) y así se guarda andar : scatto = 1 : 8,75 como en IW. El vaciado (0,5 por cuadro) y el FOV coinciden.
  - **Mando de Piccolo:** con el mando 1 "a mano" el juego pausa ("El mando del jugador 2 ha sido desconectado",
    `...-043549_...\b_npc.png`); va con el muñeco de combate.cpp (`SetMuneco(1)`, que lo da por conectado) y **una línea
    en combate.cpp**: tras `MunecoMando`, `dbz3::iwmini::Mando(base, user, true)` para inyectarle el andar. La IA no se
    mueve en la pausa. La palanca derecha ya no le llega al luchador de B3 (en la HD lanza la técnica: salió un
    Kamehameha en `...-043829_...\d_mirar.png`); se guarda y se repone como las direcciones.
  - **Campos de IW de R8 en sus coordenadas:** con el campo instalado (`CampoIW(f->campo) >= 0`) los objetos del #ITM van
    en su sitio de IW (escala 1, sin centrar, con su altura), la salida es el punto 0 y las velocidades en unidades de IW
    (k = 0,4: andar 1,6 por cuadro, al andar de B3 se le añade lo que falta; IA 1,8; scatto 14). Sin campo, lo de antes
    (juntado al escenario de B3, k = 0,33 / 4). La IA no pasa de ±1476 por eje en el campo. Copiados a `combate_test` los 6
    campos de `E:\DBZ3HD_trabajo\R8\campos\` (el 903 viejo, a `D:\DBZ3HD\respaldos\P4_combate_test_Campo_IW_903_0345`).
  - **Pruebas (compila; capturas miradas):** Piccolo `...-044419_...` (campo 906, escena 21): sin pausa del mando 2, le
    persigue y "Piccolo te alcanza"; pero al andar 3 s hacia delante desde (0, 0) el jugador entra en una meseta y la
    cámara queda dentro de la roca (`c_npc.png`): el campo de R8 no tiene paredes para el luchador de B3. Campo 903
    `...-044656_...`: salida (108, −432), tres scatto hasta agotar; la mitad baja de la vista sale negra (cielo/suelo del
    campo, R8). Camino de la Serpiente (2, campo 884) `...-044851_...`: carga, 6 aros cogidos (la prueba esperaba 8:
    en dificultad 2 son 10 − 2·2 = 6, fallo de la prueba, no del juego), pero el jugador queda abajo, en las nubes:
    el piloto ponía y = 0 (arreglado: en el campo va a la altura del objeto; compilado, sin volver a probar). Bubbles
    animado: `...-043829_...` carga la carrera (36 cuadros) pero en las capturas queda tapado por Goku o la cámara
    gira: sin captura buena de la animación en el juego (el prototipo sí: `bubbles_pose.png`).
  - **Sin hacer (ponytail):** la animación de cansado (estado 7 de IW: es de animación, para R10) y el salto del modo 2
    de Bubbles (su altura y gravedad no están leídas: no se inventa).
- **Ronda 4, sexta vuelta (05:00-05:15): pesos reales de Bubbles, Camino de la Serpiente, cruceta.**
  - **Pesos de vértice del #AMO de IW leídos** (Bubbles, DATA_CMN 118): cada hueso con malla pesada tiene en su eje
    +0x34 un registro `[hueso, 0, lista]`; la lista (desde el #AMG) es `[0, 0, 0, n]` + n × `[peso f32, vértices,
    desplazamiento, 0]`, y en el desplazamiento, por vértice, 32 B `[x y z en el espacio del hueso, destino u32,
    normal, 0]`, con **destino = dirección del vértice desde el #AMG** (el skinning por CPU de PS2 escribe ahí). Comprobado:
    las 2382 posiciones pesadas en reposo caen exactas sobre vértices del cuerpo (`E:\DBZ3HD_trabajo\P4\pesos_amo2.py`);
    los otros 1106 vértices del grupo del hueso 0 no llevan peso (rígidos al cuerpo). Posición = Σ peso × eje_animado ×
    local / Σ peso (algunas sumas no dan 1: se normaliza). Quitada la aproximación del hueso más cercano. Prototipo con
    los pesos reales: `bubbles_pose.png` (cuerpo liso, brazos y piernas siguen la carrera).
  - **Cruceta:** la flecha derecha del teclado de pruebas es la cruceta (+108 = 0x2000), y en la HD la cruceta lanza
    técnicas (salió un Kamehameha: acción 15, código 0x253/0x24A). Se le quitan al luchador de B3 los bits de la cruceta
    de +12/+14 (0x10-0x80); para andar sigue valiendo (+108). La palanca derecha gira la cámara (no está en las flechas).
  - **Bubbles en el juego (campo 897 de Kaio instalado):** carga su carrera (36 cuadros) y huye, pero sigue sin captura
    donde se le vea: en coordenadas de IW sale de (100, 100) a 141 del jugador y corre a 1,8 por cuadro (más que andar),
    y aun quieto (`--env DBZ3_IWMINI_NPC_QUIETO=1`, solo pruebas) y acercándose no aparece en
    `D:\DBZ3HD\_pruebas\20261016-051203_combate_test_iw_minijuego` (`b_bubbles1/2`, `b_npc`). Sospecha: su altura (la del
    punto 1 del #ITM 888, que con (100, 100) parece de relleno) lo deja bajo el terreno o muy alto; queda para cuando esté
    `suelo_iw::Altura` (ponerlo a la altura del suelo). La piel con los pesos reales está comprobada fuera del juego.
    Además, el planeta de Kaio da la vuelta (IW `Node_WrapPositionInGrid`): aquí solo se le para en ±1476 (ponytail).
  - **Camino de la Serpiente (2, campo 884):** la prueba ya espera los aros que da el #ITM (lee "A n ... D m" del
    registro: en dificultad 2 son 6) y pasa: `D:\DBZ3HD\_pruebas\20261016-045804_combate_test_iw_minijuego` (6 aros y
    META). A ojo: el jugador no se ve, cae a las nubes por debajo del camino (el suelo del campo no lo sostiene): es del
    frente R11 (consulta de suelo `sub_820CB0A0` y paredes), no se toca aquí.
  - **Para R10 (petición):** `anim::Scatto` lleva un solo luchador (`g_x`). Para que Piccolo (luchador 2, mgr + 2096 +
    8240) corra con la carrera de la IA de IW (código 0xE2: solo el #BSK de Bubbles, 912, lo define) hace falta una
    segunda ranura o una función aparte, p. ej. `anim::Corredor(uint32_t x, bool on)` que le ponga en bucle la 0xE2 (o
    la 0x1A del scatto) mientras on. P4 la llamaría en `Fotograma` con x1 cuando `d->npc == 2` (ahora anda con el andar
    de B3 por el mando del muñeco).
  - **Para R11:** su `src/suelo_iw.h` ya está (`Altura(x, z, sin_suelo)`), pero aún sin `suelo_iw.cpp` ni entrada en
    CMakeLists: usarlo ahora rompería el enlazado. Cuando esté, en `Fotograma`: `ojo[1] = std::max(ojo[1],
    dbz3::suelo_iw::Altura(ojo[0], ojo[2], ojo[1]) + 8.0f);` y Bubbles, plátanos y objetos a esa altura.
  - Compila (05:11). La cruceta quitada no está probada con la flecha (la prueba de Bubbles usa W/D, la palanca).
- **Ronda 4, séptima vuelta (05:15-05:35): módulo compartido `src/ps2_modelo.{h,cpp}` (para R2 y R11).**
  - El lector y el pintor de modelos de PS2 salen de `iw_minijuegos.cpp` (que ahora los usa; −450 líneas) a
    `dbz3::ps2`, en las dos listas de `CMakeLists.txt` (fuentes y la exclusión de la EU sola, como `anim_iw.cpp`). Sin
    cambios de comportamiento: regresión PASS y mirada a ojo, `D:\DBZ3HD\_pruebas\20261016-051917_*` (0, campo 903),
    `-052214_*` (Radar 0x7DF, campo 909: radar, marcas "04/10" y la bola) y `-052621_*` (Piccolo 0x7DD, campo 906).
  - **API** (`src/ps2_modelo.h`; ejemplo completo en su cabecera):
    - Carga (cualquier hilo): `ps2::CargarDeAfs(afs, amo, amt, animable)` (#AMO y #AMT sueltos de un `AfsPs2`, p. ej.
      Bubbles 118/119), `ps2::CargarModelo(amb, hijo_amo, hijo_amt, variante)` (dentro de un #AMB; lee también el #AMM de
      giro en hijo_amo + 1), `ps2::CargarSuelto(amo, amt, animable)` (bytes ya leídos). Devuelven `ps2::Modelo`: `malla`
      (triángulos con uv, textura y hueso; centrada: x/z al centro, y mínima 0), `tex` (RGBA del #AMT), `giro`, y con
      `animable` el `esq` (huesos, ejes de reposo, **pesos reales**) para animar.
    - Animación: `m.anim = ps2::LeerAnim(amm, n, m.esq->padre.size())` (animación n de un #AMM de PS2: ángulos u16 en
      1/65536 de vuelta, R = Rz·Ry·Rx, más la pista posicional) y cada cuadro `ps2::Posar(*m.esq, m.anim, t_segundos,
      malla_salida)` (bucle a 60 cuadros por segundo; piezas rígidas a su hueso, cuerpo con sus pesos).
    - Pintado (hilo de la interfaz, ImGui): `auto tx = ps2::Texturas(m)` una vez; cada cuadro
      `ps2::Camara cam = ps2::CamaraCombate(base, io.DisplaySize)` (vista mgr+0x60 y fov mgr+0x1C4 del combate),
      `escena.Empezar()`, `escena.Poner(cam, malla, tx, {x, y, z}, giro, escala)` por modelo, `escena.Tapar(cam, base,
      mgr + 2096)` (y `+ 8240` para el otro luchador: sus 22 esferas tapan lo que queda detrás) y
      `escena.Pintar(ImGui::GetBackgroundDrawList())` (de lejos a cerca). Sin z-buffer del juego: el escenario no tapa.
    - Útiles: `ps2::LeerAmo(amo, variante, malla, esq*)` (solo la malla), `ps2::LeerGiro(amm)`, `ps2::Centrar(malla)`,
      `ps2::Crear(imagen)` (una textura suelta). Probado con IW (#AMO de objetos, Bubbles); B1/B2 usan el mismo #AMO de
      PS2, pero no está probado con ellos (el Shenron de B2 de R2 es el primero).
  - **Cámara sobre el suelo:** con `suelo_iw.cpp` ya en el proyecto (R11), en el campo de IW el ojo va por encima del
    terreno (`Altura(ojo) + 8`) y Bubbles a la altura del suelo. Compilado (05:27); Piccolo PASS
    `D:\DBZ3HD\_pruebas\20261016-052921_combate_test_iw_minijuego`: al andar contra la meseta Gohan ya no se mete en
    la roca (sube por ella con el suelo de R11) y la cámara queda fuera, aunque mirando muy hacia abajo contra la pared
    (`c_npc.png`). ponytail: la cámara no esquiva paredes (solo el suelo bajo el ojo).
- **Ronda 4, octava vuelta (05:35-06:00): corredor de Piccolo, cansado, cámara contra paredes.**
  - **API nueva de R10 puesta:** Piccolo (14) corre con `anim::Corredor(x1, true)` desde que empieza la pelea (el ciclo
    de carrera de Piccolo de IW, DATA_CMN 925) y se apaga al acabar (`VigilarCombate`, junto a `Scatto(kApagado)` y
    `Cansado(false)`). Ya no se le inyecta andar por el mando del muñeco: su sitio lo lleva la IA y el muñeco le deja
    quieto (quitada la línea de combate.cpp de la quinta vuelta y la rama del mando 1 de `Mando`).
  - **Cansado, del decomp:** con la barra ≤ 0 en la frenada, `State16_Update` → `Char_EnterState7Transform` (EU
    0x2A34D0): barra = 1 y +0x1C0 = 0 (¡lo limpia!), código 0x05 (31 cuadros) → `Char_UpdateAnimToggle` 0x06 (30) →
    `State08` 0x07 (31) → la acción siguiente; en 7 y 8 no hay palanca (solo la física). Aquí: fase 7 = 92 cuadros sin
    mando, `Scatto(x0, kApagado)` + `Cansado(x0, true)` al entrar, `Cansado(x0, false)` a los 61 (ponytail: 0x07 no se
    pone, va en guardia sus 31 cuadros). Durante el cansado el círculo no vale. **Recarga, del decomp:** +0,02 por
    cuadro siempre (`ExplorationMinigameBodyAnimationsMerge`; con +0x16BB, la meta cogida, otro valor no leído), tope
    100, y las bebidas su valor del #ITM; no hay "recuperarse" aparte: después del cansado el botón vale con lo que haya
    (≈ 2,8), y un scatto así cansa otra vez enseguida, como en IW. Quitado el "agotado hasta una bebida" (no es de IW).
  - **Cámara contra paredes:** en el campo de IW, 12 muestras de `suelo_iw::Altura` en el segmento cabeza → ojo; si el
    terreno corta la vista, el ojo se acerca hasta antes del corte (como mucho al 30 %), y luego por encima del suelo.
  - **Muros (R11):** R11 confirma que el movimiento de IW no tiene paredes (se mueve y se pega al suelo), así que subir
    la meseta es lo de IW; la línea de R11 para la cámara ya estaba (la mía es esa + el acercamiento).
  - **Pruebas (compila; capturas miradas):**
    - Piccolo 0x7DD `D:\DBZ3HD\_pruebas\20261016-053735_combate_test_iw_minijuego`: PASS; la cámara ya no mira la roca
      de cerca (`c_npc.png`: Gohan delante de la pared, a la vista). En `d_mirar.png` la flecha derecha (cruceta) lanzó
      un **Masenko** contra Piccolo (250 de daño) aunque se le quitan a B3 los bits de la cruceta de +12/+14: la HD lee
      la cruceta por otro sitio (sin encontrar). La prueba usa ahora D (palanca). **Pendiente:** bloquear la cruceta de
      verdad (buscar dónde la lee la HD para las técnicas).
    - Scatto hasta cansarse, campo 903 `...-054338_...`: PASS, "sin energia (cansado)", Goku en la pose de cansado y la
      barra vacía (`c_agotado.png`; el fondo negro es fuera del campo, tras tres scatto en línea recta).
    - **Bubbles sigue sin verse:** el punto 1 del #ITM 888 es (100, **500**, 100) (le dejaba 500 por encima; arreglado:
      sale en el suelo), y con `DBZ3_IWMINI_NPC_QUIETO=1` ahora se le pone a 50 delante y a un lado de la salida, pero
      tampoco aparece (`...-055821_...\a_salida.png`: las monedas sí se pintan). Así que no es que Goku lo tape: algo
      de su pintado falla (sospecha: la y de `suelo_iw::Altura` en ese punto, o la malla posada). Siguiente paso: registrar
      su y y su proyección en `Dibujar`. La piel con pesos reales sí está comprobada fuera del juego (`bubbles_pose.png`).
- **Ronda 4, novena vuelta (06:00-06:30): Bubbles se ve; la palanca derecha ya no lanza técnicas.**
  - **Bubbles invisible, medido (sonda de un cuadro en `Dibujar`, ya quitada):** en (45, 0, 21), suelo 0, malla
    posada de 2654 triángulos con caja (−4,5 −2,8 −3,2)-(4,5 6,8 5,2) (la piel no colapsa), en pantalla (1003, 407) a
    18 px por unidad, **los 2654 triángulos entregados** y 8 texturas; una moneda en el mismo cuadro, igual. Luego no era
    ni posición, ni recorte, ni escala, ni piel: eran las **texturas**. Las del cuerpo de Bubbles (DATA_CMN 119 t00,
    t03, t05, t06; 4 bits con CLUT) tienen **alfa 0 en todos los píxeles** (la PS2 las pinta solo con RGB) y salían
    transparentes. Arreglo en `ps2::Crear` (`src/ps2_modelo.cpp`): una textura sin ningún alfa se da opaca. Captura:
    `D:\DBZ3HD\_pruebas\20261016-060911_combate_test_iw_minijuego\a_salida.png` (Bubbles marrón, corriendo, a la izquierda
    de Goku; pequeño: está a 50). Con `DBZ3_IWMINI_NPC_QUIETO=1` (solo pruebas) se le pone quieto a 50 delante y a un lado.
    El punto 1 del #ITM 888 es (100, 500, 100): Bubbles sale en el suelo (y = 0, luego `suelo_iw::Altura`).
  - **Técnicas por la "cruceta":** con la sonda del registro (184 B) y otra en el XInput de `sub_8221C178`: la flecha
    del teclado de pruebas **no es la cruceta sino la palanca derecha** (`keybind_rstick_*` = flechas), que la HD pasa a
    +108 (0x2000) y usa como atajo de técnicas (Kamehameha, Masenko). Cambio mínimo en `src/net/netplay_hooks.cpp`
    (gancho de `sub_8221C178`, usuario 0, sin online): mientras `iwmini::Activo()`, la palanca derecha (+12 del
    X_INPUT_STATE) se le da a `iwmini::PalancaDerecha` (la cámara del minijuego, `Camera_UpdateStickOrbit`) y se pone a 0
    para el juego; la cruceta (0x000F), si llega, se quita y pasa a la palanca izquierda. Comprobado con la flecha
    derecha: sin acción 15 ni Masenko, y la cámara gira (`...-062413_...\d_mirar.png`; ahí la cámara acaba dentro de un
    árbol: la marcha de rayos solo mira el terreno). Quitado lo de +24/+112 en `Mando` (no era ahí).
  - **Salto del modo 2 de Bubbles (decomp `s_BBR_POST_EXEC`):** no hay salto: el modo 2 pone el punto de trabajo a
    **25 unidades** del propio Bubbles en la dirección de huida (`CameraAdjustment(25.0, ...)` + su posición) y con +0x62
    (azar & 1) marca algo (el plátano, sin confirmar). Sin altura: no se hace salto.
  - Compila (06:26); pruebas PASS `...-060911_...` (Bubbles visible), `...-062413_...` (Piccolo, flecha sin técnica).
- **Ronda 4, décima vuelta (06:30): registro de minijuegos para R12, R13 y R14 (en `src/iw_minijuegos.h`).**
  - `using Modulo = bool (*)(int clave, int fila, int dificultad);` y `void RegistrarModulo(int clave, Modulo);`: en la
    inicialización estática de su archivo, p. ej. `const bool g_reg = (dbz3::iwmini::RegistrarModulo(0x7D7, &Empezar),
    true);`. Al elegir esa casilla del mapa, `iwmini::Empezar` llama a su `Modulo(clave, fila, dificultad 0..5 del
    MSNMW)`; si devuelve false, el mapa sigue. Un módulo registrado tiene preferencia sobre P4 en su clave.
  - Mientras corre: `Activo()` es true (y `netplay_hooks.cpp` le quita al luchador de B3 la palanca derecha y la
    cruceta, que en la HD lanzan técnicas); los ganchos de P4 (`Fotograma`, `Mando`, el panel) no hacen nada.
  - Al acabar, **una vez**: `void Terminar(const Resultado&);` con `r.clave`/`r.fila` de su `Empezar`, `superado`,
    `centesimas`, `puntos`, `rango` (0 C .. 4 Z) y `monedas`; el mapa lo recoge con `TomarResultado` como siempre.
  - Ayudas: `void HudIW(float segundos, float barra, int cara = 29);` (HUD de IW leído de DATA_USA 6/0 la primera vez:
    TIME arriba a la derecha y abajo a la izquierda el aro con la cara t29..t38 y la barra 0..1; hilo de la interfaz,
    dentro del dibujo de ImGui, cada cuadro). Ya públicas: `combate::OcultarHud(bool)` (sin el marcador de B3),
    `combate::SetMuneco(1 / -1)` (rival quieto con mando virtual, sin IA), `cine::Camara(base, ojo, obj)` (cámara del
    combate, llamada desde el gancho de cámara de clima.cpp), `hub::RequestBattleEx` (el combate), `ps2::*`
    (`src/ps2_modelo.h`: modelos de PS2, `CamaraCombate`, `Escena`), `anim::*` (R10), `suelo_iw::*` (R11).
  - Compila (06:30). Sin prueba de un módulo (aún no hay ninguno); los minijuegos de P4 siguen igual (mismo camino).
  - **Para R2:** `ps2::Crear` (`src/ps2_modelo.cpp`) da opaca una textura cuyo alfa sea 0 en **todos** los píxeles (las
    del cuerpo de Bubbles, que la PS2 pinta solo con RGB). Si el Shenron de B2 sale con partes beis planas, puede ser
    esto: texturas que en B2 sí se pintan con alfa (o que deberían ser invisibles) y que aquí pasan a opacas. Para
    probarlo, quitar esa regla para B2 (p. ej. un parámetro en `Crear`) o mirar el `vtype`/registro de alfa de esas partes.
  - **Puntos de R11 (tercera vuelta):**
    - Cámara: `suelo_iw::Rayo` con la máscara 1 hacia el ojo y, si corta, el ojo en el corte (quitada mi marcha de
      rayos con `Altura`). El rayo sale de la cabeza del jugador y no de `obj` (14 por delante): de cara a una pared,
      `obj` queda dentro de la roca y el ojo se pegaba a la pared.
    - Piccolo (14): releer el sitio del luchador **no sirve**: B3 acerca al jugador a su rival (distancia máxima entre
      luchadores) y Piccolo "le alcanzaba" a 433 en 1,7 s (`D:\DBZ3HD\_pruebas\20261016-063201_combate_test_iw_minijuego`).
      Se sigue integrando la marcha propia, y para las paredes un `suelo_iw::Rayo` a media altura con la máscara del
      luchador (7) antes de cada paso. La persecución vuelve a ir bien (`...-063647_...`: se aleja, Piccolo le sigue).
      **Pero ese rayo no le para:** Gohan entra en la meseta y la cámara se queda mirando la pared, sin verle
      (`...-063848_...\c_npc.png`). Pendiente: revisar con R11 la altura/máscara del rayo para las paredes (o usar su
      `MoveSphere`).
    - Caída (IW `Char_UpdateMovementPhysics`, EU 0x2A8E20: sin suelo, o a ≥ 500 de él, +0x1CC = 1 y fundido): en el
      campo, si `Altura(x, z, centinela) == centinela`, el jugador vuelve al último sitio con suelo. ponytail: sin
      fundido y sin leer dónde reaparece IW. Camino de la Serpiente (2) PASS `...-063404_...`: Goku sobre el camino
      (`b_aros.png`), 6 aros y META, sin caídas en el registro.
  - Bubbles más de cerca (punto 3 de la vuelta): no dio tiempo.
- **Ronda 4, undécima vuelta (07:10-07:30): `suelo_iw::Mover` de R11 y Bubbles de cerca.**
  - **Paredes con `suelo_iw::Mover` (Collision_MoveSphere de IW, R11):** en el modo Piccolo cada paso de la marcha va por
    `Mover(a0, a1, s)` (y `propio = {s[0], 0, s[2]}`); el paso del scatto, con `Mover(..., 0.5f)` (la esfera del
    scatto), en todos los minijuegos con campo; y el extra del andar de IW sobre el de B3 fuera del modo Piccolo, también
    con `Mover`. Prueba Piccolo 0x7DD en el 906 PASS `D:\DBZ3HD\_pruebas\20261016-071329_combate_test_iw_minijuego`:
    Gohan se aleja, **la meseta le para y desliza a lo largo de ella** (z de −118 a −140 contra la pared) y Piccolo le
    alcanza allí; `c_npc.png`: Gohan junto a la pared, visible, cámara fuera de la roca. Los temblores no se ven en una
    captura fija (no medidos).
  - **Bubbles de cerca:** con `DBZ3_IWMINI_NPC_QUIETO=1` (solo pruebas) ahora se le pone quieto a 28 de la salida y
    a 0,8 rad a un lado. `...-072423_...\a_salida.png`: **Bubbles se ve entero**, de espaldas, en la postura de su
    carrera de IW (cabeza con orejas, brazos abiertos, piernas, manos y pies rosas): la piel con los pesos reales
    funciona. Tamaño: 7,3 de alto (el de su #AMO, sin escala), en pantalla ~1/3 de Goku estando ~20 más lejos de la
    cámara, que cuadra con un mono bajo frente a Goku. Sin luz ni sombreado (color plano de la textura: el pintado
    ImGui no tiene luces). En la captura anterior (`...-071520_...`, a 0,45 rad) salía recortado: las esferas de Goku
    (×1,3) tapan lo que queda detrás de él en pantalla aunque esté al lado; ponytail: la oclusión por esferas es gruesa.
- **Ronda 4, duodécima vuelta (07:30-07:45): toon y borde negro en `ps2_modelo` (opcional por modelo).**
  - **Normales:** `Malla::Tri::n[3]` (vtype & 1: xyz tras la posición, giradas como su hueso) y en los pesos del cuerpo
    (`Esqueleto::Peso::normal`, bytes 16..27 de cada entrada de 32 B); `Posar` las anima con el mismo giro (pesadas:
    Σ peso × giro del eje × normal local; rígidas: giro de su eje). `M34::gira` (solo el giro).
  - **`Escena::Poner(..., const Toon&)`** (el `Poner` de siempre no cambia): 3 tonos por vértice con n·l en el espacio
    de la vista y la luz de B3 de los iconos (−0,45, 0,55, 0,70; memoria b3hd-toon-render), tonos 1 / 0,78 / 0,58 con
    cortes 0,25 / −0,25 (ponytail: los de IW no están leídos; el toon de la HD es una rampa de 64 × 64 por material,
    base − rampa, y el de PS2 no se ha buscado); y el borde: cada cara que mira hacia atrás (normal media en vista con
    z < 0) se repite empujada `borde` unidades por sus normales, negra y un poco más lejos (se pinta antes), como hacen
    B3 e IW. Sin normales en el modelo: tono medio y sin borde.
  - En `iw_minijuegos.cpp` todos los modelos de IW (aros, monedas, bebidas, nave, marcas, plátanos, Bubbles) van con
    toon, borde 0,12. **Para R2:** `escena.Poner(cam, malla, tx, pos, giro, esc, ps2::Escena::Toon{})` en las fichas
    del tablero y el Shenron de B2 (si sus #AMO traen normales: vtype & 1); el grosor con `Toon::borde` en unidades de
    su modelo.
  - Mirado a ojo: Bubbles (`D:\DBZ3HD\_pruebas\20261016-073329_combate_test_iw_minijuego\a_salida.png`, recorte en
    `E:\DBZ3HD_trabajo\P4\bubbles_toon.png`): pelo en dos marrones según la luz y **borde negro** alrededor de la silueta,
    como un personaje de B3. Aro de cerca (`...-073734_...\a_salida.png`, con `DBZ3_IWMINI_ARO_CERCA=1`, solo pruebas:
    el primer aro a 45 de la salida): sombreado en el marco dorado; el borde negro apenas se ve (o el aro no trae
    normales, o el borde de 0,12 es fino para su tamaño, ~24). La banda blanca del aro ya estaba antes (parte sin textura).
  - Compila (07:3x). No se ha tocado la oclusión (R15).
- **Ronda 4, decimotercera vuelta (07:45-08:10): el Shenron de B2 con toon, aros, alfa de PS2 y plano cercano.**
  - **Shenron roto con el toon (abanicos por toda la pantalla), causa medida:** sin borde (`DBZ3_TEST_SHEN_TOON=2`)
    sale bien (`D:\DBZ3HD\_pruebas\20261016-080015_escenarios_test3_r2_deseo`), así que era el borde; una sonda de
    triángulos grandes no encontró ninguno entre los normales, y quedarse sin índices de 16 bits tampoco era (no saltó
    el aviso). Era el **camino de dibujo**: con la cámara de R2 (sin profundidad, `za = 0`) los triángulos con textura
    van por el camino de vértices empaquetados de R15 y el borde iba por `AddTriangleFilled`, mezclado entre ellos.
    Arreglo: el borde va por el mismo camino que su triángulo (`b.prof = tp.prof || prof_sin_tex`). Prueba:
    `...-080505_...\a_te_concedere.png`: **Shenron con tonos y borde negro, entero, con el escenario**. (`DBZ3_TEST_SHEN_TOON=1`
    es un parámetro de prueba nuevo en `pasarela_b2.cpp`, junto al dibujo de Shenron; R2 decide si lo deja con toon.)
  - De paso: `Unitaria` da (0, 0, 0) = "sin normal" para normales no finitas o enormes (antes, basura → NaN), y
    `Camara::P` rechaza vértices no finitos; los vértices sin normal no se tiñen ni llevan borde. Y si una lista de
    dibujo no cabe en los índices de 16 bits de ImGui (65535 vértices), `Pintar` quita primero los bordes y luego lo
    más lejano (antes daba la vuelta), con aviso en el registro.
  - **Aros, monedas y nave de IW no traen normales** (vtype 0x36 / 0xB6 / 0x800008B6: color y uv, sin el bit 1), así que
    no llevan borde ni tono (en IW, sin normales, tampoco puede haber ese borde); Bubbles (vtype 0x1B5) sí. El borde por
    modelo ya se da con `Toon::borde` (P4: 0,12 en Bubbles, ~1,6 % de su alto; R2 usa 0,9 % del alto de la ficha).
  - **Alfa de PS2 (R13):** `TexturaAmt` (pasarela_b1.cpp:257) **ya** pasa el alfa ×2 con tope 255 (0x80 = opaco). Si las
    manos de Ten salen medio transparentes, su CLUT trae menos de 0x80: en los modelos de personaje ese alfa no es
    transparencia (en la HD elige la fila de la rampa del toon), y la PS2 los pinta opacos. **Para R13:** no hace falta
    otro ×2; lo correcto es pintar opacas las texturas de personaje. Lo único que hace ya `ps2::Crear` es dar opacas
    las que tienen alfa 0 en todo (Bubbles); si R13 lo necesita para Ten, se puede añadir un "opacas" a `Texturas`.
  - **Plano cercano (R13, disco del 8):** `PonerT` quitaba los triángulos de más de 45 px por unidad (a menos de ~19 de
    la cámara del combate); quitado: ahora corta solo `Camara::P` en z de vista −1 (ponytail: el cercano real de la
    cámara de B3 no está leído).
  - Compila (08:05). Recomprobado: Bubbles igual, con tonos y borde (`D:\DBZ3HD\_pruebas\20261016-080749_combate_test_iw_minijuego`).
    El aro de cerca (`...-081020_...`) se pinta **encima de la cabeza de Goku** aunque está detrás de él (a 45 de la
    salida): antes las esferas de Goku lo tapaban (`...-073734_...`); ahora va por el camino con z-buffer de R15 y no se
    tapa. **Para R15** (no se toca la oclusión aquí).
