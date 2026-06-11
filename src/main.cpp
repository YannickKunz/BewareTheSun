#include "Game.h"
#include "core/Constants.h"
#include <cstdlib>

int main() {
  SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(Core::SCREEN_WIDTH, Core::SCREEN_HEIGHT, "Beware The Sun");
  SetExitKey(0); // ESC opens the settings menu instead of quitting
  SetTargetFPS(60);
  InitAudioDevice();

  Game game;
  game.Init();

  // Screenshot self-test: BTS_SHOT=<level> renders day + night then exits
  const char *shot = std::getenv("BTS_SHOT");
  if (shot != nullptr)
    game.DebugStartLevel(std::atoi(shot));
  int frame = 0;

  while (!WindowShouldClose()) {
    game.Update();
    game.Draw();

    if (shot != nullptr) {
      frame++;
      if (frame == 60)
        TakeScreenshot("shot_day.png");
      if (frame == 61)
        game.DebugToggleNight();
      if (frame == 120)
        TakeScreenshot("shot_night.png");
      if (frame > 125)
        break;
    }
  }

  game.Unload();
  CloseAudioDevice();
  CloseWindow();
  return 0;
}
