"""Export edited native Blender collections without regenerating their geometry.

blender --background art-source/garden_library.blend --python tools/export_blender_assets.py
Optional: add -- --render-title to also re-render the showcase scene.
"""
from pathlib import Path
import bpy
import json
import sys

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'game/assets/models'
manifest = json.loads((OUT / 'manifest.json').read_text())
scene = bpy.data.scenes['Garden asset library']
bpy.context.window.scene = scene
for name, info in manifest['assets'].items():
    collection = bpy.data.collections[name]
    root = collection.objects[name]
    origin = root.location.copy()
    try:
        root.location = (0, 0, 0)
        scene.frame_set(1)
        bpy.ops.object.select_all(action='DESELECT')
        for obj in collection.objects:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = root
        bpy.ops.export_scene.gltf(
            filepath=str(OUT / (name + '.glb')), export_format='GLB', use_selection=True,
            export_animations=True, export_animation_mode='NLA_TRACKS', export_frame_range=False,
            export_force_sampling=True, export_optimize_animation_size=False, export_materials='EXPORT', export_yup=True, export_extras=True,
        )
        meshes = [obj for obj in collection.objects if obj.type == 'MESH']
        info['triangles'] = sum(sum(len(p.vertices) - 2 for p in obj.data.polygons) for obj in meshes)
        info['mesh_objects'] = len(meshes)
        info['bytes'] = (OUT / (name + '.glb')).stat().st_size
    finally:
        root.location = origin
manifest['generator'] = 'Blender ' + bpy.app.version_string
manifest['export_mode'] = 'native blend collections (preserves artist edits)'
(OUT / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
if '--render-title' in sys.argv:
    bpy.context.window.scene = bpy.data.scenes['Character showcase']
    bpy.context.scene.render.filepath = str(ROOT / 'game/assets/art/title_scene.png')
    bpy.ops.render.render(write_still=True)
print('BLENDER_NATIVE_EXPORT_OK', len(manifest['assets']))
