#pragma once
#include "entities/Enemy.h"
#include "entities/Player.h"
#include "gfx/ProcArt.h"
#include "raylib.h"
#include "world/Level.h"
#include <memory>
#include <vector>

class Game {
public:
  Game() = default;
  ~Game();

  void Init();
  void Update();
  void Draw();
  void Unload();

  // Test hooks (used by main when the BTS_SHOT env var is set)
  void DebugStartLevel(int index);
  void DebugToggleNight();

private:
  // --- Game state ---
  bool isDayTime = true;
  float dayBlend = 1.0f; // visual crossfade: 1 = day, 0 = night
  bool debugMode = false;
  bool isUnloaded = false;
  bool sunHintShown = false;
  float sunHintTimer = 0.0f;
  float toggleCooldown = 0.0f;

  Player player;
  bool deathStarted = false;
  float deathTimer = 0.0f;

  // --- Levels (procedurally generated) ---
  std::vector<Level> levels;
  std::vector<unsigned> seeds;
  int currentLevelIndex = 0;
  std::vector<std::unique_ptr<Enemy>> enemies;

  void GenerateAllLevels();
  void RegenerateCurrentLevel(bool newSeed);
  void LoadLevel(int index);

  // --- Level transition / banner ---
  float fade = 0.0f;
  bool fadingOut = false;
  int pendingLevel = -1; // LEVEL_COUNT means "win"
  float bannerTimer = 0.0f;

  // --- Particles & screen shake ---
  struct Particle {
    Vector2 pos, vel;
    float life, maxLife, size;
    Color color;
    bool gravity;
  };
  std::vector<Particle> particles;
  void Emit(Vector2 pos, int count, Color color, float speed, bool gravity,
            float life = 0.6f);
  void UpdateParticles(float dt);
  void DrawParticles() const;
  float shakeTime = 0.0f;
  float shakeMag = 0.0f;
  void Shake(float time, float mag);

  float walkSoundTimer = 0.0f;
  float burnSoundTimer = 0.0f;

  // --- Screens ---
  enum GameScreen { TITLE, STORY, GAMEPLAY, SETTINGS, WIN, GAME_OVER };
  GameScreen currentScreen = TITLE;
  GameScreen previousScreen = TITLE;

  void UpdateTitle();
  void UpdateStory();
  void UpdateGameplay();
  void UpdateGameOver();
  void UpdateSettings();
  void UpdateWin();

  void DrawTitle();
  void DrawStory();
  void DrawGameplay();
  void DrawGameOver();
  void DrawSettings();
  void DrawWin();
  void DrawHUD();

  // --- Settings ---
  float masterVolume = 1.0f;
  float musicVolume = 0.7f;
  float sfxVolume = 1.0f;
  bool isFullscreen = false;
  int selectedResIndex = 2; // 1280x800
  int settingsSelection = 0;

  struct ResOption {
    int width;
    int height;
    const char *label;
  };
  static constexpr int RES_COUNT = 4;
  ResOption resOptions[RES_COUNT] = {
      {800, 600, "800x600"},
      {1024, 768, "1024x768"},
      {1280, 800, "1280x800"},
      {1920, 1080, "1920x1080"},
  };

  void ApplyVolume();
  void ApplyResolution(int newW, int newH);
  void ApplyPlayerSize();

  // --- Art ---
  ProcArt art; // everything in-level is generated procedurally
  Texture2D introScreenTex{};
  Texture2D introImageTex{};
  Texture2D gameOverScreenTex{};

  float flowerAnimTimer = 0.0f;
  int flowerFrame = 0;

  // --- Audio ---
  Sound jumpSound{}, walkSound{}, burnSound{}, deathSound{},
      wateringCanSound{};
  static constexpr int TRACK_COUNT = 3;
  Music dayTracks[TRACK_COUNT]{};
  Music nightTracks[TRACK_COUNT]{};
  Music titleMusic{};
  bool audioLoaded = false;
  Music *currentPlayingMusic = nullptr; // points into the arrays above

  void PlayLevelMusic();
  void StopCurrentMusic();

  // --- Helpers ---
  int DropletsCollected(const Level &lvl) const;
  bool ExitUnlocked(const Level &lvl) const;
};
