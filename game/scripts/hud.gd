extends Control
## Resolution-independent drawn interface with native keyboard-focusable buttons.
var game: Node
var font: Font = preload("res://assets/fonts/DMSans.ttf")
var serif: Font = preload("res://assets/fonts/Fraunces.ttf")
var small_caps: FontVariation
var buttons: Array[Button] = []
var last_mode := ""
const PAPER := Color("f3ebd2")
const MUTED := Color("a7bab0")
const INK := Color("17332f")
const GOLD := Color("eac075")
const BLUE := Color("93d6dc")
var scale_ui := Vector2.ONE

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	small_caps = FontVariation.new()
	small_caps.base_font = font
	small_caps.variation_embolden = .7

func _process(_delta: float) -> void:
	if game.mode != last_mode:
		last_mode = game.mode
		_build_buttons()
	_position_buttons()

func label_at(text: String, pos: Vector2, size: int = 18, color: Color = PAPER, heading: bool = false) -> void:
	draw_string(serif if heading else font,pos,text,HORIZONTAL_ALIGNMENT_LEFT,-1,size,color)

func centered(text: String, pos: Vector2, size: int = 18, color: Color = PAPER, heading: bool = false) -> void:
	var f := serif if heading else font
	var width := f.get_string_size(text,HORIZONTAL_ALIGNMENT_LEFT,-1,size).x
	draw_string(f,pos-Vector2(width*.5,0),text,HORIZONTAL_ALIGNMENT_LEFT,-1,size,color)

func panel(rect: Rect2, color: Color, radius: int = 12, border: Color = Color.TRANSPARENT) -> void:
	var style := StyleBoxFlat.new()
	style.bg_color = color
	style.set_corner_radius_all(radius)
	if border.a > 0:
		style.border_color = border
		style.set_border_width_all(1)
	draw_style_box(style,rect)

func sun_icon(pos: Vector2, radius: float, color: Color, moon: bool = false) -> void:
	if moon:
		draw_arc(pos,radius,.7,5.6,32,color,2,true)
		draw_arc(pos+Vector2(radius*.4,-radius*.1),radius*.82,1.1,4.8,24,color,2,true)
	else:
		draw_circle(pos,radius*.5,color,false,2,true)
		for i in range(10):
			var a := i*TAU/10.0
			var dir := Vector2(cos(a),sin(a))
			draw_line(pos+dir*radius*.76,pos+dir*radius,color,1.6,true)

func keycap(key: String, pos: Vector2, width: float = 29.0) -> void:
	panel(Rect2(pos,Vector2(width,26)),Color("29473f"),5,Color("53695a"))
	centered(key,pos+Vector2(width*.5,18),12,PAPER)

func meter(pos: Vector2, width: float, value: float, color: Color) -> void:
	panel(Rect2(pos,Vector2(width,5)),Color("3d5147"),2)
	if value>0:
		panel(Rect2(pos,Vector2(width*clampf(value,0,1),5)),color,2)

func _draw() -> void:
	if game == null:
		return
	scale_ui = get_viewport_rect().size/Vector2(1440,900)
	draw_set_transform(Vector2.ZERO,0,scale_ui)
	if game.mode == "title":
		_draw_title()
		return
	_draw_gameplay()
	if game.mode in ["paused","dead","complete","won"]:
		_draw_modal()

func _draw_title() -> void:
	# Deliberate split composition: typography left, living diorama right.
	draw_rect(Rect2(0,0,552,900),Color("17332f"))
	for i in range(80):
		draw_rect(Rect2(552+i*2,0,2,900),Color(0.09,.2,.18,(1-i/80.0)*.94))
	label_at("A  LITTLE  GARDEN  ADVENTURE",Vector2(58,81),13,GOLD)
	sun_icon(Vector2(107,164),37,GOLD)
	label_at("Beware",Vector2(54,286),88,PAPER,true)
	label_at("the Sun",Vector2(54,378),88,PAPER,true)
	draw_line(Vector2(60,417),Vector2(117,417),GOLD,2)
	label_at("THE LITTLE ECLIPSE",Vector2(133,422),14,GOLD)
	label_at("A little plant. A borrowed moon.",Vector2(60,481),22,PAPER,true)
	label_at("Turn your lantern into day or night.",Vector2(60,522),17,MUTED)
	label_at("Outwit the sun. Wake a sleeping garden.",Vector2(60,549),17,MUTED)
	label_at("SIX GARDENS  /  TWO KINDS OF LIGHT",Vector2(60,760),12,MUTED)
	label_at("Made of clay, moonlight & a little courage.",Vector2(60,850),14,MUTED)
	# A field-note plaque over the right-hand miniature.
	panel(Rect2(962,740,407,96),Color(.09,.19,.17,.94),9,Color("67786a"))
	sun_icon(Vector2(1004,786),20,BLUE,true)
	label_at("YOUR POCKET-SIZED ECLIPSE",Vector2(1040,776),12,GOLD)
	label_at("Carry the night into the daylight.",Vector2(1040,805),17,PAPER,true)
	label_at("01 — THE SLEEPING GARDEN",Vector2(895,83),14,PAPER)
	draw_line(Vector2(895,98),Vector2(1370,98),Color(.8,.85,.73,.28),1)

func _draw_gameplay() -> void:
	# Minimal letterbox-like framing leaves the center entirely to the garden.
	panel(Rect2(28,25,1384,91),Color(.065,.15,.13,.94),12,Color(.6,.7,.56,.23))
	sun_icon(Vector2(67,69),21,GOLD,not game.sky_day)
	label_at("GARDEN  %02d / 06" % (game.level_index+1),Vector2(103,56),12,GOLD)
	label_at(game.level.name,Vector2(103,87),24,PAPER,true)
	var flowers_done := 0
	for flower in game.flowers:
		if flower.charge>=1:
			flowers_done += 1
	label_at("DEW",Vector2(645,55),11,MUTED)
	label_at("%02d / %02d" % [game.collected,game.drops.size()],Vector2(645,87),24,BLUE)
	label_at("BLOOMS",Vector2(785,55),11,MUTED)
	label_at("%02d / %02d" % [flowers_done,game.flowers.size()],Vector2(785,87),24,GOLD)
	label_at("VITALITY",Vector2(962,55),11,MUTED)
	label_at("%d" % ceili(game.health),Vector2(1079,55),11,PAPER)
	meter(Vector2(963, 76.0),151,game.health/100,Color("a7c693") if game.health>30 else Color("e58f69"))
	label_at("PAUSE",Vector2(1272,73),12,MUTED)
	keycap("ESC",Vector2(1333,54),49)
	# In-world objective labels and charge arcs.
	for flower in game.flowers:
		var screen: Vector2 = game.camera.unproject_position(Vector3(flower.p.x,1.9,flower.p.y))/scale_ui
		var color := GOLD if flower.day else BLUE
		panel(Rect2(screen-Vector2(32,13),Vector2(64,25)),Color(.075,.17,.15,.92),12)
		centered("DAY" if flower.day else "NIGHT",screen+Vector2(0,5),11,color)
		draw_arc(screen+Vector2(0,28),9,-PI/2,-PI/2+maxf(.01,flower.charge)*TAU,28,color,2.5,true)
		if flower.charge>=1:
			centered("+",screen+Vector2(0,33),14,color)
	var exit_screen: Vector2 = game.camera.unproject_position(Vector3(game.level.exit.x,3.2,game.level.exit.y))/scale_ui
	panel(Rect2(exit_screen-Vector2(45,12),Vector2(90,26)),Color(.08,.18,.16,.9),12)
	centered("HOME" if game.unlocked else "LOCKED",exit_screen+Vector2(0,5),11,BLUE if game.unlocked else MUTED)
	for enemy in game.enemies:
		if not enemy.awake:
			var p: Vector2 = game.camera.unproject_position(enemy.node.position+Vector3(0,1.0,0))/scale_ui
			centered("z z",p,13,MUTED)
	# A focused, always-visible lantern instrument.
	panel(Rect2(28,687,268,118),Color(.065,.15,.13,.96),12,Color(.5,.65,.55,.3))
	var color := GOLD if game.lamp_day else BLUE
	sun_icon(Vector2(64,727),19,color,not game.lamp_day)
	label_at("DAY LANTERN" if game.lamp_day else "NIGHT LANTERN",Vector2(99,721),14,color)
	label_at("ON" if game.lamp_on else "RECHARGING",Vector2(99,743),11,MUTED)
	label_at("%d%%" % ceili(game.battery),Vector2(238,743),11,PAPER)
	meter(Vector2(48,765),228,game.battery/100,color)
	var state := "IN SHADE" if game.is_safe() else "SUN EXPOSURE"
	panel(Rect2(1148,750,264,55),Color(.065,.15,.13,.96),12)
	draw_circle(Vector2(1174,778),4,Color("b5cf8f") if game.is_safe() else Color("ef956e"))
	label_at(state,Vector2(1191,783),12,PAPER)
	# Bottom control strip, no overlapping floating hints.
	panel(Rect2(28,830,1384,47),Color(.065,.15,.13,.94),9)
	var controls := [["WASD","move",44,57],["MOUSE","aim",208,60],["Q","day / night",383,29],["F","lantern",587,29],["SPACE","dash",765, 60.0],["R","retry",951,29],["M","sound",1092,29]]
	for c in controls:
		keycap(c[0],Vector2(c[2],840),c[3])
		label_at(c[1],Vector2(c[2]+c[3]+10,858),13,MUTED)
	if game.dash_cooldown>0:
		meter(Vector2(852,867),62,1-game.dash_cooldown/1.45,BLUE)
	if game.message_time>0:
		var text: String = game.message
		var width := minf(980,font.get_string_size(text,HORIZONTAL_ALIGNMENT_LEFT,-1,16).x+46)
		panel(Rect2(720-width/2,132,width,42),Color(.075,.17,.15,.94),20,Color(.6,.7,.56,.22))
		centered(text,Vector2(720,159),16,PAPER)

func _draw_modal() -> void:
	draw_rect(Rect2(0,0,1440,900),Color(.04,.1,.09,.69))
	panel(Rect2(437,206,566,498),Color("17332f"),18,Color("788771"))
	var title := "A moment of shade."
	var eyebrow := "TAKE YOUR TIME"
	var line1 := "Your garden will be right here."
	var line2 := "Rest, then carry a little light onward."
	if game.mode == "dead":
		title = "A little too much sun."
		eyebrow = "EVERY GARDENER STARTS AGAIN"
		line1 = "Use NIGHT for shelter, DAY for the beetles."
		line2 = "Turn the lantern off in shade to recharge."
	elif game.mode == "complete":
		title = "A garden, awakened."
		eyebrow = "GARDEN %02d COMPLETE" % (game.level_index+1)
		line1 = "Every drop found. Every flower in bloom."
		line2 = "The next patch of earth is waiting for you."
	elif game.mode == "won":
		title = "Home, at last."
		eyebrow = "SIX GARDENS. ONE BRAVE LITTLE PLANT."
		line1 = "You carried the night. You brought back the day."
		line2 = "And somewhere in between, the jasmine bloomed."
	sun_icon(Vector2(720,270),29,GOLD,game.mode=="paused")
	centered(eyebrow,Vector2(720,326),12,GOLD)
	centered(title,Vector2(720,385),37,PAPER,true)
	centered(line1,Vector2(720,428),16,MUTED)
	centered(line2,Vector2(720,455),16,MUTED)
	if game.mode == "complete":
		centered("%d:%02d  /  %d DEW DROPS" % [int(game.level_time)/60,int(game.level_time)%60,game.collected],Vector2(720,493),12,GOLD)

func _build_buttons() -> void:
	for button in buttons:
		remove_child(button)
		button.queue_free()
	buttons.clear()
	match game.mode:
		"title":
			_button("Continue garden %02d   →" % (game.saved_level+1) if game.saved_level>0 else "Enter the garden   →","primary",Rect2(60,602,364,61),true)
			_button("Start a new journey","new",Rect2(60,676,220,40),false)
		"paused":
			_button("Back to the garden   →","primary",Rect2(512,497,416,48),true)
			_button("Sound: OFF" if game.muted else "Sound: ON","mute",Rect2(512,559,198,38),false)
			_button("Motion: reduced" if game.reduced_motion else "Motion: full","motion",Rect2(727,559,201,38),false)
			_button("Main menu","menu",Rect2(512,615,195,40),false)
			_button("Quit game","quit",Rect2(730,615,198,40),false)
		"dead":
			_button("Try this garden again   →","primary",Rect2(512,510,416,52),true)
			_button("Main menu","menu",Rect2(612,590,216,40),false)
		"complete":
			_button("On to the next garden   →","primary",Rect2(512,525,416,52),true)
			_button("Stay a little longer / replay","retry",Rect2(537,603,366,40),false)
		"won":
			_button("Back to the garden gate   →","primary",Rect2(512,525,416,52),true)
	if not buttons.is_empty():
		buttons[0].grab_focus()

func _button(text: String, action: String, rect: Rect2, primary: bool) -> void:
	var button := Button.new()
	button.text = text
	button.set_meta("rect",rect)
	button.add_theme_font_override("font",font)
	button.add_theme_font_size_override("font_size",18 if primary else 15)
	button.add_theme_color_override("font_color",INK if primary else PAPER)
	button.add_theme_color_override("font_hover_color",INK if primary else GOLD)
	button.add_theme_color_override("font_pressed_color",INK if primary else GOLD)
	button.mouse_default_cursor_shape = Control.CURSOR_POINTING_HAND
	for state in ["normal","hover","pressed","focus"]:
		var style := StyleBoxFlat.new()
		style.set_corner_radius_all(8)
		style.bg_color = GOLD if primary else Color("234139")
		if state == "hover":
			style.bg_color = Color("f4d59d") if primary else Color("35554a")
		if state == "pressed":
			style.bg_color = Color("d5a85d") if primary else Color("28493e")
		if state == "focus":
			style.bg_color = Color.TRANSPARENT
			style.border_color = PAPER
			style.set_border_width_all(2)
		button.add_theme_stylebox_override(state,style)
	button.pressed.connect(func():
		game.command(action)
		last_mode = "" # settings labels refresh without changing modes
	)
	add_child(button)
	buttons.append(button)

func _position_buttons() -> void:
	var factor := get_viewport_rect().size/Vector2(1440,900)
	for button in buttons:
		var rect: Rect2 = button.get_meta("rect")
		button.position = rect.position*factor
		button.size = rect.size*factor
		button.add_theme_font_size_override("font_size",int((18 if rect.size.y>45 else 15)*factor.y))
