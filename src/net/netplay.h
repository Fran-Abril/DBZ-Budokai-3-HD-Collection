// dbz3 - Online P2P por IP directa (UDP, retardo de entrada fijo, lockstep).
//
// Reutiliza el 1P vs 2P del juego: en los dos PCs el usuario 0 del guest recibe el mando del
// anfitrion y el usuario 1 el del invitado, con D ticks de retardo. Un tick = una iteracion del
// bucle principal (sondeo de mandos sub_820902F8, una vez por iteracion). Si los dos juegos
// arrancan igual (mismo exe, mismos mods, misma partida guardada) y reciben los mismos mandos en
// el mismo tick, la simulacion es la misma (el RNG del juego avanza por iteracion).
//
// Hilos: el emparejamiento (Host/Join/Poll/RequestStart) va en el hilo del launcher; despues
// de Start() el socket solo lo usa el hilo del guest (RunTick), que puede bloquear esperando al
// rival. Mapa y evidencias: docs/08_online/ONLINE.md.
//
// Retardo adaptativo (ficha 02 F1, AUTO): el anfitrion mide el ping (cada INPUT lleva una marca de tiempo
// y el eco de la del otro) y las esperas, y cambia D en plena partida; el invitado sigue el D del anfitrion.
// Sin tick de activacion acordado: cada PC llena sus casillas de mando una sola vez y en orden (sched_):
// al subir D repite la muestra actual en las casillas nuevas (alarga un instante lo que ya estaba pulsado)
// y al bajarlo solo se salta una muestra igual a la anterior (no se pierde ninguna pulsacion). Las dos
// simulaciones aplican la misma casilla en el mismo tick, asi que siguen iguales aunque cada PC cambie D
// en un tick distinto.
#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace dbz3::net {

enum class Role : uint8_t { kNone, kHost, kGuest };
enum class Link : uint8_t {
  kIdle,        // sin sesion
  kWaiting,     // anfitrion esperando / invitado llamando
  kConnected,   // enlazados (mods y partida comprobados abajo)
  kStarting,    // los dos van a empezar
  kPlaying,     // lockstep en marcha
  kLost,        // el rival dejo de responder
  kError,
};

// Reglas opcionales del combate que cambian la simulacion (src/combate.cpp, F1 > COMBATE): bit0 reglas v0, bit1
// definitiva directa, bits 2-3 boton de cancelacion (4 lb, 8 rb); reversion = dano que quita la forma (0 nunca).
struct CombatRules {
  uint8_t flags = 0;
  uint16_t reversion = 0;
  bool operator==(const CombatRules& o) const { return flags == o.flags && reversion == o.reversion; }
  bool any() const { return flags || reversion; }
  static CombatRules Now();   // las del F1 de este PC
  void Apply() const;         // las pone en el F1 de este PC
};

// Un tick de mando (X_INPUT_GAMEPAD en orden del host).
struct Pad {
  uint16_t buttons = 0;
  uint8_t lt = 0, rt = 0;
  int16_t lx = 0, ly = 0, rx = 0, ry = 0;
};

class Session {
 public:
  static Session& Get();

  // ---- hilo del launcher
  // delay 0 = AUTO (empieza en kAutoStart y se ajusta solo durante la partida)
  bool Host(uint16_t port, uint8_t delay, std::string* error);
  bool Join(const std::string& ip, uint16_t port, std::string* error);
  // Must be called while idle by the isolated build's admission adapter.
  // digest: 40 lowercase hex, freshly measured executable/SDK/guest/content/rules.
  // safe_selection: adapter proves F2 disabled, Normal capsules and no private
  // form/costume reinjection. False or missing proof rejects Host/Join.
  bool ConfigureIdentity(const std::string& digest, bool safe_selection, bool safe_tagteam = false);
  void Poll();             // avanza el emparejamiento; llamar en cada fotograma del launcher
  void RequestStart();     // anfitrion: empezar (el invitado arranca al recibirlo)
  // Hilo del guest, una vez por tick: true una vez al empezar (los dos de acuerdo). Aplica la
  // semilla del RNG y, en el invitado, el estado de desbloqueos/ultima eleccion del anfitrion.
  bool TakeStart(uint8_t* base);
  // Hilo del guest: foto del estado de menu que el anfitrion manda al empezar.
  void SetUiSnapshot(const uint8_t* p, size_t n);
  void Close();

  Link link() const { return link_.load(); }
  Role role() const { return role_; }
  std::string Status() const;
  std::string LocalAddresses() const;   // IPs locales de este PC (para dar al rival)
  // Codigo de sala del anfitrion (XXXXX-XXXXX, IP publica + puerto; mientras no se sabe, el de la red
  // local) y como se consiguio: "router" (UPnP abrio el puerto), "stun" (IP publica sin abrir el
  // puerto) o "local" (solo red de casa). Join() acepta el codigo o una IP.
  std::string RoomCode(std::string* how = nullptr) const;
  bool mods_match() const { return mods_match_.load(); }
  bool save_ready() const { return save_ready_.load(); }
  // Con mods distintos la sesion sigue conectada (sin poder empezar) y se cruzan las listas.
  struct ModDelta {
    enum Kind : uint8_t { kMissing, kExtra, kDifferent } kind;   // falta aqui / sobra aqui / otra version
    std::string name;   // carpeta del mod (= id de Nova's Mods si se instalo desde alli)
  };
  // false mientras no ha llegado la lista completa del otro PC.
  bool ModDiff(std::vector<ModDelta>& out) const;
  // Sala: rango (ficha 01) y reglas de combate. Lo propio se fija antes de Host/Join (tramo -1 = sin combates; las
  // reglas, foto del F1 en ese momento); lo del otro llega con su equipo cada medio segundo (false mientras no).
  // Sin la sala del otro no se empieza.
  void SetProfile(int elo, int tramo);
  void MyRoom(int* elo, int* tramo, CombatRules* rules) const;
  bool PeerRoom(int* elo, int* tramo, CombatRules* rules) const;
  // Jugando online: las reglas opcionales del combate valen si las dos fotos eran iguales (combate.cpp, Rules()).
  // Durante la partida se quedan fijas: si alguien las cambia en F1, vuelven a las de la sala.
  bool CombatRulesAgreed() const { return active() && rules_agreed_.load(); }
  uint8_t delay() const { return delay_; }
  // Indicador en partida: ping medido (ms de ida y vuelta, media del ultimo segundo; -1 sin dato), si el
  // retardo es AUTO (en el invitado, el del anfitrion) y cuando cambio por ultima vez (steady_clock, ms).
  int ping_ms() const { return ping_ms_.load(); }
  bool auto_delay() const { return auto_view_.load(); }
  uint64_t delay_changed_ms() const { return delay_changed_ms_.load(); }
  static constexpr uint8_t kAutoStart = 3, kMinDelay = 2, kMaxDelay = 10;

  // ---- hilo del guest (hooks), solo con kPlaying
  bool active() const { return link_.load() == Link::kPlaying; }
  void Arm() { armed_ = true; }
  bool armed() const { return armed_; }
  // Intercambia el mando local del tick y espera el del rival (bloquea). rng = estado del
  // generador del juego en este punto (comprobacion de desincronia; 0 = sin dato).
  void RunTick(const Pad& local, uint32_t rng);
  // Hilo de la interfaz: salir de la partida en curso. Lo cierra el hilo del juego (RunTick).
  void RequestLeave() { leave_req_ = true; }
  const Pad& Applied(uint32_t user) const { return applied_[user & 1]; }
  uint32_t tick() const { return tick_; }
  // Al empezar cada combate: misma semilla del RNG en los dos juegos (las esperas de carga
  // gastan numeros aleatorios distintos en cada PC). Solo jugando online; US.
  void ReseedBattle(uint8_t* base);
  // Valor pseudoaleatorio compartido por los dos PCs (frente 12: decisiones de la simulacion, p.ej. el
  // escenario de la revancha aleatoria). Online sale de la semilla y del numero de combate (igual en los
  // dos PC: sin desincronia); sin sesion, de un generador local. `sal` desambigua llamadas.
  uint32_t AzarCompartido(uint32_t sal);
  // Al empezar cada combate: cada PC tarda distinto en cargarlo, asi que el primero que llega espera
  // (gastando ticks sin mover el juego) a que el otro llegue, y los dos siguen en el mismo tick.
  // Sin esto uno pelea unos fotogramas por delante y los mandos caen en momentos distintos
  // (medido 2026-10-11: DESINCRONIA medio segundo despues de empezar).
  void BattleBarrier();
  bool desynced() const { return desync_.load(); }

  // Partida guardada: el invitado juega con la del anfitrion durante la sesion; la suya se
  // copia antes y se devuelve al volver a abrir el juego (RestoreSave al arrancar).
  static void RestoreSave();
  static constexpr uint32_t kUiSnap = 0x824BA110, kUiSnapEu = 0x824BA100, kUiSnapSize = 0x100;

 private:
  Session() = default;
  // All nonblocking socket operations and the UI->guest handoff share this lock.
  // RunTick NEVER holds it while sleeping/waiting for the peer.
  mutable std::recursive_mutex io_mu_;
  bool guest_owner_ = false;
  uint64_t generation_ = 0;
  uint32_t local_nonce_ = 0, peer_nonce_ = 0;
  std::string identity_;
  bool identity_ready_ = false;
  bool identity_admission_ = false; // one fresh measurement per Host/Join
  bool safe_tagteam_ = false; // separate proof for operation ids, finite abort, resource lifetime
  std::vector<int> local_team_;
  bool team_ready_ = false;
  void ResetTransport();
  bool ValidPacket(const std::vector<uint8_t>& pkt) const;
  bool OpenSocket(uint16_t port, std::string* error);
  void Send(const std::vector<uint8_t>& pkt);
  bool Recv(std::vector<uint8_t>& pkt);
  void Handle(const std::vector<uint8_t>& pkt, bool from_peer);
  void SendHello(uint8_t type);
  void StartModScan();                   // huella de los mods propios en su hilo (my_build_ != "" = lista)
  void SendInputs();
  void Drain();                          // hilo del juego: lee todo lo que haya llegado (sin esperar)
  void FlushRtt();                       // la muestra de ping de la tanda leida pasa a la ventana
  void AdaptDelay(uint32_t t);           // cada 60 ticks: ping del segundo y, en el anfitrion AUTO, nuevo D
  void SetDelay(uint8_t d, const char* why);
  void ApplySave();
  void Fail(Link l, const std::string& why);   // estado de error con mu_ (lo leen dos hilos)

  std::atomic<Link> link_{Link::kIdle};
  std::atomic<Role> role_{Role::kNone};   // lo leen el hilo de la interfaz y el del juego
  std::atomic<uint8_t> delay_{3};
  intptr_t sock_ = -1;
  uint8_t peer_addr_[32] = {};   // sockaddr_in del rival
  int peer_len_ = 0;
  bool have_peer_ = false;
  uint64_t last_send_ms_ = 0, last_recv_ms_ = 0;
  std::string error_;
  // codigo de sala: IP publica (orden de red) y puerto vistos desde fuera
  uint32_t pub_ip_ = 0;
  uint16_t pub_port_ = 0, upnp_port_ = 0;
  std::string pub_how_;
  uint64_t stun_sent_ms_ = 0;
  int stun_tries_ = 0;
  uint8_t stun_addr_[16] = {};   // sockaddr_in del servidor STUN (lo resuelve el hilo de Host; con mu_)
  bool stun_ready_ = false;

  // emparejamiento
  std::string my_build_, my_mods_hash_;
  std::vector<std::string> my_mods_;      // "nombre|hash" por mod
  std::vector<std::string> peer_mods_;
  bool peer_mods_known_ = false;          // llego al menos un trozo de su lista (con mu_)
  uint64_t mod_req_ms_ = 0;
  std::atomic<bool> mods_match_{false};
  std::atomic<bool> save_ready_{false};
  std::vector<uint8_t> save_;            // anfitrion: la suya; invitado: la recibida
  std::vector<uint8_t> save_have_;       // invitado: trozos recibidos
  uint32_t save_size_ = 0;
  bool start_sent_ = false;
  std::atomic<bool> launch_{false};       // Handle (interfaz) lo pone, TakeStart (juego) lo consume
  std::atomic<bool> leave_req_{false};
  uint32_t seed_ = 0;
  uint32_t battles_ = 0;   // combates empezados en esta partida online
  std::vector<uint8_t> ui_snap_;          // anfitrion: la suya; invitado: la recibida
  std::vector<int> peer_team_;           // equipo de Tag Team del otro (kTeam)
  int my_elo_ = 1000, my_tramo_ = -1;    // rango propio y del otro (kProfile; con mu_)
  int peer_elo_ = 0, peer_tramo_ = -1;
  bool peer_profile_ = false;
  CombatRules my_rules_, peer_rules_;    // con mu_
  CombatRules play_rules_;               // hilo del juego: las de la partida (fijas)
  std::atomic<bool> rules_agreed_{false};
  uint64_t team_sent_ms_ = 0;
  mutable std::mutex mu_;

  // lockstep
  static constexpr int kRing = 256;
  struct Slot { uint32_t tick = 0xFFFFFFFFu; Pad pad; };
  std::array<Slot, kRing> mine_{}, theirs_{};
  std::array<uint32_t, kRing> my_rng_{}, their_rng_{};
  std::array<uint32_t, kRing> my_rng_tick_{}, their_rng_tick_{};
  Pad applied_[2]{};
  std::atomic<uint32_t> tick_{0};
  std::atomic<bool> armed_{false};
  std::atomic<bool> desync_{false};

  // retardo adaptativo (ver arriba)
  bool auto_delay_ = false;              // anfitrion: AUTO elegido en la pantalla ONLINE
  uint8_t delay0_ = 3;                   // D al empezar: los ticks < delay0_ van sin mando en los dos PCs
  uint32_t sched_ = 0;                   // siguiente casilla propia por llenar (cada una se llena una vez)
  uint32_t peer_stamp_ = 0;              // ultima marca de tiempo del otro (se le devuelve como eco)
  uint64_t peer_stamp_at_ = 0;           // cuando llego (lo que se tarda en contestar no cuenta como ping)
  uint32_t peer_delay_tick_ = 0;         // invitado: tick mas nuevo con el que se aplico el D del anfitrion
  uint32_t rtt_n_ = 0, rtt_sum_ = 0, rtt_max_ = 0, stalls_ = 0;   // ventana de 60 ticks
  uint32_t batch_rtt_ = UINT32_MAX;      // menor muestra de la tanda de paquetes en curso
  int down_votes_ = 0;                   // segundos seguidos en los que sobra retardo
  uint32_t no_down_until_ = 0;           // tras subir por esperas, no se baja hasta este tick
  uint32_t rng_ok_ = 0;                  // puntos de control del RNG comparados e iguales (log)
  std::atomic<int> ping_ms_{-1};
  std::atomic<bool> auto_view_{false};
  std::atomic<uint64_t> delay_changed_ms_{0};
};

}  // namespace dbz3::net
