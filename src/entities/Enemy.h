#pragma once
#include "../core/Constants.h"
#include "../world/Level.h"
#include "Entity.h"

class Enemy : public Entity {
public:
  bool alive = true;
  bool movingRight = true;
  float speed = Core::ENEMY_SPEED;
  float minBound = 0.0f;
  float maxBound = 0.0f;
  float drawAlpha = 1.0f; // enemies fade in with the night

  explicit Enemy(Vector2 pos) { position = pos; }

  virtual void Update(float dt, const Level &level) { AdvanceAnimation(dt); }
};
