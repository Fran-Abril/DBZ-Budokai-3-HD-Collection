# 11 - 8ª fila nativa "HUB" en el menú principal (y menú HUB con nuestros modos)

Fecha: 2026-10-15. Solo lectura del código generado (US) y de los AFS. No se ha tocado nada.
Complementa (no repite) `03_menus_y_tareas.md` §2 (creación, tope, despacho) y `01_pantallas.md` (cola de pantallas).
Restricción del usuario: **no se sustituye nada**. Las 7 filas nativas (incluida Dragon Arena, fila 6 -> pantalla 23) siguen
igual; se **añade una sola fila nueva al final, "HUB"**, que abre un menú propio con todos los añadidos (Tag Team,
Online, torre/supervivencia/kumite...). TAG TEAM sigue también con X como ahora. Las secciones 1-6 explican cómo se
construye el menú y cómo meter una 8ª fila (escritas para "Tag Team"; valen igual para "HUB"). **La sección 9 es el plan
del HUB** (etiqueta, pantalla, idas y vueltas).

Marcas: [C] comprobado en código/datos, [?] inferido, falta medir en el juego.

## 0. Veredicto corto

- **Viable, pero no con "subir el tope y ya".** El objeto del menú tiene **sitio fijo para 7 etiquetas** (`+92..+116`); la
  8ª caería en `+120`, que es otro sprite (la franja de agua del realce). Subir `+151` a 8 rompe el dibujo y las animaciones.
- Las posiciones de las filas **no se calculan**: vienen horneadas en el `#ACA` de `data_usi` 2032 (un juego de nodos para
  6 filas y otro para 7). El marco **no crece** con el número de filas.
- Plan recomendado: **fila virtual** (el contador nativo `+151` no se toca) + nuestro sprite nativo para la 8ª etiqueta,
  dibujado con las mismas funciones del juego dentro de su pasada de dibujo. Bocadillo de ayuda: textura nueva nº 8 en el
  banco 1994 (por idioma). Despacho: se intercepta A antes del juego. Riesgo de partida guardada: nulo si nunca se escribe 7
  en `0x824BA135`.
- Etiqueta "Tag Team": se puede **componer con glifos de la propia textura del juego** (T, a, g, e, m están en
  "Tournament"/"Dragon"/"Dueling" del atlas US), en tiempo de ejecución desde el AFS del jugador (como `native_ui.cpp`).
  **"HUB" no**: faltan H y B en las etiquetas del juego -> renderizarla con una fuente parecida (§9.1).
- HUB: fila 8 virtual + menú HUB. Primero panel ImGui con arte del juego (la base ya existe), después "modo HUB" de la
  propia tarea del menú principal (100 % nativo, hasta 7 entradas). Vuelta de los modos al HUB con una bandera al
  reconstruirse la pantalla 5 (§9).

## 1. Objeto del menú principal (172 B, `obj = *(tarea+48)`) [C]

Creado en `sub_8218CB88` (`generated/dbz3_recomp.2.cpp:14831`), tamaño `li r3,172`; campos puestos a cero en
`sub_8218BA80` (`6.cpp:14383`, bucle de 7 en `6.cpp:14472`).

| Offset | Contenido | Dónde |
|---|---|---|
| +0 | banco `data_usi` **2032** (`#ACA`, maqueta 2D: nodos con posición/animación) | `sub_8218B0C8` 41.cpp:14427 |
| +4 | banco `data_usi` **2033** (`#AZT`, texturas del menú; `native_ui.cpp` ya saca de aquí marco/agua/panel) | idem |
| +8, +12 | entradas 3965 y 3966 del AFS 0 (`lis r3,0`) [?] fondo/modelo | idem |
| +16, +20..+64 | 13 sprites de fondo/marco (fijos, no dependen de las filas) | `sub_8218BD28`, `sub_8218B2F0` |
| +68/+72, +76/+80, +84/+88 | sprites (nodos 1, 2, 41, 44, 60, 69 de la tabla `banco+28`) | `sub_8218BDD0/BE70/C190` |
| **+92..+116** | **7 etiquetas de fila** (`+92 + 4*i`) | `sub_8218C190` 19.cpp:14546 (bucle 14624-14661) |
| +120 | sprite del realce (nodo vía `banco+20` [3], `sub_820B0568`) | 19.cpp tras el bucle |
| +124, +128, +132, +136 | nodos 57, 63, 66, 80 | `sub_8218C190`, `sub_8218C000` |
| +140 | objeto de `+8` (`sub_8208DEE0`) | `sub_8218BA80` |
| +144 | bocadillo de ayuda (`sub_8217ED38(0x107CA = data_usi 1994, 8192, 0)`) | 2.cpp, tras `+153` |
| +148 (u16) | estado: 1 nace, 2 entrando, 5 menú activo | `sub_8218C9F0`, `sub_8218C588` |
| **+150** | fila del cursor | (ya usado en `online_panel.cpp:763`) |
| **+151** | número de filas = `6 + (flag != 0)` | 2.cpp:14922 |
| +152 | flag "Dragon Arena desbloqueado" | 2.cpp:14900 |
| **+153** | nodo base de las etiquetas: **50** con 7 filas, **82** con 6 | 2.cpp (tras `stfs 156`) |
| +156, +160 | floats (animación/temporizador) | |
| +164 | sonido de la voz de la fila | `sub_8218B8E0` |
| +168 | fila+1 al moverse | `sub_8218B8E0` |

Parámetros del constructor (`sub_821D3B78`, 21.cpp:18536): `r3 = (s8)0x824BA135` (= `0x824BA110+37`, última fila elegida),
`r4 = (s8)0x824BA1B9` (`+169`, flag Dragon Arena). Si `fila >= tope` se pone 0 (2.cpp, `cmplw r8,r6` / `stb r11,150`).

## 2. Filas: texto, posiciones, marco [C]

- Etiqueta de la fila `i` = instancia del nodo `banco2032.tabla28[+153 + i]` (`sub_820B06E0`). Hay **dos maquetas**:
  nodos 50..56 (7 filas) y 82..87 (6 filas). La separación vertical y la x salen del `#ACA`, no del código.
- Las imágenes del texto están en `data_usi` 2033 (`#AZT`, y su equivalente en `data_eng/spn/fra/ger/ita`). En US la
  textura **0 (248x280)** es el atlas "Dragon Universe / Dueling / World Tournament / Practice / Skill Editing" en letra
  redondeada cian con borde azul y halo blanco (volcado: `campana.py ver 2033`). Options/Dragon Arena están en otra
  textura del mismo banco [?] (no localizada en el tiempo disponible).
- El marco/panel son nodos fijos (`+16..+64`): **no escala** con el número de filas. Medido en pantalla por
  `online_panel.cpp:150-153`: la fila de Dragon Arena ocupa y≈523..597 (coordenadas de `a.At`), nuestra fila ImGui va en
  606..650, ya fuera del panel nativo.

## 3. Dibujo [C]

`sub_8218BBC0` (0.cpp:14784, primera llamada del estado de menú) dibuja en orden: `sub_8218B2F0` (fondo),
`sub_8218B4D0`, `sub_8218B370`, `sub_8218B3F0`, **`sub_8218B550`** (4.cpp:14805, filas).

`sub_8218B550`:
1. Bucle `i < +151` (4.cpp:14908): sprite `+92+4i`; con `+148 == 5` aplica la transformación del nodo `48 + (fila == i)`
   (49 = elegida, 48 = resto; `sub_820AF140/AF7B0/AF9D8` sobre la tabla `banco+28`), luego `sub_820B0B08` + `sub_820AB8E0`.
2. Tras el bucle (desde 4.cpp:15091): vuelve a dibujar la etiqueta elegida (nodo `+153 + +150`) con el realce y el sprite
   `+120` (franja de agua), y `+124`, `+136` (nodos 9 y 24 de animación).

Consecuencia: con `+150 = 7` el realce usaría el nodo 57 (es el de `+124`) -> posición errónea. Con `+151 = 8` el bucle
dibujaría `+120` como si fuera una etiqueta.

## 4. Entrada, ayuda y voz (estado `sub_8218C5D8`, 10.cpp:14782) [C]

- Mandos: tabla en `0x8238B780`, jugador `*(0x8238B670) * 184`; `+14` pulsado (bit0 `0x0001`, A `0x4000`, B `0x1000`),
  `+18 | +114` repetición (`0x10` arriba, `0x40` abajo).
- Despacho y tope: ver `03_menus_y_tareas.md` §2.2 (10.cpp:14914-14958). Al confirmar guarda la fila en
  `0x824BA135` (`stb r10,37(r29)`, 10.cpp:14978).
- Vuelta: arriba en 0 -> `+151 - 1`; abajo en `+151 - 1` -> 0 (10.cpp:15181-15186).
- **Ayuda**: `sub_8217E800(+144, tabla_u16[fila])` (10.cpp:15214-15216), que solo hace `bocadillo+60 = id`.
  Tabla en **`0x82020D54`**: `[1, 2, 3, 5, 4, 6, 7]` y luego `0` (fila 7 mostraría el saludo). Banco `data_usi` **1994**
  (`#AZT`, 8 texturas, igual en los 6 idiomas): 0 saludo de Goku, 1 Dragon Universe, 2 Duelo, 3 Torneo, 4 Habilidades,
  5 Práctica, 6 Opciones, 7 Dragon Arena.
- **Voz**: `sub_8218B8E0` (28.cpp:14772) monta en la pila `[4330..4336]` y llama `sub_82081FE8(1, 0x20000|id, 0)`.
  Con fila 7 lee fuera del array (basura de pila) -> **hay que engancharla**.

## 5. Plan: 8ª fila sin tocar las 7 nativas

Idea: el juego sigue creyendo que hay 6 o 7 filas (`+151` intacto, ningún campo desbordado). Nosotros añadimos la fila
`N = +151` (7 con Dragon Arena, 6 sin él; la fila nueva siempre va **después** de la última nativa, nunca en su índice de
despacho, porque el despacho es por índice crudo: 6 -> pantalla 23 aunque Dragon Arena esté bloqueado).

### 5.1 Ganchos

```cpp
// src/ingame/main_menu_ext.cpp (nuevo). US; EU: comprobar con D:\DBZ3HD\work\eu\map_addrs.py
// (sub_8218C5D8 <-> dbz3eu_sub_8218C5A0: -0x38; las vecinas probablemente igual) [?]
namespace {
uint32_t g_obj = 0;        // objeto del menú vivo
bool g_focus = false;      // cursor en TAG TEAM (fila virtual N = +151)
uint32_t g_label = 0;      // instancia nativa de nuestra etiqueta (sub_820B06E0) o 0
constexpr uint32_t kPad = 0x8238B780, kPadIdx = 0x8238B670;
uint16_t Pressed(uint8_t* base) {
  const uint32_t p = kPad + REX_LOAD_U32(kPadIdx) * 184;
  return REX_LOAD_U16(p + 14);
}
}

// 1) Estado de menú: la fila virtual. Si el cursor está en TAG TEAM, el juego no ve la entrada.
DBZ3_HOOK(sub_8218C5D8, dbz3eu_sub_8218C5A0) {
  const uint32_t obj = REX_LOAD_U32(ctx.r3.u32 + 48);
  g_obj = obj;
  const int n = REX_LOAD_U8(obj + 151), row = REX_LOAD_U8(obj + 150);
  const uint16_t pr = Pressed(base);
  if (!g_focus) {
    if (row == n - 1 && (pr & 0x40) && REXCVAR_GET(dbz3_tag_team_enabled)) {   // abajo desde la última nativa
      g_focus = true;
      REX_STORE_U16(REX_LOAD_U32(obj + 144) + 60, 8);   // ayuda nº 8 (añadida al banco 1994)
      // dibujar este fotograma sin dar la entrada al juego
      PPCContext c = ctx; c.r3.u32 = obj; sub_8218BBC0(c, base);
      return;
    }
    orig(ctx, base);
    return;
  }
  PPCContext c = ctx; c.r3.u32 = obj; sub_8218BBC0(c, base);   // dibuja (B550 enganchada pinta la fila 8)
  if (pr & 0x4000) { g_focus = false; dbz3::ingame::TagTeamOpen(); }                 // A: nuestra pantalla
  else if (pr & 0x10) { g_focus = false; /* vuelve a n-1 */ REX_STORE_U16(REX_LOAD_U32(obj+144)+60,
                         REX_LOAD_U16(0x82020D54 + 2 * (n - 1))); }
  else if (pr & 0x40) { g_focus = false; REX_STORE_U8(obj + 150, 0);                // da la vuelta
                         PPCContext m = ctx; m.r3.u32 = obj; sub_8218B8E0(m, base); }
  else if (pr & 0x1000) { g_focus = false; orig(ctx, base); }                       // B: lo hace el juego
}

// 2) Dibujo de filas: el juego pinta las suyas; nosotros la 8ª con sus mismas funciones.
DBZ3_HOOK(sub_8218B550, /*EU [?]*/ dbz3eu_sub_8218B518) {
  const uint32_t obj = ctx.r3.u32;
  orig(ctx, base);                       // con g_focus el realce nativo sigue en n-1: ver 5.3
  if (!g_label || !REXCVAR_GET(dbz3_tag_team_enabled)) return;
  // misma secuencia que el bucle nativo (4.cpp:14805..): transformación del nodo 48/49, desplazada una
  // separación (y[n-1] - y[n-2]) hacia abajo, y luego sub_820B0B08 + sub_820AB8E0 sobre g_label
}

// 3) Creación de etiquetas: tras las nativas, nuestra instancia (clon del nodo +153+n-1).
DBZ3_HOOK(sub_8218C190, /*EU [?]*/ dbz3eu_sub_8218C158) {
  orig(ctx, base);
  g_label = MakeTagLabel(ctx.r3.u32);     // ver 5.2; 0 si falta la textura -> sin fila nativa (queda la ImGui)
}

// 4) Voz al moverse: nunca leer fuera del array de 7.
DBZ3_HOOK(sub_8218B8E0, /*EU [?]*/ dbz3eu_sub_8218B8A8) {
  if (REX_LOAD_U8(ctx.r3.u32 + 150) >= 7) return;   // sin voz para la fila nueva
  orig(ctx, base);
}
```

Notas:
- Al salir del menú (B o confirmar una fila nativa) poner `g_obj = g_label = 0`; el juego libera sus instancias en
  `sub_8218C520`; la nuestra se libera igual que ellas (`sub_820ACCF0(inst, 1)`, como en `sub_8218C000`).
- Volver a TAG TEAM al regresar de su pantalla: tras `sub_8218C190`, si venimos de TAG TEAM, `g_focus = true`. **No**
  escribir 7 en `+150` ni en `0x824BA135`.
- `online_panel.cpp` (`DuelMenuFilter` rama del menú principal y `DrawTagRow`) se desactiva cuando `g_label != 0`.

### 5.2 La etiqueta "Tag Team" con letra nativa

1. **Componer con glifos del juego (recomendado)**: en US las letras T, a, g, e, m están en la textura 0 de 2033
   ("**T**ournament", "Dr**ag**on", "Du**e**ling", "Tourna**m**ent"). Se recortan en tiempo de ejecución desde el AFS del
   jugador (como `native_ui.cpp`, por hash XXH3 de la textura; nunca se reparte arte del juego) y se monta una textura
   "Tag Team". Para eng/spn/fra/ger/ita: volcar con `python "mod center hd/campana.py" ver 2033` y comprobar que tienen
   esas letras (spn "Torneo", "Dragón"... [?]); si falta alguna, usar la opción 2 solo para ese idioma.
2. Fuente parecida (redondeada gruesa) con relleno cian, borde azul oscuro y halo blanco, como `campana.py`/`capsulas.py`
   hacen con los bocadillos. Más fácil, se nota algo más.
- Dónde vive: **entrada AÑADIDA** en cada AFS de idioma (`dbz3_afs_append`, igual que las campañas) con un `#AZT` de 1
  textura, o directamente una textura guest creada al vuelo. El nodo: clon en el heap guest del nodo `+153+n-1` (misma
  x, y + separación) apuntando a esa textura. **Pendiente RE**: formato del nodo `#ACA` (2032, 31 KB, sin documentar):
  dónde guarda posición y la referencia textura/UV. Se mide comparando los nodos 55 y 56 (misma forma, distinta y).

### 5.3 Realce y marco

- Realce: con `g_focus` el juego sigue resaltando la fila `n-1`. Opciones: (a) en el gancho de B550 poner `+148 = 4`
  solo durante `orig` (todas en "no elegida", sin realce del bucle) y dibujar nosotros el realce sobre la fila 8 con la
  transformación del nodo 49 y el sprite `+120` desplazado; (b) mínimo: dejar la franja en `n-1` y resaltar solo la
  etiqueta 8 (escala/color). [?] medir en juego cuál queda bien.
- Marco: no crece. La 8ª fila cae por debajo del panel (y≈606..650). Opciones: escalar en Y el sprite del panel
  (`+20..+64`, el que use la textura "panel" de 2033) en el gancho de B2F0, o aceptar la fila fuera del panel como hoy.
  [?] medir qué nodo es el panel.

### 5.4 Bocadillo de ayuda

`extend_bank` (`mod center hd/capsulas.py:865`) sobre el banco 1994 de cada idioma (data_usi + data_eng/spn/fra/ger/ita):
textura nº 8, mismo estilo que las nativas (texto blanco con borde, 232-288 px de ancho, 2-4 líneas), p. ej.
"Equipos de 2 o 3 luchadores. / SELECT para relevar al compañero." Se sirve como entrada añadida al final del AFS
(runtime `dbz3_afs_append`) redirigiendo la lectura de 1994, o ampliando 1994 en la carpeta mods (como el resto de mods
nativos). El id 8 se escribe en `bocadillo+60` desde nuestro gancho (no hace falta tocar la tabla `0x82020D54`).

## 6. Riesgos

| Riesgo | Detalle | Mitigación |
|---|---|---|
| Partida guardada | `0x824BA135` guarda la última fila; `0x824BA110` es la estructura de perfil/desbloqueos (`roster_trace.cpp:40`) [?] se guarda | No escribir nunca 7 ahí. Aun así, el constructor pone 0 si `fila >= tope` (sin juego modificado no rompe) |
| Desborde del objeto | 7 huecos de etiqueta | Fila virtual: `+151` no se toca |
| Despacho por índice crudo | fila 6 = pantalla 23 aunque Dragon Arena esté bloqueado | La fila nueva es `N = +151`, interceptada antes del juego |
| Voz de fila | array de 7 en pila | gancho de `sub_8218B8E0` |
| Ayuda | tabla de 7 + 0 | escribir 8 en `bocadillo+60` nosotros |
| EU | direcciones distintas | `map_addrs.py`; hasta entonces solo US (la ImGui actual sigue en EU) |
| Piloto de TAG TEAM | `tagteam_panel.cpp:162` usa `obj+150` | sin cambios: la fila 1 (Duelo) no se mueve |

## 7. Menú de Duelo (ONLINE como 5ª fila)

Filas nativas, cadena y viabilidad: `03_menus_y_tareas.md` §3. Mismo patrón: imagen de filas en `data_usi` 2055
(`filas_vs`, hash `0x1732359AF24A2EFB` en `native_ui.cpp`), cursor y cuenta fija de 4 filas que hoy sigue
`online_panel.cpp:DuelMenuFilter`. El plan es idéntico (fila virtual + etiqueta "ONLINE" compuesta con glifos de
"1P VS COM"... + ayuda añadida). No se ha mapeado aquí el objeto del Duelo (huecos de fila, nodo base) por tiempo:
siguiente paso, repetir §1-§4 sobre su función de estado.

## 8. Siguientes pasos (orden)

1. RE del nodo `#ACA` (2032): comparar nodos 55/56 y 86/87 -> posición y referencia de textura.
2. Gancho de solo lectura en B550 que registre la posición de cada etiqueta en pantalla (medir separación real).
3. Implementar §5.1 en US con la etiqueta ImGui como respaldo si `MakeTagLabel` falla.
4. Banco 1994 con textura 8 en los 6 idiomas (`extend_bank`).
5. Probar en partida con y sin Dragon Arena (6 y 7 filas), ida y vuelta desde TAG TEAM, y guardar/cargar.

## 9. Plan HUB (dirección actual del usuario)

Una sola fila nueva, "HUB", en el índice virtual `N = +151` (7 con Dragon Arena, 6 sin él). Todo lo de §5.1 vale tal
cual cambiando `TagTeamOpen()` por `HubOpen()`. X sigue abriendo TAG TEAM (`online_panel.cpp`, rama `kX`).

### 9.1 La etiqueta "HUB"

- **No se puede componer entera con glifos nativos (US)**: las etiquetas del juego son "Dragon Universe, Dueling, World
  Tournament, Practice, Skill Editing, Options, Dragon Arena". Mayúsculas disponibles: D U W T P S E O A; minúsculas:
  r a g o n i v e s u l d t m c k p. Faltan **H** y **B** (y "b").
- Opciones, de mejor a peor:
  1. **Renderizar "HUB"** con una fuente redondeada gruesa parecida, mismo tratamiento (relleno cian degradado, borde azul
     oscuro, halo blanco) y misma altura de letra que el atlas 2033 textura 0 (medir alto de "Dueling" en píxeles y
     copiar sus colores por muestreo en tiempo de ejecución). La "U" puede salir del glifo nativo de "Universe" para que
     encaje. Generado en tiempo de ejecución, no se reparte arte del juego.
  2. Si se acepta otro nombre, hay palabras 100 % componibles con glifos del juego: "**Specials**", "**Arcade**",
     "**Extras**" no (falta x). Dejo la decisión al usuario: cambia el resultado visible.
- Otros idiomas: el mismo texto "HUB" en las 6 AFS (no se traduce); se renderiza una vez por idioma solo si se quiere
  igualar el color exacto de cada atlas (en principio iguales).

### 9.2 Qué pantalla usar para el HUB

| Opción | Cómo | Pros | Contras |
|---|---|---|---|
| A. **Modo HUB de la propia tarea del menú principal** (recomendada como objetivo) | Al pulsar A en la fila HUB no se pide pantalla: se cambian en sitio las etiquetas `+92..` por las nuestras (destruir instancias con `sub_820ACCF0(inst,1)`, crear clones de nodo con nuestras texturas), `+151 = nº de entradas del HUB` (≤ 7, cabe en los 7 huecos), `+150 = 0`, ayuda con ids nuevos (9, 10...). El despacho se intercepta en el gancho de `sub_8218C5D8`. | Mismo marco, cursor, vuelta, sonidos, bocadillo y animaciones: 100 % nativo. No hay pantalla nueva ni cola. | Necesita el RE del nodo `#ACA` (el mismo que la 8ª fila, §8.1). Máximo 7 entradas por página. |
| B. Pantalla ImGui con arte del juego (como TAG TEAM/ONLINE hoy) | `HubOpen()` = panel ImGui sobre el menú principal, con `native_ui` (marco, agua, panel, bocadillo, Kaio). | Ya existe toda la base (`tagteam_panel.cpp`, `game_style.cpp`, `native_ui.cpp`). Sin RE extra. | Se nota que no es nativo (letra, animaciones). |
| C. Reutilizar otra tarea de menú nativa (p. ej. la de Práctica, pantalla 19) con lista propia | Pedir su pantalla y cambiarle filas/despacho | — | Hay que mapear otro objeto entero; sin ventaja frente a A. |

Recomendación: **empezar por B** (sale con lo que ya hay, y la fila 8 nativa ya da el acceso "de verdad") y pasar a
**A** cuando esté el clon de nodo `#ACA`, que es la misma pieza que la etiqueta HUB. Las dos opciones comparten
`HubOpen()`, la lista de entradas y el despacho, así que el paso de B a A no cambia nada más.

Entradas del HUB y a dónde van:

| Entrada | Destino |
|---|---|
| Tag Team | `TagTeamOpen()` (nuestra pantalla) |
| Online | pedir Duelo `sub_821D2778(14, 0)` y abrir el panel ONLINE al llegar (`g_wanted = true` cuando el byte `0x824BA110 == 14`), o abrirlo directo sobre el HUB |
| Torre / Supervivencia / Kumite | según su implementación; si arrancan desde el Duelo, igual que Online (pantalla 14 + nuestra configuración) |
| Atajos nativos (opcional) | `sub_821D2778(14/16/19, 0)`: Duelo, Torneo, Práctica |

Llamada a la cola desde el anfitrión (mismo patrón que el resto de `src`):

```cpp
void RequestScreen(PPCContext& ctx, uint8_t* base, int screen) {
  PPCContext c = ctx;            // copia: no ensuciar los registros del llamador
  c.r3.u32 = screen; c.r4.u32 = 0;
  DBZ3_CALL2(sub_821D2778, dbz3eu_sub_821D2778_EU /*[?] map_addrs.py*/)(c, base);
}
```

Llamarla desde el gancho del estado del menú (hilo del juego), nunca desde el hilo de ImGui: desde ImGui se deja una
petición atómica que el gancho consume en el siguiente fotograma.

### 9.3 Idas y vueltas

- **Menú principal -> HUB**: A en la fila virtual `N` (gancho §5.1). En A: cambio en sitio; en B: `HubOpen()`.
- **HUB -> menú principal**: B en el HUB. En A: restaurar etiquetas nativas (recrearlas llamando a la parte de
  `sub_8218C190` o guardando las instancias originales en vez de destruirlas), `+151` original, `+150 = n-1` con
  `g_focus = true` (cursor en HUB) y ayuda de HUB. En B: cerrar el panel; el cursor queda en la fila HUB.
- **HUB -> modo nativo** (Duelo, Torneo, Práctica...): `RequestScreen(14/16/19)` y `g_return_hub = true`.
- **Modo -> HUB**: casi todos los modos vuelven con `sub_821D2778(5, 0)` (lista en `01_pantallas.md` §6: `sub_82197020`,
  `sub_82173370`, `sub_82147BE0`...). El menú principal se reconstruye (`sub_821D3B78`); en el gancho de
  `sub_8218C190` (etiquetas recién creadas), si `g_return_hub` -> entrar al HUB con `+150 = última fila del HUB`.
  `g_return_hub = false` al salir del HUB con B.
- **Modo nuestro -> HUB** (Tag Team, Online): al cerrar su panel, si se abrió desde el HUB, `HubOpen()` otra vez (en vez
  de volver al menú principal). Tag Team abierto con X sigue volviendo al menú principal como ahora.
- Guardado: nada de esto se escribe en `0x824BA135`; si el juego guarda allí la fila 1 (Duelo) al pasar por el Duelo
  desde el HUB, no pasa nada: `g_return_hub` manda sobre la fila restaurada. [?] comprobar que el paso por el Duelo no
  deja el cursor del menú principal en otra fila de forma visible antes de entrar al HUB (un fotograma).
