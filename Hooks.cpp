// Gameplay, practice tools, HUD, speed, sound and keys. Looks live in Cosmetics.cpp.
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
#include <cmath>
#include <ctime>
#include <deque>

#include "Hacks.hpp"
#include "Menu.hpp"
#include "Util.hpp"

using namespace geode::prelude;
using namespace ov;

namespace {
int s_stepsQueued = 0;        // frame stepper: ticks waiting to run
bool s_audioFrozen = false;   // frame stepper paused the sound
bool s_pitchApplied = false;  // we changed the sound pitch
bool s_ownSound = false;      // we're playing a sound ourselves (skip the mute)
int64_t s_autoLastTick = -1;  // auto clicker
int64_t s_autoTick = 0;
int s_autoHoldLeft = 0;
float s_hudTime = 0.f;

std::string clockText(double s) {
    if (s < 0) s = 0;
    int h = static_cast<int>(s / 3600);
    int m = static_cast<int>(s / 60) % 60;
    double sec = s - h * 3600 - m * 60;
    if (h > 0) return fmt::format("{}:{:02}:{:04.1f}", h, m, sec);
    return fmt::format("{}:{:04.1f}", m, sec);
}

void setAudioFrozen(bool frozen) {
    if (frozen == s_audioFrozen) return;
    auto fmod = FMODAudioEngine::get();
    if (!fmod || !fmod->m_globalChannel) return;
    fmod->m_globalChannel->setPaused(frozen);
    s_audioFrozen = frozen;
}

void notify(std::string const& text) {
    if (on(H::key_notifications)) Notification::create(text, NotificationIcon::Info, 0.8f)->show();
}

std::string gamemodeName(PlayerObject* p) {
    if (p->m_isShip) return p->m_isPlatformer ? "Jetpack" : "Ship";
    if (p->m_isBird) return "UFO";
    if (p->m_isBall) return "Ball";
    if (p->m_isDart) return "Wave";
    if (p->m_isRobot) return "Robot";
    if (p->m_isSpider) return "Spider";
    if (p->m_isSwing) return "Swing";
    return "Cube";
}

std::string portalSpeedName(float s) {
    struct { float v; char const* name; } const table[] = {{0.7f, "0.5x"}, {0.9f, "1x"}, {1.1f, "2x"}, {1.3f, "3x"}, {1.6f, "4x"}};
    char const* best = "1x";
    float bestD = 1e9f;
    for (auto const& t : table)
        if (std::abs(t.v - s) < bestD) { bestD = std::abs(t.v - s); best = t.name; }
    return best;
}

char const* const kHudFonts[] = {"bigFont.fnt", "goldFont.fnt", "chatFont.fnt"};
float const kHudFontScale[] = {0.32f, 0.36f, 0.55f};

void ovOnInput(PlayLayer* pl, bool down);
} // namespace

// ============================================================ PlayLayer: level features, practice tools, HUD

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
        // runs / deaths
        int sessionAttempts = 0;
        double sessionTime = 0;
        float runStart = 0.f;
        float bestFrom = 0.f, bestTo = 0.f;
        int sessionDeaths = 0;
        double deathSum = 0;
        float lastDeath = -1.f, sessionBest = 0.f;
        bool restartQueued = false;
        std::vector<CCNode*> markers;
        // clicks
        std::deque<double> clickTimes;
        int clicksThisAttempt = 0, totalClicks = 0, releases = 0;
        size_t maxCps = 0, bestCpsSession = 0;
        bool holding = false;
        double holdStart = 0, lastHold = 0;
        // startpos
        std::vector<StartPosObject*> startPositions;
        int startPosIndex = -1;
        CCMenu* spMenu = nullptr;
        CCLabelBMFont* spLabel = nullptr;
        // things we hid (so we can undo it)
        HideState pauseBtn, attemptLbl, progressBar, progressFill, percentLbl;
        bool hidP1 = false, hidP2 = false, hidTrail = false, hidCheckpoints = false, ownHitboxes = false;
        // HUD
        CCNode* hud = nullptr;
        CCLayerColor* hudBg = nullptr;
        std::vector<CCLabelBMFont*> hudLines;
        int hudFont = -1;
        int modCount = -1;
        float fpsAccum = 0.f;
        int fpsFrames = 0;
        float fps = 0.f, fpsLow = 0.f, fpsLowCur = 1e9f, fpsLowTimer = 0.f, frameMs = 0.f;
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

        f->hudBg = CCLayerColor::create({0, 0, 0, 110});
        f->hudBg->setID("hud-bg"_spr);
        f->hudBg->setVisible(false);
        uiParent()->addChild(f->hudBg, 9998);
        f->hud = CCNode::create();
        f->hud->setID("hud"_spr);
        uiParent()->addChild(f->hud, 9999);
        f->flash = CCLayerColor::create({255, 0, 0, 0});
        f->flash->setID("noclip-flash"_spr);
        uiParent()->addChild(f->flash, 9997);
        buildStartPosUI();

        if (on(H::auto_practice) && !m_isPracticeMode) this->togglePracticeMode(true);
    }

    void togglePracticeMode(bool practice) {
        if (practice && on(H::practice_music)) m_practiceMusicSync = true;
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
        bool show = on(H::startpos_switcher) && on(H::startpos_buttons) && !f->startPositions.empty();
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
        f->releases = 0;
        f->maxCps = 0;
        f->restartQueued = false;
        f->runStart = percentNow();
        f->cheated = cheating();
        if (on(H::safe_mode) && f->cheated) forceTestMode();
        if (m_attemptLabel && on(H::hide_attempts)) m_attemptLabel->setVisible(false);
    }

    void recordRun(float to) {
        auto f = m_fields.self();
        if (to - f->runStart > f->bestTo - f->bestFrom) {
            f->bestFrom = f->runStart;
            f->bestTo = to;
        }
    }

    void clearMarkers() {
        auto f = m_fields.self();
        for (auto m : f->markers) m->removeFromParent();
        f->markers.clear();
    }

    void addMarker(PlayerObject* p, bool noclip) {
        auto f = m_fields.self();
        if (!p || !p->getParent() || !m_objectLayer) return;
        if (!on(H::death_markers_all)) clearMarkers();
        auto pos = m_objectLayer->convertToNodeSpace(p->getParent()->convertToWorldSpace(p->getPosition()));
        auto lbl = CCLabelBMFont::create("X", "bigFont.fnt");
        lbl->setColor(noclip ? ccColor3B{255, 170, 60} : ccColor3B{255, 60, 60});
        lbl->setScale(0.6f);
        lbl->setOpacity(220);
        lbl->setPosition(pos);
        m_objectLayer->addChild(lbl, 9999);
        f->markers.push_back(lbl);
        if (f->markers.size() > 400) {
            f->markers.front()->removeFromParent();
            f->markers.erase(f->markers.begin());
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (object == m_anticheatSpike) return PlayLayer::destroyPlayer(player, object);
        auto f = m_fields.self();
        bool protect = noclipActive() && !(on(H::noclip_p1) && player == m_player2);
        if (protect) {
            if (!f->diedThisFrame && !f->wasDeadLastFrame && on(H::death_markers) && on(H::death_markers_noclip)) addMarker(player, true);
            f->diedThisFrame = true;
            return;
        }
        bool firstDeath = m_player1 && !m_player1->m_isDead;
        if (firstDeath) {
            float pct = percentNow();
            recordRun(pct);
            f->sessionDeaths++;
            f->deathSum += pct;
            f->lastDeath = pct;
            f->sessionBest = std::max(f->sessionBest, pct);
            if (on(H::death_markers)) addMarker(player, false);
        }
        PlayLayer::destroyPlayer(player, object);

        if (on(H::custom_respawn)) {
            // the game waits about a second (action tag 0x10) before respawning; swap in our own timer
            if (auto act = this->getActionByTag(0x10)) {
                this->stopAction(act);
                auto seq = CCSequence::create(CCDelayTime::create(getf(H::respawn_delay)),
                                              CCCallFunc::create(this, callfunc_selector(PlayLayer::delayedResetLevel)), nullptr);
                seq->setTag(0x10);
                this->runAction(seq);
            }
        }
        if (firstDeath && on(H::pause_on_death)) {
            Loader::get()->queueInMainThread([] {
                if (auto pl = PlayLayer::get())
                    if (!pl->m_isPaused && !pl->m_hasCompletedLevel) pl->pauseGame(false);
            });
        }
    }

    void levelComplete() {
        recordRun(100.f);
        auto f = m_fields.self();
        f->sessionBest = 100.f;
        PlayLayer::levelComplete();
    }

    void showNewBest(bool newReward, int orbs, int diamonds, bool demonKey, bool noRetry, bool noTitle) {
        if (on(H::no_new_best)) return;
        PlayLayer::showNewBest(newReward, orbs, diamonds, demonKey, noRetry, noTitle);
    }

    void updateProgressbar() {
        PlayLayer::updateProgressbar();
        if (on(H::accurate_percent) && m_percentageLabel && !isPlatformer())
            m_percentageLabel->setString(fmt::format("{:.{}f}%", percentNow(), geti(H::percent_decimals)).c_str());
    }

    // ---------------- every frame

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto f = m_fields.self();
        float realDt = CCDirector::get()->getDeltaTime();
        f->sessionTime += realDt;
        s_hudTime += realDt;

        // --- noclip accuracy (counted in physics ticks)
        int64_t prog = static_cast<int64_t>(m_gameState.m_currentProgress);
        int64_t adv = std::max<int64_t>(0, prog - f->lastProgress);
        f->lastProgress = prog;
        f->ticks += adv;
        if (f->diedThisFrame) {
            f->deadTicks += std::max<int64_t>(1, adv);
            if (!f->wasDeadLastFrame) {
                f->noclipDeaths++;
                if (on(H::noclip_flash) && f->flash) {
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
        if (on(H::safe_mode) && f->cheated) forceTestMode();
        else if (!on(H::safe_mode)) unforceTestMode();

        // --- auto restart at %
        if (on(H::auto_restart) && !isPlatformer() && !f->restartQueued && m_player1 && !m_player1->m_isDead && !m_levelEndAnimationStarted) {
            float target = getf(H::restart_percent);
            if (f->runStart < target && percentNow() >= target) {
                f->restartQueued = true;
                Loader::get()->queueInMainThread([] {
                    if (auto pl = PlayLayer::get())
                        if (!pl->m_isPaused) pl->resetLevel();
                });
            }
        }
        if (!on(H::death_markers) && !f->markers.empty()) clearMarkers();

        // --- hitboxes
        bool wantHitboxes = on(H::hitboxes) || (on(H::hitboxes_on_death) && m_player1 && m_player1->m_isDead);
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

        // --- hide players / trail
        bool hide1 = on(H::hide_player), hide2 = on(H::hide_player) || on(H::hide_player2);
        if (m_player1) {
            if (hide1) { m_player1->setVisible(false); f->hidP1 = true; }
            else if (f->hidP1) { m_player1->setVisible(!m_player1->m_isDead); f->hidP1 = false; }
        }
        if (m_player2) {
            if (hide2) { m_player2->setVisible(false); f->hidP2 = true; }
            else if (f->hidP2) { m_player2->setVisible(m_gameState.m_isDualMode && !m_player2->m_isDead); f->hidP2 = false; }
        }
        if (on(H::no_trail)) {
            for (auto p : {m_player1, m_player2})
                if (p && p->m_regularTrail) p->m_regularTrail->setVisible(false);
            f->hidTrail = true;
        } else if (f->hidTrail) {
            for (auto p : {m_player1, m_player2})
                if (p && p->m_regularTrail) p->m_regularTrail->setVisible(true);
            f->hidTrail = false;
        }

        // --- UI bits (we only touch what we hid, so the game's own settings keep working)
        if (m_uiLayer) {
            f->pauseBtn.apply(m_uiLayer->m_pauseBtn, on(H::hide_pause));
            if (m_uiLayer->m_checkpointMenu) {
                if (on(H::hide_practice_btns)) {
                    m_uiLayer->m_checkpointMenu->setVisible(false);
                    f->hidCheckpoints = true;
                } else if (f->hidCheckpoints) {
                    m_uiLayer->m_checkpointMenu->setVisible(m_isPracticeMode);
                    f->hidCheckpoints = false;
                }
            }
        }
        f->attemptLbl.apply(m_attemptLabel, on(H::hide_attempts));
        f->progressBar.apply(m_progressBar, on(H::hide_progress));
        f->progressFill.apply(m_progressFill, on(H::hide_progress));
        f->percentLbl.apply(m_percentageLabel, on(H::hide_percent));
        updateStartPosUI();

        // --- clicks per second window
        double now = nowSeconds();
        while (!f->clickTimes.empty() && now - f->clickTimes.front() > 1.0) f->clickTimes.pop_front();
        f->maxCps = std::max(f->maxCps, f->clickTimes.size());
        f->bestCpsSession = std::max(f->bestCpsSession, f->clickTimes.size());

        // --- fps
        f->fpsAccum += realDt;
        f->fpsFrames++;
        if (f->fpsAccum >= 0.25f) {
            f->fps = static_cast<float>(f->fpsFrames) / f->fpsAccum;
            f->fpsAccum = 0.f;
            f->fpsFrames = 0;
        }
        if (realDt > 0) f->fpsLowCur = std::min(f->fpsLowCur, 1.f / realDt);
        f->fpsLowTimer += realDt;
        if (f->fpsLowTimer >= 1.f) {
            f->fpsLow = f->fpsLowCur;
            f->fpsLowCur = 1e9f;
            f->fpsLowTimer = 0.f;
        }
        f->frameMs = f->frameMs * 0.9f + realDt * 1000.f * 0.1f;

        updateHud();
    }

    void onInput(bool down) {
        auto f = m_fields.self();
        double now = nowSeconds();
        if (down) {
            f->clickTimes.push_back(now);
            f->clicksThisAttempt++;
            f->totalClicks++;
            f->holding = true;
            f->holdStart = now;
            if (on(H::click_sound)) {
                s_ownSound = true;
                FMODAudioEngine::get()->playEffect("playSound_01.ogg", 1.f, 0.f, getf(H::click_volume));
                s_ownSound = false;
            }
        } else {
            f->releases++;
            if (f->holding) f->lastHold = now - f->holdStart;
            f->holding = false;
        }
    }

    // ---------------- HUD

    void updateHud() {
        auto f = m_fields.self();
        if (!f->hud) return;
        bool show = on(H::hud);
        f->hud->setVisible(show);
        if (f->hudBg) f->hudBg->setVisible(false);
        if (!show) return;

        std::vector<std::pair<std::string, ccColor3B>> lines;
        ccColor3B const white{255, 255, 255}, warn{255, 200, 120}, info{150, 220, 255};
        auto line = [&](H h, auto&& text, ccColor3B c = {255, 255, 255}) {
            if (on(h)) lines.push_back({text(), c});
        };
        auto p1 = m_player1;

        if (on(H::hud_cheat)) {
            if (cheating()) lines.push_back({on(H::safe_mode) ? "CHEATING (safe mode on)" : "CHEATING - SAFE MODE OFF", {255, 90, 90}});
            else if (f->cheated) lines.push_back({"Cheated this attempt (won't save)", {255, 170, 90}});
            else lines.push_back({"Legit", {110, 240, 120}});
        }
        line(H::hud_fps, [&] { return fmt::format("FPS {:.0f}", f->fps); });
        line(H::hud_fps_low, [&] { return fmt::format("Lowest FPS {:.0f}", f->fpsLow); });
        line(H::hud_frame_ms, [&] { return fmt::format("Frame {:.1f} ms", f->frameMs); });
        line(H::hud_cps, [&] { return fmt::format("CPS {} (best {})  {} clicks", f->clickTimes.size(), f->maxCps, f->clicksThisAttempt); });
        line(H::hud_best_cps_session, [&] { return fmt::format("Best CPS this session {}", f->bestCpsSession); });
        line(H::hud_total_clicks, [&] { return fmt::format("Total clicks {}", f->totalClicks); });
        line(H::hud_releases, [&] { return fmt::format("Releases {}", f->releases); });
        if (on(H::hud_holding)) lines.push_back({f->holding ? "HOLDING" : "Released", f->holding ? ccColor3B{120, 255, 140} : white});
        line(H::hud_hold_time, [&] { return fmt::format("Hold {:.2f}s", f->holding ? nowSeconds() - f->holdStart : f->lastHold); });
        if (noclipActive()) {
            double acc = f->ticks > 0 ? 100.0 * (1.0 - static_cast<double>(f->deadTicks) / static_cast<double>(f->ticks)) : 100.0;
            line(H::hud_noclip_acc, [&] { return fmt::format("Accuracy {:.2f}%", acc); }, acc >= 100.0 ? white : ccColor3B{255, 160, 160});
            line(H::hud_noclip_deaths, [&] { return fmt::format("Noclip deaths {}", f->noclipDeaths); });
        }
        line(H::hud_attempts, [&] { return fmt::format("Session attempts {}", f->sessionAttempts); });
        line(H::hud_attempt_gd, [&] { return fmt::format("Attempt {}", m_attempts); });
        line(H::hud_session_deaths, [&] { return fmt::format("Session deaths {}", f->sessionDeaths); });
        if (!isPlatformer()) {
            if (f->lastDeath >= 0) line(H::hud_last_death, [&] { return fmt::format("Last death {:.1f}%", f->lastDeath); });
            if (f->sessionDeaths > 0) line(H::hud_avg_death, [&] { return fmt::format("Average death {:.1f}%", f->deathSum / f->sessionDeaths); });
            line(H::hud_session_best, [&] { return fmt::format("Session best {:.1f}%", f->sessionBest); });
            if (f->bestTo > 0) line(H::hud_best_run, [&] { return fmt::format("Best run {:.0f}-{:.0f}%", f->bestFrom, f->bestTo); });
            line(H::hud_start_percent, [&] { return fmt::format("Started at {:.1f}%", f->runStart); });
            line(H::hud_percent_left, [&] { return fmt::format("{:.1f}% left", 100.f - percentNow()); });
        }
        line(H::hud_time, [&] { return fmt::format("Attempt time {}", clockText(m_attemptTime)); });
        line(H::hud_session_time, [&] { return fmt::format("Time in level {}", clockText(f->sessionTime)); });
        line(H::hud_level_time, [&] { return fmt::format("Level time {:.2f}s", m_gameState.m_levelTime); });
        line(H::hud_tick, [&] { return fmt::format("Tick {}", static_cast<int64_t>(m_gameState.m_currentProgress)); });
        line(H::hud_jumps, [&] { return fmt::format("Jumps {}", m_jumps); });
        if (p1) {
            line(H::hud_xpos, [&] { return fmt::format("X {:.1f}", p1->getPositionX()); });
            line(H::hud_ypos, [&] { return fmt::format("Y {:.1f}", p1->getPositionY()); });
            line(H::hud_yvel, [&] { return fmt::format("Y speed {:.2f}", p1->m_yVelocity); });
            line(H::hud_rotation, [&] { return fmt::format("Rotation {:.0f}", std::fmod(p1->getRotation(), 360.f)); });
            line(H::hud_gamemode, [&] { return gamemodeName(p1); }, info);
            line(H::hud_portal_speed, [&] { return fmt::format("Speed {}", portalSpeedName(p1->m_playerSpeed)); });
            line(H::hud_gravity, [&] { return std::string(p1->m_isUpsideDown ? "Gravity: flipped" : "Gravity: normal"); });
            line(H::hud_size_mode, [&] { return std::string(p1->m_vehicleSize < 0.99f ? "Size: mini" : "Size: normal"); });
            if (m_gameState.m_isDualMode) line(H::hud_dual, [] { return std::string("Dual"); }, info);
            if (p1->m_isOnGround) line(H::hud_on_ground, [] { return std::string("On ground"); });
            if (p1->m_isDashing) line(H::hud_dashing, [] { return std::string("Dashing"); }, warn);
            line(H::hud_direction, [&] { return std::string(p1->m_isGoingLeft ? "Facing left" : "Facing right"); });
        }
        line(H::hud_zoom, [&] { return fmt::format("Zoom {:.2f}", m_gameState.m_cameraZoom); });
        if (m_isPracticeMode)
            line(H::hud_checkpoints, [&] { return fmt::format("Checkpoints {}", m_checkpointArray ? m_checkpointArray->count() : 0u); });
        if (m_level) {
            line(H::hud_level_name, [&] { return std::string(m_level->m_levelName); });
            if (!std::string(m_level->m_creatorName).empty())
                line(H::hud_creator, [&] { return fmt::format("by {}", std::string(m_level->m_creatorName)); });
            line(H::hud_level_id, [&] { return fmt::format("ID {}", m_level->m_levelID.value()); });
            line(H::hud_stars, [&] { return fmt::format("{} stars", m_level->m_stars.value()); });
            line(H::hud_object_count, [&] {
                int n = m_level->m_objectCount.value();
                if (n <= 0 && m_objects) n = static_cast<int>(m_objects->count());
                return fmt::format("{} objects", n);
            });
            line(H::hud_song_id, [&] {
                return m_level->m_songID > 0 ? fmt::format("Song ID {}", m_level->m_songID)
                                              : fmt::format("Built-in song {}", m_level->m_audioTrack + 1);
            });
            line(H::hud_level_attempts, [&] { return fmt::format("Total attempts {}", m_level->m_attempts.value()); });
            line(H::hud_level_jumps, [&] { return fmt::format("Total jumps {}", m_level->m_jumps.value()); });
            line(H::hud_normal_best, [&] { return fmt::format("Best {}%", m_level->m_normalPercent.value()); });
            line(H::hud_practice_best, [&] { return fmt::format("Practice best {}%", m_level->m_practicePercent); });
        }
        if (on(H::startpos_switcher) && !f->startPositions.empty())
            line(H::hud_startpos, [&] { return fmt::format("StartPos {}/{}", f->startPosIndex + 1, f->startPositions.size()); }, info);
        if (on(H::hud_clock) || on(H::hud_date)) {
            std::time_t t = std::time(nullptr);
            std::tm tm{};
#ifdef GEODE_IS_WINDOWS
            localtime_s(&tm, &t);
#else
            localtime_r(&t, &tm);
#endif
            line(H::hud_clock, [&] { return fmt::format("{:02}:{:02}:{:02}", tm.tm_hour, tm.tm_min, tm.tm_sec); });
            line(H::hud_date, [&] { return fmt::format("{}-{:02}-{:02}", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday); });
        }
        if (on(H::hud_mod_count)) {
            if (f->modCount < 0) {
                f->modCount = 0;
                for (auto mod : Loader::get()->getAllMods())
                    if (mod->isLoaded()) f->modCount++;
            }
            lines.push_back({fmt::format("{} mods", f->modCount), white});
        }
        line(H::hud_window_size, [&] {
            auto s = CCEGLView::get()->getFrameSize();
            return fmt::format("{}x{}", static_cast<int>(s.width), static_cast<int>(s.height));
        });
        if (on(H::hud_custom_text)) {
            auto txt = Mod::get()->getSettingValue<std::string>("custom-hud-text");
            if (!txt.empty()) lines.push_back({txt, white});
        }
        if (std::abs(levelSpeed() - 1.f) > 1e-4f) lines.push_back({fmt::format("Speed {:g}x", levelSpeed()), warn});
        if (on(H::frame_stepper)) lines.push_back({"Frame stepper: press F to step", warn});
        if (on(H::autoclicker)) lines.push_back({fmt::format("Auto clicker {:g} CPS", ov::get(H::autoclick_cps)), warn});

        // ---- draw
        int font = std::clamp(geti(H::hud_font), 0, 2);
        if (font != f->hudFont) {
            for (auto l : f->hudLines) l->removeFromParent();
            f->hudLines.clear();
            f->hudFont = font;
        }
        while (f->hudLines.size() < lines.size()) {
            auto l = CCLabelBMFont::create("", kHudFonts[font]);
            if (!l) l = CCLabelBMFont::create("", "bigFont.fnt");
            f->hud->addChild(l);
            f->hudLines.push_back(l);
        }
        auto win = CCDirector::get()->getWinSize();
        float scale = kHudFontScale[font] * getf(H::hud_scale);
        int corner = geti(H::hud_corner);
        bool right = corner == 1 || corner == 3;
        bool bottom = corner >= 2;
        auto opacity = static_cast<GLubyte>(255 * ov::get(H::hud_opacity));
        int colorMode = geti(H::hud_color_mode);
        float lineH = 0.f;
        float maxW = 0.f;
        for (size_t i = 0; i < f->hudLines.size(); i++) {
            auto l = f->hudLines[i];
            if (i >= lines.size()) { l->setVisible(false); continue; }
            l->setVisible(true);
            l->setString(lines[i].first.c_str());
            l->setScale(scale);
            ccColor3B c = lines[i].second;
            switch (colorMode) {
                case 1: c = white; break;
                case 2: c = OverloadMenu::accent(); break;
                case 3: c = hsv(s_hudTime * 0.3f * getf(H::rainbow_speed) + static_cast<float>(i) * 0.04f, 0.6f, 1.f); break;
                case 4: c = rgb(geti(H::hud_r), geti(H::hud_g), geti(H::hud_b)); break;
                default: break;
            }
            l->setColor(c);
            l->setOpacity(opacity);
            if (lineH <= 0.f) lineH = l->getContentSize().height * scale * 0.82f * getf(H::hud_spacing) + 1.f;
            maxW = std::max(maxW, l->getContentSize().width * scale);
        }
        float x0 = (right ? win.width - 6.f : 6.f) + getf(H::hud_offset_x);
        float n = static_cast<float>(lines.size());
        for (size_t i = 0; i < lines.size(); i++) {
            auto l = f->hudLines[i];
            l->setAnchorPoint({right ? 1.f : 0.f, 0.5f});
            float y = bottom ? 8.f + lineH * (n - 1.f - static_cast<float>(i)) : win.height - 8.f - lineH * static_cast<float>(i);
            l->setPosition({x0, y + getf(H::hud_offset_y)});
        }
        if (on(H::hud_bg) && f->hudBg && !lines.empty()) {
            float w = maxW + 8.f, h = lineH * n + 6.f;
            float bx = right ? x0 - maxW - 4.f : x0 - 4.f;
            float topY = bottom ? 8.f + lineH * (n - 1.f) + lineH * 0.5f : win.height - 8.f + lineH * 0.5f;
            f->hudBg->setContentSize({w, h});
            f->hudBg->setPosition({bx, topY - h + 3.f + getf(H::hud_offset_y)});
            f->hudBg->setOpacity(static_cast<GLubyte>(110 * ov::get(H::hud_opacity)));
            f->hudBg->setVisible(true);
        }
    }
};

namespace {
void ovOnInput(PlayLayer* pl, bool down) { static_cast<OVPlayLayer*>(pl)->onInput(down); }
} // namespace

// ============================================================ GJBaseGameLayer: stepper, auto clicker, mirror, shake

class $modify(OVBaseLayer, GJBaseGameLayer) {
    bool isThePlayLayer() {
        auto pl = PlayLayer::get();
        return pl && static_cast<GJBaseGameLayer*>(pl) == static_cast<GJBaseGameLayer*>(this);
    }

    void update(float dt) {
        if (on(H::frame_stepper) && isThePlayLayer()) {
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
        bool p2 = on(H::autoclick_p2) && m_levelSettings && m_levelSettings->m_twoPlayerMode;

        // let go of a held click (also when the auto clicker gets switched off mid-hold)
        if (s_autoHoldLeft > 0) {
            s_autoHoldLeft--;
            if (s_autoHoldLeft == 0 || !on(H::autoclicker)) {
                s_autoHoldLeft = 0;
                this->handleButton(false, 1, true);
                if (p2) this->handleButton(false, 1, false);
            }
        }
        if (!on(H::autoclicker)) return;

        int period = std::max(2, static_cast<int>(std::round(240.0 / ov::get(H::autoclick_cps))));
        int hold = std::clamp(geti(H::autoclick_hold), 1, period - 1);
        if (++s_autoTick % period == 0 && s_autoHoldLeft == 0) {
            this->handleButton(true, 1, true);
            if (p2) this->handleButton(true, 1, false);
            s_autoHoldLeft = hold;
        }
    }

    void toggleFlipped(bool flip, bool noEffects) {
        if (on(H::no_mirror)) flip = false;
        GJBaseGameLayer::toggleFlipped(flip, noEffects);
    }

    void shakeCamera(float duration, float strength, float interval) {
        if (on(H::no_shake)) return;
        GJBaseGameLayer::shakeCamera(duration, strength, interval);
    }
};

// ============================================================ PlayerObject: jump hack, clicks, spider line

class $modify(OVPlayer, PlayerObject) {
    bool pushButton(PlayerButton button) {
        auto pl = PlayLayer::get();
        bool mine = pl && (this == pl->m_player1 || this == pl->m_player2);
        if (mine && on(H::jump_hack) && button == PlayerButton::Jump) {
            m_isOnGround = true;
            m_isOnGround2 = true;
        }
        bool ret = PlayerObject::pushButton(button);
        if (pl && this == pl->m_player1 && button == PlayerButton::Jump) ovOnInput(pl, true);
        return ret;
    }

    bool releaseButton(PlayerButton button) {
        bool ret = PlayerObject::releaseButton(button);
        auto pl = PlayLayer::get();
        if (pl && this == pl->m_player1 && button == PlayerButton::Jump) ovOnInput(pl, false);
        return ret;
    }

    void playSpiderDashEffect(CCPoint from, CCPoint to) {
        if (on(H::no_spider_line)) return;
        PlayerObject::playSpiderDashEffect(from, to);
    }
};

class $modify(OVStreak, HardStreak) {
    struct Fields {
        bool forcedSolid = false;
        bool solidBefore = false;
    };

    void addPoint(CCPoint point) {
        if (on(H::no_wave_trail)) return;
        auto f = m_fields.self();
        if (on(H::solid_wave)) {
            if (!f->forcedSolid) { f->solidBefore = m_isSolid; f->forcedSolid = true; }
            m_isSolid = true;
        } else if (f->forcedSolid) {
            m_isSolid = f->solidBefore;
            f->forcedSolid = false;
        }
        HardStreak::addPoint(point);
    }
};

// ============================================================ global: speed, sounds, transitions, unlocks

class $modify(OVScheduler, CCScheduler) {
    void update(float dt) {
        if (g_slowmoHeld && !PlayLayer::get()) g_slowmoHeld = false;
        float speed = effectiveSpeed();

        float pitch = (on(H::speed_audio) ? speed : 1.f) * (on(H::pitch_on) ? getf(H::pitch) : 1.f);
        bool wantPitch = std::abs(pitch - 1.f) > 1e-4f;
        // only touch the sound engine when there's something to do
        if (auto fmod = (wantPitch || s_pitchApplied) ? FMODAudioEngine::get() : nullptr; fmod && fmod->m_globalChannel) {
            if (wantPitch) {
                fmod->m_globalChannel->setPitch(pitch);
                s_pitchApplied = true;
            } else {
                fmod->m_globalChannel->setPitch(1.f);
                s_pitchApplied = false;
            }
        }
        // never leave the sound paused after the frame stepper is off or you left the level
        if (s_audioFrozen && (!on(H::frame_stepper) || !PlayLayer::get())) setAudioFrozen(false);

        CCScheduler::update(dt * speed);
    }
};

class $modify(OVAudio, FMODAudioEngine) {
    static bool muted(gd::string const& path) {
        if (s_ownSound) return false;
        if (on(H::no_death_sound) && std::string_view(path).find("explode_11") != std::string_view::npos) return true;
        if (on(H::mute_sfx) && PlayLayer::get()) return true;
        return false;
    }
    int playEffect(gd::string path) {
        if (muted(path)) return 0;
        return FMODAudioEngine::playEffect(path);
    }
    int playEffect(gd::string path, float speed, float unknown, float volume) {
        if (muted(path)) return 0;
        return FMODAudioEngine::playEffect(path, speed, unknown, volume);
    }
};

class $modify(OVFade, CCTransitionFade) {
    static CCTransitionFade* create(float duration, CCScene* scene) {
        return CCTransitionFade::create(on(H::no_transition) ? 0.f : duration, scene);
    }
};

class $modify(OVGameManager, GameManager) {
    bool isIconUnlocked(int id, IconType type) {
        if (on(H::unlock_icons)) return true;
        return GameManager::isIconUnlocked(id, type);
    }
    bool isColorUnlocked(int id, UnlockType type) {
        if (on(H::unlock_icons)) return true;
        return GameManager::isColorUnlocked(id, type);
    }
};

// ============================================================ buttons to open the menu

class $modify(OVPause, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (!on(H::pause_button)) return;
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
        if (!on(H::main_menu_button)) return true;
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

void toggleOption(H h, char const* name) {
    set(h, on(h) ? 0 : 1);
    notify(fmt::format("{} {}", name, on(h) ? "ON" : "OFF"));
}
} // namespace

$on_mod(Loaded) {
    loadAll();
    listenForKeybindSettingPresses("open-menu", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) OverloadMenu::toggle();
        return false;
    });
    listenForKeybindSettingPresses("toggle-noclip", [](Keybind const&, bool down, bool repeat, double) {
        if (gameplayKey(down, repeat)) toggleOption(H::noclip, "Noclip");
        return false;
    });
    listenForKeybindSettingPresses("toggle-speedhack", [](Keybind const&, bool down, bool repeat, double) {
        if (!gameplayKey(down, repeat)) return false;
        set(H::speedhack, on(H::speedhack) ? 0 : 1);
        notify(on(H::speedhack) ? fmt::format("Speedhack ON ({:g}x)", ov::get(H::speed)) : std::string("Speedhack OFF"));
        return false;
    });
    listenForKeybindSettingPresses("speed-up", [](Keybind const&, bool down, bool, double) {
        if (!down || OverloadMenu::isOpen()) return false;
        set(H::speed, ov::get(H::speed) + ov::get(H::speed_step));
        set(H::speedhack, 1);
        notify(fmt::format("Speed {:g}x", ov::get(H::speed)));
        return false;
    });
    listenForKeybindSettingPresses("speed-down", [](Keybind const&, bool down, bool, double) {
        if (!down || OverloadMenu::isOpen()) return false;
        set(H::speed, ov::get(H::speed) - ov::get(H::speed_step));
        set(H::speedhack, 1);
        notify(fmt::format("Speed {:g}x", ov::get(H::speed)));
        return false;
    });
    listenForKeybindSettingPresses("slowmo-hold", [](Keybind const&, bool down, bool repeat, double) {
        if (repeat) return false;
        g_slowmoHeld = down && PlayLayer::get() && !OverloadMenu::isOpen();
        return false;
    });
    listenForKeybindSettingPresses("toggle-hud", [](Keybind const&, bool down, bool repeat, double) {
        if (gameplayKey(down, repeat)) toggleOption(H::hud, "HUD");
        return false;
    });
    listenForKeybindSettingPresses("toggle-hitboxes", [](Keybind const&, bool down, bool repeat, double) {
        if (gameplayKey(down, repeat)) toggleOption(H::hitboxes, "Hitboxes");
        return false;
    });
    listenForKeybindSettingPresses("toggle-autoclicker", [](Keybind const&, bool down, bool repeat, double) {
        if (gameplayKey(down, repeat)) toggleOption(H::autoclicker, "Auto clicker");
        return false;
    });
    listenForKeybindSettingPresses("toggle-jump-hack", [](Keybind const&, bool down, bool repeat, double) {
        if (gameplayKey(down, repeat)) toggleOption(H::jump_hack, "Jump hack");
        return false;
    });
    listenForKeybindSettingPresses("restart-level", [](Keybind const&, bool down, bool repeat, double) {
        if (!gameplayKey(down, repeat)) return false;
        if (auto pl = PlayLayer::get())
            if (!pl->m_isPaused) pl->resetLevel();
        return false;
    });
    listenForKeybindSettingPresses("frame-step", [](Keybind const&, bool down, bool, double) {
        // holding the key repeats, which keeps stepping
        if (down && !OverloadMenu::isOpen() && on(H::frame_stepper)) s_stepsQueued = std::min(s_stepsQueued + 1, 8);
        return false;
    });
    listenForKeybindSettingPresses("startpos-prev", [](Keybind const&, bool down, bool repeat, double) {
        if (!gameplayKey(down, repeat) || !on(H::startpos_switcher)) return false;
        if (auto pl = PlayLayer::get()) static_cast<OVPlayLayer*>(pl)->switchStartPos(-1);
        return false;
    });
    listenForKeybindSettingPresses("startpos-next", [](Keybind const&, bool down, bool repeat, double) {
        if (!gameplayKey(down, repeat) || !on(H::startpos_switcher)) return false;
        if (auto pl = PlayLayer::get()) static_cast<OVPlayLayer*>(pl)->switchStartPos(1);
        return false;
    });
}
