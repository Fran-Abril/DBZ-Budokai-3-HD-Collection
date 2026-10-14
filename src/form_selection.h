// Propuesta Fase 2: puro, sin SDK, guest, reloj, E/S ni escritura de partidas.
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace dbz3::forms {
constexpr int kIds = 105, kForms = 8;
inline bool GuestRange(uint32_t address, size_t size) {
  return address && size && uint64_t(size) <= 0x100000000ull - address;
}
enum class Reason { kOk, kUnavailable, kCount, kRequested, kSuit, kModel,
                    kSpecial, kCapsule, kCapsuleData, kDisabled, kCampaign, kOnline };
struct Form {
  int model_id = -1, model_index = -1, capsule = 0, type = 0, base_ki = 0;
  bool model_valid = false;
  std::array<int, 2> prerequisites{};
  bool capsule_record_valid = true;
  int required_ki_tenths = -1;
};
struct Snapshot {
  int id = -1, suit = 0, count = 0;
  bool ready = false;
  std::string identity;
  std::array<Form, kForms> forms{};
  std::vector<uint16_t> capsules;
};
struct Result {
  int requested = 0, applied = 0, model_id = -1, model_index = -1, capsule = 0;
  Reason reason = Reason::kUnavailable;
  bool valid = false;
};
inline bool CountValid(const Snapshot& s) {
  return s.ready && s.id >= 0 && s.id < kIds && s.count >= 1 && s.count <= kForms;
}
inline int MissingCapsule(const Snapshot& s, int f) {
  if (f <= 0 || f >= s.count || f >= kForms) return 0;
  const auto& m = s.forms[size_t(f)];
  for (int cap : {m.capsule, m.prerequisites[0], m.prerequisites[1]})
    if (cap && std::find(s.capsules.begin(), s.capsules.end(), cap) == s.capsules.end()) return cap;
  return 0;
}
inline Reason Availability(const Snapshot& s, int f, bool check_capsules = true) {
  if (!s.ready) return Reason::kUnavailable;
  if (!CountValid(s)) return Reason::kCount;
  if (f < 0 || f >= s.count) return Reason::kRequested;
  if (s.suit < 0) return Reason::kSuit;
  const auto& m = s.forms[size_t(f)];
  if (!m.model_valid || m.model_id < 0 || m.model_id >= kIds ||
      m.model_index < 0 || m.model_index >= kForms) return Reason::kModel;
  // Base no requiere transformación. Solo tipos medidos normales/irreversibles.
  if (f && m.type != 1 && m.type != 5 && m.type != 6) return Reason::kSpecial;
  if (f && !m.capsule_record_valid) return Reason::kCapsuleData;
  if (f && check_capsules && MissingCapsule(s, f)) return Reason::kCapsule;
  return Reason::kOk;
}
inline Result Resolve(const Snapshot& s, int want, bool enabled = true,
                      bool campaign_owned = false, bool online = false) {
  Result r;
  r.requested = want;
  if (campaign_owned) { r.reason = Reason::kCampaign; return r; }
  // El protocolo actual no negocia identidad/formas: no inyectar estado privado.
  if (online) { r.reason = Reason::kOnline; return r; }
  r.reason = enabled ? Availability(s, want) : Reason::kDisabled;
  r.applied = r.reason == Reason::kOk ? want : 0;
  if (Availability(s, r.applied) != Reason::kOk) return r;
  const auto& m = s.forms[size_t(r.applied)];
  r.model_id = m.model_id;
  r.model_index = m.model_index;
  r.capsule = (want >= 0 && want < s.count && want < kForms) ? s.forms[size_t(want)].capsule : 0;
  if (r.reason == Reason::kCapsule) r.capsule = MissingCapsule(s, want);
  r.valid = true;
  return r;
}
inline int Step(const Snapshot& s, int current, int direction) {
  // El mínimo fijado por guest/campaña se decide antes en el contexto del resolver.
  if (!CountValid(s) || !direction) return std::max(0, current);
  current = std::clamp(current, 0, s.count - 1);
  for (int f = current + (direction > 0 ? 1 : -1); f >= 0 && f < s.count;
       f += direction > 0 ? 1 : -1) {
    // Ofrecer la forma bloqueada por cápsula permite explicar el requisito.
    if (Availability(s, f, false) == Reason::kOk) return f;
  }
  return current;
}
inline bool ImposedStart(int record_minimum, int declared_campaign_form = -1) {
  return record_minimum > 0 || declared_campaign_form >= 0;
}
struct TriggerEdges {
  bool left = false, right = false;
  int Update(uint8_t lt, uint8_t rt) {
    const bool l = lt > 64, r = rt > 64;
    const int delta = int(r && !right) - int(l && !left);
    left = l; right = r;
    return delta;
  }
};
inline uint16_t Be16(const uint8_t* p) { return uint16_t((uint16_t(p[0]) << 8) | p[1]); }
inline uint32_t Be32(const uint8_t* p) {
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
}
// read(address, destination, length) debe comprobar TODO el rango antes de copiar.
// char372 decide forma lógica; char96 del origen resuelve el modelo del traje.
template <typename Reader>
Snapshot Decode(Reader read, int id, int suit, const std::string& scope) {
  Snapshot s; s.id = id; s.suit = suit;
  if (id < 0 || id >= kIds || suit < 0 || suit > 127) return s;
  std::array<uint8_t, 372> combat{};
  std::array<uint8_t, 96> own{};
  if (!read(0x82329CF0u + 372u * uint32_t(id), combat.data(), combat.size()) ||
      !read(0x8234ABB8u + 96u * uint32_t(id), own.data(), own.size())) return s;
  const uint32_t count = Be32(combat.data() + 208);
  if (count < 1 || count > kForms) { s.ready = true; return s; }
  s.count = int(count); s.ready = true;
  const unsigned caps = Be16(own.data() + 80);
  if (caps > 7) { s.ready = false; return s; }
  for (unsigned c = 0; c < caps; ++c) s.capsules.push_back(Be16(own.data() + 82 + 2 * c));
  uint64_t hash = 14695981039346656037ull;
  auto mix = [&](uint32_t v) { for (int b = 0; b < 4; ++b) { hash ^= uint8_t(v >> (8*b)); hash *= 1099511628211ull; } };
  mix(uint32_t(id)); mix(uint32_t(suit)); mix(count);
  for (int f = 0; f < s.count; ++f) {
    const auto* p = combat.data() + 212 + 20*f;
    auto& m = s.forms[size_t(f)];
    m.capsule = Be16(p); m.model_id = Be16(p+2); m.model_index = Be16(p+4);
    m.type = p[6]; m.base_ki = p[9];
    mix(m.capsule); mix(m.model_id); mix(m.model_index); mix(m.type);
    if (m.model_id >= kIds || m.model_index >= kForms) continue;
    std::array<uint8_t, 96> model{};
    if (!read(0x8234ABB8u + 96u * uint32_t(m.model_id), model.data(), model.size())) continue;
    const int suits = model[4], per = model[5], model_forms = model[7];
    mix(suits); mix(per); mix(model_forms);
    if (suit >= suits || per < 1 || per > 127 || model_forms < 1 || model_forms > kForms ||
        m.model_index >= model_forms) continue;
    const int index = model[18 + 8*m.model_index];
    if (index >= per || index >= 128) continue; // loader interpreta s8; FF = ausente
    const uint32_t table = Be32(model.data() + 8);
    const uint64_t entry = uint64_t(table) + 12ull * (uint32_t(suit)*per + index);
    std::array<uint8_t, 12> bytes{};
    if (!table || entry + bytes.size() > 0x100000000ull || !read(uint32_t(entry), bytes.data(), bytes.size())) continue;
    const uint32_t fid = Be32(bytes.data());
    mix(fid); // NO incluir punteros/cache: la identidad debe sobrevivir al reinicio.
    m.model_valid = fid < 0x80000000u;
  }
  s.identity = scope + ":" + std::to_string(id) + ":" + std::to_string(suit) + ":" + std::to_string(hash);
  return s;
}
} // namespace dbz3::forms
