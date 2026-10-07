#pragma once
// Small helpers shared by the hook files.
#include <Geode/Geode.hpp>

#include <algorithm>
#include <cmath>
#include <optional>

namespace ov {

cocos2d::ccColor3B hsv(float h, float s, float v);
double nowSeconds();
float randf(float lo, float hi);
int randi(int lo, int hi);  // inclusive

inline cocos2d::ccColor3B rgb(int r, int g, int b) {
    return {static_cast<GLubyte>(std::clamp(r, 0, 255)), static_cast<GLubyte>(std::clamp(g, 0, 255)),
            static_cast<GLubyte>(std::clamp(b, 0, 255))};
}

// Hide a node while `want` is on; put back exactly how it was when `want` turns off.
struct HideState {
    bool active = false;
    bool wasVisible = true;
    void apply(cocos2d::CCNode* node, bool want) {
        if (!node) return;
        if (want) {
            if (!active) { wasVisible = node->isVisible(); active = true; }
            node->setVisible(false);
        } else if (active) {
            node->setVisible(wasVisible);
            active = false;
        }
    }
};

// Change a number on top of whatever the game sets it to. If the game changes the value
// while we're active, that becomes the new base. When we stop, the game's value is put back.
struct FloatTweak {
    bool active = false;
    float base = 0.f, lastSet = 0.f;
    template <class Fn>
    std::optional<float> step(float current, bool want, Fn&& fn) {
        if (want) {
            if (!active || std::abs(current - lastSet) > 1e-3f) base = current;
            active = true;
            lastSet = fn(base);
            return lastSet;
        }
        if (active) {
            active = false;
            if (std::abs(current - lastSet) <= 1e-3f) return base;
        }
        return std::nullopt;
    }
};

// Override a colour; remember the original so it can be put back.
struct ColorState {
    bool active = false;
    cocos2d::ccColor3B orig{255, 255, 255};
    template <class Get, class Set>
    void apply(bool want, cocos2d::ccColor3B c, Get&& getter, Set&& setter) {
        if (want) {
            if (!active) { orig = getter(); active = true; }
            setter(c);
        } else if (active) {
            setter(orig);
            active = false;
        }
    }
};

} // namespace ov
