# 01 — De pulsar un botón a la técnica en pantalla (runtime, US)

Fecha: 2026-10-15. Solo lectura de `generated\dbz3_recomp.N.cpp` (`N.cpp:L` = línea de la
definición o de la instrucción citada). No se ha ejecutado el juego para este documento.

- **[C]** comprobado leyendo el código recompilado o la imagen `out\analysis\guest_image\dbz3_us_image.bin`.
- **[?]** deducido por uso u orden; hay que confirmarlo (sonda o prueba en juego).

`X` = luchador (`cfg + 2096 + 8240·k`, `cfg = *0x823D6CF4`); `k` = lado (0/1) = `*(X+4944)`;
rival = `*(X+4948)`. Ver `docs\10_tag_team\18_recarga_tecnicas.md` §1 para el resto del mapa.

## Resumen en una línea por paso

```
mando ─► #CCM (árbol de entradas)       sub_820F7BB0  (I = X+4960; raíz I+484 = X+5444)
      ─► condiciones de la entrada      sub_820F7588  (bits de w5: ki, cápsula, transformar, hiper…)
      ─► cápsula + ki + forma           sub_82115980 → sub_82112088 (equipada) + sub_82115518 (ki)
      ─► rótulo de la cápsula           sub_820F74D8 → evento 48 (sub_820F2280)          [?]
      ─► acción + código de ataque      sub_82103908(X, acción) → X+3244
      ─► #CSK: animación + líneas AP    (bloque del código; AP1 golpe, AP7 efectos)
      ─► AP7 clase 4 = efecto del BSP   sub_821286C8(op 0) → sub_82128550(código, X)
             código < 4 o ≥ 350  → sub_82131378   energía (#CST = #AST de PS2)        [?]
             4 ≤ código < 300    → sub_82125D68 → sub_82125AF8  efecto visual (#CSE = #ASE)
             300 ≤ código < 350  → sub_82127B20   banco común del motor
      ─► golpe en el rival              sub_8210ED48 (víctima): línea HR = *(X+6136) + 16·situación
             daño   = HR+0  → sub_8211A5A8(X, daño, …)  (el mismo que tumba en el KO)
             chispa/grito   → sub_82103518 (HR+2..+5)
             HR tipo 3 (guion) → sub_82120C90(4, {atacante, víctima, SPX atacante, ranura = HR+6})
      ─► #SPX (VM) ranura → builtin 0xA7 = clip de cámara del #ACC (X+7900)
```

## 1. Lo que se carga por luchador al empezar (bin CAM)

`sub_8210FDE0` (27.cpp:7784) procesa el bin CAM (`#AMB` con `#ACC`, `#ACL`, `#CCM`, `#SPX`):
`X+7916` nodo crudo → `X+7896` árbol, y por posición de hijo **[C]**:

| Campo | Hijo | Contenido |
|---|---|---|
| `X+7900` | 0 | `#ACC` cámaras (`docs\03_formatos\CAMARA_ACC.md`) |
| `X+7908` | 2 | `#CCM` entradas / combos (= `#BCM` PS2) |
| `X+7912` | 3 | `#SPX` guiones |

Por posición: por eso los ports de IW necesitan el `#AML` de relleno (memoria *iw-moveset-table*).

`sub_82109638` (25.cpp:7574), inicio de asalto: copia `X+7908` a **`X+5444`** (25.cpp:7852) y
fija vida `X+6080/6084` y ki `X+6100` (actual) / `X+6104` (tope) desde el registro de selección
(`*(X+7412)` +36/+40/+44/+48) **[C]** (qué campo es cada uno: [?]).

## 2. El reconocedor de entradas (`#CCM`)

`sub_820F7BB0` (1.cpp:6302) trabaja con `I = X+4960` (lo inicia `sub_820F7868`): `I+484` = raíz
del `#CCM`, `I+488` = nodo actual **[C]**. Cada nodo es un bloque de 64 B (halfwords `w0..w31`,
big-endian en la HD) con sus hijos en `+0x40` (`w7` = cuántos) — el mismo árbol que recorre
`sb_tecnicas.sb_bcm` y `capsulas.ccm_blocks`.

Campos que lee el motor (`sub_820F7588`, 27.cpp:6440) **[C]**:

| Offset | Halfword | Uso |
|---|---|---|
| +8 | w4 (alto) | tipo; `w4 & 0x7000` exige `X+8213 == 0` |
| +10 | w5 | **condición** (bits abajo) |
| +16 | w8 | cápsula |
| +18 | w9 | ki directo (solo ráfagas: 350–425) |
| +20 | w10 | ID de forma (bit 0x10) o máscara de formas (bit 0x1000) |
| +24.. | w12..w15 | códigos de ataque (suelo / aire / variantes) |

Bits de `w5` comprobados en `sub_820F7588`:

| Bit | Qué exige | Función |
|---|---|---|
| 0x0001 | ki `w9` (lo gasta) | `sub_82115518(X, w9, 2)` |
| 0x0002 | cápsula `w8` equipada + su ki + su forma | `sub_82115980(X, w8, 2)` |
| 0x0004 | transformarse | `sub_82119108(X)` (15.cpp:8286) |
| 0x0008 | definitiva: rechazada si `cfg+35560 & 0x200` | (flag de modo) [?] |
| 0x0010 | `w10 == X+2972` (forma actual por ID) | `sub_82112450` (4.cpp:8123) |
| 0x0040 / 0x0080 / 0x0100 | estados del luchador | `sub_821153A0` / `sub_82115400` / `sub_82115448` [?] |
| 0x0400 | modo hiper: solo si `cfg+35560 & 0x80 == 0` y `X+8208 == 0` | `X+8208` = ya en hiper [?] |
| 0x0800 | ventana de combo (`I + 16·I[5980] + 504` bit 4) | tras combo [?] |
| 0x1000 | máscara `w10 & (1 << X+2980)` (índice de forma) | `sub_82112458` (4.cpp:8131) |
| 0x2000 | beam struggle (no lo mira esta función) | ver `03_definitivas_choque_hiper.md` |

`cond2` (`w6`, +12: 0x4003 respuesta de rayo, 0x8001 definitiva) no se lee aquí: [?] lo usa el
recorrido de `sub_820F7BB0`.

## 3. Cápsula, ki y forma

`sub_82115980(X, cápsula, flags)` (6.cpp:8210) **[C]**:
1. `sub_82112088(X, cápsula)` (39.cpp:7997) cuenta cuántas veces está en la lista equipada
   (`*(X+7412)` +52 n, +54 u16 × n). Registro `#SKC` = `*(0x8237560C) + 40·ID`:
   `+30` tipo de sustitución (5 = derivada: cuenta la cápsula base `+32`; 1 = exige `X+2972 == +32`),
   `+11` bits de modo (según `cfg+2018`), `+21` categoría.
2. Coste = **`+15 × 100`** (1 barra = 1000) → `sub_82115518(X, coste, flags)` (23.cpp:8448)
   (si `+11` no tiene el bit 0x02, los flags llevan además `| 3`: significado [?]); resta de
   `X+6100` y lo limita a `X+6104`
   (ver también `X+8208`, `X+8184`, `X+8151`, `X+8149/8150` = multiplicadores de cápsulas de ki [?]).
3. Forma: `+14 & (1 << X+2980)`.

Conclusión práctica: **el coste y las formas de una técnica viven en la cápsula**, no en el `#CCM`
(coincide con el censo de `docs\07_ports\GOHAN_FUTURO_EXPLORACION_2026-10-06\02_tecnicas\informe.md` §1.3).

`sub_820F74D8` (16.cpp:6394) recorre los hijos del nodo actual con bit 0x0002 y, si la cápsula
pasa, envía el evento 48 `{k, cápsula}` por `sub_820F2280` (5.cpp:6019): probable rótulo con el
nombre de la cápsula en combate [?].

## 4. Acción, animación y líneas AP

`sub_82103908(X, acción)` (40.cpp:7311) fija la acción (`X+3244`; 0 de pie, 1 guardia, 2 andar,
15 golpe, 25 recibir, 43/44 modo hiper — `src\tagteam.cpp:253`). El código de ataque elegido
indexa la lista del `#CSK` → bloque de 48 B (animación + tabla AP). Formato: `02_formatos.md`.
Función que avanza las líneas AP por frame: **no localizada** [?] (candidatas: los llamadores de
`sub_821286C8`: `sub_82103518`, `sub_82111208`, `sub_82115020`, `sub_82115080`, `sub_82118F68`,
`sub_82120FB8`).

## 5. Efectos del BSP (AP7 clase 4)

- Tablas por lado, enlazadas tras procesar la ranura de técnicas (`18_recarga_tecnicas.md` §2):
  `S1 = *(0x82333990)` → registros `#CSE` (0xD0) y `S2 = *(0x82333998)` → registros `#CST` (0xF0).
  Los buscan por nombre `sub_82125808` (0.cpp:9120, "#CSE" en 0x8201F648) y `sub_8212AEC0`
  (1.cpp:9026, "#CST" en 0x8201F774) **[C]**. `S1[k]` = primer registro, `S1[k+2]` = número.
- `sub_821286C8(r3, {X, op})` (12.cpp:9126): eventos de técnica `op` 0..7 **[C]**; op 0 →
  `sub_82128550(código, …)` (11.cpp:9318) = **lanzar el efecto del código**, con este reparto
  **[C]**:

| Código c4 | Destino | Qué es |
|---|---|---|
| 0–3 y ≥ 350 (0x15E+) | `sub_82131378` | energía `#CST` (rayo, bola, disco; 0x15E/0x15F = ráfagas de ki) |
| 4–299 | `sub_82125D68` (43.cpp:9184) → `sub_82125AF8` (28.cpp:9236) | efecto visual `#CSE` (cargas, destellos) |
| 300–349 (0x12C–0x15D) | `sub_82127B20` (2.cpp:9225) | banco común del motor |

  Esto explica la regla empírica de `sb_tecnicas.py` («los AST nuevos van en 1–4; con 0x53 la bola
  no hacía daño»): **un `#CST` con código 4..299 nunca se lanza como energía**. Solo hay 4 huecos
  de energía propios (0–3) más los ≥ 350 (que el motor usa para las ráfagas).
- `sub_82128550` además: códigos 200–299 y 400–499 se lanzan desde el **rival** (`X+4948`) [?];
  guarda en la ranura `+42` cuántas copias de la cápsula `X+6744` hay equipadas − 1 (nivel) [?];
  hay un caso fijo para el ID 11 (Piccolo) con `X+2976 == 3` que desplaza algunos códigos (+2/+10).
- Ops 4–6 de `sub_821286C8` solo con `X+2972 == 41`: contador `G2+1152k+16+44` (tope 3) y
  `G2+2330 = 0` — encaja con el forcejeo del beam struggle [?]. Ojo: `X+2972` es también lo que
  compara el bit 0x0010 con `w10` (ID de forma); 41/42 podrían ser IDs de forma y no estados [?].

## 6. El golpe y el guion

`sub_8210ED48` (38.cpp:7714) es el manejador del luchador **golpeado** **[C]**:
- `r30 = *(X+6136) + 16·situación` = línea del bloque HR (8 × 16 B, `02_formatos.md` §2).
- Daño `lha 0(r30)` → `sub_8211A5A8(X, daño, 8232)` (38.cpp:8399/8431; 41.cpp:8281). En la acción
  18 el daño se divide entre 2 antes [?].
- `sub_82103518` (31.cpp:7102): chispa, grito y sonido del golpe según `HR+2` (`grunt/visual`;
  valores `n·1000 + m`, 32 ≤ n < 48 especiales) y el tipo `HR+4`.
- **Tipo 3 (guion)**: `HR+6` (stun_code) = ranura del `#SPX` **del atacante** (`*(atacante+7912)`)
  → `sub_82120C90(4, …)` (38.cpp:9573-9581; 43.cpp:8860). Así arrancan agarres (ranura 20) y
  definitivas (ranura 0, 1, 2).

## 7. Cámara

El guion de la ranura llama al builtin `0xA7` (clip `g[0x60] + K` del `#ACC` de `X+7900`) y espera
N frames (`push N; call sub 0`). Formato y editor: `docs\03_formatos\CAMARA_ACC.md`,
`mod center hd\studio\studio_core.py`. Editar el guion aún no es seguro (VM incompleta).

## Pendiente (en orden de utilidad)

1. Función que recorre las líneas AP por frame (y la que despacha las clases 0–9 del AP7).
2. Confirmar `sub_82131378` como lanzador de `#CST` (leer su cuerpo: debería buscar `+0x12`).
3. Qué es `X+8208` (¿en hiper?) y `X+2972` (¿ID de forma? ¿41/42 = beam struggle?): sonda de
   lectura por frame (ver `04_recetas_y_pruebas.md` §4).
4. Intérprete del `#SPX`: tabla de builtins (para poder escribir guiones nuevos).
