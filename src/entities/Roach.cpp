#include "Roach.h"

void Roach::Update(float dt, const Level &level) {
  AdvanceAnimation(dt);

  position.x += speed * dt * (movingRight ? 1.0f : -1.0f);
  if (position.x + width >= maxBound) {
    position.x = maxBound - width;
    movingRight = false;
  } else if (position.x <= minBound) {
    position.x = minBound;
    movingRight = true;
  }
}

void Roach::Draw() {
  // Sprite faces right by default
  DrawSprite(!movingRight, Fade(WHITE, drawAlpha));
}
