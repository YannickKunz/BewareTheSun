#pragma once
#include "../core/Types.h"
#include "Platform.h"
#include "raylib.h"
#include <vector>

// Data-driven enemy placement.
// ROACH:  minBound/maxBound = horizontal patrol range (world x)
// SPIDER: minBound/maxBound = vertical bobbing range on its silk thread
struct EnemyConfig {
  EnemyType type;
  Vector2 position;
  float minBound;
  float maxBound;
};

struct Droplet {
  Vector2 pos;
  bool collected;
  float phase; // bobbing animation offset
};

// Pure level data: geometry + spawns. Textures and music are owned by Game.
class Level {
public:
  std::vector<Platform> platforms;
  std::vector<EnemyConfig> enemies;
  std::vector<Droplet> droplets;

  Vector2 spawnPoint{100, 300};
  Vector2 sunPosition{100, 100};
  Rectangle exitZone{0, 0, 0, 0};
  unsigned seed = 0;
  int biome = 0;

  bool IsEdge(Vector2 pos) const;
};
