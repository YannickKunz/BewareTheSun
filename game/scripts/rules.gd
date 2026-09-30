class_name GardenRules
extends RefCounted
## Shared by simulation and tests. A lantern changes only its footprint, never the sky.
const BEAM_RANGE := 6.8
const BEAM_HALF_ANGLE := 0.52
const HALO_RADIUS := 1.25
const PLAYER_RADIUS := 0.34

static func in_beam(point: Vector2, origin: Vector2, direction: Vector2) -> bool:
	var offset := point - origin
	if offset.length() <= HALO_RADIUS:
		return true
	return offset.length() <= BEAM_RANGE and offset.normalized().dot(direction.normalized()) >= cos(BEAM_HALF_ANGLE)

static func local_day(point: Vector2, origin: Vector2, direction: Vector2, sky_day: bool, lamp_on: bool, lamp_day: bool) -> bool:
	return lamp_day if lamp_on and in_beam(point, origin, direction) else sky_day

static func sheltered(point: Vector2, shelters: Array) -> bool:
	for shelter in shelters:
		if point.distance_to(shelter.p) <= shelter.r:
			return true
	return false

static func rect_blocked(point: Vector2, obstacle: Rect2, radius: float = PLAYER_RADIUS) -> bool:
	return obstacle.grow(radius).has_point(point)

static func charge_cell(value: float, active: bool, delta: float) -> float:
	# Fully charged flowers latch. Partial progress decays if the wrong polarity is used.
	if value >= 1.0:
		return 1.0
	return clampf(value + delta * (0.48 if active else -0.18), 0.0, 1.0)

static func exposure(health: float, is_day: bool, shade: bool, delta: float) -> float:
	return clampf(health + delta * (-15.0 if is_day and not shade else 8.0), 0.0, 100.0)
