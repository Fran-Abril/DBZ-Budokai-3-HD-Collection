// dbz3 - Online P2P: sesion UDP, emparejamiento (mods + partida guardada) y lockstep.
// Ver netplay.h. Protocolo (little-endian, cabecera de 6 bytes "DBZ3" + tipo + version):
//   HELLO / HELLO_ACK  build[32] mods_hash[40] delay save_size save_hash[40] save_rel[96]
//   MODREQ / MODLIST   lista "mod|hash" troceada
//   SAVE_REQ idx  ->  SAVE_DATA idx size bytes[<=1024]
//   START / START_ACK
//   INPUT  n x {tick, pad[12]} + {rng_tick, rng}
//   BYE
#include "netplay.h"
#include "room.h"
#include "../tagteam.h"

#include "../launcher/i18n.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <random>
#include <stdexcept>
#include <thread>

#include <rex/filesystem.h>
#include <rex/hash.h>
#include <rex/logging.h>

#include "../launcher/mod_pipeline.h"
#include "../launcher/settings.h"
#include "../guest_region.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

// Existing Tag Team control bits; no resource or swap state is touched here.
namespace dbz3::tag { extern std::atomic<uint16_t> g_net_local; }
namespace dbz3::net {
namespace {

namespace fs = std::filesystem;
bool HexDigest(const std::string& s, size_t width = 40) {
  return s.size() == width && std::all_of(s.begin(), s.end(), [](char x) {
    return (x >= '0' && x <= '9') || (x >= 'a' && x <= 'f');
  });
}
uint32_t NewNonce() {
  std::random_device rd;
  uint32_t n = (uint32_t(rd()) ^ uint32_t(rd() << 1));
  return n ? n : 1;
}
namespace T = dbz3::i18n;

constexpr uint8_t kProto = 3;   // F4: full identity, challenge and directional session nonces
constexpr char kMagic[4] = {'D', 'B', 'Z', '3'};
enum Msg : uint8_t {
  kHello = 1, kHelloAck, kModReq, kModList, kSaveReq, kSaveData, kStart, kStartAck, kInput, kBye,
  kTeam, kChallenge, kReject   // equipo de Tag Team de quien lo manda (n, n ids u16); al final: los PCs viejos lo ignoran
};
constexpr uint32_t kChunk = 1024;
constexpr uint64_t kLostMs = 10000;
// MODLIST: first u16, total u16, n u8 y n entradas "carpeta|hash" de 256 bytes (con 128 un nombre largo se
// cortaba y salia como "version distinta"). 5 por paquete: 6+8+5+5*256 = 1299 B cabe en un paquete de
// Ethernet (1472 B de UDP) sin fragmentar; con 7 (1811 B) un router que tire fragmentos dejaba la lista colgada
constexpr size_t kModsPerPacket = 5, kModEntry = 256, kMaxMods = 512;
std::vector<uint8_t> StartPacket(uint32_t seed, const std::vector<uint8_t>& snap);

uint64_t NowMs() {
  return uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now().time_since_epoch()).count());
}

struct Writer {
  std::vector<uint8_t> b;
  explicit Writer(uint8_t type) {
    b.insert(b.end(), kMagic, kMagic + 4);
    b.push_back(type);
    b.push_back(kProto);
  }
  void u8(uint8_t v) { b.push_back(v); }
  void u16(uint16_t v) { b.push_back(uint8_t(v)); b.push_back(uint8_t(v >> 8)); }
  void u32(uint32_t v) { for (int i = 0; i < 4; ++i) b.push_back(uint8_t(v >> (8 * i))); }
  void str(const std::string& s, size_t n) {
    for (size_t i = 0; i < n; ++i) b.push_back(i < s.size() ? uint8_t(s[i]) : 0);
  }
  void bytes(const uint8_t* p, size_t n) { b.insert(b.end(), p, p + n); }
  void pad(const Pad& p) {
    u16(p.buttons); u8(p.lt); u8(p.rt);
    u16(uint16_t(p.lx)); u16(uint16_t(p.ly)); u16(uint16_t(p.rx)); u16(uint16_t(p.ry));
  }
};

struct Reader {
  const std::vector<uint8_t>& b;
  size_t o = 6;
  bool ok = true;
  explicit Reader(const std::vector<uint8_t>& v) : b(v) {}
  bool need(size_t n) { ok = ok && o <= b.size() && n <= b.size() - o; return ok; }
  uint8_t u8() { return need(1) ? b[o++] : 0; }
  uint16_t u16() { if (!need(2)) return 0; uint16_t v = uint16_t(b[o] | b[o + 1] << 8); o += 2; return v; }
  uint32_t u32() {
    if (!need(4)) return 0;
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v |= uint32_t(b[o + i]) << (8 * i);
    o += 4;
    return v;
  }
  std::string str(size_t n) {
    if (!need(n)) return {};
    std::string s(reinterpret_cast<const char*>(&b[o]), n);
    o += n;
    return s.substr(0, s.find('\0'));
  }
  Pad pad() {
    Pad p;
    p.buttons = u16(); p.lt = u8(); p.rt = u8();
    p.lx = int16_t(u16()); p.ly = int16_t(u16()); p.rx = int16_t(u16()); p.ry = int16_t(u16());
    return p;
  }
};

// ---- huella de los mods: lo que cambia la partida (personajes, modelos, movesets, AFS).
// Fuera solo los packs de texturas (solo aspecto). _roster y las vistas previas SI cuentan: dos
// kits distintos pueden montar plazas distintas con los mismos mods (mejor rechazar que desincronizar).
std::string FileHash(const fs::path& p) {
  const auto hash = rex::hash_file(p);
  if (!HexDigest(hash, 32)) throw std::runtime_error("no se pudo verificar contenido");
  return hash;
}

std::vector<std::string> ModFingerprints() {
  std::vector<std::string> out;
  const fs::path root = launcher::ModPipeline::ModsDir();
  std::error_code ec;
  std::vector<fs::path> mods;
  for (auto [it, iec] = std::pair{fs::directory_iterator(root, ec), std::error_code{}};
       !iec && it != fs::directory_iterator(); it.increment(iec)) {
    if (!it->is_directory(ec)) continue;
    const std::string name = rex::path_to_utf8(it->path().filename());
    // ".respaldo", ".install_*": carpetas internas del launcher, no mods
    if (name.empty() || name[0] == '.' || name.ends_with(".disabled") || fs::exists(it->path() / ".disabled", ec)) continue;
    mods.push_back(it->path());
  }
  std::sort(mods.begin(), mods.end());
  for (const auto& m : mods) {
    std::vector<std::pair<std::string, fs::path>> files;
    for (auto [it, iec] = std::pair{fs::recursive_directory_iterator(m, ec), std::error_code{}};
         !iec && it != fs::recursive_directory_iterator(); it.increment(iec)) {
      if (!it->is_regular_file(ec)) continue;
      // Texturas de pack (<hash XXH3>.png/.dds): solo aspecto, cada jugador puede llevar el suyo.
      // Ademas un pack HD son miles de PNG (GB): hashearlos bloqueaba el emparejamiento.
      std::string ext = rex::path_to_utf8(it->path().extension());
      std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return char(std::tolower(c)); });
      if ((ext == ".png" || ext == ".dds") &&
          settings::IsTexturePackFileName(rex::path_to_utf8(it->path().stem())))
        continue;
      const std::string rel = rex::path_to_utf8(fs::relative(it->path(), m, ec));
      // nombre/version/autor del mod: el launcher deja editarlo y el juego no lo lee
      if (rel == "manifest.txt") continue;
      // Include previews: conservative false rejection is preferable to false equality.
      files.emplace_back(rel, it->path());
    }
    if (files.empty()) continue;   // mod solo de texturas: no cuenta para el online
    std::sort(files.begin(), files.end());
    std::string acc;
    for (const auto& [rel, p] : files) acc += rel + "|" + FileHash(p) + "\n";
    out.push_back(rex::path_to_utf8(m.filename()) + "|" + rex::hash_bytes(acc));
  }
  return out;
}

// ---- partida guardada (data.bin del perfil de la consola emulada)
fs::path SavePath() {
  const fs::path root = settings::UserDataRoot();
  std::error_code ec;
  for (auto [it, iec] = std::pair{fs::recursive_directory_iterator(root, ec), std::error_code{}};
       !iec && it != fs::recursive_directory_iterator(); it.increment(iec)) {
    const auto& p = it->path();
    if (p.filename() == "data.bin" && p.parent_path().filename() == "DBZ3" &&
        rex::path_to_utf8(p).find("_online") == std::string::npos) {
      return p;
    }
  }
  return {};
}

fs::path BackupDir() { return settings::UserDataRoot() / "_online_respaldo"; }

std::vector<uint8_t> ReadFile(const fs::path& p) {
  std::ifstream f(p, std::ios::binary);
  return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

}  // namespace

Session& Session::Get() {
  static Session s;
  return s;
}

// F4 v3: complete structural and phase validation BEFORE peer claim or mutation.
// Directional nonce pair rejects delayed packets from previous sessions.
bool Session::ConfigureIdentity(const std::string& digest, bool safe_selection, bool safe_tagteam) {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  if (link_ != Link::kIdle || guest_owner_) return false;
  identity_ready_ = safe_selection && HexDigest(digest, 32);   // XXH3-128 (rex::hash_bytes)
  identity_admission_ = identity_ready_;
  safe_tagteam_ = identity_ready_ && safe_tagteam;
  identity_ = identity_ready_ ? digest : "";
  return identity_ready_;
}

bool Session::ValidPacket(const std::vector<uint8_t>& pkt) const {
  if (pkt.size() < 6 || pkt.size() > 2048 ||
      std::memcmp(pkt.data(), kMagic, 4) || pkt[5] != kProto) return false;
  const auto state = link_.load();
  if (state == Link::kIdle || state == Link::kLost || state == Link::kError) return false;
  const auto type = pkt[4];
  Reader r(pkt);
  if (type == kHello || type == kHelloAck) {
    if (state != Link::kWaiting && state != Link::kConnected) return false;
    if ((type == kHello) != (role_ == Role::kHost)) return false;
    auto build = r.str(40), mods = r.str(40);
    const auto delay = r.u8();
    const auto save = r.u32();
    const auto region = r.u8();
    const auto nonce = r.u32(), echo = r.u32();
    return r.ok && r.o == pkt.size() && identity_ready_ && build == identity_ &&
           HexDigest(mods, 32) && delay >= 1 && delay <= 10 && !save && !region &&
           nonce && (!peer_nonce_ || peer_nonce_ == nonce) &&
           echo == local_nonce_;
  }
  if (!peer_nonce_ || r.u32() != peer_nonce_ || r.u32() != local_nonce_) return false;
  switch (type) {
    // F4 uses memory UI only. No save transfer. Mod lists: bounded, only before playing.
    case kSaveReq: case kSaveData: return false;
    case kModReq:
      return (state == Link::kWaiting || state == Link::kConnected) && r.ok && r.o == pkt.size();
    case kModList: {
      if (state != Link::kWaiting && state != Link::kConnected) return false;
      const size_t first = r.u16(), total = r.u16(), n = r.u8();
      return r.ok && total <= kMaxMods && n <= kModsPerPacket && first + n <= total &&
             r.o + n * kModEntry == pkt.size();
    }
    case kTeam: {
      if (state != Link::kConnected && state != Link::kStarting) return false;
      const auto n = r.u8();
      if (n > 3) return false;
      if (n >= 2 && !safe_tagteam_) return false;
      std::vector<int> ids;
      for (int i = 0; i < n; ++i) { const auto id = r.u16(); if (id >= 64) return false; ids.push_back(id); }
      if (team_ready_ && ids != peer_team_) return false;
      return r.ok && r.o == pkt.size();
    }
    case kStart: {
      if (role_ != Role::kGuest || !mods_match_ || !team_ready_ ||
          (state != Link::kConnected && state != Link::kStarting)) return false;
      const auto seed = r.u32();
      const auto n = r.u16();
      if (!seed || n != kUiSnapSize || !r.need(n) || r.o + n != pkt.size()) return false;
      if (state == Link::kStarting && (seed != seed_ ||
          !std::equal(pkt.begin() + r.o, pkt.end(), ui_snap_.begin(), ui_snap_.end()))) return false;
      return true;
    }
    case kStartAck:
      return role_ == Role::kHost && state == Link::kStarting && r.o == pkt.size();
    case kInput: {
      if (state != Link::kPlaying && !(role_ == Role::kHost && state == Link::kStarting)) return false;
      const auto n = r.u8();
      if (!n || n > 8 || pkt.size() != 23u + 16u * n) return false;
      const uint32_t now = tick_.load();
      uint32_t prev = 0;
      for (int i = 0; i < n; ++i) {
        const auto t = r.u32();
        const auto pad = r.pad();
        // small window prevents poisoning another modulo slot; no rollover session.
        if (t == 0xFFFFFFFFu || (i && t <= prev) ||
            uint64_t(t) + 8 < now || uint64_t(t) > uint64_t(now) + 32) return false;
        const auto& slot = theirs_[t % kRing];
        if (slot.tick == t && (slot.pad.buttons != pad.buttons || slot.pad.lt != pad.lt ||
            slot.pad.rt != pad.rt || slot.pad.lx != pad.lx || slot.pad.ly != pad.ly ||
            slot.pad.rx != pad.rx || slot.pad.ry != pad.ry)) return false;
        prev = t;
      }
      const auto rt = r.u32(); r.u32();
      return r.ok && r.o == pkt.size() && (rt == 0xFFFFFFFFu ||
          (rt % 60 == 0 && uint64_t(rt) <= uint64_t(now) + 32 && uint64_t(rt) + 240 >= now));
    }
    case kBye: return r.ok && r.o == pkt.size();
    default: return false;
  }
}

// ---------------------------------------------------------------- socket
bool Session::OpenSocket(uint16_t port, std::string* error) {
#ifdef _WIN32
  static bool wsa = [] { WSADATA d; return WSAStartup(MAKEWORD(2, 2), &d) == 0; }();
  if (!wsa) { if (error) *error = "WSAStartup"; return false; }
#endif
  const auto s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#ifdef _WIN32
  if (s == INVALID_SOCKET) { if (error) *error = "socket"; return false; }
  u_long nb = 1;
  ioctlsocket(s, FIONBIO, &nb);
#else
  if (s < 0) { if (error) *error = "socket"; return false; }
  fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
#endif
  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_ANY);
  a.sin_port = htons(port);
  if (::bind(s, reinterpret_cast<sockaddr*>(&a), sizeof(a)) != 0) {
#ifdef _WIN32
    closesocket(s);
#else
    ::close(s);
#endif
    if (error) *error = "el puerto " + std::to_string(port) + " esta ocupado";
    return false;
  }
  sock_ = intptr_t(s);
  return true;
}

void Session::Send(const std::vector<uint8_t>& pkt) {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  if (sock_ < 0 || !have_peer_) return;
  auto wire = pkt;
  if (pkt.size() >= 6 && pkt[4] != kHello && pkt[4] != kHelloAck) {
    Writer ids(0); ids.u32(local_nonce_); ids.u32(peer_nonce_);
    wire.insert(wire.begin() + 6, ids.b.begin() + 6, ids.b.end());
  }
  ::sendto(decltype(::socket(0, 0, 0))(sock_), reinterpret_cast<const char*>(wire.data()), int(wire.size()), 0,
           reinterpret_cast<const sockaddr*>(peer_addr_), peer_len_);
  last_send_ms_ = NowMs();
}

bool Session::Recv(std::vector<uint8_t>& pkt) {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  if (sock_ < 0) return false;
  uint8_t buf[2048];
  sockaddr_in from{};
  socklen_t len = sizeof(from);
  const int n = int(::recvfrom(decltype(::socket(0, 0, 0))(sock_), reinterpret_cast<char*>(buf), sizeof(buf), 0,
                               reinterpret_cast<sockaddr*>(&from), &len));
  if (n < 0) return false;                       // nada pendiente
  pkt.clear();
  if (role_ == Role::kHost && IsStun(buf, n)) {   // respuesta STUN: nuestra IP publica y puerto
    uint32_t ip = 0;
    uint16_t port = 0;
    std::lock_guard<std::mutex> lock(mu_);
    if (stun_ready_ && std::memcmp(stun_addr_, &from, sizeof(stun_addr_)) == 0 &&
        StunMapped(buf, n, &ip, &port) && pub_how_ != "router") {
      pub_ip_ = ip;
      pub_port_ = port;
      pub_how_ = "stun";
    }
    return true;
  }
  if (n < 6 || std::memcmp(buf, kMagic, 4) != 0) return true;
  if (buf[5] != kProto) {
    if (have_peer_ && role_ == Role::kGuest &&
        std::memcmp(peer_addr_, &from, sizeof(from)) == 0)
      Fail(Link::kError, "protocolo incompatible (se requiere F4/v3)");
    return true;
  }
  std::vector<uint8_t> candidate(buf, buf + n);
  if (role_ == Role::kGuest && link_ == Link::kWaiting && have_peer_ &&
      std::memcmp(peer_addr_, &from, sizeof(from)) == 0) {
    Reader reply(candidate);
    if (buf[4] == kChallenge && n == 14 && reply.u32() == local_nonce_) {
      const auto challenge = reply.u32();
      if (challenge && (!peer_nonce_ || peer_nonce_ == challenge)) {
        peer_nonce_ = challenge;
        SendHello(kHello);
      }
      return true;
    }
    if (buf[4] == kReject && n == 10 && reply.u32() == local_nonce_) {
      Fail(Link::kError, "identidad/protocolo incompatible (F4/v3)");
      return true;
    }
  }
  // Stateless admission challenge: no endpoint claim on initial HELLO.
  // Random echo prevents stale HELLO capture, not user authentication.
  if (!have_peer_ && role_ == Role::kHost && link_ == Link::kWaiting &&
      buf[4] == kHello && n == 100) {
    Reader hello(candidate);
    const auto identity = hello.str(40), mods = hello.str(40);
    const auto delay = hello.u8(); const auto save = hello.u32(); const auto region = hello.u8();
    const auto nonce = hello.u32(), echo = hello.u32();
    if (!hello.ok || !nonce || delay < 1 || delay > 10 || save || region || !HexDigest(mods, 32)) return true;
    if (identity != identity_) {
      Writer reject(kReject); reject.u32(nonce);
      ::sendto(decltype(::socket(0, 0, 0))(sock_), reinterpret_cast<const char*>(reject.b.data()),
               int(reject.b.size()), 0, reinterpret_cast<const sockaddr*>(&from), len);
      return true;
    }
    if (echo != local_nonce_) {
      Writer challenge(kChallenge); challenge.u32(nonce); challenge.u32(local_nonce_);
      ::sendto(decltype(::socket(0, 0, 0))(sock_), reinterpret_cast<const char*>(challenge.b.data()),
               int(challenge.b.size()), 0, reinterpret_cast<const sockaddr*>(&from), len);
      return true;
    }
  }
  if (!ValidPacket(candidate)) return true;
  if (!have_peer_ && role_ == Role::kHost && buf[4] == kHello) {
    std::memcpy(peer_addr_, &from, sizeof(from));
    peer_len_ = int(len);
    have_peer_ = true;
  }
  if (!have_peer_ || std::memcmp(peer_addr_, &from, sizeof(from)) != 0) return true;
  pkt = std::move(candidate);
  last_recv_ms_ = NowMs();
  return true;
}

// ---------------------------------------------------------------- emparejamiento
bool Session::Host(uint16_t port, uint8_t delay, std::string* error) {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  if (guest_owner_) { if (error) *error = "sesion del juego aun activa"; return false; }
  Close();
  if (!identity_ready_ || !identity_admission_ || dbz3::g_guest_eu) {
    if (error) *error = "F4: falta identidad verificada US/seleccion segura";
    return false;
  }
  identity_admission_ = false;
  local_nonce_ = NewNonce();
  local_team_ = dbz3::tag::MyLastTeam();
  if (local_team_.size() >= 2 && !safe_tagteam_) {
    if (error) *error = "F4: Tag Team necesita prueba de aborto seguro/relevo identificado";
    return false;
  }
  if (local_team_.size() > 3 || std::any_of(local_team_.begin(), local_team_.end(), [](int x) { return x < 0 || x >= 64; })) {
    if (error) *error = "equipo fuera del contrato F4";
    return false;
  }
  role_ = Role::kHost;
  delay_ = uint8_t(std::clamp<int>(delay, 1, 10));
  if (!OpenSocket(port, error)) { role_ = Role::kNone; return false; }
  link_ = Link::kWaiting;
  last_recv_ms_ = NowMs();
  static const bool room_ok = RoomSelfTest();   // codigo de sala: ida y vuelta
  if (!room_ok) REXLOG_ERROR("dbz3 online: el codigo de sala no pasa su prueba");
  // Internet: el router abre el puerto (UPnP) y, si no puede, un servidor STUN dice la IP publica
  // (Poll). En un hilo: tarda segundos y no debe parar el juego. Solo da el codigo de sala; la
  // admision (identidad, nonces, mods) no cambia. Lo de una sesion ya cerrada se descarta (generation_).
  std::string local = LocalAddresses();
  local = local.substr(0, local.find(' '));
  const uint64_t gen = generation_;
  std::thread([this, port, local, gen] {
    sockaddr_in stun{};
    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    const bool resolved = getaddrinfo("stun.l.google.com", "19302", &hints, &res) == 0 && res;
    if (resolved) {
      std::memcpy(&stun, res->ai_addr, sizeof(stun));
      freeaddrinfo(res);
    }
    {
      std::lock_guard<std::recursive_mutex> io(io_mu_);
      std::lock_guard<std::mutex> lock(mu_);
      if (gen != generation_) return;
      if (resolved) {
        std::memcpy(stun_addr_, &stun, sizeof(stun));
        stun_ready_ = true;
      }
    }
    const std::string ext = UpnpOpen(port, local);
    uint32_t ip = 0;
    if (ext.empty() || inet_pton(AF_INET, ext.c_str(), &ip) != 1) return;
    UpnpCloseAtExit(port);
    std::lock_guard<std::recursive_mutex> io(io_mu_);
    std::lock_guard<std::mutex> lock(mu_);
    if (gen != generation_) {   // la sesion ya se cerro: el puerto se vuelve a cerrar
      std::thread(UpnpClose, port).detach();
      return;
    }
    pub_ip_ = ip;
    pub_port_ = upnp_port_ = port;
    pub_how_ = "router";
    REXLOG_INFO("dbz3 online: el router abrio el puerto {} (IP publica {})", port, ext);
  }).detach();
  return true;
}

bool Session::Join(const std::string& ip, uint16_t port, std::string* error) {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  if (guest_owner_) { if (error) *error = "sesion del juego aun activa"; return false; }
  Close();
  if (!identity_ready_ || !identity_admission_ || dbz3::g_guest_eu) {
    if (error) *error = "F4: falta identidad verificada US/seleccion segura";
    return false;
  }
  identity_admission_ = false;
  local_nonce_ = NewNonce();
  local_team_ = dbz3::tag::MyLastTeam();
  if (local_team_.size() >= 2 && !safe_tagteam_) {
    if (error) *error = "F4: Tag Team necesita prueba de aborto seguro/relevo identificado";
    return false;
  }
  if (local_team_.size() > 3 || std::any_of(local_team_.begin(), local_team_.end(), [](int x) { return x < 0 || x >= 64; })) {
    if (error) *error = "equipo fuera del contrato F4";
    return false;
  }
  role_ = Role::kGuest;
  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  uint32_t code_ip = 0;
  uint16_t code_port = 0;
  if (DecodeRoom(ip, &code_ip, &code_port)) {          // codigo de sala: IP publica y puerto dentro
    std::memcpy(&a.sin_addr, &code_ip, 4);
    a.sin_port = htons(code_port);
  } else if (inet_pton(AF_INET, ip.c_str(), &a.sin_addr) != 1) {
    if (error) *error = T::T("Codigo o IP no valido: ", "Invalid code or IP: ") + ip;
    role_ = Role::kNone;
    return false;
  }
  if (!OpenSocket(0, error)) { role_ = Role::kNone; return false; }
  std::memcpy(peer_addr_, &a, sizeof(a));
  peer_len_ = sizeof(a);
  have_peer_ = true;
  link_ = Link::kWaiting;
  last_recv_ms_ = NowMs();
  return true;
}

void Session::SendHello(uint8_t type) {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  if (my_build_.empty()) {
    my_build_ = identity_;
    try { my_mods_ = ModFingerprints(); }
    catch (const std::exception&) { Fail(Link::kError, "no se pudo verificar contenido"); return; }
    std::string acc;
    for (const auto& m : my_mods_) acc += m + "\n";
    my_mods_hash_ = rex::hash_bytes(acc);
    // UI snapshot only: never read/write/download the personal save.
    save_size_ = 0;
    save_ready_ = true;
  }
  Writer w(type);
  w.str(my_build_, 40);
  w.str(my_mods_hash_, 40);
  w.u8(delay_);
  w.u32(save_size_);
  w.u8(dbz3::g_guest_eu ? 1 : 0);
  w.u32(local_nonce_);
  w.u32(peer_nonce_);
  Send(w.b);
}

void Session::Handle(const std::vector<uint8_t>& pkt, bool) {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  if (!ValidPacket(pkt)) return;
  Reader r(pkt);
  const uint8_t type = pkt[4];
  if (type != kHello && type != kHelloAck) r.o += 8;
  switch (type) {
    case kHello:
    case kHelloAck: {
      const std::string build = r.str(40), mods = r.str(40);
      const uint8_t delay = r.u8();
      const uint32_t save_size = r.u32();
      const uint8_t region = r.u8();   // 1 = EU
      const uint32_t nonce = r.u32(), echo = r.u32();
      (void)echo;
      if (!r.ok) return;
      if (peer_nonce_ && peer_nonce_ != nonce) return;
      peer_nonce_ = nonce;
      if (type == kHello && role_ == Role::kHost) SendHello(kHelloAck);
      if (link_ == Link::kError) return;   // no se pudieron leer los mods propios: ese es el error a ensenar
      if (my_build_.empty()) SendHello(kHello);
      if (link_ == Link::kError) return;
      if (build != my_build_) {
        Fail(Link::kError, T::T("el otro PC tiene otra version del juego", "the other PC has another game version"));
        return;
      }
      if (region != uint8_t(dbz3::g_guest_eu ? 1 : 0)) {   // US y EU no son el mismo juego (semilla, datos)
        Fail(Link::kError, T::T("el otro PC tiene otra region del juego (US/EU)",
                                "the other PC has another game region (US/EU)"));
        return;
      }
      if (role_ == Role::kGuest) {
        delay_ = delay;
        if (save_size > (16u << 20)) {   // una partida guardada no pasa de unos KB: paquete roto o hostil
          Fail(Link::kError, T::T("datos del otro PC no validos", "invalid data from the other PC"));
          return;
        }
        if (save_size_ != save_size || save_have_.empty()) {
          save_size_ = save_size;
          save_.assign(save_size, 0);
          save_have_.assign((save_size + kChunk - 1) / kChunk, 0);
          save_ready_ = save_size == 0;
        }
      }
      // Mods distintos: se queda conectado SIN poder empezar (RequestStart, kStart y TakeStart exigen
      // mods_match_) y se cruzan las listas para decir cuales faltan y ofrecer bajarlos de Nova's Mods.
      const bool was = mods_match_.exchange(mods == my_mods_hash_);
      if (!mods_match_ && (was || link_ == Link::kWaiting))
        REXLOG_INFO("dbz3 online: mods distintos; se cruzan las listas");
      if (link_ == Link::kWaiting) link_ = Link::kConnected;
      break;
    }
    case kModReq: {   // lista troceada (kModsPerPacket por paquete)
      if (my_mods_.size() > kMaxMods) break;
      for (size_t i = 0; i < my_mods_.size() || i == 0; i += kModsPerPacket) {
        Writer w(kModList);
        w.u16(uint16_t(i));
        w.u16(uint16_t(my_mods_.size()));
        const size_t n = std::min(kModsPerPacket, my_mods_.size() - std::min(i, my_mods_.size()));
        w.u8(uint8_t(n));
        for (size_t k = 0; k < n; ++k) w.str(my_mods_[i + k], kModEntry);
        Send(w.b);
        if (my_mods_.empty()) break;
      }
      break;
    }
    case kModList: {
      const uint16_t first = r.u16(), total = r.u16();
      const uint8_t n = r.u8();
      std::lock_guard<std::mutex> lock(mu_);
      if (!peer_mods_known_ || peer_mods_.size() != total) peer_mods_.assign(total, {});
      peer_mods_known_ = true;
      for (uint8_t k = 0; k < n; ++k) peer_mods_[first + k] = r.str(kModEntry);
      break;
    }
    case kSaveReq: {
      const uint32_t idx = r.u32();
      if (!r.ok || role_ != Role::kHost || size_t(idx) * kChunk >= save_.size()) return;
      const uint32_t n = std::min<uint32_t>(kChunk, uint32_t(save_.size() - size_t(idx) * kChunk));
      Writer w(kSaveData);
      w.u32(idx);
      w.u32(n);
      w.bytes(save_.data() + size_t(idx) * kChunk, n);
      Send(w.b);
      break;
    }
    case kSaveData: {
      const uint32_t idx = r.u32(), n = r.u32();
      if (!r.need(n) || role_ != Role::kGuest || idx >= save_have_.size() ||
          size_t(idx) * kChunk + n > save_.size()) return;
      std::memcpy(save_.data() + size_t(idx) * kChunk, &pkt[r.o], n);
      save_have_[idx] = 1;
      save_ready_ = std::all_of(save_have_.begin(), save_have_.end(), [](uint8_t v) { return v != 0; });
      break;
    }
    case kStart:   // semilla + estado de menu del anfitrion
      if (role_ == Role::kGuest && link_ != Link::kPlaying) {
        const uint32_t seed = r.u32();
        const uint16_t n = r.u16();
        if (!r.need(n)) return;
        Send(Writer(kStartAck).b);
        if (link_ != Link::kStarting) {
          seed_ = seed;
          std::lock_guard<std::mutex> lock(mu_);
          ui_snap_.assign(pkt.begin() + long(r.o), pkt.begin() + long(r.o + n));
          link_ = Link::kStarting;
          launch_ = true;
        }
      }
      break;
    case kStartAck:
      if (role_ == Role::kHost && link_ == Link::kStarting) launch_ = true;
      break;
    case kInput: {
      const uint8_t n = r.u8();
      for (uint8_t k = 0; k < n; ++k) {
        const uint32_t t = r.u32();
        const Pad p = r.pad();
        if (!r.ok) return;
        Slot& s = theirs_[t % kRing];
        if (s.tick == 0xFFFFFFFFu || t > s.tick) s = {t, p};
      }
      const uint32_t rt = r.u32(), rv = r.u32();
      if (r.ok && rt != 0xFFFFFFFFu) {
        const int i = int((rt / 60) % kRing);
        their_rng_tick_[i] = rt;
        their_rng_[i] = rv;
        if (my_rng_tick_[i] == rt && my_rng_[i] != rv && !desync_.exchange(true)) {
          REXLOG_WARN("dbz3 online: DESINCRONIA en el tick {} (rng {:08X} / {:08X})", rt, my_rng_[i], rv);
          Fail(Link::kError, "desincronia RNG: sesion abortada");
        }
      }
      if (role_ == Role::kHost && link_ == Link::kStarting) launch_ = true;   // el invitado ya juega
      break;
    }
    case kTeam: {   // su equipo de Tag Team (se aplica al empezar, TakeStart)
      const uint8_t n = r.u8();
      std::vector<int> ids;
      for (uint8_t i = 0; i < n && i < 3 && r.need(2); ++i) {
        const int id = int(r.u16());
        if (id < 64) ids.push_back(id);   // IDs del juego 0-43 y extra 44-63: otro valor se saldria de la tabla
      }
      std::lock_guard<std::mutex> lock(mu_);
      if (team_ready_ && peer_team_ != ids) return; // frozen agreement
      peer_team_ = ids;
      team_ready_ = true;
      break;
    }
    case kBye:
      Fail(Link::kLost, T::T("el otro jugador se ha ido", "the other player left"));
      break;
    default:
      break;
  }
}

void Session::Poll() {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  if (guest_owner_ || link_ == Link::kIdle || link_ == Link::kPlaying || link_ == Link::kError || link_ == Link::kLost) return;
  std::vector<uint8_t> pkt;
  for (int i = 0; i < 256 && Recv(pkt); ++i) {
    if (!pkt.empty()) Handle(pkt, true);
    pkt.clear();
  }
  const uint64_t now = NowMs();
  if (role_ == Role::kGuest && link_ == Link::kWaiting && now - last_send_ms_ > 300) SendHello(kHello);
  // anfitrion sin UPnP: pregunta su IP publica al STUN desde el mismo socket (asi el puerto que ve
  // el STUN es el que vera el rival)
  if (role_ == Role::kHost && link_ == Link::kWaiting && stun_tries_ < 5 && now - stun_sent_ms_ > 1500) {
    sockaddr_in stun{};
    bool ask = false;
    {
      std::lock_guard<std::mutex> lock(mu_);
      ask = stun_ready_ && !pub_ip_;
      if (ask) std::memcpy(&stun, stun_addr_, sizeof(stun));
    }
    if (ask) {
      const auto req = StunRequest();
      ::sendto(decltype(::socket(0, 0, 0))(sock_), reinterpret_cast<const char*>(req.data()), int(req.size()), 0,
               reinterpret_cast<const sockaddr*>(&stun), sizeof(stun));
      stun_sent_ms_ = now;
      ++stun_tries_;
    }
  }
  if (link_ == Link::kConnected || link_ == Link::kStarting) {
    if (role_ == Role::kGuest && !save_ready_ && now - last_send_ms_ > 30) {
      int asked = 0;   // pide hasta 8 trozos que falten
      for (uint32_t i = 0; i < save_have_.size() && asked < 8; ++i) {
        if (!save_have_[i]) { Writer w(kSaveReq); w.u32(i); Send(w.b); ++asked; }
      }
    }
    if (role_ == Role::kHost && link_ == Link::kStarting && !launch_ && now - last_send_ms_ > 200) {
      std::lock_guard<std::mutex> lock(mu_);
      Send(StartPacket(seed_, ui_snap_));
    }
    if (link_ == Link::kConnected && !mods_match_ && now - mod_req_ms_ > 1000) {
      std::vector<ModDelta> diff;
      if (!ModDiff(diff)) {   // su lista aun no esta entera: se pide otra vez (UDP)
        mod_req_ms_ = now;
        Send(Writer(kModReq).b);
      }
    }
    if (now - team_sent_ms_ > 500) {   // mi equipo de Tag Team, cada medio segundo (UDP)
      team_sent_ms_ = now;
      const std::vector<int>& team = local_team_;
      Writer w(kTeam);
      w.u8(uint8_t(std::min<size_t>(team.size(), 3)));
      for (size_t i = 0; i < team.size() && i < 3; ++i) w.u16(uint16_t(team[i]));
      Send(w.b);
    }
    if (now - last_recv_ms_ > kLostMs) Fail(Link::kLost, T::T("sin respuesta del otro PC", "no reply from the other PC"));
  }
  if (link_ == Link::kWaiting && role_ == Role::kGuest && now - last_recv_ms_ > 30000) {
    Fail(Link::kError, T::T("no contesta: revisa el codigo, el puerto y el cortafuegos del anfitrion (en la misma casa, usa su IP local)",
                            "no answer: check the code, the port and the host's firewall (in the same house, use their local IP)"));
  }
}

void Session::SetUiSnapshot(const uint8_t* p, size_t n) {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  std::lock_guard<std::mutex> lock(mu_);
  if (link_ != Link::kStarting && p && n == kUiSnapSize) ui_snap_.assign(p, p + n);
}

namespace {
std::vector<uint8_t> StartPacket(uint32_t seed, const std::vector<uint8_t>& snap) {
  Writer w(kStart);
  w.u32(seed);
  w.u16(uint16_t(snap.size()));
  w.bytes(snap.data(), snap.size());
  return w.b;
}
}  // namespace

void Session::RequestStart() {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  if (role_ != Role::kHost || link_ != Link::kConnected || !mods_match_ || !team_ready_ || ui_snap_.size() != kUiSnapSize) return;
  link_ = Link::kStarting;
  start_sent_ = true;
  seed_ = uint32_t(NowMs() * 2654435761u) | 1u;
  std::lock_guard<std::mutex> lock(mu_);
  Send(StartPacket(seed_, ui_snap_));
}

void Session::ReseedBattle(uint8_t* base) {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  if (!active() || !base || dbz3::g_guest_eu) return;
  const uint32_t v = (seed_ ^ (++battles_ * 0x9E3779B9u)) | 1u;   // nunca 0 (el LFSR se quedaria)
  const uint8_t be[4] = {uint8_t(v >> 24), uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v)};
  std::memcpy(base + 0x82310670u, be, 4);
  REXLOG_INFO("dbz3 online: combate {}: semilla del RNG igualada", battles_);
}

void Session::BattleBarrier() {
  if (!active()) return;
  // bit propio en el mando (0x0400 anfitrion, 0x0800 invitado; XInput no los usa): los dos PCs ven
  // los dos bits en el mismo tick aplicado y salen a la vez
  const uint16_t mine = role_ == Role::kHost ? 0x0400 : 0x0800;
  const uint32_t from = tick_;
  const auto deadline = NowMs() + 60000;
  for (int i = 0; i < 60 * 60 && active() && NowMs() < deadline; ++i) {   // bounded both by ticks and monotonic time
    Pad p{};
    p.buttons = mine;
    RunTick(p, 0);
    if ((applied_[0].buttons & 0x0400) && (applied_[1].buttons & 0x0800)) {
      REXLOG_INFO("dbz3 online: combate alineado en el tick {} ({} ticks de espera)", tick_.load(), tick_.load() - from);
      return;
    }
  }
  if (active()) Fail(Link::kLost, "plazo real de barrera agotado");
  REXLOG_WARN("dbz3 online: el otro PC no llego al combate");
}

bool Session::TakeStart(uint8_t* base) {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  if (link_ != Link::kStarting || !mods_match_ || !team_ready_ ||
      ui_snap_.size() != kUiSnapSize || !base || !launch_.exchange(false)) return false;
  guest_owner_ = true; // explicit handoff under io_mu_; Poll now excluded
  {
    std::lock_guard<std::mutex> lock(mu_);
    // invitado: desbloqueos, ultima eleccion... del anfitrion (misma rueda, mismos cursores)
    if (role_ == Role::kGuest && base && !ui_snap_.empty()) {
      std::memcpy(base + dbz3::GuestAddr(kUiSnap, kUiSnapEu), ui_snap_.data(),
                  std::min<size_t>(ui_snap_.size(), kUiSnapSize));
    }
  }
  if (base && !dbz3::g_guest_eu) {   // misma semilla del RNG del juego (LFSR, US 0x82310670)
    const uint8_t be[4] = {uint8_t(seed_ >> 24), uint8_t(seed_ >> 16), uint8_t(seed_ >> 8), uint8_t(seed_)};
    std::memcpy(base + 0x82310670u, be, 4);
  }
  {   // Tag Team: lado 1 el anfitrion, lado 2 el invitado, igual en los dos PCs (sin COM)
    std::vector<int> mine = local_team_, peer;
    { std::lock_guard<std::mutex> lock(mu_); peer = peer_team_; }
    dbz3::tag::SetDynamicTeam(0, role_ == Role::kHost ? mine : peer, false);   // el invitado no hace suyo el del anfitrion
    dbz3::tag::SetDynamicTeam(1, role_ == Role::kHost ? peer : mine, false);
    dbz3::tag::SetComSide(0, false);
    dbz3::tag::SetComSide(1, false);
    REXLOG_INFO("dbz3 online: equipos de Tag Team {} y {} luchadores", mine.size(), peer.size());
  }
  tick_ = 0;
  battles_ = 0;
  for (auto& s : mine_) s.tick = 0xFFFFFFFFu;
  for (auto& s : theirs_) s.tick = 0xFFFFFFFFu;
  my_rng_tick_.fill(0xFFFFFFFFu);
  their_rng_tick_.fill(0xFFFFFFFFu);
  link_ = Link::kPlaying;
  last_recv_ms_ = NowMs();
  REXLOG_INFO("dbz3 online: empieza la partida ({} , retardo {})", role_ == Role::kHost ? "anfitrion" : "invitado",
              delay_.load());
  return true;
}

void Session::Close() {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  if (guest_owner_) { leave_req_ = true; return; }
  ResetTransport();
}

void Session::ResetTransport() {
  if (sock_ >= 0) {
    if (have_peer_) Send(Writer(kBye).b);
#ifdef _WIN32
    closesocket(SOCKET(sock_));
#else
    ::close(int(sock_));
#endif
  }
  sock_ = -1;
  have_peer_ = false;
  link_ = Link::kIdle;
  dbz3::tag::ClearDynamicTeams();   // los equipos de la partida online no pasan a los combates normales
  {
    std::lock_guard<std::mutex> lock(mu_);
    if (upnp_port_) std::thread(UpnpClose, upnp_port_).detach();   // se cierra el puerto del router
    pub_ip_ = 0;
    pub_port_ = upnp_port_ = 0;
    pub_how_.clear();
  }
  stun_sent_ms_ = 0;
  stun_tries_ = 0;
  role_ = Role::kNone;
  mods_match_ = false;
  save_ready_ = false;
  save_.clear();
  save_have_.clear();
  save_size_ = 0;
  start_sent_ = false;
  launch_ = false;
  leave_req_ = false;
  // sin ticks propios todavia: el rng que mande el rival antes de empezar aqui no se compara
  my_rng_tick_.fill(0xFFFFFFFFu);
  their_rng_tick_.fill(0xFFFFFFFFu);
  desync_ = false;
  my_build_.clear();
  ++generation_;
  guest_owner_ = false;
  local_nonce_ = peer_nonce_ = 0;
  last_send_ms_ = last_recv_ms_ = team_sent_ms_ = 0;
  peer_len_ = 0;
  std::memset(peer_addr_, 0, sizeof(peer_addr_));
  local_team_.clear(); peer_team_.clear(); ui_snap_.clear();
  team_ready_ = false;
  tick_ = 0; battles_ = seed_ = 0; armed_ = false;
  applied_[0] = {}; applied_[1] = {};
  dbz3::tag::g_net_local = 0;
  dbz3::tag::NetApplied(0, 0); dbz3::tag::NetApplied(1, 0);
  for (auto& s : mine_) s = {};
  for (auto& s : theirs_) s = {};
  mod_req_ms_ = 0;
  std::lock_guard<std::mutex> lock(mu_);
  peer_mods_.clear();
  peer_mods_known_ = false;
  error_.clear();
}

void Session::Fail(Link l, const std::string& why) {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  guest_owner_ = false;
  ResetTransport();
  std::lock_guard<std::mutex> lock(mu_);
  error_ = why;
  link_ = l;
}

std::string Session::Status() const {
  std::lock_guard<std::mutex> lock(mu_);
  switch (link_.load()) {
    case Link::kIdle: return "";
    case Link::kWaiting: return role_ == Role::kHost ? T::T("esperando al rival...", "waiting for the other player...") : T::T("llamando al anfitrion...", "calling the host...");
    case Link::kConnected: return T::T("conectados", "connected");
    case Link::kStarting: return T::T("empezando...", "starting...");
    case Link::kPlaying: return T::T("jugando", "playing");
    case Link::kLost: return T::T("conexion perdida: ", "connection lost: ") + error_;
    case Link::kError: return "error: " + error_;
  }
  return "";
}

bool Session::ModDiff(std::vector<ModDelta>& out) const {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  std::lock_guard<std::mutex> lock(mu_);
  out.clear();
  if (!peer_mods_known_ ||
      std::any_of(peer_mods_.begin(), peer_mods_.end(), [](const std::string& m) { return m.empty(); }))
    return false;
  // "carpeta|hash": por carpeta, para distinguir "te falta" de "otra version"
  auto split = [](const std::vector<std::string>& v) {
    std::map<std::string, std::string> m;
    for (const auto& s : v) m[s.substr(0, s.find('|'))] = s;
    return m;
  };
  const auto mine = split(my_mods_), theirs = split(peer_mods_);
  for (const auto& [name, full] : theirs) {
    const auto it = mine.find(name);
    if (it == mine.end()) out.push_back({ModDelta::kMissing, name});
    else if (it->second != full) out.push_back({ModDelta::kDifferent, name});
  }
  for (const auto& [name, full] : mine) {
    if (!theirs.count(name)) out.push_back({ModDelta::kExtra, name});
  }
  return true;
}

std::string Session::RoomCode(std::string* how) const {
  std::lock_guard<std::recursive_mutex> io(io_mu_);
  {
    std::lock_guard<std::mutex> lock(mu_);
    if (pub_ip_) {
      if (how) *how = pub_how_;
      return EncodeRoom(pub_ip_, pub_port_);
    }
  }
  if (how) *how = "local";
  std::string local = LocalAddresses();
  local = local.substr(0, local.find(' '));
  uint32_t ip = 0;
  sockaddr_in a{};
  socklen_t len = sizeof(a);
  if (local.empty() || inet_pton(AF_INET, local.c_str(), &ip) != 1 || sock_ < 0 ||
      getsockname(decltype(::socket(0, 0, 0))(sock_), reinterpret_cast<sockaddr*>(&a), &len) != 0)
    return {};
  return EncodeRoom(ip, ntohs(a.sin_port));
}

std::string Session::LocalAddresses() const {
  std::string out;
#ifdef _WIN32
  char host[256] = {};
  if (gethostname(host, sizeof(host)) != 0) return out;
  addrinfo hints{}, *res = nullptr;
  hints.ai_family = AF_INET;
  if (getaddrinfo(host, nullptr, &hints, &res) != 0) return out;
  for (auto* p = res; p; p = p->ai_next) {
    char ip[64] = {};
    inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(p->ai_addr)->sin_addr, ip, sizeof(ip));
    if (out.find(ip) == std::string::npos) out += (out.empty() ? "" : "  ") + std::string(ip);
  }
  freeaddrinfo(res);
#else
  ifaddrs* ifs = nullptr;
  if (getifaddrs(&ifs) != 0) return out;
  for (auto* p = ifs; p; p = p->ifa_next) {
    if (!p->ifa_addr || p->ifa_addr->sa_family != AF_INET) continue;
    char ip[64] = {};
    inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(p->ifa_addr)->sin_addr, ip, sizeof(ip));
    if (std::string(ip) != "127.0.0.1") out += (out.empty() ? "" : "  ") + std::string(ip);
  }
  freeifaddrs(ifs);
#endif
  return out;
}

// ---------------------------------------------------------------- partida guardada
void Session::ApplySave() {
  if (save_.empty()) return;
  fs::path dst = SavePath();
  std::error_code ec;
  if (dst.empty()) {
    REXLOG_WARN("dbz3 online: este PC no tiene partida guardada; se juega sin la del anfitrion");
    return;
  }
  const fs::path bak = BackupDir();
  fs::create_directories(bak, ec);
  if (!fs::exists(bak / "pendiente.txt", ec)) {   // la copia buena es la primera (la del jugador)
    fs::copy_file(dst, bak / "data.bin", fs::copy_options::overwrite_existing, ec);
    std::ofstream(bak / "pendiente.txt") << rex::path_to_utf8(dst) << "\n";
  }
  std::ofstream(dst, std::ios::binary).write(reinterpret_cast<const char*>(save_.data()), std::streamsize(save_.size()));
  REXLOG_INFO("dbz3 online: partida del anfitrion puesta ({} bytes); la tuya en {}", save_.size(),
              rex::path_to_utf8(bak));
}

void Session::RestoreSave() {
  const fs::path bak = BackupDir();
  std::error_code ec;
  if (!fs::exists(bak / "pendiente.txt", ec) || !fs::exists(bak / "data.bin", ec)) return;
  std::string dst;
  std::getline(std::ifstream(bak / "pendiente.txt"), dst);
  if (dst.empty()) return;
  fs::copy_file(bak / "data.bin", rex::to_path(dst), fs::copy_options::overwrite_existing, ec);
  if (ec) {
    REXLOG_WARN("dbz3 online: no pude devolver tu partida guardada ({}); sigue en {}", ec.message(),
                rex::path_to_utf8(bak));
    return;
  }
  // la copia se queda (nunca se borra): solo se marca como devuelta
  const auto stamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
  fs::rename(bak / "pendiente.txt", bak / ("devuelta_" + stamp + ".txt"), ec);
  REXLOG_INFO("dbz3 online: tu partida guardada ha vuelto a su sitio");
}

// ---------------------------------------------------------------- lockstep
void Session::SendInputs() {
  Writer w(kInput);
  const uint32_t last = tick_ + delay_;
  const uint32_t first = last >= 7 ? last - 7 : 0;
  std::vector<const Slot*> v;
  for (uint32_t t = first; t <= last; ++t) {
    const Slot& s = mine_[t % kRing];
    if (s.tick == t) v.push_back(&s);
  }
  w.u8(uint8_t(v.size()));
  for (const Slot* s : v) { w.u32(s->tick); w.pad(s->pad); }
  const int i = int((tick_ / 60) % kRing);
  w.u32(my_rng_tick_[i]);
  w.u32(my_rng_[i]);
  Send(w.b);
}

void Session::RunTick(const Pad& local, uint32_t rng) {
  armed_ = false;
  if (!active()) return;
  if (leave_req_.exchange(false)) {   // salir pedido desde la pantalla: lo cierra el hilo del socket
    std::lock_guard<std::recursive_mutex> io(io_mu_);
    guest_owner_ = false;
    ResetTransport();
    REXLOG_INFO("dbz3 online: sesion cerrada por el jugador");
    return;
  }
  const uint32_t t = tick_;
  mine_[(t + delay_) % kRing] = {t + delay_, local};
  if (t % 60 == 0 && rng) {
    my_rng_tick_[(t / 60) % kRing] = t;
    my_rng_[(t / 60) % kRing] = rng;
  }
  SendInputs();
  const Pad mine = t < delay_ ? Pad{} : mine_[t % kRing].pad;
  Pad theirs{};
  if (t >= delay_) {
    std::vector<uint8_t> pkt;
    uint64_t start = NowMs();
    while (theirs_[t % kRing].tick != t) {
      if (leave_req_.load()) {
        std::lock_guard<std::recursive_mutex> io(io_mu_);
        guest_owner_ = false; ResetTransport(); return;
      }
      bool got = false;
      for (int batch = 0; batch < 64 && Recv(pkt); ++batch) {
        if (!pkt.empty()) { Handle(pkt, true); got = true; }
        if (!active()) return;   // BYE u otro fallo: sin esperar 10 s a que caduque
        pkt.clear();
      }
      if (theirs_[t % kRing].tick == t) break;
      const uint64_t now = NowMs();
      if (now - last_send_ms_ > 15) SendInputs();
      // el primer tick espera a que el otro PC termine de arrancar
      if (now - start > (t <= delay_ ? 30000 : kLostMs)) {
        Fail(Link::kLost, T::T("sin respuesta del otro PC", "no reply from the other PC"));
        REXLOG_WARN("dbz3 online: conexion perdida en el tick {}", t);
        return;
      }
      if (!got) std::this_thread::sleep_for(std::chrono::milliseconds(1));
      (void)start;
    }
    theirs = theirs_[t % kRing].pad;
  }
  // usuario 0 = anfitrion, usuario 1 = invitado (en los dos PCs)
  applied_[role_ == Role::kHost ? 0 : 1] = mine;
  applied_[role_ == Role::kHost ? 1 : 0] = theirs;
  ++tick_;
}

}  // namespace dbz3::net
