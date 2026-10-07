# Overload Menu v2 (Geode mod for GD 2.2081)

A huge mod menu with **273 options in 14 tabs**:
- **163** are on/off switches.
- **110** are settings for those switches: icon numbers, RGB values, sliders, modes.

Everything sits in one menu with a search box, and your settings are saved.

**Open it:**
- Press **Tab** anywhere.
- Or tap the pink **OV** button on the main menu.
- Or tap the pink **OV** button in the pause menu.

---

## 1. Get the .geode file (it has to be compiled once)

GitHub builds it for you, the same way as Frame Inspector.

1. Make a **new GitHub repository** for this mod. Private is fine.
   - If you already made one for v1, upload the new files over the old ones.
2. Upload **everything in this folder**, including the hidden `.github` folder. If the website upload skips `.github`:
   - Click **Add file → Create new file**.
   - Type `.github/workflows/multi-platform.yml` as the name.
   - Paste in the contents of that file.
3. Open the **Actions** tab and wait for "Build Geode Mod" to finish. It takes about 5–10 minutes.
4. Click the finished run and download **Build Output**. Inside is `kai.overload-menu.geode`.
5. Put it in `Geometry Dash/geode/mods` and restart GD.

**Don't run it together with another mod menu** (Mega Hack, Eclipse, QOLMod, xdBot's hacks). They change the same parts of the game and will fight each other.

---

## 2. What's in each tab

Red options are **cheats**. While one is on, **Safe mode** stops that attempt from saving progress, a new best or a completion.

| Tab | What's in it |
|---|---|
| **Player** | Noclip (P1 only option, red death flash), jump hack, auto clicker (CPS, hold length, P2), frame stepper, custom respawn time, no death effect, hide player / player 2, no spider teleport line, safe mode |
| **Practice** | Auto practice, practice music sync, **auto restart at %**, **death markers** (last or all, noclip too), pause when you die, noclip only in practice, speedhack only in practice |
| **Level** | StartPos switcher (Q / E + on-screen arrows), accurate percentage, hitboxes, hitboxes on death, no mirror, no shake, hide attempts / pause button / practice buttons / progress bar / percentage, hide new best |
| **Speed** | Speedhack 0.05×–10×, music follows speed, speedhack only in levels, **Alt+Up / Alt+Down** change speed, **hold C** for slow-mo |
| **Icons** | **Any icon for every gamemode** (cube, ship, ball, UFO, wave, robot, spider, swing, jetpack), with a preview in the menu. Also: player 2 on/off, random icons every attempt, icon size, opacity, spin, mirror, upside-down, jelly, shake, flicker, always glow / no glow, hide the cube in ship & UFO, any death effect (1–20) |
| **Colors** | For **each player**, pick main, second, glow, trail and wave trail colours separately. Each one can be Normal, a GD colour (#0–106), your own RGB, Rainbow, or Pulse, with a colour swatch in the menu. Also: P2 copies P1, and rainbow saturation, brightness, offset and pulse speed |
| **Trails** | No trail, no wave trail, solid wave trail, custom trail, always show trail, trail width, ghost trail, wave trail width, no wave pulse |
| **Visual** | Quick rainbow icons, rainbow speed, no scene transitions, unlock all icons |
| **World** | Hide ground / background / middleground / player particles, custom background colour, custom ground colour, screen tint (RGB + strength), screen rotation, zoom, mirror, flip |
| **HUD** | **56 readouts** you can switch on one by one, listed below. Plus size, opacity, corner, font, colour (default / white / menu colour / rainbow / RGB), background box, line spacing and position |
| **Audio** | No death sound, mute sound effects in levels, pitch for all sound, click sound + volume |
| **Fun** | Disco mode, spinning screen, drunk mode, earthquake, random colours every attempt, rainbow progress bar / percentage / attempt text / background / ground |
| **Extras** | Level ID, object count and song ID on level pages, level IDs / object counts in level lists, type any character, no text length limit |
| **Menu** | Menu colour, pause when opened, remember last tab, menu size, background darkness, key popups, OV buttons on/off |

The HUD readouts are grouped like this:
- **Cheats & performance:** cheat indicator, FPS, lowest FPS, frame time.
- **Clicks:** CPS, best CPS, total clicks, releases, holding, hold time.
- **Noclip:** noclip accuracy, noclip deaths.
- **Attempts & deaths:**
  - Session attempts, the game's attempt number.
  - Session deaths, last death, average death.
  - Session best, best run.
  - Start %, % left.
- **Time:** attempt time, time in level, level time, physics tick, jumps.
- **Player:**
  - X, Y, Y speed, rotation.
  - Gamemode, speed portal, gravity, size.
  - Dual, on ground, dashing, direction.
- **Camera & practice:** camera zoom, checkpoints.
- **Level info:**
  - Level name, creator, ID.
  - Stars, object count, song ID.
  - Total attempts, total jumps, your best, practice best.
  - StartPos number.
- **Other:** clock, date, mods loaded, window size, custom text.

### Keys
All keys can be changed: Geode → Overload Menu → settings.

| Key | Does |
|---|---|
| Tab | Open/close the menu |
| N | Noclip on/off |
| Alt+S | Speedhack on/off |
| Alt+Up / Alt+Down | Faster / slower |
| C (hold) | Slow-mo |
| F | Frame stepper: step one tick |
| Q / E | Previous / next start position |
| Alt+R | Restart attempt |
| H | HUD on/off |
| Alt+H | Hitboxes on/off |
| Alt+A | Auto clicker on/off |
| Alt+J | Jump hack on/off |

Gameplay keys are ignored while the menu is open, so typing in the search box doesn't toggle anything.

---

## 3. Honest notes

- **It hasn't been run in-game yet.** All 6 source files compile against the real 2.2081 game files. Every one of the ~40 game functions it hooks exists on Windows, Mac, Android and iOS, and every cocos function it calls is in the game's Windows and Android libraries. But nobody has played with it. With 273 options, expect some of them to need a fix after you try it.
- **Most likely to need a fix:**
  - **Robot / spider custom icons:** they only change when the level loads, and might not change at all.
  - **Custom trail.**
  - **Ghost trail.**
  - **Glow colour.**
  - **Icon effects on robot / spider:** size, spin and similar may only affect some parts of the icon.
  - **Jump hack in some gamemodes.**
  - **Hitboxes outside practice.**
  - **On-screen StartPos arrows on phones.**
  - **Screen effects in levels that use shader triggers.**

  If something doesn't work, send a screenshot and say which option.
- **Safe mode** puts the attempt into the game's "test mode" (what a start position does). Once you use a cheat, that attempt stays unsaved even if you turn the cheat off. The next attempt is clean.
- **If you turn safe mode off and beat rated levels with noclip or speedhack**, it counts for real and can get you **leaderboard banned**.
- **Unlock all icons:** other players can see locked icons on your profile.
- **Type any character / no length limit:** the servers can still reject or cut off what you type.

---

## 4. Adding your own option

All options live in one list in `gen_hacks.py` (run `python3 gen_hacks.py src/Hacks.hpp` to rebuild `src/Hacks.hpp`). Add a line there, and the option shows up in the menu, saved and searchable. Then read it anywhere with `ov::on(H::your_id)` or `ov::get(H::your_id)`.
