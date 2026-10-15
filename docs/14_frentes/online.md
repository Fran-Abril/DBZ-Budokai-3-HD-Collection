# Frente: Online (ronda 2, 2026-10-15) — ficha 02 (retardo adaptativo)

Solo US (como todo el online). Copias de prueba: `D:\DBZ3HD\online_a` (anfitrión) y `D:\DBZ3HD\online_b`
(invitado), refrescadas con `pruebas.py prep`. Script: `E:\DBZ3HD_trabajo\online\online_lag.py`.

## Qué ve el jugador
- **Antes**: el retardo (RETARDO 1-8, por defecto 3) se elegía al crear la partida y no cambiaba. Con mala
  conexión el juego iba a tirones o a cámara lenta (el más lento frena a los dos); con buena, sobraba retardo.
- **Ahora**:
  - En la pantalla ONLINE, la fila **RETARDO** empieza en **AUTO** (izquierda/derecha: AUTO, 1 ... 8). El Kaio
    explica: "AUTO: el juego ajusta el retardo a la conexión, también en pleno combate. Lo decide quien crea la
    partida". Con un número sigue siendo fijo, como antes.
  - Jugando online (rueda, menús y combate), abajo a la izquierda, un **indicador con el marco y la letra de los
    menús del juego**: `RETARDO 3 AUTO`, cuatro barras doradas de conexión y el ping en ms (lo mismo en los dos
    PCs). Cuando el retardo cambia, el indicador se ilumina 2 s con la franja de la fila elegida del juego.
  - Con AUTO, el anfitrión mide el ping cada segundo y sube el retardo enseguida si hace falta (o si el juego
    tiene que esperar al otro PC), y lo baja de uno en uno cuando sobra. El invitado sigue el del anfitrión. El
    juego sigue a 60 por segundo aunque el ping suba a 220 ms, y no se pierde ninguna pulsación.

- **Sala con el rango (ficha 01, propuesta 5 de `rpg.md`)**: al crear o unirse, bajo las filas sale un marco
  con dos filas, como la página de Ranking del HUB: **TÚ** (con la franja) y **RIVAL**, cada una con la medalla
  de su tramo (`retos/medalla_<tramo+1>_*`), el nombre del rango (Guerrero Z, Super Saiyan, Kaio, Kaio-shin,
  Shenron) y el Elo. El del rival llega de su PC al conectar; mientras, "Esperando...". Sin combates: "Sin
  combates". El rango propio sale de `dbz3::modos::PerfilJugador` (solo US; en la build solo EU no hay rango).
  Mensaje nuevo `ROOM` (elo u16, tramo u8, reglas u8, reversión u16), enviado junto al del equipo cada medio
  segundo antes de jugar: no cambia los mensajes que ya había. Sin la sala del otro no se puede empezar.
- **Reglas opcionales del combate en el online (propuesta de `combate.md`)**: antes, F1 > COMBATE (OPCIONAL) se
  apagaba siempre en el online. Ahora la sala compara las cuatro que cambian la simulación (`dbz3_combate_reglas_v0`,
  `_definitiva_directa`, `_boton_cancel`, `_reversion`) tal como estaban al crear/unirse. Tercera fila **REGLAS** de la
  sala: "Combate original" (ninguna), la lista (p. ej. `V0  DEF. DIRECTA  CANCELACION RB  PIERDE FORMA`, iluminada:
  van activas) o "Distintas: combate original" (en gris: apagadas, como antes). El Kaio lo dice también. Durante la
  partida quedan fijas: si alguien las cambia en F1, vuelven a las de la sala (si no, un PC simularía otra cosa).
  Con ellas activas sale en combate la línea de reglas de `combate.cpp` («REGLAS V0 ...»), como fuera del online.
  **Única línea fuera de mis ficheros** (pedida por el coordinador): `Rules()` de `src/combate.cpp` pasa de "nunca en
  online" a "en online solo si `Session::CombatRulesAgreed()`" (copia previa en `E:\DBZ3HD_trabajo\online\orig\`).

## Cómo funciona (para quien toque `src/net`)
- Protocolo 4 (`kProto`): cada INPUT lleva además una marca de tiempo, el eco de la del otro, lo que tardó en
  contestar y el retardo del que lo manda (+0x80 si es AUTO). Ping = ahora − eco − espera.
- Sin "tick de activación acordado": cada PC llena sus casillas de mando una sola vez y en orden (`sched_`). Al subir
  D, la muestra actual va también a la casilla siguiente (máx. una de más por tick: cada casilla viaja en varios
  paquetes); al bajarlo solo se salta una muestra igual a la anterior (`SameInput`). Los dos PCs aplican la misma
  casilla en el mismo tick, así que siguen iguales aunque cambien D en ticks distintos.
- Decisión (anfitrión, cada 60 ticks): D = ⌈(peor ping del segundo / 2 + 4 ms) · 60 / 1000⌉, entre 2 y 10; si ≥ 10 %
  de los ticks esperaron 3-100 ms al otro, D + 1 y 10 s sin bajar. Baja tras 3 s sobrando, uno por segundo.
- Los paquetes que esperaron en el socket durante una carga no cuentan como ping (de cada tanda leída de golpe solo
  la muestra más pequeña). Sin esto el retardo subía a 10 al entrar en cada combate.
- Además: ahora se lee el socket en cada tick (antes solo al esperar), y la comprobación del RNG también se hace
  cuando el dato del otro llega antes que el propio. Cada 10 s el log dice
  `dbz3 online: tick N: retardo D AUTO, ping X ms (peor Y), E esperas, K comprobaciones del RNG iguales`.

## Ficheros tocados (todos de este frente)
- `src/net/netplay.h`, `src/net/netplay.cpp`: protocolo 4, ping, `sched_`, `Drain`/`FlushRtt`/`AdaptDelay`/`SetDelay`,
  comprobación del RNG en los dos sentidos.
- `src/ingame/online_panel.h`, `src/ingame/online_panel.cpp`: fila RETARDO con AUTO (por defecto) y `NetBadge`.
- Originales antes de la ronda: `E:\DBZ3HD_trabajo\online\orig\`.

## Pruebas
Dos juegos en este PC con un relé UDP en medio (`online_lag.py`: retrasa, desordena y pierde paquetes según un
guion: 0 ms → 120 ms ±15 y 2 % de pérdida → 220 ms ±20 y 3 % → 40 ms → 0 ms, en pleno combate y con mandos al azar
en los dos). Ver la tabla de resultados más abajo.

Comando: `python E:\DBZ3HD_trabajo\online\online_lag.py [--equipo] [--fijo N] [--seg S]`. Cada pasada deja en su
carpeta las capturas de los dos PCs (`10_<segundo>_<pc>_ping<N>`: cada 5 s; `20_cambioNN_*`: justo al cambiar el
retardo, con el indicador iluminado), los dos logs y `veredicto.txt`.

| Pasada (`D:\DBZ3HD\_pruebas\`) | Resultado | Qué demuestra |
|---|---|---|
| `20261015-090230_online_lag` (1.ª versión) | PASS, 0 desincronías, 130 comprobaciones del RNG iguales en cada PC | El retardo sube y baja en pleno combate y los dos siguen iguales. Se vio un fallo: al cargar el combate el retardo subía a 10 (paquetes que esperaban en el socket contaban como ping). |
| `20261015-091341_online_lag` | PASS, 0 desincronías, 130 iguales | Arreglado: sin subida al cargar; baja 9 → 3 en 6 s. Quedaba un ir y venir 8 ↔ 9 con 220 ms. |
| **`20261015-092911_online_lag_equipo`** (final) | **PASS**, 0 desincronías, 129 iguales, 11 cambios en los dos PCs, 0 esperas fuera de los cambios | Versión final (con margen al bajar). Retardo 2 con 0 ms, 5 con 120 ms, 9 con 220 ms, 3 con 40 ms, 2 otra vez; juego a **60 ticks/s** en todas las fases (58-59 solo en el segundo del salto). Con `DBZ3_NET_TEAM` en los dos: el equipo de Tag Team sigue sin viajar y se juega 1 contra 1, como antes. |
| `20261015-094754_online_lag_fijo3` (retardo fijo 3, para comparar) | PASS, 0 desincronías, 80 iguales | El modo fijo sigue igual que antes. Con 120 ms el juego baja a 57 ticks/s y con 220 ms a **35 ticks/s** (cámara lenta): lo que AUTO evita. |

| `20261015-105110_online_lag` (con rango en la sala) | PASS, 0 desincronías, 130 iguales | La sala con el rango no rompe nada. Captura de la sala: `...110739_online_lag\02_conectados_*.png` (pasada corta solo para la captura, con las medallas: TÚ Kaio 1193 oro / RIVAL Super Saiyan 1011 plata, y al revés en el otro PC; su FAIL es solo "no hubo subidas de retardo en 20 s"). |
| `20261015-111936_online_lag_reglas` (mismas reglas en los dos: V0, definitiva directa, cancelación RB, pierde forma con 100) | PASS, 0 desincronías, 130 iguales | Log "reglas de combate iguales: activas" en los dos; las reglas actuaron 63 veces (24 definitivas directas, 39 cancelaciones) y **las 63 líneas son idénticas en los dos PCs** (mismo cuadro). Sala: fila REGLAS con la lista iluminada (`02_conectados_*.png`). |
| `20261015-112909_online_lag_reglas` (anfitrión V0 + RB, invitado ninguna) | PASS, 0 desincronías, 130 iguales | "distintas: apagadas (combate original)" en los dos; 0 usos de reglas. Sala: "Distintas: combate original" en gris. Indicador subido a y 806 para no pisar la línea de reglas de `combate.cpp`. |

Datos de prueba del rango: `combates.csv` de las copias `online_a`/`online_b` reescritos con combates inventados
(los de antes, en `E:\DBZ3HD_trabajo\online\respaldo_combates\`).

Capturas para ver el indicador: `...092911_online_lag_equipo\03_rueda_invitado.png` (rueda),
`10_059_invitado_ping220.png` (combate, `RETARDO 9 AUTO`, 1 barra, ~220 ms), `20_cambio04_invitado_047.png` (recién
cambiado, con la franja del juego), `01_anfitrion_ajustes.png` (fila RETARDO AUTO y el Kaio).
Compilado con `python tools\compilar.py` (OK). Batería: ver abajo.

Batería `python tools\pruebas.py todo` con este código: **5/5 PASS** (`D:\DBZ3HD\_pruebas\20261015-102023_todo.txt`).
Con el rango y las reglas en la sala (y la línea de `combate.cpp`): otra vez **5/5 PASS**
(`D:\DBZ3HD\_pruebas\20261015-115715_todo.txt`).

## Puntuación de la ficha 02 (entregable mínimo con tres partes)
| Parte | Mínimo de la ficha | Estado | Nota |
|---|---|---|---|
| A. Netplay | "F1 (input delay adaptativo) — mejora perceptible sin rollback" | **HECHO** | 1 · pruebas de arriba (60 ticks/s con 220 ms frente a 35 con retardo fijo) |
| B. Reiluminación | Toggle experimental `dbz3_relight` (SSAO/SSGI) en el launcher, apagado, con aviso | SIN EMPEZAR | 0 · no es de este frente (render del runtime y launcher) |
| C. Unificación | Un minijuego de IW como escena de B3 | SIN EMPEZAR | 0 · la pasarela hizo la Leyenda de Mr. Satán (B1), no un minijuego de IW |

**Ficha 02: 0,4** (antes 0,1). La parte que tocaba al online está entera; para llegar a 1 faltan B y C (propuestas
2 y 3). Ficha 03: su mínimo es un torneo local sin red (lo lleva el frente de modos/rpg); no pide nada online.

## Lo que queda / propuestas para otros frentes
1. **Coordinador**: en `vision_futura.md` §1, fila 02 → "A HECHO (retardo AUTO + indicador), B y C sin empezar",
   0,4. Si se quiere en la batería, pasar `E:\DBZ3HD_trabajo\online\online_lag.py` a `tools\` (usa `pruebas.Juego`,
   las copias `online_a`/`online_b` y 2 de los 3 juegos; ~6 min sin cola).
2. **Ficha 02-B (`dbz3_relight`)**, dueño del runtime/launcher: primero la prueba de acceso a la profundidad y a la
   máscara de la interfaz en el presentador D3D12 (`rexglue-sdk-0.10/src/ui/d3d12/d3d12_presenter.cpp`, donde ya
   están las capturas), después un SSAO suave tras el fotograma del juego, con la opción `dbz3_relight` apagada
   por defecto en el launcher ("Experimental: cuesta rendimiento"). Precedente de opción en vivo: Brillo HD.
3. **Ficha 02-C (minijuego de IW)**, frente pasarela/campañas: el motor de pasos de `pasarela.cpp` (charla →
   tarjeta → combate → resultado) ya sirve para una misión de IW que sea un combate con condición (p. ej. "gana en
   60 s" con `RequestBattleEx`); es la línea "misiones de IW que son combates (medio)" de `pasarela.md`.
4. **Notas de la versión**: el online pasa al protocolo 4; los dos jugadores necesitan la misma versión (ya se exigía
   el mismo programa). Retardo AUTO por defecto.
5. **`docs/08_online/ONLINE.md`** (sin dueño en esta ronda): añadir un párrafo con lo de "Cómo funciona" de arriba.
6. **rpg/modos y quien escriba `combates.csv` (`tagteam.cpp`)**: en el online el invitado juega en el lado 2, pero
   `combates.csv` y el Elo de `PerfilJugador` toman siempre el lado 1 como "yo". Visto en las pruebas: tras los
   mismos combates online, anfitrión e invitado bajaron los dos de Elo. Propuesta: con
   `dbz3::net::Session::Get().role() == Role::kGuest` al escribir la línea, intercambiar `id1`/`id2` y el ganador.
7. **combate**: `Rules()` ya consulta `Session::CombatRulesAgreed()` (1 línea, aplicada; ver arriba). Si se añade
   otra regla que cambie la simulación, sumarla a `CombatRules` (`netplay.h`, `Now()`/`Apply()`) para que la sala
   la compare.

Lo que queda de este frente: el ritmo de 60 ticks/s está fijo en la fórmula (si algún modo corre a 30, el retardo
saldría el doble de lo necesario; medir el ritmo real si pasa). Snapshot y rollback (F2/F3 de la ficha) siguen como
I+D: no forman parte del mínimo.
