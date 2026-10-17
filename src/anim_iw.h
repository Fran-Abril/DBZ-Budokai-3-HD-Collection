// dbz3 - Animaciones de Infinite World en el motor de B3 (frente R10, 2026-10-16): el "scatto" de los minijuegos de
// campo de IW. Minigame_State14_DashStartEnter / State15_DashEnter (decomp EU 0x2A0CD0) / State16_Enter ponen los
// codigos 0x19 (arranque, 12 cuadros) -> 0x1A (bucle, 60) -> 0x1B (frenada, 22); en el aire 0x1C/0x1D/0x1E. El #BSK
// comun de IW (DATA_CMN 204 hijo 3) los lleva a las animaciones 257..267 de su almacen global (204 hijo 0: los mismos
// 46 huesos GOK_* y en el mismo orden que el global de B3, 165 hijo 0). B3 no las tiene: se leen en marcha de la copia
// del jugador y se anaden detras de las 359 del almacen global de B3 (sin tocar ninguna) con un #CSK propio de 6
// codigos en un hueco libre de los 3 #CSK del luchador. Andar sigue siendo el de B3 (el andar de IW es el mismo).
// (El estado 9 / codigo 0xE2 es el corredor de la IA, Bubbles o Piccolo: solo el #BSK de Bubbles, 912, lo define.)
#pragma once

#include <cstdint>

namespace dbz3::anim {

enum Fase : int { kApagado = 0, kInicio = 1, kBucle = 2, kFin = 3 };

// Pide la fase del scatto para el luchador x (direccion del luchador, p. ej. mgr + 2096). Cualquier hilo; se aplica
// en el siguiente cuadro en el hilo del juego (gancho propio al leer el mando). kInicio pasa sola a kBucle al acabar
// su animacion; kFin pasa sola a kApagado, que deja al luchador en guardia (accion 0) y al juego andar como siempre.
// Repetir la misma fase no hace nada. aire: las variantes 0x1C-0x1E.
void Scatto(uint32_t x, Fase f, bool aire = false);
// Fase en curso (la que se ve).
Fase FaseScatto();

// Corredor de la IA de IW (codigo 0xE2, estado 9 de MgPly) en bucle para cualquier luchador (p. ej. Piccolo, el
// segundo luchador, mgr + 2096 + 8240), aparte del scatto. IW solo define 0xE2 para Bubbles (DATA_CMN 912, esqueleto
// de mono): para humanos se usa el unico ciclo de carrera humano de sus datos, el de Piccolo (DATA_CMN 925, codigo 4,
// 32 cuadros). false: a la guardia.
void Corredor(uint32_t x, bool on);
// Andar libre de los campos de IW (auditoria6): una animacion de IW en bucle, como el corredor. kQuieto = el reposo de
// pie de IW (#BSK comun codigo 0x00, Char_EnterPoseState0), kCorrer = la carrera del #BSK comun (codigo 0x16, la del
// estado 3 de IW, +0xB4 = 4.0; elegida entre 0x07/0x08/0x15/0x16/0xE2 viendolas en el juego). Con la misma no hace nada.
// El reposo es el 0x00 comun (Char_EnterPoseState0: la pose de espera de IW, 61 cuadros). DATA_CMN 914 es Cell
// semiperfecto (huesos CEL_*), no un humano: sus animaciones solo valen para Cell (minijuego de Ten Shin Han).
constexpr uint32_t kQuieto = 0x800, kCorrer = 0x816;
// Andar (palanca a menos de 0.9, IW estado 2: codigo 0x15 del #BSK comun). Salto, del #BSK comun: subir con el despegue
// 0x29 (30 cuadros, rodillas recogidas) y caer con 0x104 (en el aire). Vistas y descartadas: 0x22 (picado horizontal,
// captura 165931) y 0x103/0x105 (voltereta con las piernas por delante, 170127 y 171530, tambien con la raiz girada).
constexpr uint32_t kAndar = 0x815, kCaer = 0xA04, kSubir = 0x829;
void Bucle(uint32_t x, uint32_t codigo);
// Cansado al vaciarse la barra del scatto: en IW State16_Update (EU 0x2A04A0) con la barra a 0 pasa a
// Char_EnterState7Transform: codigo 0x05 y luego 0x06 (Char_UpdateAnimToggle), las animaciones 4/5 del almacen
// global, iguales en B3 (no hace falta portarlas). true: 0x05 y despues 0x06 en bucle; false: a la guardia.
void Cansado(uint32_t x, bool on);
// R16: animacion de IW suelta (entrada 0x11D, comienzo y final de los minijuegos de campo; ver kExtras en anim_iw.cpp)
// una vez; al acabar, a la guardia, o sujeta en su ultimo cuadro (sujetar). codigo 0: quitarla.
// tope > 0: se queda quieta en ese cuadro (una pose de la animacion, p. ej. el salto con las rodillas recogidas)
void Reproducir(uint32_t x, uint32_t codigo, bool sujetar = false, uint32_t tope = 0, uint32_t luego = 0);
// luego != 0: al acabar la pose pasa sola a ese codigo en bucle (el reposo de IW; los minijuegos de botones).
// 0 nada pedido, 1 en curso, 2 acabada (o sin esa animacion en la copia del jugador).
int EstadoPose();
// R14 (minijuegos de botones NB006/NB013/NB014): las poses de IW de cada personaje salen de su #BSK de DATA_CMN
// (cmn = 922 Goku, 920 Gohan joven, 931 Vegeta, 928 Trunks, 921 Gohan nino, 925 Piccolo; 0x57A-0x57D, las de
// espera, de 915: se piden con cmn 915). El mismo codigo se repite entre personajes, asi que van con un alias propio
// (0xB00...): este lo da. 0 = esa pose no esta en la lista de kQte (anim_iw.cpp) o no la trae la copia del jugador.
uint32_t CodigoQte(uint32_t cmn, uint32_t codigo);
// iw_fix: altura (y de la cadera, unidades de IW) de la animacion de IW que lleva puesta el luchador x en este cuadro, para que
// la camara del final siga el salto de superado (0x52: hasta 69); -1 si no lleva ninguna (o va mezclando). Hilo del juego.
float AlturaRaiz(uint8_t* base, uint32_t x);
// Lo mismo con la posicion entera de la cadera (x, y, z; el salto de superado de IW va 50 u hacia delante, z): false si no hay.
bool RaizActual(uint8_t* base, uint32_t x, float out[3]);

}  // namespace dbz3::anim
