# Frente: HUB (fila nativa "HUB" en el menu principal + pantalla HUB)

Fechas: v1 2026-10-15 (madrugada), v2 2026-10-15 03:30, **ronda 2 2026-10-15 09:40**. Solo US (la imagen EU no pasa por los ganchos;
los dos ficheros estan en el `REMOVE_ITEM` del build solo-EU). Plan de partida:
`docs/12_conectores/11_menu_principal_nativo.md` s9.

## Ronda 2 (2026-10-15, mañana)

### Qué ve el jugador (antes → después)
- **Aviso de guardado del HUB** (antes: el HUB guardaba sin decir nada). Ahora, cuando el HUB guarda (récord o reto a
  medias de Torre/Supervivencia/Kumite, monedero y partidas de los retos de rol, Laboratorio...), sale el aviso **igual
  que el del juego** al guardar la partida: la pantalla 4:3 se oscurece, caja verde translúcida en el mismo sitio,
  "Guardando datos del HUB. No cierres el juego." ~3 s y "Guardado con éxito." ~2 s, con entrada y salida como la del
  juego (crece de 0,85 a 1 con fundido, 0,3 s). A la izquierda, **nuestro icono**: esfera de 4 estrellas en marco rojo
  (CC0, `assets_propios/generar.py`); con el **pack opcional** `pack_hub` instalado, el icono del usuario (Goku SSJ4 en
  marco rojo). Medido en el juego (Opciones > Guardar/cargar > Guardar partida): capturas
  `D:\DBZ3HD\_pruebas\20261015-085521_hub_test_guardar_si` (la caja, 4:3 al 22 %, 3 s + 2 s, cierre a 0,85).
  Solo con el HUB (o el menu principal) a la vista y quieto: nunca en un combate, ni en la espera en negro de los
  combates encadenados, ni encima de las pantallas propias de los modos (selector de retos, Laboratorio, pantalla
  entre combates, pantalla final), ni en un fundido. Lo que se guarde entonces espera y el aviso sale entero al
  volver al HUB. Fuera del HUB (un Duelo normal) no sale.
- **Bocadillo de Torre, Supervivencia y Kumite** (ahora abren el selector de retos de `rpg.cpp`): arriba la lista de
  ese selector ("Torre, Torres del Tiempo y Abyss"), la ayuda, el reto clásico puesto ("Clásica: Torre de Namek
  (4)") y, con el **icono del guardado** delante, el récord y el reto guardado ("Récord: piso 4/4 con Goku -
  Guardado: piso 2") y el monedero de Zenie (`rpg.toml`; tambien con 0). También con el fondo del pack.
- **Combate rápido**: el escenario al azar puede ser también uno nuevo de los mods (altas 17+ del frente de
  escenarios), que el conector carga igual que uno de serie.
- **Textos del HUB en los idiomas del juego**: nombres y ayudas de las 12 entradas, "Muy pronto", la ayuda de la
  fila, "Reto:", "Récord", "Guardado"... en español, inglés, francés, alemán e italiano (antes, en francés, alemán e
  italiano salían en inglés). Los 6 datos de idioma del juego: usi y eng = inglés.
- **Entradas nuevas de otros frentes** sin tocar el HUB (`RegisterEntry`): salen al final, en páginas de 8, con sus
  textos en 5 idiomas, la letra del menu y una insignia genérica (estrella) hasta que tengan la suya.
- Pantallas propias con los **sonidos del menú** del juego (`QueueSe`, pedido por rpg; probado que suena, ver pruebas).
- Piezas CC0 nuevas: iconos de modificadores de reto (`retos/mod_tiempo`, `mod_regen`, `mod_objetivo`) y del
  Laboratorio grabando (`retos/lab_grabar`), insignia `modo_extra`.

### Ficheros tocados
- `src/hub_menu.cpp`: entradas fijas con nombre y ayuda en 5 idiomas, registro de entradas nuevas (`RegisterEntry`,
  hasta 32), `Tr`, `EntryName/EntryHelp/EntryCount`, `InSession`, `QueueSe`; combate rapido con las altas de
  escenarios (`escenarios::Total()`); entrada de prueba con `DBZ3_TEST_HUB`.
- `src/ingame/hub_panel.h/.cpp`: textos traducidos, aviso de guardado (vigila `user_data/dbz3/hub_*.toml` y
  `rpg.toml` con el HUB en uso; `ShowSaving()` a mano; espera al menu principal; el dialogo pasa al final de la
  lista del dibujante para quedar encima de los paneles de los modos), icono en el bocadillo, lista del selector de
  `rpg.cpp` (`modos.h`, `NombreModo`) y monedero (`rpg.toml`), insignia genérica.
- `assets_propios/generar.py` (+ `README.md`): `icono_guardado` (256/128/64), pictos `extra`, `tiempo`, `regen`,
  `objetivo`, `grabar`; `--al-juego` los copia a `res/ingame_ui/` (`hub/guardado*.png`, `modo_extra.png`, `retos/mod_*`,
  `retos/lab_grabar.png`).
- `mod center hd/packs_visuales/pack_hub.toml`: receta del icono del usuario (`Dragon HUB (pack opcional)/Icono guardado
  SSJ4.png` → `ui_ingame/hub/guardado.png`, `_128`, `_64`).

## Ficheros del frente
- `src/hub_menu.cpp`: logica en el hilo del juego, dentro del propio menu principal. Ganchos (uno por
  funcion en todo el exe; si otro frente necesita una de estas, que lo pida aqui):
  `sub_8218B2F0` (v2: primer dibujo del menu = aqui se decide con el mando y se hace el fundido),
  `sub_8218B550` (filas: sin etiquetas/franja con el HUB), `sub_8218B3F0` (cinta del titulo: quita su texto
  en la pantalla HUB), `sub_8218B998` (sin voz/ayuda de fila con el HUB), `sub_8217E800` (el bocadillo del
  menu es del HUB). `sub_8218BBC0` NO es de este frente (lo engancha `tagteam.cpp`).
- `src/ingame/hub_panel.h/.cpp`: lo que el juego no tiene (etiquetas, franja, logo, paginas, texto del
  bocadillo, fundido a negro, vuelo del pack). El dialogo se crea solo (DeferUi), sin tocar `main.cpp`.
- `assets_propios/generar.py` (+ `README.md`): todas las piezas del HUB por codigo (CC0) y el logo del
  usuario recortado; `--al-juego` las copia a `res/ingame_ui/`.
- `res/ingame_ui/`: `etq_<es|en|fr|de|it>_<clave>`, `modo_<clave>`, `modo_extra`, `beta`, `titulo_hub`,
  `hub/logo.png`, `hub/logo_512.png`, `hub/guardado*.png`, `retos/*` (marcadores, cifras, insignia de jefe,
  medallas, modificadores, laboratorio).
- `mod center hd/packs_visuales/pack_hub.toml`: pack opcional (fondo del HUB, vuelo a camara, icono de guardado).
- Interruptor: cvar `dbz3_hub_enabled` (por defecto true).

## Como funciona (medido en el juego)
- Fila virtual: el objeto del menu sigue con sus 6/7 filas; nunca se escribe 7 en `+150` ni en
  `0x824BA135`. Abajo desde la ultima fila o arriba desde la primera -> HUB.
- Botones del menu (mando del juego, disposicion PS2): A 0x4000, B 0x2000, atras del menu 0x1000 (tecla P),
  BACK 0x0001, arriba 0x10 / abajo 0x40 (+18|+114 con repeticion del juego = mantener), **LB 0x0400,
  RB 0x0800** (medido en v2 con `DBZ3_TEST_HUB`), LT 0x0100 / RT 0x0200 (sin medir). El stick izquierdo
  (teclas A/D) no da izquierda/derecha en este menu.
- Sonidos del propio menu: mover 2, aceptar 3, atras 5.
- Tarea del menu: `*(*(0x8245F230) + 16)` (sub_8218CB88); estados `+28`: 0x8218C9F0 carga, 0x8218C8E0
  entra, **0x8218C5D8 activo**, **0x8218C4C8 saliendo (solo dibuja)**. `+148`: 2 entrando, 3 activo,
  4 sale con atras, 5 sale con A. `+152` Dragon Arena, `+151` filas, `+153` nodo base de etiquetas.
- Orden de dibujo (sub_8218BBC0): B2F0 cielo/fondo (+16..+64), B4D0 (+128,+132), B370 reparto (+68,+72),
  B3F0 cinta y su texto (+76,+80), B550 marco (+84,+88), etiquetas (+92..), franja (+120), +124, +136.

### Fundido (v2, punto 9)
Medido en el juego: al entrar en el Duelo el menu se va con su animacion (<0,2 s), la pantalla se funde a
negro en ~1,3 s y la nueva entra desde negro en ~1,5 s (incluye la carga). En el HUB:
1. A en la fila HUB: sonido 3 y la salida de su A (`sub_8218C588`) sin pedir pantalla; estado 0x8218C4C8.
   B en el HUB: sonido 5 y la salida de su B (`sub_8218C3A8/C0C8/BF38`, +148 = 4) sin pedir el titulo.
2. Negro (ImGui, toda la ventana) en 520 ms + 80 ms (`kFadeOutMs`, `kFadeHoldMs` en hub_panel.h).
3. En negro, al principio de un fotograma (gancho de B2F0, nada pintado): se sueltan +84, +88,
   etiquetas, +120, +124, +136 (`sub_820ACCF0(inst,1)`) y se rehacen con sub_8218BE70, sub_8218C000 y
   sub_8218C190 (lo que hace sub_8218C4D0 al nacer, sin el cielo ni el reparto), +148 = 2.
4. Vuelve la luz en 520 ms mientras el menu entra con sus animaciones; a los 600 ms (lo que tarda el juego
   de 2 a 3), +148 = 3, +168 = 0 y estado 0x8218C5D8. Sin el saludo de Goku ni su voz (se salta C8E0).
Si la tarea no cuadra (no es la del menu o no esta activa) se abre/cierra en seco, como en v1.

### Pantalla HUB
- Mismo menu principal en "modo HUB": fondo, reparto, marco, cinta, bocadillo y "BACK: Ayuda" del juego; sin
  sus etiquetas ni el texto de la cinta; encima: filas (insignia + etiqueta con la letra del menu + BETA en
  Tag Team y Online), franja de agua del juego que se desliza, filas que entran escalonadas y el **logo
  "Dragon HUB"** del usuario sobre el papel de la cinta (centro 962,94 del area, 400 de ancho; entra como un
  sello cuando la cinta ya llego). Sin `hub/logo`: `titulo_hub` (letra de los titulos del juego).
- 12 entradas: `tagteam online rapido torre supervivencia kumite ranking laboratorio` (se ven siempre; en
  gris con "Muy pronto" si nadie las registra) y `escenarios pasarela minijuegos satan` (solo cuando se
  registran). Mas de 8 visibles: **paginas** de 8 con LB/RB (y LT/RT, cruceta izq./der.); el recuadro de
  la fila HUB pasa a ser el paginador (`LB < 1/2 > RB` con los botones y flechas del juego y cifras doradas
  nuestras). Arriba/abajo dan la vuelta por todas las entradas (de la ultima a la primera y al reves).
- Bocadillo: ayuda de la entrada; Torre/Supervivencia/Kumite con el **reto puesto** (nombre, n.o de combates
  y descripcion, leidos de `user_data/dbz3/modos/<tipo>.toml`, claves `nombre/name`,
  `descripcion/description`; sin fichero: "Reto: el de serie"). Se relee como mucho cada 2 s.
  Debajo, el record y el reto a medias (`dbz3::modos::Record()` / `Guardado()`) con el icono del guardado
  delante: "Récord: piso 4/4 con Goku - Guardado: piso 2". Con mas de 4 renglones la letra se encoge.
- Vuelta al HUB: cualquier modo lanzado desde el HUB lo reabre cuando el menu principal vuelve a estar
  activo (Duelo -> atras, fin del combate rapido, cierre de TAG TEAM, `busy()` de un modo a false).

### Pack opcional (arte de terceros, nunca en la release)
`python construir_pack.py pack_hub.toml --mods <carpeta mods>` deja `mods/pack_hub/ui_ingame/hub/`:
`fondo.png` (fondo del HUB; con el, el HUB pinta su marco, las tiras doradas y su propio bocadillo) y
`vuelo_1.png`, `vuelo_2.png` (Trunks y Android 17 "flying to camera"): al entrar al HUB el personaje vuela
hacia la camara sobre el fundido (crece cada vez mas deprisa, dos estelas, se turnan `vuelo_1..8`).
Las dos imagenes ya no estan en `assets_propios/png/add/` (CC0): se movieron a la carpeta de assets externos
del usuario, `<raiz de indice.json>/Dragon HUB (pack opcional)/`. Sin el pack, el HUB es igual con fundido.
Ronda 2: `guardado.png`, `guardado_128.png`, `guardado_64.png` del icono del usuario (Goku SSJ4 en marco rojo,
`Icono guardado SSJ4.png`): sustituyen a la esfera CC0 en el aviso de guardado y en el bocadillo (los mods ganan a
`ingame_ui/`). El HUB elige el tamano mas cercano por arriba (sin mipmaps, reducir mas de 2x hace dientes).

## Para otros frentes: como poner un modo en el HUB (API, ronda 2)
Todo en `src/ingame/hub_panel.h` (namespace `dbz3::hub`). Nada de esto obliga a tocar `hub_menu.cpp` ni
`hub_panel.cpp`: se llama desde un inicializador estatico de vuestro `.cpp`.

**1. Entrada con clave ya reservada** (`torre`, `supervivencia`, `kumite`, `ranking`, `laboratorio`,
`escenarios`, `pasarela`, `minijuegos`, `satan`: tienen nombre y ayuda en 5 idiomas, etiqueta e insignia):
```cpp
static const bool kReg = dbz3::hub::RegisterMode("escenarios", &Abrir, &Ocupado);
```
**2. Entrada NUEVA con clave propia** (sale despues de las fijas, en paginas de 8 con LB/RB):
```cpp
static const bool kReg = dbz3::hub::RegisterEntry(
    "abismo",                                                            // clave: [a-z_], unica
    {"Abismo", "Abyss", "Abysse", "Abgrund", "Abisso"},                  // nombre de la fila: es, en, fr, de, it
    {"Baja piso a piso con 3 cartas.", "Go down floor by floor with 3 cards.", "...", "...", "..."},   // bocadillo
    &Abrir, &Ocupado);
```
- Textos: `{es, en, fr, de, it}`; los que falten (nullptr) salen en ingles. El juego tiene 6 datos de idioma
  (usi y eng = ingles). Se copian: pueden ser temporales.
- La fila se pinta con la letra del menu (la del juego); con `res/ingame_ui/etq_<idioma>_<clave>.png` y
  `modo_<clave>.png` sale con la etiqueta e insignia medidas (las genera el HUB con `assets_propios/generar.py`:
  **pedidlas en vuestro `.md`** con la clave y los 5 nombres). Sin insignia propia: `modo_extra` (estrella).
- `open()`: hilo del juego, con el menu principal activo (puede llamar a `RequestBattle`/`RequestBattleEx` o abrir
  su pantalla). `busy()` (opcional): mientras sea true el HUB ni se dibuja ni lee el mando; al volver a false se
  reabre el HUB. Sin `busy`, el HUB vuelve cuando el menu principal renace.
- En gris con explicacion (p. ej. falta una ISO): `RegisterGate(clave, &Listo, &Motivo)` tras registrarla.
- Maximo 32 entradas en total (12 fijas + 20 nuevas).

**3. Aviso de guardado del HUB** (el del juego: oscurece, caja verde, "Guardando datos del HUB" 3 s y
"Guardado con exito" 2 s, con el icono del HUB): sale **solo** cuando, con el HUB en uso (pantalla HUB abierta o un
modo lanzado desde ella que aun no ha vuelto: `InSession()`), cambia cualquier `user_data/dbz3/hub_*.toml` o
`rpg.toml` (`hub_modos.toml` de `modos.cpp`, `rpg.toml` de `rpg.cpp`). Si guardais datos de un modo del HUB, llamad
al fichero `hub_<algo>.toml` (escritura `.tmp` + rename, como `modos.cpp`) y no hay que hacer nada mas. Otro
fichero: `dbz3::hub::ShowSaving()` (cualquier hilo). Fuera del HUB (un Duelo) el resumen de combate que
`modos.cpp` guarda en `hub_modos.toml` no saca el aviso. El aviso solo se dibuja con el HUB/menu principal a la
vista y sin pantalla propia encima (`busy()` de cualquier modo), sin combate (`OwnBattle()`, `tag::InBattle()`) y
sin fundido; si no, espera y sale entero al volver.

**4. Textos traducidos para vuestras pantallas**: `dbz3::hub::Tr({es, en, fr, de, it})` devuelve el del idioma del
juego.

**5. Sonidos del menu en vuestras pantallas** (propuesta 4 de `rpg.md`): `dbz3::hub::QueueSe(id)` (2 mover,
3 aceptar, 5 atras) desde cualquier hilo; suena en el siguiente fotograma del menu principal (hilo del juego, con el
volumen de efectos del jugador), asi que vale para las pantallas que se dibujan encima del menu (selector de retos,
Laboratorio, Ranking). Fuera del menu principal no suena; si se piden varios en un fotograma, suena el ultimo.
Prueba: `DBZ3_TEST_HUB_SE=1` (12 s de sonidos de la cola desde el hilo de la interfaz tras 10 s de menu; el log dice
"sonido N de la cola").

Piezas ya hechas en `res/ingame_ui/` (`S::Img("...")`):
- `retos/marcador_<torre|supervivencia|kumite>.png` (560x128, texto en la caja (128,25)-(538,106)), palabras
  `etq_<idioma>_<piso|oleada|jefe>.png`, cifras doradas `retos/num_0..9`, `num_barra`, `num_x`,
  `retos/insignia_jefe.png`, medallas `retos/medalla_<1..5>_<bronce|plata|oro|platino|diamante>.png`.
- Ronda 2: modificadores `retos/mod_tiempo.png` (reloj de arena), `retos/mod_regen.png` (corazon con cruz),
  `retos/mod_objetivo.png` (bandera) y `retos/lab_grabar.png` (Laboratorio grabando), 128x128; icono del guardado
  `hub/guardado.png` (256), `hub/guardado_128.png`, `hub/guardado_64.png`.

## Propuestas para ficheros de otros frentes (no tocados)
Ronda 1, todas hechas por sus duenos: fila ONLINE sobre el negro (bugs B15), icono del exe (`version.rc`),
interruptor HUB en el launcher, marcador y medallas en `modos.cpp`, `dbz3::modos::Nombre()`. La 6 (fuente
`proyecto:` en `construir_pack.py`) ya no hace falta.

Ronda 2 (integrado de `rpg.md`: propuesta 2, el aviso ya no sale encima de un combate: espera a que el menu
principal vuelva a estar en pantalla y sale entero; 3, el bocadillo de Torre/Supervivencia/Kumite dice la lista de su
selector con `NombreModo()`, la ayuda, el reto clasico puesto, el record y el monedero de `rpg.toml`; 4, `QueueSe`):
1. **rpg (`src/modos.cpp`, marcador y Laboratorio)**: piezas nuevas listas para usar con `S::Img`:
   `retos/mod_tiempo`, `retos/mod_regen`, `retos/mod_objetivo` (128x128) junto al marcador cuando el reto lleva
   `tiempo`, `regen` u `objetivo_tiempo`, y `retos/lab_grabar` en el Laboratorio con la captura encendida
   (hoy "Captura por fotograma: SI/NO" en texto). Diff de ejemplo en `DibujarLab`:
   ```cpp
   if (captura) if (const S::Pic g = S::Img("retos/lab_grabar")) S::DrawPic(dl, g, a.At(x, y), a.At(x + 48, y + 48));
   ```
2. **rpg (`src/modos.cpp`)** (opcional): `LabFinCombate` reescribe `hub_modos.toml` tras CADA combate (resumen del
   Laboratorio). El aviso del HUB solo sale con el HUB en uso, asi que tras un combate rapido o de reto lanzado
   desde el HUB sale tambien por ese resumen. Si se quiere el aviso solo para records y retos, guardar el resumen
   en otro fichero (p. ej. `laboratorio.toml`, sin el prefijo `hub_`).
3. **campanas (`src/pasarela.cpp`, `Motivo()`)**: el bocadillo gris de la "Leyenda de Mr. Satán" sale en ingles
   con el juego en francés, alemán o italiano (`T::T` no tiene esas frases). Con `dbz3::hub::Tr` (ya en
   `hub_panel.h`) y estas traducciones:
   ```cpp
   return dbz3::hub::Tr({"Necesita tu copia de Dragon Ball Z Budokai (PS2): pon la ISO en la carpeta ps2_games, junto al juego.",
       "Needs your copy of Dragon Ball Z Budokai (PS2): put the ISO in the ps2_games folder, next to the game.",
       "Il faut ta copie de Dragon Ball Z Budokai (PS2) : mets l'ISO dans le dossier ps2_games, à côté du jeu.",
       "Benötigt deine Kopie von Dragon Ball Z Budokai (PS2): Lege die ISO in den Ordner ps2_games neben dem Spiel.",
       "Serve la tua copia di Dragon Ball Z Budokai (PS2): metti la ISO nella cartella ps2_games, accanto al gioco."});
   // "Solo con la versión americana del juego." -> "Seulement avec la version américaine du jeu." /
   //   "Nur mit der amerikanischen Version des Spiels." / "Solo con la versione americana del gioco."
   // "La leyenda de Mr. Satán de Budokai 1: once retadores seguidos con las charlas de tu copia del juego." ->
   //   "La légende d'Hercule de Budokai 1 : onze adversaires à la suite, avec les dialogues de ta copie du jeu." /
   //   "Mr. Satans Legende aus Budokai 1: elf Herausforderer am Stück, mit den Gesprächen aus deiner Kopie des Spiels." /
   //   "La leggenda di Mr. Satan di Budokai 1: undici sfidanti di fila, con i dialoghi della tua copia del gioco."
   ```
4. **campanas / escenarios / combate**: para una entrada propia en el HUB, `RegisterEntry` (seccion de arriba) y
   pedir aqui la clave y los 5 nombres para su etiqueta e insignia medidas. Ningun frente la ha pedido aun en esta
   ronda (comprobado 09:40: `rpg.cpp` usa las entradas Torre/Supervivencia/Kumite con su selector; `combate.cpp`
   va por F1; `clima/gigantes/cine` son reglas de reto).

## Pruebas (copia D:\DBZ3HD\hub_test)
`python tools/pruebas.py correr hub_test <escenario.toml>` tras `python assets_propios/generar.py --solo-hub
--al-juego` y copiar `res/ingame_ui/` (con subcarpetas) a `D:\DBZ3HD\hub_test\ingame_ui\`.
Variables: `DBZ3_TEST_HUB=1` (4 entradas de mentira y una nueva `prueba` con `RegisterEntry` para ver las paginas,
botones al log),
`DBZ3_TEST_HUB_ARENA=1` (activa Dragon Arena para el siguiente menu principal; solo copias de prueba).
Escenarios v2 (todos PASS, 2026-10-15):
- `flujo_v2`: fila HUB -> fundido -> HUB -> Torre con el reto puesto -> RB pagina 2 -> LB -> vuelta por
  arriba -> B con fundido -> menu con el cursor en HUB -> fila 0. `..._pruebas\20261015-031532_hub_test_flujo_v2`.
- `arena`: Dragon Arena activado, titulo -> Continuar -> menu de **7 filas**: Arena Dragon visible, HUB
  despues (abajo/arriba), entrar en Arena Dragon (pantalla 23) y volver. `..._pruebas\20261015-031954_hub_test_arena`.
- `modos`: Tag Team -> B -> HUB; Online -> Duelo con el panel ONLINE -> atras -> HUB.
  `..._pruebas\20261015-032140_hub_test_modos` (su FAIL final es del guion: no elegia nada en el menu final).
- `rapido`: combate rapido, menu final -> Menu principal -> Si -> **vuelve al HUB** con el cursor en
  Combate rapido. `..._pruebas\20261015-032828_hub_test_rapido`.
- `pack`: con `pack_hub` construido en la copia: vuelo de Trunks y de Android 17 y fondo del HUB.
  `..._pruebas\20261015-033138_hub_test_pack` (el pack se guardo despues en `hub_test\mods_respaldo\`).
- `record`: `hub_modos.toml` de modos_test: record y reto guardado en el bocadillo de los tres retos.
  `..._pruebas\20261015-040605_hub_test_record`.
Resumen con capturas antes/despues: `D:\DBZ3HD\_pruebas\hub_v2_resumen\`.

### Ronda 2: pruebas (copia `D:\DBZ3HD\hub_test`, escenarios en `E:\DBZ3HD_trabajo\hub\ronda2\`)
Antes, `python assets_propios/generar.py --solo-hub --al-juego` y `res/ingame_ui/` copiado a `hub_test\ingame_ui\`.
| Prueba | Resultado | Capturas |
|---|---|---|
| Medida del aviso del juego (`guardar_partida`, `guardar_si`: Opciones > Guardar partida > Si, copia de pruebas) | medido | `20261015-085232_hub_test_guardar_partida`, `20261015-085521_hub_test_guardar_si` |
| `hub_guardado`: bocadillo de Torre con icono; Laboratorio A dos veces → aviso (y "sigue" con el segundo guardado) | PASS | `20261015-091030_hub_test_hub_guardado`, final `20261015-094240_hub_test_hub_guardado` |
| `hub_guardado_pack`: con `pack_hub` (receta nueva): icono SSJ4 del usuario en el aviso y en el bocadillo | PASS | `20261015-091417_hub_test_hub_guardado_pack` (el pack se guardo despues en `hub_test\mods_respaldo\`) |
| `hub_paginas_nueva` en francés y en alemán (`DBZ3_TEST_HUB=1`): etiquetas, ayudas y "Récord/Guardado" traducidos, pagina 2, entrada nueva de `RegisterEntry` con sus textos, A la abre | PASS (de) | `20261015-091636_..._hub_paginas_nueva` (fr), `20261015-092135_..._hub_paginas_nueva` (de) |
| `hub_reto_aviso` (`DBZ3_TEST_RINGOUT=1`): Torre clasica de 4 pisos desde el HUB; en combate no hay aviso; sale en la pantalla final del reto; al volver al HUB ya no | PASS | `20261015-093523_hub_test_hub_reto_aviso` |
| Comparacion lado a lado con el aviso del juego | | `E:\DBZ3HD_trabajo\hub\ronda2\comp_aviso.png` |
| Cierre (peticion del coordinador, propuestas 2-4 de `rpg.md`): `hub_guardado`: el aviso ya no sale encima del Laboratorio; al volver al HUB sale entero | PASS | `20261015-110536_hub_test_hub_guardado` |
| Cierre: `hub_reto_aviso`: Torre de 4 pisos encadenados; ni en combate, ni en la espera, ni en la pantalla final; sale al volver al HUB; bocadillo "Torre, Torres del Tiempo y Abyss" + "Monedero: 0 Zenie" | PASS | `20261015-105443_hub_test_hub_reto_aviso` |
| Cierre: `hub_se_sin_musica` (`DBZ3_TEST_HUB_SE=1`): musica a 0 en Opciones > Sonido de la copia; 12 s de `QueueSe` desde el hilo de la interfaz: el log de audio da pico 0,326 / rms 0,026 y 0,324 / 0,022 durante los sonidos y **0,000 / 0,000** en cuanto paran: suenan. (Con musica, `hub_se`, la musica tapa la medida.) Despues, `restaurar_musica` la devuelve al maximo | PASS | `20261015-105758_hub_test_hub_se_sin_musica`, `20261015-104921_hub_test_hub_se` |
El de francés acabo en FAIL solo por el guion (la pagina 2 deja el cursor en la misma fila: S x4 daba la vuelta al
combate rapido, que arranco bien: "combate rapido 35 contra 9 (escenario 1, Hyperbolic Time Chamber)"); repetido
en alemán con el guion corregido: PASS.

Bateria `python tools/pruebas.py todo` al cerrar (11:18, con todo lo de arriba): **5/5 PASS** (tag_swap, campana,
du_continuar, batalla, menu). El FAIL de tag_swap de las 10:20 era de dos agentes abriendo la misma copia
(arreglado por el coordinador en `pruebas.py`).

### Cierre 2 (peticion del coordinador: propuestas R1/R7 de `escenarios.md` y sonidos en la espera)
- **Opciones del jugador para clima (ficha 10), jefe gigante (19) e intro de cine (22)**, apagadas de serie:
  - F1 (`src/ingame/quick_settings.cpp`, seccion nueva **ESCENARIO (OPCIONAL)** tras COMBATE): "Clima vivo"
    (`dbz3_clima`), "Jefe gigante" (`dbz3_jefe_gigante`: No / 125 / 150 / 175 / 200 / 250 %) e "Intro de cine"
    (`dbz3_cine_intro`), cada una con su explicacion.
  - Launcher, pestana Juego (`src/launcher/launcher_state.cpp`, `kFeatureTexts`): las tres con texto propio; el jefe
    gigante es una casilla y, encendida, un deslizador de tamano 125-250 % (rama nueva `scale` en `FeatureRow`).
  - Textos en espanol, ingles, frances, aleman e italiano (`src/launcher/i18n.cpp`, 8 filas nuevas al final de
    `kTable`). Copias previas en `_respaldo/ronda2_hub/`.
  - En el online no actuan: `clima::Activo()`, `gigantes::Fotograma()`/su gancho de dibujo y `cine::Encendida()`
    comprueban `net::Session::Get().active()` (leido en el codigo).
- **Arreglo en `src/gigantes.cpp` (del frente escenarios, una linea)**: la opcion no hacia nada porque leia el mando
  `X+5028` como 32 bits; es de 16 (como en `tagteam.cpp` y `combate.cpp`): el COM daba -65535 y nunca era "COM".
  Medido con una sonda de memoria durante un combate rapido. Ahora, tras la intro, "campo de vision 0.785 -> 1.080"
  y el rival a escala. Nota: durante la intro de cine el rival sale a tamano normal (`cine.cpp` pone su mando a otro
  durante la intro); probe a cambiar `cine.cpp` a 16 bits y el juego saco "mando del jugador 1 desconectado": lo
  devolvi tal cual estaba (copia de mi intento en `_respaldo/ronda2_hub/mi_cambio_revertido/`). Propuesta al frente
  escenarios: que `gigantes::Escala` recuerde que el lado era COM antes de la intro.
- **HUB, nombre del escenario**: el HUB no ensenaba "escenario N" en pantalla (solo en el log, que ya decia el nombre).
  Ahora el bocadillo de **Combate rapido** dice el ultimo de la sesion: "Ultimo: Freezer contra Dabra (Ring de Cell)"
  con `dbz3::escenarios::Nombre()`. `rpg.cpp` ya usa el nombre en sus pantallas.
- **Sonidos en la espera en negro entre combates encadenados**: `void dbz3::hub::PlayQueuedSe(PPCContext&, uint8_t*)`
  (publica en `hub_panel.h`; vacia la cola de `QueueSe`; nada en la imagen EU). La llamo yo desde un gancho nuevo en
  `hub_menu.cpp`: `sub_821D6A08` (el estado de la tarea de escenas tras el combate, que `hub.cpp` repite en cada tick
  con `HoldChain`); nadie mas lo engancha. Tambien: un sonido pedido hace mas de 0,3 s ya no suena (la A que empezaba
  un reto sonaba 20 s despues, al acabar el combate).
- **Cierre 3: ya suenan en la espera** (RE, 2026-10-15 tarde). Medido con una sonda de las ranuras de voz del gestor de
  sonido (`sm = *(0x82386260) + 808`; reproductor k en `sm + k*9320`: `+1556` volumen, `+1560` banco, `+1564` estado,
  3 = listo; 64 ranuras de voz de 24 B): en la espera el reproductor 0 (el de los sonidos del menu) sigue listo y el
  sonido se acepta (voz nueva), pero la salida es 0: no faltaba el banco, faltaba el **volumen general**. Al acabar el
  combate el juego lo baja con su fundido de sonido (`sub_821D27C8` -> `sub_821652D0`) y lo sube al crear la escena
  siguiente (el menu principal llama a `sub_82165218` en `sub_8218C9F0`); en nuestra espera no hay escena siguiente.
  Arreglo (en el gancho de `sub_821D6A08`): la primera vez que una pantalla propia pide un sonido en esa espera, la
  misma subida del juego (`sub_82165390(-1, 0)`, fundido de entrada del sonido), una vez por espera. Sonda opcional:
  con `DBZ3_TEST_HUB_SE` el log dice "voces N, suyas M, asa H, estado 3" por sonido.
  Resultado (tienda de rpg encadenada, musica al maximo): antes 0,000; ahora pico 0,244 / rms 0,019 mientras se mueve
  el cursor, comprar da 0,319, y en cuanto paran 0,000 / 0,000 (no vuelve ni musica ni nada del combate).
  Capturas/log: `D:\DBZ3HD\_pruebas\20261015-142904_hub_test_se_espera` (antes: `..._132916_hub_test_se_espera`).

Pruebas del cierre 2 (copia `D:\DBZ3HD\hub_test`; al acabar, las tres opciones apagadas otra vez en la copia):
| Prueba | Resultado | Capturas |
|---|---|---|
| `opciones_escenario`: F1 > ESCENARIO (OPCIONAL), las tres encendidas desde el menu rapido con el mando; combate rapido: intro de cine (10 s con rotulos, "Saltar") y el cielo oscureciendose | PASS el F1, la intro y el clima; el gigante no actuaba (de ahi el arreglo) | `20261015-130049_hub_test_opciones_escenario` |
| `gigante_com` tras el arreglo: "campo de vision 0.785 -> 1.080" al acabar la intro, Broly COM gigante contra Raditz | PASS | `20261015-135322_hub_test_gigante_com` |
| `rapido_bocadillo`: tras un combate rapido, el bocadillo dice "Ultimo: Goku contra C-16 (Casa del abuelo)" | PASS | `20261015-135737_hub_test_rapido_bocadillo` |
| `launcher_escenario` en frances (`DBZ3_TEST_OVERLAY`, `DBZ3_TEST_TAB=6`, `DBZ3_TEST_SCROLL=260`): "Meteo vivante", "Boss geant" con 175 % y "Taille", "Intro cinematique", con sus explicaciones | PASS | `20261015-140115_hub_test_launcher_escenario` |
| `se_espera`: `PlayQueuedSe` se llama en la espera, pero el juego no suena alli (ver arriba) | medido | `20261015-132916_hub_test_se_espera` |
| Bateria `pruebas.py todo` | **5/5 PASS** | resumen en el log de la bateria (14:00) |

### Ronda 2: puntuacion (0-1)
| Tarea | Nota | Prueba |
|---|---|---|
| Icono propio del guardado del HUB (CC0 en la release, el del usuario solo en el pack) y aviso nativo | 1 | `hub_guardado`, `hub_guardado_pack`, `hub_reto_aviso` |
| Entradas del HUB para los frentes nuevos (API publicada, textos en 5 idiomas, prueba con una entrada nueva) | 1 | `hub_paginas_nueva`; ningun frente ha pedido aun una entrada propia |
| H1 / H2 de `vision_futura.md` §3 | 1 | ya estaban (ronda 1); comprobado en el codigo y en las capturas |
| Lista priorizada de assets §4 (1-7) | 1 | 1-5 de la ronda 1; 6: iconos de modificadores y Laboratorio generados (uso propuesto a rpg); 7: pack |

### Lo que queda
- Que `modos.cpp` use `retos/mod_*` y `retos/lab_grabar` (propuesta 1) y que la pasarela traduzca su `Motivo()`
  (propuesta 3).
- Entradas propias de campanas/escenarios/combate cuando las pidan (etiqueta e insignia con `generar.py`).
- `QueueSe` suena (prueba con la musica a 0), pero aun no lo llama ninguna pantalla de modos/rpg/campanas.
- El combate rapido con un escenario de mod no se ha visto en esta copia (no tiene altas): el camino
  (`RequestBattle` con 17+) es el que el frente de escenarios probo desde el HUB (`hub_alta` PASS).

