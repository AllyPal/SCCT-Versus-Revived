# SCCT-Versus-Revived

Enhancements and fixes for Splinter Cell Chaos Theory (SCCT) Versus.

## Features:
* Allows users to set a frame limit (no longer capped at 64 FPS on Windows with SCCT Frame Rate Editor/Framer)
* Fixes mouse input at high hz and produces an identical experience at high and low FPS
* Fixes animated textures/shaders (e.g. the water effect on Aquarius) that run too fast at higher frame rates.  Textures/shaders which already specify a limit will retain their original cap
* Fixes issue where grenades (frag, chaff, smoke etc) intermittently instantly explode on impact with the floor
* Fixes widescreen aspect ratio (stretched look)
* Improved frame timing for more consistent frame rate.  Configurable with `frameTimingMode`:
  * "0" highest CPU usage, highest compatibility
  * "1" lower CPU usage
  * "2" lowest precision (closest to original game)
* Adds `-join <ip:port>` command to allow joining servers without a VPN.  

  The server must be using SCCT-Versus-Revived, but does not need a command line.

  1. On the client, launch the game with `-join` in your command line (e.g. `SCCT_Launcher.exe -join 192.168.50.142:7776`)
  2. Select "Play on LAN" -> "Find A Session"

  For now, using `-join` will disable automatic LAN game discovery.

## Installation
* Unzip files into the root directory of SCCT Versus
* Launch the game with SCCT_Launcher.exe
* After the first run, SCCT_config.json will be created.  You can set your desired frame rate here.
* It is not advised to run above 30 FPS whilst hosting

## Latest Beta Build:
https://nightly.link/AllyPal/SCCT-Versus-Revived/workflows/msbuild/main/Beta-Release.zip
