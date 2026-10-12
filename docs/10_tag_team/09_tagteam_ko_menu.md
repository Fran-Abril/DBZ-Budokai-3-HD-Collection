# Estudio 09: KO, animaciones y menú para Tag Team

Solo lectura. No se ha editado nada del proyecto ni se ha ejecutado el juego.
Raíz: la raíz del proyecto
Código: `generated\dbz3_recomp.N.cpp` (US). Las líneas son las del fichero; la función es la de `DEFINE_REX_FUNC` que la contiene.
Convención: **[C]** = comprobado leyendo la instrucción PPC o el código de `src`/`docs`. **[I]** = inferido por uso, patrón o nombre. **[N]** = no confirmado.

Base previa: `02b_tag_team_fase1.md` (bloque X_k = cfg+2096+8240·k; vida X+6080 medida en juego) y `03_survival.md`.

## Resumen

- **KO localizado [C]:** el punto de decisión está en `sub_8211A5A8` (`dbz3_recomp.41.cpp:8281-9112`). Es el único escritor de daño y curación que se usa para golpes normales (20 sitios de llamada en al menos 9 funciones). La rama de KO está en las líneas 9003-9068.
- **Umbral de KO [I]:** la constante C1 (float en `0x8201F60C`) hace de vida mínima. Se supone 0.0; hay que medirla en juego.
- **Casi muerto [C] en código:** en vez de KO, la función puede dejar la vida en un valor `f30` (float en `0x820009FC`). Se controla con el byte X+8203 y el flag 0x4 de los argumentos.
- **Fin de combate: no localizado [N].** El KO se comunica con `sub_82100FA8(0, payload)`, un despachador por tabla de saltos (`0x8201EEE0`) cuyo manejador 0 no he decodificado.
- **Animaciones [C] en mecanismo, [N] en índices:** el reproductor es `sub_82136328(instancia, índice 0-63)`. Los índices de KO, levantarse, intro y teletransporte no aparecen como constantes; vienen de datos.
- **Mecha Freezer [C] en tabla:** es la forma 5 "Frieza's Space Ship" de Freeza (ID 27, cápsula 405, tipo 07 automático). Su disparador no está confirmado [N].
- **Senzu [C] en lista de objetos:** existen Senzu Bean, Senzu Root, Senzu Leaf y Senzu Seeding. No hay código de revivir localizado [N].
- **Menú [C] parcial:** la máquina de modo usa el global `0x8247A990` (lanza el duelo con el valor 25 en `sub_821D7400`). Las entradas del menú principal no están localizadas [N].
- **Propuesta de mínimo riesgo:** un hook sobre `sub_8211A5A8` (uno solo, con todos los llamadores detrás) más una casilla "Tag Team" en el overlay F10 (`src\ingame\menu.cpp:26`), sin tocar el menú del guest.

## 0. Nota sobre IDs de personaje y correcciones a estudios previos

- **Corrección a `03_survival.md`:** la línea "definición en `generated/dbz3_recomp.5.cpp:6912`" referida a `sub_821007E8` es errónea. `sub_821007E8` está en `dbz3_recomp.20.cpp:6982`. `dbz3_recomp.5.cpp:6912` es `sub_820FE668` (crear duelo) [C].
- Otras referencias de este estudio: `sub_820FE668` = `dbz3_recomp.5.cpp:6912`; `sub_821D7400` = `dbz3_recomp.22.cpp:18970`; `sub_82100FA8` = `dbz3_recomp.22.cpp:6916`; `sub_82119108` = `dbz3_recomp.15.cpp:8286`; `sub_820FB5D0` = `dbz3_recomp.41.cpp:6442`; `sub_82110640` = `dbz3_recomp.26.cpp:7661`.

Los IDs de `char372`/`X+7420` coinciden con la tabla `04_transformaciones\evid_tabla_b3.md` (ID 0 Goku, 2 Gohan S, 7 Vegeta, 15 Videl, 27 Freeza, 42 Saibaman, 91 Goku SSJ4) [C].

**Aviso:** la primera tabla de `docs\03_formatos\MAPA_ROSTER_HD.md` (líneas 45-85) usa otro índice ("AURA": 7 = Buu gordo, 42 = Vegito). No mezclar las dos. Este estudio usa la de transformaciones.

## 1. KO: vida, escritores y punto de decisión

### 1.1 Accesos al campo de vida X+6080 [C]

Búsqueda: `6080(rN)` en lfs/stfs/lwz/stw de `generated\dbz3_recomp.*.cpp`. Resultado: 41 accesos en 15 ficheros. Las ocurrencias `-26080` son otro desplazamiento y se descartan. No aparece el literal `0x17C0` como offset (sí como dirección de salto, sin relación).

Escritores (sobre X+6080, sin contar copias):

| Función (nombre original) | Archivo:línea | Qué hace | Estado |
|---|---|---|---|
| `sub_82109638` (reset de luchador) | `dbz3_recomp.25.cpp:7865-7884` | HP = 3000 (literal `li r11,3000`). Copias +6092 (7887) y +6096 (7919). Lo llama `sub_82110640` (según 02b) | [C] |
| `sub_8211DAC0` (acción de evento "vida al máximo") | `dbz3_recomp.39.cpp:8607-8634` | Jugador p = `*(*(ev+44)+0)`. Puntero X_p = `*(cfg + 35188 + 4p)` (`8797·4 = 35188`). HP = X+6084 | [C] código; [I] llamada por tabla de eventos (sin llamador directo) |
| `sub_82116808` (restaurar vida condicional) | `dbz3_recomp.41.cpp:8070-8277` (escritura 8263) | Pone en cero contadores por ranura (`cfg+72·k+35262/35264/35266`). HP = X+6084 solo si X+8184 ≠ 0xFF y X+8201 ≠ 0 | [C] código; [I] significado |
| **`sub_8211A5A8`** (aplicar delta de vida + KO) | `dbz3_recomp.41.cpp:8281-9112` (escrituras 8453, 8466, 8967, 9037) | Núcleo de daño/curación. Ver 1.2 | [C] |
| `sub_82118A50` | `dbz3_recomp.6.cpp:8309-8664` (escrituras 8424, 8582) | HP con tabla de floats (`lfsx`). Rol no decodificado | [N] |

Lectores de X+6080 (solo para ubicar consumidores, sin descifrar todos):
`sub_821024A8` (10.cpp:7047), `sub_82113518` (10.cpp:8010), `sub_821100B0` (12.cpp:7706; copia la vida a +6092/+6096, es la barra), `sub_8213EA28` (12.cpp:10356; compara con f30 y con X+6084, [I] IA/HUD), `sub_8213CAD8` (18.cpp:10444), `sub_82113AA8` (22.cpp:8014; predicado, ver 1.3), `sub_8211E030` (22.cpp:8577; lectura de vida como entero, [C]), `sub_8213FFF0` (23.cpp:10805), `sub_8213C888` (26.cpp:10344), `sub_8211C660` (3.cpp:8605; ver 1.3), `sub_8211C098` (30.cpp:8442; estado por luchador, cápsula X+8154, flag X+8164, pendiente X+8184), `sub_820BA388` (35.cpp:3093), `sub_820FD678` (39.cpp:6842), `sub_82140BF8` (6.cpp:10636), `sub_8213C6F8` (5.cpp:10574), `sub_82115A20` (9.cpp:8112), `sub_8213FB08` (9.cpp:10195).

Campos de cfg por jugador usados en la rama de KO (u16, `cfg + 72·ranura + offset`): offsets 35262, 35264, 35266 y 35308 [C escritura]. Son escritos en `sub_82116808`, `sub_821166A8` (27.cpp:8250-8333), `sub_82116458` (40.cpp:8390-8475) y `sub_8211A5A8`. Lector no localizado [N].

### 1.2 Función de daño `sub_8211A5A8` (llamada `AplicarVida` en este estudio)

Argumentos [C]: r3 = X (`mr r31,r3`, línea 8305); r4 = delta entero (`mr r23,r4`, 8307); r5 = flags (`mr r20,r5`, 8311).

Constantes (direcciones calculadas con `lis` + desplazamiento [C]; valores [N] hasta medirlos):
- **C1** = `lfs f13,-2548(r9)` con r9 = `0x82020000` → float en `0x8201F60C` (línea 8998). Umbral de KO. Se supone 0.0 [I].
- **f30** = `lfs f30,2556(r11)` con r11 = `0x82000000` → float en `0x820009FC` (línea 8313). Vida de "casi muerto" [I].
- **C2** = `lfs f13,-19472(r11)` en `sub_82113AA8`, float en `0x8201B3F0` (22.cpp:8049).
- **Máximo** = X+6084 (float; 3000.0 en juego según 02b §6).

Flujo, en orden:

1. **Daño / curación** (8945-8967): HP -= delta (la conversión entera a float está en 8943-8963). Activa el byte X+8221 (`stb r29,8221`, línea 8953).
2. **Rama de curación** (8430-8466, si delta < 0): HP -= delta, con tope en X+6084 (`fcmpu` en 8454-8457). Entra solo si una tabla previa vale 4 (8422-8427) [I].
3. **Comprobación de KO** (9001-9003): `fcmpu HP, C1`; si HP ≤ C1 salta a 9010 (`aa90`).
4. **Bit 0x400 de flags** (9005-9008, `rlwinm r11,r20,0,21,21`): si está activo, también entra en `aa90`, aunque HP > C1. Con flags = 3072 (0xC00), que usa `sub_8211C660`, siempre entra [C].
5. **Clamp** (9010-9034):
   - Acumula `f31 += HP` (`fadds`, 9014).
   - Si flags & 0x4 o X+8203 ≠ 0 → HP = **f30** (9019-9031). Es la vía de "casi muerto": no hay KO.
   - Si no → HP = **C1** (9025-9027).
   - Guarda HP (9034-9037).
6. **Decisión** (9039-9045): recarga HP y lo compara con C1.
   - HP > C1 → rama viva (`aaf0`, 9070-9084): llama `sub_82113AA8(X, flags)`. Si devuelve ≠ 0, X+8184 = -1 (cancela una transformación pendiente).
   - **HP ≤ C1 → KO (9047-9068):** escribe -1 en un array local de u16 indexado por la ranura (`X+4944`, línea 9051 `li r9,-1`) y llama `sub_82100FA8(0, &máscara)` (9065). Después vuelve (9068).

Interpretación del KO [I]: `sub_82100FA8` recibe en r4 un puntero a un payload con una máscara de bits por ranura (en r1+80) y los valores u16 por ranura (en r1+88). La rama de curación (8483-8493) llama al mismo despachador con valor 0 y la del KO con -1. Por eso se supone "estado de ranura: 0 = vivo, -1 = KO". Hay que confirmarlo.

Lo que devuelve: `(int)f31` (9087-9093). No hay más uso necesario para Tag Team.

Llamadores de `sub_8211A5A8` [C] (20 sitios; funciones únicas): `sub_821051C8` (21.cpp:7316), `sub_821060E8` (10.cpp:7752), `sub_8210ED48` (38.cpp:8433), `sub_8211ABB0` (19.cpp:8628), `sub_8211BA50` (24.cpp:8908 y otros 4 sitios), `sub_8211C660` (3.cpp:8651), `sub_8211DB68` (23.cpp:8908), `sub_8211DC48` (18.cpp:8611), `sub_82132868` (25.cpp:9827), y varios sitios más en 38.cpp (8821, 8927, 9346, 9700, 9875, 10037).

### 1.3 Casos especiales (no son la vía general)

- **`sub_8211C660`** (`dbz3_recomp.3.cpp:8605-8660`): solo actúa si X+7420 == **42** (Saibaman, según evid_tabla). Si no, retorna sin tocar la vida (línea 8630, salto a 8798). Para Saibaman llama a `sub_82119090`, luego `sub_8211A5A8(X, (int)HP, 3072)`. Con delta positivo y flags 0xC00, el resultado es HP = C1 → KO. Coincide con su forma "Self-Destruct" (tipo 07) [I]. Llamadores: `sub_8212D440` (20.cpp:9713) y `sub_8212E1C0` (34.cpp:9953).
- **`sub_82113AA8`** (`dbz3_recomp.22.cpp:8014-8072`): predicado. Devuelve 1 solo si X+7420 == **7** (Vegeta), flags & 0x1800 == 0, X+8154 > 0 (cápsula), X+8164 == 0, HP < C2 y HP < [X+4948]+6096 (barra del otro jugador, [I]). Efecto en el llamador: X+8184 = -1 (línea 9084).

### 1.4 Dónde termina el combate [N]

No localizado. Lo que se sabe [C]:
- El KO se comunica al despachador `sub_82100FA8` (`dbz3_recomp.22.cpp:6916`). Es un `lis/addi` + `lwzx` + `bctr`: tabla en `0x8201EEE0`, índice = r3 (0, 1, 2 o 3 según el llamador).
- Otros llamadores con op 0: `sub_820FD2B8` (17.cpp:6902), `sub_820FD1A8` (23.cpp:7139), `sub_82117A40` (28.cpp:8424), `sub_820FEF20` (40.cpp:7110), `sub_8211C3B8` (42.cpp:8710/8984). Op 3: `sub_820FF338` (25.cpp:6960). Op 2: `sub_820FD0B8` (41.cpp:6659).
- Lector que compare "todos KO" no encontrado. `sub_820FD678` (39.cpp:6842-7274) lee vida (7138) y X+8221 (7009), y es el bucle por fotograma más cercano; no lo he confirmado como chequeo de fin.

### 1.5 Punto de intercepción propuesto para Tag Team

Un solo punto de control cubre los daños normales: el hook sobre `sub_8211A5A8` (20 llamadores detrás). Patrón de hook del proyecto: `DBZ3_HOOK(sub_X, dbz3eu_sub_Y) { ... orig(ctx, base); ... }` (`src\guest_region.h:40`).

Gemelo EU localizado por el mismo patrón de constantes y escrituras [I]: `dbz3eu_sub_8211A6A8` (`generated_eu\dbz3_eu_recomp.12.cpp:8361`, con escrituras de 6080 en 8501-9085). Confirmar antes de usarlo. Otros gemelos EU (por el mismo patrón): `sub_82109638` → `dbz3eu_sub_82109738`; `sub_82116808` → `dbz3eu_sub_82116908`; `sub_8211DAC0` → `dbz3eu_sub_8211DBC8` [I].

Bosquejo (no escrito):

```
DBZ3_HOOK(sub_8211A5A8, dbz3eu_sub_8211A6A8) {
  const uint32_t X = ctx.r3.u32;     // guardar ANTES: r3 cambia al retornar
  orig(ctx, base);                   // aplica el golpe; si mata, ya ha despachado el KO
  // si HP(X+6080) <= C1 y "Tag Team" activo y hay reserva:
  //   revertir el KO (sub_82100FA8 op 0 con estado 0) [I]
  //   poner HP al máximo o f30 y hacer el relevo (02b §5, paso 2)
}
```

Riesgos: (a) revertir el KO depende de la hipótesis del payload [I]; (b) C1 no medida; (c) el evento de KO puede tener efectos fuera de la vida (contadores de cfg). Alternativa: interceptar `sub_82100FA8` al entrar, cuando r3 = 0 y el estado de la ranura es -1, en vez de dejar que haga el KO. Requiere la misma hipótesis sobre el payload.

## 2. Animaciones y transformaciones

### 2.1 Reproductor de animaciones [C]

- `sub_82136328` (`dbz3_recomp.15.cpp:10045`; hook en `src\roster_ext.cpp:1104-1121`, gemelo EU `dbz3eu_sub_82136430`). Argumentos: r3 = instancia del modelo, r4 = índice de movimiento (0-63).
- Búsqueda: `inst+656` → tabla de movimientos `mt`; `mt+20` → tabla de punteros; `tbl[índice]` debe ser ≠ 0. Si falta, usa el donante (`DonorOf`) o no anima.
- Llamadores directos: 5 (`sub_8217FE48` 11.cpp:14306 con ID del select; `sub_821A7208` 18.cpp:16483 con `[r11+12]`; `sub_821A5868` 19.cpp:16141 con s16 de tabla; `sub_82159D08` 5.cpp:12142 con r30; `sub_821D0548` 7.cpp:18588 con r3). Todos reciben el índice de datos, no constantes [C].

### 2.2 Formato de las animaciones [C, docs]

- Pool de animaciones `#ACM` (`docs\03_formatos\ACM_FORMAT.md` §3): tabla de n_anim entradas de 0x10 bytes; por hueso, datos angulares y posicionales.
- `#CSK` (ACM_FORMAT §4): reacciones al golpe (HR). El daño vive en los bloques HR, indexados por el código HR de los bloques AP tipo 1. **No hay tabla de códigos de caída/KO en la documentación** [N].
- Bin de animación por personaje = columna ANM de `docs\03_formatos\MAPA_ROSTER_HD.md` §4 (líneas 92-130). Ejemplos: Freeza 199-213, Vegeta 433-435 y 437, Vegito 444, Majin Buu gordo 132, Cell 156-159.

### 2.3 Requisitos del relevo: índices concretos [N]

| Animación pedida | Estado | Cómo averiguarla |
|---|---|---|
| KO / caída | no localizada como constante | sonda: registrar r4 de `sub_82136328` en el momento del KO (1.5) |
| Levantarse | no localizada | igual, justo después del KO |
| Casi muerto que se recupera | mecanismo localizado (vida = f30, 1.2 paso 5), animación no | sonda sobre la rama f30 |
| Intro de entrada | no localizada (solo aparece "intro" como vídeo) | sonda sobre sub_82136328 al empezar combate |
| Teletransporte | no localizado | igual |

### 2.4 Senzu [C lista, N efecto]

`docs\07_ports\GOHAN_FUTURO_EXPLORACION_2026-10-06\04_transformaciones\b3_caps.json`:
- 252 "1/3 Senzu Bean" (2 ranuras, Uncommon)
- 253 "1/2 Senzu Bean" (3 ranuras, Rare)
- 254 "Senzu Bean" (4 ranuras, Special)
- 275 "Senzu Root" (4 ranuras, Common)
- 276 "Senzu Leaf" (5 ranuras, Uncommon)
- 277 "Senzu Seeding" (6 ranuras, Rare)

Todas son "Item", "Used", "Everyone can use", con precio en Zenie. No hay texto de efecto. El código recompilado no tiene nombres, así que no se puede buscar por texto. No hay ninguna rama de KO que restaure la vida tras un objeto [C en lo leído]. Un "revive" sería un nuevo evento; no está en el código.

### 2.5 Transformaciones de tipo 07 (automáticas o especiales) [C tabla, N disparador]

Según `04_transformaciones\informe.md:54`, el tipo 07 es "especial/automática (Majin, nave)".

| ID | Personaje | Forma | Nombre | Cápsula | Requisito | Modelo | Tipo |
|---|---|---|---|---|---|---|---|
| 27 | Freeza | 5 | Frieza's Space Ship (**Mecha Freezer**) | 405 | desde forma 0 | id 27 m5 | 07 |
| 7 | Vegeta | 4 | Babidi's Mind Control | 407 | desde 0, 1, 2 | id 7 m2 | 07 |
| 42 | Saibaman | 1 | Self-Destruct | 148 | desde 0 | id 42 m0 | 07 |

- Las formas 1-4 de Freeza (cápsulas 96-99) son tipo 05 (irreversibles). La 5 es tipo 07, con ki base 5.
- **Disparador [N]:** no está confirmado que el paso a la forma 5 dependa de la vida. El código de subida (`sub_82119108`) decide por tipo. Para medirlo: sonda en `sub_82119108` / `sub_820FB5D0` al llegar a Freeza con vida baja.
- El ID 42 tiene una rama propia en `sub_8211C660` (1.3).

## 3. Menú principal y propuesta "Tag Team"

### 3.1 Qué está localizado [C]

- **Textos y páginas del menú** están en `data_eng.afs`, no en el código. La entrada 0 es el composite del select, con 5 secciones `#ACA` que son páginas del menú (personajes, stages, modos) (`docs\03_formatos\AUDITORIA_DATA_CMN.md:110-119`). Los nombres de los menús están en `data_usi` (entradas 2663 cortas y 2684 largas, `CAPSULAS_B3.md:37`).
- **Máquina de modo:** el global `0x8247A990` se lee y escribe con `lwz/stw -22128(rN)`. Lo lee `sub_821D7400` (`dbz3_recomp.22.cpp:18970-19062`): si vale 23 salta a `sub_821728A8`; si vale 25 llama a `sub_820FE668` (crear duelo, línea 19037). Es el nodo que lanza el combate.
- **Otros lectores del mismo estado** (candidatos de menú, rol [N]): `sub_821D38E8` (13.cpp:18688), `sub_821D63F0` (15.cpp:18750), `sub_821D62E8` (28.cpp:19028), `sub_821D6368` (32.cpp:18655), `sub_821D6B88` (21.cpp:18828).
- **Entradas del menú principal (Dragon Universe, Duel, World Tournament, Practice, Edit Skills, Options):** no localizadas [N]. Los índices de página `#ACA` del menú principal tampoco.
- **Ya tocado en src [C]:** el overlay F10 (`src\ingame\menu.cpp:14-40`, con un checkbox en línea 26 "Modo desarrollador") y `quick_settings` (`src\ingame\quick_settings.cpp:30`, helper `Toggle(label, cvar, help)`). No hay ningún hook del menú del guest.
- Fallos conocidos de menú (`docs\01_estructura\HISTORICO_RELEASES.md:3023-3078`): crash en Dragon Universe (EU) y en START, corregidos registrando funciones que el recompilador había plegado. Recordatorio: si se toca el menú, ojo a las funciones no registradas.

### 3.2 Propuesta (mínimo riesgo)

1. **Fase 1 (sin tocar el menú del guest):** añadir en el overlay F10 una casilla "Tag Team" con cvar `dbz3_tag_team`, siguiendo el patrón de `menu.cpp:26`. El hook de 1.5 solo actúa si la casilla está activa. Riesgo bajo. Es la única vía que se puede probar sin localizar el menú.
2. **Fase 2 (entrada real en el menú):** localizar el nodo del menú principal (los lectores de `0x8247A990` son el punto de partida) y el índice `#ACA` de la página. Riesgo alto (menú recompilado sin nombres). Igual que la fase 3 del estudio de supervivencia.

Opción descartada: añadir la entrada como subentrada de Duelo sin localizar antes el nodo. Requiere tocar `sub_821D7400` o `sub_820FE668` y no hay forma de probarlo sin el juego.

## 4. Pendiente para medir en juego (el usuario debe hacerlo)

1. Leer como float en memoria: C1 (`0x8201F60C`), f30 (`0x820009FC`) y C2 (`0x8201B3F0`). Probable: C1 = 0.0 [I].
2. Durante un KO, registrar HP (X+6080), X+8203, X+8221, flags y el índice que pasa `sub_82136328` (una sonda de solo lectura, sin tocar el juego).
3. Confirmar la hipótesis del payload de `sub_82100FA8` (máscara en r1+80, estado por ranura en r1+88).
4. Confirmar el EU `dbz3eu_sub_8211A6A8` antes de cualquier hook.
5. Identificar el disparador de Freeza forma 5 (vida baja u otra condición).

## Archivos tocados

Ninguno del proyecto. Solo este `.md` en el scratchpad.
