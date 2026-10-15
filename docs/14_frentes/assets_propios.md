# Frente: assets propios (2026-10-15)

Objetivo: piezas **nuestras y publicables** donde el arte del juego no llega (HUB, Tag Team, icono
del exe), con el aspecto del juego. Carpeta: `assets_propios/` (generador `generar.py`, salida
`png/`, `README.md` con la licencia CC0). Nada del juego se copia: solo se midio.

## Medidas tomadas (data_usi 2033, textura 0 = atlas de etiquetas del menu principal)
- Alto de mayuscula 26 px del atlas; el juego lo pinta a **1.32 px por px del atlas a 900p**
  ("Dragon Universe": 307 px en el atlas, 396 en la captura 1600x900). Filas cada 84 px a 900p,
  borde negro izquierdo en x=140 del area 4:3.
- Relleno: degradado vertical de (1,217,236) arriba a (14,131,174) en la linea base, curva t^1.5.
- Perfil negro ~2.8 px; aro claro (230,230,225) alfa 191 de 2 px; halo claro muy suave abajo-derecha.
- Filas no elegidas: el juego las oscurece a ~0.58 (relleno 222 -> 127, aro 168 -> 104).
- Dorado del marco (textura 5): (248,185,91) / (211,129,15) / (115,61,8), borde (69,20,0).
- Panel de las listas: (8,40,56).
Anchos frente al juego con el mismo texto (px del atlas): Options 150/150, Dragon Universe 304/307,
Dragon Arena 255/258, Skill Editing 217/214.

## Evaluacion (ver `assets_propios/revision/hoja_contacto.png`, no publicable)
- **Etiquetas: muy nativas.** Misma escala, degradado, perfil y aro; a tamano real no se
  distinguen de "Dueling"/"Options". Diferencias a 2x de zoom: la letra del juego es algo mas
  ancha en la x (tipo VAG Rounded) y la "a"/"g" cambian un poco de forma; "World Tournament"
  nuestra seria un 8 % mas ancha.
- **Insignias y banners: coherentes** (dorado y azul medidos del marco y del panel, pictogramas
  con el mismo perfil negro + aro claro). Son estilo propio, el juego no tiene un equivalente
  directo.
- **Ficha del Tag Team: correcta**, sigue la geometria actual de `PartnerCard` (270x40, inclinada
  14, cara 64x40); se ve algo mas "limpia" que las barras del combate.
- **Icono del exe: original y legible** a 16 px (un "3" sobre un estallido de ki en cuadro azul;
  nada parecido a la bola). El icono de la ventana en partida sigue siendo la bola del juego.

## Integracion propuesta (no hecha: no toco codigo de otros frentes)
1. **Donde viven:** copiar `assets_propios/png/**` a `res/ingame_ui/` con nombres planos
   (`etq_<idioma>_<id>.png`, `modo_<id>.png`, `banner_<id>.png`, `tt_ficha.png`, `tt_ficha_ko.png`,
   `tt_swap_listo.png`, `tt_swap_espera.png`, `beta.png`). El CMake ya copia `res/ingame_ui` junto a
   `dbz3.exe` y `make_release` lo publica; `style::Img(name)` ya los carga (y un mod los puede
   sustituir en `mods/<mod>/ui_ingame/`). Lo mas simple: que `tools/ingame_ui.py` llame a
   `assets_propios/generar.py` y copie, o copiar a mano una vez (son 1.5 MB).
2. **Etiquetas del HUB** (en `MenuText` o en el HUB): si existe `Img("etq_<idioma>_<id>")`, pintarla
   con alto `png.h * 0.66 * a.s` (= 1.32/2: los PNG van a 2x del atlas), borde izquierdo en x=140
   del area, centrada en la fila; tinte `IM_COL32(148,148,148,255)` si no esta elegida y
   `IM_COL32(90,90,110,200)` si esta desactivada. Si no hay PNG (idioma o texto nuevo), lo actual
   con la fuente. Idioma: el mismo que elige `data_eng/spn/fra/ger/ita` (region.h / ajustes).
3. **Insignias/banners:** insignia a 96-128 px a 900p; banner 800x176 a 0.5 * a.s, la etiqueta
   encima dentro de la caja (176,34)-(770,146) del PNG.
4. **Ficha del companero** (`PartnerCard`): `tt_ficha.png` es 864x132 (3x de 288x44); el origen
   `p0` de la ficha cae en el px (42,6). Dibujar con `DrawPic(dl, pic, (p0.x-14*s, p0.y-2*s),
   (p0.x+274*s, p0.y+42*s))` (con `mirror`, reflejar uv y x) **despues** de la cara (su ventana
   0..64 x 1..39 es transparente) y **antes** del nombre y de la barra; el surco de la vida
   (70..248 x 25..34) tambien es transparente, asi que el codigo sigue pintando la pista y la
   barra como ahora y deja de pintar el cuadrilatero y su linea. `tt_ficha_ko.png` si `down`.
5. **Aviso de cambio listo:** `tt_swap_listo.png` (128) a ~40 px a 900p junto a la ficha, con alfa
   pulsando (`0.6 + 0.4*sin(t*6)`); `tt_swap_espera.png` mientras no se puede cambiar.
6. **BETA:** `beta.png` a 0.5 * a.s en la esquina de arriba a la derecha de Tag Team y Online.
7. **Icono del exe:** anadir a `src/version.rc` la linea
   `1 ICON "../assets_propios/png/app/dbz3.ico"` (hoy el exe no lleva icono de recurso; la ventana
   en partida mantiene la bola del juego que pone `main.cpp`).

## Licencia
Las imagenes son originales (CC0). La letra es Arial Rounded MT Bold de Windows rasterizada (el
mismo criterio que ya usan los titulos de `tools/ingame_ui.py`); no se publica el archivo de la
fuente. Si se quiere cero dependencia de fuentes de Microsoft, cambiar `fuente()` por una OFL
redondeada (Nunito Black o M PLUS Rounded 1c) y regenerar; descargarla requiere el visto bueno
del usuario.
