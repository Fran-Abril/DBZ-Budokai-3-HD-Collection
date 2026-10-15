# 10 · Arranque de combate nativo (el "conector" del juego)

Estado: estudio de código (2026-10-15, US). Todo sale de leer `generated/dbz3_recomp.*.cpp`; nada
medido en el juego todavía. Las direcciones son de la US; la EU no está localizada para las
funciones nuevas (los ganchos de abajo van solo en US: `if (dbz3::g_guest_eu) return orig`).
Cita = `archivo:línea` del `DEFINE_REX_FUNC` (o de la línea de la llamada).

## 1. Resumen en 6 líneas

1. Hay **una sola puerta** al combate: `sub_820FE668(sel, r4)` (`dbz3_recomp.5.cpp:6912`). Copia una
   **petición de combate de 384 B** (`sel`) a `mgr+1680` y prepara a los luchadores.
2. Los modos no rellenan `sel` a mano: rellenan la **estructura global G = 0x824BA110** (la del byte de
   pantalla) y `sub_820FFA80` (`dbz3_recomp.43.cpp:7016`) construye `sel` desde G según **G+1 = tipo de juego**.
3. Las pantallas son una **máquina de escenas**: `G+0` = escena actual, `0x8247A990` = escena pedida.
   Cada escena tiene 3 manejadores en `tarea+28` (espera-petición → espera-salida → confirmar).
4. El paso select → combate es: el Duelo pide la escena 15 (`sub_821D2778(15,0)`), su manejador
   `sub_821D62E8` llama a `sub_820FFA80` → `sub_820FE668` → combate.
5. La vuelta es `sub_820FEBA8` (fin de combate) → `sub_821D3628(resultado)`, que elige la escena
   siguiente **por G+1** (Duelo → 14, Práctica → 19, …; resultado 2 → menú principal 5).
6. Camino recomendado para un HUB: **escena 25 con la CFG 0x8245EFC0** (384 B rellenados por
   nosotros; el juego la pasa tal cual a `sub_820FE668`), escenas 5 → 25 → 5, y forzar la vuelta a 5
   en `sub_821D3628`. Detalle en §9. Alternativa por G (5 → 14 → 15): §6.

## 2. Estructuras

### 2.1 G = 0x824BA110 (EU 0x824BA100) — "estado de menús" (campos usados por el conector)

| Offset | Tipo | Significado | Quién (cita) |
|---|---|---|---|
| +0 | s8 | escena actual (5 menú, 14 Duelo, 15 combate de Duelo, 19 Práctica, 20/21 combate de Práctica, 9 combate de DU, 18 combate de Torneo, 25 combate de Dragon Arena) | escrito al confirmar escena, p. ej. `sub_821D5880` (`29.cpp:18951`), `sub_821D7990` (`35.cpp:18793`) |
| +1 | s8 | **tipo de juego**: 0 demo, 1 Dragon Universe, 2 Duelo, 3 Torneo, 4 Práctica, 5 Práctica con combate preparado, 6 Dragon Arena | `sub_821D7990` al entrar (14→2, 6→1, 16→3, 23→6); `sub_821D63F0` (20→4, 21→5) |
| +26,+27 | s8 | datos del Torneo (`sub_820F3CD8`) | `sub_820FE668` modo 2 |
| +28 | s8 | **quién juega**: 0 1P vs 2P, 1 1P vs COM, 2 COM vs 2P, 3 COM vs COM | leído en `sub_820FFA80` (bloque `0x820FFC4C`) |
| +29 | s8 | lado del COM en Torneo | `sub_820FFA80` (tipo 3) |
| +30 | s8 | **escenario** | → `sel+340` |
| +31,+32 | s8 | **ID de personaje** J1/J2 | `sub_8217ADE8` (ver `select_ext.cpp:524`) |
| +33,+34 | s8 | **traje** J1/J2 | → `sel+16+80k+18` |
| +37 | s8 | clase del último combate (9→0, 15→1, 18→2, 20/21→3, 25→6) | `sub_820FEBA8` |
| +38 | s8 | Práctica: qué lado es el muñeco / índice | `sub_820FFA80` tipos 4/5 |
| +116 | u8 | "volver al menú" (1 al empezar combate) | `sub_821D5DB8` |
| +117,+118 | s8 | salida forzada por error (escena destino en +118) | `sub_821D3888` |
| +0x2C08C (0x824E619C) | s16 | **tiempo** del combate (s; ≤0 = sin tiempo) | → `sel+348` |

Bloque de cápsulas de la selección (escrito por el menú Normal/Custom, **a confirmar con sonda**):

| Dirección | Tipo | Significado |
|---|---|---|
| 0x8245EF90 + k | s8 | nivel de vida J(k+1) (×1000 → `rec+36` y `rec+40`) |
| 0x8245EF92 + 16k | u16 | nº de cápsulas J(k+1) |
| 0x8245EF94 + 16k | 7 × u16 | cápsulas J(k+1) |
| 0x8245EFC0 | 384 B | `sel` propia de Dragon Arena (`sub_821D7400` la pasa tal cual a `sub_820FE668`) |
| 0x82439F90 | 384 B | `sel` propia de Dragon Universe (`sub_82156CE8`, +360 = tipo de combate DU) |
| 0x82330E50 + 384·i | 384 B | tabla de **combates preparados** (Práctica tipo 5; `memcpy` en `sub_820FFA80` `0x820FFCF8`) |

### 2.2 `sel` — petición de combate (384 B, copia en `mgr+1680`)

Construida en la pila por `sub_820FFA80` (sel = r1+80) y copiada por `sub_820FE668` (`memcpy(mgr+1680, sel, 384)`).

| Offset sel | = mgr+ | Tipo | Significado |
|---|---|---|---|
| +16 + 80k | 1696+80k | 80 B | **registro del jugador k** (k = 0..3) |
|  rec+0 | | f32 | (float de posición/escala; DU lo toca) |
|  rec+16 | | u16 | **ID de personaje** |
|  rec+18 | | u16 | **traje** |
|  rec+26 | | s16 | **mando**: 0 = mando 1, 1 = mando 2, -1 = **COM** |
|  rec+36, rec+40 | | s32 | vida (1000 × nivel) |
|  rec+52 | | u16 | nº de cápsulas |
|  rec+54 | | 7 × u16 | cápsulas |
| +336 | 2016 | s16 | nº de jugadores (0 → 2) → `mgr+35096` (`sub_820FC9B0`) |
| +338 | **2018** | u16 | **modo de combate**: 1 Duelo/Arena, 2 Torneo, 3 Práctica, 6 demo; 4/5 los usan DU/otros (sin confirmar) |
| +340 | 2020 | s16 | **escenario** → `sub_820F3C98` → `mgr+2022` |
| +342 | 2022 | s16 | (-1; luego escenario resuelto) |
| +344 | 2024 | u16 | → byte `mgr+35573` |
| +348 | 2028 | s32 | tiempo en s (×60 → ticks dentro de `sub_820FE668`) |
| +352,+356 | | s32 | -1 |

`sub_820FE668` (orden, `5.cpp:6912`): crea `mgr` si no existe (0x823CE1E0, `memset 35584`) → copia sel
→ guarda r4 en `mgr+35574` y la config de mandos (0x824E6189) → según `mgr+2018` calcula el escenario
→ tiempo ×60 → modo 5: `sub_820FE3D8` (limpia cápsulas) → `sub_820FC9B0(0)` (jugadores, estado
inicial `mgr+35128`) → sustituye IDs prohibidos (22-26, 31, 66/67/74/75, 80-104) por Goku → por
jugador `sub_821007E8(mgr+1696+80k)` → **`sub_821D27F8()`** (escena lista: `0x8247A998 = 1`).

Nuestros ganchos ya viven aquí: `roster_ext.cpp:1118` (`sub_820FE668`, llama a
`dbz3::tag::BeforeBattleSetup`) y `roster_ext.cpp:1139` (`sub_821007E8`).

## 3. La máquina de escenas

Variables (lis -32184 = 0x82480000):

| Dirección | Significado | API |
|---|---|---|
| 0x8247A988 | argumento de la escena pedida | `sub_821D2778(escena, arg)` (`16.cpp:18337`) |
| 0x8247A98C | escena en cola (si ya había una pedida) | idem |
| 0x8247A990 | **escena pedida** (-1 = ninguna) | idem; `sub_821D7D18(escena)` la fuerza con reinicio total |
| 0x8247A994 | **salida terminada** (la escena vieja acabó su fundido) | `sub_821D27A8()` (`5.cpp:18482`), `sub_821D27C8()` |
| 0x8247A998 | **lista para confirmar** | `sub_821D27F8()` (`14.cpp:19054`) |
| 0x8247A9A0 | puntero a la tarea de escenas (T); su manejador en **T+28** | — |

Cada manejador empieza con `sub_821D3888(4)` y `sub_821D7FE8()` (salidas por error/desconexión).

### 3.1 Grafo de llamadas: Menú principal → Duelo → combate → vuelta

```
Menú principal (G+0=5), T+28 = 0x821D7C30  [27.cpp:18818]
  fila elegida (sub_8218C5D8, fila 1) → sub_821D2778(14,0)
  0x821D7C30: pedida 14 → sub_821AD130 (crea el menú de Duelo), T+28 = 0x821D7AE8
  ... fundido del menú → sub_821D27F8 (lista)
  0x821D7990 [35.cpp:18793] confirma: G+0=14, G+1=2, T+28=0x821D62E8, sub_8218BA00 (destruye menú principal)

Duelo (G+0=14), T+28 = 0x821D62E8  [28.cpp:19028]
  rueda + cápsulas + escenario escriben G+28/+30/+31../+33.. y 0x8245EF90..
  el Duelo pide 15 → 0x821D62E8: T+28 = 0x821D5DB8; sub_820FFA80(0)
       sub_820FFA80 [43.cpp:7016]: G+1==2 → sel desde G, sel+338 = 1 → sub_820FE668(sel, 0) → lista
  0x821D5DB8 [37.cpp:18330]: espera "salida terminada"; G+116=1; sub_821006F8 (carga del combate);
       T+28 = 0x821D5880
  0x821D5880 [29.cpp:18951]: espera "lista": G+0=15, T+28=0x821D53D0, sub_821AD998 (destruye menú Duelo)

Combate (G+0=15), T+28 = 0x821D53D0  [43.cpp:18882]
  fin: sub_820FEBA8 [22.cpp:6760]: G+37 = clase; sub_820FE9B0; r3 = *0x823D6CE0 (resultado);
       sub_821D3628(r3) [14.cpp:19087] → pide la escena según G+1; sub_821D27A8 (salida terminada)
  0x821D53D0: pedida 14 → sub_821AD130 (recrea el Duelo); T+28 = 0x821D4D20
  0x821D4D20 [22.cpp:18774]: pedida 5 → sub_821D7D18(5) (reinicio limpio al menú principal);
       pedida 14 → sub_821D3C08
```

### 3.2 Enrutado de la vuelta `sub_821D3628(resultado)` por G+1

| G+1 | resultado 2 | resultado -1 | otro |
|---|---|---|---|
| 1 DU | 5 | 10 | `sub_82156BA8` + 8 (mapa) |
| 2 Duelo | 5 | 14 | 14 (arg 1) |
| 3 Torneo | 5 | 17 (arg -1) | 17 (arg = resultado) |
| 4 Práctica | 5 | 19 (arg 1) | 19 (arg 1) |
| 5 Práctica preparada | 5 | 19 (arg 3) | 19 (arg 2) |
| 6 Dragon Arena | 5 | 26 | 24 |

### 3.3 Otros caminos al conector (precedentes de "sin selección")

| Llamador de `sub_820FE668` | sel | Uso |
|---|---|---|
| `sub_820FFA80` (`43.cpp:7710`) | pila, desde G | Duelo, Torneo, Práctica, demo |
| `sub_82156CE8` (`38.cpp:11701`) | 0x82439F90 | **Dragon Universe**: sel ya hecha + `sub_821006F8` en la misma función |
| `sub_821D7400` (`22.cpp:18970`) | 0x8245EFC0 | **Dragon Arena** (escenas 23/25), r4 = 0 |

Y `sub_821D38E8` (`13.cpp:18688`) arma un **combate de demostración al azar** (G+1 = 0, IDs con
`sub_82084A78`, escenario en G+30) y llama a `sub_820FFA80(0)`: el propio juego entra al combate
sin pasar por la rueda.

Constructores por escena pedida: `sub_821D62E8` (14→15), `sub_821D6368` (16→18, `sub_820FFA80(3)`),
`sub_821D63F0` (19→20 con G+1=4; 19→21 con G+1=5 y combate preparado nº `*(s16*)0x8245F142`).

## 4. Qué datos hacen falta para un combate

Mínimo para `sub_820FFA80` con G+1 = 2 (Duelo):
- G+28 (quién juega), G+30 (escenario), G+31/+32 (IDs), G+33/+34 (trajes), tiempo en 0x824E619C;
- vida/cápsulas en 0x8245EF90 / 0x8245EF92+16k / 0x8245EF94+16k (si no, valen las del último Duelo).

Alternativa sin G: entregar nuestra propia `sel` de 384 B (como DU y Arena) y llamar a `sub_820FE668`
directamente, o cambiar `ctx.r3` en el gancho existente de `sub_820FE668` para que apunte a nuestra
copia. El ID inicial de Tag Team ya se pone en ese gancho (`BeforeBattleSetup`).

## 5. Opciones evaluadas

| Opción | Qué es | Veredicto |
|---|---|---|
| (a) Rueda nativa para un modo nuestro | entrar al Duelo (14) con G+1 = 2 y abrir solo la rueda | la rueda es una subtarea del menú de Duelo (`sub_821AD130` crea el menú; `sub_8217E7A8` destruye la rueda). Falta localizar la llamada que salta el menú de filas y abre la rueda: **siguiente estudio** |
| (b) Sin selección, directo al combate | hacer de Duelo con las llamadas del juego | **recomendada**: todas las piezas están localizadas (§6) |
| (b') Escribir T+28 = 0x821D62E8 desde el menú principal y pedir 15 | saltarse la escena 14 | posible, pero hay que destruir el menú principal (`sub_8218BA00`) y evitar `sub_821AD998` sobre un Duelo que no existe. Más frágil |
| (c) Vuelta a nuestro menú | gancho en `sub_821D3628` con resultado 2 → escena 5 | **recomendada**: el juego hace `sub_821D7D18(5)` (reinicio limpio) y el panel TAG TEAM vuelve a salir sobre el menú principal |

## 6. Plan de implementación (camino recomendado)

Archivo nuevo `src/ingame/native_start.cpp` (+ `.h`), US solo. Todo se ejecuta en el hilo del
juego, dentro de los manejadores de escena (mismo patrón que `campaigns.cpp:256-325`: guardar
`ctx`, poner registros, llamar `sub_X(ctx, base)`, restaurar).

### Paso 1 · Petición
```cpp
struct NativeBattle {            // lo que pide TAG TEAM / Survival / Torneo propio
  int8_t vs = 1;                 // G+28: 0 1Pv2P, 1 1PvCOM, 2 COMv2P, 3 COMvCOM
  int8_t stage = 0;              // G+30
  int8_t id[2], suit[2];         // G+31.., G+33..
  int16_t time = -1;             // 0x824E619C
  uint16_t caps[2][8];           // n + 7 (0x8245EF92+16k)
  bool back_to_panel = true;
};
std::atomic<int> g_phase{0};     // 0 nada, 1 ir a 14, 2 pedir 15, 3 en combate
NativeBattle g_req;
void Request(const NativeBattle& r) { g_req = r; g_phase = 1; }   // desde el panel (tras el fundido negro)
```

### Paso 2 · Menú principal → Duelo (escena 14) con la API del juego
```cpp
// Manejador de espera del menú principal: corre cada tick mientras G+0 == 5.
REX_HOOK_RAW(sub_821D7C30) {
  if (!dbz3::g_guest_eu && g_phase == 1 && REX_LOAD_U32(0x8247A990) == 0xFFFFFFFF) {
    WriteG(base, g_req);                         // §4: G+1 lo pone el juego (14 → 2)
    const PPCContext saved = ctx;
    ctx.r3.u64 = 14; ctx.r4.u64 = 0; sub_821D2778(ctx, base);   // = fila "Duelo" del menú
    ctx = saved;
    g_phase = 2;
  }
  __imp__sub_821D7C30(ctx, base);                // crea el Duelo y pasa a 0x821D7AE8
}
// El menú principal no está haciendo su fundido de salida: damos nosotros "salida terminada" y
// "lista" (lo que harían su animación y sub_821D27F8). A CONFIRMAR con la sonda del paso 6.
REX_HOOK_RAW(sub_821D7AE8) {
  if (g_phase == 2) { REX_STORE_U32(0x8247A994, 1); REX_STORE_U32(0x8247A998, 1); }
  __imp__sub_821D7AE8(ctx, base);
}
```

### Paso 3 · Duelo → combate (escena 15) sin rueda
```cpp
REX_HOOK_RAW(sub_821D62E8) {                     // espera del Duelo (G+0 == 14)
  if (g_phase == 2 && REX_LOAD_U8(0x824BA110) == 14 && REX_LOAD_U32(0x8247A990) == 0xFFFFFFFF) {
    WriteG(base, g_req);                         // otra vez: el Duelo pudo pisar G+28
    const PPCContext saved = ctx;
    ctx.r3.u64 = 15; ctx.r4.u64 = 0; sub_821D2778(ctx, base);
    ctx = saved;
    g_phase = 3;
  }
  __imp__sub_821D62E8(ctx, base);                // → sub_820FFA80 → sub_820FE668 (lista) → T+28 = 0x821D5DB8
}
REX_HOOK_RAW(sub_821D5DB8) {                     // espera "salida terminada" del menú de Duelo
  if (g_phase == 3) REX_STORE_U32(0x8247A994, 1);   // el menú de Duelo no hace su fundido
  __imp__sub_821D5DB8(ctx, base);                // G+116, sub_821006F8 (carga), → 0x821D5880 → G+0 = 15
}
```
`sub_821D5880` destruye el menú de Duelo con `sub_821AD998`: como el menú se creó de verdad en el
paso 2, no hay nada que esquivar.

### Paso 4 · Datos del combate
Nada nuevo: `sub_820FFA80` lee G; el gancho existente de `sub_820FE668` (`roster_ext.cpp:1118`)
sigue llamando a `BeforeBattleSetup` (Tag Team pone el primer miembro) y la sustitución de IDs.
Si hace falta algo que G no lleva (4 jugadores, vida por lado), se edita `sel` (r3) en ese mismo
gancho antes de `orig`.

### Paso 5 · Vuelta a nuestro menú
```cpp
REX_HOOK_RAW(sub_821D3628) {                     // enrutado tras el combate
  if (g_phase == 3 && g_req.back_to_panel) {
    ctx.r3.s64 = 2;                              // "salir": escena 5 para cualquier G+1
    g_phase = 0;
    dbz3::ingame::ReopenTagTeamPanel();          // el panel se abre al volver G+0 a 5
  }
  __imp__sub_821D3628(ctx, base);
}
```
El juego va por `0x821D53D0` → `0x821D4D20` → `sub_821D7D18(5)` (el mismo reinicio limpio que usa
al salir de un combate de Duelo al menú principal). Para "otra vez" (revancha/siguiente rival de
Survival), en lugar de r3 = 2 se puede dejar que vuelva a 14 y repetir el paso 3.

### Paso 6 · Prueba mínima en el juego
1. `DBZ3_NATIVE_START=1`: un `REXLOG_INFO` por manejador (`sub_821D7C30/7AE8/62E8/5DB8/5880/53D0`,
   `sub_820FE668`, `sub_820FEBA8`, `sub_821D3628`) con G+0, G+1, 0x8247A990/94/98.
2. Partida con copia de seguridad de la partida guardada (`long_run`), ventana visible.
3. Menú principal → TAG TEAM → 1P VS COM 1 VS 1 con IDs fijos (Goku 0 vs Vegeta) → esperar en el log:
   `5 → 14 → 15`, `mgr+2018 = 1`, IDs correctos en `mgr+1696+16` y `+1776+16`.
4. Ganar o perder → log `sub_821D3628 r3=2` → G+0 = 5 y el panel abierto. Repetir 3 veces seguidas
   (fugas de memoria del menú de Duelo creado y destruido).
5. Captura antes/después; nada de pulsaciones inyectadas en todo el camino.

## 7. Riesgos

- **Pasos 2 y 3 dan "salida terminada"/"lista" a mano.** En el juego los ponen las animaciones de
  salida de cada menú (llamadores de `sub_821D27A8`: `sub_82178F78`, `sub_82178C10`, `sub_82178F10`,
  `sub_82175D60`…). Si una tarea del menú sigue viva y escribe después, puede volver a pedir escena.
  Mitigación: en vez de escribir las banderas, buscar la función "cerrar menú" del Duelo (candidatas:
  las de arriba) y llamarla; o mantener nuestro fundido negro hasta G+0 = 15.
- `sub_821D7C30` también llama a `sub_821D9860` cada tick: no tocar su orden.
- `sub_821D3888(4)`/`sub_821D7FE8` cortan con `sub_821D7D18` si hay error (mando desconectado,
  0x824BA110+117): el flujo se reinicia al menú principal; `g_phase` debe volver a 0 si G+0 = 5 sin
  combate.
- El Duelo puede reescribir G+28..+34 al crearse (`sub_821AD130`): por eso `WriteG` se repite en el
  paso 3.
- Online: este camino es local. El online (netplay) debe seguir con su propio arranque hasta medir
  que los dos PCs llegan al mismo tick de `sub_820FE668`.
- EU: ninguna de las funciones nuevas tiene gemela localizada; sin EU, el panel sigue con el piloto.
- Semántica sin confirmar: vida (0x8245EF90+k ×1000), `rec+0` float, modos 4/5 de `mgr+2018`.

## 8. Siguientes estudios

1. (a) Rueda nativa sin menú de filas: dentro de `sub_821AD130` / su tarea, localizar el estado que
   abre la rueda tras elegir "1P VS COM" (la fila escribe G+28) y saltar a él directamente.
2. Medir quién pone 0x8247A994/98 al salir del Duelo (sonda de escritura) para sustituir el paso
   manual por la llamada del juego.
3. Survival/Torneo propios: usar la tabla de combates preparados (0x82330E50, G+1 = 5) como
   plantilla: es justo "lista de combates con datos fijos" que ya sabe arrancar la Práctica.

## 9. HUB propio: arrancar con la CFG 0x8245EFC0 (escena 25) — camino preferente

Añadido tras el aviso del coordinador (cola de pantallas en `01_pantallas.md`, lista del Duelo
`sub_82177A88 → sub_82177918 → sub_82177300` que pide la escena 25). Verificado en el código:
la escena 25 **no usa G**: `sub_821D7400` pasa la CFG **0x8245EFC0 entera** a `sub_820FE668`. Es el
conector más limpio para un HUB: rellenar 384 B y pedir 25.

### 9.1 Cadena del juego (24 → 25 → vuelta)

```
sub_82173B98 → sub_82179408()            [9.cpp:13337]  CFG por defecto en 0x8245EFC0 (modo 5)
sub_82177300 [7.cpp:13465]: sub_821D2778(25,0); sub_821D27C8() (sub_821652D0(10) + "salida terminada")
0x821D7400 [22.cpp:18970] (espera de la escena 24): pedida 25 → T+28=0x821D7280;
     sub_820FE668(0x8245EFC0, 0) → "lista"; sub_821D9838(1) (sonido 43)
0x821D7280 [33.cpp:18994]: con "salida terminada": G+116=1, sub_821006F8 (carga); T+28=0x821D70F8
0x821D70F8 [37.cpp:18427]: con "lista": G+0=25, T+28=0x821D6D60, sub_8216BF78 (destruye la UI de 24)
combate → sub_820FEBA8 → sub_821D3628 (G+1=6: 2→5, -1→26, otro→24)
0x821D6D60 [31.cpp:18834] → T+28=0x821D6A08 [14.cpp:19408]: pedida 5 → sub_821D7D18(5) (menú principal limpio)
```

### 9.2 Qué escribir (CFG S = 0x8245EFC0, 384 B; llamar antes a `sub_82179408()` para los valores por defecto)

| Dirección | Campo | Valor |
|---|---|---|
| 0x8245EFE0 / 0x8245F030 | ID J1 / J2 (u16, rec+16) | personaje |
| 0x8245EFE2 / 0x8245F032 | traje (u16, rec+18) | 0.. |
| 0x8245EFEA / 0x8245F03A | mando (s16, rec+26) | 0 mando 1, 1 mando 2, **-1 COM** (por defecto 0 y 1) |
| 0x8245EFF4,+F8 / 0x8245F044,+48 | vida (s32 rec+36/+40) | por defecto 3000 |
| 0x8245F004 / 0x8245F054 | nº de cápsulas (u16 rec+52) | 0..7 |
| 0x8245F006 / 0x8245F056 | cápsulas (7 × u16 rec+54) | IDs (≥595 ya lo resuelve `roster_ext.cpp:1554`) |
| 0x8245F110 | nº de jugadores (s16 +336) | 0 (= 2) |
| 0x8245F112 | **modo** (u16 +338 → mgr+2018) | 5 (lo pone `sub_82179408`; el modo 5 pasa por `sub_820FE3D8`) |
| 0x8245F114 | **escenario** (s16 +340) | ID de escenario |
| 0x8245F11C | tiempo (s32 +348) | lo pone `sub_82179408` desde G+0x2C08C (×60; `sub_820FE668` vuelve a ×60: unidades a medir) |
| 0x824BA111 | G+1 tipo de juego | 6 (enrutado de la vuelta de la escena 25) |

### 9.3 Arranque desde el HUB (menú principal, G+0 = 5)

```cpp
// US. En la espera del menú principal (corre cada tick en el hilo del juego).
REX_HOOK_RAW(sub_821D7C30) {
  if (!dbz3::g_guest_eu && g_phase == 1 && REX_LOAD_U32(0x8247A990) == 0xFFFFFFFF) {
    const PPCContext saved = ctx;
    sub_82179408(ctx, base);                       // CFG por defecto (modo 5, tiempo)
    constexpr uint32_t S = 0x8245EFC0;
    for (uint32_t k = 0; k < 2; ++k) {
      const uint32_t rec = S + 16 + 80 * k;
      REX_STORE_U16(rec + 16, g_req.id[k]);
      REX_STORE_U16(rec + 18, g_req.suit[k]);
      REX_STORE_U16(rec + 26, uint16_t(g_req.pad[k]));   // -1 = COM
      REX_STORE_U16(rec + 52, g_req.caps[k][0]);
      for (uint32_t i = 0; i < 7; ++i) REX_STORE_U16(rec + 54 + 2 * i, g_req.caps[k][1 + i]);
    }
    REX_STORE_U16(S + 340, uint16_t(g_req.stage));
    base[0x824BA111] = 6;                          // G+1: la vuelta de la escena 25
    sub_8218BA00(ctx, base);                       // destruye el menú principal (lo que hace su confirmación)
    const uint32_t T = REX_LOAD_U32(0x8247A9A0);
    REX_STORE_U32(T + 28, 0x821D7400);             // a partir de aquí somos "la escena 24"
    ctx.r3.u64 = 25; ctx.r4.u64 = 0; sub_821D2778(ctx, base);
    ctx.r3.u64 = 10; sub_821D27C8(ctx, base);      // "salida terminada", como sub_82177300
    ctx = saved;
    g_phase = 3;
    return;                                        // este tick ya no es el menú principal
  }
  __imp__sub_821D7C30(ctx, base);
}
// sub_8216BF78 destruye la UI de la escena 24 según el byte 0x82456430 (0/1/2); nunca se creó.
REX_HOOK_RAW(sub_8216BF78) {
  if (g_phase == 3 && REX_LOAD_U8(0x824BA110) == 25) return;
  __imp__sub_8216BF78(ctx, base);
}
```
La vuelta es el mismo gancho de `sub_821D3628` del §6 paso 5 (r3 = 2 → escena 5 → `sub_821D7D18(5)`),
y el HUB se reabre al ver G+0 = 5 con `g_phase` de vuelta a 0. Para Survival/Torre: en vez de r3 = 2,
reescribir la CFG y dejar que vuelva a 24 sería otra UI del juego; mejor volver a 5 y relanzar desde el HUB
(o, sin pasar por el menú, poner T+28 = 0x821D7400 y pedir 25 otra vez en la espera 0x821D6D60).

Riesgos propios de este camino: `sub_8218BA00` fuera de su sitio (comprobar en la sonda que la tarea
del menú `sub_8218C5D8` deja de correr); `sub_821D27C8` llama a `sub_821652D0(10)` (¿sonido/fundido?);
modo 5 frente a 1 (Duelo) en reglas del combate: comparar en la prueba. Prueba mínima = §6 paso 6 con
la secuencia de log `5 → 25 → 5`.
