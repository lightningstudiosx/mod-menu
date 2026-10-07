#include "Util.hpp"

#include <chrono>
#include <random>

using namespace geode::prelude;

namespace ov {

namespace {
std::mt19937& rng() {
    static std::mt19937 r(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
    return r;
}
} // namespace

ccColor3B hsv(float h, float s, float v) {
    h = std::fmod(h, 1.f);
    if (h < 0) h += 1.f;
    h *= 6.f;
    int i = static_cast<int>(h);
    float f = h - static_cast<float>(i), p = v * (1 - s), q = v * (1 - s * f), t = v * (1 - s * (1 - f));
    float r = v, g = t, b = p;
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    return {static_cast<GLubyte>(r * 255), static_cast<GLubyte>(g * 255), static_cast<GLubyte>(b * 255)};
}

double nowSeconds() {
    return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0;
}

float randf(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng()); }

int randi(int lo, int hi) {
    if (hi < lo) return lo;
    return std::uniform_int_distribution<int>(lo, hi)(rng());
}

} // namespace ov
