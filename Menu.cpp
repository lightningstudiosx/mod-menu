#include "Menu.hpp"

#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/CCMenuItemToggler.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/SimplePlayer.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <optional>

#include "Hacks.hpp"
#include "Util.hpp"

using namespace geode::prelude;

namespace ov {

namespace {
OverloadMenu* s_open = nullptr;
constexpr float kW = 470.f, kH = 290.f;
constexpr float kRowH = 36.f;
constexpr int kTagBase = 1000;   // + def index
constexpr int kStepUp = 100000;  // tag offset for "+" buttons
constexpr float kListX = 122.f;

std::string lower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool endsWith(std::string_view s, std::string_view suffix) {
    return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

std::string valueText(HackDef const& d) {
    double v = get(d.h);
    if (d.type == Type::Choice) {
        auto ch = choicesOf(d);
        int i = static_cast<int>(v);
        return i >= 0 && i < static_cast<int>(ch.size()) ? ch[static_cast<size_t>(i)] : "?";
    }
    if (d.type == Type::Int) return fmt::format("{}", static_cast<int>(v));
    return fmt::format("{:g}", std::round(v * 1000.0) / 1000.0);
}

double stepOf(HackDef const& d) {
    double range = d.max - d.min;
    if (d.type == Type::Choice) return 1;
    if (d.type == Type::Int) return std::abs(range - 255) < 0.5 ? 5 : 1;  // colours step by 5, icons by 1
    if (range <= 1.01) return 0.05;
    if (range <= 3.01) return 0.1;
    if (range <= 10.01) return 0.25;
    if (range <= 100.01) return 1;
    return 5;
}

std::optional<IconType> iconTypeFor(H h) {
    switch (h) {
        case H::icon_cube: return IconType::Cube;
        case H::icon_ship: return IconType::Ship;
        case H::icon_ball: return IconType::Ball;
        case H::icon_ufo: return IconType::Ufo;
        case H::icon_wave: return IconType::Wave;
        case H::icon_robot: return IconType::Robot;
        case H::icon_spider: return IconType::Spider;
        case H::icon_swing: return IconType::Swing;
        case H::icon_jetpack: return IconType::Jetpack;
        default: return std::nullopt;
    }
}

// -1 = none, 0 = GD colour swatch, 1 = RGB swatch (on the "blue" row), 2 = icon
int previewKind(size_t i) {
    auto const& d = defs()[i];
    if (d.type != Type::Int) return -1;
    if (iconTypeFor(d.h)) return 2;
    std::string_view key = d.key;
    if (endsWith(key, "_idx")) return 0;
    if (endsWith(key, "_b") && i >= 2 && endsWith(defs()[i - 1].key, "_g") && endsWith(defs()[i - 2].key, "_r")) return 1;
    return -1;
}

ccColor3B previewColor(size_t i, int kind) {
    if (kind == 0) return GameManager::get()->colorForIdx(geti(defs()[i].h));
    return rgb(geti(defs()[i - 2].h), geti(defs()[i - 1].h), geti(defs()[i].h));
}

int clampIcon(IconType t, int v) {
    int count = GameManager::get()->countForType(t);
    return count > 0 ? std::clamp(v, 1, count) : std::max(1, v);
}
} // namespace

ccColor3B OverloadMenu::accent() {
    static ccColor3B const colors[] = {
        {90, 180, 255}, {180, 120, 255}, {90, 230, 120}, {255, 90, 90}, {255, 120, 200}, {255, 205, 70},
    };
    return colors[std::clamp(geti(H::menu_color), 0, 5)];
}

bool OverloadMenu::isOpen() { return s_open != nullptr; }

void OverloadMenu::toggle() {
    if (s_open) {
        s_open->onClose(nullptr);
        return;
    }
    if (on(H::menu_pause))
        if (auto pl = PlayLayer::get())
            if (!pl->m_isPaused && !pl->m_hasCompletedLevel) pl->pauseGame(false);
    if (auto m = create()) m->show();
}

OverloadMenu* OverloadMenu::create() {
    auto ret = new OverloadMenu();
    if (ret->setup()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

OverloadMenu::~OverloadMenu() {
    if (s_open == this) s_open = nullptr;
}

bool OverloadMenu::setup() {
    if (!Popup::init(kW, kH, "GJ_square05.png")) return false;
    s_open = this;
    m_mainLayer->setScale(getf(H::menu_scale));
    this->setOpacity(static_cast<GLubyte>(geti(H::menu_dim)));

    // title + option count, left of the search box so they never overlap
    m_titleLabel = CCLabelBMFont::create("OVERLOAD MENU", "goldFont.fnt");
    m_titleLabel->limitLabelWidth(200.f, 0.75f, 0.3f);
    m_titleLabel->setColor(accent());
    m_titleLabel->setPosition({150.f, kH - 20.f});
    m_mainLayer->addChild(m_titleLabel, 5);
    auto count = CCLabelBMFont::create(fmt::format("{} options", defs().size()).c_str(), "chatFont.fnt");
    count->setScale(0.5f);
    count->setOpacity(170);
    count->setAnchorPoint({0.f, 0.5f});
    count->setPosition({258.f, kH - 22.f});
    m_mainLayer->addChild(count, 5);

    m_category = categories().empty() ? "" : categories().front();
    if (on(H::menu_remember_tab)) {
        auto saved = Mod::get()->getSavedValue<std::string>("last-tab", m_category);
        if (std::find(categories().begin(), categories().end(), saved) != categories().end()) m_category = saved;
    }

    // left: categories (two columns)
    m_catMenu = CCMenu::create();
    m_catMenu->setPosition({0.f, 0.f});
    m_catMenu->setContentSize({kW, kH});
    m_mainLayer->addChild(m_catMenu);

    // top right: search
    m_searchInput = TextInput::create(110.f, "Search...", "chatFont.fnt");
    m_searchInput->setCallback([this](std::string const& s) {
        m_search = lower(s);
        this->buildCategories();
        this->buildRows();
    });
    m_mainLayer->addChildAtPosition(m_searchInput, Anchor::TopRight, {-68.f, -22.f});

    // right: the list
    m_listW = kW - kListX - 10.f;
    m_listH = kH - 66.f;
    auto bg = CCLayerColor::create({0, 0, 0, 110}, m_listW, m_listH);
    bg->setPosition({kListX, 22.f});
    m_mainLayer->addChild(bg);
    auto bar = CCLayerColor::create({accent().r, accent().g, accent().b, 200}, m_listW, 2.f);
    bar->setPosition({kListX, 22.f + m_listH});
    m_mainLayer->addChild(bar);

    m_scroll = ScrollLayer::create(CCRect(0, 0, m_listW, m_listH));
    m_scroll->setPosition({kListX, 22.f});
    m_mainLayer->addChild(m_scroll);

    m_cheatLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_cheatLabel->setScale(0.55f);
    m_cheatLabel->setAnchorPoint({0.f, 0.5f});
    m_cheatLabel->setPosition({kListX, 12.f});
    m_mainLayer->addChild(m_cheatLabel);

    buildCategories();
    buildRows();
    return true;
}

void OverloadMenu::onClose(CCObject* sender) {
    if (s_open == this) s_open = nullptr;
    Popup::onClose(sender);
}

void OverloadMenu::buildCategories() {
    m_catMenu->removeAllChildren();
    auto const& cats = categories();
    for (size_t i = 0; i < cats.size(); i++) {
        bool sel = cats[i] == m_category && m_search.empty();
        auto spr = ButtonSprite::create(cats[i].c_str(), 48, true, "bigFont.fnt", sel ? "GJ_button_01.png" : "GJ_button_04.png", 22.f, 0.45f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(OverloadMenu::onCategory));
        btn->setTag(static_cast<int>(i));
        float x = (i % 2 == 0) ? 35.f : 89.f;
        float y = kH - 52.f - 27.f * static_cast<float>(i / 2);
        btn->setPosition({x, y});
        m_catMenu->addChild(btn);
    }
    auto resetSpr = ButtonSprite::create("Reset all", 90, true, "goldFont.fnt", "GJ_button_06.png", 24.f, 0.5f);
    auto reset = CCMenuItemSpriteExtra::create(resetSpr, this, menu_selector(OverloadMenu::onResetAll));
    reset->setPosition({62.f, 24.f});
    m_catMenu->addChild(reset);
    handleTouchPriority(this);
}

void OverloadMenu::addPreview(size_t i, CCNode* content, float x, float y) {
    int kind = previewKind(i);
    if (kind < 0) return;
    if (kind == 2) {
        auto type = *iconTypeFor(defs()[i].h);
        auto gm = GameManager::get();
        auto icon = SimplePlayer::create(1);
        if (!icon) return;
        icon->updatePlayerFrame(clampIcon(type, geti(defs()[i].h)), type);
        icon->setColor(gm->colorForIdx(gm->getPlayerColor()));
        icon->setSecondColor(gm->colorForIdx(gm->getPlayerColor2()));
        icon->setScale(0.5f);
        icon->setPosition({x, y});
        content->addChild(icon, 2);
        m_previews.push_back({i, kind, icon});
        return;
    }
    auto border = CCLayerColor::create({0, 0, 0, 255}, 18.f, 18.f);
    border->setPosition({x - 9.f, y - 9.f});
    content->addChild(border, 1);
    auto swatch = CCLayerColor::create({255, 255, 255, 255}, 14.f, 14.f);
    swatch->setColor(previewColor(i, kind));
    swatch->setPosition({x - 7.f, y - 7.f});
    content->addChild(swatch, 2);
    m_previews.push_back({i, kind, swatch});
}

void OverloadMenu::refreshPreviews() {
    for (auto const& p : m_previews) {
        if (p.kind == 2) {
            auto type = *iconTypeFor(defs()[p.index].h);
            static_cast<SimplePlayer*>(p.node)->updatePlayerFrame(clampIcon(type, geti(defs()[p.index].h)), type);
        } else {
            static_cast<CCLayerColor*>(p.node)->setColor(previewColor(p.index, p.kind));
        }
    }
}

void OverloadMenu::buildRows() {
    auto content = m_scroll->m_contentLayer;
    content->removeAllChildren();
    m_previews.clear();
    m_valueLabels.assign(defs().size(), nullptr);
    m_valueInputs.assign(defs().size(), nullptr);

    std::vector<size_t> shown;
    for (size_t i = 0; i < defs().size(); i++) {
        auto const& d = defs()[i];
        if (!m_search.empty()) {
            auto hay = lower(std::string(d.name) + " " + d.desc + " " + d.category);
            if (hay.find(m_search) == std::string::npos) continue;
        } else if (m_category != d.category) {
            continue;
        }
        shown.push_back(i);
    }

    float total = std::max(m_listH, kRowH * static_cast<float>(shown.size()) + 6.f);
    content->setContentSize({m_listW, total});
    float y = total - kRowH - 3.f;
    int n = 0;
    for (auto i : shown) {
        auto const& d = defs()[i];
        if (n++ % 2 == 0) {
            auto stripe = CCLayerColor::create({255, 255, 255, 12}, m_listW, kRowH);
            stripe->setPosition({0.f, y});
            content->addChild(stripe);
        }
        bool isToggle = d.type == Type::Toggle;
        float textW = isToggle ? m_listW - 60.f : m_listW - 145.f;
        std::string name = d.name;
        if (!m_search.empty()) name += fmt::format("  ({})", d.category);
        auto nameLbl = CCLabelBMFont::create(name.c_str(), "bigFont.fnt");
        nameLbl->setAnchorPoint({0.f, 0.5f});
        nameLbl->limitLabelWidth(textW, 0.42f, 0.1f);
        nameLbl->setPosition({10.f, y + (d.desc[0] ? 23.f : 18.f)});
        if (d.cheat) nameLbl->setColor({255, 150, 150});
        content->addChild(nameLbl);
        if (d.desc[0]) {
            auto descLbl = CCLabelBMFont::create(d.desc, "chatFont.fnt");
            descLbl->setAnchorPoint({0.f, 0.5f});
            descLbl->limitLabelWidth(textW, 0.5f, 0.1f);
            descLbl->setColor({190, 190, 200});
            descLbl->setPosition({10.f, y + 9.f});
            content->addChild(descLbl);
        }

        auto menu = CCMenu::create();
        menu->setPosition({0.f, y});
        menu->setContentSize({m_listW, kRowH});
        content->addChild(menu);
        float cx = m_listW - 52.f, cy = kRowH / 2.f;

        if (isToggle) {
            auto tog = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(OverloadMenu::onToggle), 0.6f);
            tog->setTag(kTagBase + static_cast<int>(i));
            tog->toggle(on(d.h));
            tog->setPosition({m_listW - 22.f, cy});
            menu->addChild(tog);
        } else {
            auto left = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
            left->setScale(0.7f);
            auto lb = CCMenuItemSpriteExtra::create(left, this, menu_selector(OverloadMenu::onStep));
            lb->setTag(kTagBase + static_cast<int>(i));
            lb->setPosition({cx - 42.f, cy});
            menu->addChild(lb);
            auto right = CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
            right->setScale(0.7f);
            auto rb = CCMenuItemSpriteExtra::create(right, this, menu_selector(OverloadMenu::onStep));
            rb->setTag(kStepUp + kTagBase + static_cast<int>(i));
            rb->setPosition({cx + 42.f, cy});
            menu->addChild(rb);

            if (d.type == Type::Choice) {
                auto lbl = CCLabelBMFont::create(valueText(d).c_str(), "bigFont.fnt");
                lbl->limitLabelWidth(66.f, 0.38f, 0.1f);
                lbl->setPosition({cx, y + cy});
                lbl->setColor(accent());
                content->addChild(lbl);
                m_valueLabels[i] = lbl;
            } else {
                auto input = TextInput::create(62.f, "", "bigFont.fnt");
                input->setCommonFilter(d.type == Type::Int ? CommonFilter::Int : CommonFilter::Float);
                input->setString(valueText(d));
                input->setScale(0.85f);
                input->setPosition({cx, y + cy});
                H h = d.h;
                input->setCallback([this, h](std::string const& s) {
                    if (s.empty() || s == "-" || s == ".") return;
                    char* end = nullptr;
                    double v = std::strtod(s.c_str(), &end);
                    if (end == s.c_str()) return;
                    set(h, v);
                    this->refreshPreviews();
                    this->refreshCheatLabel();
                    if (h == H::menu_color && m_titleLabel) m_titleLabel->setColor(accent());
                });
                content->addChild(input);
                m_valueInputs[i] = input;
            }
            addPreview(i, content, cx - 68.f, y + cy);
        }
        y -= kRowH;
    }
    if (shown.empty()) {
        auto none = CCLabelBMFont::create("Nothing found", "bigFont.fnt");
        none->setScale(0.45f);
        none->setPosition({m_listW / 2.f, total - 30.f});
        content->addChild(none);
    }
    m_scroll->scrollToTop();
    handleTouchPriority(this);
    refreshCheatLabel();
}

void OverloadMenu::refreshCheatLabel() {
    if (!m_cheatLabel) return;
    if (cheating()) {
        m_cheatLabel->setString(fmt::format("Cheats on: {}{}", cheatList(),
                                            on(H::safe_mode) ? "  (safe mode: nothing saves)" : "  (SAFE MODE OFF!)").c_str());
        m_cheatLabel->setColor({255, 120, 120});
    } else {
        m_cheatLabel->setString("No cheats on - attempts count normally");
        m_cheatLabel->setColor({140, 230, 140});
    }
    m_cheatLabel->limitLabelWidth(m_listW, 0.55f, 0.15f);
}

void OverloadMenu::onCategory(CCObject* sender) {
    int i = static_cast<CCNode*>(sender)->getTag();
    if (i < 0 || i >= static_cast<int>(categories().size())) return;
    m_category = categories()[static_cast<size_t>(i)];
    Mod::get()->setSavedValue<std::string>("last-tab", m_category);
    m_search.clear();
    if (m_searchInput) m_searchInput->setString("");
    buildCategories();
    buildRows();
}

void OverloadMenu::onToggle(CCObject* sender) {
    auto tog = static_cast<CCMenuItemToggler*>(sender);
    int i = tog->getTag() - kTagBase;
    if (i < 0 || i >= static_cast<int>(defs().size())) return;
    // the toggler flips after this callback, so the new value is the opposite of what it shows now
    set(defs()[static_cast<size_t>(i)].h, tog->isToggled() ? 0 : 1);
    refreshCheatLabel();
}

void OverloadMenu::valueChanged(size_t i) {
    auto const& d = defs()[i];
    if (m_valueLabels[i]) {
        m_valueLabels[i]->setString(valueText(d).c_str());
        m_valueLabels[i]->limitLabelWidth(66.f, 0.38f, 0.1f);
    }
    if (m_valueInputs[i]) m_valueInputs[i]->setString(valueText(d));
    if (d.h == H::menu_color && m_titleLabel) m_titleLabel->setColor(accent());
    refreshPreviews();
    refreshCheatLabel();
}

void OverloadMenu::onStep(CCObject* sender) {
    int tag = static_cast<CCNode*>(sender)->getTag();
    bool up = tag >= kStepUp;
    int i = (up ? tag - kStepUp : tag) - kTagBase;
    if (i < 0 || i >= static_cast<int>(defs().size())) return;
    auto const& d = defs()[static_cast<size_t>(i)];
    double v = get(d.h);
    double step = stepOf(d);
    if (d.type == Type::Choice) {
        int n = static_cast<int>(choicesOf(d).size());
        set(d.h, (static_cast<int>(v) + (up ? 1 : -1) + n) % std::max(1, n));
    } else {
        set(d.h, std::round((v + (up ? step : -step)) / step) * step);
    }
    valueChanged(static_cast<size_t>(i));
}

void OverloadMenu::onResetAll(CCObject*) {
    createQuickPopup("Reset all", "Put every option back to its default?", "Cancel", "Reset", [this](FLAlertLayer*, bool yes) {
        if (!yes) return;
        resetAll();
        if (m_titleLabel) m_titleLabel->setColor(accent());
        this->buildRows();
    });
}

} // namespace ov
