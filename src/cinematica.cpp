// dbz3 - Cinematicas en el motor de B3 (frente R3). Ver cinematica.h.
//
// Piezas del juego que usa (medidas por otros frentes):
//  - camara: cine::Camara (mgr+0x60 vista, +0x190 ojo, +0x1A0 objetivo; se pisa al empezar el dibujo, cine.cpp);
//  - luchadores: X = mgr + 2096 + 8240 * lado, posicion X+48, mando X+5028 (16 bits, -1 = COM; cine.cpp deja al COM en
//    el mando 0 mientras dura su intro y sin pulsar no hace nada);
//  - mando: registro 184 B en 0x8238B780 + 184 * usuario: +12 pulsados, +14 recien pulsados, +16 soltados, +24
//    palancas, +72 + 4j cuadros pulsado del boton j (bit 0x100 << j), +108..+114 direcciones (combate.cpp);
//  - pose: Y+B = 0xA000 en +12, la carga de ki del propio personaje (tagteam.cpp, relevo).
#include "cinematica.h"

#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/immediate_drawer.h>

#include <imgui.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>

#include "generated/dbz3_init.h"
#include "cine.h"
#include "combate.h"
#include "eu_map.h"
#include "gigantes.h"
#include "ingame/game_style.h"
#include "launcher/i18n.h"
#include "launcher/launcher_state.h"

namespace dbz3::net {
void InyectarPad(uint32_t usuario, int32_t botones);   // net/netplay_hooks.cpp
}

// Parejas EU medidas con el metodo de tools/eu_map.py (E:/DBZ3HD_trabajo/P3/eu_par.py, sin reescribir el mapa):
// sub_82114FB8 -> 821150B8 (26 instr., 0 diferencias sin explicar, 67 llamadas bien) y sub_82103978 -> 82103A78
// (46 instr., 0 sin explicar, 63 llamadas bien).
#if defined(DBZ3_DUAL_REGION)
REX_EXTERN(dbz3eu_sub_821150B8);
REX_EXTERN(dbz3eu_sub_82103A78);
#endif
#define DBZ3_ARRANCA_ANIM DBZ3_CALL2(sub_82114FB8, dbz3eu_sub_821150B8)
#define DBZ3_FIN_ACCION DBZ3_CALL2(sub_82103978, dbz3eu_sub_82103A78)

namespace dbz3::cinematica {
namespace {

namespace S = dbz3::ingame::style;
constexpr dbz3::GA kMgrPtr = 0x823D6CF4;
constexpr uint32_t kX0 = 2096, kXPaso = 8240, kPos = 48, kMandoIdx = 5028;
constexpr uint32_t kPasado = 35144;
constexpr uint16_t kAccionPose = 15;   // accion de las poses (entrada 0x280/0x281, victoria 0x2A0; sonda)
constexpr uint32_t kCodigo = 2928;   // codigo de la animacion en curso (sub_82103C48 lo pone y sub_82114FB8 la arranca)   // cuadros de pelea pasados (float; el reloj del combate, pasarela.cpp)
constexpr float kFundido = 0.6f;   // s de paso de un plano a otro (y a la camara del combate al acabar)

std::mutex g_mu;
std::shared_ptr<const Escena> g_pedida, g_esc;   // pedida (la siguiente pelea) / en curso
std::atomic<bool> g_en_curso{false}, g_saltar{false}, g_conectada{false};
std::atomic<bool> g_siguiente{false};   // A: el paso siguiente sin esperar (START salta la escena)
std::atomic<int> g_paso{-1};
std::atomic<float> g_abre{0};   // 0..1 de las bandas (panel)
uint32_t g_frame = 0;
int64_t g_t0 = 0, g_t0_paso = 0;   // ms (reloj de pared: las voces van en tiempo real aunque el juego baje de 60)
int64_t AhoraMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
int16_t g_mando[2] = {-1, -1};   // X+5028 de cada lado al empezar (-1 = COM)
bool g_cambiado[2] = {};
int g_usuario[2] = {-1, -1};      // mando (usuario de XInput) virtual de cada lado mientras dura
uint32_t g_codigo0[2] = {};       // X+2928 de cada lado al empezar (la guardia a la que se vuelve)
bool g_puesto[2] = {};            // Tick puso una animacion a ese lado
uint32_t g_pasado = 0;  // mgr+35144 al empezar: el reloj se queda parado mientras dura la escena
struct V3 {
  float x, y, z;
};
V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 operator*(V3 a, float k) { return {a.x * k, a.y * k, a.z * k}; }
V3 Mezcla(V3 a, V3 b, float t) { return a + (b - a) * t; }
V3 Unit(V3 a) {
  const float l = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
  return l > 1e-6f ? a * (1.0f / l) : V3{0, 0, -1};
}
float Suave(float t) {
  t = std::clamp(t, 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}
// Valor de una pista de camara {cuadro, v...} en el cuadro f (lineal, como el juego; 1 clave por cuadro).
template <size_t N>
bool MuestraCam(const std::vector<std::array<float, N>>& k, float f, float* out) {
  if (k.empty()) return false;
  size_t i = 0;
  while (i + 1 < k.size() && k[i + 1][0] <= f) ++i;
  const size_t j = std::min(i + 1, k.size() - 1);
  const float d = k[j][0] - k[i][0], t = d > 0 ? std::clamp((f - k[i][0]) / d, 0.0f, 1.0f) : 0.0f;
  for (size_t c = 1; c < N; ++c) out[c - 1] = k[i][c] + (k[j][c] - k[i][c]) * t;
  return true;
}
void PonV(uint8_t* base, uint32_t x, V3 p) {   // la posicion va tres veces (X+48/+64/+80; tagteam.cpp StepSlide)
  for (uint32_t o : {48u, 64u, 80u}) {
    const float f[3] = {p.x, p.y, p.z};
    for (uint32_t c = 0; c < 3; ++c) {
      uint32_t u;
      std::memcpy(&u, &f[c], 4);
      REX_STORE_U32(x + o + 4 * c, u);
    }
  }
}
V3 LeeV(uint8_t* base, uint32_t a) {
  float f[3];
  for (uint32_t c = 0; c < 3; ++c) {
    const uint32_t u = REX_LOAD_U32(a + 4 * c);
    std::memcpy(&f[c], &u, 4);
  }
  return {f[0], f[1], f[2]};
}

// Marcador del combate (barras, caras, reloj): lo esconde el propio juego (combate.h, frente P4).
void OcultarHud(bool ocultar) { dbz3::combate::OcultarHud(ocultar); }

// Plano p con los luchadores en p1 y p2 (unidades de B3; un luchador mide ~12).
// s1, s2: escala de dibujo de cada lado (gigantes.cpp, 1 = normal). El gigante se agranda DESDE LOS PIES (fila 3 de
// su matriz intacta, escenarios.md R2): con el ojo y el objetivo a s veces su distancia de los pies y el mismo fov, la
// imagen es la de un luchador normal (triangulos semejantes): sale entero sin estimar nada (studies 8 fase 2).
void Encuadre(int p, V3 p1, V3 p2, V3* ojo, V3* obj, int pose = 0, float s1 = 1.0f, float s2 = 1.0f) {
  const V3 c = Mezcla(p1, p2, 0.5f), eje = Unit(V3{p2.x - p1.x, 0, p2.z - p1.z});
  const V3 lado{-eje.z, 0, eje.x};   // perpendicular horizontal
  if (p == kGeneral) {
    const float s = std::max(s1, s2);   // general: los dos; el mas alto manda
    *ojo = c + lado * (48.0f * s) + V3{0, 14.0f * s, 0};
    *obj = c + V3{0, 8.0f * s, 0};
    return;
  }
  const V3 yo = p == kLado1 ? p1 : p2, frente = p == kLado1 ? eje : eje * -1.0f;   // de cara a su rival
  // casi de lado: el otro fuera. El luchador entero por encima de la banda del subtitulo: X+48 es el suelo y mide
  // ~14 (medido en las capturas 20261016-025250); la zona libre (entre las bandas) tiene su centro 48 px por encima
  // del de la pantalla, asi que se apunta un poco por debajo del pecho. Con pose, mas lejos (la victoria se agacha y
  // senala, la entrada sube los brazos); la victoria ademas gira al personaje: la camara va al otro costado.
  const float l = (p == kLado1 ? 1.0f : -1.0f) * (pose == 2 ? -1.0f : 1.0f);
  // con pose a 46 (a 38 la victoria de Satan salia sin pies en el plano: auditoria7/G, captura c2_paso)
  const float s = p == kLado1 ? s1 : s2;
  *ojo = yo + (Unit(frente * 0.35f + lado * l) * (pose ? 46.0f : 28.0f) + V3{0, pose ? 9.0f : 8.0f, 0}) * s;
  *obj = yo + V3{0, (pose ? 7.0f : 6.0f) * s, 0};   // con pose, sitio para los altos (Ten Shin Han) y el brazo arriba
}

void Terminar(uint8_t* base, const char* por) {
  if (!g_en_curso.exchange(false)) return;
  const uint32_t mgr = REX_LOAD_U32(kMgrPtr);
  for (uint32_t k = 0; k < 2; ++k) {
    const uint32_t x = mgr + kX0 + kXPaso * k;
    if (mgr && g_cambiado[k] && int16_t(REX_LOAD_U16(x + kMandoIdx)) == g_usuario[k])
      REX_STORE_U16(x + kMandoIdx, uint16_t(g_mando[k]));
    if (g_usuario[k] >= 0) dbz3::net::InyectarPad(uint32_t(g_usuario[k]), -1);
    g_cambiado[k] = false;
    g_usuario[k] = -1;
  }
  OcultarHud(false);
  g_paso = -1;
  g_abre = 0;
  {
    std::lock_guard<std::mutex> lk(g_mu);
    g_esc.reset();
  }
  REXLOG_INFO("dbz3 cinematica: {} en el cuadro {}", por, g_frame);
}

// ---- Panel (hilo de la interfaz): bandas, retrato, subtitulo y "A: saltar" ----
struct Tex {
  std::unique_ptr<rex::ui::ImmediateTexture> t;
  float w = 0, h = 0;
};
class Panel : public rex::ui::ImGuiDialog {
 public:
  explicit Panel(rex::ui::ImGuiDrawer* d) : ImGuiDialog(d) {}

 protected:
  void OnDraw(ImGuiIO& io) override {
    if (!g_en_curso.load()) {
      a_antes_ = start_antes_ = true;
      return;
    }
    if (auto* rt = rex::Runtime::instance())
      if (auto* in = static_cast<rex::input::InputSystem*>(rt->input_system())) {
        rex::input::X_INPUT_STATE st = {};
        const bool ok = in->GetStateForUI(0, &st) == 0;
        const bool a = ok && (st.gamepad.buttons & 0x1000), start = ok && (st.gamepad.buttons & 0x0010);
        if (a && !a_antes_) g_siguiente = true;
        if (start && !start_antes_) g_saltar = true;
        a_antes_ = a;
        start_antes_ = start;
      }
    std::shared_ptr<const Escena> e;
    {
      std::lock_guard<std::mutex> lk(g_mu);
      e = g_esc;
    }
    const int paso = g_paso.load();
    if (e.get() != esc_ || paso != paso_) {   // textura del subtitulo del paso nuevo
      esc_ = e.get();
      paso_ = paso;
      texto_ = Tex{};
      auto* dr = dbz3::launcher::PreviewDrawer();
      if (e && paso >= 0 && paso < int(e->pasos.size()) && e->pasos[size_t(paso)].texto && dr) {
        const pasarela::Imagen& im = e->pasos[size_t(paso)].texto;
        texto_.t = dr->CreateTexture(uint32_t(im.w), uint32_t(im.h), rex::ui::ImmediateTextureFilter::kLinear, false,
                                     im.rgba.data());
        texto_.w = float(im.w);
        texto_.h = float(im.h);
        // letra clara (avisos de B1, para su franja oscura) o oscura (charlas, para el bocadillo claro)
        uint64_t luz = 0, n = 0;
        for (size_t i = 0; i + 3 < im.rgba.size(); i += 4)
          if (im.rgba[i + 3] > 64) luz += im.rgba[i] + im.rgba[i + 1] + im.rgba[i + 2], ++n;
        claro_ = !n || luz / (3 * n) > 100;   // por defecto franja oscura (subtitulos de IW: blanco con borde)
      }
    }
    const S::Area a = S::GameArea(io);
    const float w = io.DisplaySize.x, h = io.DisplaySize.y, abre = Suave(g_abre.load());
    const float arriba = (a.p0.y + 90.0f * a.s) * abre, abajo = (h - a.At(0, 690).y) * abre;   // el marcador ya no sale
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    dl->AddRectFilled(ImVec2(0, 0), ImVec2(w, arriba), IM_COL32(0, 0, 0, 255));
    dl->AddRectFilled(ImVec2(0, h - abajo), ImVec2(w, h), IM_COL32(0, 0, 0, 255));
    S::HelpLine(dl, ImVec2(w * 0.80f, arriba * 0.5f), S::mode::kSmall * a.s,
                {{"A", dbz3::i18n::T("Siguiente", "Next")}, {"START", dbz3::i18n::T("Saltar", "Skip")}});
    if (abre < 0.99f || !e || paso < 0 || paso >= int(e->pasos.size())) return;
    const Paso& p = e->pasos[size_t(paso)];
    // subtitulo en la banda de abajo: retrato a la izquierda (cara del marcador) y la imagen o el texto
    const float y0 = h - abajo + S::mode::kPad * a.s, alto = abajo - 2 * S::mode::kPad * a.s;
    float x = a.At(S::mode::kX0 - 220, 0).x;
    ImTextureID cara_tex = p.cara >= 0 ? pasarela::CaraDe(p.cara) : ImTextureID{};
    if (!cara_tex && p.cara_alt >= 0) cara_tex = pasarela::CaraDe(p.cara_alt);
    if (p.cara >= 0 || p.cara_alt >= 0)
      if (ImTextureID c = cara_tex) {
        dl->AddImage(c, ImVec2(x, y0), ImVec2(x + alto, y0 + alto));
        x += alto + S::mode::kGap * a.s;
      }
    if (texto_.t) {   // fondo como el del juego de origen: bocadillo claro para letra oscura, franja oscura para clara
      const float k = std::min(alto / texto_.h, (a.At(S::mode::kX1 + 220, 0).x - x) / texto_.w);
      const ImVec2 q0(x, y0 + (alto - texto_.h * k) * 0.5f), q1(x + texto_.w * k, q0.y + texto_.h * k);
      dl->AddRectFilled(ImVec2(q0.x - 8 * a.s, q0.y - 6 * a.s), ImVec2(q1.x + 8 * a.s, q1.y + 6 * a.s),
                        claro_ ? IM_COL32(8, 30, 48, 230) : IM_COL32(236, 232, 220, 235), 10 * a.s);
      dl->AddImage(reinterpret_cast<ImTextureID>(texto_.t.get()), q0, q1);
    } else if (!p.linea.empty()) {
      dl->AddText(nullptr, S::mode::kRow * a.s, ImVec2(x, y0 + alto * 0.3f), IM_COL32(255, 255, 255, 255), p.linea.c_str());
    }
  }

 private:
  bool a_antes_ = true, start_antes_ = true;
  const Escena* esc_ = nullptr;
  int paso_ = -2;
  Tex texto_;
  bool claro_ = false;
};
void CrearPanel() {
  static Panel* panel = nullptr;   // vive lo que el juego
  auto* rt = rex::Runtime::instance();
  if (!panel && rt && rt->imgui_drawer()) panel = new Panel(rt->imgui_drawer());
}

void EmpezarPaso(int k, const Escena& e) {
  g_paso = k;
  g_t0_paso = AhoraMs();
  const Paso& p = e.pasos[size_t(k)];
  if (!p.voces.empty() && !e.iso_voces.empty()) pasarela::OirVozB1(e.iso_voces, p.voces, e.volumen);
  REXLOG_INFO("dbz3 cinematica: paso {}/{} (plano {}, {:.1f} s, botones {:04X}/{:04X}, voces {})", k + 1, e.pasos.size(),
              p.plano, p.segundos, p.botones[0], p.botones[1], p.voces.size());
}

}  // namespace

void Empezar(Escena e) {
  static const bool sin = std::getenv("DBZ3_SIN_CINE") != nullptr;   // pruebas que pelean desde el primer cuadro
  if (sin) return;
  {
    std::lock_guard<std::mutex> lk(g_mu);
    g_pedida = e.pasos.empty() ? nullptr : std::make_shared<const Escena>(std::move(e));
  }
  static bool panel = false;
  if (!panel) {
    panel = true;
    dbz3::launcher::DeferUi(&CrearPanel);
  }
}
bool Activa() {
  std::lock_guard<std::mutex> lk(g_mu);
  return g_pedida || g_en_curso.load();
}
void Cancelar() {
  std::lock_guard<std::mutex> lk(g_mu);
  g_pedida.reset();
  g_saltar = g_en_curso.load();
  g_siguiente = false;
}

bool Conectada() { return g_conectada.load(); }

// Hilo del juego, con contexto (net/netplay_hooks.cpp, al leer el mando virtual): la animacion de cada lado como la
// arranca el propio juego (sub_82103C48: accion anterior en X+3246, accion nueva en X+3244, sub_82103978(X, 0),
// codigo en X+2928 y sub_82114FB8(X)). Las poses van en la accion 15 (la de la entrada y la victoria, medido con la
// sonda); cuando acaba y el juego vuelve a la 0, se repite mientras dura la linea. Al pasar a un paso sin pose, la
// guardia (accion 0, codigo del principio). US y EU (parejas arriba).
void Arrancar(PPCContext& ctx, uint8_t* base, uint32_t x, uint16_t accion, uint32_t codigo) {
  const uint16_t antes = REX_LOAD_U16(x + 3244);
  REX_STORE_U16(x + 3244, accion);
  REX_STORE_U16(x + 3246, antes);
  REX_STORE_U16(x + 3248, 0);
  PPCContext c = ctx;
  c.r3.u64 = x;
  c.r4.u64 = 0;
  DBZ3_FIN_ACCION(c, base);
  REX_STORE_U32(x + kCodigo, codigo);
  c = ctx;
  c.r3.u64 = x;
  DBZ3_ARRANCA_ANIM(c, base);
}
void Tick(PPCContext& ctx, uint8_t* base) {
  static uint32_t hecho = ~0u;
  // pose de cada lado puesta en este paso (escena, paso, cuadro): al acabarla el juego vuelve a la accion 0 y se volvia
  // a poner sin fin (la de victoria cada 3,98 s: Satan repetia el gesto ~3 veces; auditoria10/D)
  static const Escena* armada_e = nullptr;
  static int armada_k[2] = {-1, -1};
  static uint32_t armada_c[2] = {};
  if (!g_en_curso.load() || hecho == g_frame) return;
  hecho = g_frame;
  std::shared_ptr<const Escena> e;
  {
    std::lock_guard<std::mutex> lk(g_mu);
    e = g_esc;
  }
  const int k = g_paso.load();
  const uint32_t mgr = REX_LOAD_U32(kMgrPtr);
  if (!e || !mgr || k < 0 || k >= int(e->pasos.size())) return;
  for (uint32_t s = 0; s < 2; ++s) {
    const uint32_t x = mgr + kX0 + kXPaso * s;
    const int pose = e->pasos[size_t(k)].anim[s];
    // sin pose: no se fuerza la guardia (al acabar la suya el juego vuelve solo a la accion 0; forzarla a mitad dejaba
    // al luchador corriendo de vuelta, captura 20261016-035508 c4)
    if (pose < 0) continue;
    if (e.get() != armada_e) armada_e = e.get(), armada_k[0] = armada_k[1] = -1;
    // una vez por paso (los 30 primeros cuadros puede reintentar si el juego aun no la tomo)
    if (armada_k[s] == k && g_frame - armada_c[s] > 30) continue;
    const uint16_t accion = pose >= 0 ? kAccionPose : 0;
    const uint32_t quiero = pose >= 0 ? uint32_t(pose) : g_codigo0[s];
    g_puesto[s] = pose >= 0;
    if (REX_LOAD_U16(x + 3244) == accion && (REX_LOAD_U32(x + kCodigo) & 0xFFF) == quiero) continue;
    if (armada_k[s] != k) armada_k[s] = k, armada_c[s] = g_frame;
    Arrancar(ctx, base, x, accion, quiero);
    REXLOG_INFO("dbz3 cinematica: lado {} accion {} animacion {:03X} (paso {}, cuadro {})", s + 1, accion, quiero, k + 1,
                g_frame);
  }
}

void Fotograma(uint8_t* base, uint32_t estado) {
  g_conectada = true;
  const uint32_t mgr = REX_LOAD_U32(kMgrPtr);
  // SONDA (DBZ3_CINE_TRAZA=1): estado del combate, accion (X+3244) y codigo de animacion (X+2928) de cada luchador al
  // cambiar: de aqui salen los codigos de la entrada, la carga de ki y la victoria que reproducen las escenas
  static const bool traza = std::getenv("DBZ3_CINE_TRAZA") != nullptr;
  if (traza && mgr)
    for (uint32_t s = 0; s < 2; ++s) {
      static uint32_t visto[2] = {~0u, ~0u};
      const uint32_t x = mgr + kX0 + kXPaso * s, ac = REX_LOAD_U16(x + 3244), an = REX_LOAD_U32(x + kCodigo) & 0xFFF;
      const uint32_t clave = estado << 28 | ac << 12 | an;
      if (clave == visto[s]) continue;
      visto[s] = clave;
      REXLOG_INFO("dbz3 cinematica: sonda lado {} (id {}): estado {} accion {} codigo {:03X}", s + 1,
                  REX_LOAD_U32(x + 7420), estado, ac, an);
    }
  const bool saltar = g_saltar.exchange(false);
  if (!mgr || estado != 4 || saltar) {
    Terminar(base, saltar ? "saltada" : "cortada");
    return;
  }
  std::shared_ptr<const Escena> e;
  if (!g_en_curso.load()) {
    {
      std::lock_guard<std::mutex> lk(g_mu);
      e = std::move(g_pedida);
      g_esc = e;
    }
    if (!e) return;
    // mandos virtuales (XInput, net/netplay_hooks.cpp): el humano sigue en el suyo, que deja de leerse; el COM pasa
    // al del otro lado (su IA para mientras dura). ponytail: darle uno libre (el 4, como el relevo del Tag Team)
    // saca la pausa "2P PAUSE" (prueba 2026-10-16); compartido, el que no sale en el plano repite la pose.
    for (uint32_t k = 0; k < 2; ++k) g_mando[k] = int16_t(REX_LOAD_U16(mgr + kX0 + kXPaso * k + kMandoIdx));
    for (uint32_t k = 0; k < 2; ++k) {
      const uint32_t x = mgr + kX0 + kXPaso * k;
      const int16_t otro = g_mando[1 - k];
      g_cambiado[k] = g_mando[k] < 0 || g_mando[k] > 3;
      const int u = g_cambiado[k] ? (otro >= 0 && otro <= 3 ? otro : 0) : g_mando[k];
      g_usuario[k] = u;
      dbz3::net::InyectarPad(uint32_t(u), 0);
      if (g_cambiado[k]) REX_STORE_U16(x + kMandoIdx, uint16_t(u));
    }
    REXLOG_INFO("dbz3 cinematica: mandos {} / {} -> virtuales {} / {}", g_mando[0], g_mando[1], g_usuario[0], g_usuario[1]);
    OcultarHud(true);
    for (uint32_t s = 0; s < 2; ++s) {
      g_codigo0[s] = REX_LOAD_U32(mgr + kX0 + kXPaso * s + kCodigo) & 0xFFF;
      g_puesto[s] = false;
    }
    g_pasado = REX_LOAD_U32(mgr + kPasado);
    g_frame = 0;
    g_t0 = AhoraMs();
    g_en_curso = true;
    EmpezarPaso(0, *e);
  } else {
    std::lock_guard<std::mutex> lk(g_mu);
    e = g_esc;
  }
  if (!e) return Terminar(base, "sin escena");
  REX_STORE_U32(mgr + kPasado, g_pasado);   // reloj parado (el tiempo del combate empieza al acabar la escena)
  int k = g_paso.load();
  // A pasa al paso siguiente (como mucho uno cada 0,4 s: la voz nueva corta la anterior, PlaySound)
  const bool adelante = AhoraMs() - g_t0_paso > 400 && g_siguiente.exchange(false);   // antes: se aplaza, no se pierde
  // Duracion del paso: si la escena de motor trae los cuadros reales (tabla de clips de su banco), manda el
  // frame a 60 Hz; si no, los segundos del respaldo.
  const Paso& paso_antes = e->pasos[size_t(k)];
  const float seg_paso = paso_antes.cuadros > 0 ? float(paso_antes.cuadros) / 60.0f : paso_antes.segundos;
  if (adelante || float(AhoraMs() - g_t0_paso) >= seg_paso * 1000.0f) {
    if (++k >= int(e->pasos.size())) return Terminar(base, "terminada");
    EmpezarPaso(k, *e);
  }
  const Paso& p = e->pasos[size_t(k)];   // el paso que se ve AHORA (pudo avanzar arriba)
  {   // botones del paso en el mando virtual de cada lado; si lo comparten, los del lado del plano
    const uint16_t b0 = p.botones[0], b1 = p.botones[1];
    if (g_usuario[0] >= 0 && g_usuario[0] == g_usuario[1])
      dbz3::net::InyectarPad(uint32_t(g_usuario[0]), p.plano == kLado1 ? b0 : p.plano == kLado2 ? b1 : (b0 | b1));
    else
      for (uint32_t s = 0; s < 2; ++s)
        if (g_usuario[s] >= 0) dbz3::net::InyectarPad(uint32_t(g_usuario[s]), p.botones[s]);
  }
  if (g_frame % 20 == 0 && (e->pasos[size_t(k)].botones[0] | e->pasos[size_t(k)].botones[1]))   // traza: accion (X+3244)
    REXLOG_INFO("dbz3 cinematica: cuadro {} paso {}: acciones {} / {}", g_frame, k + 1,
                REX_LOAD_U16(mgr + kX0 + 3244), REX_LOAD_U16(mgr + kX0 + kXPaso + 3244));
  // camara: plano del paso, fundido desde el anterior; al final, hacia la del combate
  V3 p1 = LeeV(base, mgr + kX0 + kPos), p2 = LeeV(base, mgr + kX0 + kXPaso + kPos);
  if (e->distancia > 0) {   // separados como pide la escena (B1: lados en x = -15 y +15)
    const V3 c = Mezcla(p1, p2, 0.5f), eje = Unit(V3{p2.x - p1.x, 0, p2.z - p1.z});
    p1 = V3{c.x, p1.y, c.z} - eje * (e->distancia * 0.5f);
    p2 = V3{c.x, p2.y, c.z} + eje * (e->distancia * 0.5f);
    PonV(base, mgr + kX0, p1);
    PonV(base, mgr + kX0 + kXPaso, p2);
  }
  V3 ojo, obj, ojo0, obj0;
  auto con_pose = [&](int i) {
    const Paso& q = e->pasos[size_t(i)];
    const int a = q.plano == kLado1 ? q.anim[0] : q.plano == kLado2 ? q.anim[1] : -1;
    return a < 0 ? 0 : a == kPoseVictoria ? 2 : 1;
  };
  const float s1 = dbz3::gigantes::EscalaDe(base, 0), s2 = dbz3::gigantes::EscalaDe(base, 1);
  Encuadre(e->pasos[size_t(k)].plano, p1, p2, &ojo, &obj, con_pose(k), s1, s2);
  const float t = float(AhoraMs() - g_t0_paso) / 1000.0f, f = Suave(t / kFundido);
  if (k > 0 && f < 1.0f) {
    Encuadre(e->pasos[size_t(k - 1)].plano, p1, p2, &ojo0, &obj0, con_pose(k - 1), s1, s2);
    ojo = Mezcla(ojo0, ojo, f);
    obj = Mezcla(obj0, obj, f);
  }
  float oj[3], ob[3];
  const bool ultimo = k + 1 == int(e->pasos.size());
  // Camara del clip de la linea (escenas de MOTOR de IW): ojo/objetivo/fov/roll por cuadro, 60 Hz. Tiene
  // prioridad sobre el plano del respaldo (Encuadre); el fundido al final sigue mezclando con la del combate.
  float roll = 0.0f;
  {
    const float tf = t * 60.0f;
    float co[3], cb[3];
    if (MuestraCam(p.cam_ojo, tf, co) && MuestraCam(p.cam_obj, tf, cb)) {
      ojo = V3{co[0], co[1], co[2]};
      obj = V3{cb[0], cb[1], cb[2]};
      if (p.rel) {   // marco de los luchadores (p1/p2 ya estan en su sitio de la escena)
        const V3 c = Mezcla(p1, p2, 0.5f), eje = Unit(V3{p2.x - p1.x, 0, p2.z - p1.z});
        const V3 lado{-eje.z, 0, eje.x};
        auto mundo = [&](V3 l) { return c + eje * l.x + V3{0, l.y, 0} + lado * l.z; };
        ojo = mundo(ojo);
        obj = mundo(obj);
      }
      float fov = 0.6f;
      if (MuestraCam(p.cam_fov, tf, &fov)) {
        fov = std::clamp(fov, 0.2f, 1.3f);
        uint32_t u;
        std::memcpy(&u, &fov, 4);
        REX_STORE_U32(mgr + 0x1C4, u);   // fov vertical (cine.cpp)
      }
      MuestraCam(p.cam_roll, tf, &roll);
    }
  }
  const float queda = p.segundos - t;
  if (ultimo && queda < kFundido && cine::LeerCamara(base, oj, ob)) {
    const float v = Suave(1.0f - queda / kFundido);
    ojo = Mezcla(ojo, V3{oj[0], oj[1], oj[2]}, v);
    obj = Mezcla(obj, V3{ob[0], ob[1], ob[2]}, v);
  }
  const float eo[3] = {ojo.x, ojo.y, ojo.z}, eb[3] = {obj.x, obj.y, obj.z};
  cine::Camara(base, eo, eb, roll);
  g_abre = std::min({1.0f, float(AhoraMs() - g_t0) / 500.0f, ultimo ? queda / 0.5f : 1.0f});
  ++g_frame;
}

// Tras leer el mando: nada (los botones del paso van por XInput, InyectarPad); se queda para el gancho de combate.cpp.
void Mando(uint8_t* /*base*/, uint32_t /*usuario*/) {}

}  // namespace dbz3::cinematica
