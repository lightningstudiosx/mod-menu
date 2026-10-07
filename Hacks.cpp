#include "Hacks.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace ov {

bool g_slowmoHeld = false;

namespace {
double g_values[static_cast<int>(H::COUNT)] = {};
unsigned g_changes = 0;

std::string saveKey(HackDef const& d) { return std::string("h.") + d.key; }

bool inPractice() {
    auto pl = PlayLayer::get();
    return pl && pl->m_isPracticeMode;
}
} // namespace

std::vector<HackDef> const& defs() {
    static std::vector<HackDef> list = {
#define OV_DEF(id, name, desc, cat, type, def, mn, mx, cheat, choices) \
    HackDef{H::id, #id, name, desc, cat, Type::type, static_cast<double>(def), static_cast<double>(mn), static_cast<double>(mx), cheat, choices},
        OV_HACKS(OV_DEF)
#undef OV_DEF
    };
    return list;
}

HackDef const& def(H h) { return defs()[static_cast<size_t>(h)]; }

std::vector<std::string> const& categories() {
    static std::vector<std::string> cats = [] {
        std::vector<std::string> out;
        for (auto const& d : defs())
            if (std::find(out.begin(), out.end(), d.category) == out.end()) out.push_back(d.category);
        return out;
    }();
    return cats;
}

std::vector<std::string> choicesOf(HackDef const& d) {
    std::vector<std::string> out;
    std::string cur;
    for (char const* c = d.choices; c && *c; c++) {
        if (*c == '|') { out.push_back(cur); cur.clear(); }
        else cur += *c;
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

double get(H h) { return g_values[static_cast<int>(h)]; }

unsigned changeCounter() { return g_changes; }

void set(H h, double v) {
    auto const& d = def(h);
    if (!std::isfinite(v)) v = d.def;
    v = std::clamp(v, d.min, d.max);
    if (d.type == Type::Int || d.type == Type::Choice || d.type == Type::Toggle) v = std::round(v);
    if (g_values[static_cast<int>(h)] != v) g_changes++;
    g_values[static_cast<int>(h)] = v;
    Mod::get()->setSavedValue<double>(saveKey(d), v);
}

void loadAll() {
    for (auto const& d : defs()) {
        double v = Mod::get()->getSavedValue<double>(saveKey(d), d.def);
        if (!std::isfinite(v)) v = d.def;
        g_values[static_cast<int>(d.h)] = std::clamp(v, d.min, d.max);
    }
    g_changes++;
}

void resetAll() {
    for (auto const& d : defs()) set(d.h, d.def);
}

bool noclipActive() {
    if (!on(H::noclip)) return false;
    if (on(H::noclip_practice_only) && !inPractice()) return false;
    return true;
}

float levelSpeed() {
    float s = 1.f;
    if (on(H::speedhack) && !(on(H::speed_practice_only) && !inPractice())) s *= getf(H::speed);
    if (g_slowmoHeld) s *= getf(H::slowmo_speed);
    return s;
}

float effectiveSpeed() {
    if (PlayLayer::get()) return levelSpeed();
    // in menus
    if (!on(H::speedhack) || on(H::speed_levels_only) || on(H::speed_practice_only)) return 1.f;
    return getf(H::speed);
}

namespace {
// is this cheat option actually doing something right now?
bool cheatActive(HackDef const& d) {
    if (!d.cheat || !on(d.h)) return false;
    if (d.h == H::noclip) return noclipActive();
    if (d.h == H::speedhack) return std::abs(levelSpeed() - 1.f) > 1e-4f && !(g_slowmoHeld && std::abs(getf(H::speed) - 1.f) < 1e-4f);
    return true;
}
bool slowmoCheat() { return g_slowmoHeld && PlayLayer::get() && std::abs(getf(H::slowmo_speed) - 1.f) > 1e-4f; }
} // namespace

bool cheating() {
    if (slowmoCheat()) return true;
    for (auto const& d : defs())
        if (cheatActive(d)) return true;
    return false;
}

std::string cheatList() {
    std::string out;
    for (auto const& d : defs()) {
        if (!cheatActive(d)) continue;
        if (!out.empty()) out += ", ";
        out += d.name;
    }
    if (slowmoCheat()) out += out.empty() ? "Slow-mo key" : ", Slow-mo key";
    return out;
}

} // namespace ov
