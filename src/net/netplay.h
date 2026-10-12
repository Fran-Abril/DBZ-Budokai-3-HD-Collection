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
  bool Host(uint16_t port, uint8_t delay, std::string* error);
  bool Join(const std::string& ip, uint16_t port, std::string* error);
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
  std::vector<std::string> ModDiff() const;   // mods que solo tiene uno de los dos
  uint8_t delay() const { return delay_; }

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
  bool OpenSocket(uint16_t port, std::string* error);
  void Send(const std::vector<uint8_t>& pkt);
  bool Recv(std::vector<uint8_t>& pkt);
  void Handle(const std::vector<uint8_t>& pkt, bool from_peer);
  void SendHello(uint8_t type);
  void SendInputs();
  void ApplySave();
  void Fail(Link l, const std::string& why);   // estado de error con mu_ (lo leen dos hilos)

  std::atomic<Link> link_{Link::kIdle};
  std::atomic<Role> role_{Role::kNone};   // lo leen el hilo de la interfaz y el del juego
  uint8_t delay_ = 3;
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

  // emparejamiento
  std::string my_build_, my_mods_hash_;
  std::vector<std::string> my_mods_;      // "nombre|hash" por mod
  std::vector<std::string> peer_mods_;
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
  uint64_t team_sent_ms_ = 0;
  mutable std::mutex mu_;

  // lockstep
  static constexpr int kRing = 256;
  struct Slot { uint32_t tick = 0xFFFFFFFFu; Pad pad; };
  std::array<Slot, kRing> mine_{}, theirs_{};
  std::array<uint32_t, kRing> my_rng_{}, their_rng_{};
  std::array<uint32_t, kRing> my_rng_tick_{}, their_rng_tick_{};
  Pad applied_[2]{};
  uint32_t tick_ = 0;
  bool armed_ = false;
  std::atomic<bool> desync_{false};
};

}  // namespace dbz3::net
