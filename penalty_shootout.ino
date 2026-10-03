#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <math.h>

// =====================================================
// TFT
// =====================================================

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1
#define TFT_BL   21

// =====================================================
// TOUCH
// =====================================================

#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_CLK  25
#define TOUCH_CS   33

#define TOUCH_MIN_X 200
#define TOUCH_MAX_X 3900
#define TOUCH_MIN_Y 200
#define TOUCH_MAX_Y 3900

// =====================================================
// BUZZER
// =====================================================

#define BUZZER_PIN 22

// =====================================================
// SCREEN
// =====================================================

#define SCREEN_W 320
#define SCREEN_H 240

// =====================================================
// HARDWARE
// =====================================================

SPIClass tftSPI(HSPI);
SPIClass touchSPI(VSPI);

Adafruit_ILI9341 tft(
  &tftSPI,
  TFT_DC,
  TFT_CS,
  TFT_RST
);

XPT2046_Touchscreen ts(TOUCH_CS);

// =====================================================
// COLORS
// =====================================================

#define SKY         0x4DDF
#define GRASS       0x0540
#define GRASS_LIGHT 0x06A0
#define NET_COLOR   0xCE59
#define CROWD       0x4208

// =====================================================
// GAME STATE
// =====================================================

enum GameState {
  TITLE,
  READY,
  SHOOTING,
  RESULT,
  GAME_OVER
};

GameState gameState = TITLE;

// =====================================================
// GAME DATA
// =====================================================

int penaltyNumber = 1;
int goals = 0;

const int MAX_PENALTIES = 5;

// =====================================================
// KEEPER
// =====================================================

float keeperX = 160;

// faster than before
float keeperSpeed = 3.4;

int oldKeeperX = 160;

unsigned long lastKeeperFrame = 0;

// random direction changes
unsigned long keeperDirectionChangeAt = 0;

// dive system
bool keeperDiving = false;

float keeperDiveStartX = 160;
float keeperDiveTargetX = 160;

float keeperDiveProgress = 0;

int keeperDiveDirection = 0;

// -1 left
//  0 center
//  1 right

// =====================================================
// BALL
// =====================================================

float ballX = 160;
float ballY = 215;

float oldBallX = 160;
float oldBallY = 215;

float targetX = 160;
float targetY = 120;

float shotProgress = 0;

unsigned long lastBallFrame = 0;

// =====================================================
// RESULT
// =====================================================

bool lastShotGoal = false;

unsigned long resultStarted = 0;

bool resultDrawn = false;

// =====================================================
// TOUCH
// =====================================================

bool touchWasDown = false;

// =====================================================
// TITLE MUSIC
// =====================================================

const int titleNotes[] = {

  523, 659, 784, 1047,
  784, 659, 523,

  587, 698, 880, 1175,
  880, 698
};

const int titleDurations[] = {

  120,120,150,240,
  120,120,200,

  120,120,150,240,
  120,180
};

const int TITLE_NOTE_COUNT =
  sizeof(titleNotes) /
  sizeof(titleNotes[0]);

int titleNote = 0;

unsigned long titleNoteStarted = 0;

bool titleMusicPlaying = false;

// =====================================================
// SOUND
// =====================================================

void soundOff() {

  ledcWriteTone(
    BUZZER_PIN,
    0
  );
}

// -----------------------------------------------------
// TITLE MUSIC
// -----------------------------------------------------

void startTitleMusic() {

  titleNote = 0;

  titleNoteStarted =
    millis();

  titleMusicPlaying =
    true;

  ledcWriteTone(
    BUZZER_PIN,
    titleNotes[0]
  );
}

void stopTitleMusic() {

  titleMusicPlaying =
    false;

  soundOff();
}

void updateTitleMusic() {

  if (!titleMusicPlaying)
    return;

  unsigned long now =
    millis();

  if (
    now - titleNoteStarted >=
    titleDurations[titleNote]
  ) {

    titleNote++;

    if (
      titleNote >=
      TITLE_NOTE_COUNT
    ) {

      titleNote = 0;
    }

    ledcWriteTone(
      BUZZER_PIN,
      titleNotes[titleNote]
    );

    titleNoteStarted =
      now;
  }
}

// -----------------------------------------------------
// KICK
// -----------------------------------------------------

void soundKick() {

  ledcWriteTone(
    BUZZER_PIN,
    190
  );

  delay(35);

  ledcWriteTone(
    BUZZER_PIN,
    110
  );

  delay(25);

  soundOff();
}

// -----------------------------------------------------
// GOAL
// -----------------------------------------------------

void soundGoal() {

  int notes[] = {
    700,
    950,
    1200,
    1500,
    1900
  };

  for (
    int i = 0;
    i < 5;
    i++
  ) {

    ledcWriteTone(
      BUZZER_PIN,
      notes[i]
    );

    delay(70);
  }

  soundOff();
}

// -----------------------------------------------------
// SAVE
// -----------------------------------------------------

void soundSave() {

  ledcWriteTone(
    BUZZER_PIN,
    500
  );

  delay(100);

  ledcWriteTone(
    BUZZER_PIN,
    300
  );

  delay(120);

  ledcWriteTone(
    BUZZER_PIN,
    180
  );

  delay(160);

  soundOff();
}

// -----------------------------------------------------
// START
// -----------------------------------------------------

void soundStart() {

  int notes[] = {
    523,
    659,
    784,
    1047
  };

  for (
    int i = 0;
    i < 4;
    i++
  ) {

    ledcWriteTone(
      BUZZER_PIN,
      notes[i]
    );

    delay(80);
  }

  soundOff();
}

// -----------------------------------------------------
// PERFECT
// -----------------------------------------------------

void soundPerfect() {

  int notes[] = {

    523,
    659,
    784,
    1047,

    784,
    1047,
    1319,
    1568,

    1319,
    1568,
    2093
  };

  int durations[] = {

    90,
    90,
    90,
    180,

    90,
    90,
    100,
    200,

    100,
    100,
    400
  };

  for (
    int i = 0;
    i < 11;
    i++
  ) {

    ledcWriteTone(
      BUZZER_PIN,
      notes[i]
    );

    delay(
      durations[i]
    );

    soundOff();

    delay(20);
  }

  // final victory flourish

  ledcWriteTone(
    BUZZER_PIN,
    1047
  );

  delay(100);

  ledcWriteTone(
    BUZZER_PIN,
    1319
  );

  delay(100);

  ledcWriteTone(
    BUZZER_PIN,
    1568
  );

  delay(100);

  ledcWriteTone(
    BUZZER_PIN,
    2093
  );

  delay(450);

  soundOff();
}

// =====================================================
// CENTER TEXT
// =====================================================

void centerText(
  const char* text,
  int y,
  int size,
  uint16_t color
) {

  tft.setTextSize(size);

  tft.setTextColor(
    color
  );

  int16_t x1;
  int16_t y1;

  uint16_t w;
  uint16_t h;

  tft.getTextBounds(
    text,
    0,
    y,
    &x1,
    &y1,
    &w,
    &h
  );

  tft.setCursor(
    (SCREEN_W - w) / 2,
    y
  );

  tft.print(
    text
  );
}

// =====================================================
// TITLE
// =====================================================

void drawTitle() {

  tft.fillScreen(
    ILI9341_BLACK
  );

  tft.fillRect(
    0,
    150,
    320,
    90,
    GRASS
  );

  // goal

  tft.drawRect(
    85,
    72,
    150,
    75,
    ILI9341_WHITE
  );

  // net vertical

  for (
    int x = 95;
    x < 235;
    x += 15
  ) {

    tft.drawLine(
      x,
      72,
      x,
      147,
      ILI9341_DARKGREY
    );
  }

  // net horizontal

  for (
    int y = 82;
    y < 147;
    y += 12
  ) {

    tft.drawLine(
      85,
      y,
      235,
      y,
      ILI9341_DARKGREY
    );
  }

  // ball

  tft.fillCircle(
    160,
    175,
    14,
    ILI9341_WHITE
  );

  tft.fillCircle(
    160,
    175,
    5,
    ILI9341_BLACK
  );

  centerText(
    "PENALTY",
    18,
    3,
    ILI9341_YELLOW
  );

  centerText(
    "SHOOTOUT",
    48,
    2,
    ILI9341_WHITE
  );

  tft.fillRoundRect(
    85,
    205,
    150,
    25,
    6,
    ILI9341_DARKGREEN
  );

  centerText(
    "TAP TO START",
    213,
    1,
    ILI9341_WHITE
  );
}

// =====================================================
// FIELD
// =====================================================

void drawField() {

  // sky

  tft.fillRect(
    0,
    0,
    320,
    45,
    SKY
  );

  // crowd

  tft.fillRect(
    0,
    45,
    320,
    35,
    CROWD
  );

  for (
    int x = 5;
    x < 320;
    x += 10
  ) {

    uint16_t c =
      random(0, 2)
      ? ILI9341_RED
      : ILI9341_YELLOW;

    tft.fillCircle(
      x,
      55 + random(0, 15),
      3,
      c
    );
  }

  // field

  tft.fillRect(
    0,
    80,
    320,
    160,
    GRASS
  );

  // stripes

  for (
    int x = 0;
    x < 320;
    x += 40
  ) {

    tft.fillRect(
      x,
      80,
      20,
      160,
      GRASS_LIGHT
    );
  }

  // penalty area

  tft.drawRect(
    60,
    90,
    200,
    95,
    ILI9341_WHITE
  );

  // spot

  tft.fillCircle(
    160,
    205,
    3,
    ILI9341_WHITE
  );

  // goal

  tft.fillRect(
    70,
    85,
    180,
    5,
    ILI9341_WHITE
  );

  tft.fillRect(
    70,
    85,
    5,
    80,
    ILI9341_WHITE
  );

  tft.fillRect(
    245,
    85,
    5,
    80,
    ILI9341_WHITE
  );

  // net vertical

  for (
    int x = 80;
    x < 245;
    x += 15
  ) {

    tft.drawLine(
      x,
      90,
      x,
      165,
      NET_COLOR
    );
  }

  // net horizontal

  for (
    int y = 100;
    y < 165;
    y += 12
  ) {

    tft.drawLine(
      75,
      y,
      245,
      y,
      NET_COLOR
    );
  }
}

// =====================================================
// HUD
// =====================================================

void drawHUD() {

  tft.fillRect(
    0,
    0,
    320,
    28,
    ILI9341_BLACK
  );

  tft.setTextSize(1);

  tft.setTextColor(
    ILI9341_WHITE
  );

  tft.setCursor(
    6,
    5
  );

  tft.print(
    "PENALTY "
  );

  tft.print(
    penaltyNumber
  );

  tft.print(
    "/5"
  );

  tft.setCursor(
    220,
    5
  );

  tft.print(
    "GOALS "
  );

  tft.print(
    goals
  );

  tft.setCursor(
    95,
    17
  );

  tft.setTextColor(
    ILI9341_YELLOW
  );

  tft.print(
    "TAP INSIDE GOAL"
  );
}

// =====================================================
// KEEPER DRAW
// =====================================================

void drawKeeper(
  int x
) {

  int y = 130;

  // ===================================================
  // NORMAL
  // ===================================================

  if (
    !keeperDiving
  ) {

    // head

    tft.fillCircle(
      x,
      y - 24,
      7,
      0xFD20
    );

    // shirt

    tft.fillRect(
      x - 8,
      y - 17,
      16,
      28,
      ILI9341_BLUE
    );

    // arms

    tft.drawLine(
      x - 7,
      y - 12,
      x - 25,
      y,
      ILI9341_WHITE
    );

    tft.drawLine(
      x + 7,
      y - 12,
      x + 25,
      y,
      ILI9341_WHITE
    );

    // legs

    tft.drawLine(
      x - 6,
      y + 10,
      x - 15,
      y + 28,
      ILI9341_BLACK
    );

    tft.drawLine(
      x + 6,
      y + 10,
      x + 15,
      y + 28,
      ILI9341_BLACK
    );

    // gloves

    tft.fillCircle(
      x - 26,
      y,
      4,
      ILI9341_YELLOW
    );

    tft.fillCircle(
      x + 26,
      y,
      4,
      ILI9341_YELLOW
    );

    return;
  }

  // ===================================================
  // DIVING
  // ===================================================

  int dir =
    keeperDiveDirection;

  // center jump

  if (
    dir == 0
  ) {

    int jump =
      (int)(
        sin(
          keeperDiveProgress *
          3.14159
        ) * 18
      );

    y -= jump;

    tft.fillCircle(
      x,
      y - 24,
      7,
      0xFD20
    );

    tft.fillRect(
      x - 9,
      y - 17,
      18,
      26,
      ILI9341_BLUE
    );

    // arms up

    tft.drawLine(
      x - 7,
      y - 12,
      x - 20,
      y - 28,
      ILI9341_WHITE
    );

    tft.drawLine(
      x + 7,
      y - 12,
      x + 20,
      y - 28,
      ILI9341_WHITE
    );

    tft.fillCircle(
      x - 21,
      y - 29,
      4,
      ILI9341_YELLOW
    );

    tft.fillCircle(
      x + 21,
      y - 29,
      4,
      ILI9341_YELLOW
    );

    // legs

    tft.drawLine(
      x - 5,
      y + 8,
      x - 12,
      y + 22,
      ILI9341_BLACK
    );

    tft.drawLine(
      x + 5,
      y + 8,
      x + 12,
      y + 22,
      ILI9341_BLACK
    );

    return;
  }

  // left/right dive

  int bodyY =
    125 -
    (int)(
      sin(
        keeperDiveProgress *
        3.14159
      ) * 12
    );

  // horizontal body

  tft.fillRoundRect(
    x - 17,
    bodyY - 7,
    34,
    14,
    5,
    ILI9341_BLUE
  );

  // head on dive direction side

  tft.fillCircle(
    x + dir * 20,
    bodyY - 4,
    7,
    0xFD20
  );

  // leading arm

  tft.drawLine(
    x + dir * 14,
    bodyY,
    x + dir * 34,
    bodyY - 8,
    ILI9341_WHITE
  );

  // trailing arm

  tft.drawLine(
    x - dir * 12,
    bodyY,
    x - dir * 24,
    bodyY + 8,
    ILI9341_WHITE
  );

  // gloves

  tft.fillCircle(
    x + dir * 35,
    bodyY - 9,
    5,
    ILI9341_YELLOW
  );

  tft.fillCircle(
    x - dir * 25,
    bodyY + 8,
    4,
    ILI9341_YELLOW
  );

  // legs

  tft.drawLine(
    x - dir * 13,
    bodyY + 4,
    x - dir * 28,
    bodyY + 11,
    ILI9341_BLACK
  );

  tft.drawLine(
    x - dir * 8,
    bodyY + 5,
    x - dir * 20,
    bodyY + 17,
    ILI9341_BLACK
  );
}

// =====================================================
// RESTORE KEEPER AREA
// =====================================================

void restoreKeeperArea(
  int centerX
) {

  int x1 =
    centerX - 45;

  int x2 =
    centerX + 45;

  x1 =
    constrain(
      x1,
      70,
      250
    );

  x2 =
    constrain(
      x2,
      70,
      250
    );

  // redraw grass

  for (
    int x = x1;
    x <= x2;
    x++
  ) {

    uint16_t color =
      ((x / 20) % 2 == 0)
      ? GRASS_LIGHT
      : GRASS;

    tft.drawFastVLine(
      x,
      90,
      80,
      color
    );
  }

  // net vertical

  for (
    int x = 80;
    x < 245;
    x += 15
  ) {

    if (
      x >= x1 &&
      x <= x2
    ) {

      tft.drawLine(
        x,
        90,
        x,
        165,
        NET_COLOR
      );
    }
  }

  // net horizontal

  for (
    int y = 100;
    y < 165;
    y += 12
  ) {

    tft.drawLine(
      x1,
      y,
      x2,
      y,
      NET_COLOR
    );
  }

  // posts

  tft.fillRect(
    70,
    85,
    180,
    5,
    ILI9341_WHITE
  );

  tft.fillRect(
    70,
    85,
    5,
    80,
    ILI9341_WHITE
  );

  tft.fillRect(
    245,
    85,
    5,
    80,
    ILI9341_WHITE
  );
}

// =====================================================
// BALL
// =====================================================

void drawBall(
  int x,
  int y,
  int size
) {

  tft.fillCircle(
    x,
    y,
    size,
    ILI9341_WHITE
  );

  tft.fillCircle(
    x,
    y,
    max(
      2,
      size / 3
    ),
    ILI9341_BLACK
  );
}

// =====================================================
// RESTORE BALL AREA
// =====================================================

void restoreBallArea(
  int x,
  int y,
  int radius
) {

  int x1 =
    x - radius - 3;

  int y1 =
    y - radius - 3;

  int w =
    radius * 2 + 7;

  int h =
    radius * 2 + 7;

  for (
    int px = x1;
    px < x1 + w;
    px++
  ) {

    if (
      px < 0 ||
      px >= SCREEN_W
    ) {
      continue;
    }

    uint16_t color =
      ((px / 20) % 2 == 0)
      ? GRASS_LIGHT
      : GRASS;

    tft.drawFastVLine(
      px,
      y1,
      h,
      color
    );
  }

  // restore net when ball is in goal area

  if (
    y1 < 165
  ) {

    for (
      int nx = 80;
      nx < 245;
      nx += 15
    ) {

      if (
        nx >= x1 &&
        nx <= x1 + w
      ) {

        tft.drawLine(
          nx,
          max(90, y1),
          nx,
          min(165, y1 + h),
          NET_COLOR
        );
      }
    }

    for (
      int ny = 100;
      ny < 165;
      ny += 12
    ) {

      if (
        ny >= y1 &&
        ny <= y1 + h
      ) {

        tft.drawLine(
          x1,
          ny,
          x1 + w,
          ny,
          NET_COLOR
        );
      }
    }
  }
}

// =====================================================
// DRAW GAME
// =====================================================

void drawGame() {

  drawField();

  drawHUD();

  drawKeeper(
    (int)keeperX
  );

  drawBall(
    160,
    215,
    10
  );
}

// =====================================================
// TOUCH
// =====================================================

bool getNewTap(
  int &screenX,
  int &screenY
) {

  TS_Point p =
    ts.getPoint();

  bool down =
    p.z > 200;

  if (
    !down
  ) {

    touchWasDown =
      false;

    return false;
  }

  if (
    touchWasDown
  ) {

    return false;
  }

  touchWasDown =
    true;

  screenX =
    map(
      p.x,
      TOUCH_MIN_X,
      TOUCH_MAX_X,
      0,
      SCREEN_W
    );

  screenY =
    map(
      p.y,
      TOUCH_MIN_Y,
      TOUCH_MAX_Y,
      0,
      SCREEN_H
    );

  screenX =
    constrain(
      screenX,
      0,
      SCREEN_W - 1
    );

  screenY =
    constrain(
      screenY,
      0,
      SCREEN_H - 1
    );

  Serial.print(
    "TAP X="
  );

  Serial.print(
    screenX
  );

  Serial.print(
    " Y="
  );

  Serial.println(
    screenY
  );

  return true;
}

// =====================================================
// RANDOM KEEPER MOVEMENT
// =====================================================

void updateKeeper() {

  unsigned long now =
    millis();

  if (
    now - lastKeeperFrame <
    35
  ) {

    return;
  }

  lastKeeperFrame =
    now;

  // random direction change

  if (
    now >=
    keeperDirectionChangeAt
  ) {

    if (
      random(0, 100) <
      45
    ) {

      keeperSpeed =
        -keeperSpeed;
    }

    // slightly random speed too

    float speed =
      random(
        30,
        43
      ) / 10.0;

    keeperSpeed =
      keeperSpeed >= 0
      ? speed
      : -speed;

    keeperDirectionChangeAt =
      now +
      random(
        350,
        900
      );
  }

  // erase old keeper

  restoreKeeperArea(
    oldKeeperX
  );

  keeperX +=
    keeperSpeed;

  // edges

  if (
    keeperX < 108
  ) {

    keeperX = 108;

    keeperSpeed =
      abs(
        keeperSpeed
      );
  }

  if (
    keeperX > 212
  ) {

    keeperX = 212;

    keeperSpeed =
      -abs(
        keeperSpeed
      );
  }

  drawKeeper(
    (int)keeperX
  );

  oldKeeperX =
    (int)keeperX;
}

// =====================================================
// START SHOT
// =====================================================

void startShot(
  int x,
  int y
) {

  x =
    constrain(
      x,
      82,
      238
    );

  y =
    constrain(
      y,
      96,
      157
    );

  targetX =
    x;

  targetY =
    y;

  ballX =
    160;

  ballY =
    215;

  oldBallX =
    ballX;

  oldBallY =
    ballY;

  shotProgress =
    0;

  // ===================================================
  // RANDOM DIVE
  // ===================================================

  keeperDiving =
    true;

  keeperDiveStartX =
    keeperX;

  keeperDiveProgress =
    0;

  int randomDive =
    random(
      0,
      100
    );

  // 37% left
  if (
    randomDive < 37
  ) {

    keeperDiveDirection =
      -1;

    keeperDiveTargetX =
      105;
  }

  // 37% right
  else if (
    randomDive < 74
  ) {

    keeperDiveDirection =
      1;

    keeperDiveTargetX =
      215;
  }

  // 26% center
  else {

    keeperDiveDirection =
      0;

    keeperDiveTargetX =
      keeperX;
  }

  Serial.print(
    "KEEPER DIVE: "
  );

  if (
    keeperDiveDirection == -1
  ) {

    Serial.println(
      "LEFT"
    );

  } else if (
    keeperDiveDirection == 1
  ) {

    Serial.println(
      "RIGHT"
    );

  } else {

    Serial.println(
      "CENTER"
    );
  }

  soundKick();

  gameState =
    SHOOTING;

  lastBallFrame =
    millis();
}

// =====================================================
// UPDATE SHOT + DIVE
// =====================================================

void updateShot() {

  unsigned long now =
    millis();

  if (
    now - lastBallFrame <
    40
  ) {

    return;
  }

  lastBallFrame =
    now;

  // ===================================================
  // ERASE OLD DYNAMIC ELEMENTS
  // ===================================================

  restoreKeeperArea(
    oldKeeperX
  );

  restoreBallArea(
    (int)oldBallX,
    (int)oldBallY,
    12
  );

  // ===================================================
  // PROGRESS
  // ===================================================

  shotProgress +=
    0.09;

  if (
    shotProgress > 1
  ) {

    shotProgress =
      1;
  }

  keeperDiveProgress =
    shotProgress;

  // ===================================================
  // BALL MOVEMENT
  // ===================================================

  // use easing so ball accelerates

  float eased =
    shotProgress *
    shotProgress *
    (
      3 -
      2 * shotProgress
    );

  ballX =
    160 +
    (targetX - 160) *
    eased;

  ballY =
    215 +
    (targetY - 215) *
    eased;

  // ===================================================
  // KEEPER DIVE MOVEMENT
  // ===================================================

  keeperX =
    keeperDiveStartX +
    (
      keeperDiveTargetX -
      keeperDiveStartX
    ) *
    eased;

  // ===================================================
  // DRAW KEEPER
  // ===================================================

  drawKeeper(
    (int)keeperX
  );

  oldKeeperX =
    (int)keeperX;

  // ===================================================
  // DRAW BALL
  // ===================================================

  int ballSize =
    10 -
    (int)(
      shotProgress * 4
    );

  if (
    ballSize < 5
  ) {

    ballSize =
      5;
  }

  drawBall(
    (int)ballX,
    (int)ballY,
    ballSize
  );

  oldBallX =
    ballX;

  oldBallY =
    ballY;

  // ===================================================
  // FINISHED
  // ===================================================

  if (
    shotProgress >= 1
  ) {

    // goalkeeper save box

    int keeperCenterY =
      125;

    int dx =
      abs(
        (int)targetX -
        (int)keeperX
      );

    int dy =
      abs(
        (int)targetY -
        keeperCenterY
      );

    // =================================================
    // SAVE CHANCE
    // =================================================

    bool saved =
      false;

    // diving left/right
    if (
      keeperDiveDirection != 0
    ) {

      // generous horizontal reach,
      // but NOT so big that every shot is saved

      if (
        dx < 31 &&
        dy < 33
      ) {

        saved =
          true;
      }
    }

    // center jump
    else {

      if (
        dx < 23 &&
        dy < 30
      ) {

        saved =
          true;
      }
    }

    // =================================================
    // SMALL RANDOM HUMAN ERROR
    // =================================================

    // Even if keeper reaches it,
    // 18% chance he fails to stop it.

    if (
      saved &&
      random(0,100) < 18
    ) {

      saved =
        false;
    }

    if (
      saved
    ) {

      lastShotGoal =
        false;

      soundSave();

    } else {

      lastShotGoal =
        true;

      goals++;

      soundGoal();
    }

    resultStarted =
      millis();

    resultDrawn =
      false;

    gameState =
      RESULT;
  }
}

// =====================================================
// RESULT
// =====================================================

void drawResult() {

  tft.fillRoundRect(
    90,
    175,
    140,
    45,
    8,
    ILI9341_BLACK
  );

  if (
    lastShotGoal
  ) {

    centerText(
      "GOAL!",
      186,
      3,
      ILI9341_GREEN
    );

  } else {

    centerText(
      "SAVED!",
      186,
      3,
      ILI9341_RED
    );
  }
}

// =====================================================
// NEXT PENALTY
// =====================================================

void nextPenalty() {

  penaltyNumber++;

  keeperDiving =
    false;

  if (
    penaltyNumber >
    MAX_PENALTIES
  ) {

    gameState =
      GAME_OVER;

    return;
  }

  keeperX =
    160;

  oldKeeperX =
    160;

  keeperSpeed =
    random(0, 2)
    ? 3.4
    : -3.4;

  keeperDirectionChangeAt =
    millis() +
    random(
      350,
      900
    );

  drawGame();

  gameState =
    READY;
}

// =====================================================
// GAME OVER
// =====================================================

void drawGameOver() {

  tft.fillScreen(
    ILI9341_BLACK
  );

  centerText(
    "SHOOTOUT OVER",
    32,
    2,
    ILI9341_YELLOW
  );

  centerText(
    "YOU SCORED",
    75,
    2,
    ILI9341_WHITE
  );

  char scoreText[20];

  sprintf(
    scoreText,
    "%d / 5",
    goals
  );

  centerText(
    scoreText,
    105,
    4,
    ILI9341_GREEN
  );

  // ===================================================
  // PERFECT
  // ===================================================

  if (
    goals == 5
  ) {

    centerText(
      "PERFECT!",
      163,
      3,
      ILI9341_YELLOW
    );

    soundPerfect();

  } else if (
    goals >= 3
  ) {

    centerText(
      "GREAT JOB!",
      165,
      2,
      ILI9341_CYAN
    );

  } else {

    centerText(
      "TRY AGAIN!",
      165,
      2,
      ILI9341_RED
    );
  }

  centerText(
    "TAP TO RESTART",
    215,
    1,
    ILI9341_WHITE
  );
}

// =====================================================
// NEW GAME
// =====================================================

void newGame() {

  penaltyNumber =
    1;

  goals =
    0;

  keeperX =
    160;

  oldKeeperX =
    160;

  keeperSpeed =
    3.4;

  keeperDiving =
    false;

  keeperDirectionChangeAt =
    millis() +
    random(
      350,
      900
    );

  soundStart();

  drawGame();

  gameState =
    READY;
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(
    115200
  );

  // buzzer

  ledcAttach(
    BUZZER_PIN,
    2000,
    8
  );

  soundOff();

  // backlight

  pinMode(
    TFT_BL,
    OUTPUT
  );

  digitalWrite(
    TFT_BL,
    HIGH
  );

  // TFT

  tftSPI.begin(
    TFT_SCLK,
    TFT_MISO,
    TFT_MOSI,
    TFT_CS
  );

  tft.begin();

  tft.setRotation(
    1
  );

  tft.setTextWrap(
    false
  );

  // TOUCH

  touchSPI.begin(
    TOUCH_CLK,
    TOUCH_MISO,
    TOUCH_MOSI,
    TOUCH_CS
  );

  ts.begin(
    touchSPI
  );

  ts.setRotation(
    1
  );

  randomSeed(
    micros()
  );

  drawTitle();

  startTitleMusic();
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  unsigned long now =
    millis();

  // ===================================================
  // TITLE MUSIC
  // ===================================================

  if (
    gameState ==
    TITLE
  ) {

    updateTitleMusic();
  }

  // ===================================================
  // TOUCH
  // ===================================================

  int tapX;
  int tapY;

  if (
    getNewTap(
      tapX,
      tapY
    )
  ) {

    // TITLE

    if (
      gameState ==
      TITLE
    ) {

      stopTitleMusic();

      newGame();

      delay(200);

      return;
    }

    // READY

    if (
      gameState ==
      READY
    ) {

      if (
        tapX >= 70 &&
        tapX <= 250 &&
        tapY >= 85 &&
        tapY <= 170
      ) {

        startShot(
          tapX,
          tapY
        );
      }

      return;
    }

    // GAME OVER

    if (
      gameState ==
      GAME_OVER
    ) {

      gameState =
        TITLE;

      drawTitle();

      startTitleMusic();

      delay(200);

      return;
    }
  }

  // ===================================================
  // STATE MACHINE
  // ===================================================

  switch (
    gameState
  ) {

    case READY:

      updateKeeper();

      break;

    case SHOOTING:

      updateShot();

      break;

    case RESULT:

      if (
        !resultDrawn
      ) {

        drawResult();

        resultDrawn =
          true;
      }

      if (
        now -
        resultStarted >
        1400
      ) {

        nextPenalty();

        if (
          gameState ==
          GAME_OVER
        ) {

          drawGameOver();
        }
      }

      break;

    default:

      break;
  }

  delay(2);
}
