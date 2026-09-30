# Beware the Sun — The Little Eclipse

A complete **Godot 4 / 3D reimagining** of Beware the Sun. Petit Jasmin is still a
small potted jasmine trying to get home. This time, he carries a lantern that can
borrow daylight—or make a pocket of night.

The new game lives in **`game/`**. The original Raylib/C++ game and assets are preserved. Its old web build script
is retained as `tools/build_legacy_web.sh` (outputs to `legacy_web_build/`). See [the legacy README](docs/LEGACY.md).

## Play

```sh
./play.sh
```

Or open **`game/project.godot`** in Godot and press **F5** to run the main scene.
Godot 4.3+ is the target; **4.7.2** is the version used for the headless tests and
exports. Compatibility rendering is selected for native and web builds.

On macOS, you can also double-click **`Play Beware the Sun.command`**. The launcher
uses `godot` on your PATH or `/Applications/Godot.app`. Override with `GODOT=/path/to/godot`.

### Controls

| Input | Action |
| --- | --- |
| WASD / arrow keys | Move around the garden |
| Mouse | Aim the lantern |
| **Q** / right mouse / T | Switch **DAY ↔ NIGHT** polarity |
| **F** | Switch the lantern off/on |
| **Space** | Short dash; brief contact invulnerability |
| Esc | Pause / resume; sound, reduced motion, menu and quit |
| R | Restart the current garden |
| M | Toggle sound |
| F11 | Toggle fullscreen |
| Enter | Start / retry / next garden |

Gamepad mappings are implemented: left stick moves, right stick aims, A dashes,
X switches polarity, Y toggles the lantern, Start pauses. Hardware gamepad testing
has not been performed. The browser build is intended for desktop keyboard/mouse,
not touch-only devices.

## The lantern is local—not a global time switch

- **NIGHT:** paints a cool cone and a small protective halo around Jasmin. It
  blocks solar exposure, but can also awaken beetles inside its footprint.
- **DAY:** petrifies dusk beetles and charges gold sunflower locks. The borrowed
  light is cool: it does not create solar damage in a nighttime garden.
- **Gold flowers require DAY; blue flowers require NIGHT.** Keep the correct
  lantern footprint on them for just over two seconds. Fully charged flowers
  stay latched; unfinished charges slowly fade. The living gate opens when every
  flower is charged.
- The sky stays fixed for each garden. In daytime, **marked circles beneath trees
  are safe shelter**. Turn the lantern off there to recover charge and vitality.
- Lantern charge is finite. It recharges while off, faster in tree shelter. Empty
  lamps remain off until you press F; they do not flicker themselves back on.
- Collect **all dew drops** and bloom **all locks** to open the home arch. Dew
  restores some vitality and lantern charge.
- Beetles have a short wake-up delay when the light leaves them. Use that moment
  to move past, or dash away. Low walls, ponds, and closed living gates block
  movement, including dashes.

## Six authored gardens

1. **The Sleeping Garden** — shelter, dew, and carrying your own eclipse.
2. **Things in the Dark** — the day lantern and nocturnal beetles.
3. **The Sunflower Lock** — charging flowers to open a living gate.
4. **The Noonday Crossing** — sunlight and mixed-polarity locks.
5. **The Lunar Conservatory** — switching polarity while creatures approach.
6. **Where Jasmine Blooms** — the combined finale.

Includes a title screen, contextual hints, pause/settings, retry, garden-complete
and ending screens, saved unlock progress, synthesized music and effects, and a
reduced-motion setting. Progress and preferences use Godot's local `user://garden.cfg`;
there are no external services or accounts.

## Original art and animation

No old sprites, stock models, or old audio are used by the rework.

- `game/scripts/art.gd`: original editable low-poly clay pot, jasmine, beetles,
  trees, flowers, garden arches, paving, water, and scenery.
- `game/scripts/game.gd`: walk bob, alternating feet, bending stems, canopy sway,
  beetle gait/petrification, floating dew, flower bloom, opening vines, and bursts.
  Crowns fade near the player so they do not hide Jasmin.
- `game/shaders/ground.gdshader`: local day/night footprint, matched to gameplay
  range, cone angle, and protective halo.
- `tools/build_audio.py`: reproducible original chimes, movement effects, and a
  quiet 32-second ambient score. Python standard library only.
- `game/assets/icon.svg`: original plant-and-eclipse icon.
- **Fraunces** and **DM Sans** are the only third-party art dependencies; both
  ship with their SIL Open Font Licenses in `game/assets/fonts/`.

### Blender handoff

The exact in-game cast is provided as **five editable GLB models** in
`game/assets/models/`. Import one using Blender's glTF importer, or generate a
single arranged Blender source file:

```sh
# Rebuild GLBs and the rasterized icon from the Godot art source:
godot --headless --path game --script tools/bake_models.gd

# Import those GLBs and save art-source/garden_cast.blend:
blender --background --factory-startup --python tools/build_models.py
```

The Blender script creates a new file; it does not operate on an open scene.
Animation remains authored in Godot rather than being baked into the GLBs.
The Blender handoff script could not be executed in the development session:
Blender crashed during Metal initialization. GLB export itself was verified.

## Verify

```sh
./tools/test.sh
```

This imports the project and runs headless assertions for beam geometry,
polarity, shade/heat, charging, collision, enemy states, cooldowns, objective
latching, progression, pause/death/retry, and all six gardens. The suite flood-fills
layouts with the actual player radius and walks full resource-constrained routes
through every garden without teleporting or granting free health/charge.

For visual captures on a machine with a working graphics session:

```sh
./play.sh -- --capture=title
./play.sh -- --capture=day
./play.sh -- --capture=night
```

Captures are saved in `game/test-output/`. Capture/test modes do not modify saves.
See [verification notes](docs/VERIFICATION.md) for observed results and limitations.

## Export

Install export templates matching your Godot version, then:

```sh
./tools/export.sh Web
./tools/export.sh macOS
```

Outputs: `game/exports/web/index.html` and `game/exports/BewareTheSun.zip`.
The macOS export is unsigned and not notarized. Web is single-threaded, so it does
not require cross-origin isolation headers. Serve it over HTTP rather than opening
`index.html` directly:

```sh
python3 -m http.server 8791 --directory game/exports/web
```

### Replace the published web build

```sh
./web_build.sh
```

This exports Godot, checks the generated runtime files, and replaces the tracked
`web_build/` directory. Obsolete Emscripten assets are removed. An incomplete
export leaves the previous build untouched. Run the publication regression tests
with `python3 -m unittest discover -s tools/tests -v`. Commit the updated `web_build/`
alongside source changes and push to the Netlify-connected branch.

`netlify.toml` publishes **`web_build/`** as prebuilt static files, with no cloud
build command or Godot installation required. It includes all HTML, JavaScript,
WebAssembly, resource-pack, image, and audio-worklet files—not just `index.html`.

Nothing is uploaded or deployed by the local build commands; a Git push can
trigger the existing Netlify continuous-deployment connection.
