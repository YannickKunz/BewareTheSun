# Blender source — The Little Eclipse

**Open `garden_library.blend` in Blender 5.2.1 or compatible newer Blender.**
This is the original modeling/animation source, not a reverse import from Godot.
All geometry and materials are original; no external meshes or textures are needed.

## Scenes

- **Garden asset library:** 21 named asset collections, laid out in a grid. The
  full-size garden base is positioned separately behind the smaller assets.
- **Character showcase:** a lit Cycles studio with linked mesh copies of the
  characters/plants. This produces `game/assets/art/title_scene.png`.

## Export your edits

Save the `.blend`, then run from the repository root:

```sh
blender --background art-source/garden_library.blend --python tools/export_blender_assets.py
./web_build.sh
```

The exporter temporarily resets each collection root's layout position to zero
for its GLB, then restores it. It never saves over your Blender source. Add
`-- --render-title` to also render the showcase scene. Object-level changes to
library objects may need matching edits to their showcase copies; shared mesh
and material edits are already linked.

**Do not run `tools/build_models.py` to export manual edits.** It regenerates and
overwrites the library, GLBs, materials, animation and title render from the recipe.
Use it only when intentionally rebuilding the original art from scratch.

## Contracts used by the game

- Collection/root names match GLB filenames and entries in `manifest.json`.
- Blender is Z-up and meters; the glTF exporter converts to Godot's Y-up.
  Characters face Blender -Y / Godot +Z. World placement belongs to Godot wrappers.
- Keep `GroundSurface` separate from the base's structural meshes.
- Keep `JasminFootL`, `JasminFootR`, `JasminStem`, and `LanternLens` named.
- Keep `CanopyCrown` separate so nearby crowns can become translucent.
- Keep `SunflowerHead` / `MoonflowerHead` as charge-controlled pivots.
- Keep `LivingVines` as the gate's opening-animation pivot.
- Matching NLA track names combine animated objects into exported clips:
  `idle`, `walk`, `scuttle`, `sleep`, `sway`, `float`, `open`.
- Tracks are muted in the library's default layout so its rest poses stay tidy;
  the glTF NLA-track exporter exports them. Do not delete them to unpose the model.
- Keep assets below 20,000 triangles each. Current character is roughly 10,700.

Godot retains UI, dynamic flashlight shading, actor placement/collision, particle
trajectories, canopy fading, and flower-charge feedback. Asset geometry and the
loop/one-shot motion clips live in Blender.

## Verify after export

```sh
./tools/test.sh
```

This runs the campaign rules/layout tests and the imported-art integration suite.
A retired `game/tools/bake_models.gd` command now exits with guidance rather than
silently overwriting Blender exports with the old procedural geometry.
