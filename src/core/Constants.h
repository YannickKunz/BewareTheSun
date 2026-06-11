#pragma once

namespace Core {
// Screen dimensions (mutable for resolution changes)
inline int SCREEN_WIDTH = 1280;
inline int SCREEN_HEIGHT = 800;

// Physics constants (recalculated from SCREEN_HEIGHT so the game feels
// identical at every resolution)
inline float GRAVITY = SCREEN_HEIGHT * 1.25f;
inline float JUMP_FORCE = SCREEN_HEIGHT * 0.78f;
inline float PLAYER_SPEED = SCREEN_HEIGHT * 0.45f;
inline float ENEMY_SPEED = SCREEN_HEIGHT * 0.16f;
inline float MAX_FALL_SPEED = SCREEN_HEIGHT * 1.4f;

inline void RecalculatePhysics() {
  GRAVITY = SCREEN_HEIGHT * 1.25f;
  JUMP_FORCE = SCREEN_HEIGHT * 0.78f;
  PLAYER_SPEED = SCREEN_HEIGHT * 0.45f;
  ENEMY_SPEED = SCREEN_HEIGHT * 0.16f;
  MAX_FALL_SPEED = SCREEN_HEIGHT * 1.4f;
}

// Game-feel tuning
inline constexpr float COYOTE_TIME = 0.12f;   // grace period after leaving a ledge
inline constexpr float JUMP_BUFFER = 0.14f;   // press jump slightly before landing
inline constexpr float JUMP_CUT = 0.45f;      // release jump early -> shorter jump
inline constexpr float GROUND_ACCEL = 10.0f;  // horizontal smoothing factor
inline constexpr float AIR_ACCEL = 6.5f;

inline constexpr float MUSHROOM_BOUNCE = 1.35f; // multiple of jump force
inline constexpr float STOMP_BOUNCE = 0.75f;

inline constexpr float MAX_HP = 5.0f;
inline constexpr float SUN_DPS = 1.0f;       // hp lost per second in direct sun
inline constexpr float SHADE_REGEN = 0.40f;  // hp regained per second in shade
inline constexpr float INVULN_TIME = 1.5f;   // after touching an enemy

inline constexpr int LEVEL_COUNT = 7;
} // namespace Core
