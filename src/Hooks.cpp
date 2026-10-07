// Everything that changes the game. Each feature checks its option from Hacks.hpp.
#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/CCTransitionFade.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/Notification.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <deque>

#include "Hacks.hpp"
#include "Menu.hpp"

using namespace geode::prelude;
using namespace ov;

namespace {
int s_stepsQueued = 0;        // frame stepper: ticks waiting to run
bool s_audioFrozen = false;   // frame stepper paused the sound
bool s_pitchApplied = false;  // speedhack changed the music pitch
int64_t s_autoLastTick = -1;  // auto clicker
int64_t s_autoTick = 0;
int s_autoHoldLeft = 0;
float s_rainbowTime = 0.f;

double nowSeconds() {
    return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0;
}

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

std::string clockText(double s) {
    int m = static_cast<int>(s / 60);
    return fmt::format("{}:{:04.1f}", m, s - m * 60);
}

void setAudioFrozen(bool frozen) {
    if (frozen == s_audioFrozen) return;
    auto fmod = FMODAudioEngine::get();
    if (!fmod || !fmod->m_globalChannel) return;
    fmod->m_globalChannel->setPaused(frozen);
    s_audioFrozen = frozen;
}

// hide a node while `want` is on; put back exactly how it was when `want` turns off
struct HideState {
    bool active = false;
    bool wasVisible = true;
    void apply(CCNode* node, bool want) {
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
} // namespace

// ============================================================ PlayLayer: most level features + HUD

class $modify(OVPlayLayer, PlayLayer) {
    struct Fields {
        // noclip
        int64_t lastProgress = 0;
        int64_t ticks = 0, deadTicks = 0;
        int noclipDeaths = 0;
        bool diedThisFrame = false;
        bool wasDeadLastFrame = false;
        CCLayerColor* flash = nullptr;
        // safe mode (we only ever force test mode ON, and give back the game's own value afterwards)
        bool cheated = false;
        bool forcedTest = false;
        bool testBefore = false;
        // runs
        int sessionAttempts = 0;
        double sessionTime = 0;
        float runStart = 0.f;
        float bestFrom = 0.f, bestTo = 0.f;
        // clicks
        std::deque<double> clickTimes;
        int clicksThisAttempt = 0;
        size_t maxCps = 0;
        // startpos
        std::vector<StartPosObject*> startPositions;
        int startPosIndex = -1;
        CCMenu* spMenu = nullptr;
        CCLabelBMFont* spLabel = nullptr;
        // things we hid (so we can undo it)
        HideState pauseBtn, attemptLbl, progressBar, progressFill, percentLbl;
        bool hidPlayer = false, hidTrail = false, hidCheckpoints = false, ownHitboxes = false, rainbowWasOn = false;
        // HUD
        CCNode* hud = nullptr;
        std::vector<CCLabelBMFont*> hudLines;
        float fpsAccum = 0.f;
        int fpsFrames = 0;
        float fps = 0.f;
    };

    float percentNow() { return std::clamp(this->getCurrentPercent(), 0.f, 100.f); }

    bool isPlatformer() { return m_levelSettings && m_levelSettings->m_platformerMode; }

    CCNode* uiParent() { return m_uiLayer ? static_cast<CCNode*>(m_uiLayer) : static_cast<CCNode*>(this); }

    void forceTestMode() {
        auto f = m_fields.self();
        if (!f->forcedTest) {
            f->testBefore = m_isTestMode;
            f->forcedTest = true;
        }
        m_isTestMode = true;
    }

    void unforceTestMode() {
        auto f = m_fields.self();
        if (!f->forcedTest) return;
        m_isTestMode = f->testBefore;
        f->forcedTest = false;
    }

    void addObject(GameObject* obj) {
        PlayLayer::addObject(obj);
        if (obj && obj->m_objectID == 31) m_fields->startPositions.push_back(static_cast<StartPosObject*>(obj));
    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        auto f = m_fields.self();
        std::stable_sort(f->startPositions.begin(), f->startPositions.end(),
                         [](StartPosObject* a, StartPosObject* b) { return a->getPositionX() < b->getPositionX(); });
        f->startPosIndex = -1;
        for (size_t i = 0; i < f->startPositions.size(); i++)
            if (f->startPositions[i] == m_startPosObject) f->startPosIndex = static_cast<int>(i);

        f->hud = CCNode::create();
        f->hud->setID("hud"_spr);
        uiParent()->addChild(f->hud, 9999);
        f->flash = CCLayerColor::create({255, 0, 0, 0});
        f->flash->setID("noclip-flash"_spr);
        uiParent()->addChild(f->flash, 9998);
        buildStartPosUI();

        if (ov::on(H::auto_practice) && !m_isPracticeMode) this->togglePracticeMode(true);
    }

    void togglePracticeMode(bool practice) {
        if (practice && ov::on(H::practice_music)) m_practiceMusicSync = true;
        PlayLayer::togglePracticeMode(practice);
    }

    // ---------------- startpos switcher

    void buildStartPosUI() {
        auto f = m_fields.self();
        if (f->startPositions.empty()) return;
        auto win = CCDirector::get()->getWinSize();
        f->spMenu = CCMenu::create();
        f->spMenu->setID("startpos-menu"_spr);
        f->spMenu->setPosition({win.width / 2.f, 22.f});
        for (int side = 0; side < 2; side++) {
            auto spr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
            spr->setScale(0.55f);
            if (side == 1) spr->setFlipX(true);
            auto btn = CCMenuItemSpriteExtra::create(
                spr, this, side == 0 ? menu_selector(OVPlayLayer::onStartPosPrev) : menu_selector(OVPlayLayer::onStartPosNext));
            btn->setPosition({side == 0 ? -55.f : 55.f, 0.f});
            f->spMenu->addChild(btn);
        }
        uiParent()->addChild(f->spMenu, 100);
        f->spLabel = CCLabelBMFont::create("", "bigFont.fnt");
        f->spLabel->setID("startpos-label"_spr);
        f->spLabel->setScale(0.45f);
        uiParent()->addChild(f->spLabel, 100);
        updateStartPosUI();
    }

    void updateStartPosUI() {
        auto f = m_fields.self();
        if (!f->spMenu || !f->spLabel) return;
        bool show = ov::on(H::startpos_switcher) && ov::on(H::startpos_buttons) && !f->startPositions.empty();
        f->spMenu->setVisible(show);
        f->spLabel->setVisible(show);
        if (!show) return;
        auto win = CCDirector::get()->getWinSize();
        float y = m_isPracticeMode ? 72.f : 22.f;  // stay above the checkpoint buttons
        f->spMenu->setPosition({win.width / 2.f, y});
        f->spLabel->setPosition({win.width / 2.f, y});
        f->spLabel->setString(fmt::format("{}/{}", f->startPosIndex + 1, f->startPositions.size()).c_str());
    }

    void onStartPosPrev(CCObject*) { switchStartPos(-1); }
    void onStartPosNext(CCObject*) { switchStartPos(1); }

    void switchStartPos(int dir) {
        auto f = m_fields.self();
        if (f->startPositions.empty() || m_isPaused || m_levelEndAnimationStarted || OverloadMenu::isOpen()) return;
        int n = static_cast<int>(f->startPositions.size());
        int idx = f->startPosIndex + dir;
        if (idx < -1) idx = n - 1;
        if (idx >= n) idx = -1;
        f->startPosIndex = idx;
        StartPosObject* sp = idx < 0 ? nullptr : f->startPositions[static_cast<size_t>(idx)];

        this->stopActionByTag(0x10);  // a respawn that was waiting
        if (m_isPracticeMode) this->removeAllCheckpoints();
        unforceTestMode();
        m_isTestMode = sp != nullptr;  // the game itself does this when a level has a start position
        this->setStartPosObject(sp);
        this->resetLevelFromStart();
        this->startMusic();
        updateStartPosUI();
    }

    // ---------------- attempts

    void resetLevel() {
        unforceTestMode();  // let the game reset with its own value
        PlayLayer::resetLevel();
        auto f = m_fields.self();
        f->sessionAttempts++;
        f->ticks = f->deadTicks = 0;
        f->noclipDeaths = 0;
        f->diedThisFrame = f->wasDeadLastFrame = false;
        f->lastProgress = static_cast<int64_t>(m_gameState.m_currentProgress);
        f->clicksThisAttempt = 0;
        f->maxCps = 0;
        f->runStart = percentNow();
        f->cheated = cheating();
        if (ov::on(H::safe_mode) && f->cheated) forceTestMode();
        if (m_attemptLabel && ov::on(H::hide_attempts)) m_attemptLabel->setVisible(false);
    }

    void recordRun(float to) {
        auto f = m_fields.self();
        if (to - f->runStart > f->bestTo - f->bestFrom) {
            f->bestFrom = f->runStart;
            f->bestTo = to;
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (object == m_anticheatSpike) return PlayLayer::destroyPlayer(player, object);
        auto f = m_fields.self();
        bool protect = ov::on(H::noclip) && !(ov::on(H::noclip_p1) && player == m_player2);
        if (protect) {
            f->diedThisFrame = true;
            return;
        }
        if (m_player1 && !m_player1->m_isDead) recordRun(percentNow());
        PlayLayer::destroyPlayer(player, object);

        if (ov::on(H::custom_respawn)) {
            // the game waits about a second (action tag 0x10) before respawning; swap in our own timer
            if (auto act = this->getActionByTag(0x10)) {
                this->stopAction(act);
                auto seq = CCSequence::create(CCDelayTime::create(static_cast<float>(ov::get(H::respawn_delay))),
                                              CCCallFunc::create(this, callfunc_selector(PlayLayer::delayedResetLevel)), nullptr);
                seq->setTag(0x10);
                this->runAction(seq);
            }
        }
    }

    void levelComplete() {
        recordRun(100.f);
        PlayLayer::levelComplete();
    }

    void showNewBest(bool newReward, int orbs, int diamonds, bool demonKey, bool noRetry, bool noTitle) {
        if (ov::on(H::no_new_best)) return;
        PlayLayer::showNewBest(newReward, orbs, diamonds, demonKey, noRetry, noTitle);
    }

    void updateProgressbar() {
        PlayLayer::updateProgressbar();
        if (ov::on(H::accurate_percent) && m_percentageLabel && !isPlatformer()) {
            int dec = static_cast<int>(ov::get(H::percent_decimals));
            m_percentageLabel->setString(fmt::format("{:.{}f}%", percentNow(), dec).c_str());
        }
    }

    // ---------------- every frame

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto f = m_fields.self();
        float realDt = CCDirector::get()->getDeltaTime();
        f->sessionTime += realDt;

        // --- noclip accuracy (counted in physics ticks)
        int64_t prog = static_cast<int64_t>(m_gameState.m_currentProgress);
        int64_t adv = std::max<int64_t>(0, prog - f->lastProgress);
        f->lastProgress = prog;
        f->ticks += adv;
        if (f->diedThisFrame) {
            f->deadTicks += std::max<int64_t>(1, adv);
            if (!f->wasDeadLastFrame) {
                f->noclipDeaths++;
                if (ov::on(H::noclip_flash) && f->flash) {
                    f->flash->stopAllActions();
                    f->flash->setOpacity(90);
                    f->flash->runAction(CCFadeTo::create(0.35f, 0));
                }
            }
        }
        f->wasDeadLastFrame = f->diedThisFrame;
        f->diedThisFrame = false;

        // --- safe mode: once a cheat is used, this attempt can't save anything
        if (cheating()) f->cheated = true;
        if (ov::on(H::safe_mode) && f->cheated) forceTestMode();
        else if (!ov::on(H::safe_mode)) unforceTestMode();

        // --- hitboxes
        bool wantHitboxes = ov::on(H::hitboxes) || (ov::on(H::hitboxes_on_death) && m_player1 && m_player1->m_isDead);
        if (wantHitboxes) {
            m_isDebugDrawEnabled = true;
            if (m_debugDrawNode) m_debugDrawNode->setVisible(true);
            this->updateDebugDraw();
            f->ownHitboxes = true;
        } else if (f->ownHitboxes) {
            m_isDebugDrawEnabled = false;
            if (m_debugDrawNode) m_debugDrawNode->clear();
            f->ownHitboxes = false;
        }

        // --- player visuals
        if (ov::on(H::hide_player)) {
            if (m_player1) m_player1->setVisible(false);
            if (m_player2) m_player2->setVisible(false);
            f->hidPlayer = true;
        } else if (f->hidPlayer) {
            if (m_player1) m_player1->setVisible(!m_player1->m_isDead);
            if (m_player2) m_player2->setVisible(m_gameState.m_isDualMode && !m_player2->m_isDead);
            f->hidPlayer = false;
        }
        if (ov::on(H::no_trail)) {
            for (auto p : {m_player1, m_player2})
                if (p && p->m_regularTrail) p->m_regularTrail->setVisible(false);
            f->hidTrail = true;
        } else if (f->hidTrail) {
            for (auto p : {m_player1, m_player2})
                if (p && p->m_regularTrail) p->m_regularTrail->setVisible(true);
            f->hidTrail = false;
        }
        if (ov::on(H::rainbow)) {
            s_rainbowTime += realDt * static_cast<float>(ov::get(H::rainbow_speed)) * 0.35f;
            if (m_player1) {
                m_player1->setColor(hsv(s_rainbowTime, 0.8f, 1.f));
                m_player1->setSecondColor(hsv(s_rainbowTime + 0.5f, 0.8f, 1.f));
            }
            if (m_player2) {
                m_player2->setColor(hsv(s_rainbowTime + 0.5f, 0.8f, 1.f));
                m_player2->setSecondColor(hsv(s_rainbowTime, 0.8f, 1.f));
            }
            f->rainbowWasOn = true;
        } else if (f->rainbowWasOn) {
            auto gm = GameManager::get();
            auto c1 = gm->colorForIdx(gm->getPlayerColor());
            auto c2 = gm->colorForIdx(gm->getPlayerColor2());
            if (m_player1) { m_player1->setColor(c1); m_player1->setSecondColor(c2); }
            if (m_player2) { m_player2->setColor(c2); m_player2->setSecondColor(c1); }
            f->rainbowWasOn = false;
        }

        // --- UI bits (we only touch what we hid, so the game's own settings keep working)
        if (m_uiLayer) {
            f->pauseBtn.apply(m_uiLayer->m_pauseBtn, ov::on(H::hide_pause));
            if (m_uiLayer->m_checkpointMenu) {
                if (ov::on(H::hide_practice_btns)) {
                    m_uiLayer->m_checkpointMenu->setVisible(false);
                    f->hidCheckpoints = true;
                } else if (f->hidCheckpoints) {
                    m_uiLayer->m_checkpointMenu->setVisible(m_isPracticeMode);
                    f->hidCheckpoints = false;
                }
            }
        }
        f->attemptLbl.apply(m_attemptLabel, ov::on(H::hide_attempts));
        f->progressBar.apply(m_progressBar, ov::on(H::hide_progress));
        f->progressFill.apply(m_progressFill, ov::on(H::hide_progress));
        f->percentLbl.apply(m_percentageLabel, ov::on(H::hide_percent));
        updateStartPosUI();

        // --- clicks per second window
        double now = nowSeconds();
        while (!f->clickTimes.empty() && now - f->clickTimes.front() > 1.0) f->clickTimes.pop_front();
        f->maxCps = std::max(f->maxCps, f->clickTimes.size());

        // --- fps
        f->fpsAccum += realDt;
        f->fpsFrames++;
        if (f->fpsAccum >= 0.25f) {
            f->fps = static_cast<float>(f->fpsFrames) / f->fpsAccum;
            f->fpsAccum = 0.f;
            f->fpsFrames = 0;
        }

        updateHud();
    }

    void onClick() {
        auto f = m_fields.self();
        f->clickTimes.push_back(nowSeconds());
        f->clicksThisAttempt++;
    }

    void updateHud() {
        auto f = m_fields.self();
        if (!f->hud) return;
        f->hud->setVisible(ov::on(H::hud));
        if (!ov::on(H::hud)) return;

        std::vector<std::pair<std::string, ccColor3B>> lines;
        ccColor3B white{255, 255, 255};
        if (ov::on(H::hud_cheat)) {
            if (cheating()) lines.push_back({ov::on(H::safe_mode) ? "CHEATING (safe mode on)" : "CHEATING - SAFE MODE OFF", {255, 90, 90}});
            else if (f->cheated) lines.push_back({"Cheated this attempt (won't save)", {255, 170, 90}});
            else lines.push_back({"Legit", {110, 240, 120}});
        }
        if (ov::on(H::hud_fps)) lines.push_back({fmt::format("FPS {:.0f}", f->fps), white});
        if (ov::on(H::hud_cps))
            lines.push_back({fmt::format("CPS {} (best {})  {} clicks", f->clickTimes.size(), f->maxCps, f->clicksThisAttempt), white});
        if (ov::on(H::noclip)) {
            double acc = f->ticks > 0 ? 100.0 * (1.0 - static_cast<double>(f->deadTicks) / static_cast<double>(f->ticks)) : 100.0;
            if (ov::on(H::hud_noclip_acc)) lines.push_back({fmt::format("Accuracy {:.2f}%", acc), acc >= 100.0 ? white : ccColor3B{255, 160, 160}});
            if (ov::on(H::hud_noclip_deaths)) lines.push_back({fmt::format("Noclip deaths {}", f->noclipDeaths), white});
        }
        if (ov::on(H::hud_attempts)) lines.push_back({fmt::format("Session attempts {}", f->sessionAttempts), white});
        if (ov::on(H::hud_time)) lines.push_back({fmt::format("Attempt time {}", clockText(m_attemptTime)), white});
        if (ov::on(H::hud_session_time)) lines.push_back({fmt::format("Time in level {}", clockText(f->sessionTime)), white});
        if (ov::on(H::hud_best_run) && !isPlatformer() && f->bestTo > 0)
            lines.push_back({fmt::format("Best run {:.0f}-{:.0f}%", f->bestFrom, f->bestTo), white});
        if (ov::on(H::hud_jumps)) lines.push_back({fmt::format("Jumps {}", m_jumps), white});
        if (ov::on(H::hud_level_id) && m_level) lines.push_back({fmt::format("Level ID {}", static_cast<int>(m_level->m_levelID)), white});
        if (ov::on(H::hud_clock)) {
            std::time_t t = std::time(nullptr);
            std::tm tm{};
#ifdef GEODE_IS_WINDOWS
            localtime_s(&tm, &t);
#else
            localtime_r(&t, &tm);
#endif
            lines.push_back({fmt::format("{:02}:{:02}:{:02}", tm.tm_hour, tm.tm_min, tm.tm_sec), white});
        }
        if (ov::on(H::startpos_switcher) && !f->startPositions.empty())
            lines.push_back({fmt::format("StartPos {}/{}", f->startPosIndex + 1, f->startPositions.size()), {150, 220, 255}});
        if (ov::on(H::speedhack) && std::abs(ov::get(H::speed) - 1.0) > 1e-6)
            lines.push_back({fmt::format("Speed {:g}x", ov::get(H::speed)), {255, 200, 120}});
        if (ov::on(H::frame_stepper)) lines.push_back({"Frame stepper: press F to step", {255, 200, 120}});
        if (ov::on(H::autoclicker)) lines.push_back({fmt::format("Auto clicker {:g} CPS", ov::get(H::autoclick_cps)), {255, 200, 120}});

        while (f->hudLines.size() < lines.size()) {
            auto l = CCLabelBMFont::create("", "bigFont.fnt");
            f->hud->addChild(l);
            f->hudLines.push_back(l);
        }
        auto win = CCDirector::get()->getWinSize();
        float scale = 0.32f * static_cast<float>(ov::get(H::hud_scale));
        float lineH = 26.f * scale + 2.f;
        int corner = static_cast<int>(ov::get(H::hud_corner));
        bool right = corner == 1 || corner == 3;
        bool bottom = corner >= 2;
        auto opacity = static_cast<GLubyte>(255 * ov::get(H::hud_opacity));
        for (size_t i = 0; i < f->hudLines.size(); i++) {
            auto l = f->hudLines[i];
            if (i >= lines.size()) { l->setVisible(false); continue; }
            l->setVisible(true);
            l->setString(lines[i].first.c_str());
            l->setColor(lines[i].second);
            l->setOpacity(opacity);
            l->setScale(scale);
            l->setAnchorPoint({right ? 1.f : 0.f, 0.5f});
            float x = right ? win.width - 6.f : 6.f;
            float y = bottom ? 8.f + lineH * static_cast<float>(lines.size() - 1 - i) : win.height - 8.f - lineH * static_cast<float>(i);
            l->setPosition({x, y});
        }
    }
};

// ============================================================ GJBaseGameLayer: stepper, auto clicker, mirror, shake

class $modify(OVBaseLayer, GJBaseGameLayer) {
    bool isThePlayLayer() {
        auto pl = PlayLayer::get();
        return pl && static_cast<GJBaseGameLayer*>(pl) == static_cast<GJBaseGameLayer*>(this);
    }

    void update(float dt) {
        if (ov::on(H::frame_stepper) && isThePlayLayer()) {
            setAudioFrozen(true);
            if (s_stepsQueued > 0) {
                s_stepsQueued--;
                GJBaseGameLayer::update(1.f / 240.f);  // one physics tick
            } else {
                static_cast<OVPlayLayer*>(PlayLayer::get())->updateHud();
            }
            return;
        }
        s_stepsQueued = 0;
        GJBaseGameLayer::update(dt);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        if (!isThePlayLayer()) return;
        auto prog = static_cast<int64_t>(m_gameState.m_currentProgress);
        if (prog == s_autoLastTick) return;  // once per physics tick
        s_autoLastTick = prog;
        bool p2 = ov::on(H::autoclick_p2) && m_levelSettings && m_levelSettings->m_twoPlayerMode;

        // let go of a held click (also when the auto clicker gets switched off mid-hold)
        if (s_autoHoldLeft > 0) {
            s_autoHoldLeft--;
            if (s_autoHoldLeft == 0 || !ov::on(H::autoclicker)) {
                s_autoHoldLeft = 0;
                this->handleButton(false, 1, true);
                if (p2) this->handleButton(false, 1, false);
            }
        }
        if (!ov::on(H::autoclicker)) return;

        int period = std::max(2, static_cast<int>(std::round(240.0 / ov::get(H::autoclick_cps))));
        int hold = std::clamp(static_cast<int>(ov::get(H::autoclick_hold)), 1, period - 1);
        if (++s_autoTick % period == 0 && s_autoHoldLeft == 0) {
            this->handleButton(true, 1, true);
            if (p2) this->handleButton(true, 1, false);
            s_autoHoldLeft = hold;
        }
    }

    void toggleFlipped(bool flip, bool noEffects) {
        if (ov::on(H::no_mirror)) flip = false;
        GJBaseGameLayer::toggleFlipped(flip, noEffects);
    }

    void shakeCamera(float duration, float strength, float interval) {
        if (ov::on(H::no_shake)) return;
        GJBaseGameLayer::shakeCamera(duration, strength, interval);
    }
};

// ============================================================ PlayerObject: jump hack, clicks, death effect

class $modify(OVPlayer, PlayerObject) {
    bool pushButton(PlayerButton button) {
        auto pl = PlayLayer::get();
        bool mine = pl && (this == pl->m_player1 || this == pl->m_player2);
        if (mine && ov::on(H::jump_hack) && button == PlayerButton::Jump) {
            m_isOnGround = true;
            m_isOnGround2 = true;
        }
        bool ret = PlayerObject::pushButton(button);
        if (pl && this == pl->m_player1 && button == PlayerButton::Jump) static_cast<OVPlayLayer*>(pl)->onClick();
        return ret;
    }

    void playDeathEffect() {
        if (ov::on(H::no_death_effect)) return;
        PlayerObject::playDeathEffect();
    }
};

class $modify(OVStreak, HardStreak) {
    struct Fields {
        bool forcedSolid = false;
        bool solidBefore = false;
    };

    void addPoint(CCPoint point) {
        if (ov::on(H::no_wave_trail)) return;
        auto f = m_fields.self();
        if (ov::on(H::solid_wave)) {
            if (!f->forcedSolid) { f->solidBefore = m_isSolid; f->forcedSolid = true; }
            m_isSolid = true;
        } else if (f->forcedSolid) {
            m_isSolid = f->solidBefore;
            f->forcedSolid = false;
        }
        HardStreak::addPoint(point);
    }
};

// ============================================================ global: speedhack, sounds, transitions, unlocks

class $modify(OVScheduler, CCScheduler) {
    void update(float dt) {
        bool speedOn = ov::on(H::speedhack);
        float speed = speedOn ? static_cast<float>(ov::get(H::speed)) : 1.f;

        bool wantPitch = speedOn && ov::on(H::speed_audio) && std::abs(speed - 1.f) > 1e-4f;
        // only touch the sound engine when there's something to do
        if (auto fmod = (wantPitch || s_pitchApplied) ? FMODAudioEngine::get() : nullptr; fmod && fmod->m_globalChannel) {
            if (wantPitch) {
                fmod->m_globalChannel->setPitch(speed);
                s_pitchApplied = true;
            } else if (s_pitchApplied) {
                fmod->m_globalChannel->setPitch(1.f);
                s_pitchApplied = false;
            }
        }
        // never leave the sound paused after the frame stepper is off or you left the level
        if (s_audioFrozen && (!ov::on(H::frame_stepper) || !PlayLayer::get())) setAudioFrozen(false);

        CCScheduler::update(dt * speed);
    }
};

class $modify(OVAudio, FMODAudioEngine) {
    int playEffect(gd::string path) {
        if (ov::on(H::no_death_sound) && std::string_view(path).find("explode_11") != std::string_view::npos) return 0;
        return FMODAudioEngine::playEffect(path);
    }
    int playEffect(gd::string path, float speed, float unknown, float volume) {
        if (ov::on(H::no_death_sound) && std::string_view(path).find("explode_11") != std::string_view::npos) return 0;
        return FMODAudioEngine::playEffect(path, speed, unknown, volume);
    }
};

class $modify(OVFade, CCTransitionFade) {
    static CCTransitionFade* create(float duration, CCScene* scene) {
        return CCTransitionFade::create(ov::on(H::no_transition) ? 0.f : duration, scene);
    }
};

class $modify(OVGameManager, GameManager) {
    bool isIconUnlocked(int id, IconType type) {
        if (ov::on(H::unlock_icons)) return true;
        return GameManager::isIconUnlocked(id, type);
    }
    bool isColorUnlocked(int id, UnlockType type) {
        if (ov::on(H::unlock_icons)) return true;
        return GameManager::isColorUnlocked(id, type);
    }
};

// ============================================================ buttons to open the menu

class $modify(OVPause, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto txt = CCLabelBMFont::create("OV", "bigFont.fnt");
        auto spr = CircleButtonSprite::create(txt, CircleBaseColor::Pink, CircleBaseSize::Small);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(OVPause::onOverload));
        btn->setID("overload-button"_spr);
        if (auto menu = this->getChildByID("right-button-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        } else {
            auto m = CCMenu::create();
            m->addChild(btn);
            auto win = CCDirector::get()->getWinSize();
            m->setPosition({win.width - 36.f, win.height - 36.f});
            this->addChild(m, 10);
        }
    }
    void onOverload(CCObject*) { OverloadMenu::toggle(); }
};

class $modify(OVMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto txt = CCLabelBMFont::create("OV", "bigFont.fnt");
        auto spr = CircleButtonSprite::create(txt, CircleBaseColor::Pink, CircleBaseSize::MediumAlt);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(OVMenuLayer::onOverload));
        btn->setID("overload-button"_spr);
        if (auto menu = this->getChildByID("bottom-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        } else {
            auto m = CCMenu::create();
            m->addChild(btn);
            m->setPosition({40.f, 40.f});
            this->addChild(m, 10);
        }
        return true;
    }
    void onOverload(CCObject*) { OverloadMenu::toggle(); }
};

// ============================================================ keybinds

namespace {
// gameplay keys are ignored while the menu is open, so typing in the search box doesn't flip things
bool gameplayKey(bool down, bool repeat) { return down && !repeat && !OverloadMenu::isOpen(); }
} // namespace

$on_mod(Loaded) {
    loadAll();
    listenForKeybindSettingPresses("open-menu", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) OverloadMenu::toggle();
        return false;
    });
    listenForKeybindSettingPresses("toggle-noclip", [](Keybind const&, bool down, bool repeat, double) {
        if (!gameplayKey(down, repeat)) return false;
        ov::set(H::noclip, ov::on(H::noclip) ? 0 : 1);
        Notification::create(ov::on(H::noclip) ? "Noclip ON" : "Noclip OFF", NotificationIcon::Info, 0.8f)->show();
        return false;
    });
    listenForKeybindSettingPresses("toggle-speedhack", [](Keybind const&, bool down, bool repeat, double) {
        if (!gameplayKey(down, repeat)) return false;
        ov::set(H::speedhack, ov::on(H::speedhack) ? 0 : 1);
        Notification::create(ov::on(H::speedhack) ? fmt::format("Speedhack ON ({:g}x)", ov::get(H::speed)) : std::string("Speedhack OFF"),
                             NotificationIcon::Info, 0.8f)->show();
        return false;
    });
    listenForKeybindSettingPresses("frame-step", [](Keybind const&, bool down, bool, double) {
        // holding the key repeats, which keeps stepping
        if (down && !OverloadMenu::isOpen() && ov::on(H::frame_stepper)) s_stepsQueued = std::min(s_stepsQueued + 1, 8);
        return false;
    });
    listenForKeybindSettingPresses("startpos-prev", [](Keybind const&, bool down, bool repeat, double) {
        if (!gameplayKey(down, repeat) || !ov::on(H::startpos_switcher)) return false;
        if (auto pl = PlayLayer::get()) static_cast<OVPlayLayer*>(pl)->switchStartPos(-1);
        return false;
    });
    listenForKeybindSettingPresses("startpos-next", [](Keybind const&, bool down, bool repeat, double) {
        if (!gameplayKey(down, repeat) || !ov::on(H::startpos_switcher)) return false;
        if (auto pl = PlayLayer::get()) static_cast<OVPlayLayer*>(pl)->switchStartPos(1);
        return false;
    });
}
