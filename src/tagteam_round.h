// Fase 3 aislada: politica host, sin direcciones guest ni reloj de pared.
#pragma once
#include "form_selection.h"
#include <set>
#include <cmath>

namespace dbz3::tag::round {
struct PositionCache {
  std::array<float, 3> value{};
  bool valid = false;
  bool Remember(const std::array<float, 3>& position) {
    for (float v : position) if (!std::isfinite(v)) return false;
    value = position; valid = true; return true;
  }
};
inline bool ValidTeam(const std::vector<int>& ids) {
  if (ids.size() < 2 || ids.size() > 5) return false;
  std::set<int> unique;
  for (int id : ids) if (id < 0 || id >= forms::kIds || !unique.insert(id).second) return false;
  return true;
}
inline int OwnRequest(bool visited, int saved, int selected) { return visited ? saved : selected; }
inline bool ScratchFree(uint32_t current, bool slot6, bool slot7) {
  return current < 6 && !slot6 && !slot7;
}
inline bool CommitReady(bool local, bool online, bool peer0, bool peer1, bool free,
                        bool quiet, bool effects_quiet) {
  return local && (!online || (peer0 && peer1)) && free && quiet && effects_quiet;
}
enum class RestoreStep { kWait, kIssue, kConfirmed, kStale, kBlocked };
struct RestoreObservation {
  uint32_t generation = 0;
  int id = -1, current = 0, next = 0;
  bool legal = false, model_ready = false, context_ready = false;
  bool free = false, landed = false, fighting = false, quiet = false;
};
struct RestoreTicket {
  uint32_t generation = 0, began = 0;
  int id = -1, target = 0;
  bool issued = false, blocked = false;
  void Begin(uint32_t gen, int member, int form, uint32_t now) {
    *this = {}; generation = gen; id = member; target = form; began = now;
  }
  RestoreStep Poll(const RestoreObservation& o, uint32_t now) {
    if (!target) return RestoreStep::kWait;
    if (o.generation != generation || o.id != id) return RestoreStep::kStale;
    if (blocked) return RestoreStep::kBlocked;
    if (!o.legal) { blocked = true; return RestoreStep::kBlocked; }
    if (o.current == target && o.next == target && o.quiet) { target = 0; return RestoreStep::kConfirmed; }
    // Un timeout no autoriza liberar modelos ni forzar otra accion.
    if (now - began > 600) { blocked = true; return RestoreStep::kBlocked; }
    if (!issued && o.current == 0 && o.next == 0 && o.model_ready && o.context_ready &&
        o.free && o.landed && o.fighting && o.quiet) {
      issued = true; return RestoreStep::kIssue;
    }
    return RestoreStep::kWait;
  }
};
} // namespace dbz3::tag::round
