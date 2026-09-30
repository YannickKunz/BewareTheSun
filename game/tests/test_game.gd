extends SceneTree
const Rules = preload("res://scripts/rules.gd")
const Game = preload("res://scripts/game.gd")
var assertions := 0
var failures: Array[String] = []
var game: Node

func _initialize() -> void:
	_run.call_deferred()

func check(condition: bool, description: String) -> void:
	assertions += 1
	if not condition:
		failures.append(description)
		push_error("FAIL: "+description)

func _run() -> void:
	check(Rules.in_beam(Vector2(6,0),Vector2.ZERO,Vector2.RIGHT),"Beam reaches forward")
	check(not Rules.in_beam(Vector2(7,0),Vector2.ZERO,Vector2.RIGHT),"Beam has finite range")
	check(not Rules.in_beam(Vector2(3,4),Vector2.ZERO,Vector2.RIGHT),"Beam excludes points outside cone")
	check(Rules.in_beam(Vector2(-1,0),Vector2.ZERO,Vector2.RIGHT),"Halo protects the lantern carrier")
	check(not Rules.in_beam(Vector2(-2,0),Vector2.ZERO,Vector2.RIGHT),"Night does not become a global toggle")
	check(not Rules.local_day(Vector2(2,0),Vector2.ZERO,Vector2.RIGHT,true,true,false),"Night cone overrides day locally")
	check(Rules.local_day(Vector2(2,0),Vector2.ZERO,Vector2.RIGHT,false,true,true),"Day cone overrides night locally")
	check(Rules.local_day(Vector2(-4,0),Vector2.ZERO,Vector2.RIGHT,true,true,false),"Sky remains day beyond cone")
	check(Rules.local_day(Vector2(2,0),Vector2.ZERO,Vector2.RIGHT,true,false,false),"Disabled lamp cannot change day")
	check(is_equal_approx(Rules.exposure(80,true,false,1),65),"Sun damages exposed plant")
	check(is_equal_approx(Rules.exposure(80,true,true,1),88),"Shade restores vitality")
	check(Rules.exposure(99,false,false,1)==100,"Healing is clamped")
	check(Rules.charge_cell(1,false,99)==1,"Charged flower remains latched")
	check(Rules.charge_cell(.5,false,1)<.5,"Unfinished flower loses charge")
	check(Rules.charge_cell(.5,true,2)==1,"Correct lantern charges flower")
	game = Game.new()
	root.add_child(game)
	await process_frame
	game.test_mode = true
	game.set_physics_process(false)
	game.mode = "playing"
	# Palette and mesh creation run on all levels, even with the dummy renderer.
	for index in range(6):
		game.load_level(index)
		game.mode = "playing"
		check(not game._blocked(game.player_pos),"Garden %d spawn is clear" % index)
		check(not game._blocked(game.level.exit),"Garden %d exit is clear" % index)
		check(game.health==100 and game.battery==100 and game.collected==0,"Garden %d reset state" % index)
		var reachable := reachable_cells()
		for flower in game.flowers:
			check(can_reach(flower.p,reachable),"Garden %d flower reachable before opening gate" % index)
		# Charge through the actual objective update, with deliberately wrong polarity first.
		for flower in game.flowers:
			game.player_pos = flower.p
			game.lamp_on = true
			game.lamp_day = not flower.day
			var charge_before: float = flower.charge
			game._update_objectives(.1)
			check(flower.charge<=charge_before,"Garden %d wrong polarity does not charge" % index)
			game.lamp_day = flower.day
			for tick in range(135):
				game._update_objectives(1.0/60.0)
			check(flower.charge==1,"Garden %d flower latches" % index)
		game._update_objectives(.01)
		check(game.gate_open,"Garden %d gate opens after blooms" % index)
		reachable = reachable_cells()
		for drop in game.drops:
			check(can_reach(drop.p,reachable),"Garden %d dew reachable" % index)
		check(can_reach(game.level.exit,reachable),"Garden %d home reachable" % index)
		game.player_pos = game.level.exit
		game._update_objectives(.01)
		check(game.mode=="playing","Garden %d cannot finish without all dew" % index)
		for drop in game.drops:
			game.player_pos = drop.p
			game._update_objectives(.01)
		check(game.collected==game.drops.size(),"Garden %d collects every dew" % index)
		game._update_objectives(.01)
		check(game.collected==game.drops.size(),"Garden %d cannot double-collect" % index)
		game.player_pos = game.level.exit
		game._update_objectives(.01)
		check(game.mode==("won" if index==5 else "complete"),"Garden %d completes" % index)
		check(game.best_unlocked>=mini(index+1,5),"Garden %d progression unlocks" % index)
		await process_frame # flush old garden queue_free between layouts
	# Collision, hazards, energy, input and reset regression checks.
	game.load_level(2)
	game.mode = "playing"
	var blocked_move: Vector2 = game.move_actor(Vector2(0,0),Vector2(5,0),.34)
	check(blocked_move.x<.7,"Dash cannot tunnel through living gate")
	game.gate_open = true
	check(game.move_actor(Vector2(0,0),Vector2(5,0),.34).x>4.9,"Open gate allows passage")
	check(game.move_actor(Vector2(0,-3),Vector2(5,0),.34).x<.7,"Dash cannot tunnel through stone")
	game.load_level(0)
	game.player_pos = Vector2.ZERO
	game.health = 80
	game.lamp_on = false
	game._update_resources(1)
	check(game.health==65,"World sunlight causes actual damage")
	game.lamp_on = true
	game.lamp_day = false
	game._update_resources(1)
	check(game.health>65,"Night halo prevents sun damage")
	game.battery = .01
	game._update_resources(1)
	check(not game.lamp_on and game.battery==0,"Depleted lamp turns off")
	game._update_resources(1)
	check(game.battery>0 and not game.lamp_on,"Empty lamp recharges without flickering on")
	game.player_pos = game.level.spawn
	game.health = 70
	game._update_resources(1)
	check(game.health>70,"Tree shade actually heals")
	game.load_level(1)
	game.lamp_on = false
	var enemy = game.enemies[0]
	game.player_pos = enemy.p+Vector2(1,0)
	game._update_enemies(.016)
	check(enemy.awake,"Night wakes beetles")
	game.lamp_on = true
	game.lamp_day = true
	game.facing = Vector2.LEFT
	game._update_enemies(.016)
	check(not enemy.awake and enemy.stun>1,"Day lantern petrifies beetle")
	game.lamp_on = false
	game._update_enemies(.016)
	check(not enemy.awake,"Beetle has a wake-up grace period")
	game.player_pos = enemy.p
	game.health = 100
	game.invulnerability = 0
	game._update_enemies(1.3)
	check(game.health==74,"Awake beetle deals contact damage")
	game.player_pos = enemy.p
	game._update_enemies(.01)
	check(game.health==74,"Contact invulnerability prevents repeated instant damage")
	game.dash_cooldown = 0
	game.movement = Vector2.RIGHT
	game.start_dash()
	check(game.dash_time>0 and game.invulnerability>0,"Dash grants a short escape window")
	var cooldown: float = game.dash_cooldown
	game.start_dash()
	check(game.dash_cooldown==cooldown,"Dash cannot bypass cooldown")
	game.load_level(0)
	game.mode = "playing"
	game.command("menu")
	check(game.mode=="title","Menu command works")
	game.command("new")
	check(game.mode=="playing" and game.level_index==0,"New journey starts at garden one")
	var pause := InputEventKey.new()
	pause.physical_keycode = KEY_ESCAPE
	pause.pressed = true
	game._input(pause)
	check(game.mode=="paused","Escape pauses")
	var old_health: float = game.health
	game._physics_process(5)
	check(game.health==old_health,"Pause freezes simulation resources")
	game._input(pause)
	check(game.mode=="playing","Escape resumes")
	game.health = 0
	game.lamp_on = false
	game.player_pos = Vector2.ZERO
	game._physics_process(.016)
	check(game.mode=="dead","Zero vitality enters retry screen")
	game.command("primary")
	check(game.mode=="playing" and game.health==100,"Retry restores clean state")
	# Full resource-constrained routes: no teleporting, no invincibility, no free refills.
	for index in range(6):
		game.load_level(index)
		game.mode = "playing"
		for flower in game.flowers:
			check(walk_route(flower.p),"Garden %d route reaches flower" % index)
			game.lamp_on = true
			game.lamp_day = flower.day
			game.facing = Vector2.RIGHT
			for tick in range(135):
				simulate_tick(1.0/60.0)
		for drop in game.drops:
			if not drop.taken:
				check(walk_route(drop.p),"Garden %d route reaches dew" % index)
		check(walk_route(game.level.exit),"Garden %d route reaches home" % index)
		check(game.health>0,"Garden %d route survives hazards" % index)
		check(game.mode==("won" if index==5 else "complete"),"Garden %d full simulated route completes" % index)
		print("ROUTE %d: health %.1f, battery %.1f" % [index+1,game.health,game.battery])
		await process_frame
	game.queue_free()
	await process_frame
	print("\n%d assertions; %d failures." % [assertions,failures.size()])
	if failures.is_empty():
		print("PASS: geometry, local day/night, hazards, battery, six layouts, objective latches, progression, pause, death, retry.")
	quit(0 if failures.is_empty() else 1)

func reachable_cells() -> Dictionary:
	# Flood-fill at 0.25 m resolution with the actual player radius and live blockers.
	var start := Vector2i(roundi(game.level.spawn.x*4),roundi(game.level.spawn.y*4))
	var visited := {start:true}
	var queue: Array[Vector2i] = [start]
	var cursor := 0
	while cursor < queue.size():
		var cell := queue[cursor]
		cursor += 1
		for dir in [Vector2i.LEFT,Vector2i.RIGHT,Vector2i.UP,Vector2i.DOWN]:
			var next: Vector2i = cell+dir
			if visited.has(next) or game._blocked(Vector2(next)/4.0):
				continue
			visited[next] = true
			queue.append(next)
	return visited

func can_reach(p: Vector2, visited: Dictionary) -> bool:
	var cell := Vector2i(roundi(p.x*4),roundi(p.y*4))
	return visited.has(cell)

func simulate_tick(delta: float) -> void:
	game.clock += delta
	game.invulnerability = maxf(0,game.invulnerability-delta)
	game._update_resources(delta)
	game._update_objectives(delta)
	if game.mode == "playing":
		game._update_enemies(delta)
		if game.health<=0:
			game.mode = "dead"

func walk_route(target: Vector2) -> bool:
	var start := Vector2i(roundi(game.player_pos.x*4),roundi(game.player_pos.y*4))
	var goal := Vector2i(roundi(target.x*4),roundi(target.y*4))
	var previous := {start:start}
	var queue: Array[Vector2i] = [start]
	var cursor := 0
	while cursor < queue.size() and not previous.has(goal):
		var cell := queue[cursor]
		cursor += 1
		for dir in [Vector2i.LEFT,Vector2i.RIGHT,Vector2i.UP,Vector2i.DOWN]:
			var next: Vector2i = cell+dir
			if previous.has(next) or game._blocked(Vector2(next)/4.0):
				continue
			previous[next] = cell
			queue.append(next)
	if not previous.has(goal):
		return false
	var path: Array[Vector2] = []
	var step := goal
	while step != start:
		path.push_front(Vector2(step)/4.0)
		step = previous[step]
	game.lamp_day = not game.sky_day
	game.lamp_on = game.battery>0
	for point in path:
		if game.mode != "playing":
			break
		var difference: Vector2 = point-game.player_pos
		game.facing = difference.normalized()
		game.player_pos = game.move_actor(game.player_pos,difference,.34)
		simulate_tick(difference.length()/4.15)
	return game.player_pos.distance_to(target)<1.0
