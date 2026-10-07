// Looks: custom icons, colours, trails, icon effects, world / screen effects, fun stuff.
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

#include <algorithm>
#include <cmath>

#include "Hacks.hpp"
#include "Util.hpp"

using namespace geode::prelude;
using namespace ov;

namespace {

// ------------------------------------------------------------------ icons

struct ModeInfo {
    H onKey;
    H idKey;
    IconType type;
};
constexpr int kModes = 9;
enum Mode { MCube, MShip, MBall, MUfo, MWave, MRobot, MSpider, MSwing, MJetpack };
ModeInfo const kModeInfo[kModes] = {
    {H::icon_cube_on, H::icon_cube, IconType::Cube},       {H::icon_ship_on, H::icon_ship, IconType::Ship},
    {H::icon_ball_on, H::icon_ball, IconType::Ball},       {H::icon_ufo_on, H::icon_ufo, IconType::Ufo},
    {H::icon_wave_on, H::icon_wave, IconType::Wave},       {H::icon_robot_on, H::icon_robot, IconType::Robot},
    {H::icon_spider_on, H::icon_spider, IconType::Spider}, {H::icon_swing_on, H::icon_swing, IconType::Swing},
    {H::icon_jetpack_on, H::icon_jetpack, IconType::Jetpack},
};
int s_randomIcon[kModes] = {};               // 0 = no random icon picked
int s_randomColor[2][3] = {{-1, -1, -1}, {-1, -1, -1}};
float s_time = 0.f;
bool s_deathEffectTouched = false;

bool isP2(PlayerObject* p) {
    auto gl = p->m_gameLayer;
    return gl && gl->m_player1 && gl->m_player1 != p;
}

// which icon should this player use for this gamemode? (loads it so the game can draw it)
int pickFrame(PlayerObject* p, int mode, int frame) {
    if (!p || !p->m_gameLayer) return frame;  // menu / garage icons stay normal
    if (isP2(p) && !on(H::icons_p2)) return frame;
    auto const& mi = kModeInfo[mode];
    int want = 0;
    if (on(H::icon_random) && s_randomIcon[mode] > 0) want = s_randomIcon[mode];
    else if (on(mi.onKey)) want = geti(mi.idKey);
    if (want <= 0) return frame;
    auto gm = GameManager::get();
    int count = gm->countForType(mi.type);
    if (count > 0) want = std::clamp(want, 1, count);
    gm->loadIcon(want, static_cast<int>(mi.type), p->m_iconRequestID);
    return want;
}

unsigned iconSignature() {
    unsigned h = 2166136261u;
    auto mix = [&](int v) { h = (h ^ static_cast<unsigned>(v)) * 16777619u; };
    for (int m = 0; m < kModes; m++) {
        mix(geti(kModeInfo[m].onKey));
        mix(geti(kModeInfo[m].idKey));
        mix(on(H::icon_random) ? s_randomIcon[m] : 0);
    }
    mix(geti(H::icons_p2));
    return h;
}

// re-run the game's own "set icon" for the gamemode the player is in right now, so changes show instantly
void refreshIcons(PlayerObject* p) {
    if (!p) return;
    auto gm = GameManager::get();
    auto active = [&](IconType t) { return gm->activeIconForType(t); };
    if (p->m_isShip) {
        if (p->m_isPlatformer) p->updatePlayerJetpackFrame(active(IconType::Jetpack));
        else p->updatePlayerShipFrame(active(IconType::Ship));
        p->updatePlayerFrame(active(IconType::Cube));
    } else if (p->m_isBird) {
        p->updatePlayerBirdFrame(active(IconType::Ufo));
        p->updatePlayerFrame(active(IconType::Cube));
    } else if (p->m_isBall) {
        p->updatePlayerRollFrame(active(IconType::Ball));
    } else if (p->m_isDart) {
        p->updatePlayerDartFrame(active(IconType::Wave));
    } else if (p->m_isSwing) {
        p->updatePlayerSwingFrame(active(IconType::Swing));
    } else if (p->m_isRobot || p->m_isSpider) {
        // robot / spider sprites are built when the level loads; they change on the next level load
    } else {
        p->updatePlayerFrame(active(IconType::Cube));
    }
}

// ------------------------------------------------------------------ colours

enum Part { PMain, PSecond, PGlow, PTrail, PWave, PartCount };
#define OV_PART(p, part) {H::c_##p##_##part##_mode, H::c_##p##_##part##_idx, H::c_##p##_##part##_r, H::c_##p##_##part##_g, H::c_##p##_##part##_b}
H const kColorKeys[2][PartCount][5] = {
    {OV_PART(p1, main), OV_PART(p1, second), OV_PART(p1, glow), OV_PART(p1, trail), OV_PART(p1, wave)},
    {OV_PART(p2, main), OV_PART(p2, second), OV_PART(p2, glow), OV_PART(p2, trail), OV_PART(p2, wave)},
};
#undef OV_PART

ccColor3B rgbOf(H const* k) { return rgb(geti(k[2]), geti(k[3]), geti(k[4])); }

// the colour this part should have right now; false = leave the game's colour alone
bool partColor(int pi, int part, ccColor3B& out) {
    int src = (pi == 1 && on(H::c_p2_copy)) ? 0 : pi;
    H const* k = kColorKeys[src][part];
    int mode = geti(k[0]);
    if (mode == 0) {
        if (on(H::rainbow) && (part == PMain || part == PSecond)) mode = 3;
        else if (on(H::random_colors) && part <= PGlow && s_randomColor[pi][part] >= 0) {
            out = GameManager::get()->colorForIdx(s_randomColor[pi][part]);
            return true;
        } else return false;
    }
    switch (mode) {
        case 1: out = GameManager::get()->colorForIdx(geti(k[1])); return true;
        case 2: out = rgbOf(k); return true;
        case 3: {
            float hue = s_time * 0.35f * getf(H::rainbow_speed) + getf(H::c_rainbow_spread) * 0.2f * static_cast<float>(part) +
                        (pi ? 0.5f : 0.f);
            out = hsv(hue, getf(H::c_rainbow_sat), getf(H::c_rainbow_bright));
            return true;
        }
        case 4: {
            auto c = rgbOf(k);
            float m = 0.55f + 0.45f * std::sin(s_time * getf(H::c_pulse_speed) * 6.2832f);
            out = rgb(static_cast<int>(c.r * m), static_cast<int>(c.g * m), static_cast<int>(c.b * m));
            return true;
        }
        default: return false;
    }
}

// ------------------------------------------------------------------ small helpers

void tintChildren(CCNode* n, ccColor3B c) {
    if (!n) return;
    auto ch = n->getChildren();
    if (!ch) return;
    for (unsigned i = 0; i < ch->count(); i++)
        if (auto s = typeinfo_cast<CCSprite*>(ch->objectAtIndex(i))) s->setColor(c);
}

ccColor3B firstChildColor(CCNode* n, ccColor3B fallback) {
    if (!n) return fallback;
    auto ch = n->getChildren();
    if (!ch || ch->count() == 0) return fallback;
    if (auto s = typeinfo_cast<CCSprite*>(ch->objectAtIndex(0))) return s->getColor();
    return fallback;
}

} // namespace

// ============================================================ PlayerObject: icons, colours, trails, icon effects

class $modify(OVIcons, PlayerObject) {
    struct Fields {
        FloatTweak sx, sy, rot, px, py, waveSize, wavePulse;
        float spin = 0.f;
        bool opacityOn = false;
        GLubyte opacityBase = 255;
        bool glowTouched = false, glowWas = false;
        HideState cube, cube2, cubeGlow;
        ColorState col[PartCount];
        bool glowColorActive = false, glowHadCustom = false;
        ccColor3B glowOrig{255, 255, 255};
        CCMotionStreak* widthTrail = nullptr;
        float widthMult = 1.f;
        bool alwaysTouched = false, alwaysWas = false, alwaysActivated = false;
        bool ghostOn = false;
        bool particlesHidden = false;
    };

    // ---------------- custom icons (the game calls these whenever it sets an icon)
    void updatePlayerFrame(int frame) { PlayerObject::updatePlayerFrame(pickFrame(this, MCube, frame)); }
    void updatePlayerShipFrame(int frame) { PlayerObject::updatePlayerShipFrame(pickFrame(this, MShip, frame)); }
    void updatePlayerRollFrame(int frame) { PlayerObject::updatePlayerRollFrame(pickFrame(this, MBall, frame)); }
    void updatePlayerBirdFrame(int frame) { PlayerObject::updatePlayerBirdFrame(pickFrame(this, MUfo, frame)); }
    void updatePlayerDartFrame(int frame) { PlayerObject::updatePlayerDartFrame(pickFrame(this, MWave, frame)); }
    void createRobot(int frame) { PlayerObject::createRobot(pickFrame(this, MRobot, frame)); }
    void createSpider(int frame) { PlayerObject::createSpider(pickFrame(this, MSpider, frame)); }
    void updatePlayerSwingFrame(int frame) { PlayerObject::updatePlayerSwingFrame(pickFrame(this, MSwing, frame)); }
    void updatePlayerJetpackFrame(int frame) { PlayerObject::updatePlayerJetpackFrame(pickFrame(this, MJetpack, frame)); }

    // ---------------- custom trail (picked when the player is created)
    void setupStreak() {
        if (on(H::custom_trail)) {
            auto gm = GameManager::get();
            int count = gm->countForType(IconType::Special);
            int id = std::clamp(geti(H::trail_id), 1, std::max(1, count));
            int old = gm->m_playerStreak;
            gm->m_playerStreak = id;
            m_playerStreak = id;
            PlayerObject::setupStreak();
            gm->m_playerStreak = old;
            return;
        }
        PlayerObject::setupStreak();
    }

    // ---------------- death effect
    void playDeathEffect() {
        if (on(H::no_death_effect)) return;
        auto gm = GameManager::get();
        if (on(H::death_effect_on)) {
            int custom = std::clamp(geti(H::death_effect), 1, 20);
            int old = gm->m_playerDeathEffect;
            gm->m_playerDeathEffect = custom;
            gm->loadDeathEffect(custom);  // make sure its pictures are loaded
            s_deathEffectTouched = true;
            PlayerObject::playDeathEffect();
            gm->m_playerDeathEffect = old;
            return;
        }
        // we may have swapped which effect is loaded; load the player's own one back first
        if (s_deathEffectTouched) gm->loadDeathEffect(gm->m_playerDeathEffect);
        PlayerObject::playDeathEffect();
    }

    // ---------------- everything below runs once per frame from the PlayLayer
    void ovApply(int pi, float dt) {
        auto f = m_fields.self();
        applyTransform(f, dt);
        applyOpacity(f);
        applyGlow(f);
        applyVehicleCube(f);
        applyColors(f, pi);
        applyTrails(f);
        applyParticles(f);
    }

    void applyTransform(Fields* f, float dt) {
        auto ml = m_mainLayer;
        if (!ml) return;
        float scale = getf(H::icon_scale);
        float jx = 1.f, jy = 1.f;
        if (on(H::icon_jelly)) {
            float s = std::sin(s_time * 9.f) * 0.14f;
            jx = 1.f + s;
            jy = 1.f - s;
        }
        float sx = scale * jx * (on(H::icon_mirror) ? -1.f : 1.f);
        float sy = scale * jy * (on(H::icon_upside) ? -1.f : 1.f);
        if (auto v = f->sx.step(ml->getScaleX(), std::abs(sx - 1.f) > 1e-4f, [&](float b) { return b * sx; })) ml->setScaleX(*v);
        if (auto v = f->sy.step(ml->getScaleY(), std::abs(sy - 1.f) > 1e-4f, [&](float b) { return b * sy; })) ml->setScaleY(*v);

        if (on(H::icon_spin)) f->spin = std::fmod(f->spin + dt * 360.f * getf(H::icon_spin_speed), 360.f);
        else f->spin = 0.f;
        if (auto v = f->rot.step(ml->getRotation(), on(H::icon_spin), [&](float b) { return b + f->spin; })) ml->setRotation(*v);

        bool shake = on(H::icon_shake);
        float ox = shake ? randf(-2.f, 2.f) : 0.f, oy = shake ? randf(-2.f, 2.f) : 0.f;
        if (auto v = f->px.step(ml->getPositionX(), shake, [&](float b) { return b + ox; })) ml->setPositionX(*v);
        if (auto v = f->py.step(ml->getPositionY(), shake, [&](float b) { return b + oy; })) ml->setPositionY(*v);
    }

    void applyOpacity(Fields* f) {
        float mult = getf(H::icon_opacity);
        if (on(H::icon_flicker)) mult *= std::fmod(s_time * 11.f, 1.f) < 0.5f ? 0.3f : 1.f;
        bool want = mult < 0.999f;
        if (want) {
            if (!f->opacityOn) { f->opacityBase = this->getOpacity(); f->opacityOn = true; }
            this->setOpacity(static_cast<GLubyte>(f->opacityBase * mult));
        } else if (f->opacityOn) {
            this->setOpacity(f->opacityBase);
            f->opacityOn = false;
        }
    }

    void applyGlow(Fields* f) {
        int want = on(H::icon_glow_hide) ? 0 : on(H::icon_glow_force) ? 1 : -1;
        if (want >= 0) {
            if (!f->glowTouched) { f->glowWas = m_hasGlow; f->glowTouched = true; }
            if (m_hasGlow != (want == 1)) {
                m_hasGlow = want == 1;
                this->updatePlayerGlow();
            }
        } else if (f->glowTouched) {
            f->glowTouched = false;
            if (m_hasGlow != f->glowWas) {
                m_hasGlow = f->glowWas;
                this->updatePlayerGlow();
            }
        }
    }

    void applyVehicleCube(Fields* f) {
        bool want = on(H::hide_vehicle_cube) && (m_isShip || m_isBird);
        f->cube.apply(m_iconSprite, want);
        f->cube2.apply(m_iconSpriteSecondary, want);
        f->cubeGlow.apply(m_iconGlow, want);
    }

    void applyColors(Fields* f, int pi) {
        ccColor3B c{};
        bool want = partColor(pi, PMain, c);
        f->col[PMain].apply(want, c, [&] { return m_playerColor1; }, [&](ccColor3B v) { this->setColor(v); });
        want = partColor(pi, PSecond, c);
        f->col[PSecond].apply(want, c, [&] { return m_playerColor2; }, [&](ccColor3B v) { this->setSecondColor(v); });

        want = partColor(pi, PGlow, c);
        if (want) {
            if (!f->glowColorActive) {
                f->glowHadCustom = m_hasCustomGlowColor;
                f->glowOrig = m_glowColor;
                f->glowColorActive = true;
            }
            m_hasCustomGlowColor = true;
            m_glowColor = c;
            this->updateGlowColor();
        } else if (f->glowColorActive) {
            m_hasCustomGlowColor = f->glowHadCustom;
            m_glowColor = f->glowOrig;
            this->updateGlowColor();
            f->glowColorActive = false;
        }

        if (m_regularTrail) {
            want = partColor(pi, PTrail, c);
            f->col[PTrail].apply(want, c, [&] { return m_regularTrail->getColor(); }, [&](ccColor3B v) { m_regularTrail->setColor(v); });
        }
        if (m_waveTrail) {
            want = partColor(pi, PWave, c);
            f->col[PWave].apply(want, c, [&] { return m_waveTrail->getColor(); }, [&](ccColor3B v) { m_waveTrail->setColor(v); });
        }
    }

    void applyTrails(Fields* f) {
        // width
        if (m_regularTrail) {
            float mult = getf(H::trail_width);
            float base = m_streakStrokeWidth > 0.5f ? m_streakStrokeWidth : 10.f;
            if (std::abs(mult - 1.f) > 1e-4f) {
                if (m_regularTrail != f->widthTrail || std::abs(mult - f->widthMult) > 1e-4f) {
                    m_regularTrail->setStroke(base * mult);
                    f->widthTrail = m_regularTrail;
                    f->widthMult = mult;
                }
            } else if (f->widthTrail) {
                if (f->widthTrail == m_regularTrail) m_regularTrail->setStroke(base);
                f->widthTrail = nullptr;
                f->widthMult = 1.f;
            }
        }
        // always on
        if (on(H::always_trail)) {
            if (!f->alwaysTouched) { f->alwaysWas = m_alwaysShowStreak; f->alwaysTouched = true; }
            m_alwaysShowStreak = true;
            if (!f->alwaysActivated && m_regularTrail) {
                this->activateStreak();
                f->alwaysActivated = true;
            }
        } else if (f->alwaysTouched) {
            m_alwaysShowStreak = f->alwaysWas;
            f->alwaysTouched = false;
            f->alwaysActivated = false;
        }
        // ghost trail
        bool ghost = on(H::ghost_trail);
        if (ghost && !f->ghostOn) {
            this->toggleGhostEffect(GhostType::Enabled);
            f->ghostOn = true;
        } else if (!ghost && f->ghostOn) {
            this->toggleGhostEffect(GhostType::Disabled);
            f->ghostOn = false;
        }
        // wave trail size / pulse
        if (m_waveTrail) {
            float wm = getf(H::wave_width);
            if (auto v = f->waveSize.step(m_waveTrail->m_waveSize, std::abs(wm - 1.f) > 1e-4f, [&](float b) { return b * wm; }))
                m_waveTrail->m_waveSize = *v;
            if (auto v = f->wavePulse.step(m_waveTrail->m_pulseSize, on(H::wave_pulse_off), [](float) { return 0.f; }))
                m_waveTrail->m_pulseSize = *v;
        }
    }

    void applyParticles(Fields* f) {
        CCParticleSystemQuad* list[] = {m_playerGroundParticles, m_trailingParticles, m_shipClickParticles, m_vehicleGroundParticles,
                                        m_ufoClickParticles, m_robotBurstParticles, m_dashParticles, m_swingBurstParticles1,
                                        m_swingBurstParticles2, m_landParticles0, m_landParticles1};
        if (on(H::hide_player_particles)) {
            for (auto p : list) if (p) p->setVisible(false);
            f->particlesHidden = true;
        } else if (f->particlesHidden) {
            for (auto p : list) if (p) p->setVisible(true);
            f->particlesHidden = false;
        }
    }

    // a new attempt: some effects get switched off by the game, so turn them back on
    void ovNewAttempt() {
        auto f = m_fields.self();
        f->alwaysActivated = false;
        if (m_ghostType != GhostType::Enabled) f->ghostOn = false;
    }
};

// ============================================================ PlayLayer: world, screen and fun effects

class $modify(OVWorld, PlayLayer) {
    struct Fields {
        CCLayerColor* tint = nullptr;
        FloatTweak rot, sx, sy, px, py;
        float spin = 0.f;
        HideState ground1, ground2, bg, mg;
        ColorState bgCol, barCol, pctCol, attCol;
        bool groundActive = false;
        ccColor3B groundOrig[4]{};
        unsigned iconSig = 0;
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        auto f = m_fields.self();
        f->tint = CCLayerColor::create({0, 0, 0, 0});
        f->tint->setID("tint"_spr);
        f->tint->setVisible(false);
        int z = m_uiLayer ? m_uiLayer->getZOrder() - 1 : 1000;
        this->addChild(f->tint, z);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        // random icons / colours for this attempt
        auto gm = GameManager::get();
        for (int m = 0; m < kModes; m++)
            s_randomIcon[m] = on(H::icon_random) ? randi(1, std::max(1, gm->countForType(kModeInfo[m].type))) : 0;
        for (int p = 0; p < 2; p++)
            for (int part = 0; part < 3; part++) s_randomColor[p][part] = on(H::random_colors) ? randi(0, 106) : -1;
        for (auto p : {m_player1, m_player2})
            if (p) static_cast<OVIcons*>(p)->ovNewAttempt();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto f = m_fields.self();
        float realDt = CCDirector::get()->getDeltaTime();
        s_time += realDt;

        unsigned sig = iconSignature();
        if (sig != f->iconSig) {
            f->iconSig = sig;
            refreshIcons(m_player1);
            refreshIcons(m_player2);
        }
        if (m_player1) static_cast<OVIcons*>(m_player1)->ovApply(0, realDt);
        if (m_player2) static_cast<OVIcons*>(m_player2)->ovApply(1, realDt);

        applyWorld(f);
        applyScreen(f, realDt);
        applyRainbowUi(f);
    }

    void applyWorld(Fields* f) {
        f->ground1.apply(m_groundLayer, on(H::hide_ground));
        f->ground2.apply(m_groundLayer2, on(H::hide_ground));
        f->bg.apply(m_background, on(H::hide_background));
        f->mg.apply(m_middleground, on(H::hide_middleground));

        if (m_background) {
            bool want = on(H::bg_override) || on(H::rainbow_bg);
            ccColor3B c = on(H::rainbow_bg) ? hsv(s_time * 0.2f * getf(H::rainbow_speed), 0.7f, 0.6f)
                                            : rgb(geti(H::bg_r), geti(H::bg_g), geti(H::bg_b));
            f->bgCol.apply(want, c, [&] { return m_background->getColor(); }, [&](ccColor3B v) { m_background->setColor(v); });
        }

        CCNode* grounds[4] = {m_groundLayer ? m_groundLayer->m_ground1Sprite : nullptr, m_groundLayer ? m_groundLayer->m_ground2Sprite : nullptr,
                              m_groundLayer2 ? m_groundLayer2->m_ground1Sprite : nullptr,
                              m_groundLayer2 ? m_groundLayer2->m_ground2Sprite : nullptr};
        bool wantGround = on(H::ground_override) || on(H::rainbow_ground);
        if (wantGround) {
            if (!f->groundActive) {
                for (int i = 0; i < 4; i++) f->groundOrig[i] = firstChildColor(grounds[i], {0, 102, 255});
                f->groundActive = true;
            }
            ccColor3B c = on(H::rainbow_ground) ? hsv(s_time * 0.2f * getf(H::rainbow_speed) + 0.5f, 0.8f, 0.8f)
                                                : rgb(geti(H::ground_r), geti(H::ground_g), geti(H::ground_b));
            for (auto g : grounds) tintChildren(g, c);
        } else if (f->groundActive) {
            for (int i = 0; i < 4; i++) tintChildren(grounds[i], f->groundOrig[i]);
            f->groundActive = false;
        }

        if (f->tint) {
            bool disco = on(H::disco), tint = on(H::tint_on);
            if (disco || tint) {
                ccColor3B c = disco ? hsv(std::floor(s_time * 2.f * getf(H::disco_speed)) * 0.161f, 1.f, 1.f)
                                    : rgb(geti(H::tint_r), geti(H::tint_g), geti(H::tint_b));
                f->tint->setColor(c);
                f->tint->setOpacity(static_cast<GLubyte>(tint ? geti(H::tint_a) : 70));
                f->tint->setVisible(true);
            } else {
                f->tint->setVisible(false);
            }
        }
    }

    void applyScreen(Fields* f, float dt) {
        if (on(H::spin_screen)) f->spin = std::fmod(f->spin + dt * 36.f * getf(H::spin_screen_speed), 360.f);
        else f->spin = 0.f;
        bool drunk = on(H::drunk);
        float rot = getf(H::screen_rotation) + f->spin + (drunk ? std::sin(s_time * 1.3f) * 7.f : 0.f);
        float zoom = getf(H::screen_zoom);
        float sx = zoom * (on(H::screen_flip_x) ? -1.f : 1.f) * (drunk ? 1.f + 0.04f * std::sin(s_time * 0.9f) : 1.f);
        float sy = zoom * (on(H::screen_flip_y) ? -1.f : 1.f) * (drunk ? 1.f + 0.04f * std::cos(s_time * 1.1f) : 1.f);
        if (auto v = f->rot.step(this->getRotation(), std::abs(rot) > 1e-3f, [&](float b) { return b + rot; })) this->setRotation(*v);
        if (auto v = f->sx.step(this->getScaleX(), std::abs(sx - 1.f) > 1e-4f, [&](float b) { return b * sx; })) this->setScaleX(*v);
        if (auto v = f->sy.step(this->getScaleY(), std::abs(sy - 1.f) > 1e-4f, [&](float b) { return b * sy; })) this->setScaleY(*v);

        bool quake = on(H::earthquake);
        float s = getf(H::earthquake_strength);
        float ox = quake ? randf(-s, s) : 0.f, oy = quake ? randf(-s, s) : 0.f;
        if (auto v = f->px.step(this->getPositionX(), quake, [&](float b) { return b + ox; })) this->setPositionX(*v);
        if (auto v = f->py.step(this->getPositionY(), quake, [&](float b) { return b + oy; })) this->setPositionY(*v);
    }

    void applyRainbowUi(Fields* f) {
        ccColor3B c = hsv(s_time * 0.35f * getf(H::rainbow_speed), 0.8f, 1.f);
        if (m_progressFill)
            f->barCol.apply(on(H::rainbow_bar), c, [&] { return m_progressFill->getColor(); }, [&](ccColor3B v) { m_progressFill->setColor(v); });
        if (m_percentageLabel)
            f->pctCol.apply(on(H::rainbow_percent), c, [&] { return m_percentageLabel->getColor(); },
                            [&](ccColor3B v) { m_percentageLabel->setColor(v); });
        if (m_attemptLabel)
            f->attCol.apply(on(H::rainbow_attempt), c, [&] { return m_attemptLabel->getColor(); },
                            [&](ccColor3B v) { m_attemptLabel->setColor(v); });
    }
};
