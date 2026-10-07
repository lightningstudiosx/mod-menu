# Overload Menu (Geode mod for GD 2.2081)

A big mod menu with **59 options**: noclip, speedhack, start-position switcher, hitboxes, auto clicker, frame stepper, a HUD with a dozen readouts, visual tweaks and more. Every option is in one menu with categories and a search box. Settings are saved.

**Open it:**
- Press **Tab** anywhere.
- Or tap the pink **OV** button on the main menu.
- Or tap the pink **OV** button in the pause menu.

---

## 1. Get the .geode file (it has to be compiled once)

GitHub builds it for you, so you don't install anything. This is the same process as Frame Inspector.

1. Make a **new GitHub repository** for this mod. Don't reuse the Frame Inspector one. Private is fine.
2. Upload **everything in this folder**, including the hidden `.github` folder. If the website upload skips `.github`:
   - Click **Add file → Create new file**.
   - Type `.github/workflows/multi-platform.yml` as the name.
   - Paste in the contents of that file.
3. Open the **Actions** tab and wait for "Build Geode Mod" to finish. It takes about 5–10 minutes.
4. Click the finished run and download **Build Output**. Inside is `kai.overload-menu.geode`.
5. Put it in your GD mods folder (on Windows: `Geometry Dash/geode/mods`) and restart GD.

**Don't run it together with another mod menu** (Mega Hack, Eclipse, QOLMod, xdBot's hacks). They change the same parts of the game and will fight each other: double speedhack, noclip that doesn't turn off, and so on.

---

## 2. Everything in it

Options in red are **cheats**. While one is on, **Safe mode** makes sure the attempt can't save progress, a new best or a completion. Leave safe mode on.

### Player
| Option | What it does |
|---|---|
| **Noclip** | You can't die. Shows accuracy and "would-have-died" count on the HUD |
| Noclip only player 1 | In dual, player 2 can still die |
| Noclip death flash | Screen flashes red each time you would have died |
| **Jump hack** | Jump in mid-air (cube, robot, ball, spider) |
| **Auto clicker** | Clicks for you. Set CPS, hold length, and player 2 |
| **Frame stepper** | Freezes the game (and its sound). **F** moves one tick; hold F to keep going |
| Custom respawn time | Respawn after 0–3 seconds instead of about 1 |
| No death effect / No death sound | No explosion / no death sound |
| Hide player | Invisible icon |
| Auto practice mode | Every level starts in practice |
| Practice music sync | Practice plays the real song from the right spot |
| Safe mode | On by default. Keep it on |

### Level
| Option | What it does |
|---|---|
| StartPos switcher | **Q / E** cycle through the level's start positions |
| StartPos buttons | On-screen arrows for the same, so it works on phones and tablets too |
| Accurate percentage | 47.32% instead of 47%. You choose how many decimals |
| **Show hitboxes** | Hitboxes outside practice. Counts as a cheat |
| Hitboxes on death | Shows hitboxes only after you die. Not a cheat |
| **No mirror portals** | The screen never flips. Counts as a cheat |
| No camera shake | No screen shake |
| Hide attempt counter / pause button / practice buttons / progress bar / percentage | Hides that part of the screen |
| Hide new best popup | No "New Best" popup when you die |

### Speed
| Option | What it does |
|---|---|
| **Speedhack** | 0.05× to 10×. **Alt+S** toggles it. Speed 1 doesn't count as a cheat |
| Speedhack changes music | The music speeds up or slows down with the game |

### Visual
| Option | What it does |
|---|---|
| Rainbow icons | Your icon cycles through colours; you set the speed |
| No wave trail / Solid wave trail / No player trail | Trail tweaks |
| No scene transitions | Menus switch instantly |
| Unlock all icons | Use any icon or colour. Others can see them on your profile |

### HUD (top left by default)
- **Cheat indicator:** "Legit" / "CHEATING".
- FPS.
- CPS, best CPS and clicks.
- Noclip accuracy and noclip deaths.
- Session attempts.
- Attempt time and time in level.
- Best run (like 12–54%).
- Jumps.
- Level ID.
- Clock.
- StartPos number.

**Settings:** size, opacity, and corner.

### Menu
- **Colour:** accent colour.
- **Pause when opened in a level:** pauses the game when you open the menu mid-level.

### Keys
All keys can be changed: Geode → Overload Menu → settings (gear).

| Key | Does |
|---|---|
| Tab | Open/close the menu |
| N | Noclip on/off |
| Alt+S | Speedhack on/off |
| F | Frame stepper: step one tick |
| Q / E | Previous / next start position |

The gameplay keys are ignored while the menu is open, so typing in the search box doesn't toggle anything.

---

## 3. Honest notes

- **It hasn't been run in-game yet.** Every file compiles against the real 2.2081 bindings. Every function it hooks exists on Windows, Mac, Android and iOS. But nobody has played with it. Expect at least one or two features to need a fix.
- Most likely to need a fix:
  - **Jump hack**: some gamemodes may ignore it.
  - **Show hitboxes outside practice**.
  - **On-screen StartPos arrows on phones**: the game's own touch layer might take the taps.
  - **Frame stepper**: physics can occasionally move 0 or 2 ticks per press.

  If something doesn't work, send a screenshot and say what you expected.
- **Safe mode works by putting the attempt into the game's "test mode"**, the same thing a start position does. Once you use a cheat in an attempt, that attempt stays unsaved even if you turn the cheat off. The next attempt is clean.
- **If you turn safe mode off and beat rated levels with noclip or speedhack**, it counts as a real completion and can get you **leaderboard banned**. That's on you.
- The speedhack speeds up **everything**, menus included. That's normal for speedhacks.

---

## 4. Adding your own option

All options live in one list in `src/Hacks.hpp`. Add a line there and it shows up in the menu automatically, already saved and searchable. Then read it anywhere with `ov::on(H::your_id)` or `ov::get(H::your_id)`.
