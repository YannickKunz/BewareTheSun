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
- **`scripts/art.gd`** — shared material palette and original procedural meshes.
  Named character pieces support direct procedural animation. No external mesh
  asset is needed for runtime; the GLBs are an editable art handoff.
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

`art.gd` → `game/tools/bake_models.gd` → editable `.glb` files → optional
`tools/build_models.py` → `art-source/garden_cast.blend`.

`tools/build_audio.py` → original mono WAV effects and ambient music.

The character's feet, stem, and root are animated in Godot. Beetle legs and body,
canopies, flower heads, dew, and living gate roots follow the same pattern. No
baked animation clips are included in the Blender handoff.

## Adding a garden

Extend the array in `levels.gd`, then update the six-garden UI counts and the
campaign tests. Keep spawn, all required flowers, and charge-access points
reachable before the living gate opens. Every dew and the exit must be reachable
after it opens. Run `tools/test.sh`; its flood-fill uses the same blockers and
player radius as runtime, followed by full resource-constrained route tests.
