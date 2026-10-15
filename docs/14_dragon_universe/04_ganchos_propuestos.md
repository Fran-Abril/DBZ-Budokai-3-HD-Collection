# 04 — Propuestas para el frente de campañas (`src/campaigns.cpp`, `campana.py`)

Son bocetos, sin compilar ni probar. Todo va solo en la versión US, como el resto de `campaigns.cpp`. Cada punto dice qué hay que medir antes.

## P0. Lo que ya se puede hacer sin ganchos (solo datos de las escenas)

- **Arreglar `MAP_EVENT_WORDS`** (`campana.py:358`): añadir `3: 4, 30: 7, 31: 6, 34: 4, 41: 9, 49: 4`. Sin ellos, un evento así desalinea los siguientes (ver `02`).
- **Evento 49 = escribir en el bloque de la casilla.** La palabra 1144 son los bytes +4576..+4579 (big-endian): planeta, casilla, modelo, traje.
  `(49, 0, 1144, (planeta << 24) | (casilla << 16) | (modelo << 8) | traje)` cambia el aspecto en el mapa **y queda guardado**, al contrario que `modelo_mapa`, que lo deshace tras crear el personaje. Un día puede sustituir parte del gancho de `sub_821C24B8`. Cuidado: la casilla (+4577) tiene que seguir siendo la suya. [? probar en el juego]
- **Premios nativos** por capítulo: `(17, 0, niveles)`, `(18, 0, zeni)`, `(19, 0, cápsula)`, `(53, máx, cápsula)`. Las pantallas de nivel y de cápsula salen solas al acabar la escena.
- **Forma o personaje en un combate concreto**: `(51, 0, casilla)`, `(45, 0, modelo)`, `(46, 0, traje)` en la escena del combate. Podría sustituir a `[forma_jugador]` [? medir qué los lee el combate].
- **Visor del scouter**: `(40, 0, 0)` lo enciende. Para apagarlo: `(49, 0, 1145, ...)` (palabra de +4580..+4583) [? no hay evento que lo apague; ojo, esa palabra también lleva el nivel en +4583].

## P1. Casilla de DU nueva e independiente (la vía recomendada)

**Hecho clave:** las 38 casillas del select (0-37) ya tienen su bloque de 4624 B en memoria. El juego guarda en el archivo el nivel y las cápsulas de las 38. Solo 11 tienen historia. Quedan **27 bloques libres** (1, 5, 6, 8, 9, 14-16, 18-33, 35-37). Freezer (22) ya usa el suyo. No hace falta crear bloques.

Para un personaje nuevo (plaza extra, ID ≥ 44, celda virtual del select):

1. **Elegir una casilla anfitriona libre H** (por ejemplo 36 o 37) que no esté en `0x4_00023C9D` ni en otra `[rueda]`. Nuevo `campana.toml`: `[rueda] casillas = 36` (casillas directas, no IDs). Hay que añadirlo a `g_wheel_ids`/`ExtraSlots` para la rueda y el guardado (`src/campaigns.cpp:172-177`, `416-424`).
2. **Celda virtual → H al confirmar.** `sub_821CA020` y `sub_821C9DC8` leen la casilla elegida en el u16 `0x8245F140` (`G/dbz3_recomp.7.cpp:18261`, `.32.cpp:17938`) y la guardan en registros. Hay que cambiarla **antes** de que la lean:
   ```cpp
   // ponytail: una casilla por celda virtual; tabla en campana.toml [rueda] celda_a_casilla
   static void RemapDuCell(uint8_t* base) {
     const int cell = dbz3::select_ext::LastConfirmedCell();   // [?] no existe: exponerlo desde select_ext
     if (const auto it = g_cell_slot.find(cell); it != g_cell_slot.end())
       REX_STORE_U16(0x8245F140, uint16_t(it->second));
   }
   REX_HOOK_RAW(sub_821CA020) { if (!dbz3::g_guest_eu) RemapDuCell(base); __imp__sub_821CA020(ctx, base); }
   REX_HOOK_RAW(sub_821C9DC8) { if (!dbz3::g_guest_eu) RemapDuCell(base); __imp__sub_821C9DC8(ctx, base); }
   ```
   [?] Medir que `0x8245F140` no se usa después para el luchador del select. Si se usa, devolverlo al salir de DU.
3. **Personaje en el mapa y en los menús de DU.** El ID sale de la casilla con la fórmula `sub_82156E88`, que solo llaman 4 funciones de DU (LR `0x821C251C` mapa, `0x821C6DBC` mapa 2.º camino, `0x821CD354` y `0x821D067C` menús de DU). Un gancho único:
   ```cpp
   REX_HOOK_RAW(sub_82156E88) {        // r3 = casilla -> r3 = ID
     const uint32_t slot = ctx.r3.u32;
     __imp__sub_82156E88(ctx, base);
     if (!dbz3::g_guest_eu) if (const auto it = g_slot_id.find(slot); it != g_slot_id.end()) ctx.r3.u64 = it->second;
   }
   ```
   Ojo: el 2.º camino de creación del mapa (`sub_821C6BB8`, `.33.cpp:18220-18240`) **no pasa por el gancho de `modelo_mapa`** de `sub_821C24B8`. Puede explicar casos en que el aspecto vuelve al normal [?].
4. **Animaciones del mapa**: H usa sus entradas de `0x823742C8` / `0x823745C0`, que son de otro personaje. Usar `animacion_mapa` (ya existe) con la casilla del donante del esqueleto.
5. **Intro**: la tabla de `sub_82156DC8` ya da una intro a cada casilla (36 → 100, 37 → 104). La historia del mod escribe la entrada H de ese contenedor de intro (lo que ya hace `campana.py` con la casilla). Para empezar en otra saga, un gancho de salida:
   ```cpp
   REX_HOOK_RAW(sub_82156DC8) { const uint32_t s = ctx.r3.u32; __imp__sub_82156DC8(ctx, base);
     if (auto it = g_intro.find(s); it != g_intro.end()) ctx.r3.u64 = (uint32_t(it->second) << 16) | s; }
   ```
   El planeta inicial lo decide `sub_8215C728` (102 → Namek). Con otra intro, poner +4576 con el evento 49 en la propia intro, o con un gancho de salida de `sub_8215C728`.
6. **Combate**: el jugador del combate parece salir de la casilla de la escena (+324, copia del bloque +4577). Falta medir si el combate pasa la casilla a ID con `0x82020618` (tabla del select) o con la fórmula. Si es con la tabla, la plaza extra necesita `(51, 0, casilla_con_ese_ID)` o un gancho en el arranque del combate de historia [? sonda en `sub_8215A7A8` y en quien lee +324].
7. **Experiencia**: H usa la curva `0x82374A20[H]` del personaje anfitrión. Para otra curva, hay que cambiar el puntero de H mientras se juega (es de solo lectura: hacerlo con el patrón de páginas de `stage_ext.cpp`) [?].

Prueba mínima: celda virtual → H = 37 → "Nueva historia" → `dbz3 campanas` registra la intro `104.37` → guardar → cargar. El bloque 37 vuelve desde `historias_extra.bin`.

## P2. Casilla 38 o mayor (no recomendado)

Los 38 bloques acaban justo en la máscara u64 (`0x824A5890`), el zeni y el byte de casilla. Una casilla ≥ 38 escribiría encima. Además, 22 funciones calculan `0x8247AA30 + 4624·casilla` en línea (`mulli 4624`) y las tablas por casilla tienen 38 entradas. Si de verdad se acaban los 27 bloques libres, la vía es un **bloque en la sombra**: guardar 4624 B en el host y cambiarlo por el de una casilla libre al confirmar en la rueda (P1.2), y devolverlo al guardar o al salir de DU. Es lo mismo que P1, pero sin perder la historia del anfitrión.

## P3. Planeta nuevo

`sub_821BCC58` y `sub_821C7D30` solo conocen los planetas 0-2. Boceto: con +4576 = 3, durante la llamada se pasa 0 (la Tierra) y se redirigen las entradas 3782-3789 y `0x10000|1963/1964` a las del mod (redirección AFS del port). Después se devuelve 3. [?] Medir qué más mira el planeta: el byte `0x82373E2C[planeta]`, el evento 47 (< 3) y las zonas de las marcas.

## P4. Medidas que faltan (sondas baratas)

- Quién lee el objeto de escena +324/+328/+332 al arrancar el combate (para P1.6).
- Qué hace `sub_8215BED0`: borra el bit de "historia empezada" y carga 3849/3850 [? fin de historia o créditos]. Se llama por puntero, no por `bl`.
- Lista US de nombres de data_cmn 3780-3860 (archivos de los planetas).
- Las pantallas 16384.N (`sub_82160688`/`sub_821602F0`): qué es cada N además de 4 (guardar).
