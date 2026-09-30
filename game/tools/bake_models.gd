extends SceneTree
## Export the exact runtime meshes as editable GLB models for Blender or other DCCs.
const Art = preload("res://scripts/art.gd")
func _initialize() -> void:
	bake.call_deferred()

func bake() -> void:
	var icon := Image.new()
	var icon_error := icon.load_svg_from_string(FileAccess.get_file_as_string("res://assets/icon.svg"))
	if icon_error == OK:
		icon.save_png("res://assets/icon.png")
	var models := {"petit_jasmin":Art.player(),"dusk_beetle":Art.beetle()}
	var holder := Node3D.new()
	models["umbrella_tree"] = Art.tree(holder,Vector2.ZERO,0)
	models["garden_gate"] = Art.gate(holder,Vector2.ZERO)
	models["sunflower"] = Art.flower(holder,Vector2.ZERO,true)
	for model_name in models:
		var node: Node3D = models[model_name]
		if node.get_parent():
			node.get_parent().remove_child(node)
		root.add_child(node)
		var document := GLTFDocument.new()
		var state := GLTFState.new()
		var err := document.append_from_scene(node,state)
		if err == OK:
			err = document.write_to_filesystem(state,"res://assets/models/%s.glb" % model_name)
		if err != OK:
			push_error("Model export failed: %s (%s)" % [model_name,err])
			quit(1)
			return
		print("Baked original mesh: ",model_name)
		node.queue_free()
	holder.free()
	await process_frame
	quit()
