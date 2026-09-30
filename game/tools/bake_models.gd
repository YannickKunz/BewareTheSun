extends SceneTree
## Compatibility notice for the retired Godot-to-Blender exporter.
func _initialize() -> void:
	printerr("Assets are now authored in Blender. Run: blender --background --factory-startup --python tools/build_models.py")
	quit(1)
