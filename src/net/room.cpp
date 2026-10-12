// dbz3 - Codigo de sala, UPnP y STUN del online (ver room.h).
#include "room.h"

#include <atomic>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <natupnp.h>
#include <oleauto.h>
#else
#include <arpa/inet.h>
#endif

namespace dbz3::net {
namespace {
// Base32 de Crockford: sin I, L, O ni U para que no se confundan al dictarlo
constexpr char kAlpha[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
constexpr uint32_t kCookie = 0x2112A442;
}  // namespace

std::string EncodeRoom(uint32_t ip, uint16_t port) {
  const uint8_t* b = reinterpret_cast<const uint8_t*>(&ip);
  uint64_t v = uint64_t(b[0]) << 40 | uint64_t(b[1]) << 32 | uint64_t(b[2]) << 24 | uint64_t(b[3]) << 16 | port;
  v = v << 2 | ((b[0] ^ b[1] ^ b[2] ^ b[3] ^ (port >> 8) ^ port) & 3);   // 2 bits de comprobacion
  std::string out(11, '-');
  for (int i = 9; i >= 0; --i, v >>= 5) out[size_t(i < 5 ? i : i + 1)] = kAlpha[v & 31];
  return out;
}

bool DecodeRoom(const std::string& code, uint32_t* ip, uint16_t* port) {
  uint64_t v = 0;
  int n = 0;
  for (char c : code) {
    if (c == '-' || c == ' ') continue;
    if (c >= 'a' && c <= 'z') c = char(c - 32);
    if (c == 'O') c = '0';
    if (c == 'I' || c == 'L') c = '1';
    const char* f = std::strchr(kAlpha, c);
    if (!f || !c) return false;
    v = v << 5 | uint64_t(f - kAlpha);
    ++n;
  }
  if (n != 10) return false;
  const uint8_t check = uint8_t(v & 3);
  v >>= 2;
  const uint16_t pt = uint16_t(v & 0xFFFF);
  uint8_t b[4] = {uint8_t(v >> 40), uint8_t(v >> 32), uint8_t(v >> 24), uint8_t(v >> 16)};
  if (((b[0] ^ b[1] ^ b[2] ^ b[3] ^ (pt >> 8) ^ pt) & 3) != check || !pt) return false;
  std::memcpy(ip, b, 4);
  *port = pt;
  return true;
}

#ifdef _WIN32
std::string UpnpOpen(uint16_t port, const std::string& local_ip) {
  std::string ext;
  if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) return ext;
  IUPnPNAT* nat = nullptr;
  IStaticPortMappingCollection* maps = nullptr;
  if (SUCCEEDED(CoCreateInstance(__uuidof(UPnPNAT), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&nat))) &&
      SUCCEEDED(nat->get_StaticPortMappingCollection(&maps)) && maps) {
    BSTR udp = SysAllocString(L"UDP");
    std::wstring wip(local_ip.begin(), local_ip.end());
    BSTR client = SysAllocString(wip.c_str());
    BSTR desc = SysAllocString(L"DBZ3 HD online");
    IStaticPortMapping* m = nullptr;
    if (SUCCEEDED(maps->Add(port, udp, port, client, VARIANT_TRUE, desc, &m)) && m) {
      BSTR e = nullptr;
      if (SUCCEEDED(m->get_ExternalIPAddress(&e)) && e) {
        for (const wchar_t* p = e; *p; ++p) ext += char(*p);
        SysFreeString(e);
      }
      m->Release();
    }
    SysFreeString(udp);
    SysFreeString(client);
    SysFreeString(desc);
    maps->Release();
  }
  if (nat) nat->Release();
  CoUninitialize();
  return ext;
}

void UpnpClose(uint16_t port) {
  if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) return;
  IUPnPNAT* nat = nullptr;
  IStaticPortMappingCollection* maps = nullptr;
  if (SUCCEEDED(CoCreateInstance(__uuidof(UPnPNAT), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&nat))) &&
      SUCCEEDED(nat->get_StaticPortMappingCollection(&maps)) && maps) {
    BSTR udp = SysAllocString(L"UDP");
    maps->Remove(port, udp);
    SysFreeString(udp);
    maps->Release();
  }
  if (nat) nat->Release();
  CoUninitialize();
}
#else
// ponytail: sin UPnP fuera de Windows (queda STUN); miniupnpc si hace falta en Linux
std::string UpnpOpen(uint16_t, const std::string&) { return {}; }
void UpnpClose(uint16_t) {}
#endif

void UpnpCloseAtExit(uint16_t port) {
  static std::atomic<uint16_t> mapped{0};
  mapped = port;
  static const bool registered = [] {
    std::atexit([] {
      if (const uint16_t p = mapped.load()) UpnpClose(p);
    });
    return true;
  }();
  (void)registered;
}

std::vector<uint8_t> StunRequest() {
  std::vector<uint8_t> p(20, 0);
  p[1] = 0x01;   // Binding Request, sin atributos
  p[4] = 0x21; p[5] = 0x12; p[6] = 0xA4; p[7] = 0x42;
  for (int i = 8; i < 20; ++i) p[size_t(i)] = uint8_t(0x5A ^ (i * 37));   // id de transaccion
  return p;
}

bool IsStun(const uint8_t* p, int n) {
  return n >= 20 && (p[0] & 0xC0) == 0 && (uint32_t(p[4]) << 24 | p[5] << 16 | p[6] << 8 | p[7]) == kCookie;
}

bool StunMapped(const uint8_t* p, int n, uint32_t* ip, uint16_t* port) {
  if (!IsStun(p, n) || p[0] != 0x01 || p[1] != 0x01) return false;   // Binding Success
  for (int o = 20; o + 4 <= n;) {
    const int type = p[o] << 8 | p[o + 1], len = p[o + 2] << 8 | p[o + 3];
    if ((type == 0x0020 || type == 0x0001) && len >= 8 && o + 4 + 8 <= n && p[o + 5] == 0x01) {
      uint16_t pt = uint16_t(p[o + 6] << 8 | p[o + 7]);
      uint8_t b[4] = {p[o + 8], p[o + 9], p[o + 10], p[o + 11]};
      if (type == 0x0020) {   // XOR-MAPPED-ADDRESS
        pt ^= uint16_t(kCookie >> 16);
        for (int i = 0; i < 4; ++i) b[i] ^= uint8_t(kCookie >> (24 - 8 * i));
      }
      std::memcpy(ip, b, 4);
      *port = pt;
      return true;
    }
    o += 4 + ((len + 3) & ~3);
  }
  return false;
}

bool RoomSelfTest() {
  uint32_t ip = 0, back = 0;
  uint16_t port = 0;
  inet_pton(AF_INET, "83.45.201.7", &ip);
  const std::string c = EncodeRoom(ip, 7777);
  if (c.size() != 11 || c[5] != '-' || !DecodeRoom(c, &back, &port) || back != ip || port != 7777) return false;
  std::string low = c;
  for (auto& ch : low) ch = char(ch >= 'A' && ch <= 'Z' ? ch + 32 : ch);
  if (!DecodeRoom(low, &back, &port) || back != ip) return false;
  std::string bad = c;
  bad[0] = bad[0] == 'Z' ? 'Y' : 'Z';   // una letra mal: la comprobacion lo detecta casi siempre
  uint32_t b2 = 0;
  return !DecodeRoom("12345", &b2, &port) && (!DecodeRoom(bad, &b2, &port) || b2 != ip);
}

}  // namespace dbz3::net
