#pragma once
#include "Enemy.h"

// Patrols horizontally between minBound and maxBound (set from EnemyConfig).
class Roach : public Enemy {
public:
  explicit Roach(Vector2 pos) : Enemy(pos) {}
  void Update(float dt, const Level &level) override;
  void Draw() override;
};
