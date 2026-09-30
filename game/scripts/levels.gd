class_name GardenLevels
extends RefCounted
## Authored, deterministic layouts. Coordinates are world X/Z. Boundaries: ±11, ±7.
static func all() -> Array:
	return [
		{
			"name": "THE SLEEPING GARDEN", "subtitle": "A little shade goes a long way.", "day": true,
			"hint": "Stay beneath the trees. Use your night lantern to carry shade into the sun.",
			"spawn": Vector2(-8,4), "exit": Vector2(8,-4),
			"trees": [Vector2(-8.6,3.4),Vector2(-4,0),Vector2(1,3),Vector2(5,-2),Vector2(8.5,-5.3)],
			"water": [Vector2(-5,3),Vector2(-3,-2),Vector2(1,2),Vector2(5,-1)],
			"walls": [Rect2(-1,-5,1.2,4),Rect2(3,3.8,4,0.8)], "ponds": [Rect2(-8,-5,4,2.2)],
			"enemies": [], "flowers": [], "barriers": []
		},
		{
			"name": "THINGS IN THE DARK", "subtitle": "Not everything likes the light.", "day": false,
			"hint": "Press Q for DAY. Aim at dusk beetles to petrify them; keep moving when they wake.",
			"spawn": Vector2(-8,4), "exit": Vector2(8,-4),
			"trees": [Vector2(-8.6,3.4),Vector2(-5,-3),Vector2(5,3),Vector2(8.5,-5.3)],
			"water": [Vector2(-6,-2),Vector2(-1,3),Vector2(3,-3),Vector2(7,1)],
			"walls": [Rect2(-3,-1,1,4),Rect2(1,-5,1,4)], "ponds": [],
			"enemies": [Vector2(-2,-3),Vector2(3,2),Vector2(7,-1)], "flowers": [], "barriers": []
		},
		{
			"name": "THE SUNFLOWER LOCK", "subtitle": "Borrow a little daylight.", "day": false,
			"hint": "Focus DAY on both sunflowers until they bloom. Their light opens the garden gate.",
			"spawn": Vector2(-8,4), "exit": Vector2(8,-3),
			"trees": [Vector2(-8.6,3.4),Vector2(-6,-3),Vector2(7,3)],
			"water": [Vector2(-6,-3),Vector2(-2,3),Vector2(5,3),Vector2(8,-2)],
			"walls": [Rect2(1,-7,0.8,5.2),Rect2(1,1.8,0.8,5.2)], "ponds": [],
			"enemies": [Vector2(-1,-3),Vector2(5,0)],
			"flowers": [{"p":Vector2(-4,-2),"day":true},{"p":Vector2(-3,3),"day":true}],
			"barriers": [Rect2(1,-1.8,0.8,3.6)]
		},
		{
			"name": "THE NOONDAY CROSSING", "subtitle": "Bring your own eclipse.", "day": true,
			"hint": "NIGHT shelters you, DAY blooms the locks. Recharge under a tree before crossing.",
			"spawn": Vector2(-8,4), "exit": Vector2(8,-4),
			"trees": [Vector2(-8.6,3.4),Vector2(-4,-3),Vector2(3,3),Vector2(8.5,-5.3)],
			"water": [Vector2(-6,-4),Vector2(-1,-1),Vector2(3,4),Vector2(7,-3)],
			"walls": [Rect2(1,-7,0.8,5.3),Rect2(1,1.7,0.8,5.3)],
			"ponds": [Rect2(-8,-1,3,2)], "enemies": [Vector2(-2,2),Vector2(5,-2)],
			"flowers": [{"p":Vector2(-3,-3),"day":true},{"p":Vector2(-2,3),"day":false}],
			"barriers": [Rect2(1,-1.7,0.8,3.4)]
		},
		{
			"name": "THE LUNAR CONSERVATORY", "subtitle": "Two kinds of light. One way home.", "day": false,
			"hint": "Gold flowers need DAY; blue flowers need NIGHT. SPACE dashes past waking beetles.",
			"spawn": Vector2(-8,4), "exit": Vector2(8,-4),
			"trees": [Vector2(-8.6,3.4),Vector2(-6,-4),Vector2(0,0),Vector2(7,3)],
			"water": [Vector2(-6,-4),Vector2(-1,4),Vector2(4,-4),Vector2(8,2),Vector2(7,-3)],
			"walls": [Rect2(2,-7,0.8,5.4),Rect2(2,1.6,0.8,5.4),Rect2(-5,-1,3,.7)],
			"ponds": [], "enemies": [Vector2(-3,-3),Vector2(-2,2),Vector2(6,0)],
			"flowers": [{"p":Vector2(-5,-4),"day":true},{"p":Vector2(-1,3),"day":false},{"p":Vector2(0,-3),"day":true}],
			"barriers": [Rect2(2,-1.6,0.8,3.2)]
		},
		{
			"name": "WHERE JASMINE BLOOMS", "subtitle": "One last garden. Make it yours.", "day": true,
			"hint": "Balance shelter and daylight. Bring every dew drop home, and let the garden bloom.",
			"spawn": Vector2(-8,4), "exit": Vector2(8,-4),
			"trees": [Vector2(-8.6,3.4),Vector2(-5,-3),Vector2(0,3),Vector2(4,-3),Vector2(8.5,-5.3)],
			"water": [Vector2(-7,-4),Vector2(-3,3),Vector2(0,-3),Vector2(4,3),Vector2(7,0),Vector2(8,-3)],
			"walls": [Rect2(2,-7,.8,5.3),Rect2(2,1.7,.8,5.3)], "ponds": [Rect2(-7,-.5,3,1.5)],
			"enemies": [Vector2(-4,1),Vector2(-1,-3),Vector2(5,2),Vector2(7,-2)],
			"flowers": [{"p":Vector2(-5,-4),"day":true},{"p":Vector2(-2,3),"day":false},{"p":Vector2(0,-2),"day":true}],
			"barriers": [Rect2(2,-1.7,.8,3.4)]
		}
	]
