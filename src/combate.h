// dbz3 - Combate (src/combate.cpp): lo que pueden usar otras partes del juego.
#pragma once

namespace dbz3::combate {

// Muñeco del laboratorio (el rival del jugador no decide, no contraataca y se queda quieto o en guardia) sin tocar
// el ajuste guardado dbz3_combate_prueba_muneco: 0 = no hay muñeco, 1 = quieto, 2 = en guardia, -1 = lo que diga el
// ajuste. Cualquier hilo; vale desde la siguiente pelea (o ya, si hay una en marcha).
void SetMuneco(int modo);

}  // namespace dbz3::combate
