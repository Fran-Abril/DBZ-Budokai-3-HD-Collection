# 12 Conectores: tabla de pantallas (US)

Solo lectura. No se ha editado ningun fichero del proyecto ni se ha ejecutado el juego.

Ruta base de citas: `generated\dbz3_recomp.N.cpp:LINEA` (codigo recompilado US). Cada numero de linea es el del fichero. La funcion es la de `DEFINE_REX_FUNC` que contiene la linea.

Convencion: **[C]** = comprobado en la instruccion PPC recompilada (`// ...` del fichero). **[I]** = inferido por uso o patron. **[?]** = no confirmado.

Scripts de apoyo (solo en el scratchpad de la sesion, no en el proyecto): `scan2.py`, `funcs.py`, `sim.py`, `chain.py`, `apply_desc.py`.

---

## 1. Resumen

1. La pantalla es el byte `0x824BA110` (`UI_STATE`, byte 0). La pagina de menu es el byte `0x824BA111`. [C]
2. La mayoria de cambios de pantalla no son directos. Se **encolan** con `sub_821D2778(pantalla, arg)` y los aplica un nodo consumidor (`sub_821D4xxx` a `sub_821D7xxx`). [C]
3. El cambio se aplica en el frame siguiente, cuando la compuerta `0x8247A998` esta a 1. [C] (`generated\dbz3_recomp.9.cpp:18329-18331`)
4. Hay 30 sitios `stb` que escriben el byte de pantalla, en 28 funciones: 26 consumidores de la cola y 2 escrituras directas con valor fijo (`sub_821D7D18` y `sub_821D6B00`). Ademas hay 3 funciones que escriben la palabra de 4 bytes que contiene la pantalla (posible puesta a 0) [?]. Tabla en seccion 4.
5. No existe una tabla indexada por numero de pantalla. El handler de cada pantalla es un puntero a funcion que se guarda en `obj+28` (nodo activo). El valor se elige con cadenas de `cmpwi`/`beq` dentro de cada consumidor. Tabla en seccion 5.
6. El interruptor de "entrada a pantalla" esta en `sub_821D7C30` (8 casos: 4, 6, 14, 16, 19, 22, 23, 28). [C]
7. Los conectores (quien pide cada pantalla) son 40 llamadas a `sub_821D2778`, mas peticiones en linea dentro de `sub_821D3628`. Tabla en seccion 6.
8. Mapeo del menu: `sub_821D3628` traduce (pagina de menu, resultado de menu) a pantalla. **El mapeo de los items del menu principal (pantalla 5) no esta resuelto [?].** Ver seccion 7.
9. Limitacion: `default.xex` tiene la cabecera XEX2 con `encryption_type=1`, asi que no se puede leer la seccion de datos (`.rdata`) con los punteros de funcion. Los handlers solo se conocen como direcciones `sub_...`. [C]

---

## 2. Direcciones de estado

| Direccion | Nombre propuesto | Que es | Evidencia |
|---|---|---|---|
| `0x824BA110` | `PANTALLA` | Byte de pantalla actual. Es el byte 0 de `UI_STATE`. | [C] escritura `generated\dbz3_recomp.1.cpp:18346`; lectura `generated\dbz3_recomp.24.cpp:7028` |
| `0x824BA111` | `UI_PAGE` | Pagina del menu (0..6). La usa `sub_821D3628`. | [C] `generated\dbz3_recomp.14.cpp:19099-19104` |
| `0x824BA135` | `MODO_HUD` | Byte +37 de `UI_STATE`. Se escribe segun la pantalla. | [C] `generated\dbz3_recomp.22.cpp:6826` |
| `0x824BA185` | `UI_PEND_A` | Pendiente de reset. Lo lee `sub_821D3888`. | [C] `generated\dbz3_recomp.37.cpp:18083` |
| `0x824BA186` | `UI_PEND_B` | Modo del reset pendiente (valor que pasa a `sub_821D7D18`). | [C] `generated\dbz3_recomp.37.cpp:18101-18105` |
| `0x8247A988` | `REQ_ARG` | Argumento de la peticion (r4 de `sub_821D2778`). | [C] `generated\dbz3_recomp.16.cpp:18359` |
| `0x8247A98C` | `REQ_NEXT` | Segunda plaza de la cola. Solo se usa si `REQ_CUR` esta ocupada. | [C] `generated\dbz3_recomp.16.cpp:18354` |
| `0x8247A990` | `REQ_CUR` | Pantalla pedida ahora. `-1` = libre. | [C] `generated\dbz3_recomp.16.cpp:18347` |
| `0x8247A994` | `FLAG_A` | Bandera. La ponen `sub_821D27A8` y `sub_821D27C8` (valor 1). Los consumidores la ponen a 0. | [C] `generated\dbz3_recomp.5.cpp:18488`, `generated\dbz3_recomp.29.cpp:18688` |
| `0x8247A998` | `GATE` | Compuerta. La pone a 1 `sub_821D27F8` (26 llamadores). Los consumidores la leen y la ponen a 0. | [C] `generated\dbz3_recomp.14.cpp:19060`; lectura en `generated\dbz3_recomp.9.cpp:18325-18331` |
| `0x8247A9A0` | `MGR_PTR` | Puntero al objeto del gestor de UI que usa `sub_821D7D18`. | [I] `generated\dbz3_recomp.11.cpp:19610-19612` |
| `obj+28` | `NODO` | Puntero a funcion del nodo activo (handler de la pantalla). Lo escriben los consumidores. | [C] `stw rX,28(rY)` en los consumidores |
| `0x823D6CE0` | `MENU_R` | Resultado del menu (`R` en `sub_821D3628`). Lo escriben los manejadores de entrada del menu. | [C] escritura en `generated\dbz3_recomp.11.cpp:7084`, `31.cpp:7008` y `7027`, `32.cpp:7644`, `33.cpp:7010`, `7038`, `7065`. Significado [?]. |

Nota: `REQ_NEXT`, `REQ_CUR`, `REQ_ARG` y `REQ_FLAG` coinciden con `docs\08_online\13_duelo_online.md`. La `GATE` (`0x8247A998`) no aparece en ese documento y aqui se añade.

---

## 3. La cola de peticiones

### 3.1 Encolar: `sub_821D2778(r3 = pantalla, r4 = arg)`

Fichero: `generated\dbz3_recomp.16.cpp:18337-18361`.

- Si `REQ_CUR == -1`: `REQ_CUR = r3` (linea 18347).
- Si no: `REQ_NEXT = r3` (linea 18354).
- Siempre: `REQ_ARG = r4` (linea 18359).

Despues de encolar, casi siempre se llama a una de estas (la bandera que habilita la aplicacion): `sub_821D27F8` (GATE = 1, 26 llamadores), `sub_821D27C8` (FLAG_A = 1, 4 llamadores) o `sub_821D27A8` (FLAG_A = 1, 9 llamadores). [C] para el efecto; [I] para el orden de llamadas.

### 3.2 Aplicar: patron comun de los consumidores

Ejemplo completo: `sub_821D4190`, `generated\dbz3_recomp.9.cpp:18294-18388`.

1. Gate A: `li r3,4` (18309) + `bl sub_821D3888` (18311). Si devuelve != 0, sale. [C]
2. Gate B: `bl sub_821D7FE8` (18318). Si devuelve != 0, sale. [C]
3. Si `GATE == 0` (lectura 18327, comparacion 18329-18331), sale.
4. `GATE = 0` (18343), `FLAG_A = 0` (18357).
5. `r11 = REQ_CUR` (18345), `r10 = REQ_NEXT` (18349).
6. `REQ_NEXT = -1` (18355). `REQ_CUR = r10` (18363): la cola avanza.
7. `PANTALLA = r11` (18361, `stb r11,-24304(r4)`). Esta es la escritura del byte.
8. Si la pantalla es 8 (comparacion 18359), `NODO = 0x821D7040` (18367-18371). [C]
9. `bl 0x821C9240` (18374): funcion de entrada del consumidor. [C]

**Gate A** (`sub_821D3888`, `generated\dbz3_recomp.37.cpp:18068-18124`): lee `UI_PEND_A` (0x824BA185). Si es distinto de 0, si vale 2 pone `0x824E61E1 = 1` (18098), y llama a `sub_821D7D18(UI_PEND_B)` (18105). Devuelve 1 (la pantalla ya fue reseteada). Si es 0, devuelve 0.

**Gate B** (`sub_821D7FE8`, `generated\dbz3_recomp.36.cpp:18940-19010`): si el byte en `0x82375610` es distinto de 0 y la funcion `0x8208C1A8(0x821D4838)` devuelve distinto de 0, llama a `sub_821D7D18(4)` (linea 18993, pantalla de titulo) y devuelve 1. [I] para el significado del byte.

### 3.3 Los consumidores no tienen llamador directo

Hay 81 funciones que llaman a la compuerta (`sub_821D3888` o `sub_821D7FE8`) en `generated\dbz3_recomp.1..43.cpp`. Las 26 consumidoras de la seccion 4.1 no tienen llamador `bl` directo (busqueda de `bl` en todo el codigo). [C] Se invocan por puntero, presumiblemente desde `NODO`. [I] `sub_821D7D18` si tiene llamadores directos (seccion 4.2).

---

## 4. Escritores del byte de pantalla (`stb` a `0x824BA110`)

Columnas: funcion, linea del `stb` a la pantalla, valor escrito, nodo que se instala (`obj+28`, linea del `stw`), funcion de entrada (`bl`), notas.

Los valores "por valor" son los de cada rama del consumidor, verificados con simulacion de la cadena de comparaciones (seccion 5).

### 4.1 Consumidores de la cola (aplican `REQ_CUR`)

| Funcion | Linea del `stb` | Valor escrito | Nodo instalado | Entrada (`bl`) | Notas |
|---|---|---|---|---|---|
| `sub_821D4BE8` | `1.cpp:18346` | `REQ_CUR` | `0x821D78F8` (18350) | `0x821D3E18` (18352) | Siempre instala 78F8 |
| `sub_821D4190` | `9.cpp:18361` | `REQ_CUR` | 8 -> `0x821D7040` (18371) | `0x821C9240` (18374) | Solo la 8 cambia nodo |
| `sub_821D4238` | `20.cpp:18841` | `REQ_CUR` | `0x821D78F8` (18845) | `0x821CB328` (18847) | |
| `sub_821D4478` | `32.cpp:18532` | `REQ_CUR` | 19 -> `0x821D63F0` (18552); 5 -> `0x821D7C30` (18565) | `0x820FE9B0` (18568) | Pone pagina 0 (18563) |
| `sub_821D4548` | `10.cpp:18849` | `REQ_CUR` | `0x821D7C30` (18861) | `0x82192568` (18863) | Pone pagina 0 (18859) |
| `sub_821D45F8` | `35.cpp:18585` | `REQ_CUR` | 5 -> `0x821D7C30` (18605) | `0x82145588` (18608) | Pone pagina 0 (18603) |
| `sub_821D46B0` | `28.cpp:18909` | `REQ_CUR` | 24 -> `0x821D7400` (18929); 5 -> `0x821D7C30` (18942) | `0x82308398` (18945) | Pone pagina 0 (18940) |
| `sub_821D4A80` | `29.cpp:18898` | `REQ_CUR` | 2 -> `0x821D48C8` (18932); 3 -> `0x821D4930` | `0x8219F7B8` (18935) | |
| `sub_821D5100` | `10.cpp:18941` | `REQ_CUR` | `0x821D48C8` (18945) | `0x8218AF70` (18947) | |
| `sub_821D5880` | `29.cpp:19014` | `REQ_CUR` | 15 -> `0x821D53D0` (19034); 5 -> `0x821D7C30` (19047) | `0x821AD998` (19050) | Pone pagina 0 (19045) |
| `sub_821D57E0` | `38.cpp:18710` | `REQ_CUR` | `0x821D5368` (18714) | `0x821618D8` (18716) | |
| `sub_821D5950` | `22.cpp:18918` | `REQ_CUR` | 18 -> `0x821D5440` (18938); 5 -> `0x821D7C30` (18951) | `0x821A13C8` (18954) | Pone pagina 0 (18949) |
| `sub_821D5A20` | `7.cpp:18831` | `REQ_CUR` | 20, 21 -> `0x821D54B8` (18863); 5 -> `0x821D7C30` (18876) | `0x8219A330` (18879) | Pone pagina 0 (18874) para 5 |
| `sub_821D5B00` | `17.cpp:18944` | `REQ_CUR` | 24 -> `0x821D7400` (18976); 27 -> `0x821D5600`; 5 -> `0x821D7C30` (18989) | `0x82178AB8` (18992) | Pone pagina 0 (18987) |
| `sub_821D5C38` | `11.cpp:19132` | `REQ_CUR` | 6 -> `0x821D78F8` (19180); 8 -> `0x821D7040`; 10 -> `0x821D5260`; 13 -> `0x821D5778` | `0x820FE9B0` (19183) | Tabla por rama (sim) |
| `sub_821D6080` | `33.cpp:18972` | `REQ_CUR` | `0x821D5BE0` (18976) | `0x82178EC0` (18978) | Nodo fijo |
| `sub_821D6648` | `25.cpp:19068` | `REQ_CUR` | 17 -> `0x821D6368` (19088); 5 -> `0x821D7C30` (19101) | `0x821A8498` (19104) | Pone pagina 0 (19099) |
| `sub_821D6718` | `12.cpp:18352` | `REQ_CUR` | 24 -> `0x821D7400` (18384); 26 -> `0x821D64A8`; 5 -> `0x821D7C30` (18397) | `0x820FE9B0` (18400) | Pone pagina 0 (18395) |
| `sub_821D6868` | `19.cpp:18986` | `REQ_CUR` | 6 -> `0x821D78F8` (19034); 9 -> `0x821D65B8`; 11 -> `0x821D6268`; 12 -> `0x821D5368` | `0x821C1818` (19037) | Tabla por rama (sim) |
| `sub_821D70F8` | `37.cpp:18490` | `REQ_CUR` | 25 -> `0x821D6D60` (18508); 23 -> `0x821D7878` (18517) | `0x8216BF78` (18523); en 23 tambien `0x821D9860` (18519) | |
| `sub_821D71C0` | `10.cpp:19102` | `REQ_CUR` | 6 -> `0x821D78F8` (19128); 8 -> `0x821D7040` | `0x821D3BC0` (19131) | |
| `sub_821D75F0` | `32.cpp:18797` | `REQ_CUR` | 7 -> `0x821D74A0` (18829); 8 -> `0x821D7040`; 5 -> `0x821D7C30` (18842) | `0x821D0810` (18845) | Pone pagina 0 (18840) |
| `sub_821D7520` | `2.cpp:19085` | `REQ_CUR` | 24 -> `0x821D7400` (19105); 5 -> `0x821D7C30` (19118) | `0x82172E28` (19121) | Pone pagina 0 (19116) |
| `sub_821D7990` | `35.cpp:18856` | `REQ_CUR` | ver 5.2 | `0x8218BA00` (18968) | Pone pagina segun la pantalla (5.2) |
| `sub_821D42D8` | `40.cpp:18733` | `REQ_CUR` | 14 -> `0x821D62E8` (18753); 5 -> `0x821D7C30` (18766) | `0x820FE9B0` (18769) | Pone pagina 0 (18764) |
| `sub_821D43A8` | `43.cpp:18830` | `REQ_CUR` | 17 -> `0x821D6368` (18850); 5 -> `0x821D7C30` (18863) | `0x820FE9B0` (18866) | Pone pagina 0 (18861) |

Notas de la tabla:
- "Pone pagina 0" indica un `stb` a `0x824BA111` con valor 0 en la misma funcion.
- `sub_821D4780` (`2.cpp:18816`) no escribe la pantalla. Escribe `REQ_CUR = 6` directamente (`stw r10,-22128(r11)`) y pone nodo `0x821D3E00` (`2.cpp:18849`) [?].

### 4.2 Escrituras directas (sin cola)

| Funcion | Linea del `stb` | Valor | Nodo | Disparador | Estado |
|---|---|---|---|---|---|
| `sub_821D7D18(modo)` | `11.cpp:19524` (modo 5) | `5` (menu principal) | `0x821D7C30` (19528) | Modo 5 desde `14.cpp:19538` (`sub_821D6A08`), `22.cpp:18839`, `3.cpp:18983` y `19102`, `4.cpp:18841`, `9.cpp:18483` | [C] |
| `sub_821D7D18(modo)` | `11.cpp:19545` (modo 4) | `4` (titulo) | `0x821D56F0` (19549) | Modo 4 desde `33.cpp:18750` (`sub_821D4140`) y `36.cpp:18993` (`sub_821D7FE8`) | [C] |
| `sub_821D7D18(modo)` | `11.cpp:19589` (modo 1) | `1` | `0x821D5BE0` (19593) | Modo variable desde `37.cpp:18105` (`sub_821D3888`, con `UI_PEND_B`) | [C] |
| `sub_821D6B00` | `8.cpp:18423` | `0` | `0x821D67F8` (18429) | Sin llamador `bl` directo | [?] |

`sub_821D7D18` primero deja la cola vacia: `REQ_CUR = -1` (19456), `REQ_NEXT = -1` (19472), `FLAG_A = 0` (19468), `REQ_ARG = 0` (19476), y `UI_PAGE = 0` (19598). [C]

### 4.3 Escrituras de palabra completa que incluyen la pantalla [?]

Escriben una palabra de 4 bytes en `0x824BA110`. El byte 0 es la pantalla. Si el valor escrito tiene byte 0 igual a 0, la pantalla queda a 0. [I]

| Funcion | Lineas | Valor | Comentario |
|---|---|---|---|
| `sub_821B8808` | `17.cpp:18341`, `18345` | `stw r28,0(r30)` con `r28 = 0` (18345) | Limpia la palabra |
| `sub_821692B0` | `19.cpp:12759` | `stw r11,0(r31)` con `r11 = 1` | Pone byte 0 = 0 y byte 3 = 1 |
| `sub_8214E468` | `35.cpp:11733`, `11752`, `11780`, `11788` | valores 0 y 1 (11788: 0) | Escrituras de palabra [?] |

### 4.4 Total

30 `stb` a `0x824BA110` en 28 funciones (26 de 4.1 y 2 de 4.2; `sub_821D7D18` tiene 3 `stb`). [C] Mas 3 funciones de escritura de palabra (4.3). [I]

---

## 5. Pantalla -> nodo (handler)

### 5.1 Nodo por pantalla

El nodo es la direccion que se guarda en `obj+28`. Cada direccion coincide con el inicio de una funcion `sub_<direccion>` de `generated\dbz3_recomp.*.cpp`. Los nombres de la columna "Pantalla" vienen de la lista conocida del encargo; los marcados [?] no tienen nombre confirmado.

| Pantalla | Nombre | Nodo (`obj+28`) | Donde se instala (archivo:linea) | Consistencia |
|---|---|---|---|---|
| 0 | reset | `0x821D67F8` | `8.cpp:18429` | [C] unico |
| 1 | [?] | `0x821D5BE0` | `11.cpp:19593` (modo 1 de `sub_821D7D18`) | [C] |
| 2 | [?] | `0x821D48C8` | `29.cpp:18932`, `10.cpp:18945` | [C] |
| 3 | [?] | `0x821D4930` | `29.cpp` (`sub_821D4A80`, rama 3) | [C] |
| 4 | Titulo | `0x821D56F0` | `11.cpp:19549` (modo 4 de `sub_821D7D18`); `35.cpp:18793-18981` (`sub_821D7990`, rama 4) | [C] |
| 5 | Menu principal | `0x821D7C30` | casi todos los consumidores (4.1) y `11.cpp:19528` | [C] muy consistente |
| 6 | DU titulo / menu | `0x821D78F8` | `1.cpp:18350`, `20.cpp:18845`, `11.cpp:19180`, `19.cpp:19034`, `10.cpp:19128`, `35.cpp:18913` | [C] consistente |
| 7 | Rueda de personajes | `0x821D74A0` | `32.cpp:18829` | [C] |
| 8 | DU mapa / escenas | `0x821D7040` | `9.cpp:18371` (`sub_821D4190`); `11.cpp`, `10.cpp`, `32.cpp` por rama | [C] consistente |
| 9 | Batalla | `0x821D65B8` | `19.cpp` (`sub_821D6868`, rama 9) | [C] |
| 10 | Subida de nivel | `0x821D5260` | `11.cpp` (`sub_821D5C38`, rama 10) | [C] |
| 11 | [?] | `0x821D6268` | `19.cpp` (`sub_821D6868`, rama 11) | [C] |
| 12 | Creditos | `0x821D5368` | `38.cpp:18714` (`sub_821D57E0`); `19.cpp` (rama 12) | [C] |
| 13 | [?] | `0x821D5778` | `11.cpp` (`sub_821D5C38`, rama 13) | [C] |
| 14 | Duelo (menu) | `0x821D62E8` | `40.cpp:18753` (`sub_821D42D8`); `35.cpp:18793-18981` (rama 14) | [C] consistente |
| 15 | [?] | `0x821D53D0` | `29.cpp:19034` (`sub_821D5880`) | [C] |
| 16 | [?] | `0x821D6CD8` | `35.cpp:18793-18981` (rama 16) | [C] |
| 17 | [?] | `0x821D6368` | `43.cpp:18850`, `25.cpp:19088` | [C] |
| 18 | [?] | `0x821D5440` | `22.cpp:18938` (`sub_821D5950`) | [C] |
| 19 | Practica (menu) | `0x821D63F0` | `32.cpp:18552` (`sub_821D4478`); `35.cpp:18793-18981` (rama 19) | [C] consistente |
| 20 | Practica (combate) | `0x821D54B8` | `7.cpp:18863` (`sub_821D5A20`) | [C] |
| 21 | Practica (combate) | `0x821D54B8` | `7.cpp:18863` (`sub_821D5A20`) | [C] |
| 22 | [?] | `0x821D5590` | `35.cpp:18965` (`sub_821D7990`, rama 22) | [C] |
| 23 | [?] | `0x821D7878` | `37.cpp:18517` (`sub_821D70F8`); `35.cpp:18793-18981` (rama 23) | [C] consistente |
| 24 | [?] | `0x821D7400` | `28.cpp:18929`, `17.cpp:18976`, `12.cpp:18384`, `2.cpp:19105` | [C] consistente |
| 25 | Duelo (arranque) [I] | `0x821D6D60` | `37.cpp:18508` (`sub_821D70F8`) | [C] nodo; [?] nombre |
| 26 | [?] | `0x821D64A8` | `12.cpp` (`sub_821D6718`, rama 26) | [C] |
| 27 | [?] | `0x821D5600` | `17.cpp` (`sub_821D5B00`, rama 27) | [C] |
| 28 | Opciones | `0x821D5528` | `35.cpp:18793-18981` (rama 28) | [C] |

Conflicto con la documentacion previa: `docs\08_online\13_duelo_online.md` dice que la pantalla 25 la consume `sub_821D7400`. Aqui `sub_821D7400` es el nodo de la pantalla 24 (en 4 consumidores), y la 25 instala `0x821D6D60`. [?] Hay que medir en juego cual es correcto.

### 5.2 Pagina de menu al entrar en la pantalla (`sub_821D7990`)

`sub_821D7990` (`generated\dbz3_recomp.35.cpp:18793-18981`) pone `UI_PAGE` segun la pantalla que entra:

| Pantalla | Pagina | Donde |
|---|---|---|
| 6 | 1 | `35.cpp:18911` |
| 14 | 2 | bloque 18913-18981 (sim) |
| 16 | 3 | bloque 18913-18981 (sim) |
| 23 | 6 | bloque 18913-18981 (sim) |

La pantalla 19 (menu de Practica) no fija pagina en esta funcion. La pagina 4 la escriben `sub_8219ADB0` (`5.cpp:15585`, valor 4) y `sub_821D63F0` (`15.cpp:18833`, valor 4) [C]. Quien llama a esas dos funciones no se ha localizado [?].

### 5.3 Interruptor de entrada (`sub_821D7C30`)

`sub_821D7C30` (`generated\dbz3_recomp.27.cpp:18818-18956`) es el nodo de la pantalla 5. Si hay una peticion (`REQ_CUR != -1`, comparacion en 18856-18858), instala `NODO = 0x821D7AE8` (18864) y llama a una funcion de entrada segun `REQ_CUR`:

| `REQ_CUR` | Funcion de entrada | Linea |
|---|---|---|
| 4 | `0x8219F658` | 18908 |
| 6 | `0x821D0BE8` | 18902 |
| 14 | `0x821AD130` | 18896 |
| 16 | `0x821A80D8` | 18890 |
| 19 | `0x82199D70` | 18914 |
| 22 | `0x821461E0` | 18944 |
| 23 | `0x821728A8` | 18938 |
| 28 | `0x82198FB8` | 18932 |

Esto es un despacho por valor de pantalla (cadena de comparaciones), no una tabla de salto. [C]

---

## 6. Conectores: quien pide cada pantalla

Llamadas `bl sub_821D2778` (40 sitios). El valor de `r3` es la pantalla pedida, tomado de la instruccion `li r3,N` previa. `r4` es el argumento.

| Funcion (quien pide) | Archivo:linea | Pantalla pedida | `r4` |
|---|---|---|---|
| `sub_8216E878` | `0.cpp:13500` | 23 | 1 |
| `sub_82197020` | `1.cpp:14923` | 5 | 0 |
| `sub_8218C5D8` | `10.cpp:14956` | 6 | 0 |
| `sub_82173370` | `11.cpp:13871` | 24 | 0 |
| `sub_82173370` | `11.cpp:13973` | 5 | 0 |
| `sub_82147BE0` | `13.cpp:11043` | 5 | 0 |
| `sub_8219F9E8` | `13.cpp:16199` | 2 | 0 |
| `sub_82160A70` | `17.cpp:12400` | 12 | 0 |
| `sub_82148BB0` | `18.cpp:10993` | 5 | 0 |
| `sub_8218AFC8` | `18.cpp:14809` | 2 | 0 |
| `sub_821AC340` | `18.cpp:16781` | 20 | 0 |
| `sub_821AF298` | `19.cpp:16757`, `16843` | 4 | 0 |
| `sub_821C1B30` | `20.cpp:17897` | 6 | - |
| `sub_821C1B30` | `20.cpp:17907` | 12 | - |
| `sub_821C1B30` | `20.cpp:17917` | 11 | - |
| `sub_821C1B30` | `20.cpp:17927` | 9 | - |
| `sub_821C92A0` | `22.cpp:18233` | 8 | 0 |
| `sub_82178FD0` | `23.cpp:13901` | 1 | 0 |
| `sub_8216DB58` | `26.cpp:12949` | 23 | 0 |
| `sub_8219AF00` | `26.cpp:15729` | 5 | 0 |
| `sub_821AE440` | `26.cpp:16570` | 5 | 0 |
| `sub_82147738` | `27.cpp:10951` | 5 | 0 |
| `sub_8216C5B8` | `27.cpp:12945` | 23 | 2 |
| `sub_821A5A30` | `28.cpp:17326` | 18 | 0 |
| `sub_821CA9D0` | `29.cpp:18286` | 6 | 0 |
| `sub_82178C10` | `3.cpp:13748` | 27 | - |
| `sub_8219B1C0` | `32.cpp:15478` | 21 | 0 |
| `sub_821C9DC8` | `32.cpp:17998` | 8 | 0 |
| `sub_821AA130` | `34.cpp:15991` | 5 | 0 |
| `sub_821D1150` | `36.cpp:18632` | 5 | 0 |
| `sub_821D1150` | `36.cpp:18764` | 7 | - |
| `sub_821A1018` | `39.cpp:16137` | 5 | 0 |
| `sub_82175D60` | `4.cpp:13600` | 24 | 0 |
| `sub_821D2168` | `41.cpp:18559` | 5 | 0 |
| `sub_8218C520` | `42.cpp:15117` | 4 | 1 |
| `sub_821AA510` | `5.cpp:16374` | 17 | 255 |
| `sub_82177300` | `7.cpp:13502` | 25 | 0 |
| `sub_821CA020` | `7.cpp:18230` | 6 | 0 |
| `sub_821CA020` | `7.cpp:18368` | 8 | 0 |

"-" = `r4` no fijado en la ventana de 14 instrucciones.

Ademas, `sub_821D3628` mete peticiones en linea (seccion 7): 5, 8, 10, 14, 17, 19, 24, 26. [C]

Pendiente: ninguna de las 40 llamadas pide 14, 19 o 28. Esas pantallas se piden solo desde `sub_821D3628`, segun la pagina de menu. [C]

---

## 7. Mapeo del menu (`sub_821D3628`)

Fichero: `generated\dbz3_recomp.14.cpp:19087-19404`. Entrada: `r3 = R` (resultado del menu, `MENU_R`). Pagina = `UI_PAGE` (`0x824BA111`, lectura en 19099-19104).

"Empujar X" = poner X en la cola (`REQ_CUR` si esta libre, si no `REQ_NEXT`) y poner `REQ_ARG` al valor indicado.

| Pagina | R = 2 | R = -1 | Otro R | Bloque (lineas) |
|---|---|---|---|---|
| 1 | empujar 5 | empujar 10 | llama `0x82156BA8` y empuja 8 | 19329 (37D8), 19363 |
| 2 | empujar 5 | empujar 14 | empujar 14 | 19307 (37B0) |
| 3 | empujar 5 | empujar 17 (arg = R) | empujar 17 (arg = R) | 19274 (3774), 19301-19303 |
| 4 | empujar 5 | empujar 19 (arg 1) | empujar 19 (arg 1) | 19241 (373C), 19270 |
| 5 | empujar 5 | empujar 19 (arg 3) | empujar 19 (arg 2) | 19183 (36D4), 19209-19210, 19237 |
| 6 | empujar 5 | empujar 26 | empujar 24 | 19130 (bloque de pagina 6), 19165-19166 |
| 0, 7 o mas | sin accion | sin accion | sin accion | 19128 (`bne` a 384C) |

Llamadores de `sub_821D3628`: solo `sub_820FEBA8` (`generated\dbz3_recomp.22.cpp:6836`). Ahi `R` se lee de `0x823D6CE0` (linea 6834). [C]

Lo que no esta resuelto [?]:
- El significado de `R` (valores que escribe `MENU_R`: 4, 2, -1 y 0/1 segun la rama) y de las paginas 1..6.
- Como el menu principal (pantalla 5, nodo `sub_821D7C30`, pagina 0) traduce el item elegido. Con pagina 0 la tabla no hace nada, asi que los items del menu principal no pasan por aqui. Los candidatos son los manejadores de entrada de `MENU_R` (`sub_820FF750`, `sub_820FF578`, `sub_820FFFC8`, `sub_820FF1B8`), que contienen saltos por tabla (`bctr`). No se han recorrido.
- Quien pone pagina 2 (Duelo) o 4/5 (Practica) desde el menu principal.

---

## 8. Lectores del byte de pantalla

| Funcion | Linea de lectura | Que hace con la pantalla | Estado |
|---|---|---|---|
| `sub_821005F8` | `24.cpp:7028` (`lbz r11,-24304(r11)`) | Si la pantalla esta en {3, 9, 15, 18, 20, 21, 25}, decrementa un contador en `+48` del objeto (7061-7067) y llama a 4 funciones (7075-7102). | [C] lectura; [?] significado del contador |
| `sub_820FEBA8` | `22.cpp:6775` | Mapea pantalla: 9 -> 0, 15 -> 1, 18 -> 2, 20/21 -> 3, 25 -> 6 (6779-6825). Escribe `0x824BA135` (6826). Llama al router de menu `sub_821D3628(R)` (6836) y a `sub_821D27A8` (6839). | [C]; [?] si es un nodo |
| `sub_8215B4D0` | `14.cpp:12490` | Si la pantalla es 8, pone flags en la estructura apuntada por `lis -32200 + 25184` (12500-12505) y un valor flotante (12515). | [C] escritura; [?] estructura |
| `sub_820E63E8` | `4.cpp:5739`, `5845` | Lee la palabra completa (`lwz 0(r31)`) y compara con 0x20000 y 0x2. | [?] |
| `sub_820E5D18` | `6.cpp:5731` | Lee la palabra completa. | [?] |
| `sub_820E4820` | `13.cpp:5573`, `5729`, `5859` | Lee la palabra completa. | [?] |
| `sub_820E4EE0` | `39.cpp:5828` | Lee la palabra completa. | [?] |
| `sub_821D3628` | `14.cpp:19099-19104` | Lee `UI_PAGE` (seccion 7). | [C] |
| `sub_821D3888` | `37.cpp:18083-18103` | Lee `UI_PEND_A` y `UI_PEND_B` (seccion 3.2). | [C] |

Busqueda de tabla de salto: **no se encontro** un patron `lbz(0x824BA110) -> rlwinm/mulli -> lwzx -> mtctr -> bctrl`. Los dos `lbz` exactos a la pantalla (`24.cpp:7028`, `14.cpp:12490`) no van seguidos de salto. Los `bctr` cercanos (`33.cpp:6998`, `31.cpp:6994`) son saltos de objeto (`lwz 24(rX)`), no tablas por pantalla. [C]

---

## 9. Lo que falta para cerrar la tabla

1. **Datos de los handlers.** Hay que leer la seccion de datos (`.rdata`) en las direcciones de la seccion 5.1 para conocer las funciones reales de cada nodo. `default.xex` esta cifrado (`encryption_type=1`). Con el ejecutable en modo herramienta (como `tools\extract_jt.cpp`) se puede volcar la memoria en esas direcciones. [I]
2. **Medicion en juego.** Registrar `PANTALLA`, `REQ_CUR`, `REQ_NEXT` y `UI_PAGE` en cada transicion para confirmar la pantalla 25 (duelo) y los items del menu principal. [I]
3. **Menu principal.** Localizar el manejador de entrada de `MENU_R` (`sub_820FF750`, `sub_820FF578`, `sub_820FFFC8`, `sub_820FF1B8`) y el cursor del menu principal. [?]
4. **Escritores de `UI_PEND_A/B`** (`0x824BA185/186`) y de la compuerta `0x82375610`. No se han buscado. [?]
5. **Palabra completa** (seccion 4.3): comprobar si esas escrituras son reinicios reales de pantalla. [?]
