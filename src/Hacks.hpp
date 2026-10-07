#pragma once
// Every option in the menu lives in this one list. Add a line here and it shows up in the menu automatically.
#include <string>
#include <string_view>
#include <vector>

namespace ov {

enum class Type { Toggle, Float, Int, Choice };

// X(id, name, description, category, type, default, min, max, isCheat, choices)
// isCheat = using it makes the attempt not count (safe mode stops progress from saving)
#define OV_HACKS(X)                                                                                                           \
    /* ---------------- Player ---------------- */                                                                          \
    X(noclip, "Noclip", "You can't die.", "Player", Toggle, 0, 0, 1, true, "")                                               \
    X(noclip_p1, "Noclip only player 1", "With Noclip on, player 2 can still die (dual).", "Player", Toggle, 0, 0, 1, false, "") \
    X(noclip_flash, "Noclip death flash", "Flash the screen red whenever you would have died.", "Player", Toggle, 1, 0, 1, false, "") \
    X(jump_hack, "Jump hack", "Jump in mid-air as cube, robot, ball and spider.", "Player", Toggle, 0, 0, 1, true, "")        \
    X(autoclicker, "Auto clicker", "Clicks for you while it's on.", "Player", Toggle, 0, 0, 1, true, "")                    \
    X(autoclick_cps, "Auto clicker speed (CPS)", "Clicks per second.", "Player", Float, 10, 1, 120, false, "")               \
    X(autoclick_hold, "Auto clicker hold (ticks)", "How long each click is held (240 ticks = 1 second).", "Player", Int, 2, 1, 120, false, "") \
    X(autoclick_p2, "Auto clicker player 2", "Also click for player 2 (two-player levels).", "Player", Toggle, 0, 0, 1, false, "") \
    X(frame_stepper, "Frame stepper", "Freezes the game. Press the Step key (F) to move one tick.", "Player", Toggle, 0, 0, 1, true, "") \
    X(custom_respawn, "Custom respawn time", "Respawn faster (or slower) after dying.", "Player", Toggle, 0, 0, 1, false, "")   \
    X(respawn_delay, "Respawn time (seconds)", "Normal is 1.0.", "Player", Float, 0.3, 0, 3, false, "")                       \
    X(no_death_effect, "No death effect", "No explosion when you die.", "Player", Toggle, 0, 0, 1, false, "")                \
    X(no_death_sound, "No death sound", "Mutes the death sound.", "Player", Toggle, 0, 0, 1, false, "")                       \
    X(hide_player, "Hide player", "Makes your icon invisible.", "Player", Toggle, 0, 0, 1, false, "")                         \
    X(auto_practice, "Auto practice mode", "Starts every level in practice mode.", "Player", Toggle, 0, 0, 1, false, "")      \
    X(practice_music, "Practice music sync", "Practice mode plays the real song in sync.", "Player", Toggle, 0, 0, 1, false, "") \
    X(safe_mode, "Safe mode", "While a cheat is on, the attempt never saves progress or completions. Keep this on!", "Player", Toggle, 1, 0, 1, false, "") \
    /* ---------------- Level ---------------- */                                                                            \
    X(startpos_switcher, "StartPos switcher", "Cycle start positions with Q / E (change keys in mod settings).", "Level", Toggle, 1, 0, 1, false, "") \
    X(startpos_buttons, "StartPos buttons", "Arrows at the bottom of the screen to switch start positions (phones / tablets).", "Level", Toggle, 1, 0, 1, false, "")\
    X(accurate_percent, "Accurate percentage", "Percentage with decimals.", "Level", Toggle, 0, 0, 1, false, "")            \
    X(percent_decimals, "Percentage decimals", "How many decimals.", "Level", Int, 2, 0, 5, false, "")                     \
    X(hitboxes, "Show hitboxes", "Draws every hitbox while you play (counts as a cheat, like in practice).", "Level", Toggle, 0, 0, 1, true, "")                 \
    X(hitboxes_on_death, "Hitboxes on death", "Shows hitboxes only after you die.", "Level", Toggle, 0, 0, 1, false, "")    \
    X(no_mirror, "No mirror portals", "Mirror portals don't flip the screen (counts as a cheat).", "Level", Toggle, 0, 0, 1, true, "")          \
    X(no_shake, "No camera shake", "Turns off screen shake.", "Level", Toggle, 0, 0, 1, false, "")                           \
    X(hide_attempts, "Hide attempt counter", "Hides the 'Attempt N' text.", "Level", Toggle, 0, 0, 1, false, "")             \
    X(hide_pause, "Hide pause button", "Hides the pause button (Esc still works).", "Level", Toggle, 0, 0, 1, false, "")      \
    X(hide_practice_btns, "Hide practice buttons", "Hides the checkpoint buttons.", "Level", Toggle, 0, 0, 1, false, "")       \
    X(hide_progress, "Hide progress bar", "Hides the progress bar at the top.", "Level", Toggle, 0, 0, 1, false, "")        \
    X(hide_percent, "Hide percentage", "Hides the % text.", "Level", Toggle, 0, 0, 1, false, "")                            \
    X(no_new_best, "Hide new best popup", "No 'New Best' popup when you die.", "Level", Toggle, 0, 0, 1, false, "")          \
    /* ---------------- Speed ---------------- */                                                                            \
    X(speedhack, "Speedhack", "Speeds the whole game up or down.", "Speed", Toggle, 0, 0, 1, true, "")                       \
    X(speed, "Speed", "1 = normal, 0.5 = half speed, 2 = double.", "Speed", Float, 1, 0.05, 10, false, "")                    \
    X(speed_audio, "Speedhack changes music", "Music speeds up / slows down with the game.", "Speed", Toggle, 1, 0, 1, false, "") \
    /* ---------------- Visual ---------------- */                                                                           \
    X(rainbow, "Rainbow icons", "Your icon cycles through colours.", "Visual", Toggle, 0, 0, 1, false, "")                   \
    X(rainbow_speed, "Rainbow speed", "How fast the colours change.", "Visual", Float, 1, 0.1, 10, false, "")                \
    X(no_wave_trail, "No wave trail", "Removes the wave's trail.", "Visual", Toggle, 0, 0, 1, false, "")                     \
    X(solid_wave, "Solid wave trail", "The wave trail isn't see-through.", "Visual", Toggle, 0, 0, 1, false, "")            \
    X(no_trail, "No player trail", "Removes the streak behind your icon.", "Visual", Toggle, 0, 0, 1, false, "")             \
    X(no_transition, "No scene transitions", "Menus switch instantly (no fade).", "Visual", Toggle, 0, 0, 1, false, "")       \
    X(unlock_icons, "Unlock all icons", "Use every icon and colour, even locked ones. Other players can see them on your profile.", "Visual", Toggle, 0, 0, 1, false, "") \
    /* ---------------- HUD ---------------- */                                                                              \
    X(hud, "Show HUD", "Info text while you play.", "HUD", Toggle, 1, 0, 1, false, "")                                       \
    X(hud_fps, "FPS", "Frames per second.", "HUD", Toggle, 1, 0, 1, false, "")                                               \
    X(hud_cps, "Clicks per second", "Your CPS, best CPS and total clicks this attempt.", "HUD", Toggle, 1, 0, 1, false, "")             \
    X(hud_noclip_acc, "Noclip accuracy", "How much of the attempt you were 'alive' with noclip.", "HUD", Toggle, 1, 0, 1, false, "") \
    X(hud_noclip_deaths, "Noclip deaths", "How many times you would have died.", "HUD", Toggle, 1, 0, 1, false, "")           \
    X(hud_attempts, "Session attempts", "Attempts since you opened the level.", "HUD", Toggle, 1, 0, 1, false, "")            \
    X(hud_time, "Attempt time", "How long this attempt has lasted.", "HUD", Toggle, 0, 0, 1, false, "")                     \
    X(hud_best_run, "Best run", "Your best run this session, like 12-54%.", "HUD", Toggle, 1, 0, 1, false, "")                \
    X(hud_jumps, "Jumps", "Jumps this attempt.", "HUD", Toggle, 0, 0, 1, false, "")                                          \
    X(hud_clock, "Clock", "The time on your computer.", "HUD", Toggle, 0, 0, 1, false, "")                                   \
    X(hud_level_id, "Level ID", "The ID of the level you're playing.", "HUD", Toggle, 0, 0, 1, false, "")                   \
    X(hud_session_time, "Time in level", "How long you've played this level since opening it (pauses don't count).", "HUD", Toggle, 0, 0, 1, false, "")\
    X(hud_cheat, "Cheat indicator", "Green dot = legit, red dot = a cheat is on.", "HUD", Toggle, 1, 0, 1, false, "")         \
    X(hud_scale, "HUD size", "How big the HUD text is.", "HUD", Float, 1, 0.4, 2.5, false, "")                               \
    X(hud_opacity, "HUD opacity", "0.1 = almost invisible, 1 = solid.", "HUD", Float, 0.8, 0.1, 1, false, "")                 \
    X(hud_corner, "HUD corner", "Where the HUD sits.", "HUD", Choice, 0, 0, 3, false, "Top left|Top right|Bottom left|Bottom right") \
    /* ---------------- Menu ---------------- */                                                                             \
    X(menu_color, "Menu colour", "Accent colour of this menu.", "Menu", Choice, 0, 0, 5, false, "Blue|Purple|Green|Red|Pink|Gold") \
    X(menu_pause, "Pause when opened in a level", "Opening the menu mid-level pauses the game.", "Menu", Toggle, 1, 0, 1, false, "")

enum class H : int {
#define OV_ENUM(id, ...) id,
    OV_HACKS(OV_ENUM)
#undef OV_ENUM
    COUNT
};

struct HackDef {
    H h;
    char const* key;
    char const* name;
    char const* desc;
    char const* category;
    Type type;
    double def, min, max;
    bool cheat;
    char const* choices;  // "A|B|C"
};

std::vector<HackDef> const& defs();
std::vector<std::string> const& categories();
std::vector<std::string> choicesOf(HackDef const& d);

double get(H h);
inline bool on(H h) { return get(h) != 0; }
void set(H h, double v);   // clamps, saves
void loadAll();
void resetAll();

// true if a cheat option is switched on right now
bool cheating();
std::string cheatList();   // names of the cheats that are on

} // namespace ov
