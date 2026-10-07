#include "Hacks.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace ov {

namespace {
double g_values[static_cast<int>(H::COUNT)] = {};

std::string saveKey(HackDef const& d) { return std::string("h.") + d.key; }
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

void set(H h, double v) {
    auto const& d = defs()[static_cast<size_t>(h)];
    if (!std::isfinite(v)) v = d.def;
    v = std::clamp(v, d.min, d.max);
    if (d.type == Type::Int || d.type == Type::Choice || d.type == Type::Toggle) v = std::round(v);
    g_values[static_cast<int>(h)] = v;
    Mod::get()->setSavedValue<double>(saveKey(d), v);
}

void loadAll() {
    for (auto const& d : defs()) {
        double v = Mod::get()->getSavedValue<double>(saveKey(d), d.def);
        if (!std::isfinite(v)) v = d.def;
        g_values[static_cast<int>(d.h)] = std::clamp(v, d.min, d.max);
    }
}

void resetAll() {
    for (auto const& d : defs()) set(d.h, d.def);
}

bool cheating() {
    for (auto const& d : defs()) {
        if (!d.cheat || !on(d.h)) continue;
        if (d.h == H::speedhack && std::abs(get(H::speed) - 1.0) < 1e-6) continue;  // speed 1 isn't a cheat
        return true;
    }
    return false;
}

std::string cheatList() {
    std::string out;
    for (auto const& d : defs()) {
        if (!d.cheat || !on(d.h)) continue;
        if (d.h == H::speedhack && std::abs(get(H::speed) - 1.0) < 1e-6) continue;
        if (!out.empty()) out += ", ";
        out += d.name;
    }
    return out;
}

} // namespace ov
