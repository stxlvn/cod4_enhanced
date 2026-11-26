# Call of Duty 4: Modern Warfare Enhanced

Campaign co-op for two players in Call of Duty 4: Modern Warfare (2007, PC). Both PCs run the whole level in lockstep (the way the
Modern Warfare 2 Spec Ops co-op works): only the players' input crosses the network, so the world, the AI and the scripted scenes
are the same on both screens.

Built on [IW3SP-MOD](https://gitea.com/JerryALT/iw3sp_mod), the single-player client modification for CoD4 (its own README:
[docs/IW3SP-MOD.md](docs/IW3SP-MOD.md)). Sister projects by the same author:
[Modern Warfare 2 Enhanced](https://github.com/stxlvn/mw2_enhanced), [Shrek 2 Enhanced](https://github.com/stxlvn/shrek2-enhanced).

Русская версия: [README.ru.md](README.ru.md).

> **Status: early alpha, no release yet.** Everything below runs in automated tests with two game copies on one PC; a session
> between two real PCs has not been played yet.

## What works

- **Two players in one campaign**: host and partner, lockstep with a state hash check every tick, a load barrier per level,
  "waiting for the other player" when one PC stalls, the next level for both.
- **Bodies**: each player sees the other as a soldier with stances, movement, firing, reloads, throws, ladders, mantling, jumps
  and deaths (CoD4's AI animations).
- **Last stand and revive** in the IW4 Spec Ops style (pistol on the ground, bleed-out timer, hold Use next to the partner).
- **Nicknames** over the partner's head the way IW3 MP / IW4 draw overhead names (shown while his head is in sight, faded,
  smaller with distance). `coop_name` sets yours; empty = your Steam name, else Player 1 / Player 2.
- **Missions reworked for two so far**:
  - F.N.G. (killhouse): Gaz's range for two stations, the knife on your own watermelon, the course by turns.
  - Crew Expendable (cargoship): either player opens the container, the explosion knocks both down, the slow run, the jump to
    the helicopter in either order, the fast-rope with both players.
  - The Coup: the partner is a captive too and watches the host's camera for the whole level (native spectate).
  - Death from Above (ac130): two gunners, each on his own gun (a gun the other player uses cannot be taken), own HUD, FOV and
    thermal.
  - Stealth (Blackout, All Ghillied Up, Ultimatum): the enemies detect each player by his own stance, movement and distance.
  - The attack-dog takedown for both players (each his own first-person scene and melee window).
- **Engine fixes for two players**: player 2 fires triggers, the actor pool keeps room for the second player's sentient,
  per-player HUD elements, hints, vision sets and saved dvars, entities shown on one PC only.
- **Russian game**: the text drawn with fonts closer to the English original (port of MW2 Enhanced's font module; not checked in
  game yet).
- Screenshots (`screenshotJPEG`) are taken from the game's own frame, not from the screen.

The other missions run in co-op but their scripted scenes still follow player 1 only.

## Playing (for testers)

1. A copy of CoD4 1.0 (Steam). Install [IW3SP-MOD](https://gitea.com/JerryALT/iw3sp_mod/releases) into it.
2. Build this repository (below) and copy `build/bin/Release/game.dll` over IW3SP-MOD's `game.dll`, and the `cod4coop` folder
   next to it.
3. Start `iw3sp_mod.exe` on both PCs. Host: open the console and type `coop_host killhouse` (or any campaign map). Partner:
   `coop_connect <host IP>` (UDP port 28960, `coop_port` changes it). `coop_stop` ends the session.

## Building

Windows, Visual Studio 2022 Build Tools (C++ x86) and the Windows SDK:

```
tools\premake5.exe vs2022
msbuild build\iw3sp_dev.sln -p:Configuration=Release -p:Platform=Win32
```

The result is `build\bin\Release\game.dll`.

## Where the co-op code is

- `src/Components/Modules/Coop/`: the session and lockstep (`CoopNet`), players, bodies, last stand, engine hooks (`Coop`), the
  text patches of the game's scripts (`CoopScripts`), the QA runner (`CoopQA`).
- `cod4coop/scripts/maps/_coop*.gsc`: the co-op scripts (bodies, last stand, one module per reworked mission or system).
- `src/Components/Modules/Fonts.cpp`: the Russian fonts.

No game files are in this repository: the mod patches the game's scripts as they load.

## Licence and credits

GPL-3.0, as IW3SP-MOD ([LICENSE](LICENSE)). IW3SP-MOD and its credits: [docs/IW3SP-MOD.md](docs/IW3SP-MOD.md). The fonts in
`cod4coop/fonts` are h2-mod's, SIL Open Font License ([cod4coop/fonts/H2MOD-FONTS.txt](cod4coop/fonts/H2MOD-FONTS.txt)).
References used while building it: KisakCOD (the CoD4 decompilation), IW4x, the IW4 Spec Ops design.

Call of Duty and Modern Warfare are trademarks of Activision. This is a fan project, not affiliated with Activision or Infinity
Ward; you need your own copy of the game.
