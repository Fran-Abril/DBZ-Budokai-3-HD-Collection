# 12 · El Torneo Mundial como motor de modos reto

Fecha: 2026-10-15. Solo US. Las direcciones EU **no están calculadas**.
Fuente: lectura del código generado (`generated\dbz3_recomp.*.cpp`). No se ha probado nada en el juego.

Marcas de confianza:
- **[C]**: leído en el código.
- **[I]**: deducido del código, sin medir.
- **[N]**: pendiente de medir en el juego.

Relación con otros documentos:
- `01_pantallas.md` explica la cola de pantallas (`sub_821D2778`).
- `03_menus_y_tareas.md` explica las filas del menú. Su fila 2, la que pide la pantalla 16, es el Torneo.

---

## 0. Veredicto corto

El Torneo Mundial (TM) **sí sirve como anfitrión**. Ya trae:
- el bucle de pantallas 17 (cuadro) ⇄ 18 (combate);
- el paso del resultado de cada combate;
- la construcción del combate a partir de bytes de la partida (personaje, traje, cápsulas, vida y escenario);
- la salida al menú principal.

Pero su **cuadro eliminatorio no vale tal cual** para torres, supervivencia o kumite:
- El cuadro tiene 2^n plazas, con n de 3 a 5, es decir de 8 a 32 luchadores. Un humano solo pelea **n combates** (3 a 5).
- El orden de los luchadores es aleatorio.
- Los combates entre dos COM se resuelven sin pelearse.

La vía recomendada:
1. Usar el TM como **carcasa**: menú, pantalla 17 como pausa entre combates y pantalla 18 como combate.
2. Llevar nosotros el estado del reto: siguiente rival, vida, tienda y reglas.
3. Cambiar la configuración de cada combate en `sub_820FE668` y el resultado en `sub_821D3628` / `sub_821A0A08`.

Para la ficha 03 (torneo local con bots), el TM sirve **casi tal cual**, cambiando la lista de participantes.

---

## 1. Entrada desde el menú principal y pantallas

**Byte de pantalla**: `0x824BA110` (byte +0 de la partida en memoria). **Modo de juego**: `0x824BA111` (partida +1). [C]

### 1.1 Valores del modo de juego (partida +1)

| Valor | Modo | Dónde |
|---|---|---|
| 1 | Dragon Universe | `sub_821D7990` rama 6 (`dbz3_recomp.35.cpp:18793`) |
| 2 | Duelo | rama 14 |
| **3** | **Torneo Mundial** | rama 16 |
| 4 | Práctica | `sub_821D63F0` pantalla 20 (`15.cpp:18750`) |
| 5 | Tutorial | pantalla 21, tabla `0x82330E50` de 384 B por entrada |
| 6 | Pantalla 23 | sin identificar |

### 1.2 Grafo de pantallas del TM [C]

Los cambios de pantalla se piden con `sub_821D2778(pantalla, arg)` (`16.cpp:18337`):
- la petición se guarda en `0x8247A990`;
- el argumento, en `0x8247A988`.

Cada tarea de pantalla está en `tarea+28` y reacciona a la petición.

```
5  menú principal  (tarea sub_821D7C30, 27.cpp:18818)
   └─ pide 16 → sub_821A80D8 (39.cpp:16147): crea el menú del TM (216 B, puntero en 0x824681D8)
16 menú del TM: nivel y participantes (tarea sub_821D6CD8, 38.cpp:18732)
   └─ al aceptar, sub_821AA510 (5.cpp:16313):
        0x82456480 = nº de humanos (obj+176)
        sub_821A0110(0x82456480, nivel = obj+175)  → prepara el trabajo del TM
        sub_821D2778(17, 255)                      → 255 = "torneo nuevo"
17 cuadro (sub_821A3260(arg), 42.cpp:16402)
   ├─ arg 255 → sub_821A28A8 (5.cpp:15985): sorteo de participantes
   ├─ arg ≠ 255 → copia el cuadro guardado + sub_821A0A08(arg) (27.cpp:15847): aplica el resultado
   ├─ cada tick → sub_821A5A30 (28.cpp:16342): prepara el combate y pide la pantalla 18 (arg 0)
   └─ al pedir 18: sub_821D5950 (22.cpp:18855) → sub_821A13C8 libera el cuadro
18 combate: sub_821D6368 (32.cpp:18655) → sub_820FFA80(3) (43.cpp:7016) → sub_820FE668
   └─ fin: sub_820FEBA8 (22.cpp:6760) → sub_821D3628(R = *0x823D6CE0) (14.cpp:19087)
        modo 3: R == 2 → pide 5 (abandonar); si no → 0x8247A988 = R y pide 17
```

En el código que lee el resultado, `R` toma los valores 0, 1 y −1:
- 0 o 1: gana el lado 1 o el lado 2 [I];
- −1: empate o doble KO [I];
- 2: salir desde la pausa [I].

---

## 2. Estado del torneo

### 2.1 Bloque de trabajo `G = 0x8245F270` (36704 B) [C]

`sub_821A0110` (`28.cpp:16094`) hace lo siguiente:
1. `memset(G, 0, 36704)`.
2. `memcpy(G, 0x82456480, 36048)`: copia la sesión, con los perfiles de jugador de 4448 B cada uno.
3. `partida+26 = nivel` y `partida+27 = 0`.
4. `G+36690 = nivel < 3 ? nivel + 3 : 5`.

| Dirección | Tipo | Significado |
|---|---|---|
| `G+0` (`0x8245F270`) | s8 | Número de humanos inscritos [I]. `sub_821A0A08` y `sub_821A5A30` lo comparan con 1 y 2. |
| `G+4432+4448·h` | s8 | Casilla de selección elegida por el humano h (la lee el sorteo). |
| `G+36048` (`0x82467F40`) | 32 × 20 B | **Cuadro guardado** (640 B). La pantalla 17 lo copia a `obj+748` al entrar. |
| `G+36688` (`0x824681C0`) | s8 | Índice del participante A del combate actual. |
| `G+36689` | s8 | Índice del participante B. |
| `G+36690` | s8 | log2 de los participantes: 3 = 8, 4 = 16, 5 = 32. |
| `G+36691` | s8 | Mando del humano principal [I]. Copia de `partida+38`; `sub_821A3260` lo devuelve a `partida+38`. |

### 2.2 Objeto de la pantalla 17 [C]

Ocupa 1388 B y su puntero está en `*0x824681D0`. Campos útiles:

| Campo | Significado |
|---|---|
| `+72` | Fase: 0 = nuevo, 3 = viene de un combate. |
| `+96` | Máscara de lados humanos. |
| `+100` / `+102` | Índice del ganador y del perdedor del último combate. |
| `+104` | 1 = torneo terminado. |
| `+112` | Puntero a la siguiente pareja. |
| `+748` | 32 entradas de 20 B (copia de trabajo de `G+36048`). |

### 2.3 Entrada de participante (20 B) [C, salvo lo marcado]

| Offset | Tipo | Significado |
|---|---|---|
| +0 | u32 | Se pone a 0 al ser eliminado [I]. |
| +4 | u8 | Estado: 1 o 2 = sigue vivo; 3 = eliminado. |
| +7 | u8 | Rondas ganadas (se suma 1 en cada victoria). |
| +8 | s16 | **Casilla de la rueda** (0-37). El ID sale de la tabla s16 `0x82020618[casilla]`. −1 = libre. |
| +10 | s16 | **Mando** del humano (≥ 0) o −1 si es COM. |

### 2.4 Partida (`0x824BA110`)

| Offset | Significado |
|---|---|
| +26 | Nivel del TM: 0 Novato, 1 Experto, 2 Avanzado; 3 = Juegos de Cell [I]. |
| +27 | Contador de torneos ganados: `sub_821A3260` le suma 1 al acabar [I]. |
| +30 | Escenario elegido. |
| +31 / +32 | ID del lado 1 / 2. |
| +33 / +34 | Traje del lado 1 / 2. |
| +38 | Mando del jugador 1 [I]. `sub_820FFA80` lo pone en el bloque +10 del lado `partida+29`. |

---

## 3. Sorteo de participantes: `sub_821A28A8` (5.cpp:15985) [C]

1. Pone `+8 = −1` en las N = 2^`G+36690` entradas.
2. Cada humano h (`G+0` humanos) va a una plaza libre al azar (`rand` = `sub_82084A78`):
   - `+8` = su casilla (`G+4432+4448·h`);
   - `+10` = h.
3. Las plazas que quedan son COM (`+10 = −1`):
   - la casilla sale **al azar**, sin repetir, de una máscara de personajes disponibles;
   - la lista de candidatos es la tabla s16 `0x82020668[k]`;
   - las casillas de los humanos se quitan de la máscara.
4. `sub_821A0460(rondas, plaza, 0, &e+4, &e+8)` rellena el estado y el nivel de cada plaza.

**No hay tablas fijas de participantes por dificultad.** El nivel solo cambia N, y en el nivel 3 también el tipo de COM (ver §4).

---

## 4. Cómo se prepara y se lanza cada combate

### 4.1 Preparación en `sub_821A5A30` [C]

Para cada lado p (0/1) toma la entrada `G+36688+p`.

**Si es humano** (`+10 ≥ 0`):
- `partida+31+p` = `0x82020618[+8]` (ID);
- `partida+33+p` = traje del perfil (`0x82456480 + 4433 + 4448·mando`);
- `0x8245EF90+p` = **3** (barras de vida);
- copia 16 B del perfil a `0x8245EF92+16p` (u16 + 7 cápsulas s16).

**Si es COM** (`loc_821A5F78`):
- `partida+31+p` = ID;
- traje 0;
- barras = 3;
- cápsulas de la tabla por ID `0x8234ABB8 + 96·ID` (+80 u16, +82 = 7 × s16).
- Con nivel 3 pone `0x8245EF92+16p = 1` y `+2 = casilla+207` (variante especial [I]).

Si los dos lados tienen el mismo ID y el mismo traje, el lado 2 pasa al traje 1.

Después:
- `G+36691 = partida+38` (mando del humano [I]);
- `obj+96` |= lados humanos;
- `sub_821D2778(18, 0)`.

**Combate COM contra COM.** La rama `+48 == 1` llama a `sub_821A1A30` y luego a `sub_821A5408`: se anima el resultado en el cuadro y nadie pelea [I].

### 4.2 Construcción de la configuración: `sub_820FFA80` (43.cpp:7016), rama del modo 3 [C]

La configuración del combate ocupa **384 B**. Aquí se crea en la pila. La global equivalente está en `0x8245EFC0`. El jugador p tiene un bloque en `cfg + 32 + 80·p`:

| Offset del bloque | Significado | Origen |
|---|---|---|
| +0 | ID | `partida+31+p` |
| +2 | traje | `partida+33+p` |
| +10 | mando / COM | |
| **+20** | **vida máxima** | barras × 1000 |
| **+24** | **vida inicial** | barras × 1000 = 3000. **Aquí se reinicia la vida en cada combate** |
| +36 | u16 | `0x8245EF92+16p` |
| +38 | 7 × s16 cápsulas | |

Campos generales:
- `cfg+338` = **modo de combate = 2 (TM)**. Los otros valores: Duelo = 1; el Dragon Universe pasa por `sub_82156CE8`; demo = 6.
- `cfg+340` = escenario (`partida+30`).
- `cfg+348` = tiempo (s16 en `partida+0x2C08C`).

Aún sin medir [N]: cuál de +20/+24 es la vida máxima y cuál la actual.

### 4.3 Arranque: `sub_820FE668(cfg, r4)` (5.cpp:6912) [C]

1. Hace `memset` del gestor (`mgr = *0x823D6CF4`, 35584 B).
2. `memcpy(mgr+1680, cfg, 384)`, así que `mgr+2018` es el modo.
3. Convierte el tiempo: `mgr+2028 *= 60`.
4. Llama a `sub_820FC9B0` (9.cpp:6653).

En el **modo 2**, `sub_820FC9B0` además:
- pone el estado inicial `mgr+35128 = 2`;
- `mgr+35573 = min(partida+26, 2)`: reglas por nivel [I];
- `mgr+35574 = partida+27`.

**Este es el punto único donde se pueden cambiar las reglas de cada combate.**

---

## 5. Ganador, siguiente combate y final [C]

### 5.1 Resultado: `sub_821A0A08(R)`

1. Lee el mando de A y de B (`+10`).
2. Según R y `G+0`, pone el **perdedor** en estado 3 con `+0 = 0`.
3. Al **ganador** le suma 1 en `+7`.
4. Guarda `obj+100` (ganador) y `obj+102` (perdedor).
5. Con R = −1, los dos quedan eliminados.

### 5.2 Siguiente pareja: `sub_821A05E0(reset)` (43.cpp:16040)

Recorre las 2^n entradas. Devuelve:

| Valor | Significado |
|---|---|
| **5** | Torneo terminado. Un humano vivo llegó a `+7 == G+36690`, o no quedan humanos [I]. |
| **4** | Hay pareja nueva: guarda `G+36688/36689` y `obj+112`. |
| **3** | Seguir mostrando el cuadro. |

### 5.3 Final

Cuando `sub_821A05E0` devuelve 5, `sub_821A3260` hace lo siguiente:
1. `sub_821A0820` (32.cpp:15749): resultado o premio [I].
2. `partida+27++`.
3. `sub_821A0E80` (3.cpp:16102).
4. `obj+104 = 1`.

`sub_821A7398` usa las mismas tres llamadas.

### 5.4 Fuera del ring y tiempo

Son comunes a todos los modos y ya están medidos en `src\tagteam.cpp:865-962`:
- `mgr+35569` = aviso pendiente de fuera del ring (lado ganador + 1);
- `mgr+35132` = motivo del final (4 fuera del ring, 3 tiempo);
- `mgr+35136` = lado ganador;
- estado `mgr+35128`: 4 = pelea, 5 = acabado.

Según el código, el TM **no tiene un interruptor propio** de fuera del ring. Depende del escenario, y `mgr+35573/35574` pueden activar reglas por nivel [N].

---

## 6. Ganchos propuestos

Estilo de `src\tagteam.cpp` y `src\campaigns.cpp`. Solo US: si `g_guest_eu`, se llama al original y se sale. Los equivalentes EU (`dbz3eu_sub_*`) están pendientes, así que aquí se usa `REX_HOOK_RAW` directamente.

```cpp
// src/challenge.cpp (nuevo) — esbozo
namespace {
constexpr uint32_t kSave = 0x824BA110, kG = 0x8245F270, kBracketScreen = 0x824681D0;
constexpr uint32_t kRounds = kG + 36690, kPairA = kG + 36688, kPairB = kG + 36689;
constexpr uint32_t kCellToId = 0x82020618;
struct Challenge { bool on = false; std::vector<int16_t> cells; int next = 0; float hp = -1; } g_ch;
bool WtActive(uint8_t* base) { return !dbz3::g_guest_eu && base[kSave + 1] == 3 && g_ch.on; }
}

// (1) Lista propia: tras el sorteo nativo, cambiar las casillas de los COM.
REX_HOOK_RAW(sub_821A28A8) {
  __imp__sub_821A28A8(ctx, base);
  if (!WtActive(base)) return;
  const uint32_t obj = REX_LOAD_U32(kBracketScreen);
  const int n = 1 << int8_t(base[kRounds]);
  for (int i = 0, k = 0; i < n && k < int(g_ch.cells.size()); ++i) {
    const uint32_t e = obj + 748 + 20 * i;
    if (int16_t(REX_LOAD_U16(e + 10)) < 0) REX_STORE_U16(e + 8, uint16_t(g_ch.cells[k++]));   // COM
  }
}
// Tamaño del cuadro: tras sub_821A0110 (nivel → 8/16/32), poner base[kRounds] = 3..5.

// (2) Reglas por combate y vida que se arrastra: el único punto por donde pasa cada combate.
REX_HOOK_RAW(sub_820FE668) {
  const uint32_t cfg = ctx.r3.u32;
  if (WtActive(base) && REX_LOAD_U16(cfg + 338) == 2) {
    for (int p = 0; p < 2; ++p) {
      const uint32_t b = cfg + 32 + 80 * p;
      if (int16_t(REX_LOAD_U16(b + 10)) >= 0) {             // humano: vida guardada
        if (g_ch.hp > 0) REX_STORE_U32(b + 24, uint32_t(g_ch.hp));   // [N] confirmar +24 = actual
      } else if (g_ch.next < int(g_ch.cells.size())) {      // COM: rival de la torre
        REX_STORE_U16(b + 0, uint16_t(REX_LOAD_U16(kCellToId + 2 * g_ch.cells[g_ch.next])));
      }
    }
    // tiempo (s): REX_STORE_U32(cfg + 348, segundos); escenario: REX_STORE_U16(cfg + 340, id)
  }
  __imp__sub_820FE668(ctx, base);
}
// Leer la vida al final: en el paso del estado 4 al 5 (como LabTick, tagteam.cpp:875-893)
// g_ch.hp = LoadF(X + kHp) del lado humano.

// (3) Final propio: no pasar por el cuadro, sino seguir la secuencia.
REX_HOOK_RAW(sub_821D3628) {
  if (WtActive(base) && int32_t(ctx.r3.u32) != 2) {
    const int32_t r = int32_t(ctx.r3.u32);
    const bool won = /* lado humano */ r == 0;
    if (won && ++g_ch.next < int(g_ch.cells.size())) {
      __imp__sub_821D3628(ctx, base);                       // pide 17 → cuadro → 18 con el siguiente rival
      return;
    }
    g_ch.on = false;                                         // fin: resultado propio
    ctx.r3.u32 = 2;                                          // el juego vuelve al menú principal (5)
    // ShowChallengeResult(won, g_ch.next);                  // ventana nativa o ImGui sobre la 5
  }
  __imp__sub_821D3628(ctx, base);
}
```

### 6.1 Torre con más combates que rondas del cuadro

`sub_821A05E0` acaba el torneo cuando el humano llega a `+7 == G+36690`. Hay dos salidas:
- **a)** Con `G+36690 = 5`, un solo humano llega a 5 combates. Para más, poner a 0 su `+7` y devolver al COM rival al estado 1 tras cada victoria. Así el cuadro nunca da al humano como ganador final.
- **b)** Más sencilla: no usar el cuadro como fuente de verdad. El gancho de `sub_821D3628` decide el siguiente combate y el de `sub_820FE668` pone el rival. Así el cuadro solo sirve de pausa visual.

**Recomendada: b.**

### 6.2 Entrar desde el menú principal

Sin pantallas nuevas:
1. Fila propia, como la de TAG TEAM (`src\ingame\online_panel.cpp`).
2. Hacer lo mismo que `sub_821AA510`:
   - `partida+1 = 3`;
   - `byte 0x82456480 = 1`;
   - `sub_821A0110(0x82456480, nivel)`;
   - `sub_821D2778(17, 255)`.
3. Antes hay que elegir personaje. El menú del TM (16) lo hace en su propia rueda; la ruta más segura es entrar por la fila nativa del TM con `g_ch.on` activado desde nuestro submenú.

---

## 7. Encaje con cada ficha

| Ficha | ¿TM como motor? | Qué falta |
|---|---|---|
| 03 Torneo local con bots | **Sí, casi nativo.** Cuadro de 8/16/32, varios humanos, el COM contra COM ya se resuelve | Lista propia (gancho 1), nombre o premio propio |
| 09 Supervivencia | **Sí, como carcasa** (vía b) | Vida que se arrastra (gancho 2: leer en 4→5 y escribir `cfg` +24), contador de oleadas en la pantalla 17 |
| 24 Supervivencia + tienda | Sí, como carcasa | Tienda entre combates: la pantalla 17 es la parada natural (superponer UI); economía propia |
| 26 Torres | Sí, como carcasa | Secuencia y modificadores por piso: tiempo, escenario y vida por `cfg`. El drenaje de ki o los meteoritos necesitan ganchos de combate aparte |
| 28 Kumite de jefes | Sí, como carcasa | Vida o tiempo del jefe por `cfg`. La super armadura y las fases **no** están en la configuración (necesitan un gancho en el luchador, como en tagteam) |

Ventaja frente a la base de campañas (`BASE_MODOS_RETO.md`):
- sin límite de 10 #BPC;
- sin ocupar a Freezer en la rueda;
- cualquier ID como rival, porque `cfg` +0 acepta cualquier ID.

---

## 8. Riesgos

1. **La partida se modifica.** El TM suma `partida+27` y da zeni o premio (`sub_821A0820`/`sub_821A0E80`). Con el gancho 3 (salida con R=2) no se llega a ese final. Aun así, **hay que hacer copia de la partida antes de probar** (`long_run -Action Stop` restaura).
2. **Retrato y nombre del cuadro.** El cuadro dibuja la casilla `+8`. Si en `cfg` se pone un ID que no tiene casilla, el cuadro enseña otro personaje: es solo cosmético.
3. **IDs fuera de la rueda.** Con `cfg` +0 = ID extra (44-63), la carga pasa por roster_ext. Las cápsulas de la tabla `0x8234ABB8` solo existen para los IDs nativos: hay que rellenar +38 a mano.
4. **Campos sin medir:** +20/+24 de vida (cuál es la actual), `mgr+35573/35574` (reglas por nivel) y el significado exacto de `G+0`.
5. **Nivel 3.** Usa la variante `casilla+207`. No conviene usarlo como base.
6. **EU.** Ninguna dirección traducida. Los ganchos tienen que quedar desactivados con `g_guest_eu`.
7. **Varios humanos.** `sub_821A0A08` se ramifica según `G+0` (1 o 2). Los modos reto deben forzar 1 humano.

---

## 9. Receta de prueba en el juego (US)

1. Copia de la partida (`tools\long_run.ps1` la hace y la restaura). Juego visible y en segundo plano, como en `bg_battle.ps1`.
2. Sonda de solo lectura `DBZ3_WT_PROBE=1`. En cada cambio del byte `0x824BA110`, registrar:
   - `partida+1`, `+26`, `+27`;
   - `G+36688/36689/36690`;
   - las N entradas (`*0x824681D0 + 748`).
3. Menú principal → Torneo → Novato con un personaje. Comprobar en el registro:
   - N = 8;
   - una entrada con `+10 = 0` y 7 COM con `+10 = −1`;
   - las casillas aleatorias sin repetir.
4. Con `DBZ3_TEST_KO=1` (gana el jugador 1), avanzar 3 combates. Comprobar:
   - `+7` del humano 1 → 2 → 3;
   - `sub_821A05E0` → 5;
   - `partida+27` + 1.
5. Activar el gancho 2 con vida fija (por ejemplo 1500). Comprobar con `DBZ3_LAB=1` (`logs/lab.csv`) que la vida inicial del humano es 1500. Así se mide qué campo es +24.
6. Activar el gancho 1 con 7 casillas fijas. Comprobar que el cuadro las muestra y que los rivales coinciden.
7. Restaurar la partida.
