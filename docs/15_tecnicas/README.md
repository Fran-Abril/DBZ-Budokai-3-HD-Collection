# 15 — Técnicas: cómo funcionan por dentro y cómo crearlas

Estudio del 2026-10-15 (solo lectura del código recompilado y de las herramientas; no se ha
cambiado código ni lanzado el juego). Convención: `N.cpp:L` = `generated\dbz3_recomp.N.cpp:L`;
**[C]** comprobado, **[?]** deducido, hay que confirmarlo.

| Doc | Contenido |
|---|---|
| `01_cadena_boton_a_tecnica.md` | Runtime: mando → `#CCM` → condiciones → cápsula/ki/forma → acción → `#CSK` → efecto del BSP → golpe/HR → `#SPX` → cámara, con las funciones |
| `02_formatos.md` | `#CCM`, `#CSK` (AP1/AP7/HR), BSP (`#CST`/`#CSE`/`#ACE`), `#SPX`, `#SKC`; rayo vs bola vs disco vs embestida |
| `03_definitivas_choque_hiper.md` | Modo hiper, definitivas (ranura 0, códigos 0x4A0+, cápsula 595), choque de rayos (0x2000/0x68/0x4003) |
| `04_recetas_y_pruebas.md` | Datos mínimos para añadir/clonar/mover una técnica, propuesta `tecnica_clon.py`, recetas de prueba y sonda `DBZ3_TEC_PROBE` propuesta |

## Lo más útil en 6 líneas

1. Coste de ki y formas de una técnica = **cápsula** (`#SKC +15 × 100`, `+14`), no el `#CCM`
   (runtime: `sub_82115980` → `sub_82115518`, ki en `X+6100`, tope `X+6104`).
2. El valor de una línea AP7 clase 4 se reparte **por rango**: 0–3 y ≥ 350 = energía `#CST`,
   4–299 = efecto `#CSE`, 300–349 = banco común (`sub_82128550`, 11.cpp:9318). Solo hay **4 códigos
   de energía propios por personaje**.
3. Daño de energía = `#CST +0xC8`; daño cuerpo a cuerpo = bloque HR (compartido entre códigos).
4. HR tipo 3 → ranura del `#SPX` **del atacante** = `HR+6` (`sub_8210ED48` → `sub_82120C90`):
   agarre 20, definitiva 0/1/2.
5. Choque de rayos = bit 0x2000 en `w5` del `#CCM` + c0 0x68 en el AP7 + entrada respuesta
   cond2 0x4003 (c0 0x69).
6. Para recolorear una técnica nativa ya existe `colores.py nativo … --tecnica CAP=color`; para
   moverla entre personajes, las piezas están en `sb_tecnicas.build_bsp` (`native_ast`),
   `capsulas.csk_graft` y `b1port.Amm` (falta el pegamento: `04_…` §3).

Fuentes previas: `docs\03_formatos\{ACM_FORMAT,CAPSULAS_B3,CAMARA_ACC,SB_VS_B3_MOVESET,FORMAS_Y_KI}.md`,
`docs\07_ports\GOHAN_FUTURO_EXPLORACION_2026-10-06\02_tecnicas\informe.md`,
`docs\10_tag_team\{16_tecnicas_relevo,18_recarga_tecnicas}.md`, `awo_tools\sb_tecnicas.py` (cabecera).
