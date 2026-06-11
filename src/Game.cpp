#include "Game.h"
#include "entities/Roach.h"
#include "entities/Spider.h"
#include "world/LevelGenerator.h"
#include <algorithm>
#include <cmath>
#include <random>

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------
namespace {

bool CheckCollisionLineRect(Vector2 start, Vector2 end, Rectangle rect) {
  if (CheckCollisionPointRec(start, rect) || CheckCollisionPointRec(end, rect))
    return true;

  Vector2 p1 = {rect.x, rect.y};
  Vector2 p2 = {rect.x + rect.width, rect.y};
  Vector2 p3 = {rect.x + rect.width, rect.y + rect.height};
  Vector2 p4 = {rect.x, rect.y + rect.height};

  Vector2 cp;
  return CheckCollisionLines(start, end, p1, p2, &cp) ||
         CheckCollisionLines(start, end, p2, p3, &cp) ||
         CheckCollisionLines(start, end, p3, p4, &cp) ||
         CheckCollisionLines(start, end, p4, p1, &cp);
}

// Where a ray from 'start' toward 'end' first hits a solid platform
// (bisection refinement; good enough for god-ray visuals).
Vector2 GetRayIntersection(Vector2 start, Vector2 end,
                           const std::vector<Platform> &platforms,
                           bool isDay) {
  float minT = 1.0f;
  for (const auto &plat : platforms) {
    if (!plat.IsSolid(isDay) || plat.type == PlatformType::INVISIBLE)
      continue;
    if (!CheckCollisionLineRect(start, end, plat.rect))
      continue;

    float t0 = 0.0f, t1 = minT;
    for (int i = 0; i < 8; i++) {
      float mid = t0 + (t1 - t0) * 0.5f;
      Vector2 midPoint = {start.x + (end.x - start.x) * mid,
                          start.y + (end.y - start.y) * mid};
      if (CheckCollisionPointRec(midPoint, plat.rect) ||
          CheckCollisionLineRect(start, midPoint, plat.rect))
        t1 = mid;
      else
        t0 = mid;
    }
    minT = std::min(minT, t1);
  }
  return {start.x + (end.x - start.x) * minT,
          start.y + (end.y - start.y) * minT};
}

} // namespace

Game::~Game() {
  if (!isUnloaded)
    Unload();
}

// ---------------------------------------------------------------------------
// Init / level management
// ---------------------------------------------------------------------------

void Game::Init() {
  // All level art is generated procedurally
  art.Load();

  // Hand-drawn screens kept from the original game
  introScreenTex = LoadTexture("assets/sprites/introScreenWithBackground.png");
  introImageTex = LoadTexture("assets/sprites/introImage.png");
  gameOverScreenTex = LoadTexture("assets/sprites/gameOverScreen.png");

  // Player sprites
  player.sprite = art.playerIdle;
  player.spritesheet = art.playerWalk;
  player.frameCount = art.playerWalkFrames;
  player.frameSpeed = 0.11f;
  player.animated = true;
  player.deathSprite = art.playerDeath;
  ApplyPlayerSize();

  // Audio
  jumpSound = LoadSound("assets/audio/jump_sound.wav");
  walkSound = LoadSound("assets/audio/walk_sound.wav");
  burnSound = LoadSound("assets/audio/burn_sound.wav");
  deathSound = LoadSound("assets/audio/death_sound.wav");
  wateringCanSound = LoadSound("assets/audio/watering_can.wav");

  dayTracks[0] = LoadMusicStream("assets/audio/lvlupjam_lvl1.wav");
  dayTracks[1] = LoadMusicStream("assets/audio/lvlupjam_lvl2.wav");
  dayTracks[2] = LoadMusicStream("assets/audio/lvlupjam_lvl3.wav");
  nightTracks[0] = LoadMusicStream("assets/audio/lvlupjam_lvl1_night.wav");
  nightTracks[1] = LoadMusicStream("assets/audio/lvlupjam_lvl2_night.wav");
  // No dedicated night track for the third theme: reuse the first
  nightTracks[2] = LoadMusicStream("assets/audio/lvlupjam_lvl1_night.wav");
  titleMusic = LoadMusicStream("assets/audio/titlescreenmusicmp3.mp3");
  audioLoaded = true;
  ApplyVolume();

  // Fresh seeds every run: a new garden each time
  std::random_device rd;
  seeds.resize(Core::LEVEL_COUNT);
  for (auto &s : seeds)
    s = rd();
  GenerateAllLevels();

  currentScreen = TITLE;
}

void Game::GenerateAllLevels() {
  levels.clear();
  levels.reserve(Core::LEVEL_COUNT);
  for (int i = 0; i < Core::LEVEL_COUNT; ++i)
    levels.push_back(
        GenerateLevel(Core::SCREEN_WIDTH, Core::SCREEN_HEIGHT, seeds[i], i));
}

void Game::RegenerateCurrentLevel(bool newSeed) {
  if (newSeed)
    seeds[currentLevelIndex] = (unsigned)GetRandomValue(1, 0x7FFFFFFF);
  levels[currentLevelIndex] =
      GenerateLevel(Core::SCREEN_WIDTH, Core::SCREEN_HEIGHT,
                    seeds[currentLevelIndex], currentLevelIndex);
  LoadLevel(currentLevelIndex);
}

void Game::ApplyPlayerSize() {
  player.height = (float)Core::SCREEN_HEIGHT * 0.085f;
  player.width = player.height * (20.0f / 24.0f); // sprite aspect
}

void Game::LoadLevel(int index) {
  if (index < 0 || index >= (int)levels.size())
    return;

  currentLevelIndex = index;
  Level &lvl = levels[index];

  for (auto &d : lvl.droplets)
    d.collected = false;
  for (auto &p : lvl.platforms)
    p.rect.y = p.initialY;

  isDayTime = true;
  dayBlend = 1.0f;
  player.Reset(lvl.spawnPoint);
  deathStarted = false;
  deathTimer = 0.0f;
  particles.clear();
  bannerTimer = 3.0f;
  walkSoundTimer = 0.0f;
  burnSoundTimer = 0.0f;

  // Spawn enemies from level data
  enemies.clear();
  float H = (float)Core::SCREEN_HEIGHT;
  for (const auto &cfg : lvl.enemies) {
    if (cfg.type == EnemyType::ROACH) {
      auto r = std::make_unique<Roach>(cfg.position);
      r->spritesheet = art.roach;
      r->frameCount = art.roachFrames;
      r->frameSpeed = 0.14f;
      r->animated = true;
      r->height = H * 0.05f;
      r->width = r->height * 2.0f; // 20x10 sprite
      r->minBound = cfg.minBound;
      r->maxBound = cfg.maxBound;
      r->position.x = cfg.position.x - r->width * 0.5f;
      r->position.y = cfg.position.y - r->height; // cfg y = platform surface
      enemies.push_back(std::move(r));
    } else if (cfg.type == EnemyType::SPIDER) {
      auto s = std::make_unique<Spider>(cfg.position);
      s->spritesheet = art.spider;
      s->frameCount = art.spiderFrames;
      s->frameSpeed = 0.18f;
      s->animated = true;
      s->height = H * 0.05f;
      s->width = s->height * (16.0f / 14.0f);
      s->minBound = cfg.minBound;
      s->maxBound = cfg.maxBound;
      s->silkTop = std::max(0.0f, cfg.minBound - H * 0.06f);
      s->position.x = cfg.position.x - s->width * 0.5f;
      enemies.push_back(std::move(s));
    }
  }

  PlayLevelMusic();
}

void Game::DebugStartLevel(int index) {
  if (audioLoaded)
    StopMusicStream(titleMusic);
  currentScreen = GAMEPLAY;
  fade = 0.0f;
  fadingOut = false;
  debugMode = true; // self-test runs want hitbox overlays
  LoadLevel(index);
  bannerTimer = 0.0f; // keep screenshots clean
}

void Game::DebugToggleNight() {
  isDayTime = !isDayTime;
  dayBlend = isDayTime ? 1.0f : 0.0f;
  PlayLevelMusic();
}

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------

void Game::StopCurrentMusic() {
  if (currentPlayingMusic != nullptr) {
    StopMusicStream(*currentPlayingMusic);
    currentPlayingMusic = nullptr;
  }
}

void Game::PlayLevelMusic() {
  StopCurrentMusic();
  if (!audioLoaded)
    return;
  Music &m = isDayTime ? dayTracks[currentLevelIndex % TRACK_COUNT]
                       : nightTracks[currentLevelIndex % TRACK_COUNT];
  PlayMusicStream(m);
  SetMusicVolume(m, musicVolume);
  currentPlayingMusic = &m;
}

void Game::ApplyVolume() {
  SetMasterVolume(masterVolume);
  if (audioLoaded) {
    for (int i = 0; i < TRACK_COUNT; ++i) {
      SetMusicVolume(dayTracks[i], musicVolume);
      SetMusicVolume(nightTracks[i], musicVolume);
    }
    SetMusicVolume(titleMusic, musicVolume);
    SetSoundVolume(jumpSound, sfxVolume);
    SetSoundVolume(walkSound, sfxVolume * 0.6f);
    SetSoundVolume(burnSound, sfxVolume);
    SetSoundVolume(deathSound, sfxVolume);
    SetSoundVolume(wateringCanSound, sfxVolume);
  }
}

// ---------------------------------------------------------------------------
// Particles & shake
// ---------------------------------------------------------------------------

void Game::Emit(Vector2 pos, int count, Color color, float speed, bool gravity,
                float life) {
  for (int i = 0; i < count; ++i) {
    float a = GetRandomValue(0, 628) / 100.0f;
    float v = speed * GetRandomValue(40, 100) / 100.0f;
    Particle p;
    p.pos = pos;
    p.vel = {cosf(a) * v, sinf(a) * v - (gravity ? speed * 0.3f : 0.0f)};
    p.maxLife = p.life = life * GetRandomValue(60, 130) / 100.0f;
    p.size = (float)GetRandomValue(2, 5);
    p.color = color;
    p.gravity = gravity;
    particles.push_back(p);
  }
}

void Game::UpdateParticles(float dt) {
  float g = Core::GRAVITY * 0.6f;
  for (auto &p : particles) {
    p.life -= dt;
    if (p.gravity)
      p.vel.y += g * dt;
    p.pos.x += p.vel.x * dt;
    p.pos.y += p.vel.y * dt;
  }
  particles.erase(std::remove_if(particles.begin(), particles.end(),
                                 [](const Particle &p) { return p.life <= 0; }),
                  particles.end());
}

void Game::DrawParticles() const {
  for (const auto &p : particles) {
    float t = p.life / p.maxLife;
    DrawRectangleV(p.pos, {p.size * t + 1, p.size * t + 1}, Fade(p.color, t));
  }
}

void Game::Shake(float time, float mag) {
  shakeTime = std::max(shakeTime, time);
  shakeMag = std::max(shakeMag, mag);
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void Game::Update() {
  if (IsKeyPressed(KEY_H))
    debugMode = !debugMode;

  if (currentPlayingMusic != nullptr)
    UpdateMusicStream(*currentPlayingMusic);

  switch (currentScreen) {
  case TITLE:
  case STORY:
  case WIN:
    if (audioLoaded)
      UpdateMusicStream(titleMusic);
    break;
  default:
    break;
  }

  switch (currentScreen) {
  case TITLE:
    UpdateTitle();
    break;
  case STORY:
    UpdateStory();
    break;
  case GAMEPLAY:
    UpdateGameplay();
    break;
  case GAME_OVER:
    UpdateGameOver();
    break;
  case SETTINGS:
    UpdateSettings();
    break;
  case WIN:
    UpdateWin();
    break;
  }
}

void Game::UpdateTitle() {
  if (audioLoaded && !IsMusicStreamPlaying(titleMusic))
    PlayMusicStream(titleMusic);
  if (IsKeyPressed(KEY_ENTER))
    currentScreen = STORY;
  if (IsKeyPressed(KEY_ESCAPE)) {
    previousScreen = TITLE;
    currentScreen = SETTINGS;
  }
}

void Game::UpdateStory() {
  if (IsKeyPressed(KEY_ENTER)) {
    if (audioLoaded)
      StopMusicStream(titleMusic);
    currentScreen = GAMEPLAY;
    fade = 1.0f; // fade in from black
    fadingOut = false;
    LoadLevel(0);
  }
}

void Game::UpdateGameOver() {
  if (IsKeyPressed(KEY_R)) {
    currentScreen = GAMEPLAY;
    fade = 1.0f;
    fadingOut = false;
    LoadLevel(currentLevelIndex);
  }
}

void Game::UpdateWin() {
  if (IsKeyPressed(KEY_ENTER)) {
    // New seeds for the next playthrough
    std::random_device rd;
    for (auto &s : seeds)
      s = rd();
    GenerateAllLevels();
    currentScreen = TITLE;
  }
}

void Game::UpdateGameplay() {
  float dt = GetFrameTime();
  float W = (float)Core::SCREEN_WIDTH;
  float H = (float)Core::SCREEN_HEIGHT;
  Level &lvl = levels[currentLevelIndex];

  // Timers
  if (toggleCooldown > 0)
    toggleCooldown -= dt;
  if (sunHintTimer > 0)
    sunHintTimer -= dt;
  if (bannerTimer > 0)
    bannerTimer -= dt;
  if (shakeTime > 0) {
    shakeTime -= dt;
    if (shakeTime <= 0)
      shakeMag = 0;
  }

  // Level transition fade
  if (fadingOut) {
    fade += dt * 2.2f;
    if (fade >= 1.0f) {
      fade = 1.0f;
      fadingOut = false;
      if (pendingLevel >= Core::LEVEL_COUNT) {
        pendingLevel = -1;
        StopCurrentMusic();
        if (audioLoaded)
          PlayMusicStream(titleMusic);
        currentScreen = WIN;
        return;
      }
      if (pendingLevel >= 0) {
        int next = pendingLevel;
        pendingLevel = -1;
        LoadLevel(next);
      }
    }
  } else if (fade > 0) {
    fade = std::max(0.0f, fade - dt * 2.2f);
  }

  // Day/night visual crossfade
  dayBlend += ((isDayTime ? 1.0f : 0.0f) - dayBlend) *
              std::min(1.0f, 3.0f * dt);

  // Meta keys: R restart, N reroll layout, L skip level (debug)
  if (IsKeyPressed(KEY_R) && !player.isDead && !fadingOut)
    LoadLevel(currentLevelIndex);
  if (IsKeyPressed(KEY_N) && !fadingOut)
    RegenerateCurrentLevel(true);
  if (debugMode && IsKeyPressed(KEY_L) && !fadingOut) {
    pendingLevel = currentLevelIndex + 1;
    fadingOut = true;
  }

  // Day/night toggle
  if (IsKeyPressed(KEY_T) && toggleCooldown <= 0 && !player.isDead) {
    isDayTime = !isDayTime;
    toggleCooldown = 0.4f;
    PlayLevelMusic();
    Emit(player.Center(), 16,
         isDayTime ? Color{255, 214, 90, 255} : Color{120, 160, 255, 255},
         H * 0.18f, false, 0.5f);
  }

  // Flower platforms drift between day/night heights and carry the player
  for (auto &plat : lvl.platforms) {
    if (plat.type != PlatformType::FLOWER)
      continue;
    float targetY = plat.initialY + (isDayTime ? 0.0f : H * 0.22f);
    float moveY = (targetY - plat.rect.y) * std::min(1.0f, 5.0f * dt);
    plat.rect.y += moveY;

    Rectangle pr = player.GetRect();
    if (pr.x + pr.width > plat.rect.x &&
        pr.x < plat.rect.x + plat.rect.width) {
      float bottom = pr.y + pr.height;
      if (bottom <= plat.rect.y + 12 && bottom >= plat.rect.y - 12)
        player.position.y += moveY;
    }
  }

  // Flower sway animation
  flowerAnimTimer += dt;
  if (flowerAnimTimer >= 0.15f) {
    flowerAnimTimer -= 0.15f;
    flowerFrame = (flowerFrame + 1) % art.flowerFrames;
  }

  // Player
  player.Update(dt, lvl.platforms, isDayTime);

  if (player.jumpedThisFrame)
    PlaySound(jumpSound);
  if (player.bouncedThisFrame) {
    PlaySound(jumpSound);
    Emit({player.Center().x, player.position.y + player.height}, 12,
         isDayTime ? Color{192, 58, 43, 255} : Color{46, 196, 182, 255},
         H * 0.16f, true, 0.5f);
    Shake(0.12f, 3.0f);
  }
  if (player.landedThisFrame)
    Emit({player.Center().x, player.position.y + player.height}, 6,
         Color{150, 140, 120, 255}, H * 0.07f, true, 0.35f);

  // Footsteps
  if (player.isMoving && player.isGrounded && !player.isDead) {
    walkSoundTimer -= dt;
    if (walkSoundTimer <= 0.0f) {
      PlaySound(walkSound);
      walkSoundTimer = 0.34f;
    }
  } else {
    walkSoundTimer = 0.0f;
  }

  // Droplets: collect to unlock the watering can
  for (auto &d : lvl.droplets) {
    if (d.collected)
      continue;
    float bobY = d.pos.y + sinf((float)GetTime() * 2.0f + d.phase) * 4.0f;
    if (CheckCollisionCircleRec({d.pos.x, bobY}, H * 0.020f,
                                player.GetRect())) {
      d.collected = true;
      PlaySound(wateringCanSound);
      player.hp = std::min(player.maxHp, player.hp + 0.5f);
      Emit({d.pos.x, bobY}, 14, Color{79, 195, 247, 255}, H * 0.15f, true,
           0.6f);
    }
  }

  // Enemies prowl at night
  if (!isDayTime && !player.isDead) {
    for (auto it = enemies.begin(); it != enemies.end();) {
      Enemy *e = it->get();
      e->Update(dt, lvl);

      bool removed = false;
      if (player.invulnTimer <= 0.0f &&
          CheckCollisionRecs(player.GetRect(), e->GetRect())) {
        bool stomp = player.velocity.y > 60.0f &&
                     (player.position.y + player.height) <
                         e->position.y + e->height * 0.6f;
        if (stomp) {
          Emit(e->Center(), 16, Color{120, 80, 140, 255}, H * 0.16f, true,
               0.6f);
          PlaySound(jumpSound);
          player.velocity.y = -Core::JUMP_FORCE * Core::STOMP_BOUNCE;
          Shake(0.15f, 4.0f);
          it = enemies.erase(it);
          removed = true;
        } else {
          player.TakeDamage(1.0f);
          player.invulnTimer = Core::INVULN_TIME;
          player.Knockback(e->Center().x);
          PlaySound(burnSound);
          Shake(0.25f, 6.0f);
          Emit(player.Center(), 10, Color{224, 58, 75, 255}, H * 0.14f, true,
               0.5f);
        }
      }
      if (!removed)
        ++it;
    }
  } else {
    // Keep their little legs moving (animation only) while hidden
    for (auto &e : enemies)
      e->AdvanceAnimation(dt);
  }

  // Sun exposure: line of sight from the sun to the player
  bool exposed = false;
  if (isDayTime && !player.isDead) {
    exposed = true;
    Vector2 pc = player.Center();
    for (const auto &plat : lvl.platforms) {
      if (!plat.IsSolid(true) || plat.type == PlatformType::INVISIBLE)
        continue;
      if (CheckCollisionLineRect(lvl.sunPosition, pc, plat.rect)) {
        exposed = false;
        break;
      }
    }
    if (exposed) {
      player.TakeDamage(Core::SUN_DPS * dt);
      player.burnFlash = std::min(1.0f, player.burnFlash + dt * 2.0f);
      burnSoundTimer -= dt;
      if (burnSoundTimer <= 0.0f && player.hp > 0) {
        PlaySound(burnSound);
        burnSoundTimer = 0.9f;
      }
      if (GetRandomValue(0, 99) < 40) { // smouldering embers
        Vector2 p = {player.position.x +
                         (float)GetRandomValue(0, (int)player.width),
                     player.position.y +
                         (float)GetRandomValue(0, (int)player.height)};
        Particle ember;
        ember.pos = p;
        ember.vel = {(float)GetRandomValue(-20, 20), -H * 0.08f};
        ember.maxLife = ember.life = 0.5f;
        ember.size = 3;
        ember.color = (GetRandomValue(0, 1) == 0)
                          ? Color{255, 140, 40, 255}
                          : Color{255, 80, 40, 255};
        ember.gravity = false;
        particles.push_back(ember);
      }
      if (!sunHintShown) {
        sunHintShown = true;
        sunHintTimer = 4.0f;
      }
    }
  }
  if (!exposed) {
    player.burnFlash = std::max(0.0f, player.burnFlash - dt * 3.0f);
    burnSoundTimer = 0.0f;
    // Recover in the shade (slower at night: it's cold out there)
    if (!player.isDead && player.hp < player.maxHp)
      player.hp = std::min(
          player.maxHp,
          player.hp + Core::SHADE_REGEN * (isDayTime ? 1.0f : 0.5f) * dt);
  }

  UpdateParticles(dt);

  // Death sequence
  if (player.isDead) {
    if (!deathStarted) {
      deathStarted = true;
      deathTimer = 0.0f;
      StopCurrentMusic();
      PlaySound(deathSound);
      Shake(0.4f, 8.0f);
      Emit(player.Center(), 20, Color{178, 178, 168, 255}, H * 0.14f, true,
           0.9f);
    }
    deathTimer += dt;
    if (deathTimer > 1.3f)
      currentScreen = GAME_OVER;
  }

  // Exit: needs every droplet
  if (!player.isDead && !fadingOut && fade <= 0.01f && ExitUnlocked(lvl) &&
      CheckCollisionRecs(player.GetRect(), lvl.exitZone)) {
    PlaySound(wateringCanSound);
    Emit({lvl.exitZone.x + lvl.exitZone.width / 2,
          lvl.exitZone.y + lvl.exitZone.height / 2},
         20, Color{79, 195, 247, 255}, H * 0.16f, true, 0.8f);
    pendingLevel = currentLevelIndex + 1;
    fadingOut = true;
  }

  if (IsKeyPressed(KEY_ESCAPE)) {
    previousScreen = GAMEPLAY;
    currentScreen = SETTINGS;
  }
  (void)W;
}

int Game::DropletsCollected(const Level &lvl) const {
  int n = 0;
  for (const auto &d : lvl.droplets)
    if (d.collected)
      n++;
  return n;
}

bool Game::ExitUnlocked(const Level &lvl) const {
  return DropletsCollected(lvl) == (int)lvl.droplets.size();
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void Game::Draw() {
  BeginDrawing();
  ClearBackground(BLACK);

  switch (currentScreen) {
  case TITLE:
    DrawTitle();
    break;
  case STORY:
    DrawStory();
    break;
  case GAMEPLAY:
    DrawGameplay();
    break;
  case SETTINGS:
    DrawSettings();
    break;
  case WIN:
    DrawWin();
    break;
  case GAME_OVER:
    DrawGameOver();
    break;
  }

  EndDrawing();
}

void Game::DrawGameplay() {
  float W = (float)Core::SCREEN_WIDTH;
  float H = (float)Core::SCREEN_HEIGHT;
  Level &lvl = levels[currentLevelIndex];
  float nightAlpha = 1.0f - dayBlend;
  float time = (float)GetTime();

  // Screen shake
  Camera2D cam{};
  cam.zoom = 1.0f;
  if (shakeMag > 0.0f && shakeTime > 0.0f) {
    cam.offset = {(float)GetRandomValue(-100, 100) / 100.0f * shakeMag,
                  (float)GetRandomValue(-100, 100) / 100.0f * shakeMag};
  }
  BeginMode2D(cam);

  // --- Background crossfade ---
  int biome = lvl.biome % ProcArt::BIOME_COUNT;
  Rectangle bgDest = {0, 0, W, H};
  {
    Texture2D &d = art.bgDay[biome];
    DrawTexturePro(d, {0, 0, (float)d.width, (float)d.height}, bgDest, {0, 0},
                   0, WHITE);
    if (nightAlpha > 0.02f) {
      Texture2D &n = art.bgNight[biome];
      DrawTexturePro(n, {0, 0, (float)n.width, (float)n.height}, bgDest,
                     {0, 0}, 0, Fade(WHITE, nightAlpha));
    }
  }

  // --- Sun (day) ---
  if (dayBlend > 0.02f) {
    Vector2 sp = lvl.sunPosition;
    float glowSize = H * 0.30f * (1.0f + 0.03f * sinf(time * 2.0f));
    DrawTexturePro(art.sunGlow,
                   {0, 0, (float)art.sunGlow.width, (float)art.sunGlow.height},
                   {sp.x - glowSize / 2, sp.y - glowSize / 2, glowSize,
                    glowSize},
                   {0, 0}, 0, Fade(WHITE, dayBlend));
    DrawCircleV(sp, H * 0.055f, Fade(Color{255, 236, 150, 255}, dayBlend));
    DrawCircleV(sp, H * 0.045f, Fade(Color{255, 220, 100, 255}, dayBlend));

    // God rays, stopped by solid platforms
    Color rayColor = Fade(YELLOW, 0.13f * dayBlend);
    int step = (int)(W / 32.0f);
    for (int x = 0; x <= (int)W; x += step) {
      Vector2 target = {(float)x, H};
      Vector2 end = GetRayIntersection(sp, target, lvl.platforms, true);
      DrawLineV(sp, end, rayColor);
    }
  }

  // --- Moon (night) ---
  if (nightAlpha > 0.02f) {
    float ms = H * 0.085f;
    Vector2 mp = {std::clamp(W - lvl.sunPosition.x, W * 0.12f, W * 0.78f),
                  H * 0.10f};
    DrawTexturePro(art.moon,
                   {0, 0, (float)art.moon.width, (float)art.moon.height},
                   {mp.x - ms / 2, mp.y - ms / 2, ms, ms}, {0, 0}, 0,
                   Fade(WHITE, nightAlpha));
  }

  // --- Platforms ---
  float ts = H * 0.036f; // tile size
  auto drawTiled = [&](const Rectangle &r, Texture2D top, Texture2D fill,
                       float alpha) {
    int rows = (int)ceilf(r.height / ts);
    int cols = (int)ceilf(r.width / ts);
    for (int ry = 0; ry < rows; ++ry) {
      float h = std::min(ts, r.height - ry * ts);
      for (int cx = 0; cx < cols; ++cx) {
        float w = std::min(ts, r.width - cx * ts);
        Texture2D &t = (ry == 0) ? top : fill;
        Rectangle src = {0, 0, 16.0f * (w / ts), 16.0f * (h / ts)};
        Rectangle dst = {r.x + cx * ts, r.y + ry * ts, w, h};
        DrawTexturePro(t, src, dst, {0, 0}, 0, Fade(WHITE, alpha));
      }
    }
  };

  for (const auto &plat : lvl.platforms) {
    switch (plat.type) {
    case PlatformType::INVISIBLE:
      if (debugMode)
        DrawRectangleRec(plat.rect, Fade(LIME, 0.3f));
      break;

    case PlatformType::NORMAL:
      drawTiled(plat.rect, art.tileDayTop, art.tileDayFill, 1.0f);
      if (nightAlpha > 0.02f)
        drawTiled(plat.rect, art.tileNightTop, art.tileNightFill, nightAlpha);
      break;

    case PlatformType::FLOWER: {
      // The petal disc (rows 3..9 of the 36px frame) IS the platform
      float frameW = (float)art.flower.width / art.flowerFrames;
      float frameH = (float)art.flower.height;
      Rectangle src = {frameW * flowerFrame, 0, frameW, frameH};
      float destH = plat.rect.height * (36.0f / 7.0f);
      float destW = destH * (28.0f / 36.0f);
      Rectangle dest = {plat.rect.x + plat.rect.width / 2 - destW / 2,
                        plat.rect.y - destH * (3.0f / 36.0f), destW, destH};
      float alpha = plat.IsSolid(isDayTime) ? 1.0f : 0.35f;
      DrawTexturePro(art.flower, src, dest, {0, 0}, 0, Fade(WHITE, alpha));
      break;
    }

    case PlatformType::MUSHROOM: {
      Texture2D &day = art.mushroomDay;
      Rectangle src = {0, 0, (float)day.width, (float)day.height};
      // squash-and-stretch idle wobble
      float wob = 1.0f + 0.04f * sinf(time * 3.0f + plat.rect.x);
      Rectangle dest = {plat.rect.x, plat.rect.y + plat.rect.height * (1 - wob),
                        plat.rect.width, plat.rect.height * wob};
      // Ghosted during the day (not solid), vivid at night
      if (dayBlend > 0.02f)
        DrawTexturePro(day, src, dest, {0, 0}, 0, Fade(WHITE, dayBlend * 0.5f));
      if (nightAlpha > 0.02f) {
        Texture2D &night = art.mushroomNight;
        // glow halo at night
        DrawCircleV({plat.rect.x + plat.rect.width / 2,
                     plat.rect.y + plat.rect.height * 0.3f},
                    plat.rect.width * 0.75f,
                    Fade(Color{46, 196, 182, 255}, 0.15f * nightAlpha));
        DrawTexturePro(night, src, dest, {0, 0}, 0, Fade(WHITE, nightAlpha));
      }
      break;
    }
    }

    if (debugMode)
      DrawRectangleLinesEx(plat.rect, 2, RED);
  }

  // --- Droplets ---
  for (const auto &d : lvl.droplets) {
    if (d.collected)
      continue;
    float bobY = d.pos.y + sinf(time * 2.0f + d.phase) * 4.0f;
    float dh = H * 0.034f;
    float dw = dh * (8.0f / 12.0f);
    DrawCircleV({d.pos.x, bobY}, dh * 0.8f,
                Fade(Color{79, 195, 247, 255}, 0.18f));
    DrawTexturePro(art.droplet,
                   {0, 0, (float)art.droplet.width, (float)art.droplet.height},
                   {d.pos.x - dw / 2, bobY - dh / 2, dw, dh}, {0, 0}, 0,
                   WHITE);
  }

  // --- Exit: the watering can ---
  {
    bool unlocked = ExitUnlocked(lvl);
    Texture2D &can = isDayTime ? art.canDay : art.canNight;
    float ch = lvl.exitZone.height;
    float cw = ch * (26.0f / 20.0f);
    float cx = lvl.exitZone.x + lvl.exitZone.width / 2 - cw / 2;
    float cy = lvl.exitZone.y + lvl.exitZone.height - ch;
    if (unlocked) {
      float pulse = 0.5f + 0.5f * sinf(time * 4.0f);
      DrawCircleV({cx + cw / 2, cy + ch / 2}, ch * (0.8f + 0.15f * pulse),
                  Fade(Color{79, 195, 247, 255}, 0.25f));
    }
    DrawTexturePro(can, {0, 0, (float)can.width, (float)can.height},
                   {cx, cy, cw, ch}, {0, 0}, 0,
                   unlocked ? WHITE : Fade(WHITE, 0.45f));
    if (!unlocked) {
      // droplet counter floating above the can
      const char *txt =
          TextFormat("%d/%d", DropletsCollected(lvl), (int)lvl.droplets.size());
      int fs = (int)(H * 0.025f);
      float dh = H * 0.026f;
      float dw = dh * (8.0f / 12.0f);
      float tx = cx + cw / 2 - (MeasureText(txt, fs) + dw + 4) / 2.0f;
      float ty = cy - dh - 6;
      DrawTexturePro(art.droplet,
                     {0, 0, (float)art.droplet.width,
                      (float)art.droplet.height},
                     {tx, ty, dw, dh}, {0, 0}, 0, WHITE);
      DrawText(txt, (int)(tx + dw + 4), (int)ty, fs, RAYWHITE);
    }
    if (debugMode)
      DrawRectangleLinesEx(lvl.exitZone, 2, GOLD);
  }

  // --- Enemies (fade in with the night) ---
  if (nightAlpha > 0.03f) {
    for (auto &e : enemies) {
      e->drawAlpha = nightAlpha;
      e->Draw();
      if (debugMode)
        DrawRectangleLinesEx(e->GetRect(), 2, RED);
    }
  }

  // --- Player ---
  player.Draw();
  if (debugMode) {
    DrawRectangleLinesEx(player.GetRect(), 2, GREEN);
    DrawLineV(lvl.sunPosition, player.Center(), Fade(ORANGE, 0.8f));
  }

  DrawParticles();

  EndMode2D();

  DrawHUD();

  // Burn vignette
  if (player.burnFlash > 0.01f) {
    float a = 0.16f * player.burnFlash *
              (0.7f + 0.3f * sinf(time * 9.0f));
    DrawRectangle(0, 0, (int)W, (int)H, Fade(Color{255, 60, 30, 255}, a));
  }

  // Level banner
  if (bannerTimer > 0.0f) {
    float a = std::min(1.0f, bannerTimer);
    const char *title = TextFormat("NIVEAU %d", currentLevelIndex + 1);
    const char *name = LevelName(currentLevelIndex);
    int fs1 = (int)(H * 0.06f), fs2 = (int)(H * 0.032f);
    int y = (int)(H * 0.30f);
    DrawRectangle(0, y - 14, (int)W, fs1 + fs2 + 38,
                  Fade(BLACK, 0.45f * a));
    DrawText(title, (int)(W / 2 - MeasureText(title, fs1) / 2.0f), y, fs1,
             Fade(GOLD, a));
    DrawText(name, (int)(W / 2 - MeasureText(name, fs2) / 2.0f),
             y + fs1 + 10, fs2, Fade(RAYWHITE, a));
  }

  // First-burn hint
  if (sunHintTimer > 0.0f) {
    float alpha = std::min(1.0f, sunHintTimer);
    const char *hint = "Le soleil brule ! Appuie sur T pour passer en mode "
                       "Nuit... mais gare aux bestioles.";
    int fs = (int)(H * 0.028f);
    int textW = MeasureText(hint, fs);
    int hx = (int)(W / 2 - textW / 2.0f);
    int hy = (int)(H / 2 - H * 0.06f);
    DrawRectangle(hx - 15, hy - 10, textW + 30, fs + 20,
                  Fade(BLACK, 0.7f * alpha));
    DrawText(hint, hx, hy, fs, Fade(YELLOW, alpha));
  }

  if (debugMode) {
    DrawText(TextFormat("DEBUG  seed=%u  FPS=%d", lvl.seed, GetFPS()), 20,
             (int)(H * 0.115f), 18, RED);
  }

  // Transition fade
  if (fade > 0.0f)
    DrawRectangle(0, 0, (int)W, (int)H, Fade(BLACK, fade));
}

void Game::DrawHUD() {
  float W = (float)Core::SCREEN_WIDTH;
  float H = (float)Core::SCREEN_HEIGHT;
  Level &lvl = levels[currentLevelIndex];

  // Hearts (top-left)
  float s = H * 0.006f; // pixel scale of the 7x6 heart
  float hw = 7 * s, hh = 6 * s;
  for (int i = 0; i < (int)player.maxHp; ++i) {
    float x = 20 + i * (hw + 8);
    float y = 18;
    DrawTexturePro(art.heart, {0, 0, 7, 6}, {x, y, hw, hh}, {0, 0}, 0,
                   Fade(Color{40, 40, 40, 255}, 0.8f)); // empty slot
    float f = std::clamp(player.hp - i, 0.0f, 1.0f);
    if (f > 0)
      DrawTexturePro(art.heart, {0, 0, 7 * f, 6}, {x, y, hw * f, hh}, {0, 0},
                     0, WHITE);
  }

  // Droplet counter under the hearts
  {
    float dh = H * 0.032f;
    float dw = dh * (8.0f / 12.0f);
    DrawTexturePro(art.droplet,
                   {0, 0, (float)art.droplet.width, (float)art.droplet.height},
                   {20, 24 + hh, dw, dh}, {0, 0}, 0, WHITE);
    DrawText(TextFormat("%d/%d", DropletsCollected(lvl),
                        (int)lvl.droplets.size()),
             (int)(20 + dw + 8), (int)(26 + hh), (int)(H * 0.028f), RAYWHITE);
  }

  // Level name (top center)
  {
    const char *txt = TextFormat("%d - %s", currentLevelIndex + 1,
                                 LevelName(currentLevelIndex));
    int fs = (int)(H * 0.024f);
    DrawText(txt, (int)(W / 2 - MeasureText(txt, fs) / 2.0f), 16, fs,
             Fade(RAYWHITE, 0.85f));
  }

  // Day/night indicator (bottom right, backdrop keeps it readable anywhere)
  {
    float iconR = H * 0.018f;
    float ix = W - 150, iy = H - 30;
    DrawRectangle((int)(ix - iconR - 10), (int)(iy - iconR - 6), 160,
                  (int)(iconR * 2 + 12), Fade(BLACK, 0.45f));
    if (isDayTime)
      DrawCircleV({ix, iy}, iconR, Color{255, 220, 100, 255});
    else {
      float ms = iconR * 2.6f;
      DrawTexturePro(art.moon,
                     {0, 0, (float)art.moon.width, (float)art.moon.height},
                     {ix - ms / 2, iy - ms / 2, ms, ms}, {0, 0}, 0, WHITE);
    }
    DrawText(isDayTime ? "JOUR (T)" : "NUIT (T)", (int)(ix + iconR + 8),
             (int)(iy - 9), (int)(H * 0.024f), RAYWHITE);
  }
}

void Game::DrawTitle() {
  float W = (float)Core::SCREEN_WIDTH, H = (float)Core::SCREEN_HEIGHT;
  if (introScreenTex.id != 0) {
    DrawTexturePro(introScreenTex,
                   {0, 0, (float)introScreenTex.width,
                    (float)introScreenTex.height},
                   {0, 0, W, H}, {0, 0}, 0, WHITE);
  } else {
    ClearBackground(BLACK);
    DrawText("BEWARE THE SUN",
             (int)(W / 2 - MeasureText("BEWARE THE SUN", 40) / 2.0f),
             (int)(H / 3), 40, GOLD);
  }
  // Pulsing prompt
  float a = 0.6f + 0.4f * sinf((float)GetTime() * 3.0f);
  DrawText("PRESS ENTER TO START",
           (int)(W / 2 - MeasureText("PRESS ENTER TO START", 20) / 2.0f),
           (int)(H - 80), 20, Fade(WHITE, a));
}

void Game::DrawStory() {
  float W = (float)Core::SCREEN_WIDTH, H = (float)Core::SCREEN_HEIGHT;
  ClearBackground(BLACK);

  const char *storyText =
      "Dans un pot vit Petit Jasmin.\n\n"
      "Petit Jasmin vivait sa meilleure vie, arrose regulierement par un "
      "chill dude.\n"
      "Mais voila l'ete arrive, le soleil tape fort et la soif se fait "
      "ressentir.\n"
      "Depuis plusieurs jours, plus personne n'est la pour l'arroser.\n\n"
      "Il va falloir sortir de son pot et partir chercher de l'eau.\n"
      "Recolte toutes les gouttes pour remplir l'arrosoir de chaque "
      "jardin !\n\n"
      "Le jour, le soleil te brule : reste a l'ombre des plateformes.\n"
      "La nuit, les fleurs se fanent et les bestioles sortent...\n"
      "mais les champignons deviennent de formidables trampolines.\n\n"
      "Saute sur les bestioles pour t'en debarrasser. Bonne chance !";

  DrawText("L'HISTOIRE DE PETIT JASMIN",
           (int)(W / 2 - MeasureText("L'HISTOIRE DE PETIT JASMIN", 32) / 2.0f),
           25, 32, GOLD);
  DrawText(storyText, 80, 90, 20, WHITE);

  const char *controls = "Fleches / WASD : bouger    ESPACE : sauter    "
                         "T : jour-nuit    N : nouveau jardin    R : recommencer";
  DrawText(controls, (int)(W / 2 - MeasureText(controls, 16) / 2.0f),
           (int)(H - 70), 16, SKYBLUE);

  if (introImageTex.id != 0) {
    float imgScale = 0.55f;
    float drawW = introImageTex.width * imgScale;
    float drawH = introImageTex.height * imgScale;
    DrawTexturePro(introImageTex,
                   {0, 0, (float)introImageTex.width,
                    (float)introImageTex.height},
                   {W - drawW - 30, H - drawH - 90, drawW, drawH}, {0, 0}, 0,
                   WHITE);
  }

  float a = 0.6f + 0.4f * sinf((float)GetTime() * 3.0f);
  DrawText("PRESS ENTER TO PLAY",
           (int)(W / 2 - MeasureText("PRESS ENTER TO PLAY", 20) / 2.0f),
           (int)(H - 40), 20, Fade(GOLD, a));
}

void Game::DrawGameOver() {
  float W = (float)Core::SCREEN_WIDTH, H = (float)Core::SCREEN_HEIGHT;
  if (gameOverScreenTex.id != 0) {
    DrawTexturePro(gameOverScreenTex,
                   {0, 0, (float)gameOverScreenTex.width,
                    (float)gameOverScreenTex.height},
                   {0, 0, W, H}, {0, 0}, 0, WHITE);
  } else {
    DrawRectangle(0, 0, (int)W, (int)H, BLACK);
    DrawText("GAME OVER", (int)(W / 2 - MeasureText("GAME OVER", 60) / 2.0f),
             (int)(H / 3), 60, RED);
  }
  DrawText("PRESS 'R' TO RESTART",
           (int)(W / 2 - MeasureText("PRESS 'R' TO RESTART", 30) / 2.0f),
           (int)(H / 2), 30, WHITE);
}

void Game::DrawWin() {
  float W = (float)Core::SCREEN_WIDTH, H = (float)Core::SCREEN_HEIGHT;
  ClearBackground(Color{10, 30, 10, 255});

  int centerX = (int)(W / 2);
  DrawText("FELICITATIONS !",
           centerX - MeasureText("FELICITATIONS !", 48) / 2, (int)(H / 4), 48,
           GOLD);
  DrawText("Petit Jasmin a traverse les 7 jardins et trouve de l'eau !",
           centerX -
               MeasureText(
                   "Petit Jasmin a traverse les 7 jardins et trouve de l'eau !",
                   24) /
                   2,
           (int)(H / 4) + 80, 24, WHITE);
  DrawText("Merci d'avoir joue a Beware The Sun",
           centerX - MeasureText("Merci d'avoir joue a Beware The Sun", 20) / 2,
           (int)(H / 2), 20, LIGHTGRAY);

  if (introImageTex.id != 0) {
    float scale = 0.5f;
    float drawW = introImageTex.width * scale;
    float drawH = introImageTex.height * scale;
    DrawTexturePro(introImageTex,
                   {0, 0, (float)introImageTex.width,
                    (float)introImageTex.height},
                   {centerX - drawW / 2, H / 2 + 40, drawW, drawH}, {0, 0}, 0,
                   WHITE);
  }

  DrawText("PRESS ENTER TO RETURN TO MENU",
           centerX - MeasureText("PRESS ENTER TO RETURN TO MENU", 20) / 2,
           (int)(H - 60), 20, GOLD);
}

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------

void Game::ApplyResolution(int newW, int newH) {
  Core::SCREEN_WIDTH = newW;
  Core::SCREEN_HEIGHT = newH;
  Core::RecalculatePhysics();
  ApplyPlayerSize();
  GenerateAllLevels();
  LoadLevel(currentLevelIndex);
  if (previousScreen != GAMEPLAY)
    StopCurrentMusic(); // don't start level music from the menus
}

void Game::UpdateSettings() {
  const int ITEM_COUNT = 5;
  if (IsKeyPressed(KEY_UP))
    settingsSelection = (settingsSelection + ITEM_COUNT - 1) % ITEM_COUNT;
  if (IsKeyPressed(KEY_DOWN))
    settingsSelection = (settingsSelection + 1) % ITEM_COUNT;

  if (settingsSelection == 0) { // Resolution
    if (IsKeyPressed(KEY_LEFT))
      selectedResIndex = (selectedResIndex + RES_COUNT - 1) % RES_COUNT;
    if (IsKeyPressed(KEY_RIGHT))
      selectedResIndex = (selectedResIndex + 1) % RES_COUNT;
    if (IsKeyPressed(KEY_ENTER) && !isFullscreen) {
      SetWindowSize(resOptions[selectedResIndex].width,
                    resOptions[selectedResIndex].height);
      ApplyResolution(resOptions[selectedResIndex].width,
                      resOptions[selectedResIndex].height);
    }
  } else if (settingsSelection == 1) { // Music volume
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
      musicVolume = std::max(0.0f, musicVolume - 0.1f);
      ApplyVolume();
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
      musicVolume = std::min(1.0f, musicVolume + 0.1f);
      ApplyVolume();
    }
  } else if (settingsSelection == 2) { // SFX volume
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
      sfxVolume = std::max(0.0f, sfxVolume - 0.1f);
      ApplyVolume();
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
      sfxVolume = std::min(1.0f, sfxVolume + 0.1f);
      ApplyVolume();
    }
  } else if (settingsSelection == 3) { // Fullscreen
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_LEFT) ||
        IsKeyPressed(KEY_RIGHT)) {
      isFullscreen = !isFullscreen;
      if (isFullscreen) {
        int mon = GetCurrentMonitor();
        int mw = GetMonitorWidth(mon), mh = GetMonitorHeight(mon);
        SetWindowSize(mw, mh);
        ToggleFullscreen();
        ApplyResolution(mw, mh);
      } else {
        ToggleFullscreen();
        SetWindowSize(resOptions[selectedResIndex].width,
                      resOptions[selectedResIndex].height);
        ApplyResolution(resOptions[selectedResIndex].width,
                        resOptions[selectedResIndex].height);
      }
    }
  } else if (settingsSelection == 4) { // Back
    if (IsKeyPressed(KEY_ENTER))
      currentScreen = previousScreen;
  }

  if (IsKeyPressed(KEY_ESCAPE))
    currentScreen = previousScreen;
}

void Game::DrawSettings() {
  if (previousScreen == GAMEPLAY) {
    DrawGameplay();
    DrawRectangle(0, 0, Core::SCREEN_WIDTH, Core::SCREEN_HEIGHT,
                  Fade(BLACK, 0.75f));
  } else {
    ClearBackground(Color{20, 20, 30, 255});
  }

  int centerX = Core::SCREEN_WIDTH / 2;
  int startY = Core::SCREEN_HEIGHT / 4;
  int spacing = 50;
  int fontSize = 24;

  DrawText("SETTINGS", centerX - MeasureText("SETTINGS", 36) / 2, startY - 60,
           36, GOLD);

  const char *labels[] = {"Resolution", "Music Volume", "SFX Volume",
                          "Fullscreen", "Back"};

  for (int i = 0; i < 5; i++) {
    Color textColor = (i == settingsSelection) ? GOLD : LIGHTGRAY;
    int y = startY + i * spacing;

    if (i == settingsSelection)
      DrawText(">", centerX - 200, y, fontSize, GOLD);
    DrawText(labels[i], centerX - 170, y, fontSize, textColor);

    if (i == 0) {
      DrawText(TextFormat("< %s >", resOptions[selectedResIndex].label),
               centerX + 80, y, fontSize, textColor);
      if (i == settingsSelection)
        DrawText(isFullscreen ? "(Disable fullscreen first)"
                              : "(Press ENTER to apply)",
                 centerX + 80, y + 25, 14, DARKGRAY);
    } else if (i == 1 || i == 2) {
      float vol = (i == 1) ? musicVolume : sfxVolume;
      float barW = 150, barH = 20;
      float barX = (float)centerX + 80, barY = (float)y + 3;
      DrawRectangle((int)barX, (int)barY, (int)barW, (int)barH, DARKGRAY);
      DrawRectangle((int)barX, (int)barY, (int)(barW * vol), (int)barH,
                    (i == 1) ? GREEN : BLUE);
      DrawRectangleLines((int)barX, (int)barY, (int)barW, (int)barH, WHITE);
      DrawText(TextFormat("%d%%", (int)(vol * 100)),
               (int)(barX + barW + 10), y, fontSize, textColor);
    } else if (i == 3) {
      DrawText(isFullscreen ? "ON" : "OFF", centerX + 80, y, fontSize,
               isFullscreen ? GREEN : RED);
    }
  }

  const char *help = "UP/DOWN: Navigate  |  LEFT/RIGHT: Adjust  |  ESC: Back";
  DrawText(help, centerX - MeasureText(help, 14) / 2,
           Core::SCREEN_HEIGHT - 40, 14, GRAY);
}

// ---------------------------------------------------------------------------
// Unload
// ---------------------------------------------------------------------------

void Game::Unload() {
  if (isUnloaded)
    return;
  isUnloaded = true;

  enemies.clear();
  art.Unload();

  UnloadTexture(introScreenTex);
  UnloadTexture(introImageTex);
  UnloadTexture(gameOverScreenTex);

  UnloadSound(jumpSound);
  UnloadSound(walkSound);
  UnloadSound(burnSound);
  UnloadSound(deathSound);
  UnloadSound(wateringCanSound);

  if (audioLoaded) {
    currentPlayingMusic = nullptr;
    for (int i = 0; i < TRACK_COUNT; ++i) {
      UnloadMusicStream(dayTracks[i]);
      UnloadMusicStream(nightTracks[i]);
    }
    UnloadMusicStream(titleMusic);
  }

  levels.clear();
}
