class_name GardenArt
extends RefCounted
## Blender is the source of truth. This adapter only instances imported scenes,
## controls their authored clips, and assigns gameplay-dependent materials.
const CLAY := Color("b9603c")
const JADE := Color("41765a")
const GOLD := Color("eab955")
const IVORY := Color("f6e8bb")
const NIGHT := Color("75cbd2")
static var scenes: Dictionary = {}
static var palette: Dictionary = {}

static func model(asset: String, parent: Node3D = null, pos: Vector3 = Vector3.ZERO) -> Node3D:
	if not scenes.has(asset):
		scenes[asset] = load("res://assets/models/%s.glb" % asset)
	var wrapper := Node3D.new()
	wrapper.name = asset.to_pascal_case()
	var imported: Node3D = scenes[asset].instantiate()
	imported.name = "Model"
	wrapper.add_child(imported)
	wrapper.position = pos
	wrapper.set_meta("blender_asset", asset)
	var animator := imported.find_child("AnimationPlayer", true, false) as AnimationPlayer
	if animator:
		animator.deterministic = true
		animator.callback_mode_process = AnimationMixer.ANIMATION_CALLBACK_MODE_PROCESS_MANUAL
		for clip in animator.get_animation_list():
			if clip != "RESET" and clip != "open":
				animator.get_animation(clip).loop_mode = Animation.LOOP_LINEAR
		wrapper.set_meta("animator", animator)
	if parent:
		parent.add_child(wrapper)
	return wrapper

static func animate(node: Node3D, clip: String, delta: float, motion: bool = true, speed: float = 1.0) -> void:
	if not node.has_meta("animator"):
		return
	var animator: AnimationPlayer = node.get_meta("animator")
	# Remember the selected clip separately: completed one-shots clear current_animation.
	if node.get_meta("selected_clip", "") != clip:
		animator.play(clip, .12)
		node.set_meta("selected_clip", clip)
	if motion:
		animator.advance(delta * speed)
	else:
		animator.seek(animator.get_animation(clip).length if clip == "open" else 0.0, true)

static func mat(color: Color, glow: float = 0.0) -> StandardMaterial3D:
	var key := str(color) + str(glow)
	if palette.has(key):
		return palette[key]
	var m := StandardMaterial3D.new()
	m.albedo_color = color
	m.roughness = .72
	if glow > 0:
		m.emission_enabled = true
		m.emission = color
		m.emission_energy_multiplier = glow
	palette[key] = m
	return m

static func tint(node: Node3D, color: Color, glow: float = 0.0) -> void:
	for mesh: MeshInstance3D in node.find_children("*", "MeshInstance3D", true, false):
		mesh.material_override = mat(color, glow)

static func marker(parent: Node3D, p: Vector2, radius: float, color: Color) -> Node3D:
	var node := model("ring_marker", parent, Vector3(p.x, .045, p.y))
	node.scale = Vector3.ONE * radius
	tint(node, color, .12)
	return node

static func player() -> Node3D:
	var node := model("petit_jasmin")
	node.set_meta("lens", node.find_child("LanternLens", true, false))
	return node

static func beetle() -> Node3D:
	return model("dusk_beetle")

static func tree(parent: Node3D, p: Vector2, variation: float) -> Node3D:
	var node := model("umbrella_tree", parent, Vector3(p.x, 0, p.y))
	node.rotation.y = variation
	var materials: Array = []
	var crown := node.find_child("CanopyCrown", true, false) as MeshInstance3D
	for i in range(crown.mesh.get_surface_count()):
		var material := crown.get_active_material(i).duplicate() as StandardMaterial3D
		material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
		crown.set_surface_override_material(i, material)
		materials.append(material)
	node.set_meta("canopy_materials", materials)
	var shelter := model("shelter_marker", parent, Vector3(p.x, 0, p.y))
	for mesh: MeshInstance3D in shelter.find_children("*", "MeshInstance3D", true, false):
		mesh.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		for surface in range(mesh.mesh.get_surface_count()):
			var ink := mesh.get_active_material(surface).duplicate() as StandardMaterial3D
			ink.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
			ink.albedo_color.a = .24 if surface == 0 else .65
			mesh.set_surface_override_material(surface, ink)
	return node

static func gate(parent: Node3D, p: Vector2) -> Node3D:
	var node := model("garden_gate", parent, Vector3(p.x, 0, p.y))
	marker(parent, p, .65, NIGHT)
	return node

static func flower(parent: Node3D, p: Vector2, day: bool) -> Node3D:
	var node := model("sunflower" if day else "moonflower", parent, Vector3(p.x, 0, p.y))
	node.set_meta("head", node.find_child("SunflowerHead" if day else "MoonflowerHead", true, false))
	marker(parent, p, .55, GOLD if day else NIGHT)
	return node
