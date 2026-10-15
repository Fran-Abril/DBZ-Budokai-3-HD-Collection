# 02 — Formatos de una técnica (HD big-endian; PS2 = mismos datos en little-endian)

Fecha: 2026-10-15. Reúne lo ya medido (con su fuente) y lo nuevo de este estudio. Nombres HD ↔ PS2:
`#CCM`=`#BCM`, `#CSK`=`#BSK`, `#ACM`=`#AMM`, `#ACC`=`#AMC`, `#CST`=`#AST`, `#CSE`=`#ASE`,
`#ACE`=`#AME`, `#AZT`=`#AMT` (+ textura PS2). **[C]** medido, **[?]** por confirmar.

## 0. Dónde vive cada pieza

| Pieza | Fichero (data_cmn, columnas de `mod center hd\roster_db.json`) | Por qué importa |
|---|---|---|
| `#CCM` entradas, `#SPX` guiones, `#ACC` cámaras | bin **cam** (Goku 288, Gohan adulto 231) | uno por personaje |
| `#CSK` animación + golpes + efectos, `#ACM` animaciones | bin **anm** (uno por forma; las formas 2+ son `#CSK` que sobrescriben códigos) | por forma → variantes por forma |
| `#CST`/`#CSE`/`#ACE`/texturas | bin **bsp** (504–555; tabla por ID `0x82333208`) | uno por personaje |
| cápsula `#SKC` (ki, formas, clase) | data_usi 4 | global (596 registros) |
| nombres de cápsula | data_usi 2663/2684 (`#AZT` por ID) y ficha `SCM*.amb` (data_usi 5–52) | ver `CAPSULAS_B3.md` |

## 1. `#CCM` — bloque de entrada (64 B, halfwords `w0..w31`)

Fuente: `docs\03_formatos\CAPSULAS_B3.md`, `capsulas.py` (`ccm_blocks`, `w`), y el runtime
(`01_cadena_boton_a_tecnica.md` §2, **[C]**).

| w | Off | Campo |
|---|---|---|
| w0 | +0 | dirección (1 →, 2 ←, 0x10 ↑ en SB/IW; 0x20 = arriba+E en IW) |
| w1 | +2 | botones (8 = E; 0x0F = P+K+G+E) |
| w4 | +8 | tipo de especial (`& 0x7000` → exige `X+8213 == 0`) |
| w5 | +10 | condición: 0x1 ki, 0x2 cápsula, 0x4 transformar, 0x8 definitiva, 0x10 ID de forma, 0x400 hiper, 0x800 tras combo, 0x1000 máscara de formas, **0x2000 beam struggle** |
| w6 | +12 | cond2: 0x4003 respuesta de rayo, 0x8001 definitiva, 0x2000 hijo de combo |
| w7 | +14 | nº de hijos (punteros en +0x40) |
| w8 | +16 | **cápsula** |
| w9 | +18 | ki directo (0 en especiales: el coste está en la cápsula) |
| w10 | +20 | ID de forma o máscara de formas |
| w11 | +22 | ID de habilidad de IW (B3 no lo usa) |
| w12..w15 | +24 | **códigos de ataque** (suelo 0x2xx, aire 0x3xx) |

## 2. `#CSK` — código de ataque → animación, golpes y efectos

Fuente: `docs\03_formatos\ACM_FORMAT.md` §4, `awo_tools\csk_chain.py`, `sb_tecnicas.bsk_aps/bsk_put`.

```
+0x10 n códigos   +0x14 lista (u32 por código)   +0x18 n bloques HR   +0x1C sección HR
código → sub-bloque(s) de 48 B: [anim u16][almacén u16] … +0x24 0, +0x28 n_ap, +0x2C tabla AP
tabla AP: n_ap × [tipo u16][n líneas u16][offset u32]       líneas de 16 B
```

Tipos de línea AP: 0 mirada, **1 golpe**, 2 aire, 3 giro, 4 velocidad, 5/6 miembros/manos, **7 eventos**.

- **AP1 (golpe)**: `[frame u16][id u16][act u8][0][HR u16][props u16][hueso u8][radio u8][x y z s8][0]`;
  `HR = 0xFFFF` cierra la ventana. `props`: 0x10 normal, 0x04 armadura, 0x20 cargado, 0x40 inesquivable.
- **Bloque HR** (`+0x1C + 128·HR`, 8 líneas de 16 B por situación: normal, counter, juggle, espalda,
  suelo, **guardia (daño 0)**, ?, aturdido): `[daño s16][grito u8][visual u8][tipo u16][código u16][empuje f32][extra f32]`.
  Tipo 0 normal, 1 juggle (extra = altura), 2 despide (extra = dirección 2×s16), **3 guion SPX
  (código = ranura)**, 4/5 guardia. Los bloques HR se **comparten** entre códigos: cambiar uno cambia todos sus usuarios.
- **AP7 (eventos)**: `[frame u16][id u8][act u8 = 2][clase u32][valor u32][0 u32]` (`sb_tecnicas.ap7_line`).

| Clase | Valor | Notas |
|---|---|---|
| 0 | acción del motor | 0/1 ráfaga de ki; 0x32–0x38 cargas; **0x64 congelar / 0x66 descongelar** (todos los especiales directos); **0x68 / 0x69 beam struggle**; 0x46/0x47 inicio/fin de rayo [?]; 0x3F–0x43 desaparecer (agarres); lista blanca de B3: `sb_tablas.B3_C0` |
| 1 | sonido | |
| 2 | chispa | |
| 3 | hueco del banco de gritos (`gritos.py`) | |
| 4 | **efecto del BSP** | reparto por código en `01_…` §5: 0–3 y ≥ 350 energía, 4–299 visual, 300–349 común |
| 5 / 6 | efectos de golpe / suelo | |
| 7 / 8 | modo hiper (`c7 0x4`, `c8 0x72`; tipo 7 en frames 88/100 del bloque 0x258) | ver `CAPSULAS_B3.md` final |

## 3. BSP: `#AMB` [AMB0, AMB1, …, `#AZT` grande, `#AMO`/`#ACM` modelos, `#ATR`, `#AWV`]

Fuente: `sb_tecnicas.py` (cabecera y `build_bsp`), `colores.py` (`retint_tecnica`, `efectos_capsula`).

- **AMB0** = tabla `#CST` + `#ACE` propios. **AMB1** = tabla `#CSE` + `#ACE` propios.
- Tabla: `[magic][4 B][n u32 @+8][inicio u32 @+0xC]` + n registros de tamaño fijo, cada uno
  empieza por `"wk<n>"`. El juego los localiza por nombre (`sub_82125808`/`sub_8212AEC0`) **[C]**.
- Enlace a partícula = par `(origen u16, índice u16)`: origen **2** = hijo del propio AMB, **1** =
  hijo del AMB de los `#CSE`, **0/6** = bancos comunes (copiar uno de esos a otro BSP colgaba el
  juego, prueba 2026-10-08).

### 3.1 `#CST` (energía), 0xF0 B

| Off | Campo |
|---|---|
| +0x10 | **tipo**: 0 rayo, 1 bola, 2 barrera, 3 disco/proyectil, 11, 12 (Kaio-shin)… |
| +0x12 | **código** (el valor de la línea AP7 clase 4) |
| +0x20/+0x22 | de dónde salen las texturas del rayo (2 = el BSP) |
| +0x24 / +0x28 | duración f32 / velocidad f32 |
| +0x2C | hueso de salida |
| +0x30..+0x36 | enlace **cola** del rayo |
| +0x48 / +0x4A | texturas del rayo (índices del `#AZT` grande) |
| +0x58..+0x5E | enlace **cabeza** (o el núcleo de la bola) |
| +0x64..+0x6A | 2º enlace de la bola (núcleo otra vez) |
| +0x70..+0x7A, +0xAC..+0xB6 | enlaces de **impacto** |
| +0xC0 | 9 en todas las bolas |
| +0xC8 | **daño** (energía; no está en el HR) |
| +0xCC | radio f32 |
| +0xD4 | aturdimiento |
| +0xDE | 7 en todas |

### 3.2 `#CSE` (efecto visual), 0xD0 B

`+0x5C/+0x5E` enlace a `#ACE`; `+0x6A` tres códigos **inicio / medio / fin** (el efecto nace con
el 1º y se apaga con el 3º; carga = con fin, destello = sin medio ni fin); `+0x80` hueso; `+0xAA` formas.

### 3.3 `#ACE` (partículas)

Cabecera 0x10 con n nodos; árbol por punteros (`+0x24` hijo, `+0x28` hermano); tipo u32 en `+4`
(1 emisor, 2 partícula, 3 campo). Partícula: 2 colores RGBA f32 en `+0x80`, textura `+0xA0` = 1,
`+0xA4` = índice del `#AZT` grande (`colores._ace_nodes`, `ame_retint`). El `#AME` de Shin Budokai
es otro sistema (cabecera 0x40): no se convierte, se clona uno del donante.

## 4. Qué cambia entre rayo, bola, disco y embestida

| | Rayo (Kamehameha) | Bola (Masenko corto) | Disco / proyectil | Embestida / cuerpo a cuerpo |
|---|---|---|---|---|
| `#CST` tipo | 0 | 1 | 3 | ninguno |
| Enlaces | cola + cabeza + impacto; texturas +0x48/+0x4A | cabeza + 2º núcleo, +0xC0 = 9, sin texturas de rayo | cabeza `(2,1,2,i)` | — |
| Daño | `#CST +0xC8` | `#CST +0xC8` | `#CST +0xC8` | **bloques HR** de sus líneas AP1 |
| AP7 típico | c0 0x64 f8 · c4 carga (`#CSE`) · c0 0x68 · c0 0x66 · c4 rayo (0–3) · c0 0x46/0x47 | igual sin 0x68 | igual | AP1 varios golpes; c4 destellos 4–299; último golpe HR tipo 2 (despide) |
| Choque de rayos | `#CCM` w5 0x2000 + c0 0x68 + entrada respuesta | no | no | no |

Ejemplo real (Gohan adulto, ID 4): `informe.md` §1.2 en `docs\07_ports\GOHAN_FUTURO_EXPLORACION_2026-10-06\02_tecnicas\`.

## 5. `#SPX` (guiones) y `#ACC` (cámaras)

- `#SPX` es **little-endian también en la HD**. `+0x14` base del código (0x74), `+0x18` n ranuras,
  `+0x20` tabla de ranuras (u32 relativos a la base; 0xFFFFFFFF vacía). Opcodes conocidos y
  builtins (0xA7 clip de cámara; 17 = reproducir código `(lado 0/1, flag, código)`; `call sub 0` =
  esperar N frames): `docs\03_formatos\CAMARA_ACC.md`, `studio_core.py`, `b1port.spx_plays`.
- Ranuras: **0 = definitiva** (1, 2 = 2ª/3ª de Vegeta y Buu M), **20 = agarre**, 10 en 5 personajes.
- `#ACC`: `docs\03_formatos\CAMARA_ACC.md` (633 clips, ida y vuelta byte a byte).

## 6. Cápsula `#SKC` (40 B, data_usi 4)

`+0` u64 dueños · `+8` clase (0x21 ataque) · `+11` bits de modo · **`+14` máscara de formas** ·
**`+15` ki en décimas de barra** (coste = `+15 × 100`) · `+16` cápsula requerida · `+21` categoría ·
`+30` tipo de sustitución (5 = derivada de `+32`) · `+38` precio/100. Detalle: `CAPSULAS_B3.md`.
