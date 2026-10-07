#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/TextInput.hpp>

#include <string>
#include <vector>

namespace ov {

class OverloadMenu : public geode::Popup {
public:
    static OverloadMenu* create();
    static void toggle();          // open, or close if already open
    static bool isOpen();
    static cocos2d::ccColor3B accent();
    ~OverloadMenu() override;

protected:
    bool setup();
    void onClose(cocos2d::CCObject*) override;
    void buildCategories();
    void buildRows();
    void onCategory(cocos2d::CCObject*);
    void onToggle(cocos2d::CCObject*);
    void onStep(cocos2d::CCObject*);
    void onResetAll(cocos2d::CCObject*);
    void refreshCheatLabel();

    std::string m_category;
    std::string m_search;
    cocos2d::CCMenu* m_catMenu = nullptr;
    geode::ScrollLayer* m_scroll = nullptr;
    geode::TextInput* m_searchInput = nullptr;
    cocos2d::CCLabelBMFont* m_cheatLabel = nullptr;
    cocos2d::CCLabelBMFont* m_titleLabel = nullptr;
    std::vector<cocos2d::CCLabelBMFont*> m_valueLabels;  // per def index (choices / numbers), may be null
    std::vector<geode::TextInput*> m_valueInputs;        // per def index, may be null
    float m_listW = 0, m_listH = 0;
};

} // namespace ov
