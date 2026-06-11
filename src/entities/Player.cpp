#include "Player.h"
#include "../core/Constants.h"
#include "raylib.h"
#include <cmath>

Player::Player() {
  speed = Core::PLAYER_SPEED;
  jumpForce = Core::JUMP_FORCE;
  maxHp = Core::MAX_HP;
  hp = maxHp;
}

void Player::Reset(Vector2 spawn) {
  position = spawn;
  velocity = {0, 0};
  hp = maxHp;
  isDead = false;
  isGrounded = false;
  invulnTimer = 0.0f;
  burnFlash = 0.0f;
  controlLock = 0.0f;
  coyoteTimer = 0.0f;
  jumpBufferTimer = 0.0f;
  jumpCutDone = true;
  speed = Core::PLAYER_SPEED;
  jumpForce = Core::JUMP_FORCE;
}

void Player::TakeDamage(float amount) {
  if (isDead)
    return;
  hp -= amount;
  if (hp <= 0) {
    hp = 0;
    Die();
  }
}

void Player::Die() { isDead = true; }

void Player::Knockback(float fromX) {
  float dir = (Center().x < fromX) ? -1.0f : 1.0f;
  velocity.x = dir * speed * 0.9f;
  velocity.y = -jumpForce * 0.45f;
  controlLock = 0.22f;
  isGrounded = false;
}

void Player::Draw() {
  // Flicker while invulnerable
  if (invulnTimer > 0.0f && fmodf(invulnTimer, 0.2f) < 0.08f && !isDead)
    return;

  if (isDead && deathSprite.id != 0) {
    Rectangle src = {0, 0, (float)deathSprite.width,
                     (float)deathSprite.height};
    if (!facingRight)
      src.width = -src.width;
    Rectangle dest = {position.x, position.y, width, height};
    DrawTexturePro(deathSprite, src, dest, {0, 0}, 0.0f, WHITE);
    return;
  }

  // Reddish tint while burning in the sun
  Color tint = WHITE;
  if (burnFlash > 0.0f) {
    float t = burnFlash * (0.6f + 0.4f * sinf((float)GetTime() * 9.0f));
    tint = {255, (unsigned char)(255 - 110 * t), (unsigned char)(255 - 140 * t),
            255};
  }

  bool walking = isMoving && isGrounded;
  if (walking && animated && spritesheet.id != 0) {
    DrawSprite(!facingRight, tint);
  } else {
    Rectangle src = {0, 0, (float)sprite.width, (float)sprite.height};
    if (!facingRight)
      src.width = -src.width;
    Rectangle dest = {position.x, position.y, width, height};
    if (sprite.id != 0)
      DrawTexturePro(sprite, src, dest, {0, 0}, 0.0f, tint);
    else
      DrawRectangleV(position, {width, height}, RED);
  }
}

void Player::Update(float delta, const std::vector<Platform> &platforms,
                    bool isDayTime) {
  jumpedThisFrame = false;
  bouncedThisFrame = false;
  landedThisFrame = false;

  if (invulnTimer > 0.0f)
    invulnTimer -= delta;
  if (controlLock > 0.0f)
    controlLock -= delta;

  // --- INPUT ---
  float targetVx = 0.0f;
  isMoving = false;
  if (!isDead && controlLock <= 0.0f) {
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
      targetVx = -speed;
      facingRight = false;
      isMoving = true;
    } else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
      targetVx = speed;
      facingRight = true;
      isMoving = true;
    }
    // Smooth acceleration (exponential approach)
    float accel = isGrounded ? Core::GROUND_ACCEL : Core::AIR_ACCEL;
    velocity.x += (targetVx - velocity.x) * fminf(1.0f, accel * delta);
    if (fabsf(velocity.x) < 4.0f && targetVx == 0.0f)
      velocity.x = 0.0f;
  }

  // --- JUMP: buffer + coyote time + variable height ---
  bool jumpPressed = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) ||
                     IsKeyPressed(KEY_W);
  bool jumpHeld =
      IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);

  if (jumpPressed && !isDead)
    jumpBufferTimer = Core::JUMP_BUFFER;
  else if (jumpBufferTimer > 0.0f)
    jumpBufferTimer -= delta;

  coyoteTimer = isGrounded ? Core::COYOTE_TIME : coyoteTimer - delta;

  if (jumpBufferTimer > 0.0f && coyoteTimer > 0.0f && !isDead) {
    velocity.y = -jumpForce;
    isGrounded = false;
    coyoteTimer = 0.0f;
    jumpBufferTimer = 0.0f;
    jumpCutDone = false;
    jumpedThisFrame = true;
  }
  // Releasing jump while rising shortens the jump
  if (!jumpCutDone && !jumpHeld && velocity.y < 0.0f) {
    velocity.y *= Core::JUMP_CUT;
    jumpCutDone = true;
  }

  // Walk animation
  if (isMoving && isGrounded)
    AdvanceAnimation(delta);
  else {
    currentFrame = 0;
    frameTimer = 0.0f;
  }

  // --- PHYSICS: axis-separated movement & collision ---
  bool wasGrounded = isGrounded;

  // 1. Horizontal pass
  position.x += velocity.x * delta;
  Rectangle rect = GetRect();
  for (const auto &plat : platforms) {
    if (!plat.IsSolid(isDayTime))
      continue;
    if (CheckCollisionRecs(rect, plat.rect)) {
      if (velocity.x > 0)
        position.x = plat.rect.x - rect.width;
      else if (velocity.x < 0)
        position.x = plat.rect.x + plat.rect.width;
      velocity.x = 0;
      rect = GetRect();
    }
  }

  // 2. Vertical pass
  velocity.y += Core::GRAVITY * delta;
  if (velocity.y > Core::MAX_FALL_SPEED)
    velocity.y = Core::MAX_FALL_SPEED;
  position.y += velocity.y * delta;

  rect = GetRect();
  isGrounded = false;
  for (const auto &plat : platforms) {
    if (!plat.IsSolid(isDayTime))
      continue;
    if (CheckCollisionRecs(rect, plat.rect)) {
      if (velocity.y > 0) {
        position.y = plat.rect.y - rect.height;
        velocity.y = 0;
        isGrounded = true;
        if (plat.type == PlatformType::MUSHROOM && !isDayTime && !isDead) {
          velocity.y = -jumpForce * Core::MUSHROOM_BOUNCE;
          isGrounded = false;
          jumpCutDone = true; // bounce height is fixed
          bouncedThisFrame = true;
        }
      } else if (velocity.y < 0) {
        position.y = plat.rect.y + plat.rect.height;
        velocity.y = 0;
      }
      rect = GetRect();
    }
  }

  if (!wasGrounded && isGrounded)
    landedThisFrame = true;
}
