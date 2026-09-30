"""Create an editable Blender cast from the exact Godot runtime meshes.

First: godot --headless --path game --script tools/bake_models.gd
Then: blender --background --factory-startup --python tools/build_models.py

This makes a NEW .blend file. It never changes a user's open Blender session.
The GLB geometry is authored in game/scripts/art.gd, not downloaded.
"""
from pathlib import Path
import bpy

ROOT = Path(__file__).resolve().parents[1]
MODELS = ROOT / "game/assets/models"
OUTPUT = ROOT / "art-source"
OUTPUT.mkdir(exist_ok=True)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
for index, name in enumerate(["petit_jasmin", "dusk_beetle", "umbrella_tree", "garden_gate", "sunflower"]):
    path = MODELS / (name + ".glb")
    if not path.is_file():
        raise FileNotFoundError(f"Run the Godot mesh baker first: {path}")
    existing = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=str(path))
    for obj in set(bpy.data.objects) - existing:
        if obj.parent is None:
            obj.location.x += index * 4
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / "garden_cast.blend"))
print("Saved editable cast:", OUTPUT / "garden_cast.blend")
