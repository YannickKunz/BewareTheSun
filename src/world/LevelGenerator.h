#pragma once
#include "Level.h"

// Generates a solvable single-screen level.
// difficulty: 0-based level index; scales gaps, enemy count and hazards.
Level GenerateLevel(int screenW, int screenH, unsigned seed, int difficulty);

// Display name for a generated level (stable per difficulty index).
const char *LevelName(int difficulty);
