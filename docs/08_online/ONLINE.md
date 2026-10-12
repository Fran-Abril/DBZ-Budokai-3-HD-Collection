# Online (Duelo 1P vs 2P por internet)

Estado 2026-10-11 (noche): funciona entre dos copias del juego en un mismo PC (1v1 sin desincronía; Tag Team 2v2 con relevo en los dos). **Pendiente: prueba entre dos casas por internet.** Antes de publicar, recompilar el exe dual (ver Riesgos).

## Cómo se juega
- En el menú de **Duelo** hay una quinta fila, **ONLINE**, debajo de COM VS COM. Se llega con el cursor del juego y A abre la pantalla **DUELO ONLINE** (ES/EN/IT/DE/FR). SELECT no sirve: en todos los menús abre la ayuda del juego.
- **Anfitrión:** pulsa **Crear partida**. El juego abre solo el puerto en el router (UPnP de Windows) y muestra un **código de sala** de 10 letras (`XXXXX-XXXXX`). El código se copia al portapapeles.
- **Invitado:** pega el código en el campo de unirse (también vale la IP escrita a mano) y pulsa **Unirse**.
- El panel comprueba la versión del juego, la región (US/EU) y los mods. Las texturas HD no cuentan.
- El anfitrión pulsa **Empezar**. Desde ese momento los dos juegos van a la par: se elige **1P vs 2P** y el anfitrión es el jugador 1.
- Mientras está online, el invitado no puede navegar menús que no sean el Duelo: sus pulsaciones van al jugador 2 (`netplay_hooks.cpp:120-125`).
- **Salir:** X en el Duelo. El otro PC ve "el otro jugador se ha ido".

## Código de sala y arranque alineado
- **Código de sala:** la IP pública y el puerto del anfitrión en 10 caracteres base32 de Crockford (sin I, L, O ni U). Sin nombres de mods ni datos de la partida dentro. Se decodifica con o sin guion (`src/net/room.cpp`).
- **Sin UPnP:** el juego pregunta su IP pública a un servidor STUN (`stun.l.google.com:19302`) desde el mismo socket del juego. En ese caso el puerto 7777 UDP hay que abrirlo a mano en el router.
- **Al cerrar** (aunque sea sin pasar por Close) el juego quita el puerto del router (`UpnpCloseAtExit`).
- **Arranque alineado:** cada PC espera a que el otro llegue al inicio del combate (bits de listo 0x0400 anfitrión y 0x0800 invitado en el mando). Entonces los dos reinician el generador de números aleatorios en el mismo tick. Esto arregló una desincronía real medida medio segundo después de empezar.
- **Desincronía:** cada 60 ticks se compara el RNG. Si no coincide, sale un aviso rojo "DESINCRONIA" en pantalla. La partida sigue (no se corta).

## Cómo funciona (`src/net/`)
- **Lockstep con retardo fijo**:
  - Un tick es una iteración del bucle principal. El sondeo de mandos `sub_820902F8` (EU `820902C0`) se llama una vez por iteración.
  - Cada PC manda su mando del tick T para aplicarlo en T+D y espera el del rival antes de seguir. Si un paquete se pierde, se reenvía en el siguiente, porque cada uno lleva hasta 8 ticks.
- **Mandos**: gancho del thunk de `XamInputGetState`, `sub_8221C178` (EU `8221A940`). En los dos PCs, el usuario 0 es el anfitrión y el usuario 1 el invitado, que siempre cuenta como "conectado".
- **Mismo estado al empezar**:
  - El anfitrión manda una semilla para el LFSR del juego (US `0x82310670`).
  - También manda la foto de su estado de menú y desbloqueos, `0x824BA110` + 0x100 (EU `0x824BA100`), con la rueda, la última elección y los desbloqueos.
  - El invitado copia las dos cosas en el mismo tick 0.
  - Cada combate reinicia el RNG igual en los dos PCs (`ReseedBattle`).
- **Tag Team online**: cada PC manda su equipo al empezar (`kTeam`). El anfitrión es el lado 1 y el invitado el lado 2. El relevo termina en el mismo tick en los dos PCs (ver `docs/10_tag_team/TAG_TEAM.md`).
- **Huella de mods**: XXH3 de cada archivo de cada mod activo. No entran `_roster`, que se genera igual en los dos PCs, ni los packs de texturas.
- **Protocolo UDP** (versión 2): HELLO (build, región, mods, retardo), HELLO_ACK, lista de mods si difieren, START (semilla + foto) / START_ACK, INPUT, TEAM, BYE.

## Mapas y evidencias
`01_online.md` (estudio), `01b_online_f0.md` (F0), `12_online_mapa.md` (dónde tocar), `13_duelo_online.md` (pantallas del Duelo y bloque de configuración `0x8245EFC0`).

## Pendiente
- **Prueba entre dos casas por internet** (el anfitrión abre UDP 7777 o el router lo abre con UPnP). Sin probar todavía.
- **Partida guardada:** el anfitrión la manda por trozos, pero **no se aplica** en el invitado. Hoy los dos juegos pueden jugar con desbloqueos o personajes distintos. Decisión pendiente: aplicarla en memoria o reiniciar el juego del invitado.
- **Mods:** hoy solo se avisa de las diferencias. Sincronizarlos desde el anfitrión es una decisión de producto.
- **Código de seguridad:** el primer PC que escribe el puerto captura la sala. Falta un código de sesión que el invitado tenga que escribir.
- **EU:** el RNG de la versión EU no está mapeado (no hay semilla ni comprobación). Online EU sin garantía de sincronía.
- **Relevo online:** la carga de ficheros es asíncrona y no hay tiempo máximo online: un PC que nunca carga bloquea la partida.

## Riesgos
- **Exe dual desactualizado:** `out/build/win-amd64-dual/dbz3.exe` es de 2026-10-10, anterior a todo el código online. Hay que recompilarlo antes de cualquier release. El exe de release sí es de 2026-10-11 (23:26).
- **Versión por fecha de compilación:** la comprobación de versión usa la fecha y hora de compilación (`__DATE__ __TIME__`). Dos compilaciones distintas del mismo código dan "otra versión del juego".
- **Lectores de tiempo del invitado:** hay siete lectores de reloj en el juego sin verificar si alimentan la lógica del combate. Puede aparecer desincronía en máquinas lentas que no se ve a 143 Hz.
- **Frecuencia del bucle:** las esperas (90 ticks de relevo, 60 ticks de comprobación) suponen 60 Hz. Sin medir.
- **Scripts de prueba:** `online_e2e.ps1` y `online_e2e_code.ps1` están en el scratchpad de la sesión, no en `tools/`. Hay que moverlos a `tools/` para que no se pierdan.
- **Ayuda de reenvío de puerto:** el panel no muestra la IP pública ni guía para abrir el puerto. Pendiente de decidir (solo LAN/VPN o UPnP con ayuda visible).
