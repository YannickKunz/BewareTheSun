#pragma once

enum class PlatformType {
  NORMAL,    // Always solid
  FLOWER,    // Solid during the day; rises at day, sinks at night
  MUSHROOM,  // Solid + bouncy during the night
  INVISIBLE  // Always solid, never drawn (world borders)
};

enum class EnemyType { SPIDER, ROACH };
