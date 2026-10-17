# IW: camaras de los minijuegos de botones (R14, 2026-10-17)

Estado (2026-10-17): **portadas** NB012 (0x7DA), NB007 (0x7D5), NB014 (0x7DC), NB013 (0x7DB), NB006 (0x7D4) y el
**resultado** de MSN019 (0x7E1). **Sin portar:** la camara de juego de MSN019 (DATA_CMN 879, ranura 1) y los planos por fila.
Codigo: `src/iw_mini_botones.cpp` (`Poses.cam_*` / `PosesB.cam_*`, `PrecargarClips`, `CamB`, `PasoCamaraA`, `PasoCamaraB`).
`DBZ3_IWMINI_CAMARA_B3=1` las desactiva (vuelven la camara propia y `sep` antiguo); pruebas de ajuste: `DBZ3_R14_SEP`,
`DBZ3_R14_ROT` (grados), `DBZ3_R14_SONDA=1` (vuelca A, B y floats de X).

## Como funcionan (del decomp EU `E:\DBZ3HD_trabajo\ghidra\salida\iw_sles55347.c`)
- `READ_SYSTEMFunctions` (0x140300) es `Task_Create` mal nombrada; `s_NB0xx_PRE_EXEC` solo crea 2 tareas (no tocan la camara).
- **`UiAnim_Start(t0, slot, clip, play, loop)` (EU 0x2ED8F0) reproduce un clip de camara**, no de interfaz: eso es lo que hace
  `Camera_UpdateByMode` (0x2EE340) -> `DragonMission_SceneMatrixUpdate` (0x2EE040) -> `FUN_00127280(t, ctx+0x80, motion, clip)`.
  Escribe en `ctx+0x80`: ojo (+0x00), objetivo (+0x10), giro (+0x30), fov (+0x34). Tiempo en `ctx+0x168` (+1 por cuadro de 60 Hz),
  `+0x164` = reproduciendo, `+0x165` = bucle, `+0x16C` = ultimo cuadro (`n_frames - 1`). Sin bucle se queda en el ultimo cuadro.
- El #AMC de la ranura 0 es `iRam0049ca70 + 0x7EC` (`Camera_FinishMotionLoads`, 0x2EE470) = hijo 2 del #AMB general de los
  minijuegos de botones = **DATA_CMN 915** (`[AMM, AMM, AMC (850 KB, 1456 clips), BSK]`; 204 tiene la misma forma). Formato del
  #AMC: `docs/03_formatos/CAMARA_ACC.md` (PS2 little-endian). Lector Python: `D:\tmp\iwfix\amc915.py` (copia en
  `E:\DBZ3HD_trabajo\R14\poses\`).
- Las matrices salen de ojo/objetivo **tal cual, en el mundo de IW** (`Camera_CopyMatricesToState` 0x2ED690: sin ancla ni
  rotacion). Los luchadores de MgPly/MgPlyB estan en el origen (`+0x40/+0x44/+0x48 = 0`) mirando a **+Z**; los clips los rodean
  (objetivo a la altura del pecho, ojo a +Z). En B3 los ejes son los mismos: basta sumar la posicion del luchador y poner el
  rival-pivote a +Z para que mire igual.
- DATA_CMN 872-879 son otros #AMC (uno por modulo, 300-440 cuadros); el unico `Camera_LoadMotionFile` visto en el codigo es
  `0x36F` (MSN019, ranura 1, 0x2EC560) y `0x36E` (otro modulo, 0x30D6xx): sin portar.

## Clips por fase (ids de `UiAnim_Start`)
| Modulo | Estatico al empezar | READY (intro) | Cada ronda r>=1 | Final bueno / resultado |
|---|---|---|---|---|
| NB012 Vegeta (0x7DA) | 0x14A | 0x44C (180 cuadros; Ten 0x44F, Krilin 0x452, Yamcha 0x455) | tabla USA 0x47DAE0 = 0x14B..0x153 (2 cuadros: planos fijos distintos) | 0x154 (240) / 0x44E si rango 0 (0x44D resultado) |
| NB007 Goku (0x7D5) | 0xB4 | 0x352 (180) | tabla USA **0x472F00** (EU 0x473700 - 0x800) = 0xB5..0xBD | 0xBE (240) / 0x354 si rango 0 (0x353 resultado) |
| NB014 Goku-Gohan (0x7DC) | 0x1A4 cuadro 0 | 0x4B0 (300, entrada) y 0x1A4 (230) | - (0x1A5 en bucle: ojo (2,40,37) obj (1,39,0), fov 0,62) | gana 0x1AE / pierde 0x1AF (240; `QteMg_StartResultPhase` 0x32F8B0), final 0x4B1 (rango>0) / 0x4B2 |
| NB013 Vegeta-Trunks (0x7DB) | ? | ? | 0x190 / 0x191 al empezar ronda | choque 0x187-0x18C, resultado de ronda 0x47F / 0x480 |
| MSN019 Videl (0x7E1) | - | - | `INT_004985A0` (EU) por fila | camara propia en DATA_CMN 879 (ranura 1) |
NB006 (0x7D4): sus funciones estan en 0x378xxx del EU, sin leer.

## Que hay en el port (familia B)
- Fase 0: clip estatico; fase 1 de la ronda 0 se alarga a `dur_intro` = 180 cuadros (clip del READY + pose de intro);
  fase 1 de cada ronda r>=1: su plano fijo; fase 3: clip del final (el B3 acaba solo; cubierto de negro).
- Cada cuadro (`PasoCamaraB`, gancho de `PasoBotones`): ojo/objetivo/fov del clip + posicion del luchador, rival-pivote a +30
  en Z (`x1+48/64/80`), `cine::Camara` y fov en `mgr+0x1C4`. `DBZ3_IWMINI_CAMARA_B3=1` desactiva las camaras de IW.

## Familia A (hecho): anclaje
- Mundo de IW: los dos objetos estan en el origen y la separacion sale de la raiz de las poses. NB014 / NB013: jugador en x -4..-9,
  rival +7..+9 (z ~ -3..0, y ~ 39-45 en el aire; READY sube de y 10 a 45 en ~135 cuadros), camara desde +Z; NB006: par a lo
  largo de Z (Gohan z -8, Piccolo +7), centro x = -15, camara desde -X al ras del suelo (clips 0x96 / 0x97 en x = -57).
- En B3 los objetos estan en el eje X (A = -15, B = +11, z = 0). `PasoCamaraA`: giro alrededor de Y que lleva el eje de IW
  (`Poses.ancla` = centro x, z y eje jugador -> rival) al eje A -> B (`th = ang_iw - ang_B3`), centro = punto medio de los
  objetos, suelo = y del jugador; ojo / objetivo / fov del clip (fov en `mgr+0x1C4`).
- Las raices de las poses (rotadas por el giro de 180 grados de los lados P2 y por el rumbo del objeto) se suman a la posicion
  del objeto, asi que la separacion de los objetos no es la de IW: se calibro mirando el encuadre contra el de IW (jugador a la
  izquierda y rival a la derecha, a ~11-15 u entre caderas): `Poses.ancla[4]` = 17 (NB014, NB013) y 7 (NB006); sin la camara
  de IW siguen 26 / 15. **Calibrado a ojo**: si se quiere exacto, leer las caderas dibujadas (la matriz de WAIST) en vez de
  inferirlas, y ajustar `sep` con la distancia medida.
- Fases (A): fase 0 = primer cuadro de `cam_ini` (quieto), READY = `cam_ini` entero (`dur_intro` cuadros: 230 NB014 / NB013, 100
  NB006: la fase 1 se alarga), juego = `cam_juego` (quieto), resultado = `cam_gana` / `cam_pierde` (`QteMg_StartResultPhase`)
  y a los 100 cuadros el final `cam_fin_bien` / `cam_fin_mal` (NB014 0x4B1 / 0x4B2; NB013 0x47F / 0x480 por analogia, no
  confirmado). NB013: sin portar las rondas y el choque (clips 0x188-0x18C).
- NB006: intro 0x321 (300 cuadros, `Mg_UpdateIntroSequence`) y 0x98 / 0x9A (reinicio de ronda / resultado) sin usar.

## MSN019
- Resultado (hecho): `Results_StateFinishOutcome` (0x2EAB30) reproduce el clip `tabla USA 0x497DA0` = 0x5AA..0x5AF (Videl 0x5AA, 200
  cuadros: primer plano alrededor del origen) junto con el estado 4 de MgPly (0x7BC), no con el 0x776 / 0x7BE: el final de
  Videl pasa a ser 0x7BC (`PosesB.gana / pierde / fin`).
- Camara de juego (sin portar): `s_MSN019_UPDATE` (0x2EC440) reproduce el clip 0 de la ranura 1 (DATA_CMN 879, 300 cuadros: ojo
  (23,155,2) -> (23,36,2), objetivo (-145,172,-689) -> (588,177,-308), fov 0,6) y se queda en su ultimo cuadro. Videl vuela y su
  posicion (`+0x840`, global 0x5D9110) no esta leida: sin ella el ancla seria inventada. Falta leer `Results_InitRound`
  (0x2EB460), `UpdateProgressGauge` (0x2E94D0) y `Minigame_SetupPlayerModels` (0x2EC3B0).
