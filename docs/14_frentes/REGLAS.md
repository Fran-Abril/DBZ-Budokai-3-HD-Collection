# Reglas comunes de los frentes (ronda 2026-10-15, objetivo de 12 puntos)

Léelo entero antes de empezar. El coordinador integra; tú trabajas solo en tu frente.

## Usuario y entrega
- El usuario no es técnico: tu informe final, en español, breve, sin jerga. Nada de "listo" sin build + prueba
  en el juego (capturas). Si algo falla, dilo.
- Todo debe sentirse **nativo**: letra, marcos, colores, sonidos y fundidos del juego. Mide contra capturas
  del juego, no inventes estilos.
- **Nunca sustituir ni ocultar contenido del juego**: solo añadir o ampliar (Dragon Arena y todas las filas
  originales siguen visibles y accesibles).
- Nunca borrar: mueve a `mods_respaldo/` o `_respaldo/`. Nada de push, commit ni publicar (lo hace el
  coordinador al final). Nunca escribas tokens/claves en ficheros.
- Arte del juego: solo extraído en tiempo de ejecución del AFS del jugador (`src/ingame/native_ui.cpp`), nunca
  copiado al repo/release. Arte de terceros o de personajes con copyright (p. ej. `assets_propios/png/add/*flying*`)
  solo en **mods opcionales descargables** (`mod center hd/packs_visuales/`), nunca en el exe/Kit/release.
  El logo `assets_propios/png/add/Logo Dragon HUB.png` es del usuario: se puede usar en la release.
- `docs/07_ports` no va al repo público. Créditos de packs de texturas: @ArielPVB / SSGPrinceVegeta.

## Build y pruebas
- Compilar SOLO con `python tools/compilar.py` (candado compartido; copia las DLL canónicas).
- Probar con `python tools/pruebas.py` (máx. 3 juegos a la vez entre todos; el script respeta el tope).
  Usa tu propia copia `D:\DBZ3HD\<frente>_test` (`pruebas.py prep <copia>`; copia también
  `res/ingame_ui/*.png` a `<copia>\ingame_ui\` si los usas). Nunca la partida real del usuario.
- Para esperar menús usa señales del log o de pantalla (byte 0x824BA110), no tiempos a ciegas. Con
  `DBZ3_TEST_RINGOUT=1` el log dice `tag: prueba: estado del combate N` (5 acabado, 6 pose, 7 menú final).
- Antes de añadir `REX_HOOK_RAW(sub_X)` busca con Grep `sub_X` en `src/`: dos ganchos del mismo símbolo
  rompen el enlace (ya pasó con `sub_8218BBC0`). Si ya está enganchado, propón el cambio a su dueño.
- Toca solo tus ficheros. Para ficheros de otro frente, escribe la propuesta (diff corto) en tu
  `docs/14_frentes/<frente>.md` y avisa en el informe. `CMakeLists.txt`: solo añadir tus .cpp.

## Disco (C y D casi llenos)
- Temporales, renders, extracciones, ISOs desempaquetadas: **`E:\DBZ3HD_trabajo\<frente>\`**. Nunca en C.
- No dejes copias pesadas en D (packs HD, ISOs). Lo que sobre, muévelo a `D:\_PARA_BORRAR_CLAUDE\`.

## Informe final
- Qué ve el jugador ahora (antes → después), cómo se probó (carpeta de capturas), lo que queda y
  propuestas para otros frentes. Actualiza tu `docs/14_frentes/<frente>.md`.
