#include "Level.h"

bool Level::IsEdge(Vector2 pos) const {
  // Check if there is ground strictly below 'pos'
  Vector2 checkPoint = {pos.x, pos.y + 1.0f};

  for (const auto &plat : platforms) {
    if (plat.type == PlatformType::INVISIBLE)
      continue; // border walls don't count as ground
    if (CheckCollisionPointRec(checkPoint, plat.rect))
      return false;
  }
  return true;
}
