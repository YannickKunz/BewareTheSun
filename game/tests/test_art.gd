extends SceneTree
const Art = preload("res://scripts/art.gd")
var assertions := 0
var failures := 0

func _initialize() -> void:
	run.call_deferred()

func check(condition: bool, text: String) -> void:
	assertions += 1
	if not condition:
		failures += 1
		push_error("ART FAIL: " + text)

func run() -> void:
	var manifest: Dictionary = JSON.parse_string(FileAccess.get_file_as_string("res://assets/models/manifest.json"))
	check(manifest.assets.size() == 21, "Complete Blender library")
	var world := Node3D.new()
	root.add_child(world)
	for asset: String in manifest.assets:
		var node := Art.model(asset, world)
		var meshes := node.find_children("*", "MeshInstance3D", true, false)
		check(not meshes.is_empty(), asset + " imports actual geometry")
		for mesh: MeshInstance3D in meshes:
			check(mesh.mesh is ArrayMesh and mesh.mesh.get_surface_count() > 0, asset + " uses imported mesh data")
		for clip: String in manifest.assets[asset].clips:
			check(node.has_meta("animator"), asset + " imports animation")
			var animator: AnimationPlayer = node.get_meta("animator")
			check(animator.has_animation(clip), asset + " includes " + clip)
			Art.animate(node, clip, .18)
		check(int(manifest.assets[asset].triangles) < 20000, asset + " stays within per-model triangle budget")
		world.remove_child(node)
		node.free()
	var player := Art.player()
	world.add_child(player)
	var foot := player.find_child("JasminFootL", true, false) as Node3D
	step_clip(player, "idle", .2)
	var rest := foot.position
	step_clip(player, "walk", .24)
	check(foot.position.distance_to(rest) > .02, "Blender walk clip actually moves the root boot")
	var pose := foot.transform
	await create_timer(.1).timeout
	check(foot.transform.is_equal_approx(pose), "Imported clip does not advance without simulation time")
	step_clip(player, "idle", .35)
	check(foot.position.distance_to(rest) < .005, "Idle restores the feet after walking")
	Art.animate(player, "walk", .2, false)
	var reduced_pose := foot.transform
	Art.animate(player, "walk", .5, false)
	check(foot.transform.is_equal_approx(reduced_pose), "Reduced motion holds a stable imported pose")
	var vines := Art.model("vine_gate", world)
	Art.animate(vines, "open", 1.1)
	var hedge := vines.find_child("LivingVines", true, false) as Node3D
	check(hedge.position.y < -1.6, "Authored opening clip lowers the living gate")
	var opened := hedge.position
	Art.animate(vines, "open", .2)
	check(hedge.position.distance_to(opened) < .001, "Completed opening clip does not restart")
	var tree_a := Art.tree(world, Vector2(-2, 0), 0)
	var tree_b := Art.tree(world, Vector2(2, 0), 0)
	var mat_a: StandardMaterial3D = tree_a.get_meta("canopy_materials")[0]
	var mat_b: StandardMaterial3D = tree_b.get_meta("canopy_materials")[0]
	mat_a.albedo_color.a = .18
	check(mat_b.albedo_color.a > .99, "Canopy fade materials are isolated between trees")
	var flower := Art.flower(world, Vector2.ZERO, true)
	check(flower.get_meta("head") is Node3D, "Sunflower has the gameplay bloom pivot")
	var moon := Art.flower(world, Vector2.ONE, false)
	check(moon.get_meta("head") is Node3D, "Moonflower has the gameplay bloom pivot")
	check(player.get_meta("lens") is MeshInstance3D, "Lantern lens supports polarity changes")
	world.queue_free()
	await process_frame
	Art.scenes.clear()
	Art.palette.clear()
	print("ART: %d assertions; %d failures" % [assertions, failures])
	quit(0 if failures == 0 else 1)

func step_clip(node: Node3D, clip: String, duration: float) -> void:
	# Match the real fixed-step driver, including AnimationPlayer crossfade completion.
	for frame in range(ceili(duration * 60)):
		Art.animate(node, clip, 1.0 / 60.0)
