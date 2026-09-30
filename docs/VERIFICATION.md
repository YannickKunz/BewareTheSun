# Verification — September 29, 2026

## Observed passing checks

- Godot **4.7.2** project import and GDScript compilation.
- **209 assertions, zero failures** in `game/tests/test_game.gd`.
- All six layout flood-fills using the actual player radius and live blockers.
- Full simulated routes through all six gardens, including lantern charging,
  actual movement, exposure, enemy updates, dew pickup and completion. No
  teleportation, free refills, or invincibility is granted by those route tests.
- Original runtime cast exported to five GLB files with Godot's glTF exporter.
- Python asset scripts compile; shell launch/test/export scripts pass `sh -n`.
- Web release export succeeds.
- Unsigned macOS release ZIP export succeeds.
- The packed resource file extracted from the macOS export starts and runs for
  60 frames under the headless engine without script/runtime errors.
- `git diff --check` passes.

The six scripted route finishes retained 100 vitality; remaining charge was
92.5%, 92.5%, 99.0%, 94.6%, 80.4%, and 99.0% respectively. These are deterministic
smoke routes, not a claim that the difficulty has been human-playtested.

## Not verified yet

- **Rendered visual QA and human gameplay.** The active workspace-write sandbox
  cannot access the macOS graphics session. Native Godot rendering and isolated
  browser rendering could not be completed here. Headless correctness does not
  verify shader appearance, visual composition, or frame rate.
- **Blender source bake.** Blender 5.2.1 crashes in its Metal capability probe
  inside this sandbox before executing Python. The user's updated instructions
  identify the unrestricted `full` profile as the required environment. The
  running Blender UI was not accessed or modified.
- Hardware gamepad behavior, browser interaction/audio, and unsigned app launch
  through Finder have not been tested.

## Follow-up in the user's full-profile session

1. Run `./tools/test.sh` again.
2. Run `blender --background --factory-startup --python tools/build_models.py`.
   Confirm that `art-source/garden_cast.blend` opens and contains the five models.
3. Run `./play.sh -- --capture=title`, `day`, and `night` separately (replace the
   capture value for each run); inspect the PNGs under `game/test-output/`.
4. Human-play all six gardens. Verify player visibility through tree crowns,
   cone/halo alignment, readable HUD/menus, creature wake-up feedback, and audio.
5. Check 1280×800 and 1440×900, reduced motion, pause, and fullscreen.
6. Re-export after any fixes. Nothing has been deployed or pushed.

## Replacement web build

The Godot release export now replaces the legacy files in tracked `web_build/`.
`web_build.sh` regenerates and validates this folder. `netlify.toml` selects it
as the prebuilt publish directory; it does not require a cloud Godot installation.

Additional passing checks: three publisher regression tests (successful replacement,
missing-worklet rejection, truncated-pack rejection), compilation of the exported
WebAssembly module by Node's WebAssembly engine, and a 60-frame headless startup
of the exact resource pack in `web_build/`. The 209 game assertions still pass.

Commit/push was attempted but stopped at Git staging: creating `.git/index.lock`
was denied by the current session's filesystem permissions. No commit or push
was made; the replacement has not been deployed by this session.
