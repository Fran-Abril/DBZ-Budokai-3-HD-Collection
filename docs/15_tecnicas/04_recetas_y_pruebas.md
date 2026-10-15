# 04 — Recetas: añadir, clonar y mover técnicas + cómo probarlas

Fecha: 2026-10-15. Para el frente de ports (`importar.py`, `awo_tools`). Todo lo de aquí se
apoya en funciones que **ya existen**; lo nuevo son propuestas marcadas como tales.

## 1. Datos mínimos para AÑADIR una técnica a un personaje

Caso típico: «el Kamehameha del personaje, en otro color, con otro daño y su propia cápsula».

| # | Pieza | Qué escribir | Herramienta existente |
|---|---|---|---|
| 1 | Cápsula `#SKC` | registro nuevo (ID ≥ 596): clase 0x21, `+14` formas, `+15` ki (10 = 1 barra) | `[[capsula]]` en `personaje.toml` → `roster_build` + `src\roster_ext.cpp` |
| 2 | Entrada `#CCM` (bin cam) | copia de un bloque de especial: `w0/w1` mando, `w5 = 0x0002` (+0x2000 si choca), `w8` = cápsula, `w12/w13` = códigos nuevos (pareja 0x2xx/0x3xx libre) | `capsulas.ccm_blocks`, `w`; patrón en `sb_tecnicas.aplicar` |
| 3 | Bloque `#CSK` (cada anm de forma) | copia del bloque del código origen en los códigos nuevos; cambiar en su AP7 los **c4** a los códigos nuevos del BSP | `capsulas.csk_graft` (se lleva sus HR), `sb_tecnicas.bsk_put`, `ap7_line` |
| 4 | `#CST` (energía) | copia del registro origen con **código nuevo en 0–3** y `+0xC8` = daño | `sb_tecnicas.build_bsp` / `native_ast` |
| 5 | `#CSE` (carga / destello) | copia con códigos nuevos (libres en 4–299: `ASE_NEW` 0x58–0x5F, 0x72–0x7E) | `build_bsp` |
| 6 | Color | `#ACE` y texturas **copiados** y recoloreados (lo compartido no cambia) | `colores.retint_tecnica(bsp, tono, ast_codes, ase_codes)` |
| 7 | Nombre | `nombre` de la `[[capsula]]` | roster_build |

Restricciones duras **[C]** (`01_cadena_boton_a_tecnica.md` §5):
- **Energía: solo 4 códigos propios por BSP (0–3)**. 4–299 se lanza como efecto visual (sin daño);
  300–349 es el banco común; ≥ 350 son las ráfagas de ki (0x15E/0x15F). Si los 4 están ocupados,
  hay que reutilizar uno (o cambiar de técnica). Códigos ≥ 0x170 caen en la misma rama que la
  energía, pero no están probados [?].
- Daño de **energía** = `#CST +0xC8`; daño **cuerpo a cuerpo** = bloques HR (`csk_edit.py`). Los HR
  se comparten: `csk_edit --attack` cambia a todos los códigos que usan ese bloque → para un daño
  propio, copiar el bloque HR (lo hace `csk_graft`/`bsk_put new_hr`).
- Equilibrio de los nativos: 1 barra ≈ 250 de daño, 2 barras ≈ 400 (`sb_tecnicas.RECETAS`).
- Toda técnica directa congela: c0 0x64 en el frame 8 y 0x66 11 frames antes del c4 de la energía
  (`tech_lines`).

### 1.1 Atajo para personajes nativos (sin port)

Recolorear una técnica existente sin tocar nada más **ya funciona** (Kamehameha rojo de Goku
confirmado en juego): `python "mod center hd\colores.py" nativo 0 --mod goku_rojo --tecnica 6=rojo`
(cápsula 6 → sus códigos c4 por `efectos_capsula`). Cambiar solo el daño: `csk_edit.py` (melee) o
`+0xC8` del `#CST` (energía; hoy sin CLI → ver propuesta §3).

## 2. MOVER una técnica de un personaje a otro

Lo mínimo que hay que llevar (origen A → destino B):
1. **Animación**: el código de A usa `[anim][almacén 3]` = animación de **su** `#ACM`. Hay que
   añadirla al `#ACM` de B y reapuntar el bloque (`b1port.Amm` / `bsk_graft`, como hace el port de
   B1). Si A y B comparten esqueleto por sufijo, va tal cual; si no, `cinematica.py`-style (rotaciones
   de A + cintura de B).
2. **Bloque `#CSK`** con sus HR (`capsulas.csk_graft`).
3. **Efectos**: `native_ast(A, código)` / `native_ase` de `sb_tecnicas.build_bsp` ya importan un
   `#CST`/`#CSE` nativo con sus `#ACE` y texturas al BSP de B (receta `ast_nativo = (ID, código)`,
   p. ej. Spirit Shot de GHF = bola de Riot Javelin del ID 39). Prohibido: enlaces a bancos comunes
   (origen 0/6) — colgaban el juego.
4. **Reescribir los c4** del bloque copiado con los códigos nuevos del BSP de B.
5. **Entrada `#CCM` + cápsula** (pasos 1–2 de §1). Si la técnica tiene choque de rayos, llevar
   también la entrada respuesta (cond2 0x4003, c0 0x69).
6. **Por forma**: el `#CSK` va por forma; repetir el paso 2 en cada anm de B donde deba existir y
   poner la máscara en la cápsula (`+14`) — mecanismo de `roster_build.form_variants`.

Traducción B1/SB → B3: ya cubierta por `b1port.py`, `sbport.py` + `sb_tecnicas.py` (tablas de
equivalencias en `docs\03_formatos\SB_VS_B3_MOVESET.md`); definitivas en `03_…` §2.

## 3. Propuesta de herramienta (no implementada): `awo_tools\tecnica_clon.py`

Un solo comando para el Mod Kit («técnica de otro personaje, renombrada y recoloreada»). Reutiliza
todo lo anterior; ~150 líneas de pegamento.

```python
# python tecnica_clon.py --origen 4 --capsula 26 --destino 0 --mod goku_masenko \
#                        --nombre "Kamehameha rojo" --tono 0 --dano 300 --ki 10 --mando ">E"
def clonar(origen, cap, destino, mod, nombre, tono, dano, ki, mando):
    ast, ase = colores.efectos_capsula(anms(origen), cam(origen), cap, bsp(origen))   # qué lleva
    codes = codigos_de_capsula(cam(origen), cap)                    # w12..w15 de sus entradas
    bsp_b, mapa = importar_efectos(bsp(destino), origen, ast, ase)  # native_ast/native_ase; energía → 0..3 libre
    bsp_b, _ = colores.retint_tecnica(bsp_b, tono, mapa.ast_nuevos, mapa.ase_nuevos)
    set_dano(bsp_b, mapa.ast_nuevos, dano)                          # +0xC8
    for anm_b in anms(destino):                                      # cada forma
        for c_new, c_old in libres_par(anm_b, codes):                # 0x2xx/0x3xx sin usar
            anm_b = capsulas.csk_graft(anm_b, c_new, anm(origen), c_old)
            anm_b = remap_c4(anm_b, c_new, mapa)                     # AP7 clase 4
            anm_b = copiar_animacion(anm_b, anm(origen), c_new)      # #ACM banco 3
    cam_b = anadir_entrada_ccm(cam(destino), mando, cap_nueva, codigos_nuevos, choque=tiene_0x68)
    escribir_mod(mod, bsp_b, anms_b, cam_b, capsula=dict(nombre=nombre, tipo="especial", ki=ki))
    comprobar(bsp_b, anms_b, cam_b)        # sb_tecnicas.comprobar: c4 existen, 0x2000 ↔ 0x68, códigos 0–3
```

Comprobación mínima que debe dejar (assert, sin juego): que cada c4 nuevo exista en el BSP, que
los `#CST` nuevos tengan código < 4, y que `sb_tecnicas.comprobar` pase.

## 4. Cómo probar (en orden: sin juego → copia aislada → juego)

1. **Sin juego**: `python awo_tools\sb_tecnicas.py comprobar DIR --donante N`,
   `python awo_tools\csk_chain.py --entry <anm> --code <hex>` (cadena del código),
   `python "mod center hd\auditar_importar.py" --fuente X --mods <tmp> --construir`.
2. **Copia aislada** (nunca la partida real; memoria *protect-save-data*):
   `python tools\pruebas.py correr <copia> batalla --env DBZ3_LAB=1 --env DBZ3_ACT_PROBE=1`
   o un `.toml` con `pasos = [...]` y `log "regex" seg` para esperar una línea del log.
   - `DBZ3_LAB=1` → `logs\lab.csv` por tick de pelea: acción (`X+3244`), vida, posición de los dos
     (`src\tagteam.cpp:867`). Mide el daño real (caída de `vida2`) y la duración (ticks en la acción).
   - `DBZ3_ACT_PROBE=1` → `dbz3 actprobe: X=… accion=… antes=… vida=…` en cada `sub_82103908`
     (`src\tagteam.cpp:1594`), más `stateprobe`/`endprobe`.
   - `DBZ3_TEST_KO=1` deja al rival a 1 de vida (rápido para ver el golpe final).
3. **Prueba en juego** con el usuario: ventana visible, sonido bajo (memorias *game-window-visible*,
   *no-game-while-user-on-pc*). Las entradas de 3–4 botones simuladas no registran: la técnica la
   pulsa el usuario o la hace el COM.

### 4.1 Sonda propuesta (no implementada): `DBZ3_TEC_PROBE=1`

Tres ganchos de solo lectura en `src\` (mismo estilo que `actprobe`), para validar una técnica
nueva sin mirar la pantalla:

| Gancho | Qué registra | Para qué |
|---|---|---|
| `sub_82128550` (r3 código, r4 → X) | `tecprobe: lado k efecto c4=0x.. rama=energía/visual/común` | que el c4 nuevo se lanza y por qué rama |
| `sub_82115518` (r3 X, f1 coste) | `tecprobe: ki -N (antes A, tope T)` | coste real de la cápsula |
| `sub_8211A5A8` (r3 X, r4 daño) | `tecprobe: daño N a lado k` | daño de energía y de golpes |

Con eso, un escenario de `pruebas.py` puede exigir `log "tecprobe: .*c4=0x2 rama=energía"` y
`log "daño 300"`. También serviría para la prueba A/B del choque de rayos (§3 de `03_…`): registrar
`X+2972` y `G2+1152k+60` (contador) por tick durante un Kamehameha contra Kamehameha.
