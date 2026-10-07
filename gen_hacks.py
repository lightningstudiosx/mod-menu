# Generates overload/src/Hacks.hpp from one readable list.
# Row: (id, name, desc, category, type, default, min, max, cheat, choices)
import sys

T, F, I, C = "Toggle", "Float", "Int", "Choice"
rows = []


def add(id, name, desc, cat, typ=T, d=0, mn=0, mx=1, cheat=False, choices=""):
    rows.append((id, name, desc, cat, typ, d, mn, mx, cheat, choices))


# ---------------------------------------------------------------- Player
P = "Player"
add("noclip", "Noclip", "You can't die.", P, cheat=True)
add("noclip_p1", "Noclip only player 1", "With Noclip on, player 2 can still die (dual).", P)
add("noclip_flash", "Noclip death flash", "Flash the screen red whenever you would have died.", P, d=1)
add("jump_hack", "Jump hack", "Jump in mid-air as cube, robot, ball and spider.", P, cheat=True)
add("autoclicker", "Auto clicker", "Clicks for you while it's on (Alt+A).", P, cheat=True)
add("autoclick_cps", "Auto clicker speed (CPS)", "Clicks per second.", P, F, 10, 1, 120)
add("autoclick_hold", "Auto clicker hold (ticks)", "How long each click is held (240 ticks = 1 second).", P, I, 2, 1, 120)
add("autoclick_p2", "Auto clicker player 2", "Also click for player 2 (two-player levels).", P)
add("frame_stepper", "Frame stepper", "Freezes the game. Press the Step key (F) to move one tick.", P, cheat=True)
add("custom_respawn", "Custom respawn time", "Respawn faster (or slower) after dying.", P)
add("respawn_delay", "Respawn time (seconds)", "Normal is about 1.0.", P, F, 0.3, 0, 3)
add("no_death_effect", "No death effect", "No explosion when you die.", P)
add("hide_player", "Hide player", "Makes your icon invisible.", P)
add("hide_player2", "Hide player 2", "Makes only player 2 invisible (dual).", P)
add("no_spider_line", "No spider teleport line", "Removes the flash line when the spider teleports.", P)
add("safe_mode", "Safe mode", "While a cheat is on, the attempt never saves progress or completions. Keep this on!", P, d=1)

# ---------------------------------------------------------------- Practice
PR = "Practice"
add("auto_practice", "Auto practice mode", "Starts every level in practice mode.", PR)
add("practice_music", "Practice music sync", "Practice mode plays the real song in sync.", PR)
add("auto_restart", "Auto restart at %", "Restarts the attempt when you reach the % below. Great for drilling a part.", PR)
add("restart_percent", "Restart at (%)", "Used by Auto restart.", PR, F, 50, 1, 100)
add("death_markers", "Death markers", "A red X where you last died.", PR)
add("death_markers_all", "Keep every death marker", "Keep all X's instead of only the last one.", PR)
add("death_markers_noclip", "Markers for noclip deaths", "Also mark where you would have died with noclip.", PR)
add("pause_on_death", "Pause when you die", "Opens the pause menu every time you die.", PR)
add("noclip_practice_only", "Noclip only in practice", "Noclip does nothing outside practice mode.", PR)
add("speed_practice_only", "Speedhack only in practice", "Speedhack does nothing outside practice mode.", PR)

# ---------------------------------------------------------------- Level
L = "Level"
add("startpos_switcher", "StartPos switcher", "Cycle start positions with Q / E (change keys in mod settings).", L, d=1)
add("startpos_buttons", "StartPos buttons", "Arrows at the bottom of the screen to switch start positions (phones / tablets).", L, d=1)
add("accurate_percent", "Accurate percentage", "Percentage with decimals.", L)
add("percent_decimals", "Percentage decimals", "How many decimals.", L, I, 2, 0, 5)
add("hitboxes", "Show hitboxes", "Draws every hitbox while you play (counts as a cheat, like in practice).", L, cheat=True)
add("hitboxes_on_death", "Hitboxes on death", "Shows hitboxes only after you die.", L)
add("no_mirror", "No mirror portals", "Mirror portals don't flip the screen (counts as a cheat).", L, cheat=True)
add("no_shake", "No camera shake", "Turns off screen shake.", L)
add("hide_attempts", "Hide attempt counter", "Hides the 'Attempt N' text.", L)
add("hide_pause", "Hide pause button", "Hides the pause button (Esc still works).", L)
add("hide_practice_btns", "Hide practice buttons", "Hides the checkpoint buttons.", L)
add("hide_progress", "Hide progress bar", "Hides the progress bar at the top.", L)
add("hide_percent", "Hide percentage", "Hides the % text.", L)
add("no_new_best", "Hide new best popup", "No 'New Best' popup when you die.", L)

# ---------------------------------------------------------------- Speed
S = "Speed"
add("speedhack", "Speedhack", "Speeds the whole game up or down (Alt+S).", S, cheat=True)
add("speed", "Speed", "1 = normal, 0.5 = half speed, 2 = double. Alt+Up / Alt+Down change it.", S, F, 1, 0.05, 10)
add("speed_audio", "Speedhack changes music", "Music speeds up / slows down with the game.", S, d=1)
add("speed_levels_only", "Speedhack only in levels", "Menus stay at normal speed.", S)
add("speed_step", "Speed key step", "How much Alt+Up / Alt+Down change the speed.", S, F, 0.25, 0.05, 2)
add("slowmo_speed", "Slow-mo key speed", "Hold C in a level to play at this speed (counts as a cheat while held).", S, F, 0.5, 0.05, 1)

# ---------------------------------------------------------------- Icons
IC = "Icons"
modes = [("cube", "Cube", 485), ("ship", "Ship", 169), ("ball", "Ball", 118), ("ufo", "UFO", 149), ("wave", "Wave", 96),
         ("robot", "Robot", 68), ("spider", "Spider", 69), ("swing", "Swing", 43), ("jetpack", "Jetpack", 8)]
for key, nm, mx in modes:
    extra = " (re-open the level to see a change)" if key in ("robot", "spider") else ""
    add(f"icon_{key}_on", f"Custom {nm} icon", f"Use the {nm.lower()} below instead of your own{extra}.", IC)
    add(f"icon_{key}", f"{nm} icon number", f"Any {nm.lower()}, even locked ones (1-{mx}).", IC, I, 1, 1, mx)
add("icons_p2", "Player 2 uses custom icons", "Off = player 2 keeps your normal icons.", IC, d=1)
add("icon_random", "Random icons every attempt", "A new random icon for every gamemode each attempt.", IC)
add("icon_scale", "Icon size (looks only)", "Hitbox stays the same. 1 = normal.", IC, F, 1, 0.25, 3)
add("icon_opacity", "Icon opacity", "1 = solid, 0.1 = almost invisible.", IC, F, 1, 0.1, 1)
add("icon_spin", "Spinning icon", "Your icon spins nonstop (looks only).", IC)
add("icon_spin_speed", "Spin speed", "Turns per second.", IC, F, 1, 0.1, 10)
add("icon_mirror", "Mirror icon", "Flips your icon left-right.", IC)
add("icon_upside", "Upside-down icon", "Flips your icon top-bottom (looks only).", IC)
add("icon_jelly", "Jelly icon", "Your icon squishes and stretches.", IC)
add("icon_shake", "Shaky icon", "Your icon jitters.", IC)
add("icon_flicker", "Flickering icon", "Your icon flickers like a ghost.", IC)
add("icon_glow_force", "Always glow", "Glow outline on, even if it's off in your garage.", IC)
add("icon_glow_hide", "No glow", "Glow outline off.", IC)
add("hide_vehicle_cube", "Hide cube in ship / UFO", "Removes the little cube riding the ship, UFO and jetpack.", IC)
add("death_effect_on", "Custom death effect", "Use the death effect below.", IC)
add("death_effect", "Death effect number", "1 = default explosion, up to 20.", IC, I, 1, 1, 20)

# ---------------------------------------------------------------- Colors
CO = "Colors"
parts = [("main", "main colour", (255, 80, 200)), ("second", "second colour", (80, 220, 255)),
         ("glow", "glow colour", (255, 255, 255)), ("trail", "trail colour", (255, 255, 255)),
         ("wave", "wave trail colour", (255, 255, 255))]
for p, pn in (("p1", "P1"), ("p2", "P2")):
    for part, partName, rgb in parts:
        base = f"c_{p}_{part}"
        hint = " Glow only shows with glow on (Icons > Always glow)." if part == "glow" else ""
        add(f"{base}_mode", f"{pn} {partName}", f"Normal, a GD colour, your own RGB, rainbow or pulse.{hint}", CO, C, 0, 0, 4,
            choices="Normal|GD colour|Custom RGB|Rainbow|Pulse")
        add(f"{base}_idx", f"{pn} {partName}: GD colour #", "Used when set to 'GD colour' (0-106, like the garage).", CO, I, 12, 0, 106)
        add(f"{base}_r", f"{pn} {partName}: red", "Used by 'Custom RGB' and 'Pulse'.", CO, I, rgb[0], 0, 255)
        add(f"{base}_g", f"{pn} {partName}: green", "Used by 'Custom RGB' and 'Pulse'.", CO, I, rgb[1], 0, 255)
        add(f"{base}_b", f"{pn} {partName}: blue", "Used by 'Custom RGB' and 'Pulse'.", CO, I, rgb[2], 0, 255)
add("c_p2_copy", "P2 copies P1 colours", "Player 2 uses player 1's colour settings.", CO)
add("c_rainbow_sat", "Rainbow saturation", "0 = white, 1 = full colour.", CO, F, 0.8, 0, 1)
add("c_rainbow_bright", "Rainbow brightness", "How bright rainbow colours are.", CO, F, 1, 0.1, 1)
add("c_rainbow_spread", "Rainbow offset between parts", "0 = all parts the same colour.", CO, F, 0.5, 0, 1)
add("c_pulse_speed", "Pulse speed", "How fast 'Pulse' colours throb.", CO, F, 1, 0.1, 10)

# ---------------------------------------------------------------- Trails
TR = "Trails"
add("no_trail", "No player trail", "Removes the streak behind your icon.", TR)
add("no_wave_trail", "No wave trail", "Removes the wave's trail.", TR)
add("solid_wave", "Solid wave trail", "The wave trail isn't see-through.", TR)
add("custom_trail", "Custom trail", "Use the trail below (re-open the level to see a change).", TR)
add("trail_id", "Trail number", "Which trail (1 = default).", TR, I, 1, 1, 15)
add("always_trail", "Always show trail", "Your trail never turns off.", TR)
add("trail_width", "Trail width", "1 = normal.", TR, F, 1, 0.2, 5)
add("ghost_trail", "Ghost trail", "Afterimages follow your icon, like the ghost trigger.", TR)
add("wave_width", "Wave trail width", "1 = normal.", TR, F, 1, 0.2, 4)
add("wave_pulse_off", "No wave trail pulse", "The wave trail doesn't pulse to the music.", TR)

# ---------------------------------------------------------------- Visual
V = "Visual"
add("rainbow", "Rainbow icons (quick)", "Rainbow main + second colour. Fine-tune in Colors.", V)
add("rainbow_speed", "Rainbow speed", "How fast every rainbow effect changes.", V, F, 1, 0.1, 10)
add("no_transition", "No scene transitions", "Menus switch instantly (no fade).", V)
add("unlock_icons", "Unlock all icons", "Use every icon and colour in the garage, even locked ones. Other players can see them on your profile.", V)

# ---------------------------------------------------------------- World
W = "World"
add("hide_ground", "Hide ground", "No ground or ceiling.", W)
add("hide_background", "Hide background", "Black background.", W)
add("hide_middleground", "Hide middleground", "Hides the middleground layer.", W)
add("hide_player_particles", "Hide player particles", "No dust, ship fire particles, landing puffs...", W)
add("bg_override", "Custom background colour", "Use the RGB below for the background.", W)
add("bg_r", "Background red", "", W, I, 40, 0, 255)
add("bg_g", "Background green", "", W, I, 40, 0, 255)
add("bg_b", "Background blue", "", W, I, 90, 0, 255)
add("ground_override", "Custom ground colour", "Use the RGB below for the ground.", W)
add("ground_r", "Ground red", "", W, I, 30, 0, 255)
add("ground_g", "Ground green", "", W, I, 30, 0, 255)
add("ground_b", "Ground blue", "", W, I, 60, 0, 255)
add("tint_on", "Screen tint", "Puts a coloured layer over the level.", W)
add("tint_r", "Tint red", "", W, I, 0, 0, 255)
add("tint_g", "Tint green", "", W, I, 0, 0, 255)
add("tint_b", "Tint blue", "", W, I, 0, 0, 255)
add("tint_a", "Tint strength", "0 = invisible, 255 = solid.", W, I, 80, 0, 255)
add("screen_rotation", "Screen rotation", "Degrees. 180 = upside down.", W, F, 0, -180, 180)
add("screen_zoom", "Screen zoom", "1 = normal.", W, F, 1, 0.25, 3)
add("screen_flip_x", "Mirror screen", "Flips the whole screen left-right.", W)
add("screen_flip_y", "Flip screen vertically", "Flips the whole screen top-bottom.", W)

# ---------------------------------------------------------------- HUD
HU = "HUD"
add("hud", "Show HUD", "Info text while you play (H toggles it).", HU, d=1)
hud_lines = [
    ("hud_cheat", "Cheat indicator", "Green = legit, red = a cheat is on.", 1),
    ("hud_fps", "FPS", "Frames per second.", 1),
    ("hud_fps_low", "Lowest FPS", "Lowest FPS in the last second (spots lag).", 0),
    ("hud_frame_ms", "Frame time (ms)", "How long each frame takes.", 0),
    ("hud_cps", "Clicks per second", "Your CPS, best CPS and clicks this attempt.", 1),
    ("hud_best_cps_session", "Best CPS this session", "Your highest CPS since opening the level.", 0),
    ("hud_total_clicks", "Total clicks", "Clicks since opening the level.", 0),
    ("hud_releases", "Releases", "Releases this attempt.", 0),
    ("hud_holding", "Holding", "Shows when you're holding click.", 0),
    ("hud_hold_time", "Hold time", "How long your current / last hold lasted.", 0),
    ("hud_noclip_acc", "Noclip accuracy", "How much of the attempt you were 'alive' with noclip.", 1),
    ("hud_noclip_deaths", "Noclip deaths", "How many times you would have died.", 1),
    ("hud_attempts", "Session attempts", "Attempts since you opened the level.", 1),
    ("hud_attempt_gd", "Attempt number", "The game's own attempt counter.", 0),
    ("hud_session_deaths", "Session deaths", "Deaths since opening the level.", 0),
    ("hud_last_death", "Last death", "Where you died last.", 0),
    ("hud_avg_death", "Average death", "Average % you die at this session.", 0),
    ("hud_session_best", "Session best", "Furthest you got this session.", 0),
    ("hud_best_run", "Best run", "Your best run this session, like 12-54%.", 1),
    ("hud_start_percent", "Attempt started at", "The % this attempt started from.", 0),
    ("hud_percent_left", "Percent left", "How much of the level is left.", 0),
    ("hud_time", "Attempt time", "How long this attempt has lasted.", 0),
    ("hud_session_time", "Time in level", "How long you've played this level since opening it (pauses don't count).", 0),
    ("hud_level_time", "Level time", "The level's own clock (what time triggers use).", 0),
    ("hud_tick", "Physics tick", "Which 240 TPS tick you're on.", 0),
    ("hud_jumps", "Jumps", "Jumps this attempt.", 0),
    ("hud_xpos", "X position", "Your horizontal position.", 0),
    ("hud_ypos", "Y position", "Your height.", 0),
    ("hud_yvel", "Y velocity", "How fast you're moving up / down.", 0),
    ("hud_rotation", "Rotation", "Your icon's angle.", 0),
    ("hud_gamemode", "Gamemode", "Cube, ship, ball...", 0),
    ("hud_portal_speed", "Speed portal", "0.5x, 1x, 2x, 3x, 4x.", 0),
    ("hud_gravity", "Gravity", "Normal or flipped.", 0),
    ("hud_size_mode", "Size", "Mini or normal.", 0),
    ("hud_dual", "Dual", "Shows when you're in dual mode.", 0),
    ("hud_on_ground", "On ground", "Shows when you're on the ground.", 0),
    ("hud_dashing", "Dashing", "Shows during dash orbs.", 0),
    ("hud_direction", "Direction", "Facing left or right (platformer).", 0),
    ("hud_zoom", "Camera zoom", "The level's camera zoom.", 0),
    ("hud_checkpoints", "Checkpoints", "Practice checkpoints placed.", 0),
    ("hud_level_name", "Level name", "", 0),
    ("hud_creator", "Creator", "Who made the level.", 0),
    ("hud_level_id", "Level ID", "The ID of the level you're playing.", 0),
    ("hud_stars", "Stars", "The level's star rating.", 0),
    ("hud_object_count", "Object count", "How many objects the level has.", 0),
    ("hud_song_id", "Song ID", "Newgrounds song ID (or built-in song).", 0),
    ("hud_level_attempts", "Total attempts on level", "All your attempts on this level, ever.", 0),
    ("hud_level_jumps", "Total jumps on level", "All your jumps on this level, ever.", 0),
    ("hud_normal_best", "Your best %", "Your normal mode best.", 0),
    ("hud_practice_best", "Your practice best %", "Your practice mode best.", 0),
    ("hud_startpos", "StartPos number", "Which start position you're on.", 1),
    ("hud_clock", "Clock", "The time on your computer.", 0),
    ("hud_date", "Date", "Today's date.", 0),
    ("hud_mod_count", "Mods loaded", "How many Geode mods you have on.", 0),
    ("hud_window_size", "Window size", "Your game window in pixels.", 0),
    ("hud_custom_text", "Custom text", "Shows your own text (set it in the mod settings).", 0),
]
for id, nm, desc, d in hud_lines:
    add(id, nm, desc, HU, d=d)
add("hud_scale", "HUD size", "How big the HUD text is.", HU, F, 1, 0.4, 2.5)
add("hud_opacity", "HUD opacity", "0.1 = almost invisible, 1 = solid.", HU, F, 0.8, 0.1, 1)
add("hud_corner", "HUD corner", "Where the HUD sits.", HU, C, 0, 0, 3, choices="Top left|Top right|Bottom left|Bottom right")
add("hud_font", "HUD font", "", HU, C, 0, 0, 2, choices="Big|Gold|Chat")
add("hud_color_mode", "HUD colour", "Default keeps the coloured lines (cheat indicator etc).", HU, C, 0, 0, 4,
    choices="Default|White|Menu colour|Rainbow|Custom RGB")
add("hud_r", "HUD red", "Used by 'Custom RGB'.", HU, I, 255, 0, 255)
add("hud_g", "HUD green", "Used by 'Custom RGB'.", HU, I, 255, 0, 255)
add("hud_b", "HUD blue", "Used by 'Custom RGB'.", HU, I, 255, 0, 255)
add("hud_bg", "HUD background", "A dark box behind the HUD so it's easier to read.", HU)
add("hud_spacing", "HUD line spacing", "1 = normal.", HU, F, 1, 0.5, 2.5)
add("hud_offset_x", "HUD move sideways", "Pixels. Moves the HUD away from its corner.", HU, F, 0, -300, 300)
add("hud_offset_y", "HUD move up / down", "Pixels.", HU, F, 0, -200, 200)

# ---------------------------------------------------------------- Audio
A = "Audio"
add("no_death_sound", "No death sound", "Mutes the death sound.", A)
add("mute_sfx", "Mute sound effects in levels", "Orbs, coins, triggers... all quiet. Music keeps playing.", A)
add("pitch_on", "Change pitch", "Plays all sound at the pitch below (on top of speedhack).", A)
add("pitch", "Pitch", "1 = normal, 2 = chipmunk, 0.5 = deep.", A, F, 1, 0.25, 4)
add("click_sound", "Click sound", "Plays a sound every time you click in a level.", A)
add("click_volume", "Click sound volume", "", A, F, 0.5, 0.05, 1)

# ---------------------------------------------------------------- Fun
FU = "Fun"
add("disco", "Disco mode", "The screen flashes through colours.", FU)
add("disco_speed", "Disco speed", "", FU, F, 1, 0.1, 10)
add("spin_screen", "Spinning screen", "The whole screen slowly spins. Good luck.", FU)
add("spin_screen_speed", "Screen spin speed", "Turns per minute x 6.", FU, F, 1, 0.1, 10)
add("drunk", "Drunk mode", "The screen sways around.", FU)
add("earthquake", "Earthquake", "The screen never stops shaking.", FU)
add("earthquake_strength", "Earthquake strength", "Pixels.", FU, F, 3, 0.5, 20)
add("random_colors", "Random colours every attempt", "New random icon colours each attempt (for parts set to Normal).", FU)
add("rainbow_bar", "Rainbow progress bar", "", FU)
add("rainbow_percent", "Rainbow percentage", "", FU)
add("rainbow_attempt", "Rainbow attempt text", "", FU)
add("rainbow_bg", "Rainbow background", "", FU)
add("rainbow_ground", "Rainbow ground", "", FU)

# ---------------------------------------------------------------- Extras
E = "Extras"
add("level_page_id", "Level ID on level pages", "Shown in the bottom left of a level's page.", E, d=1)
add("level_page_objects", "Object count on level pages", "Shows after the level is downloaded.", E, d=1)
add("level_page_song", "Song ID on level pages", "", E, d=1)
add("level_cell_id", "Level ID in level lists", "Small ID next to each level's name in search / lists.", E)
add("level_cell_objects", "Object count in level lists", "Only for levels you've downloaded.", E)
add("text_bypass_chars", "Type any character", "Text boxes accept every character. Servers may still reject odd ones.", E)
add("text_bypass_length", "No text length limit", "Text boxes have no character limit. Servers may still cut it off.", E)

# ---------------------------------------------------------------- Menu
M = "Menu"
add("menu_color", "Menu colour", "Accent colour of this menu.", M, C, 0, 0, 5, choices="Blue|Purple|Green|Red|Pink|Gold")
add("menu_pause", "Pause when opened in a level", "Opening the menu mid-level pauses the game.", M, d=1)
add("menu_remember_tab", "Remember last tab", "Re-opens on the tab you used last.", M, d=1)
add("menu_scale", "Menu size", "Close and re-open the menu to apply.", M, F, 1, 0.6, 1.3)
add("menu_dim", "Menu background darkness", "0 = see-through, 255 = black. Re-open to apply.", M, I, 150, 0, 255)
add("key_notifications", "Key popups", "Show a little popup when you press a hack key.", M, d=1)
add("main_menu_button", "OV button on main menu", "Takes effect next time the main menu opens.", M, d=1)
add("pause_button", "OV button in pause menu", "", M, d=1)


# ---------------------------------------------------------------- write
def q(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


ids = [r[0] for r in rows]
assert len(ids) == len(set(ids)), [x for x in ids if ids.count(x) > 1]
lines = []
for (id, name, desc, cat, typ, d, mn, mx, cheat, choices) in rows:
    assert mn <= d <= mx, id
    if typ == C:
        assert len(choices.split("|")) == mx + 1, id
    lines.append(f"    X({id}, {q(name)}, {q(desc)}, {q(cat)}, {typ}, {d}, {mn}, {mx}, {'true' if cheat else 'false'}, {q(choices)})")
body = " \\\n".join(lines)

out = f'''#pragma once
// Every option in the menu lives in this one list (generated by gen_hacks.py).
// Add a line and it shows up in the menu automatically, saved and searchable.
#include <string>
#include <string_view>
#include <vector>

namespace ov {{

enum class Type {{ Toggle, Float, Int, Choice }};

// X(id, name, description, category, type, default, min, max, isCheat, choices)
#define OV_HACKS(X) \\
{body}

enum class H : int {{
#define OV_ENUM(id, ...) id,
    OV_HACKS(OV_ENUM)
#undef OV_ENUM
    COUNT
}};

struct HackDef {{
    H h;
    char const* key;
    char const* name;
    char const* desc;
    char const* category;
    Type type;
    double def, min, max;
    bool cheat;
    char const* choices;  // "A|B|C"
}};

std::vector<HackDef> const& defs();
std::vector<std::string> const& categories();
std::vector<std::string> choicesOf(HackDef const& d);
HackDef const& def(H h);

double get(H h);
inline bool on(H h) {{ return get(h) != 0; }}
inline int geti(H h) {{ return static_cast<int>(get(h)); }}
inline float getf(H h) {{ return static_cast<float>(get(h)); }}
void set(H h, double v);   // clamps, saves
void loadAll();
void resetAll();
unsigned changeCounter();  // goes up every time any option changes

// live state set by keys
extern bool g_slowmoHeld;

// what's actually in effect right now (depends on practice mode etc.)
bool noclipActive();
float levelSpeed();        // game speed inside a level
float effectiveSpeed();    // game speed right now (menus included)

// true if a cheat is in effect right now
bool cheating();
std::string cheatList();   // names of the cheats in effect

}} // namespace ov
'''
open(sys.argv[1], "w").write(out)
cats = []
for r in rows:
    if r[3] not in cats:
        cats.append(r[3])
print(len(rows), "options;", len(cats), "categories:", cats)
from collections import Counter
print(Counter(r[3] for r in rows))
