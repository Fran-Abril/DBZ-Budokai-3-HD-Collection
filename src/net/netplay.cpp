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
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
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

namespace dbz3::net {
namespace {

namespace fs = std::filesystem;
namespace T = dbz3::i18n;

constexpr uint8_t kProto = 2;   // 2: HELLO lleva la region (US/EU); los PCs con 1 se ignoran
constexpr char kMagic[4] = {'D', 'B', 'Z', '3'};
enum Msg : uint8_t {
  kHello = 1, kHelloAck, kModReq, kModList, kSaveReq, kSaveData, kStart, kStartAck, kInput, kBye,
  kTeam   // equipo de Tag Team de quien lo manda (n, n ids u16); al final: los PCs viejos lo ignoran
};
constexpr uint32_t kChunk = 1024;
constexpr uint64_t kLostMs = 10000;
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
  bool need(size_t n) { ok = ok && o + n <= b.size(); return ok; }
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
// Fuera: _roster (se genera igual en los dos PCs a partir de lo demas), vistas previas y packs
// de texturas (solo cambian el aspecto, no la simulacion).
std::map<std::string, std::string>& HashCache() {
  static std::map<std::string, std::string> c;
  return c;
}

std::string FileHash(const fs::path& p) {
  std::error_code ec;
  const auto size = fs::file_size(p, ec);
  const auto t = fs::last_write_time(p, ec).time_since_epoch().count();
  const std::string key = rex::path_to_utf8(p) + "|" + std::to_string(size) + "|" + std::to_string(static_cast<long long>(t));
  auto& c = HashCache();
  auto it = c.find(key);
  if (it != c.end()) return it->second;
  return c[key] = rex::hash_file(p);
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
    if (name == "_roster" || name.ends_with(".disabled") || fs::exists(it->path() / ".disabled", ec)) continue;
    if (settings::IsTexturePackMod(rex::path_to_utf8(it->path()))) continue;
    mods.push_back(it->path());
  }
  std::sort(mods.begin(), mods.end());
  for (const auto& m : mods) {
    std::vector<std::pair<std::string, fs::path>> files;
    for (auto [it, iec] = std::pair{fs::recursive_directory_iterator(m, ec), std::error_code{}};
         !iec && it != fs::recursive_directory_iterator(); it.increment(iec)) {
      if (!it->is_regular_file(ec)) continue;
      const std::string rel = rex::path_to_utf8(fs::relative(it->path(), m, ec));
      if (rel.find("_vista") != std::string::npos) continue;
      files.emplace_back(rel, it->path());
    }
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
  if (sock_ < 0 || !have_peer_) return;
  ::sendto(decltype(::socket(0, 0, 0))(sock_), reinterpret_cast<const char*>(pkt.data()), int(pkt.size()), 0,
           reinterpret_cast<const sockaddr*>(peer_addr_), peer_len_);
  last_send_ms_ = NowMs();
}

bool Session::Recv(std::vector<uint8_t>& pkt) {
  if (sock_ < 0) return false;
  uint8_t buf[2048];
  sockaddr_in from{};
  socklen_t len = sizeof(from);
  const int n = int(::recvfrom(decltype(::socket(0, 0, 0))(sock_), reinterpret_cast<char*>(buf), sizeof(buf), 0,
                               reinterpret_cast<sockaddr*>(&from), &len));
  if (n < 0) return false;                       // nada pendiente
  pkt.clear();
  if (IsStun(buf, n)) {                          // respuesta STUN: nuestra IP publica y puerto
    uint32_t ip = 0;
    uint16_t port = 0;
    if (StunMapped(buf, n, &ip, &port)) {
      std::lock_guard<std::mutex> lock(mu_);
      if (pub_how_ != "router") {
        pub_ip_ = ip;
        pub_port_ = port;
        pub_how_ = "stun";
      }
    }
    return true;
  }
  if (n < 6 || std::memcmp(buf, kMagic, 4) != 0 || buf[5] != kProto) return true;   // basura: se ignora
  if (!have_peer_ && role_ == Role::kHost && buf[4] == kHello) {
    std::memcpy(peer_addr_, &from, sizeof(from));   // el primero que llama es el rival
    peer_len_ = int(len);
    have_peer_ = true;
  }
  if (!have_peer_ || std::memcmp(peer_addr_, &from, sizeof(from)) != 0) return true;   // otro: se ignora
  pkt.assign(buf, buf + n);
  last_recv_ms_ = NowMs();
  return true;
}

// ---------------------------------------------------------------- emparejamiento
bool Session::Host(uint16_t port, uint8_t delay, std::string* error) {
  Close();
  role_ = Role::kHost;
  delay_ = uint8_t(std::clamp<int>(delay, 1, 10));
  if (!OpenSocket(port, error)) { role_ = Role::kNone; return false; }
  link_ = Link::kWaiting;
  last_recv_ms_ = NowMs();
  static const bool room_ok = RoomSelfTest();   // codigo de sala: ida y vuelta
  if (!room_ok) REXLOG_ERROR("dbz3 online: el codigo de sala no pasa su prueba");
  // el router abre el puerto solo (UPnP) en un hilo: tarda unos segundos y no debe parar el juego
  std::string local = LocalAddresses();
  local = local.substr(0, local.find(' '));
  std::thread([this, port, local] {
    const std::string ext = UpnpOpen(port, local);
    uint32_t ip = 0;
    if (ext.empty() || inet_pton(AF_INET, ext.c_str(), &ip) != 1) return;
    std::lock_guard<std::mutex> lock(mu_);
    pub_ip_ = ip;
    pub_port_ = port;
    pub_how_ = "router";
    upnp_port_ = port;
    UpnpCloseAtExit(port);
    REXLOG_INFO("dbz3 online: el router abrio el puerto {} (IP publica {})", port, ext);
  }).detach();
  return true;
}

bool Session::Join(const std::string& ip, uint16_t port, std::string* error) {
  Close();
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
  if (my_build_.empty()) {
    my_build_ = std::string(__DATE__ " " __TIME__);
    my_mods_ = ModFingerprints();
    std::string acc;
    for (const auto& m : my_mods_) acc += m + "\n";
    my_mods_hash_ = rex::hash_bytes(acc);
    if (role_ == Role::kHost) {
      const fs::path sp = SavePath();
      save_ = sp.empty() ? std::vector<uint8_t>{} : ReadFile(sp);
      save_size_ = uint32_t(save_.size());
      save_ready_ = true;
    }
  }
  Writer w(type);
  w.str(my_build_, 32);
  w.str(my_mods_hash_, 40);
  w.u8(delay_);
  w.u32(save_size_);
  w.u8(dbz3::g_guest_eu ? 1 : 0);
  Send(w.b);
}

void Session::Handle(const std::vector<uint8_t>& pkt, bool) {
  Reader r(pkt);
  const uint8_t type = pkt[4];
  switch (type) {
    case kHello:
    case kHelloAck: {
      const std::string build = r.str(32), mods = r.str(40);
      const uint8_t delay = r.u8();
      const uint32_t save_size = r.u32();
      const uint8_t region = r.u8();   // 1 = EU
      if (!r.ok) return;
      if (type == kHello && role_ == Role::kHost) SendHello(kHelloAck);
      if (my_build_.empty()) SendHello(kHello);
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
      mods_match_ = mods == my_mods_hash_;
      if (!mods_match_ && peer_mods_.empty()) Send(Writer(kModReq).b);
      if (link_ == Link::kWaiting) link_ = Link::kConnected;
      break;
    }
    case kModReq: {   // lista troceada (20 por paquete)
      for (size_t i = 0; i < my_mods_.size() || i == 0; i += 20) {
        Writer w(kModList);
        w.u16(uint16_t(i));
        w.u16(uint16_t(my_mods_.size()));
        const size_t n = std::min<size_t>(20, my_mods_.size() - std::min(i, my_mods_.size()));
        w.u8(uint8_t(n));
        for (size_t k = 0; k < n; ++k) w.str(my_mods_[i + k], 64);
        Send(w.b);
        if (my_mods_.empty()) break;
      }
      break;
    }
    case kModList: {
      const uint16_t first = r.u16(), total = r.u16();
      const uint8_t n = r.u8();
      std::lock_guard<std::mutex> lock(mu_);
      if (peer_mods_.size() != total) peer_mods_.assign(total, {});
      for (uint8_t k = 0; k < n && first + k < total; ++k) peer_mods_[first + k] = r.str(64);
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
      peer_team_ = ids;
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
  if (link_ == Link::kIdle || link_ == Link::kPlaying) return;
  std::vector<uint8_t> pkt;
  for (int i = 0; i < 256 && Recv(pkt); ++i) {
    if (!pkt.empty()) Handle(pkt, true);
    pkt.clear();
  }
  const uint64_t now = NowMs();
  if (role_ == Role::kGuest && link_ == Link::kWaiting && now - last_send_ms_ > 300) SendHello(kHello);
  // anfitrion sin UPnP: pregunta su IP publica a un servidor STUN (desde el mismo socket, asi el
  // puerto que ve el STUN es el que vera el rival)
  if (role_ == Role::kHost && link_ == Link::kWaiting && stun_tries_ < 5 && now - stun_sent_ms_ > 1500) {
    bool known;
    {
      std::lock_guard<std::mutex> lock(mu_);
      known = pub_ip_ != 0;
    }
    if (!known) {
      static sockaddr_in stun{};
      static const bool resolved = [] {
        addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        if (getaddrinfo("stun.l.google.com", "19302", &hints, &res) != 0 || !res) return false;
        std::memcpy(&stun, res->ai_addr, sizeof(stun));
        freeaddrinfo(res);
        return true;
      }();
      if (resolved) {
        const auto req = StunRequest();
        ::sendto(decltype(::socket(0, 0, 0))(sock_), reinterpret_cast<const char*>(req.data()), int(req.size()), 0,
                 reinterpret_cast<const sockaddr*>(&stun), sizeof(stun));
      }
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
    if (now - team_sent_ms_ > 500) {   // mi equipo de Tag Team, cada medio segundo (UDP)
      team_sent_ms_ = now;
      const std::vector<int> team = dbz3::tag::MyLastTeam();
      Writer w(kTeam);
      w.u8(uint8_t(std::min<size_t>(team.size(), 3)));
      for (size_t i = 0; i < team.size() && i < 3; ++i) w.u16(uint16_t(team[i]));
      Send(w.b);
    }
    if (now - last_recv_ms_ > kLostMs) Fail(Link::kLost, T::T("sin respuesta del otro PC", "no reply from the other PC"));
  }
  if (link_ == Link::kWaiting && role_ == Role::kGuest && now - last_recv_ms_ > 30000) {
    Fail(Link::kError, T::T("no contesta: revisa la IP, el puerto y el cortafuegos del anfitrion", "no answer: check the IP, the port and the host's firewall"));
  }
}

void Session::SetUiSnapshot(const uint8_t* p, size_t n) {
  std::lock_guard<std::mutex> lock(mu_);
  ui_snap_.assign(p, p + n);
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
  if (role_ != Role::kHost || link_ != Link::kConnected || !mods_match_) return;
  link_ = Link::kStarting;
  start_sent_ = true;
  seed_ = uint32_t(NowMs() * 2654435761u) | 1u;
  std::lock_guard<std::mutex> lock(mu_);
  Send(StartPacket(seed_, ui_snap_));
}

void Session::ReseedBattle(uint8_t* base) {
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
  for (int i = 0; i < 60 * 60 && active(); ++i) {   // un minuto como mucho
    Pad p{};
    p.buttons = mine;
    RunTick(p, 0);
    if ((applied_[0].buttons & 0x0400) && (applied_[1].buttons & 0x0800)) {
      REXLOG_INFO("dbz3 online: combate alineado en el tick {} ({} ticks de espera)", tick_, tick_ - from);
      return;
    }
  }
  REXLOG_WARN("dbz3 online: el otro PC no llego al combate");
}

bool Session::TakeStart(uint8_t* base) {
  if (!launch_.exchange(false)) return false;
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
    std::vector<int> mine = dbz3::tag::MyLastTeam(), peer;
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
              delay_);
  return true;
}

void Session::Close() {
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
  std::lock_guard<std::mutex> lock(mu_);
  peer_mods_.clear();
  error_.clear();
}

void Session::Fail(Link l, const std::string& why) {
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

std::vector<std::string> Session::ModDiff() const {
  std::lock_guard<std::mutex> lock(mu_);
  std::vector<std::string> out;
  auto name = [](const std::string& s) { return s.substr(0, s.find('|')); };
  for (const auto& m : my_mods_) {
    if (std::find(peer_mods_.begin(), peer_mods_.end(), m) == peer_mods_.end()) out.push_back("+ " + name(m));
  }
  for (const auto& m : peer_mods_) {
    if (!m.empty() && std::find(my_mods_.begin(), my_mods_.end(), m) == my_mods_.end()) out.push_back("- " + name(m));
  }
  return out;
}

std::string Session::RoomCode(std::string* how) const {
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
    Close();
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
      bool got = false;
      while (Recv(pkt)) {
        if (!pkt.empty()) { Handle(pkt, true); got = true; }
        if (!active()) return;   // BYE u otro fallo: sin esperar 10 s a que caduque
        pkt.clear();
      }
      if (theirs_[t % kRing].tick == t) break;
      const uint64_t now = NowMs();
      if (now - last_send_ms_ > 15) SendInputs();
      // el primer tick espera a que el otro PC termine de arrancar
      if (now - last_recv_ms_ > (t <= delay_ ? 120000 : kLostMs)) {
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
