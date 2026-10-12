# Estudio 15: escrituras de X+6048 y estado neutro del luchador

Fecha: 2026-10-10. Solo lectura de `generated\dbz3_recomp.N.cpp` y de `docs\10_tag_team\11_carga_luchador.md`. No se ha editado código ni ejecutado el juego.
Convención: **[C]** = leído en el código recompilado. **[I]** = inferido. **[N]** = no confirmado.

## 1. Todas las escrituras a X+6048

Búsqueda: `stw ...,6048(...)` / `REX_STORE_U32(... + 6048` en todo `generated\`. Hay 4 sitios, en 3 funciones.

| Función | Archivo:línea | Valor escrito | Origen |
|---|---|---|---|
| `sub_82109638` (reset) | `dbz3_recomp.25.cpp:7849` | r30 = 0 (NULL) | `li r30,0` en 25.cpp:7710 [C] |
| `sub_82104C00` (tick de acción 15) | `dbz3_recomp.42.cpp:7563` | 0 (NULL) | Solo si X+3252 ≠ 0: llama `sub_82103978(X,-1)` y luego pone NULL (42.cpp:7497-7563) [C] |
| `sub_8210A648` (siguiente movimiento) | `dbz3_recomp.4.cpp:7927` y `:8068` | r30 = `*(entrada+12)` | Entrada de la cola en X+5464+16·i, con i = X+5980 (4.cpp:7817-7847). La entrada se consume (bit 0 de su flag, 4.cpp:7850) [C] |

Ninguna escritura hace búsqueda por ID. El valor sale de una cola (anillo de 32 entradas de 16 B):
- **Quién llena la cola:** `sub_820F8470` (`dbz3_recomp.28.cpp:6647`). Lee el nodo actual de X+5448, mete el nodo en la entrada `+12` de la cola (28.cpp:6955-6966) y avanza el índice (28.cpp:7041-7048). Avanza siguiendo `*(nodo+8)`, como una lista enlazada [C escritura / I semántica].
- **Inicialización de la cola:** `sub_820F6560` (`dbz3_recomp.27.cpp:6393`) pone X+5448 = `*(CCM+8)` y borra el anillo (27.cpp:6393-6436). Lo llama `sub_820F7868` (`dbz3_recomp.19.cpp:6391`), que lo llama el reset (25.cpp:7856) [C].
- **Corrección a 11_carga_luchador.md:** X+5444 = el valor cargado de X+7908 (`lwz` en 25.cpp:7847 y `stw` en 25.cpp:7852), no la dirección de X+7908 [C].

## 2. Acción neutra

- **Función de acción:** `sub_82103908(X, r4)` (`dbz3_recomp.40.cpp:7311`). Llama a un manejador de la tabla en 0x8201EEF0 indexada por la acción [C], y después `sub_82123EC0` [C]. La tabla no se ha leído [N].
- **Neutro = acción 15** [I]. Pruebas:
  - El tick de la acción 15 (`sub_82104C00`, 42.cpp:7466-7494) cuenta frames en X+3248 mientras X+3244 = 15. Es el comportamiento de un estado parado [I].
  - `sub_8210A648` termina casi siempre con `sub_82103908(X,15)` (4.cpp:8107-8114) [C].
- **Cómo elige sub_8210A648 la acción a partir del nodo** (bits de `nodo+8`) [C lectura, I semántica]:
  - bit 0x4 → acción 50 (4.cpp:7973-7978)
  - bit 0x200 → acción 46 (4.cpp:7982-7988)
  - si no, → acción 15 (4.cpp:8107-8114)
  - Un nodo "parado" tendría `nodo+8 & 0x204 = 0`. Esto es [I].
- **Ticks que leen X+6048:**
  - `sub_8210E100` (acción 50, `dbz3_recomp.29.cpp:7737`) lee `lha 26(X+6048)` **sin comprobar X+3252** (29.cpp:7835-7838). Con X+6048 = NULL se cae [C].
  - `sub_82104C00` (acción 15) y `sub_82107088` (acción 29) lo leen solo si X+3252 = 0 (42.cpp:7502-7505; 25.cpp:7459-7463) [C].

## 3. Llamada propuesta tras `sub_82109638(X,0)` (no probada en juego)

1. `X+6048 = X+5448` (primer nodo CCM, dejado por `sub_820F6560`). Evita el NULL. Es del mismo tipo de bloque [I]. Si ese nodo tiene bit 0x4 o 0x200 en `+8`, no es un estado parado [N].
2. `sub_82103908(X, 15)` (r3 = X, r4 = 15).
3. No tocar X+3244/X+3246 a mano: los escriben los ticks cada frame [C].

Riesgos [N]:
- La cola se rellena cada frame desde X+5448 con `sub_820F8470`, y `sub_8210A648` solo corre si X+8184 ≥ 2 (`sub_821100B0`, `dbz3_recomp.12.cpp:8491-8515`). Por tanto el estado neutro puede pisarse en el siguiente frame. No se ha seguido el orden de llamadas de `sub_821100B0`.
- Qué mapea la tabla de acciones 0x8201EEF0 y si el manejador de la 15 toca X+6048 [N].
- Nada de esto se ha probado en el juego.
