#include "ProcArt.h"
#include <cmath>
#include <random>

// ---------------------------------------------------------------------------
// Small drawing helpers
// ---------------------------------------------------------------------------
namespace {

void Px(Image &img, int x, int y, Color c) {
  if (x >= 0 && y >= 0 && x < img.width && y < img.height)
    ImageDrawPixel(&img, x, y, c);
}

void RectI(Image &img, int x, int y, int w, int h, Color c) {
  ImageDrawRectangle(&img, x, y, w, h, c);
}

void DiskI(Image &img, int cx, int cy, int r, Color c) {
  ImageDrawCircle(&img, cx, cy, r, c);
}

void LineI(Image &img, int x1, int y1, int x2, int y2, Color c) {
  ImageDrawLine(&img, x1, y1, x2, y2, c);
}

Color Mul(Color c, float f) {
  auto cl = [](float v) {
    return (unsigned char)(v < 0 ? 0 : (v > 255 ? 255 : v));
  };
  return {cl(c.r * f), cl(c.g * f), cl(c.b * f), c.a};
}

Color LerpC(Color a, Color b, float t) {
  auto l = [&](unsigned char x, unsigned char y) {
    return (unsigned char)(x + (y - x) * t);
  };
  return {l(a.r, b.r), l(a.g, b.g), l(a.b, b.b), l(a.a, b.a)};
}

Texture2D MakeTexture(Image &img, bool pixelArt = true) {
  Texture2D t = LoadTextureFromImage(img);
  SetTextureFilter(t, pixelArt ? TEXTURE_FILTER_POINT
                               : TEXTURE_FILTER_BILINEAR);
  UnloadImage(img);
  return t;
}

// ---------------------------------------------------------------------------
// Sprite painters
// ---------------------------------------------------------------------------

// Petit Jasmin: 20x24 frame. sway: top lean in px, bounce: vertical hop.
void DrawPlantFrame(Image &img, int ox, int sway, int bounce, bool dead) {
  int cx = ox + 10;
  int oy = bounce;

  Color pot = {196, 99, 58, 255};
  Color potDark = {158, 74, 42, 255};
  Color rim = {216, 120, 74, 255};
  Color stem = {62, 137, 72, 255};
  Color leaf = {88, 170, 94, 255};
  Color petal = {248, 246, 240, 255};
  Color heart = {255, 214, 90, 255};

  if (!dead) {
    // Stem (leans by 'sway' toward the top)
    for (int yy = 6; yy <= 14; ++yy) {
      float t = (14 - yy) / 8.0f;
      int dx = (int)lroundf(sway * t);
      RectI(img, cx - 1 + dx, yy + oy, 2, 1, stem);
    }
    // Leaves
    int ldx = sway / 2;
    RectI(img, cx - 6 + ldx, 10 + oy, 4, 2, leaf);
    Px(img, cx - 6 + ldx, 9 + oy, leaf);
    RectI(img, cx + 2 + ldx, 8 + oy, 4, 2, leaf);
    Px(img, cx + 5 + ldx, 7 + oy, leaf);
    // Jasmine head: 5 white petals around a golden heart
    int hx = cx + sway, hy = 4 + oy;
    DiskI(img, hx, hy - 2, 2, petal);
    DiskI(img, hx - 2, hy, 2, petal);
    DiskI(img, hx + 2, hy, 2, petal);
    DiskI(img, hx - 1, hy + 2, 2, petal);
    DiskI(img, hx + 1, hy + 2, 2, petal);
    DiskI(img, hx, hy, 1, heart);
  } else {
    // Wilted: stem bends over, gray petals droop
    int bend[][2] = {{0, 13}, {0, 12}, {1, 11}, {2, 10},
                     {3, 10}, {4, 11}, {5, 12}};
    Color deadStem = {122, 122, 84, 255};
    for (auto &p : bend)
      RectI(img, cx - 1 + p[0], p[1], 2, 1, deadStem);
    Color deadPetal = {178, 178, 168, 255};
    DiskI(img, cx + 6, 13, 2, deadPetal);
    Px(img, cx + 6, 13, {158, 148, 92, 255});
    Px(img, cx - 4, 11, deadPetal); // fallen petal
    Px(img, cx - 6, 14, deadPetal);
  }

  // Terracotta pot (drawn last so the stem sits "inside")
  RectI(img, cx - 7, 14, 14, 2, rim);
  Px(img, cx - 7, 15, potDark);
  Px(img, cx + 6, 15, potDark);
  for (int yy = 16; yy <= 23; ++yy) {
    int half = 6 - (yy - 16) / 3; // gentle taper
    RectI(img, cx - half, yy, half * 2, 1, pot);
    Px(img, cx - half, yy, potDark);
    Px(img, cx + half - 1, yy, potDark);
  }
  // Face on the pot
  if (!dead) {
    Px(img, cx - 3, 18, BLACK);
    Px(img, cx + 2, 18, BLACK);
    RectI(img, cx - 1, 20, 2, 1, {96, 44, 26, 255});
  } else {
    RectI(img, cx - 4, 18, 3, 1, BLACK); // closed eyes
    RectI(img, cx + 1, 18, 3, 1, BLACK);
    RectI(img, cx - 2, 21, 4, 1, {96, 44, 26, 255}); // frown
    Px(img, cx - 2, 20, {96, 44, 26, 255});
    Px(img, cx + 1, 20, {96, 44, 26, 255});
  }
}

// Spider: 16x14 frame, hangs upside-down on a silk thread.
void DrawSpiderFrame(Image &img, int ox, int f) {
  int cx = ox + 8, cy = 7;
  Color body = {32, 27, 38, 255};
  Color leg = {52, 45, 60, 255};
  Color mark = {138, 52, 148, 255};
  Color eye = {235, 64, 64, 255};

  for (int i = 0; i < 4; ++i) {
    int ex = (i < 2) ? 1 : 2;
    int ey = 2 + i * 3 + ((i + f) % 2);
    LineI(img, cx, cy, ox + ex, ey, leg);
    LineI(img, cx, cy, ox + 15 - ex, ey, leg);
  }
  DiskI(img, cx, cy, 4, body);          // abdomen
  Px(img, cx, cy - 1, mark);            // violin marking
  Px(img, cx, cy - 2, mark);
  Px(img, cx - 1, cy - 1, mark);
  DiskI(img, cx, cy + 4, 2, Mul(body, 1.4f)); // head (hangs downward)
  Px(img, cx - 1, cy + 5, eye);
  Px(img, cx + 1, cy + 5, eye);
}

// Roach: 20x10 frame, faces right.
void DrawRoachFrame(Image &img, int ox, int f) {
  Color shell = {124, 75, 34, 255};
  Color dark = {84, 50, 22, 255};
  Color hi = {168, 110, 56, 255};

  // Legs (alternate per frame)
  for (int i = 0; i < 3; ++i) {
    int lx = ox + 5 + i * 4 + ((i + f) % 2);
    LineI(img, lx, 7, lx - 1, 9, dark);
  }
  // Body
  int cx = ox + 9;
  int halfw[6] = {4, 6, 7, 7, 6, 4};
  for (int r = 0; r < 6; ++r)
    RectI(img, cx - halfw[r], 2 + r, halfw[r] * 2, 1, shell);
  RectI(img, cx - 4, 3, 6, 1, hi);          // shine
  LineI(img, cx - 5, 5, cx + 5, 5, dark);   // wing split
  DiskI(img, ox + 16, 4, 2, dark);          // head
  Px(img, ox + 16, 3, {240, 220, 160, 255}); // eye
  LineI(img, ox + 17, 3, ox + 19, 0, dark); // antennae
  LineI(img, ox + 17, 4, ox + 19, 2, dark);
}

// Flower platform: 28x36 frame; the petal disc on top IS the platform.
void DrawFlowerFrame(Image &img, int ox, int f, int frames) {
  int dx = (int)lroundf(2.4f * sinf(2.0f * PI * f / (float)frames));
  int baseX = ox + 14;
  int cx = baseX + dx;

  Color stem = {58, 130, 70, 255};
  Color leaf = {84, 168, 90, 255};
  Color petal = {242, 167, 195, 255};
  Color petalDark = {222, 130, 166, 255};
  Color center = {255, 214, 90, 255};

  // Stem sways with the disc
  for (int yy = 9; yy <= 35; ++yy) {
    float t = (35 - yy) / 27.0f;
    int sx = baseX + (int)lroundf(dx * t);
    RectI(img, sx - 1, yy, 2, 1, stem);
  }
  RectI(img, baseX - 6 + dx / 2, 21, 4, 2, leaf);
  Px(img, baseX - 7 + dx / 2, 20, leaf);
  RectI(img, baseX + 3 + dx / 2, 16, 4, 2, leaf);
  Px(img, baseX + 7 + dx / 2, 15, leaf);

  // Petal disc (rows y 3..9)
  int halfw[7] = {8, 11, 12, 12, 12, 11, 8};
  for (int r = 0; r < 7; ++r)
    RectI(img, cx - halfw[r], 3 + r, halfw[r] * 2,
          1, (r >= 5) ? petalDark : petal);
  for (int k = -2; k <= 2; ++k) // petal separations
    if (k != 0)
      LineI(img, cx + k * 5, 4, cx + k * 4, 8, petalDark);
  DiskI(img, cx, 6, 3, center);
  Px(img, cx - 1, 5, {255, 240, 160, 255});
}

void DrawMushroom(Image &img, bool night) {
  Color stalk = {232, 217, 176, 255};
  Color stalkSh = {196, 178, 134, 255};
  Color cap = night ? Color{46, 196, 182, 255} : Color{192, 58, 43, 255};
  Color capDark = Mul(cap, 0.72f);
  Color spot = night ? Color{220, 255, 250, 255} : Color{246, 240, 230, 255};

  // Stalk
  RectI(img, 8, 10, 4, 9, stalk);
  RectI(img, 8, 10, 1, 9, stalkSh);
  // Cap dome (rows y 3..10)
  int halfw[8] = {3, 6, 8, 9, 9, 9, 8, 8};
  for (int r = 0; r < 8; ++r)
    RectI(img, 10 - halfw[r], 3 + r, halfw[r] * 2, 1,
          (r >= 6) ? capDark : cap);
  // Spots
  DiskI(img, 6, 6, 1, spot);
  DiskI(img, 13, 5, 1, spot);
  Px(img, 10, 8, spot);
  Px(img, 16, 8, spot);
  if (night) { // glow freckles
    Px(img, 4, 9, spot);
    Px(img, 9, 4, spot);
    Px(img, 15, 9, spot);
  }
}

void DrawTile(Image &img, std::mt19937 &rng, bool night, bool top) {
  Color grass = night ? Color{52, 84, 92, 255} : Color{88, 161, 78, 255};
  Color grassHi = night ? Color{78, 205, 196, 255} : Color{124, 196, 96, 255};
  Color dirt = night ? Color{52, 39, 30, 255} : Color{107, 74, 47, 255};
  Color dirtDark = Mul(dirt, 0.78f);
  Color dirtHi = Mul(dirt, 1.25f);

  RectI(img, 0, 0, 16, 16, dirt);
  std::uniform_int_distribution<int> d16(0, 15), d100(0, 99);
  for (int i = 0; i < 26; ++i) { // speckle
    int x = d16(rng), y = d16(rng);
    Px(img, x, y, (d100(rng) < 50) ? dirtDark : dirtHi);
  }
  if (top) {
    RectI(img, 0, 0, 16, 4, grass);
    for (int x = 0; x < 16; ++x) { // ragged grass edge + blades
      if (d100(rng) < 45)
        Px(img, x, 4, grass);
      if (d100(rng) < 30)
        Px(img, x, d100(rng) < 50 ? 0 : 1, grassHi);
    }
  }
}

void DrawWateringCan(Image &img, bool night) {
  Color metal = night ? Color{96, 112, 128, 255} : Color{159, 180, 199, 255};
  Color metalDark = Mul(metal, 0.7f);
  Color metalHi = Mul(metal, 1.25f);
  Color water = {62, 146, 204, 255};

  // Body (x 6..21, y 7..18)
  RectI(img, 6, 7, 16, 12, metal);
  RectI(img, 6, 7, 16, 1, metalHi);
  RectI(img, 6, 18, 16, 1, metalDark);
  RectI(img, 21, 7, 1, 12, metalDark);
  RectI(img, 7, 11, 14, 2, water); // water level band
  // Spout (toward upper-left) + rose
  LineI(img, 6, 10, 1, 4, metalDark);
  LineI(img, 7, 10, 2, 4, metal);
  LineI(img, 8, 11, 3, 5, metal);
  RectI(img, 0, 2, 4, 3, metalDark);
  // Handle arc on top
  for (int x = 9; x <= 19; ++x) {
    float t = (x - 9) / 10.0f;
    int y = (int)lroundf(2.5f - 2.3f * sinf(t * PI));
    Px(img, x, y + 2, metalDark);
    Px(img, x, y + 3, metal);
  }
  // Droplets leaking from the rose
  Px(img, 1, 7, water);
  Px(img, 3, 9, water);
}

void DrawDroplet(Image &img) {
  Color blue = {79, 195, 247, 255};
  Color deep = {41, 142, 199, 255};
  DiskI(img, 4, 8, 3, blue);
  // Tapered tip
  RectI(img, 3, 3, 2, 2, blue);
  RectI(img, 3, 5, 3, 2, blue);
  Px(img, 4, 1, blue);
  Px(img, 4, 2, blue);
  Px(img, 5, 9, deep);
  Px(img, 6, 8, deep);
  Px(img, 3, 7, WHITE); // highlight
  Px(img, 3, 8, WHITE);
}

void DrawHeart(Image &img) {
  const char *rows[6] = {".XX.XX.", "XXXXXXX", "XXXXXXX",
                         ".XXXXX.", "..XXX..", "...X..."};
  Color red = {224, 58, 75, 255};
  for (int y = 0; y < 6; ++y)
    for (int x = 0; x < 7; ++x)
      if (rows[y][x] == 'X')
        Px(img, x, y, red);
  Px(img, 1, 1, {255, 150, 160, 255}); // shine
}

void DrawMoon(Image &img) {
  Color pale = {236, 235, 215, 255};
  Color crater = {204, 202, 178, 255};
  int cx = 14, cy = 14, r = 11;
  for (int y = 0; y < 28; ++y)
    for (int x = 0; x < 28; ++x) {
      float d1 = sqrtf((float)((x - cx) * (x - cx) + (y - cy) * (y - cy)));
      float d2 = sqrtf((float)((x - cx - 6) * (x - cx - 6) +
                               (y - cy + 3) * (y - cy + 3)));
      if (d1 <= r && d2 > r - 1)
        Px(img, x, y, pale);
    }
  Px(img, 8, 10, crater);
  Px(img, 7, 16, crater);
  Px(img, 10, 19, crater);
}

// ---------------------------------------------------------------------------
// Backgrounds
// ---------------------------------------------------------------------------

struct BiomePal {
  Color daySkyTop, daySkyBot, dayFar, dayNear;
  Color nightSkyTop, nightSkyBot, nightFar, nightNear;
};

// One palette per level: garden, greenhouse, rooftops, ivy wall, dry quarry,
// windy heights, final ascent.
const BiomePal kBiomes[ProcArt::BIOME_COUNT] = {
    {{118, 190, 232, 255}, {214, 238, 210, 255}, {126, 178, 122, 255}, {86, 142, 92, 255},
     {16, 22, 48, 255}, {44, 58, 94, 255}, {38, 52, 74, 255}, {26, 38, 56, 255}},
    {{132, 206, 196, 255}, {226, 240, 214, 255}, {116, 172, 140, 255}, {78, 132, 104, 255},
     {12, 28, 42, 255}, {36, 70, 78, 255}, {30, 56, 62, 255}, {20, 42, 48, 255}},
    {{244, 176, 110, 255}, {252, 226, 168, 255}, {196, 124, 92, 255}, {148, 90, 74, 255},
     {30, 18, 44, 255}, {78, 44, 78, 255}, {56, 36, 64, 255}, {40, 26, 50, 255}},
    {{150, 200, 140, 255}, {222, 238, 190, 255}, {106, 156, 96, 255}, {70, 116, 70, 255},
     {14, 26, 34, 255}, {38, 62, 56, 255}, {30, 50, 44, 255}, {22, 38, 34, 255}},
    {{232, 198, 142, 255}, {248, 232, 198, 255}, {198, 158, 110, 255}, {158, 120, 82, 255},
     {26, 20, 36, 255}, {66, 50, 66, 255}, {52, 40, 50, 255}, {38, 30, 40, 255}},
    {{142, 178, 232, 255}, {222, 232, 246, 255}, {142, 158, 196, 255}, {104, 122, 164, 255},
     {10, 16, 40, 255}, {36, 46, 86, 255}, {30, 38, 68, 255}, {22, 28, 52, 255}},
    {{196, 150, 222, 255}, {248, 214, 188, 255}, {150, 112, 168, 255}, {110, 80, 130, 255},
     {22, 10, 38, 255}, {64, 32, 84, 255}, {48, 26, 64, 255}, {34, 20, 48, 255}},
};

Texture2D GenBackground(int biome, bool night, unsigned seed) {
  const int W = 480, H = 270;
  const BiomePal &p = kBiomes[biome % ProcArt::BIOME_COUNT];
  Color skyTop = night ? p.nightSkyTop : p.daySkyTop;
  Color skyBot = night ? p.nightSkyBot : p.daySkyBot;
  Color far = night ? p.nightFar : p.dayFar;
  Color near = night ? p.nightNear : p.dayNear;

  std::mt19937 rng(seed);
  auto frand = [&](float a, float b) {
    std::uniform_real_distribution<float> d(a, b);
    return d(rng);
  };

  Image img = GenImageColor(W, H, BLANK);

  // Sky gradient
  for (int y = 0; y < H; ++y) {
    Color c = LerpC(skyTop, skyBot, (float)y / (H - 1));
    RectI(img, 0, y, W, 1, c);
  }

  // Stars (night only)
  if (night) {
    std::uniform_int_distribution<int> dx(0, W - 1), dy(0, H * 2 / 3);
    for (int i = 0; i < 110; ++i) {
      int x = dx(rng), y = dy(rng);
      Color s = (i % 7 == 0) ? Color{255, 244, 196, 255}
                             : Color{226, 232, 246, 230};
      Px(img, x, y, s);
      if (i % 11 == 0) { // brighter cross star
        Px(img, x + 1, y, Fade(s, 0.6f));
        Px(img, x - 1, y, Fade(s, 0.6f));
        Px(img, x, y + 1, Fade(s, 0.6f));
        Px(img, x, y - 1, Fade(s, 0.6f));
      }
    }
  }

  // Two layers of rolling hills (sine-sum silhouettes)
  float f1 = frand(0.008f, 0.016f), f2 = frand(0.03f, 0.05f);
  float p1 = frand(0, 6.28f), p2 = frand(0, 6.28f);
  for (int x = 0; x < W; ++x) {
    int yh = (int)(H * 0.55f + 26 * sinf(x * f1 + p1) + 11 * sinf(x * f2 + p2));
    RectI(img, x, yh, 1, H - yh, far);
  }
  float g1 = frand(0.010f, 0.02f), g2 = frand(0.04f, 0.06f);
  float q1 = frand(0, 6.28f), q2 = frand(0, 6.28f);
  for (int x = 0; x < W; ++x) {
    int yh = (int)(H * 0.74f + 20 * sinf(x * g1 + q1) + 9 * sinf(x * g2 + q2));
    RectI(img, x, yh, 1, H - yh, near);
  }

  // Clouds (day only; ImageDraw* doesn't alpha-blend, so pick a solid color
  // close to the sky instead of a translucent one)
  if (!night) {
    for (int i = 0; i < 4; ++i) {
      int cx = (int)frand(20, W - 40);
      int cy = (int)frand(16, H * 0.35f);
      Color sky = LerpC(skyTop, skyBot, (float)cy / (H - 1));
      Color cc = LerpC(sky, WHITE, 0.8f);
      int r = (int)frand(7, 13);
      DiskI(img, cx, cy, r, cc);
      DiskI(img, cx + r, cy + 2, r - 2, cc);
      DiskI(img, cx - r + 2, cy + 3, r - 3, cc);
      DiskI(img, cx + 4, cy - 3, r - 3, cc);
    }
  }

  return MakeTexture(img, false); // bilinear: soft painted look when scaled
}

} // namespace

// ---------------------------------------------------------------------------
// ProcArt
// ---------------------------------------------------------------------------

void ProcArt::Load() {
  // Player: idle, 4-frame walk, death
  {
    Image idle = GenImageColor(20, 24, BLANK);
    DrawPlantFrame(idle, 0, 0, 0, false);
    playerIdle = MakeTexture(idle);

    int sway[4] = {0, 2, 0, -2};
    int bounce[4] = {0, -1, 0, -1};
    Image walk = GenImageColor(20 * playerWalkFrames, 24, BLANK);
    for (int f = 0; f < playerWalkFrames; ++f)
      DrawPlantFrame(walk, f * 20, sway[f], bounce[f], false);
    playerWalk = MakeTexture(walk);

    Image death = GenImageColor(20, 24, BLANK);
    DrawPlantFrame(death, 0, 0, 0, true);
    playerDeath = MakeTexture(death);
  }

  // Enemies
  {
    Image sp = GenImageColor(16 * spiderFrames, 14, BLANK);
    for (int f = 0; f < spiderFrames; ++f)
      DrawSpiderFrame(sp, f * 16, f);
    spider = MakeTexture(sp);

    Image ro = GenImageColor(20 * roachFrames, 10, BLANK);
    for (int f = 0; f < roachFrames; ++f)
      DrawRoachFrame(ro, f * 20, f);
    roach = MakeTexture(ro);
  }

  // Flower platform sheet
  {
    Image fl = GenImageColor(28 * flowerFrames, 36, BLANK);
    for (int f = 0; f < flowerFrames; ++f)
      DrawFlowerFrame(fl, f * 28, f, flowerFrames);
    flower = MakeTexture(fl);
  }

  // Mushrooms
  {
    Image d = GenImageColor(20, 20, BLANK);
    DrawMushroom(d, false);
    mushroomDay = MakeTexture(d);
    Image n = GenImageColor(20, 20, BLANK);
    DrawMushroom(n, true);
    mushroomNight = MakeTexture(n);
  }

  // Ground tiles (top row with grass, fill rows without)
  {
    std::mt19937 rng(1337);
    Image a = GenImageColor(16, 16, BLANK);
    DrawTile(a, rng, false, true);
    tileDayTop = MakeTexture(a);
    Image b = GenImageColor(16, 16, BLANK);
    DrawTile(b, rng, false, false);
    tileDayFill = MakeTexture(b);
    Image c = GenImageColor(16, 16, BLANK);
    DrawTile(c, rng, true, true);
    tileNightTop = MakeTexture(c);
    Image d = GenImageColor(16, 16, BLANK);
    DrawTile(d, rng, true, false);
    tileNightFill = MakeTexture(d);
  }

  // Watering can (exit), droplet, heart, moon
  {
    Image cd = GenImageColor(26, 20, BLANK);
    DrawWateringCan(cd, false);
    canDay = MakeTexture(cd);
    Image cn = GenImageColor(26, 20, BLANK);
    DrawWateringCan(cn, true);
    canNight = MakeTexture(cn);

    Image dr = GenImageColor(8, 12, BLANK);
    DrawDroplet(dr);
    droplet = MakeTexture(dr);

    Image he = GenImageColor(7, 6, BLANK);
    DrawHeart(he);
    heart = MakeTexture(he);

    Image mo = GenImageColor(28, 28, BLANK);
    DrawMoon(mo);
    moon = MakeTexture(mo);
  }

  // Sun glow (radial gradient sprite)
  {
    Image g = GenImageGradientRadial(128, 128, 0.0f,
                                     Color{255, 236, 150, 255},
                                     Color{255, 200, 60, 0});
    sunGlow = MakeTexture(g, false);
  }

  // Backgrounds per biome
  for (int i = 0; i < BIOME_COUNT; ++i) {
    bgDay[i] = GenBackground(i, false, 900 + i * 17);
    bgNight[i] = GenBackground(i, true, 1700 + i * 31);
  }
}

void ProcArt::Unload() {
  Texture2D all[] = {playerIdle, playerWalk, playerDeath, spider,   roach,
                     flower,     mushroomDay, mushroomNight, tileDayTop,
                     tileDayFill, tileNightTop, tileNightFill, canDay,
                     canNight,   droplet,    heart,        sunGlow,  moon};
  for (auto &t : all)
    if (t.id != 0)
      UnloadTexture(t);
  for (int i = 0; i < BIOME_COUNT; ++i) {
    if (bgDay[i].id != 0)
      UnloadTexture(bgDay[i]);
    if (bgNight[i].id != 0)
      UnloadTexture(bgNight[i]);
  }
}
