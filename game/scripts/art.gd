class_name GardenArt
extends RefCounted
## Entirely original mesh art, built from a small shared material palette.
## Named components stay editable and support procedural skeletal-style animation.
static var palette: Dictionary = {}
const CLAY := Color("b9603c")
const JADE := Color("41765a")
const GOLD := Color("eab955")
const IVORY := Color("f6e8bb")
const NIGHT := Color("75cbd2")

static func mat(color: Color, glow: float = 0.0) -> StandardMaterial3D:
	var key := str(color) + str(glow)
	if palette.has(key):
		return palette[key]
	var m := StandardMaterial3D.new()
	m.albedo_color = color
	m.roughness = 0.86
	if glow > 0:
		m.emission_enabled = true
		m.emission = color
		m.emission_energy_multiplier = glow
	if color.a < 1.0:
		m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	palette[key] = m
	return m

static func mesh(parent: Node3D, shape: Mesh, pos: Vector3, color: Color, glow: float = 0.0) -> MeshInstance3D:
	var n := MeshInstance3D.new()
	n.mesh = shape
	n.material_override = mat(color, glow)
	n.position = pos
	parent.add_child(n)
	return n

static func box(parent: Node3D, pos: Vector3, size: Vector3, color: Color) -> MeshInstance3D:
	var b := BoxMesh.new()
	b.size = size
	return mesh(parent, b, pos, color)

static func ball(parent: Node3D, pos: Vector3, size: Vector3, color: Color, glow: float = 0.0) -> MeshInstance3D:
	var b := SphereMesh.new()
	b.radial_segments = 10
	b.rings = 5
	b.radius = 1.0
	b.height = 2.0
	var n := mesh(parent, b, pos, color, glow)
	n.scale = size
	return n

static func cylinder(parent: Node3D, pos: Vector3, bottom: float, top: float, height: float, color: Color) -> MeshInstance3D:
	var b := CylinderMesh.new()
	b.bottom_radius = bottom
	b.top_radius = top
	b.height = height
	b.radial_segments = 12
	return mesh(parent, b, pos, color)

static func ring(parent: Node3D, pos: Vector3, radius: float, width: float, color: Color) -> MeshInstance3D:
	var b := TorusMesh.new()
	b.inner_radius = radius - width
	b.outer_radius = radius + width
	b.rings = 32
	b.ring_segments = 6
	return mesh(parent, b, pos, color)

static func branch(parent: Node3D, a: Vector3, b: Vector3, width: float, color: Color) -> MeshInstance3D:
	var n := cylinder(parent, (a+b)*0.5, width, width*0.75, a.distance_to(b), color)
	n.quaternion = Quaternion(Vector3.UP, (b-a).normalized())
	return n

static func blossom(parent: Node3D, pos: Vector3, size: float, color: Color = IVORY) -> void:
	for i in range(5):
		var a := i * TAU / 5.0
		var petal := ball(parent, pos + Vector3(cos(a), 0, sin(a))*.16*size, Vector3(.22,.07,.11)*size, color)
		petal.rotation.y = -a
	ball(parent, pos+Vector3(0,.06,0)*size, Vector3.ONE*.09*size, GOLD)

static func player() -> Node3D:
	var root := Node3D.new()
	root.name = "PetitJasmin"
	cylinder(root, Vector3(0,.43,0), .28, .43, .63, CLAY)
	ring(root, Vector3(0,.74,0), .405, .06, Color("e18c5b"))
	cylinder(root, Vector3(0,.74,0), .36,.36,.018, Color("433b31"))
	for side in [-1,1]:
		var foot := ball(root, Vector3(side*.18,.08,0), Vector3(.13,.09,.22), Color("6d5540"))
		foot.name = "FootL" if side == -1 else "FootR"
		ball(root, Vector3(side*.16,.49,.366), Vector3(.055,.078,.035), Color("283b39"))
		ball(root, Vector3(side*.16-.015,.515,.398), Vector3(.017,.023,.009), IVORY)
	branch(root, Vector3(-.055,.34,.396), Vector3(.055,.34,.396), .014, Color("573a2c"))
	var stem := Node3D.new()
	stem.name = "Stem"
	stem.position.y = .73
	root.add_child(stem)
	branch(stem, Vector3.ZERO, Vector3(.045,.79,0), .035, JADE)
	for i in range(3):
		var side := -1 if i%2 == 0 else 1
		var leaf := ball(stem,Vector3(side*.2,.24+i*.17,0),Vector3(.27,.065,.12), JADE if i%2 else Color("80a45c"))
		leaf.rotation.z = side*.32
	blossom(stem,Vector3(.04,.82,0),1.1)
	blossom(stem,Vector3(-.25,.6,0),.65)
	branch(root,Vector3(.32,.6,0),Vector3(.61,.48,.04),.038,JADE)
	cylinder(root,Vector3(.62,.49,.04),.13,.12,.3,Color("786244"))
	ball(root,Vector3(.62,.5,.155),Vector3(.095,.1,.045),NIGHT,0.6)
	ring(root,Vector3(.62,.72,.04),.075,.02,Color("786244"))
	return root

static func beetle() -> Node3D:
	var root := Node3D.new()
	ball(root,Vector3(0,.32,0),Vector3(.42,.28,.55),Color("293d48"))
	for side in [-1,1]:
		ball(root,Vector3(side*.19,.48,-.02),Vector3(.23,.18,.44),Color("597080"))
		ball(root,Vector3(side*.17,.42,.48),Vector3(.072,.066,.054),Color("f5a56a"),.6)
		branch(root,Vector3(side*.14,.5,.35),Vector3(side*.28,.8,.65),.025,Color("a19674"))
		for i in range(3):
			var leg := branch(root,Vector3(side*.3,.28,(i-1)*.29),Vector3(side*.66,.065,(i-1)*.32+.12),.035,Color("344954"))
			leg.name = "Leg%s%s" % [side,i]
	return root

static func tree(parent: Node3D, p: Vector2, variation: float) -> Node3D:
	var root := Node3D.new()
	parent.add_child(root)
	root.position = Vector3(p.x,0,p.y)
	cylinder(root,Vector3(0,1.1,0),.16,.105,2.2,Color("8e7753"))
	for i in range(5):
		var a := i*TAU/5.0+variation
		branch(root,Vector3(0,1.6,0),Vector3(cos(a)*.7,2.25,sin(a)*.7),.07,Color("8e7753"))
		ball(root,Vector3(cos(a)*.7,2.65,sin(a)*.7),Vector3(.98,.5,.96),Color("749471") if i%2 else Color("51785d"))
	ball(root,Vector3(0,2.93,0),Vector3(.96,.53,.99),Color("90a579"))
	for child in root.get_children():
		if child is MeshInstance3D and child.mesh is SphereMesh:
			child.set_meta("canopy",true)
			child.material_override = child.material_override.duplicate()
			child.material_override.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	# Visible circular shelter is also the exact gameplay shelter radius.
	cylinder(parent,Vector3(p.x,.012,p.y),1.85,1.85,.016,Color("607967"))
	ring(parent,Vector3(p.x,.035,p.y),1.85,.024,Color("a2b78b"))
	return root

static func gate(parent: Node3D, p: Vector2) -> Node3D:
	var root := Node3D.new()
	parent.add_child(root)
	root.position = Vector3(p.x,0,p.y)
	for side in [-1,1]:
		cylinder(root,Vector3(side*.86,1.08,0),.19,.19,2.16,Color("d5c9a5"))
		box(root,Vector3(side*.86,.12,0),Vector3(.54,.24,.54),Color("b5a47f"))
		ball(root,Vector3(side*.86,2.32,0),Vector3.ONE*.23,GOLD)
	box(root,Vector3(0,2.1,0),Vector3(2,.24,.32),Color("d5c9a5"))
	for i in range(9):
		var a := i*PI/8.0
		ball(root,Vector3(cos(a)*.9,2.13+sin(a)*.57,0),Vector3(.27,.15,.14),JADE)
	ring(root,Vector3(0,.05,0),.65,.04,NIGHT)
	return root

static func flower(parent: Node3D, p: Vector2, day: bool) -> Node3D:
	var root := Node3D.new()
	parent.add_child(root)
	root.position = Vector3(p.x,0,p.y)
	cylinder(root,Vector3(0,.5,0),.045,.03,1.0,JADE)
	var head := Node3D.new()
	head.name = "Head"
	head.position.y = 1.1
	root.add_child(head)
	var color := GOLD if day else NIGHT
	for i in range(9):
		var a := i*TAU/9.0
		var petal := ball(head,Vector3(cos(a)*.29,sin(a)*.29,0),Vector3(.23,.1,.065),color)
		petal.rotation.z = a
	ball(head,Vector3(0,0,.035),Vector3(.21,.21,.085),Color("685441") if day else Color("437a88"))
	for side in [-1,1]:
		ball(root,Vector3(side*.2,.45,0),Vector3(.25,.07,.1),JADE).rotation.z=side*.4
	ring(root,Vector3(0,.035,0),.55,.035,color)
	return root
