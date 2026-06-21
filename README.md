![license](https://img.shields.io/badge/License-GPL%20v3-green.svg)
[![Build](https://gitea.com/JerryALT/iw3sp_mod/actions/workflows/Build/badge.svg)](https://gitea.com/JerryALT/iw3sp_mod/actions/runs/latest)
[![Discord](https://img.shields.io/discord/1091304681822752778?color=%237289DA&label=members&logo=discord&logoColor=%23FFFFFF)](https://discord.gg/YzNZSEMAnf)

# IW3SP-MOD - Modification for Call of Duty 4: Modern Warfare

Singleplayer client modification for Call of Duty 4: Modern Warfare (IW3). This client based on the [IW4x Client](https://github.com/iw4x/iw4x-client).

<p align="center">
  <img src="assets/github/banner.png?raw=true"/>
</p>

___

# Credits
- [Bogdan Konstantinou](https://www.youtube.com/@BogdanKonstantinou)
- [xoxor4d - IW3xo](https://github.com/xoxor4d/iw3xo-dev)
- [X Labs Team](https://github.com/XLabsProject)
- [AlterWare](https://github.com/alterware)
- [fed - h2-mod](https://github.com/fedddddd/h2-mod)
- [Nukem9 - LinkerMod](https://github.com/Nukem9/LinkerMod)
- [CoD4X Server/Client](https://github.com/callofduty4x)
- [Vlad Loktionov](https://www.youtube.com/@ruvlad)

## [Dependencies]
- [ocornut - Dear ImGui](https://github.com/ocornut/imgui)
- [curl](https://github.com/curl/curl)
- [juliettef - imgui_markdown](https://github.com/juliettef/imgui_markdown)
- [discord-rpc](https://github.com/discord/discord-rpc)
- [nlohmann - json](https://github.com/nlohmann/json)
- [DirectX SDK](https://github.com/devKlausS/dxsdk)
- [LibTomCrypt](https://github.com/libtom/libtomcrypt)
- [LibTomMath](https://github.com/libtom/libtommath)

# Disclaimer
This software has been created purely for the purposes of academic research. Project maintainers are not responsible or liable for misuse of the software. Use responsibly.

# Features (Short)
[Mod compatibility list](https://gitea.com/JerryALT/iw3sp_mod/wiki/Mod-Compatibility)

- Increased asset limits for IW3
  - FX: 1200
  - Image: 7168
  - Loaded Sound: 2700
  - Streamed Sound: 24000
  - Material: 8192
  - Weapon: 2400
  - Xmodel: 5125
- Added the game achievements;
- Added the gamepad support with aim assist;
- Ability to launch the mods from main menu of the game and more...

More information about features of the IW3SP-MOD you can see in Wiki Page: [link](https://gitea.com/JerryALT/iw3sp_mod/wiki/IW3SP-MOD%3A+Features.-)

# How to install modification? (For regular users)
1. Download the latest [release](https://gitea.com/JerryALT/iw3sp_mod/releases/latest)
2. Place the .zip contents into your cod4 root folder
3. Start the **iw3sp_mod.exe**

# How to compile from source? (For advanced users)
1. Clone the this repository. I recommend use the [git-scm](https://git-scm.com/downloads).
</br>P.S: (**Downloading the zip does not include deps!**)</br>
2. Find any directory which you want the placed the content of repository, then open context menu and find "Git Bash Here" and write next line in the cmd: `git clone https://gitea.com/JerryALT/iw3sp_mod`
3. After cloning click the `generate.bat` and wait when all files will be ready.
4. Open build folder and find the `iw3sp_dev.sln` file and compile.
