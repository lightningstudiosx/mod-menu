// Outside levels: level page info, level list IDs, text box bypasses.
#include <Geode/Geode.hpp>
#include <Geode/modify/CCTextInputNode.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

#include "Hacks.hpp"

using namespace geode::prelude;
using namespace ov;

namespace {
std::string songText(GJGameLevel* level) {
    if (level->m_songID > 0) return fmt::format("Song ID: {}", level->m_songID);
    return fmt::format("Song: built-in #{}", level->m_audioTrack + 1);
}
} // namespace

class $modify(OVLevelInfo, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;
        if (!level) return true;
        std::string text;
        auto addLine = [&](std::string const& s) {
            if (!text.empty()) text += "\n";
            text += s;
        };
        if (on(H::level_page_id)) addLine(fmt::format("ID: {}", level->m_levelID.value()));
        if (on(H::level_page_objects)) {
            int n = level->m_objectCount.value();
            addLine(n > 0 ? fmt::format("Objects: {}", n) : std::string("Objects: ? (download first)"));
        }
        if (on(H::level_page_song)) addLine(songText(level));
        if (text.empty()) return true;
        auto lbl = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
        lbl->setID("level-info"_spr);
        lbl->setAnchorPoint({0.f, 0.f});
        lbl->setScale(0.6f);
        lbl->setOpacity(210);
        lbl->setPosition({6.f, 6.f});
        this->addChild(lbl, 50);
        return true;
    }
};

class $modify(OVLevelCell, LevelCell) {
    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);
        if (!level || !m_mainLayer) return;
        if (auto old = m_mainLayer->getChildByID("cell-info"_spr)) old->removeFromParent();
        int objects = level->m_objectCount.value();
        bool showId = on(H::level_cell_id) && level->m_levelID.value() > 0;
        bool showObj = on(H::level_cell_objects) && objects > 0;
        if (!showId && !showObj) return;
        std::string text;
        if (showId) text = fmt::format("#{}", level->m_levelID.value());
        if (showObj) text += (text.empty() ? "" : "   ") + fmt::format("{} objects", objects);
        auto lbl = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
        lbl->setID("cell-info"_spr);
        lbl->setScale(0.5f);
        lbl->setOpacity(180);
        // bottom right corner of the cell
        float w = m_width > 1.f ? m_width : 356.f;
        lbl->setAnchorPoint({1.f, 0.f});
        lbl->setPosition({w - 8.f, 3.f});
        m_mainLayer->addChild(lbl, 10);
    }
};

class $modify(OVTextInput, CCTextInputNode) {
    bool onTextFieldInsertText(CCTextFieldTTF* sender, char const* text, int len, enumKeyCodes keys) {
        if (on(H::text_bypass_chars))
            m_allowedChars = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
        if (on(H::text_bypass_length)) m_maxLabelLength = 100000;
        return CCTextInputNode::onTextFieldInsertText(sender, text, len, keys);
    }
};
