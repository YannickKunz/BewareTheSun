#pragma once
#include "raylib.h"

// All in-level art is generated procedurally at startup: cohesive pixel-art
// sprites (point-filtered) and per-biome painted backgrounds. No image files
// are required for level elements.
struct ProcArt {
  // Player (Petit Jasmin: a jasmine flower in a terracotta pot)
  Texture2D playerIdle{};
  Texture2D playerWalk{}; // 4-frame sheet
  Texture2D playerDeath{};
  int playerWalkFrames = 4;

  // Enemies
  Texture2D spider{}; // 2-frame sheet
  int spiderFrames = 2;
  Texture2D roach{}; // 2-frame sheet
  int roachFrames = 2;

  // Platforms & props
  Texture2D flower{}; // 6-frame swaying flower platform sheet
  int flowerFrames = 6;
  Texture2D mushroomDay{}, mushroomNight{};
  Texture2D tileDayTop{}, tileDayFill{};
  Texture2D tileNightTop{}, tileNightFill{};
  Texture2D canDay{}, canNight{}; // watering can (exit)
  Texture2D droplet{};

  // UI / sky
  Texture2D heart{};
  Texture2D sunGlow{};
  Texture2D moon{};

  static constexpr int BIOME_COUNT = 7;
  Texture2D bgDay[BIOME_COUNT]{};
  Texture2D bgNight[BIOME_COUNT]{};

  void Load();
  void Unload();
};
