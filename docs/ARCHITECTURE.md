# The Little Eclipse — rework architecture

The Godot project is deliberately self-contained in `game/`; nothing in the
legacy C++ build is a dependency.

## Boundaries

- **`scripts/rules.gd`** — pure cone/halo membership, local polarity, shelter,
  exposure, flower charge, and expanded-rectangle collision rules. No rendering
  or input dependencies.
- **`scripts/levels.gd`** — six deterministic, authored layout dictionaries. World
  positions use `Vector2(x, z)`. Movement stays on the garden plane.
- **`scripts/game.gd`** — scene assembly, fixed-step simulation, input, movement,
  enemy behavior, resource management, audio, progression, and persistence.
- **`scripts/art.gd`** — instances the Blender-authored GLB scenes, advances their
  imported animation clips manually, and assigns gameplay-specific material
  overrides. GLBs are now runtime assets, not just an optional art handoff.
- **`scripts/hud.gd`** — scalable 1440×900 design-space drawing with native Godot
  buttons for clickable, keyboard-focusable menu actions. World markers project
  through the actual camera.
- **`shaders/ground.gdshader`** — visible lantern footprint. Its range (6.8 m),
  half-angle (0.52 rad), and halo radius (1.25 m) match `rules.gd`; changing those
  values requires changing both the shader and the rules.

## Important decisions

- The lantern has a finite forward cone **and a carrier halo**. Without the halo,
  aiming ahead would leave the plant outside its own nighttime shelter.
- Ambient sky time is fixed by level. Only locations inside the active lantern
  footprint switch phase; different entities can therefore have different phase
  states at the same time.
- Solar damage requires a daytime sky. Day-polarity lantern light can stun bugs
  in a night level without suddenly burning the player.
- Shelter is signposted by ground circles, not inferred from rendered shadows.
  Lighting and graphics quality cannot silently change collision or safety.
- Low walls block actors but not lantern light; there is no beam-occlusion puzzle.
- Movement is planar and substepped, with axis-separated sliding. Dashes cannot
  cross walls, ponds, or closed gates. This is a top-down adventure, not a port of
  the original platformer physics.
- Flowers need active lantern illumination of the correct polarity. Ambient
  daylight alone does not solve them. Full charge latches permanently for the
  current attempt; partial charge decays.
- Beetles patrol/chase only in local night. Local daylight stops them, followed by
  a 1.2-second wake delay. Damage has a cooldown independent of their state.
- Completing a level stops combat updates immediately; an enemy cannot turn a
  successful finish into a death later in the same frame.
- Pause stops gameplay clocks and resources. Restart rebuilds all dynamic state.
- Saves contain only the highest unlocked level and accessibility/audio settings.
  Test and capture modes never overwrite real saves.

## Asset pipeline

`tools/build_models.py` authors geometry, materials, NLA clips and a studio scene
in Blender → `art-source/garden_library.blend` + 21 runtime `.glb` files + the
Cycles title render. `tools/export_blender_assets.py` exports later artist edits
from the native `.blend` without rerunning the generator.

`game/assets/models/manifest.json` records exported model sizes, triangle counts,
and expected clip names. `game/tests/test_art.gd` loads the actual Godot imports
and verifies geometry, clip behavior, and the named gameplay pivots/materials.

Imported models sit under a separate placement wrapper. This keeps authored
local animation transforms independent of world movement and collision. Clips
run in manual processing mode; the game supplies time only while unpaused.
Repeating clips loop; the gate's opening clip is a latched one-shot. Its selected
clip is tracked separately from AnimationPlayer's current clip to avoid replaying
it when Godot clears a finished animation.

The flower head tilt/scale follows gameplay charge; canopy alpha and lantern-lens
color remain runtime overrides. Each tree receives independent canopy material
instances so fading one cannot affect the other trees. The ground's dynamic
lantern shader is applied only to the named `GroundSurface` mesh, not the slab.

`tools/build_audio.py` generates the original WAV effects and ambient score.

## Adding a garden

Extend the array in `levels.gd`, then update the six-garden UI counts and the
campaign tests. Keep spawn, all required flowers, and charge-access points
reachable before the living gate opens. Every dew and the exit must be reachable
after it opens. Run `tools/test.sh`; its flood-fill uses the same blockers and
player radius as runtime, followed by full resource-constrained route tests.
