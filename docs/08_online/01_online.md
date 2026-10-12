# Estudio: online peer-to-peer sin servidores propios (DBZ Budokai 3 HD)

Solo lectura del proyecto. Ningún archivo del proyecto fue modificado. Código de referencia descargado a `scratchpad/estudios/ref/` solo para estudio (no copiar: el repo principal no tiene licencia, GitHub devuelve 404 en `/license`).

## 1. Resumen

- **Recomendación: sincronía por retardo de entrada (delay-based), no rollback.** Cada PC envía el pad de cada frame al otro; el juego espera unos pocos frames (2-8) de margen. Es lo que hace la referencia y encaja con nuestro juego (estado de guest grande, sin savestate barato).
- **Transporte:** UDP directo entre los dos PCs. Para NAT: primero IP + puerto manual (reenvío de puerto); después ICE con STUN público (libjuice, MPL-2.0) y código de sala que transporta los candidatos (sin servidor de salas). Sin TURN propio: algunas redes (NAT simétrico, CGNAT) no conectarán.
- **Punto bueno:** la referencia demuestra que no hace falta servidor de salas para jugar en directo; su modo "LAN/VPN directo" ya funciona sin lobby.
- **Punto clave que no esperábamos:** el juego original ya tiene su propio netcode (sesiones XSession y sockets). La referencia engancha ahí; eso reduce el trabajo, pero hay que confirmarlo en nuestra build.
- **Nuestro repo no tiene código de red** (`src/` no contiene ningún uso de sockets, XNet ni online). Solo existe la capa de sockets y sesiones del SDK.

## 2. Opciones técnicas

| Enfoque | Qué es | Pros | Contras | Veredicto |
|---|---|---|---|---|
| Delay-based (lockstep con retardo) | Cada frame se envía el input; si el remoto no llegó, se espera hasta D frames | Simple; no necesita savestate; determinismo solo del input | Latencia fija de D frames (4 frames = 67 ms) | **Elegido para MVP** |
| Rollback (GGPO-like) | Predice input remoto, rebobina y re-simula al llegar el real | Sin lag percibido con ping medio | Hace falta snapshot/restore del estado completo del guest y determinismo total (fallo = desync); coste de copia por frame | **Descartado por ahora** (ver §3) |
| Lockstep puro | Espera siempre al remoto | Trivial | Ping alto = juego a tirones | Solo como base del delay-based |
| Relay propio | Servidor que reenvía | Siempre conecta | Viola "sin servidor propio" | Descartado |

### NAT y conexión (sin servidor propio)
- **IP + puerto manual con reenvío de puerto (UDP):** 0 dependencias, funciona con IP pública o VPN (Radmin/Hamachi). Lo más robusto de los tres. Puerto de referencia: 59395 (ver `README` de la referencia).
- **UPnP / NAT-PMP:** abrir el puerto automáticamente en el router. Cómodo, pero muchos routers lo tienen desactivado y algunos ISP usan CGNAT (no hay IP pública que mapear).
- **STUN + ICE (hole punching):** descubrir la IP pública (STUN público, p. ej. `stun.l.google.com:19302`) y perforar NAT. Necesita intercambiar candidatos: sin servidor, el "código de sala" es ese paquete de candidatos copiado a mano (unas 200-400 caracteres en base32/base64). Éxito esperable: NAT cónicos sí; simétricos o CGNAT no (sin TURN).
- **IPv6:** si ambos tienen IPv6 global, conexión directa sin NAT (muy fiable donde esté disponible). Requiere firewall abierto.
- **Código de sala = IP:puerto codificado:** equivalente a IP manual, pero más cómodo de copiar. Es el mismo caso que el anterior.

### Librerías (licencias a verificar antes de usar)
- **libjuice** (ICE/STUN/TURN en C): **MPL-2.0** (la referencia la usa sin modificar, estática). Uso compatible si conservamos su licencia y los cambios a sus archivos. Verificado: el árbol de la referencia trae `thirdparty/libjuice/LICENSE`.
- **GameNetworkingSockets** (Valve): **BSD-3-Clause** (verificar). Trae cifrado y fiabilidad, pero su ICE exige servidor de señalización: para nosotros sería solo transporte con IP directa.
- **ENet**: **MIT** (verificar). UDP fiable/no fiable; sin NAT traversal. Útil si queremos fiabilidad sin escribirla.
- **GGPO**: **MIT** (verificar). Biblioteca de rollback; nos obligaría a guardar estado del juego: no aplica sin savestate.
- **yojimbo**: licencia BSD (verificar). Similar a ENet con más capas; no aporta nada que ENet no dé aquí.
- Recomendación: **sockets UDP propios (Winsock) para el MVP** (cero dependencias, ~300 líneas), **libjuice solo en F2**.

## 3. Lo que necesita el juego

### 3.1 Determinismo
- **Reloj lógico fijo**: el juego debe avanzar 1 frame lógico por tick de 60 Hz, sin depender del FPS real. Nuestro límite de FPS variable (`frame_cap`, vblank) afecta a la presentación, no a la lógica; hay que confirmar que la simulación cuelga del contador de frames y no del tiempo real. Pendiente de verificar en el guest (no hay código propio de esto en `src/`).
- **RNG**: no encontrado en `src/` ni en la SDK. Hay que localizar en el guest la semilla (¿tiempo? ¿contador?). Si usa tiempo del sistema, hay que forzar la misma semilla en ambos PCs (la referencia la sincroniza como cvar del host). **Riesgo alto si no se verifica.**
- **Punto flotante:** misma build y mismo binario en ambos PCs; el riesgo es bajo, pero hay que comprobarlo con el desync check.

### 3.2 Dónde inyectar el input remoto
- Entrada del juego: `XamInputGetState_entry` (`rexglue-sdk-0.10/src/kernel/xam/xam_input.cpp:95-124`) -> `InputSystem::GetState` (`rexglue-sdk-0.10/src/input/input_system.cpp:232`) -> `GetStateForUI` (`input_system.cpp:308-340`), que fusiona los dispositivos asignados a cada usuario con `MergeInto` (`rexglue-sdk-0.10/src/input/state_merge.cpp:31`: OR de botones y máximo de ejes/gatillos).
- **Punto de inyección recomendado:** en `InputSystem::GetState` (`input_system.cpp:232`), para el índice de usuario del jugador remoto, sustituir el estado por el input recibido del otro PC para ese frame. Así el juego no cambia y el input local no se toca.
- Existe `dbz3_input.req` (`rexglue-sdk-0.10/src/input/mnk/mnk_input_driver.cpp:150-215`), pero es **solo para pruebas automáticas** (teclado por archivo). No sirve para red.
- Las dos fuentes de datos de la referencia: input por frame por el canal lateral del transporte (`kind 16`) y escritura directa en los anillos de input del juego (`burstlimit_netinput.cpp:3-15`). Nosotros podemos hacerlo igual en `GetState`, sin tocar memoria del guest (más limpio).

### 3.3 Lo que el juego ya trae (no hay que inventarlo)
> **Corregido 2026-10-11:** la frase "el juego ya hace netplay por sus sockets" es **falsa** (venía de la referencia, Burst Limit). El juego no tiene netplay propio. Ver `01b_online_f0.md` §0.1. Lo de abajo queda como historial.

- El juego original tiene sesiones y sockets: `XSessionSearchEx` (`rexglue-sdk-0.10/src/kernel/xam/apps/xgi_app.cpp:213`) y la capa `xam_net.cpp` / `xsocket.cpp` del SDK.
- La referencia dice que el juego envía sus propios paquetes de input en bloques de 3 frames (`burstlimit_netinput.cpp:3-8`) y que los recibe por su ruta de datos (`sub_8217E420`). Eso significa: **el juego ya hace netplay por sus sockets**; el trabajo es el transporte (NAT, puertos) y la latencia.
- Pendiente de comprobar en nuestra build: si `XSessionCreate`/`XSessionJoin` y los sockets funcionan con dos instancias en LAN. Es la prueba más barata (ver F0).

### 3.4 Snapshot para rollback
- Un Xbox 360 tiene 512 MB de RAM (dato de hardware, no verificado en código). Copiar eso 60 veces por segundo es inviable sin trucos (deltas, compresión). Además hay estado de GPU/audio que no se guarda en RAM del guest. **Por eso no recomiendo rollback en esta fase.**

### 3.5 Desync check
- La referencia compara cada frame un hash de: el reloj de batalla, posición, acción y ki de ambos luchadores (`burstlimit_netinput.cpp:18-20`). Es barato y localiza el primer frame divergente. Nosotros podemos empezar igual y, si hace falta, ampliar a regiones de RAM del guest (por dirección conocida).

## 4. Referencia: DBZ Burst Limit Recompiled (v4.0, commit `706252d8`)

- **Lo que hace:** lobby online con salas (`online_lobby_url`, `wss://.../v1/ws`) que entrega credenciales de señalización y TURN. Después, conexión directa P2P por ICE (libjuice 1.7.4, MPL-2.0, estática) con relé TURN como respaldo. Los datagramas del juego viajan con direcciones virtuales `198.18.0.0/16` (`burstlimit_online.cpp`, `peer_transport.h`).
- **Input:** por frame, 60 Hz, con redundancia por el canal lateral; retardo exacto `online_input_delay` 2-8 frames (lo decide el host); "hold freeze" opcional para congelar efectos durante esperas; salas con versión bloqueada.
- **Modo sin lobby:** si la URL del lobby está vacía, usa el camino LAN/VPN antiguo (Radmin) sin cambios.
- **Lo que no copiamos:** el lobby con servidor (incumple nuestro requisito), y el código (sin licencia). Lo aprovechable: la idea del retardo, el canal lateral y el desync check.
- Commit de la SDK fork (`iExplosiveRage/rexglue-sdk` `4b60adc1`): licencia detectada como `NOASSERTION`; tratar igual que la anterior (solo estudio).

## 5. Plan por fases

### F0. Verificación previa (1-2 días)
- Confirmar que la simulación del juego es por frame y no por tiempo (buscar en el guest el reloj de batalla y la semilla RNG).
- Probar en LAN con dos instancias: sesión con `XSessionCreate` / `XSessionSearchEx` y paso de paquetes. Si ya funciona, el coste baja mucho.
- Si F0 falla (semilla por tiempo, RNG no controlable): el delay-based sigue siendo posible solo si el input basta; si no, hay que parchear la semilla en el guest.

### F1. MVP: IP directa, delay-based, Duelo (2-3 semanas, estimación)
- Socket UDP propio: envío de input de cada frame con redundancia (últimos N frames) y ACK.
- Inyección en `InputSystem::GetState` para el jugador remoto; cola de input con retardo D (configurable, igual en ambos).
- Espera ("hold") si falta input remoto; contador de esperas en el log.
- Desync check con hash por frame; aviso visible si diverge.
- Handshake con versión de build (mismo binario).
- Reenvío de puerto manual (documentación para el usuario). Modo Duelo solamente.
- Prueba: 2 PCs (o 1 PC con 2 instancias y latencia simulada) y prueba de retardo con el `simulated_network` ya usado en la referencia.
- Riesgos: cortes de Windows Defender/firewall (regla UDP); bloqueos de input por frame si el paquete se pierde (mitigar con redundancia).

### F2. Código de sala + STUN (2-4 semanas)
- Integrar libjuice (MPL-2.0; conservar aviso; estática o dinámica).
- "Código de sala" = candidatos ICE codificados; el usuario lo copia y pega entre los dos PCs (sin servidor).
- STUN público solamente. Sin TURN: mostrar al usuario un aviso si el NAT no permite conexión.
- Riesgo: el porcentaje de conexiones exitosas depende de los NAT de los usuarios; es imposible garantizarlo sin relé.

### F3. Extras opcionales (1 semana)
- UPnP / NAT-PMP para abrir el puerto automáticamente (biblioteca BSD/MIT a verificar).
- IPv6 directo si ambos lo tienen.
- Retardo variable negociado (el host decide, como la referencia).

### F4. Rollback (solo si F1-F3 no bastan) (2-4 meses, alto riesgo)
- Snapshot delta de la RAM del guest + estado de audio/GPU, determinismo estricto y prueba de desync en bucle. No recomendado ahora.

## 6. Riesgos y decisiones abiertas
- **Latencia:** con 4 frames de retardo más el ping, por encima de ~100 ms de ping el juego se nota; sin rollback no hay mejor opción.
- **CGNAT y NAT simétrico:** sin TURN, esos usuarios no podrán conectar (salvo IPv6 o VPN). Hay que avisarlo en la documentación.
- **Determinismo (3.1):** es el riesgo principal. Si la semilla o el reloj dependen del tiempo, el delay-based sigue viable pero el desync check saltará.
- **Licencias:** no copiar código de la referencia; usar solo libjuice/ENet con su licencia y verificar cada una antes de incluirla.
- **Decisión para el usuario:** ¿Duelo solamente en el MVP? (recomendado) ¿Retardo fijo o configurable? ¿Aceptamos que algunas redes no conecten sin servidor de relé?

## 7. Archivos de referencia en nuestro repo
- `rexglue-sdk-0.10/src/input/input_system.cpp:232-340` (GetState / GetStateForUI).
- `rexglue-sdk-0.10/src/input/state_merge.cpp:31` (MergeInto).
- `rexglue-sdk-0.10/src/kernel/xam/xam_input.cpp:95-124` (XamInputGetState).
- `rexglue-sdk-0.10/src/input/mnk/mnk_input_driver.cpp:150-215` (inyección de prueba `dbz3_input.req`).
- `rexglue-sdk-0.10/src/kernel/xam/apps/xgi_app.cpp:213` (XSessionSearchEx).
- `rexglue-sdk-0.10/src/kernel/xam/xam_net.cpp`, `rexglue-sdk-0.10/src/system/xsocket.cpp` (sockets XAM).
- `docs/HOJA_DE_RUTA_COMUNIDAD.md:147-149` (online "No iniciado", anota netplay tipo rollback como muy complejo).

Referencia descargada (solo estudio): `scratchpad/estudios/ref/burstlimit_netinput.cpp`, `burstlimit_online.cpp`, `peer_transport.h`, `online.h`.

## 8. Lo no verificado
- Semilla RNG y reloj de batalla del guest: no localizados todavía.
- Que el netcode del juego (XSession + sockets) funcione hoy en nuestra build: no probado.
- Licencias GameNetworkingSockets, ENet, yojimbo, GGPO: indicadas de memoria, verificar en sus repositorios antes de usar.
- Tamaño de RAM del guest (512 MB): dato de hardware, no comprobado en el SDK.
