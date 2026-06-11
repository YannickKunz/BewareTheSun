#include "LevelGenerator.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace {

// Names avoid accented characters: raylib's default font has no accents.
const char *kLevelNames[] = {
    "Le Jardin Oublie",     "La Serre Brisee",     "Les Toits Brulants",
    "Le Mur de Lierre",     "La Carriere Seche",   "Les Hauteurs Venteuses",
    "L'Ascension Finale",
};

bool Overlaps(const Rectangle &a, const Rectangle &b, float margin) {
  Rectangle grown = {a.x - margin, a.y - margin, a.width + 2 * margin,
                     a.height + 2 * margin};
  return CheckCollisionRecs(grown, b);
}

} // namespace

const char *LevelName(int difficulty) {
  int n = (int)(sizeof(kLevelNames) / sizeof(kLevelNames[0]));
  return kLevelNames[((difficulty % n) + n) % n];
}

Level GenerateLevel(int screenW, int screenH, unsigned seed, int difficulty) {
  std::mt19937 rng(seed);
  auto frand = [&](float a, float b) {
    std::uniform_real_distribution<float> d(a, b);
    return d(rng);
  };
  auto chance = [&](float p) { return frand(0.0f, 1.0f) < p; };

  const float W = (float)screenW;
  const float H = (float)screenH;
  const float diff = (float)difficulty;

  Level lvl;
  lvl.seed = seed;
  lvl.biome = difficulty;

  const float th = H * 0.036f; // platform thickness
  const float groundY = H * 0.94f;

  // Ground + invisible world borders
  lvl.platforms.push_back({{0, groundY, W, H - groundY}, PlatformType::NORMAL});
  lvl.platforms.push_back({{-40, -300, 40, H + 600}, PlatformType::INVISIBLE});
  lvl.platforms.push_back({{W, -300, 40, H + 600}, PlatformType::INVISIBLE});
  lvl.platforms.push_back({{0, -340, W, 40}, PlatformType::INVISIBLE});

  // --- Difficulty-scaled tuning ---
  // Max jump height ~= JUMP_FORCE^2 / (2*GRAVITY) = 0.78^2/(2*1.25) ~ 0.243*H.
  // Keep every rise comfortably below that.
  float minW = std::max(W * (0.150f - 0.011f * diff), W * 0.080f);
  float maxW = std::max(W * (0.210f - 0.013f * diff), minW + W * 0.025f);
  float gapMin = W * (0.045f + 0.007f * diff);
  float gapMax = W * (0.080f + 0.011f * diff);
  float riseMin = H * 0.080f;
  float riseMax = std::min(H * (0.125f + 0.010f * diff), H * 0.175f);

  float flowerChance = std::min(0.10f + 0.06f * diff, 0.38f);
  float dropletChance = 0.60f;
  int enemyBudget = (difficulty == 0) ? (chance(0.7f) ? 1 : 0)
                                      : 1 + (difficulty + 1) / 2;

  // --- Build a guaranteed-solvable zigzag path from the ground to the top ---
  bool startLeft = chance(0.5f);
  int dir = startLeft ? 1 : -1;
  lvl.spawnPoint = {startLeft ? W * 0.04f : W * 0.90f, groundY - H * 0.14f};

  // Current ledge interval (start: ground around the spawn)
  float px0 = startLeft ? 0.0f : W * 0.80f;
  float px1 = startLeft ? W * 0.20f : W;
  float y = groundY;
  const float exitY = H * frand(0.15f, 0.21f);
  const float sideMargin = W * 0.015f;

  std::vector<int> pathIdx; // indices of path platforms in lvl.platforms
  int sincePathFlower = 99; // avoid two flower steps in a row
  int guard = 0;

  while (y - riseMin > exitY && guard++ < 64) {
    float w = frand(minW, maxW);
    float gap = frand(gapMin, gapMax);
    float rise = frand(riseMin, riseMax);
    float ny = std::max(y - rise, exitY);

    float nx0;
    if (dir > 0) {
      nx0 = px1 + gap;
      if (nx0 + w > W - sideMargin) { // hit right edge -> bounce left
        dir = -1;
        nx0 = px0 - gap - w;
      }
    } else {
      nx0 = px0 - gap - w;
      if (nx0 < sideMargin) { // hit left edge -> bounce right
        dir = 1;
        nx0 = px1 + gap;
      }
    }
    nx0 = std::clamp(nx0, sideMargin, W - sideMargin - w);

    PlatformType type = PlatformType::NORMAL;
    if (sincePathFlower >= 2 && pathIdx.size() >= 1 && chance(flowerChance)) {
      type = PlatformType::FLOWER;
      sincePathFlower = 0;
    } else {
      sincePathFlower++;
    }

    Rectangle r = {nx0, ny, w, th};
    lvl.platforms.push_back({r, type});
    pathIdx.push_back((int)lvl.platforms.size() - 1);

    // Droplet floating above the platform
    if (chance(dropletChance)) {
      lvl.droplets.push_back(
          {{nx0 + w * frand(0.25f, 0.75f), ny - H * 0.075f}, false,
           frand(0.0f, 6.28f)});
    }

    // Roach patrolling a wide normal platform (never the first step).
    // position.y is the platform surface; Game offsets by sprite height.
    if (enemyBudget > 0 && type == PlatformType::NORMAL &&
        pathIdx.size() >= 2 && w >= W * 0.12f && chance(0.45f)) {
      lvl.enemies.push_back(
          {EnemyType::ROACH, {nx0 + w * 0.5f, ny}, nx0 + 2, nx0 + w - 2});
      enemyBudget--;
    }
    // Spider bobbing on a thread over the gap we just crossed
    else if (enemyBudget > 0 && difficulty >= 1 && pathIdx.size() >= 2 &&
             chance(0.35f)) {
      float gx = (dir > 0) ? (px1 + nx0) * 0.5f : (nx0 + w + px0) * 0.5f;
      gx = std::clamp(gx, W * 0.05f, W * 0.95f);
      float top = std::max(H * 0.04f, ny - H * 0.34f);
      float bottom = ny + H * 0.06f;
      lvl.enemies.push_back({EnemyType::SPIDER, {gx, top}, top, bottom});
      enemyBudget--;
    }

    px0 = nx0;
    px1 = nx0 + w;
    y = ny;
  }

  // --- Final (exit) platform: generous landing zone at the top ---
  {
    float w = W * 0.20f;
    float gap = frand(gapMin, gapMax * 0.8f);
    float ny = std::max(exitY - H * 0.02f, H * 0.10f);

    float nx0 = (dir > 0) ? px1 + gap : px0 - gap - w;
    nx0 = std::clamp(nx0, sideMargin, W - sideMargin - w);

    Rectangle r = {nx0, ny, w, th};
    lvl.platforms.push_back({r, PlatformType::NORMAL});
    pathIdx.push_back((int)lvl.platforms.size() - 1);

    float exitW = W * 0.055f;
    float exitH = H * 0.075f;
    // Watering can sits at the far end of the final platform
    float ex = (dir > 0) ? nx0 + w - exitW - 8 : nx0 + 8;
    lvl.exitZone = {ex, ny - exitH, exitW, exitH};

    // Sun on the opposite side of the exit, so the final approach is exposed
    bool exitOnLeft = (ex + exitW * 0.5f) < W * 0.5f;
    lvl.sunPosition = {exitOnLeft ? W - W * 0.08f : W * 0.08f, H * 0.08f};
  }

  // --- Bouncy mushrooms on the ground (fun shortcuts at night) ---
  {
    int count = 1 + (chance(0.6f) ? 1 : 0);
    for (int i = 0; i < count; i++) {
      float mw = W * 0.055f;
      float mh = H * 0.085f;
      float mx = frand(W * 0.15f, W * 0.80f);
      Rectangle r = {mx, groundY - mh, mw, mh};
      bool bad = false;
      for (const auto &p : lvl.platforms)
        if (p.type != PlatformType::INVISIBLE && p.rect.y < groundY &&
            Overlaps(r, p.rect, W * 0.01f)) {
          bad = true;
          break;
        }
      if (!bad)
        lvl.platforms.push_back({r, PlatformType::MUSHROOM});
    }
  }

  // --- A couple of decorative side ledges (extra shade, no droplets:
  // droplets are required for the exit so they must stay on the main path) ---
  {
    int tries = 6, placed = 0;
    while (tries-- > 0 && placed < 2) {
      float w = frand(W * 0.06f, W * 0.10f);
      float x = frand(sideMargin, W - sideMargin - w);
      float ly = frand(H * 0.35f, H * 0.80f);
      Rectangle r = {x, ly, w, th};
      bool bad = false;
      for (const auto &p : lvl.platforms)
        if (p.type != PlatformType::INVISIBLE &&
            Overlaps(r, p.rect, H * 0.07f)) {
          bad = true;
          break;
        }
      if (bad)
        continue;
      lvl.platforms.push_back({r, PlatformType::NORMAL});
      placed++;
    }
  }

  // Guarantee a minimum droplet count so the exit objective is meaningful
  int fillTries = 24;
  while ((int)lvl.droplets.size() < 3 && !pathIdx.empty() && fillTries-- > 0) {
    const Rectangle &r = lvl.platforms[pathIdx[rng() % pathIdx.size()]].rect;
    Vector2 pos = {r.x + r.width * frand(0.3f, 0.7f), r.y - H * 0.075f};
    bool tooClose = false;
    for (const auto &d : lvl.droplets)
      if (fabsf(d.pos.x - pos.x) < W * 0.04f &&
          fabsf(d.pos.y - pos.y) < H * 0.03f) {
        tooClose = true;
        break;
      }
    if (!tooClose)
      lvl.droplets.push_back({pos, false, frand(0.0f, 6.28f)});
  }

  return lvl;
}
