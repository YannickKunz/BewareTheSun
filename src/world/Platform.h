#pragma once
#include "../core/Types.h"
#include "raylib.h"

struct Platform {
  Rectangle rect;
  PlatformType type;
  float initialY; // FLOWER platforms animate between day/night positions

  Platform(Rectangle r, PlatformType t = PlatformType::NORMAL)
      : rect(r), type(t), initialY(r.y) {}

  bool IsSolid(bool isDayTime) const {
    switch (type) {
    case PlatformType::NORMAL:
    case PlatformType::INVISIBLE:
      return true;
    case PlatformType::FLOWER:
      return isDayTime;
    case PlatformType::MUSHROOM:
      return !isDayTime;
    }
    return false;
  }
};
