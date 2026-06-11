#include "Spider.h"
#include <cmath>

void Spider::Update(float dt, const Level &level) {
  AdvanceAnimation(dt);

  t += dt;
  float mid = (minBound + maxBound) * 0.5f;
  float amp = (maxBound - minBound) * 0.5f;
  position.y = mid + amp * sinf(t * 1.7f);
}

void Spider::Draw() {
  float cx = position.x + width * 0.5f;
  DrawLineEx({cx, silkTop}, {cx, position.y + 3.0f}, 1.5f,
             Fade(RAYWHITE, 0.55f * drawAlpha));
  DrawSprite(false, Fade(WHITE, drawAlpha));
}
