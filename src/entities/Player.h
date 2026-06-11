#pragma once
#include "../world/Platform.h"
#include "Entity.h"
#include <vector>

class Player : public Entity {
public:
  Vector2 velocity{0, 0};
  bool isGrounded = false;
  bool facingRight = true;
  bool isMoving = false;

  // Health
  float hp = 5.0f;
  float maxHp = 5.0f;
  bool isDead = false;
  float invulnTimer = 0.0f; // post-hit invulnerability (flicker)
  float burnFlash = 0.0f;   // 0..1, set by Game while exposed to the sun

  // Events for Game to react to (sounds/particles), valid for one frame
  bool jumpedThisFrame = false;
  bool bouncedThisFrame = false;
  bool landedThisFrame = false;

  Texture2D deathSprite{};
  float controlLock = 0.0f; // knockback: briefly ignore input

  Player();
  void Reset(Vector2 spawn);
  void TakeDamage(float amount);
  void Die();
  void Knockback(float fromX);

  void Update(float delta, const std::vector<Platform> &platforms,
              bool isDayTime);
  void Draw() override;

private:
  float speed;
  float jumpForce;
  float coyoteTimer = 0.0f;
  float jumpBufferTimer = 0.0f;
  bool jumpCutDone = true;
};
