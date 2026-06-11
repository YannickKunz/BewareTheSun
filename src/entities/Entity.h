#pragma once
#include "raylib.h"

class Entity {
public:
  Vector2 position{0, 0};
  float width = 32;
  float height = 32;

  // Sprite / spritesheet animation
  Texture2D sprite{};
  Texture2D spritesheet{};
  int frameCount = 1;
  int currentFrame = 0;
  float frameTimer = 0.0f;
  float frameSpeed = 0.1f; // seconds per frame
  bool animated = false;

  virtual ~Entity() = default;

  void AdvanceAnimation(float dt) {
    if (!animated || frameCount <= 1)
      return;
    frameTimer += dt;
    while (frameTimer >= frameSpeed) {
      frameTimer -= frameSpeed;
      currentFrame = (currentFrame + 1) % frameCount;
    }
  }

  // Draws the current frame (or static sprite), optionally x-flipped.
  void DrawSprite(bool flipX = false, Color tint = WHITE) const {
    Rectangle dest = {position.x, position.y, width, height};
    if (animated && spritesheet.id != 0) {
      float frameW = (float)spritesheet.width / (float)frameCount;
      Rectangle src = {frameW * currentFrame, 0, frameW,
                       (float)spritesheet.height};
      if (flipX)
        src.width = -src.width;
      DrawTexturePro(spritesheet, src, dest, {0, 0}, 0.0f, tint);
    } else if (sprite.id != 0) {
      Rectangle src = {0, 0, (float)sprite.width, (float)sprite.height};
      if (flipX)
        src.width = -src.width;
      DrawTexturePro(sprite, src, dest, {0, 0}, 0.0f, tint);
    } else {
      DrawRectangleV(position, {width, height}, MAGENTA); // missing art
    }
  }

  virtual void Draw() { DrawSprite(); }

  Rectangle GetRect() const { return {position.x, position.y, width, height}; }
  Vector2 Center() const {
    return {position.x + width * 0.5f, position.y + height * 0.5f};
  }
};
