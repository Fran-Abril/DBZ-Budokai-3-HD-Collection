# Frente: HUB (fila nativa "HUB" en el menu principal + pantalla HUB)

Fechas: v1 2026-10-15 (madrugada), **v2 2026-10-15 03:30**. Solo US (la imagen EU no pasa por los ganchos;
los dos ficheros estan en el `REMOVE_ITEM` del build solo-EU). Plan de partida:
`docs/12_conectores/11_menu_principal_nativo.md` s9.

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
- `res/ingame_ui/`: `etq_<es|en|fr|de|it>_<clave>`, `modo_<clave>`, `beta`, `titulo_hub`, `hub/logo.png`,
  `hub/logo_512.png`, `retos/*` (marcadores, cifras, insignia de jefe, medallas).
- `mod center hd/packs_visuales/pack_hub.toml`: pack opcional (fondo del HUB, vuelo a camara).
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
  Debajo, el record y el reto a medias (`dbz3::modos::Record()` / `Guardado()`): "Récord: piso 4/4 -
  Guardado: piso 2". Con mas de 4 renglones la letra del bocadillo se encoge.
- Vuelta al HUB: cualquier modo lanzado desde el HUB lo reabre cuando el menu principal vuelve a estar
  activo (Duelo -> atras, fin del combate rapido, cierre de TAG TEAM, `busy()` de un modo a false).

### Pack opcional (arte de terceros, nunca en la release)
`python construir_pack.py pack_hub.toml --mods <carpeta mods>` deja `mods/pack_hub/ui_ingame/hub/`:
`fondo.png` (fondo del HUB; con el, el HUB pinta su marco, las tiras doradas y su propio bocadillo) y
`vuelo_1.png`, `vuelo_2.png` (Trunks y Android 17 "flying to camera"): al entrar al HUB el personaje vuela
hacia la camara sobre el fundido (crece cada vez mas deprisa, dos estelas, se turnan `vuelo_1..8`).
Las dos imagenes ya no estan en `assets_propios/png/add/` (CC0): se movieron a la carpeta de assets externos
del usuario, `<raiz de indice.json>/Dragon HUB (pack opcional)/`. Sin el pack, el HUB es igual con fundido.

## Para otros frentes (sin tocar este)
`src/ingame/hub_panel.h`:
```cpp
// en su .cpp, a nivel de fichero:
static const bool kReg = dbz3::hub::RegisterMode("torre", &TorreOpen, &TorreBusy);
```
- `open()`: hilo del juego, menu principal activo (puede llamar a `RequestBattle` o pedir pantallas).
- `busy()` (opcional): mientras sea true el HUB ni se dibuja ni lee el mando (su panel encima); al pasar
  a false el HUB se abre otra vez. Sin `busy`, el HUB vuelve cuando el menu principal renazca.
- Claves: `torre`, `supervivencia`, `kumite`, `ranking`, `laboratorio`, `escenarios`, `pasarela`,
  `minijuegos`, `satan` (todas con etiqueta en 5 idiomas e insignia ya en `res/ingame_ui`). Hoy registran:
  modos.cpp (torre, supervivencia, kumite, ranking) y pasarela.cpp (pasarela).
- Piezas para el marcador de retos (M2 del frente de modos), en `res/ingame_ui/retos/`:
  `marcador_<torre|supervivencia|kumite>.png` (560x128, insignia a la izquierda; texto en la caja
  (128,25)-(538,106)), palabra `etq_<idioma>_<piso|oleada|jefe>.png` (cian, como las filas) y cifras
  `num_0..9`, `num_barra`, `num_x` (doradas, mismo alto); `insignia_jefe.png` (256);
  `medalla_<1..5>_<bronce|plata|oro|platino|diamante>.png` (256) para el Ranking. `S::Img("retos/...")`.

## Propuestas para ficheros de otros frentes (no tocados)
1. **`src/ingame/online_panel.cpp`**: su fila ONLINE del Duelo se ve sobre el negro del fundido del juego
   al entrar y salir del Duelo (capturas `04_a`, `05_a`, `13_b` y `14_b` de
   `D:\DBZ3HD\_pruebas\20261015-024304_hub_test_transicion\`). Que la dibuje solo con el menu de Duelo activo
   (o con el alfa de su entrada).
2. **`src/version.rc`**: icono propio del exe (ya generado): `1 ICON "../assets_propios/png/app/dbz3.ico"`.
3. **Launcher (Mods nativos)**: interruptor "HUB" para `dbz3_hub_enabled`, junto a TAG TEAM/ONLINE.
4. **`src/modos.cpp`**: usar las piezas de `retos/` para el contador "Piso/Oleada/Jefe N de M" (M2) y las
   medallas en la pantalla de Ranking (M1). El HUB ya lee nombre/descripcion del reto por su cuenta (H2).
5. **`src/modos.cpp`**: sacar `std::string NombrePj(int id)` del espacio anonimo (y declararlo en su API) para
   que el bocadillo diga "Récord: piso 4/4 con Goku" (hoy sin el nombre; linea `ponytail:` en hub_panel.cpp).
6. **`mod center hd/packs_visuales/construir_pack.py`** (opcional): admitir `fuente = "proyecto:<ruta>"`
   relativa a la carpeta del proyecto, para recetas con imagenes que no esten en la raiz de assets.

## Pruebas (copia D:\DBZ3HD\hub_test)
`python tools/pruebas.py correr hub_test <escenario.toml>` tras `python assets_propios/generar.py --solo-hub
--al-juego` y copiar `res/ingame_ui/` (con subcarpetas) a `D:\DBZ3HD\hub_test\ingame_ui\`.
Variables: `DBZ3_TEST_HUB=1` (4 entradas de mentira para ver las paginas, botones al log),
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
