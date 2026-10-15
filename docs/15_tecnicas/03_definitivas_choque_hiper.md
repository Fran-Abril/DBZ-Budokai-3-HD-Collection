# 03 — Definitivas, choque de rayos (beam struggle) y modo hiper

Fecha: 2026-10-15. Junta lo medido en otros estudios (fuente indicada) con el runtime de
`01_cadena_boton_a_tecnica.md`. **[C]** medido, **[?]** por confirmar.

## 1. Modo hiper (entrada obligatoria para las definitivas)

- `#CCM`: P+K+G+E (`w1 = 0x0F`), `w5 = 0x0400`, código propio (Gohan adulto 0x259; bloque común
  0x258/0x358 con la animación del banco 3) — `informe.md` §1.2 (exploración Gohan del Futuro).
- Runtime **[C]** (`sub_820F7588`, 27.cpp:6440): se acepta solo si `cfg+35560 & 0x80 == 0` y
  `X+8208 == 0` (probablemente «ya en hiper» [?]). Acciones del luchador mientras dura: 43/44
  (`src\tagteam.cpp:253`).
- El estado lo ponen las **líneas AP7 del bloque**, no el `#CCM`: c7 0x4, c8 0x72, c0 0x64, c4 0x12C,
  c0 0x2C, c0 0x66, y eventos **tipo 7 en los frames 88 y 100** (de 120). Si la animación es más
  corta, esos eventos no llegan: aura y pantalla oscura pero sin ki rojo
  (`capsulas.csk_fit_frames` los comprime; `CAPSULAS_B3.md` final).
- Ports sin hiper (IW «aura burst» cond 0x4000, SB): `capsulas.adapt_iw_bcm` + `csk_graft` del bloque
  del donante; `capsulas.hyper_check` avisa si falta.

## 2. Definitivas (ultimates)

Cadena **[C]** (censo de los 38 en `informe.md` §1.5 + runtime):

```
#CCM P+K+G+E, w5 0x000A (0x001A si depende de la forma), w6 0x8001, cápsula w8 (ki +15 = 50)
  → código de carrera (Gohan 0x25A) → AP1 con HR tipo 3, código 0
  → sub_8210ED48 (víctima) → sub_82120C90: ranura 0 del #SPX del ATACANTE
  → guion: call sub(p1 = cápsula de la definitiva, p2 = primer código WIN, p3 = ?)
      Goku: sub 0x3874(10, 0x4A0, 12); la sub compara un local con 0x10/0x11/0x12 (¿resultado
      del minijuego: gana / pierde / empate?) [?]
  → reproduce las animaciones 0x4A0+ (bloques sin AP) y los clips del #ACC (builtin 0xA7)
```

- Inicio de la serie WIN por personaje: 0x4A0 casi todos; 0x490 Saiyaman/A17/Buu L/Bardock; 0x480
  Vegeta/Piccolo/Krilin/Ten; 0x49F Nappa. Tras ella, la versión «defiende» (Trunks 0x4B2…). Nunca
  saltar huecos (0x4A8 de Piccolo es la BASE del agarre). Decodificador: `b1port.cine_call`, `cine_win`.
- 2ª/3ª definitiva = ranuras 1/2 (Vegeta cápsula 44, Buu M). Agarre = ranura 20 (BASE 0x480).
- Nativos **sin** definitiva: IDs 2, 6, 9, 16, 17, 18, 40, 42, 43 (memoria *iw-moveset-table*).
- **Traducción ya hecha** (no tocar el `#SPX`): la animación de la definitiva de B1/SB se reparte
  sobre los códigos WIN del donante a su velocidad (`b1port.receta_definitiva`,
  `sb_tecnicas.receta_ult_sb`, `awo_tools\cinematica.py`: rotaciones de la fuente + posición de
  cintura del donante, huesos raíz del donante). Cámaras = las del donante (editable en el Studio).
  Límite: si la nueva dura menos que la espera del guion, la cámara se queda en su último frame.
- **Choque de IDs de cápsula (595)**: B1 y B3 comparten numeración (Typhoon de Dodoria 85 = la
  definitiva de Nappa 85). El importador mueve la entrada de la definitiva del donante a la cápsula
  **595** y la cápsula propia lleva `reemplaza = 595` (memoria *ultimates-translation-143*).
  Ojo: `sub_821B7290`/`sub_821B7C50`/`sub_820FE3D8` descartan IDs ≥ 595 en menús y en el modo 5 —
  `roster_ext.cpp` ya lo parchea (ID prestado) (`CAPSULAS_B3.md`).

## 3. Choque de rayos (beam struggle)

Datos **[C]** (`beam_struggle_censo.txt`, 35/38 personajes):

| Pieza | Entrada que dispara | Entrada «respuesta» |
|---|---|---|
| `#CCM` | `w5` con bit **0x2000** (Gohan: 0x2002), cond2 1 | misma cápsula y botones, **cond2 0x4003** |
| AP7 | c0 0x64 (f8) · **c0 0x68** · c0 0x66 · c4 rayo | sin 0x64/0x66; **c0 0x69** + c0 0x40/0x43 |
| Código (Gohan) | 0x24B | 0x26C |

- Anclaje en el donante: 0x64 en el frame 8, 0x68 12 frames y 0x66 11 frames antes del c4 del rayo
  (`sb_tecnicas.tech_lines`, `FREEZE_BEFORE = 11`). `sb_tecnicas.aplicar` ya inserta el par completo
  y `comprobar` exige que cada 0x2000 tenga su 0x68 y su respuesta.
- Runtime [?]: `sub_821286C8` ops 4–6 llevan un contador por lado (tope 3) y limpian `G2+2330`
  con `X+2972 == 41` — candidato al forcejeo (machacar botones).
- **Desactivar el choque** (petición de Discord, pendiente en memoria): hipótesis sin probar —
  quitar las entradas cond2 0x4003 **o** cambiar las líneas c0 0x68 por nada (y quitar el bit
  0x2000). Prueba A/B recomendada en `04_recetas_y_pruebas.md` §4 antes de ofrecer el interruptor.

## 4. Lo que falta para crear una definitiva **nueva** (no traducida)

1. Tabla de builtins del intérprete `#SPX` (para escribir un guion propio en una ranura libre).
2. Qué hace `p3` del call de la ranura 0 y la rama gana/pierde.
3. Hasta entonces: reutilizar la ranura 0 del donante y cambiar solo animaciones (`cinematica.py`),
   cámaras (Studio) y efectos (el BSP del donante va en la base, así que los c4 del guion siguen
   apuntando a efectos existentes).
