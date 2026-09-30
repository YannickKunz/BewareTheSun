extends Node3D
## Simulation owns state; Art owns meshes; HUD reads state and sends commands.
const Rules = preload("res://scripts/rules.gd")
const Levels = preload("res://scripts/levels.gd")
const Art = preload("res://scripts/art.gd")
const Hud = preload("res://scripts/hud.gd")
var levels: Array = Levels.all()
var level_index := 0
var level: Dictionary
var mode := "title"
var world: Node3D
var camera: Camera3D
var sun: DirectionalLight3D
var environment: WorldEnvironment
var hud: Control
var player: Node3D
var player_pos := Vector2.ZERO
var facing := Vector2(1,-.3).normalized()
var movement := Vector2.ZERO
var sky_day := true
var lamp_day := false
var lamp_on := true
var battery := 100.0
var health := 100.0
var collected := 0
var clock := 0.0
var run_time := 0.0
var level_time := 0.0
var dash_time := 0.0
var dash_cooldown := 0.0
var dash_direction := Vector2.RIGHT
var invulnerability := 0.0
var lamp_cooldown := 0.0
var ground_material: ShaderMaterial
var beam_edge: MeshInstance3D
var beam_material: StandardMaterial3D
var lamp_light: OmniLight3D
var shelters: Array = []
var drops: Array = []
var enemies: Array = []
var flowers: Array = []
var barriers: Array = []
var tree_nodes: Array = []
var particles: Array = []
var exit_node: Node3D
var unlocked := false
var gate_open := false
var message := ""
var message_time := 0.0
var best_unlocked := 0
var reduced_motion := false
var muted := false
var saved_level := 0
var sfx: Dictionary = {}
var music: AudioStreamPlayer
var screenshot_mode := ""
var test_mode := false
var frames := 0
var rng := RandomNumberGenerator.new()
var gamepad_aim := false

func _ready() -> void:
	test_mode = "--test-mode" in OS.get_cmdline_user_args()
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--capture="):
			screenshot_mode = arg.trim_prefix("--capture=")
	_configure_inputs()
	_load_settings()
	_setup_camera()
	_setup_audio()
	load_level(0)
	hud = Hud.new()
	hud.game = self
	var canvas := CanvasLayer.new()
	add_child(canvas)
	canvas.add_child(hud)
	if screenshot_mode != "":
		if screenshot_mode == "night":
			load_level(2)
			lamp_day = true
		elif screenshot_mode != "title":
			load_level(0)
		mode = "title" if screenshot_mode == "title" else "playing"
	if not test_mode:
		DisplayServer.window_set_title("Beware the Sun — The Little Eclipse")

func _configure_inputs() -> void:
	var keys := {"left":[KEY_A,KEY_LEFT],"right":[KEY_D,KEY_RIGHT],"up":[KEY_W,KEY_UP],"down":[KEY_S,KEY_DOWN],"polarity":[KEY_Q,KEY_T],"lantern":[KEY_F],"dash":[KEY_SPACE],"pause":[KEY_ESCAPE],"restart":[KEY_R]}
	for action in keys:
		if not InputMap.has_action(action):
			InputMap.add_action(action)
		for code in keys[action]:
			var event := InputEventKey.new()
			event.physical_keycode = code
			InputMap.action_add_event(action,event)
	for pair in [["dash",JOY_BUTTON_A],["polarity",JOY_BUTTON_X],["lantern",JOY_BUTTON_Y],["pause",JOY_BUTTON_START]]:
		var event := InputEventJoypadButton.new()
		event.button_index = pair[1]
		InputMap.action_add_event(pair[0],event)

func _setup_camera() -> void:
	environment = WorldEnvironment.new()
	environment.environment = Environment.new()
	var env := environment.environment
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color("243c3b")
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color("c7dac6")
	env.ambient_light_energy = .65
	env.tonemap_mode = Environment.TONE_MAPPER_LINEAR
	add_child(environment)
	sun = DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-62,-32,0)
	sun.light_color = Color("ffe0a1")
	sun.light_energy = 1.6
	sun.shadow_enabled = true
	sun.directional_shadow_max_distance = 60
	add_child(sun)
	camera = Camera3D.new()
	camera.projection = Camera3D.PROJECTION_ORTHOGONAL
	camera.size = 20.5
	camera.position = Vector3(0,25,23)
	add_child(camera)
	camera.look_at(Vector3.ZERO)
	camera.current = true

func load_level(index: int) -> void:
	level_index = clampi(index,0,levels.size()-1)
	level = levels[level_index]
	if is_instance_valid(world):
		remove_child(world)
		world.queue_free()
	world = Node3D.new()
	world.name = "Garden"
	add_child(world)
	for list in [shelters,drops,enemies,flowers,barriers,tree_nodes,particles]:
		list.clear()
	rng.seed = 4300 + level_index
	player_pos = level.spawn
	facing = Vector2(1,-.3).normalized()
	sky_day = level.day
	lamp_day = false
	lamp_on = true
	battery = 100
	health = 100
	collected = 0
	level_time = 0
	dash_time = 0
	dash_cooldown = 0
	invulnerability = 0
	lamp_cooldown = 0
	unlocked = false
	gate_open = level.flowers.is_empty()
	_build_world()
	message = level.hint
	message_time = 11
	_apply_lighting()

func _build_world() -> void:
	# All physical scenery is imported from the Blender-authored asset library.
	var base := Art.model("garden_base", world)
	var ground := base.find_child("GroundSurface", true, false) as MeshInstance3D
	ground_material = ShaderMaterial.new()
	ground_material.shader = preload("res://shaders/ground.gdshader")
	ground_material.set_shader_parameter("sand", Color("978b70") if sky_day else Color("607a74"))
	ground.material_override = ground_material
	for x in range(-11, 12):
		for z in [-7.5, 7.5]:
			Art.model("border_stone", world, Vector3(x, 0, z))
	for z in range(-7, 8):
		for x in [-11.5, 11.5]:
			Art.model("border_stone", world, Vector3(x, 0, z)).rotation.y = PI / 2
	for i in range(24):
		var t := i / 23.0
		var p := Vector2(-8, 4).lerp(Vector2(8, -4), t)
		p.y += sin(t * TAU) * 1.7
		var tile := Art.model("path_tile", world, Vector3(p.x, .008, p.y))
		tile.rotation.y = rng.randf_range(-.25, .25)
		tile.scale = Vector3.ONE * rng.randf_range(.8, 1.0)
	for rect: Rect2 in level.ponds:
		var c := rect.get_center()
		var pond := Art.model("pond", world, Vector3(c.x, 0, c.y))
		pond.scale = Vector3(rect.size.x, 1, rect.size.y)
		for i in range(4):
			var p := c + Vector2(rng.randf_range(-rect.size.x*.33, rect.size.x*.33), rng.randf_range(-rect.size.y*.28, rect.size.y*.28))
			Art.model("lily_pad", world, Vector3(p.x, .09, p.y)).rotation.y = rng.randf()*TAU
	for rect: Rect2 in level.walls:
		_build_wall(rect)
	for p: Vector2 in level.trees:
		tree_nodes.append(Art.tree(world, p, rng.randf()*TAU))
		shelters.append({"p":p, "r":1.85})
	for rect: Rect2 in level.barriers:
		var c := rect.get_center()
		var node := Art.model("vine_gate", world, Vector3(c.x, 0, c.y))
		node.scale.z = rect.size.y / 3.6
		barriers.append({"rect":rect, "node":node})
	for p: Vector2 in level.water:
		var node := Art.model("dew_drop", world, Vector3(p.x, .58, p.y))
		Art.marker(world, p, .33, Art.NIGHT)
		drops.append({"p":p, "node":node, "taken":false, "phase":rng.randf()*TAU})
	for spec in level.flowers:
		var node := Art.flower(world, spec.p, spec.day)
		flowers.append({"p":spec.p, "day":spec.day, "node":node, "charge":0.0})
	for p: Vector2 in level.enemies:
		var node := Art.beetle()
		world.add_child(node)
		node.position = Vector3(p.x, 0, p.y)
		enemies.append({"p":p, "home":p, "node":node, "stun":0.0, "phase":rng.randf()*TAU, "awake":false})
	exit_node = Art.gate(world, level.exit)
	player = Art.player()
	world.add_child(player)
	player.position = Vector3(player_pos.x, 0, player_pos.y)
	lamp_light = OmniLight3D.new()
	lamp_light.omni_range = 3.2
	lamp_light.light_energy = .4
	lamp_light.position.y = 1.1
	player.add_child(lamp_light)
	_build_beam_edge()
	# Plant the border without obstructing navigable routes or objectives.
	for i in range(140):
		var p := Vector2(rng.randf_range(-11, 11), rng.randf_range(-7, 7))
		if absf(p.x) < 9.8 and absf(p.y) < 5.7:
			continue
		if _blocked(p, .5) or p.distance_to(level.exit) < 1.4:
			continue
		var asset := "flower_patch" if i%3 == 0 else "grass_clump"
		if i%9 == 0:
			asset = "mushroom_patch"
		Art.model(asset, world, Vector3(p.x, 0, p.y)).rotation.y = rng.randf()*TAU
	for i in range(20):
		var p := Vector2(rng.randf_range(-10.9, 10.9), [-6.7, 6.7][i%2])
		Art.model("rock_cluster", world, Vector3(p.x, 0, p.y)).rotation.y = rng.randf()*TAU
	_update_lamp_visuals()

func _build_wall(rect: Rect2) -> void:
	var c := rect.get_center()
	var along_x := rect.size.x > rect.size.y
	var length := maxf(rect.size.x, rect.size.y)
	var depth := minf(rect.size.x, rect.size.y)
	var count := maxi(1, roundi(length))
	for i in range(count):
		var p := Vector3(c.x, 0, c.y)
		if along_x:
			p.x = rect.position.x + (i + .5) * length / count
		else:
			p.z = rect.position.y + (i + .5) * length / count
		var node := Art.model("wall_segment", world, p)
		node.scale = Vector3(length / count, 1, depth / .8)
		if not along_x:
			node.rotation.y = PI / 2

func _build_beam_edge() -> void:
	beam_edge = MeshInstance3D.new()
	world.add_child(beam_edge)
	var mesh := ImmediateMesh.new()
	beam_material = StandardMaterial3D.new()
	beam_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	beam_material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	beam_material.albedo_color = Color(.5,.9,1,.5)
	mesh.surface_begin(Mesh.PRIMITIVE_LINES,beam_material)
	for i in range(32):
		var a := -Rules.BEAM_HALF_ANGLE + (i/32.0)*Rules.BEAM_HALF_ANGLE*2
		var b := -Rules.BEAM_HALF_ANGLE + ((i+1)/32.0)*Rules.BEAM_HALF_ANGLE*2
		mesh.surface_add_vertex(Vector3(cos(a),0,sin(a))*Rules.BEAM_RANGE)
		mesh.surface_add_vertex(Vector3(cos(b),0,sin(b))*Rules.BEAM_RANGE)
	for a in [-Rules.BEAM_HALF_ANGLE,Rules.BEAM_HALF_ANGLE]:
		mesh.surface_add_vertex(Vector3(cos(a),0,sin(a))*Rules.HALO_RADIUS)
		mesh.surface_add_vertex(Vector3(cos(a),0,sin(a))*Rules.BEAM_RANGE)
	mesh.surface_end()
	beam_edge.mesh = mesh

func _apply_lighting() -> void:
	var env := environment.environment
	sun.light_color = Color("fff1d8") if sky_day else Color("86b5d6")
	sun.light_energy = .55 if sky_day else .45
	env.ambient_light_color = Color("c1d4c2") if sky_day else Color("819fae")
	env.ambient_light_energy = .28 if sky_day else .36
	env.background_color = Color("3c5750") if sky_day else Color("1e333d")

func _input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		gamepad_aim = false
	if event is InputEventJoypadMotion and event.axis in [JOY_AXIS_RIGHT_X,JOY_AXIS_RIGHT_Y] and absf(event.axis_value)>.2:
		gamepad_aim = true
	if event is InputEventKey and event.pressed and not event.echo:
		if event.keycode == KEY_M:
			command("mute")
		if event.keycode == KEY_F11:
			command("fullscreen")
		if event.keycode == KEY_ENTER:
			if mode in ["title","dead","complete","won"]:
				command("primary")
	if event.is_action_pressed("pause"):
		if mode == "playing":
			mode = "paused"
		elif mode == "paused":
			mode = "playing"
		return
	if mode != "playing":
		return
	if event.is_action_pressed("polarity") or (event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_RIGHT):
		toggle_polarity()
	if event.is_action_pressed("lantern"):
		lamp_on = not lamp_on and battery > 3
		play_sound("switch")
	if event.is_action_pressed("dash"):
		start_dash()
	if event.is_action_pressed("restart"):
		load_level(level_index)

func toggle_polarity() -> void:
	if lamp_cooldown > 0:
		return
	lamp_day = not lamp_day
	lamp_cooldown = .18
	play_sound("switch")

func start_dash() -> void:
	if dash_cooldown <= 0:
		dash_direction = movement.normalized() if movement.length() > .1 else facing
		dash_time = .19
		dash_cooldown = 1.45
		invulnerability = maxf(invulnerability,.24)
		play_sound("dash")
		emit_particles(player_pos,Art.IVORY,9)

func _physics_process(delta: float) -> void:
	camera.h_offset = -6.0 if mode == "title" else 0.0
	frames += 1
	if mode not in ["paused","dead","complete","won"]:
		clock += delta
		_animate_world(delta)
	if mode == "playing":
		level_time += delta
		run_time += delta
		message_time = maxf(0,message_time-delta)
		invulnerability = maxf(0,invulnerability-delta)
		lamp_cooldown = maxf(0,lamp_cooldown-delta)
		dash_cooldown = maxf(0,dash_cooldown-delta)
		movement = Input.get_vector("left","right","up","down")
		var pads := Input.get_connected_joypads()
		if not pads.is_empty():
			var stick := Vector2(Input.get_joy_axis(pads[0],JOY_AXIS_LEFT_X),Input.get_joy_axis(pads[0],JOY_AXIS_LEFT_Y))
			if stick.length()>.2:
				movement = stick.limit_length()
		_update_aim()
		var speed := 4.15
		var direction := movement
		if dash_time > 0:
			dash_time = maxf(0,dash_time-delta)
			speed = 11.5
			direction = dash_direction
		player_pos = move_actor(player_pos,direction*speed*delta,Rules.PLAYER_RADIUS)
		_update_resources(delta)
		_update_objectives(delta)
		if mode == "playing":
			_update_enemies(delta)
			if health <= 0:
				mode = "dead"
				play_sound("hurt")
		_update_player(delta)
	_update_lamp_visuals()
	if is_instance_valid(hud):
		hud.queue_redraw()
	# Deterministic local captures; normal play never exits automatically.
	if screenshot_mode != "" and frames == 90:
		_capture.call_deferred()

func _update_aim() -> void:
	if screenshot_mode != "" or test_mode:
		return
	if gamepad_aim:
		var pads := Input.get_connected_joypads()
		if not pads.is_empty():
			var stick := Vector2(Input.get_joy_axis(pads[0],JOY_AXIS_RIGHT_X),Input.get_joy_axis(pads[0],JOY_AXIS_RIGHT_Y))
			if stick.length()>.2:
				facing = stick.normalized()
	else:
		var mouse := get_viewport().get_mouse_position()
		var origin := camera.project_ray_origin(mouse)
		var ray := camera.project_ray_normal(mouse)
		var point = Plane(Vector3.UP,0).intersects_ray(origin,ray)
		if point != null:
			var aim := Vector2(point.x,point.z)-player_pos
			if aim.length()>.3:
				facing = aim.normalized()

func move_actor(pos: Vector2, offset: Vector2, radius: float) -> Vector2:
	# Substeps keep dash and low-framerate movement from tunnelling through thin walls.
	var steps := maxi(1,int(ceil(offset.length()/.15)))
	var step := offset/steps
	for i in range(steps):
		var next := pos + Vector2(step.x,0)
		if not _blocked(next,radius):
			pos = next
		next = pos + Vector2(0,step.y)
		if not _blocked(next,radius):
			pos = next
	return pos

func _blocked(pos: Vector2, radius: float = .34) -> bool:
	if absf(pos.x)>11.15-radius or absf(pos.y)>7.15-radius:
		return true
	for rect: Rect2 in level.walls:
		if Rules.rect_blocked(pos,rect,radius):
			return true
	for rect: Rect2 in level.ponds:
		if Rules.rect_blocked(pos,rect,radius):
			return true
	if not gate_open:
		for rect: Rect2 in level.barriers:
			if Rules.rect_blocked(pos,rect,radius):
				return true
	return false

func local_day(p: Vector2) -> bool:
	return Rules.local_day(p,player_pos,facing,sky_day,lamp_on,lamp_day)

func is_safe() -> bool:
	return not sky_day or not local_day(player_pos) or Rules.sheltered(player_pos,shelters)

func _update_resources(delta: float) -> void:
	if lamp_on:
		battery = maxf(0,battery-delta*5.4)
		if battery <= 0:
			lamp_on = false
			announce("Lantern empty. Rest in shade; press F when it recharges.")
			play_sound("hurt")
	else:
		battery = minf(100,battery+delta*(24 if Rules.sheltered(player_pos,shelters) else 13))
	health = Rules.exposure(health,sky_day, is_safe(),delta)

func _update_objectives(delta: float) -> void:
	for drop in drops:
		if not drop.taken and player_pos.distance_to(drop.p)<.65:
			drop.taken = true
			drop.node.visible = false
			collected += 1
			health = minf(100,health+18)
			battery = minf(100,battery+24)
			play_sound("drop")
			emit_particles(drop.p,Art.NIGHT,12)
	var charged := true
	for flower in flowers:
		var active: bool = lamp_on and lamp_day == flower.day and Rules.in_beam(flower.p,player_pos,facing)
		var previous: float = flower.charge
		flower.charge = Rules.charge_cell(flower.charge,active,delta)
		if flower.charge >= 1 and previous < 1:
			play_sound("bloom")
			emit_particles(flower.p,Art.GOLD if flower.day else Art.NIGHT,18)
		if flower.charge < 1:
			charged = false
		var head: Node3D = flower.node.get_meta("head")
		head.rotation.x = lerpf(-.7,0,flower.charge)
		head.scale = Vector3.ONE*(.65+flower.charge*.4)
	if charged and not gate_open:
		gate_open = true
		announce("The living gate opens. Find the remaining dew and head for the arch.")
		play_sound("bloom")
	unlocked = collected == drops.size() and gate_open
	if player_pos.distance_to(level.exit)<.9:
		if unlocked:
			complete_level()
		elif message_time<1:
			announce("The arch needs every dew drop and every flower in bloom.")

func _update_enemies(delta: float) -> void:
	for enemy in enemies:
		var in_day := local_day(enemy.p)
		if in_day:
			enemy.stun = 1.2
		else:
			enemy.stun = maxf(0,enemy.stun-delta)
		enemy.awake = not in_day and enemy.stun <= 0
		var node: Node3D = enemy.node
		if enemy.awake:
			var dir: Vector2 = (player_pos-enemy.p).normalized()
			var target: Vector2 = player_pos
			if enemy.p.distance_to(player_pos)>6:
				target = enemy.home + Vector2(sin(clock*.5+enemy.phase),cos(clock*.4+enemy.phase))*1.3
				dir = (target-enemy.p).normalized()
			var speed := 1.55+level_index*.085
			var next := move_actor(enemy.p,dir*speed*delta,.31)
			if next.distance_to(enemy.p)<.003:
				next = move_actor(enemy.p,dir.orthogonal()*speed*delta,.31)
			enemy.p = next
			node.rotation.y = lerp_angle(node.rotation.y,atan2(dir.x,dir.y),delta*8)
			if enemy.p.distance_to(player_pos)<.7 and invulnerability<=0:
				health = maxf(0,health-26)
				invulnerability = 1.25
				player_pos = move_actor(player_pos,-dir*.65,Rules.PLAYER_RADIUS)
				play_sound("hurt")
				emit_particles(player_pos,Art.CLAY,10)
				notice_if_needed()
		node.position = Vector3(enemy.p.x, 0, enemy.p.y)
		Art.animate(node, "scuttle" if enemy.awake else "sleep", delta, not reduced_motion)

func notice_if_needed() -> void:
	if message_time<1:
		announce("A dusk beetle! Use DAY to still it, or SPACE to dash away.")

func _update_player(delta: float) -> void:
	player.position = Vector3(player_pos.x,0,player_pos.y)
	player.rotation.y = lerp_angle(player.rotation.y,atan2(facing.x,facing.y),1-exp(-14*delta))
	var moving := movement.length()>.1 or dash_time>0
	Art.animate(player, "walk" if moving else "idle", delta, not reduced_motion, 1.7 if dash_time > 0 else 1.0)
	player.visible = invulnerability<.15 or int(invulnerability*12)%2 == 0

func _animate_world(delta: float) -> void:
	if mode == "title":
		Art.animate(player, "idle", delta, not reduced_motion)
	for drop in drops:
		if not drop.taken:
			Art.animate(drop.node, "float", delta, not reduced_motion)
	for tree: Node3D in tree_nodes:
		Art.animate(tree, "sway", delta, not reduced_motion)
		var near_player := player_pos.distance_to(Vector2(tree.position.x, tree.position.z)) < 2.3
		for material: StandardMaterial3D in tree.get_meta("canopy_materials"):
			material.albedo_color.a = move_toward(material.albedo_color.a, .18 if near_player else 1.0, delta*3)
	for barrier in barriers:
		if gate_open:
			Art.animate(barrier.node, "open", delta, not reduced_motion)

	if is_instance_valid(exit_node):
		exit_node.scale = Vector3.ONE*(1.0+(sin(clock*2)*.014 if unlocked and not reduced_motion else 0.0))
	for i in range(particles.size()-1,-1,-1):
		var p = particles[i]
		p.life -= delta
		p.node.position += p.velocity*delta
		p.velocity.y -= delta*2.5
		p.node.scale = Vector3.ONE * .09 * maxf(.01, p.life / p.max_life)
		if p.life<=0:
			p.node.queue_free()
			particles.remove_at(i)

func _update_lamp_visuals() -> void:
	ground_material.set_shader_parameter("lamp_position",player_pos)
	ground_material.set_shader_parameter("lamp_direction",facing)
	ground_material.set_shader_parameter("lamp_on",lamp_on)
	ground_material.set_shader_parameter("lamp_day",lamp_day)
	beam_edge.position = Vector3(player_pos.x,.083,player_pos.y)
	beam_edge.rotation.y = -facing.angle()
	beam_edge.visible = lamp_on
	beam_material.albedo_color = Color(1,.84,.45,.58) if lamp_day else Color(.56,.89,.94,.55)
	lamp_light.visible = lamp_on
	lamp_light.light_color = Art.GOLD if lamp_day else Art.NIGHT
	var lens: MeshInstance3D = player.get_meta("lens")
	lens.material_override = Art.mat((Art.GOLD if lamp_day else Art.NIGHT) if lamp_on else Color("445d59"), .25 if lamp_on else 0.0)

func emit_particles(p: Vector2, color: Color, count: int) -> void:
	if reduced_motion or test_mode:
		return
	for i in range(count):
		var node := Art.model("petal_particle", world, Vector3(p.x, .55, p.y))
		node.scale = Vector3.ONE * .09
		Art.tint(node, color, .08)
		var a := rng.randf()*TAU
		var life := rng.randf_range(.4,.8)
		particles.append({"node":node,"life":life,"max_life":life,"velocity":Vector3(cos(a)*1.2,rng.randf_range(1,2.5),sin(a)*1.2)})

func complete_level() -> void:
	if mode != "playing":
		return
	mode = "won" if level_index == levels.size()-1 else "complete"
	best_unlocked = maxi(best_unlocked,mini(level_index+1,levels.size()-1))
	saved_level = best_unlocked
	_save_settings()
	play_sound("bloom")

func announce(text: String) -> void:
	message = text
	message_time = 6

func command(action: String) -> void:
	match action:
		"primary":
			match mode:
				"title":
					run_time = 0
					load_level(saved_level)
					mode = "playing"
				"paused": mode = "playing"
				"dead":
					load_level(level_index)
					mode = "playing"
				"complete":
					load_level(level_index+1)
					mode = "playing"
				"won":
					load_level(0)
					mode = "title"
		"new":
			load_level(0)
			run_time = 0
			mode = "playing"
		"menu":
			load_level(0)
			mode = "title"
		"retry":
			load_level(level_index)
			mode = "playing"
		"mute":
			muted = not muted
			AudioServer.set_bus_mute(0,muted)
			_save_settings()
		"motion":
			reduced_motion = not reduced_motion
			_save_settings()
		"fullscreen":
			var is_full := DisplayServer.window_get_mode() == DisplayServer.WINDOW_MODE_FULLSCREEN
			DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED if is_full else DisplayServer.WINDOW_MODE_FULLSCREEN)
		"quit": get_tree().quit()

func _load_settings() -> void:
	if test_mode or screenshot_mode != "":
		return
	var config := ConfigFile.new()
	if config.load("user://garden.cfg") == OK:
		best_unlocked = clampi(int(config.get_value("progress","garden",0)),0,levels.size()-1)
		saved_level = best_unlocked
		muted = bool(config.get_value("settings","muted",false))
		reduced_motion = bool(config.get_value("settings","reduced_motion",false))
	AudioServer.set_bus_mute(0,muted)

func _save_settings() -> void:
	if test_mode or screenshot_mode != "":
		return
	var config := ConfigFile.new()
	config.set_value("progress","garden",best_unlocked)
	config.set_value("settings","muted",muted)
	config.set_value("settings","reduced_motion",reduced_motion)
	config.save("user://garden.cfg")

func _setup_audio() -> void:
	for sound in ["drop","switch","dash","hurt","bloom"]:
		var path := "res://assets/audio/%s.wav" % sound
		if ResourceLoader.exists(path):
			sfx[sound] = load(path)
	music = AudioStreamPlayer.new()
	add_child(music)
	if ResourceLoader.exists("res://assets/audio/garden.wav") and not test_mode and screenshot_mode == "":
		music.stream = load("res://assets/audio/garden.wav")
		music.volume_db = -16
		music.finished.connect(music.play)
		music.play()

func play_sound(sound: String) -> void:
	if test_mode or not sfx.has(sound):
		return
	var audio := AudioStreamPlayer.new()
	add_child(audio)
	audio.stream = sfx[sound]
	audio.volume_db = -6
	audio.finished.connect(audio.queue_free)
	audio.play()

func _capture() -> void:
	await RenderingServer.frame_post_draw
	DirAccess.make_dir_recursive_absolute("res://test-output")
	get_viewport().get_texture().get_image().save_png("res://test-output/%s.png" % screenshot_mode)
	print("CAPTURE: ",screenshot_mode)
	get_tree().quit()

func _exit_tree() -> void:
	if is_instance_valid(music):
		music.stop()
		music.stream = null
	sfx.clear()
	Art.scenes.clear()
	Art.palette.clear()
