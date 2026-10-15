# 13 · Tag Team por la vía nativa (sin piloto ni fundido negro)

Fecha: 2026-10-15. Solo lectura del código recompilado (US). No se ha tocado ningún archivo existente ni se ha ejecutado el juego.

Citas: `N:línea` = `generated\dbz3_recomp.N.cpp:línea` (instrucción PPC comentada). **[C]** comprobado leyendo la instrucción; **[I]** inferido por uso; **[N]** sin confirmar (hay que medirlo con la receta del final).

## Resumen

1. El menú de Duelo **no prepara la rueda en el momento de elegir la fila**. Guarda dos números (tipo y fila) y crea la tarea de la rueda con ellos: `sub_821AD070(tipo, fila)`. **Tipo** 0 es Duelo y 1 es Práctica. Si se le pasa `-1, -1`, usa los dos últimos guardados en `0x823734E8` y `0x823734EC`. [C] `36:16712-16748`
2. La pantalla 14 se crea con `sub_821AEBA8(arg)`. Con **arg = 1** se salta el menú de Duelo y abre **directamente la rueda** con el modo guardado. Es el camino del "volver a elegir" tras un combate. [C] `11:16932-16954`
3. Por tanto, la entrada nativa es: escribir el tipo y la fila guardados, y pedir la pantalla 14 con argumento 1 (`sub_821D2778(14, 1)` y `sub_821D27B8(10, 0)`). Son las mismas llamadas que hace el menú principal (`10:14954-14995`). El juego hace su propio fundido. No hacen falta pulsaciones.
4. El ID que elige cada lado está en el estado de su rueda: `wst = *(rueda+48)`. Ahí, `+2` vale 1 cuando el lado ha elegido personaje y cápsulas. El orquestador `sub_821ACCA0` da paso al escenario cuando **los dos** lados tienen `+2 == 1` y `+0 >= 5`. [C] `6:16485-16537`
5. Para que cada lado elija varios miembros: un gancho **previo** a `sub_821ACCA0` ve `+2 == 1` antes que el juego. Apunta el miembro y devuelve la rueda a "eligiendo" (`+0 = 4`, `+2 = 0`). Es lo mismo que hace el juego cuando el lado 2 cancela en 1P VS COM (`20:16858-16868`).
6. Al pulsar atrás en la rueda, el juego vuelve al Duelo con `sub_821AC5A0` → `sub_821AC410` → `sub_821AEBA8(0, fila)` (`43:16726-16750`). Un gancho en `sub_821AEBA8` con `r3 == 0` en modo Tag puede pedir la pantalla 5, como hace el "atrás" del propio Duelo (`26:16566-16577`).

## 1. Hallazgos

### 1.1 Cola de pantallas (repaso, ver `docs/08_online/13_duelo_online.md`)

| Dirección | Qué es | Cita |
|---|---|---|
| `0x8247A988` | `REQ_ARG`, el argumento de la pantalla pedida | `14:19393-19395` (lo escribe `sub_821D3628`) |
| `0x8247A990` / `0x8247A98C` | `REQ_CUR` / `REQ_NEXT` | `16:18337-18362` (`sub_821D2778(id, arg)`) |
| `0x8247A994` | Bandera "pedido listo". La pone la llamada que se pasa a `sub_821D27B8(10, 0)`, cuyo destino es `0x821D27A8` [I] | `39:18463-18471` |
| `0x824BA110` | Byte de pantalla. Lo copia de `REQ_CUR` el estado maestro (5 menú principal, 14 Duelo/rueda, 19 Práctica) | `35:18842-18856` |
| `0x824BA111` | `UI_PAGE`, el modo de origen. Al salir del menú principal vale 1 para la pantalla 6, **2 para la 14** y 3 para la 16. Para la 19 se elige en `sub_821D63F0` (4 o 5) | `35:18858-18911` (`sub_821D7990`) |
| `0x824BA12C` | Código de enfrentamiento que escribe la rueda: 1 = 1P VS COM, 0 = 1P VS 2P, 2 = COM VS 2P, 3 = COM VS COM | `3:16675-16740` |
| `0x824BA12F + k` | ID confirmado por el lado k (ya usado en `src/select_ext.cpp:529`) | — |

Despacho del estado maestro mientras el byte de pantalla vale 5: `sub_821D7AE8` espera la bandera `0x8247A994`. Con `REQ_CUR == 14` llama a `sub_821D3C08`, que pone `UI+116 = 1` y llama a `sub_821AEBA8((s16)REQ_ARG, 0)`. [C] `17:19040-19113`, `33:18660-18698`

### 1.2 Cómo crea el Duelo su pantalla (`sub_821AEBA8`, `11:16876`)

- **`arg == 1`**: `sub_821AD940()`, después `sub_821AB488()` (crea el objeto global del select en `0x82467DE0` con los recursos 2056 y 2057), `sub_821AD070(-1, -1)` y `sub_821AD998()`. `sub_821AD998` libera el global del menú de Duelo `0x82467DE4`; si está vacío no hace nada. **No se ve ningún menú.** [C] `11:16934-16954`, `19:16558-16564`
- **Si no**: crea la tarea del menú de Duelo `sub_821AEAD8` y su objeto de 88 B en `tarea+48`. **`obj+83` es el cursor (fila)** y su valor inicial es `r4`. [C] `11:16957-17017`
  - Esto da una cadena fija hasta el cursor, que antes se seguía pulsación a pulsación (`src/ingame/online_panel.cpp:197-213`): `*(*(*(0x82467DE4)+16)+48)+83`. [C] `19:16560-16572`
- Las filas del Duelo se leen en `sub_821AE520`, con A = máscara `0x4000` del mando convertido `0x8238B780 + 184·u`:
  - fila 0 (1P VS COM): solo el mando 1;
  - fila 1 (1P VS 2P): exige otro mando de tipo 3 (conectado);
  - fila 2 (COM VS 2P): A de cualquier mando;
  - fila 3: COM VS COM. [C] `41:16655-16769`
- **Al confirmar**: `sub_821AE3D8(obj)` y `sub_821AB488()`. La tarea pasa a `0x821AE4B0`. Después se fija el mapa de mandos con `sub_821D98B8(fila < 2 ? *(0x8238B670) : -1, -1)`, que escribe `0x824E6694 = 6`, `0x824E66A0` y `0x824E66A4`. [C] `41:16820-16858`, `19:19102-19116`
- **`sub_821AE4B0`**: cuando acaba el fundido llama a `sub_821AD070(0, obj+83)` y a `sub_821AD998()`. [C] `16:16516-16523`
- **Atrás en el Duelo** (máscara `0x1000`): `sub_821AE440`, que llama a `sub_821D2778(5, 0)` y a `sub_821D27B8(10, 0)` y vuelve al menú principal. [C] `41:16905-16960`, `26:16525-16577`

### 1.3 Del modo a la rueda (`sub_821AD070` → `sub_821ACF60` → `sub_821AC098`)

`sub_821AD070(tipo, fila)` crea la tarea `0x821ACF60` con un objeto de 72 B en `tarea+56`. Guarda `tipo` en `tarea+52` y `fila` en `tarea+53`, y los copia también a `0x823734E8` y `0x823734EC` (s32). [C] `36:16659-16748`

`sub_821AC098` traduce la fila en dos ruedas, una por lado [C] `3:16675-16851`:

| Fila (`+53`) | `0x824BA12C` | Lado 1: clase / mando | Lado 2: clase / mando |
|---|---|---|---|
| 0 1P VS COM | 1 | `0x82` / 1 | `0x8B` / 0 |
| 1 1P VS 2P | 0 | `0x82` / 1 | `0x85` / 2 |
| 2 COM VS 2P | 2 | `0x8C` / 0 | `0x85` / 2 |
| 3 COM VS COM | 3 | `0x86` / 0 | `0x8F` / 0 |

- **Mando**: 0 es COM y 1 o 2 es el mando humano (r5 de `sub_8217D2D0`).
- **Clase**: el bit 0 es el lado (derecha). Los bits 1 a 3 combinan humano o COM con quién elige por él [I].
- **Modo de rueda**: `r3` vale **2** si `tipo == 0` (Duelo) y **4** si `tipo == 1` (Práctica). [C] `3:16769-16790`
- Cada rueda se crea con `sub_8217D2D0(modo, clase, mando, -1, …)`. Los últimos valores de cada lado salen de `0x8245F178` (+8/+10, +12/+13, +18/+19, +20/+21) [I: la última elección]. Las ruedas quedan en `obj56+0` (lado 1) y `obj56+4` (lado 2). [C] `3:16798-16851`
- Al acabar, la tarea pasa al orquestador `0x821ACCA0`. [C] `16:16458-16462`

### 1.4 Estado de cada rueda y cuándo acaba un lado

`wst = *(rueda+48)` (es el mismo estado que ya leen los ganchos de `select_ext.cpp`):

| Campo | Significado | Cita |
|---|---|---|
| `wst+0` | Fase: ≤3 entrada, **4 eligiendo**, ≥5 confirmado, 7 cerrando (lo pone `sub_8217C9E8`), 8 cerrado | `6:13496` (lectura), `25:13755` (`sub_8217AB80` = escritura), `29:14038`, `43:16704-16724` |
| `wst+2` | Resultado: **1 = elegido** (personaje y cápsulas), -1 = atrás | `27:13783`, `select_ext.cpp:476-480` |
| `wst+6` | Bit `0x08` = bloqueada esperando al otro lado | `20:16779-16793`, `20:16858-16864` |
| `wst+7` | Lado (0 o 1) | `select_ext.cpp:476` |

Orquestador `sub_821ACCA0` (cada fotograma) [C] `6:16257-16537`:

1. Si alguna rueda tiene `+0 <= 3`, sale.
2. Despacha según la fila: 0 → `sub_821AC610`, 1 → `sub_821AC790` (más `sub_821D98B8`), 2 → `sub_821AC938`, 3 → `sub_821ACAE0`.
3. Si **las dos** ruedas tienen `+2 == 1` y `+0 >= 5`, pone `obj+61 = 3` y la tarea pasa a `0x821AC508` (escenario y combate [I]).

Turnos en 1P VS COM (`sub_821AC610`) [C] `20:16646-16877`:

- Lado 1 eligiendo (`+0 == 4`) y atrás: los dos `sub_8217C9E8` y `sub_821AC5A0` (salir al Duelo).
- Lado 1 con `+2 == 1`: se quita el bit `0x08` del lado 2 y se le pone `+0 = 4` (le toca al COM, con el mando 1).
- Lado 2 eligiendo y atrás: el lado 2 vuelve a bloquearse (`|= 0x08`) y **el lado 1 vuelve a `+0 = 4`**. Esta es la "re-elección" nativa que se aprovecha en el paso 2.

### 1.5 Atrás desde la rueda

- `sub_821AC5A0` [C] `26:16362-16430`:
  - llama a `sub_821AB740(obj)`, que cierra las ruedas;
  - después, según el tipo: 0 → `sub_821AD130()` (vuelve a cargar los recursos del Duelo, 2054 y 2055, en `0x82467DE4`); 1 → `sub_82199D70()` (Práctica);
  - la tarea pasa a `0x821AC410`.
- `sub_821AC410` espera a que las dos ruedas tengan `+0 == 8` [C] `43:16704-16759`:
  - si el tipo es 0, llama a `sub_821AEBA8(0, fila)` y vuelve el menú de Duelo con el cursor en la fila;
  - si es 1, llama a `sub_8219BBB0(0, 0)` (menú de Práctica);
  - después limpia con `sub_821AB588` y `sub_821AB918`.

### 1.6 Práctica (pantallas 19 y 20)

- Elección de la fila (`sub_8219ADB0`) [C] `5:15583-15601`:
  - pone `UI_PAGE = 4` y `0x824BA12C = (fila != 0) + 1`;
  - llama a `sub_821AD070(1, fila)`; la fila está en `obj+99` del menú de Práctica.
  - Diferencias con el Duelo: el modo de rueda es 4 en vez de 2, la salida va al menú de Práctica y no hay `sub_821D98B8`.
- Tras el combate, `sub_821D3628` [C] `14:19183-19272`:
  - `UI_PAGE` 4 → pantalla 19 con argumento 1;
  - `UI_PAGE` 5 → pantalla 19 con argumento 2 o 3;
  - `UI_PAGE` 2 (Duelo) → **pantalla 14 con argumento 1** (vuelve a la rueda con el modo guardado);
  - `r3 == 2` → pantalla 5.
- La 20 es el combate de práctica (sin KO ni tiempo). Para Tag Team **no** conviene usar el tipo 1; se usa el Duelo (tipo 0).

## 2. Plan paso a paso

Todo va en un archivo nuevo, `src/tagteam_native.cpp`. Solo la versión US: en la EU, con `dbz3::g_guest_eu`, se sigue como ahora, igual que los ganchos de Tag Team de `select_ext.cpp`. Los nombres EU (`dbz3eu_*`) de `sub_8218C5D8`, `sub_821ACCA0` y `sub_821AEBA8` están sin localizar [N]; mientras tanto, gancho solo en US con `REX_HOOK_RAW`.

### Paso 0 · Medir antes de cambiar (solo registro)

Gancho de solo lectura en `sub_821ACCA0`. Cuando algo cambie, registra en una línea:

- para cada lado: `wst+0`, `+2` y `+6`, y `rueda+28`;
- `tarea+53`.

Hay que confirmar tres cosas [N]:

- después de un `+0 = 4` escrito a mano, `rueda+28` vuelve a `0x8217CC68` (eligiendo);
- `+2` se pone a 0 solo o hay que ponerlo;
- en qué fase está `+0` cuando `+2` pasa a 1.

### Paso 1 · Entrada directa a la rueda

Se hace dentro del estado del menú principal, para que la salida sea la del propio juego:

```cpp
// src/tagteam_native.cpp
#include <rex/hook.h>
#include "generated/dbz3_init.h"
#include "guest_region.h"
namespace {
constexpr uint32_t kSavedType = 0x823734E8, kSavedRow = 0x823734EC;   // 36:16714-16744
constexpr uint32_t kSlots = 0x8238B670, kUi = 0x824BA110;
std::atomic<int> g_enter_row{-1};   // fila del Duelo pedida por el panel: 0, 1 o 3
std::atomic<bool> g_native{false};  // Tag nativo activo (de la entrada a la vuelta al menu)
}
void dbz3_tag_native_enter(int duel_row) { g_enter_row = duel_row; g_native = true; }
bool dbz3_tag_native() { return g_native.load(); }

// Estado del menu principal (tarea+28 = sub_8218C5D8). Se repite su rama de A (10:14954-15003):
// el juego hace su fundido y su sonido, y nosotros no pulsamos nada.
// OJO: sub_8218C5D8 YA tiene gancho (src/ingame/online_panel.cpp:763, DBZ3_HOOK con EU
// dbz3eu_sub_8218C5A0). Este bloque va DENTRO de ese gancho, antes de orig(), como funcion
// bool dbz3_tag_native_main_menu(ctx, base) que devuelve true si ya hizo la salida.
// No se crea un segundo gancho.
bool dbz3_tag_native_main_menu(PPCContext& ctx, uint8_t* base) {
  const int row = g_enter_row.exchange(-1);
  if (row < 0 || dbz3::g_guest_eu) return false;
  const PPCContext saved = ctx;
  const uint32_t task = ctx.r3.u32, menu = REX_LOAD_U32(task + 48);
  REX_STORE_U32(kSavedType, 0);            // Duelo (1 seria Practica)
  REX_STORE_U32(kSavedRow, uint32_t(row));
  REX_STORE_U8(menu + 150, 1);             // el menu recuerda Duelo (10:14894, 14974-14978)
  REX_STORE_U8(kUi + 37, 1);
  ctx.r3.u64 = row < 2 ? REX_LOAD_U32(kSlots) : uint64_t(-1);   // como 41:16839-16858
  ctx.r4.u64 = uint64_t(-1);
  sub_821D98B8(ctx, base);
  ctx.r3.u64 = 14; ctx.r4.u64 = 1;         // pantalla 14, argumento 1 = rueda directa (11:16934)
  sub_821D2778(ctx, base);
  ctx.r3.u64 = menu;  sub_8218C588(ctx, base);
  ctx.r3.u64 = REX_LOAD_U32(menu + 164); sub_82081A48(ctx, base);
  ctx.r3.u64 = 10; ctx.r4.u64 = 0;  sub_821D27B8(ctx, base);
  ctx = saved;
  REX_STORE_U32(task + 28, 0x8218C4C8);    // estado "saliendo" del menu (10:14998-15003)
  return true;
}
// En online_panel.cpp:763, tras anotar la fila:  if (dbz3_tag_native_main_menu(ctx, base)) return;  orig(ctx, base);
```

Comprobar después:

- `UI_PAGE` vale 2 (lo pone `sub_821D7990`, `35:18894-18911`);
- tras el combate, "elegir personaje" vuelve a la rueda en modo Tag (pantalla 14 con argumento 1, `14:19307-19327`).

### Paso 2 · Varios miembros por lado en la propia rueda

```cpp
// Antes que el juego decida el turno o el final (6:16257). El ultimo miembro no se toca:
// sigue el flujo normal hacia el rival o el escenario.
REX_HOOK_RAW(sub_821ACCA0) {
  if (g_native.load() && !dbz3::g_guest_eu) {
    const uint32_t obj = REX_LOAD_U32(ctx.r3.u32 + 56);
    for (uint32_t k = 0; k < 2; ++k) {
      const uint32_t wheel = REX_LOAD_U32(obj + 4 * k), wst = wheel ? REX_LOAD_U32(wheel + 48) : 0;
      if (!wst || int8_t(REX_LOAD_U8(wst + 2)) != 1) continue;          // como 20:16768-16777
      const int id = REX_LOAD_U8(0x824BA12F + k);
      if (!dbz3::ingame::NativeAddMember(int(k), id)) continue;           // false = equipo completo
      REX_STORE_U8(wst + 2, 0);
      REX_STORE_U8(wst + 0, 4);              // = sub_8217AB80(rueda, 4), como 20:16858-16868
    }
  }
  __imp__sub_821ACCA0(ctx, base);
}
```

- Las cápsulas de cada miembro las sigue apuntando el gancho de `sub_8217CB10` que ya existe (`select_ext.cpp:481-522`, `OnCapsuleChoice` con `chosen = true`). El miembro llega con su opción de verdad y no hace falta convertir A en Y.
- Un ID repetido no se añade: el gancho vuelve a poner la rueda en 4 y no suma el miembro.
- **Plan B**, si el paso 0 muestra que `+0 = 4` no devuelve la rueda a `0x8217CC68`: escribir en `sub_8217CB10`, justo después de `orig`, el resultado "atrás" (`tst+2 = -1`) para los miembros que no son el último. Es lo mismo que ahora consigue la Y inyectada (`task+28` pasa a `0x8217CC68`), pero sin tocar el mando. [N]

### Paso 3 · Atrás desde la rueda → nuestra pantalla

```cpp
// sub_821AC410 llama aqui con r3 = 0 y r4 = fila cuando la rueda se cierra por "atras" (43:16744-16750).
REX_HOOK_RAW(sub_821AEBA8) {
  if (!g_native.load() || dbz3::g_guest_eu || ctx.r3.s32 != 0) { __imp__sub_821AEBA8(ctx, base); return; }
  g_native = false;
  const PPCContext saved = ctx;
  sub_821AD998(ctx, base);                   // suelta el global del Duelo de sub_821AD130 (si es nulo no hace nada)
  ctx.r3.u64 = 5; ctx.r4.u64 = 0;  sub_821D2778(ctx, base);   // como el atras del Duelo (26:16566-16577)
  ctx.r3.u64 = 10; ctx.r4.u64 = 0; sub_821D27B8(ctx, base);
  ctx = saved;
  dbz3::ingame::TagTeamReopenOnMainMenu();   // el panel se abre al ver la pantalla 5
}
```

- **Riesgo**: se salta el comienzo de `sub_821AEBA8` (`sub_821D27F8`, `sub_82186220`, `sub_82165CF8` y `sub_8216B470(6,…)`, `11:16896-16929`), que prepara el fundido y la música [I].
- **Plan B, 100 % del juego**: dejar que `sub_821AEBA8(0, fila)` cree el menú de Duelo y, en su primer `sub_821AE520`, llamar a `sub_821AE440(obj)` (su propio atrás) en vez de `orig`. El Duelo se ve unos fotogramas mientras funde a negro.

### Paso 4 · Qué se quita de `src/ingame/tagteam_panel.cpp`

| Líneas | Qué es | Qué lo sustituye |
|---|---|---|
| 150-179 | `PilotReady` / `PilotToDuel` (`Poke` del cursor, pulsaciones, `WaitScreen(14)`, `PressToLeave`) | `dbz3_tag_native_enter(fila)` (paso 1) |
| 190-202, 206 | `Gate`, `StartGate`, `TagTeamGate` | Nada: el fundido es el del juego |
| 515-516 | `StartGate(kToWheel)` y `PilotToDuel` | `dbz3_tag_native_enter(row_ == 0 ? 0 : row_ == 1 ? 1 : 3)` |
| 696-725 | Detección de "Duelo en modo Tag", `PressToLeave(kY)` y `WaitScreen(5)`, rectángulo negro | Paso 3 |
| 229, 243-246 | `PressUntilBrowse(kY)` para un repetido, `g_await` | Paso 2 (`+0 = 4`) |
| 297-327 | Conversión de A en Y en el menú de cápsulas (`g_converted`, `g_eat_a`, `g_choice`) | Paso 2 (`+2 == 1` real) |

- **Se queda**:
  - `DrawMenu`;
  - `DrawPicking` (tira de miembros);
  - Y = "quitar el último" (330-338, solo filtra Y mientras la rueda elige);
  - formas LT/RT;
  - `OnCapsuleChoice`;
  - el final de modo al volver a la pantalla 5 (663-670).
- `menu_pilot.cpp` sigue sirviendo al online (`netplay_hooks.cpp`). La nueva cadena del cursor del Duelo (§1.2) también permite quitar el seguimiento por pulsación de `online_panel.cpp`, pero eso es otro trabajo.

## 3. Riesgos

1. **Paso 2**: que `+0 = 4` no devuelva la rueda a eligiendo, o que `+2` no se limpie. Se mide con el paso 0 y, si falla, se usa el plan B.
2. **Mapa de mandos**: en la entrada directa no pasa por la confirmación del Duelo, así que `sub_821D98B8` se llama a mano. Si no se llama, en 1P VS COM el aviso de mando desconectado puede seguir al mando equivocado (ver la nota del mando 4 en la memoria del proyecto).
3. **1P VS 2P sin segundo mando**: el Duelo no deja entrar (`41:16742-16769`), pero la entrada directa no lo comprueba. El panel ya lo bloquea (`tagteam_panel.cpp:498-500`); hay que mantenerlo.
4. **Fila 2 (COM VS 2P)**: TAG TEAM no la usa. Si se añadiera, el lado 1 es el COM y lo elige el mando 2.
5. **EU**: las direcciones `0x823734E8/EC` y `0x82467DE0/E4`, y los nombres `dbz3eu_*`, no están medidos. Hasta medirlos, en EU se sigue con el piloto.
6. **Online**: no se toca. El online elige con su propio camino.
7. **Partida guardada**: `0x824BA110+37` es la fila recordada del menú principal y forma parte del bloque que viaja online como foto. Se escribe 1 (Duelo), que es lo mismo que hace el juego.

## 4. Prueba en el juego (`tools\pruebas.py`, copia aislada)

Compilar el release y **volver a copiar las DLL canónicas** (AGENTS.md §4 y §7). Después, escenario `tag_nativo.toml` (de prueba, fuera del repo):

```toml
descripcion = "Tag nativo: TAG TEAM -> rueda directa, 2 miembros por lado, atras vuelve al panel"
env = ["DBZ3_TAG_NATIVE=1"]
pasos = [
  "esperar_pantalla 5 60",
  "tecla L 3000", "captura 01_panel",                 # X abre TAG TEAM
  "tecla Space 6000", "captura 02_rueda",             # 1P VS COM: debe salir la rueda, sin menu de Duelo
  'log "dbz3 tag nativo: rueda" 10',
  "tecla Space 3000", "tecla Space 4000",             # miembro 1 + Normal
  'log "lado 1 miembro 1/2" 10', "captura 03_m1",
  "tecla S 1500", "tecla Space 3000", "tecla Space 4000",
  'log "lado 1 miembro 2/2" 10', "captura 04_turno_com", # ahora elige el COM
  "tecla P 3000", "tecla P 3000", "tecla P 3000",     # quitar y salir
  "esperar_pantalla 5 20", "captura 05_vuelta_panel",
]
```

Comando: `python tools\pruebas.py correr tag_test tag_nativo.toml`.

Qué hay que ver:

- **`02_rueda`**:
  - sin el menú de Duelo y sin nuestro negro;
  - en el registro, `UI_PAGE = 2` y `0x824BA12C = 1`.
- **`03_m1`**: el lado 1 vuelve a "eligiendo" (paso 0: `rueda+28 = 0x8217CC68`).
- **`04_turno_com`**: el lado 2 se desbloquea solo después del segundo miembro.
- **`05_vuelta_panel`**: el panel TAG TEAM encima del menú principal.
- **Repetir con un combate completo**:
  - al terminar, "elegir personaje" debe volver a la rueda en modo Tag;
  - probar también 1P VS 2P con dos mandos y COM VS COM.
