#pragma once
#include "Enemy.h"

// Bobs vertically on a silk thread between minBound and maxBound.
class Spider : public Enemy {
public:
  float silkTop = 0.0f; // y of the thread's anchor point
  float t = 0.0f;       // animation clock (randomized per spider)

  explicit Spider(Vector2 pos) : Enemy(pos) {
    t = pos.x * 0.013f; // pseudo-random phase so spiders desync
  }

  void Update(float dt, const Level &level) override;
  void Draw() override;
};
