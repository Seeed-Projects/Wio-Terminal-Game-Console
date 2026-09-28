#include <Seeed_GFX.h>
#include <Wire.h>

Seeed_GFX display(Seeed_Product::WIO_TERMINAL_PRODUCT);

// 5向开关引脚定义（Wio Terminal 官方引脚常量）
const int PIN_UP = WIO_5S_UP;
const int PIN_DOWN = WIO_5S_DOWN;
const int PIN_LEFT = WIO_5S_LEFT;
const int PIN_RIGHT = WIO_5S_RIGHT;
const int PIN_PRESS = WIO_5S_PRESS;

// 顶部三个按键定义
const int PIN_KEY_A = WIO_KEY_A;  // 暂停/继续
const int PIN_KEY_B = WIO_KEY_B;  // 重新开始
const int PIN_KEY_C = WIO_KEY_C;  // 返回菜单

// 游戏枚举
enum GameType { MENU, SNAKE, SOKOBAN, TETRIS, BREAKOUT, TOOLS };
GameType currentGame = MENU;

// 传感器工具枚举
enum SensorType { NO_SENSOR, LIGHT_SENSOR, ACCELEROMETER, IR_SENSOR, BUZZER_TOOL, MIC_SENSOR };
SensorType currentSensor = NO_SENSOR;
bool sensorFirstDraw = true;
int sensorSelection = 0;
int lastSensorSelection = -1;
const char* sensorItems[] = {"Light Sensor", "Accelerometer", "IR Emitter", "Buzzer", "Microphone"};
const int sensorCount = 5;

// 传感器数值记录（用于避免覆盖）
int lastLightValue = -1;
int16_t lastAccelX = -9999, lastAccelY = -9999, lastAccelZ = -9999;
int lastMicValue = -1;
unsigned long lastSensorUpdate = 0;
const unsigned long SENSOR_UPDATE_INTERVAL = 200;  // 传感器刷新间隔 200ms

// 防抖
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 150;

// 顶部按键防抖
unsigned long lastKeyDebounceTime = 0;
const unsigned long keyDebounceDelay = 200;

// 暂停状态
bool gamePaused = false;

// 顶部按键枚举
enum TopKey { NO_KEY, KEY_A_PAUSE, KEY_B_RESTART, KEY_C_MENU };

// 定义串口
#define SerialDebug SerialUSB

// ========== 主菜单 ==========
int menuSelection = 0;
int lastMenuSelection = -1;  // 记录上次选中项用于局部刷新
const char* menuItems[] = {"Snake", "Sokoban", "Tetris", "Breakout", "Tools"};
const int menuCount = 5;
bool menuFirstDraw = true;

// ========== 贪吃蛇游戏 ==========
const int SNAKE_GRID_SIZE = 20;
const int SNAKE_CELL_SIZE = 10;
const int SNAKE_OFFSET_X = 60;  // 居中偏移
const int SNAKE_OFFSET_Y = 10;
int snakeX[100], snakeY[100];
int snakeLength = 3;
int snakeDirX = 1, snakeDirY = 0;
int foodX = 10, foodY = 10;
bool snakeGameOver = false;
bool snakeGameOverDrawn = false;
int snakeScore = 0;
unsigned long snakeLastMove = 0;
const int SNAKE_SPEED = 150;
int lastTailX = -1, lastTailY = -1;  // 记录蛇尾位置用于清除
bool snakeFirstDraw = true;
int lastSnakeScore = -1;  // 记录上次分数，只在变化时刷新

// ========== 推箱子游戏 ==========
const int SOKOBAN_COLS = 10;
const int SOKOBAN_ROWS = 8;
const int SOKOBAN_CELL = 28;
int sokobanMap[8][10];
int playerX = 1, playerY = 1;
int lastPlayerX = -1, lastPlayerY = -1;  // 记录玩家上次位置用于局部刷新
int lastBoxX = -1, lastBoxY = -1;  // 记录箱子上次位置
int lastBoxNewX = -1, lastBoxNewY = -1;  // 记录箱子新位置
bool sokobanBoxMoved = false;  // 是否有箱子被推动
bool sokobanComplete = false;
int sokobanLevel = 0;
bool sokobanFirstDraw = true;

// 推箱子地图元素
const int SOKOBAN_EMPTY = 0;
const int SOKOBAN_WALL = 1;
const int SOKOBAN_TARGET = 2;
const int SOKOBAN_BOX = 3;
const int SOKOBAN_BOX_ON_TARGET = 4;
const int SOKOBAN_PLAYER = 5;
const int SOKOBAN_PLAYER_ON_TARGET = 6;

// 推箱子关卡
const int sokobanLevels[3][8][10] = {
  // 关卡 1 (简单)
  {
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,5,0,0,3,0,0,0,1},
    {1,0,0,0,0,0,0,2,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1}
  },
  // 关卡 2
  {
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,5,0,3,0,0,0,0,1},
    {1,0,0,0,0,3,0,0,0,1},
    {1,0,2,0,0,0,0,2,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1}
  },
  // 关卡 3
  {
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,3,0,0,0,3,0,0,1},
    {1,0,0,0,5,0,0,0,0,1},
    {1,0,0,0,0,0,3,0,3,1},
    {1,0,2,2,0,0,2,2,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1}
  }
};

// ========== 俄罗斯方块 ==========
const int TETRIS_COLS = 10;
const int TETRIS_ROWS = 20;
const int TETRIS_CELL = 11;
const int TETRIS_OFFSET_X = 105;  // 居中偏移
const int TETRIS_OFFSET_Y = 5;
int tetrisGrid[10][20];
int currentPiece[4][4];
int currentPieceX = 3, currentPieceY = 0;
int currentPieceType = 0;
bool tetrisGameOver = false;
bool tetrisGameOverDrawn = false;
int tetrisScore = 0;
unsigned long tetrisLastDrop = 0;
const int TETRIS_SPEED = 500;
bool tetrisFirstDraw = true;
int lastPieceX = -1, lastPieceY = -1;  // 记录上次方块位置
int lastPieceShape[4][4];  // 记录上次方块形状
int lastTetrisScore = -1;  // 记录上次分数，只在变化时刷新

// 方块形状
const int pieces[7][4][4] = {
  // I
  {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
  // O
  {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
  // T
  {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},
  // S
  {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
  // Z
  {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
  // J
  {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},
  // L
  {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}}
};

const uint16_t pieceColors[7] = {TFT_CYAN, TFT_YELLOW, TFT_PURPLE, TFT_GREEN, TFT_RED, TFT_BLUE, TFT_ORANGE};

// ========== 通用函数 ==========
void drawButton(const char* text, int x, int y, int w, int h, uint16_t color) {
  display.fillRoundRect(x, y, w, h, 8, color);
  display.setTextColor(TFT_WHITE);
  display.setTextSize(2);
  display.drawCentreString(text, x + w/2, y + h/2, 1);
}

bool readJoystick(int& dirX, int& dirY, bool& press) {
  if (millis() - lastDebounceTime < debounceDelay) {
    return false;
  }
  
  if (digitalRead(PIN_UP) == LOW) {
    dirX = 0; dirY = -1; press = false;
    lastDebounceTime = millis();
    return true;
  }
  if (digitalRead(PIN_DOWN) == LOW) {
    dirX = 0; dirY = 1; press = false;
    lastDebounceTime = millis();
    return true;
  }
  if (digitalRead(PIN_LEFT) == LOW) {
    dirX = -1; dirY = 0; press = false;
    lastDebounceTime = millis();
    return true;
  }
  if (digitalRead(PIN_RIGHT) == LOW) {
    dirX = 1; dirY = 0; press = false;
    lastDebounceTime = millis();
    return true;
  }
  if (digitalRead(PIN_PRESS) == LOW) {
    dirX = 0; dirY = 0; press = true;
    lastDebounceTime = millis();
    return true;
  }
  return false;
}

// 读取顶部按键（A=暂停, B=重新开始, C=返回菜单）
TopKey readTopButtons() {
  if (millis() - lastKeyDebounceTime < keyDebounceDelay) {
    return NO_KEY;
  }
  
  if (digitalRead(PIN_KEY_A) == LOW) {
    lastKeyDebounceTime = millis();
    return KEY_A_PAUSE;
  }
  if (digitalRead(PIN_KEY_B) == LOW) {
    lastKeyDebounceTime = millis();
    return KEY_B_RESTART;
  }
  if (digitalRead(PIN_KEY_C) == LOW) {
    lastKeyDebounceTime = millis();
    return KEY_C_MENU;
  }
  return NO_KEY;
}

// 显示暂停界面
void drawPauseScreen() {
  // 清除整个屏幕
  display.fillScreen(TFT_BLACK);
  display.drawRoundRect(60, 80, 200, 80, 12, TFT_YELLOW);
  display.setTextColor(TFT_YELLOW);
  display.setTextSize(3);
  display.drawCentreString("PAUSED", 160, 100, 1);
  display.setTextColor(TFT_WHITE);
  display.setTextSize(1);
  display.drawCentreString("Press A to continue", 160, 135, 1);
}

// 显示顶部按键提示
void drawButtonHints() {
  display.fillRect(0, 225, 320, 15, TFT_DARKGREY);
  display.setTextColor(TFT_WHITE);
  display.setTextSize(1);
  display.drawCentreString("A:Pause  B:Restart  C:Menu", 160, 228, 1);
}

// ========== 主菜单 ==========
const int MENU_VISIBLE_COUNT = 3;  // 屏幕最多显示 3 个
int menuScrollOffset = 0;  // 滚动偏移

// 绘制滚动条
void drawScrollbar() {
  int scrollbarX = 300;
  int scrollbarY = 75;
  int scrollbarHeight = MENU_VISIBLE_COUNT * 50;
  
  // 绘制滚动条背景
  display.fillRect(scrollbarX, scrollbarY, 4, scrollbarHeight, TFT_DARKGREY);
  
  // 计算滑块位置和大小
  float scrollRatio = (float)MENU_VISIBLE_COUNT / menuCount;
  int thumbHeight = max(20, (int)(scrollbarHeight * scrollRatio));
  
  // 根据滚动进度计算滑块位置（避免超出边界）
  int maxScrollOffset = menuCount - MENU_VISIBLE_COUNT;
  float scrollProgress = maxScrollOffset > 0 ? (float)menuScrollOffset / maxScrollOffset : 0;
  int thumbY = scrollbarY + (int)(scrollProgress * (scrollbarHeight - thumbHeight));
  
  // 绘制滑块
  display.fillRect(scrollbarX, thumbY, 4, thumbHeight, TFT_WHITE);
}

// 绘制单个菜单项（用于局部刷新）
void drawMenuItem(int index, bool selected) {
  int visibleIndex = index - menuScrollOffset;
  int y = 75 + visibleIndex * 50;
  uint16_t gameColors[] = {TFT_GREEN, TFT_ORANGE, TFT_CYAN, TFT_MAGENTA, TFT_YELLOW};
  
  if (selected) {
    // 选中项：蓝色背景 + 白色文字
    display.fillRoundRect(30, y, 260, 42, 10, TFT_BLUE);
    // 绘制彩色图标（无文字）
    display.fillRoundRect(45, y + 8, 26, 26, 6, gameColors[index]);
    // 游戏名称
    display.setTextColor(TFT_WHITE);
    display.setTextSize(2);
    display.drawString(menuItems[index], 85, y + 12, 1);
  } else {
    // 未选中项：灰色背景
    display.fillRoundRect(30, y, 260, 42, 10, TFT_DARKGREY);
    // 绘制彩色图标（无文字）
    display.fillRoundRect(45, y + 8, 26, 26, 6, gameColors[index]);
    // 游戏名称
    display.setTextColor(TFT_LIGHTGREY);
    display.setTextSize(2);
    display.drawString(menuItems[index], 85, y + 12, 1);
  }
}

void drawMenu() {
  display.fillScreen(TFT_BLACK);
  
  // 标题
  display.setTextColor(TFT_YELLOW);
  display.setTextSize(3);
  display.drawCentreString("WIO Game Console", 160, 30, 1);
  
  // 分隔线
  display.drawFastHLine(20, 60, 280, TFT_DARKGREY);
  
  // 绘制可见菜单项
  for (int i = menuScrollOffset; i < menuCount && i < menuScrollOffset + MENU_VISIBLE_COUNT; i++) {
    drawMenuItem(i, i == menuSelection);
  }
  
  // 绘制滚动条
  drawScrollbar();
  
  // 底部提示
  display.setTextColor(TFT_DARKGREY);
  display.setTextSize(1);
  display.drawCentreString("UP/DOWN: Select | PRESS: Start", 160, 230, 1);
  
  menuFirstDraw = false;
}

void updateMenuSelection() {
  int oldSelection = menuSelection;
  int oldScrollOffset = menuScrollOffset;
  
  int dirX, dirY;
  bool press;
  if (readJoystick(dirX, dirY, press)) {
    if (dirY == -1 && menuSelection > 0) {
      menuSelection--;
    }
    if (dirY == 1 && menuSelection < menuCount - 1) {
      menuSelection++;
    }
    if (press) {
      // 选择游戏
      if (menuSelection == 0) {
        initSnake();
        currentGame = SNAKE;
      } else if (menuSelection == 1) {
        initSokoban();
        currentGame = SOKOBAN;
      } else if (menuSelection == 2) {
        initTetris();
        currentGame = TETRIS;
      } else if (menuSelection == 3) {
        initBreakout();
        currentGame = BREAKOUT;
      } else if (menuSelection == 4) {
        sensorSelection = 0;
        currentSensor = NO_SENSOR;
        sensorFirstDraw = true;
        currentGame = TOOLS;
      }
      return;
    }
  }
  
  // 自动滚动：保持选中项在可见区域
  if (menuSelection < menuScrollOffset) {
    menuScrollOffset = menuSelection;
  }
  if (menuSelection >= menuScrollOffset + MENU_VISIBLE_COUNT) {
    menuScrollOffset = menuSelection - MENU_VISIBLE_COUNT + 1;
  }
  
  // 局部刷新：同页选择只更新选中项，跨页滚动才全屏刷新
  if (menuScrollOffset != oldScrollOffset) {
    drawMenu();  // 滚动时重绘整个菜单
  } else if (menuSelection != oldSelection) {
    drawMenuItem(oldSelection, false);  // 清除旧选中项
    drawMenuItem(menuSelection, true);  // 绘制新选中项
    drawScrollbar();  // 更新滚动条
  }
}

// ========== 贪吃蛇游戏 ==========
void initSnake() {
  snakeLength = 3;
  snakeX[0] = 5; snakeY[0] = 5;
  snakeX[1] = 4; snakeY[1] = 5;
  snakeX[2] = 3; snakeY[2] = 5;
  snakeDirX = 1; snakeDirY = 0;
  snakeScore = 0;
  lastSnakeScore = -1;  // 重置分数记录
  snakeGameOver = false;
  snakeGameOverDrawn = false;
  snakeFirstDraw = true;  // 重置首绘标志
  lastTailX = -1; lastTailY = -1;  // 清除蛇尾记录
  placeFood();
  snakeLastMove = millis();
}

void placeFood() {
  foodX = random(1, SNAKE_GRID_SIZE - 1);
  foodY = random(1, SNAKE_GRID_SIZE - 1);
  // 确保食物不在蛇身上
  for (int i = 0; i < snakeLength; i++) {
    if (snakeX[i] == foodX && snakeY[i] == foodY) {
      placeFood();
      return;
    }
  }
}

void drawSnakeGame() {
  if (snakeFirstDraw) {
    display.fillScreen(TFT_BLACK);
    // 绘制边框
    display.drawRect(SNAKE_OFFSET_X - 2, SNAKE_OFFSET_Y - 2, 
                     SNAKE_GRID_SIZE * SNAKE_CELL_SIZE + 4, 
                     SNAKE_GRID_SIZE * SNAKE_CELL_SIZE + 4, TFT_WHITE);
    // 绘制初始蛇
    for (int i = 0; i < snakeLength; i++) {
      if (i == 0) {
        display.fillRect(snakeX[i] * SNAKE_CELL_SIZE + SNAKE_OFFSET_X, 
                         snakeY[i] * SNAKE_CELL_SIZE + SNAKE_OFFSET_Y,
                         SNAKE_CELL_SIZE, SNAKE_CELL_SIZE, TFT_GREEN);
      } else {
        display.fillRect(snakeX[i] * SNAKE_CELL_SIZE + SNAKE_OFFSET_X, 
                         snakeY[i] * SNAKE_CELL_SIZE + SNAKE_OFFSET_Y,
                         SNAKE_CELL_SIZE - 1, SNAKE_CELL_SIZE - 1, TFT_DARKGREEN);
      }
    }
    // 绘制食物（用方形避免圆形残留）
    int foodCenterX = foodX * SNAKE_CELL_SIZE + SNAKE_CELL_SIZE/2 + SNAKE_OFFSET_X;
    int foodCenterY = foodY * SNAKE_CELL_SIZE + SNAKE_CELL_SIZE/2 + SNAKE_OFFSET_Y;
    int foodRadius = SNAKE_CELL_SIZE/2 - 1;  // 减小半径避免残留
    display.fillCircle(foodCenterX, foodCenterY, foodRadius, TFT_RED);
    // 绘制分数（首次绘制）
    display.fillRect(60, 220, 200, 20, TFT_BLACK);
    display.setTextColor(TFT_WHITE);
    display.setTextSize(2);
    display.drawCentreString("Score: " + String(snakeScore), 160, 225, 1);
    lastSnakeScore = snakeScore;  // 初始化记录值
    snakeFirstDraw = false;
    return;
  }
  
  // 清除蛇尾（如果没吃到食物）
  if (lastTailX >= 0) {
    display.fillRect(lastTailX * SNAKE_CELL_SIZE + SNAKE_OFFSET_X, 
                     lastTailY * SNAKE_CELL_SIZE + SNAKE_OFFSET_Y,
                     SNAKE_CELL_SIZE, SNAKE_CELL_SIZE, TFT_BLACK);
  }
  
  // 绘制蛇头
  display.fillRect(snakeX[0] * SNAKE_CELL_SIZE + SNAKE_OFFSET_X, 
                   snakeY[0] * SNAKE_CELL_SIZE + SNAKE_OFFSET_Y,
                   SNAKE_CELL_SIZE, SNAKE_CELL_SIZE, TFT_GREEN);
  
  // 绘制新蛇身（除了头部）
  for (int i = 1; i < snakeLength; i++) {
    display.fillRect(snakeX[i] * SNAKE_CELL_SIZE + SNAKE_OFFSET_X, 
                     snakeY[i] * SNAKE_CELL_SIZE + SNAKE_OFFSET_Y,
                     SNAKE_CELL_SIZE - 1, SNAKE_CELL_SIZE - 1, TFT_DARKGREEN);
  }
  
  // 绘制食物（用方形避免圆形残留）
  int foodCenterX = foodX * SNAKE_CELL_SIZE + SNAKE_CELL_SIZE/2 + SNAKE_OFFSET_X;
  int foodCenterY = foodY * SNAKE_CELL_SIZE + SNAKE_CELL_SIZE/2 + SNAKE_OFFSET_Y;
  int foodRadius = SNAKE_CELL_SIZE/2 - 1;  // 减小半径避免残留
  display.fillCircle(foodCenterX, foodCenterY, foodRadius, TFT_RED);
  
  // 分数（只在变化时刷新）
  if (snakeScore != lastSnakeScore) {
    display.fillRect(60, 220, 200, 20, TFT_BLACK);
    display.setTextColor(TFT_WHITE);
    display.setTextSize(2);
    display.drawCentreString("Score: " + String(snakeScore), 160, 225, 1);
    lastSnakeScore = snakeScore;
  }
}

void updateSnake() {
  if (snakeGameOver) {
    int dirX, dirY;
    bool press;
    if (readJoystick(dirX, dirY, press) && press) {
      currentGame = MENU;
      gamePaused = false;
      drawMenu();
    }
    return;
  }
  
  // 检查顶部按键
  TopKey topKey = readTopButtons();
  if (topKey == KEY_A_PAUSE) {
    gamePaused = !gamePaused;
    if (gamePaused) {
      drawPauseScreen();
    } else {
      snakeFirstDraw = true;  // 重置首绘，全屏重绘清除暂停框
      lastTailX = -1; lastTailY = -1;  // 清除蛇尾记录
      drawSnakeGame();
    }
    return;
  }
  if (topKey == KEY_B_RESTART) {
    initSnake();  // initSnake 已经设置 snakeFirstDraw = true
    drawSnakeGame();
    return;
  }
  if (topKey == KEY_C_MENU) {
    currentGame = MENU;
    gamePaused = false;
    drawMenu();
    return;
  }
  
  if (gamePaused) return;  // 暂停时不更新游戏
  
  // 首次绘制：立即全屏绘制游戏界面
  if (snakeFirstDraw) {
    drawSnakeGame();
    return;
  }
  
  if (millis() - snakeLastMove < SNAKE_SPEED) {
    // 仍然可以读取输入改变方向
    int dirX, dirY;
    bool press;
    if (readJoystick(dirX, dirY, press)) {
      if ((dirX != 0 || dirY != 0) && !(dirX == -snakeDirX && dirY == -snakeDirY)) {
        snakeDirX = dirX;
        snakeDirY = dirY;
      }
    }
    return;
  }
  
  snakeLastMove = millis();
  
  // 记录蛇尾位置（用于清除）
  lastTailX = snakeX[snakeLength - 1];
  lastTailY = snakeY[snakeLength - 1];
  
  // 移动蛇
  for (int i = snakeLength; i > 0; i--) {
    snakeX[i] = snakeX[i-1];
    snakeY[i] = snakeY[i-1];
  }
  snakeX[0] += snakeDirX;
  snakeY[0] += snakeDirY;
  
  // 检查碰撞
  if (snakeX[0] < 0 || snakeX[0] >= SNAKE_GRID_SIZE ||
      snakeY[0] < 0 || snakeY[0] >= SNAKE_GRID_SIZE) {
    snakeGameOver = true;
  }
  
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      snakeGameOver = true;
    }
  }
  
  if (snakeGameOver) {
    if (!snakeGameOverDrawn) {
      display.fillScreen(TFT_BLACK);
      display.setTextColor(TFT_RED);
      display.setTextSize(3);
      display.drawCentreString("Game Over!", 160, 100, 1);
      display.setTextColor(TFT_WHITE);
      display.setTextSize(2);
      display.drawCentreString("Score: " + String(snakeScore), 160, 140, 1);
      display.setTextColor(TFT_YELLOW);
      display.drawCentreString("Press to Menu", 160, 180, 1);
      snakeGameOverDrawn = true;
    }
    return;
  }
  
  // 检查是否吃到食物
  if (snakeX[0] == foodX && snakeY[0] == foodY) {
    snakeLength++;
    snakeScore += 10;
    placeFood();
    lastTailX = -1;  // 吃到食物不清除蛇尾
  }
  
  drawSnakeGame();
}

// ========== 推箱子游戏 ==========
void initSokoban() {
  sokobanLevel = 0;
  loadSokobanLevel();
}

void loadSokobanLevel() {
  for (int y = 0; y < SOKOBAN_ROWS; y++) {
    for (int x = 0; x < SOKOBAN_COLS; x++) {
      sokobanMap[y][x] = sokobanLevels[sokobanLevel][y][x];
      if (sokobanMap[y][x] == SOKOBAN_PLAYER) {
        playerX = x;
        playerY = y;
      }
    }
  }
  sokobanComplete = false;
  sokobanFirstDraw = true;  // 重置首绘标志
  lastPlayerX = -1; lastPlayerY = -1;
  lastBoxX = -1; lastBoxY = -1;
  lastBoxNewX = -1; lastBoxNewY = -1;
  sokobanBoxMoved = false;
  drawSokobanGame();
}

// 绘制单个推箱子格子
void drawSokobanCell(int x, int y) {
  int offsetX = (320 - SOKOBAN_COLS * SOKOBAN_CELL) / 2;
  int offsetY = 10;
  int px = offsetX + x * SOKOBAN_CELL;
  int py = offsetY + y * SOKOBAN_CELL;
  
  // 先清除格子
  display.fillRect(px, py, SOKOBAN_CELL, SOKOBAN_CELL, TFT_BLACK);
  
  switch (sokobanMap[y][x]) {
    case SOKOBAN_WALL:
      display.fillRect(px, py, SOKOBAN_CELL, SOKOBAN_CELL, TFT_DARKGREY);
      display.drawRect(px, py, SOKOBAN_CELL, SOKOBAN_CELL, TFT_NAVY);
      break;
    case SOKOBAN_TARGET:
      display.fillCircle(px + SOKOBAN_CELL/2, py + SOKOBAN_CELL/2, 6, TFT_YELLOW);
      break;
    case SOKOBAN_BOX:
      display.fillRect(px + 2, py + 2, SOKOBAN_CELL - 4, SOKOBAN_CELL - 4, TFT_BROWN);
      break;
    case SOKOBAN_BOX_ON_TARGET:
      display.fillCircle(px + SOKOBAN_CELL/2, py + SOKOBAN_CELL/2, 6, TFT_YELLOW);
      display.fillRect(px + 2, py + 2, SOKOBAN_CELL - 4, SOKOBAN_CELL - 4, TFT_GREEN);
      break;
    case SOKOBAN_PLAYER:
    case SOKOBAN_PLAYER_ON_TARGET:
      display.fillCircle(px + SOKOBAN_CELL/2, py + SOKOBAN_CELL/2, 10, TFT_BLUE);
      break;
  }
}

void drawSokobanGame() {
  if (sokobanFirstDraw) {
    display.fillScreen(TFT_BLACK);
    
    // 绘制整个地图
    for (int y = 0; y < SOKOBAN_ROWS; y++) {
      for (int x = 0; x < SOKOBAN_COLS; x++) {
        drawSokobanCell(x, y);
      }
    }
    
    // 关卡信息
    display.setTextColor(TFT_WHITE);
    display.setTextSize(2);
    display.drawCentreString("Level: " + String(sokobanLevel + 1), 160, 240, 1);
    
    lastPlayerX = playerX;
    lastPlayerY = playerY;
    sokobanFirstDraw = false;
  } else {
    // 局部刷新：只重绘变化的格子
    // 清除玩家旧位置
    if (lastPlayerX >= 0 && lastPlayerY >= 0) {
      drawSokobanCell(lastPlayerX, lastPlayerY);
    }
    
    // 清除箱子旧位置
    if (sokobanBoxMoved && lastBoxX >= 0 && lastBoxY >= 0) {
      drawSokobanCell(lastBoxX, lastBoxY);
    }
    
    // 绘制玩家新位置
    drawSokobanCell(playerX, playerY);
    
    // 绘制箱子新位置
    if (sokobanBoxMoved && lastBoxNewX >= 0 && lastBoxNewY >= 0) {
      drawSokobanCell(lastBoxNewX, lastBoxNewY);
    }
    
    sokobanBoxMoved = false;
  }
}

void moveSokobanPlayer(int dx, int dy) {
  int newX = playerX + dx;
  int newY = playerY + dy;
  
  if (newX < 0 || newX >= SOKOBAN_COLS || newY < 0 || newY >= SOKOBAN_ROWS) {
    return;
  }
  
  int targetCell = sokobanMap[newY][newX];
  
  // 如果是墙，不能移动
  if (targetCell == SOKOBAN_WALL) {
    return;
  }
  
  // 如果是空地或目标点，直接移动
  if (targetCell == SOKOBAN_EMPTY || targetCell == SOKOBAN_TARGET) {
    // 记录玩家旧位置
    lastPlayerX = playerX;
    lastPlayerY = playerY;
    sokobanBoxMoved = false;
    
    // 清除玩家当前位置
    if (sokobanMap[playerY][playerX] == SOKOBAN_PLAYER_ON_TARGET) {
      sokobanMap[playerY][playerX] = SOKOBAN_TARGET;
    } else {
      sokobanMap[playerY][playerX] = SOKOBAN_EMPTY;
    }
    
    // 移动玩家
    playerX = newX;
    playerY = newY;
    if (sokobanMap[playerY][playerX] == SOKOBAN_TARGET) {
      sokobanMap[playerY][playerX] = SOKOBAN_PLAYER_ON_TARGET;
    } else {
      sokobanMap[playerY][playerX] = SOKOBAN_PLAYER;
    }
  }
  // 如果是箱子，尝试推动
  else if (targetCell == SOKOBAN_BOX || targetCell == SOKOBAN_BOX_ON_TARGET) {
    int boxNewX = newX + dx;
    int boxNewY = newY + dy;
    
    if (boxNewX < 0 || boxNewX >= SOKOBAN_COLS || boxNewY < 0 || boxNewY >= SOKOBAN_ROWS) {
      return;
    }
    
    int boxTargetCell = sokobanMap[boxNewY][boxNewX];
    
    // 箱子后面是墙或另一个箱子，不能推动
    if (boxTargetCell == SOKOBAN_WALL || boxTargetCell == SOKOBAN_BOX || 
        boxTargetCell == SOKOBAN_BOX_ON_TARGET) {
      return;
    }
    
    // 记录旧位置用于局部刷新
    lastPlayerX = playerX;
    lastPlayerY = playerY;
    lastBoxX = newX;
    lastBoxY = newY;
    lastBoxNewX = boxNewX;
    lastBoxNewY = boxNewY;
    sokobanBoxMoved = true;
    
    // 移动箱子
    if (targetCell == SOKOBAN_BOX_ON_TARGET) {
      sokobanMap[newY][newX] = SOKOBAN_TARGET;
    } else {
      sokobanMap[newY][newX] = SOKOBAN_EMPTY;
    }
    
    if (boxTargetCell == SOKOBAN_TARGET) {
      sokobanMap[boxNewY][boxNewX] = SOKOBAN_BOX_ON_TARGET;
    } else {
      sokobanMap[boxNewY][boxNewX] = SOKOBAN_BOX;
    }
    
    // 移动玩家
    if (sokobanMap[playerY][playerX] == SOKOBAN_PLAYER_ON_TARGET) {
      sokobanMap[playerY][playerX] = SOKOBAN_TARGET;
    } else {
      sokobanMap[playerY][playerX] = SOKOBAN_EMPTY;
    }
    
    playerX = newX;
    playerY = newY;
    if (sokobanMap[playerY][playerX] == SOKOBAN_TARGET) {
      sokobanMap[playerY][playerX] = SOKOBAN_PLAYER_ON_TARGET;
    } else {
      sokobanMap[playerY][playerX] = SOKOBAN_PLAYER;
    }
  }
  
  drawSokobanGame();
  checkSokobanComplete();
}

void checkSokobanComplete() {
  bool allTargetsCovered = true;
  
  for (int y = 0; y < SOKOBAN_ROWS; y++) {
    for (int x = 0; x < SOKOBAN_COLS; x++) {
      if (sokobanMap[y][x] == SOKOBAN_TARGET) {
        allTargetsCovered = false;
        break;
      }
    }
  }
  
  if (allTargetsCovered && !sokobanComplete) {
    sokobanComplete = true;
    display.fillScreen(TFT_BLACK);
    display.setTextColor(TFT_GREEN);
    display.setTextSize(3);
    display.drawCentreString("Level Complete!", 160, 100, 1);
    display.setTextColor(TFT_WHITE);
    display.setTextSize(2);
    display.drawCentreString("Press for Next", 160, 150, 1);
  }
}

void updateSokoban() {
  if (sokobanComplete) {
    int dirX, dirY;
    bool press;
    if (readJoystick(dirX, dirY, press) && press) {
      sokobanLevel++;
      if (sokobanLevel >= 3) {
        sokobanLevel = 0;
      }
      loadSokobanLevel();
    }
    return;
  }
  
  // 检查顶部按键
  TopKey topKey = readTopButtons();
  if (topKey == KEY_A_PAUSE) {
    gamePaused = !gamePaused;
    if (gamePaused) {
      drawPauseScreen();
    } else {
      sokobanFirstDraw = true;  // 重置首绘，全屏重绘清除暂停框
      drawSokobanGame();
    }
    return;
  }
  if (topKey == KEY_B_RESTART) {
    loadSokobanLevel();  // loadSokobanLevel 已经设置 sokobanFirstDraw = true
    return;
  }
  if (topKey == KEY_C_MENU) {
    currentGame = MENU;
    gamePaused = false;
    sokobanComplete = false;
    drawMenu();
    return;
  }
  
  if (gamePaused) return;
  
  int dirX, dirY;
  bool press;
  if (readJoystick(dirX, dirY, press)) {
    if (dirX != 0 || dirY != 0) {
      moveSokobanPlayer(dirX, dirY);
    }
  }
}

// ========== 俄罗斯方块 ==========
void initTetris() {
  for (int x = 0; x < TETRIS_COLS; x++) {
    for (int y = 0; y < TETRIS_ROWS; y++) {
      tetrisGrid[x][y] = 0;
    }
  }
  tetrisScore = 0;
  tetrisGameOver = false;
  tetrisGameOverDrawn = false;
  tetrisFirstDraw = true;  // 重置首绘标志
  lastPieceX = -1; lastPieceY = -1;  // 清除上次方块记录
  spawnTetrisPiece();
  tetrisLastDrop = millis();
}

void spawnTetrisPiece() {
  currentPieceType = random(7);
  currentPieceX = 3;
  currentPieceY = 0;
  
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      currentPiece[y][x] = pieces[currentPieceType][y][x];
    }
  }
  
  // 检查游戏结束
  if (checkTetrisCollision(currentPieceX, currentPieceY)) {
    tetrisGameOver = true;
  }
}

bool checkTetrisCollision(int px, int py) {
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (currentPiece[y][x]) {
        int gridX = px + x;
        int gridY = py + y;
        
        if (gridX < 0 || gridX >= TETRIS_COLS || gridY >= TETRIS_ROWS) {
          return true;
        }
        
        if (gridY >= 0 && tetrisGrid[gridX][gridY]) {
          return true;
        }
      }
    }
  }
  return false;
}

void lockTetrisPiece() {
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (currentPiece[y][x]) {
        int gridX = currentPieceX + x;
        int gridY = currentPieceY + y;
        if (gridY >= 0) {
          tetrisGrid[gridX][gridY] = currentPieceType + 1;
        }
      }
    }
  }
  
  // 检查完整行
  int linesCleared = 0;
  for (int y = TETRIS_ROWS - 1; y >= 0; y--) {
    bool fullLine = true;
    for (int x = 0; x < TETRIS_COLS; x++) {
      if (!tetrisGrid[x][y]) {
        fullLine = false;
        break;
      }
    }
    
    if (fullLine) {
      linesCleared++;
      // 下移上面的行
      for (int moveY = y; moveY > 0; moveY--) {
        for (int x = 0; x < TETRIS_COLS; x++) {
          tetrisGrid[x][moveY] = tetrisGrid[x][moveY - 1];
        }
      }
      for (int x = 0; x < TETRIS_COLS; x++) {
        tetrisGrid[x][0] = 0;
      }
      y++;
    }
  }
  
  tetrisScore += linesCleared * 100;
  spawnTetrisPiece();
  
  // 方块锁定后需要全屏重绘（新方块+已固定的方块）
  tetrisFirstDraw = true;
}

void rotateTetrisPiece() {
  int rotated[4][4];
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      rotated[x][3 - y] = currentPiece[y][x];
    }
  }
  
  int oldPiece[4][4];
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      oldPiece[y][x] = currentPiece[y][x];
    }
  }
  
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      currentPiece[y][x] = rotated[y][x];
    }
  }
  
  if (checkTetrisCollision(currentPieceX, currentPieceY)) {
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        currentPiece[y][x] = oldPiece[y][x];
      }
    }
  }
}

void drawTetrisGame() {
  if (tetrisFirstDraw) {
    display.fillScreen(TFT_BLACK);
    // 绘制边框
    display.drawRect(TETRIS_OFFSET_X - 2, TETRIS_OFFSET_Y - 2, 
                     TETRIS_COLS * TETRIS_CELL + 4, 
                     TETRIS_ROWS * TETRIS_CELL + 4, TFT_WHITE);
    
    // 首次绘制：绘制所有已固定的方块
    for (int y = 0; y < TETRIS_ROWS; y++) {
      for (int x = 0; x < TETRIS_COLS; x++) {
        if (tetrisGrid[x][y]) {
          display.fillRect(TETRIS_OFFSET_X + x * TETRIS_CELL, 
                           TETRIS_OFFSET_Y + y * TETRIS_CELL,
                           TETRIS_CELL - 1, TETRIS_CELL - 1,
                           pieceColors[tetrisGrid[x][y] - 1]);
        }
      }
    }
    
    // 绘制分数（放在右侧）
    display.setTextColor(TFT_WHITE);
    display.setTextSize(2);
    display.drawString("Score:", 230, 30, 1);
    display.drawCentreString(String(tetrisScore), 260, 60, 1);
    lastTetrisScore = tetrisScore;
    
    // 记录当前方块形状和位置
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        lastPieceShape[y][x] = currentPiece[y][x];
      }
    }
    lastPieceX = currentPieceX;
    lastPieceY = currentPieceY;
    
    tetrisFirstDraw = false;
  }
  
  // 清除旧方块位置（使用记录的旧形状）
  if (lastPieceX >= 0 && lastPieceY >= 0) {
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        if (lastPieceShape[y][x]) {
          int px = TETRIS_OFFSET_X + (lastPieceX + x) * TETRIS_CELL;
          int py = TETRIS_OFFSET_Y + (lastPieceY + y) * TETRIS_CELL;
          if (lastPieceY + y >= 0) {
            display.fillRect(px, py, TETRIS_CELL - 1, TETRIS_CELL - 1, TFT_BLACK);
          }
        }
      }
    }
  }
  
  // 绘制当前方块
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (currentPiece[y][x]) {
        int px = TETRIS_OFFSET_X + (currentPieceX + x) * TETRIS_CELL;
        int py = TETRIS_OFFSET_Y + (currentPieceY + y) * TETRIS_CELL;
        if (currentPieceY + y >= 0) {
          display.fillRect(px, py, TETRIS_CELL - 1, TETRIS_CELL - 1,
                           pieceColors[currentPieceType]);
        }
      }
    }
  }
  
  // 更新记录位置和形状
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      lastPieceShape[y][x] = currentPiece[y][x];
    }
  }
  lastPieceX = currentPieceX;
  lastPieceY = currentPieceY;
  
  // 分数（只在变化时刷新）
  if (tetrisScore != lastTetrisScore) {
    display.fillRect(230, 50, 80, 25, TFT_BLACK);
    display.setTextColor(TFT_WHITE);
    display.setTextSize(2);
    display.drawCentreString(String(tetrisScore), 260, 60, 1);
    lastTetrisScore = tetrisScore;
  }
}

void updateTetris() {
  if (tetrisGameOver) {
    if (!tetrisGameOverDrawn) {
      display.fillScreen(TFT_BLACK);
      display.setTextColor(TFT_RED);
      display.setTextSize(3);
      display.drawCentreString("Game Over!", 160, 100, 1);
      display.setTextColor(TFT_WHITE);
      display.setTextSize(2);
      display.drawCentreString("Score: " + String(tetrisScore), 160, 140, 1);
      display.setTextColor(TFT_YELLOW);
      display.drawCentreString("Press to Menu", 160, 180, 1);
      tetrisGameOverDrawn = true;
    }
    
    int dirX, dirY;
    bool press;
    if (readJoystick(dirX, dirY, press) && press) {
      currentGame = MENU;
      gamePaused = false;
      drawMenu();
    }
    return;
  }
  
  // 检查顶部按键
  TopKey topKey = readTopButtons();
  if (topKey == KEY_A_PAUSE) {
    gamePaused = !gamePaused;
    if (gamePaused) {
      drawPauseScreen();
    } else {
      tetrisFirstDraw = true;  // 重置首绘，全屏重绘清除暂停框
      drawTetrisGame();
    }
    return;
  }
  if (topKey == KEY_B_RESTART) {
    initTetris();  // initTetris 已经设置 tetrisFirstDraw = true
    drawTetrisGame();
    return;
  }
  if (topKey == KEY_C_MENU) {
    currentGame = MENU;
    gamePaused = false;
    drawMenu();
    return;
  }
  
  if (gamePaused) return;
  
  bool pieceMoved = false;
  int dirX, dirY;
  bool press;
  if (readJoystick(dirX, dirY, press)) {
    if (dirX == -1 && !checkTetrisCollision(currentPieceX - 1, currentPieceY)) {
      currentPieceX--;
      pieceMoved = true;
    }
    if (dirX == 1 && !checkTetrisCollision(currentPieceX + 1, currentPieceY)) {
      currentPieceX++;
      pieceMoved = true;
    }
    if (dirY == 1 && !checkTetrisCollision(currentPieceX, currentPieceY + 1)) {
      currentPieceY++;
      pieceMoved = true;
    }
    if (press) {
      rotateTetrisPiece();
      pieceMoved = true;
    }
  }
  
  if (millis() - tetrisLastDrop >= TETRIS_SPEED) {
    tetrisLastDrop = millis();
    if (!checkTetrisCollision(currentPieceX, currentPieceY + 1)) {
      currentPieceY++;
      pieceMoved = true;
    } else {
      lockTetrisPiece();
      pieceMoved = true;
    }
  }
  
  if (pieceMoved || tetrisFirstDraw) {
    drawTetrisGame();
  }
}

// ========== 打砖块游戏 ==========
const int BRICK_ROWS = 5;
const int BRICK_COLS = 8;
const int BRICK_WIDTH = 36;
const int BRICK_HEIGHT = 12;
const int BRICK_PADDING = 2;
const int BRICK_OFFSET_X = 12;
const int BRICK_OFFSET_Y = 30;
const int PADDLE_WIDTH = 60;
const int PADDLE_HEIGHT = 10;
const int PADDLE_Y = 210;
const int BALL_SIZE = 8;

int bricks[BRICK_ROWS][BRICK_COLS];
int paddleX = (320 - PADDLE_WIDTH) / 2;
int lastPaddleX = -1;  // 记录上次挡板位置用于清除
int ballX = 160, ballY = 180;
int lastBallX = 160, lastBallY = 180;  // 记录球的真实旧位置
int ballDX = 2, ballDY = -2;
bool ballLaunched = false;
bool breakoutGameOver = false;
bool breakoutGameOverDrawn = false;
int breakoutScore = 0;
int lastBreakoutScore = -1;  // 记录上次分数，只在变化时刷新
int breakoutLevel = 1;
int lastBreakoutLevel = -1;  // 记录上次关卡，只在变化时刷新
bool breakoutFirstDraw = true;
unsigned long breakoutLastMove = 0;
const int BREAKOUT_SPEED = 50;

const uint16_t brickColors[5] = {TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN};

void initBreakout() {
  breakoutScore = 0;
  breakoutLevel = 1;
  breakoutGameOver = false;
  breakoutGameOverDrawn = false;
  breakoutFirstDraw = true;
  ballLaunched = false;
  loadBreakoutLevel();
}

void loadBreakoutLevel() {
  paddleX = (320 - PADDLE_WIDTH) / 2;
  ballX = 160;
  ballY = PADDLE_Y - BALL_SIZE - 2;
  ballDX = 2;
  ballDY = -2;
  ballLaunched = false;
  breakoutFirstDraw = true;
  
  // 初始化砖块
  for (int y = 0; y < BRICK_ROWS; y++) {
    for (int x = 0; x < BRICK_COLS; x++) {
      bricks[y][x] = 1;  // 1 = 存在
    }
  }
}

void drawBreakoutGame() {
  if (breakoutFirstDraw) {
    display.fillScreen(TFT_BLACK);
    breakoutFirstDraw = false;
    lastPaddleX = paddleX;
    lastBallX = ballX;
    lastBallY = ballY;
    
    // 首次绘制：画所有砖块
    for (int y = 0; y < BRICK_ROWS; y++) {
      for (int x = 0; x < BRICK_COLS; x++) {
        if (bricks[y][x]) {
          int bx = BRICK_OFFSET_X + x * (BRICK_WIDTH + BRICK_PADDING);
          int by = BRICK_OFFSET_Y + y * (BRICK_HEIGHT + BRICK_PADDING);
          display.fillRoundRect(bx, by, BRICK_WIDTH, BRICK_HEIGHT, 3, brickColors[y]);
        }
      }
    }
    
    // 首次绘制：显示分数和关卡
    display.setTextColor(TFT_WHITE);
    display.setTextSize(1);
    display.drawString("Score: " + String(breakoutScore), 5, 5, 1);
    display.drawString("Level: " + String(breakoutLevel), 260, 5, 1);
    lastBreakoutScore = breakoutScore;
    lastBreakoutLevel = breakoutLevel;
  } else {
    // 局部刷新：用真实旧位置清除球
    display.fillCircle(lastBallX, lastBallY, BALL_SIZE/2, TFT_BLACK);
    // 清除旧挡板
    if (lastPaddleX != paddleX) {
      display.fillRoundRect(lastPaddleX, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, 5, TFT_BLACK);
      lastPaddleX = paddleX;
    }
  }
  
  // 绘制挡板
  display.fillRoundRect(paddleX, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, 5, TFT_WHITE);
  
  // 绘制球
  display.fillCircle(ballX, ballY, BALL_SIZE/2, TFT_WHITE);
  
  // 分数和关卡（只在变化时刷新）
  if (breakoutScore != lastBreakoutScore) {
    display.fillRect(0, 0, 120, 18, TFT_BLACK);
    display.setTextColor(TFT_WHITE);
    display.setTextSize(1);
    display.drawString("Score: " + String(breakoutScore), 5, 5, 1);
    lastBreakoutScore = breakoutScore;
  }
  if (breakoutLevel != lastBreakoutLevel) {
    display.fillRect(240, 0, 80, 18, TFT_BLACK);
    display.setTextColor(TFT_WHITE);
    display.setTextSize(1);
    display.drawString("Level: " + String(breakoutLevel), 260, 5, 1);
    lastBreakoutLevel = breakoutLevel;
  }
  
  if (!ballLaunched) {
    display.setTextColor(TFT_YELLOW);
    display.setTextSize(1);
    display.drawCentreString("Press to Launch", 160, 180, 1);
  }
}

void updateBreakout() {
  if (breakoutGameOver) {
    if (!breakoutGameOverDrawn) {
      display.fillScreen(TFT_BLACK);
      display.setTextColor(TFT_RED);
      display.setTextSize(3);
      display.drawCentreString("Game Over!", 160, 100, 1);
      display.setTextColor(TFT_WHITE);
      display.setTextSize(2);
      display.drawCentreString("Score: " + String(breakoutScore), 160, 140, 1);
      display.setTextColor(TFT_YELLOW);
      display.drawCentreString("Press to Menu", 160, 180, 1);
      breakoutGameOverDrawn = true;
    }
    
    int dirX, dirY;
    bool press;
    if (readJoystick(dirX, dirY, press) && press) {
      currentGame = MENU;
      gamePaused = false;
      drawMenu();
    }
    return;
  }
  
  // 检查顶部按键
  TopKey topKey = readTopButtons();
  if (topKey == KEY_A_PAUSE) {
    gamePaused = !gamePaused;
    if (gamePaused) {
      drawPauseScreen();
    } else {
      breakoutFirstDraw = true;
      drawBreakoutGame();
    }
    return;
  }
  if (topKey == KEY_B_RESTART) {
    initBreakout();
    drawBreakoutGame();
    return;
  }
  if (topKey == KEY_C_MENU) {
    currentGame = MENU;
    gamePaused = false;
    drawMenu();
    return;
  }
  
  if (gamePaused) return;
  
  // 首次绘制
  if (breakoutFirstDraw) {
    drawBreakoutGame();
    return;
  }
  
  int dirX, dirY;
  bool press;
  if (readJoystick(dirX, dirY, press)) {
    if (dirX == -1) {
      paddleX = max(0, paddleX - 8);
    }
    if (dirX == 1) {
      paddleX = min(320 - PADDLE_WIDTH, paddleX + 8);
    }
    if (press && !ballLaunched) {
      ballLaunched = true;
    }
  }
  
  if (!ballLaunched) {
    ballX = paddleX + PADDLE_WIDTH / 2;
    ballY = PADDLE_Y - BALL_SIZE - 2;
    drawBreakoutGame();
    return;
  }
  
  if (millis() - breakoutLastMove < BREAKOUT_SPEED) {
    drawBreakoutGame();
    return;
  }
  breakoutLastMove = millis();
  
  // 保存旧位置
  lastBallX = ballX;
  lastBallY = ballY;
  
  // 移动球
  ballX += ballDX;
  ballY += ballDY;
  
  // 左右墙壁碰撞
  if (ballX <= BALL_SIZE/2 || ballX >= 320 - BALL_SIZE/2) {
    ballDX = -ballDX;
  }
  
  // 顶部碰撞
  if (ballY <= BALL_SIZE/2) {
    ballDY = -ballDY;
  }
  
  // 底部检测
  if (ballY >= 240) {
    breakoutGameOver = true;
    return;
  }
  
  // 挡板碰撞
  if (ballY + BALL_SIZE/2 >= PADDLE_Y && 
      ballY - BALL_SIZE/2 <= PADDLE_Y + PADDLE_HEIGHT &&
      ballX >= paddleX && ballX <= paddleX + PADDLE_WIDTH &&
      ballDY > 0) {
    ballDY = -ballDY;
    // 根据击中位置改变角度
    int hitPos = (ballX - paddleX) * 100 / PADDLE_WIDTH;
    if (hitPos < 25) ballDX = -3;
    else if (hitPos < 40) ballDX = -2;
    else if (hitPos < 60) ballDX = 0;
    else if (hitPos < 75) ballDX = 2;
    else ballDX = 3;
  }
  
  // 砖块碰撞
  for (int y = 0; y < BRICK_ROWS; y++) {
    for (int x = 0; x < BRICK_COLS; x++) {
      if (bricks[y][x]) {
        int bx = BRICK_OFFSET_X + x * (BRICK_WIDTH + BRICK_PADDING);
        int by = BRICK_OFFSET_Y + y * (BRICK_HEIGHT + BRICK_PADDING);
        
        if (ballX >= bx && ballX <= bx + BRICK_WIDTH &&
            ballY >= by && ballY <= by + BRICK_HEIGHT) {
          bricks[y][x] = 0;
          // 立即清除屏幕上的砖块（整块清除）
          display.fillRoundRect(bx, by, BRICK_WIDTH, BRICK_HEIGHT, 3, TFT_BLACK);
          ballDY = -ballDY;
          breakoutScore += 10;
        }
      }
    }
  }
  
  // 检查是否全部清除
  bool allCleared = true;
  for (int y = 0; y < BRICK_ROWS; y++) {
    for (int x = 0; x < BRICK_COLS; x++) {
      if (bricks[y][x]) {
        allCleared = false;
        break;
      }
    }
  }
  
  if (allCleared) {
    breakoutLevel++;
    loadBreakoutLevel();
    return;
  }
  
  drawBreakoutGame();
}

// ========== 主程序 ==========
void setup() {
  SerialDebug.begin(115200);
  while (!SerialDebug) {
    ;
  }
  delay(1000);
  
  SerialDebug.println("=== WIO Game Console ===");
  
  // 初始化屏幕（使用 Seeed_GFX）
  if (!display.begin()) {
    SerialDebug.println(display.lastResult().message);
    return;
  }
  
  display.fillScreen(TFT_BLACK);
  display.fillRoundRect(20, 45, 280, 150, 12, TFT_DARKGREEN);
  display.setTextColor(TFT_WHITE);
  display.setTextSize(3);
  display.drawCentreString("Wio Terminal", 160, 85, 1);
  display.setTextSize(1);
  display.drawCentreString("Game Console Loading...", 160, 135, 1);
  
  SerialDebug.println("Display initialized");
  
  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_LEFT, INPUT_PULLUP);
  pinMode(PIN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_PRESS, INPUT_PULLUP);
  
  // 配置顶部按键
  pinMode(PIN_KEY_A, INPUT_PULLUP);
  pinMode(PIN_KEY_B, INPUT_PULLUP);
  pinMode(PIN_KEY_C, INPUT_PULLUP);
  
  SerialDebug.println("Setup complete!");
  delay(1000);
  
  drawMenu();
  SerialDebug.println("Menu drawn!");
}

// ========== 传感器工具菜单 ==========
void drawSensorMenuItem(int index, bool selected) {
  int y = 60 + index * 35;
  uint16_t sensorColors[] = {TFT_GREEN, TFT_CYAN, TFT_RED, TFT_MAGENTA, TFT_ORANGE};
  
  if (selected) {
    display.fillRoundRect(30, y, 260, 30, 8, TFT_BLUE);
    display.fillRoundRect(40, y + 5, 20, 20, 5, sensorColors[index]);
    display.setTextColor(TFT_WHITE);
    display.setTextSize(2);
    display.drawString(sensorItems[index], 70, y + 7, 1);
  } else {
    display.fillRoundRect(30, y, 260, 30, 8, TFT_DARKGREY);
    display.fillRoundRect(40, y + 5, 20, 20, 5, sensorColors[index]);
    display.setTextColor(TFT_LIGHTGREY);
    display.setTextSize(2);
    display.drawString(sensorItems[index], 70, y + 7, 1);
  }
}

void drawSensorMenu() {
  display.fillScreen(TFT_BLACK);
  
  // 标题
  display.setTextColor(TFT_YELLOW);
  display.setTextSize(2);
  display.drawCentreString("Sensor Tools", 160, 20, 1);
  
  // 分隔线
  display.drawFastHLine(20, 45, 280, TFT_DARKGREY);
  
  // 绘制传感器列表
  for (int i = 0; i < sensorCount; i++) {
    drawSensorMenuItem(i, i == sensorSelection);
  }
  
  // 底部提示
  display.setTextColor(TFT_DARKGREY);
  display.setTextSize(1);
  display.drawCentreString("UP/DOWN: Select | PRESS: Open | C: Back", 160, 230, 1);
  
  sensorFirstDraw = false;
}

void drawLightSensor() {
  if (sensorFirstDraw) {
    display.fillScreen(TFT_BLACK);
    sensorFirstDraw = false;
    
    // 首次绘制：标题和边框
    display.setTextColor(TFT_YELLOW);
    display.setTextSize(2);
    display.drawCentreString("Light Sensor", 160, 30, 1);
    display.drawRoundRect(60, 70, 200, 100, 10, TFT_GREEN);
    display.setTextSize(1);
    display.drawCentreString("(0-1024)", 160, 140, 1);
    display.setTextColor(TFT_DARKGREY);
    display.setTextSize(1);
    display.drawCentreString("Press C to Back", 160, 220, 1);
    lastLightValue = -1;  // 重置记录值
    lastSensorUpdate = 0;
  }
  
  // 控制刷新频率
  if (millis() - lastSensorUpdate < SENSOR_UPDATE_INTERVAL) return;
  lastSensorUpdate = millis();
  
  int lightValue = analogRead(WIO_LIGHT);
  
  // 只在数值变化时刷新
  if (lightValue != lastLightValue) {
    // 清除旧数值区域
    display.fillRect(100, 90, 120, 40, TFT_BLACK);
    display.setTextColor(TFT_WHITE);
    display.setTextSize(3);
    display.drawCentreString(String(lightValue), 160, 100, 1);
    lastLightValue = lightValue;
  }
}

void drawAccelerometer() {
  if (sensorFirstDraw) {
    display.fillScreen(TFT_BLACK);
    Wire1.begin();
    sensorFirstDraw = false;
    
    // 首次绘制：标题
    display.setTextColor(TFT_YELLOW);
    display.setTextSize(2);
    display.drawCentreString("Accelerometer", 160, 20, 1);
    display.setTextColor(TFT_DARKGREY);
    display.setTextSize(1);
    display.drawCentreString("Press C to Back", 160, 220, 1);
    lastAccelX = -9999; lastAccelY = -9999; lastAccelZ = -9999;  // 重置记录值
  }
  
  // 直接读取加速度计原始数据 (LIS3DHTR I2C地址 0x19)
  Wire1.beginTransmission(0x19);
  Wire1.write(0x28);  // OUT_X_L
  Wire1.endTransmission(false);
  Wire1.requestFrom(0x19, 6);
  
  int16_t x = Wire1.read() | (Wire1.read() << 8);
  int16_t y = Wire1.read() | (Wire1.read() << 8);
  int16_t z = Wire1.read() | (Wire1.read() << 8);
  
  // 只在数值变化时刷新
  if (x != lastAccelX) {
    display.fillRect(40, 65, 150, 25, TFT_BLACK);
    display.setTextColor(TFT_RED);
    display.setTextSize(2);
    display.drawString("X: " + String(x), 40, 70, 1);
    lastAccelX = x;
  }
  if (y != lastAccelY) {
    display.fillRect(40, 105, 150, 25, TFT_BLACK);
    display.setTextColor(TFT_GREEN);
    display.setTextSize(2);
    display.drawString("Y: " + String(y), 40, 110, 1);
    lastAccelY = y;
  }
  if (z != lastAccelZ) {
    display.fillRect(40, 145, 150, 25, TFT_BLACK);
    display.setTextColor(TFT_BLUE);
    display.setTextSize(2);
    display.drawString("Z: " + String(z), 40, 150, 1);
    lastAccelZ = z;
  }
}

void drawIREmitter() {
  if (sensorFirstDraw) {
    display.fillScreen(TFT_BLACK);
    pinMode(WIO_IR, OUTPUT);
    sensorFirstDraw = false;
    
    display.setTextColor(TFT_RED);
    display.setTextSize(2);
    display.drawCentreString("IR Emitter", 160, 50, 1);
    display.drawRoundRect(80, 100, 160, 60, 10, TFT_RED);
    display.setTextColor(TFT_WHITE);
    display.setTextSize(2);
    display.drawCentreString("IR: ON", 160, 120, 1);
    display.setTextColor(TFT_DARKGREY);
    display.setTextSize(1);
    display.drawCentreString("Press C to Back", 160, 220, 1);
  }
  
  digitalWrite(WIO_IR, HIGH);
}

void drawBuzzer() {
  if (sensorFirstDraw) {
    display.fillScreen(TFT_BLACK);
    pinMode(WIO_BUZZER, OUTPUT);
    sensorFirstDraw = false;
    
    // 标题
    display.setTextColor(TFT_MAGENTA);
    display.setTextSize(2);
    display.drawCentreString("Buzzer Piano", 160, 15, 1);
    
    // 绘制 5 个音符按钮
    const char* noteNames[] = {"Do", "Re", "Mi", "Fa", "Sol"};
    uint16_t noteColors[] = {TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN};
    
    for (int i = 0; i < 5; i++) {
      int x = 30 + (i % 3) * 90;
      int y = 50 + (i / 3) * 55;
      display.fillRoundRect(x, y, 80, 45, 8, noteColors[i]);
      display.setTextColor(TFT_WHITE);
      display.setTextSize(2);
      display.drawCentreString(noteNames[i], x + 40, y + 15, 1);
    }
    
    display.setTextColor(TFT_DARKGREY);
    display.setTextSize(1);
    display.drawCentreString("Joystick/Press to play | C: Back", 160, 170, 1);
  }
  
  // 检测摇杆和按键
  int dirX, dirY;
  bool press;
  if (readJoystick(dirX, dirY, press)) {
    int noteIndex = -1;
    int freq = 0;
    
    if (dirY == -1) { noteIndex = 0; freq = 523; }      // Do (C5) - 高频
    else if (dirY == 1) { noteIndex = 1; freq = 659; }   // Re (D5)
    else if (dirX == -1) { noteIndex = 2; freq = 784; }  // Mi (E5)
    else if (dirX == 1) { noteIndex = 3; freq = 880; }   // Fa (F5)
    else if (press) { noteIndex = 4; freq = 988; }       // Sol (G5)
    
    if (noteIndex != -1) {
      // 使用 tone() 播放特定频率
      tone(WIO_BUZZER, freq, 200);
      
      // 高亮按钮
      int x = 30 + (noteIndex % 3) * 90;
      int y = 50 + (noteIndex / 3) * 55;
      display.fillRoundRect(x, y, 80, 45, 8, TFT_WHITE);
      display.setTextColor(TFT_BLACK);
      display.setTextSize(2);
      const char* noteNames[] = {"Do", "Re", "Mi", "Fa", "Sol"};
      display.drawCentreString(noteNames[noteIndex], x + 40, y + 15, 1);
      
      delay(200);
      noTone(WIO_BUZZER);
      
      // 恢复按钮颜色
      uint16_t noteColors[] = {TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN};
      display.fillRoundRect(x, y, 80, 45, 8, noteColors[noteIndex]);
      display.setTextColor(TFT_WHITE);
      display.drawCentreString(noteNames[noteIndex], x + 40, y + 15, 1);
    }
  }
}

void drawMicrophone() {
  if (sensorFirstDraw) {
    display.fillScreen(TFT_BLACK);
    sensorFirstDraw = false;
    
    // 首次绘制：标题和边框
    display.setTextColor(TFT_ORANGE);
    display.setTextSize(2);
    display.drawCentreString("Microphone", 160, 30, 1);
    display.drawRoundRect(60, 70, 200, 100, 10, TFT_ORANGE);
    display.setTextSize(1);
    display.drawCentreString("(0-1024)", 160, 140, 1);
    display.setTextColor(TFT_DARKGREY);
    display.setTextSize(1);
    display.drawCentreString("Press C to Back", 160, 220, 1);
    lastMicValue = -1;  // 重置记录值
    lastSensorUpdate = 0;
  }
  
  // 控制刷新频率
  if (millis() - lastSensorUpdate < SENSOR_UPDATE_INTERVAL) return;
  lastSensorUpdate = millis();
  
  int micValue = analogRead(WIO_MIC);
  
  // 只在数值变化时刷新
  if (micValue != lastMicValue) {
    // 清除旧数值区域
    display.fillRect(100, 90, 120, 40, TFT_BLACK);
    display.setTextColor(TFT_WHITE);
    display.setTextSize(3);
    display.drawCentreString(String(micValue), 160, 100, 1);
    lastMicValue = micValue;
  }
}

void updateSensorMenu() {
  // 检查返回按键 - 只返回一级
  TopKey topKey = readTopButtons();
  if (topKey == KEY_C_MENU) {
    if (currentSensor != NO_SENSOR) {
      // 从传感器页面返回传感器菜单
      currentSensor = NO_SENSOR;
      sensorFirstDraw = true;
      sensorSelection = 0;
      // 关闭蜂鸣器
      analogWrite(WIO_BUZZER, 0);
      pinMode(WIO_IR, INPUT);
    } else {
      // 从传感器菜单返回主菜单
      currentGame = MENU;
      sensorFirstDraw = true;
      sensorSelection = 0;
      drawMenu();
    }
    return;
  }
  
  // 如果当前没有选择传感器，显示菜单
  if (currentSensor == NO_SENSOR) {
    if (sensorFirstDraw) {
      drawSensorMenu();
    }
    
    int oldSelection = sensorSelection;
    int dirX, dirY;
    bool press;
    if (readJoystick(dirX, dirY, press)) {
      if (dirY == -1 && sensorSelection > 0) {
        sensorSelection--;
      }
      if (dirY == 1 && sensorSelection < sensorCount - 1) {
        sensorSelection++;
      }
      if (press) {
        // 选择传感器
        sensorFirstDraw = true;
        switch (sensorSelection) {
          case 0: currentSensor = LIGHT_SENSOR; break;
          case 1: currentSensor = ACCELEROMETER; break;
          case 2: currentSensor = IR_SENSOR; break;
          case 3: currentSensor = BUZZER_TOOL; break;
          case 4: currentSensor = MIC_SENSOR; break;
        }
        return;
      }
    }
    
    // 局部刷新：只更新选中项
    if (sensorSelection != oldSelection) {
      drawSensorMenuItem(oldSelection, false);
      drawSensorMenuItem(sensorSelection, true);
    }
    return;
  }
  
  // 根据当前传感器类型更新
  switch (currentSensor) {
    case LIGHT_SENSOR:
      drawLightSensor();
      break;
    case ACCELEROMETER:
      drawAccelerometer();
      break;
    case IR_SENSOR:
      drawIREmitter();
      break;
    case BUZZER_TOOL:
      drawBuzzer();
      break;
    case MIC_SENSOR:
      drawMicrophone();
      break;
    default:
      break;
  }
}

void loop() {
  switch (currentGame) {
    case MENU:
      updateMenuSelection();
      break;
    case SNAKE:
      updateSnake();
      break;
    case SOKOBAN:
      updateSokoban();
      break;
    case TETRIS:
      updateTetris();
      break;
    case BREAKOUT:
      updateBreakout();
      break;
    case TOOLS:
      updateSensorMenu();
      break;
  }
}