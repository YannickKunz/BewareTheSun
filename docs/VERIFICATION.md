# Verification — September 30, 2026: Blender visual rework

## Passing automated checks

- **Blender 5.2.1 LTS:** native authoring recipe completed with exit 0, produced
  `art-source/garden_library.blend`, 21 GLBs, authored NLA clips, and the Cycles
  title illustration. The saved `.blend` was then opened in a separate background
  Blender process and exported successfully through the artist-edit exporter.
- **Godot 4.7.2:** project import and GDScript compilation succeed.
- **209 gameplay assertions:** local phase geometry, heat/shade, battery, pickups,
  enemies, collisions, cooldowns, pause/retry, and resource-constrained routes
  completing all six gardens. No free health/charge or teleportation is used in
  the route tests.
- **202 imported-art assertions:** actual GLB geometry, per-model triangle budgets,
  expected clips, animated foot movement and walk-to-idle restoration, manually
  advanced/frozen animation, reduced motion, latched gate opening, independent
  canopy materials, and the named flower/lantern pivots.
- **Three Python publication tests:** successful web replacement, missing-worklet
  rejection, and truncated-pack rejection without destroying the previous build.
- The generated WebAssembly module compiles with Node's WebAssembly engine.
- Web and unsigned macOS release exports succeed. The current macOS resource pack
  runs for 60 frames in headless Godot without errors.
- Python scripts compile, shell scripts pass `sh -n`, and `git diff --check` passes.

## Rendered checks performed

The earlier sandbox/GPU blocker is resolved; these checks actually ran this time.

- Blender Cycles character showcase rendered and visually inspected.
- Native Godot title, daytime and nighttime captures rendered on Apple M4 OpenGL
  compatibility mode, at the default 1280×800 window size. Captures were inspected
  for model visibility, lighting, cone/halo placement and HUD composition.
- Browser export loaded at 1440×900 through a local HTTP server. The title render,
  actual garden, player movement, dash, polarity change and pause overlay were
  captured and inspected. The automated browser sequence recorded **no JavaScript
  page errors or error-level browser console messages**.
- Fixed overexposed day lighting, excessive ground noise, shelter-disc overlap,
  and focused primary-button text contrast during the visual pass.

Local captures are under `game/test-output/` (ignored by Git). The production title
illustration is `game/assets/art/title_scene.png`; its editable studio is included
in the native Blender source.

## Scope and remaining limitations

- This was an automated browser interaction check and visual review, not a full
  human playthrough or difficulty-balancing pass across all six gardens.
- Hardware gamepads, touch-only play, other browser engines, subjective audio
  quality, and unsigned macOS launch through Finder have not been verified.
- The refreshed Blender-art browser build is in tracked `web_build/`. Its matching
  unsigned macOS ZIP is in ignored `game/exports/`.
- Production deployment is a separate check; local builds and visual QA alone do
  not establish that Netlify has published a given commit.
