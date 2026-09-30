# BewareTheSun

Petit Jasmin, a potted jasmine, must escape the scorching sun. Collect every
water droplet to unlock the watering can and move on to the next garden.

- **7 procedurally generated levels** — a brand-new layout every run
  (press `N` in-game to reroll the current garden).
- **Day/night switching** (`T`): by day the sun burns you unless you stay in
  shade (flower platforms bloom and the light is ray-traced around
  platforms); by night enemies prowl, flowers wilt, and mushrooms become
  trampolines.
- **All level art is generated procedurally at startup** — pixel-art sprites
  and painted per-biome backgrounds, no image files needed for level elements.
- Modern platformer feel: coyote time, jump buffering, variable jump height,
  enemy stomping, knockback + invulnerability, shade regeneration, particles
  and screen shake.

**Controls:** Arrows/WASD move, SPACE/W/UP jump, `T` day-night, `R` restart,
`N` new layout, `H` debug overlay, ESC settings.

**Quick build (macOS, with `brew install raylib`):**

```bash
clang++ -std=c++17 -O2 src/main.cpp src/Game.cpp src/entities/*.cpp \
  src/world/*.cpp src/gfx/*.cpp -Isrc -I/opt/homebrew/include \
  -L/opt/homebrew/lib -lraylib -o build/BewareTheSun
./build/BewareTheSun   # run from the repo root (needs assets/)
```

Setting `BTS_SHOT=<level>` runs a self-test that screenshots day and night
of that level and exits.

## Web version of the game (old jam build)

[Play BewareTheSun](https://bewarethesun.netlify.app/)

## Installation

[Complete README and official repo](https://github.com/raylib-extras/raylib-quickstart)

# VSCode Users (all platforms)
*Note* You must have a compiler toolchain installed in addition to vscode.

1. Download the quickstart
2. Rename the folder to your game name
3. Open the folder in VSCode
4. Run the build task ( CTRL+SHIFT+B or F5 )
5. You are good to go

# Windows Users
There are two compiler toolchains available for windows, MinGW-W64 (a free compiler using GCC), and Microsoft Visual Studio
## Using MinGW-W64
* Rename the folder to your game name
* Double click the `build-MinGW-W64.bat` file
* CD into the folder in your terminal
  * if you are using the W64devkit and have not added it to your system path environment variable, you must use the W64devkit.exe terminal, not CMD.exe
  * If you want to use cmd.exe or any other terminal, please make sure that gcc/mingw-W64 is in your path environment variable.
* run `make`
* You are good to go

### Note on MinGW-64 versions
Make sure you have a modern version of MinGW-W64 (not mingw).
The best place to get it is from the W64devkit from
https://github.com/skeeto/w64devkit/releases

or the version installed with the raylib installer

#### If you have installed raylib from the installer
Make sure you have added the path

`C:\raylib\w64devkit\bin`

To your path environment variable so that the compiler that came with raylib can be found.

DO NOT INSTALL ANOTHER MinGW-W64 from another source such as msys2, you don't need it.

## Microsoft Visual Studio
* Rename the folder to your game name
* Run `build-VisualStudio2022.bat`
* double click the `.sln` file that is generated
* develop your game
* you are good to go

# Linux Users
* Rename the folder to your game name
* CD into the build folder
* run `./premake5 gmake`
* CD back to the root
* run `make`
* you are good to go

# MacOS Users
* Rename the folder to your game name
* CD into the build folder
* run `./premake5.osx gmake`
* CD back to the root
* run `make`
* you are good to go

# Output files
The built code will be in the bin dir

### MyGame

A game built with Raylib following a clean architecture separating Logic, Data, and Rendering.

#### Structure
- **src/core**: Global constants and types.
- **src/entities**: Dynamic objects like Player and Enemies.
- **src/world**: Static objects like Platforms and Levels.
- **assets**: Game assets (sprites, audio).

#### Building
```bash
mkdir build
cd build
cmake ..
make
./MyGame
```

